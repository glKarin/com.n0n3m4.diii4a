/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2002-2021 Q3Rally Team (Per Thormann - q3rally@gmail.com)

This file is part of q3rally source code.

q3rally source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

q3rally source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with q3rally; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "g_local.h"


/*
=================
G_TempRallyEntity

Spawns an event entity that will not be auto-removed
The origin will be snapped to save net bandwidth, so care
must be taken if the origin is right on a surface (snap towards start vector first)
=================
*/
gentity_t *G_TempRallyEntity( vec3_t origin, int event ) {
	gentity_t		*e;
	vec3_t			snapped;

	e = G_Spawn();
	e->s.eType = ET_EVENTS + event;

	e->classname = "tempEntity";
	e->eventTime = level.time;
//	e->freeAfterEvent = qtrue;

	VectorCopy( origin, snapped );
	SnapVector( snapped );		// save network bandwidth
	G_SetOrigin( e, snapped );

	// find cluster for PVS
	trap_LinkEntity( e );

	return e;
}


void G_GetPointOnCurveBetweenCheckpoints( gentity_t *start, gentity_t *end, float f, vec3_t origin )
{
	vec3_t		startHandle, endHandle;

	VectorAdd( start->s.origin2, start->s.angles2, startHandle );
	VectorMA( end->s.origin2, -1, end->s.angles2, endHandle );

	VectorScale( start->s.origin2, (1-f)*(1-f)*(1-f), origin );
	VectorMA( origin, 3*f*(1-f)*(1-f), startHandle, origin );
	VectorMA( origin, 3*f*f*(1-f), endHandle, origin );
	VectorMA( origin, f*f*f, end->s.origin2, origin );
}

void G_GetDervOnCurveBetweenCheckpoints( gentity_t *start, gentity_t *end, float f, vec3_t vec )
{
	vec3_t		startHandle, endHandle;

	VectorAdd( start->s.origin2, start->s.angles2, startHandle );
	VectorMA( end->s.origin2, -1, end->s.angles2, endHandle );

	VectorScale( start->s.origin2, -3*(1-f)*(1-f), vec );
	VectorMA( vec, -3*((-3*f+4)*f-1), startHandle, vec );
	VectorMA( vec, -3*((3*f-2)*f), endHandle, vec );
	VectorMA( vec, 3*f*f, end->s.origin2, vec );
}

void G_Get2ndDervOnCurveBetweenCheckpoints( gentity_t *start, gentity_t *end, float f, vec3_t vec )
{
	vec3_t		startHandle, endHandle;

	VectorAdd( start->s.origin2, start->s.angles2, startHandle );
	VectorMA( end->s.origin2, -1, end->s.angles2, endHandle );

	VectorScale( start->s.origin2, 6*(1-f), vec );
	VectorMA( vec, 6*(3*f-2), startHandle, vec );
	VectorMA( vec, 6*(1-3*f), endHandle, vec );
	VectorMA( vec, 6*f, end->s.origin2, vec );
}


/*
=================
G_ResetCar
=================
*/
void G_ResetCar( gentity_t *ent ) {
//	int			i;
	vec3_t		origin, end, angles;
	vec3_t		mins, maxs;
	trace_t		tr;
	gentity_t	*tent;
//	qboolean	reset;

/*
	if ( !g_cheats.integer ) {
		trap_SendServerCommand( ent-g_entities, va("print \"Cheats are not enabled on this server.\n\""));
		return;
	}
*/
	// doesnt work when player is dead
	if (ent->client->ps.pm_type == PM_DEAD)
		return;

	// wait for wheels to be off the ground for 3 seconds
/*
	reset = qfalse;
	for (i = 0; i < NUM_CAR_POINTS; i++){
		if (i < FIRST_FRAME_POINT){
			if (!ent->client->car.sPoints[i].onGroundTime) return;
			if (ent->client->car.sPoints[i].onGroundTime + 3000 > level.time) return;
		}
		// some other point is has been on the ground for at least a second
		else if (ent->client->car.sPoints[i].onGroundTime + 1000 > level.time){
			reset = qtrue;
			break;
		}
	}

	if (!reset)
		return;
*/

	// if a wheel has been on the ground in the last 1 seconds
	// or the body has not been on the ground for more than 1 second then dont reset
	if( ent->client->car.wheelOnGroundTime + 1000 > level.time ||
		ent->client->car.onGroundTime - ent->client->car.offGroundTime < 800 )
		return;
	
	tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_OUT );
	tent->s.clientNum = ent->s.clientNum;

	VectorCopy(ent->client->ps.origin, origin);
	VectorClear(angles);

	origin[2] += 128;
	angles[YAW] = ent->client->ps.viewangles[YAW];

	VectorCopy(origin, end);
	end[2] -= 1000;

	VectorSet(mins, -CAR_LENGTH/2, -CAR_LENGTH/2, -18);
	VectorSet(maxs, CAR_LENGTH/2, CAR_LENGTH/2, 18);
	trap_Trace(&tr, origin, mins, maxs, end, ent->s.clientNum, ent->clipmask);

	if (tr.fraction){
		VectorCopy(tr.endpos, ent->client->ps.origin);
		ent->client->ps.origin[2] += 5;
	}
	else
		VectorCopy(origin, ent->client->ps.origin);

	VectorCopy(angles, ent->client->ps.viewangles);

