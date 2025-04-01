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
#ifndef ESCAPE_POINT_MANAGER__H
#define ESCAPE_POINT_MANAGER__H

#include "precompiled.h"
#include <map>

#include "EscapePointEvaluator.h"

template<class type>
class idEntityPtr;

class idAI;
class idEntity;
class tdmPathFlee;

/**
 * greebo: The algorithm type to be used for 
 *         evaluating the escape points.
 */
enum EscapePointAlgorithm
{
	FIND_ANY = 0,                  // Find any escape point
	FIND_GUARDED,                  // Find a guarded escape point
	FIND_FRIENDLY,                 // Find a friendly escape point
	FIND_FRIENDLY_GUARDED,		   // Find a guarded AND friendly escape point
	FIND_AAS_AREA_FAR_FROM_THREAT, // Finds any AAS area that is far away enough from the threat
};

enum EscapeDistanceOption
{
	DIST_DONT_CARE = 0,       // Don't care whether nearer or farther
	DIST_NEAREST,             // Find the nearest
	DIST_FARTHEST,            // Find the farthest escape point
};

struct EscapeConditions
{
	// The AI who is fleeing
	idEntityPtr<idAI> self;

	// The position to flee from
	idVec3 fromPosition;

	// The threatening entity to flee from
	idEntityPtr<idEntity> fromEntity;

	// grayman #3317 - if we're fleeing from a murder or KO, this will be where that event took place
	idVec3 threatPosition;

	// The AAS the fleeing AI is using.
	idAAS* aas;

	// Whether the distance should be considered or not
	EscapeDistanceOption distanceOption;

	// The minimum distance to the threatening entity.
	// Set this to negative values if this should be ignored.
	float minDistanceToThreat;

	// The algorithm to use
	EscapePointAlgorithm algorithm;
};

/**
 * greebo: This represents one escape point in a given AAS grid. 
 */
class EscapePoint
{
public:
	// A unique ID for this escape point
	int id;

	// The actual entity this escape point is located in
	idEntityPtr<tdmPathFlee> pathFlee;

	// The AAS id of this point
	int aasId;

	// The actual origin of the entity
	idVec3 origin;

	// The AAS area number of the entity's origin.
	int areaNum;

	// The team this escape point is belonging to (default = 0, neutral)
	int team;

	// TRUE, if an armed AI is supposed to hang around at the escape point.
	bool isGuarded;

	// Constructor
	EscapePoint() :
		aasId(-1),
		areaNum(-1),
		team(0), // neutral
		isGuarded(false)
	{}

	void Save( idSaveGame *savefile ) const;
	void Restore( idRestoreGame *savefile );
};

// This is a result structure delivered by the escape point manager
// containing information about how to get to an escape point 
struct EscapeGoal
{
	// The escape point ID (valid IDs are > 0)
	int escapePointId;

	// The distance to this escape point
	float distance;
};

class CEscapePointManager
{
private:
	// A list of Escape Point entities
	typedef idList< idEntityPtr<tdmPathFlee> > EscapeEntityList;

	// The pointer-type for the list above
	typedef std::shared_ptr<EscapeEntityList> EscapeEntityListPtr;

	// The list of AAS-specific escape points plus shared_ptr typedef.
	typedef idList<EscapePoint> EscapePointList;
	typedef std::shared_ptr<EscapePointList> EscapePointListPtr;

	// A map associating an AAS to EscapePointLists.
	typedef std::map<idAAS*, EscapePointListPtr> AASEscapePointMap;

	// A map associating EscPointIds to EscPoints for fast lookup during runtime
	typedef std::map<int, EscapePoint*> EscapePointIndex;

	// This is the master list containing all the escape point entities in this map
	EscapeEntityListPtr _escapeEntities;

	// The map of escape points for each AAS type.
	AASEscapePointMap _aasEscapePoints;

	// A lookup table for EscapePoints (by unique ID)
	EscapePointIndex _aasEscapePointIndex;

	// The highest used escape point ID
	int _highestEscapePointId;

public:

	CEscapePointManager();
	~CEscapePointManager();

	void	Clear();

	void	Save( idSaveGame *savefile ) const;
	void	Restore( idRestoreGame *savefile );

	void	AddEscapePoint(tdmPathFlee* escapePoint);
	void	RemoveEscapePoint(tdmPathFlee* escapePoint);

	// Retrieve the escape point with the given unique ID.
	EscapePoint* GetEscapePoint(int id);

	/**
	 * greebo: Call this after the entities are spawned. This sets up the
	 *         AAS types for each pathFlee entity.
	 */
	void	InitAAS();

	/**
	 * greebo: Retrieve an escape goal for the given escape conditions.
	 */
	EscapeGoal GetEscapeGoal(EscapeConditions& conditions);

	// Accessor to the singleton instance of this class
	static CEscapePointManager* Instance();
};

#endif /* ESCAPE_POINT_MANAGER__H */
