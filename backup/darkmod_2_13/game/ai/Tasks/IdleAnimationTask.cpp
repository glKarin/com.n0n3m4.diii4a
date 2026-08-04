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



#include "IdleAnimationTask.h"
#include "../Memory.h"
#include "../Library.h"

#include <vector>
#include <string>
#include "StdString.h"

namespace ai
{

IdleAnimationTask::IdleAnimationTask() :
	_nextAnimationTime(-1),
	_idleAnimationInterval(-1),
	_lastIdleAnim(-1)
{}

// Get the name of this task
const idStr& IdleAnimationTask::GetName() const
{
	static idStr _name(TASK_IDLE_ANIMATION);
	return _name;
}

void IdleAnimationTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	// grayman #3857 - verify that this task should be running:
	// yes if in states 0,2,3
	// no if in states 1,4,5

	if ((owner->AI_AlertIndex == EObservant) || (owner->AI_AlertIndex >= EAgitatedSearching))
	{
		subsystem.FinishTask();
	}

	// grayman #3857 - clear lists, just to be sure
	_idleAnimations.Clear();
	_idleAnimationsTorso.Clear();
	_idleAnimationsSitting.Clear();

	// grayman #3857 - can now do idle search/suspicious anims while in states 2, 3, or 4
	if ( (owner->AI_AlertLevel >= owner->thresh_2) && (owner->AI_AlertLevel < owner->thresh_5) )
	{
		// grayman #3857 - Read the idle anims to be used when suspicious or searching.
		// These won't be run when the AI is actively searching (moving around), but
		// will be run when they're guarding or observing.
		ParseAnimsToList(owner->spawnArgs.GetString("idle_animations_searching"), _idleAnimations);

		// Read the animation interval from the owner's spawnarg
		_idleAnimationInterval = SEC2MS(owner->spawnArgs.GetInt("idle_search_animations_interval", "-1"));
	}
	else // when idle in state 0
	{
		// Read the general-purpose animations first
		ParseAnimsToList(owner->spawnArgs.GetString("idle_animations"), _idleAnimations);
	
		// Now read the anims for the torso only
		ParseAnimsToList(owner->spawnArgs.GetString("idle_animations_torso"), _idleAnimationsTorso);

		// Now read the anims for sitting AI
		ParseAnimsToList(owner->spawnArgs.GetString("idle_animations_sitting"), _idleAnimationsSitting);

		// Read the animation interval from the owner's spawnarg
		_idleAnimationInterval = SEC2MS(owner->spawnArgs.GetInt("idle_animations_interval", "-1"));
	}

	if ( (_idleAnimationInterval > 0) && 
			( (_idleAnimations.Num() > 0) || (_idleAnimationsTorso.Num() > 0) || (_idleAnimationsSitting.Num() > 0) ))
	{
		_nextAnimationTime = static_cast<int>(gameLocal.time + gameLocal.random.RandomFloat()*_idleAnimationInterval);
	}
	else
	{
		// No idle animation interval set or no animations, finish this task
		subsystem.FinishTask();
	}
}

void IdleAnimationTask::ParseAnimsToList(const std::string& animStringList, idStringList& targetList)
{
	std::vector<std::string> anims; // will hold the separated strings
	stdext::split(anims, animStringList, " ,");

	// Copy the strings into the target idStringList
	for (std::size_t i = 0; i < anims.size(); i++)
	{
		if (!anims[i].empty())
		{
			targetList.Append(idStr(anims[i].c_str()));
		}
	}
}

