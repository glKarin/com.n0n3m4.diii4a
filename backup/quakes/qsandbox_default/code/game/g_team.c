/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Open Arena source code.
Portions copied from Tremulous under GPL version 2 including any later version.

Open Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Open Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Open Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//

#include "g_local.h"


typedef struct teamgame_s {
	float			last_flag_capture;
	int				last_capture_team;
	flagStatus_t	redStatus;	// CTF
	flagStatus_t	blueStatus;	// CTF
	flagStatus_t	flagStatus;	// One Flag CTF
	int				redTakenTime;
	int				blueTakenTime;
	int				redObeliskAttackedTime;
	int				blueObeliskAttackedTime;
} teamgame_t;

teamgame_t teamgame;

gentity_t	*neutralObelisk;

//Some pointers for Double Domination so we don't need GFind (I think it might crash at random times...)
gentity_t	*ddA;
gentity_t	*ddB;
//Pointers for Standard Domination
gentity_t	*dom_points[MAX_DOMINATION_POINTS];

void Team_SetFlagStatus( int team, flagStatus_t status );

qboolean dominationPointsSpawned;

void Team_InitGame( void ) {
	memset(&teamgame, 0, sizeof teamgame);

	switch( g_gametype.integer ) {
	case GT_CTF:
	case GT_CTF_ELIMINATION:
	case GT_DOUBLE_D:
		teamgame.redStatus = -1; // Invalid to force update
		Team_SetFlagStatus( TEAM_RED, FLAG_ATBASE );
		 teamgame.blueStatus = -1; // Invalid to force update
		Team_SetFlagStatus( TEAM_BLUE, FLAG_ATBASE );
		ddA = NULL;
		ddB = NULL;
		break;
	case GT_DOMINATION:
		dominationPointsSpawned = qfalse;
                break;
	case GT_1FCTF:
		teamgame.flagStatus = -1; // Invalid to force update
		Team_SetFlagStatus( TEAM_FREE, FLAG_ATBASE );
		break;
	default:
		break;
	}
}

int OtherTeam(int team) {
	if (team==TEAM_RED)
		return TEAM_BLUE;
	else if (team==TEAM_BLUE)
		return TEAM_RED;
	return team;
}

const char *TeamName(int team)  {
	if (team==TEAM_RED)
		return "RED";
	else if (team==TEAM_BLUE)
		return "BLUE";
	else if (team==TEAM_SPECTATOR)
		return "SPECTATOR";
	return "FREE";
}

const char *OtherTeamName(int team) {
	if (team==TEAM_RED)
		return "BLUE";
	else if (team==TEAM_BLUE)
		return "RED";
	else if (team==TEAM_SPECTATOR)
		return "SPECTATOR";
	return "FREE";
}

const char *TeamColorString(int team) {
	if (team==TEAM_RED)
		return S_COLOR_RED;
	else if (team==TEAM_BLUE)
		return S_COLOR_BLUE;
	else if (team==TEAM_SPECTATOR)
		return S_COLOR_YELLOW;
	return S_COLOR_WHITE;
}

// NULL for everyone
void QDECL PrintMsg( gentity_t *ent, const char *fmt, ... ) {
	char		msg[1024];
	va_list		argptr;
	char		*p;
	
	va_start (argptr,fmt);
	if (Q_vsnprintf (msg, sizeof(msg), fmt, argptr) >= sizeof(msg)) {
		G_Error ( "PrintMsg overrun" );
	}
	va_end (argptr);

	// double quotes are bad
	while ((p = strchr(msg, '"')) != NULL)
		*p = '\'';

	trap_SendServerCommand ( ( (ent == NULL) ? -1 : ent-g_entities ), va("print \"%s\"", msg ));
}

/*
================
KK-OAX From Tremulous
G_TeamFromString
Return the team referenced by a string
================
*/
team_t G_TeamFromString( char *str )
{
  switch( tolower( *str ) )
  {
    case '0': case 's': return TEAM_NONE;
    case '1': case 'f': return TEAM_FREE;
    case '2': case 'r': return TEAM_RED;
    case '3': case 'b': return TEAM_BLUE;
    default: return TEAM_NUM_TEAMS;
  }
}

/*
==============
AddTeamScore

 used for gametype > GT_TEAM
 for gametype GT_TEAM the level.teamScores is updated in AddScore in g_combat.c
==============
*/
void AddTeamScore(vec3_t origin, int team, int score) {
	gentity_t	*te;


	if ( g_gametype.integer != GT_DOMINATION ) {
		te = G_TempEntity(origin, EV_GLOBAL_TEAM_SOUND );
		te->r.svFlags |= SVF_BROADCAST;

	
	
		if ( team == TEAM_RED ) {
			if ( level.teamScores[ TEAM_RED ] + score == level.teamScores[ TEAM_BLUE ] ) {
				//teams are tied sound
				te->s.eventParm = GTS_TEAMS_ARE_TIED;
			}
			else if ( level.teamScores[ TEAM_RED ] <= level.teamScores[ TEAM_BLUE ] &&
						level.teamScores[ TEAM_RED ] + score > level.teamScores[ TEAM_BLUE ]) {
				// red took the lead sound
				te->s.eventParm = GTS_REDTEAM_TOOK_LEAD;
			}
			else {
				// red scored sound
				te->s.eventParm = GTS_REDTEAM_SCORED;
			}
		}
		else {
			if ( level.teamScores[ TEAM_BLUE ] + score == level.teamScores[ TEAM_RED ] ) {
				//teams are tied sound
				te->s.eventParm = GTS_TEAMS_ARE_TIED;
			}
			else if ( level.teamScores[ TEAM_BLUE ] <= level.teamScores[ TEAM_RED ] &&
						level.teamScores[ TEAM_BLUE ] + score > level.teamScores[ TEAM_RED ]) {
				// blue took the lead sound
				te->s.eventParm = GTS_BLUETEAM_TOOK_LEAD;
			}
			else {
				// blue scored sound
				te->s.eventParm = GTS_BLUETEAM_SCORED;
			}
		}
	}
	level.teamScores[ team ] += score;
}

/*
==============
OnSameTeam
==============
*/
qboolean OnSameTeam( gentity_t *ent1, gentity_t *ent2 ) {
	if ( !ent1->client || !ent2->client ) {
		return qfalse;
	}

	if ( g_gametype.integer < GT_TEAM || g_ffa_gt==1) {
		return qfalse;
	}

	if ( ent1->client->sess.sessionTeam == ent2->client->sess.sessionTeam ) {
		return qtrue;
	}

	return qfalse;
}


static char ctfFlagStatusRemap[] = { '0', '1', '*', '*', '2' };
static char oneFlagStatusRemap[] = { '0', '1', '2', '3', '4' };

void Team_SetFlagStatus( int team, flagStatus_t status ) {
	qboolean modified = qfalse;

	switch( team ) {
	case TEAM_RED:	// CTF
		if( teamgame.redStatus != status ) {
			teamgame.redStatus = status;
			modified = qtrue;
		}
		break;

	case TEAM_BLUE:	// CTF
		if( teamgame.blueStatus != status ) {
			teamgame.blueStatus = status;
			modified = qtrue;
		}
		break;

	case TEAM_FREE:	// One Flag CTF
		if( teamgame.flagStatus != status ) {
			teamgame.flagStatus = status;
			modified = qtrue;
		}
		break;
	}


	if( modified ) {
		char st[4];

		if( g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTF_ELIMINATION) {
			st[0] = ctfFlagStatusRemap[teamgame.redStatus];
			st[1] = ctfFlagStatusRemap[teamgame.blueStatus];
			st[2] = 0;
		}
		else if (g_gametype.integer == GT_DOUBLE_D) {
			st[0] = oneFlagStatusRemap[teamgame.redStatus];
			st[1] = oneFlagStatusRemap[teamgame.blueStatus];
			st[2] = 0;
		}
		else {		// GT_1FCTF
			st[0] = oneFlagStatusRemap[teamgame.flagStatus];
			st[1] = 0;
		}

		trap_SetConfigstring( CS_FLAGSTATUS, st );
	}
}

void Team_CheckDroppedItem( gentity_t *dropped ) {
	if( dropped->item->giTag == PW_REDFLAG ) {
		Team_SetFlagStatus( TEAM_RED, FLAG_DROPPED );
	}
	else if( dropped->item->giTag == PW_BLUEFLAG ) {
		Team_SetFlagStatus( TEAM_BLUE, FLAG_DROPPED );
	}
	else if( dropped->item->giTag == PW_NEUTRALFLAG ) {
		Team_SetFlagStatus( TEAM_FREE, FLAG_DROPPED );
	}
}

/*
================
Team_ForceGesture
================
*/
void Team_ForceGesture(int team) {
	int i;
	gentity_t *ent;

	for (i = 0; i < MAX_CLIENTS; i++) {
		ent = &g_entities[i];
		if (!ent->inuse)
			continue;
		if (!ent->client)
			continue;
		if (ent->client->sess.sessionTeam != team)
			continue;
		ent->flags |= FL_FORCE_GESTURE;
	}
}

/*
================
Team_DD_bonusAtPoints
Adds bonus point to a player if he is close to the point and on the team that scores 
================
*/

void Team_DD_bonusAtPoints(int team) {
	vec3_t v1, v2;
	int i;
	gentity_t *player;

	for (i = 0; i < MAX_CLIENTS; i++) {
		player = &g_entities[i];
		if (!player->inuse)
			continue;
		if (!player->client)
			continue;
		
		if( player->client->sess.sessionTeam != team )
			return; //player was not on scoring team 

		//See if the player is close to any of the points:
		VectorSubtract(player->r.currentOrigin, ddA->r.currentOrigin, v1);
		VectorSubtract(player->r.currentOrigin, ddB->r.currentOrigin, v2);
		if (!( ( ( VectorLength(v1) < CTF_TARGET_PROTECT_RADIUS &&
				trap_InPVS(ddA->r.currentOrigin, player->r.currentOrigin ) ) ||
				( VectorLength(v2) < CTF_TARGET_PROTECT_RADIUS &&
				trap_InPVS(ddB->r.currentOrigin, player->r.currentOrigin ) ) )))
					return; //Wasn't close to any of the points
	
		AddScore(player, player->r.currentOrigin, DD_AT_POINT_AT_CAPTURE);
	}
}

