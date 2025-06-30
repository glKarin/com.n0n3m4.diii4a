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
#include "EntitiesMP/StdH/StdH.h"
#include "ModelsMP/Enemies/Mental/Mental.h"
%}

uses "EntitiesMP/EnemyBase";
uses "EntitiesMP/BasicEffects";

enum BigHeadType {
  0 BHT_NORMAL  "Normal",
  1 BHT_ZOMBIE  "Zombie",
  2 BHT_SAINT   "Saint",
};

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
  4 enum BigHeadType m_bhtType  "Type" 'Y'= BHT_NORMAL,
  5 BOOL m_bIgnorePlayer "Ignore player" 'I' = FALSE,
  6 BOOL m_bPlayingWalkSound = FALSE,
  7 BOOL m_bSleeping "Sleeping" 'S' = FALSE,  // set to make it sleep initally
  8 FLOAT m_tmLastWalkingSoundTime = -100.0f,
  9 FLOAT m_tmWalkingSound "Walk sound frequency" = 5.0f,
  
  {
    CAutoPrecacheSound m_aps;
    CAutoPrecacheTexture m_apt;
  }

components:
  1 class   CLASS_BASE            "Classes\\EnemyBase.ecl",
  2 class   CLASS_BLOOD_SPRAY     "Classes\\BloodSpray.ecl",
  3 class   CLASS_BASIC_EFFECT    "Classes\\BasicEffect.ecl",

// ************** DATA **************
 10 model   MODEL_MENTAL          "ModelsMP\\Enemies\\Mental\\Mental.mdl",
 11 texture TEXTURE_MENTAL        "ModelsMP\\Enemies\\Mental\\Mental.tex",
 12 model   MODEL_HEAD            "ModelsMP\\Enemies\\Mental\\Head.mdl",
 13 model   MODEL_HORNS           "ModelsMP\\Enemies\\Mental\\Horns.mdl",
 14 texture TEXTURE_HORNS         "ModelsMP\\Enemies\\Mental\\Horns.tex",
 15 model   MODEL_AURA            "ModelsMP\\Enemies\\Mental\\Aura.mdl",
 16 texture TEXTURE_AURA          "ModelsMP\\Enemies\\Mental\\Aura.tex",
 
 50 sound   SOUND_IDLE            "Models\\Enemies\\Mental\\Sounds\\Idle.wav",
 51 sound   SOUND_SIGHT           "Models\\Enemies\\Mental\\Sounds\\Sight.wav",
 52 sound   SOUND_WOUND           "Models\\Enemies\\Mental\\Sounds\\Wound.wav",
 53 sound   SOUND_DEATH           "Models\\Enemies\\Mental\\Sounds\\Death.wav",
 54 sound   SOUND_WALKZOMBIE      "ModelsMP\\Enemies\\Mental\\Sounds\\ComeToDaddy.wav",
 55 sound   SOUND_WALKSAINT       "ModelsMP\\Enemies\\Mental\\Sounds\\PeaceWithYou.wav",

