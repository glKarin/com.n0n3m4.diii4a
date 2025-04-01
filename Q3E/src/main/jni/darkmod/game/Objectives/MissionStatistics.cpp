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



#include "MissionStatistics.h"

void SStat::Clear() 
{
	Overall = 0;

	for (int i = 0; i < MAX_TEAMS; i++)
	{
		ByTeam[i] = 0;
	}

	for (int i = 0; i < MAX_TYPES; i++)
	{
		ByType[i] = 0;
	}

	ByInnocence[0] = 0;
	ByInnocence[1] = 0;
	WhileAirborne = 0;
}

void MissionStatistics::Clear()
{
	for (int i = 0; i < MAX_AICOMP; i++)
	{
		AIStats[i].Clear();
	}

	for (int i = 0; i < MAX_ALERTLEVELS; i++)
	{
		AIAlerts[i].Clear();
	}

	DamageDealt = 0;
	DamageReceived = 0;
	HealthReceived = 0;
	PocketsPicked = 0;
	PocketsTotal = 0;

	secretsFound = 0;
	secretsTotal = 0;

	for (int i = 0; i < LOOT_COUNT; ++i)
	{
		FoundLoot[i] = 0;
		LootInMission[i] = 0;
	}
	
	TotalGamePlayTime = 0;

	ObjectiveStates.Clear();

	totalTimePlayerSeen = 0;	// grayman #2887
	numberTimesPlayerSeen = 0;	// grayman #2887

	for ( int i = 0 ; i < DIFFICULTY_COUNT ; i++) // grayman #3292
	{
		_difficultyNames[i] = "";
	}
	totalSaveCount = 0;
	totalLoadCount = 0;
}

void MissionStatistics::Save(idSaveGame* savefile) const
{
	// Save mission stats
	for(int j = 0; j < MAX_AICOMP; j++)
	{
		savefile->WriteInt(AIStats[j].Overall);
		savefile->WriteInt(AIStats[j].WhileAirborne);

		for (int k1 = 0; k1 < MAX_TEAMS; k1++)
		{
			savefile->WriteInt(AIStats[j].ByTeam[k1]);
		}

		for (int k2 = 0; k2 < MAX_TYPES; k2++)
		{
			savefile->WriteInt(AIStats[j].ByType[k2]);
		}

		savefile->WriteInt(AIStats[j].ByInnocence[0]);
		savefile->WriteInt(AIStats[j].ByInnocence[1]);
	}

	for (int l = 0; l < MAX_ALERTLEVELS; l++)
	{
		savefile->WriteInt(AIAlerts[l].Overall);
		savefile->WriteInt(AIAlerts[l].WhileAirborne);

		for (int m1 = 0; m1 < MAX_TEAMS; m1++)
		{
			savefile->WriteInt(AIAlerts[l].ByTeam[m1]);
		}

		for (int m2 = 0; m2 < MAX_TYPES; m2++)
		{
			savefile->WriteInt(AIAlerts[l].ByType[m2]);
		}

		savefile->WriteInt(AIAlerts[l].ByInnocence[0]);
		savefile->WriteInt(AIAlerts[l].ByInnocence[1]);
	}

	savefile->WriteInt(DamageDealt);
	savefile->WriteInt(DamageReceived);
	savefile->WriteInt(PocketsPicked);

	savefile->WriteInt(secretsFound);
	savefile->WriteInt(secretsTotal);

	for (int i = 0; i < LOOT_COUNT; ++i)
	{
		savefile->WriteInt(FoundLoot[i]);
		savefile->WriteInt(LootInMission[i]);
	}

	savefile->WriteUnsignedInt(TotalGamePlayTime);
	savefile->WriteInt(numberTimesPlayerSeen);	// grayman #2887
	savefile->WriteInt(totalTimePlayerSeen);	// grayman #2887

	savefile->WriteInt(ObjectiveStates.Num());
	for (int i = 0; i < ObjectiveStates.Num(); ++i)
	{
		savefile->WriteInt(ObjectiveStates[i]);
	}
	
	for ( int i = 0 ; i < DIFFICULTY_COUNT ; i++) // grayman #3292
	{
		savefile->WriteString(_difficultyNames[i]);
	}
	( (MissionStatistics*) this )->totalSaveCount++;
	savefile->WriteInt(totalSaveCount);	// Obsttorte
	savefile->WriteInt(totalLoadCount);
}