/*
================
Team_FragBonuses

Calculate the bonuses for flag defense, flag carrier defense, etc.
Note that bonuses are not cumulative.  You get one, they are in importance
order.
================
*/
void Team_FragBonuses(gentity_t *targ, gentity_t *inflictor, gentity_t *attacker)
{
	int i;
	gentity_t *ent;
	int flag_pw, enemy_flag_pw;
	int otherteam;
	int tokens;
	gentity_t *flag, *carrier = NULL;
	char *c;
	vec3_t v1, v2;
	int team;

	// no bonus for fragging yourself or team mates
	if (!targ->client || !attacker->client || targ == attacker || OnSameTeam(targ, attacker))
		return;

	team = targ->client->sess.sessionTeam;
	otherteam = OtherTeam(targ->client->sess.sessionTeam);
	if (otherteam < 0)
		return; // whoever died isn't on a team

	// same team, if the flag at base, check to he has the enemy flag
	if (team == TEAM_RED) {
		flag_pw = PW_REDFLAG;
		enemy_flag_pw = PW_BLUEFLAG;
	} else {
		flag_pw = PW_BLUEFLAG;
		enemy_flag_pw = PW_REDFLAG;
	}

	if (g_gametype.integer == GT_1FCTF) {
		enemy_flag_pw = PW_NEUTRALFLAG;
	} 

	// did the attacker frag the flag carrier?
	tokens = 0;
	if( g_gametype.integer == GT_HARVESTER ) {
		tokens = targ->client->ps.generic1;
	}
	if (targ->client->ps.powerups[enemy_flag_pw]) {
		attacker->client->pers.teamState.lastfraggedcarrier = level.time;
		AddScore(attacker, targ->r.currentOrigin, CTF_FRAG_CARRIER_BONUS);
		attacker->client->pers.teamState.fragcarrier++;
		PrintMsg(NULL, "%s" S_COLOR_WHITE " fragged %s's flag carrier!\n",
			attacker->client->pers.netname, TeamName(team));
                if(g_gametype.integer == GT_CTF) {
                    G_LogPrintf( "CTF: %i %i %i: %s fragged %s's flag carrier!\n", attacker->client->ps.clientNum, team, 3, attacker->client->pers.netname, TeamName(team) );
                } else if(g_gametype.integer == GT_CTF_ELIMINATION) {
                    G_LogPrintf( "CTF_ELIMINATION: %i %i %i %i: %s fragged %s's flag carrier!\n", level.roundNumber, attacker->client->ps.clientNum, team, 3, attacker->client->pers.netname, TeamName(team) );
                } else if(g_gametype.integer == GT_1FCTF) {
                    G_LogPrintf( "1fCTF: %i %i %i: %s fragged %s's flag carrier!\n", attacker->client->ps.clientNum, team, 3, attacker->client->pers.netname, TeamName(team) );
                }
                

		// the target had the flag, clear the hurt carrier
		// field on the other team
		for (i = 0; i < g_maxclients.integer; i++) {
			ent = g_entities + i;
			if (ent->inuse && ent->client->sess.sessionTeam == otherteam)
				ent->client->pers.teamState.lasthurtcarrier = 0;
		}
		return;
	}

	// did the attacker frag a head carrier? other->client->ps.generic1
	if (tokens) {
		attacker->client->pers.teamState.lastfraggedcarrier = level.time;
		AddScore(attacker, targ->r.currentOrigin, CTF_FRAG_CARRIER_BONUS * tokens * tokens);
		attacker->client->pers.teamState.fragcarrier++;
		PrintMsg(NULL, "%s" S_COLOR_WHITE " fragged %s's skull carrier!\n",
			attacker->client->pers.netname, TeamName(team));

                G_LogPrintf("HARVESTER: %i %i %i %i %i: %s fragged %s (%s) who had %i skulls.\n",
                        attacker->client->ps.clientNum, team, 1, targ->client->ps.clientNum, tokens,
                        attacker->client->pers.netname, targ->client->pers.netname,TeamName(team),tokens);

		// the target had the flag, clear the hurt carrier
		// field on the other team
		for (i = 0; i < g_maxclients.integer; i++) {
			ent = g_entities + i;
			if (ent->inuse && ent->client->sess.sessionTeam == otherteam)
				ent->client->pers.teamState.lasthurtcarrier = 0;
		}
		return;
	}

	if (targ->client->pers.teamState.lasthurtcarrier &&
		level.time - targ->client->pers.teamState.lasthurtcarrier < CTF_CARRIER_DANGER_PROTECT_TIMEOUT &&
		!attacker->client->ps.powerups[flag_pw]) {
		// attacker is on the same team as the flag carrier and
		// fragged a guy who hurt our flag carrier
		AddScore(attacker, targ->r.currentOrigin, CTF_CARRIER_DANGER_PROTECT_BONUS);

		attacker->client->pers.teamState.carrierdefense++;
		targ->client->pers.teamState.lasthurtcarrier = 0;

		attacker->client->ps.persistant[PERS_DEFEND_COUNT]++;
		team = attacker->client->sess.sessionTeam;
		// add the sprite over the player's head
		attacker->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
		attacker->client->ps.eFlags |= EF_AWARD_DEFEND;
		attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;

		return;
	}

	if (targ->client->pers.teamState.lasthurtcarrier &&
		level.time - targ->client->pers.teamState.lasthurtcarrier < CTF_CARRIER_DANGER_PROTECT_TIMEOUT) {
		// attacker is on the same team as the skull carrier and
		AddScore(attacker, targ->r.currentOrigin, CTF_CARRIER_DANGER_PROTECT_BONUS);

		attacker->client->pers.teamState.carrierdefense++;
		targ->client->pers.teamState.lasthurtcarrier = 0;

		attacker->client->ps.persistant[PERS_DEFEND_COUNT]++;
		team = attacker->client->sess.sessionTeam;
		// add the sprite over the player's head
		attacker->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
		attacker->client->ps.eFlags |= EF_AWARD_DEFEND;
		attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;

		return;
	}

//We place the Double Domination bonus test here! This appears to be the best place to place them.
	if ( g_gametype.integer == GT_DOUBLE_D ) {
		if(attacker->client->sess.sessionTeam == level.pointStatusA ) { //Attack must defend point A
			//See how close attacker and target was to Point A:
			VectorSubtract(targ->r.currentOrigin, ddA->r.currentOrigin, v1);
			VectorSubtract(attacker->r.currentOrigin, ddA->r.currentOrigin, v2);

			if ( ( ( VectorLength(v1) < CTF_TARGET_PROTECT_RADIUS &&
				trap_InPVS(ddA->r.currentOrigin, targ->r.currentOrigin ) ) ||
				( VectorLength(v2) < CTF_TARGET_PROTECT_RADIUS &&
				trap_InPVS(ddA->r.currentOrigin, attacker->r.currentOrigin ) ) ) &&
				attacker->client->sess.sessionTeam != targ->client->sess.sessionTeam) {
				
				//We defended point A
				//Was we dominating and maybe close to score?
				if(attacker->client->sess.sessionTeam == level.pointStatusB && level.time - level.timeTaken > (10-DD_CLOSE)*1000)
					AddScore(attacker, targ->r.currentOrigin, DD_POINT_DEFENCE_CLOSE_BONUS);
				else
					AddScore(attacker, targ->r.currentOrigin, DD_POINT_DEFENCE_BONUS);
				attacker->client->pers.teamState.basedefense++;

				attacker->client->ps.persistant[PERS_DEFEND_COUNT]++;
                // add the sprite over the player's head
				attacker->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
				attacker->client->ps.eFlags |= EF_AWARD_DEFEND;
				attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;

				return; //Return so we don't recieve credits for point B also

			} //We denfended point A



		} //Defend point A

		if(attacker->client->sess.sessionTeam == level.pointStatusB ) { //Attack must defend point B
			//See how close attacker and target was to Point B:
			VectorSubtract(targ->r.currentOrigin, ddB->r.currentOrigin, v1);
			VectorSubtract(attacker->r.currentOrigin, ddB->r.currentOrigin, v2);

			if ( ( ( VectorLength(v1) < CTF_TARGET_PROTECT_RADIUS &&
				trap_InPVS(ddB->r.currentOrigin, targ->r.currentOrigin ) ) ||
				( VectorLength(v2) < CTF_TARGET_PROTECT_RADIUS &&
				trap_InPVS(ddB->r.currentOrigin, attacker->r.currentOrigin ) ) ) &&
				attacker->client->sess.sessionTeam != targ->client->sess.sessionTeam) {
				
				//We defended point B
				//Was we dominating and maybe close to score?
				if(attacker->client->sess.sessionTeam == level.pointStatusA && level.time - level.timeTaken > (10-DD_CLOSE)*1000)
					AddScore(attacker, targ->r.currentOrigin, DD_POINT_DEFENCE_CLOSE_BONUS);
				else
					AddScore(attacker, targ->r.currentOrigin, DD_POINT_DEFENCE_BONUS);
				attacker->client->pers.teamState.basedefense++;

				attacker->client->ps.persistant[PERS_DEFEND_COUNT]++;
				// add the sprite over the player's head
				attacker->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
				attacker->client->ps.eFlags |= EF_AWARD_DEFEND;
				attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;

				return;

			} //We denfended point B



		} //Defend point B
	return; //In double Domination we shall not go on, or we would test for team bases that we don't use
	}

	// flag and flag carrier area defense bonuses

	// we have to find the flag and carrier entities

	if( g_gametype.integer == GT_OBELISK ) {
		// find the team obelisk
		switch (attacker->client->sess.sessionTeam) {
		case TEAM_RED:
			c = "team_redobelisk";
			break;
		case TEAM_BLUE:
			c = "team_blueobelisk";
			break;		
		default:
			return;
		}
		
	} else if (g_gametype.integer == GT_HARVESTER ) {
		// find the center obelisk
		c = "team_neutralobelisk";
	} else {
	// find the flag
	switch (attacker->client->sess.sessionTeam) {
	case TEAM_RED:
		c = "team_CTF_redflag";
		break;
	case TEAM_BLUE:
		c = "team_CTF_blueflag";
		break;		
	default:
		return;
	}
	// find attacker's team's flag carrier
	for (i = 0; i < g_maxclients.integer; i++) {
		carrier = g_entities + i;
		if (carrier->inuse && carrier->client->ps.powerups[flag_pw])
			break;
		carrier = NULL;
	}
	}
	flag = NULL;
	while ((flag = G_Find (flag, FOFS(classname), c)) != NULL) {
		if (!(flag->flags & FL_DROPPED_ITEM))
			break;
	}

	if (!flag)
		return; // can't find attacker's flag

	// ok we have the attackers flag and a pointer to the carrier

	// check to see if we are defending the base's flag
	VectorSubtract(targ->r.currentOrigin, flag->r.currentOrigin, v1);
	VectorSubtract(attacker->r.currentOrigin, flag->r.currentOrigin, v2);

	if ( ( ( VectorLength(v1) < CTF_TARGET_PROTECT_RADIUS &&
		trap_InPVS(flag->r.currentOrigin, targ->r.currentOrigin ) ) ||
		( VectorLength(v2) < CTF_TARGET_PROTECT_RADIUS &&
		trap_InPVS(flag->r.currentOrigin, attacker->r.currentOrigin ) ) ) &&
		attacker->client->sess.sessionTeam != targ->client->sess.sessionTeam && g_gametype.integer != GT_ELIMINATION &&
		(g_gametype.integer != GT_CTF_ELIMINATION || !g_elimination_ctf_oneway.integer ||
		((level.eliminationSides+level.roundNumber)%2 == 0 && attacker->client->sess.sessionTeam == TEAM_BLUE ) ||
		((level.eliminationSides+level.roundNumber)%2 == 1 && attacker->client->sess.sessionTeam == TEAM_RED ) ) ) {

		// we defended the base flag
		AddScore(attacker, targ->r.currentOrigin, CTF_FLAG_DEFENSE_BONUS);
		attacker->client->pers.teamState.basedefense++;

		attacker->client->ps.persistant[PERS_DEFEND_COUNT]++;
		// add the sprite over the player's head
		attacker->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
		attacker->client->ps.eFlags |= EF_AWARD_DEFEND;
		attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;

		return;
	}

	if (carrier && carrier != attacker) {
		VectorSubtract(targ->r.currentOrigin, carrier->r.currentOrigin, v1);
		VectorSubtract(attacker->r.currentOrigin, carrier->r.currentOrigin, v1);

		if ( ( ( VectorLength(v1) < CTF_ATTACKER_PROTECT_RADIUS &&
			trap_InPVS(carrier->r.currentOrigin, targ->r.currentOrigin ) ) ||
			( VectorLength(v2) < CTF_ATTACKER_PROTECT_RADIUS &&
				trap_InPVS(carrier->r.currentOrigin, attacker->r.currentOrigin ) ) ) &&
			attacker->client->sess.sessionTeam != targ->client->sess.sessionTeam) {
			AddScore(attacker, targ->r.currentOrigin, CTF_CARRIER_PROTECT_BONUS);
			attacker->client->pers.teamState.carrierdefense++;

			attacker->client->ps.persistant[PERS_DEFEND_COUNT]++;
			// add the sprite over the player's head
			attacker->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
			attacker->client->ps.eFlags |= EF_AWARD_DEFEND;
			attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;

			return;
		}
	}
}

