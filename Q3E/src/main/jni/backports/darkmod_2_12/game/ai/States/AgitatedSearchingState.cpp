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



#include "AgitatedSearchingState.h"
#include "../Memory.h"
#include "../Tasks/InvestigateSpotTask.h"
#include "../Tasks/SingleBarkTask.h"
#include "../Tasks/RepeatedBarkTask.h"
#include "../Tasks/WaitTask.h"
#include "../Tasks/IdleAnimationTask.h" // grayman #3857
#include "CombatState.h"
#include "../Library.h"
#include "../../AbsenceMarker.h"
#include "../../AIComm_Message.h"

namespace ai
{

// Get the name of this state
const idStr& AgitatedSearchingState::GetName() const
{
	static idStr _name(STATE_AGITATED_SEARCHING);
	return _name;
}

bool AgitatedSearchingState::CheckAlertLevel(idAI* owner)
{
	// grayman #3857 - AgitatedSearchingState::Think() doesn't call this
	// method directly. It calls SearchingState::Think(), and that calls
	// this method. Use memory.leaveAlertState to tell ASS::T() whether it
	// should quit when SS::T() returns to it.

	if (!owner->m_canSearch) // grayman #3069 - AI that can't search shouldn't be here
	{
		owner->SetAlertLevel(owner->thresh_3 - 0.1);
	}

	if (owner->AI_AlertIndex < EAgitatedSearching)
	{
		// Alert index is too low for this state, fall back
		owner->GetMind()->EndState();
		owner->GetMemory().leaveAlertState = true; // grayman #3857
		return false;
	}

	// grayman #3009 - can't enter this state if sitting, sleeping,
	// sitting down, lying down, or getting up from sitting or sleeping

	moveType_t moveType = owner->GetMoveType();
	if ( moveType == MOVETYPE_SIT      || 
		 moveType == MOVETYPE_SLEEP    ||
		 moveType == MOVETYPE_SIT_DOWN ||
		 moveType == MOVETYPE_FALL_ASLEEP ) // grayman #3820 - was MOVETYPE_LAY_DOWN
	{
		owner->GetUp(); // it's okay to call this multiple times
		owner->GetMind()->EndState();
		owner->GetMemory().leaveAlertState = true; // grayman #3857
		return false;
	}

	if ( ( moveType == MOVETYPE_GET_UP ) ||	( moveType == MOVETYPE_WAKE_UP ) ) // grayman #3820 - MOVETYPE_WAKE_UP was MOVETYPE_GET_UP_FROM_LYING
	{
		owner->GetMind()->EndState();
		owner->GetMemory().leaveAlertState = true; // grayman #3857
		return false;
	}

	if (owner->AI_AlertIndex > EAgitatedSearching)
	{
		// Alert index is too high, switch to the higher State
		if (owner->m_searchID > 0)
		{
			gameLocal.m_searchManager->LeaveSearch(owner->m_searchID, owner); // grayman #3857 - leave an ongoing search
		}

		//owner->Event_CloseHidingSpotSearch();
		owner->GetMemory().combatState = -1; // grayman #3507
		owner->GetMind()->PushState(owner->backboneStates[ECombat]);
		owner->GetMemory().leaveAlertState = true; // grayman #3857
		return false;
	}

	owner->GetMemory().leaveAlertState = false; // grayman #3857
	// Alert Index is matching, return OK
	return true;
}

void AgitatedSearchingState::CalculateAlertDecreaseRate(idAI* owner)
{
	float alertTime = owner->atime4 + owner->atime4_fuzzyness * (gameLocal.random.RandomFloat() - 0.5);
	_alertLevelDecreaseRate = (owner->thresh_5 - owner->thresh_4) / alertTime;
}

// grayman #3507

void AgitatedSearchingState::DrawWeapon(idAI* owner)
{
	// grayman #3331 - draw your ranged weapon if you have one, otherwise draw your melee weapon.
	// Note that either weapon could be drawn, but if we default to melee, AI with ranged and
	// melee weapons will draw their melee weapon, and we'll never see ranged weapons get drawn.
	// Odds are that the enemy is nowhere nearby anyway, since we're just searching.

	// grayman #3549 - force a melee draw if the alert is near and we have a melee weapon

	idVec3 alertSpot = owner->GetMemory().alertPos;
	idVec3 ownerSpot = owner->GetPhysics()->GetOrigin();
	alertSpot.z = ownerSpot.z; // ignore vertical delta
	float dist2Alert = (alertSpot - ownerSpot).LengthFast();
	bool inMeleeRange = ( dist2Alert <= 3*owner->GetMeleeRange());
	bool hasMeleeWeapon2Draw = ( owner->GetNumMeleeWeapons() > 0 ) && !owner->spawnArgs.GetBool("unarmed_melee","0");
	bool hasRangedWeapon2Draw = ( owner->GetNumRangedWeapons() > 0 ) && !owner->spawnArgs.GetBool("unarmed_ranged","0");
	bool drawMeleeWeapon = false;
	bool drawRangedWeapon = false;

	_drawEndTime = gameLocal.time;
	if (inMeleeRange)
	{
		if (hasMeleeWeapon2Draw)
		{
			drawMeleeWeapon = true;
		}
		else if (hasRangedWeapon2Draw)
		{
			drawRangedWeapon = true;
		}
	}
	else // not inside melee range
	{
		if ( hasRangedWeapon2Draw )
		{
			drawRangedWeapon = true;
		}
		else if ( hasMeleeWeapon2Draw )
		{
			drawMeleeWeapon = true;
		}
	}

	if (drawMeleeWeapon)
	{
		owner->DrawWeapon(COMBAT_MELEE);
		_drawEndTime += MAX_DRAW_DURATION;// grayman #3563 - safety net when drawing a weapon
	}
	else if (drawRangedWeapon)
	{
		owner->DrawWeapon(COMBAT_RANGED);
		_drawEndTime += MAX_DRAW_DURATION; // grayman #3563 - safety net when drawing a weapon
	}
}

// grayman #3857 - different search roles and whether the AI has
// seen evidence or not are used to determine the correct agitated
// search bark. Since searchers can join and leave a search dynamically
// and evidence can be seen for the first time during a search, we need
// to determine whether the bark needs to be changed each time
// Agitated Searching thinks.

void AgitatedSearchingState::SetRepeatedBark(idAI* owner)
{
	Memory& memory = owner->GetMemory();
	ERepeatedBarkState newRepeatedBarkState = ERBS_NULL;

	Search *search = gameLocal.m_searchManager->GetSearch(owner->m_searchID);
	if (search) // should always be non-NULL
	{
		Assignment *assignment = gameLocal.m_searchManager->GetAssignment(search,owner);
		int searcherCount = search->_searcherCount;
		idStr soundName;

		if (owner->HasSeenEvidence())
		{
			if (assignment && (assignment->_searcherRole == E_ROLE_SEARCHER))
			{
				if (searcherCount > 1)
				{
					newRepeatedBarkState = ERBS_SEARCHER_MULTIPLE_EVIDENCE;
				}
				else
				{
					newRepeatedBarkState = ERBS_SEARCHER_SINGLE_EVIDENCE;
				}
			}
			else
			{
				newRepeatedBarkState = ERBS_GUARD_OBSERVER;
			}
		}
		else // has not seen evidence
		{
			if (assignment && (assignment->_searcherRole == E_ROLE_SEARCHER))
			{
				if (searcherCount > 1)
				{
					newRepeatedBarkState = ERBS_SEARCHER_MULTIPLE_NO_EVIDENCE;
				}
				else
				{
					newRepeatedBarkState = ERBS_SEARCHER_SINGLE_NO_EVIDENCE;
				}
			}
			else
			{
				newRepeatedBarkState = ERBS_GUARD_OBSERVER;
			}
		}
	}

	if (memory.repeatedBarkState != newRepeatedBarkState)
	{
		memory.repeatedBarkState = newRepeatedBarkState;

		bool sendSuspiciousMessage = true; // whether to send a 'something is suspicious' message or not

		owner->commSubsystem->ClearTasks(); // kill any previous repeated bark task; we'll be starting a new one

		idStr soundName;
		switch (newRepeatedBarkState)
		{
		case ERBS_SEARCHER_SINGLE_NO_EVIDENCE:
			soundName = "snd_state4SeenNoEvidence"; // what to say when you're by yourself and you've seen no evidence
			break;
		case ERBS_SEARCHER_MULTIPLE_NO_EVIDENCE:
			soundName = "snd_state4SeenNoEvidence_c"; // what to say when friends are helping you search and you've seen no evidence
			break;
		case ERBS_SEARCHER_SINGLE_EVIDENCE:
			soundName = "snd_state4SeenEvidence"; // what to say when you're by yourself and you've seen evidence
			break;
		case ERBS_SEARCHER_MULTIPLE_EVIDENCE:
			soundName = "snd_state4SeenEvidence_c"; // what to say when friends are helping you search and you've seen evidence
			break;
		default:
		case ERBS_NULL:
		case ERBS_GUARD_OBSERVER:
			soundName = "snd_state3"; // guards and observers say this, regardless of whether they've seen evidence or not

			// grayman #3857 - "snd_state3" repeated barks are not intended to
			// alert nearby friends. Just send along a blank message.
			sendSuspiciousMessage = false;
			break;
		}

		CommMessagePtr message;

		if (sendSuspiciousMessage)
		{
			message = CommMessagePtr(new CommMessage(
			CommMessage::DetectedSomethingSuspicious_CommType, 
			owner, NULL, // from this AI to anyone
			NULL,
			memory.alertPos,
			memory.currentSearchEventID // grayman #3438
		));
		}

		int minTime = SEC2MS(owner->spawnArgs.GetFloat("searchbark_delay_min", "10"));
		int maxTime = SEC2MS(owner->spawnArgs.GetFloat("searchbark_delay_max", "15"));

		owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new RepeatedBarkTask(soundName, minTime, maxTime, message)));
	}
}

