/* Copyright (c) 2002-2012 Croteam Ltd. 
This program is free software; you can redistribute it and/or modify
it under the terms of version 2 of the GNU General Public License as published by
the Free Software Foundation


This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#include <Engine/StdH.h>

#include <Engine/Graphics/DrawPort.h>

#include <Engine/Base/Statistics_Internal.h>
#include <Engine/Base/Console.h>
#include <Engine/Math/Projection.h>
#include <Engine/Graphics/RenderScene.h>
#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/GfxLibrary.h>
#include <Engine/Graphics/GfxProfile.h>
#include <Engine/Graphics/ShadowMap.h>
#include <Engine/Graphics/Fog_internal.h>
#include <Engine/Brushes/Brush.h>
#include <Engine/Brushes/BrushTransformed.h>

#include <Engine/Templates/StaticStackArray.cpp>

// asm shortcuts
#define O offset
#define Q qword ptr
#define D dword ptr
#define W  word ptr
#define B  byte ptr

#define MAXTEXUNITS   4
#define SHADOWTEXTURE 3

extern INDEX wld_bShowTriangles;
extern INDEX wld_bShowDetailTextures;
extern INDEX wld_bRenderShadowMaps;
extern INDEX wld_bRenderTextures;
extern INDEX wld_bRenderDetailPolygons;
extern INDEX wld_iDetailRemovingBias;
extern INDEX wld_bAccurateColors;

extern INDEX gfx_bRenderWorld;
extern INDEX shd_iForceFlats;
extern INDEX shd_bShowFlats;

extern BOOL _bMultiPlayer;
extern BOOL CVA_bWorld;
static GfxAPIType eAPI;


// vertex coordinates and elements used by one pass of polygons
static CStaticStackArray<GFXVertex>   _avtxPass;   
static CStaticStackArray<GFXTexCoord> _atexPass[MAXTEXUNITS];
static CStaticStackArray<GFXColor>    _acolPass;   
static CStaticStackArray<INDEX_T>       _aiElements;
// general coordinate stack referenced by the scene polygons
CStaticStackArray<GFXVertex3> _avtxScene;

// group flags (single-texturing)
#define GF_TX0  (1L<<0) 
#define GF_TX1  (1L<<1)
#define GF_TX2  (1L<<2)
#define GF_SHD  (1L<<3)
#define GF_FLAT (1L<<4)  // flat fill instead of texture 1
#define GF_TA1  (1L<<5)  // texture 2 after shade
#define GF_TA2  (1L<<6)  // texture 3 after shade
#define GF_FOG  (1L<<7)
#define GF_HAZE (1L<<8)
#define GF_SEL  (1L<<9)
#define GF_KEY  (1L<<10) // first layer requires alpha-keying

// texture combinations for max 4 texture units (fog, haze and selection not included)
#define GF_TX0_TX1         (1L<<11)
#define GF_TX0_TX2         (1L<<12)
#define GF_TX0_SHD         (1L<<13)
#define GF_TX2_SHD         (1L<<14)  // second pass
#define GF_TX0_TX1_TX2     (1L<<15)
#define GF_TX0_TX1_SHD     (1L<<16)
#define GF_TX0_TX2_SHD     (1L<<17)
#define GF_TX0_TX1_TX2_SHD (1L<<18)

// total number of groups
#define GROUPS_MAXCOUNT (1L<<11)   // max group +1 !
#define GROUPS_MINCOUNT (1L<<4)-1  // min group !
static ScenePolygon *_apspoGroups[GROUPS_MAXCOUNT];
static INDEX _ctGroupsCount=0;


// some static vars

static FLOAT _fHazeMul, _fHazeAdd;
static FLOAT _fFogMul;

static COLOR _colSelection;
static INDEX _ctUsableTexUnits;
static BOOL  _bTranslucentPass;      // rendering translucent polygons

static ULONG _ulLastFlags[MAXTEXUNITS];
static ULONG _ulLastBlends[MAXTEXUNITS];
static INDEX _iLastFrameNo[MAXTEXUNITS];
static CTextureData *_ptdLastTex[MAXTEXUNITS];

static CDrawPort *_pDP;
static CPerspectiveProjection3D *_ppr = NULL;


// draw batched elements
static void FlushElements(void) 
{
  // skip if empty
  const INDEX ctElements = _aiElements.Count();
  if( ctElements<3) return;
  // draw
  const INDEX ctTris = ctElements/3;
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_DRAWELEMENTS);
  _pfGfxProfile.IncrementCounter( CGfxProfile::PCI_RS_TRIANGLEPASSESOPT, ctTris);
  _sfStats.IncrementCounter( CStatForm::SCI_SCENE_TRIANGLEPASSES, ctTris);
  _pGfx->gl_ctWorldTriangles += ctTris; 
  gfxDrawElements( ctElements, &_aiElements[0]);
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_DRAWELEMENTS);
  // reset
  _aiElements.PopAll();
}


// batch elements of one polygon
static
#if (!defined __GNUC__)
__forceinline
#endif
void AddElements( ScenePolygon *pspo)
{
  const INDEX ctElems = pspo->spo_ctElements;
  INDEX_T *piDst = _aiElements.Push(ctElems);

#if (defined __MSVC_INLINE__)
  __asm {
    mov     eax,D [pspo]
    mov     ecx,D [ctElems]
    mov     edi,D [piDst]
    mov     esi,D [eax]ScenePolygon.spo_piElements
    mov     ebx,D [eax]ScenePolygon.spo_iVtx0Pass
    movd    mm1,ebx
    movq    mm0,mm1
    psllq   mm1,32
    por     mm1,mm0
    shr     ecx,1
    jz      elemRest
elemLoop:
    movq    mm0,Q [esi]
    paddd   mm0,mm1
    movq    Q [edi],mm0
    add     esi,8
    add     edi,8
    dec     ecx
    jnz     elemLoop
elemRest:
    emms
    test    [ctElems],1
    jz      elemDone
    mov     eax,D [esi]
    add     eax,ebx
    mov     D [edi],eax
elemDone:
  }
#elif (defined __GNU_INLINE_X86_32__)
  __asm__ __volatile__ (
    "movl    %[ctElems], %%ecx      \n\t"
    "movl    %[piDst], %%edi        \n\t"
    "movl    %[piElements], %%esi   \n\t"
    "movd    %[iVtx0Pass], %%mm1    \n\t"
    "movq    %%mm1, %%mm0           \n\t"
    "psllq   $32, %%mm1             \n\t"
    "por     %%mm0, %%mm1           \n\t"
    "shrl    $1, %%ecx              \n\t"
    "jz      1f                     \n\t" // elemRest
    "0:                             \n\t" // elemLoop
    "movq    (%%esi), %%mm0         \n\t"
    "paddd   %%mm1, %%mm0           \n\t"
    "movq    %%mm0, (%%edi)         \n\t"
    "addl    $8, %%esi              \n\t"
    "addl    $8, %%edi              \n\t"
    "decl    %%ecx                  \n\t"
    "jnz     0b                     \n\t" // elemLoop
    "1:                             \n\t" // elemRest
    "emms                           \n\t"
    "testl   $1, %[ctElems]         \n\t"
    "jz      2f                     \n\t" // elemDone
    "movl    (%%esi), %%eax         \n\t"
    "addl    %[iVtx0Pass], %%eax    \n\t"
    "movl    %%eax, (%%edi)         \n\t"
    "2:                             \n\t" // elemDone
        : // no outputs.
        : [ctElems] "g" (ctElems), [piDst] "g" (piDst),
          [piElements] "g" (pspo->spo_piElements),
          [iVtx0Pass] "g" (pspo->spo_iVtx0Pass)
        : FPU_REGS, "mm0", "mm1", "eax", "ecx", "esi", "edi",
          "cc", "memory"
  );

#else
  const INDEX iVtx0Pass = pspo->spo_iVtx0Pass;
  const INDEX *piSrc = pspo->spo_piElements;
  for( INDEX iElem=0; iElem<ctElems; iElem++) {
    // make an element in per-pass arrays
    piDst[iElem] = piSrc[iElem]+iVtx0Pass;
  }
#endif
}


// draw all elements of one pass
static __forceinline void DrawAllElements( ScenePolygon *pspoFirst) 
{
  ASSERT( _aiElements.Count()==0);
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_DRAWELEMENTS);
  for( ScenePolygon *pspo=pspoFirst; pspo!=NULL; pspo=pspo->spo_pspoSucc) { 
    const INDEX ctTris = pspo->spo_ctElements/3;
    _pfGfxProfile.IncrementCounter( CGfxProfile::PCI_RS_TRIANGLEPASSESOPT, ctTris);
    _sfStats.IncrementCounter( CStatForm::SCI_SCENE_TRIANGLEPASSES, ctTris);
    _pGfx->gl_ctWorldTriangles += ctTris; 
    AddElements(pspo);
  }
  FlushElements();
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_DRAWELEMENTS);
}


// calculate mip factor for a texture and adjust its mapping vectors
static BOOL RSMakeMipFactorAndAdjustMapping( ScenePolygon *pspo, INDEX iLayer)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_MAKEMIPFACTOR);
  BOOL bRemoved = FALSE;
  MEX mexTexSizeU, mexTexSizeV;
  CMappingVectors &mv = pspo->spo_amvMapping[iLayer];

  // texture map ?
  if( iLayer<SHADOWTEXTURE)
  { 
    const ULONG ulBlend = pspo->spo_aubTextureFlags[iLayer] & STXF_BLEND_MASK;
    CTextureData *ptd = (CTextureData*)pspo->spo_aptoTextures[iLayer]->GetData();
    mexTexSizeU = ptd->GetWidth();
    mexTexSizeV = ptd->GetHeight();

    // check whether detail can be rejected (but don't reject colorized textures)
    if( ulBlend==STXF_BLEND_SHADE && (ptd->td_ulFlags&TEX_EQUALIZED)
    && (pspo->spo_acolColors[iLayer]&0xFFFFFF00)==0xFFFFFF00)
    { // get nearest vertex Z distance from viewer and u and v steps
      const FLOAT fZ = pspo->spo_fNearestZ;
      const FLOAT f1oPZ1 = fZ / _ppr->ppr_PerspectiveRatios(1);
      const FLOAT f1oPZ2 = fZ / _ppr->ppr_PerspectiveRatios(2);
      const FLOAT fDUoDI = Abs( mv.mv_vU(1) *f1oPZ1);
      const FLOAT fDUoDJ = Abs( mv.mv_vU(2) *f1oPZ2);
      const FLOAT fDVoDI = Abs( mv.mv_vV(1) *f1oPZ1);
      const FLOAT fDVoDJ = Abs( mv.mv_vV(2) *f1oPZ2);
      // find mip factor and adjust removing of texture layer
      const FLOAT fMaxDoD    = Max( Max(fDUoDI,fDUoDJ), Max(fDVoDI,fDVoDJ));
      const INDEX iMipFactor = wld_iDetailRemovingBias + (((SLONG&)fMaxDoD)>>23) -127 +10;
      const INDEX iLastMip   = ptd->td_iFirstMipLevel + ptd->GetNoOfMips() -1; // determine last mipmap in texture
      bRemoved = (iMipFactor>=iLastMip);
      // check for detail texture showing
      extern INDEX wld_bShowDetailTextures;
      if( wld_bShowDetailTextures) {
        if( iLayer==2) pspo->spo_acolColors[iLayer] = C_MAGENTA|255;
        else           pspo->spo_acolColors[iLayer] = C_CYAN   |255;
      }
    }
    // check if texture has been blended with low alpha 
    else bRemoved = (ulBlend==STXF_BLEND_ALPHA) && ((pspo->spo_acolColors[iLayer]&CT_AMASK)>>CT_ASHIFT)<3;
  }

  // shadow map
  else
  { 
    mexTexSizeU = pspo->spo_psmShadowMap->sm_mexWidth;
    mexTexSizeV = pspo->spo_psmShadowMap->sm_mexHeight;
  }

  // adjust texture gradients
  if( mexTexSizeU!=1024) {
    const FLOAT fMul = 1024.0f /mexTexSizeU;  // (no need to do shift-opt, because it won't speed up much!)
    mv.mv_vU(1) *=fMul;  mv.mv_vU(2) *=fMul;  mv.mv_vU(3) *=fMul;
  }
  if( mexTexSizeV!=1024) {
    const FLOAT fMul = 1024.0f /mexTexSizeV;
    mv.mv_vV(1) *=fMul;  mv.mv_vV(2) *=fMul;  mv.mv_vV(3) *=fMul;
  }

  // all done
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_MAKEMIPFACTOR);
  return bRemoved;
}



// Remove all polygons with no triangles from a list
static void RSRemoveDummyPolygons( ScenePolygon *pspoAll, ScenePolygon **ppspoNonDummy)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_REMOVEDUMMY);
  *ppspoNonDummy = NULL;
  // for all span polygons in list (remember one ahead to be able to reconnect them)
  ScenePolygon *pspoNext;
  for( ScenePolygon *pspoThis=pspoAll; pspoThis!=NULL; pspoThis=pspoNext) {
    pspoNext = pspoThis->spo_pspoSucc;
    // if the polygon has some triangles
    if( pspoThis->spo_ctElements >0) {
      // move it to the other list
      pspoThis->spo_pspoSucc = *ppspoNonDummy;
      *ppspoNonDummy = pspoThis;
    }
  }
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_REMOVEDUMMY);
}


// bin polygons into groups
static void RSBinToGroups( ScenePolygon *pspoFirst)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_BINTOGROUPS);
  // clamp texture layers
  extern INDEX wld_bTextureLayers;
  BOOL bTextureLayer1 =(wld_bTextureLayers /100) || _bMultiPlayer; // must be enabled in multiplayer mode!
  BOOL bTextureLayer2 = wld_bTextureLayers /10 %10;
  BOOL bTextureLayer3 = wld_bTextureLayers %10;
  wld_bTextureLayers  = 0;
  if( bTextureLayer1) wld_bTextureLayers += 100;
  if( bTextureLayer2) wld_bTextureLayers += 10;
  if( bTextureLayer3) wld_bTextureLayers += 1;

  // cache rendering states
  bTextureLayer1 = bTextureLayer1 && wld_bRenderTextures;
  bTextureLayer2 = bTextureLayer2 && wld_bRenderTextures;
  bTextureLayer3 = bTextureLayer3 && wld_bRenderTextures;

  // clear all groups initially
  memset( _apspoGroups, 0, sizeof(_apspoGroups));
  _ctGroupsCount = GROUPS_MINCOUNT;

  // for all span polygons in list (remember one ahead to be able to reconnect them)
  for( ScenePolygon *pspoNext, *pspo=pspoFirst; pspo!=NULL; pspo=pspoNext)
  {
    pspoNext = pspo->spo_pspoSucc;
    const INDEX ctTris = pspo->spo_ctElements/3;
    ULONG ulBits = NONE;

    // if it has texture 1 active
    if( pspo->spo_aptoTextures[0]!=NULL && bTextureLayer1) {
      _pfGfxProfile.IncrementCounter( CGfxProfile::PCI_RS_TRIANGLEPASSESORG, ctTris);
      // prepare mapping for texture 0 and generate its mip factor
      const BOOL bRemoved = RSMakeMipFactorAndAdjustMapping( pspo, 0);
      if( !bRemoved) ulBits |= GF_TX0; // add if not removed
    } else {
      // flat fill is mutually exclusive with texture layer0
      _ctGroupsCount |= GF_FLAT;
      ulBits |= GF_FLAT; 
    }

    // if it has texture 2 active
    if( pspo->spo_aptoTextures[1]!=NULL && bTextureLayer2) {
      _pfGfxProfile.IncrementCounter( CGfxProfile::PCI_RS_TRIANGLEPASSESORG, ctTris);
      // prepare mapping for texture 1 and generate its mip factor
      const BOOL bRemoved = RSMakeMipFactorAndAdjustMapping( pspo, 1);
      if( !bRemoved) { // add if not removed
        if( pspo->spo_aubTextureFlags[1] & STXF_AFTERSHADOW) {
          _ctGroupsCount |= GF_TA1;
          ulBits |= GF_TA1;
        } else {
          ulBits |= GF_TX1;
        }
      }
    }
    // if it has texture 3 active
    if( pspo->spo_aptoTextures[2]!=NULL && bTextureLayer3) {
      _pfGfxProfile.IncrementCounter( CGfxProfile::PCI_RS_TRIANGLEPASSESORG, ctTris);
      // prepare mapping for texture 2 and generate its mip factor
      const BOOL bRemoved = RSMakeMipFactorAndAdjustMapping( pspo, 2);
      if( !bRemoved) { // add if not removed
        if( pspo->spo_aubTextureFlags[2] & STXF_AFTERSHADOW) {
          _ctGroupsCount |= GF_TA2;
          ulBits |= GF_TA2;
        } else {
          ulBits |= GF_TX2;
        }
      }
    }

    // if it has shadowmap active
    if( pspo->spo_psmShadowMap!=NULL && wld_bRenderShadowMaps) {
      _pfGfxProfile.IncrementCounter( CGfxProfile::PCI_RS_TRIANGLEPASSESORG, ctTris);
      // prepare shadow map
      CShadowMap *psmShadow = pspo->spo_psmShadowMap;
      psmShadow->Prepare();
      const BOOL bFlat = psmShadow->IsFlat();
      COLOR colFlat = psmShadow->sm_colFlat & 0xFFFFFF00; 
      const BOOL bOverbright = (colFlat & 0x80808000);
      // only need to update poly color if shadowmap is flat
      if( bFlat) {
        if( !bOverbright || shd_iForceFlats==1) {
          if( shd_bShowFlats) colFlat = C_mdMAGENTA; // show flat shadows?
          else { // enhance light color to emulate overbrighting
            if( !bOverbright) colFlat<<=1;
            else {
              UBYTE ubR,ubG,ubB;
              ColorToRGB( colFlat, ubR,ubG,ubB);
              const ULONG ulR = ClampUp( ((ULONG)ubR)<<1, (ULONG) 255);
              const ULONG ulG = ClampUp( ((ULONG)ubG)<<1, (ULONG) 255);
              const ULONG ulB = ClampUp( ((ULONG)ubB)<<1, (ULONG) 255);
              colFlat = RGBToColor(ulR,ulG,ulB);
            }
          } // mix color in the first texture layer
          COLOR &colTotal = pspo->spo_acolColors[0];
          COLOR  colLayer = pspo->spo_acolColors[3];
          if( colTotal==0xFFFFFFFF) colTotal = colLayer;
          else if( colLayer!=0xFFFFFFFF) colTotal = MulColors( colTotal, colLayer);
          if(  colTotal==0xFFFFFFFF) colTotal = colFlat;
          else colTotal = MulColors( colTotal,  colFlat);
          psmShadow->MarkDrawn();
        }
        else {
          // need to update poly color if shadowmap is flat and overbrightened
          COLOR &colTotal = pspo->spo_acolColors[3];
          if( shd_bShowFlats) colFlat = C_mdBLUE; // overbrightened!
          if(  colTotal==0xFFFFFFFF) colTotal = colFlat;
          else colTotal = MulColors( colTotal,  colFlat);
          ulBits |= GF_SHD; // mark the need for shadow layer
        } 
      } else {
        // prepare mapping for shadowmap and generate its mip factor
        RSMakeMipFactorAndAdjustMapping( pspo, SHADOWTEXTURE);
        ulBits |= GF_SHD;
      }
    }

    // if it has fog active
    if( pspo->spo_ulFlags&SPOF_RENDERFOG) {
      _pfGfxProfile.IncrementCounter( CGfxProfile::PCI_RS_TRIANGLEPASSESORG, ctTris);
      _ctGroupsCount |= GF_FOG;
      ulBits |= GF_FOG;
    }
    // if it has haze active
    if( pspo->spo_ulFlags&SPOF_RENDERHAZE) {
      _pfGfxProfile.IncrementCounter( CGfxProfile::PCI_RS_TRIANGLEPASSESORG, ctTris);
      _ctGroupsCount |= GF_HAZE;
      ulBits |= GF_HAZE;
    }

    // if it is selected
    if( pspo->spo_ulFlags&SPOF_SELECTED) {
      _pfGfxProfile.IncrementCounter( CGfxProfile::PCI_RS_TRIANGLEPASSESORG, ctTris);
      _ctGroupsCount |= GF_SEL;
      ulBits |= GF_SEL;
    }

    // if it is transparent
    if( pspo->spo_ulFlags&SPOF_TRANSPARENT) {
      _pfGfxProfile.IncrementCounter( CGfxProfile::PCI_RS_TRIANGLEPASSESORG, ctTris);
      _ctGroupsCount |= GF_KEY;
      ulBits |= GF_KEY;
    }

    // in case of at least one layer, add it to proper group
    if( ulBits) {
      _pfGfxProfile.IncrementCounter( CGfxProfile::PCI_RS_TRIANGLES, ctTris);
      pspo->spo_pspoSucc   = _apspoGroups[ulBits];
      _apspoGroups[ulBits] = pspo;
    }
  }

  // determine maximum used groups
  ASSERT( _ctGroupsCount);

#if (defined __MSVC_INLINE__)
  __asm {
    mov     eax,2
    bsr     ecx,D [_ctGroupsCount]
    shl     eax,cl
    mov     D [_ctGroupsCount],eax
  }

#elif (defined __GNU_INLINE_X86_32__)
  __asm__ __volatile__ (
    "movl     $2, %%eax          \n\t"
    "bsrl     (%%esi), %%ecx     \n\t"
    "shll     %%cl, %%eax        \n\t"
    "movl     %%eax, (%%esi)     \n\t"
        : // no outputs.
        : "S" (&_ctGroupsCount)
        : "eax", "ecx", "cc", "memory"
  );

#else
  // emulate x86's bsr opcode...

  // GCC and clang have an architecture-independent intrinsic for this
  // (it counts leading zeros starting at MSB and is undefined for 0)
  #ifdef __GNUC__
    INDEX bsr = 31;
    if(_ctGroupsCount != 0)  bsr -= __builtin_clz(_ctGroupsCount);
    else  bsr = 0;
  #else // another compiler - doing it manually.. not fast.  :/
    register DWORD val = _ctGroupsCount;
    register INDEX bsr = 31;
    if (val != 0)
    {
        while (bsr > 0)
        {
          if (val & (1l << bsr))
              break;
          bsr--;
        }
    }
  #endif

  _ctGroupsCount = 2 << bsr;
#endif

  // done with bining
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_BINTOGROUPS);
}



// bin polygons that can use dual-texturing
static void RSBinByDualTexturing( ScenePolygon *pspoGroup, INDEX iLayer1, INDEX iLayer2,
                                  ScenePolygon **ppspoST, ScenePolygon **ppspoMT)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_BINBYMULTITEXTURING);
  *ppspoST = NULL;
  *ppspoMT = NULL;
  // for all span polygons in list (remember one ahead to be able to reconnect them)
  for( ScenePolygon *pspoNext, *pspo=pspoGroup; pspo!=NULL; pspo=pspoNext)
  {
    pspoNext = pspo->spo_pspoSucc;
    // if first texture is opaque or shade and second layer is shade
    if( ((pspo->spo_aubTextureFlags[iLayer1]&STXF_BLEND_MASK)==STXF_BLEND_OPAQUE
     ||  (pspo->spo_aubTextureFlags[iLayer1]&STXF_BLEND_MASK)==STXF_BLEND_SHADE) 
     &&  (pspo->spo_aubTextureFlags[iLayer2]&STXF_BLEND_MASK)==STXF_BLEND_SHADE) {
      // can be merged,  so put to multi-texture
      pspo->spo_pspoSucc = *ppspoMT;
      *ppspoMT = pspo;
    } else {
      // cannot be merged, so put to single-texture
      pspo->spo_pspoSucc = *ppspoST;
      *ppspoST = pspo;
    }
  }
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_BINBYMULTITEXTURING);
}


// bin polygons that can use triple-texturing
static void RSBinByTripleTexturing( ScenePolygon *pspoGroup, INDEX iLayer2, INDEX iLayer3,
                                    ScenePolygon **ppspoST, ScenePolygon **ppspoMT)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_BINBYMULTITEXTURING);
  *ppspoST = NULL;
  *ppspoMT = NULL;
  // for all span polygons in list (remember one ahead to be able to reconnect them)
  for( ScenePolygon *pspoNext, *pspo=pspoGroup; pspo!=NULL; pspo=pspoNext)
  {
    pspoNext = pspo->spo_pspoSucc;
    // if texture is shade and colors allow merging
    if( (pspo->spo_aubTextureFlags[iLayer3]&STXF_BLEND_MASK)==STXF_BLEND_SHADE) {
      // can be merged, so put to multi-texture
      pspo->spo_pspoSucc = *ppspoMT;
      *ppspoMT = pspo;
    } else {
      // cannot be merged, so put to single-texture
      pspo->spo_pspoSucc = *ppspoST;
      *ppspoST = pspo;
    }
  }
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_BINBYMULTITEXTURING);
}


// bin polygons that can use quad-texturing
static void RSBinByQuadTexturing( ScenePolygon *pspoGroup, ScenePolygon **ppspoST, ScenePolygon **ppspoMT)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_BINBYMULTITEXTURING);
  *ppspoST = NULL;
  *ppspoMT = pspoGroup;
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_BINBYMULTITEXTURING);
}


// check if all layers in all shadow maps are up to date
static void RSCheckLayersUpToDate( ScenePolygon *pspoFirst)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_CHECKLAYERSUPTODATE);
  // for all span polygons in list
  for( ScenePolygon *pspo=pspoFirst; pspo!=NULL; pspo=pspo->spo_pspoSucc) {
    if( pspo->spo_psmShadowMap!=NULL) pspo->spo_psmShadowMap->CheckLayersUpToDate();
  }
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_CHECKLAYERSUPTODATE);
}


// prepare parameters individual to a polygon texture
inline void RSSetTextureWrapping( ULONG ulFlags)
{
  gfxSetTextureWrapping( (ulFlags&STXF_CLAMPU) ? GFX_CLAMP : GFX_REPEAT,
                         (ulFlags&STXF_CLAMPV) ? GFX_CLAMP : GFX_REPEAT);
}


// prepare parameters individual to a polygon texture
static void RSSetInitialTextureParameters(void)
{
  _ulLastFlags[0]  = STXF_BLEND_OPAQUE;
  _ulLastBlends[0] = STXF_BLEND_OPAQUE;
  _iLastFrameNo[0] = 0;
  _ptdLastTex[0]   = NULL;
  gfxSetTextureModulation(1);
  gfxDisableBlend();
}


static void RSSetTextureParameters( ULONG ulFlags)
{
  // if blend flags have changed
  ULONG ulBlendFlags = ulFlags&STXF_BLEND_MASK;
  if( _ulLastBlends[0] != ulBlendFlags)
  { // determine new texturing mode
    switch( ulBlendFlags) {
    case STXF_BLEND_OPAQUE: // opaque texturing
      gfxDisableBlend();
      break;
    case STXF_BLEND_ALPHA:  // blend using texture alpha
      gfxEnableBlend();
      gfxBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA); 
      break;
    case STXF_BLEND_ADD:  // add to screen
      gfxEnableBlend();
      gfxBlendFunc( GFX_ONE, GFX_ONE); 
      break;
    default: // screen*texture*2
      ASSERT( ulBlendFlags==STXF_BLEND_SHADE); 
      gfxEnableBlend();
      gfxBlendFunc( GFX_DST_COLOR, GFX_SRC_COLOR); 
      break;
    }
    // remember new flags
    _ulLastBlends[0] = ulFlags;
  }
}


// prepare initial parameters for polygon texture
static void RSSetInitialTextureParametersMT(void)
{
  INDEX i;
  // reset bleding modes
  for( i=0; i<MAXTEXUNITS; i++) {
    _ulLastFlags[i]  = STXF_BLEND_OPAQUE;
    _ulLastBlends[i] = STXF_BLEND_OPAQUE;
    _iLastFrameNo[i] = 0;
    _ptdLastTex[i]   = NULL;
  }
  // reset for texture
  gfxDisableBlend();
  for( i=1; i<_ctUsableTexUnits; i++) {
    gfxSetTextureUnit(i);
    gfxSetTextureModulation(2);
  }
  gfxSetTextureUnit(0);
  gfxSetTextureModulation(1);
}


// prepare parameters individual to a polygon texture
static void RSSetTextureParametersMT( ULONG ulFlags)
{
  // skip if the same as last time
  const ULONG ulBlendFlags = ulFlags&STXF_BLEND_MASK;
  if( _ulLastBlends[0]==ulBlendFlags) return; 
  // update
  if( ulBlendFlags==STXF_BLEND_OPAQUE) {
    // opaque texturing
    gfxDisableBlend();
  } else {
    // shade texturing
    ASSERT( ulBlendFlags==STXF_BLEND_SHADE);
    gfxEnableBlend();
    gfxBlendFunc( GFX_DST_COLOR, GFX_SRC_COLOR); 
  } // keep
  _ulLastBlends[0] = ulBlendFlags;
}


// make vertex coordinates for all polygons in the group
static void RSMakeVertexCoordinates( ScenePolygon *pspoGroup)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_MAKEVERTEXCOORDS);
  _avtxPass.PopAll();
  INDEX ctGroupVtx = 0;

  // for all scene polygons in list
  for( ScenePolygon *pspo=pspoGroup; pspo!=NULL; pspo=pspo->spo_pspoSucc)
  { 
    // create new vertices for that polygon in per-pass array
    const INDEX ctVtx   = pspo->spo_ctVtx;
    pspo->spo_iVtx0Pass = _avtxPass.Count();
    GFXVertex3 *pvtxScene = &_avtxScene[pspo->spo_iVtx0];
    GFXVertex4 *pvtxPass  =  _avtxPass.Push(ctVtx);

    // copy the vertex coordinates
    for( INDEX iVtx=0; iVtx<ctVtx; iVtx++) {
      pvtxPass[iVtx].x = pvtxScene[iVtx].x;
      pvtxPass[iVtx].y = pvtxScene[iVtx].y;
      pvtxPass[iVtx].z = pvtxScene[iVtx].z;
    }
    // add polygon vertices to total
    ctGroupVtx += ctVtx; 
  }

  // prepare texture and color arrays
  _acolPass.PopAll();
  _atexPass[0].PopAll();
  _atexPass[1].PopAll();
  _atexPass[2].PopAll();
  _atexPass[3].PopAll();
  _acolPass.Push(ctGroupVtx);
  for( INDEX i=0; i<_ctUsableTexUnits; i++) _atexPass[i].Push(ctGroupVtx);

  // done
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_MAKEVERTEXCOORDS);
}


static void RSSetPolygonColors( ScenePolygon *pspoGroup, UBYTE ubAlpha)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_SETCOLORS);
  // for all scene polygons in list
  COLOR col;
  GFXColor *pcol;
  for( ScenePolygon *pspo = pspoGroup; pspo != NULL; pspo = pspo->spo_pspoSucc) {
    col  = ByteSwap( AdjustColor( pspo->spo_cColor|ubAlpha, _slTexHueShift, _slTexSaturation));
    pcol = &_acolPass[pspo->spo_iVtx0Pass];
	for (INDEX i = 0; i<pspo->spo_ctVtx; i++) pcol[i].ul.abgr = col;
  }
  gfxSetColorArray( &_acolPass[0]);
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_SETCOLORS);
}


static void RSSetConstantColors( COLOR col)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_SETCOLORS);
  col = ByteSwap( AdjustColor( col, _slTexHueShift, _slTexSaturation));
  GFXColor *pcol = &_acolPass[0];
  for (INDEX i = 0; i<_acolPass.Count(); i++) pcol[i].ul.abgr = col;
  gfxSetColorArray( &_acolPass[0]);
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_SETCOLORS);
}


static void RSSetTextureColors( ScenePolygon *pspoGroup, ULONG ulLayerMask)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_SETCOLORS);
  ASSERT( !(ulLayerMask & (GF_TA1|GF_TA2|GF_FOG|GF_HAZE|GF_SEL)));
  // for all scene polygons in list
  COLOR colLayer, colTotal;
  for( ScenePolygon *pspo = pspoGroup; pspo != NULL; pspo = pspo->spo_pspoSucc)
  { // adjust hue/saturation and set colors
    colTotal = C_WHITE|CT_OPAQUE;
    if( ulLayerMask&GF_TX0) {
      colLayer = AdjustColor( pspo->spo_acolColors[0], _slTexHueShift, _slTexSaturation);
      if( colLayer!=0xFFFFFFFF) colTotal = MulColors( colTotal, colLayer);
    }
    if( ulLayerMask&GF_TX1) {
      colLayer = AdjustColor( pspo->spo_acolColors[1], _slTexHueShift, _slTexSaturation);
      if( colLayer!=0xFFFFFFFF) colTotal = MulColors( colTotal, colLayer);
    }
    if( ulLayerMask&GF_TX2) {
      colLayer = AdjustColor( pspo->spo_acolColors[2], _slTexHueShift, _slTexSaturation);
      if( colLayer!=0xFFFFFFFF) colTotal = MulColors( colTotal, colLayer);
    }
    if( ulLayerMask&GF_SHD) {
      colLayer = AdjustColor( pspo->spo_acolColors[3], _slShdHueShift, _slShdSaturation);
      if( colLayer!=0xFFFFFFFF) colTotal = MulColors( colTotal, colLayer);
    }
    // store
    colTotal = ByteSwap(colTotal);
    GFXColor *pcol= &_acolPass[pspo->spo_iVtx0Pass];
	for (INDEX i = 0; i<pspo->spo_ctVtx; i++) pcol[i].ul.abgr = colTotal;
  }
  // set color array
  gfxSetColorArray( &_acolPass[0]);
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_SETCOLORS);
}


// make texture coordinates for one texture in all polygons in group
static INDEX _iLastUnit = -1;
static void RSSetTextureCoords( ScenePolygon *pspoGroup, INDEX iLayer, INDEX iUnit)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_SETTEXCOORDS);
  // eventualy switch texture unit
  if( _iLastUnit != iUnit) {
    gfxSetTextureUnit(iUnit);
    gfxEnableTexture();
    _iLastUnit = iUnit;
  }

  // generate tex coord for all scene polygons in list
  const FLOATmatrix3D &mViewer = _ppr->pr_ViewerRotationMatrix;
  const INDEX iMappingOffset = iLayer * sizeof(CMappingVectors);
  (void)iMappingOffset; // shut up compiler, this is used if inline ASM is used

  for( ScenePolygon *pspo=pspoGroup; pspo!=NULL; pspo=pspo->spo_pspoSucc)
  {
    ASSERT( pspo->spo_ctVtx>0);
    const FLOAT3D &vN = ((CBrushPolygon*)pspo->spo_pvPolygon)->bpo_pbplPlane->bpl_pwplWorking->wpl_plView;
    const GFXVertex   *pvtx = &_avtxPass[pspo->spo_iVtx0Pass];
          GFXTexCoord *ptex = &_atexPass[iUnit][pspo->spo_iVtx0Pass];
    
    // reflective mapping?
    if( pspo->spo_aubTextureFlags[iLayer] & STXF_REFLECTION) {
      for( INDEX i=0; i<pspo->spo_ctVtx; i++) { 
        const FLOAT fNorm = 1.0f / sqrt(pvtx[i].x*pvtx[i].x + pvtx[i].y*pvtx[i].y + pvtx[i].z*pvtx[i].z);
        const FLOAT fVx = pvtx[i].x *fNorm;
        const FLOAT fVy = pvtx[i].y *fNorm;
        const FLOAT fVz = pvtx[i].z *fNorm;
        const FLOAT fNV = fVx*vN(1) + fVy*vN(2) + fVz*vN(3);
        const FLOAT fRVx = fVx - 2*vN(1)*fNV;
        const FLOAT fRVy = fVy - 2*vN(2)*fNV;
        const FLOAT fRVz = fVz - 2*vN(3)*fNV;
        const FLOAT fRVxT = fRVx*mViewer(1,1) + fRVy*mViewer(2,1) + fRVz*mViewer(3,1);
        const FLOAT fRVzT = fRVx*mViewer(1,3) + fRVy*mViewer(2,3) + fRVz*mViewer(3,3);
		ptex[i].st.s = fRVxT*0.5f + 0.5f;
		ptex[i].st.t = fRVzT*0.5f + 0.5f;
      }
      // advance to next polygon
      continue;
    }

#if (defined __MSVC_INLINE__)
    __asm {
      mov     esi,D [pspo]
      mov     edi,D [iMappingOffset]

// (This doesn't work with the Intel C++ compiler. :(  --ryan.)
#ifdef _MSC_VER
      lea     eax,[esi].spo_amvMapping[edi].mv_vO
      lea     ebx,[esi].spo_amvMapping[edi].mv_vU
      lea     ecx,[esi].spo_amvMapping[edi].mv_vV
#else
      lea     ebx,[esi].spo_amvMapping[edi]
      lea     eax,[ebx].mv_vO
      lea     ecx,[ebx].mv_vV
      lea     ebx,[ebx].mv_vU
#endif

      mov     edx,D [esi].spo_ctVtx
      mov     esi,D [pvtx]
      mov     edi,D [ptex]
vtxLoop:
      fld     D [ebx+0]
      fld     D [esi]GFXVertex.x
      fsub    D [eax+0]
      fmul    st(1),st(0)
      fmul    D [ecx+0]   // vV(1)*fDX, vU(1)*fDX
      fld     D [ebx+4]
      fld     D [esi]GFXVertex.y
      fsub    D [eax+4]
      fmul    st(1),st(0)
      fmul    D [ecx+4]   // vV(2)*fDY, vU(2)*fDY, vV(1)*fDX, vU(1)*fDX
      fld     D [ebx+8]
      fld     D [esi]GFXVertex.z
      fsub    D [eax+8]
      fmul    st(1),st(0)
      fmul    D [ecx+8]   // vV(3)*fDZ, vU(3)*fDZ, vV(2)*fDY, vU(2)*fDY, vV(1)*fDX, vU(1)*fDX
      fxch    st(5)
      faddp   st(3),st(0) // vU(3)*fDZ, vV(2)*fDY, vU(1)*fDX+vU(2)*fDY, vV(1)*fDX, vV(3)*fDZ
      fxch    st(1)
      faddp   st(3),st(0) // vU(3)*fDZ, vU(1)*fDX+vU(2)*fDY, vV(1)*fDX+vV(2)*fDY, vV(3)*fDZ
      faddp   st(1),st(0) // vU(1)*fDX+vU(2)*fDY+vU(3)*fDZ,  vV(1)*fDX+vV(2)*fDY, vV(3)*fDZ
      fxch    st(1)
      faddp   st(2),st(0) // vU(1)*fDX+vU(2)*fDY+vU(3)*fDZ,  vV(1)*fDX+vV(2)*fDY+vV(3)*fDZ
      //fstp    D [edi]GFXTexCoord.s
      //fstp    D [edi]GFXTexCoord.t
	  fstp    D [edi]GFXTexCoord.st.s
	  fstp    D [edi]GFXTexCoord.st.t
      add     esi,4*4
      add     edi,2*4
      dec     edx
      jnz     vtxLoop
    }

/*
    // !!! FIXME: rcg11232001 This inline conversion is broken. Use the
    // !!! FIXME: rcg11232001  C version for now on Linux.
#elif (defined __GNU_INLINE_X86_32__)
    STUBBED("debug this");
    __asm__ __volatile__ (
      "0:                                  \n\t" // vtxLoop
      "flds    (%%ebx)                     \n\t"
      "flds    (%%esi)                     \n\t"
      "fsubs   (%%eax)                     \n\t"
      "fmul    %%st(0), %%st(1)            \n\t"
      "fmuls   (%%ecx)                     \n\t" // vV(1)*fDX, vU(1)*fDX
      "flds    4(%%ebx)                    \n\t"
      "flds    4(%%esi)                    \n\t" // GFXVertex.y
      "fsubs   4(%%eax)                    \n\t"
      "fmul    %%st(0), %%st(1)            \n\t"
      "fmuls   4(%%ecx)                    \n\t" // vV(2)*fDY, vU(2)*fDY, vV(1)*fDX, vU(1)*fDX
      "flds    8(%%ebx)                    \n\t"
      "flds    8(%%esi)                    \n\t" // GFXVertex.z
      "fsubs   8(%%eax)                    \n\t"
      "fmul    %%st(0), %%st(1)            \n\t"
      "fmuls   8(%%ecx)                    \n\t" // vV(3)*fDZ, vU(3)*fDZ, vV(2)*fDY, vU(2)*fDY, vV(1)*fDX, vU(1)*fDX
      "fxch    %%st(5)                     \n\t"
      "faddp   %%st(0), %%st(3)            \n\t" // vU(3)*fDZ, vV(2)*fDY, vU(1)*fDX+vU(2)*fDY, vV(1)*fDX, vV(3)*fDZ
      "fxch    %%st(1)                     \n\t"
      "faddp   %%st(0), %%st(3)            \n\t" // vU(3)*fD Z, vU(1)*fDX+vU(2)*fDY, vV(1)*fDX+vV(2)*fDY, vV(3)*fDZ
      "faddp   %%st(0), %%st(1)            \n\t" // vU(1)*fDX+vU(2)*fDY+vU(3)*fDZ,  vV(1)*fDX+vV(2)*fDY, vV(3)*fDZ
      "fxch    %%st(1)                     \n\t"
      "faddp   %%st(0), %%st(2)            \n\t" // vU(1)*fDX+vU(2)*fDY+vU(3)*fDZ,  vV(1)*fDX+vV(2)*fDY+vV(3)*fDZ
      "fstps   0(%%edi)                    \n\t" // GFXTexCoord.st.s
      "fstps   4(%%edi)                    \n\t" // GFXTexCoord.st.t
      "addl    $16, %%esi                  \n\t"
      "addl    $8, %%edi                   \n\t"
      "decl    %%edx                       \n\t"
      "jnz     0b                          \n\t" // vtxLoop
        : // no outputs.
        : "a" (&pspo->spo_amvMapping[iMappingOffset].mv_vO.vector),
          "b" (&pspo->spo_amvMapping[iMappingOffset].mv_vU.vector),
          "c" (&pspo->spo_amvMapping[iMappingOffset].mv_vV.vector),
          "d" (pspo->spo_ctVtx), "S" (pvtx), "D" (ptex)
        : "cc", "memory"
    );
*/

