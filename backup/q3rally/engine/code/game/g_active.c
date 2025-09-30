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
//

#include "g_local.h"


// STONELANCE
/*
=================
Com_LogPrintf

Print to the logfile
=================
*/
static __attribute__ ((format (printf, 1, 2))) void QDECL Com_LogPrintf( const char *fmt, ... ) {
	va_list			argptr;
	char			string[1024];
	fileHandle_t	logFile;

	trap_FS_FOpenFile( "g_physics.log", &logFile, FS_APPEND );

	va_start( argptr, fmt );
	Q_vsnprintf (string, sizeof(string), fmt, argptr);
	va_end( argptr );

	trap_FS_Write( string, strlen( string ), logFile );

	trap_FS_FCloseFile( logFile );
}

/*
================================================================================
G_DebugDynamics
================================================================================
*/
void G_DebugDynamics( carBody_t *body, carPoint_t *points, int i ){
	Com_LogPrintf("\n");
	Com_LogPrintf("PM_DebugDynamics: point ORIGIN - %.3f, %.3f, %.3f\n", points[i].r[0], points[i].r[1], points[i].r[2]);
	Com_LogPrintf("PM_DebugDynamics: point VELOCITY - %.3f, %.3f, %.3f\n", points[i].v[0], points[i].v[1], points[i].v[2]);

	Com_LogPrintf("PM_DebugDynamics: point ONGROUND - %i\n", points[i].onGround);
	Com_LogPrintf("PM_DebugDynamics: point NORMAL 1- %.3f, %.3f, %.3f\n", points[i].normals[0][0], points[i].normals[0][1], points[i].normals[0][2]);
	Com_LogPrintf("PM_DebugDynamics: point NORMAL 2- %.3f, %.3f, %.3f\n", points[i].normals[1][0], points[i].normals[1][1], points[i].normals[1][2]);
	Com_LogPrintf("PM_DebugDynamics: point NORMAL 3- %.3f, %.3f, %.3f\n", points[i].normals[2][0], points[i].normals[2][1], points[i].normals[2][2]);
//	Com_LogPrintf("PM_DebugDynamics: point MASS - %.3f\n", points[pm->pDebug-1].mass);

	Com_LogPrintf("PM_DebugDynamics: body ORIGIN - %.3f, %.3f, %.3f\n", body->r[0], body->r[1], body->r[2]);
	Com_LogPrintf("PM_DebugDynamics: body VELOCITY - %.3f, %.3f, %.3f\n", body->v[0], body->v[1], body->v[2]);
	Com_LogPrintf("PM_DebugDynamics: body ANG VELOCITY - %.3f, %.3f, %.3f\n", body->w[0], body->w[1], body->w[2]);

//	Com_LogPrintf("PM_DebugDynamics: body COM - %.3f, %.3f, %.3f\n", body->CoM[0], body->CoM[1], body->CoM[2]);
//	Com_LogPrintf("PM_DebugDynamics: body MASS - %.3f\n", body->mass);

	Com_LogPrintf("Direction vectors ----------------------------------------\n");

	Com_LogPrintf("PM_DebugDynamics: body FORWARD - %.3f, %.3f, %.3f\n", body->forward[0], body->forward[1], body->forward[2]);
	Com_LogPrintf("PM_DebugDynamics: body RIGHT - %.3f, %.3f, %.3f\n", body->right[0], body->right[1], body->right[2]);
	Com_LogPrintf("PM_DebugDynamics: body UP - %.3f, %.3f, %.3f\n", body->up[0], body->up[1], body->up[2]);
	Com_LogPrintf("\n");
}


/*
================================================================================
G_DebugForces
================================================================================
*/
void G_DebugForces( carBody_t *body, carPoint_t *points, int i ){
	Com_LogPrintf("\n");
	Com_LogPrintf("PM_DebugForces: point GRAVITY - %.3f, %.3f, %.3f\n", points[i].forces[GRAVITY][0], points[i].forces[GRAVITY][1], points[i].forces[GRAVITY][2]);
	Com_LogPrintf("PM_DebugForces: point NORMAL - %.3f, %.3f, %.3f\n", points[i].forces[NORMAL][0], points[i].forces[NORMAL][1], points[i].forces[NORMAL][2]);
	Com_LogPrintf("PM_DebugForces: point SHOCK - %.3f, %.3f, %.3f\n", points[i].forces[SHOCK][0], points[i].forces[SHOCK][1], points[i].forces[SHOCK][2]);
	Com_LogPrintf("PM_DebugForces: point SPRING - %.3f, %.3f, %.3f\n", points[i].forces[SPRING][0], points[i].forces[SPRING][1], points[i].forces[SPRING][2]);
	Com_LogPrintf("PM_DebugForces: point SWAY_BAR - %.3f, %.3f, %.3f\n", points[i].forces[SWAY_BAR][0], points[i].forces[SWAY_BAR][1], points[i].forces[SWAY_BAR][2]);
	Com_LogPrintf("PM_DebugForces: point ROAD - %.3f, %.3f, %.3f\n", points[i].forces[ROAD][0], points[i].forces[ROAD][1], points[i].forces[ROAD][2]);
	Com_LogPrintf("PM_DebugForces: point INTERNAL - %.3f, %.3f, %.3f\n", points[i].forces[INTERNAL][0], points[i].forces[INTERNAL][1], points[i].forces[INTERNAL][2]);
	Com_LogPrintf("PM_DebugForces: point AIR_FRICTION - %.3f, %.3f, %.3f\n", points[i].forces[AIR_FRICTION][0], points[i].forces[AIR_FRICTION][1], points[i].forces[AIR_FRICTION][2]);

	Com_LogPrintf("PM_DebugForces: point NETFORCE - %.3f, %.3f, %.3f\n", points[i].netForce[0], points[i].netForce[1], points[i].netForce[2]);

	Com_LogPrintf("PM_DebugForces: point fluidDensity - %.6f\n", points[i].fluidDensity);

	Com_LogPrintf("PM_DebugForces: body netForce - %.3f, %.3f, %.3f\n", body->netForce[0], body->netForce[1], body->netForce[2]);
	Com_LogPrintf("PM_DebugForces: body netMoment - %.3f, %.3f, %.3f\n", body->netMoment[0], body->netMoment[1], body->netMoment[2]);
	Com_LogPrintf("\n");
}
// END

/*
===============
G_DamageFeedback

Called just before a snapshot is sent to the given player.
Totals up all damage and generates both the player_state_t
damage values to that client for pain blends and kicks, and
global pain sound events for all clients.
===============
*/
void P_DamageFeedback( gentity_t *player ) {
	gclient_t	*client;
	float	count;
// STONELANCE
//	vec3_t	angles;
// END

	client = player->client;
	if ( client->ps.pm_type == PM_DEAD ) {
		return;
	}

	// total points of damage shot at the player this frame
	count = client->damage_blood + client->damage_armor;
	if ( count == 0 ) {
		return;		// didn't take any damage
	}

	if ( count > 255 ) {
		count = 255;
	}

	// send the information to the client

	// world damage (falling, slime, etc) uses a special code
	// to make the blend blob centered instead of positional
// STONELANCE
/*
	if ( client->damage_fromWorld ) {
		client->ps.damagePitch = 255;
		client->ps.damageYaw = 255;

		client->damage_fromWorld = qfalse;
	} else {
		vectoangles( client->damage_from, angles );
		client->ps.damagePitch = angles[PITCH]/360.0 * 256;
		client->ps.damageYaw = angles[YAW]/360.0 * 256;
	}
*/
// END

	// play an appropriate pain sound
	if ( (level.time > player->pain_debounce_time) && !(player->flags & FL_GODMODE) ) {
		player->pain_debounce_time = level.time + 700;
		G_AddEvent( player, EV_PAIN, player->health );
// STONELANCE
//		client->ps.damageEvent++;
// END
	}

// STONELANCE
//	client->ps.damageCount = count;
// END

	//
	// clear totals
	//
	client->damage_blood = 0;
	client->damage_armor = 0;
	client->damage_knockback = 0;
}



