/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
#include "g_local.h"

// g_client.c -- client functions that don't happen every frame


static vec3_t	playerMins = {-15, -15, -24};
static vec3_t	playerMaxs = {15, 15, 32};

/*QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32) initial
potential spawning position for deathmatch games.
The first time a player enters the game, they will be at an 'initial' spot.
Targets will be fired when someone spawns in on them.
"nobots" will prevent bots from using this spot.
"nohumans" will prevent non-bots from using this spot.
"count" is used to limit the number of times the spawnpoint can be used. When 0 or omitted there is no limit
*/

void info_player_deathmatch_use (gentity_t *self, gentity_t *other, gentity_t *activator) {
	self->flags ^= FL_NO_SPAWN;	//toggle no spawn flag
}

void SP_info_player_deathmatch( gentity_t *ent ) {
	int		i;

	G_SpawnInt( "count", "0", &ent->count);
	ent->damage = 0; //damage is used to keep track of the number of times this spawnpoint was used.

	G_SpawnInt( "nobots", "0", &i);
	if ( i ) {
		ent->flags |= FL_NO_BOTS;
	}
	G_SpawnInt( "nohumans", "0", &i );
	if ( i ) {
		ent->flags |= FL_NO_HUMANS;
	}

	//if DISABLED spawnflag is, disable the spawnpoint at start
	if (ent->spawnflags & 2)
		info_player_deathmatch_use( ent, NULL, NULL );
	
	ent->use = info_player_deathmatch_use;

	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );
	ent->classname = "info_player_deathmatch";
	ent->s.eType = ET_GENERAL;
	ent->s.pos.trType = TR_STATIONARY;
	VectorSet( ent->r.mins, -10, -10, -10);
	VectorSet( ent->r.maxs, 10, 10, 10 );
	ent->r.contents = CONTENTS_TRIGGER;
	//ent->s.modelindex = G_ModelIndex( "45.md3" );
	
	trap_LinkEntity( ent );
}

/*QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
equivelant to info_player_deathmatch
*/
void SP_info_player_start(gentity_t *ent) {
	ent->classname = "info_player_deathmatch";
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );
	ent->s.eType = ET_GENERAL;
	ent->s.pos.trType = TR_STATIONARY;
	VectorSet( ent->r.mins, -10, -10, -10);
	VectorSet( ent->r.maxs, 10, 10, 10 );
	ent->r.contents = CONTENTS_TRIGGER;
	//ent->s.modelindex = G_ModelIndex( "45.md3" );

	SP_info_player_deathmatch( ent );
}

//Three for Double_D
void SP_info_player_dd(gentity_t *ent) {
		VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );
	ent->classname = "info_player_dd";
	ent->s.eType = ET_GENERAL;
	ent->s.pos.trType = TR_STATIONARY;
	VectorSet( ent->r.mins, -10, -10, -10);
	VectorSet( ent->r.maxs, 10, 10, 10 );
	ent->r.contents = CONTENTS_TRIGGER;
	//ent->s.modelindex = G_ModelIndex( "45.md3" );
	
	trap_LinkEntity( ent );
}
void SP_info_player_dd_red(gentity_t *ent) {
		VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );
	ent->classname = "info_player_dd_red";
	ent->s.eType = ET_GENERAL;
	ent->s.pos.trType = TR_STATIONARY;
	VectorSet( ent->r.mins, -10, -10, -10);
	VectorSet( ent->r.maxs, 10, 10, 10 );
	ent->r.contents = CONTENTS_TRIGGER;
	//ent->s.modelindex = G_ModelIndex( "45.md3" );
	
	trap_LinkEntity( ent );
}
void SP_info_player_dd_blue(gentity_t *ent) {
		VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );
	ent->classname = "info_player_dd_blue";
	ent->s.eType = ET_GENERAL;
	ent->s.pos.trType = TR_STATIONARY;
	VectorSet( ent->r.mins, -10, -10, -10);
	VectorSet( ent->r.maxs, 10, 10, 10 );
	ent->r.contents = CONTENTS_TRIGGER;
	//ent->s.modelindex = G_ModelIndex( "45.md3" );
	
	trap_LinkEntity( ent );
}

//One for Standard Domination, not really a player spawn point
void SP_domination_point(gentity_t *ent) {
}

/*QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
The intermission will be viewed from this point.  Target an info_notnull for the view direction.
*/
void SP_info_player_intermission( gentity_t *ent ) {

}

/*
=======================================================================

  SelectSpawnPoint

=======================================================================
*/

/*
================
SpotWouldTelefrag

================
*/
qboolean SpotWouldTelefrag( gentity_t *spot ) {
	int			i, num;
	gentity_t	*hit;
	vec3_t		mins, maxs;

	VectorAdd( spot->s.origin, playerMins, mins );
	VectorAdd( spot->s.origin, playerMaxs, maxs );
	num = trap_EntitiesInBox( mins, maxs, SourceTechEntityList, MAX_GENTITIES );

	for (i=0 ; i<num ; i++) {
		hit = &g_entities[SourceTechEntityList[i]];
		//if ( hit->client && hit->client->ps.stats[STAT_HEALTH] > 0 ) {
		if ( hit->client) {
			return qtrue;
		}

	}

	return qfalse;
}


/*
===========
SpawnPointIsActive

Returns the active state of a spawnpoint. A spawnpoint is not active
when it has reached the maximum number of spawns. A spawnpoint's
count key is used to set the max number of spawns allowed in that spot.
If a spawnpoint has it's FL_NO_SPAWN flag set, it is also disabled.
This flag can be toggled by aiming at the spawnpoint with a trigger.

============
*/
qboolean SpawnPointIsActive( gentity_t *spot ) {
	if ( spot->flags & FL_NO_SPAWN )
		return qfalse;

	if ( !spot->count )
		return qtrue;

	//damage is used to keep track of the number of spawns at this spot
	if ( spot->damage < spot->count )
		return qtrue;

	return qfalse;
}


/*
================
SelectNearestDeathmatchSpawnPoint

Find the spot that we DON'T want to use
================
*/
#define	MAX_SPAWN_POINTS	128
gentity_t *SelectNearestDeathmatchSpawnPoint( vec3_t from ) {
	gentity_t	*spot;
	vec3_t		delta;
	float		dist, nearestDist;
	gentity_t	*nearestSpot;

	nearestDist = 999999;
	nearestSpot = NULL;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {

		VectorSubtract( spot->s.origin, from, delta );
		dist = VectorLength( delta );
		if ( dist < nearestDist && SpawnPointIsActive(spot) ) {
			nearestDist = dist;
			nearestSpot = spot;
		}
	}

	return nearestSpot;
}


/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point that doesn't telefrag
================
*/
#define	MAX_SPAWN_POINTS	128
gentity_t *SelectRandomDeathmatchSpawnPoint( void ) {
	gentity_t	*spot;
	int			count;
	int			selection;
	gentity_t	*spots[MAX_SPAWN_POINTS];

	count = 0;
	spot = NULL;

	//find the spots that are active and do not telefrag
	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if ( SpotWouldTelefrag( spot ) || !SpawnPointIsActive( spot ) ) {
			continue;
		}
		spots[ count ] = spot;
		count++;
	}

	//no spots are found, so we look for active spots even if they telefrag
	if ( !count ) {
		spot = NULL;
		while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
			if ( !SpawnPointIsActive( spot ) ) {
				continue;
			}
			spots[ count ] = spot;
			count++;
		}
	}
	

	if ( !count ) {	
		// there are no active spots at all, so we'll allow spawning at a random non-active spot
		// TODO: Make a more predictable solution for this
		
		//G_Error( "Couldn't find a spawn point" );
		return G_Find( NULL, FOFS(classname), "info_player_deathmatch");
	}

	selection = rand() % count;
	return spots[ selection ];
}

/*
===========
SelectRandomFurthestSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
gentity_t *SelectRandomFurthestSpawnPoint ( vec3_t avoidPoint, vec3_t origin, vec3_t angles ) {
	gentity_t	*spot;
	vec3_t		delta;
	float		dist;
	float		list_dist[64];
	gentity_t	*list_spot[64];
	int			numSpots, rnd, i, j;

	numSpots = 0;
	spot = NULL;

	// Find spots that are active and don't telefrag
	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if ( SpotWouldTelefrag( spot ) || !SpawnPointIsActive( spot ) ) {
			continue;
		}
		VectorSubtract( spot->s.origin, avoidPoint, delta );
		dist = VectorLength( delta );
		for (i = 0; i < numSpots; i++) {
			if ( dist > list_dist[i] ) {
				if ( numSpots >= 64 )
					numSpots = 64-1;
				for (j = numSpots; j > i; j--) {
					list_dist[j] = list_dist[j-1];
					list_spot[j] = list_spot[j-1];
				}
				list_dist[i] = dist;
				list_spot[i] = spot;
				numSpots++;
				if (numSpots > 64)
					numSpots = 64;
				break;
			}
		}
		if (i >= numSpots && numSpots < 64) {
			list_dist[numSpots] = dist;
			list_spot[numSpots] = spot;
			numSpots++;
		}
	}

	if (!numSpots) {
		// No spots were found, so find spots that are active even if they telefrag
		while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
			if ( !SpawnPointIsActive( spot ) ) {
				continue;
			}
			VectorSubtract( spot->s.origin, avoidPoint, delta );
			dist = VectorLength( delta );
			for (i = 0; i < numSpots; i++) {
				if ( dist > list_dist[i] ) {
					if ( numSpots >= 64 )
						numSpots = 64-1;
					for (j = numSpots; j > i; j--) {
						list_dist[j] = list_dist[j-1];
						list_spot[j] = list_spot[j-1];
					}
					list_dist[i] = dist;
					list_spot[i] = spot;
					numSpots++;
					if (numSpots > 64)
						numSpots = 64;
					break;
				}
			}
			if (i >= numSpots && numSpots < 64) {
				list_dist[numSpots] = dist;
				list_spot[numSpots] = spot;
				numSpots++;
			}
		}
	}

	if (!numSpots) {
		// there are no active spots at all, so we'll allow spawning at a random non-active spot
		spot = G_Find( NULL, FOFS(classname), "info_player_deathmatch");
		if (!spot)
			G_Error( "Couldn't find a spawn point" );
		VectorCopy (spot->s.origin, origin);
		origin[2] += 9;
		VectorCopy (spot->s.angles, angles);
		return spot;
	}

	// select a random spot from the spawn points furthest away
	rnd = random() * (numSpots / 2);

	VectorCopy (list_spot[rnd]->s.origin, origin);
	origin[2] += 9;
	VectorCopy (list_spot[rnd]->s.angles, angles);

	return list_spot[rnd];
}

/*
===========
FindTeleporterTarget

Finds target location for holdable_teleporter if forced
============
*/
gentity_t *FindTeleporterTarget ( gentity_t *ent, vec3_t origin, vec3_t angles ) {
	gentity_t *target;
	
	target = G_PickTarget( ent->teleporterTarget );
	
	//target does not exist, so find a random spawnpoint
	if ( !target ) {
		return SelectSpawnPoint( ent->client->ps.origin, origin, angles);
	}

	VectorCopy (target->s.origin, origin);
	origin[2] += 9;
	VectorCopy (target->s.angles, angles);

	return target;
}

