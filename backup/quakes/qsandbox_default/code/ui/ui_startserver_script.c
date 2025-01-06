/*
=============================================================================

START SERVER SCRIPT ENGINE *****

=============================================================================
*/



#include "ui_local.h"

#include "ui_startserver.h"

#define BEGIN_PARAMETER_BLOCK 	"BEGIN_UIE_PARAMETER_BLOCK"
#define END_PARAMETER_BLOCK 	"END_UIE_PARAMETER_BLOCK"

#define CVAR_BUFFER 	512



// game parameters we need to save in a block
// for later recovery. Strings begining with a
// * are gametype specific

// uie_dedicated is deliberately missing to allow
// script loading in both Skirmish and Multiplayer
// versions of the browser

static const char* saveparam_list[] = {
"uie_gametype", "uie_pure", "uie_inactivity", "uie_allowmaxrate",
"uie_maxrate", "uie_allowpass","uie_password", "uie_allowvote",
"uie_allowdownload", "uie_smoothclients", "uie_pmove_fixed",
"uie_pmove_msec", "uie_syncclients", "uie_minPing", "uie_maxPing",
"uie_allowMinPing", "uie_allowMaxPing", "uie_gravity", "uie_jumpheight", "uie_knockback",
"uie_quadfactor", "uie_netport", "uie_svfps", "uie_allowprivateclients",
"uie_privateclients", "uie_privatepassword", "uie_strictAuth",
"uie_lanForceRate", 
"uie_maxEntities",
"uie_singleskill",
"uie_kill",
"uie_damageModifier",
"uie_elimination",
"uie_obeliskHealth",
"uie_obeliskRegenPeriod",
"uie_obeliskRegenAmount",
"uie_obeliskRespawnDelay",
"uie_cubeTimeout",
"uie_flagrespawn",
"uie_weaponTeamRespawn",
"uie_elimination_ctf_oneway",
"uie_elimination_selfdamage",
"uie_elimination_roundtime",
"uie_elimination_warmup",
"uie_elimination_activewarmup",
"uie_lms_lives",
"uie_lms_mode",
"uie_accelerate",
"uie_spectatorspeed",
"uie_speed",
"uie_gravity",
"uie_gravityModifier",
"uie_knockback",
"uie_noplayerclip",
"uie_jumpheight",
"uie_regenarmor",
"uie_ammolimit",
"uie_quadfactor",
"uie_respawntime",
"uie_forcerespawn",
"uie_vampire",
"uie_vampire_max_health",
"uie_regen",
"uie_maxweaponpickup",
"uie_droppeditemtime",
"uie_autoflagreturn",
"uie_armorprotect",
"uie_respawnwait",
"uie_speedfactor",
"uie_scoutspeedfactor",
"uie_scoutfirespeed",
"uie_scoutdamagefactor",
"uie_scoutgravitymodifier",
"uie_scout_infammo",
"uie_scouthealthmodifier",
"uie_doublerfirespeed",
"uie_doublerdamagefactor",
"uie_doublerspeedfactor",
"uie_doublergravitymodifier",
"uie_doubler_infammo",
"uie_doublerhealthmodifier",
"uie_guardhealthmodifier",
"uie_guardfirespeed",
"uie_guarddamagefactor",
"uie_guardspeedfactor",
"uie_guardgravitymodifier",
"uie_guard_infammo",
"uie_ammoregenfirespeed",
"uie_ammoregen_infammo",
"uie_ammoregendamagefactor",
"uie_ammoregenspeedfactor",
"uie_ammoregengravitymodifier",
"uie_ammoregenhealthmodifier",
"uie_teamred_speed",
"uie_teamred_gravityModifier",
"uie_teamred_firespeed",
"uie_teamred_damage",
"uie_teamred_infammo",
"uie_teamred_respawnwait",
"uie_teamred_pickupitems",
"uie_eliminationredrespawn",
"uie_eliminationred_startHealth",
"uie_eliminationred_startArmor",
"uie_teamblue_speed",
"uie_teamblue_gravityModifier",
"uie_teamblue_firespeed",
"uie_teamblue_damage",
"uie_teamblue_infammo",
"uie_teamblue_respawnwait",
"uie_teamblue_pickupitems",
"uie_eliminationrespawn",
"uie_elimination_startHealth",
"uie_elimination_startArmor",
"uie_eliminationred_grapple",
"uie_eliminationred_gauntlet",
"uie_eliminationred_machinegun",
"uie_eliminationred_shotgun",
"uie_eliminationred_grenade",
"uie_eliminationred_rocket",
"uie_eliminationred_railgun",
"uie_eliminationred_lightning",
"uie_eliminationred_plasmagun",
"uie_eliminationred_bfg",
"uie_eliminationred_chain",
"uie_eliminationred_mine",
"uie_eliminationred_nail",
"uie_eliminationred_flame",
"uie_eliminationred_antimatter",
"uie_elimination_grapple",
"uie_elimination_gauntlet",
"uie_elimination_machinegun",
"uie_elimination_shotgun",
"uie_elimination_grenade",
"uie_elimination_rocket",
"uie_elimination_railgun",
"uie_elimination_lightning",
"uie_elimination_plasmagun",
"uie_elimination_bfg",
"uie_elimination_chain",
"uie_elimination_mine",
"uie_elimination_nail",
"uie_elimination_flame",
"uie_elimination_antimatter",
"uie_eliminationred_quad",
"uie_eliminationred_haste",
"uie_eliminationred_bsuit",
"uie_eliminationred_invis",
"uie_eliminationred_regen",
"uie_eliminationred_flight",
"uie_eliminationred_holdable",
"uie_elimination_quad",
"uie_elimination_haste",
"uie_elimination_bsuit",
"uie_elimination_invis",
"uie_elimination_regen",
"uie_elimination_flight",
"uie_elimination_items",
"uie_elimination_holdable",
"uie_minigame",
"uie_overlay",
"uie_randomItems",
"uie_slickmove",
"uie_spawnprotect",
"uie_elimination_lockspectator",
"uie_awardpushing",
"uie_randomteleport",
"uie_falldamagesmall",
"uie_falldamagebig",
"uie_waterdamage",
"uie_lavadamage",
"uie_slimedamage",
"uie_drowndamage",
"uie_invulinf",
"uie_invulmove",
"uie_invultime",
"uie_kamikazeinf",
"uie_portalinf",
"uie_portaltimeout",
"uie_portalhealth",
"uie_teleporterinf",
"uie_medkitinf",
"uie_medkitlimit",
"uie_medkitmodifier",
"uie_fasthealthregen",
"uie_slowhealthregen",
"uie_hastefirespeed",
"uie_quadtime",
"uie_bsuittime",
"uie_hastetime",
"uie_invistime",
"uie_regentime",
"uie_flighttime",
"uie_armorrespawn",
"uie_healthrespawn",
"uie_ammorespawn",
"uie_holdablerespawn",
"uie_megahealthrespawn",
"uie_poweruprespawn",
"uie_weaponrespawn",
"uie_selectedmod",

// Крюк
"uie_ghspeed",
"uie_ghtimeout",
// Пила
"uie_gdelay",
"uie_gdamage",
"uie_grange",
"uie_gknockback",
// Автомат
"uie_mgammocount",
"uie_mgweaponcount",
"uie_mgdelay",
"uie_mgdamage",
"uie_mgspread",
"uie_mgexplode",
"uie_mgsdamage",
"uie_mgsradius",
"uie_mgvampire",
"uie_mginf",
"uie_mgknockback",
// Дробовик
"uie_sgammocount",
"uie_sgweaponcount",
"uie_sgdelay",
"uie_sgdamage",
"uie_sgspread",
"uie_sgexplode",
"uie_sgsdamage",
"uie_sgsradius",
"uie_sgcount",
"uie_sgvampire",
"uie_sginf",
"uie_sgknockback",
// Гранаты
"uie_glammocount",
"uie_glweaponcount",
"uie_gldelay",
"uie_glspeed",
"uie_glbounce",
"uie_glgravity",
"uie_gltimeout",
"uie_glsradius",
"uie_glsdamage",
"uie_gldamage",
"uie_glvampire",
"uie_glinf",
"uie_glbouncemodifier",
"uie_glknockback",
"uie_glhoming",
"uie_glguided",
// Ракеты
"uie_rlammocount",
"uie_rlweaponcount",
"uie_rldelay",
"uie_rlspeed",
"uie_rlbounce",
"uie_rlgravity",
"uie_rltimeout",
"uie_rlsradius",
"uie_rlsdamage",
"uie_rldamage",
"uie_rlvampire",
"uie_rlinf",
"uie_rlbouncemodifier",
"uie_rlknockback",
"uie_rlhoming",
"uie_rlguided",
// Молния
"uie_lgammocount",
"uie_lgweaponcount",
"uie_lgrange",
"uie_lgdelay",
"uie_lgdamage",
"uie_lgvampire",
"uie_lgexplode",
"uie_lgsdamage",
"uie_lgsradius",
"uie_lginf",
"uie_lgknockback",
// Рэйлган
"uie_rgammocount",
"uie_rgweaponcount",
"uie_rgdelay",
"uie_rgdamage",
"uie_rgvampire",
"uie_rginf",
"uie_rgknockback",
// Плазмаган
"uie_pgammocount",
"uie_pgweaponcount",
"uie_pgdelay",
"uie_pgspeed",
"uie_pgbounce",
"uie_pggravity",
"uie_pgtimeout",
"uie_pgsradius",
"uie_pgsdamage",
"uie_pgdamage",
"uie_pgvampire",
"uie_pginf",
"uie_pgbouncemodifier",
"uie_pgknockback",
"uie_pghoming",
"uie_pgguided",
// Бфг
"uie_bfgammocount",
"uie_bfgweaponcount",
"uie_bfgdelay",
"uie_bfgspeed",
"uie_bfgbounce",
"uie_bfggravity",
"uie_bfgtimeout",
"uie_bfgsradius",
"uie_bfgsdamage",
"uie_bfgdamage",
"uie_bfgvampire",
"uie_bfginf",
"uie_bfgbouncemodifier",
"uie_bfgknockback",
"uie_bfghoming",
"uie_bfgguided",
// Гвоздомёт
"uie_ngammocount",
"uie_ngweaponcount",
"uie_ngdelay",
"uie_ngspeed",
"uie_ngbounce",
"uie_nggravity",
"uie_ngtimeout",
"uie_ngcount",
"uie_ngspread",
"uie_ngdamage",
"uie_ngrandom",
"uie_ngvampire",
"uie_nginf",
"uie_ngbouncemodifier",
"uie_ngknockback",
"uie_nghoming",
"uie_ngguided",
// Мины
"uie_plammocount",
"uie_plweaponcount",
"uie_pldelay",
"uie_plspeed",
"uie_plgravity",
"uie_pltimeout",
"uie_plsradius",
"uie_plsdamage",
"uie_pldamage",
"uie_plvampire",
"uie_plinf",
"uie_plknockback",
// Пулемёт
"uie_cgammocount",
"uie_cgweaponcount",
"uie_cgdelay",
"uie_cgspread",
"uie_cgdamage",
"uie_cgvampire",
"uie_cginf",
"uie_cgknockback",
// Огнемёт
"uie_ftammocount",
"uie_ftweaponcount",
"uie_ftdelay",
"uie_ftspeed",
"uie_ftbounce",
"uie_ftgravity",
"uie_fttimeout",
"uie_ftsradius",
"uie_ftsdamage",
"uie_ftdamage",
"uie_ftvampire",
"uie_ftinf",
"uie_ftbouncemodifier",
"uie_ftknockback",
"uie_fthoming",
"uie_ftguided",
// Антиматтер
"uie_amweaponcount",
"uie_amdelay",
"uie_amspeed",
"uie_ambounce",
"uie_amgravity",
"uie_amtimeout",
"uie_amsradius",
"uie_amsdamage",
"uie_amdamage",
"uie_amvampire",
"uie_aminf",
"uie_ambouncemodifier",
"uie_amknockback",
"uie_amhoming",
"uie_amguided",

	// uie_<gametype>_*
	"*fragtype", "*timetype", "*customfraglimits", "*customcapturelimits",
	"*customtimelimits", "*maplist", "*maplistexclude", "*MapRepeat",
	"*MapSource", "*RandomMapCount", "*RandomMapType", "*slottype",
	"*botname", "*botexclude", "*botskill", "*BotSelection", "*BotCount",
	"*BotChange", "*OpenSlotCount", "*BotSkillType", "*BotSkillValue",
	"*BotSkillBias", "*PlayerJoinAs", "*hostname", "*ForceRespawn",
	"*itemGroups", "*itemsHidden", "*Warmup", "*doWarmup", "*weaponrespawn", "*viewdistance",

	// many of these are specific to a gametype, but since we
	// check for the existance of the Cvar they won't appear in
	// gametypes that don't set them

	"*AutoJoin", "*TeamBalance", "*TeamSwapped", "*AutoJoin",
	"*TeamBalance", "*TeamSwapped", "*fraglimit", "*capturelimit", "*timelimit", "*friendly",
	0
};

static const int botChange_frequency[BOTCHANGE_COUNT + 1] = {
	0,	// BOTCHANGE_NEVER
	1,
	2,
	3,
	4,	// BOTCHANGE_MAP4
	0	// size matches botChange_list[]
};

