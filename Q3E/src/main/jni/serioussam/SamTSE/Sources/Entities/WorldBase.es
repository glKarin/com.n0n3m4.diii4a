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

100
%{
#include "Entities/StdH/StdH.h"
#include "Entities/BackgroundViewer.h"
#include "Entities/WorldSettingsController.h"

#ifdef PLATFORM_UNIX
#define EAX_ENVIRONMENT_LIVINGROOM     0
#define EAX_ENVIRONMENT_STONEROOM      0
#define EAX_ENVIRONMENT_AUDITORIUM     0
#define EAX_ENVIRONMENT_HALLWAY        0
#define EAX_ENVIRONMENT_ARENA          0
#define EAX_ENVIRONMENT_STONECORRIDOR  0
#define EAX_ENVIRONMENT_QUARRY         0
#define EAX_ENVIRONMENT_MOUNTAINS      0
#define EAX_ENVIRONMENT_PLAIN          0
#define EAX_ENVIRONMENT_CAVE           0
#define EAX_ENVIRONMENT_SEWERPIPE      0
#define EAX_ENVIRONMENT_UNDERWATER     0
#endif

%}

uses "Entities/FogMarker";
uses "Entities/HazeMarker";
uses "Entities/MirrorMarker";
uses "Entities/GradientMarker";

// clasification bits
enum ClasificationBits {
 16 CB_00   "Bit 16",
 17 CB_01   "Bit 17",
 18 CB_02   "Bit 18",
 19 CB_03   "Bit 19",
 20 CB_04   "Bit 20",
 21 CB_05   "Bit 21",
 22 CB_06   "Bit 22",
 23 CB_07   "Bit 23",
 24 CB_08   "Bit 24",
 25 CB_09   "Bit 25",
 26 CB_10   "Bit 26",
 27 CB_11   "Bit 27",
 28 CB_12   "Bit 28",
 29 CB_13   "Bit 29",
 30 CB_14   "Bit 30",
 31 CB_15   "Bit 31",
};

// visibility bits
enum VisibilityBits {
  0 VB_00   "Bit 00",
  1 VB_01   "Bit 01",
  2 VB_02   "Bit 02",
  3 VB_03   "Bit 03",
  4 VB_04   "Bit 04",
  5 VB_05   "Bit 05",
  6 VB_06   "Bit 06",
  7 VB_07   "Bit 07",
  8 VB_08   "Bit 08",
  9 VB_09   "Bit 09",
 10 VB_10   "Bit 10",
 11 VB_11   "Bit 11",
 12 VB_12   "Bit 12",
 13 VB_13   "Bit 13",
 14 VB_14   "Bit 14",
 15 VB_15   "Bit 15",
};

