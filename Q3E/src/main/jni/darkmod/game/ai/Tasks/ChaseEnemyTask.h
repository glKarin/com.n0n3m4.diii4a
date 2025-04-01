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

#ifndef __AI_CHASE_ENEMY_TASK_H__
#define __AI_CHASE_ENEMY_TASK_H__

#include "Task.h"
#include "../../MultiStateMover.h"

namespace ai
{

// Define the name of this task
#define TASK_CHASE_ENEMY "ChaseEnemy"

class ChaseEnemyTask;
typedef std::shared_ptr<ChaseEnemyTask> ChaseEnemyTaskPtr;

class ChaseEnemyTask :
	public Task
{
private:
	idEntityPtr<idActor> _enemy;
	int _reachEnemyCheck;

	// Private default constructor
	ChaseEnemyTask();
public:
	ChaseEnemyTask(idActor* enemy);

	// Get the name of this task
	virtual const idStr& GetName() const override;

	virtual void Init(idAI* owner, Subsystem& subsystem) override;

	virtual bool Perform(Subsystem& subsystem) override;

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;

	// Creates a new Instance of this task
	static ChaseEnemyTaskPtr CreateInstance();

	// Class-specific methods
	virtual void SetEnemy(idActor* enemy);

private:
	CMultiStateMoverPosition* CanFetchElevator(CMultiStateMover* mover, idAI* owner);
};

} // namespace ai

#endif /* __AI_CHASE_ENEMY_TASK_H__ */