/*
================
Team_CheckHurtCarrier

Check to see if attacker hurt the flag carrier.  Needed when handing out bonuses for assistance to flag
carrier defense.
================
*/
void Team_CheckHurtCarrier(gentity_t *targ, gentity_t *attacker)
{
	int flag_pw;

	if (!targ->client || !attacker->client)
		return;

	if (targ->client->sess.sessionTeam == TEAM_RED)
		flag_pw = PW_BLUEFLAG;
	else
		flag_pw = PW_REDFLAG;

	// flags
	if (targ->client->ps.powerups[flag_pw] &&
		targ->client->sess.sessionTeam != attacker->client->sess.sessionTeam)
		attacker->client->pers.teamState.lasthurtcarrier = level.time;

	// skulls
	if (targ->client->ps.generic1 &&
		targ->client->sess.sessionTeam != attacker->client->sess.sessionTeam)
		attacker->client->pers.teamState.lasthurtcarrier = level.time;
}


gentity_t *Team_ResetFlag( int team ) {
	char *c;
	gentity_t *ent, *rent = NULL;

	switch (team) {
	case TEAM_RED:
		c = "team_CTF_redflag";
		break;
	case TEAM_BLUE:
		c = "team_CTF_blueflag";
		break;
	case TEAM_FREE:
		c = "team_CTF_neutralflag";
		break;
	default:
		return NULL;
	}

	ent = NULL;
	while ((ent = G_Find (ent, FOFS(classname), c)) != NULL) {
		if (ent->flags & FL_DROPPED_ITEM)
			G_FreeEntity(ent);
		else {
			rent = ent;
			RespawnItemCtf(ent);
		}
	}

	Team_SetFlagStatus( team, FLAG_ATBASE );

	return rent;
}

//Functions for Domination

void Team_Dom_SpawnPoints( void ) {
	char *c;
	gentity_t *flag;
	int i;
	gitem_t			*it;
        
        flag = NULL;
	
	if(dominationPointsSpawned)
		return;
	dominationPointsSpawned = qtrue;

	it = NULL;
	it = BG_FindItem ("Neutral domination point");
	if(it == NULL) {
		PrintMsg( NULL, "No domination item\n");
		return;
	} else {
		PrintMsg( NULL, "Domination item found\n");
	}
	i = 0;
	c = "domination_point";
	
	//return; Just to test, the lines below crashes game
	
	while ((flag = G_Find (flag, FOFS(classname), c)) != NULL) {
		if(i>=MAX_DOMINATION_POINTS)
			break;
		//domination_points_names[i] = flag->message;
                if(flag->message) {
                    Q_strncpyz(level.domination_points_names[i],flag->message,MAX_DOMINATION_POINTS_NAMES-1);
                    PrintMsg( NULL, "Domination point \'%s\' found\n",level.domination_points_names[i]);
                } else {
                    Q_strncpyz(level.domination_points_names[i],va("Point %i",i),MAX_DOMINATION_POINTS_NAMES-1);
                    PrintMsg( NULL, "Domination point \'%s\' found (autonamed)\n",level.domination_points_names[i]);
                }
		dom_points[i] = G_Spawn();
		VectorCopy( flag->r.currentOrigin, dom_points[i]->s.origin );
		dom_points[i]->classname = it->classname;
		G_SpawnItem(dom_points[i], it);
		FinishSpawningItem(dom_points[i] );
		
		i++;
	}
	if(!i){
		c = "info_player_deathmatch";
		while ((flag = G_Find (flag, FOFS(classname), c)) != NULL) {
		if(i>=MAX_DOMINATION_POINTS)
			break;
		//domination_points_names[i] = flag->message;
                if(flag->message) {
                    Q_strncpyz(level.domination_points_names[i],flag->message,MAX_DOMINATION_POINTS_NAMES-1);
                    PrintMsg( NULL, "Domination point \'%s\' found\n",level.domination_points_names[i]);
                } else {
                    Q_strncpyz(level.domination_points_names[i],va("Point %i",i),MAX_DOMINATION_POINTS_NAMES-1);
                    PrintMsg( NULL, "Domination point \'%s\' found (autonamed)\n",level.domination_points_names[i]);
                }
		dom_points[i] = G_Spawn();
		VectorCopy( flag->r.currentOrigin, dom_points[i]->s.origin );
		dom_points[i]->classname = it->classname;
		G_SpawnItem(dom_points[i], it);
		FinishSpawningItem(dom_points[i] );
		
		i++;
	}	
	}
	level.domination_points_count = i;
}

int getDomPointNumber( gentity_t *point ) {
	int i;
	for(i=1;i<MAX_DOMINATION_POINTS && i<level.domination_points_count;i++) {
		if(dom_points[i] == NULL)
			return 0; //Not found, just return first, so we don't crash
		if(dom_points[i] == point)
			return i;
	}
	return 0;
}

void Team_Dom_TakePoint( gentity_t *point, int team, int clientnumber ) {
	gitem_t			*it;
	vec3_t			origin;
	int i;
	i = getDomPointNumber(point);
	if(i<0)
		i = 0;
	if(i>=MAX_DOMINATION_POINTS)
		i = MAX_DOMINATION_POINTS - 1;

	it = NULL;
	VectorCopy( point->r.currentOrigin, origin );

	if(team == TEAM_RED) {
		it = BG_FindItem ("Red domination point");
		PrintMsg( NULL, "Red took \'%s\'\n",level.domination_points_names[i]);
	} else
	if(team == TEAM_BLUE) {
		it = BG_FindItem ("Blue domination point");
		PrintMsg( NULL, "Blue took \'%s\'\n",level.domination_points_names[i]);
	}
	if (!it || it == NULL) {
		PrintMsg( NULL, "No item\n");
		return;
	}

	G_FreeEntity(point);

	point = G_Spawn();

	VectorCopy( origin, point->s.origin );
	point->classname = it->classname;
	dom_points[i] = point;
	G_SpawnItem(point, it);
	FinishSpawningItem( point );
	level.pointStatusDom[i] = team;
        G_LogPrintf( "DOM: %i %i %i %i: %s takes point %s!\n",
                    clientnumber,i,0,team,
                    TeamName(team),level.domination_points_names[i]);
	SendDominationPointsStatusMessageToAllClients();
}

