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
#include "IdleSleepState.h"
#include "../Memory.h"
#include "../Tasks/RandomHeadturnTask.h"
#include "../Tasks/AnimalPatrolTask.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Tasks/RepeatedBarkTask.h"
#include "../Tasks/MoveToPositionTask.h"
#include "../Tasks/IdleAnimationTask.h"
#include "../Tasks/WaitTask.h"
#include "ObservantState.h"
#include "../Library.h"

#define		WARNING_DELAY 10000 // grayman #5164 - ms delay between "unreachable" WARNINGs

namespace ai
{

// Get the name of this state
const idStr& IdleState::GetName() const
{
	static idStr _name(STATE_IDLE);
	return _name;
}

bool IdleState::CheckAlertLevel(idAI* owner)
{
	if (owner->AI_AlertIndex > ERelaxed)
	{
		// Alert index is too high, switch to the higher State
		owner->GetMind()->PushState(owner->backboneStates[EObservant]);
		return false;
	}

	// Alert Index is matching, return OK
	return true;
}

void IdleState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("IdleState initialised.\r");
	assert(owner);

	_startSleeping = owner->spawnArgs.GetBool("sleeping", "0");
	_startSitting = owner->spawnArgs.GetBool("sitting", "0");

	// Memory shortcut
	Memory& memory = owner->GetMemory();

	owner->searchSubsystem->ClearTasks(); // grayman #3857
	memory.currentSearchEventID = -1; // grayman #3857

	owner->alertQueue.Clear(); // grayman #4002

	if (owner->HasSeenEvidence() && ( owner->spawnArgs.GetBool("disable_alert_idle", "0") == false) )
	{
		owner->GetMind()->SwitchState(STATE_ALERT_IDLE);
		return;
	}

	_alertLevelDecreaseRate = 0.01f;

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner))
	{
		return;
	}

	// Initialise the animation state
	if (_startSitting && memory.idlePosition == idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY))
	{
		owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 0);
		owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", 0);
		owner->SetAnimState(ANIMCHANNEL_HEAD, "Head_Idle", 0);

		owner->SitDown();
	}
	else if (owner->GetMoveType() == MOVETYPE_SIT)
	{
		owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle_Sit", 0);
		owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Idle_Sit", 0);
		owner->SetAnimState(ANIMCHANNEL_HEAD, "Head_Idle", 0);
	}
	else if (owner->GetMoveType() == MOVETYPE_SLEEP)
	{
		owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle_Sleep", 0);
		owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Idle_Sleep", 0);
		owner->SetAnimState(ANIMCHANNEL_HEAD, "Head_Idle_Sleep", 0);
	}
	else
	{
		owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 0);
		owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", 0);
		owner->SetAnimState(ANIMCHANNEL_HEAD, "Head_Idle", 0);
	}

	owner->actionSubsystem->ClearTasks();
	owner->senseSubsystem->ClearTasks();

	// angua: drunken AIs won't do any head turning and idle anims for now
	if (owner->spawnArgs.GetBool("drunk", "0") == false)
	{
		// The action subsystem plays the idle anims (scratching, yawning...)
		owner->actionSubsystem->PushTask(IdleAnimationTask::CreateInstance());

		// The sensory system does its Idle tasks
		owner->senseSubsystem->PushTask(RandomHeadturnTask::CreateInstance());
	}

	InitialiseMovement(owner);

	if (_startSitting)
	{
		if (owner->spawnArgs.FindKey("sit_down_angle") != NULL)
		{
			owner->AI_SIT_DOWN_ANGLE = owner->spawnArgs.GetFloat("sit_down_angle", "0");
		}
		else
		{
			owner->AI_SIT_DOWN_ANGLE = memory.idleYaw;
		}

		owner->AI_SIT_DOWN_ANGLE = idMath::AngleNormalize180(owner->AI_SIT_DOWN_ANGLE);

		owner->AI_SIT_UP_ANGLE = memory.idleYaw;
	}	

	InitialiseCommunication(owner);
	memory.alertClass = EAlertNone;
	memory.alertType = EAlertTypeNone;
	memory.alertPos = idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY); // grayman #3413

	memory.agitatedSearched = false; // grayman #3496
		
	memory.mightHaveSeenPlayer = false; // grayman #3515

	owner->m_recentHighestAlertLevel = 0; // grayman #3472

	int idleBarkIntervalMin = SEC2MS(owner->spawnArgs.GetInt("idle_bark_interval_min", "20"));
	int idleBarkIntervalMax = SEC2MS(owner->spawnArgs.GetInt("idle_bark_interval_max", "60"));
	// Push the regular patrol barking to the list too
	if (owner->spawnArgs.GetBool("drunk", "0"))
	{
		owner->commSubsystem->AddCommTask(
			CommunicationTaskPtr(new RepeatedBarkTask("snd_drunk", idleBarkIntervalMin, idleBarkIntervalMax))
		);
	}
	else
	{
		owner->commSubsystem->AddCommTask(
			CommunicationTaskPtr(new RepeatedBarkTask("snd_relaxed", idleBarkIntervalMin, idleBarkIntervalMax))
		);
	}

	// grayman #3505 - can't sheathe weapon until after the Idle anims were requested
	owner->SheathWeapon();

	// Let the AI update their weapons (make them nonsolid)
	owner->UpdateAttachmentContents(false);
}

