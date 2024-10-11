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
 * name:		g_items.c
 *
 * desc:		Items are any object that a player can touch to gain some effect.
 *				Pickup will return the number of seconds until they should respawn.
 *				all items should pop when dropped in lava or slime.
 *				Respawnable items don't actually go away when picked up, they are
 *				just made invisible and untouchable.  This allows them to ride
 *				movers and respawn apropriately.
 *
*/
#include <pthread.h>
#include <unistd.h>

#include "g_local.h"

#include "../steam/steam.h"



#define RESPAWN_SP          -1
#define RESPAWN_KEY         4
#define RESPAWN_ARMOR       25
#define RESPAWN_TEAM_WEAPON 30
#define RESPAWN_HEALTH      35
#define RESPAWN_AMMO        40
#define RESPAWN_HOLDABLE    60
#define RESPAWN_PERK        65
#define RESPAWN_MEGAHEALTH  120
#define RESPAWN_POWERUP     120
#define RESPAWN_PARTIAL     998     // for multi-stage ammo/health
#define RESPAWN_PARTIAL_DONE 999    // for multi-stage ammo/health


//======================================================================


void *remove_powerup_after_delay(void *arg) {
    gentity_t *other = (gentity_t *)arg;

    // Sleep for 30 seconds
    sleep(30);

    // Remove the FL_NOTARGET flag
    other->flags &= ~FL_NOTARGET;

    return NULL;
}


int Pickup_Powerup( gentity_t *ent, gentity_t *other ) {
	int quantity;

	if ( !other->client->ps.powerups[ent->item->giTag] ) {

		// some powerups are time based on how long the powerup is /used/
		// rather than timed from when the player picks it up.
		if ( ent->item->giTag == PW_NOFATIGUE ) {
		} else {
			// round timing to seconds to make multiple powerup timers
			// count in sync
			other->client->ps.powerups[ent->item->giTag] = level.time - ( level.time % 1000 );
		}
	}

	// if an amount was specified in the ent, use it
	if ( ent->count ) {
		quantity = ent->count;
	} else {
		quantity = ent->item->quantity;
	}

	other->client->ps.powerups[ent->item->giTag] += quantity * 1000;


	// brandy also gives a little health (10)
	if ( ent->item->giTag == PW_NOFATIGUE ) {
		if ( Q_stricmp( ent->item->classname, "item_stamina_brandy" ) == 0 ) {
			other->health += 10;
			
			other->client->ps.stats[STAT_HEALTH] = other->health;
		}

		// cap stamina
		if ( other->client->ps.powerups[PW_NOFATIGUE] > 60000 ) {
			other->client->ps.powerups[PW_NOFATIGUE] = 60000;
		}
	}
   

    // DIRTY HACK!!!!! If the invisibility powerup is picked up, set FL_NOTARGET and start a timer to remove it
    if (ent->item->giTag == PW_INVIS) {
        other->flags |= FL_NOTARGET;

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, remove_powerup_after_delay, (void *)other);
    }

	if ( ent->s.density == 2 ) {   // multi-stage health first stage
		return RESPAWN_PARTIAL;
	} else if ( ent->s.density == 1 ) {    // last stage, leave the plate
		return RESPAWN_PARTIAL_DONE;
	}

	// single player has no respawns	(SA)
		if ( !( ent->spawnflags & 8 ) ) {
			return RESPAWN_SP;
		}

	return RESPAWN_POWERUP;
}

//----(SA) Wolf keys
//======================================================================
int Pickup_Key( gentity_t *ent, gentity_t *other ) {
	other->client->ps.stats[STAT_KEYS] |= ( 1 << ent->item->giTag );

		if ( !( ent->spawnflags & 8 ) ) {
			return RESPAWN_SP;
		}

	return RESPAWN_KEY;
}



/*
==============
Pickup_Clipboard
==============
*/
int Pickup_Clipboard( gentity_t *ent, gentity_t *other ) {

	if ( ent->spawnflags & 4 ) {
		return 0;   // leave in world

	}
	return -1;
}


/*
==============
Pickup_Treasure
==============
*/
int Pickup_Treasure( gentity_t *ent, gentity_t *other ) {
	gentity_t *player = AICast_FindEntityForName( "player" );
	player->numTreasureFound++;
	G_SendMissionStats();
	return RESPAWN_SP;  // no respawn

	if ( g_gametype.integer == GT_SURVIVAL ) {
	    other->client->ps.persistant[PERS_SCORE] += 50;
	}
}


/*
==============
UseHoldableItem
	server side handling of holdable item use
==============
*/
void UseHoldableItem( gentity_t *ent, int item ) {
	switch ( item ) {
	case HI_WINE:           // 1921 Chateu Lafite - gives 25 pts health up to max health
		ent->health += 25;
		if ( !g_cheats.integer ) 
		{
		steamSetAchievement("ACH_WINE");
		}
		if (!g_decaychallenge.integer){
		if ( ent->health > ent->client->ps.stats[STAT_MAX_HEALTH] ) {
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		}
		}
		break;

	case HI_ADRENALINE:     // Adrenaline 1.0. Health+Stamina
		ent->client->ps.powerups[PW_NOFATIGUE] = 10000;
		ent->health += 100;
		
		if (!g_decaychallenge.integer){
		if ( g_gameskill.integer == GSKILL_REALISM || g_gameskill.integer == GSKILL_MAX ) {
			if ( ent->health > ent->client->ps.stats[STAT_MAX_HEALTH] ) {
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH] * 2.0;
		}
		} else if ( ent->health > ent->client->ps.stats[STAT_MAX_HEALTH] ) {
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH] * 1.25;
		}
		}

		if ( !g_cheats.integer ) 
		{
		steamSetAchievement("ACH_ADRENALINE");
		}

		break;
	case HI_EG_SYRINGE:  // Adrenaline 2.0. Health+Stamina+Speed
	    ent->health += 100;       
		ent->client->ps.powerups[PW_NOFATIGUE] = 15000;

        ent->client->ps.powerups[PW_HASTE] = level.time - ( level.time % 1000 );
		ent->client->ps.powerups[PW_HASTE] += 30 * 1000;
		
		if (!g_decaychallenge.integer){
		if ( g_gameskill.integer == GSKILL_REALISM || g_gameskill.integer == GSKILL_MAX ) {
			if ( ent->health > ent->client->ps.stats[STAT_MAX_HEALTH] ) {
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH] * 2.0;
		}
		} else if ( ent->health > ent->client->ps.stats[STAT_MAX_HEALTH] ) {
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH] * 1.25;
		}
		}

		if ( !g_cheats.integer ) 
		{
		steamSetAchievement("ACH_ADRENALINE");
		}

		break;
	case HI_BG_SYRINGE:       // Adrenaline 3.0. Health+Stamina+Speed+Armor
		ent->health += 100;
		ent->client->ps.powerups[PW_NOFATIGUE] = 15000;       

        ent->client->ps.powerups[PW_HASTE] = level.time - ( level.time % 1000 );
		ent->client->ps.powerups[PW_HASTE] += 30 * 1000;

		ent->client->ps.powerups[PW_BATTLESUIT] = level.time - ( level.time % 1000 );
		ent->client->ps.powerups[PW_BATTLESUIT] += 30 * 1000;

		if (!g_decaychallenge.integer){
		if ( g_gameskill.integer == GSKILL_REALISM || g_gameskill.integer == GSKILL_MAX ) {
			if ( ent->health > ent->client->ps.stats[STAT_MAX_HEALTH] ) {
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH] * 2.0;
		}
		} else if ( ent->health > ent->client->ps.stats[STAT_MAX_HEALTH] ) {
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH] * 1.25;
		}
		}

		if ( !g_cheats.integer ) 
		{
		steamSetAchievement("ACH_ADRENALINE");
		}
		
		break;

	case HI_LP_SYRINGE:       // Adrenaline 4.0. Health+Stamina+Speed+Armor+Acrobatics
	    ent->health += 100;
		ent->client->ps.powerups[PW_NOFATIGUE] = 15000;  

		
        ent->client->ps.powerups[PW_HASTE] = level.time - ( level.time % 1000 );
		ent->client->ps.powerups[PW_HASTE] += 30 * 1000;

		ent->client->ps.powerups[PW_BATTLESUIT] = level.time - ( level.time % 1000 );
		ent->client->ps.powerups[PW_BATTLESUIT] += 30 * 1000;

        ent->client->ps.powerups[PW_FLIGHT] = level.time - ( level.time % 1000 );
		ent->client->ps.powerups[PW_FLIGHT] += 30 * 1000;

		if (!g_decaychallenge.integer){
		if ( g_gameskill.integer == GSKILL_REALISM || g_gameskill.integer == GSKILL_MAX ) {
			if ( ent->health > ent->client->ps.stats[STAT_MAX_HEALTH] ) {
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH] * 2.0;
		}
		} else if ( ent->health > ent->client->ps.stats[STAT_MAX_HEALTH] ) {
			ent->health = ent->client->ps.stats[STAT_MAX_HEALTH] * 1.25;
		}
		}

		if ( !g_cheats.integer ) 
		{
		steamSetAchievement("ACH_ADRENALINE");
		}
		

		break;

	case HI_BANDAGES:       
		ent->health += 20;
		if ( !g_cheats.integer ) 
		{
		steamSetAchievement("ACH_BANDAGES");
		}

		if (!g_decaychallenge.integer){
		if ( ent->health > ent->client->ps.stats[STAT_MAX_HEALTH] ) {
		ent->health = ent->client->ps.stats[STAT_MAX_HEALTH];
		}
		}
		break;

	case HI_BOOK1:
	case HI_BOOK2:
	case HI_BOOK3:
	if ( !g_cheats.integer ) 
	{
	    steamSetAchievement("ACH_READ_BOOK");
	}
		G_AddEvent( ent, EV_POPUPBOOK, ( item - HI_BOOK1 ) + 1 );
		break;
	}
}


