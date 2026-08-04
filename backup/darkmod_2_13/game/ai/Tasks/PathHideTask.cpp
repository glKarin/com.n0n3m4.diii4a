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
#include "PathHideTask.h"
#include "../Library.h"

namespace ai
{

PathHideTask::PathHideTask() :
	PathTask()
{}

PathHideTask::PathHideTask(idPathCorner* path) :
	PathTask(path)
{
	_path = path;
}

// Get the name of this task
const idStr& PathHideTask::GetName() const
{
	static idStr _name(TASK_PATH_HIDE);
	return _name;
}

void PathHideTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	PathTask::Init(owner, subsystem);

	// Make invisible and nonsolid
	owner->Hide();
}

bool PathHideTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Path Hide task performing.\r");

	idPathCorner* path = _path.GetEntity();
	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointers
	assert(path != NULL && owner != NULL);

	// Move on to next target
	if (owner->IsHidden())
	{
		// Trigger path targets, now that we've hidden the owner
		// grayman #3670 - this activates owner targets, not path targets
		owner->ActivateTargets(owner);

		// NextPath();

		// Move is done, fall back to PatrolTask
		DM_LOG(LC_AI, LT_INFO)LOGSTRING("entity is hidden.\r");

		return true; // finish this task
	}
	return false;
}

PathHideTaskPtr PathHideTask::CreateInstance()
{
	return PathHideTaskPtr(new PathHideTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar pathHideTaskRegistrar(
	TASK_PATH_HIDE, // Task Name
	TaskLibrary::CreateInstanceFunc(&PathHideTask::CreateInstance) // Instance creation callback
);

} // namespace ai