functions:
  BOOL HandleEvent(const CEntityEvent &ee)
  {
    if (m_bIgnorePlayer) {
      if (ee.ee_slEvent==EVENTCODE_ETouch) {
        ETouch &et = (ETouch &)ee;
        if (IsOfClass(et.penOther, "Player")) {
          return TRUE;
        }
      }
    }
    return CEnemyBase::HandleEvent(ee);
  }

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
    PrecacheSound(SOUND_WALKZOMBIE);
    PrecacheSound(SOUND_WALKSAINT);
    m_aps.Precache(m_fnmNameSnd);
    m_apt.Precache(m_fnmHeadTex);
  };

  INDEX GetWalkAnim(void)
  {
    if (m_bhtType==BHT_ZOMBIE) {
      return MENTAL_ANIM_WALKZOMBIE;
    } else if (m_bhtType==BHT_SAINT) {
      return MENTAL_ANIM_WALKANGEL;
    } else {
      return MENTAL_ANIM_RUN;
    }
  }

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
    PlayWalkSound();
    StartModelAnim(GetWalkAnim(), AOF_LOOPING|AOF_NORESTART);

    if (!m_bIgnorePlayer && _pTimer->CurrentTick()>m_tmLastGreetTime+GREET_SENSE_DELAY) {
      m_fSenseRange = GREET_SENSE_RANGE;
      m_bDeaf = FALSE;
    }
  };
  void RunningAnim(void)
  {
    PlayWalkSound();
    StartModelAnim(GetWalkAnim(), AOF_LOOPING|AOF_NORESTART);
  };
  void RotatingAnim(void) {
    RunningAnim();
  };

  void PlayWalkSound(void)
  {
    INDEX iSound = SOUND_WALKZOMBIE;
    if (m_bhtType==BHT_ZOMBIE) {
      iSound = SOUND_WALKZOMBIE;
    } else if (m_bhtType==BHT_SAINT) {
      iSound = SOUND_WALKSAINT;
    } else {
      return;
    }
    if (!m_bPlayingWalkSound || _pTimer->CurrentTick()-m_tmLastWalkingSoundTime>m_tmWalkingSound) {
      m_bPlayingWalkSound = TRUE;
      m_tmLastWalkingSoundTime = _pTimer->CurrentTick();
      PlaySound(m_soSound, iSound, SOF_3D);
    }
  }

  // virtual sound functions
  void IdleSound(void) {
    if (m_bIgnorePlayer) {
      return;
    }
    PlaySound(m_soSound, SOUND_IDLE, SOF_3D);
    m_bPlayingWalkSound = FALSE;
  };

  void SightSound(void) {
    PlaySound(m_soSound, SOUND_SIGHT, SOF_3D);
    m_bPlayingWalkSound = FALSE;
  };
  void WoundSound(void) {
    PlaySound(m_soSound, SOUND_WOUND, SOF_3D);
    m_bPlayingWalkSound = FALSE;
  };
  void DeathSound(void) {
    PlaySound(m_soSound, SOUND_DEATH, SOF_3D);
    m_bPlayingWalkSound = FALSE;
  };

procedures:
  Fire(EVoid) : CEnemyBase::Fire {
    // hit
    if (CalcDist(m_penEnemy) <= m_fStopDistance*1.1f) {
      if (m_fnmNameSnd!="") {
        PlaySound(m_soSound, m_fnmNameSnd, SOF_3D);
        m_bPlayingWalkSound = FALSE;
      }
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

  Sleep(EVoid)
  {
    // start sleeping anim
    StartModelAnim(MENTAL_ANIM_SNORE, AOF_LOOPING);
    // repeat
    wait() {
      // if triggered
      on(ETrigger eTrigger) : {
//        // remember enemy
//        SetTargetSoft(eTrigger.penCaused);
        // wake up
        jump WakeUp();
      }
/*      // if damaged
      on(EDamage eDamage) : {
        // wake up
        jump WakeUp();
      }
      */
      otherwise() : {
        resume;
      }
    }
  }

  WakeUp(EVoid)
  {
    // wakeup anim
    SightSound();
    StartModelAnim(MENTAL_ANIM_GETUP, 0);
    autowait(GetModelObject()->GetCurrentAnimLength());

    // trigger your target
//    SendToTarget(m_penDeathTarget, m_eetDeathType);
    // proceed with normal functioning
    return EReturn();
  }

  // overridable called before main enemy loop actually begins
  PreMainLoop(EVoid) : CEnemyBase::PreMainLoop
  {
    // if sleeping
    if (m_bSleeping) {
      m_bSleeping = FALSE;
      // go to sleep until waken up
      wait() {
        on (EBegin) : {
          call Sleep();
        }
        on (EReturn) : {
          stop;
        };
        // if dead
        on(EDeath eDeath) : {
          // die
          jump CEnemyBase::Die(eDeath);
        }
      }
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
      } catch ( const char *strError) {
        // report error
        CPrintF("%s\n", (const char *)strError);
        AddAttachment(0, MODEL_HEAD, TEXTURE_MENTAL);
      }
    }
    if (m_bhtType==BHT_ZOMBIE) {
      AddAttachment(MENTAL_ATTACHMENT_HORNS, MODEL_HORNS, TEXTURE_HORNS);
    } else if (m_bhtType==BHT_SAINT) {
      AddAttachment(MENTAL_ATTACHMENT_AURA, MODEL_AURA, TEXTURE_AURA);
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
    if (m_bIgnorePlayer) {
      m_bBlind = TRUE;
      m_bDeaf = TRUE;
      m_fSenseRange = 0;
    } else {
      m_bBlind = TRUE;
      m_fSenseRange = GREET_SENSE_RANGE;
    }

    // set stretch factors for height and width
    const FLOAT fSize = 0.6f;
    GetModelObject()->StretchModel(FLOAT3D(fSize, fSize, fSize));
    ModelChangeNotify();
    StandingAnim();

    // continue behavior in base class
    jump CEnemyBase::MainLoop();
  };
};
