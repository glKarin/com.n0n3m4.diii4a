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

/*

  Items are any object that a player can touch to gain some effect.

  Pickup will return the number of seconds until they should respawn.

  all items should pop when dropped in lava or slime

  Respawnable items don't actually go away when picked up, they are
  just made invisible and untouchable.  This allows them to ride
  movers and respawn apropriately.
*/


#define	RESPAWN_ARMOR		g_armorrespawn.value
#define	RESPAWN_HEALTH		g_healthrespawn.value
#define	RESPAWN_AMMO		g_ammorespawn.value
#define	RESPAWN_HOLDABLE	g_holdablerespawn.value
#define	RESPAWN_MEGAHEALTH	g_megahealthrespawn.value//120
#define	RESPAWN_POWERUP		g_poweruprespawn.value

int Pickup_Powerup( gentity_t *ent, gentity_t *other ) {
	int			quantity;
	int			i;
	gclient_t	*client;

	if ( !other->client->ps.powerups[ent->item->giTag] ) {
		// round timing to seconds to make multiple powerup timers
		// count in sync
		other->client->ps.powerups[ent->item->giTag] =
			level.time - ( level.time % 1000 );
	}

	if ( ent->count ) {
		quantity = ent->count;
	}
	else {
		quantity = ent->item->quantity;
		other->client->ps.powerups[ent->item->giTag] += ent->count * 1000;
	}
		if (ent->item->giTag == PW_QUAD){
		if ( !ent->count )
	quantity = mod_quadtime;
	}
		if (ent->item->giTag == PW_BATTLESUIT){
		if ( !ent->count )
	quantity = mod_bsuittime;
	}
		if (ent->item->giTag == PW_HASTE){
		if ( !ent->count )
	quantity = mod_hastetime;
	}
		if (ent->item->giTag == PW_INVIS){
		if ( !ent->count )
	quantity = mod_invistime;
	}
		if (ent->item->giTag == PW_REGEN){
		if ( !ent->count )
	quantity = mod_regentime;
	}
		if (ent->item->giTag == PW_FLIGHT){
		if ( !ent->count )
	quantity = mod_flighttime;
	}

	other->client->ps.powerups[ent->item->giTag] += quantity * 1000;

	// give any nearby players a "denied" anti-reward
	for ( i = 0 ; i < level.maxclients ; i++ ) {
		vec3_t		delta;
		float		len;
		vec3_t		forward;
		trace_t		tr;

		client = &level.clients[i];
		if ( client == other->client ) {
			continue;
		}
		if ( client->pers.connected == CON_DISCONNECTED ) {
			continue;
		}
		if ( client->ps.stats[STAT_HEALTH] <= 0 ) {
			continue;
		}

    // if same team in team game, no sound
    // cannot use OnSameTeam as it expects to g_entities, not clients
  	if ( g_gametype.integer >= GT_TEAM && g_ffa_gt==0 && other->client->sess.sessionTeam == client->sess.sessionTeam  ) {
      continue;
    }

		// if too far away, no sound
		VectorSubtract( ent->s.pos.trBase, client->ps.origin, delta );
		len = VectorNormalize( delta );
		if ( len > 192 ) {
			continue;
		}

		// if not facing, no sound
		AngleVectors( client->ps.viewangles, forward, NULL, NULL );
		if ( DotProduct( delta, forward ) < 0.4 ) {
			continue;
		}

		// if not line of sight, no sound
		trap_Trace( &tr, client->ps.origin, NULL, NULL, ent->s.pos.trBase, ENTITYNUM_NONE, CONTENTS_SOLID );
		if ( tr.fraction != 1.0 ) {
			continue;
		}

		// anti-reward
		client->ps.persistant[PERS_PLAYEREVENTS] ^= PLAYEREVENT_DENIEDREWARD;
	}
	return RESPAWN_POWERUP;
}

int Pickup_PersistantPowerup( gentity_t *ent, gentity_t *other ) {
	int		clientNum;
	char	userinfo[MAX_INFO_STRING];
	float	handicap;
	int		max;

	other->client->ps.stats[STAT_PERSISTANT_POWERUP] = ent->item - bg_itemlist;
	other->client->persistantPowerup = ent;

	clientNum = other->client->ps.clientNum;
	trap_GetUserinfo( clientNum, userinfo, sizeof(userinfo) );
	handicap = atof( Info_ValueForKey( userinfo, "handicap" ) );
	if (!(other->r.svFlags & SVF_BOT)){
			if( handicap<=0.0f || handicap>100.0f) {
				handicap = 100.0f;
			}
	}

	switch( ent->item->giTag ) {
	case PW_GUARD:
		if (g_guardhealthmodifier.value > 0){

		max = (int)(g_guardhealthmodifier.value * handicap * 1);

		other->health = max;
		other->client->ps.stats[STAT_HEALTH] = max;
		other->client->ps.stats[STAT_MAX_HEALTH] = max;
		other->client->ps.stats[STAT_ARMOR] = max;
		other->client->pers.maxHealth = max;
		}

		break;

	case PW_SCOUT:
		if (g_scouthealthmodifier.value > 0){

		max = (int)(g_scouthealthmodifier.value *  handicap * 1);

		other->health = max;
		other->client->ps.stats[STAT_HEALTH] = max;
		other->client->ps.stats[STAT_MAX_HEALTH] = max;
		other->client->ps.stats[STAT_ARMOR] = max;
		other->client->pers.maxHealth = max;
		}

		break;

	case PW_DOUBLER:
		if(g_doublerhealthmodifier.value > 0){

		max = (int)(g_doublerhealthmodifier.value *  handicap * 1);

		other->health = max;
		other->client->ps.stats[STAT_HEALTH] = max;
		other->client->ps.stats[STAT_MAX_HEALTH] = max;
		other->client->ps.stats[STAT_ARMOR] = max;
		other->client->pers.maxHealth = max;
		}

		break;
	case PW_AMMOREGEN:
		if(g_ammoregenhealthmodifier.value > 0){

		max = (int)(g_ammoregenhealthmodifier.value *  handicap * 1);

		other->health = max;
		other->client->ps.stats[STAT_HEALTH] = max;
		other->client->ps.stats[STAT_MAX_HEALTH] = max;
		other->client->ps.stats[STAT_ARMOR] = max;
		other->client->pers.maxHealth = max;
		}

	default:
		other->client->pers.maxHealth = handicap;
		break;
	}

	return -1;
}

