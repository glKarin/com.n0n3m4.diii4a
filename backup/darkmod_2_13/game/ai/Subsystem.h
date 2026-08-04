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

#ifndef __AI_SUBSYSTEM_H__
#define __AI_SUBSYSTEM_H__

#include <list>

#include "Tasks/Task.h"
#include "Queue.h"

namespace ai
{

enum SubsystemId {
	SubsysSenses = 0,
	SubsysMovement,
	SubsysCommunication,
	SubsysAction,
	SubsysSearch, // grayman #3857
	SubsystemCount,
};

class Subsystem
{
protected:
	// The subsystem knows what type it is
	SubsystemId _id;

	idEntityPtr<idAI> _owner;

	// The stack of tasks, pushed tasks get added at the end
	TaskQueue _taskQueue;

	// A temporary container, gets cleared in each PerformTask() call.
	TaskQueue _recycleBin;

	// Is TRUE during task switching, this will call Task::Init() in PerformTask().
	bool _initTask;

	// TRUE if this subsystem is performing, default is ON
	bool _enabled;

public:
	Subsystem(SubsystemId subsystemId, idAI* owner);

	// Returns the currently active task
	TaskPtr GetCurrentTask() const;

	// Returns the name of the current task ("" if empty)
	idStr GetCurrentTaskName() const;

	// Called regularly by the Mind to run the currently assigned routine.
	// @returns: TRUE if the subsystem is enabled and the task was performed, 
	// @returns: FALSE if the subsystem is disabled and nothing happened.
	virtual bool PerformTask();

	/**
	 * greebo: Switches to the named Task. The currently active
	 *         Task will be pushed back in the queue and is postponed 
	 *         until the newly added Task is finished.
	 */
	virtual void PushTask(const TaskPtr& newTask);

	/**
	 * greebo: Ends the current task - the Subsystem will pick the next Task
	 *         from the TaskQueue or disable itself if none is available.
	 *
	 * @returns: TRUE if there are still Tasks in the queue after this one is finished.
	 */
	virtual bool FinishTask();

	/**
	 * greebo: Sets the new task of this subsystem.
	 * This new task REPLACES the currently active one.
	 */
	virtual void SwitchTask(const TaskPtr& newTask);

	/**
	 * greebo: Adds the named Task at the end of the queue. It becomes active when
	 *         all other currently enqueued Tasks will be finished.
	 */
	virtual void QueueTask(const TaskPtr& task);

	/**
	 * greebo: Removes all Tasks from the Queue and disables this subsystem.
	 */
	virtual void ClearTasks();

	//virtual void PrintTaskQueue();

	virtual void FinishDoorHandlingTask(idAI *owner); // grayman #4030

	// Enables/disables this subsystem
	virtual void Enable();
	virtual void Disable();

	// Returns TRUE if this subsystem is performing.
	virtual bool IsEnabled() const;

	// Returns TRUE if there are no tasks left to perform 
	virtual bool IsEmpty() const;

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const;
	virtual void Restore(idRestoreGame* savefile);

	// Returns some debug text for console or renderworld display
	virtual idStr GetDebugInfo();
};
typedef std::shared_ptr<Subsystem> SubsystemPtr;

} // namespace ai

#endif /* __AI_SUBSYSTEM_H__ */
