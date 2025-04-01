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



#include "../Game_local.h"

#pragma warning(disable : 4996)

#include "MissionData.h"
#include "../ai/Memory.h"
#include "../DifficultyManager.h"
#include "../Player.h"
#include "../StimResponse/StimResponseCollection.h"
#include "../Missions/MissionManager.h"

#include "Objective.h"
#include "CampaignStatistics.h"
#include "ObjectiveLocation.h"
#include "ObjectiveCondition.h"
#include "../Shop/Shop.h" // grayman #3731

/**
* Add in new specification types here.  Must be in exact same order as
*	ESpecificationMethod enum, defined in MissionData.h
**/
static const char *gSpecTypeName[SPEC_COUNT] =
{
	"none",
	"name",
	"overall",
	"group",
	"classname",
	"spawnclass",
	"ai_type",
	"ai_team",
	"ai_innocence"
};

// TODO: Move to config file or player spawnargs
const int s_FAILURE_FADE_TIME = 6000; // grayman #3848 - was 3000

CMissionData::CMissionData() :
	m_MissionDataLoadedIntoGUI(false),
	m_mapFile(NULL),
	m_PlayerTeam(0)
{
	Clear();

	// Initialize Hash indexes used for parsing string names to enum index
	idStrList CompTypeNames, SpecTypeNames;

	for (int i = 0; i < COMP_COUNT; i++)
	{
		CompTypeNames.Append(gCompTypeName[i]);
	}

	for (int i = 0; i < SPEC_COUNT; i++)
	{
		SpecTypeNames.Append(gSpecTypeName[i]);
	}

	CompTypeNames.Condense();
	SpecTypeNames.Condense();

	for (int i=0; i < CompTypeNames.Num(); i++)
	{
		m_CompTypeHash.Add( m_CompTypeHash.GenerateKey( CompTypeNames[i].c_str(), false ), i );
	}
	for (int i=0; i < SpecTypeNames.Num(); i++)
	{
		m_SpecTypeHash.Add( m_SpecTypeHash.GenerateKey( SpecTypeNames[i].c_str(), false ), i );
	}

	m_ObjNote = true;
}

CMissionData::~CMissionData( void )
{
	Clear();
}

void CMissionData::Clear( void )
{
	m_bObjsNeedUpdate = false;
	m_Objectives.ClearFree();
	m_ClockedComponents.ClearFree();

	// Clear all the stats 
	m_Stats.Clear();

	m_SuccessLogicStr = "";
	m_FailureLogicStr = "";

	m_SuccessLogic.Clear();
	m_FailureLogic.Clear();

	m_PlayerTeam = 0;

	m_hasMissionEnded = false;

	if (m_mapFile != NULL)
	{
		delete m_mapFile;
		m_mapFile = NULL;
	}
}

void CMissionData::Save(idSaveGame* savefile) const
{
	savefile->WriteInt(m_PlayerTeam);
	savefile->WriteBool(m_bObjsNeedUpdate);
	
	savefile->WriteInt(m_Objectives.Num());
	for (int i = 0; i < m_Objectives.Num(); i++)
	{
		m_Objectives[i].Save(savefile);
	}
	savefile->WriteBool(m_ObjNote); // Obsttorte #5967
	m_Stats.Save(savefile);

	savefile->WriteString(m_SuccessLogicStr);
	savefile->WriteString(m_FailureLogicStr);

	savefile->WriteBool(m_hasMissionEnded);
}

void CMissionData::Restore(idRestoreGame* savefile)
{
	int num(0);

	m_mapFile = NULL;

	savefile->ReadInt(m_PlayerTeam);
	savefile->ReadBool( m_bObjsNeedUpdate );
	
	savefile->ReadInt(num);
	m_Objectives.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		m_Objectives[i].Restore(savefile);
	}
	savefile->ReadBool(m_ObjNote); // Obsttorte #5967
	// Rebuild list of clocked components now that we've loaded objectives
	m_ClockedComponents.Clear();
	for (int ind = 0; ind < m_Objectives.Num(); ind++)
	{
		for (int ind2 = 0; ind2 < m_Objectives[ind].m_Components.Num(); ind2++)
		{
			CObjectiveComponent& comp = m_Objectives[ind].m_Components[ind2];

			if (comp.m_Type == COMP_CUSTOM_CLOCKED || comp.m_Type == COMP_DISTANCE || comp.m_Type == COMP_INFO_LOCATION)
			{
				m_ClockedComponents.Append( &comp );
			}
		}
	}

	m_Stats.Restore(savefile);

	savefile->ReadString(m_SuccessLogicStr);
	savefile->ReadString(m_FailureLogicStr);

	savefile->ReadBool(m_hasMissionEnded);

	// re-parse the logic strings
	ParseLogicStrs();

	// We'll need a GUI update in any case
	m_MissionDataLoadedIntoGUI = false;
}

void CMissionData::MissionEvent
	(
		EComponentType CompType,
		SObjEntParms *EntDat1,
		SObjEntParms *EntDat2,
		bool bBoolArg
	)
{
	DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("Objectives: Mission event called \r");
	SStat *pStat(NULL);
	bool bCompState;

	if( !EntDat1 )
	{
		// log error
		goto Quit;
	}

	// Update AI stats, don't add to stats if playerresponsible is false
	// Stats for KOs, kills, body found, item found
	if( ( CompType == COMP_KILL || CompType == COMP_KO
		|| CompType == COMP_AI_FIND_BODY || CompType == COMP_AI_FIND_ITEM
		|| CompType == COMP_ALERT ) && bBoolArg )
	{
		DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("Objectives: Determined AI event \r");
		if( CompType == COMP_ALERT )
		{
			if( EntDat1->value > MAX_ALERTLEVELS )
			{
				// log error
				goto Quit;
			}
			// index in this array is determined by alert value
			pStat = &m_Stats.AIAlerts[ EntDat1->value ];

			// grayman #4002 - Decide which alert level to record for the stealth score.
			// If this new alert level is too close to a previous alert level,
			// then the previous alert needs to be removed.

			// This only applies if the new alert level is higher than EObservant.
			if ( EntDat1->bIsAI )
			{
				int alertLevel = EntDat1->value;
				if ( alertLevel > ai::EObservant )
				{
					idEntity* ent = gameLocal.FindEntity(EntDat1->name.c_str());
					if ( ent->IsType(idAI::Type) )
					{
						// Has this AI registered a previous alert at a lower level,
						// during this alert cycle? An alert cycle is where an AI rises
						// from Idle to something higher, then drops back down to idle.
						idAI *ai = static_cast<idAI*>(ent);
						int removeThisAlertLevel = ai->ExamineAlerts();
						if (removeThisAlertLevel > 0)
						{
							SStat *pStatPrevious = &m_Stats.AIAlerts[removeThisAlertLevel];
							// Subtract from all appropriate stats
							pStatPrevious->Overall--;
							pStatPrevious->ByTeam[EntDat1->team]--;
							pStatPrevious->ByType[EntDat1->type]--;
							pStatPrevious->ByInnocence[EntDat1->innocence]--;

							// Even though pStatPrevious->WhileAirborne might not be the same now
							// as it was when this previous alert was registered, don't worry about it. It isn't used.
						}
					}
				}
			}
		}
		else
		{
			pStat = &m_Stats.AIStats[ CompType ];
		}

		if( CompType > MAX_AICOMP || !pStat)
		{
			DM_LOG(LC_OBJECTIVES,LT_ERROR)LOGSTRING("Objectives: No AI stat found for comptype %d\r", CompType );
			goto Quit;
		}

		// Add to all appropriate stats
		pStat->Overall++;
		pStat->ByTeam[ EntDat1->team ]++;
		pStat->ByType[ EntDat1->type ]++;
		pStat->ByInnocence[ EntDat1->innocence ]++;

		if( EntDat1->bWhileAirborne )
			pStat->WhileAirborne++;

		DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("Objectives: Done adding to stats, checking for objectives...\r" );
	}

	// Update pickpocket stat
	if( CompType == COMP_PICKPOCKET && bBoolArg )
		m_Stats.PocketsPicked++;

	// Check which objective components need updating
	for( int i=0; i<m_Objectives.Num(); i++ )
	{
		CObjective& obj = m_Objectives[i];

		for( int j=0; j < obj.m_Components.Num(); j++ )
		{
			CObjectiveComponent& comp = obj.m_Components[j];

			// match component type
			if( comp.m_Type != CompType )
				continue;
			DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("Objectives: Matching Component found: %d, %d\r", i+1, j+1 );

			// check if the specifiers match, for first spec and second if it exists
			if( !MatchSpec(&comp, EntDat1, 0) )
				continue;
			DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("Objectives: First specification check matched: %d, %d\r", i+1, j+1 );

			if( comp.m_SpecMethod[1] != SPEC_NONE )
			{
				if( !MatchSpec(&comp, EntDat2, 1) )
					continue;
			}
			DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("Objectives: Second specification check matched or absent: %d, %d\r", i+1, j+1 );

			bCompState = EvaluateObjective( &comp, EntDat1, EntDat2, bBoolArg );
			DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("Objective component evaluation result: %d \r", (int) bCompState );

			// notify the component of the current state. If the state changed,
			// this will return true and we must mark this objective for update.
			if( comp.SetState( bCompState ) )
			{
				// greebo: Check for irreversible objectives that have already "snapped" into their final state
				if (!obj.m_bReversible && obj.m_bLatched)
				{
					// don't re-evaluate latched irreversible objectives
					continue;
				}

				DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("Objective %d, Component %d state changed, needs updating", i+1, j+1 );
				obj.m_bNeedsUpdate = true;
				m_bObjsNeedUpdate = true;
			}
		}
	}
Quit:
	return;
}

void CMissionData::MissionEvent
	(
	EComponentType CompType,
	idEntity *Ent1, idEntity *Ent2,
	bool bBoolArg, bool bWhileAirborne
	)
{
	SObjEntParms data1, data2;

	// at least the first ent must exist
	if(!Ent1)
	{
		// log error
		return;
	}
	FillParmsData( Ent1, &data1 );
	data1.bWhileAirborne = bWhileAirborne;

	if( !Ent2 )
		MissionEvent( CompType, &data1, NULL, bBoolArg );
	else
	{
		FillParmsData( Ent2, &data2 );
		MissionEvent( CompType, &data1, &data2, bBoolArg );
	}
}

bool	CMissionData::MatchSpec
			(
			CObjectiveComponent *pComp,
			SObjEntParms *EntDat,
			int ind
			)
{
	bool bReturnVal(false);
	
	// objectives only have two specified ents at max
	if( !pComp || !EntDat || ind > 1 )
	{
		return false;
	}

	ESpecificationMethod SpecMethod = pComp->m_SpecMethod[ ind ];

	switch( SpecMethod )
	{
		case SPEC_NONE:
			bReturnVal = true;
			break;
		case SPEC_NAME:
			bReturnVal = ( pComp->m_SpecVal[ind] == EntDat->name );
			break;
		case SPEC_OVERALL:
			bReturnVal = true;
			break;
		case SPEC_GROUP:
			bReturnVal = ( pComp->m_SpecVal[ind] == EntDat->group );
			
			// Special case for items:
			if( pComp->m_Type == COMP_ITEM && pComp->m_SpecVal[ind] == "loot_total" )
				bReturnVal = true;

			break;
		case SPEC_CLASSNAME:
			bReturnVal = ( pComp->m_SpecVal[ind] == EntDat->classname );
			break;
		case SPEC_SPAWNCLASS:
			bReturnVal = ( pComp->m_SpecVal[ind] == EntDat->spawnclass );
			break;
		case SPEC_AI_TYPE:
			bReturnVal = ( atoi(pComp->m_SpecVal[ind]) == EntDat->type );
			break;
		case SPEC_AI_TEAM:
			bReturnVal = ( atoi(pComp->m_SpecVal[ind]) == EntDat->team );
			break;
		case SPEC_AI_INNOCENCE:
			bReturnVal = ( atoi(pComp->m_SpecVal[ind]) == EntDat->innocence );
			break;
		default:
			break;
	}

	return bReturnVal;
}