//Functions for Double Domination

void Team_DD_RemovePointAgfx( void ) {
	if(ddA!=NULL) {
		G_FreeEntity(ddA);
		ddA = NULL;
	}
}

void Team_DD_RemovePointBgfx( void ) {
	if(ddB!=NULL) {
		G_FreeEntity(ddB);
		ddB = NULL;
	}
}

void Team_DD_makeA2team( gentity_t *target, int team ) {
	gitem_t			*it;
	//gentity_t		*it_ent;
	Team_DD_RemovePointAgfx();
	it = NULL;
	if(team == TEAM_NONE)
		return;
	if(team == TEAM_RED)
		it = BG_FindItem ("Point A (Red)");
	if(team == TEAM_BLUE)
		it = BG_FindItem ("Point A (Blue)");
	if(team == TEAM_FREE)
		it = BG_FindItem ("Point A (White)");
	if (!it || it == NULL) {
		PrintMsg( NULL, "No item\n");
		return;
	}
	ddA = G_Spawn();
	
	VectorCopy( target->r.currentOrigin, ddA->s.origin );
	ddA->classname = it->classname;
	G_SpawnItem(ddA, it);
	FinishSpawningItem(ddA );
}

void Team_DD_makeB2team( gentity_t *target, int team ) {
	gitem_t			*it;
	//gentity_t		*it_ent;
	
	Team_DD_RemovePointBgfx();
        it = NULL;
	if(team == TEAM_NONE)
		return;
	if(team == TEAM_RED)
		it = BG_FindItem ("Point B (Red)");
	if(team == TEAM_BLUE)
		it = BG_FindItem ("Point B (Blue)");
	if(team == TEAM_FREE)
		it = BG_FindItem ("Point B (White)");
	if (!it || it == NULL) {
		PrintMsg( NULL, "No item\n");
		return;
	}
	ddB = G_Spawn();
	
	VectorCopy( target->r.currentOrigin, ddB->s.origin );
	ddB->classname = it->classname;
	G_SpawnItem(ddB, it);
	FinishSpawningItem(ddB );
}

void Team_ResetFlags( void ) {
	if( g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTF_ELIMINATION) {
		Team_ResetFlag( TEAM_RED );
		Team_ResetFlag( TEAM_BLUE );
	}
	else if( g_gametype.integer == GT_1FCTF ) {
		Team_ResetFlag( TEAM_FREE );
	}
}

void Team_ReturnFlagSound( gentity_t *ent, int team ) {
	gentity_t	*te;

	if (ent == NULL) {
                G_Printf ("Warning:  NULL passed to Team_ReturnFlagSound\n");
		return;
	}

	//See if we are during CTF_ELIMINATION warmup
	if((level.time<=level.roundStartTime && level.time>level.roundStartTime-1000*g_elimination_activewarmup.integer)&&g_gametype.integer == GT_CTF_ELIMINATION)
		return;

	te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_TEAM_SOUND );
	if( team == TEAM_BLUE ) {
		te->s.eventParm = GTS_RED_RETURN;
	}
	else {
		te->s.eventParm = GTS_BLUE_RETURN;
	}
	te->r.svFlags |= SVF_BROADCAST;
}

void Team_TakeFlagSound( gentity_t *ent, int team ) {
	gentity_t	*te;

	if (ent == NULL) {
                G_Printf ("Warning:  NULL passed to Team_TakeFlagSound\n");
		return;
	}

	// only play sound when the flag was at the base
	// or not picked up the last 10 seconds
	switch(team) {
		case TEAM_RED:
			if( teamgame.blueStatus != FLAG_ATBASE ) {
				if (teamgame.blueTakenTime > level.time - 10000 && g_gametype.integer != GT_CTF_ELIMINATION)
					return;
			}
			teamgame.blueTakenTime = level.time;
			break;

		case TEAM_BLUE:	// CTF
			if( teamgame.redStatus != FLAG_ATBASE ) {
				if (teamgame.redTakenTime > level.time - 10000 && g_gametype.integer != GT_CTF_ELIMINATION)
					return;
			}
			teamgame.redTakenTime = level.time;
			break;
	}

	te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_TEAM_SOUND );
	if( team == TEAM_BLUE ) {
		te->s.eventParm = GTS_RED_TAKEN;
	}
	else {
		te->s.eventParm = GTS_BLUE_TAKEN;
	}
	te->r.svFlags |= SVF_BROADCAST;
}

void Team_CaptureFlagSound( gentity_t *ent, int team ) {
	gentity_t	*te;

	if (ent == NULL) {
                G_Printf ("Warning:  NULL passed to Team_CaptureFlagSound\n");
		return;
	}

	te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_TEAM_SOUND );
	if( team == TEAM_BLUE ) {
		te->s.eventParm = GTS_BLUE_CAPTURE;
	}
	else {
		te->s.eventParm = GTS_RED_CAPTURE;
	}
	te->r.svFlags |= SVF_BROADCAST;
}

void Team_ReturnFlag( int team ) {
	Team_ReturnFlagSound(Team_ResetFlag(team), team);
	if( team == TEAM_FREE ) {
		PrintMsg(NULL, "The flag has returned!\n" );
                if(g_gametype.integer == GT_1FCTF) {
                    G_LogPrintf( "1FCTF: %i %i %i: The flag was returned!\n", -1, -1, 2 );
                }
	}
	else {
		PrintMsg(NULL, "The %s flag has returned!\n", TeamName(team));
                if(g_gametype.integer == GT_CTF_ELIMINATION) {
                    G_LogPrintf( "CTF: %i %i %i: The %s flag was returned!\n", -1, team, 2, TeamName(team) );
                } else
                if(g_gametype.integer == GT_CTF_ELIMINATION) {
                    G_LogPrintf( "CTF_ELIMINATION: %i %i %i %i: The %s flag was returned!\n", level.roundNumber, -1, team, 2, TeamName(team) );
                }
	}
}

void Team_FreeEntity( gentity_t *ent ) {
	if( ent->item->giTag == PW_REDFLAG ) {
		Team_ReturnFlag( TEAM_RED );
	}
	else if( ent->item->giTag == PW_BLUEFLAG ) {
		Team_ReturnFlag( TEAM_BLUE );
	}
	else if( ent->item->giTag == PW_NEUTRALFLAG ) {
		Team_ReturnFlag( TEAM_FREE );
	}
}

/*
==============
Team_DroppedFlagThink

Automatically set in Launch_Item if the item is one of the flags

Flags are unique in that if they are dropped, the base flag must be respawned when they time out
==============
*/
void Team_DroppedFlagThink(gentity_t *ent) {
	int		team = TEAM_FREE;

	if( ent->item->giTag == PW_REDFLAG ) {
		team = TEAM_RED;
	}
	else if( ent->item->giTag == PW_BLUEFLAG ) {
		team = TEAM_BLUE;
	}
	else if( ent->item->giTag == PW_NEUTRALFLAG ) {
		team = TEAM_FREE;
	}

	Team_ReturnFlagSound( Team_ResetFlag( team ), team );
	// Reset Flag will delete this entity
}

/*
Update DD points
*/

void updateDDpoints(void) {
	//teamgame.redStatus = -1; // Invalid to force update
	Team_SetFlagStatus( TEAM_RED, level.pointStatusA );
	//teamgame.blueStatus = -1; // Invalid to force update
	Team_SetFlagStatus( TEAM_BLUE, level.pointStatusB );
}

/*
==============
Team_SpawnDoubleDominationPoints
==============
*/

int Team_SpawnDoubleDominationPoints ( void ) {
	gentity_t	*ent;
	level.pointStatusA = TEAM_FREE;
	level.pointStatusB = TEAM_FREE;
	updateDDpoints();
	ent = NULL;
	if ((ent = G_Find (ent, FOFS(classname), "team_CTF_redflag")) != NULL) {
		Team_DD_makeA2team( ent, TEAM_FREE );
	}
        ent = NULL;
	if ((ent = G_Find (ent, FOFS(classname), "team_CTF_blueflag")) != NULL) {
		Team_DD_makeB2team( ent, TEAM_FREE );
	}
	return 1;
}

/*
==============
Team_RemoveDoubleDominationPoints
==============
*/

int Team_RemoveDoubleDominationPoints ( void ) {
	level.pointStatusA = TEAM_NONE;
	level.pointStatusB = TEAM_NONE;
	updateDDpoints();
	Team_DD_makeA2team( NULL, TEAM_NONE );
	Team_DD_makeB2team( NULL, TEAM_NONE );
	return 1;
}

/*
==============
Team_TouchDoubleDominationPoint
==============
*/