int Pickup_Holdable( gentity_t *ent, gentity_t *other ) {
	int i;

	//other->client->ps.stats[STAT_HOLDABLE_ITEM] = ent->item - bg_itemlist;
	other->client->ps.stats[STAT_HOLDABLE_ITEM] |= (1 << ent->item->giTag);
	
	//set teleportation target if player picks up a personal teleporter with fixed teleporter target
	if ( ent->item->giTag == HI_TELEPORTER && ent->teleporterTarget ) {
		other->teleporterTarget = ent->teleporterTarget;
        }



	//other->client->ps.stats[STAT_HOLDABLE_ITEM] = ent->item - bg_itemlist;

	if( ent->item->giTag == HI_KAMIKAZE ) {
		other->client->ps.eFlags |= EF_KAMIKAZE;
	}

	return RESPAWN_HOLDABLE;
}

int Pickup_Backpack( gentity_t *ent, gentity_t *other) {
	int i;

	//weapons
	for(i = 1; i < WEAPONS_NUM; i++){
		other->swep_list[i] = ent->backpackContentsList[i];
		other->swep_ammo[i] = ent->backpackContentsAmmo[i];
	}

	//holdables
	other->client->ps.stats[STAT_HOLDABLE_ITEM] = ent->backpackContentsList[0];

	return -1;
}

void Set_Weapon (gentity_t *ent, int weapon, int status)
{
	if(status == 1){
	ent->swep_list[weapon] = 1;
	} else {
	ent->swep_list[weapon] = 0;
	}
}

void Add_Ammo (gentity_t *ent, int weapon, int count)
{
	ent->swep_ammo[weapon] += count;
	if ( ent->swep_ammo[weapon] > mod_ammolimit && count != 9999 ) {
		ent->swep_ammo[weapon] = mod_ammolimit;
	}
}

void Set_Ammo (gentity_t *ent, int weapon, int count)
{
	ent->swep_ammo[weapon] = count;
}

int Pickup_Ammo (gentity_t *ent, gentity_t *other)
{
	int		quantity;

	if ( ent->count ) {
		quantity = ent->count;
	} else {
		quantity = ent->item->quantity;

    switch(ent->item->giTag)
	{
			case WP_MACHINEGUN:
					if(g_mgammocount.integer != -1){
				 	quantity = g_mgammocount.integer;
					}
				break;
			case WP_SHOTGUN:
					if(g_sgammocount.integer != -1){
					quantity = g_sgammocount.integer;
					}
				break;
			case WP_GRENADE_LAUNCHER:
					if(g_glammocount.integer != -1){
					quantity = g_glammocount.integer;
					}
				break;
			case WP_ROCKET_LAUNCHER:
					if(g_rlammocount.integer != -1){
					quantity = g_rlammocount.integer;
					}
				break;
			case WP_PLASMAGUN:
					if(g_pgammocount.integer != -1){
					quantity = g_pgammocount.integer;
					}
				break;
			case WP_RAILGUN:
					if(g_rgammocount.integer != -1){
					quantity = g_rgammocount.integer;
					}
				break;
			case WP_LIGHTNING:
					if(g_lgammocount.integer != -1){
					quantity = g_lgammocount.integer;
					}
				break;
			case WP_BFG:
					if(g_bfgammocount.integer != -1){
					quantity = g_bfgammocount.integer;
					}
				break;
			case WP_NAILGUN:
					if(g_ngammocount.integer != -1){
					quantity = g_ngammocount.integer;
					}
				break;
			case WP_PROX_LAUNCHER:
					if(g_plammocount.integer != -1){
					quantity = g_plammocount.integer;
					}
				break;
			case WP_CHAINGUN:
					if(g_cgammocount.integer != -1){
					quantity = g_cgammocount.integer;
					}
				break;
			case WP_FLAMETHROWER:
					if(g_ftammocount.integer != -1){
					quantity = g_ftammocount.integer;
					}
				break;
	}
    //end
	}
	Add_Ammo (other, ent->item->giTag, quantity);

	return RESPAWN_AMMO;
}

int Pickup_Weapon (gentity_t *ent, gentity_t *other) {
	int		quantity;



	if ( ent->count < 0 ) {
		quantity = 0; // None for you, sir!
	} else {
		if ( ent->count ) {
			quantity = ent->count;
		} else {
			quantity = ent->item->quantity;


    switch(ent->item->giTag)
	{
			case WP_MACHINEGUN:
					if(g_mgweaponcount.integer != -1){
				 	quantity = g_mgweaponcount.integer;
					}
				break;
			case WP_SHOTGUN:
					if(g_sgweaponcount.integer != -1){
					quantity = g_sgweaponcount.integer;
					}
				break;
			case WP_GRENADE_LAUNCHER:
					if(g_glweaponcount.integer != -1){
					quantity = g_glweaponcount.integer;
					}
				break;
			case WP_ROCKET_LAUNCHER:
					if(g_rlweaponcount.integer != -1){
					quantity = g_rlweaponcount.integer;
					}
				break;
			case WP_PLASMAGUN:
					if(g_pgweaponcount.integer != -1){
					quantity = g_pgweaponcount.integer;
					}
				break;
			case WP_RAILGUN:
					if(g_rgweaponcount.integer != -1){
					quantity = g_rgweaponcount.integer;
					}
				break;
			case WP_LIGHTNING:
					if(g_lgweaponcount.integer != -1){
					quantity = g_lgweaponcount.integer;
					}
				break;
			case WP_BFG:
					if(g_bfgweaponcount.integer != -1){
					quantity = g_bfgweaponcount.integer;
					}
				break;
			case WP_NAILGUN:
					if(g_ngweaponcount.integer != -1){
					quantity = g_ngweaponcount.integer;
					}
				break;
			case WP_PROX_LAUNCHER:
					if(g_plweaponcount.integer != -1){
					quantity = g_plweaponcount.integer;
					}
				break;
			case WP_CHAINGUN:
					if(g_cgweaponcount.integer != -1){
					quantity = g_cgweaponcount.integer;
					}
				break;
			case WP_FLAMETHROWER:
					if(g_ftweaponcount.integer != -1){
					quantity = g_ftweaponcount.integer;
					}
				break;
			case WP_ANTIMATTER:
					if(g_amweaponcount.integer != -1){
					quantity = g_amweaponcount.integer;
					}
				break;
	}
    //end
		}
		// dropped items and teamplay weapons always have full ammo
		if ( ! (ent->flags & FL_DROPPED_ITEM) && g_gametype.integer != GT_TEAM ) {
			// respawning rules
			// drop the quantity if the already have over the minimum
			if ( other->swep_ammo[ent->item->giTag] < quantity ) {
				quantity = quantity - other->swep_ammo[ent->item->giTag];
			} else {
				if(g_maxweaponpickup.integer == 1){
				quantity /= 2;
				}
				if(g_maxweaponpickup.integer != 1){
					quantity = g_maxweaponpickup.integer;		// only add a single shot
				}
			}
		}
		
	}

	// add the weapon
	Set_Weapon( other, ent->item->giTag, 1 );
	Add_Ammo( other, ent->item->giTag, quantity );

	SetUnlimitedWeapons(other);

	// team deathmatch has slow weapon respawns
	if ( g_gametype.integer == GT_TEAM ) {
		return g_weaponTeamRespawn.integer;
	}

	return g_weaponRespawn.integer;
}

