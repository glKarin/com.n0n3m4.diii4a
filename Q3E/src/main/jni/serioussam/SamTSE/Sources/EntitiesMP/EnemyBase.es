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

310
%{
#include "EntitiesMP/StdH/StdH.h"
#include "EntitiesMP/Common/PathFinding.h"
#include "EntitiesMP/NavigationMarker.h"
#include "EntitiesMP/TacticsHolder.h"
extern void JumpFromBouncer(CEntity *penToBounce, CEntity *penBouncer);
extern INDEX ent_bReportBrokenChains;
%}

uses "EntitiesMP/Watcher";
uses "EntitiesMP/BasicEffects";
uses "EntitiesMP/Projectile";
uses "EntitiesMP/Debris";
uses "EntitiesMP/EnemyMarker";
uses "EntitiesMP/MusicHolder";
uses "EntitiesMP/BloodSpray";

event ERestartAttack {
};

// self sent in Active loop to reconsider what enemy should do
event EReconsiderBehavior {
};

// force wound
event EForceWound {
};

enum TargetType {
  0 TT_NONE "",   // no target
  1 TT_SOFT "",   // soft target - only spoted player but not heavily angry at him
  2 TT_HARD "",   // hard target - player has provoked enemy and it is very angry
};

enum DestinationType {
  0 DT_PLAYERCURRENT "",    // go to where player is now
  1 DT_PLAYERSPOTTED "",    // go to where player was last seen
  2 DT_PATHTEMPORARY "",    // go to navigation marker - temporary, only until you spot player again
  3 DT_PATHPERSISTENT "",    // go to navigation marker - until you really get there
};

%{
#define MF_MOVEZ    (1L<<0)
#define MF_ROTATEH  (1L<<1)
#define MF_MOVEXZY  (1L<<2)
%}

class export CEnemyBase : CMovableModelEntity {
name      "Enemy Base";
thumbnail "";
features  "HasName", "IsTargetable", "CanBePredictable";

properties:
  1 CEntityPointer m_penWatcher,      // watcher
  2 FLOAT3D m_vStartPosition = FLOAT3D(0,0,0),         // start position
  3 CEntityPointer m_penEnemy,        // current enemy
  4 enum TargetType m_ttTarget = TT_NONE, // type of target
  5 CTString m_strDescription = "Enemy base",
  6 CTString m_strName  "Name" 'N' = "Enemy base",
  7 CSoundObject m_soSound,
  8 FLOAT3D m_vStartDirection = FLOAT3D(0,0,-1),         // for returning to start
  9 BOOL m_bOnStartPosition = TRUE,
 29 FLOAT m_fFallHeight "Fall height" = 8.0f,
 31 FLOAT m_fStepHeight "Step height" = -1.0f,
 17 RANGE m_fSenseRange "Sense Range" = 0.0f,       // immediately spots any player that gets closer than this
 28 FLOAT m_fViewAngle "View angle" 'V' = 360.0f, // view frustum angle for spotting players

 // moving/attack properties - CAN BE SET
 // these following must be ordered exactly like this for GetProp() to function
 10 FLOAT m_fWalkSpeed = 1.0f,                  // walk speed
 11 ANGLE m_aWalkRotateSpeed = AngleDeg(10.0f), // walk rotate speed
 12 FLOAT m_fAttackRunSpeed = 1.0f,                // attack run speed
 13 ANGLE m_aAttackRotateSpeed = AngleDeg(10.0f),  // attack rotate speed
 14 FLOAT m_fCloseRunSpeed = 1.0f,                 // close run speed
 15 ANGLE m_aCloseRotateSpeed = AngleDeg(10.0f),   // close rotate speed
 20 FLOAT m_fAttackDistance = 50.0f,          // attack distance mode
 21 FLOAT m_fCloseDistance = 10.0f,           // close distance mode
 22 FLOAT m_fAttackFireTime = 2.0f,           // attack distance fire time
 23 FLOAT m_fCloseFireTime = 1.0f,            // close distance fire time
 24 FLOAT m_fStopDistance = 0.0f,             // stop moving toward enemy if closer than stop distance
 25 FLOAT m_fIgnoreRange = 200.0f,            // cease attack if enemy farther
 26 FLOAT m_fLockOnEnemyTime = 0.0f,          // time needed to fire

 // damage/explode properties - CAN BE SET
 40 FLOAT m_fBlowUpAmount = 0.0f,             // damage in minus for blow up
 41 INDEX m_fBodyParts = 4,                   // number of spawned body parts
 42 FLOAT m_fDamageWounded = 0.0f,            // damage amount to be wounded
 43 FLOAT3D m_vDamage = FLOAT3D(0,0,0),       // current damage impact
 44 FLOAT m_tmLastDamage = -1000.0f,
 46 BOOL m_bRobotBlowup = FALSE,    // set for robots parts blowup, otherwise blowup flesh
 47 FLOAT m_fBlowUpSize = 2.0f,

 // logic temporary variables -> DO NOT USE
133 FLOAT m_fMoveTime = 0.0f,
 52 FLOAT3D m_vDesiredPosition = FLOAT3D(0,0,0),
 53 enum DestinationType m_dtDestination =  DT_PLAYERCURRENT, // type of current desired position
 59 CEntityPointer m_penPathMarker,   // current path finding marker
 18 FLOAT3D m_vPlayerSpotted =  FLOAT3D(0,0,0), // where player was last spotted
 54 FLOAT m_fMoveFrequency = 0.0f,
 55 FLOAT m_fMoveSpeed = 0.0f,
 56 ANGLE m_aRotateSpeed = 0,
 57 FLOAT m_fLockStartTime = 0.0f,
 58 FLOAT m_fRangeLast = 0.0f,
130 BOOL m_bFadeOut = FALSE,
131 FLOAT m_fFadeStartTime = 0.0f,
132 FLOAT m_fFadeTime = 0.0f,

 // attack temporary -> DO NOT USE
 60 FLOAT m_fShootTime = 0.0f,                // time when entity will try to shoot on enemy
 61 FLOAT m_fDamageConfused = 0.0f,           // damage amount when entity shoot concentration is spoiled
 62 INDEX m_iChargeHitAnimation = 0,       // charge hit (close attack) properties
 63 FLOAT m_fChargeHitDamage = 0.0f,
 64 FLOAT m_fChargeHitAngle = 0.0f,
 65 FLOAT m_fChargeHitSpeed = 0.0f,

 // editor variables
 83 CEntityPointer m_penSpawnerTarget,                 // for re-spawning
 84 CEntityPointer m_penDeathTarget "Death target" 'D',                 // death target
 85 enum EventEType m_eetDeathType  "Death event type" 'F' = EET_TRIGGER, // death event type
 86 BOOL m_bTemplate "Template" = FALSE,                  // template enemy for spawning new enemies
 88 RANGE m_fAttackRadius "Radius of attack" 'A' = 10000.0f, // attack sphere range radius
 89 COLOR m_colColor "Color" 'L' = 0x00,    // color
 90 BOOL  m_bDeaf "Deaf" = FALSE,    // deaf
 91 BOOL  m_bBlind "Blind" = FALSE,    // blind
 92 FLOAT m_tmGiveUp "Give up time" = 5.0f,    // how fast enemy gives up attack
 93 FLOAT m_tmReflexMin "Reflex Min" = 0.0f,  // how much to wait before reacting on spotting the player
 94 FLOAT m_tmReflexMax "Reflex Max" = 0.0f,
 95 FLOAT m_fActivityRange "Activity Range" = 0.0f,

 // random values variables
106 BOOL m_bApplyRandomStretch "Apply random stretch" = FALSE,       // apply random stretch
107 FLOAT m_fRandomStretchFactor "Random stretch factor" = 0.1f,     // random stretch
108 FLOAT m_fStretchMultiplier "Stretch multiplier" =  1.0f,         // stretch multiplier
109 FLOAT m_fRandomStretchMultiplier = 1.0f,                         // calculated random stretch

 // marker variables
120 CEntityPointer m_penMarker "Marker" 'M' COLOR(C_RED|0xFF),       // enemy marker pointer

// fuss variables
140 CEntityPointer m_penMainMusicHolder,
141 FLOAT m_tmLastFussTime = 0.0f,

142 FLOAT m_iScore = -100000,        // how many points this enemy gives when killed
143 FLOAT m_fMaxHealth = -1.0f,      // must set this because of crosshair colorizing
144 BOOL  m_bBoss = FALSE,           // set for bosses (for health display)

145 FLOAT m_fSpiritStartTime = 0.0f, // time when spirit effect has started

146 FLOAT m_tmSpraySpawned = 0.0f,   // time when damage has been applied
147 FLOAT m_fSprayDamage = 0.0f,     // total ammount of damage
148 CEntityPointer m_penSpray,       // the blood spray
149 FLOAT m_fMaxDamageAmmount  = 0.0f, // max ammount of damage received in in last few ticks
150 FLOAT3D m_vLastStain  = FLOAT3D(0,0,0), // where last stain was left
151 enum SprayParticlesType m_sptType = SPT_BLOOD, // type of particles

160 CEntityPointer m_penTacticsHolder "Tactics Holder",
161 BOOL  m_bTacticActive = FALSE,
162 FLOAT m_tmTacticsActivation = 0.0f,
// warning! tactic variables are also used for dust spawning
163 FLOAT3D m_vTacticsStartPosition = FLOAT3D(0,0,0),
165 FLOAT m_fTacticVar1 = 0.0f,
166 FLOAT m_fTacticVar2 = 0.0f,
167 FLOAT m_fTacticVar3 = 0.0f,
168 FLOAT m_fTacticVar4 = 0.0f,
169 FLOAT m_fTacticVar5 = 0.0f,      
170 BOOL  m_bTacticsStartOnSense "Tactics start on sense" = FALSE, 
180 COLOR m_colBurning = COLOR(C_WHITE|CT_OPAQUE), // color applied when burning

181 BOOL  m_bResizeAttachments "Stretch attachments" = FALSE, // for small enemies with big guns

//171 INDEX m_iTacticsRetried = 0,

  {
    TIME m_tmPredict;  // time to predict the entity to
  }


components:

  1 class   CLASS_WATCHER         "Classes\\Watcher.ecl",
  2 class   CLASS_PROJECTILE      "Classes\\Projectile.ecl",
  3 class   CLASS_DEBRIS          "Classes\\Debris.ecl",
  4 class   CLASS_BASIC_EFFECT    "Classes\\BasicEffect.ecl",
  5 class   CLASS_BLOOD_SPRAY     "Classes\\BloodSpray.ecl",

// ************** FLESH PARTS **************
 10 model   MODEL_FLESH          "Models\\Effects\\Debris\\Flesh\\Flesh.mdl",
 11 model   MODEL_FLESH_APPLE    "Models\\Effects\\Debris\\Fruits\\Apple.mdl",
 12 model   MODEL_FLESH_BANANA   "Models\\Effects\\Debris\\Fruits\\Banana.mdl",
 13 model   MODEL_FLESH_BURGER   "Models\\Effects\\Debris\\Fruits\\CheeseBurger.mdl",
 14 model   MODEL_FLESH_LOLLY    "Models\\Effects\\Debris\\Fruits\\LollyPop.mdl",
 15 model   MODEL_FLESH_ORANGE   "Models\\Effects\\Debris\\Fruits\\Orange.mdl",

 20 texture TEXTURE_FLESH_RED    "Models\\Effects\\Debris\\Flesh\\FleshRed.tex",
 21 texture TEXTURE_FLESH_GREEN  "Models\\Effects\\Debris\\Flesh\\FleshGreen.tex",
 22 texture TEXTURE_FLESH_APPLE  "Models\\Effects\\Debris\\Fruits\\Apple.tex",       
 23 texture TEXTURE_FLESH_BANANA "Models\\Effects\\Debris\\Fruits\\Banana.tex",      
 24 texture TEXTURE_FLESH_BURGER "Models\\Effects\\Debris\\Fruits\\CheeseBurger.tex",
 25 texture TEXTURE_FLESH_LOLLY  "Models\\Effects\\Debris\\Fruits\\LollyPop.tex",
 26 texture TEXTURE_FLESH_ORANGE "Models\\Effects\\Debris\\Fruits\\Orange.tex",

// ************** MACHINE PARTS **************
 31 model   MODEL_MACHINE        "Models\\Effects\\Debris\\Stone\\Stone.mdl",
 32 texture TEXTURE_MACHINE      "Models\\Effects\\Debris\\Stone\\Stone.tex",


functions:

  void CEnemyBase(void)
  {
    m_tmPredict = 0;
  }

  // called by other entities to set time prediction parameter
  void SetPredictionTime(TIME tmAdvance)   // give time interval in advance to set
  {
    ASSERT(!IsPredictor());
    m_tmPredict = _pTimer->CurrentTick()+tmAdvance;
  }

  // called by engine to get the upper time limit 
  TIME GetPredictionTime(void)   // return moment in time up to which to predict this entity
  {
    return m_tmPredict;
  }

  // describe how this enemy killed player
  virtual CTString GetPlayerKillDescription(const CTString &strPlayerName, const EDeath &eDeath)
  {
    CTString str;
    str.PrintF(TRANSV("%s killed %s"), (const char *) GetClass()->ec_pdecDLLClass->dec_strName, (const char *) strPlayerName);
    return str;
  }

  virtual FLOAT GetCrushHealth(void)
  {
    return 0.0f;
  }

  // if should be counted as kill
  virtual BOOL CountAsKill(void)
  {
    return TRUE;
  }

  virtual BOOL ForcesCannonballToExplode(void)
  {
    return FALSE;
  }

  // overridable function for access to different properties of derived classes (flying/diving)
  virtual FLOAT &GetProp(FLOAT &m_fBase)
  {
    return m_fBase;
  }

  // overridable function to get range for switching to another player
  virtual FLOAT GetThreatDistance(void)
  {
    // closer of close or stop range
    return Max(GetProp(m_fCloseDistance), GetProp(m_fStopDistance));
  }

  // check if we maybe switch to some other player (for large beasts in coop)
  void MaybeSwitchToAnotherPlayer(void)
  {
    // if in single player
    if (GetSP()->sp_bSinglePlayer) {
      // no need to check
      return;
    }

    // if current player is inside threat distance
    if (CalcDist(m_penEnemy)<GetThreatDistance()) {
      // do not switch
      return;
    }
    // maybe switch
    CEntity *penNewEnemy = GetWatcher()->CheckAnotherPlayer(m_penEnemy);
    if (penNewEnemy!=m_penEnemy && penNewEnemy!=NULL) {
      m_penEnemy = penNewEnemy;
      SendEvent(EReconsiderBehavior());
    }
  }

