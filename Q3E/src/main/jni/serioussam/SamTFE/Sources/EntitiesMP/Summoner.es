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

347
%{
#include "EntitiesMP/StdH/StdH.h"
#include "EntitiesMP/BackgroundViewer.h"
#include "EntitiesMP/WorldSettingsController.h"
#include "ModelsMP/Enemies/Summoner/Summoner.h"
#include "ModelsMP/Enemies/Summoner/Staff.h"
#include "EntitiesMP/Effector.h"
%}

uses "EntitiesMP/EnemyBase";
uses "EntitiesMP/SpawnerProjectile";
uses "EntitiesMP/AreaMarker";
uses "EntitiesMP/SummonerMarker";
uses "EntitiesMP/Player";

event ESummonerTeleport {
  FLOAT fWait,
};

%{
#define RAND_05 (FLOAT(rand())/(float)(RAND_MAX)-0.5f)
#define SUMMONER_SIZE 7.0f
#define TM_WAIT_BEFORE_FIRE 1.9f
#define SUMMONER_HEALTH 15000.0f
// info structure
static EntityInfo eiSummoner = {
  EIBT_FLESH, 1500.0f,
  0.0f, 1.7f*SUMMONER_SIZE, 0.0f,
  0.0f, 1.0f*SUMMONER_SIZE, 0.0f,
};

//#define FIREPOS_ARMS FLOAT3D(-0.22f, 1.63f, 0.96f)
#define FIREPOS_ARMS FLOAT3D(0.131292f, 1.61069f, -0.314068f)

#define SUMMONER_MAX_SS 6
                                        // hlth  grp1  grp2  grp3 
INDEX aiSpawnScheme[SUMMONER_MAX_SS][7] = {100,  4,7,  0,0,  0,0, 
                                            90,  3,5,  2,4,  0,0, 
                                            70,  3,4,  3,4,  0,0, 
                                            50,  1,3,  3,5,  1,1, 
                                            30,  1,2,  2,3,  2,2, 
                                            15,  1,1,  2,4,  2,3 };
#define SUMMONER_TEMP_PER_GROUP 6
%}


class CSummoner : CEnemyBase {
name      "Summoner";
thumbnail "Thumbnails\\Summoner.tbn";

properties:
  
   1 BOOL  m_bInvulnerable = FALSE, // can we be hurt?
   2 CEntityPointer m_penBeginDeathTarget "Sum. Begin Death Target",   
   3 CEntityPointer m_penEndDeathTarget "Sum. End Death Target",
   4 CEntityPointer m_penExplodeDeathTarget "Sum. Explode Target",
   5 BOOL  m_bShouldTeleport = FALSE, // are we allowed to teleport?
   6 FLOAT m_fFirePeriod = 3.0f,
   7 FLOAT m_fImmaterialDuration "Sum. Immaterial Duration" = 5.0f, // how long to stay immaterial
   8 FLOAT m_fCorporealDuration "Sum. Corporeal Duration" = 5.0f, // how long to stay material
   9 FLOAT m_tmMaterializationTime = 0.0f, // when we materialized

  10 FLOAT m_fStretch "Sum. Stretch" = SUMMONER_SIZE,
  11 INDEX m_iSize = 1,     // how big it is (gets bigger when harmed)
  12 CEntityPointer m_penControlArea "Sum. Control Area",
  
  // NOTE: if number of templates per group changes, you MUST change the
  // SUMMONER_TEMP_PER_GROUP #definition at the beginning of the file
  20 INDEX m_iGroup01Count = 0,
  21 CEntityPointer m_penGroup01Template01 "Sum. Group01 Template01",
  22 CEntityPointer m_penGroup01Template02 "Sum. Group01 Template02",
  23 CEntityPointer m_penGroup01Template03 "Sum. Group01 Template03",
  24 CEntityPointer m_penGroup01Template04 "Sum. Group01 Template04",
  25 CEntityPointer m_penGroup01Template05 "Sum. Group01 Template05",
  26 CEntityPointer m_penGroup01Template06 "Sum. Group01 Template06",
  
  30 INDEX m_iGroup02Count = 0,
  31 CEntityPointer m_penGroup02Template01 "Sum. Group02 Template01",
  32 CEntityPointer m_penGroup02Template02 "Sum. Group02 Template02",
  33 CEntityPointer m_penGroup02Template03 "Sum. Group02 Template03",
  34 CEntityPointer m_penGroup02Template04 "Sum. Group02 Template04",
  35 CEntityPointer m_penGroup02Template05 "Sum. Group02 Template05",
  36 CEntityPointer m_penGroup02Template06 "Sum. Group02 Template06",
  
  40 INDEX m_iGroup03Count = 0,
  41 CEntityPointer m_penGroup03Template01 "Sum. Group03 Template01",
  42 CEntityPointer m_penGroup03Template02 "Sum. Group03 Template02",
  43 CEntityPointer m_penGroup03Template03 "Sum. Group03 Template03",
  44 CEntityPointer m_penGroup03Template04 "Sum. Group03 Template04",
  45 CEntityPointer m_penGroup03Template05 "Sum. Group03 Template05",
  46 CEntityPointer m_penGroup03Template06 "Sum. Group03 Template06",
  
  60 CEntityPointer m_penTeleportMarker "Sum. Teleport marker",
  61 INDEX m_iTeleportMarkers = 0, // number of teleport markers
  65 CEntityPointer m_penSpawnMarker "Sum. Enemy spawn marker",
  66 INDEX m_iSpawnMarkers = 0, // number of spawn markers
  67 FLOAT m_fTeleportWaitTime = 0.0f, // internal

  70 FLOAT m_fFuss = 0.0f, // value of all enemies scores summed up
  78 INDEX m_iEnemyCount = 0, // how many enemies in the area
  71 FLOAT m_fMaxCurrentFuss = 0.0f,
  72 FLOAT m_fMaxBeginFuss "Sum. Max Begin Fuss" = 10000.0f, 
  73 FLOAT m_fMaxEndFuss "Sum. Max End Fuss" = 60000.0f, 
  75 INDEX m_iSpawnScheme = 0,
  76 BOOL  m_bFireOK = TRUE,
  79 BOOL  m_bFiredThisTurn = FALSE,
  77 FLOAT m_fDamageSinceLastSpawn = 0.0f,

  88 BOOL  m_bExploded = FALSE, // still alive and embodied
  90 BOOL  m_bDying = FALSE,  // set when dying
  92 FLOAT m_tmDeathBegin = 0.0f,
  93 FLOAT m_fDeathDuration = 0.0f,
  94 CEntityPointer m_penDeathInflictor,
 111 CEntityPointer m_penKiller,
  // internal variables
  95 FLOAT3D m_vDeathPosition = FLOAT3D(0.0f, 0.0f, 0.0f),
  96 CEntityPointer m_penDeathMarker "Sum. Death marker",
 100 INDEX m_iIndex = 0, // temp. index

 102 INDEX m_iTaunt = 0, // index of currently active taunt

 110 FLOAT m_tmParticlesDisappearStart=-1e6,
 
 120 FLOAT m_tmLastAnimation=0.0f,

 150 CSoundObject m_soExplosion,
 151 CSoundObject m_soSound,
 152 CSoundObject m_soChant,
 153 CSoundObject m_soTeleport,
 
{
  CEmiter m_emEmiter;
}

components:
  0 class   CLASS_BASE               "Classes\\EnemyBase.ecl",
  1 class   CLASS_BLOOD_SPRAY        "Classes\\BloodSpray.ecl",
  2 class   CLASS_SPAWNER_PROJECTILE "Classes\\SpawnerProjectile.ecl",
  3 class   CLASS_BASIC_EFFECT       "Classes\\BasicEffect.ecl",
  4 class   CLASS_EFFECTOR           "Classes\\Effector.ecl",
  
// ************** MAIN MODEL **************