// internal Q3 exe script buffer is limited to 16K in size
// so there's no benefit in increasing our script buffer
#define SCRIPT_BUFFER (1024 * 128 - 1)

#define SERVER_ADDBOT "uie_ab"
#define SERVER_KICKBOT "uie_kb"


typedef struct serverexec_s {
	char server_script[SCRIPT_BUFFER];

	int maxBots;
	int usedBots[MAX_BOTS];
	int lastBots[MAX_BOTS];

	int player_client;

	int rnd_nummaps;

	int cycle_count;	// number of maps in the cycle

	// for MAP_MS_RANDOMLIST: stores listed map order
	// for MAP_MS_RANDOM, MAP_MS_RANDOMEXCLUDE: stores map indices as they're generated
	qboolean random_generate;	// qtrue if map_rnd_index[] is used	
	qboolean random_order;		// qtrue if map_rnd[] is used
	int	random_count;			// number of variables stored in map_rnd[] 
	int map_rnd[MAX_MAPS_LIST];	// order in which maps are written
	int map_rnd_index[MAX_MAPS_LIST];	// indices of map that will be written

	int botcount_firstmap;	// only used with BOTTYPE_*ARENASCRIPT
	int max_scripted_bots;	// only used with BOTTYPE_*ARENASCRIPT

   int bots_in_slots[PLAYER_SLOTS]; // the bot indexes for bots found in selection slots
   int num_bots_in_slots;  // number of bots found in selection slots

	// protects against ordering problems
	qboolean map_generated;
	qboolean bot_generated;
} serverexec_t;

static serverexec_t s_serverexec;


static int addbot_primes[] = { 7, 11, 17, 23 };
static int num_addbot_primes = sizeof (addbot_primes)/sizeof(addbot_primes[0]);

static char lasterror_text[MAX_STATUSBAR_TEXT] = {'\0'};


static char* bot_teamname[] = { "blue", "red" };




/*
=============================================================================

WRITE SCRIPT *****

=============================================================================
*/




/*
=================
UIE_IsCaptureLimited

Returns qtrue if the gametype uses capturelimit instead of fraglimit
=================
*/
static qboolean UIE_IsCaptureLimited(int type)
{
	if (type >= GT_CTF && !(type == GT_LMS))
		return qtrue;

	return qfalse;
}


/*
=================
Clamp_Random

Returns a random number between 0 and r
=================
*/
int Clamp_Random(int r)
{
	int index;

	r += 1;
	do {
		index = r * random();
	} while (index == r);

	return index;
}



/*
=================
AddScript
=================
*/
static void AddScript(const char* scriptLine)
{
	Q_strcat(s_serverexec.server_script, SCRIPT_BUFFER, scriptLine);
}



/*
=================
StartServer_GetLastScriptError
=================
*/
const char* StartServer_GetLastScriptError( void )
{
	return lasterror_text;
}



/*
=================
StartServer_PrintMessage
=================
*/
static void StartServer_PrintMessage( const char* error )
{
	if (!error)
		return;

	trap_Print( va(S_COLOR_RED"StartServer: %s", error));
	Q_strncpyz(lasterror_text, error, MAX_STATUSBAR_TEXT);
}




/*
=================
StartServer_InitServerExec
=================
*/
static qboolean StartServer_InitServerExec( void )
{
	int i, count, mapsource;
   int index, botnum;

	memset(&s_serverexec, 0, sizeof(serverexec_t));

	//
	// init map stuff
	//
	count = s_scriptdata.map.num_maps;
	mapsource = s_scriptdata.map.listSource;

	if (StartServer_IsRandomGeneratedMapList(mapsource))
	{
		count = s_scriptdata.map.SourceCount;
	}

	s_serverexec.cycle_count = count;

	//
	// init bot stuff
	//
	s_serverexec.maxBots = UI_GetNumBots();

	if (s_serverexec.maxBots >= MAX_BOTS)
		s_serverexec.maxBots = MAX_BOTS - 1;

	s_serverexec.random_generate = qfalse;
	s_serverexec.random_order = qfalse;
	s_serverexec.random_count = 0;

	s_serverexec.botcount_firstmap = 0;
	s_serverexec.max_scripted_bots = 0;

	s_serverexec.map_generated = qfalse;
	s_serverexec.bot_generated = qfalse;

   // build list of bots found in slots (info may or may not be used)
   index = 0;
   for (i = 0; i < PLAYER_SLOTS; i++) {
      if (s_scriptdata.bot.slotType[i] != SLOTTYPE_BOT)
         continue;   // not a bot
      if (s_scriptdata.bot.name[i][0] == '\0')
         continue;   // blank name
      botnum = UI_GetBotNumByName(s_scriptdata.bot.name[i]);
      if (botnum >= 0) {
         s_serverexec.bots_in_slots[index] = botnum;
         index++;
      }
   }
   s_serverexec.num_bots_in_slots = index;

   if (s_scriptdata.bot.typeSelect == BOTTYPE_SELECTARENASCRIPT && s_serverexec.num_bots_in_slots == 0) {
		trap_Print(S_COLOR_RED"InitServerExec: Not enough bots for game type\n");
      return qfalse;
   }

	return qtrue;
}