/*
=============
P_WorldEffects

Check for lava / slime contents and drowning
=============
*/
void P_WorldEffects( gentity_t *ent ) {
	qboolean	envirosuit;
	int			waterlevel;

	if ( ent->client->noclip ) {
		ent->client->airOutTime = level.time + 12000;	// don't need air
		return;
	}

	waterlevel = ent->waterlevel;

	envirosuit = ent->client->ps.powerups[PW_BATTLESUIT] > level.time;

	//
	// check for drowning
	//
	if ( waterlevel == 3 ) {
		// envirosuit give air
// STONELANCE envirosuit doesnt help a car
/*
		if ( envirosuit ) {
			ent->client->airOutTime = level.time + 10000;
		}
*/
// END

		// if out of air, start drowning
		if ( ent->client->airOutTime < level.time) {
			// drown!
// STONELANCE - damage a lot fast for a car
//			ent->client->airOutTime += 1000;
			ent->client->airOutTime += 200;
// END
			if ( ent->health > 0 ) {
				// take more damage the longer underwater
				ent->damage += 2;
				if (ent->damage > 15)
					ent->damage = 15;

				// don't play a normal pain sound
				ent->pain_debounce_time = level.time + 200;

				G_Damage (ent, NULL, NULL, NULL, NULL, 
					ent->damage, DAMAGE_NO_ARMOR, MOD_WATER);
			}
		}
	} else {
// STONELANCE
//		ent->client->airOutTime = level.time + 12000;
		ent->client->airOutTime = level.time + 200;
		ent->damage = 2;
// END
	}

	//
	// check for sizzle damage (move to pmove?)
	//
	if (waterlevel && 
		(ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)) ) {
		if (ent->health > 0
			&& ent->pain_debounce_time <= level.time	) {

			if ( envirosuit ) {
				G_AddEvent( ent, EV_POWERUP_BATTLESUIT, 0 );
			} else {
				if (ent->watertype & CONTENTS_LAVA) {
					G_Damage (ent, NULL, NULL, NULL, NULL, 
						30*waterlevel, 0, MOD_LAVA);
				}

				if (ent->watertype & CONTENTS_SLIME) {
					G_Damage (ent, NULL, NULL, NULL, NULL, 
						10*waterlevel, 0, MOD_SLIME);
				}
			}
		}
	}
}



/*
===============
G_SetClientSound
===============
*/
void G_SetClientSound( gentity_t *ent ) {
#ifdef MISSIONPACK
	if( ent->s.eFlags & EF_TICKING ) {
		ent->client->ps.loopSound = G_SoundIndex( "sound/weapons/proxmine/wstbtick.wav");
	}
	else
#endif
	if (ent->waterlevel && (ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)) ) {
		ent->client->ps.loopSound = level.snd_fry;
	} else {
		ent->client->ps.loopSound = 0;
	}
}



//==============================================================

/*
==============
ClientImpacts
==============
*/
void ClientImpacts( gentity_t *ent, pmove_t *pm ) {
	int		i, j;
	trace_t	trace;
	gentity_t	*other;

	memset( &trace, 0, sizeof( trace ) );
	for (i=0 ; i<pm->numtouch ; i++) {
		for (j=0 ; j<i ; j++) {
			if (pm->touchents[j] == pm->touchents[i] ) {
				break;
			}
		}
		if (j != i) {
			continue;	// duplicated
		}
		other = &g_entities[ pm->touchents[i] ];

// STONELANCE
		VectorSubtract( other->s.origin, pm->touchPos[i], trace.plane.normal );
		VectorNormalize( trace.plane.normal );
		VectorCopy(pm->touchPos[i], trace.endpos);
// END

		if ( ( ent->r.svFlags & SVF_BOT ) && ( ent->touch ) ) {
			ent->touch( ent, other, &trace );
		}

		if ( !other->touch ) {
			continue;
		}

		other->touch( other, ent, &trace );
	}

}

/*
int entitiesInTracedBox( const vec3_t mins, const vec3_t maxs, const vec3_t start, const vec3_t end, int *list, int maxcount )
{
	int			num;
	int			ents[MAX_GENTITIES];
	int			numInMovingBox;
	vec3_t		curMins;
	vec3_t		curMaxs;

	vecCopy( mins, curMins );
	vecCopy( maxs, curMaxs );

	num = trap_EntitiesInBox( curMins, curMaxs, ents, MAX_GENTITIES );
}
*/

/*
============
G_TouchTriggers

Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.
============
*/
void	G_TouchTriggers( gentity_t *ent ) {
	int			i, num;
	int			touch[MAX_GENTITIES];
	gentity_t	*hit;
	trace_t		trace;
	vec3_t		mins, maxs;
	static vec3_t	range = { 40, 40, 52 };

	if ( !ent->client ) {
		return;
	}

	// dead clients don't activate triggers!
	if ( ent->client->ps.stats[STAT_HEALTH] <= 0 ) {
		return;
	}

	VectorSubtract( ent->client->ps.origin, range, mins );
	VectorAdd( ent->client->ps.origin, range, maxs );

// STONELANCE
	AddPointToBounds( ent->client->car.tBody.r, mins, maxs );
// END

	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	// can't use ent->absmin, because that has a one unit pad
	VectorAdd( ent->client->ps.origin, ent->r.mins, mins );
	VectorAdd( ent->client->ps.origin, ent->r.maxs, maxs );

	for ( i=0 ; i<num ; i++ ) {
		hit = &g_entities[touch[i]];

		if ( !hit->touch && !ent->touch ) {
			continue;
		}
		if ( !( hit->r.contents & CONTENTS_TRIGGER ) ) {
			continue;
		}

		// ignore most entities if a spectator
// STONELANCE
//		if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
		if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR
			|| isRaceObserver( ent->s.number ) ) {
// END
			if ( hit->s.eType != ET_TELEPORT_TRIGGER &&
				// this is ugly but adding a new ET_? type will
				// most likely cause network incompatibilities
				hit->touch != Touch_DoorTrigger) {
				continue;
			}
		}

		// use separate code for determining if an item is picked up
		// so you don't have to actually contact its bounding box
		if ( hit->s.eType == ET_ITEM ) {
			if ( !BG_PlayerTouchesItem( &ent->client->ps, &hit->s, level.time ) ) {
				continue;
			}
		} else {
			if ( !trap_EntityContact( mins, maxs, hit ) ) {
				continue;
			}
		}

		memset( &trace, 0, sizeof(trace) );

		if ( hit->touch ) {
			hit->touch (hit, ent, &trace);
		}

		if ( ( ent->r.svFlags & SVF_BOT ) && ( ent->touch ) ) {
			ent->touch( ent, hit, &trace );
		}
	}

	// if we didn't touch a jump pad this pmove frame
	if ( ent->client->ps.jumppad_frame != ent->client->ps.pmove_framecount ) {
		ent->client->ps.jumppad_frame = 0;
		ent->client->ps.jumppad_ent = 0;
	}
}

/*
=================
SpectatorThink
=================
*/
void SpectatorThink( gentity_t *ent, usercmd_t *ucmd ) {
	pmove_t	pm;
	gclient_t	*client;

	client = ent->client;

// STONELANCE
//	if ( client->sess.spectatorState != SPECTATOR_FOLLOW ) {
	if ( client->sess.spectatorState == SPECTATOR_FREE ) {
// END
		if ( client->noclip ) {
			client->ps.pm_type = PM_NOCLIP;
		} else {
			client->ps.pm_type = PM_SPECTATOR;
		}

// STONELANCE
//		client->ps.speed = 400;	// faster than normal
		client->ps.speed = 800;	// faster than normal
// END

		// set up for pmove
		memset (&pm, 0, sizeof(pm));
		pm.ps = &client->ps;
		pm.cmd = *ucmd;
		pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;	// spectators can fly through bodies
		pm.trace = trap_Trace;
		pm.pointcontents = trap_PointContents;

		// perform a pmove
		Pmove (&pm);
		// save results of pmove
		VectorCopy( client->ps.origin, ent->s.origin );

		G_TouchTriggers( ent );
		trap_UnlinkEntity( ent );
	}

// STONELANCE
	if (client->sess.spectatorState == SPECTATOR_OBSERVE)
	{
		client->ps.commandTime = ucmd->serverTime;
		UpdateObserverSpot( ent, qfalse );
	}
// END

	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;

	// attack button cycles through spectators
// STONELANCE
//	if ( ( client->buttons & BUTTON_ATTACK ) && ! ( client->oldbuttons & BUTTON_ATTACK ) ) {
	if ( ( client->buttons & BUTTON_ATTACK ) && ! ( client->oldbuttons & BUTTON_ATTACK ) 
		&& !(ent->r.svFlags & SVF_BOT) ) {
// END
		Cmd_FollowCycle_f( ent, 1 );
	}

// STONELANCE
	if ( ( client->buttons & BUTTON_REARATTACK ) && ! ( client->oldbuttons & BUTTON_REARATTACK ) ) {
		int			newState;
		vec3_t		origin, angles;
		int			clientNum;
		
		if( client->sess.spectatorState == SPECTATOR_FREE )
			newState = SPECTATOR_OBSERVE;
		else
			newState = client->sess.spectatorState + 1;

		switch (newState){
		default:
		case SPECTATOR_FOLLOW:
			trap_SendServerCommand( ent - g_entities, "print \"Spectator Mode: Follow\n\"" );
			client->sess.spectatorState = SPECTATOR_FOLLOW;
			break;

		case SPECTATOR_OBSERVE:
			trap_SendServerCommand( ent - g_entities, "print \"Spectator Mode: Observe\n\"" );

			StopFollowing( ent );

			clientNum = ent->client->sess.spectatorClient;

			// team follow1 and team follow2 go to whatever clients are playing
			if ( clientNum == -1 ) {
				clientNum = level.follow1;
			} else if ( clientNum == -2 ) {
				clientNum = level.follow2;
			}

			if ( clientNum < 0 )
				Cmd_FollowCycle_f( ent, 1 );

			if ( clientNum < 0 )
				break;

			client->sess.spectatorState = SPECTATOR_OBSERVE;
//			ent->s.eType = ET_INVISIBLE;

			if ( FindBestObserverSpot(ent, &g_entities[client->sess.spectatorClient], origin, angles) )
			{
				G_SetOrigin(ent, origin);
				VectorCopy(origin, ent->client->ps.origin);
				VectorCopy(angles, ent->client->ps.viewangles);
			}
			else
				client->sess.spectatorState = SPECTATOR_FOLLOW;

			break;
		}
	}
// END
}



