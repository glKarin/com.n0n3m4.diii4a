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



#include "RandomHeadturnTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

// Get the name of this task
const idStr& RandomHeadturnTask::GetName() const
{
	static idStr _name(TASK_RANDOM_HEADTURN);
	return _name;
}

void RandomHeadturnTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);
}

bool RandomHeadturnTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("RandomHeadturnTask performing.\r");

	idAI* owner = _owner.GetEntity();
	
	// This task may not be performed with empty entity pointers
	assert(owner != NULL);

	Memory& memory = owner->GetMemory();

	int nowTime = gameLocal.time;

	// Wait until last head turning is finished
	if (nowTime >= memory.headTurnEndTime)
	{
		if (memory.currentlyHeadTurning)
		{
			// Wait before starting the next head turn check
			memory.currentlyHeadTurning = false;
			SetNextHeadTurnCheckTime();
		}
		else if (nowTime >= owner->GetMemory().nextHeadTurnCheckTime)
		{
			PerformHeadTurnCheck();
		}
	}
	return false; // not finished yet
}

void RandomHeadturnTask::PerformHeadTurnCheck()
{
	idAI* owner = _owner.GetEntity();
	Memory& memory = owner->GetMemory();

	// Chance to turn head is higher when the AI is searching or has seen evidence of intruders
	float chance;
	if ( ( owner->AI_AlertIndex >= ESearching ) || owner->HasSeenEvidence() )
	{
		chance = owner->m_headTurnChanceIdle * owner->m_headTurnFactorAlerted;
	}
	else
	{
		chance = owner->m_headTurnChanceIdle;
	}

	float rand = gameLocal.random.RandomFloat();
	if (rand >= chance)
	{
		// Nope, set the next time when a check should be performed
		SetNextHeadTurnCheckTime();
		return;
	}

	idAnimator* animator = owner->GetAnimatorForChannel(ANIMCHANNEL_TORSO);
	int animnum = animator->CurrentAnim(ANIMCHANNEL_TORSO)->AnimNum();
	animFlags_t animflags = animator->GetAnimFlags(animnum);

	// angua: check if the current animation allows random headturning
	// if the focusTime > current time, the AI is currently looking at a specified direction or entity
	// we don't want to interrupt that
	if (!animflags.no_random_headturning && owner->GetFocusTime() < gameLocal.time && 
			owner->spawnArgs.GetBool("allow_random_headturning", "1")) // angua: allow or disable random headturning by spawn arg (also works while the map is running)
	{
		// Yep, set the angles and start head turning
		memory.currentlyHeadTurning = true;

		// Generate yaw angle in degrees
		float range = 2 * owner->m_headTurnMaxYaw;
		float headYawAngle = gameLocal.random.RandomFloat()*range - owner->m_headTurnMaxYaw;
		
		// Generate pitch angle in degrees
		range = 2 * owner->m_headTurnMaxPitch;
		float headPitchAngle = gameLocal.random.RandomFloat()*range - owner->m_headTurnMaxPitch;

		// Generate duration in seconds
		range = owner->m_headTurnMaxDuration - owner->m_headTurnMinDuration;
		int duration = static_cast<int>(gameLocal.random.RandomFloat()*range + owner->m_headTurnMinDuration);

		memory.headTurnEndTime = gameLocal.time + duration;
		
		// Call event
		owner->Event_LookAtAngles(headYawAngle, headPitchAngle, 0.0, MS2SEC(duration));
	}
}

void RandomHeadturnTask::SetNextHeadTurnCheckTime()
{
	idAI* owner = _owner.GetEntity();

	// Set the time when the next check should be performed (with a little bit of randomness)
	int nowTime = gameLocal.time;
	owner->GetMemory().nextHeadTurnCheckTime = static_cast<int>(nowTime + 0.8f * owner->m_timeBetweenHeadTurnChecks 
		+ gameLocal.random.RandomFloat() * 0.4f * owner->m_timeBetweenHeadTurnChecks);
}

RandomHeadturnTaskPtr RandomHeadturnTask::CreateInstance()
{
	return RandomHeadturnTaskPtr(new RandomHeadturnTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar randomHeadturnTaskRegistrar(
	TASK_RANDOM_HEADTURN, // Task Name
	TaskLibrary::CreateInstanceFunc(&RandomHeadturnTask::CreateInstance) // Instance creation callback
);

} // namespace ai
