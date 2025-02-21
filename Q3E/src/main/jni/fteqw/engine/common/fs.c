#include "quakedef.h"
#include "netinc.h"

//#define com_gamedir com__gamedir

#include <ctype.h>
#include <limits.h>

#include "fs.h"
#include "shader.h"
#ifdef _WIN32
#include "winquake.h"
#endif



#ifdef FTE_TARGET_WEB	//for stuff that doesn't work right...
#define FORWEB(a,b) a
#else
#define FORWEB(a,b) b
#endif

static char *vidfilenames[] =	//list of filenames to check to see if graphics stuff needs reloading on filesystem changes
{
	"gfx.wad",
	"gfx/conback.lmp"/*q1*/,"gfx/menu/conback.lmp"/*h2*/,"pics/conback.pcx"/*q2*/,	//misc stuff
	"gfx/palette.lmp", "pics/colormap.pcx",
	"gfx/conchars.png",	//conchars...
	"fonts/qfont.kfont", "gfx/mcharset.lmp",	//menu fonts
};


#if !defined(HAVE_LEGACY) || !defined(HAVE_CLIENT)
	#define ZFIXHACK
#elif defined(ANDROID) //on android, these numbers seem to be generating major weirdness, so disable these.
	#define ZFIXHACK
#elif defined(FTE_TARGET_WEB) //on firefox (but not chrome or ie), these numbers seem to be generating major weirdness, so tone them down significantly by default.
	#define ZFIXHACK "set r_polygonoffset_submodel_offset 1\nset r_polygonoffset_submodel_factor 0.05\n"
#else	//many quake maps have hideous z-fighting. this provides a way to work around it, although the exact numbers are gpu and bitdepth dependant, and trying to fix it can actually break other things.
	#define ZFIXHACK "set r_polygonoffset_submodel_offset 25\nset r_polygonoffset_submodel_factor 0.05\n"
#endif

/*ezquake cheats and compat*/
#define EZQUAKECOMPETITIVE "set ruleset_allow_fbmodels 1\nset sv_demoExtensions \"\"\n"
/*quake requires a few settings for compatibility*/
#define QRPCOMPAT "set cl_cursor_scale 0.2\nset cl_cursor_bias_x 7.5\nset cl_cursor_bias_y 0.8\n"
#define QUAKESPASMSUCKS "set mod_h2holey_bugged 1\n"
#define QUAKEOVERRIDES "set sv_listen_nq 2\nset v_gammainverted 1\nset cl_download_mapsrc \"https://maps.quakeworld.nu/all/\"\nset con_stayhidden 0\nset allow_download_pakcontents 2\nset allow_download_refpackages 0\nset r_meshpitch -1\nr_sprite_backfacing 1\nset sv_bigcoords \"\"\nmap_autoopenportals 1\n"  "sv_port "STRINGIFY(PORT_QWSERVER)" "STRINGIFY(PORT_NQSERVER)"\n" ZFIXHACK EZQUAKECOMPETITIVE QUAKESPASMSUCKS
#define QCFG "//schemes quake qw\n"   QUAKEOVERRIDES "set com_parseutf8 0\n" QRPCOMPAT
#define EZCFG "//mainconfig config\n" QCFG "fps_preset fast\n"
#define KEXCFG "//schemes quake_r2\n" QUAKEOVERRIDES "set com_parseutf8 1\nset campaign 0\nset net_enable_dtls 1\nset sv_mintic 0.016666667\nset sv_maxtic $sv_mintic\nset cl_netfps 60\n"
/*NetQuake reconfiguration, to make certain people feel more at home...*/
#define NQCFG "//disablehomedir 1\n//mainconfig ftenq\n" QCFG "cfg_save_auto 1\nfps_preset nq\nset cl_loopbackprotocol auto\ncl_sbar 1\nset plug_sbar 0\nset sv_port "STRINGIFY(PORT_NQSERVER)"\ncl_defaultport "STRINGIFY(PORT_NQSERVER)"\nset m_preset_chosen 1\nset vid_wait 1\nset cl_demoreel 1\n"
#define SPASMCFG NQCFG "fps_preset builtin_spasm\nset cl_demoreel 0\ncl_sbar 2\nset gl_load24bit 1\n"
#define FITZCFG NQCFG "fps_preset builtin_spasm\ncl_sbar 2\nset gl_load24bit 1\n"
#define TENEBRAECFG NQCFG "fps_preset builtin_tenebrae\n"
//nehahra has to be weird with its extra cvars, and buggy fullbrights.
#define NEHCFG QCFG "set nospr32 0\nset cutscene 1\nalias startmap_sp \"map nehstart\"\nr_fb_bmodels 0\nr_fb_models 0\n"
/*stuff that makes dp-only mods work a bit better*/
#define DPCOMPAT QCFG "gl_specular 1\nset _cl_playermodel \"\"\n set dpcompat_set 1\ndpcompat_console 1\nset dpcompat_corruptglobals 1\nset vid_pixelheight 1\nset dpcompat_set 1\nset dpcompat_console 1\nset r_particlesdesc effectinfo\n"
/*nexuiz/xonotic has a few quirks/annoyances...*/
#define NEXCFG DPCOMPAT "cl_loopbackprotocol dpp7\nset sv_listen_dp 1\nset sv_listen_qw 0\nset sv_listen_nq 0\nset dpcompat_nopreparse 1\nset sv_bigcoords 1\nset sv_maxairspeed \"30\"\nset sv_jumpvelocity 270\nset sv_mintic \"0.01\"\ncl_nolerp 0\n"
#define XONCFG NEXCFG "set qport $qport_\ncom_parseutf8 1\npr_fixbrokenqccarrays 2\nset pr_csqc_memsize 64m\nset pr_ssqc_memsize 96m\n"
/*some modern non-compat settings*/
#define DMFCFG "set com_parseutf8 1\npm_airstep 1\nsv_demoExtensions 1\n"
/*set some stuff so our regular qw client appears more like hexen2. sv_mintic must be 0.015 to 'fix' the ravenstaff so that its projectiles don't impact upon each other, or even 0.05 to exactly match the hardcoded assumptions in obj_push. There's maps that depend on a low framerate via waterjump framerate-dependance too.*/
#define HEX2CFG "//schemes hexen2\n" "set v_gammainverted 1\nset com_parseutf8 -1\nset gl_font gfx/hexen2\nset in_builtinkeymap 0\nset_calc cl_playerclass int (random * 5) + 1\nset cl_forwardspeed 200\nset cl_backspeed 200\ncl_sidespeed 225\nset sv_maxspeed 640\ncl_run 0\nset watervis 1\nset r_lavaalpha 1\nset r_lavastyle -2\nset r_wateralpha 0.5\nset sv_pupglow 1\ngl_shaftlight 0.5\nsv_mintic 0.05\nset r_meshpitch -1\nset r_meshroll -1\nr_sprite_backfacing 1\nset mod_warnmodels 0\nset cl_model_bobbing 1\nsv_sound_watersplash \"misc/hith2o.wav\"\nsv_sound_land \"fx/thngland.wav\"\nset sv_walkpitch 0\n"
/*yay q2!*/
#define Q2CFG "//schemes quake2\n" "set com_protocolversion "STRINGIFY(PROTOCOL_VERSION_Q2)"\nset v_gammainverted 1\nset com_parseutf8 0\ncom_gamedirnativecode 1\nset sv_bigcoords 0\nsv_port "STRINGIFY(PORT_Q2SERVER)" "STRINGIFY(PORT_Q2EXSERVER)"\ncl_defaultport "STRINGIFY(PORT_Q2SERVER)"\n"	\
	"set r_replacemodels " IFMINIMAL("","md3 md5mesh")"\n"	\
	"set r_glsl_emissive 0\n" /*work around the _glow textures not being meant to glow*/
/*Q3's ui doesn't like empty model/headmodel/handicap cvars, even if the gamecode copes*/
#define Q3CFG "//schemes quake3\n" "set v_gammainverted 0\nset snd_ignorecueloops 1\nsetfl g_gametype 0 s\nset gl_clear 1\nset r_clearcolour 0 0 0\nset com_parseutf8 0\ngl_overbright "FORWEB("0","2")"\nseta model sarge\nseta headmodel sarge\nseta handicap 100\ncom_gamedirnativecode 1\nsv_port "STRINGIFY(PORT_Q3SERVER)"\ncl_defaultport "STRINGIFY(PORT_Q3SERVER)"\ncom_protocolversion 68\n"
//#define RMQCFG "sv_bigcoords 1\n"

#define HLCFG "plug_load ffmpeg"
#define HL2CFG "plug_load ode;plug_load hl2"

#ifndef UPDATEURL
	#ifdef HAVE_SSL
		#define UPDATEURL(g)	"/downloadables.php?game=" #g
	#else
		#define UPDATEURL(g)	NULL
	#endif
#endif

#define QUAKEPROT "FTE-Quake DarkPlaces-Quake"

typedef struct {
	const char *argname;	//used if this was used as a parameter.
	const char *exename;	//used if the exe name contains this
	const char *protocolname;	//sent to the master server when this is the current gamemode (Typically set for DP compat).
	const char *auniquefile[4];	//used if this file is relative from the gamedir. needs just one file

	const char *customexec;

	const char *dir[4];
	const char *poshname;		//Full name for the game.
	const char *downloadsurl;	//url to check for updates.
	const char *needpackages;	//package name(s) that are considered mandatory for this game to work.
	const char *manifestfile;	//contents of manifest file to use.
} gamemode_info_t;
static const gamemode_info_t gamemode_info[] = {
#ifdef GAME_SHORTNAME
	#ifndef GAME_PROTOCOL
	#define GAME_PROTOCOL			DISTRIBUTION
	#endif
	#ifndef GAME_IDENTIFYINGFILES
	#define GAME_IDENTIFYINGFILES	NULL	//
	#endif
	#ifndef GAME_DEFAULTCMDS
	#define GAME_DEFAULTCMDS		NULL	//doesn't need anything
	#endif
	#ifndef GAME_BASEGAMES
	#define GAME_BASEGAMES			"data"
	#endif
	#ifndef GAME_FULLNAME
	#define GAME_FULLNAME			FULLENGINENAME
	#endif
	#ifndef GAME_MANIFESTUPDATE
	#define GAME_MANIFESTUPDATE		NULL
	#endif

	{"-"GAME_SHORTNAME,		GAME_SHORTNAME,			GAME_PROTOCOL,					{GAME_IDENTIFYINGFILES}, GAME_DEFAULTCMDS, {GAME_BASEGAMES}, GAME_FULLNAME, NULL/*updateurl*/, NULL/*needpackages*/, GAME_MANIFESTUPDATE},
#endif
//note that there is no basic 'fte' gamemode, this is because we aim for network compatability. Darkplaces-Quake is the closest we get.
//this is to avoid having too many gamemodes anyway.

//mission packs should generally come after the main game to avoid prefering the main game. we violate this for hexen2 as the mission pack is mostly a superset.
//whereas the quake mission packs replace start.bsp making the original episodes unreachable.
//for quake, we also allow extracting all files from paks. some people think it loads faster that way or something.
#ifdef HAVE_LEGACY
	//cmdline switch exename    protocol name(dpmaster)  identifying file				exec     dir1       dir2    dir3       dir(fte)     full name
	//use rerelease behaviours if we seem to be running from that dir.
	{"-quake_rerel",NULL,		"FTE-QuakeRerelease",	{"QuakeEX.kpf"},				KEXCFG,	{"id1",							"*fte"},	"Quake Re-Release",					UPDATEURL(Q1)},
	//standard quake
	{"-quake",		"q1",		QUAKEPROT,				{"id1/pak0.pak","id1/quake.rc"},QCFG,	{"id1",		"qw",				"*fte"},	"Quake",							UPDATEURL(Q1)},
	//alternative name, because fmf file install names are messy when a single name is used for registry install path.
	{"-afterquake",	NULL,		"FTE-Quake",			{"id1/pak0.pak", "id1/quake.rc"},QCFG,	{"id1",		"qw",				"*fte"},	"AfterQuake",						UPDATEURL(Q1),	NULL},
	//netquake-specific quake that avoids qw/ with its nquake fuckups, and disables nqisms
	{"-netquake",	NULL,		QUAKEPROT,				{"id1/pak0.pak","id1/quake.rc"},NQCFG,	{"id1"},									"NetQuake",							UPDATEURL(Q1)},
	//common variant of fitzquake that includes its own special pak file in the basedir
	{"-spasm",		NULL,		QUAKEPROT,				{"quakespasm.pak"},				SPASMCFG,{"/id1"},									"FauxSpasm",						UPDATEURL(Q1)},
	//because we can. 'fps_preset spasm' is hopefully close enough...
	{"-fitz",		"nq",		QUAKEPROT,				{"id1/pak0.pak","id1/quake.rc"},FITZCFG,{"id1"},									"FauxFitz",							UPDATEURL(Q1)},
	//because we can
	{"-tenebrae",	NULL,		QUAKEPROT,				{"tenebrae/Pak0.pak","id1/quake.rc"},TENEBRAECFG,{"id1",				"tenebrae"},"FauxTenebrae",						UPDATEURL(Q1)},
	//for the luls
	{"-ezquake",	"ez",		"FTE-Quake",			{"id1/pak0.pak", "id1/quake.rc"},EZCFG,	{"id1",		"qw",				"*ezquake"},"ezFTE",							UPDATEURL(Q1),	NULL},

#if defined(Q2CLIENT) || defined(Q2SERVER)
	//list quake2 before q1 missionpacks, to avoid confusion about rogue/pak0.pak
	{"-quake2",		"q2",		"Quake2",				{"baseq2/pak0.pak"},			Q2CFG,	{"baseq2",						"*fteq2"},	"Quake II",							UPDATEURL(Q2)},
	//mods of the above that should generally work.
	{"-dday",		"dday",		"Quake2",				{"dday/pak0.pak"},				Q2CFG,	{"baseq2",	"dday",				"*fteq2"},	"D-Day: Normandy"},
#endif

	//quake's mission packs technically have their own protocol (thanks to stat_items). copyrights mean its best to keep them separate, too.
	{"-hipnotic",	"hipnotic",	"FTE-Hipnotic",		{"hipnotic/pak0.pak","hipnotic/gfx.wad"},QCFG,{"id1",	"qw",	"hipnotic",	"*fte"},	"Quake: Scourge of Armagon",		UPDATEURL(Q1)},
	{"-rogue",		"rogue",	"FTE-Rogue",			{"rogue/pak0.pak","rogue/gfx.wad"},QCFG,{"id1",		"qw",	"rogue",	"*fte"},	"Quake: Dissolution of Eternity",	UPDATEURL(Q1)},

	//various quake-dependant non-standalone mods that require hacks
	//quoth needed an extra arg just to enable hipnotic hud drawing, it doesn't actually do anything weird, but most engines have a -quoth arg, so lets have one too.
	{"-quoth",		"quoth",	"FTE-Quake",			{"quoth/pak0.pak"},				QCFG,	{"id1",		"qw",	"quoth",	"*fte"},	"Quake: Quoth",						UPDATEURL(Q1)},
	{"-nehahra",	"nehahra",	"FTE-Quake",			{"nehahra/pak0.pak"},			NEHCFG,	{"id1",		"qw",	"nehahra",	"*fte"},	"Quake: Seal Of Nehahra",			UPDATEURL(Q1)},
	//various quake-based standalone mods.
	{"-librequake",	"librequake","LibreQuake",			{"lq1/pak0.pak","lq1/gfx.pk3","lq1/quake.rc"},QCFG,	{"lq1"},									"LibreQuake",			UPDATEURL(LQ)},
//	{"-nexuiz",		"nexuiz",	"Nexuiz",				{"nexuiz.exe"},					NEXCFG,	{"data",						"*ftedata"},"Nexuiz"},
//	{"-xonotic",	"xonotic",	"Xonotic",				{"data/xonotic-data.pk3dir",
//														 "data/xonotic-*data*.pk3"},	XONCFG,	{"data",						"*ftedata"},"Xonotic",							UPDATEURL(Xonotic)},
//	{"-spark",		"spark",	"Spark",				{"base/src/progs.src",
//														 "base/qwprogs.dat",
//														 "base/pak0.pak"},				DMFCFG,	{"base",								},	"Spark"},
//	{"-scouts",		"scouts",	"FTE-SJ",				{"basesj/src/progs.src",
//														 "basesj/progs.dat",
//														 "basesj/pak0.pak"},			NULL,	{"basesj",						        },	"Scouts Journey"},
//	{"-rmq",		"rmq",		"RMQ",					{NULL},							RMQCFG,	{"id1",		"qw",	"rmq",		"*fte"	},	"Remake Quake"},

#ifdef HEXEN2
	//hexen2's mission pack generally takes precedence if both are installed.
	{"-portals",	"h2mp",		"FTE-H2MP",				{"portals/hexen.rc",
														 "portals/pak3.pak"},			HEX2CFG,{"data1",	"portals",			"*fteh2"},	"Hexen II MP",						UPDATEURL(H2)},
	{"-hexen2",		"hexen2",	"FTE-Hexen2",			{"data1/pak0.pak"},				HEX2CFG,{"data1",						"*fteh2"},	"Hexen II",							UPDATEURL(H2)},
#endif

#if defined(Q3CLIENT) || defined(Q3SERVER)
	{"-quake3",		"q3",		"Quake3",				{"baseq3/pak0.pk3"},			Q3CFG,	{"baseq3",						"*fteq3"},	"Quake III Arena",					UPDATEURL(Q3),		"fteplug_quake3"},
	{"-quake3demo",	"q3demo",	"Quake3Demo",			{"demoq3/pak0.pk3"},			Q3CFG,	{"demoq3",						"*fteq3"},	"Quake III Arena Demo",				NULL,				"fteplug_quake3"},
	//the rest are not supported in any real way. maps-only mostly, if that
//	{"-quake4",		"q4",		"FTE-Quake4",			{"q4base/pak00.pk4"},			NULL,	{"q4base",						"*fteq4"},	"Quake 4"},
//	{"-et",			NULL,		"FTE-EnemyTerritory",	{"etmain/pak0.pk3"},			NULL,	{"etmain",						"*fteet"},	"Wolfenstein - Enemy Territory"},

//	{"-jk2",		"jk2",		"FTE-JK2",				{"base/assets0.pk3"},			NULL,	{"base",						"*ftejk2"},	"Jedi Knight II: Jedi Outcast"},
//	{"-warsow",		"warsow",	"FTE-Warsow",			{"basewsw/pak0.pk3"},			NULL,	{"basewsw",						"*ftewsw"},	"Warsow"},

	{"-cod4",		NULL,		"FTE-CoD4",				{"cod4.ico"},					NULL,	{"main",						"*ftecod"},	"Call of Duty 4",				NULL,				"fteplug_cod"},
	{"-cod2",		NULL,		"FTE-CoD2",				{"main/iw_00.iwd"},				NULL,	{"main",						"*ftecod"},	"Call of Duty 2",				NULL,				"fteplug_cod"},
	{"-cod",		NULL,		"FTE-CoD",				{"Main/pak0.pk3"},				NULL,	{"Main",						"*ftecod"},	"Call of Duty",					NULL,				"fteplug_cod"},
#endif
#if !defined(QUAKETC) && !defined(MINIMAL)
//	{"-doom",		"doom",		"FTE-Doom",				{"doom.wad"},					NULL,	{"*",							"*ftedoom"},"Doom"},
//	{"-doom2",		"doom2",	"FTE-Doom2",			{"doom2.wad"},					NULL,	{"*",							"*ftedoom"},"Doom2"},
//	{"-doom3",		"doom3",	"FTE-Doom3",			{"doom3.wad"},					NULL,	{"based3",						"*ftedoom3"},"Doom3"},

	//for the luls
//	{"-diablo2",	NULL,		"FTE-Diablo2",			{"d2music.mpq"},				NULL,	{"*",							"*fted2"},	"Diablo 2"},
#endif
	/* maintained by frag-net.com ~eukara */
	{"-halflife",	"halflife",	"Rad-Therapy",	{"valve/liblist.gam"},	HLCFG,	{"valve"},	"Rad-Therapy",	"https://www.frag-net.com/pkgs/halflife.txt", "valve-patch-radtherapy;fteplug_ffmpeg"},
	{"-gunman",	"gunman",	"Rad-Therapy",		{"rewolf/liblist.gam"},	HLCFG,	{"rewolf"},	"Gunman Chronicles",	"https://www.gunmanchronicles.com/packages.txt", "rewolf-patch-gunman;fteplug_ffmpeg"},
	{"-halflife2",	"halflife2",	"Rad-Therapy-II",	{"hl2/gameinfo.txt"},	HL2CFG,	{"hl2", "hl2mp"},	"Rad-Therapy II",						"https://www.frag-net.com/pkgs/halflife2.txt", "hl2-patch-radtherapy2;fteplug_ffmpeg;fteplug_ode;fteplug_hl2"},
	{"-gmod9",	"halflife2",	"Rad-Therapy-II",	{"gmod9/gameinfo.txt"},	HL2CFG,	{"css", "hl2", "hl2mp", "gmod9"},	"Free Will",		"https://www.frag-net.com/pkgs/halflife2.txt", "hl2mp-mod-gmod9;fteplug_ffmpeg;fteplug_ode;fteplug_hl2"},
#endif

	{NULL}
};




void FS_BeginManifestUpdates(void);
static void QDECL fs_game_callback(cvar_t *var, char *oldvalue);
static void COM_InitHomedir(ftemanifest_t *man);
hashtable_t filesystemhash;
static qboolean com_fschanged = true, com_fsneedreload;
qboolean com_installer = false;
qboolean fs_readonly;
static searchpath_t *fs_allowfileuri;
int waitingformanifest;
static unsigned int fs_restarts;
void *fs_thread_mutex;
float fs_accessed_time;	//timestamp of read (does not include flocates, which should normally happen via a cache).

static cvar_t fs_hidesyspaths		= CVARFD	("fs_hidesyspaths", IFWEB("0","1"), 0, "0: Show system paths in console prints that might appear in screenshots or video capture.\n1: Replace the start of filenames in console prints with generic prefixes that cannot leak private info like your operating system's user name.");
static cvar_t com_fs_cache			= CVARFD	("fs_cache", IFMINIMAL("2","1"), CVAR_ARCHIVE, "0: Do individual lookups.\n1: Scan all files for accelerated lookups. This provides a performance boost on windows and avoids case sensitivity issues on linux.\n2: like 1, but don't bother checking for external changes (avoiding the cost of rebuild the cache).");
static cvar_t fs_noreexec			= CVARD		("fs_noreexec", "0", "Disables automatic re-execing configs on gamedir switches.\nThis means your cvar defaults etc may be from the wrong mod, and cfg_save will leave that stuff corrupted!");
static cvar_t cfg_reload_on_gamedir = CVAR		("cfg_reload_on_gamedir", "1");
static cvar_t fs_game				= CVARAFCD	("fs_game"/*q3*/, "", "game"/*q2/qs*/, CVAR_NOSAVE|CVAR_NORESET, fs_game_callback, "Provided for Q2 compat. Contains the subdir of the current mod.");
static cvar_t fs_gamepath			= CVARAFD	("fs_gamepath"/*q3ish*/, "", "fs_gamedir"/*q2*/, CVAR_NOUNSAFEEXPAND|CVAR_NOSET|CVAR_NOSAVE, "Provided for Q2/Q3 compat. System path of the active gamedir.");
static cvar_t fs_basepath			= CVARAFD	("fs_basepath"/*q3*/,    "", "fs_basedir"/*q2*/, CVAR_NOUNSAFEEXPAND|CVAR_NOSET|CVAR_NOSAVE, "Provided for Q2/Q3 compat. System path of the base directory.");
static cvar_t fs_homepath			= CVARAFD	("fs_homepath"/*q3ish*/, "", "fs_homedir"/*q2ish*/, CVAR_NOUNSAFEEXPAND|CVAR_NOSET|CVAR_NOSAVE, "Provided for Q2/Q3 compat. System path of the base directory.");
static cvar_t dpcompat_ignoremodificationtimes = CVARAFD("fs_packageprioritisation", "1", "dpcompat_ignoremodificationtimes", CVAR_NOUNSAFEEXPAND|CVAR_NOSAVE, "Favours the package that is:\n0: Most recently modified\n1: Is alphabetically last (favour z over a, 9 over 0).");
cvar_t	fs_dlURL					= CVARAFD(/*ioq3*/"sv_dlURL", "", /*dp*/"sv_curl_defaulturl", CVAR_SERVERINFO|IFWEB(CVAR_NOSAVE,CVAR_ARCHIVE), "Provides clients with an external url from which they can obtain pk3s/packages from an external http server instead of having to download over udp.");
cvar_t  cl_download_mapsrc			= CVARFD("cl_download_mapsrc", "", CVAR_ARCHIVE, "Specifies an http location prefix for map downloads. EG: \"http://example.com/path/gamemaps/\"");
int active_fs_cachetype;
static int fs_referencetype;
int fs_finds;
void COM_CheckRegistered (void);
void Mods_FlushModList(void);
static void FS_ReloadPackFilesFlags(unsigned int reloadflags);
static qboolean Sys_SteamHasFile(char *basepath, int basepathlen, char *steamdir, char *fname);

static void QDECL fs_game_callback(cvar_t *var, char *oldvalue)
{
	static qboolean runaway = false;
	char buf[MAX_OSPATH];
	if (!strcmp(var->string, oldvalue))
		return;	//no change here.
	if (runaway)
		return;	//ignore that
	runaway = true;
	Cmd_ExecuteString(va("gamedir %s\n", COM_QuotedString(var->string, buf, sizeof(buf), false)), RESTRICT_LOCAL);
	runaway = false;
}

static struct
{
	void *module;
	const char *extension;
	searchpathfuncs_t *(QDECL *OpenNew)(vfsfile_t *file, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix);
	qboolean loadscan;
} searchpathformats[64];

int FS_RegisterFileSystemType(void *module, const char *extension, searchpathfuncs_t *(QDECL *OpenNew)(vfsfile_t *file, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix), qboolean loadscan)
{
	unsigned int i;
	for (i = 0; i < sizeof(searchpathformats)/sizeof(searchpathformats[0]); i++)
	{
		if (searchpathformats[i].extension && !strcmp(searchpathformats[i].extension, extension))
			break;	//extension match always replaces
		if (!searchpathformats[i].extension && !searchpathformats[i].OpenNew)
			break;
	}
	if (i == sizeof(searchpathformats)/sizeof(searchpathformats[0]))
		return 0;

	searchpathformats[i].module = module;
	searchpathformats[i].extension = extension;
	searchpathformats[i].OpenNew = OpenNew;
	searchpathformats[i].loadscan = loadscan;
	com_fschanged = true;
	com_fsneedreload = true;

	return i+1;
}

void FS_UnRegisterFileSystemType(int idx)
{
	if ((unsigned int)(idx-1) >= sizeof(searchpathformats)/sizeof(searchpathformats[0]))
		return;

	searchpathformats[idx-1].OpenNew = NULL;
	searchpathformats[idx-1].module = NULL;
	com_fschanged = true;
	com_fsneedreload = true;

	//FS_Restart will be needed
}
void FS_UnRegisterFileSystemModule(void *module)
{
	int i;
	qboolean found = false;
	if (!fs_thread_mutex || Sys_LockMutex(fs_thread_mutex))
	{
		for (i = 0; i < sizeof(searchpathformats)/sizeof(searchpathformats[0]); i++)
		{
			if (searchpathformats[i].module == module)
			{
				searchpathformats[i].extension = NULL;
				searchpathformats[i].OpenNew = NULL;
				searchpathformats[i].module = NULL;
				found = true;
			}
		}
		if (fs_thread_mutex)
		{
			Sys_UnlockMutex(fs_thread_mutex);
			if (found)
			{
				Cmd_ExecuteString("fs_restart", RESTRICT_LOCAL);
			}
		}
	}
}

char *VFS_GETS(vfsfile_t *vf, char *buffer, size_t buflen)
{
	char in;
	char *out = buffer;
	size_t len;
	if (buflen <= 1)
		return NULL;
	len = buflen-1;
	while (len > 0)
	{
		if (VFS_READ(vf, &in, 1) != 1)
		{
			if (len == buflen-1)
				return NULL;
			*out = '\0';
			return buffer;
		}
		if (in == '\n')
			break;
		*out++ = in;
		len--;
	}
	*out = '\0';

	//if there's a trailing \r, strip it.
	if (out > buffer)
		if (out[-1] == '\r')
			out[-1] = 0;

	return buffer;
}

void VARGS VFS_PRINTF(vfsfile_t *vf, const char *format, ...)
{
	va_list		argptr;
	char		string[2048];

	va_start (argptr, format);
	vsnprintf (string,sizeof(string)-1, format,argptr);
	va_end (argptr);

	VFS_PUTS(vf, string);
}



#if defined(_WIN32) && !defined(FTE_SDL) && !defined(WINRT) && !defined(_XBOX)
//windows has a special helper function to handle legacy URIs.
#else
qboolean Sys_ResolveFileURL(const char *inurl, int inlen, char *out, int outlen)
{
	const unsigned char *i = inurl, *inend = inurl+inlen;
	unsigned char *o = out, *outend = out+outlen;
	unsigned char hex;

	//make sure its a file url...
	if (inlen < 5 || strncmp(inurl, "file:", 5))
		return false;
	i += 5;

	if (i+1 < inend && i[0] == '/' && i[1] == '/')
	{	//has an authority field...
		i+=2;
		//except we don't support authorities other than ourself...
		if (i >= inend || *i != '/')
			return false;	//must be an absolute path...
#ifdef _WIN32
		i++;	//on windows, (full)absolute paths start with a drive name...
#endif
	}
	else if (i < inend && i[0] == '/')
		;	// file:/foo (no authority)
	else
		return false;

	//everything else must be percent-encoded
	while (i < inend)
	{
		if (!*i || o == outend)
			return false;	//don't allow nulls...
		else if (*i == '/' && i+1<inend && i[1] == '/')
			return false;	//two slashes is invalid (can be parent directory on some systems, or just buggy or weird)
		else if (*i == '\\')
			return false;	//don't allow backslashes. they're meant to be percent-encoded anyway.
		else if (*i == '%' && i+2<inend)
		{
			hex = 0;
			if (i[1] >= 'A' && i[1] <= 'F')
				hex += i[1]-'A'+10;
			else if (i[1] >= 'a' && i[1] <= 'f')
				hex += i[1]-'a'+10;
			else if (i[1] >= '0' && i[1] <= '9')
				hex += i[1]-'0';
			else
			{
				*o++ = *i++;
				continue;
			}
			hex <<= 4;
			if (i[2] >= 'A' && i[2] <= 'F')
				hex += i[2]-'A'+10;
			else if (i[2] >= 'a' && i[2] <= 'f')
				hex += i[2]-'a'+10;
			else if (i[2] >= '0' && i[2] <= '9')
				hex += i[2]-'0';
			else
			{
				*o++ = *i++;
				continue;
			}
			*o++ = hex;
			i += 3;
		}
		else
			*o++ = *i++;
	}

	if (o == outend)
		return false;
	*o = 0;

	return true;
}
#endif




char	gamedirfile[MAX_OSPATH];
static char	pubgamedirfile[MAX_OSPATH];	//like gamedirfile, but not set to the fte-only paths


static searchpath_t *gameonly_homedir;
static searchpath_t *gameonly_gamedir;

char	com_gamepath[MAX_OSPATH];	//c:\games\quake
char	com_homepath[MAX_OSPATH];	//c:\users\foo\my docs\fte\quake
qboolean	com_homepathenabled;
static qboolean	com_homepathusable;	//com_homepath is safe, even if not enabled.

//char	com_configdir[MAX_OSPATH];	//homedir/fte/configs

int fs_hash_dups;
int fs_hash_files;







static const char *FS_GetCleanPath(const char *pattern, qboolean silent, char *outbuf, int outlen);
static void FS_RegisterDefaultFileSystems(void);
static void	COM_CreatePath (char *path);
static ftemanifest_t *FS_ReadDefaultManifest(char *newbasedir, size_t newbasedirsize, qboolean fixedbasedir);

#define ENFORCEFOPENMODE(mode) {if (strcmp(mode, "r") && strcmp(mode, "w")/* && strcmp(mode, "rw")*/)Sys_Error("fs mode %s is not permitted here\n");}






//forget a manifest entirely.
void FS_Manifest_Free(ftemanifest_t *man)
{
	int i, j;
	if (!man)
		return;
	Z_Free(man->filename);
	Z_Free(man->updateurl);
	Z_Free(man->installation);
	Z_Free(man->formalname);
#ifdef PACKAGEMANAGER
	Z_Free(man->downloadsurl);
	Z_Free(man->installupd);
#endif
	Z_Free(man->mainconfig);
	Z_Free(man->schemes);
	Z_Free(man->protocolname);
	Z_Free(man->eula);
	Z_Free(man->defaultexec);
	Z_Free(man->defaultoverrides);
	Z_Free(man->basedir);
	Z_Free(man->iconname);
	for (i = 0; i < sizeof(man->gamepath) / sizeof(man->gamepath[0]); i++)
	{
		Z_Free(man->gamepath[i].path);
	}
	for (i = 0; i < sizeof(man->package) / sizeof(man->package[0]); i++)
	{
		Z_Free(man->package[i].packagename);
		Z_Free(man->package[i].path);
		Z_Free(man->package[i].prefix);
		Z_Free(man->package[i].condition);
		Z_Free(man->package[i].sha512);
		Z_Free(man->package[i].signature);
		for (j = 0; j < sizeof(man->package[i].mirrors) / sizeof(man->package[i].mirrors[0]); j++)
			Z_Free(man->package[i].mirrors[j]);
	}
	Z_Free(man);
}

//clone a manifest, so we can hack at it.
static ftemanifest_t *FS_Manifest_Clone(ftemanifest_t *oldm)
{
	ftemanifest_t *newm;
	int i, j;
	newm = Z_Malloc(sizeof(*newm));
	if (oldm->updateurl)
		newm->updateurl = Z_StrDup(oldm->updateurl);
	if (oldm->installation)
		newm->installation = Z_StrDup(oldm->installation);
	if (oldm->formalname)
		newm->formalname = Z_StrDup(oldm->formalname);
#ifdef PACKAGEMANAGER
	if (oldm->downloadsurl)
		newm->downloadsurl = Z_StrDup(oldm->downloadsurl);
	if (oldm->installupd)
		newm->installupd = Z_StrDup(oldm->installupd);
#endif
	if (oldm->schemes)
		newm->schemes = Z_StrDup(oldm->schemes);
	if (oldm->protocolname)
		newm->protocolname = Z_StrDup(oldm->protocolname);
	if (oldm->eula)
		newm->eula = Z_StrDup(oldm->eula);
	if (oldm->defaultexec)
		newm->defaultexec = Z_StrDup(oldm->defaultexec);
	if (oldm->defaultoverrides)
		newm->defaultoverrides = Z_StrDup(oldm->defaultoverrides);
	if (oldm->iconname)
		newm->iconname = Z_StrDup(oldm->iconname);
	if (oldm->basedir)
		newm->basedir = Z_StrDup(oldm->basedir);
	if (oldm->mainconfig)
		newm->mainconfig = Z_StrDup(oldm->mainconfig);
	newm->homedirtype = oldm->homedirtype;

	for (i = 0; i < sizeof(newm->gamepath) / sizeof(newm->gamepath[0]); i++)
	{
		if (oldm->gamepath[i].path)
			newm->gamepath[i].path = Z_StrDup(oldm->gamepath[i].path);
		newm->gamepath[i].flags = oldm->gamepath[i].flags;
	}
	for (i = 0; i < sizeof(newm->package) / sizeof(newm->package[0]); i++)
	{
		newm->package[i].type = oldm->package[i].type;
		newm->package[i].crc = oldm->package[i].crc;
		newm->package[i].crcknown = oldm->package[i].crcknown;
		if (oldm->package[i].packagename)
			newm->package[i].packagename = Z_StrDup(oldm->package[i].packagename);
		if (oldm->package[i].path)
			newm->package[i].path = Z_StrDup(oldm->package[i].path);
		if (oldm->package[i].prefix)
			newm->package[i].prefix = Z_StrDup(oldm->package[i].prefix);
		if (oldm->package[i].condition)
			newm->package[i].condition = Z_StrDup(oldm->package[i].condition);
		if (oldm->package[i].sha512)
			newm->package[i].sha512 = Z_StrDup(oldm->package[i].sha512);
		if (oldm->package[i].signature)
			newm->package[i].signature = Z_StrDup(oldm->package[i].signature);
		newm->package[i].filesize = oldm->package[i].filesize;
		for (j = 0; j < sizeof(newm->package[i].mirrors) / sizeof(newm->package[i].mirrors[0]); j++)
			if (oldm->package[i].mirrors[j])
				newm->package[i].mirrors[j] = Z_StrDup(oldm->package[i].mirrors[j]);
	}

	newm->security = oldm->security;

	return newm;
}

static void FS_Manifest_Print(ftemanifest_t *man)
{
	char buffer[65536];
	int i, j;
	if (man->updateurl)
		Con_Printf("updateurl %s\n", COM_QuotedString(man->updateurl, buffer, sizeof(buffer), false));
	if (man->eula)
		Con_Printf("eula %s\n", COM_QuotedString(man->eula, buffer, sizeof(buffer), false));
	if (man->installation)
		Con_Printf("game %s\n", COM_QuotedString(man->installation, buffer, sizeof(buffer), false));
	if (man->formalname)
		Con_Printf("name %s\n", COM_QuotedString(man->formalname, buffer, sizeof(buffer), false));
	if (man->mainconfig)
		Con_Printf("mainconfig %s\n", COM_QuotedString(man->mainconfig, buffer, sizeof(buffer), false));
#ifdef PACKAGEMANAGER
	if (man->downloadsurl)
		Con_Printf("downloadsurl %s\n", COM_QuotedString(man->downloadsurl, buffer, sizeof(buffer), false));
	if (man->installupd)
		Con_Printf("install %s\n", COM_QuotedString(man->installupd, buffer, sizeof(buffer), false));
#endif
	if (man->schemes)
		Con_Printf("schemes %s\n", COM_QuotedString(man->schemes, buffer, sizeof(buffer), false));
	if (man->protocolname)
		Con_Printf("protocolname %s\n", COM_QuotedString(man->protocolname, buffer, sizeof(buffer), false));
	if (man->defaultexec)
	{
		char *s = buffer, *e;
		for (s = man->defaultexec; *s; s = e)
		{
			e = strchr(s, '\n');
			if (e)
			{
				*e = 0;
				Con_Printf("-%s\n", s);
				*e++ = '\n';
			}
			else
			{
				Con_Printf("-%s\n", s);
				e = s+strlen(s);
			}
		}
		//Con_Printf("defaultexec %s\n", COM_QuotedString(man->defaultexec, buffer, sizeof(buffer), false));
	}
	if (man->defaultoverrides)
	{
		char *s = buffer, *e;
		for (s = man->defaultoverrides; *s; s = e)
		{
			e = strchr(s, '\n');
			if (e)
			{
				*e = 0;
				Con_Printf("+%s\n", s);
				*e++ = '\n';
			}
			else
			{
				Con_Printf("+%s\n", s);
				e = s+strlen(s);
			}
		}
		//Con_Printf("%s", man->defaultoverrides);
	}
	if (man->iconname)
		Con_Printf("icon %s\n", COM_QuotedString(man->iconname, buffer, sizeof(buffer), false));
	if (man->basedir)
		Con_Printf("basedir %s\n", COM_QuotedString(man->basedir, buffer, sizeof(buffer), false));

	for (i = 0; i < sizeof(man->gamepath) / sizeof(man->gamepath[0]); i++)
	{
		if (man->gamepath[i].path)
		{
			char *str = va("%s%s%s",
				(man->gamepath[i].flags & GAMEDIR_QSHACK)?"/":"",
				(man->gamepath[i].flags & GAMEDIR_PRIVATE)?"*":"",
				man->gamepath[i].path);

			if (man->gamepath[i].flags & GAMEDIR_BASEGAME)
				Con_Printf("basegame %s\n", COM_QuotedString(str, buffer, sizeof(buffer), false));
			else
				Con_Printf("gamedir %s\n", COM_QuotedString(str, buffer, sizeof(buffer), false));
		}
	}

	for (i = 0; i < sizeof(man->package) / sizeof(man->package[0]); i++)
	{
		if (man->package[i].path)
		{
			if (man->package[i].type == mdt_installation)
				Con_Printf("library ");
			else
				Con_Printf("package ");
			Con_Printf("%s", COM_QuotedString(man->package[i].path, buffer, sizeof(buffer), false));
			if (man->package[i].prefix)
				Con_Printf(" name %s", COM_QuotedString(man->package[i].packagename, buffer, sizeof(buffer), false));
			if (man->package[i].prefix)
				Con_Printf(" prefix %s", COM_QuotedString(man->package[i].prefix, buffer, sizeof(buffer), false));
			if (man->package[i].condition)
				Con_Printf(" condition %s", COM_QuotedString(man->package[i].condition, buffer, sizeof(buffer), false));
			if (man->package[i].filesize)
				Con_Printf(" filesize %"PRIuQOFS, man->package[i].filesize);
			if (man->package[i].sha512)
				Con_Printf(" sha512 %s", COM_QuotedString(man->package[i].sha512, buffer, sizeof(buffer), false));
			if (man->package[i].signature)
				Con_Printf(" signature %s", COM_QuotedString(man->package[i].signature, buffer, sizeof(buffer), false));
			if (man->package[i].crcknown)
				Con_Printf(" crc 0x%x", man->package[i].crc);
			for (j = 0; j < sizeof(man->package[i].mirrors) / sizeof(man->package[i].mirrors[0]); j++)
				if (man->package[i].mirrors[j])
					Con_Printf(" %s", COM_QuotedString(man->package[i].mirrors[j], buffer, sizeof(buffer), false));
			Con_Printf("\n");
		}
	}
}

//forget any mod dirs.
static void FS_Manifest_PurgeGamedirs(ftemanifest_t *man)
{
	int i;
	if (man->filename)
		Z_Free(man->filename);
	man->filename = NULL;

	for (i = 0; i < sizeof(man->gamepath) / sizeof(man->gamepath[0]); i++)
	{
		if (man->gamepath[i].path && !(man->gamepath[i].flags&GAMEDIR_BASEGAME))
		{
			Z_Free(man->gamepath[i].path);
			man->gamepath[i].path = NULL;

			//FIXME: remove packages from the removed paths.
		}
	}
}

//create a new empty manifest with default values.
static ftemanifest_t *FS_Manifest_Create(const char *syspath, const char *basedir)
{
	ftemanifest_t *man = Z_Malloc(sizeof(*man));

	if (syspath)
	{
		char base[MAX_QPATH];
		COM_FileBase(syspath, base, sizeof(base));
		if (*base && Q_strcasecmp(base, "default"))
			man->formalname = Z_StrDup(base);
	}

#ifdef _DEBUG	//FOR TEMPORARY TESTING ONLY.
//	man->doinstall = true;
#endif

	if (syspath)
		man->filename = Z_StrDup(syspath);	//this should be a system path.
	if (basedir)
		man->basedir = Z_StrDup(basedir);	//this should be a system path.

#ifdef QUAKETC
	man->mainconfig = Z_StrDup("config.cfg");
#else
	man->mainconfig = Z_StrDup("fte.cfg");
#endif
	return man;
}

static qboolean FS_Manifest_ParsePackage(ftemanifest_t *man, int packagetype)
{
	//CMD [deparch] packagename qhash [archivedfilename] [prefix skip/this/] [mirror url] [[filesize foo] [sha512 hash] [signature "base64"]]
	char *path = "";
	unsigned int crc = 0;
	qboolean crcknown = false;
	char *packagename = NULL;
	char *legacyextractname = NULL;
	char *condition = NULL;
	char *prefix = NULL;
	char *arch = NULL;
	char *signature = NULL;
	char *sha512 = NULL;
	qofs_t filesize = 0;
	unsigned int arg = 1;
	unsigned int mirrors = 0;
	char *mirror[countof(man->package[0].mirrors)];
	size_t i, j;
	char *a;

	a = Cmd_Argv(0);
	if (!Q_strcasecmp(a, "managedpackage"))
		;
	else
	{
		if (!Q_strcasecmp(a, "filedependancies") || !Q_strcasecmp(a, "archiveddependancies"))
			arch = Cmd_Argv(arg++);

		path = Cmd_Argv(arg++);

#ifdef HAVE_LEGACY
		a = Cmd_Argv(arg);
		if (!strcmp(a, "-"))
		{
			arg++;
		}
		else if (*a)
		{
			crc = strtoul(a, &a, 0);
			if (!*a)
			{
				crcknown = true;
				arg++;
			}
		}

		if (!strncmp(Cmd_Argv(0), "archived", 8))
			legacyextractname = Cmd_Argv(arg++);
#endif
	}

	while (arg < Cmd_Argc())
	{
		a = Cmd_Argv(arg++);
		if (!strcmp(a, "crc"))
		{
			crcknown = true;
			crc = strtoul(Cmd_Argv(arg++), NULL, 0);
		}
		else if (!strcmp(a, "condition"))
			condition = Cmd_Argv(arg++);
		else if (!strcmp(a, "prefix"))
			prefix = Cmd_Argv(arg++);
		else if (!strcmp(a, "arch"))
			arch = Cmd_Argv(arg++);
		else if (!strcmp(a, "signature"))
			signature = Cmd_Argv(arg++);
		else if (!strcmp(a, "sha512"))
			sha512 = Cmd_Argv(arg++);
		else if (!strcmp(a, "name"))
			packagename = Cmd_Argv(arg++);
		else if (!strcmp(a, "filesize")||!strcmp(a, "size"))
			filesize = strtoull(Cmd_Argv(arg++), NULL, 0);
		else if (!strcmp(a, "mirror"))
		{
			a = Cmd_Argv(arg++);
			goto mirror;	//oo evil.
		}
		else if (strchr(a, ':') || man->parsever < 1)
		{
mirror:
			if (mirrors == countof(mirror))
				Con_Printf(CON_WARNING"too many mirrors for package %s\n", path);
			else if (legacyextractname)
			{
				if (!strcmp(legacyextractname, "xz") || !strcmp(legacyextractname, "gz"))
					mirror[mirrors++] = Z_StrDupf("%s:%s", legacyextractname, a);
				else
					mirror[mirrors++] = Z_StrDupf("unzip:%s,%s", legacyextractname, a);
			}
			else
				mirror[mirrors++] = Z_StrDup(a);
		}
		else if (man->parsever <= MANIFEST_CURRENTVER)
			Con_Printf(CON_WARNING"unknown mirror / property %s for package %s\n", a, path);
	}

	for (i = 0; i < countof(man->package); i++)
	{
		if (!man->package[i].path)
		{
			if (packagetype == mdt_singlepackage && (!strchr(path, '/') || strchr(path, ':') || strchr(path, '\\')))
			{
				Con_Printf(CON_WARNING"invalid package path specified in manifest (%s)\n", path);
				break;
			}
			if (arch)
			{
#ifdef PLATFORM
				if (Q_strcasecmp(PLATFORM, arch)
					&& Q_strcasecmp(PLATFORM"_"ARCH_CPU_POSTFIX, arch)
#ifdef ARCH_ALTCPU_POSTFIX
					&& Q_strcasecmp(PLATFORM"_"ARCH_ALTCPU_POSTFIX, arch)
#endif
					)
#endif
				{
					Con_Printf(CON_WARNING"package archetecture does not match %s\n", PLATFORM"_"ARCH_CPU_POSTFIX);
					break;
				}
			}
			man->package[i].packagename = packagename?Z_StrDup(packagename):NULL;
			man->package[i].type = packagetype;
			man->package[i].path = Z_StrDup(path);
			man->package[i].prefix = prefix?Z_StrDup(prefix):NULL;
			man->package[i].condition = condition?Z_StrDup(condition):NULL;
			man->package[i].sha512 = sha512?Z_StrDup(sha512):NULL;
			man->package[i].signature = signature?Z_StrDup(signature):NULL;
			man->package[i].filesize = filesize;
			man->package[i].crcknown = crcknown;
			man->package[i].crc = crc;
			for (j = 0; j < mirrors; j++)
				man->package[i].mirrors[j] = mirror[j];
			return true;
		}
	}
	if (i == countof(man->package))
		Con_Printf(CON_WARNING"Too many packages specified in manifest\n");
	for (j = 0; j < mirrors; j++)
		Z_Free(mirror[j]);
	return false;
}

qboolean FS_GamedirIsOkay(const char *path)
{
	char tmp[MAX_QPATH];
	if (!*path || strchr(path, '\n') || strchr(path, '\r') || !strcmp(path, ".") || !strcmp(path, "..") || strchr(path, ':') || strchr(path, '/') || strchr(path, '\\') || strchr(path, '$'))
	{
		Con_Printf(CON_WARNING"Illegal path specified: %s\n", path);
		return false;
	}

	//don't allow leading dots, hidden files are evil.
	//don't allow complex paths. those are evil too.
	if (!*path || *path == '.' || !strcmp(path, ".") || strstr(path, "..") || strstr(path, "/")
		|| strstr(path, "\\") || strstr(path, ":") || strstr(path, "\""))
	{
		Con_Printf ("Gamedir should be a single filename, not \"%s\"\n", path);
		return false;
	}

	//some gamedirs should never be used for actual games/mods. Reject them.
	if (!Q_strncasecmp(path, "downloads", 9) ||	//QI stuff uses this for arbitrary downloads. it doesn't make sense as a gamedir.
		!Q_strncasecmp(path, "docs", 4) ||		//don't pollute this
		!Q_strncasecmp(path, "help", 4) ||		//don't pollute this
		!Q_strncasecmp(path, "bin", 3) ||		//if scripts try executing stuff here then we want to make extra sure that we don't allow writing anything within it.
		!Q_strncasecmp(path, "lib", 3))			//same deal
	{
		Con_Printf ("Gamedir should not be \"%s\"\n", path);
		return false;
	}

	//this checks for system-specific entries.
	if (!FS_GetCleanPath(path, true, tmp, sizeof(tmp)))
	{
		Con_Printf ("Gamedir should not be \"%s\"\n", path);
		return false;
	}

	return true;
}

//parse Cmd_Argv tokens into the manifest.
static qboolean FS_Manifest_ParseTokens(ftemanifest_t *man)
{
	qboolean result = true;
	char *cmd = Cmd_Argv(0);
	if (!*cmd)
		return result;

	if (*cmd == '*')
		cmd++;

	if (!Q_strcasecmp(cmd, "ftemanifestver") || !Q_strcasecmp(cmd, "ftemanifest"))
		man->parsever = atoi(Cmd_Argv(1));
	else if (!Q_strcasecmp(cmd, "minver"))
	{
		//ignore minimum versions for other engines.
		if (!strcmp(Cmd_Argv(2), DISTRIBUTION))
			man->minver = atoi(Cmd_Argv(3));
	}
	else if (!Q_strcasecmp(cmd, "maxver"))
	{
		//ignore minimum versions for other engines.
		if (!strcmp(Cmd_Argv(2), DISTRIBUTION))
			man->maxver = atoi(Cmd_Argv(3));
	}
	else if (!Q_strcasecmp(cmd, "game"))
	{
		Z_Free(man->installation);
		man->installation = Z_StrDup(Cmd_Argv(1));
	}
	else if (!Q_strcasecmp(cmd, "name"))
	{
		Z_Free(man->formalname);
		man->formalname = Z_StrDup(Cmd_Argv(1));
	}
	else if (!Q_strcasecmp(cmd, "eula"))
	{
		Z_Free(man->eula);
		man->eula = Z_StrDup(Cmd_Argv(1));
	}
#ifdef PACKAGEMANAGER
	else if (!Q_strcasecmp(cmd, "downloadsurl"))
	{
		if (man->downloadsurl)
			Z_StrCat(&man->downloadsurl, " ");
		Z_StrCat(&man->downloadsurl, Cmd_Argv(1));
	}
	else if (!Q_strcasecmp(cmd, "install"))
	{
		if (man->installupd)
			Z_StrCat(&man->installupd, va(";%s", Cmd_Argv(1)));
		else
			man->installupd = Z_StrDup(Cmd_Argv(1));
	}
#endif
	else if (!Q_strcasecmp(cmd, "schemes"))
	{
		int i;
		Z_Free(man->schemes);
		man->schemes = Z_StrDup(Cmd_Argv(1));
		for (i = 2; i < Cmd_Argc(); i++)
			Z_StrCat(&man->schemes, va(" %s", Cmd_Argv(i)));
	}
	else if (!Q_strcasecmp(cmd, "protocolname"))
	{
		Z_Free(man->protocolname);
		man->protocolname = Z_StrDup(Cmd_Argv(1));
	}
	else if (!Q_strcasecmp(cmd, "mainconfig"))
	{
		Z_Free(man->mainconfig);
		if (strcmp(".cfg", COM_GetFileExtension(Cmd_Argv(1),NULL)))
			man->mainconfig = Z_StrDupf("%s.cfg", Cmd_Argv(1));
		else
			man->mainconfig = Z_StrDup(Cmd_Argv(1));
	}
	else if (!Q_strcasecmp(cmd, "defaultexec"))
	{
		Z_Free(man->defaultexec);
		man->defaultexec = Z_StrDup(Cmd_Argv(1));
	}
	else if (!Q_strcasecmp(cmd, "-bind") || !Q_strcasecmp(cmd, "-set") || !Q_strcasecmp(cmd, "-seta") || !Q_strcasecmp(cmd, "-alias") || !Q_strncasecmp(cmd, "-", 1))
	{
		Z_StrCat(&man->defaultexec, va("%s %s\n", Cmd_Argv(0)+1, Cmd_Args()));
	}
	else if (!Q_strcasecmp(cmd, "bind") || !Q_strcasecmp(cmd, "set") || !Q_strcasecmp(cmd, "seta") || !Q_strcasecmp(cmd, "alias") || !Q_strncasecmp(cmd, "+", 1))
	{
		Z_StrCat(&man->defaultoverrides, va("%s %s\n", Cmd_Argv(0), Cmd_Args()));
	}
#ifdef HAVE_LEGACY
	else if (!Q_strcasecmp(cmd, "rtcbroker"))
	{
		Z_StrCat(&man->defaultexec, va("set %s %s\n", net_ice_broker.name, Cmd_Args()));
	}
#endif
	else if (!Q_strcasecmp(cmd, "updateurl"))
	{
		Z_Free(man->updateurl);
		man->updateurl = Z_StrDup(Cmd_Argv(1));
	}
	else if (!Q_strcasecmp(cmd, "icon"))	//relative path to an icon image (typically png)
	{
		Z_Free(man->iconname);
		man->iconname = Z_StrDup(Cmd_Argv(1));
	}
	else if (!Q_strcasecmp(cmd, "disablehomedir") || !Q_strcasecmp(cmd, "homedirmode"))
	{
		char *arg = Cmd_Argv(1);
		if (!Q_strcasecmp(arg, "auto"))
			man->homedirtype = MANIFEST_HOMEDIRWHENREADONLY;
		else if (!*arg || atoi(arg) || !Q_strcasecmp(arg, "never"))
			man->homedirtype = MANIFEST_NOHOMEDIR;
		else if (!atoi(arg) || !Q_strcasecmp(arg, "force") || !Q_strcasecmp(arg, "always"))
			man->homedirtype = MANIFEST_FORCEHOMEDIR;
	}
	else if (!Q_strcasecmp(cmd, "basegame") || !Q_strcasecmp(cmd, "gamedir"))
	{
		int i;
		char *newdir = Cmd_Argv(1);
		qboolean basegame = !Q_strcasecmp(cmd, "basegame");

		for (i = 0; i < sizeof(man->gamepath) / sizeof(man->gamepath[0]); i++)
		{
			if (man->gamepath[i].path)
			{
				if (!Q_strcasecmp(man->gamepath[i].path, newdir))
				{
					if (basegame && !(man->gamepath[i].flags & GAMEDIR_BASEGAME))
					{
						Z_Free(man->gamepath[i].path);
						man->gamepath[i].path = NULL;	//if we're adding a basegame when there's a mod game with the same name then drop the redundant mod name
						man->gamepath[i].flags = 0;
					}
					else
						return true;	//already in there, don't add a conflicting one.
				}
			}
		}

		for (i = 0; i < sizeof(man->gamepath) / sizeof(man->gamepath[0]); i++)
		{
			if (!man->gamepath[i].path)
			{
				man->gamepath[i].flags = GAMEDIR_DEFAULTFLAGS;
				if (!Q_strcasecmp(cmd, "basegame"))
					man->gamepath[i].flags |= GAMEDIR_BASEGAME;

				if (*newdir == '/')
				{
					newdir++;
					man->gamepath[i].flags |= GAMEDIR_QSHACK;
				}
				if (*newdir == '*')
				{	//*dir makes the dir 'private' and not networked.
					newdir++;
					man->gamepath[i].flags |= GAMEDIR_PRIVATE;
					if (!*newdir)
					{	//a single asterisk means load packages from the basedir (but not other files). This is for doom compat.
						man->gamepath[i].flags |= GAMEDIR_USEBASEDIR;
						man->gamepath[i].flags |= GAMEDIR_READONLY;	//must also be readonly, just in case.
						man->gamepath[i].path = Z_StrDup(newdir);
						break;
					}
				}
				if (!strncmp(newdir, "steam:", 6))
				{	//"steam:Subdir/gamedir"
					char *sl = strchr(newdir+6, '/');
					if (!sl)
						break;	//malformed steam link
					man->gamepath[i].flags |= GAMEDIR_PRIVATE|GAMEDIR_STEAMGAME;
					*sl = 0;
					if (!FS_GamedirIsOkay(sl+1))
						break;
					*sl = '/';
					man->gamepath[i].path = Z_StrDup(newdir);
					break;
				}

				if (!FS_GamedirIsOkay(newdir))
					break;
				man->gamepath[i].path = Z_StrDup(newdir);
				break;
			}
		}
		if (i == sizeof(man->gamepath) / sizeof(man->gamepath[0]))
		{
			Con_Printf("Too many game paths specified in manifest\n");
		}
	}
	//FIXME: these should generate package-manager entries.
#ifdef HAVE_LEGACY
	else if (!Q_strcasecmp(cmd, "filedependancies") || !Q_strcasecmp(cmd, "archiveddependancies"))
		FS_Manifest_ParsePackage(man, mdt_installation);
	else if (!Q_strcasecmp(cmd, "archivedpackage"))
		FS_Manifest_ParsePackage(man, mdt_singlepackage);
#endif
	else if (!Q_strcasecmp(cmd, "library"))
		FS_Manifest_ParsePackage(man, mdt_installation);
	else if (!Q_strcasecmp(cmd, "package") || !Q_strcasecmp(cmd, "archivedpackage"))
		FS_Manifest_ParsePackage(man, mdt_singlepackage);
	else if (!Q_strcasecmp(cmd, "basedir"))
	{	//allow explicit basedirs when this is an actual file on the user's system, and we don't have an explicit one.
		//this should only happen from parsing /etc/xdg/games/*.fmf
		if (!man->basedir && man->filename)
			man->basedir = Z_StrDup(Cmd_Argv(1));
	}
	else
	{
		Con_Printf("Unknown token: %s\n", cmd);
		result = false;
	}
	return result;
}
//if the manifest omits some expected stuff, give it some defaults to match known game names (so fmf files can defer to the engine instead of having to be maintained separately).
static void FS_Manifest_SetDefaultSettings(ftemanifest_t *man, const gamemode_info_t *game)
{
	int j;

	if (game)
	{
		//if there's no base dirs, edit the manifest to give it its default ones.
		for (j = 0; j < sizeof(man->gamepath) / sizeof(man->gamepath[0]); j++)
		{
			if (man->gamepath[j].path && (man->gamepath[j].flags&GAMEDIR_BASEGAME))
				break;
		}
		if (j == sizeof(man->gamepath) / sizeof(man->gamepath[0]))
		{
			for (j = 0; j < 4; j++)
				if (game->dir[j])
				{
					Cmd_TokenizeString(va("basegame \"%s\"", game->dir[j]), false, false);
					FS_Manifest_ParseTokens(man);
				}
		}

		if (!man->schemes)
		{
			Cmd_TokenizeString(va("schemes \"%s\"", game->argname+1), false, false);
			FS_Manifest_ParseTokens(man);
		}

#ifdef PACKAGEMANAGER
		if (!man->downloadsurl && game->downloadsurl)
		{
#ifndef FTE_TARGET_WEB
			if (*game->downloadsurl == '/')
			{
				conchar_t musite[256], *e;
				char site[256];
				char *oldprefix = "http://fte.";
				char *newprefix = "https://updates.";
				e = COM_ParseFunString(CON_WHITEMASK, ENGINEWEBSITE, musite, sizeof(musite), false);
				COM_DeFunString(musite, e, site, sizeof(site)-1, true, true);
				if (!strncmp(site, oldprefix, strlen(oldprefix)))
				{
					memmove(site+strlen(newprefix), site+strlen(oldprefix), strlen(site)-strlen(oldprefix)+1);
					memcpy(site, newprefix, strlen(newprefix));
				}
				man->downloadsurl = Z_StrDupf("%s%s", site, game->downloadsurl);
			}
			else
#endif
				man->downloadsurl = Z_StrDup(game->downloadsurl);
			FS_Manifest_ParseTokens(man);
		}
		if (!man->installupd && game->needpackages)
			man->installupd = Z_StrDup(game->needpackages);
#endif

		if (!man->protocolname)
			man->protocolname = Z_StrDup(game->protocolname);

		if (!man->defaultexec && game->customexec)
		{
			const char *e = game->customexec;
			while (e[0] == '/' && e[1] == '/')
			{
				e+=2;
				while(*e)
				{
					if (*e++ == '\n')
						break;
				}
			}
			man->defaultexec = Z_StrDup(e);
		}

		if (!man->formalname)
			man->formalname = Z_StrDup(game->poshname);
	}


	if (!man->formalname && man->installation && *man->installation)
		man->formalname = Z_StrDup(man->installation);
	else if (!man->formalname)
		man->formalname = Z_StrDup(FULLENGINENAME);
}
//read a manifest file
ftemanifest_t *FS_Manifest_ReadMem(const char *fname, const char *basedir, const char *data)
{
	int ver;
	int i;
	ftemanifest_t *man;
	if (!data)
		return NULL;
	while (*data == ' ' || *data == '\t' || *data == '\r' || *data == '\n')
		data++;
	if (!*data)
		return NULL;

	man = FS_Manifest_Create(fname, basedir);

	while (data && *data)
	{
		data = Cmd_TokenizeString(data, false, false);
		if (!FS_Manifest_ParseTokens(man) && man->parsever <= 1)
		{
			FS_Manifest_Free(man);
			return NULL;
		}
	}
	if (!man->installation)
	{	//every manifest should have an internal name specified, so we can guess the correct basedir
		//if we don't recognise it, then we'll typically prompt (or just use the working directory), but always assuming a default at least ensures things are sane.
		//fixme: we should probably fill in the basegame here (and share that logic with the legacy manifest generation code)
#ifdef BRANDING_NAME
		data = Cmd_TokenizeString((char*)"game "STRINGIFY(BRANDING_NAME), false, false);
#else
		data = Cmd_TokenizeString((char*)"game quake", false, false);
#endif
		FS_Manifest_ParseTokens(man);
	}
	if (man->installation)
	{	//if we know about it, fill in some defaults...
		for (i = 0; gamemode_info[i].argname; i++)
		{
			if (!strcmp(man->installation, gamemode_info[i].argname+1))
			{
				FS_Manifest_SetDefaultSettings(man, &gamemode_info[i]);
				break;
			}
		}
	}

	//svnrevision is often '-', which means we can't just use it as a constant.
	ver = revision_number(false);
	if (ver && (man->minver > ver || (man->maxver && man->maxver < ver)))
	{
		FS_Manifest_Free(man);
		return NULL;
	}
	return man;
}

ftemanifest_t *FS_Manifest_ReadSystem(const char *fname, const char *basedir)
{
	ftemanifest_t *man = NULL;
	vfsfile_t *f;
	f = VFSOS_Open(fname, "rb");
	if (f)
	{
		size_t len = VFS_GETLEN(f);
		char *fdata = BZ_Malloc(len+1);
		if (fdata)
		{
			VFS_READ(f, fdata, len);
			fdata[len] = 0;
			man = FS_Manifest_ReadMem(fname, basedir, fdata);
			if (man)
				man->security = MANIFEST_SECURITY_DEFAULT;
			BZ_Free(fdata);
		}
		VFS_CLOSE(f);
	}
	return man;
}
//reads eg $homedir/$moddir.fmf or $basedir/$moddir.fmf as appropriate.
ftemanifest_t *FS_Manifest_ReadMod(const char *moddir)
{
	ftemanifest_t *man = NULL;
	char path[MAX_OSPATH];
	if (*moddir)
	{
		//check the homedir, which is a little messy when manifests might disallow themselves...
		if (!man && com_homepathusable)
		{
			Q_snprintfz(path, sizeof(path), "%s%s", com_homepath, moddir);
			COM_RequireExtension(path, ".fmf", sizeof(path));
			man = FS_Manifest_ReadSystem(path, com_gamepath);
			if (man && man->homedirtype == MANIFEST_NOHOMEDIR)
			{
				FS_Manifest_Free(man);	//manifest doesn't like itself... pretend to not find it.
				man = NULL;
			}
		}

		if (!man)
		{
			Q_snprintfz(path, sizeof(path), "%s%s", com_gamepath, moddir);
			COM_RequireExtension(path, ".fmf", sizeof(path));
			man = FS_Manifest_ReadSystem(path, com_gamepath);
		}
	}
	return man;
}

//======================================================================================================


static char *fs_loadedcommand;			//execed once all packages are (down)loaded
ftemanifest_t	*fs_manifest;	//currently active manifest.
static searchpath_t	*com_searchpaths;
static searchpath_t	*com_purepaths;
static searchpath_t	*com_base_searchpaths;	// without gamedirs

static int fs_puremode;				//0=deprioritise pure, 1=prioritise pure, 2=pure only.
static char *fs_refnames;			//list of allowed packages
static char *fs_refcrcs;			//list of crcs for those packages. one token per package.
static char *fs_purenames;			//list of allowed packages
static char *fs_purecrcs;			//list of crcs for those packages. one token per package.
static unsigned int fs_pureseed;	//used as a key so the server knows we're obeying. completely unreliable/redundant in an open source project, but needed for q3 network compat.

char *FS_AbbreviateSize(char *buf, size_t bufsize, qofs_t fsize)
{
	//caps mean kibi instead of kilo, and bytes not bits.
	if (fsize > 512.0*1024*1024*1024)
		Q_snprintfz(buf, bufsize, "%gTB", fsize/(1024.0*1024*1024*1024));
	else if (fsize > 512.0*1024*1024)
		Q_snprintfz(buf, bufsize, "%gGB", fsize/(1024.0*1024*1024));
	else if (fsize > 512.0*1024)
		Q_snprintfz(buf, bufsize, "%gMB", fsize/(1024.0*1024));
	else if (fsize > 512.0)
		Q_snprintfz(buf, bufsize, "%gKB", fsize/1024.0);
	else
		Q_snprintfz(buf, bufsize, "%"PRIuQOFS"B", fsize);
	return buf;
}

int QDECL COM_FileSize(const char *path)
{
	flocation_t loc;
	if (FS_FLocateFile(path, FSLF_IFFOUND, &loc))
		return loc.len;
	else
		return -1;
}

//appends a / on the end of the directory if it does not already have one.
static void FS_CleanDir(char *out, int outlen)
{
	int olen = strlen(out);
	if (!olen || olen >= outlen-1)
		return;

	if (out[olen-1] == '\\')
		out[olen-1] = '/';
	else if (out[olen-1] != '/')
	{
		out[olen+1] = '\0';
		out[olen] = '/';
	}
}

/*
============
COM_Path_f

============
*/
static void COM_PathLine(searchpath_t *s)
{
	char displaypath[MAX_OSPATH];
	char *col = "";
	if (s->flags&SPF_ISDIR)
		col = S_COLOR_GREEN;
	else if (s->flags&SPF_COPYPROTECTED)
		col = S_COLOR_BLUE;
	else
		col = S_COLOR_CYAN;
	FS_DisplayPath(s->logicalpath, FS_SYSTEM, displaypath,sizeof(displaypath));
	Con_Printf(" %s" U8("%s")"  %s%s%s%s%s%s%s%s\n", col, displaypath,
		(s->flags & SPF_REFERENCED)?"^[(ref)\\tip\\Referenced\\desc\\Package will auto-download to clients^]":"",
		(s->flags & SPF_TEMPORARY)?"^[(temp)\\tip\\Temporary\\desc\\Flushed on map change^]":"",
		(s->flags & SPF_SERVER)?"^[(srv)\\tip\\Server-Specified\\desc\\Loaded to match the server, closed on disconnect^]":"",
		(s->flags & SPF_COPYPROTECTED)?"^[(c)\\tip\\Copyrighted\\desc\\Copy-Protected and is not downloadable^]":"",
		(s->flags & SPF_EXPLICIT)?"^[(e)\\tip\\Explicit\\desc\\Loaded explicitly by the gamedir^]":"",
		(s->flags & SPF_UNTRUSTED)?"^[(u)\\tip\\Untrusted\\desc\\Configs and scripts will not be given access to passwords^]":"",
		(s->flags & SPF_WRITABLE)?"^[(w)\\tip\\Writable\\desc\\We can probably write here^]":"",
		(s->handle->GeneratePureCRC)?va("^[(h)\\tip\\Hash: %x^]", s->crc_check):"");
}
qboolean FS_GameIsInitialised(void)
{
	if (!com_searchpaths && !com_purepaths)
		return false;
	return true;
}
static void COM_Path_f (void)
{
	searchpath_t	*s;

	if (!FS_GameIsInitialised())
	{
		Con_Printf("File system not initialised\n");
		Con_Printf("gamedirfile: \"%s\"\n", gamedirfile);
		Con_Printf("pubgamedirfile: \"%s\"\n", pubgamedirfile);
		Con_Printf("com_gamepath: \"%s\"\n", com_gamepath);
		Con_Printf("com_homepath: \"%s\" (enabled: %s, usable: %s)\n", com_homepath, com_homepathenabled?"yes":"no", com_homepathusable?"yes":"no");
//		Con_Printf("com_configdir: \"%s\"\n", com_configdir);
		if (fs_manifest)
			FS_Manifest_Print(fs_manifest);
		return;
	}

	if (fs_hidesyspaths.ival)
		Con_Printf("External paths are hidden, ^[click to unhide\\type\\set fs_hidesyspaths 0;path^]\n");

	if (com_purepaths || fs_puremode)
	{
		Con_Printf ("Pure paths:\n");
		for (s=com_purepaths ; s ; s=s->nextpure)
		{
			COM_PathLine(s);
		}
		if (fs_puremode == 2)
			Con_Printf ("Inactive paths:\n");
		else
			Con_Printf ("Impure paths:\n");
	}
	else
		Con_TPrintf ("Current search path:\n");


	for (s=com_searchpaths ; s ; s=s->next)
	{
		if (s == com_base_searchpaths)
			Con_Printf (" ----------\n");

		COM_PathLine(s);
	}



	if (fs_purenames && fs_purecrcs)
	{
		char crctok[64];
		char nametok[MAX_QPATH];

		int crc;
		char *pc = fs_purecrcs;
		char *pn = fs_purenames;
		for (;;)
		{
			pc = COM_ParseOut(pc, crctok, sizeof(crctok));
			pn = COM_ParseOut(pn, nametok, sizeof(nametok));
			if (!pc || !pn)
				break;
			crc = strtoul(crctok, NULL, 0);

			for (s=com_searchpaths ; s ; s=s->next)
			{
				if (s && s->crc_check == crc)
					break;
			}
			if (!s)
			{
				COM_DefaultExtension(nametok, ".pk3", sizeof(nametok));
				if (*nametok == '*')
					Con_Printf(CON_ERROR "MISSING: " U8("%s")"  (%x)\n", nametok+1, crc);
				else
					Con_Printf(CON_WARNING "MISSING: " U8("%s")"  (%x)\n", nametok, crc);
			}
		}
	}
}


/*
============
COM_Dir_f

============
*/
static int QDECL COM_Dir_List(const char *name, qofs_t size, time_t mtime, void *parm, searchpathfuncs_t *spath)
{
	searchpath_t	*s;
	const char *ext;
	char link[512];
	char pkgname[MAX_OSPATH];
	char szbuf[16];
	char *colour;
	flocation_t loc;
	for (s=com_searchpaths ; s ; s=s->next)
	{
		if (s->handle == spath)
			break;
	}

	if (*name && name[strlen(name)-1] == '/')
	{
		colour = "^7";	//superseeded
		Q_snprintfz(link, sizeof(link), "\\tip\\Scan Sub-Directory\\dir\\%s*", name);
	}
	else if (!FS_FLocateFile(name, FSLF_IFFOUND, &loc))
	{
		colour = "^1";	//superseeded
		Q_snprintfz(link, sizeof(link), "\\tip\\flocate error");
	}
	else if (loc.search->handle == spath || (fs_allowfileuri&&loc.search == fs_allowfileuri))
	{
		colour = "^2";

		ext = COM_GetFileExtension(name, NULL);
		if (!Q_strcasecmp(ext, ".gz") || !Q_strcasecmp(ext, ".xz"))
			ext = COM_GetFileExtension(name, ext);
		if (*ext == '.')
		{
			ext++;
			if (strchr(ext, '.'))
			{
				COM_StripAllExtensions(ext, link, sizeof(link));
				ext = link;
			}
		}

		if ((!Q_strcasecmp(ext, "bsp") || !Q_strcasecmp(ext, "d3dbsp") || !Q_strcasecmp(ext, "map") || !Q_strcasecmp(ext, "hmp")) && !strncmp(name, "maps/", 5) && strncmp(name, "maps/b_", 7))
		{
			Q_snprintfz(link, sizeof(link), "\\tip\\Change Map\\map\\%s", name+5);
			colour = "^4";	//disconnects
		}
#if !defined(NOBUILTINMENUS) && !defined(MINIMAL)
		else if (!Q_strcasecmp(ext, "bsp") || !Q_strcasecmp(ext, "spr") || !Q_strcasecmp(ext, "sp2") || !Q_strcasecmp(ext, "mdl") || !Q_strcasecmp(ext, "md3") || !Q_strcasecmp(ext, "iqm") ||
				 !Q_strcasecmp(ext, "vvm") || !Q_strcasecmp(ext, "psk") || !Q_strcasecmp(ext, "dpm") || !Q_strcasecmp(ext, "zym") || !Q_strcasecmp(ext, "md5mesh") ||
				 !Q_strcasecmp(ext, "mdx") || !Q_strcasecmp(ext, "md2") || !Q_strcasecmp(ext, "obj") || !Q_strcasecmp(ext, "mds") || !Q_strcasecmp(ext, "mdc") ||
				 !Q_strcasecmp(ext, "md5anim") || !Q_strcasecmp(ext, "gltf") || !Q_strcasecmp(ext, "glb") || !Q_strcasecmp(ext, "ase") || !Q_strcasecmp(ext, "lwo") ||
				 ((!Q_strncasecmp(name, "xmodel/", 7)||!Q_strncasecmp(name, "xanim/", 6)) && !Q_strcasecmp(ext, ""))	//urgh
				 )
			Q_snprintfz(link, sizeof(link), "\\tip\\Open in Model Viewer\\modelviewer\\%s", name);
#endif
#ifdef TEXTEDITOR
		else if (!Q_strcasecmp(ext, "qc") || !Q_strcasecmp(ext, "src") || !Q_strcasecmp(ext, "qh") || !Q_strcasecmp(ext, "h") || !Q_strcasecmp(ext, "c") ||
				!Q_strcasecmp(ext, "cfg") || !Q_strcasecmp(ext, "rc") ||
				!Q_strcasecmp(ext, "txt") || !Q_strcasecmp(ext, "log") ||
				!Q_strcasecmp(ext, "ent") || !Q_strcasecmp(ext, "rtlights") ||
				!Q_strcasecmp(ext, "glsl") || !Q_strcasecmp(ext, "hlsl") ||
				!Q_strcasecmp(ext, "shader") || !Q_strcasecmp(ext, "framegroups") ||
				!Q_strcasecmp(ext, "vmt") || !Q_strcasecmp(ext, "skin"))
			Q_snprintfz(link, sizeof(link), "\\tip\\Open in Text Editor\\edit\\%s", name);
#endif
		else if (!Q_strcasecmp(ext, "tga") || !Q_strcasecmp(ext, "png") || !Q_strcasecmp(ext, "jpg") || !Q_strcasecmp(ext, "jpeg")|| !Q_strcasecmp(ext, "lmp") || !Q_strcasecmp(ext, "ico") ||
				 !Q_strcasecmp(ext, "pcx") || !Q_strcasecmp(ext, "bmp") || !Q_strcasecmp(ext, "dds") || !Q_strcasecmp(ext, "ktx") || !Q_strcasecmp(ext, "ktx2")|| !Q_strcasecmp(ext, "vtf") ||
				 !Q_strcasecmp(ext, "astc")|| !Q_strcasecmp(ext, "htga")|| !Q_strcasecmp(ext, "exr") || !Q_strcasecmp(ext, "xcf") || !Q_strcasecmp(ext, "psd") || !Q_strcasecmp(ext, "iwi") ||
				 !Q_strcasecmp(ext, "pbm") || !Q_strcasecmp(ext, "ppm") || !Q_strcasecmp(ext, "pgm") || !Q_strcasecmp(ext, "pam") || !Q_strcasecmp(ext, "pfm") || !Q_strcasecmp(ext, "hdr") ||
				 !Q_strcasecmp(ext, "wal") )
		{
			//FIXME: image replacements are getting in the way here.
			Q_snprintfz(link, sizeof(link), "\\tiprawimg\\%s\\tip\\(note: image replacement rules are context-dependant, including base path, sub path, extension, or complete replacement via a shader)", name);
			colour = "^6";	//shown on mouseover
		}
		else if (!Q_strcasecmp(ext, "qwd") || !Q_strcasecmp(ext, "dem") || !Q_strcasecmp(ext, "mvd") || !Q_strcasecmp(ext, "dm2"))
		{
			Q_snprintfz(link, sizeof(link), "\\tip\\Play Demo\\demo\\%s", name);
			colour = "^4";	//disconnects
		}
		else if (!Q_strcasecmp(ext, "roq") || !Q_strcasecmp(ext, "cin") || !Q_strcasecmp(ext, "avi") || !Q_strcasecmp(ext, "mp4") || !Q_strcasecmp(ext, "mkv") || !Q_strcasecmp(ext, "ogv"))
			Q_snprintfz(link, sizeof(link), "\\tip\\Play Film\\film\\%s", name);
		else if (!Q_strcasecmp(ext, "wav") || !Q_strcasecmp(ext, "ogg") || !Q_strcasecmp(ext, "mp3") || !Q_strcasecmp(ext, "opus") || !Q_strcasecmp(ext, "flac"))
			Q_snprintfz(link, sizeof(link), "\\tip\\Play Audio\\playaudio\\%s", name);
		else
		{
			colour = "^3";	//nothing
			*link = 0;
		}
	}
	else
	{
		char *gah;
		if (FS_DisplayPath(loc.search->logicalpath, FS_SYSTEM, pkgname,sizeof(pkgname)))
			;
		else
			Q_strncpyz(pkgname, loc.search->purepath, sizeof(pkgname));

		colour = "^1";	//superseeded
		Q_snprintfz(link, sizeof(link), "\\tip\\overriden by file from %s", pkgname);
		gah = link + 20;	//whatever
		while ((gah = strchr(gah, '\\')))
			*gah = '/';
	}

	if (s && FS_DisplayPath(s->logicalpath, FS_SYSTEM, pkgname,sizeof(pkgname)))
		;
	else
		Q_strncpyz(pkgname, "??", sizeof(pkgname));

	Con_Printf(U8("(%s) ^[%s%s%s^] \t^h(%s)\n"), FS_AbbreviateSize(szbuf,sizeof(szbuf), size), colour, name, link, pkgname);
	return 1;
}

static void COM_Dir_f (void)
{
	char match[MAX_QPATH];

	if (Cmd_Argc()>1)
		Q_strncpyz(match, Cmd_Argv(1), sizeof(match));
	else
		Q_strncpyz(match, "*", sizeof(match));

	if (Cmd_Argc()>2)
	{
		strncat(match, "/*.", sizeof(match)-1);
		match[sizeof(match)-1] = '\0';
		strncat(match, Cmd_Argv(2), sizeof(match)-1);
		match[sizeof(match)-1] = '\0';
	}
//	else
//		strncat(match, "/*", sizeof(match)-1);

	COM_EnumerateFiles(match, COM_Dir_List, NULL);
}

/*
============
COM_Locate_f

============
*/
static void COM_Locate_f (void)
{
	char pkgname[MAX_OSPATH];
	flocation_t loc;
	char *f = Cmd_Argv(1);
	if (strchr(f, '^'))	//fte's filesystem is assumed to be utf-8, but that doesn't mean that console input is. and I'm too lazy to utf-8ify the string (in part because markup can be used to exploit ascii assumptions).
		Con_Printf("Warning: filename contains markup. If this is because of unicode, set com_parseutf8 1\n");
	if (FS_FLocateFile(f, FSLF_IFFOUND, &loc))
	{
		if (FS_DisplayPath(loc.search->logicalpath, FS_SYSTEM, pkgname,sizeof(pkgname)))
			;
		else
			Q_strncpyz(pkgname, "??", sizeof(pkgname));
		if (!*loc.rawname)
		{
			Con_Printf("File is %u bytes compressed inside "U8("%s")"\n", (unsigned)loc.len, pkgname);
		}
		else
		{
			Con_Printf("Inside "U8("%s")" (%u bytes)\n  "U8("%s")"\n", loc.rawname, (unsigned)loc.len, pkgname);
		}
	}
	else
		Con_Printf("Not found\n");
}

static void COM_CalcHash_Thread(void *ctx, void *fname, size_t a, size_t b)
{
	int h;
	struct
	{
		const char *name;
		hashfunc_t *hash;
		void *ctx;
	} hashes[] =
	{
//		{"crc16", &hash_crc16},
//		{"md4", &hash_md4},
#if defined(HAVE_SERVER) || defined(HAVE_CLIENT)
		{"md5", &hash_md5},
#endif
		{"sha1", &hash_sha1},
#if defined(HAVE_SERVER) || defined(HAVE_CLIENT)
//		{"sha224", &hash_sha2_224},
		{"sha256", &hash_sha2_256},
//		{"sha384", &hash_sha2_384},
//		{"sha512", &hash_sha2_512},
#endif
	};
	qbyte digest[DIGEST_MAXSIZE];
	qbyte digesttext[DIGEST_MAXSIZE*2+1];
	qbyte block[65536];
	int csize;
	quint64_t fsize = 0;
	quint64_t tsize = 0;
	unsigned int pct, opct=~0;
	vfsfile_t *f = FS_OpenVFS(fname, "rb", FS_GAME);

	if (f)
	{
		tsize = VFS_GETLEN(f);
		Con_Printf("%s: Processing...\r", (char*)fname);

		for (h = 0; h < countof(hashes); h++)
		{
			hashes[h].ctx = Z_Malloc(hashes[h].hash->contextsize);
			hashes[h].hash->init(hashes[h].ctx);
		}

		for(;;)
		{
			csize = VFS_READ(f, block, sizeof(block));
			if (csize <= 0)
				break;
			fsize += csize;
			for (h = 0; h < countof(hashes); h++)
			{
				hashes[h].hash->process(hashes[h].ctx, block, csize);
			}
			pct = (100*fsize)/tsize;
			if (pct != opct)
			{
				Con_Printf("%s: %i%%...\r", (char*)fname, pct);
				opct = pct;
			}
		}

		VFS_CLOSE(f);

		Con_Printf("%s: %s\n", (char*)fname, FS_AbbreviateSize(block,sizeof(block), fsize));
		for (h = 0; h < countof(hashes); h++)
		{
			hashes[h].hash->terminate(digest, hashes[h].ctx);
			Z_Free(hashes[h].ctx);

			digesttext[Base16_EncodeBlock(digest, hashes[h].hash->digestsize, digesttext, sizeof(digesttext)-1)] = 0;
			Con_Printf("  %s: %s\n", hashes[h].name, digesttext);
		}
	}
	Z_Free(fname);
}
static void COM_CalcHash_f(void)
{
	if (Cmd_Argc() != 2)
		Con_Printf("%s <FILENAME>: computes various hashes of the specified file\n", Cmd_Argv(0));
	else
		COM_AddWork(WG_LOADER, COM_CalcHash_Thread, NULL, Z_StrDup(Cmd_Argv(1)), 0, 0);
}

/*
============
COM_WriteFile

The filename will be prefixed by the current game directory
============
*/
qboolean COM_WriteFile (const char *filename, enum fs_relative fsroot, const void *data, int len)
{
	qboolean success = false;
	vfsfile_t *vfs;

	Sys_Printf ("COM_WriteFile: %s\n", filename);

	FS_CreatePath(filename, fsroot);
	vfs = FS_OpenVFS(filename, "wb", fsroot);
	if (vfs)
	{
		VFS_WRITE(vfs, data, len);
		success = VFS_CLOSE(vfs);

		if (fsroot >= FS_GAME)
			FS_FlushFSHashWritten(filename);
		else
			com_fschanged=true;
	}
	return success;
}

/*
============
COM_CreatePath

Only used for CopyFile and download
============
*/
static void	COM_CreatePath (char *path)
{
	char	*ofs;

	if (fs_readonly)
		return;

	for (ofs = path+1 ; *ofs ; ofs++)
	{
		if (*ofs == '/')
		{	// create the directory
			*ofs = 0;
			Sys_mkdir (path);
			*ofs = '/';
		}
	}
}


/*
===========
COM_CopyFile

Copies a file over from the net to the local cache, creating any directories
needed.  This is for the convenience of developers using ISDN from home.
===========
*/
/*
static void COM_CopyFile (char *netpath, char *cachepath)
{
	FILE	*in, *out;
	int		remaining, count;
	char	buf[4096];

	remaining = COM_FileOpenRead (netpath, &in);
	COM_CreatePath (cachepath);	// create directories up to the cache file
	out = fopen(cachepath, "wb");
	if (!out)
		Sys_Error ("Error opening %s", cachepath);

	while (remaining)
	{
		if (remaining < sizeof(buf))
			count = remaining;
		else
			count = sizeof(buf);
		fread (buf, 1, count, in);
		fwrite (buf, 1, count, out);
		remaining -= count;
	}

	fclose (in);
	fclose (out);
}
//*/

int fs_hash_dups;
int fs_hash_files;

//normally the filesystem drivers pass a pre-allocated bucket and static strings to us
//the OS driver can't really be expected to track things that reliably however, so it just gives names via the stack.
//these files are grouped up to avoid excessive memory allocations.
struct fsbucketblock
{
	struct fsbucketblock *prev;
	int used;
	int total;
	qbyte data[1];
};
static struct fsbucketblock *fs_hash_filebuckets;

static void FS_FlushFSHashReally(qboolean domutexes)
{
	COM_AssertMainThread("FS_FlushFSHashReally");
	if (!domutexes || Sys_LockMutex(fs_thread_mutex))
	{
		com_fschanged = true;

		if (filesystemhash.numbuckets)
		{
			int i;
			for (i = 0; i < filesystemhash.numbuckets; i++)
				filesystemhash.bucket[i] = NULL;
		}

		while (fs_hash_filebuckets)
		{
			struct fsbucketblock *n = fs_hash_filebuckets->prev;
			Z_Free(fs_hash_filebuckets);
			fs_hash_filebuckets = n;
		}

		if (domutexes)
			Sys_UnlockMutex(fs_thread_mutex);
	}
}

static void QDECL FS_AddFileHashUnsafe(int depth, const char *fname, fsbucket_t *filehandle, void *pathhandle)
{
	//threading stuff is fucked.
	fsbucket_t *old;

	old = Hash_GetInsensitiveBucket(&filesystemhash, fname);

	if (old)
	{
		fs_hash_dups++;
		if (depth >= old->depth)
		{
			return;
		}

		//remove the old version
		//FIXME: needs to be atomic. just live with multiple in there.
		//Hash_RemoveBucket(&filesystemhash, fname, &old->buck);
	}

	if (!filehandle)
	{
		int nlen = strlen(fname)+1;
		int plen = sizeof(*filehandle)+nlen;
		plen = (plen+fte_alignof(fsbucket_t)-1) & ~(fte_alignof(fsbucket_t)-1);
		if (!fs_hash_filebuckets || fs_hash_filebuckets->used+plen > fs_hash_filebuckets->total)
		{
			void *o = fs_hash_filebuckets;
			fs_hash_filebuckets = Z_Malloc(65536);
			fs_hash_filebuckets->total = 65536 - sizeof(*fs_hash_filebuckets);
			fs_hash_filebuckets->prev = o;
		}
		filehandle = (fsbucket_t*)(fs_hash_filebuckets->data+fs_hash_filebuckets->used);
		fs_hash_filebuckets->used += plen;

		if (!filehandle)
			return;	//eep!
		memcpy((char*)(filehandle+1), fname, nlen);
		fname = (char*)(filehandle+1);
	}
	filehandle->depth = depth;

	Hash_AddInsensitive(&filesystemhash, fname, pathhandle, &filehandle->buck);
	fs_hash_files++;
}
static void QDECL FS_AddFileHash(int depth, const char *fname, fsbucket_t *filehandle, void *pathhandle)
{
	fsbucket_t *old;

	old = Hash_GetInsensitiveBucket(&filesystemhash, fname);

	if (old)
	{
		fs_hash_dups++;
		if (depth >= old->depth)
		{
			return;
		}

		//remove the old version
		Hash_RemoveBucket(&filesystemhash, fname, &old->buck);
	}

	if (!filehandle)
	{
		int nlen = strlen(fname)+1;
		int plen = sizeof(*filehandle)+nlen;
		plen = (plen+fte_alignof(fsbucket_t)-1) & ~(fte_alignof(fsbucket_t)-1);
		if (!fs_hash_filebuckets || fs_hash_filebuckets->used+plen > fs_hash_filebuckets->total)
		{
			void *o = fs_hash_filebuckets;
			fs_hash_filebuckets = Z_Malloc(65536);
			fs_hash_filebuckets->total = 65536 - sizeof(*fs_hash_filebuckets);
			fs_hash_filebuckets->prev = o;
		}
		filehandle = (fsbucket_t*)(fs_hash_filebuckets->data+fs_hash_filebuckets->used);
		fs_hash_filebuckets->used += plen;

		if (!filehandle)
			return;	//eep!
		memcpy((char*)(filehandle+1), fname, nlen);
		fname = (char*)(filehandle+1);
	}
	filehandle->depth = depth;

	Hash_AddInsensitive(&filesystemhash, fname, pathhandle, &filehandle->buck);
	fs_hash_files++;
}

#ifndef FTE_TARGET_WEB
static void FS_RebuildFSHash(qboolean domutex)
{
	int depth = 1;
	searchpath_t	*search;
	if (!com_fschanged)
		return;

	COM_AssertMainThread("FS_RebuildFSHash");
	if (domutex && !Sys_LockMutex(fs_thread_mutex))
		return;	//amg!

	if (com_fsneedreload)
		FS_ReloadPackFilesFlags(~0);

	if (!filesystemhash.numbuckets)
	{
		filesystemhash.numbuckets = 1024;
		filesystemhash.bucket = (bucket_t**)Z_Malloc(Hash_BytesForBuckets(filesystemhash.numbuckets));
	}
	else
	{
		FS_FlushFSHashReally(false);
	}
	Hash_InitTable(&filesystemhash, filesystemhash.numbuckets, filesystemhash.bucket);

	fs_hash_dups = 0;
	fs_hash_files = 0;

	if (com_purepaths)
	{	//go for the pure paths first.
		for (search = com_purepaths; search; search = search->nextpure)
		{
			search->handle->BuildHash(search->handle, depth++, FS_AddFileHash);
		}
	}
	if (fs_puremode < 2)
	{
		for (search = com_searchpaths ; search ; search = search->next)
		{
			search->handle->BuildHash(search->handle, depth++, FS_AddFileHash);
		}
	}

	com_fschanged = false;
	com_fsneedreload = false;

	if (domutex)
		Sys_UnlockMutex(fs_thread_mutex);

	Con_DPrintf("%i unique files, %i duplicates\n", fs_hash_files, fs_hash_dups);
}
#endif

static void FS_RebuildFSHash_Update(const char *fname)
{
	flocation_t loc;
	searchpath_t *search;
	int depth = 0;
	fsbucket_t *old;
	void *filehandle = NULL;

	if (com_fschanged)
		return;

	if (!filehandle && com_purepaths)
	{	//go for the pure paths first.
		for (search = com_purepaths; search; search = search->nextpure)
		{
			if (search->handle->FindFile(search->handle, &loc, fname, NULL))
			{
				filehandle = loc.fhandle;
				break;
			}
			depth++;
		}
	}
	if (!filehandle && fs_puremode < 2)
	{
		for (search = com_searchpaths ; search ; search = search->next)
		{
			if (search->handle->FindFile(search->handle, &loc, fname, NULL))
			{
				filehandle = loc.fhandle;
				break;
			}
			depth++;
		}
	}


	COM_WorkerLock();
	if (!Sys_LockMutex(fs_thread_mutex))
		return;	//amg!

	old = Hash_GetInsensitiveBucket(&filesystemhash, fname);
	if (old)
	{
		Hash_RemoveBucket(&filesystemhash, fname, &old->buck);
		fs_hash_files--;
	}

	if (filehandle)
		FS_AddFileHash(depth, fname, NULL, filehandle);

	Sys_UnlockMutex(fs_thread_mutex);
	COM_WorkerUnlock();
}

void FS_FlushFSHashWritten(const char *fname)
{
	FS_RebuildFSHash_Update(fname);
}
void FS_FlushFSHashRemoved(const char *fname)
{
	FS_RebuildFSHash_Update(fname);
}
void FS_FlushFSHashFull(void)
{	//any calls to this are typically a bug...
	//that said, figuring out if the file was actually within quake's filesystem isn't easy.
	com_fschanged = true;

	//for safety we would need to sync with all threads, so lets just not bother.
	//FS_FlushFSHashReally(true);
}


/*
===========
COM_FindFile

Finds the file in the search path.
Sets com_filesize and one of handle or file
===========
*/
//if loc is valid, loc->search is always filled in, the others are filled on success.
//returns 0 if couldn't find.
int FS_FLocateFile(const char *filename, unsigned int lflags, flocation_t *loc)
{
	int depth=0;
	searchpath_t	*search;
	char cleanpath[MAX_OSPATH];
	flocation_t allownoloc;

	void *pf;
	unsigned int found = FF_NOTFOUND;

	if (!loc)
		loc = &allownoloc;

	loc->fhandle = NULL;
	loc->offset = 0;
	*loc->rawname = 0;
	loc->search = NULL;
	loc->len = -1;

	if (!strncmp(filename, "file:", 5))
	{
		if (fs_allowfileuri && Sys_ResolveFileURL(filename, strlen(filename), cleanpath, sizeof(cleanpath)))
		{
			fs_finds++;
			found = fs_allowfileuri->handle->FindFile(fs_allowfileuri->handle, loc, cleanpath, NULL);
			if (found)
				loc->search = fs_allowfileuri;
		}
		pf = NULL;
		goto fail;
	}

	filename = FS_GetCleanPath(filename, (lflags&FSLF_QUIET), cleanpath, sizeof(cleanpath));
	if (!filename)
	{
		pf = NULL;
		goto fail;
	}

	if (com_fs_cache.ival && !com_fschanged && !(lflags & FSLF_IGNOREPURE))
	{
		bucket_t *b = Hash_GetInsensitiveBucket(&filesystemhash, filename);
		if (b)
		{
			pf = b->data;
			filename = b->key.string;	//update the filename to use the correct file case...
		}
		else
			goto fail;
	}
	else
		pf = NULL;

	if (com_purepaths && found == FF_NOTFOUND && !(lflags & FSLF_IGNOREPURE))
	{
		//check if its in one of the 'pure' packages. these override the default ones.
		for (search = com_purepaths ; search ; search = search->nextpure)
		{
			if ((lflags & FSLF_SECUREONLY) && !(search->flags & SPF_UNTRUSTED))
				continue;
			if (!((lflags & FSLF_IGNOREBASEDEPTH) && (search->flags & SPF_BASEPATH)))
				depth += ((search->flags & SPF_EXPLICIT) || (lflags & FSLF_DEPTH_INEXPLICIT));
			fs_finds++;
			found = search->handle->FindFile(search->handle, loc, filename, pf);
			if (found)
			{
				if (!(lflags & FSLF_DONTREFERENCE))
				{
					if ((search->flags & fs_referencetype) != fs_referencetype)
						Con_DPrintf("%s became referenced due to %s\n", search->purepath, filename);
					search->flags |= fs_referencetype;
				}
				loc->search = search;
				break;
			}
		}
	}

	if (((lflags & FSLF_IGNOREPURE) || fs_puremode < 2) && found == FF_NOTFOUND)
	{
		// optionally check the non-pure paths too.
		for (search = com_searchpaths ; search ; search = search->next)
		{
			if ((lflags & FSLF_SECUREONLY) && (search->flags & SPF_UNTRUSTED))
				continue;
			if (!((lflags & FSLF_IGNOREBASEDEPTH) && (search->flags & SPF_BASEPATH)))
				depth += ((search->flags & SPF_EXPLICIT) || (lflags & FSLF_DEPTH_INEXPLICIT));
			fs_finds++;
			found = search->handle->FindFile(search->handle, loc, filename, pf);
			if (found)
			{
				if (!(lflags & FSLF_DONTREFERENCE))
				{
					if ((search->flags & fs_referencetype) != fs_referencetype)
						Con_DPrintf("%s became referenced due to %s\n", search->purepath, filename);
					search->flags |= fs_referencetype;
				}
				loc->search = search;
				break;
			}
		}
	}
fail:
	if (found == FF_SYMLINK && !(lflags & FSLF_IGNORELINKS))
	{
		static int blocklink;
		if (blocklink < 4 && loc->len < MAX_QPATH)
		{
			//read the link target
			char *s, *b;
			char targname[MAX_QPATH];
			char mergedname[MAX_QPATH];
			targname[loc->len] = 0;
			loc->search->handle->ReadFile(loc->search->handle, loc, targname);

			//properlyish unixify
			while((s = strchr(targname, '\\')))
				*s = '/';
			if (*targname == '/')
				Q_strncpyz(mergedname, targname+1, sizeof(mergedname));
			else
			{
				Q_strncpyz(mergedname, filename, sizeof(mergedname));
				while((s = strchr(mergedname, '\\')))
					*s = '/';
				b = COM_SkipPath(mergedname);
				*b = 0;
				for (s = targname; !strncmp(s, "../", 3) && b > mergedname; )
				{
					s += 3;
					if (b[-1] == '/')
						*--b = 0;
					*b = 0;
					b = strrchr(mergedname, '/');
					if (b)
						*++b = 0;
					else
					{
						//no prefix left.
						*mergedname = 0;
						break;
					}
				}
				b = mergedname + strlen(mergedname);
				Q_strncpyz(b, s, sizeof(mergedname) - (b - mergedname));
			}

			//and locate that instead.
			blocklink++;
			depth = FS_FLocateFile(mergedname, lflags, loc);
			blocklink--;
			if (!loc->search)
				Con_Printf("Symlink %s -> %s (%s) is dead\n", filename, targname, mergedname);
			return depth;
		}
	}

/*	if (len>=0)
	{
		if (loc)
			Con_Printf("Found %s:%i\n", loc->rawname, loc->len);
		else
			Con_Printf("Found %s\n", filename);
	}
	else
		Con_Printf("Failed\n");
*/
	if (found == FF_NOTFOUND || found == FF_DIRECTORY || loc->len == -1)
	{
		if (lflags & FSLF_DEEPONFAILURE)
			return 0x7fffffff;	//if we're asking for depth, the file is reported to be so far into the filesystem as to be irrelevant.
		return 0;
	}
	return depth+1;
}

//returns the location's root package (or gamedir).
//(aka: loc->search->purepath, but stripping contained nested packs)
const char *FS_GetRootPackagePath(flocation_t *loc)
{
	searchpath_t *sp, *search;

	for (sp = loc->search; ;)
	{
		for (search = com_searchpaths; search; search = search->next)
		{
			if (search != sp)
				if (search->handle->GeneratePureCRC) //only consider files that have a pure hash. this excludes system paths
					if (!strncmp(search->purepath, sp->purepath, strlen(search->purepath)))
						if (sp->purepath[strlen(search->purepath)] == '/')	//also ensures that the path gets shorter, avoiding infinite loops as it fights between base+home dirs.
							break;
		}
		if (search)
			sp = search;
		else
			break;
	}

	//
	if (sp)
		return sp->purepath;
	return NULL;
}

//returns the package/'gamedir/foo.pk3' filename to tell the client to download
//unfortunately foo.pk3 may contain a 'bar.pk3' and downloading dir/foo.pk3/bar.pk3 won't work
//so if loc->search is dir/foo.pk3/bar.pk3 find dir/foo.pk3 instead
const char *FS_GetPackageDownloadFilename(flocation_t *loc)
{
	searchpath_t *sp, *search;

	for (sp = loc->search; ;)
	{
		for (search = com_searchpaths; search; search = search->next)
		{
			if (search != sp)
				if (search->handle->GeneratePureCRC) //only consider files that have a pure hash. this excludes system paths
					if (!strncmp(search->purepath, sp->purepath, strlen(search->purepath)))
						if (sp->purepath[strlen(search->purepath)] == '/')	//also ensures that the path gets shorter, avoiding infinite loops as it fights between base+home dirs.
							break;
		}
		if (search)
			sp = search;
		else
			break;
	}

	if (sp && strchr(sp->purepath, '/') && sp->handle->GeneratePureCRC)	//never allow any packages that are directly sitting in the basedir.
		return sp->purepath;
	return NULL;
}
qboolean FS_GetLocationForPackageHandle(flocation_t *loc, searchpathfuncs_t *spath, const char *fname)
{
	searchpath_t *search;
	for (search = com_searchpaths; search; search = search->next)
	{
		if (search->handle == spath)
		{
			loc->search = search;
			return spath->FindFile(spath, loc, fname, NULL) == FF_FOUND;
		}
	}
	return false;
}
const char *FS_WhichPackForLocation(flocation_t *loc, unsigned int flags)
{
	char *ret;
	if (!loc->search)
		return NULL;	//huh? not a valid location.

	if (flags & WP_FULLPATH)
	{
		if (flags & WP_REFERENCE)
			loc->search->flags |= SPF_REFERENCED;
		return loc->search->purepath;
	}
	else
	{
		ret = strchr(loc->search->purepath, '/');
		if (ret)
		{
			ret++;
			if (!strchr(ret, '/'))
			{
				if (flags & WP_REFERENCE)
					loc->search->flags |= SPF_REFERENCED;
				return ret;
			}
		}
	}
	return NULL;
}

searchpath_t *FS_GetPackage(const char *package)
{
	searchpath_t	*search;

	for (search = com_searchpaths ; search ; search = search->next)
	{
		if (!Q_strcasecmp(package, search->purepath))
			return search;
	}
	return NULL;
}
/*requires extension*/
qboolean FS_GetPackageDownloadable(const char *package)
{
	searchpath_t	*search = FS_GetPackage(package);

	if (search)
		return !(search->flags & SPF_COPYPROTECTED);
	return false;
}

char *FS_GetPackHashes(char *buffer, int buffersize, qboolean referencedonly)
{
	searchpath_t	*search;
	buffersize--;
	*buffer = 0;

	if (com_purepaths)
	{
		for (search = com_purepaths ; search ; search = search->nextpure)
		{
			Q_strncatz(buffer, va("%i ", search->crc_check), buffersize);
		}
		return buffer;
	}
	else
	{
		for (search = com_searchpaths ; search ; search = search->next)
		{
			if (search->crc_check)
			{
				Q_strncatz(buffer, va("%i ", search->crc_check), buffersize);
			}
		}
		return buffer;
	}
}
/*
referencedonly=0: show all paks
referencedonly=1: show only paks that are referenced (q3-compat)
referencedonly=2: show all paks, but paks that are referenced are prefixed with a star
ext=0: hide extensions (q3-compat)
ext=1: show extensions.
*/
char *FS_GetPackNames(char *buffer, int buffersize, int referencedonly, qboolean ext)
{
	char temp[MAX_OSPATH];
	searchpath_t	*search;
	buffersize--;
	*buffer = 0;

	if (com_purepaths)
	{
		for (search = com_purepaths ; search ; search = search->nextpure)
		{
			if (referencedonly == 0 && !(search->flags & SPF_REFERENCED))
				continue;
			if (referencedonly == 2 && (search->flags & SPF_REFERENCED))
				Q_strncatz(buffer, "*", buffersize);

			if (!ext)
			{
				COM_StripExtension(search->purepath, temp, sizeof(temp));
				Q_strncatz(buffer, va("%s ", temp), buffersize);
			}
			else
			{
				Q_strncatz(buffer, va("%s ", search->purepath), buffersize);
			}
		}
		return buffer;
	}
	else
	{
		for (search = com_searchpaths ; search ; search = search->next)
		{
			if (search->crc_check)
			{
				if (referencedonly == 0 && !(search->flags & SPF_REFERENCED))
					continue;
				if (referencedonly == 2 && (search->flags & SPF_REFERENCED))
				{
					// '*' prefix is meant to mean 'referenced'.
					//really all that means to the client is that it definitely wants to download it.
					//if its copyrighted, the client shouldn't try to do so, as it won't be allowed.
					if (!(search->flags & SPF_COPYPROTECTED))
						Q_strncatz(buffer, "*", buffersize);
				}

				if (!ext)
				{
					COM_StripExtension(search->purepath, temp, sizeof(temp));
					Q_strncatz(buffer, va("%s ", temp), buffersize);
				}
				else
				{
					Q_strncatz(buffer, va("%s ", search->purepath), buffersize);
				}
			}
		}
		return buffer;
	}
}

void FS_ReferenceControl(unsigned int refflag, unsigned int resetflags)
{
	searchpath_t	*s;

	refflag &= SPF_REFERENCED;
	resetflags &= SPF_REFERENCED;

	if (resetflags)
	{
		for (s=com_searchpaths ; s ; s=s->next)
		{
			s->flags &= ~resetflags;
		}
	}

	fs_referencetype = refflag;
}

//outbuf might not be written into
static const char *FS_GetCleanPath(const char *pattern, qboolean silent, char *outbuf, int outlen)
{
	const char *s;
	char *o;
	char *seg;
	char *end = outbuf + outlen;
	static float throttletimer;

	s = pattern;
	seg = o = outbuf;
	if (!pattern || !*pattern)
	{
		Con_ThrottlePrintf(&throttletimer, 0, "Error: Empty filename\n");
		return NULL;
	}
	for(;;)
	{
		if (o == end)
		{
			Con_ThrottlePrintf(&throttletimer, 0, "Error: filename too long\n");
			return NULL;
		}
		if (*s == ':')
		{
			if (s == pattern+1 && (s[1] == '/' || s[1] == '\\'))
				Con_ThrottlePrintf(&throttletimer, 0, "Error: absolute path in filename %s\n", pattern);
			else
				Con_ThrottlePrintf(&throttletimer, 0, "Error: alternative data stream in filename %s\n", pattern);
			return NULL;
		}
		else if (*s == '\\' || *s == '/' || !*s)
		{	//end of segment
			if (o == seg)
			{
				if (o == outbuf)
				{
					Con_ThrottlePrintf(&throttletimer, 0, "Error: absolute path in filename %s\n", pattern);
					return NULL;
				}
				if (!*s)
				{
					*o++ = '\0';
					break;
				}
				Con_ThrottlePrintf(&throttletimer, 0, "Error: empty directory name (%s)\n", pattern);
				s++;
				continue;
			}
			//ignore any leading spaces in the name segment
			//it should just make more stuff invalid
			while (*seg == ' ')
				seg++;
			if (!seg[0])
			{
				Con_ThrottlePrintf(&throttletimer, 0, "Error: No filename (%s)\n", pattern);
				return NULL;
			}
			if (seg[0] == '.')
			{
				if (o == seg+1)
					Con_ThrottlePrintf(&throttletimer, 0, "Error: source directory (%s)\n", pattern);
				else if (seg[1] == '.')
					Con_ThrottlePrintf(&throttletimer, 0, "Error: parent directory (%s)\n", pattern);
				else
					Con_ThrottlePrintf(&throttletimer, 0, "Error: hidden name (%s)\n", pattern);
				return NULL;
			}
#if defined(_WIN32) || defined(__CYGWIN__)
			//in win32, we use the //?/ trick to get around filename length restrictions.
			//4-letter reserved paths: comX, lptX
			//we'll allow this elsewhere to save cycles, just try to avoid running it on a fat32 or ntfs filesystem from linux
			if (((seg[0] == 'c' || seg[0] == 'C') &&
				 (seg[1] == 'o' || seg[1] == 'O') &&
				 (seg[2] == 'm' || seg[2] == 'M') &&
				 (seg[3] >= '0' && seg[3] <= '9')) ||
				((seg[0] == 'l' || seg[0] == 'L') &&
				 (seg[1] == 'p' || seg[1] == 'P') &&
				 (seg[2] == 't' || seg[2] == 'T') &&
				 (seg[3] >= '0' && seg[3] <= '9')))
			{
				if (o == seg+4 || seg[4] == ' '|| seg[4] == '\t' || seg[4] == '.')
				{
					Con_ThrottlePrintf(&throttletimer, 0, "Error: reserved name in path (%c%c%c%c in %s)\n", seg[0], seg[1], seg[2], seg[3], pattern);
					return NULL;
				}
			}
			//3 letter reserved paths: con, nul, prn
			if (((seg[0] == 'c' || seg[0] == 'C') &&
				 (seg[1] == 'o' || seg[1] == 'O') &&
				 (seg[2] == 'n' || seg[2] == 'N')) ||
				((seg[0] == 'p' || seg[0] == 'P') &&
				 (seg[1] == 'r' || seg[1] == 'R') &&
				 (seg[2] == 'n' || seg[2] == 'N')) ||
				((seg[0] == 'n' || seg[0] == 'N') &&
				 (seg[1] == 'u' || seg[1] == 'U') &&
				 (seg[2] == 'l' || seg[2] == 'L')))
			{
				if (o == seg+3 || seg[3] == ' '|| seg[3] == '\t' || seg[3] == '.')
				{
					Con_ThrottlePrintf(&throttletimer, 0, "Error: reserved name in path (%c%c%c in %s)\n", seg[0], seg[1], seg[2], pattern);
					return NULL;
				}
			}
#endif

			if (*s++)
				*o++ = '/';
			else
			{
				*o++ = '\0';
				break;
			}
			seg = o;
		}
		else
			*o++ = *s++;
	}

//	Sys_Printf("%s changed to %s\n", pattern, outbuf);
	return outbuf;
}

static vfsfile_t *VFS_Filter(const char *filename, vfsfile_t *handle)
{
//	char *ext;
	if (!filename)
		return handle;	//block any filtering (so we don't do stupid stuff like having servers pre-decompressing when downloading)
	if (!handle || !handle->ReadBytes || handle->seekstyle == SS_UNSEEKABLE)	//only on readonly files for which we can undo any header read damage
		return handle;
//	if (handle->seekstyle == SS_SLOW)
//		return handle;	//we only peek at the header, so rewinding shouldn't be too expensive at least...
//	const char *ext = COM_GetFileExtension(filename, NULL);
#ifdef AVAIL_XZDEC
//	if (!Q_strcasecmp(ext, ".xz"))
	{
		vfsfile_t *nh;
		nh = FS_XZ_DecompressReadFilter(handle);
		if (nh!=handle)
			return nh;
	}
#endif
#ifdef AVAIL_GZDEC
//	if (!Q_strcasecmp(ext, ".gz"))
	{
		return FS_DecompressGZip(handle, NULL);
	}
#endif
	return handle;
}

static qboolean FS_NativePath(const char *fname, enum fs_relative relativeto, char *out, int outlen, qboolean fordisplay)
{
	flocation_t loc;
	int i;
	char cleanname[MAX_QPATH];
	char *last;
	qboolean wasbase;	//to handle out-of-order base/game dirs.
	int nlen;

	if (!fs_hidesyspaths.ival)
		fordisplay = false;	//just show system paths

	if (relativeto == FS_SYSTEM)
	{
		//system is already the native path. we can just pass it through. perhaps we should clean it up first however, although that's just making sure all \ are /	
		if (fordisplay)
		{
			if (com_homepathenabled && !strncmp(fname, com_homepath, strlen(com_homepath)))	//'FS_HOME'
				Q_snprintfz(out, outlen, "$homedir/%s", fname+strlen(com_homepath));
			else if (!strncmp(fname, com_gamepath, strlen(com_gamepath)))					//FS_ROOT-THAT-ISNT-HOME
				Q_snprintfz(out, outlen, "$basedir/%s", fname+strlen(com_gamepath));
			else if (host_parms.binarydir && !strncmp(fname, host_parms.binarydir, strlen(host_parms.binarydir)))	//FS_BINARYDIR
				Q_snprintfz(out, outlen, "$bindir/%s", fname+strlen(host_parms.binarydir));
#ifdef FTE_LIBRARY_PATH
			else if (!strncmp(fname, STRINGIFY(FTE_LIBRARY_PATH)"/", strlen(STRINGIFY(FTE_LIBRARY_PATH)"/")))	//FS_LIBRARYDIR
				Q_snprintfz(out, outlen, "$libdir/%s", fname+strlen(STRINGIFY(FTE_LIBRARY_PATH)"/"));
#endif
			else	//should try bindir
				Q_snprintfz(out, outlen, "$system/%s", COM_SkipPath(fname));			//FS_SYSTEM :(
		}
		else
			Q_snprintfz(out, outlen, "%s", fname);

#ifdef _WIN32
		for (; *out; out++)
		{
			if (*out == '\\')
				*out = '/';
		}
#endif
		return true;
	}

	if (*fname == 0)
	{
		//this is sometimes used to query the actual path.
		//don't alow it for other stuff though.
		if (relativeto != FS_ROOT && relativeto != FS_BINARYPATH && relativeto != FS_LIBRARYPATH && relativeto != FS_GAMEONLY)
			return false;
	}
	else
	{
		fname = FS_GetCleanPath(fname, false, cleanname, sizeof(cleanname));
		if (!fname)
			return false;
	}

	switch (relativeto)
	{
	case FS_GAME: //this is really for diagnostic type stuff...
		if (FS_FLocateFile(fname, FSLF_IFFOUND, &loc))
		{
			if (fordisplay)
			{
				if (!strncmp(com_homepath, loc.search->logicalpath, strlen(com_homepath)))
					nlen = Q_snprintfz(out, outlen, "$homedir/%s/%s", loc.search->purepath, fname);
				else
					nlen = Q_snprintfz(out, outlen, "$basedir/%s/%s", loc.search->purepath, fname);
			}
			else
				nlen = Q_snprintfz(out, outlen, "%s/%s", loc.search->logicalpath, fname);
			break;
		}
		//fallthrough
	case FS_GAMEONLY:
		if (com_homepathenabled)
			nlen = Q_snprintfz(out, outlen, "%s%s/%s", fordisplay?"$homedir/":com_homepath, gamedirfile, fname);
		else
			nlen = Q_snprintfz(out, outlen, "%s%s/%s", fordisplay?"$basedir/":com_gamepath, gamedirfile, fname);
		break;
	case FS_LIBRARYPATH:
#ifdef FTE_LIBRARY_PATH
		if (fordisplay)
			nlen = Q_snprintfz(out, outlen, "$libdir/%s", fname);
		else
			nlen = Q_snprintfz(out, outlen, STRINGIFY(FTE_LIBRARY_PATH)"/%s", fname);
		break;
#else
		return false;
#endif
	case FS_BINARYPATH:
		if (host_parms.binarydir && *host_parms.binarydir)
			nlen = Q_snprintfz(out, outlen, "%s%s", fordisplay?"$bindir/":host_parms.binarydir, fname);
		else
			nlen = Q_snprintfz(out, outlen, "%s%s", fordisplay?"$basedir/":host_parms.basedir, fname);
		break;
	case FS_ROOT:
		if (com_installer)
			return false;
		if (com_homepathenabled)
			nlen = Q_snprintfz(out, outlen, "%s%s", fordisplay?"$homedir/":com_homepath, fname);
		else
			nlen = Q_snprintfz(out, outlen, "%s%s", fordisplay?"$basedir/":com_gamepath, fname);
		break;

	case FS_BASEGAMEONLY:	// fte/
		last = NULL;
		for (i = 0; i < countof(fs_manifest->gamepath); i++)
		{
			if (fs_manifest && (fs_manifest->gamepath[i].flags&GAMEDIR_BASEGAME) && fs_manifest->gamepath[i].path)
			{
				if (fs_manifest->gamepath[i].flags & GAMEDIR_SPECIAL)
					continue;
				last = fs_manifest->gamepath[i].path;
			}
		}
		if (!last)
			return false;	//eep?
		if (com_homepathenabled)
			nlen = Q_snprintfz(out, outlen, "%s%s/%s", fordisplay?"$homedir/":com_homepath, last, fname);
		else
			nlen = Q_snprintfz(out, outlen, "%s%s/%s", fordisplay?"$basedir/":com_gamepath, last, fname);
		break;
	case FS_PUBGAMEONLY:	// $gamedir/ or qw/ but not fte/
		last = NULL;
		wasbase = true;
		for (i = 0; i < countof(fs_manifest->gamepath); i++)
		{
			if (fs_manifest && fs_manifest->gamepath[i].path)
			{
				qboolean isbase = fs_manifest->gamepath[i].flags&GAMEDIR_BASEGAME;
				if (fs_manifest->gamepath[i].flags&(GAMEDIR_PRIVATE|GAMEDIR_SPECIAL))
					continue;
				if (isbase && !wasbase)
					continue;
				wasbase = isbase;
				last = fs_manifest->gamepath[i].path;
			}
		}
		if (!last)
			return false;	//eep?
		if (com_homepathenabled)
			nlen = Q_snprintfz(out, outlen, "%s%s/%s", fordisplay?"$homedir/":com_homepath, last, fname);
		else
			nlen = Q_snprintfz(out, outlen, "%s%s/%s", fordisplay?"$basedir/":com_gamepath, last, fname);
		break;
	case FS_PUBBASEGAMEONLY:	// qw/ (fixme: should be the last non-private basedir)
		last = NULL;
		for (i = 0; i < countof(fs_manifest->gamepath); i++)
		{
			if (fs_manifest && (fs_manifest->gamepath[i].flags&GAMEDIR_BASEGAME) && fs_manifest->gamepath[i].path)
			{
				if (fs_manifest->gamepath[i].flags&(GAMEDIR_PRIVATE|GAMEDIR_SPECIAL))
					continue;
				last = fs_manifest->gamepath[i].path;
			}
		}
		if (!last)
			return false;	//eep?
		if (com_homepathenabled)
			nlen = Q_snprintfz(out, outlen, "%s%s/%s", fordisplay?"$homedir/":com_homepath, last, fname);
		else
			nlen = Q_snprintfz(out, outlen, "%s%s/%s", fordisplay?"$basedir/":com_gamepath, last, fname);
		break;
	default:
		Sys_Error("FS_NativePath case not handled\n");
	}
	return nlen < outlen;
}

qboolean FS_SystemPath(const char *fname, enum fs_relative relativeto, char *out, int outlen)
{
	return FS_NativePath(fname, relativeto, out, outlen, false);
}
qboolean FS_DisplayPath(const char *fname, enum fs_relative relativeto, char *out, int outlen)
{
	if (outlen)
		*out = 0;
	return FS_NativePath(fname, relativeto, out, outlen, true);
}

//returns false to stop the enumeration. check the return value of the fs enumerator to see if it was canceled by this return value.
static int QDECL FS_NullFSEnumerator(const char *fname, qofs_t fsize, time_t mtime, void *parm, searchpathfuncs_t *spath)
{
	return false;
}

//opens a file in the same (writable) path that contains an existing version of the file or one of the other patterns listed
vfsfile_t *FS_OpenWithFriends(const char *fname, char *displayname, size_t displaynamesize, int numfriends, ...)
{
	searchpath_t *search;
	searchpath_t *lastwritable = NULL;
	flocation_t loc;
	va_list ap;
	int i;
	char cleanname[MAX_QPATH];

	fname = FS_GetCleanPath(fname, false, cleanname, sizeof(cleanname));
	if (!fname)
		return NULL;

	for (search = com_searchpaths; search ; search = search->next)
	{
		if ((search->flags & SPF_EXPLICIT) && (search->flags & SPF_WRITABLE))
			lastwritable = search;
		if (search->handle->FindFile(search->handle, &loc, fname, NULL))
			break;
		
		va_start(ap, numfriends);
		for (i = 0; i < numfriends; i++)
		{
			char *path = va_arg(ap, char*);
			if (!search->handle->EnumerateFiles(search->handle, path, FS_NullFSEnumerator, NULL))
				break;
		}
		va_end(ap);
		if (i < numfriends)
			break;
	}
	if (lastwritable)
	{
		char sysname[MAX_OSPATH];
		vfsfile_t *f;
		//figure out the system path
		Q_strncpyz(sysname, lastwritable->logicalpath, sizeof(sysname));
		FS_CleanDir(sysname, sizeof(sysname));
		Q_strncatz(sysname, fname, sizeof(sysname));

		//create the dir if needed and open the file.
		COM_CreatePath(sysname);
		f = VFSOS_Open(sysname, "wbp");

		if (fs_hidesyspaths.ival)
		{
			if (*com_homepath && !strncmp(com_homepath, lastwritable->logicalpath, strlen(com_homepath)))
				Q_snprintfz(displayname, displaynamesize, "$homedir/%s/%s", lastwritable->purepath, fname);
			else
				Q_snprintfz(displayname, displaynamesize, "$basedir/%s/%s", lastwritable->purepath, fname);
		}
		else
			Q_strncpyz(displayname, sysname, displaynamesize);
		return f;
	}
	FS_DisplayPath(fname, FS_GAMEONLY, displayname, displaynamesize);
	return NULL;
}

//returns false if the string didn't fit. we're not trying to be clever and reallocate the buffer
static qboolean try_snprintf(char *buffer, size_t size, const char *format, ...)
{
	size_t ret;
	va_list		argptr;

	va_start (argptr, format);
#ifdef _WIN32
#undef _vsnprintf
	ret = _vsnprintf(buffer, size, format, argptr);
#define _vsnprintf unsafe_vsnprintf
#else
	ret = vsnprintf (buffer, size, format,argptr);
#endif
	va_end (argptr);
	if (ret > size-1)	//should cope with microsoft's -1s and linuxes total-length return values.
		return false;
	return true;
}

/*locates and opens a file
modes:
r = read
w = write
a = append
t = text mode (because windows sucks). binary is otherwise assumed.
p = persist (ie: saved games and configs, but not downloads or large content)
*/
vfsfile_t *QDECL FS_OpenVFS(const char *filename, const char *mode, enum fs_relative relativeto)
{
	char cleanname[MAX_QPATH];
	char fullname[MAX_OSPATH];
	flocation_t loc;
	vfsfile_t *vfs;

	//eventually, this function will be the *ONLY* way to get at files

	fs_accessed_time = realtime;

	if (fs_readonly && *mode == 'w')
		return NULL;

	if (!strncmp(filename, "file:", 5))
	{
		if (fs_allowfileuri || relativeto == FS_SYSTEM)
		{
			if (Sys_ResolveFileURL(filename, strlen(filename), fullname, sizeof(fullname)))
				return VFSOS_Open(fullname, mode);
		}
		return NULL;
	}

	if (relativeto == FS_SYSTEM)
		return VFSOS_Open(filename, mode);

	//blanket-bans

	filename = FS_GetCleanPath(filename, false, cleanname, sizeof(cleanname));
	if (!filename)
		return NULL;

#ifdef _DEBUG
	if (strcmp(mode, "rb"))
		if (strcmp(mode, "r+b"))
			if (strcmp(mode, "wb"))
				if (strcmp(mode, "w+b"))
					if (strcmp(mode, "ab"))
						if (strcmp(mode, "wbp"))
							return NULL; //urm, unable to write/append
#endif

	//if there can only be one file (eg: write access) find out where it is.
	switch (relativeto)
	{
	case FS_GAMEONLY:	//OS access only, no paks. Used for (re)writing files.
		vfs = NULL;
		//FIXME: go via a searchpath, because then the fscache can be selectively updated
		if (com_homepathenabled)
		{
			if (gameonly_homedir)
			{
				if ((*mode == 'w' && gameonly_gamedir->handle->CreateFile)
						? gameonly_homedir->handle->CreateFile(gameonly_homedir->handle, &loc, filename)
						: gameonly_homedir->handle->FindFile  (gameonly_homedir->handle, &loc, filename, NULL))
					vfs = gameonly_homedir->handle->OpenVFS   (gameonly_homedir->handle, &loc, mode);
				else
					vfs = NULL;
			}
			else
			{
				if (!try_snprintf(fullname, sizeof(fullname), "%s%s/%s", com_homepath, gamedirfile, filename))
					return NULL;
				if (*mode == 'w')
					COM_CreatePath(fullname);
				vfs = VFSOS_Open(fullname, mode);
			}
		}
		if (!vfs && *gamedirfile)
		{
			if (gameonly_gamedir)
			{
				if ((*mode == 'w' && gameonly_gamedir->handle->CreateFile)
						? gameonly_gamedir->handle->CreateFile(gameonly_gamedir->handle, &loc, filename)
						: gameonly_gamedir->handle->FindFile  (gameonly_gamedir->handle, &loc, filename, NULL))
					vfs = gameonly_gamedir->handle->OpenVFS   (gameonly_gamedir->handle, &loc, mode);
				else
					vfs = NULL;
			}
			else
			{
				if (!try_snprintf(fullname, sizeof(fullname), "%s%s/%s", com_gamepath, gamedirfile, filename))
					return NULL;
				if (*mode == 'w')
					COM_CreatePath(fullname);
				vfs =  VFSOS_Open(fullname, mode);
			}
		}
		if (vfs || !(*mode == 'w' || *mode == 'a'))
			return vfs;
		//fall through
	case FS_PUBGAMEONLY:		//used for $gamedir/downloads
	case FS_BASEGAMEONLY:		//used for fte/configs/*
	case FS_PUBBASEGAMEONLY:	//used for qw/skins/*
		if (!FS_SystemPath(filename, relativeto, fullname, sizeof(fullname)))
			return NULL;
		if (*mode == 'w')
			COM_CreatePath(fullname);
		return VFSOS_Open(fullname, mode);
	case FS_GAME:	//load from paks in preference to system paths. overwriting be damned.
		if (!FS_SystemPath(filename, relativeto, fullname, sizeof(fullname)))
			return NULL;
		break;
	case FS_LIBRARYPATH:
	case FS_BINARYPATH:
		if (!FS_SystemPath(filename, relativeto, fullname, sizeof(fullname)))
			return NULL;
		if (*mode == 'w')
			COM_CreatePath(fullname);
		return VFSOS_Open(fullname, mode);
	case FS_ROOT:	//always bypass packs and gamedirs
		if (com_installer)
			return NULL;
		if (com_homepathenabled)
		{
			if (!try_snprintf(fullname, sizeof(fullname), "%s%s", com_homepath, filename))
				return NULL;
			if (*mode == 'w')
				COM_CreatePath(fullname);
			vfs = VFSOS_Open(fullname, mode);
			if (vfs)
				return vfs;
		}
		if (!try_snprintf(fullname, sizeof(fullname), "%s%s", com_gamepath, filename))
			return NULL;
		if (*mode == 'w')
			COM_CreatePath(fullname);
		return VFSOS_Open(fullname, mode);
	default:
		Sys_Error("FS_OpenVFS: Bad relative path (%i)", relativeto);
		break;
	}

	FS_FLocateFile(filename, FSLF_IFFOUND, &loc);

	if (loc.search)
	{
		return loc.search->handle->OpenVFS(loc.search->handle, &loc, mode);
	}

	//if we're meant to be writing, best write to it.
	if (strchr(mode , 'w') || strchr(mode , 'a'))
	{
		COM_CreatePath(fullname);
		return VFSOS_Open(fullname, mode);
	}
	return NULL;
}

qboolean FS_GetLocMTime(flocation_t *location, time_t *modtime)
{
	*modtime = 0;
	if (!location->search->handle->FileStat || !location->search->handle->FileStat(location->search->handle, location, modtime))
		return false;
	return true;
}

/*opens a vfsfile from an already discovered location*/
vfsfile_t *FS_OpenReadLocation(const char *fname, flocation_t *location)
{
	if (location->search)
	{
		return VFS_Filter(fname, location->search->handle->OpenVFS(location->search->handle, location, "rb"));
	}
	return NULL;
}

qboolean FS_Rename2(const char *oldf, const char *newf, enum fs_relative oldrelativeto, enum fs_relative newrelativeto)
{
	char oldfullname[MAX_OSPATH];
	char newfullname[MAX_OSPATH];

	if (!FS_SystemPath(oldf, oldrelativeto, oldfullname, sizeof(oldfullname)))
		return false;
	if (!FS_SystemPath(newf, newrelativeto, newfullname, sizeof(newfullname)))
		return false;

	FS_CreatePath(newf, newrelativeto);
	if (Sys_Rename(oldfullname, newfullname))
	{
		if (oldrelativeto >= FS_GAME)
			FS_FlushFSHashRemoved(oldf);
		if (newrelativeto >= FS_GAME)
			FS_FlushFSHashWritten(newf);
		return true;
	}
	return false;
}
qboolean FS_Rename(const char *oldf, const char *newf, enum fs_relative relativeto)
{
	char cleanold[MAX_QPATH];
	char cleannew[MAX_QPATH];
	if (relativeto != FS_SYSTEM)
	{
		oldf = FS_GetCleanPath(oldf, false, cleanold, sizeof(cleanold));
		newf = FS_GetCleanPath(newf, false, cleannew, sizeof(cleannew));
	}
	return FS_Rename2(oldf, newf, relativeto, relativeto);
}
qboolean FS_Remove(const char *fname, enum fs_relative relativeto)
{
	char fullname[MAX_OSPATH];

	if (!FS_SystemPath(fname, relativeto, fullname, sizeof(fullname)))
		return false;

	if (Sys_remove (fullname))
	{
		if (relativeto >= FS_GAME)
			FS_FlushFSHashRemoved(fname);
		return true;
	}
	return false;
}
static int QDECL FS_RemoveTreeCallback(const char *fname, qofs_t fsize, time_t mtime, void *parm, searchpathfuncs_t *spath)
{
	char fullname[MAX_OSPATH];
	if (*fname && fname[strlen(fname)-1] == '/')
	{
		Q_snprintfz(fullname, sizeof(fullname), "%s*", fname);
		if (!spath->EnumerateFiles(spath, fullname, FS_RemoveTreeCallback, NULL))
			return false;
	}
	if (!spath->RemoveFile)
		return false;	//can't remove...

	if (!spath->RemoveFile(spath, fname))
	{
		Con_Printf("Unable to delete %s\n", fname);
		return false;	//remove failed
	}
	FS_RebuildFSHash_Update(fname);
	return true;
}
qboolean FS_RemoveTree(searchpathfuncs_t *pathhandle, const char *fname)
{	//this requires that the searchpath a) supports remove. b) supports listing directories...
	//path is expected to have a trailing /

	/*char cleaned[MAX_QPATH];
	fname = FS_GetCleanPath(fname, false, cleaned, sizeof(cleaned));
	if (!fname)
		return false;*/

	if (fs_readonly)
		return false;

	//FIXME: don't cross filesystems.
	//FIXME: remove dir symlinks instead of the target's contents.
	if (FS_RemoveTreeCallback(fname, 0, 0, NULL, pathhandle))
		return true;
	return false;
}

//create a path for the given filename (dir-only must have trailing slash)
void FS_CreatePath(const char *pname, enum fs_relative relativeto)
{
	char fullname[MAX_OSPATH];
	if (!FS_SystemPath(pname, relativeto, fullname, sizeof(fullname)))
		return;

	COM_CreatePath(fullname);
}

//FIXME: why is this qofs_t and not size_t?!?
void *FS_MallocFile(const char *filename, enum fs_relative relativeto, qofs_t *filesize)
{
	vfsfile_t *f;
	qbyte *buf;
	qofs_t len;

	f = FS_OpenVFS(filename, "rb", relativeto);
	if (!f)
		return NULL;
	len = VFS_GETLEN(f);
	if (filesize)
		*filesize = len;
	if (len >= ~(size_t)0)
	{
		VFS_CLOSE(f);
		Con_Printf(CON_ERROR"File %s: too large\n", filename);
		return NULL;
	}

	buf = (qbyte*)BZ_Malloc(len+1);
	if (!buf)	//this could be a soft error, but I don't want to have to deal with users reporting misc unrelated issues (and frankly most malloc failures are due to OOB writes)
		Sys_Error ("FS_MallocFile: out of memory loading %s", filename);

	((qbyte *)buf)[len] = 0;

	VFS_READ(f, buf, len);
	VFS_CLOSE(f);
	return buf;
}
qboolean FS_WriteFile (const char *filename, const void *data, int len, enum fs_relative relativeto)
{
	vfsfile_t *f;
	FS_CreatePath(filename, relativeto);
	f = FS_OpenVFS(filename, "wbp", relativeto);
	if (!f)
		return false;
	VFS_WRITE(f, data, len);
	VFS_CLOSE(f);

	return true;
}

qboolean FS_Copy(const char *source, const char *dest, enum fs_relative relativesource, enum fs_relative relativedest)
{
	vfsfile_t *d, *s;
	char buffer[8192*8];
	int read;
	qboolean result = false;
	FS_CreatePath(dest, relativedest);
	s = FS_OpenVFS(source, "rb", relativesource);
	if (s)
	{
		d = FS_OpenVFS(dest, "wbp", relativedest);
		if (d)
		{
			result = true;

			for (;;)
			{
				read = VFS_READ(s, buffer, sizeof(buffer));
				if (read <= 0)
					break;
				if (VFS_WRITE(d, buffer, read) != read)
				{
					result = false;
					break;
				}
			}

			VFS_CLOSE(d);

			if (!result)
				FS_Remove(dest, relativedest);
		}
		VFS_CLOSE(s);
	}
	return result;
}

static qbyte	*loadbuf;
static int		loadsize;

/*
============
COM_LoadFile

Filename are reletive to the quake directory.
Always appends a 0 qbyte to the loaded data.
============
*/
qbyte *COM_LoadFile (const char *path, unsigned int locateflags, int usehunk, size_t *filesize)
{
	vfsfile_t *f;
	qbyte *buf;
	qofs_t len;
	flocation_t loc;
	
	locateflags &= ~FSLF_DEEPONFAILURE;	//disable any flags that can't be supported here

	if (!FS_FLocateFile(path, locateflags, &loc) || !loc.search)
		return NULL;	//wasn't found

	if (loc.len > 0x7fffffff)	//don't malloc 5000gb sparse files or anything crazy on a 32bit system...
		return NULL;

	fs_accessed_time = realtime;

	f = loc.search->handle->OpenVFS(loc.search->handle, &loc, "rb");
	if (!f)
		return NULL;

	len = VFS_GETLEN(f);
	if (filesize)
		*filesize = len;

	if (usehunk == 2 || usehunk == 4 || usehunk == 6)
		COM_AssertMainThread("COM_LoadFile+hunk");

	if (usehunk == 0)
		buf = (qbyte*)Z_Malloc (len+1);
	else if (usehunk == 2)
		buf = (qbyte*)Hunk_TempAlloc (len+1);
	else if (usehunk == 4)
	{
		if (len+1 > loadsize)
			buf = (qbyte*)Hunk_TempAlloc (len+1);
		else
			buf = loadbuf;
	}
	else if (usehunk == 5)
		buf = (qbyte*)BZ_Malloc(len+1);
	else if (usehunk == 6)
		buf = (qbyte*)Hunk_TempAllocMore (len+1);
	else
	{
		Sys_Error ("COM_LoadFile: bad usehunk");
		buf = NULL;
	}

	if (!buf)
		Sys_Error ("COM_LoadFile: not enough space for %s", path);

	((qbyte *)buf)[len] = 0;

	VFS_READ(f, buf, len);
	VFS_CLOSE(f);

	return buf;
}

void *FS_LoadMallocFile (const char *path, size_t *fsize)
{
	return COM_LoadFile (path, 0, 5, fsize);
}
qbyte *FS_LoadMallocFileFlags (const char *path, unsigned int locateflags, size_t *fsize)
{
	return COM_LoadFile (path, locateflags, 5, fsize);
}

void *FS_LoadMallocGroupFile(zonegroup_t *ctx, char *path, size_t *fsize, qboolean filters)
{
	char *mem = NULL;
	vfsfile_t *f = FS_OpenVFS(path, "rb", FS_GAME);
	if (f && filters)
		f = VFS_Filter(path, f);
	if (f)
	{
		int len = VFS_GETLEN(f);
		if (ctx)
			mem = ZG_Malloc(ctx, len+1);
		else
			mem = BZ_Malloc(len+1);
		if (mem)
		{
			mem[len] = 0;
			if (VFS_READ(f, mem, len) == len)
				*fsize = len;
			else
				mem = NULL;
		}

		VFS_CLOSE(f);
	}
	return mem;
}

qbyte *COM_LoadTempFile (const char *path, unsigned int locateflags, size_t *fsize)
{
	return COM_LoadFile (path, locateflags, 2, fsize);
}
qbyte *COM_LoadTempMoreFile (const char *path, size_t *fsize)
{
	return COM_LoadFile (path, 0, 6, fsize);
}

// uses temp hunk if larger than bufsize
qbyte *QDECL COM_LoadStackFile (const char *path, void *buffer, int bufsize, size_t *fsize)
{
	qbyte	*buf;

	COM_AssertMainThread("COM_LoadStackFile");

	loadbuf = (qbyte *)buffer;
	loadsize = bufsize;
	buf = COM_LoadFile (path, 0, 4, fsize);

	return buf;
}


/*warning: at some point I'll change this function to return only read-only buffers*/
qofs_t FS_LoadFile(const char *name, void **file)
{
	size_t fsz;
	*file = FS_LoadMallocFile (name, &fsz);
	if (!*file)
		return (qofs_t)-1;
	return fsz;
}
void FS_FreeFile(void *file)
{
	BZ_Free(file);
}

//handle->EnumerateFiles on each a:b:c part of the given matches string.
static qboolean FS_EnumerateFilesEach(searchpathfuncs_t *handle, char *matches, int (QDECL *func)(const char *fname, qofs_t fsize, time_t mtime, void *parm, searchpathfuncs_t *spath), void *parm)
{
	char cleanpath[MAX_QPATH];
	const char *match;
	char *sep;
	for (; matches; matches = sep)
	{
		if (!strncmp(matches, "file:", 5))
		{
			sep = strchr(matches+5, ':');
			continue;
		}
		sep = strchr(matches, ':');
		if (sep)
		{
			*sep = 0;
			match = FS_GetCleanPath(matches, true, cleanpath, sizeof(cleanpath));
			*sep++ = ':';
		}
		else
			match = FS_GetCleanPath(matches, true, cleanpath, sizeof(cleanpath));

		if (match && *match)
			if (!handle->EnumerateFiles(handle, match, func, parm))
				return false;
	}
	return true;
}
static int FS_EnumerateFilesEachSys (const char *syspath, char *matches, int (*func)(const char *, qofs_t, time_t modtime, void *, searchpathfuncs_t *), void *parm, searchpathfuncs_t *spath)
{
	char cleanpath[MAX_QPATH];
	const char *match;
	char *sep;
	for (; matches; matches = sep)
	{
		if (!strncmp(matches, "file:", 5))
		{
			sep = strchr(matches+5, ':');
			continue;
		}
		sep = strchr(matches, ':');
		if (sep)
		{
			*sep = 0;
			match = FS_GetCleanPath(matches, true, cleanpath, sizeof(cleanpath));
			*sep++ = ':';
		}
		else
			match = FS_GetCleanPath(matches, true, cleanpath, sizeof(cleanpath));

		if (!Sys_EnumerateFiles(syspath, match, func, parm, spath))
			return false;
	}
	return true;
}
searchpathfuncs_t *COM_EnumerateFilesPackage (char *matches, const char *package, unsigned int flags, int (QDECL *func)(const char *, qofs_t, time_t mtime, void *, searchpathfuncs_t*), void *parm)
{	//special version of COM_EnumerateFiles that takes an explicit package name to search inside.
	//additionally accepts multiple patterns (separated by : chsrs)
	searchpathfuncs_t *handle;
	searchpath_t    *search;
	const char *sp;
	qboolean foundpackage = false;
	for (search = com_searchpaths; search ; search = search->next)
	{
		if (package)
		{
			if (flags & WP_FULLPATH)
				sp = search->purepath;
			else
			{
				sp = strchr(search->purepath, '/');
				if (sp && !strchr(++sp, '/'))
					;
				else
					continue;	//ignore packages inside other packages. they're just too weird.
			}
			if (strcmp(package, sp))
				continue;	//ignore this package
		}
		foundpackage = true;

		if (!FS_EnumerateFilesEach(search->handle, matches, func, parm))
			break;
	}

	if (!foundpackage && package && (flags&WP_FORCE) && (flags & WP_FULLPATH))
	{	//if we're forcing the package search then be prepared to open the gamedir or gamedir/package that was specified.
		char cleanname[MAX_OSPATH];
		char syspath[MAX_OSPATH];
		char *sl;

		package = FS_GetCleanPath(package, false, cleanname, sizeof(cleanname));
		if (!package)
			return NULL;

		sl = strchr(package, '/');
		if (sl)
		{	//try to open the named package.
			*sl = 0;
			if (strchr(sl+1, '/') || !FS_GamedirIsOkay(package))
				return NULL;
			*sl = '/';

			if (com_homepathenabled)
			{	//try the homedir
				Q_snprintfz(syspath, sizeof(syspath), "%s%s", com_homepath, package);
				handle = FS_OpenPackByExtension(VFSOS_Open(syspath, "rb"), NULL, package, package, "");
			}
			else
				handle = NULL;
			if (!handle)
			{	//now go for the basedir to see if ther.
				Q_snprintfz(syspath, sizeof(syspath), "%s%s", com_gamepath, package);
				handle = FS_OpenPackByExtension(VFSOS_Open(syspath, "rb"), NULL, package, package, "");
			}

			if (handle)
				FS_EnumerateFilesEach(handle, matches, func, parm);
			return handle;	//caller can use this for context, but is expected to tidy it up too.
		}
		else
		{	//we use NULLs for spath context here. caller will need to figure out which basedir to read it from.
			if (!FS_GamedirIsOkay(package))
				return NULL;

			if (com_homepathenabled)
			{
				Q_snprintfz(syspath, sizeof(syspath), "%s%s", com_homepath, package);
				FS_EnumerateFilesEachSys(syspath, matches, func, parm, NULL);
			}

			Q_snprintfz(syspath, sizeof(syspath), "%s%s", com_gamepath, package);
			FS_EnumerateFilesEachSys(syspath, matches, func, parm, NULL);
		}
	}
	return NULL;
}

struct fs_enumerate_fileuri_s
{
	int (QDECL *func)(const char *, qofs_t, time_t mtime, void *, searchpathfuncs_t*);
	void *parm;
};
static int QDECL COM_EnumerateFiles_FileURI (const char *name, qofs_t flags, time_t mtime, void *parm, searchpathfuncs_t *spath)
{
	char syspath[MAX_OSPATH];
	struct fs_enumerate_fileuri_s *e = parm;
	size_t nlen = strlen(name)+1;
	if (7+nlen > sizeof(syspath))
		return true;
	memcpy(syspath, "file://", 7);
	memcpy(syspath+7, name, nlen);
	return e->func(syspath, flags, mtime, e->parm, spath);
}

void COM_EnumerateFiles (const char *match, int (QDECL *func)(const char *, qofs_t, time_t mtime, void *, searchpathfuncs_t*), void *parm)
{
	searchpath_t    *search;

	if (!strncmp(match, "file:", 5))
	{
		if (fs_allowfileuri)
		{
			char syspath[MAX_OSPATH];
			struct fs_enumerate_fileuri_s e;
			e.func = func;
			e.parm = parm;
			if (Sys_ResolveFileURL(match, strlen(match), syspath, sizeof(syspath)))
				Sys_EnumerateFiles(NULL, syspath, COM_EnumerateFiles_FileURI, &e, NULL);
		}
		return;
	}

	for (search = com_searchpaths; search ; search = search->next)
	{
		if (!search->handle->EnumerateFiles(search->handle, match, func, parm))
			break;
	}
}

//scan packages in a reverse order, ie lowest priority first (for less scrolling upwards)
void COM_EnumerateFilesReverse (const char *match, int (QDECL *func)(const char *, qofs_t, time_t mtime, void *, searchpathfuncs_t*), void *parm)
{
	searchpath_t    *search;
	searchpath_t    **rev;
	size_t count;

	if (!strncmp(match, "file:", 5))
	{
		if (fs_allowfileuri)
		{
			char syspath[MAX_OSPATH];
			struct fs_enumerate_fileuri_s e;
			e.func = func;
			e.parm = parm;
			if (Sys_ResolveFileURL(match, strlen(match), syspath, sizeof(syspath)))
				Sys_EnumerateFiles(NULL, syspath, COM_EnumerateFiles_FileURI, &e, NULL);
		}
		return;
	}

	for (search = com_searchpaths, count=0; search ; search = search->next)
		count++;
	rev = BZ_Malloc(sizeof(*rev)*count);
	for (search = com_searchpaths, count=0; search ; search = search->next)
		rev[count++] = search;
	while(count)
	{
		search = rev[--count];
		if (!search->handle->EnumerateFiles(search->handle, match, func, parm))
			break;
	}
	BZ_Free(rev);
}

void COM_FlushTempoaryPacks(void)	//flush all temporary packages
{
	searchpath_t *sp, **link;

	COM_AssertMainThread("COM_FlushTempoaryPacks");

	if (!com_searchpaths || !fs_thread_mutex)
		return;	//we already shut down...

	COM_WorkerLock();	//make sure no workers are poking files...
	Sys_LockMutex(fs_thread_mutex);

	link = &com_searchpaths;
	while (*link)
	{
		sp = *link;
		if (sp->flags & SPF_TEMPORARY)
		{
			FS_FlushFSHashFull();

			*link = sp->next;
			com_purepaths = NULL;

			sp->handle->ClosePath(sp->handle);
			Z_Free (sp);
		}
		else
			link = &sp->next;
	}
	Sys_UnlockMutex(fs_thread_mutex);
	COM_WorkerUnlock();	//workers can continue now
}

static searchpath_t *FS_MapPackIsActive(searchpathfuncs_t *archive)
{
	searchpath_t *sp;
	Sys_LockMutex(fs_thread_mutex);
	for (sp = com_searchpaths; sp; sp = sp->next)
	{
		if (sp->handle == archive)
			break;
	}
	Sys_UnlockMutex(fs_thread_mutex);
	return sp;
}

static searchpath_t *FS_AddPathHandle(searchpath_t **oldpaths, const char *purepath, const char *probablepath, searchpathfuncs_t *handle, const char *prefix, unsigned int flags, unsigned int loadstuff);
qboolean FS_LoadMapPackFile (const char *filename, searchpathfuncs_t *archive)
{
	if (!archive)
		return false;
	if (!archive->AddReference)
		return false;	//nope...
	if (FS_MapPackIsActive(archive))
		return false;	//don't do it twice.
	archive->AddReference(archive);
	if (FS_AddPathHandle(NULL, filename, filename, archive, "", SPF_TEMPORARY, 0))
		return true;
	return false;
}
void FS_CloseMapPackFile (searchpathfuncs_t *archive)
{
	searchpath_t *sp, **link;

	COM_AssertMainThread("FS_CloseMapPackFile");

	COM_WorkerLock();	//make sure no workers are poking files...
	Sys_LockMutex(fs_thread_mutex);

	link = &com_searchpaths;
	while (*link)
	{
		sp = *link;
		if (sp->handle == archive)
		{
			FS_FlushFSHashFull();

			*link = sp->next;
			com_purepaths = NULL; //FIXME...

			sp->handle->ClosePath(sp->handle);
			Z_Free (sp);
			break;
		}
		else
			link = &sp->next;
	}

	Sys_UnlockMutex(fs_thread_mutex);
	COM_WorkerUnlock();	//workers can continue now

	archive->ClosePath(archive);
}

static searchpathfuncs_t *FS_GetOldPath(searchpath_t **oldpaths, const char *dir, unsigned int *keepflags)
{
	searchpath_t *p;
	searchpathfuncs_t *r = NULL;
	*keepflags = 0;
	while(*oldpaths)
	{
		p = *oldpaths;

		if (!Q_strcasecmp(p->logicalpath, dir))
		{
			*keepflags |= p->flags & (SPF_REFERENCED | SPF_UNTRUSTED);
			*oldpaths = p->next;
			r = p->handle;
			Z_Free(p);
			break;
		}

		oldpaths = &(*oldpaths)->next;
	}
	return r;
}

typedef struct {
	searchpathfuncs_t *(QDECL *OpenNew)(vfsfile_t *file, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix);
	searchpath_t **oldpaths;
	const char *parentdesc;
	const char *puredesc;
	unsigned int inheritflags;
} wildpaks_t;
static void FS_AddSingleDataFile (const char *descriptor, wildpaks_t *param, searchpathfuncs_t *funcs)
{
	vfsfile_t *vfs;
	searchpath_t	*search;
	searchpathfuncs_t	*newpak;
	char			pakfile[MAX_OSPATH];
	char			purefile[MAX_OSPATH];
	flocation_t loc;
	unsigned int keptflags = 0;

	Q_snprintfz (pakfile, sizeof(pakfile), "%s%s", param->parentdesc, descriptor);

	for (search = com_searchpaths; search; search = search->next)
	{
		if (!Q_strcasecmp(search->logicalpath, pakfile))	//assumption: first member of structure is a char array
			return; //already loaded (base paths?)
	}

	newpak = FS_GetOldPath(param->oldpaths, pakfile, &keptflags);
	if (!newpak)
	{
		if (param->OpenNew == VFSOS_OpenPath)
		{
			vfs = NULL;
		}
		else
		{
			fs_finds++;
			if (!funcs->FindFile(funcs, &loc, descriptor, NULL))
				return;	//not found..
			vfs = funcs->OpenVFS(funcs, &loc, "rb");
			if (!vfs)
				return;
		}
		newpak = param->OpenNew (vfs, funcs, descriptor, pakfile, "");
		if (!newpak)
		{
			VFS_CLOSE(vfs);
			return;
		}
	}

	Q_snprintfz (pakfile, sizeof(pakfile), "%s%s", param->parentdesc, descriptor);
	if (*param->puredesc)
		snprintf (purefile, sizeof(purefile), "%s/%s", param->puredesc, descriptor);
	else
		Q_strncpyz(purefile, descriptor, sizeof(purefile));
	FS_AddPathHandle(param->oldpaths, purefile, pakfile, newpak, "", ((!Q_strncasecmp(descriptor, "pak", 3))?SPF_COPYPROTECTED:0)|keptflags|param->inheritflags, (unsigned int)-1);
}
typedef struct
{
	//name table, to avoid too many reallocs
	char *names;
	size_t numnames;
	size_t maxnames;

	//file table, again to avoid excess reallocs
	struct wilddatafile_s
	{
		size_t nameofs;
		size_t size;
		time_t mtime;
		searchpathfuncs_t *source;
	} *files;
	size_t numfiles;
	size_t maxfiles;
} filelist_t;
static int QDECL FS_FindWildDataFiles (const char *descriptor, qofs_t size, time_t mtime, void *vparam, searchpathfuncs_t *funcs)
{
	filelist_t *list = vparam;
	size_t name = list->numnames;
	size_t file = list->numfiles;

	size_t dlen = strlen(descriptor);

	if (list->numnames + dlen+1 > list->maxnames)
		Z_ReallocElements((void**)&list->names, &list->maxnames, list->numnames+dlen+1+8192, sizeof(*list->names));
	strcpy(list->names + name, descriptor);
	list->numnames += dlen+1;

	if (list->numfiles + 1 > list->maxfiles)
		Z_ReallocElements((void**)&list->files, &list->maxfiles, list->numfiles+1+128, sizeof(*list->files));
	list->files[file].nameofs = name;
	list->files[file].size = size;
	list->files[file].mtime = mtime;
	list->files[file].source = funcs;
	list->numfiles += 1;

	return true;	//keep looking for more
}
static const char *qsortsucks;
static int QDECL FS_SortWildDataFiles(const void *va, const void *vb)
{
	const struct wilddatafile_s *a=va, *b=vb;
	const char *na=qsortsucks+a->nameofs, *nb=qsortsucks+b->nameofs;

	//sort by modification time...
	if (a->mtime != b->mtime && a->mtime && b->mtime && !dpcompat_ignoremodificationtimes.ival)
		return a->mtime > b->mtime;

	//then fall back and sort by name
	return strcasecmp(na, nb);
}
static void FS_LoadWildDataFiles (filelist_t *list, wildpaks_t *wp)
{
	size_t f;
	//sort them
	qsortsucks = list->names;
	qsort(list->files, list->numfiles, sizeof(*list->files), FS_SortWildDataFiles);
	qsortsucks = NULL;

	for (f = 0; f < list->numfiles; f++)
		FS_AddSingleDataFile(list->names+list->files[f].nameofs, wp, list->files[f].source);
	list->numfiles = list->numnames = 0;

	Z_Free(list->files);
	list->files = NULL;
	Z_Free(list->names);
	list->names = NULL;
	list->maxfiles = list->maxnames = 0;
}

searchpathfuncs_t *FS_OpenPackByExtension(vfsfile_t *f, searchpathfuncs_t *parent, const char *filename, const char *pakname, const char *pakpathprefix)
{
	searchpathfuncs_t *pak;
	int j;
	char ext[8];
	if (!f)
		return NULL;
	COM_FileExtension(pakname, ext, sizeof(ext));
	for (j = 0; j < sizeof(searchpathformats)/sizeof(searchpathformats[0]); j++)
	{
		if (!searchpathformats[j].extension || !searchpathformats[j].OpenNew)
			continue;
		if (!Q_strcasecmp(ext, searchpathformats[j].extension))
		{
			pak = searchpathformats[j].OpenNew(f, parent, filename, pakname, pakpathprefix);
			if (pak)
				return pak;
			Con_Printf("Unable to open %s - corrupt?\n", pakname);
			break;
		}
	}

	VFS_CLOSE(f);
	return NULL;
}

//
void FS_AddHashedPackage(searchpath_t **oldpaths, const char *parentpath, const char *logicalpaths, searchpath_t *search, unsigned int loadstuff, const char *pakpath, const char *qhash, const char *pakprefix, unsigned int packflags)
{
	searchpathfuncs_t	*handle;
	searchpath_t *oldp;
	char pname[MAX_OSPATH];
	char lname[MAX_OSPATH];
	char lname2[MAX_OSPATH];
	unsigned int	keptflags;
	flocation_t loc;
	int fmt;
	char ext[8];
	int ptlen = strlen(parentpath);

	COM_FileExtension(pakpath, ext, sizeof(ext));

	//figure out which file format its meant to be.
	for (fmt = 0; fmt < sizeof(searchpathformats)/sizeof(searchpathformats[0]); fmt++)
	{
		if (!searchpathformats[fmt].extension || !searchpathformats[fmt].OpenNew)// || !searchpathformats[i].loadscan)
			continue;
		if ((loadstuff & (1<<fmt)) && !Q_strcasecmp(ext, searchpathformats[fmt].extension))
		{
			//figure out the logical path names
			if (!FS_GenCachedPakName(pakpath, qhash, pname, sizeof(pname)))
				return;	//file name was invalid, panic.
			if (!search)
			{
				FS_SystemPath(pname, FS_ROOT, lname, sizeof(lname));
				FS_SystemPath(pakpath, FS_ROOT, lname2, sizeof(lname2));
			}
			else
			{
				snprintf (lname, sizeof(lname), "%s%s", logicalpaths, pname+ptlen+1);
				snprintf (lname2, sizeof(lname), "%s%s", logicalpaths, pakpath+ptlen+1);
			}

			//see if we already added it
			for (oldp = com_searchpaths; oldp; oldp = oldp->next)
			{
				if (strcmp(oldp->prefix, pakprefix?pakprefix:""))	//probably will only happen from typos, but should be correct.
					continue;
				if (!Q_strcasecmp(oldp->purepath, pakpath))
					break;
				if (!Q_strcasecmp(oldp->logicalpath, lname))
					break;
				if (!Q_strcasecmp(oldp->logicalpath, lname2))
					break;
			}
			if (!oldp)
			{
				//see if we can get an old archive handle from before whatever fs_restart
				handle = FS_GetOldPath(oldpaths, lname2, &keptflags);
				if (handle)
					snprintf (lname, sizeof(lname), "%s", lname2);
				else
				{
					handle = FS_GetOldPath(oldpaths, lname, &keptflags);

					//seems new, load it.
					if (!handle)
					{
						vfsfile_t *vfs = NULL;
						if (search)
						{
							if (search->handle->FindFile(search->handle, &loc, pakpath+ptlen+1, NULL))
							{
								vfs = search->handle->OpenVFS(search->handle, &loc, "rb");
								snprintf (lname, sizeof(lname), "%s", lname2);
							}
							else if (search->handle->FindFile(search->handle, &loc, pname+ptlen+1, NULL))
								vfs = search->handle->OpenVFS(search->handle, &loc, "rb");
						}
						else
						{
							vfs = FS_OpenVFS(pakpath, "rb", FS_ROOT);
							if (vfs)
								snprintf (lname, sizeof(lname), "%s", lname2);
							else
								vfs = FS_OpenVFS(pname, "rb", FS_ROOT);
						}

						if (vfs)
							handle = searchpathformats[fmt].OpenNew (vfs, search?search->handle:NULL, pakpath, lname, pakprefix?pakprefix:"");
						if (!handle && vfs)
							VFS_CLOSE(vfs);	//erk
					}
				}

				//insert it into our path lists.
				if (handle && qhash)
				{
					int truecrc = handle->GeneratePureCRC(handle, NULL);
					if (truecrc != (int)strtoul(qhash, NULL, 0))
					{
						if (pakprefix && *pakprefix)
							Con_Printf(CON_ERROR "File \"%s\" [prefix %s] has hash %#x (required: %s). Please delete it or move it away\n", lname, pakprefix, truecrc, qhash);
						else
							Con_Printf(CON_ERROR "File \"%s\" has hash %#x (required: %s). Please delete it or move it away\n", lname, truecrc, qhash);
						handle->ClosePath(handle);
						handle = NULL;
					}
				}
				if (handle)
					FS_AddPathHandle(oldpaths, pakpath, lname, handle, pakprefix, packflags|keptflags, (unsigned int)-1);
			}
			return;
		}
	}
}

static void FS_AddManifestPackages(searchpath_t **oldpaths, const char *purepath, const char *logicalpaths, searchpath_t *search, unsigned int loadstuff)
{
#ifndef PACKAGEMANAGER
	int				i;

	int ptlen, palen;
	ptlen = strlen(purepath);
	for (i = 0; i < sizeof(fs_manifest->package) / sizeof(fs_manifest->package[0]); i++)
	{
		char qhash[16];
		if (!fs_manifest->package[i].path)
			continue;

		palen = strlen(fs_manifest->package[i].path);
		if (palen > ptlen && (fs_manifest->package[i].path[ptlen] == '/' || fs_manifest->package[i].path[ptlen] == '\\' )&& !strncmp(purepath, fs_manifest->package[i].path, ptlen))
		{
			Q_snprintfz(qhash, sizeof(qhash), "%#x", fs_manifest->package[i].crc);
			FS_AddHashedPackage(oldpaths,purepath,logicalpaths,search,loadstuff, fs_manifest->package[i].path,fs_manifest->package[i].crcknown?qhash:NULL,fs_manifest->package[i].prefix, SPF_COPYPROTECTED|
#ifdef FTE_TARGET_WEB
					0	//web targets consider manifest packages as trusted, because they're about as trusted as the engine/html that goes with it.
#else
					(fs_manifest->security==MANIFEST_SECURITY_NOT?SPF_UNTRUSTED:0)
#endif
					);
		}
	}
#endif
}

static void FS_AddDownloadManifestPackages(searchpath_t **oldpaths, unsigned int loadstuff)//, const char *purepath, searchpath_t *search, const char *extension, searchpathfuncs_t *(QDECL *OpenNew)(vfsfile_t *file, const char *desc))
{
	char logicalroot[MAX_OSPATH];
	FS_SystemPath("downloads/", FS_ROOT, logicalroot, sizeof(logicalroot));

	FS_AddManifestPackages(oldpaths, "downloads", logicalroot, NULL, loadstuff);
}

static void FS_AddDataFiles(searchpath_t **oldpaths, const char *purepath, const char *logicalpath, searchpath_t *search, unsigned int pflags, unsigned int loadstuff)
{
	//search is the parent
	int				i, j;
	searchpath_t	*existing;
	searchpathfuncs_t	*handle;
	char			pakfile[MAX_OSPATH];
	char			logicalpaths[MAX_OSPATH];	//with a slash
	char			purefile[MAX_OSPATH];
	char			logicalfile[MAX_OSPATH*2];
	unsigned int	keptflags;
	vfsfile_t *vfs;
	flocation_t loc;
	wildpaks_t wp;
	filelist_t list = {0};
	qboolean qshack = (pflags&SPF_QSHACK);
	pflags &= ~SPF_QSHACK;

	Q_strncpyz(logicalpaths, logicalpath, sizeof(logicalpaths));
	FS_CleanDir(logicalpaths, sizeof(logicalpaths));
	wp.parentdesc = logicalpaths;
	wp.puredesc = purepath;
	wp.oldpaths = oldpaths;
	wp.inheritflags = pflags;

	//read pak.lst to get some sort of official ordering of pak files
	if (search->handle->FindFile(search->handle, &loc, "pak.lst", NULL) == FF_FOUND)
	{
		char filename[MAX_QPATH];
		char *buffer = BZ_Malloc(loc.len+1);
		char *names = buffer;
		search->handle->ReadFile(search->handle, &loc, buffer);
		buffer[loc.len] = 0;
		
		while (names && *names)
		{
			names = COM_ParseOut(names, filename, sizeof(filename));
			if (*filename)
			{
				char extension[MAX_QPATH];
				COM_FileExtension(filename, extension, sizeof(extension));

				//I dislike that this is tied to extensions, but whatever.
				for (j = 0; j < sizeof(searchpathformats)/sizeof(searchpathformats[0]); j++)
				{
					if (!searchpathformats[j].extension || !searchpathformats[j].OpenNew || !searchpathformats[j].loadscan)
						continue;
					if (!Q_strcasecmp(extension, searchpathformats[j].extension))
					{
						if (loadstuff & (1<<j))
						{
							wp.OpenNew = searchpathformats[j].OpenNew;
							FS_AddSingleDataFile(filename, &wp, search->handle);
						}
						break;
					}
				}
			}
		}
		BZ_Free(buffer);
	}

#ifdef PACKAGEMANAGER
	PM_LoadPackages(oldpaths, purepath, logicalpaths, search, loadstuff, 0x80000000, -1);
#endif

	for (j = 0; j < sizeof(searchpathformats)/sizeof(searchpathformats[0]); j++)
	{
		if (!searchpathformats[j].extension || !searchpathformats[j].OpenNew || !searchpathformats[j].loadscan)
			continue;
		if (loadstuff & (1<<j))
		{
			qboolean okay = true;
			const char *extension = searchpathformats[j].extension;

			//first load all the numbered pak files
			for (i=0 ; okay ; i++)
			{
				snprintf (pakfile, sizeof(pakfile), "pak%i.%s", i, extension);
				fs_finds++;
				if (search->handle->FindFile(search->handle, &loc, pakfile, NULL))
				{
					snprintf (logicalfile, sizeof(logicalfile), "%spak%i.%s", logicalpaths, i, extension);
					snprintf (purefile, sizeof(purefile), "%s/pak%i.%s", purepath, i, extension);

					for (existing = com_searchpaths; existing; existing = existing->next)
					{
						if (!Q_strcasecmp(existing->logicalpath, logicalfile))	//assumption: first member of structure is a char array
							break; //already loaded (base paths?)
					}
					if (!existing)
					{
						handle = FS_GetOldPath(oldpaths, logicalfile, &keptflags);
						if (!handle)
						{
							vfs = search->handle->OpenVFS(search->handle, &loc, "rb");
							if (!vfs)
								break;
							handle = searchpathformats[j].OpenNew (vfs, search->handle, pakfile, logicalfile, "");
							if (!handle)
								break;
						}
						FS_AddPathHandle(oldpaths, purefile, logicalfile, handle, "", SPF_COPYPROTECTED|pflags|keptflags, (unsigned int)-1);
					}
				}
				else
					okay = false;

				if (i == 0 && qshack)
				{
					snprintf (pakfile, sizeof(pakfile), "quakespasm.%s", extension);
					handle = FS_GetOldPath(oldpaths, logicalfile, &keptflags);
					if (!handle)
						handle = FS_OpenPackByExtension(VFSOS_Open(pakfile, "rb"), NULL, pakfile, pakfile, "");
					if (handle)	//logically should have SPF_EXPLICIT set, but that would give it a worse gamedir depth
						FS_AddPathHandle(oldpaths, "", pakfile, handle, "", SPF_COPYPROTECTED|SPF_PRIVATE, (unsigned int)-1);
				}
			}
		}
	}

	//now load ones from the manifest
	FS_AddManifestPackages(oldpaths, purepath, logicalpaths, search, loadstuff);

#ifdef PACKAGEMANAGER
	PM_LoadPackages(oldpaths, purepath, logicalpaths, search, loadstuff, 0x0, 1000-1);
#endif

	//now load the random ones
	for (j = 0; j < sizeof(searchpathformats)/sizeof(searchpathformats[0]); j++)
	{
		if (!searchpathformats[j].extension || !searchpathformats[j].OpenNew || !searchpathformats[j].loadscan)
			continue;
		if (loadstuff & (1<<j))
		{
			const char *extension = searchpathformats[j].extension;
			wp.OpenNew = searchpathformats[j].OpenNew;

			Q_snprintfz (pakfile, sizeof(pakfile), "*.%s", extension);
			search->handle->EnumerateFiles(search->handle, pakfile, FS_FindWildDataFiles, &list);
			FS_LoadWildDataFiles(&list, &wp);
		}
	}

#ifdef PACKAGEMANAGER
	PM_LoadPackages(oldpaths, purepath, logicalpaths, search, loadstuff, 1000, 0x7ffffffe);
#endif
}

static searchpath_t *FS_AddPathHandle(searchpath_t **oldpaths, const char *purepath, const char *logicalpath, searchpathfuncs_t *handle, const char *prefix, unsigned int flags, unsigned int loadstuff)
{
	searchpath_t *search, **link;

	if (!handle)
	{
		Con_Printf("COM_AddPathHandle: not a valid handle (%s)\n", logicalpath);
		return NULL;
	}

	if (handle->fsver != FSVER)
	{
		Con_Printf("%s: file system driver is outdated (%u should be %u)\n", logicalpath, handle->fsver, FSVER);
		handle->ClosePath(handle);
		return NULL;
	}

	search = (searchpath_t*)Z_Malloc (sizeof(searchpath_t));
	search->handle = handle;
	Q_strncpyz(search->purepath, purepath, sizeof(search->purepath));
	Q_strncpyz(search->logicalpath, logicalpath, sizeof(search->logicalpath));
	if (prefix && *prefix)
	{
		Q_strncpyz(search->prefix, prefix, sizeof(search->prefix));
		flags |= SPF_COPYPROTECTED; //don't do downloading weirdness when there's weird prefix shenanegans going on.
	}
	search->flags = flags;

	search->crc_check = 0;
	search->crc_seed = ~fs_pureseed;
	search->crc_reply = 0;
	if (search->handle->GeneratePureCRC)
	{
		search->crc_check = search->handle->GeneratePureCRC(search->handle, NULL);
		if (flags & SPF_SERVER)
		{
			search->crc_seed = fs_pureseed;
			search->crc_reply = search->handle->GeneratePureCRC(search->handle, &search->crc_seed);
		}
	}

	flags &= ~SPF_WRITABLE;

	//temp packages also do not nest
	if (!(flags & SPF_TEMPORARY))
		FS_AddDataFiles(oldpaths, purepath, logicalpath, search, flags&(SPF_COPYPROTECTED|SPF_UNTRUSTED|SPF_TEMPORARY|SPF_SERVER|SPF_PRIVATE|SPF_QSHACK), loadstuff);

	search->nextpure = (void*)0x1;	//mark as not linked

	if (flags & (SPF_TEMPORARY|SPF_SERVER))
	{
		int depth = 1;
		searchpath_t *s;
		//add at end. pureness will reorder if needed.
		link = &com_searchpaths;
		while(*link)
		{
			link = &(*link)->next;
		}

		if (com_purepaths)
		{	//go for the pure paths first.
			for (s = com_purepaths; s; s = s->nextpure)
				depth++;
		}
		if (fs_puremode < 2)
		{
			for (s = com_searchpaths ; s ; s = s->next)
				depth++;
		}
		*link = search;

		if (filesystemhash.numbuckets)
			search->handle->BuildHash(search->handle, depth, FS_AddFileHashUnsafe);
	}
	else
	{
		search->next = com_searchpaths;
		com_searchpaths = search;

		com_fschanged = true;	//depth values are screwy
	}

	return search;
}

static void COM_RefreshFSCache_f(void)
{
	com_fschanged=true;
}

//optionally purges the cache and rebuilds it
void COM_FlushFSCache(qboolean purge, qboolean domutex)
{
	searchpath_t *search;
	if (com_fs_cache.ival && com_fs_cache.ival != 2)
	{
		for (search = com_searchpaths ; search ; search = search->next)
		{
			if (search->handle->PollChanges)
				com_fschanged |= search->handle->PollChanges(search->handle);
		}
	}

#ifdef FTE_TARGET_WEB
	//web target doesn't support filesystem enumeration, so make sure the cache is kept invalid and disabled.
	com_fschanged = true;
#else
	if (com_fs_cache.ival && com_fschanged)
	{
		//rebuild it if needed
		FS_RebuildFSHash(domutex);
	}
#endif
}

/*since should start as 0, otherwise this can be used to poll*/
qboolean FS_Restarted(unsigned int *since)
{
	if (*since < fs_restarts)
	{
		*since = fs_restarts;
		return true;
	}
	return false;
}

#ifdef __WIN32	//already assumed to be case insensitive. let the OS keep fixing up the paths itself.
static qboolean FS_FixupFileCase(char *out, size_t outsize, const char *basedir, const char *entry, qboolean isdir)
{
	return Q_snprintfz(out, outsize, "%s%s", basedir, entry) < outsize;
}
#else
struct fixupcase_s
{
	char *out;
	size_t outsize;
	const char *match;
	size_t matchlen;
	qboolean isdir;	//directory results have a trailing /
};
static int FS_FixupFileCaseResult(const char *name, qofs_t sz, time_t modtime, void *vparm, searchpathfuncs_t *spath)
{
	struct fixupcase_s *parm = vparm;
	if (strlen(name) != parm->matchlen+parm->isdir)
		return true;
	if (parm->isdir && name[parm->matchlen] != '/')
		return true;
	if (Q_strncasecmp(name, parm->match, parm->matchlen))
		return true;
	memcpy(parm->out, name, parm->matchlen);
	return !!Q_strncmp(name, parm->match, parm->matchlen);	//stop if we find the exact path case. otherwise keep looking
}
//like snprintf("%s%s") but fixes up 'gamedir' case to a real file
static qboolean FS_FixupFileCase(char *out, size_t outsize, const char *basedir, const char *entry, qboolean isdir)
{
	char *s;
	struct fixupcase_s parm = {out+strlen(basedir), outsize-strlen(basedir), entry, strlen(entry), isdir};
	if (Q_snprintfz(out, outsize, "%s%s", basedir, entry) >= outsize || outsize < strlen(basedir)+1 || parm.outsize < parm.matchlen+1)
		return false;	//over sized?...
	if (strchr(entry, '/')) for(;;)
	{
		parm.match = entry;
		s = strchr(entry, '/');
		if (s)
		{
			parm.isdir = true;
			parm.matchlen = s-entry;
			Sys_EnumerateFiles(basedir, "*", FS_FixupFileCaseResult, &parm, NULL);
			parm.out += parm.matchlen+1;
			parm.outsize -= parm.matchlen+1;
			entry += (s-entry)+1;
		}
		else
		{
			parm.isdir = isdir;
			parm.matchlen = strlen(entry);
			parm.out[-1] = 0;
			Sys_EnumerateFiles(out, "*", FS_FixupFileCaseResult, &parm, NULL);
			parm.out[-1] = '/';
			break;
		}
	}
	else
		Sys_EnumerateFiles(basedir, "*", FS_FixupFileCaseResult, &parm, NULL);
	return true;
}
#endif

/*
================
FS_AddGameDirectory

Sets com_gamedir, adds the directory to the head of the path,
then loads and adds pak1.pak pak2.pak ...
================
*/
static searchpath_t *FS_AddSingleGameDirectory (searchpath_t **oldpaths, const char *puredir, const char *dir, unsigned int loadstuff, unsigned int flags)
{
	unsigned int	keptflags;
	searchpath_t	*search;

	char			*p;
	void			*handle;

	fs_restarts++;

	for (search = com_searchpaths; search; search = search->next)
	{
		if (!Q_strcasecmp(search->logicalpath, dir))
		{
			search->flags |= flags & SPF_WRITABLE;
			return search; //already loaded (base paths?)
		}
	}

	if (!(flags & SPF_PRIVATE))
	{
		if ((p = strrchr(dir, '/')) != NULL)
			Q_strncpyz(pubgamedirfile, ++p, sizeof(pubgamedirfile));
		else
			Q_strncpyz(pubgamedirfile, dir, sizeof(pubgamedirfile));
	}
	if ((p = strrchr(dir, '/')) != NULL)
		Q_strncpyz(gamedirfile, ++p, sizeof(gamedirfile));
	else
		Q_strncpyz(gamedirfile, dir, sizeof(gamedirfile));

//
// add the directory to the search path
//
	handle = FS_GetOldPath(oldpaths, dir, &keptflags);
	if (!handle)
		handle = VFSOS_OpenPath(NULL, NULL, dir, dir, "");

	return FS_AddPathHandle(oldpaths, puredir, dir, handle, "", flags|keptflags|SPF_ISDIR, loadstuff);
}
static void FS_AddGameDirectory (searchpath_t **oldpaths, const char *puredir, unsigned int loadstuff, unsigned int flags)
{
	char syspath[MAX_OSPATH];
	if (FS_FixupFileCase(syspath, sizeof(syspath),  com_gamepath, puredir, true))
		gameonly_gamedir = FS_AddSingleGameDirectory(oldpaths, puredir, syspath, loadstuff, flags&~(com_homepathenabled?SPF_WRITABLE:0u));
	else
		gameonly_gamedir = NULL;
	if (com_homepathenabled && FS_FixupFileCase(syspath, sizeof(syspath), com_homepath, puredir, true))
		gameonly_homedir = FS_AddSingleGameDirectory(oldpaths, puredir, syspath, loadstuff, flags);
	else
		gameonly_homedir = NULL;
}

//if syspath, something like c:\quake\baseq2
//otherwise just baseq2. beware of dupes.
searchpathfuncs_t *COM_IteratePaths (void **iterator, char *pathbuffer, int pathbuffersize, char *dirname, int dirnamesize)
{
	searchpath_t	*s;
	void			*prev;

	prev = NULL;
	for (s=com_searchpaths ; s ; s=s->next)
	{
		if (!(s->flags & SPF_EXPLICIT))
			continue;

		if (*iterator == prev)
		{
			*iterator = s->handle;
			if (!strchr(s->purepath, '/'))
			{
				if (pathbuffer)
				{
					Q_strncpyz(pathbuffer, s->logicalpath, pathbuffersize-1);
					FS_CleanDir(pathbuffer, pathbuffersize);
				}
				if (dirname)
				{
					Q_strncpyz(dirname, s->purepath, dirnamesize-1);
				}
				return s->handle;
			}
		}
		prev = s->handle;
	}

	*iterator = NULL;
	if (pathbuffer)
		*pathbuffer = 0;
	if (dirname)
		*dirname = 0;
	return NULL;
}

char *FS_GetGamedir(qboolean publicpathonly)
{
	if (publicpathonly)
		return pubgamedirfile;
	else
		return gamedirfile;
}

//returns the commandline arguments required to duplicate the fs details
char *FS_GetManifestArgs(void)
{
	char *homearg = com_homepathenabled?"-usehome ":"-nohome ";
	if (fs_manifest->filename)
		return va("%s-manifest %s -basedir %s", homearg, fs_manifest->filename, com_gamepath);
	
	return va("%s-game %s -basedir %s", homearg, pubgamedirfile, com_gamepath);
}
#ifdef SUBSERVERS
int FS_GetManifestArgv(char **argv, int maxargs)
{
	int c = 0;
	if (maxargs < 5)
		return c;
	argv[c++] = com_homepathenabled?"-usehome ":"-nohome ";
	if (fs_manifest->filename)
	{
		argv[c++] = "-manifest";
		argv[c++] = fs_manifest->filename;
	}
	else
	{
		argv[c++] = "-game";
		argv[c++] = pubgamedirfile;
	}

	argv[c++] = "-basedir";
	argv[c++] = com_gamepath;

	argv[c++] = "+deathmatch";
	argv[c++] = *deathmatch.string?deathmatch.string:"0";

	argv[c++] = "+coop";
	argv[c++] = *coop.string?coop.string:"0";
	return c;
}
#endif

/*
//given a 'c:/foo/bar/' path, will extract 'bar'.
static void FS_ExtractDir(char *in, char *out, int outlen)
{
	char *end;
	if (!outlen)
		return;
	end = in + strlen(in);
	//skip over any trailing slashes
	while (end > in)
	{
		if (end[-1] == '/' || end[-1] == '\\')
			end--;
		else
			break;
	}

	//skip over the path
	while (end > in)
	{
		if (end[-1] != '/' && end[-1] != '\\')
			end--;
		else
			break;
	}

	//copy string into the dest
	while (--outlen)
	{
		if (*end == '/' || *end == '\\' || !*end)
			break;
		*out++ = *end++;
	}
	*out = 0;
}*/

qboolean FS_PathURLCache(const char *url, char *path, size_t pathsize)
{
	char tmp[MAX_QPATH];
	char *o = tmp;
	const char *i = url;
	strcpy(o, "downloads/");
	o += strlen(o);
	while(*i)
	{
		if (*i == ':' || *i == '?' || *i == '*' || *i == '&')
		{
			if (i[0] == ':' && i[1] == '/' && i[2] == '/')
			{
				i+=2;
				continue;
			}
			*o++ = '_';
			i++;
			continue;
		}
		if (*i == '\\')
		{
			*o++ = '/';
			i++;
			continue;
		}
		*o++ = *i++;
	}
	*o = 0;

	if (!FS_GetCleanPath(tmp, false, path, pathsize))
		return false;

	return true;
}

static ftemanifest_t *FS_Manifest_ChangeGameDir(const char *newgamedir)
{
	ftemanifest_t *man;

	if (*newgamedir && !FS_GamedirIsOkay(newgamedir))
		return fs_manifest;

	man = FS_Manifest_ReadMod(newgamedir);

	if (!man)
	{
		//generate a new manifest based upon the current one.
		man = FS_ReadDefaultManifest(com_gamepath, sizeof(com_gamepath), true);
		if (man && strcmp(man->installation, fs_manifest->installation))
		{
			FS_Manifest_Free(man);
			man = NULL;
		}
		if (!man)
			man = FS_Manifest_Clone(fs_manifest);
		FS_Manifest_PurgeGamedirs(man);
		if (*newgamedir)
		{
			char token[MAX_QPATH], quot[MAX_QPATH];
			char *dup = Z_StrDup(newgamedir);	//FIXME: is this really needed?
			newgamedir = dup;
			while ((newgamedir = COM_ParseStringSet(newgamedir, token, sizeof(token))))
			{
				if (!strcmp(newgamedir, ";"))
					continue;
				if (!*token)
					continue;

				Cmd_TokenizeString(va("gamedir %s", COM_QuotedString(token, quot, sizeof(quot), false)), false, false);
				FS_Manifest_ParseTokens(man);
			}
			Z_Free(dup);
		}
	}
	return man;
}

/*
================
COM_Gamedir

Sets the gamedir and path to a different directory.
================
*/
void COM_Gamedir (const char *dir, const struct gamepacks *packagespaths)
{
	ftemanifest_t *man;
	COM_FlushTempoaryPacks();

	if (!fs_manifest)
		FS_ChangeGame(NULL, true, false);

	//we do allow empty here, for base.
	if (*dir && !FS_GamedirIsOkay(dir))
	{
		Con_Printf ("Gamedir should be a single filename, not \"%s\"\n", dir);
		return;
	}

	man = FS_Manifest_ChangeGameDir(dir);
	while(packagespaths && (packagespaths->package || packagespaths->path))
	{
		char quot[MAX_QPATH];
		char quot2[MAX_OSPATH];
		char quot3[MAX_OSPATH];
		char quot4[MAX_OSPATH];
		Cmd_TokenizeString(va("package %s %s%s %s%s %s%s",
					COM_QuotedString(packagespaths->path, quot, sizeof(quot), false),	//name
					packagespaths->subpath?"prefix ":"", packagespaths->subpath?COM_QuotedString(packagespaths->subpath, quot2, sizeof(quot2), false):"",	//prefix
					packagespaths->url    ?"mirror ":"", packagespaths->url    ?COM_QuotedString(packagespaths->url, quot3, sizeof(quot3), false):"",	//mirror
					packagespaths->package?"name "  :"", packagespaths->package?COM_QuotedString(packagespaths->package, quot4, sizeof(quot4), false):""	//
				), false, false);
		FS_Manifest_ParseTokens(man);
		packagespaths++;
	}
	FS_ChangeGame(man, cfg_reload_on_gamedir.ival, false);

#ifdef HAVE_SERVER
	if (!*dir)
		dir = FS_GetGamedir(true);
	InfoBuf_SetStarKey (&svs.info, "*gamedir", dir);
#endif
}

static void QDECL Q_strnlowercatz(char *d, const char *s, int n)
{
	int c = strlen(d);
	d += c;
	n -= c;
	n -= 1;	//for the null
	while (*s && n-- > 0)
	{
		if (*s >= 'A' && *s <= 'Z')
			*d = (*s-'A') + 'a';
		else
			*d = *s;
		d++;
		s++;
	}
	*d = 0;
}

//pname must be of the form "gamedir/foo.pk3"
//as a special exception, we allow "downloads/*.pk3 too"
qboolean FS_GenCachedPakName(const char *pname, const char *crc, char *local, int llen)
{
	const char *fn;
	char hex[16];
	if (strstr(pname, "dlcache"))
	{
		*local = 0;
		return false;
	}

	if (!strncmp(pname, "downloads/", 10))
	{
		*local = 0;
		Q_strnlowercatz(local, pname, llen);
		return true;
	}

	for (fn = pname; *fn; fn++)
	{
		if (*fn == '\\' || *fn == '/')
		{
			fn++;
			break;
		}
	}
//	fn = COM_SkipPath(pname);
	if (fn == pname || !*fn)
	{	//only allow it if it has some game path first.
		*local = 0;
		return false;
	}
	Q_strncpyz(local, pname, min((fn - pname) + 1, llen));
	Q_strncatz(local, "dlcache/", llen);
	Q_strnlowercatz(local, fn, llen);
	if (crc && *crc)
	{
		Q_strncatz(local, ".", llen);
		snprintf(hex, sizeof(hex), "%0x", (unsigned int)strtoul(crc, NULL, 0));
		Q_strncatz(local, hex, llen);
	}
	return true;
}

#ifdef HAVE_CLIENT
#if 0
qboolean FS_LoadPackageFromFile(vfsfile_t *vfs, char *pname, char *localname, int *crc, unsigned int flags)
{
	int i;
	char *ext = "zip";//(pname);
	searchpathfuncs_t *handle;
	searchpath_t *oldlist = NULL;

	searchpath_t *sp;

	com_fschanged = true;

	for (i = 0; i < sizeof(searchpathformats)/sizeof(searchpathformats[0]); i++)
	{
		if (!searchpathformats[i].extension || !searchpathformats[i].OpenNew)
			continue;
		if (!Q_strcasecmp(ext, searchpathformats[i].extension))
		{
			handle = searchpathformats[i].OpenNew (vfs, localname);
			if (!handle)
			{
				Con_Printf("file %s isn't a %s after all\n", pname, searchpathformats[i].extension);
				break;
			}
			if (crc)
			{
				int truecrc = handle->GeneratePureCRC(handle, 0, false);
				if (truecrc != *crc)
				{
					*crc = truecrc;
					handle->ClosePath(handle);
					return false;
				}
			}
			sp = FS_AddPathHandle(&oldlist, pname, localname, handle, flags, (unsigned int)-1);

			if (sp)
			{
				com_fschanged = true;
				return true;
			}
		}
	}

	VFS_CLOSE(vfs);
	return false;
}
#endif

//'small' wrapper to open foo.zip/bar to read files within zips that are not part of the gamedir.
//name needs to be null terminated. recursive. pass null for search.
//name is restored to its original state after the call, only technically not const
vfsfile_t *CL_OpenFileInPackage(searchpathfuncs_t *search, char *name)
{
	int found;
	vfsfile_t *f;
	flocation_t loc;
	char e, *n;
	char ext[8];
	char *end;
	int i;

	//keep chopping off the last part of the filename until we get an actual package
	//once we do, recurse into that package

	end = name + strlen(name);

	while (end > name)
	{
		e = *end;
		*end = 0;

		if (!e)
		{
			//always open the last file properly.
			loc.search = NULL;
			if (search)
				found = search->FindFile(search, &loc, name, NULL);
			else
				found = FS_FLocateFile(name, FSLF_IFFOUND, &loc); 
			if (found)
			{
				f = (search?search:loc.search->handle)->OpenVFS(search?search:loc.search->handle, &loc, "rb");
				if (f)
					return f;
			}
		}
		else
		{
			COM_FileExtension(name, ext, sizeof(ext));
			for (i = 0; i < sizeof(searchpathformats)/sizeof(searchpathformats[0]); i++)
			{
				if (!searchpathformats[i].extension || !searchpathformats[i].OpenNew)
					continue;
				if (!Q_strcasecmp(ext, searchpathformats[i].extension))
				{
					loc.search = NULL;
					if (search)
						found = search->FindFile(search, &loc, name, NULL);
					else
						found = FS_FLocateFile(name, FSLF_IFFOUND, &loc); 
					if (found)
					{
						f = (search?search:loc.search->handle)->OpenVFS(search?search:loc.search->handle, &loc, "rb");
						if (f)
						{
							searchpathfuncs_t *newsearch = searchpathformats[i].OpenNew(f, search?search:loc.search->handle, name, name, "");
							if (newsearch)
							{
								f = CL_OpenFileInPackage(newsearch, end+1);
								newsearch->ClosePath(newsearch);
								if (f)
								{
									*end = e;
									return f;
								}
							}
							else
								VFS_CLOSE(f);
						}
						break;
					}
				}
			}
		}

		n = COM_SkipPath(name);
		*end = e;
		end = n-1;
	}
	return NULL;
}

//some annoying struct+func to prefix the enumerated file name properly.
struct CL_ListFilesInPackageCB_s
{
	char *nameprefix;
	size_t nameprefixlen;

	int (QDECL *func)(const char *fname, qofs_t fsize, time_t mtime, void *parm, searchpathfuncs_t *spath);
	void *parm;
	searchpathfuncs_t *spath;
};
static int QDECL CL_ListFilesInPackageCB(const char *fname, qofs_t fsize, time_t mtime, void *parm, searchpathfuncs_t *spath)
{
	struct CL_ListFilesInPackageCB_s *cb = parm;
	char name[MAX_OSPATH];
	if (cb->nameprefixlen)
	{
		memcpy(name, cb->nameprefix, cb->nameprefixlen-1);
		name[cb->nameprefixlen-1] = '/';
		Q_strncpyz(name+cb->nameprefixlen, fname, sizeof(name)-(cb->nameprefixlen));
		return cb->func(name, fsize, mtime, cb->parm, cb->spath);
	}
	else
		return cb->func(fname, fsize, mtime, cb->parm, cb->spath);
}

//'small' wrapper to list foo.zip/* to list files within zips that are not part of the gamedir.
//same rules as CL_OpenFileInPackage, except that wildcards should only be in the final part
qboolean CL_ListFilesInPackage(searchpathfuncs_t *search, char *name, int (QDECL *func)(const char *fname, qofs_t fsize, time_t mtime, void *parm, searchpathfuncs_t *spath), void *parm, void *recursioninfo)
{
	int found;
	vfsfile_t *f;
	flocation_t loc;
	char e, *n;
	char ext[8];
	char *end;
	int i;
	qboolean ret = false;
	struct CL_ListFilesInPackageCB_s cb;
	cb.nameprefix = recursioninfo?recursioninfo:name;
	cb.nameprefixlen = name-cb.nameprefix;
	cb.func = func;
	cb.parm = parm;

	//keep chopping off the last part of the filename until we get an actual package
	//once we do, recurse into that package

	end = name + strlen(name);

	while (end > name)
	{
		e = *end;
		*end = 0;

		COM_FileExtension(name, ext, sizeof(ext));
		for (i = 0; i < countof(searchpathformats); i++)
		{
			if (!searchpathformats[i].extension || !searchpathformats[i].OpenNew)
				continue;
			if (!Q_strcasecmp(ext, searchpathformats[i].extension))
			{
				loc.search = NULL;
				if (search)
					found = search->FindFile(search, &loc, name, NULL);
				else
					found = FS_FLocateFile(name, FSLF_IFFOUND, &loc); 
				if (found)
				{
					f = (search?search:loc.search->handle)->OpenVFS(search?search:loc.search->handle, &loc, "rb");
					if (f)
					{
						searchpathfuncs_t *newsearch = searchpathformats[i].OpenNew(f, search?search:loc.search->handle, name, name, "");
						if (newsearch)
						{
							ret = CL_ListFilesInPackage(newsearch, end+1, func, parm, cb.nameprefix);
							newsearch->ClosePath(newsearch);
							if (ret)
							{
								*end = e;
								return ret;
							}
						}
						else
							VFS_CLOSE(f);
					}
				}
				break;
			}
		}

		n = COM_SkipPath(name);
		*end = e;
		end = n-1;
	}

	//always open the last file properly.
	loc.search = NULL;
	if (search)
		ret = search->EnumerateFiles(search, name, CL_ListFilesInPackageCB, &cb);
	else
	{
		ret = true;
		if (ret)
			COM_EnumerateFiles(name, CL_ListFilesInPackageCB, &cb);
	}
	return ret;
}

void FS_PureMode(const char *gamedir, int puremode, char *purenamelist, char *purecrclist, char *refnamelist, char *refcrclist, int pureseed)
{
	ftemanifest_t *man;
	qboolean pureflush;

#ifdef HAVE_SERVER
	//if we're the server, we can't be impure.
	if (sv.state)
		return;
#endif

	if (puremode == fs_puremode && fs_pureseed == pureseed)
	{
		if ((!purenamelist && !fs_purenames) || !strcmp(fs_purenames?fs_purenames:"", purenamelist?purenamelist:""))
			if ((!purecrclist && !fs_purecrcs) || !strcmp(fs_purecrcs?fs_purecrcs:"", purecrclist?purecrclist:""))
				if ((!refnamelist && !fs_refnames) || !strcmp(fs_refnames?fs_refnames:"", refnamelist?refnamelist:""))
					if ((!refcrclist && !fs_refcrcs) || !strcmp(fs_refcrcs?fs_refcrcs:"", refcrclist?refcrclist:""))
						return;
	}

	Z_Free(fs_purenames);
	Z_Free(fs_purecrcs);
	Z_Free(fs_refnames);
	Z_Free(fs_refcrcs);

	pureflush = (fs_puremode != 2 && puremode == 2);
	fs_puremode = puremode;
	fs_purenames = purenamelist?Z_StrDup(purenamelist):NULL;
	fs_purecrcs = purecrclist?Z_StrDup(purecrclist):NULL;
	fs_pureseed = pureseed;
	fs_refnames = refnamelist?Z_StrDup(refnamelist):NULL;
	fs_refcrcs = refcrclist?Z_StrDup(refcrclist):NULL;

	if (gamedir)
		man	= FS_Manifest_ChangeGameDir(gamedir);
	else
		man = fs_manifest;

	FS_ChangeGame(man, false, false);

	if (pureflush)
	{
#ifdef HAVE_CLIENT
		Shader_NeedReload(true);
#endif
		Mod_ClearAll();
		Cache_Flush();
	}
}

int FS_PureOkay(void)
{
	qboolean ret = true;
	//returns true if all pure packages that we're meant to need could load.
	//if they couldn't then they won't override things, or the game will just be completely screwed due to having absolutely no game data
	if (fs_puremode == 1 && fs_purenames && *fs_purenames && fs_purecrcs && *fs_purecrcs)
	{
		char crctok[64];
		char nametok[MAX_QPATH];
		char nametok2[MAX_QPATH];
		searchpath_t *sp = com_purepaths;
		char *names = fs_purenames, *pname;
		char *crcs = fs_purecrcs;
		int crc;
		qboolean required;

		while(names && crcs)
		{
			crcs = COM_ParseOut(crcs, crctok, sizeof(crctok));
			names = COM_ParseOut(names, nametok, sizeof(nametok));

			crc = strtoul(crctok, NULL, 0);
			if (!crc)
				continue;

			pname = nametok;

			if (fs_refnames && fs_refcrcs)
			{	//q3 is annoying as hell
				int crc2;
				char *rc = fs_refcrcs;
				char *rn = fs_refnames;
				pname = "";
				for (; rc && rn; )
				{
					rc = COM_ParseOut(rc, crctok, sizeof(crctok));
					rn = COM_ParseOut(rn, nametok2, sizeof(nametok2));
					crc2 = strtoul(crctok, NULL, 0);
					if (crc2 == crc)
					{
						COM_DefaultExtension(nametok2, ".pk3", sizeof(nametok2));
						pname = nametok2;
						break;
					}
				}
			}
			required = *pname == '*';
			if (*pname == '*')	// * means that its 'referenced' (read: actually useful) thus should be downloaded, which is not relevent here.
			{
				required = true;
				pname++;
			}
			else
				required = false;

			if (sp && sp->crc_check == crc)
			{
				sp = sp->nextpure;
				continue;
			}
			else if (!required)	//if its not referenced, then its not needed, and we probably didn't bother to download it. this might be an issue with sv_pure 1, but that has its own issues.
				continue;
			else //if (!sp)
			{
//				if (!CL_CheckDLFile(va("package/%s", pname)))
//					if (CL_CheckOrEnqueDownloadFile(pname, va("%s.%i", pname, crc), DLLF_NONGAME))
//						return -1;
				Con_Printf(CON_ERROR"Pure package %s:%08x missing.\n", pname, crc);
				ret = false;
			}
		}
	}

	return ret;
}
#endif

#ifdef Q3CLIENT
char *FSQ3_GenerateClientPacksList(char *buffer, int maxlen, int basechecksum)
{	//this is for q3 compatibility.

	flocation_t loc;
	int numpaks = 0;
	searchpath_t *sp;

	if (FS_FLocateFile("vm/cgame.qvm", FSLF_IFFOUND, &loc))
	{
		Q_strncatz(buffer, va("%i ", loc.search->crc_reply), maxlen);
		basechecksum ^= loc.search->crc_reply;
	}
	else Q_strncatz(buffer, va("%i ", 0), maxlen);

	if (FS_FLocateFile("vm/ui.qvm", FSLF_IFFOUND, &loc))
	{
		Q_strncatz(buffer, va("%i ", loc.search->crc_reply), maxlen);
		basechecksum ^= loc.search->crc_reply;
	}
	else Q_strncatz(buffer, va("%i ", 0), maxlen);

	Q_strncatz(buffer, "@ ", maxlen);

	for (sp = com_purepaths; sp; sp = sp->nextpure)
	{
		if (sp->crc_reply)
		{
			Q_strncatz(buffer, va("%i ", sp->crc_reply), maxlen);
			basechecksum ^= sp->crc_reply;
			numpaks++;
		}
	}

	basechecksum ^= numpaks;
	Q_strncatz(buffer, va("%i ", basechecksum), maxlen);

	return buffer;
}
#endif

/*
================
FS_ReloadPackFiles
================

Called when the client has downloaded a new pak/pk3 file
*/
static void FS_ReloadPackFilesFlags(unsigned int reloadflags)
{
	searchpath_t	*oldpaths;
	searchpath_t	*next;
	int i, j;
	int orderkey;
	unsigned int fl;

	COM_AssertMainThread("FS_ReloadPackFilesFlags");
	COM_WorkerFullSync();

	orderkey = 0;
	if (com_purepaths)
		for (next = com_purepaths; next; next = next->nextpure)
			next->orderkey = ++orderkey;
	if (fs_puremode < 2)
		for (next = com_purepaths; next; next = next->nextpure)
			next->orderkey = ++orderkey;

	oldpaths = com_searchpaths;
	com_searchpaths = NULL;
	com_purepaths = NULL;
	com_base_searchpaths = NULL;
	gameonly_gamedir = gameonly_homedir = NULL;

#if defined(ENGINE_HAS_ZIP) && defined(PACKAGE_PK3)
	{
		searchpathfuncs_t *pak;
		vfsfile_t *vfs;
		vfs = VFSOS_Open(com_argv[0], "rb");
		pak = FSZIP_LoadArchive(vfs, NULL, com_argv[0], com_argv[0], "");
		if (pak)	//logically should have SPF_EXPLICIT set, but that would give it a worse gamedir depth
		{
			FS_AddPathHandle(&oldpaths, "", com_argv[0], pak, "", SPF_COPYPROTECTED, reloadflags);
		}
	}
#endif

#if defined(HAVE_LEGACY) && defined(PACKAGE_PK3)
	{
		searchpathfuncs_t *pak;
		vfsfile_t *vfs;
		char pakname[MAX_OSPATH];
		int i;
		static char *names[] = {"QuakeEX.kpf", "Q2Game.kpf"};	//need a better way to handle this rubbish. fucking mod-specific translations being stored in the engine-specific data.
		for (i = 0; i < countof(names); i++)
		{
			Q_snprintfz(pakname, sizeof(pakname), "%s%s", com_gamepath, names[i]);
			vfs = VFSOS_Open(pakname, "rb");
			pak = FSZIP_LoadArchive(vfs, NULL, pakname, pakname, "");
			if (pak)	//logically should have SPF_EXPLICIT set, but that would give it a worse gamedir depth
			{
				FS_AddPathHandle(&oldpaths, "", pakname, pak, "", SPF_COPYPROTECTED, reloadflags);
				break;
			}
		}
	}
#endif

	i = COM_CheckParm ("-basepack");
	while (i && i < com_argc-1)
	{
		const char *pakname = com_argv[i+1];
		searchpathfuncs_t *pak;
		vfsfile_t *vfs = VFSOS_Open(pakname, "rb");
		pak = FS_OpenPackByExtension(vfs, NULL, pakname, pakname, "");
		if (pak)	//logically should have SPF_EXPLICIT set, but that would give it a worse gamedir depth
			FS_AddPathHandle(&oldpaths, "", pakname, pak, "", SPF_COPYPROTECTED, reloadflags);
		i = COM_CheckNextParm ("-basepack", i);
	}

#ifdef NQPROT
	standard_quake = true;
#endif
	for (i = 0; i < countof(fs_manifest->gamepath); i++)
	{
		char *dir = fs_manifest->gamepath[i].path;
		if (dir && (fs_manifest->gamepath[i].flags&GAMEDIR_BASEGAME))
		{
			//paths should be validated before here, when parsing the manifest.
			
#ifdef NQPROT
			//vanilla NQ uses a slightly different protocol when started with -rogue or -hipnotic (and by extension -quoth).
			//QW+FTE protocols don't care so we can get away with being a little loose here
			if (!strcmp(dir, "rogue") || !strcmp(dir, "hipnotic") || !strcmp(dir, "quoth"))
				standard_quake = false;
#endif

			fl = SPF_EXPLICIT;
			if (!(fs_manifest->gamepath[i].flags&GAMEDIR_READONLY))
				fl |= SPF_WRITABLE;
			if (fs_manifest->gamepath[i].flags&GAMEDIR_PRIVATE)
				fl |= SPF_PRIVATE;
			if (fs_manifest->gamepath[i].flags&GAMEDIR_QSHACK)
				fl |= SPF_QSHACK;

			if (fs_manifest->gamepath[i].flags&GAMEDIR_USEBASEDIR)
			{	//for doom - loading packages without an actual gamedir. note that this does not imply that we can write anything.
				searchpathfuncs_t *handle = VFSOS_OpenPath(NULL, NULL, com_gamepath, com_gamepath, "");
				searchpath_t *search = (searchpath_t*)Z_Malloc (sizeof(searchpath_t));
				search->flags = 0;
				search->handle = handle;
				Q_strncpyz(search->purepath, "", sizeof(search->purepath));
				Q_strncpyz(search->logicalpath, com_gamepath, sizeof(search->logicalpath));

				FS_AddDataFiles(&oldpaths, search->purepath, search->logicalpath, search, fl, reloadflags);

				handle->ClosePath(handle);
				Z_Free(search);
			}
			else if (fs_manifest->gamepath[i].flags&GAMEDIR_STEAMGAME)
			{
				char steamdir[MAX_OSPATH];
				char *sl;
				dir += 6;
				sl = strchr(dir, '/');
				if (*sl)
				{
					if (Sys_SteamHasFile(steamdir, sizeof(steamdir), dir, ""))
						FS_AddSingleGameDirectory(&oldpaths, /*pure*/dir, steamdir, reloadflags, SPF_COPYPROTECTED|(fl&~SPF_WRITABLE));
				}
			}
			else
			{
				if (!FS_GamedirIsOkay(dir))
					continue;
				FS_AddGameDirectory(&oldpaths, dir, reloadflags, fl);
			}
		}
	}

	//now mark the depth values
	if (com_searchpaths)
		for (next = com_searchpaths->next; next; next = next->next)
			next->flags |= SPF_BASEPATH;
	com_base_searchpaths = com_searchpaths;

	for (i = 0; i < countof(fs_manifest->gamepath); i++)
	{
		char *dir = fs_manifest->gamepath[i].path;
		if (dir && !(fs_manifest->gamepath[i].flags&GAMEDIR_BASEGAME))
		{
			for (j = 0; j < countof(fs_manifest->gamepath); j++)
			{
				char *dir2 = fs_manifest->gamepath[j].path;
				if (dir2 && (fs_manifest->gamepath[i].flags&GAMEDIR_BASEGAME) && !strcmp(dir, dir2))
					break;
			}
			if (j < countof(fs_manifest->gamepath))
				continue;	//already loaded above. don't mess up gameonly_gamedir.

			fl = SPF_EXPLICIT;
			if (!(fs_manifest->gamepath[i].flags&GAMEDIR_READONLY))
				fl |= SPF_WRITABLE;
			if (fs_manifest->gamepath[i].flags&GAMEDIR_PRIVATE)
				fl |= SPF_PRIVATE;
			if (fs_manifest->gamepath[i].flags&GAMEDIR_QSHACK)
				fl |= SPF_QSHACK;

			if (*dir == '*')
			{	//just in case... shouldn't be needed.
				dir++;
				fl |= GAMEDIR_PRIVATE;
			}

			if (fs_manifest->gamepath[i].flags & GAMEDIR_SPECIAL)
				; //don't.
			else
			{
				//don't use evil gamedir names.
				if (!FS_GamedirIsOkay(dir))
					continue;
				FS_AddGameDirectory(&oldpaths, dir, reloadflags, fl);
			}
		}
	}

	FS_AddDownloadManifestPackages(&oldpaths, reloadflags);

	/*sv_pure: Reload pure paths*/
	if (fs_purenames && fs_purecrcs)
	{
		char crctok[64];
		char nametok[MAX_QPATH];
		char nametok2[MAX_QPATH];
		searchpath_t *sp, *lastpure = NULL;
		char *names = fs_purenames, *pname;
		char *crcs = fs_purecrcs;
		int crc;

		for (sp = com_searchpaths; sp; sp = sp->next)
		{
			if (sp->handle->GeneratePureCRC)
			{
				sp->nextpure = (void*)0x1;
				if (sp->crc_seed != fs_pureseed)
				{
					sp->crc_seed = fs_pureseed;
					sp->crc_reply = sp->handle->GeneratePureCRC(sp->handle, &sp->crc_seed);
				}
			}
			else
			{
				sp->nextpure = NULL;
				sp->crc_check = 0;
				sp->crc_reply = 0;
			}
		}

		while(names && crcs)
		{
			crcs = COM_ParseOut(crcs, crctok, sizeof(crctok));
			names = COM_ParseOut(names, nametok, sizeof(nametok));

			crc = strtoul(crctok, NULL, 0);
			if (!*crctok)
				continue;
			if (!strcmp(crctok, "-"))
				*crctok = 0;

			pname = nametok;

			if (fs_refnames && fs_refcrcs)
			{	//q3 is annoying as hell
				int crc2;
				char *rc = fs_refcrcs;
				char *rn = fs_refnames;
				pname = "";
				for (; rc && rn; )
				{
					rc = COM_ParseOut(rc, crctok, sizeof(crctok));
					rn = COM_ParseOut(rn, nametok2, sizeof(nametok2));
					crc2 = strtoul(crctok, NULL, 0);
					if (crc2 == crc)
					{
						COM_DefaultExtension(nametok2, ".pk3", sizeof(nametok2));
						pname = nametok2;
						break;
					}
				}
			}
			if (*pname == '*')	// * means that its 'referenced' (read: actually useful) thus should be downloaded, which is not relevent here.
				pname++;

			for (sp = com_searchpaths; sp; sp = sp->next)
			{
				if (sp->nextpure == (void*)0x1)	//don't add twice.
					if ((*crctok && sp->crc_check == crc) ||
						(!*crctok && !strcmp(COM_SkipPath(sp->purepath), COM_SkipPath(pname))))
					{
						if (fs_puremode)
						{
							if (lastpure)
								lastpure->nextpure = sp;
							else
								com_purepaths = sp;
							sp->nextpure = NULL;
							lastpure = sp;
						}
						break;
					}
			}
			if (!fs_puremode && !sp)
			{	//if we're not pure, we don't care if the version differs. don't load the server's version.
				//this works around 1.01 vs 1.06 issues.
				for (sp = com_searchpaths; sp; sp = sp->next)
				{
					if (!Q_strcasecmp(pname, sp->purepath))
						break;
				}
			}
			//if its not already loaded (via wildcards), load it from the download cache, if we can
			if (!sp && *pname)
			{
				char local[MAX_OSPATH];
				char rlocal[MAX_OSPATH];
				vfsfile_t *vfs;
				char ext[8];
				void *handle;
				int i;
				COM_FileExtension(pname, ext, sizeof(ext));

				if (FS_GenCachedPakName(pname, crctok, rlocal, sizeof(rlocal)) && FS_SystemPath(rlocal, FS_ROOT, local, sizeof(local)))
				{
					unsigned int keptflags;

					handle = FS_GetOldPath(&oldpaths, local, &keptflags);
					if (handle)
					{
						sp = FS_AddPathHandle(&oldpaths, pname, local, handle, "", SPF_COPYPROTECTED|SPF_UNTRUSTED|SPF_SERVER|keptflags, (unsigned int)-1);
						if (!sp)
							continue;	//some kind of error...
						if ((*crctok && sp->crc_check == crc) || !*crctok)
						{
							if (fs_puremode)
							{
								if (lastpure)
									lastpure->nextpure = sp;
								else
									com_purepaths = sp;
								sp->nextpure = NULL;
								lastpure = sp;
							}
						}
						//else crc mismatched...
						continue;
					}
					vfs = FS_OpenVFS(rlocal, "rb", FS_ROOT);
				}
				else
					vfs = NULL;
				if (vfs)
				{
					for (i = 0; i < sizeof(searchpathformats)/sizeof(searchpathformats[0]); i++)
					{
						if (!searchpathformats[i].extension || !searchpathformats[i].OpenNew)
							continue;
						if (!Q_strcasecmp(ext, searchpathformats[i].extension))
						{
							handle = searchpathformats[i].OpenNew (vfs, NULL, local, local, "");
							if (!handle)
								break;
							sp = FS_AddPathHandle(&oldpaths, pname, local, handle, "", SPF_COPYPROTECTED|SPF_UNTRUSTED|SPF_SERVER, (unsigned int)-1);

							if ((*crctok && sp->crc_check == crc) || !*crctok)
							{
								if (fs_puremode)
								{
									if (lastpure)
										lastpure->nextpure = sp;
									else
										com_purepaths = sp;
									sp->nextpure = NULL;
									lastpure = sp;
								}
							}
							break;
						}
					}
				}

				if (!sp)
					Con_DPrintf(CON_ERROR"Pure package %s:%08x wasn't found\n", pname, crc);
			}
		}
	}

	while(oldpaths)
	{
		fs_restarts++;

		next = oldpaths->next;

		Con_DPrintf("%s is no longer needed\n", oldpaths->logicalpath);
		oldpaths->handle->ClosePath(oldpaths->handle);
		Z_Free(oldpaths);
		oldpaths = next;
	}


	i = orderkey;
	orderkey = 0;
	next = NULL;
	if (com_purepaths)
		for (next = com_purepaths; next; next = next->nextpure)
			if (next->orderkey != ++orderkey)
				break;
	if (!next && fs_puremode < 2)
		for (next = com_purepaths; next; next = next->nextpure)
			if (next->orderkey != ++orderkey)
				break;

	if (next || i != orderkey)//some path changed. make sure the fs cache is flushed.
		FS_FlushFSHashReally(false);

#ifdef HAVE_CLIENT
	Shader_NeedReload(true);
#endif
//	Mod_ClearAll();
//	Cache_Flush();
}

void FS_UnloadPackFiles(void)
{
	if (Sys_LockMutex(fs_thread_mutex))
	{
		FS_ReloadPackFilesFlags(0);
		Sys_UnlockMutex(fs_thread_mutex);
	}
}

void FS_ReloadPackFiles(void)
{
	//extra junk is to ensure the palette is reloaded if that changed.
	flocation_t oldloc[countof(vidfilenames)];
	flocation_t newloc = {NULL};
	size_t i;

	if (!FS_GameIsInitialised())
	{
		ftemanifest_t *man = FS_ReadDefaultManifest(com_gamepath, 0, true);
		if (man)
		{
			FS_ChangeGame(man, true, false);
			return;
		}
	}

	for (i = 0; i < countof(vidfilenames); i++)
		FS_FLocateFile(vidfilenames[i], 0, &oldloc[i]);

	if (Sys_LockMutex(fs_thread_mutex))
	{
		FS_ReloadPackFilesFlags(~0);
		Sys_UnlockMutex(fs_thread_mutex);
	}

	for (i = 0; i < countof(vidfilenames); i++)
	{
		FS_FLocateFile(vidfilenames[i], 0, &newloc);
		if (oldloc[i].search != newloc.search)
		{	//okay, so this file changed... reload the video stuff
			Cbuf_AddText("vid_reload\n", RESTRICT_LOCAL);
			break;
		}
	}
}

static void FS_ReloadPackFiles_f(void)
{
	if (Sys_LockMutex(fs_thread_mutex))
	{
		if (*Cmd_Argv(1))
			FS_ReloadPackFilesFlags(atoi(Cmd_Argv(1)));
		else
			FS_ReloadPackFilesFlags(~0);
		Sys_UnlockMutex(fs_thread_mutex);
	}
	if (host_initialized)
		FS_BeginManifestUpdates();
}

#ifdef NOSTDIO
qboolean Sys_DoDirectoryPrompt(char *basepath, size_t basepathsize, const char *poshname, const char *savedname)
{
	return false;
}
qboolean Sys_FindGameData(const char *poshname, const char *gamename, char *basepath, int basepathlen, qboolean allowprompts)
{
	return false;
}
static qboolean Sys_SteamHasFile(char *basepath, int basepathlen, char *steamdir, char *fname)	//returns the base system path
{
	return false;
}
#elif defined(_WIN32) && !defined(FTE_SDL) && !defined(WINRT) && !defined(_XBOX)
#include "winquake.h"
#ifdef MINGW
#define byte BYTE	//some versions of mingw headers are broken slightly. this lets it compile.
#endif
static qboolean Sys_SteamHasFile(char *basepath, int basepathlen, char *steamdir, char *fname)
{
	/*
	Find where Valve's Steam distribution platform is installed.
	Then take a look at that location for the relevent installed app.
	*/
	FILE *f;
	DWORD resultlen;
	HKEY key = NULL;
	
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"SOFTWARE\\Valve\\Steam", 0, STANDARD_RIGHTS_READ|KEY_QUERY_VALUE, &key) == ERROR_SUCCESS)
	{
		wchar_t suckysucksuck[MAX_OSPATH];
		resultlen = sizeof(suckysucksuck);
		RegQueryValueExW(key, L"SteamPath", NULL, NULL, (void*)suckysucksuck, &resultlen);
		RegCloseKey(key);
		narrowen(basepath, basepathlen, suckysucksuck);
		Q_strncatz(basepath, va("/SteamApps/common/%s", steamdir), basepathlen);
		if ((f = fopen(va("%s/%s", basepath, fname), "rb")))
		{
			fclose(f);
			return true;
		}
	}
	return false;
}

#ifdef HAVE_CLIENT
static INT CALLBACK StupidBrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pData) 
{	//'stolen' from microsoft's knowledge base.
	//required to work around microsoft being annoying.
	wchar_t szDir[MAX_PATH];
	wchar_t *foo;
	switch(uMsg) 
	{
	case BFFM_INITIALIZED: 
		if (GetCurrentDirectoryW(sizeof(szDir)/sizeof(TCHAR), szDir))
		{
//			foo = strrchr(szDir, '\\');
//			if (foo)
//				*foo = 0;
//			foo = strrchr(szDir, '\\');
//			if (foo)
//				*foo = 0;
			SendMessageW(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)szDir);
		}
		break;
	case BFFM_VALIDATEFAILEDW:
		break;	//FIXME: validate that the gamedir contains what its meant to
	case BFFM_SELCHANGED: 
		if (SHGetPathFromIDListW((LPITEMIDLIST) lp, szDir))
		{
			wchar_t statustxt[MAX_OSPATH];
			while((foo = wcschr(szDir, '\\')))
				*foo = '/';
			if (pData)
				_snwprintf(statustxt, countof(statustxt), L"%s/%s", szDir, pData);
			else
				_snwprintf(statustxt, countof(statustxt), L"%s", szDir);
			statustxt[countof(statustxt)-1] = 0;	//ms really suck.
			SendMessageW(hwnd,BFFM_SETSTATUSTEXT,0,(LPARAM)statustxt);
		}
		break;
	}
	return 0;
}
int MessageBoxU(HWND hWnd, char *lpText, char *lpCaption, UINT uType);
#endif

qboolean Sys_DoDirectoryPrompt(char *basepath, size_t basepathsize, const char *poshname, const char *savedname)
{
#ifdef HAVE_CLIENT
	wchar_t resultpath[MAX_OSPATH];
	wchar_t title[MAX_OSPATH];
	BROWSEINFOW bi;
	LPITEMIDLIST il;
	memset(&bi, 0, sizeof(bi));
	bi.hwndOwner = mainwindow; //note that this is usually still null
	bi.pidlRoot = NULL;
	GetCurrentDirectoryW(sizeof(resultpath)-1, resultpath);
	bi.pszDisplayName = resultpath;

	widen(resultpath, sizeof(resultpath), poshname);
	_snwprintf(title, countof(title), L"Please locate your existing %s installation", resultpath);

	//force mouse to deactivate, so that we can actually see it.
	INS_UpdateGrabs(false, false);

	bi.lpszTitle = title;

	bi.ulFlags = BIF_RETURNONLYFSDIRS|BIF_STATUSTEXT;
	bi.lpfn = StupidBrowseCallbackProc;
	bi.lParam = 0;//(LPARAM)poshname;
	bi.iImage = 0;

	il = SHBrowseForFolderW(&bi);
	if (il)
	{
		SHGetPathFromIDListW(il, resultpath);
		CoTaskMemFree(il);
		narrowen(basepath, basepathsize, resultpath);
		if (savedname)
		{
			if (MessageBoxU(mainwindow, va("Would you like to save the location of %s as:\n%s", poshname, basepath), "Save Instaltion path", MB_YESNO|MB_DEFBUTTON2) == IDYES)
				MyRegSetValue(HKEY_CURRENT_USER, "SOFTWARE\\" FULLENGINENAME "\\GamePaths", savedname, REG_SZ, basepath, strlen(basepath));
		}
		return true;
	}
#endif
	return false;
}

DWORD GetFileAttributesU(const char * lpFileName)
{
	wchar_t wide[MAX_OSPATH];
	widen(wide, sizeof(wide), lpFileName);
	return GetFileAttributesW(wide);
}
qboolean Sys_FindGameData(const char *poshname, const char *gamename, char *basepath, int basepathlen, qboolean allowprompts)
{
#ifndef INVALID_FILE_ATTRIBUTES
	#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif

	//first, try and find it in our game paths location
	if (MyRegGetStringValue(HKEY_CURRENT_USER, "SOFTWARE\\" FULLENGINENAME "\\GamePaths", gamename, basepath, basepathlen))
	{
		if (GetFileAttributesU(basepath) != INVALID_FILE_ATTRIBUTES)
			return true;
	}


	if (!strcmp(gamename, "quake") || !strcmp(gamename, "afterquake") || !strcmp(gamename, "netquake") || !strcmp(gamename, "spasm") || !strcmp(gamename, "fitz") || !strcmp(gamename, "tenebrae"))
	{
		char *prefix[] =
		{
			"c:/quake/",						//quite a lot of people have it in c:\quake, as that's the default install location from the quake cd.
			"c:/games/quake/",					//personally I use this

			"c:/nquake/",						//nquake seems to have moved out of programfiles now. woo.
#ifdef _WIN64
			//quite a few people have nquake installed. FIXME: we need to an api function to read the directory for non-english-windows users.
			va("%s/nQuake/", getenv("%ProgramFiles(x86)%")),	//64bit builds should look in both places
			va("%s/nQuake/", getenv("%ProgramFiles%")),			//
#else
			va("%s/nQuake/", getenv("%ProgramFiles%")),			//32bit builds will get the x86 version anyway.
#endif
			NULL
		};
		int i;

		//try and find it via steam
		//reads HKEY_LOCAL_MACHINE\SOFTWARE\Valve\Steam\InstallPath
		//append SteamApps\common\quake
		//use it if we find winquake.exe there
		if (Sys_SteamHasFile(basepath, basepathlen, "quake", "Winquake.exe"))
			return true;
		//well, okay, so they don't have quake installed from steam.

		//check various 'unadvertised' paths
		for (i = 0; prefix[i]; i++)
		{
			char syspath[MAX_OSPATH];
			Q_snprintfz(syspath, sizeof(syspath), "%sid1/pak0.pak", prefix[i]);
			if (GetFileAttributesU(syspath) != INVALID_FILE_ATTRIBUTES)
			{
				Q_strncpyz(basepath, prefix[i], basepathlen);
				return true;
			}
			Q_snprintfz(syspath, sizeof(syspath), "%squake.exe", prefix[i]);
			if (GetFileAttributesU(syspath) != INVALID_FILE_ATTRIBUTES)
			{
				Q_strncpyz(basepath, prefix[i], basepathlen);
				return true;
			}
		}
	}

	if (!strcmp(gamename, "quake2"))
	{
		//look for HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\Quake2_exe\Path
		if (MyRegGetStringValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\Quake2_exe", "Path", basepath, basepathlen))
		{
			if (GetFileAttributesU(va("%s/quake2.exe", basepath)) != INVALID_FILE_ATTRIBUTES)
				return true;
		}

		if (Sys_SteamHasFile(basepath, basepathlen, "quake 2", "quake2.exe"))
			return true;
	}

	if (!strcmp(gamename, "et"))
	{
		//reads HKEY_LOCAL_MACHINE\SOFTWARE\Activision\Wolfenstein - Enemy Territory
		if (MyRegGetStringValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\Activision\\Wolfenstein - Enemy Territory", "InstallPath", basepath, basepathlen))
		{
//			if (GetFileAttributesU(va("%s/ET.exe", basepath) != INVALID_FILE_ATTRIBUTES)
//				return true;
			return true;
		}
	}

	if (!strcmp(gamename, "quake3"))
	{
		//reads HKEY_LOCAL_MACHINE\SOFTWARE\id\Quake III Arena\InstallPath
		if (MyRegGetStringValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\id\\Quake III Arena", "InstallPath", basepath, basepathlen))
		{
			if (GetFileAttributesU(va("%s/quake3.exe", basepath)) != INVALID_FILE_ATTRIBUTES)
				return true;
		}

		if (Sys_SteamHasFile(basepath, basepathlen, "quake 3 arena", "quake3.exe"))
			return true;
	}

	if (!strcmp(gamename, "wop"))
	{
		if (MyRegGetStringValue(HKEY_LOCAL_MACHINE, "SOFTWARE\\World Of Padman", "Path", basepath, basepathlen))
			return true;
	}

/*
	if (!strcmp(gamename, "d3"))
	{
		//reads HKEY_LOCAL_MACHINE\SOFTWARE\id\Doom 3\InstallPath
		if (MyRegGetStringValue(HKEY_LOCAL_MACHINE, L"SOFTWARE\\id\\Doom 3", "InstallPath", basepath, basepathlen))
			return true;
	}
*/

	if (!strcmp(gamename, "hexen2") || !strcmp(gamename, "h2mp"))
	{
		//append SteamApps\common\hexen 2
		if (Sys_SteamHasFile(basepath, basepathlen, "hexen 2", "glh2.exe"))
			return true;
	}

#if defined(HAVE_CLIENT) //this is *really* unfortunate, but doing this crashes the browser
	if (allowprompts && poshname && *gamename && !COM_CheckParm("-manifest"))
	{
		if (Sys_DoDirectoryPrompt(basepath, basepathlen, poshname, gamename))
			return true;
	}
#endif

	return false;
}
#else
#if defined(__linux__) || defined(__unix__) || defined(__apple__)
#include <sys/stat.h>

static qboolean Sys_SteamLibraryHasFile(char *basepath, int basepathlen, char *librarypath, char *steamdir, char *fname)	//returns the base system path
{
	Q_snprintfz(basepath, basepathlen, "%s/steamapps/common/%s", librarypath, steamdir);
	if (0==access(va("%s/%s", basepath, fname), R_OK))
		return true;
	return false;
}
static qboolean Sys_SteamParseLibraries(char *basepath, int basepathlen, char *libraryfile, char *steamdir, char *fname)	//returns the base system path
{
	qboolean success = false;
	char key[1024], *end;
	char value[1024];
	char *lib = libraryfile;
	int depth = 0;
	if (!libraryfile)
		return false;
	lib = COM_ParseCString(lib, key, sizeof(key), NULL);
	lib = COM_ParseCString(lib, value, sizeof(value), NULL);
	if (!strcmp(key, "libraryfolders") && !strcmp(value, "{"))
	{
		depth=1;
		while(lib && !success)
		{
			lib = COM_ParseCString(lib, key, sizeof(key), NULL);
			if (!strcmp(key, "}"))
			{
				if (!--depth)
					break;
				continue;
			}
			lib = COM_ParseCString(lib, value, sizeof(value), NULL);

			if (!strcmp(value, "{"))
				depth++;
			else if (depth == 1 && *key)
			{	//older format...
				strtoul(key, &end, 10);
				if (!*end)
				{
					//okay, its strictly base10
					if (Sys_SteamLibraryHasFile(basepath, basepathlen, value, steamdir,fname))
						success = true;
				}
			}
			else if (depth == 2 && !strcmp(key, "path"))
			{	//newer format...
				if (Sys_SteamLibraryHasFile(basepath, basepathlen, value, steamdir,fname))
					success = true;
			}
		}
	}
	FS_FreeFile(libraryfile);
	return success;
}
static qboolean Sys_SteamHasFile(char *basepath, int basepathlen, char *steamdir, char *fname)	//returns the base system path
{
	/*
	Find where Valve's Steam distribution platform is installed.
	Then take a look at that location for the relevent installed app.
	*/

	char *userhome = getenv("HOME");
	if (userhome && *userhome)
	{
		Q_snprintfz(basepath, basepathlen, "%s/.steam/steam/steamapps/libraryfolders.vdf", userhome);
		if (Sys_SteamParseLibraries(basepath, basepathlen, FS_MallocFile(basepath, FS_SYSTEM, NULL), steamdir, fname))
			return true;

		Q_snprintfz(basepath, basepathlen, "%s/.local/share/Steam/SteamApps/libraryfolders.vdf", userhome);
		if (Sys_SteamParseLibraries(basepath, basepathlen, FS_MallocFile(basepath, FS_SYSTEM, NULL), steamdir, fname))
			return true;
	}
	return false;
}
#else
static qboolean Sys_SteamHasFile(char *basepath, int basepathlen, char *steamdir, char *fname)	//returns the base system path
{
	return false;
}
#endif

qboolean Sys_DoDirectoryPrompt(char *basepath, size_t basepathsize, const char *poshname, const char *savedname)
{
	return false;
}
//#define Sys_DoDirectoryPrompt(bp,bps,game,savename) false

#if defined(__linux__) || defined(__unix__) || defined(__apple__)
static qboolean Sys_XDGHasDirectory(char *basepath, int basepathlen, const char *pregame, const char *gamename, const char *postgame)
{	//returns true if the gamedir can be found, and fills in the basepath.
	struct stat sb;
	const char *dirs = getenv("XDG_DATA_DIRS"), *s;
	char dir[MAX_OSPATH];
	if (!dirs||!*dirs)
		dirs = "/usr/local/share:/usr/share";

	while (dirs && *dirs)
	{
		dirs = COM_ParseStringSetSep(dirs, ':', dir, sizeof(dir));

		s = va("%s/%s%s%s/", dir, pregame, gamename, postgame);
		if (stat(s, &sb) == 0)
		{
			if (S_ISDIR(sb.st_mode))
			{
				Q_strncpyz(basepath, s, basepathlen);
				return true;
			}
		}
	}
	return false;
}
#endif

//FIXME: replace with a callback version, for multiple results.
qboolean Sys_FindGameData(const char *poshname, const char *gamename, char *basepath, int basepathlen, qboolean allowprompts)
{
#if defined(__linux__) || defined(__unix__) || defined(__apple__)
	if (!*gamename)
		gamename = "quake";	//just a paranoia fallback, shouldn't be needed.
	if (!strcmp(gamename, "quake_rerel"))
		if (Sys_SteamHasFile(basepath, basepathlen, "Quake/rerelease", "id1/pak0.pak"))
			return true;
	if (!strcmp(gamename, "quake") || !strcmp(gamename, "afterquake") || !strcmp(gamename, "netquake") || !strcmp(gamename, "spasm") || !strcmp(gamename, "fitz") || !strcmp(gamename, "tenebrae"))
	{
		if (Sys_SteamHasFile(basepath, basepathlen, "Quake", "Id1/PAK0.PAK"))	//dos legacies need to die.
			return true;
		if (Sys_SteamHasFile(basepath, basepathlen, "Quake", "id1/PAK0.PAK"))	//dos legacies need to die.
			return true;
		if (Sys_SteamHasFile(basepath, basepathlen, "Quake", "id1/pak0.pak"))	//people may have tried to sanitise it already.
			return true;

		if (Sys_XDGHasDirectory(basepath, basepathlen, "", gamename, ""))
			return true;
	}
	else if (!strcmp(gamename, "quake2"))
	{
		if (Sys_SteamHasFile(basepath, basepathlen, "quake 2", "baseq2/pak0.pak"))
				return true;
	}
	else if (!strcmp(gamename, "hexen2") || !strcmp(gamename, "h2mp") || !strcmp(gamename, "portals"))
	{
		if (Sys_SteamHasFile(basepath, basepathlen, "hexen 2", "data/pak0.pak"))
			return true;
		gamename = "hexen2";
	}

	if (Sys_XDGHasDirectory(basepath, basepathlen, "games/", gamename, ""))
		return true;
	if (Sys_XDGHasDirectory(basepath, basepathlen, "", gamename, ""))
		return true;
	if (Sys_XDGHasDirectory(basepath, basepathlen, "games/", gamename, "-demo"))
		return true;
	if (Sys_XDGHasDirectory(basepath, basepathlen, "", gamename, "-demo"))
		return true;

#if defined(HAVE_CLIENT) //this is *really* unfortunate, but doing this crashes the browser
	if (allowprompts && poshname && *gamename && !COM_CheckParm("-manifest"))
	{
		if (Sys_DoDirectoryPrompt(basepath, basepathlen, poshname, gamename))
			return true;
	}
#endif

#endif
	return false;
}
#endif

static void FS_FreePaths(void)
{
	searchpath_t *next;
	FS_FlushFSHashReally(true);

	//
	// free up any current game dir info
	//
	while (com_searchpaths)
	{
		com_searchpaths->handle->ClosePath(com_searchpaths->handle);
		next = com_searchpaths->next;
		Z_Free (com_searchpaths);
		com_searchpaths = next;
	}

	com_fschanged = true;


	if (filesystemhash.numbuckets)
	{
		BZ_Free(filesystemhash.bucket);
		filesystemhash.bucket = NULL;
		filesystemhash.numbuckets = 0;
	}

	FS_Manifest_Free(fs_manifest);
	fs_manifest = NULL;
}
void FS_Shutdown(void)
{
	if (!fs_thread_mutex)
		return;

	Mods_FlushModList();

#ifdef PACKAGEMANAGER
	PM_ManifestChanged(NULL);
#endif
	FS_FreePaths();
	Sys_DestroyMutex(fs_thread_mutex);
	fs_thread_mutex = NULL;

	Cvar_SetEngineDefault(&fs_gamename, NULL);
	Cvar_SetEngineDefault(&com_protocolname, NULL);
}

//returns false if the directory is not suitable.
//returns true if it contains a known package. if we don't actually know of any packages that it should have, we just have to assume that its okay.
qboolean FS_DirHasAPackage(char *basedir, ftemanifest_t *man)
{
	qboolean defaultret = true;
	int j;
	vfsfile_t *f;

	f = VFSOS_Open(va("%sdefault.fmf", basedir), "rb");
	if (f)
	{
		VFS_CLOSE(f);
		return true;
	}

	for (j = 0; j < sizeof(fs_manifest->package) / sizeof(fs_manifest->package[0]); j++)
	{
		if (!man->package[j].path)
			continue;
		defaultret = false;

		f = VFSOS_Open(va("%s%s", basedir, man->package[j].path), "rb");
		if (f)
		{
			VFS_CLOSE(f);
			return true;
		}
	}
	return defaultret;
}

//false stops the search (and returns that value to FS_DirHasGame)
static int QDECL FS_DirDoesHaveGame(const char *fname, qofs_t fsize, time_t modtime, void *ctx, searchpathfuncs_t *subdir)
{
	return false;
}

//just check each possible file, see if one is there.
static qboolean FS_DirHasGame(const char *basedir, int gameidx)
{
	int j;
#if defined(_WIN32) || defined(NOSTDIO) || !defined(_POSIX_C_SOURCE)
#else
	char realpath[MAX_OSPATH];
#endif
	char cached[MAX_OSPATH];

	//none listed, just assume its correct.
	if (!gamemode_info[gameidx].auniquefile[0])
		return true;

	for (j = 0; j < 4; j++)
	{
		if (!gamemode_info[gameidx].auniquefile[j])
			continue;	//no more
#if defined(_WIN32) || defined(NOSTDIO) || !defined(_POSIX_C_SOURCE)	//systems that lack a working 'access' function.
		if (!Sys_EnumerateFiles(basedir, gamemode_info[gameidx].auniquefile[j], FS_DirDoesHaveGame, NULL, NULL))
			return true;	//search was cancelled by the callback, so it actually got called.
#else
		if (FS_FixupFileCase(realpath, sizeof(realpath), basedir, gamemode_info[gameidx].auniquefile[j], false) && access(realpath, R_OK) == 0)
			return true;	//something readable.
#endif

		//gah, just try a wildcard for hashes.
		if (FS_GenCachedPakName(gamemode_info[gameidx].auniquefile[j], NULL, cached,sizeof(cached)))
		{
			Q_strncatz(cached, ".*", sizeof(cached));
			if (!Sys_EnumerateFiles(basedir, cached, FS_DirDoesHaveGame, NULL, NULL))
				return true;	//search was cancelled by the callback, so it actually got called.
		}
	}
	return false;
}

//check em all
static int FS_IdentifyDefaultGameFromDir(const char *basedir)
{
	int i;
	for (i = 0; gamemode_info[i].argname; i++)
	{
		if (FS_DirHasGame(basedir, i))
			return i;
	}
	return -1;
}

//attempt to work out which game we're meant to be trying to run based upon a few things
//1: fs_changegame console command override. fixme: needs to cope with manifests too.
//2: -quake3 (etc) argument implies that the user wants to run quake3.
//3: otherwise if we are ftequake3.exe then we try to run quake3.
//4: identify characteristic files within the working directory (like id1/pak0.pak implies we're running quake)
//5: check where the exe actually is instead of simply where we're being run from.
//6: try the homedir, just in case.
//7: fallback to prompting. just returns -1 here.
//if autobasedir is not set, block gamedir changes/prompts.
static int FS_IdentifyDefaultGame(char *newbase, int sizeof_newbase, qboolean fixedbase)
{
	int i;
	int gamenum = -1;

	//use the game based on an exe name over the filesystem one (could easily have multiple fs path matches).
	if (gamenum == -1)
	{
		char *ev, *v0 = COM_SkipPath(com_argv[0]);
		for (i = 0; gamemode_info[i].argname; i++)
		{
			if (!gamemode_info[i].exename)
				continue;
			ev = strstr(v0, gamemode_info[i].exename);
			if (ev && (!strchr(ev, '\\') && !strchr(ev, '/')))
				gamenum = i;
		}
	}

	//identify the game from a telling file in the working directory
	if (gamenum == -1)
		gamenum = FS_IdentifyDefaultGameFromDir(newbase);
	//identify the game from a telling file relative to the exe's directory. for when shortcuts don't set the working dir sensibly.
	if (gamenum == -1 && host_parms.binarydir && *host_parms.binarydir && !fixedbase)
	{
		gamenum = FS_IdentifyDefaultGameFromDir(host_parms.binarydir);
		if (gamenum != -1)
			Q_strncpyz(newbase, host_parms.binarydir, sizeof_newbase);
	}
	if (gamenum == -1 && *com_homepath && com_homepathusable && !fixedbase)
	{
		gamenum = FS_IdentifyDefaultGameFromDir(com_homepath);
		if (gamenum != -1)
			Q_strncpyz(newbase, com_homepath, sizeof_newbase);
	}
	return gamenum;
}

static ftemanifest_t *FS_GenerateLegacyManifest(int game, const char *basedir)
{
	ftemanifest_t *man;
	const char *cexec;

	if (basedir)
	{	//see if the gamedir we're aiming for already has a default.fmf file...
		man = NULL;
		if (!man)
			man = FS_Manifest_ReadSystem(va("%s%s.fmf", basedir, gamemode_info[game].exename), basedir);
#ifdef BRANDING_NAME
		if (!man)
			man = FS_Manifest_ReadSystem(va("%s"STRINGIFY(BRANDING_NAME)".fmf", basedir), basedir);
#endif
		if (!man)
			man = FS_Manifest_ReadSystem(va("%sdefault.fmf", basedir), basedir);

		if (man)
		{
			if (!Q_strcasecmp(man->installation, gamemode_info[game].argname+1))
				return man;	//this seems to match what we were expecting. use its data instead of making one up.
			else
				FS_Manifest_Free(man);
		}
	}

	if (gamemode_info[game].manifestfile)
		man = FS_Manifest_ReadMem(NULL, basedir, gamemode_info[game].manifestfile);
	else
	{
		man = FS_Manifest_Create(NULL, basedir);

		Cmd_TokenizeString(va("game \"%s\"", gamemode_info[game].argname+1), false, false);
		FS_Manifest_ParseTokens(man);

		for (cexec = gamemode_info[game].customexec; cexec && cexec[0] == '/' && cexec[1] == '/'; )
		{
			char line[256];
			char *e = strchr(cexec, '\n');
			if (!e)
				break;
			Q_strncpyz(line, cexec+2, min(e-(cexec+2)+1, sizeof(line)));
			cexec = e+1;
			Cmd_TokenizeString(line, false, false);
			FS_Manifest_ParseTokens(man);
		}

		FS_Manifest_SetDefaultSettings(man, &gamemode_info[game]);
	}
	man->security = MANIFEST_SECURITY_INSTALLER;
	return man;
}

static void FS_AppendManifestGameArguments(ftemanifest_t *man)
{
	int i;

	if (!man)
		return;

	i = COM_CheckParm ("-basegame");
	if (i)
	{
		if (man->filename)
			Z_Free(man->filename);
		man->filename = NULL;
		do
		{
			Cmd_TokenizeString(va("basegame \"%s\"", com_argv[i+1]), false, false);
			FS_Manifest_ParseTokens(man);

			i = COM_CheckNextParm ("-basegame", i);
		}
		while (i && i < com_argc-1);
	}

	i = COM_CheckParm ("-game");
	if (i)
	{
		if (man->filename)
			Z_Free(man->filename);
		man->filename = NULL;
		do
		{
			Cmd_TokenizeString(va("gamedir \"%s\"", com_argv[i+1]), false, false);
			FS_Manifest_ParseTokens(man);

			i = COM_CheckNextParm ("-game", i);
		}
		while (i && i < com_argc-1);
	}

	i = COM_CheckParm ("+gamedir");
	if (i)
	{
		if (man->filename)
			Z_Free(man->filename);
		man->filename = NULL;
		do
		{
			Cmd_TokenizeString(va("gamedir \"%s\"", com_argv[i+1]), false, false);
			FS_Manifest_ParseTokens(man);

			i = COM_CheckNextParm ("+gamedir", i);
		}
		while (i && i < com_argc-1);
	}
}

#ifdef MANIFESTDOWNLOADS
static struct dl_download *curpackagedownload;
qboolean FS_DownloadingPackage(void)
{
	if (PM_IsApplying() & 3)
		return true;
	return !fs_manifest || !!curpackagedownload;
}

static void FS_ManifestUpdated(struct dl_download *dl)
{
	ftemanifest_t *man = fs_manifest;

	curpackagedownload = NULL;
	waitingformanifest--;

	if (dl->file)
	{
		if (dl->user_ctx == man)
		{
			size_t len = VFS_GETLEN(dl->file), len2;
			char *fdata = BZ_Malloc(len+1), *fdata2 = NULL;
			if (fdata)
			{
				VFS_READ(dl->file, fdata, len);
				fdata[len] = 0;
				man = FS_Manifest_ReadMem(fs_manifest->filename, fs_manifest->basedir, fdata);
				if (man)
				{
					//the updateurl MUST match the current one in order for the local version of the manifest to be saved (to avoid extra updates, and so it appears in the menu_mods)
					//this is a paranoia measure to avoid too much damage from buggy/malicious proxies that return empty pages or whatever.
					if (!man->updateurl || !fs_manifest->updateurl || strcmp(man->updateurl, fs_manifest->updateurl))
					{
						Con_Printf("Refusing to update manifest - updateurl changed from \"%s\" to \"%s\"\n", fs_manifest->updateurl, man->updateurl);
						FS_Manifest_Free(man);
					}
					else if (!man->basedir || !fs_manifest->basedir || strcmp(man->basedir, fs_manifest->basedir))	//basedir must match too... ie: not be overridden.
					{
						Con_Printf("Refusing to update manifest - basedir changed from \"%s\" to \"%s\"\n", fs_manifest->basedir, man->basedir);
						FS_Manifest_Free(man);
					}
					else if (!man->installation || !fs_manifest->installation || strcmp(man->installation, fs_manifest->installation))
					{
						Con_Printf("Refusing to update manifest - game changed from \"%s\" to \"%s\"\n", fs_manifest->installation, man->installation);
						FS_Manifest_Free(man);
					}
					else
					{
						man->blockupdate = true;	//don't download it multiple times. that's just crazy.
						if (man->filename)
						{
							vfsfile_t *f2 = FS_OpenVFS(fs_manifest->filename, "rb", FS_SYSTEM);
							if (f2)
							{
								len2 = VFS_GETLEN(f2);
								if (len != len2)
								{
									fdata2 = NULL;
									len2 = 0;
								}
								else
								{
									fdata2 = BZ_Malloc(len2);
									VFS_READ(f2, fdata2, len2);
								}
								VFS_CLOSE(f2);
								if (len == len2 && !memcmp(fdata, fdata2, len))
								{
									//files match, no need to use this new manifest at all.
									FS_Manifest_Free(man);
									man = NULL;
								}
								BZ_Free(fdata2);
							}
							if (man)
								FS_WriteFile(man->filename, fdata, len, FS_SYSTEM);
						}
						if (man)
							FS_ChangeGame(man, true, false);
					}
				}
				BZ_Free(fdata);
			}
		}

		VFS_CLOSE(dl->file);
		dl->file = NULL;
	}
	if (!waitingformanifest)
		PM_AddManifestPackages(man, true);
}
static void FS_BeginNextPackageDownload(ftemanifest_t *man)
{
	if (curpackagedownload || !man || com_installer)
		return;

	if (man == fs_manifest && man->updateurl && !man->blockupdate)
	{
		vfsfile_t *f = man->filename?FS_OpenVFS(man->filename, "ab", FS_SYSTEM):NULL;	//this is JUST to make sure its writable. don't bother updating it if it isn't.
		man->blockupdate = true;
		if (f)
		{
			VFS_CLOSE(f);

			Con_Printf("Updating manifest from %s\n", man->updateurl);
			waitingformanifest++;
			curpackagedownload = HTTP_CL_Get(man->updateurl, NULL, FS_ManifestUpdated);
			if (curpackagedownload)
			{
				curpackagedownload->user_ctx = man;
				return;
			}
		}
	}
}
void FS_BeginManifestUpdates(void)
{
	ftemanifest_t *man = fs_manifest;
	if (curpackagedownload || !man)
		return;

	if (!curpackagedownload)
		FS_BeginNextPackageDownload(man);
}
#else
qboolean FS_DownloadingPackage(void)
{
	return false;
}
void FS_BeginManifestUpdates(void)
{
}
#endif

static qboolean FS_FoundManifest(void *usr, ftemanifest_t *man, enum modsourcetype_e sourcetype)
{
	if (!*(ftemanifest_t**)usr)
		*(ftemanifest_t**)usr = man;
	else
		FS_Manifest_Free(man);
	return true;
}

//reads the default manifest based upon the basedir, the commandline arguments, the name of the exe, etc.
//may still fail if no game was identified.
//if fixedbasedir is true, stuff like -quake won't override/change the active basedir (ie: -basedir or gamedir switching without breaking gamedir)
static ftemanifest_t *FS_ReadDefaultManifest(char *newbasedir, size_t newbasedirsize, qboolean fixedbasedir)
{
	int i;
	int game = -1;
	ftemanifest_t *man = NULL;

	vfsfile_t *f;

	//commandline generally takes precedence
	if (!man && game == -1)
	{
		int i;
		for (i = 0; gamemode_info[i].argname; i++)
		{
			if (COM_CheckParm(gamemode_info[i].argname))
			{
				game = i;
				break;
			}
		}
	}

	//hopefully this will be used for TCs.
	if (!man && game == -1)
	{
		char exename[MAX_QPATH];
		COM_StripAllExtensions(COM_SkipPath(com_argv[0]), exename, sizeof(exename));
		//take away any amd64/x86/x86_64 postfix, so that people can have multiple cpu arch binaries sharing a single fmf
		if (strlen(exename) > strlen(ARCH_CPU_POSTFIX) && !strcmp(exename+strlen(exename)-strlen(ARCH_CPU_POSTFIX), ARCH_CPU_POSTFIX))
			exename[strlen(exename)-strlen(ARCH_CPU_POSTFIX)] = 0;
		//and then the trailing _ (before said postfix)
		if (exename[strlen(exename)] == '_')
			exename[strlen(exename)] = 0;
		//and hopefully we now have something consistent that we can try to use.

		if (!man)
			man = FS_Manifest_ReadSystem(va("%s%s.fmf", newbasedir, exename), newbasedir);
#ifdef BRANDING_NAME
		if (!man)
			man = FS_Manifest_ReadSystem(va("%s"STRINGIFY(BRANDING_NAME)".fmf", newbasedir), newbasedir);
#endif
		if (!man)
			man = FS_Manifest_ReadSystem(va("%sdefault.fmf", newbasedir), newbasedir);
		if (man)
			man->security = MANIFEST_SECURITY_DEFAULT;
	}

#if defined(ENGINE_HAS_ZIP) && defined(PACKAGE_PK3)
	if (!man && game == -1)
	{
		searchpathfuncs_t *pak;
		vfsfile_t *vfs;
		vfs = VFSOS_Open(com_argv[0], "rb");
		pak = FSZIP_LoadArchive(vfs, NULL, com_argv[0], com_argv[0], "");
		if (pak)
		{
			flocation_t loc;
			if (pak->FindFile(pak, &loc, "default.fmf", NULL))
			{
				f = pak->OpenVFS(pak, &loc, "rb");
				if (f)
				{
					size_t len = VFS_GETLEN(f);
					char *fdata = BZ_Malloc(len+1);
					if (fdata)
					{
						VFS_READ(f, fdata, len);
						fdata[len] = 0;
						man = FS_Manifest_ReadMem(NULL, NULL, fdata);
						if (man)
							man->security = MANIFEST_SECURITY_DEFAULT;
						BZ_Free(fdata);
					}
					VFS_CLOSE(f);
				}
			}
			pak->ClosePath(pak);
		}
	}
#endif


	//-basepack is primarily an android feature
	i = COM_CheckParm ("-basepack");
	while (!man && game == -1 && i && i < com_argc-1)
	{
		const char *pakname = com_argv[i+1];
		searchpathfuncs_t *pak;
		vfsfile_t *vfs = VFSOS_Open(pakname, "rb");
		pak = FS_OpenPackByExtension(vfs, NULL, pakname, pakname, "");
		if (pak)
		{
			flocation_t loc;
			if (pak->FindFile(pak, &loc, "default.fmf", NULL))
			{
				f = pak->OpenVFS(pak, &loc, "rb");
				if (f)
				{
					size_t len = VFS_GETLEN(f);
					char *fdata = BZ_Malloc(len+1);
					if (fdata)
					{
						VFS_READ(f, fdata, len);
						fdata[len] = 0;
						man = FS_Manifest_ReadMem(NULL, NULL, fdata);
						if (man)
							man->security = MANIFEST_SECURITY_DEFAULT;
						BZ_Free(fdata);
					}
					VFS_CLOSE(f);
				}
			}
			pak->ClosePath(pak);
		}
		i = COM_CheckNextParm ("-basepack", i);
	}


	if (!man && game == -1 && host_parms.manifest)
	{
		man = FS_Manifest_ReadMem(NULL, newbasedir, host_parms.manifest);
		if (man)
			man->security = MANIFEST_SECURITY_INSTALLER;
	}

	if (!man)
	{
		if (game == -1)
			game = FS_IdentifyDefaultGame(newbasedir, newbasedirsize, fixedbasedir);
		if (game != -1)
			man = FS_GenerateLegacyManifest(game, fixedbasedir?newbasedir:NULL);
	}

	FS_AppendManifestGameArguments(man);
	return man;
}

qboolean FS_FixPath(char *path, size_t pathsize)
{
	size_t len = strlen(path);
	if (len)
	{
		if (path[len-1] == '/')
			return true;
#ifdef _WIN32
		if (path[len-1] == '\\')
			return true;
#endif
		if (len >= pathsize-1)
			return false;
		path[len] = '/';
		path[len+1] = 0;
	}
	return true;
}

//this is potentially unsafe. needs lots of testing.
qboolean FS_ChangeGame(ftemanifest_t *man, qboolean allowreloadconfigs, qboolean allowbasedirchange)
{
	int i, j;
	char realpath[MAX_OSPATH-1];
	char newbasedir[MAX_OSPATH];
#ifdef PACKAGEMANAGER
	char *olddownloadsurl;
#endif
	qboolean fixedbasedir;
	qboolean reloadconfigs = false;
	qboolean builtingame = false;
	qboolean basedirchanged = false;
	flocation_t loc;
#ifdef MANIFESTDOWNLOADS
	qboolean allowdownload = man!=fs_manifest && allowreloadconfigs;	//or at least ask the user.
#endif

#ifdef HAVE_CLIENT
	qboolean allowvidrestart = true;
	char *vidfile[] = {"gfx.wad", "gfx/conback.lmp"/*q1*/,"gfx/menu/conback.lmp"/*h2*/,"pics/conback.pcx"/*q2*/,	//misc stuff
		"gfx/palette.lmp", "pics/colormap.pcx", "gfx/conchars.png"};		//palettes
	searchpathfuncs_t *vidpath[countof(vidfile)];
	char *menufile[] = {"menu.dat"/*mods*/, "gfx/ttl_main.lmp"/*q1*/, "pics/m_main_quit.pcx"/*q2*/, "gfx/menu/title0.lmp"/*h2*/};
	searchpathfuncs_t *menupath[countof(menufile)];
#endif

	//if any of these files change location, the configs will be re-execed.
	//note that we reuse path handles if they're still valid, so we can just check the pointer to see if it got unloaded/replaced.
	char *conffile[] = {"quake.rc", "hexen.rc", "default.cfg", "server.cfg"};
	searchpathfuncs_t *confpath[countof(conffile)];

#ifdef HAVE_CLIENT
	for (i = 0; i < countof(vidfile); i++)
	{
		if (allowvidrestart)
		{
			FS_FLocateFile(vidfile[i], FSLF_IFFOUND, &loc);	//q1
			vidpath[i] = loc.search?loc.search->handle:NULL;
		}
		else
			vidpath[i] = NULL;
	}
	for (i = 0; i < countof(menufile); i++)
	{
		if (allowreloadconfigs)
		{
			FS_FLocateFile(menufile[i], FSLF_IFFOUND|FSLF_SECUREONLY, &loc);
			menupath[i] = loc.search?loc.search->handle:NULL;
		}
		else
			menupath[i] = NULL;
	}
#endif

	if (allowreloadconfigs && fs_noreexec.ival)
		allowreloadconfigs = false;
	for (i = 0; i < countof(conffile); i++)
	{
		if (allowreloadconfigs)
		{
			FS_FLocateFile(conffile[i], FSLF_IFFOUND|FSLF_IGNOREPURE, &loc);	//q1
			confpath[i] = loc.search?loc.search->handle:NULL;
		}
		else
			confpath[i] = NULL;
	}

#if defined(FTE_TARGET_WEB) || defined(ANDROID) || defined(WINRT)
	//these targets are considered to be sandboxed already, and have their own app-based base directory which they will always use.
	Q_strncpyz (newbasedir, host_parms.basedir, sizeof(newbasedir));
	fixedbasedir = true;
#else
	i = COM_CheckParm ("-basedir");
	fixedbasedir = i && i < com_argc-1;
	Q_strncpyz (newbasedir, fixedbasedir?com_argv[i+1]:host_parms.basedir, sizeof(newbasedir));
#endif

	//make sure it has a trailing slash, or is empty. woo.
	FS_CleanDir(newbasedir, sizeof(newbasedir));

	if (!allowreloadconfigs || !allowbasedirchange || (man && fs_manifest && !Q_strcasecmp(man->installation, fs_manifest->installation)))
	{
		fixedbasedir = true;
		Q_strncpyz (newbasedir, com_gamepath, sizeof(newbasedir));
	}

	if (!man)
	{
		int found = 0;
		//if we're already running a game, don't autodetect.
		if (fs_manifest)
			return false;

		man = FS_ReadDefaultManifest(newbasedir, sizeof(newbasedir), fixedbasedir);

		if (!man)
		{
			found = FS_EnumerateKnownGames(FS_FoundManifest, &man, fixedbasedir);
			if (found != 1)
			{
				//we found more than 1 (or none)
				//if we're a client, display a menu to pick between them (or display an error)
				//servers can just use the first they find, they'd effectively just crash otherwise, but still give a warning.
				if (!isDedicated)
				{
					FS_Manifest_Free(man);
					man = NULL;
				}
				else if (found)
					Con_Printf(CON_WARNING "Warning: found multiple possible games. Using the first found (%s).\n", man->formalname);
				else
					Con_Printf(CON_ERROR "Error: unable to determine correct game/basedir.\n");
			}
		}
		if (!man)
		{
#ifdef _WIN32
			//quit straight out on windows. this prevents shitty sandboxed malware scanners from seeing bugs in opengl drivers and blaming us for it.
			if (!fixedbasedir && found == 0)
				Sys_Error("No recognised game data found in working directory:\n%s", com_gamepath);
#endif
			man = FS_Manifest_ReadMem(NULL, NULL,
				"FTEManifestVer 1\n"
				"game \"\"\n"
				"name \"" FULLENGINENAME "\"\n"
				"-set vid_fullscreen 0\n"
				"-set gl_font cour\n"
				"-set vid_width 640\n"
				"-set vid_height 480\n"
				);
		}
		if (!man)
			Sys_Error("couldn't generate dataless manifest\n");
	}

#ifdef PACKAGEMANAGER
	if (fs_manifest && fs_manifest->downloadsurl)
		olddownloadsurl = Z_StrDup(fs_manifest->downloadsurl);
	else if (!fs_manifest && man->downloadsurl)
		olddownloadsurl = Z_StrDup(man->downloadsurl);
	else
		olddownloadsurl = NULL;
#endif

	if (man->installation && *man->installation)
	{
		for (i = 0; gamemode_info[i].argname; i++)
		{
			if (!strcmp(man->installation, gamemode_info[i].argname+1))
			{
				//if there's no base dirs, edit the manifest to give it its default ones.
				for (j = 0; j < sizeof(man->gamepath) / sizeof(man->gamepath[0]); j++)
				{
					if (man->gamepath[j].path && (man->gamepath[j].flags&GAMEDIR_BASEGAME))
						break;
				}
				if (j == sizeof(man->gamepath) / sizeof(man->gamepath[0]))
				{
					for (j = 0; j < 4; j++)
						if (gamemode_info[i].dir[j])
						{
							Cmd_TokenizeString(va("basegame \"%s\"", gamemode_info[i].dir[j]), false, false);
							FS_Manifest_ParseTokens(man);
						}
				}

				if (!man->schemes)
				{
					Cmd_TokenizeString(va("schemes \"%s\"", gamemode_info[i].argname+1), false, false);
					FS_Manifest_ParseTokens(man);
				}

#ifdef PACKAGEMANAGER
				if (!man->downloadsurl && gamemode_info[i].downloadsurl)
				{
#ifndef FTE_TARGET_WEB
					if (*gamemode_info[i].downloadsurl == '/')
					{
						conchar_t musite[256], *e;
						char site[256];
						char *oldprefix = "http://fte.";
						char *newprefix = "https://updates.";
						e = COM_ParseFunString(CON_WHITEMASK, ENGINEWEBSITE, musite, sizeof(musite), false);
						COM_DeFunString(musite, e, site, sizeof(site)-1, true, true);
						if (!strncmp(site, oldprefix, strlen(oldprefix)))
						{
							memmove(site+strlen(newprefix), site+strlen(oldprefix), strlen(site)-strlen(oldprefix)+1);
							memcpy(site, newprefix, strlen(newprefix));
						}
						Cmd_TokenizeString(va("downloadsurl \"%s%s\"", site, gamemode_info[i].downloadsurl), false, false);
					}
					else
#endif
						Cmd_TokenizeString(va("downloadsurl \"%s\"", gamemode_info[i].downloadsurl), false, false);
					FS_Manifest_ParseTokens(man);
				}
				if (!man->installupd && gamemode_info[i].needpackages)
				{
					Cmd_TokenizeString(va("install \"%s\"", gamemode_info[i].needpackages), false, false);
					FS_Manifest_ParseTokens(man);
				}
#endif

				if (!man->protocolname)
				{
					Cmd_TokenizeString(va("protocolname \"%s\"", gamemode_info[i].protocolname), false, false);
					FS_Manifest_ParseTokens(man);
				}
				if (!man->defaultexec && gamemode_info[i].customexec)
				{
					const char *e = gamemode_info[i].customexec;
					while (e[0] == '/' && e[1] == '/')
					{
						e+=2;
						while(*e)
						{
							if (*e++ == '\n')
								break;
						}
					}
					man->defaultexec = Z_StrDup(e);
				}

				builtingame = true;
				if (!fixedbasedir)
				{
					if (man->basedir)
						Q_strncpyz (newbasedir, man->basedir, sizeof(newbasedir));
					else if (!FS_DirHasGame(newbasedir, i))
						if (Sys_FindGameData(man->formalname, man->installation, realpath, sizeof(realpath), man->security != MANIFEST_SECURITY_INSTALLER) && FS_FixPath(realpath, sizeof(realpath)) && FS_DirHasGame(realpath, i))
							Q_strncpyz (newbasedir, realpath, sizeof(newbasedir));
				}
				break;
			}
		}
	}

	if (!fixedbasedir)
	{
		if (!builtingame && !fixedbasedir && !FS_DirHasAPackage(newbasedir, man))
		{
			if (Sys_FindGameData(man->formalname, man->installation, realpath, sizeof(realpath), man->security != MANIFEST_SECURITY_INSTALLER) && FS_FixPath(realpath, sizeof(realpath)) && FS_DirHasAPackage(realpath, man))
				Q_strncpyz (newbasedir, realpath, sizeof(newbasedir));
			else if (man->basedir)
				Q_strncpyz (newbasedir, man->basedir, sizeof(newbasedir));
#if !defined(NOBUILTINMENUS) && defined(HAVE_CLIENT)
			else if (man != fs_manifest)
			{
#ifdef PACKAGEMANAGER
				Z_Free(olddownloadsurl);
#endif
				M_Menu_BasedirPrompt(man);
				return false;
			}
#endif
#ifdef HAVE_CLIENT
			else
			{	//no basedir known... switch to installer mode and ask the user where they want it (at least on windows)
				Z_Free(man->filename);
				man->filename = NULL;
				com_installer = true;
			}
#endif
		}
	}
	if (!fixedbasedir && !com_installer)
	{
		if (strcmp(com_gamepath, newbasedir))
		{
			Q_strncpyz (com_gamepath, newbasedir, sizeof(com_gamepath));
			basedirchanged = true;
		}
	}

	//now that we know what we're running and where we're running it, we can switch to it.
	if (man == fs_manifest)
	{
		//don't close anything. theoretically nothing is changing, and we don't want to load new defaults either.
	}
	else if (!fs_manifest || !strcmp(fs_manifest->installation?fs_manifest->installation:"", man->installation?man->installation:""))
	{
		if (!fs_manifest)
		{
			basedirchanged = true;
			reloadconfigs = true;
		}
		FS_Manifest_Free(fs_manifest);
	}
	else
	{
		FS_FreePaths();

		reloadconfigs = true;
	}
	fs_manifest = man;



#ifdef PACKAGEMANAGER
	if (basedirchanged)
		PM_ManifestChanged(man);

	if (man->security == MANIFEST_SECURITY_NOT && strcmp(man->downloadsurl?man->downloadsurl:"", olddownloadsurl?olddownloadsurl:""))
	{	//make sure we only fuck over the user if this is a 'secure' manifest, and not hacked in some way.
		Z_Free(man->downloadsurl);
		man->downloadsurl = olddownloadsurl;
	}
	else
		Z_Free(olddownloadsurl);
#else
	(void)basedirchanged;
#endif

	//make sure it has a trailing slash, or is empty. woo.
	FS_CleanDir(com_gamepath, sizeof(com_gamepath));

	{
		qboolean oldhome = com_homepathenabled;
		COM_InitHomedir(man);

		if (com_homepathenabled != oldhome)
		{
			if (com_homepathenabled)
				Con_TPrintf("Using home directory \"%s\"\n", com_homepath);
			else
				Con_TPrintf("Disabled home directory support\n");
		}
	}

#ifdef ANDROID
	{
		//write a .nomedia file to avoid people from getting random explosion sounds etc interspersed with their music
		vfsfile_t *f;
		char nomedia[MAX_OSPATH];
		//figure out the path we're going to end up writing to
		if (com_homepathenabled)
			snprintf(nomedia, sizeof(nomedia), "%s%s", com_homepath, ".nomedia");
		else
			snprintf(nomedia, sizeof(nomedia), "%s%s", com_gamepath, ".nomedia");

		//make sure it exists.
		f = VFSOS_Open(nomedia, "rb");
		if (!f)	//don't truncate
		{
			COM_CreatePath(nomedia);
			f = VFSOS_Open(nomedia, "wb");
		}
		if (f)
			VFS_CLOSE(f);
	}
#endif

	//our basic filesystem should be okay, but no packages loaded yet.
#ifdef MANIFESTDOWNLOADS
	//make sure the package manager knows what its meant to know...
	PM_AddManifestPackages(man, allowdownload);
#endif

	if (Sys_LockMutex(fs_thread_mutex))
	{
#ifdef HAVE_CLIENT
		int vidrestart = FS_GameIsInitialised()?false:2;
#endif

		FS_ReloadPackFilesFlags(~0);

		Sys_UnlockMutex(fs_thread_mutex);

		FS_BeginManifestUpdates();

#ifdef MANIFESTDOWNLOADS
		if (FS_DownloadingPackage() && fs_loadedcommand)
			allowreloadconfigs = false;
#endif

		COM_CheckRegistered();

#ifdef HAVE_CLIENT
		if (qrenderer != QR_NONE && allowvidrestart && !vidrestart)
		{
			for (i = 0; i < countof(vidfile); i++)
			{
				FS_FLocateFile(vidfile[i], FSLF_IFFOUND, &loc);
				if (vidpath[i] != (loc.search?loc.search->handle:NULL))
				{
					vidrestart = true;
					Con_DPrintf("Restarting video because %s has changed\n", vidfile[i]);
				}
			}
		}
#endif

		if (allowreloadconfigs)
		{
			if (!reloadconfigs)
				for (i = 0; i < countof(conffile); i++)
				{
					FS_FLocateFile(conffile[i], FSLF_IFFOUND|FSLF_IGNOREPURE, &loc);
					if (confpath[i] != (loc.search?loc.search->handle:NULL))
					{
						reloadconfigs = true;
						Con_DPrintf("Reloading configs because %s has changed\n", conffile[i]);
						break;
					}
				}

			if (reloadconfigs)
			{
				Cvar_SetEngineDefault(&fs_gamename, man->formalname?man->formalname:"FTE");
				Cvar_SetEngineDefault(&com_protocolname, man->protocolname?man->protocolname:"FTE");
				//FIXME: flag this instead and do it after a delay?
				Cvar_ForceSet(&fs_gamename, fs_gamename.enginevalue);
				Cvar_ForceSet(&com_protocolname, com_protocolname.enginevalue);

#ifdef HAVE_SERVER
				if (isDedicated)
					SV_ExecInitialConfigs(man->defaultexec?man->defaultexec:"");
				else
#endif
#ifdef HAVE_CLIENT
				if (1)
				{
					CL_ExecInitialConfigs(man->defaultexec?man->defaultexec:"", vidrestart==2);
					vidrestart = false;
				}
				else
#endif
				{
					COM_ParsePlusSets(true);
					Cbuf_Execute ();
				}
			}
#ifdef HAVE_CLIENT

			if (Cmd_Exists("ui_restart"))	//if we're running a q3 ui, restart it now...
				Cbuf_InsertText("ui_restart\n", RESTRICT_LOCAL, false);
#endif

			if (fs_loadedcommand)
			{
				Cbuf_AddText(fs_loadedcommand, RESTRICT_INSECURE);
				Z_Free(fs_loadedcommand);
				fs_loadedcommand = NULL;
			}
		}
#ifdef HAVE_CLIENT
		if (vidrestart == 2)
		{	//done when we picked an initial mod to run (so managed to actually read user settings instead of being forced windowed)
			Cbuf_AddText ("vid_restart\n", RESTRICT_LOCAL);
			vidrestart = false;
		}
		else if (vidrestart)
		{
			Cbuf_AddText ("vid_reload\n", RESTRICT_LOCAL);
			vidrestart = false;
		}


		if (qrenderer != QR_NONE && allowreloadconfigs)
		{
			for (i = 0; i < countof(menufile); i++)
			{
				FS_FLocateFile(menufile[i], FSLF_IFFOUND, &loc);
				if (menupath[i] != (loc.search?loc.search->handle:NULL))
				{
					Cbuf_AddText ("menu_restart\n", RESTRICT_LOCAL);
					break;
				}
			}
		}
#endif

		//rebuild the cache now, should be safe to waste some cycles on it
		COM_FlushFSCache(false, true);
	}

#ifdef HAVE_CLIENT
	Validation_FlushFileList();	//prevent previous hacks from making a difference.
#endif

	{
		void (QDECL *callback)(struct cvar_s *var, char *oldvalue) = fs_game.callback;
		fs_game.callback = NULL;
		Cvar_ForceSet(&fs_game, FS_GetGamedir(false));
		fs_game.callback = callback;
	}
	Cvar_ForceSet(&fs_gamepath, va("%s%s", com_gamepath, FS_GetGamedir(false)));
	Cvar_ForceSet(&fs_basepath, com_gamepath);
	Cvar_ForceSet(&fs_homepath, com_gamepath);

	Mods_FlushModList();

	return true;
}

void FS_CreateBasedir(const char *path)
{
	vfsfile_t *f;
	com_installer = false;
	if (path)
	{
		Q_strncpyz (com_gamepath, path, sizeof(com_gamepath));
		COM_CreatePath(com_gamepath);
	}
	fs_manifest->security = MANIFEST_SECURITY_INSTALLER;
	FS_ChangeGame(fs_manifest, true, false);

	if (path && host_parms.manifest)
	{
		f = FS_OpenVFS("default.fmf", "wb", FS_ROOT);
		if (f)
		{
			VFS_WRITE(f, host_parms.manifest, strlen(host_parms.manifest));
			VFS_CLOSE(f);
		}
	}
}

typedef struct
{
	qboolean anygamedir;
	const char *basedir;
	int found;
	qboolean (*callback)(void *usr, ftemanifest_t *man, enum modsourcetype_e sourcetype);
	enum modsourcetype_e sourcetype;
	void *usr;
} fmfenums_t;
static int QDECL FS_EnumeratedFMF(const char *fname, qofs_t fsize, time_t mtime, void *inf, searchpathfuncs_t *spath)
{
	ftemanifest_t *man = NULL;
	fmfenums_t *e = inf;
	vfsfile_t *f = NULL;		
	if (spath)
	{
		flocation_t loc;
		if (spath->FindFile(spath, &loc, fname, NULL))
		{
			f = spath->OpenVFS(spath, &loc, "rb");
			if (f)
			{
				size_t l = VFS_GETLEN(f);
				char *data = Z_Malloc(l+1);
				if (data)
				{
					VFS_READ(f, data, l);
					data[l] = 0;	//just in case.

					man = FS_Manifest_ReadMem(NULL, e->basedir, data);
					Z_Free(data);
				}
				VFS_CLOSE(f);
			}
		}
	}
	else if (e->basedir == com_gamepath)
		man = FS_Manifest_ReadMod(fname);
	else
	{
		man = FS_Manifest_ReadSystem(fname, e->basedir);

		if (man && !man->basedir && man->installation && *man->installation)
		{	//try and give it a proper gamedir...
			char basedir[MAX_OSPATH];

			//enables sources etc.
			man->security = MANIFEST_SECURITY_INSTALLER;

			if (Sys_FindGameData(NULL, man->installation, basedir, sizeof(basedir), false))
				man->basedir = Z_StrDup(basedir);
		}
	}

	if (man)
	{
		if (!e->anygamedir && man->basedir && strcmp(man->basedir, com_gamepath))
			FS_Manifest_Free(man);
		else if (e->callback(e->usr, man, e->sourcetype))
			e->found++;
		else
			FS_Manifest_Free(man);
	}

	return true;
}

//callback must call FS_Manifest_Free or return false.
int FS_EnumerateKnownGames(qboolean (*callback)(void *usr, ftemanifest_t *man, enum modsourcetype_e sourcetype), void *usr, qboolean fixedbasedir)
{
	int i;
	char basedir[MAX_OSPATH];
	fmfenums_t e;
	ftemanifest_t *man;
	e.anygamedir = (!fs_manifest || !*fs_manifest->installation) && !fixedbasedir;
	e.found = 0;
	e.callback = callback;
	e.usr = usr;

	if (e.anygamedir)
	{
		e.basedir = com_gamepath;
		man = FS_ReadDefaultManifest(com_gamepath, 0, true);
		if (man)
		{
			if (e.callback(e.usr, man, MST_DEFAULT))
				e.found++;
			else
				FS_Manifest_Free(man);
		}
	}

	//okay, no manifests in the basepack, try looking in the basedir.
	//this defaults to the working directory. perhaps try the exe's location instead?
	e.basedir = com_gamepath;
	e.sourcetype = MST_BASEDIR;
	Sys_EnumerateFiles(com_gamepath, "*.fmf", FS_EnumeratedFMF, &e, NULL);
	if (*com_homepath)
	{
		e.sourcetype = MST_HOMEDIR;
		Sys_EnumerateFiles(com_homepath, "*.fmf", FS_EnumeratedFMF, &e, NULL);
	}

//	if (e.anygamedir)
	{
#ifdef __unix__
		const char *dirs = getenv("XDG_CONFIG_DIRS");
		if (!dirs || !*dirs)
			dirs = "/etc/xdg";

		e.basedir = NULL;
		e.sourcetype = MST_SYSTEM;
		while (dirs && *dirs)
		{
			dirs = COM_ParseStringSetSep(dirs, ':', basedir, sizeof(basedir));
			if (*basedir)
				FS_CleanDir(basedir, sizeof(basedir));
			Q_strncatz(basedir, "fteqw/*.fmf", sizeof(basedir));
			Sys_EnumerateFiles(NULL, basedir, FS_EnumeratedFMF, &e, NULL);
		}
#endif
	}

	//-basepack is primarily an android feature, where the apk file is specified.
	//this allows custom mods purely by customising the apk
	e.basedir = host_parms.basedir;
	e.sourcetype = MST_SYSTEM;
	i = COM_CheckParm ("-basepack");
	while (i && i < com_argc-1)
	{
		const char *pakname = com_argv[i+1];
		searchpathfuncs_t *pak;
		vfsfile_t *vfs = VFSOS_Open(pakname, "rb");
		pak = FS_OpenPackByExtension(vfs, NULL, pakname, pakname, "");
		if (pak)
		{
			pak->EnumerateFiles(pak, "*.fmf", FS_EnumeratedFMF, &e);
			pak->ClosePath(pak);
		}
		i = COM_CheckNextParm ("-basepack", i);
	}

	//right, no fmf files anywhere.
	//just make stuff up from whatever games they may have installed on their system.
	for (i = 0; gamemode_info[i].argname; i++)
	{
		Q_strncpyz(basedir, com_gamepath, sizeof(basedir));
		if (gamemode_info[i].manifestfile ||
			((gamemode_info[i].exename || (i>0&&gamemode_info[i].customexec&&gamemode_info[i-1].customexec&&strcmp(gamemode_info[i].customexec,gamemode_info[i-1].customexec))) && FS_DirHasGame(basedir, i)) ||
			(e.anygamedir&&Sys_FindGameData(NULL, gamemode_info[i].argname+1, basedir, sizeof(basedir), false)))
		{
			man = FS_GenerateLegacyManifest(i, basedir);
			if (e.callback(e.usr, man, MST_INTRINSIC))
				e.found++;
			else
				FS_Manifest_Free(man);
		}
	}
	return e.found;
}

//attempts to find a new basedir for 'input', changing to it as appropriate
//returns fixed up filename relative to the new gamedir.
//input must be an absolute path.
qboolean FS_FixupGamedirForExternalFile(char *input, char *filename, size_t fnamelen)
{
	char syspath[MAX_OSPATH];
	char gamepath[MAX_OSPATH];
	void *iterator;
	char *sep,*bs;
	char *src = NULL;

	Q_strncpyz(filename, input, fnamelen);

	iterator = NULL;
	while(COM_IteratePaths(&iterator, syspath, sizeof(syspath), gamepath, sizeof(gamepath)))
	{
		if (!Q_strncasecmp(syspath, filename, strlen(syspath)))
		{
			src = filename+strlen(syspath);
			memmove(filename, src, strlen(src)+1);
			break;
		}
	}
	if (!src)
	{
		for(;;)
		{
			sep = strchr(filename, '\\');
			if (sep)
				*sep = '/';
			else
				break;
		}
		for (sep = NULL;;)
		{
			bs = sep;
			sep = strrchr(filename, '/');
			if (bs)
				*bs = '/';
			if (sep)
			{
				int game;
				*sep = 0;
				if (strchr(filename, '/'))	//make sure there's always at least one /
				{
					char temp[MAX_OSPATH];
					Q_snprintfz(temp, sizeof(temp), "%s/", filename);
					game = FS_IdentifyDefaultGameFromDir(temp);
					if (game != -1)
					{
						static char newbase[MAX_OSPATH];
						if (!host_parms.basedir || strcmp(host_parms.basedir, filename))
						{
							Con_Printf("switching basedir+game to %s for %s\n", filename, input);
							Q_strncpyz(newbase, filename, sizeof(newbase));
							host_parms.basedir = newbase;
							FS_ChangeGame(FS_GenerateLegacyManifest(game, newbase), true, true);
						}
						*sep = '/';
						sep = NULL;
						src = filename+strlen(host_parms.basedir);
						memmove(filename, src, strlen(src)+1);
						break;
					}
				}
			}
			else
				break;
		}
		if (sep)
			*sep = '/';
	}
	if (!src && host_parms.binarydir && !Q_strncasecmp(host_parms.binarydir, filename, strlen(host_parms.binarydir)))
	{
		src = filename+strlen(host_parms.binarydir);
		memmove(filename, src, strlen(src)+1);
	}
	if (!src && host_parms.basedir && !Q_strncasecmp(host_parms.basedir, filename, strlen(host_parms.basedir)))
	{
		src = filename+strlen(host_parms.basedir);
		memmove(filename, src, strlen(src)+1);
	}
	if (!src)
	{
		Q_snprintfz(filename, fnamelen, "#%s", input);
		return false;
	}
	if (*filename == '/' || *filename == '\\')
		memmove(filename, filename+1, strlen(filename+1)+1);

	sep = strchr(filename, '/');
	bs = strchr(filename, '\\');
	if (bs && (!sep || bs < sep))
		sep = bs;
	if (sep)
	{
		Con_Printf("switching gamedir for %s\n", filename);
		*sep = 0;
		COM_Gamedir(filename, NULL);
		memmove(filename, sep+1, strlen(sep+1)+1);
		return true;
	}
	Q_snprintfz(filename, fnamelen, "#%s", input);
	return false;
}

void Cvar_GamedirChange(void);
void		Plug_Shutdown(qboolean preliminary);

/*mod listing management*/
static struct modlist_s *modlist;
static size_t nummods;
static qboolean modsinited;
void Mods_FlushModList(void)
{
	while (nummods)
	{
		nummods--;
		if (modlist[nummods].manifest)
			FS_Manifest_Free(modlist[nummods].manifest);
		if (modlist[nummods].description)
			Z_Free(modlist[nummods].description);
		if (modlist[nummods].gamedir)
			Z_Free(modlist[nummods].gamedir);
	}
	if (modlist)
		Z_Free(modlist);
	modlist = NULL;
	modsinited = false;
}

static qboolean Mods_AddManifest(void *usr, ftemanifest_t *man, enum modsourcetype_e sourcetype)
{
	int p, best = -1;
	int i = nummods;

	switch(sourcetype)
	{
	case MST_SYSTEM:	//part of the app's install or via some other system package that should be found upfront.
	case MST_INTRINSIC:	//embedded into the engine (very little info)
		//if we seem to already know about this game in the same basedir then assume its a dupe. don't care
		//note that intrinsics are ignored entirely if someone took the time to make any other kind of fmf for that basedir.
		for (p = 0; p < nummods; p++)
		{
			if (	modlist[p].manifest &&
					!strcmp(modlist[p].manifest->basedir?modlist[p].manifest->basedir:"", man->basedir?man->basedir:"") &&
					!strcmp(modlist[p].manifest->mainconfig?modlist[p].manifest->mainconfig:"", man->mainconfig?man->mainconfig:"") &&
					((modlist[p].sourcetype!=MST_INTRINSIC&&sourcetype==MST_INTRINSIC) || !Q_strcasecmp(modlist[p].manifest->installation, man->installation)))
				return false;
		}
		break;
	case MST_DEFAULT:	//the default.fmf (basically MST_BASEDIR, but posh)
	case MST_BASEDIR:	//fmf found inside the basedir we're using  (yeah, weird, you'll have to pick the base game to switch basedir first).
	case MST_HOMEDIR:	//fmf found inside the homedir of the mod we're using (yeah, weird, you'll have to pick the base game first)
	case MST_GAMEDIR:	//found inside a gamedir...
	case MST_UNKNOWN:	//shouldn't really be hit.
		break;
	}

	for (p = 0; p < countof(man->gamepath); p++)
		if (man->gamepath[p].path)
		{
			if (man->gamepath[p].flags & (GAMEDIR_PRIVATE|GAMEDIR_STEAMGAME))
				continue;	//don't pick paths that don't make sense to others.
			if (!(man->gamepath[p].flags & GAMEDIR_BASEGAME) || (best<0||(man->gamepath[best].flags & GAMEDIR_BASEGAME)))
				best = p;
		}
	if (best < 0)
	{
		modlist = BZ_Realloc(modlist, (i+1) * sizeof(*modlist));
		modlist[i].manifest = man;
		modlist[i].sourcetype = sourcetype;
		modlist[i].gamedir = Z_StrDup("?");
		modlist[i].description = man->formalname?Z_StrDup(man->formalname):NULL;
		nummods = i+1;
		return true; //no gamedirs? wut? some partially-defined game we don't recognise?
	}

	modlist = BZ_Realloc(modlist, (i+1) * sizeof(*modlist));
	modlist[i].manifest = man;
	modlist[i].sourcetype = sourcetype;
	modlist[i].gamedir = Z_StrDup(man->gamepath[best].path);
	modlist[i].description = man->formalname?Z_StrDup(man->formalname):NULL;
	nummods = i+1;
	return true;
}
static int Mods_WasPackageOrDatFound(const char *fname, qofs_t ofs, time_t modtime, void *usr, searchpathfuncs_t *spath)
{	//we check for *.dat too, because we care about [qw]progs.dat/menu.dat/csprogs.dat or possibly addons. hopefully we can get away with such a generic extension.
	const char *ext = COM_GetFileExtension(fname, NULL);
	if (!Q_strcasecmp(ext, ".pk3") || !Q_strcasecmp(ext, ".pak") || !Q_strcasecmp(ext, ".dat"))
		return false;	//found one, can stop searching now
	//FIXME: pk3dir
	return true;	//keep looking for one
}
static int Mods_WasMapFound(const char *fname, qofs_t ofs, time_t modtime, void *usr, searchpathfuncs_t *spath)
{
	const char *ext = COM_GetFileExtension(fname, NULL);
	if (!Q_strcasecmp(ext, ".gz"))
		ext = COM_GetFileExtension(fname, ext);
	//don't bother looking for .map
	if (!Q_strcasecmp(ext, ".bsp"   ) || !Q_strcasecmp(ext, ".hmp"   ) ||
		!Q_strcasecmp(ext, ".bsp.gz") || !Q_strcasecmp(ext, ".hmp.gz"))
		return false;	//found one, can stop searching now
	return true;	//keep looking for one
}
static int QDECL Mods_AddGamedir(const char *fname, qofs_t fsize, time_t mtime, void *usr, searchpathfuncs_t *spath)
{
	char *desc;
	size_t l = strlen(fname);
	int i, p;
	char gamedir[MAX_QPATH];
	const char *basedir = usr;
	if (l && fname[l-1] == '/' && l < countof(gamedir))
	{
		l--;
		memcpy(gamedir, fname, l);
		gamedir[l] = 0;
		for (i = 0; i < nummods; i++)
		{
			//don't add dupes (can happen from basedir+homedir)
			//if the gamedir was already included in one of the manifests, don't bother including it again.
			//this generally removes id1.
			if (modlist[i].manifest)
			{
				for (p = 0; p < countof(fs_manifest->gamepath); p++)
					if (modlist[i].manifest->gamepath[p].path)
						if (!Q_strcasecmp(modlist[i].manifest->gamepath[p].path, gamedir))
							return true;
			}
			else if (modlist[i].gamedir)
			{
				if (!Q_strcasecmp(modlist[i].gamedir, gamedir))
					return true;
			}
		}

		if ((desc = FS_MallocFile(va("%s%s/modinfo.txt", basedir, gamedir), FS_SYSTEM, NULL)))
			;	//dp's modinfo.txt thing (which no mod seems to use anyway)
		else if ((desc = FS_MallocFile(va("%s%s/description.txt", basedir, gamedir), FS_SYSTEM, NULL)))
			;	//quake3's description stuff
		else if ((desc = FS_MallocFile(va("%s%s/liblist.gam", basedir, gamedir), FS_SYSTEM, NULL)))
		{	//halflifeisms? o.O mneh why not
			size_t u;
			Cmd_TokenizeString(desc, false, false);
			FS_FreeFile(desc);
			desc = NULL;
			for (u = 0; u < Cmd_Argc(); u+=2)
			{
				if (!strcasecmp(Cmd_Argv(u), "game"))
					desc = Cmd_Argv(u+1);
			}
			if (desc)
				desc = Z_StrDup(desc);
		}
		//we don't really know what it is. probably some useless subdir. report it only if it looks like there's something actually interesting in there
		else if (!Sys_EnumerateFiles(va("%s%s/", basedir, gamedir), "*.*", Mods_WasPackageOrDatFound, NULL, NULL) ||
				!Sys_EnumerateFiles(va("%s%s/maps/", basedir, gamedir), "*.*", Mods_WasMapFound, NULL, NULL))
			;	//stopped early means we found a file.
		else
			return true;	//nothing interesting there... don't bother to list it

		if (strchr(gamedir, ';') || !FS_GamedirIsOkay(gamedir))
		{
			Z_Free(desc);
			return true;	//don't list it if we can't use it anyway
		}

		modlist = BZ_Realloc(modlist, (i+1) * sizeof(*modlist));
		modlist[i].manifest = NULL;
		modlist[i].gamedir = Z_StrDup(gamedir);
		modlist[i].description = desc;
		nummods = i+1;
	}
	return true;
}

static int QDECL Mods_SortMod(const void *first, const void *second)
{
	const struct modlist_s *a = first;
	const struct modlist_s *b = second;
	int d = 0;
	if (a->manifest || b->manifest)
	{
		if (a->manifest && b->manifest)
		{
			if (!d)
				d = Q_strcasecmp(a->manifest->formalname, b->manifest->formalname);
			if (!d)
				d = Q_strcasecmp(a->manifest->basedir, b->manifest->basedir);
			if (!d)
				d = strcmp(a->gamedir, b->gamedir);
		}
		else
			d = a->manifest?1:-1;	//put manifest-based ones first.
	}
	if (!d)
		d = strcmp(a->gamedir, b->gamedir);
	return d;
}
struct modlist_s *Mods_GetMod(size_t diridx)
{
	if (!modsinited)
	{
		int mancount;
		modsinited = true;
		FS_EnumerateKnownGames(Mods_AddManifest, NULL, true);
		mancount = nummods;
		if (*fs_manifest->installation)
		{
			if (com_homepathenabled)
				Sys_EnumerateFiles(com_homepath, "*", Mods_AddGamedir, com_homepath, NULL);
			Sys_EnumerateFiles(com_gamepath, "*", Mods_AddGamedir, com_gamepath, NULL);
		}
		qsort(modlist+mancount, nummods-mancount, sizeof(*modlist), Mods_SortMod);
	}
	if (diridx < nummods)
		return &modlist[diridx];
	return NULL;
}



#if defined(HAVE_CLIENT) && defined(WEBCLIENT)
typedef struct
{
	char *manifestname;	//manifest getting written
	char *url;			//url to get the manifest from.
	char *mantext;		//contents of downloaded manifest...
	int mansize;
	ftemanifest_t *man;
} modinstall_t;
static void FS_ModInstallConfirmed(void *vctx, promptbutton_t button)
{
	modinstall_t *ctx = vctx;
	if (button == PROMPT_YES)
	{
		vfsfile_t *out = FS_OpenVFS(ctx->manifestname, "wb", FS_SYSTEM);
		if (out)
		{
			VFS_WRITE(out, ctx->mantext, ctx->mansize);
			VFS_CLOSE(out);

			FS_ChangeGame(ctx->man, true, true);
			ctx->man = NULL;
		}
	}
	Z_Free(ctx->mantext);
	Z_Free(ctx->url);
	Z_Free(ctx->manifestname);
	FS_Manifest_Free(ctx->man);
	Z_Free(ctx);
}
static void FS_ModInstallGot(struct dl_download *dl)
{
	modinstall_t *ctx = dl->user_ctx;
	if (dl->file && dl->status == DL_FINISHED)
	{
		ctx->mansize = VFS_GETLEN(dl->file);
		ctx->mantext = Z_Malloc(ctx->mansize+1);
		VFS_READ(dl->file, ctx->mantext, ctx->mansize);
		ctx->mantext[ctx->mansize] = 0;

		ctx->man = FS_Manifest_ReadMem(ctx->manifestname, com_gamepath, ctx->mantext);
		if (ctx->man && !strcmp(ctx->man->basedir, com_gamepath))
		{
			//should probably show just the hostname for brevity.
			Menu_Prompt(FS_ModInstallConfirmed, ctx, va(localtext("Install %s from\n%s ?"), ctx->man->formalname, ctx->url), "Install", NULL, "Cancel", true);
			return;
		}
	}

	FS_ModInstallConfirmed(ctx, PROMPT_CANCEL);
}
static void FS_ModInstall(const char *dest, const char *url)
{
	struct dl_download *dl;
	char fmfpath[MAX_OSPATH];

	ftemanifest_t *man = NULL;

	//find out a writable path for the fmf.
	if (!FS_SystemPath(va("%s.fmf", dest), FS_ROOT, fmfpath, sizeof(fmfpath)))
		return;

	//read it in if it exists.
	man = FS_Manifest_ReadMod(dest);
	if (man)
	{
		FS_ChangeGame(man, cfg_reload_on_gamedir.ival, false);
		return;
	}

	dl = HTTP_CL_Get(url, NULL, FS_ModInstallGot);
	if (dl)
	{
		modinstall_t *m = Z_Malloc(sizeof(*m));
		m->manifestname = Z_StrDup(fmfpath);
		m->url = Z_StrDup(url);
		dl->user_ctx = m;
#ifdef MULTITHREAD
		DL_CreateThread(dl, NULL, NULL);
#endif
	}
}
#else
static void FS_ModInstall(const char *dest, const char *url)
{
}
#endif


//switches manifests
//no args: lists known games
//1 arg:
//	~/quake/				trailing slash switches basedir (using said basedir's default manifest)
//  quake3					loads hardcoded mod
//	~/foo.fmf				loads the specified manifest
//	http://foo/bar.fmf		loads the specified manifest. archaic. doesn't save the fmf anywhere (will download its pk3s)
//2 args:
//	foo http://foo/bar.fmf	downloads to $basedir/foo.fmf if it doesn't exist (prompts), and then always loads it (like 'gamedir', no prompt when it already exists).
static void FS_ChangeGame_f(void)
{
	unsigned int i;
	const char *arg = Cmd_Argv(1);
	char *end;
	struct modlist_s *mod;
	ftemanifest_t *man;

	//don't execute this if we're executing rcon commands, as this can change game directories and ruin logging.
	if (cmd_blockwait)
		return;

	if ((i = strtol(arg, &end, 10)) && !*end)
	{	//for use by qc. loading mods by number...
		mod = Mods_GetMod(--i);
		if (mod)
		{
#ifdef HAVE_CLIENT
			CL_Disconnect(NULL);
#endif
#ifdef HAVE_SERVER
			if (sv.state)
				SV_UnspawnServer();
#endif
			if (mod->manifest)
			{
				man = FS_Manifest_Clone(mod->manifest);	//FS_ChangeGame takes ownership... don't crash if its cached.
				FS_ChangeGame(man, true, true);
			}
			else
				COM_Gamedir(mod->gamedir, NULL);
#ifdef HAVE_CLIENT
//			Cbuf_AddText("menu_restart\n", RESTRICT_LOCAL);
#endif
		}
		return;
	}
	else if (Cmd_Argc()==3)
	{	//allowed to bypass insecurity.
		//acts like gamedir, but prompts if you try anything else.
		FS_ModInstall(arg, Cmd_Argv(2));
		return;
	}
	else if (Cmd_IsInsecure())
	{
		Con_Printf("Blocking insecure command: %s %s\n", Cmd_Argv(0), Cmd_Args());
		return;
	}
	else if (!*arg)
	{
		Con_Printf("Valid games/mods are:\n");
		for (i = 0; (mod = Mods_GetMod(i)); i++)
		{
			man = mod->manifest;
			if (man)
				Con_Printf("\t^[%s\\tip\\%s\n%s\\type\\fs_changegame \"%u\" //%s^] (%s)\n", mod->description?mod->description:man->formalname, man->installation, man->filename?man->filename:"<INTERNAL>", i+1, man->filename, man->basedir?man->basedir:"not installed");
			else
				Con_Printf("\t^[%s\\type\\gamedir \"%s\"^]\n", mod->description?mod->description:mod->gamedir, mod->gamedir);
		}
	}
	else
	{
		arg = Z_StrDup(arg);
#ifdef HAVE_SERVER
		if (sv.state)
			SV_UnspawnServer();
#endif
#ifdef HAVE_CLIENT
		CL_Disconnect (NULL);
#endif
#ifdef PLUGINS
		Plug_Shutdown(true);
#endif
		Cvar_GamedirChange();

		if (strrchr(arg, '/') && !strrchr(arg, '/')[1])
		{	//ends in slash. a new basedir.
			Q_strncpyz(com_gamepath, arg, sizeof(com_gamepath));
			host_parms.basedir = com_gamepath;
			FS_ChangeGame(FS_ReadDefaultManifest(NULL, 0, true), true, true);
		}
		else
		{
			for (i = 0; gamemode_info[i].argname; i++)
			{
				if (!Q_strcasecmp(gamemode_info[i].argname+1, arg))
				{
					Con_Printf("Switching to %s\n", gamemode_info[i].argname+1);
					FS_ChangeGame(FS_GenerateLegacyManifest(i, NULL), true, true);
					break;
				}
			}

			if (!gamemode_info[i].argname)
			{
#ifdef HAVE_CLIENT
				if (!Host_RunFile(arg, strlen(arg), NULL))
					Con_Printf("Game unknown\n");
#endif
			}
		}
		Z_Free((char*)arg);

#ifdef PLUGINS
		Plug_Initialise(true);
#endif
#if defined(HAVE_CLIENT) && defined(Q3CLIENT)
		if (q3)
			q3->ui.Start();
#endif
	}
}

//this function exists for use by the QI plugin and uses hacked up variations of the default manifest.
static void FS_ChangeMod_f(void)
{
	char cachename[512];
	struct gamepacks packagespaths[16];
	int i;
	int packages = 0;
	const char *arg = "?";
	qboolean okay = false;
	char *dir = NULL;

	if (Cmd_IsInsecure())
		return;

	Z_Free(fs_loadedcommand);
	fs_loadedcommand = NULL;

	memset(packagespaths, 0, sizeof(packagespaths));

	for (i = 1; ; )
	{
		if (i == Cmd_Argc())
		{
			okay = true;
			break;
		}
		arg = Cmd_Argv(i++);
		if (!strcmp(arg, "package"))
		{
			arg = Cmd_Argv(i++);
			if (packages == countof(packagespaths))	//must leave space for one, as a terminator.
				continue;
			if (FS_PathURLCache(arg, cachename, sizeof(cachename)))
			{
				packagespaths[packages].url = Z_StrDup(arg);
				packagespaths[packages].path = Z_StrDup(cachename);
				packages++;
			}
		}
		else if (!strcmp(arg, "hash"))
		{
			if (!packages)
				break;
			arg = Cmd_Argv(i++);
//			packagespaths[packages-1].hash = Z_StrDup(arg);
		}
		else if (!strcmp(arg, "prefix"))
		{
			if (!packages)
				break;
			arg = Cmd_Argv(i++);
			packagespaths[packages-1].subpath = Z_StrDup(arg);
		}
		else if (!strcmp(arg, "dir"))
		{
			arg = Cmd_Argv(i++);
			Z_StrDupPtr(&dir, arg);
		}
		else if (!strcmp(arg, "map"))
		{
			Z_Free(fs_loadedcommand);
			arg = va("map \"%s\"\n", Cmd_Argv(i++));
			fs_loadedcommand = Z_StrDup(arg);
		}
		else if (!strcmp(arg, "spmap"))
		{
			Z_Free(fs_loadedcommand);
			arg = va("deathmatch 0;coop 0;spmap \"%s\"\n", Cmd_Argv(i++));
			fs_loadedcommand = Z_StrDup(arg);
		}
		else if (!strcmp(arg, "restart"))
		{
			Z_Free(fs_loadedcommand);
			fs_loadedcommand = Z_StrDup("restart\n");
		}
		else
			break;
	}

	if (okay)
		COM_Gamedir(dir?dir:"", packagespaths);
	else
	{
		Con_Printf("unsupported args: %s\n", arg);
		Z_Free(fs_loadedcommand);
		fs_loadedcommand = NULL;
	}
	Z_Free(dir);

	for (i = 0; i < packages; i++)
	{
		Z_Free(packagespaths[i].url);
		Z_Free(packagespaths[i].path);
		Z_Free(packagespaths[i].subpath);
	}
}

static void FS_ShowManifest_f(void)
{
	if (Cmd_IsInsecure())
		return;

	if (fs_manifest)
		FS_Manifest_Print(fs_manifest);
	else
		Con_Printf("no manifest loaded...\n");
}

static int QDECL FS_ArbitraryFile_cb (const char *name, qofs_t flags, time_t mtime, void *parm, searchpathfuncs_t *spath)
{
	struct xcommandargcompletioncb_s *ctx = parm;
	ctx->cb(name, NULL, NULL, ctx);
	return true;
}
void FS_ArbitraryFile_c(int argn, const char *partial, struct xcommandargcompletioncb_s *ctx)
{
	if (argn == 1)
	{
		COM_EnumerateFiles(va("%s*", partial), FS_ArbitraryFile_cb, ctx);
	}
}

//FIXME: this should come from the manifest, as fte_GAME or something
#ifdef _WIN32
	//windows gets formal names...
	#ifdef GAME_FULLNAME
		#define HOMESUBDIR GAME_FULLNAME
	#else
		#define HOMESUBDIR FULLENGINENAME
	#endif
#else
	//unix gets short names...
	#ifdef GAME_SHORTNAME
		#define HOMESUBDIR GAME_SHORTNAME
	#else
		#define HOMESUBDIR "fte"
	#endif
#endif

#if defined(_WIN32) && !defined(FTE_SDL) && !defined(WINRT) && !defined(_XBOX)
//so this is kinda screwy
//"CSIDL_LOCAL_APPDATA/FTE Quake" is what we switched to, but we only use it if the other home dirs don't exist
//"CSIDL_PERSONAL(My Documents)/My Games/FTE Quake" is what we used to use... but personal now somehow means upload-to-internet in a nicely insecure we-own-all-your-data kind of way...
//"%USERPROFILE%/My Documents/My Games/FTE Quake" is an attempt to fall back to the earlier lame path if everything else fails
//"%USERPROFILE%/Saved Games/FTE Quake" is what we probably should be using. I don't know who comes up with these random paths. We have updates+downloads+etc as well as just saves, so we prioritise localdata instead (stuff that you do NOT want microsoft to upload to the internet all the time).
#include <shlobj.h>
static qboolean FS_GetBestHomeDir(ftemanifest_t *manifest)
{	//win32 sucks.
	qboolean usehome = false;
	HRESULT (WINAPI *dSHGetFolderPathW) (HWND hwndOwner, int nFolder, HANDLE hToken, DWORD dwFlags, wchar_t *pszPath) = NULL;
	HRESULT (WINAPI *dSHGetKnownFolderPath) (const GUID *const rfid, DWORD dwFlags, HANDLE hToken, PWSTR *ppszPath) = NULL;
	dllfunction_t funcs[] =
	{
		{(void**)&dSHGetFolderPathW, "SHGetFolderPathW"},
		{NULL,NULL}
	};
	dllfunction_t funcsvista[] =
	{
		{(void**)&dSHGetKnownFolderPath, "SHGetKnownFolderPath"},
		{NULL,NULL}
	};
	DWORD winver = (DWORD)LOBYTE(LOWORD(GetVersion()));

	enum
	{
		WINHOME_LOCALDATA,
		WINHOME_SAVEDGAMES,
		WINHOME_DOCUMENTS,
		WINHOME_COUNT
	};

	struct
	{
		char path[MAX_OSPATH];
	} homedir[WINHOME_COUNT];
	int i;

	/*HMODULE shfolder =*/ Sys_LoadLibrary("shfolder.dll", funcs);
	/*HMODULE shfolder =*/ Sys_LoadLibrary("shell32.dll", funcsvista);

	if (dSHGetKnownFolderPath)
	{
		wchar_t *wide;
		static const GUID qFOLDERID_SavedGames = {0x4c5c32ff, 0xbb9d, 0x43b0, {0xb5, 0xb4, 0x2d, 0x72, 0xe5, 0x4e, 0xaa, 0xa4}};
		#define qKF_FLAG_CREATE 0x00008000
		if (SUCCEEDED(dSHGetKnownFolderPath(&qFOLDERID_SavedGames, qKF_FLAG_CREATE, NULL, &wide)))
		{
			narrowen(homedir[WINHOME_SAVEDGAMES].path, sizeof(homedir[WINHOME_SAVEDGAMES].path), wide);
			CoTaskMemFree(wide);
		}
	}

	if (dSHGetFolderPathW)
	{
		wchar_t wfolder[MAX_PATH];
		if (dSHGetFolderPathW(NULL, 0x5/*CSIDL_PERSONAL*/, NULL, 0, wfolder) == S_OK)
		{
			narrowen(homedir[WINHOME_DOCUMENTS].path, sizeof(homedir[WINHOME_DOCUMENTS].path), wfolder);
			Q_strncatz(homedir[WINHOME_DOCUMENTS].path, "/My Games", sizeof(homedir[WINHOME_DOCUMENTS].path));
		}
		//at some point, microsoft (in their infinitesimal wisdom) decided that 'CSIDL_PERSONAL' should mean 'CSIDL_GIVEITALLTOMICROSOFT'
		//so use the old/CSIDL_NOTACTUALLYPERSONAL path by default for compat, but if there's nothing there then switch to CSIDL_LOCAL_APPDATA instead
		if (dSHGetFolderPathW(NULL, 0x1c/*CSIDL_LOCAL_APPDATA*/, NULL, 0, wfolder) == S_OK)
		{
			narrowen(homedir[WINHOME_LOCALDATA].path, sizeof(homedir[WINHOME_LOCALDATA].path), wfolder);
		}
	}
	if (!*homedir[WINHOME_DOCUMENTS].path)
	{	//guess. sucks for non-english people.
		char *ev = getenv("USERPROFILE");
		if (ev)
			Q_snprintfz(homedir[WINHOME_DOCUMENTS].path, sizeof(homedir[WINHOME_DOCUMENTS].path), "%s/My Documents/My Games/%s/", ev, HOMESUBDIR);
	}
//	if (shfolder)
//		FreeLibrary(shfolder);


	for(i = 0; i < countof(homedir); i++)
	{
		char formal[MAX_OSPATH];
		char informal[MAX_OSPATH];
		if (!*homedir[i].path)
			continue; //erk, don't know, not valid/known on this system.
		if (!manifest || !manifest->formalname || (/*legacy compat hack case*/ !strcmp(manifest->formalname, "Quake") && strstr(HOMESUBDIR, "Quake")))
		{
			Q_snprintfz(formal, sizeof(formal), "%s/%s/", homedir[i].path, HOMESUBDIR); //'FTE Quake' or something hardcoded.
			*informal = 0;
		}
		else
		{
			if (		strchr(manifest->formalname, '(') ||	//ugly
						strchr(manifest->formalname, '[') ||	//ugly
						strchr(manifest->formalname, '.') ||	//ugly
						strchr(manifest->formalname, '\"') ||	//ugly (and invalid)
						strchr(manifest->formalname, '|') ||	//invalid
						strchr(manifest->formalname, '<') ||	//invalid
						strchr(manifest->formalname, '>') ||	//invalid
						strchr(manifest->formalname, '\\') ||	//long paths
						strchr(manifest->formalname, '/') ||	//long paths
						strchr(manifest->formalname, ':') ||	//alternative data stream separator
						strchr(manifest->formalname, '*') ||	//wildcard
						strchr(manifest->formalname, '?'))		//wildcard
				*formal = 0;	//don't use filenames with awkward chars...
			else
				Q_snprintfz(formal, sizeof(formal), "%s/%s/", homedir[i].path, manifest->formalname);		//'Quake' / 'The Wastes' / etc
			Q_snprintfz(informal, sizeof(informal), "%s/%s/", homedir[i].path, manifest->installation);	//'quake' / 'wastes' / etc
		}
		if (*formal && GetFileAttributesU(formal)!=INVALID_FILE_ATTRIBUTES) //path exists, use it.
		{
			Q_strncpyz(com_homepath, formal, sizeof(com_homepath));
			break;
		}
		else if (*informal && GetFileAttributesU(informal)!=INVALID_FILE_ATTRIBUTES) //path exists, use it.
		{
			Q_strncpyz(com_homepath, informal, sizeof(com_homepath));
			break;
		}
		else if (!*com_homepath)
		{
			if (!*informal)
				Q_strncpyz(com_homepath, formal, sizeof(com_homepath));
			else
				Q_strncpyz(com_homepath, informal, sizeof(com_homepath));
			continue;	//keep looking, we might still find one that actually exists.
		}
	}

	/*would it not be better to just check to see if we have write permission to the basedir?*/
	if (winver >= 0x6) // Windows Vista and above
		usehome = true; // always use home directory by default, as Vista+ mimics this behavior anyway
	else if (winver >= 0x5) // Windows 2000/XP/2003
		usehome = true;	//might as well follow this logic. We use .manifest stuff to avoid getting redirected to obscure locations, so access rights is all that is relevant, not whether we're an admin or not.

	if (usehome && manifest)
	{
		DWORD homeattr = GetFileAttributesU(com_homepath);
		DWORD baseattr = GetFileAttributesU(com_gamepath);
		if (homeattr != INVALID_FILE_ATTRIBUTES && (homeattr & FILE_ATTRIBUTE_DIRECTORY))
			return true; //okay something else already created it. continue using it.
		if (baseattr != INVALID_FILE_ATTRIBUTES && (baseattr & FILE_ATTRIBUTE_DIRECTORY))
		{	//windows has an _access function, but it doesn't actually bother to check if you're allowed to access it, so its utterly pointless.
			//instead try to append nothing to some file that'll probably exist anyway.
			//this MAY fail if another program has it open. windows sucks.
			vfsfile_t *writetest = VFSOS_Open("conhistory.txt", "a");
			if (!writetest)
				return true; //basedir isn't writable, we'll need our home! use it by default.
			VFS_CLOSE(writetest);
		}
		//else don't use it (unless -usehome, anyway)
	}

	return false;
}
#elif defined(NOSTDIO)
static qboolean FS_GetBestHomeDir(ftemanifest_t *man)
{	//no studio? webgl port? no file system access = no homedirs!
	return false;
}
#else
#include <sys/stat.h>
static qboolean FS_GetBestHomeDir(ftemanifest_t *man)
{
	qboolean usehome = false;

	//on unix, we use environment settings.
	//if $HOME/.fte/ exists then we use that because of legacy reasons.
	//but if it doesn't exist then we use $XDG_DATA_HOME/.fte instead
	//we used to use $HOME/.#HOMESUBDIR/ but this is now only used if it actually exists AND the new path doesn't.
	//new installs use $XDG_DATA_HOME/#HOMESUBDIR/ instead
	char *ev = getenv("FTEHOME");
	if (ev && *ev)
	{
		if (ev[strlen(ev)-1] == '/')
			Q_strncpyz(com_homepath, ev, sizeof(com_homepath));
		else
			Q_snprintfz(com_homepath, sizeof(com_homepath), "%s/", ev);
		usehome = true; // always use home on unix unless told not to
		ev = NULL;
	}
	else
		ev = getenv("HOME");
	if (ev && *ev)
	{
		const char *xdghome;
		char oldhome[MAX_OSPATH];
		char newhome[MAX_OSPATH];
		struct stat s;
		char *installation	= (man && man->installation && *man->installation)?man->installation:HOMESUBDIR;
		usehome				= (man && man->installation && *man->installation)?true:false; //use it if we're running a game, otherwise don't bother if we're still loading.

		xdghome = getenv("XDG_DATA_HOME");
		if (!xdghome || !*xdghome)
			xdghome = va("%s/.local/share", ev);
		if (xdghome[strlen(xdghome)-1] == '/')
			Q_snprintfz(newhome, sizeof(newhome), "%s%s/", xdghome, installation);
		else
			Q_snprintfz(newhome, sizeof(newhome), "%s/%s/", xdghome, installation);

		if (ev[strlen(ev)-1] == '/')
			Q_snprintfz(oldhome, sizeof(oldhome), "%s.%s/", ev, installation);
		else
			Q_snprintfz(oldhome, sizeof(oldhome), "%s/.%s/", ev, installation);

		if (stat(newhome, &s) == -1 && stat(oldhome, &s) != -1)
			Q_strncpyz(com_homepath, oldhome, sizeof(com_homepath));
		else
			Q_strncpyz(com_homepath, newhome, sizeof(com_homepath));
	}

	if (usehome && man)
	{
		struct stat statbuf;
		if (stat(com_homepath, &statbuf) >= 0 && (statbuf.st_mode & S_IFMT)==S_IFDIR)
			return true; //okay something else already created it. continue using it.
		if (access(com_gamepath, W_OK) < 0)
			return true; //baesdir isn't writable, we'll need our home! use it by default.
		//else don't use it (unless -usehome, anyway)
	}
	return false;
}
#endif
static void COM_InitHomedir(ftemanifest_t *man)
{
	int i;

	//assume the home directory is the working directory.
	*com_homepath = '\0';

	if (man && (strstr(man->installation, "..") || strchr(man->installation, '/') || strchr(man->installation, '\\')))
		com_homepathusable = false; //don't even try to generate a relative homedir.
	else
		com_homepathusable = FS_GetBestHomeDir(man);
	com_homepathenabled = false;

	if (man && man->homedirtype == MANIFEST_NOHOMEDIR)
		com_homepathusable = false;
	else if (man && man->homedirtype == MANIFEST_FORCEHOMEDIR)
		com_homepathusable = true;

	i = COM_CheckParm("-homedir");
	if (i && i+1<com_argc)
	{	//explicitly override the homedir.
		Q_strncpyz(com_homepath, com_argv[i+1], sizeof(com_homepath));
		if (*com_homepath && com_homepath[strlen(com_homepath)-1] != '/')
			Q_strncatz(com_homepath, "/", sizeof(com_homepath));
		com_homepathusable = true;
	}
	if (COM_CheckParm("-usehome"))
		com_homepathusable = true;
	if (COM_CheckParm("-nohome"))
		com_homepathusable = false;
	if (!*com_homepath)
		com_homepathusable = false;

	com_homepathenabled = com_homepathusable;
}

#if defined(HAVE_DBUS) && !defined(HAVE_CLIENT)
#undef HAVE_DBUS
#endif
#ifdef HAVE_DBUS
void FS_OpenFilePicker_f(void);
#endif

/*
================
COM_InitFilesystem

note: does not actually load any packs, just makes sure the basedir+cvars+etc is set up. vfs_fopens will still fail.
================
*/
void COM_InitFilesystem (void)
{
	int		i;


	FS_RegisterDefaultFileSystems();

	Cmd_AddCommand("fs_restart", FS_ReloadPackFiles_f);
	Cmd_AddCommandD("fs_changegame", FS_ChangeGame_f, "Switch between different manifests (or registered games)");
	Cmd_AddCommandD("fs_changemod", FS_ChangeMod_f, "Provides the backend functionality of a transient online installer. Eg, for quaddicted's map/mod database.");
	Cmd_AddCommand("fs_showmanifest", FS_ShowManifest_f);
	Cmd_AddCommand ("fs_flush", COM_RefreshFSCache_f);
	Cmd_AddCommandAD("dir", COM_Dir_f,			FS_ArbitraryFile_c, "Displays filesystem listings. Accepts wildcards."); //q3 like
	Cmd_AddCommandAD("ls", COM_Dir_f,			FS_ArbitraryFile_c, "Displays filesystem listings. Accepts wildcards."); //q3 like
	Cmd_AddCommandD("path", COM_Path_f,			"prints a list of current search paths.");
	Cmd_AddCommandAD("flocate", COM_Locate_f,	FS_ArbitraryFile_c, "Searches for a named file, and displays where it can be found in the OS's filesystem");	//prints the pak or whatever where this file can be found.
	Cmd_AddCommandAD("which", COM_Locate_f,	FS_ArbitraryFile_c, "Searches for a named file, and displays where it can be found in the OS's filesystem");	//prints the pak or whatever where this file can be found.

	Cmd_AddCommandAD("fs_hash", COM_CalcHash_f,	FS_ArbitraryFile_c, "Computes a hash of the specified file.");

#ifdef HAVE_DBUS
	Cmd_AddCommandD("sys_openfile", FS_OpenFilePicker_f,	"Select a file to open/install/etc.");
#endif

//
// -basedir <path>
// Overrides the system supplied base directory (under id1)
//
	i = COM_CheckParm ("-basedir");
	if (i && i < com_argc-1)
		strcpy (com_gamepath, com_argv[i+1]);
	else
		strcpy (com_gamepath, host_parms.basedir);

	FS_CleanDir(com_gamepath, sizeof(com_gamepath));


	Cvar_Register(&cfg_reload_on_gamedir, "Filesystem");
	Cvar_Register(&dpcompat_ignoremodificationtimes, "Filesystem");
	Cvar_Register(&com_fs_cache, "Filesystem");
	Cvar_Register(&fs_hidesyspaths, "Filesystem");
	Cvar_Register(&fs_gamename, "Filesystem");
#ifdef PACKAGEMANAGER
	Cvar_Register(&pkg_autoupdate, "Filesystem");
#endif
	Cvar_Register(&com_protocolname, "Server Info");
	Cvar_Register(&com_protocolversion, "Server Info");
	Cvar_Register(&fs_game, "Filesystem");
	Cvar_Register(&fs_gamepath, "Filesystem");
	Cvar_Register(&fs_basepath, "Filesystem");
	Cvar_Register(&fs_homepath, "Filesystem");
	Cvar_Register(&fs_dlURL,	"Filesystem");
	Cvar_Register(&cl_download_mapsrc,	"Filesystem");

	COM_InitHomedir(NULL);

	fs_readonly = COM_CheckParm("-readonly");
	if (COM_CheckParm("-allowfileuri") || COM_CheckParm("-allowfileurl"))
	{
		fs_allowfileuri = (searchpath_t*)Z_Malloc (sizeof(searchpath_t));
		fs_allowfileuri->handle = VFSOS_OpenPath(NULL, NULL, "", "", "");
	}

	fs_thread_mutex = Sys_CreateMutex();
}





//this is at the bottom of the file to ensure these globals are not used elsewhere
/*extern searchpathfuncs_t *(QDECL VFSOS_OpenPath) (vfsfile_t *file, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix);
#if 1//def AVAIL_ZLIB
extern searchpathfuncs_t *(QDECL FSZIP_LoadArchive) (vfsfile_t *packhandle, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix);
#endif
extern searchpathfuncs_t *(QDECL FSPAK_LoadArchive) (vfsfile_t *packhandle, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix);
#ifdef PACKAGE_DOOMWAD
extern searchpathfuncs_t *(QDECL FSDWD_LoadArchive) (vfsfile_t *packhandle, searchpathfuncs_t *parent, const char *filename, const char *desc, const char *prefix);
#endif*/
void FS_RegisterDefaultFileSystems(void)
{	//packages listed last will be scanned for last (and thus be favoured when searching for game files)
#ifdef PACKAGE_DOOMWAD
	FS_RegisterFileSystemType(NULL, "wad", FSDWD_LoadArchive, true);
#endif
#ifdef PACKAGE_DZIP
	FS_RegisterFileSystemType(NULL, "dz", FSDZ_LoadArchive, false);
#endif
#ifdef PACKAGE_Q1PAK
	FS_RegisterFileSystemType(NULL, "pak", FSPAK_LoadArchive, true);
#if !defined(_WIN32) && !defined(ANDROID)
	/*for systems that have case sensitive paths, also include *.PAK */
	FS_RegisterFileSystemType(NULL, "PAK", FSPAK_LoadArchive, true);
#endif
#endif
#ifdef PACKAGE_PK3
	FS_RegisterFileSystemType(NULL, "pk3", FSZIP_LoadArchive, true);	//quake3's extension for zips
	FS_RegisterFileSystemType(NULL, "pk4", FSZIP_LoadArchive, true);	//quake4's extension for zips...
#ifdef Q2CLIENT
	FS_RegisterFileSystemType(NULL, "pkz", FSZIP_LoadArchive, true);	//q2pro uses a different extension
	FS_RegisterFileSystemType(NULL, "pkx", FSZIP_LoadArchive, true);	//q2xp naturally uses a different extension too... you'll be glad to know that yq2 uses pk3 instead. yay consistency - every engine uses something different!
#endif
	FS_RegisterFileSystemType(NULL, "iwd", FSZIP_LoadArchive, true);	//cod2's variation.
	FS_RegisterFileSystemType(NULL, "apk", FSZIP_LoadArchive, false);	//android package
	FS_RegisterFileSystemType(NULL, "zip", FSZIP_LoadArchive, false);	//regular zip file (don't automatically read from these, because it gets messy)
	FS_RegisterFileSystemType(NULL, "kpf", FSZIP_LoadArchive, true);	//regular zip file (don't automatically read from these, because it gets messy)
	FS_RegisterFileSystemType(NULL, "exe", FSZIP_LoadArchive, false);	//for self-extracting zips.
	FS_RegisterFileSystemType(NULL, "dll", FSZIP_LoadArchive, false);	//for plugin metas / self-extracting zips.
	FS_RegisterFileSystemType(NULL, "so", FSZIP_LoadArchive, false);	//for plugin metas / self-extracting zips.
#endif
	FS_RegisterFileSystemType(NULL, "pk3dir", VFSOS_OpenPath, true);	//used for git repos or whatever, to make packaging easier
}

#ifdef HAVE_DBUS
//I'm adding this code primarily for the flatpak builds. this means cmake only, and only when cmake detects the libs. we can also get away with statically linking the dbus lib too, so no dlopen mess, and its only linked in client builds.
#include <dbus/dbus.h>
struct openfile_s
{
	DBusConnection *conn;
	char *replypath;
};
static void FS_DBus_Poll(int iarg, void *vctx)
{	//kept ticking over via timers so we don't stall completely
	struct openfile_s *ctx = vctx;
	DBusMessage *msg;
	DBusMessageIter args, opts, dict, var, uris;

	dbus_connection_read_write(ctx->conn, 0);	//see if there's anything new, with no timeout so we don't lock up
	msg = dbus_connection_pop_message(ctx->conn);

	if (msg)
	{	//something came back!
		if (dbus_message_is_signal(msg, "org.freedesktop.portal.Request", "Response") &&
			!strcmp(dbus_message_get_path(msg), ctx->replypath))
		{	//okay, this is the one we're interested in.
			if (dbus_message_iter_init(msg, &args) && dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_UINT32)
			{
				//dbus_message_iter_get_basic(&dict, &response); //0 for success, 1 for user cancel, 2 for error.
				if (dbus_message_iter_next(&args) && dbus_message_iter_get_arg_type(&args) == DBUS_TYPE_ARRAY)
				{
					dbus_message_iter_recurse(&args, &opts);
					for (; dbus_message_iter_get_arg_type(&opts) == DBUS_TYPE_DICT_ENTRY; dbus_message_iter_next(&opts))
					{
						dbus_message_iter_recurse(&opts, &dict);
						if (dbus_message_iter_get_arg_type(&dict) == DBUS_TYPE_STRING)
						{
							const char *optname;
							dbus_message_iter_get_basic(&dict, &optname);
							if (dbus_message_iter_next(&dict) && dbus_message_iter_get_arg_type(&dict) == DBUS_TYPE_VARIANT)
							{
								dbus_message_iter_recurse(&dict, &var);
								//if (!strcmp(optname, "choices")) //array of string pairs
								//if (!strcmp(optname, "currentfilter"))
								if (!strcmp(optname, "uris") && dbus_message_iter_get_arg_type(&var) == DBUS_TYPE_ARRAY)
								{
									dbus_message_iter_recurse(&var, &uris);
									for (; dbus_message_iter_get_arg_type(&uris) == DBUS_TYPE_STRING; dbus_message_iter_next(&uris))
									{
										char fullname[MAX_OSPATH];
										vfsfile_t *f;
										const char *filename;
										dbus_message_iter_get_basic(&uris, &filename);
										if (Sys_ResolveFileURL(filename, strlen(filename), fullname, sizeof(fullname)))
										{
											f = VFSOS_Open(fullname, "rb");
											if (f)
											{
//												filename = COM_SkipPath(filename);
												Host_RunFile(filename,strlen(filename), f);
											}
										}
									}
								}
							}
						}
					}
				}
			}
			dbus_message_unref(msg);

			dbus_connection_close(ctx->conn);
			Z_Free((char*)ctx->replypath);
			return; //can stop requeing now
		}
		dbus_message_unref(msg);
	}

	Cmd_AddTimer(0.2, FS_DBus_Poll, 0, ctx, sizeof(*ctx));
}
void FS_OpenFilePicker_f(void)
{
	const char *parentwindow = "";
	const char *title = "All Files";
	DBusMessage *msg;
	DBusMessageIter args, opts;
	struct openfile_s ctx;
	DBusError err;
	DBusPendingCall *pending;

	// initialiset the errors
	dbus_error_init(&err);

	// connect to the system bus and check for errors
	ctx.conn = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
	if (dbus_error_is_set(&err))
	{
		fprintf(stderr, "Connection Error (%s)\n", err.message);
		dbus_error_free(&err);
	}
	if (!ctx.conn)
		return;

	// request our name on the bus
	dbus_bus_register(ctx.conn, &err);
	if (dbus_error_is_set(&err))
		dbus_error_free(&err);

	// create a new method call and check for errors
	msg = dbus_message_new_method_call(
				"org.freedesktop.portal.Desktop", // target for the method call
				"/org/freedesktop/portal/desktop", // object to call on
				"org.freedesktop.portal.FileChooser", // interface to call on
				"OpenFile"); // method name
	if (msg)
	{
		dbus_message_iter_init_append(msg, &args);
		dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &parentwindow);
		dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &title);
		dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &opts);
		dbus_message_iter_close_container(&args, &opts);

		//make sure we get the response (without getting any races)
		dbus_bus_add_match(ctx.conn, "type='signal'"
							/*",path='/org/freedesktop/portal/desktop/request/UNIQUE/TOKEN'"*/
							",interface='org.freedesktop.portal.Request'"
							",member='Response'", &err);

		//now send our request
		if (!dbus_connection_send_with_reply (ctx.conn, msg, &pending, -1) || !pending)
		{
			dbus_message_unref(msg);
			msg = NULL;
		}
		else
		{
			dbus_message_unref(msg);
			dbus_connection_flush(ctx.conn);
			dbus_pending_call_block(pending);

			//see if we got a reply
			msg = dbus_pending_call_steal_reply(pending);
			dbus_pending_call_unref(pending);
		}
	}
	if (msg)
	{	//read the object path from the response.
		if (dbus_message_iter_init(msg, &args) && dbus_message_iter_get_arg_type(&args))
		{
			dbus_message_iter_get_basic(&args, &ctx.replypath);
			ctx.replypath = Z_StrDup(ctx.replypath);
		}

		dbus_message_unref(msg);
	}

	if (dbus_error_is_set(&err))
		dbus_error_free(&err);	//oops?
	if (ctx.replypath)
	{	//we sent a request, got a valid response... now we need the actual final result... which may take a while...
		Cmd_AddTimer(0.2, FS_DBus_Poll, 0, &ctx, sizeof(ctx));
		return;
	}

	Z_Free(ctx.replypath);
	dbus_connection_close(ctx.conn);
}
#endif
