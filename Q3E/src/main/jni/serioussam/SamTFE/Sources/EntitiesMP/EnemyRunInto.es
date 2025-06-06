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

312
%{
#include "EntitiesMP/StdH/StdH.h"
%}

uses "EntitiesMP/EnemyBase";


class export CEnemyRunInto : CEnemyBase {
name      "Enemy Run Into";
thumbnail "";

properties:
  1 CEntityPointer m_penLastTouched,       // last touched live entity
  2 FLOAT m_fLastTouchedTime = 0.0f,         // last touched live entity time
  3 BOOL  m_bWhileLoop = FALSE,               // internal for loops
  5 FLOAT m_fMassKicked = 0.0f,            // total mass kicked in one attack
  7 FLOAT m_fInertionRunTime = 1.3f,       // time to run before turning due to inertion
  8 FLOAT m_fStopApproachDistance = 6.75f, // at which distamce to start runnin away from enemy
  9 FLOAT m_fChargeDistance = 15.0f,       // at which distance to prepare for charge
 10 BOOL  m_bUseChargeAnimation = FALSE,   // does this monster use charging animation?
 // moving properties - CAN BE SET
 20 ANGLE m_fAttackRotateRunInto = 1.0f,  // attack rotate speed before run into enemy

components:
  1 class   CLASS_BASE    "Classes\\EnemyBase.ecl",

functions:
  virtual void AdjustDifficulty(void)
  {
    FLOAT fMoveSpeed = GetSP()->sp_fEnemyMovementSpeed;
    m_fAttackRotateRunInto *= fMoveSpeed;

    CEnemyBase::AdjustDifficulty();
  }
/************************************************************
 *                   ATTACK SPECIFIC                        *
 ************************************************************/
  void IncreaseKickedMass(CEntity *pen) {
    EntityInfo *peiTarget = (EntityInfo*) (pen->GetEntityInfo());
    if (peiTarget!=NULL) {
      m_fMassKicked += peiTarget->fMass;
    }
  };




/************************************************************
 *      VIRTUAL ATTACK FUNCTIONS THAT NEED OVERRIDE         *
 ************************************************************/
  // touched another live entity
  virtual void LiveEntityTouched(ETouch etouch) {};
  // touched entity with higher mass
  virtual BOOL HigherMass(void) { return FALSE; }
  virtual void ChargeAnim(void) {};


procedures:
/************************************************************
 *                 ATTACK ENEMY PROCEDURES                  *
 ************************************************************/
  // attack range -> fire and move toward enemy
  Fire() : CEnemyBase::Fire {
    m_fMassKicked = 0.0f;
    m_penLastTouched = NULL;

    jump RotateToEnemy();
  };


  RotateToEnemy() {
    // if the enemy not alive or deleted
    if (!(m_penEnemy->GetFlags()&ENF_ALIVE) || m_penEnemy->GetFlags()&ENF_DELETED) {
        SetTargetNone();
      return EReturn();
    }

    // rotate to enemy
    m_bWhileLoop = TRUE;
    while (m_penEnemy!=NULL && m_bWhileLoop) {
      m_fMoveFrequency = 0.1f;
      wait(m_fMoveFrequency) {
        // attack target
        on (EBegin) : {
          m_vDesiredPosition = m_penEnemy->GetPlacement().pl_PositionVector;
          // rotate to enemy
          if (!IsInPlaneFrustum(m_penEnemy, CosFast(15.0f))) {
            m_aRotateSpeed = m_fAttackRotateRunInto;
            m_fMoveSpeed = 0.0f;
            // adjust direction and speed
            ULONG ulFlags = SetDesiredMovement(); 
            MovementAnimation(ulFlags);
          } else {
            m_aRotateSpeed = 0.0f;
            m_fMoveSpeed = 0.0f;
            m_bWhileLoop = FALSE;
          }
          resume;
        }
        on (ESound) : { resume; }     // ignore all sounds
        on (EWatch) : { resume; }     // ignore watch
        on (ETimer) : { stop; }       // timer tick expire
      }
    }

    jump RunIntoEnemy();
  };


/*  WalkToEnemy() {
    // if the enemy not alive or deleted
    if (!(m_penEnemy->GetFlags()&ENF_ALIVE) || m_penEnemy->GetFlags()&ENF_DELETED) {
      SetTargetNone();
      return EReturn();
    }

    // walk to enemy if can't be seen
    while (!CanAttackEnemy(m_penEnemy, CosFast(30.0f))) {
      m_fMoveFrequency = 0.1f;
      wait(m_fMoveFrequency) {
        // move to target
        on (EBegin) : {
          m_vDesiredPosition = m_penEnemy->GetPlacement().pl_PositionVector;
          m_fMoveSpeed = m_fWalkSpeed;
          m_aRotateSpeed = m_aWalkRotateSpeed;
          // adjust direction and speed
          ULONG ulFlags = SetDesiredMovement(); 
          MovementAnimation(ulFlags);
        }
        on (ESound) : { resume; }     // ignore all sounds
        on (EWatch) : { resume; }     // ignore watch
        on (ETimer) : { stop; }       // timer tick expire
      }
    }

    jump StartRunIntoEnemy();
  };

  StartRunIntoEnemy() {
    // if the enemy not alive or deleted
    if (!(m_penEnemy->GetFlags()&ENF_ALIVE) || m_penEnemy->GetFlags()&ENF_DELETED) {
      SetTargetNone();
      return EReturn();
    }

    // run to enemy
    m_bWhileLoop = TRUE;
    m_fMoveFrequency = 0.5f;
    wait(m_fMoveFrequency) {
      on (EBegin) : {
        // if enemy can't be seen stop running
        if (!SeeEntity(m_penEnemy, CosFast(90.0f))) {
          m_bWhileLoop = FALSE;
          stop;
        }
        m_vDesiredPosition = m_penEnemy->GetPlacement().pl_PositionVector;
        // move to enemy
        m_fMoveSpeed = m_fAttackRunSpeed;
        m_aRotateSpeed = m_aAttackRotateSpeed;
        // adjust direction and speed
        SetDesiredMovement(); 
        RunningAnim();
        resume;
      }
      on (ETouch) : { resume; }
      on (ETimer) : { stop; }       // timer tick expire
    }

    jump RunIntoEnemy();
  };
  */

