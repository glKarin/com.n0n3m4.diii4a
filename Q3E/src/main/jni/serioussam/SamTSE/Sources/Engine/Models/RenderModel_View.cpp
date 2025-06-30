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

#include "Engine/StdH.h"

#include <Engine/Base/Statistics_Internal.h>
#include <Engine/Base/Console.h>
#include <Engine/Models/ModelObject.h>
#include <Engine/Models/ModelData.h>
#include <Engine/Models/ModelProfile.h>
#include <Engine/Models/RenderModel.h>
#include <Engine/Models/Model_internal.h>
#include <Engine/Models/Normals.h>
#include <Engine/Graphics/GfxLibrary.h>
#include <Engine/Graphics/Fog_internal.h>
#include <Engine/Base/Lists.inl>
#include <Engine/World/WorldEditingProfile.h>

#include <Engine/Templates/StaticArray.cpp>
#include <Engine/Templates/StaticStackArray.cpp>

#include <Engine/Models/RenderModel_internal.h>

// asm shortcuts
#define O offset
#define Q qword ptr
#define D dword ptr
#define W  word ptr
#define B  byte ptr

extern INDEX mdl_bRenderBump;

extern BOOL CVA_bModels;
extern BOOL GFX_bTruform;
extern BOOL _bMultiPlayer;

extern const UBYTE *pubClipByte;
extern const FLOAT *pfSinTable;
extern const FLOAT *pfCosTable;

static GfxAPIType _eAPI;

static BOOL  _bForceTranslucency;    // force translucency of opaque/transparent surfaces (for model fading)
static ULONG _ulMipLayerFlags;
static INDEX _icol=0;


// mip arrays
static CStaticStackArray<GFXVertex3>  _avtxMipBase;
static CStaticStackArray<GFXTexCoord> _atexMipBase;  // for reflection and specular
static CStaticStackArray<GFXNormal3>  _anorMipBase;
static CStaticStackArray<GFXColor>    _acolMipBase;

static CStaticStackArray<GFXTexCoord> _atexMipFogy;
static CStaticStackArray<UBYTE>       _ashdMipFogy;
static CStaticStackArray<FLOAT>       _atx1MipHaze;
static CStaticStackArray<UBYTE>       _ashdMipHaze;

// surface arrays
static CStaticStackArray<GFXVertex4>  _avtxSrfBase;
static CStaticStackArray<GFXNormal4>  _anorSrfBase;  // normals for Truform!
static CStaticStackArray<GFXTexCoord> _atexSrfBase;
static CStaticStackArray<GFXTexCoord> _atexSrfBump; // Bump maping
static CStaticStackArray<GFXColor>    _acolSrfBase;

// shadows arrays
static CStaticStackArray<FLOAT>        _aooqMipShad;
static CStaticStackArray<GFXTexCoord4> _atx4SrfShad;


// pointers to arrays for quicker access
static GFXColor    *pcolSrfBase;
static GFXColor    *pcolMipBase;
static GFXVertex3  *pvtxMipBase;
static GFXNormal3  *pnorMipBase;
static GFXTexCoord *ptexMipBase;
static GFXTexCoord *ptexMipFogy;
static UBYTE       *pshdMipFogy;
static FLOAT       *ptx1MipHaze;
static UBYTE       *pshdMipHaze;
static FLOAT       *pooqMipShad;
static UWORD       *puwSrfToMip;

// misc
static ULONG _ulColorMask = 0;
static INDEX _ctAllMipVx  = 0;
static INDEX _ctAllSrfVx  = 0;
static BOOL  _bFlatFill   = FALSE;
static BOOL  _bHasBump    = FALSE;
static SLONG _slLR=0, _slLG=0, _slLB=0;
static SLONG _slAR=0, _slAG=0, _slAB=0;

#if (defined __GNUC__)
static const __int64 mmRounder = 0x007F007F007F007Fll;
static const __int64 mmF000    = 0x00FF000000000000ll;
#else
static const __int64 mmRounder = (__int64) 0x007F007F007F007F;
static const __int64 mmF000    = (__int64) 0x00FF000000000000;
#endif

// viewer absolute and object space projection
static FLOAT3D _vViewer;
static FLOAT3D _vViewerObj;
static FLOAT3D _vLightObj;

// some constants for asm float ops
static const FLOAT f2  = 2.0f;
static const FLOAT f05 = 0.5f;


// convinient routine for timing of texture setting
static __forceinline void SetCurrentTexture( CTextureData *ptd, INDEX iFrame)
{
  _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_SETTEXTURE);
  _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_SETTEXTURE);
  if( ptd==NULL || _bFlatFill) gfxDisableTexture();
  else ptd->SetAsCurrent(iFrame);
  _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_SETTEXTURE);
}


// reset model vertex buffers
static void ResetVertexArrays(void)
{
  _avtxMipBase.PopAll();
  _atexMipBase.PopAll();
  _acolMipBase.PopAll();
  _anorMipBase.PopAll();

  _avtxSrfBase.PopAll();
  _anorSrfBase.PopAll();
  _atexSrfBase.PopAll();
  _atexSrfBump.PopAll();
  _acolSrfBase.PopAll();

  _atexMipFogy.PopAll();
  _ashdMipFogy.PopAll();
  _atx1MipHaze.PopAll();
  _ashdMipHaze.PopAll();
}


// reset and free all arrays
extern void Models_ClearVertexArrays(void)
{
  _avtxMipBase.Clear();
  _atexMipBase.Clear();
  _acolMipBase.Clear();
  _anorMipBase.Clear();

  _atexMipFogy.Clear();
  _ashdMipFogy.Clear();
  _atx1MipHaze.Clear();
  _ashdMipHaze.Clear();

  _avtxSrfBase.Clear();
  _anorSrfBase.Clear();
  _atexSrfBase.Clear();
  _atexSrfBump.Clear();
  _acolSrfBase.Clear();

  _aooqMipShad.Clear();
  _atx4SrfShad.Clear();
}


struct Triangle {
  INDEX i0;
  INDEX i1;
  INDEX i2;
  BOOL  bDone;
  Triangle(void) : bDone(FALSE) {};
  void Clear(void) { bDone = FALSE; };
  void Rotate(void) { const INDEX i=i0;
                      i0=i1; i1=i2; i2=i; }
};

static INDEX _ctSurfaces  = 0;
static INDEX _ctTriangles = 0;
static INDEX _ctStrips    = 0;
static INDEX _ctMaxStripLen = 0;
static INDEX _ctMinStripLen = 10000;
static FLOAT _fTriPerStrip     = 0.0f;
static FLOAT _fTriPerSurface   = 0.0f;
static FLOAT _fStripPerSurface = 0.0f;
static INDEX _ctStripStartFailed = 0;


static CStaticArray<Triangle> _atri;
static CStaticArray<Triangle> _atriDone;
static void GetNeighbourTriangleVertices(Triangle *ptri, INDEX &i1, INDEX &i2)
{
  Triangle &triA = *ptri;
  for( INDEX itri=0; itri<_atri.Count(); itri++) {
    Triangle &triB = _atri[itri];
    if( triB.bDone) continue;
    i1=triA.i1; i2=triA.i2;
    if (i2 == triB.i0 && i1 == triB.i1) return;
    if (i2 == triB.i1 && i1 == triB.i2) return;
    if (i2 == triB.i2 && i1 == triB.i0) return; 
    triA.Rotate();
    i1=triA.i1; i2=triA.i2;
    if (i2 == triB.i0 && i1 == triB.i1) return;
    if (i2 == triB.i1 && i1 == triB.i2) return;
    if (i2 == triB.i2 && i1 == triB.i0) return; 
    triA.Rotate();
    i1=triA.i1; i2=triA.i2;
    if (i2 == triB.i0 && i1 == triB.i1) return;
    if (i2 == triB.i1 && i1 == triB.i2) return;
    if (i2 == triB.i2 && i1 == triB.i0) return; 
    triA.Rotate();
  }
  // none found
  i1 = -1; 
  i2 = -1;
  _ctStripStartFailed++;
}


static Triangle *GetNextStripTriangle(INDEX &i1, INDEX &i2, INDEX iTriInStrip)
{
  for( INDEX itri=0; itri<_atri.Count(); itri++) {
    Triangle &tri = _atri[itri];
    if( tri.bDone) continue;
    if( iTriInStrip%2 == 0) {
      if (i1==tri.i0 && i2==tri.i1) { i1=i2; i2=tri.i2; return &tri;}  tri.Rotate();
      if (i1==tri.i0 && i2==tri.i1) { i1=i2; i2=tri.i2; return &tri;}  tri.Rotate();
      if (i1==tri.i0 && i2==tri.i1) { i1=i2; i2=tri.i2; return &tri;}  tri.Rotate();
    } else {
      if (i2==tri.i0 && i1==tri.i1) { i1=i2; i2=tri.i2; return &tri;}  tri.Rotate();
      if (i2==tri.i0 && i1==tri.i1) { i1=i2; i2=tri.i2; return &tri;}  tri.Rotate();
      if (i2==tri.i0 && i1==tri.i1) { i1=i2; i2=tri.i2; return &tri;}  tri.Rotate();
    }
  }
  i1 = -1;
  i2 = -1;
  return NULL;
}

static Triangle *GetFirstTriangle(void)
{
  for( INDEX itri=0; itri<_atri.Count(); itri++) {
    Triangle &tri1 = _atri[itri];
    if( tri1.bDone) continue;
    return &tri1;
  }
  ASSERTALWAYS( "WTF?");
  return &_atri[0];
}


static void PrepareSurfaceElements( ModelMipInfo &mmi, MappingSurface &ms)
{
  _ctSurfaces++;
  // find total number of triangles
  INDEX ctTriangles = 0;
  {for( INDEX iipo=0; iipo<ms.ms_aiPolygons.Count(); iipo++) {
    ModelPolygon &mp = mmi.mmpi_Polygons[ ms.ms_aiPolygons[iipo]];
    ctTriangles += (mp.mp_PolygonVertices.Count()-2);
  }}
  ASSERT( ctTriangles>=ms.ms_aiPolygons.Count());

  _ctTriangles += ctTriangles;
  // allocate that much triangles
  _atri.Clear();
  _atri.New(ctTriangles);
  _atriDone.Clear();
  _atriDone.New(ctTriangles);

  // put all triangles there (do tri-fans) -> should do tri-strips ? !!!!
  INDEX iTriangle = 0;
  {for( INDEX iipo=0; iipo<ms.ms_aiPolygons.Count(); iipo++) {
    ModelPolygon &mp = mmi.mmpi_Polygons[ ms.ms_aiPolygons[iipo]];
    {for( INDEX ivx=2; ivx<mp.mp_PolygonVertices.Count(); ivx++) {
      Triangle &tri = _atri[iTriangle++];
      tri.i0 = mp.mp_PolygonVertices[0    ].mpv_ptvTextureVertex->mtv_iSurfaceVx;
      tri.i1 = mp.mp_PolygonVertices[ivx-1].mpv_ptvTextureVertex->mtv_iSurfaceVx;
      tri.i2 = mp.mp_PolygonVertices[ivx-0].mpv_ptvTextureVertex->mtv_iSurfaceVx;
    }}
  }}

  // start with first triangle
  INDEX ctTrianglesDone = 0;
  Triangle *ptri = &_atri[0];
  INDEX i1, i2;
  GetNeighbourTriangleVertices(ptri, i1, i2);
  //_RPT2(_CRT_WARN, "Begin: i1=%d i2=%d\n", i1,i2);

  _ctStrips++;
  INDEX ctTriPerStrip = 0;
  // repeat
  FOREVER {
    // put current triangles into done triangles
    _atriDone[ctTrianglesDone++] = *ptri;
    //_RPT3(_CRT_WARN, "Added: %d %d %d\n", ptri->i0, ptri->i1, ptri->i2);
    ptri->bDone = TRUE;
    ctTriPerStrip++;
    
    // stop if all triangles are done
    if( ctTrianglesDone>=ctTriangles) break;

    // get some neighbour of current triangle
    Triangle *ptriNext = NULL;
    extern INDEX mdl_bCreateStrips;
    if( mdl_bCreateStrips) {
      ptriNext = GetNextStripTriangle( i1, i2, ctTriPerStrip);
      //_RPT2(_CRT_WARN, "Next: i1=%d i2=%d\n", i1,i2);
    }
    // if no neighbour
    if( ptriNext==NULL) {
      // get first one that is not done
      ptriNext = GetFirstTriangle();
      GetNeighbourTriangleVertices( ptriNext, i1, i2);
      //_RPT2(_CRT_WARN, "Rebegin: i1=%d i2=%d\n", i1,i2);
      _ctMaxStripLen = Max( _ctMaxStripLen, ctTriPerStrip);
      _ctMinStripLen = Max( _ctMinStripLen, ctTriPerStrip);
      _ctStrips++;
      ctTriPerStrip = 0;
    }
    // take that as current triangle
    ptri = ptriNext;
  }
  _ctMaxStripLen = Max( _ctMaxStripLen, ctTriPerStrip);
  _ctMinStripLen = Min( _ctMinStripLen, ctTriPerStrip);
  ASSERT( ctTrianglesDone==ctTriangles);

  // create elements
  ms.ms_ctSrfEl = ctTriangles*3;
  INDEX_T *paiElements = mmi.mmpi_aiElements.Push(ms.ms_ctSrfEl);
  // dump all triangles
  //_RPT0(_CRT_WARN, "Result:\n");
  INDEX iel = 0;
  {for( INDEX itri=0; itri<ctTriangles; itri++){
    paiElements[iel++] = _atriDone[itri].i0;
    paiElements[iel++] = _atriDone[itri].i1;
    paiElements[iel++] = _atriDone[itri].i2;
    //_RPT3(_CRT_WARN, "%d %d %d\n", _atriDone[itri].i0, _atriDone[itri].i1, _atriDone[itri].i2);
  }}
  mmi.mmpi_ctTriangles += ctTriangles;

  _fTriPerStrip     = FLOAT(_ctTriangles) / _ctStrips;
  _fTriPerSurface   = FLOAT(_ctTriangles) / _ctSurfaces;
  _fStripPerSurface = FLOAT(_ctStrips)    / _ctSurfaces;
  // _ctStripStartFailed
  _atri.Clear();
  _atriDone.Clear();
}



// compare two surfaces by type of diffuse surface
static int qsort_CompareSurfaceDiffuseTypes( const void *pSrf1, const void *pSrf2)
{
  MappingSurface &srf1 = *(MappingSurface*)pSrf1;
  MappingSurface &srf2 = *(MappingSurface*)pSrf2;
  // invisible, empty or obsolete surfaces goes to end ...
  if( srf1.ms_aiPolygons.Count()==0 ||
     (srf1.ms_ulRenderingFlags&SRF_INVISIBLE) ||
      srf1.ms_sttTranslucencyType==STT_ALPHAGOURAUD) return +1;
  if( srf2.ms_aiPolygons.Count()==0 ||
     (srf2.ms_ulRenderingFlags&SRF_INVISIBLE) ||
      srf2.ms_sttTranslucencyType==STT_ALPHAGOURAUD) return -1;
  // same surface types - sort by specular then reflection layer
  if( srf1.ms_sttTranslucencyType==srf2.ms_sttTranslucencyType) {
    BOOL bSrf1Spec = srf1.ms_ulRenderingFlags & SRF_SPECULAR;
    BOOL bSrf2Spec = srf2.ms_ulRenderingFlags & SRF_SPECULAR;
    BOOL bSrf1Refl = srf1.ms_ulRenderingFlags & SRF_REFLECTIONS;
    BOOL bSrf2Refl = srf2.ms_ulRenderingFlags & SRF_REFLECTIONS;
    if(  bSrf1Spec && !bSrf2Spec) return -1;
    if( !bSrf1Spec &&  bSrf2Spec) return +1;
    if(  bSrf1Refl && !bSrf2Refl) return -1;
    if( !bSrf1Refl &&  bSrf2Refl) return +1;
    // identical surfaces
    return 0;
  }
  // ... opaque surfaces goes to begining ...
  if( srf1.ms_sttTranslucencyType==STT_OPAQUE) return -1;
  if( srf2.ms_sttTranslucencyType==STT_OPAQUE) return +1;
  // ... then transparent ...
  if( srf1.ms_sttTranslucencyType==STT_TRANSPARENT) {
    if( srf2.ms_sttTranslucencyType==STT_OPAQUE) return +1;
    return -1;
  }
  if( srf2.ms_sttTranslucencyType==STT_TRANSPARENT) {
    if( srf1.ms_sttTranslucencyType==STT_OPAQUE) return -1;
    return +1;
  }
  // ... then translucent ...
  if( srf1.ms_sttTranslucencyType==STT_TRANSLUCENT) {
    if( srf2.ms_sttTranslucencyType==STT_OPAQUE ||
        srf2.ms_sttTranslucencyType==STT_TRANSPARENT) return +1;
    return -1;
  }
  if( srf2.ms_sttTranslucencyType==STT_TRANSLUCENT) {
    if( srf1.ms_sttTranslucencyType==STT_OPAQUE ||
        srf1.ms_sttTranslucencyType==STT_TRANSPARENT) return -1;
    return +1;
  }
  // ... then additive ...
  if( srf1.ms_sttTranslucencyType==STT_ADD) {
    if( srf2.ms_sttTranslucencyType==STT_MULTIPLY) return -1;
    return +1;
  }
  if( srf2.ms_sttTranslucencyType==STT_ADD) {
    if( srf1.ms_sttTranslucencyType==STT_MULTIPLY) return +1;
    return -1;
  }
  // ... then multiplicative.
  if( srf1.ms_sttTranslucencyType==STT_MULTIPLY) return +1;
  if( srf2.ms_sttTranslucencyType==STT_MULTIPLY) return -1;
  ASSERTALWAYS( "Unrecognized surface type!");
  return 0;
}


