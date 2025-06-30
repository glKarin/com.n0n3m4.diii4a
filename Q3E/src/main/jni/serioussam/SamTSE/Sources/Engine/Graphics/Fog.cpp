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

#include <Engine/Base/Memory.h>
#include <Engine/Base/FileName.h>
#include <Engine/Base/Statistics_Internal.h>
#include <Engine/Math/Matrix.h>
#include <Engine/Math/Functions.h>
#include <Engine/Graphics/Color.h>
#include <Engine/Graphics/GfxLibrary.h>
#include <Engine/Graphics/Fog_internal.h>
#include <Engine/Graphics/GfxProfile.h>
#include <Engine/Graphics/ImageInfo.h>

// asm shortcuts
#define O offset
#define Q qword ptr
#define D dword ptr
#define W  word ptr
#define B  byte ptr

// current fog parameters
BOOL _fog_bActive = FALSE;
CFogParameters _fog_fp;
CTexParams _fog_tpLocal;

FLOAT _fog_fViewerH = 0.0f;
FLOAT3D _fog_vViewPosAbs;
FLOAT3D _fog_vViewDirAbs;
FLOAT3D _fog_vHDirAbs;
FLOAT3D _fog_vHDirView;
FLOAT _fog_fMulZ=0;
FLOAT _fog_fMulH=0;
FLOAT _fog_fAddH=0;
ULONG _fog_ulAlpha=0;
ULONG _fog_ulTexture=0;
ULONG _fog_ulFormat=0;

PIX _fog_pixSizeH=0;
PIX _fog_pixSizeL=0;
FLOAT _fog_fStart=0;  // where in height fog starts
FLOAT _fog_fEnd=0;    // where in height fog ends
UBYTE *_fog_pubTable=NULL;


extern INDEX gfx_bRenderFog;
extern BOOL _bMultiPlayer;


// prepares fog and haze parameters and eventualy converts texture
ULONG PrepareTexture( UBYTE *pubTexture, PIX pixSizeI, PIX pixSizeJ)
{
  // need to upload from RGBA format
  const PIX pixTextureSize = pixSizeI*pixSizeJ;

#if (defined __MSVC_INLINE__)
  __asm {
    mov     esi,D [pubTexture]
    mov     edi,D [pubTexture]
    mov     ecx,D [pixTextureSize]
    lea     edi,[esi+ecx]
pixLoop:
    movzx   eax,B [esi]
    or      eax,0xFFFFFF00
    bswap   eax
    mov     D [edi],eax
    add     esi,1
    add     edi,4
    dec     ecx
    jnz     pixLoop
  }

#elif (defined __GNU_INLINE_X86_32__)
  __asm__ __volatile__ (
    "movl    %[pubTexture], %%esi      \n\t"
    "movl    %[pixTextureSize], %%ecx  \n\t"
    "leal    0(%%esi, %%ecx), %%edi    \n\t"
    "0:                                \n\t" // pixLoop
    "movzbl  (%%esi), %%eax            \n\t"
    "orl     $0xFFFFFF00, %%eax        \n\t"
    "bswapl  %%eax                     \n\t"
    "movl    %%eax, (%%edi)            \n\t"
    "addl    $1, %%esi                 \n\t"
    "addl    $4, %%edi                 \n\t"
    "decl    %%ecx                     \n\t"
    "jnz     0b                        \n\t" // pixLoop
        : // no outputs.
        : [pubTexture] "g" (pubTexture),
          [pixTextureSize] "g" (pixTextureSize)
        : "eax", "ecx", "esi", "edi", "cc", "memory"
  );

#else
   const UBYTE* src = pubTexture;
   DWORD* dst = (DWORD*)(pubTexture+pixTextureSize);
   for (int i=0; i<pixTextureSize; i++) {
    const DWORD tmp = ((DWORD)*src) | 0xFFFFFF00;
    *dst = BYTESWAP32_unsigned((ULONG)tmp);
    src++;
    dst++;
   }

#endif

  // determine internal format
  extern INDEX gap_bAllowGrayTextures;
  extern INDEX tex_bFineFog;
  if( gap_bAllowGrayTextures) return TS.ts_tfLA8;
  if( tex_bFineFog) return TS.ts_tfRGBA8;
  return TS.ts_tfRGBA4;
}



