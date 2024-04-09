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



#include "AnimalPatrolTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

	namespace {
		const float WANDER_RADIUS = 240;
	}

AnimalPatrolTask::AnimalPatrolTask() :
	_state(stateNone)
{}

// Get the name of this task
const idStr& AnimalPatrolTask::GetName() const
{
	static idStr _name(TASK_ANIMAL_PATROL);
	return _name;
}

void AnimalPatrolTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	Task::Init(owner, subsystem);

	_state = stateNone;

	// Check if we are supposed to patrol and make sure that there
	// is a valid PathCorner entity set in the AI's mind

	if (owner->spawnArgs.GetBool("patrol", "1")) 
	{
		idPathCorner* path = owner->GetMemory().currentPath.GetEntity();

		// Check if we already have a path entity
		if (path == NULL)
		{
			// Path not yet initialised, get it afresh
			// Find the next path associated with the owning AI
			path = idPathCorner::RandomPath(owner, NULL, owner);
		}

		// Store the path entity back into the mind, it might have changed
		owner->GetMemory().currentPath = path;
	}
	else
	{
		subsystem.FinishTask();
		return;
	}
}

bool AnimalPatrolTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("AnimalPatrolTask performing.\r");

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	switch (_state) 
	{
		case stateNone: 
			// gameRenderWorld->DebugText("Choosing", owner->GetPhysics()->GetOrigin(), 0.6f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 32);
			chooseNewState(owner);
			break;
		case stateMovingToNextSpot:
			// gameRenderWorld->DebugText("MovingToNextSpot", owner->GetPhysics()->GetOrigin(), 0.6f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 32);
			// gameRenderWorld->DebugArrow(colorYellow, owner->GetPhysics()->GetOrigin(), owner->GetMoveDest(), 0, 64);
			movingToNextSpot(owner);
			break;
		case stateMovingToNextPathCorner: 
			// gameRenderWorld->DebugText("MovingToNextCorner", owner->GetPhysics()->GetOrigin(), 0.6f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 32);
			movingToNextPathCorner(owner);
			break;
		case stateDoingSomething: 
			// gameRenderWorld->DebugText("DoingSomething", owner->GetPhysics()->GetOrigin(), 0.6f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 32);
			switchToState(stateWaiting, owner);
			break;
		case stateWaiting:
			// gameRenderWorld->DebugText("Waiting", owner->GetPhysics()->GetOrigin(), 0.6f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 32);
			waiting(owner);
			break;
		case statePreMovingToNextSpot: // grayman #2356 - don't spend so much time waiting; go somewhere
			// gameRenderWorld->DebugText("PreMovingToNextSpot", owner->GetPhysics()->GetOrigin(), 0.6f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 32);
			switchToState(stateMovingToNextSpot,owner);
			break;
		default:
			break;
	};

	return false; // not finished yet
}

void AnimalPatrolTask::switchToState(EState newState, idAI* owner) 
{
	switch (newState)
	{
	case stateMovingToNextSpot:
		{
			const idVec3& curPos = owner->GetPhysics()->GetOrigin();
			_moveEndTime = 0;
			for (int i = 0 ; i < 5 ; i++) // grayman #2356 - try a few positions to increase movement in small spaces 
			{
				// Choose a goal position, somewhere near ourselves
				float xDelta = gameLocal.random.RandomFloat()*WANDER_RADIUS - WANDER_RADIUS*0.5f;
				float yDelta = gameLocal.random.RandomFloat()*WANDER_RADIUS - WANDER_RADIUS*0.5f;

				idVec3 newPos = curPos + idVec3(xDelta, yDelta, 1); // grayman #2356 - was '5'; reduced to keep goal positions from becoming too high for rats
				if (owner->MoveToPosition(newPos))
				{
					// Run with a 20% chance
					owner->AI_RUN = (gameLocal.random.RandomFloat() < 0.2f);
					_moveEndTime = gameLocal.time + 10000;	// grayman #2356 - 10s timeout on getting to your next position.
															// This keeps you from getting stuck trying to reach a valid goal
															// that's unreachable. I.e. the goal is on a ledge and the rat
															// trying to reach it falls off the ledge.
					break;
				}
			}
		}
		break;
	case stateWaiting:
		if (owner->m_bCanDrown && owner->MouthIsUnderwater()) // grayman #2356 - don't hang around if you're drowning
		{
			_waitEndTime = gameLocal.time;
		}
		else
		{
			float wait = owner->spawnArgs.GetFloat("animal_patrol_wait", "1");
			_waitEndTime = gameLocal.time + gameLocal.random.RandomInt( wait * 1000 );
		}
		break;
	case stateMovingToNextPathCorner:
		{
			idPathCorner* path = owner->GetMemory().currentPath.GetEntity();
			if (path != NULL)
			{
				owner->MoveToPosition(path->GetPhysics()->GetOrigin());
				owner->AI_RUN = path->spawnArgs.GetBool("run", "0");
			}
			else // grayman #2356 - if not on path corners, pick somewhere to go
				 // 50% of the time. This cuts down on waiting, which seemed to be excessive.
			{
				if (gameLocal.random.RandomFloat() < 0.5f)
				{
					newState = statePreMovingToNextSpot;
				}
			}
		}
		break;
	default:
		break;
	}

	_state = newState;
}

