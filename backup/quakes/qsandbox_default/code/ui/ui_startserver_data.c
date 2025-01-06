/*
=============================================================================

START SERVER GLOBAL DATA USED BY UI AND SCRIPT *****

=============================================================================
*/


#include "ui_local.h"

#include "ui_startserver.h"


//
// global data
//

scriptdata_t s_scriptdata;

char* gametype_cvar_base[NUM_GAMETYPES] = {
	"uie_sandbox_%s",
	"uie_ffa_%s",
	"uie_single_%s",
	"uie_tourney_%s",
	"uie_team_%s",
	"uie_ctf_%s",
	"uie_oneflag_%s",
	"uie_overload_%s",
	"uie_harvester_%s",
	"uie_elim_%s",
	"uie_ctfelim_%s",
	"uie_lms_%s",
	"uie_doubledom_%s",
	"uie_domination_%s"
};

const char* idmap_list[] = {
	"gm_construct",
	"gm_flatgrass",
	"qs_construct",
	"qs_flatgrass",
	0
};

const char *gametype_items[NUM_GAMETYPES + 1] = {
	"Sandbox",
	"Free For All",
	"Single Player",
	"Tournament",
	"Team Deathmatch",
	"Capture the Flag",
	"One Flag Capture",
	"Overload",
	"Harvester",
	"Elimination",
	"CTF Elimination",
	"Last Man Standing",
	"Double Domination",
	"Domination",
	0
};

const char *gametype_itemsru[NUM_GAMETYPES + 1] = {
	"Песочница",
	"Все Против Всех",
	"Одиночная Игра",
	"Турнир",
	"Командный Бой",
	"Захват флага",
	"Один Флаг",
	"Атака Базы",
	"Жнец",
	"Устранение",
	"Устранение: Захват флага",
	"Последний оставшийся",
	"Двойное доминирование",
	"Доминирование",
	0
};

//
// individual items
// Must be in ITEM_* numerical order
//
itemnode_t server_itemlist[ITEM_COUNT] = {
	// weapon
	{ ITEMGROUP_WEAPON, ITEM_GRAPPLING_HOOK, 		"weapon_grapplinghook", 	"gh"},
	{ ITEMGROUP_WEAPON, ITEM_MGUN, 		"weapon_machinegun", 	"mg"},
	{ ITEMGROUP_WEAPON, ITEM_SHOTGUN, 	"weapon_shotgun", 		"sg"},
	{ ITEMGROUP_WEAPON, ITEM_GRENADE, 	"weapon_grenadelauncher", "gl"},
	{ ITEMGROUP_WEAPON, ITEM_ROCKET, 	"weapon_rocketlauncher", "rl"},
	{ ITEMGROUP_WEAPON, ITEM_PLASMA, 	"weapon_plasmagun", 	"pg"},
	{ ITEMGROUP_WEAPON, ITEM_LIGHTNING, "weapon_lightning", 	"lg"},
	{ ITEMGROUP_WEAPON, ITEM_RAIL, 		"weapon_railgun", 		"rg"},
	{ ITEMGROUP_WEAPON, ITEM_BFG, 		"weapon_bfg", 			"bfg"},
	{ ITEMGROUP_WEAPON, ITEM_NAILGUN, 		"weapon_nailgun", 			"ng"},
	{ ITEMGROUP_WEAPON, ITEM_PROX_LAUNCHER, 		"weapon_prox_launcher", 			"pl"},
	{ ITEMGROUP_WEAPON, ITEM_CHAINGUN, 		"weapon_chaingun", 			"cg"},
	{ ITEMGROUP_WEAPON, ITEM_FLAMETHROWER, 		"weapon_flamethrower", 			"ft"},
	{ ITEMGROUP_WEAPON, ITEM_ANTIMATTER, 		"weapon_antimatter", 			"am"},

	// ammo
	{ ITEMGROUP_AMMO, ITEM_MGUN_AMMO, 	"ammo_bullets", 		"amg"},
	{ ITEMGROUP_AMMO, ITEM_SHOTGUN_AMMO, "ammo_shells", 		"asg"},
	{ ITEMGROUP_AMMO, ITEM_GRENADE_AMMO, "ammo_grenades", 		"agl"},
	{ ITEMGROUP_AMMO, ITEM_ROCKET_AMMO, "ammo_rockets", 		"arl"},
	{ ITEMGROUP_AMMO, ITEM_PLASMA_AMMO, "ammo_cells", 			"apg"},
	{ ITEMGROUP_AMMO, ITEM_LIGHTNING_AMMO, "ammo_lightning", 	"alg"},
	{ ITEMGROUP_AMMO, ITEM_RAIL_AMMO, 	"ammo_slugs", 			"arg"},
	{ ITEMGROUP_AMMO, ITEM_BFG_AMMO, 	"ammo_bfg", 			"abfg"},
	{ ITEMGROUP_AMMO, ITEM_NAILGUN_AMMO, 	"ammo_nails", 			"abfg"},
	{ ITEMGROUP_AMMO, ITEM_PROX_LAUNCHER_AMMO, 	"ammo_mines", 			"abfg"},
	{ ITEMGROUP_AMMO, ITEM_CHAINGUN_AMMO, 	"ammo_belt", 			"abfg"},
	{ ITEMGROUP_AMMO, ITEM_FLAMETHROWER_AMMO, 	"ammo_flame", 			"aft"},

	// health
	{ ITEMGROUP_HEALTH, ITEM_MEGA,		"item_health_mega",		"mega"},
	{ ITEMGROUP_HEALTH, ITEM_HEALTH_SMALL, "item_health_small",	"hs"},

	{ ITEMGROUP_HEALTH, ITEM_HEALTH_MEDIUM, "item_health", 		"hm"},
	{ ITEMGROUP_HEALTH, ITEM_HEALTH_LARGE, "item_health_large",	"hl"},

	// armour
	{ ITEMGROUP_ARMOUR, ITEM_ARMOUR_PURPLE, "item_armor_full",	"ap"},
	{ ITEMGROUP_ARMOUR, ITEM_ARMOUR_RED,	"item_armor_body",		"ar"},
	{ ITEMGROUP_ARMOUR, ITEM_ARMOUR_YELLOW, "item_armor_combat",	"ay"},
	{ ITEMGROUP_ARMOUR, ITEM_ARMOUR_GREEN, "item_armor_vest",	"ag"},
	{ ITEMGROUP_ARMOUR, ITEM_ARMOUR_SHARD, "item_armor_shard",	"as"},

	// powerups
	{ ITEMGROUP_POWERUPS, ITEM_QUAD,		"item_quad",			"quad"},
	{ ITEMGROUP_POWERUPS, ITEM_REGEN,		"item_regen",			"reg"},
	{ ITEMGROUP_POWERUPS, ITEM_HASTE,		"item_haste",			"hst"},

	{ ITEMGROUP_POWERUPS, ITEM_BATTLESUIT,	"item_enviro",			"bs"},
	{ ITEMGROUP_POWERUPS, ITEM_INVISIBLE, 	"item_invis",			"inv"},
	{ ITEMGROUP_POWERUPS, ITEM_FLIGHT,		"item_flight",			"flt"},

	{ ITEMGROUP_POWERUPS, ITEM_SCOUT,	"item_scout",			"sc"},
	{ ITEMGROUP_POWERUPS, ITEM_GUARD, 	"item_guard",			"gur"},
	{ ITEMGROUP_POWERUPS, ITEM_DOUBLER,		"item_doubler",			"doub"},
	{ ITEMGROUP_POWERUPS, ITEM_AMMOREGEN,	"item_ammoregen",			"amreg"},

	// items
	{ ITEMGROUP_HOLDABLE,	ITEM_MEDKIT, 		"holdable_medkit",		"mk"},
	{ ITEMGROUP_HOLDABLE,	ITEM_TELEPORTER,	"holdable_teleporter",	"tel"},
	{ ITEMGROUP_HOLDABLE,	ITEM_KAMIKAZE, 		"holdable_kamikaze",		"kmz"},
	{ ITEMGROUP_HOLDABLE,	ITEM_INVULNERABILITY,	"holdable_invulnerability",	"invul"},
	{ ITEMGROUP_HOLDABLE,	ITEM_PORTAL, 		"holdable_portal",		"pt"}
};




/*
==========================================================================

DATA ARCHIVE

==========================================================================
*/


// Some of these vars are legacy cvars that were previously stored in q3config.cfg.
// In order to load them into memory for porting over to the new data format
// we need to "register" them and load their current values

// This porting process should be done only once, assuming we installed over
// a previous version of UIE


// Default values for cvars that we haven't yet created

// On startup we load from SourceTechui.dat, check for new vars from this list,
// and add them into data storage.





typedef struct {
	char		*cvarName;
	char		*defaultString;
} uie_cvarTable_t;


