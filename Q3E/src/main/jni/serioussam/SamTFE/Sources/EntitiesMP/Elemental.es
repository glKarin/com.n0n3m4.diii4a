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

322
%{
#include "EntitiesMP/StdH/StdH.h"
//#include "Models/Enemies/Elementals/AirMan.h"
//#include "Models/Enemies/Elementals/IceMan.h"
#include "Models/Enemies/Elementals/Stoneman.h"
//#include "Models/Enemies/Elementals/Twister.h"
//#include "Models/Enemies/Elementals/WaterMan.h"
//#include "Models/Enemies/Elementals/Projectile/IcePyramid.h"
#include "Models/Enemies/Elementals/Projectile/LavaStone.h"

#include "Models/Enemies/ElementalLava/ElementalLava.h"
#include "EntitiesMP/WorldSettingsController.h"
#include "EntitiesMP/BackgroundViewer.h"

// lava elemental definitions
#define LAVAMAN_SMALL_STRETCH (2.0f*0.75f)
#define LAVAMAN_BIG_STRETCH (4.0f*1.25f)
#define LAVAMAN_LARGE_STRETCH (16.0f*2.5f)

#define LAVAMAN_BOSS_FIRE_RIGHT FLOAT3D(1.01069f, 0.989616f, -1.39743f)
#define LAVAMAN_BOSS_FIRE_LEFT FLOAT3D(-0.39656f, 1.08619f, -1.34373f)
#define LAVAMAN_FIRE_LEFT FLOAT3D(-0.432948f, 1.51133f, -0.476662f)

#define LAVAMAN_FIRE_SMALL  (LAVAMAN_FIRE_LEFT*LAVAMAN_SMALL_STRETCH)
#define LAVAMAN_FIRE_BIG    (LAVAMAN_FIRE_LEFT*LAVAMAN_BIG_STRETCH)
#define LAVAMAN_FIRE_LARGE_LEFT  (LAVAMAN_BOSS_FIRE_LEFT*LAVAMAN_LARGE_STRETCH)
#define LAVAMAN_FIRE_LARGE_RIGHT (LAVAMAN_BOSS_FIRE_RIGHT*LAVAMAN_LARGE_STRETCH)

#define LAVAMAN_SPAWN_BIG   (FLOAT3D(0.0171274f, 1.78397f, -0.291414f)*LAVAMAN_BIG_STRETCH)
#define LAVAMAN_SPAWN_LARGE (FLOAT3D(0.0171274f, 1.78397f, -0.291414f)*LAVAMAN_LARGE_STRETCH)
#define DEATH_BURN_TIME 1.0f

%}

uses "EntitiesMP/EnemyBase";
//uses "EntitiesMP/Twister";
//uses "EntitiesMP/Water";

enum ElementalType {
  0 ELT_AIR           "obsolete",        // air elemental
  1 ELT_ICE           "obsolete",        // ice elemental
  2 ELT_LAVA          "Lava",       // lava elemental
  3 ELT_STONE         "obsolete",      // stone elemental
  4 ELT_WATER         "obsolete",      // water elemental
};


enum ElementalCharacter {
  0 ELC_SMALL         "Small",      // small (fighter)
  1 ELC_BIG           "Big",        // big
  2 ELC_LARGE         "Large",      // large
};


enum ElementalState {
  0 ELS_NORMAL        "Normal",     // normal state
  1 ELS_BOX           "Box",        // in box
  2 ELS_PLANE         "Plane",      // as plane
};


%{
#define ECF_AIR ( \
  ((ECBI_BRUSH|ECBI_MODEL|ECBI_CORPSE|ECBI_ITEM|ECBI_PROJECTILE_MAGIC|ECBI_PROJECTILE_SOLID)<<ECB_TEST) |\
  ((ECBI_MODEL|ECBI_CORPSE|ECBI_ITEM|ECBI_PROJECTILE_MAGIC|ECBI_PROJECTILE_SOLID)<<ECB_PASS) |\
  ((ECBI_MODEL)<<ECB_IS))
// info structure
// air
static EntityInfo eiAirElementalSmall = {
  EIBT_AIR, 50.0f,
  0.0f, 1.7f, 0.0f,
  0.0f, 1.0f, 0.0f,
};
static EntityInfo eiAirElementalBig = {
  EIBT_AIR, 200.0f,
  0.0f, 6.8f, 0.0f,
  0.0f, 4.0f, 0.0f,
};
static EntityInfo eiAirElementalLarge = {
  EIBT_AIR, 800.0f,
  0.0f, 27.2f, 0.0f,
  0.0f, 16.0f, 0.0f,
};

// ice
static EntityInfo eiIceElementalSmall = {
  EIBT_ICE, 400.0f,
  0.0f, 1.7f, 0.0f,
  0.0f, 1.0f, 0.0f,
};
static EntityInfo eiIceElementalBig = {
  EIBT_ICE, 1600.0f,
  0.0f, 6.8f, 0.0f,
  0.0f, 4.0f, 0.0f,
};
static EntityInfo eiIceElementalLarge = {
  EIBT_ICE, 6400.0f,
  0.0f, 27.2f, 0.0f,
  0.0f, 16.0f, 0.0f,
};

// lava
static EntityInfo eiLavaElementalSmall = {
  EIBT_FIRE, 2000.0f,
  0.0f, 1.7f*LAVAMAN_SMALL_STRETCH, 0.0f,
  0.0f, LAVAMAN_SMALL_STRETCH, 0.0f,
};
static EntityInfo eiLavaElementalBig = {
  EIBT_FIRE, 2800.0f,
  0.0f, 1.7f*LAVAMAN_BIG_STRETCH, 0.0f,
  0.0f, LAVAMAN_BIG_STRETCH, 0.0f,
};
static EntityInfo eiLavaElementalLarge = {
  EIBT_FIRE, 11200.0f,
  0.0f, 1.7f*LAVAMAN_LARGE_STRETCH, 0.0f,
  0.0f, LAVAMAN_LARGE_STRETCH, 0.0f,
};

// stone
static EntityInfo eiStoneElementalSmall = {
  EIBT_ROCK, 1000.0f,
  0.0f, 1.7f, 0.0f,
  0.0f, 1.0f, 0.0f,
};
static EntityInfo eiStoneElementalBig = {
  EIBT_ROCK, 4000.0f,
  0.0f, 6.8f, 0.0f,
  0.0f, 4.0f, 0.0f,
};
static EntityInfo eiStoneElementalLarge = {
  EIBT_ROCK, 16000.0f,
  0.0f, 27.2f, 0.0f,
  0.0f, 16.0f, 0.0f,
};

// water
static EntityInfo eiWaterElementalSmall = {
  EIBT_WATER, 500.0f,
  0.0f, 1.7f, 0.0f,
  0.0f, 1.0f, 0.0f,
};
static EntityInfo eiWaterElementalBig = {
  EIBT_WATER, 2000.0f,
  0.0f, 6.8f, 0.0f,
  0.0f, 4.0f, 0.0f,
};
static EntityInfo eiWaterElementalLarge = {
  EIBT_WATER, 8000.0f,
  0.0f, 27.2f, 0.0f,
  0.0f, 16.0f, 0.0f,
};


// obsolete
#define EPF_BOX_PLANE_ELEMENTAL (EPF_ORIENTEDBYGRAVITY|EPF_MOVABLE)
#define FIRE_ROCKS        FLOAT3D(-0.9f, 1.6f, -1.0f)
#define FIRE_ROCKS_BIG    FLOAT3D(-3.6f, 6.4f, -4.0f)
#define FIRE_ROCKS_LARGE  FLOAT3D(-14.4f, 25.6f, -16.0f)
#define WATER_LEFT          FLOAT3D(-0.75f,  1.3f,  -1.2f)
#define WATER_RIGHT         FLOAT3D( 0.75f,  1.3f,  -1.2f)
#define WATER_BIG_LEFT      FLOAT3D(-3.0f,   5.2f,  -4.8f)
#define WATER_BIG_RIGHT     FLOAT3D( 3.0f,   5.2f,  -4.8f)
#define WATER_LARGE_LEFT    FLOAT3D(-12.0f, 20.8f, -19.2f)
#define WATER_LARGE_RIGHT   FLOAT3D( 12.0f, 20.8f, -19.2f)

%}


class CElemental : CEnemyBase {
name      "Elemental";
thumbnail "Thumbnails\\Elemental.tbn";

properties:
  1 enum ElementalType m_EetType          "Type" 'Y' = ELT_STONE,
  2 enum ElementalCharacter m_EecChar     "Character" 'C' = ELC_SMALL,
  3 enum ElementalState m_EesStartState   "State" 'S' = ELS_NORMAL,
  4 BOOL m_bSpawnWhenHarmed               "Damage spawn" 'N' = TRUE,
  5 BOOL m_bSpawnOnBlowUp                 "Blowup spawn" 'B' = TRUE,
  6 enum ElementalState m_EesCurrentState = ELS_NORMAL,
  7 BOOL m_bSpawned = FALSE,
  8 BOOL m_bMovable                       "Movable" 'V' = TRUE,
  9 RANGE m_fLookRange                    "Look range" 'O' = 30.0f,
 10 INDEX m_iFireCount                    "Fire count" = 2,
 11 FLOAT m_fWaitTime = 0.0f,
 12 INDEX m_iCounter = 0,
 13 FLOAT m_fDensity                      "Density" 'D' = 10000.0f,

// placement for non movable elemental
 20 CEntityPointer m_penPosition1         "Position 1",
 21 CEntityPointer m_penPosition2         "Position 2",
 22 CEntityPointer m_penPosition3         "Position 3",
 23 CEntityPointer m_penPosition4         "Position 4",
 24 CEntityPointer m_penPosition5         "Position 5",
 25 CEntityPointer m_penPosition6         "Position 6",

