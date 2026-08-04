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



#include "InvestigateSpotTask.h"
#include "WaitTask.h"
#include "../Memory.h"
#include "../Library.h"

namespace ai
{

const int INVESTIGATE_SPOT_TIME_REMOTE = 2000; // ms (grayman #2640 - change from 600 -> 2000)
const int INVESTIGATE_SPOT_TIME_STANDARD = 300; // ms
const int INVESTIGATE_SPOT_TIME_CLOSELY = 2500; // ms

const int INVESTIGATE_SPOT_STOP_DIST = 64; // grayman #2640 - even if you can see the spot, keep moving if farther away than this // grayman #4220 - drop from 100 to 64
const int INVESTIGATE_SPOT_MIN_DIST  =  20;
const int INVESTIGATE_SPOT_CLOSELY_MAX_DIST = 100; // grayman #2928

const float MAX_TRAVEL_DISTANCE_WALKING = 300; // units?

InvestigateSpotTask::InvestigateSpotTask() :
	_investigateClosely(false)//,
{}

InvestigateSpotTask::InvestigateSpotTask(bool investigateClosely) :
	_investigateClosely(investigateClosely)//,
{}

// Get the name of this task
const idStr& InvestigateSpotTask::GetName() const
{
	static idStr _name(TASK_INVESTIGATE_SPOT);
	return _name;
}

void InvestigateSpotTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Just init the base class
	Task::Init(owner, subsystem);

	// Get a shortcut reference
	Memory& memory = owner->GetMemory();

	// Stop previous moves
	//owner->StopMove(MOVE_STATUS_DONE);

	if (memory.currentSearchSpot != idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY))
	{
		// Set the goal position
		SetNewGoal(memory.currentSearchSpot); // sets _searchSpot
		memory.hidingSpotInvestigationInProgress = true; // grayman #3857
		memory.stopHidingSpotInvestigation = false;
	}
	else
	{
		// Invalid hiding spot, terminate task
		subsystem.FinishTask();
		return; // grayman #4220
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

	_exitTime = 0; // grayman #3507 // grayman #4220 - was commented out (why?)
	_investigatingState = EStateInit; // grayman #4220
}