  class CWatcher *GetWatcher(void)
  {
    ASSERT(m_penWatcher!=NULL);
    return (CWatcher*) m_penWatcher.ep_pen;
  }
  export void Copy(CEntity &enOther, ULONG ulFlags)
  {
    CMovableModelEntity::Copy(enOther, ulFlags);
    CEnemyBase *penOther = (CEnemyBase *)(&enOther);
  }

  void Precache(void)
  {
    PrecacheModel(MODEL_FLESH);
    PrecacheModel(MODEL_FLESH_APPLE);
    PrecacheModel(MODEL_FLESH_BANANA);
    PrecacheModel(MODEL_FLESH_BURGER);
    PrecacheModel(MODEL_MACHINE);
    PrecacheTexture(TEXTURE_MACHINE);
    PrecacheTexture(TEXTURE_FLESH_RED);
    PrecacheTexture(TEXTURE_FLESH_GREEN);
    PrecacheTexture(TEXTURE_FLESH_APPLE); 
    PrecacheTexture(TEXTURE_FLESH_BANANA);
    PrecacheTexture(TEXTURE_FLESH_BURGER);
    PrecacheTexture(TEXTURE_FLESH_LOLLY); 
    PrecacheTexture(TEXTURE_FLESH_ORANGE); 
    PrecacheClass(CLASS_BASIC_EFFECT, BET_BLOODSPILL);
    PrecacheClass(CLASS_BASIC_EFFECT, BET_BLOODSTAIN);
    PrecacheClass(CLASS_BASIC_EFFECT, BET_BLOODSTAINGROW);
    PrecacheClass(CLASS_BASIC_EFFECT, BET_BLOODEXPLODE);
    PrecacheClass(CLASS_BASIC_EFFECT, BET_BOMB);
    PrecacheClass(CLASS_BASIC_EFFECT, BET_EXPLOSIONSTAIN);
    PrecacheClass(CLASS_DEBRIS);
  }

  // get position you would like to go to when following player
  virtual FLOAT3D PlayerDestinationPos(void)
  {
    return m_penEnemy->GetPlacement().pl_PositionVector;
  }

  // calculate delta to given entity
  FLOAT3D CalcDelta(CEntity *penEntity) 
  {
    ASSERT(penEntity!=NULL);
    // find vector from you to target
    return penEntity->GetPlacement().pl_PositionVector - GetPlacement().pl_PositionVector;
  };
  // calculate distance to given entity
  FLOAT CalcDist(CEntity *penEntity) 
  {
    return CalcDelta(penEntity).Length();
  };

  BOOL IfTargetCrushed(CEntity *penOther, const FLOAT3D &vDirection)
  {
    if( IsOfClass(penOther, "ModelHolder2"))
    {
      FLOAT fCrushHealth = GetCrushHealth();
      if( fCrushHealth>((CRationalEntity &)*penOther).GetHealth())
      {
        InflictDirectDamage(penOther, this, 
          DMT_EXPLOSION, fCrushHealth, GetPlacement().pl_PositionVector, vDirection);
        return TRUE;
      }
    }
    return FALSE;
  }

  // calculate delta to given entity in current gravity plane
  FLOAT3D CalcPlaneDelta(CEntity *penEntity) 
  {
    ASSERT(penEntity!=NULL);
    FLOAT3D vPlaneDelta;
    // find vector from you to target in XZ plane
    GetNormalComponent(
      penEntity->GetPlacement().pl_PositionVector - GetPlacement().pl_PositionVector,
      en_vGravityDir, vPlaneDelta);
    return vPlaneDelta;
  };

  // calculate distance to given entity in current gravity plane
  FLOAT CalcPlaneDist(CEntity *penEntity)
  {
    return CalcPlaneDelta(penEntity).Length();
  };

  // get cos of angle in direction
  FLOAT GetFrustumAngle(const FLOAT3D &vDir)
  {
    // find front vector
    FLOAT3D vFront = -GetRotationMatrix().GetColumn(3);
    // make dot product to determine if you can see target (view angle)
    return (vDir/vDir.Length())%vFront;
  }

  // get cos of angle in direction in current gravity plane
  FLOAT GetPlaneFrustumAngle(const FLOAT3D &vDir)
  {
    FLOAT3D vPlaneDelta;
    // find vector from you to target in XZ plane
    GetNormalComponent(vDir, en_vGravityDir, vPlaneDelta);
    // find front vector
    FLOAT3D vFront = -GetRotationMatrix().GetColumn(3);
    FLOAT3D vPlaneFront;
    GetNormalComponent(vFront, en_vGravityDir, vPlaneFront);
    // make dot product to determine if you can see target (view angle)
    vPlaneDelta.SafeNormalize();
    vPlaneFront.SafeNormalize();
    return vPlaneDelta%vPlaneFront;
  }

  // determine if you can see something in given direction
  BOOL IsInFrustum(CEntity *penEntity, FLOAT fCosHalfFrustum) 
  {
    // get direction to the entity
    FLOAT3D vDelta = CalcDelta(penEntity);
    // find front vector
    FLOAT3D vFront = -GetRotationMatrix().GetColumn(3);
    // make dot product to determine if you can see target (view angle)
    FLOAT fDotProduct = (vDelta/vDelta.Length())%vFront;
    return fDotProduct >= fCosHalfFrustum;
  };

  // determine if you can see something in given direction in current gravity plane
  BOOL IsInPlaneFrustum(CEntity *penEntity, FLOAT fCosHalfFrustum) 
  {
    // get direction to the entity
    FLOAT3D vPlaneDelta = CalcPlaneDelta(penEntity);
    // find front vector
    FLOAT3D vFront = -GetRotationMatrix().GetColumn(3);
    FLOAT3D vPlaneFront;
    GetNormalComponent(vFront, en_vGravityDir, vPlaneFront);
    // make dot product to determine if you can see target (view angle)
    vPlaneDelta.SafeNormalize();
    vPlaneFront.SafeNormalize();
    FLOAT fDot = vPlaneDelta%vPlaneFront;
    return fDot >= fCosHalfFrustum;
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

  // cast a ray to entity checking all
  BOOL IsVisibleCheckAll(CEntity *penEntity) 
  {
    ASSERT(penEntity!=NULL);
    // get ray source and target
    FLOAT3D vSource, vTarget;
    GetPositionCastRay(this, penEntity, vSource, vTarget);

    // cast the ray
    CCastRay crRay(this, vSource, vTarget);
    crRay.cr_ttHitModels = CCastRay::TT_COLLISIONBOX;   // check for model collision box
    crRay.cr_bHitTranslucentPortals = FALSE;
    en_pwoWorld->CastRay(crRay);

    // if the ray hits wanted entity
    return crRay.cr_penHit==penEntity;
  };

  /* calculates launch velocity and heading correction for angular launch */
  void CalculateAngularLaunchParams(
    FLOAT3D vShooting, FLOAT fShootHeight,
    FLOAT3D vTarget, FLOAT3D vSpeedDest,
    ANGLE aPitch,
    FLOAT &fLaunchSpeed,
    FLOAT &fRelativeHdg)
  {
    FLOAT3D vNewTarget = vTarget;
    FLOAT3D &vGravity = en_vGravityDir;
    FLOAT fYt;
    FLOAT fXt;
    FLOAT fA = TanFast(AngleDeg(aPitch));
    FLOAT fTime = 0.0f;
    FLOAT fLastTime = 0.0f;

    INDEX iIterations = 0;
    do
    {
      iIterations++;
      FLOAT3D vDistance = vNewTarget-vShooting;
      FLOAT3D vXt, vYt;
      GetParallelAndNormalComponents(vDistance, vGravity, vYt, vXt);
      fYt = vYt.Length();
      if (vGravity%vYt>0) {
        fYt=-fYt;
      }
      fXt = vXt.Length();
      fLastTime=fTime;
      fTime = Sqrt(2.0f)*Sqrt((fA*fXt+fShootHeight-fYt)/en_fGravityA);
      vNewTarget = vTarget+vSpeedDest*fTime;
    }
    while( (Abs(fTime-fLastTime) > _pTimer->TickQuantum) && (iIterations<10) );

    // calculate launch speed
    fLaunchSpeed = fXt/(fTime*Cos(aPitch));

    // calculate heading correction
    FLOAT fHdgTargetNow = GetRelativeHeading( (vTarget-vShooting).SafeNormalize());
    FLOAT fHdgTargetMoved = GetRelativeHeading( (vNewTarget-vShooting).SafeNormalize());
    fRelativeHdg = fHdgTargetMoved-fHdgTargetNow;
  }

  /* calculates predicted position for propelled projectile */
  FLOAT3D CalculatePredictedPosition( FLOAT3D vShootPos, FLOAT3D vTarget, 
    FLOAT fSpeedSrc, FLOAT3D vSpeedDst, FLOAT fClampY)
  {
    FLOAT3D vNewTarget = vTarget;
    FLOAT3D &vGravity = en_vGravityDir;
    FLOAT fTime = 0.0f;
    FLOAT fLastTime = 0.0f;
    INDEX iIterations = 0;
    FLOAT3D vDistance = vNewTarget-vShootPos;

    // iterate to obtain accurate position
    do
    {
      iIterations++;
      fLastTime=fTime;
      fTime = vDistance.Length()/fSpeedSrc;
      vNewTarget = vTarget + vSpeedDst*fTime + vGravity*0.5f*fTime*fTime;
      vNewTarget(2) = ClampDn( vNewTarget(2), fClampY);
      vDistance = vNewTarget-vShootPos;
    }
    while( (Abs(fTime-fLastTime) > _pTimer->TickQuantum) && (iIterations<10) );
    return vNewTarget;
  }
  
  /* Check if entity is moved on a route set up by its targets. */
  BOOL MovesByTargetedRoute(CTString &strTargetProperty) const {
    strTargetProperty = "Marker";
    return TRUE;
  };
  /* Check if entity can drop marker for making linked route. */
  BOOL DropsMarker(CTFileName &fnmMarkerClass, CTString &strTargetProperty) const {
    fnmMarkerClass = CTFILENAME("Classes\\EnemyMarker.ecl");
    strTargetProperty = "Marker";
    return TRUE;
  }
  const CTString &GetDescription(void) const {
    ((CTString&)m_strDescription).PrintF("-><none>");
    if (m_penMarker!=NULL) {
      ((CTString&)m_strDescription).PrintF("->%s", (const char *) m_penMarker->GetName());
    }
    return m_strDescription;
  }

  virtual const CTFileName &GetComputerMessageName(void) const {
    static CTFileName fnm(CTString(""));
    return fnm;
  }

  // add to prediction any entities that this entity depends on
  void AddDependentsToPrediction(void)
  {
    m_penSpray->AddToPrediction();
    if (m_penWatcher!=NULL) {
      GetWatcher()->AddToPrediction();
    }
  }

  // create a checksum value for sync-check
  void ChecksumForSync(ULONG &ulCRC, INDEX iExtensiveSyncCheck) {
    CMovableModelEntity::ChecksumForSync(ulCRC, iExtensiveSyncCheck);
  }
  // dump sync data to text file
  void DumpSync_t(CTStream &strm, INDEX iExtensiveSyncCheck)  // throw char *
  {
    CMovableModelEntity ::DumpSync_t(strm, iExtensiveSyncCheck);
    strm.FPrintF_t("enemy: ");
    if (m_penEnemy!=NULL) {
      strm.FPrintF_t("id: %08X\n", m_penEnemy->en_ulID);
    } else {
      strm.FPrintF_t("none\n");
    }
 
    /*INDEX ctStates = en_stslStateStack.Count();
    strm.FPrintF_t("state stack @%gs:\n", _pTimer->CurrentTick());
    for(INDEX iState=ctStates-1; iState>=0; iState--) {
      SLONG slState = en_stslStateStack[iState];
      strm.FPrintF_t("  0x%08x %s\n", slState, en_pecClass->ec_pdecDLLClass->HandlerNameForState(slState));
    }*/

  }

  /* Read from stream. */
  void Read_t( CTStream *istr) {
    CMovableModelEntity::Read_t(istr);

    // add to fuss if needed
    if (m_penMainMusicHolder!=NULL) {
      ((CMusicHolder&)*m_penMainMusicHolder).m_cenFussMakers.Add(this);
    }
  };

  /* Fill in entity statistics - for AI purposes only */
  BOOL FillEntityStatistics(EntityStats *pes)
  {
    pes->es_strName = GetClass()->ec_pdecDLLClass->dec_strName;
    if (m_bTemplate) {
      pes->es_ctCount = 0;
    } else {
      pes->es_ctCount = 1;
    }
    pes->es_ctAmmount = 1;
    pes->es_fValue = GetHealth();
    pes->es_iScore = (INDEX) m_iScore;
    return TRUE;
  }

  /* Receive damage */
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    // if template
    if (m_bTemplate) {
      // do nothing
      return;
    }

    FLOAT fNewDamage = fDamageAmmount;

    // adjust damage
    fNewDamage *=DamageStrength( ((EntityInfo*)GetEntityInfo())->Eeibt, dmtType);
    // apply game extra damage per enemy and per player
    fNewDamage *=GetGameDamageMultiplier();

    // if no damage
    if (fNewDamage==0) {
      // do nothing
      return;
    }
    FLOAT fKickDamage = fNewDamage;
    if( (dmtType == DMT_EXPLOSION) || (dmtType == DMT_IMPACT) || (dmtType == DMT_CANNONBALL_EXPLOSION) )
    {
      fKickDamage*=1.5f;
    }
    if (dmtType==DMT_DROWNING || dmtType==DMT_CLOSERANGE || dmtType==DMT_CHAINSAW) {
      fKickDamage /= 10.0f;
    }
    if (dmtType==DMT_BURNING)
    {
      fKickDamage /= 100000.0f;
      UBYTE ubR, ubG, ubB, ubA;
      FLOAT fColorFactor=fNewDamage/m_fMaxHealth*255.0f;
      ColorToRGBA(m_colBurning, ubR, ubG, ubB, ubA);
      ubR=(UBYTE)ClampDn(ubR-fColorFactor, 32.0f);
      m_colBurning=RGBAToColor(ubR, ubR, ubR, ubA);
    }

    // get passed time since last damage
    TIME tmNow = _pTimer->CurrentTick();
    TIME tmDelta = tmNow-m_tmLastDamage;
    m_tmLastDamage = tmNow;

    // fade damage out
    if (tmDelta>=_pTimer->TickQuantum*3) {
      m_vDamage=FLOAT3D(0,0,0);
    }
    // add new damage
    FLOAT3D vDirectionFixed;
    if (vDirection.ManhattanNorm()>0.5f) {
      vDirectionFixed = vDirection;
    } else {
      vDirectionFixed = -en_vGravityDir;
    }
    FLOAT3D vDamageOld = m_vDamage;
/*    if( (dmtType == DMT_EXPLOSION) || (dmtType == DMT_CANNONBALL_EXPLOSION) )
    {
      m_vDamage+=(vDirectionFixed/2-en_vGravityDir/2)*fKickDamage;
    }
    else*/
    {
      m_vDamage+=(vDirectionFixed-en_vGravityDir/2)*fKickDamage;
    }
    
    FLOAT fOldLen = vDamageOld.Length();
    FLOAT fNewLen = m_vDamage.Length();
    FLOAT fOldRootLen = Sqrt(fOldLen);
    FLOAT fNewRootLen = Sqrt(fNewLen);

    FLOAT fMassFactor = 300.0f/((EntityInfo*)GetEntityInfo())->fMass;
    
    if( !(en_ulFlags & ENF_ALIVE))
    {
      fMassFactor /= 3;
    }

    if(fOldLen != 0.0f)
    {
      // cancel last push
      GiveImpulseTranslationAbsolute( -vDamageOld/fOldRootLen*fMassFactor);
    }

    //-en_vGravityDir*fPushStrength/10

    // push it back
    GiveImpulseTranslationAbsolute( m_vDamage/fNewRootLen*fMassFactor);

    /*if ((m_tmSpraySpawned<=_pTimer->CurrentTick()-_pTimer->TickQuantum || 
      m_fSprayDamage+fNewDamage>50.0f)
      && m_fSpiritStartTime==0) {*/
    
    if( m_fMaxDamageAmmount<fDamageAmmount)
    {
      m_fMaxDamageAmmount = fDamageAmmount;
    }
    // if it has no spray, or if this damage overflows it, and not already disappearing
    if ((m_tmSpraySpawned<=_pTimer->CurrentTick()-_pTimer->TickQuantum*8 || 
      m_fSprayDamage+fNewDamage>50.0f)
      && m_fSpiritStartTime==0 &&
      dmtType!=DMT_CHAINSAW && 
      !(dmtType==DMT_BURNING && GetHealth()<0) ) {

      // spawn blood spray
      CPlacement3D plSpray = CPlacement3D( vHitPoint, ANGLE3D(0, 0, 0));
      m_penSpray = CreateEntity( plSpray, CLASS_BLOOD_SPRAY);
      if(m_sptType != SPT_ELECTRICITY_SPARKS)
      {
        m_penSpray->SetParent( this);
      }

      ESpawnSpray eSpawnSpray;
      eSpawnSpray.colBurnColor=C_WHITE|CT_OPAQUE;
      
      if( m_fMaxDamageAmmount > 10.0f)
      {
        eSpawnSpray.fDamagePower = 3.0f;
      }
      else if(m_fSprayDamage+fNewDamage>50.0f)
      {
        eSpawnSpray.fDamagePower = 2.0f;
      }
      else
      {
        eSpawnSpray.fDamagePower = 1.0f;
      }

      eSpawnSpray.sptType = m_sptType;
      eSpawnSpray.fSizeMultiplier = 1.0f;

      // setup direction of spray
      FLOAT3D vHitPointRelative = vHitPoint - GetPlacement().pl_PositionVector;
      FLOAT3D vReflectingNormal;
      GetNormalComponent( vHitPointRelative, en_vGravityDir, vReflectingNormal);
      vReflectingNormal.SafeNormalize();
      
      vReflectingNormal(1)/=5.0f;
    
      FLOAT3D vProjectedComponent = vReflectingNormal*(vDirection%vReflectingNormal);
      FLOAT3D vSpilDirection = vDirection-vProjectedComponent*2.0f-en_vGravityDir*0.5f;

      eSpawnSpray.vDirection = vSpilDirection;
      eSpawnSpray.penOwner = this;
    
      /*if (dmtType==DMT_BURNING && GetHealth()<0)
      {
        eSpawnSpray.fDamagePower = 1.0f;
      }*/

      // initialize spray
      m_penSpray->Initialize( eSpawnSpray);
      m_tmSpraySpawned = _pTimer->CurrentTick();
      m_fSprayDamage = 0.0f;
      m_fMaxDamageAmmount = 0.0f;
    }
    m_fSprayDamage+=fNewDamage;

    CMovableModelEntity::ReceiveDamage(penInflictor, 
      dmtType, fNewDamage, vHitPoint, vDirection);
  };


/************************************************************
 *                        FADE OUT                          *
 ************************************************************/

