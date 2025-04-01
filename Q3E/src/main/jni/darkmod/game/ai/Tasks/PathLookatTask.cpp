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
#include "PathLookatTask.h"
#include "../Library.h"

namespace ai
{

PathLookatTask::PathLookatTask() :
	PathTask()
{}

PathLookatTask::PathLookatTask(idPathCorner* path) : 
	PathTask(path)
{
	_path = path;
}

// Get the name of this task
const idStr& PathLookatTask::GetName() const
{
	static idStr _name(TASK_PATH_LOOKAT);
	return _name;
}

void PathLookatTask::Init(idAI* owner, Subsystem& subsystem)
{
	PathTask::Init(owner, subsystem);

	idPathCorner* path = _path.GetEntity();

	// The entity to look at is named by the "focus" spawnarg, if that spawnarg is set.
	idStr focusEntName(path->spawnArgs.GetString("focus"));
	_focusEnt = NULL;
	if (focusEntName.Length()) {
		_focusEnt = gameLocal.FindEntity(focusEntName);
		if (!_focusEnt) {
			// If it's specified, it should really exist
			gameLocal.Warning("Entity '%s' names a non-existent focus entity '%s'", path->name.c_str(), focusEntName.c_str());
		}
	}
	// If spawnarg not set or no such entity, fall back to using the path entity itself
	if (!_focusEnt) _focusEnt = path;

	// Duration is indicated by "wait" key
	_duration = path->spawnArgs.GetFloat("wait","1");
	float waitmax = path->spawnArgs.GetFloat("wait_max", "0");

	if (waitmax > 0)
	{
		_duration += (waitmax - _duration) * gameLocal.random.RandomFloat();
	}

	owner->AI_ACTIVATED = false;
}

bool PathLookatTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Path Lookat Task performing.\r");

	idPathCorner* path = _path.GetEntity();
	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointers
	assert(path != NULL && owner != NULL);

	if (_duration == 0)
	{
		// angua: waiting for trigger, no duration set
		owner->Event_LookAtEntity(_focusEnt, 1);

		if (owner->AI_ACTIVATED)
		{
			owner->AI_ACTIVATED = false;

			// Trigger path target(s)
			// grayman #3670 - this activates owner targets, not path targets
			owner->ActivateTargets(owner);

			// NextPath();
			
			return true; // finish this task
		}
		return false;
	}
	else
	{
		// Look
		owner->Event_LookAtEntity(_focusEnt, _duration);
	}
	
	// Debug
	// gameRenderWorld->DebugArrow(colorGreen, owner->GetEyePosition(), focusEnt->GetPhysics()->GetOrigin(), 10, 10000);

	// Trigger path target(s)
	// grayman #3670 - this activates owner targets, not path targets
	owner->ActivateTargets(owner);

	// Store a new path entity into the AI's mind
	// NextPath();
	
	return true; // finish this task
}



// Save/Restore methods
void PathLookatTask::Save(idSaveGame* savefile) const
{
	PathTask::Save(savefile);

	savefile->WriteObject(_focusEnt);

	savefile->WriteFloat(_duration);
}

void PathLookatTask::Restore(idRestoreGame* savefile)
{
	PathTask::Restore(savefile);

	savefile->ReadObject(reinterpret_cast<idClass*&>(_focusEnt));

	savefile->ReadFloat(_duration);
}

PathLookatTaskPtr PathLookatTask::CreateInstance()
{
	return PathLookatTaskPtr(new PathLookatTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar pathLookatTaskRegistrar(
	TASK_PATH_LOOKAT, // Task Name
	TaskLibrary::CreateInstanceFunc(&PathLookatTask::CreateInstance) // Instance creation callback
);

} // namespace ai