 10 model   MODEL_SUMMONER      "ModelsMP\\Enemies\\Summoner\\Summoner.mdl",
 11 texture TEXTURE_SUMMONER    "ModelsMP\\Enemies\\Summoner\\Summoner.tex",
 12 model   MODEL_STAFF         "ModelsMP\\Enemies\\Summoner\\Staff.mdl",
 13 texture TEXTURE_STAFF       "ModelsMP\\Enemies\\Summoner\\Staff.tex",
 
 16 model   MODEL_DEBRIS01      "ModelsMP\\Enemies\\Summoner\\Debris\\Cloth01.mdl",
 17 model   MODEL_DEBRIS02      "ModelsMP\\Enemies\\Summoner\\Debris\\Cloth02.mdl",
 18 model   MODEL_DEBRIS03      "ModelsMP\\Enemies\\Summoner\\Debris\\Cloth03.mdl",
 19 model   MODEL_DEBRIS_FLESH  "Models\\Effects\\Debris\\Flesh\\Flesh.mdl",
 20 texture TEXTURE_DEBRIS_FLESH  "Models\\Effects\\Debris\\Flesh\\FleshRed.tex",


// ************** SOUNDS **************
//200 sound   SOUND_IDLE   "ModelsMP\\Enemies\\Summoner\\Sounds\\Idle.wav",
101 sound  SOUND_LAUGH      "ModelsMP\\Enemies\\Summoner\\Sounds\\Laugh.wav",
102 sound  SOUND_EXPLODE    "ModelsMP\\Enemies\\Summoner\\Sounds\\Explode.wav",
103 sound  SOUND_TREMORS    "ModelsMP\\Enemies\\Summoner\\Sounds\\Tremors.wav",
104 sound  SOUND_DEATH      "ModelsMP\\Enemies\\Summoner\\Sounds\\Death.wav",
105 sound  SOUND_LASTWORDS  "ModelsMP\\Enemies\\Summoner\\Sounds\\LastWords.wav",
106 sound  SOUND_FIRE       "ModelsMP\\Enemies\\Summoner\\Sounds\\Fire.wav",
108 sound  SOUND_CHIMES     "ModelsMP\\Enemies\\Summoner\\Sounds\\Chimes.wav",
107 sound  SOUND_MATERIALIZE "ModelsMP\\Enemies\\Summoner\\Sounds\\Materialize.wav",
109 sound  SOUND_TELEPORT    "ModelsMP\\Enemies\\Summoner\\Sounds\\Teleport.wav",

// ***** TAUNTS ******
150 sound  SOUND_TAUNT01    "ModelsMP\\Enemies\\Summoner\\Sounds\\Quote03.wav",
151 sound  SOUND_TAUNT02    "ModelsMP\\Enemies\\Summoner\\Sounds\\Quote05.wav",
152 sound  SOUND_TAUNT03    "ModelsMP\\Enemies\\Summoner\\Sounds\\Quote07.wav",
153 sound  SOUND_TAUNT04    "ModelsMP\\Enemies\\Summoner\\Sounds\\Quote08.wav",
154 sound  SOUND_TAUNT05    "ModelsMP\\Enemies\\Summoner\\Sounds\\Quote10.wav",
155 sound  SOUND_TAUNT06    "ModelsMP\\Enemies\\Summoner\\Sounds\\Quote11.wav",
156 sound  SOUND_TAUNT07    "ModelsMP\\Enemies\\Summoner\\Sounds\\Quote14.wav",
157 sound  SOUND_TAUNT08    "ModelsMP\\Enemies\\Summoner\\Sounds\\Quote15.wav",
158 sound  SOUND_TAUNTLAST  "ModelsMP\\Enemies\\Summoner\\Sounds\\Quote16.wav",

functions:
  void Read_t( CTStream *istr) // throw char *
  { 
    CEnemyBase::Read_t(istr);
    m_emEmiter.Read_t(*istr);
  }
  
  void Write_t( CTStream *istr) // throw char *
  { 
    CEnemyBase::Write_t(istr);
    m_emEmiter.Write_t(*istr);
  }
 
  BOOL IsTargetValid(SLONG slPropertyOffset, CEntity *penTarget)
  {
    if ( static_cast<size_t>(slPropertyOffset) >= _offsetof(CSummoner, m_penGroup01Template01) &&
      static_cast<size_t>(slPropertyOffset) <= _offsetof(CSummoner, m_penGroup03Template06))
    {
      if (IsDerivedFromClass(penTarget, "Enemy Base")) {
        if (((CEnemyBase &)*penTarget).m_bTemplate) {
          return TRUE;
        } else {
          return FALSE;
        }
      } else {
        return FALSE; 
      }
    }
    if( slPropertyOffset == _offsetof(CSummoner, m_penControlArea))
    {
      if (IsDerivedFromClass(penTarget, "AreaMarker")) {
        return TRUE;
      } else {
        return FALSE;
      }
    }
    if( slPropertyOffset == _offsetof(CSummoner, m_penSpawnMarker))
    {
      if (IsDerivedFromClass(penTarget, "Enemy Marker")) {
        return TRUE;
      } else {
        return FALSE;
      }
    }
    if( slPropertyOffset == _offsetof(CSummoner, m_penTeleportMarker) ||
        slPropertyOffset == _offsetof(CSummoner, m_penDeathMarker))
    {
      if (IsDerivedFromClass(penTarget, "SummonerMarker")) {
        return TRUE;
      } else {
        return FALSE;
      }
    }
    return CEntity::IsTargetValid(slPropertyOffset, penTarget);    
  }
  
  
  BOOL DoSafetyChecks(void) {
    
    if (m_penSpawnMarker==NULL) {
      WarningMessage( "No valid Spawn Marker for Summoner boss! Destroying boss...");
      return FALSE;
    }
    if (m_penTeleportMarker==NULL) {
      WarningMessage( "No valid Teleport Marker for Summoner boss! Destroying boss...");
      return FALSE;
    }
    if (m_penDeathMarker==NULL) {
      WarningMessage( "No valid Death Marker for Summoner boss! Destroying boss...");
      return FALSE;
    }
    if (m_penControlArea==NULL) {
      WarningMessage( "No valid Area Marker for Summoner boss! Destroying boss...");
      return FALSE;
    }
    if (m_iGroup01Count<1 || m_iGroup02Count<1 || m_iGroup03Count<1)
    {
      WarningMessage( "At least one template in each group required! Destroying boss...");
      return FALSE;
    }
    return TRUE;
  }

  // describe how this enemy killed player
  virtual CTString GetPlayerKillDescription(const CTString &strPlayerName, const EDeath &eDeath)
  {
    CTString str;
    str.PrintF(TRANSV("The Summoner unsummoned %s"), (const char *) strPlayerName);
    return str;
  }
  
  virtual const CTFileName &GetComputerMessageName(void) const {
    static DECLARE_CTFILENAME(fnm, "DataMP\\Messages\\Enemies\\Summoner.txt");
    return fnm;
  };

