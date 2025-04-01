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

#include "precompiled.h"
#pragma hdrstop



#include "../Memory.h"
#include "HandleDoorTask.h"
#include "InteractionTask.h"
#include "../AreaManager.h"
#include "SingleBarkTask.h"

namespace ai
{

#define QUEUE_TIMEOUT 10000		// milliseconds (grayman #2345 - max time to wait in a door queue)
#define DOOR_TIMEOUT 20000		// milliseconds (grayman #2700 - max time to execute a move to mid pos or back pos)
#define QUEUE_DISTANCE 150		// grayman #2345 - distance from door where incoming AI pause
#define NEAR_DOOR_DISTANCE 72	// grayman #2345 - less than this and you're close to the door
#define CAN_LEAVE_DOOR_FRACTION 0.1f	// grayman #3523 - AI can leave door handling when closing
										// door that doesn't need locking reaches this fraction open.
										// If a closing door is interrupted, ignore it if fraction
										// open is below this. Become interested if at or above.
#define PUSH_PLAYER_ON_THIS_ATTEMPT 2	// grayman #3523 - When the door movement is interrupted,
										// let the door push the player after this attempt.
#define WALK_AWAY_AFTER_THIS_ATTEMPT 4  // grayman #4830 - walk away if blockedDoorCount gets larger than this
#define TURN_TOWARD_DELAY 750	// how long to wait for a turn to complete

#define CONTROLLER_HEIGHT_HIGH 66
#define CONTROLLER_HEIGHT_LOW  30
#define CONTROLLERMAX_HEIGHT  100 // AI can't reach a controller higher than this

#define REUSE_DOOR_DELAY 100 // grayman #2345 - wait before using a door again. #2706 - lower from 8s to 1s to reduce circling

#define WAIT_FOR_DOOR_TO_START_MOVING 500 // grayman #4077 - when closing a door, check it after this much time has gone by

// Get the name of this task
const idStr& HandleDoorTask::GetName() const
{
	static idStr _name(TASK_HANDLE_DOOR);
	return _name;
}

void HandleDoorTask::InitDoorPositions(idAI* owner, CFrobDoor* frobDoor, bool susDoorCloseFromSameSide)
{
	// determine which side of the door we're on
	_doorSide = owner->GetDoorSide(frobDoor,owner->GetPhysics()->GetOrigin()); // grayman #4227

	// if susDoorCloseFromSameSide is TRUE, flip the side

	if (susDoorCloseFromSameSide)
	{
		if (_doorSide == DOOR_SIDE_FRONT)
		{
			_doorSide = DOOR_SIDE_BACK;
		}
		else
		{
			_doorSide = DOOR_SIDE_FRONT;
		}
	}

	if (_doorSide == DOOR_SIDE_FRONT)
	{
		_frontPosEnt = frobDoor->GetDoorController(DOOR_SIDE_FRONT);
		_backPosEnt = frobDoor->GetDoorController(DOOR_SIDE_BACK);
		_frontDHPosition = frobDoor->GetDoorHandlePosition(DOOR_SIDE_FRONT);
		_backDHPosition = frobDoor->GetDoorHandlePosition(DOOR_SIDE_BACK);
	}
	else
	{
		_frontPosEnt = frobDoor->GetDoorController(DOOR_SIDE_BACK);
		_backPosEnt = frobDoor->GetDoorController(DOOR_SIDE_FRONT);
		_frontDHPosition = frobDoor->GetDoorHandlePosition(DOOR_SIDE_BACK);
		_backDHPosition = frobDoor->GetDoorHandlePosition(DOOR_SIDE_FRONT);
	}

	// set door positions
	_frontPos = frobDoor->GetDoorPosition(_doorSide,DOOR_POS_FRONT);
	_backPos = frobDoor->GetDoorPosition(_doorSide,DOOR_POS_BACK);
	_midPos = frobDoor->GetDoorPosition(_doorSide,DOOR_POS_MID);
}

void HandleDoorTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	Task::Init(owner, subsystem);

	owner->m_DoorQueued = false; // grayman #3647

	Memory& memory = owner->GetMemory();

	_retryCount = 0;
	_triedFitting = false;	// grayman #2345 - useful if you're stuck behind a door
	_leaveQueue = -1;		// grayman #2345
	_leaveDoor = -1;		// grayman #2700
	_canHandleDoor = true;	// grayman #2712
	_useDelay = (int)(owner->spawnArgs.GetFloat("door_open_delay_on_use_anim", "500")/1.5f); // grayman #3755

	CFrobDoor* frobDoor = memory.doorRelated.currentDoor.GetEntity();
	if (frobDoor == NULL)
	{
		subsystem.FinishTask(); // grayman #2345 - can't perform the task if there's no door
		return;
	}

	// grayman #4227 - if you're on the same side of this door as your
	// destination, and you're searching, why go through the door?

	_doorSide = owner->GetDoorSide(frobDoor,owner->GetPhysics()->GetOrigin()); // grayman #4227

	if ( owner->IsSearching() )
	{
		int pointSide = owner->GetDoorSide(frobDoor, owner->GetMoveDest()); // grayman #4227
		if ( _doorSide == pointSide )
		{
			subsystem.FinishTask();
			return;
		}
	}

	idAngles rotate = frobDoor->spawnArgs.GetAngles("rotate", "0 90 0"); // grayman #3643
	_rotates = ( (rotate.yaw != 0) || (rotate.pitch != 0) || (rotate.roll != 0) );

	if (!owner->m_bCanOperateDoors)
	{
		_canHandleDoor = false; // grayman #2712
		if (!frobDoor->IsOpen() || !FitsThrough())
		{
			owner->StopMove(MOVE_STATUS_DEST_UNREACHABLE);
			// add AAS area number of the door to forbidden areas
			AddToForbiddenAreas(owner, frobDoor);

			// grayman #2345 - to accomodate open doors and ~CanOperateDoors spawnflag

			subsystem.FinishTask(); 
			return;
		}
	}

	if (frobDoor->spawnArgs.GetBool("ai_should_not_handle"))
	{
		_canHandleDoor = false; // grayman #2712
		// AI will ignore this door (not try to handle it) 
		if (!frobDoor->IsOpen() || !FitsThrough())
		{
			// if it is closed, add to forbidden areas so AI will not try to path find through
			idAAS*	aas = owner->GetAAS();
			if (aas != NULL)
			{
				int areaNum = frobDoor->GetAASArea(aas);
				if (areaNum > 0) // grayman #2685 - footlocker lids are doors with no area number
				{
					gameLocal.m_AreaManager.AddForbiddenArea(areaNum, owner);
					owner->PostEventMS(&AI_ReEvaluateArea, owner->doorRetryTime, areaNum);
				}
			}
			subsystem.FinishTask();
			return;
		}
		// Door is open, and the AI can fit through, so continue with door-handling
	}

	// grayman #2866 - Is this a suspicious door, and is it time to close it?

	CFrobDoor* closeMe = memory.closeMe.GetEntity();

	_closeFromSameSide = false;
	if ( memory.closeSuspiciousDoor )
	{
		if ( closeMe == frobDoor )
		{
			// grayman #2866 - If this door is one we're closing because it's
			// supposed to be closed, there can't be anyone already in the door queue.
			// The assumption is that whoever's already in the queue will close the door.

			if ( frobDoor->GetUserManager().GetNumUsers() > 0 )
			{
				subsystem.FinishTask(); // quit the task
				return;
			}

			if (memory.susDoorCloseFromThisSide == owner->GetDoorSide(frobDoor,owner->GetPhysics()->GetOrigin())) // grayman #4227
			{
				_closeFromSameSide = true; // close the door w/o going through it
			}

			if (memory.susDoorSameAsCurrentDoor) // grayman #3643
			{
				_closeFromSameSide = !_closeFromSameSide;
			}
		}
	}

	// Let the owner save its move

	owner->m_RestoreMove = false;	// grayman #2706 - whether we should restore a saved move when finished with the door
	if (!owner->GetEnemy())			// grayman #2690 - AI run toward where they saw you last. Don't save that location when handling doors.
	{
		owner->PushMove();
		owner->m_RestoreMove = true;
	}

	owner->m_HandlingDoor = true;

	_wasLocked = false;

	if (frobDoor->IsLocked()) // checks controllers if used, door if not
	{
		// check if we have already tried the door
        idAAS*  aas = owner->GetAAS();
        if (aas != NULL)
        {
			int areaNum = frobDoor->GetAASArea(aas);
            if (gameLocal.m_AreaManager.AreaIsForbidden(areaNum, owner))
			{
				subsystem.FinishTask();
				return;
			}              
		}

		_wasLocked = true;
	}

	CFrobDoor* doubleDoor = frobDoor->GetDoubleDoor();

	AddUser(owner,frobDoor); // grayman #2345 - order the queue if needed
	if (doubleDoor != NULL)
	{
		AddUser(owner,doubleDoor); // grayman #2345 - order the queue if needed
	}

	_doorInTheWay = false;

	InitDoorPositions(owner, frobDoor, _closeFromSameSide);

	if ( _closeFromSameSide ) // grayman #2866
	{
		// close the door w/o going through it
		// InitDoorPositions() set up the door positions to make this happen
		_doorHandlingState = EStateMovingToBackPos; 
	}
	else
	{
		_doorHandlingState = EStateNone; // treat the door normally: go through it, then close it
	}
}

// grayman #2345 - adjust goal positions based on what's happening around the door.

void HandleDoorTask::PickWhere2Go(CFrobDoor* door)
{
	idAI* owner = _owner.GetEntity();
	bool useMid = true;

	// If you're the only AI on the queue, close the door behind you if you're supposed to.
	// grayman #1327 - but only if no one (other than you) is searching around the door

	int numUsers = door->GetUserManager().GetNumUsers();

	if ( _closeFromSameSide ) // grayman #3104 - if in this state, continue to head for _backPos
	{
		useMid = false; // use _backPos
	}
	else if (owner->AI_RUN) // grayman #2670
	{
		// run for the mid position
	}
	else if (!_canHandleDoor) // grayman #2712
	{
		// walk to the mid position
	}
	else if ( owner->AI_AlertIndex >= ESearching ) // grayman #2866 - when approaching to investigate a door, walk to mid, not to back
	{
		// walk to the mid position
	}
	else if (numUsers < 2)
	{
		bool shouldBeLocked = (_wasLocked || door->GetWasFoundLocked() || door->spawnArgs.GetBool("should_always_be_locked", "0")); // grayman #3523
		if (AllowedToClose(owner) && (_doorInTheWay || owner->ShouldCloseDoor(door, shouldBeLocked) || owner->GetMemory().closeSuspiciousDoor)) // grayman #2866
		{
			useMid = false; // use _backPos
		}
	}

	if (useMid)
	{
		if (_doorHandlingState != EStateMovingToMidPos)
		{
			owner->MoveToPosition(_midPos,owner->AI_RUN ? HANDLE_DOOR_ACCURACY_RUNNING : HANDLE_DOOR_ACCURACY); // grayman #4039
			_doorHandlingState = EStateMovingToMidPos;
		}
	}
	else
	{
		owner->MoveToPosition(_backPos, HANDLE_DOOR_ACCURACY); // don't run to backside of door to close it
		_doorHandlingState = EStateMovingToBackPos;
	}
}

void HandleDoorTask::MoveToSafePosition(CFrobDoor* door)
{
	idAI* owner = _owner.GetEntity();

	idVec3 centerPos = door->GetClosedBox().GetCenter();
	idVec3 dir2AI = owner->GetPhysics()->GetOrigin() - centerPos;
	dir2AI.z = 0;
	dir2AI.NormalizeFast();
	_safePos = centerPos + 1.5*NEAR_DOOR_DISTANCE*dir2AI;
	owner->StopMove(MOVE_STATUS_DONE); // grayman #3643
	owner->TurnToward(_safePos); // grayman #3643
	_waitEndTime = gameLocal.time + 2000;
	_doorHandlingState = EStateMovingToSafePos1;
}

// grayman #3523 - Determine whether an interrupted door
// is something to be concerned about. Also set up
// trying to push the player out of the way.

