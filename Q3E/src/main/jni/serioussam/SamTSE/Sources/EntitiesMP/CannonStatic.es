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

345
%{
#include "EntitiesMP/StdH/StdH.h"
#include "ModelsMP/Enemies/CannonStatic/Turret.h"

%}

uses "EntitiesMP/ModelHolder2";
uses "EntitiesMP/Projectile";
uses "EntitiesMP/SoundHolder";
uses "EntitiesMP/BloodSpray";
uses "EntitiesMP/CannonBall";

%{
#define CANNONS_SIZE 2.0f
  
// info structure
static EntityInfo eiCannonStatic = {
  EIBT_WOOD, 10000.0f,
  0.0f, 1.5f*CANNONS_SIZE, 0.0f,     // source (eyes)
  0.0f, 0.5f*CANNONS_SIZE, 0.0f,     // target (body)
};

#define FIRING_POSITION_MUZZLE FLOAT3D(0.0f, 0.4f, -1.0f)
#define MUZZLE_ROTATION_SPEED 45.0f //deg/sec

%}

class CCannonStatic: CEnemyBase {
name      "CannonStatic";
thumbnail "Thumbnails\\CannonStatic.tbn";

properties:
  
  1 FLOAT m_fHealth            "Cannon Health" = 100.0f,
  2 RANGE m_fFiringRangeClose  "Cannon Firing Close Range" = 50.0f,
  3 RANGE m_fFiringRangeFar    "Cannon Firing Far Range" = 150.0f,
  4 FLOAT m_fShootingPeriod    "Cannon Shooting Period" = 5.0f,
  5 FLOAT m_fSize              = CANNONS_SIZE,
  6 FLOAT m_fMaxPitch          "Cannon Max Pitch" = 20.0f,
  7 FLOAT m_fViewAngle         "Cannon View Angle" = 2.5f,
  8 BOOL  m_bActive            "Cannon Active" = TRUE,

 20 FLOAT3D m_fRotSpeedMuzzle  = ANGLE3D(0.0f, 0.0f, 0.0f),
 25 FLOAT m_fDistanceToPlayer   = 0.0f,
 26 FLOAT m_fDesiredMuzzlePitch = 0.0f,
 27 INDEX m_iMuzzleDir = 1,
 28 FLOAT3D m_vFiringPos      = FLOAT3D(0.0f, 0.0f, 0.0f),
 29 FLOAT3D m_vTarget = FLOAT3D(0.0f, 0.0f, 0.0f),

 40 FLOAT3D m_aBeginMuzzleRotation = ANGLE3D(0.0f, 0.0f, 0.0f),
 41 FLOAT3D m_aEndMuzzleRotation = ANGLE3D(0.0f, 0.0f, 0.0f),

components:
  1 class CLASS_BASE          "Classes\\EnemyBase.ecl",
  2 class CLASS_BASIC_EFFECT  "Classes\\BasicEffect.ecl",
  3 class CLASS_PROJECTILE    "Classes\\Projectile.ecl",
  4 class CLASS_CANNONBALL    "Classes\\CannonBall.ecl",
  //5 class CLASS_BLOOD_SPRAY   "Classes\\BloodSpray.ecl",
  
 // ************** CANNON MODEL **************
 10 model MODEL_TURRET        "ModelsMP\\Enemies\\CannonStatic\\Turret.mdl",
 11 model MODEL_CANNON        "ModelsMP\\Enemies\\CannonStatic\\Cannon.mdl",
 20 texture TEXTURE_TURRET    "ModelsMP\\Enemies\\CannonStatic\\Turret.tex",
 21 texture TEXTURE_CANNON    "Models\\Weapons\\Cannon\\Body.tex",

 // ************** CANNON PARTS **************
 30 model MODEL_DEBRIS_MUZZLE "ModelsMP\\Enemies\\CannonStatic\\Debris\\Cannon.mdl",
 31 model MODEL_DEBRIS_WHEEL  "ModelsMP\\Enemies\\CannonStatic\\Debris\\Wheel.mdl",
 32 model MODEL_DEBRIS_WOOD   "ModelsMP\\Enemies\\CannonStatic\\Debris\\Wood.mdl",
 35 model   MODEL_BALL         "Models\\Weapons\\Cannon\\Projectile\\CannonBall.mdl",
 36 texture TEXTURE_BALL       "Models\\Weapons\\Cannon\\Projectile\\IronBall.tex",

