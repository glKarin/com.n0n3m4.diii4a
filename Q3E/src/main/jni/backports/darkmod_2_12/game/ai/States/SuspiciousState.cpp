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



#include "SuspiciousState.h"
#include "../Memory.h"
#include "../../AIComm_Message.h"
#include "../Tasks/RandomHeadturnTask.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Tasks/IdleAnimationTask.h" // grayman #3857
#include "SearchingState.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& SuspiciousState::GetName() const
{
	static idStr _name(STATE_SUSPICIOUS);
	return _name;
}

bool SuspiciousState::CheckAlertLevel(idAI* owner)
{
	if (owner->AI_AlertIndex < ESuspicious)
	{
		// Alert index is too low for this state, fall back
		owner->GetMind()->EndState();

		// grayman #2866 - If there's a nearby door that was suspicious, close it now

		// If returning to what we were doing before takes us through
		// the door, don't close it here. It will get closed when we're on the other
		// side and close it normally.

		// If it doesn't take us through the door, then close it here.

		// To find out if our path takes us through this door,
		// compare which side of the door we were on when we first saw
		// the door with which side we're on now.

		// If, before we put ourselves on the door queue to handle it, we find that
		// others are on the queue, then we don't need to close the door.

		Memory& memory = owner->GetMemory();
		CFrobDoor* door = memory.closeMe.GetEntity();
		if ( ( door != NULL ) && ( door->GetUserManager().GetNumUsers() == 0 ) )
		{
			// grayman #3104 - if I'm handling a door now, I won't be able to initiate
			// handling of the suspicious door in order to close it. So I'll forget about
			// doing that, and let the suspicious door stim me again. Perhaps I'll see it
			// later.

			if ( owner->m_HandlingDoor )
			{
				CFrobDoor* currentDoor = owner->GetMemory().doorRelated.currentDoor.GetEntity();
				if ( currentDoor && ( currentDoor != door ) )
				{
					memory.closeMe = NULL;
					memory.closeSuspiciousDoor = false;
					door->SetSearching(NULL);
					door->AllowResponse(ST_VISUAL,owner); // respond to the next stim
					return false;
				}
			}

			memory.closeSuspiciousDoor = true; // grayman #2866
			OnFrobDoorEncounter(door); // set up the door handling task, so you can close the door
		}

		return false;
	}

	if (owner->AI_AlertIndex > ESuspicious)
	{
		// grayman #3069 - some AI don't search, so don't allow
		// them to rise into higher alert indices

		if ( owner->m_canSearch )
		{
			// Alert index is too high, switch to the higher State
			owner->GetMind()->PushState(owner->backboneStates[ESearching]);
			return false;
		}

		owner->SetAlertLevel(owner->thresh_3 - 0.1); // set alert level to just under ESearching
	}

	// Alert Index is matching, return OK
	return true;
}

void SuspiciousState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("SuspiciousState initialised.\r");
	assert(owner);

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner))
	{
		return;
	}

	float alertTime = owner->atime2 + owner->atime2_fuzzyness * (gameLocal.random.RandomFloat() - 0.5);
	_alertLevelDecreaseRate = (owner->thresh_3 - owner->thresh_2) / alertTime;

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	owner->senseSubsystem->ClearTasks();

	// grayman #3857 - if rising into Suspicious, switch idle animations
	// do this for both rising and falling
//	if ( owner->AlertIndexIncreased() )
//	{
		// grayman #3857 - allow "idle search/suspicious animations"
		owner->actionSubsystem->ClearTasks();
		owner->actionSubsystem->PushTask(IdleAnimationTask::CreateInstance());