#else

    // diffuse mapping
    const FLOAT3D &vO = pspo->spo_amvMapping[iLayer].mv_vO;
    const FLOAT3D &vU = pspo->spo_amvMapping[iLayer].mv_vU;
    const FLOAT3D &vV = pspo->spo_amvMapping[iLayer].mv_vV;
    for( INDEX i=0; i<pspo->spo_ctVtx; i++) {
      const FLOAT fDX = pvtx[i].x -vO(1);
      const FLOAT fDY = pvtx[i].y -vO(2);
      const FLOAT fDZ = pvtx[i].z -vO(3);
      ptex[i].st.s = vU(1)*fDX + vU(2)*fDY + vU(3)*fDZ;
      ptex[i].st.t = vV(1)*fDX + vV(2)*fDY + vV(3)*fDZ;
    }
#endif

  }

  // init array
  gfxSetTexCoordArray( &_atexPass[iUnit][0], FALSE);
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_SETTEXCOORDS);
}



// make fog texture coordinates for all polygons in group
static void RSSetFogCoordinates( ScenePolygon *pspoGroup)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_SETTEXCOORDS);
  // for all scene polygons in list
  for( ScenePolygon *pspo=pspoGroup; pspo!=NULL; pspo=pspo->spo_pspoSucc) {
    const GFXVertex   *pvtx = &_avtxPass[pspo->spo_iVtx0Pass];
          GFXTexCoord *ptex = &_atexPass[0][pspo->spo_iVtx0Pass];
    for( INDEX i=0; i<pspo->spo_ctVtx; i++) {
		ptex[i].st.s = pvtx[i].z *_fFogMul;
		ptex[i].st.t = (_fog_vHDirView(1)*pvtx[i].x + _fog_vHDirView(2)*pvtx[i].y
			+ _fog_vHDirView(3)*pvtx[i].z + _fog_fAddH) * _fog_fMulH;
    }
  }
  gfxSetTexCoordArray( &_atexPass[0][0], FALSE);
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_SETTEXCOORDS);
}