//team is the either TEAM_RED(A) or TEAM_BLUE(B)
int Team_TouchDoubleDominationPoint( gentity_t *ent, gentity_t *other, int team ) {
	gclient_t	*cl = other->client;
	qboolean	otherDominating, isClose;
	int 		clientTeam = cl->sess.sessionTeam;
	int		otherTeam;
	int		score; //Used to add the scores together 

	if(clientTeam == TEAM_RED)
		otherTeam = TEAM_BLUE;
	else
		otherTeam = TEAM_RED;

	otherDominating = qfalse;
	isClose = qfalse;

	if(level.pointStatusA == otherTeam && level.pointStatusB == otherTeam) {
		otherDominating = qtrue;
		if(level.time - level.timeTaken > (10-DD_CLOSE)*1000)
			isClose = qtrue;
	}	
	

	if(team == TEAM_RED) //We have touched point A
	{
		if(TEAM_NONE == level.pointStatusA)
			return 0; //Haven't spawned yet
		if(clientTeam == level.pointStatusA)
			return 0; //If we already have the flag
		//if we didn't have the point, then we have now!
		level.pointStatusA = clientTeam;
		PrintMsg( NULL, "%s" S_COLOR_WHITE " (%s) took control of A!\n", cl->pers.netname, TeamName(clientTeam) );
		Team_DD_makeA2team( ent, clientTeam );
                G_LogPrintf( "DD: %i %i %i: %s took point A for %s!\n", cl->ps.clientNum, clientTeam, 0, cl->pers.netname, TeamName(clientTeam) );
		//Give personal score
		score = DD_POINT_CAPTURE; //Base score for capture
		if(otherDominating){
			score += DD_POINT_CAPTURE_BREAK;
			if(isClose)
				score += DD_POINT_CAPTURE_CLOSE;
		}
		AddScore(other, ent->r.currentOrigin, score);
		//Do we also have point B?
		if(clientTeam == level.pointStatusB)
		{
			//We are dominating!
			level.timeTaken = level.time; //At this time
			PrintMsg( NULL, "%s" S_COLOR_WHITE " is dominating!\n", TeamName(clientTeam) );
			SendDDtimetakenMessageToAllClients();
		}
	}

	if(team == TEAM_BLUE) //We have touched point B
	{
		if(TEAM_NONE == level.pointStatusB)
			return 0; //Haven't spawned yet
		if(clientTeam == level.pointStatusB)
			return 0; //If we already have the flag
		//if we didn't have the point, then we have now!
		level.pointStatusB = clientTeam;
		PrintMsg( NULL, "%s" S_COLOR_WHITE " (%s) took control of B!\n", cl->pers.netname, TeamName(clientTeam) );
		Team_DD_makeB2team( ent, clientTeam );
                G_LogPrintf( "DD: %i %i %i: %s took point B for %s!\n", cl->ps.clientNum, clientTeam, 1, cl->pers.netname, TeamName(clientTeam) );
		//Give personal score
		score = DD_POINT_CAPTURE; //Base score for capture
		if(otherDominating){
			score += DD_POINT_CAPTURE_BREAK;
			if(isClose)
				score += DD_POINT_CAPTURE_CLOSE;
		}
		AddScore(other, ent->r.currentOrigin, score);
		//Do we also have point A?
		if(clientTeam == level.pointStatusA)
		{
			//We are dominating!
			level.timeTaken = level.time; //At this time
			PrintMsg( NULL, "%s" S_COLOR_WHITE " is dominating!\n", TeamName(clientTeam) );
			SendDDtimetakenMessageToAllClients();
		}
	}

	updateDDpoints();

	return 0;
}

/*
==============
Team_TouchOurFlag
==============
*/
int Team_TouchOurFlag( gentity_t *ent, gentity_t *other, int team ) {
	int			i;
	gentity_t	*player;
	gclient_t	*cl = other->client;
	int			enemy_flag;

	if( g_gametype.integer == GT_1FCTF ) {
		enemy_flag = PW_NEUTRALFLAG;
	}
	else {
	if (cl->sess.sessionTeam == TEAM_RED) {
		enemy_flag = PW_BLUEFLAG;
	} else {
		enemy_flag = PW_REDFLAG;
	}

	if ( ent->flags & FL_DROPPED_ITEM ) {
		// hey, its not home.  return it by teleporting it back
		PrintMsg( NULL, "%s" S_COLOR_WHITE " returned the %s flag!\n", 
			cl->pers.netname, TeamName(team));
		AddScore(other, ent->r.currentOrigin, CTF_RECOVERY_BONUS);
                if(g_gametype.integer == GT_CTF) {
                    G_LogPrintf( "CTF: %i %i %i: %s returned the %s flag!\n", cl->ps.clientNum, team, 2, cl->pers.netname, TeamName(team) );
                } else if(g_gametype.integer == GT_CTF_ELIMINATION) {
                    G_LogPrintf( "CTF_ELIMINATION: %i %i %i %i: %s returned the %s flag!\n", level.roundNumber, cl->ps.clientNum, team, 2, cl->pers.netname, TeamName(team) );
                }
		other->client->pers.teamState.flagrecovery++;
		other->client->pers.teamState.lastreturnedflag = level.time;
		//ResetFlag will remove this entity!  We must return zero
		Team_ReturnFlagSound(Team_ResetFlag(team), team);
		return 0;
	}
	}

	// the flag is at home base.  if the player has the enemy
	// flag, he's just won!
	if (!cl->ps.powerups[enemy_flag])
		return 0; // We don't have the flag
	if( g_gametype.integer == GT_1FCTF ) {
		PrintMsg( NULL, "%s" S_COLOR_WHITE " captured the flag!\n", cl->pers.netname );
                G_LogPrintf( "1FCTF: %i %i %i: %s captured the flag!\n", cl->ps.clientNum, -1, 1, cl->pers.netname );
	}
	else {
            PrintMsg( NULL, "%s" S_COLOR_WHITE " captured the %s flag!\n", cl->pers.netname, TeamName(OtherTeam(team)));
            if(g_gametype.integer == GT_CTF)
                G_LogPrintf( "CTF: %i %i %i: %s captured the %s flag!\n", cl->ps.clientNum, OtherTeam(team), 1, cl->pers.netname, TeamName(OtherTeam(team)) );
            if(g_gametype.integer == GT_CTF_ELIMINATION)
                G_LogPrintf( "CTF_ELIMINATION: %i %i %i %i: %s captured the %s flag!\n", level.roundNumber, cl->ps.clientNum, OtherTeam(team), 1, cl->pers.netname, TeamName(OtherTeam(team)) );
	}

	cl->ps.powerups[enemy_flag] = 0;

	teamgame.last_flag_capture = level.time;
	teamgame.last_capture_team = team;

	// Increase the team's score
	AddTeamScore(ent->s.pos.trBase, other->client->sess.sessionTeam, 1);
	Team_ForceGesture(other->client->sess.sessionTeam);
	//If CTF Elimination, stop the round:
	if(g_gametype.integer==GT_CTF_ELIMINATION) {
		EndEliminationRound();
	}

	other->client->pers.teamState.captures++;
	// add the sprite over the player's head
	other->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
	other->client->ps.eFlags |= EF_AWARD_CAP;
	other->client->rewardTime = level.time + REWARD_SPRITE_TIME;
	other->client->ps.persistant[PERS_CAPTURES]++;
	// other gets another 10 frag bonus
	AddScore(other, ent->r.currentOrigin, CTF_CAPTURE_BONUS);

	Team_CaptureFlagSound( ent, team );

	// Ok, let's do the player loop, hand out the bonuses
	for (i = 0; i < g_maxclients.integer; i++) {
		player = &g_entities[i];
		if (!player->inuse || player == other)
			continue;

		if (player->client->sess.sessionTeam !=
			cl->sess.sessionTeam) {
			player->client->pers.teamState.lasthurtcarrier = -5;
		} else if (player->client->sess.sessionTeam ==
			cl->sess.sessionTeam) {
			if (player != other)
				AddScore(player, ent->r.currentOrigin, CTF_TEAM_BONUS);
			// award extra points for capture assists
			if (player->client->pers.teamState.lastreturnedflag + 
				CTF_RETURN_FLAG_ASSIST_TIMEOUT > level.time) {
				AddScore (player, ent->r.currentOrigin, CTF_RETURN_FLAG_ASSIST_BONUS);
				other->client->pers.teamState.assists++;

				player->client->ps.persistant[PERS_ASSIST_COUNT]++;
				// add the sprite over the player's head
				player->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
				player->client->ps.eFlags |= EF_AWARD_ASSIST;
				player->client->rewardTime = level.time + REWARD_SPRITE_TIME;

			} 
			if (player->client->pers.teamState.lastfraggedcarrier + 
				CTF_FRAG_CARRIER_ASSIST_TIMEOUT > level.time) {
				AddScore(player, ent->r.currentOrigin, CTF_FRAG_CARRIER_ASSIST_BONUS);
				other->client->pers.teamState.assists++;
				player->client->ps.persistant[PERS_ASSIST_COUNT]++;
				// add the sprite over the player's head
				player->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
				player->client->ps.eFlags |= EF_AWARD_ASSIST;
				player->client->rewardTime = level.time + REWARD_SPRITE_TIME;
			}
		}
	}
	Team_ResetFlags();

	CalculateRanks();

	return 0; // Do not respawn this automatically
}

int Team_TouchEnemyFlag( gentity_t *ent, gentity_t *other, int team ) {
	gclient_t *cl = other->client;

	if( g_gametype.integer == GT_1FCTF ) {
		PrintMsg (NULL, "%s" S_COLOR_WHITE " got the flag!\n", other->client->pers.netname );

                G_LogPrintf( "1FCTF: %i %i %i: %s got the flag!\n", cl->ps.clientNum, team, 0, cl->pers.netname);
                
		cl->ps.powerups[PW_NEUTRALFLAG] = INT_MAX; // flags never expire

		if( team == TEAM_RED ) {
			Team_SetFlagStatus( TEAM_FREE, FLAG_TAKEN_RED );
		}
		else {
			Team_SetFlagStatus( TEAM_FREE, FLAG_TAKEN_BLUE );
		}
	}
	else{
		PrintMsg (NULL, "%s" S_COLOR_WHITE " got the %s flag!\n",
			other->client->pers.netname, TeamName(team));
                
                if(g_gametype.integer == GT_CTF) {
                    G_LogPrintf( "CTF: %i %i %i: %s got the %s flag!\n", cl->ps.clientNum, team, 0, cl->pers.netname, TeamName(team));
                } else if(g_gametype.integer == GT_CTF_ELIMINATION) {
                    G_LogPrintf( "CTF_ELIMINATION: %i %i %i %i: %s got the %s flag!\n", level.roundNumber, cl->ps.clientNum, team, 0, cl->pers.netname, TeamName(team));
                }

		if (team == TEAM_RED)
			cl->ps.powerups[PW_REDFLAG] = INT_MAX; // flags never expire
		else
			cl->ps.powerups[PW_BLUEFLAG] = INT_MAX; // flags never expire

		Team_SetFlagStatus( team, FLAG_TAKEN );
	}

	AddScore(other, ent->r.currentOrigin, CTF_FLAG_BONUS);
	cl->pers.teamState.flagsince = level.time;
	Team_TakeFlagSound( ent, team );

	return g_flagrespawn.integer; // Do not respawn this automatically, but do delete it if it was FL_DROPPED
}

