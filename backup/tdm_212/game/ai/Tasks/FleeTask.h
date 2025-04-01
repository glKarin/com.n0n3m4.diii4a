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

#ifndef __AI_FLEE_TASK_H__
#define __AI_FLEE_TASK_H__

#include "Task.h"
#include "../../EscapePointManager.h"

namespace ai
{

// Define the name of this task
#define TASK_FLEE "FleeTask"

// grayman #3548 - distance values
#define FLEE_DIST_MAX	1000 // stop fleeing when at least this far away from the threat
#define FLEE_DIST_MIN_MELEE	 300 // initial distance to put between you and a melee threat
#define FLEE_DIST_MIN_RANGED 500 // initial distance to put between you and a ranged threat
#define FLEE_DIST_DELTA	 300 // incremental distance to put between you and the threat

class FleeTask;
typedef std::shared_ptr<FleeTask> FleeTaskPtr;

class FleeTask :
	public Task
{
private:
	idEntityPtr<idActor> _enemy;
	int _escapeSearchLevel;
	int _failureCount;
	int _fleeStartTime;
	EscapeDistanceOption _distOpt;
	int _currentDistanceGoal; // grayman #3548
	bool _haveTurnedBack; // grayman #3548

	FleeTask();
public:
	// Get the name of this task
	virtual const idStr& GetName() const override;

	virtual void Init(idAI* owner, Subsystem& subsystem) override;

	virtual bool Perform(Subsystem& subsystem) override;

	virtual void OnFinish(idAI* owner) override; // grayman #3548

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;

	// Creates a new Instance of this task
	static FleeTaskPtr CreateInstance();

};

} // namespace ai

#endif /* __AI_FLEE_TASK_H__ */