// make haze texture coordinates for all polygons in group
static void RSSetHazeCoordinates( ScenePolygon *pspoGroup)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_SETTEXCOORDS);
  // for all scene polygons in list
  for( ScenePolygon *pspo=pspoGroup; pspo!=NULL; pspo=pspo->spo_pspoSucc) {
    const GFXVertex   *pvtx = &_avtxPass[pspo->spo_iVtx0Pass];
          GFXTexCoord *ptex = &_atexPass[0][pspo->spo_iVtx0Pass];
    for( INDEX i=0; i<pspo->spo_ctVtx; i++) {
	  ptex[i].st.s = (pvtx[i].z + _fHazeAdd) *_fHazeMul;
	  ptex[i].st.t = 0;
    }
  }
  gfxSetTexCoordArray( &_atexPass[0][0], FALSE);
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_SETTEXCOORDS);
}


// render textures for all triangles in polygon list
static void RSRenderTEX( ScenePolygon *pspoFirst, INDEX iLayer)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_RENDERTEXTURES);
  RSSetInitialTextureParameters();

  // for all span polygons in list
  for( ScenePolygon *pspo=pspoFirst; pspo!=NULL; pspo=pspo->spo_pspoSucc)
  {
    ASSERT(pspo->spo_aptoTextures[iLayer]!=NULL);
    CTextureData *ptdTextureData = (CTextureData*)pspo->spo_aptoTextures[iLayer]->GetData();
    const INDEX iFrameNo = pspo->spo_aptoTextures[iLayer]->GetFrame();

    if( _ptdLastTex[0]   != ptdTextureData
     || _ulLastFlags[0]  != pspo->spo_aubTextureFlags[iLayer]
     || _iLastFrameNo[0] != iFrameNo) {
      // flush
      FlushElements();
      _ptdLastTex[0]   = ptdTextureData;
      _ulLastFlags[0]  = pspo->spo_aubTextureFlags[iLayer];
      _iLastFrameNo[0] = iFrameNo;
      // set texture parameters if needed
      RSSetTextureWrapping(   pspo->spo_aubTextureFlags[iLayer]);
      RSSetTextureParameters( pspo->spo_aubTextureFlags[iLayer]);
      // prepare texture to be used by accelerator
      ptdTextureData->SetAsCurrent(iFrameNo);
    }
    // render all triangles
    AddElements(pspo);
  }
  // flush leftovers
  FlushElements();
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_RENDERTEXTURES);
}


