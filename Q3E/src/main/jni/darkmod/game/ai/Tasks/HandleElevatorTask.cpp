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
#include "HandleElevatorTask.h"
#include "../../MultiStateMoverButton.h"
#include "../EAS/EAS.h"

namespace ai
{

// grayman #3050 - values for elevator ++++++++++

#define RIDE_ELEVATOR_WAIT		 1500	// wait this long (ms) to give the elevator time to start moving
#define BUTTON_PRESS_WAIT		 1000	// start button-pressing animation, but wait this long to operate the button
#define ELEVATOR_AREA_CUTOFF	15000	// elevator < this area supports 1 rider max; > this area supports 2 max
#define MAX_RIDERS					2	// max riders cap
#define PAUSE_DISTANCE			  100	// when getting on an elevator, pause this far from the station entity
										// until the master has arrived at the ride button.
										// This cuts down on jostling on the elevator.
// ++++++++++++++++++++++++++++++++++++++++++++++

HandleElevatorTask::HandleElevatorTask() :
	_waitEndTime(0),
	_success(false),
	_maxRiders(1) // grayman #3050
{}

HandleElevatorTask::HandleElevatorTask(const eas::RouteInfoPtr& routeInfo) :
	_waitEndTime(0),
	_routeInfo(*routeInfo), // copy-construct the RouteInfo, creates a clean duplicate
	_success(false),
	_maxRiders(1) // grayman #3050
{}

// Get the name of this task
const idStr& HandleElevatorTask::GetName() const
{
	static idStr _name(TASK_HANDLE_ELEVATOR);
	return _name;
}

void HandleElevatorTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	Task::Init(owner, subsystem);

	owner->m_ElevatorQueued = false; // grayman #3647

	// grayman #3029 - when using an elevator, PushMove() once so you can
	// pop the move to get off the elevator. Then PushMove() a second time
	// so you can pop the move once you're off the elevator. This is
	// necessary because you could encounter the next elevator while
	// still getting off this one, and it's ignored because you're
	// still handling this one. A second push/pop lets you see the
	// new elevator after you're finished with the current one.

	owner->PushMove(); // Save the move for once you're off the elevator
	owner->PushMove(); // Save the move that will get you off the elevator

	owner->m_HandlingElevator = true; // grayman #3029 - moved from State

	if (_routeInfo.routeNodes.size() < 2)
	{
		// no RouteNodes available?
		subsystem.FinishTask(); 
		return;
	}

	// Grab the first RouteNode
	const eas::RouteNodePtr& node = *_routeInfo.routeNodes.begin();

	// Retrieve the elevator station info from the EAS
	eas::ElevatorStationInfoPtr stationInfo = 
		owner->GetAAS()->GetEAS()->GetElevatorStationInfo(node->elevatorStation);

	//Memory& memory = owner->GetMemory();
	CMultiStateMoverPosition* pos = stationInfo->elevatorPosition.GetEntity();
	CMultiStateMover* elevator = stationInfo->elevator.GetEntity();

	// Is the elevator station reachable?
	if (!IsElevatorStationReachable(pos))
	{
		subsystem.FinishTask();
		return;
	}

	// grayman #3050 - determine max riders based on elevator floor area

	idVec3 elevSize = elevator->GetPhysics()->GetBounds().GetSize();
	float elevArea = elevSize.x * elevSize.y;
	_maxRiders = static_cast<int>(elevArea/ELEVATOR_AREA_CUTOFF) + 1;
	if ( _maxRiders > MAX_RIDERS)
	{
		_maxRiders = MAX_RIDERS; // capped
	}

	if (owner->ReachedPos(pos->GetPhysics()->GetOrigin(), MOVE_TO_POSITION))
	{
		// We are already at the elevator position, this is true if the elevator is there
		_state = EInitiateMoveToRideButton;
	}
	// Start moving towards the elevator station
	else if (owner->MoveToPosition(pos->GetPhysics()->GetOrigin()))
	{
		// If AI_MOVE_DONE is true, we are already at the target position
		// grayman #3029 - can't set up doors if we're moving toward the ride button, but
		// we can if moving toward the station.

		if ( owner->GetMoveStatus() == MOVE_STATUS_DONE )
		{
			_state = EInitiateMoveToRideButton;
		}
		else
		{
			_state = EMovingTowardsStation;
		}
	}
	else
	{
		// Position entity cannot be reached, probably due to elevator not being there, use the button entity
		_state = EInitiateMoveToFetchButton;
	}

