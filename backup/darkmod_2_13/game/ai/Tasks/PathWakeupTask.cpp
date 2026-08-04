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
#include "PathWakeupTask.h"
#include "PathTurnTask.h"
#include "../Library.h"

#define MAXTIME 1000000 // put a limit on what we feed SEC2MS

namespace ai
{

PathWakeupTask::PathWakeupTask() :
	PathTask()
{}

PathWakeupTask::PathWakeupTask(idPathCorner* path) :
	PathTask(path)
{
	_path = path;
}

// Get the name of this task
const idStr& PathWakeupTask::GetName() const
{
	static idStr _name(TASK_PATH_WAKEUP);
	return _name;
}

void PathWakeupTask::Init(idAI* owner, Subsystem& subsystem)
{
	// grayman #3820 - This task allows a sitting, sleeping AI to wake up and
	// stay in his chair, w/o standing up.

	PathTask::Init(owner, subsystem);

	owner->WakeUp();
}

bool PathWakeupTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("PathWakeupTask performing.\r");

	idAI* owner = _owner.GetEntity();

	// This task may not be performed with an empty owner pointer
	assert(owner != NULL);

	// Exit when no longer sleeping
	if ( idStr(owner->WaitState()) != "wake_up" )
	{
		return true;
	}

	return false;
}

void PathWakeupTask::OnFinish(idAI* owner)
{
	owner->GetMind()->GetMemory().playIdleAnimations = true;
}

// Save/Restore methods
void PathWakeupTask::Save(idSaveGame* savefile) const
{
	PathTask::Save(savefile);

	savefile->WriteInt(_waitEndTime);
}

void PathWakeupTask::Restore(idRestoreGame* savefile)
{
	PathTask::Restore(savefile);

	savefile->ReadInt(_waitEndTime);
}

PathWakeupTaskPtr PathWakeupTask::CreateInstance()
{
	return PathWakeupTaskPtr(new PathWakeupTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar pathWakeupTaskRegistrar(
	TASK_PATH_WAKEUP, // Task Name
	TaskLibrary::CreateInstanceFunc(&PathWakeupTask::CreateInstance) // Instance creation callback
);

} // namespace ai