/*
===========
SelectSpawnPoint

Chooses a player start, deathmatch start, etc
============
*/
gentity_t *SelectSpawnPoint ( vec3_t avoidPoint, vec3_t origin, vec3_t angles ) {
	//return SelectRandomFurthestSpawnPoint( avoidPoint, origin, angles );


	gentity_t	*spot;
	gentity_t	*nearestSpot;

	nearestSpot = SelectNearestDeathmatchSpawnPoint( avoidPoint );

	spot = SelectRandomDeathmatchSpawnPoint ( );
	if ( spot == nearestSpot ) {
		// roll again if it would be real close to point of death
		spot = SelectRandomDeathmatchSpawnPoint ( );
		if ( spot == nearestSpot ) {
			// last try
			spot = SelectRandomDeathmatchSpawnPoint ( );
		}
	}

	// find a single player start spot
	if (!spot) {
		G_Error( "Couldn't find a spawn point" );
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
}

/*
===========
SelectInitialSpawnPoint

Try to find a spawn point marked 'initial', where the client
is allowed to spawn. Otherwise use normal spawn selection.
It is assumed there's only ONE spawnpoint marked 'initial'
in the map.
============
*/
gentity_t *SelectInitialSpawnPoint( vec3_t origin, vec3_t angles ) {
	gentity_t	*spot;

	spot = NULL;
	while ((spot = G_Find (spot, FOFS(classname), "info_player_deathmatch")) != NULL) {
		if ( (spot->spawnflags & 1) && SpawnPointIsActive(spot) ) {
			break;
		}
	}

	if ( !spot || SpotWouldTelefrag( spot ) ) {
		return SelectSpawnPoint( vec3_origin, origin, angles );
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
}

/*
===========
SelectSpectatorSpawnPoint

============
*/
gentity_t *SelectSpectatorSpawnPoint( vec3_t origin, vec3_t angles ) {
	//gentity_t	*spot;

	FindIntermissionPoint();

	VectorCopy( level.intermission_origin, origin );
	VectorCopy( level.intermission_angle, angles );



	//for some reason we need to return an specific point in elimination (this might not be neccecary anymore but to be sure...)
	//if(g_gametype.integer == GT_ELIMINATION)
	//	return SelectSpawnPoint( vec3_origin, origin, angles );

	//VectorCopy (origin,spot->s.origin);
	//spot->s.origin[2] += 9;
	//VectorCopy (angles, spot->s.angles);

	return NULL;
}

/*
=======================================================================

BODYQUE

=======================================================================
*/

/*
===============
InitBodyQue
===============
*/
void InitBodyQue (void) {
	int		i;
	gentity_t	*ent;

	level.bodyQueIndex = 0;
	for (i=0; i<BODY_QUEUE_SIZE ; i++) {
		ent = G_Spawn();
		ent->classname = "bodyque";
		ent->neverFree = qtrue;
		level.bodyQue[i] = ent;
	}
}

/*
=============
BodySink

After sitting around for five seconds, fall into the ground and dissapear
=============
*/
void BodySink( gentity_t *ent ) {
	if ( level.time - ent->timestamp > 6500 ) {


		// the body ques are never actually freed, they are just unlinked
		trap_UnlinkEntity( ent );
		ent->physicsObject = qfalse;
		return;
	}
	ent->nextthink = level.time + 100;
	ent->s.pos.trBase[2] -= 1;
	//GibEntity( ent, 0 );
}

/*
=============
CopyToBodyQue

A player is respawning, so make an entity that looks
just like the existing corpse to leave behind.
=============
*/
void CopyToBodyQue( gentity_t *ent ) {
	gentity_t	*e;
	int i;
	gentity_t		*body;
	int			contents;

	trap_UnlinkEntity (ent);

	// if client is in a nodrop area, don't leave the body
	contents = trap_PointContents( ent->s.origin, -1 );
	if ( (contents & CONTENTS_NODROP) && !(ent->s.eFlags & EF_KAMIKAZE) ) { //the check for kamikaze is a workaround for ctf4ish
            return;
	}

	// grab a body que and cycle to the next one
	body = level.bodyQue[ level.bodyQueIndex ];
	level.bodyQueIndex = (level.bodyQueIndex + 1) % BODY_QUEUE_SIZE;

        //Check if the next body has the kamikaze, in that case skip it.
        for(i=0;(level.bodyQue[ level.bodyQueIndex ]->s.eFlags & EF_KAMIKAZE) && (i<10);i++) {
            level.bodyQueIndex = (level.bodyQueIndex + 1) % BODY_QUEUE_SIZE;
        }

	body->s = ent->s;
	body->s.eFlags = EF_DEAD;		// clear EF_TALK, etc
	if ( ent->s.eFlags & EF_KAMIKAZE ) {
                ent->s.eFlags &= ~EF_KAMIKAZE;
		body->s.eFlags |= EF_KAMIKAZE;

		// check if there is a kamikaze timer around for this owner
		for (i = 0; i < MAX_GENTITIES; i++) {
			e = &g_entities[i];
			if (!e->inuse)
				continue;
			if (e->activator != ent)
				continue;
			if (strcmp(e->classname, "kamikaze timer"))
				continue;
			e->activator = body;
			break;
		}
	}
	body->s.powerups = 0;	// clear powerups
	body->s.loopSound = 0;	// clear lava burning
	body->s.number = body - g_entities;
	body->timestamp = level.time;
	body->physicsObject = qtrue;
	body->physicsBounce = 0;		// don't bounce
	if ( body->s.groundEntityNum == ENTITYNUM_NONE ) {
		body->s.pos.trType = TR_GRAVITY;
		body->s.pos.trTime = level.time;
		VectorCopy( ent->client->ps.velocity, body->s.pos.trDelta );
	} else {
		body->s.pos.trType = TR_STATIONARY;
	}
	body->s.event = 0;

	// change the animation to the last-frame only, so the sequence
	// doesn't repeat anew for the body
	switch ( body->s.legsAnim & ~ANIM_TOGGLEBIT ) {
	case BOTH_DEATH1:
	case BOTH_DEAD1:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD1;
		break;
	case BOTH_DEATH2:
	case BOTH_DEAD2:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD2;
		break;
	case BOTH_DEATH3:
	case BOTH_DEAD3:
	default:
		body->s.torsoAnim = body->s.legsAnim = BOTH_DEAD3;
		break;
	}

	body->r.svFlags = ent->r.svFlags;
	VectorCopy (ent->r.mins, body->r.mins);
	VectorCopy (ent->r.maxs, body->r.maxs);
	VectorCopy (ent->r.absmin, body->r.absmin);
	VectorCopy (ent->r.absmax, body->r.absmax);

	body->clipmask = CONTENTS_SOLID | CONTENTS_PLAYERCLIP;
	body->r.contents = CONTENTS_CORPSE;
	body->r.ownerNum = ent->s.number;

	body->nextthink = level.time + 5000;
	body->think = BodySink;

	body->die = body_die;

	// don't take more damage if already gibbed
	if ( ent->health <= GIB_HEALTH ) {
		body->takedamage = qfalse;
	} else {
		body->takedamage = qtrue;
	}


	VectorCopy ( body->s.pos.trBase, body->r.currentOrigin );
	trap_LinkEntity (body);
}

/*
==================
SetClientViewAngle

==================
*/
void SetClientViewAngle( gentity_t *ent, vec3_t angle ) {
	int			i;

	// set the delta angle
	for (i=0 ; i<3 ; i++) {
		int		cmdAngle;

		cmdAngle = ANGLE2SHORT(angle[i]);
		ent->client->ps.delta_angles[i] = cmdAngle - ent->client->pers.cmd.angles[i];
	}
	VectorCopy( angle, ent->s.angles );
	VectorCopy (ent->s.angles, ent->client->ps.viewangles);
}

/*
================
respawn
================
*/
void ClientRespawn( gentity_t *ent ) {
	gentity_t	*tent;

if(ent->singlebot >= 1){
	//kick fragged bots from game
	DropClientSilently( ent->client->ps.clientNum );
	return;
}

	if((g_gametype.integer!=GT_ELIMINATION && g_gametype.integer!=GT_CTF_ELIMINATION && g_gametype.integer !=GT_LMS) && !ent->client->isEliminated)
	{
		ent->client->isEliminated = qtrue; //must not be true in warmup
		//Tried moving CopyToBodyQue
	} else {
                //Must always be false in other gametypes
                ent->client->isEliminated = qfalse;
        }
        CopyToBodyQue (ent); //Unlinks ent

	if(g_gametype.integer==GT_LMS) {
		if(ent->client->pers.livesLeft>0)
		{
			//ent->client->pers.livesLeft--; Coutned down somewhere else
			ent->client->isEliminated = qfalse;
		}
		else //We have used all our lives
		{
			if( ent->client->isEliminated!=qtrue) {
				ent->client->isEliminated = qtrue;
				if((g_lms_mode.integer == 2 || g_lms_mode.integer == 3) && level.roundNumber == level.roundNumberStarted)
					LMSpoint();
                                //Sago: This is really bad
                                //TODO: Try not to make people spectators here
				ent->client->sess.spectatorState = PM_SPECTATOR;
                                //We have to force spawn imidiantly to prevent lag.
                                ClientSpawn(ent);
			}
			return;
		}
	}

	if((g_gametype.integer==GT_ELIMINATION || g_gametype.integer==GT_CTF_ELIMINATION || g_gametype.integer==GT_LMS)
			&& ent->client->ps.pm_type == PM_SPECTATOR && ent->client->ps.stats[STAT_HEALTH] > 0)
		return;
		ClientSpawn(ent);

		// add a teleportation effect
		if(g_gametype.integer!=GT_ELIMINATION && g_gametype.integer!=GT_CTF_ELIMINATION && g_gametype.integer!=GT_LMS)
		{
			tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_IN );
			tent->s.clientNum = ent->s.clientNum;
		}
}

/*
================
respawnRound
================
*/
void respawnRound( gentity_t *ent ) {
	gentity_t	*tent;
        if(ent->client->hook)
                Weapon_HookFree(ent->client->hook);

        trap_UnlinkEntity (ent);

	ClientSpawn(ent);

        // add a teleportation effect
        if(g_gametype.integer!=GT_ELIMINATION && g_gametype.integer!=GT_CTF_ELIMINATION && g_gametype.integer!=GT_LMS)
        {
                tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_IN );
                tent->s.clientNum = ent->s.clientNum;
        }
}

/*
================
TeamCount

Returns number of players on a team
================
*/
team_t TeamCount( int ignoreClientNum, int team ) {
	int		i;
	int		count = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( i == ignoreClientNum ) {
			continue;
		}
		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}

                if ( level.clients[i].pers.connected == CON_CONNECTING) {
                        continue;
                }

		if ( level.clients[i].sess.sessionTeam == team ) {
			count++;
		}
	}

	return count;
}

/*
================
TeamLivingCount

Returns number of living players on a team
================
*/
team_t TeamLivingCount( int ignoreClientNum, int team ) {
	int		i;
	int		count = 0;
	qboolean	LMS = (g_gametype.integer==GT_LMS);

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( i == ignoreClientNum ) {
			continue;
		}
		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}

                if ( level.clients[i].pers.connected == CON_CONNECTING) {
                        continue;
                }
		//crash if g_gametype.integer is used here, why?
		if ( level.clients[i].sess.sessionTeam == team && (level.clients[i].ps.stats[STAT_HEALTH]>0 || LMS) && !(level.clients[i].isEliminated)) {
			count++;
		}
	}

	return count;
}

/*
================
TeamHealthCount

Count total number of healthpoints on teh teams used for draws in Elimination
================
*/

team_t TeamHealthCount(int ignoreClientNum, int team ) {
	int 		i;
	int 		count = 0;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( i == ignoreClientNum ) {
			continue;
		}
		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}

                if ( level.clients[i].pers.connected == CON_CONNECTING) {
                        continue;
                }

		//only count clients with positive health
		if ( level.clients[i].sess.sessionTeam == team && (level.clients[i].ps.stats[STAT_HEALTH]>0)&& !(level.clients[i].isEliminated)) {
			count+=level.clients[i].ps.stats[STAT_HEALTH];
		}
	}

	return count;
}


/*
================
RespawnAll

Forces all clients to respawn.
================
*/

void RespawnAll(void)
{
	int i;
	gentity_t	*client;
	for(i=0;i<level.maxclients;i++)
	{
		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}

                if ( level.clients[i].pers.connected == CON_CONNECTING) {
                        continue;
                }

		if ( level.clients[i].sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}
		client = g_entities + i;
		client->client->ps.pm_type = PM_NORMAL;
		client->client->pers.livesLeft = g_lms_lives.integer;
		respawnRound(client);
	}
	return;
}

/*
================
RespawnDead

Forces all *DEAD* clients to respawn.
================
*/

void RespawnDead(void)
{
	int i;
	gentity_t	*client;
	for(i=0;i<level.maxclients;i++)
	{

		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}
                if ( level.clients[i].pers.connected == CON_CONNECTING) {
                        continue;
                }
                client = g_entities + i;
                client->client->pers.livesLeft = g_lms_lives.integer-1;
		if ( level.clients[i].isEliminated == qfalse ){
			continue;
		}
		if ( level.clients[i].sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}

		client->client->pers.livesLeft = g_lms_lives.integer;

		respawnRound(client);
	}
	return;
}