bool	CMissionData::EvaluateObjective
			(
			CObjectiveComponent *pComp,
			SObjEntParms *EntDat1,
			SObjEntParms *EntDat2,
			bool bBoolArg
			)
{
	bool bReturnVal(false);
	int value(0);

	EComponentType CompType = pComp->m_Type;
	ESpecificationMethod SpecMeth = pComp->m_SpecMethod[0];

	// LOCATION : If we get this far with location, the specifiers
	// already match, that means it's already true and no further evaluation is needed
	if( CompType == COMP_LOCATION )
	{
		// Return value is set to whether the item entered or left the location
		bReturnVal = bBoolArg;
		goto Quit;
	}

	// Player inventory items:
	else if( CompType == COMP_ITEM )
	{
		// name, classname and spawnclass are all one-shot objectives and not counted up (for now)
		if( SpecMeth == SPEC_NONE || SpecMeth == SPEC_NAME || SpecMeth == SPEC_CLASSNAME || SpecMeth == SPEC_SPAWNCLASS )
		{
			// Returnval is set based on whether item is entering or leaving inventory
			bReturnVal = bBoolArg;
			goto Quit;
		}

		switch( SpecMeth )
		{
			// overall loot
			case SPEC_OVERALL:
				// greebo: Take the stored Total Loot Value, not the supergroup stuff, 
				//         as non-loot items always have supergroup set to 1.
				value = GetFoundLoot();
				//value = EntDat1->valueSuperGroup;
				break;
			case SPEC_GROUP:
				value = EntDat1->value;

				// special case for overall loot
				if( pComp->m_SpecVal[1] == "loot_total" )
					value = GetFoundLoot();
				break;
			default:
				break;
		}
		bReturnVal = value >= atoi(pComp->m_Args[0]);
	}

	// AI ALERTS: Need to check against alert level
	else if( CompType == COMP_ALERT )
	{
		if( pComp->m_bPlayerResponsibleOnly && !bBoolArg )
		{
			goto Quit;
		}

		// The second arguments holds the minimum alert level
		int AlertLevel = atoi(pComp->m_Args[1]);

		// EntDat->value holds the alert level the AI has been alerted to
		if( EntDat1->value >= AlertLevel )
		{
			pComp->m_EventCount++;
		}

		value = pComp->m_EventCount;

		// greebo: The first component argument holds the number of times this event should happen
		bReturnVal = value >= atoi(pComp->m_Args[0]);
	}
	else if (CompType == COMP_READABLE_PAGE_REACHED) // checks page number
	{
		// The argument holds the page to be reached
		int pageToBeReached = atoi(pComp->m_Args[0]);

		// The value holds the page which has been reached
		if (EntDat1->value == pageToBeReached)
		{
			pComp->m_EventCount++;
			return true; // success
		}

		return false; // fail by default
	}
	// Everything else: Increment and check event counter
	else
	{
		if( pComp->m_bPlayerResponsibleOnly && !bBoolArg )
			goto Quit;

		pComp->m_EventCount++;

		value = pComp->m_EventCount;
		bReturnVal = value >= atoi(pComp->m_Args[0]);
	}

Quit:
	return bReturnVal;
}

void CMissionData::UpdateObjectives( void )
{
	bool bObjEnabled(true);

// =============== Begin Handling of Clocked Objective Components ===============

	for( int k=0; k < m_ClockedComponents.Num(); k++ )
	{
		CObjectiveComponent *pComp = m_ClockedComponents[k];

		// check if timer is due to fire
		if( !pComp  )
			continue;

		// if parent objective is invalid or the timer hasn't fired or it's latched, don't do anything
		// greebo: Beware the the m_Index is 1-based, not 0-based
		if( m_Objectives[ pComp->m_Index[0] - 1 ].m_state == STATE_INVALID
			|| (gameLocal.time - pComp->m_TimeStamp < pComp->m_ClockInterval)
			|| pComp->m_bLatched )
		{
			continue;
		}

// COMP_DISTANCE - Do a distance check
		else if( pComp->m_Type == COMP_DISTANCE )
		{
			pComp->m_TimeStamp = gameLocal.time;

			if( pComp->m_Args.Num() < 3 )
				continue;

			idEntity* ent1 = gameLocal.FindEntity( pComp->m_Args[0] );
			idEntity* ent2 = gameLocal.FindEntity( pComp->m_Args[1] );

			if (ent1 == NULL || ent2 == NULL)
			{
				DM_LOG(LC_OBJECTIVES, LT_WARNING)LOGSTRING("Objective %d, component %d: Distance objective component given bad entity names %s , %s \r", pComp->m_Index[0], pComp->m_Index[1], pComp->m_Args[0].c_str(), pComp->m_Args[1].c_str() );
				continue;
			}

			idVec3 delta = ent1->GetPhysics()->GetOrigin() - ent2->GetPhysics()->GetOrigin();

			float dist = atof(pComp->m_Args[2]);
			dist *= dist;

			SetComponentState( pComp, ( delta.LengthSqr() < dist ) );
		}

// COMP_INFO_LOCATION - Check if an ent by name is in an info_location or info_location group
		else if( pComp->m_Type == COMP_INFO_LOCATION )
		{
			bool bEval(false);
			idEntity *checkEnt = NULL;

			// Spec method 0 is always by name, so no need to check it.
			// we should indicate this in the objective setup GUI
			checkEnt = gameLocal.FindEntity( pComp->m_SpecVal[0].c_str() );
			if( !checkEnt )
			{
				DM_LOG(LC_OBJECTIVES, LT_WARNING)LOGSTRING("Objective %d, component %d: Info_location objective could not find entity: %s \r", pComp->m_Index[0], pComp->m_Index[1], pComp->m_SpecVal[0].c_str() );
				continue;
			}

			idLocationEntity *loc = gameLocal.LocationForPoint( checkEnt->GetPhysics()->GetOrigin() );
			if( loc )
			{
				if( pComp->m_SpecMethod[1] == SPEC_GROUP )
					bEval = (pComp->m_SpecVal[1] == loc->m_ObjectiveGroup );
				else
					bEval = ( pComp->m_SpecVal[1] == loc->name );
			}

			SetComponentState( pComp, bEval );
		}

// COMP_CUSTOM_CLOCKED - Run a clocked script
		else if( pComp->m_Type == COMP_CUSTOM_CLOCKED )
		{
			pComp->m_TimeStamp = gameLocal.time;

			function_t *pScriptFun = gameLocal.program.FindFunction( pComp->m_Args[0].c_str() );

			if(pScriptFun)
			{
				idThread *pThread = new idThread( pScriptFun );
				pThread->CallFunction( pScriptFun, true );
				pThread->DelayedStart( 0 );
			}
			else
			{
				DM_LOG(LC_OBJECTIVES, LT_WARNING)LOGSTRING("Objective %d, component %d: Custom clocked objective called bad script: %s \r", pComp->m_Index[0], pComp->m_Index[1], pComp->m_Args[0].c_str() );
				gameLocal.Printf("WARNING: Objective %d, component %d: Custom clocked objective called bad script: %s \n", pComp->m_Index[0], pComp->m_Index[1], pComp->m_Args[0].c_str() );
			}
		}
	}

// ============== End Handling of Clocked Objective Components =============

	// Check if any objective states have changed:
	if( !m_bObjsNeedUpdate )
	{
		return;
	}
	m_bObjsNeedUpdate = false;

	DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("Objectives: Objectives in need of updating \r");

	for( int i=0; i<m_Objectives.Num(); i++ )
	{
		CObjective& obj = m_Objectives[i];

		// skip objectives that don't need updating
		if( !obj.m_bNeedsUpdate || obj.m_state == STATE_INVALID )
			continue;

		obj.m_bNeedsUpdate = false;

		DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("Objectives: Found objective in need of update: %d \r", i+1);

		// If objective was just completed
		if( obj.CheckSuccess() )
		{
			// greebo: Set the bool back to true before evaluating the components
			bObjEnabled = true;

			// Check for enabling objectives
			for( int k=0; k < obj.m_EnablingObjs.Num(); k++ )
			{
				// Decrease the index to the internal range [0..N)
				int ObjNum = obj.m_EnablingObjs[k] - 1;

				if( ObjNum >= m_Objectives.Num() || ObjNum < 0 ) continue;

				CObjective& obj = m_Objectives[ObjNum];

				EObjCompletionState CompState = obj.m_state;

				// greebo: The enabling objective must be either complete or an ongoing one 
				// the latter of which are considered complete unless they are failed.
				bool temp = CompState == STATE_COMPLETE || CompState == STATE_INVALID || obj.m_bOngoing;

				bObjEnabled = bObjEnabled && temp;
			}

			if( !bObjEnabled )
			{
				return;
			}

			DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("Objectives: Objective %d COMPLETED\r", i+1);
			SetCompletionState( i, STATE_COMPLETE );
		}
		else if( obj.CheckFailure() )
		{
			DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("Objectives: Objective %d FAILED\r", i+1);
			SetCompletionState(i, STATE_FAILED );
		}
		else
		{
			// greebo: Set the objective state to INCOMPLETE, but use SetCompletionState() to
			// consider irreversible objectives.
			DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("Objectives: Objective %d INCOMPLETE\r", i+1);
			SetCompletionState(i, STATE_INCOMPLETE );
		}
	}
}

void CMissionData::Event_ObjectiveComplete( int ind )
{
	bool missionComplete = true;

	if( !m_SuccessLogic.IsEmpty() ) 
	{
		missionComplete = EvalBoolLogic( &m_SuccessLogic, true );
	}
	else
	{
		// default logic: check if all mandatory, valid objectives have been completed
		// If so, the mission is complete
		for (int i = 0; i < m_Objectives.Num(); i++)
		{
			CObjective& obj = m_Objectives[i];

			// greebo: only check visible and applicable objectives
			// Ongoing and optional ones are considered as complete
			if (obj.m_bVisible && obj.m_bApplies)
			{
				bool temp = ( obj.m_state == STATE_COMPLETE || obj.m_state == STATE_INVALID || !obj.m_bMandatory || obj.m_bOngoing);
				missionComplete = missionComplete && temp;
			}
		}
	}

	if (missionComplete)
	{
		// All objectives ok, mission complete
		Event_MissionComplete();
		return;
	}

	const CObjective& obj = m_Objectives[ind];

	// Call the objective completion script (even for ongoing objectives)
	function_t* pScriptFun = gameLocal.program.FindFunction( obj.m_CompletionScript );
	if (pScriptFun != NULL)
	{
		idThread* pThread = new idThread(pScriptFun);
		pThread->CallFunction( pScriptFun, true );
		pThread->DelayedStart(0);
	}

	idPlayer* player = gameLocal.GetLocalPlayer();
	if (player == NULL)
	{
		gameLocal.Error("No player at objective complete!\n");
	}

	// Activate the completion target
	const idStr& targetName = obj.m_CompletionTarget;
	if (!targetName.IsEmpty())
	{
		idEntity* target = gameLocal.FindEntity(targetName);

		if (target != NULL)
		{
			DM_LOG(LC_OBJECTIVES,LT_INFO)LOGSTRING("Objectives: Triggering completion target %s for objective #%d\r", targetName.c_str(), ind);
			target->Activate(player);
		}
		else
		{
			DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("Objectives: Could not find completion target %s for objective #%d\r", targetName.c_str(), ind);
		}
	}

	// Only this objective is complete, not the entire mission
	// Ongoing objectives don't play the sound or mark off in the GUI as complete during mission
	// greebo: Don't play sound or display message for invisible objectives
	if (!obj.m_bOngoing && obj.m_bVisible)
	{
		if (m_ObjNote) // Obsttorte (#5967)
		{
			player->StartSound("snd_objective_complete", SND_CHANNEL_ANY, 0, false, NULL);

			// greebo: Notify the player
			player->SendHUDMessage("#str_02453"); // "Objective complete"
		}
		player->UpdateObjectivesGUI();
	}
}