// Gets called each time the mind is thinking
void IdleState::Think(idAI* owner)
{
	Memory& memory = owner->GetMemory();
	idStr waitState = owner->WaitState();

	if (_startSleeping && !owner->HasSeenEvidence() && owner->GetMoveType() == MOVETYPE_ANIM)
	{
		if (owner->ReachedPos(memory.idlePosition, MOVE_TO_POSITION) 
			&& owner->GetCurrentYaw() == memory.idleYaw)
		{
			owner->actionSubsystem->ClearTasks();
			owner->senseSubsystem->ClearTasks();

			owner->GetMind()->SwitchState(STATE_IDLE_SLEEP);
			return;
		}
		else if ( owner->AI_DEST_UNREACHABLE ) // grayman #5164
		{
			if ( gameLocal.time >= owner->m_nextWarningTime )
			{
				gameLocal.Warning("IdleState::Think %s (%s) can't sleep: too far from sleeping location (%s)\n", owner->GetName(), owner->GetPhysics()->GetOrigin().ToString(), memory.idlePosition.ToString());
				owner->GetMind()->SwitchState(owner->backboneStates[ERelaxed]);
				owner->m_nextWarningTime = gameLocal.time + WARNING_DELAY;
			}
			return;
		}
	}
	else if (owner->GetMoveType() == MOVETYPE_SLEEP)
	{
		owner->GetMind()->SwitchState(STATE_IDLE_SLEEP);
		return;
	}
	else if (_startSitting && owner->GetMoveType() != MOVETYPE_SIT && waitState != "sit_down")
	{
		if (owner->ReachedPos(memory.idlePosition, MOVE_TO_POSITION)
			&& owner->GetCurrentYaw() == memory.idleYaw)
		{
			owner->SitDown();
		}
		else if ( owner->AI_DEST_UNREACHABLE ) // grayman #5164
		{
			if ( gameLocal.time >= owner->m_nextWarningTime )
			{
				gameLocal.Warning("%s (%s) can't sit: too far from sitting location (%s)\n", owner->GetName(), owner->GetPhysics()->GetOrigin().ToString(), memory.idlePosition.ToString());
				owner->GetMind()->SwitchState(owner->backboneStates[ERelaxed]);
				owner->m_nextWarningTime = gameLocal.time + WARNING_DELAY;
			}
			return;
		}
	}

	UpdateAlertLevel();

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner))
	{
		return;
	}

	// Let the AI check its senses
	owner->PerformVisualScan();
}

