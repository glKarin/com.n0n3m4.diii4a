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
#include "ModelsMP/Enemies/AirElemental/AirElemental.h"
#include "ModelsMP/Enemies/AirElemental/Elemental.h"
#include "Models/Enemies/Elementals/Twister.h"
%}

uses "EntitiesMP/EnemyBase";
uses "EntitiesMP/Twister";
//uses "EntitiesMP/AirShockwave";

event EElementalGrow {
};

%{
#define ECF_AIR ( \
  ((ECBI_BRUSH|ECBI_MODEL|ECBI_CORPSE|ECBI_ITEM|ECBI_PROJECTILE_MAGIC|ECBI_PROJECTILE_SOLID)<<ECB_TEST) |\
  ((ECBI_MODEL|ECBI_CORPSE|ECBI_ITEM|ECBI_PROJECTILE_MAGIC|ECBI_PROJECTILE_SOLID)<<ECB_PASS) |\
  ((ECBI_MODEL)<<ECB_IS))

#define AIRBOSS_EYES_HEIGHT 1.7f
#define AIRBOSS_BODY_HEIGHT 1.0f

// info structure
static EntityInfo eiAirElemental = {
  EIBT_AIR, 1500.0f,
  0.0f, AIRBOSS_EYES_HEIGHT, 0.0f,
  0.0f, AIRBOSS_BODY_HEIGHT, 0.0f,
};

#define RAND_05 (FLOAT(rand())/(float)(RAND_MAX)-0.5f)
#define FIREPOS_TWISTER FLOAT3D(-0.04f, 0.91f, -1.06f)
#define SIZE_NORMAL 1
#define SIZE_BIG01  2
#define SIZE_BIG02  3
#define SIZE_BIG03  4

#define AIRBOSS_MAX_TA 10
#define AIRBOSS_MAX_GA 3

FLOAT afTriggerArray[AIRBOSS_MAX_TA] = { 0.9f, 0.8f, 0.7f, 0.6f, 0.5f,
                                         0.4f, 0.3f, 0.2f, 0.1f, 0.05f };
FLOAT afGrowArray[AIRBOSS_MAX_GA][2] = { 0.8f, 25.0f,
                                         0.6f, 50.0f,
                                         0.4f, 100.0f };
%}

class CAirElemental : CEnemyBase {
name      "AirElemental";
thumbnail "Thumbnails\\AirElemental.tbn";

properties:
  
  2 BOOL m_bFloat = FALSE,
  3 FLOAT m_fAttPosY = 0.0f,

 10 BOOL m_bInitialAnim = FALSE,
  
 20 CEntityPointer m_penTrigger01 "AirBoss 90% Trigger" ,
 21 CEntityPointer m_penTrigger02 "AirBoss 80% Trigger" ,
 22 CEntityPointer m_penTrigger03 "AirBoss 70% Trigger" ,
 23 CEntityPointer m_penTrigger04 "AirBoss 60% Trigger" ,
 24 CEntityPointer m_penTrigger05 "AirBoss 50% Trigger" ,
 25 CEntityPointer m_penTrigger06 "AirBoss 40% Trigger" ,
 26 CEntityPointer m_penTrigger07 "AirBoss 30% Trigger" ,
 27 CEntityPointer m_penTrigger08 "AirBoss 20% Trigger" ,
 28 CEntityPointer m_penTrigger09 "AirBoss 10% Trigger" ,
 29 CEntityPointer m_penTrigger10 "AirBoss 05% Trigger" ,

 30 FLOAT m_fAttSizeCurrent = 0.0f,
 31 FLOAT m_fAttSizeBegin   = 12.5f, //(25)
 32 FLOAT m_fAttSizeEnd     = 100.0f, //(200)
 33 FLOAT m_fAttSizeRequested = 0.0f,
 34 BOOL  m_bAttGrow = FALSE,
 35 INDEX m_iSize = 0,        // index of size in afGrowArray array
 36 FLOAT m_fLastSize = 0.0f, 
 37 FLOAT m_fTargetSize = 0.0f,
 47 FLOAT m_fGrowSpeed "AirBoss Grow Speed" = 2.0f, // m/sec

// 40 FLOAT m_tmLastShockwave = 0.0f,
// 41 FLOAT m_fShockwaveTreshold "AirBoss Shockwave Treshold" = 50.0f,   // fire shockwave when someone closer then this
// 42 FLOAT m_fShockwavePeriod "AirBoss Shockwave Period" = 3.0f,
  
