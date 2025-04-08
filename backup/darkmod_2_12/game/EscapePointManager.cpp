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



#include "EscapePointManager.h"

#define SPAWNARG_IS_GUARDED "is_guarded"
#define SPAWNARG_TEAM "team"

/**
 * ---------------- EscapePoint implementation ----------------
 */
void EscapePoint::Save(idSaveGame *savefile) const
{
	savefile->WriteInt(id);
	pathFlee.Save(savefile);
	savefile->WriteInt(aasId);
	savefile->WriteVec3(origin);
	savefile->WriteInt(areaNum);
	savefile->WriteInt(team);
	savefile->WriteBool(isGuarded);
}

void EscapePoint::Restore(idRestoreGame *savefile)
{
	savefile->ReadInt(id);
	pathFlee.Restore(savefile);
	savefile->ReadInt(aasId);
	savefile->ReadVec3(origin);
	savefile->ReadInt(areaNum);
	savefile->ReadInt(team);
	savefile->ReadBool(isGuarded);
}

/**
 * ---------------- CEscapePointManager implementation ----------------------
 */
CEscapePointManager::CEscapePointManager() :
	_escapeEntities(new EscapeEntityList),
	_highestEscapePointId(0)
{}

CEscapePointManager::~CEscapePointManager()
{}

void CEscapePointManager::Clear()
{
	_escapeEntities->ClearFree();
}

void CEscapePointManager::Save(idSaveGame *savefile) const
{
	savefile->WriteInt(_highestEscapePointId);

	savefile->WriteInt(_escapeEntities->Num());
	for (int i = 0; i < _escapeEntities->Num(); i++)
	{
		(*_escapeEntities)[i].Save(savefile);
	}

	// The number of AAS escape point maps
    savefile->WriteInt(static_cast<int>(_aasEscapePoints.size()));
		
	for (AASEscapePointMap::const_iterator it = _aasEscapePoints.begin();
		 it != _aasEscapePoints.end();
		 ++it)
	{
		idAAS* aasPtr = it->first;
		const EscapePointList& escapePointList = *it->second;
		
		// First get the AAS id for the pointer
		savefile->WriteInt(gameLocal.GetAASId(aasPtr));

		// Second, write out the list of escape points
		savefile->WriteInt(escapePointList.Num());
		for (int i = 0; i < escapePointList.Num(); i++)
		{
			escapePointList[i].Save(savefile);
		}
	}

	// The _aasEscapePointIndex is rebuilt on restore
}

void CEscapePointManager::Restore(idRestoreGame *savefile)
{
	int num; // dummy integer

	savefile->ReadInt(_highestEscapePointId);

	savefile->ReadInt(num);
	_escapeEntities->SetNum(num);
	for (int i = 0; i < num; i++)
	{
		(*_escapeEntities)[i].Restore(savefile);
	}

	// Clear the shortcut index map, it gets filled on the fly
	_aasEscapePointIndex.clear();

	// The number of AAS escape point maps
	savefile->ReadInt(num);
	
	for (int map = 0; map < num; map++)
	{
		// Get the idAAS* pointer from the savegame
		int aasId;
		savefile->ReadInt(aasId);
		idAAS* aasPtr = gameLocal.GetAAS(aasId);

		// Allocate a new list for the aasPtr
		_aasEscapePoints[aasPtr] = EscapePointListPtr(new EscapePointList);
		// Get a shortcut reference (for better code readabiltiy)
		EscapePointList& escapePointList = *_aasEscapePoints[aasPtr];

		int numPoints;
		savefile->ReadInt(numPoints);
		
		// Resize the list according to the savefile info
		escapePointList.SetNum(numPoints);
		for (int i = 0; i < numPoints; i++)
		{
			escapePointList[i].Restore(savefile);

			// Store the pointer to this escape point into the lookup table
			_aasEscapePointIndex[escapePointList[i].id] = &escapePointList[i];
		}
	}
}

void CEscapePointManager::AddEscapePoint(tdmPathFlee* escapePoint)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Adding escape point: %s\r", escapePoint->name.c_str());

	idEntityPtr<tdmPathFlee> pathFlee;
	pathFlee = escapePoint;
	_escapeEntities->Append(pathFlee);
}

void CEscapePointManager::RemoveEscapePoint(tdmPathFlee* escapePoint)
{
	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Removing escape point: %s\r", escapePoint->name.c_str());
	for (int i = 0; i < _escapeEntities->Num(); i++)
	{
		if ((*_escapeEntities)[i].GetEntity() == escapePoint) 
		{
			_escapeEntities->RemoveIndex(i);
			return;
		}
	}

	// Not found
	DM_LOG(LC_AI, LT_ERROR)LOGSTRING("Failed to remove escape point: %s\r", escapePoint->name.c_str());
}