%{

inline void Clear(EntityStats &es) {es.es_strName.Clear();};
static CDynamicArray<EntityStats> _aes;
static CAnimObject _aoLightningColor;

EntityStats *FindStats(const CTString &strName)
{
  {FOREACHINDYNAMICARRAY(_aes, EntityStats, ites) {
    EntityStats &es = *ites;
    if (es.es_strName==strName) {
      return &es;
    }
  }}
  return NULL;
}

static void MakeWorldStatistics(void)
{
  // get the world pointer
  CWorld *pwo = _pShell->GetCurrentWorld();
  // if there is no current world
  if (pwo==NULL) {
    CPrintF("No current world.\n");
    return;
  }

  // for each entity in the world
  {FOREACHINDYNAMICCONTAINER(pwo->wo_cenEntities, CEntity, iten) {
    // get its stats
    EntityStats esCurrent;
    BOOL bHasStats = iten->FillEntityStatistics(&esCurrent);
    // if no stats
    if (!bHasStats) {
      // skip it
      continue;
    }

    // find existing stats with same name
    EntityStats *pesOld = FindStats(esCurrent.es_strName);
    // if such stats exists
    if (pesOld!=NULL) {
      // update the existing stats
      pesOld->es_ctCount    += esCurrent.es_ctCount;
      pesOld->es_ctAmmount  += esCurrent.es_ctAmmount*esCurrent.es_ctCount;
      pesOld->es_fValue     += esCurrent.es_fValue*esCurrent.es_ctCount;
      pesOld->es_iScore     += esCurrent.es_iScore*esCurrent.es_ctCount;
    // if this a new name
    } else {
      // create new stats
      EntityStats &esNew = *_aes.New();
      esNew.es_strName    = esCurrent.es_strName;
      esNew.es_ctCount    = esCurrent.es_ctCount;
      esNew.es_ctAmmount  = esCurrent.es_ctAmmount*esCurrent.es_ctCount;
      esNew.es_fValue     = esCurrent.es_fValue*esCurrent.es_ctCount;
      esNew.es_iScore     = esCurrent.es_iScore*esCurrent.es_ctCount;
    }
  }}

  // dump all stats
  try {
    CTFileStream strm;
    CTFileName fnm = CTString("Temp\\Statistics.txt");
    strm.Create_t(fnm);
    CTString strLine;
    strLine.PrintF("%-40s: %8s %8s %10s %10s", 
      "name", "count", "ammount", "health", "score");
    strm.PutLine_t(strLine);
    {FOREACHINDYNAMICARRAY(_aes, EntityStats, ites) {
      EntityStats &es = *ites;
      CTString strLine;
      strLine.PrintF("%-40s: %8d %8d %10g %10d", 
        (const char*)es.es_strName, es.es_ctCount, es.es_ctAmmount, es.es_fValue, es.es_iScore);
      strm.PutLine_t(strLine);
    }}
    CPrintF("Dumped to '%s'\n", (const char*)CTString(fnm));
  } catch (const char *strError) {
    CPrintF("Error: %s\n", (const char*)strError);
  }

  _aes.Clear();
}

static void ReoptimizeAllBrushes(void)
{
  // get the world pointer
  CWorld *pwo = _pShell->GetCurrentWorld();
  // if there is no current world
  if (pwo==NULL) {
    CPrintF("No current world.\n");
    return;
  }

  // for each brush in the world
  FOREACHINDYNAMICARRAY(pwo->wo_baBrushes.ba_abrBrushes, CBrush3D, itbr) {
    CBrush3D &br=*itbr;
    // for each mip in the brush
    FOREACHINLIST(CBrushMip, bm_lnInBrush, itbr->br_lhBrushMips, itbm) {
      // reoptimize it
      itbm->Reoptimize();
    }
  }
  CPrintF("All brushes reoptimized.\n");
}

void SetPyramidPlateActivateAlpha(CWorld *pwo, INDEX iBlending,
                                  TIME tmActivated, TIME tmDeactivated, BOOL bPulsate)
{
  TIME tmNow = _pTimer->CurrentTick();
  TIME tmStop = 2.0f;
  FLOAT fRatio;

  // get alpha
  if( tmNow>tmDeactivated)
  {
    // if plate is deactivated
    if( tmNow>tmDeactivated+tmStop)
    {
      fRatio = 0;
    }
    // if fading out
    else
    {
      fRatio = CalculateRatio(tmNow, tmDeactivated, tmDeactivated+tmStop, 0.0f, 1.0f);
    }
  }
  else if( tmNow>tmActivated)
  {
    // if full visible
    if( tmNow>tmActivated+tmStop)
    {
      fRatio = 1;
    }
    else
    {
      // fade in
      fRatio = CalculateRatio(tmNow, tmActivated, tmActivated+tmStop, 1.0f, 0.0f);
    }
  }
  // not yet activated
  else
  {
    fRatio = 0;
  }
  FLOAT fSinFactor = 1.0f;
  if( bPulsate)
  {
    fSinFactor = Sin((tmNow-tmActivated) * 720.0f)*0.5f+0.5f;
  }
  
  UBYTE ub = UBYTE( fRatio*fSinFactor*255.0f);
  // apply blend or add
  if( pwo->wo_atbTextureBlendings[iBlending].tb_ubBlendingType == STXF_BLEND_ALPHA)
  {
    pwo->wo_atbTextureBlendings[iBlending].tb_colMultiply = C_WHITE|ub;
  }
  else if( pwo->wo_atbTextureBlendings[iBlending].tb_ubBlendingType == STXF_BLEND_ADD)
  {
    pwo->wo_atbTextureBlendings[iBlending].tb_colMultiply = RGBAToColor(ub,ub,ub,255);
  }
}

void SetPyramidMorphRoomAlpha(CWorld *pwo, INDEX iBlending, TIME tmActivated)
{
  TIME tmNow = _pTimer->CurrentTick();
  TIME tmDelta = tmNow-tmActivated;
  FLOAT fRatio;
  FLOAT tmAppear=10.0f;

  if( tmNow<=tmActivated) { return;}
  
  // get alpha
  if( tmNow>=tmActivated+tmAppear)
  {
    fRatio = 1;
  }
  else
  {
    fRatio = CalculateRatio(tmNow, tmActivated, tmActivated+tmAppear, 1.0f, 0.0f);
  }

  FLOAT fSinFactor = Sin(-90+tmDelta*90*(1.0f+tmDelta/tmAppear*4))*0.5f+0.5f;
  //FLOAT fSinFactor = Sin(-90+90*tmDelta)*0.5f+0.5f;
  //UBYTE ub = fSinFactor*255.0f;
  UBYTE ub = UBYTE((fRatio+(1.0f-fRatio)*fSinFactor)*255.0f);

  // apply blend or add
  if( pwo->wo_atbTextureBlendings[iBlending].tb_ubBlendingType == STXF_BLEND_ALPHA)
  {
    pwo->wo_atbTextureBlendings[iBlending].tb_colMultiply = C_WHITE|ub;
  }
  else if( pwo->wo_atbTextureBlendings[iBlending].tb_ubBlendingType == STXF_BLEND_ADD)
  {
    pwo->wo_atbTextureBlendings[iBlending].tb_colMultiply = RGBAToColor(ub,ub,ub,255);
  }
}
    
void CWorldBase_OnWorldInit(CWorld *pwo)
{
  pwo->wo_attTextureTransformations[0].tt_strName = "None";
  pwo->wo_attTextureTransformations[1].tt_strName = "R Extremly Slow";
  pwo->wo_attTextureTransformations[2].tt_strName = "R Very Slow";
  pwo->wo_attTextureTransformations[3].tt_strName = "R Slow";
  pwo->wo_attTextureTransformations[4].tt_strName = "R Medium";
  pwo->wo_attTextureTransformations[5].tt_strName = "R Fast";
  pwo->wo_attTextureTransformations[6].tt_strName = "R Very Fast";
  pwo->wo_attTextureTransformations[7].tt_strName = "R Extremly Fast";

  pwo->wo_attTextureTransformations[8].tt_strName = "Dummy 1";
  pwo->wo_attTextureTransformations[9].tt_strName = "Dummy 2";
  pwo->wo_attTextureTransformations[10].tt_strName = "Dummy 3";

  pwo->wo_attTextureTransformations[11].tt_strName = "Water movement extremly slow";
  pwo->wo_attTextureTransformations[12].tt_strName = "Water movement very slow";
  pwo->wo_attTextureTransformations[13].tt_strName = "Water movement slow";
  pwo->wo_attTextureTransformations[14].tt_strName = "Water movement normal";
  pwo->wo_attTextureTransformations[15].tt_strName = "Water movement fast";
  
  pwo->wo_attTextureTransformations[16].tt_strName = "Stormy sky appearing";

  pwo->wo_attTextureTransformations[17].tt_strName = "Rotation Left 1";
  pwo->wo_attTextureTransformations[18].tt_strName = "Rotation Left 2";
  pwo->wo_attTextureTransformations[19].tt_strName = "Rotation Left 3";
  pwo->wo_attTextureTransformations[20].tt_strName = "Rotation Left 4";
  pwo->wo_attTextureTransformations[21].tt_strName = "Rotation Left 5";
  pwo->wo_attTextureTransformations[22].tt_strName = "Rotation Left 6";
  pwo->wo_attTextureTransformations[23].tt_strName = "Rotation Left 7";
  pwo->wo_attTextureTransformations[24].tt_strName = "Rotation Left 8";
  pwo->wo_attTextureTransformations[25].tt_strName = "Rotation Left 9";
  pwo->wo_attTextureTransformations[26].tt_strName = "Rotation Left 10";

  pwo->wo_attTextureTransformations[27].tt_strName = "Rotation Right 1";
  pwo->wo_attTextureTransformations[28].tt_strName = "Rotation Right 2";
  pwo->wo_attTextureTransformations[29].tt_strName = "Rotation Right 3";
  pwo->wo_attTextureTransformations[30].tt_strName = "Rotation Right 4";
  pwo->wo_attTextureTransformations[31].tt_strName = "Rotation Right 5";
  pwo->wo_attTextureTransformations[32].tt_strName = "Rotation Right 6";
  pwo->wo_attTextureTransformations[33].tt_strName = "Rotation Right 7";
  pwo->wo_attTextureTransformations[34].tt_strName = "Rotation Right 8";
  pwo->wo_attTextureTransformations[35].tt_strName = "Rotation Right 9";
  pwo->wo_attTextureTransformations[36].tt_strName = "Rotation Right 10";

  pwo->wo_attTextureTransformations[37].tt_strName = "D Extremly Slow";
  pwo->wo_attTextureTransformations[38].tt_strName = "D Very Slow";
  pwo->wo_attTextureTransformations[39].tt_strName = "D Slow";
  pwo->wo_attTextureTransformations[40].tt_strName = "D Medium";
  pwo->wo_attTextureTransformations[41].tt_strName = "D Fast";
  pwo->wo_attTextureTransformations[42].tt_strName = "D Very Fast";
  pwo->wo_attTextureTransformations[43].tt_strName = "D Extremly Fast";
  pwo->wo_attTextureTransformations[44].tt_strName = "D Super Fast";
  pwo->wo_attTextureTransformations[45].tt_strName = "D Abnormaly Fast";

// static
  pwo->wo_atbTextureBlendings[0].tb_strName         = "Opaque";
  pwo->wo_atbTextureBlendings[0].tb_ubBlendingType  = STXF_BLEND_OPAQUE;

  pwo->wo_atbTextureBlendings[1].tb_strName         = "Shade";
  pwo->wo_atbTextureBlendings[1].tb_ubBlendingType  = STXF_BLEND_SHADE;

  pwo->wo_atbTextureBlendings[2].tb_strName         = "Blend";
  pwo->wo_atbTextureBlendings[2].tb_ubBlendingType  = STXF_BLEND_ALPHA;

  pwo->wo_atbTextureBlendings[3].tb_strName         = "Add";
  pwo->wo_atbTextureBlendings[3].tb_ubBlendingType  = STXF_BLEND_ADD;
// pulsating
  pwo->wo_atbTextureBlendings[4].tb_strName         = "Shade pulsating";
  pwo->wo_atbTextureBlendings[4].tb_ubBlendingType  = STXF_BLEND_SHADE;
  pwo->wo_atbTextureBlendings[4].tb_colMultiply     = 0x808080FF;

  pwo->wo_atbTextureBlendings[5].tb_strName         = "Blend pulsating full";
  pwo->wo_atbTextureBlendings[5].tb_ubBlendingType  = STXF_BLEND_ALPHA;
  pwo->wo_atbTextureBlendings[5].tb_colMultiply     = C_WHITE|0x80;

  pwo->wo_atbTextureBlendings[6].tb_strName         = "Add pulsating";
  pwo->wo_atbTextureBlendings[6].tb_ubBlendingType  = STXF_BLEND_ADD;
  pwo->wo_atbTextureBlendings[6].tb_colMultiply     = 0x808080FF;

  pwo->wo_atbTextureBlendings[7].tb_strName         = "Blend pulsating half";
  pwo->wo_atbTextureBlendings[7].tb_ubBlendingType  = STXF_BLEND_ALPHA;
  pwo->wo_atbTextureBlendings[7].tb_colMultiply     = C_WHITE|0xC0;

  pwo->wo_atbTextureBlendings[8].tb_strName         = "Stormy sky blend";
  pwo->wo_atbTextureBlendings[8].tb_ubBlendingType  = STXF_BLEND_ALPHA;
  pwo->wo_atbTextureBlendings[8].tb_colMultiply     = C_WHITE|0x00;

  pwo->wo_atbTextureBlendings[9].tb_strName         = "Stormy sky shade";
  pwo->wo_atbTextureBlendings[9].tb_ubBlendingType  = STXF_BLEND_SHADE;
  pwo->wo_atbTextureBlendings[9].tb_colMultiply     = C_WHITE|0xFF;

  pwo->wo_atbTextureBlendings[10].tb_strName         = "Pyramid plate appearing";
  pwo->wo_atbTextureBlendings[10].tb_ubBlendingType  = STXF_BLEND_ALPHA;
  pwo->wo_atbTextureBlendings[10].tb_colMultiply     = C_WHITE|0x00;

  pwo->wo_atbTextureBlendings[11].tb_strName         = "Activated plate 1";
  pwo->wo_atbTextureBlendings[11].tb_ubBlendingType  = STXF_BLEND_ADD;
  pwo->wo_atbTextureBlendings[11].tb_colMultiply     = C_BLACK|CT_OPAQUE;

  pwo->wo_atbTextureBlendings[12].tb_strName         = "Activated plate 2";
  pwo->wo_atbTextureBlendings[12].tb_ubBlendingType  = STXF_BLEND_ADD;
  pwo->wo_atbTextureBlendings[12].tb_colMultiply     = C_BLACK|CT_OPAQUE;

  pwo->wo_atbTextureBlendings[13].tb_strName         = "Activated plate 3";
  pwo->wo_atbTextureBlendings[13].tb_ubBlendingType  = STXF_BLEND_ADD;
  pwo->wo_atbTextureBlendings[13].tb_colMultiply     = C_BLACK|CT_OPAQUE;

  pwo->wo_atbTextureBlendings[14].tb_strName         = "Activated plate 4";
  pwo->wo_atbTextureBlendings[14].tb_ubBlendingType  = STXF_BLEND_ADD;
  pwo->wo_atbTextureBlendings[14].tb_colMultiply     = C_BLACK|CT_OPAQUE;

  pwo->wo_atbTextureBlendings[15].tb_strName         = "Activate pyramid morph room";
  pwo->wo_atbTextureBlendings[15].tb_ubBlendingType  = STXF_BLEND_ALPHA;
  pwo->wo_atbTextureBlendings[15].tb_colMultiply     = C_WHITE|0x00;

  pwo->wo_aitIlluminationTypes[0].it_strName = "None";
  pwo->wo_aitIlluminationTypes[1].it_strName = "Vitraj 1";
  pwo->wo_aitIlluminationTypes[2].it_strName = "Vitraj 2";
  pwo->wo_aitIlluminationTypes[3].it_strName = "Vitraj 3";
  pwo->wo_aitIlluminationTypes[4].it_strName = "Lava 1";
  pwo->wo_aitIlluminationTypes[5].it_strName = "Lava 2";
  pwo->wo_aitIlluminationTypes[6].it_strName = "Lava 3";
  pwo->wo_aitIlluminationTypes[7].it_strName = "Misc 1";
  pwo->wo_aitIlluminationTypes[8].it_strName = "Misc 2";
  pwo->wo_aitIlluminationTypes[9].it_strName = "Misc 3";

  // surfaces

  pwo->wo_astSurfaceTypes[0].st_strName = "Standard";
  pwo->wo_astSurfaceTypes[0].st_fFriction = 1.0f;
  pwo->wo_astSurfaceTypes[0].st_fStairsHeight = 1.0f;
  pwo->wo_astSurfaceTypes[0].st_fJumpSlopeCos = Cos(45.0f);
  pwo->wo_astSurfaceTypes[0].st_fClimbSlopeCos = Cos(45.0f);

  pwo->wo_astSurfaceTypes[1].st_strName = "Ice";
  pwo->wo_astSurfaceTypes[1].st_fFriction = 0.045f;
  pwo->wo_astSurfaceTypes[1].st_fStairsHeight = 1.0f;
  pwo->wo_astSurfaceTypes[1].st_fJumpSlopeCos = Cos(5.0f);
  pwo->wo_astSurfaceTypes[1].st_fClimbSlopeCos = Cos(5.0f);

  pwo->wo_astSurfaceTypes[2].st_strName = "Standard - no step";
  pwo->wo_astSurfaceTypes[2].st_fFriction = 1.0f;
  pwo->wo_astSurfaceTypes[2].st_fStairsHeight = -0.2f;
  pwo->wo_astSurfaceTypes[2].st_fJumpSlopeCos = Cos(10.0f);
  pwo->wo_astSurfaceTypes[2].st_fClimbSlopeCos = Cos(10.0f);

  pwo->wo_astSurfaceTypes[3].st_strName = "Standard - high stairs";
  pwo->wo_astSurfaceTypes[3].st_fFriction = 1.0f;
  pwo->wo_astSurfaceTypes[3].st_fStairsHeight = 2.0f;
  pwo->wo_astSurfaceTypes[3].st_fJumpSlopeCos = Cos(45.0f);
  pwo->wo_astSurfaceTypes[3].st_fClimbSlopeCos = Cos(45.0f);

  pwo->wo_astSurfaceTypes[4].st_strName = "Ice climbable slope";
  pwo->wo_astSurfaceTypes[4].st_fFriction = 0.05f;
  pwo->wo_astSurfaceTypes[4].st_fStairsHeight = 1.0f;
  pwo->wo_astSurfaceTypes[4].st_fJumpSlopeCos = Cos(15.0f);
  pwo->wo_astSurfaceTypes[4].st_fClimbSlopeCos = Cos(15.0f);

  pwo->wo_astSurfaceTypes[5].st_strName = "Ice sliding slope";
  pwo->wo_astSurfaceTypes[5].st_fFriction = 0.001f;
  pwo->wo_astSurfaceTypes[5].st_fStairsHeight = 0.0f;
  pwo->wo_astSurfaceTypes[5].st_fJumpSlopeCos = Cos(5.0f);
  pwo->wo_astSurfaceTypes[5].st_fClimbSlopeCos = Cos(5.0f);

  pwo->wo_astSurfaceTypes[6].st_strName = "Ice less sliding";
  pwo->wo_astSurfaceTypes[6].st_fFriction = 0.06f;
  pwo->wo_astSurfaceTypes[6].st_fStairsHeight = 1.0f;
  pwo->wo_astSurfaceTypes[6].st_fJumpSlopeCos = Cos(5.0f);
  pwo->wo_astSurfaceTypes[6].st_fClimbSlopeCos = Cos(5.0f);

  pwo->wo_astSurfaceTypes[7].st_strName = "Roller coaster";
  pwo->wo_astSurfaceTypes[7].st_fFriction = 0.1f;
  pwo->wo_astSurfaceTypes[7].st_fStairsHeight = 0.0f;
  pwo->wo_astSurfaceTypes[7].st_fJumpSlopeCos = Cos(5.0f);
  pwo->wo_astSurfaceTypes[7].st_fClimbSlopeCos = Cos(45.0f);
  pwo->wo_astSurfaceTypes[7].st_ulFlags = STF_SLIDEDOWNSLOPE;

  pwo->wo_astSurfaceTypes[8].st_strName = "Lava";
  pwo->wo_astSurfaceTypes[8].st_fFriction = 1.0f;
  pwo->wo_astSurfaceTypes[8].st_fStairsHeight = 1.0f;
  pwo->wo_astSurfaceTypes[8].st_fJumpSlopeCos = Cos(45.0f);
  pwo->wo_astSurfaceTypes[8].st_fClimbSlopeCos = Cos(45.0f);
  pwo->wo_astSurfaceTypes[8].st_iWalkDamageType = DMT_BURNING;
  pwo->wo_astSurfaceTypes[8].st_fWalkDamageAmount = 2.0f;
  pwo->wo_astSurfaceTypes[8].st_tmWalkDamageFrequency = 0.5f;
  
  ASSERT(9==SURFACE_SAND);
  pwo->wo_astSurfaceTypes[9].st_strName = "Sand";
  pwo->wo_astSurfaceTypes[9].st_fFriction = 1.0f;
  pwo->wo_astSurfaceTypes[9].st_fStairsHeight = 1.0f;
  pwo->wo_astSurfaceTypes[9].st_fJumpSlopeCos = Cos(45.0f);
  pwo->wo_astSurfaceTypes[9].st_fClimbSlopeCos = Cos(45.0f);

  pwo->wo_astSurfaceTypes[10].st_strName = "Clibamble Slope";
  pwo->wo_astSurfaceTypes[10].st_fFriction = 2.0f;
  pwo->wo_astSurfaceTypes[10].st_fStairsHeight = 1.0f;
  pwo->wo_astSurfaceTypes[10].st_fJumpSlopeCos = Cos(60.0f);
  pwo->wo_astSurfaceTypes[10].st_fClimbSlopeCos = Cos(60.0f);

  pwo->wo_astSurfaceTypes[11].st_strName = "Standard - no impact";
  pwo->wo_astSurfaceTypes[11].st_fFriction = 1.0f;
  pwo->wo_astSurfaceTypes[11].st_fStairsHeight = 1.0f;
  pwo->wo_astSurfaceTypes[11].st_fJumpSlopeCos = Cos(45.0f);
  pwo->wo_astSurfaceTypes[11].st_fClimbSlopeCos = Cos(45.0f);
  pwo->wo_astSurfaceTypes[11].st_ulFlags = STF_NOIMPACT;

  ASSERT(12==SURFACE_WATER);
  pwo->wo_astSurfaceTypes[12].st_strName = "Water";
  pwo->wo_astSurfaceTypes[12].st_fFriction = 1.0f;
  pwo->wo_astSurfaceTypes[12].st_fStairsHeight = 1.0f;
  pwo->wo_astSurfaceTypes[12].st_fJumpSlopeCos = Cos(45.0f);
  pwo->wo_astSurfaceTypes[12].st_fClimbSlopeCos = Cos(45.0f);

  ASSERT(13==SURFACE_RED_SAND);
  pwo->wo_astSurfaceTypes[13].st_strName = "Red sand";
  pwo->wo_astSurfaceTypes[13].st_fFriction = 1.0f;
  pwo->wo_astSurfaceTypes[13].st_fStairsHeight = 1.0f;
  pwo->wo_astSurfaceTypes[13].st_fJumpSlopeCos = Cos(45.0f);
  pwo->wo_astSurfaceTypes[13].st_fClimbSlopeCos = Cos(45.0f);

  // contents
  pwo->wo_actContentTypes[0].ct_strName = "Air";
  pwo->wo_actContentTypes[0].ct_fDensity = 0.0f;
  pwo->wo_actContentTypes[0].ct_fFluidFriction     = 0.0f;
  pwo->wo_actContentTypes[0].ct_fControlMultiplier = 1.0f;
  pwo->wo_actContentTypes[0].ct_fSpeedMultiplier = 1.0f;
  pwo->wo_actContentTypes[0].ct_fDrowningDamageAmount = 10.0f;
  pwo->wo_actContentTypes[0].ct_tmDrowningDamageDelay = 1.0f;
  pwo->wo_actContentTypes[0].ct_ulFlags = CTF_FLYABLE|CTF_BREATHABLE_LUNGS;

  pwo->wo_actContentTypes[1].ct_strName = "Water";
  pwo->wo_actContentTypes[1].ct_fDensity = 1000.0f;
  pwo->wo_actContentTypes[1].ct_fFluidFriction     = 0.0f;
  pwo->wo_actContentTypes[1].ct_fControlMultiplier = 2.0f;
  pwo->wo_actContentTypes[1].ct_fSpeedMultiplier = 0.75f;
  pwo->wo_actContentTypes[1].ct_fDrowningDamageAmount = 10.0f;
  pwo->wo_actContentTypes[1].ct_tmDrowningDamageDelay = 1.0f;
  pwo->wo_actContentTypes[1].ct_ulFlags = CTF_BREATHABLE_GILLS|CTF_SWIMABLE|CTF_FADESPINNING;

  pwo->wo_actContentTypes[2].ct_strName = "Lava";
  pwo->wo_actContentTypes[2].ct_fDensity = 800.0f;
  pwo->wo_actContentTypes[2].ct_fFluidFriction     = 1.0f;
  pwo->wo_actContentTypes[2].ct_fControlMultiplier = 2.0f;
  pwo->wo_actContentTypes[2].ct_fSpeedMultiplier = 0.5f;
  pwo->wo_actContentTypes[2].ct_fDrowningDamageAmount = 0.0f;
  pwo->wo_actContentTypes[2].ct_tmDrowningDamageDelay = 1.0f;
  pwo->wo_actContentTypes[2].ct_iSwimDamageType = DMT_BURNING;
  pwo->wo_actContentTypes[2].ct_fSwimDamageAmount = 50.0f;
  pwo->wo_actContentTypes[2].ct_tmSwimDamageFrequency = 0.25f;
  pwo->wo_actContentTypes[2].ct_ulFlags = CTF_FADESPINNING;

  pwo->wo_actContentTypes[3].ct_strName = "Cold Water";
  pwo->wo_actContentTypes[3].ct_fDensity = 1000.0f;
  pwo->wo_actContentTypes[3].ct_fFluidFriction     = 0.0f;
  pwo->wo_actContentTypes[3].ct_fControlMultiplier = 2.0f;
  pwo->wo_actContentTypes[3].ct_fSpeedMultiplier = 0.75f;
  pwo->wo_actContentTypes[3].ct_fDrowningDamageAmount = 10.0f;
  pwo->wo_actContentTypes[3].ct_tmDrowningDamageDelay = 1.0f;
  pwo->wo_actContentTypes[3].ct_iSwimDamageType = DMT_FREEZING;
  pwo->wo_actContentTypes[3].ct_fSwimDamageAmount = 1.0f;
  pwo->wo_actContentTypes[3].ct_tmSwimDamageDelay = 5.0f;
  pwo->wo_actContentTypes[3].ct_tmSwimDamageFrequency = 1.0f;
  pwo->wo_actContentTypes[3].ct_ulFlags = CTF_BREATHABLE_GILLS|CTF_SWIMABLE|CTF_FADESPINNING;

  pwo->wo_actContentTypes[4].ct_strName = "Spikes";
  pwo->wo_actContentTypes[4].ct_fDensity = 500.0f;
  pwo->wo_actContentTypes[4].ct_fFluidFriction     = 0.5f;
  pwo->wo_actContentTypes[4].ct_fControlMultiplier = 1.0f;
  pwo->wo_actContentTypes[4].ct_fSpeedMultiplier = 0.75f;
  pwo->wo_actContentTypes[4].ct_iKillDamageType = DMT_SPIKESTAB;
  pwo->wo_actContentTypes[4].ct_fKillImmersion = 0.5f;
  pwo->wo_actContentTypes[4].ct_ulFlags = CTF_BREATHABLE_LUNGS|CTF_FADESPINNING;

  pwo->wo_actContentTypes[5].ct_strName = "Desert heat";
  pwo->wo_actContentTypes[5].ct_fDensity = 0.0f;
  pwo->wo_actContentTypes[5].ct_fFluidFriction     = 0.0f;
  pwo->wo_actContentTypes[5].ct_fControlMultiplier = 1.0f;
  pwo->wo_actContentTypes[5].ct_fSpeedMultiplier = 1.0f;
  pwo->wo_actContentTypes[5].ct_iSwimDamageType = DMT_HEAT;
  pwo->wo_actContentTypes[5].ct_fSwimDamageAmount = 10.0f;
  pwo->wo_actContentTypes[5].ct_tmSwimDamageDelay = 10.0f;
  pwo->wo_actContentTypes[5].ct_tmSwimDamageFrequency = 2.0f;
  pwo->wo_actContentTypes[5].ct_ulFlags = CTF_FLYABLE|CTF_BREATHABLE_LUNGS;

  // environments
  pwo->wo_aetEnvironmentTypes[ 0].et_strName = "Normal";
  pwo->wo_aetEnvironmentTypes[ 0].et_iType = 1;
  pwo->wo_aetEnvironmentTypes[ 0].et_fSize = 1.4f;

  pwo->wo_aetEnvironmentTypes[ 1].et_strName = "Generic";
  pwo->wo_aetEnvironmentTypes[ 1].et_iType = 0;
  pwo->wo_aetEnvironmentTypes[ 1].et_fSize = 7.5f;

  pwo->wo_aetEnvironmentTypes[ 2].et_strName = "Small room";
  pwo->wo_aetEnvironmentTypes[ 2].et_iType = EAX_ENVIRONMENT_LIVINGROOM;
  pwo->wo_aetEnvironmentTypes[ 2].et_fSize = 2.5f;

  pwo->wo_aetEnvironmentTypes[ 3].et_strName = "Medium room";
  pwo->wo_aetEnvironmentTypes[ 3].et_iType = EAX_ENVIRONMENT_STONEROOM;
  pwo->wo_aetEnvironmentTypes[ 3].et_fSize = 11.6f;

  pwo->wo_aetEnvironmentTypes[ 4].et_strName = "Big room";
  pwo->wo_aetEnvironmentTypes[ 4].et_iType = EAX_ENVIRONMENT_AUDITORIUM;
  pwo->wo_aetEnvironmentTypes[ 4].et_fSize = 21.6f;

  pwo->wo_aetEnvironmentTypes[ 5].et_strName = "Corridor";
  pwo->wo_aetEnvironmentTypes[ 5].et_iType = EAX_ENVIRONMENT_HALLWAY;
  pwo->wo_aetEnvironmentTypes[ 5].et_fSize = 1.8f;

  pwo->wo_aetEnvironmentTypes[ 6].et_strName = "Arena";
  pwo->wo_aetEnvironmentTypes[ 6].et_iType = EAX_ENVIRONMENT_ARENA;
  pwo->wo_aetEnvironmentTypes[ 6].et_fSize = 36.2f;

  pwo->wo_aetEnvironmentTypes[ 7].et_strName = "Long corridor";
  pwo->wo_aetEnvironmentTypes[ 7].et_iType = EAX_ENVIRONMENT_STONECORRIDOR;
  pwo->wo_aetEnvironmentTypes[ 7].et_fSize = 13.5f;

  pwo->wo_aetEnvironmentTypes[ 8].et_strName = "Small canyon";
  pwo->wo_aetEnvironmentTypes[ 8].et_iType = EAX_ENVIRONMENT_QUARRY;
  pwo->wo_aetEnvironmentTypes[ 8].et_fSize = 17.5f;

  pwo->wo_aetEnvironmentTypes[ 9].et_strName = "Big canyon";
  pwo->wo_aetEnvironmentTypes[ 9].et_iType = EAX_ENVIRONMENT_MOUNTAINS;
  pwo->wo_aetEnvironmentTypes[ 9].et_fSize = 100.0f;

  pwo->wo_aetEnvironmentTypes[10].et_strName = "Open space";
  pwo->wo_aetEnvironmentTypes[10].et_iType = EAX_ENVIRONMENT_PLAIN;
  pwo->wo_aetEnvironmentTypes[10].et_fSize = 42.5f;
                                
  pwo->wo_aetEnvironmentTypes[11].et_strName = "Cave";
  pwo->wo_aetEnvironmentTypes[11].et_iType = EAX_ENVIRONMENT_CAVE;
  pwo->wo_aetEnvironmentTypes[11].et_fSize = 14.6f;
                                
  pwo->wo_aetEnvironmentTypes[12].et_strName = "Sewers";
  pwo->wo_aetEnvironmentTypes[12].et_iType = EAX_ENVIRONMENT_SEWERPIPE;
  pwo->wo_aetEnvironmentTypes[12].et_fSize = 1.7f;
                                
  pwo->wo_aetEnvironmentTypes[13].et_strName = "Underwater";
  pwo->wo_aetEnvironmentTypes[13].et_iType = EAX_ENVIRONMENT_UNDERWATER;
  pwo->wo_aetEnvironmentTypes[13].et_fSize = 1.8f;

  // declare console variables
  _pShell->DeclareSymbol("user void MakeWorldStatistics(void);", (void*) &MakeWorldStatistics);
  _pShell->DeclareSymbol("user void ReoptimizeAllBrushes(void);", (void*) &ReoptimizeAllBrushes);
}

void CWorldBase_OnWorldRender(CWorld *pwo)
{
  // get current tick
  TIME tmNow = _pTimer->GetLerpedCurrentTick();
  // wrap time to prevent texture coordinates to get unprecise
  tmNow = fmod(tmNow, 600.0f); // (wrap every 10 minutes)

// transformations
  // right
  pwo->wo_attTextureTransformations[1].tt_mdTransformation.md_fUOffset= 128/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[2].tt_mdTransformation.md_fUOffset= 256/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[3].tt_mdTransformation.md_fUOffset= 512/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[4].tt_mdTransformation.md_fUOffset=1024/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[5].tt_mdTransformation.md_fUOffset=2048/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[6].tt_mdTransformation.md_fUOffset=4096/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[7].tt_mdTransformation.md_fUOffset=8192/1024.0f*tmNow;
  
  // down
  pwo->wo_attTextureTransformations[37].tt_mdTransformation.md_fVOffset= 128/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[38].tt_mdTransformation.md_fVOffset= 256/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[39].tt_mdTransformation.md_fVOffset= 512/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[40].tt_mdTransformation.md_fVOffset=1024/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[41].tt_mdTransformation.md_fVOffset=2048/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[42].tt_mdTransformation.md_fVOffset=4096/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[43].tt_mdTransformation.md_fVOffset=8192/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[44].tt_mdTransformation.md_fVOffset=8192*2/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[45].tt_mdTransformation.md_fVOffset=8192*4/1024.0f*tmNow;

  CMappingDefinitionUI mdui;
  mdui.mdui_fUStretch = 1.0f;
  mdui.mdui_fVStretch = 1.0f;
  mdui.mdui_fUOffset = 0.0f;
  mdui.mdui_fVOffset = 0.0f;
  // rotations left
  mdui.mdui_aURotation = 8192*1/1024.0f*tmNow;
  mdui.mdui_aVRotation = 8192*1/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[17].tt_mdTransformation.FromUI(mdui);
  mdui.mdui_aURotation = 8192*2/1024.0f*tmNow;
  mdui.mdui_aVRotation = 8192*2/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[18].tt_mdTransformation.FromUI(mdui);
  mdui.mdui_aURotation = 8192*4/1024.0f*tmNow;
  mdui.mdui_aVRotation = 8192*4/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[19].tt_mdTransformation.FromUI(mdui);
  mdui.mdui_aURotation = 8192*8/1024.0f*tmNow;
  mdui.mdui_aVRotation = 8192*8/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[20].tt_mdTransformation.FromUI(mdui);
  mdui.mdui_aURotation = 8192*16/1024.0f*tmNow;
  mdui.mdui_aVRotation = 8192*16/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[21].tt_mdTransformation.FromUI(mdui);
  mdui.mdui_aURotation = 8192*32/1024.0f*tmNow;
  mdui.mdui_aVRotation = 8192*32/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[22].tt_mdTransformation.FromUI(mdui);
  mdui.mdui_aURotation = 8192*64/1024.0f*tmNow;
  mdui.mdui_aVRotation = 8192*64/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[23].tt_mdTransformation.FromUI(mdui);
  mdui.mdui_aURotation = 8192*128/1024.0f*tmNow;
  mdui.mdui_aVRotation = 8192*128/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[24].tt_mdTransformation.FromUI(mdui);
  mdui.mdui_aURotation = 8192*256/1024.0f*tmNow;
  mdui.mdui_aVRotation = 8192*256/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[25].tt_mdTransformation.FromUI(mdui);
  // rotations right
  mdui.mdui_aURotation = -8192*1/1024.0f*tmNow;
  mdui.mdui_aVRotation = -8192*1/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[27].tt_mdTransformation.FromUI(mdui);
  mdui.mdui_aURotation = -8192*2/1024.0f*tmNow;
  mdui.mdui_aVRotation = -8192*2/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[28].tt_mdTransformation.FromUI(mdui);
  mdui.mdui_aURotation = -8192*4/1024.0f*tmNow;
  mdui.mdui_aVRotation = -8192*4/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[29].tt_mdTransformation.FromUI(mdui);
  mdui.mdui_aURotation = -8192*8/1024.0f*tmNow;
  mdui.mdui_aVRotation = -8192*8/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[30].tt_mdTransformation.FromUI(mdui);
  mdui.mdui_aURotation = -8192*16/1024.0f*tmNow;
  mdui.mdui_aVRotation = -8192*16/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[31].tt_mdTransformation.FromUI(mdui);
  mdui.mdui_aURotation = -8192*32/1024.0f*tmNow;
  mdui.mdui_aVRotation = -8192*32/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[32].tt_mdTransformation.FromUI(mdui);
  mdui.mdui_aURotation = -8192*64/1024.0f*tmNow;
  mdui.mdui_aVRotation = -8192*64/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[33].tt_mdTransformation.FromUI(mdui);
  mdui.mdui_aURotation = -8192*128/1024.0f*tmNow;
  mdui.mdui_aVRotation = -8192*128/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[34].tt_mdTransformation.FromUI(mdui);
  mdui.mdui_aURotation = -8192*256/1024.0f*tmNow;
  mdui.mdui_aVRotation = -8192*256/1024.0f*tmNow;
  pwo->wo_attTextureTransformations[35].tt_mdTransformation.FromUI(mdui);
// blendings
  FLOAT f = Abs(Sin(tmNow*AngleDeg(180.0f)));
  pwo->wo_atbTextureBlendings[4].tb_colMultiply = RGBAToColor(f*255, f*255, f*255, 255);
  pwo->wo_atbTextureBlendings[5].tb_colMultiply = C_WHITE|UBYTE(255*f);
  pwo->wo_atbTextureBlendings[6].tb_colMultiply = RGBAToColor(f*255, f*255, f*255, 255);
  pwo->wo_atbTextureBlendings[7].tb_colMultiply = C_WHITE|UBYTE(255*Lerp(0.5f, 1.0f, f));

  pwo->wo_attTextureTransformations[11].tt_mdTransformation.md_fUOffset=Sin( tmNow*22)/30;
  pwo->wo_attTextureTransformations[11].tt_mdTransformation.md_fVOffset=Cos( tmNow*26)/35;

  pwo->wo_attTextureTransformations[12].tt_mdTransformation.md_fUOffset=Sin( tmNow*32)/10;
  pwo->wo_attTextureTransformations[12].tt_mdTransformation.md_fVOffset=Cos( tmNow*22)/15;

  pwo->wo_attTextureTransformations[13].tt_mdTransformation.md_fUOffset=Sin( tmNow*15)/7;
  pwo->wo_attTextureTransformations[13].tt_mdTransformation.md_fVOffset=Cos( tmNow*25)/8;

  pwo->wo_attTextureTransformations[14].tt_mdTransformation.md_fUOffset=Sin( tmNow*32)/3;
  pwo->wo_attTextureTransformations[14].tt_mdTransformation.md_fVOffset=Cos( tmNow*22)/3;

  pwo->wo_attTextureTransformations[15].tt_mdTransformation.md_fUOffset=Sin( tmNow*15);
  pwo->wo_attTextureTransformations[15].tt_mdTransformation.md_fVOffset=Cos( tmNow*25);

  // ----------- Obtain world settings controller
  CWorldSettingsController *pwsc = NULL;
  // obtain bcg viewer
  CBackgroundViewer *penBcgViewer = (CBackgroundViewer *) pwo->GetBackgroundViewer();
  if( penBcgViewer != NULL)
  {
    // obtain world settings controller 
    pwsc = (CWorldSettingsController *) penBcgViewer->m_penWorldSettingsController.ep_pen;
  }
  
  // ***** Storm effects
  // if world settings controller is valid
  if( pwsc != NULL)
  {
    FLOAT fStormFactor = pwsc->GetStormFactor();
    // set alpha value
    UBYTE ubA = UBYTE(255.0f*fStormFactor);
    pwo->wo_atbTextureBlendings[8].tb_colMultiply = C_WHITE|ubA;

    // set world polygon shading multiplier
    UBYTE ubShd = UBYTE(64+191*(1.0f-fStormFactor));
    COLOR colShade = RGBToColor(ubShd,ubShd,ubShd)|CT_OPAQUE;

    // apply lightning FX
    if( pwsc->m_tmLightningStart != -1)
    {
      FLOAT tmLightningLife = tmNow-pwsc->m_tmLightningStart;
      _aoLightningColor.ao_tmAnimStart = pwsc->m_tmLightningStart;
      COLOR colLightning = _aoLightningColor.GetFrame();
      // calculate lightning power factor
      UBYTE ub = UBYTE( pwsc->m_fLightningPower*255);
      COLOR colFactor = RGBToColor( ub, ub, ub) | CT_OPAQUE;
      colLightning = MulColors( colLightning, colFactor);
      colShade = AddColors( colShade, colLightning);
    }
    pwo->wo_atbTextureBlendings[9].tb_colMultiply = colShade;
  }

  // ***** Pyramid blending effects
  // if world settings controller is valid
  if( pwsc != NULL)
  {
    // set alpha values for switch-controlled pyramid plate activating
    SetPyramidPlateActivateAlpha(pwo, 10, pwsc->m_tmPyramidPlatesStart, 1e6, FALSE);
    SetPyramidPlateActivateAlpha(pwo, 11, pwsc->m_tmActivatedPlate1, pwsc->m_tmDeactivatedPlate1, TRUE);
    SetPyramidPlateActivateAlpha(pwo, 12, pwsc->m_tmActivatedPlate2, pwsc->m_tmDeactivatedPlate2, TRUE);
    SetPyramidPlateActivateAlpha(pwo, 13, pwsc->m_tmActivatedPlate3, pwsc->m_tmDeactivatedPlate3, TRUE);
    SetPyramidPlateActivateAlpha(pwo, 14, pwsc->m_tmActivatedPlate4, pwsc->m_tmDeactivatedPlate4, TRUE);
    // pyramid morph room
    SetPyramidMorphRoomAlpha(pwo, 15, pwsc->m_tmPyramidMorphRoomActivated);
  }
};

void CWorldBase_OnInitClass(void)
{
  // init particle effects
  InitParticles();
  try
  {
    // setup simple model shadow texture
    _toSimpleModelShadow.SetData_t( CTFILENAME("Textures\\Effects\\Shadow\\SimpleModelShadow.tex"));
    _aoLightningColor.SetData_t( CTFILENAME("Animations\\Lightning.ani"));
    // we will use first animation in .ani file for lightning
    _aoLightningColor.PlayAnim(0, AOF_NORESTART);
  }
  catch (const char *strError)
  {
    FatalError(TRANS("Cannot load shadow texture: \n%s"), strError);
  }
}

void CWorldBase_OnEndClass(void)
{
  // close particle effects
  CloseParticles();
}

class CFixedForce {
public:
  CTString ff_strName;
  class CForceStrength ff_fsGravity;
  class CForceStrength ff_fsField;
  
  inline CFixedForce(CTString strName,
    FLOAT3D vDirection,
    FLOAT fAcceleration,
    FLOAT fVelocity)
  {
    ff_strName = strName;
    ff_fsGravity.fs_vDirection     =vDirection;
    ff_fsGravity.fs_fAcceleration  =fAcceleration;
    ff_fsGravity.fs_fVelocity      =fVelocity;
  }

};

static CFixedForce affFixedForces[] = 
{
  CFixedForce ("Normal D", FLOAT3D( 0,-1, 0), 30, 70),
  CFixedForce ("Normal U", FLOAT3D( 0,+1, 0), 30, 70),
  CFixedForce ("Normal N", FLOAT3D( 0, 0,-1), 30, 70),
  CFixedForce ("Normal S", FLOAT3D( 0, 0,+1), 30, 70),
  CFixedForce ("Normal E", FLOAT3D(-1, 0, 0), 30, 70),
  CFixedForce ("Normal W", FLOAT3D(+1, 0, 0), 30, 70),
  CFixedForce ("ZeroG",    FLOAT3D(+1, 0, 0), 0, 0),
  CFixedForce ("Unused",   FLOAT3D(+1, 0, 0), 0, 0),
  CFixedForce ("Unused",   FLOAT3D(+1, 0, 0), 0, 0),
  CFixedForce ("Unused",   FLOAT3D(+1, 0, 0), 0, 0),
};
static INDEX ctFixedForces = ARRAYCOUNT(affFixedForces);
extern void GetDefaultForce(INDEX iForce, const FLOAT3D &vPoint, 
    CForceStrength &fsGravity, CForceStrength &fsField)
{
  if (iForce<ctFixedForces) {
    fsGravity = affFixedForces[iForce].ff_fsGravity;
  }
  fsField.fs_fAcceleration = 0;
  fsField.fs_fVelocity = 0;
}

%}