 43 FLOAT m_tmWindNextFire = 0.0f,
 44 FLOAT m_fWindFireTimeMin "AirBoss Wind Fire Min. Time" = 10.0f,
 45 FLOAT m_fWindFireTimeMax "AirBoss Wind Fire Max. Time" = 20.0f,
 46 INDEX m_iWind = 0, // temp index for wind firing

 50 BOOL  m_bDying = FALSE,  // are we currently dying
 51 FLOAT m_tmDeath = 1e6f,  // time when death begins
 52 FLOAT m_fDeathDuration = 0.0f, // length of death (for particles)

 60 FLOAT3D m_fWindBlastFirePosBegin = FLOAT3D(-0.44f, 0.7f, -0.94f),
 61 FLOAT3D m_fWindBlastFirePosEnd   = FLOAT3D(0.64f, 0.37f, -0.52f),

 70 FLOAT m_tmLastAnimation=0.0f,
 
 // temporary variables for reconstructing lost events
 90 CEntityPointer m_penDeathInflictor,
 91 BOOL m_bRenderParticles=FALSE,

100 CSoundObject m_soFire,  // sound channel for firing
101 CSoundObject m_soVoice,  // sound channel for voice

110 COLOR m_colParticles "Color of particles" = COLOR(C_WHITE|CT_OPAQUE),
// 51 INDEX m_ctSpawned = 0,
 
{
  CEmiter m_emEmiter;
}

components:
  0 class   CLASS_BASE          "Classes\\EnemyBase.ecl",
  1 class   CLASS_TWISTER       "Classes\\Twister.ecl",
  2 class   CLASS_BLOOD_SPRAY   "Classes\\BloodSpray.ecl",
  3 class   CLASS_PROJECTILE    "Classes\\Projectile.ecl",
//  3 class   CLASS_AIRSHOCKWAVE  "Classes\\AirShockwave.ecl",


 // air
 10 model   MODEL_INVISIBLE     "ModelsMP\\Enemies\\AirElemental\\AirElemental.mdl",
 11 model   MODEL_ELEMENTAL     "ModelsMP\\Enemies\\AirElemental\\Elemental.mdl",
 12 texture TEXTURE_ELEMENTAL   "ModelsMP\\Enemies\\AirElemental\\Elemental.tex",
 13 texture TEXTURE_DETAIL_ELEM "ModelsMP\\Enemies\\AirElemental\\Detail.tex",

// ************** SOUNDS **************
200 sound   SOUND_FIREWINDBLAST "ModelsMP\\Enemies\\AirElemental\\Sounds\\BlastFire.wav",
201 sound   SOUND_FIRETWISTER   "ModelsMP\\Enemies\\AirElemental\\Sounds\\Fire.wav",
202 sound   SOUND_ROAR          "ModelsMP\\Enemies\\AirElemental\\Sounds\\Anger.wav",
203 sound   SOUND_DEATH         "ModelsMP\\Enemies\\AirElemental\\Sounds\\Death.wav",
204 sound   SOUND_EXPLOSION     "ModelsMP\\Enemies\\AirElemental\\Sounds\\Explosion.wav",

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

  /*BOOL IsTargetValid(SLONG slPropertyOffset, CEntity *penTarget)
  {
    if( slPropertyOffset == _offsetof(Classname, propert_var) {
      if (IsOfClass(penTarget, "???")) { return TRUE; }
      else { return FALSE; }
    return CEntity::IsTargetValid(slPropertyOffset, penTarget);
  }*/
  
  // describe how this enemy killed player
  virtual CTString GetPlayerKillDescription(const CTString &strPlayerName, const EDeath &eDeath)
  {
    CTString str;
    str.PrintF(TRANSV("%s was -*blown away*- by an Air Elemental"), (const char *) strPlayerName);
    return str;
  }
  virtual const CTFileName &GetComputerMessageName(void) const {
    static DECLARE_CTFILENAME(fnm, "DataMP\\Messages\\Enemies\\AirElemental.txt");
    return fnm;
  };