// prepare mip model's array for OpenGL
static void PrepareModelMipForRendering( CModelData &md, INDEX iMip)
{
  ModelMipInfo &mmi = md.md_MipInfos[iMip];
  mmi.mmpi_ulLayerFlags = MMI_OPAQUE|MMI_TRANSLUCENT; // initially entire model can be both opaque and translucent
  mmi.mmpi_ctTriangles  = 0;

  // get number of vertices in this mip
  INDEX ctMdlVx = md.md_VerticesCt;
  INDEX ctMipVx = 0;
  INDEX iMdlVx;
  ULONG ulVxMask = 1UL<<iMip;
  for( iMdlVx=0; iMdlVx<ctMdlVx; iMdlVx++) {
    if( md.md_VertexMipMask[iMdlVx] & ulVxMask) ctMipVx++;
  }

  // create model<->mip remapping tables for vertices
  mmi.mmpi_ctMipVx = ctMipVx;
  mmi.mmpi_auwMipToMdl.Clear();
  mmi.mmpi_auwMipToMdl.New(ctMipVx);
  CStaticArray<INDEX> aiMdlToMip;
  aiMdlToMip.New(ctMdlVx);
  INDEX iMipVx = 0;
  for( iMdlVx=0; iMdlVx<ctMdlVx; iMdlVx++) {
    aiMdlToMip[iMdlVx] = 0x12345678;  // set to invalid to catch eventual bugs
    if((md.md_VertexMipMask[iMdlVx] & ulVxMask)) {
      aiMdlToMip[iMdlVx]= iMipVx;
      mmi.mmpi_auwMipToMdl[iMipVx++] = iMdlVx;
    }
  }

  // get total number of surface vertices
  CStaticArray<struct ModelTextureVertex> &amtv = mmi.mmpi_TextureVertices;
  mmi.mmpi_ctSrfVx = amtv.Count();

  // allocate surface vertex arrays
  mmi.mmpi_auwSrfToMip.Clear();
  mmi.mmpi_avmexTexCoord.Clear();
  mmi.mmpi_auwSrfToMip.New(mmi.mmpi_ctSrfVx);
  mmi.mmpi_avmexTexCoord.New(mmi.mmpi_ctSrfVx);

  // alloc bump mapping vectors only if needed
  mmi.mmpi_avBumpU.Clear();
  mmi.mmpi_avBumpV.Clear();
  BOOL bBumpAllocated = FALSE;

  // count surfaces
  INDEX ctSurfaces = 0;
  {FOREACHINSTATICARRAY( mmi.mmpi_MappingSurfaces, MappingSurface, itms) ctSurfaces++; }
  // sort surfaces by diffuse type
  qsort( &mmi.mmpi_MappingSurfaces[0], ctSurfaces, sizeof(MappingSurface), qsort_CompareSurfaceDiffuseTypes);

  // initialize array for all surfaces' elements
  mmi.mmpi_aiElements.Clear();

  // for each surface
  INDEX iSrfVx = 0;
  INDEX iSrfEl = 0;
  {FOREACHINSTATICARRAY( mmi.mmpi_MappingSurfaces, MappingSurface, itms)
  {
    MappingSurface &ms = *itms;
    // if it is empty surface
    if( ms.ms_aiPolygons.Count()==0) {
      // just clear all its data
      ms.ms_ctSrfVx = 0;
      ms.ms_ctSrfEl = 0;
      ms.ms_iSrfVx0 = MAX_SLONG;  // set to invalid to catch eventual bugs
      // proceed to next surface
      continue;
    }

    // determine surface and mip model rendering type (write to z-buffer or not)
   const BOOL bBump = ms.ms_ulRenderingFlags&SRF_BUMP;

    if( !(ms.ms_ulRenderingFlags&SRF_DIFFUSE)
       || ms.ms_sttTranslucencyType==STT_TRANSLUCENT
       || ms.ms_sttTranslucencyType==STT_ADD
       || ms.ms_sttTranslucencyType==STT_MULTIPLY) {
      ms.ms_ulRenderingFlags &= ~SRF_OPAQUE;
      mmi.mmpi_ulLayerFlags  &= ~MMI_OPAQUE;
    } else {
      ms.ms_ulRenderingFlags |=  SRF_OPAQUE;
      mmi.mmpi_ulLayerFlags  &= ~MMI_TRANSLUCENT;
    }
    // accumulate flags
    mmi.mmpi_ulLayerFlags |= ms.ms_ulRenderingFlags;

    // alloc memory for bump coords if needed
    if( bBump && !bBumpAllocated) {
      mmi.mmpi_avBumpU.New(mmi.mmpi_ctSrfVx);
      mmi.mmpi_avBumpV.New(mmi.mmpi_ctSrfVx);
      bBumpAllocated = TRUE;
    }

    // assign surface vertex numbers
    ms.ms_iSrfVx0 = iSrfVx;
    ms.ms_ctSrfVx = ms.ms_aiTextureVertices.Count();
    // for each vertex
    for( INDEX iVxInSurface=0; iVxInSurface<ms.ms_ctSrfVx; iVxInSurface++) {
      // get texture vertex
      ModelTextureVertex &mtv = amtv[ms.ms_aiTextureVertices[iVxInSurface]];
      // remember index for elements preparing
      mtv.mtv_iSurfaceVx = iSrfVx;
      // remember index for rendering
      mmi.mmpi_auwSrfToMip[iSrfVx] = aiMdlToMip[mtv.mtv_iTransformedVertex];
      // assign data to texture array
      mmi.mmpi_avmexTexCoord[iSrfVx](1) = (FLOAT)mtv.mtv_UV(1);
      mmi.mmpi_avmexTexCoord[iSrfVx](2) = (FLOAT)mtv.mtv_UV(2);

      // assign bump mapping normals (only if surface has bump mapping)
      if( bBump) {
        mmi.mmpi_avBumpU[iSrfVx] = mtv.mtv_vU;
        mmi.mmpi_avBumpV[iSrfVx] = mtv.mtv_vV;
      }
      //

      iSrfVx++;
    } // set vertex indices
    PrepareSurfaceElements( mmi, ms);
  }}

  // for each patch
  FOREACHINSTATICARRAY( mmi.mmpi_aPolygonsPerPatch, PolygonsPerPatch, itppp)
  {
    PolygonsPerPatch &ppp = *itppp;
    // find total number of triangles
    INDEX ctTriangles = 0;
    INDEX iipo;
    for( iipo=0; iipo<ppp.ppp_iPolygons.Count(); iipo++) {
      ModelPolygon &mp = mmi.mmpi_Polygons[ ppp.ppp_iPolygons[iipo]];
      ctTriangles += (mp.mp_PolygonVertices.Count()-2);
    }

    // allocate that much elements
    ppp.ppp_auwElements.Clear();
    ppp.ppp_auwElements.New(ctTriangles*3);

    // put all triangles there (do tri-fans) -> should do tri-strips ? !!!!
    INDEX iel = 0;
    for( iipo=0; iipo<ppp.ppp_iPolygons.Count(); iipo++) {
      ModelPolygon &mp = mmi.mmpi_Polygons[ ppp.ppp_iPolygons[iipo]];
      for( INDEX ivx=2; ivx<mp.mp_PolygonVertices.Count(); ivx++) {
        ppp.ppp_auwElements[iel++] = mp.mp_PolygonVertices[0    ].mpv_ptvTextureVertex->mtv_iSurfaceVx;
        ppp.ppp_auwElements[iel++] = mp.mp_PolygonVertices[ivx-1].mpv_ptvTextureVertex->mtv_iSurfaceVx;
        ppp.ppp_auwElements[iel++] = mp.mp_PolygonVertices[ivx-0].mpv_ptvTextureVertex->mtv_iSurfaceVx;
      }
    }
  }
}


extern void PrepareModelForRendering( CModelData &md)
{
  // do nothing, if the model has already been initialized for rendering
  if( md.md_bPreparedForRendering) return;
  _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_PREPAREFORRENDERING);
  _pfWorldEditingProfile.StartTimer(CWorldEditingProfile::PTI_TRISTRIPMODELS);
  // prepare each mip model
  for( INDEX iMip=0; iMip<md.md_MipCt; iMip++) PrepareModelMipForRendering( md, iMip);
  // mark as prepared
  md.md_bPreparedForRendering = TRUE;
  // all done
  _pfWorldEditingProfile.StopTimer(CWorldEditingProfile::PTI_TRISTRIPMODELS);
  _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_PREPAREFORRENDERING);
}




// strip rendering support
// INDEX _icol=0;
COLOR _acol[] = { C_BLACK,   C_WHITE,
                  C_dGRAY,   C_GRAY,   C_lGRAY,      C_dRED,     C_RED,     C_lRED,
                  C_dGREEN,  C_GREEN,  C_lGREEN,     C_dBLUE,    C_BLUE,    C_lBLUE,
                  C_dCYAN,   C_CYAN,   C_lCYAN,      C_dMAGENTA, C_MAGENTA, C_lMAGENTA,
                  C_dYELLOW, C_YELLOW, C_lYELLOW,    C_dORANGE,  C_ORANGE,  C_lORANGE,
                  C_dBROWN,  C_BROWN,  C_lBROWN,     C_dPINK,    C_PINK,    C_lPINK };

const INDEX _ctcol = sizeof(_acol)/sizeof(_acol[0]);

static void SetCol(void)
{
  glCOLOR( _acol[_icol]);
  _icol = (_icol+1)%_ctcol;
}

#ifdef _GLES //karin: using glDrawElements instead of glArrayElement
#include <vector>
#endif
static void DrawStrips( const INDEX ct, const INDEX_T *pai)
{
  // set strip color
  pglDisableClientState( GL_COLOR_ARRAY);
  gfxDisableTexture();
  SetCol();

  // render elements as strips
#ifdef _GLES //karin: using glDrawElements instead of glArrayElement
  static std::vector<INDEX> is;
  is.clear();
#else
  pglBegin( GL_TRIANGLE_STRIP);
#endif
  INDEX ctMaxTriPerStrip = 0;
  INDEX i = 0;
  INDEX iInStrip = 0;
  INDEX iL0, iL1;

  while( i<ct/3)
  {
    INDEX_T i0 = pai[i*3+0];
    INDEX_T i1 = pai[i*3+1];
    INDEX_T i2 = pai[i*3+2];
    ctMaxTriPerStrip = Max( ctMaxTriPerStrip, INDEX(iInStrip));

    if( iInStrip==0) {
#ifdef _GLES //karin: using glDrawElements instead of glArrayElement
		if(!is.empty())
		{
			pglDrawElements(GL_TRIANGLE_STRIP, is.size(), INDEX_GL, is.data());
			is.clear();
		}
#else
      pglEnd();
#endif
      SetCol();
#if !defined(_GLES) //karin: using glDrawElements instead of glArrayElement
      pglBegin( GL_TRIANGLE_STRIP);
#endif
#ifdef _GLES //karin: using glDrawElements instead of glArrayElement
	  is.push_back(i0);
	  is.push_back(i1);
	  is.push_back(i2);
#else
      pglArrayElement(i0);
      pglArrayElement(i1);
      pglArrayElement(i2);
#endif
      iL0 = i1;
      iL1 = i2;
      i++;
      iInStrip++;
    } else {
      if (iInStrip%2==0) {
        if (iL0==i0 && iL1==i1) {
#ifdef _GLES //karin: using glDrawElements instead of glArrayElement
			is.push_back(i2);
#else
          pglArrayElement(i2);
#endif
          iL0=iL1; iL1=i2;
          i++;
          iInStrip++;
        } else {
          iInStrip=0;
        }
      } else {
        if (iL0==i1 && iL1==i0) {
#ifdef _GLES //karin: using glDrawElements instead of glArrayElement
			is.push_back(i2);
#else
          pglArrayElement(i2);
#endif
          iL0=iL1; iL1=i2;
          i++;
          iInStrip++;
        } else {
          iInStrip=0;
        }
      }
    }
  }
#ifdef _GLES //karin: using glDrawElements instead of glArrayElement
  if(!is.empty())
  {
	  pglDrawElements(GL_TRIANGLE_STRIP, is.size(), INDEX_GL, is.data());
	  is.clear();
  }
#else
  pglEnd();
#endif
  OGL_CHECKERROR;
}


// returns haze/fog value in vertex 
static FLOAT3D _vFViewerObj, _vHDirObj;
static FLOAT   _fFogAddZ, _fFogAddH;
static FLOAT   _fHazeAdd;


// check vertex against fog
static void GetFogMapInVertex( GFXVertex3 &vtx, GFXTexCoord &tex)
{
#if (defined __MSVC_INLINE__)
  __asm {
    mov     esi,D [vtx]
    mov     edi,D [tex]
    fld     D [esi]GFXVertex3.x
    fmul    D [_vFViewerObj+0]
    fld     D [esi]GFXVertex3.y
    fmul    D [_vFViewerObj+4]
    fld     D [esi]GFXVertex3.z
    fmul    D [_vFViewerObj+8]
    fxch    st(2)
    faddp   st(1),st(0)
    faddp   st(1),st(0) // fD
    fld     D [esi]GFXVertex3.x
    fmul    D [_vHDirObj+0]
    fld     D [esi]GFXVertex3.y
    fmul    D [_vHDirObj+4]
    fld     D [esi]GFXVertex3.z
    fmul    D [_vHDirObj+8]
    fxch    st(2)
    faddp   st(1),st(0)
    faddp   st(1),st(0) // fH, fD
    fxch    st(1)
    fadd    D [_fFogAddZ]
    fmul    D [_fog_fMulZ]
    fxch    st(1)
    fadd    D [_fFogAddH]
    fmul    D [_fog_fMulH]
    fxch    st(1)
    fstp    D [edi+0]
    fstp    D [edi+4]
  }
#else
  const FLOAT fD = vtx.x*_vFViewerObj(1) + vtx.y*_vFViewerObj(2) + vtx.z*_vFViewerObj(3);
  const FLOAT fH = vtx.x*_vHDirObj(1)    + vtx.y*_vHDirObj(2)    + vtx.z*_vHDirObj(3);
  tex.st.s = (fD+_fFogAddZ) * _fog_fMulZ;
  tex.st.t = (fH+_fFogAddH) * _fog_fMulH;
#endif

}


// check vertex against haze
static void GetHazeMapInVertex( GFXVertex3 &vtx, FLOAT &tx1)
{
#if (defined __MSVC_INLINE__)
  __asm {
    mov     esi,D [vtx]
    mov     edi,D [tx1]
    fld     D [esi]GFXVertex3.x
    fmul    D [_vViewerObj+0]
    fld     D [esi]GFXVertex3.y
    fmul    D [_vViewerObj+4]
    fld     D [esi]GFXVertex3.z
    fmul    D [_vViewerObj+8]
    fxch    st(2)
    faddp   st(1),st(0)
    faddp   st(1),st(0)
    fadd    D [_fHazeAdd]
    fmul    D [_haze_fMul]
    fstp    D [edi]
  }
#else
  const FLOAT fD = vtx.x*_vViewerObj(1) + vtx.y*_vViewerObj(2) + vtx.z*_vViewerObj(3);
  tx1 = (fD+_fHazeAdd) * _haze_fMul;
#endif
}


// check model's bounding box against fog
static BOOL IsModelInFog( FLOAT3D &vMin, FLOAT3D &vMax)
{
  GFXTexCoord tex;
  GFXVertex3  vtx;
  vtx.x = vMin(1); vtx.y = vMin(2); vtx.z = vMin(3); GetFogMapInVertex(vtx, tex); if (InFog(tex.st.t)) return TRUE;
  vtx.x = vMin(1); vtx.y = vMin(2); vtx.z = vMax(3); GetFogMapInVertex(vtx, tex); if (InFog(tex.st.t)) return TRUE;
  vtx.x = vMin(1); vtx.y = vMax(2); vtx.z = vMin(3); GetFogMapInVertex(vtx, tex); if (InFog(tex.st.t)) return TRUE;
  vtx.x = vMin(1); vtx.y = vMax(2); vtx.z = vMax(3); GetFogMapInVertex(vtx, tex); if (InFog(tex.st.t)) return TRUE;
  vtx.x = vMax(1); vtx.y = vMin(2); vtx.z = vMin(3); GetFogMapInVertex(vtx, tex); if (InFog(tex.st.t)) return TRUE;
  vtx.x = vMax(1); vtx.y = vMin(2); vtx.z = vMax(3); GetFogMapInVertex(vtx, tex); if (InFog(tex.st.t)) return TRUE;
  vtx.x = vMax(1); vtx.y = vMax(2); vtx.z = vMin(3); GetFogMapInVertex(vtx, tex); if (InFog(tex.st.t)) return TRUE;
  vtx.x = vMax(1); vtx.y = vMax(2); vtx.z = vMax(3); GetFogMapInVertex(vtx, tex); if (InFog(tex.st.t)) return TRUE;
  return FALSE;
}

// check model's bounding box against haze
static BOOL IsModelInHaze( FLOAT3D &vMin, FLOAT3D &vMax)
{
  FLOAT fS;
  GFXVertex3 vtx;                                
  vtx.x=vMin(1); vtx.y=vMin(2); vtx.z=vMin(3); GetHazeMapInVertex(vtx,fS); if(InHaze(fS)) return TRUE;
  vtx.x=vMin(1); vtx.y=vMin(2); vtx.z=vMax(3); GetHazeMapInVertex(vtx,fS); if(InHaze(fS)) return TRUE;
  vtx.x=vMin(1); vtx.y=vMax(2); vtx.z=vMin(3); GetHazeMapInVertex(vtx,fS); if(InHaze(fS)) return TRUE;
  vtx.x=vMin(1); vtx.y=vMax(2); vtx.z=vMax(3); GetHazeMapInVertex(vtx,fS); if(InHaze(fS)) return TRUE;
  vtx.x=vMax(1); vtx.y=vMin(2); vtx.z=vMin(3); GetHazeMapInVertex(vtx,fS); if(InHaze(fS)) return TRUE;
  vtx.x=vMax(1); vtx.y=vMin(2); vtx.z=vMax(3); GetHazeMapInVertex(vtx,fS); if(InHaze(fS)) return TRUE;
  vtx.x=vMax(1); vtx.y=vMax(2); vtx.z=vMin(3); GetHazeMapInVertex(vtx,fS); if(InHaze(fS)) return TRUE;
  vtx.x=vMax(1); vtx.y=vMax(2); vtx.z=vMax(3); GetHazeMapInVertex(vtx,fS); if(InHaze(fS)) return TRUE;
  return FALSE;
}



// render all pending elements
static void FlushElements( INDEX ctElem, INDEX_T *pai)
{
  ASSERT(ctElem>0);
  // choose rendering mode
  extern INDEX mdl_bShowStrips;
  if( _bMultiPlayer) mdl_bShowStrips = 0; // don't allow in multiplayer mode!
  if( !mdl_bShowStrips) {
    _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_DRAWELEMENTS);
    _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_DRAWELEMENTS, ctElem/3);
    _pGfx->gl_ctModelTriangles += ctElem/3;
    gfxDrawElements( ctElem, pai);
    extern INDEX mdl_bShowTriangles;
    if( _bMultiPlayer) mdl_bShowTriangles = 0; // don't allow in multiplayer mode!
    if( mdl_bShowTriangles) {
      gfxSetConstantColor(C_YELLOW|222); // this also disables color array
      gfxPolygonMode(GFX_LINE);
      gfxDrawElements( ctElem, pai);
      gfxPolygonMode(GFX_FILL);
      gfxEnableColorArray(); // need to re-enable color array
    } // done
    _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_DRAWELEMENTS);
  }
  // show strips
  else if( _eAPI==GAT_OGL) {
    DrawStrips( ctElem, pai);
    OGL_CHECKERROR;
  }
}


// returns if any type of translucent surface was required
static void SetRenderingParameters( SurfaceTranslucencyType stt, BOOL bHasBump)
{
  _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_ONESIDE_GLSETUP);
  _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_ONESIDE_GLSETUP);

  if( stt==STT_TRANSLUCENT || (_bForceTranslucency && ((stt==STT_OPAQUE) || (stt==STT_TRANSPARENT)))) {
    gfxEnableBlend();
    gfxBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
    gfxDisableAlphaTest();
    gfxDisableDepthWrite();
  } else if( stt==STT_OPAQUE) {
    gfxDisableAlphaTest();
    /*
    gfxDisableBlend();
    gfxEnableDepthWrite();
    */
    if( bHasBump) {
      gfxEnableBlend();
      gfxBlendFunc( GFX_DST_COLOR, GFX_SRC_COLOR);
      gfxDisableDepthWrite();
    } else {
      gfxDisableBlend();
      gfxEnableDepthWrite();
    }
  } else if( stt==STT_TRANSPARENT) {
    gfxDisableBlend();
    gfxEnableAlphaTest();
    gfxEnableDepthWrite();
  } else if( stt==STT_ADD) {
    gfxEnableBlend();
    gfxBlendFunc( GFX_SRC_ALPHA, GFX_ONE);
    gfxDisableAlphaTest();
    gfxDisableDepthWrite();
  } else if( stt==STT_MULTIPLY) {
    gfxEnableBlend();
    gfxBlendFunc( GFX_ZERO, GFX_INV_SRC_COLOR);
    gfxDisableAlphaTest();
    gfxDisableDepthWrite();
  } else {
    ASSERTALWAYS( "Unsupported model rendering mode.");
  }
  // all done
  _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_ONESIDE_GLSETUP);
}


// render one side of a surface (return TRUE if any type of translucent surface has been rendered)
static void RenderOneSide( CRenderModel &rm, BOOL bBackSide, ULONG ulLayerFlags)
{
  _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_ONESIDE);
  _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_ONESIDE);
  _icol = 0;

  // set face culling
  if( bBackSide) {
    if( !(_ulMipLayerFlags&SRF_DOUBLESIDED)) {
      _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_ONESIDE);
      return;
    } else gfxCullFace(GFX_FRONT);
  } else gfxCullFace(GFX_BACK);

  // start with invalid rendering parameters
  SurfaceTranslucencyType sttLast = STT_INVALID;
  SLONG slBumpLast = -1;

  // for each surface in current mip model
  INDEX iStartElem=0;
  INDEX ctElements=0;
  ModelMipInfo &mmi = *rm.rm_pmmiMip;
  {FOREACHINSTATICARRAY( mmi.mmpi_MappingSurfaces, MappingSurface, itms)
  {
    const MappingSurface &ms = *itms;
    const ULONG ulFlags = ms.ms_ulRenderingFlags;
    // end rendering if surface is invisible or empty - these are the last surfaces in surface list
    if( (ulFlags&SRF_INVISIBLE) || ms.ms_ctSrfVx==0) break;
    // skip surface if ... 
    if( !(ulFlags&ulLayerFlags)  // not in this layer,
     ||  (bBackSide && !(ulFlags&SRF_DOUBLESIDED)) // rendering back side and surface is not double sided,
     || !(_ulColorMask&ms.ms_ulOnColor)  // not on or off.
     ||  (_ulColorMask&ms.ms_ulOffColor)) {
      if( ctElements>0) FlushElements( ctElements, &mmi.mmpi_aiElements[iStartElem]);
      iStartElem+= ctElements+ms.ms_ctSrfEl;
      ctElements = 0;
      continue;
    }

    // if should set parameters
    if( ulLayerFlags&SRF_DIFFUSE) {
      // get rendering parameters
      SurfaceTranslucencyType stt = ms.ms_sttTranslucencyType;

      SLONG slBump = _bHasBump && (ms.ms_ulRenderingFlags&SRF_BUMP) && mdl_bRenderBump;  // && !_bFlatFill 

      // if surface uses rendering parameters different than last one
      if( sttLast!=stt  || slBumpLast!=slBump) {
        // set up new API states
        if( ctElements>0) FlushElements( ctElements, &mmi.mmpi_aiElements[iStartElem]);
        SetRenderingParameters(stt, slBump);
        sttLast = stt;
        slBumpLast = slBump;
        iStartElem += ctElements;
        ctElements = 0;
      }
    } // batch the surface polygons for rendering
    ctElements += ms.ms_ctSrfEl;
  }}
  // flush leftovers
  if( ctElements>0) FlushElements( ctElements, &mmi.mmpi_aiElements[iStartElem]);
  // all done
  _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_ONESIDE);
}