/*
=================
ClientInactivityTimer

Returns qfalse if the client is dropped
=================
*/
qboolean ClientInactivityTimer( gclient_t *client ) {
	if ( ! g_inactivity.integer ) {
		// give everyone some time, so if the operator sets g_inactivity during
		// gameplay, everyone isn't kicked
		client->inactivityTime = level.time + 60 * 1000;
		client->inactivityWarning = qfalse;
	} else if ( client->pers.cmd.forwardmove || 
		client->pers.cmd.rightmove || 
		client->pers.cmd.upmove ||
		(client->pers.cmd.buttons & BUTTON_ATTACK) ) {
		client->inactivityTime = level.time + g_inactivity.integer * 1000;
		client->inactivityWarning = qfalse;
	} else if ( !client->pers.localClient ) {
		if ( level.time > client->inactivityTime ) {
			trap_DropClient( client - level.clients, "Dropped due to inactivity" );
			return qfalse;
		}
		if ( level.time > client->inactivityTime - 10000 && !client->inactivityWarning ) {
			client->inactivityWarning = qtrue;
			trap_SendServerCommand( client - level.clients, "cp \"Ten seconds until inactivity drop!\n\"" );
		}
	}
	return qtrue;
}

/*
==================
ClientTimerActions

Actions that happen once a second
==================
*/
void ClientTimerActions( gentity_t *ent, int msec ) {
	gclient_t	*client;
#ifdef MISSIONPACK
	int			maxHealth;
#endif

	client = ent->client;
	client->timeResidual += msec;

	while ( client->timeResidual >= 1000 ) {
		client->timeResidual -= 1000;

		// regenerate
#ifdef MISSIONPACK
		if( bg_itemlist[client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_GUARD ) {
			maxHealth = client->ps.stats[STAT_MAX_HEALTH] / 2;
		}
		else if ( client->ps.powerups[PW_REGEN] ) {
			maxHealth = client->ps.stats[STAT_MAX_HEALTH];
		}
		else {
			maxHealth = 0;
		}
		if( maxHealth ) {
			if ( ent->health < maxHealth ) {
				ent->health += 15;
				if ( ent->health > maxHealth * 1.1 ) {
					ent->health = maxHealth * 1.1;
				}
				G_AddEvent( ent, EV_POWERUP_REGEN, 0 );
			} else if ( ent->health < maxHealth * 2) {
				ent->health += 5;
				if ( ent->health > maxHealth * 2 ) {
					ent->health = maxHealth * 2;
				}
				G_AddEvent( ent, EV_POWERUP_REGEN, 0 );
			}
#else
		if ( client->ps.powerups[PW_REGEN] ) {
			if ( ent->health < client->ps.stats[STAT_MAX_HEALTH]) {
				ent->health += 15;
				if ( ent->health > client->ps.stats[STAT_MAX_HEALTH] * 1.1 ) {
					ent->health = client->ps.stats[STAT_MAX_HEALTH] * 1.1;
				}
				G_AddEvent( ent, EV_POWERUP_REGEN, 0 );
			} else if ( ent->health < client->ps.stats[STAT_MAX_HEALTH] * 2) {
				ent->health += 5;
				if ( ent->health > client->ps.stats[STAT_MAX_HEALTH] * 2 ) {
					ent->health = client->ps.stats[STAT_MAX_HEALTH] * 2;
				}
				G_AddEvent( ent, EV_POWERUP_REGEN, 0 );
			}
#endif
		} else {
			// count down health when over max
// STONELANCE
//			if ( ent->health > client->ps.stats[STAT_MAX_HEALTH] ) {
			if (((!isRallyRace() && g_gametype.integer != GT_DERBY && g_gametype.integer != GT_LCS)
				|| level.startRaceTime) && ent->health > client->ps.stats[STAT_MAX_HEALTH]){
// END
				ent->health--;
			}
		}

		// count down armor when over max
// STONELANCE
//		if ( client->ps.stats[STAT_ARMOR] > client->ps.stats[STAT_MAX_HEALTH] ) {
		if (((!isRallyRace() && g_gametype.integer != GT_DERBY && g_gametype.integer != GT_LCS) || level.startRaceTime)
			&& client->ps.stats[STAT_ARMOR] > client->ps.stats[STAT_MAX_HEALTH]){
// END
			client->ps.stats[STAT_ARMOR]--;
		}
	}
#ifdef MISSIONPACK
	if( bg_itemlist[client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_AMMOREGEN ) {
		int w, max, inc, t, i;
    int weapList[]={WP_MACHINEGUN,WP_SHOTGUN,WP_GRENADE_LAUNCHER,WP_ROCKET_LAUNCHER,WP_LIGHTNING,WP_RAILGUN,WP_PLASMAGUN,WP_BFG,WP_NAILGUN,WP_PROX_LAUNCHER,WP_CHAINGUN,WP_FLAME_THROWER};
    int weapCount = ARRAY_LEN( weapList );
		//
    for (i = 0; i < weapCount; i++) {
		  w = weapList[i];

		  switch(w) {
			  case WP_MACHINEGUN: max = 50; inc = 4; t = 1000; break;
			  case WP_SHOTGUN: max = 10; inc = 1; t = 1500; break;
			  case WP_GRENADE_LAUNCHER: max = 10; inc = 1; t = 2000; break;
			  case WP_ROCKET_LAUNCHER: max = 10; inc = 1; t = 1750; break;
			  case WP_LIGHTNING: max = 50; inc = 5; t = 1500; break;
			  case WP_RAILGUN: max = 10; inc = 1; t = 1750; break;
			  case WP_PLASMAGUN: max = 50; inc = 5; t = 1500; break;
			  case WP_BFG: max = 10; inc = 1; t = 4000; break;
			  case WP_NAILGUN: max = 10; inc = 1; t = 1250; break;
			  case WP_PROX_LAUNCHER: max = 5; inc = 1; t = 2000; break;
			  case WP_CHAINGUN: max = 100; inc = 5; t = 1000; break;
			  case WP_FLAME_THROWER: max = 100; inc = 5; t = 1000; break;
			  default: max = 0; inc = 0; t = 1000; break;
		  }
		  client->ammoTimes[w] += msec;
		  if ( client->ps.ammo[w] >= max ) {
			  client->ammoTimes[w] = 0;
		  }
		  if ( client->ammoTimes[w] >= t ) {
			  while ( client->ammoTimes[w] >= t )
				  client->ammoTimes[w] -= t;
			  client->ps.ammo[w] += inc;
			  if ( client->ps.ammo[w] > max ) {
				  client->ps.ammo[w] = max;
			  }
		  }
    }
	}
#endif
}

/*
====================
ClientIntermissionThink
====================
*/
void ClientIntermissionThink( gclient_t *client ) {
	client->ps.eFlags &= ~EF_TALK;
	client->ps.eFlags &= ~EF_FIRING;

	// the level will exit when everyone wants to or after timeouts

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = client->pers.cmd.buttons;
	if ( client->buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) & ( client->oldbuttons ^ client->buttons ) ) {
		// this used to be an ^1 but once a player says ready, it should stick
		client->readyToExit = 1;
	}
}


/*
================
ClientEvents

Events will be passed on to the clients for presentation,
but any server game effects are handled here
================
*/
void ClientEvents( gentity_t *ent, int oldEventSequence ) {
	int		i, j;
	int		event;
	gclient_t *client;
	int		damage;
	vec3_t	origin, angles;
//	qboolean	fired;
	gitem_t *item;
	gentity_t *drop;

	client = ent->client;

	if ( oldEventSequence < client->ps.eventSequence - MAX_PS_EVENTS ) {
		oldEventSequence = client->ps.eventSequence - MAX_PS_EVENTS;
	}
	for ( i = oldEventSequence ; i < client->ps.eventSequence ; i++ ) {
		event = client->ps.events[ i & (MAX_PS_EVENTS-1) ];

		switch ( event ) {
		case EV_FALL_MEDIUM:
		case EV_FALL_FAR:
			if ( ent->s.eType != ET_PLAYER ) {
				break;		// not in the player model
			}
			if ( g_dmflags.integer & DF_NO_FALLING ) {
				break;
			}
			if ( event == EV_FALL_FAR ) {
				damage = 10;
			} else {
				damage = 5;
			}
			ent->pain_debounce_time = level.time + 200;	// no normal pain sound
			G_Damage (ent, NULL, NULL, NULL, NULL, damage, 0, MOD_FALLING);
			break;

		case EV_FIRE_WEAPON:
			FireWeapon( ent );
			break;
		case EV_ALTFIRE_WEAPON:
			FireAltWeapon( ent );
			break;
// STONELANCE
		case EV_FIRE_REARWEAPON:
			FireRearWeapon( ent );
			break;
// END

		case EV_USE_ITEM1:		// teleporter
			// drop flags in CTF
			item = NULL;
			j = 0;

			if ( ent->client->ps.powerups[ PW_REDFLAG ] ) {
				item = BG_FindItemForPowerup( PW_REDFLAG );
				j = PW_REDFLAG;
			} else if ( ent->client->ps.powerups[ PW_BLUEFLAG ] ) {
				item = BG_FindItemForPowerup( PW_BLUEFLAG );
				j = PW_BLUEFLAG;
			} else if ( ent->client->ps.powerups[ PW_NEUTRALFLAG ] ) {
				item = BG_FindItemForPowerup( PW_NEUTRALFLAG );
				j = PW_NEUTRALFLAG;
			}

			if ( item ) {
				drop = Drop_Item( ent, item, 0 );
				// decide how many seconds it has left
				drop->count = ( ent->client->ps.powerups[ j ] - level.time ) / 1000;
				if ( drop->count < 1 ) {
					drop->count = 1;
				}

				ent->client->ps.powerups[ j ] = 0;
			}

#ifdef MISSIONPACK
			if ( g_gametype.integer == GT_HARVESTER ) {
				if ( ent->client->ps.generic1 > 0 ) {
					if ( ent->client->sess.sessionTeam == TEAM_RED ) {
						item = BG_FindItem( "Blue Cube" );
					} else {
						item = BG_FindItem( "Red Cube" );
					}
					if ( item ) {
						for ( j = 0; j < ent->client->ps.generic1; j++ ) {
							drop = Drop_Item( ent, item, 0 );
							if ( ent->client->sess.sessionTeam == TEAM_RED ) {
								drop->spawnflags = TEAM_BLUE;
							} else {
								drop->spawnflags = TEAM_RED;
							}
						}
					}
					ent->client->ps.generic1 = 0;
				}
			}
#endif
			SelectSpawnPoint( ent->client->ps.origin, origin, angles, qfalse );
			TeleportPlayer( ent, origin, angles );
			break;

		case EV_USE_ITEM2:		// medkit
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH] + 25;

			break;

#ifdef MISSIONPACK
		case EV_USE_ITEM3:		// kamikaze
			// make sure the invulnerability is off
			ent->client->invulnerabilityTime = 0;
			// start the kamikze
			G_StartKamikaze( ent );
			break;

		case EV_USE_ITEM4:		// portal
			if( ent->client->portalID ) {
				DropPortalSource( ent );
			}
			else {
				DropPortalDestination( ent );
			}
			break;
		case EV_USE_ITEM5:		// invulnerability
			ent->client->invulnerabilityTime = level.time + 10000;
			break;
#endif

		default:
			break;
		}
	}

}

#ifdef MISSIONPACK
/*
==============
StuckInOtherClient
==============
*/
static int StuckInOtherClient(gentity_t *ent) {
	int i;
	gentity_t	*ent2;

	ent2 = &g_entities[0];
	for ( i = 0; i < MAX_CLIENTS; i++, ent2++ ) {
		if ( ent2 == ent ) {
			continue;
		}
		if ( !ent2->inuse ) {
			continue;
		}
		if ( !ent2->client ) {
			continue;
		}
		if ( ent2->health <= 0 ) {
			continue;
		}
		//
		if (ent2->r.absmin[0] > ent->r.absmax[0])
			continue;
		if (ent2->r.absmin[1] > ent->r.absmax[1])
			continue;
		if (ent2->r.absmin[2] > ent->r.absmax[2])
			continue;
		if (ent2->r.absmax[0] < ent->r.absmin[0])
			continue;
		if (ent2->r.absmax[1] < ent->r.absmin[1])
			continue;
		if (ent2->r.absmax[2] < ent->r.absmin[2])
			continue;
		return qtrue;
	}
	return qfalse;
}
#endif

void BotTestSolid(vec3_t origin);

/*
==============
SendPendingPredictableEvents
==============
*/
void SendPendingPredictableEvents( playerState_t *ps ) {
	gentity_t *t;
	int event, seq;
	int extEvent, number;

	// if there are still events pending
	if ( ps->entityEventSequence < ps->eventSequence ) {
		// create a temporary entity for this event which is sent to everyone
		// except the client who generated the event
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		// set external event to zero before calling BG_PlayerStateToEntityState
		extEvent = ps->externalEvent;
		ps->externalEvent = 0;
		// create temporary entity for event
		t = G_TempEntity( ps->origin, event );
		number = t->s.number;
		BG_PlayerStateToEntityState( ps, &t->s, qtrue );
		t->s.number = number;
		t->s.eType = ET_EVENTS + event;
		t->s.eFlags |= EF_PLAYER_EVENT;
		t->s.otherEntityNum = ps->clientNum;
		// send to everyone except the client who generated the event
		t->r.svFlags |= SVF_NOTSINGLECLIENT;
		t->r.singleClient = ps->clientNum;
		// set back external event
		ps->externalEvent = extEvent;
	}
}

/*
==============
ClientThink

This will be called once for each client frame, which will
usually be a couple times for each server frame on fast clients.

If "g_synchronousClients 1" is set, this will be called exactly
once for each server frame, which makes for smooth demo recording.
==============
*/
void ClientThink_real( gentity_t *ent ) {
	gclient_t	*client;
	pmove_t		pm;
	int			oldEventSequence;
	int			msec;
	usercmd_t	*ucmd;
// STONELANCE
	vec3_t		origin, forward;
	int			i;
	int			start;
	vec3_t		oldAngles;
	int			oldTime;
// END

	client = ent->client;

	// don't think if the client is not yet connected (and thus not yet spawned in)
	if (client->pers.connected != CON_CONNECTED) {
		return;
	}
	// mark the time, so the connection sprite can be removed
	ucmd = &ent->client->pers.cmd;

	// sanity check the command time to prevent speedup cheating
	if ( ucmd->serverTime > level.time + 200 ) {
		ucmd->serverTime = level.time + 200;
//		G_Printf("serverTime <<<<<\n" );
	}
	if ( ucmd->serverTime < level.time - 1000 ) {
		ucmd->serverTime = level.time - 1000;
//		G_Printf("serverTime >>>>>\n" );
	} 

	msec = ucmd->serverTime - client->ps.commandTime;
	// following others may result in bad times, but we still want
	// to check for follow toggles
	if ( msec < 1 && client->sess.spectatorState != SPECTATOR_FOLLOW ) {
		return;
	}
	if ( msec > 200 ) {
		msec = 200;
	}

	if ( pmove_msec.integer < 8 ) {
		trap_Cvar_Set("pmove_msec", "8");
		trap_Cvar_Update(&pmove_msec);
	}
	else if (pmove_msec.integer > 33) {
		trap_Cvar_Set("pmove_msec", "33");
		trap_Cvar_Update(&pmove_msec);
	}

	if ( pmove_fixed.integer || client->pers.pmoveFixed ) {
		ucmd->serverTime = ((ucmd->serverTime + pmove_msec.integer-1) / pmove_msec.integer) * pmove_msec.integer;
		//if (ucmd->serverTime - client->ps.commandTime <= 0)
		//	return;
	}

	//
	// check for exiting intermission
	//
	if ( level.intermissiontime ) {
		ClientIntermissionThink( client );
		return;
	}

	// spectators don't do much
	if ( client->sess.sessionTeam == TEAM_SPECTATOR ) {
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
			return;
		}
		SpectatorThink( ent, ucmd );
		return;
	}
// STONELANCE
	else if ( isRaceObserver( ent->s.number ) ){
		if ( client->sess.spectatorState == SPECTATOR_NOT ) {
			client->sess.spectatorState = SPECTATOR_OBSERVE;
			UpdateObserverSpot( ent, qtrue );
		}

		SpectatorThink( ent, ucmd );
		return;
	}
// END

	// check for inactivity timer, but never drop the local client of a non-dedicated server
	if ( !ClientInactivityTimer( client ) ) {
		return;
	}

	// clear the rewards if time
	if ( level.time > client->rewardTime ) {
		client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
	}

	if ( client->noclip ) {
		client->ps.pm_type = PM_NOCLIP;
	} else if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
		client->ps.pm_type = PM_DEAD;
	} else {
		client->ps.pm_type = PM_NORMAL;
	}

	client->ps.gravity = g_gravity.value;

	// set speed
	client->ps.speed = g_speed.value;