/*
================
DisableWeapons

disables all weapons
================
*/

void DisableWeapons(void)
{
	int i;
	gentity_t	*client;
	for(i=0;i<level.maxclients;i++)
	{
		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}
                if ( level.clients[i].pers.connected == CON_CONNECTING) {
                        continue;
                }

		if ( level.clients[i].sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}
		client = g_entities + i;
		client->client->ps.pm_flags |= PMF_ELIMWARMUP;
	}
        ProximityMine_RemoveAll(); //Remove all the prox mines
	return;
}

/*
================
EnableWeapons

enables all weapons
================
*/

void EnableWeapons(void)
{
	int i;
	gentity_t	*client;
	for(i=0;i<level.maxclients;i++)
	{
		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}

		if ( level.clients[i].sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}

		/*if ( level.clients[i].isEliminated == qtrue ){
			continue;
		}*/

		client = g_entities + i;
		client->client->ps.pm_flags &= ~PMF_ELIMWARMUP;
	}
	return;
}

/*
================
LMSpoint

Gives a point to the lucky survivor
================
*/

void LMSpoint(void)
{
	int i;
	gentity_t	*client;
	for(i=0;i<level.maxclients;i++)
	{
		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}

		if ( level.clients[i].sess.sessionTeam == TEAM_SPECTATOR ) {
			continue;
		}

		if ( level.clients[i].isEliminated ){
			continue;
		}

		client = g_entities + i;
		/*
		Not good in mode 2 & 3
		if ( client->health <= 0 ){
			continue;
		}
		*/

		client->client->ps.persistant[PERS_SCORE] += 1;
                        G_LogPrintf("PlayerScore: %i %i: %s now has %d points\n",
		i, client->client->ps.persistant[PERS_SCORE], client->client->pers.netname, client->client->ps.persistant[PERS_SCORE] );
	}

	CalculateRanks();
	return;
}

/*
================
TeamLeader

Returns the client number of the team leader
================
*/
int TeamLeader( int team ) {
	int		i;

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( level.clients[i].sess.sessionTeam == team ) {
			if ( level.clients[i].sess.teamLeader )
				return i;
		}
	}

	return -1;
}


/*
================
PickTeam

================
*/
team_t PickTeam( int ignoreClientNum ) {
	int		counts[TEAM_NUM_TEAMS];

	counts[TEAM_BLUE] = TeamCount( ignoreClientNum, TEAM_BLUE );
	counts[TEAM_RED] = TeamCount( ignoreClientNum, TEAM_RED );

	if ( ( counts[TEAM_BLUE] > counts[TEAM_RED] ) ) {
		return TEAM_RED;
	}
	if ( ( counts[TEAM_RED] > counts[TEAM_BLUE] ) ) {
		return TEAM_BLUE;
	}
	// equal team count, so join the team with the lowest score
	if ( ( level.teamScores[TEAM_BLUE] > level.teamScores[TEAM_RED] ) ) {
		return TEAM_RED;
	}
	if ( ( level.teamScores[TEAM_RED] > level.teamScores[TEAM_BLUE] ) ) {
	    return TEAM_BLUE;
    }

    return TEAM_BLUE;
}

/*
===========
ForceClientSkin

Forces a client's skin (for teamplay)
===========
*/
/*
static void ForceClientSkin( gclient_t *client, char *model, const char *skin ) {
	char *p;

	if ((p = strrchr(model, '/')) != 0) {
		*p = 0;
	}

	Q_strcat(model, MAX_QPATH, "/");
	Q_strcat(model, MAX_QPATH, skin);
}
*/

/*
===========
ClientCheckName
============
*/
static void ClientCleanName(const char *in, char *out, int outSize, int clientNum)
{
    int outpos = 0, colorlessLen = 0, spaces = 0, notblack=0;
    qboolean black = qfalse;

    // discard leading spaces
    for(; *in == ' '; in++);

    for(; *in && outpos < outSize - 1; in++)
    {
        out[outpos] = *in;

        if(*in == ' ')
        {
            // don't allow too many consecutive spaces
            if(spaces > 2)
                continue;

            spaces++;
        }
        else if(outpos > 0 && out[outpos - 1] == Q_COLOR_ESCAPE)
        {
            if(Q_IsColorString(&out[outpos - 1]))
            {
                colorlessLen--;

                if(ColorIndex(*in) == 0)
                {
                    // Disallow color black in names to prevent players
                    // from getting advantage playing in front of black backgrounds
                    //outpos--;
                    //continue;
                    black = qtrue;
                }
                else
                    black = qfalse;
            }
            else
            {
                spaces = 0;
                colorlessLen++;
            }
        }
        else
        {
            spaces = 0;
            colorlessLen++;
            if(!black && (Q_isalpha(*in) || (*in>='0' && *in<='9') ) )
                notblack++;
        }

        outpos++;
    }

    out[outpos] = '\0';

    //There was none not-black alphanum chars. Remove all colors
    if(notblack<1)
        Q_CleanStr(out);

    // don't allow empty names
    if( *out == '\0' || colorlessLen == 0)
        Q_strncpyz(out, va("Nameless%i",clientNum), outSize );
}

/*
===========
ClientUserInfoChanged

Called from ClientConnect when the player first connects and
directly by the server system when the player updates a userinfo variable.

The game can override any of the settings and call trap_SetUserinfo
if desired.
============
*/
void ClientUserinfoChanged( int clientNum ) {
	gentity_t *ent;
	int		teamTask, teamLeader, team, health;
	int		singlebot;
	int		botskill;
	char	*s;
	char	model[MAX_QPATH];
	char	headModel[MAX_QPATH];
	char	oldname[MAX_STRING_CHARS];
	char        err[MAX_STRING_CHARS];
	qboolean    revertName = qfalse;
	gclient_t	*client;
	char	c1[MAX_INFO_STRING];
	char	heligred[MAX_INFO_STRING];
	char	heliggreen[MAX_INFO_STRING];
	char	heligblue[MAX_INFO_STRING];
	char	toligred[MAX_INFO_STRING];
	char	toliggreen[MAX_INFO_STRING];
	char	toligblue[MAX_INFO_STRING];
	char	pligred[MAX_INFO_STRING];
	char	pliggreen[MAX_INFO_STRING];
	char	pligblue[MAX_INFO_STRING];
	char	pgred[MAX_INFO_STRING];
	char	pggreen[MAX_INFO_STRING];
	char	pgblue[MAX_INFO_STRING];
	char	swep_id[MAX_INFO_STRING];
	char	redTeam[MAX_INFO_STRING];
	char	blueTeam[MAX_INFO_STRING];
	char	userinfo[MAX_INFO_STRING];

	ent = g_entities + clientNum;
	client = ent->client;

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	// check for malformed or illegal info strings
	if ( !Info_Validate(userinfo) ) {
		strcpy (userinfo, "\\name\\badinfo");
	}

	// check for local client
	s = Info_ValueForKey( userinfo, "ip" );
	if ( !strcmp( s, "localhost" ) ) {
		client->pers.localClient = qtrue;
	}

	// check the item prediction
	s = Info_ValueForKey( userinfo, "cg_predictItems" );
	if ( !atoi( s ) ) {
		client->pers.predictItemPickup = qfalse;
	} else {
		client->pers.predictItemPickup = qtrue;
	}

	//unlagged - client options
	// see if the player has opted out
	s = Info_ValueForKey( userinfo, "cg_delag" );
	if ( !atoi( s ) ) {
		client->pers.delag = 0;
	} else {
		client->pers.delag = atoi( s );
	}

	// see if the player is nudging his shots
	s = Info_ValueForKey( userinfo, "cg_cmdTimeNudge" );
	client->pers.cmdTimeNudge = atoi( s );

	// set name
	Q_strncpyz ( oldname, client->pers.netname, sizeof( oldname ) );
	s = Info_ValueForKey (userinfo, "name");
	ClientCleanName( s, client->pers.netname, sizeof(client->pers.netname), clientNum );

    //KK-OAPub Added From Tremulous-Control Name Changes
    if( strcmp( oldname, client->pers.netname ) )
    {
        //Never revert a bots name... just to bad if it hapens... but the bot will always be expendeble :-)
        if (ent->r.svFlags & SVF_BOT)
            revertName = qfalse;

        if( revertName )
        {
            Q_strncpyz( client->pers.netname, *oldname ? oldname : "QSPlayer",
                sizeof( client->pers.netname ) );
            Info_SetValueForKey( userinfo, "name", oldname );
            trap_SetUserinfo( clientNum, userinfo );
        }
    }

	ent->tool_id = atoi( Info_ValueForKey( userinfo, "toolgun_tool" ) );

	if ( ent->r.svFlags & SVF_BOT ) {
	botskill = atoi( Info_ValueForKey( userinfo, "skill" ) );
	ent->botskill = botskill;
	singlebot = atoi( Info_ValueForKey( userinfo, "singlebot" ) );
	ent->singlebot = singlebot;
	if(ent->singlebot){
	if(!G_NpcFactionProp(NP_PICKUP, ent)){
	ent->client->ps.stats[STAT_NO_PICKUP] = 1;
	ent->wait_to_pickup = 100000000;
	}}
	}

	// N_G: this condition makes no sense to me and I'm not going to
	// try finding out what it means, I've added parentheses according to
	// evaluation rules of the original code so grab a
	// parentheses pairing highlighting text editor and see for yourself
	// if you got it right
	//Sago: One redundant check and CTF Elim and LMS was missing. Not an important function and I might never have noticed, should properly be ||
	if ( ( ( client->sess.sessionTeam == TEAM_SPECTATOR ) ||
		( ( ( client->isEliminated ) /*||
		( client->ps.pm_type == PM_SPECTATOR )*/ ) &&   //Sago: If this is true client.isEliminated or TEAM_SPECTATOR is true to and this is redundant
		( g_gametype.integer == GT_ELIMINATION || g_gametype.integer == GT_CTF_ELIMINATION || g_gametype.integer == GT_LMS) ) ) &&
		( client->sess.spectatorState == SPECTATOR_SCOREBOARD ) ) {

		Q_strncpyz( client->pers.netname, "scoreboard", sizeof(client->pers.netname) );
	}

	if ( client->pers.connected == CON_CONNECTED ) {
		if ( strcmp( oldname, client->pers.netname ) ) {
			trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " renamed to %s\n\"", oldname,
				client->pers.netname) );
		}
	}

	// set model
	if( g_gametype.integer >= GT_TEAM && g_ffa_gt==0) {
		Q_strncpyz( model, Info_ValueForKey (userinfo, "team_model"), sizeof( model ) );
		Q_strncpyz( headModel, Info_ValueForKey (userinfo, "team_headmodel"), sizeof( headModel ) );
		strcpy(redTeam, Info_ValueForKey( userinfo, "team_legsskin" ));
	} else {
		Q_strncpyz( model, Info_ValueForKey (userinfo, "model"), sizeof( model ) );
		Q_strncpyz( headModel, Info_ValueForKey (userinfo, "headmodel"), sizeof( headModel ) );
		strcpy(redTeam, Info_ValueForKey( userinfo, "legsskin" ));
	}
	
	if ( ent->r.svFlags & SVF_BOT ) {
		if( g_gametype.integer >= GT_TEAM && g_ffa_gt==0) {
			strcpy(redTeam, Info_ValueForKey( userinfo, "team_model" ));
		} else {
			strcpy(redTeam, Info_ValueForKey( userinfo, "model" ));
		}	
	}

	// bots set their team a few frames later
	if (g_gametype.integer >= GT_TEAM && g_ffa_gt==0 && g_entities[clientNum].r.svFlags & SVF_BOT) {
		s = Info_ValueForKey( userinfo, "team" );
		if ( !Q_stricmp( s, "red" ) || !Q_stricmp( s, "r" ) ) {
			team = TEAM_RED;
		} else if ( !Q_stricmp( s, "blue" ) || !Q_stricmp( s, "b" ) ) {
			team = TEAM_BLUE;
		} else {
			// pick the team with the least number of players
			team = PickTeam( clientNum );
		}
                client->sess.sessionTeam = team;
	}
	else {
		team = client->sess.sessionTeam;
	}

	if (g_gametype.integer >= GT_TEAM && g_ffa_gt!=1) {
		client->pers.teamInfo = qtrue;
	} else {
		s = Info_ValueForKey( userinfo, "teamoverlay" );
		if ( ! *s || atoi( s ) != 0 ) {
			client->pers.teamInfo = qtrue;
		} else {
			client->pers.teamInfo = qfalse;
		}
	}

	// team task (0 = none, 1 = offence, 2 = defence)
	teamTask = atoi(Info_ValueForKey(userinfo, "teamtask"));
	// team Leader (1 = leader, 0 is normal player)
	teamLeader = client->sess.teamLeader;

	// colors
        if( g_gametype.integer >= GT_TEAM && g_ffa_gt==0 ) {
            switch(team) {
                case TEAM_RED:
                    c1[0] = COLOR_BLUE;
                    c1[1] = 0;
                    break;
                case TEAM_BLUE:
                    c1[0] = COLOR_RED;
                    c1[1] = 0;
                    break;
                default:
                    break;
            }
        } else {
            strcpy(c1, Info_ValueForKey( userinfo, "color1" ));
        }

	strcpy(pligred, Info_ValueForKey( userinfo, "cg_plightred" ));
	strcpy(pliggreen, Info_ValueForKey( userinfo, "cg_plightgreen" ));
	strcpy(pligblue, Info_ValueForKey( userinfo, "cg_plightblue" ));
	strcpy(toligred, Info_ValueForKey( userinfo, "cg_tolightred" ));
	strcpy(toliggreen, Info_ValueForKey( userinfo, "cg_tolightgreen" ));
	strcpy(toligblue, Info_ValueForKey( userinfo, "cg_tolightblue" ));
	strcpy(heligred, Info_ValueForKey( userinfo, "cg_helightred" ));
	strcpy(heliggreen, Info_ValueForKey( userinfo, "cg_helightgreen" ));
	strcpy(heligblue, Info_ValueForKey( userinfo, "cg_helightblue" ));
	strcpy(pgred, Info_ValueForKey( userinfo, "cg_crosshairColorRed" ));
	strcpy(pggreen, Info_ValueForKey( userinfo, "cg_crosshairColorGreen" ));
	strcpy(pgblue, Info_ValueForKey( userinfo, "cg_crosshairColorBlue" ));
	strcpy(swep_id, va("%i", ent->swep_id));
	strcpy(blueTeam, Info_ValueForKey( userinfo, "g_blueteam" ));

	if ( ent->r.svFlags & SVF_BOT ) {
		s = va("n\\%s\\t\\%i\\model\\%s\\hmodel\\%s\\g_redteam\\%s\\g_blueteam\\%s\\si\\%s\\vn\\%i\\c1\\%s\\hc\\%i\\w\\%i\\l\\%i\\isnpc\\%i\\skill\\%s\\tt\\%d\\tl\\%d",
			client->pers.netname, team, model, headModel, redTeam, blueTeam, swep_id, client->vehiclenum, c1,
			client->pers.maxHealth, client->sess.wins, client->sess.losses, ent->singlebot,
			Info_ValueForKey( userinfo, "skill" ), teamTask, teamLeader );
	} else {
		s = va("n\\%s\\t\\%i\\model\\%s\\hmodel\\%s\\g_redteam\\%s\\g_blueteam\\%s\\hr\\%s\\hg\\%s\\hb\\%s\\tr\\%s\\tg\\%s\\tb\\%s\\pr\\%s\\pg\\%s\\pb\\%s\\pg_r\\%s\\pg_g\\%s\\pg_b\\%s\\si\\%s\\vn\\%i\\c1\\%s\\hc\\%i\\w\\%i\\l\\%i\\tt\\%d\\tl\\%d\\flashl\\%i",
			client->pers.netname, client->sess.sessionTeam, model, headModel, redTeam, blueTeam, heligred, heliggreen, heligblue, toligred, toliggreen, toligblue, pligred, pliggreen, pligblue, pgred, pggreen, pgblue, swep_id, client->vehiclenum, c1,
			client->pers.maxHealth, client->sess.wins, client->sess.losses, teamTask, teamLeader, ent->flashon);
	}

	trap_SetConfigstring( CS_PLAYERS+clientNum, s );

	G_LogPrintf( "ClientUserinfoChanged: %i %s\\id\\%s\n", clientNum, s, Info_ValueForKey(userinfo, "cl_guid") );
}