	if (_state == EInitiateMoveToRideButton)
	{
		owner->m_CanSetupDoor = false; // grayman #3029
		// add ourself to the user lists of the elevator
		elevator->GetUserManager().AddUser(owner);
		elevator->GetRiderManager().AddUser(owner);
		idActor* masterElevatorUser = elevator->GetUserManager().GetMasterUser();
		if ( masterElevatorUser != owner )
		{
			// grayman #3050 - wait for master to leave the elevator
			owner->StopMove(MOVE_STATUS_DONE);
			_state = EPause;
		}
	}
}

bool HandleElevatorTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("HandleElevatorTask performing.\r");

	idAI* owner = _owner.GetEntity();

	// grayman #2948 - leave elevator handling if KO'ed or dead

	if ( owner->AI_KNOCKEDOUT || owner->AI_DEAD )
	{
		return true;
	}
	
	// grayman #2816 - stop elevator handling for various reasons

	if ( owner->GetMemory().stopHandlingElevator )
	{
		return true;
	}
	
	// grayman #3050 - Special check for ending state that doesn't need all the elevator information.

	if ( _state == EWrapUp )
	{
		if ( gameLocal.time > _waitEndTime )
		{
			_success = true;
			return true;
		}

		return false; // not finished yet
	}

	// Grab the first RouteNode
	const eas::RouteNodePtr& node = *_routeInfo.routeNodes.begin();

	// Retrieve the elevator station info from the EAS
	eas::ElevatorStationInfoPtr stationInfo = owner->GetAAS()->GetEAS()->GetElevatorStationInfo(node->elevatorStation);

	CMultiStateMoverPosition* pos = stationInfo->elevatorPosition.GetEntity();
	CMultiStateMover* elevator = stationInfo->elevator.GetEntity();

	// check users of elevator and position
	int numElevatorUsers = elevator->GetUserManager().GetNumUsers();
	idActor* masterElevatorUser = elevator->GetUserManager().GetMasterUser();
	int numPositionUsers = pos->GetUserManager().GetNumUsers();

	// Grab the second RouteNode
	eas::RouteNodeList::const_iterator first = _routeInfo.routeNodes.begin();
	const eas::RouteNodePtr& node2 = *(++first);

	// Retrieve the elevator station info from the EAS
	eas::ElevatorStationInfoPtr stationInfo2 = 
		owner->GetAAS()->GetEAS()->GetElevatorStationInfo(node2->elevatorStation);

	CMultiStateMoverPosition* targetPos = stationInfo2->elevatorPosition.GetEntity();

	idVec3 dir;
	float dist;

	switch (_state)
	{
		case EMovingTowardsStation:
			//dist = (owner->GetPhysics()->GetOrigin() - pos->GetPhysics()->GetOrigin()).LengthFast();
			if (owner->GetMoveStatus() == MOVE_STATUS_DONE)
			{
				// Move is done, this means that we might be close enough, but it's not guaranteed
				_state = EInitiateMoveToFetchButton;
			}
			else if ((owner->GetPhysics()->GetOrigin() - pos->GetPhysics()->GetOrigin()).LengthSqr() < 500*500 &&
				     (owner->CanSeeExt(pos, true, false) || owner->CanSeeExt(elevator, true, false)))
			{
				if (elevator->IsAtPosition(pos))
				{
					// elevator is at the desired position, get onto it
					MoveToPositionEntity(owner, pos);
					_state = EMoveOntoElevator;

					// add ourself to the user lists of the elevator and the elevator position
					elevator->GetUserManager().AddUser(owner);
					pos->GetUserManager().AddUser(owner);
					ReorderQueue(pos->GetUserManager(),pos->GetPhysics()->GetOrigin()); // grayman #3050
				}
				else
				{
					// elevator is somewhere else
					_state = EInitiateMoveToFetchButton;
				}
			}	
			break;

		case EInitiateMoveToFetchButton:
		{
			CMultiStateMoverButton* fetchButton = pos->GetFetchButton(owner->GetPhysics()->GetOrigin()); // grayman #3029
			if (fetchButton == NULL)
			{
				owner->StopMove(MOVE_STATUS_DEST_UNREACHABLE);
				return true;
			}
			// it's not occupied, get to the button
			MoveToButton(owner, fetchButton);
			_state = EMovingToFetchButton;
		}
		break;

		case EMovingToFetchButton:
		{
			CMultiStateMoverButton* fetchButton = pos->GetFetchButton(owner->GetPhysics()->GetOrigin()); // grayman #3029
			if (fetchButton == NULL)
			{
				owner->StopMove(MOVE_STATUS_DEST_UNREACHABLE);
				return true;
			}

			dir = owner->GetPhysics()->GetOrigin() - fetchButton->GetPhysics()->GetOrigin();
			dir.z = 0;
			dist = dir.LengthFast();
			if (dist < owner->GetArmReachLength() + 10)
			{
				owner->StopMove(MOVE_STATUS_DONE);

				// add ourself to the user lists of the elevator and the elevator position
				elevator->GetUserManager().AddUser(owner);
				pos->GetUserManager().AddUser(owner);
				ReorderQueue(pos->GetUserManager(),pos->GetPhysics()->GetOrigin()); // grayman #3050

				numElevatorUsers = elevator->GetUserManager().GetNumUsers();
				masterElevatorUser = elevator->GetUserManager().GetMasterUser();
				numPositionUsers = pos->GetUserManager().GetNumUsers();

				if ( ( numElevatorUsers > 1 ) && ( masterElevatorUser != owner) )
				{
					// If the elevator is at the right position, get onto it.
					// If the elevator isn't at the right position, it's in
					// use and you have to wait your turn.

					if (elevator->IsAtPosition(pos))
					{
						owner->MoveToPosition(pos->GetPhysics()->GetOrigin());
						_state = EMoveOntoElevator;
					}
					else
					{
						owner->TurnToward(pos->GetPhysics()->GetOrigin()); // grayman #3050 - turn toward elevator station while waiting
					}
				}
				else if (elevator->IsAtRest()) // grayman #3029 - don't call a moving elevator (the player is probably using it)
				{
					// raise hand to press button
					owner->TurnToward(fetchButton->GetPhysics()->GetOrigin());
					owner->Event_LookAtPosition(fetchButton->GetPhysics()->GetOrigin(), 1);
					owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Use_righthand", 4);
					_state = EPressFetchButton;
					_waitEndTime = gameLocal.time + BUTTON_PRESS_WAIT; // grayman #3029 - was 400, which isn't long enough
				}
			}
			else if (owner->GetMoveStatus() == MOVE_STATUS_DEST_UNREACHABLE)
			{
				// Destination unreachable, help!
				return true;
			}
		}
		break;

		case EPressFetchButton:
		{
			CMultiStateMoverButton* fetchButton = pos->GetFetchButton(owner->GetPhysics()->GetOrigin()); // grayman #3029
			if (fetchButton == NULL)
			{
				owner->StopMove(MOVE_STATUS_DEST_UNREACHABLE);
				return true;
			}

			if (gameLocal.time >= _waitEndTime)
			{
				// Press button and wait for elevator
				fetchButton->Operate();
				owner->StopMove(MOVE_STATUS_WAITING);
				_state = EWaitForElevator;

				// this is to avoid having the AI pressing the button twice 
				// when the elevator takes too long to start moving
				_waitEndTime = gameLocal.time + 500;
			}
		}
		break;

		case EWaitForElevator:
			if (elevator->IsAtPosition(pos))
			{
				// elevator is at the desired position, get onto it
				owner->MoveToPosition(pos->GetPhysics()->GetOrigin());
				_state = EMoveOntoElevator;
			}
			else if (elevator->IsAtRest() && gameLocal.time > _waitEndTime)
			{
				// Elevator has stopped moving and is not at our station, press the fetch button!
				_state = EInitiateMoveToFetchButton;
			}
			else if ( gameLocal.time > _waitEndTime ) // grayman #3050
			{
				owner->TurnToward(pos->GetPhysics()->GetOrigin()); // turn toward station
			}
			break;

		case EMoveOntoElevator:
		{
			if ( (owner->GetMoveStatus() == MOVE_STATUS_DONE) && owner->OnElevator(false) ) // stopped on elevator?
			{
				// remove ourself from users list of elevator station
				pos->GetUserManager().RemoveUser(owner);
				elevator->GetRiderManager().AddUser(owner);

				// grayman #3029 - Once we become the master elevator user, we can push
				// the ride button, assuming we still need to do so. Otherwise,
				// a slow previous master elevator user who hasn't left the elevator yet could have
				// problems getting off if we push the button now and the elevator starts moving.

				if ( masterElevatorUser == owner )
				{
					// Move towards ride button
					_state = EInitiateMoveToRideButton;
					owner->m_CanSetupDoor = false; // grayman #3029
				}
				else // you're along for the ride
				{
					_waitEndTime = gameLocal.time + RIDE_ELEVATOR_WAIT; // grayman #3050 - bump up to keep the task alive
					_state = ERideOnElevator;
				}
			}
			else if ( (owner->GetMoveStatus() == MOVE_STATUS_DONE) && !owner->OnElevator(false) ) // stopped off elevator?
			{
				return true; // abort task and start over
			}
			else if (!elevator->IsAtPosition(pos))
			{
				// elevator moved away while we attempted to move onto it
				if (owner->OnElevator(false))
				{
					// elevator is already moving
					owner->StopMove(MOVE_STATUS_DONE);
					// remove ourself from users list of elevator station
					pos->GetUserManager().RemoveUser(owner);
					ReorderQueue(pos->GetUserManager(),pos->GetPhysics()->GetOrigin()); // grayman #3050
					elevator->GetRiderManager().AddUser(owner);
					_waitEndTime = gameLocal.time + RIDE_ELEVATOR_WAIT;
					_state = ERideOnElevator;
				}
				else
				{
					// move towards fetch button
					owner->StopMove(MOVE_STATUS_DONE);
					_state = EInitiateMoveToFetchButton;
				}
			}
			else if ( masterElevatorUser != owner ) // grayman #3050
			{
				if ( (owner->GetPhysics()->GetOrigin() - pos->GetPhysics()->GetOrigin()).LengthFast() <= PAUSE_DISTANCE )
				{
					if ( !elevator->IsMasterAtRideButton() ||
						( elevator->GetRiderManager().GetNumUsers() >= _maxRiders ) ||
						( pos->GetUserManager().GetMasterUser() != owner) ) // grayman #3050
					{
						owner->StopMove(MOVE_STATUS_DONE);
						_state = EPause;
					}
				}
			}
		}
		break;

		case EPause: // grayman #3050
			// Once the master has arrived at the ride button, and the number of
			// riders is 0 or 1, proceed onto the elevator. Or, proceed if you're
			// now the master.

			// This is also used when you terminate an elevator task because the
			// master took you to a floor different than where you wanted to go.
			// You immediately start a new task at this floor and go straight to
			// EPause under certain conditions.

			if (!elevator->IsAtPosition(pos))
			{
				// Elevator moved away while we were waiting to step on.
				// Move towards the fetch button.
				_state = EInitiateMoveToFetchButton;
			}
			else if ( ( masterElevatorUser == owner ) || ( elevator->IsMasterAtRideButton()  && ( elevator->GetRiderManager().GetNumUsers() < _maxRiders ) ) )
			{
				owner->MoveToPosition(pos->GetPhysics()->GetOrigin());
				_state = EMoveOntoElevator;
			}
			break;

		case EInitiateMoveToRideButton:
		{
			CMultiStateMoverButton* rideButton = pos->GetRideButton(targetPos);
			if (rideButton == NULL)
			{
				owner->StopMove(MOVE_STATUS_DEST_UNREACHABLE);
				return true;
			}

			if (!elevator->IsAtPosition(pos))
			{
				// elevator is already moving
				owner->StopMove(MOVE_STATUS_DONE);
				_state = ERideOnElevator;
				_waitEndTime = gameLocal.time + RIDE_ELEVATOR_WAIT;
			}
			else
			{
				MoveToButton(owner, rideButton);
				_state = EMovingToRideButton;
			}
		}
		break;

		case EMovingToRideButton:
		{
			CMultiStateMoverButton* rideButton = pos->GetRideButton(targetPos);
			if (rideButton == NULL)
			{
				owner->StopMove(MOVE_STATUS_DEST_UNREACHABLE);
				return true;
			}

			if (!elevator->IsAtPosition(pos))
			{
				// elevator is already moving
				owner->StopMove(MOVE_STATUS_DONE);
				_state = ERideOnElevator;
				_waitEndTime = gameLocal.time + RIDE_ELEVATOR_WAIT;
				break;
			}

			dir = owner->GetPhysics()->GetOrigin() - rideButton->GetPhysics()->GetOrigin();
			dir.z = 0;
			dist = dir.LengthFast();
			if (dist < owner->GetArmReachLength() + 20)
			{
				owner->StopMove(MOVE_STATUS_DONE);
				elevator->SetMasterAtRideButton(true);
				if ( ( numPositionUsers < 1 ) || ( elevator->GetRiderManager().GetNumUsers() >= _maxRiders ) ) // grayman #3050
				{
					// Either all users at this station have moved onto the elevator
					// (removed themselves from the position user list) or we've reached
					// the limit of the number of riders allowed, so you can raise your hand to press the ride button
					owner->TurnToward(rideButton->GetPhysics()->GetOrigin());
					owner->Event_LookAtPosition(rideButton->GetPhysics()->GetOrigin(), 1);
					owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_Use_righthand", 4);
					_state = EPressRideButton;
					_waitEndTime = gameLocal.time + BUTTON_PRESS_WAIT; // grayman #3029 - was 400, which isn't long enough
				}
			}
		}
		break;

		case EPressRideButton:
		{
			if (!elevator->IsAtPosition(pos))
			{
				// elevator is already moving
				owner->StopMove(MOVE_STATUS_DONE);
				_waitEndTime = gameLocal.time + RIDE_ELEVATOR_WAIT;
				_state = ERideOnElevator;
				break;
			}

			if (gameLocal.time >= _waitEndTime)
			{
				CMultiStateMoverButton* rideButton = pos->GetRideButton(targetPos);
				if (rideButton != NULL)
				{
					// Press button and wait while elevator moves
					rideButton->Operate();
					_state = ERideOnElevator;
					_waitEndTime = gameLocal.time + RIDE_ELEVATOR_WAIT;
				}
				else
				{
					owner->StopMove(MOVE_STATUS_DEST_UNREACHABLE);
					return true;
				}
			}
		}
		break;

		case ERideOnElevator:
			if (elevator->IsAtPosition(targetPos))
			{
				// reached target station, get off the elevator
				_state = EGetOffElevator;

				// Restore the movestate we had before starting this task. This gets us to walk off the elevator.
				owner->PopMove();
			}
			else if ( elevator->IsAtRest() && ( gameLocal.time > _waitEndTime) )
			{
				// elevator stopped at a different position
				// restart our pathing from here
				return true;
			}
			else if ( gameLocal.time > _waitEndTime ) // grayman #3050
			{
				owner->TurnToward(elevator->GetPhysics()->GetOrigin()); // turn toward elevator center
			}
			break;
		
		case EGetOffElevator:
			if ( !owner->OnElevator(false) || ( owner->GetMoveStatus() == MOVE_STATUS_DONE ) ) // grayman #3029 - in case we stopped for some reason
			{
				if ( masterElevatorUser == owner )
				{
					elevator->SetMasterAtRideButton(false);
				}
				_state = EWrapUp; // grayman #3050
				_waitEndTime = gameLocal.time + 1000; // grayman #3050 - add an extra second to clear the elevator before ending the task
			}
			else if (!elevator->IsAtPosition(targetPos)) // grayman #3029 - player hitting a fetch button elsewhere can cause this
			{
				// elevator moved away while we attempted to get off, so ride it out and try again
				_state = ERideOnElevator;
				owner->PushMove(); // grayman #3050 - need to re-push this, since we're going backward in state
				_waitEndTime = gameLocal.time + RIDE_ELEVATOR_WAIT;
			}
			break;

		default:
			break;
	}

	// Optional debug output
	if (cv_ai_elevator_show.GetBool())
	{
		CMultiStateMoverPosition* pos = stationInfo->elevatorPosition.GetEntity();
		CMultiStateMover* elevator = stationInfo->elevator.GetEntity();
		DebugDraw(owner, pos, elevator);
	}

	return false; // not finished yet
}