int Pickup_Team( gentity_t *ent, gentity_t *other ) {
	int team;
	gclient_t *cl = other->client;
	
	if( g_gametype.integer == GT_OBELISK ) {
		// there are no team items that can be picked up in obelisk
		G_FreeEntity( ent );
		return 0;
	}

	if( g_gametype.integer == GT_HARVESTER ) {
		// the only team items that can be picked up in harvester are the cubes
		if( ent->spawnflags != cl->sess.sessionTeam ) {
			cl->ps.generic1 += 1; //Skull pickedup
                        G_LogPrintf("HARVESTER: %i %i %i %i %i: %s picked up a skull.\n",
                            cl->ps.clientNum,cl->sess.sessionTeam,3,-1,1,
                            cl->pers.netname);
		} else {
                    G_LogPrintf("HARVESTER: %i %i %i %i %i: %s destroyed a skull.\n,",
                            cl->ps.clientNum,cl->sess.sessionTeam,2,-1,1,
                            cl->pers.netname);
                }
		G_FreeEntity( ent ); //Destory skull
		return 0;
	}
	if ( g_gametype.integer == GT_DOMINATION ) {
		Team_Dom_TakePoint(ent, cl->sess.sessionTeam,cl->ps.clientNum);
		return 0;
	}
	// figure out what team this flag is
	if( strcmp(ent->classname, "team_CTF_redflag") == 0 ) {
		team = TEAM_RED;
	}
	else if( strcmp(ent->classname, "team_CTF_blueflag") == 0 ) {
		team = TEAM_BLUE;
	}
	else if( strcmp(ent->classname, "team_CTF_neutralflag") == 0  ) {
		team = TEAM_FREE;
	}
	else {
		PrintMsg ( other, "Don't know what team the flag is on.\n");
		return 0;
	}
	if( g_gametype.integer == GT_1FCTF ) {
		if( team == TEAM_FREE ) {
			return Team_TouchEnemyFlag( ent, other, cl->sess.sessionTeam );
		}
		if( team != cl->sess.sessionTeam) {
			return Team_TouchOurFlag( ent, other, cl->sess.sessionTeam );
		}
		return 0;
	}
	if( g_gametype.integer == GT_DOUBLE_D) {
		return Team_TouchDoubleDominationPoint( ent, other, team );
	}
	// GT_CTF
	if( team == cl->sess.sessionTeam) {
		return Team_TouchOurFlag( ent, other, team );
	}
	return Team_TouchEnemyFlag( ent, other, team );
}

/*
===========
Team_GetLocation

Report a location for the player. Uses placed nearby target_location entities
============
*/
gentity_t *Team_GetLocation(gentity_t *ent)
{
	gentity_t		*eloc, *best;
	float			bestlen, len;
	vec3_t			origin;

	best = NULL;
	bestlen = 3*8192.0*8192.0;

	VectorCopy( ent->r.currentOrigin, origin );

	for (eloc = level.locationHead; eloc; eloc = eloc->nextTrain) {
		len = ( origin[0] - eloc->r.currentOrigin[0] ) * ( origin[0] - eloc->r.currentOrigin[0] )
			+ ( origin[1] - eloc->r.currentOrigin[1] ) * ( origin[1] - eloc->r.currentOrigin[1] )
			+ ( origin[2] - eloc->r.currentOrigin[2] ) * ( origin[2] - eloc->r.currentOrigin[2] );

		if ( len > bestlen ) {
			continue;
		}

		if ( !trap_InPVS( origin, eloc->r.currentOrigin ) ) {
			continue;
		}

		bestlen = len;
		best = eloc;
	}

	return best;
}


/*
===========
Team_GetLocation

Report a location for the player. Uses placed nearby target_location entities
============
*/
qboolean Team_GetLocationMsg(gentity_t *ent, char *loc, int loclen)
{
	gentity_t *best;

	best = Team_GetLocation( ent );
	
	if (!best)
		return qfalse;

	if (best->count) {
		if (best->count < 0)
			best->count = 0;
		if (best->count > 7)
			best->count = 7;
		Com_sprintf(loc, loclen, "%c%c%s" S_COLOR_WHITE, Q_COLOR_ESCAPE, best->count + '0', best->message );
	} else
		Com_sprintf(loc, loclen, "%s", best->message);

	return qtrue;
}


/*---------------------------------------------------------------------------*/

/*
================
SelectRandomDeathmatchSpawnPoint

go to a random point that doesn't telefrag
================
*/
#define	MAX_TEAM_SPAWN_POINTS	32
gentity_t *SelectRandomTeamSpawnPoint( int teamstate, team_t team ) {
	gentity_t	*spot;
	int			count;
	int			selection;
	gentity_t	*spots[MAX_TEAM_SPAWN_POINTS];
	char		*classname;

	if(g_gametype.integer == GT_ELIMINATION) { //change sides every round
		if((level.roundNumber+level.eliminationSides)%2==1){
			if(team == TEAM_RED)
				team = TEAM_BLUE;
			else if(team == TEAM_BLUE)
				team = TEAM_RED;
		}
	}

	if (teamstate == TEAM_BEGIN) {
		if (team == TEAM_RED)
			classname = "team_CTF_redplayer";
		else if (team == TEAM_BLUE)
			classname = "team_CTF_blueplayer";
		else
			return NULL;
	} else {
		if (team == TEAM_RED)
			classname = "team_CTF_redspawn";
		else if (team == TEAM_BLUE)
			classname = "team_CTF_bluespawn";
		else
			return NULL;
	}
	count = 0;

	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), classname)) != NULL) {
		if ( SpotWouldTelefrag( spot ) ) {
			continue;
		}
		spots[ count ] = spot;
		if (++count == MAX_TEAM_SPAWN_POINTS)
			break;
	}

	if ( !count ) {	// no spots that won't telefrag
		return G_Find( NULL, FOFS(classname), classname);
	}

	selection = rand() % count;
	return spots[ selection ];
}

/*
================
SelectRandomDDSpawnPoint

go to a random Double Domination Spawn Point
================
*/
#define	MAX_TEAM_SPAWN_POINTS	32
gentity_t *SelectRandomDDSpawnPoint( void ) {
	gentity_t	*spot;
	int			count;
	int			selection;
	gentity_t	*spots[MAX_TEAM_SPAWN_POINTS];
	char		*classname;

	
	classname = "info_player_dd";
		
	count = 0;

	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), classname)) != NULL) {
		if ( SpotWouldTelefrag( spot ) ) {
			continue;
		}
		spots[ count ] = spot;
		if (++count == MAX_TEAM_SPAWN_POINTS)
			break;
	}

	if ( !count ) {	// no spots that won't telefrag
		return G_Find( NULL, FOFS(classname), classname);
	}

	selection = rand() % count;
	return spots[ selection ];
}

gentity_t *SelectRandomTeamDDSpawnPoint( team_t team ) {
	gentity_t	*spot;
	int			count;
	int			selection;
	gentity_t	*spots[MAX_TEAM_SPAWN_POINTS];
	char		*classname;

	if(team == TEAM_RED)
            classname = "info_player_dd_red";
        else
            classname = "info_player_dd_blue";
		
	count = 0;

	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), classname)) != NULL) {
		if ( SpotWouldTelefrag( spot ) ) {
			continue;
		}
		spots[ count ] = spot;
		if (++count == MAX_TEAM_SPAWN_POINTS)
			break;
	}

	if ( !count ) {	// no spots that won't telefrag
		return G_Find( NULL, FOFS(classname), classname);
	}

	selection = rand() % count;
	return spots[ selection ];
}


