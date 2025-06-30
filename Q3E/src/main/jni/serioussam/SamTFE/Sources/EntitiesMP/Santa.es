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

355
%{
#include "EntitiesMP/StdH/StdH.h"
#include "ModelsMP/Player/SeriousSam/Player.h"
#include "ModelsMP/Player/SeriousSam/Body.h"
#include "ModelsMP/Player/SeriousSam/Head.h"
%}

uses "EntitiesMP/EnemyBase";

%{
// info structure
static EntityInfo eiSanta = {
  EIBT_AIR, 100.0f,
  0, 1.6f, 0,     // source (eyes)
  0.0f, 1.0f, 0.0f,     // target (body)
};
%}


class CSanta : CEnemyBase {
name      "Santa";
thumbnail "Thumbnails\\Santa.tbn";

properties:
  1 FLOAT m_tmLastSpawnTime = -10000.0f,
  2 FLOAT m_tmMinSpawnInterval "Min spawn interval" = 1.0f,
  3 FLOAT m_fSantaHealth "Santa Health" = 100.0f,
  4 CSoundObject m_soRunning,            // for running sound
  5 BOOL m_bRunSoundPlaying = FALSE,

  10 CEntityPointer m_penTemplate0 "Item template 0",
  11 CEntityPointer m_penTemplate1 "Item template 1",
  12 CEntityPointer m_penTemplate2 "Item template 2",
  13 CEntityPointer m_penTemplate3 "Item template 3",
  14 CEntityPointer m_penTemplate4 "Item template 4",
  
components:
  0 class   CLASS_BASE        "Classes\\EnemyBase.ecl",

// ************** SOUNDS **************
 50 sound   SOUND_RUN       "ModelsMP\\CutSequences\\Santa\\Sounds\\Running.wav",
 51 sound   SOUND_WOUND     "ModelsMP\\CutSequences\\Santa\\Sounds\\Wound.wav",
 52 sound   SOUND_DEATH     "ModelsMP\\CutSequences\\Santa\\Sounds\\Death.wav",

functions:
  void Precache(void) {
    CEnemyBase::Precache();
    PrecacheSound(SOUND_RUN  );
    PrecacheSound(SOUND_WOUND);
    PrecacheSound(SOUND_DEATH);
  };

  virtual const CTFileName &GetComputerMessageName(void) const {
    //static DECLARE_CTFILENAME(fnm, "DataMP\\Messages\\Enemies\\Santa.txt");
    static CTFileName fnm;
    return fnm;
  };

  /* Entity info */
  void *GetEntityInfo(void) {
    return &eiSanta;
  };

  // running sounds
  void ActivateRunningSound(void)
  {
    if (!m_bRunSoundPlaying) {
      PlaySound(m_soRunning, SOUND_RUN, SOF_3D|SOF_LOOP);
      m_bRunSoundPlaying = TRUE;
    }
  }
  void DeactivateRunningSound(void)
  {
    m_soRunning.Stop();
    m_bRunSoundPlaying = FALSE;
  }

  /* Handle an event, return false if the event is not handled. */
  BOOL HandleEvent(const CEntityEvent &ee)
  {
    // ignore touching, damaging...
    if (ee.ee_slEvent==EVENTCODE_ETouch || ee.ee_slEvent==EVENTCODE_EDamage) {
      return TRUE;
    }

    return CEnemyBase::HandleEvent(ee);
  }

  /* Receive damage */
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    // skip base enemy damage handling
    CMovableModelEntity::ReceiveDamage(penInflictor, dmtType, fDamageAmmount, vHitPoint, vDirection);

    // if not enough time passed since lst item spawning
    if (_pTimer->CurrentTick()-m_tmLastSpawnTime<m_tmMinSpawnInterval) {
      // do nothing
      return;
    }

    // play wound sound
    WoundSound();

    // remember time
    m_tmLastSpawnTime = _pTimer->CurrentTick();
    // choose an item to spawn
    INDEX ctTemplates = 0;
    if (m_penTemplate0!=NULL) { ctTemplates++; }
    if (m_penTemplate1!=NULL) { ctTemplates++; }
    if (m_penTemplate2!=NULL) { ctTemplates++; }
    if (m_penTemplate3!=NULL) { ctTemplates++; }
    if (m_penTemplate4!=NULL) { ctTemplates++; }
    if (ctTemplates==0) {
      return;
    }
    INDEX iTemplate = IRnd()%ctTemplates;
    CEntity *penItem = (&m_penTemplate0)[iTemplate];

    // if the target doesn't exist, or is destroyed
    if (penItem==NULL || (penItem->GetFlags()&ENF_DELETED)) {
      // do nothing
      return;
    }

    CEntity *penSpawned = GetWorld()->CopyEntityInWorld( *penItem,
      CPlacement3D(FLOAT3D(-32000.0f+FRnd()*200.0f, -32000.0f+FRnd()*200.0f, 0), ANGLE3D(0, 0, 0)) );

