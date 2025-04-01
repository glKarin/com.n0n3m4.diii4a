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



#include "SearchingState.h"
#include "../Memory.h"
#include "../Tasks/InvestigateSpotTask.h"
#include "../Tasks/GuardSpotTask.h" // grayman #3857
#include "../Tasks/SingleBarkTask.h"
#include "../Tasks/RepeatedBarkTask.h" // grayman #3472
#include "../Tasks/IdleAnimationTask.h" // grayman #3857
#include "../Library.h"
#include "IdleState.h"
#include "AgitatedSearchingState.h"
#include "../../AbsenceMarker.h"
#include "../../AIComm_Message.h"
#include "FleeState.h" // grayman #3317
#include "../game/SearchManager.h" // grayman #3857

#define MILL_RADIUS 150.0f // grayman #3857
#define MAX_RADIAL_SPOT_ATTEMPTS 20 // grayman #3857

namespace ai
{

// Get the name of this state
const idStr& SearchingState::GetName() const
{
	static idStr _name(STATE_SEARCHING);
	return _name;
}

bool SearchingState::CheckAlertLevel(idAI* owner)
{
	if (!owner->m_canSearch) // grayman #3069 - AI that can't search shouldn't be here
	{
		owner->SetAlertLevel(owner->thresh_3 - 0.1);
	}

	if (owner->AI_AlertIndex < ESearching)
	{
		owner->GetMemory().agitatedSearched = false; // grayman #3496 - clear this if descending
		
		// Alert index is too low for this state, fall back
		if (owner->m_searchID > 0) // grayman #3857
		{
			gameLocal.m_searchManager->LeaveSearch(owner->m_searchID, owner);
		}

		//owner->Event_CloseHidingSpotSearch();
		owner->GetMind()->EndState();
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
		return false;
	}

	if ( ( moveType == MOVETYPE_GET_UP ) ||	( moveType == MOVETYPE_WAKE_UP ) ) // grayman #3820 - MOVETYPE_WAKE_UP was MOVETYPE_GET_UP_FROM_LYING
	{
		owner->GetMind()->EndState();
		return false;
	}

	if (owner->AI_AlertIndex > ESearching)
	{
		// Alert index is too high, switch to the higher State
		owner->GetMind()->PushState(owner->backboneStates[EAgitatedSearching]);
		return false;
	}

	// Alert Index is matching, return OK
	return true;
}

void SearchingState::Init(idAI* owner)
{
	// Init base class first
	State::Init(owner);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("SearchingState initialised.\r");
	assert(owner);

	// Ensure we are in the correct alert level
	if ( !CheckAlertLevel(owner) )
	{
		return;
	}

	if (owner->GetMoveType() == MOVETYPE_SIT || owner->GetMoveType() == MOVETYPE_SLEEP)
	{
		owner->GetUp();
	}

	// Shortcut reference
	Memory& memory = owner->GetMemory();

	float alertTime = owner->atime3 + owner->atime3_fuzzyness * (gameLocal.random.RandomFloat() - 0.5);

	_alertLevelDecreaseRate = (owner->thresh_4 - owner->thresh_3) / alertTime;

	if ( owner->AlertIndexIncreased() || memory.mandatory ) // grayman #3331
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

	if ( owner->AlertIndexIncreased() )
	{
		// grayman #4220 - clear the most recent searched spot
		owner->lastSearchedSpot = idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY);

		// grayman #3423 - when the alert level is ascending, kill the repeated bark task
		owner->commSubsystem->ClearTasks();

		// Play bark if alert level is ascending

		// grayman #3496 - Enough time passed since last alert bark?
		// grayman #3857 - Enough time passed since last visual stim bark?
		if ( ( gameLocal.time >= memory.lastTimeAlertBark + MIN_TIME_BETWEEN_ALERT_BARKS ) &&
			 ( gameLocal.time >= memory.lastTimeVisualStimBark + MIN_TIME_BETWEEN_ALERT_BARKS ) )
		{
			idStr soundName;

			if ((memory.alertedDueToCommunication == false) && ((memory.alertType == EAlertTypeSuspicious) || ( memory.alertType == EAlertTypeEnemy ) || ( memory.alertType == EAlertTypeFailedKO ) ) )
			{
				bool friendsNear = ( (MS2SEC(gameLocal.time - memory.lastTimeFriendlyAISeen)) <= MAX_FRIEND_SIGHTING_SECONDS_FOR_ACCOMPANIED_ALERT_BARK );
				if ( (memory.alertClass == EAlertVisual_1) ||
					 (memory.alertClass == EAlertVisual_2) ||      // grayman #2603, #3424 
					 // (memory.alertClass == EAlertVisual_3) ) || // grayman #3472 - no longer needed
					 (memory.alertClass == EAlertVisual_4) )       // grayman #3498
				{
					if ( friendsNear )
					{
						soundName = "snd_alert3sc";
					}
					else
					{
						soundName = "snd_alert3s";
					}
				}
				else if (memory.alertClass == EAlertAudio)
				{
					if ( friendsNear )
					{
						soundName = "snd_alert3hc";
					}
					else
					{
						soundName = "snd_alert3h";
					}
				}
				else if ( friendsNear )
				{
					soundName = "snd_alert3c";
				}
				else
				{
					soundName = "snd_alert3";
				}

				// Allocate a SingleBarkTask, set the sound and enqueue it

				owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new SingleBarkTask(soundName)));

				memory.lastTimeAlertBark = gameLocal.time; // grayman #3496

				if (cv_ai_debug_transition_barks.GetBool())
				{
					gameLocal.Printf("%d: %s rises to Searching state, barks '%s'\n",gameLocal.time,owner->GetName(),soundName.c_str());
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
				gameLocal.Printf("%d: %s rises to Searching state, can't bark 'snd_alert3{s/sc/h/hc/c}' yet\n",gameLocal.time,owner->GetName());
			}
		}
	}
	else if ((memory.alertType == EAlertTypeEnemy) || (memory.alertType == EAlertTypeFailedKO))
	{
		// descending alert level
		// reduce the alert type, so we can react to other alert types (such as a dead person)
		memory.alertType = EAlertTypeSuspicious;
	}
	
	// grayman #3472 - When ascending, set up a repeated bark

	if ( owner->AlertIndexIncreased() )
	{
		owner->commSubsystem->AddSilence(5000 + gameLocal.random.RandomInt(3000)); // grayman #3424

		// grayman #3857 - "snd_state3" repeated barks are not intended to
		// alert nearby friends. Just send along a blank message.
		CommMessagePtr message;

		/*
		// This will hold the message to be delivered with the bark
		CommMessagePtr message(new CommMessage(
			CommMessage::DetectedEnemy_CommType, 
			owner, NULL,// from this AI to anyone 
			NULL,
			idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY),
			-1
		));
		*/

		int minTime = SEC2MS(owner->spawnArgs.GetFloat("searchbark_delay_min", "10"));
		int maxTime = SEC2MS(owner->spawnArgs.GetFloat("searchbark_delay_max", "15"));
		owner->commSubsystem->AddCommTask(CommunicationTaskPtr(new RepeatedBarkTask("snd_state3", minTime, maxTime, message)));
	}
	else // descending
	{
		// Allow repeated barks from Agitated Searching to continue.
	}

	if (!owner->HasSeenEvidence())
	{
		owner->SheathWeapon();
		owner->UpdateAttachmentContents(false);
	}
	else
	{
		// Let the AI update their weapons (make them solid)
		owner->UpdateAttachmentContents(true);
	}

	// grayman #3857 - allow "idle search/suspicious animations"
	owner->actionSubsystem->ClearTasks();
	owner->actionSubsystem->PushTask(IdleAnimationTask::CreateInstance());

	memory.consecutiveRadialSpotFailures = 0; // grayman #3857
}