 30 CSoundObject m_soBackground,  // sound channel for background noise
 31 INDEX m_ctSpawned = 0,
 32 FLOAT m_fSpawnDamage = 1e6f,
 33 BOOL m_bSpawnEnabled = FALSE,
 34 CSoundObject m_soFireL,
 35 CSoundObject m_soFireR,
 36 INDEX m_bCountAsKill = TRUE,
 
components:
  0 class   CLASS_BASE         "Classes\\EnemyBase.ecl",
  1 class   CLASS_TWISTER      "Classes\\Twister.ecl",
  2 class   CLASS_WATER        "Classes\\Water.ecl",
  3 class   CLASS_PROJECTILE   "Classes\\Projectile.ecl",
  4 class   CLASS_BLOOD_SPRAY  "Classes\\BloodSpray.ecl",
  5 class   CLASS_BASIC_EFFECT "Classes\\BasicEffect.ecl",

/* // air
 10 model   MODEL_AIR               "Models\\Enemies\\Elementals\\AirMan.mdl",
 11 model   MODEL_AIR_TWISTER       "Models\\Enemies\\Elementals\\Twister.mdl",
 12 texture TEXTURE_AIR             "Models\\Enemies\\Elementals\\AirMan01.tex",

 // ice
 20 model   MODEL_ICE               "Models\\Enemies\\Elementals\\IceMan.mdl",
 21 model   MODEL_ICE_PICK          "Models\\Enemies\\Elementals\\IcePick.mdl",
 22 texture TEXTURE_ICE             "Models\\Enemies\\Elementals\\IceMan01.tex",
*/
 // lava
 30 model   MODEL_LAVA              "Models\\Enemies\\ElementalLava\\ElementalLava.mdl",
 31 model   MODEL_LAVA_BODY_FLARE   "Models\\Enemies\\ElementalLava\\BodyFlare.mdl",
 32 model   MODEL_LAVA_HAND_FLARE   "Models\\Enemies\\ElementalLava\\HandFlare.mdl",
 33 texture TEXTURE_LAVA            "Models\\Enemies\\ElementalLava\\Lava04Fx.tex",
 34 texture TEXTURE_LAVA_DETAIL     "Models\\Enemies\\ElementalLava\\Detail.tex",
 35 texture TEXTURE_LAVA_FLARE      "Models\\Enemies\\ElementalLava\\Flare.tex",
/*
 // stone
 40 model   MODEL_STONE             "Models\\Enemies\\Elementals\\StoneMan.mdl",
 41 model   MODEL_STONE_MAUL        "Models\\Enemies\\Elementals\\Maul.mdl",
 42 texture TEXTURE_STONE           "Models\\Enemies\\Elementals\\StoneMan01.tex",

 // water
 50 model   MODEL_WATER             "Models\\Enemies\\Elementals\\WaterMan.mdl",
 51 model   MODEL_WATER_BODY_FLARE  "Models\\Enemies\\Elementals\\WaterManFX\\BodyFlare.mdl",
 52 texture TEXTURE_WATER           "Models\\Enemies\\Elementals\\WaterManFX.tex",
 53 texture TEXTURE_WATER_FLARE     "Models\\Enemies\\Elementals\\WaterManFX\\BodyFlare.tex",
*/
 // debris
// 80 model   MODEL_ELEM_STONE            "Models\\Enemies\\Elementals\\Projectile\\Stone.mdl",
// 82 model   MODEL_ELEM_LAVASTONE        "Models\\Enemies\\ElementalLava\\Projectile\\LavaStone.mdl",
// 83 model   MODEL_ELEM_LAVASTONE_FLARE  "Models\\Enemies\\ElementalLava\\Projectile\\LavaStoneFlare.mdl",
// 84 model   MODEL_ELEM_ICE              "Models\\Enemies\\Elementals\\Projectile\\IcePyramid.mdl",
// 85 model   MODEL_ELEM_ICE_FLARE        "Models\\Enemies\\Elementals\\Projectile\\IcePyramidFlare.mdl",

// 90 texture TEXTURE_ELEM_STONE          "Models\\Enemies\\Elementals\\Projectile\\Stone.tex",
// 92 texture TEXTURE_ELEM_LAVASTONE      "Models\\Enemies\\ElementalLava\\Projectile\\LavaStone.tex",
// 93 texture TEXTURE_ELEM_ICE            "Models\\Enemies\\Elementals\\Projectile\\IcePyramid.tex",
// 94 texture TEXTURE_ELEM_FLARE          "Textures\\Effects\\Flares\\03\\Flaire06.tex",

// ************** SPECULAR **************
//210 texture TEX_SPEC_WEAK           "Models\\SpecularTextures\\Weak.tex",
//211 texture TEX_SPEC_MEDIUM         "Models\\SpecularTextures\\Medium.tex",
//212 texture TEX_SPEC_STRONG         "Models\\SpecularTextures\\Strong.tex",

// ************** SOUNDS **************
250 sound   SOUND_LAVA_IDLE      "Models\\Enemies\\ElementalLava\\Sounds\\Idle.wav",
252 sound   SOUND_LAVA_WOUND     "Models\\Enemies\\ElementalLava\\Sounds\\Wound.wav",
253 sound   SOUND_LAVA_FIRE      "Models\\Enemies\\ElementalLava\\Sounds\\Fire.wav",
254 sound   SOUND_LAVA_KICK      "Models\\Enemies\\ElementalLava\\Sounds\\Kick.wav",
255 sound   SOUND_LAVA_DEATH     "Models\\Enemies\\ElementalLava\\Sounds\\Death.wav",
220 sound   SOUND_LAVA_LAVABURN  "Models\\Enemies\\ElementalLava\\Sounds\\LavaBurn.wav",
221 sound   SOUND_LAVA_ANGER     "Models\\Enemies\\ElementalLava\\Sounds\\Anger.wav",
222 sound   SOUND_LAVA_GROW      "ModelsMP\\Enemies\\ElementalLava\\Sounds\\Grow.wav",

functions:
  // describe how this enemy killed player
  virtual CTString GetPlayerKillDescription(const CTString &strPlayerName, const EDeath &eDeath)
  {
    CTString str;
    str.PrintF(TRANSV("%s was killed by a Lava Golem"), (const char *) strPlayerName);
    return str;
  }
  virtual const CTFileName &GetComputerMessageName(void) const {
    static DECLARE_CTFILENAME(fnm, "Data\\Messages\\Enemies\\ElementalLava.txt");
    return fnm;
  };

  // render burning particles
  void RenderParticles(void)
  {
    FLOAT fTimeFactor=1.0f;
    FLOAT fPower=0.25f;
    if (m_EesCurrentState==ELS_NORMAL)
    {
      FLOAT fDeathFactor=1.0f;
      if( m_fSpiritStartTime!=0.0f)
      {
        fDeathFactor=1.0f-Clamp((_pTimer->CurrentTick()-m_fSpiritStartTime)/DEATH_BURN_TIME, 0.0f, 1.0f);
      }
      Particles_Burning(this, fPower, fTimeFactor*fDeathFactor);
    }
  }

  void Precache(void)
  {
    CEnemyBase::Precache();

    switch(m_EetType)
    {
    case ELT_LAVA:
      {
        if( m_EecChar == ELC_LARGE)
        {
          PrecacheClass(CLASS_PROJECTILE, PRT_LAVAMAN_BIG_BOMB);
        }
        if( (m_EecChar == ELC_LARGE) || (m_EecChar == ELC_BIG) )
        {
          PrecacheClass(CLASS_PROJECTILE, PRT_LAVAMAN_BOMB);
        }

        PrecacheClass(CLASS_PROJECTILE, PRT_LAVAMAN_STONE);

        PrecacheModel  (MODEL_LAVA            );
        PrecacheModel  (MODEL_LAVA_BODY_FLARE );
        PrecacheModel  (MODEL_LAVA_HAND_FLARE );
        PrecacheTexture(TEXTURE_LAVA          );
        PrecacheTexture(TEXTURE_LAVA_DETAIL   );
        PrecacheTexture(TEXTURE_LAVA_FLARE    );

        PrecacheSound(SOUND_LAVA_IDLE    );
        PrecacheSound(SOUND_LAVA_WOUND   );
        PrecacheSound(SOUND_LAVA_FIRE    );
        PrecacheSound(SOUND_LAVA_KICK    );
        PrecacheSound(SOUND_LAVA_DEATH   );
        PrecacheSound(SOUND_LAVA_ANGER   );
        PrecacheSound(SOUND_LAVA_LAVABURN);
        PrecacheSound(SOUND_LAVA_GROW    );
        break;
      }
    }
  };

