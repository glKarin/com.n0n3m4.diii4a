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

#ifndef __AI_PLAY_ANIMATION_TASK_H__
#define __AI_PLAY_ANIMATION_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_PLAY_ANIMATION "PlayAnimation"

class PlayAnimationTask;
typedef std::shared_ptr<PlayAnimationTask> PlayAnimationTaskPtr;

class PlayAnimationTask :
	public Task
{
private:
	idStr _animName;

	int _blendFrames;

	bool _playCycle;

	// Private constructor
	PlayAnimationTask();

public:
	// Pass the animation name directly here (like "idle_armwipe")
	// Set playCycle to TRUE if this animation task should play continuously
	PlayAnimationTask(const idStr& animName, int blendFrames, bool playCycle = false);

	// Get the name of this task
	virtual const idStr& GetName() const override;

	virtual void Init(idAI* owner, Subsystem& subsystem) override;

	virtual bool Perform(Subsystem& subsystem) override;

	virtual void OnFinish(idAI* owner) override;

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;

	// Creates a new Instance of this task
	static PlayAnimationTaskPtr CreateInstance();

private:
	// Private helper
	void StartAnim(idAI* owner);
};

} // namespace ai

#endif /* __AI_PLAY_ANIMATION_TASK_H__ */
