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

341
%{
#include "EntitiesMP/StdH/StdH.h"
#include "Models/Enemies/Mental/Mental.h"
#include "Models/CutSequences/CrateRider/Crate.h"
%}

uses "EntitiesMP/EnemyBase";
uses "EntitiesMP/BasicEffects";

%{
// info structure
static EntityInfo eiCrate = {
  EIBT_FLESH, 500.0f,
  0.0f, 1.5f, 0.0f,     // source (eyes)
  0.0f, 1.0f, 0.0f,     // target (body)
};

#define GREET_SENSE_RANGE 10.0f
#define GREET_SENSE_DELAY 10.0f

%}

class CCrateRider: CEnemyBase {
name      "CrateRider";
thumbnail "Thumbnails\\Mental.tbn";

properties:
  // class internal
  1 CTFileName m_fnmHeadTex1 "Head texture1" 'H' = CTString(""),
  2 CTFileName m_fnmHeadTex2 "Head texture2" = CTString(""),
  3 CTFileName m_fnmDriveSnd "Drive sound" 'S' = CTString(""),
  
  {
    CAutoPrecacheSound m_aps;
    CAutoPrecacheTexture m_apt1;
    CAutoPrecacheTexture m_apt2;
  }

components:
  1 class   CLASS_BASE            "Classes\\EnemyBase.ecl",
  2 class   CLASS_DEBRIS          "Classes\\Debris.ecl",
  3 class   CLASS_BLOOD_SPRAY     "Classes\\BloodSpray.ecl",
  4 class   CLASS_BASIC_EFFECT    "Classes\\BasicEffect.ecl",

// ************** DATA **************
 10 model   MODEL_MENTAL           "Models\\Enemies\\Mental\\Mental.mdl",
 11 texture TEXTURE_MENTAL         "Models\\Enemies\\Mental\\Mental.tex",
 12 model   MODEL_HEAD             "Models\\Enemies\\Mental\\Head.mdl",
 13 model   MODEL_CRATE            "Models\\CutSequences\\CrateRider\\Crate.mdl",
 14 texture TEXTURE_CRATE          "Models\\CutSequences\\CrateRider\\Crate.tex",
 15 texture TEXTURE_BUMP           "Models\\CutSequences\\Bridge\\BridgeBump.tex",

functions:
  /* Entity info */
  void *GetEntityInfo(void)
  {
    return &eiCrate;
  };

  void Precache(void)
  {
    CEnemyBase::Precache();

    PrecacheClass(CLASS_DEBRIS);

    m_apt1.Precache(m_fnmHeadTex1);
    m_apt2.Precache(m_fnmHeadTex2);
    m_aps.Precache(m_fnmDriveSnd);
  };

  // damage anim
  INDEX AnimForDamage(FLOAT fDamage) {
    INDEX iAnim;
    iAnim = 0;
    StartModelAnim(iAnim, 0);
    return iAnim;
  };

  // death
  INDEX AnimForDeath(void) {
    INDEX iAnim;
    iAnim = 0;
    StartModelAnim(iAnim, 0);
    return iAnim;
  };

  void DeathNotify(void) {
//    ChangeCollisionBoxIndexWhenPossible(HEADMAN_COLLISION_BOX_DEATH);
    en_fDensity = 500.0f;
  };

  // virtual anim functions
  void StandingAnim(void) {
    StartModelAnim(CRATE_ANIM_DEFAULT, AOF_LOOPING|AOF_NORESTART|AOF_SMOOTHCHANGE);

    CModelObject *pmo0 = &(GetModelObject()->GetAttachmentModel(0)->amo_moModelObject);
    pmo0->PlayAnim(MENTAL_ANIM_CRATEANIMLEFTSEATING, AOF_LOOPING|AOF_NORESTART|AOF_SMOOTHCHANGE);
    CModelObject *pmo1 = &(GetModelObject()->GetAttachmentModel(1)->amo_moModelObject);
    pmo1->PlayAnim(MENTAL_ANIM_CRATEANIMRIGHTSEATING, AOF_LOOPING|AOF_NORESTART|AOF_SMOOTHCHANGE);

    m_soSound.Stop();
  };
  void WalkingAnim(void) {
    RunningAnim();
    StartModelAnim(CRATE_ANIM_DRIVE, AOF_LOOPING|AOF_NORESTART);
  };
  void RunningAnim(void)
  {
    if (m_fnmDriveSnd!="") {
      PlaySound(m_soSound, m_fnmDriveSnd, SOF_3D);
    }
    CModelObject *pmo0 = &(GetModelObject()->GetAttachmentModel(0)->amo_moModelObject);
    pmo0->PlayAnim(MENTAL_ANIM_CRATEANIMLEFT, AOF_LOOPING|AOF_NORESTART);
    CModelObject *pmo1 = &(GetModelObject()->GetAttachmentModel(1)->amo_moModelObject);
    pmo1->PlayAnim(MENTAL_ANIM_CRATEANIMRIGHT, AOF_LOOPING|AOF_NORESTART);

    StartModelAnim(CRATE_ANIM_DRIVE, AOF_LOOPING|AOF_NORESTART);
  };
  void RotatingAnim(void) {
    RunningAnim();
  };