/*
=================
StartServer_WriteServerParams
=================
*/
static qboolean StartServer_WriteServerParams( void )
{
	int i, value, open;
	char* password;
	int botcount;
	qboolean useping;

	AddScript("\n// WriteServerParams()\n\n");

	if (!s_serverexec.bot_generated) {
		trap_Print(S_COLOR_RED"WriteServerParams called before WriteBotParams\n");
		return qfalse;
	}

	AddScript(va("g_gameType %i\n", s_scriptdata.gametype));

	if (s_scriptdata.server.hostname[0])
		AddScript(va("sv_hostname \"%s\"\n", s_scriptdata.server.hostname));

	AddScript(va("sv_pure 0\n", s_scriptdata.server.pure));
	if (s_scriptdata.multiplayer) {
		AddScript(va("dedicated %i\n", s_scriptdata.server.dedicatedServer));
		AddScript(va("g_inactivity %i\n", s_scriptdata.server.inactivityTime));

      // LAN force rate
      AddScript(va("sv_lanForceRate %i\n", s_scriptdata.server.lanForceRate));

	}

	AddScript(va("set g_forcerespawn %i\n", s_scriptdata.server.forceRespawn));
	//Noire.dev
AddScript(va("set g_maxEntities %i\n", s_scriptdata.server.maxEntities));
AddScript(va("set g_spSkill %i\n", s_scriptdata.server.singleskill));
AddScript(va("set g_kill %i\n", s_scriptdata.server.kill));
AddScript(va("set g_damageModifier  \"%s\"\n", s_scriptdata.server.damageModifier));
AddScript(va("set cl_selectedmod  \"%s\"\n", s_scriptdata.server.selectedmod));
AddScript(va("set g_elimination %i\n", s_scriptdata.server.elimination));
AddScript(va("set g_obeliskHealth %i\n", s_scriptdata.server.obeliskHealth));
AddScript(va("set g_obeliskRegenPeriod %i\n", s_scriptdata.server.obeliskRegenPeriod));
AddScript(va("set g_obeliskRegenAmount %i\n", s_scriptdata.server.obeliskRegenAmount));
AddScript(va("set g_obeliskRespawnDelay %i\n", s_scriptdata.server.obeliskRespawnDelay));
AddScript(va("set g_cubeTimeout %i\n", s_scriptdata.server.cubeTimeout));
AddScript(va("set g_flagrespawn %i\n", s_scriptdata.server.flagrespawn));
AddScript(va("set g_weaponTeamRespawn %i\n", s_scriptdata.server.weaponTeamRespawn));
AddScript(va("set elimination_ctf_oneway %i\n", s_scriptdata.server.elimination_ctf_oneway));
AddScript(va("set elimination_selfdamage %i\n", s_scriptdata.server.elimination_selfdamage));
AddScript(va("set elimination_roundtime %i\n", s_scriptdata.server.elimination_roundtime));
AddScript(va("set elimination_warmup %i\n", s_scriptdata.server.elimination_warmup));
AddScript(va("set elimination_activewarmup %i\n", s_scriptdata.server.elimination_activewarmup));
AddScript(va("set g_lms_lives %i\n", s_scriptdata.server.lms_lives));
AddScript(va("set g_lms_mode %i\n", s_scriptdata.server.lms_mode));
AddScript(va("set g_accelerate %i\n", s_scriptdata.server.accelerate));
AddScript(va("set g_spectatorspeed %i\n", s_scriptdata.server.spectatorspeed));
AddScript(va("set g_speed %i\n", s_scriptdata.server.speed));
AddScript(va("set g_gravity %i\n", s_scriptdata.server.gravity));
AddScript(va("set g_gravityModifier  \"%s\"\n", s_scriptdata.server.gravityModifier));
AddScript(va("set g_knockback %i\n", s_scriptdata.server.knockback));
AddScript(va("set g_noplayerclip %i\n", s_scriptdata.server.noplayerclip));
AddScript(va("set g_jumpheight %i\n", s_scriptdata.server.jumpheight));
AddScript(va("set g_regenarmor %i\n", s_scriptdata.server.regenarmor));
AddScript(va("set g_ammolimit %i\n", s_scriptdata.server.ammolimit));
AddScript(va("set g_quadfactor  \"%s\"\n", s_scriptdata.server.quadfactor));
AddScript(va("set g_respawntime %i\n", s_scriptdata.server.respawntime));
AddScript(va("set g_forcerespawn %i\n", s_scriptdata.server.forcerespawn));
AddScript(va("set g_vampire  \"%s\"\n", s_scriptdata.server.vampire));
AddScript(va("set g_vampire_max_health %i\n", s_scriptdata.server.vampire_max_health));
AddScript(va("set g_regen %i\n", s_scriptdata.server.regen));
AddScript(va("set g_maxweaponpickup %i\n", s_scriptdata.server.maxweaponpickup));
AddScript(va("set g_droppeditemtime %i\n", s_scriptdata.server.droppeditemtime));
AddScript(va("set g_autoflagreturn %i\n", s_scriptdata.server.autoflagreturn));
AddScript(va("set g_armorprotect  \"%s\"\n", s_scriptdata.server.armorprotect));
AddScript(va("set g_respawnwait %i\n", s_scriptdata.server.respawnwait));
AddScript(va("set g_speedfactor  \"%s\"\n", s_scriptdata.server.speedfactor));
AddScript(va("set g_scoutspeedfactor  \"%s\"\n", s_scriptdata.server.scoutspeedfactor));
AddScript(va("set g_scoutfirespeed  \"%s\"\n", s_scriptdata.server.scoutfirespeed));
AddScript(va("set g_scoutdamagefactor  \"%s\"\n", s_scriptdata.server.scoutdamagefactor));
AddScript(va("set g_scoutgravitymodifier  \"%s\"\n", s_scriptdata.server.scoutgravitymodifier));
AddScript(va("set g_scout_infammo %i\n", s_scriptdata.server.scout_infammo));
AddScript(va("set g_scouthealthmodifier  \"%s\"\n", s_scriptdata.server.scouthealthmodifier));
AddScript(va("set g_doublerfirespeed  \"%s\"\n", s_scriptdata.server.doublerfirespeed));
AddScript(va("set g_doublerdamagefactor  \"%s\"\n", s_scriptdata.server.doublerdamagefactor));
AddScript(va("set g_doublerspeedfactor  \"%s\"\n", s_scriptdata.server.doublerspeedfactor));
AddScript(va("set g_doublergravitymodifier  \"%s\"\n", s_scriptdata.server.doublergravitymodifier));
AddScript(va("set g_doubler_infammo %i\n", s_scriptdata.server.doubler_infammo));
AddScript(va("set g_doublerhealthmodifier  \"%s\"\n", s_scriptdata.server.doublerhealthmodifier));
AddScript(va("set g_guardhealthmodifier  \"%s\"\n", s_scriptdata.server.guardhealthmodifier));
AddScript(va("set g_guardfirespeed  \"%s\"\n", s_scriptdata.server.guardfirespeed));
AddScript(va("set g_guarddamagefactor  \"%s\"\n", s_scriptdata.server.guarddamagefactor));
AddScript(va("set g_guardspeedfactor  \"%s\"\n", s_scriptdata.server.guardspeedfactor));
AddScript(va("set g_guardgravitymodifier  \"%s\"\n", s_scriptdata.server.guardgravitymodifier));
AddScript(va("set g_guard_infammo %i\n", s_scriptdata.server.guard_infammo));
AddScript(va("set g_ammoregenfirespeed  \"%s\"\n", s_scriptdata.server.ammoregenfirespeed));
AddScript(va("set g_ammoregen_infammo %i\n", s_scriptdata.server.ammoregen_infammo));
AddScript(va("set g_ammoregendamagefactor  \"%s\"\n", s_scriptdata.server.ammoregendamagefactor));
AddScript(va("set g_ammoregenspeedfactor  \"%s\"\n", s_scriptdata.server.ammoregenspeedfactor));
AddScript(va("set g_ammoregengravitymodifier  \"%s\"\n", s_scriptdata.server.ammoregengravitymodifier));
AddScript(va("set g_ammoregenhealthmodifier  \"%s\"\n", s_scriptdata.server.ammoregenhealthmodifier));
AddScript(va("set g_teamred_speed %i\n", s_scriptdata.server.teamred_speed));
AddScript(va("set g_teamred_gravityModifier  \"%s\"\n", s_scriptdata.server.teamred_gravityModifier));
AddScript(va("set g_teamred_firespeed  \"%s\"\n", s_scriptdata.server.teamred_firespeed));
AddScript(va("set g_teamred_damage  \"%s\"\n", s_scriptdata.server.teamred_damage));
AddScript(va("set g_teamred_infammo %i\n", s_scriptdata.server.teamred_infammo));
AddScript(va("set g_teamred_respawnwait %i\n", s_scriptdata.server.teamred_respawnwait));
AddScript(va("set g_teamred_pickupitems %i\n", s_scriptdata.server.teamred_pickupitems));
AddScript(va("set eliminationredrespawn %i\n", s_scriptdata.server.eliminationredrespawn));
AddScript(va("set eliminationred_startHealth %i\n", s_scriptdata.server.eliminationred_startHealth));
AddScript(va("set eliminationred_startArmor %i\n", s_scriptdata.server.eliminationred_startArmor));
AddScript(va("set g_teamblue_speed %i\n", s_scriptdata.server.teamblue_speed));
AddScript(va("set g_teamblue_gravityModifier  \"%s\"\n", s_scriptdata.server.teamblue_gravityModifier));
AddScript(va("set g_teamblue_firespeed  \"%s\"\n", s_scriptdata.server.teamblue_firespeed));
AddScript(va("set g_teamblue_damage  \"%s\"\n", s_scriptdata.server.teamblue_damage));
AddScript(va("set g_teamblue_infammo %i\n", s_scriptdata.server.teamblue_infammo));
AddScript(va("set g_teamblue_respawnwait %i\n", s_scriptdata.server.teamblue_respawnwait));
AddScript(va("set g_teamblue_pickupitems %i\n", s_scriptdata.server.teamblue_pickupitems));
AddScript(va("set eliminationrespawn %i\n", s_scriptdata.server.eliminationrespawn));
AddScript(va("set elimination_startHealth %i\n", s_scriptdata.server.elimination_startHealth));
AddScript(va("set elimination_startArmor %i\n", s_scriptdata.server.elimination_startArmor));
AddScript(va("set eliminationred_grapple %i\n", s_scriptdata.server.eliminationred_grapple));
AddScript(va("set eliminationred_gauntlet %i\n", s_scriptdata.server.eliminationred_gauntlet));
AddScript(va("set eliminationred_machinegun %i\n", s_scriptdata.server.eliminationred_machinegun));
AddScript(va("set eliminationred_shotgun %i\n", s_scriptdata.server.eliminationred_shotgun));
AddScript(va("set eliminationred_grenade %i\n", s_scriptdata.server.eliminationred_grenade));
AddScript(va("set eliminationred_rocket %i\n", s_scriptdata.server.eliminationred_rocket));
AddScript(va("set eliminationred_railgun %i\n", s_scriptdata.server.eliminationred_railgun));
AddScript(va("set eliminationred_lightning %i\n", s_scriptdata.server.eliminationred_lightning));
AddScript(va("set eliminationred_plasmagun %i\n", s_scriptdata.server.eliminationred_plasmagun));
AddScript(va("set eliminationred_bfg %i\n", s_scriptdata.server.eliminationred_bfg));
AddScript(va("set eliminationred_chain %i\n", s_scriptdata.server.eliminationred_chain));
AddScript(va("set eliminationred_mine %i\n", s_scriptdata.server.eliminationred_mine));
AddScript(va("set eliminationred_nail %i\n", s_scriptdata.server.eliminationred_nail));
AddScript(va("set eliminationred_flame %i\n", s_scriptdata.server.eliminationred_flame));
AddScript(va("set eliminationred_antimatter %i\n", s_scriptdata.server.eliminationred_antimatter));
AddScript(va("set elimination_grapple %i\n", s_scriptdata.server.elimination_grapple));
AddScript(va("set elimination_gauntlet %i\n", s_scriptdata.server.elimination_gauntlet));
AddScript(va("set elimination_machinegun %i\n", s_scriptdata.server.elimination_machinegun));
AddScript(va("set elimination_shotgun %i\n", s_scriptdata.server.elimination_shotgun));
AddScript(va("set elimination_grenade %i\n", s_scriptdata.server.elimination_grenade));
AddScript(va("set elimination_rocket %i\n", s_scriptdata.server.elimination_rocket));
AddScript(va("set elimination_railgun %i\n", s_scriptdata.server.elimination_railgun));
AddScript(va("set elimination_lightning %i\n", s_scriptdata.server.elimination_lightning));
AddScript(va("set elimination_plasmagun %i\n", s_scriptdata.server.elimination_plasmagun));
AddScript(va("set elimination_bfg %i\n", s_scriptdata.server.elimination_bfg));
AddScript(va("set elimination_chain %i\n", s_scriptdata.server.elimination_chain));
AddScript(va("set elimination_mine %i\n", s_scriptdata.server.elimination_mine));
AddScript(va("set elimination_nail %i\n", s_scriptdata.server.elimination_nail));
AddScript(va("set elimination_flame %i\n", s_scriptdata.server.elimination_flame));
AddScript(va("set elimination_antimatter %i\n", s_scriptdata.server.elimination_antimatter));
AddScript(va("set eliminationred_quad %i\n", s_scriptdata.server.eliminationred_quad));
AddScript(va("set elimination_haste %i\n", s_scriptdata.server.elimination_haste));
AddScript(va("set elimination_bsuit %i\n", s_scriptdata.server.elimination_bsuit));
AddScript(va("set elimination_invis %i\n", s_scriptdata.server.elimination_invis));
AddScript(va("set elimination_regen %i\n", s_scriptdata.server.elimination_regen));
AddScript(va("set elimination_flight %i\n", s_scriptdata.server.elimination_flight));
AddScript(va("set elimination_items %i\n", s_scriptdata.server.elimination_items));
AddScript(va("set elimination_holdable %i\n", s_scriptdata.server.elimination_holdable));
AddScript(va("set g_minigame %i\n", s_scriptdata.server.minigame));
AddScript(va("set g_overlay %i\n", s_scriptdata.server.overlay));
AddScript(va("set g_randomItems %i\n", s_scriptdata.server.randomItems));
AddScript(va("set g_slickmove %i\n", s_scriptdata.server.slickmove));
AddScript(va("set g_spawnprotect %i\n", s_scriptdata.server.spawnprotect));
AddScript(va("set elimination_lockspectator %i\n", s_scriptdata.server.elimination_lockspectator));
AddScript(va("set g_awardpushing %i\n", s_scriptdata.server.awardpushing));
AddScript(va("set g_randomteleport %i\n", s_scriptdata.server.randomteleport));
AddScript(va("set g_falldamagesmall %i\n", s_scriptdata.server.falldamagesmall));
AddScript(va("set g_falldamagebig %i\n", s_scriptdata.server.falldamagebig));
AddScript(va("set g_waterdamage %i\n", s_scriptdata.server.waterdamage));
AddScript(va("set g_lavadamage %i\n", s_scriptdata.server.lavadamage));
AddScript(va("set g_slimedamage %i\n", s_scriptdata.server.slimedamage));
AddScript(va("set g_drowndamage %i\n", s_scriptdata.server.drowndamage));
AddScript(va("set g_invulinf %i\n", s_scriptdata.server.invulinf));
AddScript(va("set g_invulmove %i\n", s_scriptdata.server.invulmove));
AddScript(va("set g_invultime %i\n", s_scriptdata.server.invultime));
AddScript(va("set g_kamikazeinf %i\n", s_scriptdata.server.kamikazeinf));
AddScript(va("set g_portalinf %i\n", s_scriptdata.server.portalinf));
AddScript(va("set g_portaltimeout %i\n", s_scriptdata.server.portaltimeout));
AddScript(va("set g_portalhealth %i\n", s_scriptdata.server.portalhealth));
AddScript(va("set g_teleporterinf %i\n", s_scriptdata.server.teleporterinf));
AddScript(va("set g_medkitinf %i\n", s_scriptdata.server.medkitinf));
AddScript(va("set g_medkitlimit %i\n", s_scriptdata.server.medkitlimit));
AddScript(va("set g_medkitmodifier %i\n", s_scriptdata.server.medkitmodifier));
AddScript(va("set g_fasthealthregen %i\n", s_scriptdata.server.fasthealthregen));
AddScript(va("set g_slowhealthregen %i\n", s_scriptdata.server.slowhealthregen));
AddScript(va("set g_hastefirespeed \"%s\"\n", s_scriptdata.server.hastefirespeed));
AddScript(va("set g_quadtime %i\n", s_scriptdata.server.quadtime));
AddScript(va("set g_bsuittime %i\n", s_scriptdata.server.bsuittime));
AddScript(va("set g_hastetime %i\n", s_scriptdata.server.hastetime));
AddScript(va("set g_invistime %i\n", s_scriptdata.server.invistime));
AddScript(va("set g_regentime %i\n", s_scriptdata.server.regentime));
AddScript(va("set g_flighttime %i\n", s_scriptdata.server.flighttime));
AddScript(va("set g_armorrespawn %i\n", s_scriptdata.server.armorrespawn));
AddScript(va("set g_healthrespawn %i\n", s_scriptdata.server.healthrespawn));
AddScript(va("set g_ammorespawn %i\n", s_scriptdata.server.ammorespawn));
AddScript(va("set g_holdablerespawn %i\n", s_scriptdata.server.holdablerespawn));
AddScript(va("set g_megahealthrespawn %i\n", s_scriptdata.server.megahealthrespawn));
AddScript(va("set g_poweruprespawn %i\n", s_scriptdata.server.poweruprespawn));
AddScript(va("set g_weaponrespawn %i\n", s_scriptdata.server.weaponrespawn));
// Крюк
AddScript(va("set g_ghspeed %i\n", s_scriptdata.server.g_ghspeed));
AddScript(va("set g_ghtimeout %i\n", s_scriptdata.server.g_ghtimeout));
// Пила
AddScript(va("set g_gdelay %i\n", s_scriptdata.server.g_gdelay));
AddScript(va("set g_gdamage %i\n", s_scriptdata.server.g_gdamage));
AddScript(va("set g_grange %i\n", s_scriptdata.server.g_grange));
AddScript(va("set g_gknockback  \"%s\"\n", s_scriptdata.server.g_gknockback));
// Автомат
AddScript(va("set g_mgammocount %i\n", s_scriptdata.server.g_mgammocount));
AddScript(va("set g_mgweaponcount %i\n", s_scriptdata.server.g_mgweaponcount));
AddScript(va("set g_mgdelay %i\n", s_scriptdata.server.g_mgdelay));
AddScript(va("set g_mgdamage %i\n", s_scriptdata.server.g_mgdamage));
AddScript(va("set g_mgspread %i\n", s_scriptdata.server.g_mgspread));
AddScript(va("set g_mgexplode %i\n", s_scriptdata.server.g_mgexplode));
AddScript(va("set g_mgsdamage %i\n", s_scriptdata.server.g_mgsdamage));
AddScript(va("set g_mgsradius %i\n", s_scriptdata.server.g_mgsradius));
AddScript(va("set g_mgvampire %i\n", s_scriptdata.server.g_mgvampire));
AddScript(va("set g_mginf %i\n", s_scriptdata.server.g_mginf));
AddScript(va("set g_mgknockback  \"%s\"\n", s_scriptdata.server.g_mgknockback));
// Дробовик
AddScript(va("set g_sgammocount %i\n", s_scriptdata.server.g_sgammocount));
AddScript(va("set g_sgweaponcount %i\n", s_scriptdata.server.g_sgweaponcount));
AddScript(va("set g_sgdelay %i\n", s_scriptdata.server.g_sgdelay));
AddScript(va("set g_sgdamage %i\n", s_scriptdata.server.g_sgdamage));
AddScript(va("set g_sgspread %i\n", s_scriptdata.server.g_sgspread));
AddScript(va("set g_sgexplode %i\n", s_scriptdata.server.g_sgexplode));
AddScript(va("set g_sgsdamage %i\n", s_scriptdata.server.g_sgsdamage));
AddScript(va("set g_sgsradius %i\n", s_scriptdata.server.g_sgsradius));
AddScript(va("set g_sgcount %i\n", s_scriptdata.server.g_sgcount));
AddScript(va("set g_sgvampire %i\n", s_scriptdata.server.g_sgvampire));
AddScript(va("set g_sginf %i\n", s_scriptdata.server.g_sginf));
AddScript(va("set g_sgknockback  \"%s\"\n", s_scriptdata.server.g_sgknockback));
// Гранаты
AddScript(va("set g_glammocount %i\n", s_scriptdata.server.g_glammocount));
AddScript(va("set g_glweaponcount %i\n", s_scriptdata.server.g_glweaponcount));
AddScript(va("set g_gldelay %i\n", s_scriptdata.server.g_gldelay));
AddScript(va("set g_glspeed %i\n", s_scriptdata.server.g_glspeed));
AddScript(va("set g_glbounce %i\n", s_scriptdata.server.g_glbounce));
AddScript(va("set g_glgravity %i\n", s_scriptdata.server.g_glgravity));
AddScript(va("set g_gltimeout %i\n", s_scriptdata.server.g_gltimeout));
AddScript(va("set g_glsradius %i\n", s_scriptdata.server.g_glsradius));
AddScript(va("set g_glsdamage %i\n", s_scriptdata.server.g_glsdamage));
AddScript(va("set g_gldamage %i\n", s_scriptdata.server.g_gldamage));
AddScript(va("set g_glvampire %i\n", s_scriptdata.server.g_glvampire));
AddScript(va("set g_glinf %i\n", s_scriptdata.server.g_glinf));
AddScript(va("set g_glbouncemodifier  \"%s\"\n", s_scriptdata.server.g_glbouncemodifier));
AddScript(va("set g_glknockback  \"%s\"\n", s_scriptdata.server.g_glknockback));
AddScript(va("set g_glhoming %i\n", s_scriptdata.server.g_glhoming));
AddScript(va("set g_glguided %i\n", s_scriptdata.server.g_glguided));
// Ракеты
AddScript(va("set g_rlammocount %i\n", s_scriptdata.server.g_rlammocount));
AddScript(va("set g_rlweaponcount %i\n", s_scriptdata.server.g_rlweaponcount));
AddScript(va("set g_rldelay %i\n", s_scriptdata.server.g_rldelay));
AddScript(va("set g_rlspeed %i\n", s_scriptdata.server.g_rlspeed));
AddScript(va("set g_rlbounce %i\n", s_scriptdata.server.g_rlbounce));
AddScript(va("set g_rlgravity %i\n", s_scriptdata.server.g_rlgravity));
AddScript(va("set g_rltimeout %i\n", s_scriptdata.server.g_rltimeout));
AddScript(va("set g_rlsradius %i\n", s_scriptdata.server.g_rlsradius));
AddScript(va("set g_rlsdamage %i\n", s_scriptdata.server.g_rlsdamage));
AddScript(va("set g_rldamage %i\n", s_scriptdata.server.g_rldamage));
AddScript(va("set g_rlvampire %i\n", s_scriptdata.server.g_rlvampire));
AddScript(va("set g_rlinf %i\n", s_scriptdata.server.g_rlinf));
AddScript(va("set g_rlbouncemodifier  \"%s\"\n", s_scriptdata.server.g_rlbouncemodifier));
AddScript(va("set g_rlknockback  \"%s\"\n", s_scriptdata.server.g_rlknockback));
AddScript(va("set g_rlhoming %i\n", s_scriptdata.server.g_rlhoming));
AddScript(va("set g_rlguided %i\n", s_scriptdata.server.g_rlguided));
// Молния
AddScript(va("set g_lgammocount %i\n", s_scriptdata.server.g_lgammocount));
AddScript(va("set g_lgweaponcount %i\n", s_scriptdata.server.g_lgweaponcount));
AddScript(va("set g_lgrange %i\n", s_scriptdata.server.g_lgrange));
AddScript(va("set g_lgdelay %i\n", s_scriptdata.server.g_lgdelay));
AddScript(va("set g_lgdamage %i\n", s_scriptdata.server.g_lgdamage));
AddScript(va("set g_lgvampire %i\n", s_scriptdata.server.g_lgvampire));
AddScript(va("set g_lgexplode %i\n", s_scriptdata.server.g_lgexplode));
AddScript(va("set g_lgsdamage %i\n", s_scriptdata.server.g_lgsdamage));
AddScript(va("set g_lgsradius %i\n", s_scriptdata.server.g_lgsradius));
AddScript(va("set g_lginf %i\n", s_scriptdata.server.g_lginf));
AddScript(va("set g_lgknockback  \"%s\"\n", s_scriptdata.server.g_lgknockback));
// Рэйлган
AddScript(va("set g_rgammocount %i\n", s_scriptdata.server.g_rgammocount));
AddScript(va("set g_rgweaponcount %i\n", s_scriptdata.server.g_rgweaponcount));
AddScript(va("set g_rgdelay %i\n", s_scriptdata.server.g_rgdelay));
AddScript(va("set g_rgdamage %i\n", s_scriptdata.server.g_rgdamage));
AddScript(va("set g_rgvampire %i\n", s_scriptdata.server.g_rgvampire));
AddScript(va("set g_rginf %i\n", s_scriptdata.server.g_rginf));
AddScript(va("set g_rgknockback  \"%s\"\n", s_scriptdata.server.g_rgknockback));
// Плазмаган
AddScript(va("set g_pgammocount %i\n", s_scriptdata.server.g_pgammocount));
AddScript(va("set g_pgweaponcount %i\n", s_scriptdata.server.g_pgweaponcount));
AddScript(va("set g_pgdelay %i\n", s_scriptdata.server.g_pgdelay));
AddScript(va("set g_pgspeed %i\n", s_scriptdata.server.g_pgspeed));
AddScript(va("set g_pgbounce %i\n", s_scriptdata.server.g_pgbounce));
AddScript(va("set g_pggravity %i\n", s_scriptdata.server.g_pggravity));
AddScript(va("set g_pgtimeout %i\n", s_scriptdata.server.g_pgtimeout));
AddScript(va("set g_pgsradius %i\n", s_scriptdata.server.g_pgsradius));
AddScript(va("set g_pgsdamage %i\n", s_scriptdata.server.g_pgsdamage));
AddScript(va("set g_pgdamage %i\n", s_scriptdata.server.g_pgdamage));
AddScript(va("set g_pgvampire %i\n", s_scriptdata.server.g_pgvampire));
AddScript(va("set g_pginf %i\n", s_scriptdata.server.g_pginf));
AddScript(va("set g_pgbouncemodifier  \"%s\"\n", s_scriptdata.server.g_pgbouncemodifier));
AddScript(va("set g_pgknockback  \"%s\"\n", s_scriptdata.server.g_pgknockback));
AddScript(va("set g_pghoming %i\n", s_scriptdata.server.g_pghoming));
AddScript(va("set g_pgguided %i\n", s_scriptdata.server.g_pgguided));
// Бфг
AddScript(va("set g_bfgammocount %i\n", s_scriptdata.server.g_bfgammocount));
AddScript(va("set g_bfgweaponcount %i\n", s_scriptdata.server.g_bfgweaponcount));
AddScript(va("set g_bfgdelay %i\n", s_scriptdata.server.g_bfgdelay));
AddScript(va("set g_bfgspeed %i\n", s_scriptdata.server.g_bfgspeed));
AddScript(va("set g_bfgbounce %i\n", s_scriptdata.server.g_bfgbounce));
AddScript(va("set g_bfggravity %i\n", s_scriptdata.server.g_bfggravity));
AddScript(va("set g_bfgtimeout %i\n", s_scriptdata.server.g_bfgtimeout));
AddScript(va("set g_bfgsradius %i\n", s_scriptdata.server.g_bfgsradius));
AddScript(va("set g_bfgsdamage %i\n", s_scriptdata.server.g_bfgsdamage));
AddScript(va("set g_bfgdamage %i\n", s_scriptdata.server.g_bfgdamage));
AddScript(va("set g_bfgvampire %i\n", s_scriptdata.server.g_bfgvampire));
AddScript(va("set g_bfginf %i\n", s_scriptdata.server.g_bfginf));
AddScript(va("set g_bfgbouncemodifier  \"%s\"\n", s_scriptdata.server.g_bfgbouncemodifier));
AddScript(va("set g_bfgknockback  \"%s\"\n", s_scriptdata.server.g_bfgknockback));
AddScript(va("set g_bfghoming %i\n", s_scriptdata.server.g_bfghoming));
AddScript(va("set g_bfgguided %i\n", s_scriptdata.server.g_bfgguided));
// Гвоздомёт
AddScript(va("set g_ngammocount %i\n", s_scriptdata.server.g_ngammocount));
AddScript(va("set g_ngweaponcount %i\n", s_scriptdata.server.g_ngweaponcount));
AddScript(va("set g_ngdelay %i\n", s_scriptdata.server.g_ngdelay));
AddScript(va("set g_ngspeed %i\n", s_scriptdata.server.g_ngspeed));
AddScript(va("set g_ngbounce %i\n", s_scriptdata.server.g_ngbounce));
AddScript(va("set g_nggravity %i\n", s_scriptdata.server.g_nggravity));
AddScript(va("set g_ngtimeout %i\n", s_scriptdata.server.g_ngtimeout));
AddScript(va("set g_ngcount %i\n", s_scriptdata.server.g_ngcount));
AddScript(va("set g_ngspread %i\n", s_scriptdata.server.g_ngspread));
AddScript(va("set g_ngdamage %i\n", s_scriptdata.server.g_ngdamage));
AddScript(va("set g_ngrandom %i\n", s_scriptdata.server.g_ngrandom));
AddScript(va("set g_ngvampire %i\n", s_scriptdata.server.g_ngvampire));
AddScript(va("set g_nginf %i\n", s_scriptdata.server.g_nginf));
AddScript(va("set g_ngbouncemodifier  \"%s\"\n", s_scriptdata.server.g_ngbouncemodifier));
AddScript(va("set g_ngknockback  \"%s\"\n", s_scriptdata.server.g_ngknockback));
AddScript(va("set g_nghoming %i\n", s_scriptdata.server.g_nghoming));
AddScript(va("set g_ngguided %i\n", s_scriptdata.server.g_ngguided));
// Мины
AddScript(va("set g_plammocount %i\n", s_scriptdata.server.g_plammocount));
AddScript(va("set g_plweaponcount %i\n", s_scriptdata.server.g_plweaponcount));
AddScript(va("set g_pldelay %i\n", s_scriptdata.server.g_pldelay));
AddScript(va("set g_plspeed %i\n", s_scriptdata.server.g_plspeed));
AddScript(va("set g_plgravity %i\n", s_scriptdata.server.g_plgravity));
AddScript(va("set g_pltimeout %i\n", s_scriptdata.server.g_pltimeout));
AddScript(va("set g_plsradius %i\n", s_scriptdata.server.g_plsradius));
AddScript(va("set g_plsdamage %i\n", s_scriptdata.server.g_plsdamage));
AddScript(va("set g_pldamage %i\n", s_scriptdata.server.g_pldamage));
AddScript(va("set g_plvampire %i\n", s_scriptdata.server.g_plvampire));
AddScript(va("set g_plinf %i\n", s_scriptdata.server.g_plinf));
AddScript(va("set g_plknockback  \"%s\"\n", s_scriptdata.server.g_plknockback));
// Пулемёт
AddScript(va("set g_cgammocount %i\n", s_scriptdata.server.g_cgammocount));
AddScript(va("set g_cgweaponcount %i\n", s_scriptdata.server.g_cgweaponcount));
AddScript(va("set g_cgdelay %i\n", s_scriptdata.server.g_cgdelay));
AddScript(va("set g_cgspread %i\n", s_scriptdata.server.g_cgspread));
AddScript(va("set g_cgdamage %i\n", s_scriptdata.server.g_cgdamage));
AddScript(va("set g_cgvampire %i\n", s_scriptdata.server.g_cgvampire));
AddScript(va("set g_cginf %i\n", s_scriptdata.server.g_cginf));
AddScript(va("set g_cgknockback  \"%s\"\n", s_scriptdata.server.g_cgknockback));
// Огнемёт
AddScript(va("set g_ftammocount %i\n", s_scriptdata.server.g_ftammocount));
AddScript(va("set g_ftweaponcount %i\n", s_scriptdata.server.g_ftweaponcount));
AddScript(va("set g_ftdelay %i\n", s_scriptdata.server.g_ftdelay));
AddScript(va("set g_ftspeed %i\n", s_scriptdata.server.g_ftspeed));
AddScript(va("set g_ftbounce %i\n", s_scriptdata.server.g_ftbounce));
AddScript(va("set g_ftgravity %i\n", s_scriptdata.server.g_ftgravity));
AddScript(va("set g_fttimeout %i\n", s_scriptdata.server.g_fttimeout));
AddScript(va("set g_ftsradius %i\n", s_scriptdata.server.g_ftsradius));
AddScript(va("set g_ftsdamage %i\n", s_scriptdata.server.g_ftsdamage));
AddScript(va("set g_ftdamage %i\n", s_scriptdata.server.g_ftdamage));
AddScript(va("set g_ftvampire %i\n", s_scriptdata.server.g_ftvampire));
AddScript(va("set g_ftinf %i\n", s_scriptdata.server.g_ftinf));
AddScript(va("set g_ftbouncemodifier  \"%s\"\n", s_scriptdata.server.g_ftbouncemodifier));
AddScript(va("set g_ftknockback  \"%s\"\n", s_scriptdata.server.g_ftknockback));
AddScript(va("set g_fthoming %i\n", s_scriptdata.server.g_fthoming));
AddScript(va("set g_ftguided %i\n", s_scriptdata.server.g_ftguided));
// Антиматтер
AddScript(va("set g_amweaponcount %i\n", s_scriptdata.server.g_amweaponcount));
AddScript(va("set g_amdelay %i\n", s_scriptdata.server.g_amdelay));
AddScript(va("set g_amspeed %i\n", s_scriptdata.server.g_amspeed));
AddScript(va("set g_ambounce %i\n", s_scriptdata.server.g_ambounce));
AddScript(va("set g_amgravity %i\n", s_scriptdata.server.g_amgravity));
AddScript(va("set g_amtimeout %i\n", s_scriptdata.server.g_amtimeout));
AddScript(va("set g_amsradius %i\n", s_scriptdata.server.g_amsradius));
AddScript(va("set g_amsdamage %i\n", s_scriptdata.server.g_amsdamage));
AddScript(va("set g_amdamage %i\n", s_scriptdata.server.g_amdamage));
AddScript(va("set g_amvampire %i\n", s_scriptdata.server.g_amvampire));
AddScript(va("set g_aminf %i\n", s_scriptdata.server.g_aminf));
AddScript(va("set g_ambouncemodifier  \"%s\"\n", s_scriptdata.server.g_ambouncemodifier));
AddScript(va("set g_amknockback  \"%s\"\n", s_scriptdata.server.g_amknockback));
AddScript(va("set g_amhoming %i\n", s_scriptdata.server.g_amhoming));
AddScript(va("set g_amguided %i\n", s_scriptdata.server.g_amguided));

	if (s_scriptdata.server.allowWarmup) {
		value = s_scriptdata.server.warmupTime;
		if (value == 0) {
			AddScript(va("set g_doWarmup 0\n"));
		}
		else {
			AddScript(va("set g_doWarmup 1\n"));
			AddScript(va("set g_warmup %i\n", value));
		}
	}
	else {
		AddScript(va("set g_doWarmup 0\n"));
	}
	
	AddScript(va("set sv_viewdistance %i\n", s_scriptdata.server.viewdistance));


	// special server parameters
	if (s_scriptdata.server.allowmaxrate)
		value = s_scriptdata.server.maxrate;
	else
		value = 0;
	AddScript(va("set sv_maxrate %i\n", value));

	password = "";
	if (s_scriptdata.server.allowpass)
		password = s_scriptdata.server.password;
	if (!password[0]) {
		AddScript("set g_needpass 0\n");
	}
	else {
		AddScript(va("set g_needpass %i\n", s_scriptdata.server.allowpass));
		AddScript(va("set g_password \"%s\"\n", password));
		AddScript(va("set password \"%s\"\n", password));
	}

	AddScript(va("set sv_allowDownload %i\n", s_scriptdata.server.allowdownload));
	AddScript(va("set g_allowVote %i\n", s_scriptdata.server.allowvote));

	// team game options
	if (s_scriptdata.gametype >= GT_TEAM) {
		AddScript(va("set g_friendlyFire %i\n", s_scriptdata.server.friendlyFire));
		AddScript(va("set g_teamAutoJoin %i\n", s_scriptdata.server.autoJoin));
		AddScript(va("set g_teamForceBalance %i\n", s_scriptdata.server.teamBalance));
	}

	// count the number of clients
	botcount = 0;
	if (s_scriptdata.bot.typeSelect == BOTTYPE_SELECT) {
		value = 0;
		open = 0;
		for (i = 0; i < PLAYER_SLOTS; i++) {
			if (s_scriptdata.bot.slotType[i] == SLOTTYPE_BOT) {
				value++;
				if (s_scriptdata.bot.name[i][0]!='\0')
					botcount++;
			}
			else if (s_scriptdata.bot.slotType[i] == SLOTTYPE_OPEN)
				value++;
		}
	}
	else if (StartServer_IsBotArenaScript(s_scriptdata.bot.typeSelect)) {
		open = s_scriptdata.bot.numberOpen;
		botcount = s_serverexec.botcount_firstmap;
		value = s_serverexec.max_scripted_bots + open;
	}
	else {	// BOTTYPE_RANDOM or BOTTYPE_RANDOMEXCLUDE
		open = s_scriptdata.bot.numberOpen;
		botcount = s_scriptdata.bot.numberBots;
		value = botcount + open;
	}

	if (s_scriptdata.bot.joinAs == 1 || open == 0)
		value++;	// spectator on, allow one more slot

	if (value > MAX_CLIENTS)
		value = MAX_CLIENTS;

	s_serverexec.player_client = botcount;

	AddScript(va("set sv_maxclients %i\n",value));

	// pmove
	value = s_scriptdata.server.pmove_fixed;
	AddScript(va("set pmove_fixed %i\n", value));
	if (value) {
		AddScript(va("set pmove_msec %i\n", s_scriptdata.server.pmove_msec));
	}

	// smoothclients
	AddScript(va("set g_smoothClients %i\n", s_scriptdata.server.smoothclients));

	// syncclients
	AddScript(va("set g_synchronousClients %i\n", s_scriptdata.server.syncClients));

	// ping limits
	useping = qtrue;
	if (s_scriptdata.server.allowMinPing && s_scriptdata.server.allowMaxPing) {
		if (s_scriptdata.server.minPing > s_scriptdata.server.maxPing)
			useping = qfalse;
	}

	if (s_scriptdata.server.allowMinPing && useping)
		AddScript(va("set sv_minPing %i\n", s_scriptdata.server.minPing));
	if (s_scriptdata.server.allowMaxPing && useping)
		AddScript(va("set sv_maxPing %i\n", s_scriptdata.server.maxPing));

	// gameplay
//	AddScript(va("g_gravity %i\n", s_scriptdata.server.gravity));
	AddScript(va("set g_knockback %i\n", s_scriptdata.server.knockback));

	// server frame rate
	AddScript(va("set sv_fps %i\n", s_scriptdata.server.sv_fps));

	// weapon respawn
	if (s_scriptdata.gametype == GT_TEAM)
		AddScript(va("set g_weaponTeamrespawn %i\n", s_scriptdata.server.weaponrespawn));
	else
		AddScript(va("set g_weaponrespawn %i\n", s_scriptdata.server.weaponrespawn));

	// private clients
	if (s_scriptdata.server.allowPrivateClients) {
		AddScript(va("set sv_privateClients %i\n", s_scriptdata.server.privateClients));
		AddScript(va("set sv_privatePassword \"%s\"\n", s_scriptdata.server.privatePassword));
	}

	// netport
	AddScript(va("set net_port %i\n", s_scriptdata.server.netport));

	// allow values to take effect
	AddScript("\nwait 25\n");

	return qtrue;
}




