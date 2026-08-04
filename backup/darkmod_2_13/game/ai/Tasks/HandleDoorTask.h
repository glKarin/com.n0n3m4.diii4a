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

#ifndef __AI_HANDLE_DOOR_TASK_H__
#define __AI_HANDLE_DOOR_TASK_H__

#include "Task.h"

#include "../../BinaryFrobMover.h"

namespace ai
{

// Define the name of this task
#define TASK_HANDLE_DOOR "HandleDoor"

#define HANDLE_DOOR_ACCURACY 12	// grayman #2345 - More accuracy when reaching position to work w/door.
								// '8' says to use a 16x16 bounding box to see if you've reached a goal
								// position instead of the default 32x32 ('-1').
								// grayman #2706 - returned to default value of -1. Tighter than this
								// might cause problems.
								// grayman #3786 - -1 leads to swinging doors bumping AI. Try 12.
#define HANDLE_DOOR_ACCURACY_RUNNING 24  // grayman #3317 - less accuracy when moving faster

class HandleDoorTask;
typedef std::shared_ptr<HandleDoorTask> HandleDoorTaskPtr;

class HandleDoorTask :
	public Task
{
private:
	idVec3 _frontPos;
	idVec3 _backPos;
	idVec3 _midPos;		// grayman #2345
	idVec3 _safePos;	// grayman #2345

	idEntityPtr<idEntity> _frontPosEnt; // front door controller
	idEntityPtr<idEntity> _backPosEnt;  // back door controller

	idEntityPtr<idEntity> _frontDHPosition; // front door handle position
	idEntityPtr<idEntity> _backDHPosition;  // back door handle position

	enum EDoorHandlingState
	{
		EStateNone,
		EStateApproachingDoor,
		EStateMovingToSafePos1, // grayman #2345
		EStateMovingToSafePos2, // grayman #3643
		EStateMovingToFrontPos,
		EStateRetryInterruptedOpen1, // grayman #3523
		EStateRetryInterruptedOpen2, // grayman #3523
		EStateRetryInterruptedOpen3, // grayman #3523
		EStateWaitBeforeOpen,
		EStateStartOpen,
		EStateOpeningDoor,
		EStateMovingToMidPos, // grayman #2345
		EStateMovingToBackPos,
		EStateRetryInterruptedClose1, // grayman #3523
		EStateRetryInterruptedClose2, // grayman #3523
		EStateRetryInterruptedClose3, // grayman #3523
		EStateRetryInterruptedClose4, // grayman #3726
		EStateRetryInterruptedClose5, // grayman #3726
		EStateRetryInterruptedClose6, // grayman #3726
		EStateWaitBeforeClose,
		EStateStartClose,
		EStateClosingDoor
	} _doorHandlingState;

	int		_waitEndTime;
	bool	_wasLocked;
	bool	_doorInTheWay;
	int		_retryCount;
	int		_leaveQueue;			// grayman #2345
	int		_leaveDoor;				// grayman #2700
	bool	_triedFitting;			// grayman #2345
	bool	_canHandleDoor;			// grayman #2712
	bool	_closeFromSameSide;		// grayman #2866
//	int		_blockedDoorCount;		// grayman #3523; grayman #4830
	int		_useDelay;				// grayman #3755 - delay during hand anim before activating door or controller
	bool	_rotates;				// grayman #3643 - true if this is a rotating door (false == sliding door)
	int		_doorSide;				// grayman #3643 - which side of door are we on? (0 or 1)

public:
	// Get the name of this task
	virtual const idStr& GetName() const override;

	virtual void Init(idAI* owner, Subsystem& subsystem) override;

	virtual bool Perform(Subsystem& subsystem) override;

	virtual void OnFinish(idAI* owner) override;

	virtual bool CanAbort() override; // grayman #2706

	void MoveToSafePosition(CFrobDoor* door); // grayman #3390

	void PickWhere2Go(CFrobDoor* door); // grayman #2345

	void DoorInTheWay(idAI* owner, CFrobDoor* frobDoor);

	// these check whether the AI is allowed to open/close/unlock/lock the door from this side
	bool AllowedToOpen(idAI* owner);
	bool AllowedToClose(idAI* owner);
	bool AllowedToUnlock(idAI* owner);
	bool AllowedToLock(idAI* owner);

	// adds the door area to forbidden areas (will be re-evaluated after some time)
	void AddToForbiddenAreas(idAI* owner, CFrobDoor* frobDoor);

	// open door routine (checks if the door is locked and starts to open it when possible)
	bool OpenDoor();

	void CloseDoor(idAI* owner, CFrobDoor* frobDoor, idEntity* controller); // grayman #3643

	void DrawDebugOutput(idAI* owner);

	virtual void Save(idSaveGame* savefile) const override;
	virtual void Restore(idRestoreGame* savefile) override;

	// Creates a new Instance of this task
	static HandleDoorTaskPtr CreateInstance();

private:
	// this checks if the gap is large enough to fit through partially openend doors (blocked, interrupted)
	bool FitsThrough();

	void ResetDoor(idAI* owner, CFrobDoor* newDoor);
	void AddUser(idAI* owner, CFrobDoor* frobDoor); // grayman #2345
	bool AssessStoppedDoor(CFrobDoor* door, bool ownerIsBlocker); // grayman #3523
	void Turn(idVec3 pos, CFrobDoor* door, idEntity* controller); // grayman #3643
	void InitDoorPositions(idAI* owner, CFrobDoor* frobDoor, bool susDoorCloseFromSameSide); // grayman #3643
	void StartHandAnim(idAI* owner, idEntity* controller); // grayman #3643
};

} // namespace ai

#endif /* __AI_HANDLE_DOOR_TASK_H__ */
