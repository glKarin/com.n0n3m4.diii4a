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

#ifndef __AI_SINGLE_BARK_TASK_H__
#define __AI_SINGLE_BARK_TASK_H__

#include "CommunicationTask.h"
#include "../../AIComm_Message.h"

namespace ai
{

// Define the name of this task
#define TASK_SINGLE_BARK "SingleBark"

class SingleBarkTask;
typedef std::shared_ptr<SingleBarkTask> SingleBarkTaskPtr;

class SingleBarkTask :
	public CommunicationTask
{
protected:
	int _startDelay;

	bool _allowDuringAnim; // grayman #3182

	int _endTime;

	// The message which should be delivered when barking
	CommMessagePtr _message;

	// Default constructor
	SingleBarkTask();

public:
	// Constructor taking a sound name as argument.
	// Optional arguments are the message to deliver
	// and the time to pass in ms before the bark should be played.
	// grayman #3182 - add another optional arg, whether this
	// bark can be played during an idle animation.
	SingleBarkTask(const idStr& soundName, 
				   const CommMessagePtr& message = CommMessagePtr(),
				   int startDelayMS = 0,
				   bool allowDuringAnim = true);

	// Get the name of this task
	virtual const idStr& GetName() const override;

	virtual void Init(idAI* owner, Subsystem& subsystem) override;
	virtual bool Perform(Subsystem& subsystem) override;
	virtual void OnFinish(idAI* owner) override; // grayman #3182

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;

	// Creates a new Instance of this task
	static SingleBarkTaskPtr CreateInstance();

	// Class-specific methods
	virtual void SetSound(const idStr& soundName);
	virtual void SetMessage(const CommMessagePtr& message);
};

} // namespace ai

#endif /* __AI_SINGLE_BARK_TASK_H__ */
