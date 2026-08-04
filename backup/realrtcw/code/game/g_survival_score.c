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

// g_survival_score.c

#include "g_local.h"
#include "g_survival.h"

/*
============
Survival_AddKillScore
============
*/
void Survival_AddKillScore(gentity_t *attacker, gentity_t *victim, int meansOfDeath) {
	if (!attacker || !attacker->client || !victim)
		return;

	if (attacker->aiTeam == victim->aiTeam)
		return; // no score for teamkills

	int score = svParams.scoreBaseKill;

	if (meansOfDeath == MOD_KNIFE || meansOfDeath == MOD_KICKED) {
		score += svParams.scoreKnifeBonus;
	}

	if (meansOfDeath == MOD_MACHINEGUN || meansOfDeath == MOD_FLAMETRAP) {
		score = 0;
	}

	attacker->client->ps.persistant[PERS_SCORE] += score;
	attacker->client->ps.persistant[PERS_KILLS]++;
}

/*
============
Survival_AddHeadshotBonus
============
*/
void Survival_AddHeadshotBonus(gentity_t *attacker, gentity_t *victim) {
	if (!attacker || !victim || !attacker->client || attacker->aiTeam == victim->aiTeam)
		return;

	attacker->client->ps.persistant[PERS_SCORE] += svParams.scoreHeadshotKill;
}

/*
============
Survival_AddPainScore
============
*/
void Survival_AddPainScore(gentity_t *attacker, gentity_t *victim, int damage) {
	if (!attacker || !victim || !attacker->client)
		return;

	if (attacker->aiTeam == victim->aiTeam)
		return;

	// Vampire perk healing
	if (attacker->client->ps.powerups[PW_VAMPIRE]) {
		attacker->health += 5;
		if (attacker->health > 300) {
			attacker->health = 300;
		}
	}

	attacker->client->ps.persistant[PERS_SCORE] += svParams.scoreHit;
}

/*
============
Survival_PickupTreasure
============
*/
void Survival_PickupTreasure(gentity_t *other) {
	if (!other || !other->client)
		return;

	// Generate a random score between 50 and 100
	int randomScore = 50 + rand() % 51;
	other->client->ps.persistant[PERS_SCORE] += randomScore;
}

/*
============
Survival_TrySpendMG42Points
============
*/
qboolean Survival_TrySpendMG42Points(gentity_t *player) {
	if (!player || !player->client)
		return qfalse;

	// Require 1 score point to shoot
	if (player->client->ps.persistant[PERS_SCORE] < 1)
		return qfalse;

	player->client->ps.persistant[PERS_SCORE] -= 1;
	return qtrue;
}