    // teleport back
    CPlacement3D pl = GetPlacement();
    pl.pl_PositionVector += GetRotationMatrix().GetColumn(2)*1.5f;  // a bit up in the air
    penSpawned->Teleport(pl, FALSE);
  };

  void LeaveStain( BOOL bGrow)
  {
  }

  // damage anim
  INDEX AnimForDamage(FLOAT fDamage) {
//    StartModelAnim(iAnim, 0);
    return 0;
  };

  // death
  INDEX AnimForDeath(void) {
//    StartModelAnim(iAnim, 0);
    return 0;
  };

  void DeathNotify(void) {
    SwitchToEditorModel();
//    ChangeCollisionBoxIndexWhenPossible(PLAYER_COLLISION_BOX_DEATH);
    SetCollisionFlags(ECF_MODEL);
    DeactivateRunningSound();
  };

  CModelObject *GetBody(void)
  {
    return &GetModelObject()->GetAttachmentModel(PLAYER_ATTACHMENT_TORSO)->amo_moModelObject;
  }

// virtual anim functions
  void StandingAnim(void) {
//    StartModelAnim(PLAYER_ANIM_STAND, AOF_LOOPING|AOF_NORESTART);
//    GetBody()->PlayAnim(BODY_ANIM_WAIT, AOF_LOOPING|AOF_NORESTART);
  };
  void WalkingAnim(void) {
    ActivateRunningSound();
//    StartModelAnim(PLAYER_ANIM_NORMALWALK, AOF_LOOPING|AOF_NORESTART);
//    GetBody()->PlayAnim(BODY_ANIM_NORMALWALK, AOF_LOOPING|AOF_NORESTART);
  };
  void RunningAnim(void) {
    ActivateRunningSound();
//    StartModelAnim(PLAYER_ANIM_RUN, AOF_LOOPING|AOF_NORESTART);
//    GetBody()->PlayAnim(BODY_ANIM_NORMALWALK, AOF_LOOPING|AOF_NORESTART);
  };
  void RotatingAnim(void) {
    ActivateRunningSound();
//    StartModelAnim(BODY_ANIM_NORMALWALK, AOF_LOOPING|AOF_NORESTART);
//    GetBody()->PlayAnim(BODY_ANIM_NORMALWALK, AOF_LOOPING|AOF_NORESTART);
  };

  // virtual sound functions
  void IdleSound(void) {
//    PlaySound(m_soSound, SOUND_IDLE, SOF_3D);
  };
  void SightSound(void) {
//    PlaySound(m_soSound, SOUND_SIGHT, SOF_3D);
  };
  void WoundSound(void) {
    PlaySound(m_soSound, SOUND_WOUND, SOF_3D);
  };
  void DeathSound(void) {
    PlaySound(m_soSound, SOUND_DEATH, SOF_3D);
  };


  // adjust sound and watcher parameters here if needed
  void EnemyPostInit(void) 
  {
    // set sound default parameters
    m_soSound.Set3DParameters(160.0f, 50.0f, 1.0f, 1.0f);
  };

procedures:
/************************************************************
 *                A T T A C K   E N E M Y                   *
 ************************************************************/
  // shoot
  Fire(EVoid) : CEnemyBase::Fire{
    return EReturn();
  };

  // hit enemy
  Hit(EVoid) : CEnemyBase::Hit {
    return EReturn();
  };

/************************************************************
 *                       M  A  I  N                         *
 ************************************************************/
  Main(EVoid) {

    // declare yourself as a model
    InitAsModel();
    SetPhysicsFlags(EPF_MODEL_WALKING|EPF_HASLUNGS);
    SetCollisionFlags(ECF_MODEL);
    SetFlags(GetFlags()|ENF_ALIVE);
    en_tmMaxHoldBreath = 25.0f;
    en_fDensity = 3000.0f;

    // set your appearance
    CTString strDummy;
    extern BOOL SetPlayerAppearance_internal(CModelObject *pmo, const CTFileName &fnmAMC, CTString &strName, BOOL bPreview);
    SetPlayerAppearance_internal(GetModelObject(), CTFILENAME("ModelsMP\\CutSequences\\Santa\\Santa.amc"), strDummy, /*bPreview=*/FALSE);

    SetHealth(m_fSantaHealth);
    m_fMaxHealth = m_fSantaHealth;
    // damage/explode properties
    m_fDamageWounded = 1E10f;
    m_fBlowUpAmount = 1E10f;
    m_fBodyParts = 30;
    // setup attack distances
    m_fAttackDistance = 150.0f;
    m_fCloseDistance = 5.0f;
    m_fStopDistance = 4.5f;
    m_fAttackFireTime = 0.5f;
    m_fCloseFireTime = 1.0f;
    m_fIgnoreRange = 300.0f;
    m_iScore = 1000;
    // setup moving speed
    m_fWalkSpeed = (FRnd() + 1.5f)*1.5f;
    m_aWalkRotateSpeed = AngleDeg(FRnd()*20.0f + 550.0f);
    m_fAttackRunSpeed = FRnd()*1.5f + 4.5f;
    m_aAttackRotateSpeed = AngleDeg(FRnd()*50.0f + 275.0f);
    m_fCloseRunSpeed = FRnd()*1.5f + 4.5f;
    m_aCloseRotateSpeed = AngleDeg(FRnd()*50.0f + 275.0f);

    // set stretch factors for height and width
    CEnemyBase::SizeModel();
    m_soRunning.Set3DParameters(500.0f, 50.0f, 1.0f, 1.0f);
    m_bRunSoundPlaying = FALSE;
    // continue behavior in base class
    jump CEnemyBase::MainLoop();

  };
};
