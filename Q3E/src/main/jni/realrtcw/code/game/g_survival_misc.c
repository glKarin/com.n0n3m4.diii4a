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

// g_survival_misc.c

#include "g_local.h"
#include "g_survival.h"

svParams_t svParams;

/*
============
TossClientItems_Survival
============
*/
void TossClientItems_Survival(gentity_t *self, gentity_t *attacker) {
    gitem_t *item;
    vec3_t forward;
    float angle;
    gentity_t *drop = NULL;

    if (!attacker || !attacker->client) return;
    if (attacker->aiTeam == self->aiTeam) return;

    const char *treasure = "item_treasure";
    const char *adrenaline = "holdable_bg_syringe";

    AngleVectors(self->r.currentAngles, forward, NULL, NULL);
    angle = 45;

    int dropChance = svParams.treasureDropChance;
    if (attacker->client->ps.perks[PERK_SCAVENGER] > 0) {
        dropChance += svParams.treasureDropChanceScavengerIncrease;
    }

    if (rand() % 100 < dropChance) {
        item = BG_FindItemForClassName(treasure);
        if (item) {
            drop = Drop_Item(self, item, 0, qfalse);
            if (drop) {
                drop->nextthink = level.time + 30000;
            }
        }
    }

    // Scavenger PRO: chance to drop adrenaline
    if (attacker->client->ps.perks[PERK_SCAVENGER] >= 2) {
        if (rand() % 100 < 3) {
            item = BG_FindItemForClassName(adrenaline);
            if (item) {
                drop = Drop_Item(self, item, 0, qfalse);
                if (drop) {
                    drop->nextthink = level.time + 30000;
                }
            }
        }
    }
}

/*
============
TossClientPowerups
============
*/
void TossClientPowerups(gentity_t *self, gentity_t *attacker) {
    gitem_t *item;
    vec3_t forward;
    float angle;
    gentity_t *drop = NULL;
    int powerup = 0;

    if (svParams.specialWaveActive && self->aiCharacter == AICHAR_LOPER_SPECIAL) {
        // Only the very last special Loper drops ammo
        if (svParams.waveKillCount == svParams.killCountRequirement) {
            item = BG_FindItemForPowerup(PW_AMMO);
            if (item) {
                drop = Drop_Item(self, item, 0, qfalse);
                if (drop) drop->nextthink = level.time + 30000;
            }
        }
        return; // All other special Lopers drop nothing
    }

    if (!attacker || !attacker->client) return;
    if (attacker->aiTeam == self->aiTeam) return;

    AngleVectors(self->r.currentAngles, forward, NULL, NULL);
    angle = 45;

    int dropChance = svParams.powerupDropChance;
    if (attacker->client->ps.perks[PERK_SCAVENGER] > 0) {
        dropChance += svParams.powerupDropChanceScavengerIncrease;
    }

    if (rand() % 100 < dropChance) {
        switch (rand() % 4) {
            case 0: powerup = PW_QUAD; break;
            case 1: powerup = PW_BATTLESUIT_SURV; break;
            case 2: powerup = PW_VAMPIRE; break;
            case 3: powerup = PW_AMMO; break;
        }

        item = BG_FindItemForPowerup(powerup);
        if (item) {
            drop = Drop_Item(self, item, 0, qfalse);
            if (drop) {
                drop->nextthink = level.time + 30000;
            }
        }
    }
}


static qboolean Survival_SpotMatchesAI( gentity_t *spot, gentity_t *ent ) {
    if ( !spot ) return qfalse;

    // disabled?
    if ( spot->spawnflags & 1 ) return qfalse;

    // strict team gate (no wildcard)
    if ( ent ) {
        if ( spot->aiTeam != ent->aiTeam ) return qfalse;
    } else {
        // If ent is NULL, only allow neutral spots
        if ( spot->aiTeam != 0 ) return qfalse;
    }

    // class/boss gate
    if ( ent ) {
        switch ( ent->aiCharacter ) {
        case AICHAR_PROTOSOLDIER:
        case AICHAR_SUPERSOLDIER:
        case AICHAR_HELGA:
        case AICHAR_HEINRICH:
        case AICHAR_SUPERSOLDIER_LAB:
            if ( !( spot->spawnflags & 2 ) ) return qfalse;
            break;
        default:
            if ( spot->spawnflags & 2 ) return qfalse;
            break;
        }
    }

    // ainame gate
    if ( spot->aiName && spot->aiName[0] ) {
        if ( !ent || !ent->aiName || !ent->aiName[0] ) return qfalse;
        if ( Q_stricmp( spot->aiName, ent->aiName ) != 0 ) return qfalse;
    }

    return qtrue;
}