 // ************** SOUNDS **************
 50 sound   SOUND_FIRE         "ModelsMP\\Enemies\\CannonStatic\\Sounds\\Fire.wav",

functions:                                        

virtual CTString GetPlayerKillDescription(const CTString &strPlayerName, const EDeath &eDeath)
  {
    CTString str;
    str.PrintF(TRANSV("A Cannon killed %s"), (const char *) strPlayerName);
    return str;
  }

  /* Entity info */
  void *GetEntityInfo(void) {
    return &eiCannonStatic;
  };

  virtual const CTFileName &GetComputerMessageName(void) const {
    static DECLARE_CTFILENAME(fnmCannon, "DataMP\\Messages\\Enemies\\CannonStatic.txt");
    return fnmCannon;
  };

  void Precache(void) {
    CEnemyBase::Precache();
    PrecacheModel(MODEL_DEBRIS_MUZZLE);
    PrecacheModel(MODEL_DEBRIS_WHEEL);
    PrecacheModel(MODEL_DEBRIS_WOOD);
    PrecacheModel(MODEL_BALL);

    PrecacheTexture(TEXTURE_BALL);

    PrecacheSound(SOUND_FIRE);
    
    PrecacheClass(CLASS_CANNONBALL);
  };


  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    // take less damage from heavy bullets (e.g. sniper)
    if(dmtType==DMT_BULLET && fDamageAmmount>100.0f)
    {
      fDamageAmmount*=0.5f;
    }