/*
=================
StartServer_GetBotSkillFloat
=================
*/
static float StartServer_GetFracBotSkill(botskill_t* skill) {
   int high, low;
   float width;
   float slope;
   float area, target;
   float result;
   qboolean reverse;

   high = skill->high;
   low = skill->low;
   width = high - low;

   // determine shape of skewed distributions
   reverse = qfalse;
	switch (s_scriptdata.bot.skillBias) {
		case SKILLBIAS_FRAC_VLOW:
         reverse = qtrue;
			slope = 0.5;
			break;
		case SKILLBIAS_FRAC_LOW:
         reverse = qtrue;
			slope = 0.25;
			break;
		case SKILLBIAS_FRAC_HIGH:
			slope = 0.25;
			break;
		case SKILLBIAS_FRAC_VHIGH:
         slope = 0.5;
			break;
		case SKILLBIAS_FRACTIONAL:
		default:
         return (1.0 + low + width * random());
	}

   area = width * width * slope;
   target = area * random();

   result = sqrt(target / slope);
   if (reverse == qtrue) {
      return 1 + high - result;
   }

   return 1 + low + result;
}



/*
=================
StartServer_GetIntBotSkill
=================
*/
static int StartServer_GetIntBotSkill(botskill_t* skill) {
   int i;
   int low, high;
	float value, step;
   float bin[MAX_SKILL];
	float selected;

	low = skill->low;
	high = skill->high;

   // determine shape of skewed distributions
	switch (s_scriptdata.bot.skillBias) {
		case SKILLBIAS_VLOW:
			value = (high - low) + 1.0;
			step = -1.0;
			break;
		case SKILLBIAS_LOW:
			value = (high - low) * 0.5 + 0.5;
			step = -0.5;
			break;
		case SKILLBIAS_HIGH:
			value = 0.5;
			step = 0.5;
			break;
		case SKILLBIAS_VHIGH:
			value = 1.0;
			step = 1.0;
			break;
		case SKILLBIAS_NONE:
		default:
			value = 1.0;
			step = 0.0;
			break;
	}

	for (i = 0; i < MAX_SKILL; i++) {
		if (i < low || i > high)
			bin[i] = 0.0;
		else {
			bin[i] = value;
			value += step;
		}
		if (i > 0)
			bin[i] = bin[i] + bin[i - 1];
	}

	selected = bin[MAX_SKILL - 1] * random();

	for (i = 0; i < MAX_SKILL ; i++)
		if (selected < bin[i])
			return i + 1;

	return MAX_SKILL;
}