bool HandleDoorTask::AssessStoppedDoor(CFrobDoor* door, bool ownerIsBlocker)
{
	idAI* owner = _owner.GetEntity();

	assert((door != NULL) && (owner != NULL)); // must be fulfilled

	// Bark about it.

	owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask("snd_alert1")));

	owner->GetMemory().blockedDoorCount++; // grayman #3726 - keep track, even if owner is blocker; grayman #4830
	int bdcount = owner->GetMemory().blockedDoorCount; // grayman #4830

	if ( ownerIsBlocker )
	{
		return false; // walk away, but don't search
	}

	if ( bdcount > WALK_AWAY_AFTER_THIS_ATTEMPT ) // grayman #4830
	{
		return false; // walk away, but don't search
	}

	// The first interruptions only elicit a bark from the AI.

	if ( bdcount < PUSH_PLAYER_ON_THIS_ATTEMPT) // grayman #4830
	{
		return false; // walk away, but don't search
	}

	// On the next attempt, let the door push the player in
	// case the player's interrupting the door movement.

	if ( bdcount == PUSH_PLAYER_ON_THIS_ATTEMPT) // grayman #4830
	{
		// The door has a flag that tells it whether it can push
		// the player or not. Normally, this is FALSE, but we want
		// to try to push the player out of the way (assuming that's
		// why the door is blocked), so let's set that flag to TRUE,
		// remember what the previous setting was, and remember that
		// we're doing this, so we can reset the flag when done.
		// These variables are cleared when the open succeeds.

		// grayman #3748 - disable frobbing on the door during this

		if (!door->IsPushingHard())
		{
			door->PushDoorHard();
		}
		return false; // walk away, but don't search
	}

	if ( bdcount == WALK_AWAY_AFTER_THIS_ATTEMPT ) // grayman #4830
	{
		return false; // walk away, but don't search
	}

	// Now get upset and treat the door as suspicious.

	// Search for a while. Remember the door so you can close it later. 

	owner->SetUpSuspiciousDoor(door); // grayman #3643
	return true; // walk away and search
}

// grayman #3643 - AI turns toward different positions
// just prior to frobbing a door or control switch
void HandleDoorTask::Turn(idVec3 pos, CFrobDoor* door, idEntity* controller)
{
	idVec3 targetPos;

	if (controller) // door controlled by switches
	{
		targetPos = controller->GetPhysics()->GetOrigin();
	}
	else // frob door directly
	{
		if (_rotates)
		{
			targetPos = pos;
		}
		else // sliding door
		{
			targetPos = door->GetClosedBox().GetCenter();
		}
	}
	_owner.GetEntity()->TurnToward(targetPos);
}

void HandleDoorTask::StartHandAnim(idAI* owner, idEntity* controller)
{
	if (controller == NULL)
	{
		owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Use_righthand", 4);
		owner->SetWaitState("use_righthand"); // grayman #3643
		return;
	}

	// Controllers can be high off the floor, door-handle high, or low to the floor.

	idStr torsoAnimation = "";
	float controllerHeight = controller->GetPhysics()->GetOrigin().z - owner->GetPhysics()->GetOrigin().z;

	torsoAnimation = "Torso_Controller"; // borrowing the electric light relight anims
	
	if (controllerHeight > CONTROLLER_HEIGHT_HIGH) // high?
	{
		torsoAnimation.Append("_High"); // reach up toward the controller
	}
	else if (controllerHeight < CONTROLLER_HEIGHT_LOW) // low?
	{
		torsoAnimation.Append("_Low"); // reach down toward the controller
	}
	else // medium
	{
		torsoAnimation.Append("_Med"); // reach out toward the controller
	}

	owner->SetAnimState(ANIMCHANNEL_TORSO, torsoAnimation.c_str(), 4); // this plays the legs anim also
	owner->SetWaitState("controller"); // grayman #3755
}