gentity_t *SelectNearestDeathmatchSpawnPoint_AI( gentity_t *player, gentity_t *ent ) {
    gentity_t *spot = NULL, *nearestSpot = NULL;
    vec3_t delta;
    float dist, nearestDist = 999999.0f;

    while ( ( spot = G_Find( spot, FOFS(classname), "info_ai_respawn" ) ) != NULL ) {

        if ( !Survival_SpotMatchesAI( spot, ent ) ) {
            continue;
        }

        // distance is measured to player, if provided
        if ( player ) {
            VectorSubtract( spot->s.origin, player->r.currentOrigin, delta );
            dist = VectorLength( delta );
        } else {
            // if no player passed, just pick first valid as "nearest"
            dist = 0.0f;
        }

        if ( dist < nearestDist ) {
            nearestDist = dist;
            nearestSpot = spot;
        }
    }

    return nearestSpot;
}


/*
================
SelectRandomDeathmatchSpawnPoint_AI
================
*/
#define MAX_SPAWN_POINTS_AI            128
#define MAX_SPAWN_POINT_DISTANCE       8196

gentity_t *SelectRandomDeathmatchSpawnPoint_AI( gentity_t *player, gentity_t *ent ) {
    gentity_t *spot = NULL;
    gentity_t *spots[MAX_SPAWN_POINTS_AI];
    int numSpots = 0;

    while ( ( spot = G_Find( spot, FOFS(classname), "info_ai_respawn" ) ) != NULL ) {

        if ( !Survival_SpotMatchesAI( spot, ent ) ) {
            continue;
        }

        if ( player ) {
            vec3_t delta;
            float dist;
            VectorSubtract( spot->s.origin, player->r.currentOrigin, delta );
            dist = VectorLength( delta );
            if ( dist >= MAX_SPAWN_POINT_DISTANCE ) continue;
            if ( SpotWouldTelefrag( spot ) ) continue;
        }

        if ( numSpots < MAX_SPAWN_POINTS_AI ) {
            spots[numSpots++] = spot;
        }
    }

    if ( numSpots == 0 ) return NULL;
    return spots[ rand() % numSpots ];
}


/*
===========
SelectSpawnPoint_AI
===========
*/
gentity_t *SelectSpawnPoint_AI( gentity_t *player, gentity_t *ent, vec3_t origin, vec3_t angles ) {
    gentity_t *spot;
    gentity_t *nearestSpot;

    nearestSpot = SelectNearestDeathmatchSpawnPoint_AI( player, ent );

    spot = SelectRandomDeathmatchSpawnPoint_AI( player, ent );
    if ( spot == nearestSpot ) {
        spot = SelectRandomDeathmatchSpawnPoint_AI( player, ent );
        if ( spot == nearestSpot ) {
            spot = SelectRandomDeathmatchSpawnPoint_AI( player, ent );
        }
    }

    // fallback: if no nearby spot was found, select any valid spot (still strict team)
    if ( !spot ) {
        spot = SelectRandomDeathmatchSpawnPoint_AI( NULL, ent );
    }

    if ( !spot ) {
        G_Error( "Couldn't find a spawn point (info_ai_respawn) for aiTeam=%d", ent ? ent->aiTeam : -1 );
    }

    VectorCopy( spot->s.origin, origin );
    origin[2] += 9;
    VectorCopy( spot->s.angles, angles );

    return spot;
}