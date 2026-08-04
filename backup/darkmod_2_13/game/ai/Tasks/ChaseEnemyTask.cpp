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



#include "ChaseEnemyTask.h"
#include "InteractionTask.h"
#include "../Memory.h"
#include "../Library.h"
#include "../../MultiStateMover.h"
#include "../EAS/EAS.h"
#include "../States/UnreachableTargetState.h"

namespace ai
{

ChaseEnemyTask::ChaseEnemyTask()
{}

ChaseEnemyTask::ChaseEnemyTask(idActor* enemy)
{
	_enemy = enemy;
}

// Get the name of this task
const idStr& ChaseEnemyTask::GetName() const
{
	static idStr _name(TASK_CHASE_ENEMY);
	return _name;
}

void ChaseEnemyTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	Task::Init(owner, subsystem);

	idActor* enemy = _enemy.GetEntity();
	if (!enemy)
	{
		_enemy = owner->GetEnemy();
	}

	_reachEnemyCheck = 0;
	owner->AI_RUN = true;
	owner->MoveToPosition(owner->lastVisibleEnemyPos);
}

bool ChaseEnemyTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Chase Enemy Task performing.\r");

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	Memory& memory = owner->GetMemory();

	// grayman #3331 - if fleeing, stop chasing the enemy
	if (memory.fleeing)
	{
		return true;
	}

	idActor* enemy = _enemy.GetEntity();
	if (enemy == NULL)
	{
		DM_LOG(LC_AI, LT_ERROR)LOGSTRING("No enemy, terminating task!\r");
		return true;
	}

	// grayman #3331 - if enemy is dead or knocked out, stop the chase

	if ( enemy->AI_DEAD || enemy->IsKnockedOut() )
	{
		return true;
	}

	bool enemyUnreachable = false;

	// are we currently flat-footed?
	if ( owner->m_bFlatFooted )
	{
		_reachEnemyCheck = 0;

		owner->StopMove(MOVE_STATUS_DONE);
		//gameLocal.Printf("Flat footed!\n");
		// Turn to the player
		owner->TurnToward(enemy->GetEyePosition());

		if ( (gameLocal.time - owner->m_FlatFootedTimer) > owner->m_FlatFootedTime )
		{
			owner->m_bFlatFooted = false;
		}
	}
	// Can we damage the enemy already? (this flag is set by the combat state)
	else if (memory.canHitEnemy)
	{
		_reachEnemyCheck = 0;

		// Yes, stop the move!
		owner->StopMove(MOVE_STATUS_DONE);
		//gameLocal.Printf("Enemy is reachable!\n");
		// Turn to the player
		owner->TurnToward(enemy->GetEyePosition());
	}
	// no, push the AI forward and try to get to the last visible reachable enemy position
	else if (owner->MoveToPosition(owner->lastVisibleReachableEnemyPos))
	{
		_reachEnemyCheck = 0;

		if (owner->GetMoveStatus() == MOVE_STATUS_DONE)
		{
			// Position has been reached, turn to enemy, if visible
			if (owner->AI_ENEMY_VISIBLE)
			{
				// angua: Turn to the enemy
				owner->TurnToward(enemy->GetEyePosition());
			}
		}
	}
	else if (owner->AI_MOVE_DONE && !owner->m_HandlingDoor)
	{
		// Enemy position itself is not reachable, try to find a position within melee range around the enemy
		if (_reachEnemyCheck < 4)
		{
			// grayman #3507 - new way
			idVec3 targetPoint; 
			if (owner->FindAttackPosition(_reachEnemyCheck, enemy, targetPoint, COMBAT_MELEE))
			{
				if (!owner->MoveToPosition(targetPoint))
				{
					_reachEnemyCheck++;
				}
			}
			else
			{
				_reachEnemyCheck++;
			}
		}
		else
		{
			// Unreachable by walking, check if the opponent is on an elevator
			CMultiStateMover* mover = enemy->OnElevator(true);
			if (mover != NULL)
			{
				//gameRenderWorld->DebugArrow(colorRed, owner->GetPhysics()->GetOrigin(), mover->GetPhysics()->GetOrigin(), 1, 48);

				// greebo: Check if we can fetch the elevator back to our floor
				CMultiStateMoverPosition* reachablePos = CanFetchElevator(mover, owner);

				if (reachablePos != NULL)
				{
					// Get the button which will fetch the elevator
					CMultiStateMoverButton* button = mover->GetButton(reachablePos, reachablePos, BUTTON_TYPE_FETCH, owner->GetPhysics()->GetOrigin()); // grayman #3029

					// gameRenderWorld->DebugArrow(colorBlue, owner->GetPhysics()->GetOrigin(), button->GetPhysics()->GetOrigin(), 1, 5000);

					// Push to an InteractionTask
					subsystem.PushTask(TaskPtr(new InteractionTask(button)));
					return false;
				}
			}

			// grayman #3331 - Try what ChaseEnemyRangedTask does - look for a nearby
			// spot that might let you sight the enemy again.

			aasGoal_t goal = owner->GetPositionWithinRange(enemy->GetEyePosition());
			if (goal.areaNum != -1)
			{
				// Found a suitable attack position, now move to it
				if (owner->MoveToPosition(goal.origin))
				{
					owner->lastVisibleReachableEnemyPos = goal.origin; // since this is your last gasp, pretend this is the last place you saw the enemy
					return false; // keep going
				}
			}

			// Destination unreachable!

			enemyUnreachable = true;
		}
	}
	else
	{
		enemyUnreachable = true;
	}

	if ( enemyUnreachable )
	{
		DM_LOG(LC_AI, LT_INFO)LOGSTRING("Destination unreachable!\r");
		//gameLocal.Printf("Destination unreachable... \n");
		owner->GetMind()->PushState(STATE_UNREACHABLE_TARGET); // grayman #3507 - push instead of switch
		//owner->GetMind()->SwitchState(STATE_UNREACHABLE_TARGET);
		return true;
	}

	return false; // not finished yet
}

