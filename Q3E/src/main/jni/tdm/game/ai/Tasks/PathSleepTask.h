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

#ifndef __AI_PATH_SLEEP_TASK_H__
#define __AI_PATH_SLEEP_TASK_H__

#include "PathTask.h"

// grayman #3820 - These must be kept in sync with their counterparts in AI.h
// and tdm_ai.script.
// The sleep_location spawnarg can be used both on path_sleep entities and
// on the AI entities. The former has priority over the latter
#define SLEEP_LOC_UNDEFINED	-1
#define SLEEP_LOC_FLOOR		 0
#define SLEEP_LOC_BED		 1
#define SLEEP_LOC_CHAIR		 2

namespace ai
{

// Define the name of this task
#define TASK_PATH_SLEEP "PathSleep"

class PathSleepTask;
typedef std::shared_ptr<PathSleepTask> PathSleepTaskPtr;

class PathSleepTask :
	public PathTask
{
private:
	// Private constructor
	PathSleepTask();

public:
	PathSleepTask(idPathCorner* path);

	// Get the name of this task
	virtual const idStr& GetName() const override;

	virtual void Init(idAI* owner, Subsystem& subsystem) override;

	virtual bool Perform(Subsystem& subsystem) override;

	virtual void OnFinish(idAI* owner) override; // grayman #3528

	// Creates a new Instance of this task
	static PathSleepTaskPtr CreateInstance();

};

} // namespace ai

#endif /* __AI_PATH_SLEEP_TASK_H__ */