//======================================================================

int Pickup_Holdable( gentity_t *ent, gentity_t *other ) {
	gitem_t *item;

//	item = BG_FindItemForHoldable(ent->item);
	item = ent->item;

	if ( item->gameskillnumber[0] ) {  // if the item specifies an amount, give it
		other->client->ps.holdable[item->giTag] += item->gameskillnumber[0];
	} else {
		other->client->ps.holdable[item->giTag] += 1;   // add default of 1

	}
	other->client->ps.holding = item->giTag;

	other->client->ps.stats[STAT_HOLDABLE_ITEM] |= ( 1 << ent->item->giTag );   //----(SA)	added

		if ( !( ent->spawnflags & 8 ) ) {
			return RESPAWN_SP;
		}

	return RESPAWN_HOLDABLE;
}


//======================================================================

int Pickup_Perk( gentity_t *ent, gentity_t *other ) {
	gitem_t *item;

	item = ent->item;

	other->client->ps.perks[item->giTag] += 1;   // add default of 1

	other->client->ps.stats[STAT_PERK] |= ( 1 << ent->item->giTag );   //----(SA)	added

		if ( !( ent->spawnflags & 8 ) ) {
			return RESPAWN_SP;
		}

	return RESPAWN_PERK;
}


//======================================================================

/*
==============
Fill_Clip
	push reserve ammo into available space in the clip
==============
*/
void Fill_Clip( playerState_t *ps, int weapon ) {
	int inclip, maxclip, ammomove;
	int ammoweap = BG_FindAmmoForWeapon( weapon );

	if ( weapon < WP_LUGER || weapon >= WP_NUM_WEAPONS ) {
		return;
	}

	if ( g_dmflags.integer & DF_NO_WEAPRELOAD ) {
		return;
	}

	inclip  = ps->ammoclip[BG_FindClipForWeapon( weapon )];
	maxclip = ammoTable[weapon].maxclip;

	ammomove = maxclip - inclip;    // max amount that can be moved into the clip

	// cap move amount if it's more than you've got in reserve
	if ( ammomove > ps->ammo[ammoweap] ) {
		ammomove = ps->ammo[ammoweap];
	}

	if ( ammomove ) {
		if ( !ps->aiChar || ps->ammo[ammoweap] < 999 ) {  // RF, dont take ammo away if they need unlimited supplies
			ps->ammo[ammoweap] -= ammomove;
		}
		ps->ammoclip[BG_FindClipForWeapon( weapon )] += ammomove;
	}
}

/*
==============
Add_Ammo
	Try to always add ammo here unless you have specific needs
	(like the AI "infinite ammo" where they get below 900 and force back up to 999)

	fillClip will push the ammo straight through into the clip and leave the rest in reserve
==============
*/
void Add_Ammo( gentity_t *ent, int weapon, int count, qboolean fillClip ) {
	int ammoweap = BG_FindAmmoForWeapon( weapon );
	qboolean noPack = qfalse;       // no extra ammo in your 'pack'

	ent->client->ps.ammo[ammoweap] += count;

	switch ( ammoweap ) {
	// some weaps load straight into the 'clip' since they have no storage outside the clip

	case WP_GRENADE_LAUNCHER:       // make sure if he picks up a grenade that he get's the "launcher" too
	case WP_GRENADE_PINEAPPLE:
	case WP_DYNAMITE:
	case WP_POISONGAS:
    case WP_KNIFE:
		COM_BitSet( ent->client->ps.weapons, ammoweap );
	case WP_TESLA:
	case WP_FLAMETHROWER:
	case WP_HOLYCROSS:
		noPack = qtrue;
		break;
	default:
		break;
	}

	if ( fillClip || noPack ) {
		Fill_Clip( &ent->client->ps, weapon );
	}

	if ( ent->aiCharacter ) {
		noPack = qfalse;    // let AI's deal with their own clip/ammo handling

	}
	// cap to max ammo
	if ( noPack ) {
		ent->client->ps.ammo[ammoweap] = 0;
	} else {
		if ( ent->client->ps.ammo[ammoweap] > ammoTable[ammoweap].maxammo ) {
			ent->client->ps.ammo[ammoweap] = ammoTable[ammoweap].maxammo;
		}

		if ( count >= 999 ) { // 'really, give /all/'
			ent->client->ps.ammo[ammoweap] = count;
		}
	}

		switch (ammoweap) {
		case WP_KNIFE:
			ent->client->ps.ammoclip[ammoweap] += count;

			if( ent->client->ps.ammoclip[ammoweap] > ammoTable[ammoweap].maxammo ) {
				ent->client->ps.ammoclip[ammoweap] = ammoTable[ammoweap].maxammo;
			}
			break;
	}

	if ( ent->client->ps.ammoclip[ammoweap] > ammoTable[ammoweap].maxclip ) {
		ent->client->ps.ammoclip[ammoweap] = ammoTable[ammoweap].maxclip;
	}

}

int G_GetWeaponPrice( int weapon );
int G_GetAmmoPrice( int weapon );

/*
==============
Pickup_Ammo
==============
*/
int Pickup_Ammo( gentity_t *ent, gentity_t *other ) {
	int quantity;


			if (g_decaychallenge.integer) {
			quantity = 999;
		}

	

	if ( ent->count ) {
		quantity = ent->count;
	} else {
		// quantity = ent->item->quantity;

		quantity = ent->item->gameskillnumber[g_gameskill.integer];

		// FIXME just for now
		if ( !quantity ) {
			quantity = ent->item->quantity;
		}
	}

	Add_Ammo( other, ent->item->giTag, quantity, qfalse );   //----(SA)	modified

	if (strcmp(ent->item->classname, "ammo_panzerfaust") == 0) {
       COM_BitSet( other->client->ps.weapons, WP_PANZERFAUST);
    }

	// single player has no respawns	(SA)

		if ( !( ent->spawnflags & 8 ) ) {
			return RESPAWN_SP;
		}

	return RESPAWN_AMMO;
}


// xkan, 9/18/2002 - Extracted AddMagicAmmo from Pickup_Weapon()
/*
=================================================================
AddMagicAmmo - added the specified number of clips of magic ammo
for any two-handed weapon

- returns whether any ammo was actually added
=================================================================
*/
qboolean AddMagicAmmo( gentity_t *receiver, int numOfClips ) {
	return BG_AddMagicAmmo( &receiver->client->ps, numOfClips );
}