#ifdef MISSIONPACK
	if( bg_itemlist[client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_SCOUT ) {
		client->ps.speed *= 1.5;
	}
	else
#endif
// STONELANCE - haste changes fire rate now
/*
	if ( client->ps.powerups[PW_HASTE] ) {
		client->ps.speed *= 1.3;
	}
*/
// END

	// Let go of the hook if we aren't firing
// STONELANCE - no more hook
/*
	if ( client->ps.weapon == WP_GRAPPLING_HOOK &&
		client->hook && !( ucmd->buttons & BUTTON_ATTACK ) ) {
		Weapon_HookFree(client->hook);
	}
*/

// STONELANCE
	// check for car reset
	G_ResetCar( ent );

	if (!level.startRaceTime &&	(isRallyRace() || g_gametype.integer == GT_DERBY || g_gametype.integer == GT_LCS)){
		if ( ucmd->buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) && !ent->ready ) {
			trap_SendServerCommand( ent->s.clientNum, "cp \"Waiting for other players...\n\"");
			ent->ready = qtrue;
		}
		else if (!ent->ready && level.time < level.startTime + (g_forceEngineStart.integer * 1000) - 10000){
			if (ent->updateTime < level.time){
				trap_SendServerCommand( ent->s.clientNum, "cp \"Press FIRE or USE when ready to race.\n\"");
				ent->updateTime = level.time + 2000;
			}
		}

		ucmd->buttons = BUTTON_HANDBRAKE;
		ucmd->forwardmove = 0;
//		ucmd->rightmove = 0;
		ucmd->upmove = 0;
	}

	if ( ent->client->finishRaceTime &&	ent->client->finishRaceTime + 500 < level.time
		&& !level.intermissiontime ){

		ent->s.weapon = WP_NONE;
		ent->s.powerups = 0;
		ent->r.contents = CONTENTS_CORPSE;

		ucmd->weapon = ent->s.weapon;
		ucmd->buttons = BUTTON_HANDBRAKE;
		ucmd->forwardmove = 0;
//		ucmd->rightmove = 0;
		ucmd->upmove = 0;
	}

	if ( ent->client->finishRaceTime && ent->client->finishRaceTime + RACE_OBSERVER_DELAY < level.time ){
		gentity_t	*tent;
		tent = G_TempEntity( ent->client->ps.origin, EV_PLAYER_TELEPORT_OUT );
		tent->s.clientNum = ent->s.clientNum;

		SelectSpectatorSpawnPoint ( ent->client->ps.origin, ent->client->ps.viewangles );

		VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );
		VectorCopy( ent->client->ps.origin, ent->s.pos.trBase );

		VectorCopy( ent->client->ps.viewangles, ent->r.currentAngles );
		VectorCopy( ent->client->ps.viewangles, ent->s.apos.trBase );

		ent->raceObserver = qtrue;

		if( ent->frontBounds )
      trap_UnlinkEntity( ent->frontBounds );
    if( ent->rearBounds )
      trap_UnlinkEntity( ent->rearBounds ); 

		trap_UnlinkEntity( client->carPoints[0] );
		trap_UnlinkEntity( client->carPoints[1] );
		trap_UnlinkEntity( client->carPoints[2] );
		trap_UnlinkEntity( client->carPoints[3] );

		return;
	}