// render shadows for all triangles in polygon list
static void RSRenderSHD( ScenePolygon *pspoFirst)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_RENDERSHADOWS);

  RSSetInitialTextureParameters();
  // for all span polygons in list
  for( ScenePolygon *pspo = pspoFirst; pspo != NULL; pspo = pspo->spo_pspoSucc)
  { 
    // get shadow map for the polygon
    CShadowMap *psmShadow = pspo->spo_psmShadowMap;
    ASSERT( psmShadow!=NULL);  // shadows have been already sorted out

    // set texture parameters if needed
    RSSetTextureWrapping(   pspo->spo_aubTextureFlags[SHADOWTEXTURE]);
    RSSetTextureParameters( pspo->spo_aubTextureFlags[SHADOWTEXTURE]);

    // upload the shadow to accelerator memory
    psmShadow->SetAsCurrent();

    // batch and render triangles
    AddElements(pspo);
    FlushElements();
  }
  // done
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_RENDERSHADOWS);
}


// render texture and shadow for all triangles in polygon list
static void RSRenderTEX_SHD( ScenePolygon *pspoFirst, INDEX iLayer)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_RENDERMT);
  RSSetInitialTextureParametersMT();

  // for all span polygons in list
  for( ScenePolygon *pspo=pspoFirst; pspo!=NULL; pspo=pspo->spo_pspoSucc)
  {
    // render batched triangles
    FlushElements();
    ASSERT( pspo->spo_aptoTextures[iLayer]!=NULL && pspo->spo_psmShadowMap!=NULL);

    // upload the shadow to accelerator memory
    gfxSetTextureUnit(1);
    RSSetTextureWrapping( pspo->spo_aubTextureFlags[SHADOWTEXTURE]);
    pspo->spo_psmShadowMap->SetAsCurrent();

    // prepare texture to be used by accelerator
    CTextureData *ptd = (CTextureData*)pspo->spo_aptoTextures[iLayer]->GetData();
    const INDEX iFrameNo = pspo->spo_aptoTextures[iLayer]->GetFrame();
    
    gfxSetTextureUnit(0);
    if( _ptdLastTex[0]!=ptd || _iLastFrameNo[0]!=iFrameNo || _ulLastFlags[0]!=pspo->spo_aubTextureFlags[iLayer]) {
      _ptdLastTex[0]=ptd;  _iLastFrameNo[0]=iFrameNo;  _ulLastFlags[0]=pspo->spo_aubTextureFlags[iLayer];
      RSSetTextureWrapping( pspo->spo_aubTextureFlags[iLayer]);
      ptd->SetAsCurrent(iFrameNo);
      // set rendering parameters if needed
      RSSetTextureParametersMT( pspo->spo_aubTextureFlags[iLayer]);
    }
    // batch triangles
    AddElements(pspo);
  }

  // flush leftovers
  FlushElements();
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_RENDERMT);
}