uie_cvarTable_t uie_cvarTable[] = {
{ "uie_gametype", "0" },
{ "uie_pure", "0" },
{ "uie_dynamiclight", "0" },
{ "uie_dedicated", "0" },
{ "uie_inactivity", "0" },
{ "uie_config_showid", "0" },
{ "uie_pmove_fixed", "1" },
{ "uie_pmove_msec", "11" },
{ "uie_smoothclients", "1" },
{ "uie_syncclients", "0" },
{ "uie_allowmaxrate", "0" },
{ "uie_maxrate", "0" },
{ "uie_allowdownload", "0" },
{ "uie_allowpass", "0" },
{ "uie_password", "" },
{ "uie_allowvote", "1" },
{ "uie_minPing", "0" },
{ "uie_maxPing", "0" },
{ "uie_allowMinPing", "0" },
{ "uie_allowMaxPing", "0" },
{ "uie_preventConfigBug", "0" },
{ "uie_gravity", "800" },
{ "uie_jumpheight", "270" },
{ "uie_knockback", "1000" },
{ "uie_quadfactor", "3" },
{ "uie_netport", "27960" },
{ "uie_svfps", "60" },
{ "uie_allowprivateclients", "0" },
{ "uie_privateclients", "0" },
{ "uie_privatepassword", "" },
{ "uie_strictAuth", "1" },
{ "uie_lanForceRate", "1" },

{ "uie_maxEntities", "1024" },
{ "uie_singleskill", "1" },
{ "uie_kill", "1" },
{ "uie_damageModifier", "1" },
{ "uie_selectedmod", "default" },
{ "uie_elimination", "0" },
{ "uie_obeliskHealth", "2500" },
{ "uie_obeliskRegenPeriod", "1" },
{ "uie_obeliskRegenAmount", "15" },
{ "uie_obeliskRespawnDelay", "10" },
{ "uie_cubeTimeout", "30" },
{ "uie_flagrespawn", "-1" },
{ "uie_weaponTeamRespawn", "30" },
{ "uie_elimination_ctf_oneway", "0" },
{ "uie_elimination_selfdamage", "0" },
{ "uie_elimination_roundtime", "600" },
{ "uie_elimination_warmup", "0" },
{ "uie_elimination_activewarmup", "0" },
{ "uie_lms_lives", "1" },
{ "uie_lms_mode", "0" },
{ "uie_accelerate", "1" },
{ "uie_spectatorspeed", "700" },
{ "uie_speed", "320" },
{ "uie_gravity", "800" },
{ "uie_gravityModifier", "1" },
{ "uie_knockback", "1000" },
{ "uie_noplayerclip", "0" },
{ "uie_jumpheight", "270" },
{ "uie_regenarmor", "0" },
{ "uie_ammolimit", "9000" },
{ "uie_quadfactor", "3" },
{ "uie_respawntime", "0" },
{ "uie_forcerespawn", "0" },
{ "uie_vampire", "0.0" },
{ "uie_vampire_max_health", "500" },
{ "uie_regen", "0" },
{ "uie_maxweaponpickup", "1" },
{ "uie_droppeditemtime", "30" },
{ "uie_autoflagreturn", "30" },
{ "uie_armorprotect", "0.66" },
{ "uie_respawnwait", "3000" },
{ "uie_speedfactor", "1.3" },
{ "uie_scoutspeedfactor", "1.5" },
{ "uie_scoutfirespeed", "0.60" },
{ "uie_scoutdamagefactor", "1" },
{ "uie_scoutgravitymodifier", "1" },
{ "uie_scout_infammo", "0" },
{ "uie_scouthealthmodifier", "1" },
{ "uie_doublerfirespeed", "1" },
{ "uie_doublerdamagefactor", "2.5" },
{ "uie_doublerspeedfactor", "1" },
{ "uie_doublergravitymodifier", "1" },
{ "uie_doubler_infammo", "0" },
{ "uie_doublerhealthmodifier", "1" },
{ "uie_guardhealthmodifier", "1.5" },
{ "uie_guardfirespeed", "1" },
{ "uie_guarddamagefactor", "1" },
{ "uie_guardspeedfactor", "1" },
{ "uie_guardgravitymodifier", "1" },
{ "uie_guard_infammo", "0" },
{ "uie_ammoregenfirespeed", "0.65" },
{ "uie_ammoregen_infammo", "0" },
{ "uie_ammoregendamagefactor", "1" },
{ "uie_ammoregenspeedfactor", "1" },
{ "uie_ammoregengravitymodifier", "1" },
{ "uie_ammoregenhealthmodifier", "1" },
{ "uie_teamred_speed", "320" },
{ "uie_teamred_gravityModifier", "1" },
{ "uie_teamred_firespeed", "1" },
{ "uie_teamred_damage", "1" },
{ "uie_teamred_infammo", "0" },
{ "uie_teamred_respawnwait", "3000" },
{ "uie_teamred_pickupitems", "1" },
{ "uie_eliminationredrespawn", "0" },
{ "uie_eliminationred_startHealth", "100" },
{ "uie_eliminationred_startArmor", "0" },
{ "uie_teamblue_speed", "320" },
{ "uie_teamblue_gravityModifier", "1" },
{ "uie_teamblue_firespeed", "1" },
{ "uie_teamblue_damage", "1" },
{ "uie_teamblue_infammo", "0" },
{ "uie_teamblue_respawnwait", "3000" },
{ "uie_teamblue_pickupitems", "1" },
{ "uie_eliminationrespawn", "0" },
{ "uie_elimination_startHealth", "100" },
{ "uie_elimination_startArmor", "0" },
{ "uie_eliminationred_grapple", "0" },
{ "uie_eliminationred_gauntlet", "1" },
{ "uie_eliminationred_machinegun", "500" },
{ "uie_eliminationred_shotgun", "0" },
{ "uie_eliminationred_grenade", "0" },
{ "uie_eliminationred_rocket", "0" },
{ "uie_eliminationred_railgun", "0" },
{ "uie_eliminationred_lightning", "0" },
{ "uie_eliminationred_plasmagun", "0" },
{ "uie_eliminationred_bfg", "0" },
{ "uie_eliminationred_chain", "0" },
{ "uie_eliminationred_mine", "0" },
{ "uie_eliminationred_nail", "0" },
{ "uie_eliminationred_flame", "0" },
{ "uie_eliminationred_antimatter", "0" },
{ "uie_elimination_grapple", "0" },
{ "uie_elimination_gauntlet", "1" },
{ "uie_elimination_machinegun", "500" },
{ "uie_elimination_shotgun", "0" },
{ "uie_elimination_grenade", "0" },
{ "uie_elimination_rocket", "0" },
{ "uie_elimination_railgun", "0" },
{ "uie_elimination_lightning", "0" },
{ "uie_elimination_plasmagun", "0" },
{ "uie_elimination_bfg", "0" },
{ "uie_elimination_chain", "0" },
{ "uie_elimination_mine", "0" },
{ "uie_elimination_nail", "0" },
{ "uie_elimination_flame", "0" },
{ "uie_elimination_antimatter", "0" },
{ "uie_eliminationred_quad", "0" },
{ "uie_eliminationred_haste", "0" },
{ "uie_eliminationred_bsuit", "0" },
{ "uie_eliminationred_invis", "0" },
{ "uie_eliminationred_regen", "0" },
{ "uie_eliminationred_flight", "0" },
{ "uie_eliminationred_holdable", "0" },
{ "uie_elimination_quad", "0" },
{ "uie_elimination_haste", "0" },
{ "uie_elimination_bsuit", "0" },
{ "uie_elimination_invis", "0" },
{ "uie_elimination_regen", "0" },
{ "uie_elimination_flight", "0" },
{ "uie_elimination_items", "0" },
{ "uie_elimination_holdable", "0" },
{ "uie_minigame", "0" },
{ "uie_overlay", "0" },
{ "uie_randomItems", "0" },
{ "uie_slickmove", "0" },
{ "uie_spawnprotect", "500" },
{ "uie_elimination_lockspectator", "0" },
{ "uie_awardpushing", "1" },
{ "uie_randomteleport", "0" },
{ "uie_falldamagesmall", "5"  },
{ "uie_falldamagebig", "10" },
{ "uie_waterdamage", "0" },
{ "uie_lavadamage", "30" },
{ "uie_slimedamage", "10" },
{ "uie_drowndamage", "1" },
{ "uie_invulinf", "0" },
{ "uie_invulmove", "0" },
{ "uie_invultime", "10" },
{ "uie_kamikazeinf", "0" },
{ "uie_portalinf", "0" },
{ "uie_portaltimeout", "120" },
{ "uie_portalhealth", "200" },
{ "uie_teleporterinf", "0" },
{ "uie_medkitinf", "0" },
{ "uie_medkitlimit", "200" },
{ "uie_medkitmodifier", "100" },
{ "uie_fasthealthregen", "15" },
{ "uie_slowhealthregen", "5" },
{ "uie_hastefirespeed", "0.65" },
{ "uie_quadtime", "30" },
{ "uie_bsuittime", "30" },
{ "uie_hastetime", "30" },
{ "uie_invistime", "30" },
{ "uie_regentime", "30" },
{ "uie_flighttime", "60" },
{ "uie_armorrespawn", "25" },
{ "uie_healthrespawn", "35" },
{ "uie_ammorespawn", "30" },
{ "uie_holdablerespawn", "60" },
{ "uie_megahealthrespawn", "35" },
{ "uie_poweruprespawn", "120" },
{ "uie_weaponrespawn", "5" },

// Крюк
{ "uie_ghspeed", "800" },
{ "uie_ghtimeout", "10000" },
// Пила
{ "uie_gdelay", "400" },
{ "uie_gdamage", "100" },
{ "uie_grange", "32" },
{ "uie_gknockback", "1" },
// Автомат
{ "uie_mgammocount", "-1" },
{ "uie_mgweaponcount", "-1" },
{ "uie_mgdelay", "100" },
{ "uie_mgdamage", "7" },
{ "uie_mgspread", "200" },
{ "uie_mgexplode", "0" },
{ "uie_mgsdamage", "50" },
{ "uie_mgsradius", "70" },
{ "uie_mgvampire", "0" },
{ "uie_mginf", "0" },
{ "uie_mgknockback", "1" },
// Дробовик
{ "uie_sgammocount", "-1" },
{ "uie_sgweaponcount", "-1" },
{ "uie_sgdelay", "1000" },
{ "uie_sgdamage", "10" },
{ "uie_sgspread", "700" },
{ "uie_sgexplode", "0" },
{ "uie_sgsdamage", "45" },
{ "uie_sgsradius", "35" },
{ "uie_sgcount", "11" },
{ "uie_sgvampire", "0" },
{ "uie_sginf", "0" },
{ "uie_sgknockback", "1" },
// Гранаты
{ "uie_glammocount", "-1" },
{ "uie_glweaponcount", "-1" },
{ "uie_gldelay", "500" },
{ "uie_glspeed", "1000" },
{ "uie_glbounce", "1" },
{ "uie_glgravity", "1" },
{ "uie_gltimeout", "2500" },
{ "uie_glsradius", "120" },
{ "uie_glsdamage", "100" },
{ "uie_gldamage", "100" },
{ "uie_glvampire", "0" },
{ "uie_glinf", "0" },
{ "uie_glbouncemodifier", "0.65" },
{ "uie_glknockback", "1" },
{ "uie_glhoming", "0" },
{ "uie_glguided", "0" },
// Ракеты
{ "uie_rlammocount", "-1" },
{ "uie_rlweaponcount", "-1" },
{ "uie_rldelay", "800" },
{ "uie_rlspeed", "900" },
{ "uie_rlbounce", "0" },
{ "uie_rlgravity", "0" },
{ "uie_rltimeout", "15000" },
{ "uie_rlsradius", "120" },
{ "uie_rlsdamage", "100" },
{ "uie_rldamage", "100" },
{ "uie_rlvampire", "0" },
{ "uie_rlinf", "0" },
{ "uie_rlbouncemodifier", "0.65" },
{ "uie_rlknockback", "1" },
{ "uie_rlhoming", "0" },
{ "uie_rlguided", "0" },
// Молния
{ "uie_lgammocount", "-1" },
{ "uie_lgweaponcount", "-1" },
{ "uie_lgrange", "768" },
{ "uie_lgdelay", "50" },
{ "uie_lgdamage", "8" },
{ "uie_lgvampire", "0" },
{ "uie_lgexplode", "0" },
{ "uie_lgsdamage", "70" },
{ "uie_lgsradius", "75" },
{ "uie_lginf", "0" },
{ "uie_lgknockback", "1" },
// Рэйлган
{ "uie_rgammocount", "-1" },
{ "uie_rgweaponcount", "-1" },
{ "uie_rgdelay", "1500" },
{ "uie_rgdamage", "100" },
{ "uie_rgvampire", "0" },
{ "uie_rginf", "0" },
{ "uie_rgknockback", "1" },
// Плазмаган
{ "uie_pgammocount", "-1" },
{ "uie_pgweaponcount", "-1" },
{ "uie_pgdelay", "100" },
{ "uie_pgspeed", "2000" },
{ "uie_pgbounce", "0" },
{ "uie_pggravity", "0" },
{ "uie_pgtimeout", "10000" },
{ "uie_pgsradius", "20" },
{ "uie_pgsdamage", "15" },
{ "uie_pgdamage", "20" },
{ "uie_pgvampire", "0" },
{ "uie_pginf", "0" },
{ "uie_pgbouncemodifier", "0.65" },
{ "uie_pgknockback", "1" },
{ "uie_pghoming", "0" },
{ "uie_pgguided", "0" },
// Бфг
{ "uie_bfgammocount", "-1" },
{ "uie_bfgweaponcount", "-1" },
{ "uie_bfgdelay", "200" },
{ "uie_bfgspeed", "2000" },
{ "uie_bfgbounce", "0" },
{ "uie_bfggravity", "0" },
{ "uie_bfgtimeout", "10000" },
{ "uie_bfgsradius", "120" },
{ "uie_bfgsdamage", "100" },
{ "uie_bfgdamage", "100" },
{ "uie_bfgvampire", "0" },
{ "uie_bfginf", "0" },
{ "uie_bfgbouncemodifier", "0.65" },
{ "uie_bfgknockback", "1" },
{ "uie_bfghoming", "0" },
{ "uie_bfgguided", "0" },
// Гвоздомёт
{ "uie_ngammocount", "-1" },
{ "uie_ngweaponcount", "-1" },
{ "uie_ngdelay", "1000" },
{ "uie_ngspeed", "555" },
{ "uie_ngbounce", "0" },
{ "uie_nggravity", "0" },
{ "uie_ngtimeout", "10000" },
{ "uie_ngcount", "15" },
{ "uie_ngspread", "500" },
{ "uie_ngdamage", "20" },
{ "uie_ngrandom", "1800" },
{ "uie_ngvampire", "0" },
{ "uie_nginf", "0" },
{ "uie_ngbouncemodifier", "0.65" },
{ "uie_ngknockback", "1" },
{ "uie_nghoming", "0" },
{ "uie_ngguided", "0" },
// Мины
{ "uie_plammocount", "-1" },
{ "uie_plweaponcount", "-1" },
{ "uie_pldelay", "800" },
{ "uie_plspeed", "700" },
{ "uie_plgravity", "1" },
{ "uie_pltimeout", "3000" },
{ "uie_plsradius", "150" },
{ "uie_plsdamage", "100" },
{ "uie_pldamage", "0" },
{ "uie_plvampire", "0" },
{ "uie_plinf", "0" },
{ "uie_plknockback", "1" },
// Пулемёт
{ "uie_cgammocount", "-1" },
{ "uie_cgweaponcount", "-1" },
{ "uie_cgdelay", "30" },
{ "uie_cgspread", "600" },
{ "uie_cgdamage", "7" },
{ "uie_cgvampire", "0" },
{ "uie_cginf", "0" },
{ "uie_cgknockback", "1" },
// Огнемёт
{ "uie_ftammocount", "-1" },
{ "uie_ftweaponcount", "-1" },
{ "uie_ftdelay", "40" },
{ "uie_ftspeed", "600" },
{ "uie_ftbounce", "1" },
{ "uie_ftgravity", "0" },
{ "uie_fttimeout", "1500" },
{ "uie_ftsradius", "20" },
{ "uie_ftsdamage", "15" },
{ "uie_ftdamage", "20" },
{ "uie_ftvampire", "0" },
{ "uie_ftinf", "0" },
{ "uie_ftbouncemodifier", "0.65" },
{ "uie_ftknockback", "1" },
{ "uie_fthoming", "0" },
{ "uie_ftguided", "0" },
// Антиматтер
{ "uie_amweaponcount", "-1" },
{ "uie_amdelay", "40" },
{ "uie_amspeed", "600" },
{ "uie_ambounce", "1" },
{ "uie_amgravity", "1" },
{ "uie_amtimeout", "1000" },
{ "uie_amsradius", "0" },
{ "uie_amsdamage", "0" },
{ "uie_amdamage", "6" },
{ "uie_amvampire", "0" },
{ "uie_aminf", "0" },
{ "uie_ambouncemodifier", "0.65" },
{ "uie_amknockback", "1" },
{ "uie_amhoming", "0" },
{ "uie_amguided", "0" },

	{ "uie_sandbox_fragtype", "0" },
	{ "uie_sandbox_timetype", "0" },
	{ "uie_sandbox_customfraglimits", "" },
	{ "uie_sandbox_customtimelimits", "" },
	{ "uie_sandbox_maplist", "" },
	{ "uie_sandbox_maplistexclude", "" },
	{ "uie_sandbox_MapRepeat", "1" },
	{ "uie_sandbox_MapSource", "0" },
	{ "uie_sandbox_RandomMapCount", "5" },
	{ "uie_sandbox_RandomMapType", "0" },
	{ "uie_sandbox_slottype", "" },
	{ "uie_sandbox_botname", "" },
	{ "uie_sandbox_botexclude", "" },
	{ "uie_sandbox_botskill", "" },
	{ "uie_sandbox_BotSelection", "1" },
	{ "uie_sandbox_BotCount", "0" },
	{ "uie_sandbox_BotChange", "0" },
	{ "uie_sandbox_OpenSlotCount", "99" },
	{ "uie_sandbox_BotSkillType", "0" },
	{ "uie_sandbox_BotSkillValue", "0" },
	{ "uie_sandbox_BotSkillBias", "0" },
	{ "uie_sandbox_PlayerJoinAs", "0" },
	{ "uie_sandbox_hostname", "QS Sandbox" },
	{ "uie_sandbox_ForceRespawn", "0" },
	{ "uie_sandbox_itemGroups", "" },
	{ "uie_sandbox_itemsHidden", "" },
	{ "uie_sandbox_WarmUp", "20" },
	{ "uie_sandbox_doWarmUp", "0" },
	{ "uie_sandbox_fraglimit", "0" },
	{ "uie_sandbox_timelimit", "0" },
	{ "uie_sandbox_weaponrespawn", "5" },
	{ "uie_sandbox_viewdistance", "0" },
	
	{ "uie_single_fragtype", "0" },
	{ "uie_single_timetype", "0" },
	{ "uie_single_customfraglimits", "" },
	{ "uie_single_customtimelimits", "" },
	{ "uie_single_maplist", "" },
	{ "uie_single_maplistexclude", "" },
	{ "uie_single_MapRepeat", "1" },
	{ "uie_single_MapSource", "0" },
	{ "uie_single_RandomMapCount", "5" },
	{ "uie_single_RandomMapType", "0" },
	{ "uie_single_slottype", "" },
	{ "uie_single_botname", "" },
	{ "uie_single_botexclude", "" },
	{ "uie_single_botskill", "" },
	{ "uie_single_BotSelection", "1" },
	{ "uie_single_BotCount", "0" },
	{ "uie_single_BotChange", "0" },
	{ "uie_single_OpenSlotCount", "99" },
	{ "uie_single_BotSkillType", "0" },
	{ "uie_single_BotSkillValue", "0" },
	{ "uie_single_BotSkillBias", "0" },
	{ "uie_single_PlayerJoinAs", "0" },
	{ "uie_single_hostname", "QS Single" },
	{ "uie_single_ForceRespawn", "0" },
	{ "uie_single_itemGroups", "" },
	{ "uie_single_itemsHidden", "" },
	{ "uie_single_WarmUp", "20" },
	{ "uie_single_doWarmUp", "0" },
	{ "uie_single_fraglimit", "0" },
	{ "uie_single_timelimit", "0" },
	{ "uie_single_weaponrespawn", "5" },
	{ "uie_single_viewdistance", "0" },

	{ "uie_ffa_fragtype", "0" },
	{ "uie_ffa_timetype", "0" },
	{ "uie_ffa_customfraglimits", "" },
	{ "uie_ffa_customtimelimits", "" },
	{ "uie_ffa_maplist", "" },
	{ "uie_ffa_maplistexclude", "" },
	{ "uie_ffa_MapRepeat", "1" },
	{ "uie_ffa_MapSource", "0" },
	{ "uie_ffa_RandomMapCount", "5" },
	{ "uie_ffa_RandomMapType", "0" },
	{ "uie_ffa_slottype", "" },
	{ "uie_ffa_botname", "" },
	{ "uie_ffa_botexclude", "" },
	{ "uie_ffa_botskill", "" },
	{ "uie_ffa_BotSelection", "1" },
	{ "uie_ffa_BotCount", "4" },
	{ "uie_ffa_BotChange", "0" },
	{ "uie_ffa_OpenSlotCount", "99" },
	{ "uie_ffa_BotSkillType", "0" },
	{ "uie_ffa_BotSkillValue", "0" },
	{ "uie_ffa_BotSkillBias", "0" },
	{ "uie_ffa_PlayerJoinAs", "0" },
	{ "uie_ffa_hostname", "QS Deathmatch" },
	{ "uie_ffa_ForceRespawn", "0" },
	{ "uie_ffa_itemGroups", "" },
	{ "uie_ffa_itemsHidden", "" },
	{ "uie_ffa_WarmUp", "20" },
	{ "uie_ffa_doWarmUp", "0" },
	{ "uie_ffa_fraglimit", "30" },
	{ "uie_ffa_timelimit", "20" },
	{ "uie_ffa_weaponrespawn", "5" },
	{ "uie_ffa_viewdistance", "0" },

	{ "uie_tourney_fragtype", "0" },
	{ "uie_tourney_timetype", "0" },
	{ "uie_tourney_customfraglimits", "" },
	{ "uie_tourney_customtimelimits", "" },
	{ "uie_tourney_maplist", "" },
	{ "uie_tourney_maplistexclude", "" },
	{ "uie_tourney_MapRepeat", "1" },
	{ "uie_tourney_MapSource", "0" },
	{ "uie_tourney_RandomMapCount", "5" },
	{ "uie_tourney_RandomMapType", "0" },
	{ "uie_tourney_slottype", "" },
	{ "uie_tourney_botname", "" },
	{ "uie_tourney_botexclude", "" },
	{ "uie_tourney_botskill", "" },
	{ "uie_tourney_BotSelection", "1" },
	{ "uie_tourney_BotCount", "4" },
	{ "uie_tourney_BotChange", "0" },
	{ "uie_tourney_OpenSlotCount", "99" },
	{ "uie_tourney_BotSkillType", "0" },
	{ "uie_tourney_BotSkillValue", "0" },
	{ "uie_tourney_BotSkillBias", "0" },
	{ "uie_tourney_PlayerJoinAs", "0" },
	{ "uie_tourney_hostname", "QS Tournament" },
	{ "uie_tourney_ForceRespawn", "0" },
	{ "uie_tourney_itemGroups", "" },
	{ "uie_tourney_itemsHidden", "" },
	{ "uie_tourney_WarmUp", "20" },
	{ "uie_tourney_doWarmUp", "1" },
	{ "uie_tourney_fraglimit", "0" },
	{ "uie_tourney_timelimit", "15" },
	{ "uie_tourney_weaponrespawn", "5" },
	{ "uie_tourney_viewdistance", "0" },

	{ "uie_team_fragtype", "0" },
	{ "uie_team_timetype", "0" },
	{ "uie_team_customfraglimits", "" },
	{ "uie_team_customtimelimits", "" },
	{ "uie_team_maplist", "" },
	{ "uie_team_maplistexclude", "" },
	{ "uie_team_MapRepeat", "1" },
	{ "uie_team_MapSource", "0" },
	{ "uie_team_RandomMapCount", "5" },
	{ "uie_team_RandomMapType", "0" },
	{ "uie_team_slottype", "" },
	{ "uie_team_botname", "" },
	{ "uie_team_botexclude", "" },
	{ "uie_team_botexclude", "" },
	{ "uie_team_botskill", "" },
	{ "uie_team_BotSelection", "1" },
	{ "uie_team_BotCount", "4" },
	{ "uie_team_BotChange", "0" },
	{ "uie_team_OpenSlotCount", "99" },
	{ "uie_team_BotSkillType", "0" },
	{ "uie_team_BotSkillValue", "0" },
	{ "uie_team_BotSkillBias", "0" },
	{ "uie_team_PlayerJoinAs", "0" },
	{ "uie_team_TeamSwapped", "0" },
	{ "uie_team_hostname", "QS Team Deathmatch" },
	{ "uie_team_ForceRespawn", "0" },
	{ "uie_team_AutoJoin", "0" },
	{ "uie_team_TeamBalance", "0" },
	{ "uie_team_itemGroups", "" },
	{ "uie_team_itemsHidden", "" },
	{ "uie_team_WarmUp", "20" },
	{ "uie_team_doWarmUp", "0" },
	{ "uie_team_friendly", "0" },
	{ "uie_team_fraglimit", "0" },
	{ "uie_team_timelimit", "20" },
	{ "uie_team_weaponrespawn", "30" },
	{ "uie_team_viewdistance", "0" },

	{ "uie_ctf_capturetype", "0" },
	{ "uie_ctf_timetype", "0" },
	{ "uie_ctf_customcapturelimits", "" },
	{ "uie_ctf_customtimelimits", "" },
	{ "uie_ctf_maplist", "" },
	{ "uie_ctf_maplistexclude", "" },
	{ "uie_ctf_MapRepeat", "1" },
	{ "uie_ctf_MapSource", "0" },
	{ "uie_ctf_RandomMapCount", "5" },
	{ "uie_ctf_RandomMapType", "0" },
	{ "uie_ctf_slottype", "" },
	{ "uie_ctf_botname", "" },
	{ "uie_ctf_botskill", "" },
	{ "uie_ctf_BotSelection", "1" },
	{ "uie_ctf_BotCount", "4" },
	{ "uie_ctf_BotChange", "0" },
	{ "uie_ctf_OpenSlotCount", "99" },
	{ "uie_ctf_BotSkillType", "0" },
	{ "uie_ctf_BotSkillValue", "0" },
	{ "uie_ctf_BotSkillBias", "0" },
	{ "uie_ctf_PlayerJoinAs", "0" },
	{ "uie_ctf_TeamSwapped", "0" },
	{ "uie_ctf_hostname", "QS CTF" },
	{ "uie_ctf_ForceRespawn", "0" },
	{ "uie_ctf_AutoJoin", "0" },
	{ "uie_ctf_TeamBalance", "0" },
	{ "uie_ctf_itemGroups", "" },
	{ "uie_ctf_itemsHidden", "" },
	{ "uie_ctf_WarmUp", "20" },
	{ "uie_ctf_doWarmUp", "0" },
	{ "uie_ctf_friendly", "0" },
	{ "uie_ctf_capturelimit", "0" },
	{ "uie_ctf_timelimit", "20" },
	{ "uie_ctf_weaponrespawn", "5" },
	{ "uie_ctf_viewdistance", "0" },
	
	{ "uie_oneflag_capturetype", "0" },
	{ "uie_oneflag_timetype", "0" },
	{ "uie_oneflag_customcapturelimits", "" },
	{ "uie_oneflag_customtimelimits", "" },
	{ "uie_oneflag_maplist", "" },
	{ "uie_oneflag_maplistexclude", "" },
	{ "uie_oneflag_MapRepeat", "1" },
	{ "uie_oneflag_MapSource", "0" },
	{ "uie_oneflag_RandomMapCount", "5" },
	{ "uie_oneflag_RandomMapType", "0" },
	{ "uie_oneflag_slottype", "" },
	{ "uie_oneflag_botname", "" },
	{ "uie_oneflag_botskill", "" },
	{ "uie_oneflag_BotSelection", "1" },
	{ "uie_oneflag_BotCount", "4" },
	{ "uie_oneflag_BotChange", "0" },
	{ "uie_oneflag_OpenSlotCount", "99" },
	{ "uie_oneflag_BotSkillType", "0" },
	{ "uie_oneflag_BotSkillValue", "0" },
	{ "uie_oneflag_BotSkillBias", "0" },
	{ "uie_oneflag_PlayerJoinAs", "0" },
	{ "uie_oneflag_TeamSwapped", "0" },
	{ "uie_oneflag_hostname", "QS Oneflag CTF" },
	{ "uie_oneflag_ForceRespawn", "0" },
	{ "uie_oneflag_AutoJoin", "0" },
	{ "uie_oneflag_TeamBalance", "0" },
	{ "uie_oneflag_itemGroups", "" },
	{ "uie_oneflag_itemsHidden", "" },
	{ "uie_oneflag_WarmUp", "20" },
	{ "uie_oneflag_doWarmUp", "0" },
	{ "uie_oneflag_friendly", "0" },
	{ "uie_oneflag_capturelimit", "0" },
	{ "uie_oneflag_timelimit", "20" },
	{ "uie_oneflag_weaponrespawn", "5" },
	{ "uie_oneflag_viewdistance", "0" },
	
	{ "uie_overload_capturetype", "0" },
	{ "uie_overload_timetype", "0" },
	{ "uie_overload_customcapturelimits", "" },
	{ "uie_overload_customtimelimits", "" },
	{ "uie_overload_maplist", "" },
	{ "uie_overload_maplistexclude", "" },
	{ "uie_overload_MapRepeat", "1" },
	{ "uie_overload_MapSource", "0" },
	{ "uie_overload_RandomMapCount", "5" },
	{ "uie_overload_RandomMapType", "0" },
	{ "uie_overload_slottype", "" },
	{ "uie_overload_botname", "" },
	{ "uie_overload_botskill", "" },
	{ "uie_overload_BotSelection", "1" },
	{ "uie_overload_BotCount", "4" },
	{ "uie_overload_BotChange", "0" },
	{ "uie_overload_OpenSlotCount", "99" },
	{ "uie_overload_BotSkillType", "0" },
	{ "uie_overload_BotSkillValue", "0" },
	{ "uie_overload_BotSkillBias", "0" },
	{ "uie_overload_PlayerJoinAs", "0" },
	{ "uie_overload_TeamSwapped", "0" },
	{ "uie_overload_hostname", "QS Overload" },
	{ "uie_overload_ForceRespawn", "0" },
	{ "uie_overload_AutoJoin", "0" },
	{ "uie_overload_TeamBalance", "0" },
	{ "uie_overload_itemGroups", "" },
	{ "uie_overload_itemsHidden", "" },
	{ "uie_overload_WarmUp", "20" },
	{ "uie_overload_doWarmUp", "0" },
	{ "uie_overload_friendly", "0" },
	{ "uie_overload_capturelimit", "0" },
	{ "uie_overload_timelimit", "20" },
	{ "uie_overload_weaponrespawn", "5" },
	{ "uie_overload_viewdistance", "0" },
	
	{ "uie_harvester_capturetype", "0" },
	{ "uie_harvester_timetype", "0" },
	{ "uie_harvester_customcapturelimits", "" },
	{ "uie_harvester_customtimelimits", "" },
	{ "uie_harvester_maplist", "" },
	{ "uie_harvester_maplistexclude", "" },
	{ "uie_harvester_MapRepeat", "1" },
	{ "uie_harvester_MapSource", "0" },
	{ "uie_harvester_RandomMapCount", "5" },
	{ "uie_harvester_RandomMapType", "0" },
	{ "uie_harvester_slottype", "" },
	{ "uie_harvester_botname", "" },
	{ "uie_harvester_botskill", "" },
	{ "uie_harvester_BotSelection", "1" },
	{ "uie_harvester_BotCount", "4" },
	{ "uie_harvester_BotChange", "0" },
	{ "uie_harvester_OpenSlotCount", "99" },
	{ "uie_harvester_BotSkillType", "0" },
	{ "uie_harvester_BotSkillValue", "0" },
	{ "uie_harvester_BotSkillBias", "0" },
	{ "uie_harvester_PlayerJoinAs", "0" },
	{ "uie_harvester_TeamSwapped", "0" },
	{ "uie_harvester_hostname", "QS Harvester" },
	{ "uie_harvester_ForceRespawn", "0" },
	{ "uie_harvester_AutoJoin", "0" },
	{ "uie_harvester_TeamBalance", "0" },
	{ "uie_harvester_itemGroups", "" },
	{ "uie_harvester_itemsHidden", "" },
	{ "uie_harvester_WarmUp", "20" },
	{ "uie_harvester_doWarmUp", "0" },
	{ "uie_harvester_friendly", "0" },
	{ "uie_harvester_capturelimit", "0" },
	{ "uie_harvester_timelimit", "20" },
	{ "uie_harvester_weaponrespawn", "5" },
	{ "uie_harvester_viewdistance", "0" },
	
	{ "uie_elim_capturetype", "0" },
	{ "uie_elim_timetype", "0" },
	{ "uie_elim_customcapturelimits", "" },
	{ "uie_elim_customtimelimits", "" },
	{ "uie_elim_maplist", "" },
	{ "uie_elim_maplistexclude", "" },
	{ "uie_elim_MapRepeat", "1" },
	{ "uie_elim_MapSource", "0" },
	{ "uie_elim_RandomMapCount", "5" },
	{ "uie_elim_RandomMapType", "0" },
	{ "uie_elim_slottype", "" },
	{ "uie_elim_botname", "" },
	{ "uie_elim_botskill", "" },
	{ "uie_elim_BotSelection", "1" },
	{ "uie_elim_BotCount", "4" },
	{ "uie_elim_BotChange", "0" },
	{ "uie_elim_OpenSlotCount", "99" },
	{ "uie_elim_BotSkillType", "0" },
	{ "uie_elim_BotSkillValue", "0" },
	{ "uie_elim_BotSkillBias", "0" },
	{ "uie_elim_PlayerJoinAs", "0" },
	{ "uie_elim_TeamSwapped", "0" },
	{ "uie_elim_hostname", "QS Elimination" },
	{ "uie_elim_ForceRespawn", "0" },
	{ "uie_elim_AutoJoin", "0" },
	{ "uie_elim_TeamBalance", "0" },
	{ "uie_elim_itemGroups", "" },
	{ "uie_elim_itemsHidden", "" },
	{ "uie_elim_WarmUp", "20" },
	{ "uie_elim_doWarmUp", "0" },
	{ "uie_elim_friendly", "0" },
	{ "uie_elim_capturelimit", "0" },
	{ "uie_elim_timelimit", "20" },
	{ "uie_elim_weaponrespawn", "5" },
	{ "uie_elim_viewdistance", "0" },
	
	{ "uie_ctfelim_capturetype", "0" },
	{ "uie_ctfelim_timetype", "0" },
	{ "uie_ctfelim_customcapturelimits", "" },
	{ "uie_ctfelim_customtimelimits", "" },
	{ "uie_ctfelim_maplist", "" },
	{ "uie_ctfelim_maplistexclude", "" },
	{ "uie_ctfelim_MapRepeat", "1" },
	{ "uie_ctfelim_MapSource", "0" },
	{ "uie_ctfelim_RandomMapCount", "5" },
	{ "uie_ctfelim_RandomMapType", "0" },
	{ "uie_ctfelim_slottype", "" },
	{ "uie_ctfelim_botname", "" },
	{ "uie_ctfelim_botskill", "" },
	{ "uie_ctfelim_BotSelection", "1" },
	{ "uie_ctfelim_BotCount", "4" },
	{ "uie_ctfelim_BotChange", "0" },
	{ "uie_ctfelim_OpenSlotCount", "99" },
	{ "uie_ctfelim_BotSkillType", "0" },
	{ "uie_ctfelim_BotSkillValue", "0" },
	{ "uie_ctfelim_BotSkillBias", "0" },
	{ "uie_ctfelim_PlayerJoinAs", "0" },
	{ "uie_ctfelim_TeamSwapped", "0" },
	{ "uie_ctfelim_hostname", "QS CTF Elimination" },
	{ "uie_ctfelim_ForceRespawn", "0" },
	{ "uie_ctfelim_AutoJoin", "0" },
	{ "uie_ctfelim_TeamBalance", "0" },
	{ "uie_ctfelim_itemGroups", "" },
	{ "uie_ctfelim_itemsHidden", "" },
	{ "uie_ctfelim_WarmUp", "20" },
	{ "uie_ctfelim_doWarmUp", "0" },
	{ "uie_ctfelim_friendly", "0" },
	{ "uie_ctfelim_capturelimit", "0" },
	{ "uie_ctfelim_timelimit", "20" },
	{ "uie_ctfelim_weaponrespawn", "5" },
	{ "uie_ctfelim_viewdistance", "0" },
	
	{ "uie_lms_fragtype", "0" },
	{ "uie_lms_timetype", "0" },
	{ "uie_lms_customcapturelimits", "" },
	{ "uie_lms_customtimelimits", "" },
	{ "uie_lms_maplist", "" },
	{ "uie_lms_maplistexclude", "" },
	{ "uie_lms_MapRepeat", "1" },
	{ "uie_lms_MapSource", "0" },
	{ "uie_lms_RandomMapCount", "5" },
	{ "uie_lms_RandomMapType", "0" },
	{ "uie_lms_slottype", "" },
	{ "uie_lms_botname", "" },
	{ "uie_lms_botskill", "" },
	{ "uie_lms_BotSelection", "1" },
	{ "uie_lms_BotCount", "4" },
	{ "uie_lms_BotChange", "0" },
	{ "uie_lms_OpenSlotCount", "99" },
	{ "uie_lms_BotSkillType", "0" },
	{ "uie_lms_BotSkillValue", "0" },
	{ "uie_lms_BotSkillBias", "0" },
	{ "uie_lms_PlayerJoinAs", "0" },
	{ "uie_lms_TeamSwapped", "0" },
	{ "uie_lms_hostname", "QS LMS" },
	{ "uie_lms_ForceRespawn", "0" },
	{ "uie_lms_AutoJoin", "0" },
	{ "uie_lms_TeamBalance", "0" },
	{ "uie_lms_itemGroups", "" },
	{ "uie_lms_itemsHidden", "" },
	{ "uie_lms_WarmUp", "20" },
	{ "uie_lms_doWarmUp", "0" },
	{ "uie_lms_friendly", "0" },
	{ "uie_lms_capturelimit", "0" },
	{ "uie_lms_timelimit", "20" },
	{ "uie_lms_weaponrespawn", "5" },
	{ "uie_lms_viewdistance", "0" },
	
	{ "uie_doubledom_capturetype", "0" },
	{ "uie_doubledom_timetype", "0" },
	{ "uie_doubledom_customcapturelimits", "" },
	{ "uie_doubledom_customtimelimits", "" },
	{ "uie_doubledom_maplist", "" },
	{ "uie_doubledom_maplistexclude", "" },
	{ "uie_doubledom_MapRepeat", "1" },
	{ "uie_doubledom_MapSource", "0" },
	{ "uie_doubledom_RandomMapCount", "5" },
	{ "uie_doubledom_RandomMapType", "0" },
	{ "uie_doubledom_slottype", "" },
	{ "uie_doubledom_botname", "" },
	{ "uie_doubledom_botskill", "" },
	{ "uie_doubledom_BotSelection", "1" },
	{ "uie_doubledom_BotCount", "4" },
	{ "uie_doubledom_BotChange", "0" },
	{ "uie_doubledom_OpenSlotCount", "99" },
	{ "uie_doubledom_BotSkillType", "0" },
	{ "uie_doubledom_BotSkillValue", "0" },
	{ "uie_doubledom_BotSkillBias", "0" },
	{ "uie_doubledom_PlayerJoinAs", "0" },
	{ "uie_doubledom_TeamSwapped", "0" },
	{ "uie_doubledom_hostname", "QS Double Domination" },
	{ "uie_doubledom_ForceRespawn", "0" },
	{ "uie_doubledom_AutoJoin", "0" },
	{ "uie_doubledom_TeamBalance", "0" },
	{ "uie_doubledom_itemGroups", "" },
	{ "uie_doubledom_itemsHidden", "" },
	{ "uie_doubledom_WarmUp", "20" },
	{ "uie_doubledom_doWarmUp", "0" },
	{ "uie_doubledom_friendly", "0" },
	{ "uie_doubledom_capturelimit", "0" },
	{ "uie_doubledom_timelimit", "20" },
	{ "uie_doubledom_weaponrespawn", "5" },
	{ "uie_doubledom_viewdistance", "0" },
	
	{ "uie_domination_capturetype", "0" },
	{ "uie_domination_timetype", "0" },
	{ "uie_domination_customcapturelimits", "" },
	{ "uie_domination_customtimelimits", "" },
	{ "uie_domination_maplist", "" },
	{ "uie_domination_maplistexclude", "" },
	{ "uie_domination_MapRepeat", "1" },
	{ "uie_domination_MapSource", "0" },
	{ "uie_domination_RandomMapCount", "5" },
	{ "uie_domination_RandomMapType", "0" },
	{ "uie_domination_slottype", "" },
	{ "uie_domination_botname", "" },
	{ "uie_domination_botskill", "" },
	{ "uie_domination_BotSelection", "1" },
	{ "uie_domination_BotCount", "4" },
	{ "uie_domination_BotChange", "0" },
	{ "uie_domination_OpenSlotCount", "99" },
	{ "uie_domination_BotSkillType", "0" },
	{ "uie_domination_BotSkillValue", "0" },
	{ "uie_domination_BotSkillBias", "0" },
	{ "uie_domination_PlayerJoinAs", "0" },
	{ "uie_domination_TeamSwapped", "0" },
	{ "uie_domination_hostname", "QS Domination" },
	{ "uie_domination_ForceRespawn", "0" },
	{ "uie_domination_AutoJoin", "0" },
	{ "uie_domination_TeamBalance", "0" },
	{ "uie_domination_itemGroups", "" },
	{ "uie_domination_itemsHidden", "" },
	{ "uie_domination_WarmUp", "20" },
	{ "uie_domination_doWarmUp", "0" },
	{ "uie_domination_friendly", "0" },
	{ "uie_domination_capturelimit", "0" },
	{ "uie_domination_timelimit", "20" },
	{ "uie_domination_weaponrespawn", "5" },
	{ "uie_domination_viewdistance", "0" },
};