class CWorldBase : CEntity {
name      "WorldBase";
thumbnail "Thumbnails\\WorldBase.tbn";
features  "HasName", "HasDescription", 
  "ImplementsOnWorldRender", "ImplementsOnWorldInit",
  "ImplementsOnInitClass", "ImplementsOnEndClass";

properties:
  1 CTString m_strName "Name" 'N' ="World Base",
  3 CTString m_strDescription = "",
  2 BOOL m_bZoning     "Zoning"     'Z' =FALSE,
  4 BOOL m_bBackground "Background" 'B' =FALSE,
  5 BOOL m_bAnchored   "Anchored"   'A' =FALSE,

  10 CEntityPointer m_penGravity0 "Gravity 0" 'G',
  11 CEntityPointer m_penGravity1 "Gravity 1",
  12 CEntityPointer m_penGravity2 "Gravity 2",
  13 CEntityPointer m_penGravity3 "Gravity 3",
  14 CEntityPointer m_penGravity4 "Gravity 4",
  15 CEntityPointer m_penGravity5 "Gravity 5",
  16 CEntityPointer m_penGravity6 "Gravity 6",
  17 CEntityPointer m_penGravity7 "Gravity 7",
  18 CEntityPointer m_penGravity8 "Gravity 8",
  19 CEntityPointer m_penGravity9 "Gravity 9",