//	}
	
	// grayman #3438 - kill the repeated bark task
	owner->commSubsystem->ClearTasks();

	if (owner->GetMoveType() == MOVETYPE_SLEEP)
	{
		owner->GetUp();
	}
	
	owner->movementSubsystem->ClearTasks();
	owner->StopMove(MOVE_STATUS_DONE);
	memory.StopReacting(); // grayman #3559

	// do various things if alert level is ascending

	if (owner->AlertIndexIncreased())
	{
		if ( !memory.alertedDueToCommunication ) // grayman #2920
		{
			if ( memory.alertClass != EAlertVisual_4) // grayman #3498
			{
				// grayman #3496 - Enough time passed since last alert bark?
				// grayman #3857 - Enough time passed since last visual stim bark?
				if ( ( gameLocal.time >= memory.lastTimeAlertBark + MIN_TIME_BETWEEN_ALERT_BARKS ) &&
					 ( gameLocal.time >= memory.lastTimeVisualStimBark + MIN_TIME_BETWEEN_ALERT_BARKS ) )
				{
					// barking
					idStr bark;

					if ((memory.alertClass == EAlertVisual_1) ||
						(memory.alertClass == EAlertVisual_2) )
						//(memory.alertClass == EAlertVisual_3)) // grayman #2603, #3424, grayman #3472 - no longer needed
					{
						bark = "snd_alert1s";
					}
					else if (memory.alertClass == EAlertAudio)
					{
						bark = "snd_alert1h";
					}
					else
					{
						bark = "snd_alert1";
					}
					owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask(bark)));

					memory.lastTimeAlertBark = gameLocal.time; // grayman #3496

					if (cv_ai_debug_transition_barks.GetBool())
					{
						gameLocal.Printf("%d: %s rises to Suspicious state, barks '%s'\n",gameLocal.time,owner->GetName(),bark.c_str());
					}
				}
				else
				{
					if (cv_ai_debug_transition_barks.GetBool())
					{
						gameLocal.Printf("%d: %s rises to Suspicious state, can't bark 'snd_alert1{s/h}' yet\n",gameLocal.time,owner->GetName());
					}
				}
			}
		}
		else
		{
			memory.alertedDueToCommunication = false; // reset
		}
	}
	else // grayman #3857 - descending
	{
		owner->searchSubsystem->ClearTasks();
		memory.currentSearchEventID = -1;
		owner->lastSearchedSpot = idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY); // grayman #4220
	}

	// Let the AI update their weapons (make them nonsolid)
	owner->UpdateAttachmentContents(false);

	// grayman #3559 - If we're to look at an
	// alert spot because there was an alert, do that.
	// Otherwise, do random head turning unless you're
	// in a conversation.

	if (!owner->m_lookAtAlertSpot && !owner->m_InConversation)
	{
		// grayman #3559 - If we're staring at a wall, turn around.

		// Trace forward to see if you're close to a wall.

		idVec3 p1 = owner->GetEyePosition();
		idVec3 forward = owner->viewAxis.ToAngles().ToForward();
		forward.z = 0; // look horizontally
		forward.NormalizeFast();
		idVec3 p2 = p1 + 64*forward;

		trace_t result;
		if ( gameLocal.clip.TracePoint(result, p1, p2, MASK_OPAQUE, owner) )
		{
			// grayman #3857 - TODO: test to make sure we hit the world, and not just another AI?
			// Hit something, so turn around.
			owner->TurnToward(owner->GetCurrentYaw() + 180);
		}

		// The sensory system does random head turning
		owner->senseSubsystem->ClearTasks();
		owner->senseSubsystem->PushTask(RandomHeadturnTask::CreateInstance());
	}
}

// Gets called each time the mind is thinking
void SuspiciousState::Think(idAI* owner)
{
	UpdateAlertLevel();
	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner))
	{
		return;
	}

	// grayman #3520 - look at an alert spot
	// grayman #3559 - unless you're in a conversation
	if (owner->m_lookAtAlertSpot && !owner->m_InConversation)
	{
		owner->m_lookAtAlertSpot = false;
		idVec3 alertSpot = owner->m_lookAtPos;
		if ( alertSpot.x != idMath::INFINITY ) // grayman #3438
		{
			bool inFOV = owner->CheckFOV(alertSpot);
			if ( !inFOV && ( owner->GetMoveType() == MOVETYPE_ANIM ) )
			{
				// Search spot is not within FOV, turn towards the position
				// don't turn while sitting
				owner->TurnToward(alertSpot);
				owner->Event_LookAtPosition(alertSpot, 2.0f);
			}
			else if (inFOV)
			{
				owner->Event_LookAtPosition(alertSpot, 2.0f);
			}
		}
		owner->m_lookAtPos = idVec3(idMath::INFINITY,idMath::INFINITY,idMath::INFINITY);
	}

	// Let the AI check its senses
	owner->PerformVisualScan();
}


StatePtr SuspiciousState::CreateInstance()
{
	return StatePtr(new SuspiciousState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar suspiciousStateRegistrar(
	STATE_SUSPICIOUS, // Task Name
	StateLibrary::CreateInstanceFunc(&SuspiciousState::CreateInstance) // Instance creation callback
);

} // namespace ai