bool HandleDoorTask::Perform(Subsystem& subsystem)
{
	idAI* owner = _owner.GetEntity();
	Memory& memory = owner->GetMemory();
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("HandleDoorTask performing by %s, state = %d\r",owner->name.c_str(),(int)_doorHandlingState);

	CFrobDoor* frobDoor = memory.doorRelated.currentDoor.GetEntity();
	if (frobDoor == NULL)
	{
		return true;
	}

	// grayman #2948 - leave door handling if KO'ed or dead

	if ( owner->AI_KNOCKEDOUT || owner->AI_DEAD )
	{
		return true;
	}

	// grayman #2816 - stop door handling for various reasons

	if ( memory.stopHandlingDoor )
	{
		return true;
	}

	int currentDoorSide = owner->GetDoorSide(frobDoor,owner->GetPhysics()->GetOrigin()); // grayman #4044 // grayman #4227

	// grayman #3755 - stop door handling if you've run through the doorway
	// and you have an enemy
	if ( owner->AI_RUN &&
		 ( owner->GetEnemy() != NULL ) &&
		 ( _doorSide != currentDoorSide ) )
	{
		return true;
	}
	
	if (frobDoor->IsOpen())
	{
		// The door is open. If it's not a "should be closed" door, and I can't
		// handle it, and I don't fit through the opening, add the door's area #
		// to my list of forbidden areas.

		if ( !memory.closeSuspiciousDoor ) // grayman #2866
		{
			if (!_canHandleDoor && !FitsThrough()) // grayman #2712
			{
				// grayman #4044 - only stop walking if you never
				// made it through the door
				if ( _doorSide == currentDoorSide )
				{
					owner->StopMove(MOVE_STATUS_DEST_UNREACHABLE);
					// add AAS area number of the door to forbidden areas
					AddToForbiddenAreas(owner, frobDoor);
				}
				return true;
			}
		}
	}
	else 
	{
		// The door is closed. If I can't deal with it, add its area #
		// to my list of forbidden areas.
		if (!_canHandleDoor) // grayman #2712
		{
			// grayman #4044 - only stop walking if you never
			// made it through the door
			if ( _doorSide == currentDoorSide )
			{
				owner->StopMove(MOVE_STATUS_DEST_UNREACHABLE);
				// add AAS area number of the door to forbidden areas
				AddToForbiddenAreas(owner, frobDoor);
			}
			return true;
		}
	}

	// grayman #2700 - we get a certain amount of time to complete a move
	// to the mid position or back position before leaving door handling

	if ((_leaveDoor > 0) && (gameLocal.time >= _leaveDoor))
	{
		_leaveDoor = -1;
		return true;
	}

	int numUsers = frobDoor->GetUserManager().GetNumUsers();
	idActor* masterUser = frobDoor->GetUserManager().GetMasterUser();
	int queuePos = frobDoor->GetUserManager().GetIndex(owner); // grayman #2345

	const idVec3& frobDoorOrg = frobDoor->GetPhysics()->GetOrigin();
	const idVec3& openPos = frobDoorOrg + frobDoor->GetOpenPos();
	const idVec3& closedPos = frobDoorOrg + frobDoor->GetClosedPos();
	const idVec3& centerPos = frobDoor->GetClosedBox().GetCenter(); // grayman #1327
	const idVec3& currentPos = frobDoor->GetCurrentPos(); // grayman #3523
	//gameRenderWorld->DebugArrow(colorCyan, currentPos, currentPos + idVec3(0, 0, 20), 2, 1000);

	// if our current door is part of a double door, this is the other part.
	CFrobDoor* doubleDoor = frobDoor->GetDoubleDoor();

	idBounds bounds = owner->GetPhysics()->GetBounds();

	// angua: move the bottom of the bounds up a bit, to avoid finding small objects on the ground that are "in the way"

	// grayman #2691 - except for AI whose bounding box height is less than maxStepHeight, otherwise applying the bump up
	// causes the clipmodel to be "upside-down", which isn't good. In that case, give the bottom a bump up equal to half
	// of the clipmodel's height so it at least gets a small bump.

	float ht = owner->GetAAS()->GetSettings()->maxStepHeight;
	if (bounds[0].z + ht < bounds[1].z)
	{
		bounds[0].z += ht;
	}
	else
	{
		bounds[0].z += (bounds[1].z - bounds[0].z)/2.0;
	}
	// bounds[0][2] += 16; // old way
	float size = bounds[1][0];

	if (cv_ai_door_show.GetBool()) 
	{
		DrawDebugOutput(owner);
	}

	idEntity *tactileEntity = owner->GetTactileEntity();	// grayman #2692
	idVec3 ownerOrigin = owner->GetPhysics()->GetOrigin();	// grayman #2692

	// grayman #3390 - refactor door handling code

	switch (_doorHandlingState)
	{
		case EStateNone:
			if (!frobDoor->IsOpen()) // closed
			{
				if ( ( doubleDoor != NULL ) && doubleDoor->IsOpen() )
				{
					// the other part of the double door is already open
					// no need to open this one
					ResetDoor(owner, doubleDoor);
				}
				else
				{
					if (!AllowedToOpen(owner))
					{
						AddToForbiddenAreas(owner, frobDoor);
						return true;
					}

					/*idEntity* controller = GetRemoteControlEntityForDoor();

					if ( ( controller != NULL ) && ( masterUser == owner ) && ( controller->GetUserManager().GetNumUsers() == 0 ) )
					{
						// We have an entity to control this door, interact with it
						subsystem.PushTask(TaskPtr(new InteractionTask(controller)));

						// After the InteractionTask completes, control will return
						// to HandleDoorTask::Init().

						return false;
					}*/
				}

				// grayman #3317 - use less position accuracy when running
				if (!owner->MoveToPosition(_frontPos,owner->AI_RUN ? HANDLE_DOOR_ACCURACY_RUNNING : HANDLE_DOOR_ACCURACY))
				{
					// TODO: position not reachable, need a better one
				}

				_doorHandlingState = EStateApproachingDoor;
			}
			else // open
			{
				if (!_canHandleDoor) // grayman #2712
				{
					_doorHandlingState = EStateApproachingDoor;
					break;
				}

				// check if we are blocking the door
				if (frobDoor->IsBlocked() || 
					frobDoor->WasInterrupted() || 
					frobDoor->WasStoppedDueToBlock())
				{
					if (FitsThrough())
					{
						if (owner->AI_AlertIndex >= ESearching)
						{
							return true;
						}

						// grayman #3317 - use less position accuracy when running
						if (!owner->MoveToPosition(_frontPos,owner->AI_RUN ? HANDLE_DOOR_ACCURACY_RUNNING : HANDLE_DOOR_ACCURACY))
						{
							// TODO: position not reachable, need a better one
						}

						_doorHandlingState = EStateApproachingDoor;
						break;
					}
					else if (!AllowedToOpen(owner))
					{
						AddToForbiddenAreas(owner, frobDoor);
						return true;
					}
					else
					{
						/*idEntity* controller = GetRemoteControlEntityForDoor();

						if (controller != NULL)
						{	 
							if (masterUser == owner && controller->GetUserManager().GetNumUsers() == 0)
							{
								// We have an entity to control this door, interact with it
								subsystem.PushTask(TaskPtr(new InteractionTask(controller)));

								// After the InteractionTask completes, control will return
								// to HandleDoorTask::Init().

								return false;
							}
						}*/

						// grayman #3317 - use less position accuracy when running
						if (!owner->MoveToPosition(_frontPos,owner->AI_RUN ? HANDLE_DOOR_ACCURACY_RUNNING : HANDLE_DOOR_ACCURACY))
						{
							// TODO: position not reachable, need a better one
						}

						_doorHandlingState = EStateApproachingDoor;
					}
				}
				else
				{
					// door is open and possibly in the way, may need to close it

					// test the angle between the view direction of the AI and the open door
					// door can only be in the way when the view direction 
					// is approximately perpendicular to the open door

					idVec3 ownerDir = owner->viewAxis.ToAngles().ToForward();

					idVec3 testVector = openPos - frobDoorOrg;
					testVector.z = 0;
					float length = testVector.LengthFast();
					float dist = size * SQUARE_ROOT_OF_2;
					length += dist;
					testVector.NormalizeFast();

					float product = idMath::Fabs(ownerDir * testVector);

					// grayman #2453 - the above test can give a false result depending
					// on the AI's orientation to the door, so make sure we're close enough
					// to the door to make the test valid.

					idVec3 dir = centerPos - owner->GetPhysics()->GetOrigin(); // grayman #3104 - use center of closed door
					dir.z = 0;
					float dist2Door = dir.LengthFast();

					if (!_rotates || (product > 0.3f) || (dist2Door > QUEUE_DISTANCE)) // grayman #3643 - no need to check sliding doors
					{
						// door is open and not (yet) in the way

						// grayman #3647 - give him a place to go
						if (!owner->MoveToPosition(_backPos,owner->AI_RUN ? HANDLE_DOOR_ACCURACY_RUNNING : HANDLE_DOOR_ACCURACY))
						{
							// TODO: position not reachable, need a better one
						}
						_doorHandlingState = EStateApproachingDoor; // grayman #2345 - you should pause if necessary
						return false;
					}

					// check if there is a way around
					idTraceModel trm(bounds);
					idClipModel clip(trm);
	
					// check point next to the open door
					
					idVec3 testPoint = frobDoorOrg + testVector * length;

					int contents = gameLocal.clip.Contents(testPoint, &clip, mat3_identity, CONTENTS_SOLID, owner);

					if (contents)
					{
						// door is in the way, there is not enough space next to the door to fit through
						// find a suitable position and close the door
						DoorInTheWay(owner, frobDoor);
						_doorHandlingState = EStateApproachingDoor;
					}
					else
					{
						// check a little bit in front and behind the test point, 
						// might not be enough space there to squeeze through
						idVec3 normal = testVector.Cross(idVec3(0, 0, 1));
						normal.NormalizeFast();
						idVec3 testPoint2 = testPoint + dist * normal;

						contents = gameLocal.clip.Contents(testPoint2, &clip, mat3_identity, CONTENTS_SOLID, owner);
						if (contents)
						{
							// door is in the way, there is not enough space to fit through
							// find a suitable position and close the door
							DoorInTheWay(owner, frobDoor);
							_doorHandlingState = EStateApproachingDoor;
						}
						else
						{
							idVec3 testPoint3 = testPoint - dist * normal;

							contents = gameLocal.clip.Contents(testPoint3, &clip, mat3_identity, CONTENTS_SOLID, owner);
							if (contents)
							{
								// door is in the way, there is not enough space to fit through
								// find a suitable position and close the door
								DoorInTheWay(owner, frobDoor);
								_doorHandlingState = EStateApproachingDoor;
							}
							else
							{
								// door is not in the way and open

								// grayman #3647 - give him a place to go
								if (!owner->MoveToPosition(_backPos,owner->AI_RUN ? HANDLE_DOOR_ACCURACY_RUNNING : HANDLE_DOOR_ACCURACY))
								{
									// TODO: position not reachable, need a better one
								}
								_doorHandlingState = EStateApproachingDoor; // grayman #2345 - you should pause if necessary
								return false;
							}
						}
					}
				}
			}
			break;

		case EStateApproachingDoor:
			if (!frobDoor->IsOpen()) // closed
			{
				idVec3 dir = centerPos - owner->GetPhysics()->GetOrigin(); // grayman #3104 - distance from center of door
				dir.z = 0;
				float dist = dir.LengthFast();
				if (masterUser == owner)
				{
					if ( owner->AI_MOVE_DONE )
					{
						owner->SetMoveAccuracy(owner->AI_RUN ? HANDLE_DOOR_ACCURACY_RUNNING : HANDLE_DOOR_ACCURACY); // grayman #4039
					}
					if (owner->ReachedPos(_frontPos, MOVE_TO_POSITION) || // grayman #2345 #2692 - are we close enough to reach around a blocking AI?
						(tactileEntity && tactileEntity->IsType(idAI::Type) && (closedPos - ownerOrigin).LengthFast() < 100))
					{
						// reached front position
						owner->StopMove(MOVE_STATUS_DONE);
						Turn(closedPos,frobDoor,_frontPosEnt.GetEntity()); // grayman #3643
						_waitEndTime = gameLocal.time + TURN_TOWARD_DELAY;
						_doorHandlingState = EStateWaitBeforeOpen;
						break;
					}

					//owner->PrintGoalData(_frontPos, 8);
					if (dist <= QUEUE_DISTANCE) // grayman #2345 - this was the next layer up
					{
						if (_doorInTheWay)
						{	
							DoorInTheWay(owner, frobDoor);
							_doorHandlingState = EStateMovingToBackPos;
						}
						else
						{
							// grayman #3317 - use less position accuracy when running
							if (!owner->MoveToPosition(_frontPos,owner->AI_RUN ? HANDLE_DOOR_ACCURACY_RUNNING : HANDLE_DOOR_ACCURACY))
							{
								// TODO: position not reachable, need a better one
							}
							_doorHandlingState = EStateMovingToFrontPos;
						}
					}
				}
				else
				{
					if (owner->GetMoveStatus() == MOVE_STATUS_WAITING)
					{
						// grayman #2345 - if you've been in queue too long, leave

						if ((_leaveQueue != -1) && (gameLocal.time >= _leaveQueue))
						{
							owner->m_leftQueue = true; // timed out of a door queue
							_leaveQueue = -1; // grayman #2345
							return true;
						}
					}
					else if (dist <= QUEUE_DISTANCE) // grayman #2345
					{
						if (dist <= NEAR_DOOR_DISTANCE) // grayman #2345 - too close to door when you're not the master?
						{
							MoveToSafePosition(frobDoor); // grayman #3390
						}
						else
						{
							owner->StopMove(MOVE_STATUS_WAITING);
							if (queuePos > 0)
							{
								_leaveQueue = gameLocal.time + (1 + gameLocal.random.RandomFloat()/2.0)*QUEUE_TIMEOUT; // set queue timeout
							}
							owner->TurnToward(centerPos); // grayman #3643
						}
					}
				}
			}
			else // open
			{
				if (_canHandleDoor) // grayman #2712
				{
					// check if we are blocking the door
					if (frobDoor->IsBlocked() || 
						frobDoor->WasInterrupted() || 
						frobDoor->WasStoppedDueToBlock())
					{
						if (FitsThrough())
						{
							if (owner->AI_AlertIndex >= ESearching)
							{
								return true;
							}
						}
						else // grayman #3390
						{
							if ( masterUser == owner )
							{
								if (!owner->MoveToPosition(_frontPos,owner->AI_RUN ? HANDLE_DOOR_ACCURACY_RUNNING : HANDLE_DOOR_ACCURACY))
								{
									// TODO: position not reachable, need a better one
								}

								_doorHandlingState = EStateMovingToFrontPos;
								break;
							}
						}
					}
				}

				// grayman #2345 - if you're waiting in the queue, use a timeout
				// to leave the queue - masterUser is most likely stuck

				if (owner->GetMoveStatus() == MOVE_STATUS_WAITING)
				{
					// grayman #2345 - if you've been in queue too long, leave

					if ((_leaveQueue != -1) && (gameLocal.time >= _leaveQueue))
					{
						owner->m_leftQueue = true; // timed out of a door queue
						_leaveQueue = -1;
						return true;
					}
				}

				idVec3 dir = centerPos - owner->GetPhysics()->GetOrigin(); // grayman #3104 - use center of closed door
				dir.z = 0;
				float dist = dir.LengthFast();
				if (dist <= QUEUE_DISTANCE)
				{
					// grayman #1327 - don't move forward if someone other than me is searching near the door

					bool canMove2Door = ( masterUser == owner );
					if ( canMove2Door )
					{
						idEntity* searchingEnt = frobDoor->GetSearching();
						if ( searchingEnt )
						{
							if ( searchingEnt->IsType(idAI::Type) )
							{
								idAI* searchingAI = static_cast<idAI*>(searchingEnt);
								if ( searchingAI != owner )
								{
									idVec3 dirSearching = centerPos - searchingAI->GetPhysics()->GetOrigin(); // grayman #3104 - use center of closed door
									dirSearching.z = 0;
									float distSearching = dirSearching.LengthFast() - 32;
									if ( distSearching <= dist )
									{
										canMove2Door = false;
									}
								}
							}
						}
					}

					if ( canMove2Door )
					{
						if (!_canHandleDoor) // grayman #2712
						{
							owner->m_canResolveBlock = false;
							PickWhere2Go(frobDoor);
							break;
						}

						if (_doorInTheWay)
						{	
							DoorInTheWay(owner, frobDoor);
							_doorHandlingState = EStateMovingToBackPos;
						}
						else
						{
							PickWhere2Go(frobDoor); // grayman #3317 - go through an already-opened door, don't bother with _frontPos
						}
					}
					else if (dist <= NEAR_DOOR_DISTANCE) // grayman #1327 - too close to door when you're not the master
														 // or if you are the master but someone's searching around the door?
					{
						MoveToSafePosition(frobDoor); // grayman #3390
					}
					else if (owner->GetMoveStatus() != MOVE_STATUS_WAITING)
					{
						owner->StopMove(MOVE_STATUS_WAITING);
						owner->TurnToward(centerPos); // grayman #3643
						if (queuePos > 0)
						{
							_leaveQueue = gameLocal.time + (1 + gameLocal.random.RandomFloat()/2.0)*QUEUE_TIMEOUT; // set queue timeout
						}
					}
				}
				else if (owner->MoveDone())
				{
					// grayman #3317 - use less position accuracy when running
					if (!owner->MoveToPosition(_frontPos,owner->AI_RUN ? HANDLE_DOOR_ACCURACY_RUNNING : HANDLE_DOOR_ACCURACY))
					{
						// TODO: position not reachable, need a better one
					}

					_doorHandlingState = EStateMovingToFrontPos; // grayman #2712
				}
			}
			break;
		case EStateMovingToSafePos1:
			{
				// don't care whether door is open or closed in this state
				float yawDiff = idMath::AngleNormalize180(owner->GetCurrentYaw() - owner->GetIdealYaw()); // how close are you to your ideal yaw?
				if ( ( idMath::Fabs( yawDiff ) < 0.1f ) || ( gameLocal.time >= _waitEndTime ) )
				{
					// turn done, move!
					if (!owner->MoveToPosition(_safePos,HANDLE_DOOR_ACCURACY))
					{
						// TODO: position not reachable, need a better one
					}
					_doorHandlingState = EStateMovingToSafePos2;
					_waitEndTime = gameLocal.time;
				}
			}
			break;
		case EStateMovingToSafePos2:
			// don't care whether door is open or closed in this state
			if (owner->AI_MOVE_DONE || owner->ReachedPos(_safePos, MOVE_TO_POSITION) || (owner->GetTactileEntity() != NULL)) // grayman #3004 - leave this state if you're not moving
			{
				owner->StopMove(MOVE_STATUS_WAITING);
				owner->TurnToward(centerPos); // grayman #1327

				// grayman #3390 - you've moved away from the door. If someone
				// else is closer than you, and you're the master, give up that role
				if ( masterUser == owner )
				{
					frobDoor->GetUserManager().ResetMaster(frobDoor); // redefine which AI is the master
					masterUser = frobDoor->GetUserManager().GetMasterUser();
					queuePos = frobDoor->GetUserManager().GetIndex(owner);
				}

				if (queuePos > 0) // not the master
				{
					_leaveQueue = gameLocal.time + (1 + gameLocal.random.RandomFloat()/2.0)*QUEUE_TIMEOUT; // set queue timeout
				}
				_doorHandlingState = EStateApproachingDoor;
			}
			else
			{
				//owner->PrintGoalData(_safePos, 2);
			}
			break;
		case EStateMovingToFrontPos:
			if (!frobDoor->IsOpen()) // closed
			{
				owner->m_canResolveBlock = false; // grayman #2345

				if (doubleDoor != NULL && doubleDoor->IsOpen())
				{
					// the other part of the double door is already open
					// no need to open this one
					ResetDoor(owner, doubleDoor);
					break;
				}
		
				if (!AllowedToOpen(owner))
				{
					AddToForbiddenAreas(owner, frobDoor);
					return true;
				}

				if ( owner->AI_MOVE_DONE )
				{
					owner->SetMoveAccuracy(owner->AI_RUN ? HANDLE_DOOR_ACCURACY_RUNNING : HANDLE_DOOR_ACCURACY); // grayman #4039
				}

				if (owner->ReachedPos(_frontPos, MOVE_TO_POSITION) || // grayman #2345 #2692 - are we close enough to reach around a blocking AI?
					(tactileEntity && tactileEntity->IsType(idAI::Type) && (closedPos - ownerOrigin).LengthFast() < 100))
				{
					// reached front position
					owner->StopMove(MOVE_STATUS_DONE);
					Turn(closedPos,frobDoor,_frontPosEnt.GetEntity()); // grayman #3643
					_waitEndTime = gameLocal.time + TURN_TOWARD_DELAY;
					_doorHandlingState = EStateWaitBeforeOpen;
					break;
				}
				else if (owner->AI_MOVE_DONE)
				{
					// grayman #3317 - use less position accuracy when running
					if (!owner->MoveToPosition(_frontPos,owner->AI_RUN ? HANDLE_DOOR_ACCURACY_RUNNING : HANDLE_DOOR_ACCURACY))
					{
						// TODO: position not reachable, need a better one
					}
					else
					{
						//owner->PrintGoalData(_frontPos, 9);
					}
				}
				else
				{
					//owner->PrintGoalData(_frontPos, 3);
				}
			}
			else // open
			{
				owner->m_canResolveBlock = false; // grayman #2345

				// check if the door was blocked or interrupted
				if (frobDoor->IsBlocked() || 
					frobDoor->WasInterrupted() || 
					frobDoor->WasStoppedDueToBlock())
				{
					if (FitsThrough())
					{
						// gap is large enough, go through if searching or in combat
						if (owner->AI_AlertIndex >= ESearching)
						{
							return true;
						}

						// gap is large enough, move to back position
						PickWhere2Go(frobDoor);
						break; // grayman #3390
					}

					// I can't fit through the door. Can I open it, even though I might
					// not be the master?

					if (!AllowedToOpen(owner))
					{
						AddToForbiddenAreas(owner, frobDoor);
						return true;
					}

					if ( owner->AI_MOVE_DONE )
					{
						owner->SetMoveAccuracy(owner->AI_RUN ? HANDLE_DOOR_ACCURACY_RUNNING : HANDLE_DOOR_ACCURACY); // grayman #4039
					}

					if (owner->ReachedPos(_frontPos, MOVE_TO_POSITION) ||
						((closedPos - ownerOrigin).LengthFast() < 100)) // grayman #3390 - are we close enough?
					{
						// reached front position, or close enough
						owner->StopMove(MOVE_STATUS_DONE);
						Turn(currentPos,frobDoor,_frontPosEnt.GetEntity()); // grayman #3643
						_waitEndTime = gameLocal.time + TURN_TOWARD_DELAY;
						_doorHandlingState = EStateWaitBeforeOpen;
					}
					else if (owner->AI_MOVE_DONE)
					{
						// grayman #3317 - use less position accuracy when running
						if (!owner->MoveToPosition(_frontPos,owner->AI_RUN ? HANDLE_DOOR_ACCURACY_RUNNING : HANDLE_DOOR_ACCURACY))
						{
							// TODO: position not reachable, need a better one
						}
					}
					else // can't fit through door, can't open it, see if I need to move out of the way
					{
						//owner->PrintGoalData(_frontPos, 4);
						_doorHandlingState = EStateApproachingDoor;
					}
				}
				// door is already open, move to back position or mid position if you're the master
				else if (masterUser == owner)
				{
					// grayman #2345 - introduce use of a midpoint to help AI through doors that were found open

					PickWhere2Go(frobDoor);
				}
				else // grayman #3390 - otherwise, you shouldn't be moving to the front pos
				{
					_doorHandlingState = EStateApproachingDoor;
				}
			}
			break;
		case EStateRetryInterruptedOpen1: // grayman #3523
			// If here, you need to close the door and try opening it again.

			if (!frobDoor->IsOpen()) // closed
			{
				// already closed, no need for a retry open
				Turn(closedPos,frobDoor,_frontPosEnt.GetEntity()); // grayman #3643
				_waitEndTime = gameLocal.time + TURN_TOWARD_DELAY;
				_doorHandlingState = EStateWaitBeforeOpen;
				_retryCount++; // might come back here, so keep track of visit count
			}
			else // open
			{
				if (gameLocal.time >= _waitEndTime)
				{
					StartHandAnim(owner, _frontPosEnt.GetEntity()); // grayman #3643
					//owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Use_righthand", 4);
					_doorHandlingState = EStateRetryInterruptedOpen2;
					_waitEndTime = gameLocal.time + _useDelay; // grayman #3755
				}
			}
			break;
		case EStateRetryInterruptedOpen2: // grayman #3523
			if (!frobDoor->IsOpen()) // closed
			{
				// already closed, no need for a retry open
				Turn(closedPos,frobDoor,_frontPosEnt.GetEntity()); // grayman #3643
				_waitEndTime = gameLocal.time + TURN_TOWARD_DELAY;
				_doorHandlingState = EStateWaitBeforeOpen;
				_retryCount++; // might come back here, so keep track of visit count
			}
			else // open
			{
				if (gameLocal.time >= _waitEndTime)
				{
					CloseDoor(owner,frobDoor,_frontPosEnt.GetEntity());
					_waitEndTime = gameLocal.time + 2000;
					_doorHandlingState = EStateRetryInterruptedOpen3;
				}
			}
			break;
		case EStateRetryInterruptedOpen3: // grayman #3523
			if (!frobDoor->IsOpen()) // closed
			{
				Turn(closedPos,frobDoor,_frontPosEnt.GetEntity()); // grayman #3643
				_waitEndTime = gameLocal.time + TURN_TOWARD_DELAY;
				_doorHandlingState = EStateWaitBeforeOpen;
				_retryCount++; // might come back here, so keep track of visit count
			}
			else // open
			{
				if (gameLocal.time >= _waitEndTime)
				{
					// grayman #3523 - need to do something, because
					// the door isn't closing. Maybe it's been interrupted again?
					if (frobDoor->IsBlocked() || 
						frobDoor->WasInterrupted() || 
						frobDoor->WasStoppedDueToBlock())
					{
						// Door was trying to close, and it stopped.
						// Try opening it again.
						_doorHandlingState = EStateMovingToFrontPos;
					}
				}

				// otherwise, wait for door to close or time to expire
			}
			break;
		case EStateWaitBeforeOpen:
			if (!frobDoor->IsOpen()) // closed
			{
				if ( (doubleDoor != NULL) && doubleDoor->IsOpen() )
				{
					// the other part of the double door is already open
					// no need to open this one
					ResetDoor(owner, doubleDoor);
					break;
				}

				if (!AllowedToOpen(owner))
				{
					AddToForbiddenAreas(owner, frobDoor);
					return true;
				}

				if (gameLocal.time >= _waitEndTime)
				{
					if (masterUser == owner)
					{
						StartHandAnim(owner, _frontPosEnt.GetEntity()); // grayman #3643
						//owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Use_righthand", 4);

						_doorHandlingState = EStateStartOpen;
						_waitEndTime = gameLocal.time + _useDelay; // grayman #3755
						memory.latchPickedPocket = true; // grayman #3559 - delay any picked pocket reaction
					}
				}
			}
			else // open
			{
				// check blocked or interrupted
				if (!FitsThrough())
				{
					if (!AllowedToOpen(owner))
					{
						if (frobDoor->IsBlocked() ||
							frobDoor->WasInterrupted() || 
							frobDoor->WasStoppedDueToBlock())
						{
							AddToForbiddenAreas(owner, frobDoor);
							return true;
						}
					}
					else if (gameLocal.time >= _waitEndTime)
					// grayman #720 - need the AI to reach for the door
					{
						StartHandAnim(owner, _frontPosEnt.GetEntity()); // grayman #3643
						//owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Use_righthand", 4);
						_doorHandlingState = EStateStartOpen;
						_waitEndTime = gameLocal.time + _useDelay; // grayman #3755
						memory.latchPickedPocket = true; // grayman #3559 - delay any picked pocket reaction
					}
				}
				else
				{
					// no need for waiting, door is already open, let's move
					if (owner->AI_AlertIndex >= ESearching)
					{
						return true;
					}

					PickWhere2Go(frobDoor);
				}
			}
			break;
		case EStateStartOpen:
			if (!frobDoor->IsOpen()) // closed
			{
				if (doubleDoor != NULL && doubleDoor->IsOpen())
				{
					// the other part of the double door is already open
					// no need to open this one
					ResetDoor(owner, doubleDoor);
					break;
				}

				if (!AllowedToOpen(owner))
				{
					AddToForbiddenAreas(owner, frobDoor);
					return true;
				}

				if (gameLocal.time >= _waitEndTime)
				{
					if (masterUser == owner)
					{
						if (_retryCount == 0) // grayman #3523
						{
							frobDoor->SetWasFoundLocked(frobDoor->IsLocked()); // grayman #3104
						}

						if (!OpenDoor())
						{
							return true;
						}
					}
				}
			}
			else // open
			{
				if (frobDoor->IsBlocked() ||
					frobDoor->WasInterrupted() || 
					frobDoor->WasStoppedDueToBlock())
				{
					if (!FitsThrough())
					{
						if (!AllowedToOpen(owner))
						{
							AddToForbiddenAreas(owner, frobDoor);
							return true;
						}

						if (gameLocal.time >= _waitEndTime)
						{
							if (!OpenDoor())
							{
								return true;
							}
						}
					}
				}
				else
				{
					if (owner->AI_AlertIndex >= ESearching)
					{
						return true;
					}

					// no need for waiting, door is already open, let's move

					PickWhere2Go(frobDoor);
				}
			}
			break;
		case EStateOpeningDoor:
			if (!frobDoor->IsOpen()) // closed
			{
				// we have already started opening the door, but it is closed

				// grayman #2862 - it's possible that we JUST opened the door and
				// it hasn't yet registered that it's not closed. So wait a short
				// while before deciding that it's still closed.

				if ( gameLocal.time < _waitEndTime )
				{
					break;
				}

				// the door isn't changing state, so it must truly be closed

				if ( (doubleDoor != NULL) && doubleDoor->IsOpen())
				{
					// the other part of the double door is already open
					// no need to open this one
					ResetDoor(owner, doubleDoor);
					break;
				}

				if (!AllowedToOpen(owner))
				{
					AddToForbiddenAreas(owner, frobDoor);
					return true;
				}

				// try again
				
				owner->StopMove(MOVE_STATUS_DONE);
				Turn(closedPos,frobDoor,_frontPosEnt.GetEntity()); // grayman #3643

				if (masterUser == owner)
				{
					frobDoor->SetWasFoundLocked(frobDoor->IsLocked()); // grayman #3104
					if (!OpenDoor())
					{
						return true;
					}
				}
			}
			else // open
			{
				// check blocked
				if (frobDoor->IsBlocked() || 
					(frobDoor->WasInterrupted() && frobDoor->WasStoppedDueToBlock()))
				{
					// grayman #3523 - This is where the door is stopped as the AI is opening it.
					// Handles both cases where the door is moving away or toward the AI.
					if ( !_triedFitting && FitsThrough() && (masterUser == owner)) // grayman #2345 - added _triedFitting
					{
						// gap is large enough, move to back position
						PickWhere2Go(frobDoor);
						_triedFitting = true; // grayman #2345
					}
					else
					{
						_triedFitting = false; // grayman #2345 - reset if needed
						if (frobDoor->GetLastBlockingEnt() == owner)
						{
							// we are blocking the door
							// check whether we should open or close it

							// grayman #3643 - handle sliding door
							bool openDoor = true; // assume a sliding door, which you always want to re-open

							if (_rotates)
							{
								// door rotates
								idVec3 forward = owner->viewAxis.ToAngles().ToForward(); // grayman #2345 - use viewaxis, not getaxis()
								idVec3 doorDir = frobDoor->GetOpenDir() * frobDoor->GetPhysics()->GetAxis();

								if (forward * doorDir < 0)
								{
									// We are facing the opposite of the opening direction of the door.
									// Close it, exit the task, and try again.

									//openDoor = false; // grayman #3949
									return true;		// grayman #3949
								}
							}

							if (!openDoor)
							{
								// Close it and try opening again.
								StartHandAnim(owner, _backPosEnt.GetEntity()); // grayman #3643
								//owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Use_righthand", 4);
								_doorHandlingState = EStateStartClose;
								_waitEndTime = gameLocal.time + _useDelay; // grayman #3755
							}
							else
							{
								// we are facing the opening direction of the door
								// open it
								owner->StopMove(MOVE_STATUS_DONE);
								Turn(closedPos,frobDoor,_frontPosEnt.GetEntity()); // grayman #3643

								if (masterUser == owner)
								{
									if (!OpenDoor())
									{
										return true;
									}
								}
							}
						}
						// something is blocking the door
						// possibly the player, another AI or an object
						else if (_retryCount > 3)
						{
							// if the door is blocked, stop after a few tries
							AddToForbiddenAreas(owner, frobDoor);
							return true;
						}

						if (masterUser == owner)
						{
							if (AssessStoppedDoor(frobDoor,(frobDoor->GetLastBlockingEnt() == owner)))
							{
								return true; // alerted, and will be searching
							}

							// try closing the door and opening it again
							
							if (_frontPosEnt.GetEntity())
							{
								// Using a controller. Go use it.
								_doorHandlingState = EStateApproachingDoor;
							}
							else
							{
								// Deal with the door directly, w/o going anywhere.
								Turn(currentPos,frobDoor,NULL); // grayman #3643
								_waitEndTime = gameLocal.time + TURN_TOWARD_DELAY;
								_doorHandlingState = EStateRetryInterruptedOpen1;
							}
						}
					}
				}
				//check interrupted
				else if (frobDoor->WasInterrupted())
				{
					// grayman #3523 - We come here if the player frobs the opening door
					// while it's moving.

					if (FitsThrough() && ( masterUser == owner ) )
					{
						if (owner->AI_AlertIndex >= ESearching)
						{
							return true;
						}

						// gap is large enough, move to back position
						PickWhere2Go(frobDoor);
					}
					else if (!AllowedToOpen(owner))
					{
						AddToForbiddenAreas(owner, frobDoor);
						return true;
					}
					else if (masterUser == owner)
					{
						// you don't fit through the opening

						if (AssessStoppedDoor(frobDoor,(frobDoor->GetLastBlockingEnt() == owner)))
						{
							return true; // alerted, and will be searching
						}

						if (_frontPosEnt.GetEntity())
						{
							// Using a controller. Go use it.
							_doorHandlingState = EStateApproachingDoor;
						}
						else
						{
							// Deal with the door directly, w/o going anywhere.
							Turn(currentPos,frobDoor,NULL); // grayman #3643
							_waitEndTime = gameLocal.time + TURN_TOWARD_DELAY;
							_doorHandlingState = EStateRetryInterruptedOpen1;
						}
					}
				}
				// when door is fully open, let's get moving
				else if	(!frobDoor->IsChangingState() && ( masterUser == owner ) )
				{
					PickWhere2Go(frobDoor); // grayman #2345 - recheck if you should continue to midPos
				}
			}
			break;
		case EStateMovingToMidPos:
			if (!frobDoor->IsOpen()) // closed
			{
				// door has closed while we were walking through it.
				// end this task (it will be initiated again if we are still in front of the door).
				return true;
			}
			else // open
			{
				// grayman #3104 - when searching, an AI can get far from the
				// door as he searches, but he still thinks he's handling a door
				// because he hasn't yet reached the mid position. If he wanders
				// too far while searching or in combat mode, quit door handling

				if ( owner->IsSearching() )
				{
					// We can assume the AI has wandered off if his distance
					// from the mid position is less than his distance to the
					// door center, and he's beyond a threshold distance.
					
					float dist2Goal = ( _midPos - ownerOrigin ).LengthFast();
					float dist2Door = ( centerPos - ownerOrigin).LengthFast();
					if ( ( dist2Door > QUEUE_DISTANCE ) && ( dist2Door > dist2Goal ) )
					{
						return true;
					}
				}
	
				if (_canHandleDoor)
				{
					// check blocked
					if (frobDoor->IsBlocked() || 
						(frobDoor->WasInterrupted() && frobDoor->WasStoppedDueToBlock()))
					{
						if (frobDoor->GetLastBlockingEnt() == owner)
						{
							if (!owner->m_bCanOperateDoors) // grayman #2345
							{
								return true; // can't operate a door
							}

							// we are blocking the door
							owner->StopMove(MOVE_STATUS_DONE);
							if (masterUser == owner)
							{
								Turn(closedPos,frobDoor,_frontPosEnt.GetEntity()); // grayman #3643

								if (!OpenDoor())
								{
									return true;
								}
								break; // grayman #3390
							}
							else
							{
								// I'm NOT the master, so I can't handle the door
								MoveToSafePosition(frobDoor); // grayman #3390
							}
						}
					}
					else if (frobDoor->WasInterrupted() && !FitsThrough())
					{
						// grayman #3390 - The door is partly open, and I can't fit through
						// the opening. Since I'm the master, it's my responsibility to
						// get the door fully opened
						if (!owner->MoveToPosition(_frontPos,owner->AI_RUN ? HANDLE_DOOR_ACCURACY_RUNNING : HANDLE_DOOR_ACCURACY))
						{
							// TODO: position not reachable, need a better one
						}
						_doorHandlingState = EStateMovingToFrontPos;
						break;
					}
				}
				
				// reached mid position?
				if (owner->AI_MOVE_DONE)
				{
					owner->SetMoveAccuracy(owner->AI_RUN ? HANDLE_DOOR_ACCURACY_RUNNING : HANDLE_DOOR_ACCURACY); // grayman #4039
					if (owner->ReachedPos(_midPos, MOVE_TO_POSITION) || (owner->GetTactileEntity() != NULL)) // grayman #2345
					{
						return true;
					}

					if ( !owner->IsSearching() ) // grayman #3104 - it's ok to stand still while searching
					{
						owner->MoveToPosition(_midPos,HANDLE_DOOR_ACCURACY);
					}
				}
				else
				{
					//owner->PrintGoalData(_midPos, 5);
					if (_canHandleDoor) // grayman #2712
					{
						PickWhere2Go(frobDoor); // grayman #2345 - recheck if you should continue to midPos
					}
				}
			}
			break;
		case EStateMovingToBackPos:
			if (!frobDoor->IsOpen()) // closed
			{
				// door has closed while we were walking through it.
				// end this task (it will be initiated again if we are still in front of the door).
				return true;
			}
			else // open
			{
				// grayman #3104 - when searching, an AI can get far from the
				// door as he searches, but he still thinks he's handling a door
				// because he hasn't yet reached the back position. If he wanders
				// too far while searching or in combat mode, quit door handling

				if ( owner->IsSearching() )
				{
					// We can assume the AI has wandered off if his distance
					// from the back position is less than his distance to the
					// door center, and he's beyond a threshold distance.
					
					float dist2Goal = ( _backPos - ownerOrigin ).LengthFast();
					float dist2Door = ( centerPos - ownerOrigin).LengthFast();
					if ( ( dist2Door > QUEUE_DISTANCE ) && ( dist2Door > dist2Goal ) )
					{
						return true;
					}
				}

				// check blocked

				if (frobDoor->IsBlocked() || 
					(frobDoor->WasInterrupted() && frobDoor->WasStoppedDueToBlock()))
				{
					if (frobDoor->GetLastBlockingEnt() == owner)
					{
						if (!owner->m_bCanOperateDoors) // grayman #2345
						{
							return true; // can't operate a door
						}

						// we are blocking the door

						if ( !memory.closeSuspiciousDoor ) // grayman #2866
						{
							owner->StopMove(MOVE_STATUS_DONE);
							Turn(closedPos,frobDoor,_frontPosEnt.GetEntity()); // grayman #3643

							if (masterUser == owner)
							{
								if (!OpenDoor())
								{
									return true;
								}
							}
						}
						else
						{
							owner->StopMove(MOVE_STATUS_DONE);
						}
					}
				}
				else
				{
					if ( !memory.closeSuspiciousDoor ) // grayman #2866
					{
						if (frobDoor->WasInterrupted())
						{
							// grayman #3745
							float dist2FrontPosSqr = (ownerOrigin - _frontPos).LengthSqr();
							float dist2BackPosSqr = (ownerOrigin - _backPos).LengthSqr();
							bool onFrontSide = (dist2BackPosSqr > dist2FrontPosSqr);

							if (onFrontSide && !FitsThrough())
							{
								// end this task, it will be reinitialized when needed
								//return true;

								// grayman #3390 - instead of leaving the queue, stay in it
								// and move away to a safe distance. Relinquish your master
								// position if you're the master.

								MoveToSafePosition(frobDoor); // grayman #3390
								break;
							}
						}
					}
				}

				if (owner->AI_MOVE_DONE)
				{
					owner->SetMoveAccuracy(HANDLE_DOOR_ACCURACY); // grayman #4039
					if (owner->ReachedPos(_backPos, MOVE_TO_POSITION) || // grayman #2345 #2692 - are we close enough to reach around a blocking AI?
						(tactileEntity && tactileEntity->IsType(idAI::Type) && (closedPos - ownerOrigin).LengthFast() < 100))
					{
						if (!AllowedToClose(owner) || owner->AI_RUN)  // grayman #2670
						{
							return true;
						}

						bool closeDoor = false;
						if (numUsers < 2)
						{
							bool shouldBeLocked = (_wasLocked || frobDoor->GetWasFoundLocked() || frobDoor->spawnArgs.GetBool("should_always_be_locked", "0")); // grayman #3523
							if (_doorInTheWay || owner->ShouldCloseDoor(frobDoor,shouldBeLocked) || memory.closeSuspiciousDoor ) // grayman #2866
							{
								// close the door
								owner->StopMove(MOVE_STATUS_DONE);
								Turn(openPos,frobDoor,_backPosEnt.GetEntity()); // grayman #3643

								_waitEndTime = gameLocal.time + 650;

								// grayman #3523 - clean up any requests we made to
								// push the player while the door was moving

								//owner->GetMemory().blockedDoorCount = 0;	// grayman #3523; grayman #4830
								if (frobDoor->IsPushingHard())
								{
									frobDoor->StopPushingDoorHard();
								}

								_doorHandlingState = EStateWaitBeforeClose;
								closeDoor = true;
							}
						}

						if (!closeDoor)
						{
							return true;
						}
					}
					else if ( !owner->IsSearching() ) // grayman #3104 - it's ok to stand still while searching
					{
						owner->MoveToPosition(_backPos,HANDLE_DOOR_ACCURACY); // grayman #2345 - need more accurate AI positioning
					}
				}
				else // grayman #2712
				{
					//owner->PrintGoalData(_backPos, 6);
					PickWhere2Go(frobDoor); // grayman #2345 - recheck if you should continue to _backPos

					// grayman #3643 - if you've changed state, you might
					// have an opportunity to leave the door

					if (_doorHandlingState == EStateMovingToMidPos)
					{
						// You've been called off of shutting the door, and told
						// to head for _midPos. But if you're past the door already
						// just exit the task and move on.

						// Compare the door side you're on now with the side you started with.

						idVec3 mid0 = frobDoor->GetDoorPosition(DOOR_SIDE_FRONT,DOOR_POS_MID);
						idVec3 mid1 = frobDoor->GetDoorPosition(DOOR_SIDE_BACK,DOOR_POS_MID);
						idVec3 ownerOrig = owner->GetPhysics()->GetOrigin();
						int currentDoorSide;

						if ( (mid0 - ownerOrig).LengthSqr() > (mid1 - ownerOrig).LengthSqr() )
						{
							currentDoorSide = DOOR_SIDE_FRONT;
						}
						else
						{
							currentDoorSide = DOOR_SIDE_BACK;
						}

						if (_doorSide != currentDoorSide)
						{
							// you're past the door, so you can leave
							return true;
						}
					}
				}
			}
			break;
		case EStateRetryInterruptedClose1: // grayman #3523
			// If here, you need to open the door and try closing it again.

			if (!frobDoor->IsOpen()) // closed
			{
				_doorHandlingState = EStateClosingDoor; // already closed, no need for a retry open
			}
			else // open
			{
				if (gameLocal.time >= _waitEndTime)
				{
					StartHandAnim(owner, _backPosEnt.GetEntity()); // grayman #3643
					//owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Use_righthand", 4);
					_doorHandlingState = EStateRetryInterruptedClose2;
					_waitEndTime = gameLocal.time + _useDelay; // grayman #3755
				}
			}
			break;
		case EStateRetryInterruptedClose2: // grayman #3523

			// If here, you need to fully open a partially open door and try closing it again.

			if (!frobDoor->IsOpen()) // closed
			{
				_doorHandlingState = EStateClosingDoor; // already closed, no need to open it
			}
			else // open
			{
				if (gameLocal.time >= _waitEndTime)
				{
					OpenDoor();
					_waitEndTime = gameLocal.time + 2000;
					_doorHandlingState = EStateRetryInterruptedClose3;
				}
			}
			break;
		case EStateRetryInterruptedClose3: // grayman #3523
			if (!frobDoor->IsOpen()) // closed
			{
				_doorHandlingState = EStateClosingDoor; // already closed, no need to close it
			}
			else // open
			{
				if (frobDoor->IsAtOpenPosition())
				{
					// fully open, now try closing it again
					_doorHandlingState = EStateWaitBeforeClose;
				}
				else if (gameLocal.time >= _waitEndTime)
				{
					// grayman #3523 - need to do something, because
					// the door isn't opening. Maybe it's been interrupted again?
					if (frobDoor->IsBlocked() || 
						frobDoor->WasInterrupted() || 
						frobDoor->WasStoppedDueToBlock())
					{
						// Door was trying to open, and it stopped.
						// Try opening it again.
						Turn(currentPos,frobDoor,_backPosEnt.GetEntity()); // grayman #3643

						_waitEndTime = gameLocal.time + TURN_TOWARD_DELAY;
						_doorHandlingState = EStateRetryInterruptedClose2;
					}
				}
			}
			break;
		case EStateRetryInterruptedClose4: // grayman #3726
			// If here, you need to open the door and walk away.

			if (!frobDoor->IsOpen()) // closed
			{
				_doorHandlingState = EStateClosingDoor; // already closed, no need for a retry open
			}
			else // open
			{
				if (gameLocal.time >= _waitEndTime)
				{
					StartHandAnim(owner, _backPosEnt.GetEntity()); // grayman #3643
					//owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Use_righthand", 4);
					_doorHandlingState = EStateRetryInterruptedClose5;
					_waitEndTime = gameLocal.time + _useDelay; // grayman #3755
				}
			}
			break;
		case EStateRetryInterruptedClose5: // grayman #3726

			// If here, you need to fully open a partially open door and then walk away.

			if (!frobDoor->IsOpen()) // closed
			{
				_doorHandlingState = EStateClosingDoor; // already closed, no need to open it
			}
			else // open
			{
				if (gameLocal.time >= _waitEndTime)
				{
					OpenDoor();
					_waitEndTime = gameLocal.time + 2000;
					_doorHandlingState = EStateRetryInterruptedClose6;
				}
			}
			break;
		case EStateRetryInterruptedClose6: // grayman #3726
			if (!frobDoor->IsOpen()) // closed
			{
				_doorHandlingState = EStateClosingDoor; // already closed, no need to close it
			}
			else // open
			{
				if (frobDoor->IsAtOpenPosition())
				{
					// fully open, now walk away
					return true;
				}

				if (gameLocal.time >= _waitEndTime)
				{
					// door isn't fully open, so give up
					return true;
				}
			}
			break;
		case EStateWaitBeforeClose:
			if (!frobDoor->IsOpen()) // closed
			{
				// door has already closed before we were attempting to do it
				// no need for more waiting
				return true;
			}
			else // open
			{
				if (!AllowedToClose(owner) ||
					(!_doorInTheWay && (owner->AI_AlertIndex >= ESearching)) ||
					 owner->AI_RUN) // grayman #2670
				{
					return true;
				}

				if ( (gameLocal.time >= _waitEndTime) && ( (numUsers < 2) || _doorInTheWay) )
				{
					if (masterUser == owner)
					{
						StartHandAnim(owner, _backPosEnt.GetEntity()); // grayman #3643
						//owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Use_righthand", 4);
						//owner->SetWaitState("use_righthand"); // grayman #3643
						_doorHandlingState = EStateStartClose;
						_waitEndTime = gameLocal.time + _useDelay; // grayman #3755
					}
				}
				else if ( (numUsers > 1) && !_doorInTheWay)
				{
					return true;
				}
			}
			break;
		case EStateStartClose:
			if (!frobDoor->IsOpen()) // closed
			{
				// door has already closed before we were attempting to do it
				// no need for more waiting
				return true;
			}
			else // open
			{
				if (!AllowedToClose(owner) ||
					(!_doorInTheWay && (owner->AI_AlertIndex >= ESearching)) ||
					 owner->AI_RUN) // grayman #2670
				{
					return true;
				}

				if ( ( gameLocal.time >= _waitEndTime ) && ( ( numUsers < 2 ) || _doorInTheWay))
				{
					CloseDoor(owner,frobDoor,_backPosEnt.GetEntity());
					_doorHandlingState = EStateClosingDoor;
					_waitEndTime = gameLocal.time + WAIT_FOR_DOOR_TO_START_MOVING; // grayman #4077
				}
				else if ( ( numUsers > 1 ) && !_doorInTheWay)
				{
					// grayman #3643 - wait for the raised hand anim to finish,
					// to keep the AI from turning and walking away with his
					// hand still raised, which doesn't look good

					if ( (idStr(owner->WaitState()) != "use_righthand") &&
						 (idStr(owner->WaitState()) != "controller") )
					{
						return true;
					}
				}
			}
			break;
		case EStateClosingDoor:
			if (!frobDoor->IsOpen()) // closed
			{
				// we have moved through the door and closed it

				// If the door should ALWAYS be locked or it was locked before => lock it
				// but only if the owner is able to unlock it in the first place
				if (owner->CanUnlock(frobDoor) && AllowedToLock(owner) &&
					(_wasLocked || frobDoor->GetWasFoundLocked() || frobDoor->spawnArgs.GetBool("should_always_be_locked", "0"))) // grayman #3104
				{
					// If the door uses controllers, the lock is kept on
					// them, not on the door
					if ( (frobDoor->GetDoorController(DOOR_SIDE_FRONT).GetEntity() != NULL) ||
						 (frobDoor->GetDoorController(DOOR_SIDE_BACK).GetEntity()  != NULL) )
					{
						frobDoor->LockControllers(false); // lock all controllers
					}
					else // lock door directly
					{
						frobDoor->Lock(false); // lock the door
					}
				}

				if (doubleDoor != NULL)
				{
					// If the other door is open, you need to close it.
					//
					// grayman #2732 - If it's closed, and needs to be locked, lock it.

					if (doubleDoor->IsOpen())
					{
						// the other part of the double door is still open
						// we want to close this one too
						ResetDoor(owner, doubleDoor);
						owner->MoveToPosition(_backPos,HANDLE_DOOR_ACCURACY); // grayman #2345 - need more accurate AI positioning
						_doorHandlingState = EStateMovingToBackPos;
						break;
					}
					else
					{
						if (owner->CanUnlock(doubleDoor) && AllowedToLock(owner) &&
							(_wasLocked || doubleDoor->GetWasFoundLocked() || doubleDoor->spawnArgs.GetBool("should_always_be_locked", "0"))) // grayman #3104
						{
							// If the door uses controllers, the lock is kept on
							// them, not on the door
							if ( (doubleDoor->GetDoorController(DOOR_SIDE_FRONT).GetEntity() != NULL) ||
								 (doubleDoor->GetDoorController(DOOR_SIDE_BACK).GetEntity()  != NULL) )
							{
								doubleDoor->LockControllers(false); // lock all controllers
							}
							else // lock door directly
							{
								doubleDoor->Lock(false); // lock the seconddoor
							}
						}
					}
				}

				// continue what we were doing before.
				return true;
			}
			else // open
			{
				if (!AllowedToClose(owner) ||
					(owner->AI_AlertIndex >= ESearching) ||
					 owner->AI_RUN) // grayman #2670
				{
					return true;
				}

				// If the door is open only a little bit, you can walk away from it.
				// Can't walk away if you're supposed to lock the door, because that
				// only happens when the door is closed.

				bool doorAlmostClosed = ( frobDoor->GetFractionalPosition() < CAN_LEAVE_DOOR_FRACTION );
				if ( doorAlmostClosed )
				{
					bool wait2LockDoor = ( (owner->CanUnlock(frobDoor) && AllowedToLock(owner) &&
											(_wasLocked || frobDoor->GetWasFoundLocked() || frobDoor->spawnArgs.GetBool("should_always_be_locked", "0")))); // grayman #3104
					bool wait2LockDoubleDoor = ( (doubleDoor != NULL) &&
													(owner->CanUnlock(frobDoor) && AllowedToLock(owner) &&
													(_wasLocked || frobDoor->GetWasFoundLocked() || frobDoor->spawnArgs.GetBool("should_always_be_locked", "0")))); // grayman #3104
					if (!wait2LockDoor && !wait2LockDoubleDoor )
					{
						return true; // leave door early
					}
				}
					
				// check blocked or interrupted
				if (frobDoor->IsBlocked() || 
					frobDoor->WasInterrupted() || 
					frobDoor->WasStoppedDueToBlock())
				{
					// Ignore block if door is almost closed, regardless
					// of whether it has to be relocked or not

					if ( doorAlmostClosed )
					{
						return true; // leave door early
					}

					// grayman #3523 - This is where the door is stopped as the AI is closing it.
					// The door is closing toward the AI or away from the AI, and either something blocks it
					// or the player frobs it.

					// Should owner become concerned that the door was interrupted?

					if (AssessStoppedDoor(frobDoor,(frobDoor->GetLastBlockingEnt() == owner)))
					{
						return true; // alerted, and will be searching
					}

					// grayman #3726 - if you've tried opening and closing enough
					// times, and you still can't close it, leave it open and walk away
					if (owner->GetMemory().blockedDoorCount < (PUSH_PLAYER_ON_THIS_ATTEMPT + 1) ) // grayman #4830
					{
						// try opening the door and closing it again
						Turn(currentPos,frobDoor,_backPosEnt.GetEntity()); // grayman #3643
						_waitEndTime = gameLocal.time + TURN_TOWARD_DELAY;
						_doorHandlingState = EStateRetryInterruptedClose1;
					}
					else
					{
						// try opening the door
						_doorHandlingState = EStateRetryInterruptedClose4;
					}
				}

				// grayman #4077 - is the door moving yet?
				else if ( ( gameLocal.time >= _waitEndTime ) && !frobDoor->IsMoving() ) // grayman #4830 - add an 'else'
				{
					_doorHandlingState = EStateWaitBeforeClose;
				}
			}
			break;
		default:
			break;
	}

	// grayman #2700 - set the door use timeout

	if ((_doorHandlingState == EStateMovingToMidPos) || (_doorHandlingState == EStateMovingToBackPos))
	{
		if (_leaveDoor < 0)
		{
			_leaveDoor = gameLocal.time + DOOR_TIMEOUT; // grayman #2700 - set door use timeout
		}
	}
	else
	{
		_leaveDoor = -1; // reset timeout
	}

	return false; // not finished yet
}