// render model thru colors
static void RenderColors( CRenderModel &rm)
{
  // only if required
  if( rm.rm_rtRenderType&RT_NO_POLYGON_FILL) return;
  _icol = 0;

  // parameters
  gfxCullFace(GFX_BACK);
  gfxDisableBlend();
  gfxDisableAlphaTest();
  gfxDisableTexture();
  gfxEnableDepthWrite();

  gfxSetVertexArray( &_avtxSrfBase[0], _avtxSrfBase.Count());
  gfxSetColorArray(  &_acolSrfBase[0]);

  // for each surface in current mip model
  INDEX iStartElem=0;
  INDEX ctElements=0;
  ModelMipInfo &mmi = *rm.rm_pmmiMip;
  FOREACHINSTATICARRAY( mmi.mmpi_MappingSurfaces, MappingSurface, itms)
  {
    const MappingSurface &ms = *itms;
    // skip if surface is invisible or empty
    if( (ms.ms_ulRenderingFlags&SRF_INVISIBLE) || ms.ms_ctSrfVx==0
    || !(_ulColorMask&ms.ms_ulOnColor) || (_ulColorMask&ms.ms_ulOffColor)) {
      if( ctElements>0) FlushElements( ctElements, &mmi.mmpi_aiElements[iStartElem]);
      iStartElem+= ctElements+ms.ms_ctSrfEl;
      ctElements = 0;
      continue;
    }
    // set surface color
    COLOR srfCol; 
    extern INDEX GetBit( ULONG ulSource);
    if( rm.rm_rtRenderType&RT_ON_COLORS) {
      srfCol = PaletteColorValues[GetBit(ms.ms_ulOnColor)]|CT_OPAQUE;
    } else if( rm.rm_rtRenderType&RT_OFF_COLORS) {
      srfCol = PaletteColorValues[GetBit(ms.ms_ulOffColor)]|CT_OPAQUE;
    } else {
      srfCol = ms.ms_colColor|CT_OPAQUE;
    }
    // batch the surface polygons for rendering
    GFXColor glcol(srfCol);
    pcolSrfBase = &_acolSrfBase[ms.ms_iSrfVx0];
    for( INDEX iSrfVx=0; iSrfVx<ms.ms_ctSrfVx; iSrfVx++) pcolSrfBase[iSrfVx] = glcol;
    ctElements += ms.ms_ctSrfEl;
  }
  // all done
  if( ctElements>0) FlushElements( ctElements, &mmi.mmpi_aiElements[iStartElem]);
}


// render model as wireframe
static void RenderWireframe(CRenderModel &rm)
{
  // only if required
  if( !(rm.rm_rtRenderType&RT_WIRE_ON) && !(rm.rm_rtRenderType&RT_HIDDEN_LINES)) return;
  _icol = 0;

  // parameters
  gfxPolygonMode(GFX_LINE);
  gfxDisableBlend();
  gfxDisableAlphaTest();
  gfxDisableTexture();
  gfxDisableDepthTest();

  gfxSetVertexArray( &_avtxSrfBase[0], _avtxSrfBase.Count());
  gfxSetColorArray(  &_acolSrfBase[0]);

  COLOR colWire = _mrpModelRenderPrefs.GetInkColor()|CT_OPAQUE;
  ModelMipInfo &mmi = *rm.rm_pmmiMip;

  // first, render hidden lines (if required)
  if( rm.rm_rtRenderType&RT_HIDDEN_LINES)
  {
    gfxCullFace(GFX_FRONT);
    INDEX iStartElem=0;
    INDEX ctElements=0;
    // for each surface in current mip model
    FOREACHINSTATICARRAY( mmi.mmpi_MappingSurfaces, MappingSurface, itms) {
      const MappingSurface &ms = *itms;
      // skip if surface is invisible or empty
      if( (ms.ms_ulRenderingFlags&SRF_INVISIBLE) || ms.ms_ctSrfVx==0) {
        if( ctElements>0) FlushElements( ctElements, &mmi.mmpi_aiElements[iStartElem]);
        iStartElem+= ctElements+ms.ms_ctSrfEl;
        ctElements = 0;
        continue;
      }
      GFXColor glcol( colWire^0x80808080);
      pcolSrfBase = &_acolSrfBase[ms.ms_iSrfVx0];
      for( INDEX iSrfVx=0; iSrfVx<ms.ms_ctSrfVx; iSrfVx++) pcolSrfBase[iSrfVx] = glcol;
      ctElements += ms.ms_ctSrfEl;
    }
    // all done
    if( ctElements>0) FlushElements( ctElements, &mmi.mmpi_aiElements[iStartElem]);
    gfxCullFace(GFX_BACK);
  }
  // then, render visible lines (if required)
  if( rm.rm_rtRenderType&RT_WIRE_ON)
  {
    gfxCullFace(GFX_BACK);
    INDEX iStartElem=0;
    INDEX ctElements=0;
    // for each surface in current mip model
    FOREACHINSTATICARRAY( mmi.mmpi_MappingSurfaces, MappingSurface, itms) {
      const MappingSurface &ms = *itms;
      // done if surface is invisible or empty
      if( (ms.ms_ulRenderingFlags&SRF_INVISIBLE) || ms.ms_ctSrfVx==0) {
        if( ctElements>0) FlushElements( ctElements, &mmi.mmpi_aiElements[iStartElem]);
        iStartElem+= ctElements+ms.ms_ctSrfEl;
        ctElements = 0;
        continue;
      }
      GFXColor glcol(colWire);
      pcolSrfBase = &_acolSrfBase[ms.ms_iSrfVx0];
      for( INDEX iSrfVx=0; iSrfVx<ms.ms_ctSrfVx; iSrfVx++) pcolSrfBase[iSrfVx] = glcol;
      ctElements += ms.ms_ctSrfEl;
    }
    // all done
    if( ctElements>0) FlushElements( ctElements, &mmi.mmpi_aiElements[iStartElem]);
  }
  // all done
  gfxPolygonMode(GFX_FILL);
}




// attenuate alphas in base surface array with attenuation array
static void AttenuateAlpha( const UBYTE *pshdMip, const INDEX ctVertices)
{
  _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_ATTENUATE_SURF);
  _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_ATTENUATE_SURF, _ctAllSrfVx);
  for( INDEX iSrfVx=0; iSrfVx<ctVertices; iSrfVx++) {
    const INDEX iMipVx = puwSrfToMip[iSrfVx];
    pcolSrfBase[iSrfVx].AttenuateA( pshdMip[iMipVx]);
  }
  _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_ATTENUATE_SURF);
}


// attenuate colors in base surface array with attenuation array
static void AttenuateColor( const UBYTE *pshdMip, const INDEX ctVertices)
{
  _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_ATTENUATE_SURF);
  _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_ATTENUATE_SURF, _ctAllSrfVx);
  for( INDEX iSrfVx=0; iSrfVx<ctVertices; iSrfVx++) {
    const INDEX iMipVx = puwSrfToMip[iSrfVx];
    pcolSrfBase[iSrfVx].AttenuateRGB( pshdMip[iMipVx]);
  }
  _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_ATTENUATE_SURF);
}


// unpack vertices (and eventually normals) of one frame
static void UnpackFrame( CRenderModel &rm, BOOL bKeepNormals)
{
  _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_INIT_UNPACK);
  _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_INIT_UNPACK, _ctAllMipVx);

  // cache lerp ratio, compression, stretch and light factors
  FLOAT fStretchX = rm.rm_vStretch(1);
  FLOAT fStretchY = rm.rm_vStretch(2);
  FLOAT fStretchZ = rm.rm_vStretch(3);
  FLOAT fOffsetX  = rm.rm_vOffset(1);
  FLOAT fOffsetY  = rm.rm_vOffset(2);
  FLOAT fOffsetZ  = rm.rm_vOffset(3);
  const FLOAT fLerpRatio = rm.rm_fRatio;
  const FLOAT fLightObjX = rm.rm_vLightObj(1) * -255.0f;  // multiplier is made here, so it doesn't need to be done per-vertex
  const FLOAT fLightObjY = rm.rm_vLightObj(2) * -255.0f;
  const FLOAT fLightObjZ = rm.rm_vLightObj(3) * -255.0f;
  const UWORD *puwMipToMdl = (const UWORD*)&rm.rm_pmmiMip->mmpi_auwMipToMdl[0];
        SWORD *pswMipCol   = (SWORD*)&pcolMipBase[_ctAllMipVx>>1];

  // if 16 bit compression
  if( rm.rm_pmdModelData->md_Flags & MF_COMPRESSED_16BIT)
  {
    // if no lerping
    const ModelFrameVertex16 *pFrame0 = rm.rm_pFrame16_0;
    const ModelFrameVertex16 *pFrame1 = rm.rm_pFrame16_1;
    if( pFrame0==pFrame1)
    {
#if (defined __MSVC_INLINE__)
      // for each vertex in mip
      const SLONG fixLerpRatio = FloatToInt(fLerpRatio*256.0f); // fix 8:8
      SLONG slTmp1, slTmp2, slTmp3;
      __asm {
        mov     edi,D [pvtxMipBase]
        mov     ebx,D [pswMipCol]
        xor     ecx,ecx
vtxLoop16:
        push    ecx
        mov     esi,D [puwMipToMdl]
        movzx   eax,W [esi+ecx*2]
        mov     esi,D [pFrame0]
        lea     esi,[esi+eax*8]
        // store vertex
        movsx   eax,W [esi]ModelFrameVertex16.mfv_SWPoint[0]
        movsx   ecx,W [esi]ModelFrameVertex16.mfv_SWPoint[2]
        movsx   edx,W [esi]ModelFrameVertex16.mfv_SWPoint[4]
        mov     D [slTmp1],eax
        mov     D [slTmp2],ecx
        mov     D [slTmp3],edx
        fild    D [slTmp1]
        fsub    D [fOffsetX]
        fmul    D [fStretchX]
        fild    D [slTmp2]
        fsub    D [fOffsetY]
        fmul    D [fStretchY]
        fild    D [slTmp3]
        fsub    D [fOffsetZ]
        fmul    D [fStretchZ]
        fxch    st(2)
        fstp    D [edi]GFXVertex3.x
        fstp    D [edi]GFXVertex3.y
        fstp    D [edi]GFXVertex3.z
        // determine normal
        movzx   eax,B [esi]ModelFrameVertex16.mfv_ubNormH
        movzx   edx,B [esi]ModelFrameVertex16.mfv_ubNormP
        mov     esi,D [pfSinTable]
        fld     D [esi+eax*4 +0]
        fmul    D [esi+edx*4 +64*4]
        fld     D [esi+eax*4 +64*4]
        fmul    D [esi+edx*4 +64*4]
        fxch    st(1)
        fstp    D [slTmp1]
        fstp    D [slTmp3]
        mov     eax,D [slTmp1]
        mov     ecx,D [slTmp3]
        xor     eax,0x80000000     
        xor     ecx,0x80000000     
        mov     D [slTmp1],eax
        mov     D [slTmp3],ecx
        // determine vertex shade
        fld     D [slTmp1]
        fmul    D [fLightObjX]
        fld     D [esi+edx*4 +0]
        fmul    D [fLightObjY]
        fld     D [slTmp3]
        fmul    D [fLightObjZ]
        fxch    st(2)
        faddp   st(1),st(0)
        faddp   st(1),st(0)
        fistp   D [ebx]
        // store normal (if needed)
        cmp     D [bKeepNormals],0
        je      vtxNext16
        mov     ecx,D [esp]
        imul    ecx,3*4
        add     ecx,D [pnorMipBase]
        mov     eax,D [slTmp1]
        mov     edx,D [esi+edx*4 +0]
        mov     esi,D [slTmp3]
        mov     D [ecx]GFXNormal.nx, eax
        mov     D [ecx]GFXNormal.ny, edx
        mov     D [ecx]GFXNormal.nz, esi
        // advance to next vertex
vtxNext16:
        pop     ecx
        add     edi,3*4
        add     ebx,1*2
        inc     ecx
        cmp     ecx,D [_ctAllMipVx]
        jl      vtxLoop16
      }
#else
      // for each vertex in mip
      for( INDEX iMipVx=0; iMipVx<_ctAllMipVx; iMipVx++) {
        // get destination for unpacking
        const INDEX iMdlVx = puwMipToMdl[iMipVx];
        const ModelFrameVertex16 &mfv0 = pFrame0[iMdlVx];
        // store vertex
        GFXVertex3 &vtx = pvtxMipBase[iMipVx];
        vtx.x = (mfv0.mfv_SWPoint(1) -fOffsetX) *fStretchX;
        vtx.y = (mfv0.mfv_SWPoint(2) -fOffsetY) *fStretchY;
        vtx.z = (mfv0.mfv_SWPoint(3) -fOffsetZ) *fStretchZ;
        // determine normal
        const FLOAT fSinH0 = pfSinTable[mfv0.mfv_ubNormH];
        const FLOAT fSinP0 = pfSinTable[mfv0.mfv_ubNormP];
        const FLOAT fCosH0 = pfCosTable[mfv0.mfv_ubNormH];
        const FLOAT fCosP0 = pfCosTable[mfv0.mfv_ubNormP];
        const FLOAT fNX = -fSinH0*fCosP0;
        const FLOAT fNY = +fSinP0;
        const FLOAT fNZ = -fCosH0*fCosP0;
        // store vertex shade
        pswMipCol[iMipVx] = FloatToInt(fNX*fLightObjX + fNY*fLightObjY + fNZ*fLightObjZ);
        // store normal (if needed)
        if( bKeepNormals) {
          pnorMipBase[iMipVx].nx = fNX;
          pnorMipBase[iMipVx].ny = fNY;
          pnorMipBase[iMipVx].nz = fNZ;
        }
      }
#endif
    }
    // if lerping
    else
    {
#if (defined __MSVC_INLINE__)
      // for each vertex in mip
      const SLONG fixLerpRatio = FloatToInt(fLerpRatio*256.0f); // fix 8:8
      SLONG slTmp1, slTmp2, slTmp3;
      __asm {
        mov     edi,D [pvtxMipBase]
        mov     ebx,D [pswMipCol]
        xor     ecx,ecx
vtxLoop16L:
        push    ecx
        push    ebx
        mov     esi,D [puwMipToMdl]
        movzx   ebx,W [esi+ecx*2]
        mov     esi,D [pFrame0]
        mov     ecx,D [pFrame1]
        // lerp vertex
        movsx   eax,W [esi+ebx*8]ModelFrameVertex16.mfv_SWPoint[0]
        movsx   edx,W [ecx+ebx*8]ModelFrameVertex16.mfv_SWPoint[0]
        sub     edx,eax
        imul    edx,D [fixLerpRatio]
        sar     edx,8
        add     eax,edx
        mov     D [slTmp1],eax
        movsx   eax,W [esi+ebx*8]ModelFrameVertex16.mfv_SWPoint[2]
        movsx   edx,W [ecx+ebx*8]ModelFrameVertex16.mfv_SWPoint[2]
        sub     edx,eax
        imul    edx,D [fixLerpRatio]
        sar     edx,8
        add     eax,edx
        mov     D [slTmp2],eax
        movsx   eax,W [esi+ebx*8]ModelFrameVertex16.mfv_SWPoint[4]
        movsx   edx,W [ecx+ebx*8]ModelFrameVertex16.mfv_SWPoint[4]
        sub     edx,eax
        imul    edx,D [fixLerpRatio]
        sar     edx,8
        add     eax,edx
        mov     D [slTmp3],eax
        // store vertex
        fild    D [slTmp1]
        fsub    D [fOffsetX]
        fmul    D [fStretchX]
        fild    D [slTmp2]
        fsub    D [fOffsetY]
        fmul    D [fStretchY]
        fild    D [slTmp3]
        fsub    D [fOffsetZ]
        fmul    D [fStretchZ]
        fxch    st(2)
        fstp    D [edi]GFXVertex3.x
        fstp    D [edi]GFXVertex3.y
        fstp    D [edi]GFXVertex3.z
        // load normals
        movzx   eax,B [esi+ebx*8]ModelFrameVertex16.mfv_ubNormH
        movzx   edx,B [esi+ebx*8]ModelFrameVertex16.mfv_ubNormP
        mov     esi,D [pfSinTable]
        fld     D [esi+eax*4 +0]
        fmul    D [esi+edx*4 +64*4]
        fld     D [esi+edx*4 +0]
        fld     D [esi+eax*4 +64*4]
        fmul    D [esi+edx*4 +64*4]  // fCosH0*fCosP0, fSinP0, fSinH0*fCosP0  
        movzx   eax,B [ecx+ebx*8]ModelFrameVertex16.mfv_ubNormH
        movzx   edx,B [ecx+ebx*8]ModelFrameVertex16.mfv_ubNormP
        fld     D [esi+eax*4 +0]
        fmul    D [esi+edx*4 +64*4]
        fld     D [esi+edx*4 +0]
        fld     D [esi+eax*4 +64*4]
        fmul    D [esi+edx*4 +64*4]  // fCosH1*fCosP1, fSinP1, fSinH1*fCosP1,  fCosH0*fCosP0, fSinP0, fSinH0*fCosP0
        // lerp normals
        fxch    st(5)        // SH0CP0,        SP1,     SH1CP1,        CH0CP0, SP0,    CH1CP1
        fsub    st(2),st(0)
        fxch    st(4)        // SP0,           SP1,     SH1CP1-SH0CP0, CH0CP0, SH0CP0, CH1CP1
        fsub    st(1),st(0)  // SP0,           SP1-SP0, SH1CP1-SH0CP0, CH0CP0, SH0CP0, CH1CP1
        fxch    st(3)        // CH0CP0,        SP1-SP0, SH1CP1-SH0CP0, SP0,    SH0CP0, CH1CP1
        fsub    st(5),st(0)  // CH0CP0,        SP1-SP0, SH1CP1-SH0CP0, SP0,    SH0CP0, CH1CP1-CH0CP0
        fxch    st(2)        // SH1CP1-SH0CP0, SP1-SP0, CH0CP0,        SP0,    SH0CP0, CH1CP1-CH0CP0
        fmul    D [fLerpRatio]
        fxch    st(1)        // SP1-SP0,       lSH1CP1, CH0CP0,        SP0,    SH0CP0, CH1CP1-CH0CP0
        fmul    D [fLerpRatio]
        fxch    st(5)        // CH1CP1-CH0CP0, lSH1CP1, CH0CP0,        SP0,    SH0CP0, lSP1SP0
        fmul    D [fLerpRatio]
        fxch    st(1)        // lSH1CP1, lCH1CP1, CH0CP0,  SP0, SH0CP0, lSP1SP0
        faddp   st(4),st(0)  // lCH1CP1, CH0CP0,  SP0,     fNX, lSP1SP0
        fxch    st(2)        // SP0,     CH0CP0,  lCH1CP1, fNX, lSP1SP0
        faddp   st(4),st(0)  // CH0CP0,  lCH1CP1, fNX,     fNY
        faddp   st(1),st(0)  // -fNZ, -fNX,  fNY
        fxch    st(2)        //  fNY, -fNX, -fNZ
        fstp    D [slTmp2]
        fstp    D [slTmp1]
        fstp    D [slTmp3]
        pop     ebx
        mov     eax,D [slTmp1]
        mov     ecx,D [slTmp3]
        xor     eax,0x80000000     
        xor     ecx,0x80000000     
        mov     D [slTmp1],eax
        mov     D [slTmp3],ecx
        // determine vertex shade
        fld     D [slTmp1]
        fmul    D [fLightObjX]
        fld     D [slTmp2]
        fmul    D [fLightObjY]
        fld     D [slTmp3]
        fmul    D [fLightObjZ]
        fxch    st(2)
        faddp   st(1),st(0)
        faddp   st(1),st(0)
        fistp   D [ebx]
        // store lerped normal (if needed)
        cmp     D [bKeepNormals],0
        je      vtxNext16L
        mov     ecx,D [esp]
        imul    ecx,3*4
        add     ecx,D [pnorMipBase]
        mov     eax,D [slTmp1]
        mov     edx,D [slTmp2]
        mov     esi,D [slTmp3]
        mov     D [ecx]GFXNormal.nx, eax
        mov     D [ecx]GFXNormal.ny, edx
        mov     D [ecx]GFXNormal.nz, esi
        // advance to next vertex
vtxNext16L:
        pop     ecx
        add     edi,3*4
        add     ebx,1*2
        inc     ecx
        cmp     ecx,D [_ctAllMipVx]
        jl      vtxLoop16L
      }
#else
      // for each vertex in mip
      for( INDEX iMipVx=0; iMipVx<_ctAllMipVx; iMipVx++) {
        // get destination for unpacking
        const INDEX iMdlVx = puwMipToMdl[iMipVx];
        const ModelFrameVertex16 &mfv0 = pFrame0[iMdlVx];
        const ModelFrameVertex16 &mfv1 = pFrame1[iMdlVx];
        // store lerped vertex
        GFXVertex3 &vtx = pvtxMipBase[iMipVx];
        vtx.x = (Lerp( (FLOAT)mfv0.mfv_SWPoint(1), (FLOAT)mfv1.mfv_SWPoint(1), fLerpRatio) -fOffsetX) * fStretchX;
        vtx.y = (Lerp( (FLOAT)mfv0.mfv_SWPoint(2), (FLOAT)mfv1.mfv_SWPoint(2), fLerpRatio) -fOffsetY) * fStretchY;
        vtx.z = (Lerp( (FLOAT)mfv0.mfv_SWPoint(3), (FLOAT)mfv1.mfv_SWPoint(3), fLerpRatio) -fOffsetZ) * fStretchZ;
        // determine lerped normal
        const FLOAT fSinH0 = pfSinTable[mfv0.mfv_ubNormH];  const FLOAT fSinH1 = pfSinTable[mfv1.mfv_ubNormH];
        const FLOAT fSinP0 = pfSinTable[mfv0.mfv_ubNormP];  const FLOAT fSinP1 = pfSinTable[mfv1.mfv_ubNormP];
        const FLOAT fCosH0 = pfCosTable[mfv0.mfv_ubNormH];  const FLOAT fCosH1 = pfCosTable[mfv1.mfv_ubNormH];
        const FLOAT fCosP0 = pfCosTable[mfv0.mfv_ubNormP];  const FLOAT fCosP1 = pfCosTable[mfv1.mfv_ubNormP];
        const FLOAT fNX = Lerp( -fSinH0*fCosP0, -fSinH1*fCosP1, fLerpRatio);
        const FLOAT fNY = Lerp( +fSinP0,        +fSinP1,        fLerpRatio);
        const FLOAT fNZ = Lerp( -fCosH0*fCosP0, -fCosH1*fCosP1, fLerpRatio);
        // store vertex shade
        pswMipCol[iMipVx] = FloatToInt(fNX*fLightObjX + fNY*fLightObjY + fNZ*fLightObjZ);
        // store lerped normal (if needed)
        if( bKeepNormals) {
          pnorMipBase[iMipVx].nx = fNX;
          pnorMipBase[iMipVx].ny = fNY;
          pnorMipBase[iMipVx].nz = fNZ;
        }
      }
#endif

    }
  }
  // if 8 bit compression
  else 
  {
    const ModelFrameVertex8 *pFrame0 = rm.rm_pFrame8_0;
    const ModelFrameVertex8 *pFrame1 = rm.rm_pFrame8_1;
    // if no lerping
    if( pFrame0==pFrame1)
    {
#if (defined __MSVC_INLINE__)
      // for each vertex in mip
      const SLONG fixLerpRatio = FloatToInt(fLerpRatio*256.0f); // fix 8:8
      SLONG slTmp1, slTmp2, slTmp3;
      __asm {
        mov     edi,D [pvtxMipBase]
        mov     ebx,D [pswMipCol]
        xor     ecx,ecx
vtxLoop8:
        push    ecx
        mov     esi,D [puwMipToMdl]
        movzx   eax,W [esi+ecx*2]
        mov     esi,D [pFrame0]
        lea     esi,[esi+eax*4]
        // store vertex
        movsx   eax,B [esi]ModelFrameVertex8.mfv_SBPoint[0]
        movsx   ecx,B [esi]ModelFrameVertex8.mfv_SBPoint[1]
        movsx   edx,B [esi]ModelFrameVertex8.mfv_SBPoint[2]
        mov     D [slTmp1],eax
        mov     D [slTmp2],ecx
        mov     D [slTmp3],edx
        fild    D [slTmp1]
        fsub    D [fOffsetX]
        fmul    D [fStretchX]
        fild    D [slTmp2]
        fsub    D [fOffsetY]
        fmul    D [fStretchY]
        fild    D [slTmp3]
        fsub    D [fOffsetZ]
        fmul    D [fStretchZ]
        fxch    st(2)
        fstp    D [edi]GFXVertex3.x
        fstp    D [edi]GFXVertex3.y
        fstp    D [edi]GFXVertex3.z
        // determine normal
        movzx   eax,B [esi]ModelFrameVertex8.mfv_NormIndex
        lea     esi,[eax*2+eax]
        // determine vertex shade
        fld     D [avGouraudNormals+ esi*4 +0]
        fmul    D [fLightObjX]
        fld     D [avGouraudNormals+ esi*4 +4]
        fmul    D [fLightObjY]
        fld     D [avGouraudNormals+ esi*4 +8]
        fmul    D [fLightObjZ]
        fxch    st(2)
        faddp   st(1),st(0)
        faddp   st(1),st(0)
        fistp   D [ebx]
        // store lerped normal (if needed)
        cmp     D [bKeepNormals],0
        je      vtxNext8
        mov     ecx,D [esp]
        imul    ecx,3*4
        add     ecx,D [pnorMipBase]
        mov     eax,D [avGouraudNormals+ esi*4 +0]
        mov     edx,D [avGouraudNormals+ esi*4 +4]
        mov     esi,D [avGouraudNormals+ esi*4 +8]
        mov     D [ecx]GFXNormal.nx, eax
        mov     D [ecx]GFXNormal.ny, edx
        mov     D [ecx]GFXNormal.nz, esi
        // advance to next vertex
vtxNext8:
        pop     ecx
        add     edi,3*4
        add     ebx,1*2
        inc     ecx
        cmp     ecx,D [_ctAllMipVx]
        jl      vtxLoop8
      }
#else
      // for each vertex in mip
      for( INDEX iMipVx=0; iMipVx<_ctAllMipVx; iMipVx++) {
        // get destination for unpacking
        const INDEX iMdlVx = puwMipToMdl[iMipVx];
        const ModelFrameVertex8 &mfv0 = pFrame0[iMdlVx];
        // store vertex
        GFXVertex3 &vtx = pvtxMipBase[iMipVx];
        vtx.x = (mfv0.mfv_SBPoint(1) -fOffsetX) * fStretchX;
        vtx.y = (mfv0.mfv_SBPoint(2) -fOffsetY) * fStretchY;
        vtx.z = (mfv0.mfv_SBPoint(3) -fOffsetZ) * fStretchZ;
        // determine normal
        const FLOAT3D &vNormal0 = avGouraudNormals[mfv0.mfv_NormIndex];
        const FLOAT fNX = vNormal0(1);
        const FLOAT fNY = vNormal0(2);
        const FLOAT fNZ = vNormal0(3);
        // store vertex shade
        pswMipCol[iMipVx] = FloatToInt(fNX*fLightObjX + fNY*fLightObjY + fNZ*fLightObjZ);
        // store lerped normal (if needed)
        if( bKeepNormals) {
          pnorMipBase[iMipVx].nx = fNX;
          pnorMipBase[iMipVx].ny = fNY;
          pnorMipBase[iMipVx].nz = fNZ;
        }
      }
#endif
    }
    // if lerping
    else
    {
#if (defined __MSVC_INLINE__)
      const SLONG fixLerpRatio = FloatToInt(fLerpRatio*256.0f); // fix 8:8
      SLONG slTmp1, slTmp2, slTmp3;
      // re-adjust stretching factors because of fixint lerping (divide by 256)
      fStretchX*=0.00390625f;  fOffsetX*=256.0f;  
      fStretchY*=0.00390625f;  fOffsetY*=256.0f;
      fStretchZ*=0.00390625f;  fOffsetZ*=256.0f;
      // for each vertex in mip
      __asm {
        mov     edi,D [pvtxMipBase]
        mov     ebx,D [pswMipCol]
        xor     ecx,ecx
vtxLoop8L:
        push    ecx
        push    ebx
        mov     esi,D [puwMipToMdl]
        movzx   ebx,W [esi+ecx*2]
        mov     esi,D [pFrame0]
        mov     ecx,D [pFrame1]
        // lerp vertex
        movsx   eax,B [esi+ebx*4]ModelFrameVertex8.mfv_SBPoint[0]
        movsx   edx,B [ecx+ebx*4]ModelFrameVertex8.mfv_SBPoint[0]
        sub     edx,eax
        imul    edx,D [fixLerpRatio]
        shl     eax,8
        add     eax,edx
        mov     D [slTmp1],eax
        movsx   eax,B [esi+ebx*4]ModelFrameVertex8.mfv_SBPoint[1]
        movsx   edx,B [ecx+ebx*4]ModelFrameVertex8.mfv_SBPoint[1]
        sub     edx,eax
        imul    edx,D [fixLerpRatio]
        shl     eax,8
        add     eax,edx
        mov     D [slTmp2],eax
        movsx   eax,B [esi+ebx*4]ModelFrameVertex8.mfv_SBPoint[2]
        movsx   edx,B [ecx+ebx*4]ModelFrameVertex8.mfv_SBPoint[2]
        sub     edx,eax
        imul    edx,D [fixLerpRatio]
        shl     eax,8
        add     eax,edx
        mov     D [slTmp3],eax
        // store vertex
        fild    D [slTmp1]
        fsub    D [fOffsetX]
        fmul    D [fStretchX]
        fild    D [slTmp2]
        fsub    D [fOffsetY]
        fmul    D [fStretchY]
        fild    D [slTmp3]
        fsub    D [fOffsetZ]
        fmul    D [fStretchZ]
        fxch    st(2)
        fstp    D [edi]GFXVertex3.x
        fstp    D [edi]GFXVertex3.y
        fstp    D [edi]GFXVertex3.z
        // load normals
        movzx   eax,B [esi+ebx*4]ModelFrameVertex8.mfv_NormIndex
        movzx   edx,B [ecx+ebx*4]ModelFrameVertex8.mfv_NormIndex
        lea     esi,[eax*2+eax]
        lea     ecx,[edx*2+edx]
        // lerp normals
        fld     D [avGouraudNormals+ ecx*4 +0]
        fsub    D [avGouraudNormals+ esi*4 +0]
        fld     D [avGouraudNormals+ ecx*4 +4]
        fsub    D [avGouraudNormals+ esi*4 +4]
        fld     D [avGouraudNormals+ ecx*4 +8]
        fsub    D [avGouraudNormals+ esi*4 +8]
        fxch    st(2)   // nx1-nx0, ny1-ny0, nz1-nz0
        fmul    D [fLerpRatio]
        fxch    st(1)   // ny1-ny0, lnx1, nz1-nz0
        fmul    D [fLerpRatio]
        fxch    st(2)   // nz1-nz0, lnx1, lny1
        fmul    D [fLerpRatio]
        fxch    st(1)   // lnx1, lnz1, lny1
        fadd    D [avGouraudNormals+ esi*4 +0]
        fxch    st(2)   // lny1, lnz1, fNX
        fadd    D [avGouraudNormals+ esi*4 +4]
        fxch    st(1)   // lnz1, fNY, fNX
        fadd    D [avGouraudNormals+ esi*4 +8]
        fxch    st(2)   // fNX, fNY, fNZ
        // determine vertex shade
        fld     D [fLightObjX]
        fmul    st(0),st(1)     // flnx, fNX, fNY, fNZ 
        pop     ebx            
        fld     D [fLightObjY]
        fmul    st(0),st(3)     // flny, flnx, fNX, fNY, fNZ 
        fld     D [fLightObjZ]
        fmul    st(0),st(5)     // flnz, flny, flnx, fNX, fNY, fNXZ
        fxch    st(2)
        faddp   st(1),st(0)
        faddp   st(1),st(0)     // FL, fNX, fNY, fNXZ
        fistp   D [ebx]
        // store lerped normal (if needed)
        cmp     D [bKeepNormals],0
        je      vtxNext8L
        mov     ecx,D [esp]
        imul    ecx,3*4
        add     ecx,D [pnorMipBase]
        fstp    D [ecx]GFXNormal.nx
        fstp    D [ecx]GFXNormal.ny
        fst     D [ecx]GFXNormal.nz
        fld     st(0)
        fld     st(0)
        // advance to next vertex
vtxNext8L:
        fstp    st(0)
        fcompp
        pop     ecx
        add     edi,3*4
        add     ebx,1*2
        inc     ecx
        cmp     ecx,D [_ctAllMipVx]
        jl      vtxLoop8L
      }
#else
      // for each vertex in mip
      for( INDEX iMipVx=0; iMipVx<_ctAllMipVx; iMipVx++) {
        // get destination for unpacking
        const INDEX iMdlVx = puwMipToMdl[iMipVx];
        const ModelFrameVertex8 &mfv0 = pFrame0[iMdlVx];
        const ModelFrameVertex8 &mfv1 = pFrame1[iMdlVx];
        // store lerped vertex
        GFXVertex3 &vtx = pvtxMipBase[iMipVx];
        vtx.x = (Lerp( (FLOAT)mfv0.mfv_SBPoint(1), (FLOAT)mfv1.mfv_SBPoint(1), fLerpRatio) -fOffsetX) * fStretchX;
        vtx.y = (Lerp( (FLOAT)mfv0.mfv_SBPoint(2), (FLOAT)mfv1.mfv_SBPoint(2), fLerpRatio) -fOffsetY) * fStretchY;
        vtx.z = (Lerp( (FLOAT)mfv0.mfv_SBPoint(3), (FLOAT)mfv1.mfv_SBPoint(3), fLerpRatio) -fOffsetZ) * fStretchZ;
        // determine lerped normal
        const FLOAT3D &vNormal0 = avGouraudNormals[mfv0.mfv_NormIndex];
        const FLOAT3D &vNormal1 = avGouraudNormals[mfv1.mfv_NormIndex];
        const FLOAT fNX = Lerp( (FLOAT)vNormal0(1), (FLOAT)vNormal1(1), fLerpRatio);
        const FLOAT fNY = Lerp( (FLOAT)vNormal0(2), (FLOAT)vNormal1(2), fLerpRatio);
        const FLOAT fNZ = Lerp( (FLOAT)vNormal0(3), (FLOAT)vNormal1(3), fLerpRatio);
        // store vertex shade
        pswMipCol[iMipVx] = FloatToInt(fNX*fLightObjX + fNY*fLightObjY + fNZ*fLightObjZ);
        // store lerped normal (if needed)
        if( bKeepNormals) {
          pnorMipBase[iMipVx].nx = fNX;
          pnorMipBase[iMipVx].ny = fNY;
          pnorMipBase[iMipVx].nz = fNZ;
        }
      }
#endif
    }
  }

  // generate colors from shades
