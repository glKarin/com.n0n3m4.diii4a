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



#include "CombatState.h"
#include "../Memory.h"
#include "../../AIComm_Message.h"
//#include "../Tasks/RandomHeadturnTask.h"
#include "../Tasks/ChaseEnemyTask.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Tasks/MeleeCombatTask.h"
#include "../Tasks/RangedCombatTask.h"
#include "../Tasks/ChaseEnemyRangedTask.h"
#include "LostTrackOfEnemyState.h"
#include "AgitatedSearchingState.h"
#include "FleeState.h"
#include "../Library.h"

//#define REACTION_TIME_MIN      100	// grayman #3063
//#define REACTION_TIME_MAX     1000	// grayman #3063 // grayman #3492
#define REACTION_TIME_MIN		1000	// grayman #5019
#define REACTION_TIME_MAX		3000	// grayman #5019
#define REACTION_TIME_SPREAD	1000	// grayman #5019

namespace ai
{

const float s_DOOM_TO_METERS = 0.0254f; // grayman #3063
const float MAX_TRAVEL_DISTANCE_WALKING = 300; // grayman #3848

// Get the name of this state
const idStr& CombatState::GetName() const
{
	static idStr _name(STATE_COMBAT);
	return _name;
}

bool CombatState::CheckAlertLevel(idAI* owner)
{
	if (!owner->m_canSearch) // grayman #3069 - AI that can't search shouldn't be here
	{
		owner->SetAlertLevel(owner->thresh_3 - 0.1);
	}

	if (owner->AI_AlertIndex < ECombat)
	{
		// Alert index is too low for this state, fall back
		owner->GetMemory().alertClass = EAlertNone; // grayman #3182 - no alert idle state rampdown bark
		owner->GetMind()->EndState();
		return false;
	}

	// Alert Index is matching, return OK
	return true;
}

void CombatState::OnTactileAlert(idEntity* tactEnt)
{
	// do nothing as of now, we are already in combat mode
}

void CombatState::OnVisualAlert(idActor* enemy)
{
	// do nothing as of now, we are already in combat mode
}

bool CombatState::OnAudioAlert(idStr soundName, bool addFuzziness, idEntity* maker) // grayman #3847 // grayman #3857
{
	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);

	Memory& memory = owner->GetMemory();

	// If alertClass is not EAlertNone,
	// don't change it to EAlertAudio. Doing so causes
	// the wrong rampdown bark when the AI comes out of a search.
	// grayman #3472 - rampdown bark changes make this check moot

//	if ( memory.alertClass == EAlertNone )
//	{
		memory.alertClass = EAlertAudio;
//	}

	memory.alertPos = owner->GetSndDir();

	if (!owner->AI_ENEMY_VISIBLE)
	{
		if (owner->m_ignorePlayer) // grayman #3063
		{
			memory.lastTimeEnemySeen = gameLocal.time;
		}
		owner->lastReachableEnemyPos = memory.alertPos;
		// gameRenderWorld->DebugArrow(colorRed, owner->GetEyePosition(), memory.alertPos, 2, 1000);
	}
	return true;
}

void CombatState::OnFailedKnockoutBlow(idEntity* attacker, const idVec3& direction, bool hitHead)
{
	// Ignore failed knockout attempts in combat mode
}

void CombatState::OnActorEncounter(idEntity* stimSource, idAI* owner)
{
	if (!stimSource->IsType(idActor::Type))
	{
		return; // No Actor, quit
	}

	if (owner->IsFriend(stimSource))
	{
		// Remember last time a friendly AI was seen
		owner->GetMemory().lastTimeFriendlyAISeen = gameLocal.time;
	}
	// angua: ignore other people during combat
}

void CombatState::Post_OnDeadPersonEncounter(idActor* person, idAI* owner) // grayman #3317
{
	// don't react to a dead person
}

void CombatState::Post_OnUnconsciousPersonEncounter(idActor* person, idAI* owner) // grayman #3317
{
	// don't react to an unconscious person
}

// grayman #3431

void CombatState::OnBlindStim(idEntity* stimSource, bool skipVisibilityCheck)
{
	if ( CanBeBlinded(stimSource, skipVisibilityCheck) )
	{
		// We're about to be blinded. Exit Combat State.
		idAI* owner = _owner.GetEntity();
		owner->GetMind()->EndState();

		// Blind me!
		State::OnBlindStim(stimSource, skipVisibilityCheck);

		//Memory& memory = owner->GetMemory();

		// Forget about the enemy, prevent UpdateEnemyPosition from "cheating".
		owner->ClearEnemy();
		//memory.visualAlert = false; // grayman #2422
		//memory.mandatory = true;	// grayman #3331 // grayman #3857 - now handled by STATE_BLIND
	}
}

