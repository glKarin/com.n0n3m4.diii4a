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

313
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/EnemyBase";
uses "EntitiesMP/Debris";
uses "EntitiesMP/EnemyMarker";


enum EnemyDiveType {
  0 EDT_GROUND_ONLY         "Ground only",                    // can't dive
  1 EDT_DIVE_ONLY           "Dive only",                      // always dive can't walk
  2 EDT_GROUND_DIVE         "Ground and dive",                // dive and walk
};


class export CEnemyDive : CEnemyBase {
name      "Enemy Dive";
thumbnail "";

properties:
  1 enum EnemyDiveType m_EedtType "Type" 'T' = EDT_DIVE_ONLY, // dive type
  2 BOOL m_bInLiquid              "In liquid" 'Q' = TRUE,     // entity is in liquid

 // moving/attack properties - CAN BE SET
 // these following must be ordered exactly like this for GetProp() to function
 10 FLOAT m_fDiveWalkSpeed = 1.0f,                    // dive walk speed
 11 ANGLE m_aDiveWalkRotateSpeed = AngleDeg(10.0f),   // dive walk rotate speed
 12 FLOAT m_fDiveAttackRunSpeed = 1.0f,               // dive attack run speed
 13 ANGLE m_aDiveAttackRotateSpeed = AngleDeg(10.0f), // dive attack rotate speed
 14 FLOAT m_fDiveCloseRunSpeed = 1.0f,                // dive close run speed
 15 ANGLE m_aDiveCloseRotateSpeed = AngleDeg(10.0f),  // dive close rotate speed
 20 FLOAT m_fDiveAttackDistance = 50.0f,     // dive attack distance mode
 21 FLOAT m_fDiveCloseDistance = 10.0f,      // dive close distance mode
 22 FLOAT m_fDiveAttackFireTime = 2.0f,      // dive attack distance fire time
 23 FLOAT m_fDiveCloseFireTime = 1.0f,       // dive close distance fire time
 24 FLOAT m_fDiveStopDistance = 0.0f,        // dive stop moving toward enemy if closer than stop distance
 25 FLOAT m_fDiveIgnoreRange = 200.0f,       // dive cease attack if enemy farther
 26 FLOAT m_fDiveLockOnEnemyTime = 0.0f,     // dive time needed to fire

components:
  1 class   CLASS_BASE            "Classes\\EnemyBase.ecl",

functions:
  // overridable function for access to different properties of derived classes (flying/diving)
  virtual FLOAT &GetProp(FLOAT &m_fBase)
  {
    if (m_bInLiquid) {
      return *((&m_fBase)+(&m_fDiveWalkSpeed-&m_fWalkSpeed));
    } else {
      return m_fBase;
    }
  }

  // diving enemies never use pathfinding
  void StartPathFinding(void)
  {
    m_dtDestination=DT_PLAYERSPOTTED;
    m_vPlayerSpotted = PlayerDestinationPos();
  }

