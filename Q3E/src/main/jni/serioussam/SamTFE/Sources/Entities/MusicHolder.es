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

222
%{
#include "Entities/StdH/StdH.h"
#include "Entities/EnemyBase.h"
#include "Entities/EnemySpawner.h"
#include "Entities/Trigger.h"
#include "Entities/Light.h"
#include "Entities/Common/LightFixes.h"
%}

enum MusicType {
  0 MT_LIGHT  "light",
  1 MT_MEDIUM "medium",
  2 MT_HEAVY  "heavy",
  3 MT_EVENT  "event",
  4 MT_CONTINUOUS  "continuous",
};

event EChangeMusic {
  enum MusicType mtType,
  CTFileName fnMusic,
  FLOAT fVolume,
  BOOL bForceStart,
};

%{
#define MUSIC_VOLUMEMIN   0.02f     // minimum volume (considered off)
#define MUSIC_VOLUMEMAX   0.98f     // maximum volume (considered full)

float FadeInFactor(TIME fFadeTime)
{
  return (float) pow(MUSIC_VOLUMEMAX/MUSIC_VOLUMEMIN, 1/(fFadeTime/_pTimer->TickQuantum));
}
float FadeOutFactor(TIME fFadeTime)
{
  return (float) pow(MUSIC_VOLUMEMIN/MUSIC_VOLUMEMAX, 1/(fFadeTime/_pTimer->TickQuantum));
}
%}

class CMusicHolder : CRationalEntity {
name      "MusicHolder";
thumbnail "Thumbnails\\MusicHolder.tbn";
features "HasName", "IsTargetable", "IsImportant";

properties:
  1 CTString m_strName     "" = "MusicHolder",
  2 FLOAT m_fScoreMedium "Score Medium" = 100.0f,
  3 FLOAT m_fScoreHeavy  "Score Heavy"  = 1000.0f,

 10 CTFileName m_fnMusic0 "Music Light" 'M' = CTFILENAME(""),
 11 CTFileName m_fnMusic1 "Music Medium"    = CTFILENAME(""),
 12 CTFileName m_fnMusic2 "Music Heavy"     = CTFILENAME(""),
 13 CTFileName m_fnMusic3                   = CTFILENAME(""),  // event music
 14 CTFileName m_fnMusic4                   = CTFILENAME(""),  // continuous music

 20 FLOAT m_fVolume0  "Volume Light" 'V' = 1.0f,
 21 FLOAT m_fVolume1  "Volume Medium"    = 1.0f,
 22 FLOAT m_fVolume2  "Volume Heavy"     = 1.0f,
 23 FLOAT m_fVolume3                     = 1.0f,  // event volume
 24 FLOAT m_fVolume4                     = 1.0f,  // continuous volume

// internals

100 CEntityPointer m_penBoss,    // current boss if any
102 CEntityPointer m_penCounter,   // enemy counter for wave-fight progress display
104 INDEX m_ctEnemiesInWorld = 0,   // count of total enemies in world
105 CEntityPointer m_penRespawnMarker,    // respawn marker for coop
106 INDEX m_ctSecretsInWorld = 0,   // count of total secrets in world
101 FLOAT m_tmFade = 1.0f,    // music cross-fade speed
103 enum MusicType m_mtCurrentMusic = MT_LIGHT, // current active channel

// for cross-fade purposes
110 FLOAT m_fCurrentVolume0a  = 1.0f,
210 FLOAT m_fCurrentVolume0b  = 1.0f,
111 FLOAT m_fCurrentVolume1a  = 1.0f,
211 FLOAT m_fCurrentVolume1b  = 1.0f,
112 FLOAT m_fCurrentVolume2a  = 1.0f,
212 FLOAT m_fCurrentVolume2b  = 1.0f,
113 FLOAT m_fCurrentVolume3a  = 1.0f,
213 FLOAT m_fCurrentVolume3b  = 1.0f,
114 FLOAT m_fCurrentVolume4a  = 1.0f,
214 FLOAT m_fCurrentVolume4b  = 1.0f,

// the music channels
120 CSoundObject m_soMusic0a,
220 CSoundObject m_soMusic0b,
121 CSoundObject m_soMusic1a,
221 CSoundObject m_soMusic1b,
122 CSoundObject m_soMusic2a,
222 CSoundObject m_soMusic2b,
123 CSoundObject m_soMusic3a,
223 CSoundObject m_soMusic3b,
124 CSoundObject m_soMusic4a,
224 CSoundObject m_soMusic4b,

// next free subchannel markers (all starts at subchannel 1(b), first switch goes to subchannel 0(a))
130 INDEX m_iSubChannel0 = 1,
131 INDEX m_iSubChannel1 = 1,
132 INDEX m_iSubChannel2 = 1,
133 INDEX m_iSubChannel3 = 1,
134 INDEX m_iSubChannel4 = 1,

