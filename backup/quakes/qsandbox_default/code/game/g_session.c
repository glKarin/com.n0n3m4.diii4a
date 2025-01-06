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
#include "../qcommon/ns_local.h"


/*
=======================================================================
SESSION DATA

Session data is the only data that stays persistant across level loads
and tournament restarts.
=======================================================================
*/

/*
================
G_WriteClientSessionData

Called on game shutdown
================
*/
void G_WriteClientSessionData( gclient_t *client ) {
	const char	*s;
	const char	*var;

	s = va("%i %i %i %i %i %i %i", 
		client->sess.sessionTeam,
		client->sess.spectatorNum,
		client->sess.spectatorState,
		client->sess.spectatorClient,
		client->sess.wins,
		client->sess.losses,
		client->sess.teamLeader
		);

	var = va( "session%i", (int)(client - level.clients) );
	trap_Cvar_Set( var, s );
}

/*
================
G_ReadSessionData

Called on a reconnect
================
*/
void G_ReadSessionData( gclient_t *client ) {
	char	s[MAX_STRING_CHARS];
	const char	*var;

	// bk001205 - format
	int teamLeader;
	int spectatorState;
	int sessionTeam;

	var = va( "session%i", (int)(client - level.clients) );
	trap_Cvar_VariableStringBuffer( var, s, sizeof(s) );

	sscanf( s, "%i %i %i %i %i %i %i",
		&sessionTeam,                 // bk010221 - format
		&client->sess.spectatorNum,
		&spectatorState,              // bk010221 - format
		&client->sess.spectatorClient,
		&client->sess.wins,
		&client->sess.losses,
		&teamLeader                   // bk010221 - format
		);

	// bk001205 - format issues
	client->sess.sessionTeam = (team_t)sessionTeam;
	client->sess.spectatorState = (spectatorState_t)spectatorState;
	client->sess.teamLeader = (qboolean)teamLeader;
}


/*
================
G_InitSessionData

Called on a first-time connect
================
*/
void G_InitSessionData( gclient_t *client, char *userinfo ) {
	clientSession_t	*sess;
	const char		*value;

	sess = &client->sess;

	// initial team determination
	if ( g_gametype.integer >= GT_TEAM && g_ffa_gt!=1) {
		if ( g_teamAutoJoin.integer ) {
			sess->sessionTeam = PickTeam( -1 );
			BroadcastTeamChange( client, -1 );
		} else {
			// always spawn as spectator in team games
			sess->sessionTeam = TEAM_SPECTATOR;	
		}
	} else {
		value = Info_ValueForKey( userinfo, "team" );
		if ( value[0] == 's' ) {
			// a willing spectator, not a waiting-in-line
			sess->sessionTeam = TEAM_SPECTATOR;
		} else {
			switch ( g_gametype.integer ) {
			default:
			case GT_SANDBOX:
			case GT_FFA:
			case GT_SINGLE:
			case GT_LMS:
				if ( g_maxGameClients.integer > 0 && 
					level.numNonSpectatorClients >= g_maxGameClients.integer ) {
					sess->sessionTeam = TEAM_SPECTATOR;
				} else {
					sess->sessionTeam = TEAM_FREE;
				}
				break;
			case GT_TOURNAMENT:
				// if the game is full, go into a waiting mode
				if ( level.numNonSpectatorClients >= 2 ) {
					sess->sessionTeam = TEAM_SPECTATOR;
				} else {
					sess->sessionTeam = TEAM_FREE;
				}
				break;
			}
		}
	}

	sess->spectatorState = SPECTATOR_FREE;
	//sess->spectatorNum = level.time;
	 AddTournamentQueue(client);

	G_WriteClientSessionData( client );
}

/*
==================
G_Sav_SaveData

Updates session data for a client prior to a map change that's forced by a target_mapchange entity
==================
*/
void G_Sav_SaveData( gentity_t *ent, int slot ) {
	int secretFound, secretCount;
	int i;

	NS_setCvar(va("sav_%i_health", slot), va("%i", ent->client->ps.stats[STAT_HEALTH]));
	NS_setCvar(va("sav_%i_armor", slot), va("%i", ent->client->ps.stats[STAT_ARMOR]));
	NS_setCvar(va("sav_%i_weapon", slot), va("%i", ent->client->ps.generic2));

	for (i = 1; i < WEAPONS_NUM; i++){
		if(ent->swep_list[i] > 0){
			NS_setCvar(va("sav_%i_weapon%i", slot, i), va("%i", ent->swep_ammo[i]));
		} else {
			NS_setCvar(va("sav_%i_weapon%i", slot, i), va("%i", -999));
		}
	}

	NS_setCvar(va("sav_%i_holdable", slot), va("%i", ent->client->ps.stats[STAT_HOLDABLE_ITEM]));
	NS_setCvar(va("sav_%i_carnage", slot), va("%i", ent->client->ps.persistant[PERS_SCORE]));
	NS_setCvar(va("sav_%i_deaths", slot), va("%i", ent->client->ps.persistant[PERS_KILLED]));

	secretFound = (ent->client->ps.persistant[PERS_SECRETS] & 0x7F);
	secretCount = ((ent->client->ps.persistant[PERS_SECRETS] >> 7) & 0x7F) + level.secretCount;

	NS_setCvar(va("sav_%i_secrets", slot), va("%i", secretFound + (secretCount << 7)));
	NS_setCvar(va("sav_%i_accShots", slot), va("%i", ent->client->accuracy_shots));
	NS_setCvar(va("sav_%i_accHits", slot), va("%i", ent->client->accuracy_hits));
}