  /* Entity info */
  void *GetEntityInfo(void) {
    switch (m_EetType) {
      case ELT_AIR:
        switch(m_EecChar) {
          case ELC_LARGE: return &eiAirElementalLarge;
          case ELC_BIG: return &eiAirElementalBig;
          default: { return &eiAirElementalSmall; }
        }
        break;
      case ELT_ICE:
        switch(m_EecChar) {
          case ELC_LARGE: return &eiIceElementalLarge;
          case ELC_BIG: return &eiIceElementalBig;
          default: { return &eiIceElementalSmall; }
        }
        break;
      case ELT_LAVA:
        switch(m_EecChar) {
          case ELC_LARGE: return &eiLavaElementalLarge;
          case ELC_BIG: return &eiLavaElementalBig;
          default: { return &eiLavaElementalSmall; }
        }
        break;
      case ELT_STONE:
        switch(m_EecChar) {
          case ELC_LARGE: return &eiStoneElementalLarge;
          case ELC_BIG: return &eiStoneElementalBig;
          default: { return &eiStoneElementalSmall; }
        }
        break;
      //case ELT_WATER:
      default: {
        switch(m_EecChar) {
          case ELC_LARGE: return &eiWaterElementalLarge;
          case ELC_BIG: return &eiWaterElementalBig;
          default: { return &eiWaterElementalSmall; }
      }}
    }
  };

  /* Fill in entity statistics - for AI purposes only */
  BOOL FillEntityStatistics(EntityStats *pes)
  {
    CEnemyBase::FillEntityStatistics(pes);
    switch(m_EetType) {
    case ELT_WATER : { pes->es_strName+=" Water"; } break;
    case ELT_AIR   : { pes->es_strName+=" Air"; } break;
    case ELT_STONE : { pes->es_strName+=" Stone"; } break;
    case ELT_LAVA  : { pes->es_strName+=" Lava"; } break;
    case ELT_ICE   : { pes->es_strName+=" Ice"; } break;
    }
    switch(m_EecChar) {
    case ELC_LARGE: pes->es_strName+=" Large"; break;
    case ELC_BIG:   pes->es_strName+=" Big"; break;
    case ELC_SMALL: pes->es_strName+=" Small"; break;
    }
    return TRUE;
  }

  /* Receive damage */
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    // elemental can't harm elemental
    if( IsOfClass(penInflictor, "Elemental")) {
      return;
    }

    // boss can't be telefragged
    if( m_EecChar==ELC_LARGE && dmtType==DMT_TELEPORT)
    {
      return;
    }

    // elementals take less damage from heavy bullets (e.g. sniper)
    if( m_EecChar==ELC_BIG && dmtType==DMT_BULLET && fDamageAmmount>100.0f)
    {
      fDamageAmmount/=2.5f;
    }


    INDEX ctShouldSpawn = Clamp( INDEX((m_fMaxHealth-GetHealth())/m_fSpawnDamage), INDEX(0), INDEX(10));
    CTString strChar = ElementalCharacter_enum.NameForValue(INDEX(m_EecChar));
    //CPrintF( "Character: %s, MaxHlt = %g, Hlt = %g, SpwnDmg = %g, Spawned: %d, Should: %d\n",
    //  strChar, m_fMaxHealth, GetHealth(), m_fSpawnDamage, m_ctSpawned, ctShouldSpawn);

    if (m_bSpawnEnabled && m_bSpawnWhenHarmed && (m_EecChar==ELC_LARGE || m_EecChar==ELC_BIG))
    {
      INDEX ctShouldSpawn = Clamp( INDEX((m_fMaxHealth-GetHealth())/m_fSpawnDamage), INDEX(0), INDEX(10));
      if(m_ctSpawned<ctShouldSpawn)
      {
        SendEvent( EForceWound() );
      }
    }

    // if not in normal state can't be harmed
    if (m_EesCurrentState!=ELS_NORMAL) {
      return;
    }