/*
===========
ClientConnect

Called when a player begins connecting to the server.
Called again for every map change or tournement restart.

The session information will be valid after exit.

Return NULL if the client should be allowed, otherwise return
a string with the reason for denial.

Otherwise, the client will be sent the current gamestate
and will eventually get to ClientBegin.

firstTime will be qtrue the very first time a client connects
to the server machine, but qfalse on map changes and tournement
restarts.
============
*/
char *ClientConnect( int clientNum, qboolean firstTime, qboolean isBot ) {
	char		*value;
	gclient_t	*client;
	char		userinfo[MAX_INFO_STRING];
	gentity_t	*ent;

	/*if ( !isBot && level.player )
		return "Server is running a single player gametype.";*/

    //KK-OAX I moved these up so userinfo could be assigned/used.
	ent = &g_entities[ clientNum ];
	client = &level.clients[ clientNum ];
	ent->client = client;
	memset( client, 0, sizeof(*client) );

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	value = Info_ValueForKey (userinfo, "ip");
    //Check for local client
    if( !strcmp( value, "localhost" ) )
        client->pers.localClient = qtrue;

	if ( G_FilterPacket( value ) && !Q_stricmp(value,"localhost") ) {
            G_Printf("Player with IP: %s is banned\n",value);
		return "You are banned from this server.";
	}

	if ( !isBot && (strcmp(value, "localhost") != 0)) {
		// check for a password
		value = Info_ValueForKey (userinfo, "password");
		if ( g_password.string[0] && Q_stricmp( g_password.string, "none" ) &&
			strcmp( g_password.string, value) != 0) {
			return "Invalid password";
		}
	}

	client->pers.connected = CON_CONNECTING;

	// read or initialize the session data
	if(g_gametype.integer != GT_SINGLE){
	if ( firstTime || level.newSession ) {
		G_InitSessionData( client, userinfo );
	}
	}
	G_ReadSessionData( client );

	if( isBot ) {
		ent->r.svFlags |= SVF_BOT;
		ent->inuse = qtrue;
		if( !G_BotConnect( clientNum, !firstTime ) ) {
			return "BotConnectfailed";
		}

		//link the bot to the target_botspawn entity. For this we abuse the parent property.
		if(ent){
		LinkBotSpawnEntity( ent, Info_ValueForKey (userinfo, "parentid") );
		}
	}

	//KK-OAX Swapped these in order...seemed to help the connection process.
	// get and distribute relevent paramters
	ClientUserinfoChanged( clientNum );
	G_LogPrintf( "ClientConnect: %i\n", clientNum );


	// don't do the "xxx connected" messages if they were caried over from previous level
	if ( firstTime && ( !ent->singlebot )) {
		trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " connected\n\"", client->pers.netname) );
	}

	if ( g_gametype.integer >= GT_TEAM &&
		client->sess.sessionTeam != TEAM_SPECTATOR ) {
		BroadcastTeamChange( client, -1 );
	}

	// count current clients and rank for scoreboard
	CalculateRanks();

	// for statistics
//	client->areabits = areabits;
//	if ( !client->areabits )
//		client->areabits = G_Alloc( (trap_AAS_PointReachabilityAreaIndex( NULL ) + 7) / 8 );
	if ( !isBot && !level.player ) {
		level.player = ent;
	}
//Sago: Changed the message
//unlagged - backward reconciliation #5
	// announce it
	if ( g_delagHitscan.integer ) {
		trap_SendServerCommand( clientNum, "print \"Full lag compensation is ON!\n\"" );
	}
	else {
		trap_SendServerCommand( clientNum, "print \"Full lag compensation is OFF!\n\"" );
	}
	trap_SendServerCommand( clientNum, "print \"QSandbox By Noire.dev\n\"" );

//unlagged - backward reconciliation #5
	return NULL;
}

void motd (gentity_t *ent)
{
	char motd[1024];
	fileHandle_t motdFile;
	int motdLen;
	int fileLen;

	strcpy (motd, "cp \"");
	fileLen = trap_FS_FOpenFile(g_motdfile.string, &motdFile, FS_READ);
	if(motdFile)
	{
		char * p;

		motdLen = strlen(motd);
		if((motdLen + fileLen) > (sizeof(motd) - 2))
			fileLen = (sizeof(motd) - 2 - motdLen);
		trap_FS_Read(motd + motdLen, fileLen, motdFile);
		motd[motdLen + fileLen] = '"';
		motd[motdLen + fileLen + 1] = 0;
		trap_FS_FCloseFile(motdFile);

		while((p = strchr(motd, '\r'))) //Remove carrier return. 0x0D
		memmove(p, p + 1, motdLen + fileLen - (p - motd));
	}
	trap_SendServerCommand(ent - g_entities, motd);
}

