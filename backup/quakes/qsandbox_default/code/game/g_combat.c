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
// g_combat.c

#include "g_local.h"

/*
============
ScorePlum
============
*/
void ScorePlum( gentity_t *ent, vec3_t origin, int score, int dmgf )
{
	gentity_t *plum;
	plum = G_TempEntity( origin, EV_SCOREPLUM );
	plum->r.singleClient = ent->s.number;
	plum->s.otherEntityNum = ent->s.number;
        if (dmgf == 10 || dmgf == 11) {
                plum->r.svFlags |= SVF_BROADCAST;
        } else {
                plum->r.svFlags |= SVF_SINGLECLIENT;
        }
	plum->s.time = score;
    plum->s.weapon = dmgf;
}

/*
============
AddScore

Adds score to both the client and his team
============
*/
void AddScore( gentity_t *ent, vec3_t origin, int score ) {
        int i;

	if ( !ent->client ) {
		return;
	}
	// no scoring during pre-match warmup
	if ( level.warmupTime ) {
		return;
	}

        //No scoring during intermission
        if ( level.intermissiontime ) {
            return;
        }
	// show score plum
        if( level.numNonSpectatorClients<3 && score < 0 && (g_gametype.integer<GT_TEAM || g_ffa_gt==1)) {
            for ( i = 0 ; i < level.maxclients ; i++ ) {
                if ( level.clients[ i ].pers.connected != CON_CONNECTED )
                    continue; //Client was not connected

                if (level.clients[i].sess.sessionTeam == TEAM_SPECTATOR)
                    continue; //Don't give anything to spectators

                if (g_entities+i == ent)
                    continue; //Don't award dead one

                level.clients[i].ps.persistant[PERS_SCORE] -= score;
                //ScorePlum(ent, origin, -score, 0);
            }
        }
        else {
            //ScorePlum(ent, origin, score, 0);
            //
			if(score == 1){
            ent->client->ps.persistant[PERS_SCORE] += score;
            if ( g_gametype.integer == GT_TEAM ) {
                int team = ent->client->ps.persistant[PERS_TEAM];
                    level.teamScores[ team ] += score;
					G_LogPrintf("TeamScore: add 1\n" );
                    G_LogPrintf("TeamScore: %i %i: Team %d now has %d points\n",
                        team, level.teamScores[ team ], team, level.teamScores[ team ] );
            }
			}
        }
        G_LogPrintf("PlayerScore: %i %i: %s now has %d points\n",
		ent->s.number, ent->client->ps.persistant[PERS_SCORE], ent->client->pers.netname, ent->client->ps.persistant[PERS_SCORE] );
	CalculateRanks();
}

/*
=================
TossClientItems

Toss the weapon and powerups for the killed player
=================
*/
void TossClientItems( gentity_t *self ) {
	gitem_t		*item;
	int			weapon;
	float		angle;
	int			i;
	gentity_t	*drop;

	// drop the weapon if not a gauntlet or machinegun
	weapon = self->s.weapon;

        if(!g_npcdrop.integer){
		if(self->singlebot >= 1){
        return;
		}
		}

	//Never drop in elimination or last man standing mode!
	if( g_gametype.integer == GT_ELIMINATION || g_gametype.integer == GT_LMS)
		return;

if(!self->singlebot){
if(g_gametype.integer != GT_SINGLE){
	if (g_gametype.integer == GT_CTF_ELIMINATION || g_elimination_allgametypes.integer || weapon == WP_GAUNTLET){
	//Nothing!
	}
	else
	if ( self->swep_ammo[ weapon ] ) {
		// find the item type for this weapon
		item = BG_FindItemForWeapon( weapon );

		// spawn the item
		Drop_Item( self, item, 0 );
	}
}
if(g_gametype.integer == GT_SINGLE){
	//the player drops a backpack in single player
	item = BG_FindItemForBackpack(); 
	Drop_Item( self, item, 0 );
}
}
if(self->singlebot){
	if (g_gametype.integer == GT_CTF_ELIMINATION || g_elimination_allgametypes.integer || weapon == WP_GAUNTLET){
	//Nothing!
	}
	else
	if ( self->swep_ammo[ weapon ] ) {
		// find the item type for this weapon
		item = BG_FindItemForWeapon( weapon );

		// spawn the item
		Drop_Item( self, item, 0 );
	}
}

	// drop all the powerups if not in teamplay

		angle = 45;
		for ( i = 1 ; i < PW_NUM_POWERUPS ; i++ ) {
			if ( self->client->ps.powerups[ i ] > level.time ) {
				item = BG_FindItemForPowerup( i );
				if ( !item ) {
					continue;
				}
				drop = Drop_Item( self, item, angle );
				// decide how many seconds it has left
				drop->count = ( self->client->ps.powerups[ i ] - level.time ) / 1000;
				if ( drop->count < 1 ) {
					drop->count = 1;
				}
				angle += 45;
			}
		}
	if(!self->singlebot){
	if(g_gametype.integer != GT_SINGLE){
	if(self->client->ps.stats[STAT_HOLDABLE_ITEM] & (1 << HI_TELEPORTER)) {
	item = BG_FindItem( "Personal Teleporter" );
	Drop_Item( self, item, 0 );
	}
	if(self->client->ps.stats[STAT_HOLDABLE_ITEM] & (1 << HI_MEDKIT)) {
	item = BG_FindItem( "Medkit" );
	Drop_Item( self, item, 0 );
	}
	if(self->client->ps.stats[STAT_HOLDABLE_ITEM] & (1 << HI_KAMIKAZE)) {
	item = BG_FindItem( "Kamikaze" );
	Drop_Item( self, item, 0 );
	}
	if(self->client->ps.stats[STAT_HOLDABLE_ITEM] & (1 << HI_INVULNERABILITY)) {
	item = BG_FindItem( "Invulnerability" );
	Drop_Item( self, item, 0 );
	}
	if(self->client->ps.stats[STAT_HOLDABLE_ITEM] & (1 << HI_PORTAL)) {
	item = BG_FindItem( "Portal" );
	Drop_Item( self, item, 0 );
	}
	}
	}
	if(self->singlebot){
	if(self->client->ps.stats[STAT_HOLDABLE_ITEM] & (1 << HI_TELEPORTER)) {
	item = BG_FindItem( "Personal Teleporter" );
	Drop_Item( self, item, 0 );
	}
	if(self->client->ps.stats[STAT_HOLDABLE_ITEM] & (1 << HI_MEDKIT)) {
	item = BG_FindItem( "Medkit" );
	Drop_Item( self, item, 0 );
	}
	if(self->client->ps.stats[STAT_HOLDABLE_ITEM] & (1 << HI_KAMIKAZE)) {
	item = BG_FindItem( "Kamikaze" );
	Drop_Item( self, item, 0 );
	}
	if(self->client->ps.stats[STAT_HOLDABLE_ITEM] & (1 << HI_INVULNERABILITY)) {
	item = BG_FindItem( "Invulnerability" );
	Drop_Item( self, item, 0 );
	}
	if(self->client->ps.stats[STAT_HOLDABLE_ITEM] & (1 << HI_PORTAL)) {
	item = BG_FindItem( "Portal" );
	Drop_Item( self, item, 0 );
	}
	}

}