  void Precache(void)
  {
    CEnemyBase::Precache();

    PrecacheClass(CLASS_TWISTER       );
    PrecacheClass(CLASS_BLOOD_SPRAY   );
    PrecacheClass(CLASS_PROJECTILE, PRT_AIRELEMENTAL_WIND );   

    PrecacheModel(MODEL_INVISIBLE     );
    PrecacheModel(MODEL_ELEMENTAL     );

    PrecacheTexture(TEXTURE_ELEMENTAL );

    PrecacheSound(SOUND_FIREWINDBLAST );
    PrecacheSound(SOUND_FIRETWISTER   );
    PrecacheSound(SOUND_ROAR          );
    PrecacheSound(SOUND_DEATH         );
    PrecacheSound(SOUND_EXPLOSION     );  
  };

  // Entity info
  void *GetEntityInfo(void) {
    return &eiAirElemental;
  };

  // get the attachment that IS the AirElemental
  CModelObject *ElementalModel(void) {
    CAttachmentModelObject &amo0 = *GetModelObject()->GetAttachmentModel(AIRELEMENTAL_ATTACHMENT_BODY);
    return &(amo0.amo_moModelObject);
  };

  // Receive damage
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    // nothing can harm elemental during initial animation
    if (m_bInitialAnim) { return; }

    // make sure we don't trigger another growth while growing
    FLOAT fHealth = GetHealth();
    FLOAT fFullDamage = fDamageAmmount * DamageStrength( ((EntityInfo*)GetEntityInfo())->Eeibt, dmtType) * GetGameDamageMultiplier();
    if (m_bAttGrow && m_iSize<2) { 
      if (fHealth-fFullDamage<afGrowArray[m_iSize+1][0]*m_fMaxHealth) {
        CEnemyBase::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);
        SetHealth(fHealth);
        return; 
      }
    } else if (m_bAttGrow && m_iSize==2) {
      if (fHealth-fFullDamage<1.0f) {
        CEnemyBase::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);
        SetHealth(fHealth);
        return;
      }
    }


    // elemental can't harm elemental
    if(IsOfClass(penInflictor, "AirElemental")) {
      return;
    }

    // boss cannot be telefragged
    if(dmtType==DMT_TELEPORT)
    {
      return;
    }
    
    // air elemental cannot be harmed by following kinds of damage:
    if(dmtType==DMT_CLOSERANGE ||
       dmtType==DMT_BULLET ||
       dmtType==DMT_IMPACT ||
       dmtType==DMT_CHAINSAW)
    {
      return;
    }
    
    // cannonballs inflict less damage then the default
    if(dmtType==DMT_CANNONBALL)
    {
      fDamageAmmount *= 0.6f;
    }
    
    FLOAT fOldHealth = GetHealth();
    CEnemyBase::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);
    FLOAT fNewHealth = GetHealth();
        
    CEntityPointer *penTrigger = &m_penTrigger01;
    // see if any triggers have to be set
    INDEX i;
    for (i=0; i<AIRBOSS_MAX_TA; i++) {
      FLOAT fHealth = afTriggerArray[i]*m_fMaxHealth;
      // triggers
      if (fHealth<=fOldHealth && fHealth>fNewHealth)
      {
        if (penTrigger[i].ep_pen) {
          SendToTarget(penTrigger[i].ep_pen, EET_TRIGGER, FixupCausedToPlayer(this, m_penEnemy));
        }
      }
    }
    // see if we have to grow
    for (i=0; i<AIRBOSS_MAX_GA; i++) {
      FLOAT fHealth = afGrowArray[i][0]*m_fMaxHealth;
      // growing
      if (fHealth<=fOldHealth && fHealth>fNewHealth)
      {
        m_fAttSizeRequested = afGrowArray[i][1];
        m_iSize = i;
        EElementalGrow eeg;
        SendEvent(eeg);
      }
    }

    // bosses don't darken when burning
    m_colBurning=COLOR(C_WHITE|CT_OPAQUE);

  };

  // damage anim
  INDEX AnimForDamage(FLOAT fDamage) {
    INDEX iAnim = ELEMENTAL_ANIM_IDLE;
    ElementalModel()->PlayAnim(iAnim, 0);
    return iAnim;
  };

  void StandingAnimFight(void) {
    ElementalModel()->PlayAnim(ELEMENTAL_ANIM_IDLE, AOF_LOOPING|AOF_NORESTART);
  };

  // virtual anim functions
  void StandingAnim(void) {
    ElementalModel()->PlayAnim(ELEMENTAL_ANIM_IDLE, AOF_LOOPING|AOF_NORESTART);
  };

  void WalkingAnim(void)
  {
    ElementalModel()->PlayAnim(ELEMENTAL_ANIM_IDLE, AOF_LOOPING|AOF_NORESTART);
  };

  void RunningAnim(void)
  {
    WalkingAnim();
  };

  void RotatingAnim(void) {
    WalkingAnim();
  };

  INDEX AnimForDeath(void)
  {
    INDEX iAnim;
    iAnim = ELEMENTAL_ANIM_IDLE;
    ElementalModel()->PlayAnim(iAnim, 0);
    return iAnim;
  };

  // virtual sound functions
  void IdleSound(void) {
    //PlaySound(m_soSound, SOUND_IDLE, SOF_3D);
  };
  void WoundSound(void) {
    //PlaySound(m_soSound, SOUND_WOUND, SOF_3D);
  };
  
  void SizeModel(void)
  {
    return;
  };

  // per-frame adjustments
  BOOL AdjustShadingParameters(FLOAT3D &vLightDirection, COLOR &colLight, COLOR &colAmbient) {
    return CMovableModelEntity::AdjustShadingParameters(vLightDirection, colLight, colAmbient);
  };
  

