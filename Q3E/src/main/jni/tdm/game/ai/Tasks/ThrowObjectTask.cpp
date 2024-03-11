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



#include "ThrowObjectTask.h"
#include "../States/TakeCoverState.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

// Get the name of this task
const idStr& ThrowObjectTask::GetName() const
{
	static idStr _name(TASK_THROW_OBJECT);
	return _name;
}

void ThrowObjectTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	Task::Init(owner, subsystem);

	_projectileDelayMin = SEC2MS(owner->spawnArgs.GetFloat("outofreach_projectile_delay_min", "7.0"));
	_projectileDelayMax = SEC2MS(owner->spawnArgs.GetFloat("outofreach_projectile_delay_max", "10.0"));

	// grayman #3331 - add randomness to first throw, anywhere from 2 to 4 seconds
	_nextThrowObjectTime = static_cast<int>(gameLocal.time + 2000 + 2000*gameLocal.random.RandomFloat());
}

bool ThrowObjectTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Throw Object Task performing.\r");

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);
	
	//Memory& memory = owner->GetMemory();
	idActor* enemy = owner->GetEnemy();

	if (enemy == NULL)
	{
		return true;
	}

	// grayman #3775 - no need to throw if you're drawing a weapon
	if (idStr(owner->WaitState()) == "draw")
	{
		return true;
	}

	if (owner->AI_ENEMY_VISIBLE)
	{
		// Turn to the player
		// We don't need the check for enemy == NULL, since this should not be the case if the enemy is visible
		owner->TurnToward(enemy->GetEyePosition());

		// grayman #3331 - look at enemy's eyes
		owner->Event_LookAtPosition(enemy->GetEyePosition(),3.0f);
	}

	// angua: Throw after the delay has expired, but only if
	// the player is visible and object throwing is enabled for this AI
	if ( ( _nextThrowObjectTime <= gameLocal.time ) && 
		 owner->AI_ENEMY_VISIBLE && // grayman #4343
		 owner->spawnArgs.GetBool("outofreach_projectile_enabled", "0"))
	{
		idStr waitState(owner->WaitState());
		if (waitState != "throw")
		{
			// Waitstate is not matching, this means that the animation 
			// can be started.
			owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Throw", 5);
			owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Throw", 5);

			// Set the waitstate, this gets cleared by 
			// the script function when the animation is done.
			owner->SetWaitState("throw");
		}

		// Set next throwing time
		_nextThrowObjectTime = static_cast<int>(gameLocal.time + _projectileDelayMin
							+ gameLocal.random.RandomFloat() * (_projectileDelayMax - _projectileDelayMin));
	}
	return false; // not finished yet
}

void ThrowObjectTask::OnFinish(idAI* owner)
{
	// Set animations back to idle
	owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", 0);
	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 0);
	owner->SetWaitState("");
}

void ThrowObjectTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteInt(_projectileDelayMin);
	savefile->WriteInt(_projectileDelayMax);
	savefile->WriteInt(_nextThrowObjectTime);
}

void ThrowObjectTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadInt(_projectileDelayMin);
	savefile->ReadInt(_projectileDelayMax);
	savefile->ReadInt(_nextThrowObjectTime);
}

ThrowObjectTaskPtr ThrowObjectTask::CreateInstance()
{
	return ThrowObjectTaskPtr(new ThrowObjectTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar throwObjectTaskRegistrar(
	TASK_THROW_OBJECT, // Task Name
	TaskLibrary::CreateInstanceFunc(&ThrowObjectTask::CreateInstance) // Instance creation callback
);

} // namespace ai