/*
=================
StartServer_GetBotSkillInteger
=================
*/
static char* StartServer_GetBotSkill(botskill_t* skill)
{
   static char skill_out[10];

	if (!skill->range) {
      Q_strncpyz(skill_out, va("%i", skill->value + 1), sizeof(skill_out));
      return skill_out;
   }

   // prepare default value
   Q_strncpyz(skill_out, va("%i", skill->low + 1), sizeof(skill_out));
	if (skill->low == skill->high) {
      return skill_out;
   }

	switch (s_scriptdata.bot.skillBias) {
		case SKILLBIAS_FRAC_VLOW:
		case SKILLBIAS_FRAC_LOW:
		case SKILLBIAS_FRAC_HIGH:
		case SKILLBIAS_FRAC_VHIGH:
		case SKILLBIAS_FRACTIONAL:
      {
         float f_skill = StartServer_GetFracBotSkill(skill);
         Q_strncpyz(skill_out, va("%4.2f", f_skill), sizeof(skill_out));
         break;
      }
		case SKILLBIAS_VLOW:
		case SKILLBIAS_LOW:
		case SKILLBIAS_HIGH:
		case SKILLBIAS_VHIGH:
		case SKILLBIAS_NONE:
      {
         int i_skill = StartServer_GetIntBotSkill(skill);
         Q_strncpyz(skill_out, va("%i", i_skill), sizeof(skill_out));
         break;
      }
      default:
         break;
   }

   //trap_Print(va("Bot skill: %s\n", skill_out));
   return skill_out;
}




/*
=================
StartServer_GetPlayerTeam
=================
*/
static int StartServer_GetPlayerTeam( void )
{
	if (s_scriptdata.bot.teamSwapped)
		return 1;
	return 0;
}



/*
=================
StartServer_WriteSelectedBotParams
=================
*/
static void StartServer_WriteSelectedBotParams( void )
{
    int i;
    const char* skill;
    qboolean custom;
    botskill_t* skillrange;

    skillrange = &s_scriptdata.bot.globalSkill;

    custom = qfalse;
    if (s_scriptdata.bot.skillType >= BOTSKILL_CUSTOMSINGLE)
        custom = qtrue;

    if (s_scriptdata.gametype >= GT_TEAM && s_scriptdata.gametype != GT_LMS) {
        // team game

        int playerTeam = StartServer_GetPlayerTeam();
        int otherTeam = 1 - playerTeam;

        for (i = 0; i < PLAYER_SLOTS_PERCOL; i++) {
            if (s_scriptdata.bot.slotType[i] != SLOTTYPE_BOT)
                continue;
            if (s_scriptdata.bot.name[i][0] == '\0')
                continue;

            if (custom)
                skillrange = &s_scriptdata.bot.skill[i];

            skill = StartServer_GetBotSkill(skillrange);

            AddScript(va("addbot %s %s %s; ", s_scriptdata.bot.name[i], skill, bot_teamname[playerTeam]));
        }

        for (i = PLAYER_SLOTS_PERCOL; i < PLAYER_SLOTS; i++) {
            if (s_scriptdata.bot.slotType[i] != SLOTTYPE_BOT)
                continue;
            if (s_scriptdata.bot.name[i][0] == '\0')
                continue;

            if (custom)
                skillrange = &s_scriptdata.bot.skill[i];

            skill = StartServer_GetBotSkill(skillrange);

            AddScript(va("addbot %s %s %s; ", s_scriptdata.bot.name[i], skill, bot_teamname[otherTeam]));
        }
    }
    else {
        // single player
        for (i = 0; i < PLAYER_SLOTS; i++) {
            if (s_scriptdata.bot.slotType[i] != SLOTTYPE_BOT)
                continue;

            if (s_scriptdata.bot.name[i][0] == '\0')
                continue;

            if (custom)
                skillrange = &s_scriptdata.bot.skill[i];

            skill = StartServer_GetBotSkill(skillrange);

            AddScript(va("addbot %s %s; ", s_scriptdata.bot.name[i], skill));
        }
    }
}



/*
=================
StartServer_RejectRandomBot
=================
*/
static qboolean StartServer_RejectRandomBot( int newbot, int* botlist, int max )
{
	int i;
	char botname[MAX_NAME_LENGTH];
	char* bot;

	for (i = 0 ; i < max; i++)
	{
		if (newbot == botlist[i])
			return qtrue;
	}

	// compare only if we have an exclude list
	if (StartServer_IsRandomBotExclude(s_scriptdata.bot.typeSelect))
	{
		bot = UI_GetBotInfoByNumber( newbot );
		Q_strncpyz(botname, Info_ValueForKey(bot, "name"), MAX_NAME_LENGTH);

		for (i = 0; i < PLAYER_SLOTS; i++)
			if (!Q_stricmp(s_scriptdata.bot.name[i], botname))
				return qtrue;
	}

	return qfalse;
}