void CMissionData::Event_ObjectiveFailed(int ind)
{
	// play an objective failed sound for optional objectives?
	const CObjective& obj = m_Objectives[ind];

	// Call failure script
	function_t *pScriptFun = gameLocal.program.FindFunction( obj.m_FailureScript );
	if (pScriptFun != NULL)
	{
		idThread *pThread = new idThread(pScriptFun);
		pThread->CallFunction( pScriptFun, true );
		pThread->DelayedStart( 0 );
	}

	idPlayer* player = gameLocal.GetLocalPlayer();
	assert(player != NULL);

	// Activate the failure target
	const idStr& targetName = obj.m_FailureTarget;
	if (!targetName.IsEmpty())
	{
		idEntity* target = gameLocal.FindEntity(targetName);

		if (target != NULL)
		{
			DM_LOG(LC_OBJECTIVES,LT_INFO)LOGSTRING("Objectives: Triggering failure target %s for objective #%d\r", targetName.c_str(), ind);
			target->Activate(player);
		}
		else
		{
			DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("Objectives: Could not find failure target %s for objective #%d\r", targetName.c_str(), ind);
		}
	}

	// greebo: Notify the player for visible objectives only
	if (obj.m_bVisible)
	{
		if (m_ObjNote) // Obsttorte (#5967)
		{
			player->StartSound("snd_objective_failed", SND_CHANNEL_ANY, 0, false, NULL);
			player->SendHUDMessage("#str_02454"); // "Objective failed"
		}
		player->UpdateObjectivesGUI();
	}

	// Check for mission failure
	bool missionFailed = false;

	if (!m_FailureLogic.IsEmpty())
	{
		missionFailed = EvalBoolLogic(&m_FailureLogic, true);
	}
	else
	{
		// default logic: if the objective was mandatory, fail the mission
		missionFailed = obj.m_bMandatory;
	}

	if (missionFailed)
	{
		Event_MissionFailed();
	}
}

void CMissionData::Event_NewObjective() 
{
	DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("Objectives: NEW OBJECTIVE. \r");
	gameLocal.Printf("NEW OBJECTIVE\n");

	idPlayer* player = gameLocal.GetLocalPlayer();
	if (player == NULL) return;
	if (m_ObjNote) // Obsttorte (#5967)
	{
		player->StartSound("snd_new_objective", SND_CHANNEL_ANY, 0, false, NULL);

		// greebo: notify the player
		player->SendHUDMessage("#str_02455"); // "New Objective"
	}
	player->UpdateObjectivesGUI();
}

void CMissionData::Event_MissionComplete()
{
	DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("Objectives: MISSION COMPLETED.\r");
	gameLocal.Printf("MISSION COMPLETED\n");

	// stgatilov #6489: block winning twice, since it often causes crash
	if (m_hasMissionEnded)
		return;
	m_hasMissionEnded = true;

	// stgatilov #6509: allow mapper to save game state into persistent info
	const function_t* func = gameLocal.program.FindFunction( "on_mission_complete" );
	if ( func ) {
		idThread *thread = new idThread( func );
		thread->DelayedStart( 0 );
	}

	// Fire the general mission end event
	Event_MissionEnd();

	// greebo: Stop the gameplay timer, we've completed all objectives
	m_Stats.TotalGamePlayTime = gameLocal.m_GamePlayTimer.GetTimeInSeconds();

	// Save the state of all objectives for possible later reference
	for (int i = 0; i < m_Objectives.Num(); ++i)
	{
		m_Stats.SetObjectiveState(i, m_Objectives[i].m_state);
	}

	// Copy our current mission statistics to the correct slot of the campaign statistics
	int curMission = gameLocal.m_MissionManager->GetCurrentMissionIndex();

	CampaignStats& campaignStats = *gameLocal.m_CampaignStats;
	campaignStats[curMission] = m_Stats;
	
	idPlayer* player = gameLocal.GetLocalPlayer();

	if (player != NULL)
	{
		// Remember the player team, all entities are about to be removed
		SetPlayerTeam(player->team);

		// This sound is played by the success.gui
		//player->StartSoundShader( declManager->FindSound( "mission_complete" ), SND_CHANNEL_ANY, 0, false, NULL );
		player->SendHUDMessage( "#str_02456" ); // "Mission Complete"
		player->PostEventMS(&EV_TriggerMissionEnd, 100);

		player->UpdateObjectivesGUI();

		// Notify the mission database
		gameLocal.m_MissionManager->OnMissionComplete();
	}
	
	// grayman #3723 - we no longer need the shop, so clear it
	// grayman #3731 - do this at mission end, not when done with shop,
	// because a Restart will want to use the shop again
	gameLocal.m_Shop->Clear();
}

void CMissionData::Event_MissionFailed( void )
{
	DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("Objectives: MISSION FAILED. \r");
	gameLocal.Printf("MISSION FAILED\n");

	// stgatilov #6489: block losing twice, since it often causes crash
	if (m_hasMissionEnded)
		return;
	m_hasMissionEnded = true;

	// stgatilov #6509: allow mapper to save game state into persistent info
	const function_t* func = gameLocal.program.FindFunction( "on_mission_failed" );
	if ( func ) {
		idThread *thread = new idThread( func );
		thread->DelayedStart( 0 );
	}

	// Fire the general mission end event
	Event_MissionEnd();

	// greebo: Notify the local game about this
	gameLocal.SetMissionResult(MISSION_FAILED);

	idPlayer *player = gameLocal.GetLocalPlayer();
	if(player)
	{
		player->playerView.Fade( colorBlack, s_FAILURE_FADE_TIME );
		player->PostEventMS( &EV_Player_DeathMenu, s_FAILURE_FADE_TIME + 1 );

		player->UpdateObjectivesGUI();
	}
}

void CMissionData::Event_MissionEnd()
{
	// grayman #2887 - make a final check if any AI have the player in sight.
	// Wrap up calculation of how long a player has been seen.

	// grayman #4002 - make a final pass through the queued alert events for
	// each AI to bring stealth score up to date

	// Obsttorte #5678 - calculate the total amount of pickables for the end mission screen
	m_Stats.PocketsTotal = m_Stats.PocketsPicked;

	for ( int i = 0 ; i < gameLocal.num_entities ; i++ )
	{
		idEntity* ent = gameLocal.entities[i];
		if ( !ent )
		{
			continue;
		}
		// Obsttorte (#5678) 
		if (ent->CanBePickedUp())
		{
			idEntity* master = ent->GetBindMaster();
			if (master && master->health > 0)
			{
				m_Stats.PocketsTotal++;
			}
		}
		if ( ent->IsType(idAI::Type) )
		{
			idAI* ai = static_cast<idAI*>(ent);
			if ( ai->GetEnemy() != NULL )
			{
				if ( ( ai->lastTimePlayerSeen > 0 ) && ( ai->lastTimePlayerSeen > ai->lastTimePlayerLost ) )
				{
					/*gameLocal.m_MissionData->*/Add2TimePlayerSeen(gameLocal.time - ai->lastTimePlayerSeen);
				}
			}
		}
	}
}

// ============================== Stats =================================

// SteveL #3304: Solution by Zbyl. Adding GetStat() function to support new
//               scriptevents, and converting existing accessors to use it too
SStat* CMissionData::GetStat( EComponentType CompType, int AlertLevel )
{
	if( CompType < 0 || CompType >= MAX_AICOMP )
	{
		return NULL;
	}

	if( AlertLevel < 0 || AlertLevel >= MAX_ALERTLEVELS )
	{
		return NULL;
	}

	if( CompType == COMP_ALERT )
	{
		return &m_Stats.AIAlerts[ AlertLevel ];
	}

	return &m_Stats.AIStats[ CompType ];
}

int CMissionData::GetStatOverall( EComponentType CompType, int AlertLevel )
{
	SStat* pStat = GetStat( CompType, AlertLevel );
	if( pStat == NULL )
	{
		return 0;
	}

	return pStat->Overall;
}

int CMissionData::GetStatByTeam( EComponentType CompType, int index, int AlertLevel )
{
	SStat* pStat = GetStat( CompType, AlertLevel );
	if( pStat == NULL )
	{
		return 0;
	}

	if( index < 0 || index >= MAX_TEAMS )
	{
		return 0;
	}

	return pStat->ByTeam[index];
}

int CMissionData::GetStatByType( EComponentType CompType, int index, int AlertLevel )
{
	SStat* pStat = GetStat( CompType, AlertLevel );
	if( pStat == NULL )
	{
		return 0;
	}

	if( index < 0 || index >= MAX_TYPES )
	{
		return 0;
	}

	return pStat->ByType[index];
}

int CMissionData::GetStatByInnocence( EComponentType CompType, int index, int AlertLevel )
{
	SStat* pStat = GetStat( CompType, AlertLevel );
	if( pStat == NULL )
	{
		return 0;
	}

	if( index < 0 || index > 1 )
	{
		return 0;
	}

	return pStat->ByInnocence[index];
}

int CMissionData::GetStatAirborne( EComponentType CompType, int AlertLevel )
{
	SStat* pStat = GetStat( CompType, AlertLevel );
	if( pStat == NULL )
	{
		return 0;
	}

	return pStat->WhileAirborne;
}

void CMissionData::AIDamagedByPlayer( int DamageAmount )
{
	m_Stats.DamageDealt += DamageAmount;
}

void CMissionData::PlayerDamaged( int DamageAmount )
{
	m_Stats.DamageReceived += DamageAmount;
}

unsigned int CMissionData::GetTotalGamePlayTime() // SteveL #3304, courtesy Zbyl.
{
	return m_Stats.TotalGamePlayTime;
}

int CMissionData::GetDamageDealt()
{
	return m_Stats.DamageDealt;
}

int CMissionData::GetDamageReceived()
{
	return m_Stats.DamageReceived;
}

int CMissionData::GetHealthReceived()
{
	return m_Stats.HealthReceived;
}

int CMissionData::GetPocketsPicked() // SteveL #3304, courtesy Zbyl
{
	return m_Stats.PocketsPicked;
}

void CMissionData::HealthReceivedByPlayer(int amount)
{
	m_Stats.HealthReceived += amount;
}

void CMissionData::HandleMissionEvent(idEntity* objEnt, EMissionEventType eventType, const char* argument)
{
	if (objEnt == NULL) return;

	// Setup the entity parameters
	SObjEntParms parms;

	FillParmsData(objEnt, &parms);

	switch (eventType)
	{
	case EVENT_NOTHING:
		break;
	case EVENT_READABLE_OPENED:
		MissionEvent(COMP_READABLE_OPENED, &parms, true);
		break;
	case EVENT_READABLE_CLOSED:
		MissionEvent(COMP_READABLE_CLOSED, &parms, true);
		break;
	case EVENT_READABLE_PAGE_REACHED:
		// The first argument should contain the reached page number
		parms.value = atoi(argument);
		MissionEvent(COMP_READABLE_PAGE_REACHED, &parms, true);
		break;
	default:
		gameLocal.Warning("Unknown event type encountered in HandleMissionEvent: %d", eventType);
		break;
	};
}

// ============================== Misc.  ==============================

void CMissionData::FillParmsData( idEntity *ent, SObjEntParms *parms )
{
	if (ent == NULL || parms == NULL) return;

	parms->name = ent->name;

	// group is interpreted differently for location entities
	if( ent->IsType(idLocationEntity::Type) || ent->IsType(CObjectiveLocation::Type) )
	{
		parms->group = ent->spawnArgs.GetString("objective_group");
	}
	else
	{
		parms->group = ent->spawnArgs.GetString("inv_name");
	}

	parms->classname = ent->spawnArgs.GetString("classname");
	parms->spawnclass = ent->spawnArgs.GetString("spawnclass");

	if( ent->IsType(idActor::Type) )
	{
		idActor *actor = static_cast<idActor *>(ent);

		parms->team = actor->team;
		parms->type = actor->m_AItype;
		parms->innocence = (int) actor->m_Innocent;
		parms->bIsAI = true;
	}
}


