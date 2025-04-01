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

#ifndef __AI_WAIT_TASK_H__
#define __AI_WAIT_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_WAIT "Wait"

class WaitTask;
typedef std::shared_ptr<WaitTask> WaitTaskPtr;

class WaitTask :
	public Task
{
private:

	int _waitTime;
	int _waitEndTime;

	// Default constructor
	WaitTask();

public:

	// Constructor with waittime (in ms) as input argument
	WaitTask(const int waitTime);

	// Get the name of this task
	virtual const idStr& GetName() const override;

	virtual void Init(idAI* owner, Subsystem& subsystem) override;

	virtual bool Perform(Subsystem& subsystem) override;

	void SetTime(int waitTime);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;

	// Creates a new Instance of this task
	static WaitTaskPtr CreateInstance();
};

} // namespace ai

#endif /* __AI_WAIT_TASK_H__ */
