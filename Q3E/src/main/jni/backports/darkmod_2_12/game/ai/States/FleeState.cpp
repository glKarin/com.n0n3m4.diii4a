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



#include "FleeState.h"
#include "../Memory.h"
#include "../../AIComm_Message.h"
#include "../Library.h"
#include "../Tasks/WaitTask.h"
#include "../Tasks/FleeTask.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Tasks/RepeatedBarkTask.h"
#include "FleeDoneState.h"

namespace ai
{

// Get the name of this state
const idStr& FleeState::GetName() const
{
	static idStr _name(STATE_FLEE);
	return _name;
}

void FleeState::Init(idAI* owner)
{
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("%s - FleeState initialised.\r",owner->name.c_str());
	assert(owner);

	// Shortcut reference
	Memory& memory = owner->GetMemory();
	memory.fleeing = true;

	// Fill the subsystems with their tasks

	// The movement subsystem should wait half a second before starting to run
	owner->StopMove(MOVE_STATUS_DONE);
	memory.StopReacting(); // grayman #3559

	// grayman #3857- If participating in a search, leave the search
	if (owner->m_searchID > 0)
	{
		gameLocal.m_searchManager->LeaveSearch(owner->m_searchID, owner);
	}

	if (owner->fleeingFromPerson.GetEntity()) // grayman #3847
	{
		owner->TurnToward(owner->fleeingFromPerson.GetEntity()->GetPhysics()->GetOrigin());
	}

	//if (owner->GetEnemy())
	//{
	//	owner->FaceEnemy();
	//}

	owner->movementSubsystem->ClearTasks();
	owner->movementSubsystem->PushTask(TaskPtr(new WaitTask(1000)));
	owner->movementSubsystem->QueueTask(FleeTask::CreateInstance());

	if ( owner->emitFleeBarks ) // grayman #3474
	{
		// The communication system cries for help
		owner->StopSound(SND_CHANNEL_VOICE, false);
		owner->GetSubsystem(SubsysCommunication)->ClearTasks();
	/*	owner->GetSubsystem(SubsysCommunication)->PushTask(TaskPtr(new WaitTask(200)));*/// TODO_AI
	
		// Setup the message to be delivered each time
		CommMessagePtr message(new CommMessage(
			CommMessage::RequestForHelp_CommType, 
			owner, NULL, // from this AI to anyone 
			NULL,
			memory.alertPos,
			memory.currentSearchEventID // grayman #3857 (was '0')
		));

		// grayman #3317 - Use a different initial flee bark
		// depending on whether the AI is fleeing an enemy, or fleeing
		// a witnessed murder or KO.

		idStr singleBark = "snd_to_flee";
		if ( owner->fleeingEvent )
		{
			singleBark = "snd_to_flee_event";
		}

		// grayman #3140 - if hit by an arrow, issue a different bark
		if ( memory.causeOfPain == EPC_Projectile )
		{
			singleBark = "snd_taking_fire";
			memory.causeOfPain = EPC_None;
		}

		owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask(singleBark,message)));

		if (cv_ai_debug_transition_barks.GetBool())
		{
			gameLocal.Printf("%d: %s flees, barks '%s'\n",gameLocal.time,owner->GetName(),singleBark.c_str());
		}

		owner->commSubsystem->AddSilence(3000 + gameLocal.random.RandomInt(1500)); // grayman #3424;

		CommunicationTaskPtr barkTask(new RepeatedBarkTask("snd_flee", 4000,8000, message));
		owner->commSubsystem->AddCommTask(barkTask);
	}

	// The sensory system 
	owner->senseSubsystem->ClearTasks();

	// No action
	owner->actionSubsystem->ClearTasks();

	owner->searchSubsystem->ClearTasks(); // grayman #3857

	// Play the surprised animation
	// grayman #2416 - don't play if sitting or sleeping
	moveType_t moveType = owner->GetMoveType();

	if ( ( moveType == MOVETYPE_ANIM  ) ||
		 ( moveType == MOVETYPE_SLIDE ) ||
		 ( moveType == MOVETYPE_FLY   ) )
	{
		owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Surprise", 5);
		owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Surprise", 5);
//		owner->SetWaitState("surprise"); // grayman #2416 - no one's watching or resetting this
	}
}

// Gets called each time the mind is thinking
void FleeState::Think(idAI* owner)
{
	// Shortcut reference
	Memory& memory = owner->GetMemory();

	if (!memory.fleeing)
	{
		owner->ClearEnemy();
		owner->fleeingEvent = false; // grayman #3317

		if ( !owner->AI_DEAD && !owner->AI_KNOCKEDOUT ) // grayman #3317 - only go to STATE_FLEE_DONE if not KO'ed or dead
		{
			owner->GetMind()->SwitchState(STATE_FLEE_DONE);
		}
	}
	else // grayman #3548
	{
		// Let the AI check its senses
		owner->PerformVisualScan();
	}
}

// grayman #3848 - set of ignored events
void FleeState::OnFailedKnockoutBlow(idEntity* attacker, const idVec3& direction, bool hitHead) {}
void FleeState::OnVisualStimWeapon(idEntity* stimSource, idAI* owner) {}
void FleeState::OnVisualStimSuspicious(idEntity* stimSource, idAI* owner) {}
void FleeState::OnVisualStimRope( idEntity* stimSource, idAI* owner, idVec3 ropeStimSource ) {}
void FleeState::OnVisualStimBlood(idEntity* stimSource, idAI* owner) {}
void FleeState::OnVisualStimLightSource(idEntity* stimSource, idAI* owner) {}
void FleeState::OnVisualStimMissingItem(idEntity* stimSource, idAI* owner) {}
void FleeState::OnVisualStimBrokenItem(idEntity* stimSource, idAI* owner) {}
void FleeState::OnVisualStimDoor(idEntity* stimSource, idAI* owner) {}
void FleeState::OnHitByMoveable(idAI* owner, idEntity* tactEnt) {}

StatePtr FleeState::CreateInstance()
{
	return StatePtr(new FleeState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar fleeStateRegistrar(
	STATE_FLEE, // Task Name
	StateLibrary::CreateInstanceFunc(&FleeState::CreateInstance) // Instance creation callback
);

} // namespace ai
