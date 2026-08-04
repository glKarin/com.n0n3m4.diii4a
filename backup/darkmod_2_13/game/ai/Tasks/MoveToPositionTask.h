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

#ifndef __AI_MOVE_TO_POSITION_H__
#define __AI_MOVE_TO_POSITION_H__

#include "Task.h"

namespace ai
{

// Define the name of this task
#define TASK_MOVE_TO_POSITION "MoveToPosition"

#define DEFAULT_ENTITY_REACH_DISTANCE 50.0f

class MoveToPositionTask;
typedef std::shared_ptr<MoveToPositionTask> MoveToPositionTaskPtr;

class MoveToPositionTask :
	public Task
{
private:

	// The target position
	idVec3 _targetPosition;

	// The previous target position
	idVec3 _prevTargetPosition;

	// Target yaw (is not INFINITY if set)
	float _targetYaw;

	idEntity* _targetEntity;
	
	// the distance below which entities are considered "reached"
	float _entityReachDistance;

	// angua: if > 0, this changes the size of the bounding box used for checking whether the AI has reached their destination
	float _accuracy;

	// Default constructor
	MoveToPositionTask();

public:
	// Constructor taking the target position (and optional target yaw) as input argument
	MoveToPositionTask(const idVec3& targetPosition, float targetYaw = idMath::INFINITY, float accuracy = -1);

	// Constructor taking a target entity
	MoveToPositionTask(idEntity* targetEntity, float entityReachDistance = DEFAULT_ENTITY_REACH_DISTANCE, float accuracy = -1);

	// Get the name of this task
	virtual const idStr& GetName() const override;

	virtual void Init(idAI* owner, Subsystem& subsystem) override;

	virtual bool Perform(Subsystem& subsystem) override;

	void SetPosition(idVec3 targetPosition);

	// Sets the distance below which entities are considered "reached"
	void SetEntityReachDistance(float distance);

	// Save/Restore methods
	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;

	// Creates a new Instance of this task
	static MoveToPositionTaskPtr CreateInstance();

private:
	// Refines the goal position, if appropriate
	void UpdateTargetPosition(idAI* owner);
};

} // namespace ai

#endif /* __AI_MOVE_TO_POSITION_H__ */