/*
	if( client->sess.sessionTeam != TEAM_SPECTATOR && ent->finishRaceTime &&
		ent->finishRaceTime + 10000 < level.time ){
		SetTeam( ent, "racerSpectator" );
	}
*/

	if (isRallyNonDMRace()/* TEMP DERBY || g_gametype.integer == GT_DERBY*/){
		ent->s.weapon = WP_NONE;
		ucmd->weapon = ent->s.weapon;
	}

// UPDATE - enable this
	// sound horn
	if ( client->sess.sessionTeam != TEAM_SPECTATOR && !level.intermissiontime &&
		ucmd->buttons & BUTTON_HORN && client->horn_sound_time < level.time){


		G_Sound(ent, CHAN_AUTO, G_SoundIndex("*horn.wav"));
		client->horn_sound_time = level.time + 100;
	}

	// use turbo
	if ((ucmd->buttons & BUTTON_TURBO) && !(client->oldbuttons & BUTTON_TURBO)
		&& client->ps.powerups[ PW_TURBO ] < 0){

		client->ps.powerups[ PW_TURBO ] = level.time + (-client->ps.powerups[ PW_TURBO ]);
	}
	else if (!(ucmd->buttons & BUTTON_TURBO) && (client->oldbuttons & BUTTON_TURBO)
		&& ent->client->ps.powerups[ PW_TURBO ] > level.time){

		client->ps.powerups[ PW_TURBO ] = -(client->ps.powerups[ PW_TURBO ] - level.time);
	}
// END

	// set up for pmove
	oldEventSequence = client->ps.eventSequence;

	memset (&pm, 0, sizeof(pm));

	// check for the hit-scan gauntlet, don't let the action
	// go through as an attack unless it actually hits something
	if ( client->ps.weapon == WP_GAUNTLET && !( ucmd->buttons & BUTTON_TALK ) &&
// STONELANCE
//		( ucmd->buttons & BUTTON_ATTACK ) && client->ps.weaponTime <= 0 ) {
		( ucmd->buttons & BUTTON_ATTACK ) && (client->ps.weaponTime & NORMAL_WEAPON_TIME_MASK) <= 0 ) {
// END
		pm.gauntletHit = CheckGauntletAttack( ent );
	}

// STONELANCE
/*
	if ( ent->flags & FL_FORCE_GESTURE ) {
		ent->flags &= ~FL_FORCE_GESTURE;
		ent->client->pers.cmd.buttons |= BUTTON_GESTURE;
	}
*/
// END

#ifdef MISSIONPACK
	// check for invulnerability expansion before doing the Pmove
	if (client->ps.powerups[PW_INVULNERABILITY] ) {
		if ( !(client->ps.pm_flags & PMF_INVULEXPAND) ) {
			vec3_t mins = { -42, -42, -42 };
			vec3_t maxs = { 42, 42, 42 };
			vec3_t oldmins, oldmaxs;

			VectorCopy (ent->r.mins, oldmins);
			VectorCopy (ent->r.maxs, oldmaxs);
			// expand
			VectorCopy (mins, ent->r.mins);
			VectorCopy (maxs, ent->r.maxs);
			trap_LinkEntity(ent);
			// check if this would get anyone stuck in this player
			if ( !StuckInOtherClient(ent) ) {
				// set flag so the expanded size will be set in PM_CheckDuck
				client->ps.pm_flags |= PMF_INVULEXPAND;
			}
			// set back
			VectorCopy (oldmins, ent->r.mins);
			VectorCopy (oldmaxs, ent->r.maxs);
			trap_LinkEntity(ent);
		}
	}
#endif

	pm.ps = &client->ps;
	pm.cmd = *ucmd;
// STONELANCE - dead cars can still hit things
/*
	if ( pm.ps->pm_type == PM_DEAD ) {
		pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
	}
	else
*/
// END
	if ( ent->r.svFlags & SVF_BOT ) {
		pm.tracemask = MASK_PLAYERSOLID | CONTENTS_BOTCLIP;
	}
	else {
		pm.tracemask = MASK_PLAYERSOLID;
	}

	pm.trace = trap_Trace;
	pm.pointcontents = trap_PointContents;
	pm.frictionFunc = G_FrictionCalc;
	pm.debugLevel = g_debugMove.integer;
	pm.noFootsteps = ( g_dmflags.integer & DF_NO_FOOTSTEPS ) > 0;

	pm.pmove_fixed = pmove_fixed.integer | client->pers.pmoveFixed;
	pm.pmove_msec = pmove_msec.integer;

// STONELANCE
	// TEMP used to move car around
/*
	client->car.sBody.r[2] += 3.0f * (pm.cmd.upmove * msec / 1000.0f);
	client->car.sBody.CoM[2] += 3.0f * (pm.cmd.upmove * msec / 1000.0f);
	for (i = 0; i < 8; i++){
		client->car.sPoints[i].r[2] += 3.0f * (pm.cmd.upmove * msec / 1000.0f);
	}
*/
	// END TEMP

	// load car position etc into pmove
	level.cars[ent->s.clientNum] = &client->car;

	pm.car = &client->car;
	pm.cars = level.cars;
	pm.pDebug = ent->pDebug;

	pm.controlMode = client->pers.controlMode;
	pm.manualShift = client->pers.manualShift;
	pm.client = qfalse;

	if (ent->pDebug > 0){
		Com_LogPrintf("Server Time: %i\n", pm.cmd.serverTime);
		Com_LogPrintf("Source: Debug %d\n", ent->pDebug);
		G_DebugForces(&pm.car->sBody, pm.car->sPoints, ent->pDebug-1);
		G_DebugDynamics(&pm.car->sBody, pm.car->sPoints, ent->pDebug-1);
	}

	start = trap_Milliseconds();

	oldTime = client->ps.commandTime;
	VectorCopy( client->ps.viewangles, oldAngles );

	pm.car_spring = car_spring.value;
	pm.car_shock_up = car_shock_up.value;
	pm.car_shock_down = car_shock_down.value;
	pm.car_swaybar = car_swaybar.value;
	pm.car_wheel = car_wheel.value;
	pm.car_wheel_damp = car_wheel_damp.value;

	pm.car_frontweight_dist = car_frontweight_dist.value;
	pm.car_IT_xScale = car_IT_xScale.value;
	pm.car_IT_yScale = car_IT_yScale.value;
	pm.car_IT_zScale = car_IT_zScale.value;
	pm.car_body_elasticity = car_body_elasticity.value;

	pm.car_air_cof = car_air_cof.value;
	pm.car_air_frac_to_df = car_air_frac_to_df.value;
	pm.car_friction_scale = car_friction_scale.value;
// END

	VectorCopy( client->ps.origin, client->oldOrigin );