void AgitatedSearchingState::Init(idAI* owner)
{
	// Init base class first (note: we're not calling SearchingState::Init() on purpose here)
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("AgitatedSearchingState initialised.\r");
	assert(owner);

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	memory.leaveAlertState = false;

	// Ensure we are in the correct alert level
	if ( !CheckAlertLevel(owner) )
	{
		return;
	}

	// grayman #3496 - note that we spent time in Agitated Search

	memory.agitatedSearched = true;

	CalculateAlertDecreaseRate(owner);
	
	if (owner->GetMoveType() == MOVETYPE_SIT || owner->GetMoveType() == MOVETYPE_SLEEP)
	{
		owner->GetUp();
	}

	// Set up a new hiding spot search if not already assigned to one
	if (owner->m_searchID <= 0)
	{
		if (!StartNewHidingSpotSearch(owner)) // grayman #3857 - AI gets his assignment
		{
			// grayman - this section can't run because it causes
			// the stealth score to rise dramatically during player sightings
			//owner->SetAlertLevel(owner->thresh_3 - 0.1); // failed to create a search, so drop down to Suspicious mode
			//owner->GetMind()->EndState();
			//return;
		}
	}

	// kill the repeated and single bark tasks
	owner->commSubsystem->ClearTasks(); // grayman #3182
	memory.repeatedBarkState = ERBS_NULL; // grayman #3857

	if (owner->AlertIndexIncreased())
	{
		// grayman #3496 - enough time passed since last alert bark?
		// grayman #3857 - enough time passed since last visual stim bark?
		if ( ( gameLocal.time >= memory.lastTimeAlertBark + MIN_TIME_BETWEEN_ALERT_BARKS ) &&
			 ( gameLocal.time >= memory.lastTimeVisualStimBark + MIN_TIME_BETWEEN_ALERT_BARKS ) )
		{
			idStr soundName = "";
			if ( ( memory.alertedDueToCommunication == false ) && ( ( memory.alertType == EAlertTypeSuspicious ) || ( memory.alertType == EAlertTypeEnemy ) || ( memory.alertType == EAlertTypeFailedKO ) ) )
			{
				if (owner->HasSeenEvidence())
				{
					soundName = "snd_alert4";
				}
				else
				{
					soundName = "snd_alert4NoEvidence";
				}

				CommMessagePtr message = CommMessagePtr(new CommMessage(
					CommMessage::DetectedSomethingSuspicious_CommType, 
					owner, NULL, // from this AI to anyone
					NULL,
					memory.alertPos,
					memory.currentSearchEventID // grayman #3438
				));

				owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask(soundName,message)));

				memory.lastTimeAlertBark = gameLocal.time; // grayman #3496

				if (cv_ai_debug_transition_barks.GetBool())
				{
					gameLocal.Printf("%d: %s rises to Agitated Searching state, barks '%s'\n",gameLocal.time,owner->GetName(),soundName.c_str());
				}
			}
			else if ( memory.respondingToSomethingSuspiciousMsg ) // grayman #3857
			{
				soundName = "snd_helpSearch";

				// Allocate a SingleBarkTask, set the sound and enqueue it
				owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask(soundName)));

				memory.lastTimeAlertBark = gameLocal.time; // grayman #3496

				if (cv_ai_debug_transition_barks.GetBool())
				{
					gameLocal.Printf("%d: %s rises to Searching state, barks '%s'\n",gameLocal.time,owner->GetName(),soundName.c_str());
				}
			}
		}
		else
		{
			if (cv_ai_debug_transition_barks.GetBool())
			{
				gameLocal.Printf("%d: %s rises to Agitated Searching state, can't bark 'snd_alert4{NoEvidence}' yet\n",gameLocal.time,owner->GetName());
			}
		}
	}

	owner->commSubsystem->AddSilence(5000 + gameLocal.random.RandomInt(3000)); // grayman #3424

	SetRepeatedBark(owner); // grayman #3857
	
	DrawWeapon(owner); // grayman #3507

	// Let the AI update their weapons (make them solid)
	owner->UpdateAttachmentContents(true);

	// grayman #3857 - no idle search anims in this state
	owner->actionSubsystem->ClearTasks();
}

