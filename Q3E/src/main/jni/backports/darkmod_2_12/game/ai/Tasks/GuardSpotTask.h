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

#ifndef __AI_GUARD_SPOT_TASK_H__
#define __AI_GUARD_SPOT_TASK_H__

#include "Task.h"

namespace ai
{

/**
* greebo: This task requires memory.currentSearchSpot to be valid.
* 
* This task is intended to be pushed into the action Subsystem and
* performs single-handedly how the given hiding spot should be handled.
*
* Note: This Task employs the Movement Subsystem when the algorithm
* judges to walk/run over to the given search spot.
**/

// Define the name of this task
#define TASK_GUARD_SPOT "GuardSpot"

class GuardSpotTask;
typedef std::shared_ptr<GuardSpotTask> GuardSpotTaskPtr;

class GuardSpotTask :
	public Task
{
private:

	enum EGuardSpotState
	{
		EStateSetup,
		EStateMoving,
		//EStateStartIdleSearchAnims,
		EStateStanding
	} _guardSpotState;

	// The spot to guard
	idVec3 _guardSpot;

	// The time this task may exit
	int _exitTime;

	int _giveOrderTime; // time to give order to guard, if one is needed

	// Is milling is the only thing we'll be doing?
	bool _millingOnly;

	// The next time the guard should turn
	int _nextTurnTime;

	// Direction you face when you arrive at the spot
	float _baseYaw;

	// Private default constructor
	GuardSpotTask();
public:
	// Get the name of this task
	virtual const idStr& GetName() const override;

	virtual void Init(idAI* owner, Subsystem& subsystem) override;

	virtual bool Perform(Subsystem& subsystem) override;

	/** 
	 * greebo: Sets a new goal position for this task.
	 *
	 * @newPos: The new position
	 */
	virtual void SetNewGoal(const idVec3& newPos);

	virtual void OnFinish(idAI* owner) override; // grayman #2560

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;

	// Creates a new Instance of this task
	static GuardSpotTaskPtr CreateInstance();
};

} // namespace ai

#endif /* __AI_GUARD_SPOT_TASK_H__ */