/* SteveL #3741: Eliminate SetComponentState_Ext to align SetComponentState with all other 
				 objective-related functions. Bounds check moved to CMissionData::SetComponentState.

void CMissionData::SetComponentState_Ext( int ObjIndex, int CompIndex, bool bState )
{
	DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("SetComponentState: Called for obj %d, comp %d, state %d. \r", ObjIndex, CompIndex, (int) bState );

	// Offset the indices into "internal" values (start at 0)
	ObjIndex--;
	CompIndex--;

	if( ObjIndex >= m_Objectives.Num() || ObjIndex < 0  )
	{
		DM_LOG(LC_OBJECTIVES,LT_WARNING)LOGSTRING("SetComponentState: Objective num %d out of bounds. \r", (ObjIndex+1) );
		return;
	}
	if( CompIndex >= m_Objectives[ObjIndex].m_Components.Num() || CompIndex < 0 )
	{
		DM_LOG(LC_OBJECTIVES,LT_WARNING)LOGSTRING("SetComponentState: Component num %d out of bounds for objective %d. \r", (CompIndex+1), (ObjIndex+1) );
		return;
	}

	// call internal SetComponentState
	SetComponentState( ObjIndex, CompIndex, bState );
}
*/

void CMissionData::SetComponentState(int ObjIndex, int CompIndex, bool bState)
{
	if ( ObjIndex >= m_Objectives.Num() || ObjIndex < 0 )
	{
		DM_LOG( LC_OBJECTIVES, LT_WARNING )LOGSTRING( "SetComponentState: Objective num %d out of bounds. \r", ( ObjIndex + 1 ) );
		gameLocal.Printf( "WARNING: Objective System: SetComponentState: Objective num %d out of bounds. \n", ( ObjIndex + 1 ) );
		return;
	}
	if ( CompIndex >= m_Objectives[ObjIndex].m_Components.Num() || CompIndex < 0 )
	{
		DM_LOG( LC_OBJECTIVES, LT_WARNING )LOGSTRING( "SetComponentState: Component num %d out of bounds for objective %d. \r", ( CompIndex + 1 ), ( ObjIndex + 1 ) );
		gameLocal.Printf( "WARNING: Objective System: SetComponentState: Component num %d out of bounds for objective %d. \n", ( CompIndex + 1 ), ( ObjIndex + 1 ) );
		return;
	}
	
	CObjectiveComponent& comp = m_Objectives[ObjIndex].m_Components[CompIndex];

	if( comp.SetState(bState) )
	{
		DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("SetComponentState: Objective %d, Component %d state changed, needs updating", (ObjIndex+1), (CompIndex+1) );
		m_Objectives[ObjIndex].m_bNeedsUpdate = true;
		m_bObjsNeedUpdate = true;
	}
}

void CMissionData::SetComponentState( CObjectiveComponent *pComp, bool bState )
{
	if (pComp == NULL) return;

	SetComponentState( pComp->m_Index[0]-1, pComp->m_Index[1]-1, bState );
}

void CMissionData::SetCompletionState( int ObjIndex, int State, bool fireEvents )
{
	if( ObjIndex >= m_Objectives.Num() || ObjIndex < 0 )
	{
		DM_LOG(LC_OBJECTIVES,LT_WARNING)LOGSTRING("Attempt was made to set completion state of invalid objective index: %d \r", ObjIndex );
		gameLocal.Printf("WARNING: Objective system: Attempt was made to set completion state of invalid objective index: %d \n", ObjIndex);
		return;
	}

	// check if the state int is valid by comparing to highest number in enum
	if( State < 0 || State > STATE_FAILED )
	{
		DM_LOG(LC_OBJECTIVES,LT_WARNING)LOGSTRING("Attempt was made to set objective index: %d to invalid completion state: %d \r", ObjIndex, State);
		gameLocal.Printf("WARNING: Objective system: Attempt was made to set objective index: %d to invalid completion state: %d \n", ObjIndex, State);
		return;
	}

	CObjective& obj = m_Objectives[ObjIndex];

	// Don't do anything if we are already in that state
	if( obj.m_state == State ) return;

	// Check for latching:
	if( !obj.m_bReversible )
	{
		// do not do anything if latched
		if( obj.m_bLatched )
		{
			return;
		}

		// Irreversible objectives latch to either complete or failed
		if( State == STATE_COMPLETE || State == STATE_FAILED )
		{
			obj.m_bLatched = true;
		}
	}

	obj.m_state = static_cast<EObjCompletionState>(State);

	if (fireEvents)
	{
		if( State == STATE_COMPLETE )
		{
			Event_ObjectiveComplete( ObjIndex );
		}
		else if( State == STATE_FAILED )
		{
			Event_ObjectiveFailed( ObjIndex );
		}
	}
}

// for scripters:

int CMissionData::GetCompletionState( int ObjIndex )
{
	if( ObjIndex >= m_Objectives.Num() || ObjIndex < 0 )
	{
		DM_LOG(LC_OBJECTIVES,LT_WARNING)LOGSTRING("GetCompletionState: Bad objective index: %d \r", ObjIndex );
		gameLocal.Printf("WARNING: Objective system: Attempt was made to get completion state of invalid objective index: %d \n", ObjIndex);
		return -1;
	}

	return m_Objectives[ObjIndex].m_state;
}

bool CMissionData::GetComponentState( int ObjIndex, int CompIndex )
{
	if( ObjIndex >= m_Objectives.Num() || ObjIndex < 0  )
	{
		DM_LOG(LC_OBJECTIVES,LT_WARNING)LOGSTRING("GetComponentState: Objective num %d out of bounds. \r", (ObjIndex+1) );
		gameLocal.Printf("WARNING: Objective System: GetComponentState: Objective num %d out of bounds. \n", (ObjIndex+1) );
		return false;
	}
	if( CompIndex >= m_Objectives[ObjIndex].m_Components.Num() || CompIndex < 0 )
	{
		DM_LOG(LC_OBJECTIVES,LT_WARNING)LOGSTRING("GetComponentState: Component num %d out of bounds for objective %d. \r", (CompIndex+1), (ObjIndex+1) );
		gameLocal.Printf("WARNING: Objective System: GetComponentState: Component num %d out of bounds for objective %d. \n", (CompIndex+1), (ObjIndex+1) );
		return false;
	}

	return m_Objectives[ObjIndex].m_Components[CompIndex].m_bState;
}

void CMissionData::UnlatchObjective( int ObjIndex )
{
	if( ObjIndex >= m_Objectives.Num() || ObjIndex < 0 )
	{
		DM_LOG(LC_OBJECTIVES,LT_WARNING)LOGSTRING("UnlatchObjective: Bad objective index: %d \r", ObjIndex );
		gameLocal.Printf("WARNING: Objective system: Attempt was made to unlatch an invalid objective index: %d \n", ObjIndex);
		return;
	}

	m_Objectives[ObjIndex].m_bLatched = false;
}

void CMissionData::UnlatchObjectiveComp(int ObjIndex, int CompIndex )
{
	if( ObjIndex >= m_Objectives.Num() || ObjIndex < 0 )
	{
		DM_LOG(LC_OBJECTIVES,LT_WARNING)LOGSTRING("UnlatchObjective: Bad objective index: %d \r", ObjIndex );
		gameLocal.Printf("WARNING: Objective system: Attempt was made to unlatch a component of invalid objective index: %d \n", ObjIndex);
		return;
	}

	if( CompIndex >= m_Objectives[ObjIndex].m_Components.Num() || CompIndex < 0 )
	{
		DM_LOG(LC_OBJECTIVES,LT_WARNING)LOGSTRING("UnlatchObjective: Component num %d out of bounds for objective %d. \r", (CompIndex+1), (ObjIndex+1) );
		gameLocal.Printf("WARNING: Objective system: Attempt was made to unlatch invalid component: %d of objective: %d \n", (CompIndex+1), (ObjIndex+1) );
		return;
	}

	m_Objectives[ObjIndex].m_Components[CompIndex].m_bLatched = false;
}

bool CMissionData::GetObjectiveVisibility( int ObjIndex )
{
	if (ObjIndex >= m_Objectives.Num() || ObjIndex < 0)
	{
		DM_LOG(LC_OBJECTIVES, LT_WARNING)LOGSTRING("GetCompletionState: Bad objective index: %d \r", ObjIndex);
		gameLocal.Printf("WARNING: Objective system: Attempt was made to get visibility of invalid objective index: %d \n", ObjIndex);
		return false;
	}

	return m_Objectives[ObjIndex].m_bVisible;
}

void CMissionData::SetObjectiveVisibility(int objIndex, bool visible, bool fireEvents)
{
	if (objIndex >= m_Objectives.Num() || objIndex < 0)
	{
		DM_LOG(LC_OBJECTIVES, LT_ERROR)LOGSTRING("SetObjectiveVisibility: Invalid objective index: %d\r", objIndex);
		return;
	}

	CObjective& obj = m_Objectives[objIndex];

	bool wasVisible = obj.m_bVisible;

	// Set the new state
	obj.m_bVisible = visible;

	// greebo: If we show a previously hidden objective, notify the player
	// Only do this for applicable objectives
	if (fireEvents && visible && !wasVisible && obj.m_bApplies)
	{
		Event_NewObjective(); 
	}
}

void CMissionData::SetObjectiveMandatory(int objIndex, bool mandatory)
{
	if (objIndex >= m_Objectives.Num() || objIndex < 0)
	{
		DM_LOG(LC_OBJECTIVES, LT_ERROR)LOGSTRING("SetObjectiveMandatory: Invalid objective index: %d\r", objIndex);
		return;
	}

	m_Objectives[objIndex].m_bMandatory = mandatory;
}

void CMissionData::SetObjectiveOngoing(int objIndex, bool ongoing)
{
	if (objIndex >= m_Objectives.Num() || objIndex < 0)
	{
		DM_LOG(LC_OBJECTIVES, LT_ERROR)LOGSTRING("SetObjectiveOngoing: Invalid objective index: %d\r", objIndex);
		return;
	}

	m_Objectives[objIndex].m_bOngoing = ongoing;
}

void CMissionData::SetObjectiveText(int objIndex, const char *descr)
{
	if (objIndex >= m_Objectives.Num() || objIndex < 0)
	{
		DM_LOG(LC_OBJECTIVES, LT_ERROR)LOGSTRING("SetObjectiveText: Invalid objective index: %d\r", objIndex);
		return;
	}
	m_Objectives[objIndex].m_text = descr;
	// make the GUI update itself
	idPlayer* player = gameLocal.GetLocalPlayer();
	if (player == NULL)
	{
		gameLocal.Error("No player at SetObjectiveText!\n");
	}
	player->UpdateObjectivesGUI();
}

void CMissionData::SetEnablingObjectives(int objIndex, const idStr& enablingStr)
{
	if (objIndex >= m_Objectives.Num() || objIndex < 0)
	{
		DM_LOG(LC_OBJECTIVES, LT_ERROR)LOGSTRING("SetEnablingObjectives: Invalid objective index: %d\r", objIndex);
		return;
	}

	// parse in the int list of "enabling objectives"
	idLexer src;
	src.LoadMemory(enablingStr.c_str(), enablingStr.Length(), "");

	idToken token;
	idList<int>& list = m_Objectives[objIndex].m_EnablingObjs;

	list.Clear();

	while (src.ReadToken(&token))
	{
		if (token.IsNumeric())
		{
			list.Append(token.GetIntValue());
		}
	}

	src.FreeSource();
}

// Objective parsing:
// returns the index of the first objective added, for scripting purposes
int CMissionData::AddObjsFromEnt( idEntity *ent )
{
	if( !ent )
	{
		return m_Objectives.Num();
	}

	// greebo: pass the call further on
	return AddObjsFromDict(ent->spawnArgs);
}