    CEnemyBase::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);
  };

  void LeaveStain( BOOL bGrow)
  {
    return;
  }

  // damage anim
  INDEX AnimForDamage(FLOAT fDamage) {
    INDEX iAnim;

    if (m_EetType == ELT_LAVA) {
      switch (IRnd()%3) {
        case 0: iAnim = ELEMENTALLAVA_ANIM_WOUND01; break;
        case 1: iAnim = ELEMENTALLAVA_ANIM_WOUND02; break;
        default: iAnim = ELEMENTALLAVA_ANIM_WOUND03; break;
      }
    } else {
        iAnim=0; // DG: not sure this makes sense, but at least it has a deterministic value
/*      switch (IRnd()%3) {
        case 0: iAnim = STONEMAN_ANIM_WOUND01; break;
        case 1: iAnim = STONEMAN_ANIM_WOUND02; break;
        default: iAnim = STONEMAN_ANIM_WOUND03; break;
      }*/
    }
    StartModelAnim(iAnim, 0);
    return iAnim;
  };

  void StandingAnimFight(void) {
    StartModelAnim(ELEMENTALLAVA_ANIM_STANDFIGHT, AOF_LOOPING|AOF_NORESTART);
  }

  // virtual anim functions
  void StandingAnim(void) {
    if (m_EetType == ELT_LAVA) {
      switch (m_EesCurrentState) {
        case ELS_NORMAL: StartModelAnim(ELEMENTALLAVA_ANIM_WALKBIG, AOF_LOOPING|AOF_NORESTART); break;
        case ELS_BOX: StartModelAnim(ELEMENTALLAVA_ANIM_MELTFLY, AOF_LOOPING|AOF_NORESTART); break;
        //case ELS_PLANE:
        default: StartModelAnim(ELEMENTALLAVA_ANIM_STANDPLANE, AOF_LOOPING|AOF_NORESTART); break;
      }
    } else {
/*      switch (m_EesCurrentState) {
        case ELS_NORMAL: StartModelAnim(STONEMAN_ANIM_STAND, AOF_LOOPING|AOF_NORESTART); break;
        case ELS_BOX: StartModelAnim(STONEMAN_ANIM_STANDBOX, AOF_LOOPING|AOF_NORESTART); break;
        //case ELS_PLANE:
        default: StartModelAnim(STONEMAN_ANIM_STANDPLANE, AOF_LOOPING|AOF_NORESTART); break;
      }*/
    }
  };

  void WalkingAnim(void)
  {
    if (m_EetType == ELT_LAVA) {
      if (m_EecChar==ELC_LARGE) {
        StartModelAnim(ELEMENTALLAVA_ANIM_WALKBIG, AOF_LOOPING|AOF_NORESTART);
      } else if (m_EecChar==ELC_BIG) {
        StartModelAnim(ELEMENTALLAVA_ANIM_RUNMEDIUM, AOF_LOOPING|AOF_NORESTART);
      } else {
        StartModelAnim(ELEMENTALLAVA_ANIM_RUNSMALL, AOF_LOOPING|AOF_NORESTART);
      }
    } else {
//      StartModelAnim(STONEMAN_ANIM_WALK, AOF_LOOPING|AOF_NORESTART);
    }
  };
  void RunningAnim(void)
  {
    if (m_EetType == ELT_LAVA) {
      WalkingAnim();
    } else {
//      StartModelAnim(STONEMAN_ANIM_RUN, AOF_LOOPING|AOF_NORESTART);
    }
  };
  void RotatingAnim(void) {
    if (m_EetType == ELT_LAVA) {
      WalkingAnim();
    } else {
//      StartModelAnim(STONEMAN_ANIM_WALK, AOF_LOOPING|AOF_NORESTART);
    }
  };

  INDEX AnimForDeath(void)
  {
    INDEX iAnim;
    if (m_EetType == ELT_LAVA) {
      iAnim = ELEMENTALLAVA_ANIM_DEATH03;
    } else {
      iAnim = 0; // DG: set to deterministic value
//      iAnim = STONEMAN_ANIM_DEATH03;
    }
    StartModelAnim(iAnim, 0);
    return iAnim;
  };

  // virtual sound functions
  void IdleSound(void) {
    PlaySound(m_soSound, SOUND_LAVA_IDLE, SOF_3D);
  };
  void SightSound(void) {
  };
  void WoundSound(void) {
    PlaySound(m_soSound, SOUND_LAVA_WOUND, SOF_3D);
  };
  void DeathSound(void) {
    PlaySound(m_soSound, SOUND_LAVA_DEATH, SOF_3D);
  };

  BOOL CountAsKill(void)
  {
    return m_bCountAsKill;
  }

  // spawn new elemental
  void SpawnNewElemental(void) 
  {
    INDEX ctShouldSpawn = Clamp( INDEX((m_fMaxHealth-GetHealth())/m_fSpawnDamage), INDEX(0), INDEX(10));
    // disable too much spawning
    if (m_bSpawnOnBlowUp && (m_EecChar==ELC_LARGE || m_EecChar==ELC_BIG) && (GetHealth()<=0.0f) )
    {
      ctShouldSpawn+=2;
    }

    ASSERT(m_ctSpawned<=ctShouldSpawn);
    if(m_ctSpawned>=ctShouldSpawn)
    {
      return;
    }

    CPlacement3D pl;
    // spawn placement
    if (m_EecChar==ELC_LARGE) {
      pl = CPlacement3D(LAVAMAN_SPAWN_LARGE, ANGLE3D(-90.0f+FRnd()*180.0f, 30+FRnd()*30, 0));
    } else {
      pl = CPlacement3D(LAVAMAN_SPAWN_BIG, ANGLE3D(-90.0f+FRnd()*180.0f, 40+FRnd()*20, 0));
    }
    pl.RelativeToAbsolute(GetPlacement());

    // create entity
    CEntityPointer pen = GetWorld()->CreateEntity(pl, GetClass());
    ((CElemental&)*pen).m_EetType = m_EetType;
    // elemental size
    if (m_EecChar==ELC_LARGE) {
      ((CElemental&)*pen).m_EecChar = ELC_BIG;
    } else {
      ((CElemental&)*pen).m_EecChar = ELC_SMALL;
    }
    // start properties
    ((CElemental&)*pen).m_EesStartState = ELS_BOX;
    ((CElemental&)*pen).m_fDensity = m_fDensity;
    ((CElemental&)*pen).m_colColor = m_colColor;
    ((CElemental&)*pen).m_penEnemy = m_penEnemy;
    ((CElemental&)*pen).m_ttTarget = m_ttTarget;
    ((CElemental&)*pen).m_bSpawned = TRUE;
    pen->Initialize(EVoid());
    // set moving
    if (m_EecChar==ELC_LARGE) {
      ((CElemental&)*pen).LaunchAsFreeProjectile(FLOAT3D(0, 0, -40.0f), this);
    } else {
      ((CElemental&)*pen).LaunchAsFreeProjectile(FLOAT3D(0, 0, -20.0f), this);
    }
    ((CElemental&)*pen).SetDesiredRotation(ANGLE3D(0, 0, FRnd()*360-180));

    // spawn particle debris explosion
    CEntity *penSpray = CreateEntity( pl, CLASS_BLOOD_SPRAY);
    penSpray->SetParent( pen);
    ESpawnSpray eSpawnSpray;
    eSpawnSpray.fDamagePower = 4.0f;
    eSpawnSpray.fSizeMultiplier = 0.5f;
    eSpawnSpray.sptType = SPT_LAVA_STONES;
    eSpawnSpray.vDirection = FLOAT3D(0,-0.5f,0);
    eSpawnSpray.colBurnColor=C_WHITE|CT_OPAQUE;
    eSpawnSpray.penOwner = pen;
    penSpray->Initialize( eSpawnSpray);
    m_ctSpawned++;
  };

  // throw rocks
  void ThrowRocks(ProjectileType EptProjectile) {
    // projectile type and position
    FLOAT3D vPos;
    ANGLE3D aAngle;
    // throw rocks
    switch (m_EecChar) {
      case ELC_LARGE: {
        vPos = FIRE_ROCKS_LARGE;
        ShootProjectile(EptProjectile, vPos, ANGLE3D(0, 0, 0));
        aAngle = ANGLE3D(FRnd()*5.0f+5.0f, FRnd()*3.0f-2.0f, 0);
        ShootProjectile(EptProjectile, vPos, aAngle);
        aAngle = ANGLE3D(FRnd()*-5.0f-5.0f, FRnd()*3.0f-2.0f, 0);
        ShootProjectile(EptProjectile, vPos, aAngle);
        break; }
      case ELC_BIG: {
        vPos = FIRE_ROCKS_BIG;
        ShootProjectile(EptProjectile, vPos, ANGLE3D(0, 0, 0));
        aAngle = ANGLE3D(FRnd()*4.0f+4.0f, FRnd()*3.0f-2.0f, 0);
        ShootProjectile(EptProjectile, vPos, aAngle);
        aAngle = ANGLE3D(FRnd()*-4.0f-4.0f, FRnd()*3.0f-2.0f, 0);
        ShootProjectile(EptProjectile, vPos, aAngle);
        break; }
      default: {
        vPos = FIRE_ROCKS;
        ShootProjectile(EptProjectile, vPos, ANGLE3D(0, 0, 0));
        aAngle = ANGLE3D(FRnd()*3.0f+3.0f, FRnd()*3.0f-2.0f, 0);
        ShootProjectile(EptProjectile, vPos, aAngle);
        aAngle = ANGLE3D(FRnd()*-3.0f-3.0f, FRnd()*3.0f-2.0f, 0);
        ShootProjectile(EptProjectile, vPos, aAngle);
      }
    }
  };

  void BossFirePredictedLavaRock(FLOAT3D vFireingRel)
  {
    FLOAT3D vShooting = GetPlacement().pl_PositionVector+vFireingRel*GetRotationMatrix();
    FLOAT3D vTarget = m_penEnemy->GetPlacement().pl_PositionVector;
    FLOAT3D vSpeedDest = ((CMovableEntity&) *m_penEnemy).en_vCurrentTranslationAbsolute;
    FLOAT fLaunchSpeed;
    FLOAT fRelativeHdg;
  
    FLOAT fDistanceFactor = ClampUp( (vShooting-vTarget).Length()/150.0f, 1.0f)-0.75f;
    FLOAT fPitch = fDistanceFactor*45.0f;
  
    // calculate parameters for predicted angular launch curve
    EntityInfo *peiTarget = (EntityInfo*) (m_penEnemy->GetEntityInfo());
    CalculateAngularLaunchParams( vShooting, peiTarget->vTargetCenter[1]-6.0f/3.0f, vTarget, 
      vSpeedDest, fPitch, fLaunchSpeed, fRelativeHdg);

    // target enemy body
    FLOAT3D vShootTarget;
    GetEntityInfoPosition(m_penEnemy, peiTarget->vTargetCenter, vShootTarget);
    // launch
    CPlacement3D pl;
    PrepareFreeFlyingProjectile(pl, vShootTarget, vFireingRel, ANGLE3D( fRelativeHdg, fPitch, 0));
    CEntityPointer penProjectile = CreateEntity(pl, CLASS_PROJECTILE);
    ELaunchProjectile eLaunch;
    eLaunch.penLauncher = this;
    eLaunch.prtType = PRT_LAVAMAN_BIG_BOMB;
    eLaunch.fSpeed = fLaunchSpeed;
    penProjectile->Initialize(eLaunch);
  }

  /* Shake ground */
  void ShakeItBaby(FLOAT tmShaketime, FLOAT fPower)
  {
    CWorldSettingsController *pwsc = GetWSC(this);
    if (pwsc!=NULL) {
      pwsc->m_tmShakeStarted = tmShaketime;
      pwsc->m_vShakePos = GetPlacement().pl_PositionVector;
      pwsc->m_fShakeFalloff = 450.0f;
      pwsc->m_fShakeFade = 3.0f;

      pwsc->m_fShakeIntensityZ = 0;
      pwsc->m_tmShakeFrequencyZ = 5.0f;
      pwsc->m_fShakeIntensityY = 0.1f*fPower;
      pwsc->m_tmShakeFrequencyY = 5.0f;
      pwsc->m_fShakeIntensityB = 2.5f*fPower;
      pwsc->m_tmShakeFrequencyB = 7.2f;

      pwsc->m_bShakeFadeIn = FALSE;
    }
  }

  void SpawnShockwave(FLOAT fSize)
  {
    CPlacement3D pl = GetPlacement();
    pl.pl_PositionVector(2) += 0.1f;
    CEntityPointer penShockwave = CreateEntity(pl, CLASS_BASIC_EFFECT);
        
    ESpawnEffect eSpawnEffect;
    eSpawnEffect.colMuliplier = C_WHITE|CT_OPAQUE;
    eSpawnEffect.betType = BET_CANNONSHOCKWAVE;
    eSpawnEffect.vStretch = FLOAT3D(fSize, fSize, fSize);
    penShockwave->Initialize(eSpawnEffect);
  }

  // hit ground
  void HitGround(void) {
    FLOAT3D vSource;
    if( m_penEnemy != NULL)
    {
      vSource = GetPlacement().pl_PositionVector +
      FLOAT3D(m_penEnemy->en_mRotation(1, 2), m_penEnemy->en_mRotation(2, 2), m_penEnemy->en_mRotation(3, 2));
    }
    else
    {
      vSource = GetPlacement().pl_PositionVector;
    }

    // damage
    if (m_EecChar==ELC_LARGE) {
      InflictRangeDamage(this, DMT_IMPACT, 150.0f, vSource, 7.5f, m_fCloseDistance);
      ShakeItBaby(_pTimer->CurrentTick(), 5.0f);
      SpawnShockwave(10.0f);
    } else if (m_EecChar==ELC_BIG) {
      InflictRangeDamage(this, DMT_IMPACT, 75.0f, vSource, 5.0f, m_fCloseDistance);
      ShakeItBaby(_pTimer->CurrentTick(), 2.0f);
      SpawnShockwave(3.0f);
    } else {
      InflictRangeDamage(this, DMT_IMPACT, 25.0f, vSource, 2.5f, m_fCloseDistance);
      SpawnShockwave(1.0f);
    }
  };

  // fire water
