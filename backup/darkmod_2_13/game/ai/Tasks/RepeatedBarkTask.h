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

#ifndef __AI_REPEATED_BARK_TASK_H__
#define __AI_REPEATED_BARK_TASK_H__

#include "CommunicationTask.h"
#include "../../AIComm_Message.h"

namespace ai
{

// Define the name of this task
#define TASK_REPEATED_BARK "RepeatedBark"

class RepeatedBarkTask;
typedef std::shared_ptr<RepeatedBarkTask> RepeatedBarkTaskPtr;

class RepeatedBarkTask :
	public CommunicationTask
{
private:
	// times in milliseconds:
	int _barkRepeatIntervalMin;
	int _barkRepeatIntervalMax;
	int _nextBarkTime;
	bool _prevBarkDone; // grayman #3182

	// The message which should be delivered when barking
	CommMessagePtr _message;

	// Default Constructor
	RepeatedBarkTask();

public:
	/**
	 * greebo: Pass the sound shader name plus the interval range in milliseconds.
	 * The message argument is optional and can be used to let this Task emit messages
	 * when playing the sound.
	 */
	RepeatedBarkTask(const idStr& soundName, 
					 int barkRepeatIntervalMin, int barkRepeatIntervalMax, 
					 const CommMessagePtr& message = CommMessagePtr());

	// Get the name of this task
	virtual const idStr& GetName() const override;

	virtual void Init(idAI* owner, Subsystem& subsystem) override;

	virtual bool Perform(Subsystem& subsystem) override;

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;

	// Creates a new Instance of this task
	static RepeatedBarkTaskPtr CreateInstance();
};

} // namespace ai

#endif /* __AI_REPEATED_BARK_TASK_H__ */
