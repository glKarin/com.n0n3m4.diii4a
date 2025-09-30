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
// bg_misc.c -- both games misc functions, all completely stateless

#include "../qcommon/q_shared.h"
#include "bg_public.h"

/*QUAKED item_***** ( 0 0 0 ) (-16 -16 -16) (16 16 16) suspended
DO NOT USE THIS CLASS, IT JUST HOLDS GENERAL INFORMATION.
The suspended flag will allow items to hang in the air, otherwise they are dropped to the next surface.

If an item is the target of another entity, it will not spawn in until fired.

An item fires all of its targets when it is picked up.  If the toucher can't carry it, the targets won't be fired.

"notfree" if set to 1, don't spawn in free for all games
"notteam" if set to 1, don't spawn in team games
"notsingle" if set to 1, don't spawn in single player games
"wait"	override the default wait before respawning.  -1 = never respawn automatically, which can be used with targeted spawning.
"random" random number of plus or minus seconds varied from the respawn time
"count" override quantity or duration on most items.
*/

gitem_t	bg_itemlist[] = 
{
	{
		NULL,
		NULL,
		{ NULL,
		NULL,
		NULL, NULL} ,
/* icon */		NULL,
/* pickup */	NULL,
		0,
		0,
		0,
/* precache */ "",
/* sounds */ ""
	},	// leave index 0 alone

	//
	// ARMOR
	//

/*QUAKED item_armor_shard (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
// STONELANCE
//		"item_armor_shard", 
		"item_armor_green", 
// END
		"sound/misc/ar1_pkup.wav",
// STONELANCE
//		{ "models/powerups/armor/shard.md3", 
//		"models/powerups/armor/shard_sphere.md3",
		{ "models/powerups/armor/greenshard.md3",
		NULL,
// END
		NULL, NULL} ,
// STONELANCE
//		"icons/iconr_shard",		// icon
//		"Armor Shard",				// pickup
//		5,
		"icons/shardg",				// icon
		"Green Armor",				// pickup
		25,
// END
		IT_ARMOR,
		0,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_armor_combat (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
// STONELANCE
//		"item_armor_combat", 
		"item_armor_yellow", 
// END
		"sound/misc/ar2_pkup.wav",
// STONELANCE
//        { "models/powerups/armor/armor_yel.md3",
		{ "models/powerups/armor/yellowshard.md3",
// END
		NULL, NULL, NULL},
// STONELANCE
//		"icons/iconr_yellow",		// icon
//		"Armor",					// pickup
		"icons/shardy",				// icon
		"Yellow Armor",				// pickup
// END
		50,
		IT_ARMOR,
		0,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_armor_body (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
// STONELANCE
//		"item_armor_body", 
		"item_armor_red", 
// END
		"sound/misc/ar2_pkup.wav",
// STONELANCE
//		{ "models/powerups/armor/armor_red.md3",
		{ "models/powerups/armor/redshard.md3",
// END
		NULL, NULL, NULL},
// STONELANCE
//		"icons/iconr_red",		// icon
//		"Heavy Armor",			// pickup
		"icons/shardr",			// icon
		"Red Armor",			// pickup
// END
		100,
		IT_ARMOR,
		0,
/* precache */ "",
/* sounds */ ""
	},

	//
	// health
	//