  20 CEntityPointer m_penFog0 "Fog 0" 'F',
  21 CEntityPointer m_penFog1 "Fog 1",
  22 CEntityPointer m_penFog2 "Fog 2",
  23 CEntityPointer m_penFog3 "Fog 3",
  24 CEntityPointer m_penFog4 "Fog 4",

  30 CEntityPointer m_penHaze0 "Haze 0" 'H',
  31 CEntityPointer m_penHaze1 "Haze 1",
  32 CEntityPointer m_penHaze2 "Haze 2",
  33 CEntityPointer m_penHaze3 "Haze 3",
  34 CEntityPointer m_penHaze4 "Haze 4",

  40 CEntityPointer m_penMirror0 "Mirror 0" 'M',
  41 CEntityPointer m_penMirror1 "Mirror 1",
  42 CEntityPointer m_penMirror2 "Mirror 2",
  43 CEntityPointer m_penMirror3 "Mirror 3",
  44 CEntityPointer m_penMirror4 "Mirror 4",

  50 CEntityPointer m_penGradient0 "Gradient 0" 'R',
  51 CEntityPointer m_penGradient1 "Gradient 1",
  52 CEntityPointer m_penGradient2 "Gradient 2",
  53 CEntityPointer m_penGradient3 "Gradient 3",
  54 CEntityPointer m_penGradient4 "Gradient 4",
  55 CEntityPointer m_penGradient5 "Gradient 5",
  56 CEntityPointer m_penGradient6 "Gradient 6",
  57 CEntityPointer m_penGradient7 "Gradient 7",
  58 CEntityPointer m_penGradient8 "Gradient 8",
  59 CEntityPointer m_penGradient9 "Gradient 9",
  60 CEntityPointer m_penGradient10 "Gradient 10",
  61 CEntityPointer m_penGradient11 "Gradient 11",
  62 CEntityPointer m_penGradient12 "Gradient 12",
  63 CEntityPointer m_penGradient13 "Gradient 13",
  64 CEntityPointer m_penGradient14 "Gradient 14",
  65 CEntityPointer m_penGradient15 "Gradient 15",
  66 CEntityPointer m_penGradient16 "Gradient 16",
  67 CEntityPointer m_penGradient17 "Gradient 17",
  68 CEntityPointer m_penGradient18 "Gradient 18",
  69 CEntityPointer m_penGradient19 "Gradient 19",

