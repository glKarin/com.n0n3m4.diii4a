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



#include "GuardSpotTask.h"
#include "WaitTask.h"
#include "../Memory.h"
#include "../Library.h"
#include "SingleBarkTask.h"
//#include "IdleAnimationTask.h"

namespace ai
{

const float MAX_TRAVEL_DISTANCE_WALKING = 300; // units?
const float MAX_YAW = 90; // max yaw (+/-) from original yaw for idle turning
const int   TURN_DELAY = 5000; // will make the guard turn every 5-8 seconds
const int   TURN_DELAY_DELTA = 3000;
const int   MILLING_DELAY = 3500; // will generate milling times between 3.5 and 7 seconds
const float CLOSE_ENOUGH = 48.0f; // try to get this close to goal
const float TRY_AGAIN_DISTANCE = 100.0f; // have reached point if this close when stopped
const int	LENGTH_OF_GIVE_ORDER_BARK = 1500; // estimated duration of 'snd_giveOrder' bark

GuardSpotTask::GuardSpotTask() :
	_nextTurnTime(0),
	_exitTime(0)
{}

// Get the name of this task
const idStr& GuardSpotTask::GetName() const
{
	static idStr _name(TASK_GUARD_SPOT);
	return _name;
}

void GuardSpotTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	// Get a shortcut reference
	Memory& memory = owner->GetMemory();

	if (memory.currentSearchSpot.x == idMath::INFINITY)
	{
		// Invalid spot, terminate task
		subsystem.FinishTask();
		return;
	}

	// Set the goal position
	SetNewGoal(memory.currentSearchSpot);
	_guardSpotState = EStateSetup;

	// Milling?
	if (memory.millingInProgress)
	{
		// Is there any activity after milling is over?
		// If so, we want a short _exitTime so we can make the run
		// before we drop out of searching mode. If no, we can continue
		// milling until we drop out.

		Search* search = gameLocal.m_searchManager->GetSearch(owner->m_searchID);
		Assignment* assignment = gameLocal.m_searchManager->GetAssignment(search,owner);

		if (assignment)
		{
			_millingOnly = true;
			if (assignment->_searcherRole == E_ROLE_SEARCHER)
			{
				if (search->_assignmentFlags & SEARCH_SEARCH)
				{
					_millingOnly = false;
				}
			}
			else if (assignment->_searcherRole == E_ROLE_GUARD)
			{
				if (search->_assignmentFlags & SEARCH_GUARD)
				{
					_millingOnly = false;
				}
			}
			else // observer
			{
				if (search->_assignmentFlags & SEARCH_OBSERVE)
				{
					_millingOnly = false;
				}
			}
		}
		memory.stopMilling = false;
	}
	else
	{
		memory.stopGuarding = false;
	}
}

