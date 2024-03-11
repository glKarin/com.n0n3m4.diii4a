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
#include "PathWaitTask.h"
#include "../Library.h"

#define MAXTIME 1000000 // put a limit on what we feed SEC2MS

namespace ai
{

PathWaitTask::PathWaitTask() :
	PathTask()
{}

PathWaitTask::PathWaitTask(idPathCorner* path) : 
	PathTask(path)
{
	_path = path;
}

// Get the name of this task
const idStr& PathWaitTask::GetName() const
{
	static idStr _name(TASK_PATH_WAIT);
	return _name;
}

void PathWaitTask::Init(idAI* owner, Subsystem& subsystem)
{
	PathTask::Init(owner, subsystem);

	idPathCorner* path = _path.GetEntity();

	// grayman #4046 - If there was some leftover time from the previous
	// path_wait task, apply that to this path_wait task. Otherwise,
	// calculate a new wait time.

	if ( (owner->m_pathWaitTaskEndtime > 0) && (owner->m_pathWaitTaskEndtime > gameLocal.time) )
	{
		_endtime = owner->m_pathWaitTaskEndtime;
	}
	else
	{
		float waittime = path->spawnArgs.GetFloat("wait", "0");
		float waitmax = path->spawnArgs.GetFloat("wait_max", "0");

		if ( waitmax > 0 )
		{
			waittime += (waitmax - waittime) * gameLocal.random.RandomFloat();
		}

		if ( waittime > MAXTIME ) // SEC2MS can't handle very large numbers
		{
			waittime = MAXTIME;
		}

		waittime = SEC2MS(waittime);

		_endtime = waittime + gameLocal.time;
	}
}

bool PathWaitTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("PathWaitTask performing.\r");

	idPathCorner* path = _path.GetEntity();
	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointers
	assert(path != NULL && owner != NULL);

	if (gameLocal.time >= _endtime)
	{
		// Trigger path targets, now that we're done waiting
		// grayman #3670 - need to keep the owner->Activate() calls to not break
		// existing maps, but the intent was path->Activate().
		owner->ActivateTargets(owner);
		path->ActivateTargets(owner);

		// NextPath();

		// Wait is done, fall back to PatrolTask
		DM_LOG(LC_AI, LT_INFO)LOGSTRING("Wait is done.\r");

		return true; // finish this task
	}
	return false;
}

void PathWaitTask::OnFinish(idAI* owner)
{
	// grayman #4046 - It's possible that this wait task was interrupted early, and
	// there's a chance it will be restarted when the interruption is over. So that
	// the restart doesn't try to wait the entire duration over again, let's save
	// _endtime in the AI, and check that when the AI starts a new path_wait task.

	if ( _endtime > gameLocal.time )
	{
		owner->m_pathWaitTaskEndtime = _endtime;
	}
	else
	{
		owner->m_pathWaitTaskEndtime = 0;
	}
}


// Save/Restore methods
void PathWaitTask::Save(idSaveGame* savefile) const
{
	PathTask::Save(savefile);

	savefile->WriteFloat(_endtime);
}

void PathWaitTask::Restore(idRestoreGame* savefile)
{
	PathTask::Restore(savefile);

	savefile->ReadFloat(_endtime);
}

PathWaitTaskPtr PathWaitTask::CreateInstance()
{
	return PathWaitTaskPtr(new PathWaitTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar PathWaitTaskRegistrar(
	TASK_PATH_WAIT, // Task Name
	TaskLibrary::CreateInstanceFunc(&PathWaitTask::CreateInstance) // Instance creation callback
);

} // namespace ai