#if (defined __MSVC_INLINE__)
  __asm {
    pxor    mm0,mm0
    // construct 64-bit RGBA light
    mov     eax,D [_slLR]
    mov     ebx,D [_slLG]
    mov     ecx,D [_slLB]
    shl     ebx,16
    or      eax,ebx
    or      ecx,0x01FE0000
    movd    mm5,eax
    movd    mm7,ecx
    psllq   mm7,32
    por     mm5,mm7
    psllw   mm5,1 // boost for multiply
    // construct 64-bit RGBA ambient
    mov     eax,D [_slAR]
    mov     ebx,D [_slAG]
    mov     ecx,D [_slAB]
    shl     ebx,16
    or      eax,ebx
    movd    mm6,eax
    movd    mm7,ecx
    psllq   mm7,32
    por     mm6,mm7
    // init
    mov     esi,D [pswMipCol]
    mov     edi,D [pcolMipBase]
    mov     ecx,D [_ctAllMipVx]
    shr     ecx,2
    jz      colRest
    // 4-colors loop
colLoop4:
    movq    mm1,Q [esi]
    packuswb mm1,mm0
    punpcklbw mm1,mm1
    psrlw   mm1,1
    movq    mm3,mm1
    punpcklwd mm1,mm1
    punpckhwd mm3,mm3
    movq    mm2,mm1
    movq    mm4,mm3
    punpckldq mm1,mm1
    punpckhdq mm2,mm2
    punpckldq mm3,mm3
    punpckhdq mm4,mm4
    pmulhw  mm1,mm5
    pmulhw  mm2,mm5
    pmulhw  mm3,mm5
    pmulhw  mm4,mm5
    paddsw  mm1,mm6
    paddsw  mm2,mm6
    paddsw  mm3,mm6
    paddsw  mm4,mm6
    packuswb mm1,mm2
    packuswb mm3,mm4
    movq    Q [edi+0],mm1
    movq    Q [edi+8],mm3
    add     esi,2*4
    add     edi,4*4
    dec     ecx
    jnz     colLoop4
    // 1-color loop
colRest:
    mov     ecx,D [_ctAllMipVx]
    and     ecx,3
    jz      colEnd
colLoop1:
    movsx   eax,W [esi]
    movd    mm1,eax
    packuswb mm1,mm0
    punpcklbw mm1,mm1
    psrlw   mm1,1
    punpcklwd mm1,mm1
    punpckldq mm1,mm1
    pmulhw  mm1,mm5
    paddsw  mm1,mm6
    packuswb mm1,mm0
    movd    D [edi],mm1    
    add     esi,2
    add     edi,4
    dec     ecx
    jnz     colLoop1
colEnd:
    emms
  }
#else
    // generate colors from shades
    for( INDEX iMipVx=0; iMipVx<_ctAllMipVx; iMipVx++) {
      GFXColor &col = pcolMipBase[iMipVx];
      const SLONG slShade = Clamp( (SLONG)pswMipCol[iMipVx], (SLONG)0, (SLONG)255);
      col.ub.r = pubClipByte[_slAR + ((_slLR*slShade)>>8)];
      col.ub.g = pubClipByte[_slAG + ((_slLG*slShade)>>8)];
      col.ub.b = pubClipByte[_slAB + ((_slLB*slShade)>>8)];
      col.ub.a = slShade;
    }
#endif

  // all done
  _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_INIT_UNPACK);
}




// BEGIN MODEL RENDERING *******************************************************************************


