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



#include "IdleState.h"
#include "AlertIdleState.h"
#include "../Memory.h"
#include "../Tasks/RandomHeadturnTask.h"
#include "../Tasks/AnimalPatrolTask.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Tasks/MoveToPositionTask.h"
#include "../Tasks/IdleAnimationTask.h"
#include "../Tasks/RepeatedBarkTask.h"
#include "ObservantState.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& AlertIdleState::GetName() const
{
	static idStr _name(STATE_ALERT_IDLE);
	return _name;
}

void AlertIdleState::Init(idAI* owner)
{
	// Init state class first
	// Note: we do not call IdleState::Init
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("AlertIdleState initialised.\r");
	assert(owner);

	_alertLevelDecreaseRate = 0.005f;

	_startSleeping = owner->spawnArgs.GetBool("sleeping", "0");
	_startSitting = owner->spawnArgs.GetBool("sitting", "0");

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner))
	{
		return;
	}

	InitialiseMovement(owner);
	InitialiseCommunication(owner);

	// grayman #2603 - clear recent alerts, which allows us to see new, lower-weighted, alerts
	Memory& memory = owner->GetMemory();
	memory.alertClass = EAlertNone;
	memory.alertType = EAlertTypeNone;
	memory.alertPos = idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY); // grayman #3413

	memory.agitatedSearched = false; // grayman #3496

	memory.mightHaveSeenPlayer = false; // grayman #3515
		
	owner->m_recentHighestAlertLevel = 0; // grayman #3472

	int idleBarkIntervalMin = SEC2MS(owner->spawnArgs.GetInt("alert_idle_bark_interval_min", "40"));
	int idleBarkIntervalMax = SEC2MS(owner->spawnArgs.GetInt("alert_idle_bark_interval_max", "120"));

	// grayman #3848
	idStr bark = "";
	if (owner->m_lastKilled.GetEntity() != NULL)
	{
		bark = "snd_alert_idle_victor";
	}
	else
	{
		bark = "snd_alert_idle";
	}
	owner->commSubsystem->AddCommTask(
			CommunicationTaskPtr(new RepeatedBarkTask(bark, idleBarkIntervalMin, idleBarkIntervalMax))
		);

	// Initialise the animation state
	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 0);
	owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", 0);

	// The sensory system does its Idle tasks
	owner->senseSubsystem->ClearTasks();
	owner->senseSubsystem->PushTask(RandomHeadturnTask::CreateInstance());

	if (!owner->GetAttackFlag(COMBAT_MELEE) && !owner->GetAttackFlag(COMBAT_RANGED))
	{
		// grayman #3331 - draw your ranged weapon if you have one, otherwise draw your melee weapon.
		// Note that either weapon could be drawn, but if we default to melee, AI with ranged and
		// melee weapons will draw their melee weapon, and we'll never see ranged weapons get drawn.
		// Odds are that the enemy is nowhere nearby anyway, since we're just searching.

		bool shouldDrawMeleeWeapon  = ( ( owner->GetNumMeleeWeapons() > 0 )  && !owner->spawnArgs.GetBool("unarmed_melee","0") );
		bool shouldDrawRangedWeapon = ( ( owner->GetNumRangedWeapons() > 0 ) && !owner->spawnArgs.GetBool("unarmed_ranged","0") );

		// grayman #3424 - don't pause to draw a weapon if you have no weapon to draw

		if ( shouldDrawMeleeWeapon || shouldDrawRangedWeapon )
		{
			// grayman #2920 - if patrolling, stop for a moment
			// to draw weapon, then continue on. This lets the "walk_alerted"
			// animation take hold.

			bool startMovingAgain = false;
			if ( owner->AI_FORWARD )
			{
				startMovingAgain = true;
				owner->movementSubsystem->ClearTasks();
				owner->StopMove(MOVE_STATUS_DONE);
			}

			// grayman #3331 - draw your ranged weapon if you have one, otherwise draw your melee weapon.
			// Note that either weapon could be drawn, but if we default to melee, AI with ranged and
			// melee weapons will draw their melee weapon, and we'll never see ranged weapons get drawn.
			// Odds are that the enemy is nowhere nearby anyway, since we're just searching.

			if ( shouldDrawRangedWeapon )
			{
				owner->DrawWeapon(COMBAT_RANGED);
			}
			else if ( shouldDrawMeleeWeapon )
			{
				owner->DrawWeapon(COMBAT_MELEE);
			}

			if ( startMovingAgain )
			{
				// allow enough time for weapon to be drawn,
				// then start patrolling again
				owner->PostEventMS(&AI_RestartPatrol,1500);
			}
		}
	}

	// Let the AI update their weapons (make them nonsolid)
	owner->UpdateAttachmentContents(false);
}

idStr AlertIdleState::GetInitialIdleBark(idAI* owner)
{
	Memory& memory = owner->GetMemory();

	// Decide what sound it is appropriate to play
	idStr soundName("");

	if ( !owner->m_RelightingLight &&	// grayman #2603 - No rampdown bark if relighting a light.
		 !owner->m_ExaminingRope ) 		// grayman #2872 - No rampdown bark if examining a rope.
	{
		EAlertClass aclass = memory.alertClass;
		if (owner->m_justKilledSomeone) // grayman #2816 - no bark, since we barked about the death when it happened
		{
			owner->m_justKilledSomeone = false; // but turn off the flag
		}
		else if (owner->m_recentHighestAlertLevel >= owner->thresh_4)
		{
			// has gone up to at least Agitated Searching
			soundName = "snd_alertdown0SeenEvidence";
		}
		else if (owner->m_recentHighestAlertLevel >= owner->thresh_2) // has gone up to Suspicious or Searching
		{
			// grayman #3472 - no longer needed
//			if (aclass == EAlertVisual_3) // grayman #3424
//			{
//				soundName = "snd_alertdown0SeenEvidence";
//			}
			if ( ( aclass == EAlertVisual_2 ) || ( aclass == EAlertVisual_4 ) ) // grayman #2603, grayman #3498
			{
				soundName = "snd_alertdown0sus";
			}
			else if (aclass == EAlertVisual_1) // grayman #3182
			{
				if (memory.alertType != EAlertTypeMissingItem)
				{
					soundName = "snd_alertdown0s";
				}
			}
			else if (aclass == EAlertAudio)
			{
				soundName = "snd_alertdown0h";
			}
			else if (aclass != EAlertNone) // grayman #3182
			{
				soundName = "snd_alertdown0";
			}
		}
	}

	return soundName;
}

StatePtr AlertIdleState::CreateInstance()
{
	return StatePtr(new AlertIdleState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar alertIdleStateRegistrar(
	STATE_ALERT_IDLE, // Task Name
	StateLibrary::CreateInstanceFunc(&AlertIdleState::CreateInstance) // Instance creation callback
);

} // namespace ai