//======================================================================


int Pickup_Weapon( gentity_t *ent, gentity_t *other ) {
	int quantity;
	qboolean alreadyHave = qfalse;
	int weapon;

	weapon = ent->item->giTag;


	if ( ent->count < 0 ) {
		quantity = 0; // None for you, sir!
	} else {
		if ( ent->count ) {
			quantity = ent->count;
		} else {
			quantity = ( random() * ( ammoTable[weapon].maxclip - 4 ) ) + 4;    // giving 4-<item default count>
		}

		// Increase quantity if player has PERK_SCAVENGER
        if (other->client->ps.perks[PERK_SCAVENGER] > 0) {
            quantity *= 2; // Increase quantity
        }

		if (g_decaychallenge.integer) {
			quantity = 999;
		}

	}

	if (( weapon == WP_PPSH ) && strstr (level.scriptAI, "Factory"))
	{
	if ( !g_cheats.integer ) 
	{
    steamSetAchievement("ACH_PPSH");
	}
	}

	if (( weapon == WP_MOSIN ) && strstr (level.scriptAI, "Village2_118"))
	{
	if ( !g_cheats.integer ) 
	{
    steamSetAchievement("ACH_MOSIN");
	}
	}

	if (( weapon == WP_TESLA ) && strstr (level.scriptAI, "Escape #2"))
	{
	if ( !g_cheats.integer ) 
	{
    steamSetAchievement("ACH_WINTERSTEIN_TESLA");
	}
	}

	if (( weapon == WP_REVOLVER ) && strstr (level.scriptAI, "Escape #2"))
	{
	if ( !g_cheats.integer ) 
	{
    steamSetAchievement("ACH_AGENT1");
	}
	}

	// check for special colt->akimbo add (if you've got a colt already, add the second now)
	if ( weapon == WP_COLT ) {
		if ( COM_BitCheck( other->client->ps.weapons, WP_COLT ) ) {
			weapon = WP_AKIMBO;
		}
	}

		if ( weapon == WP_TT33 ) {
		if ( COM_BitCheck( other->client->ps.weapons, WP_TT33 ) ) {
			weapon = WP_DUAL_TT33;
		}
	}


		if ( ent->item->giTag == WP_KNIFE ){
		if ( other->client->ps.ammoclip[ent->item->giTag] < ammoTable[WP_KNIFE].maxammo  ){
			Add_Ammo( other, ent->item->giTag, 1, qfalse );
			return -1;
		}
		return 0;
	}

	// check if player already had the weapon
	alreadyHave = COM_BitCheck( other->client->ps.weapons, weapon );

	// add the weapon
	COM_BitSet( other->client->ps.weapons, weapon );

	// snooper/garand
	if ( weapon == WP_SNOOPERSCOPE ) {
		COM_BitSet( other->client->ps.weapons, WP_GARAND );
	} else if ( weapon == WP_GARAND ) {
		COM_BitSet( other->client->ps.weapons, WP_SNOOPERSCOPE );
	}
	// fg42/scope
	else if ( weapon == WP_FG42SCOPE ) {
		COM_BitSet( other->client->ps.weapons, WP_FG42 );
	} else if ( weapon == WP_FG42 ) {
		COM_BitSet( other->client->ps.weapons, WP_FG42SCOPE );
	} else if ( weapon == WP_SNIPERRIFLE ) {
		COM_BitSet( other->client->ps.weapons, WP_MAUSER );
	} else if (weapon == WP_SILENCER) {
		COM_BitSet( other->client->ps.weapons, WP_LUGER );
	} else if ( weapon == WP_M1GARAND ) {
		COM_BitSet( other->client->ps.weapons, WP_M7 );
	} else if ( weapon == WP_M7 ) {
		COM_BitSet( other->client->ps.weapons, WP_M1GARAND );
	} else if ( weapon == WP_DELISLESCOPE ) {
		COM_BitSet( other->client->ps.weapons, WP_DELISLE );
	} else if ( weapon == WP_M1941SCOPE ) {
		COM_BitSet( other->client->ps.weapons, WP_M1941 );
	}
	
	Add_Ammo( other, weapon, quantity, !alreadyHave );


		if ( !( ent->spawnflags & 8 ) ) {
			return RESPAWN_SP;
		}

	return g_weaponRespawn.integer;
}

//======================================================================

int G_FindWeaponSlot( gentity_t *other, int weapon ) {
	int i;

	for ( i = 1; i < MAX_WEAPON_SLOTS; ++i ) {
		if ( other->client->ps.weaponSlots[i] == weapon ) {
			return i;
		}
	}

	return -1;
}

int G_GetFreeWeaponSlot( gentity_t *other ) {
	return G_FindWeaponSlot( other, WP_NONE );
}

int GetCurrentSlotId( gentity_t *other ) {
	return G_FindWeaponSlot( other, other->client->ps.weapon );
}

qboolean IsThereEmptySlot( gentity_t *other ) {
	return G_GetFreeWeaponSlot( other ) > 0;
}

weapon_t GetComplexWeapon( weapon_t weapon ) {
	switch ( weapon )
	{
	case WP_LUGER:
	case WP_GARAND:
	case WP_FG42:
	case WP_M1GARAND:
	case WP_COLT:
	case WP_TT33:
	case WP_MAUSER:
	case WP_DELISLE:
	case WP_M1941:
		return GetWeaponTableData( weapon )->weapAlts;
	default:
		return weapon;
	}
}

weapon_t GetSimpleWeapon( weapon_t weapon ) {
	switch ( weapon )
	{
	case WP_SILENCER:
	case WP_SNOOPERSCOPE:
	case WP_FG42SCOPE:
	case WP_M7:
	case WP_AKIMBO:
	case WP_DUAL_TT33:
	case WP_SNIPERRIFLE:
	case WP_DELISLESCOPE:
	case WP_M1941SCOPE:
		return GetWeaponTableData( weapon )->weapAlts;
	default:
		return weapon;
	}
}

qboolean IsWeaponComplex( weapon_t weapon ) {
	switch ( weapon )
	{
	case WP_LUGER:
	case WP_GARAND:
	case WP_FG42:
	case WP_M1GARAND:

	case WP_SILENCER:
	case WP_SNOOPERSCOPE:
	case WP_FG42SCOPE:
	case WP_M7:

	// semi complex
	case WP_SNIPERRIFLE:
	case WP_DELISLESCOPE:
	case WP_M1941SCOPE:
		return qtrue;
	default:
		return qfalse;
	}
}

qboolean IsUpgradingWeapon( gentity_t *other, weapon_t weapon ) {
	weapon_t simpleWeapon = GetSimpleWeapon( weapon );
	weapon_t complexWeapon = GetComplexWeapon( weapon );
	int simpleWeaponSlotId = G_FindWeaponSlot( other, simpleWeapon );

	return simpleWeaponSlotId > 0 && other->client->ps.weaponSlots[ simpleWeaponSlotId ] == simpleWeapon && weapon == complexWeapon;
}

qboolean NeedAmmo(gentity_t *other, weapon_t weapon ) {
	int ammoweap;

	if ( G_FindWeaponSlot( other, weapon ) < 0 ) {
		weapon_t altWeapon = GetWeaponTableData( weapon )->weapAlts;

		if ( !altWeapon ) {
			return qfalse;
		}

		if ( G_FindWeaponSlot( other, altWeapon ) < 0 ) {
			return qfalse;
		}

		ammoweap = BG_FindAmmoForWeapon( altWeapon );

	} else {
		ammoweap = BG_FindAmmoForWeapon( weapon );
	}

	return other->client->ps.ammo[ ammoweap ] < ammoTable[ ammoweap ].maxammo;
}

/**
 * @brief Drop Weapon
 * @param[in] ent
 * @param[in] weapon
 */
