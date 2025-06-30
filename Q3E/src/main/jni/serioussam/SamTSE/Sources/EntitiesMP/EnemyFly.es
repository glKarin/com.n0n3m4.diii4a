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

311
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/EnemyBase";
uses "EntitiesMP/Debris";
uses "EntitiesMP/EnemyMarker";


enum EnemyFlyType {
  0 EFT_GROUND_ONLY       "Ground only",                  // can't fly
  1 EFT_FLY_ONLY          "Fly only",                     // always fly can't land
  2 EFT_FLY_GROUND_GROUND "Fly(ground) - ground attack",  // start attack on ground - ground
  3 EFT_FLY_GROUND_AIR    "Fly(ground) - air attack",     // start attack in air - ground
  4 EFT_FLY_AIR_GROUND    "Fly(air) - ground attack",     // start attack on ground - air
  5 EFT_FLY_AIR_AIR       "Fly(air) - air attack",        // start attack in air - air
};


class export CEnemyFly : CEnemyBase {
name      "Enemy Fly";
thumbnail "";

properties:
  1 enum EnemyFlyType m_EeftType "Type" 'T' = EFT_FLY_GROUND_AIR,   // fly type
  2 BOOL m_bInAir = FALSE,       // entity is in air
  3 BOOL m_bAirAttack = FALSE,   // start air attack
  4 BOOL m_bStartInAir = FALSE,  // initially in air

 // moving/attack properties - CAN BE SET
 16 FLOAT m_fGroundToAirSpeed = 2.0f,                 // ground to air speed
 17 FLOAT m_fAirToGroundSpeed = 4.0f,                 // air to ground speed
 18 FLOAT m_fAirToGroundMin = 1.0f,     // how long to fly up before attacking
 19 FLOAT m_fAirToGroundMax = 2.0f,
 27 FLOAT m_fFlyHeight = 2.0f,              // fly height above player handle

 // these following must be ordered exactly like this for GetProp() to function
 10 FLOAT m_fFlyWalkSpeed = 1.0f,                     // fly walk speed
 11 ANGLE m_aFlyWalkRotateSpeed = AngleDeg(10.0f),    // fly walk rotate speed
 12 FLOAT m_fFlyAttackRunSpeed = 1.0f,                // fly attack run speed
 13 ANGLE m_aFlyAttackRotateSpeed = AngleDeg(10.0f),  // fly attack rotate speed
 14 FLOAT m_fFlyCloseRunSpeed = 1.0f,                 // fly close run speed
 15 ANGLE m_aFlyCloseRotateSpeed = AngleDeg(10.0f),   // fly close rotate speed
 20 FLOAT m_fFlyAttackDistance = 50.0f,     // fly attack distance mode
 21 FLOAT m_fFlyCloseDistance = 10.0f,      // fly close distance mode
 22 FLOAT m_fFlyAttackFireTime = 2.0f,      // fly attack distance fire time
 23 FLOAT m_fFlyCloseFireTime = 1.0f,       // fly close distance fire time
 24 FLOAT m_fFlyStopDistance = 0.0f,        // fly stop moving toward enemy if closer than stop distance
 25 FLOAT m_fFlyIgnoreRange = 200.0f,       // fly cease attack if enemy farther
 26 FLOAT m_fFlyLockOnEnemyTime = 0.0f,     // fly time needed to fire

 // marker variables
100 BOOL m_bFlyToMarker = FALSE,

components:
  1 class   CLASS_BASE            "Classes\\EnemyBase.ecl",

functions:
  // overridable function for access to different properties of derived classes (flying/diving)
  virtual FLOAT &GetProp(FLOAT &m_fBase)
  {
    if (m_bInAir) {
      return *((&m_fBase)+(&m_fFlyWalkSpeed-&m_fWalkSpeed));
    } else {
      return m_fBase;
    }
  }

