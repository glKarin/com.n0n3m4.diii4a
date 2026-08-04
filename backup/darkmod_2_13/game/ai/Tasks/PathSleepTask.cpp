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
#include "PathSleepTask.h"
#include "PathTurnTask.h"
#include "WaitTask.h"
#include "RepeatedBarkTask.h"
#include "../States/IdleSleepState.h"
#include "../Library.h"


namespace ai
{

PathSleepTask::PathSleepTask() :
	PathTask()
{}

PathSleepTask::PathSleepTask(idPathCorner* path) :
	PathTask(path)
{
	_path = path;
}

// Get the name of this task
const idStr& PathSleepTask::GetName() const
{
	static idStr _name(TASK_PATH_SLEEP);
	return _name;
}

void PathSleepTask::Init(idAI* owner, Subsystem& subsystem)
{
	PathTask::Init(owner, subsystem);

	idPathCorner* lastPath = owner->GetMemory().lastPath.GetEntity(); // grayman #5164

	// grayman #5164 - Am I too far away to sleep?

	// grayman #5265 - if already sitting, there's no problem going to sleep

	if ( owner->GetMoveType() != MOVETYPE_SIT )
	{
		if ( lastPath )
		{
			idVec3 aiOrigin = owner->GetPhysics()->GetOrigin();
			idVec3 sleepLocation = lastPath->GetPhysics()->GetOrigin();
			sleepLocation.z = aiOrigin.z; // remove z vector
			float dist = (sleepLocation - aiOrigin).LengthFast();

			// if dist is too far, terminate the sleep.

			float accuracy = 16; // default

			if ( dist > idMath::Sqrt(2 * accuracy*accuracy) ) // grayman #5265 extend the required distance
			{
				gameLocal.Warning("PathSleepTask::Init %s (%s) can't sleep: too far from sleeping location %s (%s)\n", owner->GetName(), aiOrigin.ToString(), lastPath->GetName(), lastPath->GetPhysics()->GetOrigin().ToString()); // grayman #5164
				_activateTargets = false; // don't activate targets if you didn't sleep
				subsystem.FinishTask();
				return;
			}
		}
	}

	_activateTargets = true; // grayman #5164 - activate targets after you sleep

	// grayman #3820 - AI can fall asleep on the floor, on a bed, or on a chair.
	// This is defined by the 'sleep_location' spawnarg on the path_sleep entity.
	// To support 2.05 and earlier, where the 'sleep_location' is defined on the AI,
	// we'll continue to allow both definitions, but the one on the path_sleep overrides
	// the one on the AI, if it exists.

	int sleepLocation = _path.GetEntity()->spawnArgs.GetInt("sleep_location", "-1"); // grayman #3820
	if ( sleepLocation == SLEEP_LOC_UNDEFINED )
	{
		// There is no sleep_location spawnarg on the path_sleep, so see if there's one
		// on the AI. If there isn't, then default to sleeping on a bed.
		sleepLocation = owner->spawnArgs.GetInt("sleep_location", "1"); // grayman #3820
	}

	owner->AI_SleepLocation = sleepLocation; // tell the animation scripts what to do

	if ( (sleepLocation == SLEEP_LOC_FLOOR) || (sleepLocation == SLEEP_LOC_BED) )
	{
		if ( _path.GetEntity()->spawnArgs.GetBool("lay_down_left", "1") )
		{
			owner->AI_LAY_DOWN_LEFT = true;
		}
		else
		{
			owner->AI_LAY_DOWN_LEFT = false;
		}
		if (owner->GetMoveType() == MOVETYPE_ANIM)
		{
			owner->FallAsleep();
		}
	}
	else if ( sleepLocation == SLEEP_LOC_CHAIR ) // sleep while sitting in a chair
	{
		if (owner->GetMoveType() == MOVETYPE_SIT) // need to be already sitting in a chair
		{
			owner->FallAsleep();
		}
	}
}

bool PathSleepTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("PathSleepTask performing.\r");

	idAI* owner = _owner.GetEntity();

	// This task may not be performed with an empty owner pointer
	assert(owner != NULL);

	if (owner->GetMoveType() == MOVETYPE_SLEEP)
	{
		/* grayman #3528 - move to OnFinish()
		// grayman #3670 - fire targets
		idPathCorner* path = _path.GetEntity();

		// This task may not be performed with an empty path pointer
		assert( path != NULL );
		path->ActivateTargets(owner);
		*/
		return true;
	}
	return false;
}

// grayman #3528

void PathSleepTask::OnFinish(idAI* owner)
{
	if ( _activateTargets ) // grayman #5164 - activate targets if allowed to sleep
	{
		// grayman #3670 - fire targets
		idPathCorner* path = _path.GetEntity();

		// This task may not be performed with an empty path pointer
		assert( path != NULL );
		path->ActivateTargets(owner);
	}
}

PathSleepTaskPtr PathSleepTask::CreateInstance()
{
	return PathSleepTaskPtr(new PathSleepTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar pathSleepTaskRegistrar(
	TASK_PATH_SLEEP, // Task Name
	TaskLibrary::CreateInstanceFunc(&PathSleepTask::CreateInstance) // Instance creation callback
);

} // namespace ai
