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
// bg_misc.c -- both games misc functions, all completely stateless

#include "../qcommon/q_shared.h"
#include "bg_public.h"
#include "g_local.h"

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
/* pickup */	NULL,
		0,
		0,
		0,
/* precache */ "",
/* sounds */ ""
	},	// leave index 0 alone

	{
		"item_armor_shard", 
		"sound/misc/ar1_pkup.wav",
		{ "models/powerups/armor/shard.md3", 
		"models/powerups/armor/shard_sphere.md3",
		NULL, NULL} ,
/* icon */		"icons/iconr_shard",
/* pickup */	"Armor Shard",
/* pickup */	"Кусок брони",
		5,
		IT_ARMOR,
		0,
/* precache */ "",
/* sounds */ ""
	},

	{
		"item_armor_combat", 
		"sound/misc/ar2_pkup.wav",
        { "models/powerups/armor/armor_yel.md3",
		NULL, NULL, NULL},
/* icon */		"icons/iconr_yellow",
/* pickup */	"Armor 50",
/* pickup */	"Броня 50",
		50,
		IT_ARMOR,
		0,
/* precache */ "",
/* sounds */ ""
	},

	{
		"item_armor_body", 
		"sound/misc/ar2_pkup.wav",
        { "models/powerups/armor/armor_red.md3",
		NULL, NULL, NULL},
/* icon */		"icons/iconr_red",
/* pickup */	"Heavy Armor",
/* pickup */	"Тяжелая броня",
		100,
		IT_ARMOR,
		0,
/* precache */ "",
/* sounds */ ""
	},

	{
		"item_health_small",
		"sound/items/s_health.wav",
        { "models/powerups/health/small_cross.md3", 
		"models/powerups/health/small_sphere.md3", 
		NULL, NULL },
/* icon */		"icons/iconh_green",
/* pickup */	"Health 5",
/* pickup */	"Жизни 5",
		5,
		IT_HEALTH,
		0,
/* precache */ "",
/* sounds */ ""
	},

	{
		"item_health",
		"sound/items/n_health.wav",
        { "models/powerups/health/medium_cross.md3", 
		"models/powerups/health/medium_sphere.md3", 
		NULL, NULL },
/* icon */		"icons/iconh_yellow",
/* pickup */	"Health 25",
/* pickup */	"Жизни 25",
		25,
		IT_HEALTH,
		0,
/* precache */ "",
/* sounds */ ""
	},

	{
		"item_health_large",
		"sound/items/l_health.wav",
        { "models/powerups/health/large_cross.md3", 
		"models/powerups/health/large_sphere.md3", 
		NULL, NULL },
/* icon */		"icons/iconh_red",
/* pickup */	"Health 50",
/* pickup */	"Жизни 50",
		50,
		IT_HEALTH,
		0,
/* precache */ "",
/* sounds */ ""
	},

	{
		"item_health_mega",
		"sound/items/m_health.wav",
        { "models/powerups/health/mega_cross.md3", 
		"models/powerups/health/mega_sphere.md3", 
		NULL, NULL },
/* icon */		"icons/iconh_mega",
/* pickup */	"Mega Health",
/* pickup */	"Мега Жизнь",
		100,
		IT_HEALTH,
		0,
/* precache */ "",
/* sounds */ ""
	},

	{
		"weapon_gauntlet", 
		"sound/misc/w_pkup.wav",
        { "models/weapons2/gauntlet/gauntlet.md3",
		NULL, NULL, NULL},
/* icon */		"icons/iconw_gauntlet",
/* pickup */	"Melee",
/* pickup */	"Кулак",
		0,
		IT_WEAPON,
		WP_GAUNTLET,
/* precache */ "",
/* sounds */ ""
	},

	{
		"weapon_shotgun", 
		"sound/misc/w_pkup.wav",
        { "models/weapons2/shotgun/shotgun.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_shotgun",
/* pickup */	"Shotgun",
/* pickup */	"Дробовик",
		10,
		IT_WEAPON,
		WP_SHOTGUN,
/* precache */ "",
/* sounds */ ""
	},

	{
		"weapon_machinegun", 
		"sound/misc/w_pkup.wav",
        { "models/weapons2/machinegun/machinegun.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_machinegun",
/* pickup */	"Machinegun",
/* pickup */	"Автомат",
		40,
		IT_WEAPON,
		WP_MACHINEGUN,
/* precache */ "",
/* sounds */ ""
	},

	{
		"weapon_grenadelauncher",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/grenadel/grenadel.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_grenade",
/* pickup */	"Grenade Launcher",
/* pickup */	"Гранатомёт",
		10,
		IT_WEAPON,
		WP_GRENADE_LAUNCHER,
/* precache */ "",
/* sounds */ "sound/weapons/grenade/hgrenb1a.wav sound/weapons/grenade/hgrenb2a.wav"
	},

	{
		"weapon_rocketlauncher",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/rocketl/rocketl.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_rocket",
/* pickup */	"Rocket Launcher",
/* pickup */	"Ракетница",
		10,
		IT_WEAPON,
		WP_ROCKET_LAUNCHER,
/* precache */ "",
/* sounds */ ""
	},

	{
		"weapon_lightning", 
		"sound/misc/w_pkup.wav",
        { "models/weapons2/lightning/lightning.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_lightning",
/* pickup */	"Lightning Gun",
/* pickup */	"Молния",
		100,
		IT_WEAPON,
		WP_LIGHTNING,
/* precache */ "",
/* sounds */ ""
	},

	{
		"weapon_railgun", 
		"sound/misc/w_pkup.wav",
        { "models/weapons2/railgun/railgun.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_railgun",
/* pickup */	"Railgun",
/* pickup */	"Рэйлган",
		10,
		IT_WEAPON,
		WP_RAILGUN,
/* precache */ "",
/* sounds */ ""
	},

	{
		"weapon_plasmagun", 
		"sound/misc/w_pkup.wav",
        { "models/weapons2/plasma/plasma.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_plasma",
/* pickup */	"Plasma Gun",
/* pickup */	"Плазмаган",
		50,
		IT_WEAPON,
		WP_PLASMAGUN,
/* precache */ "",
/* sounds */ ""
	},

	{
		"weapon_bfg",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/bfg/bfg.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_bfg",
/* pickup */	"BFG10K",
/* pickup */	"БФГ10К",
		20,
		IT_WEAPON,
		WP_BFG,
/* precache */ "",
/* sounds */ ""
	},

	{
		"weapon_grapplinghook",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/grapple/grapple.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_grapple",
/* pickup */	"Grappling Hook",
/* pickup */	"Крюк",
		0,
		IT_WEAPON,
		WP_GRAPPLING_HOOK,
/* precache */ "",
/* sounds */ ""
	},

	{
		"ammo_shells",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/shotgunam.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/icona_shotgun",
/* pickup */	"Shells",
/* pickup */	"Дробь",
		10,
		IT_AMMO,
		WP_SHOTGUN,
/* precache */ "",
/* sounds */ ""
	},

	{
		"ammo_bullets",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/machinegunam.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/icona_machinegun",
/* pickup */	"Bullets",
/* pickup */	"Пули",
		50,
		IT_AMMO,
		WP_MACHINEGUN,
/* precache */ "",
/* sounds */ ""
	},

	{
		"ammo_grenades",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/grenadeam.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/icona_grenade",
/* pickup */	"Grenades",
/* pickup */	"Гранаты",
		5,
		IT_AMMO,
		WP_GRENADE_LAUNCHER,
/* precache */ "",
/* sounds */ ""
	},

	{
		"ammo_cells",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/plasmaam.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/icona_plasma",
/* pickup */	"Cells",
/* pickup */	"Энергоэлемент",
		30,
		IT_AMMO,
		WP_PLASMAGUN,
/* precache */ "",
/* sounds */ ""
	},

	{
		"ammo_lightning",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/lightningam.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/icona_lightning",
/* pickup */	"Lightning",
/* pickup */	"Энергия",
		60,
		IT_AMMO,
		WP_LIGHTNING,
/* precache */ "",
/* sounds */ ""
	},

	{
		"ammo_rockets",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/rocketam.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/icona_rocket",
/* pickup */	"Rockets",
/* pickup */	"Ракеты",
		5,
		IT_AMMO,
		WP_ROCKET_LAUNCHER,
/* precache */ "",
/* sounds */ ""
	},

	{
		"ammo_slugs",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/railgunam.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/icona_railgun",
/* pickup */	"Slugs",
/* pickup */	"Лучи",
		10,
		IT_AMMO,
		WP_RAILGUN,
/* precache */ "",
/* sounds */ ""
	},

	{
		"ammo_bfg",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/bfgam.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/icona_bfg",
/* pickup */	"Bfg Ammo",
/* pickup */	"БФГ Заряд",
		15,
		IT_AMMO,
		WP_BFG,
/* precache */ "",
/* sounds */ ""
	},

	{
		"holdable_teleporter", 
		"sound/items/holdable.wav",
        { "models/powerups/holdable/teleporter.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/teleporter",
/* pickup */	"Personal Teleporter",
/* pickup */	"Телепортер",
		60,
		IT_HOLDABLE,
		HI_TELEPORTER,
/* precache */ "",
/* sounds */ ""
	},

	{
		"holdable_medkit", 
		"sound/items/holdable.wav",
        { 
		"models/powerups/holdable/medkit.md3", 
		"models/powerups/holdable/medkit_sphere.md3",
		NULL, NULL},
/* icon */		"icons/medkit",
/* pickup */	"Medkit",
/* pickup */	"Аптечка",
		60,
		IT_HOLDABLE,
		HI_MEDKIT,
/* precache */ "",
/* sounds */ "sound/items/use_medkit.wav"
	},

	{
		"item_quad", 
		"sound/items/quaddamage.wav",
        { "models/powerups/instant/quad.md3", 
        "models/powerups/instant/quad_ring.md3",
		NULL, NULL },
/* icon */		"icons/quad",
/* pickup */	"Quad Damage",
/* pickup */	"Квадратный урон",
		5,
		IT_POWERUP,
		PW_QUAD,
/* precache */ "",
/* sounds */ "sound/items/damage2.wav sound/items/damage3.wav"
	},

	{
		"item_enviro",
		"sound/items/protect.wav",
        { "models/powerups/instant/enviro.md3", 
		"models/powerups/instant/enviro_ring.md3", 
		NULL, NULL },
/* icon */		"icons/envirosuit",
/* pickup */	"Battle Suit",
/* pickup */	"Боевой щит",
		30,
		IT_POWERUP,
		PW_BATTLESUIT,
/* precache */ "",
/* sounds */ "sound/items/airout.wav sound/items/protect3.wav"
	},

	{
		"item_haste",
		"sound/items/haste.wav",
        { "models/powerups/instant/haste.md3", 
		"models/powerups/instant/haste_ring.md3", 
		NULL, NULL },
/* icon */		"icons/haste",
/* pickup */	"Speed",
/* pickup */	"Скорость",
		30,
		IT_POWERUP,
		PW_HASTE,
/* precache */ "",
/* sounds */ ""
	},

	{
		"item_invis",
		"sound/items/invisibility.wav",
        { "models/powerups/instant/invis.md3", 
		"models/powerups/instant/invis_ring.md3", 
		NULL, NULL },
/* icon */		"icons/invis",
/* pickup */	"Invisibility",
/* pickup */	"Невидимость",
		30,
		IT_POWERUP,
		PW_INVIS,
/* precache */ "",
/* sounds */ ""
	},

	{
		"item_regen",
		"sound/items/regeneration.wav",
        { "models/powerups/instant/regen.md3", 
		"models/powerups/instant/regen_ring.md3", 
		NULL, NULL },
/* icon */		"icons/regen",
/* pickup */	"Regeneration",
/* pickup */	"Регенерация",
		30,
		IT_POWERUP,
		PW_REGEN,
/* precache */ "",
/* sounds */ "sound/items/regen.wav"
	},

	{
		"item_flight",
		"sound/items/flight.wav",
        { "models/powerups/instant/flight.md3", 
		"models/powerups/instant/flight_ring.md3", 
		NULL, NULL },
/* icon */		"icons/flight",
/* pickup */	"Flight",
/* pickup */	"Полет",
		60,
		IT_POWERUP,
		PW_FLIGHT,
/* precache */ "",
/* sounds */ "sound/items/flight.wav"
	},

	{
		"team_CTF_redflag",
		NULL,
        { "models/flags/r_flag.md3",
		NULL, NULL, NULL },
/* icon */		"icons/iconf_red1",
/* pickup */	"Red Flag",
/* pickup */	"Красный флаг",
		0,
		IT_TEAM,
		PW_REDFLAG,
/* precache */ "",
/* sounds */ ""
	},

	{
		"team_CTF_blueflag",
		NULL,
        { "models/flags/b_flag.md3",
		NULL, NULL, NULL },
/* icon */		"icons/iconf_blu1",
/* pickup */	"Blue Flag",
/* pickup */	"Синий флаг",
		0,
		IT_TEAM,
		PW_BLUEFLAG,
/* precache */ "",
/* sounds */ ""
	},

	{
		"holdable_kamikaze", 
		"sound/items/holdable.wav",
        { "models/powerups/kamikazi.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/kamikaze",
/* pickup */	"Kamikaze",
/* pickup */	"Камикадзе",
		60,
		IT_HOLDABLE,
		HI_KAMIKAZE,
/* precache */ "",
/* sounds */ "sound/items/kamikazerespawn.wav"
	},

	{
		"holdable_portal", 
		"sound/items/holdable.wav",
        { "models/powerups/holdable/porter.md3",
		NULL, NULL, NULL},
/* icon */		"icons/portal",
/* pickup */	"Portal",
/* pickup */	"Портал",
		60,
		IT_HOLDABLE,
		HI_PORTAL,
/* precache */ "",
/* sounds */ ""
	},

	{
		"holdable_invulnerability", 
		"sound/items/holdable.wav",
        { "models/powerups/holdable/invulnerability.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/invulnerability",
/* pickup */	"Invulnerability",
/* pickup */	"Оболочка",
		60,
		IT_HOLDABLE,
		HI_INVULNERABILITY,
/* precache */ "",
/* sounds */ ""
	},

	{
		"ammo_nails",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/nailgunam.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/icona_nailgun",
/* pickup */	"Nails",
/* pickup */	"Гвозди",
		20,
		IT_AMMO,
		WP_NAILGUN,
/* precache */ "",
/* sounds */ ""
	},

	{
		"ammo_mines",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/proxmineam.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/icona_proxlauncher",
/* pickup */	"Proximity Mines",
/* pickup */	"Мины",
		10,
		IT_AMMO,
		WP_PROX_LAUNCHER,
/* precache */ "",
/* sounds */ ""
	},

	{
		"ammo_belt",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/chaingunam.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/icona_chaingun",
/* pickup */	"Chaingun Belt",
/* pickup */	"Патроны",
		100,
		IT_AMMO,
		WP_CHAINGUN,
/* precache */ "",
/* sounds */ ""
	},

	{
		"item_scout",
		"sound/items/scout.wav",
        { "models/powerups/scout.md3", 
		NULL, NULL, NULL },
/* icon */		"icons/scout",
/* pickup */	"Scout",
/* pickup */	"Разветчик",
		30,
		IT_PERSISTANT_POWERUP,
		PW_SCOUT,
/* precache */ "",
/* sounds */ ""
	},

	{
		"item_guard",
		"sound/items/guard.wav",
        { "models/powerups/guard.md3", 
		NULL, NULL, NULL },
/* icon */		"icons/guard",
/* pickup */	"Guard",
/* pickup */	"Защитник",
		30,
		IT_PERSISTANT_POWERUP,
		PW_GUARD,
/* precache */ "",
/* sounds */ ""
	},

	{
		"item_doubler",
		"sound/items/doubler.wav",
        { "models/powerups/doubler.md3", 
		NULL, NULL, NULL },
/* icon */		"icons/doubler",
/* pickup */	"Doubler",
/* pickup */	"Стрелок",
		30,
		IT_PERSISTANT_POWERUP,
		PW_DOUBLER,
/* precache */ "",
/* sounds */ ""
	},

	{
		"item_ammoregen",
		"sound/items/ammoregen.wav",
        { "models/powerups/ammo.md3",
		NULL, NULL, NULL },
/* icon */		"icons/ammo_regen",
/* pickup */	"Ammo Regen",
/* pickup */	"Регенерация пуль",
		30,
		IT_PERSISTANT_POWERUP,
		PW_AMMOREGEN,
/* precache */ "",
/* sounds */ ""
	},

	{
		"team_CTF_neutralflag",
		NULL,
        { "models/flags/n_flag.md3",
		NULL, NULL, NULL },
/* icon */		"icons/iconf_neutral1",
/* pickup */	"Neutral Flag",
/* pickup */	"Белый флаг",
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
/* pickup */	"Красный череп",
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
/* pickup */	"Синий череп",
		0,
		IT_TEAM,
		0,
/* precache */ "",
/* sounds */ ""
	},

	{
		"weapon_nailgun", 
		"sound/misc/w_pkup.wav",
        { "models/weapons/nailgun/nailgun.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_nailgun",
/* pickup */	"Nailgun",
/* pickup */	"Гвоздомёт",
		10,
		IT_WEAPON,
		WP_NAILGUN,
/* precache */ "",
/* sounds */ ""
	},

	{
		"weapon_prox_launcher", 
		"sound/misc/w_pkup.wav",
        { "models/weapons/proxmine/proxmine.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_proxlauncher",
/* pickup */	"Prox Launcher",
/* pickup */	"Миномёт",
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

	{
		"weapon_chaingun", 
		"sound/misc/w_pkup.wav",
        { "models/weapons/vulcan/vulcan.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_chaingun",
/* pickup */	"Chaingun",
/* pickup */	"Пулемет",
		80,
		IT_WEAPON,
		WP_CHAINGUN,
/* precache */ "",
/* sounds */ "sound/weapons/vulcan/wvulwind.wav"
	},
	
	{
		"weapon_flamethrower",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/flamethrower/rocketl.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_flamethrower",
/* pickup */	"Flamethrower",
/* pickup */	"Огнемёт",
		100,
		IT_WEAPON,
		WP_FLAMETHROWER,
/* precache */ "",
/* sounds */ ""
	},
	
	{
		"ammo_flame",
		"sound/misc/am_pkup.wav",
        { "models/powerups/ammo/flamethroweram.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/icona_flamethrower",
/* pickup */	"Flame",
/* pickup */	"Пламя",
		50,
		IT_AMMO,
		WP_FLAMETHROWER,
/* precache */ "",
/* sounds */ ""
	},

	{
		"item_armor_vest", 
		"sound/misc/ar2_pkup.wav",
        { "models/powerups/armor/armor_gre.md3", 0, 0, 0},
		"icons/iconr_green",
		"Light Armor",
		"Легкая броня",
		25,
		IT_ARMOR,
		0,
		"",
		""
	},
	
	{
		"item_armor_full", 
		"sound/misc/ar2_pkup.wav",
        { "models/powerups/armor/armor_pur.md3", 0, 0, 0},
		"icons/iconr_full",
		"Full Armor",
		"Полная броня",
		200,
		IT_ARMOR,
		0,
		"",
		""
	},
	
	{
		"weapon_antimatter",
		"sound/misc/w_pkup.wav",
        { "models/weapons2/antimatter/plasma.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_voodoo",
/* pickup */	"Dark Flare",
/* pickup */	"Черная вспышка",
		100,
		IT_WEAPON,
		WP_ANTIMATTER,
/* precache */ "",
/* sounds */ ""
	},

	{
		"item_backpack", 
		"sound/misc/w_pkup.wav",				//TODO: Make dedicated sound?
        { "models/powerups/backpack/backpack.md3", NULL, NULL, NULL},
		"icons/icon_backpack",
		"Backpack",
		"Рюкзак",	
		0,
		IT_BACKPACK,
		1,
		"",
		""
	},

	{
		"holdable_key_red", 
		"sound/items/keycard_03.wav",
        { "models/powerups/keys/keycard-r.md3", NULL, NULL, NULL},
		"icons/key_red",
		"Red keycard",
        "Красная ключ-карта",
		60,
		IT_HOLDABLE,
		HI_KEY_RED,
		"",
		""
	},

	{
		"holdable_key_green", 
		"sound/items/keycard_03.wav",
        { "models/powerups/keys/keycard-g.md3", NULL, NULL, NULL},
		"icons/key_green",
		"Green keycard",
        "Зеленая ключ-карта",
		60,
		IT_HOLDABLE,
		HI_KEY_GREEN,
		"",
		""
	},

	{
		"holdable_key_blue", 
		"sound/items/keycard_03.wav",
        { "models/powerups/keys/keycard-b.md3", NULL, NULL, NULL},
		"icons/key_blue",
		"Blue keycard",
        "Синяя ключ-карта",
		60,
		IT_HOLDABLE,
		HI_KEY_BLUE,
		"",
		""
	},

	{
		"holdable_key_yellow", 
		"sound/items/keycard_03.wav",
        { "models/powerups/keys/keycard-y.md3", NULL, NULL, NULL},
		"icons/key_yellow",
		"Yellow keycard",
        "Желтая ключ-карта",
		60,
		IT_HOLDABLE,
		HI_KEY_YELLOW,
		"",
		""
	},

	{
		"holdable_key_master", 
		"sound/items/key_01+02.wav",
        { "models/powerups/keys/key_master.md3", NULL, NULL, NULL},
		"icons/key_master",
		"Master key",
        "Мастер ключ",
		60,
		IT_HOLDABLE,
		HI_KEY_MASTER,
		"",
		""
	},

	{
		"holdable_key_gold", 
		"sound/items/key_01+02.wav",
        { "models/powerups/keys/key_gold.md3", NULL, NULL, NULL},
		"icons/key_gold",
		"Gold key",
        "Золотой ключ",
		60,
		IT_HOLDABLE,
		HI_KEY_GOLD,
		"",
		""
	},

	{
		"holdable_key_silver", 
		"sound/items/key_01+02.wav",
        { "models/powerups/keys/key_silver.md3", NULL, NULL, NULL},
		"icons/key_silver",
		"Silver key",
        "Серебряный ключ",
		60,
		IT_HOLDABLE,
		HI_KEY_SILVER,
		"",
		""
	},
/*QUAKED holdable_key_iron (.2 .2 .2) (-16 -16 -16) (16 16 16) suspended*/
	{
		"holdable_key_iron", 
		"sound/items/key_01+02.wav",
        { "models/powerups/keys/key_iron.md3", NULL, NULL, NULL},
		"icons/key_iron",
		"Iron key",
        "Железный ключ",
		60,
		IT_HOLDABLE,
		HI_KEY_IRON,
		"",
		""
	},

/*QUAKED team_DD_point
Only in DD games
*/
	{
		"team_DD_pointAblue",
		NULL,
        { "models/dpoints/a_blue.md3",
		NULL, NULL, NULL },
/* icon */		"icons/icona_blue",
/* pickup */	"Point A (Blue)",
/* pickup */	"Точка A (Синяя)",
		0,
		IT_TEAM,
		DD_POINTABLUE,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED team_DD_point
Only in DD games
*/
	{
		"team_DD_pointBblue",
		NULL,
        { "models/dpoints/b_blue.md3",
		NULL, NULL, NULL },
/* icon */		"icons/iconb_blue",
/* pickup */	"Point B (Blue)",
/* pickup */	"Точка B (Синяя)",
		0,
		IT_TEAM,
		DD_POINTBBLUE,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED team_DD_point
Only in DD games
*/
	{
		"team_DD_pointAred",
		NULL,
        { "models/dpoints/a_red.md3",
		NULL, NULL, NULL },
/* icon */		"icons/icona_red",
/* pickup */	"Point A (Red)",
/* pickup */	"Точка A (Красная)",
		0,
		IT_TEAM,
		DD_POINTARED,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED team_DD_point
Only in DD games
*/
	{
		"team_DD_pointBred",
		NULL,
        { "models/dpoints/b_red.md3",
		NULL, NULL, NULL },
/* icon */		"icons/iconb_red",
/* pickup */	"Point B (Red)",
/* pickup */	"Точка B (Красная)",
		0,
		IT_TEAM,
		DD_POINTBRED,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED team_DD_point
Only in DD games
*/
	{
		"team_DD_pointAwhite",
		NULL,
        { "models/dpoints/a_white.md3",
		NULL, NULL, NULL },
/* icon */		"icons/icona_white",
/* pickup */	"Point A (White)",
/* pickup */	"Точка A (Белая)",
		0,
		IT_TEAM,
		DD_POINTAWHITE,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED team_DD_point
Only in DD games
*/
	{
		"team_DD_pointBwhite",
		NULL,
        { "models/dpoints/b_white.md3",
		NULL, NULL, NULL },
/* icon */		"icons/iconb_white",
/* pickup */	"Point B (White)",
/* pickup */	"Точка B (Белая)",
		0,
		IT_TEAM,
		DD_POINTBWHITE,
/* precache */ "",
/* sounds */ ""
	},

//Now things for standard domination:


/*QUAKED 
Only in Domination games
*/
	{
		"team_dom_pointWhite",
		NULL,
        { "models/flags/n_flag.md3",
		NULL, NULL, NULL },
/* icon */		"icons/iconf_neutral1",
/* pickup */	"Neutral domination point",
/* pickup */	"Белый флаг",
		0,
		IT_TEAM,
		DOM_POINTWHITE,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED 
Only in Domination games
*/
	{
		"team_dom_pointRed",
		NULL,
        { "models/flags/r_flag.md3",
		NULL, NULL, NULL },
/* icon */		"icons/iconf_red1",
/* pickup */	"Red domination point",
/* pickup */	"Красный флаг",
		0,
		IT_TEAM,
		DOM_POINTRED,
/* precache */ "",
/* sounds */ ""
	},

/*QUAKED 
Only in Domination games
*/
	{
		"team_dom_pointBlue",
		NULL,
        { "models/flags/b_flag.md3",
		NULL, NULL, NULL },
/* icon */		"icons/iconf_blu1",
/* pickup */	"Blue domination point",
/* pickup */	"Синий флаг",
		0,
		IT_TEAM,
		DOM_POINTBLUE,
/* precache */ "",
/* sounds */ ""
	},
	
/*QUAKED weapon_toolgun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_toolgun", 
		"sound/misc/w_pkup.wav",
        { "models/weapons/toolgun/toolgun.md3",
		NULL, NULL, NULL},
/* icon */		"icons/iconw_toolgun",
/* pickup */	"Toolgun",
/* pickup */	"Тулган",
		0,
		IT_WEAPON,
		WP_TOOLGUN,
/* precache */ "",
/* sounds */ ""
	},
	
/*QUAKED weapon_physgun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_physgun", 
		"sound/misc/w_pkup.wav",
        { "models/weapons/physgun/physgun.md3",
		NULL, NULL, NULL},
/* icon */		"icons/iconw_physgun",
/* pickup */	"Physgun",
/* pickup */	"Физган",
		0,
		IT_WEAPON,
		WP_PHYSGUN,
/* precache */ "",
/* sounds */ ""
	},
	
/*QUAKED weapon_gravitygun (.3 .3 1) (-16 -16 -16) (16 16 16) suspended
*/
	{
		"weapon_gravitygun", 
		"sound/misc/w_pkup.wav",
        { "models/weapons/physgun/physgun.md3",
		NULL, NULL, NULL},
/* icon */		"icons/iconw_gravitygun",
/* pickup */	"Gravitygun",
/* pickup */	"Гравитиган",
		0,
		IT_WEAPON,
		WP_GRAVITYGUN,
/* precache */ "",
/* sounds */ ""
	},

	{
		"weapon_thrower",
		"sound/misc/w_pkup.wav",
        { "models/weapons3/machinegun/machinegun.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_thrower",
/* pickup */	"Thrower",
/* pickup */	"Метатель",
		100,
		IT_WEAPON,
		WP_THROWER,
/* precache */ "",
/* sounds */ ""
	},

	{
		"weapon_bouncer",
		"sound/misc/w_pkup.wav",
        { "models/weapons3/shotgun/shotgun.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_bouncer",
/* pickup */	"Bouncer",
/* pickup */	"Прыгун",
		15,
		IT_WEAPON,
		WP_BOUNCER,
/* precache */ "",
/* sounds */ ""
	},

	{
		"weapon_thunder",
		"sound/misc/w_pkup.wav",
        { "models/weapons3/grenadel/grenadel.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_thunder",
/* pickup */	"Thunder",
/* pickup */	"Гроза",
		10,
		IT_WEAPON,
		WP_THUNDER,
/* precache */ "",
/* sounds */ ""
	},

	{
		"weapon_exploder",
		"sound/misc/w_pkup.wav",
        { "models/weapons3/rocketl/rocketl.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_exploder",
/* pickup */	"Exploder",
/* pickup */	"Взрывник",
		10,
		IT_WEAPON,
		WP_EXPLODER,
/* precache */ "",
/* sounds */ ""
	},

	{
		"weapon_knocker",
		"sound/misc/w_pkup.wav",
        { "models/weapons3/lightning/lightning.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_knocker",
/* pickup */	"Knocker",
/* pickup */	"Швырятель",
		25,
		IT_WEAPON,
		WP_KNOCKER,
/* precache */ "",
/* sounds */ ""
	},

	{
		"weapon_propgun",
		"sound/misc/w_pkup.wav",
        { "models/weapons3/railgun/railgun.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_propgun",
/* pickup */	"Propgun",
/* pickup */	"Пропган",
		20,
		IT_WEAPON,
		WP_PROPGUN,
/* precache */ "",
/* sounds */ ""
	},

	{
		"weapon_regenerator",
		"sound/misc/w_pkup.wav",
        { "models/weapons3/plasma/plasma.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_regenerator",
/* pickup */	"Regenerator",
/* pickup */	"Регенератор",
		50,
		IT_WEAPON,
		WP_REGENERATOR,
/* precache */ "",
/* sounds */ ""
	},

	{
		"weapon_nuke",
		"sound/misc/w_pkup.wav",
        { "models/weapons3/bfg/bfg.md3", 
		NULL, NULL, NULL},
/* icon */		"icons/iconw_nuke",
/* pickup */	"Nuke",
/* pickup */	"Ядерка",
		10,
		IT_WEAPON,
		WP_NUKE,
/* precache */ "",
/* sounds */ ""
	},

	// end of list marker
	{NULL}
};

//look for this to add new ones - WEAPONS_HYPER

int		bg_numItems = sizeof(bg_itemlist) / sizeof(bg_itemlist[0]) - 1;


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
					bg_itemlist[i].giType == IT_PERSISTANT_POWERUP) && 
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
		if ( it->giType == IT_WEAPON && it->giTag == weapon ) {
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
===============
BG_FindClassname
===============
*/
gitem_t		*BG_FindClassname( const char *classname ) {
	gitem_t	*it;
	
	for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
		if ( !Q_stricmp( it->classname, classname ) )
			return it;
	}

	return NULL;
}

/*
===============
BG_CheckClassname
===============
*/
qboolean	BG_CheckClassname( const char *classname ) {
	gitem_t	*it;
	
	for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
		if ( !Q_stricmp( it->classname, classname ) )
			return qtrue;
	}

	return qfalse;
}

/*
===============
BG_FindSwep

===============
*/
gitem_t	*BG_FindSwep( int id ) {
	gitem_t	*it;
	
	for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
		if ( it->giType == IT_WEAPON && it->giTag == id ) {
			return it;
		}
	}
		

	return NULL;
}

/*
===============
BG_FindSwepAmmo

===============
*/
gitem_t	*BG_FindSwepAmmo( int id ) {
	gitem_t	*it;
	
	for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
		if ( it->giType == IT_AMMO && it->giTag == id ) {
			return it;
		}
	}
		

	return NULL;
}

/*
===============
BG_FindBackpack

===============
*/
gitem_t *BG_FindItemForBackpack() {
	gitem_t	*it;
	
	for ( it = bg_itemlist + 1 ; it->classname ; it++ ) {
		if ( it->giType == IT_BACKPACK ) {
			return it;
		}
	}

	Com_Error( ERR_DROP, "Couldn't find bg_itemlist definition for backpack" );
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
	int		upperBound;

	if ( ent->modelindex < 1 || ent->modelindex >= bg_numItems ) {
		Com_Printf( "BG_CanItemBeGrabbed: index out of range\n");
		//Com_Error( ERR_DROP, "BG_CanItemBeGrabbed: index out of range" );
	}

	item = &bg_itemlist[ent->modelindex];

	switch( item->giType ) {
	case IT_BACKPACK:
		return qtrue;	// backpack can always be picked up
	case IT_WEAPON:
		if(ps->stats[STAT_NO_PICKUP]==1){ return qfalse; }
		return qtrue;	// weapons are always picked up

	case IT_AMMO:
		// oatmeal begin
		if(ps->stats[STAT_NO_PICKUP]==1){ return qfalse; }
		// oatmeal end
		if ( ps->stats[STAT_SWEPAMMO] >= mod_ammolimit ) {
			return qfalse;		// can't hold any more
		}
		return qtrue;

	case IT_ARMOR:
		// oatmeal begin
		if(ps->stats[STAT_NO_PICKUP]==1){ return qfalse; }
		// oatmeal end
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
		return qtrue;

	case IT_HEALTH:
		// oatmeal begin
		if(ps->stats[STAT_NO_PICKUP]==1){ return qfalse; }
		// oatmeal end

		// small and mega healths will go over the max, otherwise
		// don't pick up if already at max
		if( bg_itemlist[ps->stats[STAT_PERSISTANT_POWERUP]].giTag == PW_GUARD ) {
			upperBound = ps->stats[STAT_MAX_HEALTH];
		}
		else
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
		if(ps->stats[STAT_NO_PICKUP]==1){ return qfalse; }
		return qtrue;	// powerups are always picked up

	case IT_PERSISTANT_POWERUP:
		if(ps->stats[STAT_NO_PICKUP]==1){ return qfalse; }

		//In Double D we don't want persistant Powerups (or maybe, can be discussed)
		if(gametype == GT_DOUBLE_D)
			return qfalse;

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

	case IT_TEAM: // team items, such as flags	
		if(ps->stats[STAT_NO_PICKUP]==1){ return qfalse; }
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
		if( gametype == GT_CTF || gametype == GT_CTF_ELIMINATION) {
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

		if( gametype == GT_DOUBLE_D) {
			//We can touch both flags
			if(item->giTag == PW_BLUEFLAG || item->giTag == PW_REDFLAG)
				return qtrue;
		}

		if( gametype == GT_DOMINATION ) {
			if(item->giTag == DOM_POINTWHITE)
				return qtrue;
			if (ps->persistant[PERS_TEAM] == TEAM_RED) {
				if(item->giTag == DOM_POINTBLUE)
					return qtrue;
			} else if (ps->persistant[PERS_TEAM] == TEAM_BLUE) {
				if(item->giTag == DOM_POINTRED)
					return qtrue;
			}
		}

		if( gametype == GT_HARVESTER ) {
			return qtrue;
		}
		return qfalse;

	case IT_HOLDABLE:
		if(ps->stats[STAT_NO_PICKUP]==1){ return qfalse; }
		// can only hold one item at a time
		if ( item->giTag < HI_HOLDABLE_SPLIT && GetPlayerHoldable( ps->stats[STAT_HOLDABLE_ITEM] ) != HI_NONE ) {
			//player tries to pick up a holdable of which only one can be held while he already has one
			return qfalse;
		}
		else {
			//player tries to pick up a key. Do not allow pickup if that specific key is already in possession
			if ( ps->stats[STAT_HOLDABLE_ITEM] & (1 << item->giTag) )
				return qfalse;
		}
		return qtrue;

        case IT_BAD:
		Com_Printf( "BG_CanItemBeGrabbed: index out of range\n");
        default:
#ifndef Q3_VM
#ifndef NDEBUG // bk0001204
		Com_Printf( "BG_CanItemBeGrabbed: index out of range\n");
#endif
#endif
         break;
	}

	return qfalse;
}

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
		result[2] -= 0.5 * mod_gravity * deltaTime * deltaTime;		// FIXME: local gravity...
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
	case TR_GRAVITY_WATER:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5 * (mod_gravity*0.50) * deltaTime * deltaTime;		// FIXME: local gravity...
		break;
	default:
		//Com_Error( ERR_DROP, "BG_EvaluateTrajectory: unknown trType: %i", tr->trTime );
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
		result[2] -= mod_gravity * deltaTime;		// FIXME: local gravity...
		break;
	case TR_GRAVITY_WATER:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorCopy( tr->trDelta, result );
		result[2] -= (mod_gravity*0.50) * deltaTime;		// FIXME: local gravity...
		break;
	default:
		//Com_Error( ERR_DROP, "BG_EvaluateTrajectoryDelta: unknown trType: %i", tr->trTime );
		break;
	}
}

/*
================
ST_EvaluateTrajectory

================
*/
void ST_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result, int mass ) {
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
		result[2] -= 0.5 * (mod_gravity * (mass/800)) * deltaTime * deltaTime;		// FIXME: local gravity...
		break;
	case TR_GRAVITY_WATER:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
		result[2] -= 0.5 * (mod_gravity * ((mass*0.50)/800)) * deltaTime * deltaTime;		// FIXME: local gravity...
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
		//Com_Error( ERR_DROP, "ST_EvaluateTrajectory: unknown trType: %i", tr->trTime );
		break;
	}
}

/*
================
ST_EvaluateTrajectoryDelta

For determining velocity at a given time
================
*/
void ST_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result, int mass ) {
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
		result[2] -= (mod_gravity * (mass/800)) * deltaTime;		// FIXME: local gravity...
		break;
	case TR_GRAVITY_WATER:
		deltaTime = ( atTime - tr->trTime ) * 0.001;	// milliseconds to seconds
		VectorCopy( tr->trDelta, result );
		result[2] -= (mod_gravity * (mass*0.50)/800) * deltaTime;		// FIXME: local gravity...
		break;
	default:
		//Com_Error( ERR_DROP, "ST_EvaluateTrajectoryDelta: unknown trType: %i", tr->trTime );
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
	"EV_JUMP_PAD",			// boing sound at origin, jump sound on player
	"EV_JUMP",                        //Event 14
	"EV_WATER_TOUCH",	// foot touches
	"EV_WATER_LEAVE",	// foot leaves
	"EV_WATER_UNDER",	// head touches
	"EV_WATER_CLEAR",	// head leaves
	"EV_ITEM_PICKUP",			// normal item pickups are predictable
	"EV_GLOBAL_ITEM_PICKUP",	// powerup / team sounds are broadcast to everyone
	"EV_NOAMMO",
	"EV_CHANGE_WEAPON",
	"EV_FIRE_WEAPON",
	"EV_USE_ITEM0",                   //Event 24
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
	"EV_ITEM_RESPAWN",                //Event 40
	"EV_ITEM_POP",
	"EV_PLAYER_TELEPORT_IN",
	"EV_PLAYER_TELEPORT_OUT",
	"EV_GRENADE_BOUNCE",		// eventParm will be the soundindex
	"EV_GENERAL_SOUND",
	"EV_GLOBAL_SOUND",		// no attenuation
	"EV_GLOBAL_TEAM_SOUND",
	"EV_BULLET_HIT_FLESH",
	"EV_BULLET_HIT_WALL",
	"EV_MISSILE_HIT",                 //Event 50
	"EV_MISSILE_MISS",
	"EV_MISSILE_MISS_METAL",
	"EV_RAILTRAIL",
	"EV_SHOTGUN",
	"EV_BULLET",				// otherEntity is the shooter
	"EV_PAIN",
	"EV_PAINVEHICLE",
	"EV_DEATH1",
	"EV_DEATH2",
	"EV_DEATH3",
	"EV_OBITUARY",                    //Event 60
	"EV_POWERUP_QUAD",
	"EV_POWERUP_BATTLESUIT",
	"EV_POWERUP_REGEN",
	"EV_GIB_PLAYER",			// gib a previously living player
	"EV_SCOREPLUM",			// score plum
	"EV_PROXIMITY_MINE_STICK",
	"EV_PROXIMITY_MINE_TRIGGER",
	"EV_KAMIKAZE",			// kamikaze explodes
	"EV_OBELISKEXPLODE",		// obelisk explodes
	"EV_OBELISKPAIN",			// obelisk is in pain
	"EV_INVUL_IMPACT",		// invulnerability sphere impact
	"EV_JUICED",				// invulnerability juiced effect
	"EV_LIGHTNINGBOLT",		// lightning bolt bounced of invulnerability sphere
	"EV_DEBUG_LINE",
	"EV_STOPLOOPINGSOUND",
	"EV_TAUNT",
	"EV_TAUNT_YES",
	"EV_TAUNT_NO",
	"EV_TAUNT_FOLLOWME",
	"EV_TAUNT_GETFLAG",
	"EV_TAUNT_GUARDBASE",
	"EV_TAUNT_PATROL",
	"EV_EMIT_DEBRIS_LIGHT",		// emit light concrete chunks
	"EV_EMIT_DEBRIS_DARK",		// emit dark concrete chunks
	"EV_EMIT_DEBRIS_LIGHT_LARGE",	// emit light large concrete chunks
	"EV_EMIT_DEBRIS_DARK_LARGE",	// emit dark large concrete chunks
	"EV_EMIT_DEBRIS_WOOD",		// emit wooden chunks
	"EV_EMIT_DEBRIS_FLESH",		// emit gibs
	"EV_EMIT_DEBRIS_GLASS",		// emite shards of glass
	"EV_EMIT_DEBRIS_STONE",		// emit chunks of stone
	"EV_EARTHQUAKE",
	"EV_EXPLOSION",
	"EV_PARTICLES_GRAVITY",
	"EV_PARTICLES_LINEAR",
	"EV_PARTICLES_LINEAR_UP",
	"EV_PARTICLES_LINEAR_DOWN",
	"EV_OVERLAY",
	"EV_SMOKEPUFF",
	"EV_WATERPUFF",
	"EV_SILENT_ITEM_PICKUP",		// item pickup without pickup sound
	"EV_FOOTSTEP_FLESH",
	"EV_HORN",		//VEHICLES
	"EV_CRASH25",
	"EV_OT1_IMPACT",
	"EV_GRAVITYSOUND"
};

/*
===============
BG_AddPredictableEventToPlayerstate

Handles the sequence numbers
===============
*/

void	trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );

void BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps ) {

	switch(newEvent) {
		case EV_FOOTSTEP:
		case EV_FOOTSTEP_METAL:
		case EV_FOOTSTEP_FLESH:
		case EV_FALL_MEDIUM:
		case EV_FALL_FAR:
		case EV_JUMP:
		case EV_TAUNT:
		case EV_HORN:
		case EV_NOAMMO:
		case EV_CHANGE_WEAPON:
		case EV_FIRE_WEAPON:
			ps->eFlags |= EF_HEARED;
			ps->pm_time = 5;
			break;
	}

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
void BG_TouchJumpPad( playerState_t *ps, entityState_t *jumppad, qboolean playsound ) {
	vec3_t	angles;
	float p;
	int effectNum;

	// spectators don't use jump pads
	if ( ps->pm_type != PM_NORMAL ) {
		return;
	}

	// flying characters don't hit bounce pads
	if ( ps->powerups[PW_FLIGHT] ) {
		return;
	}

	// if we didn't hit this same jumppad the previous frame
	// then don't play the event sound again if we are in a fat trigger
	if ( playsound == qtrue && ps->jumppad_ent != jumppad->number ) {

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

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR ) {
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
	VectorCopy( ps->velocity, s->pos.trDelta );

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy( ps->viewangles, s->apos.trBase );
	if ( snap ) {
		SnapVector( s->apos.trBase );
	}

	s->angles2[YAW] = ps->movementDir;
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

	s->weapon = ps->generic2;
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( ps->powerups[ i ] ) {
			s->powerups |= 1 << i;
		}
	}

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;
}

/*
========================
BG_PlayerStateToEntityStateExtraPolate

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s, int time, qboolean snap ) {
	int		i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR ) {
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
	VectorCopy( ps->velocity, s->pos.trDelta );
	// set the time for linear prediction
	s->pos.trTime = time;
	// set maximum extra polation time
	s->pos.trDuration = 50; // 1000 / sv_fps (default = 20)

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy( ps->viewangles, s->apos.trBase );
	if ( snap ) {
		SnapVector( s->apos.trBase );
	}

	s->angles2[YAW] = ps->movementDir;
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

	s->weapon = ps->generic2;
	s->groundEntityNum = ps->groundEntityNum;

	s->powerups = 0;
	for ( i = 0 ; i < MAX_POWERUPS ; i++ ) {
		if ( ps->powerups[ i ] ) {
			s->powerups |= 1 << i;
		}
	}

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;
}

/*
============
BG_TeamName
KK-OAX Copied from Tremulous
============
*/
char *BG_TeamName( team_t team )
{
  if( team == TEAM_NONE )
    return "spectator";
  if( team == TEAM_RED )
    return "Red";
  if( team == TEAM_BLUE )
    return "Blue";
  if( team == TEAM_FREE )
    return "Free For All";
  return "<team>";
}

int rq3_random(int min, int max)
{
	int number;
	number = (rand() % (max - min + 1)) + (min);
	return number;
}

/*
==================
GetPlayerHoldable( int stat_holdable )
Determines what unique holdable item the player is carrying.
==================
*/
int GetPlayerHoldable( int stat_holdable ) {

	if ( stat_holdable & (1 << HI_TELEPORTER) )
		return HI_TELEPORTER;

	if ( stat_holdable & (1 << HI_MEDKIT) )
		return HI_MEDKIT;

	if ( stat_holdable & (1 << HI_KAMIKAZE) )
		return HI_KAMIKAZE;

	if ( stat_holdable & (1 << HI_PORTAL) )
		return HI_PORTAL;

	if ( stat_holdable & (1 << HI_INVULNERABILITY) )
		return HI_INVULNERABILITY;
	
	return HI_NONE;
}

/*
==================
GetHoldableListIndex( int giTag )
Determines what the index is of a holdable in the bg_itemlist
==================
*/
int GetHoldableListIndex( int giTag ) {
	int i;

	if ( giTag <= HI_NONE || giTag >= HI_NUM_HOLDABLE || giTag == HI_HOLDABLE_SPLIT )
		return 0;

	for (i = 0; i < bg_numItems; i++ ){
		if ( bg_itemlist[i].giType == IT_HOLDABLE && bg_itemlist[i].giTag == giTag )
			return i;
	}

	return 0;
}

int BG_VehicleCheckClass (int id){
	if(id == 1){ return VCLASS_CAR; }
	if(id == 2){ return VCLASS_CAR; }
	
	return 0;
}

float BG_GetVehicleSettings (int id, int set){
	if(id == 1){
		if(set==VSET_SPEED){ return 900; }
		if(set==VSET_GRAVITY){ return 0.4; }
		if(set==VSET_WEAPON){ return 0; }
		if(set==VSET_WEAPONRATE){ return 3; }
	}
	if(id == 2){
		if(set==VSET_SPEED){ return 900; }
		if(set==VSET_GRAVITY){ return 0.4; }
		if(set==VSET_WEAPON){ return 1; }
		if(set==VSET_WEAPONRATE){ return 3; }
	}
	
	return 0;
}