void MissionStatistics::Restore(idRestoreGame* savefile)
{
	// Restore mission stats
	for (int j = 0; j < MAX_AICOMP; j++)
	{
		savefile->ReadInt(AIStats[j].Overall);
		savefile->ReadInt(AIStats[j].WhileAirborne);

		for (int k1 = 0; k1 < MAX_TEAMS; k1++)
		{
			savefile->ReadInt(AIStats[j].ByTeam[k1]);
		}

		for (int k2 = 0; k2 < MAX_TYPES; k2++)
		{
			savefile->ReadInt(AIStats[j].ByType[k2]);
		}

		savefile->ReadInt(AIStats[j].ByInnocence[0]);
		savefile->ReadInt(AIStats[j].ByInnocence[1]);
	}

	for (int l = 0; l < MAX_ALERTLEVELS; l++)
	{
		savefile->ReadInt(AIAlerts[l].Overall);
		savefile->ReadInt(AIAlerts[l].WhileAirborne);

		for (int m1 = 0; m1 < MAX_TEAMS; m1++)
		{
			savefile->ReadInt(AIAlerts[l].ByTeam[m1]);
		}

		for (int m2 = 0; m2 < MAX_TYPES; m2++)
		{
			savefile->ReadInt(AIAlerts[l].ByType[m2]);
		}

		savefile->ReadInt(AIAlerts[l].ByInnocence[0]);
		savefile->ReadInt(AIAlerts[l].ByInnocence[1]);
	}	

	savefile->ReadInt(DamageDealt);
	savefile->ReadInt(DamageReceived);
	savefile->ReadInt(PocketsPicked);

	savefile->ReadInt(secretsFound);
	savefile->ReadInt(secretsTotal);

	for (int i = 0; i < LOOT_COUNT; ++i)
	{
		savefile->ReadInt(FoundLoot[i]);
		savefile->ReadInt(LootInMission[i]);
	}

	savefile->ReadUnsignedInt(TotalGamePlayTime);
	savefile->ReadInt(numberTimesPlayerSeen);	// grayman #2887
	savefile->ReadInt(totalTimePlayerSeen);		// grayman #2887

	int num;
	savefile->ReadInt(num);
	ObjectiveStates.SetNum(num);
	
	for (int i = 0; i < ObjectiveStates.Num(); ++i)
	{
		int state;
		savefile->ReadInt(state);

		assert(state >= STATE_INCOMPLETE && state <= STATE_FAILED);

		ObjectiveStates[i] = static_cast<EObjCompletionState>(state);
	}

	for (int i = 0; i < DIFFICULTY_COUNT; i++) // grayman #3292
	{
		savefile->ReadString(_difficultyNames[i]);
	}
	savefile->ReadInt(totalSaveCount);
	savefile->ReadInt(totalLoadCount);
	totalLoadCount++;
}

EObjCompletionState MissionStatistics::GetObjectiveState(int objNum) const
{
	if (objNum < 0 || objNum >= ObjectiveStates.Num())
	{
		return STATE_INVALID;
	}

	return ObjectiveStates[objNum];
}

void MissionStatistics::SetObjectiveState(int objNum, EObjCompletionState state)
{
	// Resize to fit if necessary
	if (objNum >= ObjectiveStates.Num())
	{
		ObjectiveStates.SetNum(objNum + 1);
	}

	ObjectiveStates[objNum] = state;
}

int MissionStatistics::GetFoundLootValue() const
{
	int sum = 0;

	for (int i = LOOT_NONE+1; i < LOOT_COUNT; ++i)
	{
		sum += FoundLoot[i];
	}

	return sum;
}

int MissionStatistics::GetTotalLootInMission() const
{
	int sum = 0;

	for (int i = LOOT_NONE+1; i < LOOT_COUNT; ++i)
	{
		sum += LootInMission[i];
	}

	return sum;
}
