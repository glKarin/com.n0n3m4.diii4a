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



#include "StayInCoverState.h"
#include "FleeState.h"
#include "../Memory.h"
#include "../Tasks/MoveToCoverTask.h"
#include "../Tasks/WaitTask.h"
#include "../Tasks/MoveToPositionTask.h"
#include "../Tasks/RandomHeadturnTask.h"
#include "LostTrackOfEnemyState.h"
#include "EmergeFromCoverState.h"
#include "../Library.h"
#include "UnreachableTargetState.h"

namespace ai
{

// Get the name of this state
const idStr& StayInCoverState::GetName() const
{
	static idStr _name(STATE_STAY_IN_COVER);
	return _name;
}

void StayInCoverState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("StayInCoverState initialised.\r");
	assert(owner);

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	// Fill the subsystems with their tasks

	// The movement subsystem should do nothing.
	owner->StopMove(MOVE_STATUS_DONE);
	owner->movementSubsystem->ClearTasks();
	memory.StopReacting(); // grayman #3559

	// Calculate the time we should stay in cover
	int coverDelayMin = SEC2MS(owner->spawnArgs.GetFloat("emerge_from_cover_delay_min"));
	int coverDelayMax = SEC2MS(owner->spawnArgs.GetFloat("emerge_from_cover_delay_max"));
	int waitTime = static_cast<int>(
		coverDelayMin + gameLocal.random.RandomFloat() * (coverDelayMax - coverDelayMin)
	);
	_emergeDelay = gameLocal.time + waitTime;

	// The communication system 
	// owner->GetSubsystem(SubsysCommunication)->ClearTasks(); // TODO_AI

	// The sensory system does random head turning
	owner->senseSubsystem->ClearTasks();
	owner->senseSubsystem->PushTask(RandomHeadturnTask::CreateInstance());

	// Waiting in cover
	owner->actionSubsystem->ClearTasks();
}

// Gets called each time the mind is thinking
void StayInCoverState::Think(idAI* owner)
{
	// grayman #3507 - still have an enemy?
	idActor* enemy = owner->GetEnemy();
	if ( enemy == NULL )
	{
		owner->GetMind()->SwitchState(STATE_LOST_TRACK_OF_ENEMY);
		return;
	}

	// grayman #3507 - is enemy dead or unconscious?
	if ( enemy->AI_DEAD || enemy->IsKnockedOut() )
	{
		DM_LOG(LC_AI, LT_ERROR)LOGSTRING("StayInCoverState::Think (%s) enemy dead or unconscious, come out of hiding\r",owner->GetName());
		owner->GetMind()->EndState();
		return;
	}

	// grayman #3507 - return to Combat if bumped by an enemy

	bool combatCheck = owner->GetMind()->PerformCombatCheck();
	if ( owner->AI_TACTALERT )
	{
		if ( owner->GetTactEnt() == enemy )
		{
			// Get back to combat if we are done moving to cover and encounter the enemy
			owner->GetMind()->EndState();
			return;
		}
	}

	// grayman #3507 - try a simple path to the enemy

	if ( owner->AI_ENEMY_VISIBLE && combatCheck )
	{
		// grayman #3507 - Can I reach the enemy?

		idVec3 enemyOrigin = enemy->GetPhysics()->GetOrigin();
		int enemyAreaNum = owner->PointReachableAreaNum(enemyOrigin, 1.0f);

		if (enemyAreaNum > 0)
		{
			idVec3 ownerOrigin = owner->GetPhysics()->GetOrigin();
			int ownerAreaNum = owner->PointReachableAreaNum(ownerOrigin, 1.0f);
			aasPath_t path;

			if (owner->PathToGoal(path, ownerAreaNum, ownerOrigin, enemyAreaNum, enemyOrigin, owner))
			{
				// Get back to combat if we can reach the enemy
				owner->GetMind()->EndState();
				return;
			}
		}
		owner->GetMind()->SwitchState(STATE_UNREACHABLE_TARGET);
		return;
	}

	if (gameLocal.time >= _emergeDelay)
	{
		// emerge from cover after waiting is done
		owner->GetMind()->SwitchState(STATE_EMERGE_FROM_COVER);
		return;
	}
}

/* grayman #3140 - no longer used
void StayInCoverState::OnProjectileHit(idProjectile* projectile)
{
	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	// Call the base class first
	State::OnProjectileHit(projectile);

	if ((owner->GetNumMeleeWeapons() == 0 && owner->GetNumRangedWeapons() == 0) ||
		owner->spawnArgs.GetBool("is_civilian"))
	{
		// We are unarmed or civilian and got hit by a projectile while in cover, run!
		owner->GetMind()->SwitchState(STATE_FLEE);
	}
	else
	{
		// We are armed, so let's just end this state and deal with it
		owner->GetMind()->EndState();
	}
}
*/

void StayInCoverState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);

	savefile->WriteInt(_emergeDelay);
}

void StayInCoverState::Restore(idRestoreGame* savefile)
{
	State::Restore(savefile);

	savefile->ReadInt(_emergeDelay);
}

StatePtr StayInCoverState::CreateInstance()
{
	return StatePtr(new StayInCoverState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar stayInCoverStateRegistrar(
	STATE_STAY_IN_COVER, // Task Name
	StateLibrary::CreateInstanceFunc(&StayInCoverState::CreateInstance) // Instance creation callback
);

} // namespace ai