static const int uie_cvarTableSize = sizeof(uie_cvarTable)/sizeof(uie_cvarTable[0]);


typedef struct {
	const char* from;
	const char* to;
	const char* init;
} uieCvarConversion_t;




// list of cvars that we need to convert to our "unified" data type
const uieCvarConversion_t uie_cvarImport[] = {
	{ "ui_ffa_fraglimit", "uie_ffa_fraglimit", "0" },
	{ "ui_ffa_timelimit", "uie_ffa_timelimit", "0" },

	{ "ui_tourney_fraglimit", "uie_tourney_fraglimit", "0" },
	{ "ui_tourney_timelimit", "uie_tourney_timelimit", "0" },

	{ "ui_team_friendly", "uie_team_friendly", "0" },
	{ "ui_team_fraglimit", "uie_team_fraglimit", "0" },
	{ "ui_team_timelimit", "uie_team_timelimit", "0" },

	{ "ui_ctf_friendly", "uie_ctf_friendly", "0" },
	{ "ui_ctf_capturelimit", "uie_ctf_capturelimit", "0" },
	{ "ui_ctf_timelimit", "uie_ctf_timelimit", "0" },

	{ "g_weaponrespawn", "uie_ffa_weaponrespawn", "5" },
	{ "g_weaponrespawn", "uie_tourney_weaponrespawn", "5" },
	{ "g_weaponTeamrespawn", "uie_team_weaponrespawn", "30" },
	{ "g_weaponrespawn", "uie_ctf_weaponrespawn", "5" }
};


const int uie_cvarImportSize = sizeof(uie_cvarImport)/sizeof(uie_cvarImport[0]);



#define MAX_CVAR_DATA (30*1024)
#define UIE_SKIRMISH_DATAFILE "SourceTechui.dat"

static qboolean skirmishCvarLoaded = qfalse;
static char skirmishCvarData[MAX_CVAR_DATA];






/*
=================
UIE_ConvertOldCvar
=================
*/
void UIE_ConvertOldCvar( char* cvar, int size)
{
	int i;

	for (i = 0; i < uie_cvarImportSize; i++) {
		if (!Q_stricmp(cvar, uie_cvarImport[i].from)) {
			Q_strncpyz(cvar, uie_cvarImport[i].to, size);

			if (uis.debug)
				Com_Printf("Cvar converted: %s\n", cvar);
			return;
		}
	}
}




/*
=================
UIE_SkirmishCvarExists
=================
*/
qboolean UIE_SkirmishCvarExists( char* base, const char* var_name)
{
	int i;
	char name[64];

	if (!var_name || !*var_name)
		return qfalse;

	if (base) {
		Q_strncpyz(name, va(base, var_name), 64);
	}
	else {
		Q_strncpyz(name, var_name, 64);
	}

	for (i = 0; i < uie_cvarTableSize; i++) {
		if (!Q_stricmp(uie_cvarTable[i].cvarName, name))
			return qtrue;
	}

	return qfalse;
}




/*
=================
UIE_GetSkirmishCvar
=================
*/
qboolean UIE_GetSkirmishCvar( char* base, const char* var_name, char* buffer, int buflen )
{
	char *p, *pnext;
	int len;
	char *str, *str2;
	char name[64];

	if (!var_name || !*var_name || !buffer)
		return qfalse;

	if (!skirmishCvarLoaded)
		UIE_StartServer_LoadSkirmishCvars();

	if (base) {
		Q_strncpyz(name, va(base, var_name), 64);
	}
	else {
		Q_strncpyz(name, var_name, 64);
	}

	len = strlen(name) + 1;
	p = skirmishCvarData;
	while (*p) {
		pnext = p + strlen(p) + 1;
		if (!Q_stricmp(name, COM_Parse(&p))) {
			break;
		}

		p = pnext;
	}

	buffer[0] = '\0';
	if (!*p) {
//		Com_Printf("Cvar not found: %s\n",name);
		return qfalse;
	}

	// return string inside quotes
	Q_strncpyz(buffer, COM_Parse(&p), buflen);

	return qtrue;
}




/*
=================
UIE_GetSkirmishCvarInt
=================
*/
int UIE_GetSkirmishCvarInt( char* base, const char* name )
{
	char buf[32];

	UIE_GetSkirmishCvar(base, name, buf, 32);
	return atoi(buf);
}




/*
=================
UIE_GetSkirmishCvarIntClamp
=================
*/
int UIE_GetSkirmishCvarIntClamp( int min, int max, char* base, const char* name )
{
	int value;
	char buf[32];

	UIE_GetSkirmishCvar(base, name, buf, 32);

	value = atoi(buf);
	if (value < min)
		value = min;
	if (value > max)
		value = max;

	return value;
}





/*
=================
UIE_SetSkirmishCvar
=================
*/
void UIE_SetSkirmishCvar( char* base, const char* var_name, const char* string )
{
	char *p;
	char *old;
	char *newstr;
	int len, next, lenmove, oldlen, addlen;
	char name[64];

	if (!var_name || !*var_name || !string)
		return;

	if (base) {
		Q_strncpyz(name, va(base, var_name), 64);
	}
	else {
		Q_strncpyz(name, var_name, 64);
	}

	len = strlen(name)+1;
	p = skirmishCvarData;
	old = NULL;

	// do search
	while (*p) {
		next = strlen(p)+1;
		// prevent premature termination on longer name string
		if (!old && (next > len) && p[len - 1] == ' ' && Q_stricmpn(p, name, len - 1) == 0) {
			old = p;
		}

		p += next;
	}

	// delete old entry
	if (old) {
		oldlen = strlen(old) + 1;
		lenmove = p -(old + oldlen) + 1;
		memmove(old, old + oldlen, lenmove);

		p -= oldlen;
	}

	// check for overflow - bad news
	newstr = va("%s \"%s\"", name, string);
	addlen = strlen(newstr) + 1;
	if (p - skirmishCvarData + addlen >= MAX_CVAR_DATA) {
		Com_Printf("Skirmish Cvar overflow, cvar %s lost\n", name);
		return;
	}

	// add to end, keeping \0\0 integrity
	Q_strncpyz(p, newstr, addlen);
	p[addlen] = '\0';

//	Com_Printf("Cvar wrote: %s\n", newstr);
}




/*
=================
UIE_SetSkirmishCvarInt
=================
*/
void UIE_SetSkirmishCvarInt( char* base, const char* name, int value)
{
	char buf[32];

	Q_strncpyz(buf, va("%i", value), 32);
	UIE_SetSkirmishCvar(base, name, buf);
}



/*
=================
UIE_StartServer_MergeSkirmishCvars

Convert from text format to internal NUL buffer terminated

May have been edited by a program that changes line terminator formats
so we attempt to recover from this
=================
*/
static void UIE_StartServer_MergeSkirmishCvars( char* cvarList )
{
	char *p;
	char varname[MAX_TOKEN_CHARS];
	char* data;

	p = cvarList;

	while (*p) {
		Q_strncpyz( &varname[0], COM_Parse(&p), MAX_TOKEN_CHARS);
		if (!varname[0])
			break;

		data = COM_Parse(&p);

		UIE_SetSkirmishCvar(NULL, varname, data);

	};
}



/*
=================
UIE_StartServer_SaveSkirmishCvars
=================
*/
void UIE_StartServer_SaveSkirmishCvars( void )
{
	char *p;
	fileHandle_t file;
	int len;

	p = skirmishCvarData;
	while (*p) {
		p+= strlen(p);
		*p++ = '\r';
	}

	len = strlen(skirmishCvarData);
	trap_FS_FOpenFile(UIE_SKIRMISH_DATAFILE, &file, FS_WRITE);
	trap_FS_Write(skirmishCvarData, len, file);
	trap_FS_FCloseFile(file);

	p = skirmishCvarData;
	while (*p) {
		if (*p == '\r')
			*p = '\0';
		p++;
	}

//	Com_Printf("Wrote %s, %i bytes\n", UIE_SKIRMISH_DATAFILE, len);
}



/*
=================
UIE_StartServer_LoadSkirmishCvars
=================
*/
void UIE_StartServer_LoadSkirmishCvars( void )
{
	int i;
	vmCvar_t cvar;
	fileHandle_t file;
	int len;
	char newCvars[MAX_CVAR_DATA];

	memset(skirmishCvarData, 1, MAX_CVAR_DATA);
	skirmishCvarLoaded = qfalse;
	skirmishCvarData[0] = '\0';

	// load from cvars in memory or default values on list.
	// The flags marked here don't override the existing ARCHIVE flag,
	// which is set when the cvar is loaded from q3config.cfg
	for (i = 0; i < uie_cvarTableSize; i++) {
		trap_Cvar_Register(&cvar, uie_cvarTable[i].cvarName, uie_cvarTable[i].defaultString, CVAR_TEMP|CVAR_USER_CREATED);
		UIE_SetSkirmishCvar(NULL, uie_cvarTable[i].cvarName, cvar.string);
	}

	// now override cvars that we need to import
	for (i = 0; i < uie_cvarImportSize; i++) {
		trap_Cvar_Register(&cvar, uie_cvarImport[i].from, uie_cvarImport[i].init, CVAR_TEMP|CVAR_USER_CREATED);
		UIE_SetSkirmishCvar(NULL, uie_cvarImport[i].to, cvar.string);
	}

	skirmishCvarLoaded = qtrue;

	// load cvars from file, and merge them with this prepared list
	len = trap_FS_FOpenFile(UIE_SKIRMISH_DATAFILE, &file, FS_READ);

	if (!(len < MAX_CVAR_DATA - 1)) {
		Com_Printf(UIE_SKIRMISH_DATAFILE" is too large, skirmish reset to default.\n");
		trap_FS_FCloseFile(file);
		return;
	}
	else if (len <= 0) {
		Com_Printf(UIE_SKIRMISH_DATAFILE" doesn't exist, imported default cvars.\n");
		trap_FS_FCloseFile(file);
		return;
	}

	trap_FS_Read(newCvars, len, file);
	trap_FS_FCloseFile(file);

	newCvars[len] = '\0';
	skirmishCvarLoaded = qtrue;

	UIE_StartServer_MergeSkirmishCvars(newCvars);
	UIE_StartServer_SaveSkirmishCvars();
}





/*
==========================================================================

MISC FUNCTIONS

==========================================================================
*/



/*
=================
UIE_StartServer_RegisterDisableCvars
=================
*/
void UIE_StartServer_RegisterDisableCvars(qboolean init)
{
	int i;
	char* disable;

	for (i = 0; i < ITEM_COUNT; i++)
	{
		disable = va("disable_%s", server_itemlist[i].mapitem);
		if (init)
			trap_Cvar_Register(NULL, disable, "0", 0);
		else
			trap_Cvar_Set(disable, "0");
	}
}



/*
=================
StartServer_CanFight
=================
*/
qboolean StartServer_CanFight(void)
{
   int bot_count = 0;

   // number of maps
	if (StartServer_IsRandomGeneratedMapList(s_scriptdata.map.listSource)) {
		if (s_scriptdata.map.SourceCount == 0)
			return qfalse;
	}
	else {
		if (s_scriptdata.map.num_maps == 0)
			return qfalse;
	}

   // number of bots
   if (s_scriptdata.bot.typeSelect == BOTTYPE_SELECTARENASCRIPT) {
      if (StartServer_ValidBotCount() == 0) {
         return qfalse;
      }
   }

	return qtrue;
}



/*
=================
UI_SaveMultiArray
=================
*/
void UI_SaveMultiArray(char* base, const char* key, String_Callback callback, int count, int size, char newnull)
{
	char buf[MAX_STRING_CHARS];
	int i, last;
	char* arraychar;

	if (size * count >= MAX_STRING_CHARS)
	{
		trap_Error( "size*step >= MAX_STRING_CHARS" );
		return;
	}

	last = 0;
	for (i = 0; i < count; i++)
	{
		arraychar = callback(i);
		Q_strncpyz(&buf[last], arraychar, size);
		last += strlen(arraychar);
		buf[last++] = newnull;
	}

	buf[last] = '\0';
	UIE_SetSkirmishCvar( base, key, buf );
}



/*
=================
UI_LoadMultiArray
=================
*/
void UI_LoadMultiArray(char* base, const char* key, String_Callback callback, int count, int size, char newnull)
{
	char buf[MAX_STRING_CHARS];
	char* arraychar;
	int len, i, c;

	UIE_GetSkirmishCvar( base, key, buf, MAX_STRING_CHARS);

	len = strlen(buf);
	for (i = 0; i < len; i++)
		if (buf[i] == newnull)
			buf[i] = '\0';

	i = 0;
	c = 0;
	while (i < len && c < count)
	{
		arraychar = callback(c);
		Q_strncpyz(arraychar, &buf[i], size);
		i += strlen( &buf[i] ) + 1;

		c++;
	}

	// clear remaining elements
	while (c < count)
	{
		arraychar = callback(c);
		memset(arraychar, 0, size);
		c++;
	}
}



/*
==========================================================================

LOADING AND SAVING OF MAP SCRIPT DATA

==========================================================================
*/



// additional map type strings are set in ui_startserver_custommaps.c
const char* randommaptype_items[MAP_RND_MAX + MAX_MAPTYPES + 1] = {
	"(Any)", // MAP_RND_ANY
	"(QS only)", // MAP_RND_ID
	"(Non QS only)", // MAP_RND_NONID
	"(Bias QS)", // MAP_RND_BIASID
	"(Bias Non QS)", // MAP_RND_BIASNONID
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0
};
const char* randommaptype_itemsru[MAP_RND_MAX + MAX_MAPTYPES + 1] = {
	"(Любые)", // MAP_RND_ANY
	"(QS только)", // MAP_RND_ID
	"(Не QS только)", // MAP_RND_NONID
	"(Разброс QS)", // MAP_RND_BIASID
	"(Разброс не QS)", // MAP_RND_BIASNONID
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,
	0
};





/*
=================
StartServer_IsIdMap
=================
*/
qboolean StartServer_IsIdMap(const char* mapname)
{
	const char* const *ptr;

	// check list of idmaps
	ptr = idmap_list;
	while (*ptr) {
		if (!Q_stricmp(*ptr, mapname))
		{
			return qtrue;
		}
		ptr++;
	}

	return qfalse;
}





/*
=================
StartServer_IsRandomGeneratedMapList

Map generation will be from a list of randomly selected map names,
not a user list of map name
=================
*/
qboolean StartServer_IsRandomGeneratedMapList( int type )
{
	if (type == MAP_MS_RANDOM || type == MAP_MS_RANDOMEXCLUDE)
		return qtrue;
	return qfalse;
}





/*
=================
StartServer_MapPageCount
=================
*/
int StartServer_MapPageCount(void)
{
	int count;

	count = 1 + ( s_scriptdata.map.num_maps / NUMMAPS_PERPAGE );
	if (count > MAX_MAPPAGES)
		count = MAX_MAPPAGES;
	return count;
}



/*
=================
StartServer_RangeClipMapIndex
=================
*/
int StartServer_RangeClipMapIndex(int index)
{
	if (index < 0)
		return 0;

	if (index > s_scriptdata.map.num_maps )
		index = s_scriptdata.map.num_maps;

	if (index == MAX_NUMMAPS)
		return MAX_NUMMAPS - 1;

	return index;
}






/*
=================
SSMP_ShortName_Callback
=================
*/
static char* SSMP_ShortName_Callback(int index)
{
	return s_scriptdata.map.data[index].shortName;
}



/*
=================
SSMP_FragLimit_Callback
=================
*/
static char* SSMP_FragLimit_Callback(int index)
{
	return s_scriptdata.map.data[index].fragLimit;
}


/*
=================
SSMP_TimeLimit_Callback
=================
*/
static char* SSMP_TimeLimit_Callback(int index)
{
	return s_scriptdata.map.data[index].timeLimit;
}




/*
=================
StartServer_MapPage_LoadMapList

must be called after s_scriptdata.map.type has been set
=================
*/
void StartServer_LoadMapList(void)
{
	char* s;
	char* ml;

	ml = "maplist";
	if (s_scriptdata.map.listSource == MAP_MS_RANDOMEXCLUDE)
		ml = "maplistexclude";

	s = gametype_cvar_base[s_scriptdata.gametype];
	UI_LoadMultiArray(s, ml, SSMP_ShortName_Callback, MAX_NUMMAPS, SHORTMAP_BUFFER, ';');
}




/*
=================
StartServer_SwapMaps
=================
*/
qboolean StartServer_SwapMaps(int from, int to)
{
	static mapdata_t tmp;

	if (from < 0 || to < 0 || from == to)
		return qfalse;

	if (from >= s_scriptdata.map.num_maps || to >= s_scriptdata.map.num_maps)
		return qfalse;

	memcpy(&tmp, &s_scriptdata.map.data[from], sizeof (mapdata_t));
	memcpy(&s_scriptdata.map.data[from], &s_scriptdata.map.data[to], sizeof (mapdata_t));
	memcpy(&s_scriptdata.map.data[to], &tmp, sizeof (mapdata_t));

	return qtrue;
}





/*
=================
StartServer_StoreMap

Places map data into the array, overwriting a previous entry
=================
*/
void StartServer_StoreMap(int pos, int arena)
{
	const char* info;
	char* shortname;
	char* longname;
	int len;
	int base;

	pos = StartServer_RangeClipMapIndex(pos);
	info = UI_GetArenaInfoByNumber(arena);

	shortname = Info_ValueForKey(info, "map");
	len = strlen(shortname) + 1;
	Q_strncpyz(s_scriptdata.map.data[pos].shortName, shortname, len);

	longname = Info_ValueForKey(info, "longname");
	if (!longname || !*longname)
		longname = shortname;

	len = strlen(longname) + 1;
	Q_strncpyz(s_scriptdata.map.data[pos].longName, longname, len);

	// increase map count if we put map into a previously empty slot
	// set frag/time limits too
	if (pos == s_scriptdata.map.num_maps) {
		s_scriptdata.map.num_maps++;
		Q_strncpyz(s_scriptdata.map.data[pos].fragLimit, va("%i", s_scriptdata.map.fragLimit), MAX_LIMIT_BUF);
		Q_strncpyz(s_scriptdata.map.data[pos].timeLimit, va("%i", s_scriptdata.map.timeLimit), MAX_LIMIT_BUF);
	}
}



/*
=================
StartServer_InsertMap

Creates an empty slot for map data to be added
=================
*/
void StartServer_InsertMap(int pos, int arena)
{
	int last;
	int i;

	// drop any maps that try to overflow
	if (pos > MAX_NUMMAPS - 1)
		return;

	pos = StartServer_RangeClipMapIndex(pos);

	// quietly drop last map on list if we are inserting earlier
	last = s_scriptdata.map.num_maps - 1;
	if (last == MAX_NUMMAPS - 1 )
		last--;
	else
		s_scriptdata.map.num_maps++;

	// move maps up one slot
	for (i = last; i >= pos; i--)
		memcpy(&s_scriptdata.map.data[i+1], &s_scriptdata.map.data[i], sizeof(mapdata_t));

	StartServer_StoreMap(pos, arena);
	Q_strncpyz(s_scriptdata.map.data[pos].fragLimit, va("%i", s_scriptdata.map.fragLimit), MAX_LIMIT_BUF);
	Q_strncpyz(s_scriptdata.map.data[pos].timeLimit, va("%i", s_scriptdata.map.timeLimit), MAX_LIMIT_BUF);
}



