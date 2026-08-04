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



#include "LootRuleSet.h"

void LootRuleSet::Clear()
{
	conversionRate[LOOT_NONE] = 0;
	conversionRate[LOOT_GOLD] = 1;	// 1:1 exchange rates by default
	conversionRate[LOOT_JEWELS] = 1;
	conversionRate[LOOT_GOODS] = 1;

	goldLoss = 0;
	goldLossPercent = 0;
	goldMin = 0;
	goldCap = -1;
}

bool LootRuleSet::operator==(const LootRuleSet& other) const
{
	return goldLoss == other.goldLoss && goldLossPercent == other.goldLossPercent &&
		goldMin == other.goldMin && goldCap == other.goldCap && 
		conversionRate[LOOT_GOLD] == other.conversionRate[LOOT_GOLD] &&
		conversionRate[LOOT_JEWELS] == other.conversionRate[LOOT_JEWELS] &&
		conversionRate[LOOT_GOODS] == other.conversionRate[LOOT_GOODS];
}

void LootRuleSet::LoadFromDict(const idDict& dict, const idStr& prefix)
{
	// greebo: Read each value from spawnarg and use the existing value as default argument, 
	// such that missing spawnargs don't change anything.
	conversionRate[LOOT_GOLD] = dict.GetFloat(prefix + "loot_convrate_gold", va("%f", conversionRate[LOOT_GOLD]));
	conversionRate[LOOT_JEWELS] = dict.GetFloat(prefix + "loot_convrate_jewels", va("%f", conversionRate[LOOT_JEWELS]));
	conversionRate[LOOT_GOODS] = dict.GetFloat(prefix + "loot_convrate_goods", va("%f", conversionRate[LOOT_GOODS]));

	goldLoss = dict.GetInt(prefix + "gold_loss", va("%d", goldLoss));
	goldLossPercent = dict.GetFloat(prefix + "gold_loss_percent", va("%f", goldLossPercent));
	goldMin = dict.GetInt(prefix + "gold_min", va("%d", goldMin));
	goldCap = dict.GetInt(prefix + "gold_cap", va("%d", goldCap));
}

int LootRuleSet::ApplyToFoundLoot(const int foundLoot[LOOT_COUNT], int shopStartingGold)
{
	int totalGold = 0;

	// First, convert all incoming loot types into gold
	for (int i = LOOT_NONE + 1; i < LOOT_COUNT; ++i)
	{
		int valueInGold = static_cast<int>(floor(conversionRate[i] * foundLoot[i]));

		DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Applying conversion rate %f to loot type %d. Before: %d, after: %d\r", 
			conversionRate[i], i, foundLoot[i], valueInGold);

		totalGold += valueInGold;
	}

	DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Gold after conversion: %d\r", totalGold);

	// Percentile loss comes first
	totalGold -= static_cast<int>(totalGold * goldLossPercent*0.01f);

	DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Gold after percentile loss: %d\r", totalGold);

	// Absolute loss
	totalGold -= goldLoss;

	DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Gold after absolute loss: %d\r", totalGold);

	totalGold += shopStartingGold;

	DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Gold after adding shop starting budget: %d\r", totalGold);

	if (goldCap != -1 && totalGold > goldCap)
	{
		totalGold = goldCap;

		DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Cap defined, gold after cap: %d\r", totalGold);
	}

	if (totalGold < goldMin)
	{
		totalGold = goldMin;
	}

	DM_LOG(LC_MAINMENU, LT_DEBUG)LOGSTRING("Gold after minimum check: %d\r", totalGold);

	return totalGold;
}