/*  void FireWater(void) {
    // target enemy body
    EntityInfo *peiTarget = (EntityInfo*) (m_penEnemy->GetEntityInfo());
    FLOAT3D vShootTarget;
    GetEntityInfoPosition(m_penEnemy, peiTarget->vTargetCenter, vShootTarget);

    // water projectile
    CPlacement3D pl;
    EWater ew;
    ew.penLauncher = this;
    if (m_EecChar==ELC_LARGE) {
      ew.EwsSize = WTS_LARGE;
      // launch
      PreparePropelledProjectile(pl, vShootTarget, WATER_LARGE_LEFT, ANGLE3D(0, 0, 0));
      CEntityPointer penWater = CreateEntity(pl, CLASS_WATER);
      penWater->Initialize(ew);
      // launch
      PreparePropelledProjectile(pl, vShootTarget, WATER_LARGE_RIGHT, ANGLE3D(0, 0, 0));
      penWater = CreateEntity(pl, CLASS_WATER);
      penWater->Initialize(ew);
    } else if (m_EecChar==ELC_BIG) {
      ew.EwsSize = WTS_BIG;
      // launch
      PreparePropelledProjectile(pl, vShootTarget, WATER_BIG_LEFT, ANGLE3D(0, 0, 0));
      CEntityPointer penWater = CreateEntity(pl, CLASS_WATER);
      penWater->Initialize(ew);
      // launch
      PreparePropelledProjectile(pl, vShootTarget, WATER_BIG_RIGHT, ANGLE3D(0, 0, 0));
      penWater = CreateEntity(pl, CLASS_WATER);
      penWater->Initialize(ew);
    } else {
      ew.EwsSize = WTS_SMALL;
      // launch
      PreparePropelledProjectile(pl, vShootTarget, WATER_LEFT, ANGLE3D(0, 0, 0));
      CEntityPointer penWater = CreateEntity(pl, CLASS_WATER);
      penWater->Initialize(ew);
      // launch
      PreparePropelledProjectile(pl, vShootTarget, WATER_RIGHT, ANGLE3D(0, 0, 0));
      penWater = CreateEntity(pl, CLASS_WATER);
      penWater->Initialize(ew);
    }
  };
*/

  // add attachments
  void AddAttachments(void) {
    switch (m_EetType) {
/*      case ELT_AIR:
        if (GetModelObject()->GetAttachmentModel(AIRMAN_ATTACHMENT_TWISTER)==NULL) {
          AddAttachmentToModel(this, *GetModelObject(), AIRMAN_ATTACHMENT_TWISTER,
            MODEL_AIR_TWISTER, TEXTURE_AIR, 0, 0, 0);
          GetModelObject()->mo_ColorMask &= ~AIRMAN_PART_BODYDOWN;
        }
        break;
      case ELT_ICE:
        if (GetModelObject()->GetAttachmentModel(ICEMAN_ATTACHMENT_ICEPICK)==NULL) {
          AddAttachmentToModel(this, *GetModelObject(), ICEMAN_ATTACHMENT_ICEPICK,
            MODEL_ICE_PICK, TEXTURE_ICE, TEXTURE_ICE, TEX_SPEC_STRONG, 0);
        }
        break;*/
      case ELT_LAVA:
        if (GetModelObject()->GetAttachmentModel(ELEMENTALLAVA_ATTACHMENT_BODY_FLARE)==NULL) {
          AddAttachmentToModel(this, *GetModelObject(), ELEMENTALLAVA_ATTACHMENT_BODY_FLARE, MODEL_LAVA_BODY_FLARE, TEXTURE_LAVA_FLARE, 0, 0, 0);
          AddAttachmentToModel(this, *GetModelObject(), ELEMENTALLAVA_ATTACHMENT_RIGHT_HAND_FLARE, MODEL_LAVA_HAND_FLARE, TEXTURE_LAVA_FLARE, 0, 0, 0);
          AddAttachmentToModel(this, *GetModelObject(), ELEMENTALLAVA_ATTACHMENT_LEFT_HAND_FLARE, MODEL_LAVA_HAND_FLARE, TEXTURE_LAVA_FLARE, 0, 0, 0);
        }
        break;
/*      case ELT_STONE:
        if (GetModelObject()->GetAttachmentModel(STONEMAN_ATTACHMENT_MAUL)==NULL) {
          AddAttachmentToModel(this, *GetModelObject(), STONEMAN_ATTACHMENT_MAUL,
            MODEL_STONE_MAUL, TEXTURE_STONE, 0, 0, 0);
        }
        break;
      case ELT_WATER:
        if (GetModelObject()->GetAttachmentModel(WATERMAN_ATTACHMENT_BODY_FLARE)==NULL) {
          AddAttachmentToModel(this, *GetModelObject(), WATERMAN_ATTACHMENT_BODY_FLARE,
            MODEL_WATER_BODY_FLARE, TEXTURE_WATER_FLARE, 0, 0, 0);
        }
        break;*/
    }
    GetModelObject()->StretchModel(GetModelObject()->mo_Stretch);
    ModelChangeNotify();
  };

  // remove attachments
  void RemoveAttachments(void) {
    switch (m_EetType) {
/*      case ELT_AIR:
        RemoveAttachmentFromModel(*GetModelObject(), AIRMAN_ATTACHMENT_TWISTER);
        GetModelObject()->mo_ColorMask |= AIRMAN_PART_BODYDOWN;
        break;
      case ELT_ICE:
        RemoveAttachmentFromModel(*GetModelObject(), ICEMAN_ATTACHMENT_ICEPICK);
        break;*/
      case ELT_LAVA:
        RemoveAttachmentFromModel(*GetModelObject(), ELEMENTALLAVA_ATTACHMENT_BODY_FLARE);
        RemoveAttachmentFromModel(*GetModelObject(), ELEMENTALLAVA_ATTACHMENT_RIGHT_HAND_FLARE);
        RemoveAttachmentFromModel(*GetModelObject(), ELEMENTALLAVA_ATTACHMENT_LEFT_HAND_FLARE);
        break;
/*      case ELT_STONE:
        RemoveAttachmentFromModel(*GetModelObject(), STONEMAN_ATTACHMENT_MAUL);
        break;
      case ELT_WATER:
        RemoveAttachmentFromModel(*GetModelObject(), WATERMAN_ATTACHMENT_BODY_FLARE);
        break;*/
    }
  };

/************************************************************
 *                 BLOW UP FUNCTIONS                        *
 ************************************************************/
  // spawn body parts
  void BlowUp(void) {
    // get your size
    FLOATaabbox3D box;
    GetBoundingBox(box);
    FLOAT fEntitySize = box.Size().MaxNorm()/2;

    FLOAT3D vNormalizedDamage = m_vDamage-m_vDamage*(m_fBlowUpAmount/m_vDamage.Length());
    vNormalizedDamage /= Sqrt(vNormalizedDamage.Length());
    vNormalizedDamage *= 1.75f;
/*
    FLOAT3D vBodySpeed = en_vCurrentTranslationAbsolute-en_vGravityDir*(en_vGravityDir%en_vCurrentTranslationAbsolute);

    // spawn debris

	INDEX iCount = 1;
    switch (m_EecChar) {
      case ELC_SMALL: iCount = 3; break;
      case ELC_BIG: iCount = 5; break;
      case ELC_LARGE: iCount = 7; break;
    }
    switch (m_EetType) {
      case ELT_ICE: {
        Debris_Begin(EIBT_ICE, DPT_NONE, BET_NONE, fEntitySize, vNormalizedDamage, vBodySpeed, 1.0f, 0.0f);
        for (iDebris=0; iDebris<iCount; iDebris++) {
          CEntityPointer pen;
          pen = Debris_Spawn(this, this, MODEL_ELEM_ICE, TEXTURE_ELEM_ICE, 0, 0, 0, 0, 0.5f,
            FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
          AddAttachmentToModel(this, *(pen->GetModelObject()), ICEPYRAMID_ATTACHMENT_FLARE,
            MODEL_ELEM_ICE_FLARE, TEXTURE_ELEM_FLARE, 0, 0, 0);
          pen->GetModelObject()->StretchModel(pen->GetModelObject()->mo_Stretch);
          ModelChangeNotify();
        }}
        break;
      case ELT_LAVA: {
        Debris_Begin(EIBT_FIRE, DPT_NONE, BET_NONE, fEntitySize, vNormalizedDamage, vBodySpeed, 1.0f, 0.0f);
        for (iDebris=0; iDebris<iCount; iDebris++) {
          CEntityPointer pen;
          pen = Debris_Spawn(this, this, MODEL_ELEM_LAVASTONE, TEXTURE_ELEM_LAVASTONE, 0, 0, 0, 0, 0.5f,
            FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
          AddAttachmentToModel(this, *(pen->GetModelObject()), LAVASTONE_ATTACHMENT_FLARE,
            MODEL_ELEM_LAVASTONE_FLARE, TEXTURE_ELEM_FLARE, 0, 0, 0);
          pen->GetModelObject()->StretchModel(pen->GetModelObject()->mo_Stretch);
          ModelChangeNotify();
        }}
        break;
      case ELT_STONE: {
        Debris_Begin(EIBT_ROCK, DPT_NONE, BET_NONE, fEntitySize, vNormalizedDamage, vBodySpeed, 1.0f, 0.0f);
        for (iDebris=0; iDebris<iCount; iDebris++) {
          Debris_Spawn(this, this, MODEL_ELEM_STONE, TEXTURE_ELEM_STONE, 0, 0, 0, 0, 0.5f,
            FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
        }}
        break;
    }
    */

    // hide yourself (must do this after spawning debris)
    SwitchToEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);
  };


  // adjust sound and watcher parameters here if needed
  void EnemyPostInit(void) 
  {
    if (m_EecChar==ELC_LARGE && m_EetType==ELT_LAVA)
    {
      m_soBackground.Set3DParameters(400.0f, 0.0f, 1.0f, 1.0f);
      m_soSound.Set3DParameters(400.0f, 50.0f, 1.0f, 1.0f);
      m_soFireL.Set3DParameters(400.0f, 50.0f, 1.0f, 1.0f);
      m_soFireR.Set3DParameters(400.0f, 50.0f, 1.0f, 1.0f);
    }
    else if (m_EecChar==ELC_BIG && m_EetType==ELT_LAVA)
    {
      m_soBackground.Set3DParameters(150.0f, 15.0f, 0.5f, 1.0f);
      m_soSound.Set3DParameters(200.0f, 0.0f, 1.0f, 1.0f);
      m_soFireL.Set3DParameters(200.0f, 0.0f, 1.0f, 1.0f);
      m_soFireR.Set3DParameters(200.0f, 0.0f, 1.0f, 1.0f);
    }
  };