bool HandleElevatorTask::CanAbort() // grayman #3050
{
	return ( _state < EPressFetchButton );
}

bool HandleElevatorTask::IsElevatorStationReachable(CMultiStateMoverPosition* pos)
{
	// TODO: Implement check here

	return true;
}

#if 0
// grayman - print elevator queue for debugging

void HandleElevatorTask::PrintElevQueue(CMultiStateMover* elevator)
{
	DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Elevator queue ...\r");
	UserManager eum = elevator->GetUserManager();
	int numUsers = eum.GetNumUsers();
	for ( int i = 0 ; i < numUsers ; i++ )
	{
		idActor* user = eum.GetUserAtIndex(i);
		if ( user != NULL )
		{
			DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("     %d: %s\r",i,user->name.c_str());
		}
	}
}
#endif

// grayman #3050 - reorder a queue based on members' distance from a point

void HandleElevatorTask::ReorderQueue(UserManager &um, idVec3 point)
{
	int numUsers = um.GetNumUsers();
	if (numUsers > 1)
	{
		idActor* closestUser = NULL;			// the user closest to the point
		int masterIndex = 0;					// index of closest user
		float minDistance = idMath::INFINITY;	// minimum distance of all users
		for ( int i = 0 ; i < numUsers ; i++ )
		{
			idActor* user = um.GetUserAtIndex(i);
			if ( user != NULL )
			{
				float distance = (user->GetPhysics()->GetOrigin() - point).LengthFast();
				if (distance < minDistance)
				{
					masterIndex = i;
					closestUser = user;
					minDistance = distance;
				}
			}
		}

		if (masterIndex > 0) // only rearrange the queue if someone other than the current master is closer
		{
			um.RemoveUser(closestUser);				// remove AI from current spot
			um.InsertUserAtIndex(closestUser,0);	// and put him at the top
		}
	}
}

