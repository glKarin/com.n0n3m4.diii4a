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



#include "FailedKnockoutState.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

FailedKnockoutState::FailedKnockoutState() :
	_attacker(NULL),
	_hitHead(false)
{}

FailedKnockoutState::FailedKnockoutState(idEntity* attacker, const idVec3& attackDirection, bool hitHead) :
	_attacker(attacker),
	_attackDirection(attackDirection),
	_hitHead(hitHead)
{}

// Get the name of this state
const idStr& FailedKnockoutState::GetName() const
{
	static idStr _name(STATE_FAILED_KNOCKED_OUT);
	return _name;
}

void FailedKnockoutState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("FailedKnockoutState initialised.\r");
	assert(owner);

	Memory& memory = owner->GetMemory();

	// Failed KO counts as attack
	memory.hasBeenAttackedByEnemy = true;

	if (cv_ai_debug_transition_barks.GetBool())
	{
		gameLocal.Printf("%d: %s is attacked by an enemy (failed KO), will use Alert Idle\n",gameLocal.time,owner->GetName());
	}

	// grayman #4423 - don't play the failed KO animation if the AI
	// is in the process of sitting or sleeping

	moveType_t moveType = owner->GetMoveType();
	if ( moveType == MOVETYPE_SIT ||
		moveType == MOVETYPE_SLEEP ||
		moveType == MOVETYPE_SIT_DOWN ||
		moveType == MOVETYPE_FALL_ASLEEP || // grayman #3820 - was MOVETYPE_LAY_DOWN
		moveType == MOVETYPE_GET_UP ||
		moveType == MOVETYPE_WAKE_UP ) // grayman #3820 - was MOVETYPE_GET_UP_FROM_LYING
	{
		// do nothing
	}
	else
	{	// Play the animation
		owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_FailedKO", 4);
		owner->PostEventMS(&AI_OverrideAnim, 0, ANIMCHANNEL_LEGS); // SteveL #3964
		owner->SetWaitState("failed_ko"); // #3964: Set overall waitstate not just torso wait state else IdleAnimationTask::OnFinish will cancel it.
	}

	// 800 msec stun time
	_allowEndTime = gameLocal.time + 800;

	// Set end time
	_stateEndTime = gameLocal.time + 1000; // grayman #3857 - was 3000, but the failed_ko anim
											// is being aborted. If it runs, it only takes a second
	
	// Set the alert position 50 units in the attacking direction
	//memory.alertPos = owner->GetPhysics()->GetOrigin() - _attackDirection * 50;

	// grayman #3857 - moved into Think(). If we do this here, the pain
	// bark gets aborted by this bark.
/*	// Do a single bark and assemble an AI message
	CommMessagePtr message = CommMessagePtr(new CommMessage(
		CommMessage::DetectedEnemy_CommType, 
		owner, NULL, // from this AI to anyone
		_attacker,
		memory.alertPos,
		-1
	));

	owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask("snd_failed_knockout", message)));

	if (cv_ai_debug_transition_barks.GetBool())
	{
		gameLocal.Printf("%d: %s is attacked by an enemy (failed KO), barks 'snd_failed_knockout'\n",gameLocal.time,owner->GetName());
	}
	*/
}

// Gets called each time the mind is thinking
void FailedKnockoutState::Think(idAI* owner)
{
	if (gameLocal.time < _allowEndTime)
	{
		return; // wait...
	}

	if ( ( gameLocal.time >= _stateEndTime ) || 
		 ( idStr(owner->WaitState(ANIMCHANNEL_TORSO)) != "failed_ko" ) )
	{
		Memory& memory = owner->GetMemory();

		memory.countEvidenceOfIntruders += EVIDENCE_COUNT_INCREASE_FAILED_KO;
		memory.posEvidenceIntruders = owner->GetPhysics()->GetOrigin(); // grayman #2903
		memory.timeEvidenceIntruders = gameLocal.time; // grayman #2903
		memory.StopReacting(); // grayman #3559

		// grayman #3857 - alert and bark moved down from Init()

		// Set the alert position 50 units in the attacking direction
		_attackDirection.z = 0; // grayman #3857
		_attackDirection.NormalizeFast(); // grayman #3857
		memory.alertPos = owner->GetPhysics()->GetOrigin() - _attackDirection * 50;

		// grayman #3857 - move alert setup into one method
		SetUpSearchData(EAlertTypeFailedKO, memory.alertPos, NULL, false, 0);

		// Do a single bark and assemble an AI message
		CommMessagePtr message = CommMessagePtr(new CommMessage(
			CommMessage::DetectedEnemy_CommType, 
			owner, NULL, // from this AI to anyone
			_attacker,
			memory.alertPos,
			memory.currentSearchEventID
		));

		owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask("snd_failed_knockout", message)));

		if (cv_ai_debug_transition_barks.GetBool())
		{
			gameLocal.Printf("%d: %s is attacked by an enemy (failed KO), barks 'snd_failed_knockout'\n",gameLocal.time,owner->GetName());
		}
	
		// End this state
		owner->GetMind()->EndState();
	}
}

// Save/Restore methods
void FailedKnockoutState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);

	savefile->WriteInt(_stateEndTime);
	savefile->WriteInt(_allowEndTime);
	savefile->WriteObject(_attacker);
	savefile->WriteVec3(_attackDirection);
}

void FailedKnockoutState::Restore(idRestoreGame* savefile) 
{
	State::Restore(savefile);
	savefile->ReadInt(_stateEndTime);
	savefile->ReadInt(_allowEndTime);
	savefile->ReadObject(reinterpret_cast<idClass*&>(_attacker));
	savefile->ReadVec3(_attackDirection);
}

StatePtr FailedKnockoutState::CreateInstance()
{
	return StatePtr(new FailedKnockoutState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar failedKnockoutStateRegistrar(
	STATE_FAILED_KNOCKED_OUT, // Task Name
	StateLibrary::CreateInstanceFunc(&FailedKnockoutState::CreateInstance) // Instance creation callback
);

} // namespace ai
