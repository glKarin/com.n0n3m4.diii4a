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

/*
 * name:		g_combat.c
 *
 * desc:
 *
*/

#include "g_local.h"
#include "g_survival.h"

#include <pthread.h>
#include <unistd.h>

#include "../steam/steam.h"

extern vec3_t muzzleTrace; // used by falloff mechanic

/*
============
AddScore

Adds score to both the client and his team
============
*/
void AddScore( gentity_t *ent, int score ) {
	if ( !ent->client ) {
		return;
	}
	// no scoring during pre-match warmup
	if ( level.warmupTime ) {
		return;
	}

	if ( g_gametype.integer != GT_SURVIVAL ) {
		return;
	}

	ent->client->ps.persistant[PERS_SCORE] += score;

}



extern qboolean G_ThrowChair( gentity_t *ent, vec3_t dir, qboolean force );

/*
=================
TossClientWeapons

Toss the weapon and powerups for the killed entity
=================
*/
void TossClientWeapons( gentity_t *self )
{
	gitem_t *item;
	vec3_t forward;
	int weapon;
	gentity_t *drop = 0;

	// drop the weapon if not a gauntlet or machinegun
	weapon = self->s.weapon;

	if (g_gametype.integer == GT_SURVIVAL)
	{
		return;
	}

	if (g_gametype.integer == GT_GOTHIC)
	{ // Gothicstein. Robots never drop weapons.
		switch (self->aiCharacter)
		{
		case AICHAR_ZOMBIE:
		case AICHAR_WARZOMBIE:
		case AICHAR_LOPER:
		case AICHAR_LOPER_SPECIAL:
		case AICHAR_PROTOSOLDIER:
		case AICHAR_SUPERSOLDIER:
		case AICHAR_SUPERSOLDIER_LAB:
		case AICHAR_DOG:
		case AICHAR_PRIEST:
		case AICHAR_XSHEPHERD:
			return;
		default:
			break;
		}
	}
	else
	{ // Default case. Robots do drop weapons.
		switch (self->aiCharacter)
		{
		case AICHAR_ZOMBIE:
		case AICHAR_WARZOMBIE:
		case AICHAR_LOPER:
		case AICHAR_LOPER_SPECIAL:
		case AICHAR_DOG:
		case AICHAR_PRIEST:
		case AICHAR_XSHEPHERD:
			return;
		default:
			break;
		}
	}

	AngleVectors(self->r.currentAngles, forward, NULL, NULL);

	G_ThrowChair(self, forward, qtrue); // drop chair if you're holding one  //----(SA)	added

	// make a special check to see if they are changing to a new
	// weapon that isn't the mg or gauntlet.  Without this, a client
	// can pick up a weapon, be killed, and not drop the weapon because
	// their weapon change hasn't completed yet and they are still holding the MG.

	// (SA) always drop what you were switching to
	if (1)
	{
		if (self->client->ps.weaponstate == WEAPON_DROPPING || self->client->ps.weaponstate == WEAPON_DROPPING_TORELOAD)
		{
			weapon = self->client->pers.cmd.weapon;
		}
		if (!(COM_BitCheck(self->client->ps.weapons, weapon)))
		{
			weapon = WP_NONE;
		}
	}

	//----(SA)	added
	if (weapon == WP_SNOOPERSCOPE)
	{
		weapon = WP_GARAND;
	}
	if (weapon == WP_FG42SCOPE)
	{
		weapon = WP_FG42;
	}

	if (weapon > WP_NONE && weapon < WP_DUMMY_MG42 && self->client->ps.ammo[BG_FindAmmoForWeapon(weapon)])
	{
		// find the item type for this weapon
		item = BG_FindItemForWeapon(weapon);
		// spawn the item

		// Rafael
		if (!(self->client->ps.persistant[PERS_HWEAPON_USE]))
		{
			drop = Drop_Item(self, item, 0, qfalse);
		}
	}

	// dropped items stay forever in SP
	if (drop)
	{
		if (g_gametype.integer == GT_SURVIVAL)
		{
			drop->nextthink = level.time + 100000;
		}
		else
		{
			drop->nextthink = 0;
		}

	}
}

/*
==================
LookAtKiller
==================
*/
void LookAtKiller( gentity_t *self, gentity_t *inflictor, gentity_t *attacker ) {
	vec3_t dir;

	if ( attacker && attacker != self ) {
		VectorSubtract( attacker->s.pos.trBase, self->s.pos.trBase, dir );
	} else if ( inflictor && inflictor != self ) {
		VectorSubtract( inflictor->s.pos.trBase, self->s.pos.trBase, dir );
	} else {
		self->client->ps.stats[STAT_DEAD_YAW] = self->s.angles[YAW];
		return;
	}

	self->client->ps.stats[STAT_DEAD_YAW] = vectoyaw( dir );
}


/*
==============
GibHead
==============
*/
void GibHead( gentity_t *self, int killer ) {
	G_AddEvent( self, EV_GIB_HEAD, killer );
}

/*
==================
GibEntity
==================
*/
void GibEntity( gentity_t *self, int killer ) {
	gentity_t *other = &g_entities[killer];
	vec3_t dir;

	VectorClear( dir );
	if ( other->inuse ) {
		if ( other->client ) {
			VectorSubtract( self->r.currentOrigin, other->r.currentOrigin, dir );
			VectorNormalize( dir );
		} else if ( !VectorCompare( other->s.pos.trDelta, vec3_origin ) ) {
			VectorNormalize2( other->s.pos.trDelta, dir );
		}
	}

	G_AddEvent( self, EV_GIB_PLAYER, DirToByte( dir ) );
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
		self->health = GIB_HEALTH + 1;
		return;
	}

	if ( meansOfDeath == MOD_POISONGAS || meansOfDeath == MOD_KNIFE || meansOfDeath == MOD_THROWKNIFE  ) {
		self->health = GIB_HEALTH + 1;
		return;
	}

	if ( self->aiCharacter == AICHAR_HEINRICH || self->aiCharacter == AICHAR_HELGA || self->aiCharacter == AICHAR_SUPERSOLDIER || self->aiCharacter == AICHAR_SUPERSOLDIER_LAB || self->aiCharacter == AICHAR_PROTOSOLDIER ) {
		if ( self->health <= GIB_HEALTH ) {
			self->health = -1;
			return;
		}
	}

	GibEntity( self, 0 );
}