// Gets called each time the mind is thinking
void AgitatedSearchingState::Think(idAI* owner)
{
	SearchingState::Think(owner);

	// grayman #3857 - AgitatedSearchingState::CheckAlertLevel() is called
	// from SearchingState::Think(), and if that determines we're in the wrong
	// state, we don't want to set repeated barks or check
	// for a drawn weapon. AgitatedSearchingState::CheckAlertLevel() sets
	// leaveAlertState to true if this is the case.
	if (owner->GetMemory().leaveAlertState) // grayman #3857
	{
		return;
	}

	SetRepeatedBark(owner); // grayman #3857 - in case the bark has to change

	// grayman #3563 - check safety net for drawing a weapon
	if ( gameLocal.time >= _drawEndTime )
	{
		// if the weapon isn't drawn at this point, redraw it

		if ( !owner->GetAttackFlag(COMBAT_MELEE) && !owner->GetAttackFlag(COMBAT_RANGED) )
		{
			_drawEndTime = gameLocal.time;
			DrawWeapon(owner); // grayman #3507
		}
	}
}

StatePtr AgitatedSearchingState::CreateInstance()
{
	return StatePtr(static_cast<State*>(new AgitatedSearchingState));
}

// Register this state with the StateLibrary
StateLibrary::Registrar agitatedSearchingStateRegistrar(
	STATE_AGITATED_SEARCHING, // Task Name
	StateLibrary::CreateInstanceFunc(&AgitatedSearchingState::CreateInstance) // Instance creation callback
);

} // namespace ai