// render two textures for all triangles in polygon list
static void RSRender2TEX( ScenePolygon *pspoFirst, INDEX iLayer2)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_RENDERMT);
  RSSetInitialTextureParametersMT();

  // for all span polygons in list
  for( ScenePolygon *pspo=pspoFirst; pspo!=NULL; pspo=pspo->spo_pspoSucc)
  {
    ASSERT( pspo->spo_aptoTextures[0]!=NULL && pspo->spo_aptoTextures[iLayer2]!=NULL);
    CTextureData *ptd0 = (CTextureData*)pspo->spo_aptoTextures[0]->GetData();
    CTextureData *ptd1 = (CTextureData*)pspo->spo_aptoTextures[iLayer2]->GetData();
    const INDEX iFrameNo0 = pspo->spo_aptoTextures[0]->GetFrame();
    const INDEX iFrameNo1 = pspo->spo_aptoTextures[iLayer2]->GetFrame();

    if( _ptdLastTex[0]!=ptd0 || _iLastFrameNo[0]!=iFrameNo0 || _ulLastFlags[0]!=pspo->spo_aubTextureFlags[0]
     || _ptdLastTex[1]!=ptd1 || _iLastFrameNo[1]!=iFrameNo1 || _ulLastFlags[1]!=pspo->spo_aubTextureFlags[iLayer2]) {
      FlushElements();
      _ptdLastTex[0]=ptd0;  _iLastFrameNo[0]=iFrameNo0;  _ulLastFlags[0]=pspo->spo_aubTextureFlags[0];
      _ptdLastTex[1]=ptd1;  _iLastFrameNo[1]=iFrameNo1;  _ulLastFlags[1]=pspo->spo_aubTextureFlags[iLayer2];
      // upload the second texture to unit 1
      gfxSetTextureUnit(1);
      RSSetTextureWrapping( pspo->spo_aubTextureFlags[iLayer2]);
      ptd1->SetAsCurrent(iFrameNo1);
      // upload the first texture to unit 0
      gfxSetTextureUnit(0);
      RSSetTextureWrapping( pspo->spo_aubTextureFlags[0]);
      ptd0->SetAsCurrent(iFrameNo0);
      // set rendering parameters if needed
      RSSetTextureParametersMT( pspo->spo_aubTextureFlags[0]);
    }
    // render all triangles
    AddElements(pspo);
  }

  // flush leftovers
  FlushElements();
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_RENDERMT);
}



// render two textures and shadowmap for all triangles in polygon list
static void RSRender2TEX_SHD( ScenePolygon *pspoFirst, INDEX iLayer2)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_RENDERMT);
  RSSetInitialTextureParametersMT();

  // for all span polygons in list
  for( ScenePolygon *pspo=pspoFirst; pspo!=NULL; pspo=pspo->spo_pspoSucc)
  {
    ASSERT( pspo->spo_aptoTextures[0]!=NULL
         && pspo->spo_aptoTextures[iLayer2]!=NULL
         && pspo->spo_psmShadowMap!=NULL);

    // render batched triangles
    FlushElements();

    // upload the shadow to accelerator memory
    gfxSetTextureUnit(2);
    RSSetTextureWrapping( pspo->spo_aubTextureFlags[SHADOWTEXTURE]);
    pspo->spo_psmShadowMap->SetAsCurrent();

    // prepare textures to be used by accelerator
    CTextureData *ptd0 = (CTextureData*)pspo->spo_aptoTextures[0]->GetData();
    CTextureData *ptd1 = (CTextureData*)pspo->spo_aptoTextures[iLayer2]->GetData();
    const INDEX iFrameNo0 = pspo->spo_aptoTextures[0]->GetFrame();
    const INDEX iFrameNo1 = pspo->spo_aptoTextures[iLayer2]->GetFrame();

    gfxSetTextureUnit(0);
    if( _ptdLastTex[0]!=ptd0 || _iLastFrameNo[0]!=iFrameNo0 || _ulLastFlags[0]!=pspo->spo_aubTextureFlags[0]
     || _ptdLastTex[1]!=ptd1 || _iLastFrameNo[1]!=iFrameNo1 || _ulLastFlags[1]!=pspo->spo_aubTextureFlags[iLayer2]) {
      _ptdLastTex[0]=ptd0;  _iLastFrameNo[0]=iFrameNo0;  _ulLastFlags[0]=pspo->spo_aubTextureFlags[0];      
      _ptdLastTex[1]=ptd1;  _iLastFrameNo[1]=iFrameNo1;  _ulLastFlags[1]=pspo->spo_aubTextureFlags[iLayer2];
      // upload the second texture to unit 1
      gfxSetTextureUnit(1);
      RSSetTextureWrapping( pspo->spo_aubTextureFlags[iLayer2]);
      ptd1->SetAsCurrent(iFrameNo1);
      // upload the first texture to unit 0
      gfxSetTextureUnit(0);
      RSSetTextureWrapping( pspo->spo_aubTextureFlags[0]);
      ptd0->SetAsCurrent(iFrameNo0);
      // set rendering parameters if needed
      RSSetTextureParametersMT( pspo->spo_aubTextureFlags[0]);
    }
    // render all triangles
    AddElements(pspo);
  }

  // flush leftovers
  FlushElements();
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_RENDERMT);
}



