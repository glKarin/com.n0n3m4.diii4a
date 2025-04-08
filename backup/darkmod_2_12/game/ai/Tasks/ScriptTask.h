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

#ifndef __AI_SCRIPT_TASK_H__
#define __AI_SCRIPT_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_SCRIPT "ScriptTask"

class ScriptTask;
typedef std::shared_ptr<ScriptTask> ScriptTaskPtr;

/** 
 * greebo: A ScriptTask can be plugged into any AI subsystem and
 * has the only purpose to execute a script thread. The actual
 * script function must be specified in the constructor and
 * can bei either global or local (=defined on the AI's scriptobject).
 *
 * For local scripts, the function does not to take any arguments.
 * For global scripts, the function needs to take an entity argument (=this owner).
 *
 * The Task is finishing itself when the script thread is done executing.
 *
 * Terminating this task will of course kill the thread.
 */
class ScriptTask :
	public Task
{
private:
	// The name of the script function to execute
	idStr _functionName;	

	// The script thread
	idThread* _thread;

	// Private constructor
	ScriptTask();

public:
	~ScriptTask();

	// Optional constructor taking the function name
	ScriptTask(const idStr& functionName);

	// Get the name of this task
	virtual const idStr& GetName() const override;

	virtual void Init(idAI* owner, Subsystem& subsystem) override;

	virtual bool Perform(Subsystem& subsystem) override;

	// Gets called when this task is finished (or gets terminated)
	virtual void OnFinish(idAI* owner) override;

	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;

	// Creates a new Instance of this task
	static ScriptTaskPtr CreateInstance();
};

} // namespace ai

#endif /* __AI_SCRIPT_TASK_H__ */
