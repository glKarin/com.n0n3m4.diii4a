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



#include "KnockedOutState.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& KnockedOutState::GetName() const
{
	static idStr _name(STATE_KNOCKED_OUT);
	return _name;
}

void KnockedOutState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("KnockedOutState initialised.\r");
	assert(owner);
	
	// Clear all the subsystems
	owner->movementSubsystem->ClearTasks();
	owner->senseSubsystem->ClearTasks();
	owner->actionSubsystem->ClearTasks();
	owner->commSubsystem->ClearTasks();
	owner->searchSubsystem->ClearTasks(); // grayman #3857

	_waitingForKnockout = true;

	// Stop move!
	owner->StopMove(MOVE_STATUS_DONE);
	owner->GetMemory().StopReacting(); // grayman #3559

	//owner->StopAnim(ANIMCHANNEL_TORSO, 0);
	//owner->StopAnim(ANIMCHANNEL_LEGS, 0);
	//owner->StopAnim(ANIMCHANNEL_HEAD, 0);

	// angua: disabled for Thief's Den release
	// anims didn't look good and produced problems
/*
	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_KO", 0);
	owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_KO", 0);

	// greebo: Set the waitstate, this gets cleared by 
	// the script function when the animation is done.
	owner->SetWaitState(ANIMCHANNEL_TORSO, "knock_out");
	owner->SetWaitState(ANIMCHANNEL_LEGS, "knock_out");
	// Don't do anything else, the KO animation will finish in a few frames
	// and the AI is done afterwards.
*/
	if (owner->m_koState != KO_FALL) // grayman #3699 - skip animations for a falling KO
	{
		owner->SetAnimState(ANIMCHANNEL_HEAD, "Head_KO", 0);
		owner->SetWaitState(ANIMCHANNEL_HEAD, "knock_out");
	}

	// Tels: #3297 Run a KO FX
	idStr koFX;
	if (owner->spawnArgs.GetString("fx_on_ko", "", koFX))
	{
		owner->Event_StartFx(koFX);
	}

	// Tels: #3297 Run a ko script, if applicable
	// TODO: We should figure out who is responsible for the KO and pass it along to the script
	idStr koScript;
	if (owner->spawnArgs.GetString("ko_script", "", koScript))
	{
		const function_t* scriptFunction = owner->scriptObject.GetFunction(koScript);
		if (scriptFunction == NULL)
		{
			DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Action: %s not found in local space, checking for global.\r", koScript.c_str());
			scriptFunction = gameLocal.program.FindFunction(koScript.c_str());
		}

		if (scriptFunction)
		{
			DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Running KO Script\r");

			idThread* thread = new idThread(scriptFunction);
			thread->CallFunctionArgs(scriptFunction, true, "e", owner);
			thread->DelayedStart(0);

			// Start execution immediately
			thread->Execute();
		}
		else
		{
			DM_LOG(LC_AI, LT_ERROR)LOGSTRING("KO Script not found! [%s]\r", koScript.c_str());
		}
	}
}

// Gets called each time the mind is thinking
void KnockedOutState::Think(idAI* owner)
{
	// wait until the animations are finished, then enter the knockout state
	if (   _waitingForKnockout 
		&& idStr(owner->WaitState(ANIMCHANNEL_HEAD)) != "knock_out"
		&& idStr(owner->WaitState(ANIMCHANNEL_TORSO)) != "knock_out"
		&& idStr(owner->WaitState(ANIMCHANNEL_LEGS)) != "knock_out")
	{
		owner->PostKnockOut();
		_waitingForKnockout = false;
	}
}

// Save/Restore methods
void KnockedOutState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);
	savefile->WriteBool(_waitingForKnockout);
}

void KnockedOutState::Restore(idRestoreGame* savefile) 
{
	State::Restore(savefile);
	savefile->ReadBool(_waitingForKnockout);
}

StatePtr KnockedOutState::CreateInstance()
{
	return StatePtr(new KnockedOutState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar knockedOutStateRegistrar(
	STATE_KNOCKED_OUT, // Task Name
	StateLibrary::CreateInstanceFunc(&KnockedOutState::CreateInstance) // Instance creation callback
);

} // namespace ai
