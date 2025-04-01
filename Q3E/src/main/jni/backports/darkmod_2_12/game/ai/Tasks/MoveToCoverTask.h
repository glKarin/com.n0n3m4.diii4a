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

#ifndef __AI_MOVE_TO_COVER_TASK_H__
#define __AI_MOVE_TO_COVER_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_MOVE_TO_COVER "MoveToCover"

class MoveToCoverTask;
typedef std::shared_ptr<MoveToCoverTask> MoveToCoverTaskPtr;

class MoveToCoverTask :
	public Task
{
public:
	// Get the name of this task
	virtual const idStr& GetName() const override;

	virtual void Init(idAI* owner, Subsystem& subsystem) override;

	virtual bool Perform(Subsystem& subsystem) override;

	// Creates a new Instance of this task
	static MoveToCoverTaskPtr CreateInstance();
};

} // namespace ai

#endif /* __AI_MOVE_TO_COVER_TASK_H__ */