// these are just for logging, the client prints its own messages
char    *modNames[] = {
	"MOD_UNKNOWN",
	"MOD_SHOTGUN",
	"MOD_MONSTER_MELEE",
	"MOD_MACHINEGUN",
	"MOD_GRENADE",
	"MOD_GRENADE_SPLASH",
	"MOD_ROCKET",
	"MOD_ROCKET_SPLASH",
	"MOD_RAILGUN",
	"MOD_LIGHTNING",
	"MOD_BFG",
	"MOD_BFG_SPLASH",
	"MOD_KNIFE",
	"MOD_KNIFE2",
	"MOD_KNIFE_STEALTH",
	"MOD_LUGER",
	"MOD_COLT",
	"MOD_MP40",
	"MOD_THOMPSON",
	"MOD_STEN",
	"MOD_MAUSER",
	"MOD_SNIPERRIFLE",
	"MOD_GARAND",
	"MOD_SNOOPERSCOPE",
	"MOD_SILENCER", //----(SA)
	"MOD_AKIMBO",    //----(SA)
	"MOD_BAR",   //----(SA)
	"MOD_FG42",
	"MOD_FG42SCOPE",
	"MOD_PANZERFAUST",
	"MOD_ROCKET_LAUNCHER",
	"MOD_GRENADE_LAUNCHER",
	"MOD_VENOM",
	"MOD_VENOM_FULL",
	"MOD_FLAMETHROWER",
	"MOD_FLAMETRAP",
	"MOD_TESLA",
	"MOD_GRENADE_PINEAPPLE",
	"MOD_CROSS",
	"MOD_MORTAR",
	"MOD_MORTAR_SPLASH",
	"MOD_KICKED",
	"MOD_GRABBER",
	"MOD_DYNAMITE",
	"MOD_DYNAMITE_SPLASH",
	"MOD_AIRSTRIKE", // JPW NERVE
	"MOD_WATER",
	"MOD_SLIME",
	"MOD_LAVA",
	"MOD_CRUSH",
	"MOD_TELEFRAG",
	"MOD_FALLING",
	"MOD_SUICIDE",
	"MOD_TARGET_LASER",
	"MOD_TRIGGER_HURT",
	"MOD_GRAPPLE",
	"MOD_EXPLOSIVE",
	"MOD_POISONGAS",
	"MOD_ZOMBIESPIT",
	"MOD_ZOMBIESPIT_SPLASH",
	"MOD_ZOMBIESPIRIT",
	"MOD_ZOMBIESPIRIT_SPLASH",
	"MOD_LOPER_LEAP",
	"MOD_LOPER_GROUND",
	"MOD_LOPER_HIT",
// JPW NERVE
	"MOD_LT_ARTILLERY",
	"MOD_LT_AIRSTRIKE",
	"MOD_ENGINEER",  // not sure if we'll use
	"MOD_MEDIC",     // these like this or not
	"MOD_M97", // jaymod
// jpw
	"MOD_BAT"
};


void *remove_powerup_after_delay2(void *arg) {
    gentity_t *other = (gentity_t *)arg;

    // Sleep for 30 seconds
    sleep(30);

    // Remove the FL_NOTARGET flag
    other->flags &= ~FL_NOTARGET;

    return NULL;
}


/*
==================
player_die_secondchance
==================
*/
void player_die_secondchance( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {

	int clientNum;
	clientNum = level.sortedClients[0];

	if ( self->client->ps.pm_type == PM_DEAD ) {
		return;
	}

	if ( level.intermissiontime ) {
		return;
	}

	if ( !g_cheats.integer ) 
	{
    steamSetAchievement("ACH_SECOND_CHANCE");
	}

	// Grant the player 200 health points
    self->health = 200;

	 // Grant the player invisibility powerup for a limited time (e.g., 30 seconds)
    self->client->ps.powerups[PW_INVIS] = level.time + 30000;
    G_AddPredictableEvent( self, EV_ITEM_PICKUP, BG_FindItemForClassName( "item_invis" ) - bg_itemlist );
	self->flags |= FL_NOTARGET;
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, remove_powerup_after_delay2, (void *)self);

	// Remove all current perks from the player
    memset(self->client->ps.perks, 0, sizeof(self->client->ps.perks));
	self->client->ps.stats[STAT_PERK] = 0; // Clear all perk bits

	 // Reset the player's state to prevent immediate death again
    self->client->ps.pm_type = PM_NORMAL;
    self->client->ps.stats[STAT_HEALTH] = self->health;

	ClientUserinfoChanged( clientNum );

}