  BOOL AdjustShadingParameters(FLOAT3D &vLightDirection, COLOR &colLight, COLOR &colAmbient)
  {
    colAmbient = AddColors( colAmbient, m_colColor);
    if( m_bFadeOut) {
      FLOAT fTimeRemain = m_fFadeStartTime + m_fFadeTime - _pTimer->CurrentTick();
      if( fTimeRemain < 0.0f) { fTimeRemain = 0.0f; }
      COLOR colAlpha;
      if(en_RenderType == RT_SKAMODEL || en_RenderType == RT_SKAEDITORMODEL) {
        colAlpha = GetModelInstance()->GetModelColor();
        colAlpha = (colAlpha&0xFFFFFF00) + (COLOR(fTimeRemain/m_fFadeTime*0xFF)&0xFF);
        GetModelInstance()->SetModelColor(colAlpha);
      }
      else {
        colAlpha = GetModelObject()->mo_colBlendColor;
        colAlpha = (colAlpha&0xFFFFFF00) + (COLOR(fTimeRemain/m_fFadeTime*0xFF)&0xFF);
        GetModelObject()->mo_colBlendColor = colAlpha;
      }
       
    } else {
      if (GetSP()->sp_bMental) {
        if (GetHealth()<=0) {
          if(en_RenderType == RT_SKAMODEL || en_RenderType == RT_SKAEDITORMODEL) {
            GetModelInstance()->SetModelColor(C_WHITE&0xFF);
          } else {
            GetModelObject()->mo_colBlendColor = C_WHITE&0xFF;
          }
        } else {
          extern FLOAT ent_tmMentalIn  ;
          extern FLOAT ent_tmMentalOut ;
          extern FLOAT ent_tmMentalFade;
          FLOAT tmIn   = ent_tmMentalIn  ;
          FLOAT tmOut  = ent_tmMentalOut ;
          FLOAT tmFade = ent_tmMentalFade;
          FLOAT tmExist = tmFade+tmIn+tmFade;
          FLOAT tmTotal = tmFade+tmIn+tmFade+tmOut;
          
          FLOAT tmTime = _pTimer->GetLerpedCurrentTick();
          FLOAT fFactor = 1;
          if (tmTime>0.1f) {
            tmTime += en_ulID*123.456f;
            tmTime = fmod(tmTime, tmTotal);
            fFactor = CalculateRatio(tmTime, 0, tmExist, tmFade/tmExist, tmFade/tmExist);
          }
          
          if(en_RenderType == RT_SKAMODEL || en_RenderType == RT_SKAEDITORMODEL) {
            GetModelInstance()->SetModelColor(C_WHITE|INDEX(0xFF*fFactor)); 
          } else {
            GetModelObject()->mo_colBlendColor = C_WHITE|INDEX(0xFF*fFactor);
          }
        }
      }
    }
    if(m_colBurning!=COLOR(C_WHITE|CT_OPAQUE))
    {
      colAmbient = MulColors( colAmbient, m_colBurning);
      colLight = MulColors( colLight, m_colBurning);
    }
    return CMovableModelEntity::AdjustShadingParameters(vLightDirection, colLight, colAmbient);
  };


  // fuss functions
  void AddToFuss(void)
  {
    if (IsPredictor()) {
      // remember last fuss time
      m_tmLastFussTime = _pTimer->CurrentTick();
      return;
    }

    // if no music holder remembered - not in fuss
    if (m_penMainMusicHolder==NULL) {
      // find main music holder
      m_penMainMusicHolder = _pNetwork->GetEntityWithName("MusicHolder", 0);
      // if no music holder found
      if (m_penMainMusicHolder==NULL) {
        // just remember last fuss time
        m_tmLastFussTime = _pTimer->CurrentTick();
        // cannot make fuss
        return;
      }
      // add to end of fuss list
      ((CMusicHolder&)*m_penMainMusicHolder).m_cenFussMakers.Add(this);
      // if boss set as boss
      if (m_bBoss) {
        ((CMusicHolder&)*m_penMainMusicHolder).m_penBoss = this;
      }
      // remember last fuss time
      m_tmLastFussTime = _pTimer->CurrentTick();

    // if music holder remembered - still in fuss
    } else {
      // must be in list
      ASSERT(((CMusicHolder&)*m_penMainMusicHolder).m_cenFussMakers.IsMember(this));
      // if boss set as boss
      if (m_bBoss) {
        ((CMusicHolder&)*m_penMainMusicHolder).m_penBoss = this;
      }
      // just remember last fuss time
      m_tmLastFussTime = _pTimer->CurrentTick();
    }
  }
  void RemoveFromFuss(void)
  {
    if (IsPredictor()) {
      return;
    }
    // if no music holder remembered
    if (m_penMainMusicHolder==NULL) {
      // not in fuss
      return;
    }
    // just remove from list
    ((CMusicHolder&)*m_penMainMusicHolder).m_cenFussMakers.Remove(this);
    // if boss, clear boss
    if (m_bBoss) {
      if (((CMusicHolder&)*m_penMainMusicHolder).m_penBoss != this) {
        CPrintF(TRANSV("More than one boss active!\n"));
        ((CMusicHolder&)*m_penMainMusicHolder).m_penBoss = NULL;
      }
    }
    m_penMainMusicHolder = NULL;
  }

  // check if should give up attacking
  BOOL ShouldCeaseAttack(void)
  {
    // if there is no valid the enemy
    if (m_penEnemy==NULL ||
      !(m_penEnemy->GetFlags()&ENF_ALIVE) || 
       (m_penEnemy->GetFlags()&ENF_DELETED)) {
      // cease attack
      return TRUE;
    }
    // if not active in fighting
    if (_pTimer->CurrentTick()>m_tmLastFussTime+m_tmGiveUp) {
      // cease attack
      return TRUE;
    }
    // otherwise, continue
    return FALSE;
  }

  // Stretch model - MUST BE DONE BEFORE SETTING MODEL!
  virtual void SizeModel(void)
  {
    FLOAT3D vStretch = GetModelStretch();
    
    // apply defined stretch
    vStretch *= m_fStretchMultiplier;

    // if should apply random stretch
    if (m_bApplyRandomStretch)
    {
      // will be done only when user clicks "Apply random" switch
      m_bApplyRandomStretch = FALSE;
      // get rnd for random stretch
      m_fRandomStretchMultiplier = (FRnd()-0.5f)*m_fRandomStretchFactor+1.0f;
    }
    
    // apply random stretch
    vStretch *= m_fRandomStretchMultiplier;

    if (m_bResizeAttachments) {
      StretchModel( vStretch);
    } else if (TRUE) {      
      StretchSingleModel( vStretch);
    }
    ModelChangeNotify();
  };

  // check if an entity is valid for being your new enemy
  BOOL IsValidForEnemy(CEntity *penPlayer)
  {
    return 
      penPlayer!=NULL && 
      IsDerivedFromClass(penPlayer, "Player") &&
      penPlayer->GetFlags()&ENF_ALIVE;
  }
  
  // unset target
  void SetTargetNone(void)
  {
    m_ttTarget = TT_NONE;
    m_dtDestination = DT_PLAYERCURRENT;
    m_penEnemy = NULL;
  }

  // set new player as soft target if possible
  BOOL SetTargetSoft(CEntity *penPlayer)
  {
    // if invalid target
    if (!IsValidForEnemy(penPlayer)) {
      // do nothing
      return FALSE;
    }
    // if we already have any kind of target
    if (m_ttTarget!=TT_NONE) {
      // do nothing
      return FALSE;
    }
    // remember new soft target
    CEntity *penOld = m_penEnemy;
    m_ttTarget = TT_SOFT;
    m_dtDestination = DT_PLAYERCURRENT;
    m_penEnemy = penPlayer;
    return penOld!=penPlayer;
  }

  // set new player as hard target if possible
  BOOL SetTargetHard(CEntity *penPlayer)
  {
    // if invalid target
    if (!IsValidForEnemy(penPlayer)) {
      // do nothing
      return FALSE;
    }
    // if we already have hard target
    if (m_ttTarget==TT_HARD) {
      // do nothing
      return FALSE;
    }
    // remember new hard target
    CEntity *penOld = m_penEnemy;
    m_ttTarget = TT_HARD;
    m_dtDestination = DT_PLAYERCURRENT;
    m_penEnemy = penPlayer;
    return penOld!=penPlayer;
  }

  // force new player to be hard target
  BOOL SetTargetHardForce(CEntity *penPlayer)
  {
    // if invalid target
    if (!IsValidForEnemy(penPlayer)) {
      // do nothing
      return FALSE;
    }
    // remember new hard target
    CEntity *penOld = m_penEnemy;
    m_ttTarget = TT_HARD;
    m_dtDestination = DT_PLAYERCURRENT;
    m_penEnemy = penPlayer;
    return penOld!=penPlayer;
  }

/************************************************************
 *                     MOVING FUNCTIONS                     *
 ************************************************************/

  // get movement frequency for attack
  virtual FLOAT GetAttackMoveFrequency(FLOAT fEnemyDistance)
  {
    if (fEnemyDistance>GetProp(m_fCloseDistance)) {
      return 0.5f;
    } else {
      return 0.25f;
    }
  }

  // set speeds for movement towards desired position
  virtual void SetSpeedsToDesiredPosition(const FLOAT3D &vPosDelta, FLOAT fPosDist, BOOL bGoingToPlayer)
  {
    FLOAT fEnemyDistance = CalcDist(m_penEnemy);
    FLOAT fCloseDistance = GetProp(m_fCloseDistance);
    FLOAT fStopDistance = GetProp(m_fStopDistance);
    // find relative direction angle
    FLOAT fCos = GetPlaneFrustumAngle(vPosDelta);
    // if may move and
    if (MayMoveToAttack() && 
      // more or less ahead and
      fCos>CosFast(45.0f) && 
      // not too close
      fEnemyDistance>fStopDistance) {
      // move and rotate towards it
      if (fEnemyDistance<fCloseDistance) {
        m_fMoveSpeed = GetProp(m_fCloseRunSpeed);
        m_aRotateSpeed = GetProp(m_aCloseRotateSpeed);
      } else {
        m_fMoveSpeed = GetProp(m_fAttackRunSpeed);
        m_aRotateSpeed = GetProp(m_aAttackRotateSpeed);
      }

    // otherwise if following tactics, move anyway
    } else if (m_bTacticActive) {
      // move and rotate towards it  
      if (fEnemyDistance<fCloseDistance) {
        m_fMoveSpeed = GetProp(m_fCloseRunSpeed);
        m_aRotateSpeed = GetProp(m_aCloseRotateSpeed);
      } else {
        m_fMoveSpeed = GetProp(m_fAttackRunSpeed);
        m_aRotateSpeed = GetProp(m_aAttackRotateSpeed);
      }

      // otherwise if not exactly in front
    } else if (fCos<CosFast(15.0f)) {
      // just rotate towards it
      m_fMoveSpeed = 0;
      if (fEnemyDistance<fCloseDistance) {
        m_aRotateSpeed = GetProp(m_aCloseRotateSpeed);
      } else {
        m_aRotateSpeed = GetProp(m_aAttackRotateSpeed);
      }

      // otherwise (stop range)
    } else {
      // if going towards player, or would leave attackradius
      if (bGoingToPlayer || !WouldNotLeaveAttackRadius()) {
        // stand in place
        m_fMoveSpeed = 0;
        m_aRotateSpeed = 0;
      // if going to some other location (some pathfinding AI scheme)
      } else {
        m_fMoveSpeed = GetProp(m_fCloseRunSpeed);
        m_aRotateSpeed = GetProp(m_aCloseRotateSpeed);
      }
    }
  }

