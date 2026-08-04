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



#include "InteractionTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

InteractionTask::InteractionTask() :
	_waitEndTime(-1)
{}

InteractionTask::InteractionTask(idEntity* interactEnt) :
	_interactEnt(interactEnt),
	_waitEndTime(-1)
{}

// Get the name of this task
const idStr& InteractionTask::GetName() const
{
	static idStr _name(TASK_INTERACTION);
	return _name;
}

void InteractionTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	// Store the current move state
	owner->PushMove();

	if (_interactEnt == NULL) 
	{
		subsystem.FinishTask();
		return; // grayman #3670
	}
	
	float moveToPositionTolerance = _interactEnt->spawnArgs.GetFloat("move_to_position_tolerance", "-1");

	// Start moving towards that entity
	if (!owner->MoveToPosition(_interactEnt->GetPhysics()->GetOrigin(), moveToPositionTolerance))
	{
		// No path to that entity!
		subsystem.FinishTask();
		return; // grayman #3670
	}

	_interactEnt->GetUserManager().AddUser(owner);

}

bool InteractionTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("InteractionTask performing.\r");

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	// grayman #3029 - special case for elevator fetch buttons: we don't
	// want an AI to fetch a moving elevator. He has to wait for it to come to rest.

	if (_interactEnt->spawnArgs.GetBool("fetch","0"))
	{
		for ( int i = 0 ; i < _interactEnt->targets.Num() ; i++ )
		{
			idEntity* ent = _interactEnt->targets[i].GetEntity();

			if (ent == NULL)
			{
				continue;
			}

			const char *classname;
			ent->spawnArgs.GetString("classname", NULL, &classname);
			if (idStr::Cmp(classname, "atdm:mover_elevator") == 0)
			{
				if (!ent->IsAtRest())
				{
					return false; // wait for the elevator to come to rest
				}
			}
		}
	}

	if ( ( _waitEndTime > 0 ) && ( gameLocal.time >= _waitEndTime) )
	{
		// Trigger the frob action script
		_interactEnt->FrobAction(true);
		return true;
	}
	else if ( ( _waitEndTime < 0 ) && ( owner->GetMoveStatus() == MOVE_STATUS_DONE) )
	{
		// We've reached our target, turn and look
		owner->TurnToward(_interactEnt->GetPhysics()->GetOrigin());
		owner->Event_LookAtEntity(_interactEnt, 1);

		// Start anim
		owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Use_righthand", 4);
		_waitEndTime = gameLocal.time + 600;
	}
	else 
	{
		// If horizontal distance smaller than 33, stop running
		idVec3 ownerOrigin = owner->GetPhysics()->GetOrigin();
		idVec3 targetOrigin = _interactEnt->GetPhysics()->GetOrigin();
		ownerOrigin.z = 0;
		targetOrigin.z = 0;

		if ((ownerOrigin - targetOrigin).LengthSqr() < 1000) 
		{
			owner->AI_RUN = false;
		}
	}
	
	return false; // continue this task
}

void InteractionTask::OnFinish(idAI* owner)
{
	_interactEnt->GetUserManager().RemoveUser(owner);

	owner->PopMove();
}

// Save/Restore methods
void InteractionTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteObject(_interactEnt);
	savefile->WriteInt(_waitEndTime);
}

void InteractionTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadObject(reinterpret_cast<idClass*&>(_interactEnt));
	savefile->ReadInt(_waitEndTime);
}

InteractionTaskPtr InteractionTask::CreateInstance()
{
	return InteractionTaskPtr(new InteractionTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar interactionTaskRegistrar(
	TASK_INTERACTION, // Task Name
	TaskLibrary::CreateInstanceFunc(&InteractionTask::CreateInstance) // Instance creation callback
);

} // namespace ai