/*
=================
TossClientCubes
=================
*/
extern gentity_t	*neutralObelisk;

void TossClientCubes( gentity_t *self ) {
	gitem_t		*item;
	gentity_t	*drop;
	vec3_t		velocity;
	vec3_t		angles;
	vec3_t		origin;

	self->client->ps.generic1 = 0;

	// this should never happen but we should never
	// get the server to crash due to skull being spawned in
	if (!G_EntitiesFree()) {
		return;
	}

	if( self->client->sess.sessionTeam == TEAM_RED ) {
		item = BG_FindItem( "Red Cube" );
	}
	else {
		item = BG_FindItem( "Blue Cube" );
	}

	angles[YAW] = (float)(level.time % 360);
	angles[PITCH] = 0;	// always forward
	angles[ROLL] = 0;

	AngleVectors( angles, velocity, NULL, NULL );
	VectorScale( velocity, 150, velocity );
	velocity[2] += 200 + crandom() * 50;

	if( neutralObelisk ) {
		VectorCopy( neutralObelisk->s.pos.trBase, origin );
		origin[2] += 44;
	} else {
		VectorClear( origin ) ;
	}

	drop = LaunchItem( item, origin, velocity );

	drop->nextthink = level.time + g_cubeTimeout.integer * 1000;
	drop->think = G_FreeEntity;
	drop->spawnflags = self->client->sess.sessionTeam;
}

/*
=================
TossClientPersistantPowerups
=================
*/
void TossClientPersistantPowerups( gentity_t *ent ) {
	gentity_t	*powerup;

	if( !ent->client ) {
		return;
	}

	if( !ent->client->persistantPowerup ) {
		return;
	}

	powerup = ent->client->persistantPowerup;

	powerup->r.svFlags &= ~SVF_NOCLIENT;
	powerup->s.eFlags &= ~EF_NODRAW;
	powerup->r.contents = CONTENTS_TRIGGER;
	trap_LinkEntity( powerup );

	ent->client->ps.stats[STAT_PERSISTANT_POWERUP] = 0;
	ent->client->persistantPowerup = NULL;
}


/*
==================
LookAtKiller
==================
*/
void LookAtKiller( gentity_t *self, gentity_t *inflictor, gentity_t *attacker ) {
	vec3_t		dir;
	//vec3_t		angles;

	if ( attacker && attacker != self ) {
		VectorSubtract (attacker->s.pos.trBase, self->s.pos.trBase, dir);
	} else if ( inflictor && inflictor != self ) {
		VectorSubtract (inflictor->s.pos.trBase, self->s.pos.trBase, dir);
	} else {
		self->client->ps.stats[STAT_DEAD_YAW] = self->s.angles[YAW];
		return;
	}

	self->client->ps.stats[STAT_DEAD_YAW] = vectoyaw ( dir );

	/*angles[YAW] =*/ vectoyaw ( dir );
	//angles[PITCH] = 0;
	//angles[ROLL] = 0;
}

/*
==================
GibEntity
==================
*/
void GibEntity( gentity_t *self, int killer ) {
	gentity_t *ent;
	int i;

	//if this entity still has kamikaze
	if (self->s.eFlags & EF_KAMIKAZE) {
		// check if there is a kamikaze timer around for this owner
		for (i = 0; i < MAX_GENTITIES; i++) {
			ent = &g_entities[i];
			if (!ent->inuse)
				continue;
			if (ent->activator != self)
				continue;
			if (strcmp(ent->classname, "kamikaze timer"))
				continue;
			G_FreeEntity(ent);
			break;
		}
	}
	G_AddEvent( self, EV_GIB_PLAYER, killer );
	self->takedamage = qfalse;
	self->s.eType = ET_INVISIBLE;
	self->r.contents = 0;
}

/*
==================
body_die
==================
*/
void body_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
	if ( self->health > GIB_HEALTH ) {
		return;
	}
	if ( !g_blood.integer ) {
		self->health = GIB_HEALTH+1;
		return;
	}

	GibEntity( self, 0 );
}


// these are just for logging, the client prints its own messages
char	*modNames[] = {
	"MOD_UNKNOWN",
	"MOD_SHOTGUN",
	"MOD_GAUNTLET",
	"MOD_MACHINEGUN",
	"MOD_GRENADE",
	"MOD_GRENADE_SPLASH",
	"MOD_ROCKET",
	"MOD_ROCKET_SPLASH",
	"MOD_PLASMA",
	"MOD_PLASMA_SPLASH",
	"MOD_RAILGUN",
	"MOD_LIGHTNING",
	"MOD_BFG",
	"MOD_BFG_SPLASH",
	"MOD_FLAME",
	"MOD_FLAME_SPLASH",
	"MOD_ANTIMATTER",
	"MOD_ANTIMATTER_SPLASH",
	"MOD_TOOLGUN",
	"MOD_WATER",
	"MOD_SLIME",
	"MOD_LAVA",
	"MOD_CRUSH",
	"MOD_TELEFRAG",
	"MOD_FALLING",
	"MOD_SUICIDE",
	"MOD_TARGET_LASER",
	"MOD_TRIGGER_HURT",
	"MOD_NAIL",
	"MOD_CHAINGUN",
	"MOD_PROXIMITY_MINE",
	"MOD_KAMIKAZE",
	"MOD_JUICED",
	"MOD_GRAPPLE",
	"MOD_CAR",
	"MOD_CAREXPLODE",
	"MOD_PROP",
	"MOD_BREAKABLE_SPLASH"
};

/*
==================
Kamikaze_DeathActivate
==================
*/
void Kamikaze_DeathActivate( gentity_t *ent ) {
	G_StartKamikaze(ent);
	G_FreeEntity(ent);
}

/*
==================
Kamikaze_DeathTimer
==================
*/
void Kamikaze_DeathTimer( gentity_t *self ) {
	gentity_t *ent;

	ent = G_Spawn();
	ent->classname = "kamikaze timer";
	VectorCopy(self->s.pos.trBase, ent->s.pos.trBase);
	ent->r.svFlags |= SVF_NOCLIENT;
	ent->think = Kamikaze_DeathActivate;
	ent->nextthink = level.time + 50;

	ent->activator = self;
}


/*
==================
CheckAlmostCapture
==================
*/
void CheckAlmostCapture( gentity_t *self, gentity_t *attacker ) {
	gentity_t	*ent;
	vec3_t		dir;
	char		*classname;

	// if this player was carrying a flag
	if ( self->client->ps.powerups[PW_REDFLAG] ||
		self->client->ps.powerups[PW_BLUEFLAG] ||
		self->client->ps.powerups[PW_NEUTRALFLAG] ) {
		// get the goal flag this player should have been going for
		if ( g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTF_ELIMINATION) {
			if ( self->client->sess.sessionTeam == TEAM_BLUE ) {
				classname = "team_CTF_blueflag";
			}
			else {
				classname = "team_CTF_redflag";
			}
		}
		else {
			if ( self->client->sess.sessionTeam == TEAM_BLUE ) {
				classname = "team_CTF_redflag";
			}
			else {
				classname = "team_CTF_blueflag";
			}
		}
		ent = NULL;
		do
		{
			ent = G_Find(ent, FOFS(classname), classname);
		} while (ent && (ent->flags & FL_DROPPED_ITEM));
		// if we found the destination flag and it's not picked up
		if (ent && !(ent->r.svFlags & SVF_NOCLIENT) ) {
			// if the player was *very* close
			VectorSubtract( self->client->ps.origin, ent->s.origin, dir );
			if ( VectorLength(dir) < 200 ) {
				self->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_HOLYSHIT;
				if ( attacker->client ) {
					attacker->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_HOLYSHIT;
				}
			}
		}
	}
}