  // get movement animation for given flags with current movement type
  virtual void MovementAnimation(ULONG ulFlags)
  {
    if (ulFlags&MF_MOVEZ) {
      if (m_fMoveSpeed==GetProp(m_fAttackRunSpeed) || m_fMoveSpeed==GetProp(m_fCloseRunSpeed)
        || m_fMoveSpeed>GetProp(m_fWalkSpeed)) {
        RunningAnim();
      } else {
        WalkingAnim();
      }
    } else if (ulFlags&MF_ROTATEH) {
      RotatingAnim();
    } else {
      if (m_penEnemy!=NULL) {
        StandingAnimFight();
      } else {
        StandingAnim();
      }
    }
  }

  // set desired rotation and translation to go/orient towards desired position
  // and get the resulting movement type
  virtual ULONG SetDesiredMovement(void) 
  {
    ULONG ulFlags = 0;

    // get delta to desired position
    FLOAT3D vDelta = m_vDesiredPosition - GetPlacement().pl_PositionVector;

    if (m_dtDestination==DT_PLAYERCURRENT) {
      ApplyTactics(vDelta);
    }
    
    // if we may rotate
    if (m_aRotateSpeed>0.0f) {
      // get desired heading orientation
      FLOAT3D vDir = vDelta;
      vDir.SafeNormalize();
      ANGLE aWantedHeadingRelative = GetRelativeHeading(vDir);

      // normalize it to [-180,+180] degrees
      aWantedHeadingRelative = NormalizeAngle(aWantedHeadingRelative);

      ANGLE aHeadingRotation;
      // if desired position is left
      if (aWantedHeadingRelative < -m_aRotateSpeed*m_fMoveFrequency) {
        // start turning left
        aHeadingRotation = -m_aRotateSpeed;
      // if desired position is right
      } else if (aWantedHeadingRelative > m_aRotateSpeed*m_fMoveFrequency) {
        // start turning right
        aHeadingRotation = +m_aRotateSpeed;
      // if desired position is more-less ahead
      } else {
        // keep just the adjusting fraction of speed 
        aHeadingRotation = aWantedHeadingRelative/m_fMoveFrequency;
      }
      // start rotating
      SetDesiredRotation(ANGLE3D(aHeadingRotation, 0, 0));
      
      if (Abs(aHeadingRotation)>1.0f) {
        ulFlags |= MF_ROTATEH;
      }

    // if we may not rotate
    } else {
      // stop rotating
      SetDesiredRotation(ANGLE3D(0, 0, 0));
    }

    // if we may move
    if (m_fMoveSpeed>0.0f) {
      // determine translation speed
      FLOAT3D vTranslation(0.0f, 0.0f, 0.0f);
      vTranslation(3) = -m_fMoveSpeed;

      // start moving
      SetDesiredTranslation(vTranslation);

      ulFlags |= MF_MOVEZ;

    // if we may not move
    } else {
      // stop translating
      SetDesiredTranslation(FLOAT3D(0, 0, 0));
    }

    return ulFlags;
  };

  // stop moving entity
  void StopMoving() 
  {
    StopRotating();
    StopTranslating();
  };

  // stop desired rotation
  void StopRotating() 
  {
    SetDesiredRotation(ANGLE3D(0, 0, 0));
  };

  // stop desired translation
  void StopTranslating() 
  {
    SetDesiredTranslation(FLOAT3D(0.0f, 0.0f, 0.0f));
  };

  // calc distance to entity in one plane (relative to owner gravity)
  FLOAT CalcDistanceInPlaneToDestination(void) 
  {
    // find vector from you to target in XZ plane
    FLOAT3D vNormal;
    GetNormalComponent(m_vDesiredPosition - GetPlacement().pl_PositionVector, en_vGravityDir, vNormal);
    return vNormal.Length();
  };

  // initialize path finding
  virtual void StartPathFinding(void)
  {
    ASSERT(m_dtDestination==DT_PATHPERSISTENT || m_dtDestination==DT_PATHTEMPORARY);

    CEntity *penMarker;
    FLOAT3D vPath;
    // find first marker to go to
    PATH_FindFirstMarker(this, 
      GetPlacement().pl_PositionVector, m_penEnemy->GetPlacement().pl_PositionVector,
      penMarker, vPath);
    // if not found, or not visible
    if (penMarker==NULL || !IsVisible(penMarker)) {
      // no path finding
      m_dtDestination=DT_PLAYERSPOTTED;
      // remember as if spotted position
      m_vPlayerSpotted = PlayerDestinationPos();
      return;
    }
    // remember the marker and position
    m_vDesiredPosition = vPath,
    m_penPathMarker = penMarker;
  }

  // find next navigation marker to go to
  virtual void FindNextPathMarker(void)
  {
    // if invalid situation
    if (m_penPathMarker==NULL) {
      // this should not happen
      ASSERT(FALSE);
      // no path finding
      m_dtDestination=DT_PLAYERCURRENT;
      return;
    }

    // find first marker to go to
    CEntity *penMarker = m_penPathMarker;
    FLOAT3D vPath;
    PATH_FindNextMarker(this,
      GetPlacement().pl_PositionVector, m_penEnemy->GetPlacement().pl_PositionVector,
      penMarker, vPath);

    // if not found
    if (penMarker==NULL || !IsVisible(penMarker)) {
      // no path finding
      m_dtDestination=DT_PLAYERSPOTTED;
      // remember as if spotted position
      m_vPlayerSpotted = PlayerDestinationPos();
      return;
    }

    // remember the marker and position
    m_vDesiredPosition = vPath,
    m_penPathMarker = penMarker;
  }

  // check if a touch event triggers pathfinding
  BOOL CheckTouchForPathFinding(const ETouch &eTouch)
  {
    // if no enemy
    if (m_penEnemy==NULL) {
      // do nothing
      return FALSE;
    }

    // if already path finding
    if (m_dtDestination==DT_PATHPERSISTENT || m_dtDestination==DT_PATHTEMPORARY) {
      // do nothing
      return FALSE;
    }

    FLOAT3D vDir = en_vDesiredTranslationRelative;
    vDir.SafeNormalize();
    vDir*=GetRotationMatrix();
    // if the touched plane is more or less orthogonal to the current velocity
    if ((eTouch.plCollision%vDir)<-0.5f) {
      if (m_penEnemy!=NULL && IsVisible(m_penEnemy)) {
        m_dtDestination = DT_PATHPERSISTENT;
      } else {
        m_dtDestination = DT_PATHTEMPORARY;
      }
      StartPathFinding();
      return m_penPathMarker!=NULL;
    } else {
      return FALSE;
    }
  }

  // check if a wouldfall event triggers pathfinding
  BOOL CheckFallForPathFinding(const EWouldFall &eWouldFall)
  {
    // if no enemy
    if (m_penEnemy==NULL) {
      // do nothing
      return FALSE;
    }

    // if already path finding
    if (m_dtDestination==DT_PATHPERSISTENT || m_dtDestination==DT_PATHTEMPORARY) {
      // do nothing
      return FALSE;
    }

    if (m_penEnemy!=NULL && IsVisible(m_penEnemy)) {
      m_dtDestination = DT_PATHPERSISTENT;
    } else {
      m_dtDestination = DT_PATHTEMPORARY;
    }
    StartPathFinding();

    return m_penPathMarker!=NULL;
  }

  /************************************************************
 *                   TACTICS FUNCTIONS                      *
 ************************************************************/

  void InitializeTactics( void )   {
  
    // return if there is no tactics manager or if it points to wrong type of entity
    // or if there is no enemy
    if (m_penTacticsHolder==NULL || !IsOfClass(m_penTacticsHolder, "TacticsHolder")
        || m_penEnemy==NULL) {
      return;
    }
  
    CTacticsHolder *penTactics = &(CTacticsHolder &)*m_penTacticsHolder;

    //m_tmTacticsActivation = penTactics->m_tmLastActivation;
    m_tmTacticsActivation = _pTimer->CurrentTick();
    m_vTacticsStartPosition = GetPlacement().pl_PositionVector;
    //m_iTacticsRetried = penTactics->m_bRetryCount;
    
    FLOAT fSign;
    // sign for randomized parameters
    if (Sgn(penTactics->m_fParam2)>0 && Sgn(penTactics->m_fParam1)>0) {
      fSign = +1.0f;
    } else if (Sgn(penTactics->m_fParam2)<0 && Sgn(penTactics->m_fParam1)<0) {
      fSign = -1.0f;
    } else {
      fSign = Sgn(FRnd()-0.5f);
    }
    
    switch (penTactics->m_tctType) {
      case TCT_DAMP_ANGLE_STRIFE: {
        // 1) random angle (<max, >min)
        m_fTacticVar1=Lerp(Abs(penTactics->m_fParam1), Abs(penTactics->m_fParam2), FRnd())*fSign;
        // 2) time dump
        m_fTacticVar2=penTactics->m_fParam4;
        // 3) dump factor, factor (0-1) of min distance when linear behaviour begins
        m_fTacticVar3=penTactics->m_fParam3;
        // 4) initial distance
        m_fTacticVar4=(m_penEnemy->GetPlacement().pl_PositionVector - m_vTacticsStartPosition).Length();
        // 5) tactics stop distance
        m_fTacticVar5=penTactics->m_fParam5;
        break; }

      case TCT_PARALLEL_RANDOM_DISTANCE:
        // 1) randomized distance
        m_fTacticVar1=Lerp(penTactics->m_fParam4, penTactics->m_fParam5, FRnd());
        // 4) emission angle
        m_fTacticVar4=Lerp(Abs(penTactics->m_fParam1), Abs(penTactics->m_fParam2), FRnd())*fSign;
        // 2) tolerance strip width
        m_fTacticVar2=m_fAttackRunSpeed*2.0f*90.0f/m_aAttackRotateSpeed;
        //m_fTacticVar2=2.0f*m_fAttackRunSpeed;
        // 3) fade in/out ratio
        m_fTacticVar3=penTactics->m_fParam3;
        // 5) initial distance
        m_fTacticVar5=(GetPlacement().pl_PositionVector - m_penEnemy->GetPlacement().pl_PositionVector).Length();
        // as a precausion, assume minimal strip width of 2m
        m_fTacticVar2=ClampDn(m_fTacticVar2, 2.0f);
        
        break;

      case TCT_STATIC_RANDOM_V_DISTANCE:
        // 1) starting angle  
        m_fTacticVar1=Lerp(Abs(penTactics->m_fParam1), Abs(penTactics->m_fParam2), FRnd())*fSign;
        // 2) time to run in the desired V direction
        m_fTacticVar2=Lerp(penTactics->m_fParam3, penTactics->m_fParam4, FRnd());
        break;
    }
  }

  virtual void ApplyTactics( FLOAT3D &vDesiredPos) {
    
    // return if there is no tactics manager or if it points to wrong type of entity
    // or if there is no enemy
    if (m_penTacticsHolder==NULL || !IsOfClass(m_penTacticsHolder, "TacticsHolder")
        || m_penEnemy==NULL) {
      return;
    }
  
    CTacticsHolder *penTactics = &(CTacticsHolder &)*m_penTacticsHolder;

    // See if the last activation time of TacticsHolder is greater then the activation
    // time of this monster. If so, start (or reinitialize) the tactics.
    if (penTactics->m_tmLastActivation==-1 || penTactics->m_tctType==TCT_NONE) {
      m_bTacticActive = FALSE;
    } else if (m_tmTacticsActivation < penTactics->m_tmLastActivation) {
      InitializeTactics();
      m_bTacticActive = TRUE;
    }
    
    if (m_bTacticActive) {

      // calculate shared parameters
      FLOAT3D vEnemyDistance=m_vTacticsStartPosition - m_penEnemy->GetPlacement().pl_PositionVector;
      FLOAT   fEnemyDistance=vEnemyDistance.Length();
              vEnemyDistance.SafeNormalize();
      ANGLE3D angEnemy = ANGLE3D(0.0f, 0.0f, 0.0f);
              //DirectionVectorToAngles(vEnemyDistance, angEnemy);
      
      FLOAT fDistanceRatio = 0.0f;
      FLOAT fTimeRatio = 0.0f;

      switch(penTactics->m_tctType)
      {
      case TCT_DAMP_ANGLE_STRIFE: {
        // if very close to player, stop using tactics
        if (CalcDist(m_penEnemy)<m_fTacticVar5) {
          m_bTacticActive = FALSE;
        }
        
        fDistanceRatio=1.0f;
        if(m_fTacticVar3>0) {
          // get enemy distance
          FLOAT fClamped=Clamp(CalcDist(m_penEnemy)-(m_fTacticVar4*m_fTacticVar3), 0.0f, m_fTacticVar4);
          fDistanceRatio=fClamped/(m_fTacticVar4*(1-m_fTacticVar3));
        }
        
        fTimeRatio=1.0f;
        if(m_fTacticVar2>0) {
          fTimeRatio=1.0f-(ClampUp((_pTimer->CurrentTick() - m_tmTacticsActivation)/m_fTacticVar2, 1.0f));
        }
      
        angEnemy(1) = m_fTacticVar1*fDistanceRatio*fTimeRatio;
        angEnemy(2) = 0.0f;
        angEnemy(3) = 0.0f;

        FLOATmatrix3D mHeading;
        MakeRotationMatrixFast(mHeading, angEnemy);        
        vDesiredPos = vDesiredPos*!en_mRotation;
        vDesiredPos = vDesiredPos*mHeading;
        vDesiredPos = vDesiredPos*en_mRotation;

        break; }
      
      case TCT_PARALLEL_RANDOM_DISTANCE: {
        // line from spawner to player
        FLOAT3D vLinePlayerToSpawn = m_vTacticsStartPosition - m_penEnemy->GetPlacement().pl_PositionVector;
        // line from *this to player
        FLOAT3D vLinePlayerToThis = GetPlacement().pl_PositionVector - m_penEnemy->GetPlacement().pl_PositionVector;       
                
        FLOAT fThisOnLine = (vLinePlayerToThis%vLinePlayerToSpawn)/vLinePlayerToSpawn.Length();
        FLOAT3D vThisOnLine = m_penEnemy->GetPlacement().pl_PositionVector + vLinePlayerToSpawn.SafeNormalize()*fThisOnLine;
        
        FLOAT fLineDist = (GetPlacement().pl_PositionVector - vThisOnLine).Length();
        
        FLOATmatrix3D mHeading;

        //CPrintF("line dst = %f at %f\n", fLineDist, _pTimer->CurrentTick());
        // if close enough to enemy stop tactics and go linear
        if (vLinePlayerToThis.Length()<m_fTacticVar1) {
          m_bTacticActive = FALSE;
        // if closer to spawner-enemy line then supposed to
        } else if (fLineDist<m_fTacticVar1) {
          if (fLineDist<1.0f) { fLineDist=1.0f; }
          angEnemy(1) = m_fTacticVar4/fLineDist;
          angEnemy(2) = 0.0f;
          angEnemy(3) = 0.0f;
                  
          MakeRotationMatrixFast(mHeading, angEnemy);        
          vDesiredPos = vDesiredPos*!en_mRotation;
          vDesiredPos = vDesiredPos*mHeading;
          vDesiredPos = vDesiredPos*en_mRotation;
        // if further from spawner-enemy line then supposed to
        } else if (fLineDist>m_fTacticVar1+m_fTacticVar2) {       
          if (fLineDist<1.0f) { fLineDist=1.0f; }
          angEnemy(1) = -m_fTacticVar4/fLineDist;
          angEnemy(2) = 0.0f;
          angEnemy(3) = 0.0f;
                  
          MakeRotationMatrixFast(mHeading, angEnemy);        
          vDesiredPos = vDesiredPos*!en_mRotation;
          vDesiredPos = vDesiredPos*mHeading;
          vDesiredPos = vDesiredPos*en_mRotation;
          // right on the line
        } else {
          vDesiredPos = -vLinePlayerToSpawn;
        }
        break; }

      case TCT_STATIC_RANDOM_V_DISTANCE: {
        if (_pTimer->CurrentTick()<m_tmTacticsActivation+m_fTacticVar2) {
          angEnemy(1) = m_fTacticVar1;
          angEnemy(2) = 0.0f;
          angEnemy(3) = 0.0f;        
        } else {
          m_bTacticActive = FALSE;
        }
        
        FLOATmatrix3D mHeading;
        MakeRotationMatrixFast(mHeading, angEnemy);        
        vDesiredPos = vDesiredPos*!en_mRotation;
        vDesiredPos = vDesiredPos*mHeading;
        vDesiredPos = vDesiredPos*en_mRotation;
        
        break; }
      } 
    }
  }