  {
    // array of enemies that make fuss
    CDynamicContainer<CEntity> m_cenFussMakers;
  }

components:
  1 model   MODEL_MARKER      "Models\\Editor\\MusicHolder.mdl",
  2 texture TEXTURE_MARKER    "Models\\Editor\\MusicHolder.tex",
  3 class CLASS_LIGHT         "Classes\\Light.ecl"

functions:

  //***************************************************************
  //****************  Fix Textures on some levels  ****************
  //***************************************************************

  void ClearLights(void)
  {
    {FOREACHINDYNAMICCONTAINER(_pNetwork->ga_World.wo_cenEntities, CEntity, pen) {
      if(IsDerivedFromClass(pen, "Light")) {
        if(((CLight&)*pen).m_strName == "fix_texture"){
          pen->Destroy();
        }
      }
    }}
  }

  void FixTexturesValleyOfTheKings(void) 
  {
    ClearLights();
    CEntity *pen = NULL;
    CPlacement3D pl;
    for(int i = 0; i < 4; i++) {
      FLOAT m_fCoord1 = _fValleyOfTheKingsCoordinates[i][0];
      FLOAT m_fCoord2 = _fValleyOfTheKingsCoordinates[i][1];
      FLOAT m_fCoord3 = _fValleyOfTheKingsCoordinates[i][2];
      pl = CPlacement3D(FLOAT3D(m_fCoord1, m_fCoord2, m_fCoord3), ANGLE3D(0, 0, 0));
      pen = CreateEntity(pl, CLASS_LIGHT);
      pen->Initialize();
      ((CLight&)*pen).m_colColor = C_GRAY;
      ((CLight&)*pen).m_ltType = LT_POINT;
      ((CLight&)*pen).m_bDarkLight = TRUE;
      ((CLight&)*pen).m_rFallOffRange = 8.0f;
      ((CLight&)*pen).m_strName = "fix_texture";
      pen->en_ulSpawnFlags =0xFFFFFFFF;
      pen->Reinitialize();
    }    
  }

  void FixTexturesDunes(void) 
  {
    ClearLights();
    CEntity *pen = NULL;
    CPlacement3D pl;
    for(int i = 0; i < 8; i++) {
      FLOAT m_fCoord1 = _fDunesCoordinates[i][0];
      FLOAT m_fCoord2 = _fDunesCoordinates[i][1];
      FLOAT m_fCoord3 = _fDunesCoordinates[i][2];
      pl = CPlacement3D(FLOAT3D(m_fCoord1, m_fCoord2, m_fCoord3), ANGLE3D(0, 0, 0));
      pen = CreateEntity(pl, CLASS_LIGHT);
      pen->Initialize();
      ((CLight&)*pen).m_colColor = C_GRAY;
      ((CLight&)*pen).m_ltType = LT_POINT;
      ((CLight&)*pen).m_bDarkLight = TRUE;
      ((CLight&)*pen).m_rFallOffRange = 8.0f;
      ((CLight&)*pen).m_strName = "fix_texture";
      pen->en_ulSpawnFlags =0xFFFFFFFF;
      pen->Reinitialize();
    }
  }

  void FixTexturesSuburbs(void) 
  {
    ClearLights();
    CEntity *pen = NULL;
    CPlacement3D pl;
    for(int i = 0; i < 21; i++) {
      FLOAT m_fCoord1 = _fSuburbsCoordinates[i][0];
      FLOAT m_fCoord2 = _fSuburbsCoordinates[i][1];
      FLOAT m_fCoord3 = _fSuburbsCoordinates[i][2];
      pl = CPlacement3D(FLOAT3D(m_fCoord1, m_fCoord2, m_fCoord3), ANGLE3D(0, 0, 0));
      pen = CreateEntity(pl, CLASS_LIGHT);
      pen->Initialize();
      ((CLight&)*pen).m_colColor = C_GRAY;
      ((CLight&)*pen).m_ltType = LT_POINT;
      ((CLight&)*pen).m_bDarkLight = TRUE;
      ((CLight&)*pen).m_rFallOffRange = 8.0f;
      ((CLight&)*pen).m_strName = "fix_texture";
      pen->en_ulSpawnFlags =0xFFFFFFFF;
      pen->Reinitialize();
    }
  }