void HandleDoorTask::ResetDoor(idAI* owner, CFrobDoor* newDoor)
{
	Memory& memory = owner->GetMemory();

	// reset the active door to this door					
	memory.doorRelated.currentDoor = newDoor;

	idAngles rotate = newDoor->spawnArgs.GetAngles("rotate", "0 90 0"); // grayman #3643
	_rotates = ( (rotate.yaw != 0) || (rotate.pitch != 0) || (rotate.roll != 0) );

	InitDoorPositions(owner, newDoor, false);

	if (_doorHandlingState >= EStateRetryInterruptedClose1) // grayman #3643 - all states from here on
	{
		// we have already walked through, so we are standing on the back side,
		// so swap positions

		idVec3 tempPos = _frontPos;
		_frontPos = _backPos;
		_backPos = tempPos;
		_midPos = _backPos; // don't think this gets used when already beyond the door

		idEntityPtr<idEntity> tempEntityPtr;
		tempEntityPtr = _frontPosEnt; // front door controller
		_frontPosEnt = _backPosEnt;  // back door controller
		_backPosEnt = tempEntityPtr;

		tempEntityPtr = _frontDHPosition; // front door controller
		_frontDHPosition = _backDHPosition; // front door handle position
		_backDHPosition = tempEntityPtr;  // back door handle position
	}
}