void CombatState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	// Set end time to something invalid
	_endTime = -1;

	_endgame = false; // grayman #3848

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("CombatState initialised.\r");
	assert(owner);

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner))
	{
		return;
	}

	if (!owner->GetMind()->PerformCombatCheck())
	{
		return;
	}

	Memory& memory = owner->GetMemory();

	// grayman #3507 - if we were already in combat, and had to pause to
	// deal with an unreachable enemy, and have now come back, our
	// previous state is stored in memory. Use that to skip over the
	// initialization that normally happens when we come up from Agitated Searching.

	if ( memory.combatState != -1 )
	{
		_combatType = COMBAT_NONE;

		// grayman #4184 - If returning from dealing with an unreachable
		// enemy, it's possible that we're still sheathing our weapon because
		// we didn't have time to finish that before leaving the unreachable
		// enemy state. If so, go to the combat state where we're sheathing
		// our weapon.
		if ( idStr(owner->WaitState()) == "sheath" )
		{
			_waitEndTime = gameLocal.time + 2000; // safety net
			_combatSubState = EStateSheathingWeapon;
		}
		else
		{
			_combatSubState = EStateCheckWeaponState;
		}
		return;
	}

	if ( ( owner->GetMoveType() == MOVETYPE_SIT ) || ( owner->GetMoveType() == MOVETYPE_SLEEP) )
	{
		owner->GetUp();
	}

	// grayman #3075 - if we're kneeling, doing close inspection of
	// a spot, stop the animation. Otherwise, the kneeling animation gets
	// restarted a few moments later.

	idStr torsoString = "Torso_KneelDown";
	idStr legsString = "Legs_KneelDown";
	bool torsoKneelingAnim = (torsoString.Cmp(owner->GetAnimState(ANIMCHANNEL_TORSO)) == 0);
	bool legsKneelingAnim = (legsString.Cmp(owner->GetAnimState(ANIMCHANNEL_LEGS)) == 0);

	if ( torsoKneelingAnim || legsKneelingAnim )
	{
		// Reset anims
		owner->StopAnim(ANIMCHANNEL_TORSO, 0);
		owner->StopAnim(ANIMCHANNEL_LEGS, 0);
		owner->SetWaitState(""); // grayman #3848
	}

	// grayman #3472 - kill the repeated bark task
	owner->commSubsystem->ClearTasks(); // grayman #3182

	// grayman #3496 - Enough time passed since last alert bark?
	// grayman #3857 - Enough time passed since last visual stim bark?

	if ( ( gameLocal.time >= memory.lastTimeAlertBark + MIN_TIME_BETWEEN_ALERT_BARKS ) &&
		 ( gameLocal.time >= memory.lastTimeVisualStimBark + MIN_TIME_BETWEEN_ALERT_BARKS ) )
	{
		// grayman #3496 - But only bark if you haven't recently spent time in Agitated Search.

		if ( !memory.agitatedSearched )
		{
			// The communication system plays reaction bark
			CommMessagePtr message;
			owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask("snd_alert5", message)));

			memory.lastTimeAlertBark = gameLocal.time; // grayman #3496

			if (cv_ai_debug_transition_barks.GetBool())
			{
				gameLocal.Printf("%d: %s enters Combat State, barks surprised reaction 'snd_alert5'\n",gameLocal.time,owner->GetName());
			}
		}
		else
		{
			if (cv_ai_debug_transition_barks.GetBool())
			{
				gameLocal.Printf("%d: %s enters Combat State after spending time in Agitated Searching, so won't bark 'snd_alert5'\n",gameLocal.time,owner->GetName());
			}
		}
	}
	else
	{
		if (cv_ai_debug_transition_barks.GetBool())
		{
			gameLocal.Printf("%d: %s enters Combat State, can't bark 'snd_alert5' yet\n",gameLocal.time,owner->GetName());
		}
	}

	// All remaining init code is moved into Think() and done in the EStateReaction substate,
	// because the things it does need to occur after the initial reaction delay.

	// grayman #3063
	// Add a delay before you process the remainder of Init().
	// The length of the delay depends on the distance to the enemy.

	// We have an enemy, store the enemy entity locally
	_enemy = owner->GetEnemy();
	idActor* enemy = _enemy.GetEntity();

	// grayman #3331 - clear combat state
	_combatType = COMBAT_NONE;
	
	// get melee possibilities

	_meleePossible  = ( owner->GetNumMeleeWeapons()  > 0 );
	_rangedPossible = ( owner->GetNumRangedWeapons() > 0 );

	/* grayman #3492 - should wait until after the reaction time before fleeing

	// grayman #3355 - flee if you have no weapons

	if (!_meleePossible && !_rangedPossible)
	{
		owner->fleeingEvent = false; // grayman #3356
		owner->emitFleeBarks = true; // grayman #3474
		owner->GetMind()->SwitchState(STATE_FLEE);
		return;
	} */

	// grayman #3331 - save combat possibilities
	_unarmedMelee = owner->spawnArgs.GetBool("unarmed_melee","0");
	_unarmedRanged = owner->spawnArgs.GetBool("unarmed_ranged","0");
	_armedMelee = _meleePossible && !_unarmedMelee;
	_armedRanged = _rangedPossible && !_unarmedRanged;

	// grayman #3331 - do we need an initial delay at weapon drawing?
	_needInitialDrawDelay = !( owner->GetAttackFlag(COMBAT_MELEE) || owner->GetAttackFlag(COMBAT_RANGED) ); // not if we have a weapon raised

	idVec3 vec2Enemy = enemy->GetPhysics()->GetOrigin() - owner->GetPhysics()->GetOrigin();
	//vec2Enemy.z = 0; // ignore vertical component
	float dist2Enemy = vec2Enemy.LengthFast();
	int reactionTime =  REACTION_TIME_MIN + (dist2Enemy*(REACTION_TIME_MAX - REACTION_TIME_MIN))/(cv_ai_sight_combat_cutoff.GetFloat()/s_DOOM_TO_METERS);
	if ( reactionTime > REACTION_TIME_MAX )
	{
		reactionTime = REACTION_TIME_MAX;
	}

	// grayman #3331 - add a bit of variability so multiple AI spotting the enemy in the same frame aren't in sync

//	reactionTime += gameLocal.random.RandomInt(REACTION_TIME_MAX/2);
	reactionTime += gameLocal.random.RandomInt(REACTION_TIME_SPREAD); // grayman #5019

	_combatSubState = EStateReaction;
	_reactionEndTime = gameLocal.time + reactionTime;
}

