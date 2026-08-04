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



#include "FleeTask.h"
#include "../Memory.h"
#include "../Library.h"
#include "../States/TakeCoverState.h"

namespace ai
{

FleeTask::FleeTask() :
	_escapeSearchLevel(5), // 5 means FIND_FRIENDLY_GUARDED // grayman #3548
	_failureCount(0), // This is used for _escapeSearchLevel 1 only
	_fleeStartTime(gameLocal.time),
	_distOpt(DIST_NEAREST)
{}

// Get the name of this task
const idStr& FleeTask::GetName() const
{
	static idStr _name(TASK_FLEE);
	return _name;
}

void FleeTask::Init(idAI* owner, Subsystem& subsystem)
{
	// Init the base class
	Task::Init(owner, subsystem);

	_enemy = owner->fleeingFromPerson.GetEntity(); // grayman #3847

	// grayman #3848 - set flee distance based on attack type
	if (_enemy.GetEntity() && ( _enemy.GetEntity()->GetNumRangedWeapons() > 0))
	{
		_currentDistanceGoal = FLEE_DIST_MIN_RANGED;
	}
	else
	{
		_currentDistanceGoal = FLEE_DIST_MIN_MELEE;
	}

	_haveTurnedBack = false;  // grayman #3548

	owner->AI_MOVE_DONE = false;
	owner->AI_RUN = true;
}


bool FleeTask::Perform(Subsystem& subsystem)
{
	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);
	Memory& memory = owner->GetMemory();
	_enemy = owner->fleeingFromPerson.GetEntity(); // grayman #3847
	idActor* enemy = _enemy.GetEntity();

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("%s Flee Task performing.\r",owner->name.c_str());

	// angua: bad luck, my friend, you've been too slow...
	// no more fleeing necessary when dead or ko'ed
	if (owner->AI_DEAD || owner->AI_KNOCKEDOUT)
	{
		//owner->fleeingEvent = false; // grayman #3317 // grayman #3548
		//memory.fleeing = false; // grayman #3548
		return true;
	}

	// grayman #3847 - if your enemy is dead or KO'ed, you can quit looking for somewhere to flee to
	if (enemy && (enemy->AI_DEAD || enemy->IsKnockedOut()))
	{
		return true;
	}

	// grayman #3847 - if you're still running, but near your destination, and the enemy is
	// close on your tail, you don't want to stop at the destination. You
	// want to calculate the next point to run to while you're still running.

	bool keepGoing = (
		!owner->fleeingEvent && // you're not fleeing an event
		(owner->GetMoveStatus() == MOVE_STATUS_MOVING) && // still running
		((owner->GetPhysics()->GetOrigin() - owner->GetMoveDest()).LengthSqr() < 10000) && // w/in 100 of goal
		(enemy && (owner->GetPhysics()->GetOrigin() - enemy->GetPhysics()->GetOrigin()).LengthSqr() < FLEE_DIST_MIN_MELEE*FLEE_DIST_MIN_MELEE) // enemy is too close
		);

	// grayman #3548 - when your move is done, turn to face where you came from
	if (owner->AI_MOVE_DONE && !_haveTurnedBack)
	{
		// grayman #3848 - turn back
		// grayman #3847 - face enemy if you have one, even if you can't see him

		if (enemy)
		{
			owner->TurnToward(enemy->GetPhysics()->GetOrigin());
		}
		else
		{
			owner->TurnToward(owner->fleeingFrom);
		}
		_haveTurnedBack = true;
	}

	//gameRenderWorld->DebugText( va("%d  %d",_escapeSearchLevel, _distOpt), owner->GetPhysics()->GetAbsBounds().GetCenter(), 
	// 	1.0f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, USERCMD_MSEC );

	// angua: in any case stop fleeing after max time (1 min).
	// Might stay in flee task forever if pathing to destination not possible otherwise
	// TODO: should be spawn arg, make member 
	int maxFleeTime = 60000;

