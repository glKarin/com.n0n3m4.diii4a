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
#include "PathInteractTask.h"
#include "../Library.h"

namespace ai
{

PathInteractTask::PathInteractTask() :
	PathTask()
{}

PathInteractTask::PathInteractTask(idPathCorner* path) :	
	PathTask(path)
{
	_path = path;
}

// Get the name of this task
const idStr& PathInteractTask::GetName() const
{
	static idStr _name(TASK_PATH_INTERACT);
	return _name;
}

void PathInteractTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	PathTask::Init(owner, subsystem);

	idPathCorner* path = _path.GetEntity();

	idStr targetStr = path->spawnArgs.GetString("ent", "");
	_target = gameLocal.FindEntity(targetStr);

	if (_target == NULL)
	{
		return;
	}

	owner->StopMove(MOVE_STATUS_DONE);
	// Turn and look
	owner->TurnToward(_target->GetPhysics()->GetOrigin());
	owner->Event_LookAtEntity(_target, 1);

	// Start anim
	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Use_righthand", 4);

	_waitEndTime = gameLocal.time + 600;
}

bool PathInteractTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("PathInteractTask performing.\r");

	idPathCorner* path = _path.GetEntity();
	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointers
	assert(path != NULL && owner != NULL);

	if (_target == NULL)
	{
		return true;
	}
	
	if (gameLocal.time >= _waitEndTime)
	{
		// Trigger the frob action script
		_target->FrobAction(true);
		return true;
	}
	
	// Debug
	// gameRenderWorld->DebugArrow(colorGreen, owner->GetEyePosition(), _target->GetPhysics()->GetOrigin(), 10, 10000);
	
	return false;
}

void PathInteractTask::OnFinish(idAI* owner)
{
	// This task may not be performed with empty entity pointers
	assert( owner != NULL );

	// Trigger path target(s)
	// grayman #3670 - this activates owner targets, not path targets
	owner->ActivateTargets(owner);

	// NextPath();
}

// Save/Restore methods
void PathInteractTask::Save(idSaveGame* savefile) const
{
	PathTask::Save(savefile);

	savefile->WriteObject(_target);

	savefile->WriteInt(_waitEndTime);
}

void PathInteractTask::Restore(idRestoreGame* savefile)
{
	PathTask::Restore(savefile);

	savefile->ReadObject(reinterpret_cast<idClass*&>( _target ));

	savefile->ReadInt(_waitEndTime);
}

PathInteractTaskPtr PathInteractTask::CreateInstance()
{
	return PathInteractTaskPtr(new PathInteractTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar pathInteractTaskRegistrar(
	TASK_PATH_INTERACT, // Task Name
	TaskLibrary::CreateInstanceFunc(&PathInteractTask::CreateInstance) // Instance creation callback
);

} // namespace ai