// Gets called each time the mind is thinking
void CombatState::Think(idAI* owner)
{
	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner))
	{
//		owner->GetMind()->EndState(); // grayman #3182 - already done in CheckAlertLevel()
		return;
	}

	owner->GetMemory().combatState = (int)_combatSubState; // grayman #3507

	// grayman #3331 - make sure you're still fighting the same enemy.
	idActor* enemy = _enemy.GetEntity();
	idActor* newEnemy = owner->GetEnemy();

	if (!_endgame)
	{
		if ( enemy )
		{
			if ( newEnemy && ( newEnemy != enemy ) )
			{
				// grayman #3355 - fight the closest enemy
				idVec3 ownerOrigin = owner->GetPhysics()->GetOrigin();
				float dist2EnemySqr = ( enemy->GetPhysics()->GetOrigin() - ownerOrigin ).LengthSqr();
				float dist2NewEnemySqr = ( newEnemy->GetPhysics()->GetOrigin() - ownerOrigin ).LengthSqr();
				if ( dist2NewEnemySqr < dist2EnemySqr )
				{
					owner->GetMind()->EndState();
					return; // state has ended
				}
			}
		}
		else
		{
			enemy = newEnemy;
		}
	}
	else // in endgame
	{
		if (newEnemy)
		{
			// grayman #3847 - if kneeling, abort the kneel anim

			if ( idStr(owner->WaitState(ANIMCHANNEL_TORSO)) == "kneel_down" )
			{
				// Reset anims
				owner->StopAnim(ANIMCHANNEL_TORSO, 0);
				owner->StopAnim(ANIMCHANNEL_LEGS, 0);
				owner->SetWaitState(""); // grayman #3848
			}

			// abandon the endgame and fight your new enemy
			owner->GetMind()->EndState();
			return;
		}
	}

	// grayman #3848 - are we in the endgame of combat, where we've
	// killed the enemy and are performing a few final tasks?

	if (_endgame)
	{
		idActor* victim = owner->m_lastKilled.GetEntity();
		switch(_combatSubState)
		{
		case EStateVictor1:
			if (gameLocal.time >= _endTime)
			{
				_combatSubState = EStateVictor2;
			}
			break;
		case EStateVictor2:
			{
			// Bark your victory!

			// only allow the killer to bark

			idStr bark = "";
			idStr enemyAiUse = victim->spawnArgs.GetString("AIUse");
			bool monster = ( ( enemyAiUse == AIUSE_MONSTER ) || ( enemyAiUse == AIUSE_UNDEAD ) );

			if (owner == victim->m_killedBy.GetEntity())
			{
				// different barks for human and non-human enemies

				if ( monster )
				{
					bark = "snd_killed_monster";
				}
				else
				{
					bark = "snd_killed_enemy";
				}
			}

			if (!bark.IsEmpty())
			{
				int length = owner->PlayAndLipSync(bark, "talk1", 0);
				_endTime = gameLocal.time + length; // ignored in EStateVictor3 and used in EStateVictor8
			}

			if ( monster )
			{
				_combatSubState = EStateVictor8; // don't walk to monsters
			}
			else
			{
				_combatSubState = EStateVictor3; // walk to humans
			}

			break;
			}
		case EStateVictor3:
			{
			_destination = victim->GetPhysics()->GetOrigin();
			idVec3 ownerOrigin = owner->GetPhysics()->GetOrigin();
			idVec3 vec2Victim = _destination - ownerOrigin;
			float dist2Victim = vec2Victim.LengthFast();
			if (dist2Victim > 64)
			{
				// Can we walk to the victim?
				// Don't step up to the very spot, to prevent the AI
				// from kneeling into bloodspots or corpses
				idVec3 dir = -vec2Victim;
				dir.z = 0; // remove vertical component
				dir.NormalizeFast();

				// 32 units before the actual spot
				_destination += dir * 32;
				bool canWalkTo = owner->MoveToPosition(_destination);
				if (canWalkTo)
				{
					float actualDist = (ownerOrigin - _destination).LengthFast();
					owner->AI_RUN = ( actualDist > MAX_TRAVEL_DISTANCE_WALKING ); // close enough to walk?
					_combatSubState = EStateVictor4;
				}
				else
				{
					_combatSubState = EStateVictor9;
				}
			}
			else
			{
				_combatSubState = EStateVictor5;
			}
			break;
			}
		case EStateVictor4:
			if (owner->ReachedPos(_destination, MOVE_TO_POSITION)) // grayman #3848
			{
				// Re-check where the body is. It might have been sliding down some
				// steps and is in a different place than _destination.

				float dist2Victim = (victim->GetPhysics()->GetOrigin() - owner->GetPhysics()->GetOrigin()).LengthFast();
				if (dist2Victim > 64)
				{
					_combatSubState = EStateVictor3; // too far away, so get closer
				}
				else
				{
					_combatSubState = EStateVictor5; // close enough
				}
			}
			else if (owner->MoveDone())
			{
				_combatSubState = EStateVictor9;
			}
			break;
		case EStateVictor5:
			{
			idVec3 bodyCenter = victim->GetPhysics()->GetAbsBounds().GetCenter();
			owner->TurnToward(bodyCenter);
			_endTime = gameLocal.time + 750; // allow time for turn to complete
			_combatSubState = EStateVictor6;
			break;
			}
		case EStateVictor6:
			if (gameLocal.time >= _endTime)
			{
				idVec3 bodyCenter = victim->GetPhysics()->GetAbsBounds().GetCenter();
				owner->Event_LookAtPosition(bodyCenter, 2.0f);
				// There's a 50% chance of kneeling and kneel only if no one else has knealt
				if ( (gameLocal.random.RandomFloat() < 0.5) && !victim->m_victorHasKnealt)
				{
					// Check the position of the body, is it closer to the eyes than to the feet?
					// If it's lower than the eye position, kneel down and investigate
					idVec3 ownerOrigin = owner->GetPhysics()->GetOrigin();
					idVec3 eyePos = owner->GetEyePosition();
					if ((bodyCenter - ownerOrigin).LengthSqr() < (bodyCenter - eyePos).LengthSqr())
					{
						// Close to the feet, kneel down and investigate closely
						owner->SetAnimState(ANIMCHANNEL_TORSO, "Torso_KneelDown", 6);
						owner->SetAnimState(ANIMCHANNEL_LEGS, "Legs_KneelDown", 6);
						owner->SetWaitState("kneel_down"); // grayman #3563
						victim->m_victorHasKnealt = true;
						_combatSubState = EStateVictor7;
					}
					else
					{
						_combatSubState = EStateVictor9;
					}
				}
				else
				{
					_combatSubState = EStateVictor9;
				}
			}
			break;
		case EStateVictor7:
			if (idStr(owner->WaitState()) != "kneel_down")
			{
				_combatSubState = EStateVictor9;
			}
			break;
		case EStateVictor8:
			if (gameLocal.time >= _endTime)
			{
				_combatSubState = EStateVictor9;
			}
			break;
		case EStateVictor9:
			owner->SetAlertLevel(owner->thresh_1 + (owner->thresh_2 - owner->thresh_1) * 0.9);
			owner->GetMind()->EndState(); // end combat state
			break;
		}
		return;
	}

	if (!CheckEnemyStatus(enemy, owner))
	{
		owner->GetMind()->EndState();
		return; // state has ended
	}

	// grayman #3520 - don't look toward new alerts
	if ( owner->m_lookAtAlertSpot )
	{
		owner->m_lookAtAlertSpot = false;
		owner->m_lookAtPos = idVec3(idMath::INFINITY,idMath::INFINITY,idMath::INFINITY);
	}

	// angua: look at enemy
	owner->Event_LookAtPosition( enemy->GetEyePosition(), USERCMD_MSEC );

	Memory& memory = owner->GetMemory();

	idVec3 vec2Enemy = enemy->GetPhysics()->GetOrigin() - owner->GetPhysics()->GetOrigin();
	float dist2Enemy = vec2Enemy.LengthFast();

	// grayman #3331 - need to take vertical separation into account. It's possible to have the origins
	// close enough to be w/in the melee zone, but still be unable to hit the enemy.

	bool inMeleeRange = ( dist2Enemy <= ( 3 * owner->GetMeleeRange() ) );

	ECombatType newCombatType;
	
	if ( inMeleeRange && !_meleePossible ) // grayman #3355 - can't fight up close
	{
		owner->fleeingEvent = false; // grayman #3356
		owner->fleeingFrom = enemy->GetPhysics()->GetOrigin(); // grayman #3848
		owner->fleeingFromPerson = enemy; // grayman #3847
		// grayman #3548 - allow flee bark if unarmed
		owner->emitFleeBarks = !_rangedPossible;
		if (!memory.fleeing) // grayman #3847 - only flee if not already fleeing
		{
			owner->GetMind()->SwitchState(STATE_FLEE);
			owner->m_ignorePlayer = false;	// dragofer #5286: make sure ignorePlayer flag is cleared by the time the AI begins to flee
		}
		return;
	}

	if ( !inMeleeRange && _rangedPossible )
	{
		newCombatType = COMBAT_RANGED;
	}
	else
	{
		newCombatType = COMBAT_MELEE;
	}

	// Check for situation where you're in the melee zone, yet you're unable to hit
	// the enemy. This can happen if the enemy is above or below you and you can't
	// reach them.

	switch(_combatSubState)
	{
	case EStateReaction:
		{
		if ( gameLocal.time < _reactionEndTime )
		{
			return; // stay in this state until the reaction time expires
		}

		// Check to see if the enemy is still visible.
		// grayman #2816 - Visibility doesn't matter if you're in combat because
		// you bumped into your enemy.

		idEntity* tactEnt = owner->GetTactEnt();
		if ( ( tactEnt == NULL ) || !tactEnt->IsType(idActor::Type) || ( tactEnt != enemy ) || !owner->AI_TACTALERT ) 
		{
			// Check the candidate's visibility.

			// grayman #3705 - change the way the player's visibility is checked,
			// so it more closely matches how the player is spotted before the
			// AI enters combat.

			bool fail;
			if (enemy->IsType(idPlayer::Type))
			{
				fail = (owner->GetVisibility(enemy) == 0.0f);
			}
			else // enemy is another AI
			{
				fail = !owner->CanSee(enemy, true);
			}

			if (fail)
			{
				owner->ClearEnemy();
				owner->SetAlertLevel(owner->thresh_5 - 0.1); // reset alert level just under Combat

				// grayman #3857 - dropping to agitated searching, so we need search parameters

				memory.alertClass = EAlertVisual_1;
				memory.alertType = EAlertTypeEnemy; // grayman #3857
				memory.alertPos = owner->GetPhysics()->GetOrigin();
				//memory.alertRadius = LOST_ENEMY_ALERT_RADIUS;
				memory.alertSearchVolume = LOST_ENEMY_SEARCH_VOLUME;
				memory.alertSearchExclusionVolume.Zero();
				memory.alertedDueToCommunication = false;
				memory.stimulusLocationItselfShouldBeSearched = true;
				memory.investigateStimulusLocationClosely = false; // grayman #3857
				memory.mandatory = false; // grayman #3857

				// Log the event
				memory.currentSearchEventID = owner->LogSuspiciousEvent( E_EventTypeEnemy, memory.alertPos, enemy, true ); // grayman #3857

				owner->GetMind()->EndState();
				return;
			}

			/* old way
			if ( !owner->CanSee(enemy, true) )
			{
				owner->ClearEnemy();
				owner->SetAlertLevel(owner->thresh_5 - 0.1); // reset alert level just under Combat
				owner->GetMind()->EndState();
				return;
			}
			*/
		}

		owner->m_ignorePlayer = false; // grayman #3063 - clear flag that prevents mission statistics on player sightings

		// The AI has processed his reaction, and needs to move into combat, or flee.

		//_criticalHealth = owner->spawnArgs.GetInt("health_critical", "0");

		// greebo: Check for weapons and flee if ...
		if ( owner->IsAfraid()) // grayman #3848
		{
			owner->fleeingEvent = false; // grayman #3356
			owner->fleeingFrom = enemy->GetPhysics()->GetOrigin();
			owner->fleeingFromPerson = enemy; // grayman #3847
			owner->emitFleeBarks = true; // grayman #3474
			if (!memory.fleeing) // grayman #3847 - only flee if not already fleeing
			{
				owner->GetMind()->SwitchState(STATE_FLEE);
			}
			return;
		}

		memory.StopReacting(); // grayman #3559

		_combatSubState = EStateDoOnce;
		break;
		}

	case EStateDoOnce:
		{

		// grayman #3857 - If participating in a search, leave the search
		if (owner->m_searchID > 0)
		{
			gameLocal.m_searchManager->LeaveSearch(owner->m_searchID, owner);
		}

		// Check for sitting or sleeping

		moveType_t moveType = owner->GetMoveType();
		if (   moveType == MOVETYPE_SIT 
			|| moveType == MOVETYPE_SLEEP
			|| moveType == MOVETYPE_SIT_DOWN
			|| moveType == MOVETYPE_FALL_ASLEEP ) // grayman #3820 - was MOVETYPE_LAY_DOWN
		{
			owner->GetUp(); // okay if called multiple times
			return;
		}

		// grayman #3009 - check for getting up from sitting or sleeping
		if ( moveType == MOVETYPE_GET_UP || 
			 moveType == MOVETYPE_WAKE_UP ) // grayman #3820 - was MOVETYPE_GET_UP_FROM_LYING
		{
			return;
		}

		// not sitting or sleeping at this point

		// Stop what you're doing
		owner->StopMove(MOVE_STATUS_DONE);
		owner->movementSubsystem->ClearTasks();
		owner->senseSubsystem->ClearTasks();
		owner->actionSubsystem->ClearTasks();
		owner->searchSubsystem->ClearTasks(); // grayman #3857

		// Bark

		// This will hold the message to be delivered with the bark, if appropriate
		CommMessagePtr message;
	
		// Only alert the bystanders if we didn't receive the alert by message ourselves
		if (!memory.alertedDueToCommunication)
		{
			message = CommMessagePtr(new CommMessage(
				CommMessage::DetectedEnemy_CommType, 
				owner, NULL, // from this AI to anyone 
				enemy,
				memory.lastEnemyPos,
				memory.currentSearchEventID // grayman #3857
			));
		}

		// grayman #3496 - This is an alert bark, but it's not subject to the delay
		// between alert barks because the previous bark will end before we get here.
		// It's also the culmination of a journey up from Idle State, and if the AI is
		// going to go after an enemy, this bark needs to be heard.
		// We'll still reset the last alert bark time, though.

		// The communication system plays starting bark

		// grayman #3343 - accommodate different barks for human and non-human enemies

		idPlayer* player(NULL);
		if (enemy->IsType(idPlayer::Type))
		{
			player = static_cast<idPlayer*>(enemy);
		}

		idStr bark = "";

		if (player && player->m_bShoulderingBody)
		{
			bark = "snd_spotted_player_with_body";
		}
		else if ((MS2SEC(gameLocal.time - memory.lastTimeFriendlyAISeen)) <= MAX_FRIEND_SIGHTING_SECONDS_FOR_ACCOMPANIED_ALERT_BARK)
		{
			idStr enemyAiUse = enemy->spawnArgs.GetString("AIUse");
			if ( ( enemyAiUse == AIUSE_MONSTER ) || ( enemyAiUse == AIUSE_UNDEAD ) )
			{
				bark = "snd_to_combat_company_monster";
			}
			else
			{
				bark = "snd_to_combat_company";
			}
		}
		else
		{
			idStr enemyAiUse = enemy->spawnArgs.GetString("AIUse");
			if ( ( enemyAiUse == AIUSE_MONSTER ) || ( enemyAiUse == AIUSE_UNDEAD ) )
			{
				bark = "snd_to_combat_monster";
			}
			else
			{
				bark = "snd_to_combat";
			}
		}

		owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask(bark, message)));

		owner->GetMemory().lastTimeAlertBark = gameLocal.time; // grayman #3496

		if (cv_ai_debug_transition_barks.GetBool())
		{
			gameLocal.Printf("%d: %s starts combat, barks '%s'\n",gameLocal.time,owner->GetName(),bark.c_str());
		}

		_justDrewWeapon = false;
		_combatSubState = EStateCheckWeaponState;
		break;
		}

	case EStateCheckWeaponState:
		// check which combat type we should use
		{
		// Can you continue with your current combat type, and not have to switch weapons?

		// Check for case where melee combat has stalled. You're in the melee zone, but you're
		// unable to hit the enemy. Perhaps he's higher or lower than you and you can't reach him.

		if ( !owner->AI_FORWARD && // not moving
			 ( _combatType == COMBAT_MELEE ) && // in melee combat
			 _rangedPossible &&    // ranged combat is possible
			 !owner->TestMelee() ) // I can't hit the enemy
		{
			float        orgZ = owner->GetPhysics()->GetOrigin().z;
			float      height = owner->GetPhysics()->GetBounds().GetSize().z;
			float   enemyOrgZ = enemy->GetPhysics()->GetOrigin().z;
			float enemyHeight = enemy->GetPhysics()->GetBounds().GetSize().z;
			if ( ( (orgZ + height + owner->melee_range_vert) < enemyOrgZ ) || // enemy too high
							     ( (enemyOrgZ + enemyHeight) < orgZ ) ) // enemy too low
			{
				newCombatType = COMBAT_RANGED;
			}
		}

		if ( newCombatType == _combatType )
		{
			// yes - no need to run weapon-switching animations
			_combatSubState = EStateCombatAndChecks;
			return;
		}

		// Do you need to switch a melee or ranged weapon? You might already have one
		// drawn, or you might have none drawn, or you might have to change weapons,
		// or you might be using unarmed attacks, and you don't need a drawn weapon.

		// Check for unarmed combat.

		if ( _unarmedMelee && ( newCombatType == COMBAT_MELEE ) )
		{
			// unarmed combat doesn't need attached weapons
			_combatType = COMBAT_NONE; // clear ranged combat tasks and start melee combat tasks
			_combatSubState = EStateCombatAndChecks;
			return;
		}

		if ( _unarmedRanged && ( newCombatType == COMBAT_RANGED ) )
		{
			// unarmed combat doesn't need attached weapons
			_combatType = COMBAT_NONE; // clear melee combat tasks and start ranged combat tasks
			_combatSubState = EStateCombatAndChecks;
			return;
		}

		// Do you have a drawn weapon?

		if ( owner->GetAttackFlag(COMBAT_MELEE) && ( newCombatType == COMBAT_MELEE ) )
		{
			// melee weapon is already drawn
			_combatSubState = EStateCombatAndChecks;
			return;
		}

		if ( owner->GetAttackFlag(COMBAT_RANGED) && ( newCombatType == COMBAT_RANGED ) )
		{
			// ranged weapon is already drawn
			_combatSubState = EStateCombatAndChecks;
			return;
		}

		// At this point, we know we need to draw a weapon that's not already drawn.
		// See if you need to sheathe a drawn weapon.

		if ( ( ( newCombatType == COMBAT_RANGED ) && owner->GetAttackFlag(COMBAT_MELEE)  ) ||
		     ( ( newCombatType == COMBAT_MELEE  ) && owner->GetAttackFlag(COMBAT_RANGED) ) )
		{
			// switch from one type of weapon to another
			owner->movementSubsystem->ClearTasks();
			owner->actionSubsystem->ClearTasks();
			_combatType = COMBAT_NONE;

			// sheathe current weapon so you can draw the other weapon
			owner->SheathWeapon();
			_waitEndTime = gameLocal.time + 2000; // safety net
			_combatSubState = EStateSheathingWeapon;
			return;
		}

		// No need to sheathe a weapon
		_combatSubState = EStateDrawWeapon;

		break;
		}

	case EStateSheathingWeapon:
		{
		// if you're sheathing a weapon, stay in this state until it's done, or until the timer expires
		// grayman #3355 - check wait state
		if ( ( gameLocal.time < _waitEndTime ) && ( idStr(owner->WaitState()) == "sheath") )
		{
			return;
		}

		_combatSubState = EStateDrawWeapon;

		break;
		}

	case EStateDrawWeapon:
		{
		// grayman #3331 - if you don't already have the correct weapon drawn,
		// draw a ranged weapon if you're far from the enemy, and you have a 
		// ranged weapon, otherwise draw your melee weapon

		bool drawingWeapon = false;

		if ( !inMeleeRange )
		{
			// beyond melee range
			if ( !owner->GetAttackFlag(COMBAT_RANGED) && _rangedPossible )
			{
				drawingWeapon = owner->DrawWeapon(COMBAT_RANGED); // grayman #3775
				if (!drawingWeapon) // grayman #3775
				{
					break; // need to leave early and come back here again
				}
				//drawingWeapon = true;
			}
			else // no ranged weapon
			{
				drawingWeapon = owner->DrawWeapon(COMBAT_MELEE); // grayman #3775
				if (!drawingWeapon) // grayman #3775
				{
					break; // need to leave early and come back here again
				}
				//drawingWeapon = true;
			}
		}
		else // in melee range
		{
			if ( _meleePossible && !owner->GetAttackFlag(COMBAT_MELEE) )
			{
				drawingWeapon = owner->DrawWeapon(COMBAT_MELEE); // grayman #3775
				if (!drawingWeapon) // grayman #3775
				{
					break; // need to leave early and come back here again
				}
				//drawingWeapon = true;
			}
		}

		// grayman #3331 - if this is the first weapon draw, to make sure the weapon is drawn
		// before starting combat, delay some before starting to chase the enemy.
		// The farther away the enemy is, the better the chance that you'll start chasing before your
		// weapon is drawn. If he's close, this gives you time to completely draw your weapon before
		// engaging him. The interesting distance is how far you have to travel to get w/in melee range.

		if ( _needInitialDrawDelay ) // True if this is the first time through, and you don't already have a raised weapon
		{
			int delay = 0;

			if ( drawingWeapon )
			{
				delay = (int)(2064.0f - 20.0f*(dist2Enemy - owner->GetMeleeRange()));
				if ( delay < 0 )
				{
					delay = gameLocal.random.RandomInt(2064);
				}
				_waitEndTime = gameLocal.time + delay;
				_combatSubState = EStateDrawingWeapon;

				// grayman #3563 - safety net when drawing a weapon
				_drawEndTime = gameLocal.time + MAX_DRAW_DURATION;
			}
			else
			{
				_combatSubState = EStateCombatAndChecks;
			}
			_needInitialDrawDelay = false; // No need to do this again
		}
		else
		{
			if ( drawingWeapon )
			{
				_waitEndTime = gameLocal.time;
				_combatSubState = EStateDrawingWeapon;

				// grayman #3563 - safety net when drawing a weapon
				_drawEndTime = gameLocal.time + MAX_DRAW_DURATION;
			}
			else
			{
				_combatSubState = EStateCombatAndChecks;
			}
		}
		break;
		}

	case EStateDrawingWeapon:
		{
		// grayman #3355 - check wait state
		if ( idStr(owner->WaitState()) == "draw" )
		{
			if ( gameLocal.time < _drawEndTime ) // grayman #3563 - check safety net
			{
				return; // wait until weapon is drawn
			}
		}

		if ( gameLocal.time < _waitEndTime )
		{
			return; // wait until timer expires
		}

		// Weapon is now drawn

		_justDrewWeapon = true;
		_combatSubState = EStateCombatAndChecks;

		break;
		}

	case EStateCombatAndChecks:
		{
		// Need to check if a weapon that was just drawn is correct for the zone you're now in, in case
		// you started drawing the correct weapon for one zone, and while it was drawing, you switched
		// to the other zone.
		
		if ( _justDrewWeapon )
		{
			if ( newCombatType == COMBAT_RANGED )
			{
				// beyond melee range
				if ( !owner->GetAttackFlag(COMBAT_RANGED) && _rangedPossible )
				{
					// wrong weapon raised - go back and get the correct one
					_justDrewWeapon = false;
					_combatSubState = EStateCheckWeaponState;
					return;
				}
			}
			else // in melee combat
			{
				if ( !owner->GetAttackFlag(COMBAT_MELEE) && _meleePossible )
				{
					// wrong weapon raised - go back and get the correct one
					_justDrewWeapon = false;
					_combatSubState = EStateCheckWeaponState;
					return;
				}
			}
		}

		_justDrewWeapon = false;
		if ( _combatType == COMBAT_NONE ) // Either combat hasn't been initially set up, or you're switching weapons
		{
			if ( newCombatType == COMBAT_RANGED )
			{
				// Set up ranged combat
				owner->actionSubsystem->PushTask(RangedCombatTask::CreateInstance());
				owner->movementSubsystem->PushTask(ChaseEnemyRangedTask::CreateInstance());
				_combatType = COMBAT_RANGED;
			}
			else
			{
				// Set up melee combat
				ChaseEnemyTaskPtr chaseEnemy = ChaseEnemyTask::CreateInstance();
				chaseEnemy->SetEnemy(enemy);
				owner->movementSubsystem->PushTask(chaseEnemy);

				owner->actionSubsystem->PushTask(MeleeCombatTask::CreateInstance());
				_combatType = COMBAT_MELEE;
			}

			// Let the AI update their weapons (make them nonsolid)
			owner->UpdateAttachmentContents(false);
		}

		// Check the distance to the enemy, the subsystem tasks need it.
		memory.canHitEnemy = owner->CanHitEntity(enemy, _combatType);
		// grayman #3331 - willBeAbleToHitEnemy is only relevant if canHitEnemy is FALSE
		if ( owner->m_bMeleePredictProximity && !memory.canHitEnemy )
		{
			memory.willBeAbleToHitEnemy = owner->WillBeAbleToHitEntity(enemy, _combatType);
		}

		// Check whether the enemy can hit us in the near future
		memory.canBeHitByEnemy = owner->CanBeHitByEntity(enemy, _combatType);

		if ( !owner->AI_ENEMY_VISIBLE &&
			 ( ( ( _combatType == COMBAT_MELEE )  && !memory.canHitEnemy ) || ( _combatType == COMBAT_RANGED) ) )
		{
			// The enemy is not visible, let's keep track of him for a small amount of time
			if (gameLocal.time - memory.lastTimeEnemySeen < MAX_BLIND_CHASE_TIME)
			{
				// Cheat a bit and take the last reachable position as "visible & reachable"
				owner->lastVisibleReachableEnemyPos = owner->lastReachableEnemyPos;
			}
			else if (owner->ReachedPos(owner->lastVisibleReachableEnemyPos, MOVE_TO_POSITION) || 
					( ( gameLocal.time - memory.lastTimeEnemySeen ) > 2 * MAX_BLIND_CHASE_TIME) )
			{
				// BLIND_CHASE_TIME has expired, we have lost the enemy!
				owner->GetMind()->SwitchState(STATE_LOST_TRACK_OF_ENEMY);
				return;
			}
		}

		// Flee if you're damaged and the current action is finished
		// grayman #3848 - allow archers to flee
		if (owner->health < owner->spawnArgs.GetInt("health_critical", "0") )
		{
			bool flee = false;
			if (( _combatType == COMBAT_MELEE ) && ( owner->m_MeleeStatus.m_ActionState == MELEEACTION_READY ) ) // not swinging weapon
			{
				flee = true;
			}
			else if (( _combatType == COMBAT_RANGED ) && ( idStr(owner->WaitState()) != "ranged_attack")) // not shooting a missile
			{
				flee = true;
			}

			if (flee)
			{
				DM_LOG(LC_AI, LT_INFO)LOGSTRING("I'm badly hurt, I'm afraid, and am fleeing!\r");
				owner->fleeingEvent = false; // grayman #3182
				owner->fleeingFrom = enemy->GetPhysics()->GetOrigin(); // grayman #3848
				owner->fleeingFromPerson = enemy; // grayman #3847
				owner->emitFleeBarks = true; // grayman #3474
				if (!memory.fleeing) // grayman #3847 - only flee if not already fleeing
				{
					owner->GetMind()->SwitchState(STATE_FLEE);
				}
				return;
			}
		}

		_combatSubState = EStateCheckWeaponState;

		break;
		}

	default:
		break;
	}
}