bool IdleAnimationTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("IdleAnimationTask performing.\r");

	idAI* owner = _owner.GetEntity();

	// This task may not be performed with empty entity pointers
	assert(owner != NULL);

	// grayman #3857 - verify that this task should be running:
	// yes if in states 0,2,3
	// no if in states 1,4,5

	if ((owner->AI_AlertIndex == EObservant) || (owner->AI_AlertIndex >= EAgitatedSearching))
	{
		return true;
	}

	Memory& memory = owner->GetMemory();

	if (gameLocal.time > _nextAnimationTime)
	{
		// grayman #2169 - no idle animations while drowning

		idAI* ai = static_cast<idAI*>(owner);
		bool drowning = false;
		if (ai)
		{
			drowning = ai->MouthIsUnderwater();
		}

		int delay;

		// angua: don't play idle animations while sitting / lying down or getting up

		// grayman #2345 - no idle animations while handling a door and not waiting
		// in a door queue, since they can interfere with reaching for the door handle

		// SteveL #3806 - no idles if the AI wants to turn: check FacingIdeal too

		// grayman #3857 - Don't play if the AI is an active searcher (it's okay if
		// he's a guard or observer participating in a search. Test this by seeing if
		// a hiding spot investigation is in progress.

		// grayman #3857 - Don't play if this is an idle search anim and the AI is moving.

		// grayman #4040 - No idle anims if AI_RUN is set and waiting in the door queue.

		moveType_t moveType = owner->GetMoveType();
		if (memory.playIdleAnimations &&
			owner->FacingIdeal() &&
			!(owner->AI_RUN && owner->AI_FORWARD) && // grayman #3857 - AI_RUN might be left over after coming to a full stop
			!(owner->AI_FORWARD && (owner->AI_AlertIndex >= ESuspicious)) &&  // grayman #3857 - only play idle search anims when standing
			moveType != MOVETYPE_SIT_DOWN &&
			moveType != MOVETYPE_FALL_ASLEEP && // grayman #3820 - was MOVETYPE_LAY_DOWN
			moveType != MOVETYPE_SLEEP &&
			moveType != MOVETYPE_GET_UP &&
			moveType != MOVETYPE_WAKE_UP && // grayman #3820 - MOVETYPE_WAKE_UP was MOVETYPE_GET_UP_FROM_LYING
			!drowning &&
			(!owner->m_HandlingDoor || (owner->GetMoveStatus() == MOVE_STATUS_WAITING)) &&
			!(owner->AI_RUN && owner->m_HandlingDoor && (owner->GetMoveStatus() == MOVE_STATUS_WAITING)) && // grayman #4040
			!owner->GetMemory().hidingSpotInvestigationInProgress &&
			owner->spawnArgs.GetBool("allow_idle_anims", "1")) // angua: allow or disable idle anims by spawn arg (also works while the map is running)
		{
			// Check if the AI is moving or sitting, this determines which channel we can play on
			if (!owner->AI_FORWARD && (moveType != MOVETYPE_SIT))
			{
				// AI is not walking or sitting, play animations affecting all channels

				AttemptToPlayAnim(owner, _idleAnimations, false);
			}
			else if (moveType == MOVETYPE_SIT)
			{
				AttemptToPlayAnim(owner, _idleAnimationsSitting, false); // Changed in #3914 to play on legs too instead of torso only
			}
			else
			{
				// AI is walking, only use animations for the Torso channel
				AttemptToPlayAnim(owner, _idleAnimationsTorso, true);
			}

			// grayman #3857 - full delay when playing an anim
			delay = static_cast<int>(_idleAnimationInterval*(0.8f + gameLocal.random.RandomFloat()*0.4f));
		}
		else if (!owner->FacingIdeal() || (owner->AI_FORWARD && (owner->AI_AlertIndex >= ESuspicious)))
		{
			// grayman #3857 - shorter delay if not playing an anim for these reasons
			delay = static_cast<int>(_idleAnimationInterval*(0.2f + gameLocal.random.RandomFloat()*0.1f));
		}
		else
		{
			delay = static_cast<int>(_idleAnimationInterval*(0.8f + gameLocal.random.RandomFloat()*0.4f));
		}
		// Reset the timer
		_nextAnimationTime = gameLocal.time + delay;
	}

	return false; // not finished yet
}

void IdleAnimationTask::AttemptToPlayAnim(idAI* owner, const idStringList& anims, bool torsoOnly)
{
	// Get a new index into the given array
	int animIndex = GetNewIdleAnimIndex(anims, owner);

	if (animIndex != -1)
	{
		_lastIdleAnim = animIndex;

		// Issue the playanim call
		owner->SetNextIdleAnim(anims[animIndex]);

		// Play on TORSO and LEGS
		owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_CustomIdleAnim", 4);

		if (!torsoOnly)
		{
			// SteveL #4012: Use OverrideAnim instead of a matching "Legs_CustomIdleAnim", 
			// which invites race conditions and conflicts between game code and scripts.
			owner->SetAnimState( ANIMCHANNEL_LEGS, "Legs_Idle", 4 ); // Queue up the next state before sync'ing legs to torso
			owner->Event_SetBlendFrames( ANIMCHANNEL_LEGS, 10 ); // ~0.4 seconds.
			owner->PostEventMS( &AI_OverrideAnim, 0, ANIMCHANNEL_LEGS );
		}

		// grayman #3182 - Set one of two different wait states.
		// "idle" represents an idle animation that includes a
		// voice frame command, and "idle_no_voice" represents an
		// idle animation that doesn't include a voice frame command.
		if ( AnimHasVoiceFlag(owner, anims[animIndex]) )
		{
			owner->SetWaitState("idle");
		}
		else
		{
			owner->SetWaitState("idle_no_voice");
		}
	}
}

int IdleAnimationTask::GetNewIdleAnimIndex(const idStringList& anims, idAI* owner)
{
	int animCount = anims.Num();

	if (animCount > 1)
	{
		idList<int> excludedIndices;
		excludedIndices.Append(_lastIdleAnim);

		// Be sure to select one which doesn't interfere and is different to the last one
		while ( excludedIndices.Num() < animCount )
		{
			int animIdx = gameLocal.random.RandomInt(animCount);

			// If we already tried that anim, get a new one
			if ( excludedIndices.FindIndex(animIdx) != -1 )
			{
				continue;
			}

			// Check if anim is suitable at this point
			if ( !AnimIsApplicable(owner, anims[animIdx]) )
			{
				// Cannot play this one
				excludedIndices.Append(animIdx);
				continue;
			}

			// Found a good one
			return animIdx;
		}

		// Did not find a suitable one
		return -1;
	}
	else if (animCount == 1)
	{
		// Only one single anim present
		if (AnimIsApplicable(owner, anims[0]))
		{
			return 0; // anim is OK
		}
		
		// Anim not suitable
		return -1;
	}
	else
	{
		// No idle anims in list
		return -1;
	}
}