int Pickup_Health (gentity_t *ent, gentity_t *other) {
	int			max;
	int			quantity;


        if( !other->client)
            return RESPAWN_HEALTH;

	// small and mega healths will go over the max
	if( other->client && bg_itemlist[other->client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_GUARD ) {
		max = other->client->ps.stats[STAT_MAX_HEALTH];
	}
	else
	if ( ent->item->quantity != 5 && ent->item->quantity != 100 ) {
		max = other->client->ps.stats[STAT_MAX_HEALTH];
	} else {
		max = other->client->ps.stats[STAT_MAX_HEALTH] * 2;
	}

	if ( ent->count ) {
		quantity = ent->count;
	} else {
		quantity = ent->item->quantity;
	}

	other->health += quantity;

	if (other->health > max ) {
		other->health = max;
	}
	other->client->ps.stats[STAT_HEALTH] = other->health;

	if ( ent->item->quantity == 100 ) {		// mega health respawns slow
		return RESPAWN_MEGAHEALTH;
	}

	return RESPAWN_HEALTH;
}

int Pickup_Armor( gentity_t *ent, gentity_t *other ) {
	int		upperBound;



	other->client->ps.stats[STAT_ARMOR] += ent->item->quantity;

	if( other->client && bg_itemlist[other->client->ps.stats[STAT_PERSISTANT_POWERUP]].giTag == PW_GUARD ) {
		upperBound = other->client->ps.stats[STAT_MAX_HEALTH];
	}
	else {
		upperBound = other->client->ps.stats[STAT_MAX_HEALTH] * 2;
	}

	if ( other->client->ps.stats[STAT_ARMOR] > upperBound ) {
		other->client->ps.stats[STAT_ARMOR] = upperBound;
	}

	return RESPAWN_ARMOR;
}

/*
===============
RespawnItem
===============
*/
void RespawnItem( gentity_t *ent ) {
	int		spawn_item;

    if(g_randomItems.integer) {
		spawn_item = rq3_random(1, 56);

		if(spawn_item == 8){
		spawn_item = 55;
		}
		if(spawn_item == 34){
		spawn_item = 55;
		}
		if(spawn_item == 35){
		spawn_item = 55;
		}
		if(spawn_item == 46){
		spawn_item = 55;
		}
		if(spawn_item == 47){
		spawn_item = 55;
		}
		if(spawn_item == 48){
		spawn_item = 55;
		}
		ent->item = &bg_itemlist[spawn_item];
		ent->item->classname = bg_itemlist[spawn_item].classname;
		ent->s.modelindex = spawn_item;
	}
	//end

	// randomly select from teamed entities
	if (ent->team) {
		gentity_t	*master;
		int	count;
		int choice;

		if ( !ent->teammaster ) {
			G_Printf( "RespawnItem: bad teammaster");
		}
		master = ent->teammaster;

		for (count = 0, ent = master; ent; ent = ent->teamchain, count++)
			;

		choice = (count > 0)? rand()%count : 0;

		for (count = 0, ent = master; count < choice; ent = ent->teamchain, count++)
			;
	}

	ent->r.contents = CONTENTS_TRIGGER;
	ent->s.eFlags &= ~EF_NODRAW;
	ent->r.svFlags &= ~SVF_NOCLIENT;
	trap_LinkEntity (ent);

	if ( ent->item->giType == IT_POWERUP ) {
		// play powerup spawn sound to all clients
		gentity_t	*te;

		// if the powerup respawn sound should Not be global
		if (ent->speed) {
			te = G_TempEntity( ent->s.pos.trBase, EV_GENERAL_SOUND );
		}
		else {
			te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_SOUND );
		}
		te->s.eventParm = G_SoundIndex( "sound/items/poweruprespawn.wav" );
		te->r.svFlags |= SVF_BROADCAST;
	}

	if ( ent->item->giType == IT_HOLDABLE && ent->item->giTag == HI_KAMIKAZE ) {
		// play powerup spawn sound to all clients
		gentity_t	*te;

		// if the powerup respawn sound should Not be global
		if (ent->speed) {
			te = G_TempEntity( ent->s.pos.trBase, EV_GENERAL_SOUND );
		}
		else {
			te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_SOUND );
		}
		te->s.eventParm = G_SoundIndex( "sound/items/kamikazerespawn.wav" );
		te->r.svFlags |= SVF_BROADCAST;
	}

	// play the normal respawn sound only to nearby clients
	G_AddEvent( ent, EV_ITEM_RESPAWN, 0 );

	ent->nextthink = 0;
}