procedures:
/************************************************************
 *                    CLASS INTERNAL                        *
 ************************************************************/
  FallOnFloor(EVoid) {
    // drop to floor
    SetPhysicsFlags(EPF_MODEL_WALKING);
    // wait at most 10 seconds
    wait (10.0f) {
      on (ETimer) : { stop; }
      on (EBegin) : { resume; }
      // if a brush is touched
      on (ETouch et) : {
        if (et.penOther->GetRenderType()&RT_BRUSH) {
          // stop waiting
          StopMoving();
          stop;
        }
        resume;
      }
      otherwise() : { resume; }
    }
    StartModelAnim(ELEMENTALLAVA_ANIM_MELTUP, 0);
    return EReturn();
  };



/************************************************************
 *                      FIRE PROCEDURES                     *
 ************************************************************/
  //
  // STONEMAN
  //
/*  StonemanFire(EVoid) {
    StartModelAnim(STONEMAN_ANIM_ATTACK05, 0);
    autowait(0.7f);
    // throw rocks
    if (m_EecChar==ELC_LARGE) {
      ThrowRocks(PRT_STONEMAN_LARGE_FIRE);
    } else if (m_EecChar==ELC_BIG) {
      ThrowRocks(PRT_STONEMAN_BIG_FIRE);
    } else {
      ThrowRocks(PRT_STONEMAN_FIRE);
    }
    PlaySound(m_soSound, SOUND_LAVA_FIRE, SOF_3D);
    autowait(0.9f);
    // stand a while
    StandingAnim();
    autowait(FRnd()/3+_pTimer->TickQuantum);
    return EReturn();
  };

  StonemanHit(EVoid) {
    StartModelAnim(STONEMAN_ANIM_ATTACK06, 0);
    autowait(0.6f);
    HitGround();
    PlaySound(m_soSound, SOUND_LAVA_KICK, SOF_3D);
    autowait(0.5f);
    // stand a while
    StandingAnim();
    autowait(FRnd()/3+_pTimer->TickQuantum);
    return EReturn();
  };
  */

  //
  // LAVAMAN
  //

  LavamanFire(EVoid)
  {
    m_bSpawnEnabled = TRUE;
    // shoot projectiles
    if (m_EecChar==ELC_LARGE)
    {
      CModelObject &mo = *GetModelObject();
      FLOAT tmWait = mo.GetAnimLength( mo.ao_iCurrentAnim )-mo.GetPassedTime();
      StartModelAnim(ELEMENTALLAVA_ANIM_ATTACKBOSS, AOF_SMOOTHCHANGE);
      autowait(tmWait+0.95f);
      BossFirePredictedLavaRock(LAVAMAN_FIRE_LARGE_RIGHT);
      PlaySound(m_soFireR, SOUND_LAVA_FIRE, SOF_3D);
      autowait(2.0150f-0.95f);
      BossFirePredictedLavaRock(LAVAMAN_FIRE_LARGE_LEFT);
      PlaySound(m_soFireL, SOUND_LAVA_FIRE, SOF_3D);
      StartModelAnim(ELEMENTALLAVA_ANIM_WALKBIG, AOF_SMOOTHCHANGE);
      autocall CMovableModelEntity::WaitUntilScheduledAnimStarts() EReturn;
      MaybeSwitchToAnotherPlayer();
      // set next shoot time
      m_fShootTime = _pTimer->CurrentTick() + m_fAttackFireTime*(1.0f + FRnd()/5.0f);
      return EReturn();
    }
    else if (m_EecChar==ELC_BIG)
    {
      CModelObject &mo = *GetModelObject();
      FLOAT tmWait = mo.GetAnimLength( mo.ao_iCurrentAnim )-mo.GetPassedTime();
      StartModelAnim(ELEMENTALLAVA_ANIM_ATTACKLEFTHAND, AOF_SMOOTHCHANGE);
      autowait(tmWait+0.90f);
      FLOAT3D vShooting = GetPlacement().pl_PositionVector;
      FLOAT3D vTarget = m_penEnemy->GetPlacement().pl_PositionVector;
      FLOAT3D vSpeedDest = ((CMovableEntity&) *m_penEnemy).en_vCurrentTranslationAbsolute;
      FLOAT fLaunchSpeed;
      FLOAT fRelativeHdg;
      
      FLOAT fPitch = 20.0f;
      
      // calculate parameters for predicted angular launch curve
      EntityInfo *peiTarget = (EntityInfo*) (m_penEnemy->GetEntityInfo());
      CalculateAngularLaunchParams( vShooting, LAVAMAN_FIRE_BIG(2)-peiTarget->vTargetCenter[1]-1.5f/3.0f, vTarget, 
        vSpeedDest, fPitch, fLaunchSpeed, fRelativeHdg);

      // target enemy body
      FLOAT3D vShootTarget;
      GetEntityInfoPosition(m_penEnemy, peiTarget->vTargetCenter, vShootTarget);
      // launch
      CPlacement3D pl;
      PrepareFreeFlyingProjectile(pl, vShootTarget, LAVAMAN_FIRE_BIG, ANGLE3D( fRelativeHdg, fPitch, 0));
      CEntityPointer penProjectile = CreateEntity(pl, CLASS_PROJECTILE);
      ELaunchProjectile eLaunch;
      eLaunch.penLauncher = this;
      eLaunch.prtType = PRT_LAVAMAN_BOMB;
      eLaunch.fSpeed = fLaunchSpeed;
      penProjectile->Initialize(eLaunch);
      PlaySound(m_soSound, SOUND_LAVA_FIRE, SOF_3D);
    }
    else if (TRUE)
    {
      CModelObject &mo = *GetModelObject();
      FLOAT tmWait = mo.GetAnimLength( mo.ao_iCurrentAnim )-mo.GetPassedTime();
      StartModelAnim(ELEMENTALLAVA_ANIM_ATTACKLEFTHAND, AOF_SMOOTHCHANGE);
      autowait(tmWait+0.8f);
      ShootProjectile(PRT_LAVAMAN_STONE, LAVAMAN_FIRE_SMALL, ANGLE3D(0, 0, 0));
      PlaySound(m_soSound, SOUND_LAVA_FIRE, SOF_3D);
    }

    autowait( GetModelObject()->GetAnimLength( ELEMENTALLAVA_ANIM_ATTACKLEFTHAND) - 0.9f);

    StandingAnim();
    autowait(_pTimer->TickQuantum);

    if (m_EecChar!=ELC_SMALL) {
      MaybeSwitchToAnotherPlayer();
    }

    // set next shoot time
    m_fShootTime = _pTimer->CurrentTick() + m_fAttackFireTime*(1.0f + FRnd()/5.0f);

    return EReturn();
  };

  LavamanStones(EVoid)
  {
    StartModelAnim(ELEMENTALLAVA_ANIM_ATTACKLEFTHAND, 0);
    autowait(0.7f);
    // throw rocks
    if (m_EecChar==ELC_LARGE) {
      ThrowRocks(PRT_LAVAMAN_STONE);
    } else if (m_EecChar==ELC_BIG) {
      ThrowRocks(PRT_LAVAMAN_STONE);
    } else {
      ThrowRocks(PRT_LAVAMAN_STONE);
    }
    PlaySound(m_soSound, SOUND_LAVA_FIRE, SOF_3D);
    autowait(0.9f);
    // stand a while
    StandingAnim();
    autowait(FRnd()/3+_pTimer->TickQuantum);
    return EReturn();
  };

  LavamanHit(EVoid)
  {
    StartModelAnim(ELEMENTALLAVA_ANIM_ATTACKTWOHANDS, 0);
    autowait(0.6f);
    HitGround();
    PlaySound(m_soFireL, SOUND_LAVA_KICK, SOF_3D);
    StartModelAnim(ELEMENTALLAVA_ANIM_WALKBIG, AOF_SMOOTHCHANGE);
    autocall CMovableModelEntity::WaitUntilScheduledAnimStarts() EReturn;
    return EReturn();
  };