bool GuardSpotTask::Perform(Subsystem& subsystem)
{
	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	// quit if incapable of continuing
	if (owner->AI_DEAD || owner->AI_KNOCKEDOUT)
	{
		return true;
	}

	Memory& memory = owner->GetMemory();

	if (memory.millingInProgress && memory.stopMilling)
	{
		return true; // told to cancel this task
	}

	if (memory.guardingInProgress && memory.stopGuarding)
	{
		return true; // told to cancel this task
	}

	// if we've entered combat mode, we want to
	// end this task.

	if ( owner->AI_AlertIndex == ECombat )
	{
		return true;
	}

	// If Searching is over, end this task.

	if ( owner->AI_AlertIndex < ESearching)
	{
		return true;
	}
	
	if (_exitTime > 0)
	{
		if (memory.millingInProgress)
		{
			// If milling, and you'll be running to a guard or observation
			// spot once milling ends, have the guards talk to each other.
			// One of the active searchers should bark a "get to your post"
			// command to this AI, who should respond.

			if ( gameLocal.time >= _giveOrderTime )
			{
				// Have a searcher bark an order at owner if owner is a guard.
				// Searchers won't bark an order to observers.

				_giveOrderTime = idMath::INFINITY; // you won't need to check this again

				Search* search = gameLocal.m_searchManager->GetSearch(owner->m_searchID);
				Assignment* assignment = gameLocal.m_searchManager->GetAssignment(search, owner);
				if (assignment && (assignment->_searcherRole == E_ROLE_GUARD))
				{
					assignment = &search->_assignments[0];
					idAI *searcher = assignment->_searcher;
					if (searcher == NULL)
					{
						// First searcher has left the search. Try the second.
						assignment = &search->_assignments[1];
						searcher = assignment->_searcher;
						if (searcher == NULL)
						{
							return true;
						}
					}

					if (searcher != owner)
					{
						CommMessagePtr message = CommMessagePtr(new CommMessage(
							CommMessage::GuardLocationOrder_CommType,
							searcher, owner, // from searcher to owner
							NULL,
							vec3_zero,
							-1 // grayman #3438
							));

						searcher->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask("snd_giveOrder", message)));
					}
				}
			}
		}

		if (gameLocal.time >= _exitTime)
		{
			return true;
		}
	}

	// No exit time set, or it hasn't expired, so continue with ordinary process

	if (owner->m_HandlingDoor || owner->m_HandlingElevator)
	{
		// Wait, we're busy with a door or elevator
		return false;
	}

	// grayman #3510
	if (owner->m_RelightingLight)
	{
		// Wait, we're busy relighting a light so we have more light to search by
		return false;
	}
	
	switch (_guardSpotState)
	{
	case EStateSetup:
		{
			idVec3 destPos = _guardSpot;

			// Let's move

			// If the AI is searching and not handling a door or handling
			// an elevator or resolving a block: If the spot PointReachableAreaNum()/PushPointIntoAreaNum()
			// wants to move us to is outside the vertical boundaries of the
			// search volume, consider the point bad.
		
			bool pointValid = true;
			idVec3 goal = destPos;
			int toAreaNum = owner->PointReachableAreaNum( goal );
			if ( toAreaNum == 0 )
			{
				pointValid = false;
			}
			else
			{
				owner->GetAAS()->PushPointIntoAreaNum( toAreaNum, goal ); // if this point is outside this area, it will be moved to one of the area's edges
			}

			if ( pointValid )
			{
				pointValid = owner->MoveToPosition(goal,CLOSE_ENOUGH); // allow for someone else standing on it
			}

			if ( !pointValid || ( owner->GetMoveStatus() == MOVE_STATUS_DEST_UNREACHABLE) )
			{
				// Guard spot not reachable, terminate task
				// If milling, we must try to get another milling spot
				if (memory.millingInProgress)
				{
					memory.shouldMill = true;
				}
				return true;
			}

			// Run if the point is more than MAX_TRAVEL_DISTANCE_WALKING
			// grayman #4238 - don't run if in less than Agitated Searching mode

			bool shouldRun = false;
			if ( owner->AI_AlertIndex >= EAgitatedSearching )
			{
				float actualDist = (owner->GetPhysics()->GetOrigin() - _guardSpot).LengthFast();
				shouldRun = actualDist > MAX_TRAVEL_DISTANCE_WALKING;
				if ( !shouldRun && (owner->m_searchID > 0) )
				{
					// When searching, and assigned guard or observer roles, AI should run.
					Search* search = gameLocal.m_searchManager->GetSearch(owner->m_searchID);
					Assignment* assignment = gameLocal.m_searchManager->GetAssignment(search, owner);
					if ( search && assignment )
					{
						if ( ((assignment->_searcherRole == E_ROLE_GUARD) && (search->_assignmentFlags & SEARCH_GUARD)) ||
							((assignment->_searcherRole == E_ROLE_OBSERVER) && (search->_assignmentFlags & SEARCH_OBSERVE)) )
						{
							shouldRun = true;
						}
					}
				}
			}

			if (shouldRun)
			{
				owner->AI_RUN = true;
			}
			else
			{
				owner->AI_RUN = false;
			}

			_guardSpotState = EStateMoving;
			break;
		}
	case EStateMoving:
		{
			// Moving. Have we arrived?

			if (owner->GetMoveStatus() == MOVE_STATUS_DEST_UNREACHABLE)
			{
				// If milling, we must try to get another milling spot
				if (memory.millingInProgress)
				{
					memory.shouldMill = true;
				}
				return true;
			}	

			idVec3 ownerOrigin = owner->GetPhysics()->GetOrigin();
			if (owner->GetMoveStatus() == MOVE_STATUS_DONE)
			{
				// TODO: The following can cause jitter near the goal.
				// We might have stopped some distance
				// from the goal. If so, try again.
				if ((abs(ownerOrigin.x - _guardSpot.x) <= TRY_AGAIN_DISTANCE) &&
					(abs(ownerOrigin.y - _guardSpot.y) <= TRY_AGAIN_DISTANCE))
				{
					// We've successfully reached the spot

					// If a facing angle is specified, turn to that angle.
					// If no facing angle is specified, turn toward the origin of the search

					Search* search = gameLocal.m_searchManager->GetSearch(owner->m_searchID);

					if (search)
					{
						if (memory.guardingAngle == idMath::INFINITY)
						{
							owner->TurnToward(search->_origin);
						}
						else
						{
							owner->TurnToward(owner->GetMemory().guardingAngle);
						}

						_baseYaw = owner->GetIdealYaw();

						// Milling?
						if (memory.millingInProgress)
						{
							if (!_millingOnly)
							{
								// leave milling early, so we can get to the following activity (searching/guarding/observing)
								_exitTime = gameLocal.time + MILLING_DELAY + gameLocal.random.RandomInt(MILLING_DELAY);
								_giveOrderTime = _exitTime - LENGTH_OF_GIVE_ORDER_BARK; // time when we hear an order to guard
								_nextTurnTime = gameLocal.time + (_exitTime - gameLocal.time)/2; // turn halfway through your stay
							}
							else
							{
								// we can hang around until we drop out of searching mode
								_giveOrderTime = idMath::INFINITY; // will never hear an order to guard
								_nextTurnTime = gameLocal.time + TURN_DELAY + gameLocal.random.RandomInt(TURN_DELAY_DELTA);
							}
						}
						else // guarding or observing
						{
							_nextTurnTime = gameLocal.time + TURN_DELAY + gameLocal.random.RandomInt(TURN_DELAY_DELTA);
						}
					}

					if (owner->HasSeenEvidence())
					{
						// Draw weapon, if we haven't already
						if (!owner->GetAttackFlag(COMBAT_MELEE) && !owner->GetAttackFlag(COMBAT_RANGED))
						{
							if ( ( owner->GetNumRangedWeapons() > 0 ) && !owner->spawnArgs.GetBool("unarmed_ranged","0") )
							{
								owner->DrawWeapon(COMBAT_RANGED);
							}
							else if ( ( owner->GetNumMeleeWeapons() > 0 ) && !owner->spawnArgs.GetBool("unarmed_melee","0") )
							{
								owner->DrawWeapon(COMBAT_MELEE);
							}
						}
					}

					_guardSpotState = EStateStanding;
				}
				else
				{
					_guardSpotState = EStateSetup; // try again
				}
			}
			else
			{
				// check for closeness to goal to keep from running in circles around the spot
				//owner->PrintGoalData(_guardSpot,1);

				float distToSpot = (_guardSpot - ownerOrigin).LengthFast();
				if (distToSpot <= CLOSE_ENOUGH)
				{
					// Stop moving, we're close enough
					owner->StopMove(MOVE_STATUS_DONE);
				}
				// stay in this state
			}

			break;
		}
	case EStateStanding:
		{
			if ( (_nextTurnTime > 0) && (gameLocal.time >= _nextTurnTime) )
			{
				// turn randomly in place
				float newYaw = _baseYaw + MAX_YAW*(gameLocal.random.RandomFloat() - 0.5f); // +- MAX_YAW/2 degrees
				owner->TurnToward(newYaw);

				// Milling?
				if (memory.millingInProgress)
				{
					if (!_millingOnly)
					{
						_nextTurnTime = 0; // no more random turning, because you'll be quitting soon
					}
					else
					{
						_nextTurnTime = gameLocal.time + TURN_DELAY + gameLocal.random.RandomInt(TURN_DELAY_DELTA);
					}
				}
				else
				{
					_nextTurnTime = gameLocal.time + TURN_DELAY + gameLocal.random.RandomInt(TURN_DELAY_DELTA);
				}
			}
			break;
		}
	}

	return false; // not finished yet
}

