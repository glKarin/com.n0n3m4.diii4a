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



#include "BlindedState.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Tasks/IdleAnimationTask.h" // grayman #3857
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& BlindedState::GetName() const
{
	static idStr _name(STATE_BLINDED);
	return _name;
}


void BlindedState::Init(idAI* owner)
{
	// grayman #4270 - if the AI is sitting, get up first

	if ( owner->GetMoveType() == MOVETYPE_SIT )
	{
		owner->GetUp();
	}

	Memory& memory = owner->GetMemory();
	memory.currentSearchEventID = owner->LogSuspiciousEvent(E_EventTypeMisc, memory.alertPos, NULL, true); // grayman #3857

	CommMessagePtr message(new CommMessage(
		CommMessage::RequestForHelp_CommType,
		owner, NULL, // from this AI to anyone 
		NULL,
		memory.alertPos,
		memory.currentSearchEventID // grayman #3857 (was '0')
	));

	owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask("snd_blinded", message)));

	if ( cv_ai_debug_transition_barks.GetBool() )
	{
		gameLocal.Printf("%d: %s is blinded, barks 'snd_blinded'\n", gameLocal.time, owner->GetName());
	}

	_oldVisAcuity = owner->GetBaseAcuity("vis"); // grayman #3552
	owner->SetAcuity("vis", 0);

	_oldAudAcuity = owner->GetBaseAcuity("aud"); // grayman #3552
	owner->SetAcuity("aud", _oldAudAcuity*0.25f); // Smoke #2829

	_initialized = false; // grayman #4270
}

// Gets called each time the mind is thinking
void BlindedState::Think(idAI* owner)
{
	// grayman #4270 - Delay initialization if getting up from sitting.
	if ( (owner->GetMoveType() == MOVETYPE_SIT) || (owner->GetMoveType() == MOVETYPE_GET_UP) )
	{
		return;
	}

	if (!_initialized)
	{
		// Init base class first
		State::Init(owner);

		owner->movementSubsystem->ClearTasks();
		owner->senseSubsystem->ClearTasks();
		owner->actionSubsystem->ClearTasks();
		owner->searchSubsystem->ClearTasks(); // grayman #3857

		owner->StopMove(MOVE_STATUS_DONE);

		owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Blinded", 4);
		owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Blinded", 4);

		owner->SetWaitState("blinded"); // grayman #3431
		owner->SetWaitState(ANIMCHANNEL_TORSO, "blinded");
		owner->SetWaitState(ANIMCHANNEL_LEGS, "blinded");

		Memory& memory = owner->GetMemory();
		memory.StopReacting(); // grayman #3559

		float duration = SEC2MS(owner->spawnArgs.GetFloat("blind_time", "8")) +
			(gameLocal.random.RandomFloat() - 0.5f) * 2 * SEC2MS(owner->spawnArgs.GetFloat("blind_time_fuzziness", "4"));

		_endTime = gameLocal.time + static_cast<int>(duration);

		_staring = false; // grayman #3431 (set to true when you stare at the ground)
		_initialized = true; // grayman #4270
	}

	if (gameLocal.time >= _endTime)
	{
		owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 4);
		owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", 4);

		owner->SetWaitState("");
		owner->SetWaitState(ANIMCHANNEL_TORSO, "");
		owner->SetWaitState(ANIMCHANNEL_LEGS, "");

		owner->SetAcuity("vis", _oldVisAcuity);
		owner->SetAcuity("aud", _oldAudAcuity); // Smoke #2829

		// Set up a search if you have no enemy. If you have an enemy,
		// the queued Lost Track of Enemy state will set up a search
		// of its own.

		if ( !owner->GetEnemy() )
		{
			// grayman #3857 - move alert setup into one method
			SetUpSearchData(EAlertTypeBlinded, owner->GetMemory().alertPos, NULL, false, 0); // grayman #3857
		}

		owner->GetMind()->EndState();
	}
	else if ( !_staring && ( idStr(owner->WaitState()) != "blinded" ) ) // grayman #3431
	{
		int duration = _endTime - gameLocal.time;
		if ( duration > 0 )
		{
			// Stare at the ground in front of you while waiting to get your sight back

			idVec3 vec = owner->viewAxis.ToAngles().ToForward();
			vec.z = 0;
			vec.NormalizeFast();
			idVec3 lookAtMe = owner->GetPhysics()->GetOrigin() + 48*vec;
			owner->Event_LookAtPosition(lookAtMe, MS2SEC(duration));
			_staring = true;
		}
	}
}

void BlindedState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);

	savefile->WriteInt(_endTime);
	savefile->WriteFloat(_oldVisAcuity);
	savefile->WriteFloat(_oldAudAcuity); // Smoke #2829
	savefile->WriteBool(_staring); // grayman #3431
	savefile->WriteBool(_initialized); // grayman #4270
}

void BlindedState::Restore(idRestoreGame* savefile)
{
	State::Restore(savefile);

	savefile->ReadInt(_endTime);
	savefile->ReadFloat(_oldVisAcuity);
	savefile->ReadFloat(_oldAudAcuity); // Smoke #2829
	savefile->ReadBool(_staring); // grayman #3431
	savefile->ReadBool(_initialized); // grayman #4270
}

StatePtr BlindedState::CreateInstance()
{
	return StatePtr(new BlindedState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar blindedStateRegistrar(
	STATE_BLINDED, // Task Name
	StateLibrary::CreateInstanceFunc(&BlindedState::CreateInstance) // Instance creation callback
);

} // namespace ai