/*
===============
RespawnItemCtf
===============
*/
void RespawnItemCtf( gentity_t *ent ) {
	int		spawn_item;

	// randomly select from teamed entities
	if (ent->team) {
		gentity_t	*master;
		int	count;
		int choice;

		if ( !ent->teammaster ) {
			G_Printf( "RespawnItem: bad teammaster");
		}
		master = ent->teammaster;

		for (count = 0, ent = master; ent; ent = ent->teamchain, count++)
			;

		choice = (count > 0)? rand()%count : 0;

		for (count = 0, ent = master; count < choice; ent = ent->teamchain, count++)
			;
	}

	ent->r.contents = CONTENTS_TRIGGER;
	ent->s.eFlags &= ~EF_NODRAW;
	ent->r.svFlags &= ~SVF_NOCLIENT;
	trap_LinkEntity (ent);

	if ( ent->item->giType == IT_POWERUP ) {
		// play powerup spawn sound to all clients
		gentity_t	*te;

		// if the powerup respawn sound should Not be global
		if (ent->speed) {
			te = G_TempEntity( ent->s.pos.trBase, EV_GENERAL_SOUND );
		}
		else {
			te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_SOUND );
		}
		te->s.eventParm = G_SoundIndex( "sound/items/poweruprespawn.wav" );
		te->r.svFlags |= SVF_BROADCAST;
	}

	if ( ent->item->giType == IT_HOLDABLE && ent->item->giTag == HI_KAMIKAZE ) {
		// play powerup spawn sound to all clients
		gentity_t	*te;

		// if the powerup respawn sound should Not be global
		if (ent->speed) {
			te = G_TempEntity( ent->s.pos.trBase, EV_GENERAL_SOUND );
		}
		else {
			te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_SOUND );
		}
		te->s.eventParm = G_SoundIndex( "sound/items/kamikazerespawn.wav" );
		te->r.svFlags |= SVF_BROADCAST;
	}

	// play the normal respawn sound only to nearby clients
	G_AddEvent( ent, EV_ITEM_RESPAWN, 0 );

	ent->nextthink = 0;
}

/*
===============
Touch_Item
===============
*/
void Touch_Item(gentity_t *ent, gentity_t *other, trace_t *trace) {
	Touch_Item2(ent, other, trace, qfalse);
}

void Touch_Item2 (gentity_t *ent, gentity_t *other, trace_t *trace, qboolean allowBot ) {
	int			respawn;
	qboolean	predict;

		if(ent->singlebot){
		if(!G_NpcFactionProp(NP_PICKUP, ent)){
		return;		// npc can't pickup
		}
		}
		
		if(other->client->sess.sessionTeam == TEAM_BLUE){
		if(g_teamblue_pickupitems.integer == 0){
		return;
		}
		}
		
		if(other->client->sess.sessionTeam == TEAM_RED){
		if(g_teamred_pickupitems.integer == 0){
		return;
		}
		}
	
	//instant gib
	if (g_elimination_items.integer == 0){
	if ((g_gametype.integer == GT_CTF_ELIMINATION || g_elimination_allgametypes.integer)
                && ent->item->giType != IT_TEAM)
		return;
	}

	//Cannot touch flag before round starts
	if(g_gametype.integer == GT_CTF_ELIMINATION && level.roundNumber != level.roundNumberStarted)
		return;

	//Cannot take ctf elimination oneway
	if(g_gametype.integer == GT_CTF_ELIMINATION && g_elimination_ctf_oneway.integer!=0 && (
			(other->client->sess.sessionTeam==TEAM_BLUE && (level.eliminationSides+level.roundNumber)%2 == 0 ) ||
			(other->client->sess.sessionTeam==TEAM_RED && (level.eliminationSides+level.roundNumber)%2 != 0 ) ))
		return;
	if (g_elimination_items.integer == 0)
	if (g_gametype.integer == GT_ELIMINATION || g_gametype.integer == GT_LMS)
		return;		//nothing to pick up in elimination

	if (!other->client)
		return;
	if (other->health < 1)
		return;		// dead people can't pickup

	/*if ( !allowBot && IsBot(other) )
		return;		// bots don't pick up items in entityplus*/

	// the same pickup rules are used for client side and server side
	if ( !BG_CanItemBeGrabbed( g_gametype.integer, &ent->s, &other->client->ps ) ) {
		return;
	}

	//In double DD we cannot "pick up" a flag we already got
	if(g_gametype.integer == GT_DOUBLE_D) {
		if( strcmp(ent->classname, "team_CTF_redflag") == 0 )
			if(other->client->sess.sessionTeam == level.pointStatusA)
				return;
		if( strcmp(ent->classname, "team_CTF_blueflag") == 0 )
			if(other->client->sess.sessionTeam == level.pointStatusB)
				return;
	}

	G_LogPrintf( "Item: %i %s\n", other->s.number, ent->item->classname );

	predict = other->client->pers.predictItemPickup;

	// call the item-specific pickup function
	switch( ent->item->giType ) {
	case IT_WEAPON:
		respawn = Pickup_Weapon(ent, other);
//		predict = qfalse;
		break;
	case IT_AMMO:
		respawn = Pickup_Ammo(ent, other);
//		predict = qfalse;
		break;
	case IT_ARMOR:
		respawn = Pickup_Armor(ent, other);
		break;
	case IT_HEALTH:
		respawn = Pickup_Health(ent, other);
		break;
	case IT_POWERUP:
		respawn = Pickup_Powerup(ent, other);
		predict = qfalse;
		break;
	case IT_PERSISTANT_POWERUP:
		respawn = Pickup_PersistantPowerup(ent, other);
		break;
	case IT_TEAM:
		respawn = Pickup_Team(ent, other);
                //If touching a team item remove spawnprotection
                if(other->client->spawnprotected)
                    other->client->spawnprotected = qfalse;
		break;
	case IT_HOLDABLE:
		respawn = Pickup_Holdable(ent, other);
		break;
	case IT_BACKPACK:
		respawn = Pickup_Backpack(ent, other);
		break;
	default:
		return;
	}

	if ( !respawn ) {
		return;
	}

	// play the normal pickup sound
	if (!(ent->spawnflags & 2)) {
	if (predict) {
		G_AddPredictableEvent( other, EV_ITEM_PICKUP, ent->s.modelindex );
	} else {
		G_AddEvent( other, EV_ITEM_PICKUP, ent->s.modelindex );
	}

	// powerup pickups are global broadcasts
	if ( /*ent->item->giType == IT_POWERUP ||*/ ent->item->giType == IT_TEAM) {	//disabled powerup sound for all cuz it annoying
		// if we want the global sound to play
		if (!ent->speed) {
			gentity_t	*te;

			te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_ITEM_PICKUP );
			te->s.eventParm = ent->s.modelindex;
			te->r.svFlags |= SVF_BROADCAST;
		} else {
			gentity_t	*te;

			te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_ITEM_PICKUP );
			te->s.eventParm = ent->s.modelindex;
			// only send this temp entity to a single client
			te->r.svFlags |= SVF_SINGLECLIENT;
			te->r.singleClient = other->s.number;
		}
	}
	}

	

	// fire item targets
	G_UseTargets (ent, other);