//	PM_InitializeVehicle(&ent->client->car, ent->client->ps.origin, ent->client->ps.viewangles, vec3_origin, car_frontweight_dist.value );
	ent->client->car.initializeOnNextMove = qtrue;

	ent->client->ps.eFlags ^= EF_TELEPORT_BIT;

	// set the times so that we dont get multiple car resets in a row
	ent->client->car.onGroundTime = level.time;
	ent->client->car.offGroundTime = level.time;

	tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_IN );
	tent->s.clientNum = ent->s.clientNum;

//	TeleportPlayer( ent, origin, angles );
}


void G_DropRearWeapon( gentity_t *ent ) {
	gentity_t	*drop;
	gitem_t		*item;
	int			i;

	for (i = RWP_SMOKE; i < WP_NUM_WEAPONS; i++){
		if (ent->client->ps.stats[STAT_WEAPONS] & ( 1 << i )){
			ent->client->ps.stats[STAT_WEAPONS] &= ~( 1 << i );

			if (ent->client->ps.ammo[ i ]){
				item = BG_FindItemForWeapon( i );
				drop = Drop_Item(ent, item, 0);
				drop->count = ent->client->ps.ammo[i];
				ent->client->ps.ammo[i] = 0;
			}
		}
	}
}

void CenterPrint_All( const char *s ){
	trap_SendServerCommand( -1, va("cp \"%s\n\"", s) );
}

qboolean isRallyRace( void ){
	if(g_gametype.integer == GT_RACING
		|| g_gametype.integer == GT_RACING_DM
		|| g_gametype.integer == GT_TEAM_RACING
		|| g_gametype.integer == GT_TEAM_RACING_DM){
		return qtrue;
	}

	return qfalse;
}

qboolean isRallyNonDMRace( void ){
	if(g_gametype.integer == GT_RACING
		|| g_gametype.integer == GT_TEAM_RACING){
		return qtrue;
	}

	return qfalse;
}

/*
=================
isRaceObserver

Assumes ent has a valid client
=================
*/
qboolean isRaceObserver( int clientNum ){
//	return (ent->client->finishRaceTime && ent->client->finishRaceTime + RACE_OBSERVER_DELAY < level.time);
	return g_entities[clientNum].raceObserver;
}

