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



#include "SingleBarkTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

SingleBarkTask::SingleBarkTask() :
	CommunicationTask(""),
	_startDelay(0),
	_allowDuringAnim(true), // grayman #3182
	_endTime(-1)
{}

SingleBarkTask::SingleBarkTask(const idStr& soundName, 
							   const CommMessagePtr& message, 
							   int startDelay,
							   bool allowDuringAnim) // grayman #3182
:	CommunicationTask(soundName),
	_message(message),
	_startDelay(startDelay),
	_allowDuringAnim(allowDuringAnim), // grayman #3182
	_endTime(-1)
{}

// Get the name of this task
const idStr& SingleBarkTask::GetName() const
{
	static idStr _name(TASK_SINGLE_BARK);
	return _name;
}

void SingleBarkTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	CommunicationTask::Init(owner, subsystem);

	// End time is -1 until the bark has been emitted
	_endTime = -1;

	// Set up the start time
	_barkStartTime = gameLocal.time + _startDelay;
}

bool SingleBarkTask::Perform(Subsystem& subsystem)
{
	if (gameLocal.time < _barkStartTime)
	{
		return false; // waiting for start delay to pass
	}

	// This task may not be performed with empty entity pointers
	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	// If an endtime has been set, the bark is already playing
	if (_endTime > 0)
	{
		// Finish the task when the time is over
		return (gameLocal.time >= _endTime);
	}
	
	if (_soundName.IsEmpty())
	{
		DM_LOG(LC_AI, LT_ERROR)LOGSTRING("SingleBarkTask has empty soundname, ending task.\r");
		return true;
	}

	// No end time set yet, emit our bark

	// grayman #2169 - no barks while underwater

	// grayman #3182 - no barks when an idle animation is playing and _allowDuringAnim == false
	// An idle animation that includes a voice frame command will have set
	// the wait state to 'idle'. An idle animation that has no voice frame
	// command will have set the wait state to 'idle_no_voice'.

	_barkLength = 0;
	bool canPlay = true;
	if ( ( idStr(owner->WaitState() ) == "idle" ) && !_allowDuringAnim ) // grayman #3182
	{
		canPlay = false;
	}

	if ( canPlay && !owner->MouthIsUnderwater() ) // grayman #3182
	{
		int msgTag = 0; // grayman #3355
		// Push the message and play the sound
		if (_message != NULL)
		{
			// Setup the message to be propagated, if we have one
			msgTag = gameLocal.GetNextMessageTag(); // grayman #3355
			owner->AddMessage(_message,msgTag);
		}

		owner->GetMind()->GetMemory().currentlyBarking = true; // grayman #3182 - idle anims w/voices cannot start
															   // until this bark is finished
		_barkLength = owner->PlayAndLipSync(_soundName, "talk1", msgTag); // grayman #3355

		_barkStartTime = gameLocal.time; // grayman #3857
		_endTime = _barkStartTime + _barkLength; // grayman #3857

		// Sanity check the returned length
		if (_barkLength == 0)
		{
			DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Received 0 sound length when playing %s.\r", _soundName.c_str());
		}

		return false; // grayman #3857
	}

	// grayman #3857 - moved up
	//_barkStartTime = gameLocal.time;
	//_endTime = _barkStartTime + _barkLength;

	// End the task as soon as we've finished playing the sound
	//return !IsBarking();
	return true; // grayman #3857
}

void SingleBarkTask::OnFinish(idAI* owner)
{
	owner->GetMind()->GetMemory().currentlyBarking = false;
}

void SingleBarkTask::SetSound(const idStr& soundName)
{
	_soundName = soundName;
}

void SingleBarkTask::SetMessage(const CommMessagePtr& message)
{
	_message = message;
}

// Save/Restore methods
void SingleBarkTask::Save(idSaveGame* savefile) const
{
	CommunicationTask::Save(savefile);

	savefile->WriteInt(_startDelay);
	savefile->WriteBool(_allowDuringAnim); // grayman #3182

	savefile->WriteInt(_endTime);

	savefile->WriteBool(_message != NULL);
	if (_message != NULL)
	{
		_message->Save(savefile);
	}
}

void SingleBarkTask::Restore(idRestoreGame* savefile)
{
	CommunicationTask::Restore(savefile);

	savefile->ReadInt(_startDelay);
	savefile->ReadBool(_allowDuringAnim); // grayman #3182
	savefile->ReadInt(_endTime);

	bool hasMessage;
	savefile->ReadBool(hasMessage);
	if (hasMessage)
	{
		_message = CommMessagePtr(new CommMessage);
		_message->Restore(savefile);
	}
	else
	{
		_message = CommMessagePtr();
	}
}

SingleBarkTaskPtr SingleBarkTask::CreateInstance()
{
	return SingleBarkTaskPtr(new SingleBarkTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar singleBarkTaskRegistrar(
	TASK_SINGLE_BARK, // Task Name
	TaskLibrary::CreateInstanceFunc(&SingleBarkTask::CreateInstance) // Instance creation callback
);

} // namespace ai