// grayman #720 - FitsThrough() tries to fit the AI through
// from a head-on direction, which doesn't care about wall thickness. Doors
// need to be more open for the AI to fit through, but it no longer gives
// false positives.

bool HandleDoorTask::FitsThrough()
{
	// This calculates the gap left by a partially open door
	// and checks if it is large enough for the AI to fit through it.

	idAI* owner = _owner.GetEntity();
	Memory& memory = owner->GetMemory();
	CFrobDoor* frobDoor = memory.doorRelated.currentDoor.GetEntity();

	idBounds bounds = owner->GetPhysics()->GetBounds();
	float size = 2*bounds[1][0] + 8;

	if (_rotates)
	{
		idAngles tempAngle;
		idPhysics_Parametric* physics = frobDoor->GetMoverPhysics();
		physics->GetLocalAngles(tempAngle);

		const idVec3& closedPos = frobDoor->GetClosedPos();
		idVec3 dir = closedPos;
		dir.z = 0;
		float dist = dir.LengthFast();

		idAngles alpha = frobDoor->GetClosedAngles() - tempAngle;
		float absAlpha = idMath::Fabs(alpha.yaw);
		float delta = dist*(1.0 - idMath::Fabs(idMath::Cos(DEG2RAD(absAlpha))));

		return (delta >= size);
	}

	// grayman #3643 - sliding door

	idVec3 origin = frobDoor->GetMoverPhysics()->GetOrigin(); // where origin is now
	idVec3 closedOrigin = frobDoor->GetClosedOrigin(); // where origin is when door is closed
	idVec3 delta = closedOrigin - origin;
	delta.x = idMath::Fabs(delta.x);
	delta.y = idMath::Fabs(delta.y);
	delta.z = idMath::Fabs(delta.z);
	if ( delta.x > 0 )
	{
		if (delta.x >= size)
		{
			return true; // assume vertical fit
		}
	}
	else if (delta.y > 0)
	{
		if (delta.y >= size) // assume vertical fit
		{
			return true;
		}
	}
	else if (delta.z > 0)
	{
		if (delta.z >= 96) // 96 = AI height
		{
			return true; // assume horizontal fit
		}
	}

	return false;
}