/*
==================
CheckAlmostScored
==================
*/
void CheckAlmostScored( gentity_t *self, gentity_t *attacker ) {
	gentity_t	*ent;
	vec3_t		dir;
	char		*classname;

	// if the player was carrying cubes
	if ( self->client->ps.generic1 ) {
		if ( self->client->sess.sessionTeam == TEAM_BLUE ) {
			classname = "team_redobelisk";
		}
		else {
			classname = "team_blueobelisk";
		}
		ent = G_Find(NULL, FOFS(classname), classname);
		// if we found the destination obelisk
		if ( ent ) {
			// if the player was *very* close
			VectorSubtract( self->client->ps.origin, ent->s.origin, dir );
			if ( VectorLength(dir) < 200 ) {
				self->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_HOLYSHIT;
				if ( attacker->client ) {
					attacker->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_HOLYSHIT;
				}
			}
		}
	}
}

/*
==================
player_die
==================
*/
void player_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
	gentity_t	*ent;
	int			anim;
	int			contents;
	int			killer;
	int			i,counter2;
	char		*killerName, *obit;

	if ( self->client->ps.pm_type == PM_DEAD ) {
		return;
	}

	if ( level.intermissiontime ) {
		return;
	}
	
	self->client->noclip = 0;

	//if we're in SP mode and player killed a bot, award score for the kill
	if(g_gametype.integer == GT_SINGLE){
	if ( self->singlebot == 1 ) {
		if ( self->botspawn && self->botspawn->health && attacker->client ) {
			AddScore( attacker, self->r.currentOrigin, self->botspawn->health );
			self->s.time = level.time;
		}
	}		
	}
			
	self->client->pers.oldmoney = self->client->pers.oldmoney;

	//unlagged - backward reconciliation #2
	// make sure the body shows up in the client's current position
	G_UnTimeShiftClient( self );
	//unlagged - backward reconciliation #2

	// check for an almost capture
	CheckAlmostCapture( self, attacker );
	// check for a player that almost brought in cubes
	CheckAlmostScored( self, attacker );

	if (self->client && self->client->hook) {
		Weapon_HookFree(self->client->hook);
	}
	if ((self->client->ps.eFlags & EF_TICKING) && self->activator) {
		self->client->ps.eFlags &= ~EF_TICKING;
		self->activator->think = G_FreeEntity;
		self->activator->nextthink = level.time;
	}
	self->client->ps.pm_type = PM_DEAD;

	if ( attacker ) {
		killer = attacker->s.number;
		if ( attacker->client ) {
			killerName = attacker->client->pers.netname;
		} else {
			killerName = "<non-client>";
		}
	} else {
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if ( killer < 0 || killer >= MAX_CLIENTS ) {
		killer = ENTITYNUM_WORLD;
		killerName = "<world>";
	}

	if ( meansOfDeath < 0 || meansOfDeath >= sizeof( modNames ) / sizeof( modNames[0] ) ) {
		obit = "<bad obituary>";
	} else {
		obit = modNames[meansOfDeath];
	}

	//G_Printf("Kill: %i %i %i: %s killed %s by %s\n", killer, self->s.number, meansOfDeath, killerName, self->client->pers.netname, obit );

	// broadcast the death event to everyone
	ent = G_TempEntity( self->r.currentOrigin, EV_OBITUARY );
	ent->s.eventParm = meansOfDeath;
	ent->s.otherEntityNum = self->s.number;
	ent->s.otherEntityNum2 = killer;
        //Sago: Hmmm... generic? Can I transmit anything I like? Like if it is a team kill? Let's try
        ent->s.generic1 = OnSameTeam (self, attacker);
        if( !((g_gametype.integer==GT_ELIMINATION || g_gametype.integer==GT_CTF_ELIMINATION) && level.time < level.roundStartTime) )
            ent->r.svFlags = SVF_BROADCAST;	// send to everyone (if not an elimination gametype during active warmup)
        else
            ent->r.svFlags = SVF_NOCLIENT;

	self->enemy = attacker;

	self->client->ps.persistant[PERS_KILLED]++;

	if (attacker) {
		attacker->client->lastkilled_client = self->s.number;

		if ( attacker == self || OnSameTeam (self, attacker ) ) {
			if(g_gametype.integer!=GT_LMS && !((g_gametype.integer==GT_ELIMINATION || g_gametype.integer==GT_CTF_ELIMINATION) && level.time < level.roundStartTime))
                            if( (g_gametype.integer <GT_TEAM && g_ffa_gt!=1 && self->client->ps.persistant[PERS_SCORE]>0) || level.numNonSpectatorClients<3) //Cannot get negative scores by suicide
                                AddScore( attacker, self->r.currentOrigin, -1 );
								G_LogPrintf( "Score: Non LMS Elim 1!\n" );
		} else {
			if(g_gametype.integer != GT_LMS){
				AddScore( attacker, self->r.currentOrigin, 1 );
				attacker->client->pers.oldmoney += 1;
				G_LogPrintf( "Score: Non LMS 1!\n" );
			}

			if( meansOfDeath == MOD_GAUNTLET ) {

				attacker->client->pers.oldmoney += 1;
				
				// play humiliation on player
				attacker->client->ps.persistant[PERS_GAUNTLET_FRAG_COUNT]++;

				// add the sprite over the player's head
				attacker->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
				attacker->client->ps.eFlags |= EF_AWARD_GAUNTLET;
				attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;

				// also play humiliation on target
				self->client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_GAUNTLETREWARD;
			}

			// check for two kills in a short amount of time
			// if this is close enough to the last kill, give a reward sound
			if ( level.time - attacker->client->lastKillTime < CARNAGE_REWARD_TIME ) {
				attacker->client->ps.persistant[PERS_EXCELLENT_COUNT]++;
				attacker->client->pers.oldmoney += 1;
				// add the sprite over the player's head
				attacker->client->ps.eFlags &= ~(EF_AWARD_IMPRESSIVE | EF_AWARD_EXCELLENT | EF_AWARD_GAUNTLET | EF_AWARD_ASSIST | EF_AWARD_DEFEND | EF_AWARD_CAP );
				attacker->client->ps.eFlags |= EF_AWARD_EXCELLENT;
				attacker->client->rewardTime = level.time + REWARD_SPRITE_TIME;
			}
			attacker->client->lastKillTime = level.time;
		}
	} else {
		if(g_gametype.integer!=GT_LMS && !((g_gametype.integer==GT_ELIMINATION || g_gametype.integer==GT_CTF_ELIMINATION) && level.time < level.roundStartTime))
                    if(self->client->ps.persistant[PERS_SCORE]>0 || level.numNonSpectatorClients<3) //Cannot get negative scores by suicide
			AddScore( self, self->r.currentOrigin, -1 );
	}

	// Add team bonuses
	Team_FragBonuses(self, inflictor, attacker);

	// if I committed suicide, the flag does not fall, it returns.
	if (meansOfDeath == MOD_SUICIDE) {
		if ( self->client->ps.powerups[PW_NEUTRALFLAG] ) {		// only happens in One Flag CTF
			Team_ReturnFlag( TEAM_FREE );
			self->client->ps.powerups[PW_NEUTRALFLAG] = 0;
		}
		else if ( self->client->ps.powerups[PW_REDFLAG] ) {		// only happens in standard CTF
			Team_ReturnFlag( TEAM_RED );
			self->client->ps.powerups[PW_REDFLAG] = 0;
		}
		else if ( self->client->ps.powerups[PW_BLUEFLAG] ) {	// only happens in standard CTF
			Team_ReturnFlag( TEAM_BLUE );
			self->client->ps.powerups[PW_BLUEFLAG] = 0;
		}
	}
        TossClientPersistantPowerups( self );
        if( g_gametype.integer == GT_HARVESTER ) {
                TossClientCubes( self );
        }
	// if client is in a nodrop area, don't drop anything (but return CTF flags!)
	TossClientItems( self );


	Cmd_Score_f( self );		// show scores
	// send updated scores to any clients that are following this one,
	// or they would get stale scoreboards
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		gclient_t	*client;

		client = &level.clients[i];
		if ( client->pers.connected != CON_CONNECTED ) {
			continue;
		}
		if ( client->sess.sessionTeam != TEAM_SPECTATOR ) {
			continue;
		}
		if ( client->sess.spectatorClient == self->s.number ) {
			Cmd_Score_f( g_entities + i );
		}
	}

	self->takedamage = qtrue;	// can still be gibbed

	self->s.weapon = WP_NONE;
	self->s.powerups = 0;
	self->r.contents = CONTENTS_CORPSE;

	self->s.angles[0] = 0;
	self->s.angles[2] = 0;
	LookAtKiller (self, inflictor, attacker);

	VectorCopy( self->s.angles, self->client->ps.viewangles );

	self->s.loopSound = 0;

	self->r.maxs[2] = -8;

	// don't allow respawn until the death anim is done
	// g_forcerespawn may force spawning at some later time
	self->client->respawnTime = level.time + g_respawnwait.integer +i;
		if(self->client->sess.sessionTeam == TEAM_BLUE){
		self->client->respawnTime = level.time + g_teamblue_respawnwait.integer +i;
		}
		if(self->client->sess.sessionTeam == TEAM_RED){
		self->client->respawnTime = level.time + g_teamred_respawnwait.integer +i;
		}
        if(g_respawntime.integer>0) {
            for(i=0; self->client->respawnTime > i*g_respawntime.integer*1000;i++);

            self->client->respawnTime = i*g_respawntime.integer*1000;
if(ent->singlebot){
	self->client->respawnTime = level.time + 5000;	//keep bot bodies around slightly longer
}
        }
        //For testing:
        //G_Printf("Respawntime: %i\n",self->client->respawnTime);
	//However during warm up, we should respawn quicker!
	if(g_gametype.integer == GT_ELIMINATION || g_gametype.integer == GT_CTF_ELIMINATION || g_gametype.integer == GT_LMS)
		if(level.time<=level.roundStartTime && level.time>level.roundStartTime-1000*g_elimination_activewarmup.integer)
			self->client->respawnTime = level.time + rand()%800;

        RespawnTimeMessage(self, self->client->respawnTime);


	// remove powerups
	memset( self->client->ps.powerups, 0, sizeof(self->client->ps.powerups) );

	// never gib in a nodrop
	contents = trap_PointContents( self->r.currentOrigin, -1 );

	if ( (self->health <= GIB_HEALTH && !(contents & CONTENTS_NODROP) && g_blood.integer) || meansOfDeath == MOD_SUICIDE) {
		// gib death
		GibEntity( self, killer );
	} else {
		// normal death
		static int i;

		switch ( i ) {
		case 0:
			anim = BOTH_DEATH1;
			break;
		case 1:
			anim = BOTH_DEATH2;
			break;
		case 2:
		default:
			anim = BOTH_DEATH3;
			break;
		}

		// for the no-blood option, we need to prevent the health
		// from going to gib level
		if ( self->health <= GIB_HEALTH ) {
			self->health = GIB_HEALTH+1;
		}

		self->client->ps.legsAnim =
			( ( self->client->ps.legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;
		self->client->ps.torsoAnim =
			( ( self->client->ps.torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;

		G_AddEvent( self, EV_DEATH1 + i, killer );

		// the body can still be gibbed
		self->die = body_die;

		// globally cycle through the different death animations
		i = ( i + 1 ) % 3;

		if (self->s.eFlags & EF_KAMIKAZE) {
			Kamikaze_DeathTimer( self );
		}
	}

	trap_LinkEntity (self);

	// Fire trigger_death and trigger_frag target entities and the deathtarget for the related target_botspawn 
	G_UseTriggerFragAndDeathEntities ( self, attacker );
	
	// Trigger deathtarget and drop loot
	if ( self->botspawn ) {
		G_UseDeathTargets(self->botspawn, self);
		G_DropLoot(self->botspawn, self);
	}

	if(g_gametype.integer == GT_SINGLE){
	if ( !IsBot( self ) )
		G_FadeOut( 1.0, self-g_entities );
	}
}


/*
================
CheckArmor
================
*/
int CheckArmor (gentity_t *ent, int damage, int dflags)
{
	gclient_t	*client;
	int			save;
	int			count;

	if (!damage)
		return 0;

	client = ent->client;

	if (!client)
		return 0;

	if (dflags & DAMAGE_NO_ARMOR)
		return 0;

	// armor
	count = client->ps.stats[STAT_ARMOR];
	save = ceil( damage * g_armorprotect.value );
	if (save >= count)
		save = count;

	if (!save)
		return 0;

	client->ps.stats[STAT_ARMOR] -= save;

	return save;
}

/*
================
RaySphereIntersections
================
*/
int RaySphereIntersections( vec3_t origin, float radius, vec3_t point, vec3_t dir, vec3_t intersections[2] ) {
	float b, c, d, t;

	//	| origin - (point + t * dir) | = radius
	//	a = dir[0]^2 + dir[1]^2 + dir[2]^2;
	//	b = 2 * (dir[0] * (point[0] - origin[0]) + dir[1] * (point[1] - origin[1]) + dir[2] * (point[2] - origin[2]));
	//	c = (point[0] - origin[0])^2 + (point[1] - origin[1])^2 + (point[2] - origin[2])^2 - radius^2;

	// normalize dir so a = 1
	VectorNormalize(dir);
	b = 2 * (dir[0] * (point[0] - origin[0]) + dir[1] * (point[1] - origin[1]) + dir[2] * (point[2] - origin[2]));
	c = (point[0] - origin[0]) * (point[0] - origin[0]) +
		(point[1] - origin[1]) * (point[1] - origin[1]) +
		(point[2] - origin[2]) * (point[2] - origin[2]) -
		radius * radius;

	d = b * b - 4 * c;
	if (d > 0) {
		t = (- b + sqrt(d)) / 2;
		VectorMA(point, t, dir, intersections[0]);
		t = (- b - sqrt(d)) / 2;
		VectorMA(point, t, dir, intersections[1]);
		return 2;
	}
	else if (d == 0) {
		t = (- b ) / 2;
		VectorMA(point, t, dir, intersections[0]);
		return 1;
	}
	return 0;
}

/*
================
G_InvulnerabilityEffect
================
*/
int G_InvulnerabilityEffect( gentity_t *targ, vec3_t dir, vec3_t point, vec3_t impactpoint, vec3_t bouncedir ) {
	gentity_t	*impact;
	vec3_t		intersections[2], vec;
	int			n;

	if ( !targ->client ) {
		return qfalse;
	}
	VectorCopy(dir, vec);
	VectorInverse(vec);
	// sphere model radius = 42 units
	n = RaySphereIntersections( targ->client->ps.origin, 42, point, vec, intersections);
	if (n > 0) {
		impact = G_TempEntity( targ->client->ps.origin, EV_INVUL_IMPACT );
		VectorSubtract(intersections[0], targ->client->ps.origin, vec);
		vectoangles(vec, impact->s.angles);
		impact->s.angles[0] += 90;
		if (impact->s.angles[0] > 360)
			impact->s.angles[0] -= 360;
		if ( impactpoint ) {
			VectorCopy( intersections[0], impactpoint );
		}
		if ( bouncedir ) {
			VectorCopy( vec, bouncedir );
			VectorNormalize( bouncedir );
		}
		return qtrue;
	}
	else {
		return qfalse;
	}
}

/*
catchup_damage
*/
static int catchup_damage(int damage, int attacker_points, int target_points) {
    int newdamage;
    if(g_catchup.integer <= 0 )
        return damage;
    //Reduce damage
    if(attacker_points<=target_points+5)
        return damage; //Never reduce damage if only 5 points ahead.

    newdamage=damage-((attacker_points-target_points-5) * (g_catchup.integer*damage))/100;
    if(newdamage<damage/2)
        return damage/2;
    return newdamage;
}

/*
============
T_Damage

targ		entity that is being damaged
inflictor	entity that is causing the damage
attacker	entity that caused the inflictor to damage targ
	example: targ=monster, inflictor=rocket, attacker=player

dir			direction of the attack for knockback
point		point at which the damage is being inflicted, used for headshots
damage		amount of damage being inflicted
knockback	force to be applied against targ as a result of the damage

inflictor, attacker, dir, and point can be NULL for environmental effects

dflags		these flags are used to control how T_Damage works
	DAMAGE_RADIUS			damage was indirect (from a nearby explosion)
	DAMAGE_NO_ARMOR			armor does not protect from this damage
	DAMAGE_NO_KNOCKBACK		do not affect velocity, just view angles
	DAMAGE_NO_PROTECTION	kills godmode, armor, everything
============
*/

double angle45hook(double value, double src_min, double src_max) {
    return -(90.0 * (value - src_min) / (src_max - src_min));
}

int engine10hook(int value, int src_min, int src_max) {
    return 10 * (value - src_min) / (src_max - src_min);
}

void VehiclePhys( gentity_t *self ) {
	
	if(!self->parent || !self || !self->parent->client->vehiclenum || self->parent->health <= 0 || self->health <= 0){
	self->think = 0;
	self->nextthink = 0;
	self->r.contents = CONTENTS_SOLID;
	self->sb_coll = 0;
	self->s.pos.trType = TR_GRAVITY;
	self->s.pos.trTime = level.time;
	self->physicsObject = qtrue;
	ClientUserinfoChanged( self->parent->s.clientNum );
	VectorSet( self->parent->r.mins, -15, -15, -24 );
	VectorSet( self->parent->r.maxs, 15, 15, 32 );
	VectorSet( self->parent->client->ps.origin, self->r.currentOrigin[0], self->r.currentOrigin[1], self->r.currentOrigin[2] + 40);
	self->parent->client->vehiclenum = 0;
	self->s.legsAnim = 0;
	self->s.generic1 = 0; 		//smooth vehicles
	self->parent->client->ps.gravity = (g_gravity.value*g_gravityModifier.value);
	return;
	}
	
	self->s.pos.trType = TR_STATIONARY;
	self->physicsObject = qfalse;
	self->sb_phys = 1;
	
	self->r.contents = CONTENTS_TRIGGER;
	self->sb_coll = 1;

	trap_UnlinkEntity( self );
	
	VectorCopy(self->parent->s.origin, self->s.origin);
	VectorCopy(self->parent->s.pos.trBase, self->s.pos.trBase);
	if (VectorLength(self->parent->client->ps.velocity) > 5) {
	self->s.apos.trBase[1] = self->parent->s.apos.trBase[1];
	}
	/*if(BG_VehicleCheckClass(self->parent->client->ps.stats[STAT_VEHICLE]) == VCLASS_CAR){ //VEHICLE-SYSTEM: turn vehicle fake phys
	self->s.apos.trBase[0] = angle45hook(self->parent->client->ps.velocity[2], 0, 900); //900 is car speed
	}*/
	if(engine10hook(sqrt(self->parent->client->ps.velocity[0] * self->parent->client->ps.velocity[0] + self->parent->client->ps.velocity[1] * self->parent->client->ps.velocity[1]), 0, 900) <= 10){ //900 is car speed
	self->s.legsAnim = engine10hook(sqrt(self->parent->client->ps.velocity[0] * self->parent->client->ps.velocity[0] + self->parent->client->ps.velocity[1] * self->parent->client->ps.velocity[1]), 0, 900); //900 is car speed
	}
	VectorCopy(self->parent->r.currentOrigin, self->r.currentOrigin);
	self->parent->client->ps.stats[STAT_VEHICLEHP] = self->health; //VEHICLE-SYSTEM: vehicle's hp instead player
	self->s.generic1 = self->parent->s.clientNum+1; 		//smooth vehicles
	
	trap_LinkEntity( self );
	
	self->think = VehiclePhys;
	self->nextthink = level.time + 1;
	
}

void G_Damage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
			   vec3_t dir, vec3_t point, int damage, int dflags, int mod ) {
	gclient_t	*client;
	int			take;
	int			asave = 0;
	int			knockback;
	int			max;
	int			i;
	vec3_t		bouncedir, impactpoint;
	gentity_t 	*act;
	gentity_t 	*vehicle;

	//in entityplus bots cannot harm other bots (unless it's a telefrag)
	if ( !G_NpcFactionProp(NP_HARM, attacker) && attacker->singlebot >= 1 && targ->singlebot == attacker->singlebot && attacker && mod != MOD_TELEFRAG )
		return;

	if(g_gametype.integer == GT_SANDBOX){
		if(mod == MOD_TOOLGUN){
			return;
		}
	}
	
	if(mod == MOD_GAUNTLET){
		if(targ->vehicle && !attacker->client->vehiclenum){
		attacker->client->vehiclenum = targ->s.number;
		targ->parent = attacker;
		ClientUserinfoChanged( attacker->s.clientNum );
		VectorCopy(targ->s.origin, attacker->s.origin);
		VectorCopy(targ->s.pos.trBase, attacker->s.pos.trBase);
		attacker->s.apos.trBase[1] = targ->s.apos.trBase[1];
		VectorCopy(targ->r.currentOrigin, attacker->r.currentOrigin);
		VectorSet( attacker->r.mins, -25, -25, -15 );
		VectorSet( attacker->r.maxs, 25, 25, 15 );
		targ->think = VehiclePhys;
		targ->nextthink = level.time + 1;
		return;
		}
		if(attacker->client->vehiclenum){
		return;	
		}
	}

	if(mod == MOD_REGENERATOR && targ->client){
		if (!targ->client->ps.powerups[PW_REGEN]) {
			targ->client->ps.powerups[PW_REGEN] = level.time - ( level.time % 1000 );
		}
		targ->client->ps.powerups[PW_REGEN] += 5 * 1000;
	}
	
	if (targ->damagetarget){
	act = G_PickTarget( targ->damagetarget );
	if ( act && act->use ) {
		act->use( act, attacker, attacker );
	}
	}

	if (!targ->takedamage) {
		return;
	}
	
	if (targ->client && targ->client->vehiclenum){ //VEHICLE-SYSTEM: damage vehicle instead player
 		targ = G_FindEntityForEntityNum(targ->client->vehiclenum);
	}

	// the intermission has allready been qualified for, so don't
	// allow any extra scoring
	if ( level.intermissionQueued ) {
		return;
	}

	if ( targ->client && mod != MOD_JUICED) {
		if ( targ->client->invulnerabilityTime > level.time) {
			if ( dir && point ) {
				G_InvulnerabilityEffect( targ, dir, point, impactpoint, bouncedir );
			}
			return;
		}
	}

    //Sago: This was moved up
    client = targ->client;
    //Sago: See if the client was sent flying
    //Check if damage is by somebody who is not a player!
    if( (!attacker || (attacker->s.eType != ET_PLAYER && attacker->s.eType != ET_GENERAL)) && client && client->lastSentFlying>-1 && ( mod==MOD_FALLING || mod==MOD_LAVA || mod==MOD_SLIME || mod==MOD_TRIGGER_HURT || mod==MOD_SUICIDE) )  {
        if( client->lastSentFlyingTime+5000<level.time) {
            client->lastSentFlying = -1; //More than 5 seconds, not a kill!
        } else {
            //G_Printf("LastSentFlying %i\n",client->lastSentFlying);
            attacker = &g_entities[client->lastSentFlying];
        }
    }

	if ( !inflictor ) {
		inflictor = &g_entities[ENTITYNUM_WORLD];
	}
	if ( !attacker ) {
		attacker = &g_entities[ENTITYNUM_WORLD];
	}

	// shootable doors / buttons don't actually have any health
	if ( targ->s.eType == ET_MOVER) {
		if (strcmp(targ->classname, "func_breakable") && targ->use && (targ->moverState == MOVER_POS1 || targ->moverState == ROTATOR_POS1)) {
			targ->use(targ, inflictor, attacker);
		}
		if (!strcmp(targ->classname, "func_train") && targ->health > 0) {
			G_UseTargets(targ, attacker);
		} else {	// entity is a func_breakable
			if ( (targ->spawnflags & 1024) && attacker == attacker )
			return;
			if ( (targ->spawnflags & 2048) && IsBot(attacker) )
				return;
			if ( (targ->spawnflags & 4096) && strstr(attacker->classname, "shooter_") )
				return;
			
		}
	}
	if( g_gametype.integer == GT_OBELISK && CheckObeliskAttack( targ, attacker ) ) {
		return;
	}

	if ( attacker && attacker->singlebot){
			float skill = trap_Cvar_VariableValue( "g_spSkill" );
			int orgdmg = damage;
	
			if ( attacker->botspawn && attacker->botspawn->skill )
				skill += attacker->botspawn->skill;
			
			if (skill < 1)
				skill = 1;	//relative skill level should not drop below 1 but is allowed to rise above 5
			
			damage *= ( ( 0.1 * skill  ) - 0.05 ); //damage is always rounded down.
			
			if ( damage < 1 )
				damage = 1;	//make sure bot does at least -some- damage
	}

	if ( !dir ) {
		dflags |= DAMAGE_NO_KNOCKBACK;
	} else {
		VectorNormalize(dir);
	}

	knockback = damage+1;

	if ( mod == MOD_CAR )
	knockback *= 1.00;
	if ( mod == MOD_PROP )
	knockback *= 1.00;
	if ( mod == MOD_GAUNTLET )
	knockback *= g_gknockback.value;
	if ( mod == MOD_MACHINEGUN )
	knockback *= g_mgknockback.value;
	if ( mod == MOD_SHOTGUN )
	knockback *= g_sgknockback.value;
	if ( mod == MOD_GRENADE )
	knockback *= g_glknockback.value;
	if ( mod == MOD_GRENADE_SPLASH )
	knockback *= g_glknockback.value;
	if ( mod == MOD_ROCKET )
	knockback *= g_rlknockback.value;
	if ( mod == MOD_ROCKET_SPLASH )
	knockback *= g_rlknockback.value;
	if ( mod == MOD_PLASMA )
	knockback *= g_pgknockback.value;
	if ( mod == MOD_PLASMA_SPLASH )
	knockback *= g_pgknockback.value;
	if ( mod == MOD_RAILGUN )
	knockback *= g_rgknockback.value;
	if ( mod == MOD_LIGHTNING )
	knockback *= g_lgknockback.value;
	if ( mod == MOD_BFG )
	knockback *= g_bfgknockback.value;
	if ( mod == MOD_BFG_SPLASH )
	knockback *= g_bfgknockback.value;
	if ( mod == MOD_NAIL )
	knockback *= g_ngknockback.value;
	if ( mod == MOD_CHAINGUN )
	knockback *= g_cgknockback.value;
	if ( mod == MOD_PROXIMITY_MINE )
	knockback *= g_plknockback.value;
	if ( mod == MOD_FLAME )
	knockback *= g_ftknockback.value;
	if ( mod == MOD_FLAME_SPLASH )
	knockback *= g_ftknockback.value;
	if ( mod == MOD_ANTIMATTER )
	knockback *= g_amknockback.value;
	if ( mod == MOD_ANTIMATTER_SPLASH )
	knockback *= g_amknockback.value;
	if ( mod == MOD_TARGET_LASER )
	knockback = attacker->msdamage;
	if ( mod == MOD_KNOCKER )
	knockback *= 200;

	if ( targ->flags & FL_NO_KNOCKBACK ) {
		knockback = 0;
	}
	if ( dflags & DAMAGE_NO_KNOCKBACK ) {
		knockback = 0;
	}

	// figure momentum add, even if the damage won't be taken
	if ( knockback && targ->client || knockback && targ->sandboxObject ) {
		vec3_t	kvel;
		float	mass;

		mass = 200;

		if(targ->sandboxObject){
			VectorScale (dir, g_knockback.value*2 * (float)knockback / mass, kvel);
		} else {
			VectorScale (dir, g_knockback.value * (float)knockback / mass, kvel);
		}
		if(targ->client){
		VectorAdd (targ->client->ps.velocity, kvel, targ->client->ps.velocity);
		}
		if(targ->sandboxObject){
		G_EnablePropPhysics( targ );
		targ->lastPlayer = attacker;
		VectorAdd (targ->s.pos.trDelta, kvel, targ->s.pos.trDelta);
		}

		// set the timer so that the other client can't cancel
		// out the movement immediately
		if(!targ->sandboxObject){
			if ( !targ->client->ps.pm_time ) {
				int		t;

				t = knockback * 2;
				if ( t < 50 ) {
					t = 50;
				}
				if ( t > 200 ) {
					t = 200;
				}
				targ->client->ps.pm_time = t;
				targ->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;
			}
            //Remeber the last person to hurt the player
            if( !g_awardpushing.integer || targ==attacker || OnSameTeam (targ, attacker)) {
                targ->client->lastSentFlying = -1;
            } else {
                targ->client->lastSentFlying = attacker->s.number;
                targ->client->lastSentFlyingTime = level.time;
            }
		}
	}
	
	if (targ->sandboxObject) {
	if (!targ->takedamage2) {
		return;
	}
	}

	// check for completely getting out of the damage
	if ( !(dflags & DAMAGE_NO_PROTECTION) ) {

		// if TF_NO_FRIENDLY_FIRE is set, don't do damage to the target
		// if the attacker was on the same team
		if ( mod != MOD_JUICED && mod != MOD_CRUSH && targ != attacker && !(dflags & DAMAGE_NO_TEAM_PROTECTION) && OnSameTeam (targ, attacker)  ) {
			if ( ( !g_friendlyFire.integer && g_gametype.integer != GT_ELIMINATION && g_gametype.integer != GT_CTF_ELIMINATION ) || ( g_elimination_selfdamage.integer<2 &&	(g_gametype.integer == GT_ELIMINATION || g_gametype.integer == GT_CTF_ELIMINATION) ) ) {
				return;
			}
		}
		if (mod == MOD_PROXIMITY_MINE) {
			if (inflictor && inflictor->parent && OnSameTeam(targ, inflictor->parent)) {
				return;
			}
			if (targ == attacker) {
				return;
			}
		}

		// check for godmode
		if ( targ->flags & FL_GODMODE ) {
			return;
		}

                if(targ->client && targ->client->spawnprotected) {
                   if(level.time>targ->client->respawnTime+g_spawnprotect.integer)
                       targ->client->spawnprotected = qfalse;
                   else
                       if( (mod > MOD_UNKNOWN && mod < MOD_WATER) || mod == MOD_TELEFRAG || mod>MOD_TRIGGER_HURT)
                       return;
                }
	}

	// battlesuit protects from all radius damage (but takes knockback)
	// and protects 50% against all damage
	if ( client && client->ps.powerups[PW_BATTLESUIT] ) {
		G_AddEvent( targ, EV_POWERUP_BATTLESUIT, 0 );
		if ( ( dflags & DAMAGE_RADIUS ) || ( mod == MOD_FALLING ) ) {
			return;
		}
		damage *= 0.5;
	}

	// add to the attacker's hit counter (if the target isn't a general entity like a prox mine)
	if ( attacker->client && client && targ != attacker && targ->health > 0 && targ->s.eType != ET_MISSILE && targ->s.eType != ET_GENERAL) {
		if ( OnSameTeam( targ, attacker ) ) {
			attacker->client->ps.persistant[PERS_HITS]--;
		} else {
			attacker->client->ps.persistant[PERS_HITS]++;
		}
		attacker->client->ps.persistant[PERS_ATTACKEE_ARMOR] = (targ->health<<8)|(client->ps.stats[STAT_ARMOR]);
	}

	// always give half damage if hurting self
	// calculated after knockback, so rocket jumping works
	if ( targ == attacker) {
		damage *= 0.20;
	}

        if(targ && targ->client && attacker->client )
            damage = catchup_damage(damage, attacker->client->ps.persistant[PERS_SCORE], targ->client->ps.persistant[PERS_SCORE]);

        if(g_damageModifier.value > 0.01) {
            damage *= g_damageModifier.value;
        }

	if ((g_gametype.integer == GT_ELIMINATION || g_gametype.integer == GT_CTF_ELIMINATION || g_gametype.integer == GT_LMS || g_elimination_allgametypes.integer)
				&& g_elimination_selfdamage.integer<1 && ( targ == attacker ||  mod == MOD_FALLING )) {
		damage = 0;
	}

	//So people can be telefragged!
	if ((g_gametype.integer == GT_ELIMINATION || g_gametype.integer == GT_CTF_ELIMINATION || g_gametype.integer == GT_LMS) && level.time < level.roundStartTime && ((mod == MOD_LAVA) || (mod == MOD_SLIME)) ) {
		damage = 1000;
	}

	take = damage;

	// save some from armor
	asave = CheckArmor (targ, take, dflags);
	take -= asave;

	if ( g_debugDamage.integer ) {
		G_Printf( "%i: client:%i health:%i damage:%i armor:%i\n", level.time, targ->s.number,
			targ->health, take, asave );
	}
	
	if ( client ) {
		if ( attacker ) {
			client->ps.persistant[PERS_ATTACKER] = attacker->s.number;
                } else if(client->lastSentFlying) {
                        client->ps.persistant[PERS_ATTACKER] = client->lastSentFlying;
                } else {
			client->ps.persistant[PERS_ATTACKER] = ENTITYNUM_WORLD;
		}
		client->damage_armor += asave;
		client->damage_blood += take;
		client->damage_knockback += knockback;
		if ( dir ) {
			VectorCopy ( dir, client->damage_from );
			client->damage_fromWorld = qfalse;
		} else {
			VectorCopy ( targ->r.currentOrigin, client->damage_from );
			client->damage_fromWorld = qtrue;
		}
	}

	// See if it's the player hurting the emeny flag carrier
	if( g_gametype.integer == GT_CTF || g_gametype.integer == GT_1FCTF || g_gametype.integer == GT_CTF_ELIMINATION) {
		Team_CheckHurtCarrier(targ, attacker);
	}

	if (targ->client) {
		// set the last client who damaged the target
		targ->client->lasthurt_client = attacker->s.number;
		targ->client->lasthurt_mod = mod;
	}

	//If vampire is enabled, gain health but not from self or teammate, cannot steal more than targ has
	if( g_vampire.value>0.0 && (targ != attacker) && take > 0 &&
                !(OnSameTeam(targ, attacker)) && attacker->health > 0 && targ->health > 0 )
	{
		if(take<targ->health)
			attacker->health += (int)(((float)take)*g_vampire.value);
		else
			attacker->health += (int)(((float)targ->health)*g_vampire.value);
		if(attacker->health>g_vampireMaxHealth.integer)
			attacker->health = g_vampireMaxHealth.integer;
	}

	if(mod==MOD_MACHINEGUN){
		if(g_mgvampire.integer==1){
			if(attacker->health<mod_vampire_max_health){
				attacker->health += take;
				if(attacker->health>mod_vampire_max_health){
					attacker->health = mod_vampire_max_health;
				}
			}
		}
	}
	if(mod==MOD_SHOTGUN){
		if(g_sgvampire.integer==1){
			if(attacker->health<mod_vampire_max_health){
				attacker->health += take;
				if(attacker->health>mod_vampire_max_health){
					attacker->health = mod_vampire_max_health;
				}
			}
		}
	}
	if(mod==MOD_RAILGUN){
		if(g_rgvampire.integer==1){
			if(attacker->health<mod_vampire_max_health){
				attacker->health += take;
				if(attacker->health>mod_vampire_max_health){
					attacker->health = mod_vampire_max_health;
				}
			}
		}
	}
	if(mod==MOD_LIGHTNING){
		if(g_lgvampire.integer==1){
			if(attacker->health<mod_vampire_max_health){
				attacker->health += take;
				if(attacker->health>mod_vampire_max_health){
					attacker->health = mod_vampire_max_health;
				}
			}
		}
	}
	if(mod==MOD_NAIL){
		if(g_ngvampire.integer==1){
			if(attacker->health<mod_vampire_max_health){
				attacker->health += take;
				if(attacker->health>mod_vampire_max_health){
					attacker->health = mod_vampire_max_health;
				}
			}
		}
	}
	if(mod==MOD_CHAINGUN){
		if(g_cgvampire.integer==1){
			if(attacker->health<mod_vampire_max_health){
				attacker->health += take;
				if(attacker->health>mod_vampire_max_health){
					attacker->health = mod_vampire_max_health;
				}
			}
		}
	}
	if(mod==MOD_PROXIMITY_MINE){
		if(g_plvampire.integer==1){
			if(attacker->health<mod_vampire_max_health){
				attacker->health += take;
				if(attacker->health>mod_vampire_max_health){
					attacker->health = mod_vampire_max_health;
				}
			}
		}
	}
	if((mod==MOD_GRENADE)||(mod==MOD_GRENADE_SPLASH)){
		if(g_glvampire.integer==1){
			if(attacker->health<mod_vampire_max_health){
				attacker->health += take;
				if(attacker->health>mod_vampire_max_health){
					attacker->health = mod_vampire_max_health;
				}
			}
		}
	}
	if((mod==MOD_ROCKET)||(mod==MOD_ROCKET_SPLASH)){
		if(g_rlvampire.integer==1){
			if(attacker->health<mod_vampire_max_health){
				attacker->health += take;
				if(attacker->health>mod_vampire_max_health){
					attacker->health = mod_vampire_max_health;
				}
			}
		}
	}
	if((mod==MOD_PLASMA)||(mod==MOD_PLASMA_SPLASH)){
		if(g_pgvampire.integer==1){
			if(attacker->health<mod_vampire_max_health){
				attacker->health += take;
				if(attacker->health>mod_vampire_max_health){
					attacker->health = mod_vampire_max_health;
				}
			}
		}
	}
	if((mod==MOD_BFG)||(mod==MOD_BFG_SPLASH)){
		if(g_bfgvampire.integer==1){
			if(attacker->health<mod_vampire_max_health){
				attacker->health += take;
				if(attacker->health>mod_vampire_max_health){
					attacker->health = mod_vampire_max_health;
				}
			}
		}
	}
	if((mod==MOD_FLAME)||(mod==MOD_FLAME_SPLASH)){
		if(g_ftvampire.integer==1){
			if(attacker->health<mod_vampire_max_health){
				attacker->health += take;
				if(attacker->health>mod_vampire_max_health){
					attacker->health = mod_vampire_max_health;
				}
			}
		}
	}
	if((mod==MOD_ANTIMATTER)||(mod==MOD_ANTIMATTER_SPLASH)){
		if(g_amvampire.integer==1){
			if(attacker->health<mod_vampire_max_health){
				attacker->health += take;
				if(attacker->health>mod_vampire_max_health){
					attacker->health = mod_vampire_max_health;
				}
			}
		}
	}
	
	// do the damage
	if (take) {
		targ->health = targ->health - take;
		if ( targ->client ) {
			targ->client->ps.stats[STAT_HEALTH] = targ->health;
		}
		
		if ( !strcmp(targ->classname, "func_breakable") ) {
			targ->health -= damage;
			if ( targ->health <= 0 ){
			    Break_Breakable(targ, attacker);
			}
		}
                ScorePlum(attacker, targ->r.currentOrigin, damage, asave);

		if ( targ->health <= 0 ) {
			if ( client )
				targ->flags |= FL_NO_KNOCKBACK;

			if (targ->health < -999)
				targ->health = -999;

			targ->enemy = attacker;
			targ->die (targ, inflictor, attacker, take, mod);
			return;
		} else if ( targ->pain ) {
			targ->pain (targ, attacker, take);
		}
	}
}

void G_PropDamage (gentity_t *targ, gentity_t *attacker, int damage){
	G_Damage( targ, attacker, attacker, NULL, NULL, damage, 0, MOD_PROP );
}

void G_CarDamage (gentity_t *targ, gentity_t *attacker, int damage){
	G_Damage( targ, attacker, attacker, NULL, NULL, damage, 0, MOD_CAR );
}

void G_ExitVehicle (int num){
	Cmd_VehicleExit_f( G_FindEntityForClientNum(num));
}


/*
============
CanDamage

Returns qtrue if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
qboolean CanDamage (gentity_t *targ, vec3_t origin) {
	vec3_t	dest;
	trace_t	tr;
	vec3_t	midpoint;

	// use the midpoint of the bounds instead of the origin, because
	// bmodels may have their origin is 0,0,0
	VectorAdd (targ->r.absmin, targ->r.absmax, midpoint);
	VectorScale (midpoint, 0.5, midpoint);

	VectorCopy (midpoint, dest);
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0 || tr.entityNum == targ->s.number)
		return qtrue;

	// this should probably check in the plane of projection,
	// rather than in world coordinate, and also include Z
	VectorCopy (midpoint, dest);
	dest[0] += 15.0;
	dest[1] += 15.0;
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy (midpoint, dest);
	dest[0] += 15.0;
	dest[1] -= 15.0;
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy (midpoint, dest);
	dest[0] -= 15.0;
	dest[1] += 15.0;
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy (midpoint, dest);
	dest[0] -= 15.0;
	dest[1] -= 15.0;
	trap_Trace ( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);
	if (tr.fraction == 1.0)
		return qtrue;


	return qfalse;
}


/*
============
G_RadiusDamage
============
*/
qboolean G_RadiusDamage ( vec3_t origin, gentity_t *attacker, float damage, float radius,
					 gentity_t *ignore, int mod) {
	float		points, dist;
	gentity_t	*ent;
	int			numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		v;
	vec3_t		dir;
	int			i, e;
	qboolean	hitClient = qfalse;

	if ( radius < 1 ) {
		radius = 1;
	}

	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = origin[i] - radius;
		maxs[i] = origin[i] + radius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, SourceTechEntityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		ent = &g_entities[SourceTechEntityList[ e ]];

		if (ent == ignore)
			continue;
		if (!ent->takedamage)
			continue;

		// find the distance from the edge of the bounding box
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( origin[i] < ent->r.absmin[i] ) {
				v[i] = ent->r.absmin[i] - origin[i];
			} else if ( origin[i] > ent->r.absmax[i] ) {
				v[i] = origin[i] - ent->r.absmax[i];
			} else {
				v[i] = 0;
			}
		}

		dist = VectorLength( v );
		if ( dist >= radius ) {
			continue;
		}

		points = damage * ( 1.0 - dist / radius );

		if( CanDamage (ent, origin) ) {
			if( LogAccuracyHit( ent, attacker ) ) {
				hitClient = qtrue;
			}
			VectorSubtract (ent->r.currentOrigin, origin, dir);
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir[2] += 24;
			G_Damage (ent, NULL, attacker, dir, origin, (int)points, DAMAGE_RADIUS, mod);
		}
	}

	return hitClient;
}
