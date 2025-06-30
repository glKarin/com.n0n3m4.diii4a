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

340
%{
#include "Entities/StdH/StdH.h"
#include "Models/Enemies/Mental/Mental.h"
%}

uses "Entities/EnemyBase";
uses "Entities/BasicEffects";

%{
// info structure
static EntityInfo eiMental = {
  EIBT_FLESH, 200.0f,
  0.0f, 1.5f, 0.0f,     // source (eyes)
  0.0f, 1.0f, 0.0f,     // target (body)
};

#define GREET_SENSE_RANGE 10.0f
#define GREET_SENSE_DELAY 30.0f

%}

class CBigHead: CEnemyBase {
name      "BigHead";
thumbnail "Thumbnails\\Mental.tbn";

properties:
  // class internal
  1 CTFileName m_fnmHeadTex "Head texture" 'H' = CTString(""),
  2 CTFileName m_fnmNameSnd "Name sound" 'S' = CTString(""),
  3 FLOAT m_tmLastGreetTime = -100.0f,
  
  {
    CAutoPrecacheSound m_aps;
    CAutoPrecacheTexture m_apt;
  }

components:
  1 class   CLASS_BASE            "Classes\\EnemyBase.ecl",
  2 class   CLASS_BLOOD_SPRAY     "Classes\\BloodSpray.ecl",
  3 class   CLASS_BASIC_EFFECT    "Classes\\BasicEffect.ecl",

// ************** DATA **************
 10 model   MODEL_MENTAL           "Models\\Enemies\\Mental\\Mental.mdl",
 11 texture TEXTURE_MENTAL         "Models\\Enemies\\Mental\\Mental.tex",
 12 model   MODEL_HEAD             "Models\\Enemies\\Mental\\Head.mdl",
 
 50 sound   SOUND_IDLE            "Models\\Enemies\\Mental\\Sounds\\Idle.wav",
 51 sound   SOUND_SIGHT           "Models\\Enemies\\Mental\\Sounds\\Sight.wav",
 52 sound   SOUND_WOUND           "Models\\Enemies\\Mental\\Sounds\\Wound.wav",
 53 sound   SOUND_DEATH           "Models\\Enemies\\Mental\\Sounds\\Death.wav",

functions:
  /* Entity info */
  void *GetEntityInfo(void)
  {
    return &eiMental;
  };

  void Precache(void)
  {
    CEnemyBase::Precache();
    PrecacheSound(SOUND_SIGHT);
    PrecacheSound(SOUND_IDLE);
    PrecacheSound(SOUND_WOUND);
    PrecacheSound(SOUND_DEATH);
    m_aps.Precache(m_fnmNameSnd);
    m_apt.Precache(m_fnmHeadTex);
  };

  // damage anim
  INDEX AnimForDamage(FLOAT fDamage) {
    INDEX iAnim;
    iAnim = MENTAL_ANIM_PANIC;
    StartModelAnim(iAnim, 0);
    return iAnim;
  };

  // death
  INDEX AnimForDeath(void) {
    INDEX iAnim;
    iAnim = MENTAL_ANIM_DEATH;
    StartModelAnim(iAnim, 0);
    return iAnim;
  };

  void DeathNotify(void) {
//    ChangeCollisionBoxIndexWhenPossible(HEADMAN_COLLISION_BOX_DEATH);
    en_fDensity = 500.0f;
  };

  // virtual anim functions
  void StandingAnim(void) {
    StartModelAnim(MENTAL_ANIM_GROUNDREST, AOF_LOOPING|AOF_NORESTART);
  };
  void WalkingAnim(void) {
    StartModelAnim(MENTAL_ANIM_RUN, AOF_LOOPING|AOF_NORESTART);
    if (_pTimer->CurrentTick()>m_tmLastGreetTime+GREET_SENSE_DELAY) {
      m_fSenseRange = GREET_SENSE_RANGE;
      m_bDeaf = FALSE;
    }
  };
  void RunningAnim(void)
  {
    StartModelAnim(MENTAL_ANIM_RUN, AOF_LOOPING|AOF_NORESTART);
  };
  void RotatingAnim(void) {
    RunningAnim();
  };