  virtual void AdjustDifficulty(void)
  {
    FLOAT fMoveSpeed = GetSP()->sp_fEnemyMovementSpeed;
    FLOAT fAttackSpeed = GetSP()->sp_fEnemyMovementSpeed;
//    m_fDiveWalkSpeed *= fMoveSpeed;
//    m_aDiveWalkRotateSpeed *= fMoveSpeed;
    m_fDiveAttackRunSpeed *= fMoveSpeed;
    m_aDiveAttackRotateSpeed *= fMoveSpeed;
    m_fDiveCloseRunSpeed *= fMoveSpeed;
    m_aDiveCloseRotateSpeed *= fMoveSpeed;
    m_fDiveAttackFireTime *= 1/fAttackSpeed;
    m_fDiveCloseFireTime *= 1/fAttackSpeed;
    m_fDiveLockOnEnemyTime *= 1/fAttackSpeed;

    CEnemyBase::AdjustDifficulty();
  }
  // close attack if possible
  virtual BOOL CanHitEnemy(CEntity *penTarget, FLOAT fCosAngle) {
    if (IsInPlaneFrustum(penTarget, fCosAngle)) {
      return IsVisibleCheckAll(penTarget);
    }
    return FALSE;
  };
/************************************************************
 *                        POST MOVING                       *
 ************************************************************/
  void PostMoving(void) {
    CEnemyBase::PostMoving();
    // change to liquid
    if (m_EedtType!=EDT_GROUND_ONLY && !m_bInLiquid && en_fImmersionFactor>0.9f &&
        (GetWorld()->wo_actContentTypes[en_iDnContent].ct_ulFlags&CTF_SWIMABLE)) {
      m_bInLiquid = TRUE;
      ChangeCollisionToLiquid();
      SendEvent(ERestartAttack());
    }
    // change to ground
    if (m_EedtType!=EDT_DIVE_ONLY && m_bInLiquid && (en_fImmersionFactor<0.5f || en_fImmersionFactor==1.0f) &&
        en_penReference!=NULL && !(GetWorld()->wo_actContentTypes[en_iUpContent].ct_ulFlags&CTF_SWIMABLE)) {
      m_bInLiquid = FALSE;
      ChangeCollisionToGround();
      SendEvent(ERestartAttack());
    }
  };



/************************************************************
 *                     MOVING FUNCTIONS                     *
 ************************************************************/
  // set desired rotation and translation to go/orient towards desired position
  // and get the resulting movement type
  virtual ULONG SetDesiredMovement(void) 
  {
    // if not in air
    if (!m_bInLiquid) {
      // use base class
      return CEnemyBase::SetDesiredMovement();
    }

    // get base rotation from base class
    ULONG ulFlags = CEnemyBase::SetDesiredMovement();

    // if we may move
    if (m_fMoveSpeed>0.0f) {
      // fix translation for 3d movement
      FLOAT3D vTranslation = (m_vDesiredPosition - GetPlacement().pl_PositionVector) * !en_mRotation;
      vTranslation(1) = 0.0f;
      if (vTranslation(3)>0) { 
        vTranslation(3) = 0.0f;
      }
      vTranslation.Normalize();
      vTranslation *= m_fMoveSpeed;
      SetDesiredTranslation(vTranslation);
    }

    return ulFlags;
  }

  // check whether may move while attacking
  BOOL MayMoveToAttack(void) 
  {
    return WouldNotLeaveAttackRadius();
  }

/************************************************************
 *                CLASS SUPPORT FUNCTIONS                   *
 ************************************************************/
  // set entity position
  void SetEntityPosition() {
    switch (m_EedtType) {
      case EDT_GROUND_ONLY:         // can't dive
        m_bInLiquid = FALSE;
        break;
      case EDT_DIVE_ONLY:           // always dive can't walk
        m_bInLiquid = TRUE;
        break;
      case EDT_GROUND_DIVE:         // dive and walk
        break;
    }

    // in liquid
    if (m_bInLiquid) {
      ChangeCollisionToLiquid();
    } else {
      ChangeCollisionToGround();
    }

    StandingAnim();
  };


/************************************************************
 *          VIRTUAL FUNCTIONS THAT NEED OVERRIDE            *
 ************************************************************/
  virtual void ChangeCollisionToLiquid(void) {}
  virtual void ChangeCollisionToGround(void) {}



procedures:
/************************************************************
 *      PROCEDURES WHEN NO ANY SPECIAL ACTION               *
 ************************************************************/
/************************************************************
 *                 ATTACK ENEMY PROCEDURES                  *
 ************************************************************/
  // this is called to hit the player when near
  Hit(EVoid) : CEnemyBase::Hit
  { 
    if (m_bInLiquid) {
      jump DiveHit();
    } else {
      jump GroundHit();
    }
  }

  // this is called to shoot at player when far away or otherwise unreachable
  Fire(EVoid) : CEnemyBase::Fire
  { 
    if (m_bInLiquid) {
      jump DiveFire();
    } else {
      jump GroundFire();
    }
  }

/************************************************************
 *                M  A  I  N    L  O  O  P                  *
 ************************************************************/
  // main loop
  MainLoop(EVoid) : CEnemyBase::MainLoop {
    SetEntityPosition();
    jump CEnemyBase::MainLoop();
  };

  // dummy main
  Main(EVoid) {
    return;
  };



/************************************************************
 *          VIRTUAL PROCEDURES THAT NEED OVERRIDE           *
 ************************************************************/
  // this is called to hit the player when near and you are on ground
  GroundHit(EVoid) 
  { 
    return EReturn(); 
  }
  // this is called to shoot at player when far away or otherwise unreachable and you are on ground
  GroundFire(EVoid) 
  { 
    return EReturn(); 
  }

  // this is called to hit the player when near and you are in water
  DiveHit(EVoid) 
  { 
    return EReturn(); 
  }

  // this is called to shoot at player when far away or otherwise unreachable and you are in water
  DiveFire(EVoid) 
  { 
    return EReturn(); 
  }
};
