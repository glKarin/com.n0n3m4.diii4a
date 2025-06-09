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

507
%{
#include "Entities/StdH/StdH.h"

#define ECF_TWISTER ( \
  ((ECBI_BRUSH|ECBI_MODEL|ECBI_CORPSE|ECBI_ITEM|ECBI_PROJECTILE_MAGIC|ECBI_PROJECTILE_SOLID)<<ECB_TEST) |\
  ((ECBI_MODEL|ECBI_CORPSE|ECBI_ITEM|ECBI_PROJECTILE_MAGIC|ECBI_PROJECTILE_SOLID)<<ECB_PASS) |\
  ((ECBI_MODEL)<<ECB_IS))
#define EPF_TWISTER ( \
  EPF_ONBLOCK_CLIMBORSLIDE|EPF_ORIENTEDBYGRAVITY|\
  EPF_TRANSLATEDBYGRAVITY|EPF_MOVABLE)
%}


uses "Entities/Elemental";


enum TwisterSize {
  0 TWS_SMALL       "",     // small twister
  1 TWS_BIG         "",     // big twister
  2 TWS_LARGE       "",     // large twister
};


// input parameter for twister
event ETwister {
  CEntityPointer penOwner,        // entity which owns it
  enum TwisterSize EtsSize,       // twister size
};


%{
static EntityInfo eiTwister = {
  EIBT_AIR, 0.0f,
  0.0f, 1.0f, 0.0f,
  0.0f, 0.75f, 0.0f,
};


#define MOVE_FREQUENCY 0.1f
#define ROTATE_SPEED 10000.0f
#define MOVE_SPEED 7.5f
%}

class CTwister : CMovableModelEntity {
name      "Twister";
thumbnail "";

properties:
  1 CEntityPointer m_penOwner,                  // entity which owns it
  2 enum TwisterSize m_EtsSize = TWS_SMALL,     // size (type)
  3 FLOAT3D m_vStartPosition = FLOAT3D(0,0,0),  // start position

 // internal -> do not use
 10 FLOAT3D m_vDesiredPosition = FLOAT3D(0,0,0),
 11 FLOAT3D m_vDesiredAngle = FLOAT3D(0,0,0),
 12 FLOAT m_fStopTime = 0.0f,
 13 FLOAT m_fActionRadius = 0.0f,
 14 FLOAT m_fActionTime = 0.0f,
 15 FLOAT m_fDiffMultiply = 0.0f,
 16 FLOAT m_fUpMultiply = 0.0f,
 20 BOOL m_bFadeOut = FALSE,
 21 FLOAT m_fFadeStartTime = 0.0f,
 22 FLOAT m_fFadeTime = 2.0f,

components:
// ********* TWISTER *********
 10 model   MODEL_TWISTER         "Models\\Enemies\\Elementals\\Twister.mdl",
 11 texture TEXTURE_TWISTER       "Models\\Enemies\\Elementals\\AirMan01.tex",

functions:
  /* Entity info */
  void *GetEntityInfo(void) {
    return &eiTwister;
  };

  /* Receive damage */
  void ReceiveDamage(CEntity *penInflictor, enum DamageType dmtType,
    FLOAT fDamageAmmount, const FLOAT3D &vHitPoint, const FLOAT3D &vDirection) 
  {
    return;
  };



/************************************************************
 *                        FADE OUT                          *
 ************************************************************/
  BOOL AdjustShadingParameters(FLOAT3D &vLightDirection, COLOR &colLight, COLOR &colAmbient) {
    if (m_bFadeOut) {
      FLOAT fTimeRemain = m_fFadeStartTime + m_fFadeTime - _pTimer->CurrentTick();
      if (fTimeRemain < 0.0f) { fTimeRemain = 0.0f; }
      COLOR colAlpha = GetModelObject()->mo_colBlendColor;
      colAlpha = (colAlpha&0xffffff00) + (COLOR(fTimeRemain/m_fFadeTime*0xff)&0xff);
      GetModelObject()->mo_colBlendColor = colAlpha;
    }
    return CMovableModelEntity::AdjustShadingParameters(vLightDirection, colLight, colAmbient);
  };



/************************************************************
 *                     MOVING FUNCTIONS                     *
 ************************************************************/
  // calculate rotation
  void CalcHeadingRotation(ANGLE aWantedHeadingRelative, ANGLE &aRotation) {
    // normalize it to [-180,+180] degrees
    aWantedHeadingRelative = NormalizeAngle(aWantedHeadingRelative);

    // if desired position is left
    if (aWantedHeadingRelative < -ROTATE_SPEED*MOVE_FREQUENCY) {
      // start turning left
      aRotation = -ROTATE_SPEED;
    // if desired position is right
    } else if (aWantedHeadingRelative > ROTATE_SPEED*MOVE_FREQUENCY) {
      // start turning right
      aRotation = +ROTATE_SPEED;
    // if desired position is more-less ahead
    } else {
      aRotation = aWantedHeadingRelative/MOVE_FREQUENCY;
    }
  };

  // calculate angle from position
  void CalcAngleFromPosition() {
    // find relative orientation towards the desired position
    m_vDesiredAngle = (m_vDesiredPosition - GetPlacement().pl_PositionVector).Normalize();
  };