	if ( ( _failureCount > 5 ) || 
		 ( owner->AI_MOVE_DONE && !owner->AI_DEST_UNREACHABLE && !owner->m_HandlingDoor && !owner->m_HandlingElevator ) ||
		 ( gameLocal.time > _fleeStartTime + maxFleeTime ) || 
		 keepGoing)	// grayman #3847 - you're close to your destination, still moving, and the enemy is also close
	{
		if (!keepGoing) // don't stop if you're supposed to keep going
		{
			owner->StopMove(MOVE_STATUS_DONE);
		}

		// Done fleeing?

		// grayman #3317 - If we were fleeing a murder or KO, quit

		if ( owner->fleeingEvent )
		{
			if (!_haveTurnedBack) // grayman #3548
			{
				// Turn around
				owner->TurnToward(owner->fleeingFrom);
			}
			return true;
		}

		// check if we can see the enemy
		if ( (enemy && owner->AI_ENEMY_VISIBLE)) // grayman #3847
		{
			// grayman #3355 - we might have fled because we were too close to the
			// enemy, and don't have a melee weapon. Check if we're far
			// enough away to use our ranged weapon, if we have one. Don't worry if
			// our health is low or we're a civilian. The combat code will sort it out.

			// grayman #3548 - refactored
			// grayman #3847 - don't stop and turn back if afraid
			if (( owner->GetNumRangedWeapons() > 0 ) && !owner->IsAfraid())
			{
				float dist2Enemy = ( enemy->GetPhysics()->GetOrigin() - owner->GetPhysics()->GetOrigin()).LengthFast();
				if ( dist2Enemy > ( 3 * owner->GetMeleeRange() ) )
				{
					// Turn toward enemy. Note that this is different
					// than just turning to look back the way you came.
					owner->TurnToward(enemy->GetPhysics()->GetOrigin());
					return true;
				}
			}
		}

		if ( keepGoing || (enemy && owner->AI_ENEMY_VISIBLE)) // grayman #3847
		{
			// continue fleeing

			// grayman #3848 - set flee distance based on attack type
			if (_enemy.GetEntity() && ( _enemy.GetEntity()->GetNumRangedWeapons() > 0))
			{
				_currentDistanceGoal = FLEE_DIST_MIN_RANGED;
			}
			else
			{
				_currentDistanceGoal = FLEE_DIST_MIN_MELEE;
			}

			if (_distOpt == DIST_NEAREST)
			{
				// Find fleepoint far away
				_distOpt = DIST_FARTHEST;
				_escapeSearchLevel = 5;
			}
			else if (_escapeSearchLevel > 1)
			{
				_escapeSearchLevel--;
			}
		}

		if ( !keepGoing && enemy && !owner->AI_ENEMY_VISIBLE) // grayman #3847
		{
			if (!_haveTurnedBack) // grayman #3548
			{
				// Turn around to look back to where we came from
				owner->TurnToward(enemy->GetPhysics()->GetOrigin());
				//owner->TurnToward(owner->fleeingFrom);
			}
			return true;
		}
	}

	idVec3 ownerLoc = owner->GetPhysics()->GetOrigin();
	idVec3 threatLoc;
	if ( enemy != NULL )
	{
		threatLoc = enemy->GetPhysics()->GetOrigin();
	}
	else // otherwise use distance to the event we're fleeing
	{
		threatLoc = owner->fleeingFrom; // grayman #3847
	}
	float threatDistance = (ownerLoc - threatLoc).LengthFast();
	
