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

#ifndef __AI_COMMUNICATION_TASK_H__
#define __AI_COMMUNICATION_TASK_H__

#include "Task.h"

namespace ai
{

#define BARK_PRIORITY_DEF "atdm:ai_bark_priority"
#define VERY_HIGH_BARK_PRIORITY 9000000

class CommunicationTask;
typedef std::shared_ptr<CommunicationTask> CommunicationTaskPtr;

/** 
 * A CommunicationTask is a more specialised task, extending
 * the interface by a few methods, which facilitate the handling
 * of concurrent AI barks. A bark sound always has an associated 
 * priority, which the CommunicationSubsystem is using to decide
 * whether a new incoming bark is allowed to override an existing
 * one or not.
 */
class CommunicationTask :
	public Task
{
protected:
	// The sound to play
	idStr _soundName;

	// The priority of this sound
	int _priority;

	int _barkStartTime;
	int _barkLength;

	// Private constructors
	CommunicationTask();

	CommunicationTask(const idStr& soundName);

public:
	// Returns the priority of this bark
	int GetPriority();
	void SetPriority(int priority);

	// Returns TRUE if the task is still playing a bark 
	// (this is excluding any possible delay after the actual sound)
	bool IsBarking();

	const idStr& GetSoundName();

	virtual void Init(idAI* owner, Subsystem& subsystem) override;

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;
};

} // namespace ai

#endif /* __AI_COMMUNICATION_TASK_H__ */
