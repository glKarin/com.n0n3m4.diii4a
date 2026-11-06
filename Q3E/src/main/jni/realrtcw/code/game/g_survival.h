/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).  

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/



// g_survival.h

#ifndef __G_SURVIVAL_H__
#define __G_SURVIVAL_H__

// Everything related to the score system
void Survival_AddKillScore(gentity_t *attacker, gentity_t *victim, int meansOfDeath);
void Survival_AddHeadshotBonus(gentity_t *attacker, gentity_t *victim);
void Survival_AddPainScore(gentity_t *attacker, gentity_t *victim, int damage);
void Survival_PickupTreasure(gentity_t *other);
qboolean Survival_TrySpendMG42Points(gentity_t *player);

// Purchase system
void Use_Target_buy(gentity_t *ent, gentity_t *other, gentity_t *activator);
qboolean Survival_HandleRandomWeaponBox(gentity_t *ent, gentity_t *activator, char *itemName, int *itemIndex);
qboolean Survival_HandleRandomPerkBox(gentity_t *ent, gentity_t *activator, char **itemName, int *itemIndex);
qboolean Survival_HandleAmmoPurchase(gentity_t *ent, gentity_t *activator, int price);
qboolean Survival_HandleWeaponOrGrenade(gentity_t *ent, gentity_t *activator, gitem_t *item, int price);
qboolean Survival_HandleWeaponUpgrade(gentity_t *ent, gentity_t *activator, int price) ;
qboolean Survival_HandleArmorPurchase(gentity_t *activator, gitem_t *item, int price);
qboolean Survival_HandlePerkPurchase(gentity_t *activator, gitem_t *item, int price);
int Survival_GetDefaultWeaponPrice(int weapon);
int Survival_GetDefaultPerkPrice(int perk);
void Touch_objective_info ( gentity_t * ent , gentity_t * other , trace_t * trace ) ;

// Misc stuff
void TossClientItems_Survival(gentity_t *self, gentity_t *attacker);
void TossClientPowerups(gentity_t *self, gentity_t *attacker);
gentity_t *SelectSpawnPoint_AI ( gentity_t *player, gentity_t *ent, vec3_t origin, vec3_t angles ) ;
void AICast_TickSurvivalWave( void );


// Survival parameters
typedef struct svParams_s
{
	// not loaded
	int activeAI[NUM_CHARACTERS];
	int survivalKillCount;
	int maxActiveAI[NUM_CHARACTERS];
	int waveCount;
	int waveKillCount;
	int killCountRequirement;

	int spawnedThisWave;
	int spawnedThisWaveFriendly;
	qboolean wavePending;              
    int waveChangeTime;
	qboolean waveInProgress;

	qboolean specialWaveActive;
	int lastSpecialWave;

	// loaded from .surv file
	int initialKillCountRequirement;

	int initialSoldiersCount;
	int initialMercsCount;
	int initialTrenchCount;
	int initialEliteGuardsCount;
	int initialBlackGuardsCount;
	int initialVenomsCount;

	int initialZombiesCount;
	int initialFleshCount;
	int initialWarriorsCount;
	int initialProtosCount;
	int initialGhostsCount;
	int initialPriestsCount;
	int initialPartisansCount;
	int initialFlamersCount;
	int initialLopersCount;

    int   defaultSpawnTime;
	int   egSpawnTime;
	int   trenchSpawnTime;
	int   bgSpawnTime;
	int   vSpawnTime;
	int   protoSpawnTime;

	int   warzSpawnTime;
	int   ghostSpawnTime;
	int   priestSpawnTime;
	int   flamerSpawnTime;
	int   loperSpawnTime;

	int   friendlySpawnTime;
	int   aliveFriendliestoCallReinforce;

	int soldiersIncrease;
	int mercsIncrease;
	int trenchIncrease;
	int eliteGuardsIncrease;
	int blackGuardsIncrease;
	int venomsIncrease;
	int zombiesIncrease;
	int fleshIncrease;
	int warriorsIncrease;
	int protosIncrease;
	int ghostsIncrease;
	int priestsIncrease;
	int flamersIncrease;
	int lopersIncrease;

	int maxSoldiers;
	int maxMercs;
	int maxTrench;
	int maxEliteGuards;
	int maxBlackGuards;
	int maxVenoms;

	int maxZombies;
	int maxFlesh;
	int maxWarriors;
	int maxProtos;
	int maxGhosts;
	int maxPriests;
	int maxFlamers;
	int maxLopers;

	int waveEg;
	int waveTrench;
	int waveBg;
	int waveV;

	int waveWarz;
	int waveProtos;
	int waveGhosts;
	int wavePriests;
	int waveFlamers;
	int waveLopers;

	int powerupDropChance;
	int powerupDropChanceScavengerIncrease;

	int treasureDropChance;
	int treasureDropChanceScavengerIncrease;

	int scoreHeadshotKill;
	int scoreHit;
	int scoreBaseKill;
	int scoreKnifeBonus;

	int maxPerks;
	int maxPerksEng;

	int armorPrice;
	int randomPerkPrice;
	int randomWeaponPrice;
	int weaponUpgradePrice;
	int upgradedAmmoPrice;

	int secondchancePrice;
	int runnerPrice;
	int scavengerPrice;
	int fasthandsPrice;
	int doubleshotPrice;
	int resiliencePrice;
	int defaultPerkPrice;

	int knifePrice;
	int lugerPrice;
	int coltPrice;
	int silencerPrice;
	int tt33Price;
	int revolverPrice;
	int akimboPrice;
	int hdmPrice;
	int dualtt33Price;
	int mp40Price;
	int stenPrice;
	int mp34Price;
	int thompsonPrice;
	int ppshPrice;
	int mauserPrice;
	int mosinPrice;
	int delislePrice;
	int sniperriflePrice;
	int snooperScopePrice;
	int m1garandPrice;
	int g43Price;
	int m1941Price;
	int mp44Price;
	int barPrice;
	int fg42Price;
	int shotgunPrice;
	int auto5Price;
	int mg42mPrice;
	int browningPrice;
	int panzerPrice;
	int flamerPrice;
	int teslaPrice;
	int venomPrice;
	int grenPrice;
	int pineapplePrice;
	int defaultWeaponPrice;

	int intermissionTime;
	int prepareTime;

	float ltAmmoBonus;
	float soldierExplosiveDmgBonus;
	float cvopsmeleeDmgBonus;
	float cvopsthrowspeedBonus;

	int specialWaveMinStart;
	int specialWaveChance;		
	int specialLopersInitialCount;		
	int specialLopersIncrease;
	int specialLopersMax;
	int specialMinGap;
	int specialMaxGap;

} svParams_t;

extern svParams_t svParams;

#endif // __G_SURVIVAL_H__