#ifdef MISSIONPACK
		if (level.intermissionQueued != 0 && g_singlePlayer.integer) {
			if ( level.time - level.intermissionQueued >= 1000  ) {
				pm.cmd.buttons = 0;
				pm.cmd.forwardmove = 0;
				pm.cmd.rightmove = 0;
				pm.cmd.upmove = 0;
				if ( level.time - level.intermissionQueued >= 2000 && level.time - level.intermissionQueued <= 2500 ) {
					trap_SendConsoleCommand( EXEC_APPEND, "centerview\n");
				}
				ent->client->ps.pm_type = PM_SPINTERMISSION;
			}
		}
		Pmove (&pm);
#else
		Pmove (&pm);
#endif

// STONELANCE
	AnglesSubtract( client->ps.viewangles, oldAngles, ent->s.apos.trDelta );
	VectorScale( ent->s.apos.trDelta, 1000.0f / ( client->ps.commandTime - oldTime ), ent->s.apos.trDelta );

	if (level.time > 2000){
		client->frameNum++;
		client->pmoveTime = ((client->frameNum - 1) / (float)client->frameNum) * client->pmoveTime + (trap_Milliseconds() - start) / (float)client->frameNum;
	}

//	Com_Printf("Average pmoveTime %f, Instantanious pmoveTime %d, n=%d\n", client->pmoveTime, (trap_Milliseconds() - start), client->frameNum);

	AngleVectors(client->ps.viewangles, forward, NULL, NULL);

	if ( ent->frontBounds ){
		VectorMA( client->ps.origin, (CAR_LENGTH - CAR_WIDTH) / 2, forward, origin );
		G_SetOrigin( ent->frontBounds, origin );
		trap_LinkEntity ( ent->frontBounds );
	}

	if ( ent->rearBounds ){
		VectorMA( client->ps.origin, -(CAR_LENGTH - CAR_WIDTH) / 2, forward, origin );
		G_SetOrigin( ent->rearBounds, origin );
		trap_LinkEntity ( ent->rearBounds );
	}

	if (ent->pDebug > 0){
		Com_LogPrintf("Target\n");
		G_DebugDynamics(&pm.car->tBody, pm.car->tPoints, ent->pDebug-1);
	}

	for (i = 0; i < FIRST_FRAME_POINT; i++){
		if ( client->carPoints[i] ){
//			client->carPoints[i]->s.modelindex = G_ModelIndex( "models/test/sphere01.md3" );

//			G_SetOrigin(client->carPoints[i], client->car.sPoints[i].r);
			ent->s.pos.trType = TR_LINEAR;
			ent->s.pos.trTime = level.time;
			client->carPoints[i]->s.otherEntityNum = ent->s.clientNum;
			client->carPoints[i]->s.otherEntityNum2 = i;
			client->carPoints[i]->s.apos.trDelta[0] = client->car.sPoints[i].w;
			client->carPoints[i]->s.apos.trDelta[1] = client->car.wheelAngle;
			client->carPoints[i]->s.frame = client->car.sPoints[i].slipping;
			VectorCopy(client->car.sPoints[i].r, client->carPoints[i]->s.pos.trBase);
			VectorCopy(client->car.sPoints[i].r, ent->r.currentOrigin);
			VectorCopy(client->car.sPoints[i].v, client->carPoints[i]->s.pos.trDelta);
			VectorCopy(client->car.sPoints[i].normals[0], client->carPoints[i]->s.origin2);
			client->carPoints[i]->s.groundEntityNum = client->car.sPoints[i].onGround;

			// FIXME: remove this for bandwidth
//			client->carPoints[i]->r.svFlags |= SVF_BROADCAST;

			// send to owner only and ignore PVS ( SVF_BROADCAST )
			client->carPoints[i]->r.svFlags |= SVF_SINGLECLIENT | SVF_BROADCAST;
			client->carPoints[i]->r.singleClient = ent->s.number;

			trap_LinkEntity(client->carPoints[i]);
		}
		else
			Com_Printf("Warning: no carpoint entities\n");
	}
/*
	if (ent->pDebug == -1){
		Com_LogPrintf("server time %d\n", pm.ps->commandTime);

		Com_LogPrintf("wheelAngle %f\n", client->car.wheelAngle);
		Com_LogPrintf("throttle %f\n", client->car.throttle);
		Com_LogPrintf("gear %d\n", client->car.gear);
		Com_LogPrintf("rpm %f\n", client->car.rpm);
		
		// Body
		Com_LogPrintf("r %f, %f, %f\n", client->car.sBody.r[0], client->car.sBody.r[1], client->car.sBody.r[2]);
		Com_LogPrintf("v %f, %f, %f\n", client->car.sBody.v[0], client->car.sBody.v[1], client->car.sBody.v[2]);
		Com_LogPrintf("L %f, %f, %f\n", client->car.sBody.L[0], client->car.sBody.L[1], client->car.sBody.L[2]);
		Com_LogPrintf("viewangles: %f, %f, %f\n", pm.ps->viewangles[0], pm.ps->viewangles[1], pm.ps->viewangles[2]);

		// Point
		for (i = 0; i < 4; i++){
			Com_LogPrintf("wheel %d\n", i);
			Com_LogPrintf("r %f, %f, %f\n", client->car.sPoints[i].r[0], client->car.sPoints[i].r[1], client->car.sPoints[i].r[2]);
			Com_LogPrintf("v %f, %f, %f\n", client->car.sPoints[i].v[0], client->car.sPoints[i].v[1], client->car.sPoints[i].v[2]);
			Com_LogPrintf("w %f\n", client->car.sPoints[i].w);
			Com_LogPrintf("normals %f, %f, %f\n", client->car.sPoints[i].normals[0][0], client->car.sPoints[i].normals[0][1], client->car.sPoints[i].normals[0][2]);
			Com_LogPrintf("fluidDensity %f\n", client->car.sPoints[i].fluidDensity);
			Com_LogPrintf("onGround %d\n", client->car.sPoints[i].onGround);
			Com_LogPrintf("slipping %d\n", client->car.sPoints[i].slipping);
		}
		ent->pDebug = 0;
	}
*/

	/* Car
	Com_Printf("springStrength %f\n", client->car.springStrength);
	Com_Printf("springMaxLength %f\n", client->car.springMaxLength);
	Com_Printf("springMinLength %f\n", client->car.springMinLength);
	Com_Printf("shockStrength %f\n", client->car.shockStrength);
	Com_Printf("wheelAngle %f\n", client->car.wheelAngle);
	Com_Printf("throttle %f\n", client->car.throttle);
	Com_Printf("gear %d\n", client->car.gear);
	Com_Printf("rpm %f\n", client->car.rpm);
	Com_Printf("aCOF %f\n", client->car.aCOF);
	Com_Printf("sCOF %f\n", client->car.sCOF);
	Com_Printf("kCOF %f\n", client->car.kCOF);
	Com_Printf("dfCOF %f\n", client->car.dfCOF);
	Com_Printf("ewCOF %f\n", client->car.ewCOF);
	Com_Printf("inverseBodyInertiaTensor:\n");
	Com_Printf("%f, %f, %f\n", client->car.inverseBodyInertiaTensor[0][0], client->car.inverseBodyInertiaTensor[0][1], client->car.inverseBodyInertiaTensor[0][2]);
	Com_Printf("%f, %f, %f\n", client->car.inverseBodyInertiaTensor[1][0], client->car.inverseBodyInertiaTensor[1][1], client->car.inverseBodyInertiaTensor[1][2]);
	Com_Printf("%f, %f, %f\n", client->car.inverseBodyInertiaTensor[2][0], client->car.inverseBodyInertiaTensor[2][1], client->car.inverseBodyInertiaTensor[2][2]);
	*/
	
	/* Body
	Com_Printf("r %f, %f, %f\n", client->car.sBody.r[0], client->car.sBody.r[1], client->car.sBody.r[2]);
	Com_Printf("v %f, %f, %f\n", client->car.sBody.v[0], client->car.sBody.v[1], client->car.sBody.v[2]);
	Com_Printf("w %f, %f, %f\n", client->car.sBody.w[0], client->car.sBody.w[1], client->car.sBody.w[2]);
	Com_Printf("L %f, %f, %f\n", client->car.sBody.L[0], client->car.sBody.L[1], client->car.sBody.L[2]);
	Com_Printf("CoM %f, %f, %f\n", client->car.sBody.CoM[0], client->car.sBody.CoM[1], client->car.sBody.CoM[2]);
	Com_Printf("t:\n");
	Com_Printf("%f, %f, %f\n", client->car.sBody.t[0][0], client->car.sBody.t[0][1], client->car.sBody.t[0][2]);
	Com_Printf("%f, %f, %f\n", client->car.sBody.t[1][0], client->car.sBody.t[1][1], client->car.sBody.t[1][2]);
	Com_Printf("%f, %f, %f\n", client->car.sBody.t[2][0], client->car.sBody.t[2][1], client->car.sBody.t[2][2]);
	*/

	/* Point
	i = 4;
	Com_Printf("r %f, %f, %f\n", client->car.sPoints[i].r[0], client->car.sPoints[i].r[1], client->car.sPoints[i].r[2]);
	Com_Printf("v %f, %f, %f\n", client->car.sPoints[i].v[0], client->car.sPoints[i].v[1], client->car.sPoints[i].v[2]);
	Com_Printf("w %f\n", client->car.sPoints[i].w);
	Com_Printf("netForce %f, %f, %f\n", client->car.sPoints[i].netForce[0], client->car.sPoints[i].netForce[1], client->car.sPoints[i].netForce[2]);
	Com_Printf("netMoment %f\n", client->car.sPoints[i].netMoment);
	Com_Printf("normals %f, %f, %f\n", client->car.sPoints[i].normals[0][0], client->car.sPoints[i].normals[0][1], client->car.sPoints[i].normals[0][2]);
	Com_Printf("mass %f\n", client->car.sPoints[i].mass);
	Com_Printf("elasticity %f\n", client->car.sPoints[i].elasticity);
	Com_Printf("kcof %f\n", client->car.sPoints[i].kcof);
	Com_Printf("scof %f\n", client->car.sPoints[i].scof);
	Com_Printf("fluidDensity %f\n", client->car.sPoints[i].fluidDensity);
	Com_Printf("onGround %d\n", client->car.sPoints[i].onGround);
	Com_Printf("slipping %d\n", client->car.sPoints[i].slipping);
	*/