  void StartTacticsNow ( void ) {
    m_tmTacticsActivation = -1.0f;    
  }

/************************************************************
 *                   ATTACK SPECIFIC                        *
 ************************************************************/
  // can attack (shoot) at entity in plane - ground support
  BOOL CanAttackEnemy(CEntity *penTarget, FLOAT fCosAngle) {
    if (IsInPlaneFrustum(penTarget, fCosAngle)) {
      if (IsVisibleCheckAll(penTarget)) {
        return TRUE;
      }
    }
    return FALSE;
  };

  // close attack if possible
  virtual BOOL CanHitEnemy(CEntity *penTarget, FLOAT fCosAngle) {
    if (IsInFrustum(penTarget, fCosAngle)) {
      return IsVisibleCheckAll(penTarget);
    }
    return FALSE;
  };

  // see entity
  BOOL SeeEntity(CEntity *pen, FLOAT fCosAngle) {
    if (IsInFrustum(pen, fCosAngle)) {
      return IsVisible(pen);
    }
    return FALSE;
  };

  // see entity in plane
  BOOL SeeEntityInPlane(CEntity *pen, FLOAT fCosAngle) {
    CalcPlaneDist(pen);
    if (IsInPlaneFrustum(pen, fCosAngle)) {
      return IsVisible(pen);
    }
    return FALSE;
  };

  // prepare propelled projectile
  void PreparePropelledProjectile(CPlacement3D &plProjectile, const FLOAT3D vShootTarget,
    const FLOAT3D &vOffset, const ANGLE3D &aOffset)
  {
    FLOAT3D vDiff = (vShootTarget - (GetPlacement().pl_PositionVector + vOffset*GetRotationMatrix())).SafeNormalize();
    
    // find orientation towards target
    FLOAT3D mToTargetX, mToTargetY, mToTargetZ;
    mToTargetZ = -vDiff;
    mToTargetY = -en_vGravityDir;
    mToTargetX = (mToTargetY*mToTargetZ).SafeNormalize();
    mToTargetY = (mToTargetZ*mToTargetX).SafeNormalize();
    FLOATmatrix3D mToTarget;
    mToTarget(1,1) = mToTargetX(1); mToTarget(1,2) = mToTargetY(1); mToTarget(1,3) = mToTargetZ(1);
    mToTarget(2,1) = mToTargetX(2); mToTarget(2,2) = mToTargetY(2); mToTarget(2,3) = mToTargetZ(2);
    mToTarget(3,1) = mToTargetX(3); mToTarget(3,2) = mToTargetY(3); mToTarget(3,3) = mToTargetZ(3);

    // calculate placement of projectile to be at given offset
    plProjectile.pl_PositionVector = GetPlacement().pl_PositionVector + vOffset*GetRotationMatrix();
    FLOATmatrix3D mDirection;
    MakeRotationMatrixFast(mDirection, aOffset);
    DecomposeRotationMatrixNoSnap(plProjectile.pl_OrientationAngle, mToTarget*mDirection);
  };

  // prepare free flying projectile
  void PrepareFreeFlyingProjectile(CPlacement3D &plProjectile, const FLOAT3D vShootTarget,
    const FLOAT3D &vOffset, const ANGLE3D &aOffset)
  {
    FLOAT3D vDiff = (vShootTarget - (GetPlacement().pl_PositionVector + vOffset*GetRotationMatrix())).SafeNormalize();
    
    // find orientation towards target
    FLOAT3D mToTargetX, mToTargetY, mToTargetZ;
    mToTargetZ = -vDiff;
    mToTargetY = -en_vGravityDir;
    mToTargetX = (mToTargetY*mToTargetZ).SafeNormalize();
    mToTargetZ = (mToTargetX*mToTargetY).SafeNormalize();
    FLOATmatrix3D mToTarget;
    mToTarget(1,1) = mToTargetX(1); mToTarget(1,2) = mToTargetY(1); mToTarget(1,3) = mToTargetZ(1);
    mToTarget(2,1) = mToTargetX(2); mToTarget(2,2) = mToTargetY(2); mToTarget(2,3) = mToTargetZ(2);
    mToTarget(3,1) = mToTargetX(3); mToTarget(3,2) = mToTargetY(3); mToTarget(3,3) = mToTargetZ(3);

    // calculate placement of projectile to be at given offset
    plProjectile.pl_PositionVector = GetPlacement().pl_PositionVector + vOffset*GetRotationMatrix();
    FLOATmatrix3D mDirection;
    MakeRotationMatrixFast(mDirection, aOffset);
    DecomposeRotationMatrixNoSnap(plProjectile.pl_OrientationAngle, mToTarget*mDirection);
  };

  // shoot projectile on enemy
  CEntity *ShootProjectile(enum ProjectileType pt, const FLOAT3D &vOffset, const ANGLE3D &aOffset) {
    ASSERT(m_penEnemy != NULL);

    // target enemy body
    EntityInfo *peiTarget = (EntityInfo*) (m_penEnemy->GetEntityInfo());
    FLOAT3D vShootTarget;
    GetEntityInfoPosition(m_penEnemy, peiTarget->vTargetCenter, vShootTarget);

    // launch
    CPlacement3D pl;
    PreparePropelledProjectile(pl, vShootTarget, vOffset, aOffset);
    CEntityPointer penProjectile = CreateEntity(pl, CLASS_PROJECTILE);
    ELaunchProjectile eLaunch;
    eLaunch.penLauncher = this;
    eLaunch.fStretch=1.0f;
    eLaunch.prtType = pt;
    penProjectile->Initialize(eLaunch);

    return penProjectile;
  };

  // shoot projectile at an exact spot
  CEntity *ShootProjectileAt(FLOAT3D vShootTarget, enum ProjectileType pt, const FLOAT3D &vOffset, const ANGLE3D &aOffset) {
  
    // launch
    CPlacement3D pl;
    PreparePropelledProjectile(pl, vShootTarget, vOffset, aOffset);
    CEntityPointer penProjectile = CreateEntity(pl, CLASS_PROJECTILE);
    ELaunchProjectile eLaunch;
    eLaunch.penLauncher = this;
    eLaunch.prtType = pt;
    penProjectile->Initialize(eLaunch);

    return penProjectile;
  };

  // shoot projectile on enemy
  CEntity *ShootPredictedProjectile(enum ProjectileType pt, const FLOAT3D vPredictedPos, const FLOAT3D &vOffset, const ANGLE3D &aOffset) {
    ASSERT(m_penEnemy != NULL);

    // target enemy body (predicted)
    EntityInfo *peiTarget = (EntityInfo*) (m_penEnemy->GetEntityInfo());
    FLOAT3D vShootTarget = vPredictedPos;
    if (peiTarget != NULL)
    {
      // get body center vector
      FLOAT3D vBody = FLOAT3D(peiTarget->vTargetCenter[0],peiTarget->vTargetCenter[1],peiTarget->vTargetCenter[2]);
      FLOATmatrix3D mRotation;
      MakeRotationMatrixFast(mRotation, m_penEnemy->GetPlacement().pl_OrientationAngle);
      vShootTarget = vPredictedPos + vBody*mRotation;
    }
    // launch
    CPlacement3D pl;
    PreparePropelledProjectile(pl, vShootTarget, vOffset, aOffset);
    CEntityPointer penProjectile = CreateEntity(pl, CLASS_PROJECTILE);
    ELaunchProjectile eLaunch;
    eLaunch.penLauncher = this;
    eLaunch.prtType = pt;
    penProjectile->Initialize(eLaunch);

    return penProjectile;
  };

  BOOL WouldNotLeaveAttackRadius(void)
  {
    if (m_fAttackRadius<=0) {
      return FALSE;
    }
    // test if we are inside radius
    BOOL bInsideNow = (m_vStartPosition-GetPlacement().pl_PositionVector).Length() < m_fAttackRadius;
    // test if going towards enemy leads us to center of attack radius circle
    BOOL bEnemyTowardsCenter = 
      (m_vStartPosition-m_penEnemy->GetPlacement().pl_PositionVector).Length() <
      (GetPlacement().pl_PositionVector-m_penEnemy->GetPlacement().pl_PositionVector).Length();
    return bInsideNow || bEnemyTowardsCenter;
  }

  // check whether may move while attacking
  virtual BOOL MayMoveToAttack(void) 
  {
    // check if enemy is diving
    CMovableEntity *pen = (CMovableEntity *) m_penEnemy.ep_pen;
    CContentType &ctUp = pen->en_pwoWorld->wo_actContentTypes[pen->en_iUpContent];
    BOOL bEnemyDiving = !(ctUp.ct_ulFlags&CTF_BREATHABLE_LUNGS);
    // may move if can go to enemy without leaving attack radius, and entity is not diving
    return  WouldNotLeaveAttackRadius() && !bEnemyDiving;
  };


/************************************************************
 *                 BLOW UP FUNCTIONS                        *
 ************************************************************/
  // should this enemy blow up (spawn debris)
  virtual BOOL ShouldBlowUp(void) 
  {
    // exotech larva boss allways blows up
    if (IsOfClass(this, "ExotechLarva")) { return TRUE; }
    
    // blow up if
    return
      // allowed 
      GetSP()->sp_bGibs && 
      // dead and
      GetHealth()<=0 && 
      // has received large enough damage lately and
      m_vDamage.Length() > m_fBlowUpAmount && 
      // not already disappearing
      m_fSpiritStartTime==0;
  }


  // base function for blowing up
  void BlowUpBase(void)
  {
    // call derived function
    BlowUp();
  }


  // spawn body parts
  virtual void BlowUp(void)
  {
    // blow up notify
    BlowUpNotify();
    const BOOL bGibs = GetSP()->sp_bGibs;

    FLOAT3D vNormalizedDamage = m_vDamage-m_vDamage*(m_fBlowUpAmount/m_vDamage.Length());
    vNormalizedDamage /= Sqrt(vNormalizedDamage.Length());
    vNormalizedDamage *= 0.75f;
    FLOAT3D vBodySpeed = en_vCurrentTranslationAbsolute-en_vGravityDir*(en_vGravityDir%en_vCurrentTranslationAbsolute);

    // if allowed and fleshy
    if( bGibs && !m_bRobotBlowup)
    {
      // readout blood type
      const INDEX iBloodType = GetSP()->sp_iBlood;
      // determine debris texture (color)
      ULONG ulFleshTexture = TEXTURE_FLESH_GREEN;
      ULONG ulFleshModel   = MODEL_FLESH;
      if( iBloodType==2) { ulFleshTexture = TEXTURE_FLESH_RED; }
      // spawn debris
      Debris_Begin(EIBT_FLESH, DPT_BLOODTRAIL, BET_BLOODSTAIN, m_fBlowUpSize, vNormalizedDamage, vBodySpeed, 1.0f, 0.0f);
      for( INDEX iDebris = 0; iDebris<m_fBodyParts; iDebris++) {
        // flowerpower mode?
        if( iBloodType==3) {
          switch( IRnd()%5) {
          case 1:  { ulFleshModel = MODEL_FLESH_APPLE;   ulFleshTexture = TEXTURE_FLESH_APPLE;   break; }
          case 2:  { ulFleshModel = MODEL_FLESH_BANANA;  ulFleshTexture = TEXTURE_FLESH_BANANA;  break; }
          case 3:  { ulFleshModel = MODEL_FLESH_BURGER;  ulFleshTexture = TEXTURE_FLESH_BURGER;  break; }
          case 4:  { ulFleshModel = MODEL_FLESH_LOLLY;   ulFleshTexture = TEXTURE_FLESH_LOLLY;   break; }
          default: { ulFleshModel = MODEL_FLESH_ORANGE;  ulFleshTexture = TEXTURE_FLESH_ORANGE;  break; }
          }
        }
        Debris_Spawn( this, this, ulFleshModel, ulFleshTexture, 0, 0, 0, IRnd()%4, 0.5f,
                      FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
      }
      // leave a stain beneath
      LeaveStain(FALSE);
    }

    // if allowed and robot/machine
    if( bGibs && m_bRobotBlowup)
    {
      // spawn debris
      Debris_Begin(EIBT_ROBOT, DPR_SMOKETRAIL, BET_EXPLOSIONSTAIN, m_fBlowUpSize, vNormalizedDamage, vBodySpeed, 1.0f, 0.0f);
      for( INDEX iDebris = 0; iDebris<m_fBodyParts; iDebris++) {
        Debris_Spawn( this, this, MODEL_MACHINE, TEXTURE_MACHINE, 0, 0, 0, IRnd()%4, 0.2f,
                      FLOAT3D(FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f, FRnd()*0.6f+0.2f));
      }
      // spawn explosion
      CPlacement3D plExplosion = GetPlacement();
      CEntityPointer penExplosion = CreateEntity(plExplosion, CLASS_BASIC_EFFECT);
      ESpawnEffect eSpawnEffect;
      eSpawnEffect.colMuliplier = C_WHITE|CT_OPAQUE;
      eSpawnEffect.betType = BET_BOMB;
      FLOAT fSize = m_fBlowUpSize*0.3f;
      eSpawnEffect.vStretch = FLOAT3D(fSize,fSize,fSize);
      penExplosion->Initialize(eSpawnEffect);
    }

    // hide yourself (must do this after spawning debris)
    SwitchToEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);
  }


/************************************************************
 *                CLASS SUPPORT FUNCTIONS                   *
 ************************************************************/