/*
=================
StartServer_DeleteMap
=================
*/
void StartServer_DeleteMap(int index)
{
	int lines;
	int i;

	if (index < 0 || index >= MAX_NUMMAPS)
		return;

	lines = (MAX_NUMMAPS - index - 1);
	if (lines) {
		for (i =0 ; i < lines; i++)
			memcpy(&s_scriptdata.map.data[index + i], &s_scriptdata.map.data[index + i + 1], sizeof(mapdata_t));
	}

	// zero final element only
	memset(&s_scriptdata.map.data[MAX_NUMMAPS - 1], 0, sizeof(mapdata_t));

	if (index < s_scriptdata.map.num_maps)
		s_scriptdata.map.num_maps--;
}






/*
=================
StartServer_RefreshMapNames
=================
*/
void StartServer_RefreshMapNames(void)
{
	int i, j, count;
	const char	*info;
	char* arena_mapname;

	i = 0;
	count = UI_GetNumArenas();
	while (i < MAX_NUMMAPS && s_scriptdata.map.data[i].shortName[0]) {
		for( j = 0; j < count; j++ ) {
			info = UI_GetArenaInfoByNumber( j );

			if (!info)
				continue;

			arena_mapname = Info_ValueForKey( info, "map");
			if (!arena_mapname || arena_mapname[0] == '\0') {
				continue;
			}

			if (!Q_stricmp(s_scriptdata.map.data[i].shortName, arena_mapname))
			{
				Q_strncpyz( s_scriptdata.map.data[i].longName, Info_ValueForKey( info, "longname"), LONGMAP_BUFFER);
				break;
			}
		}
		// map not found, quietly delete it from list
		if (j == count) {
			StartServer_DeleteMap(i);
			continue;
		}

		i++;
	}

	s_scriptdata.map.num_maps = i;
}




/*
=================
StartServer_GetArenaFragLimit
=================
*/
static const char* StartServer_GetArenaFragLimit( int map )
{
	static char fraglimit[16];
	const char* info;
	const char* infofrag;

	info = UI_GetArenaInfoByMap(s_scriptdata.map.data[map].shortName);
	infofrag = Info_ValueForKey(info, "fraglimit");

	if (infofrag[0]) {
		Q_strncpyz(fraglimit, infofrag, 16);
	}
	else {
		Q_strncpyz(fraglimit, va("%i", s_scriptdata.map.fragLimit), 16);
	}
		
	return fraglimit;
}




/*
=================
StartServer_MapDoAction
=================
*/
void StartServer_MapDoAction( int src, int dest, int page, int selected )
{
	int i;
	int pageindex;
	const char* fragsrc;
	const char* timesrc;

	pageindex = page * NUMMAPS_PERPAGE;

	//
	// actions using src
	//

	if (src == MAP_CF_CLEARALL)
	{
		while (s_scriptdata.map.num_maps)
		{
			StartServer_DeleteMap(0);
		}
		return;
	}

	if (src == MAP_CF_CLEARPAGE)
	{
		for (i = 0; i < NUMMAPS_PERPAGE; i++)
			StartServer_DeleteMap(pageindex);
		return;
	}

	//
	// actions that combine src and dest
	//

	if (src == MAP_CF_ARENASCRIPT) {
		switch (dest) {
			case MAP_CT_SELECTED:
				if (selected >= 0) {
					fragsrc = StartServer_GetArenaFragLimit(pageindex + selected);
					Q_strncpyz(s_scriptdata.map.data[pageindex + selected].fragLimit, fragsrc, MAX_LIMIT_BUF);
				}
				break;

			case MAP_CT_PAGE:
				for (i = 0; i < NUMMAPS_PERPAGE; i++) {
					fragsrc = StartServer_GetArenaFragLimit(pageindex + i);
					Q_strncpyz(s_scriptdata.map.data[pageindex + i].fragLimit, fragsrc, MAX_LIMIT_BUF);
				}
				break;

			case MAP_CT_ALL:
				for (i = 0; i < s_scriptdata.map.num_maps; i++) {
					fragsrc = StartServer_GetArenaFragLimit(i);
					Q_strncpyz(s_scriptdata.map.data[i].fragLimit, fragsrc, MAX_LIMIT_BUF);
				}

				break;
		}
		return;
	}

	// there's some overlap between the copying of time and frag limits
	// "flow through" from the time to the frag code is expected
	// ANY OTHER CUSTOM VALUES SHOULD APPEAR BEFORE THIS CODE

	// copy time
	timesrc = va("%i", s_scriptdata.map.timeLimit);
	if (src != MAP_CF_FRAG) {
		switch (dest) {
			case MAP_CT_SELECTED:
				if (selected >= 0)
					Q_strncpyz(s_scriptdata.map.data[pageindex + selected].timeLimit, timesrc, MAX_LIMIT_BUF);
				break;
			case MAP_CT_PAGE:
				for (i = 0; i < NUMMAPS_PERPAGE; i++)
					Q_strncpyz(s_scriptdata.map.data[pageindex + i].timeLimit, timesrc, MAX_LIMIT_BUF);
				break;
			case MAP_CT_ALL:
				for (i = 0; i < s_scriptdata.map.num_maps; i++)
					Q_strncpyz(s_scriptdata.map.data[i].timeLimit, timesrc, MAX_LIMIT_BUF);

				break;
		}
	}


	// copy frags/caps
	fragsrc = va("%i", s_scriptdata.map.fragLimit);
	if (src != MAP_CF_TIME) {
		switch (dest) {
			case MAP_CT_SELECTED:
				if (selected >= 0)
					Q_strncpyz(s_scriptdata.map.data[pageindex + selected].fragLimit, fragsrc, MAX_LIMIT_BUF);
				break;
			case MAP_CT_PAGE:
				for (i = 0; i < NUMMAPS_PERPAGE; i++)
					Q_strncpyz(s_scriptdata.map.data[pageindex + i].fragLimit, fragsrc, MAX_LIMIT_BUF);
				break;
			case MAP_CT_ALL:
				for (i = 0; i < s_scriptdata.map.num_maps; i++)
					Q_strncpyz(s_scriptdata.map.data[i].fragLimit, fragsrc, MAX_LIMIT_BUF);
				break;
		}
	}
}





/*
=================
StartServer_BuildMapDistribution
=================
*/
static void StartServer_BuildMapDistribution(void)
{
	int i, j, count, maptype, gametype;
	const char	*info;
	char* arena_mapname;
	int matchbits, gamebits;

	// set zero
	for (i = 0; i < MAX_GAME_TYPE; i++)
		for (j = 0; j < MAX_MAP_GROUP; j++)
			s_scriptdata.map.TypeCount[i][j] = 0;

	count = UI_GetNumArenas();
	for( i = 0; i < count; i++ ) {
		info = UI_GetArenaInfoByNumber( i );

		if (!info)
			continue;

		arena_mapname = Info_ValueForKey( info, "map");
		if (!arena_mapname || arena_mapname[0] == '\0') {
			continue;
		}

		if (StartServer_IsIdMap(arena_mapname))
			maptype = MAP_GROUP_ID;
		else
			maptype = MAP_GROUP_NONID;

		gamebits = GametypeBits( Info_ValueForKey( info, "type") );
		for (j = 0; j < NUM_GAMETYPES; j++)
		{
			gametype = j;
			matchbits = 1 << gametype;

			if (matchbits & gamebits)
				s_scriptdata.map.TypeCount[ gametype ][maptype]++;
		}
	}
}


/*
=================
StartServer_SaveMapList
=================
*/
void StartServer_SaveMapList(void)
{
	char* s;
	char* s1;

	s1 = "maplist";
	if (s_scriptdata.map.listSource == MAP_MS_RANDOMEXCLUDE)
		s1 = "maplistexclude";

	s = gametype_cvar_base[s_scriptdata.gametype];
	UI_SaveMultiArray(s, s1, SSMP_ShortName_Callback, MAX_NUMMAPS, SHORTMAP_BUFFER, ';');
}








/*
=================
StartServer_LoadMapScriptData

Loads map specific gametype data
=================
*/
static void StartServer_LoadMapScriptData(void)
{
	char buf[64];
	int i, index, num, max;
	char* s;
	char *f, *f2;

	index = 0;
	f = "customfraglimits";
	f2 = "fragtype";

	s = gametype_cvar_base[s_scriptdata.gametype];

	if (s_scriptdata.gametype >= GT_CTF && !(s_scriptdata.gametype == GT_LMS)) {
		s_scriptdata.map.fragLimit = UIE_GetSkirmishCvarIntClamp(0, 999, s, "capturelimit");
		f = "customcapturelimits";
		f2 = "capturetype";
	}
	else {
		s_scriptdata.map.fragLimit = UIE_GetSkirmishCvarIntClamp(0, 99, s, "fraglimit");
	}

	s_scriptdata.map.timeLimit = UIE_GetSkirmishCvarIntClamp(0, 999, s, "timelimit");

	// load custom frag/time values
	UI_LoadMultiArray(s, f, SSMP_FragLimit_Callback, MAX_NUMMAPS, MAX_LIMIT_BUF, ';');
	UI_LoadMultiArray(s, "customtimelimits", SSMP_TimeLimit_Callback, MAX_NUMMAPS, MAX_LIMIT_BUF, ';');

	// load type of frag/time value used to start game (none, default, custom)
	s_scriptdata.map.fragLimitType = UIE_GetSkirmishCvarIntClamp(0, 2, s, f2 );
	s_scriptdata.map.timeLimitType = UIE_GetSkirmishCvarIntClamp(0, 2, s, "timetype" );

	// load map source and repeat info
	s_scriptdata.map.Repeat = UIE_GetSkirmishCvarIntClamp(0, 1, s, "MapRepeat");
	s_scriptdata.map.listSource = UIE_GetSkirmishCvarIntClamp(0, MAP_MS_MAX - 1, s, "MapSource");

	// load maps
	// must be after s_scriptdata.map.type is set
	StartServer_LoadMapList();

	s_scriptdata.map.Repeat = UIE_GetSkirmishCvarIntClamp(0, 1, s, "MapRepeat");
	s_scriptdata.map.SourceCount = UIE_GetSkirmishCvarIntClamp(2, 99, s, "RandomMapCount");

	UIE_GetSkirmishCvar( s, "RandomMapType", buf, 64);
	s_scriptdata.map.SourceType = (int)Com_Clamp(0, MAP_RND_MAX - 1, atoi(buf));	// non-numerical values give zero

	max = StartServer_NumCustomMapTypes();
	for (i = 0; i < max; i++)
	{
		if (!Q_stricmp(buf, randommaptype_items[MAP_RND_MAX + i]))
		{
			s_scriptdata.map.SourceType = MAP_RND_MAX + i;
			break;
		}
	}

	// validate each of the map names
	StartServer_RefreshMapNames();
}



/*
=================
StartServer_SaveMapScriptData

Saves map specific gametype data
=================
*/
static void StartServer_SaveMapScriptData(void)
{
	int type;
	char* s;
	char *f, *f2;

	s = gametype_cvar_base[s_scriptdata.gametype];
	f = "customfraglimits";
	f2 = "fragtype";

	UIE_SetSkirmishCvarInt( s, "timelimit", s_scriptdata.map.timeLimit);
	if (s_scriptdata.gametype >= GT_CTF && !(s_scriptdata.gametype == GT_LMS)) {
		UIE_SetSkirmishCvarInt( s, "capturelimit", s_scriptdata.map.fragLimit);
		f = "customcapturelimits";
		f2 = "capturetype";
	}
	else {
		UIE_SetSkirmishCvarInt( s, "fraglimit", s_scriptdata.map.fragLimit);
	}

	StartServer_SaveMapList();

	UIE_SetSkirmishCvarInt( s, "MapSource", s_scriptdata.map.listSource);

	// save custom frag/time values
	UI_SaveMultiArray(s, f, SSMP_FragLimit_Callback, MAX_NUMMAPS, MAX_LIMIT_BUF, ';');
	UI_SaveMultiArray(s, "customtimelimits", SSMP_TimeLimit_Callback, MAX_NUMMAPS, MAX_LIMIT_BUF, ';');

	// save type of frag/time value used to start game (none, default, custom)
	UIE_SetSkirmishCvarInt( s,f2, s_scriptdata.map.fragLimitType);
	UIE_SetSkirmishCvarInt( s,"timetype", s_scriptdata.map.timeLimitType);

	// save map source and repeat info
	UIE_SetSkirmishCvarInt( s, "MapRepeat", s_scriptdata.map.Repeat);
	UIE_SetSkirmishCvarInt( s, "RandomMapCount", s_scriptdata.map.SourceCount);

	type = s_scriptdata.map.SourceType;
	if (type < MAP_RND_MAX)
		UIE_SetSkirmishCvarInt( s, "RandomMapType", type);
	else
		UIE_SetSkirmishCvar( s, "RandomMapType", randommaptype_items[type]);
}















/*
==========================================================================

LOADING AND SAVING OF BOT SCRIPT DATA

==========================================================================
*/

#define BOT_TMPBUFFER 4

static char botskill_tmpbuffer[PLAYER_SLOTS][BOT_TMPBUFFER];	// tmp used to load/save bot skill values




/*
=================
StartServer_IsBotArenaScript
=================
*/
qboolean StartServer_IsBotArenaScript(int type)
{
	if (type == BOTTYPE_ARENASCRIPT || type == BOTTYPE_RANDOMARENASCRIPT ||
         type == BOTTYPE_SELECTARENASCRIPT)
		return qtrue;

	return qfalse;
}


/*
=================
StartServer_IsRandomBotExclude
=================
*/
qboolean StartServer_IsRandomBotExclude(int type)
{
	if (type == BOTTYPE_RANDOMEXCLUDE || type == BOTTYPE_RANDOMARENASCRIPT)
		return qtrue;

	return qfalse;
}




/*
=================
StartServer_IsRandomBotList
=================
*/
qboolean StartServer_IsRandomBotList( int type )
{
	if (type == BOTTYPE_RANDOM || type == BOTTYPE_RANDOMEXCLUDE || type == BOTTYPE_RANDOMARENASCRIPT)
		return qtrue;
	return qfalse;
}







/*
=================
StartServer_SetBotSkillValue
=================
*/
static void StartServer_SetBotSkillValue(botskill_t* b, int value)
{
	value = (int)Com_Clamp(0 , 999, value);
	b->value = (int)Com_Clamp(0, 4, (value / 100) % 10);
	b->low = (int)Com_Clamp(0, 4, (value / 10) % 10);
	b->high = (int)Com_Clamp(0, 4, value % 10);
}



/*
=================
StartServer_SetBotSkillRangeType

Assumes s_scriptdata.bot.typeSelect is already initialized
=================
*/
void StartServer_SetBotSkillRangeType(int skill)
{
	int i;
	qboolean qbool;

	// wrap skill early if selecting from random
	if ((StartServer_IsRandomBotList(s_scriptdata.bot.typeSelect) || StartServer_IsBotArenaScript(s_scriptdata.bot.typeSelect)) 
		&& skill >= BOTSKILL_CUSTOMSINGLE)
	{
		skill = BOTSKILL_SAME;
	}

	s_scriptdata.bot.skillType = skill;
	if (skill == BOTSKILL_SAME || skill == BOTSKILL_CUSTOMSINGLE) {
		qbool = qfalse;
	}
	else {
		qbool = qtrue;
	}

	s_scriptdata.bot.globalSkill.range = qbool;
	for (i = 0; i < PLAYER_SLOTS; i++)
		s_scriptdata.bot.skill[i].range = qbool;
}


/*
=================
SSBP_BotName_Callback
=================
*/
static char* SSBP_BotName_Callback(int index)
{
	return s_scriptdata.bot.name[index];
}



/*
=================
SSBP_BotBuffer_Callback
=================
*/
static char* SSBP_BotBuffer_Callback(int index)
{
	return botskill_tmpbuffer[index];
}




/*
=================
StartServer_ValidBotCount
=================
*/
int StartServer_ValidBotCount( void )
{
   int count = 0;
   int i;

   for (i = 0; i < PLAYER_SLOTS; i++) {
      if (s_scriptdata.bot.slotType[i] != SLOTTYPE_BOT) {
         continue;
      }
      if (s_scriptdata.bot.name[i][0] == '\0') {
         continue;
      }
      count++;
   }

   return count;
}



/*
=================
StartServer_BotOnSelectionList
=================
*/
qboolean StartServer_BotOnSelectionList( const char *checkName )
{
	int i;

	for (i = 0 ; i < PLAYER_SLOTS; i++) {
		if (s_scriptdata.bot.slotType[i] != SLOTTYPE_BOT)
			continue;

		if (Q_stricmp(checkName, s_scriptdata.bot.name[i]) == 0)
			return qtrue;
	}

	return qfalse;
}





/*
=================
StartServer_DeleteBotSlot
=================
*/
qboolean StartServer_DeleteBotSlot(int index)
{
	int i, count, last;

	if (index < 0 || index >= PLAYER_SLOTS)
		return qfalse;

	// number of slots to move
	count = PLAYER_SLOTS - index - 1;
	if (s_scriptdata.gametype >= GT_TEAM && !(s_scriptdata.gametype == GT_LMS)  && !StartServer_IsRandomBotExclude(s_scriptdata.bot.typeSelect) && index < PLAYER_SLOTS_PERCOL) {
		count -= PLAYER_SLOTS_PERCOL;
	}

	if (count) {
		for (i = index; i < index + count; i++) {
			memcpy(s_scriptdata.bot.name[i], s_scriptdata.bot.name[i + 1], MAX_NAME_LENGTH);
			memcpy(&s_scriptdata.bot.skill[i], &s_scriptdata.bot.skill[i + 1], sizeof(botskill_t));
			s_scriptdata.bot.slotType[i] = s_scriptdata.bot.slotType[i + 1];
		}
	}

	// zero last slot
	last = index + count;
	memset(s_scriptdata.bot.name[last], 0, MAX_NAME_LENGTH);
	s_scriptdata.bot.skill[last].value = 0;
	s_scriptdata.bot.skill[last].low = 0;
	s_scriptdata.bot.skill[last].high = 0;

	s_scriptdata.bot.slotType[last] = SLOTTYPE_EMPTY;

	return qtrue;
}



/*
=================
StartServer_InsertBotSlot
=================
*/
qboolean StartServer_InsertBotSlot(int index)
{
	int i, count;

	if (index < 0 || index >= PLAYER_SLOTS)
		return qfalse;

	// number of slots to move
	count = PLAYER_SLOTS - index - 1;
	if (s_scriptdata.gametype >= GT_TEAM && !(s_scriptdata.gametype == GT_LMS)  && index < PLAYER_SLOTS_PERCOL) {
		count -= PLAYER_SLOTS_PERCOL;
	}

	if (count) {
		for (i = index + count - 1 ; i >= index; i--) {
			// memcpy causes problems when copying "up", so move one entry at a time
			// write this before discovering memmove!
			memcpy(s_scriptdata.bot.name[i + 1], s_scriptdata.bot.name[i], MAX_NAME_LENGTH);
			memcpy(&s_scriptdata.bot.skill[i + 1], &s_scriptdata.bot.skill[i], sizeof(botskill_t));
			s_scriptdata.bot.slotType[i + 1] = s_scriptdata.bot.slotType[i];
		}
	}

	memset(s_scriptdata.bot.name[index], 0, MAX_NAME_LENGTH);
	s_scriptdata.bot.skill[index].value = 0;
	s_scriptdata.bot.skill[index].low = 0;
	s_scriptdata.bot.skill[index].high = 0;

	s_scriptdata.bot.slotType[index] = SLOTTYPE_EMPTY;

	return qtrue;
}



/*
=================
StartServer_SetNamedBot
=================
*/
void StartServer_SetNamedBot(int index, char* name)
{
	qboolean init;

	if (index < 0 || index >= PLAYER_SLOTS || !name)
		return;

	if (s_scriptdata.bot.name[index][0] == '\0') {
		s_scriptdata.bot.skill[index] = s_scriptdata.bot.globalSkill;
	}

	Q_strncpyz(s_scriptdata.bot.name[index], name, MAX_NAME_LENGTH);
	s_scriptdata.bot.slotType[index] = SLOTTYPE_BOT;
}



/*
=================
StartServer_InsertNamedBot
=================
*/
void StartServer_InsertNamedBot(int index, char* name)
{
	if (index < 0 || index >= PLAYER_SLOTS || !name)
		return;

	StartServer_InsertBotSlot(index);
	StartServer_SetNamedBot(index, name);
}



/*
=================
StartServer_SlotTeam
=================
*/
int StartServer_SlotTeam(int index)
{
	if (index < 0)
		return SLOTTEAM_INVALID;

	if (s_scriptdata.gametype >= GT_TEAM && !(s_scriptdata.gametype == GT_LMS) ) {
		if (index < PLAYER_SLOTS_PERCOL)
			return SLOTTEAM_ONE;
		if (index < PLAYER_SLOTS)
			return SLOTTEAM_TWO;
	}
	else {
		if (index < PLAYER_SLOTS)
			return SLOTTEAM_NONE;
	}

	return SLOTTEAM_INVALID;
}




/*
=================
StartServer_MoveBotToOtherTeam
=================
*/
void StartServer_MoveBotToOtherTeam(int selected)
{
	int i, firstopen, dest, max, start;

	if (selected < 0 || selected >= PLAYER_SLOTS)
		return;

	if (s_scriptdata.bot.slotType[selected] != SLOTTYPE_BOT)
		return;

	// try to find an empty slot first
	firstopen = -1;
	max = PLAYER_SLOTS_PERCOL;
	start = 0;
	if (selected < PLAYER_SLOTS_PERCOL) {
		max = PLAYER_SLOTS;
		start = PLAYER_SLOTS_PERCOL;
	}

	dest = -1;
	for (i = start; i < max ; i++)
	{
		if (firstopen == -1 && s_scriptdata.bot.slotType[i] == SLOTTYPE_OPEN)
			firstopen = i;

		if (s_scriptdata.bot.slotType[i] == SLOTTYPE_EMPTY) {
			dest = i;
			break;
		}
	}

	// use openslot if we have no free ones
	if (dest == -1) {
		if (firstopen == -1)
			return;
		dest = firstopen;
	}

	
	// copy over details
	Q_strncpyz(s_scriptdata.bot.name[dest], s_scriptdata.bot.name[selected], MAX_NAME_LENGTH);

	s_scriptdata.bot.slotType[dest] = s_scriptdata.bot.slotType[selected];

	s_scriptdata.bot.skill[dest].value = s_scriptdata.bot.skill[selected].value;
	s_scriptdata.bot.skill[dest].high= s_scriptdata.bot.skill[selected].high;
	s_scriptdata.bot.skill[dest].low = s_scriptdata.bot.skill[selected].low;

	StartServer_DeleteBotSlot(selected);
}



/*
=================
StartServer_BotNameDrawn
=================
*/
void StartServer_BotNameDrawn(int index, qboolean drawn)
{
	s_scriptdata.bot.drawName[index] = drawn;
}