void CEscapePointManager::InitAAS()
{
	for (int i = 0; i < gameLocal.NumAAS(); i++)
	{
		idAAS* aas = gameLocal.GetAAS(i);

		if (aas != NULL) {
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("EscapePointManager: Initializing AAS: %s\r", aas->GetSettings()->fileExtension.c_str());

			// Allocate a new list for this AAS type
			_aasEscapePoints[aas] = EscapePointListPtr(new EscapePointList);

			// Now go through our master list and retrieve the area numbers 
			// for each tdmPathFlee entity
			for (int i = 0; i < _escapeEntities->Num(); i++)
			{
				tdmPathFlee* escapeEnt = (*_escapeEntities)[i].GetEntity();
				int areaNum = aas->PointAreaNum(escapeEnt->GetPhysics()->GetOrigin());

				DM_LOG(LC_AI, LT_INFO)LOGSTRING("Flee entity %s is in area number %d\r", escapeEnt->name.c_str(), areaNum);
				if (areaNum != -1)
				{
					// Increase the unique escape point ID
					_highestEscapePointId++;

					// Fill the EscapePoint structure with the relevant information
					EscapePoint escPoint;

					escPoint.id = _highestEscapePointId;
					escPoint.aasId = gameLocal.GetAASId(aas);
					escPoint.areaNum = areaNum;
					escPoint.origin = escapeEnt->GetPhysics()->GetOrigin();
					escPoint.pathFlee = (*_escapeEntities)[i];
					escPoint.isGuarded = escapeEnt->spawnArgs.GetBool(SPAWNARG_IS_GUARDED);
					escPoint.team = escapeEnt->spawnArgs.GetInt(SPAWNARG_TEAM);

					// Pack the info structure to this list
					int newIndex = _aasEscapePoints[aas]->Append(escPoint);

					// Store the pointer to this escape point into the lookup table
					// Looks ugly, it basically does this: map[int] = EscapePoint*
					_aasEscapePointIndex[escPoint.id] = &( (*_aasEscapePoints[aas])[newIndex] );
				}
			}

			DM_LOG(LC_AI, LT_INFO)LOGSTRING("EscapePointManager: AAS initialized: %s\r", aas->GetSettings()->fileExtension.c_str());
		}
	}
}

EscapePoint* CEscapePointManager::GetEscapePoint(int id)
{
	// Check the id for validity
	assert(_aasEscapePointIndex.find(id) != _aasEscapePointIndex.end());
	return _aasEscapePointIndex[id];
}

EscapeGoal CEscapePointManager::GetEscapeGoal(EscapeConditions& conditions)
{
	assert(conditions.aas != NULL);
	// The AAS pointer has to be known
	assert(_aasEscapePoints.find(conditions.aas) != _aasEscapePoints.end());

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Calculating escape point info.\r");

	// A timer object to measure the time it needs to calculate the escape route
	idTimer timer;
	timer.Start();

	// Create a shortcut to the list
	EscapePointList& escapePoints = *_aasEscapePoints[conditions.aas];

	EscapeGoal goal;

	if (escapePoints.Num() == 0)
	{
		gameLocal.Warning("No escape point information available for the given aas type in map!");
		goal.escapePointId = -1;
		goal.distance = -1;
		return goal;
	}

	if (escapePoints.Num() == 1) 
	{
		// grayman #3548 - Don't just return the only escape point.
		// Let the evaluator check for hostile AI in the escape point's neighborhood.
		// Dispense with the guarded and friendly checks.
		conditions.algorithm = FIND_ANY;

		/*
		// Only one point available, return that one
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("Only one escape point available, returning this one: %d.\r", escapePoints[0].id);

		goal.escapePointId = escapePoints[0].id;
		goal.distance = (conditions.self.GetEntity()->GetPhysics()->GetOrigin() - escapePoints[0].origin).LengthFast();
		return goal;
		*/
	}

	// Run the evaluation

	// The evaluator pointer
	EscapePointEvaluatorPtr evaluator;
	
	switch (conditions.algorithm)
	{
		case FIND_ANY:
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("EscapePoint Lookup Algorithm: FIND_ANY\r");
			evaluator = EscapePointEvaluatorPtr(new AnyEscapePointFinder(conditions));
			break;
		case FIND_GUARDED:
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("EscapePoint Lookup Algorithm: FIND_GUARDED\r");
			evaluator = EscapePointEvaluatorPtr(new GuardedEscapePointFinder(conditions));
			break;
		case FIND_FRIENDLY:
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("EscapePoint Lookup Algorithm: FIND_FRIENDLY\r");
			evaluator = EscapePointEvaluatorPtr(new FriendlyEscapePointFinder(conditions));
			break;
		case FIND_FRIENDLY_GUARDED:
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("EscapePoint Lookup Algorithm: FIND_FRIENDLY_GUARDED\r");
			evaluator = EscapePointEvaluatorPtr(new FriendlyGuardedEscapePointFinder(conditions));
			break;
		default:
			DM_LOG(LC_AI, LT_INFO)LOGSTRING("EscapePoint Lookup Algorithm: DEFAULT = FIND_ANY\r");
			// This is the default algorithm: seek the farthest point
			evaluator = EscapePointEvaluatorPtr(new AnyEscapePointFinder(conditions));
	};

	assert(evaluator); // the pointer must be non-NULL after this point
	
	for ( int i = 0 ; i < escapePoints.Num() ; i++ )
	{
		if ( !evaluator->Evaluate(escapePoints[i]) ) 
		{
			// Evaluator returned FALSE, break the loop
			break;
		}
	}

	goal.escapePointId = evaluator->GetBestEscapePoint();

	if ( goal.escapePointId == -1 )
	{
		// No point found, return false
		DM_LOG(LC_AI, LT_DEBUG)LOGSTRING("No escape point found!\r");
		return goal;
	}

	timer.Stop();

	// Calculate the distance and store it into the goal structure
	EscapePoint* bestPoint = GetEscapePoint(goal.escapePointId);
	goal.distance = (conditions.self.GetEntity()->GetPhysics()->GetOrigin() - bestPoint->origin).LengthFast();

	DM_LOG(LC_AI, LT_DEBUG)LOGSTRING(
		"Best escape point has ID %d at %f %f %f in area %d.\r", 
		goal.escapePointId, 
		bestPoint->origin.x, bestPoint->origin.y, bestPoint->origin.z, 
		bestPoint->areaNum
	);

	DM_LOG(LC_AI, LT_INFO)LOGSTRING("Escape route calculation took %f msec.\r", timer.Milliseconds());

	return goal;
}

CEscapePointManager* CEscapePointManager::Instance()
{
	static CEscapePointManager _manager;
	return &_manager;
}
