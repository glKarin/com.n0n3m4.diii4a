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



#include "ScriptTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

ScriptTask::~ScriptTask() {
	if (_thread)
		delete _thread;
}

ScriptTask::ScriptTask() :
	_thread(NULL)
{}

ScriptTask::ScriptTask(const idStr& functionName) :
	_functionName(functionName),
	_thread(NULL)
{}

// Get the name of this task
const idStr& ScriptTask::GetName() const
{
	static idStr _name(TASK_SCRIPT);
	return _name;
}

void ScriptTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	Task::Init(owner, subsystem);

	const function_t* scriptFunction = owner->scriptObject.GetFunction(_functionName);
	if (scriptFunction == NULL)
	{
		// Local function not found, check in global namespace
		scriptFunction = gameLocal.program.FindFunction(_functionName);
	}

	if (scriptFunction != NULL)
	{
		_thread = new idThread(scriptFunction);
		_thread->CallFunctionArgs(scriptFunction, true, "e", owner);
		_thread->DelayedStart(0);
		_thread->ManualDelete();
	}
	else
	{
		// script function not found!
		DM_LOG(LC_AI, LT_ERROR)LOGSTRING("ScriptTask could not find task %s.\r", _functionName.c_str());
		subsystem.FinishTask();
		return;
	}
}

bool ScriptTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("ScriptTask performing.\r");
	assert(_thread != NULL);

	if (_thread->IsDying())
	{
		// thread is done, return TRUE to terminate this task
		return true;
	}

	return false; // not finished yet
}

void ScriptTask::OnFinish(idAI* owner)
{
	if (_thread != NULL)
	{
		// We've got a non-NULL thread, this means it's still alive, end it now
		_thread->EndThread();
	}
}

void ScriptTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteString(_functionName);
	savefile->WriteObject(_thread);
}

void ScriptTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadString(_functionName);
	savefile->ReadObject(reinterpret_cast<idClass*&>(_thread));
}

ScriptTaskPtr ScriptTask::CreateInstance()
{
	return ScriptTaskPtr(new ScriptTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar scriptTaskRegistrar(
	TASK_SCRIPT, // Task Name
	TaskLibrary::CreateInstanceFunc(&ScriptTask::CreateInstance) // Instance creation callback
);

} // namespace ai