  80 flags ClasificationBits m_cbClassificationBits "Clasification bits" 'C' = 0,
  81 flags VisibilityBits m_vbVisibilityBits "Visibility bits" 'V' = 0,

components:
functions:
  // get visibility tweaking bits
  ULONG GetVisTweaks(void)
  {
    return m_cbClassificationBits|m_vbVisibilityBits;
  }

  // Validate offered target for one property
  BOOL IsTargetValid(SLONG slPropertyOffset, CEntity *penTarget)
  {
    if(penTarget==NULL)
    {
      return FALSE;
    }
    
    ULONG ulFirst, ulLast;
    CTString strClass;

    // if gradient marker
    ulFirst = offsetof(CWorldBase, m_penGradient0);
    ulLast  = offsetof(CWorldBase, m_penGradient19);
    strClass = "Gradient Marker";
    if( (static_cast<ULONG>(slPropertyOffset) >= ulFirst) && (static_cast<ULONG>(slPropertyOffset) <=ulLast) ) {
      return (IsDerivedFromClass(penTarget, strClass));
    }

    // if gravity marker
    ulFirst = offsetof(CWorldBase, m_penGravity0);
    ulLast  = offsetof(CWorldBase, m_penGravity9);
    if( (static_cast<ULONG>(slPropertyOffset) >= ulFirst) && (static_cast<ULONG>(slPropertyOffset) <=ulLast) ) {
      return 
        IsDerivedFromClass(penTarget, "Gravity Marker")||
        IsDerivedFromClass(penTarget, "Gravity Router");
    }

    // if mirror marker
    ulFirst = offsetof(CWorldBase, m_penMirror0);
    ulLast  = offsetof(CWorldBase, m_penMirror4);
    strClass = "Mirror Marker";
    if( (static_cast<ULONG>(slPropertyOffset) >= ulFirst) && (static_cast<ULONG>(slPropertyOffset) <=ulLast) ) {
      return (IsDerivedFromClass(penTarget, strClass));
    }

    // if fog marker
    ulFirst = offsetof(CWorldBase, m_penFog0);
    ulLast  = offsetof(CWorldBase, m_penFog4);
    strClass = "Fog Marker";
    if( (static_cast<ULONG>(slPropertyOffset) >= ulFirst) && (static_cast<ULONG>(slPropertyOffset) <=ulLast) ) {
      return (IsDerivedFromClass(penTarget, strClass));
    }

    // if haze marker
    ulFirst = offsetof(CWorldBase, m_penHaze0);
    ulLast  = offsetof(CWorldBase, m_penHaze4);
    strClass = "Haze Marker";
    if( (static_cast<ULONG>(slPropertyOffset) >= ulFirst) && (static_cast<ULONG>(slPropertyOffset) <=ulLast) ) {
      return (IsDerivedFromClass(penTarget, strClass));
    }

    return CEntity::IsTargetValid(slPropertyOffset, penTarget);
  }