/*
===========
ClientBegin

called when a client has finished connecting, and is ready
to be placed into the level.  This will happen every level load,
and on transition between teams, but doesn't happen on respawns
============
*/
void ClientBegin( int clientNum ) {
	gentity_t	*ent;
	gclient_t	*client;
	gentity_t       *tent;
	int			flags;
    char		userinfo[MAX_INFO_STRING];

    trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	ent = g_entities + clientNum;

	client = level.clients + clientNum;

	if ( ent->r.linked ) {
		trap_UnlinkEntity( ent );
	}
	G_InitGentity( ent );
	ent->touch = 0;
	ent->pain = 0;
	ent->client = client;

	client->pers.connected = CON_CONNECTED;
	client->pers.enterTime = level.time;
	client->pers.teamState.state = TEAM_BEGIN;

	//Elimination:
	client->pers.roundReached = 0; //We will spawn in next round
	if(g_gametype.integer == GT_LMS) {
		client->isEliminated = qtrue; //So player does not give a point in gamemode 2 and 3
		//trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " will start dead\n\"", client->pers.netname) );
	}

	// save eflags around this, because changing teams will
	// cause this to happen with a valid entity, and we
	// want to make sure the teleport bit is set right
	// so the viewpoint doesn't interpolate through the
	// world to the new position
	flags = client->ps.eFlags;
	memset( &client->ps, 0, sizeof( client->ps ) );
        if( client->sess.sessionTeam != TEAM_SPECTATOR )
            PlayerStore_restore(Info_ValueForKey(userinfo,"cl_guid"),&(client->ps));
	client->ps.eFlags = flags;

	// locate ent at a spawn point
	ClientSpawn( ent );

	if( ( client->sess.sessionTeam != TEAM_SPECTATOR ) &&
		( ( !( client->isEliminated ) /*&&
		( ( !client->ps.pm_type ) == PM_SPECTATOR ) */ ) || //Sago: Yes, it made no sense
		( ( g_gametype.integer != GT_ELIMINATION || level.intermissiontime) &&
		( g_gametype.integer != GT_CTF_ELIMINATION || level.intermissiontime) &&
		( g_gametype.integer != GT_LMS || level.intermissiontime ) ) ) ) {
		// send event
		tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_IN );
		tent->s.clientNum = ent->s.clientNum;

		if ( g_gametype.integer != GT_TOURNAMENT && !ent->singlebot ) {
			trap_SendServerCommand( -1, va("print \"%s" S_COLOR_WHITE " entered the game\n\"", client->pers.netname) );
		}
	}

    motd ( ent );
	// set info that persisted after mapchange
	if (!IsBot(ent)) {
		G_Sav_LoadData(ent, 0);
	}
	G_LogPrintf( "ClientBegin: %i\n", clientNum );

	//Send domination point names:
	if(g_gametype.integer == GT_DOMINATION) {
		DominationPointNamesMessage(ent);
		DominationPointStatusMessage(ent);
	}
	// count current clients and rank for scoreboard
	CalculateRanks();
	G_SendWeaponProperties( ent );
}
/*
===========
ClientSpawn

Called every time a client is placed fresh in the world:
after the first ClientBegin, and after each respawn
Initializes all non-persistant parts of playerState
============
*/
void ClientSpawn(gentity_t *ent) {
	int		index;
	vec3_t	spawn_origin, spawn_angles;
	gclient_t	*client;
	int		i;
	clientPersistant_t	saved;
	clientSession_t		savedSess;
	int		persistant[MAX_PERSISTANT];
	gentity_t	*spawnPoint;
	int		flags;
	int		health;
	int		savedPing;
	int		accuracy_hits, accuracy_shots,vote;
	int		eventSequence;
	char	userinfo[MAX_INFO_STRING];

	index = ent - g_entities;
	client = ent->client;

	if(g_gametype.integer == GT_SINGLE){
	if ( !IsBot( ent ) )
		G_FadeIn( 1.0, ent-g_entities );
	}

	//In Elimination the player should not spawn if he have already spawned in the round (but not for spectators)
	// N_G: You've obviously wanted something ELSE
	//Sago: Yes, the !level.intermissiontime is currently redundant but it might still be the bast place to make the test, CheckElimination in g_main makes sure the next if will fail and the rest of the things this block does might not affect if in Intermission (I'll just test that)
	if(
	(
		(
			g_gametype.integer == GT_ELIMINATION ||
			g_gametype.integer == GT_CTF_ELIMINATION || (g_gametype.integer == GT_LMS && client->isEliminated)) &&
			(!level.intermissiontime || level.warmupTime != 0)
		) &&
		( client->sess.sessionTeam != TEAM_SPECTATOR )
	)
	{
		// N_G: Another condition that makes no sense to me, see for
		// yourself if you really meant this
		// Sago: I beleive the TeamCount is to make sure people can join even if the game can't start
		if( ( level.roundNumber == level.roundNumberStarted ) ||
			( (level.time < level.roundStartTime - g_elimination_activewarmup.integer*1000 ) &&
			TeamCount( -1, TEAM_BLUE ) &&
			TeamCount( -1, TEAM_RED )  ) )
		{
		if	(client->sess.sessionTeam == TEAM_BLUE) {
			if(eliminationrespawn.integer == 0){
			client->sess.spectatorState = SPECTATOR_FREE;
			client->isEliminated = qtrue;
                        if(g_gametype.integer == GT_LMS)
                                G_LogPrintf( "LMS: %i %i %i: Player \"%s^7\" eliminated!\n", level.roundNumber, index, 1, client->pers.netname );
			client->ps.pm_type = PM_SPECTATOR;
			}
		}
		if	(client->sess.sessionTeam == TEAM_RED) {
			if(eliminationredrespawn.integer == 0){
			client->sess.spectatorState = SPECTATOR_FREE;
			client->isEliminated = qtrue;
                        if(g_gametype.integer == GT_LMS)
                                G_LogPrintf( "LMS: %i %i %i: Player \"%s^7\" eliminated!\n", level.roundNumber, index, 1, client->pers.netname );
			client->ps.pm_type = PM_SPECTATOR;
			}
		}


			if	(client->sess.sessionTeam == TEAM_BLUE) {
			if(eliminationrespawn.integer == 1){
				// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	if ( ent->botspawn ) {
		spawnPoint = ent->botspawn;
		VectorCopy( spawnPoint->s.origin, spawn_origin );
		VectorCopy(  spawnPoint->s.angles, spawn_angles );
	} else {
	if ((client->sess.sessionTeam == TEAM_SPECTATOR)
			|| ( (client->ps.pm_type == PM_SPECTATOR || client->isEliminated )  && (g_gametype.integer == GT_ELIMINATION || g_gametype.integer == GT_CTF_ELIMINATION) ) ) {
		spawnPoint = SelectSpectatorSpawnPoint ( spawn_origin, spawn_angles);
	} else if (g_gametype.integer == GT_DOUBLE_D) {
		//Double Domination uses special spawn points:
		spawnPoint = SelectDoubleDominationSpawnPoint (client->sess.sessionTeam, spawn_origin, spawn_angles);
	} else if (g_gametype.integer >= GT_CTF && g_ffa_gt==0 && g_gametype.integer!= GT_DOMINATION) {
		// all base oriented team games use the CTF spawn points
		spawnPoint = SelectCTFSpawnPoint (
						client->sess.sessionTeam,
						client->pers.teamState.state,
						spawn_origin, spawn_angles);
	} else {
		do {
			// the first spawn should be at a good looking spot
			if ( !client->pers.initialSpawn && client->pers.localClient ) {
				client->pers.initialSpawn = qtrue;
				spawnPoint = SelectInitialSpawnPoint( spawn_origin, spawn_angles );
			} else {
				// don't spawn near existing origin if possible
				spawnPoint = SelectSpawnPoint (
					client->ps.origin,
					spawn_origin, spawn_angles);
			}

			// Tim needs to prevent bots from spawning at the initial point
			// on q3dm0...
			if ( ( spawnPoint->flags & FL_NO_BOTS ) && ( ent->r.svFlags & SVF_BOT ) ) {
                            //Sago: The game has a tendency to select the furtest spawn point
                            //This is a problem if the fursest spawnpoint keeps being NO_BOTS and it does!
                            //This is a hot fix that seeks a spawn point faraway from the the currently found one
                            vec3_t old_origin;
                            VectorCopy(spawn_origin,old_origin);
                            spawnPoint = SelectSpawnPoint (old_origin, spawn_origin, spawn_angles);
                            if ( ( spawnPoint->flags & FL_NO_BOTS ) && ( ent->r.svFlags & SVF_BOT ) ) {
				continue;	// try again
                            }
			}
			// just to be symetric, we have a nohumans option...
			if ( ( spawnPoint->flags & FL_NO_HUMANS ) && !( ent->r.svFlags & SVF_BOT ) ) {
				continue;	// try again
			}

			break;

		} while ( 1 );
	}
	}
	client->pers.teamState.state = TEAM_ACTIVE;

	// always clear the kamikaze flag
	ent->s.eFlags &= ~EF_KAMIKAZE;

	// toggle the teleport bit so the client knows to not lerp
	// and never clear the voted flag
	flags = ent->client->ps.eFlags & (EF_TELEPORT_BIT | EF_VOTED | EF_TEAMVOTED);
	flags ^= EF_TELEPORT_BIT;

//unlagged - backward reconciliation #3
	// we don't want players being backward-reconciled to the place they died
	G_ResetHistory( ent );
	// and this is as good a time as any to clear the saved state
	ent->client->saved.leveltime = 0;
//unlagged - backward reconciliation #3

	// clear everything but the persistant data

	saved = client->pers;
	savedSess = client->sess;
	savedPing = client->ps.ping;
	vote = client->vote;
//	savedAreaBits = client->areabits;
	accuracy_hits = client->accuracy_hits;
	accuracy_shots = client->accuracy_shots;

    memcpy(persistant,client->ps.persistant,MAX_PERSISTANT*sizeof(int));
	eventSequence = client->ps.eventSequence;

	Com_Memset (client, 0, sizeof(*client));

	client->pers = saved;
	client->sess = savedSess;
	client->ps.ping = savedPing;
	client->vote = vote;
//	client->areabits = savedAreaBits;
	client->accuracy_hits = accuracy_hits;
	client->accuracy_shots = accuracy_shots;

	client->lastkilled_client = -1;

	for ( i = 0 ; i < MAX_PERSISTANT ; i++ ) {
		client->ps.persistant[i] = persistant[i];
	}
	client->ps.eventSequence = eventSequence;
	// increment the spawncount so the client will detect the respawn
	client->ps.persistant[PERS_SPAWN_COUNT]++;
	client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;

	client->airOutTime = level.time + 12000;

	trap_GetUserinfo( index, userinfo, sizeof(userinfo) );
	// set max health
	health = 100;
	client->pers.maxHealth = health;
	if (!(ent->r.svFlags & SVF_BOT)){
		if ( client->pers.maxHealth < 1 || client->pers.maxHealth > 100 ) {
			client->pers.maxHealth = 100;
		}
	}

	// clear entity values
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
	client->ps.eFlags = flags;

	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->client = &level.clients[index];
	ent->takedamage = qtrue;
	ent->inuse = qtrue;
	ent->classname = "player";
	ent->r.contents = CONTENTS_BODY;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags = 0;

    //Sago: No one has hit the client yet!
    client->lastSentFlying = -1;
	VectorCopy (playerMins, ent->r.mins);
	VectorCopy (playerMaxs, ent->r.maxs);

	client->ps.clientNum = index;

	G_SetOrigin( ent, spawn_origin );
	VectorCopy( spawn_origin, client->ps.origin );

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;
	if(g_gametype.integer==GT_ELIMINATION || g_gametype.integer==GT_CTF_ELIMINATION || g_gametype.integer==GT_LMS)
		client->ps.pm_flags &= ~PMF_ELIMWARMUP;

	trap_GetUsercmd( client - level.clients, &ent->client->pers.cmd );
	SetClientViewAngle( ent, spawn_angles );

	if ( !ent->botspawn ) { G_KillBox( ent );}
	trap_LinkEntity (ent);

	client->ps.weaponstate = WEAPON_READY;
	for(i = 1 ; i < WEAPONS_NUM; i++){
		ent->swep_list[i] = 0;
		ent->swep_ammo[i] = 0;
	}
	SetUnlimitedWeapons(ent);
	SetSandboxWeapons(ent);
	if ( ent->botspawn ) {
		SetupCustomBot( ent );
	} else {
		SetCustomWeapons( ent );
	}
	ent->client->isEliminated = qfalse;

	// don't allow full run speed for a bit
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

	client->respawnTime = level.time;
	client->inactivityTime = level.time + g_inactivity.integer * 1000;
	client->latched_buttons = 0;

	// set default animations
	client->ps.torsoAnim = TORSO_STAND;
	client->ps.legsAnim = LEGS_IDLE;

	if ( level.intermissiontime ) {
		MoveClientToIntermission( ent );
	} else {
		// fire the targets of the spawn point
		G_UseTargets( spawnPoint, ent );
	}

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	client->ps.commandTime = level.time - 100;
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink( ent-g_entities );

	// positively link the client, even if the command times are weird
	if ( (ent->client->sess.sessionTeam != TEAM_SPECTATOR) || ( (!client->isEliminated || client->ps.pm_type != PM_SPECTATOR)&&
		(g_gametype.integer == GT_ELIMINATION || g_gametype.integer == GT_CTF_ELIMINATION || g_gametype.integer == GT_LMS) ) ) {
		BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );
		VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );
		trap_LinkEntity( ent );
	}

	// run the presend to set anything else
	ClientEndFrame( ent );

	// clear entity state values
	BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );

    if(g_spawnprotect.integer)
        client->spawnprotected = qtrue;

    RespawnTimeMessage(ent,0);

		client->ps.weaponstate = WEAPON_READY;
		ent->client->isEliminated = qfalse;
		return;
			}
			}
			if	(client->sess.sessionTeam == TEAM_RED) {
			if(eliminationredrespawn.integer == 1){
				// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	if ( ent->botspawn ) {
		spawnPoint = ent->botspawn;
		VectorCopy( spawnPoint->s.origin, spawn_origin );
		VectorCopy(  spawnPoint->s.angles, spawn_angles );
	} else {
	if ((client->sess.sessionTeam == TEAM_SPECTATOR)
			|| ( (client->ps.pm_type == PM_SPECTATOR || client->isEliminated )  && (g_gametype.integer == GT_ELIMINATION || g_gametype.integer == GT_CTF_ELIMINATION) ) ) {
		spawnPoint = SelectSpectatorSpawnPoint ( spawn_origin, spawn_angles);
	} else if (g_gametype.integer == GT_DOUBLE_D) {
		//Double Domination uses special spawn points:
		spawnPoint = SelectDoubleDominationSpawnPoint (client->sess.sessionTeam, spawn_origin, spawn_angles);
	} else if (g_gametype.integer >= GT_CTF && g_ffa_gt==0 && g_gametype.integer!= GT_DOMINATION) {
		// all base oriented team games use the CTF spawn points
		spawnPoint = SelectCTFSpawnPoint (
						client->sess.sessionTeam,
						client->pers.teamState.state,
						spawn_origin, spawn_angles);
	} else {
		do {
			// the first spawn should be at a good looking spot
			if ( !client->pers.initialSpawn && client->pers.localClient ) {
				client->pers.initialSpawn = qtrue;
				spawnPoint = SelectInitialSpawnPoint( spawn_origin, spawn_angles );
			} else {
				// don't spawn near existing origin if possible
				spawnPoint = SelectSpawnPoint (
					client->ps.origin,
					spawn_origin, spawn_angles);
			}

			// Tim needs to prevent bots from spawning at the initial point
			// on q3dm0...
			if ( ( spawnPoint->flags & FL_NO_BOTS ) && ( ent->r.svFlags & SVF_BOT ) ) {
                            //Sago: The game has a tendency to select the furtest spawn point
                            //This is a problem if the fursest spawnpoint keeps being NO_BOTS and it does!
                            //This is a hot fix that seeks a spawn point faraway from the the currently found one
                            vec3_t old_origin;
                            VectorCopy(spawn_origin,old_origin);
                            spawnPoint = SelectSpawnPoint (old_origin, spawn_origin, spawn_angles);
                            if ( ( spawnPoint->flags & FL_NO_BOTS ) && ( ent->r.svFlags & SVF_BOT ) ) {
				continue;	// try again
                            }
			}
			// just to be symetric, we have a nohumans option...
			if ( ( spawnPoint->flags & FL_NO_HUMANS ) && !( ent->r.svFlags & SVF_BOT ) ) {
				continue;	// try again
			}

			break;

		} while ( 1 );
	}
	}
	client->pers.teamState.state = TEAM_ACTIVE;

	// always clear the kamikaze flag
	ent->s.eFlags &= ~EF_KAMIKAZE;

	// toggle the teleport bit so the client knows to not lerp
	// and never clear the voted flag
	flags = ent->client->ps.eFlags & (EF_TELEPORT_BIT | EF_VOTED | EF_TEAMVOTED);
	flags ^= EF_TELEPORT_BIT;

//unlagged - backward reconciliation #3
	// we don't want players being backward-reconciled to the place they died
	G_ResetHistory( ent );
	// and this is as good a time as any to clear the saved state
	ent->client->saved.leveltime = 0;
//unlagged - backward reconciliation #3

	// clear everything but the persistant data

	saved = client->pers;
	savedSess = client->sess;
	savedPing = client->ps.ping;
	vote = client->vote;
//	savedAreaBits = client->areabits;
	accuracy_hits = client->accuracy_hits;
	accuracy_shots = client->accuracy_shots;

    memcpy(persistant,client->ps.persistant,MAX_PERSISTANT*sizeof(int));
	eventSequence = client->ps.eventSequence;

	Com_Memset (client, 0, sizeof(*client));

	client->pers = saved;
	client->sess = savedSess;
	client->ps.ping = savedPing;
	client->vote = vote;
//	client->areabits = savedAreaBits;
	client->accuracy_hits = accuracy_hits;
	client->accuracy_shots = accuracy_shots;

	client->lastkilled_client = -1;

	for ( i = 0 ; i < MAX_PERSISTANT ; i++ ) {
		client->ps.persistant[i] = persistant[i];
	}
	client->ps.eventSequence = eventSequence;
	// increment the spawncount so the client will detect the respawn
	client->ps.persistant[PERS_SPAWN_COUNT]++;
	client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;

	client->airOutTime = level.time + 12000;

	trap_GetUserinfo( index, userinfo, sizeof(userinfo) );
	// set max health
	health = 100;
	client->pers.maxHealth = health;
	if (!(ent->r.svFlags & SVF_BOT)){
		if ( client->pers.maxHealth < 1 || client->pers.maxHealth > 100 ) {
			client->pers.maxHealth = 100;
		}
	}

	// clear entity values
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
	client->ps.eFlags = flags;

	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->client = &level.clients[index];
	ent->takedamage = qtrue;
	ent->inuse = qtrue;
	ent->classname = "player";
	ent->r.contents = CONTENTS_BODY;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags = 0;

    //Sago: No one has hit the client yet!
    client->lastSentFlying = -1;

	VectorCopy (playerMins, ent->r.mins);
	VectorCopy (playerMaxs, ent->r.maxs);

	client->ps.clientNum = index;

	G_SetOrigin( ent, spawn_origin );
	VectorCopy( spawn_origin, client->ps.origin );

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;
	if(g_gametype.integer==GT_ELIMINATION || g_gametype.integer==GT_CTF_ELIMINATION || g_gametype.integer==GT_LMS)
		client->ps.pm_flags &= ~PMF_ELIMWARMUP;

	trap_GetUsercmd( client - level.clients, &ent->client->pers.cmd );
	SetClientViewAngle( ent, spawn_angles );

	if ( !ent->botspawn ) { G_KillBox( ent );}
	trap_LinkEntity (ent);

	client->ps.weaponstate = WEAPON_READY;
	for(i = 1 ; i < WEAPONS_NUM; i++){
		ent->swep_list[i] = 0;
		ent->swep_ammo[i] = 0;
	}
	SetUnlimitedWeapons(ent);
	SetSandboxWeapons(ent);
	if ( ent->botspawn ) {
		SetupCustomBot( ent );
	} else {
		SetCustomWeapons( ent );
	}
	ent->client->isEliminated = qfalse;

	// don't allow full run speed for a bit
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

	client->respawnTime = level.time;
	client->inactivityTime = level.time + g_inactivity.integer * 1000;
	client->latched_buttons = 0;

	// set default animations
	client->ps.torsoAnim = TORSO_STAND;
	client->ps.legsAnim = LEGS_IDLE;

	if ( level.intermissiontime ) {
		MoveClientToIntermission( ent );
	} else {
		// fire the targets of the spawn point
		G_UseTargets( spawnPoint, ent );
	}

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	client->ps.commandTime = level.time - 100;
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink( ent-g_entities );

	// positively link the client, even if the command times are weird
	if ( (ent->client->sess.sessionTeam != TEAM_SPECTATOR) || ( (!client->isEliminated || client->ps.pm_type != PM_SPECTATOR)&&
		(g_gametype.integer == GT_ELIMINATION || g_gametype.integer == GT_CTF_ELIMINATION || g_gametype.integer == GT_LMS) ) ) {
		BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );
		VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );
		trap_LinkEntity( ent );
	}

	// run the presend to set anything else
	ClientEndFrame( ent );

	// clear entity state values
	BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );

    if(g_spawnprotect.integer)
        client->spawnprotected = qtrue;

    RespawnTimeMessage(ent,0);

		client->ps.weaponstate = WEAPON_READY;
		ent->client->isEliminated = qfalse;
		return;
			}
			}

                        CalculateRanks();
			return;
		}
		else
		{
			client->pers.roundReached = level.roundNumber+1;
			client->sess.spectatorState = SPECTATOR_NOT;
			client->ps.pm_type = PM_NORMAL;
			client->isEliminated = qfalse;
                        CalculateRanks();
		}
	} else {
            //Force false.
            if(client->isEliminated) {
                client->isEliminated = qfalse;
                CalculateRanks();
            }
        }

	if(g_gametype.integer == GT_LMS && client->sess.sessionTeam != TEAM_SPECTATOR && (!level.intermissiontime || level.warmupTime != 0))
	{
		if(level.roundNumber==level.roundNumberStarted /*|| level.time<level.roundStartTime-g_elimination_activewarmup.integer*1000*/ && 1>client->pers.livesLeft)
		{
			client->sess.spectatorState = SPECTATOR_FREE;
			if( ent->client->isEliminated!=qtrue) {
				client->isEliminated = qtrue;
				if((g_lms_mode.integer == 2 || g_lms_mode.integer == 3) && level.roundNumber == level.roundNumberStarted)
					LMSpoint();
                                G_LogPrintf( "LMS: %i %i %i: Player \"%s^7\" eliminated!\n", level.roundNumber, index, 1, client->pers.netname );
			}
			client->ps.pm_type = PM_SPECTATOR;
			return;
		}

		client->sess.spectatorState = SPECTATOR_NOT;
		client->ps.pm_type = PM_NORMAL;
		client->isEliminated = qfalse;
		if(client->pers.livesLeft>0)
			client->pers.livesLeft--;
	}

	// find a spawn point
	// do it before setting health back up, so farthest
	// ranging doesn't count this client
	if ( ent->botspawn ) {
		spawnPoint = ent->botspawn;
		VectorCopy( spawnPoint->s.origin, spawn_origin );
		VectorCopy(  spawnPoint->s.angles, spawn_angles );
	} else {
	if ((client->sess.sessionTeam == TEAM_SPECTATOR)
			|| ( (client->ps.pm_type == PM_SPECTATOR || client->isEliminated )  && (g_gametype.integer == GT_ELIMINATION || g_gametype.integer == GT_CTF_ELIMINATION) ) ) {
		spawnPoint = SelectSpectatorSpawnPoint ( spawn_origin, spawn_angles);
	} else if (g_gametype.integer == GT_DOUBLE_D) {
		//Double Domination uses special spawn points:
		spawnPoint = SelectDoubleDominationSpawnPoint (client->sess.sessionTeam, spawn_origin, spawn_angles);
	} else if (g_gametype.integer >= GT_CTF && g_ffa_gt==0 && g_gametype.integer!= GT_DOMINATION) {
		// all base oriented team games use the CTF spawn points
		spawnPoint = SelectCTFSpawnPoint (
						client->sess.sessionTeam,
						client->pers.teamState.state,
						spawn_origin, spawn_angles);
	} else {
		do {
			// the first spawn should be at a good looking spot
			if ( !client->pers.initialSpawn && client->pers.localClient ) {
				client->pers.initialSpawn = qtrue;
				spawnPoint = SelectInitialSpawnPoint( spawn_origin, spawn_angles );
			} else {
				// don't spawn near existing origin if possible
				spawnPoint = SelectSpawnPoint (
					client->ps.origin,
					spawn_origin, spawn_angles);
			}

			// Tim needs to prevent bots from spawning at the initial point
			// on q3dm0...
			if ( ( spawnPoint->flags & FL_NO_BOTS ) && ( ent->r.svFlags & SVF_BOT ) ) {
                            //Sago: The game has a tendency to select the furtest spawn point
                            //This is a problem if the fursest spawnpoint keeps being NO_BOTS and it does!
                            //This is a hot fix that seeks a spawn point faraway from the the currently found one
                            vec3_t old_origin;
                            VectorCopy(spawn_origin,old_origin);
                            spawnPoint = SelectSpawnPoint (old_origin, spawn_origin, spawn_angles);
                            if ( ( spawnPoint->flags & FL_NO_BOTS ) && ( ent->r.svFlags & SVF_BOT ) ) {
				continue;	// try again
                            }
			}
			// just to be symetric, we have a nohumans option...
			if ( ( spawnPoint->flags & FL_NO_HUMANS ) && !( ent->r.svFlags & SVF_BOT ) ) {
				continue;	// try again
			}

			break;

		} while ( 1 );
	}
	}
	client->pers.teamState.state = TEAM_ACTIVE;

	// always clear the kamikaze flag
	ent->s.eFlags &= ~EF_KAMIKAZE;

	// toggle the teleport bit so the client knows to not lerp
	// and never clear the voted flag
	flags = ent->client->ps.eFlags & (EF_TELEPORT_BIT | EF_VOTED | EF_TEAMVOTED);
	flags ^= EF_TELEPORT_BIT;