/*
  //
  // ICEMAN
  //
  IcemanFire(EVoid) {
    StartModelAnim(STONEMAN_ANIM_ATTACK05, 0);
    autowait(0.7f);
    // throw rocks
    if (m_EecChar==ELC_LARGE) {
      ThrowRocks(PRT_ICEMAN_LARGE_FIRE);
    } else if (m_EecChar==ELC_BIG) {
      ThrowRocks(PRT_ICEMAN_BIG_FIRE);
    } else {
      ThrowRocks(PRT_ICEMAN_FIRE);
    }
    PlaySound(m_soSound, SOUND_LAVA_FIRE, SOF_3D);
    autowait(0.9f);
    // stand a while
    StandingAnim();
    autowait(FRnd()/3+_pTimer->TickQuantum);
    return EReturn();
  };

  IcemanHit(EVoid) {
    StartModelAnim(STONEMAN_ANIM_ATTACK01, 0);
    autowait(0.6f);
    HitGround();
    PlaySound(m_soSound, SOUND_LAVA_KICK, SOF_3D);
    autowait(0.5f);
    // stand a while
    StandingAnim();
    autowait(FRnd()/3+_pTimer->TickQuantum);
    return EReturn();
  };


  //
  // AIRMAN
  //
  AirmanFire(EVoid) {
    StartModelAnim(STONEMAN_ANIM_ATTACK06, 0);
    autowait(1.0f);
    // spawn twister
    CPlacement3D pl = m_penEnemy->GetPlacement();
    FLOAT fR, fA;
    ETwister et;
    fA = FRnd()*360.0f;
    if (m_EecChar==ELC_LARGE) {
      fR = FRnd()*10.0f;
      et.EtsSize = TWS_LARGE;
    } else if (m_EecChar==ELC_BIG) {
      fR = FRnd()*7.5f;
      et.EtsSize = TWS_BIG;
    } else {
      fR = FRnd()*5.0f;
      et.EtsSize = TWS_SMALL;
    }
    pl.pl_PositionVector += FLOAT3D(CosFast(fA)*fR, 0, SinFast(fA)*fR);;
    CEntityPointer penTwister = CreateEntity(pl, CLASS_TWISTER);
    et.penOwner = this;
    penTwister->Initialize(et);
    PlaySound(m_soSound, SOUND_LAVA_FIRE, SOF_3D);
    autowait(0.6f);
    // stand a while
    StandingAnim();
    autowait(FRnd()/3+_pTimer->TickQuantum);
    return EReturn();
  };


  //
  // WATERMAN
  //
  WatermanFire(EVoid) {
    StartModelAnim(STONEMAN_ANIM_ATTACK02, 0);
    autowait(0.5f);
    // throw rocks
    FireWater();
    PlaySound(m_soSound, SOUND_LAVA_FIRE, SOF_3D);
    autowait(0.6f);
    // stand a while
    StandingAnim();
    autowait(FRnd()/3+_pTimer->TickQuantum);
    return EReturn();
  };
  */


/************************************************************
 *                PROCEDURES WHEN HARMED                    *
 ************************************************************/
  // Play wound animation and falling body part
  BeWounded(EDamage eDamage) : CEnemyBase::BeWounded {
    // spawn additional elemental
    if( m_bSpawnEnabled)
    {
      SpawnNewElemental();
    }
    jump CEnemyBase::BeWounded(eDamage);
  };

/************************************************************
 *                 CHANGE STATE PROCEDURES                  *
 ************************************************************/
   // box to normal
  BoxToNormal(EVoid) {
    m_EesCurrentState = ELS_NORMAL;
    SetPhysicsFlags(EPF_MODEL_WALKING);
    ChangeCollisionBoxIndexWhenPossible(STONEMAN_COLLISION_BOX_NORMAL);
    PlaySound(m_soFireL, SOUND_LAVA_GROW, SOF_3D);
    StartModelAnim(STONEMAN_ANIM_MORPHBOXUP, 0);
    AddAttachments();
    autowait(GetModelObject()->GetAnimLength(STONEMAN_ANIM_MORPHBOXUP));
    return EReturn();
  };

  // normal to box
/*  NormalToBox(EVoid) {
    StartModelAnim(STONEMAN_ANIM_MORPHBOXDOWN, 0);
    autowait(GetModelObject()->GetAnimLength(STONEMAN_ANIM_MORPHBOXDOWN));
    m_EesCurrentState = ELS_BOX;
    SetPhysicsFlags(EPF_BOX_PLANE_ELEMENTAL);
    ChangeCollisionBoxIndexWhenPossible(STONEMAN_COLLISION_BOX_BOX);
    RemoveAttachments();
    return EReturn();
  };*/

  // plane to normal
  PlaneToNormal(EVoid) {
    m_EesCurrentState = ELS_NORMAL;
    SwitchToModel();
    SetPhysicsFlags(EPF_MODEL_WALKING);
    ChangeCollisionBoxIndexWhenPossible(ELEMENTALLAVA_COLLISION_BOX_NORMAL);
    PlaySound(m_soFireL, SOUND_LAVA_GROW, SOF_3D);
    INDEX iAnim;
    if (m_EetType == ELT_LAVA) {
      iAnim = ELEMENTALLAVA_ANIM_MELTUP;
    } else {
      iAnim = 0; // DG: initialize to deterministic value
//      iAnim = STONEMAN_ANIM_MORPHPLANEUP;
    }
    StartModelAnim(iAnim, 0);
    AddAttachments();
    autowait(GetModelObject()->GetAnimLength(iAnim));
    return EReturn();
  };

/************************************************************
 *                A T T A C K   E N E M Y                   *
 ************************************************************/
  InitializeAttack(EVoid) : CEnemyBase::InitializeAttack {
    // change state from box to normal
    if (m_EesCurrentState==ELS_BOX)
    {
      autocall BoxToNormal() EReturn;
    }
    // change state from plane to normal
    else if (m_EesCurrentState==ELS_PLANE)
    {
      autocall PlaneToNormal() EReturn;
    }
    jump CEnemyBase::InitializeAttack();
  };

  Fire(EVoid) : CEnemyBase::Fire {
    // fire projectile
    switch (m_EetType) {
//      case ELT_STONE: jump StonemanFire(); break;
      case ELT_LAVA: jump LavamanFire(); break;
//      case ELT_ICE: jump IcemanFire(); break;
//      case ELT_AIR: jump AirmanFire(); break;
//      case ELT_WATER: jump WatermanFire(); break;
    }
    return EReturn();
  };

  Hit(EVoid) : CEnemyBase::Hit {
    // hit ground
    switch (m_EetType) {
//      case ELT_STONE: jump StonemanHit(); break;
      case ELT_LAVA: jump LavamanHit(); break;
//      case ELT_ICE: jump IcemanHit(); break;
//      case ELT_AIR: jump AirmanFire(); break;
//      case ELT_WATER: jump WatermanFire(); break;
    }
    return EReturn();
  };

