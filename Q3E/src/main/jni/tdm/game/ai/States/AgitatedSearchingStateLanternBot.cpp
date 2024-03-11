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



#include "AgitatedSearchingStateLanternBot.h"
#include "../Tasks/ScriptTask.h"
#include "../Tasks/RepeatedBarkTask.h"

namespace ai
{

// Get the name of this state
const idStr& AgitatedSearchingStateLanternBot::GetName() const
{
	static idStr _name(STATE_AGITATED_SEARCHING_LANTERN_BOT);
	return _name;
}

bool AgitatedSearchingStateLanternBot::CheckAlertLevel(idAI* owner)
{
	if (owner->AI_AlertIndex < EAgitatedSearching)
	{
		// Alert index is too low for this state, fall back
		owner->GetMind()->EndState();
		return false;
	}

	// Don't let this AI go higher than this state

	// Alert Index is matching, return OK
	return true;
}

void AgitatedSearchingStateLanternBot::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("AgitatedSearchingStateLanternBot initialised.\r");
	assert(owner);

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner))
	{
		return;
	}

	owner->movementSubsystem->ClearTasks();
	owner->senseSubsystem->ClearTasks();
	owner->actionSubsystem->ClearTasks();
	owner->commSubsystem->ClearTasks(); // grayman #3182
	owner->searchSubsystem->ClearTasks(); // grayman #3857

	owner->StopMove(MOVE_STATUS_DONE);

	// Start with an invalid position
	_curAlertPos = idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY);

	// Move to a position where we can light up the alert position from
	MoveTowardAlertPos(owner);

	owner->GetMemory().currentSearchEventID = owner->LogSuspiciousEvent( E_EventTypeMisc, owner->GetPhysics()->GetOrigin(), NULL, true ); // grayman #3857

	// This will hold the message to be delivered with the inaudible bark
	CommMessagePtr message(new CommMessage(
		CommMessage::RequestForHelp_CommType, // grayman #3857 - asking for a response
		//CommMessage::DetectedEnemy_CommType,  // grayman #3857 - this does nothing when no entity (parameter 4) is provided
		owner, NULL,// from this AI to anyone 
		NULL,
		_curAlertPos,
		owner->GetMemory().currentSearchEventID // grayman #3857 (was '0')
	));

	// The communication system plays starting bark
	owner->commSubsystem->AddCommTask(
		CommunicationTaskPtr(new RepeatedBarkTask("snd_spotted_noise", 3000, 4000, message))
	);

	// Add the script task blowing the alarm whistle
	owner->actionSubsystem->PushTask(TaskPtr(new ScriptTask("startAlarmWhistle")));
}

void AgitatedSearchingStateLanternBot::OnMovementBlocked(idAI* owner)
{
	AgitatedSearchingState::OnMovementBlocked(owner);

	// Begin counting down the alertness, we're blocked
	CalculateAlertDecreaseRate(owner);
}

// Gets called each time the mind is thinking
void AgitatedSearchingStateLanternBot::Think(idAI* owner)
{
	UpdateAlertLevel();

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner))
	{
//		owner->GetMind()->EndState(); // grayman #3182 - already done in CheckAlertLevel()
		return;
	}

	// Move (if alert position has changed)
	MoveTowardAlertPos(owner);

	if (owner->GetMoveStatus() == MOVE_STATUS_DONE)
	{
		// Look at alert position, now that we've finished moving
		owner->TurnToward(_curAlertPos);

		// Let the alertness decrease from now on
		CalculateAlertDecreaseRate(owner);
	}
	else
	{
		// Moving along, ensure that we're not dropping out of this state as long as we're moving
		_alertLevelDecreaseRate = 0;
		return;
	}
}

void AgitatedSearchingStateLanternBot::MoveTowardAlertPos(idAI* owner)
{
	Memory& memory = owner->GetMemory();

	if (_curAlertPos.Compare(memory.alertPos, 150))
	{
		// alert position is roughly the same as the current one, quit
		return;
	}

	// Memorise this position
	_curAlertPos = memory.alertPos;
	
	aasGoal_t goal = owner->GetPositionWithinRange(_curAlertPos);

	if (goal.areaNum != -1)
	{
		// Found a suitable attack position, now move to it
		owner->MoveToPosition(goal.origin);
	}
	else
	{
		// No suitable position fall back to the direct position
		owner->MoveToPosition(_curAlertPos);
	}
}

void AgitatedSearchingStateLanternBot::Save(idSaveGame* savefile) const
{
	AgitatedSearchingState::Save(savefile);

	savefile->WriteVec3(_curAlertPos);
}

void AgitatedSearchingStateLanternBot::Restore(idRestoreGame* savefile)
{
	AgitatedSearchingState::Restore(savefile);

	savefile->ReadVec3(_curAlertPos);
}

StatePtr AgitatedSearchingStateLanternBot::CreateInstance()
{
	return StatePtr(new AgitatedSearchingStateLanternBot);
}

// Register this state with the StateLibrary
StateLibrary::Registrar agitatedSearchingStateLanternBotRegistrar(
	STATE_AGITATED_SEARCHING_LANTERN_BOT, // Task Name
	StateLibrary::CreateInstanceFunc(&AgitatedSearchingStateLanternBot::CreateInstance) // Instance creation callback
);

} // namespace ai
