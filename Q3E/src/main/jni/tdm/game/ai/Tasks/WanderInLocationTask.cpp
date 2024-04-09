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



#include "WanderInLocationTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

const float WANDER_RADIUS = 400;

WanderInLocationTask::WanderInLocationTask() :
	_location(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY)
{}

WanderInLocationTask::WanderInLocationTask(const idVec3& location) :
	_location(location)
{}

// Get the name of this task
const idStr& WanderInLocationTask::GetName() const
{
	static idStr _name(TASK_WANDER_IN_LOCATION);
	return _name;
}

void WanderInLocationTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	Task::Init(owner, subsystem);

	owner->AI_RUN = false;

	// Turn toward the initial location
	owner->TurnToward(_location);

	// Move toward the initial loation
	if (!owner->MoveToPosition(_location))
	{
		// If the initial location isn't reachable, try to find a reachable position nearby
		idVec3 direction = owner->GetPhysics()->GetOrigin() - _location;
		direction.z = 0;
		if (direction.LengthFast() < 80)
		{
			// We're close enough to the alert position, take the current position of the AI
			_location = owner->GetPhysics()->GetOrigin();
		}
		else
		{
			direction.Normalize();
			idVec3 startPoint = _location + 80 * direction;
			startPoint.z += 200;

			idVec3 bottomPoint = startPoint;
			bottomPoint.z -= 400;
				
			idVec3 targetPoint = startPoint;
			trace_t result;
			if (gameLocal.clip.TracePoint(result, startPoint, bottomPoint, MASK_OPAQUE, NULL))
			{
				targetPoint.z = result.endpos.z + 1;
				
				int areaNum = owner->PointReachableAreaNum(owner->GetPhysics()->GetOrigin(), 1.0f);
				int targetAreaNum = owner->PointReachableAreaNum(targetPoint, 1.0f);
				aasPath_t path;
				if (owner->PathToGoal(path, areaNum, owner->GetPhysics()->GetOrigin(), targetAreaNum, targetPoint, owner))
				{
					_location = targetPoint;
				}
				else
				{
					// This didn't work out, the current position of the ai is still better than nothing
					_location = owner->GetPhysics()->GetOrigin();
				}
			}
			else
			{
				
				_location = owner->GetPhysics()->GetOrigin();
			}
		}
	}
}

bool WanderInLocationTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("WanderInLocationTask performing.\r");

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	//Memory& memory = owner->GetMemory();

	if (owner->AI_MOVE_DONE)
	{
		// Wander in a new direction if wander phase time expired or we got stopped by something
		owner->WanderAround();
	}
	// if we are approaching or past maximum distance, next wander is back toward the initial position
	else if ((owner->GetPhysics()->GetOrigin() - _location).LengthFast() >= WANDER_RADIUS)
	{
		if (!owner->MoveToPosition(_location))
		{
			//gameLocal.Printf("WanderInLocationTask: Destination unreachable... \n");
		}

	}

	return false; // not finished yet
}

void WanderInLocationTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteVec3(_location);
}

void WanderInLocationTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadVec3(_location);
}

WanderInLocationTaskPtr WanderInLocationTask::CreateInstance()
{
	return WanderInLocationTaskPtr(new WanderInLocationTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar wanderInLocationTaskRegistrar(
	TASK_WANDER_IN_LOCATION, // Task Name
	TaskLibrary::CreateInstanceFunc(&WanderInLocationTask::CreateInstance) // Instance creation callback
);

} // namespace ai
