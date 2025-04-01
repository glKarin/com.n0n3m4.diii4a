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
#include "PathAnimTask.h"
#include "PathTurnTask.h"
#include "../Library.h"

namespace ai
{

PathAnimTask::PathAnimTask() :
	PathTask()
{}

PathAnimTask::PathAnimTask(idPathCorner* path) :
	PathTask(path)
{
	_path = path;
}

// Get the name of this task
const idStr& PathAnimTask::GetName() const
{
	static idStr _name(TASK_PATH_ANIM);
	return _name;
}

void PathAnimTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	PathTask::Init(owner, subsystem);

	idPathCorner* path = _path.GetEntity();

	// Parse animation spawnargs here
	idStr animName = path->spawnArgs.GetString("anim");
	if (animName.IsEmpty())
	{
		gameLocal.Warning("path_anim entity %s without 'anim' spawnarg found.",path->name.c_str());
		subsystem.FinishTask();
		return; // grayman #3670
	}

	int blendIn = path->spawnArgs.GetInt("blend_in");
	
	// Custom AnimState scripts should start the anim themselves, so pass it as a key -- SteveL #3800
	owner->spawnArgs.Set( "customAnim_requested_anim", animName );
	owner->spawnArgs.Set( "customAnim_cycle", "0" );

	owner->SetAnimState( ANIMCHANNEL_TORSO, "Torso_CustomAnim", blendIn );
	// SteveL #4012: Use OverrideAnim instead of a matching "Legs_CustomAnim", 
	// which invites race conditions and conflicts between game code and scripts.
	owner->SetAnimState( ANIMCHANNEL_LEGS, "Legs_Idle", 4 ); // Queue up the next state before sync'ing legs to torso
	owner->Event_SetBlendFrames( ANIMCHANNEL_LEGS, 10 ); // ~0.4 seconds.
	owner->PostEventMS( &AI_OverrideAnim, 0, ANIMCHANNEL_LEGS ); 

	// greebo: Set the waitstate, this gets cleared by 
	// the script function when the animation is done.
	owner->SetWaitState("customAnim");

	owner->GetMind()->GetMemory().playIdleAnimations = false;
}

void PathAnimTask::OnFinish(idAI* owner)
{
	// NextPath();

	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 5);
	owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", 5);

	owner->SetWaitState("");

	owner->GetMind()->GetMemory().playIdleAnimations = true;

	// grayman #3670 - Trigger path targets, now that the anim is done

	idPathCorner* path = _path.GetEntity();

	// This task may not be performed with empty entity pointers
	assert( path != NULL );

	path->ActivateTargets(owner);
}

bool PathAnimTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("PathAnimTask performing.\r");

	idAI* owner = _owner.GetEntity();

	// This task may not be performed with an empty owner pointer
	assert(owner != NULL);

	// Exit when the waitstate is not "customAnim" anymore
	idStr waitState(owner->WaitState());
	return (waitState != "customAnim");
}

PathAnimTaskPtr PathAnimTask::CreateInstance()
{
	return PathAnimTaskPtr(new PathAnimTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar pathAnimTaskRegistrar(
	TASK_PATH_ANIM, // Task Name
	TaskLibrary::CreateInstanceFunc(&PathAnimTask::CreateInstance) // Instance creation callback
);

} // namespace ai