  void Precache(void)
  {
    CEnemyBase::Precache();
    
    PrecacheClass(CLASS_BLOOD_SPRAY        );
    PrecacheClass(CLASS_SPAWNER_PROJECTILE );

    PrecacheClass(CLASS_BASIC_EFFECT, BET_CANNON );
    
    PrecacheModel(MODEL_SUMMONER     );
    PrecacheModel(MODEL_STAFF        );
    PrecacheTexture(TEXTURE_SUMMONER );
    PrecacheTexture(TEXTURE_STAFF    );
  
    PrecacheModel(MODEL_DEBRIS01     );
    PrecacheModel(MODEL_DEBRIS02     );
    PrecacheModel(MODEL_DEBRIS03     );
    PrecacheModel(MODEL_DEBRIS_FLESH );
    PrecacheTexture(TEXTURE_DEBRIS_FLESH );

    PrecacheSound(SOUND_LAUGH       );
    PrecacheSound(SOUND_EXPLODE     );
    PrecacheSound(SOUND_TREMORS     );
    PrecacheSound(SOUND_DEATH       );
    PrecacheSound(SOUND_LASTWORDS   );
    PrecacheSound(SOUND_FIRE        );
    PrecacheSound(SOUND_CHIMES      );
    PrecacheSound(SOUND_MATERIALIZE );
    PrecacheSound(SOUND_TELEPORT    );
  
    for (INDEX i=SOUND_TAUNT01; i<=SOUND_TAUNTLAST; i++) { 
      PrecacheSound(i); 
    }
  
  };


  // Entity info
  void *GetEntityInfo(void) {
    return &eiSummoner;
  };

  CMusicHolder *GetMusicHolder()
  {
    CEntity *penMusicHolder;
    penMusicHolder = _pNetwork->GetEntityWithName("MusicHolder", 0);
    return (CMusicHolder *)&*penMusicHolder;
  }

  BOOL DistanceToAllPlayersGreaterThen(FLOAT fDistance)
  {
    // find actual number of players
    INDEX ctMaxPlayers = GetMaxPlayers();
    CEntity *penPlayer;
    
    for(INDEX i=0; i<ctMaxPlayers; i++) {
      penPlayer=GetPlayerEntity(i);
      if (penPlayer) {
        if (DistanceTo(this, penPlayer)<fDistance) {
          return FALSE;
        }
      }
    }
    return TRUE;
  };

  /* Shake ground */
  void ShakeItBaby(FLOAT tmShaketime, FLOAT fPower, BOOL bFadeIn)
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

