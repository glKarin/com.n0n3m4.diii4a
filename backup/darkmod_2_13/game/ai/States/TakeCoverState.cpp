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



#include "TakeCoverState.h"
#include "../Memory.h"
#include "../Tasks/MoveToCoverTask.h"
#include "../Tasks/WaitTask.h"
#include "../Tasks/MoveToPositionTask.h"
//#include "../Tasks/RandomHeadturnTask.h"
#include "LostTrackOfEnemyState.h"
#include "StayInCoverState.h"
#include "../Library.h"

namespace ai
{

// Get the name of this state
const idStr& TakeCoverState::GetName() const
{
	static idStr _name(STATE_TAKE_COVER);
	return _name;
}

void TakeCoverState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("TakeCoverState initialised.\r");
	assert(owner);

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	// angua: The last position of the AI before it takes cover, so it can return to it later.
	memory.positionBeforeTakingCover = owner->GetPhysics()->GetOrigin();

	// Fill the subsystems with their tasks

	// The movement subsystem should wait half a second and then run to Cover position, 
	// wait there for some time and then emerge to have a look.
	owner->StopMove(MOVE_STATUS_DONE);
	memory.StopReacting(); // grayman #3559
	owner->movementSubsystem->ClearTasks();
	owner->movementSubsystem->PushTask(TaskPtr(new WaitTask(500)));
	owner->movementSubsystem->QueueTask(MoveToCoverTask::CreateInstance());
	owner->AI_MOVE_DONE = false;

	// The communication system 
	// owner->GetSubsystem(SubsysCommunication)->ClearTasks(); // TODO_AI

	// The sensory system 
	owner->senseSubsystem->ClearTasks();

	// No action (in case you were throwing rocks)
	owner->actionSubsystem->ClearTasks();

	// grayman #3857 - Stop searching
	owner->searchSubsystem->ClearTasks();
}

// Gets called each time the mind is thinking
void TakeCoverState::Think(idAI* owner)
{
	if (owner->AI_MOVE_DONE && !owner->AI_DEST_UNREACHABLE)
	{
		// When we are done moving to cover position, stay for some time there 
		// then come out and move back to where we were standing before taking cover
		owner->AI_RUN = false;
		owner->TurnToward(owner->lastVisibleEnemyPos);
		
		owner->GetMind()->SwitchState(STATE_STAY_IN_COVER);
	}
	else if (owner->AI_DEST_UNREACHABLE)
	{
		owner->GetMind()->EndState();
	}
}

StatePtr TakeCoverState::CreateInstance()
{
	return StatePtr(new TakeCoverState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar takeCoverStateRegistrar(
	STATE_TAKE_COVER, // Task Name
	StateLibrary::CreateInstanceFunc(&TakeCoverState::CreateInstance) // Instance creation callback
);

} // namespace ai
