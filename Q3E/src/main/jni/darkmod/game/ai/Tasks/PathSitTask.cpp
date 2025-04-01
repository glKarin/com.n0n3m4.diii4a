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



#include "../Memory.h"
#include "PathSitTask.h"
#include "PathTurnTask.h"
#include "../Library.h"

#define		WARNING_DELAY 10000 // grayman #5164 - ms delay between "unreachable" WARNINGs

namespace ai
{

PathSitTask::PathSitTask() :
	PathTask()
{}

PathSitTask::PathSitTask(idPathCorner* path) :
	PathTask(path)
{
	_path = path;
}

// Get the name of this task
const idStr& PathSitTask::GetName() const
{
	static idStr _name(TASK_PATH_SIT);
	return _name;
}

void PathSitTask::Init(idAI* owner, Subsystem& subsystem)
{
	PathTask::Init(owner, subsystem);

	idPathCorner* path = _path.GetEntity();

	// grayman #5164 - Am I too far away to sit?

	idPathCorner* lastPath = owner->GetMemory().lastPath.GetEntity(); // path_corner preceding path_sit
	if ( lastPath )
	{
		idVec3 aiOrigin = owner->GetPhysics()->GetOrigin();
		idVec3 sitLocation = lastPath->GetPhysics()->GetOrigin();
		sitLocation.z = aiOrigin.z; // remove z vector
		float dist = (sitLocation - aiOrigin).LengthFast();

		// if dist is too far, terminate the sit.

		float accuracy = 16; // default

		if ( dist > idMath::Sqrt(2 * accuracy*accuracy) ) // grayman #5265 extend the required distance
		{
			if ( gameLocal.time >= owner->m_nextWarningTime )
			{
				gameLocal.Warning("%s (%s) can't sit: too far from sitting location %s (%s)\n", owner->GetName(), aiOrigin.ToString(), lastPath->GetName(), lastPath->GetPhysics()->GetOrigin().ToString());
				owner->m_nextWarningTime = gameLocal.time + WARNING_DELAY;
			}
			subsystem.FinishTask();
			return;
		}
	}

	// Parse animation spawnargs here
	
	float waittime = path->spawnArgs.GetFloat("wait","0");
	float waitmax = path->spawnArgs.GetFloat("wait_max", "0");

	if (waitmax > 0)
	{
		waittime += (waitmax - waittime) * gameLocal.random.RandomFloat();
	}

	if (waittime > 0)
	{
		_waitEndTime = gameLocal.time + SEC2MS(waittime);
	}
	else
	{
		_waitEndTime = -1;
	}

	// angua: check whether the AI should turn to a specific angle after sitting down
	if (path->spawnArgs.FindKey("sit_down_angle") != NULL)
	{
		owner->AI_SIT_DOWN_ANGLE = path->spawnArgs.GetFloat("sit_down_angle", "0");
	}
	else
	{
		owner->AI_SIT_DOWN_ANGLE = owner->GetCurrentYaw();
	}
	owner->AI_SIT_UP_ANGLE = owner->GetCurrentYaw(); // the anim script uses AI_SIT_UP_ANGLE

	owner->AI_SIT_DOWN_ANGLE = idMath::AngleNormalize180(owner->AI_SIT_DOWN_ANGLE); // the anim script uses AI_SIT_DOWN_ANGLE

	_sittingState = EStateSitStart;
}

bool PathSitTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("PathSitTask performing.\r");

	idAI* owner = _owner.GetEntity();

	// This task may not be performed with an empty owner pointer
	assert(owner != NULL);

	idStr waitState(owner->WaitState()); // grayman #3670

	moveType_t moveType = owner->GetMoveType();

	// grayman #4113 - if we find ourself getting up from sitting because we've been
	// alerted, we have to kill this task, regardless of which state
	// we're in

	if ((moveType == MOVETYPE_GET_UP) && (owner->AI_AlertIndex >= ESearching))
	{
		return true;
	}

	// grayman #4054 - rewrite to use machine states

	switch (_sittingState)
	{
	case EStateSitStart:
		if (moveType == MOVETYPE_SIT)
		{
			// sitting. check sitting angle next
			_sittingState = EStateTurning;
		}
		else if (moveType != MOVETYPE_SIT_DOWN)
		{
			// sit down
			owner->SitDown();
			// we can't move to a separate state to check for
			// the waitState to change from "sit_down" because
			// SitDown() might have failed. we have to come
			// back through here to give it another try.
		}
		break;
	case EStateTurning:
		// sitting. check sitting angle
		if ( owner->AI_SIT_DOWN_ANGLE == owner->GetCurrentYaw() )
		{
			// sitting and at sitting angle, so fire the targets

			idPathCorner* path = _path.GetEntity(); // grayman #3670

			// This task may not be performed with an empty path pointer
			assert(path != NULL);
			path->ActivateTargets(owner);

			if ( _waitEndTime < 0 )
			{
				// no end time set, so we're free to leave and not come back
				return true;
			}

			_sittingState = EStateWaiting;
		}
		break;
	case EStateWaiting:
		// there's an end time, let's see if we're there yet
		if ( gameLocal.time >= _waitEndTime )
		{
			// yes, we're past the end time, so let's get up
			owner->GetUp();
			_sittingState = EStateGetUp;
		}
		break;
	case EStateGetUp:
		// The get up script clears the "get_up" waitState
		// when the get up portion of the anim is finished.
		if (waitState != "get_up")
		{
			// Exit when the waitstate is not "get_up" anymore
			return true;
		}
		break;
	};

	return false;
}

// Save/Restore methods
void PathSitTask::Save(idSaveGame* savefile) const
{
	PathTask::Save(savefile);

	savefile->WriteInt(_waitEndTime);
	savefile->WriteInt(static_cast<int>(_sittingState)); // grayman #4054
}

void PathSitTask::Restore(idRestoreGame* savefile)
{
	PathTask::Restore(savefile);

	savefile->ReadInt(_waitEndTime);

	// grayman #4054
	int temp;
	savefile->ReadInt(temp);
	_sittingState = static_cast<ESitState>(temp);
}

PathSitTaskPtr PathSitTask::CreateInstance()
{
	return PathSitTaskPtr(new PathSitTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar pathSitTaskRegistrar(
	TASK_PATH_SIT, // Task Name
	TaskLibrary::CreateInstanceFunc(&PathSitTask::CreateInstance) // Instance creation callback
);

} // namespace ai