	if (keepGoing || (owner->GetMoveStatus() != MOVE_STATUS_MOVING))
	{
		// If the AI is not running yet, start fleeing
		owner->AI_RUN = true;
		bool success = false; // grayman #3548

		if (_escapeSearchLevel >= 5)
		{
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Trying to find escape route - FIND_FRIENDLY_GUARDED.\r");
			// Flee to the nearest (or farthest, depending on _distOpt) friendly guarded escape point
			if (!owner->Flee(enemy, owner->fleeingEvent, FIND_FRIENDLY_GUARDED, _distOpt))
			{
				_escapeSearchLevel--;
			}
			else // grayman #3548
			{
				DM_LOG(LC_AI, LT_INFO)LOGSTRING("Found a FRIENDLY_GUARDED flee point!\r");
				success = true;
				_haveTurnedBack = false;
			}

			_fleeStartTime = gameLocal.time;
		}
		else if (_escapeSearchLevel == 4)
		{
			// Try to find another escape route
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Trying alternate escape route - FIND_FRIENDLY.\r");
			// Find another escape route to a friendly escape point
			if (!owner->Flee(enemy, owner->fleeingEvent, FIND_FRIENDLY, _distOpt))
			{
				_escapeSearchLevel--;
			}
			else // grayman #3548
			{
				DM_LOG(LC_AI, LT_INFO)LOGSTRING("Found a FRIENDLY flee point!\r");
				success = true;
				_haveTurnedBack = false;
			}
		}
		else if (_escapeSearchLevel == 3)
		{
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Trying alternate escape route - FIND_GUARDED.\r");
			// Flee to the nearest (or farthest, depending on _distOpt) friendly guarded escape point
			if (!owner->Flee(enemy, owner->fleeingEvent, FIND_GUARDED, _distOpt))
			{
				_escapeSearchLevel--;
			}
			else // grayman #3548
			{
				DM_LOG(LC_AI, LT_INFO)LOGSTRING("Found a GUARDED flee point!\r");
				success = true;
				_haveTurnedBack = false;
			}
		}
		else if (_escapeSearchLevel == 2)
		{
			// Try to find another escape route
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Trying alternate escape route - FIND_ANY.\r");
			// Find another escape route to a friendly escape point
			if (!owner->Flee(enemy, owner->fleeingEvent, FIND_ANY, _distOpt))
			{
				_escapeSearchLevel--;
			}
			else // grayman #3548
			{
				DM_LOG(LC_AI, LT_INFO)LOGSTRING("Found ANY flee point!\r");
				success = true;
				_haveTurnedBack = false;
			}
		}
		else // _escapeSearchLevel == 1
		{
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Searchlevel = 1\r");
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("Threat is %f away\r", threatDistance);
			if ( threatDistance < FLEE_DIST_MAX ) // grayman #3548
			{
				if ( ( threatDistance < _currentDistanceGoal ) || ( enemy && (enemy->GetNumRangedWeapons() > 0) ) ) // grayman #3548
				{
					DM_LOG(LC_AI, LT_INFO)LOGSTRING("OMG, Panic mode, gotta run now!\r");
					// Increase the fleeRadius (the nearer the threat, the more)
					// The threat is still near, run farther away
					_currentDistanceGoal += FLEE_DIST_DELTA;
					if (_currentDistanceGoal > FLEE_DIST_MAX)
					{
						_currentDistanceGoal = FLEE_DIST_MAX;
					}

					if (!owner->Flee(enemy, owner->fleeingEvent, FIND_AAS_AREA_FAR_FROM_THREAT, _currentDistanceGoal)) // grayman #3548
					{
						// No point could be found.
						_failureCount++;

						// grayman #3848 - try taking cover if you're fleeing an enemy
						if ((_failureCount >= 5) && enemy)
						{
							if (owner->spawnArgs.GetBool("taking_cover_enabled","0"))
							{
								aasGoal_t hideGoal;
								bool takingCoverPossible = owner->LookForCover(hideGoal, enemy, enemy->GetEyePosition());
								if (takingCoverPossible)
								{
									owner->MoveToPosition(hideGoal.origin);
									success = true;
									_haveTurnedBack = false;
								}
							}
						}
					}
					else // grayman #3548
					{
						// point found - how far away?
						idVec3 goal = owner->GetMoveDest();
						idVec3 owner2goal = goal - ownerLoc;
						float owner2goalDist = owner2goal.LengthFast();
						_currentDistanceGoal = owner2goalDist;
						idVec3 owner2threat = threatLoc - ownerLoc; 
						float owner2threatDist = owner2threat.LengthFast();

						float threat2goalDist = (goal - threatLoc).LengthFast();

						owner2goal.NormalizeFast();
						owner2threat.NormalizeFast();

						if ( threat2goalDist <= owner2threatDist )
						{
							// don't move closer to the enemy,
							// so let's kill the move to the found point
							owner->StopMove(MOVE_STATUS_DONE);
							owner->AI_MOVE_DONE = false;
							_failureCount++;
						}
						else if ( ( owner2threatDist >= FLEE_DIST_MIN_MELEE ) && ( owner2goal * owner2threat > 0.965926 ))
						{
							// don't pass close to the enemy when fleeing,
							// unless they're getting too close,
							// so let's kill the move to the found point
							owner->StopMove(MOVE_STATUS_DONE);
							owner->AI_MOVE_DONE = false;
							_failureCount++;
						}
						else
						{
							success = true;
							_haveTurnedBack = false;
						}
					}
				}
			}
			else
			{
				// Fleeing is done for now. We'll hang around to see if the
				// enemy comes after us, which will mean we have to flee again.
				owner->StopMove(MOVE_STATUS_DONE);
				owner->AI_MOVE_DONE = false;
			}
		}

		// grayman #3548 - If told to run somewhere this frame, and you're
		// handling a door, stop handling the door. If the door is on
		// your escape path, deal with it then.

		if (success && owner->m_HandlingDoor)
		{
			memory.stopHandlingDoor = true;
		}

		if (success)
		{
			_failureCount = 0;
		}
	}

	return false; // not finished yet
}

void FleeTask::OnFinish(idAI* owner) // grayman #3548
{
	Memory& memory = owner->GetMemory();
	memory.fleeing = false;
	owner->fleeingEvent = false;
	owner->fleeingFromPerson = NULL; // grayman #3847
}

void FleeTask::Save(idSaveGame* savefile) const
{
	Task::Save(savefile);

	savefile->WriteInt(_escapeSearchLevel);
	savefile->WriteInt(_failureCount);
	savefile->WriteInt(_fleeStartTime);

	savefile->WriteInt(static_cast<int>(_distOpt));
	savefile->WriteInt(_currentDistanceGoal); // grayman #3548
	savefile->WriteBool(_haveTurnedBack); // grayman #3548

	_enemy.Save(savefile);
}

void FleeTask::Restore(idRestoreGame* savefile)
{
	Task::Restore(savefile);

	savefile->ReadInt(_escapeSearchLevel);
	savefile->ReadInt(_failureCount);
	savefile->ReadInt(_fleeStartTime);

	int distOptInt;
	savefile->ReadInt(distOptInt);
	_distOpt = static_cast<EscapeDistanceOption>(distOptInt);

	savefile->ReadInt(_currentDistanceGoal); // grayman #3548
	savefile->ReadBool(_haveTurnedBack); // grayman #3548

	_enemy.Restore(savefile);
}

FleeTaskPtr FleeTask::CreateInstance()
{
	return FleeTaskPtr(new FleeTask);
}

// Register this task with the TaskLibrary
TaskLibrary::Registrar fleeTaskRegistrar(
	TASK_FLEE, // Task Name
	TaskLibrary::CreateInstanceFunc(&FleeTask::CreateInstance) // Instance creation callback
);

} // namespace ai