void HandleElevatorTask::OnFinish(idAI* owner)
{
	//Memory& memory = owner->GetMemory();

	// Grab the first RouteNode
	const eas::RouteNodePtr& node = *_routeInfo.routeNodes.begin();

	// Retrieve the elevator station info from the EAS
	eas::ElevatorStationInfoPtr stationInfo = 
		owner->GetAAS()->GetEAS()->GetElevatorStationInfo(node->elevatorStation);

	// remove ourself from the user lists of the elevator and the elevator position
	CMultiStateMoverPosition* pos = stationInfo->elevatorPosition.GetEntity();
	pos->GetUserManager().RemoveUser(owner);
	ReorderQueue(pos->GetUserManager(),pos->GetPhysics()->GetOrigin()); // grayman #3050

	CMultiStateMover* elevator = stationInfo->elevator.GetEntity();
	elevator->GetUserManager().RemoveUser(owner);

	// grayman #3050 - leave the rider list if you haven't already
	elevator->GetRiderManager().RemoveUser(owner);

	// grayman #3029 - reorder the elevator queue. This should handle this situation:
	//
	// 1 - elevator arrives at a station and A gets off
	// 2 - B is waiting at the station, but isn't the master
	// 3 - as soon as the elevator arrives, B moves to get on the elevator
	// 4 - C is waiting at a different station, and is the master
	// 5 - as soon as A exits, C is allowed to push the fetch button
	// 6 - the elevator starts to move to C's station, but B may or may not be on the elevator

	// If we reorder the queue to make B the master because he's closer to the elevator,
	// C won't push his fetch button, and B won't have a problem. This can make AI wait a bit
	// longer at times for the elevator, but it's better than having AI start to step onto
	// an elevator, just to have to turn around and awkwardly step off the elevator as it
	// begins to move. Or, worse yet, get stuck on the elevator edge and bump into the
	// architecture, causing the elevator to stop moving and get stuck.

	ReorderQueue(elevator->GetUserManager(),elevator->GetPhysics()->GetOrigin());

	if (!_success)
	{
		// This task could not finish successfully, stop the move
		owner->StopMove(MOVE_STATUS_DEST_UNREACHABLE);

		// Restore the movestate we had before starting this task
		// grayman #3647 - don't instigate an actual move from this one;
		// we're just getting it out of the way so we can get at the
		// final one a few lines below.
		// owner->PopMove();
		owner->PopMoveNoRestoreMove();
	}

	owner->m_HandlingElevator = false;
	owner->m_CanSetupDoor = true; // grayman #3029
	owner->GetMemory().stopHandlingElevator = false; // grayman #2816
	owner->m_ElevatorQueued = false; // grayman #3647

	// grayman #3029 - restore from the first push, regardless of success or failure
	owner->PopMove();

	owner->GetMemory().latchPickedPocket = false; // grayman #3559 - free to react to a picked pocket
}