#pragma warning(disable: 4731)
void CModelObject::RenderModel_View( CRenderModel &rm)
{
  // cache API
  _eAPI = _pGfx->gl_eCurrentAPI;
  ASSERT( GfxValidApi(_eAPI) );

  if( _eAPI==GAT_NONE) return;  // must have API

  // adjust Truform usage
  extern INDEX mdl_bTruformWeapons;
  extern INDEX gap_bForceTruform;
  // if weapon models don't allow tessellation or no tessellation has been set at all
  if( ((rm.rm_ulFlags&RMF_WEAPON) && !mdl_bTruformWeapons) || _pGfx->gl_iTessellationLevel<1) {
    // just disable truform
    gfxDisableTruform();
  } else {
    // enable truform for everything?
    if( gap_bForceTruform) gfxEnableTruform();
    else {
      // enable truform only for truform-ready models!
      const INDEX iTesselationLevel = Min( rm.rm_iTesselationLevel, _pGfx->gl_iTessellationLevel);
      if( iTesselationLevel>0) {
        extern INDEX ogl_bTruformLinearNormals;
        gfxSetTruform( iTesselationLevel, ogl_bTruformLinearNormals);
        gfxEnableTruform();
      }
      else gfxDisableTruform();
    }
  }
  // setup drawing direction (in case of mirror)
  if( rm.rm_ulFlags & RMF_INVERTED) gfxFrontFace(GFX_CW);
  else gfxFrontFace(GFX_CCW);

  // declare pointers for general usage
  INDEX iSrfVx0, ctSrfVx;
  GFXTexCoord *ptexSrfBase;
  GFXTexCoord *ptexSrfBump;
  GFXVertex   *pvtxSrfBase;
  FLOAT2D     *pvTexCoord;
        ModelMipInfo &mmi = *rm.rm_pmmiMip;
  const ModelMipInfo &mmi0 = rm.rm_pmdModelData->md_MipInfos[0];

  // calculate projection of viewer in absolute space
  FLOATmatrix3D &mViewer = _aprProjection->pr_ViewerRotationMatrix;
  _vViewer(1) = -mViewer(3,1);
  _vViewer(2) = -mViewer(3,2);
  _vViewer(3) = -mViewer(3,3);
  // calculate projection of viewer in object space
  _vViewerObj = _vViewer * !rm.rm_mObjectRotation;

  _pfModelProfile.IncrementCounter( CModelProfile::PCI_VERTICES_FIRSTMIP,        mmi0.mmpi_ctMipVx);
  _pfModelProfile.IncrementCounter( CModelProfile::PCI_SURFACEVERTICES_FIRSTMIP, mmi0.mmpi_ctSrfVx);
  _pfModelProfile.IncrementCounter( CModelProfile::PCI_TRIANGLES_FIRSTMIP,       mmi0.mmpi_ctTriangles);
  _pfModelProfile.IncrementCounter( CModelProfile::PCI_VERTICES_USEDMIP,         mmi.mmpi_ctMipVx);
  _pfModelProfile.IncrementCounter( CModelProfile::PCI_SURFACEVERTICES_USEDMIP,  mmi.mmpi_ctSrfVx);
  _pfModelProfile.IncrementCounter( CModelProfile::PCI_TRIANGLES_USEDMIP,        mmi.mmpi_ctTriangles);
  _sfStats.IncrementCounter( CStatForm::SCI_TRIANGLES_FIRSTMIP, mmi0.mmpi_ctTriangles);
  _sfStats.IncrementCounter( CStatForm::SCI_TRIANGLES_USEDMIP,  mmi.mmpi_ctTriangles); 

  // allocate vertex arrays
  _ctAllMipVx = mmi.mmpi_ctMipVx;
  _ctAllSrfVx = mmi.mmpi_ctSrfVx;
  ASSERT( _ctAllMipVx>0 && _ctAllSrfVx>0);
  ASSERT( _avtxMipBase.Count()==0);  _avtxMipBase.Push(_ctAllMipVx);
  ASSERT( _atexMipBase.Count()==0);  _atexMipBase.Push(_ctAllMipVx);
  ASSERT( _acolMipBase.Count()==0);  _acolMipBase.Push(_ctAllMipVx);
  ASSERT( _anorMipBase.Count()==0);  _anorMipBase.Push(_ctAllMipVx);

  ASSERT( _atexMipFogy.Count()==0);  _atexMipFogy.Push(_ctAllMipVx);
  ASSERT( _ashdMipFogy.Count()==0);  _ashdMipFogy.Push(_ctAllMipVx);
  ASSERT( _atx1MipHaze.Count()==0);  _atx1MipHaze.Push(_ctAllMipVx);
  ASSERT( _ashdMipHaze.Count()==0);  _ashdMipHaze.Push(_ctAllMipVx);

  ASSERT( _avtxSrfBase.Count()==0);  _avtxSrfBase.Push(_ctAllSrfVx);   
  ASSERT( _atexSrfBase.Count()==0);  _atexSrfBase.Push(_ctAllSrfVx);   
  ASSERT( _acolSrfBase.Count()==0);  _acolSrfBase.Push(_ctAllSrfVx);   

  if( GFX_bTruform) {
    ASSERT( _anorSrfBase.Count()==0);
    _anorSrfBase.Push(_ctAllSrfVx);   
  }

  // determine multitexturing capability for overbrighting purposes
  extern INDEX mdl_bAllowOverbright;
  const BOOL bOverbright = mdl_bAllowOverbright && _pGfx->gl_ctTextureUnits>1;
  
  // saturate light and ambient color
  const COLOR colL = AdjustColor( rm.rm_colLight,   _slShdHueShift, _slShdSaturation);
  const COLOR colA = AdjustColor( rm.rm_colAmbient, _slShdHueShift, _slShdSaturation);
  // cache light intensities (-1 in case of overbrighting compensation)
  const INDEX iBright = bOverbright ?  0 : 1;
  _slLR = (colL & CT_RMASK)>>(CT_RSHIFT-iBright);
  _slLG = (colL & CT_GMASK)>>(CT_GSHIFT-iBright);
  _slLB = (colL & CT_BMASK)>>(CT_BSHIFT-iBright);
  _slAR = (colA & CT_RMASK)>>(CT_RSHIFT-iBright);
  _slAG = (colA & CT_GMASK)>>(CT_GSHIFT-iBright);
  _slAB = (colA & CT_BMASK)>>(CT_BSHIFT-iBright);
  if( bOverbright) {
    _slAR = ClampUp( _slAR, (SLONG)127);
    _slAG = ClampUp( _slAG, (SLONG)127);
    _slAB = ClampUp( _slAB, (SLONG)127);
  }

  // set forced translucency and color mask
  _bForceTranslucency = ((rm.rm_colBlend&CT_AMASK)>>CT_ASHIFT) != CT_OPAQUE;
  _ulColorMask = mo_ColorMask;
  // adjust all surfaces' params for eventual forced-translucency case
  _ulMipLayerFlags = mmi.mmpi_ulLayerFlags;
  if( _bForceTranslucency) {
    _ulMipLayerFlags &= ~MMI_OPAQUE;
    _ulMipLayerFlags |=  MMI_TRANSLUCENT;
  }

  // unpack one model frame vertices and eventually normals (lerped or not lerped, as required)
  pvtxMipBase = &_avtxMipBase[0];
  pcolMipBase = &_acolMipBase[0];
  pnorMipBase = &_anorMipBase[0];
  const BOOL bNeedNormals = GFX_bTruform || (_ulMipLayerFlags&(SRF_REFLECTIONS|SRF_SPECULAR|SRF_BUMP));
  UnpackFrame( rm, bNeedNormals);

  // cache some more pointers and vars
  ptexMipBase = &_atexMipBase[0];
  ptexMipFogy = &_atexMipFogy[0];
  pshdMipFogy = &_ashdMipFogy[0];
  ptx1MipHaze = &_atx1MipHaze[0];
  pshdMipHaze = &_ashdMipHaze[0];


  // PREPARE FOG AND HAZE MIP --------------------------------------------------------------------------


  // if this model has haze
  if( rm.rm_ulFlags & RMF_HAZE)
  {
    _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_INIT_HAZE_MIP);
    _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_INIT_HAZE_MIP, _ctAllMipVx);
    // get viewer offset
    // _fHazeAdd = (_vViewer%(rm.rm_vObjectPosition-_aprProjection->pr_vViewerPosition)) - _haze_hp.hp_fNear; // might cause a BUG in compiler ????
    _fHazeAdd  = -_haze_hp.hp_fNear;
    _fHazeAdd += _vViewer(1) * (rm.rm_vObjectPosition(1) - _aprProjection->pr_vViewerPosition(1));
    _fHazeAdd += _vViewer(2) * (rm.rm_vObjectPosition(2) - _aprProjection->pr_vViewerPosition(2));
    _fHazeAdd += _vViewer(3) * (rm.rm_vObjectPosition(3) - _aprProjection->pr_vViewerPosition(3));

    // if it'll be cost-effective (i.e. model has enough vertices to be potentionaly trivialy rejected)
    // check bounding box of model against haze
    if( _ctAllMipVx>12 && !IsModelInHaze( rm.rm_vObjectMinBB, rm.rm_vObjectMaxBB)) {
      // this model has no haze after all
      rm.rm_ulFlags &= ~RMF_HAZE;
    } 
    // model is in haze (at least partially)
    else {
      // if model is all opaque
      if( _ulMipLayerFlags&MMI_OPAQUE) {
        // setup haze tex coords only
        for( INDEX iMipVx=0; iMipVx<_ctAllMipVx; iMipVx++) {
          GetHazeMapInVertex( pvtxMipBase[iMipVx], ptx1MipHaze[iMipVx]);
        }
      // if model is all translucent
      } else if( _ulMipLayerFlags&MMI_TRANSLUCENT) {
        // setup haze attenuation values only
        FLOAT tx1;
        for( INDEX iMipVx=0; iMipVx<_ctAllMipVx; iMipVx++) {
          GetHazeMapInVertex( pvtxMipBase[iMipVx], tx1);
          pshdMipHaze[iMipVx] = GetHazeAlpha(tx1) ^255;
        }
      // if model is partially opaque and partially translucent
      } else {
        // setup haze both tex coords and attenuation values
        for( INDEX iMipVx=0; iMipVx<_ctAllMipVx; iMipVx++) {
          FLOAT &tx1 = ptx1MipHaze[iMipVx];
          GetHazeMapInVertex( pvtxMipBase[iMipVx], tx1);
          pshdMipHaze[iMipVx] = GetHazeAlpha(tx1) ^255;
        }
      }
    } // haze mip setup done
    _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_INIT_HAZE_MIP);
  }

  // if this model has fog
  if( rm.rm_ulFlags & RMF_FOG)
  {
    _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_INIT_FOG_MIP);
    _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_INIT_FOG_MIP, _ctAllMipVx);
    // get viewer -z in object space
    _vFViewerObj = FLOAT3D(0,0,-1) * !rm.rm_mObjectToView;
    // get fog direction in object space
    _vHDirObj = _fog_vHDirAbs * !(!mViewer*rm.rm_mObjectToView);
    // get viewer offset
    // _fFogAddZ = _vViewer % (rm.rm_vObjectPosition - _aprProjection->pr_vViewerPosition);  // BUG in compiler !!!!
    _fFogAddZ  = _vViewer(1) * (rm.rm_vObjectPosition(1) - _aprProjection->pr_vViewerPosition(1));
    _fFogAddZ += _vViewer(2) * (rm.rm_vObjectPosition(2) - _aprProjection->pr_vViewerPosition(2));
    _fFogAddZ += _vViewer(3) * (rm.rm_vObjectPosition(3) - _aprProjection->pr_vViewerPosition(3));
    // get fog offset
    _fFogAddH = (_fog_vHDirAbs % rm.rm_vObjectPosition) + _fog_fp.fp_fH3;

    // if it'll be cost-effective (i.e. model has enough vertices to be potentionaly trivialy rejected)
    // check bounding box of model against fog
    if( _ctAllMipVx>16 && !IsModelInFog( rm.rm_vObjectMinBB, rm.rm_vObjectMaxBB)) {
      // this model has no fog after all
      rm.rm_ulFlags &= ~RMF_FOG;
    } 
    // model is in fog (at least partially)
    else { 
      // if model is all opaque
      if( _ulMipLayerFlags&MMI_OPAQUE) {
        // setup for tex coords only
        for( INDEX iMipVx=0; iMipVx<_ctAllMipVx; iMipVx++) {
          GetFogMapInVertex( pvtxMipBase[iMipVx], ptexMipFogy[iMipVx]);
        }
      // if model is all translucent
      } else if( _ulMipLayerFlags&MMI_TRANSLUCENT) {
        // setup fog attenuation values only
        GFXTexCoord tex;
        for( INDEX iMipVx=0; iMipVx<_ctAllMipVx; iMipVx++) {
          GetFogMapInVertex( pvtxMipBase[iMipVx], tex);
          pshdMipFogy[iMipVx] = GetFogAlpha(tex) ^255;
        }
      // if model is partially opaque and partially translucent
      } else {
        // setup fog both tex coords and attenuation values
        for( INDEX iMipVx=0; iMipVx<_ctAllMipVx; iMipVx++) {
          GFXTexCoord &tex = ptexMipFogy[iMipVx];
          GetFogMapInVertex( pvtxMipBase[iMipVx], tex);
          pshdMipFogy[iMipVx] = GetFogAlpha(tex) ^255;
        }
      }
    } // fog mip setup done
    _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_INIT_FOG_MIP);
  }

  // begin model rendering
  const BOOL bModelSetupTimer = _sfStats.CheckTimer(CStatForm::STI_MODELSETUP);
  if( bModelSetupTimer) _sfStats.StopTimer(CStatForm::STI_MODELSETUP);
  _sfStats.StartTimer(CStatForm::STI_MODELRENDERING);


  // PREPARE SURFACE VERTICES ------------------------------------------------------------------------
  

  _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_INIT_VERTICES);
  _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_INIT_VERTICES, _ctAllSrfVx);

  // for each surface in current mip model
  //BOOL bEmpty = TRUE; // ########### not used
  {FOREACHINSTATICARRAY( mmi.mmpi_MappingSurfaces, MappingSurface, itms)
  {
    const MappingSurface &ms = *itms;
    iSrfVx0 = ms.ms_iSrfVx0;
    ctSrfVx = ms.ms_ctSrfVx;
    // skip to next in case of invisible or empty surface
    if( (ms.ms_ulRenderingFlags&SRF_INVISIBLE) || ctSrfVx==0) break;
    //bEmpty = FALSE;
    puwSrfToMip = &mmi.mmpi_auwSrfToMip[iSrfVx0];
    pvtxSrfBase = &_avtxSrfBase[iSrfVx0];
    INDEX iSrfVx;

#if (defined __MSVC_INLINE__)
    __asm {
      push    ebx
      mov     ebx,D [puwSrfToMip]
      mov     esi,D [pvtxMipBase]
      mov     edi,D [pvtxSrfBase]
      mov     ecx,D [ctSrfVx]
srfVtxLoop:
      movzx   eax,W [ebx]
      lea     eax,[eax*2+eax]     // *3
      mov     edx,D [esi+eax*4+0] 
      movq    mm1,Q [esi+eax*4+4]
      mov     D [edi+0],edx
      movq    Q [edi+4],mm1
      add     ebx,2
      add     edi,4*4
      dec     ecx
      jnz     srfVtxLoop
      emms
      pop     ebx
    }
#else
    // setup vertex array
    for( iSrfVx=0; iSrfVx<ctSrfVx; iSrfVx++) {
      const INDEX iMipVx = puwSrfToMip[iSrfVx];
      pvtxSrfBase[iSrfVx].x = pvtxMipBase[iMipVx].x;
      pvtxSrfBase[iSrfVx].y = pvtxMipBase[iMipVx].y;
      pvtxSrfBase[iSrfVx].z = pvtxMipBase[iMipVx].z;
    }
#endif
    // setup normal array for truform (if enabled)
    if( GFX_bTruform) {
      GFXNormal *pnorSrfBase = &_anorSrfBase[iSrfVx0];
      for( iSrfVx=0; iSrfVx<ctSrfVx; iSrfVx++) {
        const INDEX iMipVx = puwSrfToMip[iSrfVx];
        pnorSrfBase[iSrfVx].nx = pnorMipBase[iMipVx].nx;
        pnorSrfBase[iSrfVx].ny = pnorMipBase[iMipVx].ny;
        pnorSrfBase[iSrfVx].nz = pnorMipBase[iMipVx].nz;
      }
    }
  }}
  // prepare (and lock) vertex array
  gfxEnableDepthTest();
  gfxSetVertexArray( &_avtxSrfBase[0], _ctAllSrfVx);
  if(GFX_bTruform) gfxSetNormalArray( &_anorSrfBase[0]);
  if(CVA_bModels) gfxLockArrays();
  // cache light in object space (for reflection and/or specular mapping)
  _vLightObj = rm.rm_vLightObj;
  // texture mapping correction factors (mex -> norm float)
  FLOAT fTexCorrU, fTexCorrV;
  gfxSetTextureWrapping( GFX_REPEAT, GFX_REPEAT);
  // color and fill mode setup
  _bFlatFill = (rm.rm_rtRenderType&RT_WHITE_TEXTURE) || mo_toTexture.GetData()==NULL;
  const BOOL bTexMode = rm.rm_rtRenderType & (RT_TEXTURE|RT_WHITE_TEXTURE);
  const BOOL bAllLayers = bTexMode && !_bFlatFill;  // disallow rendering of every layer except diffuse

  // model surface vertices prepared
  _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_INIT_VERTICES);


  // RENDER BUMP LAYER -----------------------------------------------------------------------------

  // if this model has bump mapping
  _bHasBump = FALSE;
  CTextureData *ptdBump = (CTextureData*)mo_toBump.GetData();
  if( (_ulMipLayerFlags&SRF_BUMP) && mdl_bRenderBump && ptdBump!=NULL && bAllLayers && _eAPI == GAT_OGL)
  {
    _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_RENDER_BUMP);
    _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_RENDER_BUMP, _ctAllSrfVx);
    // prepare array and pointers
     FLOAT3D *pvBmpCoordU, *pvBmpCoordV;
    ASSERT( _atexSrfBump.Count()==0);
    _atexSrfBump.Push(_ctAllSrfVx);     


    // get model bump color
    GFXColor colMdlBump;
    COLOR colB = AdjustColor( rm.rm_pmdModelData->md_colBump, _slTexHueShift, _slTexSaturation);
    colMdlBump.ul.abgr = (ByteSwap(colB)>>1) & 0x7F7F7F7F; // divide by 2 for bump equation (1-A+B)/2
    // alpha controls bump strength - premultiplied for per surface calculation *(1/128)*(1/16)*(1/128)
    const FLOAT fMdlBump = ((colB&0xFF)-128.0f) * 3.8144E-6; 
    // get bump texture corrections
    fTexCorrU = 1.0f / ptdBump->GetWidth(); 
    fTexCorrV = 1.0f / ptdBump->GetHeight();

    // for each bump surface in current mip model
    FOREACHINSTATICARRAY( mmi.mmpi_MappingSurfaces, MappingSurface, itms)
    {
      const MappingSurface &ms = *itms;
      iSrfVx0 = ms.ms_iSrfVx0;
      ctSrfVx = ms.ms_ctSrfVx;
      if(  (ms.ms_ulRenderingFlags&SRF_INVISIBLE) || ctSrfVx==0) break;  // done if found invisible or empty surface
      if( !(ms.ms_ulRenderingFlags&SRF_BUMP)) continue;  // skip non-bump or empty surface
      // cache surface pointers
      pvTexCoord  = &mmi.mmpi_avmexTexCoord[iSrfVx0];
      pvBmpCoordU = &mmi.mmpi_avBumpU[iSrfVx0];
      pvBmpCoordV = &mmi.mmpi_avBumpV[iSrfVx0];
      puwSrfToMip = &mmi.mmpi_auwSrfToMip[iSrfVx0];
      ptexSrfBump = &_atexSrfBump[iSrfVx0];
      ptexSrfBase = &_atexSrfBase[iSrfVx0];
      pcolSrfBase = &_acolSrfBase[iSrfVx0];

      // get surface bump color and combine with model color
      GFXColor colSrfBump;
      colB = AdjustColor( ms.ms_colBump, _slTexHueShift, _slTexSaturation);
      colSrfBump.MultiplyRGB( colB, colMdlBump);
      const FLOAT fSrfBump = ((colB&0xFF)-128.0f)*fMdlBump; // alpha controls bump strength

      // for each vertex in the surface
      for( INDEX iSrfVx=0; iSrfVx<ctSrfVx; iSrfVx++) {
        const INDEX iMipVx = puwSrfToMip[iSrfVx];
        GFXNormal3 &nor = pnorMipBase[iMipVx];
        // reflect viewer vector around vertex normal in object space
        // and find delta between reflected viewer and light
        FLOAT3D vOffset;
        const FLOAT fNV = nor.nx*_vViewerObj(1) + nor.ny*_vViewerObj(2) + nor.nz*_vViewerObj(3);
        vOffset(1) = (_vViewerObj(1) - 2*nor.nx*fNV) +_vLightObj(1);
        vOffset(2) = (_vViewerObj(2) - 2*nor.ny*fNV) +_vLightObj(2);
        vOffset(3) = (_vViewerObj(3) - 2*nor.nz*fNV) +_vLightObj(3);
        // get light strenght along texture axes and adjust texture coordinates
        const FLOAT fDU   = fSrfBump* (pvBmpCoordU[iSrfVx] %vOffset);
        const FLOAT fDV   = fSrfBump* (pvBmpCoordV[iSrfVx] %vOffset);
        const FLOAT fTexU = pvTexCoord[iSrfVx](1) *fTexCorrU;
        const FLOAT fTexV = pvTexCoord[iSrfVx](2) *fTexCorrV;
        ptexSrfBump[iSrfVx].st.s = fTexU + fDU;
        ptexSrfBump[iSrfVx].st.t = fTexV - fDV;
        ptexSrfBase[iSrfVx].st.s = fTexU - fDU;
        ptexSrfBase[iSrfVx].st.t = fTexV + fDV;
        // set bump color
        pcolSrfBase[iSrfVx] = colSrfBump;
      }
    } 
    // bump prep done
    _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_INIT_BUMP_SURF);

    // NOTE: bump is emulated by doing (1-A+B)/2
    // A and B is bump texture that was offset towards and away from light
    _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_RENDER_BUMP);
    _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_RENDER_BUMP);

    // setup texture/color arrays and rendering mode for 1st pass
    SetCurrentTexture( ptdBump, mo_toBump.GetFrame());
    gfxSetTexCoordArray(&_atexSrfBump[0], FALSE);  // pglTexCoordPointer( 2, GL_FLOAT,      0, &_atexSrfBump[0]);
    gfxSetColorArray( &_acolSrfBase[0]);           // pglColorPointer( 4, GL_UNSIGNED_BYTE, 0, &_acolSrfBase[0]);
    gfxDisableAlphaTest();
    gfxDisableBlend();
    gfxEnableDepthWrite();  //pglDepthMask( GL_TRUE);

    // render 1st pass
    RenderOneSide( rm, TRUE,  SRF_BUMP);
    RenderOneSide( rm, FALSE, SRF_BUMP);

    // setup texture array and rendering mode for 2nd pass
    gfxSetTexCoordArray(&_atexSrfBase[0], FALSE); // pglTexCoordPointer( 2, GL_FLOAT, 0, &_atexSrfBase[0]);
    pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_BLEND);
    gfxEnableBlend();
    gfxBlendFunc(GFX_ONE, GFX_ONE); //pglBlendFunc( GL_ONE, GL_ONE);
    gfxDisableDepthWrite();  //pglDepthMask( GL_FALSE);

    // render 2nd pass
    RenderOneSide( rm, TRUE,  SRF_BUMP);
    RenderOneSide( rm, FALSE, SRF_BUMP);

    // restore texture modulation
    pglTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    // bump rendering done
    _bHasBump = TRUE;
    _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_RENDER_BUMP);
  }

  // RENDER DIFFUSE LAYER -------------------------------------------------------------------

  _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_INIT_DIFF_SURF);
  _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_INIT_DIFF_SURF, _ctAllSrfVx);

  // get diffuse texture corrections
  CTextureData *ptdDiff = (CTextureData*)mo_toTexture.GetData();
  if( ptdDiff!=NULL) {
    fTexCorrU = 1.0f / ptdDiff->GetWidth();
    fTexCorrV = 1.0f / ptdDiff->GetHeight();
  } else {
    fTexCorrU = 1.0f;
    fTexCorrV = 1.0f;
  }

  // get model diffuse color
  GFXColor colMdlDiff;
  const COLOR colD = AdjustColor( rm.rm_pmdModelData->md_colDiffuse, _slTexHueShift, _slTexSaturation);
  const COLOR colB = AdjustColor( rm.rm_colBlend,                    _slTexHueShift, _slTexSaturation);
  colMdlDiff.MultiplyRGBA( colD, colB);

  // for each surface in current mip model
  {FOREACHINSTATICARRAY( mmi.mmpi_MappingSurfaces, MappingSurface, itms)
  {
    const MappingSurface &ms = *itms;
    iSrfVx0 = ms.ms_iSrfVx0;
    ctSrfVx = ms.ms_ctSrfVx;
    if( (ms.ms_ulRenderingFlags&SRF_INVISIBLE) || ctSrfVx==0) break;  // done if found invisible or empty surface
    // cache surface pointers
    puwSrfToMip = &mmi.mmpi_auwSrfToMip[iSrfVx0];
    pvTexCoord  = &mmi.mmpi_avmexTexCoord[iSrfVx0];
    ptexSrfBase = &_atexSrfBase[iSrfVx0];
    pcolSrfBase = &_acolSrfBase[iSrfVx0];

    // get surface diffuse color and combine with model color
    GFXColor colSrfDiff;
    const COLOR colD = AdjustColor( ms.ms_colDiffuse, _slTexHueShift, _slTexSaturation);
    colSrfDiff.MultiplyRGBA( colD, colMdlDiff);

#if (defined __MSVC_INLINE__)
    // setup texcoord array
    __asm {
      push    ebx
      mov     esi,D [pvTexCoord]
      mov     edi,D [ptexSrfBase]
      mov     ecx,D [ctSrfVx]
      shr     ecx,1
      jz      vtxRest
vtxLoop:
      fld     D [esi+0]
      fmul    D [fTexCorrU]
      fld     D [esi+8]
      fmul    D [fTexCorrU]
      fld     D [esi+4]
      fmul    D [fTexCorrV]
      fld     D [esi+12]
      fmul    D [fTexCorrV]
      fxch    st(3)   // u1, v1, u2, v2
      fstp    D [edi+0]
      fstp    D [edi+4]
      fstp    D [edi+8]
      fstp    D [edi+12]
      add     esi,2*2*4
      add     edi,2*2*4
      dec     ecx
      jnz     vtxLoop
vtxRest:
      test    D [ctSrfVx],1
      jz      vtxEnd
      fld     D [esi+0]
      fmul    D [fTexCorrU]
      fld     D [esi+4]
      fmul    D [fTexCorrV]
      fxch    st(1)
      fstp    D [edi+0]
      fstp    D [edi+4]
vtxEnd:
      pop     ebx
    }
#elif defined(__ARM_NEON__) && !defined (PLATFORM_MACOSX)
    register float tc_u __asm__("s0") = fTexCorrU;
    register float tc_v __asm__("s1") = fTexCorrV;
    const void *tc_src = pvTexCoord->vector;
    GFXTexCoord *tc_dst = ptexSrfBase;
    int tc_cnt = ctSrfVx;
    __asm__ __volatile__ (
      "vmov     d1, d0\n"
      "0:\n"
      "subs     %[c], #2\n"
      "blt      1f\n"
      "vld1.32  {d2}, [%[src]]\n"
      "add      %[src], %[src_s]\n"
      "vld1.32  {d3}, [%[src]]\n"
      "add      %[src], %[src_s]\n"
      "vmul.f32 q1, q1, q0\n"
      "pld      [%[src], #64]\n"
      "vst1.32  {q1}, [%[dst]]!\n"
      "b        0b\n"
      "1:\n"
      "tst      %[c], #1\n"
      "beq      2f\n"
      "vld1.32  {d2}, [%[src]]\n"
      "vmul.f32 d2, d2, d0\n"
      "vst1.32  {d2}, [%[dst]]\n"
      "2:\n"
      : [c] "=&r"(tc_cnt), [src] "=&r"(tc_src), [dst] "=&r"(tc_dst)
      :     "[c]"(tc_cnt),     "[src]"(tc_src),     "[dst]"(tc_dst),
        "t"(tc_u), "t"(tc_v), [src_s] "I"(sizeof(pvTexCoord[0]))
      : "d1", "q1", "cc", "memory"
    );
#else
    // setup texcoord array
    for( INDEX iSrfVx=0; iSrfVx<ctSrfVx; iSrfVx++) {
      ptexSrfBase[iSrfVx].st.s = pvTexCoord[iSrfVx](1) *fTexCorrU;
      ptexSrfBase[iSrfVx].st.t = pvTexCoord[iSrfVx](2) *fTexCorrV;
    }
#endif

    // setup color array
    if( ms.ms_sstShadingType==SST_FULLBRIGHT) {
      // eventually adjust reflection color for overbrighting
      GFXColor colSrfDiffAdj = colSrfDiff;
      if( bOverbright) {
		  colSrfDiffAdj.ub.r >>= 1;
		  colSrfDiffAdj.ub.g >>= 1;
		  colSrfDiffAdj.ub.b >>= 1;
      } // just copy diffuse color
      for( INDEX iSrfVx=0; iSrfVx<ctSrfVx; iSrfVx++) pcolSrfBase[iSrfVx] = colSrfDiffAdj;
    }
    else {
#if (defined __MSVC_INLINE__)
      // setup color array
	  const COLOR colS = colSrfDiff.ul.abgr;
      __asm {
        push    ebx
        mov     ebx,D [puwSrfToMip]
        mov     esi,D [pcolMipBase]
        mov     edi,D [pcolSrfBase]
        pxor    mm0,mm0
        movd    mm4,D [colS]
        punpcklbw mm4,mm0
        psllw   mm4,7
        paddw   mm4,Q [mmRounder]
        xor     ecx,ecx
diffColLoop:
        movzx   eax,W [ebx+ecx*2]
        movd    mm1,D [esi+eax*4]
        punpcklbw mm1,mm0
        por     mm1,Q [mmF000]
        psllw   mm1,1
        pmulhw  mm1,mm4
        packuswb mm1,mm1
        movd    D [edi+ecx*4],mm1
        inc     ecx
        cmp     ecx,D [ctSrfVx]
        jl      diffColLoop
        emms
        pop     ebx
      }
#else
      // setup color array
      const COLOR colS = colSrfDiff.ul.abgr;
      // setup diffuse color array
      for( INDEX iSrfVx=0; iSrfVx<ctSrfVx; iSrfVx++) {
        const INDEX iMipVx = puwSrfToMip[iSrfVx];
        pcolSrfBase[iSrfVx].MultiplyRGBCopyA1( colSrfDiff, pcolMipBase[iMipVx]);
      }
#endif
    }
    // eventually attenuate color in case of fog or haze
    if( (ms.ms_ulRenderingFlags&SRF_OPAQUE) && !_bForceTranslucency) continue;
    // eventually do some haze and/or fog attenuation of alpha channel in surface
    if( rm.rm_ulFlags & RMF_HAZE) {
      if( ms.ms_sttTranslucencyType==STT_MULTIPLY) AttenuateColor( pshdMipHaze, ctSrfVx);
      else AttenuateAlpha( pshdMipHaze, ctSrfVx);
    }
    if( rm.rm_ulFlags & RMF_FOG) {
      if( ms.ms_sttTranslucencyType==STT_MULTIPLY) AttenuateColor( pshdMipFogy, ctSrfVx);
      else AttenuateAlpha( pshdMipFogy, ctSrfVx);
    }
  }}
  // done with diffuse surfaces setup
  _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_INIT_DIFF_SURF);

  // if no texture mode is active
  if( !bTexMode && _eAPI==GAT_OGL) { 
    gfxUnlockArrays();
    // just render colors
    RenderColors(rm);
    // and eventually wireframe
    RenderWireframe(rm);
    // done
    gfxDepthFunc( GFX_LESS_EQUAL);
    gfxCullFace(GFX_BACK);
    // reset to defaults
    ResetVertexArrays();
    // done
    _sfStats.StopTimer(CStatForm::STI_MODELRENDERING);
    if( bModelSetupTimer) _sfStats.StartTimer(CStatForm::STI_MODELSETUP);
    _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_RENDERMODEL);
    return;
  }

  // proceed with rendering
  _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_RENDER_DIFFUSE);
  _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_RENDER_DIFFUSE);

  // must render diffuse if there is no texture (white mode)
  if( (_ulMipLayerFlags&SRF_DIFFUSE) || ptdDiff==NULL)
  { 
    // prepare overbrighting if supported
    if( bOverbright) gfxSetTextureModulation(2);
    // set texture/color arrays 
    INDEX iFrame=0;
    if( ptdDiff!=NULL) iFrame = mo_toTexture.GetFrame();;
    SetCurrentTexture( ptdDiff, iFrame);
    gfxSetTexCoordArray( &_atexSrfBase[0], FALSE);
    gfxSetColorArray(    &_acolSrfBase[0]);
    // do rendering
    RenderOneSide( rm, TRUE,  SRF_DIFFUSE);
    RenderOneSide( rm, FALSE, SRF_DIFFUSE);
    // revert to normal brightness if overbrighting was on
    if( bOverbright) gfxSetTextureModulation(1);
  }

  // adjust z-buffer and blending functions
  if( _ulMipLayerFlags&MMI_OPAQUE) gfxDepthFunc( GFX_EQUAL);
  else gfxDepthFunc( GFX_LESS_EQUAL);
  gfxDisableDepthWrite();
  gfxDisableAlphaTest(); // disable alpha testing if enabled after some surface
  gfxEnableBlend();

  // done with diffuse
  _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_RENDER_DIFFUSE);


  // RENDER PATCHES -------------------------------------------------------------------

    
  // if patches are on and are enabled for this mip model
  // TODO !!!!
  // if( (mo_PatchMask!=0) && (mmi.mmpi_ulFlags&MM_PATCHES_VISIBLE)) RenderPatches_View(rm);



  // RENDER DETAIL LAYER -------------------------------------------------------------------


  // if this model has detail mapping
  extern INDEX mdl_bRenderDetail;
  const ULONG ulTransAlpha = (rm.rm_colBlend&CT_AMASK)>>CT_ASHIFT;
  if( (_ulMipLayerFlags&(SRF_DETAIL)) && mdl_bRenderDetail && ptdBump!=NULL && ulTransAlpha>192 && bAllLayers && !_bHasBump)
  {
    _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_INIT_BUMP_SURF);
    _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_INIT_BUMP_SURF, _ctAllSrfVx);

    // get model detail color
    GFXColor colMdlBump;
    const COLOR colB = AdjustColor( rm.rm_pmdModelData->md_colBump, _slTexHueShift, _slTexSaturation);
	colMdlBump.ul.abgr = ByteSwap(colB);
    // get detail texture corrections
    fTexCorrU = 1.0f / ptdBump->GetWidth(); 
    fTexCorrV = 1.0f / ptdBump->GetHeight();
    // adjust detail stretch if needed get model's stretch
    if( !(rm.rm_pmdModelData->md_Flags&MF_STRETCH_DETAIL)) {
      const FLOAT  fStretch = mo_Stretch.Length() * 0.57735f; // /Sqrt(3);
      fTexCorrU *= fStretch; 
      fTexCorrV *= fStretch;
    }

    // for each bump surface in current mip model
    FOREACHINSTATICARRAY( mmi.mmpi_MappingSurfaces, MappingSurface, itms)
    {
      const MappingSurface &ms = *itms;
      iSrfVx0 = ms.ms_iSrfVx0;
      ctSrfVx = ms.ms_ctSrfVx;
      if(  (ms.ms_ulRenderingFlags&SRF_INVISIBLE) || ctSrfVx==0) break;  // done if found invisible or empty surface
      if( !(ms.ms_ulRenderingFlags&SRF_DETAIL)) continue;  // skip non-detail surface
      // cache surface pointers
      pvTexCoord  = &mmi.mmpi_avmexTexCoord[iSrfVx0];
      puwSrfToMip = &mmi.mmpi_auwSrfToMip[iSrfVx0];
      ptexSrfBase = &_atexSrfBase[iSrfVx0];
      pcolSrfBase = &_acolSrfBase[iSrfVx0];
      // get surface detail color and combine with model color
      GFXColor colSrfBump;
      const COLOR colB = AdjustColor( ms.ms_colBump, _slTexHueShift, _slTexSaturation);
      colSrfBump.MultiplyRGB( colB, colMdlBump);

      // for each vertex in the surface
      for( INDEX iSrfVx=0; iSrfVx<ctSrfVx; iSrfVx++) {
        // set detail texcoord and color
        INDEX iMipVx = mmi.mmpi_auwSrfToMip[iSrfVx];
		ptexSrfBase[iSrfVx].st.s = pvTexCoord[iSrfVx](1) * fTexCorrU;
		ptexSrfBase[iSrfVx].st.t = pvTexCoord[iSrfVx](2) * fTexCorrV;
        pcolSrfBase[iSrfVx]   = colSrfBump;
      }
    }
    // detail prep done
    _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_INIT_BUMP_SURF);

    // begin rendering
    _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_RENDER_BUMP);
    _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_RENDER_BUMP);

    // setup texture/color arrays and rendering mode
    SetCurrentTexture( ptdBump, mo_toBump.GetFrame());
    gfxSetTexCoordArray( &_atexSrfBase[0], FALSE);
    gfxSetColorArray(    &_acolSrfBase[0]);
    gfxDisableAlphaTest();
    gfxBlendFunc( GFX_DST_COLOR, GFX_SRC_COLOR);
    // do rendering
    RenderOneSide( rm, TRUE,  SRF_DETAIL);
    RenderOneSide( rm, FALSE, SRF_DETAIL);

    // detail rendering done
    _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_RENDER_BUMP);
  }


  // RENDER REFLECTION LAYER -------------------------------------------------------------------


  // if this model has reflection mapping
  extern INDEX mdl_bRenderReflection;
  CTextureData *ptdReflection = (CTextureData*)mo_toReflection.GetData();
  if( (_ulMipLayerFlags&SRF_REFLECTIONS) && mdl_bRenderReflection && ptdReflection!=NULL && bAllLayers)
  {
    _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_INIT_REFL_MIP);
    _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_INIT_REFL_MIP, _ctAllMipVx);
    // cache rotation
    const FLOATmatrix3D &m = rm.rm_mObjectRotation;