      pwsc->m_bShakeFadeIn = bFadeIn;
    }
  }

  void ChangeEnemyNumberForAllPlayers(INDEX iDelta)
  {
    // find actual number of players
    INDEX ctMaxPlayers = GetMaxPlayers();
    CEntity *penPlayer;
    
    for(INDEX i=0; i<ctMaxPlayers; i++) {
      penPlayer=GetPlayerEntity(i);
      if (penPlayer) {
        // set totals for level and increment for game
        ((CPlayer &)*penPlayer).m_psLevelTotal.ps_iKills+=iDelta;
        ((CPlayer &)*penPlayer).m_psGameTotal.ps_iKills+=iDelta;
      }
    }
  };

  // Receive damage
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    
    // while we are invulnerable, receive no damage
    if (m_bInvulnerable) {
      return;
    }

    // summoner doesn't receive damage from other monsters
    if(!IsOfClass(penInflictor, "Player")) {
      return;
    }

    // boss cannot be telefragged
    if(dmtType==DMT_TELEPORT)
    {
      return;
    }

    // cannonballs inflict less damage then the default
    if(dmtType==DMT_CANNONBALL)
    {
      fDamageAmmount *= 0.5f;
    }
    
    FLOAT fOldHealth = GetHealth();
    CEnemyBase::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);
    FLOAT fNewHealth = GetHealth();

    // increase temp. damage 
    m_fDamageSinceLastSpawn += fOldHealth - fNewHealth;

    // see if we have to change the spawning scheme
    for (INDEX i=0; i<SUMMONER_MAX_SS; i++) {
      FLOAT fHealth = (FLOAT)aiSpawnScheme[i][0]*m_fMaxHealth/100.0f;
      if (fHealth<=fOldHealth && fHealth>fNewHealth)
      {
        m_iSpawnScheme = i;
      }
    }
    
    // adjust fuss
    m_fMaxCurrentFuss = (1.0f-(GetHealth()/m_fMaxHealth))*(m_fMaxEndFuss-m_fMaxBeginFuss)+m_fMaxBeginFuss;
    
    // bosses don't darken when burning
    m_colBurning=COLOR(C_WHITE|CT_OPAQUE);

  };


  // damage anim
  /*INDEX AnimForDamage(FLOAT fDamage) {
    INDEX iAnim;
    iAnim = SUMMONER_ANIM_WOUND;
    StartModelAnim(iAnim, 0);
    return iAnim;
  };*/

  void StandingAnimFight(void) {
    StartModelAnim(SUMMONER_ANIM_IDLE, AOF_LOOPING|AOF_NORESTART);
  };

  // virtual anim functions
  void StandingAnim(void) {
    StartModelAnim(SUMMONER_ANIM_IDLE, AOF_LOOPING|AOF_NORESTART);        
  };

  void WalkingAnim(void) {
    StartModelAnim(SUMMONER_ANIM_IDLE, AOF_LOOPING|AOF_NORESTART);
  };

  void RunningAnim(void) {
    WalkingAnim();
  };

  void RotatingAnim(void) {
    WalkingAnim();
  };

  /*INDEX AnimForDeath(void) {
    INDEX iAnim;
    iAnim = SUMMONER_ANIM_DEATH;
    StartModelAnim(iAnim, 0);
    return iAnim;
  };*/

  // virtual sound functions
  void IdleSound(void) {
    //PlaySound(m_soSound, SOUND_IDLE, SOF_3D);
  };
  
  FLOAT3D AcquireTarget()
  {
    CEnemyMarker *marker;
    marker = &((CEnemyMarker &)*m_penSpawnMarker);
    
    INDEX iMarker = IRnd()%m_iSpawnMarkers;

    while (iMarker>0)
    {
      marker = &((CEnemyMarker &)*marker->m_penTarget);
      iMarker--;
    }
    FLOAT3D vTarget = marker->GetPlacement().pl_PositionVector;
    FLOAT fR = FRnd()*marker->m_fMarkerRange;
    FLOAT fA = FRnd()*360.0f;
    vTarget += FLOAT3D(CosFast(fA)*fR, 0.05f, SinFast(fA)*fR);
    return vTarget;
  };

  void LaunchMonster(FLOAT3D vTarget, CEntity *penTemplate)
  {
    ASSERT(penTemplate!=NULL);
    // calculate parameters for predicted angular launch curve
    FLOAT3D vFirePos = FIREPOS_ARMS*m_fStretch;
    FLOAT3D vShooting = GetPlacement().pl_PositionVector + vFirePos*GetRotationMatrix();
    FLOAT fLaunchSpeed;
    FLOAT fRelativeHdg;
    //FLOAT fPitch = FRnd()*30.0f - 5.0f;
    FLOAT fPitch = FRnd()*10.0f + 25.0f;

    CPlacement3D pl;
    CalculateAngularLaunchParams( vShooting, 0.0f, vTarget,
      FLOAT3D(0.0f, 0.0f, 0.0f), fPitch, fLaunchSpeed, fRelativeHdg);
    
    PrepareFreeFlyingProjectile(pl, vTarget, vFirePos, ANGLE3D( fRelativeHdg, fPitch, 0.0f));
    
    ESpawnerProjectile esp;
    CEntityPointer penSProjectile = CreateEntity(pl, CLASS_SPAWNER_PROJECTILE);
    esp.penOwner = this;
    esp.penTemplate = penTemplate;
    penSProjectile->Initialize(esp);
    
    ((CMovableEntity &)*penSProjectile).LaunchAsFreeProjectile(FLOAT3D(0.0f, 0.0f, -fLaunchSpeed), (CMovableEntity*)(CEntity*)this);
  }
 
  FLOAT FussModifier(INDEX iEnemyCount) {
    return (0.995 + 0.005 * pow(m_iEnemyCount , 2.8));
  }

  void RecalculateFuss(void)
  {
    // get area box
    FLOATaabbox3D box;
    ((CAreaMarker &)*m_penControlArea).GetAreaBox(box);
    
    static CStaticStackArray<CEntity *> apenNearEntities;
    GetWorld()->FindEntitiesNearBox(box, apenNearEntities);

    INDEX m_iEnemyCount = 0;
    m_fFuss = 0.0f;
  
    for (INDEX i=0; i<apenNearEntities.Count(); i++)
    {
      if (IsDerivedFromClass(apenNearEntities[i], "Enemy Base") &&
        !IsOfClass(apenNearEntities[i], "Summoner")) {
        if (!((CEnemyBase &)*apenNearEntities[i]).m_bTemplate &&
          apenNearEntities[i]->GetFlags()&ENF_ALIVE) {
          m_fFuss += ((CEnemyBase &)*apenNearEntities[i]).m_iScore;
          m_iEnemyCount  ++;
        }
      }
    }

    m_fFuss *= FussModifier(m_iEnemyCount);

    // if too much fuss, make disable firing
    if (m_fFuss>m_fMaxCurrentFuss) {
//CPrintF("FIRE DISABLED -> too much fuss\n");
      m_bFireOK = FALSE;
    // enable firing only when very little fuss
    } else if (m_fFuss<0.4f*m_fMaxCurrentFuss) {
//CPrintF("FIRE ENABLE -> fuss more then %f\n", 0.4*m_fMaxCurrentFuss);
      m_bFireOK = TRUE; 
    // but if significant damage since last spawn, enable firing anyway
    } else if (m_fDamageSinceLastSpawn>0.07f*m_fMaxHealth) {
//CPrintF("FIRE ENABLED -> much damagesincelastspawn - %f\n", m_fDamageSinceLastSpawn);
      m_bFireOK = TRUE;
    }

//CPrintF("Fuss = %f/%f (%d enemies) at %f\n", m_fFuss, m_fMaxCurrentFuss, m_iEnemyCount, _pTimer->CurrentTick());
//CPrintF("health: %f, scheme: %i\n", GetHealth(), m_iSpawnScheme);
    return;
  }

  void CountEnemiesAndScoreValue(INDEX& iEnemies, FLOAT& fScore)
  {
    // get area box
    FLOATaabbox3D box;
    ((CAreaMarker &)*m_penControlArea).GetAreaBox(box);
    
    static CStaticStackArray<CEntity *> apenNearEntities;
    GetWorld()->FindEntitiesNearBox(box, apenNearEntities);
    
    iEnemies = 0;
    fScore = 0.0f;

    for (INDEX i=0; i<apenNearEntities.Count(); i++)
    {
      if (IsDerivedFromClass(apenNearEntities[i], "Enemy Base") &&
        !IsOfClass(apenNearEntities[i], "Summoner")) {
        if (!((CEnemyBase &)*apenNearEntities[i]).m_bTemplate &&
          apenNearEntities[i]->GetFlags()&ENF_ALIVE) {
          fScore += ((CEnemyBase &)*apenNearEntities[i]).m_iScore;
          iEnemies  ++;
        }
      }
    }
    return;
  }

  CEnemyBase *GetRandomTemplate (INDEX iGroup)
  {
    CEntityPointer *pen;
    INDEX iCount;
    if (iGroup == 0) {
      pen = &m_penGroup01Template01;
      iCount = IRnd()%m_iGroup01Count+1;
    } else if (iGroup == 1) {
      pen = &m_penGroup02Template01;
      iCount = IRnd()%m_iGroup02Count+1;
    } else if (iGroup == 2) {
      pen = &m_penGroup03Template01;
      iCount = IRnd()%m_iGroup03Count+1;
    } else {
      ASSERTALWAYS("Invalid group!");
      iCount = 0; // DG: this should have a deterministic value in case this happens after all!
    }
    ASSERT(iCount>0);

    INDEX i=-1;
    while (iCount>0)
    {
      i++;
      while (pen[i].ep_pen==NULL) {
        i++;
      } 
      iCount--;        
    }
    ASSERT (pen[i].ep_pen!=NULL);
    return (CEnemyBase *) pen[i].ep_pen;
  }

  void DisappearEffect(void)
  {
    CPlacement3D plFX=GetPlacement();
    ESpawnEffect ese;
    ese.colMuliplier = C_WHITE|CT_OPAQUE;
    ese.vStretch = FLOAT3D(3,3,3);
    ese.vNormal = FLOAT3D(0,1,0);
    ese.betType = BET_DUST_FALL;
    for( INDEX iSmoke=0; iSmoke<3; iSmoke++)
    {
      CPlacement3D plSmoke=plFX;
      plSmoke.pl_PositionVector+=FLOAT3D(0,iSmoke*4+4.0f,0);
      CEntityPointer penFX = CreateEntity(plSmoke, CLASS_BASIC_EFFECT);
      penFX->Initialize(ese);
    }

    /*
    // growing swirl
    ese.betType = BET_DISAPPEAR_DUST;
    penFX = CreateEntity(plFX, CLASS_BASIC_EFFECT);
    penFX->Initialize(ese);
    */
  }

  void SpawnTeleportEffect(void)
  {
    ESpawnEffect ese;
    ese.colMuliplier = C_lMAGENTA|CT_OPAQUE;
    ese.vStretch = FLOAT3D(5,5,5);
    ese.vNormal = FLOAT3D(0,1,0);

    // explosion debris
    ese.betType = BET_EXPLOSION_DEBRIS;
    CPlacement3D plFX=GetPlacement();
    CEntityPointer penFX = CreateEntity(plFX, CLASS_BASIC_EFFECT);
    penFX->Initialize(ese);
    ese.colMuliplier = C_MAGENTA|CT_OPAQUE;
    CEntityPointer penFX2 = CreateEntity(plFX, CLASS_BASIC_EFFECT);
    penFX2->Initialize(ese);
    ese.colMuliplier = C_lCYAN|CT_OPAQUE;
    CEntityPointer penFX3 = CreateEntity(plFX, CLASS_BASIC_EFFECT);
    penFX3->Initialize(ese);
    ese.betType = BET_CANNON;
    ese.colMuliplier = C_CYAN|CT_OPAQUE;
    CEntityPointer penFX4 = CreateEntity(plFX, CLASS_BASIC_EFFECT);
    penFX4->Initialize(ese);

    // explosion smoke
    /*
    ese.betType = BET_EXPLOSION_SMOKE;
    penFX = CreateEntity(plFX, CLASS_BASIC_EFFECT);
    penFX->Initialize(ese);
    */

    ESpawnEffector eLightning;
    eLightning.eetType = ET_LIGHTNING;
    eLightning.tmLifeTime = 0.5f;
    eLightning.fSize = 24;
    eLightning.ctCount = 32;

    CEntity *penLightning = CreateEntity( plFX, CLASS_EFFECTOR);
    ANGLE3D angRnd=ANGLE3D(
      0.0f,
      90.0f+(FRnd()-0.5f)*30.0f,
      (FRnd()-0.5f)*30.0f);

    FLOAT3D vRndDir;
    AnglesToDirectionVector(angRnd, vRndDir);
    FLOAT3D vDest=plFX.pl_PositionVector;
    vDest+=vRndDir*512.0f;
    eLightning.vDestination = vDest; 
    penLightning->Initialize( eLightning);
  }

  void KillAllEnemiesInArea(EDeath eDeath)
  {
    EDeath eDeath2;
    FLOATaabbox3D box;
    ((CAreaMarker &)*m_penControlArea).GetAreaBox(box);
    
    static CStaticStackArray<CEntity *> apenNearEntities;
    GetWorld()->FindEntitiesNearBox(box, apenNearEntities);
    
    for (INDEX i=0; i<apenNearEntities.Count(); i++)
    {
      if (IsDerivedFromClass(apenNearEntities[i], "Enemy Base") &&
        !IsOfClass(apenNearEntities[i], "Summoner")) {
        if (!((CEnemyBase &)*apenNearEntities[i]).m_bTemplate &&
          apenNearEntities[i]->GetFlags()&ENF_ALIVE) {
          eDeath2.eLastDamage.penInflictor = eDeath.eLastDamage.penInflictor;
          eDeath2.eLastDamage.vDirection = apenNearEntities[i]->GetPlacement().pl_PositionVector;
          eDeath2.eLastDamage.vHitPoint = eDeath2.eLastDamage.vDirection;
          eDeath2.eLastDamage.fAmount = 10000.0f;
          eDeath2.eLastDamage.dmtType = DMT_CLOSERANGE;
          apenNearEntities[i]->SendEvent(eDeath);
        }
      }
 
      CMusicHolder *penMusicHolder = GetMusicHolder();
      if (IsOfClass(apenNearEntities[i],"SpawnerProjectile")) {
        CPlacement3D pl;
        pl.pl_OrientationAngle = ANGLE3D(0.0f, 0.0f, 0.0f);
        pl.pl_PositionVector = apenNearEntities[i]->GetPlacement().pl_PositionVector;
        CEntityPointer penExplosion = CreateEntity(pl, CLASS_BASIC_EFFECT);
        ESpawnEffect eSpawnEffect;
        eSpawnEffect.colMuliplier = C_WHITE|CT_OPAQUE;
        eSpawnEffect.betType = BET_CANNON;
        eSpawnEffect.vStretch = FLOAT3D(2.0f, 2.0f, 2.0f);
        penExplosion->Initialize(eSpawnEffect);
        apenNearEntities[i]->Destroy();
        // decrease the number of spawned enemies for those that haven't been born
        if (penMusicHolder!=NULL) {          
          penMusicHolder->m_ctEnemiesInWorld--;
        }          
        ChangeEnemyNumberForAllPlayers(-1);
      }
    }    
  }

  void RenderParticles(void)
  {
    FLOAT tmNow = _pTimer->CurrentTick();
    if( tmNow>m_tmParticlesDisappearStart && tmNow<m_tmParticlesDisappearStart+4.0f)
    {
      Particles_SummonerDisappear(this, m_tmParticlesDisappearStart);
    }

    if( tmNow>m_tmLastAnimation)
    {
      INDEX ctInterpolations=2;
      // if invisible or dead don't add new sparks
      if(!m_bInvulnerable && !m_bExploded && GetHealth()>0)
      {
        for( INDEX iInter=0; iInter<ctInterpolations; iInter++)
        {
          // for holding attachment data
          FLOATmatrix3D mEn=GetRotationMatrix();
          FLOATmatrix3D mRot;
          FLOAT3D vPos;
          FLOAT tmBirth=tmNow+iInter*_pTimer->TickQuantum/ctInterpolations;
          // head
          FLOAT fLife=2.5f;
          FLOAT fCone=360.0f;
          FLOAT fStretch=1.0f;
          FLOAT fRotSpeed=360.0f;
          COLOR col=C_lYELLOW|CT_OPAQUE;

          MakeRotationMatrixFast(mRot, ANGLE3D(0.0f, 0.0f, 0.0f));
          vPos=FLOAT3D(0.0f, 0.0f, 0.0f);
          GetModelObject()->GetAttachmentTransformations(SUMMONER_ATTACHMENT_STAFF, mRot, vPos, FALSE);
          // next in hierarchy
          CAttachmentModelObject *pamo = GetModelObject()->GetAttachmentModel(SUMMONER_ATTACHMENT_STAFF);
          pamo->amo_moModelObject.GetAttachmentTransformations( STAFF_ATTACHMENT_PARTICLES, mRot, vPos, TRUE);
          vPos=GetPlacement().pl_PositionVector+vPos*GetRotationMatrix();

          FLOAT3D vSpeed=FLOAT3D( 0.1f+RAND_05, 0.1f+RAND_05, -1.0f-RAND_05);
          vSpeed=vSpeed.Normalize()*8.0f;
          m_emEmiter.AddParticle(vPos, vSpeed*mRot*mEn, RAND_05*360.0f, fRotSpeed, tmBirth, fLife, fStretch, col);
        }
      }

      m_emEmiter.em_vG=m_emEmiter.GetGravity(this);
      m_emEmiter.em_vG/=2.0f;
      m_emEmiter.AnimateParticles();
      m_tmLastAnimation=tmNow;
    }
    m_emEmiter.RenderParticles();
  }
  

