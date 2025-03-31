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



#include "../Memory.h"
#include "MoveToCoverTask.h"
#include "../Library.h"

namespace ai
{

// Get the name of this task
const idStr& MoveToCoverTask::GetName() const
{
	static idStr _name(TASK_MOVE_TO_COVER);
	return _name;
}

void MoveToCoverTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	Task::Init(owner, subsystem);
	idActor* enemy = owner->GetEnemy();

	//Move to cover position
	owner->AI_RUN = true;
	owner->AI_FORWARD = true;
	owner->m_pathRank = owner->rank; // grayman #2345

	// grayman #3280 - enemies look with their eyes, not their feet
	owner->MoveToCover(enemy, enemy->GetEyePosition()); 
}

bool MoveToCoverTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Move to Cover Task performing.\r");

	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointer
	assert(owner != NULL);

	if (owner->AI_DEST_UNREACHABLE)
	{
		//TODO
		DM_LOG(LC_AI, LT_INFO)LOGSTRING("Destination unreachable.\r");
		return true;
	}

	if (owner->AI_MOVE_DONE)
	{
		// Move is done, 
		DM_LOG(LC_AI, LT_INFO)LOGSTRING("Move is done.\r");
		owner->TurnToward(owner->lastVisibleEnemyPos);

		// finish this task

		return true;
	}

	return false; // not finished yet
}

MoveToCoverTaskPtr MoveToCoverTask::CreateInstance()
{
	return MoveToCoverTaskPtr(new MoveToCoverTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar moveToCoverTaskRegistrar(
	TASK_MOVE_TO_COVER, // Task Name
	TaskLibrary::CreateInstanceFunc(&MoveToCoverTask::CreateInstance) // Instance creation callback
);

} // namespace ai