// Objective parsing:
// returns the index of the first objective added, for scripting purposes
int CMissionData::AddObjsFromDict(const idDict& dict)
{
	CObjective			ObjTemp;
	idLexer				src;
	idToken				token;
	idStr				StrTemp, StrTemp2, TempStr2;
	int					Counter(1), Counter2(1); // objective indices start at 1 and must be offset for the inner code
	bool				bLogicMod(false); // modified mission logic

	// store the first index of first added objective
	int ReturnVal = m_Objectives.Num();

	// go thru all the objective-related spawnargs
	while( dict.MatchPrefix( va("obj%d_", Counter) ) != NULL )
	{
		ObjTemp.m_Components.ClearFree();
		ObjTemp.m_ObjNum = Counter - 1;

		StrTemp = va("obj%d_", Counter);
		ObjTemp.m_state = (EObjCompletionState) dict.GetInt( StrTemp + "state", "0");
		ObjTemp.m_text = dict.GetString( StrTemp + "desc", "" );
		ObjTemp.m_bMandatory = dict.GetBool( StrTemp + "mandatory", "1");
		ObjTemp.m_bReversible = !dict.GetBool (StrTemp + "irreversible", "0" );
		ObjTemp.m_bVisible = dict.GetBool( StrTemp + "visible", "1");
		ObjTemp.m_bOngoing = dict.GetBool( StrTemp + "ongoing", "0");
		ObjTemp.m_CompletionScript = dict.GetString( StrTemp + "script_complete" );
		ObjTemp.m_FailureScript = dict.GetString( StrTemp + "script_failed" );
		ObjTemp.m_CompletionTarget = dict.GetString( StrTemp + "target_complete" );
		ObjTemp.m_FailureTarget = dict.GetString( StrTemp + "target_failed" );
		ObjTemp.m_SuccessLogicStr = dict.GetString( StrTemp + "logic_success", "" );
		ObjTemp.m_FailureLogicStr = dict.GetString( StrTemp + "logic_failure", "" );

		// parse in the int list of "enabling objectives"
		TempStr2 = dict.GetString( StrTemp + "enabling_objs", "" );
		src.LoadMemory( TempStr2, TempStr2.Length(), "" );
		while( src.ReadToken( &token ) )
		{
			if( token.IsNumeric() )
				ObjTemp.m_EnablingObjs.Append( token.GetIntValue() );
		}
		src.FreeSource();

		// Parse difficulty level. If difficulty not specified, then
		// this objective applies to all levels.
		TempStr2 = dict.GetString( StrTemp + "difficulty", "" );
		if (!TempStr2.IsEmpty())
		{
			ObjTemp.m_bApplies = false;
			src.LoadMemory( TempStr2, TempStr2.Length(), "" );

			while( src.ReadToken( &token ) )
			{
				if (token.IsNumeric() && 
					gameLocal.m_DifficultyManager.GetDifficultyLevel() == token.GetIntValue())
				{
					ObjTemp.m_bApplies = true;
					break;
				}
			}

			if (!ObjTemp.m_bApplies)
			{
				// Objectives that don't apply to this difficulty level are considered invalid.
				// They don't need to be completed.
				ObjTemp.m_state = STATE_INVALID;
				// greebo: Also set them to invisible so that they aren't displayed on the GUI.
				ObjTemp.m_bVisible = false;
			}

			src.FreeSource();
		}

		// parse objective components
		Counter2 = 1;
		while( dict.MatchPrefix( va("obj%d_%d_", Counter, Counter2) ) != NULL )
		{
			StrTemp2 = StrTemp + va("%d_", Counter2);
			CObjectiveComponent CompTemp;

			CompTemp.m_bState = dict.GetBool( StrTemp2 + "state", "0" );
			CompTemp.m_bPlayerResponsibleOnly = dict.GetBool( StrTemp2 + "player_responsible", "1" );
			CompTemp.m_bNotted = dict.GetBool( StrTemp2 + "not", "0" );
			CompTemp.m_bReversible = !dict.GetBool( StrTemp2 + "irreversible", "0" );

			// use comp. type hash to convert text type to EComponentType
			idStr TypeString = dict.GetString( StrTemp2 + "type", "");
			int TypeNum = m_CompTypeHash.First(m_CompTypeHash.GenerateKey( TypeString, false ));
			DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("Parsing objective component type '%s', typenum %d \r", TypeString.c_str(), TypeNum );

			if( TypeNum == -1 )
			{
				DM_LOG(LC_OBJECTIVES,LT_ERROR)LOGSTRING("Unknown objective component type '%s' when adding objective %d, component %d \r", TypeString.c_str(), Counter, Counter2 );
				gameLocal.Printf("Objective System Error: Unknown objective component type '%s' when adding objective %d, component %d.  Objective component ignored. \n", TypeString.c_str(), Counter, Counter2 );
				continue;
			}
			CompTemp.m_Type = (EComponentType) TypeNum;

			for( int ind=0; ind<2; ind++ )
			{
				// Use spec. type hash to convert text specifier to ESpecificationMethod enum
				idStr SpecString = dict.GetString(va(StrTemp2 + "spec%d", ind + 1), "none");
				int SpecNum = m_SpecTypeHash.First(m_SpecTypeHash.GenerateKey( SpecString, false ));

				if( SpecNum == -1 )
				{
					DM_LOG(LC_OBJECTIVES,LT_ERROR)LOGSTRING("Unknown objective component specification type '%s' when adding objective %d, component %d \r", TypeString.c_str(), Counter, Counter2 );
					gameLocal.Printf("Objective System Error: Unknown objective component specification type '%s' when adding objective %d, component %d.  Setting default specifier type 'none' \n", TypeString.c_str(), Counter, Counter2 );
					SpecNum = 0;
				}
				CompTemp.m_SpecMethod[ind] = (ESpecificationMethod) SpecNum;
			}

			for( int ind=0; ind < 2; ind++ )
			{
				CompTemp.m_SpecVal[ind] = dict.GetString( va(StrTemp2 + "spec_val%d", ind + 1), "" );
			}

			// Use idLexer to read in args, a space-delimited string list
			TempStr2 = dict.GetString( StrTemp2 + "args", "" );
			src.LoadMemory( TempStr2.c_str(), TempStr2.Length(), "" );
			src.SetFlags( LEXFL_NOSTRINGCONCAT | LEXFL_NOFATALERRORS | LEXFL_ALLOWPATHNAMES );

			while( src.ReadToken( &token ) )
				CompTemp.m_Args.Append( token.c_str() );
			src.FreeSource();

			// Pad args with dummies to prevent a hard crash when they are read, if otherwise empty
			CompTemp.m_Args.Append("");
			CompTemp.m_Args.Append("");

			CompTemp.m_ClockInterval = SEC2MS(dict.GetFloat(StrTemp2 + "clock_interval", "1"));

			CompTemp.m_Index[0] = Counter;
			CompTemp.m_Index[1] = Counter2;

			ObjTemp.m_Components.Append( CompTemp );
			Counter2++;
		}

		if( ObjTemp.m_Components.Num() > 0 )
		{
			m_Objectives.Append( ObjTemp );
			
			// Parse success/failure logic
			gameLocal.Printf("Objective %d: Parsing success and failure logic\n", Counter);
			m_Objectives[ m_Objectives.Num() - 1 ].ParseLogicStrs();

			ObjTemp.Clear();
		}
		Counter++;
	}

	// Process the objectives and add clocked components to clocked components list
	for( int ind = 0; ind < m_Objectives.Num(); ind++ )
	{
		for( int ind2 = 0; ind2 < m_Objectives[ind].m_Components.Num(); ind2++ )
		{
			CObjectiveComponent& comp = m_Objectives[ind].m_Components[ind2];

			if (comp.m_Type == COMP_CUSTOM_CLOCKED || comp.m_Type == COMP_DISTANCE || comp.m_Type == COMP_INFO_LOCATION)
			{
				m_ClockedComponents.Append( &comp );
			}
		}
	}

	// parse overall mission logic (for specific difficulty if applicable)
	idStr DiffStr = va("_diff_%d", gameLocal.m_DifficultyManager.GetDifficultyLevel() );
	StrTemp = "mission_logic_success";
	if( dict.FindKey( StrTemp + DiffStr ) )
		StrTemp = StrTemp + DiffStr;
	// Only one of these per mission, so empty args on this object should not overwrite existing args
	StrTemp = dict.GetString(StrTemp, "");
	if( StrTemp != "" )
	{
		bLogicMod = true;
		m_SuccessLogicStr = StrTemp;
	}

	StrTemp = "mission_logic_failure";
	if( dict.FindKey( StrTemp + DiffStr ) )
		StrTemp = StrTemp + DiffStr;
	StrTemp = dict.GetString(StrTemp, "");
	if( StrTemp != "" )
	{
		bLogicMod = true;
		m_FailureLogicStr = StrTemp;
	}
	
	if( bLogicMod )
		ParseLogicStrs();

	// greebo: Parse spawnargs for conditional objectives
	ParseObjectiveConditions(dict);

	// check if any objectives were actually added, if not return -1
	if( m_Objectives.Num() == ReturnVal )
		ReturnVal = -1;

	return ReturnVal;
}

void CMissionData::ParseObjectiveConditions(const idDict& dict)
{
	int index = 1;
	
	while (true)
	{
		DM_LOG(LC_OBJECTIVES, LT_INFO)LOGSTRING("Trying to parse objective condition with index %d.\r", index);

		// Try to parse a condition from the given spawnargs
		ObjectiveCondition cond(dict, index);

		if (!cond.IsValid())
		{
			DM_LOG(LC_OBJECTIVES, LT_INFO)LOGSTRING("Condition with index %d failed to parse, stopping.\r", index);
			break;
		}

		DM_LOG(LC_OBJECTIVES, LT_INFO)LOGSTRING("Objective condition with index %d successfully parsed.\r", index);

		// We have a valid objective condition, apply it right away
		cond.Apply(*this);

		index++;
	}

	gameLocal.Printf("Applied %d objective conditions.\n", index - 1);
}

bool    CMissionData::MatchLocationObjectives( idEntity * entity )
{
    if ( !entity )
        return false;
    
    SObjEntParms    entParms;
    
    //  iterate over all components of the objectives and test the COMP_LOCATION components against the entity.
    //  returns on the first match.
    for ( int i = 0; i < m_Objectives.Num(); i++ )
    {
        CObjective  & currentObjective = m_Objectives[ i ];
        for ( int j = 0; j < currentObjective.m_Components.Num(); j++ )
        {
            CObjectiveComponent & currentComponent = currentObjective.m_Components[ j ];
            if ( currentComponent.m_Type != COMP_LOCATION )
            {
                continue;
            }
            
            entParms.Clear();
            FillParmsData( entity, &entParms );
            if ( MatchSpec( &currentComponent, &entParms, 0 ) )
            {
                return true;
            }
        }
    }
    
    return false;
}

idMapFile* CMissionData::LoadMap(const idStr& fn)
{
	//stgatilov: idMapFile::GetName returns filename without extension
	//if we don't strip it here, then "mapFileName == m_mapFile->GetName()" won't work
	idStr mapFileName = fn;
	mapFileName.StripFileExtension();

	// First, check if we already have a map loaded
	if (m_mapFile != NULL)
	{
		if (mapFileName == m_mapFile->GetName() && !m_mapFile->NeedsReload())
		{
			// Nothing to do, we already have an up-to-date map loaded
			return m_mapFile;
		}

		// Map was different, discard this one and load afresh
		delete m_mapFile;
		m_mapFile = NULL;
	}

	// Map file is NULL at this point, load from disk
	m_mapFile = new idMapFile;

	if (!m_mapFile->Parse(mapFileName))
	{
		delete m_mapFile;
		m_mapFile = NULL;

		gameLocal.Warning( "Couldn't load %s", mapFileName.c_str());
		return NULL;
	}

	return m_mapFile;
}

