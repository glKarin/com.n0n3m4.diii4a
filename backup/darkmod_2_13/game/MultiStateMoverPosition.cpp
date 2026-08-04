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



#include "MultiStateMoverPosition.h"
#include "MultiStateMoverButton.h"
#include "MultiStateMover.h"

CLASS_DECLARATION( idEntity, CMultiStateMoverPosition )
	EVENT( EV_PostSpawn,	CMultiStateMoverPosition::Event_PostSpawn )
END_CLASS

void CMultiStateMoverPosition::Spawn() 
{
	PostEventMS(&EV_PostSpawn, 4);
}

void CMultiStateMoverPosition::Event_PostSpawn()
{
	// Find all AAS obstacle entities among the targets
	for (int i = 0; i < targets.Num(); i++)
	{
		idEntity* target = targets[i].GetEntity();

		if (target != NULL && target->IsType(idFuncAASObstacle::Type))
		{
			// Allocate a new list element and call the operator=
			aasObstacleEntities.Alloc() = static_cast<idFuncAASObstacle*>(target);
		}
	}

	// Remove all AAS obstacle entities from our targets, they should not be blindly triggered
	for (int i = 0; i < aasObstacleEntities.Num(); i++)
	{
		RemoveTarget(aasObstacleEntities[i].GetEntity());
	}
}

void CMultiStateMoverPosition::SetMover(CMultiStateMover* newMover)
{
	mover = newMover;
}

CMultiStateMoverButton*	CMultiStateMoverPosition::GetFetchButton( idVec3 riderOrg ) // grayman #3029
{
	CMultiStateMover* m = mover.GetEntity();
	if (m == NULL)
	{
		return NULL;
	}

	return m->GetButton(this, NULL, BUTTON_TYPE_FETCH, riderOrg); // grayman #3029
}

CMultiStateMoverButton*	CMultiStateMoverPosition::GetRideButton(CMultiStateMoverPosition* toPosition)
{
	CMultiStateMover* m = mover.GetEntity();
	if (m == NULL) return NULL;

	return m->GetButton(toPosition, this, BUTTON_TYPE_RIDE, vec3_zero); // grayman #3029
}

void CMultiStateMoverPosition::Save(idSaveGame *savefile) const
{
	savefile->WriteInt(aasObstacleEntities.Num());
	for (int i = 0; i < aasObstacleEntities.Num(); i++)
	{
		aasObstacleEntities[i].Save(savefile);
	}

	mover.Save(savefile);
}

void CMultiStateMoverPosition::Restore(idRestoreGame *savefile)
{
	int num;
	savefile->ReadInt(num);
	aasObstacleEntities.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		aasObstacleEntities[i].Restore(savefile);
	}

	mover.Restore(savefile);
}

void CMultiStateMoverPosition::OnMultistateMoverArrive(CMultiStateMover* mover)
{
	if (mover == NULL) return;

	// First, check if we should trigger our targets
	if (spawnArgs.GetBool("always_trigger_targets", "1"))
	{
		ActivateTargets(mover);

		// Tell the idFuncAASObstacles to (re-)activate the AAS areas
		for (int i = 0; i < aasObstacleEntities.Num(); i++)
		{
			aasObstacleEntities[i].GetEntity()->SetAASState(false);
		}
	}

	// Run the mover event script
	RunMoverEventScript("call_on_arrive", mover);
}

void CMultiStateMoverPosition::OnMultistateMoverLeave(CMultiStateMover* mover)
{
	if (mover == NULL) return;

	// First, check if we should trigger our targets
	if (spawnArgs.GetBool("always_trigger_targets", "1"))
	{
		ActivateTargets(mover);

		// Handle the idFuncAASObstacles separately, tell them to deactivate the AAS areas
		for (int i = 0; i < aasObstacleEntities.Num(); i++)
		{
			aasObstacleEntities[i].GetEntity()->SetAASState(true);
		}
	}

	// Run the mover event script
	RunMoverEventScript("call_on_leave", mover);
}

void CMultiStateMoverPosition::RunMoverEventScript(const idStr& spawnArg, CMultiStateMover* mover)
{
	idStr scriptFuncName;
	if (!spawnArgs.GetString(spawnArg, "", scriptFuncName))
	{
		return; // no scriptfunction
	}

	// Script function signature is like this: void scriptobj::onMultiStateMover(entity mover)
	idThread* thread = CallScriptFunctionArgs(scriptFuncName, true, 0, "ee", this, mover);
	if (thread != NULL)
	{
		// greebo: Run the thread at once, the script result might be needed below.
		thread->Execute();
	}
}