void G_DropWeapon( gentity_t *ent, weapon_t weapon ) {
	vec3_t    angles, velocity, org, offset, mins, maxs;
	gclient_t *client = ent->client;
	gentity_t *ent2;
	gitem_t   *item;
	trace_t   tr;

	if ( !IS_VALID_WEAPON( weapon ) ) {
		return;
	}

	// item = BG_GetItem( GetWeaponTableData( weapon )->item );
	item = BG_FindItemForWeapon( GetWeaponTableData( weapon )->weaponindex );

	if ( item->giType != IT_WEAPON || item->giWeapon != weapon ) {
		Com_Error( ERR_DROP, "Couldn't get item for weapon %i", weapon );
	}

	VectorCopy( client->ps.viewangles, angles );

	// clamp pitch
	if ( angles[ PITCH ] < -30 ) {
		angles[ PITCH ] = -30;
	} else if ( angles[ PITCH ] > 30 ) {
		angles[ PITCH ] = 30;
	}

	AngleVectors( angles, velocity, NULL, NULL );
	VectorScale( velocity, 64, offset );
	offset[ 2 ] += client->ps.viewheight / 2.f;
	VectorScale( velocity, 75, velocity );
	velocity[ 2 ] += 50 + random( ) * 35;

	VectorAdd( client->ps.origin, offset, org );

	VectorSet( mins, -ITEM_RADIUS, -ITEM_RADIUS, 0 );
	VectorSet( maxs, ITEM_RADIUS, ITEM_RADIUS, 2 * ITEM_RADIUS );

	trap_Trace( &tr, client->ps.origin, mins, maxs, org, ent->s.number, MASK_SOLID );
	VectorCopy( tr.endpos, org );

	ent2 = LaunchItem( item, org, velocity );

	if ( IsWeaponComplex( weapon ) ) {
		weapon_t altWeapon = GetWeaponTableData( weapon )->weapAlts;
		int complexWeaponSlotId = G_FindWeaponSlot( ent, weapon );
		COM_BitClear( ent->client->ps.weapons, weapon );
		COM_BitClear( ent->client->ps.weapons, altWeapon );
		ent->client->ps.weaponSlots[ complexWeaponSlotId ] = WP_NONE;
		
	} else {
		int simpleSlotId = G_FindWeaponSlot( ent, weapon );
		COM_BitClear( ent->client->ps.weapons, weapon );
		ent->client->ps.weaponSlots[ simpleSlotId ] = WP_NONE;
	}

	// Clear out empty weapon, change to next best weapon
	// 																G_AddEvent( ent, EV_CHANGE_WEAPON, 0 );

	if ( weapon == client->ps.weapon ) {
		client->ps.weapon = 0;
	}

	// now pickup the other one
	client->dropWeaponTime = level.time;
}

//======================================================================

int Pickup_Weapon_New_Inventory( gentity_t *ent, gentity_t *other ) {
	int quantity;
	qboolean alreadyHave = qfalse;
	weapon_t weapon = ent->item->giTag;

	if ( ent->count < 0 ) {
		quantity = 0; // None for you, sir!
	} else {
		if ( ent->count ) {
			quantity = ent->count;
		} else {
			quantity = ( random() * ( ammoTable[ weapon ].maxclip - 4 ) ) + 4;    // giving 4-<item default count>
		}

		if ( g_decaychallenge.integer ) {
			quantity = 999;
		}
	}

	if ( ( weapon == WP_PPSH ) && strstr ( level.scriptAI, "Factory" ) ) {
		if ( !g_cheats.integer ) {
			steamSetAchievement( "ACH_PPSH" );
		}
	}

	if ( ( weapon == WP_MOSIN ) && strstr ( level.scriptAI, "Village2_118" ) ) {
		if ( !g_cheats.integer ) {
			steamSetAchievement( "ACH_MOSIN" );
		}
	}

	if ( ( weapon == WP_TESLA ) && strstr ( level.scriptAI, "Escape #2" ) ) {
		if ( !g_cheats.integer ) {
			steamSetAchievement( "ACH_WINTERSTEIN_TESLA" );
		}
	}

	if ( ( weapon == WP_REVOLVER ) && strstr ( level.scriptAI, "Escape #2") ) {
		if ( !g_cheats.integer ) {
			steamSetAchievement( "ACH_AGENT1" );
		}
	}

	// check for special colt->akimbo add (if you've got a colt already, add the second now)
	if ( weapon == WP_COLT ) {
		if ( COM_BitCheck( other->client->ps.weapons, WP_COLT ) ) {
			weapon = WP_AKIMBO;
		}
	}

	if ( weapon == WP_TT33 ) {
		if ( COM_BitCheck( other->client->ps.weapons, WP_TT33 ) ) {
			weapon = WP_DUAL_TT33;
		}
	}

	if ( weapon == WP_KNIFE ) {
		if ( other->client->ps.ammoclip[ weapon ] < ammoTable[ WP_KNIFE ].maxammo ) {
			Add_Ammo( other, weapon, quantity, qfalse );
		}

		if ( !( ent->spawnflags & 8 ) ) {
			return RESPAWN_SP;
		}

		return g_weaponRespawn.integer;
	}

	if ( level.time - other->client->dropWeaponTime < 1000 ) {
		ent->active = qtrue;
		return 0;
	}

	if ( !COM_BitCheck( other->client->ps.weapons, weapon ) ) {
		if ( IsThereEmptySlot( other ) || IsUpgradingWeapon( other, weapon ) || other->client->latched_buttons & BUTTON_ACTIVATE ) {
			if ( IsUpgradingWeapon( other, weapon ) ) {
				weapon_t simpleWeapon = GetSimpleWeapon( weapon );
				int simpleWeaponSlotId = G_FindWeaponSlot( other, simpleWeapon );
				COM_BitSet( other->client->ps.weapons, weapon );
				other->client->ps.weaponSlots[ simpleWeaponSlotId ] = weapon;

			} else {
				int slotId;

				if ( IsThereEmptySlot( other ) ) {
					int freeWeaponSlotId = G_GetFreeWeaponSlot( other );

					slotId = freeWeaponSlotId;

				} else {
					int currentWeaponSlotId = GetCurrentSlotId( other );

					if ( currentWeaponSlotId < 1 ) {
						slotId = 1;

					} else {
						slotId = currentWeaponSlotId;
					}

					G_DropWeapon( other, other->client->ps.weaponSlots[ slotId ] );
				}

				if ( IsWeaponComplex( weapon ) ) {
					weapon_t altWeapon = GetWeaponTableData( weapon )->weapAlts;
					COM_BitSet( other->client->ps.weapons, weapon );
					COM_BitSet( other->client->ps.weapons, altWeapon );
					other->client->ps.weaponSlots[ slotId ] = GetComplexWeapon( weapon );

				} else {
					COM_BitSet( other->client->ps.weapons, weapon );
					other->client->ps.weaponSlots[ slotId ] = weapon;
				}
			}
		}
	}

	if ( NeedAmmo( other, weapon ) ) {
		Add_Ammo( other, weapon, quantity, !alreadyHave );
	} else {
		ent->active = qtrue;
		return 0;
	}

	if ( !( ent->spawnflags & 8 ) ) {
		return RESPAWN_SP;
	}

	return g_weaponRespawn.integer;
}

int Pickup_Health( gentity_t *ent, gentity_t *other ) {
	int max;
	int quantity = 0;

	// small and mega healths will go over the max
	if ( ent->item->quantity != 5 && ent->item->quantity != 100  ) {
		max = other->client->ps.stats[STAT_MAX_HEALTH];
	} else {
		max = other->client->ps.stats[STAT_MAX_HEALTH] * 2;
	}

	if ( ent->count ) {
		quantity = ent->count;
	} else {
		if ( ent->s.density ) {    // multi-stage health
			if ( ent->s.density == 2 ) {       // first stage (it counts down)
				quantity = ent->item->gameskillnumber[g_gameskill.integer];
			} else if ( ent->s.density == 1 )      { // second stage
				quantity = ent->item->quantity;
			}
		} else {
			quantity = ent->item->gameskillnumber[g_gameskill.integer];
		}
	}

	other->health += quantity;

	if ( other->health > max ) {
		other->health = max;
	}
	other->client->ps.stats[STAT_HEALTH] = other->health;

	if ( ent->s.density == 2 ) {   // multi-stage health first stage
		return RESPAWN_PARTIAL;
	} else if ( ent->s.density == 1 ) {    // last stage, leave the plate
		return RESPAWN_PARTIAL_DONE;
	}

	// single player has no respawns	(SA)
		if ( !( ent->spawnflags & 8 ) ) {
			return RESPAWN_SP;
		}

	if ( ent->item->giTag == 100 ) {        // mega health respawns slow
		return RESPAWN_MEGAHEALTH;
	}

	return RESPAWN_HEALTH;
}