void CMissionData::LoadDirectlyFromMapFile(idMapFile* mapFile) 
{
	// greebo: get the worldspawn entity
	idMapEntity* worldspawn = mapFile->GetEntity(0);
	idDict worldspawnDict = worldspawn->epairs;

	// Now go and look for suitable objective entites
	for (int i = 0; i < mapFile->GetNumEntities(); i++)
	{
		idMapEntity* mapEnt = mapFile->GetEntity(i);
		idDict& mapDict = mapEnt->epairs;

		idStr classname = mapDict.GetString("classname");

		if (classname != "target_tdm_addobjectives" && classname != "atdm:target_addobjectives")
		{
			continue; // not the right entity
		}

		// Let's see if this entity has to be triggered
		if (!mapDict.GetBool("wait_for_trigger", "0"))
		{
			// Doesn't need trigger, take it immediately
			AddObjsFromDict(mapDict);
		}
		else 
		{
			// Entity is waiting for trigger, is it triggered by worldspawn?
			const idKeyValue* target = worldspawnDict.MatchPrefix("target");
			while (target != NULL)
			{
				if (target->GetValue() == mapDict.GetString("name"))
				{
					// Worldspawn triggers this entity, consider this
					AddObjsFromDict(mapDict);
				}

				// Next key
				target = worldspawnDict.MatchPrefix("target", target);
			}
		}
	}
}

void CMissionData::InventoryCallback(idEntity *ent, idStr ItemName, int value, int OverallVal, bool bPickedUp)
{
	SObjEntParms Parms;

	if (ent)
	{
		FillParmsData( ent, &Parms );
	}

	Parms.group = ItemName;
	Parms.value = value;
	Parms.valueSuperGroup = OverallVal;

	MissionEvent( COMP_ITEM, &Parms, bPickedUp );

	DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("Inventory Callback: Overall loot value %d\r", OverallVal );
	
	// Also call the pickocket event if stolen from living AI
	if ( bPickedUp && ( ent != NULL ) && ent->GetBindMaster() )
	{
		idEntity *bm = ent->GetBindMaster();
		if ( bm->IsType( idAI::Type ) &&
			 ( bm->health > 0 ) &&
			 !static_cast<idAI *>(bm)->IsKnockedOut() )
		{
			// Player is always responsible for a pickpocket
			MissionEvent( COMP_PICKPOCKET, &Parms, true );

			// grayman #3559 - The victim should react.
			// grayman #3738 - but not to all objects
			if (ent->spawnArgs.GetBool("notice_when_stolen","1"))
			{
				static_cast<idAI *>(bm)->PocketPicked(); // react to losing something
			}
		}
	}
}

void CMissionData::AlertCallback(idEntity *Alerted, idEntity *Alerter, int AlertVal)
{
	SObjEntParms Parms1, Parms2;
	bool bPlayerResponsible(false);

	if( Alerted )
	{
		FillParmsData( Alerted, &Parms1 );
		// The alert value is stored in the alerted entity data packet
		Parms1.value = AlertVal;
	}

	if( Alerter )
	{
		FillParmsData( Alerter, &Parms2 );

		if( Alerter == gameLocal.GetLocalPlayer() )
			bPlayerResponsible = true;
	}

	MissionEvent( COMP_ALERT, &Parms1, &Parms2, bPlayerResponsible );
}

int CMissionData::GetFoundLoot()
{
	return m_Stats.GetFoundLootValue();
}

// SteveL #3304: Adding Zbyl's new accessors
int CMissionData::GetMissionLoot()
{
	return m_Stats.GetTotalLootInMission();
}

int CMissionData::GetSecretsFound()
{
	return m_Stats.secretsFound;
}

int CMissionData::GetSecretsTotal()
{
	return m_Stats.secretsTotal;
}

int CMissionData::GetTotalTimePlayerSeen()
{
	return m_Stats.totalTimePlayerSeen;
}

int CMissionData::GetNumberTimesPlayerSeen()
{
	return m_Stats.numberTimesPlayerSeen;
}

int CMissionData::GetNumberTimesAISuspicious()
{
	return m_Stats.AIAlerts[ai::EObservant].Overall + m_Stats.AIAlerts[ai::ESuspicious].Overall;
}

int CMissionData::GetNumberTimesAISearched()
{
	return m_Stats.AIAlerts[ai::ESearching].Overall + m_Stats.AIAlerts[ai::EAgitatedSearching].Overall;
}

// SteveL #3304: Zbyl's patch. Moving this logic from UpdateStatisticsGUI() so it can be reused for new scriptevents
float CMissionData::GetSightingScore()
{
	int busted = GetNumberTimesPlayerSeen();	// how many times AI saw the player
	float sightingScore = 5*busted; // alertLevel = 5
	return sightingScore;
}

// SteveL #3304: ditto
float CMissionData::GetStealthScore()
{
	float stealthScore = 0;

	for (int i = ai::ESuspicious; i < ai::ECombat; i++) // Adds up alerts from levels 2-4; if you want all alerts, use "1 < ai::EAlertStateNum"
	{
		/*key = idStr("AI alerted to level '") + ai::AlertStateNames[i] + "'";
		value = idStr(m_Stats.MaxAlertIndices[i]);
		gui->SetStateString(prefix + idStr(index++), key + divider + value);*/

		// Increase the stealth factor based on the number of alerted AI (m_Stats.AIAlerts[i].Overall) weighted with the seriousness
		stealthScore += ( i - 1 ) * m_Stats.AIAlerts[i].Overall;
	}

	float sightingScore = GetSightingScore();
	stealthScore += sightingScore;

	return stealthScore;
}

void CMissionData::ChangeFoundLoot(LootType lootType, int amount)
{
	m_Stats.FoundLoot[lootType] += amount;
}

void CMissionData::AddMissionLoot(LootType lootType, int amount)
{
	// greebo: add to the individual sum
	m_Stats.LootInMission[lootType] += amount;
}

// grayman #2887

void CMissionData::IncrementPlayerSeen()
{
	m_Stats.numberTimesPlayerSeen++;
}

void CMissionData::Add2TimePlayerSeen( int amount )
{
	m_Stats.totalTimePlayerSeen += amount;
}

/**
* Parse the boolean logic strings into matrices.
* Returns false if there was an error in the parsing.
**/
bool CMissionData::ParseLogicStrs( void )
{
	bool bReturnVal(true), bTemp(false);

	if( m_SuccessLogicStr != "" )
	{
		bReturnVal = ParseLogicStr( &m_SuccessLogicStr, &m_SuccessLogic );
		
		if( !bReturnVal )
			gameLocal.Error("Mission success logic failed to parse \n");
	}

	if( m_FailureLogicStr != "" )
	{
		bTemp = ParseLogicStr( &m_FailureLogicStr, &m_FailureLogic );
		
		if( !bTemp )
			gameLocal.Error("Mission failure logic failed to parse \n");

		bReturnVal = bReturnVal && bTemp;
	}

	return bReturnVal;
}


/**
* Parse a string into a logic matrix.
* Returns false if there was an error in the parsing
**/
bool CMissionData::ParseLogicStr( idStr *input, SBoolParseNode *output )
{
	idLexer		src;
	idToken		token;
	int			col(0), row(0), level(0);

	bool		bReturnVal( false );
	bool		bFollowingOperator( false ); // whether we expect an identifier or open parenthesis
	bool		bOperatorOK( false ); // can the next token be an operator
	bool		bNOTed( false ); // next parse node will be NOTted
	// initialize as advancing to 0,0 at start of parsing
	bool		bRowAdvanced( true );
	bool		bColAdvanced( true );

	SBoolParseNode *CurrentNode( NULL );


	// set up outer node:
	output->Clear();
	CurrentNode = output;

	DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] Parsing string: %s \r", input->c_str() );
	DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("Outer parse node is %08lx \r", CurrentNode);

	src.LoadMemory( input->c_str(), input->Length(), "" );
	DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] Loaded memory to lexer \r" );

	while( src.ReadToken( &token ))
	{
		DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] Parsing token: %s At level: %d \r", token.c_str(), level );
		if( level < 0 )
		{
			gameLocal.Printf("[Objective Logic] ERROR: Unbalanced parenthesis, found unexpected \")\" \n"); 
			DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] ERROR: Unbalanced parenthesis, found unexpected \")\" \r");
			goto Quit;
		}

		// New parse node (identifier or parenthesis)
		if( token.IsNumeric() || (token.Cmp( "(" ) == 0) )
		{
			DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] New parse node ( identifier or \"(\" ) \r" );
			bFollowingOperator = false;

			SBoolParseNode NewNode;
			NewNode.bNotted = bNOTed;
			NewNode.PrevRow = row;
			NewNode.PrevCol = col;
			NewNode.PrevNode = CurrentNode;
			DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("Parse: New node at %d, %d, points at previous node: %08lx \r", row, col, CurrentNode);

			if( token.IsNumeric() )
			{
				// Node is a leaf: set Ident to the identifier
				NewNode.Ident = token.GetIntValue() - 1;
			}

			// Add node to the appropriate point in the matrix-tree - same for leaves and branches
			DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] Adding new node to matrix-tree \r" );
			if( bColAdvanced )
			{
				idList< SBoolParseNode > NewCol;
				NewCol.Append( NewNode );
				CurrentNode->Cols.Append( NewCol );
			}
			else if( bRowAdvanced )
			{
				CurrentNode->Cols[ col ].Append( NewNode );
			}

			// If neither row nor column advanced, we have a problem, such as two leaves in a row
			else
			{
				gameLocal.Printf("[Objective Logic] ERROR: Unexpected identifier found \n");
				DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] ERROR: Unexpected identifier found \r");
				goto Quit;
			}

			// Node is a branch, step in
			if( token.Cmp( "(" ) == 0 )
			{
				level++;
				bOperatorOK = false;

				CurrentNode = &CurrentNode->Cols[ col ].operator[]( row );
				row = 0;
				col = 0;
				// new level expects these to be true
				bRowAdvanced = true;
				bColAdvanced = true;
			}
			// node is a leaf, keep going on same level
			else
			{
				bOperatorOK = true;
				bRowAdvanced = false;
				bColAdvanced = false;
			}

			bNOTed = false;
		} // New Parse Node

		else if( token.Icmp( "AND" ) == 0 )
		{
			DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] Parsing AND operator \r" );

			if( bFollowingOperator || !bOperatorOK )
			{
				gameLocal.Printf("[Objective Logic] ERROR: Found unexpected operator AND \n");
				DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] ERROR: Found unexpected operator AND \r");
				goto Quit;
			}
			bFollowingOperator = true;
			bOperatorOK = false;

			col++;
			bColAdvanced = true;
		} // AND

		else if( token.Icmp( "OR" ) == 0 )
		{
			DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] Parsing OR operator \r" );

			if( bFollowingOperator || !bOperatorOK )
			{
				gameLocal.Printf("[Objective Logic] ERROR: Found unexpected operator OR \n");
				DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] ERROR: Found unexpected operator OR \r");
				goto Quit;
			}

			bFollowingOperator = true;
			bOperatorOK = false;

			row++;
			bRowAdvanced = true;
		} // OR

		else if( token.Icmp( "NOT" ) == 0 )
		{
			DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] Parsing NOT operator \r" );
			bNOTed = true;
			bOperatorOK = false;
		} // NOT

		else if( token.Cmp( ")" ) == 0 )
		{
			DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] Parsing \")\" \r" );
			if( bFollowingOperator )
			{
				gameLocal.Printf("[Objective Logic] ERROR: Identifier expected, found \")\" \n");
				DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] ERROR: Identifier expected, found \")\" \r");
				goto Quit;
			}

			// If level is empty, report error
			if( CurrentNode->Cols.Num() == 0 )
			{
				gameLocal.Printf("[Objective Logic] ERROR: Identifier expected, found \")\" \n");
				DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] ERROR: Identifier expected, found \")\" \r");
				goto Quit;
			}
		
			// step out
			level--;

			row = CurrentNode->PrevRow;
			col = CurrentNode->PrevRow;
			CurrentNode = CurrentNode->PrevNode;
		} // Step Out of Node

		else
		{
			gameLocal.Printf("[Objective Logic] ERROR: Unrecognized token: %s \n", token.c_str() ); 
			DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] ERROR: Unrecognized token: %s \r", token.c_str() ); 
			goto Quit;
		}
	}

	// Finished parsing
	DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] Reached EOF \r");

	if( level != 0 )
	{
		gameLocal.Printf("[Objective Logic] ERROR: Unbalanced parenthesis, expected \")\" not found \n");
		DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] ERROR: Unbalanced parenthesis, expected \")\" not found \r");
		goto Quit;
	}

	if( bFollowingOperator )
	{
		gameLocal.Printf("[Objective Logic] ERROR: Expected identifier, found EOF \n");
		DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] ERROR: Expected identifier, found EOF \n");
		goto Quit;
	}

	// Successfully parsed
	DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] Successfully parsed \r");
	bReturnVal = true;

