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

#ifndef __AI_PATH_LOOKAT_TASK_H__
#define __AI_PATH_LOOKAT_TASK_H__

#include "PathTask.h"

namespace ai
{

// Define the name of this task
#define TASK_PATH_LOOKAT "PathLookat"

class PathLookatTask;
typedef std::shared_ptr<PathLookatTask> PathLookatTaskPtr;

class PathLookatTask :
	public PathTask
{
private:
	PathLookatTask();

	idEntity* _focusEnt;
	float _duration;

public:
	PathLookatTask(idPathCorner* path);

	// Get the name of this task
	virtual const idStr& GetName() const override;

	virtual void Init(idAI* owner, Subsystem& subsystem) override;

	virtual bool Perform(Subsystem& subsystem) override;

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;

	// Creates a new Instance of this task
	static PathLookatTaskPtr CreateInstance();
};

} // namespace ai

#endif /* __AI_PATH_LOOKAT_TASK_H__ */