  RunIntoEnemy() {
    // if the enemy not alive or deleted
    if (!(m_penEnemy->GetFlags()&ENF_ALIVE) || m_penEnemy->GetFlags()&ENF_DELETED) {
      SetTargetNone();
      return EReturn();
    }

    // run to enemy
    m_bWhileLoop = TRUE;
    while(m_penEnemy!=NULL && m_bWhileLoop) {
      m_fMoveFrequency = 0.1f;
      wait(m_fMoveFrequency) {
        on (EBegin) : {
          // if enemy can't be seen, or too close
          if (!SeeEntity(m_penEnemy, CosFast(90.0f)) || CalcDist(m_penEnemy)<m_fStopApproachDistance) {
            // continue past it
            m_bWhileLoop = FALSE;
            stop;
          }
          // move to enemy
          m_fMoveSpeed = m_fAttackRunSpeed;
          m_aRotateSpeed = m_fAttackRotateRunInto;
          m_vDesiredPosition = m_penEnemy->GetPlacement().pl_PositionVector;
          SetDesiredMovement(); 
          if (m_bUseChargeAnimation && CalcDist(m_penEnemy)<m_fChargeDistance) {
            ChargeAnim();
          } else {
            RunningAnim();
          }
          // make fuss
          AddToFuss();
          resume;
        }
        // if touch another
        on (ETouch etouch) : {
          // if the entity is live
          if (etouch.penOther->GetFlags()&ENF_ALIVE) {
            // react to hitting it
            LiveEntityTouched(etouch);
            // if hit something bigger than us
            if (HigherMass()) {
              // stop attack
              m_penLastTouched = NULL;
              return EReturn();
            }
            // if hit the enemy
            if (etouch.penOther==m_penEnemy) {
              // continue past it
              m_bWhileLoop = FALSE;
              stop;
            }
          // if hit wall
          } else if (!(etouch.penOther->GetPhysicsFlags()&EPF_MOVABLE) &&
                     (FLOAT3D(etouch.plCollision)% -en_vGravityDir)<CosFast(50.0f)) {
            // stop run to enemy
            m_penLastTouched = NULL;
            return EReturn();
          }
          resume;
        }
        on (ETimer) : { stop; }       // timer tick expire
        on (EDeath) : { pass; }
        otherwise() : { resume; }
      }
    }
    jump RunAwayFromEnemy();
  };

  RunAwayFromEnemy() {
    // if the enemy not alive or deleted
    if (!(m_penEnemy->GetFlags()&ENF_ALIVE) || m_penEnemy->GetFlags()&ENF_DELETED) {
      SetTargetNone();
      return EReturn();
    }

    // run in direction due to inertion before turning 
    StopRotating();
    wait(m_fInertionRunTime) {
      on (EBegin) : { resume; }
      on (ETouch etouch) : {
        // live entity touched
        if (etouch.penOther->GetFlags()&ENF_ALIVE) {
          LiveEntityTouched(etouch);
          // stop moving on higher mass
          if (HigherMass()) {
            m_penLastTouched = NULL;
            return EReturn();
          }
        // stop go away from enemy
        } else if (!(etouch.penOther->GetPhysicsFlags()&EPF_MOVABLE) &&
                   (FLOAT3D(etouch.plCollision)% -en_vGravityDir)<CosFast(50.0f)) {
          m_penLastTouched = NULL;
          return EReturn();
        }
        resume;
      }
      on (ETimer) : { stop; }
      on (EDeath) : { pass; }
      otherwise() : { resume; }
    }

    m_penLastTouched = NULL;
    autocall PostRunAwayFromEnemy() EReturn;
    return EReturn();
  };

  PostRunAwayFromEnemy() {
    return EReturn();
  }

  // main loop
  MainLoop(EVoid) : CEnemyBase::MainLoop {
    jump CEnemyBase::MainLoop();
  };

  // dummy main
  Main(EVoid) {
    return;
  };
};
