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
#include "PathWaitForTriggerTask.h"
#include "../Library.h"

namespace ai
{

PathWaitForTriggerTask::PathWaitForTriggerTask() :
	PathTask()
{}

PathWaitForTriggerTask::PathWaitForTriggerTask(idPathCorner* path) : 
	PathTask(path)
{
	_path = path;
}

// Get the name of this task
const idStr& PathWaitForTriggerTask::GetName() const
{
	static idStr _name(TASK_PATH_WAIT_FOR_TRIGGER);
	return _name;
}

void PathWaitForTriggerTask::Init(idAI* owner, Subsystem& subsystem)
{
	PathTask::Init(owner, subsystem);

	owner->AI_ACTIVATED = false;
}

bool PathWaitForTriggerTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Path WaitForTrigger Task performing.\r");

	//idPathCorner* path = _path.GetEntity();
	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointers
	assert( owner != NULL );

	if (owner->AI_ACTIVATED)
	{
		owner->AI_ACTIVATED = false;

		// Trigger path targets, now that the owner has been activated
		// grayman #3670 - this activates owner targets, not path targets
		owner->ActivateTargets(owner);

		// NextPath();

		// Move is done, fall back to PatrolTask
		DM_LOG(LC_AI, LT_INFO)LOGSTRING("Waiting for trigger is done.\r");

		return true; // finish this task
	}
	return false;
}

PathWaitForTriggerTaskPtr PathWaitForTriggerTask::CreateInstance()
{
	return PathWaitForTriggerTaskPtr(new PathWaitForTriggerTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar pathWaitForTriggerTaskRegistrar(
	TASK_PATH_WAIT_FOR_TRIGGER, // Task Name
	TaskLibrary::CreateInstanceFunc(&PathWaitForTriggerTask::CreateInstance) // Instance creation callback
);

} // namespace ai