//unlagged - backward reconciliation #3
	// we don't want players being backward-reconciled to the place they died
	G_ResetHistory( ent );
	// and this is as good a time as any to clear the saved state
	ent->client->saved.leveltime = 0;
//unlagged - backward reconciliation #3

	// clear everything but the persistant data

	saved = client->pers;
	savedSess = client->sess;
	savedPing = client->ps.ping;
	vote = client->vote;
//	savedAreaBits = client->areabits;
	accuracy_hits = client->accuracy_hits;
	accuracy_shots = client->accuracy_shots;

    memcpy(persistant,client->ps.persistant,MAX_PERSISTANT*sizeof(int));
	eventSequence = client->ps.eventSequence;

	Com_Memset (client, 0, sizeof(*client));

	client->pers = saved;
	client->sess = savedSess;
	client->ps.ping = savedPing;
	client->vote = vote;
//	client->areabits = savedAreaBits;
	client->accuracy_hits = accuracy_hits;
	client->accuracy_shots = accuracy_shots;

	client->lastkilled_client = -1;

	for ( i = 0 ; i < MAX_PERSISTANT ; i++ ) {
		client->ps.persistant[i] = persistant[i];
	}
	client->ps.eventSequence = eventSequence;
	// increment the spawncount so the client will detect the respawn
	client->ps.persistant[PERS_SPAWN_COUNT]++;
	client->ps.persistant[PERS_TEAM] = client->sess.sessionTeam;

	client->airOutTime = level.time + 12000;

	trap_GetUserinfo( index, userinfo, sizeof(userinfo) );
	// set max health
	health = 100;
	client->pers.maxHealth = health;
	if (!(ent->r.svFlags & SVF_BOT)){
		if ( client->pers.maxHealth < 1 || client->pers.maxHealth > 100 ) {
			client->pers.maxHealth = 100;
		}
	}

	// clear entity values
	client->ps.stats[STAT_MAX_HEALTH] = client->pers.maxHealth;
	client->ps.eFlags = flags;

	ent->s.groundEntityNum = ENTITYNUM_NONE;
	ent->client = &level.clients[index];
	ent->takedamage = qtrue;
	ent->inuse = qtrue;
	ent->classname = "player";
	ent->r.contents = CONTENTS_BODY;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags = 0;

	//Sago: No one has hit the client yet!
	client->lastSentFlying = -1;

	VectorCopy (playerMins, ent->r.mins);
	VectorCopy (playerMaxs, ent->r.maxs);

	client->ps.clientNum = index;

	G_SetOrigin( ent, spawn_origin );
	VectorCopy( spawn_origin, client->ps.origin );

	// the respawned flag will be cleared after the attack and jump keys come up
	client->ps.pm_flags |= PMF_RESPAWNED;
	if(g_gametype.integer==GT_ELIMINATION || g_gametype.integer==GT_CTF_ELIMINATION || g_gametype.integer==GT_LMS)
		client->ps.pm_flags |= PMF_ELIMWARMUP;

	trap_GetUsercmd( client - level.clients, &ent->client->pers.cmd );
	SetClientViewAngle( ent, spawn_angles );

	if ( (ent->client->sess.sessionTeam == TEAM_SPECTATOR) || ((client->ps.pm_type == PM_SPECTATOR || client->isEliminated) &&
		(g_gametype.integer == GT_ELIMINATION || g_gametype.integer == GT_CTF_ELIMINATION || g_gametype.integer == GT_LMS) ) ) {

	} else {
		if ( !ent->botspawn ) { G_KillBox( ent );}
		trap_LinkEntity (ent);

		client->ps.weaponstate = WEAPON_READY;
		for(i = 1 ; i < WEAPONS_NUM; i++){
			ent->swep_list[i] = 0;
			ent->swep_ammo[i] = 0;
		}
		SetUnlimitedWeapons(ent);
		SetSandboxWeapons(ent);
		if ( ent->botspawn ) {
			SetupCustomBot( ent );
		} else {
			SetCustomWeapons( ent );
		}

	}

	// don't allow full run speed for a bit
	client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
	client->ps.pm_time = 100;

	client->respawnTime = level.time;
	client->inactivityTime = level.time + g_inactivity.integer * 1000;
	client->latched_buttons = 0;

	// set default animations
	client->ps.torsoAnim = TORSO_STAND;
	client->ps.legsAnim = LEGS_IDLE;

	if ( level.intermissiontime ) {
		MoveClientToIntermission( ent );
	} else {
		// fire the targets of the spawn point
		G_UseTargets( spawnPoint, ent );
	}

	// run a client frame to drop exactly to the floor,
	// initialize animations and other things
	client->ps.commandTime = level.time - 100;
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink( ent-g_entities );

	// positively link the client, even if the command times are weird
	if ( (ent->client->sess.sessionTeam != TEAM_SPECTATOR) || ( (!client->isEliminated || client->ps.pm_type != PM_SPECTATOR)&&
		(g_gametype.integer == GT_ELIMINATION || g_gametype.integer == GT_CTF_ELIMINATION || g_gametype.integer == GT_LMS) ) ) {
		BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );
		VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );
		trap_LinkEntity( ent );
	}

	//Send spawnbuffer for remember by client
	G_SendSpawnSwepWeapons( ent );

	// run the presend to set anything else
	ClientEndFrame( ent );

	// clear entity state values
	BG_PlayerStateToEntityState( &client->ps, &ent->s, qtrue );

    if(g_spawnprotect.integer)
        client->spawnprotected = qtrue;

    RespawnTimeMessage(ent,0);
}

