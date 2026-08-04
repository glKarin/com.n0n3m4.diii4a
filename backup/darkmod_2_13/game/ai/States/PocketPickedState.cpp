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



#include "../Tasks/SingleBarkTask.h"
#include "../Tasks/IdleAnimationTask.h" // grayman #3857
#include "PocketPickedState.h"
#include "ConversationState.h" // grayman #3559

namespace ai
{

PocketPickedState::PocketPickedState()
{}

// Get the name of this state
const idStr& PocketPickedState::GetName() const
{
	static idStr _name(STATE_POCKET_PICKED);
	return _name;
}

void PocketPickedState::Cleanup(idAI* owner)
{
	owner->m_ReactingToPickedPocket = false;
	owner->GetMemory().stopReactingToPickedPocket = false;
	owner->GetMemory().latchPickedPocket = false;
	owner->GetMemory().insideAlertWindow = false;
}

// Wrap up and end state

void PocketPickedState::Wrapup(idAI* owner)
{
	Cleanup(owner);
	owner->GetMind()->EndState();
}

void PocketPickedState::Init(idAI* owner)
{
	State::Init(owner);

	assert(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("PocketPickedState::Init (%s)\r",owner->GetName());

	// stop if something more important has happened
	if (owner->GetMemory().stopReactingToPickedPocket)
	{
		Wrapup(owner);
		return;
	}

	_pocketPickedState = EStateReact;
}

// Gets called each time the mind is thinking
void PocketPickedState::Think(idAI* owner)
{
	owner->PerformVisualScan();	// Let the AI check its senses

	// Check conditions for continuing.
	
	moveType_t moveType = owner->GetMoveType();
	Memory& memory = owner->GetMemory();

	if ( owner->AI_DEAD || // stop reacting if dead
		 owner->AI_KNOCKEDOUT || // or unconscious
		 (owner->AI_AlertIndex >= ESearching) || // stop if alert level is too high
		 memory.stopReactingToPickedPocket || // check if something happened to abort the state
		 (moveType == MOVETYPE_SLEEP) || // or asleep
		 (moveType == MOVETYPE_SIT_DOWN) || // or in the act of sitting down
		 (moveType == MOVETYPE_FALL_ASLEEP) || // or in the act of lying down to sleep // grayman #3820 - was MOVETYPE_LAY_DOWN
		 (moveType == MOVETYPE_GET_UP) ||   // or getting up from sitting
		 (moveType == MOVETYPE_WAKE_UP) ) // or getting up from lying down // grayman #3820 - MOVETYPE_WAKE_UP was MOVETYPE_GET_UP_FROM_LYING
	{
		Wrapup(owner);
		return;
	}

	switch (_pocketPickedState)
	{
		case EStateReact:
			{
			owner->actionSubsystem->ClearTasks();
			owner->movementSubsystem->ClearTasks();
			owner->m_ReactingToPickedPocket = true;

			// Emit the picked pocket bark if alert level is low, and we're not in Alert Idle
				
			if (owner->AI_AlertIndex < EObservant)
			{
				if (!(owner->HasSeenEvidence() && ( owner->spawnArgs.GetBool("disable_alert_idle", "0") == false) ) )
				{
					CommMessagePtr message;
					owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask("snd_notice_pickpocket", message)));
				}
			}

			// If this occurred w/in the original alert window that surrounds the
			// pickpocket event, it's one more piece of evidence of something out of place.
			if (memory.insideAlertWindow)
			{
				memory.insideAlertWindow = false;
				memory.countEvidenceOfIntruders += EVIDENCE_COUNT_INCREASE_SUSPICIOUS;
				memory.posEvidenceIntruders = owner->GetPhysics()->GetOrigin();
				memory.timeEvidenceIntruders = gameLocal.time;
			}

			_pocketPickedState = EStateStopping;
			break;
			}
		case EStateStopping:
			// If AI is sitting, he won't stand or stop moving

			if ( owner->GetMoveType() == MOVETYPE_SIT )
			{
				_waitEndTime = gameLocal.time;
				_pocketPickedState = EStateTurnToward;
			}
			else
			{
				// Stop moving
				owner->StopMove(MOVE_STATUS_DONE);
				_pocketPickedState = EStateStartAnim;
			}
			break;
		case EStateStartAnim:
			// Don't play the animation if you're holding a torch or lantern.
			// If you're holding a weapon, you probably shouldn't be here, but
			// the weapon defs include replacement anims.
			if ( ( owner->GetTorch() == NULL ) && ( owner->GetLantern() == NULL ))
			{
				// Start the animation
				owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_PocketPicked", 4);
				owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_PocketPicked", 4);

				owner->SetWaitState("pocketPicked");
				owner->SetWaitState(ANIMCHANNEL_TORSO, "pocketPicked");
				owner->SetWaitState(ANIMCHANNEL_LEGS, "pocketPicked");
			}
			_pocketPickedState = EStatePlayAnim;
			break;
		case EStatePlayAnim:
			if (idStr(owner->WaitState()) != "pocketPicked")
			{
				// Turn to look behind you.
				owner->TurnToward(owner->GetCurrentYaw() + 180);
				_waitEndTime = gameLocal.time + 1000;
				_pocketPickedState = EStateTurnToward;
			}
			break;
		case EStateTurnToward:
			if (gameLocal.time >= _waitEndTime)
			{
				// Done turning.

				idVec3 vec;
				float duration = LOOK_TIME_MIN + gameLocal.random.RandomFloat()*(LOOK_TIME_MAX - LOOK_TIME_MIN);
				if ( owner->GetMoveType() == MOVETYPE_SIT )
				{
					// Stare at the ground to your right or left, as if looking for something

					idAngles angles = owner->viewAxis.ToAngles();
					float forward = angles.yaw;
					float lookAngle = forward + (gameLocal.random.RandomFloat() < 0.5 ? 70 : -70);
					idAngles lookAngles(0.0f, lookAngle, 0.0f);
					vec = lookAngles.ToForward()*100; // ToForward() returns unit vector
				}
				else
				{
					// Stare at the ground in front of you, as if looking for something

					vec = owner->viewAxis.ToAngles().ToForward()*100; // ToForward() returns unit vector
				}
				vec.z = 0;
				idVec3 lookAtMe = owner->GetEyePosition() + vec;
				lookAtMe.z = owner->GetPhysics()->GetOrigin().z;
				owner->Event_LookAtPosition(lookAtMe, MS2SEC(duration));
				_pocketPickedState = EStateLookAt;
				_waitEndTime = gameLocal.time + duration;
			}
			break;
		case EStateLookAt: // look for a while
			if (gameLocal.time >= _waitEndTime)
			{
				// Done looking, end this state

				// Increase the alert level?
				float alertInc = owner->spawnArgs.GetFloat("pickpocket_alert","0");
				if (alertInc > 0.0f)
				{
					// Set the new alert level, but cap it just under Combat.
					// If the new alert level pushes the AI up into
					// Searching or Agitated Searching, the Picked Pocket State will end.

					float newAlertLevel = owner->AI_AlertLevel + alertInc;
					if ( newAlertLevel >= owner->thresh_5 )
					{
						newAlertLevel = owner->thresh_5 - 0.1;
					}
					owner->SetAlertLevel(newAlertLevel);

					// If the alert level is now in Searching or Agitated Searching,
					// create a search area.

					if ( owner->AI_AlertIndex >= ESearching )
					{
						// grayman #3857 - move alert setup into one method
						SetUpSearchData(EAlertTypePickedPocket, owner->GetPhysics()->GetOrigin(), NULL, false, 0);
					}
				}
				Wrapup(owner);
				return;
			}
			break;
		default:
			break;
	}
}

void PocketPickedState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);

	savefile->WriteInt(_waitEndTime);
	savefile->WriteInt(static_cast<int>(_pocketPickedState));
}

void PocketPickedState::Restore(idRestoreGame* savefile)
{
	State::Restore(savefile);

	savefile->ReadInt(_waitEndTime);
	int temp;
	savefile->ReadInt(temp);
	_pocketPickedState = static_cast<EPocketPickedState>(temp);
}

StatePtr PocketPickedState::CreateInstance()
{
	return StatePtr(new PocketPickedState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar pocketPickedStateRegistrar(
	STATE_POCKET_PICKED, // Task Name
	StateLibrary::CreateInstanceFunc(&PocketPickedState::CreateInstance) // Instance creation callback
);

} // namespace ai