// END

	// save results of pmove
	if ( ent->client->ps.eventSequence != oldEventSequence ) {
		ent->eventTime = level.time;
	}
	if (g_smoothClients.integer) {
// STONELANCE
//		BG_PlayerStateToEntityStateExtraPolate( &ent->client->ps, &ent->s, ent->client->ps.commandTime, qtrue );
		BG_PlayerStateToEntityStateExtraPolate( &ent->client->ps, &ent->s, ent->client->ps.commandTime, trap_Cvar_VariableIntegerValue("sv_fps"), qtrue );
//		BG_PlayerStateToEntityStateExtraPolate( &ent->client->ps, &ent->s, ent->client->ps.commandTime, 2000, qtrue );
// END
	}
	else {
		BG_PlayerStateToEntityState( &ent->client->ps, &ent->s, qtrue );
	}
	SendPendingPredictableEvents( &ent->client->ps );

	if ( !( ent->client->ps.eFlags & EF_FIRING ) ) {
		client->fireHeld = qfalse;		// for grapple
	}

	// use the snapped origin for linking so it matches client predicted versions
	VectorCopy( ent->s.pos.trBase, ent->r.currentOrigin );

	VectorCopy (pm.mins, ent->r.mins);
	VectorCopy (pm.maxs, ent->r.maxs);

	ent->waterlevel = pm.waterlevel;
	ent->watertype = pm.watertype;

	// execute client events
	ClientEvents( ent, oldEventSequence );

// STONELANCE - do damage from pmove
	if (pm.damage.damage)
	{
		if( !(pm.damage.dflags & DAMAGE_NO_PROTECTION) )
			pm.damage.damage *= g_damageScale.value;

		if( pm.damage.damage > 0 )
		{
			if (pm.damage.otherEnt >= 0){
				G_Damage(ent, NULL, &g_entities[pm.damage.otherEnt], pm.damage.dir, pm.damage.origin, pm.damage.damage, pm.damage.dflags, pm.damage.mod);
				VectorInverse(pm.damage.dir);
				G_Damage(&g_entities[pm.damage.otherEnt], NULL, ent, pm.damage.dir, pm.damage.origin, pm.damage.damage, pm.damage.dflags, pm.damage.mod);
			}
			else
				G_Damage(ent, NULL, NULL, pm.damage.dir, pm.damage.origin, pm.damage.damage, pm.damage.dflags, pm.damage.mod);
		}
	}
// END

	// link entity now, after any personal teleporters have been used
	trap_LinkEntity (ent);
	if ( !ent->client->noclip ) {
		G_TouchTriggers( ent );
	}

	// NOTE: now copy the exact origin over otherwise clients can be snapped into solid
	VectorCopy( ent->client->ps.origin, ent->r.currentOrigin );

	//test for solid areas in the AAS file
	BotTestAAS(ent->r.currentOrigin);

	// touch other objects
	ClientImpacts( ent, &pm );

	// save results of triggers and client events
	if (ent->client->ps.eventSequence != oldEventSequence) {
		ent->eventTime = level.time;
	}

	// swap and latch button actions
	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;

	// check for respawning
	if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
		// wait for the attack button to be pressed
		if ( level.time > client->respawnTime ) {
			// forcerespawn is to prevent users from waiting out powerups
			if ( g_forcerespawn.integer > 0 && 
				( level.time - client->respawnTime ) > g_forcerespawn.integer * 1000 ) {
// STONELANCE
				if ((g_gametype.integer != GT_DERBY && g_gametype.integer != GT_LCS) || !ent->client->finishRaceTime)
// END
				ClientRespawn( ent );
				return;
			}
		
			// pressing attack or use is the normal respawn method
			if ( ucmd->buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) ) {
// STONELANCE
				if ((g_gametype.integer != GT_DERBY && g_gametype.integer != GT_LCS) || !ent->client->finishRaceTime)
// END
				ClientRespawn( ent );
			}
		}
		return;
	}

	// perform once-a-second actions
	ClientTimerActions( ent, msec );

// STONELANCE - UPDATE: enable this (use flags instead?)
/*
	if ((client->ps.stats[STAT_HEALTH] < 25 && ent->prevHealth >= 25) ||
		(client->ps.stats[STAT_HEALTH] < 50 && ent->prevHealth >= 50) ||
		(client->ps.stats[STAT_HEALTH] >= 50 && ent->prevHealth < 50)){
		BG_AddPredictableEventToPlayerstate( EV_ENGINE_SMOKE, client->ps.stats[STAT_HEALTH], &ent->client->ps );
	}
	ent->prevHealth = client->ps.stats[STAT_HEALTH];
*/
// END
}