bool CombatState::CheckEnemyStatus(idActor* enemy, idAI* owner)
{
	if (enemy == NULL)
	{
		DM_LOG(LC_AI, LT_ERROR)LOGSTRING("No enemy, terminating task!\r");
		owner->GetMind()->EndState();
		return false;
	}

	if (enemy->AI_DEAD)
	{
		// grayman #2816 - remember the last enemy killed
		owner->SetLastKilled(enemy);

		owner->ClearEnemy();
		owner->StopMove(MOVE_STATUS_DONE);

		// Stop fighting
		owner->actionSubsystem->ClearTasks();

		// TODO: Check if more enemies are in range

		// grayman #3848 - don't bring the alert level down yet
		//owner->SetAlertLevel(owner->thresh_1 + (owner->thresh_2 - owner->thresh_1) * 0.9);

		// grayman #3473 - stop looking at the spot you were looking at when you killed the enemy
		owner->SetFocusTime(gameLocal.time);

		/* grayman #3848 - moved into the endgame code
		// grayman #2816 - need to delay the victory bark, because it's
		// being emitted too soon. Can't simply put a delay on SingleBarkTask()
		// because the AI clears his communication tasks when he drops back into
		// Observant mode, which wipes out the victory bark.
		// We need to post an event for later and emit the bark then.

		// new way

		// grayman #3343 - accommodate different barks for human and non-human enemies

		idStr bark = "";
		idStr enemyAiUse = enemy->spawnArgs.GetString("AIUse");
		if ( ( enemyAiUse == AIUSE_MONSTER ) || ( enemyAiUse == AIUSE_UNDEAD ) )
		{
			bark = "snd_killed_monster";
		}
		else
		{
			bark = "snd_killed_enemy";
		}
		owner->PostEventMS(&AI_Bark,ENEMY_DEAD_BARK_DELAY,bark);
		*/

/* old way
		// Emit the killed enemy bark
		owner->commSubsystem->AddCommTask(
			CommunicationTaskPtr(new SingleBarkTask("snd_killed_enemy"))
		);
 */
		_endgame = true; // grayman #3848
		_endTime = gameLocal.time + 1500 + gameLocal.random.RandomInt(1500);
		_combatSubState = EStateVictor1;

		return true; // enemy is dead, but there's still more to do
	}

	if (!owner->IsEnemy(enemy))
	{
		// angua: the relation to the enemy has changed, this is not an enemy any more
		owner->StopMove(MOVE_STATUS_DONE);
		owner->SetAlertLevel(owner->thresh_2 + (owner->thresh_3 - owner->thresh_2) * 0.9);
		owner->ClearEnemy();
		owner->GetMind()->EndState();
		
		owner->movementSubsystem->ClearTasks();
		owner->senseSubsystem->ClearTasks();
		owner->actionSubsystem->ClearTasks();
		owner->searchSubsystem->ClearTasks(); // grayman #3857

		return false;
	}

	return true; // Enemy still alive and kicking
}