bool HandleDoorTask::OpenDoor()
{
	idAI* owner = _owner.GetEntity();
	Memory& memory = owner->GetMemory();
	CFrobDoor* frobDoor = memory.doorRelated.currentDoor.GetEntity();

	// Update our door info structure
	DoorInfo& doorInfo = memory.GetDoorInfo(frobDoor);
	//doorInfo.lastTimeSeen = gameLocal.time; // grayman #3755 - not used
	doorInfo.lastTimeTriedToOpen = gameLocal.time;
	doorInfo.wasLocked = frobDoor->IsLocked();
	bool shouldOpenDoor = false; // grayman #3643
	bool doorWasLocked = false; // grayman #3643

	idEntityPtr<idEntity> controllerPtr; // door controller

	int currentDoorSide = owner->GetDoorSide(frobDoor, owner->GetPhysics()->GetOrigin()); // grayman #4830
	controllerPtr = frobDoor->GetDoorController(currentDoorSide); // grayman #4830
	//controllerPtr = frobDoor->GetDoorController(_doorSide); // grayman #4830

	if (controllerPtr.GetEntity() == NULL) // grayman #3643
	{
		// dealing directly with the door

		if (frobDoor->IsLocked())
		{
			doorWasLocked = true; // grayman #3643
			if (!owner->CanUnlock(frobDoor) || !AllowedToUnlock(owner))
			{
				// Door is locked and we cannot unlock it
				// Check if we can open the other part of a double door
				CFrobDoor* doubleDoor = frobDoor->GetDoubleDoor();
				if ( ( doubleDoor != NULL ) && (!doubleDoor->IsLocked() || owner->CanUnlock(doubleDoor)))
				{
					ResetDoor(owner, doubleDoor);
					if (AllowedToUnlock(owner))
					{
						_doorHandlingState = EStateMovingToFrontPos;
						return true;
					}

					return false;
				}

				owner->StopMove(MOVE_STATUS_DONE);
				// Rattle the door once
				frobDoor->Open(true);
				
				// add AAS area number of the door to forbidden areas
				AddToForbiddenAreas(owner, frobDoor);
				return false;
			}

			// unlock the door directly
			shouldOpenDoor = frobDoor->Unlock(true); // grayman #3643
			doorInfo.wasLocked = frobDoor->IsLocked(); // for use by subsequent AI until the door is closed
		}
	}
	else // dealing with a controller
	{
		CBinaryFrobMover* controller = static_cast<CBinaryFrobMover*>(controllerPtr.GetEntity());
		if (controller->IsLocked())
		{
			doorWasLocked = true; // grayman #3643
			if (!owner->CanUnlock(frobDoor) || !AllowedToUnlock(owner))
			{
				owner->StopMove(MOVE_STATUS_DONE);
				// Can't unlock the controller, but rattle it once
				controller->Open(true);
				
				// add AAS area number of the door to forbidden areas
				AddToForbiddenAreas(owner, frobDoor);
				return false;
			}
			else
			{
				// Unlock the controllers. The first one to unlock will open the door.
				shouldOpenDoor = frobDoor->UnlockControllers(true);
				doorInfo.wasLocked = frobDoor->IsLocked(); // for use by subsequent AI until the door is closed
			}
		}
	}

	frobDoor->SetLastUsedBy(owner); // grayman #2859
	owner->StopMove(MOVE_STATUS_DONE);

	// grayman #3643 - If the door was locked and the Unlock() above automatically
	// opened the door, the door will be opened when the unlocking sound is finished.
	// If that's what's happening, we can't issue an Open() request here, because it would cause the
	// door to start moving now, and the delayed ToggleOpen() queued by the Unlock() would
	// toggle the door to a stop, causing AI to believe the door movement was interrupted.

	// This wasn't a problem before 2.02 because when the door was interrupted, the AI
	// immediately issued another Open() request. The interruption was not visible to the
	// player because the door was only stopped for a max of 4 frames. In 2.02, we want the
	// AI to pay attention to interruptions, so they can't issue another Open() request
	// immediately. In this case, the player would see the door stop for no apparent reason.

	// So we register whether Unlock() set up a delayed ToggleOpen() request, and if so, we don't do it here.

	// This only applies to doors that don't use controllers. A door that uses controllers
	// should have the lock set on them, not on the door, since the controller is where
	// you want the player to use his key.

	if (!doorWasLocked || shouldOpenDoor)
	{
		// Either the door was not locked or it was but the Unlock() didn't automatically open it.
		// In either case, open the door here.
		if (controllerPtr.GetEntity() == NULL) // grayman #3643
		{
			// grayman #3755 - if in a hurry, bang the door open
			if ( !frobDoor->IsPushingHard() && ( owner->IsSearching() || owner->AI_RUN ) )
			{
				frobDoor->PushDoorHard();
			}
			frobDoor->Open(true);
		}
		else // use controller to open the door
		{
			// Trigger the frob action script
			controllerPtr.GetEntity()->FrobAction(true);
		}
	}
	else
	{
		// Door was locked and the unlock set up a delayed ToggleOpen() request,
		// so you don't need to do it here.
	}

	_doorHandlingState = EStateOpeningDoor;
	_waitEndTime = gameLocal.time + 1000; // grayman #2862 - a short wait to allow the door to begin opening
	return true;
}