/*QUAKED item_health_small (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_health_small",
		"sound/items/s_health.wav",
        { "models/powerups/health/small_cross.md3", NULL, NULL },
/* icon */		"icons/iconh_green",
/* pickup */	"5 Health",
		5,
		IT_HEALTH,
		0,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_health (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_health",
		"sound/items/n_health.wav",
        { "models/powerups/health/medium_cross.md3", NULL, NULL },
/* icon */		"icons/iconh_yellow",
/* pickup */	"25 Health",
		25,
		IT_HEALTH,
		0,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_health_large (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_health_large",
		"sound/items/l_health.wav",
        { "models/powerups/health/large_cross.md3", NULL, NULL },
/* icon */		"icons/iconh_red",
/* pickup */	"50 Health",
		50,
		IT_HEALTH,
		0,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_health_mega (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"item_health_mega",
		"sound/items/m_health.wav",
        { "models/powerups/health/mega_cross.md3", NULL, NULL },
/* icon */		"icons/iconh_mega",
/* pickup */	"Mega Health",
		100,
		IT_HEALTH,
		0,
/* precache */ "",
/* sounds */ ""
	},


	//
	// WEAPONS 
	//

/*QUAKED weapon_gauntlet (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_gauntlet", 
		"sound/misc/w_pkup.wav",
        { "models/weapons2/gauntlet/gauntlet.md3",
		NULL, NULL, NULL},
/* icon */		"icons/iconw_gauntlet",
/* pickup */	"Chainsaw",
		0,
		IT_WEAPON,
		WP_GAUNTLET,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_shotgun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_shotgun", 
		"sound/misc/w_pkup.wav",
        { "models/weapons2/shotgun/shotgun.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_shotgun",
/* pickup */	"Shotgun",
		10,
		IT_WEAPON,
		WP_SHOTGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_machinegun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_machinegun", 
		"sound/misc/w_pkup.wav",
        { "models/weapons2/machinegun/machinegun.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_machinegun",
/* pickup */	"Machinegun",
		40,
		IT_WEAPON,
		WP_MACHINEGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_grenadelauncher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_grenadelauncher",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/grenadel/grenadel.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_grenade",
/* pickup */	"Grenade Launcher",
		10,
		IT_WEAPON,
		WP_GRENADE_LAUNCHER,
/* precache */ "",
/* sounds */ "sound/weapons/grenade/hgrenb1a.wav sound/weapons/grenade/hgrenb2a.wav"
	},

/*QUAKED weapon_rocketlauncher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_rocketlauncher",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/rocketl/rocketl.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_rocket",
/* pickup */	"Rocket Launcher",
		10,
		IT_WEAPON,
		WP_ROCKET_LAUNCHER,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_lightning (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_lightning", 
		"sound/misc/w_pkup.wav",
        { "models/weapons2/lightning/lightning.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_lightning",
/* pickup */	"Lightning Gun",
		100,
		IT_WEAPON,
		WP_LIGHTNING,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_railgun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_railgun", 
		"sound/misc/w_pkup.wav",
        { "models/weapons2/railgun/railgun.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_railgun",
/* pickup */	"Railgun",
		10,
		IT_WEAPON,
		WP_RAILGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_plasmagun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_plasmagun", 
		"sound/misc/w_pkup.wav",
        { "models/weapons2/plasma/plasma.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_plasma",
/* pickup */	"Plasma Gun",
		50,
		IT_WEAPON,
		WP_PLASMAGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_bfg (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_bfg",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/bfg/bfg.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_bfg",
/* pickup */	"BFG10K",
		20,
		IT_WEAPON,
		WP_BFG,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_grapplinghook (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
// STONELANCE
/*
	{
		"weapon_grapplinghook",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/grapple/grapple.md3", 
		NULL, NULL, NULL},
		"icons/iconw_grapple",	// icon
		"Grappling Hook",		// pickup
		0,
		IT_WEAPON,
		WP_GRAPPLING_HOOK,
		"",				// precache
		""				// sounds
	},
*/
// END

//Q3Rally Code Start

/*QUAKED weapon_flamethrower (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
{
	"weapon_flamethrower",
	"sound/misc/w_pkup.wav",
	{ "models/weapons2/flamethrower/flamethrower.md3",
	NULL, NULL, NULL},
/* icon */	"icons/iconw_flame",
/* pickup */	"Flame Thrower",
	20,
	IT_WEAPON,
	WP_FLAME_THROWER,
/* precache */ "",
/* sounds */ ""
},

//Q3Rally Code END

// STONELANCE
	// rally_weapon_smoke
	{
		"rally_weapon_smoke",
		"sound/misc/w_pkup.wav",
        { "models/rearfire/rf_smoke.md3", 
		NULL, NULL, NULL},
		"icons/rf_smoke",				// icon
		"Rearfire Smoke Screen",		// pickup
		15,
		IT_RFWEAPON,
		RWP_SMOKE,
		"",								// precache
		""								// sounds
	},

	// rally_weapon_oil
	{
		"rally_weapon_oil",
		"sound/misc/w_pkup.wav",
        { "models/rearfire/rf_oil.md3", 
		NULL, NULL, NULL},
		"icons/rf_oil",					// icon
		"Rearfire Oil Slick",			// pickup
		15,
		IT_RFWEAPON,
		RWP_OIL,
		"",								// precache
		""								// sounds
	},

	// rally_weapon_mine
	{
		"rally_weapon_mine",
		"sound/misc/w_pkup.wav",
        { "models/rearfire/rf_mines.md3", 
		NULL, NULL, NULL},
		"icons/rf_mine",				// icon
		"Rearfire Mine",				// pickup
		5,
		IT_RFWEAPON,
		RWP_MINE,
		"",								// precache
		"sound/weapons/grenade/hgrenb1a.wav sound/weapons/grenade/hgrenb2a.wav" // sounds
	},

	// rally_weapon_flame
	{
		"rally_weapon_flame",
		"sound/misc/w_pkup.wav",
        { "models/rearfire/rf_flame.md3", 
		NULL, NULL, NULL},
		"icons/rf_flame",				// icon
		"Rearfire Flame Trail",			// pickup
		15,
		IT_RFWEAPON,
		RWP_FLAME,
		"",								// precache
		""								// sounds
	},

	// rally_weapon_bio
	{
		"rally_weapon_bio",
		"sound/misc/w_pkup.wav",
        { "models/rearfire/rf_bio.md3", 
		NULL, NULL, NULL},
		"icons/rf_bio",					// icon
		"Rearfire Bio Sludge",			// pickup
		15,
		IT_RFWEAPON,
		RWP_BIO,
		"",								// precache
		""								// sounds
	},
// END

	//
	// AMMO ITEMS
	//

/*QUAKED ammo_shells (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_shells",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/shotgunam.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/icona_shotgun",
/* pickup */	"Shells",
		10,
		IT_AMMO,
		WP_SHOTGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_bullets (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_bullets",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/machinegunam.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/icona_machinegun",
/* pickup */	"Bullets",
		50,
		IT_AMMO,
		WP_MACHINEGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_grenades (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_grenades",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/grenadeam.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/icona_grenade",
/* pickup */	"Grenades",
		5,
		IT_AMMO,
		WP_GRENADE_LAUNCHER,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_cells (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_cells",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/plasmaam.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/icona_plasma",
/* pickup */	"Cells",
		30,
		IT_AMMO,
		WP_PLASMAGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_lightning (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_lightning",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/lightningam.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/icona_lightning",
/* pickup */	"Lightning",
		60,
		IT_AMMO,
		WP_LIGHTNING,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_rockets (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_rockets",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/rocketam.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/icona_rocket",
/* pickup */	"Rockets",
		5,
		IT_AMMO,
		WP_ROCKET_LAUNCHER,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_slugs (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_slugs",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/railgunam.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/icona_railgun",
/* pickup */	"Slugs",
		10,
		IT_AMMO,
		WP_RAILGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_bfg (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_bfg",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/bfgam.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/icona_bfg",
/* pickup */	"Bfg Ammo",
		15,
		IT_AMMO,
		WP_BFG,
/* precache */ "",
/* sounds */ ""
	},

//Q3Rally Code Start

/*QUAKED ammo_flame (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
{
	"ammo_flame",
	"sound/misc/am_pkup.wav",
	{ "models/powerups/ammo/flameam.md3", 
	NULL, NULL, NULL},
/* icon */ "icons/icona_flame",
/* pickup */	"Flame Ammo",
	50,
	IT_AMMO,
	WP_FLAME_THROWER,
/* precache */ "",
/* sounds */ ""
},

//Q3Rally Code END

	//
	// HOLDABLE ITEMS
	//
/*QUAKED holdable_teleporter (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"holdable_teleporter", 
		"sound/items/holdable.wav",
        { "models/powerups/holdable/teleporter.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/teleporter",
/* pickup */	"Personal Teleporter",
		60,
		IT_HOLDABLE,
		HI_TELEPORTER,
/* precache */ "",
/* sounds */ ""
	},
/*QUAKED holdable_medkit (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"holdable_medkit", 
		"sound/items/holdable.wav",
        { 
		"models/powerups/holdable/medkit.md3", 
		"models/powerups/holdable/medkit_sphere.md3",
		NULL, NULL},
/* icon */		"icons/medkit",
/* pickup */	"Medkit",
		60,
		IT_HOLDABLE,
		HI_MEDKIT,
/* precache */ "",
/* sounds */ "sound/items/use_medkit.wav"
	},

// STONELANCE
	// rally_item_turbo
	{
		"rally_item_turbo", 
		"sound/items/turbo.wav",
        { "models/powerups/turbo/turbo.md3",
		"models/powerups/turbo/turbogear.md3",
		NULL, NULL},
		"icons/turbo",				// icons
		"Turbo",					// pickup
		5,
		IT_POWERUP,
		PW_TURBO,
		"",							// precache
		""							// sounds
	},

// END

	//
	// POWERUP ITEMS
	//
/*QUAKED item_quad (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
// STONELANCE
//		"item_quad", 
		"rally_item_quad", 
// END
		"sound/items/quaddamage.wav",
// STONELANCE
//		{ "models/powerups/instant/quad.md3", 
//		"models/powerups/instant/quad_ring.md3",
		{ "models/powerups/quad/quad.md3", 
		"models/powerups/quad/quadring.md3",
// END
		NULL, NULL },
/* icon */		"icons/quad",
/* pickup */	"Quad Damage",
		30,
		IT_POWERUP,
		PW_QUAD,
/* precache */ "",
/* sounds */ "sound/items/damage2.wav sound/items/damage3.wav"
	},

/*QUAKED item_enviro (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
// STONELANCE
//		"item_enviro",
		"rally_item_env",
// END
		"sound/items/protect.wav",
// STONELANCE
//		{ "models/powerups/instant/enviro.md3", 
//		"models/powerups/instant/enviro_ring.md3", 
		{ "models/powerups/env/atomic.md3", 
		"models/powerups/env/envgear.md3", 
// END
		NULL, NULL },
// STONELANCE
//		"icons/envirosuit",				// icons
//		"Battle Suit",					// pickup
		"icons/env",					// icons
		"Environmental Protection",		// pickup
// END
		30,
		IT_POWERUP,
		PW_BATTLESUIT,
/* precache */ "",
/* sounds */ "sound/items/airout.wav sound/items/protect3.wav"
	},

/*QUAKED item_haste (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
// STONELANCE
//		"item_haste",
		"rally_item_haste",
// END
		"sound/items/haste.wav",
// STONELANCE
//		{ "models/powerups/instant/haste.md3", 
//		"models/powerups/instant/haste_ring.md3", 
//		NULL, NULL },
		{ "models/powerups/haste/haste.md3", 
		"models/powerups/haste/hagear.md3", 
		"models/powerups/haste/hasteshell.md3",
		NULL },
// END
/* icon */		"icons/haste",
// STONELANCE
//		"Speed",				// pickup
		"Haste",				// pickup
// END
		30,
		IT_POWERUP,
		PW_HASTE,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_invis (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
// STONELANCE
//		"item_invis",
		"rally_item_invis",
// END
		"sound/items/invisibility.wav",
// STONELANCE
//		{ "models/powerups/instant/invis.md3", 
//		"models/powerups/instant/invis_ring.md3", 
		{ "models/powerups/invis/invis.md3", 
		"models/powerups/invis/invisgear.md3", 
// END
		NULL, NULL },
/* icon */		"icons/invis",
/* pickup */	"Invisibility",
		30,
		IT_POWERUP,
        PW_INVIS,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_regen (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
// STONELANCE
//		"item_regen",
		"rally_item_regen",
// END
		"sound/items/regeneration.wav",
// STONELANCE
//		{ "models/powerups/instant/regen.md3", 
//		"models/powerups/instant/regen_ring.md3", 
		{ "models/powerups/regen/regen.md3", 
		"models/powerups/regen/regear.md3", 
// END
		NULL, NULL },
/* icon */		"icons/regen",
/* pickup */	"Auto Repair",
		30,
		IT_POWERUP,
		PW_REGEN,
/* precache */ "",
/* sounds */ "sound/items/regen.wav"
	},

	// item_flight
/*
	{
		"item_flight",
		"sound/items/flight.wav",
        { "models/powerups/instant/flight.md3", 
		"models/powerups/instant/flight_ring.md3", 
		NULL, NULL },
		"icons/flight",				// icons
		"Flight",					// pickup
		60,
		IT_POWERUP,
		PW_FLIGHT,
		"",							// precache
		"sound/items/flight.wav"	// sounds
	},
*/

// STONELANCE
	// rally_item_shield
	{
		"rally_item_shield", 
		"sound/items/shield.wav",
        { "models/powerups/shield/shield.md3", 
		"models/powerups/shield/shgear.md3",
		NULL, NULL },
		"icons/shield",				// icons
		"Shielding",				// pickup
		30,
		IT_POWERUP,
		PW_SHIELD,
		"",							// precache
		"sound/items/damage2.wav sound/items/damage3.wav" // sounds
	},
// END

/*QUAKED team_CTF_redflag (1 0 0) (-16 -16 -16) (16 16 16)
Only in CTF games
*/
	{
		"team_CTF_redflag",
		NULL,
        { "models/flags/r_flag.md3",
		NULL, NULL, NULL },
/* icon */		"icons/iconf_red1",
/* pickup */	"Red Flag",
		0,
		IT_TEAM,
		PW_REDFLAG,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED team_CTF_blueflag (0 0 1) (-16 -16 -16) (16 16 16)
Only in CTF games
*/
	{
		"team_CTF_blueflag",
		NULL,
        { "models/flags/b_flag.md3",
		NULL, NULL, NULL },
/* icon */		"icons/iconf_blu1",
/* pickup */	"Blue Flag",
		0,
		IT_TEAM,
		PW_BLUEFLAG,
/* precache */ "",
/* sounds */ ""
	},

// Q3Rally Code Start
	
/*QUAKED team_DOMINATION_sigil_red
Only in Domination games
*/
  {
    "team_DOMINATION_sigil_red",
    NULL,
    { "models/flags/r_sigil.md3",
    0, 0, 0 },
/* icon */    "icons/icons_red",
/* pickup */  "Control Point",
    0,
    IT_SIGIL,
    PW_SIGILRED,
/* precache */    "",
/* sounds */    ""
    },
    
/*QUAKED team_DOMINATION_sigil_blue
Only in Domination games
*/
  {
    "team_DOMINATION_sigil_blue",
    NULL,
    { "models/flags/b_sigil.md3",
    0, 0, 0 },
/* icon */    "icons/icons_blu",
/* pickup */  "Control Point",
    0,
    IT_SIGIL,
    PW_SIGILBLUE,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED team_DOMINATION_sigil_green
Only in Domination games
*/
  {
    "team_DOMINATION_sigil_green",
    NULL,
    { "models/flags/g_sigil.md3",
    0, 0, 0 },
/* icon */    "icons/icons_green",
/* pickup */  "Control Point",
    0,
    IT_SIGIL,
    PW_SIGILGREEN,
/* precache */    "",
/* sounds */    ""
    },

/*QUAKED team_DOMINATION_sigil_yellow
Only in Domination games
*/
  {
    "team_DOMINATION_sigil_yellow",
    NULL,
    { "models/flags/y_sigil.md3",
    0, 0, 0 },
/* icon */    "icons/icons_yellow",
/* pickup */  "Control Point",
    0,
    IT_SIGIL,
    PW_SIGILYELLOW,
/* precache */    "",
/* sounds */    ""
    },
    
/*QUAKED team_DOMINATION_sigil
Only in Domination games
*/
  {
    "team_DOMINATION_sigil",
    NULL,
    { "models/flags/n_sigil.md3",
    0, 0, 0 },
/* icon */    "icons/icons_neutral",
/* pickup */  "Control Point",
    0,
    IT_SIGIL,
    PW_SIGILWHITE,
/* precache */    "",
/* sounds */    ""
    },
    
// Q3Rally Code END

#ifdef MISSIONPACK
/*QUAKED holdable_kamikaze (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"holdable_kamikaze", 
		"sound/items/holdable.wav",
        { "models/powerups/kamikazi.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/kamikaze",
/* pickup */	"Kamikaze",
		60,
		IT_HOLDABLE,
		HI_KAMIKAZE,
/* precache */ "",
/* sounds */ "sound/items/kamikazerespawn.wav"
	},

/*QUAKED holdable_portal (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"holdable_portal", 
		"sound/items/holdable.wav",
        { "models/powerups/holdable/porter.md3",
		NULL, NULL, NULL},
/* icon */		"icons/portal",
/* pickup */	"Portal",
		60,
		IT_HOLDABLE,
		HI_PORTAL,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED holdable_invulnerability (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"holdable_invulnerability", 
		"sound/items/holdable.wav",
        { "models/powerups/holdable/invulnerability.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/invulnerability",
/* pickup */	"Invulnerability",
		60,
		IT_HOLDABLE,
		HI_INVULNERABILITY,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_nails (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_nails",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/nailgunam.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/icona_nailgun",
/* pickup */	"Nails",
		20,
		IT_AMMO,
		WP_NAILGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_mines (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_mines",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/proxmineam.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/icona_proxlauncher",
/* pickup */	"Proximity Mines",
		10,
		IT_AMMO,
		WP_PROX_LAUNCHER,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED ammo_belt (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"ammo_belt",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/chaingunam.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/icona_chaingun",
/* pickup */	"Chaingun Belt",
		100,
		IT_AMMO,
		WP_CHAINGUN,
/* precache */ "",
/* sounds */ ""
	},

	//
	// PERSISTANT POWERUP ITEMS
	//
/*QUAKED item_scout (.3 .3 1) (-16 -16 -16) (16 16 16) suspended redTeam blueTeam
*/
	{
		"item_scout",
		"sound/items/scout.wav",
        { "models/powerups/scout.md3", 
		NULL, NULL, NULL },
/* icon */		"icons/scout",
/* pickup */	"Scout",
		30,
		IT_PERSISTANT_POWERUP,
		PW_SCOUT,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_guard (.3 .3 1) (-16 -16 -16) (16 16 16) suspended redTeam blueTeam
*/
	{
		"item_guard",
		"sound/items/guard.wav",
        { "models/powerups/guard.md3", 
		NULL, NULL, NULL },
/* icon */		"icons/guard",
/* pickup */	"Guard",
		30,
		IT_PERSISTANT_POWERUP,
		PW_GUARD,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_doubler (.3 .3 1) (-16 -16 -16) (16 16 16) suspended redTeam blueTeam
*/
	{
		"item_doubler",
		"sound/items/doubler.wav",
        { "models/powerups/doubler.md3", 
		NULL, NULL, NULL },
/* icon */		"icons/doubler",
/* pickup */	"Doubler",
		30,
		IT_PERSISTANT_POWERUP,
		PW_DOUBLER,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED item_doubler (.3 .3 1) (-16 -16 -16) (16 16 16) suspended redTeam blueTeam
*/
	{
		"item_ammoregen",
		"sound/items/ammoregen.wav",
        { "models/powerups/ammo.md3",
		NULL, NULL, NULL },
/* icon */		"icons/ammo_regen",
/* pickup */	"Ammo Regen",
		30,
		IT_PERSISTANT_POWERUP,
		PW_AMMOREGEN,
/* precache */ "",
/* sounds */ ""
	},

	/*QUAKED team_CTF_neutralflag (0 0 1) (-16 -16 -16) (16 16 16)
Only in One Flag CTF games
*/
	{
		"team_CTF_neutralflag",
		NULL,
        { "models/flags/n_flag.md3",
		NULL, NULL, NULL },
/* icon */		"icons/iconf_neutral1",
/* pickup */	"Neutral Flag",
		0,
		IT_TEAM,
		PW_NEUTRALFLAG,
/* precache */ "",
/* sounds */ ""
	},

	{
		"item_redcube",
		"sound/misc/am_pkup.wav",
        { "models/powerups/orb/r_orb.md3",
		NULL, NULL, NULL },
/* icon */		"icons/iconh_rorb",
/* pickup */	"Red Cube",
		0,
		IT_TEAM,
		0,
/* precache */ "",
/* sounds */ ""
	},

	{
		"item_bluecube",
		"sound/misc/am_pkup.wav",
        { "models/powerups/orb/b_orb.md3",
		NULL, NULL, NULL },
/* icon */		"icons/iconh_borb",
/* pickup */	"Blue Cube",
		0,
		IT_TEAM,
		0,
/* precache */ "",
/* sounds */ ""
	},
/*QUAKED weapon_nailgun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_nailgun", 
		"sound/misc/w_pkup.wav",
        { "models/weapons/nailgun/nailgun.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_nailgun",
/* pickup */	"Nailgun",
		10,
		IT_WEAPON,
		WP_NAILGUN,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED weapon_prox_launcher (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_prox_launcher", 
		"sound/misc/w_pkup.wav",
        { "models/weapons/proxmine/proxmine.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_proxlauncher",
/* pickup */	"Prox Launcher",
		5,
		IT_WEAPON,
		WP_PROX_LAUNCHER,
/* precache */ "",
/* sounds */ "sound/weapons/proxmine/wstbtick.wav "
			"sound/weapons/proxmine/wstbactv.wav "
			"sound/weapons/proxmine/wstbimpl.wav "
			"sound/weapons/proxmine/wstbimpm.wav "
			"sound/weapons/proxmine/wstbimpd.wav "
			"sound/weapons/proxmine/wstbactv.wav"
	},

/*QUAKED weapon_chaingun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_chaingun", 
		"sound/misc/w_pkup.wav",
        { "models/weapons/vulcan/vulcan.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_chaingun",
/* pickup */	"Chaingun",
		80,
		IT_WEAPON,
		WP_CHAINGUN,
/* precache */ "",
/* sounds */ "sound/weapons/vulcan/wvulwind.wav"
	},
#endif

	// end of list marker
	{NULL}
};

int		bg_numItems = ARRAY_LEN( bg_itemlist ) - 1;


/*
==============
BG_FindItemForPowerup
==============
*/
gitem_t	*BG_FindItemForPowerup( powerup_t pw ) {
	int		i;

	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( (bg_itemlist[i].giType == IT_POWERUP || 
					bg_itemlist[i].giType == IT_TEAM ||
					bg_itemlist[i].giType == IT_PERSISTANT_POWERUP ||
          bg_itemlist[i].giType == IT_SIGIL ) && 
			bg_itemlist[i].giTag == pw ) {
			return &bg_itemlist[i];
		}
	}

	return NULL;
}


/*
==============
BG_FindItemForHoldable
==============
*/
gitem_t	*BG_FindItemForHoldable( holdable_t pw ) {
	int		i;

	for ( i = 0 ; i < bg_numItems ; i++ ) {
		if ( bg_itemlist[i].giType == IT_HOLDABLE && bg_itemlist[i].giTag == pw ) {
			return &bg_itemlist[i];
		}
	}

	Com_Error( ERR_DROP, "HoldableItem not found" );

	return NULL;
}


/*
===============
BG_FindItemForWeapon

===============
*/
gitem_t	*BG_FindItemForWeapon( weapon_t weapon ) {
	gitem_t	*it;
	
	for ( it = bg_itemlist + 1 ; it->classname ; it++) {
// STONELANCE
//		if ( it->giType == IT_WEAPON && it->giTag == weapon ) {
		if ( (it->giType == IT_WEAPON || it->giType == IT_RFWEAPON) && it->giTag == weapon ) {
// END
			return it;
		}
	}

	Com_Error( ERR_DROP, "Couldn't find item for weapon %i", weapon);
	return NULL;
}

/*
===============
BG_FindItem

===============
*/
gitem_t	*BG_FindItem( const char *pickupName ) {
	gitem_t	*it;
	
	for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
		if ( !Q_stricmp( it->pickup_name, pickupName ) )
			return it;
	}

	return NULL;
}

/*
============
BG_PlayerTouchesItem

Items can be picked up without actually touching their physical bounds to make
grabbing them easier
============
*/
qboolean	BG_PlayerTouchesItem( playerState_t *ps, entityState_t *item, int atTime ) {
	vec3_t		origin;

	BG_EvaluateTrajectory( &item->pos, atTime, origin );

	// we are ignoring ducked differences here
	if ( ps->origin[0] - origin[0] > 44
		|| ps->origin[0] - origin[0] < -50
		|| ps->origin[1] - origin[1] > 36
		|| ps->origin[1] - origin[1] < -36
		|| ps->origin[2] - origin[2] > 36
		|| ps->origin[2] - origin[2] < -36 ) {
		return qfalse;
	}

	return qtrue;
}



/*
================
BG_CanItemBeGrabbed

Returns false if the item should not be picked up.
This needs to be the same for client side prediction and server use.
================
*/
qboolean BG_CanItemBeGrabbed( int gametype, const entityState_t *ent, const playerState_t *ps ) {
	gitem_t	*item;
#ifdef MISSIONPACK
	int		upperBound;
#endif
// STONELANCE
	int		i;
// END

	if ( ent->modelindex < 1 || ent->modelindex >= bg_numItems ) {
		Com_Error( ERR_DROP, "BG_CanItemBeGrabbed: index out of range" );
	}

	item = &bg_itemlist[ent->modelindex];

	switch( item->giType ) {
	case IT_WEAPON:
		return qtrue;	// weapons are always picked up

// STONELANCE
	case IT_RFWEAPON:	// only have one rearweapon at a time.
		for (i = RWP_SMOKE; i < WP_NUM_WEAPONS; i++){
			if (item->giTag == i) continue;

			// FIXME: need this?
/*
			if (ps->stats[STAT_WEAPONS] & ( 1 << i ) && !ps->ammo[ i ]){
				ps->stats[STAT_WEAPONS] &= ~( 1 << i );
			}
*/

			if (ps->stats[STAT_WEAPONS] & ( 1 << i ))
				return qfalse;
		}
		return qtrue;
// END

	case IT_AMMO:
		if ( ps->ammo[ item->giTag ] >= 200 ) {
			return qfalse;		// can't hold any more
		}
		return qtrue;

	case IT_ARMOR:
#ifdef MISSIONPACK
		if( bg_itemlist[ps->stats[STAT_PERSISTANT_POWERUP]].giTag == PW_SCOUT ) {
			return qfalse;
		}

		// we also clamp armor to the maxhealth for handicapping
		if( bg_itemlist[ps->stats[STAT_PERSISTANT_POWERUP]].giTag == PW_GUARD ) {
			upperBound = ps->stats[STAT_MAX_HEALTH];
		}
		else {
			upperBound = ps->stats[STAT_MAX_HEALTH] * 2;
		}

		if ( ps->stats[STAT_ARMOR] >= upperBound ) {
			return qfalse;
		}
#else
		if ( ps->stats[STAT_ARMOR] >= ps->stats[STAT_MAX_HEALTH] * 2 ) {
			return qfalse;
		}
#endif
		return qtrue;

	case IT_HEALTH:
		// small and mega healths will go over the max, otherwise
		// don't pick up if already at max
#ifdef MISSIONPACK
		if( bg_itemlist[ps->stats[STAT_PERSISTANT_POWERUP]].giTag == PW_GUARD ) {
		}
		else
#endif
		if ( item->quantity == 5 || item->quantity == 100 ) {
			if ( ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH] * 2 ) {
				return qfalse;
			}
			return qtrue;
		}

		if ( ps->stats[STAT_HEALTH] >= ps->stats[STAT_MAX_HEALTH] ) {
			return qfalse;
		}
		return qtrue;

	case IT_POWERUP:
// STONELANCE
//		return qtrue;	// powerups are always picked up

		// powerups past turbo can always be picked up
		if (item->giTag >= PW_TURBO)
			return qtrue;

		// can only have one powerup below turbo
		for (i = 0; i < PW_TURBO; i++){
			if (ps->powerups[i])
				return qfalse;
		}
		return qtrue;
// END

#ifdef MISSIONPACK
	case IT_PERSISTANT_POWERUP:
		// can only hold one item at a time
		if ( ps->stats[STAT_PERSISTANT_POWERUP] ) {
			return qfalse;
		}

		// check team only
		if( ( ent->generic1 & 2 ) && ( ps->persistant[PERS_TEAM] != TEAM_RED ) ) {
			return qfalse;
		}
		if( ( ent->generic1 & 4 ) && ( ps->persistant[PERS_TEAM] != TEAM_BLUE ) ) {
			return qfalse;
		}

		return qtrue;
#endif

	case IT_TEAM: // team items, such as flags
#ifdef MISSIONPACK		
		if( gametype == GT_1FCTF ) {
			// neutral flag can always be picked up
			if( item->giTag == PW_NEUTRALFLAG ) {
				return qtrue;
			}
			if (ps->persistant[PERS_TEAM] == TEAM_RED) {
				if (item->giTag == PW_BLUEFLAG  && ps->powerups[PW_NEUTRALFLAG] ) {
					return qtrue;
				}
			} else if (ps->persistant[PERS_TEAM] == TEAM_BLUE) {
				if (item->giTag == PW_REDFLAG  && ps->powerups[PW_NEUTRALFLAG] ) {
					return qtrue;
				}
			}
		}
#endif
		if( gametype == GT_CTF ) {
			// ent->modelindex2 is non-zero on items if they are dropped
			// we need to know this because we can pick up our dropped flag (and return it)
			// but we can't pick up our flag at base
			if (ps->persistant[PERS_TEAM] == TEAM_RED) {
				if (item->giTag == PW_BLUEFLAG ||
					(item->giTag == PW_REDFLAG && ent->modelindex2) ||
					(item->giTag == PW_REDFLAG && ps->powerups[PW_BLUEFLAG]) )
					return qtrue;
			} else if (ps->persistant[PERS_TEAM] == TEAM_BLUE) {
				if (item->giTag == PW_REDFLAG ||
					(item->giTag == PW_BLUEFLAG && ent->modelindex2) ||
					(item->giTag == PW_BLUEFLAG && ps->powerups[PW_REDFLAG]) )
					return qtrue;
			}
		}

#ifdef MISSIONPACK
		if( gametype == GT_HARVESTER ) {
			return qtrue;
		}
#endif
		return qfalse;
        
        // Q3Rally Code Start
	case IT_SIGIL:
        // red team cannot touch a red sigil
        if (ps->persistant[PERS_TEAM] == TEAM_RED && ent->powerups == PW_SIGILRED)
            return qfalse;
        // blue team cannot touch a blue sigil
        else if (ps->persistant[PERS_TEAM] == TEAM_BLUE && ent->powerups == PW_SIGILBLUE)
            return qfalse;
        // green team cannot touch a green sigil
        else if (ps->persistant[PERS_TEAM] == TEAM_GREEN && ent->powerups == PW_SIGILGREEN)
            return qfalse;
        // yellow team cannot touch a yellow sigil
        else if (ps->persistant[PERS_TEAM] == TEAM_YELLOW && ent->powerups == PW_SIGILYELLOW)
            return qfalse;
	  else
	     return qtrue;
// Q3Rally Code END

	case IT_HOLDABLE:
		// can only hold one item at a time
		if ( ps->stats[STAT_HOLDABLE_ITEM] ) {
			return qfalse;
		}
		return qtrue;

        case IT_BAD:
            Com_Error( ERR_DROP, "BG_CanItemBeGrabbed: IT_BAD" );
        default:
#ifndef Q3_VM
#ifndef NDEBUG
          Com_Printf("BG_CanItemBeGrabbed: unknown enum %d\n", item->giType );
#endif
#endif
         break;
	}

	return qfalse;
}

//======================================================================

/*
================
BG_EvaluateTrajectory

================
*/
void BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result ) {
	float		deltaTime;
	float		phase;

	switch( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorCopy( tr->trBase, result );
		break;
	case TR_LINEAR:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float) tr->trDuration;
		phase = sin( deltaTime * M_PI * 2 );
		VectorMA( tr->trBase, phase, tr->trDelta, result );
		break;
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			atTime = tr->trTime + tr->trDuration;
		}
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		if ( deltaTime < 0 ) {
			deltaTime = 0;
		}
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime;		// FIXME: local gravity...
		break;
	case TR_ROTATING:
		if ( tr->trTime > 0 )
			deltaTime = tr->trTime * 0.001;	// milliseconds to seconds
		else if ( tr->trTime < 0 )
			deltaTime = ( atTime + tr->trTime ) * 0.001;
		else
			deltaTime = ( atTime - tr->trTime ) * 0.001;
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		break;
	default:
		Com_Error( ERR_DROP, "BG_EvaluateTrajectory: unknown trType: %i", tr->trType );
		break;
	}
}

/*
================
BG_EvaluateTrajectoryDelta

For determining velocity at a given time
================
*/
void BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result ) {
	float	deltaTime;
	float	phase;

	switch( tr->trType ) {
	case TR_STATIONARY:
	case TR_INTERPOLATE:
		VectorClear( result );
		break;
	case TR_ROTATING:
	case TR_LINEAR:
		VectorCopy( tr->trDelta, result );
		break;
	case TR_SINE:
		deltaTime = ( atTime - tr->trTime ) / (float) tr->trDuration;
		phase = cos( deltaTime * M_PI * 2 );	// derivative of sin = cos
		phase *= 0.5;
		VectorScale( tr->trDelta, phase, result );
		break;
	case TR_LINEAR_STOP:
		if ( atTime > tr->trTime + tr->trDuration ) {
			VectorClear( result );
			return;
		}
		VectorCopy( tr->trDelta, result );
		break;
	case TR_GRAVITY:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorCopy( tr->trDelta, result );
		result[2] -= DEFAULT_GRAVITY * deltaTime;		// FIXME: local gravity...
		break;
	default:
		Com_Error( ERR_DROP, "BG_EvaluateTrajectoryDelta: unknown trType: %i", tr->trType );
		break;
	}
}

char *eventnames[] = {
	"EV_NONE",

	"EV_FOOTSTEP",
	"EV_FOOTSTEP_METAL",
	"EV_FOOTSPLASH",
	"EV_FOOTWADE",
	"EV_SWIM",

	"EV_STEP_4",
	"EV_STEP_8",
	"EV_STEP_12",
	"EV_STEP_16",

	"EV_FALL_SHORT",
	"EV_FALL_MEDIUM",
	"EV_FALL_FAR",

	"EV_JUMP_PAD",			// boing sound at origin", jump sound on player

	"EV_JUMP",
	"EV_WATER_TOUCH",	// foot touches
	"EV_WATER_LEAVE",	// foot leaves
	"EV_WATER_UNDER",	// head touches
	"EV_WATER_CLEAR",	// head leaves

	"EV_ITEM_PICKUP",			// normal item pickups are predictable
	"EV_GLOBAL_ITEM_PICKUP",	// powerup / team sounds are broadcast to everyone

	"EV_NOAMMO",
	"EV_CHANGE_WEAPON",
	"EV_FIRE_WEAPON",
// STONELANCE
	"EV_HAZARD",
	"EV_ALTFIRE_WEAPON",
	"EV_FIRE_REARWEAPON",
// END

	"EV_USE_ITEM0",
	"EV_USE_ITEM1",
	"EV_USE_ITEM2",
	"EV_USE_ITEM3",
	"EV_USE_ITEM4",
	"EV_USE_ITEM5",
	"EV_USE_ITEM6",
	"EV_USE_ITEM7",
	"EV_USE_ITEM8",
	"EV_USE_ITEM9",
	"EV_USE_ITEM10",
	"EV_USE_ITEM11",
	"EV_USE_ITEM12",
	"EV_USE_ITEM13",
	"EV_USE_ITEM14",
	"EV_USE_ITEM15",

	"EV_ITEM_RESPAWN",
	"EV_ITEM_POP",
	"EV_PLAYER_TELEPORT_IN",
	"EV_PLAYER_TELEPORT_OUT",

	"EV_GRENADE_BOUNCE",		// eventParm will be the soundindex

	"EV_GENERAL_SOUND",
	"EV_GLOBAL_SOUND",		// no attenuation
	"EV_GLOBAL_TEAM_SOUND",

	"EV_BULLET_HIT_FLESH",
	"EV_BULLET_HIT_WALL",

	"EV_MISSILE_HIT",
	"EV_MISSILE_MISS",
	"EV_MISSILE_MISS_METAL",
	"EV_RAILTRAIL",
	"EV_SHOTGUN",
	"EV_BULLET",				// otherEntity is the shooter

	"EV_PAIN",
	"EV_DEATH1",
	"EV_DEATH2",
	"EV_DEATH3",
	"EV_OBITUARY",

	"EV_POWERUP_QUAD",
	"EV_POWERUP_BATTLESUIT",
// STONELANCE
	"EV_POWERUP_SHIELD",
	"EV_POWERUP_TURBO",
// END
	"EV_POWERUP_REGEN",

	"EV_GIB_PLAYER",			// gib a previously living player
//Q3Rally Code Start
	"EV_BREAK_GLASS",
	"EV_BREAKWOOD",
	"EV_BREAKMETAL",
//Q3Rally Code End
	"EV_SCOREPLUM",			// score plum

#ifdef MISSIONPACK
	"EV_PROXIMITY_MINE_STICK",
	"EV_PROXIMITY_MINE_TRIGGER",
	"EV_KAMIKAZE",			// kamikaze explodes
	"EV_OBELISKEXPLODE",		// obelisk explodes
	"EV_OBELISKPAIN",		// obelisk pain
	"EV_INVUL_IMPACT",		// invulnerability sphere impact
	"EV_JUICED",				// invulnerability juiced effect
	"EV_LIGHTNINGBOLT",		// lightning bolt bounced of invulnerability sphere
#endif

	"EV_DEBUG_LINE",
	"EV_STOPLOOPINGSOUND",
	"EV_TAUNT",
	"EV_TAUNT_YES",
	"EV_TAUNT_NO",
	"EV_TAUNT_FOLLOWME",
	"EV_TAUNT_GETFLAG",
	"EV_TAUNT_GUARDBASE",
	"EV_TAUNT_PATROL",

//Q3Rally Code Start
	"EV_EMIT_DEBRIS_LIGHT",
	"EV_EMIT_DEBRIS_DARK",
	"EV_EMIT_DEBRIS_LIGHT_LARGE",
	"EV_EMIT_DEBRIS_DARK_LARGE",
	"EV_EMIT_DEBRIS_WOOD",
	"EV_EMIT_DEBRIS_FLESH",
	"EV_EMIT_DEBRIS_GLASS",
	"EV_EMIT_DEBRIS_STONE",
	"EV_EARTHQUAKE",
    
	"EV_EXPLOSION",
	"EV_PARTICLES_GRAVITY",
	"EV_PARTICLES_LINEAR",
	"EV_PARTICLES_LINEAR_UP",
	"EV_PARTICLES_LINEAR_DOWN"
//Q3Rally Code End
};

/*
===============
BG_AddPredictableEventToPlayerstate

Handles the sequence numbers
===============
*/

void	trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );

void BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps ) {

#ifdef _DEBUG
	{
		char buf[256];
		trap_Cvar_VariableStringBuffer("showevents", buf, sizeof(buf));
		if ( atof(buf) != 0 ) {
#ifdef QAGAME
			Com_Printf(" game event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount/*ps->commandTime*/, ps->eventSequence, eventnames[newEvent], eventParm);
#else
			Com_Printf("Cgame event svt %5d -> %5d: num = %20s parm %d\n", ps->pmove_framecount/*ps->commandTime*/, ps->eventSequence, eventnames[newEvent], eventParm);
#endif
		}
	}
#endif
	ps->events[ps->eventSequence & (MAX_PS_EVENTS-1)] = newEvent;
	ps->eventParms[ps->eventSequence & (MAX_PS_EVENTS-1)] = eventParm;
	ps->eventSequence++;
}

/*
========================
BG_TouchJumpPad
========================
*/
// STONELANCE
// void BG_TouchJumpPad( playerState_t *ps, entityState_t *jumppad ) {
void BG_TouchJumpPad( car_t *car, playerState_t *ps, entityState_t *jumppad ) {
	int		i;
// END
	vec3_t	angles;
	float p;
	int effectNum;

	// spectators don't use jump pads
	if ( ps->pm_type != PM_NORMAL ) {
		return;
	}

	// flying characters don't hit bounce pads
// STONELANCE
/*
	if ( ps->powerups[PW_FLIGHT] ) {
		return;
	}
*/
// END

	// if we didn't hit this same jumppad the previous frame
	// then don't play the event sound again if we are in a fat trigger
	if ( ps->jumppad_ent != jumppad->number ) {

		vectoangles( jumppad->origin2, angles);
		p = fabs( AngleNormalize180( angles[PITCH] ) );
		if( p < 45 ) {
			effectNum = 0;
		} else {
			effectNum = 1;
		}
		BG_AddPredictableEventToPlayerstate( EV_JUMP_PAD, effectNum, ps );
	}
	// remember hitting this jumppad this frame
	ps->jumppad_ent = jumppad->number;
	ps->jumppad_frame = ps->pmove_framecount;
	// give the player the velocity from the jumppad
	VectorCopy( jumppad->origin2, ps->velocity );

// STONELANCE
	VectorScale( ps->velocity, 0.65f, ps->velocity );

	VectorCopy(ps->velocity, car->sBody.v);
	VectorCopy(ps->velocity, car->tBody.v);
	for (i = 0; i < NUM_CAR_POINTS; i++){
		VectorCopy(ps->velocity, car->sPoints[i].v);
		VectorCopy(ps->velocity, car->tPoints[i].v);
	}
// END
}

/*
========================
BG_PlayerStateToEntityState

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qboolean snap ) {
	int		i;

// STONELANCE
//	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR ) {
	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR ||
		( ps->pm_flags & PMF_OBSERVE ) ) {
// END
		s->eType = ET_INVISIBLE;
	} else if ( ps->stats[STAT_HEALTH] <= GIB_HEALTH ) {
		s->eType = ET_INVISIBLE;
	} else {
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_INTERPOLATE;
	VectorCopy( ps->origin, s->pos.trBase );
	if ( snap ) {
		SnapVector( s->pos.trBase );
	}

	// set the trDelta for flag direction
// STONELANCE
	VectorCopy( ps->velocity, s->pos.trDelta );
	if ( snap ) {
		SnapVector( s->pos.trDelta );
	}
// END

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy( ps->viewangles, s->apos.trBase );
	if ( snap ) {
		SnapVector( s->apos.trBase );
	}

// STONELANCE
	VectorCopy( ps->angularMomentum, s->apos.trDelta );
	if ( snap ) {
		SnapVector( s->apos.trDelta );
	}

	// View Angles
	s->angles2[PITCH] = ps->damageAngles[PITCH];
	s->angles2[YAW] = ps->damageAngles[YAW];
//	s->angles2[YAW] = ps->movementDir;
// END
	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->clientNum = ps->clientNum;		// ET_PLAYER looks here instead of at number
										// so corpses can also reference the proper config
	s->eFlags = ps->eFlags;
	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		s->eFlags |= EF_DEAD;
	} else {
		s->eFlags &= ~EF_DEAD;
	}

// STONELANCE
	if ( ps->stats[STAT_HEALTH] <= 25 )
		s->eFlags |= EF_SMOKE_DARK;
	else if ( ps->stats[STAT_HEALTH] <= 50 )
		s->eFlags |= EF_SMOKE_LIGHT;

	if (ps->extra_eFlags & CF_REVERSE)
		s->eFlags |= EF_REVERSE;
	if (ps->extra_eFlags & CF_BRAKE)
		s->eFlags |= EF_BRAKE;
// END

	if ( ps->externalEvent ) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	} else if ( ps->entityEventSequence < ps->eventSequence ) {
		int		seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS) {
			ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
		}
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}

	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
// STONELANCE - so we can turn turbo off
//		if ( ps->powerups[ i ] ) {
		if ( ps->powerups[ i ] > 0 ) {
// END
			s->powerups |= 1 << i;
		}
	}

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;
}

qboolean isRaceObserver( int clientNum );

/*
========================
BG_PlayerStateToEntityStateExtraPolate

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
// STONELANCE
// void BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s, int time, qboolean snap ) {
void BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s, int time, int sv_fps, qboolean snap ) {
// END
	int		i;

// STONELANCE
//	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR ) {
	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR ||
		( ps->pm_flags & PMF_OBSERVE ) || isRaceObserver( ps->clientNum ) ) {
// END
		s->eType = ET_INVISIBLE;
	} else if ( ps->stats[STAT_HEALTH] <= GIB_HEALTH ) {
		s->eType = ET_INVISIBLE;
	} else {
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_LINEAR_STOP;
	VectorCopy( ps->origin, s->pos.trBase );
	if ( snap ) {
		SnapVector( s->pos.trBase );
	}
	// set the trDelta for flag direction and linear prediction
// STONELANCE
	VectorCopy( ps->velocity, s->pos.trDelta );
	if ( snap ) {
		SnapVector( s->pos.trDelta );
	}
// END

	// set the time for linear prediction
	s->pos.trTime = time;
	// set maximum extra polation time
// STONELANCE
//	s->pos.trDuration = 50; // 1000 / sv_fps (default = 20)
	s->pos.trDuration = 3 * 1000 / sv_fps;

//	s->apos.trType = TR_INTERPOLATE;
	s->apos.trType = TR_LINEAR_STOP;
// END
	VectorCopy( ps->viewangles, s->apos.trBase );
	if ( snap ) {
		SnapVector( s->apos.trBase );
	}

// STONELANCE FIXME: use angular velocity instead of momentum
//	VectorCopy( ps->angularMomentum, s->apos.trDelta );
	if ( snap ) {
		SnapVector( s->apos.trDelta );
	}
	s->apos.trTime = time;
	s->apos.trDuration = 3 * 1000 / sv_fps;
// END

	// View Angles
	s->angles2[PITCH] = ps->damageAngles[PITCH];
	s->angles2[YAW] = ps->damageAngles[YAW];
//	s->angles2[YAW] = ps->movementDir;
// END
	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->clientNum = ps->clientNum;		// ET_PLAYER looks here instead of at number
										// so corpses can also reference the proper config
	s->eFlags = ps->eFlags;
	if ( ps->stats[STAT_HEALTH] <= 0 ) {
		s->eFlags |= EF_DEAD;
	} else {
		s->eFlags &= ~EF_DEAD;
	}

// STONELANCE
	if ( ps->stats[STAT_HEALTH] <= 25 )
		s->eFlags |= EF_SMOKE_DARK;
	else if ( ps->stats[STAT_HEALTH] <= 50 )
		s->eFlags |= EF_SMOKE_LIGHT;

	if (ps->extra_eFlags & CF_REVERSE)
		s->eFlags |= EF_REVERSE;
	if (ps->extra_eFlags & CF_BRAKE)
		s->eFlags |= EF_BRAKE;
// END

	if ( ps->externalEvent ) {
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	} else if ( ps->entityEventSequence < ps->eventSequence ) {
		int		seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_PS_EVENTS) {
			ps->entityEventSequence = ps->eventSequence - MAX_PS_EVENTS;
		}
		seq = ps->entityEventSequence & (MAX_PS_EVENTS-1);
		s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}

	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
// STONELANCE - so we can turn turbo off
//		if ( ps->powerups[ i ] ) {
		if ( ps->powerups[ i ] > 0 ) {
// END
			s->powerups |= 1 << i;
		}
	}

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;
}