/************************************************************
 *                 BLOW UP FUNCTIONS                        *
 ************************************************************/
  // spawn body parts
  void BlowUp(void) {
    // get your size
    /*FLOATaabbox3D box;
    GetBoundingBox(box);
    FLOAT fEntitySize = box.Size().MaxNorm()/2;

    INDEX iCount = 7;
    FLOAT3D vNormalizedDamage = m_vDamage-m_vDamage*(m_fBlowUpAmount/m_vDamage.Length());
    vNormalizedDamage /= Sqrt(vNormalizedDamage.Length());
    vNormalizedDamage *= 1.75f;
    FLOAT3D vBodySpeed = en_vCurrentTranslationAbsolute-en_vGravityDir*(en_vGravityDir%en_vCurrentTranslationAbsolute);

    // hide yourself (must do this after spawning debris)
    SwitchToEditorModel();
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetCollisionFlags(ECF_IMMATERIAL);*/
  };


  // adjust sound and watcher parameters here if needed
  void EnemyPostInit(void) 
  {
    m_soFire.Set3DParameters(600.0f, 150.0f, 2.0f, 1.0f);
    m_soVoice.Set3DParameters(600.0f, 150.0f, 2.0f, 1.0f);
    m_soSound.Set3DParameters(600.0f, 150.0f, 2.0f, 1.0f);
  };

  void LaunchTwister(FLOAT3D vEnemyOffset)
  {
    // calculate parameters for predicted angular launch curve
    FLOAT3D vFirePos = FIREPOS_TWISTER*m_fAttSizeCurrent*GetRotationMatrix();
    FLOAT3D vShooting = GetPlacement().pl_PositionVector + vFirePos;
    FLOAT3D vTarget = m_penEnemy->GetPlacement().pl_PositionVector;
    FLOAT fLaunchSpeed;
    FLOAT fRelativeHdg;
    
    // shoot in front of the enemy
    EntityInfo *peiTarget = (EntityInfo*) (m_penEnemy->GetEntityInfo());
    
    // adjust target position
    vTarget += vEnemyOffset;

    CPlacement3D pl;
    CalculateAngularLaunchParams( vShooting, peiTarget->vTargetCenter[1]-6.0f/3.0f,
      vTarget, FLOAT3D(0.0f, 0.0f, 0.0f), 0.0f, fLaunchSpeed, fRelativeHdg);
    
    PrepareFreeFlyingProjectile(pl, vTarget, vFirePos, ANGLE3D( fRelativeHdg, 0.0f, 0.0f));
    
    ETwister et;
    CEntityPointer penTwister = CreateEntity(pl, CLASS_TWISTER);
    et.penOwner = this;
//    et.fSize = FRnd()*15.0f+5.0f;
    et.fSize = FRnd()*10.0f+m_fAttSizeCurrent/5.0f+3.0f;
    et.fDuration = 15.0f + FRnd()+5.0f;
    et.sgnSpinDir = (INDEX)(Sgn(FRnd()-0.5f));
    et.bGrow = TRUE;
    et.bMovingAllowed=TRUE;
    penTwister->Initialize(et);
    
    ((CMovableEntity &)*penTwister).LaunchAsFreeProjectile(FLOAT3D(0.0f, 0.0f, -fLaunchSpeed), (CMovableEntity*)(CEntity*)this);
  }

  void PreMoving() {

    // TODO: decomment this when shockwave is fixed
    /*// see if any of the players are really close to us
    INDEX ctMaxPlayers = GetMaxPlayers();
    CEntity *penPlayer;
        
    for(INDEX i=0; i<ctMaxPlayers; i++) {
      penPlayer=GetPlayerEntity(i);
      if (penPlayer!=NULL) {
        if (DistanceTo(this, penPlayer)<m_fShockwaveTreshold &&
          _pTimer->CurrentTick()>(m_tmLastShockwave+m_fShockwavePeriod)) {
          EAirShockwave eas;
          CEntityPointer penShockwave = CreateEntity(GetPlacement(), CLASS_AIRSHOCKWAVE);
          eas.penLauncher = this;
          eas.fHeight = 15.0f + m_iSize*10.0f;
          eas.fEndWidth = 80.0f + m_iSize*30.0f;
          eas.fDuration = 3.0f;
          penShockwave->Initialize(eas);
          m_tmLastShockwave = _pTimer->CurrentTick();
        }
      }
    }*/
    CEnemyBase::PreMoving();
  };

  void GetAirElementalAttachmentData(INDEX iAttachment, FLOATmatrix3D &mRot, FLOAT3D &vPos)
  {
    MakeRotationMatrixFast(mRot, ANGLE3D(0.0f, 0.0f, 0.0f));
    vPos=FLOAT3D(0.0f, 0.0f, 0.0f);
    GetModelObject()->GetAttachmentTransformations(AIRELEMENTAL_ATTACHMENT_BODY, mRot, vPos, FALSE);
    // next in hierarchy
    CAttachmentModelObject *pamo = GetModelObject()->GetAttachmentModel(AIRELEMENTAL_ATTACHMENT_BODY);
    pamo->amo_moModelObject.GetAttachmentTransformations( iAttachment, mRot, vPos, TRUE);
    vPos=GetPlacement().pl_PositionVector+vPos*GetRotationMatrix();
  }

  FLOAT GetCurrentStretchRatio(void)
  {
    CAttachmentModelObject &amo=*GetModelObject()->GetAttachmentModel(AIRELEMENTAL_ATTACHMENT_BODY);
    FLOAT fCurrentStretch=amo.amo_moModelObject.mo_Stretch(1);
    FLOAT fStretch=(fCurrentStretch-m_fAttSizeBegin)/(m_fAttSizeEnd-m_fAttSizeBegin);
    return fStretch;
  }

  void RenderParticles(void)
  {
    static TIME tmLastGrowTime = 0.0f;
    
    if (m_bFloat) {
      FLOAT fTime = _pTimer->GetLerpedCurrentTick();
      CAttachmentModelObject &amo0 = *GetModelObject()->GetAttachmentModel(AIRELEMENTAL_ATTACHMENT_BODY);
      amo0.amo_plRelative.pl_PositionVector(2) = m_fAttPosY + pow(sin(fTime*2.0f),2.0f)*m_fAttSizeCurrent*2.0f/m_fAttSizeBegin;
    }
    if (m_bAttGrow) {
      FLOAT fSize = Lerp(m_fLastSize, m_fTargetSize, _pTimer->GetLerpFactor());
      ElementalModel()->StretchModel(FLOAT3D(fSize, fSize, fSize));
    }

    if(m_bRenderParticles)
    {
      FLOAT fStretchRatio=GetCurrentStretchRatio();
      FLOAT fStretch=1.0f+(fStretchRatio)*6.0f;
      Particles_AirElemental(this, fStretch, 1.0f, m_tmDeath, m_colParticles);
    }
  }