/*
===========
ClientDisconnect

Called when a player drops from the server.
Will not be called between levels.

This should NOT be called directly by any game logic,
call trap_DropClient(), which will call this and do
server system housekeeping.
============
*/
void ClientDisconnect( int clientNum ) {
	gentity_t	*ent;
	int			i;
        char	userinfo[MAX_INFO_STRING];

	// cleanup if we are kicking a bot that
	// hasn't spawned yet
	G_RemoveQueuedBotBegin( clientNum );

	ent = g_entities + clientNum;
	if ( !ent->client ) {
		return;
	}

    ClientLeaving( clientNum);
    trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	// stop any following clients
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( (level.clients[i].sess.sessionTeam == TEAM_SPECTATOR || level.clients[i].ps.pm_type == PM_SPECTATOR)
			&& level.clients[i].sess.spectatorState == SPECTATOR_FOLLOW
			&& level.clients[i].sess.spectatorClient == clientNum ) {
			StopFollowing( &g_entities[i] );
		}
	}

        //Is the player alive?
        i = (ent->client->ps.stats[STAT_HEALTH]>0);
        //Commit suicide!
        if ( ent->client->pers.connected == CON_CONNECTED
		&& ent->client->sess.sessionTeam != TEAM_SPECTATOR && i ) {
                //Prevent a team from loosing point because of player leaving
                int teamscore = 0;
                if(g_gametype.integer == GT_TEAM)
                    teamscore = level.teamScores[ ent->client->sess.sessionTeam ];
		// Kill him (makes sure he loses flags, etc)
		ent->flags &= ~FL_GODMODE;
		ent->client->ps.stats[STAT_HEALTH] = ent->health = 0;
		player_die (ent, ent, g_entities + ENTITYNUM_WORLD, 100000, MOD_SUICIDE);
                if(g_gametype.integer == GT_TEAM)
                    level.teamScores[ ent->client->sess.sessionTeam ] = teamscore;
	}



        if ( ent->client->pers.connected == CON_CONNECTED && ent->client->sess.sessionTeam != TEAM_SPECTATOR)
            PlayerStore_store(Info_ValueForKey(userinfo,"cl_guid"),ent->client->ps);

	G_LogPrintf( "ClientDisconnect: %i\n", clientNum );

	// if we are playing in tourney mode and losing, give a win to the other player
	if ( (g_gametype.integer == GT_TOURNAMENT )
		&& !level.intermissiontime
		&& !level.warmupTime && level.sortedClients[1] == clientNum ) {
		level.clients[ level.sortedClients[0] ].sess.wins++;
		ClientUserinfoChanged( level.sortedClients[0] );
	}

	if( g_gametype.integer == GT_TOURNAMENT &&
		ent->client->sess.sessionTeam == TEAM_FREE &&
		level.intermissiontime ) {

		trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );
		level.restarted = qtrue;
		level.changemap = NULL;
		level.intermissiontime = 0;
	}

	trap_UnlinkEntity (ent);
	ent->s.modelindex = 0;
	ent->inuse = qfalse;
	ent->classname = "disconnected";
	ent->client->pers.connected = CON_DISCONNECTED;
	ent->client->ps.persistant[PERS_TEAM] = TEAM_FREE;
	ent->client->sess.sessionTeam = TEAM_FREE;

	trap_SetConfigstring( CS_PLAYERS + clientNum, "");

	CalculateRanks();
        CountVotes();

	if ( ent->r.svFlags & SVF_BOT ) {
		BotAIShutdownClient( clientNum, qfalse );
	}
}

/*
===========
DropClientSilently

Drops a client without displaying a message about it.
See http://www.quake3world.com/forum/viewtopic.php?f=16&t=45625
============
*/
void DropClientSilently( int clientNum ) {
	if ( 0 )
		trap_DropClient( clientNum, " : removing dead bot" );
	else
		trap_DropClient( clientNum, "DR_SILENT_DROP" );
}

/*
===========
SetupCustomBot

Applies properties from the entity that spawned the bot to the bot
============
*/
void SetupCustomBot( gentity_t *bot ) {
	if ( !bot->singlebot || !(bot->botspawn) )
		return;

	//give bot weapons
	if (bot->botspawn->mtype <= 1) {
	    Set_Weapon(bot, WP_GAUNTLET, 1);
	}
    Set_Weapon(bot, bot->botspawn->mtype, 1);
	if (bot->swep_ammo[bot->botspawn->mtype] != -1){
	Set_Ammo(bot, bot->botspawn->mtype, 9999);
	}

	bot->health = bot->client->ps.stats[STAT_HEALTH] = bot->client->ps.stats[STAT_MAX_HEALTH] = bot->botspawn->health;

	//set walking behavior
	if (bot->botspawn->spawnflags & 1 || bot->botspawn->spawnflags & 2)
		bot->client->ps.pm_flags |= PMF_FORCE_WALK;

	if (bot->botspawn->spawnflags & 1 && !(bot->botspawn->spawnflags & 2))
		bot->client->ps.pm_flags |= PMF_ATTACK_RUN;

	//use targets of target_botspawn
	if ( bot->botspawn->target ) {
		G_UseTargets( bot->botspawn, bot);
	}
	
	if(g_gametype.integer == GT_SANDBOX){
	CopyAlloc(bot->target, bot->botspawn->target);
	}
}

void SetUnlimitedWeapons( gentity_t *ent ) {
	Set_Ammo(ent, WP_GAUNTLET, -1);
	Set_Ammo(ent, WP_GRAPPLING_HOOK, -1);
	Set_Ammo(ent, WP_TOOLGUN, -1);
	Set_Ammo(ent, WP_PHYSGUN, -1);
	Set_Ammo(ent, WP_GRAVITYGUN, -1);
}

void SetSandboxWeapons( gentity_t *ent ) {
	if(g_gametype.integer == GT_SANDBOX){
		if(g_allowtoolgun.integer){
		Set_Weapon(ent, WP_TOOLGUN, 1);
		}
		if(g_allowphysgun.integer){
		Set_Weapon(ent, WP_PHYSGUN, 1);
		}
		if(g_allowgravitygun.integer){
		Set_Weapon(ent, WP_GRAVITYGUN, 1);
		}
	}
}