  void FixTexturesMetropolis(void) 
  {
    ClearLights();
    CEntity *pen = NULL;
    CPlacement3D pl;
    FLOAT m_fCoord1 = _fMetropolisCoordinates[0][0];
    FLOAT m_fCoord2 = _fMetropolisCoordinates[0][1];
    FLOAT m_fCoord3 = _fMetropolisCoordinates[0][2];
    pl = CPlacement3D(FLOAT3D(m_fCoord1, m_fCoord2, m_fCoord3), ANGLE3D(0, 0, 0));
      pen = CreateEntity(pl, CLASS_LIGHT);
      pen->Initialize();
      ((CLight&)*pen).m_colColor = C_GRAY;
      ((CLight&)*pen).m_ltType = LT_POINT;
      ((CLight&)*pen).m_bDarkLight = TRUE;
      ((CLight&)*pen).m_rFallOffRange = 8.0f;
      ((CLight&)*pen).m_strName = "fix_texture";
      pen->en_ulSpawnFlags =0xFFFFFFFF;
      pen->Reinitialize();
  }

  void FixTexturesAlleyOfSphinxes(void) 
  {
    ClearLights();
    CEntity *pen = NULL;
    CPlacement3D pl;
    for(int i = 0; i < 37; i++) {
      FLOAT m_fCoord1 = _fAlleyOfSphinxesCoordinates[i][0];
      FLOAT m_fCoord2 = _fAlleyOfSphinxesCoordinates[i][1];
      FLOAT m_fCoord3 = _fAlleyOfSphinxesCoordinates[i][2];
      pl = CPlacement3D(FLOAT3D(m_fCoord1, m_fCoord2, m_fCoord3), ANGLE3D(0, 0, 0));
      pen = CreateEntity(pl, CLASS_LIGHT);
      pen->Initialize();
      ((CLight&)*pen).m_colColor = C_GRAY;
      ((CLight&)*pen).m_ltType = LT_POINT;
      ((CLight&)*pen).m_bDarkLight = TRUE;
      ((CLight&)*pen).m_rFallOffRange = 8.0f;
      ((CLight&)*pen).m_strName = "fix_texture";
      pen->en_ulSpawnFlags =0xFFFFFFFF;
      pen->Reinitialize();
    } 
  }

  void FixTexturesKarnak(void) 
  {
    ClearLights();
    CEntity *pen = NULL;
    CPlacement3D pl;
    for(int i = 0; i < 41; i++) {
      FLOAT m_fCoord1 = _fKarnakCoordinates[i][0];
      FLOAT m_fCoord2 = _fKarnakCoordinates[i][1];
      FLOAT m_fCoord3 = _fKarnakCoordinates[i][2];
      pl = CPlacement3D(FLOAT3D(m_fCoord1, m_fCoord2, m_fCoord3), ANGLE3D(0, 0, 0));
      pen = CreateEntity(pl, CLASS_LIGHT);
      pen->Initialize();
      ((CLight&)*pen).m_colColor = C_GRAY;
      ((CLight&)*pen).m_ltType = LT_POINT;
      ((CLight&)*pen).m_bDarkLight = TRUE;
      ((CLight&)*pen).m_rFallOffRange = 8.0f;
      ((CLight&)*pen).m_strName = "fix_texture";
      pen->en_ulSpawnFlags =0xFFFFFFFF;
      pen->Reinitialize();
    }
    FLOAT m_fCoord1 = _fKarnakCoordinates[41][0];
    FLOAT m_fCoord2 = _fKarnakCoordinates[41][1];
    FLOAT m_fCoord3 = _fKarnakCoordinates[41][2];
    pl = CPlacement3D(FLOAT3D(m_fCoord1, m_fCoord2, m_fCoord3), ANGLE3D(0, 0, 0));
    pen = CreateEntity(pl, CLASS_LIGHT);
    pen->Initialize();
    ((CLight&)*pen).m_colColor = C_GRAY;
    ((CLight&)*pen).m_ltType = LT_POINT;
    ((CLight&)*pen).m_bDarkLight = TRUE;
    ((CLight&)*pen).m_rFallOffRange = 4.0f;
    ((CLight&)*pen).m_strName = "fix_texture";
    pen->en_ulSpawnFlags =0xFFFFFFFF;
    pen->Reinitialize();
  }

