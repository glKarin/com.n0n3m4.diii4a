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

#ifndef __AI_COMBAT_TASK_H__
#define __AI_COMBAT_TASK_H__

#include "Task.h"

namespace ai
{

class CombatTask :
	public Task
{
protected:
	idEntityPtr<idActor> _enemy;

	int _lastCombatBarkTime;
	int _nextAttackTime; // grayman #4412

	CombatTask();

public:
	virtual void Init(idAI* owner, Subsystem& subsystem) override;

	//  This task lacks a Perform() method, this is to be implemented by subclasses

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;

protected:

	// Emits a combat bark plus an AI message to be delivered by soundprop
	// about the enemy's position
	void EmitCombatBark(idAI* owner, const idStr& sndName);
};
typedef std::shared_ptr<CombatTask> CombatTaskPtr;

} // namespace ai

#endif /* __AI_RANGED_COMBAT_TASK_H__ */