void SetCustomWeapons( gentity_t *ent ) {
	int i;
	Set_Ammo(ent, WP_GAUNTLET, -1);
	Set_Ammo(ent, WP_GRAPPLING_HOOK, -1);
	Set_Ammo(ent, WP_TOOLGUN, -1);
	Set_Ammo(ent, WP_PHYSGUN, -1);
	Set_Ammo(ent, WP_GRAVITYGUN, -1);
	if (ent->client->sess.sessionTeam == TEAM_FREE || ent->client->sess.sessionTeam == TEAM_BLUE) {
		if (g_elimination_gauntlet.integer) {
			Set_Weapon(ent, WP_GAUNTLET, 1);
		}
		if (g_elimination_machinegun.integer > 0) {
			Set_Weapon(ent, WP_MACHINEGUN, 1);
			Set_Ammo(ent, WP_MACHINEGUN, g_elimination_machinegun.integer);
		}
		if (g_elimination_shotgun.integer > 0) {
			Set_Weapon(ent, WP_SHOTGUN, 1);
			Set_Ammo(ent, WP_SHOTGUN, g_elimination_shotgun.integer);
		}
		if (g_elimination_grenade.integer > 0) {
			Set_Weapon(ent, WP_GRENADE_LAUNCHER, 1);
			Set_Ammo(ent, WP_GRENADE_LAUNCHER, g_elimination_grenade.integer);
		}
		if (g_elimination_rocket.integer > 0) {
			Set_Weapon(ent, WP_ROCKET_LAUNCHER, 1);
			Set_Ammo(ent, WP_ROCKET_LAUNCHER, g_elimination_rocket.integer);
		}
		if (g_elimination_lightning.integer > 0) {
			Set_Weapon(ent, WP_LIGHTNING, 1);
			Set_Ammo(ent, WP_LIGHTNING, g_elimination_lightning.integer);
		}
		if (g_elimination_railgun.integer > 0) {
			Set_Weapon(ent, WP_RAILGUN, 1);
			Set_Ammo(ent, WP_RAILGUN, g_elimination_railgun.integer);
		}
		if (g_elimination_plasmagun.integer > 0) {
			Set_Weapon(ent, WP_PLASMAGUN, 1);
			Set_Ammo(ent, WP_PLASMAGUN, g_elimination_plasmagun.integer);
		}
		if (g_elimination_bfg.integer > 0) {
			Set_Weapon(ent, WP_BFG, 1);
			Set_Ammo(ent, WP_BFG, g_elimination_bfg.integer);
		}
		if (g_elimination_grapple.integer) {
			Set_Weapon(ent, WP_GRAPPLING_HOOK, 1);
		}
		if (g_elimination_nail.integer > 0) {
			Set_Weapon(ent, WP_NAILGUN, 1);
			Set_Ammo(ent, WP_NAILGUN, g_elimination_nail.integer);
		}
		if (g_elimination_mine.integer > 0) {
			Set_Weapon(ent, WP_PROX_LAUNCHER, 1);
			Set_Ammo(ent, WP_PROX_LAUNCHER, g_elimination_mine.integer);
		}
		if (g_elimination_chain.integer > 0) {
			Set_Weapon(ent, WP_CHAINGUN, 1);
			Set_Ammo(ent, WP_CHAINGUN, g_elimination_chain.integer);
		}
		if (g_elimination_flame.integer > 0) {
			Set_Weapon(ent, WP_FLAMETHROWER, 1);
			Set_Ammo(ent, WP_FLAMETHROWER, g_elimination_flame.integer);
		}
		if (g_elimination_antimatter.integer > 0) {
			Set_Weapon(ent, WP_ANTIMATTER, 1);
			Set_Ammo(ent, WP_ANTIMATTER, g_elimination_antimatter.integer);
		}
		if(g_elimination_quad.integer) {
			ent->client->ps.powerups[PW_QUAD] =  level.time - ( level.time % 1000 );
			ent->client->ps.powerups[PW_QUAD] +=  g_elimination_quad.integer * 1000;
		}
		if(g_elimination_regen.integer) {
			ent->client->ps.powerups[PW_REGEN] =  level.time - ( level.time % 1000 );
			ent->client->ps.powerups[PW_REGEN] +=  g_elimination_regen.integer * 1000;
		}
		if(g_elimination_haste.integer) {
			ent->client->ps.powerups[PW_HASTE] =  level.time - ( level.time % 1000 );
			ent->client->ps.powerups[PW_HASTE] +=  g_elimination_haste.integer * 1000;
		}
		if(g_elimination_bsuit.integer) {
			ent->client->ps.powerups[PW_BATTLESUIT] =  level.time - ( level.time % 1000 );
			ent->client->ps.powerups[PW_BATTLESUIT] +=  g_elimination_bsuit.integer * 1000;
		}
		if(g_elimination_invis.integer) {
			ent->client->ps.powerups[PW_INVIS] =  level.time - ( level.time % 1000 );
			ent->client->ps.powerups[PW_INVIS] +=  g_elimination_invis.integer * 1000;
		}
		if(g_elimination_flight.integer) {
			ent->client->ps.powerups[PW_FLIGHT] =  level.time - ( level.time % 1000 );
			ent->client->ps.powerups[PW_FLIGHT] +=  g_elimination_flight.integer * 1000;
		}
		if(g_elimination_holdable.integer == 1) {
			ent->client->ps.stats[STAT_HOLDABLE_ITEM] |= (1 << HI_TELEPORTER);
		}
		if(g_elimination_holdable.integer == 2) {
			ent->client->ps.stats[STAT_HOLDABLE_ITEM] |= (1 << HI_MEDKIT);
		}
		if(g_elimination_holdable.integer == 3) {
			ent->client->ps.stats[STAT_HOLDABLE_ITEM] |= (1 << HI_KAMIKAZE);
			ent->client->ps.eFlags |= EF_KAMIKAZE;
		}
		if(g_elimination_holdable.integer == 4) {
			ent->client->ps.stats[STAT_HOLDABLE_ITEM] |= (1 << HI_INVULNERABILITY);
		}
		if(g_elimination_holdable.integer == 5) {
			ent->client->ps.stats[STAT_HOLDABLE_ITEM] |= (1 << HI_PORTAL);
		}

		ent->health = ent->client->ps.stats[STAT_ARMOR] = g_elimination_startArmor.integer;
		ent->health = ent->client->ps.stats[STAT_HEALTH] = g_elimination_startHealth.integer;
		if(ent->botskill == 7){
			ent->health = ent->client->ps.stats[STAT_HEALTH] = 65000;
		}
	}
	if (ent->client->sess.sessionTeam == TEAM_RED) {
		if (g_eliminationred_gauntlet.integer) {
			Set_Weapon(ent, WP_GAUNTLET, 1);
		}
		if (g_eliminationred_machinegun.integer > 0) {
			Set_Weapon(ent, WP_MACHINEGUN, 1);
			Set_Ammo(ent, WP_MACHINEGUN, g_eliminationred_machinegun.integer);
		}
		if (g_eliminationred_shotgun.integer > 0) {
			Set_Weapon(ent, WP_SHOTGUN, 1);
			Set_Ammo(ent, WP_SHOTGUN, g_eliminationred_shotgun.integer);
		}
		if (g_eliminationred_grenade.integer > 0) {
			Set_Weapon(ent, WP_GRENADE_LAUNCHER, 1);
			Set_Ammo(ent, WP_GRENADE_LAUNCHER, g_eliminationred_grenade.integer);
		}
		if (g_eliminationred_rocket.integer > 0) {
			Set_Weapon(ent, WP_ROCKET_LAUNCHER, 1);
			Set_Ammo(ent, WP_ROCKET_LAUNCHER, g_eliminationred_rocket.integer);
		}
		if (g_eliminationred_lightning.integer > 0) {
			Set_Weapon(ent, WP_LIGHTNING, 1);
			Set_Ammo(ent, WP_LIGHTNING, g_eliminationred_lightning.integer);
		}
		if (g_eliminationred_railgun.integer > 0) {
			Set_Weapon(ent, WP_RAILGUN, 1);
			Set_Ammo(ent, WP_RAILGUN, g_eliminationred_railgun.integer);
		}
		if (g_eliminationred_plasmagun.integer > 0) {
			Set_Weapon(ent, WP_PLASMAGUN, 1);
			Set_Ammo(ent, WP_PLASMAGUN, g_eliminationred_plasmagun.integer);
		}
		if (g_eliminationred_bfg.integer > 0) {
			Set_Weapon(ent, WP_BFG, 1);
			Set_Ammo(ent, WP_BFG, g_eliminationred_bfg.integer);
		}
		if (g_eliminationred_grapple.integer) {
			Set_Weapon(ent, WP_GRAPPLING_HOOK, 1);
		}
		if (g_eliminationred_nail.integer > 0) {
			Set_Weapon(ent, WP_NAILGUN, 1);
			Set_Ammo(ent, WP_NAILGUN, g_eliminationred_nail.integer);
		}
		if (g_eliminationred_mine.integer > 0) {
			Set_Weapon(ent, WP_PROX_LAUNCHER, 1);
			Set_Ammo(ent, WP_PROX_LAUNCHER, g_eliminationred_mine.integer);
		}
		if (g_eliminationred_chain.integer > 0) {
			Set_Weapon(ent, WP_CHAINGUN, 1);
			Set_Ammo(ent, WP_CHAINGUN, g_eliminationred_chain.integer);
		}
		if (g_eliminationred_flame.integer > 0) {
			Set_Weapon(ent, WP_FLAMETHROWER, 1);
			Set_Ammo(ent, WP_FLAMETHROWER, g_eliminationred_flame.integer);
		}
		if (g_eliminationred_antimatter.integer > 0) {
			Set_Weapon(ent, WP_ANTIMATTER, 1);
			Set_Ammo(ent, WP_ANTIMATTER, g_eliminationred_antimatter.integer);
		}
		if(g_eliminationred_quad.integer) {
			ent->client->ps.powerups[PW_QUAD] =  level.time - ( level.time % 1000 );
			ent->client->ps.powerups[PW_QUAD] +=  g_eliminationred_quad.integer * 1000;
		}
		if(g_eliminationred_regen.integer) {
			ent->client->ps.powerups[PW_REGEN] =  level.time - ( level.time % 1000 );
			ent->client->ps.powerups[PW_REGEN] +=  g_eliminationred_regen.integer * 1000;
		}
		if(g_eliminationred_haste.integer) {
			ent->client->ps.powerups[PW_HASTE] =  level.time - ( level.time % 1000 );
			ent->client->ps.powerups[PW_HASTE] +=  g_eliminationred_haste.integer * 1000;
		}
		if(g_eliminationred_bsuit.integer) {
			ent->client->ps.powerups[PW_BATTLESUIT] =  level.time - ( level.time % 1000 );
			ent->client->ps.powerups[PW_BATTLESUIT] +=  g_eliminationred_bsuit.integer * 1000;
		}
		if(g_eliminationred_invis.integer) {
			ent->client->ps.powerups[PW_INVIS] =  level.time - ( level.time % 1000 );
			ent->client->ps.powerups[PW_INVIS] +=  g_eliminationred_invis.integer * 1000;
		}
		if(g_eliminationred_flight.integer) {
			ent->client->ps.powerups[PW_FLIGHT] =  level.time - ( level.time % 1000 );
			ent->client->ps.powerups[PW_FLIGHT] +=  g_eliminationred_flight.integer * 1000;
		}
		if(g_eliminationred_holdable.integer == 1) {
			ent->client->ps.stats[STAT_HOLDABLE_ITEM] |= (1 << HI_TELEPORTER);
		}
		if(g_eliminationred_holdable.integer == 2) {
			ent->client->ps.stats[STAT_HOLDABLE_ITEM] |= (1 << HI_MEDKIT);
		}
		if(g_eliminationred_holdable.integer == 3) {
			ent->client->ps.stats[STAT_HOLDABLE_ITEM] |= (1 << HI_KAMIKAZE);
			ent->client->ps.eFlags |= EF_KAMIKAZE;
		}
		if(g_eliminationred_holdable.integer == 4) {
			ent->client->ps.stats[STAT_HOLDABLE_ITEM] |= (1 << HI_INVULNERABILITY);
		}
		if(g_eliminationred_holdable.integer == 5) {
			ent->client->ps.stats[STAT_HOLDABLE_ITEM] |= (1 << HI_PORTAL);
		}

		ent->health = ent->client->ps.stats[STAT_ARMOR] = g_eliminationred_startArmor.integer;
		ent->health = ent->client->ps.stats[STAT_HEALTH] = g_eliminationred_startHealth.integer;
		if(ent->botskill == 7){
			ent->health = ent->client->ps.stats[STAT_HEALTH] = 65000;
		}
	}
	//Set spawnweapon
	if(g_gametype.integer == GT_SANDBOX){
		ent->swep_id = WP_PHYSGUN;
		ent->client->ps.generic2 = WP_PHYSGUN;
		ClientUserinfoChanged( ent->s.clientNum );
		return;
	} else {
		for ( i = WEAPONS_NUM; i > 1; i-- ) {
			if(ent->swep_list[i] == 1 ){
				ent->swep_id = i;
				ent->client->ps.generic2 = i;
				ClientUserinfoChanged( ent->s.clientNum );
				return;
			}
		}
		ent->swep_id = 1;
		ent->client->ps.generic2 = 1;
		ClientUserinfoChanged( ent->s.clientNum );
	}
}

/*
===========
LinkBotSpawnEntity

Links a bot to the entity that spawned it
============
*/
void LinkBotSpawnEntity( gentity_t *bot, char parentid[] ) {
	gentity_t	*t;
	int			entityNum;
	
	if ( !bot )
		return;

	if ( !parentid || !strcmp(parentid, "") )
		return;

	entityNum = atoi(parentid);

	t = NULL;
	bot->botspawn = NULL;
	while ( (t = G_Find (t, FOFS(classname), "target_botspawn")) != NULL ) {
		if ( t->s.number == entityNum ) {
			bot->botspawn = t;
		}
	}
}