// grayman #3643 - Close door with a controller if requested,
// otherwise just ask the door to close. Locking is not handled until
// the door shuts, so we don't have to deal with it here.

void HandleDoorTask::CloseDoor(idAI* owner, CFrobDoor* frobDoor, idEntity* controller)
{
	frobDoor->SetLastUsedBy(owner); // grayman #2859
	if (controller == NULL) // grayman #3643
	{
		frobDoor->Close(true);
	}
	else // door controlled by switches
	{
		// Trigger the frob action script
		controller->FrobAction(true);
	}
}

void HandleDoorTask::DoorInTheWay(idAI* owner, CFrobDoor* frobDoor)
{
	// grayman #3643 - TODO: Handle sliding door

	_doorInTheWay = true;
	// check if the door swings towards or away from us
	const idVec3& openDir = frobDoor->GetOpenDir();
	const idVec3& frobDoorOrg = frobDoor->GetPhysics()->GetOrigin();
	const idVec3& closedPos = frobDoorOrg + frobDoor->GetClosedPos();
	const idVec3& openPos = frobDoorOrg + frobDoor->GetOpenPos();

	if (openDir * (owner->GetPhysics()->GetOrigin() - frobDoorOrg) > 0)
	{
		// Door opens towards us
		idVec3 closedDir = closedPos - frobDoorOrg;
		closedDir.z = 0;
		idVec3 org = owner->GetPhysics()->GetOrigin();
		idVec3 ownerDir = org - frobDoorOrg;
		ownerDir.z = 0;
		idVec3 frontPosDir = _frontPos - frobDoorOrg;
		frontPosDir.z = 0;

		float l1 = closedDir * ownerDir;
		float l2 = closedDir * frontPosDir;

		if (l1 * l2 < 0)
		{	
			const idBounds& bounds = owner->GetPhysics()->GetBounds();
			float size = bounds[1][0];

			// can't reach standard position
			idVec3 parallelOffset = openPos - frobDoorOrg;
			parallelOffset.z = 0;
			float len = parallelOffset.LengthFast();
			parallelOffset.NormalizeFast();
			parallelOffset *= len - 1.2f * size;

			idVec3 normalOffset = closedPos - frobDoorOrg;
			normalOffset.z = 0;
			normalOffset.NormalizeFast();
			normalOffset *= 1.5f * size;

			if ( _closeFromSameSide ) // grayman #2866
			{
				_backPos = frobDoorOrg + parallelOffset - normalOffset;
			}
			else
			{
				_frontPos = frobDoorOrg + parallelOffset - normalOffset;
			}
		}
		
		if ( _closeFromSameSide ) // grayman #2866
		{
			owner->MoveToPosition(_backPos,HANDLE_DOOR_ACCURACY); // grayman #2345 - need more accurate AI positioning
		}
		else
		{
			owner->MoveToPosition(_frontPos,HANDLE_DOOR_ACCURACY); // grayman #2345 - need more accurate AI positioning
		}
	}
	else
	{
		//Door opens away from us
		PickWhere2Go(frobDoor); // grayman #2712
	}
}

bool HandleDoorTask::AllowedToOpen(idAI* owner)
{
	idEntity* frontDHPosition = _frontDHPosition.GetEntity();

	if (frontDHPosition && frontDHPosition->spawnArgs.GetBool("ai_no_open"))
	{
		// AI is not allowed to open the door from this side
		return false;
	}
	return true;
}

bool HandleDoorTask::AllowedToClose(idAI* owner)
{
	idEntity* backDHPosition = _backDHPosition.GetEntity();

	if (!owner->m_bCanCloseDoors || (backDHPosition && backDHPosition->spawnArgs.GetBool("ai_no_close")))
	{
		// AI is not allowed to close the door from this side
		return false;
	}
	return true;
}

bool HandleDoorTask::AllowedToUnlock(idAI* owner)
{
	idEntity* frontDHPosition = _frontDHPosition.GetEntity();

	if (frontDHPosition && frontDHPosition->spawnArgs.GetBool("ai_no_unlock"))
	{
		// AI is not allowed to unlock the door from this side
		return false;
	}
	return true;
}

bool HandleDoorTask::AllowedToLock(idAI* owner)
{
	idEntity* backDHPosition = _backDHPosition.GetEntity();

	if (backDHPosition && backDHPosition->spawnArgs.GetBool("ai_no_lock"))
	{
		// AI is not allowed to lock the door from this side
		return false;
	}
	return true;
}

void HandleDoorTask::AddToForbiddenAreas(idAI* owner, CFrobDoor* frobDoor)
{
	// add AAS area number of the door to forbidden areas
	idAAS*	aas = owner->GetAAS();
	if (aas != NULL)
	{
		int areaNum = frobDoor->GetAASArea(aas);
		gameLocal.m_AreaManager.AddForbiddenArea(areaNum, owner);
		owner->PostEventMS(&AI_ReEvaluateArea, owner->doorRetryTime, areaNum);
		frobDoor->RegisterAI(owner); // grayman #1145 - this AI is interested in this door

		// grayman #1327 - terminate a hiding spot search if one is going on. The AI
		// tried to get through this door to get to a spot, but since he can't reach
		// it, he should get another spot.
		// grayman #3857 - also terminate a search if guarding a spot for that search,
		// or milling about at the start of a search

		Memory& memory = owner->GetMemory();
		if ( memory.hidingSpotInvestigationInProgress || memory.guardingInProgress || memory.millingInProgress ) // grayman #3857
		{
			memory.stopHidingSpotInvestigation = true;
			memory.stopGuarding = true;
			memory.stopMilling = true;
			memory.currentSearchSpot = idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY);
		}
	}
}

// grayman - for debugging door queues

#if 0
void PrintDoorQ(CFrobDoor* frobDoor)
{
	DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("     %s's door queue ...\r", frobDoor->name.c_str());
	int n = frobDoor->GetUserManager().GetNumUsers();
	if ( n == 0 )
	{
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("     is EMPTY\r");
		return;
	}

	idVec3 doorOrigin = frobDoor->GetPhysics()->GetOrigin();
	for ( int j = 0 ; j < n ; j++ )
	{
		idActor* u = frobDoor->GetUserManager().GetUserAtIndex(j);
		if ( u != NULL )
		{
			idVec3 dir = u->GetPhysics()->GetOrigin() - doorOrigin;
			dir.z = 0;
			float dist = dir.LengthFast();
			DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("     %s at %f\r", u->name.c_str(),dist);
		}
		else
		{
			DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("     NULL\r");
		}
	}
}
#endif

// grayman #2345 - when adding a door user, order the queue so that
// users still moving toward the door are in
// "distance from door" order. This handles the situation where a user
// who is close to the door is added after a user who is farther from
// the door. For example, an AI who is at the other end of a long hall,
// walking toward the door, and another AI comes around a corner near
// the door. The second AI should use the door first.

void HandleDoorTask::AddUser(idAI* newUser, CFrobDoor* frobDoor)
{
	int numUsers = frobDoor->GetUserManager().GetNumUsers();
	if (numUsers > 0)
	{
		frobDoor->GetUserManager().RemoveUser(newUser); // If newUser is already on the door queue, remove
														// them, because the queue is ordered by distance
														// from the door, and they need to be re-inserted
														// at the correct spot. If they're not already on
														// the queue, RemoveUser() does nothing.

		idVec3 centerPos = frobDoor->GetClosedBox().GetCenter();		// grayman #3104 - use center of closed door
		idVec3 dir = newUser->GetPhysics()->GetOrigin() - centerPos;	// grayman #3104 - use center of closed door
		dir.z = 0;
		float newUserDistanceSqr = dir.LengthSqr();
		float qSqr = Square(QUEUE_DISTANCE);
		for ( int i = 0 ; i < numUsers ; i++ )
		{
			idActor* user = frobDoor->GetUserManager().GetUserAtIndex(i);
			if ( user != NULL )
			{
				idVec3 userDir = user->GetPhysics()->GetOrigin() - centerPos; // grayman #3104 - use center of closed door
				userDir.z = 0;
				float userDistanceSqr = userDir.LengthSqr();
				if ( userDistanceSqr > qSqr ) // only cut in front of users not yet standing
				{
					if ( newUserDistanceSqr < userDistanceSqr ) // cut in front of a user farther away
					{
						frobDoor->GetUserManager().InsertUserAtIndex(newUser,i);
						
						//PrintDoorQ(frobDoor); // grayman - for debugging door queues

						return;
					}
				}
			}
		}
	}

	frobDoor->GetUserManager().AppendUser(newUser);

	//PrintDoorQ(frobDoor); // grayman - for debugging door queues
}

