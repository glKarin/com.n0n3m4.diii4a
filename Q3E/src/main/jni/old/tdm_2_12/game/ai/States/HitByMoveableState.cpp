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



#include "../Tasks/SingleBarkTask.h"
#include "HitByMoveableState.h"
#include "../Tasks/IdleAnimationTask.h" // grayman #3857

namespace ai
{

HitByMoveableState::HitByMoveableState()
{}

// Get the name of this state
const idStr& HitByMoveableState::GetName() const
{
	static idStr _name(STATE_HIT_BY_MOVEABLE);
	return _name;
}

// grayman #3559 - make part of WrapUp() visible to outside world

void HitByMoveableState::Cleanup(idAI* owner)
{
	// grayman #4304 - when finished, change who put the tactEnt in motion. It bounced
	// off of me, so I'm now responsible. When it comes to rest, these motion settings
	// will be cleared.

	idEntity* tactEnt = owner->GetMemory().hitByThisMoveable.GetEntity();
	if ( tactEnt )
	{
		tactEnt->m_SetInMotionByActor = owner;
		tactEnt->m_MovedByActor = owner;
	}

	owner->GetMemory().hitByThisMoveable = NULL;
	owner->m_ReactingToHit = false;
	owner->GetMemory().stopReactingToHit = false;

	// grayman #4304 - restart the search for hiding spots, in case an ongoing
	// search was interrupted by getting hit by a moveable
	owner->GetMemory().restartSearchForHidingSpots = true;
}

// Wrap up and end state

void HitByMoveableState::Wrapup(idAI* owner)
{
	Cleanup(owner);
	owner->GetMind()->EndState();
}

void HitByMoveableState::Init(idAI* owner)
{
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("HitByMoveableState initialized.\r");
	assert(owner);

	idEntity* tactEnt = owner->GetMemory().hitByThisMoveable.GetEntity();
	if ( tactEnt == NULL )
	{
		Wrapup(owner); // no entity = no reaction
		return;
	}

	// grayman #3424 - don't process if dead or unconscious
	if ( owner->AI_DEAD || owner->AI_KNOCKEDOUT )
	{
		Wrapup(owner);
		return;
	}

	// stop if something more important has happened
	if (owner->GetMemory().stopReactingToHit)
	{
		Wrapup(owner);
		return;
	}

	if ( !owner->spawnArgs.GetBool("canSearch","1") )
	{
		// TODO: AI that won't search should probably change
		// direction, or flee.

		Wrapup(owner); // won't search, so shouldn't react
		return;
	}

	idVec3 dir = tactEnt->GetPhysics()->GetOrigin() - owner->GetPhysics()->GetOrigin(); // direction to object
	dir.z = 0; // zero vertical component
	dir.NormalizeFast();

	// If the object is above the AI's eyes, assume it arrived
	// from above. Raise the point the AI will look at when he turns back.

	float vertDelta = tactEnt->GetPhysics()->GetOrigin().z - owner->GetEyePosition().z;

	if ( vertDelta > 0 )
	{
		dir.z = 0.5;
		dir.NormalizeFast();
	}

	_pos = owner->GetEyePosition() + dir*HIT_DIST; // where to look when turning back

	_responsibleActor = tactEnt->m_SetInMotionByActor;	// who threw it

	_lookAtDuration   = owner->spawnArgs.GetFloat("hitByMoveableLookAtTime","2.0");   // how long to look at what hit you
	_lookBackDuration = owner->spawnArgs.GetFloat("hitByMoveableLookBackTime","2.0"); // how long to look back at where the object came from

	owner->actionSubsystem->ClearTasks();
	owner->movementSubsystem->ClearTasks();
	owner->searchSubsystem->ClearTasks(); // grayman #3857

	owner->StopMove(MOVE_STATUS_DONE);
	owner->GetMemory().StopReacting();
	owner->GetMemory().stopReactingToHit = false;

	// if AI is sitting or sleeping, he has to stand before looking around

	if ( ( owner->GetMoveType() == MOVETYPE_SIT ) || ( owner->GetMoveType() == MOVETYPE_SLEEP ) )
	{
		owner->GetUp();
		_hitByMoveableState = EStateSittingSleeping;
	}
	else
	{
		_hitByMoveableState = EStateStarting;
	}

	_waitEndTime = gameLocal.time + 500; // pause before reacting
	owner->m_ReactingToHit = true;
}

// Gets called each time the mind is thinking
void HitByMoveableState::Think(idAI* owner)
{
	// grayman #3424 - don't process if dead or unconscious
	if ( owner->AI_DEAD || owner->AI_KNOCKEDOUT )
	{
		Wrapup(owner);
		return;
	}

	// check if something happened to abort the state
	if (owner->GetMemory().stopReactingToHit)
	{
		Wrapup(owner);
		return;
	}

	owner->PerformVisualScan();	// Let the AI check its senses
	if (owner->AI_AlertLevel >= owner->thresh_5) // finished if alert level is too high
	{
		Wrapup(owner);
		return;
	}

	idEntity* tactEnt = owner->GetMemory().hitByThisMoveable.GetEntity();
	if ( tactEnt == NULL )
	{
		Wrapup(owner);
		return;
	}

	switch (_hitByMoveableState)
	{
		case EStateSittingSleeping:
			if (gameLocal.time >= _waitEndTime)
			{
				if (owner->AI_MOVE_DONE && (owner->GetMoveType() != MOVETYPE_GET_UP)) // standing yet?
				{
					_hitByMoveableState = EStateTurnToward;
				}
			}
			break;
		case EStateStarting:
			if (gameLocal.time >= _waitEndTime)
			{
				_hitByMoveableState = EStateTurnToward;
			}
			break;
		case EStateTurnToward: // Turn toward the object that hit you.
			{
				// if _lookAtDuration is 0, skip turning toward and looking at the object

				if ( _lookAtDuration == 0 )
				{
					_hitByMoveableState = EStateTurnBack;
				}
				else
				{
					owner->TurnToward(tactEnt->GetPhysics()->GetOrigin());
					_hitByMoveableState = EStateLookAt;
					_waitEndTime = gameLocal.time + 1000;
				}
			}
			break;
		case EStateLookAt: // look at the entity
			if (gameLocal.time >= _waitEndTime)
			{
				float duration = _lookAtDuration + (_lookAtDuration/5)*(gameLocal.random.RandomFloat() - 0.5f );
				owner->Event_LookAtEntity( tactEnt, duration );

				_waitEndTime = gameLocal.time + duration*1000;
				_hitByMoveableState = EStateTurnBack;
			}
			break;
		case EStateTurnBack: // turn toward the direction the entity came from
			if (gameLocal.time >= _waitEndTime)
			{
				owner->TurnToward(_pos);
				_waitEndTime = gameLocal.time + 1000;
				_hitByMoveableState = EStateLookBack;
			}
			break;
		case EStateLookBack: // look in the direction the entity came from
			if (gameLocal.time >= _waitEndTime)
			{
				if ( _lookBackDuration > 0 )
				{
					float duration = _lookBackDuration + (_lookBackDuration/5)*(gameLocal.random.RandomFloat() - 0.5f );
					owner->Event_LookAtPosition( _pos, duration );
					_waitEndTime = gameLocal.time + duration*1000;
				}
				_hitByMoveableState = EStateFinal;
			}
			break;
		case EStateFinal:
			if (gameLocal.time >= _waitEndTime)
			{
				// If we make it to here, we haven't spotted
				// an enemy, otherwise our alert level would
				// have been high enough to abort this state
				// at the beginning of Think().

				// If we see a friend or a neutral, we assume
				// this was a harmless impact, and we can return
				// to what we were doing.

				idVec3 ownerOrg = owner->GetPhysics()->GetOrigin();
				idBounds bounds( ownerOrg - idVec3( HIT_FIND_THROWER_HORZ, HIT_FIND_THROWER_HORZ, HIT_FIND_THROWER_VERT ), ownerOrg + idVec3( HIT_FIND_THROWER_HORZ, HIT_FIND_THROWER_HORZ, HIT_FIND_THROWER_VERT ) );

				idClip_EntityList ents;
				int num = gameLocal.clip.EntitiesTouchingBounds( bounds, CONTENTS_BODY, ents );

				for ( int i = 0 ; i < num ; i++ )
				{
					idEntity *candidate = ents[i];

					if ( candidate == NULL ) // just in case
					{
						continue;
					}

					if ( candidate == owner ) // skip myself
					{
						continue;
					}

					if ( !candidate->IsType(idActor::Type) ) // skip non-Actors
					{
						continue;
					}

					idActor *candidateActor = static_cast<idActor *>(candidate);

					if ( candidateActor->GetPhysics()->GetMass() <= SMALL_AI_MASS ) // skip actors with small mass (rats, spiders)
					{
						continue;
					}

					if ( candidateActor->IsKnockedOut() || ( candidateActor->health < 0 ) )
					{
						continue; // skip if knocked out or dead
					}

					if ( candidateActor->IsType(idAI::Type) )
					{
						if ( static_cast<idAI*>(candidateActor)->GetMoveType() == MOVETYPE_SLEEP )
						{
							continue; // skip if asleep
						}
					}

					if ( candidateActor->fl.notarget )
					{
						continue; // skip if NOTARGET is set
					}

					if ( owner->IsFriend(candidateActor) || owner->IsNeutral(candidateActor) )
					{
						// Don't admonish the alleged thrower if tactEnt came in from above (dropped)

						if ( _pos.z <= owner->GetEyePosition().z )
						{
							// make sure there's LOS (no walls in between)

							if ( owner->CanSeeExt( candidateActor, false, false ) ) // don't use FOV or lighting, to increase chance of success
							{
								// look at your friend/neutral if they're in your FOV
								int delay = 0; // bark delay
								if ( owner->CanSeeExt( candidateActor, true, false) ) // use FOV this time, but ignore lighting
								{
									owner->Event_LookAtPosition(candidateActor->GetEyePosition(),2.0f);
									delay = 1000; // give head time to move before barking
								}

								// bark an admonishment whether you're facing them or not

								CommMessagePtr message; // no message, but the argument is needed so the start delay can be included
								owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask("snd_admonish_friend",message,delay)));

								if (cv_ai_debug_transition_barks.GetBool())
								{
									gameLocal.Printf("%d: %s hit by moveable, barks 'snd_admonish_friend'\n",gameLocal.time,owner->GetName());
								}

								Wrapup(owner);
								return;
							}
						}
					}
				}

				// No one about, so let's cheat. If a friend or neutral threw
				// the object, we return to what we were doing.

				idActor* responsible = _responsibleActor.GetEntity();

				if ( ( responsible == NULL ) || owner->IsFriend(responsible) || owner->IsNeutral(responsible) )
				{
					Wrapup(owner);
					return;
				}

				// No one about. If an enemy threw the object, start searching.

				if ( owner->IsEnemy(responsible) )
				{
					// NOTE: Latest tactile alert always overides other alerts
					owner->m_TactAlertEnt = tactEnt;
					owner->m_AlertedByActor = responsible;

					// grayman #3857 - set the alert amount to some value in Searching mode
					float newAlertLevel = owner->thresh_3 + (owner->thresh_4 - 0.1 - owner->thresh_3)*owner->GetAcuity("tact");
					float alertIncrement = newAlertLevel - owner->AI_AlertLevel;

					// Check if current alert level is higher than new alert level.

					if (alertIncrement < 0)
					{
						alertIncrement = 0;
					}

					// grayman #3009 - pass the alert position so the AI can look in the direction of who's responsible
					owner->PreAlertAI("tact", alertIncrement, responsible->GetEyePosition()); // grayman #3356

					// grayman #3857 - move alert setup into one method
					SetUpSearchData(EAlertTypeHitByMoveable, _pos, NULL, false, 0);
		
					// Set last visual contact location to this location as that is used in case
					// the target gets away.
					owner->m_LastSight = ownerOrg;

					// If no enemy set so far, set the last visible enemy position.
					if ( owner->GetEnemy() == NULL )
					{
						owner->lastVisibleEnemyPos = ownerOrg;
					}
				}

				Wrapup(owner);
				return;
			}
			break;
		default:
			break;
	}
}

void HitByMoveableState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);
	_responsibleActor.Save(savefile);

	savefile->WriteInt(_waitEndTime);
	savefile->WriteInt(static_cast<int>(_hitByMoveableState));
	savefile->WriteVec3(_pos);
	savefile->WriteFloat(_lookAtDuration);
	savefile->WriteFloat(_lookBackDuration);
}

void HitByMoveableState::Restore(idRestoreGame* savefile)
{
	State::Restore(savefile);
	_responsibleActor.Restore(savefile);

	savefile->ReadInt(_waitEndTime);
	int temp;
	savefile->ReadInt(temp);
	_hitByMoveableState = static_cast<EHitByMoveableState>(temp);
	savefile->ReadVec3(_pos);
	savefile->ReadFloat(_lookAtDuration);
	savefile->ReadFloat(_lookBackDuration);
}

StatePtr HitByMoveableState::CreateInstance()
{
	return StatePtr(new HitByMoveableState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar hitByMoveableStateRegistrar(
	STATE_HIT_BY_MOVEABLE, // Task Name
	StateLibrary::CreateInstanceFunc(&HitByMoveableState::CreateInstance) // Instance creation callback
);

} // namespace ai