// grayman #2924 - don't process any stim that ends up here; we're busy

void CombatState::DelayedVisualStim(idEntity* stimSource, idAI* owner)
{
	stimSource->AllowResponse(ST_VISUAL, owner); // see it again later
}

void CombatState::OnVisualStimBlood(idEntity* stimSource, idAI* owner) {}

void CombatState::Save(idSaveGame* savefile) const
{
	State::Save(savefile);

	//savefile->WriteInt(_criticalHealth);
	savefile->WriteBool(_meleePossible);
	savefile->WriteBool(_rangedPossible);
	savefile->WriteInt(static_cast<int>(_combatType));
	_enemy.Save(savefile);
	savefile->WriteInt(_endTime);
	savefile->WriteBool(_endgame);			// grayman #3848
	savefile->WriteVec3(_destination);		// grayman #3848
	savefile->WriteInt(static_cast<int>(_combatSubState)); // grayman #3063
	savefile->WriteInt(_reactionEndTime); // grayman #3063
	savefile->WriteInt(_waitEndTime);	  // grayman #3331
	savefile->WriteBool(_needInitialDrawDelay); // grayman #3331
	savefile->WriteBool(_unarmedMelee);		// grayman #3331
	savefile->WriteBool(_unarmedRanged);	// grayman #3331
	savefile->WriteBool(_armedMelee);		// grayman #3331
	savefile->WriteBool(_armedRanged);		// grayman #3331
	savefile->WriteBool(_justDrewWeapon);	// grayman #3331
}