  void FixTexturesLuxor(void) 
  {
    ClearLights();
    CEntity *pen = NULL;
    CPlacement3D pl;
    for(int i = 0; i < 51; i++) {
      FLOAT m_fCoord1 = _fLuxorCoordinates[i][0];
      FLOAT m_fCoord2 = _fLuxorCoordinates[i][1];
      FLOAT m_fCoord3 = _fLuxorCoordinates[i][2];
      pl = CPlacement3D(FLOAT3D(m_fCoord1, m_fCoord2, m_fCoord3), ANGLE3D(0, 0, 0));
      pen = CreateEntity(pl, CLASS_LIGHT);
      pen->Initialize();
      ((CLight&)*pen).m_colColor = C_GRAY;
      ((CLight&)*pen).m_ltType = LT_POINT;
      ((CLight&)*pen).m_bDarkLight = TRUE;
      ((CLight&)*pen).m_rFallOffRange = 8.0f;
      ((CLight&)*pen).m_strName = "fix_texture";
      pen->en_ulSpawnFlags =0xFFFFFFFF;
      pen->Reinitialize();
    }
    FLOAT m_fCoord1 = _fLuxorCoordinates[51][0];
    FLOAT m_fCoord2 = _fLuxorCoordinates[51][1];
    FLOAT m_fCoord3 = _fLuxorCoordinates[51][2];
    pl = CPlacement3D(FLOAT3D(m_fCoord1, m_fCoord2, m_fCoord3), ANGLE3D(0, 0, 0));
    pen = CreateEntity(pl, CLASS_LIGHT);
    pen->Initialize();
    ((CLight&)*pen).m_colColor = C_GRAY;
    ((CLight&)*pen).m_ltType = LT_POINT;
    ((CLight&)*pen).m_bDarkLight = TRUE;
    ((CLight&)*pen).m_rFallOffRange = 1.0f;
    ((CLight&)*pen).m_strName = "fix_texture";
    pen->en_ulSpawnFlags =0xFFFFFFFF;
    pen->Reinitialize();
  }

  void FixTexturesSacredYards(void) 
  {
    ClearLights();
    CEntity *pen = NULL;
    CPlacement3D pl;
    for(int i = 0; i < 27; i++) {
      FLOAT m_fCoord1 = _fSacredYardsCoordinates[i][0];
      FLOAT m_fCoord2 = _fSacredYardsCoordinates[i][1];
      FLOAT m_fCoord3 = _fSacredYardsCoordinates[i][2];
      pl = CPlacement3D(FLOAT3D(m_fCoord1, m_fCoord2, m_fCoord3), ANGLE3D(0, 0, 0));
      pen = CreateEntity(pl, CLASS_LIGHT);
      pen->Initialize();
      ((CLight&)*pen).m_colColor = C_GRAY;
      ((CLight&)*pen).m_ltType = LT_POINT;
      ((CLight&)*pen).m_bDarkLight = TRUE;
      ((CLight&)*pen).m_rFallOffRange = 8.0f;
      ((CLight&)*pen).m_strName = "fix_texture";
      pen->en_ulSpawnFlags =0xFFFFFFFF;
      pen->Reinitialize();
    }
  }

  void FixTexturesKarnakDemo(void) 
  {
    ClearLights();
    CEntity *pen = NULL;
    CPlacement3D pl;
    for(int i = 0; i < 49; i++) {
      FLOAT m_fCoord1 = _fKarnakDemoCoordinates[i][0];
      FLOAT m_fCoord2 = _fKarnakDemoCoordinates[i][1];
      FLOAT m_fCoord3 = _fKarnakDemoCoordinates[i][2];
      pl = CPlacement3D(FLOAT3D(m_fCoord1, m_fCoord2, m_fCoord3), ANGLE3D(0, 0, 0));
      pen = CreateEntity(pl, CLASS_LIGHT);
      pen->Initialize();
      ((CLight&)*pen).m_colColor = C_GRAY;
      ((CLight&)*pen).m_ltType = LT_POINT;
      ((CLight&)*pen).m_bDarkLight = TRUE;
      ((CLight&)*pen).m_rFallOffRange = 8.0f;
      ((CLight&)*pen).m_strName = "fix_texture";
      pen->en_ulSpawnFlags =0xFFFFFFFF;
      pen->Reinitialize();
    }
    FLOAT m_fCoord1 = _fKarnakDemoCoordinates[49][0];
    FLOAT m_fCoord2 = _fKarnakDemoCoordinates[49][1];
    FLOAT m_fCoord3 = _fKarnakDemoCoordinates[49][2];
    pl = CPlacement3D(FLOAT3D(m_fCoord1, m_fCoord2, m_fCoord3), ANGLE3D(0, 0, 0));
    pen = CreateEntity(pl, CLASS_LIGHT);
    pen->Initialize();
    ((CLight&)*pen).m_colColor = C_GRAY;
    ((CLight&)*pen).m_ltType = LT_POINT;
    ((CLight&)*pen).m_bDarkLight = TRUE;
    ((CLight&)*pen).m_rFallOffRange = 4.0f;
    ((CLight&)*pen).m_strName = "fix_texture";
    pen->en_ulSpawnFlags =0xFFFFFFFF;
    pen->Reinitialize();
  }