// render three textures for all triangles in polygon list
static void RSRender3TEX( ScenePolygon *pspoFirst)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_RENDERMT);
  RSSetInitialTextureParametersMT();

  // for all span polygons in list
  for( ScenePolygon *pspo=pspoFirst; pspo!=NULL; pspo=pspo->spo_pspoSucc)
  {
    ASSERT( pspo->spo_aptoTextures[0]!=NULL
         && pspo->spo_aptoTextures[1]!=NULL
         && pspo->spo_aptoTextures[2]!=NULL);
    CTextureData *ptd0 = (CTextureData*)pspo->spo_aptoTextures[0]->GetData();
    CTextureData *ptd1 = (CTextureData*)pspo->spo_aptoTextures[1]->GetData();
    CTextureData *ptd2 = (CTextureData*)pspo->spo_aptoTextures[2]->GetData();
    const INDEX iFrameNo0 = pspo->spo_aptoTextures[0]->GetFrame();
    const INDEX iFrameNo1 = pspo->spo_aptoTextures[1]->GetFrame();
    const INDEX iFrameNo2 = pspo->spo_aptoTextures[2]->GetFrame();

    if( _ptdLastTex[0]!=ptd0 || _iLastFrameNo[0]!=iFrameNo0 || _ulLastFlags[0]!=pspo->spo_aubTextureFlags[0]
     || _ptdLastTex[1]!=ptd1 || _iLastFrameNo[1]!=iFrameNo1 || _ulLastFlags[1]!=pspo->spo_aubTextureFlags[1]
     || _ptdLastTex[2]!=ptd2 || _iLastFrameNo[2]!=iFrameNo2 || _ulLastFlags[2]!=pspo->spo_aubTextureFlags[2]) {
      FlushElements();
      _ptdLastTex[0]=ptd0;  _iLastFrameNo[0]=iFrameNo0;  _ulLastFlags[0]=pspo->spo_aubTextureFlags[0];      
      _ptdLastTex[1]=ptd1;  _iLastFrameNo[1]=iFrameNo1;  _ulLastFlags[1]=pspo->spo_aubTextureFlags[1];
      _ptdLastTex[2]=ptd2;  _iLastFrameNo[2]=iFrameNo2;  _ulLastFlags[2]=pspo->spo_aubTextureFlags[2];
      // upload the third texture to unit 2
      gfxSetTextureUnit(2);
      RSSetTextureWrapping( pspo->spo_aubTextureFlags[2]);
      ptd2->SetAsCurrent(iFrameNo2);
      // upload the second texture to unit 1
      gfxSetTextureUnit(1);
      RSSetTextureWrapping( pspo->spo_aubTextureFlags[1]);
      ptd1->SetAsCurrent(iFrameNo1);
      // upload the first texture to unit 0
      gfxSetTextureUnit(0);
      RSSetTextureWrapping( pspo->spo_aubTextureFlags[0]);
      ptd0->SetAsCurrent(iFrameNo0);
      // set rendering parameters if needed
      RSSetTextureParametersMT( pspo->spo_aubTextureFlags[0]);
    }
    // render all triangles
    AddElements(pspo);
  }

  // flush leftovers
  FlushElements();
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_RENDERMT);
}


// render three textures and shadowmap for all triangles in polygon list
static void RSRender3TEX_SHD( ScenePolygon *pspoFirst)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_RENDERMT);
  RSSetInitialTextureParametersMT();

  // for all span polygons in list
  for( ScenePolygon *pspo=pspoFirst; pspo!=NULL; pspo=pspo->spo_pspoSucc)
  {
    ASSERT( pspo->spo_aptoTextures[0]!=NULL
         && pspo->spo_aptoTextures[1]!=NULL
         && pspo->spo_aptoTextures[2]!=NULL
         && pspo->spo_psmShadowMap!=NULL);

    // render batched triangles
    FlushElements();

    // upload the shadow to accelerator memory
    gfxSetTextureUnit(3);
    RSSetTextureWrapping( pspo->spo_aubTextureFlags[SHADOWTEXTURE]);
    pspo->spo_psmShadowMap->SetAsCurrent();

    // prepare textures to be used by accelerator
    CTextureData *ptd0 = (CTextureData*)pspo->spo_aptoTextures[0]->GetData();
    CTextureData *ptd1 = (CTextureData*)pspo->spo_aptoTextures[1]->GetData();
    CTextureData *ptd2 = (CTextureData*)pspo->spo_aptoTextures[2]->GetData();
    const INDEX iFrameNo0 = pspo->spo_aptoTextures[0]->GetFrame();
    const INDEX iFrameNo1 = pspo->spo_aptoTextures[1]->GetFrame();
    const INDEX iFrameNo2 = pspo->spo_aptoTextures[2]->GetFrame();

    gfxSetTextureUnit(0);
    if( _ptdLastTex[0]!=ptd0 || _iLastFrameNo[0]!=iFrameNo0 || _ulLastFlags[0]!=pspo->spo_aubTextureFlags[0]
     || _ptdLastTex[1]!=ptd1 || _iLastFrameNo[1]!=iFrameNo1 || _ulLastFlags[1]!=pspo->spo_aubTextureFlags[1]
     || _ptdLastTex[2]!=ptd2 || _iLastFrameNo[2]!=iFrameNo2 || _ulLastFlags[2]!=pspo->spo_aubTextureFlags[2]) {
      _ptdLastTex[0]=ptd0;  _iLastFrameNo[0]=iFrameNo0;  _ulLastFlags[0]=pspo->spo_aubTextureFlags[0];      
      _ptdLastTex[1]=ptd1;  _iLastFrameNo[1]=iFrameNo1;  _ulLastFlags[1]=pspo->spo_aubTextureFlags[1];
      _ptdLastTex[2]=ptd2;  _iLastFrameNo[2]=iFrameNo2;  _ulLastFlags[2]=pspo->spo_aubTextureFlags[2];
      // upload the third texture to unit 2
      gfxSetTextureUnit(2);
      RSSetTextureWrapping( pspo->spo_aubTextureFlags[2]);
      ptd2->SetAsCurrent(iFrameNo2);
      // upload the second texture to unit 1
      gfxSetTextureUnit(1);
      RSSetTextureWrapping( pspo->spo_aubTextureFlags[1]);
      ptd1->SetAsCurrent(iFrameNo1);
      // upload the first texture to unit 0
      gfxSetTextureUnit(0);
      RSSetTextureWrapping( pspo->spo_aubTextureFlags[0]);
      ptd0->SetAsCurrent(iFrameNo0);
      // set rendering parameters if needed
      RSSetTextureParametersMT( pspo->spo_aubTextureFlags[0]);
    }
    // render all triangles
    AddElements(pspo);
  }

  // flush leftovers
  FlushElements();
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_RENDERMT);
}


// render fog for all affected triangles in polygon list
__forceinline void RSRenderFog( ScenePolygon *pspoFirst)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_RENDERFOG);
  // for all scene polygons in list
  for( ScenePolygon *pspo=pspoFirst; pspo!=NULL; pspo=pspo->spo_pspoSucc)
  { // for all vertices in the polygon
    const GFXTexCoord *ptex = &_atexPass[0][pspo->spo_iVtx0Pass];
    for( INDEX i=0; i<pspo->spo_ctVtx; i++) {
      // polygon is in fog, stop searching
	  if (InFog(ptex[i].st.t)) goto hasFog;
    }
    // hasn't got any fog, so skip it
    continue;
hasFog:
    // render all triangles
    AddElements(pspo);
  }
  // all done
  FlushElements();
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_RENDERFOG);
}


// render haze for all affected triangles in polygon list
__forceinline void RSRenderHaze( ScenePolygon *pspoFirst)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_RENDERFOG);
  // for all scene polygons in list
  for( ScenePolygon *pspo=pspoFirst; pspo!=NULL; pspo=pspo->spo_pspoSucc)
  { // for all vertices in the polygon
    const GFXTexCoord *ptex = &_atexPass[0][pspo->spo_iVtx0Pass];
    for( INDEX i=0; i<pspo->spo_ctVtx; i++) {
      // polygon is in haze, stop searching
	  if (InHaze(ptex[i].st.s)) goto hasHaze;
    }
    // hasn't got any haze, so skip it
    continue;
hasHaze:
    // render all triangles
    AddElements(pspo);
  }
  // all done
  FlushElements();
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_RENDERFOG);
}



static void RSStartupFog(void)
{
  // upload fog texture
  gfxSetTextureWrapping( GFX_CLAMP, GFX_CLAMP);
  gfxSetTexture( _fog_ulTexture, _fog_tpLocal);
  // prepare fog rendering parameters
  gfxEnableBlend();
  gfxBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
  // calculate fog mapping
  _fFogMul = -1.0f / _fog_fp.fp_fFar;
}


static void RSStartupHaze(void)
{
  // upload haze texture
  gfxEnableTexture();
  gfxSetTextureWrapping( GFX_CLAMP, GFX_CLAMP);
  gfxSetTexture( _haze_ulTexture, _haze_tpLocal);
  // prepare haze rendering parameters
  gfxEnableBlend();
  gfxBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
  // calculate haze mapping
  _fHazeMul = -1.0f / (_haze_hp.hp_fFar - _haze_hp.hp_fNear);
  _fHazeAdd = _haze_hp.hp_fNear;
}