procedures:
  
  InitiateTeleport()
  {
    m_bInvulnerable = TRUE;
    StartModelAnim(SUMMONER_ANIM_VANISHING, 0);

    // start disappear particles
    FLOAT tmNow = _pTimer->CurrentTick();
    m_tmParticlesDisappearStart=tmNow;

    PlaySound(m_soSound, SOUND_TELEPORT, SOF_3D);

    autowait(GetModelObject()->GetAnimLength(SUMMONER_ANIM_VANISHING)-0.2f);
    jump Immaterial();
  }
  
  Fire(EVoid) : CEnemyBase::Fire {
    
    // if not allready fired
    if (!m_bFiredThisTurn) {
      // if not too much fuss, we can really fire
      if (m_bFireOK) {
                
        INDEX iTaunt = SOUND_TAUNT01 + m_iTaunt%(SOUND_TAUNTLAST-SOUND_TAUNT01+1);
        PlaySound(m_soChant, iTaunt, SOF_3D);
        m_iTaunt++;

        StartModelAnim(SUMMONER_ANIM_MAGICATTACK, SOF_SMOOTHCHANGE);
        
        //wait to get into spawner emitting position
        autowait(TM_WAIT_BEFORE_FIRE);
        PlaySound(m_soSound, SOUND_FIRE, SOF_3D);

        INDEX i,j;
        FLOAT3D vTarget;
        FLOAT fTotalSpawnedScore = 0.0f;
        INDEX iTotalSpawnedCount = 0;
        INDEX iEnemyCount;
        FLOAT fScore;
        CountEnemiesAndScoreValue(iEnemyCount, fScore);
        
        CMusicHolder *penMusicHolder = GetMusicHolder();
        
        FLOAT fTmpFuss = 0.0f;
        
        // for each group in current spawn scheme
        for (i=0; i<3; i++) {
          INDEX iMin = aiSpawnScheme[m_iSpawnScheme][i*2+1];
          INDEX iMax = aiSpawnScheme[m_iSpawnScheme][i*2+2];
          ASSERT(iMin<=iMax);
          INDEX iToSpawn;
          iToSpawn = iMin + IRnd()%(iMax - iMin + 1);
          for (j=0; j<iToSpawn; j++) {
            CEnemyBase *penTemplate = GetRandomTemplate(i);
            vTarget = AcquireTarget();
            LaunchMonster(vTarget, penTemplate);
            fTotalSpawnedScore+= penTemplate->m_iScore;
            iTotalSpawnedCount++;
            // increase the number of spawned enemies in music holder
            if (penMusicHolder!=NULL) {
              penMusicHolder->m_ctEnemiesInWorld++;
            }
            ChangeEnemyNumberForAllPlayers(+1);
            // stop spawning if too much fuss or too many enemies
            fTmpFuss = (fTotalSpawnedScore+fScore)*FussModifier(iTotalSpawnedCount+iEnemyCount);
            if (fTmpFuss>m_fMaxCurrentFuss) { 
              break; 
            }
          }
        }
        
        m_fDamageSinceLastSpawn = 0.0f;
                
        //wait for firing animation to stop
        autowait(GetModelObject()->GetAnimLength(SUMMONER_ANIM_MAGICATTACK)-TM_WAIT_BEFORE_FIRE);
        
        // stand a while
        StartModelAnim(SUMMONER_ANIM_IDLE, SOF_SMOOTHCHANGE);
        
        // teleport by sending an event to ourselves
        ESummonerTeleport est;
        est.fWait = FRnd()*1.0f+3.0f;
        SendEvent(est);
      // if too much fuss, just laugh and initiate teleport
      } else if (TRUE) {
      
        PlaySound(m_soExplosion, SOUND_LAUGH, SOF_3D);
        autowait(1.0f);
        StartModelAnim(SUMMONER_ANIM_MAGICATTACK, SOF_SMOOTHCHANGE);
        
        INDEX iTaunt = SOUND_TAUNT01 + m_iTaunt%(SOUND_TAUNTLAST-SOUND_TAUNT01+1);
        PlaySound(m_soChant, iTaunt, SOF_3D);
        m_iTaunt++;
        
        //wait to get into spawner emitting position
        autowait(TM_WAIT_BEFORE_FIRE);
        PlaySound(m_soSound, SOUND_FIRE, SOF_3D);

        INDEX iEnemyCount;
        FLOAT fScore;
        CountEnemiesAndScoreValue(iEnemyCount, fScore);
        FLOAT fToSpawn;

        INDEX iScheme;
        // find last active scheme
        for (INDEX i=0; i<3; i++) {
          if (aiSpawnScheme[m_iSpawnScheme][i*2+2]>0) {
            iScheme = i;
          }
        }
        
        if (m_iSpawnScheme>3) {
          iEnemyCount += (m_iSpawnScheme-3);
        }
        
        if (iEnemyCount<6) {
          fToSpawn = (6.0f - (FLOAT)iEnemyCount)/2.0f;
        } else {
          fToSpawn = 1.0f;
        }
        INDEX iToSpawn = (INDEX) ceilf(fToSpawn);
        
        CMusicHolder *penMusicHolder = GetMusicHolder();
//CPrintF("spawning %d from %d group\n", iToSpawn, iScheme);
        // spawn
        for (INDEX j=0; j<iToSpawn; j++) {
          CEnemyBase *penTemplate = GetRandomTemplate(iScheme);
          FLOAT3D vTarget = AcquireTarget();
          LaunchMonster(vTarget, penTemplate);
          // increase the number of spawned enemies in music holder
          if (penMusicHolder!=NULL) {
            penMusicHolder->m_ctEnemiesInWorld++;
          }
          ChangeEnemyNumberForAllPlayers(+1);
        }

        // teleport by sending an event to ourselves
        ESummonerTeleport est;
        est.fWait = FRnd()*1.0f+3.0f;
        SendEvent(est);
        // laugh
        autowait(1.0f);
        
      }

      m_bFiredThisTurn = TRUE;
    }

    return EReturn();
  };
      
  Hit(EVoid) : CEnemyBase::Hit {
    jump Fire();
    return EReturn();
  };