/*
==================
ClientThink

A new command has arrived from the client
==================
*/
void ClientThink( int clientNum ) {
	gentity_t *ent;

	ent = g_entities + clientNum;
	trap_GetUsercmd( clientNum, &ent->client->pers.cmd );

	// mark the time we got info, so we can display the
	// phone jack if they don't get any for a while
	ent->client->lastCmdTime = level.time;

	if ( !(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer ) {
		ClientThink_real( ent );
	}
}


void G_RunClient( gentity_t *ent ) {
	if ( !(ent->r.svFlags & SVF_BOT) && !g_synchronousClients.integer ) {
		return;
	}
	ent->client->pers.cmd.serverTime = level.time;
	ClientThink_real( ent );
}


/*
==================
SpectatorClientEndFrame

==================
*/
void SpectatorClientEndFrame( gentity_t *ent ) {
	gclient_t	*cl;
// STONELANCE
	int			i;
	int			savedDmgDealt;
	int			savedDmgTaken;

//	Com_Printf( "Spectator Mode: %i\n", ent->client->sess.spectatorState );
// END

	// if we are doing a chase cam or a remote view, grab the latest info
	if ( ent->client->sess.spectatorState == SPECTATOR_FOLLOW ) {
		int		clientNum, flags;

		clientNum = ent->client->sess.spectatorClient;

		// team follow1 and team follow2 go to whatever clients are playing
		if ( clientNum == -1 ) {
			clientNum = level.follow1;
		} else if ( clientNum == -2 ) {
			clientNum = level.follow2;
		}
		if ( clientNum >= 0 ) {
			cl = &level.clients[ clientNum ];
// STONELANCE
//			if ( cl->pers.connected == CON_CONNECTED && cl->sess.sessionTeam != TEAM_SPECTATOR ) {
			if ( cl->pers.connected == CON_CONNECTED && cl->sess.sessionTeam != TEAM_SPECTATOR
				&& !isRaceObserver( clientNum )) {
// END
				flags = (cl->ps.eFlags & ~(EF_VOTED | EF_TEAMVOTED)) | (ent->client->ps.eFlags & (EF_VOTED | EF_TEAMVOTED));
// STONELANCE - FIXME: dont overwrite stats if the player was in the race or derby
//				ent->client->ps = cl->ps;

				savedDmgDealt = ent->client->ps.stats[STAT_DAMAGE_DEALT];
				savedDmgTaken = ent->client->ps.stats[STAT_DAMAGE_TAKEN];
				memcpy( &ent->client->ps, &cl->ps, sizeof(playerState_t) );
				ent->client->ps.stats[STAT_DAMAGE_DEALT] = savedDmgDealt;
				ent->client->ps.stats[STAT_DAMAGE_TAKEN] = savedDmgTaken;
/*
				VectorCopy( cl->ps.origin, ent->client->ps.origin );
				VectorCopy( cl->ps.velocity, ent->client->ps.velocity );
				VectorCopy( cl->ps.angularMomentum, ent->client->ps.angularMomentum );
				VectorCopy( cl->ps.viewangles, ent->client->ps.viewangles );
				
				ent->client->ps.commandTime = cl->ps.commandTime;
				ent->client->ps.clientNum = cl->ps.clientNum;
				ent->client->ps.pm_flags = cl->ps.pm_flags;
				ent->client->ps.pm_type = cl->ps.pm_type;
				ent->client->ps.damagePitch = cl->ps.damagePitch;
				ent->client->ps.damageYaw = cl->ps.damageYaw;
				ent->client->ps.delta_angles[0] = cl->ps.delta_angles[0];
				ent->client->ps.delta_angles[1] = cl->ps.delta_angles[1];
				ent->client->ps.delta_angles[2] = cl->ps.delta_angles[2];

				ent->client->ps.legsAnim = cl->ps.legsAnim;
				ent->client->ps.legsTimer = cl->ps.legsTimer;
				ent->client->ps.torsoAnim = cl->ps.torsoAnim;
				ent->client->ps.torsoTimer = cl->ps.torsoTimer;
*/
// END
				ent->client->ps.pm_flags |= PMF_FOLLOW;
				ent->client->ps.eFlags = flags;

// STONELANCE
				// FIXME: could this fuck some shit up with entity numbers or anything like that?
				// TODO: optimize this so it only copies the fields that change and not everything
				for( i = 0; i < 4; i++ )
				{
					memcpy( ent->client->carPoints[i], cl->carPoints[i], sizeof(gentity_t) );
					ent->client->carPoints[i]->r.singleClient = ent->s.number;
				}
// END

				return;
			}
		}

		if ( ent->client->ps.pm_flags & PMF_FOLLOW ) {
			// drop them to free spectators unless they are dedicated camera followers
			if ( ent->client->sess.spectatorClient >= 0 ) {
				ent->client->sess.spectatorState = SPECTATOR_FREE;
			}

			ClientBegin( ent->client - level.clients );
		}
	}
// STONELANCE
	else if ( ent->client->sess.spectatorState == SPECTATOR_OBSERVE )
	{
		int		clientNum;
		vec3_t	angles, delta;
		float	dist;

		clientNum = ent->client->sess.spectatorClient;
		if ( clientNum == -1 ) {
			clientNum = level.follow1;
		} else if ( clientNum == -2 ) {
			clientNum = level.follow2;
		}

		ent->client->ps.persistant[PERS_ATTACKER] = -1;

		if ( clientNum >= 0 ) {
//			ent->client->ps.clientNum = clientNum;
			cl = &level.clients[ clientNum ];

			if ( cl->pers.connected == CON_CONNECTED && cl->sess.sessionTeam != TEAM_SPECTATOR
				&& !isRaceObserver( clientNum ) ) {
				VectorClear(delta);

				if ( !(ent->spotflags & OBSERVERCAM_FIXED) ){
					VectorSubtract(cl->ps.origin, ent->client->ps.origin, delta);
					vectoangles(delta, angles);
					VectorCopy(angles, ent->client->ps.viewangles);

					ent->client->ps.persistant[PERS_ATTACKER] = clientNum;
				}

				VectorClear(ent->client->ps.velocity);

				if ( ent->spotflags & OBSERVERCAM_ZOOM ){
					dist = VectorLength(delta);

					// zoom in closer to target if too far away
					if (dist > 300)
					{
						dist -= 300;
						VectorSet(ent->client->ps.velocity, dist, 0, 0);
					}
				}

				VectorCopy( ent->client->ps.viewangles, ent->client->ps.damageAngles );
				ent->client->ps.damagePitch = ANGLE2BYTE(ent->client->ps.damageAngles[PITCH]);
				ent->client->ps.damageYaw = ANGLE2BYTE(ent->client->ps.damageAngles[YAW]);

				ent->client->ps.pm_flags |= PMF_OBSERVE;
				return;
			}
		}
//		else
//			G_DebugLogPrintf( "Observer Cam: Target client %i\n", clientNum );

		if ( ent->client->ps.pm_flags & PMF_OBSERVE ) {
			// drop them to free spectators unless they are dedicated camera followers
			if ( ent->client->sess.spectatorClient >= 0 ) {
// STONELANCE
				// try cycling to a new client
				clientNum = ent->client->sess.spectatorClient;
				Cmd_FollowCycle_f( ent, 1 );
				if ( clientNum != ent->client->sess.spectatorClient ) {
//					Com_Printf( "Observer Cam: cycle to next player\n" );
//					G_DebugLogPrintf( "Observer Cam: cycle to next player\n" );
					return;
				}
//				Com_Printf( "Observer Cam: drop back to free\n" );
//				G_DebugLogPrintf( "Observer Cam: drop back to free\n" );
// END

				ent->client->sess.spectatorState = SPECTATOR_FREE;
			}

			ClientBegin( ent->client - level.clients );
		}
	}
// END

	if ( ent->client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
		ent->client->ps.pm_flags |= PMF_SCOREBOARD;
	} else {
		ent->client->ps.pm_flags &= ~PMF_SCOREBOARD;
	}
}

/*
==============
ClientEndFrame

Called at the end of each server frame for each connected client
A fast client will have multiple ClientThink for each ClientEdFrame,
while a slow client may have multiple ClientEndFrame between ClientThink.
==============
*/
void ClientEndFrame( gentity_t *ent ) {
	int			i;

// STONELANCE
//	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR ) {
	if ( ent->client->sess.sessionTeam == TEAM_SPECTATOR || isRaceObserver( ent->s.number ) ){
// END
		SpectatorClientEndFrame( ent );
		return;
	}

	// turn off any expired powerups
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
// STONELANCE
		if ( i == PW_TURBO && ent->client->ps.powerups[ i ] < level.time && 
			ent->client->ps.powerups[ i ] > 0) {

			ent->client->ps.powerups[ i ] = 0;
		}
		else if ( i != PW_TURBO && ent->client->ps.powerups[ i ] < level.time ) {
//		if ( ent->client->ps.powerups[ i ] < level.time ) {
// END
			ent->client->ps.powerups[ i ] = 0;
		}
	}

#ifdef MISSIONPACK
	// set powerup for player animation
	if( bg_itemlist[ent->client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_GUARD ) {
		ent->client->ps.powerups[PW_GUARD] = level.time;
	}
	if( bg_itemlist[ent->client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_SCOUT ) {
		ent->client->ps.powerups[PW_SCOUT] = level.time;
	}
	if( bg_itemlist[ent->client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_DOUBLER ) {
		ent->client->ps.powerups[PW_DOUBLER] = level.time;
	}
	if( bg_itemlist[ent->client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_AMMOREGEN ) {
		ent->client->ps.powerups[PW_AMMOREGEN] = level.time;
	}
	if ( ent->client->invulnerabilityTime > level.time ) {
		ent->client->ps.powerups[PW_INVULNERABILITY] = level.time;
	}
#endif

	// save network bandwidth
#if 0
	if ( !g_synchronousClients->integer && ent->client->ps.pm_type == PM_NORMAL ) {
		// FIXME: this must change eventually for non-sync demo recording
		VectorClear( ent->client->ps.viewangles );
	}
#endif

	//
	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	//
	if ( level.intermissiontime ) {
		return;
	}

	// burn from lava, etc
	P_WorldEffects (ent);

	// apply all the damage taken this frame
	P_DamageFeedback (ent);

	// add the EF_CONNECTION flag if we haven't gotten commands recently
	if ( level.time - ent->client->lastCmdTime > 1000 ) {
		ent->client->ps.eFlags |= EF_CONNECTION;
	} else {
		ent->client->ps.eFlags &= ~EF_CONNECTION;
	}

	ent->client->ps.stats[STAT_HEALTH] = ent->health;	// FIXME: get rid of ent->health...

	G_SetClientSound (ent);

	// set the latest infor
	if (g_smoothClients.integer) {
// STONELANCE
//		BG_PlayerStateToEntityStateExtraPolate( &ent->client->ps, &ent->s, ent->client->ps.commandTime, qtrue );
		BG_PlayerStateToEntityStateExtraPolate( &ent->client->ps, &ent->s, ent->client->ps.commandTime, trap_Cvar_VariableIntegerValue("sv_fps"), qtrue );
//		BG_PlayerStateToEntityStateExtraPolate( &ent->client->ps, &ent->s, ent->client->ps.commandTime, 2000, qtrue );

//		Com_Printf( "Extrapolating playerstate, trType %i\n", ent->s.pos.trType );
// END
	}
	else {
		BG_PlayerStateToEntityState( &ent->client->ps, &ent->s, qtrue );
	}
	SendPendingPredictableEvents( &ent->client->ps );

/*
	if( g_entities[0].client )
	{
		int		rearTime;

		g_entities[0].client->ps.weaponTime |= 0x7fff;

		rearTime = ( g_entities[0].client->ps.weaponTime & REAR_WEAPON_TIME_MASK ) >> 16;
		Com_Printf( "g forward: cur weapon time %i\n", g_entities[0].client->ps.weaponTime & NORMAL_WEAPON_TIME_MASK );
		Com_Printf( "g rear: cur weapon time %i\n", rearTime );
		Com_Printf( "g: cur weapon time %i\n", g_entities[0].client->ps.weaponTime );
	}
*/

	// set the bit for the reachability area the client is currently in
//	i = trap_AAS_PointReachabilityAreaIndex( ent->client->ps.origin );
//	ent->client->areabits[i >> 3] |= 1 << (i & 7);
}


