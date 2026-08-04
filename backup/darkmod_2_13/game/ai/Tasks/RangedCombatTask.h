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

#ifndef __AI_RANGED_COMBAT_TASK_H__
#define __AI_RANGED_COMBAT_TASK_H__

#include "CombatTask.h"

namespace ai
{

// Define the name of this task
#define TASK_RANGED_COMBAT "RangedCombat"

class RangedCombatTask;
typedef std::shared_ptr<RangedCombatTask> RangedCombatTaskPtr;

class RangedCombatTask :
	public CombatTask
{
	//int _lastCombatBarkTime; // grayman #4394
public:
	// Get the name of this task
	virtual const idStr& GetName() const override;

	virtual void Init(idAI* owner, Subsystem& subsystem) override;

	virtual bool Perform(Subsystem& subsystem) override;

	// Creates a new Instance of this task
	static RangedCombatTaskPtr CreateInstance();

	virtual void OnFinish(idAI* owner) override;

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;
};

} // namespace ai

#endif /* __AI_RANGED_COMBAT_TASK_H__ */