/*
=================
StartServer_GenerateBotList
=================
*/
static void StartServer_GenerateBotList( int* botlist, int listmax, int listnum )
{
	int count, i, j, newbot;
	qboolean repeat;

	for (i = 0; i < listnum; i++) {
		// find an unused bot number, but don't try too hard
		count = 0;
		do {
			repeat = qfalse;
			newbot = Clamp_Random( listmax - 1 );
			if (StartServer_RejectRandomBot(newbot, botlist, i))
			{
				repeat = qtrue;
			}
			count++;
		} while (repeat && count < 32);

		botlist[i] = newbot;
	}
}



/*
=================
StartServer_WriteBotList
=================
*/
static void StartServer_WriteBotList( int* botlist, int listnum, qboolean kick )
{
	int i;
   const char* skill;
	char* bot;
	char* funname;
	char* spacechar;
	char botname[MAX_NAME_LENGTH];
	botskill_t* skillrange;
	int team, player_team;

	skillrange = &s_scriptdata.bot.globalSkill;
	player_team = StartServer_GetPlayerTeam();
	for (i = 0; i < listnum; i++) {
		bot = UI_GetBotInfoByNumber( botlist[i] );
		Q_strncpyz(botname, Info_ValueForKey(bot, "name"), MAX_NAME_LENGTH);

		if (kick) {
			// kicking is based on the funname, if available
			funname = Info_ValueForKey(bot, "funname");
			if (funname[0])
			{
				Q_strncpyz(botname, funname, MAX_NAME_LENGTH);
				Q_CleanStr(botname);
				Q_strncpyz(botname, va("\"%s\"", botname), MAX_NAME_LENGTH);
			}

			AddScript(va("kick %s; ", botname));
		}
		else {
			skill = StartServer_GetBotSkill(skillrange);
			if (s_scriptdata.gametype >= GT_TEAM) {
				team = (player_team + i + 1) % 2;	// start with opposite team

				if (i == listnum - 1 && team == player_team)
					team = 1 - team;	// even number of bots: put last one on opposite team

				AddScript(va("addbot %s %s %s; ", botname, skill, bot_teamname[ team ]));
			}
			else {
				AddScript(va("addbot %s %s; ", botname, skill));
			}
		}
	}
}



/*
=================
StartServer_WriteRandomBotParams
=================
*/
static void StartServer_WriteRandomBotParams( void )
{
	int i, j;
	int max, index, period, next;
	int frequency, bots_perlevel;
	int* kickbots;
	int* addbots;

	frequency = botChange_frequency[ s_scriptdata.bot.changeBots ];

	bots_perlevel = s_scriptdata.bot.numberBots;
	if (bots_perlevel > s_serverexec.maxBots)
		bots_perlevel = s_serverexec.maxBots;

	if (frequency == 0 || bots_perlevel == 0) {	// one-off addbot
		// write kickbot (never kicks a bot, loops forever)
		AddScript("set "SERVER_KICKBOT"0 \"set "SERVER_KICKBOT" vstr "SERVER_KICKBOT"0\"\n");

		// write first addbot
		AddScript("set "SERVER_ADDBOT"0 \"");

		if (bots_perlevel > 0) {
			StartServer_GenerateBotList(s_serverexec.usedBots, s_serverexec.maxBots, bots_perlevel);
			StartServer_WriteBotList(s_serverexec.usedBots, bots_perlevel, qfalse);
		}

		AddScript("set "SERVER_ADDBOT" vstr "SERVER_ADDBOT"1\"\n");

		// write second addbot
		// does nothing
		AddScript("set "SERVER_ADDBOT"1 \"\"\n");
	}
	else {
		// bots that we need to delete when loop comes around again
		StartServer_GenerateBotList(s_serverexec.lastBots, s_serverexec.maxBots, bots_perlevel);

		// to minimize the overlap and repeat of bot/map combinations
		// the period of bot repeat is a prime number > than the number
		// of maps
		period = addbot_primes[ num_addbot_primes - 1 ];
		for (i = 0; i < num_addbot_primes; i++) {
			if (addbot_primes[i] > s_serverexec.cycle_count) {
				period = addbot_primes[i];
				break;
			}
		}

		// write the list
		index = 0;
		max = frequency * period;
		kickbots = s_serverexec.lastBots;
		addbots = s_serverexec.usedBots;
		for (i = period - 1; i >= 0; i--) {
			next = (index + 1)%max;

			// kick the previous bots
			AddScript(va("set "SERVER_KICKBOT"%i \"", index));
			if (bots_perlevel > 0) {
				StartServer_WriteBotList(kickbots, bots_perlevel, qtrue);
			}

			AddScript(va("set "SERVER_KICKBOT" vstr "SERVER_KICKBOT"%i\"\n", next));

			// add the bots
			AddScript(va("set "SERVER_ADDBOT"%i \"", index));
			if (bots_perlevel > 0) {
				if (i == 0) {
					// make sure we add the correct bots that we
					// kicked at the start of the cycle
					addbots = s_serverexec.lastBots;
				}
				else {
					StartServer_GenerateBotList(addbots, s_serverexec.maxBots, bots_perlevel);
				}
				StartServer_WriteBotList(addbots, bots_perlevel, qfalse);
				kickbots = s_serverexec.usedBots;
			}

			AddScript(va("set "SERVER_ADDBOT" vstr "SERVER_ADDBOT"%i\"\n\n", next));

			// pad out so new bots are kicked at the correct number of maps
			for (j = 1; j < frequency; j++) {
				index++;
				next = (index + 1)%max;

				AddScript(va("set "SERVER_KICKBOT"%i \"set "SERVER_KICKBOT" vstr "SERVER_KICKBOT"%i\"\n", index, next));
				AddScript(va("set "SERVER_ADDBOT"%i \"set "SERVER_ADDBOT" vstr "SERVER_ADDBOT"%i\"\n\n", index, next));
			}

			index++;
		}

	}
}



/*
=================
StartServer_GenerateSelectedArenaBotList
=================
*/
static void StartServer_GenerateSelectedArenaBotList(int* bots, int botnum) {
	int count, i, j, newbot;
	qboolean repeat;

	for (i = 0; i < botnum; i++) {
		// find an unused bot number, but don't try too hard
		count = 0;
		do {
			repeat = qfalse;
			newbot = s_serverexec.bots_in_slots[Clamp_Random( s_serverexec.num_bots_in_slots - 1 )];
			if (StartServer_RejectRandomBot(newbot, bots, i))
			{
				repeat = qtrue;
			}
			count++;
		} while (repeat && count < 32);

		bots[i] = newbot;
	}
}



/*
=================
StartServer_GenerateArenaBotList
=================
*/
static int StartServer_GenerateArenaBotList( int* bots, int max, const char* mapname )
{
	const char* info;
	const char* botinfo;
	char* p;
	const char* botname;
	int num;
	int botnum;
	char botlist[MAX_INFO_STRING];

	info = UI_GetArenaInfoByMap(mapname);

	num = 0;

	// we're counting up the number of bots for this level,
	// as well as parsing the bot list
	if (info) {
		Q_strncpyz(botlist, Info_ValueForKey(info, "bots"), MAX_INFO_STRING);
		if (*botlist) {
			// arena has bots, search through them
			p = botlist;
			while ( num < max) {
				botname = COM_Parse(&p);
				if (!botname || !botname[0])
					break;

				botnum = UI_GetBotNumByName(botname);
				if (botnum < 0)
					continue;

				// found the bot, get its index
				bots[num] = botnum;
				num++;
			}

			// only return if we use the parsed results
			if (s_scriptdata.bot.typeSelect == BOTTYPE_ARENASCRIPT)
				return num;
		}
	}

	// no bots on map, so generate a random number
	if (num == 0)
		num = s_scriptdata.bot.numberBots;

	if (num > max)
		num = max;

   // now build the list of bots
   if (s_scriptdata.bot.typeSelect == BOTTYPE_SELECTARENASCRIPT) {
      if (s_serverexec.num_bots_in_slots > 0) {
         StartServer_GenerateSelectedArenaBotList(bots, num);
         return num;
      }
   }

   // list built is randomly generated
	if (num)
		StartServer_GenerateBotList(bots, UI_GetNumBots(), num);

	return num;
}





/*
=================
StartServer_WriteArenaScriptBotParams

Includes awareness of map ordering and random map generation
Copied into static buffer for safe keeping until next call
=================
*/
static char* StartServer_GetArenaScriptMapName( int index )
{
	static char mapname[SHORTMAP_BUFFER];
	char* map_ptr;
	const char* info;
	int mapnum;

	if (index < 0)
		return NULL;

	if (s_serverexec.random_order) {
		// a random order was imposed on the map
		index %= s_serverexec.random_count;

		if (s_serverexec.random_generate) {
			// the map list was also randomly generated
			mapnum = s_serverexec.map_rnd_index[ s_serverexec.map_rnd[index] ];
			info = UI_GetArenaInfoByNumber(mapnum);
			map_ptr = Info_ValueForKey(info, "map");
		}
		else {
			// random order on a defined list of maps
			map_ptr = s_scriptdata.map.data[ s_serverexec.map_rnd[index] ].shortName;
		}
	}
	else {
		// we used a linear list
		map_ptr = s_scriptdata.map.data[index].shortName;
	}

	// copy into local storage
	if (map_ptr && *map_ptr) {
		Q_strncpyz(mapname, map_ptr, SHORTMAP_BUFFER);
	}
	else {
		mapname[0] = '\0';
	}

	return mapname;
}



/*
=================
StartServer_WriteArenaScriptBotParams

Includes awareness of map ordering and random map generation
=================
*/
static void StartServer_WriteArenaScriptBotParams( void )
{
	int i;
	int count, period;
	int bots_onlevel, lastlevel_numbots;
	char *mapname;
	int *kickbots;
	int *addbots;
	int next;

	period = s_serverexec.cycle_count;
	count = period;
	if (s_serverexec.random_order) {
		count = s_serverexec.random_count;
	}

	// bots that we need to delete when loop comes around again
	mapname = StartServer_GetArenaScriptMapName(count - 1);
	lastlevel_numbots = StartServer_GenerateArenaBotList(s_serverexec.lastBots, s_serverexec.maxBots, mapname);

	s_serverexec.max_scripted_bots = lastlevel_numbots;

	// write the list
	kickbots = s_serverexec.lastBots;
	addbots = s_serverexec.usedBots;
	bots_onlevel = lastlevel_numbots;
	for (i = 0; i < count; i++) {
		next = (i + 1)%period;

		// kick the previous bots
		AddScript(va("set "SERVER_KICKBOT"%i \"", i));
		if (bots_onlevel > 0) {
			StartServer_WriteBotList(kickbots, bots_onlevel, qtrue);
		}

		AddScript(va("set "SERVER_KICKBOT" vstr "SERVER_KICKBOT"%i\"\n", next));

		// add the bots
		AddScript(va("set "SERVER_ADDBOT"%i \"", i));

		if (i == count - 1) {
			// make sure we add the correct bots that we
			// kicked at the start of the cycle
			addbots = s_serverexec.lastBots;
			bots_onlevel = lastlevel_numbots;
		}
		else {
			mapname = StartServer_GetArenaScriptMapName(i);
			bots_onlevel = StartServer_GenerateArenaBotList(addbots, s_serverexec.maxBots, mapname);
		}

		if (i == 0)
			s_serverexec.botcount_firstmap = bots_onlevel;

		if (bots_onlevel > s_serverexec.max_scripted_bots)
			s_serverexec.max_scripted_bots = bots_onlevel;

		StartServer_WriteBotList(addbots, bots_onlevel, qfalse);
		kickbots = s_serverexec.usedBots;

		AddScript(va("set "SERVER_ADDBOT" vstr "SERVER_ADDBOT"%i\"\n\n", next));
	}
}





/*
=================
StartServer_WriteBotParams

May depend on map ordering list, so StartServer_WriteMapParams()
must be called first
=================
*/
static qboolean StartServer_WriteBotParams( void )
{
	AddScript("\n// WriteBotParams()\n\n");

	if (!s_serverexec.map_generated) {
		trap_Print(S_COLOR_RED"WriteBotParams called before WriteMapParams\n");
		return qfalse;
	}

   // build list from selected bots
	if (s_scriptdata.bot.typeSelect == BOTTYPE_SELECT)
	{
		// bots are only added once

		AddScript("// allows 'reset "SERVER_ADDBOT"' to clear "SERVER_ADDBOT"\n");
		AddScript("set "SERVER_ADDBOT" \"\"\n");
		AddScript("set "SERVER_ADDBOT" \"");

		StartServer_WriteSelectedBotParams();

		// no more bots added, so clear addbot
		// last command in set SERVER_ADDBOT
		AddScript("reset "SERVER_ADDBOT" \"\n");

		// bots never kicked
		AddScript("set "SERVER_KICKBOT" \"\"\n");

		s_serverexec.bot_generated = qtrue;
		return qtrue;
	}

   if (StartServer_IsBotArenaScript(s_scriptdata.bot.typeSelect)) {
		StartServer_WriteArenaScriptBotParams();
	}
	else {	// BOTTYPE_RANDOM or BOTTYPE_RANDOMEXCLUDE
		StartServer_WriteRandomBotParams();
	}

	// setup bot add and kick commands
	// we don't need to kick any bots on first connect
	AddScript("\nset "SERVER_ADDBOT" \"vstr "SERVER_ADDBOT"0\"\n");
	AddScript("set "SERVER_KICKBOT" \"set "SERVER_KICKBOT" vstr "SERVER_KICKBOT"1\"\n");

	s_serverexec.bot_generated = qtrue;
	return qtrue;
}




