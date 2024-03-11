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



#include "FleeDoneState.h"
#include "../Memory.h"
#include "../../AIComm_Message.h"
#include "../Library.h"
#include "../Tasks/WaitTask.h"
#include "../Tasks/FleeTask.h"
#include "../Tasks/RepeatedBarkTask.h"
#include "../Tasks/RandomHeadturnTask.h"
#include "../Tasks/SingleBarkTask.h"

#include "../Tasks/RandomTurningTask.h"
#include "IdleState.h"

namespace ai
{

// Get the name of this state
const idStr& FleeDoneState::GetName() const
{
	static idStr _name(STATE_FLEE_DONE);
	return _name;
}

void FleeDoneState::Init(idAI* owner)
{
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("FleeDoneState initialised.\r");
	assert(owner);

	// Shortcut reference
	//Memory& memory = owner->GetMemory();

	// greebo: At this point we should be at a presumably safe place, 
	// start looking for allies
	_searchForFriendDone = false;

	// alert level ramps down
	float alertTime = owner->atime_fleedone + owner->atime_fleedone_fuzzyness * (gameLocal.random.RandomFloat() - 0.5);
	_alertLevelDecreaseRate = (owner->thresh_5 - owner->thresh_3) / alertTime;

	owner->senseSubsystem->ClearTasks();
 	owner->GetSubsystem(SubsysCommunication)->ClearTasks();
	owner->actionSubsystem->ClearTasks();

	if ( owner->AI_DEAD || owner->AI_KNOCKEDOUT ) // grayman #3317 - only go to STATE_FLEE_DONE if not KO'ed or dead
	{
		owner->GetMind()->EndState();
		return;
	}
		
	// Slow turning for 5 seconds to look for friends
	owner->StopMove(MOVE_STATUS_DONE);
	_oldTurnRate = owner->GetTurnRate();
	owner->SetTurnRate(90);
	owner->movementSubsystem->ClearTasks();
	owner->movementSubsystem->PushTask(RandomTurningTask::CreateInstance());
	_turnEndTime = gameLocal.time + 5000;

	owner->senseSubsystem->PushTask(RandomHeadturnTask::CreateInstance());
}

void FleeDoneState::WrapUp(idAI* owner)
{
	owner->movementSubsystem->ClearTasks();
	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 4);
	owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", 4);
	owner->SetTurnRate(_oldTurnRate);
}

// Gets called each time the mind is thinking
void FleeDoneState::Think(idAI* owner)
{
	if ( owner->AI_DEAD || owner->AI_KNOCKEDOUT ) // grayman #3317 - quit if KO'ed or dead
	{
		WrapUp(owner);
		owner->GetMind()->EndState();
		return;
	}

	UpdateAlertLevel();
		
	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner)) 
	{
		// terminate FleeDoneState when time is over
		WrapUp(owner);
		owner->GetMind()->EndState();
		return;
	}

	// Let the AI check its senses
	owner->PerformVisualScan();
	if (owner->AI_ALERTED)
	{
		// terminate FleeDoneState when the AI is alerted
		WrapUp(owner);

		owner->GetMind()->EndState();
		return; // grayman #3474
	}

	if ( !owner->emitFleeBarks ) // grayman #3474
	{
		// not crying for help, time to end this state
		WrapUp(owner);
		owner->GetMind()->EndState();
		return;
	}

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	if (!_searchForFriendDone)
	{
		idActor* friendlyAI = owner->FindFriendlyAI(-1);
		if ( friendlyAI != NULL)
		{
			// We found a friend, cry for help to him
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("%s found friendly AI %s, crying for help\r", owner->name.c_str(),friendlyAI->name.c_str());

			_searchForFriendDone = true;
			owner->movementSubsystem->ClearTasks();
			owner->SetTurnRate(_oldTurnRate);

			owner->TurnToward(friendlyAI->GetPhysics()->GetOrigin());
			//float distanceToFriend = (friendlyAI->GetPhysics()->GetOrigin() - owner->GetPhysics()->GetOrigin()).LengthFast();

			// Cry for help
			// Create a new help message
			CommMessagePtr message(new CommMessage(
				CommMessage::RequestForHelp_CommType, 
				owner, friendlyAI,
				NULL,
				memory.alertPos,
				memory.currentSearchEventID // grayman #3857 (was '0')
			)); 

			CommunicationTaskPtr barkTask(new SingleBarkTask("snd_flee", message));

			if (cv_ai_debug_transition_barks.GetBool())
			{
				gameLocal.Printf("%d: %s flees, barks 'snd_flee'\n",gameLocal.time,owner->GetName());
			}

			owner->commSubsystem->AddCommTask(barkTask);
			memory.lastTimeVisualStimBark = gameLocal.time;
		}
		else if (gameLocal.time >= _turnEndTime)
		{
			// We didn't find a friend, stop looking for them after some time
			_searchForFriendDone = true;
			owner->movementSubsystem->ClearTasks();
			owner->SetTurnRate(_oldTurnRate);

			// Play the cowering animation
			owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Cower", 4);
			owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Cower", 4);
		}
	}
}