/*
===========
SelectCTFSpawnPoint

============
*/
gentity_t *SelectCTFSpawnPoint ( team_t team, int teamstate, vec3_t origin, vec3_t angles ) {
	gentity_t	*spot;

	spot = SelectRandomTeamSpawnPoint ( teamstate, team );


	if (!spot) {
		return SelectSpawnPoint( vec3_origin, origin, angles );
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
}

/*
===========
SelectDoubleDominationSpawnPoint

============
*/
gentity_t *SelectDoubleDominationSpawnPoint ( team_t team, vec3_t origin, vec3_t angles ) {
	gentity_t	*spot;

        spot = SelectRandomTeamDDSpawnPoint( team );
        
        if(!spot) {
            spot = SelectRandomDDSpawnPoint ( );
        }
        
	if (!spot) {
		return SelectSpawnPoint( vec3_origin, origin, angles );
	}

	VectorCopy (spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy (spot->s.angles, angles);

	return spot;
}

/*---------------------------------------------------------------------------*/

static int QDECL SortClients( const void *a, const void *b ) {
	return *(int *)a - *(int *)b;
}


/*
==================
TeamplayLocationsMessage

Format:
	clientNum location health armor weapon powerups

==================
*/
void TeamplayInfoMessage( gentity_t *ent ) {
	char		entry[1024];
	char		string[8192];
	int			stringlength;
	int			i, j;
	gentity_t	*player;
	int			cnt;
	int			h, a, w;
	int			clients[TEAM_MAXOVERLAY];

	if ( ! ent->client->pers.teamInfo )
		return;

	// figure out what client should be on the display
	// we are limited to 8, but we want to use the top eight players
	// but in client order (so they don't keep changing position on the overlay)
	for (i = 0, cnt = 0; i < g_maxclients.integer && cnt < TEAM_MAXOVERLAY; i++) {
		player = g_entities + level.sortedClients[i];
		if (player->inuse && player->client->sess.sessionTeam == 
			ent->client->sess.sessionTeam ) {
			clients[cnt++] = level.sortedClients[i];
		}
	}

	// We have the top eight players, sort them by clientNum
	qsort( clients, cnt, sizeof( clients[0] ), SortClients );

	// send the latest information on all clients
	string[0] = 0;
	stringlength = 0;

	for (i = 0, cnt = 0; i < g_maxclients.integer && cnt < TEAM_MAXOVERLAY; i++) {
		player = g_entities + i;
		if (player->inuse && player->client->sess.sessionTeam == 
			ent->client->sess.sessionTeam ) {

			h = player->client->ps.stats[STAT_HEALTH];
			a = player->client->ps.stats[STAT_ARMOR];
			w = player->client->ps.generic2;
			if(player->client->isEliminated)
			{
				h = 0;
				a = 0;
				w = 0;
			}			
			if (h < 0) h = 0;
			if (a < 0) a = 0;

			Com_sprintf (entry, sizeof(entry),
				" %i %i %i %i %i %i", 
//				level.sortedClients[i], player->client->pers.teamState.location, h, a, 
				i, player->client->pers.teamState.location, h, a, 
				w, player->s.powerups);
			j = strlen(entry);
			if (stringlength + j > sizeof(string))
				break;
			strcpy (string + stringlength, entry);
			stringlength += j;
			cnt++;
		}
	}

	trap_SendServerCommand( ent-g_entities, va("tinfo %i %s", cnt, string) );
}

void CheckTeamStatus(void) {
	int i;
	gentity_t *loc, *ent;

	if (level.time - level.lastTeamLocationTime > TEAM_LOCATION_UPDATE_TIME) {

		level.lastTeamLocationTime = level.time;

		for (i = 0; i < g_maxclients.integer; i++) {
			ent = g_entities + i;

			if ( ent->client->pers.connected != CON_CONNECTED ) {
				continue;
			}

			if (ent->inuse && (ent->client->sess.sessionTeam == TEAM_RED ||	ent->client->sess.sessionTeam == TEAM_BLUE)) {
				loc = Team_GetLocation( ent );
				if (loc)
					ent->client->pers.teamState.location = loc->health;
				else
					ent->client->pers.teamState.location = 0;
			}
		}

		for (i = 0; i < g_maxclients.integer; i++) {
			ent = g_entities + i;

			if ( ent->client->pers.connected != CON_CONNECTED ) {
				continue;
			}

			if (ent->inuse && (ent->client->sess.sessionTeam == TEAM_RED ||	ent->client->sess.sessionTeam == TEAM_BLUE)) {
				TeamplayInfoMessage( ent );
			}
		}
	}
}

/*-----------------------------------------------------------------*/

/*QUAKED team_CTF_redplayer (1 0 0) (-16 -16 -16) (16 16 32)
Only in CTF games.  Red players spawn here at game start.
*/
void SP_team_CTF_redplayer( gentity_t *ent ) {
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );
	ent->classname = "team_CTF_redplayer";
	ent->s.eType = ET_GENERAL;
	ent->s.pos.trType = TR_STATIONARY;
	VectorSet( ent->r.mins, -10, -10, -10);
	VectorSet( ent->r.maxs, 10, 10, 10 );
	ent->r.contents = CONTENTS_TRIGGER;
	//ent->s.modelindex = G_ModelIndex( "45.md3" );
	
	trap_LinkEntity( ent );
}


/*QUAKED team_CTF_blueplayer (0 0 1) (-16 -16 -16) (16 16 32)
Only in CTF games.  Blue players spawn here at game start.
*/
void SP_team_CTF_blueplayer( gentity_t *ent ) {
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );
	ent->classname = "team_CTF_blueplayer";
	ent->s.eType = ET_GENERAL;
	ent->s.pos.trType = TR_STATIONARY;
	VectorSet( ent->r.mins, -10, -10, -10);
	VectorSet( ent->r.maxs, 10, 10, 10 );
	ent->r.contents = CONTENTS_TRIGGER;
	//ent->s.modelindex = G_ModelIndex( "45.md3" );
	
	trap_LinkEntity( ent );
}


/*QUAKED team_CTF_redspawn (1 0 0) (-16 -16 -24) (16 16 32)
potential spawning position for red team in CTF games.
Targets will be fired when someone spawns in on them.
*/
void SP_team_CTF_redspawn(gentity_t *ent) {
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );
	ent->classname = "team_CTF_redspawn";
	ent->s.eType = ET_GENERAL;
	ent->s.pos.trType = TR_STATIONARY;
	VectorSet( ent->r.mins, -10, -10, -10);
	VectorSet( ent->r.maxs, 10, 10, 10 );
	ent->r.contents = CONTENTS_TRIGGER;
	//ent->s.modelindex = G_ModelIndex( "45.md3" );
	
	trap_LinkEntity( ent );
}

/*QUAKED team_CTF_bluespawn (0 0 1) (-16 -16 -24) (16 16 32)
potential spawning position for blue team in CTF games.
Targets will be fired when someone spawns in on them.
*/
void SP_team_CTF_bluespawn(gentity_t *ent) {
	VectorCopy( ent->s.origin, ent->s.pos.trBase );
	VectorCopy( ent->s.origin, ent->r.currentOrigin );
	ent->classname = "team_CTF_bluespawn";
	ent->s.eType = ET_GENERAL;
	ent->s.pos.trType = TR_STATIONARY;
	VectorSet( ent->r.mins, -10, -10, -10);
	VectorSet( ent->r.maxs, 10, 10, 10 );
	ent->r.contents = CONTENTS_TRIGGER;
	//ent->s.modelindex = G_ModelIndex( "45.md3" );
	
	trap_LinkEntity( ent );
}

/*
================
Obelisks
================
*/

static void ObeliskHealthChange( int team, int health ) {
    int currentPercentage;
    int percentage = (health*100)/g_obeliskHealth.integer;
    if(percentage < 0)
        percentage = 0;
    currentPercentage = level.healthRedObelisk;
    if(team != TEAM_RED)
        currentPercentage = level.healthBlueObelisk;
    if(percentage != currentPercentage) {
        if(team == TEAM_RED) {
            level.healthRedObelisk = percentage;
        } else {
            level.healthBlueObelisk = percentage;
        }
        level.MustSendObeliskHealth = qtrue;
        //G_Printf("Obelisk health %i %i\n",team,percentage);
        ObeliskHealthMessage();
    }
} 

static void ObeliskRegen( gentity_t *self ) {
	self->nextthink = level.time + g_obeliskRegenPeriod.integer * 1000;
        ObeliskHealthChange(self->spawnflags,self->health);
	if( self->health >= g_obeliskHealth.integer ) {
		return;
	}

	G_AddEvent( self, EV_POWERUP_REGEN, 0 );
	self->health += g_obeliskRegenAmount.integer;
	if ( self->health > g_obeliskHealth.integer ) {
		self->health = g_obeliskHealth.integer;
	}

	self->activator->s.modelindex2 = self->health * 0xff / g_obeliskHealth.integer;
	self->activator->s.frame = 0;
}


static void ObeliskRespawn( gentity_t *self ) {
	self->takedamage = qtrue;
	self->health = g_obeliskHealth.integer;

	self->think = ObeliskRegen;
	self->nextthink = level.time + g_obeliskRegenPeriod.integer * 1000;

	self->activator->s.frame = 0;
}


static void ObeliskDie( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod ) {
	int			otherTeam;

	otherTeam = OtherTeam( self->spawnflags );
	AddTeamScore(self->s.pos.trBase, otherTeam, 1);
	Team_ForceGesture(otherTeam);

	CalculateRanks();

	self->takedamage = qfalse;
	self->think = ObeliskRespawn;
	self->nextthink = level.time + g_obeliskRespawnDelay.integer * 1000;

	self->activator->s.modelindex2 = 0xff;
	self->activator->s.frame = 2;

	G_AddEvent( self->activator, EV_OBELISKEXPLODE, 0 );

	AddScore(attacker, self->r.currentOrigin, CTF_CAPTURE_BONUS);

	// add the sprite over the player's head
	attacker->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
	attacker->client->ps.eFlags |= EF_AWARD_CAP;
	attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;
	attacker->client->ps.persistant[PERS_CAPTURES]++;
    G_LogPrintf("OBELISK: %i %i %i %i: %s destroyed the enemy obelisk.\n",
    attacker->client->ps.clientNum,attacker->client->sess.sessionTeam,3,0,
    attacker->client->pers.netname);
    ObeliskHealthChange(self->spawnflags,self->health);
	teamgame.redObeliskAttackedTime = 0;
	teamgame.blueObeliskAttackedTime = 0;
}