  /* Get force type name, return empty string if not used. */
  const CTString &GetForceName(INDEX iForce)
  {
    static const CTString strDummyName("");
    static const CTString strMarkerUnused("Marker not set");
    INDEX ctGravityMarkers = &m_penGravity9-&m_penGravity0+1;

    if (iForce<ctFixedForces) {
      return affFixedForces[iForce].ff_strName;
    } else {
      if (iForce-ctFixedForces<=ctGravityMarkers){
        CEntity *pen = (&m_penGravity0)[iForce-ctFixedForces].ep_pen;
        if (pen != NULL) {
          return pen->GetForceName(0);
        } else {
          return strMarkerUnused;
        }

      }
      return strDummyName;
    }
  }
  /* Get force in given point. */
  void GetForce(INDEX iForce, const FLOAT3D &vPoint, 
    CForceStrength &fsGravity, CForceStrength &fsField)
  {
    INDEX ctGravityMarkers = &m_penGravity9-&m_penGravity0+1;
    if (iForce<ctFixedForces) {
      fsGravity = affFixedForces[iForce].ff_fsGravity;
    } else {
      if (iForce-ctFixedForces<ctGravityMarkers){
        CEntity *pen = (&m_penGravity0)[iForce-ctFixedForces].ep_pen;
        if (pen != NULL) {
          pen->GetForce(0, vPoint, fsGravity, fsField);
          return;
        }
      }
      fsGravity.fs_fAcceleration = 30;
      fsGravity.fs_fVelocity = 70;
      fsGravity.fs_vDirection = FLOAT3D(1,0,0);
    }
    fsField.fs_fAcceleration = 0;
    fsField.fs_fVelocity = 0;
  }