// start fog with given parameters
void StartFog( CFogParameters &fp, const FLOAT3D &vViewPosAbs, const FLOATmatrix3D &mAbsToView)
{
  ASSERT( !_fog_bActive);
  if( _bMultiPlayer) gfx_bRenderFog = 1;
  if( !gfx_bRenderFog) return;
  _fog_bActive = TRUE;

  _fog_fp = fp;
  _fog_vHDirAbs = -_fog_fp.fp_vFogDir;
  _fog_vViewPosAbs = vViewPosAbs;
  _fog_vViewDirAbs(1) = -mAbsToView(3, 1);
  _fog_vViewDirAbs(2) = -mAbsToView(3, 2);
  _fog_vViewDirAbs(3) = -mAbsToView(3, 3);
  _fog_fViewerH = _fog_vViewPosAbs%-_fog_fp.fp_vFogDir;
  _fog_vHDirView = _fog_vHDirAbs*mAbsToView;
  // calculate fog mapping factors
  _fog_fMulZ = 1/(_fog_fp.fp_fFar);
  _fog_fMulH = 1/(_fog_fp.fp_fH3-_fog_fp.fp_fH0);
  _fog_fAddH = _fog_fp.fp_fH3+_fog_fViewerH;

  // calculate fog table size wanted
  extern INDEX tex_iFogSize;
  tex_iFogSize = Clamp( tex_iFogSize, (INDEX)4, (INDEX)8);
  PIX pixSizeH = ClampUp( _fog_fp.fp_iSizeH, ((INDEX)1)<<tex_iFogSize);
  PIX pixSizeL = ClampUp( _fog_fp.fp_iSizeL, ((INDEX)1)<<tex_iFogSize);
  BOOL bNoDiscard = TRUE;

  // if fog table is not allocated in right size
  if( (_fog_pixSizeH!=pixSizeH || _fog_pixSizeL!=pixSizeL) && _fog_pubTable!=NULL) {
    FreeMemory( _fog_pubTable); // free it
    _fog_pubTable = NULL;
  }
  // allocate table if needed
  if( _fog_pubTable==NULL) {
    // allocate byte table (for intensity values) and ULONG table (color values for uploading) right behind!
    _fog_pubTable = (UBYTE*)AllocMemory( pixSizeH*pixSizeL * (sizeof(UBYTE)+sizeof(ULONG)));
    _fog_pixSizeH = pixSizeH;
    _fog_pixSizeL = pixSizeL;
    _fog_tpLocal.Clear();
    bNoDiscard = FALSE;
  } 

  // update fog alpha value
  _fog_ulAlpha = (_fog_fp.fp_colColor&CT_AMASK)>>CT_ASHIFT;

  // get parameters
  const FLOAT fH0  = _fog_fp.fp_fH0;   // lowest point in LUT    ->texture t=1
  const FLOAT fH1  = _fog_fp.fp_fH1;   // bottom of fog in LUT
  const FLOAT fH2  = _fog_fp.fp_fH2;   // top of fog in LUT   
  const FLOAT fH3  = _fog_fp.fp_fH3;   // highest point in LUT   ->texture t=0
  const FLOAT fFar = _fog_fp.fp_fFar;  // farthest point in LUT  ->texture s=1
  const FLOAT fDensity = _fog_fp.fp_fDensity;
  const AttenuationType at = _fog_fp.fp_atType;
  const FogGraduationType fgt = _fog_fp.fp_fgtType;
  const FLOAT fHFogSize = fH2-fH1;
  const FLOAT fHV = -_fog_fViewerH;
  const FLOAT fEpsilon = 0.001f;
  ASSERT( fHFogSize>0);

  // for each row (height in fog)
  for( PIX pixH=0; pixH<pixSizeH; pixH++)
  {
    // get fog height of the point from row coordinate in texture
    FLOAT fHP = fH3+FLOAT(pixH)/pixSizeH*(fH0-fH3);
    // sort viewer and point and get A (lower) and B (higher) fog coord
    // making sure that they are never same
    FLOAT fHA, fHB;
    if (fHP<fHV-fEpsilon) {
      fHA=fHP;
      fHB=fHV;
    } else if (fHP>fHV+fEpsilon) {
      fHA=fHV;
      fHB=fHP;
    } else {
      fHA=fHV-fEpsilon;
      fHB=fHP+fEpsilon;
    }

    // get distance between the two points in height axis
    FLOAT fDH = fHB-fHA;
    FLOAT fOoDH = 1/fDH;
    // calculate relative part of height that goes through the fog
    FLOAT fA2 = (fH2-fHA)*fOoDH;
    fA2 = Clamp(fA2,0.0f,1.0f);
    FLOAT fA1 = (fH1-fHA)*fOoDH;
    fA1 = Clamp(fA1,0.0f,1.0f);
    FLOAT fA = fA2-fA1;
    fA = Clamp(fA,0.0f,1.0f);
    
    // if not constant graduation
    if( fgt!=FGT_CONSTANT) {
      // calculate fog height for two points, limited to be inside fog
      FLOAT fFH0 = (fHFogSize-Clamp(fHA-fH1, 0.0f, fHFogSize));
      FLOAT fFH1 = (fHFogSize-Clamp(fHB-fH1, 0.0f, fHFogSize));

      // multiply the heights by graduation factor
      fFH0 *= _fog_fp.fp_fGraduation;
      fFH1 *= _fog_fp.fp_fGraduation;

      FLOAT fDens;
      // if linear graduation
      if (fgt==FGT_LINEAR) {
        // get linear integrated density factor
        fDens = (fFH0+fFH1)/2.0f;
      // if exponential graduation
      } else {
        ASSERT(fgt==FGT_EXP);
        // sort the two heights and make sure they are not same
        FLOAT fFA, fFB;
        if (fFH0<fFH1-fEpsilon) {
          fFA=fFH0;
          fFB=fFH1;
        } else if (fFH0>fFH1+fEpsilon) {
          fFA=fFH1;
          fFB=fFH0;
        } else {
          fFA=fFH1-fEpsilon;
          fFB=fFH0+fEpsilon;
        }
        // calculate exponential integrated density factor normally
        fDens = 1.0f+(exp(-fFB)-exp(-fFA))/(fFB-fFA);
      }

      // limit the intergrated density factor
      fDens = Clamp(fDens, 0.0f, 1.0f);

      // relative size multiplied by integrated density factor gives total fog sum
      fA *= fDens;
    }

    // do per-row loop
    switch(at)
    {
    // linear fog
    case AT_LINEAR: {
      // calculate linear step for the fog parameter
      FLOAT fT = 0.0f;
      FLOAT fTStep = 1.0f/pixSizeL *fFar*fDensity*fA *255;
      // fog is just clamped fog parameter in each pixel
      for( INDEX pixL=0; pixL<pixSizeL; pixL++) {
        _fog_pubTable[pixH*pixSizeL+pixL] = Clamp( FloatToInt(fT), (SLONG)0, (SLONG)255);
        fT += fTStep;
      } 
    } break;
    // exp fog
    case AT_EXP: {
      // calculate linear step for the fog parameter
      //FLOAT fT = 0.0f;
      FLOAT fTStep = 1.0f/pixSizeL*fFar*fDensity*fA;
      // fog is exp(-t) function of fog parameter, now calculate
      // step (actually multiplication) for the fog
      FLOAT fExp = 255.0f;
      FLOAT fExpMul = exp(-fTStep);
      for( INDEX pixL=0; pixL<pixSizeL; pixL++) {
        _fog_pubTable[pixH*pixSizeL+pixL] = 255-FloatToInt(fExp);
        fExp *= fExpMul;
      } 
    } break;
    case AT_EXP2: {
      // calculate linear step for the fog parameter
      //FLOAT fT = 0.0f;
      FLOAT fTStep = 1.0f/pixSizeL*fFar*fDensity*fA;
      // fog is exp(-t^2) function of fog parameter, now calculate
      // first and second order step (actually multiplication) for the fog
      FLOAT fExp2 = 255.0f;
      FLOAT fExp2Mul = exp(-fTStep*fTStep);
      FLOAT fExp2MulMul = exp(-2*fTStep*fTStep);
      for( INDEX pixL=0; pixL<pixSizeL; pixL++) {
        _fog_pubTable[pixH*pixSizeL+pixL] = 255-FloatToInt(fExp2);
        fExp2    *= fExp2Mul;
        fExp2Mul *= fExp2MulMul;
      } 
    } break;
    }
  }

  // determine where fog starts and ends
  _fog_fStart = LowerLimit(0.0f);
  _fog_fEnd   = UpperLimit(0.0f);

  INDEX pix;
  if( _fog_pubTable[pixSizeL-1]) {
    // going from bottom
    for( pix=pixSizeH-1; pix>0; pix--) {
      if( (_fog_pubTable[(pix+1)*pixSizeL-1]*_fog_ulAlpha)>>8) break;
    }
    if( pix<(pixSizeH-1)) _fog_fEnd = (FLOAT)(pix+1) / (FLOAT)(pixSizeH-1);
  } else {
    // going from top
    for( pix=0; pix<pixSizeH; pix++) {
      if( (_fog_pubTable[(pix+1)*pixSizeL-1]*_fog_ulAlpha)>>8) break;
    }
    if( pix>0) _fog_fStart = (FLOAT)(pix-1) / (FLOAT)(pixSizeH-1);
  }

  // prepare and upload the fog table
  _fog_tpLocal.tp_bSingleMipmap = TRUE;
  const ULONG ulFormat = PrepareTexture( _fog_pubTable, _fog_pixSizeL, _fog_pixSizeH);
  if( _fog_ulFormat!=ulFormat) {
    _fog_ulFormat = ulFormat;
    bNoDiscard = FALSE;
  } // set'n'upload
  gfxSetTextureWrapping( GFX_CLAMP, GFX_CLAMP);
  gfxSetTexture( _fog_ulTexture, _fog_tpLocal);
  gfxUploadTexture( (ULONG*)(_fog_pubTable + _fog_pixSizeL*_fog_pixSizeH),
                   _fog_pixSizeL, _fog_pixSizeH, ulFormat, bNoDiscard);
}