// process one group of polygons
void RSRenderGroupInternal( ScenePolygon *pspoGroup, ULONG ulGroupFlags);
void RSRenderGroup( ScenePolygon *pspoGroup, ULONG ulGroupFlags, ULONG ulTestedFlags)
{
  // skip if the group is empty
  if( pspoGroup==NULL) return;
  ASSERT( !(ulTestedFlags&(GF_FOG|GF_HAZE))); // paranoia

  // if multitexturing is enabled (start with 2-layer MT)
  if( _ctUsableTexUnits>=2)
  {
    // if texture 1 could be merged with shadow
    if( !(ulTestedFlags&GF_TX0_SHD)
     &&  (ulGroupFlags &GF_TX0)
     && !(ulGroupFlags &GF_TX1)
     && !(ulGroupFlags &GF_TX2)
     &&  (ulGroupFlags &GF_SHD))
    { // bin polygons that can use the merge and those that cannot
      ScenePolygon *pspoST, *pspoMT;
      RSBinByDualTexturing( pspoGroup, 0, SHADOWTEXTURE, &pspoST, &pspoMT);
      // process the two groups separately
      ulTestedFlags |= GF_TX0_SHD;
      RSRenderGroup( pspoST, ulGroupFlags, ulTestedFlags);
      ulGroupFlags &= ~(GF_TX0|GF_SHD);
      ulGroupFlags |=   GF_TX0_SHD;
      RSRenderGroup( pspoMT, ulGroupFlags, ulTestedFlags);
      return;
    }

    // if texture 1 could be merged with texture 2
    if( !(ulTestedFlags&GF_TX0_TX1)
     &&  (ulGroupFlags &GF_TX0)
     &&  (ulGroupFlags &GF_TX1))
    { // bin polygons that can use the merge and those that cannot
      ScenePolygon *pspoST, *pspoMT;
      RSBinByDualTexturing( pspoGroup, 0, 1, &pspoST, &pspoMT);
      // process the two groups separately
      ulTestedFlags |= GF_TX0_TX1;
      RSRenderGroup( pspoST, ulGroupFlags, ulTestedFlags);
      ulGroupFlags &= ~(GF_TX0|GF_TX1);
      ulGroupFlags |=   GF_TX0_TX1;
      RSRenderGroup( pspoMT, ulGroupFlags, ulTestedFlags);
      return;
    }

    // if texture 1 could be merged with texture 3
    if( !(ulTestedFlags&GF_TX0_TX2)
     &&  (ulGroupFlags &GF_TX0)
     && !(ulGroupFlags &GF_TX1)
     &&  (ulGroupFlags &GF_TX2))
    { // bin polygons that can use the merge and those that cannot
      ScenePolygon *pspoST, *pspoMT;
      RSBinByDualTexturing( pspoGroup, 0, 2, &pspoST, &pspoMT);
      // process the two groups separately
      ulTestedFlags |= GF_TX0_TX2;
      RSRenderGroup( pspoST, ulGroupFlags, ulTestedFlags);
      ulGroupFlags &= ~(GF_TX0|GF_TX2);
      ulGroupFlags |=   GF_TX0_TX2;
      RSRenderGroup( pspoMT, ulGroupFlags, ulTestedFlags);
      return;
    }

    // if texture 3 could be merged with shadow
    if( !(ulTestedFlags&GF_TX2_SHD)
     &&  (ulGroupFlags &GF_TX0_TX1)
     &&  (ulGroupFlags &GF_TX2)
     &&  (ulGroupFlags &GF_SHD))
    { // bin polygons that can use the merge and those that cannot
      ScenePolygon *pspoST, *pspoMT;
      RSBinByDualTexturing( pspoGroup, 2, SHADOWTEXTURE, &pspoST, &pspoMT);
      // process the two groups separately
      ulTestedFlags |= GF_TX2_SHD;
      RSRenderGroup( pspoST, ulGroupFlags, ulTestedFlags);
      ulGroupFlags &= ~(GF_TX2|GF_SHD);
      ulGroupFlags |=   GF_TX2_SHD;
      RSRenderGroup( pspoMT, ulGroupFlags, ulTestedFlags);
      return;
    }
  }

  // 4-layer multitexturing?
  if( _ctUsableTexUnits>=4)
  {
    // if texture 1 and 2 could be merged with 3 and shadow
    if( !(ulTestedFlags&GF_TX0_TX1_TX2_SHD)
      && (ulGroupFlags &GF_TX0_TX1)
      && (ulGroupFlags &GF_TX2_SHD)) 
    { // bin polygons that can use the merge and those that cannot
      ScenePolygon *pspoST, *pspoMT;
      RSBinByQuadTexturing( pspoGroup, &pspoST, &pspoMT);
      // process the two groups separately
      ulTestedFlags |= GF_TX0_TX1_TX2_SHD;
      RSRenderGroup( pspoST, ulGroupFlags, ulTestedFlags);
      ulGroupFlags &= ~(GF_TX0_TX1|GF_TX2_SHD);
      ulGroupFlags |=   GF_TX0_TX1_TX2_SHD;
      RSRenderGroup( pspoMT, ulGroupFlags, ulTestedFlags);
      return;
    }
  } 

  // 3-layer multitexturing?
  if( _ctUsableTexUnits>=3)
  {
    // if texture 1 and 2 could be merged with 3
    if( !(ulTestedFlags&GF_TX0_TX1_TX2)
     &&  (ulGroupFlags &GF_TX0_TX1)
     &&  (ulGroupFlags &GF_TX2))
    { // bin polygons that can use the merge and those that cannot
      ScenePolygon *pspoST, *pspoMT;
      RSBinByTripleTexturing( pspoGroup, 1, 2, &pspoST, &pspoMT);
      // process the two groups separately
      ulTestedFlags |= GF_TX0_TX1_TX2;
      RSRenderGroup( pspoST, ulGroupFlags, ulTestedFlags);
      ulGroupFlags &= ~(GF_TX0_TX1|GF_TX2);
      ulGroupFlags |=   GF_TX0_TX1_TX2;
      RSRenderGroup( pspoMT, ulGroupFlags, ulTestedFlags);
      return;
    }

    // if texture 1 and 2 could be merged with shadow
    if( !(ulTestedFlags&GF_TX0_TX1_SHD)
     &&  (ulGroupFlags &GF_TX0_TX1)
     && !(ulGroupFlags &GF_TX2)
     &&  (ulGroupFlags &GF_SHD))
    { // bin polygons that can use the merge and those that cannot
      ScenePolygon *pspoST, *pspoMT;
      RSBinByTripleTexturing( pspoGroup, 1, SHADOWTEXTURE, &pspoST, &pspoMT);
      // process the two groups separately
      ulTestedFlags |= GF_TX0_TX1_SHD;
      RSRenderGroup( pspoST, ulGroupFlags, ulTestedFlags);
      ulGroupFlags &= ~(GF_TX0_TX1|GF_SHD);
      ulGroupFlags |=   GF_TX0_TX1_SHD;
      RSRenderGroup( pspoMT, ulGroupFlags, ulTestedFlags);
      return;
    }

    // if texture 1 and 3 could be merged with shadow
    if( !(ulTestedFlags&GF_TX0_TX2_SHD)
     &&  (ulGroupFlags &GF_TX0_TX2)
     && !(ulGroupFlags &GF_TX1)
     &&  (ulGroupFlags &GF_SHD))
    { // bin polygons that can use the merge and those that cannot
      ScenePolygon *pspoST, *pspoMT;
      RSBinByTripleTexturing( pspoGroup, 2, SHADOWTEXTURE, &pspoST, &pspoMT);
      // process the two groups separately
      ulTestedFlags |= GF_TX0_TX2_SHD;
      RSRenderGroup( pspoST, ulGroupFlags, ulTestedFlags);
      ulGroupFlags &= ~(GF_TX0_TX2|GF_SHD);
      ulGroupFlags |=   GF_TX0_TX2_SHD;
      RSRenderGroup( pspoMT, ulGroupFlags, ulTestedFlags);
      return;
    }
  }

  // render one group
  extern INDEX ogl_iMaxBurstSize;
  extern INDEX d3d_iMaxBurstSize;
  ogl_iMaxBurstSize = Clamp( ogl_iMaxBurstSize, (INDEX)0, (INDEX)9999);
  d3d_iMaxBurstSize = Clamp( d3d_iMaxBurstSize, (INDEX)0, (INDEX)9999);

  const INDEX iMaxBurstSize = (eAPI == GAT_OGL) ? ogl_iMaxBurstSize : d3d_iMaxBurstSize;

  // if unlimited lock count
  if( iMaxBurstSize==0)
  { // render whole group
    RSRenderGroupInternal( pspoGroup, ulGroupFlags);
  }
  // if lock count is specified
  else
  { // render group in segments
    while( pspoGroup!=NULL)
    { // find segment size
      INDEX ctVtx = 0;
      ScenePolygon *pspoThis = pspoGroup;
      ScenePolygon *pspoLast = pspoGroup;
      while( ctVtx<iMaxBurstSize && pspoGroup!=NULL) {
        ctVtx    += pspoGroup->spo_ctVtx;
        pspoLast  = pspoGroup;
        pspoGroup = pspoGroup->spo_pspoSucc;
      } // render one group segment
      pspoLast->spo_pspoSucc = NULL;
      RSRenderGroupInternal( pspoThis, ulGroupFlags);
    }
  }
}



// internal group rendering routine
void RSRenderGroupInternal( ScenePolygon *pspoGroup, ULONG ulGroupFlags)
{
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_RENDERGROUPINTERNAL);
  _pfGfxProfile.IncrementCounter( CGfxProfile::PCI_RS_POLYGONGROUPS);

  // make vertex coordinates for all polygons in the group
  RSMakeVertexCoordinates(pspoGroup);
  // prepare vertex, texture and color arrays
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_LOCKARRAYS);
  gfxSetVertexArray( &_avtxPass[0], _avtxPass.Count());
  if(CVA_bWorld) gfxLockArrays();
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_LOCKARRAYS);

  // set alpha keying if required
  if( ulGroupFlags & GF_KEY) gfxEnableAlphaTest();
  else gfxDisableAlphaTest();
  
  _iLastUnit = 0; // reset mulitex unit change
  BOOL bUsedMT = FALSE;
  BOOL bUsesMT = ulGroupFlags & (GF_TX0_TX1 | GF_TX0_TX2 | GF_TX0_SHD | GF_TX2_SHD 
                              |  GF_TX0_TX1_TX2 | GF_TX0_TX1_SHD | GF_TX0_TX2_SHD
                              |  GF_TX0_TX1_TX2_SHD);
  // dual texturing
  if( ulGroupFlags & GF_TX0_SHD) {
    RSSetTextureCoords( pspoGroup, SHADOWTEXTURE, 1);
    RSSetTextureCoords( pspoGroup, 0, 0);
    RSSetTextureColors( pspoGroup, GF_TX0|GF_SHD);
    RSRenderTEX_SHD( pspoGroup, 0);
    bUsedMT = TRUE;
  }
  else if( ulGroupFlags & GF_TX0_TX1) {
    RSSetTextureCoords( pspoGroup, 1, 1);
    RSSetTextureCoords( pspoGroup, 0, 0);
    RSSetTextureColors( pspoGroup, GF_TX0|GF_TX1);
    RSRender2TEX( pspoGroup, 1);
    bUsedMT = TRUE;
  } 
  else if( ulGroupFlags & GF_TX0_TX2) {
    RSSetTextureCoords( pspoGroup, 2, 1);
    RSSetTextureCoords( pspoGroup, 0, 0);
    RSSetTextureColors( pspoGroup, GF_TX0|GF_TX2);
    RSRender2TEX( pspoGroup, 2);
    bUsedMT = TRUE;
  }

  // triple texturing
  else if( ulGroupFlags & GF_TX0_TX1_TX2) {
    RSSetTextureCoords( pspoGroup, 2, 2);
    RSSetTextureCoords( pspoGroup, 1, 1);
    RSSetTextureCoords( pspoGroup, 0, 0);
    RSSetTextureColors( pspoGroup, GF_TX0|GF_TX1|GF_TX2);
    RSRender3TEX( pspoGroup);
    bUsedMT = TRUE;
  }
  else if( ulGroupFlags & GF_TX0_TX1_SHD) {
    RSSetTextureCoords( pspoGroup, SHADOWTEXTURE, 2);
    RSSetTextureCoords( pspoGroup, 1, 1);
    RSSetTextureCoords( pspoGroup, 0, 0);
    RSSetTextureColors( pspoGroup, GF_TX0|GF_TX1|GF_SHD);
    RSRender2TEX_SHD( pspoGroup, 1);
    bUsedMT = TRUE;
  }
  else if( ulGroupFlags & GF_TX0_TX2_SHD) {
    RSSetTextureCoords( pspoGroup, SHADOWTEXTURE, 2);
    RSSetTextureCoords( pspoGroup, 2, 1);
    RSSetTextureCoords( pspoGroup, 0, 0);
    RSSetTextureColors( pspoGroup, GF_TX0|GF_TX2|GF_SHD);
    RSRender2TEX_SHD( pspoGroup, 2);
    bUsedMT = TRUE;
  }

  // quad texturing
  else if( ulGroupFlags & GF_TX0_TX1_TX2_SHD) {
    RSSetTextureCoords( pspoGroup, SHADOWTEXTURE, 3);
    RSSetTextureCoords( pspoGroup, 2, 2);
    RSSetTextureCoords( pspoGroup, 1, 1);
    RSSetTextureCoords( pspoGroup, 0, 0);
    RSSetTextureColors( pspoGroup, GF_TX0|GF_TX1|GF_TX2|GF_SHD);
    RSRender3TEX_SHD( pspoGroup);
    bUsedMT = TRUE;
  }

  // if something was drawn and alpha keying was used
  if( bUsedMT && (ulGroupFlags&GF_KEY)) {
    // force z-buffer test to equal and disable subsequent alpha tests
    gfxDepthFunc( GFX_EQUAL);
    gfxDisableAlphaTest();
  }

  // dual texturing leftover
  if( ulGroupFlags & GF_TX2_SHD) {
    RSSetTextureCoords( pspoGroup, SHADOWTEXTURE, 1);
    RSSetTextureCoords( pspoGroup, 2, 0);
    RSSetTextureColors( pspoGroup, GF_TX2|GF_SHD);
    RSRenderTEX_SHD( pspoGroup, 2);
    bUsedMT = TRUE;
  }

  ASSERT( !bUsedMT==!bUsesMT);

  // if some multi-tex units were used
  if( bUsesMT) {
    // disable them now
    for( INDEX i=1; i<_ctUsableTexUnits; i++) {
      gfxSetTextureUnit(i);
      gfxDisableTexture();
    }
    _iLastUnit = 0;
    gfxSetTextureUnit(0);
  }
  
  // if group has color for first layer
  if( ulGroupFlags&GF_FLAT)
  { // render colors
    if( _bTranslucentPass) {
      // set opacity to 50%
      if( !wld_bRenderTextures) RSSetConstantColors( 0x3F3F3F7F);
      else RSSetPolygonColors( pspoGroup, 0x7F);
      gfxEnableBlend();
      gfxBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
    } else {
      // set opacity to 100%
      if( !wld_bRenderTextures) RSSetConstantColors( 0x7F7F7FFF);
      else RSSetPolygonColors( pspoGroup, CT_OPAQUE);
      gfxDisableBlend();
    }
    gfxDisableTexture();
    DrawAllElements( pspoGroup);
  }   

  // if group has texture for first layer
  if( ulGroupFlags&GF_TX0) {
    // render texture 0
    RSSetTextureCoords( pspoGroup, 0, 0);
    RSSetTextureColors( pspoGroup, GF_TX0);
    RSRenderTEX( pspoGroup, 0);
    // eventually prepare subsequent layers for transparency
    if( ulGroupFlags & GF_KEY) {
      gfxDepthFunc( GFX_EQUAL);
      gfxDisableAlphaTest();
    }
  }
  // if group has texture for second layer
  if( ulGroupFlags & GF_TX1) {
    // render texture 1
    RSSetTextureCoords( pspoGroup, 1, 0);
    RSSetTextureColors( pspoGroup, GF_TX1);
    RSRenderTEX( pspoGroup, 1);
  }
  // if group has texture for third layer
  if( ulGroupFlags & GF_TX2) {
    // render texture 2
    RSSetTextureCoords( pspoGroup, 2, 0);
    RSSetTextureColors( pspoGroup, GF_TX2);
    RSRenderTEX( pspoGroup, 2);
  }

  // if group has shadow
  if( ulGroupFlags & GF_SHD) {
    // render shadow
    RSSetTextureCoords( pspoGroup, SHADOWTEXTURE, 0);
    RSSetTextureColors( pspoGroup, GF_SHD);
    RSRenderSHD( pspoGroup);
  }

  // if group has aftershadow texture for second layer
  if( ulGroupFlags & GF_TA1) {
    // render texture 1
    RSSetTextureCoords( pspoGroup, 1, 0);
    RSSetTextureColors( pspoGroup, GF_TX1);
    RSRenderTEX( pspoGroup, 1);
  }
  // if group has aftershadow texture for third layer
  if( ulGroupFlags & GF_TA2) {
    // render texture 2
    RSSetTextureCoords( pspoGroup, 2, 0);
    RSSetTextureColors( pspoGroup, GF_TX2);
    RSRenderTEX( pspoGroup, 2);
  }

  // if group has fog
  if( ulGroupFlags & GF_FOG) {
    // render fog
    RSStartupFog();
    RSSetConstantColors( _fog_fp.fp_colColor);
    RSSetFogCoordinates( pspoGroup);
    RSRenderFog( pspoGroup);
  }
  // if group has haze
  if( ulGroupFlags & GF_HAZE) {
    // render haze
    RSStartupHaze();
    RSSetConstantColors( _haze_hp.hp_colColor);
    RSSetHazeCoordinates( pspoGroup);
    RSRenderHaze( pspoGroup);
  }

  // reset depth function and alpha keying back
  // (maybe it was altered for transparent polygon rendering)
  gfxDepthFunc( GFX_LESS_EQUAL);
  gfxDisableAlphaTest();
  
  // if group has selection
  if( ulGroupFlags & GF_SEL) {
    // render selection
    gfxEnableBlend();
    gfxBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA); 
    RSSetConstantColors( _colSelection|128);
    gfxDisableTexture();
    DrawAllElements( pspoGroup);
  }
  
  // render triangle wireframe if needed
  extern INDEX wld_bShowTriangles;
  if( wld_bShowTriangles) {
    gfxEnableBlend();
    gfxBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA); 
    RSSetConstantColors( C_mdYELLOW|222);
    gfxDisableTexture();
    // must write to front in z-buffer
    gfxPolygonMode(GFX_LINE);
    gfxEnableDepthTest();
    gfxEnableDepthWrite();
    gfxDepthFunc(GFX_ALWAYS);
    gfxDepthRange( 0,0);
    DrawAllElements(pspoGroup);
    gfxDepthRange( _ppr->pr_fDepthBufferNear, _ppr->pr_fDepthBufferFar);
    gfxDepthFunc(GFX_LESS_EQUAL);
    if( _bTranslucentPass) gfxDisableDepthWrite();
    gfxPolygonMode(GFX_FILL);
  }

  // all done
  gfxUnlockArrays();
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_RENDERGROUPINTERNAL);
}


