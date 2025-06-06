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

#include <Engine/Graphics/Texture.h>
#include <Engine/Graphics/TextureEffects.h>

#include <Engine/Math/Functions.h>
#include <Engine/Base/Timer.h>
#include <Engine/Base/Statistics_Internal.h>
#include <Engine/Templates/DynamicArray.cpp>
#include <Engine/Templates/Stock_CTextureData.h>
#include <Engine/Templates/StaticArray.cpp>

// asm shortcuts
#define O offset
#define Q qword ptr
#define D dword ptr
#define W  word ptr
#define B  byte ptr

#if (defined __MSVC_INLINE__)
#define ASMOPT 1
#elif (defined __GNU_INLINE_X86_32__)
#define ASMOPT 1
#else
#define ASMOPT 0
#endif

__int64 mmBaseWidthShift=0;
__int64 mmBaseWidth=0;
__int64 mmBaseWidthMask=0;
__int64 mmBaseHeightMask=0;
__int64 mmBaseMasks=0;
__int64 mmShift=0;

#if (defined __GNUC__)
/*
 * If these are "const" vars, they get optimized to hardcoded values when gcc
 *  builds with optimization, which means the linker can't resolve the
 *  references to them in the inline ASM. That's obnoxious.
 */
__int64 mm1LO   = 0x0000000000000001ll;
__int64 mm1HI   = 0x0000000100000000ll;
__int64 mm1HILO = 0x0000000100000001ll;
__int64 mm0001  = 0x0000000000000001ll;
__int64 mm0010  = 0x0000000000010000ll;
__int64 mm00M0  = 0x00000000FFFF0000ll;

static void *force_syms_to_exist = NULL;
void asm_force_mm1LO() { force_syms_to_exist = &mm1LO; }
void asm_force_mm1HI() { force_syms_to_exist = &mm1HI; }
void asm_force_mm1HILO() { force_syms_to_exist = &mm1HILO; }
void asm_force_mm0001() { force_syms_to_exist = &mm0001; }
void asm_force_mm0010() { force_syms_to_exist = &mm0010; }
void asm_force_mm00M0() { force_syms_to_exist = &mm00M0; }
void asm_force_mmBaseWidthShift() { force_syms_to_exist = &mmBaseWidthShift; }
void asm_force_mmBaseWidth() { force_syms_to_exist = &mmBaseWidth; }
void asm_force_mmBaseWidthMask() { force_syms_to_exist = &mmBaseWidthMask; }
void asm_force_mmBaseHeightMask() { force_syms_to_exist = &mmBaseHeightMask; }
void asm_force_mmBaseMasks() { force_syms_to_exist = &mmBaseMasks; }
void asm_force_mmShift() { force_syms_to_exist = &mmShift; }

#else
const __int64 mm1LO   = 0x0000000000000001;
const __int64 mm1HI   = 0x0000000100000000;
const __int64 mm1HILO = 0x0000000100000001;
const __int64 mm0001  = 0x0000000000000001;
const __int64 mm0010  = 0x0000000000010000;
const __int64 mm00M0  = 0x00000000FFFF0000;
#endif


// speed table
SBYTE asbMod3Sub1Table[256];
static BOOL  bTableSet = FALSE;

static CTextureData *_ptdEffect, *_ptdBase;
static ULONG _ulBufferMask;
static INDEX _iWantedMipLevel;
static UBYTE *_pubDrawBuffer;
static SWORD *_pswDrawBuffer;

PIX _pixTexWidth,    _pixTexHeight;
PIX _pixBufferWidth, _pixBufferHeight;


// randomizer
ULONG ulRNDSeed;

inline void Randomize( ULONG ulSeed)
{
  if( ulSeed==0) ulSeed = 0x87654321;
  ulRNDSeed = ulSeed*262147;
};

inline ULONG Rnd(void)
{
	ulRNDSeed = ulRNDSeed*262147;
  return ulRNDSeed;
};

#define RNDW (Rnd()>>16)



// Initialize the texture effect source.
void CTextureEffectSource::Initialize( class CTextureEffectGlobal *ptegGlobalEffect,
                                       ULONG ulEffectSourceType, PIX pixU0, PIX pixV0,
                                       PIX pixU1, PIX pixV1)
{ // remember global effect for cross linking
  tes_ptegGlobalEffect = ptegGlobalEffect;
  tes_ulEffectSourceType = ulEffectSourceType;

  // obtain effect source table for current effect class
  struct TextureEffectSourceType *patestSourceEffectTypes =
    _ategtTextureEffectGlobalPresets[ ptegGlobalEffect->teg_ulEffectType].tet_atestEffectSourceTypes;

  // init for animating
  patestSourceEffectTypes[ulEffectSourceType].test_Initialize(this, pixU0, pixV0, pixU1, pixV1);
}

// Animate the texture effect source.
void CTextureEffectSource::Animate(void)
{
  // obtain effect source table for current effect class
  struct TextureEffectSourceType *patestSourceEffectTypes =
    _ategtTextureEffectGlobalPresets[ tes_ptegGlobalEffect->teg_ulEffectType]
    .tet_atestEffectSourceTypes;

  // animating it
  patestSourceEffectTypes[tes_ulEffectSourceType].test_Animate(this);
}


// ----------------------------------------
//            SLONG WATER
// ----------------------------------------
inline void PutPixelSLONG_WATER( PIX pixU, PIX pixV, INDEX iHeight)
{
  _pswDrawBuffer[(pixV*_pixBufferWidth+pixU)&_ulBufferMask] += iHeight;
}

inline void PutPixel9SLONG_WATER( PIX pixU, PIX pixV, INDEX iHeightMid)
{
  INDEX iHeightSide = (iHeightMid*28053) >>16;  // iHeight /0.851120 *0.364326;
  INDEX iHeightDiag = (iHeightMid*12008) >>16;  // iHeight /0.851120 *0.155951;

  PutPixelSLONG_WATER( pixU-1, pixV-1, iHeightDiag);
  PutPixelSLONG_WATER( pixU,   pixV-1, iHeightSide);
  PutPixelSLONG_WATER( pixU+1, pixV-1, iHeightDiag);

  PutPixelSLONG_WATER( pixU-1, pixV,   iHeightSide);
  PutPixelSLONG_WATER( pixU,   pixV,   iHeightMid);
  PutPixelSLONG_WATER( pixU+1, pixV,   iHeightSide);

  PutPixelSLONG_WATER( pixU-1, pixV+1, iHeightDiag);
  PutPixelSLONG_WATER( pixU,   pixV+1, iHeightSide);
  PutPixelSLONG_WATER( pixU+1, pixV+1, iHeightDiag);
}


// ----------------------------------------
//            UBYTE FIRE
// ----------------------------------------
inline void PutPixelUBYTE_FIRE( PIX pixU, PIX pixV, INDEX iHeight)
{
  PIX pixLoc = (pixV*_pixBufferWidth+pixU) & _ulBufferMask;
  _pubDrawBuffer[pixLoc] = Clamp( _pubDrawBuffer[pixLoc] +iHeight, (INDEX)0, (INDEX)255);
}

inline void PutPixel9UBYTE_FIRE( PIX pixU, PIX pixV, INDEX iHeightMid)
{
  INDEX iHeightSide = (iHeightMid*28053) >>16;  // iHeight /0.851120 *0.364326;
  INDEX iHeightDiag = (iHeightMid*12008) >>16;  // iHeight /0.851120 *0.155951;

  PutPixelUBYTE_FIRE( pixU-1, pixV-1, iHeightDiag);
  PutPixelUBYTE_FIRE( pixU,   pixV-1, iHeightSide);
  PutPixelUBYTE_FIRE( pixU+1, pixV-1, iHeightDiag);

  PutPixelUBYTE_FIRE( pixU-1, pixV,   iHeightSide);
  PutPixelUBYTE_FIRE( pixU,   pixV,   iHeightMid);
  PutPixelUBYTE_FIRE( pixU+1, pixV,   iHeightSide);

  PutPixelUBYTE_FIRE( pixU-1, pixV+1, iHeightDiag);
  PutPixelUBYTE_FIRE( pixU,   pixV+1, iHeightSide);
  PutPixelUBYTE_FIRE( pixU+1, pixV+1, iHeightDiag);
}

inline void PutPixel25UBYTE_FIRE( PIX pixU, PIX pixV, INDEX iHeightMid)
{
  INDEX iHeightSide = (iHeightMid*28053) >>16;  // iHeight /0.851120 *0.364326;
  INDEX iHeightDiag = (iHeightMid*12008) >>16;  // iHeight /0.851120 *0.155951;

  PutPixelUBYTE_FIRE( pixU-2, pixV-2, iHeightDiag);
  PutPixelUBYTE_FIRE( pixU-1, pixV-2, iHeightSide);
  PutPixelUBYTE_FIRE( pixU,   pixV-2, iHeightSide);
  PutPixelUBYTE_FIRE( pixU+1, pixV-2, iHeightSide);
  PutPixelUBYTE_FIRE( pixU+2, pixV-2, iHeightDiag);

  PutPixelUBYTE_FIRE( pixU-2, pixV-1, iHeightSide);
  PutPixelUBYTE_FIRE( pixU-1, pixV-1, iHeightSide);
  PutPixelUBYTE_FIRE( pixU,   pixV-1, iHeightMid);
  PutPixelUBYTE_FIRE( pixU+1, pixV-1, iHeightSide);
  PutPixelUBYTE_FIRE( pixU+2, pixV-1, iHeightSide);

  PutPixelUBYTE_FIRE( pixU-2, pixV,   iHeightSide);
  PutPixelUBYTE_FIRE( pixU-1, pixV,   iHeightMid);
  PutPixelUBYTE_FIRE( pixU,   pixV,   iHeightMid);
  PutPixelUBYTE_FIRE( pixU+1, pixV,   iHeightMid);
  PutPixelUBYTE_FIRE( pixU+2, pixV,   iHeightSide);

  PutPixelUBYTE_FIRE( pixU-2, pixV+1, iHeightSide);
  PutPixelUBYTE_FIRE( pixU-1, pixV+1, iHeightSide);
  PutPixelUBYTE_FIRE( pixU,   pixV+1, iHeightMid);
  PutPixelUBYTE_FIRE( pixU+1, pixV+1, iHeightSide);
  PutPixelUBYTE_FIRE( pixU+2, pixV+1, iHeightSide);

  PutPixelUBYTE_FIRE( pixU+2, pixV+2, iHeightDiag);
  PutPixelUBYTE_FIRE( pixU-1, pixV+2, iHeightSide);
  PutPixelUBYTE_FIRE( pixU,   pixV+2, iHeightSide);
  PutPixelUBYTE_FIRE( pixU+1, pixV+2, iHeightSide);
  PutPixelUBYTE_FIRE( pixU-2, pixV+2, iHeightDiag);
}


/////////////////////////////////////////////////////////////////////
//                        WATER EFFECTS
/////////////////////////////////////////////////////////////////////


// WARNING: Changing this value will BREAK the inline asm on
//  GNU-based platforms (Linux, etc.) YOU HAVE BEEN WARNED.
#define DISTORTION 3 //3


///////////////// random surfer
struct Surfer {
  FLOAT fU;
  FLOAT fV;
  FLOAT fAngle;
};