/************************************************************
 *                    D  E  A  T  H                         *
 ************************************************************/
  
  Die(EDeath eDeath) : CEnemyBase::Die
  {
  
    m_bDying = TRUE;

    m_penDeathInflictor = eDeath.eLastDamage.penInflictor;

    // find the one who killed, or other best suitable player
    m_penKiller = m_penDeathInflictor;
    if (m_penKiller ==NULL || !IsOfClass(m_penKiller, "Player")) {
      m_penKiller = m_penEnemy;
    }

    if (m_penKiller==NULL || !IsOfClass(m_penKiller, "Player")) {
      m_penKiller = FixupCausedToPlayer(this, m_penKiller, /*bWarning=*/FALSE);
    }

    // stop rotations
    SetDesiredRotation(ANGLE3D(0.0f, 0.0f, 0.0f));
    // first kill off all enemies inside the control area
    KillAllEnemiesInArea(eDeath);

    StartModelAnim(SUMMONER_ANIM_CHANTING, SOF_SMOOTHCHANGE);
    PlaySound(m_soExplosion, SOUND_LASTWORDS, SOF_3D);
    autowait(4.0f);

    autocall TeleportToDeathMarker() EReturn;

    // do this once more, just in case anything survived
    EDeath eDeath;
    eDeath.eLastDamage.penInflictor = m_penDeathInflictor;
    KillAllEnemiesInArea(eDeath);

    // slowly start shaking
    ShakeItBaby(_pTimer->CurrentTick(), 0.25f, TRUE);
    PlaySound(m_soExplosion, SOUND_TREMORS, SOF_3D);

    m_vDeathPosition = GetPlacement().pl_PositionVector;

    // notify possible targets of beginning of the death sequence
    if (m_penBeginDeathTarget!=NULL) {
      SendToTarget(m_penBeginDeathTarget, EET_TRIGGER, m_penKiller);
    }
    
    PlaySound(m_soSound, SOUND_DEATH, SOF_3D);
    StartModelAnim(SUMMONER_ANIM_DEATHBLOW, SOF_SMOOTHCHANGE);
    autowait(GetModelObject()->GetAnimLength(SUMMONER_ANIM_DEATHBLOW)-0.25f);    

    // hide model
    SwitchToEditorModel();

    // start death starts
    CPlacement3D plStars;
    plStars = GetPlacement();
    ESpawnEffect eSpawnEffect;
    eSpawnEffect.betType = BET_SUMMONERSTAREXPLOSION;
    eSpawnEffect.colMuliplier = C_WHITE|CT_OPAQUE;
    CEntityPointer penStars = CreateEntity(plStars, CLASS_BASIC_EFFECT);
    penStars->Initialize(eSpawnEffect);

    m_tmDeathBegin = _pTimer->CurrentTick();
    m_fDeathDuration = 12.0f;
    m_bExploded = TRUE;
    ShakeItBaby(_pTimer->CurrentTick(), 5.0f, FALSE);

    PlaySound(m_soExplosion, SOUND_EXPLODE, SOF_3D);

    // spawn debris
    Debris_Begin(EIBT_FLESH, DPT_BLOODTRAIL, BET_BLOODSTAIN, 1.0f, 
      FLOAT3D(0.0f, 10.0f, 0.0f), FLOAT3D(0.0f, 0.0f, 0.0f), 5.0f, 2.0f);
    for (INDEX i=0; i<15; i++) {
      
      FLOAT3D vSpeed = FLOAT3D(0.3f+FRnd()*0.1f, 1.0f+FRnd()*0.5f, 0.3f+FRnd()*0.1f)*1.5f*m_fStretch;
      FLOAT3D vPos = vSpeed + GetPlacement().pl_PositionVector;
      ANGLE3D aAng = ANGLE3D(FRnd()*360.0f, FRnd()*360.0f, FRnd()*360.0f);
      
      vSpeed.Normalize();
      vSpeed(2) *= vSpeed(2);

      CPlacement3D plPos = CPlacement3D (vPos, aAng);
      
      switch(i%3) {
      case 0:
        Debris_Spawn_Independent(this, this, MODEL_DEBRIS01, TEXTURE_SUMMONER, 0, 0, 0, 0, m_fStretch, 
          plPos, vSpeed*70.0f, aAng);
        Debris_Spawn_Independent(this, this, MODEL_DEBRIS_FLESH, TEXTURE_DEBRIS_FLESH , 0, 0, 0, 0, m_fStretch*0.33f, 
          plPos, vSpeed*70.0f, aAng);        
        break;
      case 1:
        Debris_Spawn_Independent(this, this, MODEL_DEBRIS02, TEXTURE_SUMMONER, 0, 0, 0, 0, m_fStretch, 
          plPos, vSpeed*70.0f, aAng);
        Debris_Spawn_Independent(this, this, MODEL_DEBRIS_FLESH, TEXTURE_DEBRIS_FLESH , 0, 0, 0, 0, m_fStretch*0.33f, 
          plPos, vSpeed*70.0f, aAng);        
        break;
      case 2:
        Debris_Spawn_Independent(this, this, MODEL_DEBRIS03, TEXTURE_SUMMONER, 0, 0, 0, 0, m_fStretch, 
          plPos, vSpeed*70.0f, aAng);
        Debris_Spawn_Independent(this, this, MODEL_DEBRIS_FLESH, TEXTURE_DEBRIS_FLESH , 0, 0, 0, 0, m_fStretch*0.33f, 
          plPos, vSpeed*70.0f, aAng);                
        break;
      }
    }
 
    // notify possible targets of end of the death sequence
    if (m_penExplodeDeathTarget!=NULL) {
      SendToTarget(m_penExplodeDeathTarget, EET_TRIGGER, m_penKiller);
    }

    // turn off collision and physics
    //SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    //SetCollisionFlags(ECF_IMMATERIAL);

    PlaySound(m_soSound, SOUND_CHIMES, SOF_3D);

    m_iIndex = 20;
    while ((m_iIndex--)>1) {
      CPlacement3D plExplosion;
      plExplosion.pl_OrientationAngle = ANGLE3D(0.0f, 0.0f, 0.0f);
      plExplosion.pl_PositionVector = FLOAT3D(0.3f+FRnd()*0.1f, 1.0f+FRnd()*0.5f, 0.3f+FRnd()*0.1f)*m_fStretch;
      plExplosion.pl_PositionVector += GetPlacement().pl_PositionVector;
      ESpawnEffect eSpawnEffect;
      eSpawnEffect.colMuliplier = C_WHITE|CT_OPAQUE;
      eSpawnEffect.betType = BET_CANNON;
      FLOAT fSize = (m_fStretch*m_iIndex)*0.333f;
      eSpawnEffect.vStretch = FLOAT3D(fSize, fSize, fSize);
      CEntityPointer penExplosion = CreateEntity(plExplosion, CLASS_BASIC_EFFECT);
      penExplosion->Initialize(eSpawnEffect);

      ShakeItBaby(_pTimer->CurrentTick(), m_iIndex/4.0f, FALSE);

      autowait(0.05f + FRnd()*0.2f);
    }

    autowait(m_fDeathDuration);

    // notify possible targets of end of the death sequence
    if (m_penEndDeathTarget!=NULL) {
      SendToTarget(m_penEndDeathTarget, EET_TRIGGER, m_penKiller);
    }

    EDeath eDeath;
    eDeath.eLastDamage.penInflictor = m_penDeathInflictor;
    jump CEnemyBase::Die(eDeath);
  }

  TeleportToDeathMarker(EVoid)
  {

    m_bInvulnerable = TRUE;

    StartModelAnim(SUMMONER_ANIM_VANISHING, SOF_SMOOTHCHANGE);
    autowait(GetModelObject()->GetAnimLength(SUMMONER_ANIM_VANISHING));

    // hide model
    DisappearEffect();
    SwitchToEditorModel();
    SetCollisionFlags(ECF_IMMATERIAL);
    
    // destroy possible flames
    CEntityPointer penFlame = GetChildOfClass("Flame");
    if (penFlame!=NULL) {
      penFlame->Destroy();
    }

    // wait a little bit
    autowait(2.0f);

    CPlacement3D pl;
    pl.pl_PositionVector = m_penDeathMarker->GetPlacement().pl_PositionVector;
    FLOAT3D vToPlayer;
    if (m_penEnemy!=NULL) {
      vToPlayer = m_penEnemy->GetPlacement().pl_PositionVector - pl.pl_PositionVector;
    } else {
      vToPlayer = m_vPlayerSpotted - pl.pl_PositionVector;
    }
    vToPlayer.Normalize();
    DirectionVectorToAngles(vToPlayer, pl.pl_OrientationAngle);
    Teleport(pl);

    // show model
    SpawnTeleportEffect();

    autowait(0.5f);
    SwitchToModel();
    SetCollisionFlags(ECF_MODEL);
    
    m_bInvulnerable = FALSE;

    StartModelAnim(SUMMONER_ANIM_APPEARING, SOF_SMOOTHCHANGE);
    autowait(GetModelObject()->GetAnimLength(SUMMONER_ANIM_APPEARING));

    return EReturn();
  }

  BossAppear(EVoid)
  {
    return EReturn();
  }

  // overridable called before main enemy loop actually begins
  PreMainLoop(EVoid) : CEnemyBase::PreMainLoop
  {
    autocall BossAppear() EReturn;
    return EReturn();
  }

  Immaterial() {
    
    // hide model
    DisappearEffect();
    SwitchToEditorModel();
    SetCollisionFlags(ECF_IMMATERIAL);
    
    // destroy possible flames
    CEntityPointer penFlame = GetChildOfClass("Flame");
    if (penFlame!=NULL) {
      penFlame->Destroy();
    }

    // wait required time
    autowait(m_fImmaterialDuration+FRnd()*2.0f-1.0f);
    
    INDEX iMaxTries = 10;
    FLOAT3D vTarget;
    // move to a new position
    do {
      CSummonerMarker *marker = &((CSummonerMarker &)*m_penTeleportMarker);
      INDEX iMarker = IRnd()%m_iTeleportMarkers;
      while (iMarker>0) {
        marker = &((CSummonerMarker &)*marker->m_penTarget);
        iMarker--;
      }
      vTarget = marker->GetPlacement().pl_PositionVector;
      FLOAT fR = FRnd()*marker->m_fMarkerRange;
      FLOAT fA = FRnd()*360.0f;
      vTarget += FLOAT3D(CosFast(fA)*fR, 0.05f, SinFast(fA)*fR);
    } while (!DistanceToAllPlayersGreaterThen(1.0f) || (iMaxTries--)<1);
    
    CPlacement3D pl;
    pl.pl_PositionVector = vTarget;
    FLOAT3D vToPlayer;
    if (m_penEnemy!=NULL) {
      vToPlayer = m_penEnemy->GetPlacement().pl_PositionVector - vTarget;
    } else {
      vToPlayer = m_vPlayerSpotted - vTarget;
    }
    vToPlayer.Normalize();
    DirectionVectorToAngles(vToPlayer, pl.pl_OrientationAngle);
    Teleport(pl);

    // show model
    SpawnTeleportEffect();
    SwitchToModel();
    SetCollisionFlags(ECF_MODEL);
    
    m_bShouldTeleport = FALSE;
    m_tmMaterializationTime = _pTimer->CurrentTick();
    m_bFiredThisTurn = FALSE;

    m_bInvulnerable = FALSE;
    
    PlaySound(m_soTeleport, SOUND_MATERIALIZE, SOF_3D);

    StartModelAnim(SUMMONER_ANIM_APPEARING, SOF_SMOOTHCHANGE);
    autowait(GetModelObject()->GetAnimLength(SUMMONER_ANIM_APPEARING));

    SendEvent(EBegin());
    return EReturn();

  }

  SummonerLoop() {
    // spawn a 1sec reminder
    SpawnReminder(this, 1.0f, 128);
    wait () {
      on (EBegin) :
      {
        call CEnemyBase::MainLoop();
      }
      on (EReminder er) :
      {
        // pass all reminders but the 128 one
        if (er.iValue==128) {
          RecalculateFuss();
          // see if we have to teleport
          if (_pTimer->CurrentTick()>m_tmMaterializationTime+m_fCorporealDuration) {
            m_bShouldTeleport = TRUE;
          }
          // spawn the reminder again so that we return here
          SpawnReminder(this, 1.0f, 128);
        } else if (er.iValue==129 && !m_bDying) {
          call InitiateTeleport();
        } else if (TRUE) {
          pass;
        }
        resume;
      }      
      // we want to teleport in near future
      on (ESummonerTeleport est) :
      {
        //m_fTeleportWaitTime = est.fWait;
        SpawnReminder(this, est.fWait, 129);
        resume;
      }
      otherwise () : {
        resume;
      }
    }
  }