#if (defined __MSVC_INLINE__)
    __asm {
      push    ebx
      mov     ebx,D [m]
      mov     esi,D [pnorMipBase]
      mov     edi,D [ptexMipBase]
      mov     ecx,D [_ctAllMipVx]
reflMipLoop:
      // get normal in absolute space
      fld     D [esi]GFXNormal.nx
      fmul    D [ebx+ 0*3+0]
      fld     D [esi]GFXNormal.ny
      fmul    D [ebx+ 0*3+4]
      fld     D [esi]GFXNormal.nz
      fmul    D [ebx+ 0*3+8]
      fxch    st(2)
      faddp   st(1),st(0) 
      faddp   st(1),st(0)   // fNx
      fld     D [esi]GFXNormal.nx
      fmul    D [ebx+ 4*3+0]
      fld     D [esi]GFXNormal.ny
      fmul    D [ebx+ 4*3+4]
      fld     D [esi]GFXNormal.nz
      fmul    D [ebx+ 4*3+8]
      fxch    st(2)
      faddp   st(1),st(0) 
      faddp   st(1),st(0)   // fNy, fNx
      fld     D [esi]GFXNormal.nx
      fmul    D [ebx+ 8*3+0]
      fld     D [esi]GFXNormal.ny
      fmul    D [ebx+ 8*3+4]
      fld     D [esi]GFXNormal.nz
      fmul    D [ebx+ 8*3+8]
      fxch    st(2)
      faddp   st(1),st(0) 
      faddp   st(1),st(0)   // fNz, fNy, fNx
      // reflect viewer around normal
      fld     D [_vViewer+0]
      fmul    st(0), st(3)
      fld     D [_vViewer+4]
      fmul    st(0), st(3)
      fld     D [_vViewer+8]
      fmul    st(0), st(3)  // vNz, vNy, vNx, fNz, fNy, fNx
      fxch    st(2)
      faddp   st(1),st(0) 
      faddp   st(1),st(0)   // fNV, fNz, fNy, fNx
      fmul    st(3),st(0)
      fmul    st(2),st(0)
      fmulp   st(1),st(0)   // fNz*fNV, fNy*fNV, fNx*fNV
      fxch    st(2)
      fadd    st(0),st(0)   // 2*fNx*fNV, fNy*fNV, fNz*fNV
      fxch    st(1)
      fadd    st(0),st(0)   // 2*fNy*fNV, 2*fNx*fNV, fNz*fNV
      fxch    st(2)
      fadd    st(0),st(0)    // 2*fNz*fNV, 2*fNx*fNV, 2*fNy*fNV
      fxch    st(1)          // 2*fNx*fNV, 2*fNz*fNV, 2*fNy*fNV
      fsubr   D [_vViewer+0]
      fxch    st(2)          // 2*fNy*fNV, 2*fNz*fNV, fRVx
      fsubr   D [_vViewer+4]
      fxch    st(1)          // 2*fNz*fNV, fRVy, fRVx
      fsubr   D [_vViewer+8] // fRVz, fRVy, fRVx
      // calc 1oFM
      fxch    st(1)          // fRVy, fRVz, fRVx
      fadd    st(0),st(0)
      fadd    D [f2]
      fsqrt
      fdivr   D [f05]
      // map reflected vector to texture
      fmul    st(2),st(0)    // f1oFM, fRVz, s
      fmulp   st(1),st(0) 
      fxch    st(1)          // s, t
      fadd    D [f05]
      fxch    st(1)     
      fadd    D [f05]
      fxch    st(1)     
      fstp    D [edi+0]
      fstp    D [edi+4]
      add     esi,3*4
      add     edi,2*4
      dec     ecx
      jnz     reflMipLoop
      pop     ebx
    }
#else
    // for each mip vertex
    for( INDEX iMipVx=0; iMipVx<_ctAllMipVx; iMipVx++)
    { // get normal in absolute space
      const GFXNormal3 &nor = pnorMipBase[iMipVx];
      const FLOAT fNx = nor.nx*m(1,1) + nor.ny*m(1,2) + nor.nz*m(1,3);
      const FLOAT fNy = nor.nx*m(2,1) + nor.ny*m(2,2) + nor.nz*m(2,3);
      const FLOAT fNz = nor.nx*m(3,1) + nor.ny*m(3,2) + nor.nz*m(3,3);
      // reflect viewer around normal
      const FLOAT fNV  = fNx*_vViewer(1) + fNy*_vViewer(2) + fNz*_vViewer(3);
      const FLOAT fRVx = _vViewer(1) - 2*fNx*fNV;
      const FLOAT fRVy = _vViewer(2) - 2*fNy*fNV;
      const FLOAT fRVz = _vViewer(3) - 2*fNz*fNV;
      // map reflected vector to texture 
      // NOTE: using X and Z axes, so that singularity gets on -Y axis (where it will least probably be seen)
      const FLOAT f1oFM = 0.5f / sqrt(2+2*fRVy);  
      ptexMipBase[iMipVx].st.s = fRVx*f1oFM +0.5f;
      ptexMipBase[iMipVx].st.t = fRVz*f1oFM +0.5f;
    }
#endif
    _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_INIT_REFL_MIP);

    // setup surface vertices
    _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_INIT_REFL_SURF);
    _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_INIT_REFL_SURF, _ctAllSrfVx);

    // get model reflection color
    GFXColor colMdlRefl;
    const COLOR colR = AdjustColor( rm.rm_pmdModelData->md_colReflections, _slTexHueShift, _slTexSaturation);
	colMdlRefl.ul.abgr = ByteSwap(colR);
    colMdlRefl.AttenuateA( (rm.rm_colBlend&CT_AMASK)>>CT_ASHIFT);

    // for each reflective surface in current mip model
    FOREACHINSTATICARRAY( mmi.mmpi_MappingSurfaces, MappingSurface, itms)
    {
      const MappingSurface &ms = *itms;
      iSrfVx0 = ms.ms_iSrfVx0;
      ctSrfVx = ms.ms_ctSrfVx;
      if(  (ms.ms_ulRenderingFlags&SRF_INVISIBLE) || ctSrfVx==0) break;  // done if found invisible or empty surface
      if( !(ms.ms_ulRenderingFlags&SRF_REFLECTIONS)) continue;  // skip non-reflection surface
      // cache surface pointers
      puwSrfToMip = &mmi.mmpi_auwSrfToMip[iSrfVx0];
      ptexSrfBase = &_atexSrfBase[iSrfVx0];
      pcolSrfBase = &_acolSrfBase[iSrfVx0];
      // get surface reflection color and combine with model color
      GFXColor colSrfRefl;
      const COLOR colR = AdjustColor( ms.ms_colReflections, _slTexHueShift, _slTexSaturation);
      colSrfRefl.MultiplyRGBA( colR, colMdlRefl);

      // set reflection texture coords
      for( INDEX iSrfVx=0; iSrfVx<ctSrfVx; iSrfVx++) {
        const INDEX iMipVx = puwSrfToMip[iSrfVx];
        ptexSrfBase[iSrfVx] = ptexMipBase[iMipVx];
      }
      // for full-bright surfaces
      if( ms.ms_sstShadingType==SST_FULLBRIGHT) {
        // just copy reflection color
        for( INDEX iSrfVx=0; iSrfVx<ctSrfVx; iSrfVx++) pcolSrfBase[iSrfVx] = colSrfRefl;
      }
      else { // for smooth surface
        for( INDEX iSrfVx=0; iSrfVx<ctSrfVx; iSrfVx++) {
          // set reflection color smooth
          const INDEX iMipVx = puwSrfToMip[iSrfVx];
          pcolSrfBase[iSrfVx].MultiplyRGBCopyA1( colSrfRefl, pcolMipBase[iMipVx]);
        }
      }
      // eventually attenuate color in case of fog or haze
      if( (ms.ms_ulRenderingFlags&SRF_OPAQUE) && !_bForceTranslucency) continue;
      // eventually do some haze and/or fog attenuation of alpha channel in surface
      if( rm.rm_ulFlags & RMF_HAZE) {
        if( ms.ms_sttTranslucencyType==STT_MULTIPLY) AttenuateColor( pshdMipHaze, ctSrfVx);
        else AttenuateAlpha( pshdMipHaze, ctSrfVx);
      }
      if( rm.rm_ulFlags & RMF_FOG) {
        if( ms.ms_sttTranslucencyType==STT_MULTIPLY) AttenuateColor( pshdMipFogy, ctSrfVx);
        else AttenuateAlpha( pshdMipFogy, ctSrfVx);
      }
    }
    // reflection prep done
    _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_INIT_REFL_SURF);

    // begin rendering
    _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_RENDER_REFLECTIONS);
    _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_RENDER_REFLECTIONS);

    // setup texture/color arrays and rendering mode
    SetCurrentTexture( ptdReflection, mo_toReflection.GetFrame());
    gfxSetTexCoordArray( &_atexSrfBase[0], FALSE);
    gfxSetColorArray(    &_acolSrfBase[0]);
    gfxBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
    // do rendering
    RenderOneSide( rm, TRUE,  SRF_REFLECTIONS);
    RenderOneSide( rm, FALSE, SRF_REFLECTIONS);

    // reflection rendering done
    _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_RENDER_REFLECTIONS);
  }


  // RENDER SPECULAR LAYER -------------------------------------------------------------------


  // if this model has specular mapping
  extern INDEX mdl_bRenderSpecular;
  CTextureData *ptdSpecular = (CTextureData*)mo_toSpecular.GetData();
  if( (_ulMipLayerFlags&SRF_SPECULAR) && mdl_bRenderSpecular && ptdSpecular!=NULL && bAllLayers)
  {
    _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_INIT_SPEC_MIP);
    _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_INIT_SPEC_MIP, _ctAllMipVx);
    // cache object view rotation
    const FLOATmatrix3D &m = rm.rm_mObjectToView;