CMultiStateMoverPosition* ChaseEnemyTask::CanFetchElevator(CMultiStateMover* mover, idAI* owner)
{
	if (!owner->CanUseElevators() || owner->GetAAS() == NULL) 
	{
		// Can't use elevators at all, skip this check
		return NULL;
	}

	// Check all elevator stations
	const idList<MoverPositionInfo> positionList = mover->GetPositionInfoList();
	eas::tdmEAS* elevatorSystem = owner->GetAAS()->GetEAS();

	if (elevatorSystem == NULL) return NULL;

	idVec3 ownerOrigin;
	int areaNum;
	owner->GetAASLocation(owner->GetAAS(), ownerOrigin, areaNum);
	
	for (int i = 0; i < positionList.Num(); i++)
	{
		CMultiStateMoverPosition* positionEnt = positionList[i].positionEnt.GetEntity();

		int stationIndex = elevatorSystem->GetElevatorStationIndex(positionEnt);
		if (stationIndex < 0) continue;

		eas::ElevatorStationInfoPtr stationInfo = elevatorSystem->GetElevatorStationInfo(stationIndex);

		aasPath_t path;
		bool pathFound = owner->PathToGoal(path, areaNum, ownerOrigin, stationInfo->areaNum, owner->GetAAS()->AreaCenter(stationInfo->areaNum), NULL);

		if (pathFound)
		{
			// We can reach the elevator position from here, go there
			return stationInfo->elevatorPosition.GetEntity();
		}
	}

	return NULL;
}

void ChaseEnemyTask::SetEnemy(idActor* enemy)
{
	_enemy = enemy;
}

void ChaseEnemyTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	_enemy.Save(savefile);
	savefile->WriteInt(_reachEnemyCheck);
}

void ChaseEnemyTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	_enemy.Restore(savefile);
	savefile->ReadInt(_reachEnemyCheck);
}

ChaseEnemyTaskPtr ChaseEnemyTask::CreateInstance()
{
	return ChaseEnemyTaskPtr(new ChaseEnemyTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar chaseEnemyTaskRegistrar(
	TASK_CHASE_ENEMY, // Task Name
	TaskLibrary::CreateInstanceFunc(&ChaseEnemyTask::CreateInstance) // Instance creation callback
);

} // namespace ai