void GuardSpotTask::SetNewGoal(const idVec3& newPos)
{
	idAI* owner = _owner.GetEntity();
	if (owner == NULL)
	{
		return;
	}

	// If newPos is in a portal, there might be a door there.

	CFrobDoor *door = NULL;

	// Determine if this spot is in or near a door
	idBounds clipBounds = idBounds(newPos);
	clipBounds.ExpandSelf(64.0f);

	// newPos might be sitting on the floor. If it is, we don't
	// want to expand downward and pick up a door on the floor below.
	// We also want to make sure the clipBounds is high enough to
	// catch doors that slide up.
	// Set the .z values accordingly.

	clipBounds[0].z = newPos.z;
	clipBounds[1].z = newPos.z + 128;
	int clipmask = owner->GetPhysics()->GetClipMask();
	idClipModel *clipModel;
	idClip_ClipModelList clipModelList;
	int numListedClipModels = gameLocal.clip.ClipModelsTouchingBounds( clipBounds, clipmask, clipModelList );
	for ( int i = 0 ; i < numListedClipModels ; i++ )
	{
		clipModel = clipModelList[i];
		idEntity* ent = clipModel->GetEntity();

		if (ent == NULL)
		{
			continue;
		}

		if (ent->IsType(CFrobDoor::Type))
		{
			door = static_cast<CFrobDoor*>(ent);
			break;
		}
	}

	if (door)
	{
		idVec3 frontPos = door->GetDoorPosition(owner->GetDoorSide(door,owner->GetPhysics()->GetOrigin()),DOOR_POS_FRONT); // grayman #4227

		// Can't stand at the front position, because you'll be in the way
		// of anyone wanting to use the door from this side. Move toward the
		// search origin.

		idVec3 dir = gameLocal.m_searchManager->GetSearch(owner->m_searchID)->_origin - frontPos;
		dir.Normalize();
		frontPos += 50*dir;

		_guardSpot = frontPos;
	}
	else
	{
		_guardSpot = newPos;
	}

	_guardSpotState = EStateSetup;

	// Set the exit time back to negative default, so that the AI starts walking again
	_exitTime = -1;
}

