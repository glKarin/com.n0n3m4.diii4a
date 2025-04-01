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



#include "CommWaitTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

CommWaitTask::CommWaitTask() :
	CommunicationTask(""),
	_duration(0),
	_endTime(-1)
{}

CommWaitTask::CommWaitTask(int duration, int priority) :
	CommunicationTask(""),
	_duration(duration),
	_endTime(-1)
{
	_priority = priority;
}

// Get the name of this task
const idStr& CommWaitTask::GetName() const
{
	static idStr _name(TASK_COMM_WAIT);
	return _name;
}

void CommWaitTask::Init(idAI* owner, Subsystem& subsystem)
{
	CommunicationTask::Init(owner, subsystem);

	_endTime = gameLocal.time + _duration;
}

bool CommWaitTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("CommWaitTask performing.\r");

	return (gameLocal.time >= _endTime);
}

// Save/Restore methods
void CommWaitTask::Save(idSaveGame* savefile) const
{
	CommunicationTask::Save(savefile);

	savefile->WriteInt(_duration);
	savefile->WriteInt(_endTime);
}

void CommWaitTask::Restore(idRestoreGame* savefile)
{
	CommunicationTask::Restore(savefile);

	savefile->ReadInt(_duration);
	savefile->ReadInt(_endTime);
}

CommWaitTaskPtr CommWaitTask::CreateInstance()
{
	return CommWaitTaskPtr(new CommWaitTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar commWaitTaskRegistrar(
	TASK_COMM_WAIT, // Task Name
	TaskLibrary::CreateInstanceFunc(&CommWaitTask::CreateInstance) // Instance creation callback
);

} // namespace ai