procedures:
  
  Die(EDeath eDeath) : CEnemyBase::Die { 
    
    SetDesiredRotation(ANGLE3D(0.0f, 0.0f, 0.0f));
    PlaySound(m_soFire, SOUND_DEATH, SOF_3D);
    ElementalModel()->PlayAnim(ELEMENTAL_ANIM_DEATH, AOF_NORESTART);  
    m_tmDeath = _pTimer->CurrentTick()+ElementalModel()->GetAnimLength(ELEMENTAL_ANIM_DEATH);
    m_bFloat = FALSE;
    autowait(ElementalModel()->GetAnimLength(ELEMENTAL_ANIM_DEATH)-0.1f);

    PlaySound(m_soVoice, SOUND_EXPLOSION, SOF_3D);
    m_bDying = TRUE;
    m_fDeathDuration = 4.0f;

    autowait(m_fDeathDuration);

    EDeath eDeath;
    eDeath.eLastDamage.penInflictor = m_penDeathInflictor;
    jump CEnemyBase::Die(eDeath);
  }

/************************************************************
 *                      FIRE PROCEDURES                     *
 ************************************************************/

  Fire(EVoid) : CEnemyBase::Fire {
    
    if (m_tmWindNextFire<_pTimer->CurrentTick()) {
      ElementalModel()->PlayAnim(ELEMENTAL_ANIM_FIREPROJECTILES, AOF_NORESTART);  
      m_iWind = 0;
      PlaySound(m_soFire, SOUND_FIREWINDBLAST, SOF_3D);

      autowait(1.8f);
      while(m_iWind<5)
      {
        FLOAT3D vFirePos;
        vFirePos = Lerp(m_fWindBlastFirePosBegin*m_fAttSizeCurrent, 
                        m_fWindBlastFirePosEnd*m_fAttSizeCurrent,
                       (FLOAT)m_iWind*0.25f);
        ShootProjectile(PRT_AIRELEMENTAL_WIND, vFirePos,
                        ANGLE3D(30.0f-m_iWind*10.0f, 0.0f, 0.0f));
        m_iWind++;
        autowait(0.1f);
      }
      m_tmWindNextFire = _pTimer->CurrentTick() + Lerp(m_fWindFireTimeMin, m_fWindFireTimeMax, FRnd());
      
      autowait(ElementalModel()->GetAnimLength(ELEMENTAL_ANIM_FIREPROJECTILES)-1.75f);
      // stand a while
      ElementalModel()->PlayAnim(ELEMENTAL_ANIM_IDLE, AOF_LOOPING|AOF_SMOOTHCHANGE);
  
      autowait(0.05f);
    
      return EReturn();
    }

    ElementalModel()->PlayAnim(ELEMENTAL_ANIM_FIRETWISTER, AOF_NORESTART);
    //wait to get into twister emitting position
    PlaySound(m_soFire, SOUND_FIRETWISTER, SOF_3D);
    autowait(4.0f);
    
    FLOAT3D vOffset;
    // static enemy
    if (((CMovableEntity &)*m_penEnemy).en_vCurrentTranslationAbsolute.Length()==0.0f) {
      // almost directly at the enemy
      FLOAT3D vPlayerToThis = GetPlacement().pl_PositionVector - m_penEnemy->GetPlacement().pl_PositionVector;
      vPlayerToThis.Normalize();
      vOffset = FLOAT3D(vPlayerToThis*(FRnd()*10.0f+5.0f));
      LaunchTwister(vOffset);
      // to the left
      vOffset = FLOAT3D(-(FRnd()*5.0f+15.0f), 0.0f, (FRnd()-0.5f)*20.0f)*((CMovableEntity &)*m_penEnemy).GetRotationMatrix();
      LaunchTwister(vOffset);
      // to the right
      vOffset = FLOAT3D(+(FRnd()*5.0f+15.0f), 0.0f, 20.0f)*((CMovableEntity &)*m_penEnemy).GetRotationMatrix();
      LaunchTwister(vOffset);
    // moving enemy
    } else {
      FLOAT3D vPlayerSpeed = ((CMovableEntity &)*m_penEnemy).en_vCurrentTranslationAbsolute;
      if (vPlayerSpeed.Length()>15.0f) {
        vPlayerSpeed.Normalize();
        vPlayerSpeed = vPlayerSpeed*15.0f;
      }
      vOffset = vPlayerSpeed*(2.0f+FRnd());
      FLOAT3D vToPlayer = ((CMovableEntity &)*m_penEnemy).GetPlacement().pl_PositionVector - GetPlacement().pl_PositionVector;
      vToPlayer.Normalize();
      vToPlayer*=15.0f + FRnd()*5.0f;
      vOffset -= vToPlayer;
      LaunchTwister(vOffset);
      //LaunchTwister(vOffset+FLOAT3D(-5.0f-FRnd()*5.0f, 0.0f, -15.0f-FRnd()*5.0f));
      LaunchTwister(FLOAT3D(0.0f, 0.0f, 0.0f));
      LaunchTwister(vOffset+FLOAT3D(+5.0f+FRnd()*5.0f, 0.0f, -15.0f-FRnd()*5.0f));
    }
        
    //PlaySound(m_soSound, SOUND_FIRE, SOF_3D);
    
    autowait(ElementalModel()->GetAnimLength(ELEMENTAL_ANIM_FIRETWISTER)-4.0f);
    // stand a while
    ElementalModel()->PlayAnim(ELEMENTAL_ANIM_IDLE, AOF_LOOPING|AOF_SMOOTHCHANGE);
    autowait(0.05f);
    
    return EReturn();
  };
  
  Hit(EVoid) : CEnemyBase::Hit {
    jump Fire();
    return EReturn();
  };