/*
=================
StartServer_IsRecentMap
=================
*/
static qboolean StartServer_IsRecentMap(int count, int index)
{
	int i;
	int start;
	int period;

	period = s_serverexec.rnd_nummaps;
	// no previous entries, or not enough maps
	if (count == 0 || period < 2)
		return qfalse;

	// must have used about half of maps before a repeat is allowed
	start = count - (period / 2);
	if (start < 0)
		start = 0;

	for (i = start; i < count; i++)
		if (s_serverexec.map_rnd_index[i] == index)
			return qtrue;

	return qfalse;
}



/*
=================
StartServer_IsBiasedMap
=================
*/
static qboolean StartServer_IsBiasedMap(int idtype, int index)
{
	const char* info;
	qboolean idmap;

	if (idtype != MAP_RND_BIASID && idtype != MAP_RND_BIASNONID)
		return qfalse;

	info = UI_GetArenaInfoByNumber(index);
	idmap = StartServer_IsIdMap(Info_ValueForKey(info, "map"));
	if (idtype == MAP_RND_BIASID && idmap)
		return qfalse;
	if (idtype == MAP_RND_BIASNONID && !idmap)
		return qfalse;

	if (random() < 0.25)
		return qfalse;

	return qtrue;		
}




/*
=================
StartServer_ExcludeMap_callback
=================
*/
static qboolean StartServer_ExcludeMap_callback(const char* info)
{
	int i;
	const char* mapname;
	int idtype;
	int subtype;
	qboolean idmap;

	subtype = s_scriptdata.map.SourceType;
	if (subtype < MAP_RND_MAX)
	{
		idtype = subtype;
		subtype = -1;
	}
	else
	{
		idtype = MAP_RND_ANY;
		subtype -= MAP_RND_MAX;
	}

	mapname = Info_ValueForKey(info, "map");
	if (!StartServer_MapSupportsBots(mapname))
		return qfalse;

	idmap = StartServer_IsIdMap(mapname);
	if (idmap && (idtype == MAP_RND_NONID))
		return qfalse;
	if (!idmap && (idtype == MAP_RND_ID))
		return qfalse;

	if (subtype >=0 && !StartServer_IsCustomMapType(mapname, subtype))
		return qfalse;

	if (s_scriptdata.map.listSource != MAP_MS_RANDOMEXCLUDE)
		return qtrue;	

	// check if on exclude list
	for (i = 0; i < s_scriptdata.map.num_maps; i++)
		if (!Q_stricmp(mapname, s_scriptdata.map.data[i].shortName))
			return qfalse;

	return qtrue;
}




/*
=================
StartServer_GenerateRandomMaps
=================
*/
static qboolean StartServer_GenerateRandomMaps(int count)
{
	int i, j;
	int map;
	int failed;
	int idtype;
	int maplist[MAX_MAPS_LIST * 2];
	callbackMapList callback;

	idtype = s_scriptdata.map.SourceType;
	if (idtype >= MAP_RND_MAX)
	{
		idtype = MAP_RND_ANY;
	}

	callback = StartServer_ExcludeMap_callback;
	s_serverexec.rnd_nummaps = UI_BuildMapListByType(
		maplist, MAX_MAPS_LIST * 2, s_scriptdata.gametype, callback);

	if (s_serverexec.rnd_nummaps == 0)
	{
		StartServer_PrintMessage( "No random maps available");
		return qfalse;
	}

	// try to add the map, but not too hard
	for (i = 0; i < count; i++)
	{
		failed = 0;
		do {
			map = Clamp_Random(s_serverexec.rnd_nummaps - 1 );
			if (StartServer_IsBiasedMap(idtype, maplist[map]))
				continue;

		} while (StartServer_IsRecentMap(i, maplist[map]) && failed++ < 32);

		s_serverexec.map_rnd_index[i] = maplist[map];
	}

	return qtrue;
}



/*
=================
StartServer_GenerateRandomMapOrder
=================
*/
static void StartServer_GenerateRandomMapOrder( int count, int avoid, int* map_rnd )
{
	int i, repeat;
	int src, dest, value;

	for (i = 0; i < count; i++)
		map_rnd[i] = i;

	repeat = 0;
	do {
		// do shuffle
		for (i = 0; i < 2 * count; i++) {
			src = Clamp_Random(count - 1);
			dest = Clamp_Random(count - 1);
			if (src != dest) {
				value = map_rnd[src];
				map_rnd[src] = map_rnd[dest];
				map_rnd[dest] = value;
			}
		}
		repeat++;
		// avoid matching first value
		// ensure there might be a different map,
		// and that we don't try too hard
	} while (avoid == map_rnd[0] && count > 1 && repeat < 32);
}






/*
=================
StartServer_WriteMapParams
=================
*/
static qboolean StartServer_WriteMapParams( void )
{
	int i, value, count, index, lastmap;
	int source;
	int fraglimit, timelimit;
	const char* info;
	const char* mapname;
	qboolean random_order;
	qboolean random_generate;

	AddScript("\n// WriteMapParams()\n\n");

	fraglimit = s_scriptdata.map.fragLimit;
	timelimit = s_scriptdata.map.timeLimit;

	if (s_scriptdata.map.fragLimitType == MAP_LT_NONE)
		fraglimit = 0;
	if (s_scriptdata.map.timeLimitType == MAP_LT_NONE)
		timelimit = 0;

	// set for unlimited frags
	if (UIE_IsCaptureLimited(s_scriptdata.gametype))
		AddScript("fraglimit 0\n");

	source = s_scriptdata.map.listSource;
	count = s_serverexec.cycle_count;

	random_generate = StartServer_IsRandomGeneratedMapList(source);
	random_order = (source == MAP_MS_RANDOMLIST || random_generate);
	if (random_order)
	{
		// the internal Q3 exe script buffer is limited to 16K.
		// Empirical evidence indicate problems start to happen at about
		// 80-100 randomly ordered map rotations, so we clip the values
		// and keep then under 100
		if (count < 20)
			count *= 4;
		else if (count < 40)
			count *= 2;

		if (count > MAX_MAPS_LIST)
			count = MAX_MAPS_LIST;
	}

	s_serverexec.random_generate = random_generate;
	s_serverexec.random_order = random_order;
	s_serverexec.random_count = count;

	if (StartServer_IsRandomGeneratedMapList(source) && !StartServer_GenerateRandomMaps(s_serverexec.cycle_count))
		return qfalse;

	// generate the random order before we do the output
	if (random_order) {
		lastmap = -1;
		for (i = 0; i < count; i += s_serverexec.cycle_count) {
			if (i > 0)
				lastmap = s_serverexec.map_rnd[ i - 1 ];
			StartServer_GenerateRandomMapOrder(s_serverexec.cycle_count, lastmap, &s_serverexec.map_rnd[i]);
		}
	}

	// build the script
	for (i = 0; i < count; i++) {
		// we start with the "in order" index
		index = i % s_serverexec.cycle_count;

		// now adjust the index for a random map order
		if (random_order) {
			if ( index == 0 && i > 0)
				AddScript("\n");

			index = s_serverexec.map_rnd[i];
		}

		// create the variable in the script
		AddScript(va("set m%i \"", i));

		// find and error check the map limits
		if (s_scriptdata.map.fragLimitType == MAP_LT_CUSTOM)
			fraglimit = atoi(s_scriptdata.map.data[index].fragLimit);
		if (s_scriptdata.map.timeLimitType == MAP_LT_CUSTOM)
			timelimit = atoi(s_scriptdata.map.data[index].timeLimit);

		if (timelimit == 0 && fraglimit == 0 && s_serverexec.cycle_count > 1)
			timelimit = 30;

		// write the map limits
		AddScript(va("timelimit %i; ", timelimit));

		if (UIE_IsCaptureLimited(s_scriptdata.gametype))
			AddScript(va("capturelimit %i; ", fraglimit));
		else
			AddScript(va("fraglimit %i; ", fraglimit));

		if (s_scriptdata.server.preventConfigBug)
			AddScript("writeconfig q3config; ");

		// write map name
		if (random_generate) {
			info = UI_GetArenaInfoByNumber(s_serverexec.map_rnd_index[index]);
			mapname = Info_ValueForKey(info, "map");
		}
		else {
			mapname = s_scriptdata.map.data[index].shortName;
		}
		
		if (s_scriptdata.gametype != GT_SINGLE){
		AddScript("set cl_sprun 0; ");
		AddScript(va("map %s; ", mapname));
		} else {
		AddScript("set cl_sprun 1; ");
		AddScript(va("spmap %s; ", mapname));
		}

		// sort the bots out
		AddScript("vstr "SERVER_KICKBOT"; vstr "SERVER_ADDBOT"; ");

		// set gravity if overriding default
		if (s_scriptdata.server.gravity != 800)
			AddScript(va("g_gravity %i; ", s_scriptdata.server.gravity));
		
		// newcvars
			AddScript(va("g_jumpheight %i; ", s_scriptdata.server.jumpheight));

		// terminate string
		value = (i + 1) % count;
		if (value == 0 && !s_scriptdata.map.Repeat) {
			// don't loop map list
			AddScript("\"\n");
			continue;
		}
		AddScript(va("set nextmap vstr m%i\"\n", value));
	}

	s_serverexec.map_generated = qtrue;
	return qtrue;
}

/*
=================
LoadServer_WriteMapParams
=================
*/
static qboolean LoadServer_WriteMapParams( void )
{
	int i, value, count, index, lastmap;
	int source;
	int fraglimit, timelimit;
	const char* info;
	const char* mapname;
	qboolean random_order;
	qboolean random_generate;

	AddScript("\n// WriteMapParams()\n\n");

	fraglimit = s_scriptdata.map.fragLimit;
	timelimit = s_scriptdata.map.timeLimit;

	if (s_scriptdata.map.fragLimitType == MAP_LT_NONE)
		fraglimit = 0;
	if (s_scriptdata.map.timeLimitType == MAP_LT_NONE)
		timelimit = 0;

	// set for unlimited frags
	if (UIE_IsCaptureLimited(s_scriptdata.gametype))
		AddScript("fraglimit 0\n");

	source = s_scriptdata.map.listSource;
	count = s_serverexec.cycle_count;

	random_generate = StartServer_IsRandomGeneratedMapList(source);
	random_order = (source == MAP_MS_RANDOMLIST || random_generate);

	s_serverexec.random_generate = random_generate;
	s_serverexec.random_order = random_order;
	s_serverexec.random_count = count;

	// build the script
	for (i = 0; i < 1; i++) {
		// we start with the "in order" index
		index = i % s_serverexec.cycle_count;

		// create the variable in the script
		AddScript(va("set m%i \"", i));

		// find and error check the map limits
		if (s_scriptdata.map.fragLimitType == MAP_LT_CUSTOM)
			fraglimit = atoi(s_scriptdata.map.data[index].fragLimit);
		if (s_scriptdata.map.timeLimitType == MAP_LT_CUSTOM)
			timelimit = atoi(s_scriptdata.map.data[index].timeLimit);

		if (timelimit == 0 && fraglimit == 0 && s_serverexec.cycle_count > 1)
			timelimit = 30;

		// write the map limits
		AddScript(va("timelimit %i; ", timelimit));

		if (UIE_IsCaptureLimited(s_scriptdata.gametype))
			AddScript(va("capturelimit %i; ", fraglimit));
		else
			AddScript(va("fraglimit %i; ", fraglimit));

		if (s_scriptdata.server.preventConfigBug)
			AddScript("writeconfig q3config; ");

		// write map name
		if (random_generate) {
			info = UI_GetArenaInfoByNumber(s_serverexec.map_rnd_index[index]);
			mapname = Info_ValueForKey(info, "map");
		}
		else {
			mapname = s_scriptdata.map.data[index].shortName;
		}
		
		if (s_scriptdata.gametype != GT_SINGLE){
		AddScript(va("map %s; ", UI_Cvar_VariableString( "save_curmap" )));
		} else {
		AddScript(va("spmap %s; ", UI_Cvar_VariableString( "save_curmap" )));
		}

		// sort the bots out
		AddScript("vstr "SERVER_KICKBOT"; vstr "SERVER_ADDBOT"; ");

		// set gravity if overriding default
		if (s_scriptdata.server.gravity != 800)
			AddScript(va("g_gravity %i; ", s_scriptdata.server.gravity));
		
		// newcvars
			AddScript(va("g_jumpheight %i; ", s_scriptdata.server.jumpheight));

		// terminate string
		value = (i + 1) % count;
		if (value == 0 && !s_scriptdata.map.Repeat) {
			// don't loop map list
			AddScript("\"\n");
			continue;
		}
		AddScript(va("set nextmap vstr m%i\"\n", value));
	}

	s_serverexec.map_generated = qtrue;
	return qtrue;
}