//======================================================================

int Pickup_Armor( gentity_t *ent, gentity_t *other ) {
	other->client->ps.stats[STAT_ARMOR] += ent->item->quantity;
//	if ( other->client->ps.stats[STAT_ARMOR] > other->client->ps.stats[STAT_MAX_HEALTH] * 2 ) {
//		other->client->ps.stats[STAT_ARMOR] = other->client->ps.stats[STAT_MAX_HEALTH] * 2;
	if ( other->client->ps.stats[STAT_ARMOR] > 100 ) {
		other->client->ps.stats[STAT_ARMOR] = 100;
	}

	// single player has no respawns	(SA)
		if ( !( ent->spawnflags & 8 ) ) {
			return RESPAWN_SP;
		}

	return RESPAWN_ARMOR;
}

//======================================================================

/*
===============
RespawnItem
===============
*/
void RespawnItem( gentity_t *ent ) {
	if ( !ent ) {
		return;
	}

	// randomly select from teamed entities
	if ( ent->team ) {
		gentity_t   *master;
		int count;
		int choice;

		if ( !ent->teammaster ) {
			G_Error( "RespawnItem: bad teammaster" );
		}
		master = ent->teammaster;

		for ( count = 0, ent = master; ent; ent = ent->teamchain, count++ )
			;

		choice = rand() % count;

		for ( count = 0, ent = master; ent && count < choice; ent = ent->teamchain, count++ )
			;
	}

	if ( !ent ) {
		return;
	}

	ent->r.contents = CONTENTS_TRIGGER;
	//ent->s.eFlags &= ~EF_NODRAW;
	ent->flags &= ~FL_NODRAW;
	ent->r.svFlags &= ~SVF_NOCLIENT;
	trap_LinkEntity( ent );

	// play the normal respawn sound only to nearby clients
	G_AddEvent( ent, EV_ITEM_RESPAWN, 0 );

	ent->nextthink = 0;
}


/*
==============
Touch_Item
	if other->client->pers.autoActivate == PICKUP_ACTIVATE	(0), he will pick up items only when using +activate
	if other->client->pers.autoActivate == PICKUP_TOUCH		(1), he will pickup items when touched
	if other->client->pers.autoActivate == PICKUP_FORCE		(2), he will pickup the next item when touched (and reset to PICKUP_ACTIVATE when done)
==============
*/
void Touch_Item_Auto( gentity_t *ent, gentity_t *other, trace_t *trace ) {
	if ( other->client->pers.autoActivate == PICKUP_ACTIVATE ) {
		return;
	}

	ent->active = qtrue;
	Touch_Item( ent, other, trace );

	if ( other->client->pers.autoActivate == PICKUP_FORCE ) {      // autoactivate probably forced by the "Cmd_Activate_f()" function
		other->client->pers.autoActivate = PICKUP_ACTIVATE;     // so reset it.
	}
}