/************************************************************
 *                       M  A  I  N                         *
 ************************************************************/
  Main(EVoid) {
    
    // declare yourself as a model
    InitAsEditorModel();
    
    SetPhysicsFlags(EPF_MODEL_WALKING);
    SetCollisionFlags(ECF_IMMATERIAL);
    SetFlags(GetFlags()|ENF_ALIVE);
    
    en_fDensity = 10000.0f;
    m_fDamageWounded = 1e6f;
    
    m_sptType = SPT_BLOOD;
    m_bBoss = TRUE;
    SetHealth(SUMMONER_HEALTH);
    m_fMaxHealth = SUMMONER_HEALTH;
    m_fBodyParts = 0;
    // setup moving speed
    m_fWalkSpeed = 0.0f;
    m_aWalkRotateSpeed = AngleDeg(270.0f);
    m_fAttackRunSpeed = 0.0f;
    m_aAttackRotateSpeed = AngleDeg(270.0f);
    m_fCloseRunSpeed = 0.0f;
    m_aCloseRotateSpeed = AngleDeg(270.0f);
    // setup attack distances
    m_fAttackDistance = 500.0f;
    m_fCloseDistance = 50.0f;
    m_fStopDistance = 500.0f;
    m_fIgnoreRange = 600.0f;
    m_iScore = 1000000;
    // setup attack times
    m_fAttackFireTime = m_fFirePeriod;
    m_fCloseFireTime = m_fFirePeriod;
    
    SetPhysicsFlags(EPF_MODEL_WALKING);
    StandingAnim();

    // set your appearance
    //m_fStretch = SIZE;
    SetComponents(this, *GetModelObject(), MODEL_SUMMONER, TEXTURE_SUMMONER, 0, 0, 0); 
    AddAttachmentToModel(this, *GetModelObject(), SUMMONER_ATTACHMENT_STAFF, MODEL_STAFF, TEXTURE_STAFF, 0, 0, 0);
    GetModelObject()->StretchModel(FLOAT3D(m_fStretch, m_fStretch, m_fStretch ));
    ModelChangeNotify();

    AddToMovers();

    autowait(_pTimer->TickQuantum);

    m_emEmiter.Initialize(this);
    m_emEmiter.em_etType=ET_SUMMONER_STAFF;

    // count templates by groups
    INDEX i;
    CEntityPointer *pen;
    m_iGroup01Count = 0;
    pen = &m_penGroup01Template01;
    for (i=0; i<SUMMONER_TEMP_PER_GROUP; i++) {
      if (pen[i].ep_pen!=NULL) { m_iGroup01Count++; }
    }
    m_iGroup02Count = 0;
    pen = &m_penGroup02Template01;
    for (i=0; i<SUMMONER_TEMP_PER_GROUP; i++) {
      if (pen[i].ep_pen!=NULL) { m_iGroup02Count++; }
    }
    m_iGroup03Count = 0;
    pen = &m_penGroup03Template01;
    for (i=0; i<SUMMONER_TEMP_PER_GROUP; i++) {
      if (pen[i].ep_pen!=NULL) { m_iGroup03Count++; }
    }

    if (!DoSafetyChecks()) {
      Destroy();
      return;
    }
   
    // count spawn markers
    CEnemyMarker *it;
    m_iSpawnMarkers = 1;
    it = &((CEnemyMarker &)*m_penSpawnMarker);
    while (it->m_penTarget!=NULL)
    {
      it = &((CEnemyMarker &)*it->m_penTarget);
      m_iSpawnMarkers ++;
    }

    // count teleport markers
    m_iTeleportMarkers = 1;
    it = &((CEnemyMarker &)*m_penTeleportMarker);
    while (it->m_penTarget!=NULL)
    {
      it = &((CEnemyMarker &)*it->m_penTarget);
      m_iTeleportMarkers ++;
    }

    m_iSpawnScheme = 0;
    m_fMaxCurrentFuss = m_fMaxBeginFuss;
    m_bDying = FALSE;
    m_tmDeathBegin = 0.0f;
    m_fDeathDuration = 0.0f;
    m_bInvulnerable = TRUE;
    m_bExploded = FALSE;

    // wait to be triggered
    wait() {
      on (EBegin) : { resume; }
      on (ETrigger) : { stop; }
      otherwise (): { resume; }
    }

    m_soExplosion.Set3DParameters(1500.0f, 1000.0f, 2.0f, 1.0f);
    m_soSound.Set3DParameters(1500.0f, 1000.0f, 2.0f, 1.0f);
    m_soChant.Set3DParameters(1500.0f, 1000.0f, 2.0f, 1.0f);
    m_soTeleport.Set3DParameters(1500.0f, 1000.0f, 3.0f, 1.0f);
    m_iTaunt = 0;

    //PlaySound(m_soSound, SOUND_APPEAR, SOF_3D);
    // teleport in
    SpawnTeleportEffect();
    SwitchToModel();
    m_bInvulnerable = FALSE;
    SetCollisionFlags(ECF_MODEL);

    PlaySound(m_soTeleport, SOUND_MATERIALIZE, SOF_3D);

    StartModelAnim(SUMMONER_ANIM_APPEARING, SOF_SMOOTHCHANGE);
    autowait(GetModelObject()->GetAnimLength(SUMMONER_ANIM_APPEARING));

    m_tmMaterializationTime = _pTimer->CurrentTick();
    
    // one state under base class to intercept some events
    jump SummonerLoop();
    //jump CEnemyBase::MainLoop();
  };
};