void CombatState::Restore(idRestoreGame* savefile)
{
	State::Restore(savefile);

	//savefile->ReadInt(_criticalHealth);
	savefile->ReadBool(_meleePossible);
	savefile->ReadBool(_rangedPossible);

	int temp;
	savefile->ReadInt(temp);
	_combatType = static_cast<ECombatType>(temp);

	_enemy.Restore(savefile);
	savefile->ReadInt(_endTime);
	savefile->ReadBool(_endgame); // grayman #3848
	savefile->ReadVec3(_destination); // grayman #3848

	// grayman #3063
	savefile->ReadInt(temp);
	_combatSubState = static_cast<ECombatSubState>(temp);

	savefile->ReadInt(_reactionEndTime); // grayman #3063
	savefile->ReadInt(_waitEndTime);	 // grayman #3331
	savefile->ReadBool(_needInitialDrawDelay); // grayman #3331
	savefile->ReadBool(_unarmedMelee);	 // grayman #3331
	savefile->ReadBool(_unarmedRanged);	 // grayman #3331
	savefile->ReadBool(_armedMelee);	 // grayman #3331
	savefile->ReadBool(_armedRanged);	 // grayman #3331
	savefile->ReadBool(_justDrewWeapon); // grayman #3331
}

StatePtr CombatState::CreateInstance()
{
	return StatePtr(new CombatState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar combatStateRegistrar(
	STATE_COMBAT, // Task Name
	StateLibrary::CreateInstanceFunc(&CombatState::CreateInstance) // Instance creation callback
);

} // namespace ai