void AnimalPatrolTask::chooseNewState(idAI* owner) 
{
	// For now, choose randomly between the various possibilities
	int rand = gameLocal.random.RandomInt(static_cast<int>(stateCount-1)) + 1;
	
	// Switch and initialise the states
	switchToState(static_cast<EState>(rand), owner);
}

void AnimalPatrolTask::movingToNextSpot(idAI* owner) 
{
	if ((_moveEndTime > 0) && (gameLocal.time > _moveEndTime)) // grayman #2356 - if movement timeout occurs, end the move (prevents getting stuck)
	{
		owner->MoveToPosition(owner->GetPhysics()->GetOrigin()); // reset goal position to your own origin
	}

	if (owner->AI_MOVE_DONE) 
	{
		// We've reached the destination, wait a bit
		switchToState(stateWaiting, owner);
		if (owner->AI_DEST_UNREACHABLE) 
		{
			// Destination is unreachable, switch to new state
			chooseNewState(owner);
		}
	}
	else
	{
		if (gameLocal.random.RandomFloat() < 0.1f)
		{
			// Still moving, maybe turn a bit
			owner->Event_SaveMove();

			float xDelta = gameLocal.random.RandomFloat()*20 - 10;
			float yDelta = gameLocal.random.RandomFloat()*20 - 10;

			// Try to move the goal a bit
			if (!owner->MoveToPosition(owner->GetMoveDest() + idVec3(xDelta, yDelta, 1))) // grayman #2356 - was '2'; increase chances of reaching the spot
			{
				// MoveToPosition failed, restore the move state
				owner->Event_RestoreMove();
			}
		}

		// grayman #2356 - if you're far from the goal, run. This keeps rats from slow-hopping great distances.

		if ((owner->GetMoveDest() - owner->GetPhysics()->GetOrigin()).LengthFast() > WANDER_RADIUS/2)
		{
			owner->AI_RUN = true;
		}
	}
}

void AnimalPatrolTask::movingToNextPathCorner(idAI* owner) 
{
	if (owner->AI_MOVE_DONE) 
	{
		// Find the next path associated with the owning AI
		idPathCorner* curCorner = owner->GetMemory().currentPath.GetEntity();
		if (curCorner != NULL)
		{
			owner->GetMemory().currentPath = idPathCorner::RandomPath(curCorner, NULL, owner);
		}

		if (owner->AI_DEST_UNREACHABLE) 
		{
			// Destination is unreachable, switch to new state
			chooseNewState(owner);
		}

		// We've reached the destination, wait a bit
		switchToState(stateDoingSomething, owner);
	}
	else 
	{
		// Toggle running with a 5% chance
		if (gameLocal.random.RandomFloat() < 0.05f)
		{
			owner->AI_RUN = !owner->AI_RUN;
		}
	}
}

void AnimalPatrolTask::waiting(idAI* owner)
{
	// Switch the state if the time has come
	if (gameLocal.time > _waitEndTime) 
	{
		chooseNewState(owner);
	}
}

void AnimalPatrolTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteInt(static_cast<int>(_state));
	savefile->WriteInt(_waitEndTime);
	savefile->WriteInt(_moveEndTime); // grayman #2356
}

void AnimalPatrolTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	int temp;
	savefile->ReadInt(temp);
	_state = static_cast<EState>(temp);

	savefile->ReadInt(_waitEndTime);
	savefile->ReadInt(_moveEndTime); // grayman #2356
}

AnimalPatrolTaskPtr AnimalPatrolTask::CreateInstance()
{
	return AnimalPatrolTaskPtr(new AnimalPatrolTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar animalPatrolTaskRegistrar(
	TASK_ANIMAL_PATROL, // Task Name
	TaskLibrary::CreateInstanceFunc(&AnimalPatrolTask::CreateInstance) // Instance creation callback
);

} // namespace ai