#if (defined __MSVC_INLINE__)
    __asm {
      push    ebx
      mov     ebx,D [m]
      mov     esi,D [pnorMipBase]
      mov     edi,D [ptexMipBase]
      mov     ecx,D [_ctAllMipVx]
specMipLoop:
      // reflect light vector around vertex normal in object space
      fld     D [esi]GFXNormal.nx
      fmul    D [_vLightObj+0]
      fld     D [esi]GFXNormal.ny
      fmul    D [_vLightObj+4]
      fld     D [esi]GFXNormal.nz
      fmul    D [_vLightObj+8]
      fxch    st(2)
      faddp   st(1),st(0) 
      faddp   st(1),st(0) // fNL
      fld     D [esi]GFXNormal.nx
      fmul    st(0),st(1) // fnl*nx, fnl
      fld     D [esi]GFXNormal.ny
      fmul    st(0),st(2) // fnl*ny, fnl*nx, fnl
      fld     D [esi]GFXNormal.nz
      fmulp   st(3),st(0) // fnl*ny, fnl*nx, fnl*nz
      fxch    st(1)       // fnl*nx, fnl*ny, fnl*nz
      fadd    st(0),st(0)
      fxch    st(1)       // fnl*ny, 2*fnl*nx, fnl*nz
      fadd    st(0),st(0)
      fxch    st(2)       
      fadd    st(0),st(0) 
      fxch    st(1)       // 2*fnl*nx, 2*fnl*nz, 2*fnl*ny
      fsubr   D [_vLightObj+0]
      fxch    st(2)       // 2*fnl*ny, 2*fnl*nz, fRx
      fsubr   D [_vLightObj+4]
      fxch    st(1)       // 2*fnl*nz, fRy, fRx
      fsubr   D [_vLightObj+8]
      fxch    st(2)       // fRx, fRy, fRz
      // transform the reflected vector to viewer space
      fld     D [ebx+ 8*3+0]
      fmul    st(0),st(1)
      fld     D [ebx+ 8*3+4]
      fmul    st(0),st(3)
      fld     D [ebx+ 8*3+8]
      fmul    st(0),st(5)
      fxch    st(2)
      faddp   st(1),st(0)
      faddp   st(1),st(0)     // fRVz, fRx, fRy, fRz
      fld     D [ebx+ 4*3+0]
      fmul    st(0),st(2)
      fld     D [ebx+ 4*3+4]
      fmul    st(0),st(4)
      fld     D [ebx+ 4*3+8]
      fmul    st(0),st(6)    
      fxch    st(2)
      faddp   st(1),st(0)
      faddp   st(1),st(0)    // fRVy, fRVz, fRx, fRy, fRz
      fxch    st(2)          // fRx,  fRVz, fRVy, fRy, fRz
      fmul    D [ebx+ 0*3+0] // fRxx, fRVz, fRVy, fRy, fRz
      fxch    st(3)
      fmul    D [ebx+ 0*3+4] // fRxy, fRVz, fRVy, fRxx, fRz
      fxch    st(4)
      fmul    D [ebx+ 0*3+8] // fRxz, fRVz, fRVy, fRxx, fRxy
      fxch    st(3)          // fRxx, fRVz, fRVy, fRxz, fRxy
      faddp   st(4),st(0)    // fRVz, fRVy, fRxz, fRxy+fRxx
      fxch    st(2)          // fRxz, fRVy, fRVz, fRxy+fRxx
      faddp   st(3),st(0)    // fRVy, fRVz, fRVx
      // calc 1oFM
      fxch    st(1)          // fRVz, fRVy, fRVx
      fadd    st(0),st(0)
      fadd    D [f2]
      fsqrt
      fdivr   D [f05]
      // map reflected vector to texture
      fmul    st(2),st(0)    // f1oFM, fRVy, s
      fmulp   st(1),st(0) 
      fxch    st(1)          // s, t
      fadd    D [f05]
      fxch    st(1)     
      fadd    D [f05]
      fxch    st(1)     
      fstp    D [edi+0]
      fstp    D [edi+4]
      add     esi,3*4
      add     edi,2*4
      dec     ecx
      jnz     specMipLoop
      pop     ebx
    }
#else
    // for each mip vertex
    for( INDEX iMipVx=0; iMipVx<_ctAllMipVx; iMipVx++)
    { // reflect light vector around vertex normal in object space
      GFXNormal3 &nor = pnorMipBase[iMipVx];
      const FLOAT fNL = nor.nx*_vLightObj(1) + nor.ny*_vLightObj(2) +	nor.nz*_vLightObj(3);
      const FLOAT fRx = _vLightObj(1) - 2*nor.nx*fNL;
      const FLOAT fRy = _vLightObj(2) - 2*nor.ny*fNL;
      const FLOAT fRz = _vLightObj(3) - 2*nor.nz*fNL;
      // transform the reflected vector to viewer space
      const FLOAT fRVx = fRx*m(1,1) + fRy*m(1,2) + fRz*m(1,3);
      const FLOAT fRVy = fRx*m(2,1) + fRy*m(2,2) + fRz*m(2,3);
      const FLOAT fRVz = fRx*m(3,1) + fRy*m(3,2) + fRz*m(3,3);
      // map reflected vector to texture
      const FLOAT f1oFM = 0.5f / sqrt(2+2*fRVz);  // was 2*sqrt(2+2*fRVz)
      ptexMipBase[iMipVx].st.s = fRVx*f1oFM +0.5f;
      ptexMipBase[iMipVx].st.t = fRVy*f1oFM +0.5f;
    }
#endif
    _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_INIT_SPEC_MIP);

    // setup surface vertices
    _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_INIT_SPEC_SURF);
    _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_INIT_SPEC_SURF, _ctAllSrfVx);

    // get model specular color and multiply with light color
    GFXColor colMdlSpec;
    const COLOR colS = AdjustColor( rm.rm_pmdModelData->md_colSpecular, _slTexHueShift, _slTexSaturation);
	colMdlSpec.ul.abgr = ByteSwap(colS);
	colMdlSpec.AttenuateRGB((rm.rm_colBlend&CT_AMASK) >> CT_ASHIFT);
	colMdlSpec.ub.r = ClampUp((colMdlSpec.ub.r *_slLR) >> 8, (SLONG)255);
	colMdlSpec.ub.g = ClampUp((colMdlSpec.ub.g *_slLG) >> 8, (SLONG)255);
	colMdlSpec.ub.b = ClampUp((colMdlSpec.ub.b *_slLB) >> 8, (SLONG)255);
    // for each specular surface in current mip model
    FOREACHINSTATICARRAY( mmi.mmpi_MappingSurfaces, MappingSurface, itms)
    {
      const MappingSurface &ms = *itms;
      iSrfVx0 = ms.ms_iSrfVx0;
      ctSrfVx = ms.ms_ctSrfVx;
      if(  (ms.ms_ulRenderingFlags&SRF_INVISIBLE) || ctSrfVx==0) break;  // done if found invisible or empty surface
      if( !(ms.ms_ulRenderingFlags&SRF_SPECULAR)) continue;  // skip non-specular surface
      // cache surface pointers
      puwSrfToMip = &mmi.mmpi_auwSrfToMip[iSrfVx0];
      ptexSrfBase = &_atexSrfBase[iSrfVx0];
      pcolSrfBase = &_acolSrfBase[iSrfVx0];
      // get surface specular color and combine with model color
      GFXColor colSrfSpec;
      const COLOR colS = AdjustColor( ms.ms_colSpecular, _slTexHueShift, _slTexSaturation);
      colSrfSpec.MultiplyRGB( colS, colMdlSpec);

      // for each vertex in the surface
      for( INDEX iSrfVx=0; iSrfVx<ctSrfVx; iSrfVx++) {
        const INDEX iMipVx = puwSrfToMip[iSrfVx];
        // set specular texture and color
        ptexSrfBase[iSrfVx] = ptexMipBase[iMipVx];
		const SLONG slShade = pcolMipBase[iMipVx].ub.a;
		pcolSrfBase[iSrfVx].ul.abgr = (((colSrfSpec.ub.r)*slShade) >> 8)
			| (((colSrfSpec.ub.g)*slShade) & 0x0000FF00)
			| ((((colSrfSpec.ub.b)*slShade) << 8) & 0x00FF0000);
      }
      // eventually attenuate color in case of fog or haze
      if( (ms.ms_ulRenderingFlags&SRF_OPAQUE) && !_bForceTranslucency) continue;
      // eventually do some haze and/or fog attenuation of alpha channel in surface
      if( rm.rm_ulFlags & RMF_HAZE) {
        if( ms.ms_sttTranslucencyType==STT_MULTIPLY) AttenuateColor( pshdMipHaze, ctSrfVx);
        else AttenuateAlpha( pshdMipHaze, ctSrfVx);
      }
      if( rm.rm_ulFlags & RMF_FOG) {
        if( ms.ms_sttTranslucencyType==STT_MULTIPLY) AttenuateColor( pshdMipFogy, ctSrfVx);
        else AttenuateAlpha( pshdMipFogy, ctSrfVx);
      }
    }
    // specular prep done
    _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_INIT_SPEC_SURF);

    // begin rendering
    _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_RENDER_SPECULAR);
    _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_RENDER_SPECULAR);

    // setup texture/color arrays and rendering mode
    SetCurrentTexture( ptdSpecular, mo_toSpecular.GetFrame());
    gfxSetTexCoordArray( &_atexSrfBase[0], FALSE);
    gfxSetColorArray(    &_acolSrfBase[0]);
    gfxBlendFunc( GFX_INV_SRC_ALPHA, GFX_ONE);
    // do rendering
    RenderOneSide( rm, TRUE , SRF_SPECULAR);
    RenderOneSide( rm, FALSE, SRF_SPECULAR);

    // specular rendering done
    _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_RENDER_SPECULAR);
  }


 
  // RENDER HAZE LAYER -------------------------------------------------------------------

  
  // if this model has haze and some opaque surfaces
  if( (rm.rm_ulFlags&RMF_HAZE) && !(_ulMipLayerFlags&MMI_TRANSLUCENT))
  {
    _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_INIT_HAZE_SURF);
    _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_INIT_HAZE_SURF, _ctAllSrfVx);
    // unpack haze color
    const COLOR colH = AdjustColor( _haze_hp.hp_colColor, _slTexHueShift, _slTexSaturation);
    GFXColor colHaze(colH);

    // for each surface in current mip model
    FOREACHINSTATICARRAY( mmi.mmpi_MappingSurfaces, MappingSurface, itms)
    {
      MappingSurface &ms = *itms;
      iSrfVx0 = ms.ms_iSrfVx0;
      ctSrfVx = ms.ms_ctSrfVx;
      if(  (ms.ms_ulRenderingFlags&SRF_INVISIBLE) || ctSrfVx==0) break;  // done if found invisible or empty surface
      if( !(ms.ms_ulRenderingFlags&SRF_OPAQUE)) continue;  // skip not-solid or empty surfaces
      // cache surface pointers
      puwSrfToMip = &mmi.mmpi_auwSrfToMip[iSrfVx0];
      ptexSrfBase = &_atexSrfBase[iSrfVx0];
      pcolSrfBase = &_acolSrfBase[iSrfVx0];

      // prepare haze vertices
      for( INDEX iSrfVx=0; iSrfVx<ctSrfVx; iSrfVx++) {
        const INDEX iMipVx = puwSrfToMip[iSrfVx];
		ptexSrfBase[iSrfVx].st.s = ptx1MipHaze[iMipVx];
		ptexSrfBase[iSrfVx].st.t = 0.0f;
        pcolSrfBase[iSrfVx] = colHaze;
      }
      // mark that this surface has haze
      ms.ms_ulRenderingFlags |= SRF_HAZE;
    }
    // haze prep done
    _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_INIT_HAZE_SURF);

    // begin rendering
    _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_RENDER_HAZE);
    _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_RENDER_HAZE);

    // setup texture/color arrays and rendering mode
    gfxSetTextureWrapping( GFX_CLAMP, GFX_CLAMP);
    gfxSetTexture( _haze_ulTexture, _haze_tpLocal);
    gfxSetTexCoordArray( &_atexSrfBase[0], FALSE);
    gfxSetColorArray(    &_acolSrfBase[0]);
    gfxBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
    // do rendering
    RenderOneSide( rm, TRUE,  SRF_HAZE);
    RenderOneSide( rm, FALSE, SRF_HAZE);

    // haze rendering done
    _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_RENDER_HAZE);
  }



  // RENDER FOG LAYER -------------------------------------------------------------------

  
  // if this model has fog and some opaque surfaces
  if( (rm.rm_ulFlags&RMF_FOG) && !(_ulMipLayerFlags&MMI_TRANSLUCENT))
  {
    _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_INIT_FOG_SURF);
    _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_INIT_FOG_SURF, _ctAllSrfVx);
    // unpack fog color
    const COLOR colF = AdjustColor( _fog_fp.fp_colColor, _slTexHueShift, _slTexSaturation);
    GFXColor colFog(colF);

    // for each surface in current mip model
    FOREACHINSTATICARRAY( mmi.mmpi_MappingSurfaces, MappingSurface, itms)
    {
      MappingSurface &ms = *itms;
      iSrfVx0 = ms.ms_iSrfVx0;
      ctSrfVx = ms.ms_ctSrfVx;
      if(  (ms.ms_ulRenderingFlags&SRF_INVISIBLE) || ctSrfVx==0) break;  // done if found invisible or empty surface
      if( !(ms.ms_ulRenderingFlags&SRF_OPAQUE)) continue;  // skip not-solid or empty surfaces
      // cache surface pointers
      puwSrfToMip = &mmi.mmpi_auwSrfToMip[iSrfVx0];
      ptexSrfBase = &_atexSrfBase[iSrfVx0];
      pcolSrfBase = &_acolSrfBase[iSrfVx0];

      // prepare fog vertices
      for( INDEX iSrfVx=0; iSrfVx<ctSrfVx; iSrfVx++) {
        const INDEX iMipVx = puwSrfToMip[iSrfVx];
        ptexSrfBase[iSrfVx] = ptexMipFogy[iMipVx];
        pcolSrfBase[iSrfVx] = colFog;
      }
      // mark that this surface has fog
      ms.ms_ulRenderingFlags |= SRF_FOG;
    }
    // fog prep done
    _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_INIT_FOG_SURF);

    // begin rendering
    _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_RENDER_FOG);
    _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_RENDER_FOG);

    // setup texture/color arrays and rendering mode
    gfxSetTextureWrapping( GFX_CLAMP, GFX_CLAMP);
    gfxSetTexture( _fog_ulTexture, _fog_tpLocal);
    gfxSetTexCoordArray( &_atexSrfBase[0], FALSE);
    gfxSetColorArray(    &_acolSrfBase[0]);
    gfxBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
    gfxDisableAlphaTest();
    // do rendering
    RenderOneSide( rm, TRUE,  SRF_FOG);
    RenderOneSide( rm, FALSE, SRF_FOG);

    // fog rendering done
    _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_RENDER_FOG);
  }

  // almost done
  gfxDepthFunc( GFX_LESS_EQUAL);
  gfxUnlockArrays();

  // eventually render wireframe
  RenderWireframe(rm);

  // reset model vertex buffers and rendering face
  ResetVertexArrays();
  
  // model rendered (restore cull mode)
  gfxCullFace(GFX_BACK);
  _sfStats.StopTimer(CStatForm::STI_MODELRENDERING);
  if( bModelSetupTimer) _sfStats.StartTimer(CStatForm::STI_MODELSETUP);
  _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_RENDERMODEL);
}
#pragma warning(default: 4731)



// *******************************************************************************



// render patches on model
void CModelObject::RenderPatches_View( CRenderModel &rm)
{
  _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_RENDERPATCHES);
  _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_RENDERPATCHES);

  // setup API rendering functions
  gfxSetTextureWrapping( GFX_CLAMP, GFX_CLAMP);
  gfxDepthFunc( GFX_LESS_EQUAL);
  gfxBlendFunc( GFX_SRC_ALPHA, GFX_INV_SRC_ALPHA);
  gfxDisableAlphaTest();

  pglMatrixMode( GL_TEXTURE);
  gfxSetColorArray( &_acolSrfBase[0]);
  CModelData   *pmd  =  rm.rm_pmdModelData;
  ModelMipInfo *pmmi = &rm.rm_pmdModelData->md_MipInfos[rm.rm_iMipLevel];

  // get main texture size
  FLOAT fmexMainSizeU=1, fmexMainSizeV=1;
  FLOAT f1oMainSizeV =1, f1oMainSizeU =1;
  CTextureData *ptd = (CTextureData*)mo_toTexture.GetData();
  if( ptd!=NULL) {
    fmexMainSizeU = GetWidth();
    fmexMainSizeV = GetHeight();
    f1oMainSizeU  = 1.0f / fmexMainSizeU;
    f1oMainSizeV  = 1.0f / fmexMainSizeV;
  }

  // for each possible patch
  INDEX iExistingPatch=0;
  for( INDEX iMaskBit=0; iMaskBit<MAX_TEXTUREPATCHES; iMaskBit++)
  {
    CTextureObject &toPatch = pmd->md_mpPatches[ iMaskBit].mp_toTexture;
    CTextureData  *ptdPatch = (CTextureData*)toPatch.GetData();
    if( ptdPatch==NULL) continue;
    if( mo_PatchMask & ((1UL)<<iMaskBit)) {
      PolygonsPerPatch &ppp = pmmi->mmpi_aPolygonsPerPatch[iExistingPatch];
      if( ppp.ppp_auwElements.Count()==0) continue;
      // calculate correction factor (relative to greater texture dimension)
      const MEX2D mexPatchOffset = pmd->md_mpPatches[iMaskBit].mp_mexPosition;
      const FLOAT fSizeDivider   = 1.0f / pmd->md_mpPatches[iMaskBit].mp_fStretch;
      const FLOAT fmexSizeU = ptdPatch->GetWidth();
      const FLOAT fmexSizeV = ptdPatch->GetHeight();
      // set texture for API
      SetCurrentTexture( ptdPatch, toPatch.GetFrame());
      pglLoadIdentity();
      pglScalef( fmexMainSizeU/fmexSizeU*fSizeDivider, fmexMainSizeV/fmexSizeV*fSizeDivider, 0);
      pglTranslatef( -mexPatchOffset(1)*f1oMainSizeU, -mexPatchOffset(2)*f1oMainSizeV, 0);
      gfxSetTexCoordArray( &_atexSrfBase[0], FALSE);
      //AddElements( &ppp.ppp_auwElements[0], ppp.ppp_auwElements.Count());
      //FlushElements();
    }
    iExistingPatch++;
  }
  // all done
  pglLoadIdentity();
  pglMatrixMode( GL_MODELVIEW);
  OGL_CHECKERROR;

  // patches rendered
  _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_RENDERPATCHES);
}



// *******************************************************************************