if(g_gametype.integer == GT_SINGLE){
	// items with no specified respawn will not respawn in entityplus
	if ( !ent->wait )
		ent->wait = -1;
}

	// wait of -1 will not respawn
	if ( ent->wait == -1 ) {
		ent->r.svFlags |= SVF_NOCLIENT;
		ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		ent->unlinkAfterEvent = qtrue;

		G_FreeEntity( ent );	//completely free the entity. It no longer serves a purpose.
		return;
	}

	// non zero wait overrides respawn time
	if ( ent->wait ) {
		respawn = ent->wait;
	}

	// random can be used to vary the respawn time
	if ( ent->random ) {
		respawn += crandom() * ent->random;
		if ( respawn < 1 ) {
			respawn = 1;
		}
	}

	// dropped items will not respawn
	if ( ent->flags & FL_DROPPED_ITEM ) {
		ent->freeAfterEvent = qtrue;
	}

	// picked up items still stay around, they just don't
	// draw anything.  This allows respawnable items
	// to be placed on movers.
	ent->r.svFlags |= SVF_NOCLIENT;
	ent->s.eFlags |= EF_NODRAW;
	ent->r.contents = 0;

	// ZOID
	// A negative respawn times means to never respawn this item (but don't
	// delete it).  This is used by items that are respawned by third party
	// events such as ctf flags
	if ( respawn <= 0 ) {
		ent->nextthink = 0;
		ent->think = 0;
	} else {
		ent->nextthink = level.time + respawn * 1000;
		ent->think = RespawnItem;
	}
	trap_LinkEntity( ent );
}

/*
================
LaunchItem

Spawns an item and tosses it forward
================
*/
gentity_t *LaunchItem( gitem_t *item, vec3_t origin, vec3_t velocity ) {
	gentity_t	*dropped;

	dropped = G_Spawn();

	dropped->s.eType = ET_ITEM;
	dropped->s.modelindex = item - bg_itemlist;	// store item number in modelindex
	dropped->s.modelindex2 = 1; // This is non-zero is it's a dropped item

	dropped->classname = item->classname;
	dropped->item = item;
	VectorSet (dropped->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS);
	VectorSet (dropped->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS);
	dropped->r.contents = CONTENTS_TRIGGER;

	dropped->touch = Touch_Item;

	G_SetOrigin( dropped, origin );
	dropped->s.pos.trType = TR_GRAVITY;
	dropped->s.pos.trTime = level.time;
	VectorCopy( velocity, dropped->s.pos.trDelta );

	dropped->s.eFlags |= EF_BOUNCE_HALF;
	if ((g_gametype.integer == GT_CTF || g_gametype.integer == GT_1FCTF || g_gametype.integer == GT_CTF_ELIMINATION || g_gametype.integer == GT_DOUBLE_D)			&& item->giType == IT_TEAM) { // Special case for CTF flags
		dropped->think = Team_DroppedFlagThink;
		dropped->nextthink = level.time + g_autoflagreturn.integer*1000;
		Team_CheckDroppedItem( dropped );
	} else { // auto-remove after 30 seconds
	if ( item->giType != IT_BACKPACK ) { // auto-remove after 30 seconds if it's not a backpack
		dropped->think = G_FreeEntity;
		dropped->nextthink = level.time + g_droppeditemtime.integer*1000;
}
	}

	dropped->flags = FL_DROPPED_ITEM;

	trap_LinkEntity (dropped);

	return dropped;
}

void BackpackThink(gentity_t* self) {
	gentity_t* ent2;
	
	/*(
	ent2 = G_TempEntity(self->r.currentOrigin, EV_PARTICLES_LINEAR_UP);
	ent2->s.constantLight = (255 << 8);	//constantLight is used to determine particle color
	ent2->s.eventParm = 25; //eventParm is used to determine the number of particles
	ent2->s.generic1 = 50; //generic1 is used to determine the speed of the particles
	*/

	ent2 = G_TempEntity(self->r.currentOrigin, EV_SMOKEPUFF);
	ent2->s.constantLight = (255 << 8);
	ent2->s.eventParm = 10;	//eventParm is used to determine the amount of time the smoke puff exists
	ent2->s.generic1 = 16;	//generic1 is used to determine the movement speed of the smoke puff
	ent2->s.otherEntityNum = 1 * 32; //otherEntityNum is used to determine the size of the smokepuff. The default is 32.
	ent2->s.angles[2] = 1;

	self->nextthink = level.time + 1000;
}

