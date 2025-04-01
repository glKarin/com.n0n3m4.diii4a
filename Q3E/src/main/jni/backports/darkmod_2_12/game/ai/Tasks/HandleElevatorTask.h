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

#ifndef __AI_HANDLE_ELEVATOR_TASK_H__
#define __AI_HANDLE_ELEVATOR_TASK_H__

#include "Task.h"
#include "../EAS/RouteInfo.h"
#include "../../BinaryFrobMover.h"

namespace ai
{

// Define the name of this task
#define TASK_HANDLE_ELEVATOR "HandleElevator"

class HandleElevatorTask;
typedef std::shared_ptr<HandleElevatorTask> HandleElevatorTaskPtr;

class HandleElevatorTask :
	public Task
{
private:
	enum State
	{
		EMovingTowardsStation,
		EInitiateMoveToFetchButton,
		EMovingToFetchButton,
		EPressFetchButton,
		EWaitForElevator,
		EMoveOntoElevator,
		EPause, // grayman #3050
		EInitiateMoveToRideButton,
		EMovingToRideButton,
		EPressRideButton,
		ERideOnElevator,
		EGetOffElevator,
		EWrapUp, // grayman #3050
		ENumStates,
	} _state;

	int _waitEndTime;

	// The actual route info structure
	eas::RouteInfo _routeInfo;

	// Is TRUE if this task has finished successfully
	bool _success;

	// grayman #3050 - max elevator riders
	int _maxRiders;
	
	// Private constructor
	HandleElevatorTask();
public:

	HandleElevatorTask(const eas::RouteInfoPtr& routeInfo);
	// Get the name of this task
	virtual const idStr& GetName() const override;

	virtual void Init(idAI* owner, Subsystem& subsystem) override;

	virtual bool Perform(Subsystem& subsystem) override;

	virtual void OnFinish(idAI* owner) override;

	void ReorderQueue(UserManager &um, idVec3 point); // grayman #3050

	virtual bool CanAbort() override; // grayman #3050

#if 0 // grayman - for debugging an elevator queue
	void PrintElevQueue(CMultiStateMover* elevator);
#endif

	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;

	// Creates a new Instance of this task
	static HandleElevatorTaskPtr CreateInstance();

private:
	// Checks if the elevator station is reachable, returns TRUE if this is the case
	bool IsElevatorStationReachable(CMultiStateMoverPosition* pos);

	void DebugDraw(idAI* owner, CMultiStateMoverPosition* pos, CMultiStateMover* elevator);

	// Lets the AI move towards the position entity (is slightly more complicated than just idAI::MoveToPos)
	bool MoveToPositionEntity(idAI* owner, CMultiStateMoverPosition* pos);

	// Lets the Ai move to the button
	bool MoveToButton(idAI* owner, CMultiStateMoverButton* button);

};

} // namespace ai

#endif /* __AI_HANDLE_ELEVATOR_TASK_H__ */
