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



#include "PainState.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Memory.h"
#include "../Library.h"
#include "FleeState.h"	 // grayman #3331

namespace ai
{

// Get the name of this state
const idStr& PainState::GetName() const
{
	static idStr _name(STATE_PAIN);
	return _name;
}

void PainState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("PainState initialised.\r");
	assert(owner);

	// grayman #3424 - if already playing a pain anim, skip this one

	if ( idStr(owner->WaitState()) == "pain" )
	{
		// End this state
		owner->GetMind()->EndState();
		return;
	}

	Memory& memory = owner->GetMemory();

	// grayman #4406 - If sitting or sleeping, don't allow the
	// pain animation, because it screws up these other animations.

	moveType_t moveType = owner->GetMoveType();
	if ( moveType == MOVETYPE_SIT ||
		moveType == MOVETYPE_SLEEP ||
		moveType == MOVETYPE_SIT_DOWN ||
		moveType == MOVETYPE_FALL_ASLEEP || // grayman #3820 - was MOVETYPE_LAY_DOWN
		moveType == MOVETYPE_GET_UP ||
		moveType == MOVETYPE_WAKE_UP ) // grayman #3820 - was MOVETYPE_GET_UP_FROM_LYING
	{
		// Set end time
		_stateEndTime = gameLocal.time;
	}
	else
	{
		owner->StopMove(MOVE_STATUS_DONE); // grayman #3857

		// Play the animation
		owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Pain", 4);
		owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Pain", 4);

		owner->SetWaitState("pain");
		owner->SetWaitState(ANIMCHANNEL_TORSO, "pain");
		owner->SetWaitState(ANIMCHANNEL_LEGS, "pain");

		// Set end time
		_stateEndTime = gameLocal.time + 5000;
	}

	// grayman #3140 - if drowning, skip issuing a message. The drowning
	// sound effect is handled in idActor::Damage().
	if ( memory.causeOfPain != EPC_Drown )
	{
		memory.alertPos = owner->GetPhysics()->GetOrigin();

		// grayman #3857 - Ask others to join your existing search
		// by yelling out. If not searching, create a new event
		// and attempt to rally others to you.
		// problem: the "ouch" bark is issued elsewhere. "ouch"
		// barks in and of themselves shouldn't be broadcast to bring others to you.
		// other barks in other areas of the code should handle that.
		/*
		if (owner->m_searchID <= 0)
		{
			memory.currentSearchEventID = owner->LogSuspiciousEvent(E_EventTypeEnemy,memory.alertPos,NULL);
		}

		// Do a single bark and assemble an AI message
		CommMessagePtr message = CommMessagePtr(new CommMessage(
			CommMessage::RequestForHelp_CommType, // grayman #3857 - asking for a response
			//CommMessage::DetectedEnemy_CommType,  // grayman #3857 - this does nothing when no entity (parameter 4) is provided
			owner, NULL, // from this AI to anyone
			NULL,
			memory.alertPos,
			memory.currentSearchEventID // grayman #3857 (was '0')
		));

		owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask("snd_pain_large", message)));

		if (cv_ai_debug_transition_barks.GetBool())
		{
			gameLocal.Printf("%d: %s is hurt, barks 'snd_pain_large'\n",gameLocal.time,owner->GetName());
		}
		*/
	}
}

// Gets called each time the mind is thinking
void PainState::Think(idAI* owner)
{
	if ( ( gameLocal.time >= _stateEndTime ) || 
		 ( idStr(owner->WaitState()) != "pain" ) ) 
	{
		bool willBark = ( owner->AI_AlertLevel < owner->thresh_5 ); // don't bark a response if in combat

		bool willFlee = ( ( ( owner->GetNumMeleeWeapons()  == 0 ) && ( owner->GetNumRangedWeapons() == 0 ) ) ||
							  owner->spawnArgs.GetBool("is_civilian", "0") );

		// grayman #3140 - what caused this pain?

		Memory& memory = owner->GetMemory();
		if ( memory.causeOfPain == EPC_Drown )
		{
			// no bark and no fleeing if drowning
			willBark = false;
			willFlee = false;
		}
		else if ( memory.causeOfPain == EPC_Projectile )
		{
			// If fleeing, snd_taking_fire will be played at the start of the flee state,
			// so there's no reason to play it here.

			// If not fleeing, play snd_taking_fire here.

			willBark = !willFlee;
			memory.posEnemySeen = owner->GetPhysics()->GetOrigin();
		}

		if ( willBark )
		{
			// grayman #3140 - Emit the snd_taking_fire bark

			// This will hold the message to be delivered with the bark
			CommMessagePtr message;
	
			message = CommMessagePtr(new CommMessage(
				CommMessage::RequestForHelp_CommType, 
				owner, NULL, // from this AI to anyone 
				NULL,
				owner->GetPhysics()->GetOrigin(),
				memory.currentSearchEventID // grayman #3857 (was '0')
			));

			owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask("snd_taking_fire", message)));

			if (cv_ai_debug_transition_barks.GetBool())
			{
				gameLocal.Printf("%d: %s is hurt, barks 'snd_taking_fire'\n",gameLocal.time,owner->GetName());
			}
		}

		if ( willFlee && !memory.fleeing) // grayman #3331 - civilians and unarmed AI should flee // grayman #3847 - already fleeing?
		{
			if ( owner->AI_AlertLevel >= owner->thresh_5 ) // grayman #3847
			{
				owner->fleeingEvent = false; // I'm fleeing an enemy
				owner->fleeingFromPerson = owner->GetEnemy(); // grayman #3847
			}
			else
			{
				owner->fleeingEvent = true; // I'm fleeing the scene, not fleeing an enemy
				owner->fleeingFromPerson = NULL; // grayman #3847
			}
			owner->fleeingFrom = owner->GetPhysics()->GetOrigin(); // grayman #3848
			owner->emitFleeBarks = true; // grayman #3474
			if (!memory.fleeing) // grayman #3847 - only flee if not already fleeing
			{
				owner->GetMind()->SwitchState(STATE_FLEE);
			}
			return;
		}

		// End this state
		owner->GetMind()->EndState();
	}
}

// Save/Restore methods
void PainState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);

	savefile->WriteInt(_stateEndTime);
}

void PainState::Restore(idRestoreGame* savefile) 
{
	State::Restore(savefile);
	savefile->ReadInt(_stateEndTime);
}

StatePtr PainState::CreateInstance()
{
	return StatePtr(new PainState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar painStateRegistrar(
	STATE_PAIN, // Task Name
	StateLibrary::CreateInstanceFunc(&PainState::CreateInstance) // Instance creation callback
);

} // namespace ai