// stop fog
void StopFog(void)
{
  _fog_bActive = FALSE;
}


// current haze parameters
BOOL _haze_bActive = FALSE;
CHazeParameters _haze_hp;
CTexParams _haze_tpLocal;

PIX _haze_pixSize=0;
FLOAT _haze_fStart=0;  // where in depth haze starts
UBYTE *_haze_pubTable=NULL;
FLOAT3D _haze_vViewPosAbs;
FLOAT3D _haze_vViewDirAbs;
FLOAT _haze_fMul=0;
FLOAT _haze_fAdd=0;
ULONG _haze_ulAlpha=0;
ULONG _haze_ulTexture=0;
ULONG _haze_ulFormat=0;


// start haze with given parameters
void StartHaze( CHazeParameters &hp,
                const FLOAT3D &vViewPosAbs, const FLOATmatrix3D &mAbsToView)
{
  ASSERT( !_haze_bActive);
  if( _bMultiPlayer) gfx_bRenderFog = 1;
  if( !gfx_bRenderFog) return;
  _haze_bActive = TRUE;

  _haze_hp = hp;
  _haze_vViewPosAbs = vViewPosAbs;
  _haze_vViewDirAbs(1) = -mAbsToView(3, 1);
  _haze_vViewDirAbs(2) = -mAbsToView(3, 2);
  _haze_vViewDirAbs(3) = -mAbsToView(3, 3);

  // calculate haze mapping factors
  _haze_fMul = 1/(_haze_hp.hp_fFar-_haze_hp.hp_fNear);
  _haze_fAdd = -_haze_hp.hp_fNear;

  PIX pixSize = _haze_hp.hp_iSize;
  BOOL bNoDiscard = TRUE;

  // if haze table is not allocated in right size
  if( _haze_pixSize!=pixSize && _haze_pubTable!=NULL) {
    FreeMemory( _haze_pubTable);  // free it
    _haze_pubTable = NULL;
  }
  // allocate table if needed
  if( _haze_pubTable==NULL) {
    // allocate byte table (for intensity values) and ULONG table (color values for uploading) right behind!
    _haze_pubTable = (UBYTE*)AllocMemory(pixSize *(sizeof(UBYTE)+sizeof(ULONG)));
    _haze_pixSize  = pixSize;
    _haze_tpLocal.Clear();
    bNoDiscard = FALSE;
  }

  // update fog alpha value
  _haze_ulAlpha = (_haze_hp.hp_colColor&CT_AMASK)>>CT_ASHIFT;

  // get parameters
  FLOAT fNear = _haze_hp.hp_fNear;
  FLOAT fFar  = _haze_hp.hp_fFar;
  FLOAT fDensity = _haze_hp.hp_fDensity;
  AttenuationType at = _haze_hp.hp_atType;
  // generate table
  INDEX pix;
  for( pix=0; pix<pixSize; pix++) {
    FLOAT fD = FLOAT(pix)/pixSize*(fFar-fNear);
    FLOAT fT = fDensity*fD;
    FLOAT fHaze=0.0f;
    switch(at) {
    case AT_LINEAR: fHaze = Clamp(fT,0.0f,1.0f); break;
    case AT_EXP:    fHaze = 1-exp(-fT);          break;
    case AT_EXP2:   fHaze = 1-exp(-fT*fT);       break;
    }
    const UBYTE ubValue = NormFloatToByte(fHaze);
    _haze_pubTable[pix] = ubValue;
  }

  // determine where haze starts
  for( pix=1; pix<pixSize; pix++) if( (_haze_pubTable[pix]*_haze_ulAlpha)>>8) break;
  _haze_fStart = (FLOAT)(pix-1) / (FLOAT)(pixSize-1);

  // prepare haze table
  _haze_tpLocal.tp_bSingleMipmap = TRUE;
  const ULONG ulFormat = PrepareTexture( _haze_pubTable, _haze_pixSize, 1);
  if( _haze_ulFormat!=ulFormat) {
    _haze_ulFormat = ulFormat;
    bNoDiscard = FALSE;
  } // set'n'upload
  gfxSetTextureWrapping( GFX_CLAMP, GFX_CLAMP);
  gfxSetTexture( _haze_ulTexture, _haze_tpLocal);
  gfxUploadTexture( (ULONG*)(_haze_pubTable + _haze_pixSize*1), _haze_pixSize, 1, ulFormat, bNoDiscard);
}


// stop haze
void StopHaze(void)
{
  _haze_bActive = FALSE;
}