  // leave stain
  virtual void LeaveStain( BOOL bGrow)
  {
    ESpawnEffect ese;
    FLOAT3D vPoint;
    FLOATplane3D vPlaneNormal;
    FLOAT fDistanceToEdge;
    // get your size
    FLOATaabbox3D box;
    GetBoundingBox(box);
  
    // on plane
    if( GetNearestPolygon(vPoint, vPlaneNormal, fDistanceToEdge)) {
      // if near to polygon and away from last stain point
      if( (vPoint-GetPlacement().pl_PositionVector).Length()<0.5f
        && (m_vLastStain-vPoint).Length()>1.0f ) {
        m_vLastStain = vPoint;
        FLOAT fStretch = box.Size().Length();
        ese.colMuliplier = C_WHITE|CT_OPAQUE;
        // stain
        if (bGrow) {
          ese.betType    = BET_BLOODSTAINGROW;
          ese.vStretch   = FLOAT3D( fStretch*1.5f, fStretch*1.5f, 1.0f);
        } else {
          ese.betType    = BET_BLOODSTAIN;
          ese.vStretch   = FLOAT3D( fStretch*0.75f, fStretch*0.75f, 1.0f);
        }
        ese.vNormal    = FLOAT3D( vPlaneNormal);
        ese.vDirection = FLOAT3D( 0, 0, 0);
        FLOAT3D vPos = vPoint+ese.vNormal/50.0f*(FRnd()+0.5f);
        CEntityPointer penEffect = CreateEntity( CPlacement3D(vPos, ANGLE3D(0,0,0)), CLASS_BASIC_EFFECT);
        penEffect->Initialize(ese);
      }
    }
  };

  virtual void AdjustDifficulty(void)
  {
    FLOAT fMoveSpeed = GetSP()->sp_fEnemyMovementSpeed;
    FLOAT fAttackSpeed = GetSP()->sp_fEnemyMovementSpeed;
//    m_fWalkSpeed *= fMoveSpeed;
//    m_aWalkRotateSpeed *= fMoveSpeed;
    m_fAttackRunSpeed *= fMoveSpeed;
    m_aAttackRotateSpeed *= fMoveSpeed;
    m_fCloseRunSpeed *= fMoveSpeed;
    m_aCloseRotateSpeed *= fMoveSpeed;
    m_fAttackFireTime *= 1/fAttackSpeed;
    m_fCloseFireTime *= 1/fAttackSpeed;
/*
    CSessionProperties::GameDificulty gd = GetSP()->sp_gdGameDificulty;

    switch(gd) {
    case CSessionProperties::GD_EASY: {
                                      } break;
    case CSessionProperties::GD_NORMAL: {
                                      } break;
    case CSessionProperties::GD_HARD: {
                                      } break;
    case CSessionProperties::GD_SERIOUS: {
                                      } break;
    }
    */
  }


/************************************************************
 *                SOUND VIRTUAL FUNCTIONS                   *
 ************************************************************/

  // wounded -> yell
  void WoundedNotify(const EDamage &eDamage)
  {
    // if no enemy
    if (m_penEnemy==NULL) {
      // do nothing
      return;
    }

    // if not killed from short distance
    if (eDamage.dmtType!=DMT_CLOSERANGE && eDamage.dmtType!=DMT_CHAINSAW) {
      // yell
      ESound eSound;
      eSound.EsndtSound = SNDT_YELL;
      eSound.penTarget = m_penEnemy;
      SendEventInRange(eSound, FLOATaabbox3D(GetPlacement().pl_PositionVector, 25.0f));
    }
  };

  // see enemy -> shout
  void SeeNotify() 
  {
    // if no enemy
    if (m_penEnemy==NULL) {
      // do nothing
      return;
    }
    // yell
    ESound eSound;
    eSound.EsndtSound = SNDT_SHOUT;
    eSound.penTarget = m_penEnemy;
    SendEventInRange(eSound, FLOATaabbox3D(GetPlacement().pl_PositionVector, 50.0f));
  };



/************************************************************
 *          VIRTUAL FUNCTIONS THAT NEED OVERRIDE            *
 ************************************************************/
  virtual void StandingAnim(void) {};
  virtual void StandingAnimFight(void) { StandingAnim(); };
  virtual void WalkingAnim(void) {};
  virtual void RunningAnim(void) {};
  virtual void RotatingAnim(void) {};
  virtual void ChargeAnim(void) {};
  virtual INDEX AnimForDamage(FLOAT fDamage) { return 0; };
  virtual void BlowUpNotify(void) {};
  virtual INDEX AnimForDeath(void) { return 0; };
  virtual FLOAT WaitForDust(FLOAT3D &vStretch) { return -1; };
  virtual void DeathNotify(void) {};
  virtual void IdleSound(void) {};
  virtual void SightSound(void) {};
  virtual void WoundSound(void) {};
  virtual void DeathSound(void) {};
  virtual FLOAT GetLockRotationSpeed(void) { return 2000.0f;};


  // render particles
  void RenderParticles(void) {
    // no particles when not existing
    if (GetRenderType()!=CEntity::RT_MODEL && GetRenderType()!=CEntity::RT_SKAMODEL) {
      return;
    }
    // if is dead
    if( m_fSpiritStartTime != 0.0f)
    {
      // const FLOAT tmNow = _pTimer->CurrentTick();
      // Particles_ModelGlow(this, tmNow + 20,PT_STAR08, 0.15f, 2, 0.03f, 0xff00ff00);
      Particles_Death(this, m_fSpiritStartTime);
    }
  }

  // adjust sound and watcher parameters here if needed
  virtual void EnemyPostInit(void) {};

  /* Handle an event, return false if the event is not handled. */
  BOOL HandleEvent(const CEntityEvent &ee)
  {
    if (ee.ee_slEvent==EVENTCODE_ETouch)
    {
      if( GetCrushHealth() != 0.0f)
      {
        ETouch eTouch = ((ETouch &) ee);
        if (IsOfClass(eTouch.penOther, "ModelHolder2") ||
            IsOfClass(eTouch.penOther, "MovingBrush") ||
            IsOfClass(eTouch.penOther, "DestroyableArchitecture") )
        {
          InflictDirectDamage(eTouch.penOther, this, DMT_EXPLOSION, GetCrushHealth(),
            eTouch.penOther->GetPlacement().pl_PositionVector, -(FLOAT3D&)eTouch.plCollision);
        }
      }
    }
    return CMovableModelEntity::HandleEvent(ee);
  }
  
  // returns length of animation
  FLOAT GetAnimLength(int iAnim)
  {
    if(en_RenderType==RT_SKAMODEL) {
      return GetModelInstance()->GetAnimLength(iAnim);
    } else {
      return GetModelObject()->GetAnimLength(iAnim);
    }
  }

  // returns lenght of current anim length
  FLOAT GetCurrentAnimLength()
  {
    if(en_RenderType==RT_SKAMODEL) {
      return 0.5f;
    } else {
      return GetModelObject()->GetCurrentAnimLength();
    }
  }

  // is animation finished
  BOOL IsAnimFinished()
  {
    if(en_RenderType==RT_SKAMODEL) {
      return TRUE;
    } else {
      return GetModelObject()->IsAnimFinished();
    }
  }

  // 
  FLOAT GetPassedTime()
  {
    if(en_RenderType==RT_SKAMODEL) {
      return 0.0f;
    } else { 
      return GetModelObject()->GetPassedTime();
    }
  }

  FLOAT3D &GetModelStretch()
  {
    if(en_RenderType==RT_SKAMODEL) {
      return GetModelInstance()->mi_vStretch;
    } else {
      return GetModelObject()->mo_Stretch;
    }
  }

  // Stretch model
  void StretchModel(FLOAT3D vStretch)
  {
    if(en_RenderType==RT_SKAMODEL) {
      GetModelInstance()->StretchModel( vStretch);
    } else {
      GetModelObject()->StretchModel( vStretch);
    }
  }

  // Stretch single model
  void StretchSingleModel( FLOAT3D vStretch)
  {
    if(en_RenderType==RT_SKAMODEL) {
      GetModelInstance()->StretchSingleModel( vStretch);
    } else {
      GetModelObject()->StretchSingleModel( vStretch);
    }
  }


  // returns bytes of memory used by this object
  SLONG GetUsedMemory(void)
  {
    // initial
    SLONG slUsedMemory = sizeof(CEnemyBase) - sizeof(CMovableModelEntity) + CMovableModelEntity::GetUsedMemory();
    // add some more
    slUsedMemory += m_strDescription.Length();
    slUsedMemory += m_strName.Length();
    slUsedMemory += 1* sizeof(CSoundObject);
    return slUsedMemory;
  }



procedures:

//**********************************************************
//                 MOVEMENT PROCEDURES
//**********************************************************

  // move to given destination position
  MoveToDestination(EVoid) 
  {
    // setup parameters
    m_fMoveFrequency = 0.25f;
    m_fMoveTime = _pTimer->CurrentTick() + 45.0f;
    // while not there and time not expired
    while (CalcDistanceInPlaneToDestination()>m_fMoveSpeed*m_fMoveFrequency*2.0f &&
           m_fMoveTime>_pTimer->CurrentTick()) {
      // every once in a while
      wait (m_fMoveFrequency) {
        on (EBegin) : { 
          // adjust direction and speed
          ULONG ulFlags = SetDesiredMovement(); 
          MovementAnimation(ulFlags);
          resume;
        }
        on (ETimer) : { stop; }
      }
    }

    // return to the caller
    return EReturn();
  };

  // go to next randomly choosen position inside patrol range
  MoveToRandomPatrolPosition(EVoid) 
  {
    // if the marker is invalid
    if (!IsOfClass(m_penMarker, "Enemy Marker")) {
      // this should not happen
      ASSERT(FALSE);
      // return to caller
      return EReturn();
    }
    // get the marker
    CEnemyMarker *pem = (CEnemyMarker *) m_penMarker.ep_pen;

    // get random destination position 
    FLOAT fMin = pem->m_fPatrolAreaInner;
    FLOAT fMax = pem->m_fPatrolAreaOuter;
    if (fMin<0) {
      fMin = 0;
    }
    if (fMax<fMin) {
      fMax = fMin;
    }
    FLOAT fR = Lerp(fMin, fMax, FRnd());
    FLOAT fA = FRnd()*360.0f;
    FLOAT3D vOffsetDir;
    GetHeadingDirection(fA, vOffsetDir);
    m_vDesiredPosition = m_vStartPosition+vOffsetDir*fR;

    // use walking to get there
    m_fMoveSpeed = GetProp(m_fWalkSpeed);
    m_aRotateSpeed = GetProp(m_aWalkRotateSpeed);
    WalkingAnim();

    // go
    autocall MoveToDestination() EReturn;

    // return to caller
    return EReturn();
  };

  // patrol around start position
  DoPatrolling()
  {
    // repeat forever
    while(TRUE) {
      // stop where you are
      StopMoving();
      StandingAnim();
      // wait a bit
      autowait(0.5f + FRnd()/2);
      // patrol to a random position
      autocall MoveToRandomPatrolPosition() EReturn;
    }
  }

  // just wait until something happens
  BeIdle(EVoid)
  {
    // start watching
    GetWatcher()->SendEvent(EStart());

    // stand in place
    StandingAnim();

    // repeat forever
    while(TRUE) {
      // wait some time
      autowait(Lerp(5.0f, 20.0f, FRnd()));
      // play idle sound
      IdleSound();
    }
  }

  // return to start position
  ReturnToStartPosition(EVoid) 
  {
    jump BeIdle();
/*
    // start watching
    GetWatcher()->SendEvent(EStart());

    m_vDesiredPosition = m_vStartPosition;
    m_vStartDirection = (GetPlacement().pl_PositionVector-m_vStartPosition).SafeNormalize();
    m_fMoveSpeed = GetProp(m_fAttackRunSpeed);
    m_aRotateSpeed = GetProp(m_aAttackRotateSpeed);
    RunningAnim();
    autocall MoveToDestination() EReturn;

    WalkingAnim();
    m_vDesiredAngle = m_vStartDirection;
    StopTranslating();

    autocall RotateToStartDirection() EReturn;

    StopMoving();
    StandingAnim();

    jump BeIdle();
    */
  };
  /*
  // rotate to destination
  RotateToStartDirection(EVoid) 
  {

    m_fMoveFrequency = 0.1f;
    m_fMoveTime = _pTimer->CurrentTick() + 45.0f;
    while (Abs(GetRelativeHeading(GetDesiredPositionDir()))>GetProp(m_aRotateSpeed)*m_fMoveFrequency*1.5f &&
           m_fMoveTime>_pTimer->CurrentTick()) {
      autowait(m_fMoveFrequency);
    }

    return EReturn();
  };
  */


