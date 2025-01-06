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

#include "g_local.h"

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
	vec3_t	angles;

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
	if ( client->damage_fromWorld ) {
		client->ps.damagePitch = 255;
		client->ps.damageYaw = 255;

		client->damage_fromWorld = qfalse;
	} else {
		vectoangles( client->damage_from, angles );
		client->ps.damagePitch = angles[PITCH]/360.0 * 256;
		client->ps.damageYaw = angles[YAW]/360.0 * 256;
	}

	// play an apropriate pain sound
	if(!player->client->vehiclenum){ //VEHICLE-SYSTEM: disable pain sound for all
	if ( (level.time > player->pain_debounce_time) && !(player->flags & FL_GODMODE) ) {
		player->pain_debounce_time = level.time + 700;
		G_AddEvent( player, EV_PAIN, player->health );
		client->ps.damageEvent++;
	}
	} else {
		G_AddEvent( player, EV_PAINVEHICLE, player->health );
	}


	client->ps.damageCount = count;

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
	if (g_drowndamage.integer == 1)
	if ( waterlevel == 3 ) {
		// envirosuit give air
		if ( envirosuit ) {
			ent->client->airOutTime = level.time + 10000;
		}

		// if out of air, start drowning
		if ( ent->client->airOutTime < level.time) {
			// drown!
			ent->client->airOutTime += 1000;
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
		ent->client->airOutTime = level.time + 12000;
		ent->damage = 2;
	}

	//
	// check for sizzle damage (move to pmove?)
	//
	if (waterlevel &&
		(ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME|CONTENTS_WATER)) ) {
		if (ent->health > 0
			&& ent->pain_debounce_time <= level.time	) {
			if ( envirosuit ) {
				G_AddEvent( ent, EV_POWERUP_BATTLESUIT, 0 );
			} else {
				if (ent->watertype & CONTENTS_LAVA) {
					if(g_lavadamage.integer > 0){
					G_Damage (ent, NULL, NULL, NULL, NULL,
						g_lavadamage.integer, 0, MOD_LAVA);
					}
				}

				if (ent->watertype & CONTENTS_SLIME) {
					if(g_slimedamage.integer > 0){
					G_Damage (ent, NULL, NULL, NULL, NULL,
						g_slimedamage.integer, 0, MOD_SLIME);
					}
				}

				if (ent->watertype & CONTENTS_WATER) {
					if(g_waterdamage.integer > 0){
					G_Damage (ent, NULL, NULL, NULL, NULL,
						g_waterdamage.integer, 0, MOD_WATER);
					}
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
	if( ent->s.eFlags & EF_TICKING ) {
		ent->client->ps.loopSound = G_SoundIndex( "sound/weapons/proxmine/wstbtick.wav");
	}
	else
	if (ent->waterlevel && (ent->watertype&(CONTENTS_LAVA|CONTENTS_SLIME)) ) {
		ent->client->ps.loopSound = level.snd_fry;
	} else {
		if(ent->singlebot){
		ent->client->ps.loopSound = G_SoundIndex(va("bots/%s", ent->target));
		} else {
		ent->client->ps.loopSound = 0;
		}
	}
}

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
============
G_TouchTriggers

Find all trigger entities that ent's current position touches.
Spectators will only interact with teleporters.
============
*/
void	G_TouchTriggers( gentity_t *ent ) {
	int			i, num;
	gentity_t	*hit;
	trace_t		trace;
	vec3_t		mins, maxs;
	static vec3_t	range = { 40, 40, 52 };

	if ( !ent->client || ent->client->ps.pm_type == PM_CUTSCENE ) {
		return;
	}

	//ELIMINATION LMS
	// dead clients don't activate triggers! The reason our pm_spectators can't do anything
	if ( ent->client->ps.stats[STAT_HEALTH] <= 0 && ent->client->ps.pm_type != PM_SPECTATOR) {
		return;
	}

	VectorSubtract( ent->client->ps.origin, range, mins );
	VectorAdd( ent->client->ps.origin, range, maxs );

	num = trap_EntitiesInBox( mins, maxs, SourceTechEntityList, MAX_GENTITIES );

	// can't use ent->absmin, because that has a one unit pad
	VectorAdd( ent->client->ps.origin, ent->r.mins, mins );
	VectorAdd( ent->client->ps.origin, ent->r.maxs, maxs );

	for ( i=0 ; i<num ; i++ ) {
		hit = &g_entities[SourceTechEntityList[i]];

		if ( !hit->touch && !ent->touch ) {
			continue;
		}
		if ( !( hit->r.contents & CONTENTS_TRIGGER ) ) {
			continue;
		}

		// ignore most entities if a spectator
		if ( (ent->client->sess.sessionTeam == TEAM_SPECTATOR) || ent->client->ps.pm_type == PM_SPECTATOR ) {
			if ( hit->s.eType != ET_TELEPORT_TRIGGER &&
				// this is ugly but adding a new ET_? type will
				// most likely cause network incompatibilities
				//We need to stop eliminated players from opening doors somewhere else /Sago007 20070814
				hit->touch != Touch_DoorTrigger ) {
				continue;
			}
		}

		// use seperate code for determining if an item is picked up
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

void G_KillVoid( gentity_t *ent ) {
	vec3_t		orig;

	if ( !ent->client || ent->client->ps.pm_type == PM_CUTSCENE ) {
		return;
	}

	// dead clients don't activate triggers! The reason our pm_spectators can't do anything
	if ( ent->client->ps.stats[STAT_HEALTH] <= 0 && ent->client->ps.pm_type != PM_SPECTATOR) {
		return;
	}

	VectorCopy( ent->client->ps.origin, orig );
	if(orig[2] <= -520000){
	G_Damage (ent, NULL, NULL, NULL, NULL, 1000, 0, MOD_UNKNOWN);	
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

        if ( ( g_gametype.integer == GT_ELIMINATION || g_gametype.integer == GT_CTF_ELIMINATION) &&
                client->sess.spectatorState != SPECTATOR_FOLLOW &&
                g_elimination_lockspectator.integer>1 &&
                ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
            Cmd_FollowCycle_f(ent);
        }

	if ( client->sess.spectatorState != SPECTATOR_FOLLOW ) {
		client->ps.pm_type = PM_SPECTATOR;
		client->ps.speed = g_spectatorspeed.integer;	// faster than normal

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

	/* Stopped players from going into follow mode in B5, should be fixed in B9
	if(ent->client->sess.sessionTeam != TEAM_SPECTATOR && g_gametype.integer>=GT_ELIMINATION && g_gametype.integer<=GT_LMS)
		return;
	*/

	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;

    //KK-OAX Changed to keep followcycle functional
	// attack button cycles through spectators
	if ( ( client->buttons & BUTTON_ATTACK ) && ! ( client->oldbuttons & BUTTON_ATTACK ) ) {
		Cmd_FollowCycle_f( ent );
	}

	if ( ( client->buttons & BUTTON_USE_HOLDABLE ) && ! ( client->oldbuttons & BUTTON_USE_HOLDABLE ) ) {
		if ( ( g_gametype.integer == GT_ELIMINATION || g_gametype.integer == GT_CTF_ELIMINATION) &&
                g_elimination_lockspectator.integer>1 &&
                ent->client->sess.sessionTeam != TEAM_SPECTATOR ) {
                    return;
                }
		StopFollowing(ent);
	}
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

void MakeUnlimitedAmmo(gentity_t *ent) {
	Set_Ammo(ent, WP_MACHINEGUN, 9999);
	Set_Ammo(ent, WP_SHOTGUN, 9999);
	Set_Ammo(ent, WP_GRENADE_LAUNCHER, 9999);
	Set_Ammo(ent, WP_ROCKET_LAUNCHER, 9999);	
	Set_Ammo(ent, WP_LIGHTNING, 9999);
	Set_Ammo(ent, WP_RAILGUN, 9999);
	Set_Ammo(ent, WP_PLASMAGUN, 9999);
	Set_Ammo(ent, WP_BFG, 9999);
	Set_Ammo(ent, WP_NAILGUN, 9999);
	Set_Ammo(ent, WP_PROX_LAUNCHER, 9999);
	Set_Ammo(ent, WP_CHAINGUN, 9999);
}
/*
==================
ClientTimerActions

Actions that happen once a second
==================
*/
void ClientTimerActions( gentity_t *ent, int msec ) {
	gclient_t	*client;
	int			maxHealth;
	int			mins, seconds, tens;

	client = ent->client;
	client->timeResidual += msec;

	// Dropped Ammo No-Pickup
	if(ent->wait_to_pickup <= 60000000){
	if(level.time < ent->wait_to_pickup){
		client->ps.stats[STAT_NO_PICKUP] = 1;
	} else {
		client->ps.stats[STAT_NO_PICKUP] = 0;
	}
	}

	while ( client->timeResidual >= 1000 ) {
		client->timeResidual -= 1000;


	ent->s.eFlags &= ~EF_HEARED;		//hear update

	// Infinite Ammo
	if(g_rlinf.integer==1){ Set_Ammo(ent, WP_ROCKET_LAUNCHER, 9999); }
	if(g_glinf.integer==1){ Set_Ammo(ent, WP_GRENADE_LAUNCHER, 9999); }
	if(g_pginf.integer==1){ Set_Ammo(ent, WP_PLASMAGUN, 9999); }
	if(g_mginf.integer==1){ Set_Ammo(ent, WP_MACHINEGUN, 9999); }
	if(g_sginf.integer==1){ Set_Ammo(ent, WP_SHOTGUN, 9999); }
	if(g_bfginf.integer==1){ Set_Ammo(ent, WP_BFG, 9999); }
	if(g_rginf.integer==1){ Set_Ammo(ent, WP_RAILGUN, 9999); }
	if(g_cginf.integer==1){ Set_Ammo(ent, WP_CHAINGUN, 9999); }
	if(g_lginf.integer==1){ Set_Ammo(ent, WP_LIGHTNING, 9999); }
	if(g_nginf.integer==1){ Set_Ammo(ent, WP_NAILGUN, 9999); }
	if(g_plinf.integer==1){ Set_Ammo(ent, WP_PROX_LAUNCHER, 9999); }
	if(g_ftinf.integer==1){ Set_Ammo(ent, WP_FLAMETHROWER, 9999); }
	if(g_aminf.integer==1){ Set_Ammo(ent, WP_ANTIMATTER, 9999); }

	// guard inf ammo
	if( bg_itemlist[client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_GUARD ) {
	if (g_guard_infammo.integer == 1){
		MakeUnlimitedAmmo(ent);
	}
	}

	// scout inf ammo
	if( bg_itemlist[client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_SCOUT ) {
	if (g_scout_infammo.integer == 1){
		MakeUnlimitedAmmo(ent);
	}
	}

	// doubler inf ammo
	if( bg_itemlist[client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_DOUBLER ) {
	if (g_doubler_infammo.integer == 1){
		MakeUnlimitedAmmo(ent);
	}
	}

	//team red infammo
	if(client->sess.sessionTeam == TEAM_RED){
	if (g_teamred_infammo.integer == 1){
		MakeUnlimitedAmmo(ent);
	}
	}

	//team blue infammo
	if(client->sess.sessionTeam == TEAM_BLUE){
	if (g_teamblue_infammo.integer == 1){
		MakeUnlimitedAmmo(ent);
	}
	}

	if(ent->botskill == 6){
		MakeUnlimitedAmmo(ent);
	}

		//Stop in elimination!!!
		if (client->ps.pm_flags & PMF_ELIMWARMUP)
			continue;

		// regenerate
		if( bg_itemlist[client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_GUARD ) {
			maxHealth = client->ps.stats[STAT_MAX_HEALTH];
		}
		else if ( client->ps.powerups[PW_REGEN] ) {
			maxHealth = client->ps.stats[STAT_MAX_HEALTH];
		}
		else {
			maxHealth = 0;
		}
		if( maxHealth ) {
			if ( ent->health < maxHealth ) {
				ent->health += g_fasthealthregen.integer;
				if ( ent->health > maxHealth * 1.1 ) {
					ent->health = maxHealth * 1.1;
				}
				G_AddEvent( ent, EV_POWERUP_REGEN, 0 );
			} else if ( ent->health < maxHealth * 2) {
				ent->health += g_slowhealthregen.integer;
				if ( ent->health > maxHealth * 2 ) {
					ent->health = maxHealth * 2;
				}
				G_AddEvent( ent, EV_POWERUP_REGEN, 0 );
			}
		} else {
			// count down health when over max
			if ( !ent->singlebot ){
			if ( ent->health > client->ps.stats[STAT_MAX_HEALTH] ) {
				ent->health--;
			}
			}
			
			client->ps.stats[STAT_MONEY] = client->pers.oldmoney;
			
			
			//Start killing players in LMS, if we are in overtime
			if(g_elimination_roundtime.integer&&g_gametype.integer==GT_LMS && TeamHealthCount( -1, TEAM_FREE ) != ent->health &&(level.roundNumber==level.roundNumberStarted)&&(level.time>=level.roundStartTime+1000*g_elimination_roundtime.integer)) {
				ent->damage=5;
				G_Damage (ent, NULL, NULL, NULL, NULL,
					ent->damage, DAMAGE_NO_ARMOR, MOD_UNKNOWN);
			}
			else
			if ( ent->health < client->ps.stats[STAT_MAX_HEALTH] ) {
				ent->health+=g_regen.integer;
				if(ent->health>client->ps.stats[STAT_MAX_HEALTH])
					ent->health= client->ps.stats[STAT_MAX_HEALTH];
			}
		}

		G_SendWeaponProperties( ent );		//send game setting to client for sync
		G_SendSwepWeapons( ent );			//send sweps list to client for sync

		// count down armor when over max
		if ( !ent->singlebot ){
		if ( client->ps.stats[STAT_ARMOR] > client->ps.stats[STAT_MAX_HEALTH] ) {
			client->ps.stats[STAT_ARMOR]--;
		}
		}
		if ( client->ps.stats[STAT_ARMOR] < client->ps.stats[STAT_MAX_HEALTH] ) {
			client->ps.stats[STAT_ARMOR]+=g_regenarmor.integer;
		}
	}
	if( bg_itemlist[client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_AMMOREGEN ) {
		int w, max, inc, t, i;
    int weapList[]={WP_MACHINEGUN,WP_SHOTGUN,WP_GRENADE_LAUNCHER,WP_ROCKET_LAUNCHER,WP_LIGHTNING,WP_RAILGUN,WP_PLASMAGUN,WP_BFG,WP_NAILGUN,WP_PROX_LAUNCHER,WP_CHAINGUN,WP_FLAMETHROWER,WP_ANTIMATTER};
    int weapCount = sizeof(weapList) / sizeof(int);
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
			  case WP_FLAMETHROWER: max = 100; inc = 5; t = 1000; break;
			  case WP_ANTIMATTER: max = 5; inc = 1; t = 4000; break;
			  default: max = 0; inc = 0; t = 1000; break;
		  }
		  client->ammoTimes[w] += msec;
		  if (g_ammoregen_infammo.integer == 1) {
			  max = 9999;
			  inc = 9999;
			  t = 1;
		  }

		  if ( ent->swep_ammo[w] >= max ) {
			  client->ammoTimes[w] = 0;
		  }
		  if ( client->ammoTimes[w] >= t ) {
			  while ( client->ammoTimes[w] >= t )
				  client->ammoTimes[w] -= t;
			  Add_Ammo( ent, w, inc );
			  if ( ent->swep_ammo[w] > max ) {
				  ent->swep_ammo[w] = max;
			  }
		  }
    }
	}
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

        if( g_entities[client->ps.clientNum].r.svFlags & SVF_BOT )
            return; //Bots cannot mark themself as ready

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
	vec3_t	dir;
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
				damage = g_falldamagebig.integer;
			} else {
				damage = g_falldamagesmall.integer;
			}
			VectorSet (dir, 0, 0, 1);
			ent->pain_debounce_time = level.time + 200;	// no normal pain sound
			G_Damage (ent, NULL, NULL, NULL, NULL, damage, 0, MOD_FALLING);
			break;

		case EV_FIRE_WEAPON:
			FireWeapon( ent );
			break;

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
			if ( ent->teleporterTarget ) {
				FindTeleporterTarget( ent, origin, angles );
				ent->teleporterTarget = NULL;		//reset the teleporter target again
			}
			else
				
			if (g_gametype.integer == GT_DOUBLE_D) {
			SelectDoubleDominationSpawnPoint (ent->client->sess.sessionTeam, origin, angles);
			TeleportPlayer( ent, origin, angles );
			} else if (g_gametype.integer >= GT_CTF && g_ffa_gt==0 && g_gametype.integer!= GT_DOMINATION) {
			SelectCTFSpawnPoint ( ent->client->sess.sessionTeam, ent->client->pers.teamState.state, origin, angles);
			TeleportPlayer( ent, origin, angles );
			} else {
			SelectSpawnPoint( ent->client->ps.origin, origin, angles );
			TeleportPlayer( ent, origin, angles );
			}
			break;

		case EV_USE_ITEM2:		// medkit
			ent->health += g_medkitmodifier.integer;

			break;

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
			ent->client->invulnerabilityTime = level.time + g_invultime.integer*1000;
			break;

		default:
			break;
		}
	}

}

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

void PhysgunHold(gentity_t *player) {
	gentity_t *findent;
	vec3_t		end;
	vec3_t		newvelocity;
	
	if(!g_allowphysgun.integer){
		return; 
	}

	if (player->client->ps.generic2 == WP_GRAVITYGUN){
		return; 
	}
	
    if (player->client->ps.generic2 == WP_PHYSGUN && player->client->buttons & BUTTON_ATTACK && player->client->ps.stats[STAT_HEALTH] && player->client->ps.pm_type != PM_DEAD) {
        if (!player->grabbedEntity) {
            findent = FindEntityForPhysgun(player, PHYSGUN_RANGE);
			if(findent && findent->isGrabbed == qfalse ){
			if(findent->owner != player->s.clientNum + 1){
				if(findent->owner != 0){
					trap_SendServerCommand( player->s.clientNum, va( "cp \"Owned by %s\n\"", findent->ownername ));
					return;
				}	
			}
			if(!findent->client){
			player->grabbedEntity = findent;
			} else if (findent->singlebot) {
			player->grabbedEntity = findent;
			}
			}
            if (player->grabbedEntity) {
                player->grabbedEntity->isGrabbed = qtrue;
				if(!player->grabbedEntity->client){
				player->grabbedEntity->grabNewPhys = 2;
				player->grabbedEntity->s.pos.trType = TR_GRAVITY;
				player->grabbedEntity->physicsObject = qtrue;
				player->grabbedEntity->sb_phys = 2;
				G_EnablePropPhysics(player->grabbedEntity);
				}
            }
        } else {
			trap_UnlinkEntity( player->grabbedEntity );			//Unlink entity for check coll for other props
			CrosshairPointPhys(player, player->grabDist, end);	//player->grabDist set in FindEntityForPhysgun()
			trap_LinkEntity( player->grabbedEntity );			//Link entity for work prop-flight
			VectorAdd(end, player->grabOffset, end);			//physgun offset
			VectorCopy(player->grabbedEntity->r.currentOrigin, player->grabbedEntity->grabOldOrigin);	//save old frame for speed apply
			player->grabbedEntity->lastPlayer = player;		//for save attacker
			if(!player->grabbedEntity->client){
			VectorCopy(end, player->grabbedEntity->s.origin);
			VectorCopy(end, player->grabbedEntity->s.pos.trBase);
			VectorCopy(end, player->grabbedEntity->r.currentOrigin);
			G_EnablePropPhysics(player->grabbedEntity);
			} else {
			VectorCopy ( end, player->grabbedEntity->client->ps.origin );
			player->grabbedEntity->client->ps.origin[2] += 1;				//player not stuck
			VectorCopy( player->grabbedEntity->client->ps.origin, player->grabbedEntity->r.currentOrigin );
			}
			VectorSubtract(player->grabbedEntity->r.currentOrigin, player->grabbedEntity->grabOldOrigin, newvelocity);		//calc speed with old frame
			if(!player->grabbedEntity->client){
			VectorScale(newvelocity, 45, newvelocity);																		//vector sens
			VectorAdd(player->grabbedEntity->s.pos.trDelta, newvelocity, player->grabbedEntity->s.pos.trDelta);				//apply new speed
			if(player->client->pers.cmd.buttons & BUTTON_GESTURE){
				player->grabbedEntity->s.apos.trBase[0] = player->client->pers.cmd.angles[0];
				player->grabbedEntity->s.apos.trBase[1] = player->client->pers.cmd.angles[1];
			}
			} else {
			VectorScale(newvelocity, 5, newvelocity);																		//vector player sens
			VectorAdd(player->grabbedEntity->client->ps.velocity, newvelocity, player->grabbedEntity->client->ps.velocity);		//apply new player speed	
			}
        }
    } else if (player->grabbedEntity) {
        player->grabbedEntity->isGrabbed = qfalse;				//start
		if(player->grabbedEntity->grabNewPhys == 1){
			if(!player->grabbedEntity->client){
			player->grabbedEntity->s.pos.trType = TR_STATIONARY;	//phys 1 settings
			player->grabbedEntity->physicsObject = qfalse;			//phys 1 settings
			player->grabbedEntity->sb_phys = 1;						//phys 1 settings
			VectorClear( player->grabbedEntity->s.pos.trDelta );	//clear speed
			} else {
			VectorClear( player->grabbedEntity->client->ps.velocity );	//clear speed
			VectorSubtract(player->grabbedEntity->r.currentOrigin, player->grabbedEntity->grabOldOrigin, newvelocity);		//calc player speed with old frame
			VectorScale(newvelocity, 5, newvelocity);																		//vector player sens
			VectorAdd(player->grabbedEntity->client->ps.velocity, newvelocity, player->grabbedEntity->client->ps.velocity);		//apply new player speed	
			}
		}
		if(player->grabbedEntity->grabNewPhys == 2){
			if(!player->grabbedEntity->client){
			player->grabbedEntity->s.pos.trType = TR_GRAVITY;		//phys 2 settings
			player->grabbedEntity->s.pos.trTime = level.time;		//phys 2 settings
			player->grabbedEntity->physicsObject = qtrue;			//phys 2 settings
			player->grabbedEntity->sb_phys = 2;						//phys 2 settings
			VectorClear( player->grabbedEntity->s.pos.trDelta );	//clear speed
			} else {
			VectorClear( player->grabbedEntity->client->ps.velocity );	//clear player speed	
			}
			VectorSubtract(player->grabbedEntity->r.currentOrigin, player->grabbedEntity->grabOldOrigin, newvelocity);		//calc speed with old frame
			if(!player->grabbedEntity->client){
			VectorScale(newvelocity, 15, newvelocity);																		//vector sens
			VectorAdd(player->grabbedEntity->s.pos.trDelta, newvelocity, player->grabbedEntity->s.pos.trDelta);				//apply new speed
			} else {
			VectorScale(newvelocity, 5, newvelocity);																		//vector player sens
			VectorAdd(player->grabbedEntity->client->ps.velocity, newvelocity, player->grabbedEntity->client->ps.velocity);		//apply new player speed
			}
		}
		VectorClear( player->grabOffset );																				//clear offset
		player->grabbedEntity = 0;																						//end
    }
}

void GravitygunHold(gentity_t *player) {
	gentity_t *findent;
	vec3_t		end;
	vec3_t		newvelocity;
	
	if(!g_allowgravitygun.integer){
		return; 
	}

	if (player->client->ps.generic2 == WP_PHYSGUN){
		return; 
	}
	
    if (player->client->ps.generic2 == WP_GRAVITYGUN && player->client->buttons & BUTTON_ATTACK && player->client->ps.stats[STAT_HEALTH] && player->client->ps.pm_type != PM_DEAD) {
        if (!player->grabbedEntity) {
            findent = FindEntityForGravitygun(player, GRAVITYGUN_RANGE);
			if(findent && findent->isGrabbed == qfalse ){
			if(findent->owner != player->s.clientNum + 1){
				if(findent->owner != 0){
					trap_SendServerCommand( player->s.clientNum, va( "cp \"Owned by %s\n\"", findent->ownername ));
					return;
				}	
			}
			if(!findent->client){
			player->grabbedEntity = findent;
			} else if (findent->singlebot) {
			player->grabbedEntity = findent;
			}
			}
            if (player->grabbedEntity) {
                player->grabbedEntity->isGrabbed = qtrue;
				if(!player->grabbedEntity->client){
				player->grabbedEntity->s.pos.trType = TR_GRAVITY;
				player->grabbedEntity->physicsObject = qtrue;
				player->grabbedEntity->sb_phys = 2;
				G_EnablePropPhysics(player->grabbedEntity);
				}
            }
        } else {
			trap_UnlinkEntity( player->grabbedEntity );			//Unlink entity for check coll for other props
			CrosshairPointGravity(player, GRAVITYGUN_DIST, end);			//player->grabDist set in FindEntityForPhysgun()
			trap_LinkEntity( player->grabbedEntity );			//Link entity for work prop-flight
			VectorCopy(player->grabbedEntity->r.currentOrigin, player->grabbedEntity->grabOldOrigin);	//save old frame for speed apply
			player->grabbedEntity->lastPlayer = player;		//for save attacker
			if(!player->grabbedEntity->client){
			VectorCopy(end, player->grabbedEntity->s.origin);
			VectorCopy(end, player->grabbedEntity->s.pos.trBase);
			VectorCopy(end, player->grabbedEntity->r.currentOrigin);
			VectorClear( player->grabbedEntity->s.pos.trDelta );	//clear speed
			G_EnablePropPhysics(player->grabbedEntity);
			} else {
			VectorClear( player->grabbedEntity->client->ps.velocity );	//clear player speed	
			VectorCopy ( end, player->grabbedEntity->client->ps.origin );
			VectorCopy( player->grabbedEntity->client->ps.origin, player->grabbedEntity->r.currentOrigin );
			}
        }
    } else if (player->grabbedEntity) {
        player->grabbedEntity->isGrabbed = qfalse;				//start
			if(!player->grabbedEntity->client){
			player->grabbedEntity->physicsObject = qtrue;			//phys 2 settings
			player->grabbedEntity->sb_phys = 2;						//phys 2 settings
			G_EnablePropPhysics(player->grabbedEntity);				//turn phys
			VectorClear( player->grabbedEntity->s.pos.trDelta );	//clear speed
			} else {
			VectorClear( player->grabbedEntity->client->ps.velocity );	//clear player speed	
			}
			VectorSubtract(player->grabbedEntity->r.currentOrigin, player->r.currentOrigin, newvelocity);		//calc speed with player pos and prop pos
			if(!player->grabbedEntity->client){
			VectorScale(newvelocity, 10, newvelocity);																		//vector sens
			VectorAdd(player->grabbedEntity->s.pos.trDelta, newvelocity, player->grabbedEntity->s.pos.trDelta);				//apply new speed
			} else {
			VectorScale(newvelocity, 10, newvelocity);																			//vector player sens
			VectorAdd(player->grabbedEntity->client->ps.velocity, newvelocity, player->grabbedEntity->client->ps.velocity);		//apply new player speed
			}
		VectorClear( player->grabOffset );																				//clear offset
		player->grabbedEntity = 0;																						//end
		G_AddEvent( player, EV_GRAVITYSOUND, 0 );
    }
}

void CheckCarCollisions(gentity_t *ent) {
	vec3_t newMins, newMaxs;
    trace_t tr;
	gentity_t *hit;
	float impactForce;
	vec3_t impactVector;
	vec3_t end, start, forward, up, right;
	
	if(BG_VehicleCheckClass(ent->client->ps.stats[STAT_VEHICLE]) != VCLASS_CAR && ent->botskill != 9){
	return;
	}
	
	//Set Aiming Directions
	AngleVectors(ent->client->ps.viewangles, forward, right, up);
	CalcMuzzlePoint(ent, forward, right, up, start);
	VectorMA (start, 1, forward, end);
	
	VectorCopy(ent->r.mins, newMins);
	VectorCopy(ent->r.maxs, newMaxs);
	VectorScale(newMins, 1.15, newMins);
	VectorScale(newMaxs, 1.15, newMaxs);
	newMins[2] *= 0.20;
	newMaxs[2] *= 0.20;
	trap_Trace(&tr, ent->r.currentOrigin, newMins, newMaxs, end, ent->s.number, MASK_PLAYERSOLID);
	
	if (tr.fraction < 1.0f && tr.entityNum != ENTITYNUM_NONE) {
        hit = &g_entities[tr.entityNum];

        if (hit->s.number != ent->s.number) {  // Ignore self
            // Calculate the impact force
            impactForce = sqrt(ent->client->ps.velocity[0] * ent->client->ps.velocity[0] + ent->client->ps.velocity[1] * ent->client->ps.velocity[1]);

            // Optionally apply a force or velocity to the hit entity to simulate the push
			if (impactForce > VEHICLE_SENS) {
			if (!hit->client){
			G_EnablePropPhysics(hit);
			}
			VectorCopy(ent->client->ps.velocity, impactVector);
			VectorScale(impactVector, VEHICLE_PROP_IMPACT, impactVector);
			impactVector[2] = impactForce*0.15;
			if (!hit->client){
			hit->lastPlayer = ent;		//for save attacker
            VectorAdd(hit->s.pos.trDelta, impactVector, hit->s.pos.trDelta);  // Transfer velocity from the prop to the hit entity
			} else {
			VectorAdd(hit->client->ps.velocity, impactVector, hit->client->ps.velocity);  // Transfer velocity from the prop to the hit player
			}
			}
			if(impactForce > VEHICLE_DAMAGESENS){
			if(hit->grabbedEntity != ent){
			if(BG_VehicleCheckClass(ent->client->ps.stats[STAT_VEHICLE]) == VCLASS_CAR){
				G_CarDamage(hit, ent, (int)(impactForce * VEHICLE_DAMAGE));
			}
			}
			}
			if(impactForce > VEHICLE_DAMAGESENS*6){
				if(BG_VehicleCheckClass(ent->client->ps.stats[STAT_VEHICLE]) == VCLASS_CAR){
					G_PropSmoke( ent, impactForce*0.20);
				}
			}
        }
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
	gentity_t	*vehicle;
	pmove_t		pm;
	int			oldEventSequence;
	int			msec;
	usercmd_t	*ucmd;

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

	client->frameOffset = trap_Milliseconds() - level.frameStartTime;

	// we use level.previousTime to account for 50ms lag correction
	// besides, this will turn out numbers more like what players are used to
	client->pers.pingsamples[client->pers.samplehead] = level.previousTime + client->frameOffset - ucmd->serverTime;
	client->pers.samplehead++;
	if ( client->pers.samplehead >= NUM_PING_SAMPLES ) {
		client->pers.samplehead -= NUM_PING_SAMPLES;
	}

	// initialize the real ping
	if ( g_truePing.integer ) {
		int i, sum = 0;

		// get an average of the samples we saved up
		for ( i = 0; i < NUM_PING_SAMPLES; i++ ) {
			sum += client->pers.pingsamples[i];
		}

		client->pers.realPing = sum / NUM_PING_SAMPLES;
	}
	else {
		// if g_truePing is off, use the normal ping
		client->pers.realPing = client->ps.ping;
	}
	
	client->attackTime = ucmd->serverTime;

	client->lastUpdateFrame = level.framenum;

	if ( client->pers.realPing < 0 ) {
		client->pers.realPing = 0;
	}

	msec = ucmd->serverTime - client->ps.commandTime;
	if ( msec < 1 && client->sess.spectatorState != SPECTATOR_FOLLOW ) {
		return;
	}
	if ( msec > 200 ) {
		msec = 200;
	}

	if ( pmove_msec.integer < 1 ) {
		trap_Cvar_Set("pmove_msec", "1");
	}
	else if (pmove_msec.integer > 125) {
		trap_Cvar_Set("pmove_msec", "125");
	}

	if ( pmove_fixed.integer || client->pers.pmoveFixed ) {
		ucmd->serverTime = ((ucmd->serverTime + pmove_msec.integer-1) / pmove_msec.integer) * pmove_msec.integer;
	}

	//
	// check for exiting intermission
	//
	if ( level.intermissiontime ) {
		ClientIntermissionThink( client );
		return;
	}

	// spectators don't do much
	if ( (client->sess.sessionTeam == TEAM_SPECTATOR) || client->isEliminated ) {
		if ( client->sess.spectatorState == SPECTATOR_SCOREBOARD ) {
			return;
		}
		SpectatorThink( ent, ucmd );
		return;
	}

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
	} else if ( client->ps.pm_type != PM_FREEZE && client->ps.pm_type != PM_CUTSCENE ) {
		client->ps.pm_type = PM_NORMAL;
	}
	// set gravity
if ( !client->ps.gravity ){
		if(client->sess.sessionTeam == TEAM_FREE){
			client->ps.gravity = g_gravity.value*g_gravityModifier.value;
		}
		if(client->sess.sessionTeam == TEAM_BLUE){
			client->ps.gravity = g_gravity.value*g_teamblue_gravityModifier.value;
		}
		if(client->sess.sessionTeam == TEAM_RED){
			client->ps.gravity = g_gravity.value*g_teamred_gravityModifier.value;
		}
}
		if( bg_itemlist[client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_SCOUT ) {
			client->ps.gravity *= g_scoutgravitymodifier.value;
		}
		if( bg_itemlist[client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_AMMOREGEN ) {
			client->ps.gravity *= g_ammoregengravitymodifier.value;
		}
		if( bg_itemlist[client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_DOUBLER ) {
			client->ps.gravity *= g_doublergravitymodifier.value;
		}
		if( bg_itemlist[client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_GUARD ) {
			client->ps.gravity *= g_guardgravitymodifier.value;
		}


	// set speed
if ( !ent->speed ){
	if ( !ent->client->noclip ) {
	client->ps.speed = g_speed.value;
	} else {
	client->ps.speed = g_speed.value*2.5;	
	}
	if(client->sess.sessionTeam == TEAM_BLUE){
	client->ps.speed = g_teamblue_speed.integer;
	}
	if(client->sess.sessionTeam == TEAM_RED){
	client->ps.speed = g_teamred_speed.integer;
	}
}
	else if ( ent->speed == -1 )
		client->ps.speed = 0;
	else
		client->ps.speed = ent->speed;			//ent->speed holds a modified speed value that's set by a target_playerspeed
	
	if(client->vehiclenum){	//VEHICLE-SYSTEM: setup physics for all
	if(G_FindEntityForEntityNum(client->vehiclenum)){
	vehicle = G_FindEntityForEntityNum(client->vehiclenum);
	client->ps.stats[STAT_VEHICLE] = vehicle->vehicle;
	if(BG_VehicleCheckClass(vehicle->vehicle)){
	client->ps.speed = BG_GetVehicleSettings(vehicle->vehicle, VSET_SPEED);
	client->ps.gravity = (g_gravity.value*g_gravityModifier.value)*BG_GetVehicleSettings(vehicle->vehicle, VSET_GRAVITY);
	}
	}
	} else {
	client->ps.stats[STAT_VEHICLE] = VCLASS_NONE;
	}
	
	if( bg_itemlist[client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_SCOUT ) {
		client->ps.speed *= g_scoutspeedfactor.value;
	}
	if( bg_itemlist[client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_AMMOREGEN ) {
		client->ps.speed *= g_ammoregenspeedfactor.value;
	}
	if( bg_itemlist[client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_DOUBLER ) {
		client->ps.speed *= g_doublerspeedfactor.value;
	}
	if( bg_itemlist[client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_GUARD ) {
		client->ps.speed *= g_guardspeedfactor.value;
	}
	if ( client->ps.powerups[PW_HASTE] ) {
		client->ps.speed *= g_speedfactor.value;
	}
	if ( ent->botskill == 9 ) {
		client->ps.speed *= 2.20;
	}

	// Let go of the hook if we aren't firing
	if ( client->ps.generic2 == WP_GRAPPLING_HOOK &&
		client->hook && !( ucmd->buttons & BUTTON_ATTACK ) ) {
		Weapon_HookFree(client->hook);
	}

	// set up for pmove
	oldEventSequence = client->ps.eventSequence;

	memset (&pm, 0, sizeof(pm));

	// check for the hit-scan gauntlet, don't let the action
	// go through as an attack unless it actually hits something
	if ( client->ps.generic2 == WP_GAUNTLET && !( ucmd->buttons & BUTTON_TALK ) &&
		( ucmd->buttons & BUTTON_ATTACK ) && client->ps.weaponTime <= 0 ) {
		pm.gauntletHit = CheckGauntletAttack( ent );
	}

	if ( ent->flags & FL_FORCE_GESTURE ) {
		ent->flags &= ~FL_FORCE_GESTURE;
		ent->client->pers.cmd.buttons |= BUTTON_GESTURE;
	}

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

	pm.ps = &client->ps;
	pm.cmd = *ucmd;
	if ( pm.ps->pm_type == PM_DEAD ) {
		pm.tracemask = MASK_PLAYERSOLID & ~CONTENTS_BODY;
	}
	else if ( ent->r.svFlags & SVF_BOT ) {
		pm.tracemask = MASK_PLAYERSOLID | CONTENTS_BOTCLIP;
	}
	else {
		pm.tracemask = MASK_PLAYERSOLID;
	}
	pm.trace = trap_Trace;
	pm.pointcontents = trap_PointContents;
	pm.debugLevel = g_debugMove.integer;
	pm.noFootsteps = ( g_dmflags.integer & DF_NO_FOOTSTEPS ) > 0;

	pm.pmove_fixed = pmove_fixed.integer | client->pers.pmoveFixed;
	pm.pmove_msec = pmove_msec.integer;
        pm.pmove_float = pmove_float.integer;
        pm.pmove_flags = g_dmflags.integer;

	VectorCopy( client->ps.origin, client->oldOrigin );
	Pmove (&pm);

	// save results of pmove
	if ( ent->client->ps.eventSequence != oldEventSequence ) {
		ent->eventTime = level.time;
	}
	BG_PlayerStateToEntityState( &ent->client->ps, &ent->s, qtrue );
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
	ent->client->ps.generic2 = ent->swep_id;
	ent->client->ps.stats[STAT_SWEPAMMO] = ent->swep_ammo[ent->swep_id];
	ent->s.generic3 = ent->swep_ammo[ent->swep_id];
	if(ent->singlebot){
	if(!G_NpcFactionProp(NP_PICKUP, ent)){
	ent->client->ps.stats[STAT_NO_PICKUP] = 1;
	ent->wait_to_pickup = 70000000;
	}}

	// execute client events
	ClientEvents( ent, oldEventSequence );

	// link entity now, after any personal teleporters have been used
	trap_LinkEntity (ent);
	if ( !ent->client->noclip ) {
		G_TouchTriggers( ent );
		G_KillVoid( ent );
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

	PhysgunHold( ent );
	GravitygunHold( ent );
	
	CheckCarCollisions( ent );

	// check for respawning
	if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
		// wait for the attack button to be pressed
		// forcerespawn is to prevent users from waiting out powerups
		// In Last man standing, we force a quick respawn, since
		// the player must be able to loose health
		// pressing attack or use is the normal respawn method
		if ( ( level.time > client->respawnTime ) &&
			( ( ( g_forcerespawn.integer > 0 ) &&
			( level.time - client->respawnTime  > g_forcerespawn.integer * 1000 ) ) ||
			( ( ( g_gametype.integer == GT_LMS ) ||
			( g_gametype.integer == GT_ELIMINATION ) ||
			( g_gametype.integer == GT_CTF_ELIMINATION ) ) &&
			( level.time - client->respawnTime > 0 ) ) ||
			( ucmd->buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) ) ) ) {

			ClientRespawn( ent );
		}
		return;
	}

        if ( pm.waterlevel <= 1 && pm.ps->groundEntityNum!=ENTITYNUM_NONE && client->lastSentFlyingTime+500>level.time) {
			if ( ! (pm.ps->pm_flags & PMF_TIME_KNOCKBACK) ) {
                            client->lastSentFlying = -1;
			}
	}

	// perform once-a-second actions
	ClientTimerActions( ent, msec );
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

qboolean G_CheckSwep( int clientNum, int wp, int finish ) {
gentity_t *ent;

ent = g_entities + clientNum;

if(ent->swep_list[wp] == 1){
	if(finish){
	ent->swep_id = wp;
	}
	ClientUserinfoChanged(clientNum);
	return qtrue;
} else {
	return qfalse;
}
}

int G_CheckSwepAmmo( int clientNum, int wp ) {
gentity_t *ent;

ent = g_entities + clientNum;

return ent->swep_ammo[wp];
}

void PM_Add_SwepAmmo( int clientNum, int wp, int count ) {
	gentity_t *ent;

	ent = g_entities + clientNum;
	
if(!(ent->swep_ammo[wp] == -1)){
if(!(ent->swep_ammo[wp] >= 9999)){
	ent->swep_ammo[wp] += count;
	
}
}
}

void G_DefaultSwep( int clientNum, int wp ) {
gentity_t *ent;

ent = g_entities + clientNum;
ent->swep_id = wp;
ClientUserinfoChanged(clientNum);

return;
}

/*
==================
SpectatorClientEndFrame

==================
*/
void SpectatorClientEndFrame( gentity_t *ent ) {
	gclient_t	*cl;
	int i, preservedScore[MAX_PERSISTANT]; //for keeping in elimination

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
			if ( cl->pers.connected == CON_CONNECTED && cl->sess.sessionTeam != TEAM_SPECTATOR ) {
				flags = (cl->ps.eFlags & ~(EF_VOTED | EF_TEAMVOTED)) | (ent->client->ps.eFlags & (EF_VOTED | EF_TEAMVOTED));
				//this is here LMS/Elimination goes wrong with player follow
				if(ent->client->sess.sessionTeam!=TEAM_SPECTATOR){
					for(i = 0; i < MAX_PERSISTANT; i++)
						preservedScore[i] = ent->client->ps.persistant[i];
					ent->client->ps = cl->ps;
					for(i = 0; i < MAX_PERSISTANT; i++)
						ent->client->ps.persistant[i] = preservedScore[i];
				}
				else
					ent->client->ps = cl->ps;
				ent->client->ps.pm_flags |= PMF_FOLLOW;
				ent->client->ps.eFlags = flags;
				return;
			} else {
				// drop them to free spectators unless they are dedicated camera followers
				if ( ent->client->sess.spectatorClient >= 0 ) {
					ent->client->sess.spectatorState = SPECTATOR_FREE;
					ClientBegin( ent->client - level.clients );
				}
			}
		}



	}

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
	clientPersistant_t	*pers;

//unlagged - smooth clients #1
	int frames;
//unlagged - smooth clients #1

	if ( (ent->client->sess.sessionTeam == TEAM_SPECTATOR) || ent->client->isEliminated ) {
		SpectatorClientEndFrame( ent );
		return;
	}

	pers = &ent->client->pers;

	// turn off any expired powerups
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( ent->client->ps.powerups[ i ] < level.time ) {
			ent->client->ps.powerups[ i ] = 0;
		}
	}

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

	ent->client->ps.stats[STAT_HEALTH] = ent->health;	// FIXME: get rid of ent->health...

	G_SetClientSound (ent);

//Unlagged: Always do the else clause
	// set the latest infor
/*	if (g_smoothClients.integer) {
		BG_PlayerStateToEntityStateExtraPolate( &ent->client->ps, &ent->s, ent->client->ps.commandTime, qtrue );
	}
	else { */
		BG_PlayerStateToEntityState( &ent->client->ps, &ent->s, qtrue );
//	}
	SendPendingPredictableEvents( &ent->client->ps );

//unlagged - smooth clients #1
	// mark as not missing updates initially
	ent->client->ps.eFlags &= ~EF_CONNECTION;

	// see how many frames the client has missed
	frames = level.framenum - ent->client->lastUpdateFrame - 1;

	// don't extrapolate more than two frames
	if ( frames > 2 ) {
		frames = 2;

		// if they missed more than two in a row, show the phone jack
		ent->client->ps.eFlags |= EF_CONNECTION;
		ent->s.eFlags |= EF_CONNECTION;
	}

	// did the client miss any frames?
	if ( frames > 0 && g_smoothClients.integer ) {
		// yep, missed one or more, so extrapolate the player's movement
		G_PredictPlayerMove( ent, (float)frames / sv_fps.integer );
		// save network bandwidth
		SnapVector( ent->s.pos.trBase );
	}
//unlagged - smooth clients #1

//unlagged - backward reconciliation #1
	// store the client's position for backward reconciliation later
	G_StoreHistory( ent );
//unlagged - backward reconciliation #1

	// set the bit for the reachability area the client is currently in
//	i = trap_AAS_PointReachabilityAreaIndex( ent->client->ps.origin );
//	ent->client->areabits[i >> 3] |= 1 << (i & 7);
}