bool HandleElevatorTask::MoveToPositionEntity(idAI* owner, CMultiStateMoverPosition* pos)
{
	if (!owner->MoveToPosition(pos->GetPhysics()->GetOrigin()))
	{
		// Position entity cannot be reached, probably due to elevator not being there, so use the button entity
		CMultiStateMoverButton* button = pos->GetFetchButton(owner->GetPhysics()->GetOrigin()); // grayman #3029
		if (button != NULL)
		{
			return MoveToButton(owner, button);
		}

		return false;
	}

	return true;
}

bool HandleElevatorTask::MoveToButton(idAI* owner, CMultiStateMoverButton* button)
{
	idBounds bounds = owner->GetPhysics()->GetBounds();
	float size = idMath::Fabs(bounds[0][1]);

	// Find spot in front of the button,
	// assuming that the button translates in when pressed
	idVec3 trans = button->spawnArgs.GetVector("translate", "1 0 0"); // grayman #3389 - was "translation"
	trans.z = 0;
	if (trans.NormalizeFast() == 0)
	{
		trans = idVec3(1, 0, 0); // somewhere to start
	}

	const idVec3& buttonOrigin = button->GetPhysics()->GetOrigin();

	// angua: target position is in front of the button with a distance 
	// a little bit larger than the AI's bounding box
	idVec3 target = buttonOrigin - size * 1.2f * trans;

	// grayman #3389 - find floor beneath 'target' and subsequent 'targets' if necessary

	idVec3 bottomPoint = target;
	bottomPoint.z -= 1000;
	trace_t result;
	gameLocal.clip.TracePoint(result, target, bottomPoint, MASK_OPAQUE, NULL);
	target.z = result.endpos.z + 1; // move the target point to just above the floor

	if (!owner->MoveToPosition(target))
	{
		// not reachable, try alternate target positions at the sides and behind the button

		const idVec3& gravity = owner->GetPhysics()->GetGravityNormal();

		trans = trans.Cross(gravity);
		target = buttonOrigin - size * 1.2f * trans;
		bottomPoint = target;
		bottomPoint.z -= 1000;
		gameLocal.clip.TracePoint(result, target, bottomPoint, MASK_OPAQUE, NULL);
		target.z = result.endpos.z + 1; // move the target point to just above the floor
		if (!owner->MoveToPosition(target))
		{
			trans *= -1;
			target = buttonOrigin - size * 1.2f * trans;
			bottomPoint = target;
			bottomPoint.z -= 1000;
			gameLocal.clip.TracePoint(result, target, bottomPoint, MASK_OPAQUE, NULL);
			target.z = result.endpos.z + 1; // move the target point to just above the floor
			if (!owner->MoveToPosition(target))
			{
				trans = trans.Cross(gravity);
				target = buttonOrigin - size * 1.2f * trans;
				bottomPoint = target;
				bottomPoint.z -= 1000;
				gameLocal.clip.TracePoint(result, target, bottomPoint, MASK_OPAQUE, NULL);
				target.z = result.endpos.z + 1; // move the target point to just above the floor
				if (!owner->MoveToPosition(target))
				{
					owner->StopMove(MOVE_STATUS_DEST_UNREACHABLE);
					return false;
				}
			}
		}
	}
	return true;
}

