/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "UnreachableTargetState.h"
#include "../Memory.h"
#include "../../AIComm_Message.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Tasks/ThrowObjectTask.h"
#include "../Tasks/MoveToPositionTask.h"
#include "LostTrackOfEnemyState.h"
#include "TakeCoverState.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& UnreachableTargetState::GetName() const
{
	static idStr _name(STATE_UNREACHABLE_TARGET);
	return _name;
}

void UnreachableTargetState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("UnreachableTargetState initialised.\r");
	assert(owner);

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	idActor* enemy = owner->GetEnemy();

	if (!enemy)
	{
		owner->GetMind()->SwitchState(STATE_LOST_TRACK_OF_ENEMY);
		return;
	}

	_enemy = enemy;

	// This checks if taking cover is possible and enabled for this AI
	// grayman #3507 - This is being done too early. It needs to be checked
	// after a rock is thrown (if rock-throwing is enabled).
	// Move this to Think().
/*	_takingCoverPossible = false;
	if (owner->spawnArgs.GetBool("taking_cover_enabled","0"))
	{
		aasGoal_t hideGoal;
		 // grayman #3280 - enemies look with their eyes, not their feet
		_takingCoverPossible = owner->LookForCover(hideGoal, enemy, enemy->GetEyePosition());
		if (_takingCoverPossible)
		{
			// We should not go into TakeCoverState if we are already at a suitable position

			if (hideGoal.origin == owner->GetPhysics()->GetOrigin() )
			{
				_takingCoverPossible = false;
			}
		}
	}
*/
	_takeCoverTime = -1;

	// Fill the subsystems with their tasks

	// Create the message
	CommMessagePtr message(new CommMessage(
		CommMessage::RequestForMissileHelp_CommType, 
		owner, NULL, // from this AI to anyone 
		enemy,
		memory.lastEnemyPos,
		-1
	));

	// grayman #3343 - accommodate different barks for human and non-human enemies

	idStr bark = "";
	idStr enemyAiUse = enemy->spawnArgs.GetString("AIUse");
	if ( ( enemyAiUse == AIUSE_MONSTER ) || ( enemyAiUse == AIUSE_UNDEAD ) )
	{
		bark = "snd_cantReachTargetMonster";
	}
	else
	{
		bark = "snd_cantReachTarget";
	}
	owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask(bark, message)));

	if (cv_ai_debug_transition_barks.GetBool())
	{
		gameLocal.Printf("%d: %s can't reach the target, barks '%s'\n",gameLocal.time,owner->GetName(),bark.c_str());
	}

	// The sensory system does nothing so far
	owner->senseSubsystem->ClearTasks();

	owner->StopMove(MOVE_STATUS_DONE);
	owner->movementSubsystem->ClearTasks();
	memory.StopReacting(); // grayman #3559

	owner->actionSubsystem->ClearTasks();

	_moveRequired = false;

	if (owner->spawnArgs.GetBool("outofreach_projectile_enabled", "0"))
	{
		// Check the distance between AI and the enemy. If it is too large, try to move closer.
		// Start throwing objects if we are close enough
		idVec3 enemyDirection = enemy->GetPhysics()->GetOrigin() - owner->GetPhysics()->GetOrigin();
		float dist = (enemyDirection).LengthFast();
		
		//TODO: make not hardcoded
		if (dist > 300)
		{
			_moveRequired = true;

			// grayman #3507 - new way, staying off the floor and then tracing down to it
			idVec3 throwPos = enemy->GetEyePosition() - enemyDirection / dist * 300;
			idVec3 bottomPos = throwPos;
			bottomPos.z -= 256;

			trace_t result;
			if ( gameLocal.clip.TracePoint( result, throwPos, bottomPos, MASK_OPAQUE, NULL ) )
			{
				throwPos = result.endpos;
				throwPos.z += 1;
				owner->movementSubsystem->PushTask(TaskPtr(new MoveToPositionTask(throwPos)));
				owner->AI_MOVE_DONE = false;
			}

			// end new way

			// grayman #3507 - old way
/*			idVec3 throwPos = enemy->GetPhysics()->GetOrigin() - enemyDirection / dist * 300;

			// TODO: Trace to get floor position
			throwPos.z = owner->GetPhysics()->GetOrigin().z;

			owner->movementSubsystem->PushTask(TaskPtr(new MoveToPositionTask(throwPos)));
			owner->AI_MOVE_DONE = false;
 */			// end old way
		}
		else 
		{
			// greebo: Sheathe weapon before starting to throw // FIXME: put weapon to left hand?
			// grayman: No, don't switch weapon to left hand, because we have no animations to do
			// so, and the code everywhere assumes weapons are in the right hand.
			owner->SheathWeapon();

			owner->FaceEnemy();
			owner->actionSubsystem->PushTask(ThrowObjectTask::CreateInstance());

			// Wait after starting to throw before taking cover
			// grayman #3507 - randomize wait time
			_takeCoverTime = gameLocal.time + 2000 + gameLocal.random.RandomFloat()*2000; // wait 2-4 seconds
		}
		DM_LOG(LC_AI, LT_INFO)LOGSTRING("move required: %d \r" , _moveRequired);
	}
	else
	{
		owner->movementSubsystem->PushTask(
			TaskPtr(new MoveToPositionTask(owner->lastVisibleReachableEnemyPos))
		);
		// grayman #3507 - randomize wait time
		_takeCoverTime = gameLocal.time + 2000 + gameLocal.random.RandomFloat()*2000; // wait 2-4 seconds
	}

	_reachEnemyCheck = 0;
}

