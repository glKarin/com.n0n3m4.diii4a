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

511
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/BasicEffects";
uses "EntitiesMP/Light";

// input parameter for launching the projectile
event EDevilProjectile {
  CEntityPointer penLauncher,     // who launched it
  CEntityPointer penTarget,       // target entity
};


%{
#define FLY_TIME  15.0f
#define ROTATE_SPEED 200.0f
#define MOVING_SPEED 30.0f
#define MOVING_FREQUENCY 0.1f
%}


class CDevilProjectile : CMovableModelEntity {
name      "Devil projectile";
thumbnail "";

properties:
  1 CEntityPointer m_penLauncher,     // who lanuched it
  2 CEntityPointer m_penTarget,       // target entity

 10 FLOAT m_fIgnoreTime = 0.0f,              // time when laucher will be ignored
 11 FLOAT m_fStartTime = 0.0f,               // start time when launched
 12 FLOAT3D m_vDesiredAngle = FLOAT3D(0,0,0),
 13 BOOL m_bFly = FALSE,

 20 CSoundObject m_soEffect,          // sound channel

{
  CLightSource m_lsLightSource;
}

components:
  1 class   CLASS_BASIC_EFFECT  "Classes\\BasicEffect.ecl",
  2 class   CLASS_LIGHT         "Classes\\Light.ecl",

// ********* PLAYER ROCKET *********
 10 model   MODEL_FLARE         "Models\\Enemies\\Devil\\Flare.mdl",
 11 texture TEXTURE_FLARE       "Models\\Enemies\\Devil\\12.tex",


functions:
  /* Read from stream. */
  void Read_t( CTStream *istr) // throw char *
  {
    CMovableModelEntity::Read_t(istr);
    // setup light source
    SetupLightSource();
  };

  /* Get static light source information. */
  CLightSource *GetLightSource(void)
  {
    if (!IsPredictor()) {
      return &m_lsLightSource;
    } else {
      return NULL;
    }
  };

  // Setup light source
  void SetupLightSource(void)
  {
    // setup light source
    CLightSource lsNew;
    lsNew.ls_ulFlags = LSF_NONPERSISTENT|LSF_DYNAMIC;
    lsNew.ls_rHotSpot = 0.0f;
    lsNew.ls_colColor = RGBToColor(0, 128, 128);
    lsNew.ls_rFallOff = 5.0f;
    lsNew.ls_plftLensFlare = NULL;
    lsNew.ls_ubPolygonalMask = 0;
    lsNew.ls_paoLightAnimation = NULL;

    m_lsLightSource.ls_penEntity = this;
    m_lsLightSource.SetLightSource(lsNew);
  };



/************************************************************
 *                     MOVING FUNCTIONS                     *
 ************************************************************/
  // calculate rotation
  void CalcHeadingRotation(ANGLE aWantedHeadingRelative, ANGLE &aRotation) {
    // normalize it to [-180,+180] degrees
    aWantedHeadingRelative = NormalizeAngle(aWantedHeadingRelative);

    // if desired position is left
    if (aWantedHeadingRelative < -ROTATE_SPEED*MOVING_FREQUENCY) {
      // start turning left
      aRotation = -ROTATE_SPEED;
    // if desired position is right
    } else if (aWantedHeadingRelative > ROTATE_SPEED*MOVING_FREQUENCY) {
      // start turning right
      aRotation = +ROTATE_SPEED;
    // if desired position is more-less ahead
    } else {
      aRotation = aWantedHeadingRelative/MOVING_FREQUENCY;
    }
  };

  // calculate angle from position
  void CalcAngleFromPosition() {
    // target enemy body
    FLOAT3D vTarget;
/*    EntityInfo *peiTarget = (EntityInfo*) (m_penTarget->GetEntityInfo());
    GetEntityInfoPosition(m_penTarget, peiTarget->vTargetCenter, vTarget);*/
    vTarget = m_penTarget->GetPlacement().pl_PositionVector;
    vTarget += FLOAT3D(m_penTarget->en_mRotation(1, 2),
                       m_penTarget->en_mRotation(2, 2),
                       m_penTarget->en_mRotation(3, 2)) * 2.0f;

    // find relative orientation towards the desired position
    m_vDesiredAngle = (vTarget - GetPlacement().pl_PositionVector).Normalize();
  };

