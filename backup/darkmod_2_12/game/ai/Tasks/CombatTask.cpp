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



#include "CombatTask.h"
#include "SingleBarkTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

CombatTask::CombatTask() :
	_lastCombatBarkTime(-1),
	_nextAttackTime(0) // grayman #4412
{}

void CombatTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	Task::Init(owner, subsystem);
	
	_enemy = owner->GetEnemy();
}

void CombatTask::EmitCombatBark(idAI* owner, const idStr& sndName)
{
	// This will hold the message to be delivered with the bark, if appropriate
	CommMessagePtr message;
	
	// Only alert the bystanders if we didn't receive the alert by message ourselves
	message = CommMessagePtr(new CommMessage(
		CommMessage::DetectedEnemy_CommType, 
		owner, NULL, // from this AI to anyone 
		owner->GetEnemy(),
		owner->GetPhysics()->GetOrigin(),
		-1
	));

	_lastCombatBarkTime = gameLocal.time;

	// The communication system 
	owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask(sndName, message)));
}

void CombatTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	_enemy.Save(savefile);
	savefile->WriteInt(_lastCombatBarkTime);
	savefile->WriteInt(_nextAttackTime); // grayman #4412
}

void CombatTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	_enemy.Restore(savefile);
	savefile->ReadInt(_lastCombatBarkTime);
	savefile->ReadInt(_nextAttackTime); // grayman #4412
}

} // namespace ai