/*
================
LaunchBackpack

Spawns a backpack and tosses it forward
================
*/
gentity_t *LaunchBackpack( gitem_t *item, gentity_t *self, vec3_t velocity ) {
	gentity_t	*dropped;
	vec3_t		origin;
	int			weapons = 0;
	int			i;

	VectorCopy(self->s.pos.trBase, origin);

	dropped = G_Spawn();

	dropped->s.eType = ET_ITEM;
	dropped->s.modelindex = item - bg_itemlist;	// store item number in modelindex
	dropped->s.modelindex2 = 1; // This is non-zero is it's a dropped item

	dropped->classname = item->classname;
	dropped->item = item;
	VectorSet (dropped->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS);
	VectorSet (dropped->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS);
	dropped->r.contents = CONTENTS_TRIGGER;

	dropped->touch = Touch_Item;

	G_SetOrigin( dropped, origin );
	dropped->s.pos.trType = TR_GRAVITY;
	dropped->s.pos.trTime = level.time;
	VectorCopy( velocity, dropped->s.pos.trDelta );

	dropped->s.eFlags |= EF_BOUNCE_HALF;
	dropped->flags = FL_DROPPED_ITEM;

	// emit ligth
	// dropped->s.constantLight = (255 << 8) | (50 << 24);

	// emit smoke
	// dropped->nextthink = level.time + 1000;
	// dropped->think = BackpackThink;

	trap_LinkEntity (dropped);

	//set contents of backpack
	
	//holdables
	dropped->backpackContentsList[0] = self->client->ps.stats[STAT_HOLDABLE_ITEM];
	
	//weapons
	for(i = 1; i < WEAPONS_NUM; i++){
		dropped->backpackContentsList[i] = self->swep_list[i];
		dropped->backpackContentsAmmo[i] = self->swep_ammo[i];
	}

	return dropped;
}

/*
================
Drop_Item

Spawns an item and tosses it forward
================
*/
gentity_t *Drop_Item( gentity_t *ent, gitem_t *item, float angle ) {
	vec3_t	velocity;
	vec3_t	angles;

	VectorCopy( ent->s.apos.trBase, angles );
	angles[YAW] += angle;
	angles[PITCH] = 0;	// always forward

	AngleVectors( angles, velocity, NULL, NULL );
	VectorScale( velocity, 150, velocity );
	velocity[2] += 200 + crandom() * 50;
	
	if ( item->giType == IT_BACKPACK )
		return LaunchBackpack( item, ent, velocity );
	else
	return LaunchItem( item, ent->s.pos.trBase, velocity );
}

// oatmeal begin

gentity_t *Throw_Item( gentity_t *ent, gitem_t *item, float angle ) {
	vec3_t	velocity;
	vec3_t	angles;

	VectorCopy( ent->s.apos.trBase, angles );
	angles[YAW] += angle;
	angles[PITCH] = 0;	// always forward

	AngleVectors( angles, velocity, NULL, NULL );
	VectorScale( velocity, 300, velocity );
	velocity[2] += 300 + crandom() * 50;

	ent->wait_to_pickup = level.time + 1000;
	ent->client->ps.stats[STAT_NO_PICKUP] = 1;

	return LaunchItem( item, ent->s.pos.trBase, velocity );
}

// oatmeal end

/*
================
Use_Item

Respawn the item
================
*/
void Use_Item( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	RespawnItem( ent );
}

/*
================
FinishSpawningItem

Traces down to find where an item should rest, instead of letting them
free fall from their spawn points
================
*/
void FinishSpawningItem( gentity_t *ent ) {
	trace_t		tr;
	vec3_t		dest;

	VectorSet( ent->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS );
	VectorSet( ent->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS );

	ent->s.eType = ET_ITEM;
	ent->s.modelindex = ent->item - bg_itemlist;		// store item number in modelindex
	ent->s.modelindex2 = 0; // zero indicates this isn't a dropped item

	ent->r.contents = CONTENTS_TRIGGER;
	ent->touch = Touch_Item;
	// useing an item causes it to respawn
	ent->use = Use_Item;

	if ( ent->spawnflags & 1 ) {
		// suspended
		G_SetOrigin( ent, ent->s.origin );
	} else {
		// drop to floor
		VectorSet( dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 4096 );
		trap_Trace( &tr, ent->s.origin, ent->r.mins, ent->r.maxs, dest, ent->s.number, MASK_SOLID );
		if ( tr.startsolid ) {
			G_Printf ("FinishSpawningItem: %s startsolid at %s\n", ent->classname, vtos(ent->s.origin));
			G_FreeEntity( ent );
			return;
		}

		// allow to ride movers
		ent->s.groundEntityNum = tr.entityNum;

		G_SetOrigin( ent, tr.endpos );
	}

	// team slaves and targeted items aren't present at start
	if ( ( ent->flags & FL_TEAMSLAVE ) || ent->targetname ) {
		ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		return;
	}


	// powerups don't spawn in for a while (but not in elimination)
	if(g_gametype.integer != GT_ELIMINATION && g_gametype.integer != GT_CTF_ELIMINATION && g_gametype.integer != GT_LMS
                && !g_elimination_allgametypes.integer)
	if ( ent->item->giType == IT_POWERUP ) {
		float	respawn;

		respawn = 1;
		ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		ent->nextthink = level.time + respawn * 1000;
		ent->think = RespawnItem;
		return;
	}


	trap_LinkEntity (ent);
}


qboolean	itemRegistered[MAX_ITEMS];