Quit:

	src.FreeSource();
	return bReturnVal;
}

bool CMissionData::EvalBoolLogic( SBoolParseNode *StartNode, bool bObjComp, int ObjNum )
{

	int level(0); // current level of branching
	bool bReturnVal(false);
	bool bResolvedLevel(false); // have we completely evaluated the lower level in the previous pass of the loop?
	bool bLowerLevResult(false); // the result of evaluating the lower level in the prev. pass of loop
	SBoolParseNode *CurrentNode = NULL;

	int CurrentCol(0), CurrentRow(0); // matrix coordinates in the current level


	if(!StartNode)
	{
		// Log error
		goto Quit;
	}

	CurrentNode = StartNode;


/* PSUEDOCODE:
Will always do 1 of 3 things:
1. Back up a level because we hit a leaf
2. Advance the matrix and back up a level because the matrix is "done"
3. Advance the matrix and go down a level because the matrix is not yet done

Going down one level can only happen because we finished an evaluation in
	the previous step, and we now want to advance to the next matrix spot and
	go down a level at the node at that next matrix spot

When we advance to the next matrix spot, it can happen in two ways:
1. We have evaluated TRUE in the previous step

	If there is a next column, go to the first row of that next column and go down a level

	If there is no next column, we are done with this level, eval it as TRUE and go up

2. We have evaluated FALSE in the previous step
	If there is another row, go to that next column and go down a level

	If there is no next row, we are done with this level, eval it as FALSE and go up
*/

	while( level >= 0 && (CurrentNode != NULL) )
	{
		DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] Evaluating: level %d, row %d, column %d\r", level, CurrentRow, CurrentCol);
		
		// check if the node on this level contains a matrix (branch)
		// If it does not, it must be directly addressing a component (leaf)
		if( CurrentNode->Cols.Num() <= 0 )
		{
			// Leaf found, evaluate and go up a level
			DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] Evaluating leaf with identifier %d\r", CurrentNode->Ident);
			
			if( bObjComp )
			{
				int state = GetCompletionState( CurrentNode->Ident );
				bLowerLevResult = (state == STATE_COMPLETE);
			}
			else
				bLowerLevResult = GetComponentState( ObjNum, CurrentNode->Ident );

			if( CurrentNode->bNotted )
				bLowerLevResult = !bLowerLevResult;

			bResolvedLevel = true;

			CurrentCol = CurrentNode->PrevCol;
			CurrentRow = CurrentNode->PrevRow;
			DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("PrevNode in leaf at %d, %d, points at previous node: %08lx \r", CurrentRow, CurrentCol, CurrentNode->PrevNode);
			CurrentNode = CurrentNode->PrevNode;

			level--;
			DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] Leaf evaluated, stepping out \r");
			continue;
		}

		// if we have just backed up a level, advance the matrix appropriately

		// If we evaluate TRUE in the lower level:
		if( bResolvedLevel && bLowerLevResult )
		{
			DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] Resolved previous level as TRUE\r");
			// if there is no next column, this level evals to TRUE due to AND logic success
			if( CurrentCol + 1 >= CurrentNode->Cols.Num() )
			{
				DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] AND logic success at current level, stepping out \r");
				bLowerLevResult = true;

				CurrentCol = CurrentNode->PrevCol;
				CurrentRow = CurrentNode->PrevRow;
				CurrentNode = CurrentNode->PrevNode;

				level--;
				continue;
			}
			// else, advance to next column and go down a level
			else
			{
				DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] Continuing on in current level, to next column \r");
				CurrentCol++;
				CurrentRow = 0;
			}
		}
		// If we came back up after we evaluated to FALSE in the lower level
		if( bResolvedLevel && !bLowerLevResult )
		{
			DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] Resolved previous level as FALSE\r");

			// If there are no more rows in this column, evaluate this level to FALSE (Due to AND logic failure)
			if( CurrentRow + 1 >= CurrentNode->Cols.operator[](CurrentCol).Num() )
			{
				DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] Current level failed (no more rows to OR), stepping out\r");
				bLowerLevResult = false;

				CurrentCol = CurrentNode->PrevCol;
				CurrentRow = CurrentNode->PrevRow;
				CurrentNode = CurrentNode->PrevNode;

				level--;
				continue;
			}
			// else, advance to the next row
			{
				DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] Continuing on in current level, to next row\r");
				CurrentRow++;
			}
		}

		DM_LOG(LC_OBJECTIVES,LT_DEBUG)LOGSTRING("[Objective Logic] Going down a level\r");
		// If we get to this point in the loop, we must be going down a level
		bResolvedLevel = false;
		CurrentNode = &CurrentNode->Cols[CurrentCol].operator[](CurrentRow);

		level++;
	}

	bReturnVal = bLowerLevResult;

Quit:
	return bReturnVal;
}

void CMissionData::UpdateGUIState(idUserInterface* ui) 
{
	// Update UI element positioning
	if (ui->GetStateBool("ingame"))
	{
		// Let the GUI adjust its position for in-game setup
		ui->HandleNamedEvent("ObjMenuHideDifficultyOptions");
	}
	else
	{
		// Let the GUI adjust its position for pre-game setup
		ui->HandleNamedEvent("ObjMenuShowDifficultyOptions");
	}

	// The list of indices of visible, applicable objectives
	idList<int> objIndices;

	for (int i = 0; i < m_Objectives.Num(); i++) 
	{
		CObjective& obj = m_Objectives[i];

		// Don't consider invisible, inapplicable objectives
		if (obj.m_bVisible && obj.m_bApplies)
		{
			objIndices.Append(i);
		}
	}

	ui->SetStateInt("NumVisibleObjectives", objIndices.Num());
	ui->SetStateInt("ObjectiveBoxIsVisible", 1);
	ui->SetStateFloat("objectiveTextSize", cv_gui_objectiveTextSize.GetFloat());

	for (int i = 0; i < objIndices.Num(); i++)
	{
		int index = objIndices[i];

		// greebo: GUI objective numbers are starting with 1
		int guiObjNum = i + 1;

		idStr prefix = va("obj%d", guiObjNum);

		// Get a shortcut to the target objective
		CObjective& obj = m_Objectives[index];

		// Set the text (in translated form if the original is "#str_xxxxx")
		ui->SetStateString(prefix + "_text", common->Translate( obj.m_text ));

		// Set the state, this requires some logic
		EObjCompletionState state = obj.m_state;

		// State is not complete for ongoing objectives
		if (obj.m_state == STATE_COMPLETE && obj.m_bOngoing)
		{
			state = STATE_INCOMPLETE;
		}

		// Write the state to the GUI
		ui->SetStateInt(prefix + "_state", static_cast<int>(state));

		// Call UpdateObjectiveStateN to perform some GUI-specific updates
		ui->HandleNamedEvent(va("UpdateObjective%d", guiObjNum));
	}
}

void CMissionData::HandleMainMenuCommands(const idStr& cmd, idUserInterface* gui)
{
   
	if (cmd == "mainmenu_heartbeat")
	{
		// The main menu is visible, check if we should display the "Objectives" option
		if (!m_MissionDataLoadedIntoGUI)
		{
			// Load the objectives into the GUI
			UpdateGUIState(gui);
		}

		m_MissionDataLoadedIntoGUI = true;
	}
	else if (cmd == "loadstatistics")
	{
		// Load the statistics into the GUI
		UpdateStatisticsGUI(gui, "listStatistics");
	}
	else if (cmd == "objective_open_request")
	{
		gui->HandleNamedEvent("GetObjectivesInfo");

		if (!gui->GetStateBool("ingame") && gameLocal.m_MissionResult != MISSION_COMPLETE )
		{
			// We're coming from the start screen
			// Clear the objectives data and load them from the map
			Clear();

			// Get the starting map file name
			idStr startingMapfilename = va("maps/%s", gameLocal.m_MissionManager->GetCurrentStartingMap().c_str());

			// Ensure that the map is loaded
			idMapFile* map = LoadMap(startingMapfilename);

			if (map == NULL)
			{
				gameLocal.Error("Couldn't load map %s", startingMapfilename.c_str());
			}

			// Load the objectives from the map
			LoadDirectlyFromMapFile(m_mapFile);

			// Determine the difficulty level strings. The defaults are the "difficultyMenu" entityDef.
			// Maps can override these values by use of the difficulty#Name value on the spawnargs of 
			// the worldspawn.
			const idDecl* diffDecl = declManager->FindType(DECL_ENTITYDEF, "difficultyMenu", false);
	
			if (diffDecl != NULL)
			{
				const idDeclEntityDef *diffDef = static_cast<const idDeclEntityDef *>( diffDecl );
				idMapEntity* worldspawn = m_mapFile->GetEntity(0);

				const idDict& worldspawnDict = worldspawn->epairs;

				for (int diffLevel = 0; diffLevel < DIFFICULTY_COUNT; diffLevel++)
				{
					// Tels: #3411 - translate common names like "Easy" back to "#str_12345":
					const char* diffName = common->GetI18N()->TemplateFromEnglish(
						worldspawnDict.GetString(va("difficulty%dName",diffLevel),
						diffDef->dict.GetString(va("diff%ddefault",diffLevel), ""))
					);
					// Tels: Make sure we translate the name for the GUI
					gui->SetStateString(va("diff%dName",diffLevel), common->Translate( diffName) );
				}

				gui->SetStateBool("SkipShop", worldspawnDict.GetBool("shop_skip", "0"));

				// Let the GUI know what the current difficulty level is
				gui->SetStateInt("diffSelect", gameLocal.m_DifficultyManager.GetDifficultyLevel());

				// Clear the flag so that the objectives get updated
				ClearGUIState();
			}
			else
			{
				// This is critical, throw an error
				gameLocal.Error("Could not find difficulty entityDef %s", "difficultyMenu");
			}			
		}

		if (!m_MissionDataLoadedIntoGUI)
		{
			// Load the objectives into the GUI
			UpdateGUIState(gui); 
		}

		m_MissionDataLoadedIntoGUI = true;
	}
	else if (cmd == "objective_scroll_down_request") 
	{
		// Increment the start index
		int curIdx = gui->GetStateInt("ObjStartIdx");
		gui->SetStateInt("ObjStartIdx", curIdx + 1);
		ClearGUIState();
	}
	else if (cmd == "objective_scroll_up_request") 
	{
		// Increment the start index
		int curIdx = gui->GetStateInt("ObjStartIdx");
		gui->SetStateInt("ObjStartIdx", curIdx - 1);
		ClearGUIState();
	}
	else if (cmd == "close") 
	{
		// Set the objectives state flag back to dirty
		ClearGUIState();
	}
	else if (cmd == "diffselect")
	{
		// change the difficulty (skill) level to selected value
		gameLocal.m_DifficultyManager.SetDifficultyLevel(gui->GetStateInt("diffSelect", "0"));
		gui->SetStateInt("ObjStartIdx", 0);

		// reload and redisplay objectives
		m_Objectives.ClearFree();

		idStr startingMapfilename = va("maps/%s", gameLocal.m_MissionManager->GetCurrentStartingMap().c_str());

		// Ensure that the starting map is loaded
		LoadMap(startingMapfilename);

		LoadDirectlyFromMapFile(m_mapFile);

		ClearGUIState();
	}
}

void CMissionData::ClearGUIState() 
{
	m_MissionDataLoadedIntoGUI = false;
}

