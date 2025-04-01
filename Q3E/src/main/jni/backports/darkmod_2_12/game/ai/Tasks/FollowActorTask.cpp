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
#include "FollowActorTask.h"
#include "../Library.h"

namespace ai
{

#define DISTANCE_FOLLOWER_REACHED						60
#define DISTANCE_FOLLOWER_CATCHUP_DISTANCE				480
#define DISTANCE_FOLLOWER_CLOSE_ENOUGH_TO_STOP_RUNNING	180

FollowActorTask::FollowActorTask()
{}

FollowActorTask::FollowActorTask(idActor* actor)
{
	_actor = actor;
}

// Get the name of this task
const idStr& FollowActorTask::GetName() const
{
	static idStr _name(TASK_FOLLOW_ACTOR);
	return _name;
}

void FollowActorTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	if (_actor.GetEntity() == NULL)
	{
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("FollowActorTask will terminate after Perform() since a NULL actor was being passed in.\r");
	}
}

bool FollowActorTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("FollowActorTask performing.\r");

	idActor* actor = _actor.GetEntity();

	if (actor == NULL || actor->AI_DEAD)
	{
		return true; // no actor (anymore, or maybe we never had one)
	}

	if (actor->IsType(idAI::Type) && static_cast<idAI*>(actor)->AI_KNOCKEDOUT)
	{
		return true; // AI got knocked out
	}

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	// Classify how far the target actor is away from us
	float distance = (actor->GetPhysics()->GetOrigin() - owner->GetPhysics()->GetOrigin()).LengthSqr();

	if (distance < Square(DISTANCE_FOLLOWER_REACHED))
	{
		owner->StopMove(MOVE_STATUS_DONE);
		owner->TurnToward(actor->GetEyePosition());
	}
	else 
	{
		if (!owner->MoveToPosition(actor->GetPhysics()->GetOrigin()))
		{
			// Can't reach the actor, stop moving
			owner->StopMove(MOVE_STATUS_DONE);
			owner->TurnToward(actor->GetEyePosition());

			// Keep trying
			return false;
		}

		if (distance > Square(DISTANCE_FOLLOWER_CATCHUP_DISTANCE))
		{
			owner->AI_RUN = true;
		}
		// We're below catch up distance, but keep running until we've reached about 180 units
		else if (owner->AI_RUN && distance < Square(DISTANCE_FOLLOWER_CLOSE_ENOUGH_TO_STOP_RUNNING))
		{
			owner->AI_RUN = false;
		}
	}
	
	return false; // not finished yet
}

// Save/Restore methods
void FollowActorTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	_actor.Save(savefile);
}

void FollowActorTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	_actor.Restore(savefile);
}

FollowActorTaskPtr FollowActorTask::CreateInstance()
{
	return FollowActorTaskPtr(new FollowActorTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar followActorTaskRegistrar(
	TASK_FOLLOW_ACTOR, // Task Name
	TaskLibrary::CreateInstanceFunc(&FollowActorTask::CreateInstance) // Instance creation callback
);

} // namespace ai