  /* Get entity that controls the force, used for change notification checking. */
  CEntity *GetForceController(INDEX iForce)
  {
    INDEX ctGravityMarkers = &m_penGravity9-&m_penGravity0+1;
    if (iForce<ctFixedForces) {
      return NULL;
    } else {
      if (iForce-ctFixedForces<ctGravityMarkers){
        CEntity *pen = (&m_penGravity0)[iForce-ctFixedForces].ep_pen;
        if (pen != NULL) {
          return pen->GetForceController(0);
        }
      }
    }
    return NULL;
  }
  
  /* Get fog type name, return empty string if not used. */
  const CTString &GetFogName(INDEX iFog)
  {
    INDEX ctFogMarkers = &m_penFog4-&m_penFog0+1;
    static const CTString strDummyName("");
    static const CTString strMarkerUnused("Marker not set");
    if (iFog<ctFogMarkers){
      CFogMarker *pfm = (CFogMarker *)(&m_penFog0)[iFog].ep_pen;
      if (pfm != NULL) {
        return pfm->GetFogName();
      } else {
        return strMarkerUnused;
      }
    }
    return strDummyName;
  }
  /* Get fog, return FALSE for none. */
  BOOL GetFog(INDEX iFog, class CFogParameters &fpFog)
  {
    INDEX ctFogMarkers = &m_penFog4-&m_penFog0+1;
    if (iFog<ctFogMarkers){
      CFogMarker *pfm = (CFogMarker *)(&m_penFog0)[iFog].ep_pen;
      if (pfm != NULL) {
        pfm->GetFog(fpFog);
        return TRUE;
      }
    }
    return FALSE;
  }
  