/************************************************************
 *                    D  E  A  T  H                         *
 ************************************************************/
  Death(EVoid) : CEnemyBase::Death
  {
    if (m_bSpawnOnBlowUp && (m_EecChar==ELC_LARGE || m_EecChar==ELC_BIG)) {
      SpawnNewElemental();
      SpawnNewElemental();
    }
    // air fade out
    if (m_EetType == ELT_AIR) {
      m_fFadeStartTime = _pTimer->CurrentTick();
      m_bFadeOut = TRUE;
      m_fFadeTime = 2.0f;
      autowait(m_fFadeTime);
    }
    autocall CEnemyBase::Death() EEnd;
    GetModelObject()->mo_toBump.SetData( NULL);
    return EEnd();
  };

  BossAppear(EVoid)
  {
    autowait(2.0f);
    m_fFadeStartTime = _pTimer->CurrentTick();
    GetModelObject()->PlayAnim(ELEMENTALLAVA_ANIM_ANGER, 0);
    PlaySound(m_soSound, SOUND_LAVA_ANGER, SOF_3D);
    autowait(GetModelObject()->GetAnimLength(ELEMENTALLAVA_ANIM_ANGER)-_pTimer->TickQuantum);

    StartModelAnim(ELEMENTALLAVA_ANIM_ATTACKTWOHANDS, AOF_SMOOTHCHANGE);
    autowait(0.7f);
    HitGround();
    PlaySound(m_soFireL, SOUND_LAVA_KICK, SOF_3D);
    autowait(GetModelObject()->GetAnimLength(ELEMENTALLAVA_ANIM_ATTACKTWOHANDS)-0.7f-_pTimer->TickQuantum);

    StartModelAnim(ELEMENTALLAVA_ANIM_ATTACKTWOHANDS, 0);
    autowait(0.6f);
    HitGround();
    PlaySound(m_soFireR, SOUND_LAVA_KICK, SOF_3D);
    autowait(GetModelObject()->GetAnimLength(ELEMENTALLAVA_ANIM_ATTACKTWOHANDS)-0.6f-_pTimer->TickQuantum);


    return EReturn();
  }

  // overridable called before main enemy loop actually begins
  PreMainLoop(EVoid) : CEnemyBase::PreMainLoop
  {
    // if spawned by other entity
    if (m_bSpawned) {
      m_bSpawned = FALSE;
      m_bCountAsKill = FALSE;
      // wait till touching the ground
      autocall FallOnFloor() EReturn;
    }

    if ((m_EecChar==ELC_LARGE || m_EecChar==ELC_BIG) && m_EetType==ELT_LAVA)
    {
      PlaySound(m_soBackground, SOUND_LAVA_LAVABURN, SOF_3D|SOF_LOOP);
    }

    if( m_EecChar==ELC_LARGE)
    {
      autocall BossAppear() EReturn;
    }
    return EReturn();
  }

/************************************************************
 *                       M  A  I  N                         *
 ************************************************************/
  Main(EVoid) {
    if (m_EetType!=ELT_LAVA) {
      m_EetType=ELT_LAVA;
    }
    // declare yourself as a model
    InitAsModel();
    // movable
    if (m_bMovable) {
      SetPhysicsFlags(EPF_MODEL_WALKING);
    // non movable
    } else {
      SetPhysicsFlags(EPF_MODEL_IMMATERIAL|EPF_MOVABLE);
    }
    // air elemental
    if (m_EetType==ELT_AIR) {
      SetCollisionFlags(ECF_AIR);
    // solid elemental
    } else {
      SetCollisionFlags(ECF_MODEL);
    }
    SetFlags(GetFlags()|ENF_ALIVE);
    en_fDensity = m_fDensity;
    m_fSpawnDamage = 1e6f;
    m_fDamageWounded = 1e6f;
    m_bSpawnEnabled = FALSE;
    m_bBoss = FALSE;

    // set your appearance
    switch (m_EetType) {
/*      case ELT_AIR: 
        SetComponents(this, *GetModelObject(), MODEL_AIR, TEXTURE_AIR, 0, 0, 0); 
        break;
      case ELT_ICE: 
        SetComponents(this, *GetModelObject(), MODEL_ICE, TEXTURE_ICE, TEXTURE_ICE, TEX_SPEC_STRONG, 0); 
        break;*/
      case ELT_LAVA:
        m_fBlowUpAmount = 1E30f;
        SetComponents(this, *GetModelObject(), MODEL_LAVA, TEXTURE_LAVA, 0, 0, TEXTURE_LAVA_DETAIL);
        break;
/*      case ELT_STONE: 
        SetComponents(this, *GetModelObject(), MODEL_STONE, TEXTURE_STONE, 0, 0, 0); 
        break;
      case ELT_WATER: 
        SetComponents(this, *GetModelObject(), MODEL_WATER, TEXTURE_WATER, TEXTURE_WATER, TEX_SPEC_STRONG, 0); 
        break;*/
    }
    ModelChangeNotify();

    // character settings
    if (m_EecChar==ELC_LARGE)
    {
      // this one is boss!
      m_sptType = SPT_SMALL_LAVA_STONES;
      m_bBoss = TRUE;
      SetHealth(10000.0f);
      m_fMaxHealth = 10000.0f;
      // after loosing this ammount of damage we will spawn new elemental
      m_fSpawnDamage = 2000.0f;
      // setup moving speed
      m_fWalkSpeed = FRnd()/2 + 1.0f;
      m_aWalkRotateSpeed = AngleDeg(FRnd()*10.0f + 25.0f);
      m_fAttackRunSpeed = FRnd() + 2.0f;
      m_aAttackRotateSpeed = AngleDeg(FRnd()*50 + 245.0f);
      m_fCloseRunSpeed = FRnd() + 2.0f;
      m_aCloseRotateSpeed = AngleDeg(FRnd()*50 + 245.0f);
      // setup attack distances
      m_fAttackDistance = 300.0f;
      m_fCloseDistance = 60.0f;
      m_fStopDistance = 30.0f;
      m_fAttackFireTime = 0.5f;
      m_fCloseFireTime = 1.0f;
      m_fIgnoreRange = 600.0f;
      m_iScore = 50000;
    }
    else if (m_EecChar==ELC_BIG)
    {
      m_sptType = SPT_LAVA_STONES;
      SetHealth(800.0f);
      m_fMaxHealth = 800.0f;
      // after loosing this ammount of damage we will spawn new elemental
      m_fSpawnDamage = 500.0f;
      // setup moving speed
      m_fWalkSpeed = FRnd() + 1.5f;
      m_aWalkRotateSpeed = AngleDeg(FRnd()*10.0f + 25.0f);
      m_fAttackRunSpeed = FRnd()*1.0f + 6.0f;
      m_aAttackRotateSpeed = AngleDeg(FRnd()*50 + 300.0f);
      m_fCloseRunSpeed = FRnd()*2.0f + 2.0f;
      m_aCloseRotateSpeed = AngleDeg(FRnd()*50 + 300.0f);
      // setup attack distances
      m_fAttackDistance = 150.0f;
      m_fCloseDistance = 20.0f;
      m_fStopDistance = 5.0f;
      m_fAttackFireTime = 0.5f;
      m_fCloseFireTime = 1.0f;
      m_fIgnoreRange = 400.0f;
      // damage/explode properties
      m_iScore = 2500;
    }
    else
    {
      m_sptType = SPT_LAVA_STONES;
      SetHealth(100.0f);
      m_fMaxHealth = 100.0f;
      // setup moving speed
      m_fWalkSpeed = FRnd() + 1.5f;
      m_aWalkRotateSpeed = AngleDeg(FRnd()*10.0f + 25.0f);
      m_fAttackRunSpeed = FRnd()*2.0f + 6.0f;
      m_aAttackRotateSpeed = AngleDeg(FRnd()*50 + 500.0f);
      m_fCloseRunSpeed = FRnd()*3.0f + 4.0f;
      m_aCloseRotateSpeed = AngleDeg(FRnd()*50 + 500.0f);
      // setup attack distances
      m_fAttackDistance = 100.0f;
      m_fCloseDistance = 10.0f;
      m_fStopDistance = 5.0f;
      m_fAttackFireTime = 1.5f;
      m_fCloseFireTime = 1.0f;
      m_fIgnoreRange = 200.0f;
      // damage/explode properties
      m_iScore = 500;
    }

    // non movable
    if (!m_bMovable)
    {
      m_EesStartState = ELS_NORMAL;
      m_bSpawnWhenHarmed = FALSE;
      m_bSpawnOnBlowUp = FALSE;
      // fire count
      if (m_iFireCount <= 0)
      {
        WarningMessage("Entity: %s - Fire count must be greater than zero", (const char *) GetName());
        m_iFireCount = 1;
      }
    }

    // state and flare attachments
    m_EesCurrentState = m_EesStartState;
    RemoveAttachments();
    switch (m_EesCurrentState) {
      case ELS_NORMAL:
        SetPhysicsFlags(EPF_MODEL_WALKING);
        AddAttachments();
        break;
      case ELS_BOX:
        SetPhysicsFlags(EPF_BOX_PLANE_ELEMENTAL);
        break;
      case ELS_PLANE:
        SetPhysicsFlags(EPF_MODEL_IMMATERIAL|EPF_MOVABLE);
        SwitchToEditorModel();
        break;
    }
    StandingAnim();

    // stretch
    if (m_EecChar==ELC_SMALL) {
      GetModelObject()->StretchModel(FLOAT3D(LAVAMAN_SMALL_STRETCH, LAVAMAN_SMALL_STRETCH, LAVAMAN_SMALL_STRETCH));
    }
    else if (m_EecChar==ELC_LARGE) {
      GetModelObject()->StretchModel(FLOAT3D(LAVAMAN_LARGE_STRETCH, LAVAMAN_LARGE_STRETCH, LAVAMAN_LARGE_STRETCH));
    } else if (m_EecChar==ELC_BIG) {
      GetModelObject()->StretchModel(FLOAT3D(LAVAMAN_BIG_STRETCH, LAVAMAN_BIG_STRETCH, LAVAMAN_BIG_STRETCH));
    }
    ModelChangeNotify();

    // continue behavior in base class
    jump CEnemyBase::MainLoop();
  };
};