static void RSPrepare(void)
{
  // set general params
  gfxCullFace(GFX_NONE);
  gfxEnableDepthTest();
  gfxEnableClipping();
}


static void RSEnd(void)
{
  // reset unusual gfx API parameters
  gfxSetTextureUnit(0);
  gfxSetTextureModulation(1);
}


void RenderScene( CDrawPort *pDP, ScenePolygon *pspoFirst, CAnyProjection3D &prProjection,
                  COLOR colSelection, BOOL bTranslucent)
{
  // check API
  eAPI = _pGfx->gl_eCurrentAPI;
  ASSERT( GfxValidApi(eAPI) );

#ifdef SE1_D3D
  if( eAPI!=GAT_OGL && eAPI!=GAT_D3D) return;
#else
  if( eAPI!=GAT_OGL) return;
#endif

  // some cvars cannot be altered in multiplayer mode!
  if( _bMultiPlayer) {
    shd_bShowFlats = FALSE;
    gfx_bRenderWorld = TRUE;
    wld_bRenderShadowMaps = TRUE;
    wld_bRenderTextures = TRUE; 
    wld_bRenderDetailPolygons = TRUE;
    wld_bShowDetailTextures = FALSE;
    wld_bShowTriangles = FALSE;
  }

  // skip if not rendering world
  if( !gfx_bRenderWorld) return;

  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RENDERSCENE);

  // remember input parameters
  ASSERT( pDP != NULL);
  _ppr = (CPerspectiveProjection3D*)&*prProjection;
  _pDP = pDP;
  _colSelection = colSelection;
  _bTranslucentPass = bTranslucent;

  // clamp detail textures LOD biasing
  wld_iDetailRemovingBias = Clamp( wld_iDetailRemovingBias, (INDEX)-9, (INDEX)9);

  // set perspective projection
  _pDP->SetProjection(prProjection);

  // adjust multi-texturing support (for clip-plane emulation thru texture units)
  extern BOOL  GFX_bClipPlane; // WATCHOUT: set by 'SetProjection()' !
  extern INDEX gap_iUseTextureUnits;
  extern INDEX ogl_bAlternateClipPlane;
  INDEX ctMaxUsableTexUnits = _pGfx->gl_ctTextureUnits;
  if( eAPI==GAT_OGL && ogl_bAlternateClipPlane && GFX_bClipPlane && ctMaxUsableTexUnits>1) ctMaxUsableTexUnits--;
  _ctUsableTexUnits = Clamp( gap_iUseTextureUnits, (INDEX)1, ctMaxUsableTexUnits);

  // prepare
  RSPrepare();

  // turn depth buffer writing on or off
  if( bTranslucent) gfxDisableDepthWrite();
  else gfxEnableDepthWrite();

  // remove all polygons with no triangles from the polygon list
  ScenePolygon *pspoNonDummy;
  RSRemoveDummyPolygons( pspoFirst, &pspoNonDummy);

  // check that layers of all shadows are up to date
  RSCheckLayersUpToDate(pspoNonDummy);

  // bin polygons to groups by texture passes
  RSBinToGroups(pspoNonDummy);

  // for each group                           
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RS_RENDERGROUP);
  ASSERT( _apspoGroups[0]==NULL); // zero group must always be empty
  for( INDEX iGroup=1; iGroup<_ctGroupsCount; iGroup++) {
    // get the group polygon list and render it if not empty
    ScenePolygon *pspoGroup = _apspoGroups[iGroup];
    if( pspoGroup!=NULL) RSRenderGroup( pspoGroup, iGroup, 0);
  }
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RS_RENDERGROUP);

  // all done
  RSEnd();
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RENDERSCENE);
}


// renders only scene z-buffer
void RenderSceneZOnly( CDrawPort *pDP, ScenePolygon *pspoFirst, CAnyProjection3D &prProjection)
{
  if( _bMultiPlayer) gfx_bRenderWorld = 1;
  if( !gfx_bRenderWorld) return;
  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RENDERSCENE_ZONLY);

  // set perspective projection
  ASSERT(pDP!=NULL);
  pDP->SetProjection(prProjection);

  // prepare
  RSPrepare();

  // set for depth-only rendering
  const ULONG ulCurrentColorMask = gfxGetColorMask();
  gfxSetColorMask(NONE);
  gfxEnableDepthTest();
  gfxEnableDepthWrite();
  gfxDisableTexture();

  // make vertex coordinates for all polygons in the group and render the polygons
  RSMakeVertexCoordinates(pspoFirst);
  gfxSetVertexArray( &_avtxPass[0], _avtxPass.Count());
  gfxDisableColorArray();
  DrawAllElements(pspoFirst);

  // restore color masking
  gfxSetColorMask( ulCurrentColorMask);
  RSEnd();

  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RENDERSCENE_ZONLY);
}


// renders flat background of the scene
void RenderSceneBackground(CDrawPort *pDP, COLOR col)
{
  if( _bMultiPlayer) gfx_bRenderWorld = 1;
  if( !gfx_bRenderWorld) return;

  // set orthographic projection
  ASSERT(pDP!=NULL);
  pDP->SetOrtho();

  _pfGfxProfile.StartTimer( CGfxProfile::PTI_RENDERSCENE_BCG);
  // prepare
  gfxEnableDepthTest();
  gfxDisableDepthWrite();
  gfxDisableBlend();
  gfxDisableAlphaTest();
  gfxDisableTexture();
  gfxEnableClipping();

  col = AdjustColor( col, _slTexHueShift, _slTexSaturation);
  GFXColor glcol(col|CT_OPAQUE);
  const INDEX iW = pDP->GetWidth();
  const INDEX iH = pDP->GetHeight();

  // set arrays
  gfxResetArrays();
  GFXVertex   *pvtx = _avtxCommon.Push(4);
  /* GFXTexCoord *ptex = */ _atexCommon.Push(4);
  GFXColor    *pcol = _acolCommon.Push(4);
  pvtx[0].x =  0;  pvtx[0].y =  0;  pvtx[0].z = 1;
  pvtx[1].x =  0;  pvtx[1].y = iH;  pvtx[1].z = 1;
  pvtx[2].x = iW;  pvtx[2].y = iH;  pvtx[2].z = 1;
  pvtx[3].x = iW;  pvtx[3].y =  0;  pvtx[3].z = 1;
  pcol[0] = glcol;
  pcol[1] = glcol;
  pcol[2] = glcol;
  pcol[3] = glcol;

  // render
  _pGfx->gl_ctWorldTriangles += 2; 
  gfxFlushQuads();
  _pfGfxProfile.StopTimer( CGfxProfile::PTI_RENDERSCENE_BCG);
}