void IdleState::InitialiseMovement(idAI* owner)
{
	Memory& memory = owner->GetMemory();

	owner->AI_RUN = false;

	// The movement subsystem should start patrolling
	owner->movementSubsystem->ClearTasks();

	// angua: store the position at map start
	if ( memory.idlePosition == idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY) )
	{
		// No idle position saved yet, take the current one
		memory.idlePosition = owner->GetPhysics()->GetOrigin();
		memory.returnSitPosition = memory.idlePosition; // grayman #3989
		memory.idleYaw = owner->GetCurrentYaw();
	}

	if ( owner->spawnArgs.GetBool("patrol", "1") ) // grayman #2683 - only start patrol if you're supposed to 
	{
		// grayman #3528 - if you need to stay where you are, don't start patrolling yet

		if ( memory.stayPut )
		{
			memory.stayPut = false; // reset
		}
		else
		{
			owner->movementSubsystem->StartPatrol();
		}
	}

	// Check if the owner has patrol routes set
	idPathCorner* path = memory.currentPath.GetEntity();
	idPathCorner* lastPath = memory.lastPath.GetEntity();

	if (path == NULL && lastPath == NULL)
	{
		// We don't have any patrol routes, so we're supposed to stand around
		// where the mapper has put us.
		
		if (owner->GetMoveType() == MOVETYPE_ANIM)
		{
			// angua: don't do this when we are sitting or sleeping
			// We already HAVE an idle position set, this means that we are
			// supposed to be there, let's move
			float startPosTolerance = owner->spawnArgs.GetFloat("startpos_tolerance", "-1"); // grayman #3989
			owner->movementSubsystem->PushTask(
				TaskPtr(new MoveToPositionTask(memory.idlePosition, memory.idleYaw, startPosTolerance))
			);
		}
	}
}

void IdleState::InitialiseCommunication(idAI* owner)
{
	owner->commSubsystem->ClearTasks(); // grayman #3182

	if (owner->m_recentHighestAlertLevel >= owner->thresh_3)
	{
		// grayman #3338 - Delay greetings if coming off a search.
		// Divide the max alert level achieved so far in half, and
		// delay for that number of minutes.
		if ( owner->greetingState != ECannotGreet )
		{
			owner->greetingState = ECannotGreetYet;
			float delay = (owner->m_maxAlertIndex/2.0f)*60.0f;
			owner->PostEventSec(&AI_AllowGreetings,delay);
		}
	}

	// Push a single bark to the communication subsystem first, it fires only once
	idStr bark = GetInitialIdleBark(owner);
	if (!bark.IsEmpty())
	{
		owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask(bark)));

		if (cv_ai_debug_transition_barks.GetBool())
		{
			if (owner->HasSeenEvidence())
			{
				gameLocal.Printf("%d: %s barks '%s' when ramping down to Alert Idle state\n",gameLocal.time,owner->GetName(),bark.c_str());
			}
			else
			{
				gameLocal.Printf("%d: %s barks '%s' when ramping down to Idle state\n",gameLocal.time,owner->GetName(),bark.c_str());
			}
		}
	}
}


void IdleState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);

	savefile->WriteBool(_startSitting);
	savefile->WriteBool(_startSleeping);
}

void IdleState::Restore(idRestoreGame* savefile)
{
	State::Restore(savefile);

	savefile->ReadBool(_startSitting);
	savefile->ReadBool(_startSleeping);
}

idStr IdleState::GetInitialIdleBark(idAI* owner)
{
	// greebo: Ported from ai_darkmod_base::task_Idle written by SZ

	Memory& memory = owner->GetMemory();

	// Decide what sound it is appropriate to play
	idStr soundName("");

	if ( !owner->m_RelightingLight &&	// grayman #2603 - No rampdown bark if relighting a light.
		 !owner->m_ExaminingRope )		// grayman #2872 - No rampdown bark if examining a rope.
	{
		EAlertClass aclass = memory.alertClass;
		if (owner->m_recentHighestAlertLevel >= owner->thresh_4)
		{
			// has gone up to Agitated Searching
			soundName = "snd_alertdown0SeenNoEvidence";
		}
		else if (owner->m_recentHighestAlertLevel >= owner->thresh_2) // has gone up to Suspicious or Searching
		{
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

StatePtr IdleState::CreateInstance()
{
	return StatePtr(new IdleState);
}

void IdleState::OnChangeTarget(idAI* owner)
{
	// re-initialize movement subsystem to catch new path_corners
	InitialiseMovement(owner);
}

// Register this state with the StateLibrary
StateLibrary::Registrar idleStateRegistrar(
	STATE_IDLE, // Task Name
	StateLibrary::CreateInstanceFunc(&IdleState::CreateInstance) // Instance creation callback
);

} // namespace ai