// Gets called each time the mind is thinking
void UnreachableTargetState::Think(idAI* owner)
{
	Memory& memory = owner->GetMemory();

	idActor* enemy = _enemy.GetEntity();
	if (enemy == NULL)
	{
		DM_LOG(LC_AI, LT_ERROR)LOGSTRING("No enemy!\r");
		owner->GetMind()->SwitchState(STATE_LOST_TRACK_OF_ENEMY);
		return;
	}

	// grayman #3492 - if you kill the enemy with a rock, behave the same
	// way you would had you killed him with a weapon

	if (enemy->AI_DEAD)
	{
		owner->SetLastKilled(enemy);
		owner->ClearEnemy();
		owner->StopMove(MOVE_STATUS_DONE);
		owner->SetAlertLevel(owner->thresh_2 + (owner->thresh_3 - owner->thresh_2) * 0.5);
		
		// grayman #3473 - stop looking at the spot you were looking at when you killed the enemy
		owner->SetFocusTime(gameLocal.time);

		// bark about death

		idStr bark = "";
		idStr enemyAiUse = enemy->spawnArgs.GetString("AIUse");
		if ( ( enemyAiUse == AIUSE_MONSTER ) || ( enemyAiUse == AIUSE_UNDEAD ) )
		{
			bark = "snd_killed_monster";
		}
		else
		{
			bark = "snd_killed_enemy";
		}
		owner->PostEventMS(&AI_Bark,ENEMY_DEAD_BARK_DELAY,bark);

		owner->GetMind()->EndState();
		return;
	}

	// Check the distance to the enemy, the other subsystem tasks need it.
	// This handles both melee and ranged weapons.
	memory.canHitEnemy = owner->CanHitEntity(enemy);
	if (!owner->AI_ENEMY_VISIBLE)
	{
		// The enemy is not visible, let's keep track of him for a small amount of time
		if (gameLocal.time - memory.lastTimeEnemySeen < MAX_BLIND_UNREACHABLE_TIME ) // grayman #4343
		{
			// Cheat a bit and take the last reachable position as "visible & reachable"
			owner->lastVisibleReachableEnemyPos = owner->lastReachableEnemyPos;
		}
		else
		{
			// BLIND_CHASE_TIME has expired, we have lost the enemy!
			owner->GetMind()->SwitchState(STATE_LOST_TRACK_OF_ENEMY);
			return;
		}
	}

	owner->TurnToward(enemy->GetPhysics()->GetOrigin());
	
	// grayman #3775 - test reachability before deciding to throw an object

	// This checks if the enemy is reachable again so we can go into combat state
	if (owner->enemyReachable || owner->TestMelee() || memory.canHitEnemy)
	{
		if (owner->GetMind()->PerformCombatCheck())
		{
			owner->GetMind()->EndState();
			return;
		}
	}

	// This checks for a reachable position within combat range

	// grayman #3507 - new way

	// FindAttackPosition() handles both melee and ranged attacks

	idVec3 targetPoint;
	if (owner->FindAttackPosition(_reachEnemyCheck, enemy, targetPoint, COMBAT_NONE))
	{
		owner->GetMind()->EndState();
		return;
	}

	// Throw rock if you can.
	// If you had to get closer first, the throw is handled here.
	// If you didn't have to get closer, the throw was handled in Init().

	if (owner->spawnArgs.GetBool("outofreach_projectile_enabled", "0") &&
			_moveRequired && (owner->AI_MOVE_DONE || owner->AI_DEST_UNREACHABLE))
	{
		// We are finished moving closer
		// Start throwing now
		_moveRequired = false;

		// greebo: Sheathe weapon before starting to throw // FIXME: put weapon to left hand?
		// grayman: No, don't switch weapon to left hand, because we have no animations to do
		// so, and the code everywhere assumes weapons are in the right hand.
		owner->SheathWeapon();

		owner->FaceEnemy();
		owner->actionSubsystem->PushTask(ThrowObjectTask::CreateInstance());
		// grayman #3507 - randomize wait time
		_takeCoverTime = gameLocal.time + 2000 + gameLocal.random.RandomFloat()*2000; // wait 2-4 seconds
	}

	/* grayman #3775 - moved up
	// This checks if the enemy is reachable again so we can go into combat state
	if (owner->enemyReachable || owner->TestMelee() || memory.canHitEnemy)
	{
		if (owner->GetMind()->PerformCombatCheck())
		{
			owner->GetMind()->EndState();
			return;
		}
	}

	// This checks for a reachable position within combat range

	// grayman #3507 - new way

	// FindAttackPosition() handles both melee and ranged attacks

	idVec3 targetPoint;
	if (owner->FindAttackPosition(_reachEnemyCheck, enemy, targetPoint, COMBAT_NONE))
	{
		owner->GetMind()->EndState();
		return;
	}
	*/

	_reachEnemyCheck++;
	_reachEnemyCheck %= 4;
	
	// Wait some time (_takeCoverTime) after starting to throw before taking cover
	// If a ranged threat from the player is detected (bow is out)
	// take cover if possible after throwing animation is finished
	idStr waitState(owner->WaitState());

	if ( ( waitState != "throw" ) &&
		 ( _takeCoverTime > 0 ) &&
		 ( gameLocal.time >= _takeCoverTime ) &&
		 ( enemy->RangedThreatTo(owner) || !owner->spawnArgs.GetBool("taking_cover_only_from_archers","0") ))
	{
		if (owner->spawnArgs.GetBool("taking_cover_enabled","0"))
		{
			aasGoal_t hideGoal;
			 // grayman #3280 - enemies look with their eyes, not their feet
			bool takingCoverPossible = owner->LookForCover(hideGoal, enemy, enemy->GetEyePosition());
			if (takingCoverPossible)
			{
				// We should not go into TakeCoverState if we are already at a suitable position

				// grayman #3507 - use a small distance check instead of an exact check
				// if (hideGoal.origin == owner->GetPhysics()->GetOrigin() )
				if ((hideGoal.origin - owner->GetPhysics()->GetOrigin()).LengthSqr() <= 20*20 )
				{
					takingCoverPossible = false;
				}
			}

			if (takingCoverPossible)
			{
				owner->GetMind()->SwitchState(STATE_TAKE_COVER);
			}
		}
	}
}

void UnreachableTargetState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);

//	savefile->WriteBool(_takingCoverPossible); // grayman #3507 - made local to Think(), no need to save
	savefile->WriteInt(_takeCoverTime);
	savefile->WriteBool(_moveRequired);
	savefile->WriteInt(_reachEnemyCheck);
	_enemy.Save(savefile);
}

void UnreachableTargetState::Restore(idRestoreGame* savefile)
{
	State::Restore(savefile);

//	savefile->ReadBool(_takingCoverPossible); // grayman #3507 - made local to Think(), no need to save
	savefile->ReadInt(_takeCoverTime);
	savefile->ReadBool(_moveRequired);
	savefile->ReadInt(_reachEnemyCheck);
	_enemy.Restore(savefile);
}

StatePtr UnreachableTargetState::CreateInstance()
{
	return StatePtr(new UnreachableTargetState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar unreachableTargetStateRegistrar(
	STATE_UNREACHABLE_TARGET, // Task Name
	StateLibrary::CreateInstanceFunc(&UnreachableTargetState::CreateInstance) // Instance creation callback
);

} // namespace ai