void GuardSpotTask::OnFinish(idAI* owner) // grayman #2560
{
	// The action subsystem has finished guarding the spot, so reset
	owner->GetMemory().guardingInProgress = false;
	owner->GetMemory().millingInProgress = false;
	owner->GetMemory().currentSearchSpot = idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY);
}

void GuardSpotTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteInt(_exitTime);
	savefile->WriteInt(_giveOrderTime);
	savefile->WriteInt(static_cast<int>(_guardSpotState));
	savefile->WriteVec3(_guardSpot);
	savefile->WriteInt(_nextTurnTime);
	savefile->WriteFloat(_baseYaw);
	savefile->WriteBool(_millingOnly);
}

void GuardSpotTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadInt(_exitTime);
	savefile->ReadInt(_giveOrderTime);

	int temp;
	savefile->ReadInt(temp);
	_guardSpotState = static_cast<EGuardSpotState>(temp);

	savefile->ReadVec3(_guardSpot);
	savefile->ReadInt(_nextTurnTime);
	savefile->ReadFloat(_baseYaw);
	savefile->ReadBool(_millingOnly);
}

GuardSpotTaskPtr GuardSpotTask::CreateInstance()
{
	return GuardSpotTaskPtr(new GuardSpotTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar guardSpotTaskRegistrar(
	TASK_GUARD_SPOT, // Task Name
	TaskLibrary::CreateInstanceFunc(&GuardSpotTask::CreateInstance) // Instance creation callback
);

} // namespace ai