  void FixTexturesIntro(void) 
  { 
    ClearLights();
    CEntity *pen = NULL;
    CPlacement3D pl;
    for(int i = 0; i < 8; i++) {
      FLOAT m_fCoord1 = _fIntroCoordinates[i][0];
      FLOAT m_fCoord2 = _fIntroCoordinates[i][1];
      FLOAT m_fCoord3 = _fIntroCoordinates[i][2];
      pl = CPlacement3D(FLOAT3D(m_fCoord1, m_fCoord2, m_fCoord3), ANGLE3D(0, 0, 0));
      pen = CreateEntity(pl, CLASS_LIGHT);
      pen->Initialize();
      ((CLight&)*pen).m_colColor = C_GRAY;
      ((CLight&)*pen).m_ltType = LT_POINT;
      ((CLight&)*pen).m_bDarkLight = TRUE;
      ((CLight&)*pen).m_rFallOffRange = 8.0f;
      ((CLight&)*pen).m_strName = "fix_texture";
      pen->en_ulSpawnFlags =0xFFFFFFFF;
      pen->Reinitialize();
    } 
  }

  //***************************************************************
  //*********************** Old metods: ***************************
  //****************  Fix Textures on Obelisk  ********************
  //***************************************************************
  void FixTexturesOnObelisk(CTFileName strLevelName)
  {
    // for each entity in the world
    {FOREACHINDYNAMICCONTAINER(GetWorld()->wo_cenEntities, CEntity, iten) {
      // if it is brush entity
      if (iten->en_RenderType == CEntity::RT_BRUSH) {
        // for each mip in its brush
        FOREACHINLIST(CBrushMip, bm_lnInBrush, iten->en_pbrBrush->br_lhBrushMips, itbm) {
          // for all sectors in this mip
          FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc) {
            // for all polygons in sector
            FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo)
            {
              CTFileName strTextureName = itbpo->bpo_abptTextures[1].bpt_toTexture.GetName().FileName();
              int _Obelisk02Light_found   = strncmp((const char *)strTextureName, (const char *) "Obelisk02Light", (size_t) 14 );
              if (_Obelisk02Light_found == 0 ){
                  // Settings:
                  // itbpo->bpo_abptTextures[1].bpt_toTexture.GetName().FileName()
                  // itbpo->bpo_abptTextures[1].s.bpt_ubBlend
                  // itbpo->bpo_abptTextures[1].s.bpt_ubFlags 
                  // itbpo->bpo_abptTextures[1].s.bpt_colColor
                if ( strLevelName=="KarnakDemo" || strLevelName=="Intro" || strLevelName=="08_Suburbs"
                  || strLevelName=="13_Luxor" || strLevelName=="14_SacredYards") {
                  itbpo->bpo_abptTextures[1].s.bpt_colColor = (C_WHITE| 0x5F);
                } else if ( strLevelName=="04_ValleyOfTheKings" || strLevelName=="11_AlleyOfSphinxes" || strLevelName=="12_Karnak"){
                  itbpo->bpo_abptTextures[1].s.bpt_colColor = (C_GRAY| 0x2F);
                }
              }
            }
          }
        }
      } /// END if()
    }}
  }

  //***************************************************************
  //**********^**  Fix Textures on Alley Of Sphinxes  *************
  //***************************************************************
  void FixTexturesOnAlleyOfSphinxes(void)
  {
    // for each entity in the world
    {FOREACHINDYNAMICCONTAINER(GetWorld()->wo_cenEntities, CEntity, iten) {
      // if it is brush entity
      if (iten->en_RenderType == CEntity::RT_BRUSH) {
        // for each mip in its brush
        FOREACHINLIST(CBrushMip, bm_lnInBrush, iten->en_pbrBrush->br_lhBrushMips, itbm) {
          // for all sectors in this mip
          FOREACHINDYNAMICARRAY(itbm->bm_abscSectors, CBrushSector, itbsc) {
            // for all polygons in sector
            FOREACHINSTATICARRAY(itbsc->bsc_abpoPolygons, CBrushPolygon, itbpo)
            {
              CTFileName strTextureName = itbpo->bpo_abptTextures[1].bpt_toTexture.GetName().FileName();
              int _EyeOfRa_found = strncmp((const char *)strTextureName, (const char *) "EyeOfRa", (size_t) 7 );
              int _Wall12_found  = strncmp((const char *)strTextureName, (const char *) "Wall12",  (size_t) 6 );
              int _Wingy02_found = strncmp((const char *)strTextureName, (const char *) "Wingy02", (size_t) 7 );
              if (_EyeOfRa_found == 0 || _Wall12_found == 0 || _Wingy02_found == 0){
                itbpo->bpo_abptTextures[1].s.bpt_ubBlend  = BPT_BLEND_BLEND;
                itbpo->bpo_abptTextures[1].s.bpt_colColor = C_GRAY|0x80;
              }
            }
          }
        }
      } // END if()
    }}
  }

  //***************************************************************
  //***************************************************************
  //***************************************************************

  // count enemies in current world
  void CountEnemies(void)
  {
    m_ctEnemiesInWorld = 0;
    m_ctSecretsInWorld = 0;
    // for each entity in the world
    {FOREACHINDYNAMICCONTAINER(GetWorld()->wo_cenEntities, CEntity, iten) {
      CEntity *pen = iten;
      // if enemybase
      if (IsDerivedFromClass(pen, "Enemy Base")) {
        CEnemyBase *penEnemy = (CEnemyBase *)pen;
        // if not template
        if (!penEnemy->m_bTemplate) {
          // count one
          m_ctEnemiesInWorld++;
        }
      // if spawner
      } else if (IsDerivedFromClass(pen, "Enemy Spawner")) {
        CEnemySpawner *penSpawner = (CEnemySpawner *)pen;
        // if not teleporting
        if (penSpawner->m_estType!=EST_TELEPORTER) {
          // add total count
          m_ctEnemiesInWorld+=penSpawner->m_ctTotal;
        }
      // if trigger
      } else if (IsDerivedFromClass(pen, "Trigger")) {
        CTrigger *penTrigger = (CTrigger *)pen;
        // if has score
        if (penTrigger->m_fScore>0) {
          // it counts as a secret
          m_ctSecretsInWorld++;
        }
      }
    }}
  }

  // check for stale fuss-makers
  void CheckOldFussMakers(void)
  {
    TIME tmNow = _pTimer->CurrentTick();
    TIME tmTooOld = tmNow-10.0f;
    CDynamicContainer<CEntity> cenOldFussMakers;
    // for each fussmaker
    {FOREACHINDYNAMICCONTAINER(m_cenFussMakers, CEntity, itenFussMaker) {
      CEnemyBase & enFussMaker = (CEnemyBase&)*itenFussMaker;
      // if haven't done fuss for too long
      if (enFussMaker.m_tmLastFussTime<tmTooOld) {
        // add to old fuss makers
        cenOldFussMakers.Add(&enFussMaker);
      }
    }}
    // for each old fussmaker
    {FOREACHINDYNAMICCONTAINER(cenOldFussMakers, CEntity, itenOldFussMaker) {
      CEnemyBase &enOldFussMaker = (CEnemyBase&)*itenOldFussMaker;
      // remove from fuss
      enOldFussMaker.RemoveFromFuss();
    }}
  }
  
  // get total score of all active fuss makers
  INDEX GetFussMakersScore(void) {
    INDEX iScore = 0;
    {FOREACHINDYNAMICCONTAINER(m_cenFussMakers, CEntity, itenFussMaker) {
      CEnemyBase &enFussMaker = (CEnemyBase&)*itenFussMaker;
      iScore += enFussMaker.m_iScore;
    }}
    return iScore;
  }

  // change given music channel
  void ChangeMusicChannel(enum MusicType mtType, const CTFileName &fnNewMusic, FLOAT fNewVolume)
  {
    INDEX &iSubChannel = (&m_iSubChannel0)[mtType];
    // take next sub-channel if needed
    if (fnNewMusic!="") {
      iSubChannel = (iSubChannel+1)%2;
    }
    // find channel's variables
    FLOAT &fVolume = (&m_fVolume0)[mtType];
    CSoundObject &soMusic = (&m_soMusic0a)[mtType*2+iSubChannel];
    FLOAT &fCurrentVolume = (&m_fCurrentVolume0a)[mtType*2+iSubChannel];

    // setup looping/non looping flags
    ULONG ulFlags;
    if (mtType==MT_EVENT) {
      ulFlags = SOF_MUSIC;
    } else {
      ulFlags = SOF_MUSIC|SOF_LOOP|SOF_NONGAME;
    }

    // remember volumes
    fVolume = fNewVolume;
    // start new music file if needed
    if (fnNewMusic!="") {
      PlaySound( soMusic, fnNewMusic, ulFlags);
      // initially, not playing
      fCurrentVolume = MUSIC_VOLUMEMIN;
      soMusic.Pause();
      soMusic.SetVolume(fCurrentVolume, fCurrentVolume);
    }
  }

  // fade out one channel
  void FadeOutChannel(INDEX iChannel, INDEX iSubChannel)
  {
    // find channel's variables
    FLOAT &fVolume = (&m_fVolume0)[iChannel];
    CSoundObject &soMusic = (&m_soMusic0a)[iChannel*2+iSubChannel];
    FLOAT &fCurrentVolume = (&m_fCurrentVolume0a)[iChannel*2+iSubChannel];

    // do nothing, if music is not playing
    if( !soMusic.IsPlaying()) { return; }

    // do nothing, if music is already paused
    if( soMusic.IsPaused()) { return; }

    // if minimum volume reached 
    if( fCurrentVolume<MUSIC_VOLUMEMIN) {
      // pause music
      soMusic.Pause();
    } else {
      // music isn't even faded yet, so continue on fading it out
      fCurrentVolume *= FadeOutFactor( m_tmFade);
      soMusic.SetVolume( fCurrentVolume*fVolume, fCurrentVolume*fVolume);
    }
  }

  // fade in one channel
  void FadeInChannel(INDEX iChannel, INDEX iSubChannel)
  {
    // find channel's variables
    FLOAT &fVolume = (&m_fVolume0)[iChannel];
    CSoundObject &soMusic = (&m_soMusic0a)[iChannel*2+iSubChannel];
    FLOAT &fCurrentVolume = (&m_fCurrentVolume0a)[iChannel*2+iSubChannel];

    // do nothing, if music is not playing
    if( !soMusic.IsPlaying()) { return; }

    // resume music if needed
    if( soMusic.IsPaused()) {
      soMusic.Resume();
    }
    // fade in music if needed
    if( fCurrentVolume<MUSIC_VOLUMEMAX) {
      fCurrentVolume *= FadeInFactor( m_tmFade);
      fCurrentVolume = ClampUp( fCurrentVolume, 1.0f);
    }
    soMusic.SetVolume( fCurrentVolume*fVolume, fCurrentVolume*fVolume);
  }

  // fade one channel in or out
  void CrossFadeOneChannel(enum MusicType mtType)
  {
    INDEX iSubChannelActive = (&m_iSubChannel0)[mtType];
    INDEX iSubChannelInactive = (iSubChannelActive+1)%2;
    // if it is current channel
    if (mtType==m_mtCurrentMusic) {
      // fade in active subchannel
      FadeInChannel(mtType, iSubChannelActive);
      // fade out inactive subchannel
      FadeOutChannel(mtType, iSubChannelInactive);
    // if it is not current channel
    } else {
      // fade it out
      FadeOutChannel(mtType, 0);
      FadeOutChannel(mtType, 1);
    }
  }
  