  // rotate entity to desired angle
  void RotateToAngle() {
    // find relative heading towards the desired angle
    ANGLE aRotation;
    CalcHeadingRotation(GetRelativeHeading(m_vDesiredAngle), aRotation);

    // start rotating
    SetDesiredRotation(ANGLE3D(aRotation, 0, 0));
  };

  // fly move in direction
  void FlyInDirection() {
    RotateToAngle();

    // target enemy body
    FLOAT3D vTarget;
/*    EntityInfo *peiTarget = (EntityInfo*) (m_penTarget->GetEntityInfo());
    GetEntityInfoPosition(m_penTarget, peiTarget->vTargetCenter, vTarget);*/
    vTarget = m_penTarget->GetPlacement().pl_PositionVector;
    vTarget += FLOAT3D(m_penTarget->en_mRotation(1, 2),
                       m_penTarget->en_mRotation(2, 2),
                       m_penTarget->en_mRotation(3, 2)) * 2.0f;

    // determine translation speed
    FLOAT3D vTranslation = (vTarget - GetPlacement().pl_PositionVector) * !en_mRotation;
    vTranslation(1) = 0.0f;
    vTranslation.Normalize();
    vTranslation *= MOVING_SPEED;

    // start moving
    SetDesiredTranslation(vTranslation);
  };

  // fly entity to desired position
  void FlyToPosition() {
    CalcAngleFromPosition();
    FlyInDirection();
  };

  // rotate entity to desired position
  void RotateToPosition() {
    CalcAngleFromPosition();
    RotateToAngle();
  };

  // stop moving entity
  void StopMoving() {
    StopRotating();
    StopTranslating();
  };

  // stop rotating entity
  void StopRotating() {
    SetDesiredRotation(ANGLE3D(0, 0, 0));
  };

  // stop translating
  void StopTranslating() {
    SetDesiredTranslation(FLOAT3D(0.0f, 0.0f, 0.0f));
  };



/************************************************************
 *             C O M M O N   F U N C T I O N S              *
 ************************************************************/
  void ProjectileTouch(CEntityPointer penHit) {
    // direct damage
    FLOAT3D vDirection;
    AnglesToDirectionVector(GetPlacement().pl_OrientationAngle, vDirection);
    InflictDirectDamage(penHit, m_penLauncher, DMT_PROJECTILE, 15.0f,
               GetPlacement().pl_PositionVector, vDirection);
  };



/************************************************************
 *                   P R O C E D U R E S                    *
 ************************************************************/
procedures:
  Fly(EVoid) {
    // bounce loop
    m_bFly = TRUE;
    while(m_bFly && m_fStartTime+FLY_TIME > _pTimer->CurrentTick()) {
      wait(0.1f) {
        on (EBegin) : {
          FlyToPosition();
          resume;
        }
        on (EPass epass) : {
          BOOL bHit;
          // ignore launcher within 1 second
          bHit = epass.penOther!=m_penLauncher || _pTimer->CurrentTick()>m_fIgnoreTime;
          // ignore twister
          bHit &= !IsOfClass(epass.penOther, "Twister");
          if (bHit) {
            ProjectileTouch(epass.penOther);
            m_bFly = FALSE;
            stop;
          }
          resume;
        }
        on (ETouch etouch) : {
          // clear time limit for launcher
          m_fIgnoreTime = 0.0f;
          resume;
        }
        on (ETimer) : { stop; }
      }
    }
    return EEnd();
  };

  // --->>> MAIN
  Main(EDevilProjectile eLaunch) {
    // remember the initial parameters
    ASSERT(eLaunch.penLauncher!=NULL);
    ASSERT(eLaunch.penTarget!=NULL);
    m_penLauncher = eLaunch.penLauncher;
    m_penTarget = eLaunch.penTarget;

    // set appearance
    InitAsModel();
    SetPhysicsFlags(EPF_PROJECTILE_FLYING);
    SetCollisionFlags(ECF_PROJECTILE_MAGIC);
    SetModel(MODEL_FLARE);
    SetModelMainTexture(TEXTURE_FLARE);

    // setup light source
    SetupLightSource();

    // remember lauching time
    m_fIgnoreTime = _pTimer->CurrentTick() + 1.0f;

    // fly
    m_fStartTime = _pTimer->CurrentTick();
    autocall Fly() EEnd;

    // cease to exist
    Destroy();

    return;
  }
};