  // virtual sound functions
  void IdleSound(void) {
    PlaySound(m_soSound, SOUND_IDLE, SOF_3D);
  };

  void SightSound(void) {
    PlaySound(m_soSound, SOUND_SIGHT, SOF_3D);
  };
  void WoundSound(void) {
    PlaySound(m_soSound, SOUND_WOUND, SOF_3D);
  };
  void DeathSound(void) {
    PlaySound(m_soSound, SOUND_DEATH, SOF_3D);
  };

procedures:
  Fire(EVoid) : CEnemyBase::Fire {
    // hit
    if (CalcDist(m_penEnemy) <= m_fStopDistance*1.1f) {
      PlaySound(m_soSound, m_fnmNameSnd, SOF_3D);
      m_bBlind = TRUE;
      m_bDeaf = TRUE;
      m_fSenseRange = 0.0f;
      m_tmLastGreetTime = _pTimer->CurrentTick();
      SetTargetNone();
      StartModelAnim(MENTAL_ANIM_GREET, 0);
      autowait(GetModelObject()->GetCurrentAnimLength());
      StandingAnim();
      return EReconsiderBehavior();
    }
    return EReturn();
  }

 /************************************************************
 *                       M  A  I  N                         *
 ************************************************************/
  Main(EVoid) {
    // declare yourself as a model
    InitAsModel();
    SetPhysicsFlags(EPF_MODEL_WALKING|EPF_HASLUNGS);
    SetCollisionFlags(ECF_MODEL);
    SetFlags(GetFlags()|ENF_ALIVE);
    SetHealth(20.0f);
    m_fMaxHealth = 20.0f;
    en_tmMaxHoldBreath = 5.0f;
    en_fDensity = 2000.0f;
    m_fBlowUpSize = 2.0f;

    // set your appearance
    SetModel(MODEL_MENTAL);
    SetModelMainTexture(TEXTURE_MENTAL);
    AddAttachment(0, MODEL_HEAD, TEXTURE_MENTAL);
    if (m_fnmHeadTex!="") {
      // try to
      try {
        CAttachmentModelObject *pamoHead = GetModelObject()->GetAttachmentModel(0);
        if (pamoHead!=NULL) {
          pamoHead->amo_moModelObject.mo_toTexture.SetData_t(m_fnmHeadTex);
        }
      // if anything failed
      } catch (const char *strError) {
        // report error
        CPrintF("%s\n", (const char *)strError);
        AddAttachment(0, MODEL_HEAD, TEXTURE_MENTAL);
      }
    }

    // setup moving speed
    m_fWalkSpeed = FRnd() + 1.5f;
    m_aWalkRotateSpeed = AngleDeg(FRnd()*10.0f + 500.0f);
    m_fAttackRunSpeed = FRnd()*2.0f + 6.0f;
    m_aAttackRotateSpeed = AngleDeg(FRnd()*50 + 245.0f);
    m_fCloseRunSpeed = FRnd()*2.0f + 6.0f;
    m_aCloseRotateSpeed = AngleDeg(FRnd()*50 + 245.0f);
    // setup attack distances
    m_fAttackDistance = 50.0f;
    m_fCloseDistance = 0.0f;
    m_fStopDistance = 5.0f; // greeting distance
    m_fAttackFireTime = 0.1f;
    m_fCloseFireTime = 0.1f;
    m_fIgnoreRange = 200.0f;
    // damage/explode properties
    m_fBlowUpAmount = 65.0f;
    m_fBodyParts = 4;
    m_fDamageWounded = 1.0f;
    m_iScore = 0;
    m_bBlind = TRUE;
    m_fSenseRange = GREET_SENSE_RANGE;

    // set stretch factors for height and width
    const FLOAT fSize = 0.6f;
    GetModelObject()->StretchModel(FLOAT3D(fSize, fSize, fSize));
    ModelChangeNotify();
    StandingAnim();

    // continue behavior in base class
    jump CEnemyBase::MainLoop();
  };
};