/*
=================
StartServer_BotDoAction
=================
*/
void StartServer_DoBotAction(int action, int selected)
{
	int i;
	int count, index, open, slot, bots_done;

	switch (action) {
		case BOT_CT_CLEARALL:
			for (i = 0; i < PLAYER_SLOTS; i++) {
				if (s_scriptdata.bot.slotType[i] == SLOTTYPE_BOT || s_scriptdata.bot.slotType[i] == SLOTTYPE_OPEN)
				{
					s_scriptdata.bot.slotType[i] = SLOTTYPE_EMPTY;
					StartServer_BotNameDrawn(i, qfalse);
					memset(s_scriptdata.bot.name[i], 0, MAX_NAME_LENGTH);
				}
			}
			break;
		case BOT_CT_INDIV_SELECTED:
			if (selected >=0) {
				s_scriptdata.bot.skill[selected].value = s_scriptdata.bot.globalSkill.value;
			}
			break;
		case BOT_CT_RANGE_SELECTED:
			if (selected >=0) {
				s_scriptdata.bot.skill[selected].low = s_scriptdata.bot.globalSkill.low;
				s_scriptdata.bot.skill[selected].high = s_scriptdata.bot.globalSkill.high;
			}
			break;
		case BOT_CT_INDIV_ALL:
			for (i = 0; i < PLAYER_SLOTS; i++) {
				s_scriptdata.bot.skill[i].value = s_scriptdata.bot.globalSkill.value;
			}
			break;
		case BOT_CT_RANGE_ALL:
			for (i = 0; i < PLAYER_SLOTS; i++) {
				s_scriptdata.bot.skill[i].low = s_scriptdata.bot.globalSkill.low;
				s_scriptdata.bot.skill[i].high = s_scriptdata.bot.globalSkill.high;
			}
			break;

		case BOT_CT_NEATEN:

			// perform two passes when we have a team arrangement
			// once on each column, treated separately
			bots_done = 0;
			do {
				open = 0;
				count = PLAYER_SLOTS;
				index = bots_done;
				if (s_scriptdata.gametype >= GT_TEAM && !(s_scriptdata.gametype == GT_LMS)  && !StartServer_IsRandomBotExclude(s_scriptdata.bot.typeSelect)) {
					count = PLAYER_SLOTS_PERCOL;
				}

				// compact all the bots, counting the open slots
				while (count > 0) {
					slot = s_scriptdata.bot.slotType[index];
					if (slot == SLOTTYPE_OPEN)
						open++;

					if (slot == SLOTTYPE_BOT || slot == SLOTTYPE_HUMAN)
						index++;
					else
						StartServer_DeleteBotSlot(index);

					count--;
					bots_done++;
				}

				// place all the open slots
				for (i = 0; i < open; i++) {
					s_scriptdata.bot.slotType[index] = SLOTTYPE_OPEN;
					index++;
				}
			} while (bots_done < PLAYER_SLOTS);

			break;
	}
}



/*
=================
StartServer_ValidateBotSlotCount
=================
*/
void StartServer_ValidateBotSlotCount(int bots, int open)
{
	if (bots < 0)
		bots = 0;

	if (open < 0)
		open = 0;

	if (bots > MAX_CLIENTS)
		bots = MAX_CLIENTS;

	if (open > MAX_CLIENTS)
		open = MAX_CLIENTS;

	// sacrifice open slots for bots
	if (bots + open > MAX_CLIENTS)
		open = MAX_CLIENTS - bots;

	s_scriptdata.bot.numberBots = bots;
	s_scriptdata.bot.numberOpen = open;	
}



/*
=================
StartServer_AdjustBotSelectionFromGametype

wraps the bot.typeSelect safely, based on the current gametype
=================
*/
static void StartServer_AdjustBotSelectionFromGametype(void)
{
	if (s_scriptdata.gametype >= GT_TEAM && !(s_scriptdata.gametype == GT_LMS)  && StartServer_IsBotArenaScript(s_scriptdata.bot.typeSelect))
		s_scriptdata.bot.typeSelect = BOTTYPE_SELECT;
}



/*
=================
StartServer_LoadBotNameList
=================
*/
void StartServer_LoadBotNameList(int type)
{
	char* s;
	char* s1;
	int i;

	s_scriptdata.bot.typeSelect = type;
	s = gametype_cvar_base[ s_scriptdata.gametype ];

	memset(&s_scriptdata.bot.name, 0, PLAYER_SLOTS*MAX_NAME_LENGTH);

	for (i = 0 ; i < PLAYER_SLOTS; i++) {
		s_scriptdata.bot.slotType[i] = SLOTTYPE_EMPTY;
		StartServer_SetBotSkillValue(&s_scriptdata.bot.skill[i], 0 );
	}

    StartServer_AdjustBotSelectionFromGametype();

	// check if we need to load any bot data at all
	if (s_scriptdata.bot.typeSelect == BOTTYPE_ARENASCRIPT)
		return;

	// find the right type of bot data to load
	if (StartServer_IsRandomBotExclude(s_scriptdata.bot.typeSelect)) {
		s1 = "botexclude";
	}
	else {
		s1 = "botname";
	}	

    // do the load
	UI_LoadMultiArray( s, s1, SSBP_BotName_Callback, PLAYER_SLOTS, MAX_NAME_LENGTH, ';' );

	if (StartServer_IsRandomBotExclude(s_scriptdata.bot.typeSelect))
	{
		for ( i = 0 ; i < PLAYER_SLOTS; i++) {
			if (s_scriptdata.bot.name[i][0])
				s_scriptdata.bot.slotType[i] = SLOTTYPE_BOT;
			else
				s_scriptdata.bot.slotType[i] = SLOTTYPE_EMPTY;
			StartServer_SetBotSkillValue(&s_scriptdata.bot.skill[i], 0 );
		}
	}
	else
	{
		UI_LoadMultiArray( s, "slottype", SSBP_BotBuffer_Callback, PLAYER_SLOTS, BOT_TMPBUFFER, ';' );
		for ( i = 0 ; i < PLAYER_SLOTS; i++) {
			s_scriptdata.bot.slotType[i] = (int)Com_Clamp(0, SLOTTYPE_COUNT, atoi(botskill_tmpbuffer[i]));
		}

		UI_LoadMultiArray( s, "botskill", SSBP_BotBuffer_Callback, PLAYER_SLOTS, BOT_TMPBUFFER, ';' );
		for ( i = 0 ; i < PLAYER_SLOTS; i++) {
			StartServer_SetBotSkillValue(&s_scriptdata.bot.skill[i], atoi(botskill_tmpbuffer[i]) );
		}

		if (!s_scriptdata.bot.joinAs)
			s_scriptdata.bot.slotType[0] = SLOTTYPE_HUMAN;
	}

	// set any slot other that first as non-human
	// (only tampering should cause this)
	for ( i = 1; i < PLAYER_SLOTS; i++ ) {
		if (s_scriptdata.bot.slotType[i] == SLOTTYPE_HUMAN) {
			s_scriptdata.bot.slotType[i] = SLOTTYPE_OPEN;
		}
	}
}



/*
=================
StartServer_SaveBotNameList
=================
*/
void StartServer_SaveBotNameList(void)
{
	char* s;
	char* s1;
	int i;
	botskill_t* b;
	qboolean exclude;

	s = gametype_cvar_base[ s_scriptdata.gametype ];

	if (StartServer_IsRandomBotExclude(s_scriptdata.bot.typeSelect)) {
		exclude = qtrue;
		s1 = "botexclude";
	}
	else
	{
		exclude = qfalse;
		s1 = "botname";
	}

	UI_SaveMultiArray( s, s1, SSBP_BotName_Callback, PLAYER_SLOTS, MAX_NAME_LENGTH, ';' );

	if (!exclude)
	{
		for ( i = 0 ; i < PLAYER_SLOTS; i++) {
			Q_strncpyz( botskill_tmpbuffer[i], va("%i", s_scriptdata.bot.slotType[i]), BOT_TMPBUFFER);
		}
		UI_SaveMultiArray( s, "slottype", SSBP_BotBuffer_Callback, PLAYER_SLOTS, BOT_TMPBUFFER, ';' );

		for ( i = 0 ; i < PLAYER_SLOTS; i++) {
			b = &s_scriptdata.bot.skill[i];
			Q_strncpyz( botskill_tmpbuffer[i], va("%i%i%i", b->value, b->low, b->high), BOT_TMPBUFFER);
		}
		UI_SaveMultiArray( s, "botskill", SSBP_BotBuffer_Callback, PLAYER_SLOTS, BOT_TMPBUFFER, ';' );
	}
}





/*
=================
StartServer_LoadBotScriptData

Loads bot specific gametype data
=================
*/
void StartServer_LoadBotScriptData(void)
{
	char* s;
	int i, value, max;
	botskill_t* b;

	s = gametype_cvar_base[ s_scriptdata.gametype ];

	//
	// load state values
	//

	// join game as
	s_scriptdata.bot.joinAs = UIE_GetSkirmishCvarIntClamp(0, 1, s, "PlayerJoinAs");

	// skill selection method for bots
	// assumes BotSelection method is already set in Start_Server_LoadBotNameList()
	StartServer_SetBotSkillRangeType(UIE_GetSkirmishCvarIntClamp(0, BOTSKILL_COUNT, s, "BotSkillType"));

	// number of bots if randomly generated
	s_scriptdata.bot.numberBots = UIE_GetSkirmishCvarIntClamp(0, 256, s, "BotCount");

	// frequency of bot changing on maps
	s_scriptdata.bot.changeBots = UIE_GetSkirmishCvarIntClamp(0, BOTCHANGE_COUNT, s, "BotChange");

	// number of open slots if bots are randomly selected
	s_scriptdata.bot.numberOpen = UIE_GetSkirmishCvarIntClamp(0, 256, s, "OpenSlotCount");

	// skill range values
	StartServer_SetBotSkillValue(&s_scriptdata.bot.globalSkill, UIE_GetSkirmishCvarInt(s, "BotSkillValue"));

	// skill bias
	s_scriptdata.bot.skillBias = UIE_GetSkirmishCvarIntClamp(0, SKILLBIAS_COUNT, s, "BotSkillBias");

	// swap teams
	if (s_scriptdata.gametype >= GT_TEAM && !(s_scriptdata.gametype == GT_LMS) ) {
		s_scriptdata.bot.teamSwapped = UIE_GetSkirmishCvarIntClamp(0, 1, s, "TeamSwapped");
	}

	// load bot stats
	// requires bot.joinAs to be set
	StartServer_LoadBotNameList(UIE_GetSkirmishCvarIntClamp(0, BOTTYPE_MAX, s, "BotSelection"));
}




/*
=================
StartServer_SaveBotScriptData

Saves bot specific gametype data
=================
*/
static void StartServer_SaveBotScriptData(void)
{
	char* s;
	int i, value;
	botskill_t* b;

	//
	// save state values
	//

	s = gametype_cvar_base[ s_scriptdata.gametype ];

	// method of selecting bots
	UIE_SetSkirmishCvarInt(s, "BotSelection", s_scriptdata.bot.typeSelect);

	// number of bots if randomly generated
	UIE_SetSkirmishCvarInt(s, "BotCount", s_scriptdata.bot.numberBots);

	// frequency of bot changing on maps
	UIE_SetSkirmishCvarInt(s, "BotChange", s_scriptdata.bot.changeBots);

	// number of open slots if bots are randomly selected
	UIE_SetSkirmishCvarInt(s, "OpenSlotCount", s_scriptdata.bot.numberOpen);

	// skill selection method for bots
	UIE_SetSkirmishCvarInt(s, "BotSkillType", s_scriptdata.bot.skillType);

	// skill range values
	b = &s_scriptdata.bot.globalSkill;
	value = (b->value * 100) +  (b->low * 10) + b->high;
	UIE_SetSkirmishCvarInt(s, "BotSkillValue", value);

	// skill bias
	UIE_SetSkirmishCvarInt(s, "BotSkillBias", s_scriptdata.bot.skillBias);

	// join game as
	UIE_SetSkirmishCvarInt(s, "PlayerJoinAs", s_scriptdata.bot.joinAs);

	// swap teams
	if (s_scriptdata.gametype >= GT_TEAM)
		UIE_SetSkirmishCvarInt(s, "TeamSwapped", s_scriptdata.bot.teamSwapped);

	//
	// bots
	//

	// save bot stats
	StartServer_SaveBotNameList();
}



/*
==========================================================================

LOADING AND SAVING OF ITEM SCRIPT DATA

==========================================================================
*/



/*
=================
StartServer_LoadItemScriptData

Loads item specific gametype data
=================
*/
void StartServer_LoadDisabledItems(void)
{
	char* s;
	char buffer[256];
	char* ptr, *ptr_last;
	int i;

	s = gametype_cvar_base[ s_scriptdata.gametype ];

	// load the group on/off/custom values
	memset(buffer,0,sizeof(buffer));
	UIE_GetSkirmishCvar(s, "itemGroups", buffer, 256);
	ptr = buffer;
	for (i = 0; i < ITEMGROUP_COUNT; i++)
	{
		if (*ptr) {
			s_scriptdata.item.groupstate[i] = (int)Com_Clamp(0, ALLGROUPS_HIDDEN, *ptr - '0');
			ptr++;
		}
		else {
			s_scriptdata.item.groupstate[i] = ALLGROUPS_ENABLED;
		}
	}

	// load individual item values
	// we only load a list of items that are "off"
	// no assumption about order is made
	for (i = 0; i < ITEM_COUNT; i++)
	{
		s_scriptdata.item.enabled[i] = qtrue;
	}

	memset(buffer, 0, sizeof(buffer));
	UIE_GetSkirmishCvar(s, "itemsHidden", buffer, 256);
	ptr = buffer;
	while (*ptr) {
		ptr_last = strchr(ptr, '\\');

		if (!ptr_last)
			break;

		*ptr_last = '\0';
		for (i = 0; i < ITEM_COUNT; i++) {
			if (!Q_stricmp(ptr, server_itemlist[i].shortitem))
			{
				s_scriptdata.item.enabled[i] = qfalse;
				break;
			}

		}

		// move to next char
		ptr = ptr_last + 1;
	}
}



/*
=================
StartServer_LoadItemScriptData

Loads item specific gametype data
=================
*/
static void StartServer_LoadItemScriptData(void)
{
	StartServer_LoadDisabledItems();
}



/*
=================
StartServer_SaveItemScriptData

Saves item specific gametype data
=================
*/
static void StartServer_SaveItemScriptData(void)
{
	char* s;
	char buffer[256];
	int i;

	s = gametype_cvar_base[ s_scriptdata.gametype ];

	// save the group values
	for (i = 0; i < ITEMGROUP_COUNT; i++)
	{
		buffer[i] = '0' + s_scriptdata.item.groupstate[i];
	}
	buffer[ITEMGROUP_COUNT] = '\0';

	UIE_SetSkirmishCvar(s, "itemGroups", buffer);

	// save individual item values
	// we only save a list of items that are "off"
	// always terminate with a slash
	buffer[0] = '\0';
	for (i = 0; i < ITEM_COUNT; i++)
	{
		if (!s_scriptdata.item.enabled[i]) {
			Q_strcat(buffer, 256, va("%s\\", server_itemlist[i].shortitem));
		}
	}
	UIE_SetSkirmishCvar(s, "itemsHidden", buffer);
}




/*
==========================================================================

LOADING AND SAVING OF SERVER SCRIPT DATA

==========================================================================
*/



