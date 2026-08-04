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

#ifndef __AI_GREETING_BARK_TASK_H__
#define __AI_GREETING_BARK_TASK_H__

#include "SingleBarkTask.h"
#include "../../AIComm_Message.h"

namespace ai
{

// Define the name of this task
#define TASK_GREETING_BARK "GreetingBark"

class GreetingBarkTask;
typedef std::shared_ptr<GreetingBarkTask> GreetingBarkTaskPtr;

class GreetingBarkTask :
	public SingleBarkTask
{
protected:

	idActor* _greetingTarget;
	bool	 _isInitialGreeting; // grayman #3415 - true when this AI initiates the greeting,
								 // false when this AI is responding to the greeting

	// Default constructor
	GreetingBarkTask();

public:
	// Constructor taking a sound name and the target actor as argument
	GreetingBarkTask(const idStr& soundName, idActor* greetingTarget, bool isInitialGreeting); // grayman #3415

	// Get the name of this task
	virtual const idStr& GetName() const override;

	virtual void Init(idAI* owner, Subsystem& subsystem) override;
	virtual bool Perform(Subsystem& subsystem) override;

	virtual void OnFinish(idAI* owner) override;

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;

	// Creates a new Instance of this task
	static GreetingBarkTaskPtr CreateInstance();
};

} // namespace ai

#endif /* __AI_GREETING_BARK_TASK_H__ */
