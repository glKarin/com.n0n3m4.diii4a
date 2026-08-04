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

#ifndef MISSIONSTATISTICS_H
#define MISSIONSTATISTICS_H

#include "precompiled.h"

#include "Objective.h" // for objective state enum
#include "../Inventory/LootType.h" // for loot type enum
#include "../ai/Memory.h" // for alert state enum // SteveL #3304

// Maximum array sizes:
#define MAX_TEAMS 64
#define MAX_TYPES 16
#define MAX_AICOMP COMP_COUNT				 // SteveL #3304: Zbyl's patch. Tie these 
#define MAX_ALERTLEVELS (ai::EAlertStateNum) //               constants to their enums.

struct SStat
{
	int Overall;
	int ByTeam[ MAX_TEAMS ];
	int ByType[ MAX_TYPES ];
	int ByInnocence[2];
	int WhileAirborne;

	SStat() 
	{
		Clear();
	}

	void Clear();
};

/**
* Mission stats: Keep track of everything except for loot groups, which are tracked by the inventory
**/
class MissionStatistics
{
public:
	// AI Stats:
	SStat AIStats[ MAX_AICOMP ];
	
	SStat AIAlerts[ MAX_ALERTLEVELS ];

	int DamageDealt;
	int DamageReceived;
	int HealthReceived;
	int PocketsPicked;
	int PocketsTotal;

	// Item stats are handled by the inventory, not here, 
	// Might need this for copying over to career stats though
	int FoundLoot[LOOT_COUNT];

	// greebo: This is the available amount of loot in the mission
	int LootInMission[LOOT_COUNT];

	// This gets read out right at "mission complete" time, is 0 before
	unsigned int TotalGamePlayTime;

	// Use an array to store the objective states after mission complete
	// We need the historic state data to handle conditional objectives.
	// This list will be empty throughout the mission, and is filled on mission complete
	idList<EObjCompletionState> ObjectiveStates;

	// grayman #2887 - for tracking how often and for how long the player was seen
	int numberTimesPlayerSeen;
	int totalTimePlayerSeen;

	// Obsttorte - Total save count

	int totalSaveCount;
	int totalLoadCount;

	// Dragofer - for tracking secrets

	int secretsFound;
	int secretsTotal;

	// grayman #3292 - since the map-defined difficulty names get wiped at map
	// shutdown, and that happens before the statistics screen is displayed,
	// the difficulty names need to be stored where the statistics screen can
	// get at them
		
	idStr _difficultyNames[DIFFICULTY_COUNT]; // The name of each difficulty level

	MissionStatistics() 
	{
		Clear();
	}

	void Clear();

	// Returns the state of the objective specified by the (0-based) index
	// Will return INVALID if the objective index is out of bounds or no data is available
	EObjCompletionState GetObjectiveState(int objNum) const;

	// Store the objective state into the ObjectiveStates array
	void SetObjectiveState(int objNum, EObjCompletionState state);

	// Returns the sum of all found loot types (gold+jewels+goods)
	int GetFoundLootValue() const;

	// Returns the total of all loot types in the mission (gold+jewels+goods)
	int GetTotalLootInMission() const;

	void Save(idSaveGame* savefile) const;
	void Restore(idRestoreGame* savefile);
};

#endif /* MISSIONSTATISTICS_H */