/*
=================
StartServer_LoadServerScriptData

Loads server specific gametype data
=================
*/
static void StartServer_LoadServerScriptData(void)
{
	char* s, *t;
	int value, gametype;

	s_scriptdata.server.pure = UIE_GetSkirmishCvarIntClamp(0, 1, NULL, "uie_pure" );
	s_scriptdata.server.dynamiclight = UIE_GetSkirmishCvarIntClamp(0, 1, NULL, "uie_dynamiclight" );
	s_scriptdata.server.pmove_fixed = UIE_GetSkirmishCvarIntClamp(0, 1, NULL, "uie_pmove_fixed" );
	s_scriptdata.server.pmove_msec = UIE_GetSkirmishCvarIntClamp(0, 999, NULL, "uie_pmove_msec" );

	s_scriptdata.server.smoothclients = UIE_GetSkirmishCvarIntClamp(0, 1, NULL, "uie_smoothclients" );
	s_scriptdata.server.syncClients = UIE_GetSkirmishCvarIntClamp(0, 1, NULL, "uie_syncclients" );

	s_scriptdata.server.minPing = UIE_GetSkirmishCvarIntClamp(0, 999, NULL, "uie_minping" );
	s_scriptdata.server.maxPing = UIE_GetSkirmishCvarIntClamp(0, 999, NULL, "uie_maxping" );
	s_scriptdata.server.allowMinPing = UIE_GetSkirmishCvarIntClamp(0, 1, NULL, "uie_allowMinPing" );
	s_scriptdata.server.allowMaxPing = UIE_GetSkirmishCvarIntClamp(0, 1, NULL, "uie_allowMaxPing" );

	s_scriptdata.server.allowmaxrate = UIE_GetSkirmishCvarIntClamp(0, 1, NULL, "uie_allowmaxrate" );
	s_scriptdata.server.maxrate = UIE_GetSkirmishCvarIntClamp(0, 99999, NULL, "uie_maxrate" );

	s_scriptdata.server.allowdownload = UIE_GetSkirmishCvarIntClamp(0, 1, NULL, "uie_allowdownload" );
	s_scriptdata.server.allowvote = UIE_GetSkirmishCvarIntClamp(0, 1, NULL, "uie_allowvote" );
	s_scriptdata.server.allowpass = UIE_GetSkirmishCvarIntClamp(0, 1, NULL, "uie_allowpass" );

	s_scriptdata.server.netport = UIE_GetSkirmishCvarIntClamp(1024, 65535, NULL, "uie_netport" );
	s_scriptdata.server.sv_fps = UIE_GetSkirmishCvarIntClamp(0, 160, NULL, "uie_svfps" );

	UIE_GetSkirmishCvar(NULL, "uie_password",s_scriptdata.server.password, MAX_PASSWORD_LENGTH);

	s_scriptdata.server.allowPrivateClients = UIE_GetSkirmishCvarIntClamp(0, 32, NULL, "uie_allowprivateclients" );
	s_scriptdata.server.privateClients = UIE_GetSkirmishCvarIntClamp(0, 32, NULL, "uie_privateclients" );
	UIE_GetSkirmishCvar(NULL, "uie_privatepassword",s_scriptdata.server.privatePassword, MAX_PASSWORD_LENGTH);

	s_scriptdata.server.preventConfigBug = UIE_GetSkirmishCvarIntClamp(0, 1, NULL, "uie_preventConfigBug");

	if (s_scriptdata.multiplayer) {
		s_scriptdata.server.dedicatedServer = UIE_GetSkirmishCvarIntClamp(0, SRVDED_INTERNET, NULL, "uie_dedicated" );
		s_scriptdata.server.inactivityTime = UIE_GetSkirmishCvarIntClamp(0, 999, NULL, "uie_inactivity" );

		s_scriptdata.server.strictAuth = UIE_GetSkirmishCvarIntClamp(0, 1, NULL, "uie_strictAuth" );
		s_scriptdata.server.lanForceRate = UIE_GetSkirmishCvarIntClamp(0, 1, NULL, "uie_lanForceRate" );
	}
	else {
		s_scriptdata.server.dedicatedServer = SRVDED_OFF;
		s_scriptdata.server.inactivityTime = 0;

		s_scriptdata.server.strictAuth = 0;
		s_scriptdata.server.lanForceRate = 0;
	}
	
//Noire.dev cvars
s_scriptdata.server.maxEntities = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_maxEntities" );
s_scriptdata.server.singleskill = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_singleskill" );
s_scriptdata.server.kill = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_kill" );
UIE_GetSkirmishCvar(NULL, "uie_damageModifier",s_scriptdata.server.damageModifier, 32);
UIE_GetSkirmishCvar(NULL, "uie_selectedmod",s_scriptdata.server.selectedmod, 32);
s_scriptdata.server.elimination = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination" );
s_scriptdata.server.obeliskHealth = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_obeliskHealth" );
s_scriptdata.server.obeliskRegenPeriod = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_obeliskRegenPeriod" );
s_scriptdata.server.obeliskRegenAmount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_obeliskRegenAmount" );
s_scriptdata.server.obeliskRespawnDelay = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_obeliskRespawnDelay" );
s_scriptdata.server.cubeTimeout = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_cubeTimeout" );
s_scriptdata.server.flagrespawn = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_flagrespawn" );
s_scriptdata.server.weaponTeamRespawn = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_weaponTeamRespawn" );
s_scriptdata.server.elimination_ctf_oneway = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_ctf_oneway" );
s_scriptdata.server.elimination_selfdamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_selfdamage" );
s_scriptdata.server.elimination_roundtime = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_roundtime" );
s_scriptdata.server.elimination_warmup = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_warmup" );
s_scriptdata.server.elimination_activewarmup = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_activewarmup" );
s_scriptdata.server.lms_lives = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_lms_lives" );
s_scriptdata.server.lms_mode = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_lms_mode" );
s_scriptdata.server.accelerate = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_accelerate" );
s_scriptdata.server.spectatorspeed = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_spectatorspeed" );
s_scriptdata.server.speed = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_speed" );
s_scriptdata.server.gravity = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_gravity" );
UIE_GetSkirmishCvar(NULL, "uie_gravityModifier",s_scriptdata.server.gravityModifier, 32);
s_scriptdata.server.knockback = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_knockback" );
s_scriptdata.server.noplayerclip = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_noplayerclip" );
s_scriptdata.server.jumpheight = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_jumpheight" );
s_scriptdata.server.regenarmor = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_regenarmor" );
s_scriptdata.server.ammolimit = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ammolimit" );
UIE_GetSkirmishCvar(NULL, "uie_quadfactor",s_scriptdata.server.quadfactor, 32);
s_scriptdata.server.respawntime = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_respawntime" );
s_scriptdata.server.forcerespawn = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_forcerespawn" );
UIE_GetSkirmishCvar(NULL, "uie_vampire",s_scriptdata.server.vampire, 32);
s_scriptdata.server.vampire_max_health = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_vampire_max_health" );
s_scriptdata.server.regen = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_regen" );
s_scriptdata.server.maxweaponpickup = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_maxweaponpickup" );
s_scriptdata.server.droppeditemtime = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_droppeditemtime" );
s_scriptdata.server.autoflagreturn = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_autoflagreturn" );
UIE_GetSkirmishCvar(NULL, "uie_armorprotect",s_scriptdata.server.armorprotect, 32);
s_scriptdata.server.respawnwait = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_respawnwait" );
UIE_GetSkirmishCvar(NULL, "uie_speedfactor",s_scriptdata.server.speedfactor, 32);
UIE_GetSkirmishCvar(NULL, "uie_scoutspeedfactor",s_scriptdata.server.scoutspeedfactor, 32);
UIE_GetSkirmishCvar(NULL, "uie_scoutfirespeed",s_scriptdata.server.scoutfirespeed, 32);
UIE_GetSkirmishCvar(NULL, "uie_scoutdamagefactor",s_scriptdata.server.scoutdamagefactor, 32);
UIE_GetSkirmishCvar(NULL, "uie_scoutgravitymodifier",s_scriptdata.server.scoutgravitymodifier, 32);
s_scriptdata.server.scout_infammo = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_scout_infammo" );
UIE_GetSkirmishCvar(NULL, "uie_scouthealthmodifier",s_scriptdata.server.scouthealthmodifier, 32);
UIE_GetSkirmishCvar(NULL, "uie_doublerfirespeed",s_scriptdata.server.doublerfirespeed, 32);
UIE_GetSkirmishCvar(NULL, "uie_doublerdamagefactor",s_scriptdata.server.doublerdamagefactor, 32);
UIE_GetSkirmishCvar(NULL, "uie_doublerspeedfactor",s_scriptdata.server.doublerspeedfactor, 32);
UIE_GetSkirmishCvar(NULL, "uie_doublergravitymodifier",s_scriptdata.server.doublergravitymodifier, 32);
s_scriptdata.server.doubler_infammo = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_doubler_infammo" );
UIE_GetSkirmishCvar(NULL, "uie_doublerhealthmodifier",s_scriptdata.server.doublerhealthmodifier, 32);
UIE_GetSkirmishCvar(NULL, "uie_guardhealthmodifier",s_scriptdata.server.guardhealthmodifier, 32);
UIE_GetSkirmishCvar(NULL, "uie_guardfirespeed",s_scriptdata.server.guardfirespeed, 32);
UIE_GetSkirmishCvar(NULL, "uie_guarddamagefactor",s_scriptdata.server.guarddamagefactor, 32);
UIE_GetSkirmishCvar(NULL, "uie_guardspeedfactor",s_scriptdata.server.guardspeedfactor, 32);
UIE_GetSkirmishCvar(NULL, "uie_guardgravitymodifier",s_scriptdata.server.guardgravitymodifier, 32);
s_scriptdata.server.guard_infammo = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_guard_infammo" );
UIE_GetSkirmishCvar(NULL, "uie_ammoregenfirespeed",s_scriptdata.server.ammoregenfirespeed, 32);
s_scriptdata.server.ammoregen_infammo = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ammoregen_infammo" );
UIE_GetSkirmishCvar(NULL, "uie_ammoregendamagefactor",s_scriptdata.server.ammoregendamagefactor, 32);
UIE_GetSkirmishCvar(NULL, "uie_ammoregenspeedfactor",s_scriptdata.server.ammoregenspeedfactor, 32);
UIE_GetSkirmishCvar(NULL, "uie_ammoregengravitymodifier",s_scriptdata.server.ammoregengravitymodifier, 32);
UIE_GetSkirmishCvar(NULL, "uie_ammoregenhealthmodifier",s_scriptdata.server.ammoregenhealthmodifier, 32);
s_scriptdata.server.teamred_speed = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_teamred_speed" );
UIE_GetSkirmishCvar(NULL, "uie_teamred_gravityModifier",s_scriptdata.server.teamred_gravityModifier, 32);
UIE_GetSkirmishCvar(NULL, "uie_teamred_firespeed",s_scriptdata.server.teamred_firespeed, 32);
UIE_GetSkirmishCvar(NULL, "uie_teamred_damage",s_scriptdata.server.teamred_damage, 32);
s_scriptdata.server.teamred_infammo = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_teamred_infammo" );
s_scriptdata.server.teamred_respawnwait = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_teamred_respawnwait" );
s_scriptdata.server.teamred_pickupitems = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_teamred_pickupitems" );
s_scriptdata.server.eliminationredrespawn = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_eliminationredrespawn" );
s_scriptdata.server.eliminationred_startHealth = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_eliminationred_startHealth" );
s_scriptdata.server.eliminationred_startArmor = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_eliminationred_startArmor" );
s_scriptdata.server.teamblue_speed = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_teamblue_speed" );
UIE_GetSkirmishCvar(NULL, "uie_teamblue_gravityModifier",s_scriptdata.server.teamblue_gravityModifier, 32);
UIE_GetSkirmishCvar(NULL, "uie_teamblue_firespeed",s_scriptdata.server.teamblue_firespeed, 32);
UIE_GetSkirmishCvar(NULL, "uie_teamblue_damage",s_scriptdata.server.teamblue_damage, 32);
s_scriptdata.server.teamblue_infammo = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_teamblue_infammo" );
s_scriptdata.server.teamblue_respawnwait = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_teamblue_respawnwait" );
s_scriptdata.server.teamblue_pickupitems = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_teamblue_pickupitems" );
s_scriptdata.server.eliminationrespawn = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_eliminationrespawn" );
s_scriptdata.server.elimination_startHealth = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_startHealth" );
s_scriptdata.server.elimination_startArmor = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_startArmor" );
s_scriptdata.server.eliminationred_grapple = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_eliminationred_grapple" );
s_scriptdata.server.eliminationred_gauntlet = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_eliminationred_gauntlet" );
s_scriptdata.server.eliminationred_machinegun = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_eliminationred_machinegun" );
s_scriptdata.server.eliminationred_shotgun = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_eliminationred_shotgun" );
s_scriptdata.server.eliminationred_grenade = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_eliminationred_grenade" );
s_scriptdata.server.eliminationred_rocket = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_eliminationred_rocket" );
s_scriptdata.server.eliminationred_railgun = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_eliminationred_railgun" );
s_scriptdata.server.eliminationred_lightning = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_eliminationred_lightning" );
s_scriptdata.server.eliminationred_plasmagun = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_eliminationred_plasmagun" );
s_scriptdata.server.eliminationred_bfg = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_eliminationred_bfg" );
s_scriptdata.server.eliminationred_chain = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_eliminationred_chain" );
s_scriptdata.server.eliminationred_mine = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_eliminationred_mine" );
s_scriptdata.server.eliminationred_nail = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_eliminationred_nail" );
s_scriptdata.server.eliminationred_flame = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_eliminationred_flame" );
s_scriptdata.server.eliminationred_antimatter = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_eliminationred_antimatter" );
s_scriptdata.server.elimination_grapple = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_grapple" );
s_scriptdata.server.elimination_gauntlet = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_gauntlet" );
s_scriptdata.server.elimination_machinegun = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_machinegun" );
s_scriptdata.server.elimination_shotgun = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_shotgun" );
s_scriptdata.server.elimination_grenade = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_grenade" );
s_scriptdata.server.elimination_rocket = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_rocket" );
s_scriptdata.server.elimination_railgun = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_railgun" );
s_scriptdata.server.elimination_lightning = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_lightning" );
s_scriptdata.server.elimination_plasmagun = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_plasmagun" );
s_scriptdata.server.elimination_bfg = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_bfg" );
s_scriptdata.server.elimination_chain = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_chain" );
s_scriptdata.server.elimination_mine = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_mine" );
s_scriptdata.server.elimination_nail = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_nail" );
s_scriptdata.server.elimination_flame = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_flame" );
s_scriptdata.server.elimination_antimatter = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_antimatter" );
s_scriptdata.server.eliminationred_quad = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_eliminationred_quad" );
s_scriptdata.server.elimination_haste = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_haste" );
s_scriptdata.server.elimination_bsuit = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_bsuit" );
s_scriptdata.server.elimination_invis = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_invis" );
s_scriptdata.server.elimination_regen = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_regen" );
s_scriptdata.server.elimination_flight = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_flight" );
s_scriptdata.server.elimination_items = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_items" );
s_scriptdata.server.elimination_holdable = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_holdable" );
s_scriptdata.server.minigame = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_minigame" );
s_scriptdata.server.overlay = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_overlay" );
s_scriptdata.server.randomItems = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_randomItems" );
s_scriptdata.server.slickmove = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_slickmove" );
s_scriptdata.server.spawnprotect = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_spawnprotect" );
s_scriptdata.server.elimination_lockspectator = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_elimination_lockspectator" );
s_scriptdata.server.awardpushing = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_awardpushing" );
s_scriptdata.server.randomteleport = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_randomteleport" );
s_scriptdata.server.falldamagesmall = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_falldamagesmall" );
s_scriptdata.server.falldamagebig = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_falldamagebig" );
s_scriptdata.server.waterdamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_waterdamage" );
s_scriptdata.server.lavadamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_lavadamage" );
s_scriptdata.server.slimedamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_slimedamage" );
s_scriptdata.server.drowndamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_drowndamage" );
s_scriptdata.server.invulinf = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_invulinf" );
s_scriptdata.server.invulmove = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_invulmove" );
s_scriptdata.server.invultime = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_invultime" );
s_scriptdata.server.kamikazeinf = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_kamikazeinf" );
s_scriptdata.server.portalinf = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_portalinf" );
s_scriptdata.server.portaltimeout = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_portaltimeout" );
s_scriptdata.server.portalhealth = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_portalhealth" );
s_scriptdata.server.teleporterinf = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_teleporterinf" );
s_scriptdata.server.medkitinf = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_medkitinf" );
s_scriptdata.server.medkitlimit = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_medkitlimit" );
s_scriptdata.server.medkitmodifier = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_medkitmodifier" );
s_scriptdata.server.fasthealthregen = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_fasthealthregen" );
s_scriptdata.server.slowhealthregen = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_slowhealthregen" );
UIE_GetSkirmishCvar(NULL, "uie_hastefirespeed",s_scriptdata.server.hastefirespeed, 32);
s_scriptdata.server.quadtime = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_quadtime" );
s_scriptdata.server.bsuittime = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_bsuittime" );
s_scriptdata.server.hastetime = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_hastetime" );
s_scriptdata.server.invistime = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_invistime" );
s_scriptdata.server.regentime = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_regentime" );
s_scriptdata.server.flighttime = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_flighttime" );
s_scriptdata.server.armorrespawn = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_armorrespawn" );
s_scriptdata.server.healthrespawn = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_healthrespawn" );
s_scriptdata.server.ammorespawn = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ammorespawn" );
s_scriptdata.server.holdablerespawn = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_holdablerespawn" );
s_scriptdata.server.megahealthrespawn = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_megahealthrespawn" );
s_scriptdata.server.poweruprespawn = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_poweruprespawn" );
s_scriptdata.server.weaponrespawn = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_weaponrespawn" );
// Крюк
s_scriptdata.server.g_ghspeed = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ghspeed" );
s_scriptdata.server.g_ghtimeout = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ghtimeout" );
// Пила
s_scriptdata.server.g_gdelay = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_gdelay" );
s_scriptdata.server.g_gdamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_gdamage" );
s_scriptdata.server.g_grange = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_grange" );
// Автомат
s_scriptdata.server.g_mgammocount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_mgammocount" );
s_scriptdata.server.g_mgweaponcount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_mgweaponcount" );
s_scriptdata.server.g_mgdelay = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_mgdelay" );
s_scriptdata.server.g_mgdamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_mgdamage" );
s_scriptdata.server.g_mgspread = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_mgspread" );
s_scriptdata.server.g_mgexplode = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_mgexplode" );
s_scriptdata.server.g_mgsdamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_mgsdamage" );
s_scriptdata.server.g_mgsradius = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_mgsradius" );
s_scriptdata.server.g_mgvampire = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_mgvampire" );
s_scriptdata.server.g_mginf = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_mginf" );
// Дробовик
s_scriptdata.server.g_sgammocount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_sgammocount" );
s_scriptdata.server.g_sgweaponcount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_sgweaponcount" );
s_scriptdata.server.g_sgdelay = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_sgdelay" );
s_scriptdata.server.g_sgdamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_sgdamage" );
s_scriptdata.server.g_sgspread = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_sgspread" );
s_scriptdata.server.g_sgexplode = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_sgexplode" );
s_scriptdata.server.g_sgsdamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_sgsdamage" );
s_scriptdata.server.g_sgsradius = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_sgsradius" );
s_scriptdata.server.g_sgcount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_sgcount" );
s_scriptdata.server.g_sgvampire = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_sgvampire" );
s_scriptdata.server.g_sginf = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_sginf" );

// Гранаты
s_scriptdata.server.g_glammocount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_glammocount" );
s_scriptdata.server.g_glweaponcount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_glweaponcount" );
s_scriptdata.server.g_gldelay = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_gldelay" );
s_scriptdata.server.g_glspeed = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_glspeed" );
s_scriptdata.server.g_glbounce = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_glbounce" );
s_scriptdata.server.g_glgravity = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_glgravity" );
s_scriptdata.server.g_gltimeout = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_gltimeout" );
s_scriptdata.server.g_glsradius = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_glsradius" );
s_scriptdata.server.g_glsdamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_glsdamage" );
s_scriptdata.server.g_gldamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_gldamage" );
s_scriptdata.server.g_glvampire = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_glvampire" );
s_scriptdata.server.g_glinf = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_glinf" );
s_scriptdata.server.g_glhoming = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_glhoming" );
s_scriptdata.server.g_glguided = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_glguided" );
// Ракеты
s_scriptdata.server.g_rlammocount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_rlammocount" );
s_scriptdata.server.g_rlweaponcount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_rlweaponcount" );
s_scriptdata.server.g_rldelay = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_rldelay" );
s_scriptdata.server.g_rlspeed = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_rlspeed" );
s_scriptdata.server.g_rlbounce = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_rlbounce" );
s_scriptdata.server.g_rlgravity = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_rlgravity" );
s_scriptdata.server.g_rltimeout = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_rltimeout" );
s_scriptdata.server.g_rlsradius = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_rlsradius" );
s_scriptdata.server.g_rlsdamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_rlsdamage" );
s_scriptdata.server.g_rldamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_rldamage" );
s_scriptdata.server.g_rlvampire = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_rlvampire" );
s_scriptdata.server.g_rlinf = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_rlinf" );
s_scriptdata.server.g_rlhoming = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_rlhoming" );
s_scriptdata.server.g_rlguided = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_rlguided" );
// Молния
s_scriptdata.server.g_lgammocount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_lgammocount" );
s_scriptdata.server.g_lgweaponcount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_lgweaponcount" );
s_scriptdata.server.g_lgrange = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_lgrange" );
s_scriptdata.server.g_lgdelay = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_lgdelay" );
s_scriptdata.server.g_lgdamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_lgdamage" );
s_scriptdata.server.g_lgvampire = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_lgvampire" );
s_scriptdata.server.g_lgexplode = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_lgexplode" );
s_scriptdata.server.g_lgsdamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_lgsdamage" );
s_scriptdata.server.g_lgsradius = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_lgsradius" );
s_scriptdata.server.g_lginf = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_lginf" );
// Рэйлган
s_scriptdata.server.g_rgammocount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_rgammocount" );
s_scriptdata.server.g_rgweaponcount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_rgweaponcount" );
s_scriptdata.server.g_rgdelay = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_rgdelay" );
s_scriptdata.server.g_rgdamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_rgdamage" );
s_scriptdata.server.g_rgvampire = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_rgvampire" );
s_scriptdata.server.g_rginf = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_rginf" );
// Плазмаган
s_scriptdata.server.g_pgammocount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_pgammocount" );
s_scriptdata.server.g_pgweaponcount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_pgweaponcount" );
s_scriptdata.server.g_pgdelay = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_pgdelay" );
s_scriptdata.server.g_pgspeed = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_pgspeed" );
s_scriptdata.server.g_pgbounce = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_pgbounce" );
s_scriptdata.server.g_pggravity = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_pggravity" );
s_scriptdata.server.g_pgtimeout = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_pgtimeout" );
s_scriptdata.server.g_pgsradius = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_pgsradius" );
s_scriptdata.server.g_pgsdamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_pgsdamage" );
s_scriptdata.server.g_pgdamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_pgdamage" );
s_scriptdata.server.g_pgvampire = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_pgvampire" );
s_scriptdata.server.g_pginf = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_pginf" );
s_scriptdata.server.g_pghoming = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_pghoming" );
s_scriptdata.server.g_pgguided = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_pgguided" );
// Бфг
s_scriptdata.server.g_bfgammocount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_bfgammocount" );
s_scriptdata.server.g_bfgweaponcount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_bfgweaponcount" );
s_scriptdata.server.g_bfgdelay = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_bfgdelay" );
s_scriptdata.server.g_bfgspeed = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_bfgspeed" );
s_scriptdata.server.g_bfgbounce = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_bfgbounce" );
s_scriptdata.server.g_bfggravity = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_bfggravity" );
s_scriptdata.server.g_bfgtimeout = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_bfgtimeout" );
s_scriptdata.server.g_bfgsradius = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_bfgsradius" );
s_scriptdata.server.g_bfgsdamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_bfgsdamage" );
s_scriptdata.server.g_bfgdamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_bfgdamage" );
s_scriptdata.server.g_bfgvampire = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_bfgvampire" );
s_scriptdata.server.g_bfginf = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_bfginf" );
s_scriptdata.server.g_bfghoming = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_bfghoming" );
s_scriptdata.server.g_bfgguided = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_bfgguided" );
// Гвоздомёт
s_scriptdata.server.g_ngammocount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ngammocount" );
s_scriptdata.server.g_ngweaponcount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ngweaponcount" );
s_scriptdata.server.g_ngdelay = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ngdelay" );
s_scriptdata.server.g_ngspeed = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ngspeed" );
s_scriptdata.server.g_ngbounce = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ngbounce" );
s_scriptdata.server.g_nggravity = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_nggravity" );
s_scriptdata.server.g_ngtimeout = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ngtimeout" );
s_scriptdata.server.g_ngcount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ngcount" );
s_scriptdata.server.g_ngspread = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ngspread" );
s_scriptdata.server.g_ngdamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ngdamage" );
s_scriptdata.server.g_ngrandom = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ngrandom" );
s_scriptdata.server.g_ngvampire = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ngvampire" );
s_scriptdata.server.g_nginf = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_nginf" );
s_scriptdata.server.g_nghoming = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_nghoming" );
s_scriptdata.server.g_ngguided = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ngguided" );
// Мины
s_scriptdata.server.g_plammocount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_plammocount" );
s_scriptdata.server.g_plweaponcount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_plweaponcount" );
s_scriptdata.server.g_pldelay = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_pldelay" );
s_scriptdata.server.g_plspeed = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_plspeed" );
s_scriptdata.server.g_plgravity = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_plgravity" );
s_scriptdata.server.g_pltimeout = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_pltimeout" );
s_scriptdata.server.g_plsradius = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_plsradius" );
s_scriptdata.server.g_plsdamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_plsdamage" );
s_scriptdata.server.g_pldamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_pldamage" );
s_scriptdata.server.g_plvampire = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_plvampire" );
s_scriptdata.server.g_plinf = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_plinf" );
// Пулемёт
s_scriptdata.server.g_cgammocount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_cgammocount" );
s_scriptdata.server.g_cgweaponcount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_cgweaponcount" );
s_scriptdata.server.g_cgdelay = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_cgdelay" );
s_scriptdata.server.g_cgspread = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_cgspread" );
s_scriptdata.server.g_cgdamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_cgdamage" );
s_scriptdata.server.g_cgvampire = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_cgvampire" );
s_scriptdata.server.g_cginf = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_cginf" );
// Огнемёт
s_scriptdata.server.g_ftammocount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ftammocount" );
s_scriptdata.server.g_ftweaponcount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ftweaponcount" );
s_scriptdata.server.g_ftdelay = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ftdelay" );
s_scriptdata.server.g_ftspeed = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ftspeed" );
s_scriptdata.server.g_ftbounce = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ftbounce" );
s_scriptdata.server.g_ftgravity = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ftgravity" );
s_scriptdata.server.g_fttimeout = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_fttimeout" );
s_scriptdata.server.g_ftsradius = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ftsradius" );
s_scriptdata.server.g_ftsdamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ftsdamage" );
s_scriptdata.server.g_ftdamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ftdamage" );
s_scriptdata.server.g_ftvampire = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ftvampire" );
s_scriptdata.server.g_ftinf = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ftinf" );
s_scriptdata.server.g_fthoming = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_fthoming" );
s_scriptdata.server.g_ftguided = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ftguided" );
// Антиматтер
s_scriptdata.server.g_amweaponcount = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_amweaponcount" );
s_scriptdata.server.g_amdelay = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_amdelay" );
s_scriptdata.server.g_amspeed = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_amspeed" );
s_scriptdata.server.g_ambounce = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_ambounce" );
s_scriptdata.server.g_amgravity = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_amgravity" );
s_scriptdata.server.g_amtimeout = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_amtimeout" );
s_scriptdata.server.g_amsradius = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_amsradius" );
s_scriptdata.server.g_amsdamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_amsdamage" );
s_scriptdata.server.g_amdamage = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_amdamage" );
s_scriptdata.server.g_amvampire = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_amvampire" );
s_scriptdata.server.g_aminf = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_aminf" );
s_scriptdata.server.g_amhoming = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_amhoming" );
s_scriptdata.server.g_amguided = UIE_GetSkirmishCvarIntClamp(-9999999, 9999999, NULL, "uie_amguided" );


UIE_GetSkirmishCvar(NULL, "uie_glbouncemodifier",s_scriptdata.server.g_glbouncemodifier, 32);
UIE_GetSkirmishCvar(NULL, "uie_rlbouncemodifier",s_scriptdata.server.g_rlbouncemodifier, 32);
UIE_GetSkirmishCvar(NULL, "uie_pgbouncemodifier",s_scriptdata.server.g_pgbouncemodifier, 32);
UIE_GetSkirmishCvar(NULL, "uie_bfgbouncemodifier",s_scriptdata.server.g_bfgbouncemodifier, 32);
UIE_GetSkirmishCvar(NULL, "uie_ngbouncemodifier",s_scriptdata.server.g_ngbouncemodifier, 32);
UIE_GetSkirmishCvar(NULL, "uie_ftbouncemodifier",s_scriptdata.server.g_ftbouncemodifier, 32);
UIE_GetSkirmishCvar(NULL, "uie_ambouncemodifier",s_scriptdata.server.g_ambouncemodifier, 32);
UIE_GetSkirmishCvar(NULL, "uie_gknockback",s_scriptdata.server.g_gknockback, 32);
UIE_GetSkirmishCvar(NULL, "uie_mgknockback",s_scriptdata.server.g_mgknockback, 32);
UIE_GetSkirmishCvar(NULL, "uie_sgknockback",s_scriptdata.server.g_sgknockback, 32);
UIE_GetSkirmishCvar(NULL, "uie_glknockback",s_scriptdata.server.g_glknockback, 32);
UIE_GetSkirmishCvar(NULL, "uie_rlknockback",s_scriptdata.server.g_rlknockback, 32);
UIE_GetSkirmishCvar(NULL, "uie_lgknockback",s_scriptdata.server.g_lgknockback, 32);
UIE_GetSkirmishCvar(NULL, "uie_rgknockback",s_scriptdata.server.g_rgknockback, 32);
UIE_GetSkirmishCvar(NULL, "uie_pgknockback",s_scriptdata.server.g_pgknockback, 32);
UIE_GetSkirmishCvar(NULL, "uie_bfgknockback",s_scriptdata.server.g_bfgknockback, 32);
UIE_GetSkirmishCvar(NULL, "uie_ngknockback",s_scriptdata.server.g_ngknockback, 32);
UIE_GetSkirmishCvar(NULL, "uie_plknockback",s_scriptdata.server.g_plknockback, 32);
UIE_GetSkirmishCvar(NULL, "uie_cgknockback",s_scriptdata.server.g_cgknockback, 32);
UIE_GetSkirmishCvar(NULL, "uie_ftknockback",s_scriptdata.server.g_ftknockback, 32);
UIE_GetSkirmishCvar(NULL, "uie_amknockback",s_scriptdata.server.g_amknockback, 32);

	//
	// gametype specific values
	//
	
	trap_Cvar_Set( "cl_selectedmod", s_scriptdata.server.selectedmod );

	gametype = s_scriptdata.gametype;
	s = gametype_cvar_base[ gametype ];

	// reset server text control
	UIE_GetSkirmishCvar( s, "hostname", s_scriptdata.server.hostname, MAX_HOSTNAME_LENGTH );

	//s_scriptdata.server.forceRespawn = UIE_GetSkirmishCvarIntClamp(0, 999, s, "ForceRespawn" );
	s_scriptdata.server.allowWarmup = UIE_GetSkirmishCvarIntClamp(0, 1, s, "doWarmUp" );
	s_scriptdata.server.warmupTime = UIE_GetSkirmishCvarIntClamp(0, 999, s, "WarmUp" );
	s_scriptdata.server.viewdistance = UIE_GetSkirmishCvarIntClamp(0, 90, s, "viewdistance" );

	if (gametype >= GT_TEAM && !(gametype == GT_LMS) ) {
		t = 0;
		if (gametype == GT_TEAM)
			t = "ui_team_friendly";
		else if (gametype == GT_CTF)
			t = "ui_ctf_friendly";

		if (t)
			s_scriptdata.server.friendlyFire = (int)Com_Clamp(0, 1, trap_Cvar_VariableValue(t));

		s_scriptdata.server.autoJoin = UIE_GetSkirmishCvarIntClamp(0, 1, s, "AutoJoin" );
		s_scriptdata.server.teamBalance = UIE_GetSkirmishCvarIntClamp(0, 1, s, "TeamBalance" );
	}
	else {
		s_scriptdata.server.autoJoin = 0;
		s_scriptdata.server.teamBalance = 0;
		s_scriptdata.server.friendlyFire = 0;
	}
}



