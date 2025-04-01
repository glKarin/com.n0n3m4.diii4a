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

#ifndef __AI_RESOLVE_MOVEMENT_BLOCK_TASK_H__
#define __AI_RESOLVE_MOVEMENT_BLOCK_TASK_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_RESOLVE_MOVEMENT_BLOCK "ResolveMovementBlock"
#define RESOLVE_MOVE_DIST 16 // grayman #2345

class ResolveMovementBlockTask;
typedef std::shared_ptr<ResolveMovementBlockTask> ResolveMovementBlockTaskPtr;

class ResolveMovementBlockTask :
	public Task
{
private:
	// The entity in the way
	idEntity* _blockingEnt;

	// The angles we had when starting this task
	idAngles _initialAngles;

	int _preTaskContents;

	bool _turning; // grayman #3725

	int _endTime;

	// Default constructor
	ResolveMovementBlockTask();

public:
	ResolveMovementBlockTask(idEntity* blockingEnt);

	// Get the name of this task
	virtual const idStr& GetName() const override;

	virtual void Init(idAI* owner, Subsystem& subsystem) override;

	virtual bool Perform(Subsystem& subsystem) override;

	virtual void OnFinish(idAI* owner) override;

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;

	// Creates a new Instance of this task
	static ResolveMovementBlockTaskPtr CreateInstance();

private:
	void InitBlockingAI(idAI* owner, Subsystem& subsystem);
	void InitBlockingStatic(idAI* owner, Subsystem& subsystem);

	bool PerformBlockingAI(idAI* owner);
	bool PerformBlockingStatic(idAI* owner);
	bool Room2Pass(idAI* owner);	// grayman #2345
	//bool IsSolid();			// grayman #2345 - grayman #4238 - not used
	void BecomeNonSolid(idAI* owner); // grayman #2345
};

} // namespace ai

#endif /* __AI_RESOLVE_MOVEMENT_BLOCK_TASK_H__ */