void G_PrintMapStats( gentity_t *player, qboolean generateArenaFile, char *longname ){
	gentity_t		*ent;
	vec3_t			start, delta, last;
	int				i, j;
	fileHandle_t	arenaFile;
	char			string[1024];
	char			serverinfo[MAX_INFO_STRING];

	float			trackLength = 0;
	float			metricTrackLength = 0;
	int				numCheckpoints = 0;
	int				numLaps = 0;
	int				numSpawnPoints = 0;
	int				numObserverSpots = 0;
	int				numWeapons = 0;
	int				numPowerups = 0;

	VectorClear(start);
	VectorClear(last);

	for(i = 0; i < MAX_GENTITIES; i++){
		ent = &g_entities[i];
		if (!ent) continue;

		if (!strcmp(ent->classname, "rally_checkpoint")){
			numCheckpoints++;

			if (ent->laps){
				numLaps = ent->laps;
			}
		}
		else if (!strcmp(ent->classname, "info_player_start") ||
			!strcmp(ent->classname, "info_player_deathmatch")){
			numSpawnPoints++;
		}
		else if (!strcmp(ent->classname, "info_observer_spot")){
			numObserverSpots++;
		}
		else if (ent->s.eType == ET_ITEM){
			if (ent->item->giType == IT_WEAPON || ent->item->giType == IT_RFWEAPON)
				numWeapons++;
			else if (ent->item->giType == IT_POWERUP)
				numPowerups++;
			
		}
	}

	// track length
	for(i = 0; i < numCheckpoints; i++){
		for(j = 0; j < MAX_GENTITIES; j++){
			ent = &g_entities[j];
			if (!ent) continue;

			if ( !strcmp(ent->classname, "rally_checkpoint") ){
				if (ent->number == 1 && i == 0){
					VectorCopy(ent->s.origin, start);
					VectorCopy(ent->s.origin, last);
					break;
				}
				else if (ent->number == i + 1){
					VectorSubtract(last, ent->s.origin, delta);
					trackLength += VectorLength(delta);
					VectorCopy(ent->s.origin, last);
					break;
				}
			}
		}
	}
	VectorSubtract(last, start, delta);
	trackLength += VectorLength(delta);

	metricTrackLength = trackLength / CP_M_2_QU / 1000.0F;
	trackLength = trackLength / CP_FT_2_QU / 5280.0F;

	trap_SendServerCommand( player->s.number, va("print \"Laps: %i\nCheckpoints: %i\nSpawn Points: %i\nObserver Spots: %i\nNumber of Weapons: %i\nNumber of Powerups: %i\nTrack Length: %.3fmi / %.3fkm\n\"", 
							numLaps,
							numCheckpoints,
							numSpawnPoints,
							numObserverSpots,
							numWeapons,
							numPowerups,
							trackLength,
							metricTrackLength));

	if (!generateArenaFile)
		return;

	// generate .arena file
/*
{
map "demobowl_01"
longname "Demolition Derby Bowl"
fraglimit 10
type "q3r_derby"
starts "10"
}
*/
	trap_GetServerinfo( serverinfo, sizeof(serverinfo) );

	if ( isRallyRace() )
		Com_sprintf( string, sizeof(string), "{\nmap \"%s\"\nlongname \"%s\"\nfraglimit %i\ntype \"q3r_racing q3r_team_racing q3r_racing_dm q3r_team_racing_dm\"\nstarts \"%i\"\nlaps \"%i\"\nlength \"%.3f miles\"\ncheckpoints \"%i\"\nobserverspots \"%i\"\nweapons \"%i\"\npowerups \"%i\"\n}\n", Info_ValueForKey( serverinfo, "mapname" ), longname, level.numberOfLaps, numSpawnPoints, numLaps, trackLength, numCheckpoints, numObserverSpots, numWeapons, numPowerups);
	else if ( g_gametype.integer == GT_DERBY )
		Com_sprintf( string, sizeof(string), "{\nmap \"%s\"\nlongname \"%s\"\nfraglimit %i\ntype \"q3r_derby\"\nstarts \"%i\"\nobserverspots \"%i\"\nweapons \"%i\"\npowerups \"%i\"\n}\n", Info_ValueForKey( serverinfo, "mapname" ), longname, g_fraglimit.integer, numSpawnPoints, numObserverSpots, numWeapons, numPowerups);
	else if ( g_gametype.integer == GT_LCS )
		Com_sprintf( string, sizeof(string), "{\nmap \"%s\"\nlongname \"%s\"\nfraglimit %i\ntype \"q3r_lcs\"\nstarts \"%i\"\nobserverspots \"%i\"\nweapons \"%i\"\npowerups \"%i\"\n}\n", Info_ValueForKey( serverinfo, "mapname" ), longname, g_fraglimit.integer, numSpawnPoints, numObserverSpots, numWeapons, numPowerups);
	else if ( g_gametype.integer == GT_CTF )
		Com_sprintf( string, sizeof(string), "{\nmap \"%s\"\nlongname \"%s\"\nfraglimit %i\ntype \"q3r_ctf\"\nstarts \"%i\"\nobserverspots \"%i\"\nweapons \"%i\"\npowerups \"%i\"\n}\n", Info_ValueForKey( serverinfo, "mapname" ), longname, g_fraglimit.integer, numSpawnPoints, numObserverSpots, numWeapons, numPowerups);
	else if ( g_gametype.integer == GT_DEATHMATCH || g_gametype.integer == GT_TEAM )
		Com_sprintf( string, sizeof(string), "{\nmap \"%s\"\nlongname \"%s\"\nfraglimit %i\ntype \"q3r_dm q3r_team\"\nstarts \"%i\"\nobserverspots \"%i\"\nweapons \"%i\"\npowerups \"%i\"\n}\n", Info_ValueForKey( serverinfo, "mapname" ), longname, g_fraglimit.integer, numSpawnPoints, numObserverSpots, numWeapons, numPowerups);
	else
	{
		Com_Printf("Unknown game type while writing .arena file\n");
		return;
	}

	trap_FS_FOpenFile( va("scripts/%s.arena", Info_ValueForKey( serverinfo, "mapname" )), &arenaFile, FS_WRITE );
	if ( !arenaFile ) {
		return;
	}
	trap_FS_Write( string, strlen( string ), arenaFile );
	trap_FS_FCloseFile( arenaFile );
}