bool InvestigateSpotTask::Perform(Subsystem& subsystem)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("InvestigateSpotTask performing.\r");

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	// grayman #4220 - Get a shortcut reference
	Memory& memory = owner->GetMemory();

	// grayman #3857 - quit if incapable of continuing
	if (owner->AI_DEAD || owner->AI_KNOCKEDOUT)
	{
		return true;
	}

	if (owner->GetMemory().stopHidingSpotInvestigation) // grayman #3857
	{
		return true; // told to cancel this task
	}

	// grayman #3075 - if we've entered combat mode, we want to
	// end this task. But first, if we're kneeling, kill the
	// kneeling animation

	if ( owner->AI_AlertIndex == ECombat )
	{
		idStr torsoString = "Torso_KneelDown";
		idStr legsString = "Legs_KneelDown";
		bool torsoKneelingAnim = (torsoString.Cmp(owner->GetAnimState(ANIMCHANNEL_TORSO)) == 0);
		bool legsKneelingAnim = (legsString.Cmp(owner->GetAnimState(ANIMCHANNEL_LEGS)) == 0);

		if ( torsoKneelingAnim && legsKneelingAnim )
		{
			// Reset anims
			owner->StopAnim(ANIMCHANNEL_TORSO, 0);
			owner->StopAnim(ANIMCHANNEL_LEGS, 0);
			owner->SetWaitState(""); // grayman #3848
		}
		return true;
	}
	
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

	idVec3 ownerOrigin = owner->GetPhysics()->GetOrigin(); // grayman #3492

	// grayman #4220 - change to a state-driven method

	switch ( _investigatingState )
	{
		case EStateInit:
		{
			idVec3 destPos = _searchSpot;

			// greebo: For close investigation, don't step up to the very spot, to prevent the AI
			// from kneeling into bloodspots or corpses
			idVec3 direction = ownerOrigin - _searchSpot;
			if ( _investigateClosely )
			{
				idVec3 dir = direction;
				dir.NormalizeFast();

				// 20 units before the actual spot
				destPos += dir * 20;
			}

			// grayman #2603 - The fix to #2640 had the AI stopping if he's w/in INVESTIGATE_SPOT_STOP_DIST of the
			// search spot. This caused sudden jerks if he's closer than that at the start of the
			// move. To prevent that, check if he's close and--if so--don't start the move.

			// grayman #3756 - If the stimulus location should be walked to,
			// as in the case of a suspicious door, don't check whether you're
			// close enough to just look at the spot. Go there.

			if ( cv_ai_search_type.GetInteger() >= 2 ) // grayman #4220 - walk/run to all spots
			{
				memory.stimulusLocationItselfShouldBeSearched = true;
			}

			if ( !memory.stimulusLocationItselfShouldBeSearched && // grayman #3756
				!_investigateClosely &&
				(direction.LengthFast() < INVESTIGATE_SPOT_STOP_DIST) )
			{
				// Wait a bit
				_exitTime = static_cast<int>(gameLocal.time + INVESTIGATE_SPOT_TIME_REMOTE + 1000*(gameLocal.random.RandomFloat() - 0.5f)); // grayman #4220

				// Look at the point to investigate
				owner->Event_LookAtPosition(_searchSpot + idVec3(0, 0, 60), MS2SEC(_exitTime - gameLocal.time + 100)); // grayman #3857 - look at a point off the floor
				_investigatingState = EStateWaiting;

				return false; // grayman #2422
			}

			memory.stimulusLocationItselfShouldBeSearched = false; // grayman #3756

			// Let's move

			// grayman #2422
			// Here's the root of the problem. PointReachableAreaNum()
			// doesn't always look to the side to find the nearest AAS
			// area at the point's z position. It can move the point to
			// the AAS area above, or the AAS area below. This makes the AI
			// run upstairs or downstairs when all we really want him to do
			// is stay on the same floor.

			// If the AI is searching and not handling a door or handling
			// an elevator or resolving a block: If the spot PointReachableAreaNum()/PushPointIntoAreaNum()
			// wants to move us to is outside the vertical boundaries of the
			// search volume, consider the point bad.

			bool pointValid = true;
			idVec3 goal = destPos;
			int toAreaNum = owner->PointReachableAreaNum(goal);
			if ( toAreaNum == 0 )
			{
				pointValid = false;
			}
			else
			{
				owner->GetAAS()->PushPointIntoAreaNum(toAreaNum, goal); // if this point is outside this area, it will be moved to one of the area's edges
				// double-check reachability
				toAreaNum = owner->PointReachableAreaNum(goal);
				if ( toAreaNum == 0 )
				{
					pointValid = false;
				}
			}

			if ( !pointValid )
			{
				// point not valid, terminate task
				return true;
			}

			owner->MoveToPosition(goal);

			// Run if the point is more than MAX_TRAVEL_DISTANCE_WALKING
			// greebo: This is taxing and can be replaced by a simpler distance check 
			// TravelDistance takes about ~0.1 msec on my 2.2 GHz system.
			// grayman #4238 - don't run if in less than Agitated Searching mode

			//gameRenderWorld->DebugArrow(colorYellow, owner->GetEyePosition(), _searchSpot, 1, MS2SEC(_exitTime - gameLocal.time + 100));
			float actualDist = (ownerOrigin - _searchSpot).LengthFast();
			owner->AI_RUN = (owner->AI_AlertIndex >= EAgitatedSearching) && (actualDist > MAX_TRAVEL_DISTANCE_WALKING);
			_investigatingState = EStateMovingTo;
		}
		break;

		case EStateMovingTo:
		{
			if ( owner->GetMoveStatus() == MOVE_STATUS_DEST_UNREACHABLE )
			{
				// grayman #3492 - look at the spot

				idVec3 p = _searchSpot;
				p.z += 60; // look up a bit, to simulate searching for the player's head

				if ( owner->CanSeePositionExt(p, false, false) )
				{
					_exitTime = static_cast<int>(gameLocal.time + INVESTIGATE_SPOT_TIME_REMOTE + 1000*(gameLocal.random.RandomFloat() - 0.5f)); // grayman #2640
					owner->TurnToward(p);
					owner->Event_LookAtPosition(p, MS2SEC(_exitTime - gameLocal.time));
					_investigatingState = EStateWaiting;
					return false;
				}
				return true;
			}

			if ( owner->GetMoveStatus() == MOVE_STATUS_DONE )
			{
				// grayman #2928 - don't kneel down if you're too far from the original stim

				float dist = (ownerOrigin - owner->GetMemory().alertSearchCenter).LengthFast();

				// grayman #3563 - don't kneel down if you're drawing a weapon

				if ( _investigateClosely && (dist < INVESTIGATE_SPOT_CLOSELY_MAX_DIST) && (idStr(owner->WaitState()) != "draw") )
				{
					// Stop previous moves
					owner->StopMove(MOVE_STATUS_WAITING);

					// Check the position of the stim, is it closer to the eyes than to the feet?
					// If it's lower than the eye position, kneel down and investigate
					//const idVec3& origin = owner->GetPhysics()->GetOrigin();
					idVec3 eyePos = owner->GetEyePosition();
					if ( (_searchSpot - ownerOrigin).LengthSqr() < (_searchSpot - eyePos).LengthSqr() )
					{
						// Close to the feet, kneel down and investigate closely
						owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_KneelDown", 6);
						owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_KneelDown", 6);
						owner->SetWaitState("kneel_down"); // grayman #3563
						_investigatingState = EStateKneeling;
						return false;
					}
				}

				// Wait a bit, setting _exitTime sets the lifetime of this task
				_exitTime = static_cast<int>(gameLocal.time + INVESTIGATE_SPOT_TIME_STANDARD*(1 + gameLocal.random.RandomFloat()*0.2f));
				_investigatingState = EStateWaiting;
				return false;
			}

			// Still moving.

			// Can we already see the point? Only stop moving when the spot shouldn't be investigated closely
			// angua: added distance check to avoid running in circles if the point is too close to a wall.
			// grayman #2640 - keep moving if you're > INVESTIGATE_SPOT_STOP_DIST from a point you can see

			bool stopping = false;
			if ( !_investigateClosely )
			{
				float distToSpot = (_searchSpot - ownerOrigin).LengthFast();
				if ( owner->CanSeePositionExt(_searchSpot, true, false) ) // grayman #4220 - ignore lighting (you're approaching dark places!)
				{
					if ( distToSpot < INVESTIGATE_SPOT_STOP_DIST )
					{
						stopping = true;
					}
				}
				else if ( distToSpot < INVESTIGATE_SPOT_MIN_DIST )
				{
					stopping = true;
				}
			}

			if ( stopping )
			{
				// Stop moving, we can see the point
				owner->StopMove(MOVE_STATUS_DONE);

				// grayman #3492 - Look at a random point that may be anywhere
				// between the search point and a point 1/2 the AI's height
				// above his eye level.
				// grayman #3857 - now that AI are allowed closer to the search spot,
				// they shouldn't look so high up

				idVec3 p = _searchSpot;
				//float height = owner->GetPhysics()->GetBounds().GetSize().z;
				float bottom = p.z;
				float top = owner->GetEyePosition().z + 20;
				float dist = top - bottom;
				dist *= gameLocal.random.RandomFloat();
				p.z += dist;

				// Look at the point to investigate
				owner->Event_LookAtPosition(p, 3.0f);

				// Wait a bit
				_exitTime = static_cast<int>(gameLocal.time + INVESTIGATE_SPOT_TIME_REMOTE + 1000*(gameLocal.random.RandomFloat() - 0.5f)); // grayman #4220
				_investigatingState = EStateWaiting;
			}
			else if (!owner->AI_RUN) // if walking, see if you should switch to running
			{
				// Run if the point is more than MAX_TRAVEL_DISTANCE_WALKING
				// grayman #4238 - don't run if in less than Agitated Searching mode

				float actualDist = (ownerOrigin - _searchSpot).LengthFast();
				if ( (owner->AI_AlertIndex >= EAgitatedSearching) && (actualDist > MAX_TRAVEL_DISTANCE_WALKING ))
				{
					owner->AI_RUN = true;
				}
			}
		}

		// continue walking

		break;
	case EStateKneeling:
		if ( idStr(owner->WaitState()) != "kneel_down" )
		{
			return true;
		}
	case EStateWaiting:
		if ( gameLocal.time >= _exitTime)
		{
			return true;
		}
	}
	
	return false; // not finished yet
}