  // get position you would like to go to when following player
  virtual FLOAT3D PlayerDestinationPos(void)
  {
    // if not in air
    if (!m_bInAir) {
      // use base class
      return CEnemyBase::PlayerDestinationPos();
    }

    // get distance to player
    FLOAT fDist = CalcDist(m_penEnemy);
    // determine height above from the distance
    FLOAT fHeight;
    // if in close attack range
    if (fDist<=m_fFlyCloseDistance) {
      // go to fixed height above player
      fHeight = m_fFlyHeight;
    // if in further
    } else {
      // fly more above if further
      fHeight = m_fFlyHeight + fDist/5.0f;
    }

    // calculate the position from the height
    return 
      m_penEnemy->GetPlacement().pl_PositionVector
      + FLOAT3D(m_penEnemy->en_mRotation(1, 2), m_penEnemy->en_mRotation(2, 2), m_penEnemy->en_mRotation(3, 2))
        * fHeight;
  }

  // flying enemies never use pathfinding
  void StartPathFinding(void)
  {
    if (m_bInAir) {
      m_dtDestination=DT_PLAYERSPOTTED;
      m_vPlayerSpotted = PlayerDestinationPos();
    } else {
      CEnemyBase::StartPathFinding();
    }
  }

  virtual void AdjustDifficulty(void)
  {
    FLOAT fMoveSpeed = GetSP()->sp_fEnemyMovementSpeed;
    FLOAT fAttackSpeed = GetSP()->sp_fEnemyMovementSpeed;

    m_fFlyAttackFireTime  *= 1/fAttackSpeed;
    m_fFlyCloseFireTime  *= 1/fAttackSpeed;
    m_fFlyLockOnEnemyTime  *= 1/fAttackSpeed;
//    m_fFlyWalkSpeed *= fMoveSpeed;
//    m_aFlyWalkRotateSpeed *= fMoveSpeed;
    m_fFlyAttackRunSpeed *= fMoveSpeed;
    m_aFlyAttackRotateSpeed *= fMoveSpeed;
    m_fFlyCloseRunSpeed *= fMoveSpeed;
    m_aFlyCloseRotateSpeed *= fMoveSpeed;
    m_fGroundToAirSpeed *= fMoveSpeed;
    m_fAirToGroundSpeed *= fMoveSpeed;

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
 *                     MOVING FUNCTIONS                     *
 ************************************************************/

  // set desired rotation and translation to go/orient towards desired position
  // and get the resulting movement type
  virtual ULONG SetDesiredMovement(void) 
  {
    // if not in air
    if (!m_bInAir) {
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

/************************************************************
 *                CLASS SUPPORT FUNCTIONS                   *
 ************************************************************/
  // set entity position
  void SetEntityPosition() {
    switch (m_EeftType) {
      case EFT_GROUND_ONLY:         // ground only enemy
      case EFT_FLY_GROUND_GROUND:   // fly, on ground, start attack on ground
        m_bAirAttack = FALSE;
        m_bStartInAir = m_bInAir = FALSE;
        m_bFlyToMarker = FALSE;
        SetPhysicsFlags((GetPhysicsFlags() & ~EPF_MODEL_FLYING) | EPF_MODEL_WALKING);
        ChangeCollisionToGround();
        break;

      case EFT_FLY_GROUND_AIR:      // fly, on ground, start attack in air
        m_bAirAttack = TRUE;
        m_bStartInAir = m_bInAir = FALSE;
        m_bFlyToMarker = FALSE;
        SetPhysicsFlags((GetPhysicsFlags() & ~EPF_MODEL_FLYING) | EPF_MODEL_WALKING);
        ChangeCollisionToGround();
        break;

      case EFT_FLY_AIR_GROUND:      // fly, in air, start attack on ground
        m_bAirAttack = FALSE;
        m_bStartInAir = m_bInAir = TRUE;
        m_bFlyToMarker = TRUE;
        SetPhysicsFlags((GetPhysicsFlags() & ~EPF_MODEL_WALKING) | EPF_MODEL_FLYING);
        ChangeCollisionToAir();
        break;

      case EFT_FLY_ONLY:            // air only enemy
      case EFT_FLY_AIR_AIR:         // fly, in air, start attack in air
        m_bAirAttack = TRUE;
        m_bStartInAir = m_bInAir = TRUE;
        m_bFlyToMarker = TRUE;
        SetPhysicsFlags((GetPhysicsFlags() & ~EPF_MODEL_WALKING) | EPF_MODEL_FLYING);
        ChangeCollisionToAir();
        break;
    }
    StandingAnim();
  };


/************************************************************
 *          VIRTUAL FUNCTIONS THAT NEED OVERRIDE            *
 ************************************************************/
  virtual FLOAT AirToGroundAnim(void) { return _pTimer->TickQuantum; }
  virtual FLOAT GroundToAirAnim(void) { return _pTimer->TickQuantum; }
  virtual void ChangeCollisionToAir(void) {}
  virtual void ChangeCollisionToGround(void) {}



procedures:
/************************************************************
 *      PROCEDURES WHEN NO ANY SPECIAL ACTION               *
 ************************************************************/
/*
#### !!!!
  MoveToDestinationFlying(EVoid) {
    m_fMoveFrequency = 0.25f;
    m_fMoveTime = _pTimer->CurrentTick() + 45.0f;
    while ((m_vDesiredPosition-GetPlacement().pl_PositionVector).Length()>m_fMoveSpeed*m_fMoveFrequency*2.0f &&
            m_fMoveTime>_pTimer->CurrentTick()) {
      wait (m_fMoveFrequency) {
        on (EBegin) : { FlyToPosition(); }
        on (ETimer) : { stop; }
      }
    }
    return EReturn();
  }
  // Move to destination
  MoveToDestination(EVoid) : CEnemyBase::MoveToDestination {
    if (m_bFlyToMarker && !m_bInAir) {
      autocall GroundToAir() EReturn;
    } else if (!m_bFlyToMarker && m_bInAir) {
      autocall AirToGround() EReturn;
    }

    // animation
    if (m_bRunToMarker) {
      RunningAnim();
    } else {
      WalkingAnim();
    }
    // fly to position
    if (m_bInAir) {
      jump MoveToDestinationFlying();
    // move to position
    } else {
      jump CEnemyBase::MoveToDestination();
    }
  };*/

  // return to start position
  ReturnToStartPosition(EVoid) : CEnemyBase::ReturnToStartPosition
  {
    jump CEnemyBase::BeIdle();
/*    // if on ground, but can fly
    if (!m_bInAir && m_EeftType!=EFT_GROUND_ONLY) {
      // fly up
      autocall GroundToAir() EReturn;
    }

    GetWatcher()->StartPlayers();     // start watching
    m_vDesiredPosition = m_vStartPosition-en_vGravityDir*2.0f;
    m_vStartDirection = (GetPlacement().pl_PositionVector-m_vStartPosition).Normalize();
    m_fMoveSpeed = m_fAttackRunSpeed;
    m_aRotateSpeed = m_aAttackRotateSpeed;
    RunningAnim();
    autocall MoveToDestinationFlying() EReturn;

    WalkingAnim();
    m_vDesiredAngle = m_vStartDirection;
    StopTranslating();

    autocall CEnemyBase::RotateToStartDirection() EReturn;

    // if should be on ground
    if (m_bInAir && !m_bStartInAir) {
      // fly down
      autocall AirToGround() EReturn;
    }
    StopMoving();
    StandingAnim();

    jump CEnemyBase::BeIdle();*/
  };

/************************************************************
 *                PROCEDURES WHEN HARMED                    *
 ************************************************************/
  // Play wound animation and falling body part
  BeWounded(EDamage eDamage) : CEnemyBase::BeWounded {
    // land on ground
    if (!(m_EeftType!=EFT_FLY_ONLY && m_bInAir && ((IRnd()&3)==0))) {
      jump CEnemyBase::BeWounded(eDamage);
    } else if (TRUE) {
      m_bAirAttack = FALSE;
      autocall AirToGround() EReturn;
    }
    return EReturn();
  };



/************************************************************
 *                 AIR - GROUND PROCEDURES                  *
 ************************************************************/
  // air to ground
  AirToGround(EVoid) 
  {
    // land on brush
    SetDesiredTranslation(FLOAT3D(0, -m_fAirToGroundSpeed, 0));
    SetDesiredRotation(ANGLE3D(0, 0, 0));
    WalkingAnim();
    wait() {
      on (EBegin) : { resume; }
      // on brush stop
      on (ETouch etouch) : {
        if (etouch.penOther->GetRenderType() & RT_BRUSH) {
          SetDesiredTranslation(FLOAT3D(0, 0, 0));
          stop;
        }
        resume;
      }
      on (EDeath) : { pass; }
      otherwise() : { resume; }
    }
    // on ground
    SetPhysicsFlags((GetPhysicsFlags() & ~EPF_MODEL_FLYING) | EPF_MODEL_WALKING);
    m_bInAir = FALSE;
    ChangeCollisionToGround();
    // animation
    wait(AirToGroundAnim()) {
      on (EBegin) : { resume; }
      on (ETimer) : { stop; }
      on (EDeath) : { pass; }
      otherwise() : { resume; }
    }
    return EReturn();
  };

  // ground to air
  GroundToAir(EVoid) 
  {
    // fly in air
    SetPhysicsFlags((GetPhysicsFlags() & ~EPF_MODEL_WALKING) | EPF_MODEL_FLYING);
    m_bInAir = TRUE;
    SetDesiredTranslation(FLOAT3D(0, m_fGroundToAirSpeed, 0));
    SetDesiredRotation(ANGLE3D(0, 0, 0));
    ChangeCollisionToAir();
    // animation
    wait(GroundToAirAnim()) {
      on (EBegin) : { resume; }
      on (EDeath) : { pass; }
      on (ETimer) : { stop; }
      otherwise() : { resume; }
    }
    // move in air further
    WalkingAnim();
    wait(Lerp(m_fAirToGroundMin, m_fAirToGroundMax, FRnd())) {
      on (EBegin) : { resume; }
      on (EDeath) : { pass; }
      on (ETimer) : { stop; }
      otherwise() : { resume; }
    }
    SetDesiredTranslation(FLOAT3D(0, 0, 0));
    return EReturn();
  };



/************************************************************
 *                 ATTACK ENEMY PROCEDURES                  *
 ************************************************************/
  // initialize attack is overridden to switch fly/walk modes if needed
  AttackEnemy() : CEnemyBase::AttackEnemy
  {
    // air attack
    if (m_bAirAttack) {
      // ground to air
      if (!m_bInAir) {
        autocall GroundToAir() EReturn;
      }
    // ground attack
    } else if (TRUE) {
      // air to ground
      if (m_bInAir) {
        autocall AirToGround() EReturn;
      }
    }

    jump CEnemyBase::AttackEnemy();
  }

  // this is called to hit the player when near
  Hit(EVoid) : CEnemyBase::Hit
  { 
    if (m_bInAir) {
      jump FlyHit();
    } else {
      jump GroundHit();
    }
  }

  // this is called to shoot at player when far away or otherwise unreachable
  Fire(EVoid) : CEnemyBase::Fire
  { 
    if (m_bInAir) {
      jump FlyFire();
    } else {
      jump GroundFire();
    }
  }

/************************************************************
 *                    D  E  A  T  H                         *
 ************************************************************/
  Death(EVoid) : CEnemyBase::Death {
    // clear fly flag
    SetPhysicsFlags((GetPhysicsFlags() & ~EPF_MODEL_FLYING) | EPF_MODEL_WALKING);
    ChangeCollisionToGround();
    jump CEnemyBase::Death();
  };



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

  // this is called to hit the player when near and you are in air
  FlyHit(EVoid) 
  { 
    return EReturn(); 
  }

  // this is called to shoot at player when far away or otherwise unreachable and you are in air
  FlyFire(EVoid) 
  { 
    return EReturn(); 
  }
};