bool IdleAnimationTask::AnimIsApplicable(idAI* owner, const idStr& animName)
{
	int torsoAnimNum = owner->GetAnim(ANIMCHANNEL_TORSO, animName);

	if (torsoAnimNum == 0)
	{
		gameLocal.Warning("Could not find anim %s on entity %s", animName.c_str(), owner->name.c_str());
		DM_LOG(LC_AI, LT_ERROR)LOGSTRING("Could not find anim %s on entity %s\r", animName.c_str(), owner->name.c_str());

		return false;
	}

	// Check if this anim interferes with random head turning
	if ( owner->GetMemory().currentlyHeadTurning && AnimHasNoHeadTurnFlag(owner, torsoAnimNum) )
	{
//		gameLocal.Printf("Inhibited idle animation %s, since random head turning is active.\n", animName.c_str());

		// Cannot play this one at this point
		return false;
	}

	// grayman #3182 - Check if this anim plays a voice bark, which would interfere with ongoing voice barks
	if ( owner->GetMemory().currentlyBarking && AnimHasVoiceFlag(owner, animName) )
	{
//		gameLocal.Printf("Inhibited idle animation %s, since barking is active.\n", animName.c_str());

		// Cannot play this one at this point
		return false;
	}

	// OK
	return true; 
}

bool IdleAnimationTask::AnimHasNoHeadTurnFlag(idAI* owner, int animNum)
{
	idAnimator* animator = owner->GetAnimatorForChannel(ANIMCHANNEL_TORSO);
	animFlags_t animflags = animator->GetAnimFlags(animNum);

	return animflags.no_random_headturning;
}

// grayman #3182
bool IdleAnimationTask::AnimHasVoiceFlag(idAI* owner, const idStr& animName)
{
	int torsoAnimNum = owner->GetAnim(ANIMCHANNEL_TORSO, animName);

	if ( torsoAnimNum == 0 )
	{
		return false;
	}

	idAnimator* animator = owner->GetAnimatorForChannel(ANIMCHANNEL_TORSO);
	animFlags_t animflags = animator->GetAnimFlags(torsoAnimNum);

	return animflags.has_voice_fc;
}

void IdleAnimationTask::OnFinish(idAI* owner)
{
	if (!owner->AI_KNOCKEDOUT && (owner->health > 0) && (idStr(owner->WaitState()) == "")) // grayman #3857 - let current anim finish
	{
		owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Idle", 5);
		owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_Idle", 5);
		owner->SetWaitState("");
	}
}

// Save/Restore methods
void IdleAnimationTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);
	savefile->WriteInt(_nextAnimationTime);
	savefile->WriteInt(_idleAnimationInterval);

	savefile->WriteInt(_idleAnimations.Num());
	for (int i = 0; i < _idleAnimations.Num(); i++)
	{
		savefile->WriteString(_idleAnimations[i].c_str());
	}

	savefile->WriteInt(_idleAnimationsTorso.Num());
	for (int i = 0; i < _idleAnimationsTorso.Num(); i++)
	{
		savefile->WriteString(_idleAnimationsTorso[i].c_str());
	}

	savefile->WriteInt(_idleAnimationsSitting.Num());
	for (int i = 0; i < _idleAnimationsSitting.Num(); i++)
	{
		savefile->WriteString(_idleAnimationsSitting[i].c_str());
	}

	savefile->WriteInt(_lastIdleAnim);
}

void IdleAnimationTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);
	savefile->ReadInt(_nextAnimationTime);
	savefile->ReadInt(_idleAnimationInterval);

	int num;
	savefile->ReadInt(num);
	_idleAnimations.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		savefile->ReadString(_idleAnimations[i]);
	}

	savefile->ReadInt(num);
	_idleAnimationsTorso.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		savefile->ReadString(_idleAnimationsTorso[i]);
	}

	savefile->ReadInt(num);
	_idleAnimationsSitting.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		savefile->ReadString(_idleAnimationsSitting[i]);
	}

	savefile->ReadInt(_lastIdleAnim);
}

IdleAnimationTaskPtr IdleAnimationTask::CreateInstance()
{
	return IdleAnimationTaskPtr(new IdleAnimationTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar idleAnimationTaskRegistrar(
	TASK_IDLE_ANIMATION, // Task Name
	TaskLibrary::CreateInstanceFunc(&IdleAnimationTask::CreateInstance) // Instance creation callback
);

} // namespace ai