/*
=================
StartServer_SaveServerScriptData

Saves server specific gametype data
=================
*/
static void StartServer_SaveServerScriptData(void)
{
	char* s;
	int friendly, gametype;

	UIE_SetSkirmishCvarInt( NULL, "uie_pure", s_scriptdata.server.pure);
	UIE_SetSkirmishCvarInt( NULL, "uie_dynamiclight", s_scriptdata.server.dynamiclight);

	UIE_SetSkirmishCvarInt( NULL, "uie_pmove_fixed", s_scriptdata.server.pmove_fixed);
	UIE_SetSkirmishCvarInt( NULL, "uie_pmove_msec", s_scriptdata.server.pmove_msec);
	UIE_SetSkirmishCvarInt( NULL, "uie_smoothclients", s_scriptdata.server.smoothclients);

	UIE_SetSkirmishCvarInt( NULL, "uie_syncclients", s_scriptdata.server.syncClients);

	UIE_SetSkirmishCvarInt( NULL, "uie_minping", s_scriptdata.server.minPing);
	UIE_SetSkirmishCvarInt( NULL, "uie_maxping", s_scriptdata.server.maxPing);
	UIE_SetSkirmishCvarInt( NULL, "uie_allowMinPing", s_scriptdata.server.allowMinPing);
	UIE_SetSkirmishCvarInt( NULL, "uie_allowMaxPing", s_scriptdata.server.allowMaxPing);

	UIE_SetSkirmishCvarInt( NULL, "uie_maxrate", s_scriptdata.server.maxrate);
	UIE_SetSkirmishCvarInt( NULL, "uie_allowmaxrate", s_scriptdata.server.allowmaxrate);
	UIE_SetSkirmishCvarInt( NULL, "uie_allowdownload", s_scriptdata.server.allowdownload);
	UIE_SetSkirmishCvarInt( NULL, "uie_allowvote", s_scriptdata.server.allowvote);
	UIE_SetSkirmishCvarInt( NULL, "uie_allowpass", s_scriptdata.server.allowpass);

	UIE_SetSkirmishCvarInt( NULL, "uie_netport", s_scriptdata.server.netport);
	UIE_SetSkirmishCvarInt( NULL, "uie_svfps", s_scriptdata.server.sv_fps);

	UIE_SetSkirmishCvar( NULL, "uie_password", s_scriptdata.server.password);

	UIE_SetSkirmishCvarInt( NULL, "uie_allowprivateclients", s_scriptdata.server.allowPrivateClients);
	UIE_SetSkirmishCvarInt( NULL, "uie_privateclients", s_scriptdata.server.privateClients);
	UIE_SetSkirmishCvar( NULL, "uie_privatepassword", s_scriptdata.server.privatePassword);

	UIE_SetSkirmishCvarInt( NULL, "uie_preventConfigBug", s_scriptdata.server.preventConfigBug);

	if (s_scriptdata.multiplayer) {
		UIE_SetSkirmishCvarInt( NULL, "uie_dedicated", s_scriptdata.server.dedicatedServer);
		UIE_SetSkirmishCvarInt( NULL, "uie_inactivity", s_scriptdata.server.inactivityTime);

		UIE_SetSkirmishCvarInt( NULL, "uie_strictAuth", s_scriptdata.server.strictAuth);
		UIE_SetSkirmishCvarInt( NULL, "uie_lanForceRate", s_scriptdata.server.lanForceRate);
	}
	
//Noire.dev cvars
UIE_SetSkirmishCvarInt( NULL, "uie_maxEntities", s_scriptdata.server.maxEntities);
UIE_SetSkirmishCvarInt( NULL, "uie_singleskill", s_scriptdata.server.singleskill);
UIE_SetSkirmishCvarInt( NULL, "uie_kill", s_scriptdata.server.kill);
UIE_SetSkirmishCvar( NULL, "uie_damageModifier", s_scriptdata.server.damageModifier);
UIE_SetSkirmishCvar( NULL, "uie_selectedmod", s_scriptdata.server.selectedmod);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination", s_scriptdata.server.elimination);
UIE_SetSkirmishCvarInt( NULL, "uie_obeliskHealth", s_scriptdata.server.obeliskHealth);
UIE_SetSkirmishCvarInt( NULL, "uie_obeliskRegenPeriod", s_scriptdata.server.obeliskRegenPeriod);
UIE_SetSkirmishCvarInt( NULL, "uie_obeliskRegenAmount", s_scriptdata.server.obeliskRegenAmount);
UIE_SetSkirmishCvarInt( NULL, "uie_obeliskRespawnDelay", s_scriptdata.server.obeliskRespawnDelay);
UIE_SetSkirmishCvarInt( NULL, "uie_cubeTimeout", s_scriptdata.server.cubeTimeout);
UIE_SetSkirmishCvarInt( NULL, "uie_flagrespawn", s_scriptdata.server.flagrespawn);
UIE_SetSkirmishCvarInt( NULL, "uie_weaponTeamRespawn", s_scriptdata.server.weaponTeamRespawn);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_ctf_oneway", s_scriptdata.server.elimination_ctf_oneway);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_selfdamage", s_scriptdata.server.elimination_selfdamage);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_roundtime", s_scriptdata.server.elimination_roundtime);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_warmup", s_scriptdata.server.elimination_warmup);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_activewarmup", s_scriptdata.server.elimination_activewarmup);
UIE_SetSkirmishCvarInt( NULL, "uie_lms_lives", s_scriptdata.server.lms_lives);
UIE_SetSkirmishCvarInt( NULL, "uie_lms_mode", s_scriptdata.server.lms_mode);
UIE_SetSkirmishCvarInt( NULL, "uie_accelerate", s_scriptdata.server.accelerate);
UIE_SetSkirmishCvarInt( NULL, "uie_spectatorspeed", s_scriptdata.server.spectatorspeed);
UIE_SetSkirmishCvarInt( NULL, "uie_speed", s_scriptdata.server.speed);
UIE_SetSkirmishCvarInt( NULL, "uie_gravity", s_scriptdata.server.gravity);
UIE_SetSkirmishCvar( NULL, "uie_gravityModifier", s_scriptdata.server.gravityModifier);
UIE_SetSkirmishCvarInt( NULL, "uie_knockback", s_scriptdata.server.knockback);
UIE_SetSkirmishCvarInt( NULL, "uie_noplayerclip", s_scriptdata.server.noplayerclip);
UIE_SetSkirmishCvarInt( NULL, "uie_jumpheight", s_scriptdata.server.jumpheight);
UIE_SetSkirmishCvarInt( NULL, "uie_regenarmor", s_scriptdata.server.regenarmor);
UIE_SetSkirmishCvarInt( NULL, "uie_ammolimit", s_scriptdata.server.ammolimit);
UIE_SetSkirmishCvar( NULL, "uie_quadfactor", s_scriptdata.server.quadfactor);
UIE_SetSkirmishCvarInt( NULL, "uie_respawntime", s_scriptdata.server.respawntime);
UIE_SetSkirmishCvarInt( NULL, "uie_forcerespawn", s_scriptdata.server.forcerespawn);
UIE_SetSkirmishCvar( NULL, "uie_vampire", s_scriptdata.server.vampire);
UIE_SetSkirmishCvarInt( NULL, "uie_vampire_max_health", s_scriptdata.server.vampire_max_health);
UIE_SetSkirmishCvarInt( NULL, "uie_regen", s_scriptdata.server.regen);
UIE_SetSkirmishCvarInt( NULL, "uie_maxweaponpickup", s_scriptdata.server.maxweaponpickup);
UIE_SetSkirmishCvarInt( NULL, "uie_droppeditemtime", s_scriptdata.server.droppeditemtime);
UIE_SetSkirmishCvarInt( NULL, "uie_autoflagreturn", s_scriptdata.server.autoflagreturn);
UIE_SetSkirmishCvar( NULL, "uie_armorprotect", s_scriptdata.server.armorprotect);
UIE_SetSkirmishCvarInt( NULL, "uie_respawnwait", s_scriptdata.server.respawnwait);
UIE_SetSkirmishCvar( NULL, "uie_speedfactor", s_scriptdata.server.speedfactor);
UIE_SetSkirmishCvar( NULL, "uie_scoutspeedfactor", s_scriptdata.server.scoutspeedfactor);
UIE_SetSkirmishCvar( NULL, "uie_scoutfirespeed", s_scriptdata.server.scoutfirespeed);
UIE_SetSkirmishCvar( NULL, "uie_scoutdamagefactor", s_scriptdata.server.scoutdamagefactor);
UIE_SetSkirmishCvar( NULL, "uie_scoutgravitymodifier", s_scriptdata.server.scoutgravitymodifier);
UIE_SetSkirmishCvarInt( NULL, "uie_scout_infammo", s_scriptdata.server.scout_infammo);
UIE_SetSkirmishCvar( NULL, "uie_scouthealthmodifier", s_scriptdata.server.scouthealthmodifier);
UIE_SetSkirmishCvar( NULL, "uie_doublerfirespeed", s_scriptdata.server.doublerfirespeed);
UIE_SetSkirmishCvar( NULL, "uie_doublerdamagefactor", s_scriptdata.server.doublerdamagefactor);
UIE_SetSkirmishCvar( NULL, "uie_doublerspeedfactor", s_scriptdata.server.doublerspeedfactor);
UIE_SetSkirmishCvar( NULL, "uie_doublergravitymodifier", s_scriptdata.server.doublergravitymodifier);
UIE_SetSkirmishCvarInt( NULL, "uie_doubler_infammo", s_scriptdata.server.doubler_infammo);
UIE_SetSkirmishCvar( NULL, "uie_doublerhealthmodifier", s_scriptdata.server.doublerhealthmodifier);
UIE_SetSkirmishCvar( NULL, "uie_guardhealthmodifier", s_scriptdata.server.guardhealthmodifier);
UIE_SetSkirmishCvar( NULL, "uie_guardfirespeed", s_scriptdata.server.guardfirespeed);
UIE_SetSkirmishCvar( NULL, "uie_guarddamagefactor", s_scriptdata.server.guarddamagefactor);
UIE_SetSkirmishCvar( NULL, "uie_guardspeedfactor", s_scriptdata.server.guardspeedfactor);
UIE_SetSkirmishCvar( NULL, "uie_guardgravitymodifier", s_scriptdata.server.guardgravitymodifier);
UIE_SetSkirmishCvarInt( NULL, "uie_guard_infammo", s_scriptdata.server.guard_infammo);
UIE_SetSkirmishCvar( NULL, "uie_ammoregenfirespeed", s_scriptdata.server.ammoregenfirespeed);
UIE_SetSkirmishCvarInt( NULL, "uie_ammoregen_infammo", s_scriptdata.server.ammoregen_infammo);
UIE_SetSkirmishCvar( NULL, "uie_ammoregendamagefactor", s_scriptdata.server.ammoregendamagefactor);
UIE_SetSkirmishCvar( NULL, "uie_ammoregenspeedfactor", s_scriptdata.server.ammoregenspeedfactor);
UIE_SetSkirmishCvar( NULL, "uie_ammoregengravitymodifier", s_scriptdata.server.ammoregengravitymodifier);
UIE_SetSkirmishCvar( NULL, "uie_ammoregenhealthmodifier", s_scriptdata.server.ammoregenhealthmodifier);
UIE_SetSkirmishCvarInt( NULL, "uie_teamred_speed", s_scriptdata.server.teamred_speed);
UIE_SetSkirmishCvar( NULL, "uie_teamred_gravityModifier", s_scriptdata.server.teamred_gravityModifier);
UIE_SetSkirmishCvar( NULL, "uie_teamred_firespeed", s_scriptdata.server.teamred_firespeed);
UIE_SetSkirmishCvar( NULL, "uie_teamred_damage", s_scriptdata.server.teamred_damage);
UIE_SetSkirmishCvarInt( NULL, "uie_teamred_infammo", s_scriptdata.server.teamred_infammo);
UIE_SetSkirmishCvarInt( NULL, "uie_teamred_respawnwait", s_scriptdata.server.teamred_respawnwait);
UIE_SetSkirmishCvarInt( NULL, "uie_teamred_pickupitems", s_scriptdata.server.teamred_pickupitems);
UIE_SetSkirmishCvarInt( NULL, "uie_eliminationredrespawn", s_scriptdata.server.eliminationredrespawn);
UIE_SetSkirmishCvarInt( NULL, "uie_eliminationred_startHealth", s_scriptdata.server.eliminationred_startHealth);
UIE_SetSkirmishCvarInt( NULL, "uie_eliminationred_startArmor", s_scriptdata.server.eliminationred_startArmor);
UIE_SetSkirmishCvarInt( NULL, "uie_teamblue_speed", s_scriptdata.server.teamblue_speed);
UIE_SetSkirmishCvar( NULL, "uie_teamblue_gravityModifier", s_scriptdata.server.teamblue_gravityModifier);
UIE_SetSkirmishCvar( NULL, "uie_teamblue_firespeed", s_scriptdata.server.teamblue_firespeed);
UIE_SetSkirmishCvar( NULL, "uie_teamblue_damage", s_scriptdata.server.teamblue_damage);
UIE_SetSkirmishCvarInt( NULL, "uie_teamblue_infammo", s_scriptdata.server.teamblue_infammo);
UIE_SetSkirmishCvarInt( NULL, "uie_teamblue_respawnwait", s_scriptdata.server.teamblue_respawnwait);
UIE_SetSkirmishCvarInt( NULL, "uie_teamblue_pickupitems", s_scriptdata.server.teamblue_pickupitems);
UIE_SetSkirmishCvarInt( NULL, "uie_eliminationrespawn", s_scriptdata.server.eliminationrespawn);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_startHealth", s_scriptdata.server.elimination_startHealth);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_startArmor", s_scriptdata.server.elimination_startArmor);
UIE_SetSkirmishCvarInt( NULL, "uie_eliminationred_grapple", s_scriptdata.server.eliminationred_grapple);
UIE_SetSkirmishCvarInt( NULL, "uie_eliminationred_gauntlet", s_scriptdata.server.eliminationred_gauntlet);
UIE_SetSkirmishCvarInt( NULL, "uie_eliminationred_machinegun", s_scriptdata.server.eliminationred_machinegun);
UIE_SetSkirmishCvarInt( NULL, "uie_eliminationred_shotgun", s_scriptdata.server.eliminationred_shotgun);
UIE_SetSkirmishCvarInt( NULL, "uie_eliminationred_grenade", s_scriptdata.server.eliminationred_grenade);
UIE_SetSkirmishCvarInt( NULL, "uie_eliminationred_rocket", s_scriptdata.server.eliminationred_rocket);
UIE_SetSkirmishCvarInt( NULL, "uie_eliminationred_railgun", s_scriptdata.server.eliminationred_railgun);
UIE_SetSkirmishCvarInt( NULL, "uie_eliminationred_lightning", s_scriptdata.server.eliminationred_lightning);
UIE_SetSkirmishCvarInt( NULL, "uie_eliminationred_plasmagun", s_scriptdata.server.eliminationred_plasmagun);
UIE_SetSkirmishCvarInt( NULL, "uie_eliminationred_bfg", s_scriptdata.server.eliminationred_bfg);
UIE_SetSkirmishCvarInt( NULL, "uie_eliminationred_chain", s_scriptdata.server.eliminationred_chain);
UIE_SetSkirmishCvarInt( NULL, "uie_eliminationred_mine", s_scriptdata.server.eliminationred_mine);
UIE_SetSkirmishCvarInt( NULL, "uie_eliminationred_nail", s_scriptdata.server.eliminationred_nail);
UIE_SetSkirmishCvarInt( NULL, "uie_eliminationred_flame", s_scriptdata.server.eliminationred_flame);
UIE_SetSkirmishCvarInt( NULL, "uie_eliminationred_antimatter", s_scriptdata.server.eliminationred_antimatter);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_grapple", s_scriptdata.server.elimination_grapple);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_gauntlet", s_scriptdata.server.elimination_gauntlet);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_machinegun", s_scriptdata.server.elimination_machinegun);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_shotgun", s_scriptdata.server.elimination_shotgun);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_grenade", s_scriptdata.server.elimination_grenade);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_rocket", s_scriptdata.server.elimination_rocket);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_railgun", s_scriptdata.server.elimination_railgun);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_lightning", s_scriptdata.server.elimination_lightning);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_plasmagun", s_scriptdata.server.elimination_plasmagun);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_bfg", s_scriptdata.server.elimination_bfg);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_chain", s_scriptdata.server.elimination_chain);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_mine", s_scriptdata.server.elimination_mine);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_nail", s_scriptdata.server.elimination_nail);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_flame", s_scriptdata.server.elimination_flame);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_antimatter", s_scriptdata.server.elimination_antimatter);
UIE_SetSkirmishCvarInt( NULL, "uie_eliminationred_quad", s_scriptdata.server.eliminationred_quad);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_haste", s_scriptdata.server.elimination_haste);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_bsuit", s_scriptdata.server.elimination_bsuit);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_invis", s_scriptdata.server.elimination_invis);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_regen", s_scriptdata.server.elimination_regen);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_flight", s_scriptdata.server.elimination_flight);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_items", s_scriptdata.server.elimination_items);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_holdable", s_scriptdata.server.elimination_holdable);
UIE_SetSkirmishCvarInt( NULL, "uie_minigame", s_scriptdata.server.minigame);
UIE_SetSkirmishCvarInt( NULL, "uie_overlay", s_scriptdata.server.overlay);
UIE_SetSkirmishCvarInt( NULL, "uie_randomItems", s_scriptdata.server.randomItems);
UIE_SetSkirmishCvarInt( NULL, "uie_slickmove", s_scriptdata.server.slickmove);
UIE_SetSkirmishCvarInt( NULL, "uie_spawnprotect", s_scriptdata.server.spawnprotect);
UIE_SetSkirmishCvarInt( NULL, "uie_elimination_lockspectator", s_scriptdata.server.elimination_lockspectator);
UIE_SetSkirmishCvarInt( NULL, "uie_awardpushing", s_scriptdata.server.awardpushing);
UIE_SetSkirmishCvarInt( NULL, "uie_randomteleport", s_scriptdata.server.randomteleport);
UIE_SetSkirmishCvarInt( NULL, "uie_falldamagesmall", s_scriptdata.server.falldamagesmall);
UIE_SetSkirmishCvarInt( NULL, "uie_falldamagebig", s_scriptdata.server.falldamagebig);
UIE_SetSkirmishCvarInt( NULL, "uie_waterdamage", s_scriptdata.server.waterdamage);
UIE_SetSkirmishCvarInt( NULL, "uie_lavadamage", s_scriptdata.server.lavadamage);
UIE_SetSkirmishCvarInt( NULL, "uie_slimedamage", s_scriptdata.server.slimedamage);
UIE_SetSkirmishCvarInt( NULL, "uie_drowndamage", s_scriptdata.server.drowndamage);
UIE_SetSkirmishCvarInt( NULL, "uie_invulinf", s_scriptdata.server.invulinf);
UIE_SetSkirmishCvarInt( NULL, "uie_invulmove", s_scriptdata.server.invulmove);
UIE_SetSkirmishCvarInt( NULL, "uie_invultime", s_scriptdata.server.invultime);
UIE_SetSkirmishCvarInt( NULL, "uie_kamikazeinf", s_scriptdata.server.kamikazeinf);
UIE_SetSkirmishCvarInt( NULL, "uie_portalinf", s_scriptdata.server.portalinf);
UIE_SetSkirmishCvarInt( NULL, "uie_portaltimeout", s_scriptdata.server.portaltimeout);
UIE_SetSkirmishCvarInt( NULL, "uie_portalhealth", s_scriptdata.server.portalhealth);
UIE_SetSkirmishCvarInt( NULL, "uie_teleporterinf", s_scriptdata.server.teleporterinf);
UIE_SetSkirmishCvarInt( NULL, "uie_medkitinf", s_scriptdata.server.medkitinf);
UIE_SetSkirmishCvarInt( NULL, "uie_medkitlimit", s_scriptdata.server.medkitlimit);
UIE_SetSkirmishCvarInt( NULL, "uie_medkitmodifier", s_scriptdata.server.medkitmodifier);
UIE_SetSkirmishCvarInt( NULL, "uie_fasthealthregen", s_scriptdata.server.fasthealthregen);
UIE_SetSkirmishCvarInt( NULL, "uie_slowhealthregen", s_scriptdata.server.slowhealthregen);
UIE_SetSkirmishCvar( NULL, "uie_hastefirespeed", s_scriptdata.server.hastefirespeed);
UIE_SetSkirmishCvarInt( NULL, "uie_quadtime", s_scriptdata.server.quadtime);
UIE_SetSkirmishCvarInt( NULL, "uie_bsuittime", s_scriptdata.server.bsuittime);
UIE_SetSkirmishCvarInt( NULL, "uie_hastetime", s_scriptdata.server.hastetime);
UIE_SetSkirmishCvarInt( NULL, "uie_invistime", s_scriptdata.server.invistime);
UIE_SetSkirmishCvarInt( NULL, "uie_regentime", s_scriptdata.server.regentime);
UIE_SetSkirmishCvarInt( NULL, "uie_flighttime", s_scriptdata.server.flighttime);
UIE_SetSkirmishCvarInt( NULL, "uie_armorrespawn", s_scriptdata.server.armorrespawn);
UIE_SetSkirmishCvarInt( NULL, "uie_healthrespawn", s_scriptdata.server.healthrespawn);
UIE_SetSkirmishCvarInt( NULL, "uie_ammorespawn", s_scriptdata.server.ammorespawn);
UIE_SetSkirmishCvarInt( NULL, "uie_holdablerespawn", s_scriptdata.server.holdablerespawn);
UIE_SetSkirmishCvarInt( NULL, "uie_megahealthrespawn", s_scriptdata.server.megahealthrespawn);
UIE_SetSkirmishCvarInt( NULL, "uie_poweruprespawn", s_scriptdata.server.poweruprespawn);
UIE_SetSkirmishCvarInt( NULL, "uie_weaponrespawn", s_scriptdata.server.weaponrespawn);
// Крюк
UIE_SetSkirmishCvarInt( NULL, "uie_ghspeed", s_scriptdata.server.g_ghspeed);
UIE_SetSkirmishCvarInt( NULL, "uie_ghtimeout", s_scriptdata.server.g_ghtimeout);
// Пила
UIE_SetSkirmishCvarInt( NULL, "uie_gdelay", s_scriptdata.server.g_gdelay);
UIE_SetSkirmishCvarInt( NULL, "uie_gdamage", s_scriptdata.server.g_gdamage);
UIE_SetSkirmishCvarInt( NULL, "uie_grange", s_scriptdata.server.g_grange);
UIE_SetSkirmishCvar( NULL, "uie_gknockback", s_scriptdata.server.g_gknockback);
// Автомат
UIE_SetSkirmishCvarInt( NULL, "uie_mgammocount", s_scriptdata.server.g_mgammocount);
UIE_SetSkirmishCvarInt( NULL, "uie_mgweaponcount", s_scriptdata.server.g_mgweaponcount);
UIE_SetSkirmishCvarInt( NULL, "uie_mgdelay", s_scriptdata.server.g_mgdelay);
UIE_SetSkirmishCvarInt( NULL, "uie_mgdamage", s_scriptdata.server.g_mgdamage);
UIE_SetSkirmishCvarInt( NULL, "uie_mgspread", s_scriptdata.server.g_mgspread);
UIE_SetSkirmishCvarInt( NULL, "uie_mgexplode", s_scriptdata.server.g_mgexplode);
UIE_SetSkirmishCvarInt( NULL, "uie_mgsdamage", s_scriptdata.server.g_mgsdamage);
UIE_SetSkirmishCvarInt( NULL, "uie_mgsradius", s_scriptdata.server.g_mgsradius);
UIE_SetSkirmishCvarInt( NULL, "uie_mgvampire", s_scriptdata.server.g_mgvampire);
UIE_SetSkirmishCvarInt( NULL, "uie_mginf", s_scriptdata.server.g_mginf);
UIE_SetSkirmishCvar( NULL, "uie_mgknockback", s_scriptdata.server.g_mgknockback);
// Дробовик
UIE_SetSkirmishCvarInt( NULL, "uie_sgammocount", s_scriptdata.server.g_sgammocount);
UIE_SetSkirmishCvarInt( NULL, "uie_sgweaponcount", s_scriptdata.server.g_sgweaponcount);
UIE_SetSkirmishCvarInt( NULL, "uie_sgdelay", s_scriptdata.server.g_sgdelay);
UIE_SetSkirmishCvarInt( NULL, "uie_sgdamage", s_scriptdata.server.g_sgdamage);
UIE_SetSkirmishCvarInt( NULL, "uie_sgspread", s_scriptdata.server.g_sgspread);
UIE_SetSkirmishCvarInt( NULL, "uie_sgexplode", s_scriptdata.server.g_sgexplode);
UIE_SetSkirmishCvarInt( NULL, "uie_sgsdamage", s_scriptdata.server.g_sgsdamage);
UIE_SetSkirmishCvarInt( NULL, "uie_sgsradius", s_scriptdata.server.g_sgsradius);
UIE_SetSkirmishCvarInt( NULL, "uie_sgcount", s_scriptdata.server.g_sgcount);
UIE_SetSkirmishCvarInt( NULL, "uie_sgvampire", s_scriptdata.server.g_sgvampire);
UIE_SetSkirmishCvarInt( NULL, "uie_sginf", s_scriptdata.server.g_sginf);
UIE_SetSkirmishCvar( NULL, "uie_sgknockback", s_scriptdata.server.g_sgknockback);
// Гранаты
UIE_SetSkirmishCvarInt( NULL, "uie_glammocount", s_scriptdata.server.g_glammocount);
UIE_SetSkirmishCvarInt( NULL, "uie_glweaponcount", s_scriptdata.server.g_glweaponcount);
UIE_SetSkirmishCvarInt( NULL, "uie_gldelay", s_scriptdata.server.g_gldelay);
UIE_SetSkirmishCvarInt( NULL, "uie_glspeed", s_scriptdata.server.g_glspeed);
UIE_SetSkirmishCvarInt( NULL, "uie_glbounce", s_scriptdata.server.g_glbounce);
UIE_SetSkirmishCvarInt( NULL, "uie_glgravity", s_scriptdata.server.g_glgravity);
UIE_SetSkirmishCvarInt( NULL, "uie_gltimeout", s_scriptdata.server.g_gltimeout);
UIE_SetSkirmishCvarInt( NULL, "uie_glsradius", s_scriptdata.server.g_glsradius);
UIE_SetSkirmishCvarInt( NULL, "uie_glsdamage", s_scriptdata.server.g_glsdamage);
UIE_SetSkirmishCvarInt( NULL, "uie_gldamage", s_scriptdata.server.g_gldamage);
UIE_SetSkirmishCvarInt( NULL, "uie_glvampire", s_scriptdata.server.g_glvampire);
UIE_SetSkirmishCvarInt( NULL, "uie_glinf", s_scriptdata.server.g_glinf);
UIE_SetSkirmishCvar( NULL, "uie_glbouncemodifier", s_scriptdata.server.g_glbouncemodifier);
UIE_SetSkirmishCvar( NULL, "uie_glknockback", s_scriptdata.server.g_glknockback);
UIE_SetSkirmishCvarInt( NULL, "uie_glhoming", s_scriptdata.server.g_glhoming);
UIE_SetSkirmishCvarInt( NULL, "uie_glguided", s_scriptdata.server.g_glguided);
// Ракеты
UIE_SetSkirmishCvarInt( NULL, "uie_rlammocount", s_scriptdata.server.g_rlammocount);
UIE_SetSkirmishCvarInt( NULL, "uie_rlweaponcount", s_scriptdata.server.g_rlweaponcount);
UIE_SetSkirmishCvarInt( NULL, "uie_rldelay", s_scriptdata.server.g_rldelay);
UIE_SetSkirmishCvarInt( NULL, "uie_rlspeed", s_scriptdata.server.g_rlspeed);
UIE_SetSkirmishCvarInt( NULL, "uie_rlbounce", s_scriptdata.server.g_rlbounce);
UIE_SetSkirmishCvarInt( NULL, "uie_rlgravity", s_scriptdata.server.g_rlgravity);
UIE_SetSkirmishCvarInt( NULL, "uie_rltimeout", s_scriptdata.server.g_rltimeout);
UIE_SetSkirmishCvarInt( NULL, "uie_rlsradius", s_scriptdata.server.g_rlsradius);
UIE_SetSkirmishCvarInt( NULL, "uie_rlsdamage", s_scriptdata.server.g_rlsdamage);
UIE_SetSkirmishCvarInt( NULL, "uie_rldamage", s_scriptdata.server.g_rldamage);
UIE_SetSkirmishCvarInt( NULL, "uie_rlvampire", s_scriptdata.server.g_rlvampire);
UIE_SetSkirmishCvarInt( NULL, "uie_rlinf", s_scriptdata.server.g_rlinf);
UIE_SetSkirmishCvar( NULL, "uie_rlbouncemodifier", s_scriptdata.server.g_rlbouncemodifier);
UIE_SetSkirmishCvar( NULL, "uie_rlknockback", s_scriptdata.server.g_rlknockback);
UIE_SetSkirmishCvarInt( NULL, "uie_rlhoming", s_scriptdata.server.g_rlhoming);
UIE_SetSkirmishCvarInt( NULL, "uie_rlguided", s_scriptdata.server.g_rlguided);
// Молния
UIE_SetSkirmishCvarInt( NULL, "uie_lgammocount", s_scriptdata.server.g_lgammocount);
UIE_SetSkirmishCvarInt( NULL, "uie_lgweaponcount", s_scriptdata.server.g_lgweaponcount);
UIE_SetSkirmishCvarInt( NULL, "uie_lgrange", s_scriptdata.server.g_lgrange);
UIE_SetSkirmishCvarInt( NULL, "uie_lgdelay", s_scriptdata.server.g_lgdelay);
UIE_SetSkirmishCvarInt( NULL, "uie_lgdamage", s_scriptdata.server.g_lgdamage);
UIE_SetSkirmishCvarInt( NULL, "uie_lgvampire", s_scriptdata.server.g_lgvampire);
UIE_SetSkirmishCvarInt( NULL, "uie_lgexplode", s_scriptdata.server.g_lgexplode);
UIE_SetSkirmishCvarInt( NULL, "uie_lgsdamage", s_scriptdata.server.g_lgsdamage);
UIE_SetSkirmishCvarInt( NULL, "uie_lgsradius", s_scriptdata.server.g_lgsradius);
UIE_SetSkirmishCvarInt( NULL, "uie_lginf", s_scriptdata.server.g_lginf);
UIE_SetSkirmishCvar( NULL, "uie_lgknockback", s_scriptdata.server.g_lgknockback);
// Рэйлган
UIE_SetSkirmishCvarInt( NULL, "uie_rgammocount", s_scriptdata.server.g_rgammocount);
UIE_SetSkirmishCvarInt( NULL, "uie_rgweaponcount", s_scriptdata.server.g_rgweaponcount);
UIE_SetSkirmishCvarInt( NULL, "uie_rgdelay", s_scriptdata.server.g_rgdelay);
UIE_SetSkirmishCvarInt( NULL, "uie_rgdamage", s_scriptdata.server.g_rgdamage);
UIE_SetSkirmishCvarInt( NULL, "uie_rgvampire", s_scriptdata.server.g_rgvampire);
UIE_SetSkirmishCvarInt( NULL, "uie_rginf", s_scriptdata.server.g_rginf);
UIE_SetSkirmishCvar( NULL, "uie_rgknockback", s_scriptdata.server.g_rgknockback);
// Плазмаган
UIE_SetSkirmishCvarInt( NULL, "uie_pgammocount", s_scriptdata.server.g_pgammocount);
UIE_SetSkirmishCvarInt( NULL, "uie_pgweaponcount", s_scriptdata.server.g_pgweaponcount);
UIE_SetSkirmishCvarInt( NULL, "uie_pgdelay", s_scriptdata.server.g_pgdelay);
UIE_SetSkirmishCvarInt( NULL, "uie_pgspeed", s_scriptdata.server.g_pgspeed);
UIE_SetSkirmishCvarInt( NULL, "uie_pgbounce", s_scriptdata.server.g_pgbounce);
UIE_SetSkirmishCvarInt( NULL, "uie_pggravity", s_scriptdata.server.g_pggravity);
UIE_SetSkirmishCvarInt( NULL, "uie_pgtimeout", s_scriptdata.server.g_pgtimeout);
UIE_SetSkirmishCvarInt( NULL, "uie_pgsradius", s_scriptdata.server.g_pgsradius);
UIE_SetSkirmishCvarInt( NULL, "uie_pgsdamage", s_scriptdata.server.g_pgsdamage);
UIE_SetSkirmishCvarInt( NULL, "uie_pgdamage", s_scriptdata.server.g_pgdamage);
UIE_SetSkirmishCvarInt( NULL, "uie_pgvampire", s_scriptdata.server.g_pgvampire);
UIE_SetSkirmishCvarInt( NULL, "uie_pginf", s_scriptdata.server.g_pginf);
UIE_SetSkirmishCvar( NULL, "uie_pgbouncemodifier", s_scriptdata.server.g_pgbouncemodifier);
UIE_SetSkirmishCvar( NULL, "uie_pgknockback", s_scriptdata.server.g_pgknockback);
UIE_SetSkirmishCvarInt( NULL, "uie_pghoming", s_scriptdata.server.g_pghoming);
UIE_SetSkirmishCvarInt( NULL, "uie_pgguided", s_scriptdata.server.g_pgguided);
// Бфг
UIE_SetSkirmishCvarInt( NULL, "uie_bfgammocount", s_scriptdata.server.g_bfgammocount);
UIE_SetSkirmishCvarInt( NULL, "uie_bfgweaponcount", s_scriptdata.server.g_bfgweaponcount);
UIE_SetSkirmishCvarInt( NULL, "uie_bfgdelay", s_scriptdata.server.g_bfgdelay);
UIE_SetSkirmishCvarInt( NULL, "uie_bfgspeed", s_scriptdata.server.g_bfgspeed);
UIE_SetSkirmishCvarInt( NULL, "uie_bfgbounce", s_scriptdata.server.g_bfgbounce);
UIE_SetSkirmishCvarInt( NULL, "uie_bfggravity", s_scriptdata.server.g_bfggravity);
UIE_SetSkirmishCvarInt( NULL, "uie_bfgtimeout", s_scriptdata.server.g_bfgtimeout);
UIE_SetSkirmishCvarInt( NULL, "uie_bfgsradius", s_scriptdata.server.g_bfgsradius);
UIE_SetSkirmishCvarInt( NULL, "uie_bfgsdamage", s_scriptdata.server.g_bfgsdamage);
UIE_SetSkirmishCvarInt( NULL, "uie_bfgdamage", s_scriptdata.server.g_bfgdamage);
UIE_SetSkirmishCvarInt( NULL, "uie_bfgvampire", s_scriptdata.server.g_bfgvampire);
UIE_SetSkirmishCvarInt( NULL, "uie_bfginf", s_scriptdata.server.g_bfginf);
UIE_SetSkirmishCvar( NULL, "uie_bfgbouncemodifier", s_scriptdata.server.g_bfgbouncemodifier);
UIE_SetSkirmishCvar( NULL, "uie_bfgknockback", s_scriptdata.server.g_bfgknockback);
UIE_SetSkirmishCvarInt( NULL, "uie_bfghoming", s_scriptdata.server.g_bfghoming);
UIE_SetSkirmishCvarInt( NULL, "uie_bfgguided", s_scriptdata.server.g_bfgguided);
// Гвоздомёт
UIE_SetSkirmishCvarInt( NULL, "uie_ngammocount", s_scriptdata.server.g_ngammocount);
UIE_SetSkirmishCvarInt( NULL, "uie_ngweaponcount", s_scriptdata.server.g_ngweaponcount);
UIE_SetSkirmishCvarInt( NULL, "uie_ngdelay", s_scriptdata.server.g_ngdelay);
UIE_SetSkirmishCvarInt( NULL, "uie_ngspeed", s_scriptdata.server.g_ngspeed);
UIE_SetSkirmishCvarInt( NULL, "uie_ngbounce", s_scriptdata.server.g_ngbounce);
UIE_SetSkirmishCvarInt( NULL, "uie_nggravity", s_scriptdata.server.g_nggravity);
UIE_SetSkirmishCvarInt( NULL, "uie_ngtimeout", s_scriptdata.server.g_ngtimeout);
UIE_SetSkirmishCvarInt( NULL, "uie_ngcount", s_scriptdata.server.g_ngcount);
UIE_SetSkirmishCvarInt( NULL, "uie_ngspread", s_scriptdata.server.g_ngspread);
UIE_SetSkirmishCvarInt( NULL, "uie_ngdamage", s_scriptdata.server.g_ngdamage);
UIE_SetSkirmishCvarInt( NULL, "uie_ngrandom", s_scriptdata.server.g_ngrandom);
UIE_SetSkirmishCvarInt( NULL, "uie_ngvampire", s_scriptdata.server.g_ngvampire);
UIE_SetSkirmishCvarInt( NULL, "uie_nginf", s_scriptdata.server.g_nginf);
UIE_SetSkirmishCvar( NULL, "uie_ngbouncemodifier", s_scriptdata.server.g_ngbouncemodifier);
UIE_SetSkirmishCvar( NULL, "uie_ngknockback", s_scriptdata.server.g_ngknockback);
UIE_SetSkirmishCvarInt( NULL, "uie_nghoming", s_scriptdata.server.g_nghoming);
UIE_SetSkirmishCvarInt( NULL, "uie_ngguided", s_scriptdata.server.g_ngguided);
// Мины
UIE_SetSkirmishCvarInt( NULL, "uie_plammocount", s_scriptdata.server.g_plammocount);
UIE_SetSkirmishCvarInt( NULL, "uie_plweaponcount", s_scriptdata.server.g_plweaponcount);
UIE_SetSkirmishCvarInt( NULL, "uie_pldelay", s_scriptdata.server.g_pldelay);
UIE_SetSkirmishCvarInt( NULL, "uie_plspeed", s_scriptdata.server.g_plspeed);
UIE_SetSkirmishCvarInt( NULL, "uie_plgravity", s_scriptdata.server.g_plgravity);
UIE_SetSkirmishCvarInt( NULL, "uie_pltimeout", s_scriptdata.server.g_pltimeout);
UIE_SetSkirmishCvarInt( NULL, "uie_plsradius", s_scriptdata.server.g_plsradius);
UIE_SetSkirmishCvarInt( NULL, "uie_plsdamage", s_scriptdata.server.g_plsdamage);
UIE_SetSkirmishCvarInt( NULL, "uie_pldamage", s_scriptdata.server.g_pldamage);
UIE_SetSkirmishCvarInt( NULL, "uie_plvampire", s_scriptdata.server.g_plvampire);
UIE_SetSkirmishCvarInt( NULL, "uie_plinf", s_scriptdata.server.g_plinf);
UIE_SetSkirmishCvar( NULL, "uie_plknockback", s_scriptdata.server.g_plknockback);
// Пулемёт
UIE_SetSkirmishCvarInt( NULL, "uie_cgammocount", s_scriptdata.server.g_cgammocount);
UIE_SetSkirmishCvarInt( NULL, "uie_cgweaponcount", s_scriptdata.server.g_cgweaponcount);
UIE_SetSkirmishCvarInt( NULL, "uie_cgdelay", s_scriptdata.server.g_cgdelay);
UIE_SetSkirmishCvarInt( NULL, "uie_cgspread", s_scriptdata.server.g_cgspread);
UIE_SetSkirmishCvarInt( NULL, "uie_cgdamage", s_scriptdata.server.g_cgdamage);
UIE_SetSkirmishCvarInt( NULL, "uie_cgvampire", s_scriptdata.server.g_cgvampire);
UIE_SetSkirmishCvarInt( NULL, "uie_cginf", s_scriptdata.server.g_cginf);
UIE_SetSkirmishCvar( NULL, "uie_cgknockback", s_scriptdata.server.g_cgknockback);
// Огнемёт
UIE_SetSkirmishCvarInt( NULL, "uie_ftammocount", s_scriptdata.server.g_ftammocount);
UIE_SetSkirmishCvarInt( NULL, "uie_ftweaponcount", s_scriptdata.server.g_ftweaponcount);
UIE_SetSkirmishCvarInt( NULL, "uie_ftdelay", s_scriptdata.server.g_ftdelay);
UIE_SetSkirmishCvarInt( NULL, "uie_ftspeed", s_scriptdata.server.g_ftspeed);
UIE_SetSkirmishCvarInt( NULL, "uie_ftbounce", s_scriptdata.server.g_ftbounce);
UIE_SetSkirmishCvarInt( NULL, "uie_ftgravity", s_scriptdata.server.g_ftgravity);
UIE_SetSkirmishCvarInt( NULL, "uie_fttimeout", s_scriptdata.server.g_fttimeout);
UIE_SetSkirmishCvarInt( NULL, "uie_ftsradius", s_scriptdata.server.g_ftsradius);
UIE_SetSkirmishCvarInt( NULL, "uie_ftsdamage", s_scriptdata.server.g_ftsdamage);
UIE_SetSkirmishCvarInt( NULL, "uie_ftdamage", s_scriptdata.server.g_ftdamage);
UIE_SetSkirmishCvarInt( NULL, "uie_ftvampire", s_scriptdata.server.g_ftvampire);
UIE_SetSkirmishCvarInt( NULL, "uie_ftinf", s_scriptdata.server.g_ftinf);
UIE_SetSkirmishCvar( NULL, "uie_ftbouncemodifier", s_scriptdata.server.g_ftbouncemodifier);
UIE_SetSkirmishCvar( NULL, "uie_ftknockback", s_scriptdata.server.g_ftknockback);
UIE_SetSkirmishCvarInt( NULL, "uie_fthoming", s_scriptdata.server.g_fthoming);
UIE_SetSkirmishCvarInt( NULL, "uie_ftguided", s_scriptdata.server.g_ftguided);
// Антиматтер
UIE_SetSkirmishCvarInt( NULL, "uie_amweaponcount", s_scriptdata.server.g_amweaponcount);
UIE_SetSkirmishCvarInt( NULL, "uie_amdelay", s_scriptdata.server.g_amdelay);
UIE_SetSkirmishCvarInt( NULL, "uie_amspeed", s_scriptdata.server.g_amspeed);
UIE_SetSkirmishCvarInt( NULL, "uie_ambounce", s_scriptdata.server.g_ambounce);
UIE_SetSkirmishCvarInt( NULL, "uie_amgravity", s_scriptdata.server.g_amgravity);
UIE_SetSkirmishCvarInt( NULL, "uie_amtimeout", s_scriptdata.server.g_amtimeout);
UIE_SetSkirmishCvarInt( NULL, "uie_amsradius", s_scriptdata.server.g_amsradius);
UIE_SetSkirmishCvarInt( NULL, "uie_amsdamage", s_scriptdata.server.g_amsdamage);
UIE_SetSkirmishCvarInt( NULL, "uie_amdamage", s_scriptdata.server.g_amdamage);
UIE_SetSkirmishCvarInt( NULL, "uie_amvampire", s_scriptdata.server.g_amvampire);
UIE_SetSkirmishCvarInt( NULL, "uie_aminf", s_scriptdata.server.g_aminf);
UIE_SetSkirmishCvar( NULL, "uie_ambouncemodifier", s_scriptdata.server.g_ambouncemodifier);
UIE_SetSkirmishCvar( NULL, "uie_amknockback", s_scriptdata.server.g_amknockback);
UIE_SetSkirmishCvarInt( NULL, "uie_amhoming", s_scriptdata.server.g_amhoming);
UIE_SetSkirmishCvarInt( NULL, "uie_amguided", s_scriptdata.server.g_amguided);

	//
	// save gametype specific data
	//

	gametype = s_scriptdata.gametype;
	s = gametype_cvar_base[ gametype ];

	//
	// save state values
	//

	UIE_SetSkirmishCvar(s, "hostname", s_scriptdata.server.hostname);
	//UIE_SetSkirmishCvarInt(s, "ForceRespawn", s_scriptdata.server.forceRespawn);

	UIE_SetSkirmishCvarInt(s, "doWarmup", s_scriptdata.server.allowWarmup);
	UIE_SetSkirmishCvarInt(s, "WarmUp", s_scriptdata.server.warmupTime);
	UIE_SetSkirmishCvarInt(s, "viewdistance", s_scriptdata.server.viewdistance);

	if (gametype >= GT_TEAM && !(gametype == GT_LMS) ) {
		// ff is an existing cvar, so we use the existing cvar 
		friendly = s_scriptdata.server.friendlyFire;
		if (gametype == GT_TEAM) {
			trap_Cvar_SetValue( "ui_team_friendly", friendly );
		}
		else if (gametype == GT_CTF) {
			trap_Cvar_SetValue( "ui_ctf_friendly", friendly );
		}

		UIE_SetSkirmishCvarInt(s, "AutoJoin", s_scriptdata.server.autoJoin);
		UIE_SetSkirmishCvarInt(s, "TeamBalance", s_scriptdata.server.teamBalance);
	}
}