  // move through markers
  MoveThroughMarkers() 
  {
    // start watching
    GetWatcher()->SendEvent(EStart());
  
    // while there is a valid marker, take values from it
    while (m_penMarker!=NULL && IsOfClass(m_penMarker, "Enemy Marker")) {
      CEnemyMarker *pem = (CEnemyMarker *) m_penMarker.ep_pen;

      // the marker position is our new start position for attack range
      m_vStartPosition = m_penMarker->GetPlacement().pl_PositionVector;
      // make a random position to walk to at the marker
      FLOAT fR = FRnd()*pem->m_fMarkerRange;
      FLOAT fA = FRnd()*360.0f;
      m_vDesiredPosition = m_vStartPosition+FLOAT3D(CosFast(fA)*fR, 0, SinFast(fA)*fR);
      // if running 
      if (pem->m_betRunToMarker==BET_TRUE) {
        // use attack speeds
        m_fMoveSpeed = GetProp(m_fAttackRunSpeed);
        m_aRotateSpeed = GetProp(m_aAttackRotateSpeed);
        // start running anim
        RunningAnim();
      // if not running
      } else {
        // use walk speeds
        m_fMoveSpeed = GetProp(m_fWalkSpeed);
        m_aRotateSpeed = GetProp(m_aWalkRotateSpeed);
        // start walking anim
        WalkingAnim();
      }

      // move to the new destination position
      autocall MoveToDestination() EReturn;

      // read new blind/deaf values
      CEnemyMarker *pem = (CEnemyMarker *) m_penMarker.ep_pen;
      SetBoolFromBoolEType(m_bBlind, pem->m_betBlind);
      SetBoolFromBoolEType(m_bDeaf,  pem->m_betDeaf);

      // if should start tactics
      if (pem->m_bStartTactics){
        // start to see/hear
        m_bBlind = FALSE;
        m_bDeaf = FALSE;
        // unconditional tactics start
        StartTacticsNow();
      }
      
      // if should patrol there
      if (pem->m_fPatrolTime>0.0f) {
        // spawn a reminder to notify us when the time is up
        SpawnReminder(this, pem->m_fPatrolTime, 0);
        // wait
        wait() {
          // initially
          on (EBegin) : { 
            // start patroling
            call DoPatrolling(); 
          }
          // if time is up
          on (EReminder) : {
            // stop patroling
            stop;
          }
        }
      }

      CEnemyMarker *pem = (CEnemyMarker *) m_penMarker.ep_pen;
      // if should wait on the marker
      if (pem->m_fWaitTime > 0.0f) {
        // stop there
        StopMoving();
        StandingAnim();
        // wait
        autowait(pem->m_fWaitTime);
      }

      // wait a bit always (to prevent eventual busy-looping)
      autowait(0.05f);

      // take next marker in loop
      m_penMarker = ((CEnemyMarker&)*m_penMarker).m_penTarget;
    } // when no more markers

    // stop where you are
    StopMoving();
    StandingAnim();

    // return to called
    return EReturn();
  };


//**********************************************************
//                 ATTACK PROCEDURES
//**********************************************************

  // sequence that is activated when a new player is spotted visually or heard
  NewEnemySpotted()
  {
    // calculate reflex time
    FLOAT tmReflex = Lerp(m_tmReflexMin, m_tmReflexMax, FRnd());
    tmReflex = ClampDn(tmReflex, 0.0f);

    // if should wait
    if (tmReflex>=_pTimer->TickQuantum) {
      // stop where you are
      StopMoving();
      StandingAnim();

      // wait the reflex time
      wait(tmReflex) {
        on (ETimer) : { stop; }
        // pass all damage events
        on (EDamage) : { pass; }
        // pass space beam hit
        on (EHitBySpaceShipBeam) : { pass;}
        // ignore all other events
        otherwise () : { resume; }
      }
    }

    // play sight sound
    SightSound();

    // return to caller
    return EReturn();
  }

  // stop attack
  StopAttack(EVoid) {
    // start watching
    GetWatcher()->SendEvent(EStart());
    // no damager
    SetTargetNone();
    m_fDamageConfused = 0.0f;
    // stop moving
    StopMoving();

    return EReturn();
  };

  // initial preparation
  InitializeAttack(EVoid) 
  {
    // disable blind/deaf mode
    m_bBlind = FALSE;
    m_bDeaf = FALSE;

    SeeNotify();          // notify seeing
    GetWatcher()->SendEvent(EStop());   // stop looking
    // make fuss
    AddToFuss();
    // remember spotted position
    m_vPlayerSpotted = PlayerDestinationPos();

    // set timers
    if (CalcDist(m_penEnemy)<GetProp(m_fCloseDistance)) {
      m_fShootTime = 0.0f;
    } else {
      m_fShootTime = _pTimer->CurrentTick() + FRnd();
    }
    m_fDamageConfused = m_fDamageWounded;

    return EReturn();
  };


  // attack enemy
  AttackEnemy(EVoid) {
    // initial preparation
    autocall InitializeAttack() EReturn;

    // while you have some enemy
    while (m_penEnemy != NULL) {
      // attack it
      autocall PerformAttack() EReturn;
    }

    // stop attack
    autocall StopAttack() EReturn;

    // return to Active() procedure
    return EBegin();
  };

  // repeat attacking enemy until enemy is dead or you give up
  PerformAttack()
  {
    // reset last range
    m_fRangeLast = 1E9f;

    // set player's position as destination position
    m_vDesiredPosition = PlayerDestinationPos();
    m_dtDestination = DT_PLAYERCURRENT;

    // repeat
    while(TRUE)
    {
      // if attacking is futile
      if (ShouldCeaseAttack()) {
        // cease it
        SetTargetNone();
        return EReturn();
      }

      // get distance from enemy
      FLOAT fEnemyDistance = CalcDist(m_penEnemy);
      // if just entered close range
      if (m_fRangeLast>GetProp(m_fCloseDistance) && fEnemyDistance<=GetProp(m_fCloseDistance)) {
        // reset shoot time to make it attack immediately
        m_fShootTime = 0.0f;
      }
      m_fRangeLast = fEnemyDistance;

      // determine movement frequency depending on enemy distance or path finding
      m_fMoveFrequency = GetAttackMoveFrequency(fEnemyDistance);
      if (m_dtDestination==DT_PATHPERSISTENT||m_dtDestination==DT_PATHTEMPORARY) {
        m_fMoveFrequency = 0.1f;
      }

      // wait one time interval
      wait(m_fMoveFrequency) {
        on (ETimer) : { stop; }
        // initially
        on (EBegin) : {

          // if you haven't fired/hit enemy for some time
          if (_pTimer->CurrentTick() > m_fShootTime) {

            // if you have new player visible closer than current and in threat distance
            CEntity *penNewEnemy = GetWatcher()->CheckCloserPlayer(m_penEnemy, GetThreatDistance());
            if (penNewEnemy!=NULL) {
              // switch to that player
              SetTargetHardForce(penNewEnemy);
              // start new behavior
              SendEvent(EReconsiderBehavior());
              stop;
            }

            // if you can see player
            if (IsVisible(m_penEnemy)) {
              // remember spotted position
              m_vPlayerSpotted = PlayerDestinationPos();
              // if going to player spotted or temporary path position
              if (m_dtDestination==DT_PLAYERSPOTTED||m_dtDestination==DT_PATHTEMPORARY) {
                // switch to player current position
                m_dtDestination=DT_PLAYERCURRENT;
              }

            // if you cannot see player
            } else {
              // if going to player's current position
              if (m_dtDestination==DT_PLAYERCURRENT) {
                // switch to position where player was last seen
                m_dtDestination=DT_PLAYERSPOTTED;
              }
            }

            // try firing/hitting again now
            call FireOrHit();

          // if between two fire/hit moments
          } else {
            // if going to player spotted or temporary path position and you just seen the player
            if ((m_dtDestination==DT_PLAYERSPOTTED||m_dtDestination==DT_PATHTEMPORARY)
              && IsVisible(m_penEnemy)) {
              // switch to player current position
              m_dtDestination=DT_PLAYERCURRENT;
              // remember spotted position
              m_vPlayerSpotted = PlayerDestinationPos();
            }
          }

          // if you are not following the player and you are near current destination position
          FLOAT fAllowedError = m_fMoveSpeed*m_fMoveFrequency*2.0f;
          if (m_dtDestination==DT_PATHPERSISTENT||m_dtDestination==DT_PATHTEMPORARY) {
            fAllowedError = ((CNavigationMarker&)*m_penPathMarker).m_fMarkerRange;
          }
          if (m_dtDestination!=DT_PLAYERCURRENT &&
            (CalcDistanceInPlaneToDestination()<fAllowedError || fAllowedError<0.1f)) {
            // if going to where player was last spotted
            if (m_dtDestination==DT_PLAYERSPOTTED) {
              // if you see the player
              if (IsVisible(m_penEnemy)) {
                // switch to following player
                m_dtDestination=DT_PLAYERCURRENT;
              // if you don't see him
              } else {
                // switch to temporary path finding
                m_dtDestination=DT_PATHTEMPORARY;
                StartPathFinding();
              }
            // if using pathfinding
            } else if (m_dtDestination==DT_PATHTEMPORARY||m_dtDestination==DT_PATHPERSISTENT) {
              // find next path marker
              FindNextPathMarker();
            }
          }

          // if following player
          if (m_dtDestination==DT_PLAYERCURRENT) {
            // set player's position as destination position
            m_vDesiredPosition = PlayerDestinationPos();

          // if going to where player was last seen
          } else if (m_dtDestination==DT_PLAYERSPOTTED) {
            // use that as destination position
            m_vDesiredPosition = m_vPlayerSpotted;
          }

          // set speeds for movement towards desired position
          FLOAT3D vPosDelta = m_vDesiredPosition-GetPlacement().pl_PositionVector;
          FLOAT fPosDistance = vPosDelta.Length();
          
          SetSpeedsToDesiredPosition(vPosDelta, fPosDistance, m_dtDestination==DT_PLAYERCURRENT);

          // adjust direction and speed
          ULONG ulFlags = SetDesiredMovement(); 
          MovementAnimation(ulFlags);
          resume;
        }
        // if touched something
        on (ETouch eTouch) : { 
          if( IfTargetCrushed(eTouch.penOther, (FLOAT3D&)eTouch.plCollision))
          {
            resume;
          }
          // if pathfinding must begin
          else if (CheckTouchForPathFinding(eTouch)) {
            // stop the loop
            stop;
          // if touch is ignored
          } else if (m_bTacticActive) {
            // reinitialize tactics?
            if (eTouch.penOther->GetRenderType()==CEntity::RT_BRUSH) {
              FLOAT3D vDir = en_vDesiredTranslationRelative;
              vDir.SafeNormalize();
              vDir*=GetRotationMatrix();
              // if the touched plane is more or less orthogonal to the current velocity
              if ((eTouch.plCollision%vDir)<-0.5f) { m_bTacticActive = 0; }
              resume;
            } else {
              resume;
            }
          } else {
            // pass the event
            pass;
          }
        }
        // if came to an edge
        on (EWouldFall eWouldFall) : { 
          // if pathfinding must begin
          if (CheckFallForPathFinding(eWouldFall)) {
            // stop the loop
            stop;
          } else if (m_bTacticActive) {
            // stop tactics
            m_bTacticActive = 0;
            resume;
          // if edge is ignored
          } else {
            // pass the event
            pass;
          }
        }
        on (ESound) : { resume; }     // ignore all sounds
        on (EWatch) : { resume; }     // ignore watch
        on (EReturn) : { stop; }  // returned from subprocedure
      }
    }
  }


  // fire or hit the enemy if possible
  FireOrHit() 
  {
    // if player is in close range and in front
    if (CalcDist(m_penEnemy)<GetProp(m_fCloseDistance) && CanHitEnemy(m_penEnemy, Cos(AngleDeg(45.0f)))) {
      // make fuss
      AddToFuss();
      // stop moving (rotation and translation)
      StopMoving();
      // set next shoot time
      m_fShootTime = _pTimer->CurrentTick() + GetProp(m_fCloseFireTime)*(1.0f + FRnd()/3.0f);
      // hit
      autocall Hit() EReturn;

    // else if player is in fire range and in front
    } else if (CalcDist(m_penEnemy)<GetProp(m_fAttackDistance) && CanAttackEnemy(m_penEnemy, Cos(AngleDeg(45.0f)))) {
      // make fuss
      AddToFuss();
      // stop moving (rotation and translation)
      StopMoving();
      // set next shoot time
      if (CalcDist(m_penEnemy)<GetProp(m_fCloseDistance)) {
        m_fShootTime = _pTimer->CurrentTick() + GetProp(m_fCloseFireTime)*(1.0f + FRnd()/3.0f);
      } else {
        m_fShootTime = _pTimer->CurrentTick() + GetProp(m_fAttackFireTime)*(1.0f + FRnd()/3.0f);
      }
      // fire
      autocall Fire() EReturn;

    // if cannot fire nor hit
    } else {
      // make sure we don't retry again too soon
      m_fShootTime = _pTimer->CurrentTick() + 0.25f;
    }

    // return to caller
    return EReturn();
  };

//**********************************************************
//                 COMBAT IMPLEMENTATION
//**********************************************************

  // this is called to hit the player when near
  Hit(EVoid) 
  { 
    return EReturn(); 
  }

  // this is called to shoot at player when far away or otherwise unreachable
  Fire(EVoid) 
  { 
    return EReturn(); 
  }

//**********************************************************
//                 COMBAT HELPERS
//**********************************************************

  // call this to lock on player for some time - set m_fLockOnEnemyTime before calling
  LockOnEnemy(EVoid) 
  {
    // stop moving
    StopMoving();
    // play animation for locking
    ChargeAnim();
    // wait charge time
    m_fLockStartTime = _pTimer->CurrentTick();
    while (m_fLockStartTime+GetProp(m_fLockOnEnemyTime) > _pTimer->CurrentTick()) {
      // each tick
      m_fMoveFrequency = 0.05f;
      wait (m_fMoveFrequency) {
        on (ETimer) : { stop; }
        on (EBegin) : {
          m_vDesiredPosition = PlayerDestinationPos();
          // if not heading towards enemy
          if (!IsInPlaneFrustum(m_penEnemy, CosFast(5.0f))) {
            m_fMoveSpeed = 0.0f;
            m_aRotateSpeed = GetLockRotationSpeed();
          // if heading towards enemy
          } else {
            m_fMoveSpeed = 0.0f;
            m_aRotateSpeed = 0.0f;
          }
          // adjust direction and speed
          ULONG ulFlags = SetDesiredMovement(); 
          //MovementAnimation(ulFlags);  don't do this, or they start to jive
          resume;
        }
      }
    }
    // stop rotating
    StopRotating();

    // return to caller
    return EReturn();
  };

