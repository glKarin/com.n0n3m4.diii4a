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



#include "DeadState.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& DeadState::GetName() const
{
	static idStr _name(STATE_DEAD);
	return _name;
}

void DeadState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("DeadState initialised.\r");
	assert(owner);

	// Stop move!
	owner->StopMove(MOVE_STATUS_DONE);
	owner->GetMemory().StopReacting(); // grayman #3559

	// Clear all the subsystems, this might cause animstate changes
	owner->movementSubsystem->ClearTasks();
	owner->senseSubsystem->ClearTasks();
	owner->actionSubsystem->ClearTasks();
	owner->commSubsystem->ClearTasks();
	owner->searchSubsystem->ClearTasks(); // grayman #3857

	// Reset anims
	owner->StopAnim(ANIMCHANNEL_TORSO, 0);
	owner->StopAnim(ANIMCHANNEL_LEGS, 0);
	owner->StopAnim(ANIMCHANNEL_HEAD, 0);

	// greebo: Play death anim only for AI which want to play them
	if (owner->spawnArgs.GetBool("enable_death_anim", "0"))
	{
		owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Death", 0);
		owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Death", 0);

		owner->SetWaitState(ANIMCHANNEL_TORSO, "death");
		owner->SetWaitState(ANIMCHANNEL_LEGS, "death");
	}

	// Play the death anim on the head channel regardless
	owner->SetAnimState(ANIMCHANNEL_HEAD, "Head_Death", 0);

	// greebo: Set the waitstate, this gets cleared by 
	// the script function when the animation is done.
	owner->SetWaitState(ANIMCHANNEL_HEAD, "death");

	// Don't do anything else, the death animation will finish in a few frames
	// and the AI is done afterwards.

	// Swap skin on death if required
	idStr deathSkin;
	if (owner->spawnArgs.GetString("skin_dead", "", deathSkin))
	{
		owner->Event_SetSkin(deathSkin);
	}

	// Run a death FX
	idStr deathFX;
	if (owner->spawnArgs.GetString("fx_on_death", "", deathFX))
	{
		owner->Event_StartFx(deathFX);
	}

	// Run a death script, if applicable
	// grayman #3679: death scripts can now retrieve who is responsible for the death
	// Use:
	// entity attacker = ai->getAttacker(); // returns attacking entity (could be player or something else)
	// float playerResponsible = ai->getPlayerResponsibleForDeath(); // returns 1 if true, 0 if false
	idStr deathScript;
	if (owner->spawnArgs.GetString("death_script", "", deathScript))
	{
		const function_t* scriptFunction = owner->scriptObject.GetFunction(deathScript);
		if (scriptFunction == NULL)
		{
			DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Action: %s not found in local space, checking for global.\r", deathScript.c_str());
			scriptFunction = gameLocal.program.FindFunction(deathScript.c_str());
		}

		if (scriptFunction)
		{
			DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Running Death Script\r");

			idThread* thread = new idThread(scriptFunction);
			thread->CallFunctionArgs(scriptFunction, true, "e", owner);
			thread->DelayedStart(0);

			// Start execution immediately
			thread->Execute();
		}
		else
		{
			DM_LOG(LC_AI, LT_ERROR)LOGSTRING("Death Script not found! [%s]\r", deathScript.c_str());
		}
	}

	DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Death state entered for AI: %s, frame %d\r", owner->name.c_str(), gameLocal.framenum);

	_waitingForDeath = true;
}

// Gets called each time the mind is thinking
void DeadState::Think(idAI* owner)
{
	if (_waitingForDeath 
		&& idStr(owner->WaitState(ANIMCHANNEL_TORSO)) != "death"
		&& idStr(owner->WaitState(ANIMCHANNEL_LEGS)) != "death"
		&& idStr(owner->WaitState(ANIMCHANNEL_HEAD)) != "death") 
	{
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Post Death state entered for AI: %s, frame %d\r", owner->name.c_str(), gameLocal.framenum);
		owner->PostDeath();
		_waitingForDeath = false;
	}
}

// Save/Restore methods
void DeadState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);
	savefile->WriteBool(_waitingForDeath);
}

void DeadState::Restore(idRestoreGame* savefile) 
{
	State::Restore(savefile);
	savefile->ReadBool(_waitingForDeath);
}

StatePtr DeadState::CreateInstance()
{
	return StatePtr(new DeadState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar deadStateRegistrar(
	STATE_DEAD, // Task Name
	StateLibrary::CreateInstanceFunc(&DeadState::CreateInstance) // Instance creation callback
);

} // namespace ai