// render comlex model shadow
void CModelObject::RenderShadow_View( CRenderModel &rm, const CPlacement3D &plLight,
                                        const FLOAT fFallOff, const FLOAT fHotSpot, const FLOAT fIntensity,
                                        const FLOATplane3D &plShadowPlane)
{
  // no shadow, if projection is not perspective or no textures
  if( !_aprProjection.IsPerspective() || !(rm.rm_rtRenderType & RT_TEXTURE)) return;

  _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_RENDERSHADOW);
  _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_RENDERSHADOW);

  // get viewer in absolute space
  FLOAT3D vViewerAbs = _aprProjection->ViewerPlacementR().pl_PositionVector;
  if( plShadowPlane.PointDistance(vViewerAbs)<0.01f) {
    // shadow destination plane is not visible - don't cast shadows
    _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_RENDERSHADOW);
    return;
  }

  // get light position in object space
  FLOATmatrix3D mAbsToObj = !rm.rm_mObjectRotation;
  const FLOAT3D vAbsToObj = -rm.rm_vObjectPosition;
  _vLightObj = (plLight.pl_PositionVector+vAbsToObj)*mAbsToObj;
  // get shadow plane in object space
  FLOATplane3D plShadowPlaneObj = (plShadowPlane+vAbsToObj)*mAbsToObj;

  // project object handle so we can calc how it is far away from viewer
  const FLOAT3D vRef = plShadowPlaneObj.ProjectPoint(FLOAT3D(0,0,0)) *rm.rm_mObjectToView +rm.rm_vObjectToView;
  plShadowPlaneObj.pl_distance += ClampDn( -vRef(3)*0.001f, 0.01f); // move plane towards the viewer a bit to avoid z-fighting

  // get distance from shadow plane to light
  const FLOAT fDl = plShadowPlaneObj.PointDistance(_vLightObj);
  if( fDl<0.1f) {
    // too small - do nothing
    _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_RENDERSHADOW);
    return;
  }

  ModelMipInfo &mmi = *rm.rm_pmmiMip;
  ModelMipInfo &mmi0 = rm.rm_pmdModelData->md_MipInfos[0];
  _pfModelProfile.IncrementCounter( CModelProfile::PCI_SHADOWVERTICES_FIRSTMIP, mmi0.mmpi_ctMipVx);
  _pfModelProfile.IncrementCounter( CModelProfile::PCI_SHADOWVERTICES_USEDMIP,  mmi.mmpi_ctMipVx);
  _pfModelProfile.IncrementCounter( CModelProfile::PCI_SHADOWSURFACEVERTICES_FIRSTMIP, mmi0.mmpi_ctSrfVx);
  _pfModelProfile.IncrementCounter( CModelProfile::PCI_SHADOWSURFACEVERTICES_USEDMIP,  mmi.mmpi_ctSrfVx);
  _pfModelProfile.IncrementCounter( CModelProfile::PCI_SHADOWTRIANGLES_FIRSTMIP, mmi0.mmpi_ctTriangles);
  _pfModelProfile.IncrementCounter( CModelProfile::PCI_SHADOWTRIANGLES_USEDMIP,  mmi.mmpi_ctTriangles);

  _sfStats.IncrementCounter( CStatForm::SCI_SHADOWTRIANGLES_FIRSTMIP, mmi0.mmpi_ctTriangles);
  _sfStats.IncrementCounter( CStatForm::SCI_SHADOWTRIANGLES_USEDMIP,  mmi.mmpi_ctTriangles); 

  // allocate vertex arrays
  _ctAllMipVx = mmi.mmpi_ctMipVx;
  _ctAllSrfVx = mmi.mmpi_ctSrfVx;
  ASSERT( _ctAllMipVx>0 && _ctAllSrfVx>0);
  ASSERT( _avtxMipBase.Count()==0);  _avtxMipBase.Push(_ctAllMipVx);
  ASSERT( _acolMipBase.Count()==0);  _acolMipBase.Push(_ctAllMipVx);
  ASSERT( _aooqMipShad.Count()==0);  _aooqMipShad.Push(_ctAllMipVx);
  ASSERT( _avtxSrfBase.Count()==0);  _avtxSrfBase.Push(_ctAllSrfVx);   
  ASSERT( _acolSrfBase.Count()==0);  _acolSrfBase.Push(_ctAllSrfVx);  
  ASSERT( _atx4SrfShad.Count()==0);  _atx4SrfShad.Push(_ctAllSrfVx);     

  // unpack one model frame vertices and eventually normals (lerped or not lerped, as required)
  pvtxMipBase = &_avtxMipBase[0];
  pooqMipShad = &_aooqMipShad[0];
  pcolMipBase = &_acolMipBase[0];
  UnpackFrame( rm, FALSE);
  UBYTE *pubsMipBase = (UBYTE*)pcolMipBase;

  _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_SHAD_INIT_MIP);
  _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_SHAD_INIT_MIP, _ctAllMipVx);

  // calculate light interpolants attenuated by ambient factor
  const INDEX iLightMax  = NormFloatToByte(fIntensity);  
  const FLOAT fLightStep = 255.0f* fIntensity/(fFallOff-fHotSpot);

  // for each mip vertex
  {for( INDEX iMipVx=0; iMipVx<_ctAllMipVx; iMipVx++)
  {
    // get object coordinates
    GFXVertex3 &vtx = pvtxMipBase[iMipVx];
    // get distance of the vertex from shadow plane
    FLOAT fDt = plShadowPlaneObj(1) *vtx.x
              + plShadowPlaneObj(2) *vtx.y
              + plShadowPlaneObj(3) *vtx.z
              - plShadowPlaneObj.Distance();
    // if vertex below shadow plane
    if( fDt<0) {
      // THIS MIGHT BE WRONG - it'll make shadow vertex to be the same as model vertex !!!!
      // fake as beeing just on plane
      fDt=0.0f;
      FLOAT fP = fDl/(fDl-fDt); // =1
      FLOAT fL = fDt/(fDl-fDt); // =0
      // calculate shadow vertex
      vtx.x = vtx.x*fP - _vLightObj(1)*fL;
      vtx.y = vtx.y*fP - _vLightObj(2)*fL;
      vtx.z = vtx.z*fP - _vLightObj(3)*fL;
      pooqMipShad[iMipVx] = 1.0f;
      // make it transparent
      pubsMipBase[iMipVx] = 0;
    }
    // if vertex above light
    else if( (fDl-fDt)<0.01f) {
      // fake as beeing just a bit below light
      fDt = fDl-0.1f;
      const FLOAT fDiv = 1.0f / (fDl-fDt);
      const FLOAT fP = fDl *fDiv;
      const FLOAT fL = fDt *fDiv;
      // calculate shadow vertex
      vtx.x = vtx.x*fP - _vLightObj(1)*fL;
      vtx.y = vtx.y*fP - _vLightObj(2)*fL;
      vtx.z = vtx.z*fP - _vLightObj(3)*fL;
      pooqMipShad[iMipVx] = 1.0f;
      // make it transparent
      pubsMipBase[iMipVx] = 0;
    }
    // if vertex between shadow plane and light
    else {
      const FLOAT fDiv = 1.0f / (fDl-fDt);
      const FLOAT fP = fDl *fDiv;
      const FLOAT fL = fDt *fDiv;
      // calculate shadow vertex
      vtx.x = vtx.x*fP - _vLightObj(1)*fL;
      vtx.y = vtx.y*fP - _vLightObj(2)*fL;
      vtx.z = vtx.z*fP - _vLightObj(3)*fL;
      pooqMipShad[iMipVx] = fP;
      // get distance between light and shadow vertex
      const FLOAT fDlsx = vtx.x - _vLightObj(1);
      const FLOAT fDlsy = vtx.y - _vLightObj(2);
      const FLOAT fDlsz = vtx.z - _vLightObj(3);
      const FLOAT fDls  = sqrt( fDlsx*fDlsx + fDlsy*fDlsy + fDlsz*fDlsz);
      // calculate shadow value for this vertex
           if( fDls<fHotSpot) pubsMipBase[iMipVx] = iLightMax;
      else if( fDls>fFallOff) pubsMipBase[iMipVx] = 0;
      else                    pubsMipBase[iMipVx] = FloatToInt( (fFallOff-fDls)*fLightStep);
    }
  }}
  _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_SHAD_INIT_MIP);

  // get diffuse texture corrections
  FLOAT fTexCorrU, fTexCorrV;
  CTextureData *ptd = (CTextureData*)mo_toTexture.GetData();
  if( ptd!=NULL) {
    fTexCorrU = 1.0f / ptd->GetWidth();
    fTexCorrV = 1.0f / ptd->GetHeight();
  } else {
    fTexCorrU = 1.0f;
    fTexCorrV = 1.0f;
  }

  _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_SHAD_INIT_SURF);
  _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_SHAD_INIT_SURF, _ctAllSrfVx);
  // for each surface in current mip model
  FOREACHINSTATICARRAY( mmi.mmpi_MappingSurfaces, MappingSurface, itms)
  {
    const MappingSurface &ms = *itms;
    const INDEX iSrfVx0 = ms.ms_iSrfVx0;
    const INDEX ctSrfVx = ms.ms_ctSrfVx;
    if( (ms.ms_ulRenderingFlags&SRF_INVISIBLE) || ctSrfVx==0) break;  // done if found invisible or empty surface
    // cache surface pointers
    puwSrfToMip = &mmi.mmpi_auwSrfToMip[iSrfVx0];
    const FLOAT2D *pvTexCoord = (const FLOAT2D*)&mmi.mmpi_avmexTexCoord[iSrfVx0];
    GFXVertex    *pvtxSrfBase  = &_avtxSrfBase[iSrfVx0];
    GFXTexCoord4 *ptx4SrfShad  = &_atx4SrfShad[iSrfVx0];
    GFXColor     *pcolSrfBase  = &_acolSrfBase[iSrfVx0];
    // for each vertex in the surface
    for( INDEX iSrfVx=0; iSrfVx<ctSrfVx; iSrfVx++) {
      const INDEX iMipVx = puwSrfToMip[iSrfVx];
      // get 3d coords projected on plane and color (alpha only)
      pvtxSrfBase[iSrfVx].x = pvtxMipBase[iMipVx].x;
      pvtxSrfBase[iSrfVx].y = pvtxMipBase[iMipVx].y;
      pvtxSrfBase[iSrfVx].z = pvtxMipBase[iMipVx].z;
	  pcolSrfBase[iSrfVx].ub.a = pubsMipBase[iMipVx];
      // get texture adjusted for perspective correction (aka projected mapping)
      const FLOAT fooq = pooqMipShad[iMipVx];
      ptx4SrfShad[iSrfVx].s = pvTexCoord[iSrfVx](1) *fTexCorrU *fooq;
      ptx4SrfShad[iSrfVx].t = pvTexCoord[iSrfVx](2) *fTexCorrV *fooq;
      ptx4SrfShad[iSrfVx].q = fooq;
    }
  }
  _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_SHAD_INIT_SURF);
  _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_SHAD_GLSETUP);
  _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_SHAD_GLSETUP, 1);

  // begin rendering
  const BOOL bModelSetupTimer = _sfStats.CheckTimer(CStatForm::STI_MODELSETUP);
  if( bModelSetupTimer) _sfStats.StopTimer(CStatForm::STI_MODELSETUP);
  _sfStats.StartTimer(CStatForm::STI_MODELRENDERING);

  // prepare vertex arrays for API
  gfxSetVertexArray( &_avtxSrfBase[0], _avtxSrfBase.Count());
  if(CVA_bModels) gfxLockArrays();
  gfxSetTexCoordArray( (GFXTexCoord*)&_atx4SrfShad[0], TRUE); // projective mapping!
  gfxSetColorArray( &_acolSrfBase[0]);

  // set projection and texture for API
  gfxSetTextureWrapping( GFX_REPEAT, GFX_REPEAT);
  INDEX iFrame=0;
  if( ptd!=NULL) iFrame = mo_toTexture.GetFrame();
  SetCurrentTexture( ptd, iFrame);

  gfxEnableDepthTest();
  gfxDepthFunc( GFX_LESS_EQUAL);
  gfxDisableDepthWrite();
  gfxEnableBlend();
  gfxBlendFunc( GFX_ZERO, GFX_INV_SRC_ALPHA);
  gfxDisableAlphaTest();
  gfxDisableTruform();

  gfxEnableClipping();
  gfxCullFace(GFX_BACK);

  _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_SHAD_GLSETUP);

  _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_SHAD_RENDER);
  // for each surface in current mip model
  INDEX iStartElem=0;
  INDEX ctElements=0;
  {FOREACHINSTATICARRAY( mmi.mmpi_MappingSurfaces, MappingSurface, itms)
  { 
    // done if surface is invisible or empty
    const MappingSurface &ms = *itms;
    if( (ms.ms_ulRenderingFlags&SRF_INVISIBLE) || ms.ms_ctSrfVx==0) break;
    // batch elements up to non-diffuse surface
    if( ms.ms_ulRenderingFlags&SRF_DIFFUSE) {
      ctElements += ms.ms_ctSrfEl;
      continue;
    }
    // flush batched elements
    if( ctElements>0) FlushElements( ctElements, &mmi.mmpi_aiElements[iStartElem]);
    _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_SHAD_RENDER, ctElements/3);
    iStartElem+= ctElements+ms.ms_ctSrfEl;
    ctElements = 0;
  }}
  // flush leftovers
  if( ctElements>0) FlushElements( ctElements, &mmi.mmpi_aiElements[iStartElem]);

  // done
  gfxUnlockArrays();

  // reset vertex arrays
  _avtxMipBase.PopAll();
  _acolMipBase.PopAll();
  _aooqMipShad.PopAll();
  _avtxSrfBase.PopAll();
  _acolSrfBase.PopAll();
  _atx4SrfShad.PopAll();

  // shadow rendered
  _sfStats.StopTimer(CStatForm::STI_MODELRENDERING);
  if( bModelSetupTimer) _sfStats.StartTimer(CStatForm::STI_MODELSETUP);
  _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_SHAD_RENDER);
  _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_RENDERSHADOW);
}


// *******************************************************************************


// render simple model shadow
void CModelObject::AddSimpleShadow_View( CRenderModel &rm, const FLOAT fIntensity,
                                           const FLOATplane3D &plShadowPlane)
{
  _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_RENDERSIMPLESHADOW);
  _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_RENDERSIMPLESHADOW);

  // get viewer in absolute space
  FLOAT3D vViewerAbs = _aprProjection->ViewerPlacementR().pl_PositionVector;
  // if shadow destination plane is not visible, don't cast shadows
  if( plShadowPlane.PointDistance(vViewerAbs)<0.01f) {
    _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_RENDERSIMPLESHADOW);
    return;
  }

  _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_SIMP_CALC);
  _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_SIMP_CALC);

  // get shadow plane in object space
  const FLOATmatrix3D mAbsToObj = !rm.rm_mObjectRotation;
  const FLOAT3D vAbsToObj = -rm.rm_vObjectPosition;
  FLOATplane3D plShadowPlaneObj = (plShadowPlane+vAbsToObj) * mAbsToObj;

  // project object handle so we can calc how it is far away from viewer
  const FLOAT3D vRef = plShadowPlaneObj.ProjectPoint(FLOAT3D(0,0,0)) *rm.rm_mObjectToView +rm.rm_vObjectToView;
  plShadowPlaneObj.pl_distance += ClampDn( -vRef(3)*0.001f, 0.01f); // move plane towards the viewer a bit to avoid z-fighting

  // find points on plane nearest to bounding box edges
  FLOAT3D vMin = rm.rm_vObjectMinBB * 1.25f;
  FLOAT3D vMax = rm.rm_vObjectMaxBB * 1.25f;
  if( rm.rm_ulFlags & RMF_SPECTATOR) { vMin*=2; vMax*=2; } // enlarge shadow for 1st person view
  const FLOAT3D v00 = plShadowPlaneObj.ProjectPoint(FLOAT3D(vMin(1),vMin(2),vMin(3))) *rm.rm_mObjectToView +rm.rm_vObjectToView;
  const FLOAT3D v01 = plShadowPlaneObj.ProjectPoint(FLOAT3D(vMin(1),vMin(2),vMax(3))) *rm.rm_mObjectToView +rm.rm_vObjectToView;
  const FLOAT3D v10 = plShadowPlaneObj.ProjectPoint(FLOAT3D(vMax(1),vMin(2),vMin(3))) *rm.rm_mObjectToView +rm.rm_vObjectToView;
  const FLOAT3D v11 = plShadowPlaneObj.ProjectPoint(FLOAT3D(vMax(1),vMin(2),vMax(3))) *rm.rm_mObjectToView +rm.rm_vObjectToView;
  // calc done
  _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_SIMP_CALC);

  _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_SIMP_COPY);
  _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_SIMP_COPY);

  // prepare color
  ASSERT( fIntensity>=0 && fIntensity<=1);
  ULONG ulAAAA = NormFloatToByte(fIntensity);
  ulAAAA |= (ulAAAA<<8) | (ulAAAA<<16); // alpha isn't needed

  // add to vertex arrays
  GFXVertex   *pvtx = _avtxCommon.Push(4);
  GFXTexCoord *ptex = _atexCommon.Push(4);
  GFXColor    *pcol = _acolCommon.Push(4);
  // vertices
  pvtx[0].x = v00(1);  pvtx[0].y = v00(2);  pvtx[0].z = v00(3);
  pvtx[2].x = v11(1);  pvtx[2].y = v11(2);  pvtx[2].z = v11(3);
  if( rm.rm_ulFlags & RMF_INVERTED) { // must re-adjust order for mirrored projection
    pvtx[1].x = v10(1);  pvtx[1].y = v10(2);  pvtx[1].z = v10(3);
    pvtx[3].x = v01(1);  pvtx[3].y = v01(2);  pvtx[3].z = v01(3);
  } else {
    pvtx[1].x = v01(1);  pvtx[1].y = v01(2);  pvtx[1].z = v01(3);
    pvtx[3].x = v10(1);  pvtx[3].y = v10(2);  pvtx[3].z = v10(3);
  }
  // texture coords
  ptex[0].st.s = 0;  ptex[0].st.t = 0;
  ptex[1].st.s = 0;  ptex[1].st.t = 1;
  ptex[2].st.s = 1;  ptex[2].st.t = 1;
  ptex[3].st.s = 1;  ptex[3].st.t = 0;
  // colors
  pcol[0].ul.abgr = ulAAAA;
  pcol[1].ul.abgr = ulAAAA;
  pcol[2].ul.abgr = ulAAAA;
  pcol[3].ul.abgr = ulAAAA;

  // if this model has fog
  if( rm.rm_ulFlags & RMF_FOG)
  { // for each vertex in shadow quad
    GFXTexCoord tex;
    for( INDEX i=0; i<4; i++) {
      GFXVertex &vtx = pvtx[i];
      // get distance along viewer axis and fog axis and map to texture and attenuate shadow color
      const FLOAT fH = vtx.x*_fog_vHDirView(1) + vtx.y*_fog_vHDirView(2) + vtx.z*_fog_vHDirView(3);
	  tex.st.s = -vtx.z *_fog_fMulZ;
	  tex.st.t = (fH + _fog_fAddH) *_fog_fMulH;
      pcol[i].AttenuateRGB(GetFogAlpha(tex)^255);
    }
  }
  // if this model has haze
  if( rm.rm_ulFlags & RMF_HAZE)
  { // for each vertex in shadow quad
    for( INDEX i=0; i<4; i++) {
      // get distance along viewer axis map to texture  and attenuate shadow color
      const FLOAT fS = (_haze_fAdd-pvtx[i].z) *_haze_fMul;
      pcol[i].AttenuateRGB(GetHazeAlpha(fS)^255);
    }
  }

  // one simple shadow added to rendering queue
  _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_SIMP_COPY);
  _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_RENDERSIMPLESHADOW);
}



// render several simple model shadows
void RenderBatchedSimpleShadows_View(void)
{
  const INDEX ctVertices = _avtxCommon.Count();
  if( ctVertices<=0) return;
  // safety checks
  ASSERT( _atexCommon.Count()==ctVertices);
  ASSERT( _acolCommon.Count()==ctVertices);
  _pfModelProfile.StartTimer( CModelProfile::PTI_VIEW_SIMP_BATCHED);
  _pfModelProfile.IncrementTimerAveragingCounter( CModelProfile::PTI_VIEW_SIMP_BATCHED);

  // begin rendering
  const BOOL bModelSetupTimer = _sfStats.CheckTimer(CStatForm::STI_MODELSETUP);
  if( bModelSetupTimer) _sfStats.StopTimer(CStatForm::STI_MODELSETUP);
  _sfStats.StartTimer(CStatForm::STI_MODELRENDERING);

  // setup texture and projection
  _bFlatFill = FALSE;
  gfxSetViewMatrix(NULL);
  gfxCullFace(GFX_BACK);
  gfxSetTextureWrapping( GFX_REPEAT, GFX_REPEAT);
  CTextureData *ptd = (CTextureData*)_toSimpleModelShadow.GetData();
  SetCurrentTexture( ptd, _toSimpleModelShadow.GetFrame());

  // setup rendering mode
  gfxEnableDepthTest();
  gfxDepthFunc( GFX_LESS_EQUAL);
  gfxDisableDepthWrite();
  gfxEnableBlend();
  gfxBlendFunc( GFX_ZERO, GFX_INV_SRC_COLOR);
  gfxDisableAlphaTest();
  gfxEnableClipping();
  gfxDisableTruform();

  // draw!
  _pGfx->gl_ctModelTriangles += ctVertices/2; 
  gfxFlushQuads();

  // all simple shadows rendered
  gfxResetArrays();
  _sfStats.StopTimer(CStatForm::STI_MODELRENDERING);
  if( bModelSetupTimer) _sfStats.StartTimer(CStatForm::STI_MODELSETUP);
  _pfModelProfile.StopTimer( CModelProfile::PTI_VIEW_SIMP_BATCHED);
}