void FleeDoneState::OnActorEncounter(idEntity* stimSource, idAI* owner)
{
	assert(stimSource != NULL && owner != NULL); // must be fulfilled

	Memory& memory = owner->GetMemory();
	
	if (!stimSource->IsType(idActor::Type)) return; // No Actor, quit

	// Hard-cast the stimsource onto an actor 
	idActor* other = static_cast<idActor*>(stimSource);	
	if (owner->IsFriend(other))
	{
		if (gameLocal.time - memory.lastTimeVisualStimBark >= MINIMUM_SECONDS_BETWEEN_STIMULUS_BARKS)
		{

			memory.lastTimeVisualStimBark = gameLocal.time;
			CommMessagePtr message(new CommMessage(
				CommMessage::RequestForHelp_CommType, 
				owner, other,
				NULL,
				memory.alertPos,
				memory.currentSearchEventID // grayman #3857 (was '0')
			)); 

			CommunicationTaskPtr barkTask(new SingleBarkTask("snd_flee", message));

			if (cv_ai_debug_transition_barks.GetBool())
			{
				gameLocal.Printf("%d: %s flees, barks 'snd_flee'\n",gameLocal.time,owner->GetName());
			}

			owner->commSubsystem->AddCommTask(barkTask);
			memory.lastTimeVisualStimBark = gameLocal.time;
		}
	}

	State::OnActorEncounter(stimSource, owner);
}


bool FleeDoneState::CheckAlertLevel(idAI* owner)
{
	// FleeDoneState terminates itself when the AI reaches Suspicious (aka Investigating)
	if (owner->AI_AlertIndex < ESearching)
	{
		// Alert index is too low for this state, fall back
		WrapUp(owner);
		return false;
	}
	
	// Alert Index is matching, return OK
	return true;
}

// grayman #3848 - set of ignored events
void FleeDoneState::OnVisualStimWeapon(idEntity* stimSource, idAI* owner) {}
void FleeDoneState::OnVisualStimSuspicious(idEntity* stimSource, idAI* owner) {}
void FleeDoneState::OnVisualStimRope( idEntity* stimSource, idAI* owner, idVec3 ropeStimSource ) {}
void FleeDoneState::OnVisualStimBlood(idEntity* stimSource, idAI* owner) {}
void FleeDoneState::OnVisualStimLightSource(idEntity* stimSource, idAI* owner) {}
void FleeDoneState::OnVisualStimMissingItem(idEntity* stimSource, idAI* owner) {}
void FleeDoneState::OnVisualStimBrokenItem(idEntity* stimSource, idAI* owner) {}
void FleeDoneState::OnVisualStimDoor(idEntity* stimSource, idAI* owner) {}
void FleeDoneState::OnHitByMoveable(idAI* owner, idEntity* tactEnt) {}

// Save/Restore methods
void FleeDoneState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);

	savefile->WriteFloat(_oldTurnRate);
	savefile->WriteInt(_turnEndTime);
	savefile->WriteBool(_searchForFriendDone);
} 

void FleeDoneState::Restore(idRestoreGame* savefile)
{
	State::Restore(savefile);

	savefile->ReadFloat(_oldTurnRate);
	savefile->ReadInt(_turnEndTime);
	savefile->ReadBool(_searchForFriendDone);
} 

StatePtr FleeDoneState::CreateInstance()
{
	return StatePtr(new FleeDoneState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar fleeDoneStateRegistrar(
	STATE_FLEE_DONE, // Task Name
	StateLibrary::CreateInstanceFunc(&FleeDoneState::CreateInstance) // Instance creation callback
);

} // namespace ai
