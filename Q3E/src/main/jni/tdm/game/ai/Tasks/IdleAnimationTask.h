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

#ifndef __AI_IDLE_ANIMATION_TASK_H__
#define __AI_IDLE_ANIMATION_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_IDLE_ANIMATION "IdleAnimation"

class IdleAnimationTask;
typedef std::shared_ptr<IdleAnimationTask> IdleAnimationTaskPtr;

class IdleAnimationTask :
	public Task
{
private:
	int _nextAnimationTime;

	idStringList _idleAnimations;
	idStringList _idleAnimationsTorso;
	idStringList _idleAnimationsSitting;

	int _idleAnimationInterval;

	// The index of the last anim played (to avoid duplicates)
	int _lastIdleAnim;

	// Default constructor is private
	IdleAnimationTask();
public:
	// Get the name of this task
	virtual const idStr& GetName() const override;

	virtual void Init(idAI* owner, Subsystem& subsystem) override;

	virtual bool Perform(Subsystem& subsystem) override;

	virtual void OnFinish(idAI* owner) override;

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;

	// Creates a new Instance of this task
	static IdleAnimationTaskPtr CreateInstance();

protected:
	// De-serialises the comma-separated string list of animations
	void ParseAnimsToList(const std::string& animStringList, idStringList& targetList);

	// Attempt to play an animation from the given list. Set torsoOnly to true if legs channel is forbidden
	void AttemptToPlayAnim(idAI* owner, const idStringList& anims, bool torsoOnly);

	// Returns TRUE if the given anim has no_random_head_turning set 
	bool AnimHasNoHeadTurnFlag(idAI* owner, int animNum);

	// grayman #3182 - Returns TRUE if the given anim has has_voice_fc set (has a voice frame command)
	bool AnimHasVoiceFlag(idAI* owner, const idStr& animName);

	// Returns a new idle anim index
	virtual int GetNewIdleAnimIndex(const idStringList& anims, idAI* owner);

	// Returns true if the named anim is ok at this point
	virtual bool AnimIsApplicable(idAI* owner, const idStr& animName);
};

} // namespace ai

#endif /* __AI_IDLE_ANIMATION_TASK_H__ */