procedures:
  // initialize music
  Main(EVoid) {

    // init as model
    InitAsEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);

    // set appearance
    SetModel(MODEL_MARKER);
    SetModelMainTexture(TEXTURE_MARKER);

    // wait for game to start
    autowait(_pTimer->TickQuantum);

    // Get Level Name and Mod Name
    CTString strLevelName = _pNetwork->ga_fnmWorld.FileName();
    CTString strModName = _pShell->GetValue("sys_strModName");
    INDEX iBugFixMetod = _pShell->GetINDEX("gam_bFixIlluminationsMetod");

    if(iBugFixMetod == 1) {
      // Fix Obelisk textures
      if ( strModName=="" ) {
        if ( strLevelName=="04_ValleyOfTheKings" || strLevelName=="11_AlleyOfSphinxes" || strLevelName=="12_Karnak" 
          || strLevelName=="13_Luxor" || strLevelName=="KarnakDemo" || strLevelName=="Intro" 
          || strLevelName=="08_Suburbs" || strLevelName=="14_SacredYards") {
          FixTexturesOnObelisk(strLevelName);
        }
      }
      // Fix Alley Of Sphinxes textures
      if (/* strModName=="" && */ strLevelName=="11_AlleyOfSphinxes") {
        FixTexturesOnAlleyOfSphinxes();
      }
    } else if(iBugFixMetod == 2) {
      // Fix textures
      if (/* strModName=="" && */ strLevelName=="04_ValleyOfTheKings") {
        FixTexturesValleyOfTheKings();
      } else if (/* strModName=="" && */ strLevelName=="07_Dunes") {
        FixTexturesDunes();
      } else if (/* strModName=="" && */ strLevelName=="08_Suburbs") {
        FixTexturesSuburbs();
      } else if (/* strModName=="" && */ strLevelName=="10_Metropolis") {
        FixTexturesMetropolis();
      } else if (/* strModName=="" && */ strLevelName=="11_AlleyOfSphinxes") {
        FixTexturesAlleyOfSphinxes();
      } else if (/* strModName=="" && */ strLevelName=="12_Karnak") {
        FixTexturesKarnak();
      } else if (/* strModName=="" && */ strLevelName=="13_Luxor") {
        FixTexturesLuxor();
      } else if (/* strModName=="" && */ strLevelName=="14_SacredYards") {
        FixTexturesSacredYards();
      } else if (/* strModName=="" && */ strLevelName=="KarnakDemo") {
        FixTexturesKarnakDemo();
      } else if (/* strModName=="" && */ strLevelName=="Intro") {
        FixTexturesIntro();
      }
    }

    // prepare initial music channel values
    ChangeMusicChannel(MT_LIGHT,        m_fnMusic0, m_fVolume0);
    ChangeMusicChannel(MT_MEDIUM,       m_fnMusic1, m_fVolume1);
    ChangeMusicChannel(MT_HEAVY,        m_fnMusic2, m_fVolume2);
    ChangeMusicChannel(MT_EVENT,        m_fnMusic3, m_fVolume3);
    ChangeMusicChannel(MT_CONTINUOUS,   m_fnMusic4, m_fVolume4);

    // start with light music
    m_mtCurrentMusic = MT_LIGHT;
    m_fCurrentVolume0a = MUSIC_VOLUMEMAX*0.98f;
    m_tmFade = 0.01f;
    CrossFadeOneChannel(MT_LIGHT);

    // must react after enemyspawner and all enemies, but before player for proper enemy counting
    // (total wait is two ticks so far)
    autowait(_pTimer->TickQuantum);

    // count enemies in current world
    CountEnemies();

    // main loop
    while(TRUE) {
      // wait a bit
      wait(0.1f) {
        on (ETimer) : {
          stop;
        };
        // if music is to be changed
        on (EChangeMusic ecm) : { 
          // change parameters
          ChangeMusicChannel(ecm.mtType, ecm.fnMusic, ecm.fVolume);
          // if force started
          if (ecm.bForceStart) {
            // set as current music
            m_mtCurrentMusic = ecm.mtType;
          }
          // stop waiting
          stop;
        }
      }
      // check fuss
      CheckOldFussMakers();
      // get total score of all active fuss makers
      FLOAT fFussScore = GetFussMakersScore();
      // if event is on
      if (m_mtCurrentMusic==MT_EVENT) {
        // if event has ceased playing
        if (!m_soMusic3a.IsPlaying() && !m_soMusic3b.IsPlaying()) {
          // switch to light music
          m_mtCurrentMusic=MT_LIGHT;
        }
      }
      // if heavy fight is on
      if (m_mtCurrentMusic==MT_HEAVY) {
        // if no more fuss
        if (fFussScore<=0.0f) {
          // switch to no fight
          m_mtCurrentMusic=MT_LIGHT;
        }
      // if medium fight is on
      } else if (m_mtCurrentMusic==MT_MEDIUM) {
        // if no more fuss
        if (fFussScore<=0.0f) {
          // switch to no fight
          m_mtCurrentMusic=MT_LIGHT;
        // if larger fuss
        } else if (fFussScore>=m_fScoreHeavy) {
          // switch to heavy fight
          m_mtCurrentMusic=MT_HEAVY;
        }
      // if no fight is on
      } else if (m_mtCurrentMusic==MT_LIGHT) {
        // if heavy fuss
        if (fFussScore>=m_fScoreHeavy) {
          // switch to heavy fight
          m_mtCurrentMusic=MT_HEAVY;
        // if medium fuss
        } else if (fFussScore>=m_fScoreMedium) {
          // switch to medium fight
          m_mtCurrentMusic=MT_MEDIUM;
        }
      }

      // setup fade speed depending on music type
      if (m_mtCurrentMusic==MT_LIGHT) {
        m_tmFade = 2.0f;
      } else if (m_mtCurrentMusic==MT_MEDIUM) {
        m_tmFade = 1.0f;
      } else if (m_mtCurrentMusic==MT_HEAVY) {
        m_tmFade = 1.0f;
      } else if (m_mtCurrentMusic==MT_EVENT || m_mtCurrentMusic==MT_CONTINUOUS) {
        m_tmFade = 0.5f;
      }

      // fade all channels
      CrossFadeOneChannel(MT_LIGHT);
      CrossFadeOneChannel(MT_MEDIUM);
      CrossFadeOneChannel(MT_HEAVY);
      CrossFadeOneChannel(MT_EVENT);
      CrossFadeOneChannel(MT_CONTINUOUS);
    }
    return;
  }
};