  /* Get haze type name, return empty string if not used. */
  const CTString &GetHazeName(INDEX iHaze)
  {
    INDEX ctHazeMarkers = &m_penHaze4-&m_penHaze0+1;
    static const CTString strDummyName("");
    static const CTString strMarkerUnused("Marker not set");
    if (iHaze<ctHazeMarkers){
      CHazeMarker *pfm = (CHazeMarker *)(&m_penHaze0)[iHaze].ep_pen;
      if (pfm != NULL) {
        return pfm->GetHazeName();
      } else {
        return strMarkerUnused;
      }
    }
    return strDummyName;
  }
  
  /* Get haze, return FALSE for none. */
  BOOL GetHaze(INDEX iHaze, class CHazeParameters &hpHaze, FLOAT3D &vViewDir)
  {
    INDEX ctHazeMarkers = &m_penHaze4-&m_penHaze0+1;
    if (iHaze<ctHazeMarkers){
      CHazeMarker *phm = (CHazeMarker *)(&m_penHaze0)[iHaze].ep_pen;
      if (phm != NULL) {
        phm->GetHaze(hpHaze, vViewDir);
        return TRUE;
      }
    }
    return FALSE;
  }

  /* Get mirror type name, return empty string if not used. */
  const CTString &GetMirrorName(INDEX iMirror)
  {
    static const CTString strDummyName("");
    static const CTString strMarkerUnused("Marker not set");
    if (iMirror==0) {
      return strDummyName;
    }

    switch (iMirror) {
    case 1: { static const CTString str("std mirror 1"); return str; }; break;
    case 2: { static const CTString str("std mirror 2"); return str; }; break;
    case 3: { static const CTString str("std mirror 3"); return str; }; break;
    case 4: { static const CTString str("std mirror 4"); return str; }; break;
    case 5: { static const CTString str("std mirror 5"); return str; }; break;
    case 6: { static const CTString str("std mirror 6"); return str; }; break;
    case 7: { static const CTString str("std mirror 7"); return str; }; break;
    case 8: { static const CTString str("std mirror 8"); return str; }; break;
    default: {
      iMirror-=9;
      INDEX ctMirrorMarkers = &m_penMirror4-&m_penMirror0+1;
      if (iMirror<ctMirrorMarkers){
        CMirrorMarker *pfm = (CMirrorMarker *)(&m_penMirror0)[iMirror].ep_pen;
        if (pfm != NULL) {
          return pfm->GetMirrorName();
        } else {
          return strMarkerUnused;
        }
      }
             }
    }
    return strDummyName;
  }

  /* Get mirror, return FALSE for none. */
  BOOL GetMirror(INDEX iMirror, class CMirrorParameters &mpMirror)
  {
    if (iMirror==0) {
      return FALSE;
    }
    if (iMirror>=1 && iMirror<=8) {
      mpMirror.mp_ulFlags = 0;
      return TRUE;
    }
    iMirror-=9;
    INDEX ctMirrorMarkers = &m_penMirror4-&m_penMirror0+1;
    if (iMirror<ctMirrorMarkers){
      CMirrorMarker *pmm = (CMirrorMarker *)(&m_penMirror0)[iMirror].ep_pen;
      if (pmm != NULL) {
        pmm->GetMirror(mpMirror);
        return TRUE;
      }
    }
    return FALSE;
  }
  
  /* Get gradient type name, return empty string if not used. */
  const CTString &GetGradientName(INDEX iGradient)
  {
    INDEX ctGradientMarkers = &m_penGradient19 - &m_penGradient0 + 1;
    static const CTString strDummyName("");
    static const CTString strMarkerUnused("Marker not set");
    if (iGradient < ctGradientMarkers)
	{
      CGradientMarker *pgm = (CGradientMarker *)(&m_penGradient0)[iGradient].ep_pen;
      if (pgm != NULL) {
        return pgm->GetGradientName();
      } else {
        return strMarkerUnused;
      }
    }
    return strDummyName;
  }
  /* Uncache shadows for given gradient */
  void UncacheShadowsForGradient(class CGradientMarker *penDiscard)
  {
    INDEX ctGradientMarkers = &m_penGradient19-&m_penGradient0+1;
    for( INDEX iGradient=0; iGradient < ctGradientMarkers; iGradient++)
    {
      CGradientMarker *pgm = (CGradientMarker *)(&m_penGradient0)[iGradient].ep_pen;
      if(pgm == penDiscard)
      {
        CEntity::UncacheShadowsForGradient( iGradient+1);
      }
    }
  }

  /* Get gradient, return FALSE for none. */
  BOOL GetGradient(INDEX iGradient, class CGradientParameters &fpGradient)
  {
    INDEX ctGradientMarkers = &m_penGradient19 - &m_penGradient0 + 1;
    if ( (iGradient <= ctGradientMarkers) && (iGradient > 0) ){
      CGradientMarker *pgm = (CGradientMarker *)(&m_penGradient0)[iGradient-1].ep_pen;
      if (pgm != NULL) {
        return pgm->GetGradient(0, fpGradient);
      }
    }
    return FALSE;
  }
  
  /* Handle an event, return false if the event is not handled. */
  BOOL HandleEvent(const CEntityEvent &ee)
  {
    // when someone in range is destroyed
    if (ee.ee_slEvent==EVENTCODE_EFirstWorldBase) {
      SetFlags(GetFlags()|ENF_ZONING);
      m_bZoning = TRUE;
      SetFlags(GetFlags()|ENF_ANCHORED);
      m_bAnchored = TRUE;
      return TRUE;
    }
    return FALSE;
  }

procedures:
  Main(EVoid evoid)
  {
    // declare yourself as a brush
    InitAsBrush();
    SetPhysicsFlags(EPF_BRUSH_FIXED);
    SetCollisionFlags(ECF_BRUSH);
    
    // set zoning
    if( m_bZoning) {
      m_strDescription = "zoning";
      SetFlags(GetFlags()|ENF_ZONING);
    } else {
      m_strDescription = "non zoning";
      SetFlags(GetFlags()&~ENF_ZONING);
    }

    // set background
    if( m_bBackground) {
      m_strDescription += " background";
      SetFlags(GetFlags()|ENF_BACKGROUND);
    } else {
      SetFlags(GetFlags()&~ENF_BACKGROUND);
    }

    // set anchor for SEd
    if( m_bAnchored) {
      m_strDescription += " anchored";
      SetFlags(GetFlags()|ENF_ANCHORED);
    } else {
      SetFlags(GetFlags()&~ENF_ANCHORED);
    }

    return;
  }
};