// Gets called each time the mind is thinking
void SearchingState::Think(idAI* owner)
{
	UpdateAlertLevel();

	// Ensure we are in the correct alert level
	if (!CheckAlertLevel(owner))
	{
		return;
	}

	// Let the AI check its senses
	owner->PerformVisualScan();

	if (owner->GetMoveType() == MOVETYPE_SIT 
		|| owner->GetMoveType() == MOVETYPE_SLEEP
		|| owner->GetMoveType() == MOVETYPE_SIT_DOWN
		|| owner->GetMoveType() == MOVETYPE_FALL_ASLEEP) // grayman #3820 - was MOVETYPE_LAY_DOWN
	{
		owner->GetUp();
		return;
	}

	Memory& memory = owner->GetMemory();

	// grayman #3200 - if asked to restart the hiding spot search,
	// and you can successfully join a new search, don't continue
	// with the current hiding spot search
	if (memory.restartSearchForHidingSpots)
	{
		memory.restartSearchForHidingSpots = false;

		// We should restart the search (probably due to a new incoming stimulus)

		// Set up a new hiding spot search
		if (StartNewHidingSpotSearch(owner)) // grayman #3857 - AI will leave an existing search and get a new assignment for the new search
		{
			return;
		}
	}

	// grayman #3520 - look at alert spots
	if ( owner->m_lookAtAlertSpot )
	{
		owner->m_lookAtAlertSpot = false; // only set up one look
		idVec3 alertSpot = owner->m_lookAtPos;
		if ( alertSpot.x != idMath::INFINITY ) // grayman #3438
		{
			if ( !owner->CheckFOV(alertSpot) )
			{
				// Search spot is not within FOV, turn towards the position
				owner->TurnToward(alertSpot);
				owner->Event_LookAtPosition(alertSpot, 2.0f);
			}
			else
			{
				owner->Event_LookAtPosition(alertSpot, 2.0f);
			}
		}
		owner->m_lookAtPos = idVec3(idMath::INFINITY,idMath::INFINITY,idMath::INFINITY);
	}

	// grayman #3857 - What is my role? If I'm a guard, I should go guard a spot.
	// If a searcher, continuously ask for a new hiding spot to investigate.
	// If an observer, I should go stand at the perimeter of the search.

	Search *search = gameLocal.m_searchManager->GetSearch(owner->m_searchID);
	Assignment* assignment = gameLocal.m_searchManager->GetAssignment(search,owner);

	if (search && assignment)
	{
		/* uncomment to more clearly see who's actively searching and who's milling/searching/guarding/observing
		idVec4 color;
		if (memory.millingInProgress)
		{
			color = colorYellow;
		}
		else if (assignment->_searcherRole == E_ROLE_SEARCHER)
		{
			color = colorGreen;
		}
		else if (assignment->_searcherRole == E_ROLE_GUARD)
		{
			color = colorRed;
		}
		else // observer
		{
			color = colorBlue;
		}
		if ( memory.currentSearchSpot.x != idMath::INFINITY )
		{
			gameRenderWorld->DebugArrow(color, owner->GetEyePosition(), memory.currentSearchSpot, 1, 100);
		}
		gameRenderWorld->DebugArrow(colorPink, owner->GetEyePosition(), search->_origin, 1, 100);
		*/

		// Prepare the hiding spots if they're going to be needed.

		if (search->_assignmentFlags & SEARCH_SEARCH)
		{
			// Do we have an ongoing hiding spot search?
			if (!memory.hidingSpotSearchDone)
			{
				// Let the hiding spot search do its task
				gameLocal.m_searchManager->PerformHidingSpotSearch(owner->m_searchID,owner); // grayman #3857
			}
		}

		// If the search calls for it, send the AI to mill about the alert spot.

		if (memory.millingInProgress)
		{
			return;
		}

		smRole_t role = assignment->_searcherRole;

		if (memory.shouldMill)
		{
			// Before beginning milling, do we need to finish a preliminary spot investigation?
			// This is typically the spot that brings an AI into the search area.

			if (memory.hidingSpotInvestigationInProgress)
			{
				return;
			}

			idVec3 spot;
			if (FindRadialSpot(owner, search->_origin, MILL_RADIUS, spot)) // grayman #3857
			{
				// the spot is good
				memory.consecutiveRadialSpotFailures = 0; // reset
				memory.millingInProgress = true;
				memory.guardingInProgress = false;
				memory.shouldMill = false;
				memory.currentSearchSpot = spot; // spot to guard
				memory.guardingAngle = idMath::INFINITY; // face search origin when spot is reached
				owner->searchSubsystem->SwitchTask(TaskPtr(GuardSpotTask::CreateInstance())); // grayman #3857 - switch from action to search

				return;
			}

			// grayman #3857 - couldn't find a spot to mill at; only allow
			// a certain number of consecutive failures before skipping
			// milling entirely

			if (++memory.consecutiveRadialSpotFailures >= MAX_RADIAL_SPOT_ATTEMPTS)
			{
				memory.consecutiveRadialSpotFailures = 0; // reset
				memory.shouldMill = false;
			}
			else
			{
				return; // grayman #3857 - try again next time
			}
		}

		// Any required milling is finished. Does the search need active searchers?

		if ((search->_assignmentFlags & SEARCH_SEARCH) && (role == E_ROLE_SEARCHER))
		{
			// Do we have an ongoing hiding spot search?
			if (!memory.hidingSpotSearchDone)
			{
				// hiding spot search not done yet, but don't call PerformHidingSpotSearch() here
				// Let the hiding spot search do its task
				//gameLocal.m_searchManager->PerformHidingSpotSearch(owner->m_searchID,owner); // grayman #3857
				return;
			}

			// Is InvestigateSpotTask() running? 
			if (memory.hidingSpotInvestigationInProgress)
			{
				return;
			}

			// grayman #4220 - a search type of 1 or 2 uses hiding spots, and
			// a search type of 3 or greater doesn't

			if ( cv_ai_search_type.GetInteger() < 3 )
			{
				// Have we run out of hiding spots?

				if ( memory.noMoreHidingSpots )
				{
					if ( gameLocal.time >= memory.nextTime2GenRandomSpot )
					{
						memory.nextTime2GenRandomSpot = gameLocal.time + DELAY_RANDOM_SPOT_GEN*(1 + (gameLocal.random.RandomFloat() - 0.5) / 3);

						// grayman #2422
						// Generate a random search point, but make sure it's inside an AAS area
						// and that it's also inside the search volume.

						idVec3 p;		// random point
						int areaNum;	// p's area number
						idVec3 searchSize = assignment->_limits.GetSize();
						idVec3 searchCenter = assignment->_limits.GetCenter();

						//gameRenderWorld->DebugBox(colorWhite, idBox(assignment->_limits), MS2SEC(memory.nextTime2GenRandomSpot - gameLocal.time));

						bool validPoint = false;
						idVec3 ownerOrigin = owner->GetPhysics()->GetOrigin();
						int ownerAreaNum = owner->PointReachableAreaNum(ownerOrigin, 1.0f);
						aasPath_t path;
						for ( int i = 0; i < 6; i++ )
						{
							p = searchCenter;
							p.x += gameLocal.random.RandomFloat()*(searchSize.x) - searchSize.x / 2;
							p.y += gameLocal.random.RandomFloat()*(searchSize.y) - searchSize.y / 2;
							p.z += gameLocal.random.RandomFloat()*(searchSize.z) - searchSize.z / 2;
							//p.z += gameLocal.random.RandomFloat()*(searchSize.z/2) - searchSize.z/4;
							areaNum = owner->PointReachableAreaNum(p);
							if ( areaNum == 0 )
							{
								//gameRenderWorld->DebugArrow(colorRed, owner->GetEyePosition(), p, 1, MS2SEC(memory.nextTime2GenRandomSpot - gameLocal.time));
								continue;
							}
							owner->GetAAS()->PushPointIntoAreaNum(areaNum, p); // if this point is outside this area, it will be moved to one of the area's edges
							if ( !assignment->_limits.ContainsPoint(p) )
							{
								//gameRenderWorld->DebugArrow(colorPink, owner->GetEyePosition(), p, 1, MS2SEC(memory.nextTime2GenRandomSpot - gameLocal.time));
								continue;
							}

							if ( !owner->PathToGoal(path, ownerAreaNum, ownerOrigin, areaNum, p, owner) )
							{
								continue;
							}

							//gameRenderWorld->DebugArrow(colorGreen, owner->GetEyePosition(), p, 1, MS2SEC(memory.nextTime2GenRandomSpot - gameLocal.time));
							validPoint = true;
							break;
						}

						if ( validPoint )
						{
							// grayman #2422 - the point chosen 
							memory.currentSearchSpot = p;

							// Choose to investigate spots closely on a random basis
							// grayman #2801 - and only if you weren't hit by a projectile

							memory.investigateStimulusLocationClosely = ((gameLocal.random.RandomFloat() < 0.3f) && (memory.alertType != EAlertTypeHitByProjectile));

							owner->searchSubsystem->SwitchTask(TaskPtr(InvestigateSpotTask::CreateInstance())); // grayman #3857 - switch from action to search

							// Set the flag to TRUE, so that the sensory scan can be performed
							memory.hidingSpotInvestigationInProgress = true;
						}
						else // no valid random point found
						{
							// Stop moving, the algorithm will choose another spot the next round
							owner->StopMove(MOVE_STATUS_DONE);
							memory.StopReacting(); // grayman #3559

							// grayman #2422 - at least turn toward and look at the last invalid point some of the time
							p.z += 60; // look up a bit, to simulate searching for the player's head
							if ( !owner->CheckFOV(p) )
							{
								owner->TurnToward(p);
							}
							owner->Event_LookAtPosition(p, MS2SEC(memory.nextTime2GenRandomSpot - gameLocal.time));
							//gameRenderWorld->DebugArrow(colorPink, owner->GetEyePosition(), p, 1, MS2SEC(memory.nextTime2GenRandomSpot - gameLocal.time));
						}
					}
				}
				// We should have more hiding spots, try to get the next one
				else if ( !gameLocal.m_searchManager->GetNextHidingSpot(search, owner, memory.currentSearchSpot) ) // grayman #3857
				{
					// No more hiding spots to search
					DM_LOG(LC_AI, LT_INFO)LOGSTRING("No more hiding spots!\r");

					// Stop moving, the algorithm will choose another spot the next round
					owner->StopMove(MOVE_STATUS_DONE);
					memory.StopReacting(); // grayman #3559
				}
				else
				{
					// GetNextHidingSpot() returned TRUE, so we have memory.currentSearchSpot set

					// Delegate the spot investigation to a new task, which will send the searcher to investigate the new spot.
					owner->searchSubsystem->SwitchTask(InvestigateSpotTask::CreateInstance()); // grayman #3857 - switch from action to search

					// Prevent falling into the same hole twice
					memory.hidingSpotInvestigationInProgress = true;
				}
			}
			else if ( cv_ai_search_type.GetInteger() >= 3 )
			{
				if ( !gameLocal.m_searchManager->GetNextSearchSpot(search, owner, memory.currentSearchSpot) ) // grayman #4220
				{
					// Stop moving, the algorithm will choose another spot the next round
					owner->StopMove(MOVE_STATUS_DONE);
					memory.StopReacting(); // grayman #3559
				}
				else
				{
					// GetNextSearchSpot() returned TRUE, so we have memory.currentSearchSpot set

					// Delegate the spot investigation to a new task, which will send the searcher to investigate the new spot.
					owner->searchSubsystem->SwitchTask(InvestigateSpotTask::CreateInstance()); // grayman #3857 - switch from action to search

					// Prevent falling into the same hole twice
					memory.hidingSpotInvestigationInProgress = true;
				}

				/* grayman - uncomment to see search areas
				idVec4 color = colorWhite;

				if ( (assignment->_sector == 1) || (assignment->_sector == 4) )
				{
					color = colorGreen;
				}
				else if ( (assignment->_sector == 2) || (assignment->_sector == 3) )
				{
					color = colorBlue;
				}
				gameRenderWorld->DebugBox(color, idBox(assignment->_limits), 2000);
				gameRenderWorld->DebugText(va("%s", owner->GetName()), assignment->_limits.GetCenter(), 0.25f, color,
					gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 2000);
				*/
			}

			return;
		}

		// Does this search require guards?

		if ((search->_assignmentFlags & SEARCH_GUARD) && (role == E_ROLE_GUARD))
		{
			// Is a guard spot task in progress by this AI (GuardSpotTask())?
			if (memory.guardingInProgress)
			{
				// Do nothing here. Wait for the GuardSpot task to complete
				return;
			}

			// Pick a spot to guard

			if (search->_guardSpotsReady)
			{
				// Pick a spot.

				// For guard spots, don't worry about LOS from the spot to the
				// search origin. This collection of spots is a set of points that the
				// mapper has chosen (by using guard entities), or points that
				// are at the area's portals.

				idVec3 ownerOrigin = owner->GetPhysics()->GetOrigin();
				int ownerAreaNum = owner->PointReachableAreaNum(ownerOrigin, 1.0f);
				aasPath_t path;
				for (int i = 0; i < search->_guardSpots.Num(); i++)
				{
					idVec3 spot = search->_guardSpots[i].ToVec3();
					if (spot.x == idMath::INFINITY) // spot already taken?
					{
						continue;
					}

					// grayman #3857 - can we walk to the spot?

					int toAreaNum = owner->PointReachableAreaNum(spot);
					if (toAreaNum == 0)
					{
						continue;
					}

					if (!owner->PathToGoal(path, ownerAreaNum, ownerOrigin, toAreaNum, spot, owner))
					{
						continue;
					}

					memory.currentSearchSpot = spot; // spot to guard
					memory.guardingAngle = search->_guardSpots[i].w; // angle to face when guard spot is reached
					memory.guardingInProgress = true;
					memory.millingInProgress = false;
					owner->searchSubsystem->SwitchTask(TaskPtr(GuardSpotTask::CreateInstance())); // grayman #3857 - switch from action to search

					search->_guardSpots[i].x = idMath::INFINITY; // mark the spot as taken
					break;
				}

				// If you get here w/o having been assigned a spot, there aren't enough
				// spots to go around. Become an observer, and get your spot the next
				// time through here.
				if (!memory.guardingInProgress)
				{
					assignment->_searcherRole = E_ROLE_OBSERVER;
				}
			}
			else // need to build a list of spots
			{
				// Should be able to do this in one frame, since we're looking
				// for exits from the AAS Cluster, and special mapper-defined
				// locations, and there shouldn't be too many of them.

				gameLocal.m_searchManager->CreateListOfGuardSpots(search,owner);
			}

			return;
		}

		// Does this search require observers?

		if ((search->_assignmentFlags & SEARCH_OBSERVE) && (role == E_ROLE_OBSERVER))
		{
			// As a civilian, you can search, but if the search assignments are
			// used up, you can't be a guard, because they should be armed. What
			// you can do is stand around at the perimeter of the search and watch for a while.
			// Of course, if the alert event was bad enough to send you fleeing, you won't
			// be around anyway.

			// We'll treat this as if you're guarding a spot, but the spots will be chosen
			// randomly around the perimeter of the search.

			// Is a guard spot task in progress by this AI (GuardSpotTask())?
			if (memory.guardingInProgress)
			{
				// Do nothing here. Wait for the GuardSpot task to complete
				return;
			}

			// Pick a spot.

			// What is the radius of the perimeter? Though we're not actively searching,
			// the radius is included in our assignment. Pick a spot that's between you and
			// the search origin, so you don't have to run through the search area to get
			// to your spot.

			Assignment* assignment = gameLocal.m_searchManager->GetAssignment(search,owner);
			float radius = assignment->_outerRadius;
			radius += 32.0f; // given the accuracy of the Guard Spot Task (64.0f), tighten up the perimeter slightly
			idVec3 spot;
			if (FindRadialSpot(owner, search->_origin, radius, spot)) // grayman #3857
			{
				// the spot is good
				memory.consecutiveRadialSpotFailures = 0; // reset
				memory.currentSearchSpot = spot; // spot to observe from
				memory.guardingAngle = idMath::INFINITY; // face search origin when spot is reached
				memory.guardingInProgress = true;
				memory.millingInProgress = false;
				owner->searchSubsystem->SwitchTask(TaskPtr(GuardSpotTask::CreateInstance())); // grayman #3857 - switch from action to search
			}

			// grayman #3857 - couldn't find a spot to observe from; only allow
			// a certain number of consecutive failures before skipping
			// observing entirely

			else if (++memory.consecutiveRadialSpotFailures >= MAX_RADIAL_SPOT_ATTEMPTS)
			{
				// There's no place for you in the search.
				
				// If, perchance, you have LOS to the search origin, just
				// stand where you are and face the search origin until your alert level drops.
				
				if (owner->CanSeePositionExt(search->_origin, false, false))
				{
					// where you are is good

					memory.consecutiveRadialSpotFailures = 0; // reset
					memory.currentSearchSpot = owner->GetPhysics()->GetOrigin(); // spot to observe from
					memory.guardingAngle = idMath::INFINITY; // face search origin
					memory.guardingInProgress = true;
					memory.millingInProgress = false;
					owner->searchSubsystem->SwitchTask(TaskPtr(GuardSpotTask::CreateInstance())); // grayman #3857 - switch from action to search
				}
				else
				{
					// grayman - this section can't run because it causes
					// the stealth score to rise dramatically during player sightings
					// If you don't have LOS, drop your
					// alert level below searching and continue with whatever
					// you were doing before.
					//owner->SetAlertLevel(owner->thresh_3 - 0.1);
				}
			}
		}
	}
}