void InvestigateSpotTask::SetNewGoal(const idVec3& newPos)
{
	idAI* owner = _owner.GetEntity();
	if (owner == NULL)
	{
		return;
	}

	_searchSpot = newPos;
	_investigatingState = EStateInit;
	_exitTime = 0;
}

void InvestigateSpotTask::SetInvestigateClosely(bool closely)
{
	_investigateClosely = closely;
}

void InvestigateSpotTask::OnFinish(idAI* owner) // grayman #2560
{
	// The action subsystem has finished investigating the spot, set the
	// boolean back to false, so that the next spot can be chosen
	owner->GetMemory().hidingSpotInvestigationInProgress = false;
	owner->GetMemory().currentSearchSpot = idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY);
}

void InvestigateSpotTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteInt(_exitTime);
	savefile->WriteBool(_investigateClosely);
	savefile->WriteVec3(_searchSpot);
	savefile->WriteInt(static_cast<int>(_investigatingState)); // grayman #4220
}

void InvestigateSpotTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadInt(_exitTime);
	savefile->ReadBool(_investigateClosely);
	savefile->ReadVec3(_searchSpot);
	int temp;
	savefile->ReadInt(temp);
	_investigatingState = static_cast<EInvestigateState>(temp); // grayman #4220
}

InvestigateSpotTaskPtr InvestigateSpotTask::CreateInstance()
{
	return InvestigateSpotTaskPtr(new InvestigateSpotTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar investigateSpotTaskRegistrar(
	TASK_INVESTIGATE_SPOT, // Task Name
	TaskLibrary::CreateInstanceFunc(&InvestigateSpotTask::CreateInstance) // Instance creation callback
);

} // namespace ai