void HandleElevatorTask::DebugDraw(idAI* owner, CMultiStateMoverPosition* pos, CMultiStateMover* elevator)
{
	// Draw current state
	idMat3 viewMatrix = gameLocal.GetLocalPlayer()->viewAngles.ToMat3();

	idStr str = "";
	switch (_state)
	{
		case EMovingTowardsStation:
			str = "MovingTowardsStation";
			break;
		case EInitiateMoveToFetchButton:
			str = "InitiateMoveToFetchButton";
			break;
		case EMovingToFetchButton:
			str = "MovingToFetchButton";
			break;
		case EPressFetchButton:
			str = "PressButton";
			break;
		case EWaitForElevator:
			str = "WaitForElevator";
			break;
		case EMoveOntoElevator:
			str = "MoveOntoElevator";
			break;
		case EPause:
			str = "Pause";
			break;
		case EInitiateMoveToRideButton:
			str = "InitiateMoveToRideButton";
			break;
		case EMovingToRideButton:
			str = "MovingToRideButton";
			break;
		case EPressRideButton:
			str = "PressRideButton";
			break;
		case ERideOnElevator:
			str = "RideOnElevator";
			break;
		case EGetOffElevator:
			str = "EGetOffElevator";
			break;
		case EWrapUp:
			str = "EWrapUp";
			break;
			
		default:
			break;
	}

	// Show position in both queues, if any

	int index = pos->GetUserManager().GetIndex(owner) + 1;
	idStr s = "";
	if ( index > 0 )
	{
		s = va("-P%d",index);
	}
	str += s;

	s = "";
	index = elevator->GetUserManager().GetIndex(owner) + 1;
	if ( index > 0 )
	{
		s = va("-E%d",index);
	}
	str += s;
	
	gameRenderWorld->DebugText(str.c_str(), 
		(owner->GetEyePosition() - owner->GetPhysics()->GetGravityNormal()*10.0f), 
		0.25f, colorYellow, viewMatrix, 1, 4 * USERCMD_MSEC);
}

void HandleElevatorTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);
	_routeInfo.Save(savefile);

	savefile->WriteInt(static_cast<int>(_state));
	savefile->WriteInt(_waitEndTime);

	savefile->WriteBool(_success);

	savefile->WriteInt(_maxRiders); // grayman #3050
}

void HandleElevatorTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);
	_routeInfo.Restore(savefile);

	int temp;
	savefile->ReadInt(temp);
	_state = static_cast<State>(temp);
	savefile->ReadInt(_waitEndTime);

	savefile->ReadBool(_success);

	savefile->ReadInt(_maxRiders); // grayman #3050
}

HandleElevatorTaskPtr HandleElevatorTask::CreateInstance()
{
	return HandleElevatorTaskPtr(new HandleElevatorTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar handleElevatorTaskRegistrar(
	TASK_HANDLE_ELEVATOR, // Task Name
	TaskLibrary::CreateInstanceFunc(&HandleElevatorTask::CreateInstance) // Instance creation callback
);

} // namespace ai