static void ObeliskTouch( gentity_t *self, gentity_t *other, trace_t *trace ) {
	int			tokens,i;

	if ( !other->client ) {
		return;
	}

	if ( OtherTeam(other->client->sess.sessionTeam) != self->spawnflags ) {
		return;
	}

	tokens = other->client->ps.generic1;
	if( tokens <= 0 ) {
		return;
	}

	PrintMsg(NULL, "%s" S_COLOR_WHITE " brought in %i skull%s.\n",
					other->client->pers.netname, tokens, tokens>1 ? "s" : "" );

        G_LogPrintf("HARVESTER: %i %i %i %i %i: %s brought in %i skull%s for %s\n",
            other->client->ps.clientNum,other->client->sess.sessionTeam,0,-1,tokens,
            other->client->pers.netname,tokens, tokens>1 ? "s" : "", TeamName(other->client->sess.sessionTeam));

	AddTeamScore(self->s.pos.trBase, other->client->sess.sessionTeam, tokens);
	Team_ForceGesture(other->client->sess.sessionTeam);

	AddScore(other, self->r.currentOrigin, CTF_CAPTURE_BONUS*tokens);

	// add the sprite over the player's head
	other->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
	other->client->ps.eFlags |= EF_AWARD_CAP;
	other->client->rewardTime = level.time + REWARD_SPRITE_TIME;
	other->client->ps.persistant[PERS_CAPTURES] += tokens;
	other->client->ps.generic1 = 0;
	CalculateRanks();

	Team_CaptureFlagSound( self, self->spawnflags );
}

static void ObeliskPain( gentity_t *self, gentity_t *attacker, int damage ) {
	int actualDamage = damage / 10;
	if (actualDamage <= 0) {
		actualDamage = 1;
	}
	self->activator->s.modelindex2 = self->health * 0xff / g_obeliskHealth.integer;
	if (!self->activator->s.frame) {
		G_AddEvent(self, EV_OBELISKPAIN, 0);
	}
	self->activator->s.frame = 1;
	AddScore(attacker, self->r.currentOrigin, actualDamage);
        G_LogPrintf("OBELISK: %i %i %i %i: %s dealt %i damage to the enemy obelisk.\n",
                 attacker->client->ps.clientNum,attacker->client->sess.sessionTeam,1,actualDamage,
                 attacker->client->pers.netname,actualDamage);
}

gentity_t *SpawnObelisk( vec3_t origin, int team, int spawnflags) {
	trace_t		tr;
	vec3_t		dest;
	gentity_t	*ent;

	ent = G_Spawn();

	VectorCopy( origin, ent->s.origin );
	VectorCopy( origin, ent->s.pos.trBase );
	VectorCopy( origin, ent->r.currentOrigin );

	VectorSet( ent->r.mins, -15, -15, 0 );
	VectorSet( ent->r.maxs, 15, 15, 87 );

	ent->s.eType = ET_GENERAL;
	ent->flags = FL_NO_KNOCKBACK;

	if( g_gametype.integer == GT_OBELISK ) {
		ent->r.contents = CONTENTS_SOLID;
		ent->takedamage = qtrue;
		ent->health = g_obeliskHealth.integer;
		ent->die = ObeliskDie;
		ent->pain = ObeliskPain;
		ent->think = ObeliskRegen;
		ent->nextthink = level.time + g_obeliskRegenPeriod.integer * 1000;
	}
	if( g_gametype.integer == GT_HARVESTER ) {
		ent->r.contents = CONTENTS_TRIGGER;
		ent->touch = ObeliskTouch;
	}

	if ( spawnflags & 1 ) {
		// suspended
		G_SetOrigin( ent, ent->s.origin );
	} else {
		// mappers like to put them exactly on the floor, but being coplanar
		// will sometimes show up as starting in solid, so lif it up one pixel
		ent->s.origin[2] += 1;

		// drop to floor
		VectorSet( dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 4096 );
		trap_Trace( &tr, ent->s.origin, ent->r.mins, ent->r.maxs, dest, ent->s.number, MASK_SOLID );
		if ( tr.startsolid ) {
			ent->s.origin[2] -= 1;
                        G_Printf( "SpawnObelisk: %s startsolid at %s\n", ent->classname, vtos(ent->s.origin) );

			ent->s.groundEntityNum = ENTITYNUM_NONE;
			G_SetOrigin( ent, ent->s.origin );
		}
		else {
			// allow to ride movers
			ent->s.groundEntityNum = tr.entityNum;
			G_SetOrigin( ent, tr.endpos );
		}
	}

	ent->spawnflags = team;

	trap_LinkEntity( ent );

	return ent;
}

/*QUAKED team_redobelisk (1 0 0) (-16 -16 0) (16 16 8)
*/
void SP_team_redobelisk( gentity_t *ent ) {
	gentity_t *obelisk;

	if ( g_gametype.integer <= GT_TEAM || g_ffa_gt>0) {
		G_FreeEntity(ent);
		return;
	}
	ent->s.eType = ET_TEAM;
	if ( g_gametype.integer == GT_OBELISK ) {
		obelisk = SpawnObelisk( ent->s.origin, TEAM_RED, ent->spawnflags );
		obelisk->activator = ent;
		// initial obelisk health value
		ent->s.modelindex2 = 0xff;
		ent->s.frame = 0;
	}
	if ( g_gametype.integer == GT_HARVESTER ) {
		obelisk = SpawnObelisk( ent->s.origin, TEAM_RED, ent->spawnflags );
		obelisk->activator = ent;
	}
	ent->s.modelindex = TEAM_RED;
	trap_LinkEntity(ent);
}

/*QUAKED team_blueobelisk (0 0 1) (-16 -16 0) (16 16 88)
*/
void SP_team_blueobelisk( gentity_t *ent ) {
	gentity_t *obelisk;

	if ( g_gametype.integer <= GT_TEAM || g_ffa_gt>0) {
		G_FreeEntity(ent);
		return;
	}
	ent->s.eType = ET_TEAM;
	if ( g_gametype.integer == GT_OBELISK ) {
		obelisk = SpawnObelisk( ent->s.origin, TEAM_BLUE, ent->spawnflags );
		obelisk->activator = ent;
		// initial obelisk health value
		ent->s.modelindex2 = 0xff;
		ent->s.frame = 0;
	}
	if ( g_gametype.integer == GT_HARVESTER ) {
		obelisk = SpawnObelisk( ent->s.origin, TEAM_BLUE, ent->spawnflags );
		obelisk->activator = ent;
	}
	ent->s.modelindex = TEAM_BLUE;
	trap_LinkEntity(ent);
}

/*QUAKED team_neutralobelisk (0 0 1) (-16 -16 0) (16 16 88)
*/
void SP_team_neutralobelisk( gentity_t *ent ) {
	if ( g_gametype.integer != GT_1FCTF && g_gametype.integer != GT_HARVESTER ) {
		G_FreeEntity(ent);
		return;
	}
	ent->s.eType = ET_TEAM;
	if ( g_gametype.integer == GT_HARVESTER) {
		neutralObelisk = SpawnObelisk( ent->s.origin, TEAM_FREE, ent->spawnflags);
		neutralObelisk->spawnflags = TEAM_FREE;
	}
	ent->s.modelindex = TEAM_FREE;
	trap_LinkEntity(ent);
}


/*
================
CheckObeliskAttack
================
*/
qboolean CheckObeliskAttack( gentity_t *obelisk, gentity_t *attacker ) {
	gentity_t	*te;

	// if this really is an obelisk
	if( obelisk->die != ObeliskDie ) {
		return qfalse;
	}

	// if the attacker is a client
	if( !attacker->client ) {
		return qfalse;
	}

	// if the obelisk is on the same team as the attacker then don't hurt it
	if( obelisk->spawnflags == attacker->client->sess.sessionTeam ) {
		return qtrue;
	}

	// obelisk may be hurt

	// if not played any sounds recently
	if ((obelisk->spawnflags == TEAM_RED &&
		teamgame.redObeliskAttackedTime < level.time - OVERLOAD_ATTACK_BASE_SOUND_TIME) ||
		(obelisk->spawnflags == TEAM_BLUE &&
		teamgame.blueObeliskAttackedTime < level.time - OVERLOAD_ATTACK_BASE_SOUND_TIME) ) {

		// tell which obelisk is under attack
		te = G_TempEntity( obelisk->s.pos.trBase, EV_GLOBAL_TEAM_SOUND );
		if( obelisk->spawnflags == TEAM_RED ) {
			te->s.eventParm = GTS_REDOBELISK_ATTACKED;
			teamgame.redObeliskAttackedTime = level.time;
		}
		else {
			te->s.eventParm = GTS_BLUEOBELISK_ATTACKED;
			teamgame.blueObeliskAttackedTime = level.time;
		}
		te->r.svFlags |= SVF_BROADCAST;
	}

	return qfalse;
}

/*
================
ShuffleTeams
 *Randomizes the teams based on a type of function and then restarts the map
 *Currently there is only one type so type is ignored. You can add total randomizaion or waighted randomization later.
 *
 *The function will split the teams like this:
 *1. Red team
 *2. Blue team
 *3. Blue team
 *4. Red team
 *5. Go to 1
================
*/
void ShuffleTeams(void) {
    int i;
    int assignedClients=1, nextTeam=TEAM_RED;

    if ( g_gametype.integer < GT_TEAM || g_ffa_gt==1)
        return; //Can only shuffle team games!

    for( i=0;i < level.numConnectedClients; i++ ) {
        if( g_entities[ &level.clients[level.sortedClients[i]] - level.clients].r.svFlags & SVF_BOT)
            continue; //Don't sort bots... they are always equal
        
        if(level.clients[level.sortedClients[i]].sess.sessionTeam==TEAM_RED || level.clients[level.sortedClients[i]].sess.sessionTeam==TEAM_BLUE ) {
            //For every second client we chenge team. But we do it a little of to make it slightly more fair
            if(assignedClients>1) {
                assignedClients=0;
                if(nextTeam == TEAM_RED)
                    nextTeam = TEAM_BLUE;
                else
                    nextTeam = TEAM_RED;
            }

            //Set the team
            //We do not run all the logic because we shall run map_restart in a moment.
            level.clients[level.sortedClients[i]].sess.sessionTeam = nextTeam;

            ClientUserinfoChanged( level.sortedClients[i] );
            ClientBegin( level.sortedClients[i] );

            assignedClients++;
        }
    }

    //Restart!
    trap_SendConsoleCommand( EXEC_APPEND, "map_restart 0\n" );

}