  // virtual sound functions
  void IdleSound(void) {
//    PlaySound(m_soSound, SOUND_IDLE, SOF_3D);
  };

  void SightSound(void) {
//    PlaySound(m_soSound, SOUND_SIGHT, SOF_3D);
  };
  void WoundSound(void) {
//    PlaySound(m_soSound, SOUND_WOUND, SOF_3D);
  };
  void DeathSound(void) {
//    PlaySound(m_soSound, SOUND_DEATH, SOF_3D);
  };

  void AddRider(INDEX i, const CTFileName &fnmHead)
  {
    AddAttachment(i, MODEL_MENTAL, TEXTURE_MENTAL);
    CModelObject *pmoMain = &(GetModelObject()->GetAttachmentModel(i)->amo_moModelObject);
//    pmoMain->PlayAnim(i==0 ? MENTAL_ANIM_CRATEANIMLEFTSEATING : MENTAL_ANIM_CRATEANIMRIGHTSEATING, AOF_LOOPING);
    pmoMain->PlayAnim(i==0 ? MENTAL_ANIM_CRATEANIMLEFT: MENTAL_ANIM_CRATEANIMRIGHT, AOF_LOOPING);
    AddAttachmentToModel(this, *pmoMain, 0, MODEL_HEAD, TEXTURE_MENTAL, 0, 0, 0);
    CModelObject *pmoHead = &(pmoMain->GetAttachmentModel(0)->amo_moModelObject);
    if (fnmHead!="") {
      // try to
      try {
        pmoHead->mo_toTexture.SetData_t(fnmHead);
      // if anything failed
      } catch ( const char *strError) {
        // report error
        CPrintF("%s\n", (const char *)strError);
      }
    }
  }

procedures:

 /************************************************************
 *                       M  A  I  N                         *
 ************************************************************/
  Main(EVoid) {
    // declare yourself as a model
    InitAsModel();
    SetPhysicsFlags(EPF_MODEL_WALKING|EPF_HASLUNGS);
    SetCollisionFlags(ECF_MODEL);
    SetFlags(GetFlags()|ENF_ALIVE);
    SetHealth(1.0f);
    m_fMaxHealth = 1.0f;
    en_tmMaxHoldBreath = 5.0f;
    en_fDensity = 2000.0f;
    m_fBlowUpSize = 2.0f;

    // set your appearance
    SetModel(MODEL_CRATE);
    SetModelMainTexture(TEXTURE_CRATE);
    AddRider(0, m_fnmHeadTex1);
    AddRider(1, m_fnmHeadTex2);

    // setup moving speed
    m_fWalkSpeed = 
    m_fAttackRunSpeed = 
    m_fCloseRunSpeed = 1.0f;
    m_aWalkRotateSpeed = AngleDeg(30.0f);
    m_aAttackRotateSpeed = AngleDeg(30);
    m_aCloseRotateSpeed = AngleDeg(30);
    // setup attack distances
    m_fAttackDistance = 50.0f;
    m_fCloseDistance = 0.0f;
    m_fStopDistance = 5.0f; // greeting distance
    m_fAttackFireTime = 2.0f;
    m_fCloseFireTime = 1.0f;
    m_fIgnoreRange = 200.0f;
    // damage/explode properties
    m_fBlowUpAmount = 0.0f;
    m_fBodyParts = 4;
    m_fDamageWounded = 1.0f;
    m_iScore = 0;
    m_bBlind = TRUE;
    m_bRobotBlowup = TRUE;
//    m_fSenseRange = GREET_SENSE_RANGE;

    // set stretch factors for height and width
    const FLOAT fSize = 0.6f;
    GetModelObject()->StretchModel(FLOAT3D(fSize, fSize, fSize));
    ModelChangeNotify();
    StandingAnim();

    // continue behavior in base class
    jump CEnemyBase::MainLoop();
  };
};
