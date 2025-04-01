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
#include "PlayAnimationTask.h"
#include "../Library.h"

#define DEFAULT_BLEND_FRAMES 4

namespace ai
{

PlayAnimationTask::PlayAnimationTask() :
	_blendFrames(DEFAULT_BLEND_FRAMES),
	_playCycle(false)
{}

PlayAnimationTask::PlayAnimationTask(const idStr& animName, int blendFrames, bool playCycle) :
	_animName(animName),
	_blendFrames(blendFrames),
	_playCycle(playCycle)
{}

// Get the name of this task
const idStr& PlayAnimationTask::GetName() const
{
	static idStr _name(TASK_PLAY_ANIMATION);
	return _name;
}

void PlayAnimationTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	// Parse animation spawnargs here
	if (_animName.IsEmpty())
	{
		gameLocal.Warning("%s cannot start PlayAnimationTask with empty animation name.",owner->name.c_str());
		subsystem.FinishTask();
		return; // grayman #3670
	}

	StartAnim(owner);
}

void PlayAnimationTask::OnFinish(idAI* owner)
{
	owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", _blendFrames);
	owner->SetWaitState("");
}

bool PlayAnimationTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("PlayAnimationTask performing.\r");

	idAI* owner = _owner.GetEntity();

	// This task may not be performed with an empty owner pointer
	assert(owner != NULL);

	// Exit when the waitstate is not "customAnim" anymore
	idStr waitState(owner->WaitState());

	if (waitState != "customAnim")
	{
		// We're finished, what now?
		if (_playCycle)
		{
			// Starte the anim cycle again
			StartAnim(owner);
		}
		else
		{
			return true;
		}
	}

	return false;
}

void PlayAnimationTask::StartAnim(idAI* owner)
{
	// Allow AnimState scripts to start the anim themselves by passing it as a key -- SteveL #3800
	owner->spawnArgs.Set( "customAnim_requested_anim", _animName );
	owner->spawnArgs.Set( "customAnim_cycle", _playCycle ? "1" : "0" );

	// Set the name of the state script
	owner->SetAnimState( ANIMCHANNEL_TORSO, "Torso_CustomAnim", _blendFrames );
	// SteveL #4012: Use OverrideAnim instead of a matching "Legs_CustomAnim", 
	// which invites race conditions and conflicts between game code and scripts.
	owner->SetAnimState( ANIMCHANNEL_LEGS, "Legs_Idle", 4 ); // Queue up the next state before sync'ing legs to torso
	owner->Event_SetBlendFrames( ANIMCHANNEL_LEGS, 10 ); // ~0.4 seconds.
	owner->PostEventMS( &AI_OverrideAnim, 0, ANIMCHANNEL_LEGS ); 
	
	// greebo: Set the waitstate, this gets cleared by 
	// the script function when the animation is done.
	owner->SetWaitState("customAnim");
}

// Save/Restore methods
void PlayAnimationTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteString(_animName);
	savefile->WriteInt(_blendFrames);
	savefile->WriteBool(_playCycle);
}

void PlayAnimationTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadString(_animName);
	savefile->ReadInt(_blendFrames);
	savefile->ReadBool(_playCycle);
}

PlayAnimationTaskPtr PlayAnimationTask::CreateInstance()
{
	return PlayAnimationTaskPtr(new PlayAnimationTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar playAnimationTaskRegistrar(
	TASK_PLAY_ANIMATION, // Task Name
	TaskLibrary::CreateInstanceFunc(&PlayAnimationTask::CreateInstance) // Instance creation callback
);

} // namespace ai