/*
==================
player_die
==================
*/
void player_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath ) {
	gentity_t   *ent;
	int anim;
	int contents = 0;
	int killer;
	int i;
	char        *killerName, *obit;
	qboolean nogib = qtrue;


	// Check if the player has the PERK_SECONDCHANCE perk
    if (self->client->ps.perks[PERK_SECONDCHANCE]) {
        player_die_secondchance(self, inflictor, attacker, damage, meansOfDeath);
        return;
    }

	if ( self->client->ps.pm_type == PM_DEAD ) {
		return;
	}

	if ( level.intermissiontime ) {
		return;
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

	if ( meansOfDeath < 0 || meansOfDeath >= ARRAY_LEN( modNames ) ) {
		obit = "<bad obituary>";
	} else {
		obit = modNames[ meansOfDeath ];
	}

	G_LogPrintf( "Kill: %i %i %i: %s killed %s by %s\n",
				 killer, self->s.number, meansOfDeath, killerName,
				 self->client->pers.netname, obit );

	// broadcast the death event to everyone
	ent = G_TempEntity( self->r.currentOrigin, EV_OBITUARY );
	ent->s.eventParm = meansOfDeath;
	ent->s.otherEntityNum = self->s.number;
	ent->s.otherEntityNum2 = killer;
	ent->r.svFlags = SVF_BROADCAST; // send to everyone

	self->enemy = attacker;

	self->client->ps.persistant[PERS_KILLED]++;

	if ( attacker && attacker->client ) {
		if ( attacker == self || OnSameTeam( self, attacker ) ) {
			AddScore( attacker, -1 );
		} else {
			AddScore( attacker, 1 );

			attacker->client->lastKillTime = level.time;
		}
	} else {
		AddScore( self, -1 );
	}

	// Add team bonuses
	Team_FragBonuses( self, inflictor, attacker );

		contents = trap_PointContents( self->r.currentOrigin, -1 );
		if ( !( contents & CONTENTS_NODROP ) ) {
			TossClientWeapons( self );
		}

	Cmd_Score_f( self );        // show scores
	// send updated scores to any clients that are following this one,
	// or they would get stale scoreboards
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		gclient_t   *client;

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

	self->takedamage = qtrue;   // can still be gibbed

	self->s.powerups = 0;
// JPW NERVE -- only corpse in SP; in MP, need CONTENTS_BODY so medic can operate
	self->r.contents = CONTENTS_CORPSE;
	self->s.weapon = WP_NONE;

	self->s.angles[0] = 0;
	self->s.angles[2] = 0;
	LookAtKiller( self, inflictor, attacker );

	VectorCopy( self->s.angles, self->client->ps.viewangles );

	self->s.loopSound = 0;

	self->r.maxs[2] = -8;

	// don't allow respawn until the death anim is done
	// g_forcerespawn may force spawning at some later time
	if (g_gametype.integer == GT_SURVIVAL)
	{
		self->client->respawnTime = level.time + 12000;

		int wave = svParams.waveCount;
		int enemiesKilled = svParams.survivalKillCount;

		// This uses the exact same system as missionfail0, missionfail1, etc.
		trap_SendServerCommand(self - g_entities,
							   va("egp survival_gameover %d %d", wave, enemiesKilled));

		trap_SendServerCommand(-1, "mu_play sound/music/l_finale.wav 0");
		trap_SetConfigstring(CS_MUSIC_QUEUE, "");
		trap_SetConfigstring(CS_MUSIC, ""); // extra safety
	}
	else
	{
		self->client->respawnTime = level.time + 1700;
		trap_SendServerCommand(-1, "mu_play sound/music/l_failed_1.wav 0\n");
		trap_SetConfigstring(CS_MUSIC_QUEUE, ""); // clear queue so it'll be quiet after hit
		trap_SendServerCommand(-1, "cp missionfail0");
	}

	// remove powerups
	memset(self->client->ps.powerups, 0, sizeof(self->client->ps.powerups));

	// never gib in a nodrop
	contents = trap_PointContents( self->r.currentOrigin, -1 );

	if ( self->health <= GIB_HEALTH && !( contents & CONTENTS_NODROP ) && g_blood.integer ) {
		GibEntity( self, killer );
		nogib = qfalse;
	}

	if ( nogib ) {
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
			self->health = GIB_HEALTH + 1;
		}

		self->client->medicHealAmt = 0;

		self->client->ps.legsAnim =
			( ( self->client->ps.legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;
		self->client->ps.torsoAnim =
			( ( self->client->ps.torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;

		G_AddEvent( self, EV_DEATH1 + 1, killer );

		// the body can still be gibbed
		self->die = body_die;

		// globally cycle through the different death animations
		i = ( i + 1 ) % 3;
	}

	trap_LinkEntity( self );

	AICast_ScriptEvent( AICast_GetCastState( self->s.number ), "death", "" );

}


/*
================
CheckArmor
================
*/
int CheckArmor( gentity_t *ent, int damage, int dflags ) {
	gclient_t   *client;
	int save;
	int count;

	if ( !damage ) {
		return 0;
	}

	client = ent->client;

	if ( !client ) {
		return 0;
	}

	if ( dflags & DAMAGE_NO_ARMOR ) {
		return 0;
	}

	// armor
	count = client->ps.stats[STAT_ARMOR];
	save = ceil( damage * ARMOR_PROTECTION );
	if ( save >= count ) {
		save = count;
	}

	if ( !save ) {
		return 0;
	}

	client->ps.stats[STAT_ARMOR] -= save;

	if (client->ps.stats[STAT_ARMOR] < 0) client->ps.stats[STAT_ARMOR] = 0;

	return save;
}


/*
==============
IsHeadShotWeapon
==============
*/
qboolean IsHeadShotWeapon( int mod, gentity_t *targ, gentity_t *attacker ) {
	// distance rejection
	if ( DistanceSquared( targ->r.currentOrigin, attacker->r.currentOrigin )  >  ( g_headshotMaxDist.integer * g_headshotMaxDist.integer ) ) {
		return qfalse;
	}

    if (g_aicanheadshot.integer == 1) {
	    if ( attacker->aiCharacter ) {
		// ai's are always allowed headshots from these weapons
		   if ( mod == MOD_SNIPERRIFLE ||
			    mod == MOD_SNOOPERSCOPE ||
			    mod == MOD_DELISLESCOPE ||
				mod == MOD_M1941SCOPE ) {
			    return qtrue;
		        }
		return qfalse;
	    }
	} else {
		if ( attacker->aiCharacter ) 
		{
		return qfalse; // no ai headshots at all if cvar is off
	    }
	}

	switch ( targ->aiCharacter ) {
	// get out quick for ai's that don't take headshots
	case AICHAR_ZOMBIE:
	case AICHAR_ZOMBIE_SURV:
	case AICHAR_ZOMBIE_FLAME:
	case AICHAR_ZOMBIE_GHOST:
	case AICHAR_WARZOMBIE:
	case AICHAR_HELGA:     
	case AICHAR_LOPER:
	case AICHAR_LOPER_SPECIAL:
	case AICHAR_VENOM:      
	return qfalse;
	default:
	break;
	}

	switch ( mod ) {
	// players are allowed headshots from these weapons
	case MOD_LUGER:
	case MOD_COLT:
	case MOD_AKIMBO:
	case MOD_DUAL_TT33:
	case MOD_MP40:
	case MOD_MP34:
	case MOD_TT33:
	case MOD_HDM:
	case MOD_PPSH:
	case MOD_MOSIN:
	case MOD_G43:
	case MOD_M1941:
	case MOD_M1GARAND:
	case MOD_BAR:
	case MOD_MP44:
	case MOD_REVOLVER:
	case MOD_THOMPSON:
	case MOD_STEN:
	case MOD_FG42:
	case MOD_MAUSER:
	case MOD_GARAND:
	case MOD_SILENCER:
	case MOD_FG42SCOPE:
	case MOD_SNOOPERSCOPE:
	case MOD_DELISLE:
	case MOD_SNIPERRIFLE:
	case MOD_BROWNING:
	case MOD_MG42M:
	case MOD_M1941SCOPE:
	return qtrue;
	}
	return qfalse;
}


/*
==============
IsHeadShot
==============
*/
qboolean IsHeadShot( gentity_t *targ, gentity_t *attacker, vec3_t dir, vec3_t point, int mod ) {
	gentity_t   *head;
	trace_t tr;
	vec3_t start, end;
	gentity_t   *traceEnt;
	orientation_t or;

	qboolean head_shot_weapon = qfalse;

	// not a player or critter so bail
	if ( !( targ->client ) ) {
		return qfalse;
	}

	if ( targ->health <= 0 ) {
		return qfalse;
	}

	head_shot_weapon = IsHeadShotWeapon( mod, targ, attacker );

	if ( head_shot_weapon ) {
		head = G_Spawn();

		G_SetOrigin( head, targ->r.currentOrigin );

		// RF, if there is a valid tag_head for this entity, then use that
		if ( ( targ->r.svFlags & SVF_CASTAI ) && trap_GetTag( targ->s.number, "tag_head", &or ) ) {
			VectorCopy( or.origin, head->r.currentOrigin );
			VectorMA( head->r.currentOrigin, 6, or.axis[2], head->r.currentOrigin );    // tag is at base of neck
		} else if ( targ->client->ps.pm_flags & PMF_DUCKED ) { // closer fake offset for 'head' box when crouching
			head->r.currentOrigin[2] += targ->client->ps.crouchViewHeight + 8; // JPW NERVE 16 is kludge to get head height to match up
		}
		//else if(targ->client->ps.legsAnim == LEGS_IDLE && targ->aiCharacter == AICHAR_SOLDIER)	// standing with legs bent (about a head shorter than other leg poses)
		//	head->r.currentOrigin[2] += targ->client->ps.viewheight;
		else {
			head->r.currentOrigin[2] += targ->client->ps.viewheight; // JPW NERVE pulled this	// 6 is fudged "head height" value

		}
		VectorCopy( head->r.currentOrigin, head->s.origin );
		VectorCopy( targ->r.currentAngles, head->s.angles );
		VectorCopy( head->s.angles, head->s.apos.trBase );
		VectorCopy( head->s.angles, head->s.apos.trDelta );
		VectorSet( head->r.mins, -6, -6, -6 ); // JPW NERVE changed this z from -12 to -6 for crouching, also removed standing offset
		VectorSet( head->r.maxs, 6, 6, 6 ); // changed this z from 0 to 6
		head->clipmask = CONTENTS_SOLID;
		head->r.contents = CONTENTS_SOLID;

		trap_LinkEntity( head );

		// trace another shot see if we hit the head
		VectorCopy( point, start );
		VectorMA( start, 64, dir, end );
		trap_Trace( &tr, start, NULL, NULL, end, targ->s.number, MASK_SHOT );

		traceEnt = &g_entities[ tr.entityNum ];

		if ( g_debugBullets.integer >= 3 ) {   // show hit player head bb
			gentity_t *tent;
			vec3_t b1, b2;
			VectorCopy( head->r.currentOrigin, b1 );
			VectorCopy( head->r.currentOrigin, b2 );
			VectorAdd( b1, head->r.mins, b1 );
			VectorAdd( b2, head->r.maxs, b2 );
			tent = G_TempEntity( b1, EV_RAILTRAIL );
			VectorCopy( b2, tent->s.origin2 );
			tent->s.dmgFlags = 1;

			// show headshot trace
			// end the headshot trace at the head box if it hits
			if ( tr.fraction != 1 ) {
				VectorMA( start, ( tr.fraction * 64 ), dir, end );
			}
			tent = G_TempEntity( start, EV_RAILTRAIL );
			VectorCopy( end, tent->s.origin2 );
			tent->s.dmgFlags = 0;
		}

		G_FreeEntity( head );

		if ( traceEnt == head ) {
			return qtrue;
		}
	}

	return qfalse;
}





/*
==============
G_ArmorDamage
	brokeparts is how many should be broken off now
	curbroke is how many are broken
	the difference is how many to pop off this time
==============
*/
void G_ArmorDamage( gentity_t *targ ) {
	int brokeparts, curbroke;
	int numParts;
	int dmgbits = 16;   // 32/2;
	int i;

	if ( !targ->client ) {
		return;
	}

	if ( targ->s.aiChar == AICHAR_PROTOSOLDIER ) {
		numParts = 9;
	} else if ( targ->s.aiChar == AICHAR_SUPERSOLDIER ) {
		numParts = 14;
	} else if ( targ->s.aiChar == AICHAR_SUPERSOLDIER_LAB ) {
		numParts = 14;
	} else if ( targ->s.aiChar == AICHAR_HEINRICH ) {
		numParts = 20;
	} else {
		return;
	}

	if ( numParts > dmgbits ) {
		numParts = dmgbits; // lock this down so it doesn't overwrite any bits that it shouldn't.  TODO: fix this


	}
	// determined here (on server) by location of hit and existing armor, you're updating here so
	// the client knows which pieces are still in place, and by difference with previous state, which
	// pieces to play an effect where the part is blown off.
	// Need to do it here so we have info on where the hit registered (head, torso, legs or if we go with more detail; arm, leg, chest, codpiece, etc)

	// ... Ick, just discovered that the refined hit detection ("hit nearest to which tag") is clientside...

	// For now, I'll randomly pick a part that hasn't been cleared.  This might end up looking okay, and we won't need the refined hits.
	//	however, we still have control on the server-side of which parts come off, regardless of what shceme is used.

	brokeparts = (int)( ( 1 - ( (float)( targ->health ) / (float)( targ->client->ps.stats[STAT_MAX_HEALTH] ) ) ) * numParts );

	// RF, remove flame protection after enough parts gone
	if ( AICast_NoFlameDamage( targ->s.number ) && ( (float)brokeparts / (float)numParts >= 5.0 / 6.0 ) ) { // figure from DM
		AICast_SetFlameDamage( targ->s.number, qfalse );
	}

	if ( brokeparts && ( ( targ->s.dmgFlags & ( ( 1 << numParts ) - 1 ) ) != ( 1 << numParts ) - 1 ) ) {   // there are still parts left to clear

		// how many are removed already?
		curbroke = 0;
		for ( i = 0; i < numParts; i++ ) {
			if ( targ->s.dmgFlags & ( 1 << i ) ) {
				curbroke++;
			}
		}

		// need to remove more
		if ( brokeparts - curbroke >= 1 && curbroke < numParts ) {
			for ( i = 0; i < ( brokeparts - curbroke ); i++ ) {
				int remove = rand() % ( numParts );

				if ( !( ( targ->s.dmgFlags & ( ( 1 << numParts ) - 1 ) ) != ( 1 << numParts ) - 1 ) ) { // no parts are available any more
					break;
				}

				// FIXME: lose the 'while' loop.  Still should be safe though, since the check above verifies that it will eventually find a valid part
				while ( targ->s.dmgFlags & ( 1 << remove ) ) {
					remove = rand() % ( numParts );
				}

				targ->s.dmgFlags |= ( 1 << remove );    // turn off 'undamaged' part
				if ( (int)( random() + 0.5 ) ) {                       // choose one of two possible replacements
					targ->s.dmgFlags |= ( 1 << ( numParts + remove ) );
				}
			}
		}
	}
}
/*
============
G_Damage

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

void G_Damage( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
				vec3_t dir, vec3_t point, int damage, int dflags, int mod ) {
	G_DamageExt( targ, inflictor, attacker, dir, point, damage, dflags, mod, NULL );
}

void G_DamageExt( gentity_t *targ, gentity_t *inflictor, gentity_t *attacker,
			   vec3_t dir, vec3_t point, int damage, int dflags, int mod, int *hitEventOut ) {
	gclient_t   *client;
	int take;
	int asave;
	int knockback;
	int hitEventType = HIT_NONE;

	if ( hitEventOut ) {
		*hitEventOut = HIT_NONE;
	}

	if ( !targ->takedamage ) {
		return;
	}

//----(SA)	added
	if ( !targ->aiCharacter && targ->client && targ->client->cameraPortal ) {
		// get out of damage in sp if in cutscene.
		return;
	}
//----(SA)	end

//	if (reloading || saveGamePending) {	// map transition is happening, don't do anything
	if ( g_reloading.integer || saveGamePending ) {
		return;
	}

	// the intermission has already been qualified for, so don't
	// allow any extra scoring
	if ( level.intermissionQueued ) {
		return;
	}

	// RF, track pain for player
	// This is used by AI to determine how long it has been since their enemy was injured

	if ( attacker ) { // (SA) whoops, for falling damage there's no attacker
		if ( targ->client && attacker->client && !( targ->r.svFlags & SVF_CASTAI ) && ( attacker->r.svFlags & SVF_CASTAI ) ) {
			AICast_RegisterPain( targ->s.number );
		}
	}

	if ( !( targ->r.svFlags & SVF_CASTAI ) ) { // the player
		switch ( mod )
		{
		case MOD_GRENADE:
		case MOD_GRENADE_SPLASH:
		case MOD_ROCKET:
		case MOD_ROCKET_SPLASH:
			// Rafael - had to change this since the
			// we added a new lvl of diff
			if ( g_gameskill.integer == GSKILL_EASY ) {
				damage *= 0.75;
			} else if ( g_gameskill.integer == GSKILL_MEDIUM ) {
				damage *= 0.80;
			} else if ( g_gameskill.integer == GSKILL_HARD ) {
				damage *= 0.9;
			} else {
				damage *= 0.9;
			}
		default:
			break;
		}
	}

	if ( !inflictor ) {
		inflictor = &g_entities[ENTITYNUM_WORLD];
	}
	if ( !attacker ) {
		attacker = &g_entities[ENTITYNUM_WORLD];
	}

	// shootable doors / buttons don't actually have any health
	if ( targ->s.eType == ET_MOVER && !( targ->aiName ) && !( targ->isProp ) && !targ->scriptName ) {
		if ( targ->use && targ->moverState == MOVER_POS1 ) {
			targ->use( targ, inflictor, attacker );
		}
		return;
	}

	if ( targ->s.eType == ET_MOVER && targ->aiName && !( targ->isProp ) && !targ->scriptName ) {
		switch ( mod ) {
		case MOD_GRENADE:
		case MOD_GRENADE_SPLASH:
		case MOD_ROCKET:
		case MOD_ROCKET_SPLASH:
			break;
		default:
			return; // no damage from other weapons
		}
	} else if ( targ->s.eType == ET_EXPLOSIVE )   {
		// 32 Explosive
		// 64 Dynamite only
		if ( ( targ->spawnflags & 32 ) || ( targ->spawnflags & 64 ) ) {
			switch ( mod ) {
			case MOD_GRENADE:
			case MOD_GRENADE_SPLASH:
			case MOD_ROCKET:
			case MOD_ROCKET_SPLASH:
			case MOD_AIRSTRIKE:
			case MOD_GRENADE_PINEAPPLE:
			case MOD_MORTAR:
			case MOD_MORTAR_SPLASH:
			case MOD_EXPLOSIVE:
				if ( targ->spawnflags & 64 ) {
					return;
				}

				break;

			case MOD_DYNAMITE:
			case MOD_DYNAMITE_SPLASH:
				break;

			default:
				return;
			}
		}
	}

	// Ridah, Cast AI's don't hurt other Cast AI's as much
	if ( ( attacker->r.svFlags & SVF_CASTAI ) && ( targ->r.svFlags & SVF_CASTAI ) ) {
		if ( !AICast_AIDamageOK( AICast_GetCastState( targ->s.number ), AICast_GetCastState( attacker->s.number ) ) ) {
			return;
		}
		damage = (int)( ceil( (float)damage * 0.5 ) );
	}
	// done.

	client = targ->client;

	if ( client ) {
		if ( client->noclip ) {
			return;
		}
	}

	if ( !dir ) {
		dflags |= DAMAGE_NO_KNOCKBACK;
	} else {
		VectorNormalize( dir );
	}

	knockback = damage;

//	if ( knockback > 200 )
//		knockback = 200;
	if ( knockback > 60 ) { // /way/ lessened for SP.  keeps dynamite-jumping potential down
		knockback = 60;
	}

	if ( targ->flags & FL_NO_KNOCKBACK ) {
		knockback = 0;
	}
	if ( dflags & DAMAGE_NO_KNOCKBACK ) {
		knockback = 0;
	}

	// figure momentum add, even if the damage won't be taken
	if ( knockback && targ->client ) {
		vec3_t kvel;
		float mass;

		mass = 200;

		if ( mod == MOD_LIGHTNING && !( ( level.time + targ->s.number * 50 ) % 400 ) ) {
			knockback = 60;
			dir[2] = 0.3;
		}

		VectorScale( dir, g_knockback.value * (float)knockback / mass, kvel );
		VectorAdd( targ->client->ps.velocity, kvel, targ->client->ps.velocity );

		if ( targ == attacker && !(  mod != MOD_ROCKET &&
									 mod != MOD_ROCKET_SPLASH &&
									 mod != MOD_GRENADE &&
									 mod != MOD_GRENADE_SPLASH &&
									 mod != MOD_DYNAMITE &&
									 mod != MOD_M7 ) ) {
			targ->client->ps.velocity[2] *= 0.25;
		}

		// set the timer so that the other client can't cancel
		// out the movement immediately
		if ( !targ->client->ps.pm_time ) {
			int t;

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
	}

	// check for completely getting out of the damage
	if ( !( dflags & DAMAGE_NO_PROTECTION ) ) {

		// if TF_NO_FRIENDLY_FIRE is set, don't do damage to the target
		// if the attacker was on the same team
		if ( targ != attacker && OnSameTeam( targ, attacker )  ) {
			if ( !g_friendlyFire.integer ) {
				return;
			}
		}

		// check for godmode
		if ( targ->flags & FL_GODMODE ) {
			return;
		}

		// RF, warzombie defense position is basically godmode for the time being
		if ( targ->flags & FL_DEFENSE_GUARD ) {
			return;
		}

		// check for invulnerability // (SA) moved from below so DAMAGE_NO_PROTECTION will still work
		if ( client && client->ps.powerups[PW_INVULNERABLE] ) { //----(SA)	added
			return;
		}

	}

	// add to the attacker's hit counter (but only if target is a client)
	if ( attacker && attacker->client && targ->client  && targ != attacker && mod != MOD_SUICIDE ) {
		if ( OnSameTeam( targ, attacker ) /*|| ( targ->client->ps.powerups[PW_OPS_DISGUISED]*/ && g_friendlyFire.integer & 1 && g_gametype.integer != GT_SURVIVAL ) {
			hitEventType = HIT_TEAMSHOT;
		}
		else /*if ( !targ->client->ps.powerups[PW_OPS_DISGUISED] )*/ {
			hitEventType = HIT_BODYSHOT;
		}

		//BG_UpdateConditionValue( targ->client->ps.clientNum, ANIM_COND_ENEMY_WEAPON, attacker->client->ps.weapon, qtrue );
	}

	// battlesuit protects from all radius damage (but takes knockback)
	// and protects 50% against all damage
	if ( client && client->ps.powerups[PW_BATTLESUIT] ) {
		G_AddEvent( targ, EV_POWERUP_BATTLESUIT, 0 );

		damage *= 0.30;
	}

	if ( client && client->ps.powerups[PW_BATTLESUIT_SURV] ) {
		G_AddEvent( targ, EV_POWERUP_BATTLESUIT_SURV, 0 );

		damage *= 0.05;
	}

	// always give half damage if hurting self
	// calculated after knockback, so rocket jumping works

		qboolean dynamite = (qboolean)( mod == MOD_DYNAMITE || mod == MOD_DYNAMITE_SPLASH );

		qboolean shotguns = (qboolean)( mod == MOD_M97 || mod == MOD_AUTO5 );

		qboolean panzer = (qboolean)( mod == MOD_PANZERFAUST );

		qboolean venomgun = (qboolean)( mod == MOD_VENOM );

		if ( targ == attacker ) {
			if ( !dynamite ) {
				damage *= 0.5;
			}
		}

        // Helga special damage cases
		if ( dynamite && targ->aiCharacter == AICHAR_HELGA ) {
			damage *= 0.5;
		}

        // Heinrich special damage cases
		if ( venomgun && targ->aiCharacter == AICHAR_HEINRICH ) {
			damage *= 0.5;
		}

		if ( panzer && targ->aiCharacter == AICHAR_HEINRICH ) {
			damage *= 0.6;
		}

		if ( shotguns && targ->aiCharacter == AICHAR_HEINRICH ) {
			damage *= 0.5;
		}

        // Loper special damage cases
		if ( venomgun && targ->aiCharacter == AICHAR_LOPER ) {
			damage *= 1.2;
		}


        // Supersoldier special damage cases
		if ( venomgun && targ->aiCharacter == AICHAR_SUPERSOLDIER ) {
			damage *= 0.7;
		}

		if ( venomgun && targ->aiCharacter == AICHAR_SUPERSOLDIER_LAB ) {
			damage *= 0.7;
		}

		if ( panzer && targ->aiCharacter == AICHAR_SUPERSOLDIER_LAB ) {
			damage *= 0.6;
		}

		if ( panzer && targ->aiCharacter == AICHAR_SUPERSOLDIER ) {
			damage *= 0.6;
		}

		if ( shotguns && targ->aiCharacter == AICHAR_SUPERSOLDIER ) {
			damage *= 0.6;
		}

		if ( shotguns && targ->aiCharacter == AICHAR_SUPERSOLDIER_LAB ) {
			damage *= 0.6;
		}

	

	if ( damage < 1 ) {
		damage = 1;
	}
	take = damage;

	// save some from armor
	asave = CheckArmor( targ, take, dflags );
	take -= asave;


	if ( IsHeadShot( targ, attacker, dir, point, mod ) ) {

			// Upgrade the hit event to headshot if we have not yet classified it as a teamshot (covertops etc..)
			if ( hitEventType != HIT_TEAMSHOT ) {
				hitEventType = HIT_HEADSHOT;
			}

			// by default, a headshot means damage x2
			take *= 2;

			// RF, allow headshot damage multiplier (helmets, etc)
			// yes, headshotDamageScale of 0 gives no damage, thats because
			// the bullet hit the head which is fully protected.
			take *= targ->headshotDamageScale;
             
			// damage falloff for headshots
			if ( g_gametype.integer == GT_GOTHIC || g_weaponfalloff.integer == 1 ) {
			if ( dflags & DAMAGE_DISTANCEFALLOFF ) {
			vec_t dist;
			vec3_t shotvec;
			float scale;

			VectorSubtract( point, muzzleTrace, shotvec );
			dist = VectorLength( shotvec );

			// ~~~---______
			// zinx - start at 100% at 1500 units (and before),
			// and go to 20% at 2500 units (and after)

			// 1500 to 2500 -> 0.0 to 1.0
			scale = ( dist - ammoTable [attacker->s.weapon].falloffDistance[0] ) / ( ammoTable [attacker->s.weapon].falloffDistance[1] - ammoTable [attacker->s.weapon].falloffDistance[0] );
			// 0.0 to 1.0 -> 0.0 to 0.8
			scale *= 0.8f;
			// 0.0 to 0.8 -> 1.0 to 0.2
			scale = 1.0f - scale;

			// And, finally, cap it.
			if ( scale > 1.0f ) {
				scale = 1.0f;
			} else if ( scale < 0.2f ) {
				scale = 0.2f;
			}

			take *= scale;
		}
		}

			// player only code
			if ( !attacker->aiCharacter ) {
				// (SA) id reqests one-shot kills for head shots on common humanoids

				// (SA) except pistols.
				// first pistol head shot does normal 2x damage and flings hat, second gets kill
				//			if((mod != MOD_LUGER && mod != MOD_COLT ) || (targ->client->ps.eFlags & EF_HEADSHOT))	{	// (SA) DM requests removing double shot pistol head shots (3/19)

				// (SA) removed BG for DM.

				if ( !( dflags & DAMAGE_PASSTHRU ) ) {     // ignore headshot 2x damage and snooper-instant-death if the bullet passed through something.  just do reg damage.
					switch ( targ->aiCharacter ) {
					case AICHAR_BLACKGUARD:
						if ( !( targ->client->ps.eFlags & EF_HEADSHOT ) ) { // only obliterate him after he's lost his helmet
							break;
						}
					case AICHAR_SOLDIER:
					case AICHAR_AMERICAN:
					case AICHAR_ELITEGUARD:
					case AICHAR_PARTISAN:
					case AICHAR_RUSSIAN:
					case AICHAR_CIVILIAN:
						take = 200;
						break;
					default:
						break;
					}
				}

				if ( !( targ->client->ps.eFlags & EF_HEADSHOT ) ) {  // only toss hat on first headshot
					G_AddEvent( targ, EV_LOSE_HAT, DirToByte( dir ) );
				}
			}


		// shared by both player and ai
		targ->client->ps.eFlags |= EF_HEADSHOT;

	} else {    // non headshot

		if ( !( dflags & DAMAGE_PASSTHRU ) ) {     // ignore headshot 2x damage and snooper-instant-death if the bullet passed through something.  just do reg damage.
			// snooper kills these types in one shot with any contact
			if ( ( mod == MOD_SNOOPERSCOPE || mod == MOD_GARAND ) && !( attacker->aiCharacter ) ) {
				switch ( targ->aiCharacter ) {
				case AICHAR_SOLDIER:
				case AICHAR_AMERICAN:
				case AICHAR_ELITEGUARD:
				case AICHAR_BLACKGUARD:
				case AICHAR_PARTISAN:
				case AICHAR_RUSSIAN:
				case AICHAR_CIVILIAN:
					take = 200;
					break;
				default:
					break;
				}
			}
		}
	}

	if ( targ->client && targ->client->ps.stats[STAT_HEALTH] > 0 ) {
		if ( hitEventType ) {
			if ( !hitEventOut ) {
				G_AddEvent( attacker, EV_PLAYER_HIT, hitEventType );
			} else {
				*hitEventOut = hitEventType;
			}
		}
	}

	if ( g_debugDamage.integer ) {
		G_Printf( "client:%i health:%i damage:%i armor:%i\n", targ->s.number,
				  targ->health, take, asave );
	}

	// add to the damage inflicted on a player this frame
	// the total will be turned into screen blends and view angle kicks
	// at the end of the frame
	if ( client ) {
		if ( attacker ) {
			client->ps.persistant[PERS_ATTACKER] = attacker->s.number;
		} else {
			client->ps.persistant[PERS_ATTACKER] = ENTITYNUM_WORLD;
		}
		client->damage_armor += asave;
		client->damage_blood += take;
		client->damage_knockback += knockback;

		client->healthRegenStartTime = level.time + 2500; // This will reset health regen timer

		if ( dir ) {
			VectorCopy( dir, client->damage_from );
			client->damage_fromWorld = qfalse;
		} else {
			VectorCopy( targ->r.currentOrigin, client->damage_from );
			client->damage_fromWorld = qtrue;
		}
	}

	// See if it's the player hurting the emeny flag carrier
	Team_CheckHurtCarrier( targ, attacker );

	if ( targ->client ) {
		// set the last client who damaged the target
		targ->client->lasthurt_client = attacker->s.number;
		targ->client->lasthurt_mod = mod;
	}

	// do the damage
	if ( take ) {
		targ->health = targ->health - take;

		// Ridah, can't gib with bullet weapons (except VENOM)
        if ( targ->client ) {
            if ( mod != MOD_VENOM && attacker == inflictor && targ->health <= GIB_HEALTH ) {
                if ( targ->aiCharacter != AICHAR_ZOMBIE ) { // zombie needs to be able to gib so we can kill him (although he doesn't actually GIB, he just dies)
                    if (!(attacker && attacker->client && attacker->client->ps.powerups[PW_QUAD])) {
                        targ->health = GIB_HEALTH + 1;
                    }
                }
            }
        }

		//G_Printf("health at: %d\n", targ->health);
		if ( targ->health <= 0 ) {
			if ( client ) {
				targ->flags |= FL_NO_KNOCKBACK;
			}

			if ( targ->health < -999 ) {
				targ->health = -999;
			}

			targ->enemy = attacker;
			if ( targ->die ) { // Ridah, mg42 doesn't have die func (FIXME)
				targ->die( targ, inflictor, attacker, take, mod );
			}

			// if we freed ourselves in death function
			if ( !targ->inuse ) {
				return;
			}

			// RF, entity scripting
			if ( targ->s.number >= MAX_CLIENTS && targ->health <= 0 ) { // might have revived itself in death function
				G_Script_ScriptEvent( targ, "death", "" );
			}

		} else if ( targ->pain ) {
			targ->lastPainMOD = mod; // set mod for AICast_Pain to access
			
			if ( dir ) {  // Ridah, had to add this to fix NULL dir crash
				VectorCopy( dir, targ->rotate );
				VectorCopy( point, targ->pos3 ); // this will pass loc of hit
			} else {
				VectorClear( targ->rotate );
				VectorClear( targ->pos3 );
			}

			targ->pain(targ, attacker, take, point);

			targ->lastPainMOD = 0; // optional: reset after use
		}

		G_ArmorDamage( targ );    //----(SA)	moved out to separate routine

		// Ridah, this needs to be done last, incase the health is altered in one of the event calls
		if ( targ->client ) {
			targ->client->ps.stats[STAT_HEALTH] = targ->health;
		}

		 // Cheap way to ID inflictor entity as poison smoke.
		if (inflictor->poisonGasAlarm && mod == MOD_POISONGAS && targ->health >= 0)
			G_AddEvent(targ, EV_COUGH, 0);
	}

}


/*
============
CanDamage

Returns qtrue if the inflictor can directly damage the target.  Used for
explosions and melee attacks.
============
*/
qboolean CanDamage( gentity_t *targ, vec3_t origin ) {
	vec3_t dest;
	trace_t tr;
	vec3_t midpoint;
	vec3_t	offsetmins = {-15, -15, -15};
	vec3_t	offsetmaxs = {15, 15, 15};

	// use the midpoint of the bounds instead of the origin, because
	// bmodels may have their origin is 0,0,0
	VectorAdd( targ->r.absmin, targ->r.absmax, midpoint );
	VectorScale( midpoint, 0.5, midpoint );

	VectorCopy(midpoint, dest);
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if ( tr.fraction == 1.0 ) {
		return qtrue;
	}



	if ( &g_entities[tr.entityNum] == targ ) {
		return qtrue;
	}

	// this should probably check in the plane of projection,
	// rather than in world coordinate
	VectorCopy(midpoint, dest);
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmaxs[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy(midpoint, dest);
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmins[1];
	dest[2] += offsetmaxs[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if ( tr.fraction == 1.0 ) {
		return qtrue;
	}

	VectorCopy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmaxs[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if ( tr.fraction == 1.0 ) {
		return qtrue;
	}

	VectorCopy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmins[1];
	dest[2] += offsetmaxs[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if ( tr.fraction == 1.0 ) {
		return qtrue;
	}

	VectorCopy(midpoint, dest);
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if ( tr.fraction == 1.0 ) {
		return qtrue;
	}

	VectorCopy(midpoint, dest);
	dest[0] += offsetmaxs[0];
	dest[1] += offsetmins[1];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmaxs[1];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if (tr.fraction == 1.0)
		return qtrue;

	VectorCopy(midpoint, dest);
	dest[0] += offsetmins[0];
	dest[1] += offsetmins[1];
	dest[2] += offsetmins[2];
	trap_Trace(&tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID);

	if (tr.fraction == 1.0)
		return qtrue;

	return qfalse;
}

/*
============
G_RadiusDamage
============
*/
qboolean G_RadiusDamage( vec3_t origin, gentity_t *attacker, float damage, float radius,
						 gentity_t *ignore, int mod ) {
	float points, dist;
	gentity_t   *ent;
	int entityList[MAX_GENTITIES];
	int numListedEntities;
	vec3_t mins, maxs;
	vec3_t v;
	vec3_t dir;
	int i, e;
	qboolean hitClient = qfalse;
// JPW NERVE
	float boxradius;
	//vec3_t dest;
	//trace_t tr;
	//vec3_t midpoint;
// jpw


	if ( radius < 1 ) {
		radius = 1;
	}

	boxradius = 1.41421356 * radius; // radius * sqrt(2) for bounding box enlargement --
	// bounding box was checking against radius / sqrt(2) if collision is along box plane
	for ( i = 0 ; i < 3 ; i++ ) {
		mins[i] = origin[i] - boxradius; // JPW NERVE
		maxs[i] = origin[i] + boxradius; // JPW NERVE
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for ( e = 0 ; e < numListedEntities ; e++ ) {
		ent = &g_entities[entityList[ e ]];

		if ( ent == ignore ) {
			continue;
		}
		if ( !ent->takedamage ) {
			continue;
		}

/* JPW NERVE -- we can put this back if we need to, but it kinna sucks for human-sized bboxes
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
*/
// JPW NERVE
		if ( !ent->r.bmodel ) {
			VectorSubtract( ent->r.currentOrigin,origin,v ); // JPW NERVE simpler centroid check that doesn't have box alignment weirdness
		} else {
			for ( i = 0 ; i < 3 ; i++ ) {
				if ( origin[i] < ent->r.absmin[i] ) {
					v[i] = ent->r.absmin[i] - origin[i];
				} else if ( origin[i] > ent->r.absmax[i] ) {
					v[i] = origin[i] - ent->r.absmax[i];
				} else {
					v[i] = 0;
				}
			}
		}
// jpw
		dist = VectorLength( v );

		if ( dist >= radius ) {
			continue;
		}

		points = damage * ( 1.0 - dist / radius );

// JPW NERVE -- different radiusdmg behavior for MP -- big explosions should do less damage (over less distance) through failed traces
		if (CanDamage(ent, origin))
		{
			if (LogAccuracyHit(ent, attacker))
			{
				hitClient = qtrue;
			}
			VectorSubtract(ent->r.currentOrigin, origin, dir);
			dir[2] += 24;

			int finalDamage = (int)points;

			// Reduce self-damage in Survival mode only
			if (ent == attacker && attacker->client)
			{
				if (mod == MOD_ROCKET_SPLASH && g_gametype.integer == GT_SURVIVAL)
				{
					finalDamage *= 0.3f;
				}
			}

			G_Damage(ent, NULL, attacker, dir, origin, finalDamage, DAMAGE_RADIUS, mod);
		}
	}
	return hitClient;
}

/*
============
G_AdjustedDamageVec
Used by: WP_POISONGAS
============
*/

void G_AdjustedDamageVec( gentity_t *ent, vec3_t origin, vec3_t v )
{
	int i;

	if (!ent->r.bmodel)
		VectorSubtract(ent->r.currentOrigin,origin,v); // JPW NERVE simpler centroid check that doesn't have box alignment weirdness
	else {
		for ( i = 0 ; i < 3 ; i++ ) {
			if ( origin[i] < ent->r.absmin[i] ) {
				v[i] = ent->r.absmin[i] - origin[i];
			} else if ( origin[i] > ent->r.absmax[i] ) {
				v[i] = origin[i] - ent->r.absmax[i];
			} else {
				v[i] = 0;
			}
		}
	}
}

/*
============
G_RadiusDamage2
mutation of G_RadiusDamage which lets us selectively damage only clients or only non clients. 
Used by: WP_POISONGAS
============
*/
qboolean G_RadiusDamage2( vec3_t origin, gentity_t *inflictor, gentity_t *attacker, float damage, float radius, gentity_t *ignore, int mod, RadiusScope scope ) {
	float		points, dist;
	gentity_t	*ent;
	int			entityList[MAX_GENTITIES];
	int			numListedEntities;
	vec3_t		mins, maxs;
	vec3_t		v;
	vec3_t		dir;
	int			i, e;
	qboolean	hitClient = qfalse;
	float		boxradius;
	vec3_t		dest; 
	trace_t		tr;
	vec3_t		midpoint;
	int			flags = DAMAGE_RADIUS;

	if( radius < 1 ) {
		radius = 1;
	}

	boxradius = 1.41421356 * radius; // radius * sqrt(2) for bounding box enlargement -- 
	// bounding box was checking against radius / sqrt(2) if collision is along box plane
	for( i = 0 ; i < 3 ; i++ ) {
		mins[i] = origin[i] - boxradius;
		maxs[i] = origin[i] + boxradius;
	}

	numListedEntities = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );

	for( e = 0 ; e < numListedEntities ; e++ ) {
		ent = &g_entities[entityList[ e ]];

		if( ent == ignore ) {
			continue;
		}
		if( !ent->takedamage && ( !ent->dmgparent || !ent->dmgparent->takedamage )) {
			continue;
		}

        switch (scope) {
            default:
            case RADIUS_SCOPE_ANY:
                break;

            case RADIUS_SCOPE_CLIENTS:
		        if (!ent->client) //&& ent->s.eType != ET_CORPSE )
                    continue;
                break;
            case RADIUS_SCOPE_NOCLIENTS:
		        if (ent->client)
                    continue;
                break;
            case RADIUS_SCOPE_AI:
				if (!ent->aiCharacter)
					continue; 
				if (ent->health <= 0)
					continue;
				break;
		}

		if(	ent->waterlevel == 3 && mod == MOD_POISONGAS) {
			continue;
		}

		G_AdjustedDamageVec( ent, origin, v );

		dist = VectorLength( v );
		if ( dist >= radius ) {
			continue;
		}

		points = damage * ( 1.0 - dist / radius );

		if( CanDamage( ent, origin ) ) {
			if( ent->dmgparent ) {
				ent = ent->dmgparent;
			}

			if( LogAccuracyHit( ent, attacker ) ) {
				hitClient = qtrue;
			}
			//VectorSubtract (ent->r.currentOrigin, origin, dir);
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			//dir[2] += 24;

			G_Damage( ent, inflictor, attacker, dir, origin, (int)points, flags, mod );
		} else {
			VectorAdd( ent->r.absmin, ent->r.absmax, midpoint );
			VectorScale( midpoint, 0.5, midpoint );
			VectorCopy( midpoint, dest );

			trap_Trace( &tr, origin, vec3_origin, vec3_origin, dest, ENTITYNUM_NONE, MASK_SOLID );
			if( tr.fraction < 1.0 ) {
				VectorSubtract( dest, origin, dest );
				dist = VectorLength( dest );
				if( dist < radius * 0.2f ) { // closer than 1/4 dist
					if( ent->dmgparent ) {
						ent = ent->dmgparent;
					}

					if( LogAccuracyHit( ent, attacker ) ) {
						hitClient = qtrue;
					}
					VectorSubtract (ent->r.currentOrigin, origin, dir);
					dir[2] += 24;
					G_Damage( ent, inflictor, attacker, dir, origin, (int)(points*0.1f), flags, mod );
				}
			}
		}
	}

	return hitClient;
}