    CEnemyBase::ReceiveDamage(penInflictor, dmtType, fDamageAmmount,
                              vHitPoint, vDirection);    
  };

  // damage anim
  INDEX AnimForDamage(FLOAT fDamage) {
    return 0;
  };

  // death
  INDEX AnimForDeath(void) {
    return 0;
  };

  // cast a ray to entity checking only for brushes
  BOOL IsVisible(CEntity *penEntity) 
  {
    ASSERT(penEntity!=NULL);
    // get ray source and target
    FLOAT3D vSource, vTarget;
    GetPositionCastRay(this, penEntity, vSource, vTarget);

    // cast the ray
    CCastRay crRay(this, vSource, vTarget);
    crRay.cr_ttHitModels = CCastRay::TT_NONE;     // check for brushes only
    crRay.cr_bHitTranslucentPortals = FALSE;
    en_pwoWorld->CastRay(crRay);

    // if hit nothing (no brush) the entity can be seen
    return (crRay.cr_penHit==NULL);     
  };

  BOOL IsInTheLineOfFire(CEntity *penEntity, FLOAT fAngle)
  {
    ASSERT(penEntity!=NULL);

    FLOAT fCosAngle;
    FLOAT3D vHeading;
    FLOAT3D vToPlayer;

    FLOAT3D vSide = FLOAT3D(1.0f, 0.0f, 0.0f)*GetRotationMatrix();
    FLOAT3D vFront = FLOAT3D(0.0f, 0.0f, -1.0f)*GetRotationMatrix();
    vToPlayer = penEntity->GetPlacement().pl_PositionVector - GetPlacement().pl_PositionVector;
    vToPlayer.Normalize();
    
    fCosAngle = vToPlayer%vSide;
    
    // if on the firing plane
    if (Abs(fCosAngle)<CosFast(90.0f-fAngle)) {
      // if in front
      if ((vToPlayer%vFront)>0.0f) {
       return TRUE;
      }
    }
    return FALSE;    
  }

  CPlayer *AcquireTarget() {
    // find actual number of players
    INDEX ctMaxPlayers = GetMaxPlayers();
    CEntity *penPlayer;

    for(INDEX i=0; i<ctMaxPlayers; i++) {
      penPlayer=GetPlayerEntity(i);
      if (penPlayer!=NULL && DistanceTo(this, penPlayer)<m_fFiringRangeFar) {
        // if this player is more or less directly in front of the shooter
        if (IsInTheLineOfFire(penPlayer, m_fViewAngle)) {
          // see if something blocks the path to the player
          if (IsVisible(penPlayer)) {
            return (CPlayer *)penPlayer; 
          }
        }        
      }
    }
    return NULL;
  };

  // spawn body parts
  void CannonBlowUp(void)
  {
    FLOAT3D vNormalizedDamage = m_vDamage-m_vDamage*(m_fBlowUpAmount/m_vDamage.Length());
    vNormalizedDamage /= Sqrt(vNormalizedDamage.Length());
    vNormalizedDamage *= 0.75f;
    vNormalizedDamage += FLOAT3D(0.0f, 10.0f+FRnd()*10.0f, 0.0f);

    FLOAT3D vBodySpeed = en_vCurrentTranslationAbsolute-en_vGravityDir*(en_vGravityDir%en_vCurrentTranslationAbsolute);
    
    // spawn debris
    Debris_Begin(EIBT_WOOD, DPT_NONE, BET_NONE, 1.0f, vNormalizedDamage, vBodySpeed, 5.0f, 2.0f);
    
    Debris_Spawn(this, this, MODEL_DEBRIS_MUZZLE, TEXTURE_CANNON, 0, 0, 0, 0, m_fSize,
      FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
    Debris_Spawn(this, this, MODEL_DEBRIS_WHEEL, TEXTURE_TURRET, 0, 0, 0, 0, m_fSize,
      FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
    Debris_Spawn(this, this, MODEL_DEBRIS_WHEEL, TEXTURE_TURRET, 0, 0, 0, 0, m_fSize,
      FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
    Debris_Spawn(this, this, MODEL_DEBRIS_WOOD, TEXTURE_TURRET, 0, 0, 0, 0, m_fSize,
      FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
    Debris_Spawn(this, this, MODEL_DEBRIS_WOOD, TEXTURE_TURRET, 0, 0, 0, 0, m_fSize,
      FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
    Debris_Spawn(this, this, MODEL_BALL, TEXTURE_BALL, 0, 0, 0, 0, m_fSize/2.0f,
      FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));

    // spawn explosion
    CPlacement3D plExplosion = GetPlacement();
    CEntityPointer penExplosion = CreateEntity(plExplosion, CLASS_BASIC_EFFECT);
    ESpawnEffect eSpawnEffect;
    eSpawnEffect.colMuliplier = C_WHITE|CT_OPAQUE;
    eSpawnEffect.betType = BET_CANNON;
    FLOAT fSize = m_fBlowUpSize*1.0f;
    eSpawnEffect.vStretch = FLOAT3D(fSize,fSize,fSize);
    penExplosion->Initialize(eSpawnEffect);

    // spawn shockwave
    plExplosion = GetPlacement();
    penExplosion = CreateEntity(plExplosion, CLASS_BASIC_EFFECT);
    eSpawnEffect.colMuliplier = C_WHITE|CT_OPAQUE;
    eSpawnEffect.betType = BET_CANNONSHOCKWAVE;
    fSize = m_fBlowUpSize*1.0f;
    eSpawnEffect.vStretch = FLOAT3D(fSize,fSize,fSize);
    penExplosion->Initialize(eSpawnEffect);

    // hide yourself (must do this after spawning debris)
    SwitchToEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);
  }

  void PreMoving() {
    // manually update rotation of the attachments
    UpdateAttachmentRotations();
    CEnemyBase::PreMoving();
  }

  void PostMoving() {
    CEnemyBase::PostMoving();
    // make sure this entity stays in the moving list
    SetFlags(GetFlags()&~ENF_INRENDERING);
  }

  BOOL AdjustShadingParameters(FLOAT3D &vLightDirection, COLOR &colLight, COLOR &colAmbient) {
    CAttachmentModelObject &amo0 = *GetModelObject()->GetAttachmentModel(TURRET_ATTACHMENT_CANNON);
    // rotate to between-tick position
    amo0.amo_plRelative.pl_OrientationAngle =  Lerp(m_aBeginMuzzleRotation, m_aEndMuzzleRotation, _pTimer->GetLerpFactor());
    return CEnemyBase::AdjustShadingParameters(vLightDirection, colLight, colAmbient);
  };

  void UpdateAttachmentRotations( void )
  {
    // muzzle
    m_aBeginMuzzleRotation = m_aEndMuzzleRotation;
    m_aEndMuzzleRotation += m_fRotSpeedMuzzle*_pTimer->TickQuantum;
  }

  void UpdateFiringPos() {
    FLOATmatrix3D m;
    // initial position
    m_vFiringPos = FIRING_POSITION_MUZZLE*m_fSize;
    
    // pitch rotation
    MakeRotationMatrixFast(m, m_aBeginMuzzleRotation);
    m_vFiringPos = m_vFiringPos*m;
    
    // add translation
    CAttachmentModelObject &amo0 = *GetModelObject()->GetAttachmentModel(TURRET_ATTACHMENT_CANNON);
    m_vFiringPos += amo0.amo_plRelative.pl_PositionVector;    
  }

procedures:
  
  MainLoop() {
    wait() {
      on (EBegin) : {
        call WatchPlayers();
        resume;
      }
      on (EDeactivate) : {
        jump Inactive();
      }
      on (EDeath eDeath) : {
        jump Die(eDeath);
      }
    };
    return;
  };

  
  Die(EDeath eDeath) {
    // not alive anymore
    SetFlags(GetFlags()&~ENF_ALIVE);

    // find the one who killed, or other best suitable player
    CEntityPointer penKiller = eDeath.eLastDamage.penInflictor;
    if (penKiller==NULL || !IsOfClass(penKiller, "Player")) {
      penKiller = m_penEnemy;
    }

    if (penKiller==NULL || !IsOfClass(penKiller, "Player")) {
      penKiller = FixupCausedToPlayer(this, penKiller, /*bWarning=*/FALSE);
    }

    // if killed by someone
    if (penKiller!=NULL) {
      // give him score
      EReceiveScore eScore;
      eScore.iPoints = (INDEX) m_iScore;
      penKiller->SendEvent(eScore);
      if( CountAsKill())
      {
        penKiller->SendEvent(EKilledEnemy());
      }
      // send computer message
      EComputerMessage eMsg;
      eMsg.fnmMessage = GetComputerMessageName();
      if (eMsg.fnmMessage!="") {
        penKiller->SendEvent(eMsg);
      }
    }

    // send event to death target
    SendToTarget(m_penDeathTarget, m_eetDeathType, penKiller);
    // send event to spawner if any
    // NOTE: trigger's penCaused has been changed from penKiller to THIS;
    if (m_penSpawnerTarget) {
      SendToTarget(m_penSpawnerTarget, EET_TRIGGER, this);
    }

    // spawn debris
    CannonBlowUp();
    Destroy();
    return;
  };


  RotateMuzzle() {
        
    FLOAT fDeltaP = m_fDesiredMuzzlePitch - m_aBeginMuzzleRotation(2);

    // if close enough to desired rotation, don't rotate
    if (Abs(fDeltaP)<5.0f) { return EReturn(); }

    m_fRotSpeedMuzzle = ANGLE3D(0.0f, MUZZLE_ROTATION_SPEED*Sgn(fDeltaP), 0.0f);
    autowait(Abs(fDeltaP/MUZZLE_ROTATION_SPEED));
    m_fRotSpeedMuzzle = ANGLE3D(0.0f, 0.0f, 0.0f);
    UpdateFiringPos();
    
    return EReturn();
  };

  FireCannon() {

    FLOAT3D vToTarget = m_penEnemy->GetPlacement().pl_PositionVector -
                        GetPlacement().pl_PositionVector + m_vFiringPos;
    vToTarget.Normalize();
    
    // get vector pointing in heading direction of the muzzle
    FLOAT3D vCannonFront = FLOAT3D(0.0f, 0.0f, -1.0f)*GetRotationMatrix();
    ANGLE aToPlayer = ACos(vToTarget%vCannonFront);

    FLOAT fPitch = aToPlayer + 5.0f;
    FLOAT3D vCannonUp = FLOAT3D(0.0, 1.0f, 0.0f)*GetRotationMatrix();
    
    // if too far, do not fire
    if (m_fDistanceToPlayer>m_fFiringRangeFar) {
      return EReturn();
    // if player under cannon, minimize pitch
    } else if (vToTarget%vCannonUp<0.0f) {
      fPitch = 5.0f;
    // if in far range
    } else if (m_fDistanceToPlayer>m_fFiringRangeClose) {
      if (aToPlayer<m_fMaxPitch) {
        fPitch = aToPlayer + m_fMaxPitch*(m_fDistanceToPlayer-m_fFiringRangeClose)/(m_fFiringRangeFar-m_fFiringRangeClose);
      } else if (TRUE) {
        fPitch = aToPlayer + 10.0f + m_fMaxPitch*(m_fDistanceToPlayer-m_fFiringRangeClose)/(m_fFiringRangeFar-m_fFiringRangeClose);
      }
      // just to make sure
      fPitch = Clamp(fPitch, 1.0f, 80.0f);
    }
   
    m_vTarget = m_penEnemy->GetPlacement().pl_PositionVector;

    m_fDesiredMuzzlePitch = fPitch;
    autocall RotateMuzzle() EReturn;

    FLOAT3D vShooting = GetPlacement().pl_PositionVector + m_vFiringPos;
    FLOAT3D vSpeedDest = FLOAT3D(0.0f, 0.0f, 0.0f);
    FLOAT fLaunchSpeed;
    FLOAT fRelativeHdg;
  
    // calculate parameters for predicted angular launch curve
    EntityInfo *peiTarget = (EntityInfo*) (m_penEnemy->GetEntityInfo());
    CalculateAngularLaunchParams( vShooting, peiTarget->vTargetCenter[1]-6.0f/3.0f, m_vTarget, 
      vSpeedDest, m_fDesiredMuzzlePitch , fLaunchSpeed, fRelativeHdg);

    // target enemy body
    FLOAT3D vShootTarget;
    GetEntityInfoPosition(m_penEnemy, peiTarget->vTargetCenter, vShootTarget);
    // launch
    CPlacement3D pl;
    PrepareFreeFlyingProjectile(pl, vShootTarget, m_vFiringPos, ANGLE3D( fRelativeHdg, m_fDesiredMuzzlePitch, 0));
    CEntityPointer penBall = CreateEntity(pl, CLASS_CANNONBALL);
    ELaunchCannonBall eLaunch;
    eLaunch.penLauncher = this;
    eLaunch.fLaunchPower = fLaunchSpeed;
    eLaunch.cbtType = CBT_IRON;
    eLaunch.fSize = 1.0f;
    penBall->Initialize(eLaunch);
   
    return EReturn();
  };


  WatchPlayers()
  {
    // this is a kind of 'sleep' mode - check to see if any player entered
    // the attack radius every once in a while
    while(TRUE) {
      autowait(0.20f);
      CPlayer *pTarget = AcquireTarget();
      if (pTarget) {
        if ((pTarget->GetFlags()&ENF_ALIVE) && !(pTarget->GetFlags()&ENF_DELETED)) {
          m_penEnemy = pTarget;
          m_fDistanceToPlayer = DistanceTo(this, pTarget);
          autocall FireCannon() EReturn;
          autowait(m_fShootingPeriod);
        }
      }
    }
  }

  Inactive()
  {
    m_fRotSpeedMuzzle  = ANGLE3D(0.0f, 0.0f, 0.0f);
    wait() {
      on (EBegin) : { resume; }
      on (EActivate) : {
        jump MainLoop();
      }
      on (EDeath eDeath) : {
        jump Die(eDeath);
      }
      otherwise (): { resume; }
    }
  }

  Main(EVoid) {
    // declare yourself as a model
    InitAsModel();
    SetPhysicsFlags(EPF_MODEL_WALKING|EPF_HASLUNGS);
    SetCollisionFlags(ECF_MODEL);
    SetFlags(GetFlags()|ENF_ALIVE);
    en_fDensity = 2000.0f;
    
    // set your appearance
    SetModel(MODEL_TURRET);
    SetModelMainTexture(TEXTURE_TURRET);
    
    AddAttachment(TURRET_ATTACHMENT_CANNON, MODEL_CANNON, TEXTURE_CANNON);
    // setup moving speed
    m_fWalkSpeed = 0.0f;
    m_aWalkRotateSpeed = 0.0f;
    m_fAttackRunSpeed = 0.0f;
    m_aAttackRotateSpeed = 0.0f;
    m_fCloseRunSpeed = 0.0f;
    m_aCloseRotateSpeed = 0.0f;
    // setup attack distances
    m_fStopDistance = 100.0f;
    //m_fBlowUpAmount = 65.0f;
    m_fBlowUpAmount = 100.0f;
    m_fBodyParts = 4;
    m_fDamageWounded = 0.0f;
    m_iScore = 750;
    m_sptType = SPT_WOOD;
    
     // properties that modify EnemyBase properties
    if (m_fHealth<=0.0f) { m_fHealth=1.0f; }
    m_fCloseFireTime = m_fAttackFireTime = m_fShootingPeriod;
    SetHealth(m_fHealth); m_fMaxHealth = m_fHealth;
    if (m_fFiringRangeFar<m_fFiringRangeClose) { m_fFiringRangeFar=m_fFiringRangeClose+1.0f; }
        
    // set stretch factors for height and width
    GetModelObject()->StretchModel(FLOAT3D(m_fSize, m_fSize, m_fSize));
    
    ModelChangeNotify();
    StandingAnim();
    
    // don't continue behavior in base class! - this enemy is derived
    // from CEnemyBase only because of its properties
    autowait(0.05f);
    SetDesiredTranslation(FLOAT3D(0.0f, 0.0f, 0.0f));
    UpdateFiringPos();

    if (!m_bActive) { jump Inactive(); }
    jump MainLoop();
    
  };
};
