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

#ifndef __AI_COMMUNICATION_SUBSYSTEM_H__
#define __AI_COMMUNICATION_SUBSYSTEM_H__

#include <list>

#include "Tasks/CommunicationTask.h"
#include "Subsystem.h"
#include "Queue.h"

namespace ai
{

class CommunicationSubsystem : 
	public Subsystem 
{
protected:

	enum EActionTypeOnConflict
	{
		EDefault,	// default behaviour
		EOverride,	// override the existing sound
		EQueue,		// queue after the current sound
		EDiscard,	// discard the new sound
		EPush,		// push on top of existing sound
	};

public:
	CommunicationSubsystem(SubsystemId subsystemId, idAI* owner);

	/**
	 * greebo: Handle a new incoming communication task and decide whether
	 * to push or queue it or do whatever action is defined according to the
	 * settings in the entityDef. 
	 *
	 * Note: Does not accept NULL pointers.
	 *
	 * @returns: TRUE if the new bark has been accepted, FALSE if it has been ignored.
	 */
	bool AddCommTask(const CommunicationTaskPtr& communicationTask);

	// returns the priority of the currently active communication task
	int GetCurrentPriority();

	/**
	 * greebo: Queues a silence task at the end of the queue. The task
	 * gets the same priority assigned as the last one of the queue.
	 */
	void AddSilence(int duration);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;

	// Returns some debug text for console or renderworld display
	virtual idStr GetDebugInfo() override;

protected:
	// Priority difference is "new snd prio - current snd prio"
	EActionTypeOnConflict GetActionTypeForSound(const CommunicationTaskPtr& communicationTask);

	// Returns the currently active commtask or NULL if no commtask is active
	CommunicationTaskPtr GetCurrentCommTask();
};
typedef std::shared_ptr<CommunicationSubsystem> CommunicationSubsystemPtr;

} // namespace ai

#endif /* __AI_COMMUNICATION_SUBSYSTEM_H__ */