/************************************************************
 *                    D  E  A  T  H                         *
 ************************************************************/
  Death(EVoid) : CEnemyBase::Death
  {
    m_fFadeStartTime = _pTimer->CurrentTick();
    m_bFadeOut = TRUE;
    m_fFadeTime = 2.0f;
    autowait(m_fFadeTime);
    autocall CEnemyBase::Death() EEnd;
    //GetModelObject()->mo_toBump.SetData( NULL);
    return EEnd();
  };

/************************************************************
 *                       M  A  I  N                         *
 ************************************************************/

  Grow() {
    // we can only grow, never shrink
    ASSERT(m_fAttSizeRequested>m_fAttSizeCurrent);
    m_fLastSize = m_fTargetSize = m_fAttSizeCurrent;

    PlaySound(m_soSound, SOUND_ROAR, SOF_3D);
    ElementalModel()->PlayAnim(ELEMENTAL_ANIM_IDLE, AOF_NORESTART);

    // stop rotations
    SetDesiredRotation(ANGLE3D(0.0f, 0.0f, 0.0f));
    m_bAttGrow = TRUE;
    while (m_fLastSize<m_fAttSizeRequested) {
      // keep turning towards target
      if (m_penEnemy) {
        FLOAT3D vToTarget;
        ANGLE3D aToTarget;
        vToTarget = m_penEnemy->GetPlacement().pl_PositionVector - GetPlacement().pl_PositionVector;
        vToTarget.Normalize();
        DirectionVectorToAngles(vToTarget, aToTarget);
        aToTarget(1) = aToTarget(1) - GetPlacement().pl_OrientationAngle(1);
        aToTarget(1) = NormalizeAngle(aToTarget(1));
        SetDesiredRotation(FLOAT3D(aToTarget(1)/2.0f, 0.0f, 0.0f));                 
      }

      // grow
      m_fLastSize = m_fTargetSize;
      m_fTargetSize += m_fGrowSpeed*_pTimer->TickQuantum;
      
      // change collision box in the middle of growth
      // NOTE: collision box definitions and animations in AirElemental.h
      // have to be ordered so that the one with value 0 represents
      // the initial one, then the boxes from 1-3 represent
      // the scaled versions of the original, in order
      FLOAT fMiddleSize = Lerp(m_fAttSizeCurrent, m_fAttSizeRequested, 0.33f);
      if (m_fLastSize<=fMiddleSize && fMiddleSize<m_fTargetSize) {
        if (m_iSize<2) {
          ChangeCollisionBoxIndexWhenPossible(m_iSize+1);
        } else if (TRUE) {
          ForceCollisionBoxIndexChange(m_iSize+1);
        }
      }
    
      autowait(_pTimer->TickQuantum);
    }
    m_bAttGrow = FALSE;

    m_fAttSizeCurrent = afGrowArray[m_iSize][1];
    
    m_fGrowSpeed *= 2.0f;
    if (m_iSize==1) {
      GetModelObject()->PlayAnim(AIRELEMENTAL_ANIM_SIZE50, AOF_LOOPING);
    }

    jump CEnemyBase::MainLoop();
  }

  ElementalLoop() {
    wait () {
      on (EBegin) :
      {
        call CEnemyBase::MainLoop();
      }
      on (EElementalGrow) :
      {
        call Grow();
      }
      otherwise (): {
        resume;
      }
    }
  }

  Main(EVoid) {
    
    // declare yourself as a model
    InitAsEditorModel();
    
    SetCollisionFlags(ECF_IMMATERIAL);
    SetPhysicsFlags(EPF_MODEL_IMMATERIAL);
    SetFlags(GetFlags()|ENF_ALIVE);
    
    en_fDensity = 10000.0f;
    m_fDamageWounded = 1e6f;
    
    m_sptType = SPT_AIRSPOUTS;
    m_bBoss = TRUE;
    SetHealth(15000.0f);
    m_fMaxHealth = 15000.0f;
    // setup moving speed
    m_fWalkSpeed = 0.0f;
    m_aWalkRotateSpeed = AngleDeg(FRnd()*10.0f + 245.0f);
    m_fAttackRunSpeed = m_fWalkSpeed;
    m_aAttackRotateSpeed = m_aWalkRotateSpeed;
    m_fCloseRunSpeed = m_fWalkSpeed;
    m_aCloseRotateSpeed = m_aWalkRotateSpeed;
    // setup attack distances
    m_fAttackDistance = 500.0f;
    m_fCloseDistance = 60.0f;
    m_fStopDistance = 30.0f;
    m_fAttackFireTime = 4.0f;
    m_fCloseFireTime = 4.0f;
    m_fIgnoreRange = 1000.0f;
    m_iScore = 500000;
   
    eiAirElemental.vSourceCenter[1] = AIRBOSS_EYES_HEIGHT*m_fAttSizeBegin;
    eiAirElemental.vTargetCenter[1] = AIRBOSS_BODY_HEIGHT*m_fAttSizeBegin;

    // set your appearance
    SetModel(MODEL_INVISIBLE);
    AddAttachmentToModel(this, *GetModelObject(), AIRELEMENTAL_ATTACHMENT_BODY, MODEL_ELEMENTAL, TEXTURE_ELEMENTAL, 0, 0, TEXTURE_DETAIL_ELEM);
    CAttachmentModelObject &amo0 = *GetModelObject()->GetAttachmentModel(AIRELEMENTAL_ATTACHMENT_BODY);
    m_fAttPosY = amo0.amo_plRelative.pl_PositionVector(2);
    StandingAnim();

    m_fAttSizeCurrent = m_fAttSizeBegin;

    GetModelObject()->StretchModel(FLOAT3D(1.0f, 1.0f, 1.0f));
    ModelChangeNotify();
    ElementalModel()->StretchModel(FLOAT3D(m_fAttSizeBegin, m_fAttSizeBegin, m_fAttSizeBegin));

    m_bRenderParticles=FALSE;
    autowait(_pTimer->TickQuantum);

    m_emEmiter.Initialize(this);
    m_emEmiter.em_etType=ET_AIR_ELEMENTAL;
    
    m_tmDeath = 1e6f;

    /*
    CPlacement3D pl=GetPlacement();
    ETwister et;
    CEntityPointer penTwister = CreateEntity(pl, CLASS_TWISTER);
    et.penOwner = this;
    et.fSize = 6.0f;
    et.fDuration = 1e6;
    et.sgnSpinDir = (INDEX)(Sgn(FRnd()-0.5f));
    et.bMovingAllowed=FALSE;
    et.bGrow = FALSE;
    penTwister->Initialize(et);
    penTwister->SetParent(this);
    */

    // wait to be triggered
    wait() {
      on (EBegin) : { resume; }
      on (ETrigger) : { stop; }
      otherwise (): { resume; }
    }

    SetCollisionFlags(ECF_AIR);
    SetPhysicsFlags(EPF_MODEL_WALKING);

    ElementalModel()->PlayAnim(ELEMENTAL_ANIM_RAISE, AOF_NORESTART);
    m_bRenderParticles=TRUE;
    //SwitchToModel();
    m_bInitialAnim = TRUE;
    // TODO: start particle animation
    autowait(ElementalModel()->GetAnimLength(ELEMENTAL_ANIM_RAISE));
    // TODO: stop particle animation
    ChangeCollisionBoxIndexWhenPossible(AIRELEMENTAL_COLLISION_BOX_COLLISION01);

    // deafult size
    GetModelObject()->PlayAnim(AIRELEMENTAL_ANIM_DEFAULT, AOF_LOOPING);
    
    ElementalModel()->PlayAnim(ELEMENTAL_ANIM_IDLE, AOF_LOOPING);
    m_bInitialAnim = FALSE;
    m_bFloat = TRUE;
    
    m_tmWindNextFire = _pTimer->CurrentTick() + 10.0f;

    // one state under base class to intercept some events
    jump ElementalLoop();
    //jump CEnemyBase::MainLoop();
  };
};