void CMissionData::UpdateStatisticsGUI(idUserInterface* gui, const idStr& listDefName)
{
	if (gui == NULL) {
		gameLocal.Warning("Can't update statistics GUI, invalid handle.");
		return; // invalid handle, do nothing
	}

	int index(0);
	idStr key("");
	idStr value("");
	idStr sightingBust("");
	idStr space(" ");
	idStr timeSeenString(" "); // grayman #4363

	// The listdef item (name + _) prefix
	idStr prefix = va("%s_item_", listDefName.c_str());
	
	idStr divider(": ");

	key = common->Translate( "#str_02208" );	// Time
	value = idStr(GamePlayTimer::TimeToStr(GetTotalGamePlayTime()));
	gui->SetStateString(prefix + idStr(index++), key + divider + value);

	gui->SetStateString(prefix + idStr(index++), " ");	// Empty line

	key = common->Translate( "#str_02209" );	// Damage Dealt
	value = idStr(GetDamageDealt()) + space + common->Translate( "#str_02210" ) + divider + idStr(GetDamageReceived());	// and received
	gui->SetStateString(prefix + idStr(index++), key + divider + value);

	/*key = "Damage Received"; 
	value = idStr(GetDamageReceived());
	gui->SetStateString(prefix + idStr(index++), key + divider + value);*/

	key = common->Translate( "#str_02211" );	// Health Restored
	value = idStr(GetHealthReceived());
	gui->SetStateString(prefix + idStr(index++), key + divider + value);

	//gui->SetStateString(prefix + idStr(index++), " ");	// Empty line // grayman #4363 - remove empty line

	key = common->Translate( "#str_02212" );	// Pockets Picked 
	value = idStr(GetPocketsPicked()) + common->Translate("#str_02214") + getPocketsTotal();
	gui->SetStateString(prefix + idStr(index++), key + divider + value);

	key = common->Translate( "#str_02213" );	// Loot Acquired
	value = idStr(GetFoundLoot()) + common->Translate( "#str_02214" ) + GetMissionLoot();
	gui->SetStateString(prefix + idStr(index++), key + divider + value);

	//gui->SetStateString(prefix + idStr(index++), " ");	 // Empty line // grayman #4363 - remove empty line

	key = common->Translate( "#str_02215" );	// Killed by the Player
	value = idStr(GetStatOverall(COMP_KILL));
	gui->SetStateString(prefix + idStr(index++), key + divider + value);

	key = common->Translate( "#str_02216" );	// KOed by the Player
	value = idStr(GetStatOverall(COMP_KO));
	gui->SetStateString(prefix + idStr(index++), key + divider + value);

	key = common->Translate( "#str_02217" );	// Bodies found by AI
	value = idStr(GetStatOverall(COMP_AI_FIND_BODY));
	gui->SetStateString(prefix + idStr(index++), key + divider + value);

	// only show secrets statistic if the mission uses the system introduced in 2.10
	if ( m_Stats.secretsTotal ) {
		key = common->Translate("#str_02320");	// Secrets found
		value = idStr(GetSecretsFound()) + common->Translate("#str_02214") + GetSecretsTotal();
		gui->SetStateString(prefix + idStr(index++), key + divider + value);
	}

	gui->SetStateString(prefix + idStr(index++), " ");	// Empty line

	gui->SetStateString(prefix + idStr(index++), common->Translate( "#str_02218" ) ); 	// Alerts:
	
	float sightingScore = GetSightingScore();   // SteveL #3304:
	float stealthScore = GetStealthScore();     //    Zbyl patch

	// grayman #2887 - new way of dealing with number of times the player's been seen

	int timeSeen = GetTotalTimePlayerSeen();	// the amount of time the player was seen
	int busted = GetNumberTimesPlayerSeen();	// how many times AI saw the player

	int secondsSeen = timeSeen/1000; // drop fractional seconds
	int hoursSeen = secondsSeen/(60*60);
	int minutesSeen = (secondsSeen - hoursSeen*60*60) / 60;
	secondsSeen = secondsSeen - hoursSeen*60*60 - minutesSeen*60;
	
	// SteveL #3304: Refactored in Zbyl patch
	if ( busted == 0 )  
	{
		sightingBust = common->Translate( "#str_02221" );	// 0 Sightings.
		// timeSeen won't be displayed
	}
	else if ( busted == 1 )
	{
		// 1 Sighting
		sightingBust = /*va(*/ common->Translate( "#str_02219" )/*, minutesSeen, secondsSeen)*/; // grayman #4363
		timeSeenString = va("%im %is", minutesSeen, secondsSeen); // grayman #4363
	}
	else
	{
		sightingBust = va( common->Translate( "#str_02220" ), busted/*, minutesSeen, secondsSeen*/ ); // N Sightings. // grayman #4363
		timeSeenString = va("%ih %im %is", hoursSeen, minutesSeen, secondsSeen); // grayman #4363
	}

	value = idStr(GetNumberTimesAISuspicious()) + space + common->Translate("#str_02223") + ", " +			// Suspicious
		idStr(GetNumberTimesAISearched()) + space + common->Translate("#str_02224") + ", " +				// Searches
		sightingBust;
	gui->SetStateString(prefix + idStr(index++), value);

	// grayman #4363 - add new 'Time Seen' line if busted one or more times

	key = common->Translate( "#str_02519" );	// Time Seen
	if ( busted > 0 )
	{
		gui->SetStateString(prefix + idStr(index++), key + divider + timeSeenString);
	}
	
	key = common->Translate( "#str_02225" );	// Stealth Score
	value = idStr(stealthScore);
	gui->SetStateString(prefix + idStr(index++), key + divider + value);

	if ( busted == 0 )
	{
		gui->SetStateString(prefix + idStr(index++), " ");	// Empty line
	}

	gui->SetStateString(prefix + idStr(index++), " ");	// Empty line (filler to synch with gui reading)

	int difficultyLevel = gameLocal.m_DifficultyManager.GetDifficultyLevel();
	key = common->Translate( "#str_02226" );	// Difficulty Level
	value = GetDifficultyName(difficultyLevel); // grayman #3292 - get from mission stats, not from difficulty manager
	gui->SetStateString(prefix + idStr(index++), key + divider + value);
	
	// Obsttorte: Times saved
	key = common->Translate( "#str_02915" );	// Times saved
	value = idStr(m_Stats.totalSaveCount-1);	// -1 due to final save
	gui->SetStateString(prefix+idStr(index++), key+divider+value);
	key = "Times loaded";// common->Translate( "#str_02915" );	// Times loaded
	value = idStr( m_Stats.totalLoadCount );
	gui->SetStateString( prefix + idStr( index++ ), key + divider + value );

	/*key = "Frames";
	value = idStr(gameLocal.framenum);
	gui->SetStateString(prefix + idStr(index++), key + "\t" + value);
	key = "GameLocal.time";
	value = idStr(gameLocal.time);
	gui->SetStateString(prefix + idStr(index++), key + "\t" + value);
	key = "GameLocal.realClientTime";
	value = idStr(gameLocal.realClientTime);
	gui->SetStateString(prefix + idStr(index++), key + "\t" + value);*/
	
	index = 30;  // Reset the index to 30 to start .gui lines for "Stealth Score Details" sub-page, starting from gui::listStatistics_item_30.
	
	key = common->Translate( "#str_02227" );		// Stealth Score Details (all alerts * severity)
	gui->SetStateString(prefix + idStr(index++), key);
	
	gui->SetStateString(prefix + idStr(index++), " "); // Empty line

	// these are right aligned
	key = idStr("0");
	gui->SetStateString(prefix + idStr(index++), key);

	key = idStr(GetStatOverall(COMP_ALERT, ai::ESuspicious));
	gui->SetStateString(prefix + idStr(index++), key);
		
	key = idStr(GetStatOverall(COMP_ALERT, ai::ESearching) * 2);
	gui->SetStateString(prefix + idStr(index++), key);

	key = idStr(GetStatOverall(COMP_ALERT, ai::EAgitatedSearching) * 3);
	gui->SetStateString(prefix + idStr(index++), key);
	
	key = idStr(sightingScore);
	gui->SetStateString(prefix + idStr(index++), key);
	
	key = idStr(stealthScore);
	gui->SetStateString(prefix + idStr(index++), key);
	
	gui->SetStateString(prefix + idStr(index++), " "); // Empty line

	value = common->Translate( "#str_02228" );	// Key to Alert Levels:
	gui->SetStateString(prefix + idStr(index++), value);
	
	value = idStr("  1. ") + common->Translate( "#str_02229" );	// Suspicious-1. AI mumbles, continuing on.
	gui->SetStateString(prefix + idStr(index++), value);
	
	value = idStr("  2. ") + common->Translate( "#str_02230" );	// Suspicious-2. AI mumbles, stops and looks.
	gui->SetStateString(prefix + idStr(index++), value);
	
	value = idStr("  3. ") + common->Translate( "#str_02231" );	// Search-1. AI searches.
	gui->SetStateString(prefix + idStr(index++), value);

	value = idStr("  4. ") + common->Translate( "#str_02232" );	// Search-2. AI searches, runs, draws sword.
	gui->SetStateString(prefix + idStr(index++), value);
	
	value = idStr("  5. ") + common->Translate( "#str_02233" );	// Sighting. AI sees you, attacks if can.
	gui->SetStateString(prefix + idStr(index++), value);

	gui->SetStateString(prefix + idStr(index++), " "); // Empty line

	gui->SetStateString(prefix + idStr(index++), " "); // Empty line

	idStr alert = idStr( common->Translate( "#str_02234" ) );	// Alert
	key = alert + " 1." + space + idStr(GetStatOverall(COMP_ALERT, ai::EObservant)) + " * 0:";
	gui->SetStateString(prefix + idStr(index++), key);
	
	key = alert + " 2." + space + idStr(GetStatOverall(COMP_ALERT, ai::ESuspicious)) + " * 1:";
	gui->SetStateString(prefix + idStr(index++), key);

	key = alert + " 3." + space + idStr(GetStatOverall(COMP_ALERT, ai::ESearching)) + " * 2:";
	gui->SetStateString(prefix + idStr(index++), key);

	key = alert + " 4." + space + idStr(GetStatOverall(COMP_ALERT, ai::EAgitatedSearching)) + " * 3:";
	gui->SetStateString(prefix + idStr(index++), key);
	
	key = alert + " 5." + space + idStr(busted) + " * 5:";
	gui->SetStateString(prefix + idStr(index++), key);
	
	key = common->Translate( "#str_02235" );			// Stealth Score Total
	gui->SetStateString(prefix + idStr(index++), key + divider);
	
}

void CMissionData::SetPlayerTeam(int team)
{
	m_PlayerTeam = team;
}

// grayman #3292 - retain difficulty names after map shutdown,
// for the mission statistics screen

void CMissionData::SetDifficultyNames(idStr _difficultyNames[])
{
	for ( int i = 0 ; i < DIFFICULTY_COUNT ; i++ )
	{
		m_Stats._difficultyNames[i] = _difficultyNames[i];
	}
}

// grayman #3292
//
// A copy of DifficultyManager::GetDifficultyName(), but working with
// the copy of the difficulty names stored in the statistics data,
// which survives after the map data is cleared at the end of a
// mission, but before the statistics screen is displayed.

idStr CMissionData::GetDifficultyName(int level)
{
	assert( ( level >= 0 ) && ( level < DIFFICULTY_COUNT ) );

	if ( m_Stats._difficultyNames[level].Length() > 0 )
	{
		return common->Translate( m_Stats._difficultyNames[level] ); // Translate difficulty name
	}

	// Return default name from entityDef

	const idDecl* diffDecl = declManager->FindType(DECL_ENTITYDEF, "difficultyMenu", false);
	const idDeclEntityDef* diffDef = static_cast<const idDeclEntityDef*>(diffDecl);

	// Translate default difficulty name

	return common->Translate( diffDef->dict.GetString(va("diff%ddefault", level), "") );
}

// Obsttorte

int CMissionData::getTotalSaves()
{
	return m_Stats.totalSaveCount;
}

int CMissionData::getPocketsTotal()
{
	return m_Stats.PocketsTotal;
}
// Dragofer

void CMissionData::SetSecretsFound( float secrets )
{
	m_Stats.secretsFound = (int)secrets;
}

void CMissionData::SetSecretsTotal( float secrets )
{
	m_Stats.secretsTotal = (int)secrets;
}