void HandleDoorTask::OnFinish(idAI* owner)
{
	Memory& memory = owner->GetMemory();
	CFrobDoor* frobDoor = memory.doorRelated.currentDoor.GetEntity();

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("HandleDoorTask finished - %s\r",owner->name.c_str());
	if (owner->m_HandlingDoor)
	{
		owner->m_HandlingDoor = false; // grayman #3647 - has to be done BEFORE the PopMove()
		if (owner->m_RestoreMove) // grayman #2690/#2712 - AI run toward where they saw you last. Don't save that location when handling doors.
		{
			owner->PopMove();
		}
		//owner->m_HandlingDoor = false; // grayman #3647 - has to be done BEFORE the PopMove()
	}

	_doorInTheWay = false;

	if (frobDoor != NULL) 
	{
		// Update our door info structure
		DoorInfo& doorInfo = memory.GetDoorInfo(frobDoor);
		//doorInfo.lastTimeSeen = gameLocal.time; // grayman #3755 - not used
		int timeCanUseAgain = gameLocal.time; // grayman #3755
		if (_doorHandlingState > EStateMovingToBackPos)
		{
			// only add a delay if the AI has actually gone
			// through the door
			timeCanUseAgain += REUSE_DOOR_DELAY; // grayman #2345 grayman #3755
		}
		doorInfo.timeCanUseAgain = timeCanUseAgain; // grayman #2345 grayman #3755
		doorInfo.wasLocked = frobDoor->IsLocked();
		doorInfo.wasOpen = frobDoor->IsOpen();

		frobDoor->GetUserManager().RemoveUser(owner);
		frobDoor->GetUserManager().ResetMaster(frobDoor); // grayman #2345/#2706 - redefine which AI is the master

		// grayman #3104 - If you're the last one through the
		// door, set whether it's locked or unlocked. This takes
		// care of the problem of a door that's left open because
		// the last AI through was called off the door before
		// closing it. We don't want the door's locked state
		// from that use to govern what happens the next time
		// the door is used.

		if ( frobDoor->GetUserManager().GetNumUsers() == 0 )
		{
			frobDoor->SetWasFoundLocked(frobDoor->IsLocked()); // will only be true if the door is closed and locked
		}

		//PrintDoorQ(frobDoor); // grayman - for debugging door queues

		CFrobDoor* doubleDoor = frobDoor->GetDoubleDoor();
		if (doubleDoor != NULL)
		{
			doubleDoor->GetUserManager().RemoveUser(owner);		// grayman #2345 - need to do for this what we did for a single door
			doubleDoor->GetUserManager().ResetMaster(doubleDoor);	// grayman #2345/#2706 - redefine which AI is the master

			// grayman #3104 - reset locked state for 2nd door

			if ( doubleDoor->GetUserManager().GetNumUsers() == 0 )
			{
				doubleDoor->SetWasFoundLocked(doubleDoor->IsLocked()); // will only be true if the door is closed and locked
			}
		}
		memory.lastDoorHandled = frobDoor; // grayman #2712

		// grayman #3755 - delete all this; these changes are made when the door stops moving
		// grayman #3748 - If the AI was pushing a door, the door
		// was not frobable, and it was pushing the player out of
		// the way if he was blocking it. It's time to reset the door.
		//if (frobDoor->IsPushingHard())
		//{
			//frobDoor->StopPushingDoorHard();
		//}
	}

	memory.doorRelated.currentDoor = NULL;

	if ( frobDoor && (memory.closeMe.GetEntity() == frobDoor) ) // grayman #4830
	{
		if ( memory.closeSuspiciousDoor ) // grayman #2866 - grayman #3104 - only forget suspicious door if it's the one I'm finishing now
		{
			memory.closeMe = NULL;
			memory.closeSuspiciousDoor = false;
			memory.blockedDoorCount = 0; // grayman #4830
			_closeFromSameSide = false;
			frobDoor->SetSearching(NULL);
			idAngles doorRotation = frobDoor->spawnArgs.GetAngles("rotate", "0 90 0");
			float angleAdjustment = 90;
			if ( doorRotation.yaw != 0 )
			{
				angleAdjustment = doorRotation.yaw;
			}
			owner->TurnToward(owner->GetCurrentYaw() - angleAdjustment); // turn away from the door
			owner->SetAlertLevel((owner->thresh_1 + owner->thresh_2) / 2.0f); // alert level is just below thresh_2 at this point, so this drops it down halfway to thresh_1
			frobDoor->AllowResponse(ST_VISUAL, owner); // grayman #3104 - respond again to this visual stim, in case the door was never closed
		}
	}
	else
	{
		memory.blockedDoorCount = 0;	  // grayman #4830
	}

	//_leaveDoor = -1; // reset timeout for leaving the door
	//_doorHandlingState = EStateNone;
	owner->m_canResolveBlock = true;  // grayman #2345
	memory.stopHandlingDoor = false;  // grayman #2816
	memory.latchPickedPocket = false; // grayman #3559 - free to react to a picked pocket
	owner->m_DoorQueued = false;	  // grayman #3647
}

bool HandleDoorTask::CanAbort() // grayman #2706
{
	return ( _doorHandlingState <= EStateMovingToSafePos2 );
}

void HandleDoorTask::DrawDebugOutput(idAI* owner)
{
	CFrobDoor* frobDoor = owner->GetMemory().doorRelated.currentDoor.GetEntity();
	if (frobDoor != NULL)
	{
		gameRenderWorld->DebugArrow(colorRed, owner->GetEyePosition(), frobDoor->GetPhysics()->GetOrigin(), 1, 70);
	}

	gameRenderWorld->DebugArrow(colorCyan, _frontPos, _frontPos + idVec3(0, 0, 20), 2, 1000);
	gameRenderWorld->DebugText("front", 
		(_frontPos + idVec3(0, 0, 30)), 
		0.2f, colorCyan, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 4 * USERCMD_MSEC);

	gameRenderWorld->DebugArrow(colorYellow, _backPos, _backPos + idVec3(0, 0, 20), 2, 1000);
	gameRenderWorld->DebugText("back", 
		(_backPos + idVec3(0, 0, 30)), 
		0.2f, colorYellow, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 4 * USERCMD_MSEC);

	gameRenderWorld->DebugArrow(colorPink, _midPos, _midPos + idVec3(0, 0, 20), 2, 1000);
	gameRenderWorld->DebugText("mid", 
		(_midPos + idVec3(0, 0, 30)), 
		0.2f, colorPink, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 4 * USERCMD_MSEC);

	/*	grayman #2345/#2706

	Show the door situation.

	Format is:
  
		<DoorHandlingState> <DoorName> - <Position in door queue>/<# users in door queue>

	For example:

		EStateMovingToFrontPos Door4 - 2/3

	means the AI is moving toward the front of the door and is currently in the second
	slot in a door queue of 3 users on door Door4.
	 */

	idStr position = "";
	if (owner->m_HandlingDoor)
	{
		if (frobDoor != NULL)
		{
			idStr doorName = frobDoor->name;
			int numUsers = frobDoor->GetUserManager().GetNumUsers();
			int slot = frobDoor->GetUserManager().GetIndex(owner) + 1;
			if (slot > 0)
			{
				position = " " + doorName + " - " + slot + "/" + numUsers;
			}
		}
		else
		{
			position = " (ERROR: no door)";
		}
	}

	idStr str;
	switch (_doorHandlingState)
	{
		case EStateNone:
			str = "EStateNone";
			break;
		case EStateApproachingDoor:
			str = "EStateApproachingDoor";
			break;
		case EStateMovingToSafePos1: // grayman #2345
			str = "EStateMovingToSafePos1";
			break;
		case EStateMovingToSafePos2: // grayman #3643
			str = "EStateMovingToSafePos2";
			break;
		case EStateMovingToFrontPos:
			str = "EStateMovingToFrontPos";
			break;
		case EStateRetryInterruptedOpen1: // grayman #3523
			str = "EStateRetryInterruptedOpen1";
			break;
		case EStateRetryInterruptedOpen2: // grayman #3523
			str = "EStateRetryInterruptedOpen2";
			break;
		case EStateRetryInterruptedOpen3: // grayman #3523
			str = "EStateRetryInterruptedOpen3";
			break;
		case EStateWaitBeforeOpen:
			str = "EStateWaitBeforeOpen";
			break;
		case EStateStartOpen:
			str = "EStateStartOpen";
			break;
		case EStateOpeningDoor:
			str = "EStateOpeningDoor";
			break;
		case EStateMovingToMidPos: // grayman #2345
			str = "EStateMovingToMidPos";
			break;
		case EStateMovingToBackPos:
			str = "EStateMovingToBackPos";
			break;
		case EStateRetryInterruptedClose1: // grayman #3523
			str = "EStateRetryInterruptedClose1";
			break;
		case EStateRetryInterruptedClose2: // grayman #3523
			str = "EStateRetryInterruptedClose2";
			break;
		case EStateRetryInterruptedClose3: // grayman #3523
			str = "EStateRetryInterruptedClose3";
			break;
		case EStateRetryInterruptedClose4: // grayman #3726
			str = "EStateRetryInterruptedClose4";
			break;
		case EStateRetryInterruptedClose5: // grayman #3726
			str = "EStateRetryInterruptedClose5";
			break;
		case EStateRetryInterruptedClose6: // grayman #3726
			str = "EStateRetryInterruptedClose6";
			break;
		case EStateWaitBeforeClose:
			str = "EStateWaitBeforeClose";
			break;
		case EStateStartClose:
			str = "EStateStartClose";
			break;
		case EStateClosingDoor:
			str = "EStateClosingDoor";
			break;
		default: // grayman #2345
			str = "";
			break;
	}
	str += position;

	gameRenderWorld->DebugText(str.c_str(), 
		(owner->GetEyePosition() - owner->GetPhysics()->GetGravityNormal()*60.0f), 
		0.25f, colorYellow, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 4 * USERCMD_MSEC);

	//Memory& memory = owner->GetMemory();
	//CFrobDoor* frobDoor = memory.doorRelated.currentDoor.GetEntity();
	if (frobDoor != NULL)
	{
		idActor* masterUser = frobDoor->GetUserManager().GetMasterUser();

		if (owner == masterUser)
		{
			gameRenderWorld->DebugText("Master", 
				(owner->GetPhysics()->GetOrigin() + idVec3(0, 0, 20)), 
				0.25f, colorOrange, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 4 * USERCMD_MSEC);
		}
		else
		{
			gameRenderWorld->DebugText("Slave", 
				(owner->GetPhysics()->GetOrigin() + idVec3(0, 0, 20)), 
				0.25f, colorMdGrey, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 4 * USERCMD_MSEC);

		}
	}
}

void HandleDoorTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteVec3(_frontPos);
	savefile->WriteVec3(_backPos);
	savefile->WriteVec3(_midPos);	// grayman #2345
	savefile->WriteVec3(_safePos);	// grayman #2345
	savefile->WriteInt(static_cast<int>(_doorHandlingState));
	savefile->WriteInt(_waitEndTime);
	savefile->WriteBool(_wasLocked);
	savefile->WriteBool(_doorInTheWay);
	savefile->WriteInt(_retryCount);
	savefile->WriteInt(_leaveQueue);		// grayman #2345
	savefile->WriteInt(_leaveDoor);			// grayman #2700
	savefile->WriteBool(_triedFitting);		// grayman #2345
	savefile->WriteBool(_canHandleDoor);	// grayman #2712
	savefile->WriteBool(_closeFromSameSide); // grayman #2866
	//savefile->WriteInt(_blockedDoorCount);	// grayman #3523 // grayman #4830
	savefile->WriteInt(_useDelay);			// grayman #3755
	savefile->WriteBool(_rotates);			// grayman #3643
	savefile->WriteInt(_doorSide);			// grayman #3643

	_frontPosEnt.Save(savefile);
	_backPosEnt.Save(savefile);
}

void HandleDoorTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadVec3(_frontPos);
	savefile->ReadVec3(_backPos);
	savefile->ReadVec3(_midPos);	// grayman #2345
	savefile->ReadVec3(_safePos);	// grayman #2345
	int temp;
	savefile->ReadInt(temp);
	_doorHandlingState = static_cast<EDoorHandlingState>(temp);
	savefile->ReadInt(_waitEndTime);
	savefile->ReadBool(_wasLocked);
	savefile->ReadBool(_doorInTheWay);
	savefile->ReadInt(_retryCount);
	savefile->ReadInt(_leaveQueue);		// grayman #2345
	savefile->ReadInt(_leaveDoor);		// grayman #2700
	savefile->ReadBool(_triedFitting);	// grayman #2345
	savefile->ReadBool(_canHandleDoor);	// grayman #2712
	savefile->ReadBool(_closeFromSameSide); // grayman #2866
	//savefile->ReadInt(_blockedDoorCount);	// grayman #3523 // grayman #4830
	savefile->ReadInt(_useDelay);			// grayman #3755
	savefile->ReadBool(_rotates);			// grayman #3643
	savefile->ReadInt(_doorSide);			// grayman #3643

	_frontPosEnt.Restore(savefile);
	_backPosEnt.Restore(savefile);
}

HandleDoorTaskPtr HandleDoorTask::CreateInstance()
{
	return HandleDoorTaskPtr(new HandleDoorTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar handleDoorTaskRegistrar(
	TASK_HANDLE_DOOR, // Task Name
	TaskLibrary::CreateInstanceFunc(&HandleDoorTask::CreateInstance) // Instance creation callback
);

} // namespace ai
