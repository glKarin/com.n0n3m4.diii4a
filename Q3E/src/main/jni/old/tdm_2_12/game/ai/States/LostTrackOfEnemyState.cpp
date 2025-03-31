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



#include "LostTrackOfEnemyState.h"
#include "../Memory.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Tasks/IdleAnimationTask.h" // grayman #3857
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& LostTrackOfEnemyState::GetName() const
{
	static idStr _name(STATE_LOST_TRACK_OF_ENEMY);
	return _name;
}

void LostTrackOfEnemyState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("LostTrackOfEnemyState initialised.\r");
	assert(owner);

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	// Draw weapon, if we haven't already
	if (!owner->GetAttackFlag(COMBAT_MELEE) && !owner->GetAttackFlag(COMBAT_RANGED))
	{
		// grayman #3331 - draw your ranged weapon if you have one, otherwise draw your melee weapon.
		// Note that either weapon could be drawn, but if we default to melee, AI with ranged and
		// melee weapons will draw their melee weapon, and we'll never see ranged weapons get drawn.
		// Odds are that the enemy is nowhere nearby anyway, since we're just searching.

		if ( ( owner->GetNumRangedWeapons() > 0 ) && !owner->spawnArgs.GetBool("unarmed_ranged","0") )
		{
			owner->DrawWeapon(COMBAT_RANGED);
		}
		else if ( ( owner->GetNumMeleeWeapons() > 0 ) && !owner->spawnArgs.GetBool("unarmed_melee","0") )
		{
			owner->DrawWeapon(COMBAT_MELEE);
		}
	}

	// Setup the search parameters

	// grayman #3507 - if there's no 'lastVisibleReachableEnemyPos',
	// use 'lastVisibleEnemyPos' instead

	idVec3 v3Zero;
	v3Zero.Zero();
	if (owner->lastVisibleReachableEnemyPos != v3Zero)
	{
		memory.alertPos = owner->lastVisibleReachableEnemyPos;
	}
	else
	{
		memory.alertPos = owner->lastVisibleEnemyPos;
	}

	// grayman #3857 - move alert setup into one method
	SetUpSearchData(EAlertTypeLostTrackOfEnemy, memory.alertPos, NULL, false, 0);

	// Forget about the enemy, prevent UpdateEnemyPosition from "cheating".
	owner->ClearEnemy();

	// Enqueue a lost track of enemy bark
	owner->commSubsystem->AddCommTask(
		CommunicationTaskPtr(new SingleBarkTask("snd_lostTrackOfEnemy"))
	);

	if (cv_ai_debug_transition_barks.GetBool())
	{
		gameLocal.Printf("%d: %s loses track of the enemy, barks 'snd_lostTrackOfEnemy'\n",gameLocal.time,owner->GetName());
	}

	// For now, clear the action tasks and movement tasks
	owner->actionSubsystem->ClearTasks();
	owner->movementSubsystem->ClearTasks();

	owner->GetMind()->EndState();
}

// Gets called each time the mind is thinking
void LostTrackOfEnemyState::Think(idAI* owner)
{
}

StatePtr LostTrackOfEnemyState::CreateInstance()
{
	return StatePtr(new LostTrackOfEnemyState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar lostTrackOfEnemyStateRegistrar(
	STATE_LOST_TRACK_OF_ENEMY, // Task Name
	StateLibrary::CreateInstanceFunc(&LostTrackOfEnemyState::CreateInstance) // Instance creation callback
);

} // namespace ai