/*
==================
G_CheckTeamItems
==================
*/
void G_CheckTeamItems( void ) {

	// Set up team stuff
	Team_InitGame();

	if( g_gametype.integer == GT_CTF || g_gametype.integer == GT_CTF_ELIMINATION || g_gametype.integer == GT_DOUBLE_D) {
		gitem_t	*item;

		// check for the two flags
		item = BG_FindItem( "Red Flag" );
		if ( !item || !itemRegistered[ item - bg_itemlist ] ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_CTF_redflag in map\n" );
		}
		item = BG_FindItem( "Blue Flag" );
		if ( !item || !itemRegistered[ item - bg_itemlist ] ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_CTF_blueflag in map\n" );
		}
	}
	if( g_gametype.integer == GT_1FCTF ) {
		gitem_t	*item;

		// check for all three flags
		item = BG_FindItem( "Red Flag" );
		if ( !item || !itemRegistered[ item - bg_itemlist ] ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_CTF_redflag in map\n" );
		}
		item = BG_FindItem( "Blue Flag" );
		if ( !item || !itemRegistered[ item - bg_itemlist ] ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_CTF_blueflag in map\n" );
		}
		item = BG_FindItem( "Neutral Flag" );
		if ( !item || !itemRegistered[ item - bg_itemlist ] ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_CTF_neutralflag in map\n" );
		}
	}

	if( g_gametype.integer == GT_OBELISK ) {
		gentity_t	*ent;

		// check for the two obelisks
		ent = NULL;
		ent = G_Find( ent, FOFS(classname), "team_redobelisk" );
		if( !ent ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_redobelisk in map\n" );
		}

		ent = NULL;
		ent = G_Find( ent, FOFS(classname), "team_blueobelisk" );
		if( !ent ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_blueobelisk in map\n" );
		}
	}

	if( g_gametype.integer == GT_HARVESTER ) {
		gentity_t	*ent;

		// check for all three obelisks
		ent = NULL;
		ent = G_Find( ent, FOFS(classname), "team_redobelisk" );
		if( !ent ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_redobelisk in map\n" );
		}

		ent = NULL;
		ent = G_Find( ent, FOFS(classname), "team_blueobelisk" );
		if( !ent ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_blueobelisk in map\n" );
		}

		ent = NULL;
		ent = G_Find( ent, FOFS(classname), "team_neutralobelisk" );
		if( !ent ) {
			G_Printf( S_COLOR_YELLOW "WARNING: No team_neutralobelisk in map\n" );
		}
	}
}

/*
==============
ClearRegisteredItems
==============
*/
void ClearRegisteredItems( void ) {
	memset( itemRegistered, 0, sizeof( itemRegistered ) );

	// players always start with the base weapon
	RegisterItem( BG_FindItemForWeapon( WP_MACHINEGUN ) );
	RegisterItem( BG_FindItemForWeapon( WP_GAUNTLET ) );
	if(g_gametype.integer == GT_ELIMINATION || g_gametype.integer == GT_CTF_ELIMINATION
                    || g_gametype.integer == GT_LMS || g_elimination_allgametypes.integer)
	{
		RegisterItem( BG_FindItemForWeapon( WP_SHOTGUN ) );
		RegisterItem( BG_FindItemForWeapon( WP_GRENADE_LAUNCHER ) );
		RegisterItem( BG_FindItemForWeapon( WP_ROCKET_LAUNCHER ) );
		RegisterItem( BG_FindItemForWeapon( WP_LIGHTNING ) );
		RegisterItem( BG_FindItemForWeapon( WP_RAILGUN ) );
		RegisterItem( BG_FindItemForWeapon( WP_PLASMAGUN ) );
		RegisterItem( BG_FindItemForWeapon( WP_BFG ) );
		RegisterItem( BG_FindItemForWeapon( WP_NAILGUN ) );
		RegisterItem( BG_FindItemForWeapon( WP_PROX_LAUNCHER ) );
		RegisterItem( BG_FindItemForWeapon( WP_CHAINGUN ) );
		RegisterItem( BG_FindItemForWeapon( WP_FLAMETHROWER ) );
		RegisterItem( BG_FindItemForWeapon( WP_ANTIMATTER ) );
	}
	if( g_gametype.integer == GT_HARVESTER ) {
		RegisterItem( BG_FindItem( "Red Cube" ) );
		RegisterItem( BG_FindItem( "Blue Cube" ) );
	}

	if(g_gametype.integer == GT_DOUBLE_D ) {
		RegisterItem( BG_FindItem( "Point A (Blue)" ) );
		RegisterItem( BG_FindItem( "Point A (Red)" ) );
		RegisterItem( BG_FindItem( "Point A (White)" ) );
		RegisterItem( BG_FindItem( "Point B (Blue)" ) );
		RegisterItem( BG_FindItem( "Point B (Red)" ) );
		RegisterItem( BG_FindItem( "Point B (White)" ) );
	}

	if(g_gametype.integer == GT_DOMINATION ) {
		RegisterItem( BG_FindItem( "Neutral domination point" ) );
		RegisterItem( BG_FindItem( "Red domination point" ) );
		RegisterItem( BG_FindItem( "Blue domination point" ) );
	}

	// precache backpack in entityplus
	RegisterItem( BG_FindItemForBackpack() );
}

/*
===============
RegisterItem

The item will be added to the precache list
===============
*/
void RegisterItem( gitem_t *item ) {
	if ( !item ) {
		G_Printf( "RegisterItem: NULL");
	}
	itemRegistered[ item - bg_itemlist ] = qtrue;
}


/*
===============
SaveRegisteredItems

Write the needed items to a config string
so the client will know which ones to precache
===============
*/
void SaveRegisteredItems( void ) {
	char	string[MAX_ITEMS+1];
	int		i;
	int		count;

	count = 0;
	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( itemRegistered[i] ) {
			count++;
			string[i] = '1';
		} else {
			string[i] = '0';
		}
	}
	string[ bg_numItems ] = 0;

        G_Printf( "%i items registered\n", count );
	trap_SetConfigstring(CS_ITEMS, string);
}

/*
============
G_ItemDisabled
============
*/
int G_ItemDisabled( gitem_t *item ) {

	char name[128];

	Com_sprintf(name, sizeof(name), "disable_%s", item->classname);
	return trap_Cvar_VariableIntegerValue( name );
}