  // rotate entity to desired angle
  void RotateToAngle() {
    // find relative heading towards the desired angle
    ANGLE aRotation;
    CalcHeadingRotation(GetRelativeHeading(m_vDesiredAngle), aRotation);

    // start rotating
    SetDesiredRotation(ANGLE3D(aRotation, 0, 0));
  };

  // move in direction
  void MoveInDirection() {
    RotateToAngle();

    // determine translation speed
    FLOAT3D vTranslation(0.0f, 0.0f, 0.0f);
    vTranslation(3) = -MOVE_SPEED;

    // start moving
    SetDesiredTranslation(vTranslation);
  };

  // move entity to desired position
  void MoveToPosition() {
    CalcAngleFromPosition();
    MoveInDirection();
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
 *                   ATTACK SPECIFIC                        *
 ************************************************************/
  void SpinEntity(CEntity *pen) {
    // don't spin air elemental and another twister
    if (!(IsOfClass(pen, "Elemental") && ((CElemental&)*pen).m_EetType==ELT_AIR) &&
        !(IsOfClass(pen, "Twister"))) {
      if (pen->GetPhysicsFlags()&EPF_MOVABLE) {
        // throw target away
        FLOAT3D vSpeed;
        vSpeed = FLOAT3D(en_mRotation(1, 2), en_mRotation(2, 2), en_mRotation(3, 2)) * m_fUpMultiply;
        FLOAT3D vDiff = (pen->GetPlacement().pl_PositionVector - GetPlacement().pl_PositionVector).Normalize();
        vSpeed += (vDiff*m_fDiffMultiply) * pen->en_mRotation;
        // give absolute speed
        ((CMovableEntity&)*pen).en_vCurrentTranslationAbsolute = vSpeed;
        ((CMovableEntity&)*pen).en_aDesiredRotationRelative = ANGLE3D(180.0f, 0, 0);
        // damage
        FLOAT3D vDirection;
        AnglesToDirectionVector(GetPlacement().pl_OrientationAngle, vDirection);
        InflictDirectDamage(pen, m_penOwner, DMT_IMPACT, 0.1f, GetPlacement().pl_PositionVector, vDirection);
      }
    }
  };



/************************************************************
 *                   P R O C E D U R E S                    *
 ************************************************************/
procedures:
  // --->>> MAIN
  Main(ETwister et) {
    // remember the initial parameters
    ASSERT(et.penOwner!=NULL);
    m_penOwner = et.penOwner;
    m_EtsSize = et.EtsSize;

    // initialization
    InitAsModel();
    SetPhysicsFlags(EPF_TWISTER);
    SetCollisionFlags(ECF_TWISTER);
    SetFlags(GetFlags() | ENF_SEETHROUGH);
    SetModel(MODEL_TWISTER);
    SetModelMainTexture(TEXTURE_TWISTER);

    // setup
    switch(m_EtsSize) {
      case TWS_SMALL:
        m_fActionRadius = 10;
        m_fActionTime = 10;
        m_fUpMultiply = 5.0f;
        m_fDiffMultiply = 1.0f;
        break;
      case TWS_BIG:
        m_fActionRadius = 15;
        m_fActionTime = 20;
        m_fUpMultiply = 10.0f;
        m_fDiffMultiply = 4.0f;
        GetModelObject()->StretchModel(FLOAT3D(4.0f, 4.0f, 4.0f));
        break;
      case TWS_LARGE:
        m_fActionRadius = 20;
        m_fActionTime = 30;
        m_fUpMultiply = 20.0f;
        m_fDiffMultiply = 16.0f;
        GetModelObject()->StretchModel(FLOAT3D(16.0f, 16.0f, 16.0f));
        break;
    }
    ModelChangeNotify();
    
    // start position
    m_vStartPosition = GetPlacement().pl_PositionVector;

    // move in range
    m_fStopTime = _pTimer->CurrentTick() + 10.0f;
    while(_pTimer->CurrentTick() < m_fStopTime) {
      // new destination
      FLOAT fR = FRnd()*10.0f;
      FLOAT fA = FRnd()*360.0f;
      m_vDesiredPosition = m_vStartPosition + FLOAT3D(CosFast(fA)*fR, 0, SinFast(fA)*fR);
      while((m_vDesiredPosition - GetPlacement().pl_PositionVector).Length() > MOVE_SPEED*MOVE_FREQUENCY*2.0f &&
            _pTimer->CurrentTick() < m_fStopTime) {
        MoveToPosition();
        wait(MOVE_FREQUENCY) {
          on (EBegin) : { resume; }
          on (ETimer) : { stop; }
          on (EPass ep) : {
            if (ep.penOther->GetRenderType()&RT_MODEL &&
                ep.penOther->GetPhysicsFlags()&EPF_MOVABLE) {
              SpinEntity(ep.penOther);
            }
            resume;
          }
        }
      }
    }

    // fade out
    m_fFadeStartTime = _pTimer->CurrentTick();
    m_bFadeOut = TRUE;
    m_fFadeTime = 2.0f;
    autowait(m_fFadeTime);

    // cease to exist
    Destroy();

    return;
  }
};