/*
==================
G_Sav_ClearData

Clears session data for map changes so that data does not persist through a hard map change (a map change not caused by target_mapchange) 
==================
*/
void G_Sav_ClearData( gclient_t *client, int slot ) {
	int i;

	NS_setCvar(va("sav_%i_health", slot), va("%i", 0));
	NS_setCvar(va("sav_%i_armor", slot), va("%i", 0));
	NS_setCvar(va("sav_%i_weapon", slot), va("%i", 0));

	for (i = 1; i < WEAPONS_NUM; i++){
		NS_setCvar(va("sav_%i_weapon%i", slot, i), va("%i", -999));
	}

	NS_setCvar(va("sav_%i_holdable", slot), va("%i", 0));
	NS_setCvar(va("sav_%i_carnage", slot), va("%i", 0));
	NS_setCvar(va("sav_%i_deaths", slot), va("%i", 0));

	NS_setCvar(va("sav_%i_secrets", slot), va("%i", 0));
	NS_setCvar(va("sav_%i_accShots", slot), va("%i", 0));
	NS_setCvar(va("sav_%i_accHits", slot), va("%i", 0));
}

/*
==================
G_Sav_LoadData

Updates a client entity with the data that's stored in that client's session data
==================
*/
void G_Sav_LoadData( gentity_t *ent, int slot ) {
	int i;

	if ( get_cvar_int(va("sav_%i_health", slot)) <= 0 ) {
		return;
	}

	ent->health = ent->client->ps.stats[STAT_HEALTH] = get_cvar_int(va("sav_%i_health", slot));
	ent->client->ps.stats[STAT_ARMOR] = get_cvar_int(va("sav_%i_armor", slot));
	ent->client->ps.weapon = get_cvar_int(va("sav_%i_weapon", slot));

	for (i = 1; i < WEAPONS_NUM; i++){
		if(get_cvar_int(va("sav_%i_weapon%i", slot, i)) != -999){
			if(get_cvar_int(va("sav_%i_weapon%i", slot, i)) != 0){
				ent->swep_list[i] = 1;
				ent->swep_ammo[i] = get_cvar_int(va("sav_%i_weapon%i", slot, i));
			} else {
				ent->swep_list[i] = 2;
				ent->swep_ammo[i] = get_cvar_int(va("sav_%i_weapon%i", slot, i));
			}
		} else {
			ent->swep_list[i] = 0;
		}
	}

	ent->client->ps.stats[STAT_HOLDABLE_ITEM] = get_cvar_int(va("sav_%i_holdable", slot));
	ent->client->ps.persistant[PERS_SCORE] = get_cvar_int(va("sav_%i_carnage", slot));
	ent->client->ps.persistant[PERS_KILLED] = get_cvar_int(va("sav_%i_deaths", slot));
	
	ent->client->ps.persistant[PERS_SECRETS] = get_cvar_int(va("sav_%i_secrets", slot));
	ent->client->accuracy_shots = get_cvar_int(va("sav_%i_accShots", slot));
	ent->client->accuracy_hits = get_cvar_int(va("sav_%i_accHits", slot));
	
	// clear map change session data
	G_Sav_ClearData( ent->client, slot );
}

/*
==================
G_InitWorldSession

==================
*/
void G_InitWorldSession( void ) {
	char	s[MAX_STRING_CHARS];
	int			gt;
	char	buf[MAX_INFO_STRING];

	//restore session from vQ3 session data
	trap_Cvar_VariableStringBuffer( "session", s, sizeof(s) );
	gt = atoi( s );

	// if the gametype changed since the last session, don't use any client sessions
	if ( g_gametype.integer != gt ) {
		level.newSession = qtrue;
        G_Printf( "Gametype changed, clearing session data.\n" );
	}
}

/*
==================
G_WriteSessionData

==================
*/
void G_WriteSessionData( void ) {
	int		i;

	trap_Cvar_Set( "session", va("%i", g_gametype.integer) );

	for ( i = 0 ; i < level.maxclients ; i++ ) {
		if ( level.clients[i].pers.connected == CON_CONNECTED ) {
			G_WriteClientSessionData( &level.clients[i] );
		}
	}
}