/*
============
G_SpawnItem

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
============
*/
void G_SpawnItem (gentity_t *ent, gitem_t *item) {
	char	buffer[MAX_QPATH];
	char	*s;

	G_SpawnFloat( "random", "0", &ent->random );
	G_SpawnFloat( "wait", "0", &ent->wait );
	
	ent->s.generic1 = ent->spawnflags;	//we want to know spawnflags for muting predicted pickup sounds client-side.

	if(item->giType == IT_TEAM)
	{
		//Don't load pickups in Elimination (or maybe... gives warnings)
			RegisterItem( item );
		//Registrer flags anyway in CTF Elimination:
		if (g_gametype.integer == GT_CTF_ELIMINATION && item->giType == IT_TEAM)
			RegisterItem( item );
		if ( G_ItemDisabled(item) )
			return;
	}

	ent->item = item;
	// some movers spawn on the second frame, so delay item
	// spawns until the third frame so they can ride trains
	ent->nextthink = level.time + FRAMETIME * 2;
	ent->think = FinishSpawningItem;

	ent->physicsBounce = 0.50;		// items are bouncy
	if (g_elimination_items.integer == 0) {
	if (g_gametype.integer == GT_ELIMINATION || g_gametype.integer == GT_LMS ||
			( item->giType != IT_TEAM && (g_elimination_allgametypes.integer || g_gametype.integer==GT_CTF_ELIMINATION) ) ) {
		ent->s.eFlags |= EF_NODRAW; //Invisible in elimination
                ent->r.svFlags |= SVF_NOCLIENT;  //Don't broadcast
        }
	}


	if(g_gametype.integer == GT_DOUBLE_D && (strcmp(ent->classname, "team_CTF_redflag")==0 || strcmp(ent->classname, "team_CTF_blueflag")==0 || strcmp(ent->classname, "team_CTF_neutralflag") == 0 || item->giType == IT_PERSISTANT_POWERUP  ))
		ent->s.eFlags |= EF_NODRAW; //Don't draw the flag models/persistant powerups

	if( g_gametype.integer != GT_1FCTF && strcmp(ent->classname, "team_CTF_neutralflag") == 0)
		ent->s.eFlags |= EF_NODRAW; // Don't draw the flag in CTF_elimination

        if(strcmp(ent->classname, "domination_point") == 0)
                ent->s.eFlags |= EF_NODRAW; // Don't draw domination_point. It is just a pointer to where the Domination points should be placed
	if ( item->giType == IT_POWERUP ) {
		G_SoundIndex( "sound/items/poweruprespawn.wav" );
		G_SpawnFloat( "noglobalsound", "0", &ent->speed);
	}

	if ( item->giType == IT_PERSISTANT_POWERUP ) {
		ent->s.generic1 = ent->spawnflags;
	}
}


/*
================
G_BounceItem

================
*/
void G_BounceItem( gentity_t *ent, trace_t *trace ) {
	vec3_t	velocity;
	float	dot;
	int		hitTime;

	// reflect the velocity on the trace plane
	hitTime = level.previousTime + ( level.time - level.previousTime ) * trace->fraction;
	BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity );
	dot = DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, -2*dot, trace->plane.normal, ent->s.pos.trDelta );

	// cut the velocity to keep from bouncing forever
	VectorScale( ent->s.pos.trDelta, ent->physicsBounce, ent->s.pos.trDelta );

	// check for stop
	if ( trace->plane.normal[2] > 0 && ent->s.pos.trDelta[2] < 40 ) {
		trace->endpos[2] += 1.0;	// make sure it is off ground
		SnapVector( trace->endpos );
		G_SetOrigin( ent, trace->endpos );
		ent->s.groundEntityNum = trace->entityNum;
		return;
	}

	VectorAdd( ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	ent->s.pos.trTime = level.time;
}

/*
================
RespawnBackpack

When a backpack falls into a nodrop area, it will be teleported towards the nearest info_backpack entity.
Returns false if no info_backpack entity to teleport to was found or supplied entity was not a backpack. Otherwise returns true.
================
*/
qboolean TeleportBackpack( gentity_t *backpack ) {
	gentity_t	*spot;
	vec3_t		from, delta;
	float		dist, nearestDist;
	gentity_t	*nearestSpot;

	if ( backpack->item->giType != IT_BACKPACK )
		return qfalse;

	VectorCopy(backpack->r.currentOrigin, from);

	nearestDist = 999999;
	nearestSpot = NULL;
	spot = NULL;

	while ((spot = G_Find (spot, FOFS(classname), "info_backpack")) != NULL) {
		VectorSubtract( spot->s.origin, from, delta );
		dist = VectorLength( delta );
		if ( dist < nearestDist ) {
			nearestDist = dist;
			nearestSpot = spot;
		}
	}

	if (nearestSpot != NULL) {
		G_SetOrigin(backpack, nearestSpot->s.origin);
		backpack->s.pos.trType = TR_GRAVITY;
		return qtrue;
	}

	return qfalse;
}

/*
================
G_RunItem

================
*/
void G_RunItem( gentity_t *ent ) {
	vec3_t		origin;
	trace_t		tr;
	int			contents;
	int			mask;


	// if groundentity has been set to -1, it may have been pushed off an edge
	if ( ent->s.groundEntityNum == -1 ) {
		if ( ent->s.pos.trType != TR_GRAVITY ) {
			ent->s.pos.trType = TR_GRAVITY;
			ent->s.pos.trTime = level.time;
		}
	}

	if ( ent->s.pos.trType == TR_STATIONARY ) {
		// check think function
		G_RunThink( ent );
		return;
	}

	// get current position
	BG_EvaluateTrajectory( &ent->s.pos, level.time, origin );

	// trace a line from the previous position to the current position
	if ( ent->clipmask ) {
		mask = ent->clipmask;
	} else {
		mask = MASK_PLAYERSOLID & ~CONTENTS_BODY;//MASK_SOLID;
	}
	trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin,
		ent->r.ownerNum, mask );

	VectorCopy( tr.endpos, ent->r.currentOrigin );

	if ( tr.startsolid ) {
		tr.fraction = 0;
	}

	trap_LinkEntity( ent );	// FIXME: avoid this for stationary?

	// check think function
	G_RunThink( ent );

	if ( tr.fraction == 1 ) {
		return;
	}

	// if it is in a nodrop volume, remove it
	contents = trap_PointContents( ent->r.currentOrigin, -1 );
	if ( contents & CONTENTS_NODROP ) {
		if (ent->item && ent->item->giType == IT_TEAM) {
			Team_FreeEntity(ent);
		} else {
		if ( !TeleportBackpack( ent ) ) {
			G_FreeEntity( ent );
                }
		}
		return;
	}

	G_BounceItem( ent, &tr );
}