/*
==========================================================================

INIT SCRIPT DATA

==========================================================================
*/



/*
=================
StartServer_LoadScriptDataFromType

Loads script data for the give game type
=================
*/
void StartServer_LoadScriptDataFromType(int gametype)
{
	s_scriptdata.gametype = gametype;

	StartServer_LoadMapScriptData();
	StartServer_LoadBotScriptData();
	StartServer_LoadItemScriptData();
	StartServer_LoadServerScriptData();
}




/*
=================
StartServer_InitScriptData

Loads all script data
=================
*/
void StartServer_InitScriptData(qboolean multiplayer)
{
	memset(&s_scriptdata, 0, sizeof(scriptdata_t));

	UIE_StartServer_LoadSkirmishCvars();

	s_scriptdata.multiplayer = multiplayer;
	StartServer_LoadScriptDataFromType((int)Com_Clamp( 0, MAX_GAME_TYPE, UIE_GetSkirmishCvarInt( NULL, "uie_gametype" ) ));
	StartServer_BuildMapDistribution();
}





/*
=================
StartServer_SaveScriptData

Saves all script data
=================
*/
void StartServer_SaveScriptData(void)
{
	UIE_SetSkirmishCvarInt( NULL, "uie_gametype", s_scriptdata.gametype );

	StartServer_SaveMapScriptData();
	StartServer_SaveBotScriptData();
	StartServer_SaveItemScriptData();
	StartServer_SaveServerScriptData();

	UIE_StartServer_SaveSkirmishCvars();
}