/*
=================
StartServer_WriteItemParams
=================
*/
static qboolean StartServer_WriteItemParams( void )
{
	int 		i, j;
	int 		type;
	qboolean	disable;

	AddScript("\n// WriteItemParams()\n\n");

	// write out the disabled items list
	for (i = 0; i < ITEM_COUNT; i++)
	{
		disable = qfalse;
		type = s_scriptdata.item.groupstate[ server_itemlist[i].groupid ];
		if (type == ALLGROUPS_HIDDEN) {
			disable = qtrue;
		}	
		else if (type == ALLGROUPS_CUSTOM) {
			disable = qtrue;
			if (s_scriptdata.item.enabled[i])
				disable = qfalse;
		}

		AddScript(va("set disable_%s %i\n", server_itemlist[i].mapitem, disable));
	}

	return qtrue;
}



static	char	uie_token[MAX_TOKEN_CHARS];
static	int		uie_lines;


/*
=================
UIE_Parse

Custom version of COM_ParseExt() that ignores // comments
We shouldn't modify the version in q_shared.c
=================
*/
char *UIE_Parse( char **data_p)
{
	int c = 0, len;
	int cc;
	qboolean hasNewLines = qfalse;
	char *data;
	qboolean allowLineBreaks;

	allowLineBreaks = qtrue;
	data = *data_p;
	len = 0;
	uie_token[0] = 0;

	// make sure incoming data is valid
	if ( !data )
	{
		*data_p = NULL;
		return uie_token;
	}

	while ( 1 )
	{
		// skip whitespace

		while( (cc = *data) <= ' ') {
			if( !cc ) {
				data = NULL;
				break;
			}
			if( cc == '\n' ) {
				uie_lines++;
				hasNewLines = qtrue;
			}
			data++;
		}

		if ( !data )
		{
			*data_p = NULL;
			return uie_token;
		}
		if ( hasNewLines && !allowLineBreaks )
		{
			*data_p = data;
			return uie_token;
		}

		c = *data;

		// skip /* */ comments
		if ( c=='/' && data[1] == '*' )
		{
			data += 2;
			while ( *data && ( *data != '*' || data[1] != '/' ) )
			{
				data++;
			}
			if ( *data )
			{
				data += 2;
			}
		}
		else
		{
			break;
		}
	}

	// handle quoted strings
	if (c == '\"')
	{
		data++;
		while (1)
		{
			c = *data++;
			if (c=='\"' || !c)
			{
				uie_token[len] = 0;
				*data_p = ( char * ) data;
				return uie_token;
			}
			if (len < MAX_TOKEN_CHARS)
			{
				uie_token[len] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if (len < MAX_TOKEN_CHARS)
		{
			uie_token[len] = c;
			len++;
		}
		data++;
		c = *data;
		if ( c == '\n' )
			uie_lines++;
	} while (c>32);

	if (len == MAX_TOKEN_CHARS)
	{
//		Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}
	uie_token[len] = 0;

	*data_p = ( char * ) data;
	return uie_token;
}




/*
=================
StartServer_LoadFromConfig
=================
*/
qboolean StartServer_LoadFromConfig( const char* filename)
{
	fileHandle_t 	handle;
	char			cvar[CVAR_BUFFER];
	int 			len;
	char* 			begin;
	char* 			end;
	char*			ptr;
	char* 			token;
	int 			gametype;

	// load the script, re-use the server script buffer
	len = trap_FS_FOpenFile(filename, &handle, FS_READ);
	if (len <= 0)
	{
		Com_Printf("Config file not found: %s\n", filename);
		return qfalse;
	}

    UIE_StartServer_LoadSkirmishCvars();

	if (len >= SCRIPT_BUFFER)
		len = SCRIPT_BUFFER - 1;

	trap_FS_Read(s_serverexec.server_script, len, handle);
	s_serverexec.server_script[len] = '\0';
	trap_FS_FCloseFile(handle);

	// find the begin and end of block terminators
	begin = strstr(s_serverexec.server_script, BEGIN_PARAMETER_BLOCK);
	end = strstr(s_serverexec.server_script, END_PARAMETER_BLOCK);

	if (begin == 0) {
		StartServer_PrintMessage(va("Config file %s has no BEGIN block\n", filename));
		return qfalse;
	}

	if (end == 0) {
		StartServer_PrintMessage(va("Config file %s has no END block\n", filename));
		return qfalse;
	}

	if (begin > end) {
		StartServer_PrintMessage(va("Config file %s has bad parameter block\n", filename));
		return qfalse;
	}

	// scan in parameters
	// first token will be BEGIN, after that we have
	// Cvar/value pairs until we hit the END block
	ptr = begin;
	gametype = -1;
	COM_Parse(&ptr);	// drop BEGIN

	do {
		// grab Cvar name
		token = UIE_Parse(&ptr);
		if (!token) {
			// should never happen, unless tampered with
			StartServer_PrintMessage(va("Unexpected end of %s, possible corruption or tampering\n", filename));
			return qtrue;	// update anyway... fingers crossed
		}

		// backward compatibility with earlier cfg save data format
		if (strlen(token) > 2 && token[0] == '/' && token[1] == '/') {
			token += 2;
		}

		if (!Q_stricmp(token, END_PARAMETER_BLOCK))
			break;

		if (*token == '*') {
			if (gametype == -1) {
				StartServer_PrintMessage(va("Unknown gametype from %s, possible corruption or tampering\n", filename));

				// restore last saved cvar list
				UIE_StartServer_LoadSkirmishCvars();
				return qfalse;	// don't update
			}
			Q_strncpyz(cvar, va(gametype_cvar_base[gametype], token + 1), CVAR_BUFFER);
		}
		else
			Q_strncpyz(cvar, token, CVAR_BUFFER);

		// grab value
		token = UIE_Parse(&ptr);
		UIE_ConvertOldCvar(cvar, CVAR_BUFFER);

		if (uis.debug)
			Com_Printf("Cvar: %s=%s\n", cvar, token);

		if (UIE_SkirmishCvarExists(NULL, cvar)) {
			UIE_SetSkirmishCvar(NULL, cvar, token);
		}

		// must get gametype before we can process "*cvarName" Cvars
		if (!Q_stricmp(cvar,"uie_gametype")) {
			gametype = (int)Com_Clamp(0, MAX_GAME_TYPE - 1, atoi(token));
		}

	} while (qtrue);

	UIE_StartServer_SaveSkirmishCvars();
	return qtrue;
}




/*
=================
StartServer_WriteScriptCvar
=================
*/
static void StartServer_WriteScriptCvar( const char* cvarTemplate )
{
	char 	buffer[MAX_INFO_STRING];
	const char*	cvar;
	char* 	s;

	if (!cvarTemplate || !*cvarTemplate)
		return;

	if (*cvarTemplate == '*') {
		cvar = cvarTemplate + 1;
		s = gametype_cvar_base[s_scriptdata.gametype];
	}
	else {
		cvar = cvarTemplate;
		s = NULL;
	}

	if (UIE_GetSkirmishCvar(s, cvar, buffer, MAX_INFO_STRING)) {
		AddScript(va("//%s \"%s\"\n", cvarTemplate, buffer));
	}
}



/*
=================
StartServer_WriteScriptParams
=================
*/
static qboolean StartServer_WriteScriptParams( void )
{
	const char*	cvar;
	int  	i, count;
	char*	s;

	AddScript("\n// WriteScriptParams()\n\n");

	AddScript("// This block of data allows UIE to load\n");
	AddScript("// this script back into the map/bot setup pages.\n");
	AddScript("// Don't edit the contents!!!\n");
	AddScript("//"BEGIN_PARAMETER_BLOCK"\n");

    UIE_StartServer_LoadSkirmishCvars();

	// save the common parameters
	for (i = 0; saveparam_list[i]; i++) {
		StartServer_WriteScriptCvar(saveparam_list[i]);
	}

	// close off the block
	AddScript("//"END_PARAMETER_BLOCK"\n\n");
	return qtrue;
}



/*
=================
StartServer_ExecuteScript
=================
*/
static qboolean StartServer_ExecuteScript(const char* scriptFile)
{
	fileHandle_t f;
	int len;

	if (!s_serverexec.server_script[0])
		return qfalse;

	if (scriptFile) {
		len = strlen( s_serverexec.server_script );
		trap_FS_FOpenFile(scriptFile, &f, FS_WRITE);
		if ( !f) {
			StartServer_PrintMessage( va( "Can't open file (%s)", scriptFile) );
			return qfalse;
		}

		trap_FS_Write( s_serverexec.server_script, len, f );

		trap_FS_FCloseFile( f );
	}
	else {
		trap_Cmd_ExecuteText( EXEC_APPEND, s_serverexec.server_script );
	}

	return qtrue;
}



/*
=================
StartServer_CreateServer

Data for generating script must already be loaded and initialized
=================
*/
qboolean StartServer_CreateServer(const char* scriptFile)
{
	int i;
	float f;
	char* teamjoin;

//	StartServer_InitScriptData();

	// shift random number generator
	// otherwise we see identical map and bot rotations

	for (i = uis.realtime % 200; i; i--)
		f = random();

	if (!StartServer_InitServerExec())
		return qfalse;

	if (scriptFile) {
		AddScript("// Script generated by \"QSandbox\" " QS_VERSION "\n");
	}

	//
	// script recovery parameters
	//
	if (scriptFile && !StartServer_WriteScriptParams())
		return qfalse;

	//
	// item parameters
	//
	if (!StartServer_WriteItemParams())
		return qfalse;

	//
	// map parameters
	//
	if (!StartServer_WriteMapParams())
		return qfalse;

	//
	// bot parameters
	//

	// bot parameters written may depend on map ordering
	if (!StartServer_WriteBotParams())
		return qfalse;

	//
	// server parameters
	//

	// server parameters may depend on bot generation
	if (!StartServer_WriteServerParams())
		return qfalse;

	//
	// force player to join team/specs
	//
	if (s_scriptdata.server.dedicatedServer == SRVDED_OFF) {
		if (scriptFile)
			AddScript("\n// Things that need to be done when first map starts\n");

		AddScript("set activeAction \"");

		teamjoin = "forceteam %i free";
		if (s_scriptdata.bot.joinAs) {
			teamjoin = "forceteam %i spectator";
		}
		else if (s_scriptdata.gametype >= GT_TEAM) {
			// use %%i so a single %i is generated for player client
			teamjoin = va("forceteam %%i %s", bot_teamname[StartServer_GetPlayerTeam()]);
		}
		else if (s_scriptdata.gametype == GT_TOURNAMENT) {
			for (i = 0; i < s_serverexec.player_client; i++)
				AddScript(va("forceteam %i spectator;", i));
		}

		AddScript(va(teamjoin, s_serverexec.player_client));
		AddScript("\"\n");
	}

	// start the map rotation running
	AddScript("\n");
	if (scriptFile)
		AddScript("// start map rotation\n");

	AddScript("vstr m0\n");

	//
	// execute!
	//
	return StartServer_ExecuteScript( scriptFile );
}

/*
=================
StartServer_LoadServer

Data for generating script must already be loaded and initialized
=================
*/
qboolean StartServer_LoadServer(const char* scriptFile)
{
	int i;
	float f;
	char* teamjoin;

//	StartServer_InitScriptData();

	// shift random number generator
	// otherwise we see identical map and bot rotations

	for (i = uis.realtime % 200; i; i--)
		f = random();

	if (!StartServer_InitServerExec())
		return qfalse;

	if (scriptFile) {
		AddScript("// Script generated by \"QSandbox\" " QS_VERSION "\n");
	}

	//
	// script recovery parameters
	//
	if (scriptFile && !StartServer_WriteScriptParams())
		return qfalse;

	//
	// item parameters
	//
	if (!StartServer_WriteItemParams())
		return qfalse;

	//
	// map parameters
	//
	if (!LoadServer_WriteMapParams())
		return qfalse;

	//
	// bot parameters
	//

	// bot parameters written may depend on map ordering
	if (!StartServer_WriteBotParams())
		return qfalse;

	//
	// server parameters
	//

	// server parameters may depend on bot generation
	if (!StartServer_WriteServerParams())
		return qfalse;

	//
	// force player to join team/specs
	//
	if (s_scriptdata.server.dedicatedServer == SRVDED_OFF) {
		if (scriptFile)
			AddScript("\n// Things that need to be done when first map starts\n");

		AddScript("set activeAction \"");

		teamjoin = "forceteam %i free";
		if (s_scriptdata.bot.joinAs) {
			teamjoin = "forceteam %i spectator";
		}
		else if (s_scriptdata.gametype >= GT_TEAM) {
			// use %%i so a single %i is generated for player client
			teamjoin = va("forceteam %%i %s", bot_teamname[StartServer_GetPlayerTeam()]);
		}
		else if (s_scriptdata.gametype == GT_TOURNAMENT) {
			for (i = 0; i < s_serverexec.player_client; i++)
				AddScript(va("forceteam %i spectator;", i));
		}

		AddScript(va(teamjoin, s_serverexec.player_client));
		AddScript("\"\n");
	}

	// start the map rotation running
	AddScript("\n");
	if (scriptFile)
		AddScript("// start map rotation\n");

	AddScript("vstr m0\n");

	//
	// execute!
	//
	return StartServer_ExecuteScript( scriptFile );
}