/*
===============
Touch_Item
===============
*/
void Touch_Item( gentity_t *ent, gentity_t *other, trace_t *trace ) {
	int respawn;
	int makenoise = EV_ITEM_PICKUP;

	// only activated items can be picked up
	if ( !ent->active ) {
		return;
	} else {
		// need to set active to false if player is maxed out
		ent->active = qfalse;
	}

	if ( !other->client ) {
		return;
	}
	if ( other->health < 1 ) {
		return;     // dead people can't pickup

	}
	// the same pickup rules are used for client side and server side
	if ( !BG_CanItemBeGrabbed( &ent->s, &other->client->ps ) ) {
		return;
	}

	// jaquboss, dont catch hot knives
	if ( ent->damage && ent->s.pos.trType != TR_STATIONARY && ent->s.pos.trType != TR_GRAVITY_PAUSED && ent->s.pos.trType != TR_GRAVITY_FLOAT ) {
		return;
	}

	G_LogPrintf( "Item: %i %s\n", other->s.number, ent->item->classname );

	// call the item-specific pickup function
	switch ( ent->item->giType ) {
	case IT_WEAPON:
		if (g_newinventory.integer > 0 || g_gametype.integer == GT_SURVIVAL)
		{
			respawn = Pickup_Weapon_New_Inventory(ent, other);
		}
		else
		{
			respawn = Pickup_Weapon(ent, other);
		}

		if (g_gametype.integer == GT_SURVIVAL)
		{
			ent->wait = -1;
		}

		break;
	case IT_AMMO:
		respawn = Pickup_Ammo( ent, other );

		if ( g_gametype.integer == GT_SURVIVAL) {
			ent->wait = -1;
		}

		break;
	case IT_ARMOR:
		respawn = Pickup_Armor( ent, other );
		break;
	case IT_HEALTH:
		respawn = Pickup_Health( ent, other );
		break;
	case IT_POWERUP:
		respawn = Pickup_Powerup( ent, other );
		break;
	case IT_TEAM:
		respawn = Pickup_Team( ent, other );
		break;
	case IT_HOLDABLE:
		respawn = Pickup_Holdable( ent, other );
		break;
	case IT_PERK:
		respawn = Pickup_Perk( ent, other );
		break;
	case IT_KEY:
		respawn = Pickup_Key( ent, other );
		break;
	case IT_TREASURE:
		respawn = Pickup_Treasure( ent, other );
		break;
	case IT_CLIPBOARD:
		respawn = Pickup_Clipboard( ent, other );
		// send the event to the client to request that the UI draw a popup
		// (specified by the configstring in ent->s.density).
		G_AddEvent( other, EV_POPUP, ent->s.density );
		if ( ent->key ) {
			G_AddEvent( other, EV_GIVEPAGE, ent->key );
		}
		break;
	default:
		return;
	}

	if ( !respawn ) {
		return;
	}

	// play sounds
	if ( ent->noise_index ) {
		// (SA) a sound was specified in the entity, so play that sound
		// (this G_AddEvent) and send the pickup as "EV_ITEM_PICKUP_QUIET"
		// so it doesn't make the default pickup sound when the pickup event is recieved
		makenoise = EV_ITEM_PICKUP_QUIET;
		G_AddEvent( other, EV_GENERAL_SOUND, ent->noise_index );
	}


	// send the pickup event
	if ( other->client->pers.predictItemPickup ) {
		G_AddPredictableEvent( other, makenoise, ent->s.modelindex );
	} else {
		G_AddEvent( other, makenoise, ent->s.modelindex );
	}

	// powerup pickups are global broadcasts
	if ( ent->item->giType == IT_POWERUP || ent->item->giType == IT_TEAM ) {
		// (SA) probably need to check for IT_KEY here too... (coop?)
		gentity_t   *te;

		te = G_TempEntity( ent->s.pos.trBase, EV_GLOBAL_ITEM_PICKUP );
		te->s.eventParm = ent->s.modelindex;
		te->r.svFlags |= SVF_BROADCAST;

		// (SA) set if we want this to only go to the pickup client
//		te->r.svFlags |= SVF_SINGLECLIENT;
//		te->r.singleClient = other->s.number;

	}

	// fire item targets
	G_UseTargets( ent, other );

	// wait of -1 will not respawn
	if ( ent->wait == -1 ) {
		ent->flags |= FL_NODRAW;
		ent->r.svFlags |= SVF_NOCLIENT; // (SA) commented back in.
		ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		ent->unlinkAfterEvent = qtrue;
		return;
	}

	// wait of -2 will respawn but not be available for pickup anymore
	// (partial use things that leave a spent modle (ex. plate for turkey)
	if ( respawn == RESPAWN_PARTIAL_DONE ) {
		ent->s.density = ( 1 << 9 );    // (10 bits of data transmission for density)
		ent->active = qtrue;        // re-activate
		trap_LinkEntity( ent );
		return;
	}

	if ( respawn == RESPAWN_PARTIAL ) {    // multi-stage health
		ent->s.density--;
		if ( ent->s.density ) {        // still not completely used up ( (SA) this will change to == 0 and stage 1 will be a destroyable item (plate/etc.) )
			ent->active = qtrue;        // re-activate
			trap_LinkEntity( ent );
			return;
		}
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
	ent->flags |= FL_NODRAW;
	//ent->s.eFlags |= EF_NODRAW;
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


//======================================================================

/*
================
LaunchItem

Spawns an item and tosses it forward
================
*/
gentity_t *LaunchItem( gitem_t *item, vec3_t origin, vec3_t velocity ) {
	gentity_t   *dropped;

	dropped = G_Spawn();

	dropped->s.eType = ET_ITEM;
	dropped->s.modelindex = item - bg_itemlist; // store item number in modelindex
//	dropped->s.modelindex2 = 1; // This is non-zero is it's a dropped item	//----(SA)	commented out since I'm using modelindex2 for model indexing now
	dropped->s.otherEntityNum2 = 1;     // DHM - Nerve :: this is taking modelindex2's place for signaling a dropped item

	dropped->classname = item->classname;
	dropped->item = item;
    
	if (item->giType == IT_POWERUP) {
		VectorSet( dropped->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS );
		VectorSet( dropped->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS );
	} else {
	   VectorSet( dropped->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, 0 );            //----(SA)	so items sit on the ground
	   VectorSet( dropped->r.maxs, ITEM_RADIUS, ITEM_RADIUS, 2 * ITEM_RADIUS );  //----(SA)	so items sit on the ground
	}
	dropped->r.contents = CONTENTS_TRIGGER | CONTENTS_ITEM;

	dropped->touch = Touch_Item_Auto;

	G_SetOrigin( dropped, origin );
	dropped->s.pos.trType = TR_GRAVITY;
	dropped->s.pos.trTime = level.time;
	VectorCopy( velocity, dropped->s.pos.trDelta );

	dropped->s.eFlags |= EF_BOUNCE_HALF;

	dropped->physicsSlide = qtrue;
	dropped->physicsFlush = qtrue;

	// (SA) TODO: FIXME: don't do this right now.  bug needs to be found.
	if (item->giType == IT_POWERUP)
	{
		dropped->s.eFlags |= EF_SPINNING; // spin the weapon as it flies from the dead player.  it will stop when it hits the ground
		// Add dynamic light to the dropped powerup
		dropped->s.constantLight = 150;			 // RGB intensity
		dropped->s.constantLight |= (255 << 8);	 // R
		dropped->s.constantLight |= (223 << 16); // G
		dropped->s.constantLight |= (0 << 24);	 // B

		// Play a sound at the location of the dropped item
		dropped->s.loopSound = G_SoundIndex("sound/misc/powerup_ambience.wav");
	}

	if ( item->giType == IT_TEAM ) { // Special case for CTF flags
		dropped->think = Team_DroppedFlagThink;
		dropped->nextthink = level.time + 30000;
	} else { // auto-remove after 30 seconds
		dropped->think = G_FreeEntity;
		dropped->nextthink = level.time + 30000;
	}

	dropped->flags = FL_DROPPED_ITEM;

	trap_LinkEntity( dropped );

	return dropped;
}

/*
================
Drop_Item

Spawns an item and tosses it forward
================
*/
gentity_t *Drop_Item( gentity_t *ent, gitem_t *item, float angle, qboolean novelocity ) {
	vec3_t velocity;
	vec3_t angles;

	VectorCopy( ent->s.apos.trBase, angles );
	angles[YAW] += angle;
	angles[PITCH] = 0;  // always forward

	if ( novelocity ) {
		VectorClear( velocity );
	} else
	{
		AngleVectors( angles, velocity, NULL, NULL );
		VectorScale( velocity, 150, velocity );
		velocity[2] += 200 + crandom() * 50;
	}

	return LaunchItem( item, ent->s.pos.trBase, velocity );
}


/*
================
Use_Item

Respawn the item
================
*/
void Use_Item( gentity_t *ent, gentity_t *other, gentity_t *activator ) {
	RespawnItem( ent );
}

//======================================================================

/*
================
FinishSpawningItem

Traces down to find where an item should rest, instead of letting them
free fall from their spawn points
================
*/
void FinishSpawningItem( gentity_t *ent ) {
	trace_t tr;
	vec3_t dest;
	vec3_t maxs;

	if ( ent->spawnflags & 1 ) { // suspended
		VectorSet( ent->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, -ITEM_RADIUS );
		VectorSet( ent->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS );
		VectorCopy( ent->r.maxs, maxs );
	} else
	{
		// Rafael
		// had to modify this so that items would spawn in shelves
		VectorSet( ent->r.mins, -ITEM_RADIUS, -ITEM_RADIUS, 0 );
		VectorSet( ent->r.maxs, ITEM_RADIUS, ITEM_RADIUS, ITEM_RADIUS );
		VectorCopy( ent->r.maxs, maxs );
		maxs[2] /= 2;
	}

	ent->r.contents = CONTENTS_TRIGGER | CONTENTS_ITEM;
	ent->touch = Touch_Item_Auto;
	ent->s.eType = ET_ITEM;
	ent->s.modelindex = ent->item - bg_itemlist;        // store item number in modelindex

	ent->s.otherEntityNum2 = 0;     // DHM - Nerve :: takes modelindex2's place in signaling a dropped item
//----(SA)	we don't use this (yet, anyway) so I'm taking it so you can specify a model for treasure items and clipboards
//	ent->s.modelindex2 = 0; // zero indicates this isn't a dropped item
	if ( ent->model ) {
		ent->s.modelindex2 = G_ModelIndex( ent->model );
	}

	if ( g_nopickupchallenge.integer && ent->item->giType == IT_HEALTH )
	{
    return;
	}

	if ( g_regen.integer && ent->item->giType == IT_HEALTH )
	{
    return;
	}

	if ( g_decaychallenge.integer && ent->item->giType == IT_HEALTH )
	{
    return;
	}

	if ( g_nopickupchallenge.integer && ent->item->giType == IT_ARMOR )
	{
    return;
	}

    // Classic arsenal
	if ( g_fullarsenal.integer == 0 && (   ent->item->giWeapon == WP_MP34 
	                                || ent->item->giWeapon == WP_REVOLVER 
									|| ent->item->giWeapon == WP_G43 
									|| ent->item->giWeapon == WP_M1GARAND 
									|| ent->item->giWeapon == WP_BAR 
									|| ent->item->giWeapon == WP_MG42M
									|| ent->item->giWeapon == WP_M97
									|| ent->item->giWeapon == WP_MP44
									|| ent->item->giWeapon == WP_M7
									|| ent->item->giWeapon == WP_BROWNING ) )
	{
    return;
	}
       
    // No new ammo types too
	if ( g_fullarsenal.integer == 0 && ent->item->giType == IT_AMMO && (ent->item->giAmmoIndex == WP_MP44 || ent->item->giAmmoIndex == WP_M97 || ent->item->giAmmoIndex == WP_BAR)) 
	{
	return;
	} 
    
	// RealRTCW arsenal without extra guns, value 2 will ge everything
	if ( !g_dlc1.integer && ( ent->item->giWeapon == WP_M1941SCOPE
									|| ent->item->giWeapon == WP_DELISLE
									|| ent->item->giWeapon == WP_M1941
									|| ent->item->giWeapon == WP_AUTO5 ) )
	{
    return;
	}



	// if clipboard, add the menu name string to the client's configstrings
	if ( ent->item->giType == IT_CLIPBOARD ) {
		if ( !ent->message ) {
			ent->s.density = G_FindConfigstringIndex( "clip_test", CS_CLIPBOARDS, MAX_CLIPBOARD_CONFIGSTRINGS, qtrue );
		} else {
			ent->s.density = G_FindConfigstringIndex( ent->message, CS_CLIPBOARDS, MAX_CLIPBOARD_CONFIGSTRINGS, qtrue );
		}

		ent->touch = Touch_Item;    // no auto-pickup, only activate
	} else if ( ent->item->giType == IT_HOLDABLE )      {
		if ( ent->item->giTag >= HI_BOOK1 && ent->item->giTag <= HI_BOOK3 ) {
			G_FindConfigstringIndex( va( "hbook%d", ent->item->giTag - HI_BOOK1 ), CS_CLIPBOARDS, MAX_CLIPBOARD_CONFIGSTRINGS, qtrue );
		}
//		ent->touch = Touch_Item;	// no auto-pickup, only activate
	}


	// using an item causes it to respawn
	ent->use = Use_Item;

//----(SA) moved this up so it happens for suspended items too (and made it a function)
	G_SetAngle( ent, ent->s.angles );

	if ( ent->spawnflags & 1 ) {    // suspended
		G_SetOrigin( ent, ent->s.origin );
	} else {

		VectorSet( dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 4096 );
		trap_Trace( &tr, ent->s.origin, ent->r.mins, maxs, dest, ent->s.number, MASK_SOLID );

		if ( tr.startsolid ) {
			vec3_t temp;

			VectorCopy( ent->s.origin, temp );
			temp[2] -= ITEM_RADIUS;

			VectorSet( dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 4096 );
			trap_Trace( &tr, temp, ent->r.mins, maxs, dest, ent->s.number, MASK_SOLID );
		}

#if 0
		// drop to floor
		VectorSet( dest, ent->s.origin[0], ent->s.origin[1], ent->s.origin[2] - 4096 );
		trap_Trace( &tr, ent->s.origin, ent->r.mins, maxs, dest, ent->s.number, MASK_SOLID );
#endif
		if ( tr.startsolid ) {
			G_Printf( "FinishSpawningItem: %s startsolid at %s\n", ent->classname, vtos( ent->s.origin ) );
			G_FreeEntity( ent );
			return;
		}

		// allow to ride movers
		ent->s.groundEntityNum = tr.entityNum;

		G_SetOrigin( ent, tr.endpos );
	}

	if ( ent->spawnflags & 2 ) {      // spin
		ent->s.eFlags |= EF_SPINNING;
	}


	// team slaves and targeted items aren't present at start
	if ( ( ent->flags & FL_TEAMSLAVE ) || ent->targetname ) {
		ent->flags |= FL_NODRAW;
		//ent->s.eFlags |= EF_NODRAW;
		ent->r.contents = 0;
		return;
	}

	// health/ammo can potentially be multi-stage (multiple use)
	if ( ent->item->giType == IT_HEALTH || ent->item->giType == IT_AMMO || ent->item->giType == IT_POWERUP ) {
		int i;

		// having alternate models defined in bg_misc.c for a health or ammo item specify it as "multi-stage"
		// TTimo: left-hand operand of comma expression has no effect
		// was:
		// for(i=0;i<4,ent->item->world_model[i];i++) {}
		i = 0;
		while ( i < 3 && ent->item->world_model[i] )
			i++;

		ent->s.density = i - 1;   // store number of stages in 'density' for client (most will have '1')
	}

	trap_LinkEntity( ent );
}


qboolean itemRegistered[MAX_ITEMS];


/*
==============
ClearRegisteredItems
==============
*/
void ClearRegisteredItems( void ) {
	memset( itemRegistered, 0, sizeof( itemRegistered ) );

	// players always start with the base weapon
	// (SA) Nope, not any more...

//----(SA)	this will be determined by the level or starting position, or the savegame
//			but for now, re-register the MP40 automatically
//	RegisterItem( BG_FindItemForWeapon( WP_MP40 ) );
	RegisterItem( BG_FindItem( "Med Health" ) );           // NERVE - SMF - this is so med packs properly display
}

/*
===============
RegisterItem

The item will be added to the precache list
===============
*/
void RegisterItem( gitem_t *item ) {
	if ( !item ) {
		G_Error( "RegisterItem: NULL" );
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
	char string[MAX_ITEMS + 1];
	int i;
	int count;

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

	trap_SetConfigstring( CS_ITEMS, string );
}


/*
============
G_SpawnItem

Sets the clipping size and plants the object on the floor.

Items can't be immediately dropped to floor, because they might
be on an entity that hasn't spawned yet.
============
*/
void G_SpawnItem( gentity_t *ent, gitem_t *item ) {
	char    *noise;
	int page;

	G_SpawnFloat( "random", "0", &ent->random );
	G_SpawnFloat( "wait", "0", &ent->wait );

	RegisterItem( item );
	ent->item = item;
	// some movers spawn on the second frame, so delay item
	// spawns until the third frame so they can ride trains
	ent->nextthink = level.time + FRAMETIME * 2;
	ent->think = FinishSpawningItem;

	if ( G_SpawnString( "noise", 0, &noise ) ) {
		ent->noise_index = G_SoundIndex( noise );
	}

	ent->physicsBounce = 0.50;      // items are bouncy
	ent->physicsSlide = qtrue;
	ent->physicsFlush = qtrue;

	if ( ent->model ) {
		ent->s.modelindex2 = G_ModelIndex( ent->model );
	}

	if ( item->giType == IT_CLIPBOARD ) {
		if ( G_SpawnInt( "notebookpage", "1", &page ) ) {
			ent->key = page;
		}
	}

	if ( item->giType == IT_POWERUP ) {
		G_SoundIndex( "sound/items/poweruprespawn.wav" );
	}


	if ( item->giType == IT_TREASURE ) {
		level.numTreasure++;
		G_SendMissionStats();
	}
}

void G_FlushItem( gentity_t *ent, trace_t *trace )
{
	vec3_t  forward, start, end;
	trace_t tr;
	vec3_t outAxis[ 3 ];
	float s;

	if ( g_flushItems.integer &&
		ent->physicsFlush &&
		trace->plane.normal[2] > 0.7f &&
		( trace->plane.normal[0] != 0.0f || trace->plane.normal[1] != 0.0f  || trace->plane.normal[2] != 1.0f ) ) { // no need to adjust flat ground, make it faster
		AngleVectors( ent->r.currentAngles, forward, NULL, NULL );
		VectorCopy( trace->plane.normal, outAxis[ 2 ] );
		ProjectPointOnPlane( outAxis[ 0 ], forward, outAxis[ 2 ] );

		if( !VectorNormalize( outAxis[ 0 ] ) )
		{
			AngleVectors( ent->r.currentAngles, NULL, NULL, forward );
			ProjectPointOnPlane( outAxis[ 0 ], forward, outAxis[ 2 ] );
			VectorNormalize( outAxis[ 0 ] );
		}

		CrossProduct( outAxis[ 0 ], outAxis[ 2 ], outAxis[ 1 ] );
		outAxis[ 1 ][ 0 ] = -outAxis[ 1 ][ 0 ];
		outAxis[ 1 ][ 1 ] = -outAxis[ 1 ][ 1 ];
		outAxis[ 1 ][ 2 ] = -outAxis[ 1 ][ 2 ];

		AxisToAngles( outAxis, ent->r.currentAngles );
		VectorMA( trace->endpos, -64.0f, trace->plane.normal, end );
		VectorMA( trace->endpos, 1.0f, trace->plane.normal, start );

		trap_Trace( &tr, start, NULL, NULL, end, ent->s.number, MASK_SOLID );

		if ( !tr.startsolid ) {
			s = tr.fraction * -64;
			VectorMA( trace->endpos, s, trace->plane.normal, trace->endpos );
		}

		// make sure it is off ground
		VectorMA( trace->endpos, 1.0f, trace->plane.normal, trace->endpos );

	}
	else {
		trace->endpos[2] += 1.0;

		if ( ent->physicsFlush )
			ent->r.currentAngles[0] = ent->r.currentAngles[2] = 0;
	}

	G_SetAngle( ent, ent->r.currentAngles);
	SnapVector( trace->endpos );
	G_SetOrigin( ent, trace->endpos );
	ent->s.groundEntityNum = trace->entityNum;

	if ( ent->s.groundEntityNum != ENTITYNUM_WORLD )
		ent->s.pos.trType = TR_GRAVITY_PAUSED; // jaquboss, so items will fall down when needed

}


qboolean G_ItemStick( gentity_t *ent, trace_t *trace, vec3_t velocity )
{
	float	dot;

	// check if it is knife
	if ( !ent->damage )
		return qfalse;

	if ( ent->s.weapon != WP_KNIFE )
		return qfalse;

	// reset flushing
	ent->physicsFlush = qtrue;

	// get a direction
	VectorNormalize( velocity );
	dot = DotProduct( velocity, trace->plane.normal );

	// do not lodge
	if ( dot > -0.75 )
		return qfalse;

	if ( trace->surfaceFlags & (SURF_GRASS|SURF_SNOW|SURF_WOOD|SURF_GRAVEL) ){

		vectoangles( velocity, ent->r.currentAngles );
		ent->physicsFlush = qfalse;
		return qtrue;
	}

	return qfalse;
}


/*
================
G_BounceItem

================
*/
void G_BounceItem( gentity_t *ent, trace_t *trace ) {
	vec3_t   velocity;
	float	dot;
	int      hitTime;

	hitTime = level.previousTime + ( level.time - level.previousTime ) * trace->fraction;

	// reflect the velocity on the trace plane
	BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, velocity, qfalse, ent->s.effect2Time );
	dot = -2 * DotProduct( velocity, trace->plane.normal );
	VectorMA( velocity, dot, trace->plane.normal, ent->s.pos.trDelta );


	// bounce or just slide? - check if stuck or surface not too step
	if ( trace->plane.normal[2] >= 0.7 || VectorLength( ent->s.pos.trDelta ) < 16 || !ent->physicsSlide ) {
		// cut the velocity to keep from bouncing forever
		
		if (trace->contents & (CONTENTS_BODY)) {
		VectorScale( ent->s.pos.trDelta, ent->physicsBounce * 0.1, ent->s.pos.trDelta );
		} else {
		VectorScale( ent->s.pos.trDelta, ent->physicsBounce, ent->s.pos.trDelta );
		}

		// do a bounce
        if ( ent->item && ( ent->item->giType == IT_WEAPON || ent->item->giType == IT_AMMO ) && ent->item->giTag > WP_NONE && ent->item->giTag < WP_NUM_WEAPONS)
		{
			G_AddEvent( ent, EV_BOUNCE_SOUND, ent->item->giType == IT_WEAPON ? 0 : 1 );
			ent->s.weapon = ent->item->giTag;
		}

		// check for stop
		if ( G_ItemStick( ent, trace, velocity ) || ( VectorLength(ent->s.pos.trDelta) < 40 && trace->plane.normal[2] > 0)  )
		{
			G_FlushItem( ent, trace );
			return;
		}

		// bounce the angles
		if ( ent->s.apos.trType != TR_STATIONARY )
		{
			if (trace->contents & (CONTENTS_BODY)) {
		    VectorScale( ent->s.pos.trDelta, ent->physicsBounce * 0.1, ent->s.pos.trDelta);
		    } else {
		    VectorScale( ent->s.apos.trDelta, ent->physicsBounce, ent->s.apos.trDelta );
		    }
			ent->s.apos.trTime = level.time;
		}


	} else if ( ent->physicsSlide ) {
		BG_ClipVelocity( ent->s.pos.trDelta, trace->plane.normal, ent->s.pos.trDelta, 1.001 );
	}

	VectorCopy( ent->r.currentOrigin, ent->s.pos.trBase );
	ent->s.pos.trTime = level.time;
	VectorAdd( ent->r.currentOrigin, trace->plane.normal, ent->r.currentOrigin);
}

/*
=================
G_RunItemProp
=================
*/

void G_RunItemProp( gentity_t *ent, vec3_t origin ) {
	gentity_t   *traceEnt;
	trace_t trace;
	gentity_t   *owner;
	vec3_t start;
	vec3_t end;

	owner = &g_entities[ent->r.ownerNum];

	VectorCopy( ent->r.currentOrigin, start );
	start[2] += 1;

	VectorCopy( origin, end );
	end[2] += 1;

	trap_Trace( &trace, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, end,
				ent->r.ownerNum, MASK_SHOT );

	traceEnt = &g_entities[ trace.entityNum ];

	if ( traceEnt && traceEnt->takedamage && traceEnt != ent ) {
		ent->enemy = traceEnt;
	}

	if ( owner->client && trace.startsolid && traceEnt != owner && traceEnt != ent /* && !traceEnt->active*/ ) {

		ent->takedamage = qfalse;
		ent->die( ent, ent, NULL, 10, 0 );
		Prop_Break_Sound( ent );

		return;
	} else if ( trace.surfaceFlags & SURF_NOIMPACT )    {
		ent->takedamage = qfalse;

		Props_Chair_Skyboxtouch( ent );

		return;
	}
}

/*
================
G_RunItem

================
*/
void G_RunItem( gentity_t *ent ) {
	vec3_t origin;
	trace_t tr;
	int contents;
	int mask;

	// if its groundentity has been set to none, it may have been pushed off an edge
	if ( ent->s.groundEntityNum == ENTITYNUM_NONE ) {
		if ( ent->s.pos.trType != TR_GRAVITY ) {
			ent->s.pos.trType = TR_GRAVITY;
			ent->s.pos.trTime = level.time;
		}
	}

	if ( ent->s.pos.trType == TR_STATIONARY || ent->s.pos.trType == TR_GRAVITY_PAUSED ) { //----(SA)
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
		mask = MASK_SOLID | CONTENTS_MISSILECLIP;
	}
	trap_Trace( &tr, ent->r.currentOrigin, ent->r.mins, ent->r.maxs, origin,
				ent->r.ownerNum, mask );

	if ( ent->isProp && ent->takedamage ) {
		G_RunItemProp( ent, origin );
	}

	VectorCopy( tr.endpos, ent->r.currentOrigin );

	if ( tr.startsolid ) {
		tr.fraction = 0;
	}

	trap_LinkEntity( ent ); // FIXME: avoid this for stationary?

	// check think function
	G_RunThink( ent );

	if ( tr.fraction == 1 ) {
		return;
	}

	// if it is in a nodrop volume, remove it
	contents = trap_PointContents( ent->r.currentOrigin, -1 );
	if ( contents & CONTENTS_NODROP ) {
		if ( ent->item && ent->item->giType == IT_TEAM ) {
			Team_FreeEntity( ent );
		} else {
			G_FreeEntity( ent );
		}
		return;
	}


        // This is needed for throwing knives
		if ( ent->damage && tr.entityNum != ENTITYNUM_NONE ) {
		float	speed;
		vec3_t	delta, dir;
		int      hitTime;

		hitTime = level.previousTime + ( level.time - level.previousTime ) * tr.fraction;

		BG_EvaluateTrajectoryDelta( &ent->s.pos, hitTime, delta, qfalse, ent->s.effect2Time );
		VectorCopy ( delta, dir );
		VectorNormalize( dir );
		speed = VectorLength( delta );
        
		// Let the AI know that the knife hit the ground
		AICast_ProcessBullet( &g_entities[ent->r.ownerNum], g_entities[ent->r.ownerNum].s.pos.trBase, tr.endpos );


		if ( speed > MIN_KNIFESPEED ){


			gentity_t		*temp, *traceEnt;

			traceEnt = &g_entities[tr.entityNum];

			// do damage
			if ( traceEnt->takedamage ){
				int	damage = ent->damage; //+ ((speed-300)/20);

				G_Damage( traceEnt, ent, ent->parent, dir, tr.endpos, damage, 0, ent->methodOfDeath );
			}

			// do impact
			if ( traceEnt->client && traceEnt->takedamage ) {
				temp = G_TempEntity( tr.endpos, EV_MISSILE_HIT );
				temp->s.otherEntityNum = traceEnt->s.number;
				temp->s.weapon = ent->s.weapon;
				temp->s.clientNum = ent->r.ownerNum;
			}

		}
	}

	G_BounceItem( ent, &tr );
}