  // call this to jump onto player - set charge properties before calling and spawn a reminder
  ChargeHitEnemy(EVoid) 
  {
    // wait for length of hit animation
    wait(GetAnimLength(m_iChargeHitAnimation)) {
      on (EBegin) : { resume; }
      on (ETimer) : { stop; }
      // ignore damages
      on (EDamage) : { resume; }
      // if user-set reminder expired
      on (EReminder) : {
        // stop moving
        StopMoving();
        resume;
      }
      // if you touch some entity
      on (ETouch etouch) : {
        // if it is alive and in front
        if ((etouch.penOther->GetFlags()&ENF_ALIVE) && IsInPlaneFrustum(etouch.penOther, CosFast(60.0f))) {
          // get your direction
          FLOAT3D vSpeed;
          GetHeadingDirection(m_fChargeHitAngle, vSpeed);
          // damage entity in that direction
          InflictDirectDamage(etouch.penOther, this, DMT_CLOSERANGE, m_fChargeHitDamage, FLOAT3D(0, 0, 0), vSpeed);
          // push it away
          vSpeed = vSpeed * m_fChargeHitSpeed;
          KickEntity(etouch.penOther, vSpeed);
          // stop waiting
          stop;
        }
        pass;
      }
    }
    // if the anim is not yet finished
    if (!IsAnimFinished()) {
      // wait the rest of time till the anim end
      wait(GetCurrentAnimLength() - GetPassedTime()) {
        on (EBegin) : { resume; }
        on (ETimer) : { stop; }
        // if timer expired
        on (EReminder) : {
          // stop moving
          StopMoving();
          resume;
        }
      }
    }

    // return to caller
    return EReturn();
  };

//**********************************************************
//             WOUNDING AND DYING PROCEDURES
//**********************************************************

  // play wounding animation
  BeWounded(EDamage eDamage)
  { 
    StopMoving();
    // determine damage anim and play the wounding
    autowait(GetAnimLength(AnimForDamage(eDamage.fAmount)));
    return EReturn();
  };

  // we get here once the enemy is dead
  Die(EDeath eDeath)
  {
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
      // send computer message if in coop
      if (GetSP()->sp_bCooperative) {
        EComputerMessage eMsg;
        eMsg.fnmMessage = GetComputerMessageName();
        if (eMsg.fnmMessage!="") {
          penKiller->SendEvent(eMsg);
        }
      }
    }


    // destroy watcher class
    GetWatcher()->SendEvent(EStop());
    GetWatcher()->SendEvent(EEnd());

    // send event to death target
    SendToTarget(m_penDeathTarget, m_eetDeathType, penKiller);
    // send event to spawner if any
    // NOTE: trigger's penCaused has been changed from penKiller to THIS;
    if (m_penSpawnerTarget) {
      SendToTarget(m_penSpawnerTarget, EET_TRIGGER, this);
    }
    

    // wait
    wait() {
      // initially
      on (EBegin) : {
        // if should already blow up
        if (ShouldBlowUp()) {
          // blow up now
          BlowUpBase();
          // stop waiting
          stop;
        // if shouldn't blow up yet
        } else {
          // invoke death animation sequence
          call DeathSequence();
        }
      }
      // if damaged
      on (EDamage) : {
        // if should already blow up
        if (ShouldBlowUp()) {
          // blow up now
          BlowUpBase();
          // stop waiting
          stop;
        }
        // otherwise, ignore the damage
        resume;
      }
      // if death sequence is over
      on (EEnd) : { 
        // stop waiting
        stop; 
      }
    }

    // stop making fuss
    RemoveFromFuss();
    // cease to exist
    Destroy();

    // all is over now, entity will be deleted
    return;
  };

  Death(EVoid) 
  {
    StopMoving();     // stop moving
    DeathSound();     // death sound
    LeaveStain(FALSE);

    // set physic flags
    SetPhysicsFlags(EPF_MODEL_CORPSE);
    SetCollisionFlags(ECF_CORPSE);
    SetFlags(GetFlags() | ENF_SEETHROUGH);

    // stop making fuss
    RemoveFromFuss();

    // death notify (usually change collision box and change body density)
    DeathNotify();

    // start death anim
    INDEX iAnim = AnimForDeath();
    // use tactic variables for temporary data
    m_vTacticsStartPosition=FLOAT3D(1,1,1);
    m_fTacticVar4=WaitForDust(m_vTacticsStartPosition);
    // remember start time
    m_fTacticVar5=_pTimer->CurrentTick();
    // mark that we didn't spawned dust yet
    m_fTacticVar3=-1;
    // if no dust should be spawned
    if( m_fTacticVar4<0)
    {
      autowait(GetAnimLength(iAnim));
    }
    // should spawn dust
    else if( TRUE)
    {
      while(_pTimer->CurrentTick()<m_fTacticVar5+GetCurrentAnimLength())
      {
        autowait(_pTimer->TickQuantum);
        if(en_penReference!=NULL && _pTimer->CurrentTick()>=m_fTacticVar5+m_fTacticVar4 && m_fTacticVar3<0)
        {
          // spawn dust effect
          CPlacement3D plFX=GetPlacement();
          ESpawnEffect ese;
          ese.colMuliplier = C_WHITE|CT_OPAQUE;
          ese.vStretch = m_vTacticsStartPosition;
          ese.vNormal = FLOAT3D(0,1,0);
          ese.betType = BET_DUST_FALL;
          CPlacement3D plSmoke=plFX;
          plSmoke.pl_PositionVector+=FLOAT3D(0,0.35f*m_vTacticsStartPosition(2),0);
          CEntityPointer penFX = CreateEntity(plSmoke, CLASS_BASIC_EFFECT);
          penFX->Initialize(ese);
          penFX->SetParent(this);
          // mark that we spawned dust
          m_fTacticVar3=1;
        }
      }
    }

    return EEnd();
  };

  DeathSequence(EVoid)
  {
    // entity death
    autocall Death() EEnd;

    // start bloody stain growing out from beneath the corpse
    LeaveStain(TRUE);

    // check if you have attached flame
    CEntityPointer penFlame = GetChildOfClass("Flame");
    if (penFlame!=NULL)
    {
      // send the event to stop burning
      EStopFlaming esf;
      esf.m_bNow=FALSE;
      penFlame->SendEvent(esf);
    }

    autowait(2.0f);

    // start fading out and turning into stardust effect
    m_fSpiritStartTime = _pTimer->CurrentTick();
    m_fFadeStartTime = _pTimer->CurrentTick();
    m_fFadeTime = 1.0f,
    m_bFadeOut = TRUE;
    // become passable even if very large corpse
    SetCollisionFlags(ECF_CORPSE&~((ECBI_PROJECTILE_MAGIC|ECBI_PROJECTILE_SOLID)<<ECB_TEST));
    // wait for fading
    autowait(m_fFadeTime);
    // wait for the stardust effect
    autowait(6.0f);

    return EEnd();
  }
//**********************************************************
//                MAIN LOOP PROCEDURES
//**********************************************************

  // move
  Active(EVoid) 
  {
    m_fDamageConfused = 0.0f;
    // logic loop
    wait () {
      // initially
      on (EBegin) : {
        // start new behavior
        SendEvent(EReconsiderBehavior());
        resume;
      }
      // if new behavior is requested
      on (EReconsiderBehavior) : {
        // if we have an enemy
        if (m_penEnemy!=NULL) {
          // attack it
          call AttackEnemy();
        // if we have a marker to walk to
        } else if (m_penMarker != NULL) {
          // go to the marker
          call MoveThroughMarkers();
        // if on start position
        } else if (m_bOnStartPosition) {
          // just wait here
          m_bOnStartPosition = FALSE;
          call BeIdle();
        // otherwise
        } else {
          // return to start position
          call ReturnToStartPosition();
        }
        resume;
      }
      // on return from some of the sub-procedures
      on (EReturn) : {
        // start new behavior
        SendEvent(EReconsiderBehavior());
        resume;
      }
      // if attack restart is requested
      on (ERestartAttack) : {
        // start new behavior
        SendEvent(EReconsiderBehavior());
        resume;
      }
      // if enemy has been seen
      on (EWatch eWatch) : {
        // if new enemy
        if (SetTargetSoft(eWatch.penSeen)) {
          // if blind till now and start tactics on sense
          if (m_bBlind && m_bTacticsStartOnSense ) {
            StartTacticsNow();
          }
          // react to it
          call NewEnemySpotted();
        }
        resume;
      }
      // if you get damaged by someone
      on (EDamage eDamage) : {
        // eventually set new hard target
        SetTargetHard(eDamage.penInflictor);

        // if confused
        m_fDamageConfused -= eDamage.fAmount;
        if (m_fDamageConfused < 0.001f) {
          m_fDamageConfused = m_fDamageWounded;
          // notify wounding to others
          WoundedNotify(eDamage);
          // make pain sound
          WoundSound();
          // play wounding animation
          call BeWounded(eDamage);
        }
        resume;
      }
      on (EForceWound) :
      {
        call BeWounded(EDamage());
        resume;
      }
      // if you hear something
      on (ESound eSound) : {
        // if deaf
        if (m_bDeaf) {
          // ignore the sound
          resume;
        }

        // if the target is visible and can be set as new enemy
        if (IsVisible(eSound.penTarget) && SetTargetSoft(eSound.penTarget)) {
          // react to it
          call NewEnemySpotted();
        }
        resume;
      }
      // on touch
      on (ETouch eTouch) : {
        // set the new target if needed
        BOOL bTargetChanged = SetTargetHard(eTouch.penOther);
        // if target changed
        if (bTargetChanged) {
          // make sound that you spotted the player
          SightSound();
          // start new behavior
          SendEvent(EReconsiderBehavior());
        }
        pass;
      }
      // if triggered manually
      on (ETrigger eTrigger) : {
        CEntity *penCaused = FixupCausedToPlayer(this, eTrigger.penCaused);
        // if can set the trigerer as soft target
        if (SetTargetSoft(penCaused)) {
          // make sound that you spotted the player
          SightSound();
          // start new behavior
          SendEvent(EReconsiderBehavior());
        }
        resume;
      }
      // on stop -> stop enemy
      on (EStop) : {
        jump Inactive();
      }

      // warn for all obsolete events
      on (EStartAttack) : {
        //CPrintF("%s: StartAttack event is obsolete!\n", GetName());
        resume;
      }
      on (EStopAttack) : {
        //CPrintF("%s: StopAttack event is obsolete!\n", GetName());
        resume;
      }
    }
  };

  // not doing anything, waiting until some player comes close enough to start patroling or similar
  Inactive(EVoid) 
  {
    // stop moving
    StopMoving();                 
    StandingAnim();
    // start watching
    GetWatcher()->SendEvent(EStart());
    // wait forever
    wait() {
      on (EBegin) : { resume; }
      // if watcher detects that a player is near
      on (EStart) : { 
        // become active (patroling etc.)
        jump Active(); 
      }
      // if returned from wounding
      on (EReturn) : { 
        // become active (this will attack the enemy)
        jump Active(); 
      }
      // if triggered manually
      on (ETrigger eTrigger) : {
        CEntity *penCaused = FixupCausedToPlayer(this, eTrigger.penCaused);
        // if can set the trigerer as soft target
        if (SetTargetSoft(penCaused)) {
          // become active (this will attack the player)
          jump Active(); 
        }
      }
      // if you get damaged by someone
      on (EDamage eDamage) : {
        // if can set the damager as hard target
        if (SetTargetHard(eDamage.penInflictor)) {
          // notify wounding to others
          WoundedNotify(eDamage);
          // make pain sound
          WoundSound();
          // play wounding animation
          call BeWounded(eDamage);
        }
        return;
      }
    }
  };

  // overridable called before main enemy loop actually begins
  PreMainLoop(EVoid)
  {
    return EReturn();
  }

  // main entry point for enemy behavior
  MainLoop(EVoid) 
  {
    // setup some model parameters that are global for all enemies
    SizeModel();
    // check that max health is properly set
    ASSERT(m_fMaxHealth==GetHealth() || IsOfClass(this, "Devil") || IsOfClass(this, "ExotechLarva") || IsOfClass(this, "AirElemental") || IsOfClass(this, "Summoner"));

    // normalize parameters
    if (m_tmReflexMin<0) {
      m_tmReflexMin = 0.0f;
    }
    if (m_tmReflexMin>m_tmReflexMax) {
      m_tmReflexMax = m_tmReflexMin;
    }

    // adjust falldown and step up values
    if (m_fStepHeight==-1) {
      m_fStepHeight = 2.0f;
    }

    // if this is a template
    if (m_bTemplate) {
      // do nothing at all
      return;
    }

    // wait for just one tick
    // NOTES:
    // if spawned, we have to wait a bit after spawning for spawner to teleport us into proper place
    // if placed directly, we have to wait for game to start (not to start behaving while in editor)
    // IMPORTANT: 
    // this wait amount has to be lower than the one in music holder, so that the enemies are initialized before
    // they get counted
    autowait(_pTimer->TickQuantum);

    // spawn your watcher
    m_penWatcher = CreateEntity(GetPlacement(), CLASS_WATCHER);
    EWatcherInit eInitWatcher;
    eInitWatcher.penOwner = this;
    GetWatcher()->Initialize(eInitWatcher);

    // switch to next marker (enemies usually point to the marker they stand on)
    if (m_penMarker!=NULL && IsOfClass(m_penMarker, "Enemy Marker")) {
      CEnemyMarker *pem = (CEnemyMarker *) m_penMarker.ep_pen;
      m_penMarker = pem->m_penTarget;
    }


    // store starting position
    m_vStartPosition = GetPlacement().pl_PositionVector;

    // set sound default parameters
    m_soSound.Set3DParameters(80.0f, 5.0f, 1.0f, 1.0f);

    // adjust falldown and step up values
    en_fStepUpHeight = m_fStepHeight+0.01f;
    en_fStepDnHeight = m_fFallHeight+0.01f;

    // let derived class(es) adjust parameters if needed
    EnemyPostInit();

    // adjust your difficulty
    AdjustDifficulty();

    // check enemy params
    ASSERT(m_fStopDistance>=0);
    ASSERT(m_fCloseDistance>=0);
    ASSERT(m_fAttackDistance>m_fCloseDistance);
    ASSERT(m_fIgnoreRange>m_fAttackDistance);

    SetPredictable(TRUE);

    autocall PreMainLoop() EReturn;

    jump StandardBehavior();
  }

  StandardBehavior(EVoid)
  {
    // this is the main enemy loop
    wait() {
      // initially
      on (EBegin) : {
        // start in active or inactive state
        if (m_penEnemy!=NULL) {
          call Active();
        } else {
          call Inactive();
        }
      };
      // if dead
      on (EDeath eDeath) : {
        // die
        jump Die(eDeath);
      }
      // if an entity exits a teleport nearby
      on (ETeleport et) : {
        // proceed message to watcher (so watcher can quickly recheck for players)
        GetWatcher()->SendEvent(et);
        resume;
      }
      // if should stop being blind
      on (EStopBlindness) : {
        // stop being blind
        m_bBlind = FALSE;
        resume;
      }
      // if should stop being deaf
      on (EStopDeafness) : {
        // stop being deaf
        m_bDeaf = FALSE;
        resume;
      }
      // support for jumping using bouncers
      on (ETouch eTouch) : {
        IfTargetCrushed(eTouch.penOther, (FLOAT3D&)eTouch.plCollision);
        if (IsOfClass(eTouch.penOther, "Bouncer")) {
          JumpFromBouncer(this, eTouch.penOther);
        }
        resume;
      }
    }
  };

  // dummy main - never called
  Main(EVoid) {
    return;
  };
};