void InitializeRandomSurfer(CTextureEffectSource *ptes,
    PIX pixU0, PIX pixV0, PIX pixU1, PIX pixV1)
{
  Surfer &sf =
    (*((Surfer *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  sf.fU = pixU0;
  sf.fV = pixV0;
  sf.fAngle = RNDW&7;
}

void AnimateRandomSurfer(CTextureEffectSource *ptes)
{
  Surfer &sf =
    (*((Surfer *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));

  PutPixel9SLONG_WATER(sf.fU, sf.fV, 125);
  sf.fU += 2*sin(sf.fAngle);
  sf.fV += 2*cos(sf.fAngle);
  PutPixel9SLONG_WATER(sf.fU, sf.fV, 250);

  if((RNDW&15)==0) {
    sf.fAngle += 3.14f/7.0f;
  }
  if((RNDW&15)==0) {
    sf.fAngle -= 3.14f/5.0f;
  }
}

///////////////// raindrops
struct Raindrop {
  UBYTE pixU;
  UBYTE pixV;
  SWORD iHeight;
  SWORD iIndex;
};


void InitializeRaindrops(CTextureEffectSource *ptes,
    PIX pixU0, PIX pixV0, PIX pixU1, PIX pixV1, int iHeight)
{
  for (int iIndex=0; iIndex<5; iIndex++) {
    Raindrop &rd =
      ((Raindrop&) ptes->tes_tespEffectSourceProperties.tesp_achDummy[iIndex*sizeof(Raindrop)]);
    rd.pixU = RNDW&(_pixBufferWidth -1);  
    rd.pixV = RNDW&(_pixBufferHeight-1); 
    rd.iHeight = RNDW&iHeight;
    rd.iIndex = iIndex*8;
  }
}
void InitializeRaindropsStandard(CTextureEffectSource *ptes,
    PIX pixU0, PIX pixV0, PIX pixU1, PIX pixV1) {
  InitializeRaindrops(ptes, pixU0, pixV0, pixU1, pixV1, 255);
}
void InitializeRaindropsBig(CTextureEffectSource *ptes,
    PIX pixU0, PIX pixV0, PIX pixU1, PIX pixV1) {
  InitializeRaindrops(ptes, pixU0, pixV0, pixU1, pixV1, 1023);
}
void InitializeRaindropsSmall(CTextureEffectSource *ptes,
    PIX pixU0, PIX pixV0, PIX pixU1, PIX pixV1) {
  InitializeRaindrops(ptes, pixU0, pixV0, pixU1, pixV1, 31);
}


void AnimateRaindrops(CTextureEffectSource *ptes, int iHeight)
{
  for (int iIndex=0; iIndex<5; iIndex++) {
    Raindrop &rd =
      ((Raindrop&) ptes->tes_tespEffectSourceProperties.tesp_achDummy[iIndex*sizeof(Raindrop)]);
    if (rd.iIndex < 48) {
      rd.iIndex++;

      if (rd.iIndex < 8) {
        PutPixel9SLONG_WATER(rd.pixU, rd.pixV, sin(rd.iIndex/4.0f*(-3.14f))*rd.iHeight);
      }
    } else {
      rd.pixU = RNDW&(_pixBufferWidth -1);  
      rd.pixV = RNDW&(_pixBufferHeight-1); 
      rd.iHeight = RNDW&iHeight;
      rd.iIndex = 0;
    }
  }
}
void AnimateRaindropsStandard(CTextureEffectSource *ptes) {
  AnimateRaindrops(ptes, 255);
}
void AnimateRaindropsBig(CTextureEffectSource *ptes) {
  AnimateRaindrops(ptes, 1023);
}
void AnimateRaindropsSmall(CTextureEffectSource *ptes) {
  AnimateRaindrops(ptes, 31);
}



///////////////// oscilator
struct Oscilator {
  UBYTE pixU;
  UBYTE pixV;
  FLOAT fAngle;
};

void InitializeOscilator(CTextureEffectSource *ptes,
    PIX pixU0, PIX pixV0, PIX pixU1, PIX pixV1)
{
  Oscilator &os =
    (*((Oscilator *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  os.pixU = pixU0;
  os.pixV = pixV0;
  os.fAngle = -3.14f;
}

void AnimateOscilator(CTextureEffectSource *ptes)
{
  Oscilator &os =
    (*((Oscilator *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  PutPixel9SLONG_WATER(os.pixU, os.pixV, sin(os.fAngle)*150);
  os.fAngle += (3.14f/6);
}


///////////////// Vertical Line
struct VertLine{
  UBYTE pixU;
  UBYTE pixV;
  UWORD uwSize;
  FLOAT fAngle;
};

void InitializeVertLine(CTextureEffectSource *ptes,
    PIX pixU0, PIX pixV0, PIX pixU1, PIX pixV1)
{
  VertLine &vl =
    (*((VertLine *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  vl.pixU = pixU0;
  vl.pixV = pixV0;
  vl.fAngle = -3.14f;
  if (pixV0==pixV1) {
    vl.uwSize = 16;
  } else {
    vl.uwSize = abs(pixV1-pixV0);
  }
}

void AnimateVertLine(CTextureEffectSource *ptes)
{
  VertLine &vl =
    (*((VertLine *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  PIX pixV = vl.pixV;
  for (int iCnt=0; iCnt<vl.uwSize; iCnt++) {
    PutPixelSLONG_WATER(vl.pixU, pixV, (sin(vl.fAngle)*25));
    pixV = (pixV+1)&(_pixBufferHeight-1);
  }
  vl.fAngle += (3.14f/6);
}


///////////////// Horizontal Line
struct HortLine{
  UBYTE pixU;
  UBYTE pixV;
  UWORD uwSize;
  FLOAT fAngle;
};

void InitializeHortLine(CTextureEffectSource *ptes,
    PIX pixU0, PIX pixV0, PIX pixU1, PIX pixV1)
{
  HortLine &hl =
    (*((HortLine *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  hl.pixU = pixU0;
  hl.pixV = pixV0;
  hl.fAngle = -3.14f;
  if (pixU0==pixU1) {
    hl.uwSize = 16;
  } else {
    hl.uwSize = abs(pixU1-pixU0);
  }
}

void AnimateHortLine(CTextureEffectSource *ptes)
{
  HortLine &hl =
    (*((HortLine *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  PIX pixU = hl.pixU;
  for (int iCnt=0; iCnt<hl.uwSize; iCnt++) {
    PutPixelSLONG_WATER(pixU, hl.pixV, (sin(hl.fAngle)*25));
    pixU = (pixU+1)&(_pixBufferWidth-1);
  }
  hl.fAngle += (3.14f/6);
}


/////////////////////////////////////////////////////////////////////
//                        FIRE EFFECTS
/////////////////////////////////////////////////////////////////////


///////////////// Fire Point
struct FirePoint{
  UBYTE pixU;
  UBYTE pixV;
};

void InitializeFirePoint(CTextureEffectSource *ptes,
    PIX pixU0, PIX pixV0, PIX pixU1, PIX pixV1)
{
  FirePoint &ft =
    (*((FirePoint *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  ft.pixU = pixU0;
  ft.pixV = pixV0;
}

void AnimateFirePoint(CTextureEffectSource *ptes)
{
  FirePoint &ft =
    (*((FirePoint *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  PutPixel9UBYTE_FIRE(ft.pixU, ft.pixV, 255);
}

void InitializeRandomFirePoint(CTextureEffectSource *ptes,
    PIX pixU0, PIX pixV0, PIX pixU1, PIX pixV1)
{
  FirePoint &ft =
    (*((FirePoint *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  ft.pixU = pixU0;
  ft.pixV = pixV0;
}

void AnimateRandomFirePoint(CTextureEffectSource *ptes)
{
  FirePoint &ft =
    (*((FirePoint *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  PutPixel9UBYTE_FIRE(ft.pixU, ft.pixV, RNDW&255);
}

void InitializeFireShakePoint(CTextureEffectSource *ptes,
    PIX pixU0, PIX pixV0, PIX pixU1, PIX pixV1)
{
  FirePoint &ft =
    (*((FirePoint *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  ft.pixU = pixU0;
  ft.pixV = pixV0;
}

void AnimateFireShakePoint(CTextureEffectSource *ptes)
{
  FirePoint &ft =
    (*((FirePoint *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  UBYTE pixU, pixV;
  pixU = RNDW%3 - 1;
  pixV = RNDW%3 - 1;
  PutPixel9UBYTE_FIRE(ft.pixU+pixU, ft.pixV+pixV, 255);
}


///////////////// Fire Place
#define FIREPLACE_SIZE 60

struct FirePlace{
  UBYTE pixU;
  UBYTE pixV;
  UBYTE ubWidth;
  UBYTE aubFire[FIREPLACE_SIZE];
};

void InitializeFirePlace(CTextureEffectSource *ptes,
    PIX pixU0, PIX pixV0, PIX pixU1, PIX pixV1)
{
  FirePlace &fp =
    (*((FirePlace *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  fp.pixU = pixU0;
  fp.pixV = pixV0;
  fp.ubWidth = abs(pixU1-pixU0);
  if (fp.ubWidth>FIREPLACE_SIZE) fp.ubWidth=FIREPLACE_SIZE;
  if (fp.ubWidth<10) fp.ubWidth = 10;
  // clear fire array
  for (int iCnt=0; iCnt<fp.ubWidth; iCnt++) {
    fp.aubFire[iCnt] = 0;
  }
}

void AnimateFirePlace(CTextureEffectSource *ptes)
{
  INDEX iIndex;
  FirePlace &fp =
    (*((FirePlace *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  ULONG ulRND = RNDW&255;
  // match
  if (ulRND>200) {
    ULONG ulMatchIndex = ulRND%(fp.ubWidth-5);
    for (iIndex=0; iIndex<5; iIndex++) {
      fp.aubFire[ulMatchIndex+iIndex] = 255;
    }
  // water
  } else if (ulRND<50) {
    for (iIndex=0; iIndex<10; iIndex++) {
      fp.aubFire[RNDW%fp.ubWidth] = 0;
    }
  }
  // fix fire place
  for (iIndex=0; iIndex<fp.ubWidth; iIndex++) {
    UBYTE ubFlame = fp.aubFire[iIndex];
    // flame is fading ?
    if (ubFlame < 50) {
      // starting to burn
      if (ubFlame > 10) {
        ubFlame += RNDW%30;    //30
      // give more fire
      } else {
        ubFlame += RNDW%30+30; //30,30
      }
    }
    fp.aubFire[iIndex] = ubFlame;
  }
  // water on edges
  for (iIndex=0; iIndex<4; iIndex++) {
    INDEX iWater = RNDW%4;
    fp.aubFire[iWater] = 0;
    fp.aubFire[fp.ubWidth-1-iWater] = 0;
  }
  // smooth fire place
  for (iIndex=1; iIndex<(fp.ubWidth-1); iIndex++) {
    fp.aubFire[iIndex] = (fp.aubFire[iIndex-1]+fp.aubFire[iIndex]+fp.aubFire[iIndex+1])/3;
  }
  // draw fire place in buffer
  for (iIndex=0; iIndex<fp.ubWidth; iIndex++) {
    PutPixel9UBYTE_FIRE(fp.pixU+iIndex, fp.pixV, fp.aubFire[iIndex]);
  }
}


///////////////// Fire Roler
struct FireRoler{
  UBYTE pixU;
  UBYTE pixV;
  //FLOAT fRadius;
  FLOAT fRadiusU;
  FLOAT fRadiusV;
  FLOAT fAngle;
  FLOAT fAngleAdd;
};

void InitializeFireRoler(CTextureEffectSource *ptes,
    PIX pixU0, PIX pixV0, PIX pixU1, PIX pixV1)
{
  FireRoler &fr =
    (*((FireRoler *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  fr.pixU = pixU0;
  fr.pixV = pixV0;
  if (pixU0==pixU1 && pixV0==pixV1) {
    //fr.fRadius = 3;
    fr.fRadiusU = 3;
    fr.fRadiusV = 3;
    fr.fAngleAdd = (3.14f/6);
  } else {
    //fr.fRadius = sqrt((pixU1-pixU0)*(pixU1-pixU0) + (pixV1-pixV0)*(pixV1-pixV0));
    fr.fRadiusU = pixU1-pixU0;
    fr.fRadiusV = pixV1-pixV0;
    //fr.fAngleAdd = (3.14f/((fr.fRadius)*2));
    fr.fAngleAdd = (3.14f/(Abs(fr.fRadiusU)+Abs(fr.fRadiusV)));
  }
  fr.fAngle = 0;
}

void AnimateFireRoler(CTextureEffectSource *ptes)
{
  FireRoler &fr =
    (*((FireRoler *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  PutPixel9UBYTE_FIRE( (cos(fr.fAngle)*fr.fRadiusU + fr.pixU),
                       (sin(fr.fAngle)*fr.fRadiusV + fr.pixV), 255);
  fr.fAngle += fr.fAngleAdd;
  PutPixel9UBYTE_FIRE( (cos(fr.fAngle)*fr.fRadiusU + fr.pixU),
                       (sin(fr.fAngle)*fr.fRadiusV + fr.pixV), 200);
  fr.fAngle += fr.fAngleAdd;
  PutPixel9UBYTE_FIRE( (cos(fr.fAngle)*fr.fRadiusU + fr.pixU),
                       (sin(fr.fAngle)*fr.fRadiusV + fr.pixV), 150);
  fr.fAngle += fr.fAngleAdd;
}


///////////////// Fire Fall
#define FIREFALL_POINTS 100

struct FireFall{
  UBYTE pixU;
  UBYTE pixV;
  ULONG ulWidth;
  ULONG ulPointToReinitialize;
};

struct FireFallPixel{
  UBYTE pixU;
  UBYTE pixV;
  UBYTE ubSpeed;
};

void InitializeFireFall(CTextureEffectSource *ptes,
    PIX pixU0, PIX pixV0, PIX pixU1, PIX pixV1)
{
  FireFall &ff =
    (*((FireFall *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  ff.pixU = pixU0;
  ff.pixV = pixV0;
  if (pixU0==pixU1) {
    ff.ulWidth = 15;
  } else {
    ff.ulWidth = abs(pixU1-pixU0);
  }
  // initialize fall points
  ptes->tes_atepPixels.New(FIREFALL_POINTS);
  ff.ulPointToReinitialize = 0;
  for (INDEX iIndex=0; iIndex<FIREFALL_POINTS; iIndex++) {
    FireFallPixel &ffp = ((FireFallPixel&) ptes->tes_atepPixels[iIndex]);
    ffp.pixU = ff.pixU+(RNDW%ff.ulWidth);
    ffp.pixV = ff.pixV+(RNDW%_pixBufferHeight);
    ffp.ubSpeed = (RNDW&1)+2;
  }
}

void AnimateFireFall(CTextureEffectSource *ptes)
{
  FireFall &ff =
    (*((FireFall *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  // animate fall points
  for (INDEX iIndex=0; iIndex<FIREFALL_POINTS; iIndex++) {
    FireFallPixel &ffp = ((FireFallPixel&) ptes->tes_atepPixels[iIndex]);
    // fall from fall
    int iHeight = (RNDW&3)*64 + 40;
    if (ffp.ubSpeed == 2) {
      PutPixelUBYTE_FIRE(ffp.pixU+(RNDW%3)-1, ffp.pixV, iHeight);
      PutPixelUBYTE_FIRE(ffp.pixU+(RNDW%3)-1, ffp.pixV+1, iHeight-40);
    } else {
      PutPixelUBYTE_FIRE(ffp.pixU, ffp.pixV, iHeight);
      PutPixelUBYTE_FIRE(ffp.pixU, ffp.pixV+1, iHeight-40);
    }
    ffp.pixV+=ffp.ubSpeed;
    // when falled down reinitialize
    if (ffp.pixV >= _pixBufferHeight) {
      if (static_cast<INDEX>(ff.ulPointToReinitialize) == iIndex) {
        ff.ulPointToReinitialize++;
        if (ff.ulPointToReinitialize >= FIREFALL_POINTS) ff.ulPointToReinitialize = 0;
        ffp.pixU = ff.pixU+(RNDW%ff.ulWidth);
        ffp.pixV -= _pixBufferHeight;
        ffp.ubSpeed = (RNDW&1)+2;
      } else {
        ffp.pixV -= _pixBufferHeight;
      }
    }
  }
}


///////////////// Fire Fountain
#define FIREFOUNTAIN_POINTS 100

struct FireFountain{
  UBYTE pixU;
  UBYTE pixV;
  ULONG ulWidth;
  ULONG ulBaseHeight;
  ULONG ulRandomHeight;

};

struct FireFountainPixel{
  SWORD pixU;
  SWORD pixV;
  UBYTE pixLastU;
  UBYTE pixLastV;
  SWORD sbSpeedU;
  SWORD sbSpeedV;
};

void InitializeFireFountain(CTextureEffectSource *ptes,
    PIX pixU0, PIX pixV0, PIX pixU1, PIX pixV1)
{
  FireFountain &ff =
    (*((FireFountain *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  ff.pixU = pixU0;
  ff.pixV = pixV0;
  // fountain width
  if (pixU0==pixU1) {
    ff.ulWidth = 31;
  } else {
    ff.ulWidth = abs(pixU1-pixU0)*2;
  }
  // fountain height
  if (pixV0==pixV1) {
    ff.ulBaseHeight = 120;
    ff.ulRandomHeight = 40;
  } else {
    ff.ulBaseHeight = abs(pixV1-pixV0)*3;
    ff.ulRandomHeight = abs(pixV1-pixV0);
  }
  // initialize fountain points
  ptes->tes_atepPixels.New(FIREFOUNTAIN_POINTS*2);
  for (INDEX iIndex=0; iIndex<FIREFOUNTAIN_POINTS*2; iIndex+=2) {
    FireFountainPixel &ffp = ((FireFountainPixel&) ptes->tes_atepPixels[iIndex]);
    ffp.pixU = (ff.pixU)<<6;
    ffp.pixV = (RNDW%(_pixBufferHeight-(_pixBufferHeight>>3))+(_pixBufferHeight>>3))<<6;
    ffp.pixLastU = (ffp.pixU)>>6;
    ffp.pixLastV = (ffp.pixV)>>6;
    ffp.sbSpeedU = 0;
    ffp.sbSpeedV = 0;
  }
}

void AnimateFireFountain(CTextureEffectSource *ptes)
{
  FireFountain &ff =
    (*((FireFountain *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  // animate fountain points
  for (INDEX iIndex=0; iIndex<FIREFOUNTAIN_POINTS*2; iIndex+=2) {
    FireFountainPixel &ffp = ((FireFountainPixel&) ptes->tes_atepPixels[iIndex]);
    // fall from fountain
    PutPixelUBYTE_FIRE((ffp.pixU)>>6, (ffp.pixV)>>6, 200);
    PutPixelUBYTE_FIRE(ffp.pixLastU, ffp.pixLastV, 150);
    // move pixel
    ffp.pixLastU = (ffp.pixU)>>6;
    ffp.pixLastV = (ffp.pixV)>>6;
    ffp.pixU+=ffp.sbSpeedU;
    ffp.pixV-=ffp.sbSpeedV;
    ffp.sbSpeedV-=8;
    // when falled down reinitialize
    if ((ffp.pixV>>6) >= (_pixBufferHeight-5)) {
      ffp.pixU = (ff.pixU)<<6;
      ffp.pixV = (ff.pixV)<<6;
      ffp.pixLastU = (ffp.pixU)>>6;
      ffp.pixLastV = (ffp.pixV)>>6;
      ffp.sbSpeedU = (RNDW%ff.ulWidth)-(ff.ulWidth/2-1);
      ffp.sbSpeedV = (RNDW%ff.ulRandomHeight)+ff.ulBaseHeight;
    }
  }
}


///////////////// Fire Fountain
#define FIRESIDEFOUNTAIN_POINTS 100

struct FireSideFountain{
  UBYTE pixU;
  UBYTE pixV;
  ULONG ulBaseWidth;
  ULONG ulRandomWidth;
  ULONG ulSide;
};

struct FireSideFountainPixel{
  SWORD pixU;
  SWORD pixV;
  UBYTE pixLastU;
  UBYTE pixLastV;
  SWORD sbSpeedU;
  SWORD sbSpeedV;
};

void InitializeFireSideFountain(CTextureEffectSource *ptes,
    PIX pixU0, PIX pixV0, PIX pixU1, PIX pixV1)
{
  FireSideFountain &fsf =
    (*((FireSideFountain *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  fsf.pixU = pixU0;
  fsf.pixV = pixV0;
  // fountain width
  if (pixU0==pixU1) {
    fsf.ulBaseWidth = 80;
    fsf.ulRandomWidth = 40;
    fsf.ulSide = (pixU0>(_pixBufferWidth/2));
  } else {
    fsf.ulBaseWidth = abs(pixU1-pixU0)*2;
    fsf.ulRandomWidth = abs(pixU1-pixU0);
    fsf.ulSide = (pixU1<pixU0);
  }
  // initialize fountain points
  ptes->tes_atepPixels.New(FIRESIDEFOUNTAIN_POINTS*2);
  for (INDEX iIndex=0; iIndex<FIRESIDEFOUNTAIN_POINTS*2; iIndex+=2) {
    FireSideFountainPixel &fsfp = ((FireSideFountainPixel&) ptes->tes_atepPixels[iIndex]);
    fsfp.pixU = (fsf.pixU)<<6;
    fsfp.pixV = (RNDW%(_pixBufferHeight-(_pixBufferHeight>>3))+(_pixBufferHeight>>3))<<6;
    fsfp.pixLastU = (fsfp.pixU)>>6;
    fsfp.pixLastV = (fsfp.pixV)>>6;
    fsfp.sbSpeedU = 0;
    fsfp.sbSpeedV = 0;
  }
}

void AnimateFireSideFountain(CTextureEffectSource *ptes)
{
  FireSideFountain &fsf =
    (*((FireSideFountain *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  // animate fountain points
  for (INDEX iIndex=0; iIndex<FIRESIDEFOUNTAIN_POINTS*2; iIndex+=2) {
    FireSideFountainPixel &fsfp = ((FireSideFountainPixel&) ptes->tes_atepPixels[iIndex]);
    // fall from fountain
    PutPixelUBYTE_FIRE((fsfp.pixU)>>6, (fsfp.pixV)>>6, 200);
    PutPixelUBYTE_FIRE(fsfp.pixLastU, fsfp.pixLastV, 150);
    // move pixel
    fsfp.pixLastU = (fsfp.pixU)>>6;
    fsfp.pixLastV = (fsfp.pixV)>>6;
    fsfp.pixU+=fsfp.sbSpeedU;
    fsfp.pixV-=fsfp.sbSpeedV;
    fsfp.sbSpeedV-=8;
    // when falled down reinitialize
    if ((fsfp.pixV>>6) >= (_pixBufferHeight-5)) {
      fsfp.pixU = (fsf.pixU)<<6;
      fsfp.pixV = (fsf.pixV)<<6;
      fsfp.pixLastU = (fsfp.pixU)>>6;
      fsfp.pixLastV = (fsfp.pixV)>>6;
      fsfp.sbSpeedU = (RNDW%fsf.ulRandomWidth)+fsf.ulBaseWidth;
      if (fsf.ulSide) {
        fsfp.sbSpeedU = -fsfp.sbSpeedU;
      }
      fsfp.sbSpeedV = 0;
    }
  }
}


///////////////// Fire Lightning
struct FireLightning{
  FLOAT fpixUFrom;
  FLOAT fpixVFrom;
  FLOAT fpixUTo;
  FLOAT fpixVTo;
  FLOAT fvU;
  FLOAT fvV;
  FLOAT fvNormalU;
  FLOAT fvNormalV;
  FLOAT fDistance;
  SLONG slCnt;
};

void InitializeFireLightning(CTextureEffectSource *ptes,
    PIX pixU0, PIX pixV0, PIX pixU1, PIX pixV1)
{
  FireLightning &fl =
    (*((FireLightning *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  fl.fpixUFrom = (FLOAT) pixU0;
  fl.fpixVFrom = (FLOAT) pixV0;
  if (pixU0==pixU1 && pixV0==pixV1) {
    fl.fpixUTo = Abs((FLOAT)_pixBufferWidth -fl.fpixUFrom);
    fl.fpixVTo = Abs((FLOAT)_pixBufferHeight-fl.fpixVFrom);
  } else {
    fl.fpixUTo = (FLOAT) pixU1;
    fl.fpixVTo = (FLOAT) pixV1;
  }
  fl.fDistance = sqrt((fl.fpixUTo-fl.fpixUFrom)*(fl.fpixUTo-fl.fpixUFrom)+
                      (fl.fpixVTo-fl.fpixVFrom)*(fl.fpixVTo-fl.fpixVFrom));
  // vector
  fl.fvU = (fl.fpixUTo-fl.fpixUFrom)/fl.fDistance;
  fl.fvV = (fl.fpixVTo-fl.fpixVFrom)/fl.fDistance;
  // normal vector
  fl.fvNormalU = -fl.fvV;
  fl.fvNormalV = fl.fvU;
  // frame counter
  fl.slCnt = 2;
}

void AnimateFireLightning(CTextureEffectSource *ptes)
{
  FLOAT fU, fV, fLastU, fLastV;
  FLOAT fDU, fDV, fCnt;
  SLONG slRND;
  ULONG ulDist;

  FireLightning &fl =
    (*((FireLightning *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  // last point -> starting point
  fLastU = fl.fpixUFrom;
  fLastV = fl.fpixVFrom;

  fl.slCnt--;
  if (fl.slCnt == 0) {
    ulDist = 0;
    while ((FLOAT)ulDist<fl.fDistance) {
      // go away from source point to destination point
      ulDist += (RNDW%5)+5;
      if ((FLOAT)ulDist>=fl.fDistance) {
        // move point to line end
        fU = fl.fpixUTo;
        fV = fl.fpixVTo;
      } else {
        // move point on line
        fU = fl.fpixUFrom + fl.fvU*(FLOAT)ulDist;
        fV = fl.fpixVFrom + fl.fvV*(FLOAT)ulDist;
        // move point offset on normal line
        slRND = (SLONG) (RNDW%11)-5;
        fU += fl.fvNormalU*(FLOAT)slRND;
        fV += fl.fvNormalV*(FLOAT)slRND;
      }
      // draw line
      fDU = fU-fLastU;
      fDV = fV-fLastV;
      if (Abs(fDU)>Abs(fDV)) fCnt = Abs(fDU);
                          else fCnt = Abs(fDV);
      fDU = fDU/fCnt;
      fDV = fDV/fCnt;
      while (fCnt>0.0f) {
        PutPixelUBYTE_FIRE((PIX) fLastU, (PIX) fLastV, 255);
        fLastU += fDU;
        fLastV += fDV;
        fCnt -= 1;
      }
      // store last point
      fLastU = fU;
      fLastV = fV;
    }
    fl.slCnt = 2;
  }
}


///////////////// Fire Lightning Ball
#define FIREBALL_LIGHTNINGS 2

struct FireLightningBall{
  FLOAT fpixU;
  FLOAT fpixV;
  FLOAT fRadiusU;
  FLOAT fRadiusV;
};

void InitializeFireLightningBall(CTextureEffectSource *ptes,
    PIX pixU0, PIX pixV0, PIX pixU1, PIX pixV1)
{
  FireLightningBall &flb =
    (*((FireLightningBall *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  flb.fpixU = (FLOAT) pixU0;
  flb.fpixV = (FLOAT) pixV0;
  if (pixU0==pixU1 && pixV0==pixV1) {
    flb.fRadiusU = 20;
    flb.fRadiusV = 20;
  } else {
    flb.fRadiusU = pixU1-pixU0;
    flb.fRadiusV = pixV1-pixV0;
  }
}

void AnimateFireLightningBall(CTextureEffectSource *ptes)
{
  FLOAT fU, fV, fLastU, fLastV, fvU, fvV, fvNormalU, fvNormalV;
  FLOAT fDU, fDV, fCnt, fDistance;
  FLOAT fDestU, fDestV, fAngle;
  SLONG slRND;
  ULONG ulDist;

  FireLightningBall &flb =
    (*((FireLightningBall *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  for (int iBalls=0; iBalls<FIREBALL_LIGHTNINGS; iBalls++) {
    // last point -> starting point
    fLastU = flb.fpixU;
    fLastV = flb.fpixV;
    // destination point
    fAngle = (FLOAT) RNDW/10000;
    fDestU = flb.fpixU + flb.fRadiusU*cos(fAngle);
    fDestV = flb.fpixV + flb.fRadiusV*sin(fAngle);
    fDistance = sqrt((fDestU-fLastU)*(fDestU-fLastU)+
                     (fDestV-fLastV)*(fDestV-fLastV));
    // vector
    fvU = (fDestU-fLastU)/fDistance;
    fvV = (fDestV-fLastV)/fDistance;
    // normal vector
    fvNormalU = -fvV;
    fvNormalV = fvU;
    ulDist = 0;
    while ((FLOAT)ulDist<fDistance) {
      // go away from source point to destination point
      ulDist += (RNDW%5)+5;
      if ((FLOAT)ulDist>=fDistance) {
        // move point on line
        fU = fDestU;
        fV = fDestV;
      } else {
        // move point on line
        fU = flb.fpixU + fvU*(FLOAT)ulDist;
        fV = flb.fpixV + fvV*(FLOAT)ulDist;
        // move point offset on normal line
        slRND = (SLONG) (RNDW%11)-5;
        fU += fvNormalU*(FLOAT)slRND;
        fV += fvNormalV*(FLOAT)slRND;
      }
      // draw line
      fDU = fU-fLastU;
      fDV = fV-fLastV;
      // counter
      if (Abs(fDU)>Abs(fDV)) fCnt = Abs(fDU);
                        else fCnt = Abs(fDV);
      fDU = fDU/fCnt;
      fDV = fDV/fCnt;
      while (fCnt>0.0f) {
        PutPixelUBYTE_FIRE((PIX) fLastU, (PIX) fLastV, 255);
        fLastU += fDU;
        fLastV += fDV;
        fCnt -= 1;
      }
      // store last point
      fLastU = fU;
      fLastV = fV;
    }
  }
}


///////////////// Fire Smoke
#define SMOKE_POINTS 50

struct FireSmoke{
  FLOAT fpixU;
  FLOAT fpixV;
};

struct FireSmokePoint{
  FLOAT fpixU;
  FLOAT fpixV;
  FLOAT fSpeedV;
};

void InitializeFireSmoke(CTextureEffectSource *ptes,
    PIX pixU0, PIX pixV0, PIX pixU1, PIX pixV1)
{
  FireSmoke &fs =
    (*((FireSmoke *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  fs.fpixU = (FLOAT) pixU0;
  fs.fpixV = (FLOAT) pixV0;
  if (pixU0==pixU1 && pixV0==pixV1) {
  } else {
  }
  // initialize smoke points
  ptes->tes_atepPixels.New(SMOKE_POINTS*2);
  for (INDEX iIndex=0; iIndex<SMOKE_POINTS*2; iIndex+=2) {
    FireSmokePoint &fsp = ((FireSmokePoint&) ptes->tes_atepPixels[iIndex]);
    fsp.fpixU = FLOAT (pixU0 + (iIndex-(SMOKE_POINTS))/8);
    fsp.fpixV = FLOAT (pixV0);
    fsp.fSpeedV = 0.0f;
  }
}

void AnimateFireSmoke(CTextureEffectSource *ptes)
{
  int iHeat;
  FLOAT fRatio = 32.0f / (FLOAT)_pixBufferHeight;
  UBYTE pixU, pixV;

  FireSmoke &fs =
    (*((FireSmoke *) ptes->tes_tespEffectSourceProperties.tesp_achDummy));
  // animate smoke points
  for (INDEX iIndex=0; iIndex<SMOKE_POINTS*2; iIndex+=2) {
    FireSmokePoint &fsp = ((FireSmokePoint&) ptes->tes_atepPixels[iIndex]);
    pixU = RNDW%3 - 1;
    pixV = RNDW%3 - 1;
    if (fsp.fSpeedV<0.1f) {
      PutPixelUBYTE_FIRE((PIX) fsp.fpixU, (PIX) fsp.fpixV, RNDW%128);
    } else {
      iHeat = int(fsp.fpixV*fRatio+1);
      PutPixel25UBYTE_FIRE((PIX) fsp.fpixU+pixU, (PIX) fsp.fpixV+pixV, RNDW%iHeat);
    }
    // start moving up
    if (fsp.fSpeedV<0.1f && (RNDW&255)==0) {
      fsp.fSpeedV = 1.0f;
    }
    // move up
    fsp.fpixV -= fsp.fSpeedV;
    // at the end of texture go on bottom
    if (fsp.fpixV<=(FLOAT)_pixBufferHeight) {
      fsp.fpixV = fs.fpixV;
      fsp.fSpeedV = 0.0f;
    }
  }
}



/////////////////   Water


void InitializeWater(void)
{
  Randomize( (ULONG)(_pTimer->GetHighPrecisionTimer().GetMilliseconds()));
}


/*******************************
       Water Animation
********************************/
static void AnimateWater( SLONG slDensity)
{
  _sfStats.StartTimer(CStatForm::STI_EFFECTRENDER);

/////////////////////////////////// move water

  SWORD *pNew = (SWORD*)_ptdEffect->td_pubBuffer1;
  SWORD *pOld = (SWORD*)_ptdEffect->td_pubBuffer2;

  PIX pixV, pixU;
  PIX pixOffset, iNew;
  SLONG slLineAbove, slLineBelow, slLineLeft, slLineRight;

  // inner rectangle (without 1 pixel top and bottom line)
  pixOffset = _pixBufferWidth + 1;
  for( pixV=_pixBufferHeight-2; pixV>0; pixV--) {
    for( pixU=_pixBufferWidth; pixU>0; pixU--) {
      iNew = (( (SLONG)pOld[pixOffset - _pixBufferWidth]
              + (SLONG)pOld[pixOffset + _pixBufferWidth]
              + (SLONG)pOld[pixOffset - 1]
              + (SLONG)pOld[pixOffset + 1]
             ) >> 1)
              - (SLONG)pNew[pixOffset];
      pNew[pixOffset] =  iNew - (iNew >> slDensity);
      pixOffset++;
    }
  }

  // upper horizontal border (without corners)
  slLineAbove = ((_pixBufferHeight-1)*_pixBufferWidth) + 1;
  slLineBelow = _pixBufferWidth + 1;
  slLineLeft = 0;
  slLineRight = 2;
  pixOffset = 1;
  for( pixU=_pixBufferWidth-2; pixU>0; pixU--) {
    iNew = (( (SLONG)pOld[slLineAbove]
            + (SLONG)pOld[slLineBelow]
            + (SLONG)pOld[slLineLeft]
            + (SLONG)pOld[slLineRight]
           ) >> 1)
            - (SLONG)pNew[pixOffset];
    pNew[pixOffset] =  iNew - (iNew >> slDensity);
    slLineAbove++;
    slLineBelow++;
    slLineLeft++;
    slLineRight++;
    pixOffset++;
  }
  // lower horizontal border (without corners)
  slLineAbove = ((_pixBufferHeight-2)*_pixBufferWidth) + 1;
  slLineBelow = 1;
  slLineLeft = (_pixBufferHeight-1)*_pixBufferWidth;
  slLineRight = ((_pixBufferHeight-1)*_pixBufferWidth) + 2;
  pixOffset = ((_pixBufferHeight-1)*_pixBufferWidth) + 1;
  for( pixU=_pixBufferWidth-2; pixU>0; pixU--) {
    iNew = (( (SLONG)pOld[slLineAbove]
            + (SLONG)pOld[slLineBelow]
            + (SLONG)pOld[slLineLeft]
            + (SLONG)pOld[slLineRight]
           ) >> 1)
            - (SLONG)pNew[pixOffset];
    pNew[pixOffset] =  iNew - (iNew >> slDensity);
    slLineAbove++;
    slLineBelow++;
    slLineLeft++;
    slLineRight++;
    pixOffset++;
  }
  // corner ( 0, 0)
  iNew = (( (SLONG)pOld[_pixBufferWidth]
          + (SLONG)pOld[(_pixBufferHeight-1)*_pixBufferWidth]
          + (SLONG)pOld[1]
          + (SLONG)pOld[_pixBufferWidth-1]
         ) >> 1)
          - (SLONG)pNew[0];
  pNew[0] =  iNew - (iNew >> slDensity);
  // corner ( 0, _pixBufferWidth)
  iNew = (( (SLONG)pOld[(2*_pixBufferWidth) - 1]
          + (SLONG)pOld[(_pixBufferHeight*_pixBufferWidth) - 1]
          + (SLONG)pOld[0]
          + (SLONG)pOld[_pixBufferWidth-2]
         ) >> 1)
          - (SLONG)pNew[_pixBufferWidth-1];
  pNew[_pixBufferWidth-1] =  iNew - (iNew >> slDensity);
  // corner ( _pixBufferHeight, 0)
  iNew = (( (SLONG)pOld[0]
          + (SLONG)pOld[(_pixBufferHeight-2)*_pixBufferWidth]
          + (SLONG)pOld[((_pixBufferHeight-1)*_pixBufferWidth) + 1]
          + (SLONG)pOld[(_pixBufferHeight*_pixBufferWidth) - 1]
         ) >> 1)
          - (SLONG)pNew[(_pixBufferHeight-1)*_pixBufferWidth];
  pNew[(_pixBufferHeight-1)*_pixBufferWidth] =  iNew - (iNew >> slDensity);
  // corner ( _pixBufferHeight, _pixBufferWidth)
  iNew = (( (SLONG)pOld[_pixBufferWidth-1]
          + (SLONG)pOld[((_pixBufferHeight-1)*_pixBufferWidth) - 1]
          + (SLONG)pOld[(_pixBufferHeight-1)*_pixBufferWidth]
          + (SLONG)pOld[(_pixBufferHeight*_pixBufferWidth) - 2]
         ) >> 1)
          - (SLONG)pNew[(_pixBufferHeight*_pixBufferWidth) - 1];
  pNew[(_pixBufferHeight*_pixBufferWidth) - 1] =  iNew - (iNew >> slDensity);

  // swap buffers
  Swap( _ptdEffect->td_pubBuffer1, _ptdEffect->td_pubBuffer2);

  _sfStats.StopTimer(CStatForm::STI_EFFECTRENDER);
}



//////////////////////////// displace texture


#define PIXEL(u,v) pulTextureBase[ ((u)&(SLONG&)mmBaseWidthMask) + ((v)&(SLONG&)mmBaseHeightMask) *pixBaseWidth]

ULONG _slHeightMapStep_renderWater = 0;
PIX _pixBaseWidth_renderWater = 0;

#pragma warning(disable: 4731)
static void RenderWater(void)
{
  _sfStats.StartTimer(CStatForm::STI_EFFECTRENDER);

  // get textures' parameters
  ULONG *pulTexture     = _ptdEffect->td_pulFrames;
  PIX pixBaseWidth      = _ptdBase->GetPixWidth();
  PIX pixBaseHeight     = _ptdBase->GetPixHeight();
  ULONG *pulTextureBase = _ptdBase->td_pulFrames
                        + GetMipmapOffset( _iWantedMipLevel, pixBaseWidth, pixBaseHeight);
  pixBaseWidth   >>= _iWantedMipLevel;
  pixBaseHeight  >>= _iWantedMipLevel;
  mmBaseWidthMask  = pixBaseWidth -1;
  mmBaseHeightMask = pixBaseHeight-1;

  ASSERT( _ptdEffect->td_pulFrames!=NULL && _ptdBase->td_pulFrames!=NULL);
  SWORD *pswHeightMap = (SWORD*)_ptdEffect->td_pubBuffer1; // height map pointer

  // copy top 2 lines from height map to bottom (so no mask offset will be needed)
  memcpy( (void*)(pswHeightMap+(_pixBufferHeight*_pixBufferWidth)), (void*)pswHeightMap,
          _pixBufferWidth*sizeof(SWORD)*2);

  // execute corresponding displace routine
  if( _pixBufferWidth >= _pixTexWidth)
  { // SUB-SAMPLING
    SLONG slHeightMapStep, slHeightRowStep;

#if (defined __MSVC_INLINE__)
    __asm {
      push    ebx
      bsf     ecx,D [_pixTexWidth]
      dec     ecx
      mov     eax,D [_pixBufferWidth]
      sar     eax,cl
      mov     D [slHeightMapStep],eax

      bsf     edx,eax
      add     edx,DISTORTION+2-1
      mov     D [mmShift],edx

      sub     eax,2
      imul    eax,D [_pixBufferWidth]
      mov     D [slHeightRowStep],eax

      mov     eax,D [pixBaseWidth]
      mov     edx,D [pixBaseHeight]
      shl     edx,16
      or      eax,edx
      sub     eax,0x00010001
      mov     D [mmBaseMasks],eax

      mov     eax,D [pixBaseWidth]
      shl     eax,16
      or      eax,1
      mov     D [mmBaseWidth],eax

      mov     ebx,D [pswHeightMap]
      mov     esi,D [pulTextureBase]
      mov     edi,D [pulTexture]
      pxor    mm6,mm6   // MM5 = 0 | 0 || pixV | pixU
      mov     eax,D [_pixBufferWidth]
      mov     edx,D [_pixTexHeight]
rowLoop:
      push    edx
      mov     ecx,D [_pixTexWidth]
pixLoop:
      movd    mm1,D [ebx]
      movd    mm3,D [ebx+ eax*2]
      movq    mm2,mm1
      psubw   mm3,mm1
      pslld   mm1,16
      psubw   mm2,mm1
      pand    mm2,Q [mm00M0]
      por     mm2,mm3
      psraw   mm2,Q [mmShift]

      paddw   mm2,mm6
      pand    mm2,Q [mmBaseMasks]
      pmaddwd mm2,Q [mmBaseWidth]
      movd    edx,mm2
      mov     edx,D [esi+ edx*4]
      mov     D [edi],edx
      // advance to next texture pixel
      add     ebx,D [slHeightMapStep]
      add     edi,4
      paddd   mm6,Q [mm0001]
      dec     ecx
      jnz     pixLoop
      // advance to next texture row
      pop     edx
      add     ebx,D [slHeightRowStep]
      paddd   mm6,Q [mm0010]
      dec     edx
      jnz     rowLoop
      emms
      pop     ebx
    }

#elif (defined __GNU_INLINE_X86_32__)
    // rcg12152001 needed extra registers. :(
    _slHeightMapStep_renderWater = slHeightMapStep;
    _pixBaseWidth_renderWater = pixBaseWidth;

    __asm__ __volatile__ (
      // this sucks :(
      "movl   %[pixBaseHeight], %%eax       \n\t"
      "movl   %[pswHeightMap], %%ecx        \n\t"
      "movl   %[pulTexture], %%edx          \n\t"
      "movl   %[pulTextureBase], %%esi      \n\t"
      "movl   %[slHeightRowStep], %%edi     \n\t"

      "pushl  %%ebx                         \n\t"  // GCC needs this.
      "movl   (" ASMSYM(_pixBaseWidth_renderWater) "),%%ebx \n\t"

      "pushl  %%eax                         \n\t"  // pixBaseHeight
      "pushl  %%ebx                         \n\t"  // pixBaseWidth
      "pushl  %%ecx                         \n\t"  // pswHeightMap
      "pushl  %%edx                         \n\t"  // pulTexture
      "pushl  %%esi                         \n\t"  // pulTextureBase
      "pushl  %%edi                         \n\t"  // slHeightRowStep

      "bsfl     (" ASMSYM(_pixTexWidth) "), %%ecx       \n\t"
      "decl     %%ecx                       \n\t"
      "movl     (" ASMSYM(_pixBufferWidth) "), %%eax    \n\t"
      "sarl     %%cl, %%eax                 \n\t"
      "movl     %%eax, (" ASMSYM(_slHeightMapStep_renderWater) ")   \n\t"

      "bsfl     %%eax, %%edx                \n\t"
      "addl     $4, %%edx                   \n\t"
      "movl     %%edx, (" ASMSYM(mmShift) ")            \n\t"

      "subl     $2, %%eax                   \n\t"
      "imul     (" ASMSYM(_pixBufferWidth) "), %%eax    \n\t"
      "movl     %%eax, (%%esp)              \n\t"  // slHeightRowStep

      "movl     16(%%esp), %%eax            \n\t"  // pixBaseWidth
      "movl     20(%%esp), %%edx            \n\t"  // pixBaseHeight
      "shll     $16, %%edx                  \n\t"
      "orl      %%edx, %%eax                \n\t"
      "subl     $0x00010001, %%eax          \n\t"
      "movl     %%eax, (" ASMSYM(mmBaseMasks) ")        \n\t"

      "movl     16(%%esp), %%eax            \n\t"  // pixBaseWidth
      "shl      $16, %%eax                  \n\t"
      "orl      $1, %%eax                   \n\t"
      "movl     %%eax, (" ASMSYM(mmBaseWidth) ")        \n\t"

      "movl     12(%%esp), %%ebx            \n\t"  // pswHeightMap
      "movl     4(%%esp), %%esi             \n\t"  // pulTextureBase
      "movl     8(%%esp), %%edi             \n\t"  // pulTexture
      "pxor     %%mm6, %%mm6                \n\t"  // MM5 = 0 | 0 || pixV | pixU
      "movl     (" ASMSYM(_pixBufferWidth) "), %%eax    \n\t"
      "movl     (" ASMSYM(_pixTexHeight) "), %%edx      \n\t"

      "0:                                   \n\t"  // rowLoop
      "pushl    %%edx                       \n\t"
      "movl     (" ASMSYM(_pixTexWidth) "), %%ecx       \n\t"
      "1:                                   \n\t"  // pixLoop
      "movd     (%%ebx), %%mm1              \n\t"
      "movd     (%%ebx, %%eax, 2), %%mm3    \n\t"
      "movq     %%mm1, %%mm2                \n\t"
      "psubw    %%mm1, %%mm3                \n\t"
      "pslld    $16, %%mm1                  \n\t"
      "psubw    %%mm1, %%mm2                \n\t"
      "pand     (" ASMSYM(mm00M0) "), %%mm2             \n\t"
      "por      %%mm3, %%mm2                \n\t"
      "psraw    (" ASMSYM(mmShift) "), %%mm2            \n\t"

      "paddw    %%mm6, %%mm2                \n\t"
      "pand     (" ASMSYM(mmBaseMasks) "), %%mm2        \n\t"
      "pmaddwd  (" ASMSYM(mmBaseWidth) "), %%mm2        \n\t"
      "movd     %%mm2, %%edx                \n\t"
      "movl     (%%esi, %%edx, 4), %%edx    \n\t"
      "movl     %%edx, (%%edi)              \n\t"

      // advance to next texture pixel
      "addl     (" ASMSYM(_slHeightMapStep_renderWater) "), %%ebx   \n\t"
      "addl     $4, %%edi                   \n\t"
      "paddd    (" ASMSYM(mm0001) "), %%mm6             \n\t"
      "decl     %%ecx                       \n\t"
      "jnz      1b                          \n\t"  // pixLoop

      // advance to next texture row
      "popl     %%edx                       \n\t"
      "addl     (%%esp), %%ebx              \n\t"  // slHeightRowStep
      "paddd    (" ASMSYM(mm0010) "), %%mm6             \n\t"
      "decl     %%edx                       \n\t"
      "jnz      0b                          \n\t"  // rowLoop
      "addl     $24, %%esp                  \n\t"  // lose our locals...
      "popl     %%ebx                       \n\t"  // restore GCC's register.
      "emms                                 \n\t"
        :  // no outputs.
        : [pixBaseHeight] "g" (pixBaseHeight),
          [pswHeightMap] "g" (pswHeightMap),
          [pulTexture] "g" (pulTexture),
          [pulTextureBase] "g" (pulTextureBase),
          [slHeightRowStep] "g" (slHeightRowStep)
        : FPU_REGS, MMX_REGS, "eax", "ecx", "edx", "esi", "edi",
          "cc", "memory"
    );

#else

    PIX pixPos, pixDU, pixDV;
    slHeightMapStep  = _pixBufferWidth/pixBaseWidth;
    slHeightRowStep  = (slHeightMapStep-1)*_pixBufferWidth;
    mmShift = DISTORTION+ FastLog2(slHeightMapStep) +2;
    for( PIX pixV=0; pixV<_pixTexHeight; pixV++)
    { // row loop
      for( PIX pixU=0; pixU<_pixTexWidth; pixU++)
      { // texel loop
        pixPos =  pswHeightMap[0];
        pixDU  = (pswHeightMap[1]               - pixPos) >>(SLONG&)mmShift;
        pixDV  = (pswHeightMap[_pixBufferWidth] - pixPos) >>(SLONG&)mmShift;
        pixDU  = (pixU +pixDU) & (SLONG&)mmBaseWidthMask;
        pixDV  = (pixV +pixDV) & (SLONG&)mmBaseHeightMask;
        *pulTexture++ = pulTextureBase[pixDV*pixBaseWidth + pixDU];
        // advance to next texel in height map
        pswHeightMap += slHeightMapStep;
      }
      pswHeightMap += slHeightRowStep;
    }

#endif

  }
  else if( _pixBufferWidth*2 == _pixTexWidth)
  { // BILINEAR SUPER-SAMPLING 2

#if ASMOPT == 1

  #if (defined __MSVC_INLINE__)
    __asm {
      push    ebx
      bsf     eax,D [pixBaseWidth]
      mov     edx,32
      sub     edx,eax
      mov     D [mmBaseWidthShift],edx

      movq    mm0,Q [mmBaseHeightMask]
      psllq   mm0,32
      por     mm0,Q [mmBaseWidthMask]
      movq    Q [mmBaseMasks],mm0

      pxor    mm6,mm6   // MM6 = pixV|pixU
      mov     ebx,D [pswHeightMap]
      mov     esi,D [pulTextureBase]
      mov     edi,D [pulTexture]
      mov     edx,D [_pixBufferHeight]
rowLoop2:
      push    edx
      mov     edx,D [_pixTexWidth]
      mov     ecx,D [_pixBufferWidth]
pixLoop2:
      mov     eax,D [_pixBufferWidth]

      movd    mm1,D [ebx+ 2]
      movd    mm0,D [ebx+ eax*2]
      psllq   mm0,32
      por     mm1,mm0
      movd    mm0,D [ebx]
      punpckldq mm0,mm0
      psubd   mm1,mm0
      movq    mm0,mm6
      pslld   mm0,DISTORTION+1+1
      paddd   mm1,mm0               // MM1 = slV_00 | slU_00

      movd    mm2,D [ebx+ 4]
      movd    mm0,D [ebx+ eax*2 +2]
      psllq   mm0,32
      por     mm2,mm0
      movd    mm0,D [ebx+ 2]
      punpckldq mm0,mm0
      psubd   mm2,mm0
      movq    mm0,mm6
      paddd   mm0,Q [mm1LO]
      pslld   mm0,DISTORTION+1+1
      paddd   mm2,mm0               // MM2 = slV_01 | slU_01

      movd    mm3,D [ebx+ eax*2 +2]
      movd    mm0,D [ebx+ eax*4]
      psllq   mm0,32
      por     mm3,mm0
      movd    mm0,D [ebx+ eax*2]
      punpckldq mm0,mm0
      psubd   mm3,mm0
      movq    mm0,mm6
      paddd   mm0,Q [mm1HI]
      pslld   mm0,DISTORTION+1+1
      paddd   mm3,mm0               // MM3 = slV_10 | slU_10

      movd    mm4,D [ebx+ eax*2 +4]
      movd    mm0,D [ebx+ eax*4 +2]
      psllq   mm0,32
      por     mm4,mm0
      movd    mm0,D [ebx+ eax*2 +2]
      punpckldq mm0,mm0
      psubd   mm4,mm0
      movq    mm0,mm6
      paddd   mm0,Q [mm1HILO]
      pslld   mm0,DISTORTION+1+1
      paddd   mm4,mm0               // MM4 = slV_11 | slU_11

      movq    mm0,mm1
      psrad   mm0,DISTORTION+1+0
      pand    mm0,Q [mmBaseMasks]
      movq    mm7,mm0
      psrlq   mm7,Q [mmBaseWidthShift]
      paddd   mm0,mm7
      movd    eax,mm0
      mov     eax,D [esi+ eax*4]
      mov     D [edi],eax

      movq    mm0,mm1
      paddd   mm0,mm2
      psrad   mm0,DISTORTION+1+1
      pand    mm0,Q [mmBaseMasks]
      movq    mm7,mm0
      psrlq   mm7,Q [mmBaseWidthShift]
      paddd   mm0,mm7
      movd    eax,mm0
      mov     eax,D [esi+ eax*4]
      mov     D [edi+ 4],eax

      movq    mm0,mm1
      paddd   mm0,mm3
      psrad   mm0,DISTORTION+1+1
      pand    mm0,Q [mmBaseMasks]
      movq    mm7,mm0
      psrlq   mm7,Q [mmBaseWidthShift]
      paddd   mm0,mm7
      movd    eax,mm0
      mov     eax,D [esi+ eax*4]
      mov     D [edi+ edx*4],eax

      paddd   mm1,mm2
      paddd   mm1,mm3
      paddd   mm1,mm4
      psrad   mm1,DISTORTION+1+2
      pand    mm1,Q [mmBaseMasks]
      movq    mm7,mm1
      psrlq   mm7,Q [mmBaseWidthShift]
      paddd   mm1,mm7
      movd    eax,mm1
      mov     eax,D [esi+ eax*4]
      mov     D [edi+ edx*4 +4],eax

      // advance to next texture pixels
      paddd   mm6,Q [mm1LO]
      add     edi,8
      add     ebx,2
      dec     ecx
      jnz     pixLoop2
      // advance to next texture row
      lea     edi,[edi+ edx*4]
      pop     edx
      paddd   mm6,Q [mm1HI]
      dec     edx
      jnz     rowLoop2
      emms
      pop     ebx
    }

  #elif (defined __GNU_INLINE_X86_32__)
    __asm__ __volatile__ (
      "bsfl      %[pixBaseWidth], %%eax             \n\t"
      "movl      $32, %%edx                         \n\t"
      "subl      %%eax, %%edx                       \n\t"
      "movl      %%edx, (" ASMSYM(mmBaseWidthShift) ")         \n\t"

      "movq      (" ASMSYM(mmBaseHeightMask) "), %%mm0          \n\t"
      "psllq     $32, %%mm0                         \n\t"
      "por       (" ASMSYM(mmBaseWidthMask) "), %%mm0           \n\t"
      "movq      %%mm0, (" ASMSYM(mmBaseMasks) ")               \n\t"

      "pxor      %%mm6, %%mm6                       \n\t" // MM6 = pixV|pixU

      "movl      %[pswHeightMap], %%edx             \n\t"
      "movl      %[pulTextureBase], %%esi           \n\t"
      "movl      %[pulTexture], %%edi               \n\t"
      "pushl     %%ebx                              \n\t"  // GCC's register.
      "movl      %%edx, %%ebx                       \n\t"
      "movl      (" ASMSYM(_pixBufferHeight) "), %%edx          \n\t"

      "0:                                           \n\t" // rowLoop2
      "pushl     %%edx                              \n\t"
      "movl      (" ASMSYM(_pixTexWidth) "), %%edx              \n\t"
      "movl      (" ASMSYM(_pixBufferWidth) "), %%ecx           \n\t"

      "1:                                           \n\t" // pixLoop2
      "mov       (" ASMSYM(_pixBufferWidth) "), %%eax           \n\t"

      "movd      2(%%ebx), %%mm1                    \n\t"
      "movd      0(%%ebx, %%eax, 2), %%mm0          \n\t"
      "psllq     $32, %%mm0                         \n\t"
      "por       %%mm0, %%mm1                       \n\t"
      "movd      (%%ebx), %%mm0                     \n\t"
      "punpckldq %%mm0, %%mm0                       \n\t"
      "psubd     %%mm0, %%mm1                       \n\t"
      "movq      %%mm6, %%mm0                       \n\t"
      "pslld     $5, %%mm0                          \n\t"
      "paddd     %%mm0, %%mm1                       \n\t" // MM1 = slV_00 | slU_00

      "movd      4(%%ebx), %%mm2                    \n\t"
      "movd      2(%%ebx, %%eax, 2), %%mm0          \n\t"
      "psllq     $32, %%mm0                         \n\t"
      "por       %%mm0, %%mm2                       \n\t"
      "movd      2(%%ebx), %%mm0                    \n\t"
      "punpckldq %%mm0, %%mm0                       \n\t"
      "psubd     %%mm0, %%mm2                       \n\t"
      "movq      %%mm6, %%mm0                       \n\t"
      "paddd     (" ASMSYM(mm1LO) "), %%mm0                     \n\t"
      "pslld     $5, %%mm0                          \n\t"
      "paddd     %%mm0, %%mm2                       \n\t" // MM2 = slV_01 | slU_01

      "movd      2(%%ebx, %%eax, 2), %%mm3          \n\t"
      "movd      (%%ebx, %%eax, 4), %%mm0           \n\t"
      "psllq     $32, %%mm0                         \n\t"
      "por       %%mm0, %%mm3                       \n\t"
      "movd      (%%ebx, %%eax, 2), %%mm0           \n\t"
      "punpckldq %%mm0, %%mm0                       \n\t"
      "psubd     %%mm0, %%mm3                       \n\t"
      "movq      %%mm6, %%mm0                       \n\t"
      "paddd     (" ASMSYM(mm1HI) "), %%mm0                     \n\t"
      "pslld     $5, %%mm0                          \n\t"
      "paddd     %%mm0, %%mm3                       \n\t" // MM3 = slV_10 | slU_10

      "movd      4(%%ebx, %%eax, 2), %%mm4          \n\t"
      "movd      2(%%ebx, %%eax, 4), %%mm0          \n\t"
      "psllq     $32, %%mm0                         \n\t"
      "por       %%mm0, %%mm4                       \n\t"
      "movd      2(%%ebx, %%eax, 2), %%mm0          \n\t"
      "punpckldq %%mm0, %%mm0                       \n\t"
      "psubd     %%mm0, %%mm4                       \n\t"
      "movq      %%mm6, %%mm0                       \n\t"
      "paddd     (" ASMSYM(mm1HILO) "), %%mm0                   \n\t"
      "pslld     $5, %%mm0                          \n\t"
      "paddd     %%mm0, %%mm4                       \n\t" // MM4 = slV_11 | slU_11

      "movq      %%mm1, %%mm0                       \n\t"
      "psrad     $4, %%mm0                          \n\t"
      "pand      (" ASMSYM(mmBaseMasks) "), %%mm0               \n\t"
      "movq      %%mm0, %%mm7                       \n\t"
      "psrlq     (" ASMSYM(mmBaseWidthShift) "), %%mm7          \n\t"
      "paddd     %%mm7, %%mm0                       \n\t"
      "movd      %%mm0, %%eax                       \n\t"
      "movl      (%%esi, %%eax, 4), %%eax           \n\t"
      "movl      %%eax, (%%edi)                     \n\t"

      "movq      %%mm1, %%mm0                       \n\t"
      "paddd     %%mm2, %%mm0                       \n\t"
      "psrad     $5, %%mm0                          \n\t"
      "pand      (" ASMSYM(mmBaseMasks) "), %%mm0               \n\t"
      "movq      %%mm0, %%mm7                       \n\t"
      "psrlq     (" ASMSYM(mmBaseWidthShift) "), %%mm7          \n\t"
      "paddd     %%mm7, %%mm0                       \n\t"
      "movd      %%mm0, %%eax                       \n\t"
      "movl      (%%esi, %%eax, 4), %%eax           \n\t"
      "movl      %%eax, 4(%%edi)                    \n\t"

      "movq      %%mm1, %%mm0                       \n\t"
      "paddd     %%mm3, %%mm0                       \n\t"
      "psrad     $5, %%mm0                          \n\t"
      "pand      (" ASMSYM(mmBaseMasks) "), %%mm0               \n\t"
      "movq      %%mm0, %%mm7                       \n\t"
      "psrlq     (" ASMSYM(mmBaseWidthShift) "), %%mm7          \n\t"
      "paddd     %%mm7, %%mm0                       \n\t"
      "movd      %%mm0, %%eax                       \n\t"
      "movl      (%%esi, %%eax, 4), %%eax           \n\t"
      "movl      %%eax, (%%edi, %%edx, 4)           \n\t"

      "paddd     %%mm2, %%mm1                       \n\t"
      "paddd     %%mm3, %%mm1                       \n\t"
      "paddd     %%mm4, %%mm1                       \n\t"
      "psrad     $6, %%mm1                          \n\t"
      "pand      (" ASMSYM(mmBaseMasks) "), %%mm1               \n\t"
      "movq      %%mm1, %%mm7                       \n\t"
      "psrlq     (" ASMSYM(mmBaseWidthShift) "), %%mm7          \n\t"
      "paddd     %%mm7, %%mm1                       \n\t"
      "movd      %%mm1, %%eax                       \n\t"
      "mov       (%%esi, %%eax, 4), %%eax           \n\t"
      "mov       %%eax, 4(%%edi, %%edx, 4)          \n\t"

      // advance to next texture pixels
      "paddd     (" ASMSYM(mm1LO) "), %%mm6                     \n\t"
      "addl      $8, %%edi                          \n\t"
      "addl      $2, %%ebx                          \n\t"
      "decl      %%ecx                              \n\t"
      "jnz       1b                                 \n\t"  // pixLoop2

      // advance to next texture row
      "leal      (%%edi, %%edx, 4), %%edi           \n\t"
      "popl      %%edx                              \n\t"
      "paddd     (" ASMSYM(mm1HI) "), %%mm6                     \n\t"
      "decl      %%edx                              \n\t"
      "jnz       0b                                 \n\t"  // rowLoop2
      "popl      %%ebx                              \n\t"  // GCC's value.
      "emms                                         \n\t"
        : // no outputs.
        : [pixBaseWidth] "g" (pixBaseWidth),
          [pswHeightMap] "g" (pswHeightMap),
          [pulTextureBase] "g" (pulTextureBase),
          [pulTexture] "g" (pulTexture)
        : FPU_REGS, MMX_REGS, "eax", "ecx", "edx", "esi", "edi",
          "cc", "memory"
    );

  #else
    #error fill in for you platform.
  #endif


#else

    SLONG slU_00, slU_01, slU_10, slU_11;
    SLONG slV_00, slV_01, slV_10, slV_11;
    for( PIX pixV=0; pixV<_pixBufferHeight; pixV++)
    { // row loop
      for( PIX pixU=0; pixU<_pixBufferWidth; pixU++)
      { // texel loop
        slU_00 = pswHeightMap[_pixBufferWidth*0+1] - pswHeightMap[_pixBufferWidth*0+0] + ((pixU+0)<<(DISTORTION+1+1));
        slV_00 = pswHeightMap[_pixBufferWidth*1+0] - pswHeightMap[_pixBufferWidth*0+0] + ((pixV+0)<<(DISTORTION+1+1));
        slU_01 = pswHeightMap[_pixBufferWidth*0+2] - pswHeightMap[_pixBufferWidth*0+1] + ((pixU+1)<<(DISTORTION+1+1));
        slV_01 = pswHeightMap[_pixBufferWidth*1+1] - pswHeightMap[_pixBufferWidth*0+1] + ((pixV+0)<<(DISTORTION+1+1));
        slU_10 = pswHeightMap[_pixBufferWidth*1+1] - pswHeightMap[_pixBufferWidth*1+0] + ((pixU+0)<<(DISTORTION+1+1));
        slV_10 = pswHeightMap[_pixBufferWidth*2+0] - pswHeightMap[_pixBufferWidth*1+0] + ((pixV+1)<<(DISTORTION+1+1));
        slU_11 = pswHeightMap[_pixBufferWidth*1+2] - pswHeightMap[_pixBufferWidth*1+1] + ((pixU+1)<<(DISTORTION+1+1));
        slV_11 = pswHeightMap[_pixBufferWidth*2+1] - pswHeightMap[_pixBufferWidth*1+1] + ((pixV+1)<<(DISTORTION+1+1));

        pulTexture[_pixTexWidth*0+0] = PIXEL( (slU_00                     ) >>(DISTORTION+1  ), (slV_00                     ) >>(DISTORTION+1  ) );
        pulTexture[_pixTexWidth*0+1] = PIXEL( (slU_00+slU_01              ) >>(DISTORTION+1+1), (slV_00+slV_01              ) >>(DISTORTION+1+1) );
        pulTexture[_pixTexWidth*1+0] = PIXEL( (slU_00       +slU_10       ) >>(DISTORTION+1+1), (slV_00       +slV_10       ) >>(DISTORTION+1+1) );
        pulTexture[_pixTexWidth*1+1] = PIXEL( (slU_00+slU_01+slU_10+slU_11) >>(DISTORTION+1+2), (slV_00+slV_01+slV_10+slV_11) >>(DISTORTION+1+2) );

        // advance to next texel
        pulTexture+=2;
        pswHeightMap++;
      }
      pulTexture+=_pixTexWidth;
    }

#endif

  }
  else if( _pixBufferWidth*4 == _pixTexWidth)
  { // BILINEAR SUPER-SAMPLING 4

#if ASMOPT == 1

  #if (defined __MSVC_INLINE__)
    __asm {
      push    ebx
      bsf     eax,D [pixBaseWidth]
      mov     edx,32
      sub     edx,eax
      mov     D [mmBaseWidthShift],edx

      movq    mm0,Q [mmBaseHeightMask]
      psllq   mm0,32
      por     mm0,Q [mmBaseWidthMask]
      movq    Q [mmBaseMasks],mm0

      pxor    mm6,mm6   // MM6 = pixV|pixU
      mov     ebx,D [pswHeightMap]
      mov     esi,D [pulTextureBase]
      mov     edi,D [pulTexture]
      mov     edx,D [_pixBufferHeight]
rowLoop4:
      push    edx
      mov     ecx,D [_pixBufferWidth]
pixLoop4:
      mov     eax,D [_pixBufferWidth]
      mov     edx,D [_pixTexWidth]

      movd    mm1,D [ebx+ 2]
      movd    mm0,D [ebx+ eax*2]
      psllq   mm0,32
      por     mm1,mm0
      movd    mm0,D [ebx]
      punpckldq mm0,mm0
      psubd   mm1,mm0
      movq    mm0,mm6
      pslld   mm0,DISTORTION+1+1
      paddd   mm1,mm0               // MM1 = slV_00 | slU_00

      movd    mm2,D [ebx+ 4]
      movd    mm0,D [ebx+ eax*2 +2]
      psllq   mm0,32
      por     mm2,mm0
      movd    mm0,D [ebx+ 2]
      punpckldq mm0,mm0
      psubd   mm2,mm0
      movq    mm0,mm6
      paddd   mm0,Q [mm1LO]
      pslld   mm0,DISTORTION+1+1
      paddd   mm2,mm0               // MM2 = slV_01 | slU_01

      movd    mm3,D [ebx+ eax*2 +2]
      movd    mm0,D [ebx+ eax*4]
      psllq   mm0,32
      por     mm3,mm0
      movd    mm0,D [ebx+ eax*2]
      punpckldq mm0,mm0
      psubd   mm3,mm0
      movq    mm0,mm6
      paddd   mm0,Q [mm1HI]
      pslld   mm0,DISTORTION+1+1
      paddd   mm3,mm0               // MM3 = slV_10 | slU_10

      movd    mm4,D [ebx+ eax*2 +4]
      movd    mm0,D [ebx+ eax*4 +2]
      psllq   mm0,32
      por     mm4,mm0
      movd    mm0,D [ebx+ eax*2 +2]
      punpckldq mm0,mm0
      psubd   mm4,mm0
      movq    mm0,mm6
      paddd   mm0,Q [mm1HILO]
      pslld   mm0,DISTORTION+1+1
      paddd   mm4,mm0               // MM4 = slV_11 | slU_11

      // texel 00
      movq    mm0,mm1
      psrad   mm0,DISTORTION
      pand    mm0,Q [mmBaseMasks]
      movq    mm7,mm0
      psrlq   mm7,Q [mmBaseWidthShift]
      paddd   mm0,mm7
      movd    eax,mm0
      mov     eax,D [esi+ eax*4]
      mov     D [edi],eax
      // texel 01
      movq    mm0,mm1
      paddd   mm0,mm1
      paddd   mm0,mm1
      paddd   mm0,mm2
      psrad   mm0,DISTORTION+2
      pand    mm0,Q [mmBaseMasks]
      movq    mm7,mm0
      psrlq   mm7,Q [mmBaseWidthShift]
      paddd   mm0,mm7
      movd    eax,mm0
      mov     eax,D [esi+ eax*4]
      mov     D [edi +4],eax
      // texel 02
      movq    mm0,mm1
      paddd   mm0,mm2
      psrad   mm0,DISTORTION+1
      pand    mm0,Q [mmBaseMasks]
      movq    mm7,mm0
      psrlq   mm7,Q [mmBaseWidthShift]
      paddd   mm0,mm7
      movd    eax,mm0
      mov     eax,D [esi+ eax*4]
      mov     D [edi +8],eax
      // texel 03
      movq    mm0,mm1
      paddd   mm0,mm2
      paddd   mm0,mm2
      paddd   mm0,mm2
      psrad   mm0,DISTORTION+2
      pand    mm0,Q [mmBaseMasks]
      movq    mm7,mm0
      psrlq   mm7,Q [mmBaseWidthShift]
      paddd   mm0,mm7
      movd    eax,mm0
      mov     eax,D [esi+ eax*4]
      mov     D [edi +12],eax

      // texel 10
      movq    mm0,mm1
      paddd   mm0,mm1
      paddd   mm0,mm1
      paddd   mm0,mm3
      psrad   mm0,DISTORTION+2
      pand    mm0,Q [mmBaseMasks]
      movq    mm7,mm0
      psrlq   mm7,Q [mmBaseWidthShift]
      paddd   mm0,mm7
      movd    eax,mm0
      mov     eax,D [esi+ eax*4]
      mov     D [edi+ edx*4],eax
      // texel 11
      movq    mm0,mm1
      pslld   mm0,3
      paddd   mm0,mm1
      paddd   mm0,mm2
      paddd   mm0,mm2
      paddd   mm0,mm2
      paddd   mm0,mm3
      paddd   mm0,mm3
      paddd   mm0,mm3
      paddd   mm0,mm4
      psrad   mm0,DISTORTION+4
      pand    mm0,Q [mmBaseMasks]
      movq    mm7,mm0
      psrlq   mm7,Q [mmBaseWidthShift]
      paddd   mm0,mm7
      movd    eax,mm0
      mov     eax,D [esi+ eax*4]
      mov     D [edi+ edx*4 +4],eax
      // texel 12
      movq    mm0,mm1
      paddd   mm0,mm0
      paddd   mm0,mm1
      paddd   mm0,mm2
      paddd   mm0,mm2
      paddd   mm0,mm2
      paddd   mm0,mm3
      paddd   mm0,mm4
      psrad   mm0,DISTORTION+3
      pand    mm0,Q [mmBaseMasks]
      movq    mm7,mm0
      psrlq   mm7,Q [mmBaseWidthShift]
      paddd   mm0,mm7
      movd    eax,mm0
      mov     eax,D [esi+ eax*4]
      mov     D [edi+ edx*4 +8],eax
      // texel 13
      movq    mm0,mm2
      pslld   mm0,3
      paddd   mm0,mm2
      paddd   mm0,mm1
      paddd   mm0,mm1
      paddd   mm0,mm1
      paddd   mm0,mm3
      paddd   mm0,mm4
      paddd   mm0,mm4
      paddd   mm0,mm4
      psrad   mm0,DISTORTION+4
      pand    mm0,Q [mmBaseMasks]
      movq    mm7,mm0
      psrlq   mm7,Q [mmBaseWidthShift]
      paddd   mm0,mm7
      movd    eax,mm0
      mov     eax,D [esi+ eax*4]
      mov     D [edi+ edx*4 +12],eax

      // texel 20
      movq    mm0,mm1
      paddd   mm0,mm3
      psrad   mm0,DISTORTION+1
      pand    mm0,Q [mmBaseMasks]
      movq    mm7,mm0
      psrlq   mm7,Q [mmBaseWidthShift]
      paddd   mm0,mm7
      movd    eax,mm0
      mov     eax,D [esi+ eax*4]
      mov     D [edi+ edx*8],eax
      // texel 21
      movq    mm0,mm1
      paddd   mm0,mm1
      paddd   mm0,mm1
      paddd   mm0,mm2
      paddd   mm0,mm3
      paddd   mm0,mm3
      paddd   mm0,mm3
      paddd   mm0,mm4
      psrad   mm0,DISTORTION+3
      pand    mm0,Q [mmBaseMasks]
      movq    mm7,mm0
      psrlq   mm7,Q [mmBaseWidthShift]
      paddd   mm0,mm7
      movd    eax,mm0
      mov     eax,D [esi+ eax*4]
      mov     D [edi+ edx*8 +4],eax
      // texel 22
      movq    mm0,mm1
      paddd   mm0,mm2
      paddd   mm0,mm3
      paddd   mm0,mm4
      psrad   mm0,DISTORTION+2
      pand    mm0,Q [mmBaseMasks]
      movq    mm7,mm0
      psrlq   mm7,Q [mmBaseWidthShift]
      paddd   mm0,mm7
      movd    eax,mm0
      mov     eax,D [esi+ eax*4]
      mov     D [edi+ edx*8 +8],eax
      // texel 23
      movq    mm0,mm1
      paddd   mm0,mm2
      paddd   mm0,mm2
      paddd   mm0,mm2
      paddd   mm0,mm3
      paddd   mm0,mm4
      paddd   mm0,mm4
      paddd   mm0,mm4
      psrad   mm0,DISTORTION+3
      pand    mm0,Q [mmBaseMasks]
      movq    mm7,mm0
      psrlq   mm7,Q [mmBaseWidthShift]
      paddd   mm0,mm7
      movd    eax,mm0
      mov     eax,D [esi+ eax*4]
      mov     D [edi+ edx*8 +12],eax

      imul    edx,3 // _pixTexWidth*=3
      // texel 30
      movq    mm0,mm1
      paddd   mm0,mm3
      paddd   mm0,mm3
      paddd   mm0,mm3
      psrad   mm0,DISTORTION+2
      pand    mm0,Q [mmBaseMasks]
      movq    mm7,mm0
      psrlq   mm7,Q [mmBaseWidthShift]
      paddd   mm0,mm7
      movd    eax,mm0
      mov     eax,D [esi+ eax*4]
      mov     D [edi+ edx*4],eax
      // texel 31
      movq    mm0,mm3
      pslld   mm0,3
      paddd   mm0,mm3
      paddd   mm0,mm1
      paddd   mm0,mm1
      paddd   mm0,mm1
      paddd   mm0,mm2
      paddd   mm0,mm4
      paddd   mm0,mm4
      paddd   mm0,mm4
      psrad   mm0,DISTORTION+4
      pand    mm0,Q [mmBaseMasks]
      movq    mm7,mm0
      psrlq   mm7,Q [mmBaseWidthShift]
      paddd   mm0,mm7
      movd    eax,mm0
      mov     eax,D [esi+ eax*4]
      mov     D [edi+ edx*4 +4],eax
      // texel 32
      movq    mm0,mm4
      paddd   mm0,mm0
      paddd   mm0,mm4
      paddd   mm0,mm3
      paddd   mm0,mm3
      paddd   mm0,mm3
      paddd   mm0,mm2
      paddd   mm0,mm1
      psrad   mm0,DISTORTION+3
      pand    mm0,Q [mmBaseMasks]
      movq    mm7,mm0
      psrlq   mm7,Q [mmBaseWidthShift]
      paddd   mm0,mm7
      movd    eax,mm0
      mov     eax,D [esi+ eax*4]
      mov     D [edi+ edx*4 +8],eax
      // texel 33
      movq    mm0,mm4
      pslld   mm0,3
      paddd   mm0,mm4
      paddd   mm0,mm1
      paddd   mm0,mm2
      paddd   mm0,mm2
      paddd   mm0,mm2
      paddd   mm0,mm3
      paddd   mm0,mm3
      paddd   mm0,mm3
      psrad   mm0,DISTORTION+4
      pand    mm0,Q [mmBaseMasks]
      movq    mm7,mm0
      psrlq   mm7,Q [mmBaseWidthShift]
      paddd   mm0,mm7
      movd    eax,mm0
      mov     eax,D [esi+ eax*4]
      mov     D [edi+ edx*4 +12],eax

      // advance to next texture pixels
      paddd   mm6,Q [mm1LO]
      add     edi,16
      add     ebx,2
      dec     ecx
      jnz     pixLoop4
      // advance to next texture row
      lea     edi,[edi+ edx*4] // +=[_pixTexWidth]*3
      pop     edx
      paddd   mm6,Q [mm1HI]
      dec     edx
      jnz     rowLoop4
      emms
      pop     ebx
    }

  #elif (defined __GNU_INLINE_X86_32__)
    __asm__ __volatile__ (
      "bsfl      %[pixBaseWidth], %%eax             \n\t"
      "movl      $32, %%edx                         \n\t"
      "subl      %%eax, %%edx                       \n\t"
      "movl      %%edx, (" ASMSYM(mmBaseWidthShift) ")         \n\t"

      "movq      (" ASMSYM(mmBaseHeightMask) "), %%mm0          \n\t"
      "psllq     $32, %%mm0                         \n\t"
      "por       (" ASMSYM(mmBaseWidthMask) "), %%mm0           \n\t"
      "movq      %%mm0, (" ASMSYM(mmBaseMasks) ")               \n\t"

      "pxor      %%mm6, %%mm6                       \n\t" // MM6 = pixV|pixU

      "movl      %[pswHeightMap], %%edx             \n\t"
      "movl      %[pulTextureBase], %%esi           \n\t"
      "movl      %[pulTexture], %%edi               \n\t"
      "pushl     %%ebx                              \n\t"  // GCC's register.
      "movl      %%edx, %%ebx                       \n\t"
      "movl      (" ASMSYM(_pixBufferHeight) "), %%edx          \n\t"
      "0:                                      \n\t" // rowLoop4
      "pushl     %%edx                         \n\t"
      "movl      (" ASMSYM(_pixBufferWidth) "), %%ecx      \n\t"
      "1:                                      \n\t" // pixLoop4
      "movl      (" ASMSYM(_pixBufferWidth) "), %%eax      \n\t"
      "movl      (" ASMSYM(_pixTexWidth) "), %%edx         \n\t"

      "movd      2(%%ebx), %%mm1               \n\t"
      "movd      (%%ebx, %%eax, 2), %%mm0      \n\t"
      "psllq     $32, %%mm0                    \n\t"
      "por       %%mm0, %%mm1                  \n\t"
      "movd      (%%ebx), %%mm0                \n\t"
      "punpckldq %%mm0, %%mm0                  \n\t"
      "psubd     %%mm0, %%mm1                  \n\t"
      "movq      %%mm6, %%mm0                  \n\t"
      "pslld     $5, %%mm0                     \n\t"
      "paddd     %%mm0, %%mm1                  \n\t" // MM1 = slV_00 | slU_00

      "movd      4(%%ebx), %%mm2               \n\t"
      "movd      2(%%ebx, %%eax, 2), %%mm0     \n\t"
      "psllq     $32, %%mm0                    \n\t"
      "por       %%mm0, %%mm2                  \n\t"
      "movd      2(%%ebx), %%mm0               \n\t"
      "punpckldq %%mm0, %%mm0                  \n\t"
      "psubd     %%mm0, %%mm2                  \n\t"
      "movq      %%mm6, %%mm0                  \n\t"
      "paddd     (" ASMSYM(mm1LO) "), %%mm0                \n\t"
      "pslld     $5, %%mm0                     \n\t"
      "paddd     %%mm0, %%mm2                  \n\t" // MM2 = slV_01 | slU_01

      "movd      2(%%ebx, %%eax, 2), %%mm3     \n\t"
      "movd      (%%ebx, %%eax, 4), %%mm0      \n\t"
      "psllq     $32, %%mm0                    \n\t"
      "por       %%mm0, %%mm3                  \n\t"
      "movd      (%%ebx, %%eax, 2), %%mm0      \n\t"
      "punpckldq %%mm0, %%mm0                  \n\t"
      "psubd     %%mm0, %%mm3                  \n\t"
      "movq      %%mm6, %%mm0                  \n\t"
      "paddd     (" ASMSYM(mm1HI) "), %%mm0                \n\t"
      "pslld     $5, %%mm0                     \n\t"
      "paddd     %%mm0, %%mm3                  \n\t" // MM3 = slV_10 | slU_10

      "movd      4(%%ebx, %%eax, 2), %%mm4     \n\t"
      "movd      2(%%ebx, %%eax, 4), %%mm0     \n\t"
      "psllq     $32, %%mm0                    \n\t"
      "por       %%mm0, %%mm4                  \n\t"
      "movd      2(%%ebx, %%eax, 2), %%mm0     \n\t"
      "punpckldq %%mm0, %%mm0                  \n\t"
      "psubd     %%mm0, %%mm4                  \n\t"
      "movq      %%mm6, %%mm0                  \n\t"
      "paddd     (" ASMSYM(mm1HILO) "), %%mm0              \n\t"
      "pslld     $5, %%mm0                     \n\t"
      "paddd     %%mm0, %%mm4                  \n\t" // MM4 = slV_11 | slU_11

      // texel 00
      "movq      %%mm1, %%mm0                  \n\t"
      "psrad     $3, %%mm0                     \n\t"
      "pand      (" ASMSYM(mmBaseMasks) "), %%mm0          \n\t"
      "movq      %%mm0, %%mm7                  \n\t"
      "psrlq     (" ASMSYM(mmBaseWidthShift) "), %%mm7     \n\t"
      "paddd     %%mm7, %%mm0                  \n\t"
      "movd      %%mm0, %%eax                  \n\t"
      "movl      (%%esi, %%eax, 4), %%eax      \n\t"
      "movl      %%eax, (%%edi)                \n\t"

      // texel 01
      "movq      %%mm1, %%mm0                  \n\t"
      "paddd     %%mm1, %%mm0                  \n\t"
      "paddd     %%mm1, %%mm0                  \n\t"
      "paddd     %%mm2, %%mm0                  \n\t"
      "psrad     $5, %%mm0                     \n\t"
      "pand      (" ASMSYM(mmBaseMasks) "), %%mm0          \n\t"
      "movq      %%mm0, %%mm7                  \n\t"
      "psrlq     (" ASMSYM(mmBaseWidthShift) "), %%mm7     \n\t"
      "paddd     %%mm7, %%mm0                  \n\t"
      "movd      %%mm0, %%eax                  \n\t"
      "movl      (%%esi, %%eax, 4), %%eax      \n\t"
      "movl      %%eax, 4(%%edi)               \n\t"

      // texel 02
      "movq      %%mm1, %%mm0                  \n\t"
      "paddd     %%mm2, %%mm0                  \n\t"
      "psrad     $4, %%mm0                     \n\t"
      "pand      (" ASMSYM(mmBaseMasks) "), %%mm0          \n\t"
      "movq      %%mm0, %%mm7                  \n\t"
      "psrlq     (" ASMSYM(mmBaseWidthShift) "), %%mm7     \n\t"
      "paddd     %%mm7, %%mm0                  \n\t"
      "movd      %%mm0, %%eax                  \n\t"
      "movl      (%%esi, %%eax, 4), %%eax      \n\t"
      "movl      %%eax, 8(%%edi)               \n\t"

      // texel 03
      "movq      %%mm1, %%mm0                  \n\t"
      "paddd     %%mm2, %%mm0                  \n\t"
      "paddd     %%mm2, %%mm0                  \n\t"
      "paddd     %%mm2, %%mm0                  \n\t"
      "psrad     $5, %%mm0                     \n\t"
      "pand      (" ASMSYM(mmBaseMasks) "), %%mm0          \n\t"
      "movq      %%mm0, %%mm7                  \n\t"
      "psrlq     (" ASMSYM(mmBaseWidthShift) "), %%mm7     \n\t"
      "paddd     %%mm7, %%mm0                  \n\t"
      "movd      %%mm0, %%eax                  \n\t"
      "movl      (%%esi, %%eax, 4), %%eax      \n\t"
      "movl      %%eax, 12(%%edi)              \n\t"

      // texel 10
      "movq      %%mm1, %%mm0                  \n\t"
      "paddd     %%mm1, %%mm0                  \n\t"
      "paddd     %%mm1, %%mm0                  \n\t"
      "paddd     %%mm3, %%mm0                  \n\t"
      "psrad     $5, %%mm0                     \n\t"
      "pand      (" ASMSYM(mmBaseMasks) "), %%mm0          \n\t"
      "movq      %%mm0, %%mm7                  \n\t"
      "psrlq     (" ASMSYM(mmBaseWidthShift) "), %%mm7     \n\t"
      "paddd     %%mm7, %%mm0                  \n\t"
      "movd      %%mm0, %%eax                  \n\t"
      "movl      (%%esi, %%eax, 4), %%eax      \n\t"
      "movl      %%eax, (%%edi, %%edx, 4)      \n\t"

      // texel 11
      "movq      %%mm1, %%mm0                  \n\t"
      "pslld     $3, %%mm0                     \n\t"
      "paddd     %%mm1, %%mm0                  \n\t"
      "paddd     %%mm2, %%mm0                  \n\t"
      "paddd     %%mm2, %%mm0                  \n\t"
      "paddd     %%mm2, %%mm0                  \n\t"
      "paddd     %%mm3, %%mm0                  \n\t"
      "paddd     %%mm3, %%mm0                  \n\t"
      "paddd     %%mm3, %%mm0                  \n\t"
      "paddd     %%mm4, %%mm0                  \n\t"
      "psrad     $7, %%mm0                     \n\t"
      "pand      (" ASMSYM(mmBaseMasks) "), %%mm0          \n\t"
      "movq      %%mm0, %%mm7                  \n\t"
      "psrlq     (" ASMSYM(mmBaseWidthShift) "), %%mm7     \n\t"
      "paddd     %%mm7, %%mm0                  \n\t"
      "movd      %%mm0, %%eax                  \n\t"
      "movl      (%%esi, %%eax, 4), %%eax      \n\t"
      "movl      %%eax, 4(%%edi, %%edx, 4)     \n\t"

      // texel 12
      "movq      %%mm1, %%mm0                  \n\t"
      "paddd     %%mm0, %%mm0                  \n\t"
      "paddd     %%mm1, %%mm0                  \n\t"
      "paddd     %%mm2, %%mm0                  \n\t"
      "paddd     %%mm2, %%mm0                  \n\t"
      "paddd     %%mm2, %%mm0                  \n\t"
      "paddd     %%mm3, %%mm0                  \n\t"
      "paddd     %%mm4, %%mm0                  \n\t"
      "psrad     $6, %%mm0                     \n\t"
      "pand      (" ASMSYM(mmBaseMasks) "), %%mm0          \n\t"
      "movq      %%mm0, %%mm7                  \n\t"
      "psrlq     (" ASMSYM(mmBaseWidthShift) "), %%mm7     \n\t"
      "paddd     %%mm7, %%mm0                  \n\t"
      "movd      %%mm0, %%eax                  \n\t"
      "movl      (%%esi, %%eax, 4), %%eax      \n\t"
      "movl      %%eax, 8(%%edi, %%edx, 4)     \n\t"

      // texel 13
      "movq      %%mm2, %%mm0                  \n\t"
      "pslld     $3, %%mm0                     \n\t"
      "paddd     %%mm2, %%mm0                  \n\t"
      "paddd     %%mm1, %%mm0                  \n\t"
      "paddd     %%mm1, %%mm0                  \n\t"
      "paddd     %%mm1, %%mm0                  \n\t"
      "paddd     %%mm3, %%mm0                  \n\t"
      "paddd     %%mm4, %%mm0                  \n\t"
      "paddd     %%mm4, %%mm0                  \n\t"
      "paddd     %%mm4, %%mm0                  \n\t"
      "psrad     $7, %%mm0                     \n\t"
      "pand      (" ASMSYM(mmBaseMasks) "), %%mm0          \n\t"
      "movq      %%mm0, %%mm7                  \n\t"
      "psrlq     (" ASMSYM(mmBaseWidthShift) "), %%mm7     \n\t"
      "paddd     %%mm7, %%mm0                  \n\t"
      "movd      %%mm0, %%eax                  \n\t"
      "movl      (%%esi, %%eax, 4), %%eax      \n\t"
      "movl      %%eax, 12(%%edi, %%edx, 4)    \n\t"

      // texel 20
      "movq      %%mm1, %%mm0                  \n\t"
      "paddd     %%mm3, %%mm0                  \n\t"
      "psrad     $4, %%mm0                     \n\t"
      "pand      (" ASMSYM(mmBaseMasks) "), %%mm0          \n\t"
      "movq      %%mm0, %%mm7                  \n\t"
      "psrlq     (" ASMSYM(mmBaseWidthShift) "), %%mm7     \n\t"
      "paddd     %%mm7, %%mm0                  \n\t"
      "movd      %%mm0, %%eax                  \n\t"
      "movl      (%%esi, %%eax, 4), %%eax      \n\t"
      "movl      %%eax, (%%edi, %%edx, 8)      \n\t"

      // texel 21
      "movq      %%mm1, %%mm0                  \n\t"
      "paddd     %%mm1, %%mm0                  \n\t"
      "paddd     %%mm1, %%mm0                  \n\t"
      "paddd     %%mm2, %%mm0                  \n\t"
      "paddd     %%mm3, %%mm0                  \n\t"
      "paddd     %%mm3, %%mm0                  \n\t"
      "paddd     %%mm3, %%mm0                  \n\t"
      "paddd     %%mm4, %%mm0                  \n\t"
      "psrad     $6, %%mm0                     \n\t"
      "pand      (" ASMSYM(mmBaseMasks) "), %%mm0          \n\t"
      "movq      %%mm0, %%mm7                  \n\t"
      "psrlq     (" ASMSYM(mmBaseWidthShift) "), %%mm7     \n\t"
      "paddd     %%mm7, %%mm0                  \n\t"
      "movd      %%mm0, %%eax                  \n\t"
      "movl      (%%esi, %%eax, 4), %%eax      \n\t"
      "movl      %%eax, 4(%%edi, %%edx, 8)     \n\t"

      // texel 22
      "movq      %%mm1, %%mm0                  \n\t"
      "paddd     %%mm2, %%mm0                  \n\t"
      "paddd     %%mm3, %%mm0                  \n\t"
      "paddd     %%mm4, %%mm0                  \n\t"
      "psrad     $5, %%mm0                     \n\t"
      "pand      (" ASMSYM(mmBaseMasks) "), %%mm0          \n\t"
      "movq      %%mm0, %%mm7                  \n\t"
      "psrlq     (" ASMSYM(mmBaseWidthShift) "), %%mm7     \n\t"
      "paddd     %%mm7, %%mm0                  \n\t"
      "movd      %%mm0, %%eax                  \n\t"
      "movl      (%%esi, %%eax, 4), %%eax      \n\t"
      "movl      %%eax, 8(%%edi, %%edx, 8)     \n\t"

      // texel 23
      "movq      %%mm1, %%mm0                  \n\t"
      "paddd     %%mm2, %%mm0                  \n\t"
      "paddd     %%mm2, %%mm0                  \n\t"
      "paddd     %%mm2, %%mm0                  \n\t"
      "paddd     %%mm3, %%mm0                  \n\t"
      "paddd     %%mm4, %%mm0                  \n\t"
      "paddd     %%mm4, %%mm0                  \n\t"
      "paddd     %%mm4, %%mm0                  \n\t"
      "psrad     $6, %%mm0                     \n\t"
      "pand      (" ASMSYM(mmBaseMasks) "), %%mm0          \n\t"
      "movq      %%mm0, %%mm7                  \n\t"
      "psrlq     (" ASMSYM(mmBaseWidthShift) "), %%mm7     \n\t"
      "paddd     %%mm7, %%mm0                  \n\t"
      "movd      %%mm0, %%eax                  \n\t"
      "movl      (%%esi, %%eax, 4), %%eax      \n\t"
      "movl      %%eax, 12(%%edi, %%edx, 8)    \n\t"

      "imull     $3, %%edx                     \n\t" // _pixTexWidth*=3

      // texel 30
      "movq      %%mm1, %%mm0                  \n\t"
      "paddd     %%mm3, %%mm0                  \n\t"
      "paddd     %%mm3, %%mm0                  \n\t"
      "paddd     %%mm3, %%mm0                  \n\t"
      "psrad     $5, %%mm0                     \n\t"
      "pand      (" ASMSYM(mmBaseMasks) "), %%mm0          \n\t"
      "movq      %%mm0, %%mm7                  \n\t"
      "psrlq     (" ASMSYM(mmBaseWidthShift) "), %%mm7     \n\t"
      "paddd     %%mm7, %%mm0                  \n\t"
      "movd      %%mm0, %%eax                  \n\t"
      "movl      (%%esi, %%eax, 4), %%eax      \n\t"
      "movl      %%eax, (%%edi, %%edx, 4)      \n\t"

      // texel 31
      "movq      %%mm3, %%mm0                  \n\t"
      "pslld     $3, %%mm0                     \n\t"
      "paddd     %%mm3, %%mm0                  \n\t"
      "paddd     %%mm1, %%mm0                  \n\t"
      "paddd     %%mm1, %%mm0                  \n\t"
      "paddd     %%mm1, %%mm0                  \n\t"
      "paddd     %%mm2, %%mm0                  \n\t"
      "paddd     %%mm4, %%mm0                  \n\t"
      "paddd     %%mm4, %%mm0                  \n\t"
      "paddd     %%mm4, %%mm0                  \n\t"
      "psrad     $7, %%mm0                     \n\t"
      "pand      (" ASMSYM(mmBaseMasks) "), %%mm0          \n\t"
      "movq      %%mm0, %%mm7                  \n\t"
      "psrlq     (" ASMSYM(mmBaseWidthShift) "), %%mm7     \n\t"
      "paddd     %%mm7, %%mm0                  \n\t"
      "movd      %%mm0, %%eax                  \n\t"
      "movl      (%%esi, %%eax, 4), %%eax      \n\t"
      "movl      %%eax, 4(%%edi, %%edx, 4)     \n\t"

      // texel 32
      "movq      %%mm4, %%mm0                  \n\t"
      "paddd     %%mm0, %%mm0                  \n\t"
      "paddd     %%mm4, %%mm0                  \n\t"
      "paddd     %%mm3, %%mm0                  \n\t"
      "paddd     %%mm3, %%mm0                  \n\t"
      "paddd     %%mm3, %%mm0                  \n\t"
      "paddd     %%mm2, %%mm0                  \n\t"
      "paddd     %%mm1, %%mm0                  \n\t"
      "psrad     $6, %%mm0                     \n\t"
      "pand      (" ASMSYM(mmBaseMasks) "), %%mm0          \n\t"
      "movq      %%mm0, %%mm7                  \n\t"
      "psrlq     (" ASMSYM(mmBaseWidthShift) "), %%mm7     \n\t"
      "paddd     %%mm7, %%mm0                  \n\t"
      "movd      %%mm0, %%eax                  \n\t"
      "movl      (%%esi, %%eax, 4), %%eax      \n\t"
      "movl      %%eax, 8(%%edi, %%edx, 4)     \n\t"

      // texel 33
      "movq      %%mm4, %%mm0                  \n\t"
      "pslld     $3, %%mm0                     \n\t"
      "paddd     %%mm4, %%mm0                  \n\t"
      "paddd     %%mm1, %%mm0                  \n\t"
      "paddd     %%mm2, %%mm0                  \n\t"
      "paddd     %%mm2, %%mm0                  \n\t"
      "paddd     %%mm2, %%mm0                  \n\t"
      "paddd     %%mm3, %%mm0                  \n\t"
      "paddd     %%mm3, %%mm0                  \n\t"
      "paddd     %%mm3, %%mm0                  \n\t"
      "psrad     $7, %%mm0                     \n\t"
      "pand      (" ASMSYM(mmBaseMasks) "), %%mm0          \n\t"
      "movq      %%mm0, %%mm7                  \n\t"
      "psrlq     (" ASMSYM(mmBaseWidthShift) "), %%mm7     \n\t"
      "paddd     %%mm7, %%mm0                  \n\t"
      "movd      %%mm0, %%eax                  \n\t"
      "movl      (%%esi, %%eax, 4), %%eax      \n\t"
      "movl      %%eax, 12(%%edi, %%edx, 4)    \n\t"

      // advance to next texture pixels
      "paddd     (" ASMSYM(mm1LO) "), %%mm6                \n\t"
      "addl      $16, %%edi                    \n\t"
      "addl      $2, %%ebx                     \n\t"
      "decl      %%ecx                         \n\t"
      "jnz       1b                            \n\t"  // pixLoop4

      // advance to next texture row
      "leal      (%%edi, %%edx, 4), %%edi      \n\t"// +=[_pixTexWidth]*3
      "popl      %%edx                         \n\t"
      "paddd     (" ASMSYM(mm1HI) "), %%mm6                \n\t"
      "decl      %%edx                         \n\t"
      "jnz       0b                            \n\t"  // rowLoop4
      "popl      %%ebx                         \n\t"  // Restore GCC's value.
      "emms                                    \n\t"
        : // no outputs.
        : [pixBaseWidth] "g" (pixBaseWidth),
          [pswHeightMap] "g" (pswHeightMap),
          [pulTextureBase] "g" (pulTextureBase),
          [pulTexture] "g" (pulTexture)
        : FPU_REGS, MMX_REGS, "eax", "ecx", "edx", "esi", "edi",
          "cc", "memory"
    );


  #else
    #error fill in for you platform.
  #endif

#else

    SLONG slU_00, slU_01, slU_10, slU_11;
    SLONG slV_00, slV_01, slV_10, slV_11;
    mmBaseWidthShift = FastLog2( pixBaseWidth);        // faster multiplying with shift
    for( PIX pixV=0; pixV<_pixBufferHeight; pixV++)
    { // row loop
      for( PIX pixU=0; pixU<_pixBufferWidth; pixU++)
      { // texel loop
        slU_00 = pswHeightMap[_pixBufferWidth*0+1] - pswHeightMap[_pixBufferWidth*0+0] + ((pixU+0)<<(DISTORTION+2));
        slV_00 = pswHeightMap[_pixBufferWidth*1+0] - pswHeightMap[_pixBufferWidth*0+0] + ((pixV+0)<<(DISTORTION+2));
        slU_01 = pswHeightMap[_pixBufferWidth*0+2] - pswHeightMap[_pixBufferWidth*0+1] + ((pixU+1)<<(DISTORTION+2));
        slV_01 = pswHeightMap[_pixBufferWidth*1+1] - pswHeightMap[_pixBufferWidth*0+1] + ((pixV+0)<<(DISTORTION+2));
        slU_10 = pswHeightMap[_pixBufferWidth*1+1] - pswHeightMap[_pixBufferWidth*1+0] + ((pixU+0)<<(DISTORTION+2));
        slV_10 = pswHeightMap[_pixBufferWidth*2+0] - pswHeightMap[_pixBufferWidth*1+0] + ((pixV+1)<<(DISTORTION+2));
        slU_11 = pswHeightMap[_pixBufferWidth*1+2] - pswHeightMap[_pixBufferWidth*1+1] + ((pixU+1)<<(DISTORTION+2));
        slV_11 = pswHeightMap[_pixBufferWidth*2+1] - pswHeightMap[_pixBufferWidth*1+1] + ((pixV+1)<<(DISTORTION+2));

        pulTexture[_pixTexWidth*0+0] = PIXEL( (slU_00                                 ) >>(DISTORTION  ), (slV_00                                 ) >>(DISTORTION  ) );
        pulTexture[_pixTexWidth*0+1] = PIXEL( (slU_00* 3+slU_01* 1                    ) >>(DISTORTION+2), (slV_00* 3+slV_01* 1                    ) >>(DISTORTION+2) );
        pulTexture[_pixTexWidth*0+2] = PIXEL( (slU_00   +slU_01                       ) >>(DISTORTION+1), (slV_00   +slV_01                       ) >>(DISTORTION+1) );
        pulTexture[_pixTexWidth*0+3] = PIXEL( (slU_00* 1+slU_01* 3                    ) >>(DISTORTION+2), (slV_00* 1+slV_01* 3                    ) >>(DISTORTION+2) );

        pulTexture[_pixTexWidth*1+0] = PIXEL( (slU_00* 3          +slU_10* 1          ) >>(DISTORTION+2), (slV_00* 3          +slV_10             ) >>(DISTORTION+2) );
        pulTexture[_pixTexWidth*1+1] = PIXEL( (slU_00* 9+slU_01* 3+slU_10* 3+slU_11* 1) >>(DISTORTION+4), (slV_00* 9+slV_01* 3+slV_10* 3+slV_11* 1) >>(DISTORTION+4) );
        pulTexture[_pixTexWidth*1+2] = PIXEL( (slU_00* 3+slU_01* 3+slU_10* 1+slU_11* 1) >>(DISTORTION+3), (slV_00* 3+slV_01* 3+slV_10* 1+slV_11* 1) >>(DISTORTION+3) );
        pulTexture[_pixTexWidth*1+3] = PIXEL( (slU_00* 3+slU_01* 9+slU_10* 1+slU_11* 3) >>(DISTORTION+4), (slV_00* 3+slV_01* 9+slV_10* 1+slV_11* 3) >>(DISTORTION+4) );

        pulTexture[_pixTexWidth*2+0] = PIXEL( (slU_00             +slU_10             ) >>(DISTORTION+1), (slV_00             +slV_10             ) >>(DISTORTION+1) );
        pulTexture[_pixTexWidth*2+1] = PIXEL( (slU_00* 3+slU_01* 1+slU_10* 3+slU_11* 1) >>(DISTORTION+3), (slV_00* 3+slV_01* 1+slV_10* 3+slV_11* 1) >>(DISTORTION+3) );
        pulTexture[_pixTexWidth*2+2] = PIXEL( (slU_00   +slU_01   +slU_10   +slU_11   ) >>(DISTORTION+2), (slV_00   +slV_01   +slV_10   +slV_11   ) >>(DISTORTION+2) );
        pulTexture[_pixTexWidth*2+3] = PIXEL( (slU_00* 1+slU_01* 3+slU_10* 1+slU_11* 3) >>(DISTORTION+3), (slV_00* 1+slV_01* 3+slV_10* 1+slV_11* 3) >>(DISTORTION+3) );

        pulTexture[_pixTexWidth*3+0] = PIXEL( (slU_00* 1          +slU_10* 3          ) >>(DISTORTION+2), (slV_00* 1          +slV_10* 3          ) >>(DISTORTION+2) );
        pulTexture[_pixTexWidth*3+1] = PIXEL( (slU_00* 3+slU_01* 1+slU_10* 9+slU_11* 3) >>(DISTORTION+4), (slV_00* 3+slV_01* 1+slV_10* 9+slV_11* 3) >>(DISTORTION+4) );
        pulTexture[_pixTexWidth*3+2] = PIXEL( (slU_00* 1+slU_01* 1+slU_10* 3+slU_11* 3) >>(DISTORTION+3), (slV_00* 1+slV_01* 1+slV_10* 3+slV_11* 3) >>(DISTORTION+3) );
        pulTexture[_pixTexWidth*3+3] = PIXEL( (slU_00* 1+slU_01* 3+slU_10* 3+slU_11* 9) >>(DISTORTION+4), (slV_00* 1+slV_01* 3+slV_10* 3+slV_11* 9) >>(DISTORTION+4) );

        // advance to next texel
        pulTexture+=4;
        pswHeightMap++;
      }
      pulTexture+=_pixTexWidth*3;
    }

#endif

  }
  else
  { // DO NOTHING
    ASSERTALWAYS( "Effect textures larger than 256 pixels aren't supported");
  }

  _sfStats.StopTimer(CStatForm::STI_EFFECTRENDER);
}
#pragma warning(default: 4731)



/////////////////   Fire


void InitializeFire(void)
{
  Randomize( (ULONG)(_pTimer->GetHighPrecisionTimer().GetMilliseconds()));
}
        
enum PlasmaType {
  ptNormal = 0,
  ptUp,
  ptUpTile,
  ptDown,
  ptDownTile
};

/*******************************
       Plasma Animation
********************************/
static void AnimatePlasma( SLONG slDensity, PlasmaType eType)
{
  _sfStats.StartTimer(CStatForm::STI_EFFECTRENDER);

/////////////////////////////////// move plasma

  UBYTE *pNew = (UBYTE*)_ptdEffect->td_pubBuffer1;
  UBYTE *pOld = (UBYTE*)_ptdEffect->td_pubBuffer2;

  PIX pixV, pixU;
  PIX pixOffset;
  SLONG slLineAbove, slLineBelow, slLineLeft, slLineRight;
  ULONG ulNew;

  // --------------------------
  //        Normal plasma
  // --------------------------
  if (eType == ptNormal) {
    // inner rectangle (without 1 pixel border)
    pixOffset = _pixBufferWidth;
    for( pixV=1; pixV<_pixBufferHeight-1; pixV++) {
      for( pixU=0; pixU<_pixBufferWidth; pixU++) {
        ulNew = ((((ULONG)pOld[pixOffset - _pixBufferWidth] +
                   (ULONG)pOld[pixOffset + _pixBufferWidth] +
                   (ULONG)pOld[pixOffset - 1] +
                   (ULONG)pOld[pixOffset + 1]
                  )>>2) +
                   (ULONG)pOld[pixOffset]
                )>>1;
        pNew[pixOffset] = ulNew - (ulNew >> slDensity);
        pixOffset++;
      }
    }
    // upper horizontal border (without corners)
    slLineAbove = ((_pixBufferHeight-1)*_pixBufferWidth) + 1;
    slLineBelow = _pixBufferWidth + 1;
    slLineLeft = 0;
    slLineRight = 2;
    pixOffset = 1;
    for( pixU=_pixBufferWidth-2; pixU>0; pixU--) {
      ulNew = ((((ULONG)pOld[slLineAbove] +
                 (ULONG)pOld[slLineBelow] +
                 (ULONG)pOld[slLineLeft] +
                 (ULONG)pOld[slLineRight]
                )>>2) +
                 (ULONG)pOld[pixOffset]
              )>>1;
      pNew[pixOffset] = ulNew - (ulNew >> slDensity);
      slLineAbove++;
      slLineBelow++;
      slLineLeft++;
      slLineRight++;
      pixOffset++;
    }
    // lower horizontal border (without corners)
    slLineAbove = ((_pixBufferHeight-2)*_pixBufferWidth) + 1;
    slLineBelow = 1;
    slLineLeft = (_pixBufferHeight-1)*_pixBufferWidth;
    slLineRight = ((_pixBufferHeight-1)*_pixBufferWidth) + 2;
    pixOffset = ((_pixBufferHeight-1)*_pixBufferWidth) + 1;
    for( pixU=_pixBufferWidth-2; pixU>0; pixU--) {
      ulNew = ((((ULONG)pOld[slLineAbove] +
                 (ULONG)pOld[slLineBelow] +
                 (ULONG)pOld[slLineLeft] +
                 (ULONG)pOld[slLineRight]
                )>>2) +
                 (ULONG)pOld[pixOffset]
              )>>1;
      pNew[pixOffset] = ulNew - (ulNew >> slDensity);
      slLineAbove++;
      slLineBelow++;
      slLineLeft++;
      slLineRight++;
      pixOffset++;
    }
    // corner ( 0, 0)
    ulNew = ((((ULONG)pOld[_pixBufferWidth] +
               (ULONG)pOld[(_pixBufferHeight-1)*_pixBufferWidth] +
               (ULONG)pOld[1] +
               (ULONG)pOld[_pixBufferWidth-1]
              )>>2) +
               (ULONG)pOld[0]
            )>>1;
    pNew[0] = ulNew - (ulNew >> slDensity);
    // corner ( 0, _pixBufferWidth)
    ulNew = ((((ULONG)pOld[(2*_pixBufferWidth) - 1] +
               (ULONG)pOld[(_pixBufferHeight*_pixBufferWidth) - 1] +
               (ULONG)pOld[0] +
               (ULONG)pOld[_pixBufferWidth-2]
              )>>2) +
               (ULONG)pOld[_pixBufferWidth-1]
            )>>1;
    pNew[_pixBufferWidth-1] = ulNew - (ulNew >> slDensity);
    // corner ( _pixBufferHeight, 0)
    ulNew = ((((ULONG)pOld[0] +
               (ULONG)pOld[(_pixBufferHeight-2)*_pixBufferWidth] +
               (ULONG)pOld[((_pixBufferHeight-1)*_pixBufferWidth) + 1] +
               (ULONG)pOld[(_pixBufferHeight*_pixBufferWidth) - 1]
              )>>2) +
               (ULONG)pOld[(_pixBufferHeight-1)*_pixBufferWidth]
            )>>1;
    pNew[(_pixBufferHeight-1)*_pixBufferWidth] = ulNew - (ulNew >> slDensity);
    // corner ( _pixBufferHeight, _pixBufferWidth)
    ulNew = ((((ULONG)pOld[_pixBufferWidth-1] +
               (ULONG)pOld[((_pixBufferHeight-1)*_pixBufferWidth) - 1] +
               (ULONG)pOld[(_pixBufferHeight-1)*_pixBufferWidth] +
               (ULONG)pOld[(_pixBufferHeight*_pixBufferWidth) - 2]
              )>>2) +
               (ULONG)pOld[(_pixBufferHeight*_pixBufferWidth) - 1]
            )>>1;
    pNew[(_pixBufferHeight*_pixBufferWidth) - 1] = ulNew - (ulNew >> slDensity);


  // --------------------------
  //      Plasma going up
  // --------------------------
  } else if (eType==ptUp || eType==ptUpTile) {
    // inner rectangle (without 1 pixel border)
    pixOffset = _pixBufferWidth;
    for( pixV=1; pixV<_pixBufferHeight-1; pixV++) {
      for( pixU=0; pixU<_pixBufferWidth; pixU++) {
        ulNew = ((((ULONG)pOld[pixOffset - _pixBufferWidth] +
                   (ULONG)pOld[pixOffset + _pixBufferWidth] +
                   (ULONG)pOld[pixOffset - 1] +
                   (ULONG)pOld[pixOffset + 1]
                  )>>2) +
                   (ULONG)pOld[pixOffset]
                )>>1;
        pNew[pixOffset-_pixBufferWidth] = ulNew - (ulNew >> slDensity);
        pixOffset++;
      }
    }
    // tile
    if (eType==ptUpTile) {
      // upper horizontal border (without corners)
      slLineAbove = ((_pixBufferHeight-1)*_pixBufferWidth) + 1;
      slLineBelow = _pixBufferWidth + 1;
      slLineLeft = 0;
      slLineRight = 2;
      pixOffset = 1;
      for( pixU=_pixBufferWidth-2; pixU>0; pixU--) {
        ulNew = ((((ULONG)pOld[slLineAbove] +
                   (ULONG)pOld[slLineBelow] +
                   (ULONG)pOld[slLineLeft] +
                   (ULONG)pOld[slLineRight]
                  )>>2) +
                   (ULONG)pOld[pixOffset]
                )>>1;
        pNew[slLineAbove] = ulNew - (ulNew >> slDensity);
        slLineAbove++;
        slLineBelow++;
        slLineLeft++;
        slLineRight++;
        pixOffset++;
      }
      // lower horizontal border (without corners)
      slLineAbove = ((_pixBufferHeight-2)*_pixBufferWidth) + 1;
      slLineBelow = 1;
      slLineLeft = (_pixBufferHeight-1)*_pixBufferWidth;
      slLineRight = ((_pixBufferHeight-1)*_pixBufferWidth) + 2;
      pixOffset = ((_pixBufferHeight-1)*_pixBufferWidth) + 1;
      for( pixU=_pixBufferWidth-2; pixU>0; pixU--) {
        ulNew = ((((ULONG)pOld[slLineAbove] +
                   (ULONG)pOld[slLineBelow] +
                   (ULONG)pOld[slLineLeft] +
                   (ULONG)pOld[slLineRight]
                  )>>2) +
                   (ULONG)pOld[pixOffset]
                )>>1;
        pNew[slLineAbove] = ulNew - (ulNew >> slDensity);
        slLineAbove++;
        slLineBelow++;
        slLineLeft++;
        slLineRight++;
        pixOffset++;
      }
      // corner ( 0, 0)
      ulNew = ((((ULONG)pOld[_pixBufferWidth] +
                 (ULONG)pOld[(_pixBufferHeight-1)*_pixBufferWidth] +
                 (ULONG)pOld[1] +
                 (ULONG)pOld[_pixBufferWidth-1]
                )>>2) +
                 (ULONG)pOld[0]
              )>>1;
      pNew[(_pixBufferHeight-1)*_pixBufferWidth] = ulNew - (ulNew >> slDensity);
      // corner ( 0, _pixBufferWidth)
      ulNew = ((((ULONG)pOld[(2*_pixBufferWidth) - 1] +
                 (ULONG)pOld[(_pixBufferHeight*_pixBufferWidth) - 1] +
                 (ULONG)pOld[0] +
                 (ULONG)pOld[_pixBufferWidth-2]
                )>>2) +
                 (ULONG)pOld[_pixBufferWidth-1]
              )>>1;
      pNew[(_pixBufferHeight*_pixBufferWidth) - 1] = ulNew - (ulNew >> slDensity);
      // corner ( _pixBufferHeight, 0)
      ulNew = ((((ULONG)pOld[0] +
                 (ULONG)pOld[(_pixBufferHeight-2)*_pixBufferWidth] +
                 (ULONG)pOld[((_pixBufferHeight-1)*_pixBufferWidth) + 1] +
                 (ULONG)pOld[(_pixBufferHeight*_pixBufferWidth) - 1]
                )>>2) +
                 (ULONG)pOld[(_pixBufferHeight-1)*_pixBufferWidth]
              )>>1;
      pNew[(_pixBufferHeight-2)*_pixBufferWidth] = ulNew - (ulNew >> slDensity);
      // corner ( _pixBufferHeight, _pixBufferWidth)
      ulNew = ((((ULONG)pOld[_pixBufferWidth-1] +
                 (ULONG)pOld[((_pixBufferHeight-1)*_pixBufferWidth) - 1] +
                 (ULONG)pOld[(_pixBufferHeight-1)*_pixBufferWidth] +
                 (ULONG)pOld[(_pixBufferHeight*_pixBufferWidth) - 2]
                )>>2) +
                 (ULONG)pOld[(_pixBufferHeight*_pixBufferWidth) - 1]
              )>>1;
      pNew[((_pixBufferHeight-1)*_pixBufferWidth) - 1] = ulNew - (ulNew >> slDensity);
    }


  // --------------------------
  //     Plasma going down
  // --------------------------
  } else if (eType==ptDown || eType==ptDownTile) {
    // inner rectangle (without 1 pixel border)
    pixOffset = _pixBufferWidth;
    for( pixV=1; pixV<_pixBufferHeight-1; pixV++) {
      for( pixU=0; pixU<_pixBufferWidth; pixU++) {
        ulNew = ((((ULONG)pOld[pixOffset - _pixBufferWidth] +
                   (ULONG)pOld[pixOffset + _pixBufferWidth] +
                   (ULONG)pOld[pixOffset - 1] +
                   (ULONG)pOld[pixOffset + 1]
                  )>>2) +
                   (ULONG)pOld[pixOffset]
                )>>1;
        pNew[pixOffset+_pixBufferWidth] = ulNew - (ulNew >> slDensity);
        pixOffset++;
      }
    }
    // tile
    if (eType==ptDownTile) {
      // upper horizontal border (without corners)
      slLineAbove = ((_pixBufferHeight-1)*_pixBufferWidth) + 1;
      slLineBelow = _pixBufferWidth + 1;
      slLineLeft = 0;
      slLineRight = 2;
      pixOffset = 1;
      for( pixU=_pixBufferWidth-2; pixU>0; pixU--) {
        ulNew = ((((ULONG)pOld[slLineAbove] +
                   (ULONG)pOld[slLineBelow] +
                   (ULONG)pOld[slLineLeft] +
                   (ULONG)pOld[slLineRight]
                  )>>2) +
                   (ULONG)pOld[pixOffset]
                )>>1;
        pNew[slLineBelow] = ulNew - (ulNew >> slDensity);
        slLineAbove++;
        slLineBelow++;
        slLineLeft++;
        slLineRight++;
        pixOffset++;
      }
      // lower horizontal border (without corners)
      slLineAbove = ((_pixBufferHeight-2)*_pixBufferWidth) + 1;
      slLineBelow = 1;
      slLineLeft = (_pixBufferHeight-1)*_pixBufferWidth;
      slLineRight = ((_pixBufferHeight-1)*_pixBufferWidth) + 2;
      pixOffset = ((_pixBufferHeight-1)*_pixBufferWidth) + 1;
      for( pixU=_pixBufferWidth-2; pixU>0; pixU--) {
        ulNew = ((((ULONG)pOld[slLineAbove] +
                   (ULONG)pOld[slLineBelow] +
                   (ULONG)pOld[slLineLeft] +
                   (ULONG)pOld[slLineRight]
                  )>>2) +
                   (ULONG)pOld[pixOffset]
                )>>1;
        pNew[slLineBelow] = ulNew - (ulNew >> slDensity);
        slLineAbove++;
        slLineBelow++;
        slLineLeft++;
        slLineRight++;
        pixOffset++;
      }
      // corner ( 0, 0)
      ulNew = ((((ULONG)pOld[_pixBufferWidth] +
                 (ULONG)pOld[(_pixBufferHeight-1)*_pixBufferWidth] +
                 (ULONG)pOld[1] +
                 (ULONG)pOld[_pixBufferWidth-1]
                )>>2) +
                 (ULONG)pOld[0]
              )>>1;
      pNew[_pixBufferWidth] = ulNew - (ulNew >> slDensity);
      // corner ( 0, _pixBufferWidth)
      ulNew = ((((ULONG)pOld[(2*_pixBufferWidth) - 1] +
                 (ULONG)pOld[(_pixBufferHeight*_pixBufferWidth) - 1] +
                 (ULONG)pOld[0] +
                 (ULONG)pOld[_pixBufferWidth-2]
                )>>2) +
                 (ULONG)pOld[_pixBufferWidth-1]
              )>>1;
      pNew[(2*_pixBufferWidth) - 1] = ulNew - (ulNew >> slDensity);
      // corner ( _pixBufferHeight, 0)
      ulNew = ((((ULONG)pOld[0] +
                 (ULONG)pOld[(_pixBufferHeight-2)*_pixBufferWidth] +
                 (ULONG)pOld[((_pixBufferHeight-1)*_pixBufferWidth) + 1] +
                 (ULONG)pOld[(_pixBufferHeight*_pixBufferWidth) - 1]
                )>>2) +
                 (ULONG)pOld[(_pixBufferHeight-1)*_pixBufferWidth]
              )>>1;
      pNew[0] = ulNew - (ulNew >> slDensity);
      // corner ( _pixBufferHeight, _pixBufferWidth)
      ulNew = ((((ULONG)pOld[_pixBufferWidth-1] +
                 (ULONG)pOld[((_pixBufferHeight-1)*_pixBufferWidth) - 1] +
                 (ULONG)pOld[(_pixBufferHeight-1)*_pixBufferWidth] +
                 (ULONG)pOld[(_pixBufferHeight*_pixBufferWidth) - 2]
                )>>2) +
                 (ULONG)pOld[(_pixBufferHeight*_pixBufferWidth) - 1]
              )>>1;
      pNew[_pixBufferWidth-1] = ulNew - (ulNew >> slDensity);
    }
  }

  // swap buffers
  Swap( _ptdEffect->td_pubBuffer1, _ptdEffect->td_pubBuffer2);

  _sfStats.StopTimer(CStatForm::STI_EFFECTRENDER);
}


/*******************************
       Fire Animation
********************************/
static void AnimateFire( SLONG slDensity)
{
//  _sfStats.StartTimer(CStatForm::STI_EFFECTRENDER);

/////////////////////////////////// move fire

  // use only one buffer (otherwise it's not working)
  UBYTE *pubNew = (UBYTE*)_ptdEffect->td_pubBuffer2;
  SLONG slBufferMask   = _pixBufferWidth*_pixBufferHeight -1;

#if ASMOPT == 1
  SLONG slColumnModulo = _pixBufferWidth*(_pixBufferHeight-2) -1;

 #if (defined __MSVC_INLINE__)
  __asm {
    push    ebx
    mov     edi,D [ulRNDSeed] ;// EDI = randomizer
    mov     esi,D [pubNew]
    xor     ebx,ebx

colLoopFM:
    mov     ecx,D [_pixBufferHeight]
    sub     ecx,2

rowLoopFM:
    mov     edx,D [_pixBufferWidth]
    add     edx,esi
    movzx   eax,B [ebx+ edx]
    add     edx,D [_pixBufferWidth]
    movzx   edx,B [ebx+ edx]
    add     eax,edx
    shr     eax,1
    cmp     eax,D [slDensity]
    jg      doCalc
    mov     B [esi+ebx],0
    jmp     pixDone
doCalc:
    mov     edx,edi
    sar     edx,16
    and     edx,D [slDensity]
    sub     eax,edx
    movsx   edx,B [asbMod3Sub1Table +edx]
    add     edx,ebx
    and     edx,D [slBufferMask]
    mov     B [esi+edx],al
    imul    edi,262147

pixDone:
    // advance to next row
    add     ebx,D [_pixBufferWidth]
    dec     ecx
    jnz     rowLoopFM

    // advance to next column
    sub     ebx,D [slColumnModulo]
    cmp     ebx,D [_pixBufferWidth]
    jl      colLoopFM

    // all done
    mov     D [ulRNDSeed],edi
    pop     ebx
  }

 #elif (defined __GNU_INLINE_X86_32__)
  __asm__ __volatile__ (
    "movl    %[slColumnModulo], %%edx             \n\t"
    "movl    %[slBufferMask], %%ecx               \n\t"
    "movl    %[slDensity], %%eax                  \n\t"
    "movl    (" ASMSYM(ulRNDSeed) "), %%edi       \n\t"

    "pushl   %%ebx                                \n\t"   // GCC's register.
    "xorl    %%ebx, %%ebx                         \n\t"
    "pushl   %%edx                                \n\t"   // slColumnModulo
    "pushl   %%ecx                                \n\t"   // slBufferMask
    "pushl   %%eax                                \n\t"   // slDensity

    "0:                                           \n\t" // colLoopFM
    "movl     (" ASMSYM(_pixBufferHeight) "), %%ecx           \n\t"
    "subl     $2, %%ecx                           \n\t"

    "1:                                           \n\t" // rowLoopFM
    "movl     (" ASMSYM(_pixBufferWidth) "), %%edx            \n\t"
    "addl     %[pubNew], %%edx                    \n\t"
    "movzbl   (%%ebx, %%edx), %%eax               \n\t"
    "addl     (" ASMSYM(_pixBufferWidth) "), %%edx            \n\t"
    "movzbl   (%%ebx, %%edx), %%edx               \n\t"
    "addl     %%edx, %%eax                        \n\t"
    "shrl     $1, %%eax                           \n\t"
    "cmpl     (%%esp), %%eax                      \n\t"
    "jg       doCalc_animateFire                  \n\t"
    "movb     $0, (%[pubNew], %%ebx)              \n\t"
    "jmp      pixDone_animateFire                 \n\t"

    "doCalc_animateFire:                          \n\t"
    "movl     %%edi, %%edx                        \n\t"
    "sarl     $16, %%edx                          \n\t"
    "andl     (%%esp), %%edx                      \n\t"
    "subl     %%edx, %%eax                        \n\t"
    "movsbl   " ASMSYM(asbMod3Sub1Table) "(%%edx), %%edx      \n\t"
    "addl     %%ebx, %%edx                        \n\t"
    "andl     4(%%esp), %%edx                     \n\t"  // slBufferMask
    "movb     %%al, (%[pubNew], %%edx)            \n\t"
    "imull    $262147, %%edi                      \n\t"

    "pixDone_animateFire:                         \n\t"
    // advance to next row
    "addl     (" ASMSYM(_pixBufferWidth) "), %%ebx            \n\t"
    "decl     %%ecx                               \n\t"
    "jnz      1b                                  \n\t"  // rowLoopFM

    // advance to next column
    "subl     8(%%esp), %%ebx                     \n\t"  // slColumnModulo
    "cmpl     (" ASMSYM(_pixBufferWidth) "), %%ebx            \n\t"
    "jl       0b                                  \n\t"  // colLoopFM

    // all done
    "movl     %%edi, (" ASMSYM(ulRNDSeed) ")                  \n\t"
    "addl     $12, %%esp                          \n\t"  // lose our locals.
    "popl     %%ebx                               \n\t"  // Restore GCC's var.
        : // no outputs.
        : [slBufferMask] "g" (slBufferMask),
          [slColumnModulo] "g" (slColumnModulo),
          [pubNew] "r" (pubNew), [slDensity] "g" (slDensity)
        : "eax", "ecx", "edx", "edi", "cc", "memory"
  );

 #else
   #error fill in for you platform.
 #endif

#else

  // inner rectangle (without 1 pixel border)
  for( PIX pixU=0; pixU<_pixBufferWidth; pixU++)
  {
    SLONG slOffset = pixU;
    for( PIX pixV=1; pixV<_pixBufferHeight-1; pixV++)
    {
      ULONG ulNew = ((ULONG)pubNew[_pixBufferWidth+slOffset] + (ULONG)pubNew[_pixBufferWidth*2+slOffset]) >>1;
      if( ulNew>static_cast<ULONG>(slDensity)) {
        ULONG ulNewDensity = RNDW&slDensity;
        ulNew -= ulNewDensity;
        SLONG slDifusion = (SLONG)asbMod3Sub1Table[ulNewDensity]; // (SLONG)(ulNewDensity%3-1);
        SLONG slPos = (slDifusion+slOffset) & slBufferMask;
        pubNew[slPos] = ulNew;
      } else {
        pubNew[slOffset] = 0;
      }
      slOffset += _pixBufferWidth;
    }
  }

#endif

//  _sfStats.StopTimer(CStatForm::STI_EFFECTRENDER);
}

//////////////////////////// displace texture

UBYTE *_pubHeat_RenderPlasmaFire = NULL;

static void RenderPlasmaFire(void)
{
//  _sfStats.StartTimer(CStatForm::STI_EFFECTRENDER);

  // get and adjust textures' parameters
  PIX    pixBaseWidth   = _ptdBase->GetPixWidth();
  ULONG *pulTextureBase = _ptdBase->td_pulFrames;
  ULONG *pulTexture     = _ptdEffect->td_pulFrames;

  ASSERT( _ptdEffect->td_pulFrames!=NULL && _ptdBase->td_pulFrames!=NULL && pixBaseWidth<=256);
  UBYTE *pubHeat = (UBYTE*)_ptdEffect->td_pubBuffer2;  // heat map pointer
  SLONG slHeatMapStep  = _pixBufferWidth/_pixTexWidth;
  SLONG slHeatRowStep  = (slHeatMapStep-1)*_pixBufferWidth;
  SLONG slBaseMipShift = 8 - FastLog2(pixBaseWidth);

#if ASMOPT == 1

 #if (defined __MSVC_INLINE__)
  __asm {
    push    ebx
    mov     ebx,D [pubHeat]
    mov     esi,D [pulTextureBase]
    mov     edi,D [pulTexture]
    mov     ecx,D [_pixTexHeight]
rowLoopF:
    push    ecx
    mov     edx,D [_pixTexWidth]
    mov     ecx,D [slBaseMipShift]
pixLoopF:
    movzx   eax,B [ebx]
    shr     eax,cl
    mov     eax,D [esi+ eax*4]
    mov     D [edi],eax
    // advance to next pixel
    add     ebx,D [slHeatMapStep]
    add     edi,4
    dec     edx
    jnz     pixLoopF
    // advance to next row
    pop     ecx
    add     ebx,D [slHeatRowStep]
    dec     ecx
    jnz     rowLoopF
    pop     ebx
  }
 #elif (defined __GNU_INLINE_X86_32__)
  _pubHeat_RenderPlasmaFire = pubHeat;  // ran out of registers.  :/
  __asm__ __volatile__ (
    "movl    %[slHeatRowStep], %%eax     \n\t"
    "movl    %[slHeatMapStep], %%edx     \n\t"
    "movl    %[slBaseMipShift], %%ecx    \n\t"
    "movl    %[pulTextureBase], %%esi    \n\t"
    "movl    %[pulTexture], %%edi        \n\t"

    "pushl    %%ebx                      \n\t"
    "movl     (" ASMSYM(_pubHeat_RenderPlasmaFire) "),%%ebx \n\t"
    "pushl    %%eax                      \n\t" // slHeatRowStep
    "pushl    %%edx                      \n\t" // slHeatMapStep
    "pushl    %%ecx                      \n\t" // slBaseMipShift
    "movl     (" ASMSYM(_pixTexHeight) "), %%ecx     \n\t"
    "0:                                  \n\t" // rowLoopF
    "pushl    %%ecx                      \n\t"
    "movl     (" ASMSYM(_pixTexWidth) "), %%edx      \n\t"
    "movl     4(%%esp), %%ecx            \n\t" // slBaseMipShift
    "1:                                  \n\t" // pixLoopF
    "movzbl   (%%ebx), %%eax             \n\t"
    "shrl     %%cl, %%eax                \n\t"
    "movl     (%%esi, %%eax, 4), %%eax   \n\t"
    "movl     %%eax, (%%edi)             \n\t"
    // advance to next pixel
    "addl     8(%%esp), %%ebx            \n\t" // slHeatMapStep
    "addl     $4, %%edi                  \n\t"
    "decl     %%edx                      \n\t"
    "jnz      1b                         \n\t" // pixLoopF
    // advance to next row
    "popl     %%ecx                      \n\t"
    "addl     8(%%esp), %%ebx            \n\t" // slHeatRowStep
    "decl     %%ecx                      \n\t"
    "jnz      0b                         \n\t" // rowLoopF
    "addl     $12, %%esp                 \n\t" // lose our locals.
    "popl     %%ebx                      \n\t" // restore GCC's register.
        : // no outputs.
        : [pulTextureBase] "g" (pulTextureBase),
          [pulTexture] "g" (pulTexture),
          [slBaseMipShift] "g" (slBaseMipShift),
          [slHeatRowStep] "g" (slHeatRowStep),
          [slHeatMapStep] "g" (slHeatMapStep)
        : "eax", "ecx", "edx", "esi", "edi", "cc", "memory"
  );

 #else
   #error fill in for you platform.
 #endif

#else

  INDEX iPalette;
  for( INDEX pixV=0; pixV<_pixTexHeight; pixV++) {
    // for every pixel in horizontal line
    for( INDEX pixU=0; pixU<_pixTexWidth; pixU++) {
      iPalette = (*pubHeat)>>slBaseMipShift;
      *pulTexture++ = pulTextureBase[iPalette];
      pubHeat += slHeatMapStep;
    }
    pubHeat += slHeatRowStep;
  }

#endif

//  _sfStats.StopTimer(CStatForm::STI_EFFECTRENDER);
}



/////////////////////////////////////////////////////////////////////
//                      EFFECT TABLES
/////////////////////////////////////////////////////////////////////

struct TextureEffectSourceType atestWater[] = {
  {
    "Raindrops",
    InitializeRaindropsStandard,
    AnimateRaindropsStandard
  },
  {
    "RaindropsBig",
    InitializeRaindropsBig,
    AnimateRaindropsBig
  },
  {
    "RaindropsSmall",
    InitializeRaindropsSmall,
    AnimateRaindropsSmall
  },
  {
    "Random Surfer",
    InitializeRandomSurfer,
    AnimateRandomSurfer
  },
  {
    "Oscilator",
    InitializeOscilator,
    AnimateOscilator
  },
  {
    "Vertical Line",
    InitializeVertLine,
    AnimateVertLine
  },
  {
    "Horizontal Line",
    InitializeHortLine,
    AnimateHortLine
  },
};

struct TextureEffectSourceType atestFire[] = {
  {
    "Point",
    InitializeFirePoint,
    AnimateFirePoint
  },
  {
    "Random Point",
    InitializeRandomFirePoint,
    AnimateRandomFirePoint
  },
  {
    "Shake Point",
    InitializeFireShakePoint,
    AnimateFireShakePoint
  },
  {
    "Fire Place",
    InitializeFirePlace,
    AnimateFirePlace
  },
  {
    "Roler",
    InitializeFireRoler,
    AnimateFireRoler
  },
  {
    "Fall",
    InitializeFireFall,
    AnimateFireFall
  },
  {
    "Fountain",
    InitializeFireFountain,
    AnimateFireFountain
  },
  {
    "Side Fountain",
    InitializeFireSideFountain,
    AnimateFireSideFountain
  },
  {
    "Lightning",
    InitializeFireLightning,
    AnimateFireLightning
  },
  {
    "Lightning Ball",
    InitializeFireLightningBall,
    AnimateFireLightningBall
  },
  {
    "Smoke",
    InitializeFireSmoke,
    AnimateFireSmoke
  },
};


inline void AWaterFast(void)   { AnimateWater(2); };
inline void AWaterMedium(void) { AnimateWater(3); };
inline void AWaterSlow(void)   { AnimateWater(5); };

inline void APlasma(void)         { AnimatePlasma(4, ptNormal);   };
inline void APlasmaUp(void)       { AnimatePlasma(4, ptUp);       };
inline void APlasmaUpTile(void)   { AnimatePlasma(4, ptUpTile);   };
inline void APlasmaDown(void)     { AnimatePlasma(5, ptDown);     };
inline void APlasmaDownTile(void) { AnimatePlasma(5, ptDownTile); };
inline void APlasmaUpSlow(void)   { AnimatePlasma(6, ptUp);       };

inline void AFire(void) { AnimateFire(15); };


struct TextureEffectGlobalType _ategtTextureEffectGlobalPresets[] = {
  {
    "Water Fast",
    InitializeWater,
    AWaterFast,
    sizeof(atestWater)/sizeof(atestWater[0]),
    atestWater
  },
  {
    "Water Medium",
    InitializeWater,
    AWaterMedium,
    sizeof(atestWater)/sizeof(atestWater[0]),
    atestWater
  },
  {
    "Water Slow",
    InitializeWater,
    AWaterSlow,
    sizeof(atestWater)/sizeof(atestWater[0]),
    atestWater
  },
  {
    "",
    InitializeWater,
    AWaterSlow,
    sizeof(atestWater)/sizeof(atestWater[0]),
    atestWater
  },
  {
    "Plasma Tile",
    InitializeFire,
    APlasma,
    sizeof(atestFire)/sizeof(atestFire[0]),
    atestFire
  },
  {
    "Plasma Up",
    InitializeFire,
    APlasmaUp,
    sizeof(atestFire)/sizeof(atestFire[0]),
    atestFire
  },
  {
    "Plasma Up Tile",
    InitializeFire,
    APlasmaUpTile,
    sizeof(atestFire)/sizeof(atestFire[0]),
    atestFire
  },
  {
    "Plasma Down",
    InitializeFire,
    APlasmaDown,
    sizeof(atestFire)/sizeof(atestFire[0]),
    atestFire
  },
  {
    "Plasma Down Tile",
    InitializeFire,
    APlasmaDownTile,
    sizeof(atestFire)/sizeof(atestFire[0]),
    atestFire
  },
  {
    "Plasma Up Slow",
    InitializeFire,
    APlasmaUpSlow,
    sizeof(atestFire)/sizeof(atestFire[0]),
    atestFire
  },
  {
    "Fire",
    InitializeFire,
    AFire,
    sizeof(atestFire)/sizeof(atestFire[0]),
    atestFire
  },
};

INDEX _ctTextureEffectGlobalPresets = sizeof(_ategtTextureEffectGlobalPresets)
                                    / sizeof(_ategtTextureEffectGlobalPresets[0]);


// get effect type (TRUE if water type effect, FALSE if plasma or fire effect)
BOOL CTextureEffectGlobal::IsWater(void)
{
  return( _ategtTextureEffectGlobalPresets[teg_ulEffectType].tegt_Initialize == InitializeWater);
}

// default constructor
CTextureEffectGlobal::CTextureEffectGlobal(CTextureData *ptdTexture, ULONG ulGlobalEffect)
{
  // remember global effect's texture data for cross linking
  teg_ptdTexture = ptdTexture;
  teg_ulEffectType = ulGlobalEffect;
  // init for animating
  _ategtTextureEffectGlobalPresets[teg_ulEffectType].tegt_Initialize();
  // make sure the texture will be updated next time when used
  teg_updTexture.Invalidate();
}

// add new effect source.
void CTextureEffectGlobal::AddEffectSource( ULONG ulEffectSourceType, PIX pixU0, PIX pixV0,
                                            PIX pixU1, PIX pixV1)
{
  CTextureEffectSource* ptesNew = teg_atesEffectSources.New(1);
  ptesNew->Initialize(this, ulEffectSourceType, pixU0, pixV0, pixU1, pixV1);
}

// animate effect texture
void CTextureEffectGlobal::Animate(void)
{
  // if not set yet (funny word construction:)
  if( !bTableSet) {
    // set table for fast modulo 3 minus 1
    for( INDEX i=0; i<256; i++) asbMod3Sub1Table[i]=(SBYTE)((i%3)-1);
    bTableSet = TRUE;
  }

  // setup some internal vars
  _ptdEffect       = teg_ptdTexture;
  _pixBufferWidth  = _ptdEffect->td_pixBufferWidth;
  _pixBufferHeight = _ptdEffect->td_pixBufferHeight;
  _ulBufferMask    = _pixBufferHeight*_pixBufferWidth -1;

  // remember buffer pointers
  _pubDrawBuffer=(UBYTE*)_ptdEffect->td_pubBuffer2;
  _pswDrawBuffer=(SWORD*)_ptdEffect->td_pubBuffer2;
  
  // for each effect source
  FOREACHINDYNAMICARRAY( teg_atesEffectSources, CTextureEffectSource, itEffectSource) {
    // let it animate itself
    itEffectSource->Animate();
  }
  // use animation function for this global effect type
  _ategtTextureEffectGlobalPresets[teg_ulEffectType].tegt_Animate();
  // remember that it was calculated
  teg_updTexture.MarkUpdated();
}

#pragma warning(disable: 4731)
// render effect texture
void CTextureEffectGlobal::Render( INDEX iWantedMipLevel, PIX pixTexWidth, PIX pixTexHeight)
{
  // setup some internal vars
  _ptdEffect = teg_ptdTexture;
  _ptdBase   = teg_ptdTexture->td_ptdBaseTexture;
  _pixBufferWidth  = _ptdEffect->td_pixBufferWidth;
  _pixBufferHeight = _ptdEffect->td_pixBufferHeight;

  if( IsWater()) {
    // use water rendering routine
    _pixTexWidth  = pixTexWidth;
    _pixTexHeight = pixTexHeight;
    _iWantedMipLevel = iWantedMipLevel;
    RenderWater();
  } else {
    // use plasma & fire rendering routine
    _pixTexWidth  = _ptdEffect->GetWidth()  >>iWantedMipLevel;
    _pixTexHeight = _ptdEffect->GetHeight() >>iWantedMipLevel;
    RenderPlasmaFire();
  }
}
#pragma warning(default: 4731)

// returns number of second it took to render effect texture
DOUBLE CTextureEffectGlobal::GetRenderingTime(void)
{
  return( _sfStats.sf_astTimers[CStatForm::STI_EFFECTRENDER].st_tvElapsed.GetSeconds());
}