bool SearchingState::FindRadialSpot(idAI* owner, idVec3 searchOrigin, float radius, idVec3 &spot)
{
	// Take 5 attempts at finding a spot that doesn't require
	// having to run through the search area to get to it.

	idVec3 candidateSpot;
	bool spotGood = false;
	idVec3 ownerOrigin = owner->GetPhysics()->GetOrigin();
	float distAI2SearchOriginSqr = (searchOrigin - ownerOrigin).LengthSqr();
	idVec3 dir;

	for ( int i = 0 ; i < 5 ; i++ )
	{
		dir = idAngles( 0, gameLocal.random.RandomInt(360), 0 ).ToForward();
		dir.NormalizeFast();
		candidateSpot = searchOrigin + radius*dir;

		float distAI2CandidateSqr = (candidateSpot - ownerOrigin).LengthSqr();

		// The spot is good if:
		// 1 - the ai is inside the search radius
		// or
		// 2 - the ai is farther from the search origin than from the spot

		if ((distAI2CandidateSqr < distAI2SearchOriginSqr) || ((searchOrigin - ownerOrigin).LengthFast() < radius))
		{
			// grayman #4238 - is another AI already there?
			if ( owner->PointObstructed(candidateSpot) )
			{
				continue;
			}

			spotGood = true;
			break;
		}
	}

	if (!spotGood)
	{
		// Give up and take anything.

		dir = idAngles( 0, gameLocal.random.RandomInt(360), 0 ).ToForward();
		dir.NormalizeFast();
		candidateSpot = searchOrigin + radius*dir;

		// grayman #4238 - is another AI already there?
		if ( owner->PointObstructed(candidateSpot) )
		{
			return false;
		}
	}

	// You must be able to see the search origin from this location.
	// This keeps locations from being chosen in other rooms.

	// Find the floor first.

	idVec3 start = candidateSpot;
	idVec3 end = candidateSpot;
	end.z -= 300;
	trace_t result;
	if ( gameLocal.clip.TracePoint(result, start, end, MASK_OPAQUE, NULL) )
	{
		// found floor; is there LOS from an eye above the spot to the search origin?
		candidateSpot = result.endpos;
		idVec3 eyePos = candidateSpot + idVec3(0,0,77.0f); // assume eye is 77 above feet
		if ( !gameLocal.clip.TracePoint(result, searchOrigin, eyePos, MASK_OPAQUE, NULL) )
		{
			spot = candidateSpot;

			// grayman #4238 - is another AI already there?
			if ( owner->PointObstructed(candidateSpot) )
			{
				return false;
			}

			// grayman #3857 - can we walk to the spot?

			int ownerAreaNum = owner->PointReachableAreaNum(ownerOrigin, 1.0f);
			aasPath_t path;
			int spotAreaNum = owner->PointReachableAreaNum(spot);
			if (spotAreaNum > 0)
			{
				if (owner->PathToGoal(path, ownerAreaNum, ownerOrigin, spotAreaNum, spot, owner))
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool SearchingState::OnAudioAlert(idStr soundName, bool addFuzziness, idEntity* maker) // grayman #3847 // grayman #3857
{
	// First, call the base class
	if (!State::OnAudioAlert(soundName,addFuzziness, maker)) // grayman #3847
	{
		return true;
	}

	idAI* owner = _owner.GetEntity();
	assert(owner != NULL);
	Memory& memory = owner->GetMemory();

	if (!memory.alertPos.Compare(memory.currentSearchSpot, 50))
	{
		// The position of the sound is different from the current search spot, so redefine the goal
		TaskPtr curTask = owner->searchSubsystem->GetCurrentTask(); // grayman #3857 - switch from action to search 
		InvestigateSpotTaskPtr investigateSpotTask = std::dynamic_pointer_cast<InvestigateSpotTask>(curTask);
		if (investigateSpotTask)
		{
			investigateSpotTask->SetNewGoal(memory.alertPos); // Redirect the owner to a new position
			investigateSpotTask->SetInvestigateClosely(false);
			memory.restartSearchForHidingSpots = true; // grayman #3200
		}
		else
		{
			GuardSpotTaskPtr guardSpotTask = std::dynamic_pointer_cast<GuardSpotTask>(curTask);
			if (guardSpotTask)
			{
				guardSpotTask->SetNewGoal(memory.alertPos); // Redirect the owner to a new position
				memory.restartSearchForHidingSpots = true; // grayman #3200
			}
		}
	}
	else
	{
		// grayman #3200 - we're about to ignore the new sound and continue with
		// the current search, but we should at least turn toward the new sound
		// to acknowledge having heard it

		// grayman #3424 - If we're on the move, only look at alertPos. Turning toward
		// it disrupts whatever movement we're doing. If we're standing still, turn
		// toward the spot as well as look at it.

		//owner->StopMove(MOVE_STATUS_DONE);
		if ( !owner->AI_FORWARD )
		{
			owner->TurnToward(memory.alertPos);
		}

		idVec3 target = memory.alertPos;
		target.z += 32;
		owner->Event_LookAtPosition(target,MS2SEC(LOOK_AT_AUDIO_SPOT_DURATION + (gameLocal.random.RandomFloat() - 0.5)*1000));
	}

	if (!memory.restartSearchForHidingSpots)
	{
		if (memory.alertSearchCenter != idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY))
		{
			// We have a valid search center
			float distanceToSearchCenter = (memory.alertPos - memory.alertSearchCenter).LengthSqr();
			if (distanceToSearchCenter > memory.alertSearchVolume.LengthSqr())
			{
				// The alert position is far from the current search center, restart search
				memory.restartSearchForHidingSpots = true;
			}
		}
	}
	return true;
}

bool SearchingState::StartNewHidingSpotSearch(idAI* owner) // grayman #3857
{
	int newSearchID = gameLocal.m_searchManager->ObtainSearchID(owner);
	if (newSearchID < 0)
	{
		return false;
	}

	bool assigned = (newSearchID == owner->m_searchID);

	if (!assigned)
	{
		assigned = gameLocal.m_searchManager->JoinSearch(newSearchID,owner); // gives the ai his assignment
	}

	if (!assigned)
	{
		return false;
	}

	// Clear ai flags
	ai::Memory& memory = owner->GetMemory();
	memory.restartSearchForHidingSpots = false;
	memory.noMoreHidingSpots = false;
	memory.mandatory = false; // grayman #3331

	// Clear all the ongoing tasks
	owner->senseSubsystem->ClearTasks();
	//owner->actionSubsystem->ClearTasks(); // grayman #3857
	owner->movementSubsystem->ClearTasks();

	// Stop moving
	owner->StopMove(MOVE_STATUS_DONE);
	memory.StopReacting(); // grayman #3559

	owner->MarkEventAsSearched(memory.currentSearchEventID); // grayman #3424

	memory.lastAlertPosSearched = memory.alertPos; // grayman #3492

	// greebo: Remember the initial alert position
	memory.alertSearchCenter = memory.alertPos;

	// If we are supposed to search the stimulus location do that instead 
	// of just standing around while the search completes
	if (memory.stimulusLocationItselfShouldBeSearched)
	{
		// The InvestigateSpotTask will take this point as first hiding spot
		// It's okay for the AI to move toward the alert position, even if he's
		// later assigned to be a guard.
		memory.currentSearchSpot = memory.alertPos;

		// Delegate the spot investigation to a new task, this will take the correct action.
		owner->searchSubsystem->SwitchTask(TaskPtr(new InvestigateSpotTask(memory.investigateStimulusLocationClosely))); // grayman #3857 - switch from action to search

		// Prevent overwriting this hiding spot in the upcoming Think() call
		memory.hidingSpotInvestigationInProgress = true;

		// Reset flag
		memory.investigateStimulusLocationClosely = false;
	}
	else
	{
		// AI is not moving, wait for spot search to complete
		memory.hidingSpotInvestigationInProgress = false;
		memory.currentSearchSpot = idVec3(idMath::INFINITY, idMath::INFINITY, idMath::INFINITY);
	}

	// Hiding spot test now started
	memory.hidingSpotSearchDone = false;
	memory.hidingSpotTestStarted = true;

	Search *search = gameLocal.m_searchManager->GetSearch(newSearchID);

	// grayman #4220 - only deploy a hiding spot search if the search type needs it

	int res = 0;
	if ( cv_ai_search_type.GetInteger() < 3 )
	{
		// Start search
		// TODO: Is the eye position necessary? Since the hiding spot list can be
		// used by several AI, why is the first AI's eye position relevant
		// to the other AIs' eye positions?
		res = gameLocal.m_searchManager->StartSearchForHidingSpotsWithExclusionArea(search, owner->GetEyePosition(), 255, owner);
	}

	if (res == 0)
	{
		// Search completed on first round
		memory.hidingSpotSearchDone = true;
	}

	return true;
}

void SearchingState::OnSubsystemTaskFinished(idAI* owner, SubsystemId subSystem)
{
/* grayman #2560 - InvestigateSpotTask is the only task this was needed
   for, and this code has been moved to a new OnFinish() for that task. No longer
   needed here.

	Memory& memory = owner->GetMemory();

	if (memory.hidingSpotInvestigationInProgress && subSystem == SubsysAction)
	{
		// The action subsystem has finished investigating the spot, set the
		// boolean back to false, so that the next spot can be chosen
		memory.hidingSpotInvestigationInProgress = false;
	}
 */
}

StatePtr SearchingState::CreateInstance()
{
	return StatePtr(new SearchingState);
}

// Register this state with the StateLibrary
StateLibrary::Registrar searchingStateRegistrar(
	STATE_SEARCHING, // Task Name
	StateLibrary::CreateInstanceFunc(&SearchingState::CreateInstance) // Instance creation callback
);

} // namespace ai
