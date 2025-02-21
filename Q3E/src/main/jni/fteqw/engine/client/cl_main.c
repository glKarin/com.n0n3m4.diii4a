/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// cl_main.c  -- client main loop

#include "quakedef.h"
#include "winquake.h"
#include <sys/types.h>
#include "netinc.h"
#include "cl_master.h"
#include "cl_ignore.h"
#include "shader.h"
#include "vr.h"
#include <ctype.h>
// callbacks
void QDECL CL_Sbar_Callback(struct cvar_s *var, char *oldvalue);
#ifdef NQPROT
void QDECL Name_Callback(struct cvar_s *var, char *oldvalue);
#else
#define Name_Callback NULL
#endif
void GnuTLS_Shutdown(void);

static void CL_ForceStopDownload (qboolean finish);

// we need to declare some mouse variables here, because the menu system
// references them even when on a unix system.

qboolean	noclip_anglehack;		// remnant from old quake
int startuppending;
extern int	r_blockvidrestart;

void Host_FinishLoading(void);


cvar_t	cl_crypt_rcon = CVARFD("cl_crypt_rcon", "1", CVAR_ARCHIVE|CVAR_NOTFROMSERVER, "Controls whether to send a hash instead of sending your rcon password as plain-text. Set to 1 for security, or 0 for backwards compatibility.\nYour command and any responses will still be sent as plain text.\nInstead, it is recommended to use rcon ONLY via dtls/tls/wss connections.");	//CVAR_NOTFROMSERVER prevents evil servers from degrading it to send plain-text passwords.
cvar_t	rcon_password = CVARF("rcon_password", "", CVAR_NOUNSAFEEXPAND);

cvar_t	rcon_address = CVARF("rcon_address", "", CVAR_NOUNSAFEEXPAND);

cvar_t	cl_timeout = CVAR("cl_timeout", "60");

cvar_t	cl_shownet = CVARD("cl_shownet","0", "Debugging var. 0 shows nothing. 1 shows incoming packet sizes. 2 shows individual messages. 3 shows entities too.");	// can be 0, 1, or 2

cvar_t	cl_disconnectreason = CVARAFD("_cl_disconnectreason", "", /*q3*/"com_errorMessage", CVAR_NOSAVE, "This cvar contains the reason for the last disconnection, so that mod menus can know why things failed.");

cvar_t	cl_pure		= CVARD("cl_pure", "0", "0=standard quake rules.\n1=clients should prefer files within packages present on the server.\n2=clients should use *only* files within packages present on the server.\nDue to quake 1.01/1.06 differences, a setting of 2 is only reliable with total conversions.\nIf sv_pure is set, the client will prefer the highest value set.");
cvar_t	cl_sbar		= CVARFC("cl_sbar", "0", CVAR_ARCHIVE, CL_Sbar_Callback);
cvar_t	cl_hudswap	= CVARF("cl_hudswap", "0", CVAR_ARCHIVE);
cvar_t	cl_maxfps	= CVARFD("cl_maxfps", "250", CVAR_ARCHIVE, "Sets the maximum allowed framerate. If you're using vsync or want to uncap framerates entirely then you should probably set this to 0. Set cl_yieldcpu 0 if you're trying to benchmark.");
static cvar_t	cl_maxfps_slop	= CVARFD("cl_maxfps_slop", "3", CVAR_ARCHIVE, "If a frame is delayed (eg because of poor system timer precision), this is how much sooner to pretend the frame happened (in milliseconds). If it is set too low then the average framerate will drop below the target, while too high may result in excessively fast frames.");
static cvar_t	cl_idlefps	= CVARAFD("cl_idlefps", "60", "cl_maxidlefps"/*dp*/, CVAR_ARCHIVE, "This is the maximum framerate to attain while idle/paused/unfocused.");
cvar_t	cl_yieldcpu = CVARFD("cl_yieldcpu", "1", CVAR_ARCHIVE, "Attempt to yield between frames. This can resolve issues with certain drivers and background software, but can mean less consistant frame times. Will reduce power consumption/heat generation so should be set on laptops or similar (over-hot/battery powered) devices.");
cvar_t	cl_nopext	= CVARF("cl_nopext", "0", CVAR_ARCHIVE);
static cvar_t	cl_pext_mask = CVAR("cl_pext_mask", "0xffffffff");
cvar_t	cl_nolerp	= CVARD("cl_nolerp", "0", "Disables interpolation. If set, missiles/monsters will be show exactly what was last received, which will be jerky. Does not affect players. A value of 2 means 'interpolate only in single-player/coop'.");
#ifdef NQPROT
cvar_t	cl_nolerp_netquake = CVARD("cl_nolerp_netquake", "0", "Disables interpolation when connected to an NQ server. Does affect players, even the local player. You probably don't want to set this.");
static cvar_t	cl_fullpitch_nq = CVARAFD("cl_fullpitch", "0", "pq_fullpitch", CVAR_SEMICHEAT, "When set, attempts to unlimit the default view pitch. Note that some servers will screw over your angles if you use this, resulting in terrible gameplay, while some may merely clamp your angle serverside. This is also considered a cheat in quakeworld, ^1so this will not function there^7. For the equivelent in quakeworld, use serverinfo minpitch+maxpitch instead, which applies to all players fairly.");
#endif
static cvar_t	cl_vrui_force = CVARD("cl_vrui_force", "0", "Force the use of VR UIs, even with no VR headset active.");
cvar_t	cl_vrui_lock = CVARD("cl_vrui_lock", "1", "Controls how the UI is positioned when using VR/XR. 0: Repositioned infront of the head when the console/menus are toggled. 1: Locked infront of the reference position (which may require the user to turn around to find it).");
cvar_t	*hud_tracking_show;
cvar_t	*hud_miniscores_show;
extern cvar_t net_compress;

cvar_t	cl_defaultport		= 
	#ifdef GAME_DEFAULTPORT	//remove the confusing port alias if we're running as a TC, as well as info about irrelevant games.
		CVARFD("cl_defaultport", STRINGIFY(PORT_DEFAULTSERVER),			 0, "The default port used to connect to servers.")
	#else
		CVARAFD("cl_defaultport", STRINGIFY(PORT_DEFAULTSERVER), "port", 0, "The default port used to connect to servers."
			"\nQW: "STRINGIFY(PORT_QWSERVER)
			", NQ: "STRINGIFY(PORT_NQSERVER)
			", Q2: "STRINGIFY(PORT_Q2SERVER)
			", Q3: "STRINGIFY(PORT_Q3SERVER)
		"."
		)
	#endif
	;

cvar_t	cfg_save_name = CVARFD("cfg_save_name", "fte", CVAR_ARCHIVE|CVAR_NOTFROMSERVER, "This is the config name that is saved by default when no argument is specified.");

cvar_t	cl_splitscreen = CVARD("cl_splitscreen", "0", "Enables splitscreen support. See also: allow_splitscreen, in_rawinput*, the \"p\" command.");

cvar_t	lookspring = CVARFD("lookspring","0", CVAR_ARCHIVE, "Recentre the camera when the mouse-look is released.");
cvar_t	lookstrafe = CVARFD("lookstrafe","0", CVAR_ARCHIVE, "Mouselook enables mouse strafing.");
cvar_t	sensitivity = CVARF("sensitivity","10", CVAR_ARCHIVE);

cvar_t cl_staticsounds = CVARF("cl_staticsounds", "1", CVAR_ARCHIVE);

cvar_t	m_pitch = CVARF("m_pitch","0.022", CVAR_ARCHIVE);
cvar_t	m_yaw = CVARF("m_yaw","0.022", CVAR_ARCHIVE);
cvar_t	m_forward = CVARF("m_forward","1", CVAR_ARCHIVE);
cvar_t	m_side = CVARF("m_side","0.8", CVAR_ARCHIVE);

cvar_t	cl_lerp_maxinterval = CVARD("cl_lerp_maxinterval", "0.3", "Maximum interval between keyframes, in seconds. Larger values can result in entities drifting very slowly when they move sporadically.");
cvar_t	cl_lerp_maxdistance = CVARD("cl_lerp_maxdistance", "200", "Maximum distance that an entity may move between snapshots without being considered as having teleported.");
cvar_t	cl_lerp_players = CVARD("cl_lerp_players", "0", "Set this to make other players smoother, though it may increase effective latency. Affects only QuakeWorld.");
cvar_t	cl_predict_players			= CVARD("cl_predict_players", "1", "Clear this cvar to see ents exactly how they are on the server.");
cvar_t	cl_predict_players_frac		= CVARD("cl_predict_players_frac", "0.9", "How much of other players to predict. Values less than 1 will help minimize overruns.");
cvar_t	cl_predict_players_latency	= CVARD("cl_predict_players_latency", "1.0", "Push the player back according to your latency, to give a smooth consistent simulation of the server.");
cvar_t	cl_predict_players_nudge	= CVARD("cl_predict_players_nudge", "0.02", "An extra nudge of time, to cover video latency.");
cvar_t	cl_solid_players = CVARD("cl_solid_players", "1", "Consider other players as solid for player prediction.");
cvar_t	cl_noblink = CVARD("cl_noblink", "0", "Disable the ^^b text blinking feature.");
cvar_t	cl_servername = CVARFD("cl_servername", "", CVAR_NOSET, "The hostname of the last server you connected to");
cvar_t	cl_serveraddress = CVARD("cl_serveraddress", "none", "The address of the last server you connected to");
cvar_t	qtvcl_forceversion1 = CVAR("qtvcl_forceversion1", "0");
cvar_t	qtvcl_eztvextensions = CVAR("qtvcl_eztvextensions", "1");

cvar_t	record_flush = CVARD("record_flush", "0", "If set, explicitly flushes demo data to disk while recording. This may be inefficient, depending on how your operating system is configured.");
cvar_t cl_demospeed = CVARF("cl_demospeed", "1", 0);
cvar_t	cl_demoreel = CVARFD("cl_demoreel", "0", CVAR_SAVE, "When enabled, the engine will begin playing a demo loop on startup.");

cvar_t cl_loopbackprotocol = CVARD("cl_loopbackprotocol", "qw", "Which protocol to use for single-player/the internal client. Should be one of: qw, qwid, nqid, nq, fitz, bjp3, dp6, dp7, auto. If 'auto', will use qw protocols for qw mods, and nq protocols for nq mods.");
#ifdef FTE_TARGET_WEB
static cvar_t cl_verify_urischeme = CVARAFD("cl_verify_urischeme", "2", "cl_verify_qwprotocol"/*ezquake, inappropriate for misc schemes*/, CVAR_NOSAVE/*checked at startup, so its only really default.cfg that sets it*/, "0: Do nothing.\n1: Check whether our protocol scheme is registered and prompt the user to register associations.\n2: Always re-register on every startup, without prompting. Sledgehammer style.");
#else
static cvar_t cl_verify_urischeme = CVARAFD("cl_verify_urischeme", "0", "cl_verify_qwprotocol"/*ezquake, inappropriate for misc schemes*/, CVAR_NOSAVE/*checked at startup, so its only really default.cfg that sets it*/, "0: Do nothing.\n1: Check whether our protocol scheme is registered and prompt the user to register associations.\n2: Always re-register on every startup, without prompting. Sledgehammer style.");
#endif


cvar_t	cl_threadedphysics = CVARD("cl_threadedphysics", "0", "When set, client input frames are generated and sent on a worker thread");

#ifdef QUAKESPYAPI
cvar_t  localid = SCVAR("localid", "");
static qboolean allowremotecmd = true;
#endif

cvar_t	r_drawflame = CVARD("r_drawflame", "1", "Set to -1 to disable ALL static entities. Set to 0 to disable only wall torches and standing flame. Set to 1 for everything drawn as normal.");

qboolean forcesaveprompt;

extern int			total_loading_size, current_loading_size, loading_stage;

//
// info mirrors
//
cvar_t	password	= CVARAF("password",	"",	"pq_password", CVAR_USERINFO | CVAR_NOUNSAFEEXPAND); //this is parhaps slightly dodgy... added pq_password alias because baker seems to be using this for user accounts.
cvar_t	spectator	= CVARF("spectator",	"",			CVAR_USERINFO);
cvar_t	name		= CVARFC("name",		"Player",	CVAR_ARCHIVE | CVAR_USERINFO, Name_Callback);
cvar_t	team		= CVARF("team",			"",			CVAR_ARCHIVE | CVAR_USERINFO);
cvar_t	skin		= CVARAF("skin",		"",			"_cl_playerskin"/*dp*/, CVAR_ARCHIVE | CVAR_USERINFO);
cvar_t	model		= CVARAF("model",		"",			"_cl_playermodel"/*dp*/, CVAR_ARCHIVE | CVAR_USERINFO);
cvar_t	topcolor	= CVARF("topcolor",		"13",			CVAR_ARCHIVE | CVAR_USERINFO);
cvar_t	bottomcolor	= CVARF("bottomcolor",	"12",			CVAR_ARCHIVE | CVAR_USERINFO);
cvar_t	rate		= CVARAFD("rate",		"30000"/*"6480"*/,		"_cl_rate"/*dp*/, CVAR_ARCHIVE | CVAR_USERINFO, "A rough measure of the bandwidth to try to use while playing. Too high a value may result in 'buffer bloat'.");
static cvar_t	drate		= CVARFD("drate",		"3000000",	CVAR_ARCHIVE | CVAR_USERINFO, "A rough measure of the bandwidth to try to use while downloading (in bytes per second).");		// :)
static cvar_t	noaim		= CVARF("noaim",		"",			CVAR_ARCHIVE | CVAR_USERINFO);
cvar_t	msg			= CVARFD("msg",			"1",		CVAR_ARCHIVE | CVAR_USERINFO, "Filter console prints/messages. Only functions on QuakeWorld servers. 0=pickup messages. 1=death messages. 2=critical messages. 3=chat.");
static cvar_t	b_switch	= CVARF("b_switch",		"",			CVAR_ARCHIVE | CVAR_USERINFO);
static cvar_t	w_switch	= CVARF("w_switch",		"",			CVAR_ARCHIVE | CVAR_USERINFO);
#ifdef HEXEN2
cvar_t	cl_playerclass=CVARF("cl_playerclass","",		CVAR_ARCHIVE | CVAR_USERINFO);
#endif
#ifdef Q2CLIENT
static cvar_t	hand		= CVARFD("hand",		"",			CVAR_ARCHIVE | CVAR_USERINFO, "For gamecode to know which hand to fire from.\n0: Right\n1: Left\n2: Chest");
#endif
cvar_t	cl_nofake	= CVARD("cl_nofake",		"2", "value 0: permits \\r chars in chat messages\nvalue 1: blocks all \\r chars\nvalue 2: allows \\r chars, but only from teammates");
cvar_t	cl_chatsound	= CVAR("cl_chatsound","1");
cvar_t	cl_enemychatsound	= CVAR("cl_enemychatsound", "misc/talk.wav");
cvar_t	cl_teamchatsound	= CVAR("cl_teamchatsound", "misc/talk.wav");

cvar_t	r_torch					= CVARFD("r_torch",	"0",	CVAR_CHEAT, "Generate a dynamic light at the player's position.");
cvar_t	r_rocketlight			= CVARFC("r_rocketlight",	"1", CVAR_ARCHIVE, Cvar_Limiter_ZeroToOne_Callback);
cvar_t	r_lightflicker			= CVAR("r_lightflicker",	"1");
cvar_t	cl_r2g					= CVARFD("cl_r2g",	"0", CVAR_ARCHIVE, "Uses progs/grenade.mdl instead of progs/missile.mdl when 1.");
cvar_t	r_powerupglow			= CVAR("r_powerupglow", "1");
cvar_t	v_powerupshell			= CVARF("v_powerupshell", "0", CVAR_ARCHIVE);
cvar_t	cl_gibfilter			= CVARF("cl_gibfilter", "0", CVAR_ARCHIVE);
cvar_t	cl_deadbodyfilter		= CVARF("cl_deadbodyfilter", "0", CVAR_ARCHIVE);

cvar_t  cl_gunx					= CVAR("cl_gunx", "0");
cvar_t  cl_guny					= CVAR("cl_guny", "0");
cvar_t  cl_gunz					= CVAR("cl_gunz", "0");

cvar_t  cl_gunanglex			= CVAR("cl_gunanglex", "0");
cvar_t  cl_gunangley			= CVAR("cl_gunangley", "0");
cvar_t  cl_gunanglez			= CVAR("cl_gunanglez", "0");

cvar_t	cl_proxyaddr			= CVAR("cl_proxyaddr", "");
cvar_t	cl_sendguid				= CVARD("cl_sendguid", "", "Send a randomly generated 'globally unique' id to servers, which can be used by servers for score rankings and stuff. Different servers will see different guids. Delete the 'qkey' file in order to appear as a different user.\nIf set to 2, all servers will see the same guid. Be warned that this can show other people the guid that you're using.");
cvar_t	cl_downloads			= CVARAFD("cl_downloads", "1", /*q3*/"cl_allowDownload", CVAR_NOTFROMSERVER|CVAR_ARCHIVE, "Allows you to block all automatic downloads.");
cvar_t	cl_download_csprogs		= CVARFD("cl_download_csprogs", "1", CVAR_NOTFROMSERVER|CVAR_ARCHIVE, "Download updated client gamecode if available. Warning: If you clear this to avoid downloading vm code, you should also clear cl_download_packages.");
cvar_t	cl_download_redirection	= CVARFD("cl_download_redirection", "2", CVAR_NOTFROMSERVER|CVAR_ARCHIVE, "Follow download redirection to download packages instead of individual files. Also allows the server to send nearly arbitary download commands.\n2: allows redirection only to named packages files (and demos/*.mvd), which is a bit safer.");
cvar_t	cl_download_packages	= CVARFD("cl_download_packages", "1", CVAR_NOTFROMSERVER, "0=Do not download packages simply because the server is using them. 1=Download and load packages as needed (does not affect games which do not use this package). 2=Do download and install permanently (use with caution!)");
cvar_t	requiredownloads		= CVARAFD("cl_download_wait", "1", /*old*/"requiredownloads", CVAR_ARCHIVE, "0=join the game before downloads have even finished (might be laggy). 1=wait for all downloads to complete before joining.");
cvar_t	mod_precache			= CVARD("mod_precache","1", "Controls when models are loaded.\n0: Load them only when they're actually needed.\n1: Load them upfront.\n2: Lazily load them to shorten load times at the risk of brief stuttering during only the start of the map.");

cvar_t	cl_muzzleflash			= CVAR("cl_muzzleflash", "1");

cvar_t	gl_simpleitems			= CVARFD("gl_simpleitems", "0", CVAR_ARCHIVE, "Replace models with simpler sprites.");
cvar_t	cl_item_bobbing			= CVARFD("cl_model_bobbing", "0", CVAR_ARCHIVE, "Makes rotating pickup items bob too.");
cvar_t	cl_countpendingpl		= CVARD("cl_countpendingpl", "0", "If set to 1, packet loss percentages will show packets still in transit as lost, even if they might still be received.");

cvar_t	cl_standardchat			= CVARFD("cl_standardchat", "0", CVAR_ARCHIVE, "Disables auto colour coding in chat messages.");
cvar_t	msg_filter				= CVARD("msg_filter", "0", "Filter out chat messages: 0=neither. 1=broadcast chat. 2=team chat. 3=all chat.");
cvar_t	msg_filter_frags		= CVARD("msg_filter_frags", "0", "Prevents frag messages from appearing on the console.");
cvar_t	msg_filter_pickups		= CVARD("msg_filter_pickups", "0", "Prevents pickup messages from appearing on the console. This would normally be filtered by 'msg 1', but nq servers cannot respect that (nor nq mods running in qw servers).");
cvar_t  cl_standardmsg			= CVARFD("cl_standardmsg", "0", CVAR_ARCHIVE, "Disables auto colour coding in console prints.");
cvar_t  cl_parsewhitetext		= CVARD("cl_parsewhitetext", "1", "When parsing chat messages, enable support for messages like: red{white}red");

cvar_t	cl_dlemptyterminate		= CVARD("cl_dlemptyterminate", "1", "Terminate downloads when reciving an empty download packet. This should help work around buggy mvdsv servers.");

static void QDECL Cvar_CheckServerInfo(struct cvar_s *var, char *oldvalue)
{	//values depend upon the serverinfo, so reparse for overrides.
	CL_CheckServerInfo();
}

#define RULESETADVICE " You should not normally change this cvar from its permissive default, instead impose limits on yourself only through the 'ruleset' cvar."
cvar_t	ruleset_allow_playercount			= CVARD("ruleset_allow_playercount", "1", "Specifies whether teamplay triggers that count nearby players are allowed in the current ruleset."RULESETADVICE);
cvar_t	ruleset_allow_frj					= CVARD("ruleset_allow_frj", "1", "Specifies whether Forward-Rocket-Jump scripts are allowed in the current ruleset. If 0, limits on yaw speed will be imposed so they cannot be scripted."RULESETADVICE);
												//FIXME: rename ruleset_allow_frj to allow_scripts to match ezquake - 0: block multiple commands in binds, 1: cap angle speed changes, 2: vanilla quake
cvar_t	ruleset_allow_semicheats			= CVARD("ruleset_allow_semicheats", "1", "If 0, this blocks a range of cvars that are marked as semi-cheats. Such cvars will be locked to their empty/0 value."RULESETADVICE);
cvar_t	ruleset_allow_packet				= CVARD("ruleset_allow_packet", "1", "If 0, network packets sent via the 'packet' command will be blocked. This makes scripting timers a little harder."RULESETADVICE);
cvar_t	ruleset_allow_particle_lightning	= CVARD("ruleset_allow_particle_lightning", "1", "A setting of 0 blocks using the particle system to replace lightning gun trails. This prevents making the trails thinner thus preventing them from obscuring your view of your enemies."RULESETADVICE);
cvar_t	ruleset_allow_overlongsounds		= CVARD("ruleset_allow_overlong_sounds", "1", "A setting of 0 will block the use of extra-long pickup sounds as item respawn timers."RULESETADVICE);
cvar_t	ruleset_allow_larger_models			= CVARD("ruleset_allow_larger_models", "1", "Enforces a maximum bounds limit on models, to prevent the use of additional spikes attached to the model from being used as a kind of wallhack."RULESETADVICE);
cvar_t	ruleset_allow_modified_eyes			= CVARD("ruleset_allow_modified_eyes", "0", "When 0, completely hides progs/eyes.mdl if it is not strictly identical to vanilla quake."RULESETADVICE);
cvar_t	ruleset_allow_sensitive_texture_replacements = CVARD("ruleset_allow_sensitive_texture_replacements", "1", "Allows the replacement of certain model textures (as well as the models themselves). This prevents adding extra fullbrights to make them blatently obvious."RULESETADVICE);
cvar_t	ruleset_allow_localvolume			= CVARD("ruleset_allow_localvolume", "1", "Allows the use of the snd_playersoundvolume cvar. Muting your own sounds can make it easier to hear where your opponent is."RULESETADVICE);
cvar_t  ruleset_allow_shaders				= CVARFD("ruleset_allow_shaders", "1", CVAR_SHADERSYSTEM, "When 0, this completely disables the use of external shader files, preventing custom shaders from being used for wallhacks."RULESETADVICE);
cvar_t  ruleset_allow_watervis				= CVARFCD("ruleset_allow_watervis", "1", CVAR_SHADERSYSTEM, Cvar_CheckServerInfo, "When 0, this enforces ugly opaque water."RULESETADVICE);
cvar_t  ruleset_allow_fbmodels				= CVARFD("ruleset_allow_fbmodels", "0", CVAR_SHADERSYSTEM, "When 1, allows all models to be displayed fullbright, completely ignoring the lightmaps. This feature exists only for parity with ezquake's defaults."RULESETADVICE);
cvar_t  ruleset_allow_triggers				= CVARAD("ruleset_allow_triggers", "1", "tp_msgtriggers"/*ez*/, "When 0, blocks the use of msg_trigger checks."RULESETADVICE);

extern cvar_t cl_hightrack;
extern cvar_t	vid_renderer;

char cl_screengroup[] = "Screen options";
char cl_controlgroup[] = "client operation options";
char cl_inputgroup[] = "client input controls";
char cl_predictiongroup[] = "Client side prediction";


client_static_t	cls;
client_state_t	cl;

// alot of this should probably be dynamically allocated
entity_state_t	*cl_baselines;
static_entity_t *cl_static_entities;
unsigned int    cl_max_static_entities;
lightstyle_t	*cl_lightstyle;
size_t			cl_max_lightstyles;
dlight_t		*cl_dlights;
size_t	cl_maxdlights; /*size of cl_dlights array*/

int cl_baselines_count;
size_t rtlights_first, rtlights_max;

// refresh list
// this is double buffered so the last frame
// can be scanned for oldorigins of trailing objects
int				cl_numvisedicts;
int				cl_maxvisedicts;
entity_t		*cl_visedicts;
int				cl_framecount;

scenetris_t		*cl_stris;
vecV_t			*fte_restrict cl_strisvertv;
vec4_t			*fte_restrict cl_strisvertc;
vec2_t			*fte_restrict cl_strisvertt;
index_t			*fte_restrict cl_strisidx;
unsigned int cl_numstrisidx;
unsigned int cl_maxstrisidx;
unsigned int cl_numstrisvert;
unsigned int cl_maxstrisvert;
unsigned int cl_numstris;
unsigned int cl_maxstris;

static struct
{
	qboolean			trying;
	qboolean			istransfer;		//ignore the user's desired server (don't change connect.adr).
	qboolean			resolving;
	int					numadr;
	int					nextadr;
	netadr_t			adr[8];			//addresses that we're trying to transfer to, one entry per dns result, eg both ::1 AND 127.0.0.1
	int					protocol;		//nq/qw/q2/q3. guessed based upon server replies
	int					subprotocol;	//the monkeys are trying to eat me.
	struct
	{
		//flags
		unsigned int		fte1;
		unsigned int		fte2;
		unsigned int		ez1;

		int					mtu;		//0 for unsupported, otherwise a size.
		unsigned int		compresscrc;//0 for unsupported, otherwise the peer's hash

		unsigned char		guidsalt[64];//server->client (for servers that want to share guids between themselves, with noticably lower security)
	} ext;
	int					qport;
	int					challenge;		//tracked as part of guesswork based upon what replies we get.
	int					clchallenge;	//generated by the client, to ensure the response wasn't spoofed/spammed.
	double				time;			//for connection retransmits
	qboolean			clogged;		//ignore time...
	enum coninfomode_e
	{
		CIM_DEFAULT,	//sends both a qw getchallenge and nq connect (also with postfixed getchallenge so modified servers can force getchallenge)
		CIM_NQONLY,		//disables getchallenge (so fte servers treat us as an nq client). should not be used for dpp7 servers.
		CIM_QEONLY,		//forces dtls and uses a different nq netchan version
		CIM_Q2EONLY,	//forces dtls and uses a different nq netchan version
	}					mode;
	enum coninfospec_e
	{
		CIS_DEFAULT,	//default
		CIS_JOIN,		//force join
		CIS_OBSERVE,	//force observe
	}					spec;
	int					defaultport;
	int					tries;			//increased each try, every fourth trys nq connect packets.
	unsigned char		guid[64];		//client->server guid (so doesn't change with transfers)

	struct dtlspeercred_s peercred;
} connectinfo;

qboolean	nomaster;

double		oldrealtime;			// last frame run
int			host_framecount;

qbyte		*host_basepal;
qbyte		*h2playertranslations;

cvar_t	host_speeds = CVAR("host_speeds","0");		// set for running times

int			fps_count;
qboolean	forcesaveprompt;

jmp_buf 	host_abort;

void Master_Connect_f (void);

char emodel_name[] =
	{ 'e' ^ 0xff, 'm' ^ 0xff, 'o' ^ 0xff, 'd' ^ 0xff, 'e' ^ 0xff, 'l' ^ 0xff, 0 };
char pmodel_name[] =
	{ 'p' ^ 0xff, 'm' ^ 0xff, 'o' ^ 0xff, 'd' ^ 0xff, 'e' ^ 0xff, 'l' ^ 0xff, 0 };
char prespawn_name[] =
	{ 'p'^0xff, 'r'^0xff, 'e'^0xff, 's'^0xff, 'p'^0xff, 'a'^0xff, 'w'^0xff, 'n'^0xff,
		' '^0xff, '%'^0xff, 'i'^0xff, ' '^0xff, '0'^0xff, ' '^0xff, '%'^0xff, 'i'^0xff, 0 };
char modellist_name[] =
	{ 'm'^0xff, 'o'^0xff, 'd'^0xff, 'e'^0xff, 'l'^0xff, 'l'^0xff, 'i'^0xff, 's'^0xff, 't'^0xff,
		' '^0xff, '%'^0xff, 'i'^0xff, ' '^0xff, '%'^0xff, 'i'^0xff, 0 };
char soundlist_name[] =
	{ 's'^0xff, 'o'^0xff, 'u'^0xff, 'n'^0xff, 'd'^0xff, 'l'^0xff, 'i'^0xff, 's'^0xff, 't'^0xff,
		' '^0xff, '%'^0xff, 'i'^0xff, ' '^0xff, '%'^0xff, 'i'^0xff, 0 };

vrui_t vrui;
void VRUI_SnapAngle(void)
{
//	VectorCopy(cl.playerview[0].viewangles, vrui.angles);
	vrui.angles[0] = 0;
	if (cl_vrui_lock.ival)	//if its locked, then its locked infront of the unpitched player entity
		vrui.angles[1] = cl.playerview[0].viewangles[1];
	else	//otherwise its moved to infront of the player's head any time the menu is displayed.
		vrui.angles[1] = cl.playerview[0].aimangles[1];
	vrui.angles[2] = 0;
}

void CL_UpdateWindowTitle(void)
{
	if (VID_SetWindowCaption)
	{
		if (cl.windowtitle)
		{	//gamecode wanted some explicit title.
			VID_SetWindowCaption(cl.windowtitle);
			return;
		}
		else
		{
			char title[2048];
			switch (cls.state)
			{
			default:
#ifdef HAVE_SERVER
				if (sv.state)
					Q_snprintfz(title, sizeof(title), "%s: %s", fs_gamename.string, svs.name);
				else
#endif
				if (cls.demoplayback)
					Q_snprintfz(title, sizeof(title), "%s: %s", fs_gamename.string, cls.lastdemoname);
				else
					Q_snprintfz(title, sizeof(title), "%s: %s", fs_gamename.string, cls.servername);
				break;
			case ca_disconnected:
				if (CSQC_UnconnectedOkay(false))	//pure csqc mods can have a world model and yet be disconnected. we don't really know what the current map should be called though.
					Q_snprintfz(title, sizeof(title), "%s", fs_gamename.string);
				else
					Q_snprintfz(title, sizeof(title), "%s: disconnected", fs_gamename.string);
				break;
			}
			VID_SetWindowCaption(title);
		}
	}
}

#ifdef __GLIBC__
#include <malloc.h>
#endif
void CL_MakeActive(char *gamename)
{
	extern int fs_finds;
	if (fs_finds)
	{
		Con_DPrintf("%i additional FS searches\n", fs_finds);
		fs_finds = 0;
	}
	cl.matchgametimestart = 0;
	cls.state = ca_active;

	//this might be expensive, don't count any of this as time spent *playing* the demo. this avoids skipping the first $LOADDURATION seconds.
	cl.stillloading = true;

	//kill sounds left over from the last map.
	S_Purge(true);

	//kill models left over from the last map.
	Mod_Purge(MP_MAPCHANGED);

	//and reload shaders now if needed (this was blocked earlier)
	Shader_DoReload();

	//and now free any textures that were not still needed.
	Image_Purge();

	SCR_EndLoadingPlaque();
	CL_UpdateWindowTitle();

#ifdef MVD_RECORDING
	if (sv_demoAutoRecord.ival && !sv.mvdrecording && !cls.demorecording && !cls.demoplayback && MVD_CheckSpace(false))
	{	//don't auto-record if we're already recording... or playing a different demo.
		extern cvar_t sv_demoAutoPrefix;
		char timestamp[64];
		time_t tm = time(NULL);
		strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", localtime(&tm));
		Cbuf_AddText(va("record %s%s%s%s_%s\n", sv_demoDir.string, *sv_demoDir.string?"/":"", sv_demoAutoPrefix.string, host_mapname.string, timestamp), RESTRICT_LOCAL);
	}
#endif

	TP_ExecTrigger("f_begin", true);
	if (cls.demoplayback)
		TP_ExecTrigger("f_spawndemo", true);
	else
		TP_ExecTrigger("f_spawn", false);

#ifdef __GLIBC__
	malloc_trim(0);
#endif
}
/*
==================
CL_Quit_f
==================
*/
void CL_Quit_f (void)
{
	if (!host_initialized)
		return;

	if (forcesaveprompt && strcmp(Cmd_Argv(1), "force"))
	{
		forcesaveprompt = false;
		if (Cmd_Exists("menu_quit"))
		{
			Cmd_ExecuteString("menu_quit", RESTRICT_LOCAL);
			return;
		}
	}

	TP_ExecTrigger("f_quit", true);
	Cbuf_Execute();
/*
#ifdef HAVE_SERVER
	if (!isDedicated)
#endif
	{
		M_Menu_Quit_f ();
		return;
	}*/
	Sys_Quit ();
}

#ifdef NQPROT
void CL_ConnectToDarkPlaces(char *challenge, netadr_t *adr)
{
	char	data[2048];
	cls.fteprotocolextensions = 0;
	cls.fteprotocolextensions2 = 0;
	cls.ezprotocolextensions1 = 0;

	connectinfo.time = realtime;	// for retransmit requests

	Q_snprintfz(data, sizeof(data), "%c%c%c%cconnect\\protocol\\darkplaces "STRINGIFY(NQ_NETCHAN_VERSION)"\\protocols\\DP7 DP6 DP5 RMQ FITZ NEHAHRABJP2 NEHAHRABJP NEHAHRABJP3 QUAKE\\challenge\\%s\\name\\%s", 255, 255, 255, 255, challenge, name.string);

	NET_SendPacket (cls.sockets, strlen(data), data, adr);

	cl.splitclients = 0;
}
#endif

void CL_SupportedFTEExtensions(unsigned int *pext1, unsigned int *pext2, unsigned int *ezpext1)
{
	unsigned int fteprotextsupported1 = Net_PextMask(PROTOCOL_VERSION_FTE1, false);
	unsigned int fteprotextsupported2 = Net_PextMask(PROTOCOL_VERSION_FTE2, false);
	unsigned int ezprotextsupported1 = Net_PextMask(PROTOCOL_VERSION_EZQUAKE1, false) & EZPEXT1_CLIENTADVERTISE;

	fteprotextsupported1 &= strtoul(cl_pext_mask.string, NULL, 16);
//	fteprotextsupported2 &= strtoul(cl_pext2_mask.string, NULL, 16);
//	ezprotextsupported1 &= strtoul(cl_ezpext1_mask.string, NULL, 16);

	if (cl_nopext.ival)
	{
		fteprotextsupported1 = 0;
		fteprotextsupported2 = 0;
		ezprotextsupported1 = 0;
	}

	*pext1 = fteprotextsupported1;
	*pext2 = fteprotextsupported2;
	*ezpext1 = ezprotextsupported1;
}

char *CL_GUIDString(netadr_t *adr)
{
	static qbyte buf[2048];
	static int buflen;
	qbyte digest[DIGEST_MAXSIZE];
	char serveraddr[256];
	void *ctx;

	if (!*cl_sendguid.string && *connectinfo.ext.guidsalt)
	{
		serveraddr[0] = '#';	//leading hash is to stop servers from being able to scrape from other servers (ones that don't use a custom/reproducible salt).
		Q_strncpyz(serveraddr+1, connectinfo.ext.guidsalt, sizeof(serveraddr)-1);
	}
	else if (cl_sendguid.ival == 2)
		*serveraddr = 0;
	else if (cl_sendguid.ival)
		NET_AdrToString(serveraddr, sizeof(serveraddr), adr);
	else
		return NULL;

	if (*connectinfo.guid && connectinfo.istransfer)
		return connectinfo.guid;

	if (!buflen)
	{
		vfsfile_t *f;
		f = FS_OpenVFS("qkey", "rb", FS_ROOT);
		if (f)
		{
			buflen = VFS_GETLEN(f);
			if (buflen > 2048)
				buflen = 2048;
			buflen = VFS_READ(f, buf, buflen);
			VFS_CLOSE(f);
		}
		if (buflen < 16)
		{
			buflen = sizeof(buf);
			if (!Sys_RandomBytes(buf, buflen))
			{
				int i;
				srand(time(NULL));
				for (i = 0; i < buflen; i++)
					buf[i] = rand() & 0xff;
			}
			f = FS_OpenVFS("qkey", "wb", FS_ROOT);
			if (f)
			{
				VFS_WRITE(f, buf, buflen);
				VFS_CLOSE(f);
			}
		}
	}

	ctx = alloca(hash_md4.contextsize);
	hash_md4.init(ctx);
	hash_md4.process(ctx, buf, buflen);
	hash_md4.process(ctx, serveraddr, strlen(serveraddr));
	hash_md4.terminate(digest, ctx);
	Base16_EncodeBlock(digest, hash_md4.digestsize, connectinfo.guid, sizeof(connectinfo.guid));
	return connectinfo.guid;
}

static void CL_ConnectAbort(const char *format, ...)
{	//stops trying to connect, doesn't affect the _current_ connection, so usable for transfers.
	va_list		argptr;
	char		reason[1024];

	if (format)
	{
		va_start (argptr, format);
		Q_vsnprintfz (reason, sizeof(reason), format,argptr);
		va_end (argptr);

		Cvar_Set(&cl_disconnectreason, reason);
		Con_Printf (CON_ERROR"%s\n", reason);
	}
#ifdef HAVE_DTLS
	while (connectinfo.numadr)
		NET_DTLS_Disconnect(cls.sockets, &connectinfo.adr[--connectinfo.numadr]);
#endif
	connectinfo.numadr = 0;
	SCR_EndLoadingPlaque();
	connectinfo.trying = false;

	if (format)
	{
		//try and force the menu to show again. this should force the disconnectreason to show.
		if (!Key_Dest_Has(kdm_console))
		{
#ifdef MENU_DAT
			if (!MP_Toggle(1))
#endif
				Menu_Prompt(NULL, NULL, reason, NULL, NULL, "Okay", true);
		}
	}
}


size_t Q2EX_UserInfoToString(char *infostring, size_t maxsize, const char **ignore, int seats)
{	//infoblobs are not a thing here. don't need to maintain sync objects - can't really do anything with them anyway.
	size_t k, r = 1, l;
	char *o = infostring;
	char *e = infostring?infostring + maxsize-1:infostring;
	int s;

	for (s = 0; s < seats; s++)
	{
		char *pf = s?va("_%i", s):"";
		size_t pfl = strlen(pf);
		infobuf_t *info = &cls.userinfo[s];
		for (k = 0; k < info->numkeys; k++)
		{
			if (ignore)
			{
				for (l = 0; ignore[l]; l++)
				{
					if (!strcmp(ignore[l], info->keys[k].name))
						break;
					else if (ignore[l][0] == '*' && !ignore[l][1] && *info->keys[k].name == '*')
						break;	//read-only
					else if (ignore[l][0] == '_' && !ignore[l][1] && *info->keys[k].name == '_')
						break;	//comment
				}
				if (ignore[l])
					continue;	//ignore when in the list
			}

			if (!info->keys[k].large)	//lower priorities don't bother with extended blocks. be sure to prioritise them explicitly. they'd just bug stuff out.
			{
				size_t knl = strlen(info->keys[k].name);
				size_t kvl = info->keys[k].size;
				r+= 1+knl+pfl+1+kvl;
				if (o + 1+knl+1+kvl >= e)
					continue;
				o[0] = '\\';
				memcpy(o+1, info->keys[k].name, knl);
				memcpy(o+1+knl, pf, pfl);
				o[1+knl+pfl] = '\\';
				memcpy(o+1+knl+pfl+1, info->keys[k].value, kvl);
				o[1+knl+pfl+1+kvl] = 0;

				o += 1+knl+pfl+1+kvl;
			}
		}
	}
	*o = 0;
	return r;
}

/*
=======================
CL_SendConnectPacket

called by CL_Connect_f and CL_CheckResend
======================
*/
static void CL_SendConnectPacket (netadr_t *to)
{
	extern cvar_t qport;
	netadr_t	addr;
	char	data[2048];
	char *info;
	double t1, t2;
	char *a;

// JACK: Fixed bug where DNS lookups would cause two connects real fast
//       Now, adds lookup time to the connect time.
//		 Should I add it to realtime instead?!?!

	if (!connectinfo.trying)
		return;

	if (cl_nopext.ival)	//imagine it's an unenhanced server
	{
		connectinfo.ext.compresscrc = 0;
	}

	if (connectinfo.protocol == CP_QUAKEWORLD)
	{
		int fteprotextsupported1=0;
		int fteprotextsupported2=0;
		int ezprotextsupported1=0;
		CL_SupportedFTEExtensions(&fteprotextsupported1, &fteprotextsupported2, &ezprotextsupported1);

		connectinfo.ext.fte1 &= fteprotextsupported1;
		connectinfo.ext.fte2 &= fteprotextsupported2;
		connectinfo.ext.ez1 &= ezprotextsupported1;
	}
#ifdef Q2CLIENT
	else if (connectinfo.protocol == CP_QUAKE2)
	{
		if (!(scr_fov.flags & CVAR_USERINFO))
		{	//q2 does server-controlled fov, so make sure the cvar is flagged properly.
			//FIXME: this hack needs better support, for dynamically switching between protocols without spamming too many cvars for other games.
			scr_fov.flags |= CVAR_USERINFO;
			Cvar_Set(&scr_fov, scr_fov.string);	//make sure the userinfo is set properly.
		}

		connectinfo.ext.fte1 &= (PEXT_MODELDBL|PEXT_SOUNDDBL|PEXT_SPLITSCREEN);
		connectinfo.ext.fte2 &= PEXT2_STUNAWARE;
		connectinfo.ext.ez1 &= 0;
	}
#endif
	else
	{
		connectinfo.ext.fte1 = 0;
		connectinfo.ext.fte2 = 0;
		connectinfo.ext.ez1 = 0;
	}

	t1 = Sys_DoubleTime ();

#ifdef HAVE_DTLS
	if (connectinfo.peercred.hash && net_enable_dtls.ival>0)
	{
		char cert[8192];
		char digest[DIGEST_MAXSIZE];
		int sz = NET_GetConnectionCertificate(cls.sockets, to, QCERT_PEERCERTIFICATE, cert, sizeof(cert));
		if (sz <= 0 || memcmp(connectinfo.peercred.digest, digest, CalcHash(connectinfo.peercred.hash, digest, sizeof(digest), cert, sz)))
		{	//FIXME: we may have already pinned the bad cert, which may cause issues when reconnecting without FP info later.
			if (NET_GetConnectionCertificate(cls.sockets, to, QCERT_ISENCRYPTED, NULL, 0)<0)
				CL_ConnectAbort ("Fingerprint specified, but server did not report any certificate\n");
			else
				CL_ConnectAbort ("Server certificate does not match specified fingerprint\n");
			return;
		}
	}
#endif

	if (!to)
	{
		to = &addr;
		if (!NET_StringToAdr (cls.servername, PORT_DEFAULTSERVER, to))
		{
			CL_ConnectAbort ("CL_SendConnectPacket: Bad server address \"%s\"\n", cls.servername);
			return;
		}
	}

	NET_AdrToString(data, sizeof(data), to);
	Cvar_ForceSet(&cl_serveraddress, data);
//	Info_SetValueForStarKey (cls.userinfo, "*ip", data, MAX_INFO_STRING);

	if (!NET_IsClientLegal(to))
	{
		CL_ConnectAbort("Illegal server address\n");
		return;
	}

	t2 = Sys_DoubleTime ();

	connectinfo.time = realtime+t2-t1;	// for retransmit requests

	//fixme: we shouldn't cycle these so much
	connectinfo.qport = qport.value;
	if (to->type != NA_LOOPBACK)
		Cvar_SetValue(&qport, (connectinfo.qport+1)&0xffff);

	if (connectinfo.protocol == CP_QUAKE2 && (connectinfo.subprotocol == PROTOCOL_VERSION_R1Q2 || connectinfo.subprotocol == PROTOCOL_VERSION_Q2PRO))
		connectinfo.qport &= 0xff;

#ifdef Q3CLIENT
	if (connectinfo.protocol == CP_QUAKE3)
	{	//q3 requires some very strange things.
		//cl.splitclients = 1;
		if (q3)
			q3->cl.SendConnectPacket(cls.sockets, to, connectinfo.challenge, connectinfo.qport, cls.userinfo);
		else
			CL_ConnectAbort("Q3 plugin not loaded, cannot connect to q3 servers without it.\n");
		return;
	}
#endif

	if (connectinfo.subprotocol == PROTOCOL_VERSION_Q2EX)
	{
		size_t foo;
		int seats = bound(1, cl_splitscreen.ival+1, MAX_SPLITS), i;
		Q_snprintfz(data, sizeof(data), "%c%c%c%cconnect", 255, 255, 255, 255);

		//qport+challenge were removed.
		Q_strncatz(data, va(" %i %i", connectinfo.subprotocol, seats), sizeof(data));

		//socials...
		for (i = 0; i < seats; i++)
		{
			Q_strncatz(data, va(" anon"), sizeof(data));

			//also make sure they have a valid name field, Q2E kinda bugs out otherwise.
			if (!InfoBuf_FindKey(&cls.userinfo[i], "name", &foo))
				InfoBuf_SetValueForKey(&cls.userinfo[i], "name", va("%s-%i", name.string, i+1));
			if (!InfoBuf_FindKey(&cls.userinfo[i], "fov", &foo))
				InfoBuf_SetValueForKey(&cls.userinfo[i], "fov", scr_fov.string);
		}

		Q_strncatz(data, " \"", sizeof(data));
		//note: this specific info string should lack the leading \\ char.
		Q_strncatz(data, va("qport\\%i", connectinfo.qport), sizeof(data));	//just in case a server enforces it (ie: someone connecting directly raw udp without the lobby junk)
		Q_strncatz(data, va("\\challenge\\%i", connectinfo.challenge), sizeof(data));	//just in case a server enforces it (ie: someone connecting directly raw udp without the lobby junk)
		if (connectinfo.spec==CIS_OBSERVE)
			Q_strncatz(data, "\\spectator\\1", sizeof(data));

		{
			const char *ignorekeys[] = {"prx", "*z_ext", (connectinfo.spec!=CIS_DEFAULT)?"spectator":NULL, NULL};
			Q2EX_UserInfoToString(data+strlen(data), sizeof(data)-strlen(data), ignorekeys, seats);
		}
		Q_strncatz(data, "\"", sizeof(data));
		Q_strncatz(data, "\n", sizeof(data));

		connectinfo.ext.fte1 = connectinfo.ext.fte2 = connectinfo.ext.ez1 = 0;
	}
	else
	{
		Q_snprintfz(data, sizeof(data), "%c%c%c%cconnect", 255, 255, 255, 255);

		Q_strncatz(data, va(" %i %i %i", connectinfo.subprotocol, connectinfo.qport, connectinfo.challenge), sizeof(data));

		//userinfo0 has some twiddles for extensions from other qw engines.
		Q_strncatz(data, " \"", sizeof(data));
		//qwfwd proxy routing
		if ((a = strrchr(cls.servername, '@')))
		{
			*a = 0;
			Q_strncatz(data, va("\\prx\\%s", cls.servername), sizeof(data));
			*a = '@';
		}
		if (connectinfo.spec==CIS_OBSERVE)
			Q_strncatz(data, "\\spectator\\1", sizeof(data));
		//the info itself
		{
			static const char *prioritykeys[] = {"name", "password", "spectator", "lang", "rate", "team", "topcolor", "bottomcolor", "skin", "_", "*", NULL};
			const char *ignorekeys[] = {"prx", "*z_ext", (connectinfo.spec!=CIS_DEFAULT)?"spectator":NULL, NULL};
			InfoBuf_ToString(&cls.userinfo[0], data+strlen(data), sizeof(data)-strlen(data), prioritykeys, ignorekeys, NULL, &cls.userinfosync, &cls.userinfo[0]);
		}
		if (connectinfo.protocol == CP_QUAKEWORLD)	//zquake extension info.
			Q_strncatz(data, va("\\*z_ext\\%i", CLIENT_SUPPORTED_Z_EXTENSIONS), sizeof(data));

		Q_strncatz(data, "\"", sizeof(data));

		if (connectinfo.protocol == CP_QUAKE2 && connectinfo.subprotocol == PROTOCOL_VERSION_R1Q2)
			Q_strncatz(data, va(" %d %d", connectinfo.ext.mtu, 1905), sizeof(data));	//mti, sub-sub-version
		else if (connectinfo.protocol == CP_QUAKE2 && connectinfo.subprotocol == PROTOCOL_VERSION_Q2PRO)
			Q_strncatz(data, va(" %d 0 0 %d", connectinfo.ext.mtu, 1021), sizeof(data));	//mtu, netchan-fragmentation, zlib, sub-sub-version

		Q_strncatz(data, "\n", sizeof(data));
	}
	if (connectinfo.ext.fte1)
		Q_strncatz(data, va("0x%x 0x%x\n", PROTOCOL_VERSION_FTE1, connectinfo.ext.fte1), sizeof(data));
	if (connectinfo.ext.fte2)
		Q_strncatz(data, va("0x%x 0x%x\n", PROTOCOL_VERSION_FTE2, connectinfo.ext.fte2), sizeof(data));

	if (connectinfo.ext.ez1)
		Q_strncatz(data, va("0x%x 0x%x\n", PROTOCOL_VERSION_EZQUAKE1, connectinfo.ext.ez1), sizeof(data));

	{
		int ourmtu;
		if (to->type == NA_LOOPBACK)
			ourmtu = MAX_UDP_PACKET;
		else if (*net_mtu.string)
			ourmtu = net_mtu.ival;
		else
			ourmtu = 1440;	//a safe bet. servers have an unsafe bet by default
		if (ourmtu < 0)
			ourmtu = 0;
		if (connectinfo.ext.mtu > ourmtu)
			connectinfo.ext.mtu = ourmtu;
		connectinfo.ext.mtu &= ~7;

		if (connectinfo.ext.mtu > 0)
			Q_strncatz(data, va("0x%x %i\n", PROTOCOL_VERSION_FRAGMENT, connectinfo.ext.mtu), sizeof(data));
	}

#ifdef HUFFNETWORK
	if (connectinfo.ext.compresscrc && net_compress.ival && Huff_CompressionCRC(connectinfo.ext.compresscrc))
		Q_strncatz(data, va("0x%x 0x%x\n", PROTOCOL_VERSION_HUFFMAN, LittleLong(connectinfo.ext.compresscrc)), sizeof(data));
	else
#endif
		connectinfo.ext.compresscrc = 0;

	info = CL_GUIDString(to);
	if (info)
		Q_strncatz(data, va("0x%x \"%s\"\n", PROTOCOL_INFO_GUID, info), sizeof(data));

	NET_SendPacket (cls.sockets, strlen(data), data, to);
}

char *CL_TryingToConnect(void)
{
	if (!connectinfo.trying)
		return NULL;

	if (connectinfo.numadr < 1)
		;
	else if (connectinfo.adr[0].prot == NP_KEXLAN
#ifdef SUPPORT_ICE
			|| connectinfo.adr[0].type == NA_ICE
#endif
			)
	{
		char status[1024];
		if (NET_GetConnectionCertificate(cls.sockets, &connectinfo.adr[0], QCERT_LOBBYSTATUS, status, sizeof(status))>0)
			return va("%s\n%s", cls.servername, status);
	}
	else if (connectinfo.adr[0].prot == NP_RTC_TCP || connectinfo.adr[0].prot == NP_RTC_TLS)
		return va("%s\n%s", cls.servername, "Waiting for broker connection");
	return cls.servername;
}

#if defined(NQPROT) && defined(HAVE_SERVER)
static void CL_NullReadPacket(void)
{	//just drop it all
}
#endif


struct resolvectx_s
{
	netadr_t adr[countof(connectinfo.adr)];
	size_t found;
	char servername[1];
	/*servername text*/
};
static void CL_ResolvedServer(void *vctx, void *data, size_t a, size_t b)
{
	size_t i;
	struct resolvectx_s *ctx = vctx;

	//something screwed us over...
	if (strcmp(ctx->servername, cls.servername))
		return;

	if (!ctx->found)
	{
		CL_ConnectAbort("Unable to resolve server address \"%s\"\n", ctx->servername);
		return;
	}

#ifdef HAVE_DTLS
	for (i = 0; i < ctx->found; i++)
	{
		if (net_enable_dtls.ival>=4 || connectinfo.mode==CIM_QEONLY)// || (connectinfo.peercred.hash && net_enable_dtls.ival >= 1))
		{	//if we've already established a dtls connection, stick with it
			if (ctx->adr[i].prot == NP_DGRAM)
				ctx->adr[i].prot = NP_DTLS;
		}
	}
#endif

	if (connectinfo.mode == CIM_Q2EONLY)
	{
		for (i = 0; i < ctx->found; i++)
		{	//if we've already established a dtls connection, stick with it, otherwise 'upgrade' from udp to 'kexlan' transport layer.
			if (ctx->adr[i].prot == NP_DGRAM)
				ctx->adr[i].prot = NP_KEXLAN;
		}
	}

	connectinfo.numadr = ctx->found;
	connectinfo.nextadr = 0;
	connectinfo.resolving = false;
	memcpy(connectinfo.adr, ctx->adr, sizeof(*connectinfo.adr)*ctx->found);

}
static void CL_ResolveServer(void *vctx, void *data, size_t a, size_t b)
{
	struct resolvectx_s *ctx = vctx;

	//stupid logic for targ@prox2@[ws[s]://]prox1 chaining. just disable it if there's weird ws:// or whatever in there.
	//FIXME: really shouldn't be in there
	const char *res = strrchr(ctx->servername, '/');
	const char *host = strrchr(ctx->servername+1, '@');
	if (host && (!res || res > host))
		host++;
	else
		host = ctx->servername;

	ctx->found = NET_StringToAdr2 (host, connectinfo.defaultport, ctx->adr, countof(ctx->adr), NULL);

	COM_AddWork(WG_MAIN, CL_ResolvedServer, ctx, data, a, b);
}
qboolean CL_IsPendingServerAddress(netadr_t *adr)
{
	size_t i;
	for (i = 0; i < connectinfo.numadr; i++)
		if (NET_CompareAdr(&connectinfo.adr[i], adr))
			return true;
	return false;
}
static qboolean CL_IsPendingServerBaseAddress(netadr_t *adr)
{
	size_t i;
	for (i = 0; i < connectinfo.numadr; i++)
		if (NET_CompareBaseAdr(&connectinfo.adr[i], adr))
			return true;
	return false;
}

/*
=================
CL_CheckForResend

Resend a connect message if the last one has timed out

=================
*/
void CL_CheckForResend (void)
{
	char	data[2048];
	double t1, t2;
	int contype = 0;
	qboolean keeptrying = true;
	netadr_t *to;

#ifdef HAVE_SERVER
	if (!cls.state && (!connectinfo.trying || sv.state != ss_clustermode) && sv.state)
	{
		const char *lbp;
#ifdef NQPROT
		qboolean proquakeangles = false;
#endif
#ifdef NETPREPARSE
		extern cvar_t dpcompat_nopreparse;
#endif
		extern cvar_t sv_guidhash;

		if (connectinfo.time && realtime - connectinfo.time < 1.0)
			return;
		memset(&connectinfo, 0, sizeof(connectinfo));
		connectinfo.time = realtime;
		Q_strncpyz (cls.servername, "internalserver", sizeof(cls.servername));
		Cvar_ForceSet(&cl_servername, cls.servername);
		connectinfo.numadr = NET_StringToAdr(cls.servername, 0, &connectinfo.adr[0]);
		connectinfo.nextadr = 0;
		if (!connectinfo.numadr)
			return;	//erk?

		if (*cl_disconnectreason.string)
			Cvar_Set(&cl_disconnectreason, "");
		connectinfo.trying = true;
		connectinfo.istransfer = false;
		connectinfo.adr[0].prot = NP_DGRAM;

		NET_InitClient(sv.state != ss_clustermode);

		//netchan extensions... we skip the getchallenge part so we need to set these up still.
		connectinfo.ext.mtu = 8192-16;
		connectinfo.ext.compresscrc = 0;
		Q_strncpyz(connectinfo.ext.guidsalt, sv_guidhash.string, sizeof(connectinfo.ext.guidsalt));

		cls.state = ca_disconnected;
		switch (svs.gametype)
		{
#ifdef Q3CLIENT
		case GT_QUAKE3:
			connectinfo.protocol = CP_QUAKE3;
			break;
#endif
#ifdef Q2CLIENT
		case GT_QUAKE2:
			connectinfo.protocol = CP_QUAKE2;
			lbp = cl_loopbackprotocol.string;
			if (!strcmp(lbp, "q2"))
			{	//vanilla
				connectinfo.subprotocol = PROTOCOL_VERSION_Q2;
				connectinfo.ext.fte1 = 0;
			}
			else if (!strcmp(lbp, "q2e") || !strcmp(lbp, STRINGIFY(PROTOCOL_VERSION_Q2EX)))
			{	//no fte extensions
				//still provides big coords, bigger index sizes, and splitscreen(non-dynamic though)
				connectinfo.subprotocol = PROTOCOL_VERSION_Q2EX;
				connectinfo.ext.fte1 = 0;
			}
			//else if (!strcmp(lbp, "r1q2") || !strcmp(lbp, STRINGIFY(PROTOCOL_VERSION_R1Q2)))
			//else if (!strcmp(lbp, "q2pro") || !strcmp(lbp, STRINGIFY(PROTOCOL_VERSION_Q2PRO)))
			else
			{
				connectinfo.subprotocol = PROTOCOL_VERSION_Q2EX;
				connectinfo.ext.fte1 = 0;//PEXT_MODELDBL|PEXT_SOUNDDBL|PEXT_SPLITSCREEN;
			}
			connectinfo.ext.fte2 = 0;
			connectinfo.ext.ez1 = 0;
			break;
#endif
		default:
			cl.movesequence = 0;
			lbp = cl_loopbackprotocol.string;
			if (!strcmp(lbp, "") || !strcmp(lbp, "qw") || progstype == PROG_H2)
			{	//qw with all supported extensions -default
				//for hexen2 we always force fte's native qw protocol. other protocols won't cut it.
				connectinfo.protocol = CP_QUAKEWORLD;
				connectinfo.subprotocol = PROTOCOL_VERSION_QW;
				connectinfo.ext.fte1 = Net_PextMask(PROTOCOL_VERSION_FTE1, false);
				connectinfo.ext.fte2 = Net_PextMask(PROTOCOL_VERSION_FTE2, false);
				connectinfo.ext.ez1 = Net_PextMask(PROTOCOL_VERSION_EZQUAKE1, false) & EZPEXT1_CLIENTADVERTISE;
			}
			else if (!strcmp(lbp, "qwid") || !strcmp(lbp, "idqw"))
			{	//for recording .qwd files in any client
				connectinfo.protocol = CP_QUAKEWORLD;
				connectinfo.subprotocol = PROTOCOL_VERSION_QW;
				connectinfo.ext.fte1 = 0;
				connectinfo.ext.fte2 = 0;
				connectinfo.ext.ez1 = 0;
			}
#ifdef Q3CLIENT
			else if (!strcmp(lbp, "q3"))
				cls.protocol = CP_QUAKE3;
#endif
#ifdef NQPROT
			else if (!strcmp(lbp, "random"))
			{	//for debugging.
				if (rand() & 1)
				{
					connectinfo.protocol = CP_NETQUAKE;
					connectinfo.subprotocol = CPNQ_FITZ666;
				}
				else
				{
					connectinfo.protocol = CP_QUAKEWORLD;
					connectinfo.subprotocol = PROTOCOL_VERSION_QW;
					connectinfo.ext.fte1 = Net_PextMask(PROTOCOL_VERSION_FTE1, false);
					connectinfo.ext.fte2 = Net_PextMask(PROTOCOL_VERSION_FTE2, false);
					connectinfo.ext.ez1 = Net_PextMask(PROTOCOL_VERSION_EZQUAKE1, false) & EZPEXT1_CLIENTADVERTISE;
				}
			}
			else if (!strcmp(lbp, "fitz") || !strcmp(lbp, "rmqe") ||
					 !strcmp(lbp, "qs") ||
					 !strcmp(lbp, "666") || !strcmp(lbp, "999"))
			{	//we don't really distinguish between fitz and rmq protocols. we just use 999 with bigcoords and 666 othewise.
				connectinfo.protocol = CP_NETQUAKE;
				connectinfo.subprotocol = CPNQ_FITZ666;
			}
			else if (!strcmp(lbp, "qe")||!strcmp(lbp, "qex")||!strcmp(lbp, "kex"))
			{	//quake-ex has special quirks that cannot be defined by protocol numbers alone.
				connectinfo.protocol = CP_NETQUAKE;
				connectinfo.subprotocol = CPNQ_FITZ666;
				connectinfo.mode = CIM_QEONLY;
			}
			else if (!strcmp(lbp, "bjp1") || !strcmp(lbp, "bjp2") || //placeholders only
					 !strcmp(lbp, "bjp3") || !strcmp(lbp, "bjp"))
			{
				connectinfo.protocol = CP_NETQUAKE;
				connectinfo.subprotocol = CPNQ_BJP3;
			}
			else if (!strcmp(lbp, "nq"))
			{
				connectinfo.protocol = CP_NETQUAKE;
				connectinfo.subprotocol = CPNQ_ID;
				proquakeangles = true;
			}
			else if (!strcmp(lbp, "nqid") || !strcmp(lbp, "idnq"))
			{
				connectinfo.protocol = CP_NETQUAKE;
				connectinfo.subprotocol = CPNQ_ID;
			}
			else if (!strcmp(lbp, "dp1") || !strcmp(lbp, "dpp1")||	//most of these are not supported, but parsed as placeholders on the slim chance that we ever do support them
					 !strcmp(lbp, "dp2") || !strcmp(lbp, "dpp2")||
					 !strcmp(lbp, "dp3") || !strcmp(lbp, "dpp3")||
					 !strcmp(lbp, "dp4") || !strcmp(lbp, "dpp4")||
					 !strcmp(lbp, "dp5") || !strcmp(lbp, "dpp5")||	//we support this serverside, but not clientside.
					 !strcmp(lbp, "dp6") || !strcmp(lbp, "dpp6"))	//this one is supported.
			{
				connectinfo.protocol = CP_NETQUAKE;
				connectinfo.subprotocol = CPNQ_DP6;
			}
			else if (!strcmp(lbp, "dp7") || !strcmp(lbp, "dpp7") ||
					 !strcmp(lbp, "dp") || !strcmp(lbp, "xonotic"))	//family name, common usage.
			{
				connectinfo.protocol = CP_NETQUAKE;
				connectinfo.subprotocol = CPNQ_DP7;
			}
			else if (!strcmp(lbp, "qss") ||
					 (progstype != PROG_QW && progstype != PROG_H2 && sv.state!=ss_clustermode && cl_splitscreen.ival <= 0))	//h2 depends on various extensions and doesn't really match either protocol, but we go for qw because that gives us all sorts of extensions.
			{
				connectinfo.protocol = CP_NETQUAKE;
				connectinfo.subprotocol = CPNQ_FITZ666;
				connectinfo.ext.fte1 = Net_PextMask(PROTOCOL_VERSION_FTE1, true);
				connectinfo.ext.fte2 = Net_PextMask(PROTOCOL_VERSION_FTE2, true);
				connectinfo.ext.ez1 = Net_PextMask(PROTOCOL_VERSION_EZQUAKE1, true) & EZPEXT1_CLIENTADVERTISE;
			}
#endif
			else
			{	//protocol wasn't recognised, and we didn't take the nq fallback, so that must mean we're going for qw.
				connectinfo.protocol = CP_QUAKEWORLD;
				connectinfo.subprotocol = PROTOCOL_VERSION_QW;
				connectinfo.ext.fte1 = Net_PextMask(PROTOCOL_VERSION_FTE1, false);
				connectinfo.ext.fte2 = Net_PextMask(PROTOCOL_VERSION_FTE2, false);
				connectinfo.ext.ez1 = Net_PextMask(PROTOCOL_VERSION_EZQUAKE1, false) & EZPEXT1_CLIENTADVERTISE;
			}

#ifdef NETPREPARSE
			if (dpcompat_nopreparse.ival)
#endif
			{
				//disabling preparsing with hexen2 is unsupported.
				if (progstype == PROG_H2)
					Con_Printf("dpcompat_nopreparse is unsupported with hexen2\n");
				else if (progstype == PROG_QW && cls.protocol != CP_QUAKEWORLD)
				{
					connectinfo.protocol = CP_QUAKEWORLD;
					connectinfo.subprotocol = PROTOCOL_VERSION_QW;
					connectinfo.ext.fte1 = Net_PextMask(PROTOCOL_VERSION_FTE1, false);
					connectinfo.ext.fte2 = Net_PextMask(PROTOCOL_VERSION_FTE2, false);
					connectinfo.ext.ez1 = Net_PextMask(PROTOCOL_VERSION_EZQUAKE1, false) & EZPEXT1_CLIENTADVERTISE;
				}
				else if (progstype != PROG_QW && cls.protocol == CP_QUAKEWORLD)
				{
					connectinfo.protocol = CP_NETQUAKE;
					connectinfo.subprotocol = CPNQ_DP7;	//dpcompat_nopreparse is only really needed for DP mods that send unknowable svc_tempentity messages to the client.
				}
			}

			//make sure the protocol within demos is actually correct/sane
			if (cls.demorecording == DPB_QUAKEWORLD && cls.protocol != CP_QUAKEWORLD)
			{
				connectinfo.protocol = CP_QUAKEWORLD;
				connectinfo.subprotocol = PROTOCOL_VERSION_QW;
				connectinfo.ext.fte1 = Net_PextMask(PROTOCOL_VERSION_FTE1, false);
				connectinfo.ext.fte2 = Net_PextMask(PROTOCOL_VERSION_FTE2, false);
				connectinfo.ext.ez1 = Net_PextMask(PROTOCOL_VERSION_EZQUAKE1, false) & EZPEXT1_CLIENTADVERTISE;
			}
#ifdef NQPROT
			else if (cls.demorecording == DPB_NETQUAKE && cls.protocol != CP_NETQUAKE)
			{
				connectinfo.protocol = CP_NETQUAKE;
				connectinfo.subprotocol = CPNQ_FITZ666;
				//FIXME: use pext.
			}
#endif
#ifdef Q2CLIENT
			else if (cls.demorecording == DPB_QUAKE2 && cls.protocol != CP_QUAKE2)
			{
				connectinfo.protocol = CP_QUAKE2;
				connectinfo.subprotocol = PROTOCOL_VERSION_Q2;
				connectinfo.ext.fte1 = PEXT_MODELDBL|PEXT_SOUNDDBL|PEXT_SPLITSCREEN;
				//FIXME: use pext.
			}
#endif
			break;
		}

		CL_FlushClientCommands();	//clear away all client->server clientcommands.

#ifdef NQPROT
		if (connectinfo.protocol == CP_NETQUAKE)
		{
			connectinfo.numadr = NET_StringToAdr2 (cls.servername, connectinfo.defaultport, connectinfo.adr, 1, NULL);
			connectinfo.nextadr = 0;
			if (!connectinfo.numadr)
			{
				CL_ConnectAbort("CL_CheckForResend: Bad server address \"%s\"\n", cls.servername);
				return;
			}
			NET_AdrToString(data, sizeof(data), &connectinfo.adr[connectinfo.nextadr]);

			/*eat up the server's packets, to clear any lingering loopback packets (like disconnect commands... yes this might cause packetloss for other clients)*/
			svs.sockets->ReadGamePacket = CL_NullReadPacket;
			NET_ReadPackets(svs.sockets);
			svs.sockets->ReadGamePacket = SV_ReadPacket;

			net_message.packing = SZ_RAWBYTES;
			net_message.cursize = 0;
			MSG_BeginReading(&net_message, net_message.prim);

			if (connectinfo.mode == CIM_QEONLY)
			{
				net_from = connectinfo.adr[connectinfo.nextadr];
				Cmd_TokenizeString (va("connect %i %i %i \"\\name\\unconnected\"", NQ_NETCHAN_VERSION_QEX, 0, SV_NewChallenge()), false, false);

				SVC_DirectConnect(0);
			}
			else if (connectinfo.subprotocol == CPNQ_ID && !proquakeangles)
			{
				net_from = connectinfo.adr[connectinfo.nextadr];
				Cmd_TokenizeString (va("connect %i %i %i \"\\name\\unconnected\"", NQ_NETCHAN_VERSION, 0, SV_NewChallenge()), false, false);

				SVC_DirectConnect(0);
			}
			else if (connectinfo.subprotocol == CPNQ_BJP3)
			{
				net_from = connectinfo.adr[connectinfo.nextadr];
				Cmd_TokenizeString (va("connect %i %i %i \"\\name\\unconnected\\mod\\%i\"", NQ_NETCHAN_VERSION, 0, SV_NewChallenge(), PROTOCOL_VERSION_BJP3), false, false);

				SVC_DirectConnect(0);
			}
			else if (connectinfo.subprotocol == CPNQ_FITZ666)
			{
				net_from = connectinfo.adr[connectinfo.nextadr];
				Cmd_TokenizeString (va("connect %i %i %i \"\\name\\unconnected\\mod\\%i\"", NQ_NETCHAN_VERSION, 0, SV_NewChallenge(), PROTOCOL_VERSION_FITZ), false, false);

				SVC_DirectConnect(0);
			}
			else if (proquakeangles)
			{
				net_from = connectinfo.adr[connectinfo.nextadr];
				Cmd_TokenizeString (va("connect %i %i %i \"\\name\\unconnected\\mod\\1\"", NQ_NETCHAN_VERSION, 0, SV_NewChallenge()), false, false);

				SVC_DirectConnect(0);
			}
			else if (1)
			{
				net_from = connectinfo.adr[connectinfo.nextadr];
				Q_snprintfz(net_message.data, net_message.maxsize, "xxxxconnect\\protocol\\darkplaces "STRINGIFY(NQ_NETCHAN_VERSION)"\\protocols\\DP7 DP6 DP5 RMQ FITZ NEHAHRABJP2 NEHAHRABJP NEHAHRABJP3 QUAKE\\challenge\\0x%x\\name\\%s", SV_NewChallenge(), name.string);
				Cmd_TokenizeString (net_message.data+4, false, false);
				SVC_DirectConnect(0);
			}
			else
				CL_ConnectToDarkPlaces("", &connectinfo.adr[connectinfo.nextadr]);
//			connectinfo.trying = false;
		}
		else
#endif
		{
			if (!connectinfo.challenge)
				connectinfo.challenge = rand();
			CL_SendConnectPacket (NULL);
		}

		return;
	}
#endif

	if (!connectinfo.trying)
	{
		if (*cl_servername.string)
			Cvar_ForceSet(&cl_servername, "");
		return;
	}
	if (startuppending || r_blockvidrestart || FS_DownloadingPackage())
		return;	//don't send connect requests until we've actually initialised fully. this isn't a huge issue, but makes the startup prints a little more sane.

	if (connectinfo.time && realtime - connectinfo.time < 5.0)
	{
		if (!connectinfo.clogged)
			return;
	}
	else
		connectinfo.clogged = false; //do the prints and everything.

	if (!cls.sockets)	//only if its needed... we don't want to keep using a new port unless we have to
		NET_InitClient(false);

#ifdef HAVE_DTLS
	if (connectinfo.numadr>0 && connectinfo.adr[0].prot == NP_DTLS)
	{	//get through the handshake first, instead of waiting for a 5-sec timeout between polls.
		switch(NET_SendPacket (cls.sockets, 0, NULL, &connectinfo.adr[0]))
		{
		case NETERR_CLOGGED:	//temporary failure
			connectinfo.clogged = true;
			return;
		case NETERR_DISCONNECTED:
			CL_ConnectAbort("DTLS Certificate Verification Failure\n");
			break;
		case NETERR_NOROUTE:	//not an error here, just means we need to send a new handshake.
			break;
		default:
			break;
		}
	}
#endif

	t1 = Sys_DoubleTime ();
	if (!connectinfo.istransfer)
	{
		if ((!connectinfo.numadr||connectinfo.nextadr>connectinfo.numadr*60) && !connectinfo.resolving)
		{
			struct resolvectx_s *rctx = Z_Malloc(sizeof(*rctx) + strlen(cls.servername));
			strcpy(rctx->servername, cls.servername);
			connectinfo.resolving = true;
			COM_AddWork(WG_LOADER, CL_ResolveServer, rctx, NULL, 0, 0);
		}
	}

	CL_FlushClientCommands();

	t2 = Sys_DoubleTime ();

	Cvar_ForceSet(&cl_servername, cls.servername);

	if (!connectinfo.numadr || !cls.sockets || connectinfo.resolving)
		return;	//nothing to do yet...
	if (!connectinfo.clogged)
		connectinfo.time = realtime+t2-t1;	// for retransmit requests

	to = &connectinfo.adr[connectinfo.nextadr%connectinfo.numadr];
	if (!NET_IsClientLegal(to))
	{
		CL_ConnectAbort ("Illegal server address\n");
		return;
	}

	if (!connectinfo.clogged)
	{
#ifdef Q3CLIENT
		if (q3)
			q3->cl.SendAuthPacket(cls.sockets, to);
#endif

		if ((connectinfo.istransfer || connectinfo.numadr>1) && to->prot != NP_RTC_TCP && to->prot != NP_RTC_TLS
#ifdef SUPPORT_ICE
		&& to->type != NA_ICE
#endif
		)
			Con_TPrintf ("Connecting to %s" S_COLOR_GRAY "(%s)" S_COLOR_WHITE "...\n", cls.servername, NET_AdrToString(data, sizeof(data), to));
		else
			Con_TPrintf ("Connecting to %s...\n", cls.servername);
	}

	if (connectinfo.clogged)
		connectinfo.clogged = false;

	if (connectinfo.tries == 0 && connectinfo.nextadr < connectinfo.numadr)
	{
		//stupid logic for targ@prox2@[ws[s]://]prox1 chaining. just disable it if there's weird ws:// or whatever in there.
		//FIXME: really shouldn't be in there
		const char *res = strrchr(cls.servername, '/');
		const char *host = strrchr(cls.servername+1, '@');
		if (host && (!res || res > host))
			host++;
		else
			host = cls.servername;

		if (!NET_EnsureRoute(cls.sockets, "conn", &connectinfo.peercred, host, to, true))
		{
			CL_ConnectAbort ("Unable to establish connection to %s\n", cls.servername);
			return;
		}
	}

	if (to->prot == NP_DGRAM)
		connectinfo.nextadr++;	//cycle hosts with each ping (if we got multiple).

	if (connectinfo.mode==CIM_Q2EONLY)
		contype |= 1;	//don't ever try nq packets here.
	else if (connectinfo.mode==CIM_QEONLY || connectinfo.mode==CIM_NQONLY)
		contype |= 2;
	else
	{
		contype |= 1; /*always try qw type connections*/
#ifdef VM_UI
		if (!(q3&&q3->ui.IsRunning()))	//don't try to connect to nq servers when running a q3ui. I was getting annoying error messages from q3 servers due to this.
#endif
		{
			COM_Parse(com_protocolname.string);
			if (!strcmp(com_token, "Quake2"))
			{
				if (connectinfo.nextadr>3)	//don't create an extra channel until we know our preferred one has failed.
					contype |= 4; /*q2e's kex lan layer*/
			}
			else
				contype |= 2; /*try nq connections periodically (or if its the default nq port)*/
		}
	}

	/*DP, QW, Q2, Q3*/
	/*NOTE: ioq3 has <challenge> <gamename> args. yes, a challenge to get a challenge.*/
	if (contype & 1)
	{
		char tmp[256];
		if (strrchr(cls.servername, '@'))	//if we're apparently using qwfwd then strictly adhere to vanilla's protocol so that qwfwd does not bug out. this will pollute gamedirs if stuff starts autodownloading from the wrong type of server, and probably show up vanilla-vs-rerelease glitches everywhere..
			Q_snprintfz (data, sizeof(data), "%c%c%c%cgetchallenge\n", 255, 255, 255, 255);
		else
			Q_snprintfz (data, sizeof(data), "%c%c%c%cgetchallenge %i %s\n", 255, 255, 255, 255, connectinfo.clchallenge, COM_QuotedString(com_protocolname.string, tmp, sizeof(tmp), false));
		switch(NET_SendPacket (cls.sockets, strlen(data), data, to))
		{
		case NETERR_CLOGGED:	//temporary failure
			connectinfo.clogged = true;	//inhibits the wait between sends
			break;
		case NETERR_SENT:		//yay, works!
			break;
		default:
			keeptrying = false;
			break;
		}
	}
	/*NQ*/
#ifdef NQPROT
	if ((contype & 2) && !connectinfo.clogged)
	{
		char *e;
		int pwd;
		sizebuf_t sb;
		memset(&sb, 0, sizeof(sb));
		sb.data = data;
		sb.maxsize = sizeof(data);

		MSG_WriteLong(&sb, LongSwap(NETFLAG_CTL | (strlen(NQ_NETCHAN_GAMENAME)+7)));
		MSG_WriteByte(&sb, CCREQ_CONNECT);
		MSG_WriteString(&sb, NQ_NETCHAN_GAMENAME);
		if (connectinfo.mode==CIM_QEONLY)
			MSG_WriteByte(&sb, NQ_NETCHAN_VERSION_QEX);
		else
		{
			MSG_WriteByte(&sb, NQ_NETCHAN_VERSION);

			/*NQ engines have a few extra bits on the end*/
			/*proquake servers wait for us to send them a packet before anything happens,
			  which means it corrects for our public port if our nat uses different public ports for different remote ports
			  thus all nq engines claim to be proquake
			*/
			if (!*password.string || !strcmp(password.string, "none"))
				pwd = 0;
			else
			{
				pwd = strtol(password.string, &e, 0);
				if (*e)
					pwd = CalcHashInt(&hash_md4, password.string, strlen(password.string));
			}
			MSG_WriteByte(&sb, MOD_PROQUAKE); /*'mod'*/
			MSG_WriteByte(&sb, 34); /*'mod' version*/
			MSG_WriteByte(&sb, 0); /*flags*/
			MSG_WriteLong(&sb, pwd); /*password*/

			/*FTE servers will detect this string and treat it as a qw challenge instead (if it allows qw clients), so protocol choice is deterministic*/
			if (contype & 1)
			{
				char tmp[256];
				MSG_WriteString(&sb, va("getchallenge %i %s\n", connectinfo.clchallenge, COM_QuotedString(com_protocolname.string, tmp, sizeof(tmp), false)));
			}
		}

		*(int*)sb.data = LongSwap(NETFLAG_CTL | sb.cursize);
		switch(NET_SendPacket (cls.sockets, sb.cursize, sb.data, to))
		{
		case NETERR_CLOGGED:	//temporary failure
		case NETERR_SENT:		//yay, works!
			break;
		default:
			keeptrying = false;
			break;
		}
	}
#endif
#ifdef Q2CLIENT
	if ((contype & 4) && !connectinfo.clogged)
	{
#define KEXLAN_SHAMELESSSELFPROMOMAGIC "\x08""CRANTIME"	//hey, if you can't shove your own nick in your network protocols then you're doing it wrong.
#define KEXLAN_SUBPROTOCOL "\x08""Quake II"	//this should be cvar-ised at some point, if its to ever be useful for anything but q2.
		static char pkt[] = "\x01\x60\x80"KEXLAN_SHAMELESSSELFPROMOMAGIC KEXLAN_SUBPROTOCOL"\x01";
		NET_SendPacket (cls.sockets, strlen(pkt), pkt, to);
	}
#endif

	connectinfo.tries++;

	if (!keeptrying)
	{
		if (to->prot != NP_DGRAM && connectinfo.nextadr+1 < connectinfo.numadr)
		{
			connectinfo.nextadr++;	//cycle hosts with each connection failure (if we got multiple addresses).
			connectinfo.tries = 0;
		}
		else
		{
			CL_ConnectAbort ("Unable to connect to %s, giving up\n", cls.servername);

			NET_CloseClient();
		}
	}
}

static void CL_BeginServerConnect(char *chain, int port, qboolean noproxy, enum coninfomode_e mode, enum coninfospec_e spec)
{
	const char *host = chain;
	const char *schemeend;
	char *arglist, *c;
	size_t presize;

	Q_strncpyz(cls.serverurl, chain, sizeof(cls.serverurl));

	for (c = chain; *c; c++)
	{
		if (*c == '@')
			host=c+1;
		else if (*c == '/' || *c == '?')
			break;	//stop if we find some path weirdness (like an authority).
	}
	presize = host-chain;
	if (presize >= sizeof(cls.servername))
	{
		CL_ConnectAbort("server address too long");
		return;	//no, get lost. panic.
	}
	memcpy(cls.servername, chain, presize);

	schemeend = strstr(host, "://");
	if (schemeend)
	{
		//"qw:tcp://host/observe"
		const char *schemestart = strchr(host, ':');
		const struct urischeme_s *scheme;

		if (!schemestart || schemestart==schemeend)
			schemestart = host;
		else
			schemestart++;

		//the scheme is either a network scheme in which case we use it directly, or a game-specific scheme.
		scheme = NET_IsURIScheme(schemestart);
		if (scheme && scheme->prot == NP_INVALID)
			scheme = NULL;	//qw:// or q3:// something that's just noise here.
		if (scheme && scheme->flags&URISCHEME_NEEDSRESOURCE)
		{
			Q_strncpyz (cls.servername+presize, schemestart, sizeof(cls.servername)-presize);	//oh. will probably be okay then
			arglist = NULL;
		}
		else
		{	//not some '/foo' name, not rtc:// either...
			char *sl = strchr(schemeend+3, '/');
			if (sl)
			{
				if (!strncmp(sl, "/observe", 8))
				{
					if (spec == CIS_DEFAULT)
						spec = CIS_OBSERVE;
					else if (spec != CIS_OBSERVE)
						Con_Printf("Ignoring 'observe'\n");
					memmove(sl, sl+8, strlen(sl+8)+1);
				}
				else if (!strncmp(sl, "/join", 5))
				{
					if (spec == CIS_DEFAULT)
						spec = CIS_JOIN;
					else if (spec != CIS_OBSERVE)
						Con_Printf("Ignoring 'join'\n");
					memmove(sl, sl+5, strlen(sl+5)+1);
				}
				else if (!strncmp(sl, "/qtvplay", 5))
				{
					char buf[256];
					*sl = 0;
					Cmd_ExecuteString(va("qtvplay %s\n", COM_QuotedString(schemeend+3, buf,sizeof(buf), false)), RESTRICT_LOCAL);
					return;
				}
				else if (!strncmp(sl, "/", 1) && (sl[1] == 0 || sl[1]=='?'))
				{
					//current spectator mode
					memmove(sl, sl+1, strlen(sl+1)+1);
				}
			}
			if (scheme)	//preserve the scheme, the netchan cares.
				Q_strncpyz (cls.servername+presize, schemestart, sizeof(cls.servername)-presize);	//probably some game-specific mess that we don't know
			else
				Q_strncpyz (cls.servername+presize, schemeend+3, sizeof(cls.servername)-presize);	//probably some game-specific mess that we don't know
			arglist = strchr(cls.servername, '?');
		}
	}
	else
	{
		if (!strncmp(host, "localhost", 9))
			noproxy = true;	//FIXME: resolve the address here or something so that we don't end up using a proxy for lan addresses.

		if (strstr(host, "://") || *host == '/' || !*cl_proxyaddr.string || noproxy)
			Q_strncpyz (cls.servername+presize, host, sizeof(cls.servername)-presize);
		else
			Q_snprintfz(cls.servername+presize, sizeof(cls.servername)-presize, "%s@%s", host, cl_proxyaddr.string);
		arglist = strchr(cls.servername, '?');
	}

	if (!port)
		port = cl_defaultport.value;

	CL_ConnectAbort(NULL);
	memset(&connectinfo, 0, sizeof(connectinfo));
	if (*cl_disconnectreason.string)
		Cvar_Set(&cl_disconnectreason, "");
	connectinfo.trying = true;
	connectinfo.defaultport = port;
	connectinfo.protocol = CP_UNKNOWN;
	connectinfo.mode = mode;
	connectinfo.spec = spec;
	connectinfo.clchallenge = rand()^(rand()<<16);

	connectinfo.peercred.name = cls.servername;
	if (arglist)
	{
		*arglist++ = 0;
		while (*arglist)
		{
			char *e = strchr(arglist, '&');
			if (e)
				*e=0;
			if (!strncasecmp(arglist, "fp=", 3))
			{
				size_t l = 8*Base64_DecodeBlock(arglist+3, arglist+strlen(arglist), connectinfo.peercred.digest, sizeof(connectinfo.peercred.digest));
				if (l <= 160)
					connectinfo.peercred.hash = &hash_sha1;
				else if (l <= 256)
					connectinfo.peercred.hash = &hash_sha2_256;
				else if (l <= 512)
					connectinfo.peercred.hash = &hash_sha2_512;
				else
					connectinfo.peercred.hash = NULL;
			}
			else
				Con_Printf(CON_WARNING"uri arg not known: \"%s\"\n", arglist);

			if (e)
				arglist=e+1;
			else
				break;
		}
	}

	SCR_SetLoadingStage(LS_CONNECTION);
	CL_CheckForResend();
}

void CL_BeginServerReconnect(void)
{
#ifdef HAVE_SERVER
	if (isDedicated)
	{
		Con_TPrintf ("Connect ignored - dedicated. set a renderer first\n");
		return;
	}
#endif
#ifdef HAVE_DTLS
	{
		int i;
		for (i = 0; i < connectinfo.numadr; i++)
			NET_DTLS_Disconnect(cls.sockets, &connectinfo.adr[i]);
	}
#endif
#ifdef SUPPORT_ICE
	while (connectinfo.numadr)	//remove any ICE addresses. probably we'll end up with no addresses left leaving us free to re-resolve giving us the original(ish) rtc connection.
	{
		if (connectinfo.adr[connectinfo.numadr-1].type != NA_ICE)
			break;
		connectinfo.numadr--;
	}
#endif
	if (*cl_disconnectreason.string)
		Cvar_Set(&cl_disconnectreason, "");
	connectinfo.trying = true;
	connectinfo.istransfer = false;
	connectinfo.time = 0;
	connectinfo.tries = 0;	//re-ensure routes.
	connectinfo.nextadr = 0; //should at least be consistent, other than packetloss. yay.
	connectinfo.clchallenge = rand()^(rand()<<16);

	NET_InitClient(false);
}

void CL_Transfer(netadr_t *adr)
{
	connectinfo.adr[0] = *adr;
	connectinfo.numadr = 1;
	connectinfo.istransfer = true;
	CL_CheckForResend();
}
void CL_Transfer_f(void)
{
	char oldguid[64];
	char	*server;
	if (Cmd_Argc() != 2)
	{
		Con_TPrintf ("usage: cl_transfer <server>\n");
		return;
	}

	CL_ConnectAbort(NULL);
	server = Cmd_Argv (1);
	if (!*server)
	{
		//if they didn't specify a server, abort any active transfer/connection.
		return;
	}

	Q_strncpyz(oldguid, connectinfo.guid, sizeof(oldguid));
	memset(&connectinfo, 0, sizeof(connectinfo));
	connectinfo.numadr = NET_StringToAdr(server, 0, &connectinfo.adr[0]);
	if (connectinfo.numadr)
	{
		connectinfo.istransfer = true;
		Q_strncpyz(connectinfo.guid, oldguid, sizeof(oldguid));	//retain the same guid on transfers

		Cvar_Set(&cl_disconnectreason, "Transferring....");
		connectinfo.trying = true;
		connectinfo.defaultport = cl_defaultport.value;
		connectinfo.protocol = CP_UNKNOWN;
		SCR_SetLoadingStage(LS_CONNECTION);
		CL_CheckForResend();
	}
	else
	{
		Con_Printf("cl_transfer: bad address\n");
	}
}
/*
================
CL_Connect_f

================
*/
void CL_Connect_c(int argn, const char *partial, struct xcommandargcompletioncb_s *ctx);
static void CL_Connect_f (void)
{
	char	*server;

	if (Cmd_Argc() != 2)
	{
		Con_TPrintf ("usage: connect <server>\n");
		return;
	}

	server = Cmd_Argv (1);
	server = strcpy(alloca(strlen(server)+1), server);

/*#ifdef HAVE_SERVER
	if (sv.state == ss_clustermode)
		CL_Disconnect (NULL);
	else
#endif*/
		CL_Disconnect_f ();

	CL_BeginServerConnect(server, 0, false, CIM_DEFAULT, CIS_DEFAULT);
}
#if defined(CL_MASTER) && defined(HAVE_PACKET)
static void CL_ConnectBestRoute_f (void)
{
	char	server[1024];
	int		proxies;
	int		directcost, chainedcost;
	if (Cmd_Argc() != 2)
	{
		Con_TPrintf ("usage: connectbr <server>\n");
		return;
	}

	proxies = Master_FindBestRoute(Cmd_Argv(1), server, sizeof(server), &directcost, &chainedcost);
	if (!*server)
	{
		Con_TPrintf ("Unable to route to server\n");
		return;
	}
	else if (proxies < 0)
		Con_TPrintf ("Routing database is not initialised, connecting directly\n");
	else if (!proxies)
		Con_TPrintf ("Routing table favours a direct connection\n");
	else if (proxies == 1)
		Con_TPrintf ("Routing table favours a single proxy (%ims vs %ims)\n", chainedcost, directcost);
	else
		Con_TPrintf ("Routing table favours chaining through %i proxies (%ims vs %ims)\n", proxies, chainedcost, directcost);

/*#ifdef HAVE_SERVER
	if (sv.state == ss_clustermode)
		CL_Disconnect (NULL);
	else
#endif*/
		CL_Disconnect_f ();
	CL_BeginServerConnect(server, 0, true, CIM_DEFAULT, CIS_DEFAULT);
}
#endif

static void CL_Join_f (void)
{
	char	*server;

	if (Cmd_Argc() != 2)
	{
		if (cls.state)
		{	//Hmm. This server sucks.
			if ((cls.z_ext & Z_EXT_JOIN_OBSERVE) || cls.protocol != CP_QUAKEWORLD)
				Cmd_ForwardToServer();
			else
				Cbuf_AddText("\nspectator 0;reconnect\n", RESTRICT_LOCAL);
			return;
		}
		Con_Printf ("join requires a connection or servername/ip\n");
		return;
	}

	server = Cmd_Argv (1);
	server = strcpy(alloca(strlen(server)+1), server);

	CL_Disconnect_f ();

	CL_BeginServerConnect(server, 0, false, CIM_DEFAULT, CIS_JOIN);
}

void CL_Observe_f (void)
{
	char	*server;

	if (Cmd_Argc() != 2)
	{
		if (cls.state)
		{
			if ((cls.z_ext & Z_EXT_JOIN_OBSERVE) || cls.protocol != CP_QUAKEWORLD)
				Cmd_ForwardToServer();
			else	//Hmm. This server sucks.
				Cbuf_AddText("\nspectator 1;reconnect\n", RESTRICT_LOCAL);
			return;
		}
		Con_Printf ("observe requires a connection or servername/ip\n");
		return;
	}

	server = Cmd_Argv (1);
	server = strcpy(alloca(strlen(server)+1), server);

	CL_Disconnect_f ();

	Cvar_Set(&spectator, "1");

	CL_BeginServerConnect(server, 0, false, CIM_DEFAULT, CIS_OBSERVE);
}

#ifdef NQPROT
void CLNQ_Connect_f (void)
{
	char	*server;
	enum coninfomode_e mode;
	int port = 26000;

	if (Cmd_Argc() != 2)
	{
		Con_TPrintf ("usage: connect <server>\n");
		return;
	}

	if (!strcmp(Cmd_Argv(0), "connectqe"))
		mode = CIM_QEONLY;
	else
		mode = CIM_NQONLY;

	server = Cmd_Argv (1);
	server = strcpy(alloca(strlen(server)+1), server);

	CL_Disconnect_f ();

	CL_BeginServerConnect(server, port, true, mode, CIS_DEFAULT/*doesn't really do spec/join stuff, but if the server asks for our info later...*/);
}
#endif
#ifdef Q2CLIENT
void CLQ2E_Connect_f (void)
{
	char	*server;

	if (Cmd_Argc() != 2)
	{
		Con_TPrintf ("usage: connect <server>\n");
		return;
	}

	server = Cmd_Argv (1);
	server = strcpy(alloca(strlen(server)+1), server);

	CL_Disconnect_f ();

	CL_BeginServerConnect(server, PORT_Q2EXSERVER/*q2e servers ignore their own port cvar, so don't use the standard q2 port number here*/, true, CIM_Q2EONLY, CIS_DEFAULT);
}
#endif
 
#ifdef IRCCONNECT
void CL_IRCConnect_f (void)
{
	CL_Disconnect_f ();

	if (FTENET_AddToCollection(cls.sockets, "TCP", Cmd_Argv(2), NA_IP, NP_IRC, false))
	{
		char *server;
		server = Cmd_Argv (1);

		CL_BeginServerConnect(va("irc://%s", server), 0, true);
	}
}
#endif

#ifdef TCPCONNECT
void CL_TCPConnect_f (void)
{
	if (!Q_strcasecmp(Cmd_Argv(0), "tlsconnect"))
		Cbuf_InsertText(va("connect tls://%s", Cmd_Argv(1)), Cmd_ExecLevel, true);
	else
		Cbuf_InsertText(va("connect tcp://%s", Cmd_Argv(1)), Cmd_ExecLevel, true);
}
#endif

/*
=====================
CL_Rcon_f

  Send the rest of the command line over as
  an unconnected command.
=====================
*/
void CL_Rcon_f (void)
{
	char	message[1024];
	char	*password;
	int		i;
	netadr_t	to;

	i = 1;
	password = rcon_password.string;
	if (!*password)	//FIXME: this is strange...
	{
		if (Cmd_Argc() < 3)
		{
			Con_TPrintf ("'rcon_password' is not set.\n");
			Con_TPrintf("usage: rcon (password) <command>\n");
			return;
		}
		password = Cmd_Argv(1);
		i = 2;
	}
	else
	{
		if (Cmd_Argc() < 2)
		{
			Con_Printf("usage: rcon <command>\n");
			return;
		}
	}

	message[0] = (char)255;
	message[1] = (char)255;
	message[2] = (char)255;
	message[3] = (char)255;
	message[4] = 0;

	Q_strncatz (message, "rcon ", sizeof(message));
	if (cl_crypt_rcon.ival)
	{
		char	cryptpass[1024], crypttime[64];
		const char *hex = "0123456789ABCDEF";	//must be upper-case for compat with mvdsv.
		time_t clienttime = time(NULL);
		size_t digestsize;
		unsigned char digest[64];
		const unsigned char **tokens = alloca(sizeof(*tokens)*(4+Cmd_Argc()*2));
		size_t *tokensizes = alloca(sizeof(*tokensizes)*(4+Cmd_Argc()*2));
		int j, k;
		void *ctx = alloca(hash_sha1.contextsize);

		for (j = 0; j < sizeof(time_t); j++)
		{	//little-endian byte order, but big-endian nibble order. just screwed. for compat with ezquake.
			crypttime[j*2+0] = hex[(clienttime>>(j*8+4))&0xf];
			crypttime[j*2+1] = hex[(clienttime>>(j*8))&0xf];
		}
		crypttime[j*2] = 0;

		tokens[0] = "rcon ";
		tokens[1] = password;
		tokens[2] = crypttime;
		tokens[3] = " ";
		for (j=0 ; j<Cmd_Argc()-i ; j++)
		{
			tokens[4+j*2+0] = Cmd_Argv(i+j);
			tokens[4+j*2+1] = " ";
		}
		hash_sha1.init(ctx);
		for (k = 0; k < 4+j*2; k++)
			hash_sha1.process(ctx, tokens[k], strlen(tokens[k]));
		hash_sha1.terminate(digest, ctx);
		digestsize = hash_sha1.digestsize;

		for (j = 0; j < digestsize; j++)
		{
			cryptpass[j*2+0] = hex[digest[j]>>4];
			cryptpass[j*2+1] = hex[digest[j]&0xf];
		}
		cryptpass[j*2] = 0;
		Q_strncatz (message, cryptpass, sizeof(message));
		Q_strncatz (message, crypttime, sizeof(message));
	}
	else
		Q_strncatz (message, password, sizeof(message));
	Q_strncatz (message, " ", sizeof(message));

	for ( ; i<Cmd_Argc() ; i++)
	{
		Q_strncatz (message, Cmd_Argv(i), sizeof(message));
		Q_strncatz (message, " ", sizeof(message));
	}

	if (cls.state >= ca_connected)
		to = cls.netchan.remote_address;
	else
	{
		if (!strlen(rcon_address.string))
		{
			Con_TPrintf ("You must either be connected,\nor set the 'rcon_address' cvar\nto issue rcon commands\n");

			return;
		}
		if (!NET_StringToAdr (rcon_address.string, PORT_QWSERVER, &to))
		{
			Con_Printf("Unable to resolve target address\n");
			return;
		}
	}

	NET_SendPacket (cls.sockets, strlen(message)+1, message, &to);
}

void CL_BlendFog(fogstate_t *result, fogstate_t *oldf, float time, fogstate_t *newf)
{
	float nfrac;
	if (time >= newf->time)
		nfrac = 1;
	else if (time < oldf->time)
		nfrac = 0;
	else
		nfrac = (time - oldf->time) / (newf->time - oldf->time);

	FloatInterpolate(oldf->alpha, nfrac, newf->alpha, result->alpha);
	FloatInterpolate(oldf->depthbias, nfrac, newf->depthbias, result->depthbias);
	FloatInterpolate(oldf->density, nfrac, newf->density, result->density);	//this should be non-linear, but that sort of maths is annoying.
	VectorInterpolate(oldf->colour, nfrac, newf->colour, result->colour);

	result->time = time;
}
void CL_ResetFog(int ftype)
{
	//blend from the current state, not the old state. this means things work properly if we've not reached the new state yet.
	CL_BlendFog(&cl.oldfog[ftype], &cl.oldfog[ftype], realtime, &cl.fog[ftype]);

	//reset the new state to defaults, to be filled in by the caller.
	memset(&cl.fog[ftype], 0, sizeof(cl.fog[ftype]));
	cl.fog[ftype].time = realtime;
	cl.fog[ftype].density = 0;
	cl.fog[ftype].colour[0] = //SRGBf(0.3);
	cl.fog[ftype].colour[1] = //SRGBf(0.3);
	cl.fog[ftype].colour[2] = SRGBf(0.3);
	cl.fog[ftype].alpha = 1;
	cl.fog[ftype].depthbias = 0;
	/*
	cl.fog[ftype].end = 16384;
	cl.fog[ftype].height = 1<<30;
	cl.fog[ftype].fadedepth = 128;
	*/
}

static void CL_ReconfigureCommands(int newgame)
{
	static int oldgame = ~0;
	extern void SCR_SizeUp_f (void);	//cl_screen
	extern void SCR_SizeDown_f (void);	//cl_screen
#ifdef QUAKESTATS
	extern void IN_Weapon (void);		//cl_input
	extern void IN_FireDown (void);		//cl_input
	extern void IN_FireUp (void);		//cl_input
	extern void IN_WWheelDown (void);
	extern void IN_WWheelUp (void);

	extern void IN_IWheelDown (void);
	extern void IN_IWheelUp (void);
	extern void IN_WeapNext_f (void);
	extern void IN_WeapPrev_f (void);
#endif
	extern void CL_Say_f (void);
	extern void CL_SayTeam_f (void);
	extern void CL_Color_f (void);
	static const struct
	{
		const char *name;
		void (*func) (void);
		const char *description;
		unsigned int problemgames; //1<<CP_*
	} problemcmds[] =
#define Q1 ((1u<<CP_QUAKEWORLD)|(1u<<CP_NETQUAKE))
#define Q2 (1u<<CP_QUAKE2)
#define Q3 (1u<<CP_QUAKE3)
	{
		{"sizeup",		SCR_SizeUp_f,	"Increase viewsize",		Q3},
		{"sizedown",	SCR_SizeDown_f,	"Decrease viewsize",		Q3},
		{"color",		CL_Color_f,		"Change Player Colours",	Q3},
		{"say",			CL_Say_f,		NULL, Q3},
		{"say_team",	CL_SayTeam_f,	NULL, Q3},

#ifdef QUAKESTATS
		{"weapon",		IN_Weapon,		"Configures weapon priorities for the next +attack as an alternative for the impulse command", ~Q1},
		{"+fire",		IN_FireDown,	"'+fire 8 7' will fire lg if you have it and fall back on rl if you don't, and just fire your current weapon if neither are held. Releasing fire will then switch away to exploit a bug in most mods to deny your weapon upgrades to your killer.", ~Q1},
		{"-fire",		IN_FireUp,		NULL, ~Q1},
		{"+weaponwheel",IN_WWheelDown,	"Quickly select a weapon without needing too many extra keybinds", ~Q1},
		{"-weaponwheel",IN_WWheelUp,	NULL, ~Q1},

#ifdef Q2CLIENT
		{"+wheel",		IN_WWheelDown,	"Quickly select a weapon without needing too many extra keybinds", ~Q2},
		{"-wheel",		IN_WWheelUp,	NULL, ~Q2},
		{"+wheel2",		IN_IWheelDown,	"Quickly use a powerup without needing too many extra keybinds", ~Q2},
		{"-wheel2",		IN_IWheelUp,	NULL, ~Q2},
		{"cl_weapnext",	IN_WeapNext_f,	"Select the next weapon", ~Q2},
		{"cl_weapprev",	IN_WeapPrev_f,	"Select the previous weapon", ~Q2},
#endif
#endif
	};
#undef Q1
#undef Q2
#undef Q3

	size_t i;

	newgame = 1<<newgame;
	for (i = 0; i < countof(problemcmds); i++)
	{
		if ((problemcmds[i].problemgames & newgame) && !(problemcmds[i].problemgames & oldgame))
			Cmd_RemoveCommand(problemcmds[i].name);
		if (!(problemcmds[i].problemgames & newgame) && (problemcmds[i].problemgames & oldgame))
			Cmd_AddCommandD(problemcmds[i].name, problemcmds[i].func, problemcmds[i].description);
	}
	oldgame = newgame;
}

/*
=====================
CL_ClearState

gamestart==true says that we're changing map, as opposed to servers.
=====================
*/
void CL_ClearState (qboolean gamestart)
{
	extern cvar_t cfg_save_auto;
	int			i, j;
	downloadlist_t *pendingdownloads, *faileddownloads;
#ifdef HAVE_SERVER
#define serverrunning (sv.state != ss_dead)
#define tolocalserver NET_IsLoopBackAddress(&cls.netchan.remote_address)
#else
#define serverrunning false
#define tolocalserver false
#define SV_UnspawnServer()
#endif

	CL_ReconfigureCommands(cls.protocol);

	CL_UpdateWindowTitle();

	CL_AllowIndependantSendCmd(false);	//model stuff could be a problem.

	S_StopAllSounds (true);
	S_UntouchAll();
	S_ResetFailedLoad();

	Cvar_ApplyLatches(CVAR_SERVEROVERRIDE, true);

	Con_DPrintf ("Clearing memory\n");
	if (!serverrunning || !tolocalserver)
	{
#ifdef HAVE_SERVER
		if (serverrunning && sv.state != ss_clustermode)
			SV_UnspawnServer();
#endif
		Mod_ClearAll ();
		r_regsequence++;

		Cvar_ApplyLatches(CVAR_MAPLATCH, false);
	}

	CL_ClearParseState();
	CL_ClearTEnts();
	CL_ClearCustomTEnts();
	Surf_ClearSceneCache();
#ifdef HEXEN2
	T_FreeInfoStrings();
#endif
	SCR_ShowPic_ClearAll(false);

	if (cl.playerview[0].playernum == -1)
	{	//left over from q2 connect.
		Media_StopFilm(true);
	}

	for (i = 0; i < UPDATE_BACKUP; i++)
	{
		if (cl.inframes[i].packet_entities.entities)
		{
			Z_Free(cl.inframes[i].packet_entities.entities);
			cl.inframes[i].packet_entities.entities = NULL;
		}
	}

	if (cl.lerpents)
		BZ_Free(cl.lerpents);
	if (cl.particle_ssprecaches)
	{
		for (i = 0; i < MAX_SSPARTICLESPRE; i++)
			if (cl.particle_ssname[i])
				free(cl.particle_ssname[i]);
	}
	if (cl.particle_csprecaches)
	{
		for (i = 0; i < MAX_CSPARTICLESPRE; i++)
			if (cl.particle_csname[i])
				free(cl.particle_csname[i]);
	}
	for (i = 0; i < countof(cl.model_name); i++)
		if (cl.model_name[i])
			BZ_Free(cl.model_name[i]);
#ifdef HAVE_LEGACY
	for (i = 0; i < countof(cl.model_name_vwep); i++)
		if (cl.model_name_vwep[i])
			BZ_Free(cl.model_name_vwep[i]);
#endif
	for (i = 0; i < countof(cl.sound_name); i++)
		if (cl.sound_name[i])
			BZ_Free(cl.sound_name[i]);
#ifdef Q2CLIENT
	for (i = 0; i < Q2MAX_IMAGES; i++)
		if (cl.image_name[i])
			BZ_Free(cl.image_name[i]);
	for (i = 0; i < Q2MAX_ITEMS; i++)
		if (cl.item_name[i])
			BZ_Free(cl.item_name[i]);
#endif

	while (cl.itemtimers)
	{
		struct itemtimer_s *t = cl.itemtimers;
		cl.itemtimers = t->next;
		Z_Free(t);
	}

	if (!gamestart)
	{
		downloadlist_t *next;
		while(cl.downloadlist)
		{
			next = cl.downloadlist->next;
			Z_Free(cl.downloadlist);
			cl.downloadlist = next;
		}
		while(cl.faileddownloads)
		{
			next = cl.faileddownloads->next;
			Z_Free(cl.faileddownloads);
			cl.faileddownloads = next;
		}
	}
	pendingdownloads = cl.downloadlist;
	faileddownloads = cl.faileddownloads;

#ifdef Q2CLIENT
	for (i = 0; i < countof(cl.configstring_general); i++)
	{
		if (cl.configstring_general[i])
			Z_Free(cl.configstring_general[i]);
	}
#endif

	for (i = 0; i < MAX_SPLITS; i++)
	{
		for (j = 0; j < MAX_CL_STATS; j++)
			if (cl.playerview[i].statsstr[j])
				Z_Free(cl.playerview[i].statsstr[j]);
	}

	Z_Free(cl.windowtitle);
	Z_Free(cl.serverpacknames);
	Z_Free(cl.serverpackhashes);

	InfoBuf_Clear(&cl.serverinfo, true);

	for (i = 0; i < MAX_CLIENTS; i++)
		InfoBuf_Clear(&cl.players[i].userinfo, true);

// wipe the entire cl structure
	memset (&cl, 0, sizeof(cl));

	CL_ResetFog(FOGTYPE_AIR);
	CL_ResetFog(FOGTYPE_WATER);
	CL_ResetFog(FOGTYPE_SKYROOM);

	cl.mapstarttime = realtime;
	cl.gamespeed = 1;
	cl.protocol_qw = PROTOCOL_VERSION_QW;	//until we get an svc_serverdata
	cl.allocated_client_slots = QWMAX_CLIENTS;
#ifdef HAVE_SERVER
	//FIXME: we should just set it to 0 to make sure its set up properly elsewhere.
	if (sv.state)
		cl.allocated_client_slots = sv.allocated_client_slots;
#endif

//	SZ_Clear (&cls.netchan.message);

	r_worldentity.model = NULL;

// clear other arrays
//	memset (cl_dlights, 0, sizeof(cl_dlights));
	Z_Free(cl_lightstyle);
	cl_lightstyle = NULL;
	cl_max_lightstyles = 0;

	rtlights_first = rtlights_max = RTL_FIRST;

	for (i = 0; i < MAX_SPLITS; i++)
	{
		VectorSet(cl.playerview[i].gravitydir, 0, 0, -1);
		cl.playerview[i].viewheight = DEFAULT_VIEWHEIGHT;
		cl.playerview[i].maxspeed = 320;
		cl.playerview[i].entgravity = 1;

		cl.playerview[i].chatstate = atoi(InfoBuf_ValueForKey(&cls.userinfo[i], "chat"));
	}
#ifdef QUAKESTATS
	for (i = 0; i < MAX_CLIENTS; i++)	//in case some server doesn't support it
		cl.players[i].stats[STAT_VIEWHEIGHT] = cl.players[i].statsf[STAT_VIEWHEIGHT] = DEFAULT_VIEWHEIGHT;
#endif
	cl.minpitch = -70;
	cl.maxpitch = 80;

	cl.oldgametime = 0;
	cl.gametime = 0;
	cl.gametimemark = 0;
	cl.splitclients = 1;
	cl.autotrack_hint = -1;
	cl.autotrack_killer = -1;
	cl.downloadlist = pendingdownloads;
	cl.faileddownloads = faileddownloads;
	cl.skyautorotate = 1;

	if (cfg_save_auto.ival && Cvar_UnsavedArchive())
		Cmd_ExecuteString("cfg_save\n", RESTRICT_LOCAL);
#ifdef CL_MASTER
	MasterInfo_WriteServers();
#endif

	R_GAliasFlushSkinCache(false);
}

/*
=====================
CL_Disconnect

Sends a disconnect message to the server
This is also called on Host_Error, so it shouldn't cause any errors
=====================
*/
void CL_Disconnect (const char *reason)
{
	qbyte	final[13];
	int i;

	if (reason)
		Cvar_Set(&cl_disconnectreason, reason);

	connectinfo.trying = false;

	SCR_SetLoadingStage(0);

	Cvar_ApplyLatches(CVAR_SERVEROVERRIDE, true);

// stop sounds (especially looping!)
	S_StopAllSounds (true);
#ifdef VM_CG
	if (q3)
		q3->cl.Disconnect(cls.sockets);
#endif
#ifdef CSQC_DAT
	CSQC_Shutdown();
#endif
	// if running a local server, shut it down
	if (cls.demoplayback != DPB_NONE)
		CL_StopPlayback ();
	else if (cls.state != ca_disconnected)
	{
		if (cls.demorecording)
			CL_Stop_f ();

		switch(cls.protocol)
		{
		case CP_NETQUAKE:
#ifdef NQPROT
			final[0] = clc_disconnect;
			final[1] = clc_stringcmd;
			strcpy (final+2, "drop");
			Netchan_Transmit (&cls.netchan, strlen(final)+1, final, 250000);
			Netchan_Transmit (&cls.netchan, strlen(final)+1, final, 250000);
			Netchan_Transmit (&cls.netchan, strlen(final)+1, final, 250000);
#endif
			break;
		case CP_PLUGIN:
			break;
		case CP_QUAKE2:
#ifdef Q2CLIENT
			final[0] = clcq2_stringcmd;
			if (cls.protocol_q2 == PROTOCOL_VERSION_Q2EX)
			{
				final[1] = 1;
				strcpy (final+2, "disconnect");
			}
			else
				strcpy (final+1, "disconnect");
			Netchan_Transmit (&cls.netchan, strlen(final)+1, final, 2500);
			Netchan_Transmit (&cls.netchan, strlen(final)+1, final, 2500);
			Netchan_Transmit (&cls.netchan, strlen(final)+1, final, 2500);
#endif
			break;
		case CP_QUAKE3:
			break;
		case CP_QUAKEWORLD:
			final[0] = clc_stringcmd;
			strcpy (final+1, "drop");
			Netchan_Transmit (&cls.netchan, strlen(final)+1, final, 2500);
			Netchan_Transmit (&cls.netchan, strlen(final)+1, final, 2500);
			Netchan_Transmit (&cls.netchan, strlen(final)+1, final, 2500);
			break;
		case CP_UNKNOWN:
			break;
		}

		cls.state = ca_disconnected;
		cls.protocol = CP_UNKNOWN;

		cls.demoplayback = DPB_NONE;
		cls.demorecording = cls.timedemo = false;

#ifdef HAVE_SERVER
	//running a server, and it's our own
		if (serverrunning && !tolocalserver && sv.state != ss_clustermode)
			SV_UnspawnServer();
#endif
	}
	Cam_Reset();

	if (cl.worldmodel)
	{
		Mod_ClearAll();
		cl.worldmodel = NULL;
	}

	CL_Parse_Disconnected();

	COM_FlushTempoaryPacks();

	r_worldentity.model = NULL;
	for (i = 0; i < cl.splitclients; i++)
		cl.playerview[i].spectator = 0;
	cl.sendprespawn = false;
	cl.intermissionmode = IM_NONE;
	cl.oldgametime = 0;

	memset(&r_refdef, 0, sizeof(r_refdef));

#ifdef NQPROT
	cls.signon=0;
#endif
	CL_StopUpload();

	CL_FlushClientCommands();

#ifdef HAVE_SERVER
	if (!isDedicated)
#endif
	{
		SCR_EndLoadingPlaque();
		V_ClearCShifts();
	}

	cl.servercount = 0;
	cls.findtrack = false;
	cls.realserverip.type = NA_INVALID;

	while (cls.qtvviewers)
	{
		struct qtvviewers_s *v = cls.qtvviewers;
		cls.qtvviewers = v->next;
		Z_Free(v);
	}

#ifdef TCPCONNECT
	//disconnects it, without disconnecting the others.
	FTENET_AddToCollection(cls.sockets, "conn", NULL, NA_INVALID, NP_DGRAM);
#endif

	Cvar_ForceSet(&cl_servername, "");

	CL_ClearState(false);

	FS_PureMode(NULL, 0, NULL, NULL, NULL, NULL, 0);

	Alias_WipeStuffedAliases();

	//now start up the csqc/menu module again.
//	(void)CSQC_UnconnectedInit();
}

#undef serverrunning
#undef tolocalserver

void CL_Disconnect_f (void)
{
#ifdef HAVE_SERVER
	if (sv.state)
		SV_UnspawnServer();
#endif

	CL_Disconnect (NULL);
	CL_ConnectAbort(NULL);
	NET_CloseClient();

	(void)CSQC_UnconnectedInit();
}

void CL_Disconnect2_f (void)
{
	char *reason = Cmd_Argv(1);
	if (*reason)
		Cvar_Set(&cl_disconnectreason, reason);
	CL_Disconnect_f();
}

/*
====================
CL_User_f

user <name or userid>

Dump userdata / masterdata for a user
====================
*/
void CL_User_f (void)
{
	int		uid;
	int		i;
	qboolean found = false;

#ifdef HAVE_SERVER
	if (sv.state)
	{
		SV_User_f();
		return;
	}
#endif

	if (Cmd_Argc() != 2)
	{
		Con_TPrintf ("Usage: user <username / userid>\n");
		return;
	}

	uid = atoi(Cmd_Argv(1));

	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		if (!cl.players[i].name[0])
			continue;
		if (cl.players[i].userid == uid
		|| !strcmp(cl.players[i].name, Cmd_Argv(1)) )
		{
			if (!cl.players[i].userinfovalid)
				Con_Printf("name: %s\ncolour %i %i\nping: %i\n", cl.players[i].name, cl.players[i].rbottomcolor, cl.players[i].rtopcolor, cl.players[i].ping);
			else
			{
				InfoBuf_Print (&cl.players[i].userinfo, "");
				Con_Printf("[%u, %u]\n", (unsigned)cl.players[i].userinfo.totalsize, (unsigned)cl.players[i].userinfo.numkeys);
			}
			found = true;
		}
	}
	if (!found)
		Con_TPrintf ("User not in server.\n");
}

/*
====================
CL_Users_f

Dump userids for all current players
====================
*/
void CL_Users_f (void)
{
	int		i;
	int		c;
	struct qtvviewers_s *v;

	c = 0;
	Con_TPrintf ("userid frags name\n");
	Con_TPrintf ("------ ----- ----\n");
	for (i=0 ; i<MAX_CLIENTS ; i++)
	{
		if (cl.players[i].name[0])
		{
			Con_TPrintf ("%6i %4i ^[%s\\player\\%i^]\n", cl.players[i].userid, cl.players[i].frags, cl.players[i].name, i);
			c++;
		}
	}

	for (v = cls.qtvviewers; v; v = v->next)
	{
		Con_Printf ("%6s %4s ^[%s^]\n", "", "-", v->name);
	}

	Con_TPrintf ("%i total users\n", c);
}


static struct {
	const char *name;
	unsigned int rgb;
} csscolours[] = {
	//php-defined colours
	{"aliceblue",		0xf0f8ff},
	{"antiquewhite",	0xfaebd7},
	{"aqua",			0x00ffff},
	{"aquamarine",		0x7fffd4},
	{"azure",			0xf0ffff},
	{"beige",			0xf5f5dc},
	{"bisque",			0xffe4c4},
	{"black",			0x000000},
	{"blanchedalmond",	0xffebcd},
	{"blue",			0x0000ff},
	{"blueviolet",		0x8a2be2},
	{"brown",			0xa52a2a},
	{"burlywood",		0xdeb887},
	{"cadetblue",		0x5f9ea0},
	{"chartreuse",		0x7fff00},
	{"chocolate",		0xd2691e},
	{"coral",			0xff7f50},
	{"cornflowerblue",	0x6495ed},
	{"cornsilk",		0xfff8dc},
	{"crimson",			0xdc143c},
	{"cyan",			0x00ffff},
	{"darkblue",		0x00008b},
	{"darkcyan",		0x008b8b},
	{"darkgoldenrod",	0xb8860b},
	{"darkgray",		0xa9a9a9},
	{"darkgreen",		0x006400},
	{"darkgrey",		0xa9a9a9},
	{"darkkhaki",		0xbdb76b},
	{"darkmagenta",		0x8b008b},
	{"darkolivegreen",	0x556b2f},
	{"darkorange",		0xff8c00},
	{"darkorchid",		0x9932cc},
	{"darkred",			0x8b0000},
	{"darksalmon",		0xe9967a},
	{"darkseagreen",	0x8fbc8f},
	{"darkslateblue",	0x483d8b},
	{"darkslategray",	0x2f4f4f},
	{"darkslategrey",	0x2f4f4f},
	{"darkturquoise",	0x00ced1},
	{"darkviolet",		0x9400d3},
	{"deeppink",		0xff1493},
	{"deepskyblue",		0x00bfff},
	{"dimgray",			0x696969},
	{"dimgrey",			0x696969},
	{"dodgerblue",		0x1e90ff},
	{"firebrick",		0xb22222},
	{"floralwhite",		0xfffaf0},
	{"forestgreen",		0x228b22},
	{"fuchsia",			0xff00ff},
	{"gainsboro",		0xdcdcdc},
	{"ghostwhite",		0xf8f8ff},
	{"gold",			0xffd700},
	{"goldenrod",		0xdaa520},
	{"gray",			0x808080},
	{"green",			0x008000},
	{"greenyellow",		0xadff2f},
	{"grey",			0x808080},
	{"honeydew",		0xf0fff0},
	{"hotpink",			0xff69b4},
	{"indianred",		0xcd5c5c},
	{"indigo",			0x4b0082},
	{"ivory",			0xfffff0},
	{"khaki",			0xf0e68c},
	{"lavender",		0xe6e6fa},
	{"lavenderblush",	0xfff0f5},
	{"lawngreen",		0x7cfc00},
	{"lemonchiffon",	0xfffacd},
	{"lightblue",		0xadd8e6},
	{"lightcoral",		0xf08080},
	{"lightcyan",		0xe0ffff},
	{"lightgoldenrodyellow",0xfafad2},
	{"lightgray",		0xd3d3d3},
	{"lightgreen",		0x90ee90},
	{"lightgrey",		0xd3d3d3},
	{"lightpink",		0xffb6c1},
	{"lightsalmon",		0xffa07a},
	{"lightseagreen",	0x20b2aa},
	{"lightskyblue",	0x87cefa},
	{"lightslategray",	0x778899},
	{"lightslategrey",	0x778899},
	{"lightsteelblue",	0xb0c4de},
	{"lightyellow",		0xffffe0},
	{"lime",			0x00ff00},
	{"limegreen",		0x32cd32},
	{"linen",			0xfaf0e6},
	{"magenta",			0xff00ff},
	{"maroon",			0x800000},
	{"mediumaquamarine",0x66cdaa},
	{"mediumblue",		0x0000cd},
	{"mediumorchid",	0xba55d3},
	{"mediumpurple",	0x9370db},
	{"mediumseagreen",	0x3cb371},
	{"mediumslateblue",	0x7b68ee},
	{"mediumspringgreen",0x00fa9a},
	{"mediumturquoise",	0x48d1cc},
	{"mediumvioletred",	0xc71585},
	{"midnightblue",	0x191970},
	{"mintcream",		0xf5fffa},
	{"mistyrose",		0xffe4e1},
	{"moccasin",		0xffe4b5},
	{"navajowhite",		0xffdead},
	{"navy",			0x000080},
	{"oldlace",			0xfdf5e6},
	{"olive",			0x808000},
	{"olivedrab",		0x6b8e23},
	{"orange",			0xffa500},
	{"orangered",		0xff4500},
	{"orchid",			0xda70d6},
	{"palegoldenrod",	0xeee8aa},
	{"palegreen",		0x98fb98},
	{"paleturquoise",	0xafeeee},
	{"palevioletred",	0xdb7093},
	{"papayawhip",		0xffefd5},
	{"peachpuff",		0xffdab9},
	{"peru",			0xcd853f},
	{"pink",			0xffc0cb},
	{"plum",			0xdda0dd},
	{"powderblue",		0xb0e0e6},
	{"purple",			0x800080},
	{"red",				0xff0000},
	{"rosybrown",		0xbc8f8f},
	{"royalblue",		0x4169e1},
	{"saddlebrown",		0x8b4513},
	{"salmon",			0xfa8072},
	{"sandybrown",		0xf4a460},
	{"seagreen",		0x2e8b57},
	{"seashell",		0xfff5ee},
	{"sienna",			0xa0522d},
	{"silver",			0xc0c0c0},
	{"skyblue",			0x87ceeb},
	{"slateblue",		0x6a5acd},
	{"slategray",		0x708090},
	{"slategrey",		0x708090},
	{"snow",			0xfffafa},
	{"springgreen",		0x00ff7f},
	{"steelblue",		0x4682b4},
	{"tan",				0xd2b48c},
	{"teal",			0x008080},
	{"thistle",			0xd8bfd8},
	{"tomato",			0xff6347},
	{"turquoise",		0x40e0d0},
	{"violet",			0xee82ee},
	{"wheat",			0xf5deb3},
	{"white",			0xffffff},
	{"whitesmoke",		0xf5f5f5},
	{"yellow",			0xffff00},
	{"yellowgreen",		0x9acd32},
};

int CL_ParseColour(const char *colt)
{
	char *e;
	int col;
	size_t i;
	if (!strncmp(colt, "0x", 2))
		col = 0xff000000|strtoul(colt+2, NULL, 16);
	else
	{
		col = strtoul(colt, &e, 0);
		if (*e)
		{
			col = 0;
			for (i = 0; i < countof(csscolours); i++)
				if (!Q_strcasecmp(colt, csscolours[i].name))
				{
					col = 0xff000000 | csscolours[i].rgb;
					break;
				}
		}
		else
		{
			col &= 15;
			if (col > 13)
				col = 13;
		}
	}
	return col;
}
const char *CL_ColourName(const char *colt)
{
	int col = CL_ParseColour(colt);
	size_t i;
	if (col & 0xff000000)
	{
		col &= ~0xff000000;
		for (i = 0; i < countof(csscolours); i++)
			if (csscolours[i].rgb == col)
				colt = csscolours[i].name;
	}
	return colt;
}


void CL_Color_c(int argn, const char *partial, struct xcommandargcompletioncb_s *ctx)
{
	int len;
	size_t i;
	if (argn == 1 || argn == 2)
	{
		len = strlen(partial);
		if (*partial >= '0' && *partial <= '9')
			;
		else for (i = 0; i < countof(csscolours); i++)
		{
			if (!Q_strncasecmp(partial, csscolours[i].name, len))
				ctx->cb(csscolours[i].name, va("^x%x%x%x%s", (csscolours[i].rgb>>20)&15, (csscolours[i].rgb>>12)&15, (csscolours[i].rgb>>4)&15, csscolours[i].name), NULL, ctx);
		}
	}
}
void CL_Color_f (void)
{
	// just for quake compatability...
	int		top, bottom;
	char	num[16];
	int  pnum = CL_TargettedSplit(true);

	qboolean server_owns_colour;

	char *t;
	char *b;

	if (Cmd_Argc() == 1)
	{
		const char *t = InfoBuf_ValueForKey(&cls.userinfo[pnum], "topcolor");
		const char *b = InfoBuf_ValueForKey(&cls.userinfo[pnum], "bottomcolor");
		t = CL_ColourName(t);
		b = CL_ColourName(b);
		if (!*t)
			t = "0";
		if (!*b)
			b = "0";
		if (!strcmp(t, b))
			Con_TPrintf ("\"color\" is \"%s\"\n", t, b);
		else
			Con_TPrintf ("\"color\" is \"%s %s\"\n", t, b);
		Con_TPrintf ("usage: color <0xRRGGBB> [0xRRGGBB]\n");
		return;
	}

	if (Cmd_FromGamecode())
		server_owns_colour = true;
	else
		server_owns_colour = false;


	t = Cmd_Argv(1);
	b = (Cmd_Argc()==2)?t:Cmd_Argv(2);
	if (!strcmp(t, "-1"))
		t = InfoBuf_ValueForKey(&cls.userinfo[pnum], "topcolor");
	top = CL_ParseColour(t);
	if (!strcmp(b, "-1"))
		b = InfoBuf_ValueForKey(&cls.userinfo[pnum], "bottomcolor");
	bottom = CL_ParseColour(b);

	Q_snprintfz (num, sizeof(num), (top&0xff000000)?"0x%06x":"%i", top & 0xffffff);
	if (top == 0)
		*num = '\0';
	if (Cmd_ExecLevel>RESTRICT_SERVER) //colour command came from server for a split client
		Cbuf_AddText(va("p%i cmd setinfo topcolor \"%s\"\n", pnum+1, num), Cmd_ExecLevel);
//	else if (server_owns_colour)
//		Cvar_LockFromServer(&topcolor, num);
	else
		CL_SetInfo(pnum, "topcolor", num);
	Q_snprintfz (num, sizeof(num), (bottom&0xff000000)?"0x%06x":"%i", bottom & 0xffffff);
	if (bottom == 0)
		*num = '\0';
	if (Cmd_ExecLevel>RESTRICT_SERVER) //colour command came from server for a split client
		Cbuf_AddText(va("p%i cmd setinfo bottomcolor \"%s\"\n", pnum+1, num), Cmd_ExecLevel);
	else if (server_owns_colour)
		Cvar_LockFromServer(&bottomcolor, num);
	else
		CL_SetInfo (pnum, "bottomcolor", num);
#ifdef NQPROT
	if (cls.protocol == CP_NETQUAKE)
		Cmd_ForwardToServer();
#endif
}

void CL_PakDownloads(int mode)
{
	/*
	mode=0 no downloads (forced to 1 for pure)
	mode=1 archived names so local stuff is not poluted
	mode=2 downloaded packages will always be present. Use With Caution.
	mode&4 download even packages that are not referenced.
	*/
	char local[256];
	char *pname, *sep;
	char *s = cl.serverpackhashes;
	int i;

	if (!cl.serverpakschanged || !mode)
		return;

	Cmd_TokenizeString(cl.serverpacknames, false, false);
	for (i = 0; i < Cmd_Argc(); i++)
	{
		s = COM_Parse(s);
		pname = Cmd_Argv(i);

		//'*' prefix means 'referenced'. so if the server isn't using any files from it, don't bother downloading it.
		if (*pname == '*')
			pname++;
		else if (!(mode & 4))
			continue;

		sep = strchr(pname, '/');
		if (!sep || strchr(sep+1, '/'))
			continue;	//don't try downloading weird ones here... paks inside paks is screwy stuff!

		if ((mode&3) != 2)
		{
			/*if we already have such a file, this is a no-op*/
			if (CL_CheckDLFile(va("package/%s", pname)))
				continue;
			if (!FS_GenCachedPakName(pname, com_token, local, sizeof(local)))
				continue;
		}
		else
			Q_strncpyz(local, pname, sizeof(local));
		CL_CheckOrEnqueDownloadFile(pname, local, DLLF_ALLOWWEB|DLLF_NONGAME);
	}
}

void CL_CheckServerPacks(void)
{
	static int oldpure;
	int pure = atof(InfoBuf_ValueForKey(&cl.serverinfo, "sv_pure"));
	if (pure < cl_pure.ival)
		pure = cl_pure.ival;
	pure = bound(0, pure, 2);

	if (!cl.serverpackhashes || cls.demoplayback)
		pure = 0;

	if (pure != oldpure || cl.serverpakschanged)
	{
		CL_PakDownloads((pure && !cl_download_packages.ival)?1:cl_download_packages.ival);
		FS_PureMode(NULL, pure, cl.serverpacknames, cl.serverpackhashes, NULL, NULL, cls.challenge);

		if (pure)
		{
			/*when enabling pure, kill cached models/sounds/etc*/
			Cache_Flush();
			/*make sure cheating lamas can't use old shaders from a different srver*/
			Shader_NeedReload(true);
		}
	}
	oldpure = pure;
	cl.serverpakschanged = false;
}

void CL_CheckServerInfo(void)
{
	char *s;
	unsigned int allowed;
#ifdef QUAKESTATS
	int oldstate;
#endif
#ifdef HAVE_SERVER
	extern cvar_t sv_cheats;
#endif
	int oldteamplay = cl.teamplay;
	qboolean spectating = true;
	int i;
	qboolean oldwatervis = cls.allow_watervis;
	int oldskyboxes = cls.allow_unmaskedskyboxes;

	//spectator 2 = spectator-with-scores, considered to be players. this means we don't want to allow spec cheats while they're inactive, because that would be weird.
	for (i = 0; i < cl.splitclients; i++)
		if (cl.playerview[i].spectator != 1)
			spectating = false;

	cl.teamplay = atoi(InfoBuf_ValueForKey(&cl.serverinfo, "teamplay"));
	cls.deathmatch = cl.deathmatch = atoi(InfoBuf_ValueForKey(&cl.serverinfo, "deathmatch"));

	cls.allow_cheats = false;
	cls.allow_semicheats=true;
	cls.allow_unmaskedskyboxes=false;
	cls.allow_fbskins = 1;
//	cls.allow_fbskins = 0;
//	cls.allow_overbrightlight;


	cls.allow_csqc = atoi(InfoBuf_ValueForKey(&cl.serverinfo, "anycsqc")) || *InfoBuf_ValueForKey(&cl.serverinfo, "*csprogs");
	cl.csqcdebug = atoi(InfoBuf_ValueForKey(&cl.serverinfo, "*csqcdebug"));

	s = InfoBuf_ValueForKey(&cl.serverinfo, "watervis");
	if (spectating || cls.demoplayback || atoi(s) || (!*s && ruleset_allow_watervis.ival))
		cls.allow_watervis=true;
	else
		cls.allow_watervis=false;

	s = InfoBuf_ValueForKey(&cl.serverinfo, "allow_skybox");
	if (!*s)
		s = InfoBuf_ValueForKey(&cl.serverinfo, "allow_skyboxes");
	if (!*s)
		cls.allow_unmaskedskyboxes = (cl.worldmodel && cl.worldmodel->fromgame != fg_quake);
	else cls.allow_unmaskedskyboxes = !!atoi(s);

	s = InfoBuf_ValueForKey(&cl.serverinfo, "fbskins");
	if (*s)
		cls.allow_fbskins = atof(s);
	else if (cl.teamfortress)
		cls.allow_fbskins = 0;
	else
		cls.allow_fbskins = 1;

	s = InfoBuf_ValueForKey(&cl.serverinfo, "*cheats");
	if (spectating || cls.demoplayback || !stricmp(s, "on"))
		cls.allow_cheats = true;

#ifdef HAVE_SERVER
	//allow cheats in single player regardless of sv_cheats.
	//(also directly read the sv_cheats cvar to avoid issues with nq protocols that don't support serverinfo.
	if (sv.state == ss_active && (sv.allocated_client_slots == 1 || sv_cheats.ival))
		cls.allow_cheats = true;
#endif

	s = InfoBuf_ValueForKey(&cl.serverinfo, "strict");
	if ((!spectating && !cls.demoplayback && *s && strcmp(s, "0")) || !ruleset_allow_semicheats.ival)
	{
		cls.allow_semicheats = false;
		cls.allow_cheats	= false;
	}

	cls.z_ext = atoi(InfoBuf_ValueForKey(&cl.serverinfo, "*z_ext")) & CLIENT_SUPPORTED_Z_EXTENSIONS;

#ifdef NQPROT
	if (cls.protocol == CP_NETQUAKE && CPNQ_IS_DP)
	{
		//movevars come from stats.
	}
	else
#endif
	{
		cls.maxfps = atof(InfoBuf_ValueForKey(&cl.serverinfo, "maxfps"));
		if (cls.maxfps < 20)
			cls.maxfps = 72;

		// movement vars for prediction
		cl.bunnyspeedcap = Q_atof(InfoBuf_ValueForKey(&cl.serverinfo, "pm_bunnyspeedcap"));
		movevars.slidefix = (Q_atof(InfoBuf_ValueForKey(&cl.serverinfo, "pm_slidefix")) != 0);
		movevars.slidyslopes = (Q_atof(InfoBuf_ValueForKey(&cl.serverinfo, "pm_slidyslopes")) != 0);
		movevars.bunnyfriction = (Q_atof(InfoBuf_ValueForKey(&cl.serverinfo, "pm_bunnyfriction")) != 0);
		movevars.airstep = (Q_atof(InfoBuf_ValueForKey(&cl.serverinfo, "pm_airstep")) != 0);
		movevars.pground = (Q_atof(InfoBuf_ValueForKey(&cl.serverinfo, "pm_pground")) != 0);
		movevars.stepdown = (Q_atof(InfoBuf_ValueForKey(&cl.serverinfo, "pm_stepdown")) != 0);
		movevars.walljump = (Q_atof(InfoBuf_ValueForKey(&cl.serverinfo, "pm_walljump")));
		movevars.ktjump = Q_atof(InfoBuf_ValueForKey(&cl.serverinfo, "pm_ktjump"));
		movevars.autobunny = (Q_atof(InfoBuf_ValueForKey(&cl.serverinfo, "pm_autobunny")) != 0);
		s = InfoBuf_ValueForKey(&cl.serverinfo, "pm_stepheight");
		movevars.stepheight = *s?Q_atof(s):PM_DEFAULTSTEPHEIGHT;
		s = InfoBuf_ValueForKey(&cl.serverinfo, "pm_watersinkspeed");
		movevars.watersinkspeed = *s?Q_atof(s):60;
		s = InfoBuf_ValueForKey(&cl.serverinfo, "pm_flyfriction");
		movevars.flyfriction = *s?Q_atof(s):4;
		s = InfoBuf_ValueForKey(&cl.serverinfo, "pm_edgefriction");
		movevars.edgefriction = *s?Q_atof(s):2;
		if (!(movevars.flags&MOVEFLAG_VALID))
			movevars.flags = (movevars.flags&~MOVEFLAG_QWEDGEBOX) | (*s?0:MOVEFLAG_QWEDGEBOX);
	}
	movevars.coordtype = cls.netchan.netprim.coordtype;

	// Initialize cl.maxpitch & cl.minpitch
	if (cls.protocol == CP_QUAKEWORLD || cls.protocol == CP_NETQUAKE)
	{
#ifdef NQPROT
		s = InfoBuf_ValueForKey(&cl.serverinfo, "maxpitch");
		cl.maxpitch = *s ? Q_atof(s) : ((cl_fullpitch_nq.ival && !cl.haveserverinfo)?90.0f:80.0f);
		s = InfoBuf_ValueForKey(&cl.serverinfo, "minpitch");
		cl.minpitch = *s ? Q_atof(s) : ((cl_fullpitch_nq.ival && !cl.haveserverinfo)?-90.0f:-70.0f);

		if (cls.protocol == CP_NETQUAKE)
		{	//proquake likes spamming us with fixangles
			//should be about 0.5/65536, but there's some precision issues with such small numbers around 80, so we need to bias it more than we ought
			cl.maxpitch -= 1.0/2048;
		}
#else
		s = InfoBuf_ValueForKey(&cl.serverinfo, "maxpitch");
		cl.maxpitch = *s ? Q_atof(s) : 80.0f;
		s = InfoBuf_ValueForKey(&cl.serverinfo, "minpitch");
		cl.minpitch = *s ? Q_atof(s) : -70.0f;
#endif
	}
	else
	{
		cl.maxpitch = 90;
		cl.minpitch = -90;
	}
	//bound it, such that we never end up looking slightly more back than forwards
	//FIXME: we should probably tweak our movement code instead.
	cl.maxpitch = bound(-89.9, cl.maxpitch, 89.9);
	cl.minpitch = bound(-89.9, cl.minpitch, 89.9);
	cl.disablemouse = atoi(InfoBuf_ValueForKey(&cl.serverinfo, "nomouse"));

	cl.hexen2pickups = atoi(InfoBuf_ValueForKey(&cl.serverinfo, "sv_pupglow"));

	allowed = atoi(InfoBuf_ValueForKey(&cl.serverinfo, "allow"));
	if (allowed & 1)
		cls.allow_watervis = true;
//	if (allowed & 2)
//		cls.allow_rearview = true;
	if (allowed & 4)
		cls.allow_unmaskedskyboxes = true;
//	if (allowed & 8)
//		cls.allow_mirrors = true;
	//16
	//32
//	if (allowed & 128)
//		cls.allow_postproc = true;
//	if (allowed & 256)
//		cls.allow_lightmapgamma = true;
	if (allowed & 512)
		cls.allow_cheats = true;

	if (cls.allow_semicheats)
		cls.allow_anyparticles = true;
	else
		cls.allow_anyparticles = false;


	if (spectating || cls.demoplayback)
		cl.fpd = 0;
	else
		cl.fpd = atoi(InfoBuf_ValueForKey(&cl.serverinfo, "fpd"));

	cl.gamespeed = atof(InfoBuf_ValueForKey(&cl.serverinfo, "*gamespeed"))/100.f;
	if (cl.gamespeed < 0.1)
		cl.gamespeed = 1;

#ifdef QUAKESTATS
	s = InfoBuf_ValueForKey(&cl.serverinfo, "status");
	oldstate = cl.matchstate;
	if (!stricmp(s, "standby"))
		cl.matchstate = MATCH_STANDBY;
	else if (!stricmp(s, "countdown"))
		cl.matchstate = MATCH_COUNTDOWN;
	else
	{
		float time = strtod(s, &s);
		if (!strcmp(s, " min left") || !strcmp(s, " mins left"))
			time *= 60;
		else if (!strcmp(s, " sec left") || !strcmp(s, " secs left"))
			time *= 1;
		else if (!strcmp(s, " hour left") || !strcmp(s, " hours left"))
			time *= 60*60;
		else
			time = -1;

		if (time >= 0)
		{
			//always update it. this is to try to cope with overtime.
			oldstate = cl.matchstate = MATCH_INPROGRESS;
			cl.matchgametimestart = cl.gametime + time - 60*atof(InfoBuf_ValueForKey(&cl.serverinfo, "timelimit"));
		}
		else
		{
			if (*s && cl.matchstate == MATCH_INPROGRESS)
				Con_DPrintf("Match state changed to unknown meaning: %s\n", s);
			else
				cl.matchstate = MATCH_DONTKNOW;	//don't revert from inprogress to don't know
		}
	}
	if (oldstate != cl.matchstate)
		cl.matchgametimestart = cl.gametime;
#endif

	CL_CheckServerPacks();

	Cvar_ForceCheatVars(cls.allow_semicheats, cls.allow_cheats);

	if (oldteamplay != cl.teamplay)
		Skin_FlushPlayers();
	if (oldwatervis != cls.allow_watervis || oldskyboxes != cls.allow_unmaskedskyboxes)
		Shader_NeedReload(false);

	CSQC_ServerInfoChanged();
}

/*
==================
CL_FullInfo_f

Allow clients to change userinfo
==================
*/
void CL_FullInfo_f (void)
{
	char	key[512];
	char	value[512];
	char	*o;
	char	*s;
	int pnum = CL_TargettedSplit(true);

	if (Cmd_Argc() != 2)
	{
		Con_TPrintf ("fullinfo <complete info string>\n");
		return;
	}

	s = Cmd_Argv(1);
	if (*s == '\\')
		s++;
	while (*s)
	{
		o = key;
		while (*s && *s != '\\' && o < key + sizeof(key))
			*o++ = *s++;
		if (o == key + sizeof(key))
		{
			Con_Printf ("key length too long\n");
			return;
		}
		*o = 0;

		if (!*s)
		{
			Con_Printf ("key %s has no value\n", key);
			return;
		}

		o = value;
		s++;
		while (*s && *s != '\\' && o < value + sizeof(value))
			*o++ = *s++;
		if (o == value + sizeof(value))
		{
			Con_Printf ("value length too long\n");
			return;
		}
		*o = 0;

		if (*s)
			s++;

		if (!stricmp(key, pmodel_name) || !stricmp(key, emodel_name))
			continue;

		InfoBuf_SetKey (&cls.userinfo[pnum], key, value);
	}
}

void CL_SetInfoBlob (int pnum, const char *key, const char *value, size_t valuesize)
{
	cvar_t *var;
	if (!pnum)
	{
		var = Cvar_FindVar(key);
		if (var && (var->flags & CVAR_USERINFO))
		{	//get the cvar code to set it. the server might have locked it.
			Cvar_Set(var, value);
			return;
		}
	}
	else if (pnum < 0 || pnum >= MAX_SPLITS)
		return;

	InfoBuf_SetStarBlobKey(&cls.userinfo[pnum], key, value, valuesize);
}
void CL_SetInfo (int pnum, const char *key, const char *value)
{
	CL_SetInfoBlob(pnum, key, value, strlen(value));
}
/*
==================
CL_SetInfo_f

Allow clients to change userinfo
==================
*/
void CL_SetInfo_f (void)
{
	char *key, *val;
	size_t keysize, valsize;
	cvar_t *var;
	int pnum = CL_TargettedSplit(true);
	if (Cmd_Argc() == 1)
	{
		InfoBuf_Print (&cls.userinfo[pnum], "");
		Con_Printf("[%u]\n", (unsigned int)cls.userinfo[pnum].totalsize);
		return;
	}
	if (Cmd_Argc() != 3)
	{
		Con_TPrintf ("usage: setinfo [ <key> <value> ]\n");
		return;
	}
	if (!stricmp(Cmd_Argv(1), pmodel_name) || !strcmp(Cmd_Argv(1), emodel_name))
		return;

	if (Cmd_Argv(1)[0] == '*')
	{
		int i;
		if (!strcmp(Cmd_Argv(1), "*"))
			if (!strcmp(Cmd_Argv(2), ""))
			{	//clear it out
				const char *k;
				for(i=0;;)
				{
					k = InfoBuf_KeyForNumber(&cls.userinfo[pnum], i);
					if (!k)
						break;	//no more.
					else if (*k == '*')
						i++;	//can't remove * keys
					else if ((var = Cvar_FindVar(k)) && (var->flags&CVAR_USERINFO))
						i++;	//this one is a cvar.
					else
						InfoBuf_RemoveKey(&cls.userinfo[pnum], k);	//we can remove this one though, so yay.
				}

				return;
			}
		Con_Printf ("Can't set * keys\n");
		return;
	}

	key = Cmd_Argv(1);
	val = Cmd_Argv(2);

	key = InfoBuf_DecodeString(key, key+strlen(key), &keysize);
	val = InfoBuf_DecodeString(val, val+strlen(val), &valsize);
	if (keysize != strlen(key))
		Con_Printf ("setinfo: ignoring key name with embedded null\n");
	else
		CL_SetInfoBlob(pnum, key, val, valsize);
	Z_Free(key);
	Z_Free(val);
}

#if 1//def _DEBUG
void CL_SetInfoBlob_f (void)
{
	qofs_t fsize;
	void *data;
	int pnum = CL_TargettedSplit(true);
	if (Cmd_Argc() == 1)
	{
		InfoBuf_Print (&cls.userinfo[pnum], "");
		return;
	}
	if (Cmd_Argc() != 3)
	{
		Con_TPrintf ("usage: setinfo [ <key> <filename> ]\n");
		return;
	}

	//user isn't allowed to set pmodel, emodel, *foo as these could break stuff.
	if (!stricmp(Cmd_Argv(1), pmodel_name) || !strcmp(Cmd_Argv(1), emodel_name))
		return;
	if (Cmd_Argv(1)[0] == '*')
	{
		Con_Printf ("Can't set * keys\n");
		return;
	}

	data = FS_MallocFile(Cmd_Argv(2), FS_GAME, &fsize);
	if (!data)
	{
		Con_Printf ("Unable to read %s\n", Cmd_Argv(2));
		return;
	}
	if (fsize > 64*1024*1024)
		Con_Printf ("File is over 64mb\n");
	else
		CL_SetInfoBlob(pnum, Cmd_Argv(1), data, fsize);
	FS_FreeFile(data);
}
#endif

void CL_SaveInfo(vfsfile_t *f)
{
	int i;
	for (i = 0; i < MAX_SPLITS; i++)
	{
		VFS_WRITE(f, "\n", 1);
		if (i)
		{
			VFS_WRITE(f, va("p%i setinfo * \"\"\n", i+1), 16);
			InfoBuf_WriteToFile(f, &cls.userinfo[i],  va("p%i setinfo", i+1), 0);
		}
		else
		{
			VFS_WRITE(f, "setinfo * \"\"\n", 13);
			InfoBuf_WriteToFile(f, &cls.userinfo[i], "setinfo", CVAR_USERINFO);
		}
	}
}

/*
====================
CL_Packet_f

packet <destination> <contents>

Contents allows \n escape character
====================
*/
void CL_Packet_f (void)
{
#ifdef FTE_TARGET_WEB
	//either this creates some expensive alternative rtc connection that screws us over, or just generally fails. don't allow it.
	Con_Printf (CON_WARNING "Ignoring 'packet %s' request.\n", Cmd_Argv(1));
#else
	char	send[2048];
	int		i, l;
	char	*in, *out;
	netadr_t	adr;
	struct dtlspeercred_s cred = {Cmd_Argv(1)};

	if (Cmd_Argc() != 3)
	{
		Con_TPrintf ("usage: packet <destination> <contents>\n");
		return;
	}

	if (!NET_StringToAdr (Cmd_Argv(1), PORT_DEFAULTSERVER, &adr))
	{
		Con_Printf ("Bad address: %s\n", Cmd_Argv(1));
		return;
	}


	if (Cmd_FromGamecode())	//some mvdsv servers stuffcmd a packet command which lets them know which ip the client is from.
	{						//unfortunatly, 50% of servers are badly configured resulting in them poking local services that THEY MUST NOT HAVE ACCESS TO.
		const char *addrdesc;
		const char *realdesc;
		if (cls.demoplayback)
		{
			Con_DPrintf ("Not sending realip packet from demo\n");
			return;
		}

		if (!NET_CompareAdr(&adr, &cls.netchan.remote_address))
		{
			if (NET_ClassifyAddress(&adr, &addrdesc) < ASCOPE_LAN)
			{
				if (NET_ClassifyAddress(&cls.netchan.remote_address, &realdesc) < ASCOPE_LAN)
				{	//this isn't necessarily buggy... but its still a potential exploit so we need to block it regardless.
					Con_Printf (CON_WARNING "Ignoring buggy %s realip request for %s server.\n", addrdesc, realdesc);
				}
				else
				{
					adr = cls.netchan.remote_address;
					Con_Printf (CON_WARNING "Ignoring buggy %s realip request, sending to %s server instead.\n", addrdesc, realdesc);
				}
			}
		}

		cls.realserverip = adr;
		Con_DPrintf ("Sending realip packet\n");
	}
	else if (!ruleset_allow_packet.ival)
	{
		Con_Printf("Sorry, the %s command is disallowed\n", Cmd_Argv(0));
		return;
	}
	cls.lastarbiatarypackettime = Sys_DoubleTime();	//prevent the packet command from causing a reconnect on badly configured mvdsv servers.

	in = Cmd_Argv(2);
	out = send+4;
	send[0] = send[1] = send[2] = send[3] = 0xff;

	l = strlen (in);
	for (i=0 ; i<l ; i++)
	{
		if (in[i] == '\\' && in[i+1] == 'n')
		{
			*out++ = '\n';
			i++;
		}
		else if (in[i] == '\\' && in[i+1] == '\\')
		{
			*out++ = '\\';
			i++;
		}
		else if (in[i] == '\\' && in[i+1] == 'r')
		{
			*out++ = '\r';
			i++;
		}
		else if (in[i] == '\\' && in[i+1] == '\"')
		{
			*out++ = '\"';
			i++;
		}
		else if (in[i] == '\\' && in[i+1] == '0')
		{
			*out++ = '\0';
			i++;
		}
		else
			*out++ = in[i];
	}
	*out = 0;

	if (!cls.sockets)
		NET_InitClient(false);
	if (!NET_EnsureRoute(cls.sockets, "packet", &cred, Cmd_Argv(1), &adr, true))
		return;
	NET_SendPacket (cls.sockets, out-send, send, &adr);

	if (Cmd_FromGamecode())
	{
		//realip
		char *temp = Z_Malloc(strlen(in)+1);
		strcpy(temp, in);
		Cmd_TokenizeString(temp, false, false);
		cls.realip_ident = atoi(Cmd_Argv(2));
		Z_Free(temp);
	}
#endif
}


/*
=====================
CL_NextDemo

Called to play the next demo in the demo loop
=====================
*/
void CL_NextDemo (void)
{
	cvar_t *cl_autodemos;
	char	str[1024];

	if (cls.demonum < 0)
		return;		// don't play demos

	cl_autodemos = Cvar_FindVar("cl_autodemos");
	if (cl_autodemos && *cl_autodemos->string)
	{
		Cmd_TokenizeString(cl_autodemos->string, false, false);
		if (!Cmd_Argc())
		{	//none...
			cls.demonum = -1;
			return;
		}
		if (cls.demonum >= Cmd_Argc())
			cls.demonum = 0;	//restart the loop

		if (!strcmp(Cmd_Argv(cls.demonum), "quit"))
			Q_snprintfz (str, sizeof(str), "quit\n");
		else
			Q_snprintfz (str, sizeof(str), "playdemo \"demos/%s\"\n", Cmd_Argv(cls.demonum));
	}
	else
	{
		if (!cls.demos[cls.demonum][0] || cls.demonum >= MAX_DEMOS)
		{
			cls.demonum = 0;
			if (!cls.demos[cls.demonum][0])
			{
	//			Con_Printf ("No demos listed with startdemos\n");
				cls.demonum = -1;
				return;
			}
		}

		if (!strcmp(cls.demos[cls.demonum], "quit"))
			Q_snprintfz (str, sizeof(str), "quit\n");
		else
			Q_snprintfz (str, sizeof(str), "playdemo %s\n", cls.demos[cls.demonum]);
	}
	Cbuf_InsertText (str, RESTRICT_LOCAL, false);
	cls.demonum++;

	if (!cls.state)
		cls.state = ca_demostart;
}

/*
===============================================================================

DEMO LOOP CONTROL

===============================================================================
*/


/*
==================
CL_Startdemos_f
==================
*/
void CL_Startdemos_f (void)
{
	int		i, c;

	c = Cmd_Argc() - 1;
	if (c > MAX_DEMOS)
	{
		Con_Printf ("Max %i demos in demoloop\n", MAX_DEMOS);
		c = MAX_DEMOS;
	}
	Con_DPrintf ("%i demo(s) in loop\n", c);

	for (i=1 ; i<c+1 ; i++)
		Q_strncpyz (cls.demos[i-1], Cmd_Argv(i), sizeof(cls.demos[0]));
	for ( ; i<MAX_DEMOS ; i++)
		Q_strncpyz (cls.demos[i-1], "", sizeof(cls.demos[0]));

	cls.demonum = -1;
	//don't start it here - we might have been given a +connect or whatever argument.
}


/*
==================
CL_Demos_f

Return to looping demos
==================
*/
void CL_Demos_f (void)
{
	const char *mode = Cmd_Argv(1);
	if (!strcmp(mode, "idle"))
	{	//'demos idle' only plays the demos when we're idle.
		//this can be used for menu backgrounds (engine and menuqc).
		if (cls.state || cls.demoplayback || CL_TryingToConnect())
			return;
	}
	//else disconnects can switch gamedirs/paks and kill menuqc etc.

	if (cls.demonum == -1)
		cls.demonum = 1;
	CL_Disconnect_f ();
	CL_NextDemo ();
}

/*
==================
CL_Stopdemo_f

stop demo
==================
*/
void CL_Stopdemo_f (void)
{
	if (cls.demoplayback == DPB_NONE)
		return;
	CL_StopPlayback ();
	CL_Disconnect (NULL);
}



/*
=================
CL_Changing_f

Just sent as a hint to the client that they should
drop to full console
=================
*/
void CL_Changing_f (void)
{
	char *mapname = Cmd_Argv(1);
	if (cls.download && cls.download->method <= DL_QWPENDING)  // don't change when downloading
		return;

	cls.demoseeking = DEMOSEEK_NOT;	//don't seek over it

	if (*mapname)
		SCR_ImageName(mapname);
	else
		SCR_BeginLoadingPlaque();

	S_StopAllSounds (true);
	cl.intermissionmode = IM_NONE;
	if (cls.state)
	{
		cls.state = ca_connected;	// not active anymore, but not disconnected
		Con_TPrintf ("\nChanging map...\n");
	}
	else
		Con_Printf("Changing while not connected\n");

#ifdef NQPROT
	cls.signon=0;
#endif
}


/*
=================
CL_Reconnect_f

User command, or NQ protocol command (messy).
=================
*/
void CL_Reconnect_f (void)
{
	if (cls.download && cls.download->method <= DL_QWPENDING)  // don't change when downloading
		return;
#ifdef NQPROT
	if (cls.protocol == CP_NETQUAKE && Cmd_IsInsecure())
	{
		CL_Changing_f();
		return;
	}
#endif
	S_StopAllSounds (true);

	if (cls.state == ca_connected)
	{
		Con_TPrintf ("reconnecting...\n");
		CL_SendClientCommand(true, "new");
		return;
	}

	if (!*cls.servername)
	{
		Con_TPrintf ("No server to reconnect to...\n");
		return;
	}

#if defined(HAVE_SERVER) && defined(SUBSERVERS)
	if (sv.state == ss_clustermode)
	{	//reconnecting while we're a cluster... o.O
		char oldguid[sizeof(connectinfo.guid)];
		Q_strncpyz(oldguid, connectinfo.guid, sizeof(oldguid));
		memset(&connectinfo, 0, sizeof(connectinfo));
		connectinfo.istransfer = false;
		Q_strncpyz(connectinfo.guid, oldguid, sizeof(oldguid));	//retain the same guid on transfers

		Cvar_Set(&cl_disconnectreason, "Transferring....");
		connectinfo.trying = true;
		connectinfo.defaultport = cl_defaultport.value;
		connectinfo.protocol = CP_UNKNOWN;
		SCR_SetLoadingStage(LS_CONNECTION);
		CL_CheckForResend();
		return;
	}
#endif

	CL_Disconnect(NULL);
	CL_BeginServerReconnect();
}

static void CL_ConnectionlessPacket_Connection(char *tokens)
{
	unsigned int ncflags;
	int qportsize = -1;
	if (net_from.type == NA_INVALID)
		return;	//I've found a qizmo demo that contains one of these. its best left ignored.

	if (!CL_IsPendingServerAddress(&net_from))
	{
		if (net_from.type != NA_LOOPBACK)
			Con_TPrintf ("ignoring connection\n");
		return;
	}

	if (cls.state >= ca_connected)
	{
		if (!NET_CompareAdr(&cls.netchan.remote_address, &net_from))
		{
#ifdef HAVE_SERVER
			if (sv.state != ss_clustermode)
#endif
				CL_Disconnect (NULL);
		}
		else
		{
			if (cls.demoplayback == DPB_NONE)
				Con_TPrintf ("Dup connect received.  Ignored.\n");
			return;
		}
	}
	if (net_from.type != NA_LOOPBACK)
	{
//			Con_TPrintf (S_COLOR_GRAY"connection\n");

#ifdef HAVE_SERVER
		if (sv.state && sv.state != ss_clustermode)
			SV_UnspawnServer();
#endif
	}

#if defined(Q2CLIENT)
	if (tokens && connectinfo.protocol == CP_QUAKE2)
	{
		if (cls.protocol_q2 == PROTOCOL_VERSION_R1Q2 || cls.protocol_q2 == PROTOCOL_VERSION_Q2PRO)
			qportsize = 1;
		tokens = COM_Parse(tokens);	//skip the client_connect bit
		while((tokens = COM_Parse(tokens)))
		{
			if (!strncmp(com_token, "ac=", 3))
			{
				if (atoi(com_token+3))
				{
					CL_ConnectAbort("Server requires anticheat support");
					return;
				}
			}
			else if (!strncmp(com_token, "nc=", 3))
				qportsize = atoi(com_token+3)?1:2;
			else if (!strncmp(com_token, "map=", 4))
				SCR_ImageName(com_token+4);
			else if (!strncmp(com_token, "dlserver=", 9))
				Q_strncpyz(cls.downloadurl, com_token+9, sizeof(cls.downloadurl));
			else if (!strcmp(com_token, STRINGIFY(PROTOCOL_VERSION_Q2EX)))
				connectinfo.subprotocol = atoi(com_token);
			else
				Con_DPrintf("client_connect: Unknown token \"%s\"\n", com_token);
		}
	}
#endif

	connectinfo.trying = false;
	cl.splitclients = 0;
	cls.protocol = connectinfo.protocol;
	cls.proquake_angles_hack = false;
	cls.fteprotocolextensions = connectinfo.ext.fte1;
	cls.fteprotocolextensions2 = connectinfo.ext.fte2;
	cls.ezprotocolextensions1 = connectinfo.ext.ez1;
	cls.challenge = connectinfo.challenge;
	ncflags = NCF_CLIENT;
	if (connectinfo.ext.mtu)
		ncflags |= NCF_FRAGABLE;
	if (connectinfo.ext.fte2&PEXT2_STUNAWARE)
		ncflags |= NCF_STUNAWARE;
	Netchan_Setup (ncflags, &cls.netchan, &net_from, connectinfo.qport, connectinfo.ext.mtu);
	cls.protocol_q2 = (cls.protocol == CP_QUAKE2)?connectinfo.subprotocol:0;
	if (qportsize>=0)
		cls.netchan.qportsize = qportsize;

#ifdef HUFFNETWORK
	cls.netchan.compresstable = Huff_CompressionCRC(connectinfo.ext.compresscrc);
#else
	cls.netchan.compresstable = NULL;
#endif
	CL_ParseEstablished();
#ifdef Q3CLIENT
	if (cls.protocol == CP_QUAKE3)
		q3->cl.Established();
	else
#endif
		CL_SendClientCommand(true, "new");
	cls.state = ca_connected;
#ifdef QUAKESPYAPI
	allowremotecmd = false; // localid required now for remote cmds
#endif

	total_loading_size = 100;
	current_loading_size = 0;
	SCR_SetLoadingStage(LS_CLIENT);

	Validation_Apply_Ruleset();

	CL_WriteSetDemoMessage();
}

/*
=================
CL_ConnectionlessPacket

Responses to broadcasts, etc
=================
*/
void CL_ConnectionlessPacket (void)
{
	char	*s;
	int		c;
	char	adr[MAX_ADR_SIZE];

	MSG_BeginReading (&net_message, msg_nullnetprim);
	MSG_ReadLong ();        // skip the -1

	Cmd_TokenizeString(net_message.data+4, false, false);

	if (net_message.cursize == sizeof(net_message_buffer))
		net_message.data[sizeof(net_message_buffer)-1] = '\0';
	else
		net_message.data[net_message.cursize] = '\0';

#ifdef PLUGINS
	if (Plug_ConnectionlessClientPacket(net_message.data+4, net_message.cursize-4))
		return;
#endif

	c = MSG_ReadByte ();

	// ping from somewhere
	if (c == A2A_PING)
	{
		char	data[256];
		int len;

		if (cls.realserverip.type == NA_INVALID)
			return;	//not done a realip yet

		if (NET_CompareBaseAdr(&cls.realserverip, &net_from) == false)
			return;	//only reply if it came from the real server's ip.

		data[0] = 0xff;
		data[1] = 0xff;
		data[2] = 0xff;
		data[3] = 0xff;
		data[4] = A2A_ACK;
		data[5] = 0;

		//ack needs two parameters to work with realip properly.
		//firstly it needs an auth message, so it can't be spoofed.
		//secondly, it needs a copy of the realip ident, so you can't report a different player's client (you would need access to their ip).
		data[5] = ' ';
		Q_snprintfz(data+6, sizeof(data)-6, "%i %i", atoi(MSG_ReadString()), cls.realip_ident);
		len = strlen(data);

		NET_SendPacket (cls.sockets, len, &data, &net_from);
		return;
	}

	if (c == A2C_PRINT)
	{
		if (!strncmp(net_message.data+MSG_GetReadCount(), "\\chunk", 6))
		{
			if (NET_CompareBaseAdr(&cls.netchan.remote_address, &net_from) == false)
				if (cls.realserverip.type == NA_INVALID || NET_CompareBaseAdr(&cls.realserverip, &net_from) == false)
					return;	//only use it if it came from the real server's ip (this breaks on proxies).

			MSG_ReadLong();
			MSG_ReadChar();
			MSG_ReadChar();

			if (CL_ParseOOBDownload())
			{
				if (MSG_GetReadCount() != net_message.cursize)
				{
					Con_Printf ("junk on the end of the packet\n");
					CL_Disconnect_f();
				}
				cls.netchan.last_received = realtime;	//in case there's some virus scanner running on the server making it stall... for instance...
			}
			return;
		}
	}

	if (cls.demoplayback == DPB_NONE && net_from.type != NA_LOOPBACK)
		Con_Printf (S_COLOR_GRAY"%s: ", NET_AdrToString (adr, sizeof(adr), &net_from));
//	Con_DPrintf ("%s", net_message.data + 4);

	if (c == 'f')	//using 'f' as a prefix so that I don't need lots of hacks
	{
		s = MSG_ReadStringLine ();
		if (!strcmp(s, "redir"))
		{
			netadr_t adr;
			char *data = MSG_ReadStringLine();
			Con_TPrintf (S_COLOR_GRAY"redirect to %s\n", data);
			if (NET_StringToAdr(data, PORT_DEFAULTSERVER, &adr))
			{
				if (CL_IsPendingServerAddress(&net_from))
				{
					struct dtlspeercred_s cred = {cls.servername}; //FIXME
					if (!NET_EnsureRoute(cls.sockets, "redir", &cred, data, &adr, true))
						Con_Printf (CON_ERROR"Unable to redirect to %s\n", data);
					else
					{
						connectinfo.istransfer = true;
						connectinfo.numadr = 1;
						connectinfo.adr[0] = adr;

						data = "\xff\xff\xff\xffgetchallenge\n";
						NET_SendPacket (cls.sockets, strlen(data), data, &adr);
					}
				}
			}
			return;
		}
		else if (!strcmp(s, "reject"))
		{	//generic rejection. stop trying.
			char *data = MSG_ReadStringLine();
			Con_Printf ("reject\n");
			if (CL_IsPendingServerAddress(&net_from))
				CL_ConnectAbort("%s\n", data);
			return;
		}
		else if (!strcmp(s, "badname"))
		{	//rejected purely because of player name
			if (CL_IsPendingServerAddress(&net_from))
				CL_ConnectAbort("bad player name\n");
		}
		else if (!strcmp(s, "badaccount"))
		{	//rejected because username or password is wrong
			if (CL_IsPendingServerAddress(&net_from))
				CL_ConnectAbort("invalid username or password\n");
		}
		
		Con_Printf ("f%s\n", s);
		return;
	}

	if (c == S2C_CHALLENGE)
	{
		static unsigned int lasttime = 0xdeadbeef;
		static netadr_t lastadr;
		unsigned int curtime = Sys_Milliseconds();
#ifdef HAVE_DTLS
		int candtls = 0;	//0=no,1=optional,2=mandatory
#endif

		s = MSG_ReadString ();
		COM_Parse(s);

#ifdef Q3CLIENT
		if (!strcmp(com_token, "onnectResponse"))
		{
			connectinfo.protocol = CP_QUAKE3;
			CL_ConnectionlessPacket_Connection(s);
			return;
		}
#endif
#ifdef Q2CLIENT
		if (!strcmp(com_token, "lient_connect"))
		{
			connectinfo.protocol = CP_QUAKE2;
			CL_ConnectionlessPacket_Connection(s);
			return;
		}
#endif

		Con_TPrintf (S_COLOR_GRAY"challenge\n");

		if (!CL_IsPendingServerAddress(&net_from))
		{
			if (net_from.prot != NP_RTC_TCP && net_from.prot != NP_RTC_TLS)
				Con_Printf(CON_WARNING"Challenge from wrong server, ignoring\n");
			return;
		}
		connectinfo.numadr = 1;
		connectinfo.adr[0] = net_from; //lock in only this specific address.

		if (!strcmp(com_token, "hallengeResponse"))
		{
			/*Quake3 - "\xff\xff\xff\xffchallengeResponse challenge [clchallenge protover]" (no \n)*/
#ifdef Q3CLIENT
			if (connectinfo.protocol == CP_QUAKE3 || connectinfo.protocol == CP_UNKNOWN)
			{
				/*throttle*/
				if (curtime - lasttime < 500)
					return;
				lasttime = curtime;

				memset(&connectinfo.ext, 0, sizeof(connectinfo.ext));

				connectinfo.protocol = CP_QUAKE3;
				connectinfo.challenge = atoi(s+17);
				CL_SendConnectPacket (&net_from);
			}
			else
			{
				Con_Printf("\nChallenge from another protocol, ignoring Q3 challenge\n");
				return;
			}
			return;
#else
			Con_Printf("\nUnable to connect to Quake3\n");
			return;
#endif
		}
		else if (!strcmp(com_token, "hallenge"))
		{
			/*Quake2 or Darkplaces*/
			char *s2;

			for (s2 = s+9; *s2; s2++)
			{
				if ((*s2 < '0' || *s2 > '9') && *s2 != '-')
					break;
			}
			if (!strncmp(s2, "FTE", 3) || !strncmp(s2, "QW", 2))
			{	//hack to work around NQ+QW+DP servers that reply with both qw and dp challenge requests.
				//we DON'T want to treat it as a dp server. because then we end up with nq-based protocols.
				return;
			}
			else if (*s2 && *s2 != ' ')
			{//and if it's not, we're unlikly to be compatible with whatever it is that's talking at us.
#ifdef NQPROT
				if (connectinfo.protocol == CP_NETQUAKE || connectinfo.protocol == CP_UNKNOWN)
				{
					/*throttle*/
					if (curtime - lasttime < 500)
						return;
					lasttime = curtime;

					connectinfo.protocol = CP_NETQUAKE;
					CL_ConnectToDarkPlaces(s+9, &net_from);
				}
				else
					Con_Printf("\nChallenge from another protocol, ignoring DP challenge\n");
#else
				Con_Printf("\nUnable connect to DarkPlaces\n");
#endif
				return;
			}

#ifdef Q2CLIENT
			if (connectinfo.protocol == CP_QUAKE2 || connectinfo.protocol == CP_UNKNOWN)
			{
				connectinfo.protocol = CP_QUAKE2;
				if (connectinfo.mode == CIM_Q2EONLY)
					connectinfo.subprotocol = PROTOCOL_VERSION_Q2EX;
				else
					connectinfo.subprotocol = PROTOCOL_VERSION_Q2;
			}
			else
			{
				Con_Printf("\nChallenge from another protocol, ignoring Q2 challenge\n");
				return;
			}
#else
			Con_Printf("\nUnable to connect to Quake2\n");
			return;
#endif
			s+=9;
		}

		/*no idea, assume a QuakeWorld challenge response ('c' packet)*/

		else if (connectinfo.protocol == CP_QUAKEWORLD || connectinfo.protocol == CP_UNKNOWN)
		{
			connectinfo.protocol = CP_QUAKEWORLD;
			connectinfo.subprotocol = PROTOCOL_VERSION_QW;
		}
		else
		{
			Con_Printf("\nChallenge from another protocol, ignoring QW challenge\n");
			return;
		}

		s = COM_Parse(s);	//read the challenge.
		/*throttle connect requests*/
		if (curtime - lasttime < 500 && NET_CompareAdr(&net_from, &lastadr) && connectinfo.challenge == atoi(com_token))
			return;
		lasttime = curtime;
		lastadr = net_from;
		connectinfo.challenge = atoi(com_token);
		memset(&connectinfo.ext, 0, sizeof(connectinfo.ext));

		while((s = COM_Parse(s)))
		{
			if (connectinfo.protocol == CP_QUAKE2 && !strncmp(com_token, "p=", 2))
			{
				char *p = com_token+2;
				do
				{
					switch(strtoul(p, &p, 0))
					{
					case PROTOCOL_VERSION_R1Q2:
#ifdef AVAIL_ZLIB		//r1q2 will typically send us compressed data, which is a problem if we can't handle that (q2pro has a way to disable it).
						if (connectinfo.subprotocol < PROTOCOL_VERSION_R1Q2)
							connectinfo.subprotocol = PROTOCOL_VERSION_R1Q2;
#endif
						break;
					case PROTOCOL_VERSION_Q2PRO:
						if (connectinfo.subprotocol < PROTOCOL_VERSION_Q2PRO)
							connectinfo.subprotocol = PROTOCOL_VERSION_Q2PRO;
						break;
					case PROTOCOL_VERSION_Q2EX:
						if (connectinfo.subprotocol < PROTOCOL_VERSION_Q2EX)
							connectinfo.subprotocol = PROTOCOL_VERSION_Q2EX;
						break;
					}
				} while (*p++ == ',');
			}
		}

		//if its over q2e's lan layer then pretend there was a p=2023 hint in there...
		if (connectinfo.protocol == CP_QUAKE2 && net_from.prot == NP_KEXLAN)
			if (connectinfo.subprotocol < PROTOCOL_VERSION_Q2EX)
				connectinfo.subprotocol = PROTOCOL_VERSION_Q2EX;

		for(;;)
		{
			int cmd = MSG_ReadLong ();
			if (msg_badread)
				break;
			if (cmd == PROTOCOL_VERSION_VARLENGTH)
			{
				int len = MSG_ReadLong();
				if (len < 0 || len > 8192)
					break;
				c = MSG_ReadLong();/*ident*/
				switch(c)
				{
				case PROTOCOL_INFO_GUID:
					if (len > sizeof(connectinfo.ext.guidsalt)-1)
					{
						MSG_ReadData(connectinfo.ext.guidsalt, sizeof(connectinfo.ext.guidsalt));
						MSG_ReadSkip(len-sizeof(connectinfo.ext.guidsalt));
						len = sizeof(connectinfo.ext.guidsalt)-1;
					}
					else
						MSG_ReadData(connectinfo.ext.guidsalt, len);
					connectinfo.ext.guidsalt[len] = 0;
					break;
				default:
					MSG_ReadSkip(len); /*payload*/
					break;
				}
			}
			else
			{
				unsigned int l = MSG_ReadLong();
				switch(cmd)
				{
				case PROTOCOL_VERSION_FTE1:			connectinfo.ext.fte1 = l;		break;
				case PROTOCOL_VERSION_FTE2:			connectinfo.ext.fte2 = l;		break;
				case PROTOCOL_VERSION_EZQUAKE1:		connectinfo.ext.ez1 = l;		break;
				case PROTOCOL_VERSION_FRAGMENT:		connectinfo.ext.mtu = l;		break;
#ifdef HAVE_DTLS
				case PROTOCOL_VERSION_DTLSUPGRADE:	candtls = l;	break;	//0:not enabled. 1:explicit use allowed. 2:favour it. 3: require it
#endif
#ifdef HUFFNETWORK
				case PROTOCOL_VERSION_HUFFMAN:		connectinfo.ext.compresscrc = l;	break;
#endif
				case PROTOCOL_INFO_GUID:			Q_snprintfz(connectinfo.ext.guidsalt, sizeof(connectinfo.ext.guidsalt), "0x%x", l);	break;
				default:
					break;
				}
			}
		}

#ifdef HAVE_DTLS
		if ((candtls && net_enable_dtls.ival) && net_from.prot == NP_DGRAM && (connectinfo.peercred.hash || net_enable_dtls.ival>1 || candtls > 1) && !NET_IsEncrypted(&net_from))
		{
			//c2s getchallenge			<no client details, only leaks that its quakelike, something you can maybe guess from port numbers>
			//s2c c%u\0DTLS=$candtls	<may leak server details>
			//<<YOU ARE HERE>>
			//c2s dtlsconnect %u [REALTARGET]	<FIXME: target server is plain text, not entirely unlike tls1.2, but still worse than a vpn and could be improved>
			//s2c dtlsopened			<no details at all, other than that the server is now willing to accept dtls handshakes etc>
			//c2s DTLS(getchallenge)	<start here if you're using dtls:// scheme>
			//DTLS(etc)

			//NOTE: the dtlsconnect/dtlsopened parts are redundant and the non-dtls parts are now entirely optional (and should be skipped if the client requries/knows the server supports dtls)
			//the challenge response includes server capabilities, so we still need the getchallenge/response part of the handshake despite dtls making the actual challenge part redundant.

			//getchallenge has to be done twice, with the outer one only reporting whether dtls can/should be used.
			//this means the actual connect packet is already over dtls, which protects the user's userinfo.
			//FIXME: do rcon via dtls too, but requires tracking pending rcon packets until the handshake completes.

			//server says it can do dtls, but will still need to ask it to allocate extra resources for us (I hadn't gotten dtls cookies working properly at that point).

			if (net_enable_dtls.ival>0)
			{
				char *pkt;
				//qwfwd proxy routing. it doesn't support it yet, but hey, if its willing to forward the dtls packets its all good.
				char *at;
				if ((at = strrchr(cls.servername, '@')) && !strchr(cls.servername, '/'))
				{
					*at = 0;
					pkt = va("%c%c%c%c""dtlsconnect %i %s", 255, 255, 255, 255, connectinfo.challenge, cls.servername);
					*at = '@';
				}
				else
					pkt = va("%c%c%c%c""dtlsconnect %i", 255, 255, 255, 255, connectinfo.challenge);
				NET_SendPacket (cls.sockets, strlen(pkt), pkt, &net_from);
				return;
			}
			else if (candtls >= 3)
			{
				Cvar_Set(&cl_disconnectreason, va("DTLS is disabled, but server requires it. not connecting\n"));
				connectinfo.trying = false;
				Con_Printf("DTLS is disabled, but server requires it. Set ^[/net_enable_dtls 1^] before connecting again.\n");
				return;
			}
		}
		if (net_enable_dtls.ival>=3 && !NET_IsEncrypted(&net_from))
		{
			Cvar_Set(&cl_disconnectreason, va("Server does not support/allow dtls. not connecting\n"));
			connectinfo.trying = false;
			Con_Printf("Server does not support/allow dtls. not connecting.\n");
			return;
		}
#endif

		CL_SendConnectPacket (&net_from);
		return;
	}
#ifdef Q2CLIENT
	if (connectinfo.protocol == CP_QUAKE2)
	{
		char *nl;
		MSG_ReadSkip(-1);
		c = MSG_GetReadCount();
		s = MSG_ReadString ();
		nl = strchr(s, '\n');
		if (nl)
		{
			MSG_ReadSkip(c + nl-s + 1 - MSG_GetReadCount());
			msg_badread = false;
			*nl = '\0';
		}

		COM_Parse(s);
		if (!strcmp(com_token, "print"))
		{
			Con_TPrintf (S_COLOR_GRAY"print\n");

			s = MSG_ReadString ();
			if (connectinfo.trying && CL_IsPendingServerBaseAddress(&net_from) == false)
				Cvar_Set(&cl_disconnectreason, s);
			Con_Printf ("%s", s);
			return;
		}
		else if (!strcmp(com_token, "client_connect"))
		{
			connectinfo.protocol = CP_QUAKE2;
			CL_ConnectionlessPacket_Connection(s);
			return;
		}
		else if (!strcmp(com_token, "disconnect"))
		{
			if (NET_CompareAdr(&net_from, &cls.netchan.remote_address))
			{
				Cvar_Set(&cl_disconnectreason, "Disconnect request from server");
				Con_Printf ("disconnect\n");
				CL_Disconnect_f();
				return;
			}
			else
			{
				Con_Printf("Ignoring random disconnect command\n");
				return;
			}
		}
		else
		{
			Con_TPrintf ("unknown connectionless packet for q2:  %s\n", s);
			MSG_ReadSkip(c - MSG_GetReadCount());
			c = MSG_ReadByte();
		}
	}
#endif

#ifdef NQPROT
	if (c == 'a')
	{
		s = MSG_ReadString ();
		COM_Parse(s);
		if (!strcmp(com_token, "ccept"))
		{
			/*this is a DP server... but we don't know which version nor nq protocol*/
			Con_Printf (S_COLOR_GRAY"accept\n");
			if (cls.state == ca_connected)
				return;	//we're already connected. don't do it again!

			if (!CL_IsPendingServerAddress(&net_from))
			{
				//if (net_from.type != NA_LOOPBACK)
				Con_TPrintf ("ignoring connection\n");
				return;
			}

			Validation_Apply_Ruleset();
			Netchan_Setup(NCF_CLIENT, &cls.netchan, &net_from, connectinfo.qport, 0);
			CL_ParseEstablished();

			cls.netchan.isnqprotocol = true;
			cls.protocol = CP_NETQUAKE;
			cls.protocol_nq = CPNQ_ID;	//assume vanilla protocol until we know better.
			cls.proquake_angles_hack = false;
			cls.challenge = connectinfo.challenge;
			connectinfo.trying = false;

			cls.demonum = -1;			// not in the demo loop now
			cls.state = ca_connected;


			SCR_BeginLoadingPlaque();
			return;
		}
	}

	if (c == 'i')
	{
		if (!strncmp(net_message.data+4, "infoResponse\n", 13))
		{
			Con_TPrintf (S_COLOR_GRAY"infoResponse\n");
			Info_Print(net_message.data+17, "");
			return;
		}
	}
	if (c == 'g')
	{
		if (!strncmp(net_message.data+4, "getserversResponse", 18))
		{
			qbyte *b = net_message.data+4+18;
			Con_TPrintf (S_COLOR_GRAY"getserversResponse\n");
			while (b+7 <= net_message.data+net_message.cursize)
			{
				if (*b == '\\')
				{
					b+=1;
					Con_Printf("%u.%u.%u.%u:%u\n", b[0], b[1], b[2], b[3], b[5]|(b[4]<<8));
					b+=6;
				}
				else if (*b == '/')
				{
					b+=1;
					Con_Printf("[%04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x]:%u\n", (b[0]<<8)|b[1], (b[2]<<8)|b[3], (b[4]<<8)|b[5], (b[6]<<8)|b[7], (b[8]<<8)|b[9], (b[10]<<8)|b[11], (b[12]<<8)|b[13], (b[14]<<8)|b[15], b[17]|(b[16]<<8));
					b+=18;
				}
			}
			return;
		}
	}
#endif

	if (c == 'd'/*M2C_MASTER_REPLY*/)
	{
		s = MSG_ReadString ();
		COM_Parse(s);

		if (!strcmp(com_token, "isconnect"))
		{
			Con_Printf("Disconnect\n");
			if (CL_IsPendingServerAddress(&net_from))
			{
				Cvar_Set(&cl_disconnectreason, "Disconnect request from server");
				CL_Disconnect_f();
			}
		}
		else if (!strcmp(com_token, "tlsopened"))
		{	//server is letting us know that its now listening for a dtls handshake.
#ifdef HAVE_DTLS
			dtlscred_t cred;
			Con_Printf (S_COLOR_GRAY"dtlsopened\n");
			if (!CL_IsPendingServerAddress(&net_from))
				return;

			memset(&cred, 0, sizeof(cred));
			cred.peer = connectinfo.peercred;
			if (NET_DTLS_Create(cls.sockets, &net_from, &cred, true))
			{
				connectinfo.numadr = 1;	//fixate on this resolved address.
				connectinfo.adr[0] = net_from;
				connectinfo.adr[0].prot = NP_DTLS;

				connectinfo.time = 0;	//send a new challenge NOW.
			}
			else
				CL_ConnectAbort("Unable to initialise dtls driver. You may need to adjust tls_provider or disable dtls with ^[/net_enable_dtls 0^]\n");	//this is a local issue, and not a result on remote packets.
#else
			Con_Printf ("dtlsopened (unsupported)\n");
#endif
		}
		else if (*s != '\n')
		{	//qw master server list response
			Con_Printf ("server ip list\n");
		}
		else
		{
			Con_Printf ("disconnect\n");
			if (cls.demoplayback != DPB_NONE)
			{
				Con_Printf("Disconnect\n");
				CL_Disconnect_f();
			}
		}
		return;
	}

	if (c == S2C_CONNECTION)
	{
		s = NULL;
		connectinfo.protocol = CP_QUAKEWORLD;
		connectinfo.subprotocol = PROTOCOL_VERSION_QW;

		CL_ConnectionlessPacket_Connection(NULL);
		return;
	}
#ifdef QUAKESPYAPI
	// remote command from gui front end
	if (c == A2C_CLIENT_COMMAND)	//man I hate this.
	{
		char	cmdtext[2048];

		if (net_from.type == NA_INVALID || net_from.type != net_local_cl_ipadr.type || net_from.type != NA_IP
			|| ((*(unsigned *)net_from.address.ip != *(unsigned *)net_local_cl_ipadr.address.ip) && (*(unsigned *)net_from.address.ip != htonl(INADDR_LOOPBACK))))
		{
			Con_TPrintf ("Command packet from remote host.  Ignored.\n");
			return;
		}
#if defined(_WIN32) && !defined(WINRT)
		ShowWindow (mainwindow, SW_RESTORE);
		SetForegroundWindow (mainwindow);
#endif
		s = MSG_ReadString ();

		Con_TPrintf ("client command: %s\n", s);

		Q_strncpyz(cmdtext, s, sizeof(cmdtext));

		s = MSG_ReadString ();

		while (*s && isspace(*s))
			s++;
		while (*s && isspace(s[strlen(s) - 1]))
			s[strlen(s) - 1] = 0;

		if (!allowremotecmd && (!*localid.string || strcmp(localid.string, s)))
		{
			if (!*localid.string)
			{
				Con_TPrintf ("^&C0Command packet received from local host, but no localid has been set.  You may need to upgrade your server browser.\n");
				return;
			}
			Con_TPrintf ("^&C0Invalid localid on command packet received from local host. \n|%s| != |%s|\nYou may need to reload your server browser and game.\n",
				s, localid.string);
			Cvar_Set(&localid, "");
			return;
		}

		Cbuf_AddText (cmdtext, RESTRICT_SERVER);
		allowremotecmd = false;
		return;
	}
#endif
	// print command from somewhere
	if (c == 'p')
	{
		if (!strncmp(net_message.data+4, "print\n", 6))
		{	//quake2+quake3 send rejects this way
			Con_TPrintf (S_COLOR_GRAY"print\n");
			Con_Printf ("%s", net_message.data+10);

			if (connectinfo.trying && CL_IsPendingServerBaseAddress(&net_from) == false)
				Cvar_Set(&cl_disconnectreason, net_message.data+10);
			return;
		}
	}
	if (c == A2C_PRINT)
	{	//closest quakeworld has to a reject message
		Con_TPrintf (S_COLOR_GRAY"print\n");

		s = MSG_ReadString ();
		Con_Printf ("%s", s);

		if (connectinfo.trying && CL_IsPendingServerBaseAddress(&net_from) == false)
			Cvar_Set(&cl_disconnectreason, s);
		return;
	}
	if (c == 'r')
	{	//darkplaces-style rejects
		s = MSG_ReadString ();
		Con_Printf("r%s\n", s);

		if (connectinfo.trying && CL_IsPendingServerBaseAddress(&net_from) == false)
			Cvar_Set(&cl_disconnectreason, s);
		return;
	}

//happens in demos
	if (c == svc_disconnect && cls.demoplayback != DPB_NONE && net_from.type == NA_INVALID)
	{
		CL_NextDemo();
		Host_EndGame (NULL);	//end of demo.
		return;
	}

	Con_TPrintf ("unknown connectionless packet:  %c\n", c);
}

#ifdef NQPROT
void CLNQ_ConnectionlessPacket(void)
{
	char *s;
	int length;
	unsigned short port;

	if (net_message.cursize < 5)
		return;	//not enough size to be meaningful (qe does not include a port number)

	MSG_BeginReading (&net_message, msg_nullnetprim);
	length = LongSwap(MSG_ReadLong ());
	if (!(length & NETFLAG_CTL))
		return;	//not an nq control packet.
	length &= NETFLAG_LENGTH_MASK;
	if (length != net_message.cursize)
		return;	//not an nq packet.

	switch(MSG_ReadByte())
	{
	case CCREP_ACCEPT:
		connectinfo.trying = false;
		if (cls.state >= ca_connected)
		{
			if (cls.demoplayback == DPB_NONE)
				Con_TPrintf ("Dup connect received.  Ignored.\n");
			return;
		}

		if (length == 5)
		{	//QE strips the port entirely.
			cls.proquake_angles_hack = false;
			cls.protocol_nq = CPNQ_ID;
			Con_DPrintf("QuakeEx server...\n");
		}
		else
		{
			port = htons((unsigned short)MSG_ReadLong()); //this is the port that we're meant to respond to...
			if (msg_badread)	//qe has no port specified. and that's fine when its over dtls anyway.
				port = 0;


			cls.proquake_angles_hack = false;
			cls.protocol_nq = CPNQ_ID;
			if (MSG_ReadByte() == 1)	//a proquake server adds a little extra info
			{
				int ver = MSG_ReadByte();
				int flags = MSG_ReadByte();
				Con_DPrintf("ProQuake server %i.%i\n", ver/10, ver%10);

//				if (ver >= 34)
				cls.proquake_angles_hack = true;
				if (flags & 1)
				{
					//its a 'pure' server.
					Con_Printf("pure ProQuake server\n");
					return;
				}
				if (flags & 0x80)
					port = 0;	//don't force the port.
			}

			if (port && port != net_from.port)
			{
				char buf[256];
				net_from.port = port;
				Con_Printf("redirecting to port %s\n", NET_AdrToString(buf, sizeof(buf), &net_from));
			}
		}

		Validation_Apply_Ruleset();

		cls.fteprotocolextensions = connectinfo.ext.fte1;
		cls.fteprotocolextensions2 = connectinfo.ext.fte2;
		cls.ezprotocolextensions1 = connectinfo.ext.ez1;
		Netchan_Setup (NCF_CLIENT, &cls.netchan, &net_from, connectinfo.qport, 0);
		CL_ParseEstablished();
		cls.netchan.isnqprotocol = true;
		cls.netchan.compresstable = NULL;
		cls.protocol = CP_NETQUAKE;
		cls.state = ca_connected;

		total_loading_size = 100;
		current_loading_size = 0;
		SCR_SetLoadingStage(LS_CLIENT);

#ifdef QUAKESPYAPI
		allowremotecmd = false; // localid required now for remote cmds
#endif

		if (length == 5)
			cls.qex = (connectinfo.mode==CIM_QEONLY);
		else
		{
			//send a dummy packet.
			//this makes our local firewall think we initialised the conversation, so that we can receive their packets. however this only works if our nat uses the same public port for private ports.
			Netchan_Transmit(&cls.netchan, 1, "\x01", 2500);
		}
		return;

	case CCREP_REJECT:
		s = MSG_ReadString();
		Con_Printf("Connect failed\n%s\n", s);

		if (connectinfo.trying && CL_IsPendingServerBaseAddress(&net_from) == false)
			Cvar_Set(&cl_disconnectreason, s);
		return;
	}
}
#endif

void CL_MVDUpdateSpectator (void);
void CL_WriteDemoMessage (sizebuf_t *msg, int payloadoffset);

void CL_ReadPacket(void)
{
	if (!qrenderer)
		return;

#ifdef HAVE_DTLS
	if (*(int *)net_message.data != -1)
		if (NET_DTLS_Decode(cls.sockets))
			if (!net_message.cursize)
				return;
#endif

#if defined(SUPPORT_ICE)
	if (ICE_WasStun(cls.sockets))
		return;
#endif

#ifdef NQPROT
	if (cls.demoplayback == DPB_NETQUAKE)
	{
		MSG_BeginReading (&net_message, cls.netchan.netprim);
		cls.netchan.last_received = realtime;
		CLNQ_ParseServerMessage ();
		return;
	}
#endif
#ifdef Q2CLIENT
	if (cls.demoplayback == DPB_QUAKE2)
	{
		MSG_BeginReading (&net_message, cls.netchan.netprim);
		cls.netchan.last_received = realtime;
		CLQ2_ParseServerMessage ();
		return;
	}
#endif
	//
	// remote command packet
	//
	if (*(int *)net_message.data == -1)
	{
		CL_ConnectionlessPacket ();
		return;
	}

	if (cls.state == ca_disconnected)
	{	//connect to nq servers, but don't get confused with sequenced packets.
		if (NET_WasSpecialPacket(cls.sockets))
			return;
#ifdef NQPROT
		CLNQ_ConnectionlessPacket ();
#endif
		return;	//ignore it. We arn't connected.
	}

	if (net_message.cursize < 6 && cls.demoplayback != DPB_MVD) //MVDs don't have the whole sequence header thing going on
	{
		char adr[MAX_ADR_SIZE];
		if (net_message.cursize == 1 && net_message.data[0] == A2A_ACK)
			Con_TPrintf ("%s: Ack (Pong)\n", NET_AdrToString(adr, sizeof(adr), &net_from));
		else
			Con_TPrintf ("%s: Runt packet (%i bytes)\n", NET_AdrToString(adr, sizeof(adr), &net_from), net_message.cursize);
		return;
	}

	//
	// packet from server
	//
	if (!cls.demoplayback &&
		!NET_CompareAdr (&net_from, &cls.netchan.remote_address))
	{
		char adr[MAX_ADR_SIZE];
		if (NET_WasSpecialPacket(cls.sockets))
			return;
		Con_DPrintf ("%s:sequenced packet from wrong server\n"
			,NET_AdrToString(adr, sizeof(adr), &net_from));
		return;
	}

	if (cls.netchan.flags&NCF_STUNAWARE)	//should be safe to do this here.
		if (NET_WasSpecialPacket(cls.sockets))
			return;

	switch(cls.protocol)
	{
	case CP_NETQUAKE:
#ifdef NQPROT
		switch(NQNetChan_Process(&cls.netchan))
		{
		case NQNC_IGNORED:
			break;
		case NQNC_ACK:
		case NQNC_RELIABLE:
		case NQNC_UNRELIABLE:
			MSG_ChangePrimitives(cls.netchan.netprim);
			CL_WriteDemoMessage (&net_message, MSG_GetReadCount());
			CLNQ_ParseServerMessage ();
			break;
		}
#endif
		break;
	case CP_PLUGIN:
		break;
	case CP_QUAKE2:
#ifdef Q2CLIENT
		if (!Netchan_Process(&cls.netchan))
			return;		// wasn't accepted for some reason
		CLQ2_ParseServerMessage ();
		break;
#endif
	case CP_QUAKE3:
#ifdef Q3CLIENT
		{
			cactive_t newstate = q3->cl.ParseServerMessage(&net_message);
			if (newstate != cls.state)
			{
				cls.state = newstate;
				if (cls.state == ca_active)
					CL_MakeActive("Quake3Arena");	//became active, can flush old stuff now.
			}
		}
#endif
		break;
	case CP_QUAKEWORLD:
		if (cls.demoplayback == DPB_MVD)
		{
			MSG_BeginReading(&net_message, cls.netchan.netprim);
			cls.netchan.last_received = realtime;
			cls.netchan.outgoing_sequence = cls.netchan.incoming_sequence;
		}
		else if (!Netchan_Process(&cls.netchan))
			return;		// wasn't accepted for some reason

		CL_WriteDemoMessage (&net_message, MSG_GetReadCount());

		if (cls.netchan.incoming_sequence > cls.netchan.outgoing_sequence)
		{	//server should not be responding to packets we have not sent yet
			Con_DPrintf("Server is from the future! (%i packets)\n", cls.netchan.incoming_sequence - cls.netchan.outgoing_sequence);
			cls.netchan.outgoing_sequence = cls.netchan.incoming_sequence;
		}
		MSG_ChangePrimitives(cls.netchan.netprim);
		CLQW_ParseServerMessage ();
		break;
	case CP_UNKNOWN:
		break;
	}
}
/*
=================
CL_ReadPackets
=================
*/
void CL_ReadPackets (void)
{
	if	(cls.demoplayback != DPB_NONE)
	{
		while(CL_GetDemoMessage())
			CL_ReadPacket();
	}
	else
		NET_ReadPackets(cls.sockets);

	//
	// check timeout
	//
	if (cls.state >= ca_connected
	 && realtime - cls.netchan.last_received > cl_timeout.value && !cls.demoplayback)
	{
#ifdef HAVE_SERVER
		/*don't timeout when we're the actual server*/
		if (!sv.state)
#endif
		{
			Con_TPrintf ("\nServer connection timed out.\n");
			CL_Disconnect ("Connection Timed Out");
			return;
		}
	}

	if (cls.demoplayback == DPB_MVD)
	{
		CL_MVDUpdateSpectator();
	}
}

//=============================================================================

qboolean CL_AllowArbitaryDownload(const char *oldname, const char *localfile)
{
	int allow;
	//never allow certain (native code) arbitary downloads.
	if (!Q_strncasecmp(localfile, "game", 4) ||	//q2-ey things
		!Q_strcasecmp(localfile, "progs.dat") || !Q_strcasecmp(localfile, "menu.dat") || !Q_strcasecmp(localfile, "csprogs.dat") || !Q_strcasecmp(localfile, "qwprogs.dat") || //overriding gamecode is bad (csqc should be dlcached)
		strstr(localfile, "\\") || strstr(localfile, "..") || strstr(localfile, "./") || strstr(localfile, ":") || strstr(localfile, "//") ||	//certain path patterns are just bad
		Q_strcasestr(localfile, ".qvm") || Q_strcasestr(localfile, ".dll") || Q_strcasestr(localfile, ".so") || Q_strcasestr(localfile, ".dylib"))	//disallow any native code
	{	//yes, I know the user can use a different progs from the one that is specified. If you leave it blank there will be no problem. (server isn't allowed to stuff progs cvar)
		Con_Printf("Ignoring arbitrary download to \"%s\" due to possible security risk\n", localfile);
		return false;
	}
	allow = cl_download_redirection.ival;
	if (allow == 2)
	{
		char ext[8];
		COM_FileExtension(localfile, ext, sizeof(ext));
		if (!strncmp(localfile, "demos/", 6) && (!Q_strcasecmp(ext, "mvd") || !Q_strcasecmp(ext, "gz")))
			return true;	//mvdsv popularised the server sending 'download demo/foobar.mvd' in response to 'download demonum/5' aka 'cmd dl #'
		else if (!strncmp(localfile, "package/", 8) && (!Q_strcasecmp(ext, "pak") || !Q_strcasecmp(ext, "pk3") || !Q_strcasecmp(ext, "pk4")))
			return true;	//packages, woo.
							//fixme: we should probably try using package/$gamedir/foo.pak if we get redirected to that.
		else
		{
			Con_Printf("Ignoring non-package download redirection to \"%s\"\n", localfile);
			return false;
		}
	}
	if (allow)
		return true;
	Con_Printf("Ignoring download redirection to \"%s\". This server may require you to set cl_download_redirection to 2.\n", localfile);
	return false;
}

#if defined(NQPROT) && defined(HAVE_LEGACY)
//this is for DP compat.
static void CL_Curl_f(void)
{
	//curl --args url
	int i, argc = Cmd_Argc();
	const char *arg, *gamedir, *localterse/*no dlcache*/= NULL;
	char localname[MAX_QPATH];
	char localnametmp[MAX_QPATH];
	int usage = 0;
	qboolean alreadyhave = false;
	extern char *cl_dp_packagenames;
	unsigned int dlflags = DLLF_VERBOSE|DLLF_ALLOWWEB;
	const char *ext;
	if (argc < 2)
	{
		Con_Printf("%s: No args\n", Cmd_Argv(0));
		return;
	}
//	Con_Printf("%s %s\n", Cmd_Argv(0), Cmd_Args());
	for (i = 1; i < argc; i++)
	{
		arg = Cmd_Argv(i);
		if (!strcmp(arg, "--info"))
		{
			Con_Printf("%s %s: not implemented\n", Cmd_Argv(0), arg);
			return;
		}
		else if (!strcmp(arg, "--cancel"))
		{
			Con_Printf("%s %s: not implemented\n", Cmd_Argv(0), arg);
			return;
		}
		else if (!strcmp(arg, "--pak"))
			usage |= 1;
		else if (!strcmp(arg, "--cachepic"))
			usage |= 2;
		else if (!strcmp(arg, "--skinframe"))
			usage |= 4;
		else if (!strcmp(arg, "--for"))
		{
			alreadyhave = true;	//assume we have a package that satisfies the file name.
			for (i++; i < argc-1; i++)	//all but the last...
			{
				arg = Cmd_Argv(i);
				if (!CL_CheckDLFile(arg))
				{
					alreadyhave = false;	//I guess we didn't after all.
					break;
				}
			}
		}
		else if (!strcmp(arg, "--forthismap"))
		{
			//'don't reconnect on failure'
			//though I'm guessing its better expressed as just flagging it as mandatory.
			dlflags |= DLLF_REQUIRED;
		}
		else if (!strcmp(arg, "--as"))
		{
			//explicit local filename
			localterse = Cmd_Argv(++i);
		}
		else if (!strcmp(arg, "--clear_autodownload"))
		{
			Z_Free(cl_dp_packagenames);
			cl_dp_packagenames = NULL;
			return;
		}
		else if (!strcmp(arg, "--finish_autodownload"))
		{
			//not really sure why this is needed
//			Con_Printf("%s %s: not implemented\n", Cmd_Argv(0), arg);
			return;
		}
		else if (!strcmp(arg, "--maxspeed="))
			;
		else if (*arg == '-')
			Con_Printf("%s: Unknown option %s\n", Cmd_Argv(0), arg);
		else
			;	//usually just the last arg, but may also be some parameter for an unknown arg.
	}
	arg = Cmd_Argv(argc-1);
	if (!localterse)
	{
		char *t;
		localterse = strrchr(arg, '/');
		if (!localterse)
			localterse = arg;
		t = strchr(localterse, '?');
		if (t)
			*t = 0;
		if (t-localterse < countof(localnametmp))
		{
			memcpy(localnametmp, localterse, t-localterse);
			localnametmp[t-localterse] = 0;
			localterse = localnametmp;
		}
	}
	if (!localterse)
	{
		//for compat, we should look for the last / and truncate on a ?.
		Con_Printf("%s: skipping download of %s, as the local name was not explicitly given\n", Cmd_Argv(0), arg);
		return;
	}
	ext = COM_GetFileExtension(localterse, NULL);
	if (usage == 1 && (!strcmp(ext, ".pk3") || !strcmp(ext, ".pak")))
	{
		dlflags |= DLLF_NONGAME;
		gamedir = FS_GetGamedir(true);
		FS_GenCachedPakName(va("%s/%s", gamedir, localterse), NULL, localname, sizeof(localname));

		if (!alreadyhave)
			if (!CL_CheckOrEnqueDownloadFile(arg, localname, dlflags))
				Con_Printf("Downloading %s to %s\n", arg, localname);

		if (cl_dp_packagenames)
			Z_StrCat(&cl_dp_packagenames, va("%s%s/%s", cl_dp_packagenames?" ":"", gamedir, localterse));
	}
	else
	{
		Con_Printf("%s: %s: non-package downloads are not supported\n", Cmd_Argv(0), arg);
		return;
	}
}
#endif

/*
=====================
CL_Download_f
=====================
*/
void CL_Download_f (void)
{
//	char *p, *q;
	char *url = Cmd_Argv(1);
	char *localname = Cmd_Argv(2);

#ifdef WEBCLIENT
	if (!strnicmp(url, "http://", 7) || !strnicmp(url, "https://", 8) || !strnicmp(url, "ftp://", 6))
	{
		if (Cmd_IsInsecure())
			return;
		if (!*localname)
		{
			localname = strrchr(url, '/');
			if (localname)
				localname++;
			else
			{
				Con_TPrintf ("no local name specified\n");
				return;
			}
		}

		HTTP_CL_Get(url, localname, NULL);//"test.txt");
		return;
	}
#endif

	if (!strnicmp(url, "qw://", 5) || !strnicmp(url, "q2://", 5))
	{
		url += 5;
		if (*url == '/')	//a conforming url should always have a host section, an empty one is simply three slashes.
			url++;
	}

	if (!*localname)
		localname = url;

	if ((cls.state == ca_disconnected || cls.demoplayback) && !(cls.demoplayback == DPB_MVD && (cls.demoeztv_ext&EZTV_DOWNLOAD)))
	{
		Con_TPrintf ("Must be connected.\n");
		return;
	}

	if (cls.netchan.remote_address.type == NA_LOOPBACK)
	{
		Con_TPrintf ("Must be connected.\n");
		return;
	}

	if (Cmd_Argc() != 2 && Cmd_Argc() != 3)
	{
		Con_TPrintf ("Usage: download <datafile> <localname>\n");
		return;
	}

	if (Cmd_IsInsecure())	//mark server specified downloads.
	{
		if (cls.download && cls.download->method == DL_QWPENDING)
			DL_Abort(cls.download, QDL_FAILED);

		//don't let gamecode order us to download random junk
		if (!CL_AllowArbitaryDownload(NULL, localname))
			return;

		CL_CheckOrEnqueDownloadFile(url, localname, DLLF_REQUIRED|DLLF_VERBOSE);
		return;
	}

	CL_EnqueDownload(url, localname, DLLF_USEREXPLICIT|DLLF_IGNOREFAILED|DLLF_REQUIRED|DLLF_OVERWRITE|DLLF_VERBOSE);
}

void CL_DownloadSize_f(void)
{
	downloadlist_t *dl;
	char *rname;
	char *size;
	char *redirection;

	//if this is a demo.. urm?
	//ignore it. This saves any spam.
	if (cls.demoplayback)
		return;

	rname = Cmd_Argv(1);
	size = Cmd_Argv(2);
	if (!strcmp(size, "e"))
	{
		Con_Printf(CON_ERROR"Download of \"%s\" failed. Not found.\n", rname);
		CL_DownloadFailed(rname, NULL, DLFAIL_SERVERFILE);
	}
	else if (!strcmp(size, "p"))
	{
		if (cls.download && stricmp(cls.download->remotename, rname))
		{
			Con_Printf(CON_ERROR"Download of \"%s\" failed. Not allowed.\n", rname);
			CL_DownloadFailed(rname, NULL, DLFAIL_SERVERCVAR);
		}
	}
	else if (!strcmp(size, "r"))
	{	//'download this file instead'
		redirection = Cmd_Argv(3);

		if (!CL_AllowArbitaryDownload(rname, redirection))
			return;

		dl = CL_DownloadFailed(rname, NULL, DLFAIL_REDIRECTED);
		Con_DPrintf("Download of \"%s\" redirected to \"%s\".\n", rname, redirection);

		if (!strncmp(redirection, "package/", 8))
		{	//redirected to a package, make sure we cache it in the proper place.
			char pkn[MAX_QPATH], pkh[32];
			char localname[MAX_QPATH];
			char *spn = cl.serverpacknames, *sph = cl.serverpackhashes;
			*pkh = 0;
			while(spn && sph)
			{
				spn=COM_ParseOut(spn, pkn, sizeof(pkn));
				sph=COM_ParseOut(sph, pkh, sizeof(pkh));
				if (!spn || !sph)
					break;
				if (!strcmp(pkn, redirection+8))
					break;
				*pkh = 0;
			}
			if (*pkh)
				if (FS_GenCachedPakName(redirection+8, pkh, localname, sizeof(localname)))
					CL_CheckOrEnqueDownloadFile(redirection+8, localname, DLLF_NONGAME);
		}
		else
			CL_CheckOrEnqueDownloadFile(redirection, NULL, dl->flags);
	}
	else
	{
		for (dl = cl.downloadlist; dl; dl = dl->next)
		{
			if (!strcmp(dl->rname, rname))
			{
				dl->size = strtoul(size, NULL, 0);
				dl->flags &= ~DLLF_SIZEUNKNOWN;
				return;
			}
		}
	}
}

void CL_FinishDownload(char *filename, char *tempname);
static void CL_ForceStopDownload (qboolean finish)
{
	qdownload_t *dl = cls.download;
	if (Cmd_IsInsecure())
	{
		Con_Printf(CON_WARNING "Execution from server rejected for %s\n", Cmd_Argv(0));
		return;
	}
	if (!dl)
		return;

	if (!dl->file)
	{
		if (dl->method == DL_QWPENDING)
			finish = false;
		else
		{
			Con_Printf("No files downloading by QW protocol\n");
			return;
		}
	}

	if (finish)
		DL_Abort(dl, QDL_COMPLETED);
	else
		DL_Abort(dl, QDL_FAILED);

	// get another file if needed
	CL_RequestNextDownload ();
}
void CL_SkipDownload_f (void)
{
	CL_ForceStopDownload(false);
}
void CL_FinishDownload_f (void)
{
	CL_ForceStopDownload(true);
}

#if defined(_WIN32) && !defined(WINRT) && !defined(_XBOX)
#include "winquake.h"
/*
=================
CL_Minimize_f
=================
*/
void CL_Windows_f (void)
{
	if (!mainwindow)
	{
		Con_Printf("Cannot comply\n");
		return;
	}
//	if (modestate == MS_WINDOWED)
//		ShowWindow(mainwindow, SW_MINIMIZE);
//	else
		SendMessage(mainwindow, WM_SYSKEYUP, VK_TAB, 1 | (0x0F << 16) | (1<<29));
}
#endif

#ifdef HAVE_SERVER
void CL_ServerInfo_f(void)
{
	if (!sv.state && cls.state && Cmd_Argc() < 2)
	{
		if (cl.haveserverinfo)
		{
			InfoBuf_Print (&cl.serverinfo, "");
			Con_Printf("[%u, %s]\n", (unsigned int)cl.serverinfo.totalsize, cls.servername);
		}
		else
			Cmd_ForwardToServer ();
	}
	else
	{
		SV_Serverinfo_f();	//allow it to be set... (whoops)
	}
}
#endif

#ifdef FTPCLIENT
void CL_FTP_f(void)
{
	FTP_Client_Command(Cmd_Args(), NULL);
}
#endif

//fixme: make a cvar
void CL_Fog_f(void)
{
	int ftype;
	vec3_t rgb;
	if (!Q_strcasecmp(Cmd_Argv(0), "waterfog"))
		ftype = FOGTYPE_WATER;
	else if (!Q_strcasecmp(Cmd_Argv(0), "skyroomfog"))
		ftype = FOGTYPE_SKYROOM;
	else //fog
		ftype = FOGTYPE_AIR;
	if ((cl.fog_locked && !Cmd_FromGamecode() && !cls.allow_cheats) || Cmd_Argc() <= 1)
	{
		static const char *fognames[FOGTYPE_COUNT]={"fog","waterfog","skyroomfog"};
		if (Cmd_ExecLevel != RESTRICT_INSECURE)
			Con_Printf("Current %s %f (r:%f g:%f b:%f, a:%f bias:%f)\n", fognames[ftype], cl.fog[ftype].density, cl.fog[ftype].colour[0], cl.fog[ftype].colour[1], cl.fog[ftype].colour[2], cl.fog[ftype].alpha, cl.fog[ftype].depthbias);
	}
	else
	{
		CL_ResetFog(ftype);
		VectorSet(rgb, 0.3,0.3,0.3);

		switch(Cmd_Argc())
		{
		case 1:
			break;
		case 2:
			cl.fog[ftype].density = atof(Cmd_Argv(1));
			break;
		case 3:
			cl.fog[ftype].density = atof(Cmd_Argv(1));
			rgb[0] = rgb[1] = rgb[2] = atof(Cmd_Argv(2));
			break;
		case 4:
			cl.fog[ftype].density = 0.05;	//make something up for vauge compat with fitzquake, so it doesn't get the default of 0
			rgb[0] = atof(Cmd_Argv(1));
			rgb[1] = atof(Cmd_Argv(2));
			rgb[2] = atof(Cmd_Argv(3));
			break;
		case 5:
		default:
			cl.fog[ftype].density = atof(Cmd_Argv(1));
			rgb[0] = atof(Cmd_Argv(2));
			rgb[1] = atof(Cmd_Argv(3));
			rgb[2] = atof(Cmd_Argv(4));
			break;
		}

		if (rgb[0]>=2 || rgb[1]>=2 || rgb[2]>=2)	//we allow SOME slop for hdr fog... hopefully we won't need it. this is mostly just an issue when skyfog is enabled[default .5] ('why is my sky white on map FOO')
			Con_Printf(CON_WARNING "Fog colour of %g %g %g exceeds standard 0-1 range\n", rgb[0], rgb[1], rgb[2]);
		cl.fog[ftype].colour[0] = SRGBf(rgb[0]);
		cl.fog[ftype].colour[1] = SRGBf(rgb[1]);
		cl.fog[ftype].colour[2] = SRGBf(rgb[2]);

		if (cls.state == ca_active)
			cl.fog[ftype].time += 1;

		//fitz:
		//if (Cmd_Argc() >= 6) cl.fog[ftype].time += atof(Cmd_Argv(5));
		//dp:
		if (Cmd_Argc() >= 6) cl.fog[ftype].alpha = atof(Cmd_Argv(5));
		if (Cmd_Argc() >= 7) cl.fog[ftype].depthbias = atof(Cmd_Argv(6));
		//if (Cmd_Argc() >= 8) cl.fog[ftype].end = atof(Cmd_Argv(7));
		//if (Cmd_Argc() >= 9) cl.fog[ftype].height = atof(Cmd_Argv(8));
		//if (Cmd_Argc() >= 10) cl.fog[ftype].fadedepth = atof(Cmd_Argv(9));

		if (Cmd_FromGamecode())
			cl.fog_locked = !!cl.fog[ftype].density;

#ifdef HAVE_LEGACY
		if (cl.fog[ftype].colour[0] > 1 || cl.fog[ftype].colour[1] > 1 || cl.fog[ftype].colour[2] > 1)
			Con_DPrintf(CON_WARNING "Fog is oversaturated. This can result in compatibility issues.\n");
#endif
	}
}

#ifdef _DEBUG
void CL_FreeSpace_f(void)
{
	char buf[32];
	quint64_t freespace;
	const char *freepath = Cmd_Argv(1);
	if (Sys_GetFreeDiskSpace(freepath, &freespace))
		Con_Printf("%s: %s available\n", freepath, FS_AbbreviateSize(buf,sizeof(buf),freespace));
	else
		Con_Printf("%s: disk free not queryable\n", freepath);
}
#endif

void CL_CrashMeEndgame_f(void)
{
	Host_EndGame("crashme! %s", Cmd_Args());
}
void CL_CrashMeError_f(void)
{
	Sys_Error("crashme! %s", Cmd_Args());
}


static char *ShowTime(unsigned int seconds)
{
	char buf[1024];
	char *b = buf;
	*b = 0;

	if (seconds > 60)
	{
		if (seconds > 60*60)
		{
			if (seconds > 24*60*60)
			{
				strcpy(b, va("%id ", seconds/(24*60*60)));
				b += strlen(b);
				seconds %= 24*60*60;
			}

			strcpy(b, va("%ih ", seconds/(60*60)));
			b += strlen(b);
			seconds %= 60*60;
		}
		strcpy(b, va("%im ", seconds/60));
		b += strlen(b);
		seconds %= 60;
	}
	strcpy(b, va("%is", seconds));
	b += strlen(b);

	return va("%s", buf);
}
void CL_Status_f(void)
{
#ifdef CSQC_DAT
	extern world_t csqc_world;
#endif
	char adr[128];
	float pi, po, bi, bo;

	NET_PrintAddresses(cls.sockets);
	NET_PrintConnectionsStatus(cls.sockets);
	if (NET_GetRates(cls.sockets, &pi, &po, &bi, &bo))
		Con_Printf("packets,bytes/sec: in: %g %g  out: %g %g\n", pi, bi, po, bo);	//not relevent as a limit.

	if (cls.state)
	{
		char cert[8192];
		qbyte fp[DIGEST_MAXSIZE+1];
		char b64[(DIGEST_MAXSIZE*4)/3+1];
		if (NET_GetConnectionCertificate(cls.sockets, &cls.netchan.remote_address, QCERT_ISENCRYPTED, NULL, 0))
			Q_strncpyz(b64, "<UNENCRYPTED>", sizeof(b64));
		else
		{
			int sz = NET_GetConnectionCertificate(cls.sockets, &cls.netchan.remote_address, QCERT_PEERCERTIFICATE, cert, sizeof(cert));
			if (sz<0)
				Q_strncpyz(b64, "<UNAVAILABLE>", sizeof(b64));
			else
			{
				sz = Base64_EncodeBlockURI(fp, CalcHash(&hash_certfp, fp,sizeof(fp), cert, sz), b64, sizeof(b64));
				b64[sz] = 0;
			}
		}
		Con_Printf("Server address   : %s\n", NET_AdrToString(adr, sizeof(adr), &cls.netchan.remote_address));	//not relevent as a limit.
		Con_Printf("Server cert fp   : %s\n", b64);	//not relevent as a limit.

		Con_Printf("Network MTU      : %u (max %u) %s\n", cls.netchan.mtu_cur, cls.netchan.mtu_max, (cls.netchan.flags&NCF_FRAGABLE)?"":" (strict)");	//not relevent as a limit.
		switch(cls.protocol)
		{
		default:
		case CP_UNKNOWN:
			Con_Printf("Network Protocol : Unknown\n");
			break;
		case CP_QUAKEWORLD:
			Con_Printf("Network Protocol : QuakeWorld\n");
			break;
	#ifdef NQPROT
		case CP_NETQUAKE:
			switch(cls.protocol_nq)
			{
			case CPNQ_ID:
				if (cls.proquake_angles_hack)
					Con_Printf("Network Protocol : ProQuake\n");
				else
					Con_Printf("Network Protocol : NetQuake\n");
				break;
			case CPNQ_NEHAHRA:
				Con_Printf("Network Protocol : Nehahra\n");
				break;
			case CPNQ_BJP1:
				Con_Printf("Network Protocol : BJP1\n");
				break;
			case CPNQ_BJP2:
				Con_Printf("Network Protocol : BJP2\n");
				break;
			case CPNQ_BJP3:
				Con_Printf("Network Protocol : BJP3\n");
				break;
			case CPNQ_H2MP:
				Con_Printf("Network Protocol : H2MP\n");
				break;
			case CPNQ_FITZ666:
				Con_Printf("Network Protocol : FitzQuake\n");
				break;
			case CPNQ_DP5:
				Con_Printf("Network Protocol : DPP5\n");
				break;
			case CPNQ_DP6:
				Con_Printf("Network Protocol : DPP6\n");
				break;
			case CPNQ_DP7:
				Con_Printf("Network Protocol : DPP7\n");
				break;
			}
			break;
	#endif
	#ifdef Q2CLIENT
		case CP_QUAKE2:
			switch (cls.protocol_q2)
			{
			case PROTOCOL_VERSION_Q2:
				Con_Printf("Network Protocol : Quake2\n");
				break;
			case PROTOCOL_VERSION_R1Q2:
				Con_Printf("Network Protocol : R1Q2\n");
				break;
			case PROTOCOL_VERSION_Q2PRO:
				Con_Printf("Network Protocol : Q2Pro\n");
				break;
			case PROTOCOL_VERSION_Q2EXDEMO:
			case PROTOCOL_VERSION_Q2EX:
				Con_Printf("Network Protocol : Quake2Ex\n");
				break;
			default:
				Con_Printf("Network Protocol : Quake2 (OLD)\n");
				break;
			}
			break;
	#endif
	#ifdef Q3CLIENT
		case CP_QUAKE3:
			Con_Printf("Network Protocol : Quake3\n");
			break;
	#endif
	#ifdef PLUGINS
		case CP_PLUGIN:
			Con_Printf("Network Protocol : (unknown, provided by plugin)\n");
			break;
	#endif
		}

		//just show the more interesting extensions.
		if (cls.fteprotocolextensions & PEXT_FLOATCOORDS)
			Con_Printf("\textended coords\n");
		if (cls.fteprotocolextensions & PEXT_SPLITSCREEN)
			Con_Printf("\tsplit screen\n");
		if (cls.fteprotocolextensions & PEXT_CSQC)
			Con_Printf("\tcsqc info\n");
		if (cls.fteprotocolextensions2 & PEXT2_VOICECHAT)
			Con_Printf("\tvoice chat\n");
		if (cls.fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS)
			Con_Printf("\treplacement deltas\n");
		if (cls.fteprotocolextensions2 & PEXT2_VRINPUTS)
			Con_Printf("\tvrinputs\n");
		if (cls.fteprotocolextensions2 & PEXT2_INFOBLOBS)
			Con_Printf("\tinfoblobs\n");
	}

	if (cl.worldmodel)
	{
		Con_Printf("map uptime       : %s\n", ShowTime(cl.time));
		COM_FileBase(cl.worldmodel->name, adr, sizeof(adr));
		Con_Printf ("current map      : %s (%s)\n", adr, cl.levelname);
	}

#ifdef CSQC_DAT
	if (csqc_world.progs)
	{
		extern int num_sfx;
		int count = 0, i;
		edict_t *e;
		Con_Printf ("csqc             : loaded\n");
		for (i = 0; i < csqc_world.num_edicts; i++)
		{
			e = EDICT_NUM_PB(csqc_world.progs, i);
			if (e && e->ereftype == ER_FREE && Sys_DoubleTime() - e->freetime > 0.5)
				continue;	//free, and older than the zombie time
			count++;
		}
		Con_Printf("csqc entities    : %i/%i/%i (mem: %.1f%%)\n", count, csqc_world.num_edicts, csqc_world.max_edicts, 100*(float)(csqc_world.progs->stringtablesize/(double)csqc_world.progs->stringtablemaxsize));
		for (count = 1; count < MAX_CSMODELS; count++)
			if (!*cl.model_csqcname[count])
				break;
		Con_Printf("csqc models      : %i/%i\n", count, MAX_CSMODELS);
		Con_Printf("client sounds    : %i\n", num_sfx);	//there is a limit, its just private. :(

		for (count = 1; count < MAX_SSPARTICLESPRE; count++)
			if (!cl.particle_csname[count])
				break;
		if (count!=1)
			Con_Printf("csqc particles   : %i/%i\n", count, MAX_CSPARTICLESPRE);
		if (cl.csqcdebug)
			Con_Printf("csqc debug       : true\n");
	}
	else
		Con_Printf ("csqc             : not loaded\n");
#endif
	Con_Printf("gamedir          : %s\n", FS_GetGamedir(true));
}

void CL_Demo_SetSpeed_f(void)
{
	char *s = Cmd_Argv(1);
	if (s)
	{
		float f = atof(s)/100;
		Cvar_SetValue(&cl_demospeed, f);
	}
	else
		Con_Printf("demo playback speed %g%%\n", cl_demospeed.value * 100);
}

static void CL_UserinfoChanged(void *ctx, const char *keyname)
{
	InfoSync_Add(&cls.userinfosync, ctx, keyname);
}


void CL_Skygroup_f(void);
void WAD_ImageList_f(void);
/*
=================
CL_Init
=================
*/
void CL_Init (void)
{
	extern void CL_Say_f (void);
	extern void CL_SayMe_f (void);
	extern void CL_SayTeam_f (void);
#ifdef QWSKINS
	extern	cvar_t		baseskin;
	extern	cvar_t		noskins;
#endif
	char *ver;
	size_t seat;

	cls.state = ca_disconnected;
	cls.demotrack = -1;
	cls.demonum = -1;

#ifdef SVNREVISION
	if (strcmp(STRINGIFY(SVNREVISION), "-"))
		ver = va("%s v%i.%02i %s", DISTRIBUTION, FTE_VER_MAJOR, FTE_VER_MINOR, STRINGIFY(SVNREVISION));
	else
#endif
		ver = va("%s v%i.%02i", DISTRIBUTION, FTE_VER_MAJOR, FTE_VER_MINOR);

	for (seat = 0; seat < MAX_SPLITS; seat++)
	{
		cls.userinfo[seat].ChangeCTX = &cls.userinfo[seat];
		cls.userinfo[seat].ChangeCB = CL_UserinfoChanged;
		InfoBuf_SetStarKey (&cls.userinfo[seat], "*ver", ver);
	}

	InitValidation();

	CL_InitInput ();
	CL_InitTEnts ();
	CL_InitPrediction ();
	CL_InitCam ();
	CL_InitDlights();
	PM_Init ();
	TP_Init();

//
// register our commands
//
	CLSCR_Init();
#ifdef MENU_DAT
	MP_RegisterCvarsAndCmds();
#endif
#ifdef CSQC_DAT
	CSQC_RegisterCvarsAndThings();
#endif
	Cvar_Register (&host_speeds, cl_controlgroup);

	Cvar_Register (&cfg_save_name, cl_controlgroup);

	Cvar_Register (&cl_disconnectreason, cl_controlgroup);
	Cvar_Register (&cl_proxyaddr, cl_controlgroup);
	Cvar_Register (&cl_sendguid, cl_controlgroup);
	Cvar_Register (&cl_defaultport, cl_controlgroup);
	Cvar_Register (&cl_servername, cl_controlgroup);
	Cvar_Register (&cl_serveraddress, cl_controlgroup);
	Cvar_Register (&cl_demospeed, "Demo playback");
	Cmd_AddCommand("demo_setspeed", CL_Demo_SetSpeed_f);
	Cvar_Register (&cl_upspeed, cl_inputgroup);
	Cvar_Register (&cl_forwardspeed, cl_inputgroup);
	Cvar_Register (&cl_backspeed, cl_inputgroup);
	Cvar_Register (&cl_sidespeed, cl_inputgroup);
	Cvar_Register (&cl_movespeedkey, cl_inputgroup);
	Cvar_Register (&cl_yawspeed, cl_inputgroup);
	Cvar_Register (&cl_pitchspeed, cl_inputgroup);
	Cvar_Register (&cl_anglespeedkey, cl_inputgroup);
	Cvar_Register (&cl_shownet,	cl_screengroup);
	Cvar_Register (&cl_sbar,	cl_screengroup);
	Cvar_Register (&cl_pure,	cl_screengroup);
	Cvar_Register (&cl_hudswap,	cl_screengroup);
	Cvar_Register (&cl_maxfps,	cl_screengroup);
	Cvar_Register (&cl_maxfps_slop,	cl_screengroup);
	Cvar_Register (&cl_idlefps, cl_screengroup);
	Cvar_Register (&cl_yieldcpu, cl_screengroup);
	Cvar_Register (&cl_timeout, cl_controlgroup);
	Cvar_Register (&cl_vrui_force, cl_controlgroup);
	Cvar_Register (&cl_vrui_lock, cl_controlgroup);
	Cvar_Register (&lookspring, cl_inputgroup);
	Cvar_Register (&lookstrafe, cl_inputgroup);
	Cvar_Register (&sensitivity, cl_inputgroup);

	Cvar_Register (&m_pitch, cl_inputgroup);
	Cvar_Register (&m_yaw, cl_inputgroup);
	Cvar_Register (&m_forward, cl_inputgroup);
	Cvar_Register (&m_side, cl_inputgroup);

	Cvar_Register (&cl_crypt_rcon,	cl_controlgroup);
	Cvar_Register (&rcon_password,	cl_controlgroup);
	Cvar_Register (&rcon_address,	cl_controlgroup);

	Cvar_Register (&cl_lerp_maxinterval, cl_controlgroup);
	Cvar_Register (&cl_lerp_maxdistance, cl_controlgroup);
	Cvar_Register (&cl_lerp_players, cl_controlgroup);
	Cvar_Register (&cl_predict_players,	cl_predictiongroup);
	Cvar_Register (&cl_predict_players_frac,	cl_predictiongroup);
	Cvar_Register (&cl_predict_players_latency,	cl_predictiongroup);
	Cvar_Register (&cl_predict_players_nudge,	cl_predictiongroup);
	Cvar_Register (&cl_solid_players,	cl_predictiongroup);

#ifdef QUAKESPYAPI
	Cvar_Register (&localid,	cl_controlgroup);
#endif

	Cvar_Register (&cl_muzzleflash, cl_controlgroup);

#ifdef QWSKINS
	Cvar_Register (&baseskin,	"Teamplay");
	Cvar_Register (&noskins,	"Teamplay");
#endif
	Cvar_Register (&cl_noblink,	"Console controls");	//for lack of a better group

	Cvar_Register (&cl_item_bobbing, "Item effects");
	Cvar_Register (&gl_simpleitems, "Item effects");

	Cvar_Register (&cl_staticsounds, "Item effects");

	Cvar_Register (&r_torch, "Item effects");
	Cvar_Register (&r_rocketlight, "Item effects");
	Cvar_Register (&r_lightflicker, "Item effects");
	Cvar_Register (&cl_r2g, "Item effects");
	Cvar_Register (&r_powerupglow, "Item effects");
	Cvar_Register (&v_powerupshell, "Item effects");

	Cvar_Register (&cl_gibfilter, "Item effects");
	Cvar_Register (&cl_deadbodyfilter, "Item effects");

	Cvar_Register (&cl_nolerp, "Item effects");
#ifdef NQPROT
	Cvar_Register (&cl_nolerp_netquake, "Item effects");
	Cvar_Register (&cl_fullpitch_nq, "Cheats");
#endif

	Cvar_Register (&r_drawflame, "Item effects");

	Cvar_Register (&cl_downloads, cl_controlgroup);
	Cvar_Register (&cl_download_csprogs, cl_controlgroup);
	Cvar_Register (&cl_download_redirection, cl_controlgroup);
	Cvar_Register (&cl_download_packages, cl_controlgroup);

	//
	// info mirrors
	//
	Cvar_Register (&name,						cl_controlgroup);
	Cvar_Register (&password,					cl_controlgroup);
	Cvar_Register (&spectator,					cl_controlgroup);
	Cvar_Register (&skin,						cl_controlgroup);
	Cvar_Register (&model,						cl_controlgroup);
	Cvar_Register (&team,						cl_controlgroup);
	Cvar_Register (&topcolor,					cl_controlgroup);
	Cvar_Register (&bottomcolor,				cl_controlgroup);
	Cvar_Register (&rate,						cl_controlgroup);
	Cvar_Register (&drate,						cl_controlgroup);
	Cvar_Register (&msg,						cl_controlgroup);
#ifdef Q2CLIENT
	Cvar_Register (&hand,						cl_controlgroup);
#endif
	Cvar_Register (&noaim,						cl_controlgroup);
	Cvar_Register (&b_switch,					cl_controlgroup);
	Cvar_Register (&w_switch,					cl_controlgroup);
#ifdef HEXEN2
	Cvar_Register (&cl_playerclass,				cl_controlgroup);
#endif

	Cvar_Register (&cl_demoreel,				cl_controlgroup);
	Cvar_Register (&record_flush,					cl_controlgroup);

	Cvar_Register (&cl_nofake,					cl_controlgroup);
	Cvar_Register (&cl_chatsound,					cl_controlgroup);
	Cvar_Register (&cl_enemychatsound,				cl_controlgroup);
	Cvar_Register (&cl_teamchatsound,				cl_controlgroup);

	Cvar_Register (&requiredownloads,				cl_controlgroup);
	Cvar_Register (&mod_precache,					cl_controlgroup);
	Cvar_Register (&cl_standardchat,				cl_controlgroup);
	Cvar_Register (&msg_filter,						cl_controlgroup);
	Cvar_Register (&msg_filter_frags,				cl_controlgroup);
	Cvar_Register (&cl_standardmsg,					cl_controlgroup);
	Cvar_Register (&cl_parsewhitetext,				cl_controlgroup);
	Cvar_Register (&cl_nopext,						cl_controlgroup);
	Cvar_Register (&cl_pext_mask,					cl_controlgroup);

	Cvar_Register (&cl_splitscreen,					cl_controlgroup);

#ifndef SERVERONLY
	Cvar_Register (&cl_loopbackprotocol,			cl_controlgroup);
#endif
	Cvar_Register (&cl_verify_urischeme,			cl_controlgroup);

	Cvar_Register (&cl_countpendingpl,				cl_controlgroup);
	Cvar_Register (&cl_threadedphysics,				cl_controlgroup);
	hud_tracking_show = Cvar_Get("hud_tracking_show", "1", 0, "statusbar");
	hud_miniscores_show = Cvar_Get("hud_miniscores_show", "1", 0, "statusbar");

	Cvar_Register (&cl_dlemptyterminate,				cl_controlgroup);

	Cvar_Register (&cl_gunx,					cl_controlgroup);
	Cvar_Register (&cl_guny,					cl_controlgroup);
	Cvar_Register (&cl_gunz,					cl_controlgroup);

	Cvar_Register (&cl_gunanglex,					cl_controlgroup);
	Cvar_Register (&cl_gunangley,					cl_controlgroup);
	Cvar_Register (&cl_gunanglez,					cl_controlgroup);

	Cvar_Register (&ruleset_allow_playercount,			cl_controlgroup);
	Cvar_Register (&ruleset_allow_frj,				cl_controlgroup);
	Cvar_Register (&ruleset_allow_semicheats,			cl_controlgroup);
	Cvar_Register (&ruleset_allow_packet,				cl_controlgroup);
	Cvar_Register (&ruleset_allow_particle_lightning,		cl_controlgroup);
	Cvar_Register (&ruleset_allow_overlongsounds,			cl_controlgroup);
	Cvar_Register (&ruleset_allow_larger_models,			cl_controlgroup);
	Cvar_Register (&ruleset_allow_modified_eyes,			cl_controlgroup);
	Cvar_Register (&ruleset_allow_sensitive_texture_replacements,	cl_controlgroup);
	Cvar_Register (&ruleset_allow_localvolume,			cl_controlgroup);
	Cvar_Register (&ruleset_allow_shaders,			cl_controlgroup);
	Cvar_Register (&ruleset_allow_watervis,			cl_controlgroup);
	Cvar_Register (&ruleset_allow_fbmodels,			cl_controlgroup);

	Cvar_Register (&qtvcl_forceversion1,				cl_controlgroup);
	Cvar_Register (&qtvcl_eztvextensions,				cl_controlgroup);
#ifdef FTPCLIENT
	Cmd_AddCommand ("ftp", CL_FTP_f);
#endif

	Cmd_AddCommandD ("changing", CL_Changing_f, "Part of network protocols. This command should not be used manually.");
	Cmd_AddCommand ("disconnect", CL_Disconnect_f);
	Cmd_AddCommandAD ("record", CL_Record_f, CL_DemoList_c, NULL);
	Cmd_AddCommandAD ("rerecord", CL_ReRecord_f, CL_DemoList_c, "Reconnects to the previous/current server, but starts recording a clean demo.");
	Cmd_AddCommandD ("stop", CL_Stop_f, "Stop demo recording.");
	Cmd_AddCommandAD ("playdemo", CL_PlayDemo_f, CL_DemoList_c, NULL);
	Cmd_AddCommand ("qtvplay", CL_QTVPlay_f);
	Cmd_AddCommand ("qtvlist", CL_QTVList_f);
	Cmd_AddCommand ("qtvdemos", CL_QTVDemos_f);
	Cmd_AddCommandD ("demo_jump",		CL_DemoJump_f, "Jump to a specified time in a demo. Prefix with a + or - for a relative offset. Seeking backwards will restart the demo and the fast forward, which can take some time in long demos.");
	Cmd_AddCommandD ("demo_jump_mark",	CL_DemoJump_f, "Jump to the next '//demomark' marker.");
	Cmd_AddCommandD ("demo_jump_end",	CL_DemoJump_f, "Jump to the next intermission message.");
	Cmd_AddCommandD ("demo_nudge", CL_DemoNudge_f, "Nudge the demo by one frame. Argument should be +1 or -1. Nudging backwards is limited.");
	Cmd_AddCommandAD ("timedemo", CL_TimeDemo_f, CL_DemoList_c, NULL);
#ifdef _DEBUG
	Cmd_AddCommand ("freespace", CL_FreeSpace_f);
	Cmd_AddCommand ("crashme_endgame", CL_CrashMeEndgame_f);
	Cmd_AddCommand ("crashme_error", CL_CrashMeError_f);
#endif

	Cmd_AddCommandD ("showpic", SCR_ShowPic_Script_f, 	"showpic <imagename> <placename> <x> <y> <zone> [width] [height] [touchcommand]\nDisplays an image onscreen, that potentially has a key binding attached to it when clicked/touched.\nzone should be one of: TL, TR, BL, BR, MM, TM, BM, ML, MR. This serves as an extra offset to move the image around the screen without any foreknowledge of the screen resolution.");
	Cmd_AddCommandD ("showpic_removeall", SCR_ShowPic_Remove_f, 	"removes any pictures inserted with the showpic command.");

	Cmd_AddCommandD ("startdemos", CL_Startdemos_f, "Sets the demoreel list, but does not start playing them (use the 'demos' command for that)");
	Cmd_AddCommandD ("demos", CL_Demos_f, "Starts playing the demo reel.");
	Cmd_AddCommand ("stopdemo", CL_Stopdemo_f);

	Cmd_AddCommand ("skins", Skin_Skins_f);
#ifdef QWSKINS
	Cmd_AddCommand ("allskins", Skin_AllSkins_f);
#endif

	Cmd_AddCommand ("cl_status", CL_Status_f);
	Cmd_AddCommandD ("quit", CL_Quit_f, "Use this command when you get angry. Does not save any cvars. Use cfg_save to save settings, or use the menu for a prompt.");

#if defined(CL_MASTER) && defined(HAVE_PACKET)
	Cmd_AddCommandAD ("connectbr", CL_ConnectBestRoute_f, CL_Connect_c, "connect address:port\nConnect to a qw server using the best route we can detect.");
#endif
	Cmd_AddCommandAD("connect", CL_Connect_f, CL_Connect_c, "connect scheme://address:port\nConnect to a server. "
#if defined(FTE_TARGET_WEB)
		"Use a scheme of rtc[s]://broker/gamename to connect via a webrtc broker."
		"Use a scheme of ws[s]://server to connect via websockets."
#elif defined(TCPCONNECT)
		"Use a scheme of tcp:// or tls:// to connect via non-udp protocols."
//		"Use a scheme of ws[s]://server to connect via websockets."
#endif
#ifdef HAVE_DTLS
		"Use a scheme of dtls://server to connect securely."
#endif
#if defined(IRCCONNECT)
		"Use irc://network:6667/user[@channel] to connect via an irc server. Not recommended."
#endif
#if defined(NQPROT) || defined(Q2CLIENT) || defined(Q3CLIENT)
		"\nDefault port is port "STRINGIFY(PORT_DEFAULTSERVER)"."
	#ifndef GAME_DEFAULTPORT
		#ifdef NQPROT
				" NQ:"STRINGIFY(PORT_NQSERVER)"."
		#endif
				" QW:"STRINGIFY(PORT_QWSERVER)"."
		#ifdef Q2CLIENT
				" Q2:"STRINGIFY(PORT_Q2SERVER)"."
		#endif
		#ifdef Q3CLIENT
				" Q3:"STRINGIFY(PORT_Q3SERVER)"."
		#endif
	#endif
#endif
		);
	Cmd_AddCommandD ("cl_transfer", CL_Transfer_f, "Connect to a different server, disconnecting from the current server only when the new server replies.");
#ifdef TCPCONNECT
	Cmd_AddCommandAD ("connecttcp", CL_TCPConnect_f, CL_Connect_c, "Connect to a server using the tcp:// prefix");
	Cmd_AddCommandAD ("tcpconnect", CL_TCPConnect_f, CL_Connect_c, "Connect to a server using the tcp:// prefix");
#endif
#ifdef IRCCONNECT
	Cmd_AddCommand ("connectirc", CL_IRCConnect_f);
#endif
#ifdef NQPROT
	Cmd_AddCommandD ("connectnq", CLNQ_Connect_f, "Connects to the specified server, defaulting to port "STRINGIFY(PORT_NQSERVER)". Also disables QW/Q2/Q3/DP handshakes preventing them from being favoured, so should only be used when you actually want NQ protocols specifically.");
	#ifdef HAVE_DTLS
	Cmd_AddCommandD ("connectqe", CLNQ_Connect_f, "Connects to the specified server, defaulting to port "STRINGIFY(PORT_NQSERVER)". Also forces the use of DTLS and QE-specific handshakes. You will also need to ensure the dtls_psk_* cvars are set properly or the server will refuse the connection.");
	#endif
#endif
#ifdef Q2CLIENT
	Cmd_AddCommandD ("connectq2e", CLQ2E_Connect_f, "Connects to the specified server, defaulting to port "STRINGIFY(PORT_Q2EXSERVER)".");
#endif
#ifdef HAVE_LEGACY
	Cmd_AddCommandAD("qwurl", CL_Connect_f, CL_Connect_c, "For compat with ezquake.");
#endif
	Cmd_AddCommand ("reconnect", CL_Reconnect_f);
	Cmd_AddCommandAD ("join", CL_Join_f, CL_Connect_c, "Switches away from spectator mode, optionally connecting to a different server.");
	Cmd_AddCommandAD ("observe", CL_Observe_f, CL_Connect_c, "Switches to spectator mode, optionally connecting to a different server.");

	Cmd_AddCommand ("rcon", CL_Rcon_f);
	Cmd_AddCommand ("packet", CL_Packet_f);
	Cmd_AddCommand ("user", CL_User_f);
	Cmd_AddCommand ("users", CL_Users_f);

#if 1//def _DEBUG
	Cmd_AddCommand ("setinfoblob", CL_SetInfoBlob_f);
#endif
	Cmd_AddCommand ("setinfo", CL_SetInfo_f);
	Cmd_AddCommand ("fullinfo", CL_FullInfo_f);

	Cmd_AddCommandAD ("color", CL_Color_f, CL_Color_c, NULL);
#if defined(NQPROT) && defined(HAVE_LEGACY)
	Cmd_AddCommandD ("curl",	CL_Curl_f, "For use by xonotic.");
#endif
	Cmd_AddCommand ("download", CL_Download_f);
	Cmd_AddCommandD ("dlsize", CL_DownloadSize_f, "For internal use");
	Cmd_AddCommandD ("nextul", CL_NextUpload, "For internal use");
	Cmd_AddCommandD ("stopul", CL_StopUpload, "For internal use");

	Cmd_AddCommand ("skipdl", CL_SkipDownload_f);
	Cmd_AddCommand ("finishdl", CL_FinishDownload_f);

//
// forward to server commands
//
	Cmd_AddCommand ("god", NULL);	//cheats
	Cmd_AddCommand ("give", NULL);
	Cmd_AddCommand ("noclip", NULL);
	Cmd_AddCommand ("6dof", NULL);
	Cmd_AddCommand ("spiderpig", NULL);
	Cmd_AddCommand ("fly", NULL);
	Cmd_AddCommand ("setpos", NULL);
	Cmd_AddCommand ("notarget", NULL);

	Cmd_AddCommand ("topten", NULL);

	Cmd_AddCommand ("kill", NULL);
	Cmd_AddCommand ("pause", NULL);
	Cmd_AddCommandAD ("say", CL_Say_f, Key_EmojiCompletion_c, NULL);
	Cmd_AddCommandAD ("me", CL_SayMe_f, Key_EmojiCompletion_c, NULL);
	Cmd_AddCommandAD ("sayone", CL_Say_f, Key_EmojiCompletion_c, NULL);
	Cmd_AddCommandAD ("say_team", CL_SayTeam_f, Key_EmojiCompletion_c, NULL);
#ifdef HAVE_SERVER
	Cmd_AddCommand ("serverinfo", CL_ServerInfo_f);
#else
	Cmd_AddCommand ("serverinfo", NULL);
#endif

	Cmd_AddCommandD ("fog", CL_Fog_f, "fog <density> <red> <green> <blue> <alpha> <depthbias>");
	Cmd_AddCommandD ("waterfog", CL_Fog_f, "waterfog <density> <red> <green> <blue> <alpha> <depthbias>");
	Cmd_AddCommandD ("skyroomfog", CL_Fog_f, "skyroomfog <density> <red> <green> <blue> <alpha> <depthbias>");
	Cmd_AddCommandD ("skygroup", CL_Skygroup_f, "Provides a way to associate a skybox name with a series of maps, so that the requested skybox will override on a per-map basis.");
	Cmd_AddCommandD ("r_imagelist_wad", WAD_ImageList_f, "displays the available wad images.");
//
//  Windows commands
//
#if defined(_WIN32) && !defined(WINRT) && !defined(_XBOX)
	Cmd_AddCommand ("windows", CL_Windows_f);
#endif

	Ignore_Init();
#ifdef QUAKEHUD
	Stats_Init();
#endif
	CL_ClearState(false);	//make sure the cl.* fields are set properly if there's no ssqc or whatever.
	R_BumpLightstyles(1);
}


/*
================
Host_EndGame

Call this to drop to a console without exiting the qwcl
================
*/
NORETURN void VARGS Host_EndGame (const char *message, ...)
{
	va_list		argptr;
	char		string[1024];

	if (message)
	{
		va_start (argptr,message);
		vsnprintf (string,sizeof(string)-1, localtext(message),argptr);
		va_end (argptr);
	}
	else
		*string = 0;

	COM_AssertMainThread(string);

	SCR_EndLoadingPlaque();

	if (message)
	{
		Con_TPrintf ("^&C0Host_EndGame: %s\n", string);
		Con_Printf ("\n");
	}

	SCR_EndLoadingPlaque();

	CL_Disconnect (string);
	CL_ConnectAbort(NULL);

	SV_UnspawnServer();

	Cvar_Set(&cl_shownet, "0");

	longjmp (host_abort, 1);
}

/*
================
Host_Error

This shuts down the client and exits qwcl
================
*/
void VARGS Host_Error (const char *error, ...)
{
	va_list		argptr;
	char		string[1024];
	static	qboolean inerror = false;

	if (inerror)
		Sys_Error ("Host_Error: recursively entered");
	inerror = true;

	va_start (argptr,error);
	vsnprintf (string,sizeof(string)-1, localtext(error),argptr);
	va_end (argptr);
	COM_AssertMainThread(string);
	Con_TPrintf ("Host_Error: %s\n", string);

	CL_Disconnect (string);
	cls.demonum = -1;

	inerror = false;

// FIXME
	Sys_Error ("Host_Error: %s\n",string);
}


/*
===============
Host_WriteConfiguration

Writes key bindings and archived cvars to config.cfg
===============
*/
void Host_WriteConfiguration (void)
{
	vfsfile_t	*f;
	char savename[MAX_OSPATH];
	char sysname[MAX_OSPATH];

	if (host_initialized && cfg_save_name.string && *cfg_save_name.string)
	{
		if (strchr(cfg_save_name.string, '.'))
		{
			Con_TPrintf (CON_ERROR "Couldn't write config.cfg.\n");
			return;
		}

		Q_snprintfz(savename, sizeof(savename), "%s.cfg", cfg_save_name.string);

		f = FS_OpenVFS(savename, "wb", FS_GAMEONLY);
		if (!f)
		{
			FS_DisplayPath(savename, FS_GAMEONLY, sysname, sizeof(sysname));
			Con_TPrintf (CON_ERROR "Couldn't write %s.\n", sysname);
			return;
		}

		Key_WriteBindings (f);
		Cvar_WriteVariables (f, false, false);

		VFS_CLOSE (f);

		FS_DisplayPath(savename, FS_GAMEONLY, sysname, sizeof(sysname));
		Con_Printf("Wrote %s\n", savename);
	}
}


//============================================================================

#if 0
/*
==================
Host_SimulationTime

This determines if enough time has passed to run a simulation frame
==================
*/
qboolean Host_SimulationTime(float time)
{
	float fps;

	if (oldrealtime > realtime)
		oldrealtime = 0;

	if (cl_maxfps.value)
		fps = max(30.0, min(cl_maxfps.value, 72.0));
	else
		fps = max(30.0, min(rate.value/80.0, 72.0));

	if (!cls.timedemo && (realtime + time) - oldrealtime < 1.0/fps)
		return false;			// framerate is too high
	return true;
}
#endif

#include "fs.h"
#define HRF_OVERWRITE	(1<<0)
#define HRF_NOOVERWRITE	(1<<1)
//						(1<<2)
#define HRF_ABORT		(1<<3)

#define HRF_OPENED		(1<<4)
#define HRF_DOWNLOADED	(1<<5)	//file was actually downloaded, and not from the local system
#define HRF_WAITING		(1<<6)	//file looks important enough that we should wait for it to start to download or something before we try doing other stuff.
#define HRF_DECOMPRESS	(1<<7)	//need to degzip it, which prevents streaming.

#define HRF_DEMO_MVD	(1<<8)
#define HRF_DEMO_QWD	(1<<9)
#define HRF_DEMO_DM2	(1<<10)
#define HRF_DEMO_DEM	(1<<11)

#define HRF_QTVINFO		(1<<12)
#define HRF_MANIFEST	(1<<13)
#define HRF_BSP			(1<<14)
#define HRF_PACKAGE		(1<<15)	//pak or pk3 that should be installed.
#define	HRF_ARCHIVE		(1<<16)	//zip - treated as a multiple-file 'installer'
#define HRF_MODEL		(1<<17)
#define HRF_CONFIG		(1<<18)	//exec it on the console...

#define HRF_ACTION (HRF_OVERWRITE|HRF_NOOVERWRITE|HRF_ABORT)
#define HRF_DEMO		(HRF_DEMO_MVD|HRF_DEMO_QWD|HRF_DEMO_DM2|HRF_DEMO_DEM)
#define HRF_FILETYPES	(HRF_DEMO|HRF_QTVINFO|HRF_MANIFEST|HRF_BSP|HRF_PACKAGE|HRF_ARCHIVE|HRF_MODEL|HRF_CONFIG)
typedef struct {
	struct dl_download *dl;
	vfsfile_t *srcfile;
	vfsfile_t *dstfile;
	char *packageinfo;
	unsigned int flags;
	char fname[1];	//system path or url.
} hrf_t;

extern int waitingformanifest;
void Host_DoRunFile(hrf_t *f);
void CL_PlayDemoStream(vfsfile_t *file, char *filename, qboolean issyspath, int demotype, float bufferdelay, unsigned int eztv_ext);
void CL_ParseQTVDescriptor(vfsfile_t *f, const char *name);

//guesses the file type based upon its file extension. mdl/md3/iqm distinctions are not important, so we can usually get away with this in the context of quake.
unsigned int Host_GuessFileType(const char *mimetype, const char *filename)
{
	if (mimetype)
	{
		if (!strcmp(mimetype, "application/x-qtv"))	//what uses this?
			return HRF_QTVINFO;
		else if (!strcmp(mimetype, "text/x-quaketvident"))
			return HRF_QTVINFO;
		else if (!strcmp(mimetype, "application/x-fteplugin"))
			return HRF_MANIFEST;
		else if (!strcmp(mimetype, "application/x-ftemanifest"))
			return HRF_MANIFEST;
		else if (!strcmp(mimetype, "application/x-multiviewdemo"))
			return HRF_DEMO_MVD;
		else if (!strcmp(mimetype, "application/zip"))
			return HRF_ARCHIVE;
//		else if (!strcmp(mimetype, "application/x-ftebsp"))
//			return HRF_BSP;
//		else if (!strcmp(mimetype, "application/x-ftepackage"))
//			return HRF_PACKAGE;
	}

	if (filename)
	{	//find the query or location part of the url, so we can ignore extra stuff.
		struct
		{
			unsigned int type;
			const char *ext;
		} exts[] =
		{
			//demo formats
			{HRF_DEMO_QWD, "qwd"},
			{HRF_DEMO_QWD|HRF_DECOMPRESS, "qwd.gz"},
			{HRF_DEMO_MVD, "mvd"},
			{HRF_DEMO_MVD|HRF_DECOMPRESS, "mvd.gz"},
			{HRF_DEMO_DM2, "dm2"},
			{HRF_DEMO_DM2|HRF_DECOMPRESS, "dm2.gz"},
			{HRF_DEMO_DEM, "dem"},
			{HRF_DEMO_DEM|HRF_DECOMPRESS, "dem.gz"},
			{HRF_QTVINFO, "qtv"},
			//other stuff
			{HRF_MANIFEST, "fmf"},
			{HRF_BSP, "bsp"},
			{HRF_BSP, "map"},
			{HRF_CONFIG, "cfg"},
			{HRF_CONFIG, "rc"},
			{HRF_PACKAGE, "kpf"},
			{HRF_PACKAGE, "pak"},
			{HRF_PACKAGE, "pk3"},
			{HRF_PACKAGE, "pk4"},
			{HRF_PACKAGE, "wad"},
			{HRF_ARCHIVE, "zip"},
			//model formats
			{HRF_MODEL, "mdl"},
			{HRF_MODEL, "md2"},
			{HRF_MODEL, "md3"},
			{HRF_MODEL, "iqm"},
			{HRF_MODEL, "vvm"},
			{HRF_MODEL, "psk"},
			{HRF_MODEL, "zym"},
			{HRF_MODEL, "dpm"},
			{HRF_MODEL, "gltf"},
			{HRF_MODEL, "glb"},
			//sprites
			{HRF_MODEL, "spr"},
			{HRF_MODEL, "spr2"},
			//static stuff
			{HRF_MODEL, "obj"},
			{HRF_MODEL, "lwo"},
			{HRF_MODEL, "ase"},
		};
		size_t i;
		const char *ext;
		const char *stop = filename+strlen(filename);
		const char *tag = strchr(filename, '?');
		if (tag && tag < stop)
			stop = tag;
		tag = strchr(filename, '#');
		if (tag && tag < stop)
			stop = tag;

		ext = COM_GetFileExtension(filename, stop);
		if (!Q_strstopcasecmp(ext, stop, ".php"))	//deal with extra extensions the easy way
			ext = COM_GetFileExtension(filename, stop=ext);
		if (!Q_strstopcasecmp(ext, stop, ".gz") || !Q_strstopcasecmp(ext, stop, ".xz"))	//deal with extra extensions the easy way
			ext = COM_GetFileExtension(filename, ext);
		if (*ext == '.')
			ext++;

		for (i = 0; i < countof(exts); i++)
			if (!Q_strstopcasecmp(ext, stop, exts[i].ext))
				return exts[i].type;
	}
	return 0;
}

void Host_RunFileDownloaded(struct dl_download *dl)
{
	hrf_t *f = dl->user_ctx;
	if(!f)	//download was previously cancelled.
		return;
	if (dl->status == DL_FAILED)
	{
		f->flags |= HRF_ABORT;
		f->srcfile = NULL;
	}
	else
	{
		if (f->srcfile)	//erk?
			VFS_CLOSE(f->srcfile);
		f->flags |= HRF_OPENED;
		f->srcfile = dl->file;
		dl->file = NULL;
	}

	Host_DoRunFile(f);
}
qboolean Host_BeginFileDownload(struct dl_download *dl, char *mimetype)
{
	qboolean result = false;
	//at this point the file is still downloading, so don't copy it out just yet.
	hrf_t *f = dl->user_ctx;

	if (f->flags & HRF_WAITING)
	{
		f->flags &= ~HRF_WAITING;
		waitingformanifest--;
	}

	if (!(f->flags & HRF_FILETYPES))
	{
		f->flags |= Host_GuessFileType(mimetype, f->fname);
		if (!(f->flags & HRF_FILETYPES))
		{
			if (mimetype)
				Con_Printf("mime type \"%s\" nor file extension of \"%s\" not known\n", mimetype, f->fname);
			else
				Con_Printf("file extension of \"%s\" not known\n", f->fname);
			//file type not guessable from extension either.
			f->flags |= HRF_ABORT;
			Host_DoRunFile(f);
			return false;
		}

		if ((f->flags & HRF_MANIFEST) && !(f->flags & HRF_WAITING))
		{
			f->flags |= HRF_WAITING;
			waitingformanifest++;
		}
	}

#ifdef AVAIL_GZDEC
	//seeking means we can rewind
	if (f->flags & HRF_DECOMPRESS)
	{	//if its a gzip, we'll probably need to decompress it ourselves... in case the server doesn't use content-encoding:gzip
		//our demo playback should decompress it when its fin ally available.
		dl->file = VFSPIPE_Open(1, true);
		return true;
	}
	else
#endif
		 if (f->flags & HRF_DEMO_QWD)
		CL_PlayDemoStream((dl->file = VFSPIPE_Open(2, true)), f->fname, true, DPB_QUAKEWORLD, 0, 0);
	else if (f->flags & HRF_DEMO_MVD)
		CL_PlayDemoStream((dl->file = VFSPIPE_Open(2, true)), f->fname, true, DPB_MVD, 0, 0);
#ifdef Q2CLIENT
	else if (f->flags & HRF_DEMO_DM2)
		CL_PlayDemoStream((dl->file = VFSPIPE_Open(2, true)), f->fname, true, DPB_QUAKE2, 0, 0);
#endif
#ifdef NQPROT
	else if (f->flags & HRF_DEMO_DEM)
	{	//fixme: the demo code can't handle the cd track with streamed/missing-so-far writes.
		dl->file = VFSPIPE_Open(1, true);	//make sure the reader will be seekable, so we can rewind.
//		CL_PlayDemoStream((dl->file = VFSPIPE_Open(2, true)), f->fname, DPB_NETQUAKE, 0, 0);
	}
#endif
	else if (f->flags & (HRF_MANIFEST | HRF_QTVINFO))
	{
		//just use a pipe instead of a temp file, working around an issue with temp files on android
		dl->file = VFSPIPE_Open(1, false);
		return true;
	}
	else if (f->flags & HRF_ARCHIVE)
	{
		char cachename[MAX_QPATH];
		if (!FS_PathURLCache(f->fname, cachename, sizeof(cachename)))
			return false;
		f->srcfile = FS_OpenVFS(cachename, "rb", FS_ROOT);
		if (f->srcfile)
		{
			f->flags |= HRF_OPENED;
			Host_DoRunFile(f);
			return false;
		}
		FS_CreatePath(cachename, FS_ROOT);
		dl->file = FS_OpenVFS(cachename, "wb", FS_ROOT);
		if (dl->file)
			return true;	//okay, continue downloading.
	}
	else if (f->flags & HRF_DEMO)
		Con_Printf("%s: format not supported\n", f->fname);	//demos that are not supported in this build for one reason or another
	else
		return true;

	//demos stream, so we want to continue the http download, but we don't want to do anything with the result.
	if (f->flags & HRF_DEMO)
		result = true;
	else
	{
		f->flags |= HRF_ABORT;
		Host_DoRunFile(f);
	}
	return result;
}
void Host_RunFilePrompted(void *ctx, promptbutton_t button)
{
	hrf_t *f = ctx;
	switch(button)
	{
	case PROMPT_YES:
		f->flags |= HRF_OVERWRITE;
		break;
	case PROMPT_NO:
		f->flags |= HRF_NOOVERWRITE;
		break;
	default:
		f->flags |= HRF_ABORT;
		break;
	}
	Host_DoRunFile(f);
}

#ifdef WEBCLIENT
static qboolean isurl(char *url)
{
#ifdef FTE_TARGET_WEB
	return true;	//assume EVERYTHING is a url, because the local filesystem is pointless.
#endif
	return /*!strncmp(url, "data:", 5) || */!strncmp(url, "http://", 7) || !strncmp(url, "https://", 8);
}
#endif

qboolean FS_FixupGamedirForExternalFile(char *input, char *filename, size_t fnamelen);
void CL_PlayDemoFile(vfsfile_t *f, char *demoname, qboolean issyspath);

void Host_DoRunFile(hrf_t *f)
{
	char qname[MAX_QPATH];
	char displayname[MAX_QPATH];
	char loadcommand[MAX_OSPATH];
	qboolean isnew = false;
	qboolean haschanged = false;
	enum fs_relative qroot = FS_GAME;

	if (f->flags & HRF_WAITING)
	{
		f->flags &= ~HRF_WAITING;
		waitingformanifest--;
	}
	
	if (f->flags & HRF_ABORT)
	{
done:
		if (f->flags & HRF_WAITING)
			waitingformanifest--;

		if (f->packageinfo)
			Z_Free(f->packageinfo);
		if (f->srcfile)
			VFS_CLOSE(f->srcfile);
		if (f->dstfile)
			VFS_CLOSE(f->dstfile);
		Z_Free(f);
		return;
	}

#ifdef WEBCLIENT
	if (isurl(f->fname) && !f->srcfile)
	{
		if (!(f->flags & HRF_OPENED))
		{
			struct dl_download *dl;
			f->flags |= HRF_OPENED;
			dl = HTTP_CL_Get(f->fname, NULL, Host_RunFileDownloaded);
			if (dl)
			{
				f->flags |= HRF_DOWNLOADED;
				dl->notifystarted = Host_BeginFileDownload;
				dl->user_ctx = f;

				if (!(f->flags & HRF_WAITING))
				{
					f->flags |= HRF_WAITING;
					waitingformanifest++;
				}
				return;
			}
		}
	}
#endif

	if (!(f->flags & HRF_FILETYPES))
	{
		f->flags |= Host_GuessFileType(NULL, f->fname);
		
		//if we still don't know what it is, give up.
		if (!(f->flags & HRF_FILETYPES))
		{
			Con_Printf("Host_DoRunFile: unknown filetype for \"%s\"\n", f->fname);
			goto done;
		}

		if (f->flags & HRF_MANIFEST)
		{
			if (!(f->flags & HRF_WAITING))
			{
				f->flags |= HRF_WAITING;
				waitingformanifest++;
			}
		}
	}

	if (f->flags & HRF_DEMO)
	{
		if (f->srcfile)
		{
			VFS_SEEK(f->srcfile, 0);
#ifdef AVAIL_GZDEC
			f->srcfile = FS_DecompressGZip(f->srcfile, NULL);
#endif

			if (f->flags & HRF_DEMO_QWD)
				CL_PlayDemoStream(f->srcfile, f->fname, true, DPB_QUAKEWORLD, 0, 0);
#ifdef Q2CLIENT
			else if (f->flags & HRF_DEMO_DM2)
				CL_PlayDemoStream(f->srcfile, f->fname, true, DPB_QUAKE2, 0, 0);
#endif
#ifdef NQPROT
			else if (f->flags & HRF_DEMO_DEM)
				CL_PlayDemoFile(f->srcfile, f->fname, true);	//should be able to handle the cd-track header.
#endif
			else //if (f->flags & HRF_DEMO_MVD)
				CL_PlayDemoStream(f->srcfile, f->fname, true, DPB_MVD, 0, 0);
			f->srcfile = NULL;
		}
		else
		{
			//play directly via system path, no prompts needed
			FS_FixupGamedirForExternalFile(f->fname, loadcommand, sizeof(loadcommand));
			Cbuf_AddText(va("playdemo \"%s\"\n", loadcommand), RESTRICT_LOCAL);
		}
		goto done;
	}
	else if (f->flags & HRF_BSP)
	{
		char shortname[MAX_QPATH];
		COM_StripExtension(COM_SkipPath(f->fname), shortname, sizeof(shortname));
		if (FS_FixupGamedirForExternalFile(f->fname, qname, sizeof(qname)) && !Q_strncasecmp(qname, "maps/", 5))
		{
			COM_StripExtension(qname+5, loadcommand, sizeof(loadcommand));
			Cbuf_AddText(va("map \"%s\"\n", loadcommand), RESTRICT_LOCAL);
			goto done;
		}

		Q_snprintfz(loadcommand, sizeof(loadcommand), "map \"%s\"\n", shortname);
		Q_snprintfz(displayname, sizeof(displayname), "map: %s", shortname);
		Q_snprintfz(qname, sizeof(qname), "maps/%s.bsp", shortname);
	}
	else if (f->flags & HRF_PACKAGE)
	{
		char *shortname;
		shortname = COM_SkipPath(f->fname);
		Q_snprintfz(qname, sizeof(qname), "%s", shortname);
		Q_snprintfz(loadcommand, sizeof(loadcommand), "fs_restart\n");
		Q_snprintfz(displayname, sizeof(displayname), "package: %s", shortname);
	}
	else if (f->flags & HRF_MANIFEST)
	{
		if (f->flags & HRF_OPENED)
		{
			if (f->srcfile)
			{
				ftemanifest_t *man;
				int len = VFS_GETLEN(f->srcfile);
				int foo;
				char *fdata = BZ_Malloc(len+1);
				foo = VFS_READ(f->srcfile, fdata, len);
				fdata[len] = 0;
				if (foo != len || !len)
				{
					Con_Printf("Host_DoRunFile: unable to read file properly\n");
					BZ_Free(fdata);
				}
				else
				{
					host_parms.manifest = Z_StrDup(fdata);
					man = FS_Manifest_ReadMem(NULL, NULL, fdata);
					if (man)
					{
						if (!man->updateurl)
							man->updateurl = Z_StrDup(f->fname);
//						if (f->flags & HRF_DOWNLOADED)
						man->blockupdate = true;
						//man->security = MANIFEST_SECURITY_DEFAULT;
						BZ_Free(fdata);
						FS_ChangeGame(man, true, true);
					}
					else
					{
						Con_Printf("Manifest file %s does not appear valid\n", f->fname);
						BZ_Free(fdata);
					}
				}

				goto done;
			}
		}
	}
	else if (f->flags & HRF_MODEL)
	{
		if (!FS_FixupGamedirForExternalFile(f->fname, loadcommand, sizeof(loadcommand)))
			Con_TPrintf("%s is not within the current gamedir\n", f->fname);
		else
			Cbuf_AddText(va("modelviewer \"%s\"\n", loadcommand), RESTRICT_LOCAL);
		goto done;
	}
	else if (f->flags & HRF_ARCHIVE)
	{
		char cachename[MAX_QPATH];
		struct gamepacks packagespaths[2];
		if (f->srcfile)
			VFS_CLOSE(f->srcfile);
		f->srcfile = NULL;

		memset(packagespaths, 0, sizeof(packagespaths));
		packagespaths[0].url = f->fname;
		packagespaths[0].path = cachename;
		packagespaths[0].package = NULL;
		if (FS_PathURLCache(f->fname, cachename, sizeof(cachename)))
		{
			COM_Gamedir("", packagespaths);
		}
		goto done;
	}
	else if (f->flags & HRF_CONFIG)
	{
		if (!(f->flags & HRF_ACTION))
		{
			Key_Dest_Remove(kdm_console);
			Menu_Prompt(Host_RunFilePrompted, f, va(localtext("Exec %s?\n"), COM_SkipPath(f->fname)), "Yes", NULL, "Cancel", true);
			return;
		}
		if (f->flags & HRF_OPENED)
		{
			size_t len = VFS_GETLEN(f->srcfile);
			char *fdata = BZ_Malloc(len+2);
			if (fdata)
			{
				VFS_READ(f->srcfile, fdata, len);
				fdata[len++] = '\n';
				fdata[len] = 0;
				Cbuf_AddText(fdata, RESTRICT_INSECURE);
				BZ_Free(fdata);
			}
			goto done;
		}
	}
	else if (!(f->flags & HRF_QTVINFO))
	{
		Con_Printf("Host_DoRunFile: filetype not handled\n");
		goto done;
	}

	//at this point we need the file to have been opened.
	if (!(f->flags & HRF_OPENED))
	{
		f->flags |= HRF_OPENED;
		if (!f->srcfile)
		{
#ifdef WEBCLIENT
			if (isurl(f->fname))
			{
				struct dl_download *dl = HTTP_CL_Get(f->fname, NULL, Host_RunFileDownloaded);
				if (dl)
				{
					dl->notifystarted = Host_BeginFileDownload;
					dl->user_ctx = f;
					return;
				}
			}
#endif
			f->srcfile = VFSOS_Open(f->fname, "rb");	//input file is a system path, or something.
		}
	}

	if (!f->srcfile)
	{
		Con_TPrintf("Unable to open %s\n", f->fname);
		goto done;
	}

	if (f->flags & HRF_PACKAGE)
	{
#ifdef PACKAGEMANAGER
		if (f->packageinfo)
			Z_Free(f->packageinfo);	//sucks that we have to do this again, to recompute the proper qname+qroot
		Q_strncpyz(qname, COM_SkipPath(f->fname), sizeof(qname));
		f->packageinfo = PM_GeneratePackageFromMeta(f->srcfile, qname,sizeof(qname), &qroot);
		if (!f->packageinfo)
			goto done;
#endif
	}
	else if (f->flags & HRF_MANIFEST)
	{
		Host_DoRunFile(f);
		return;
	}
	else if (f->flags & HRF_QTVINFO)
	{
		//pass the file object to the qtv code instead of trying to install it.
		CL_ParseQTVDescriptor(f->srcfile, f->fname);
		f->srcfile = NULL;

		goto done;
	}

	VFS_SEEK(f->srcfile, 0);

	if (f->flags & HRF_OVERWRITE)
		;//haschanged = isnew = true;
	else
	{
		f->dstfile = FS_OpenVFS(qname, "rb", (qroot==FS_GAMEONLY)?FS_GAME:qroot);
		if (f->dstfile)
		{
			//do a real diff.
			if (f->srcfile->seekstyle == SS_UNSEEKABLE || VFS_GETLEN(f->srcfile) != VFS_GETLEN(f->dstfile))
			{
				//if we can't seek, or the sizes differ, just assume that the file is modified.
				haschanged = true;
			}
			else
			{
				int len = VFS_GETLEN(f->srcfile);
				char sbuf[8192], dbuf[8192];
				if (len > sizeof(sbuf))
					len = sizeof(sbuf);
				VFS_READ(f->srcfile, sbuf, len);
				VFS_READ(f->dstfile, dbuf, len);
				haschanged = memcmp(sbuf, dbuf, len);
				VFS_SEEK(f->srcfile, 0);
			}
			VFS_CLOSE(f->dstfile);
			f->dstfile = NULL;
		}
		else
			isnew = true;
	}

	if (!(f->flags & HRF_ACTION))
	{
		Key_Dest_Remove(kdm_console);
		if (haschanged)
		{
			Menu_Prompt(Host_RunFilePrompted, f, va(localtext("File already exists.\nWhat would you like to do?\n%s\n"), displayname), "Overwrite", "Run old", "Cancel", true);
			return;
		}
		else if (isnew)
		{
			if (f->packageinfo && strstr(f->packageinfo, "\nguessed"))
				Menu_Prompt(Host_RunFilePrompted, f, va(localtext("File appears new.\nWould you like to install\n%s\n"CON_ERROR"File contains no metadata so will be installed to\n%s"), displayname, qname), "Install!", "", "Cancel", true);
			else
				Menu_Prompt(Host_RunFilePrompted, f, va(localtext("File appears new.\nWould you like to install\n%s\n"), displayname), "Install!", "", "Cancel", true);
			return;
		}
		else
		{
			Menu_Prompt(NULL, NULL, va(localtext("File is already installed\n%s\n"), displayname), NULL, NULL, "Cancel", true);
			f->flags |= HRF_ABORT;
		}
	}
	else if (f->flags & HRF_OVERWRITE)
	{
		char buffer[65536];
		int len;
		f->dstfile = FS_OpenVFS(qname, (f->flags & HRF_PACKAGE)?"wbp":"wb", qroot);
		if (f->dstfile)
		{
#ifdef FTE_TARGET_WEB
			VFS_SEEK(f->dstfile, VFS_GETLEN(f->srcfile));
			VFS_WRITE(f->dstfile, "zomg", 0);	//hack to ensure the file is there, avoiding excessive copies.
			VFS_SEEK(f->dstfile, 0);
#endif
			while(1)
			{
				len = VFS_READ(f->srcfile, buffer, sizeof(buffer));
				if (len <= 0)
					break;
				VFS_WRITE(f->dstfile, buffer, len);
			}

			VFS_CLOSE(f->dstfile);
			f->dstfile = NULL;
		}

#ifdef PACKAGEMANAGER
		if (f->flags & HRF_PACKAGE)
			PM_FileInstalled(qname, qroot, f->packageinfo, true);
#endif

		if (!strcmp(loadcommand, "fs_restart\n"))
			FS_ReloadPackFiles();
		else
			Cbuf_AddText(loadcommand, RESTRICT_LOCAL);
	}

	goto done;
}

//only valid once the host has been initialised, as it needs a working filesystem.
//if file is specified, takes full ownership of said file, including destruction.
qboolean Host_RunFile(const char *fname, int nlen, vfsfile_t *file)
{
	hrf_t *f;
#if defined(FTE_TARGET_WEB)
	if (nlen >= 8 && !strncmp(fname, "file:///", 8))
	{	//just here so we don't get confused by the arbitrary scheme check below.
	}
#else
	//file urls need special handling, if only for percent-encoding.
	char utf8[MAX_OSPATH*3];
	if (nlen >= 5 && !strncmp(fname, "file:", 5))
	{
		if (!Sys_ResolveFileURL(fname, nlen, utf8, sizeof(utf8)))
		{
			Con_Printf("Cannot resolve file url\n");
			if(file)
				VFS_CLOSE(file);
			return false;
		}
		fname = utf8;
		nlen = strlen(fname);
	}
#endif
	else if((nlen >= 7 && !strncmp(fname, "http://", 7)) ||
			(nlen >= 8 && !strncmp(fname, "https://", 8)))
		;	//don't interpret these as our custom uri schemes
	else
	{
		const char *schemeend = strstr(fname, "://");

		if (schemeend)
		{	//this is also implemented by ezquake, so be careful here...
			//examples:
			//	"quake2://broker:port"
			//	"quake2:rtc://broker:port/game"
			//	"qw://[stream@]host[:port]/COMMAND" join, spectate, qtvplay
			//we'll chop off any non-auth prefix, its just so we can handle multiple protocols via a single uri scheme.
			char *t, *cmd, *args;
			const char *url;
			char buffer[8192];
			const char *schemestart = strchr(fname, ':');
			int schemelen, urilen;

			//if its one of our explicit protocols then use the url as-is
			const char *netschemes[] = {"udp", "udp4", "udp6", "ipx", "tcp", "tcp4", "tcp6", "spx", "ws", "wss", "tls", "dtls", "ice", "rtc", "ices", "rtcs", "irc", "udg", "unix"};
			int i;
			size_t slen;

			if (!schemestart || schemestart==schemeend)
				schemestart = fname;
			else
				schemestart++;
			schemelen = schemeend-schemestart;
			urilen = nlen-(schemestart-fname);

			for (i = 0; i < countof(netschemes); i++)
			{
				slen = strlen(netschemes[i]);
				if (schemelen == slen && !strncmp(schemestart, netschemes[i], slen))
				{
					char quoted[8192];
					char *t = Z_Malloc(urilen+1);
					memcpy(t, schemestart, urilen);
					t[urilen] = 0;

					Cbuf_AddText(va("connect %s\n", COM_QuotedString(t, quoted, sizeof(quoted), false)), RESTRICT_LOCAL);

					if(file)
						VFS_CLOSE(file);
					Z_Free(t);
					return true;
				}
			}

			schemelen++;
			if (!strncmp(schemestart+schemelen, "//", 2))
				schemelen+=2;

			t = Z_Malloc(urilen+1);
			memcpy(t, schemestart, urilen);
			t[urilen] = 0;
			url = t+schemelen;

			*buffer = 0;
			for (args = t+schemelen; *args; args++)
			{
				if (*args == '?')
				{
					*args++ = 0;
					break;
				}
			}
			for (cmd = t+schemelen; *cmd; cmd++)
			{
				if (*cmd == '/')
				{
					*cmd++ = 0;
					break;
				}
			}

			//quote the url safely.
			url = COM_QuotedString(url, buffer, sizeof(buffer), false);

			//now figure out what the command actually was
			if (!Q_strcasecmp(cmd, "join"))
				Cbuf_AddText(va("join %s\n", url), RESTRICT_LOCAL);
			else if (!Q_strcasecmp(cmd, "spectate") || !strcmp(cmd, "observe"))
				Cbuf_AddText(va("observe %s\n", url), RESTRICT_LOCAL);
			else if (!Q_strcasecmp(cmd, "qtvplay"))
				Cbuf_AddText(va("qtvplay %s\n", url), RESTRICT_LOCAL);
			else if (!*cmd || !Q_strcasecmp(cmd, "connect"))
				Cbuf_AddText(va("connect %s\n", url), RESTRICT_LOCAL);
			else
				Con_Printf("Unknown url command: %s\n", cmd);

			if(file)
				VFS_CLOSE(file);
			Z_Free(t);
			return true;
		}
	}

	f = Z_Malloc(sizeof(*f) + nlen);
	memcpy(f->fname, fname, nlen);
	f->fname[nlen] = 0;
	f->srcfile = file;
	if (file)
		f->flags |= HRF_OPENED;

	{
		char dpath[MAX_OSPATH];
		FS_DisplayPath(f->fname, FS_SYSTEM, dpath,sizeof(dpath));
		Con_TPrintf("Opening external file: %s\n", dpath);
	}

	Host_DoRunFile(f);
	return true;
}

/*
==================
Host_Frame

Runs all active servers
==================
*/
extern cvar_t cl_netfps;

void CL_StartCinematicOrMenu(void);
int		nopacketcount;
void SNDDMA_SetUnderWater(qboolean underwater);
double Host_Frame (double time)
{
	static double		time0 = 0;
	static double		time1 = 0;
	static double		time2 = 0;
	static double		time3 = 0;
	int			pass0, pass1, pass2, pass3, i;
//	float fps;
	double newrealtime, spare;
	float maxfps;
	qboolean maxfpsignoreserver;
	qboolean idle;
	static qboolean hadwork;
	unsigned int vrflags;
	qboolean mustrenderbeforeread;

	RSpeedLocals();

	if (setjmp (host_abort) )
	{
		return 0;			// something bad happened, or the server disconnected
	}

	vrflags = vid.vr?vid.vr->SyncFrame(&time):0;				//fiddle with frame timings
	newrealtime = Media_TweekCaptureFrameTime(realtime, time);	//fiddle with time some more

	time = newrealtime - realtime;
	realtime = newrealtime;

	if (oldrealtime > realtime)
		oldrealtime = realtime;

	if (cl.gamespeed<0.1)
		cl.gamespeed = 1;
	time *= cl.gamespeed;

#ifdef WEBCLIENT
//	FTP_ClientThink();
	HTTP_CL_Think(NULL, NULL);
#endif

	if (r_blockvidrestart)
	{
		Cbuf_Execute();
		if (waitingformanifest)
		{
			COM_MainThreadWork();
			return 0.1;
		}
		Host_FinishLoading();
		return 0;
	}
	if (startuppending)
		CL_StartCinematicOrMenu();

	if (cl.paused)
		cl.gametimemark += time;

	//if we're at a menu/console/thing
//	idle = !Key_Dest_Has_Higher(kdm_menu);
//	idle = ((cls.state == ca_disconnected) || cl.paused) && idle;	//idle if we're disconnected/paused and not at a menu
	idle = !vid.activeapp; //always idle when tabbed out

	//read packets early and always, so we don't have stuff waiting for reception quite so often.
	//should smooth out a few things, and increase download speeds.
	if (!cls.timedemo)
		CL_ReadPackets ();

	if (idle && cl_idlefps.value > 0 && !(vrflags&VRF_OVERRIDEFRAMETIME))
	{
		double idlesec = 1.0 / cl_idlefps.value;
		if (idlesec > 0.1)
			idlesec = 0.1; // limit to at least 10 fps
#ifdef HAVE_MEDIA_ENCODER
		if (Media_Capturing())
			idlesec = 0;
#endif
		if ((realtime - oldrealtime) < idlesec)
		{
#ifdef HAVE_SERVER
			if (sv.state)
			{
				RSpeedRemark();
				SV_Frame();
				RSpeedEnd(RSPEED_SERVER);
			}
			else
				MSV_PollSlaves();
#endif
			while(COM_DoWork(0, false))
				;
			return idlesec - (realtime - oldrealtime);
		}
	}

#ifdef PLUGINS
	Plug_Tick();
#endif
	NET_Tick();

/*
	if (cl_maxfps.value)
		fps = cl_maxfps.value;//max(30.0, min(cl_maxfps.value, 72.0));
	else
		fps = max(30.0, min(rate.value/80.0, 72.0));

	if (!cls.timedemo && realtime - oldrealtime < 1.0/fps)
		return;			// framerate is too high

	*/
#ifdef RUNTIMELIGHTING
	RelightThink();	//think even on idle (which means small walls and a fast cpu can get more surfaces done.
#endif

#ifdef HAVE_SERVER
	if (sv.state && cls.state != ca_active)
	{
		maxfpsignoreserver = false;
		maxfps = 0;//cl_maxfps.ival;
	}
	else
#endif
		if ((cl_netfps.value>0 || cls.demoplayback || runningindepphys))
	{	//limit the fps freely, and expect the netfps to cope.
		maxfpsignoreserver = true;
		maxfps = cl_maxfps.ival;
	}
	else
	{
		maxfpsignoreserver = false;
		maxfps = (cl_maxfps.ival>0||cls.protocol!=CP_QUAKEWORLD)?cl_maxfps.value:((cl_netfps.value>0)?cl_netfps.value:cls.maxfps);
		/*gets buggy at times longer than 250ms (and 0/negative, obviously)*/
		if (maxfps < 4)
			maxfps = 4;
	}

	if (vid.isminimized && (maxfps <= 0 || maxfps > 10))
		maxfps = 10;

	if (maxfps > 0
#ifdef HAVE_MEDIA_ENCODER
		&& Media_Capturing() != 2
#endif
		&& !(vrflags&VRF_OVERRIDEFRAMETIME))
	{
		spare = CL_FilterTime((realtime - oldrealtime)*1000, maxfps, 1.5, maxfpsignoreserver);
		if (!spare)
		{
			while(COM_DoWork(0, false))
				;
			return (cl_yieldcpu.ival || vid.isminimized || idle)? (1.0 / maxfps - (realtime - oldrealtime)) : 0;
		}
		if (spare > cl_maxfps_slop.ival)
			spare = cl_maxfps_slop.ival;
		spare /= 1000;
		if (spare > 0.5/maxfps)	//don't delay the next by
			spare = 0.5/maxfps;
		if (spare < 0 || cls.state < ca_onserver)
			spare = 0;
	}
	else
		spare = 0;
	host_frametime = (realtime-spare - oldrealtime)*cl.gamespeed;
	oldrealtime = realtime-spare;

	if (host_speeds.ival)
		time0 = Sys_DoubleTime ();	//end-of-idle

	if (cls.demoplayback && !cl.stillloading)
	{
		qboolean haswork = cl.sendprespawn || COM_HasWork();
		if (!hadwork && !haswork)
			CL_ProgressDemoTime();
		hadwork = haswork;
	}
	cl.stillloading = cl.sendprespawn
#ifdef LOADERTHREAD
		|| (cls.state < ca_active && worker_flush.ival && COM_HasWork())
#endif
		;
	COM_MainThreadWork();

//	if (host_frametime > 0.2)
//		host_frametime = 0.2;

	// get new key events
	Sys_SendKeyEvents ();	//from windowing system
	INS_Move();				//from things that need special polling

	// check what we got, and handle any click/button events
	IN_Commands ();

	// process console commands from said click/button events
	Cbuf_Execute ();

#ifdef HAVE_SERVER
	if (isDedicated)	//someone changed it.
	{
		if (sv.state)
		{
			float ohft = host_frametime;
			RSpeedRemark();
			SV_Frame();
			RSpeedEnd(RSPEED_SERVER);
			host_frametime = ohft;
		}
		else
			MSV_PollSlaves();
		return 0;
	}
#endif

	cls.framecount++;

	RSpeedRemark();

	CL_UseIndepPhysics(cls.state != ca_disconnected && !!cl_threadedphysics.ival);	//starts/stops the input frame thread.

	cl.do_lerp_players = cl_lerp_players.ival || cls.demoplayback==DPB_MVD || (cls.demoplayback && !cl_nolerp.ival && !cls.timedemo);
	CL_AllowIndependantSendCmd(false);

	mustrenderbeforeread = cls.protocol == CP_QUAKE2;	//FIXME: quake2 MUST render a frame (or a later one) before it can read any acks from the server, otherwise its prediction screws up. I'm too lazy to rewrite that right now.
//	if (mustrenderbeforeread)
	CL_ReadPackets();	//this should be redundant.

	CL_RequestNextDownload();

	// send intentions now
	// resend a connection request if necessary
	if (cls.state == ca_disconnected)
	{
		CL_SendCmd (host_frametime, true);
//		IN_Move(NULL, 0, time);
		CL_CheckForResend ();

#ifdef VOICECHAT
		S_Voip_Transmit(0, NULL);
#endif
	}
	else
	{
		if (connectinfo.trying)
			CL_CheckForResend ();
		CL_SendCmd (cl.gamespeed?host_frametime/cl.gamespeed:host_frametime, true);

		if (cls.state == ca_onserver && cl.validsequence && cl.worldmodel)
		{	// first update is the final signon stage
			if (cls.protocol == CP_NETQUAKE)
			{
				//nq can send 'frames' without any entities before we're on the server, leading to short periods where the local player's position is not known. this is bad. so be more cautious with nq. this might break csqc.
				CL_TransitionEntities();
				if (cl.currentpackentities->num_entities || cl.currentpackentities->servertime
#ifdef CSQC_DAT
					|| (cls.fteprotocolextensions & PEXT_CSQC)
#endif
					)
					CL_MakeActive("Quake");
			}
			else
				CL_MakeActive("QuakeWorld");
		}
	}
	CL_AllowIndependantSendCmd(true);

	RSpeedEnd(RSPEED_PROTOCOL);

#ifdef HAVE_SERVER
	if (sv.state)
	{
		float ohft = host_frametime;
		RSpeedRemark();
		SV_Frame();
		RSpeedEnd(RSPEED_SERVER);
		host_frametime = ohft;
//		if (cls.protocol != CP_QUAKE3 && cls.protocol != CP_QUAKE2)
//			CL_ReadPackets ();	//q3's cgame cannot cope with input commands with the same time as the most recent snapshot value
	}
	else
		MSV_PollSlaves();
#endif

	// fetch results from server... now that we've run it.
	if (!mustrenderbeforeread)
	{
		CL_AllowIndependantSendCmd(false);
		CL_ReadPackets ();
		CL_AllowIndependantSendCmd(true);
	}

	CL_CalcClientTime();

	// update video
	if (host_speeds.ival)
		time1 = Sys_DoubleTime ();

	if (!VID_MayRefresh || VID_MayRefresh())
	{
		if (R2D_Flush)
		{
			R2D_Flush();
			Con_Printf("R2D_Flush was set outside of SCR_UpdateScreen\n");
		}

		cl.mouseplayerview = NULL;
		cl.mousenewtrackplayer = -1;
		for (i = 0; i < MAX_SPLITS; i++)
		{
			cl.playerview[i].audio.defaulted = true;
			cl.playerview[i].audio.entnum = cl.playerview[i].viewentity;
			VectorClear(cl.playerview[i].audio.origin);
			VectorSet(cl.playerview[i].audio.forward, 1, 0, 0);
			VectorSet(cl.playerview[i].audio.right, 0, 1, 0);
			VectorSet(cl.playerview[i].audio.up, 0, 0, 1);
			cl.playerview[i].audio.reverbtype = 0;
			VectorClear(cl.playerview[i].audio.velocity);
		}
		if (cls.state && r_worldentity.model && r_worldentity.model->loadstate == MLS_NOTLOADED)
			Mod_LoadModel(cl.worldmodel, MLV_WARNSYNC);

		if (SCR_UpdateScreen && !vid.isminimized)
		{
			extern cvar_t r_stereo_method;
			r_refdef.warndraw = false;
			r_refdef.stereomethod = r_stereo_method.ival;
			{
				RSpeedMark();
				vid.ime_allow = false;
				vrui.enabled |= cl_vrui_force.ival || (vrflags&VRF_UIACTIVE);
				if (SCR_UpdateScreen())
					fps_count++;
				if (R2D_Flush)
					Sys_Error("update didn't flush 2d cache\n");
				RSpeedEnd(RSPEED_TOTALREFRESH);
			}
			r_refdef.warndraw = true;
		}
		else
			fps_count++;

		sh_config.showbatches = false;
	}

	if (host_speeds.ival)
		time2 = Sys_DoubleTime ();

	// update audio
	for (i = 0 ; i < MAX_SPLITS; i++)
	{
		playerview_t *pv = &cl.playerview[cl.splitclients?i % cl.splitclients:0];
		S_UpdateListener (i, pv->audio.entnum, pv->audio.origin, pv->audio.forward, pv->audio.right, pv->audio.up, pv->audio.reverbtype, pv->audio.velocity);
	}

	S_Update ();

	CDAudio_Update();

	if (host_speeds.ival)
	{
		pass0 = (time0 - time3)*1000000;
		time3 = Sys_DoubleTime ();
		pass1 = (time1 - time0)*1000000;
		pass2 = (time2 - time1)*1000000;
		pass3 = (time3 - time2)*1000000;
		Con_Printf ("%4i tot %4i idle %4i server %4i gfx %4i snd\n",
					pass0+pass1+pass2+pass3, pass0, pass1, pass2, pass3);
	}


//	IN_Commands ();

	// process console commands
//	Cbuf_Execute ();


	CL_QTVPoll();

#ifdef QUAKESTATS
	TP_UpdateAutoStatus();
#endif

	host_framecount++;
	cl.lasttime = cl.time;
	return 0;
}

static void simple_crypt(char *buf, int len)
{
	if (!(*buf & 128))
		return;
	while (len--)
		*buf++ ^= 0xff;
}

void Host_FixupModelNames(void)
{
	simple_crypt(emodel_name, sizeof(emodel_name) - 1);
	simple_crypt(pmodel_name, sizeof(pmodel_name) - 1);
	simple_crypt(prespawn_name,  sizeof(prespawn_name)  - 1);
	simple_crypt(modellist_name, sizeof(modellist_name) - 1);
	simple_crypt(soundlist_name, sizeof(soundlist_name) - 1);
}



#ifdef Q3CLIENT
void CL_ReadCDKey(void)
{	//q3 cdkey
	//you don't need one, just use a server without sv_strictauth set to 0.
	char *buffer;
	buffer = COM_LoadTempFile("q3key", FSLF_IGNOREPURE, NULL);
	if (buffer)	//a cdkey is meant to be 16 chars
	{
		char *chr;
		for (chr = buffer; *chr; chr++)
		{
			if (*(unsigned char*)chr < ' ')
			{
				*chr = '\0';	//don't get more than one line.
				break;
			}
		}
		Cvar_Get("cl_cdkey", buffer, CVAR_MAPLATCH|CVAR_NOUNSAFEEXPAND, "Q3 compatability");
	}
}
#endif

//============================================================================

void CL_StartCinematicOrMenu(void)
{
	COM_MainThreadWork();

	if (com_installer && FS_DownloadingPackage())
	{
		startuppending = true;
		return;
	}
	if (cls.download)
	{
		startuppending = true;
		return;
	}
	Cmd_StuffCmds();
	if (startuppending)
	{
		if (startuppending == 2)	//installer finished.
			Cbuf_AddText("\nfs_restart\nvid_restart\n", RESTRICT_LOCAL);
		startuppending = false;
		Key_Dest_Remove(kdm_console);	//make sure console doesn't stay up weirdly.
	}

	Cbuf_AddText("menu_restart\n", RESTRICT_LOCAL);

	Con_TPrintf ("^Ue080^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081 %s %sInitialized ^Ue081^Ue081^Ue081^Ue081^Ue081^Ue081^Ue082\n", *fs_gamename.string?fs_gamename.string:"Nothing", com_installer?"Installer ":"");

	//there might be some console command or somesuch waiting for the renderer to begin (demos or map command or whatever all need model support).
	realtime+=1;
	Cbuf_Execute ();	//server may have been waiting for the renderer

	Con_ClearNotify();

	if (com_installer)
	{
		com_installer = false;
#if 0
		Key_Dest_Remove(kdm_console);	//make sure console doesn't stay up weirdly.
		M_Menu_Installer();
		return;
#endif
	}

	if (!sv_state && !cls.demoinfile && !cls.state && !*cls.servername)
	{	//this is so default.cfg can define startup commands to exec if the engine starts up with no +connect / +map etc arg
		TP_ExecTrigger("f_startup", true);
		Cbuf_Execute ();
	}

	//and any startup cinematics
#ifdef HAVE_MEDIA_DECODER
	if (!sv_state && !cls.demoinfile && !cls.state && !*cls.servername)
	{
		int ol_depth;
		int idcin_depth;
		int idroq_depth;

		idcin_depth = COM_FDepthFile("video/idlog.cin", true);	//q2
		idroq_depth = COM_FDepthFile("video/idlogo.roq", true);	//q3
		ol_depth = COM_FDepthFile("video/openinglogos.roq", true);	//jk2

		if (ol_depth != FDEPTH_MISSING && (ol_depth <= idroq_depth || ol_depth <= idcin_depth))
			Media_PlayFilm("video/openinglogos.roq", true);
		else if (idroq_depth != FDEPTH_MISSING && idroq_depth <= idcin_depth)
			Media_PlayFilm("video/idlogo.roq", true);
		else if (idcin_depth != FDEPTH_MISSING)
			Media_PlayFilm("video/idlog.cin", true);

#ifdef HAVE_LEGACY
		//and for fun (blame spirit):
		if (COM_FCheckExists("data/local/video/New_Bliz640x480.bik"))
			Media_PlayFilm("av:data/local/video/New_Bliz640x480.bik", true);
		if (COM_FCheckExists("data/local/video/BlizNorth640x480.bik"))
			Media_PlayFilm("av:data/local/video/BlizNorth640x480.bik", true);
		if (COM_FCheckExists("data/local/video/eng/d2intro640x292.bik"))
			Media_PlayFilm("av:data/local/video/eng/d2intro640x292.bik", true);
		if (COM_FCheckExists("Data/Local/Video/ENG/D2x_Intro_640x292.bik"))
			Media_PlayFilm("av:Data/Local/Video/ENG/D2x_Intro_640x292.bik", true);
#endif
	}
#endif

	if (!sv_state && !cls.demoinfile && !cls.state && !*cls.servername)
	{
		if (qrenderer > QR_NONE && !Key_Dest_Has(~kdm_game))
		{
#ifndef NOBUILTINMENUS
			if (!cls.state && !Key_Dest_Has(~kdm_game) && !*FS_GetGamedir(false))
				M_Menu_Mods_f();
#endif
			if (!cls.state && !Key_Dest_Has(~kdm_game) && cl_demoreel.ival)
			{
				cls.demonum = MAX_DEMOS;
				CL_NextDemo();
			}
			if (!cls.state && !Key_Dest_Has(~kdm_game))
				//if we're (now) meant to be using csqc for menus, make sure that its running.
				if (!CSQC_UnconnectedInit())
					M_ToggleMenu_f();
		}
		//Con_ForceActiveNow();
	}
}

void CL_ArgumentOverrides(void)
{
	int i;
	if (COM_CheckParm ("-window") || COM_CheckParm ("-startwindowed"))
		Cvar_Set(Cvar_FindVar("vid_fullscreen"), "0");
	if (COM_CheckParm ("-fullscreen"))
		Cvar_Set(Cvar_FindVar("vid_fullscreen"), "1");

	if ((i = COM_CheckParm ("-width")))	//width on it's own also sets height
	{
		Cvar_Set(Cvar_FindVar("vid_width"), com_argv[i+1]);
		Cvar_SetValue(Cvar_FindVar("vid_height"), (atoi(com_argv[i+1])/4)*3);
	}
	if ((i = COM_CheckParm ("-height")))
		Cvar_Set(Cvar_FindVar("vid_height"), com_argv[i+1]);

	if ((i = COM_CheckParm ("-conwidth")))	//width on it's own also sets height
	{
		Cvar_Set(Cvar_FindVar("vid_conwidth"), com_argv[i+1]);
		Cvar_SetValue(Cvar_FindVar("vid_conheight"), (atoi(com_argv[i+1])/4)*3);
	}
	if ((i = COM_CheckParm ("-conheight")))
		Cvar_Set(Cvar_FindVar("vid_conheight"), com_argv[i+1]);

	if ((i = COM_CheckParm ("-bpp")))
		Cvar_Set(Cvar_FindVar("vid_bpp"), com_argv[i+1]);

	if (COM_CheckParm ("-current"))
		Cvar_Set(Cvar_FindVar("vid_desktopsettings"), "1");

	if (COM_CheckParm("-condebug"))
		Cvar_Set(Cvar_FindVar("log_enable"), "1");

	if ((i = COM_CheckParm ("-particles")))
		Cvar_Set(Cvar_FindVar("r_part_maxparticles"), com_argv[i+1]);

	if (COM_CheckParm("-qmenu"))
		Cvar_ForceSet(Cvar_FindVar("forceqmenu"), "1");
}

//note that this does NOT include commandline.
void CL_ExecInitialConfigs(char *resetcommand, qboolean fullvidrestart)
{
#ifndef QUAKETC
	int qrc, hrc;
#endif
	int def;

	Cbuf_Execute ();	//make sure any pending console commands are done with. mostly, anyway...
	
	Cbuf_AddText("unbindall\nshowpic_removeall\n", RESTRICT_LOCAL);
	Cbuf_AddText("alias restart_ents \"changelevel . .\"\n",RESTRICT_LOCAL);
	Cbuf_AddText("alias restart map_restart\n",RESTRICT_LOCAL);
	Cbuf_AddText("alias startmap_sp \"map start\"\n", RESTRICT_LOCAL);
#ifdef QUAKESTATS
	Cbuf_AddText("alias +attack2 +button3\n", RESTRICT_LOCAL);
	Cbuf_AddText("alias -attack2 -button3\n", RESTRICT_LOCAL);
#endif
	Cbuf_AddText("cl_warncmd 0\n", RESTRICT_LOCAL);
	Cbuf_AddText("cvar_purgedefaults\n", RESTRICT_LOCAL);	//reset cvar defaults to their engine-specified values. the tail end of 'exec default.cfg' will update non-cheat defaults to mod-specified values.
	Cbuf_AddText("cvarreset *\n", RESTRICT_LOCAL);			//reset all cvars to their current (engine) defaults
#ifdef HAVE_SERVER
	Cbuf_AddText(va("sv_gamedir \"%s\"\n", FS_GetGamedir(true)), RESTRICT_LOCAL);
#endif
	Cbuf_AddText(resetcommand, RESTRICT_LOCAL);
	Cbuf_AddText("\n", RESTRICT_LOCAL);
	COM_ParsePlusSets(true);

#ifdef QUAKESTATS
	Cbuf_AddText("register_bestweapon reset\n", RESTRICT_LOCAL);
#endif

	def = COM_FDepthFile("default.cfg", true);	//q2/q3/tc
#ifdef QUAKETC
	Cbuf_AddText ("exec default.cfg\n", RESTRICT_LOCAL);
	if (COM_FDepthFile ("config.cfg", true) <= def)
		Cbuf_AddText ("exec config.cfg\n", RESTRICT_LOCAL);
	if (COM_FCheckExists ("autoexec.cfg"))
		Cbuf_AddText ("exec autoexec.cfg\n", RESTRICT_LOCAL);
#else
	//who should we imitate?
	qrc = COM_FDepthFile("quake.rc", true);	//q1
	hrc = COM_FDepthFile("hexen.rc", true);	//h2

	if (qrc <= def && qrc <= hrc && qrc!=FDEPTH_MISSING)
	{
		Cbuf_AddText ("exec quake.rc\n", RESTRICT_LOCAL);
		def = qrc;
	}
	else if (hrc <= def && hrc!=FDEPTH_MISSING)
	{
		Cbuf_AddText ("exec hexen.rc\n", RESTRICT_LOCAL);
		def = hrc;
	}
	else
	{	//they didn't give us an rc file!
//		int cfg = COM_FDepthFile ("config.cfg", true);
		int q3cfg = COM_FDepthFile ("q3config.cfg", true);
	//	Cbuf_AddText ("bind ` toggleconsole\n", RESTRICT_LOCAL);	//in case default.cfg does not exist. :(
		Cbuf_AddText ("exec default.cfg\n", RESTRICT_LOCAL);
		if (q3cfg <= def && q3cfg!=FDEPTH_MISSING)
			Cbuf_AddText ("exec q3config.cfg\n", RESTRICT_LOCAL);
		else //if (cfg <= def && cfg!=0x7fffffff)
			Cbuf_AddText ("exec config.cfg\n", RESTRICT_LOCAL);
		if (def!=FDEPTH_MISSING)
			Cbuf_AddText ("exec autoexec.cfg\n", RESTRICT_LOCAL);
	}
#endif
#ifdef QUAKESPYAPI
	if (COM_FCheckExists ("frontend.cfg"))
		Cbuf_AddText ("exec frontend.cfg\n", RESTRICT_LOCAL);
#endif
	Cbuf_AddText ("cl_warncmd 1\n", RESTRICT_LOCAL);	//and then it's allowed to start moaning.
	COM_ParsePlusSets(true);

	com_parseutf8.ival = com_parseutf8.value;

	//if the renderer is already up and running, be prepared to reload content to match the new conback/font/etc
	if (r_blockvidrestart)
		;
	else if (fullvidrestart)
		Cbuf_AddText ("vid_restart\n", RESTRICT_LOCAL);
	else if (qrenderer != QR_NONE)
		Cbuf_AddText ("vid_reload\n", RESTRICT_LOCAL);
//	if (Key_Dest_Has(kdm_menu))
//		Cbuf_AddText ("closemenu\ntogglemenu\n", RESTRICT_LOCAL);	//make sure the menu has the right content loaded.

	Cbuf_Execute ();	//if the server initialisation causes a problem, give it a place to abort to

	//assuming they didn't use any waits in their config (fools)
	//the configs should be fully loaded.
	//so convert the backwards compable commandline parameters in cvar sets.
	CL_ArgumentOverrides();
#ifdef HAVE_SERVER
	SV_ArgumentOverrides();
#endif

	//and disable the 'you have unsaved stuff' prompt.
	Cvar_Saved();

	Ruleset_Scan();
}

static void Host_URIPrompt(void *ctx, promptbutton_t btn)
{
	if (btn == PROMPT_YES)
		Cbuf_AddText ("\nsys_register_file_associations\n", RESTRICT_LOCAL);
}

void Host_FinishLoading(void)
{
	int i;
	extern qboolean r_forceheadless;
	if (r_blockvidrestart == true)
	{
		//1 means we need to init the filesystem

		//the filesystem has retrieved its manifest, but might still be waiting for paks to finish downloading.

		//make sure the filesystem has some default if no manifest was loaded.
		FS_ChangeGame(NULL, true, true);

		if (waitingformanifest)
		{
#ifdef MULTITHREAD
			Sys_Sleep(0.1);
#endif
			return;
		}

#ifdef PLUGINS
		Plug_Initialise(true);
#endif

		Con_History_Load();

		r_blockvidrestart = 2;

		CL_ArgumentOverrides();
	#ifdef HAVE_SERVER
		SV_ArgumentOverrides();
	#endif

		Con_TPrintf ("\nEngine Version: %s\n", version_string());

		Con_DPrintf("This program is free software; you can redistribute it and/or "
					"modify it under the terms of the GNU General Public License "
					"as published by the Free Software Foundation; either version 2 "
					"of the License, or (at your option) any later version."
					"\n"
					"This program is distributed in the hope that it will be useful, "
					"but WITHOUT ANY WARRANTY; without even the implied warranty of "
					"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. "
					"\n"
					"See the GNU General Public License for more details.\n");

	#if defined(_WIN32) && !defined(FTE_SDL) && !defined(_XBOX) && defined(MANIFESTDOWNLOADS)
		if (Sys_RunInstaller())
			Sys_Quit();
	#endif

		Menu_Download_Update();

#ifdef IPLOG
		IPLog_Merge_File("iplog.txt");
		IPLog_Merge_File("iplog.dat");	//legacy crap, for compat with proquake
#endif

		if (!PM_IsApplying())
			Cmd_StuffCmds();
	}

	if (PM_IsApplying() == 1)
	{
#ifdef MULTITHREAD
		Sys_Sleep(0.1);
#endif
		return;
	}

	//open any files specified on the commandline (urls, paks, models, I dunno).
	for (i = 1; i < com_argc; i++)
	{
		if (!com_argv[i])
			continue;
		if (*com_argv[i] == '+' || *com_argv[i] == '-')
			break;
		Host_RunFile(com_argv[i], strlen(com_argv[i]), NULL);
	}

	//android may find that it has no renderer at various points.
	if (r_forceheadless)
		return;

	if (r_blockvidrestart == 2)
	{	//2 is part of the initial startup
		Renderer_Start();
		CL_StartCinematicOrMenu();
	}
	else	//3 flags for a renderer restart
		Renderer_Start();


	if (fs_manifest->schemes && Cmd_IsCommand("sys_register_file_associations"))
	{
		if (cl_verify_urischeme.ival >= 2)
			Cbuf_AddText ("\nsys_register_file_associations\n", RESTRICT_LOCAL);
		else if (cl_verify_urischeme.ival)
		{
			char *scheme = Sys_URIScheme_NeedsRegistering();
			if (scheme)
			{
				Menu_Prompt(Host_URIPrompt, NULL, va(localtext("The URI scheme %s:// is not configured.\nRegister now?"), scheme), "Register", NULL, "No", true);
				Z_Free(scheme);
			}
		}
	}
}

/*
====================
Host_Init
====================
*/
void Host_Init (quakeparms_t *parms)
{
/*#ifdef PACKAGEMANAGER
	char engineupdated[MAX_OSPATH];
#endif*/
	int man;

	com_parseutf8.ival = 1;	//enable utf8 parsing even before cvars are registered.

	COM_InitArgv (parms->argc, parms->argv);

	if (setjmp (host_abort) )
		Sys_Error("Host_Init: An error occured. Try the -condebug commandline parameter\n");


	host_parms = *parms;

	Cvar_Init();
	Memory_Init ();

	/*memory is working, its safe to printf*/
	Con_Init ();

	Sys_Init();

	COM_ParsePlusSets(false);
	Cbuf_Init ();
	Cmd_Init ();
	COM_Init ();

/*this may be tripping some bullshit huristic in microsoft's insecurity mafia software(tbh I really don't know what they're detecting), plus causes firewall issues on updates.
#ifdef PACKAGEMANAGER
	//we have enough of the filesystem inited now that we can read the package list and figure out which engine was last installed.
	if (PM_FindUpdatedEngine(engineupdated, sizeof(engineupdated)))
	{
		PM_Shutdown();	//will restart later as needed, but we need to be sure that no files are open or anything.
		if (Sys_EngineWasUpdated(engineupdated))
		{
			COM_Shutdown();
			Cmd_Shutdown();
			Sys_Shutdown();
			Con_Shutdown();
			Memory_DeInit();
			Cvar_Shutdown();
			Sys_Quit();
			return;
		}
		PM_Shutdown();	//will restart later as needed, but we need to be sure that no files are open or anything.
	}
#endif
*/

	V_Init ();
	NET_Init ();

#if defined(Q2BSPS) || defined(Q3BSPS)
	CM_Init();
#endif
#ifdef TERRAIN
	Terr_Init();
#endif
	Host_FixupModelNames();

	Netchan_Init ();
	Renderer_Init();
	Mod_Init(true);

#if defined(CSQC_DAT) || defined(MENU_DAT)
	PF_Common_RegisterCvars();
#endif
#ifdef HAVE_SERVER
	SV_Init(parms);
#endif

//	W_LoadWadFile ("gfx.wad");
	Key_Init ();
	M_Init ();
	IN_Init ();
	S_Init ();
	cls.state = ca_disconnected;
	CDAudio_Init ();
	Sbar_Init ();
	CL_Init ();

#ifdef PLUGINS
	Plug_Initialise(false);
#endif

#ifdef TEXTEDITOR
	Editor_Init();
#endif

#ifdef CL_MASTER
	Master_SetupSockets();
#endif

#ifdef Q3CLIENT
	CL_ReadCDKey();
#endif

	//	Con_Printf ("Exe: "__TIME__" "__DATE__"\n");
	//Con_Printf ("%4.1f megs RAM available.\n", parms->memsize/ (1024*1024.0));

	R_SetRenderer(NULL);//set the renderer stuff to unset...

	Cvar_ParseWatches();
	host_initialized = true;
	forcesaveprompt = false;

#ifdef PLUGINS
	Plug_Initialise(false);
#endif

	Sys_SendKeyEvents();

	//the engine is fully running, except the file system may be nulled out waiting for a manifest to download.

	man = COM_CheckParm("-manifest");
	if (man && man < com_argc-1 && com_argv[man+1])
		Host_RunFile(com_argv[man+1], strlen(com_argv[man+1]), NULL);
}

/*
===============
Host_Shutdown

FIXME: this is a callback from Sys_Quit and Sys_Error.  It would be better
to run quit through here before the final handoff to the sys code.
===============
*/
void Host_Shutdown(void)
{
	size_t i;
	if (!host_initialized)
		return;
	host_initialized = false;

	CL_UseIndepPhysics(false);

#ifdef WEBCLIENT
	HTTP_CL_Terminate();
#endif

	//disconnect server/client/etc
	CL_Disconnect_f();

	M_Shutdown(true);

#ifdef CSQC_DAT
	CSQC_Shutdown();
#endif

	S_Shutdown(true);
	CDAudio_Shutdown ();
	IN_Shutdown ();
	R_ShutdownRenderer(true);

#ifdef PLUGINS
	Plug_Shutdown(false);
#endif

//	Host_WriteConfiguration ();
#ifdef CL_MASTER
	MasterInfo_Shutdown();
#endif
	CL_FreeDlights();
	CL_FreeVisEdicts();
	Mod_Shutdown(true);
	Wads_Flush();
	Con_History_Save();	//do this outside of the console code so that the filesystem is still running at this point but still allowing the filesystem to make console prints (you might not see them, but they should be visible to sys_printf still, for debugging).
#ifdef HAVE_SERVER
	SV_Shutdown();
#else
	Log_ShutDown();
	NET_Shutdown ();
	FS_Shutdown();
#endif
#ifdef QUAKEHUD
	Stats_Clear();
#endif
	Ruleset_Shutdown();

	COM_DestroyWorkerThread();

	P_ShutdownParticleSystem();
	Cvar_Shutdown();
	Validation_FlushFileList();

#ifdef HAVE_GNUTLS
	GnuTLS_Shutdown();
#endif

	Cmd_Shutdown();
#ifdef PACKAGEMANAGER
	PM_Shutdown(false);
#endif
	Key_Unbindall_f();

#ifdef PLUGINS
	Plug_Shutdown(true);
#endif

	for (i = 0; i < MAX_SPLITS; i++)
		InfoBuf_Clear(&cls.userinfo[i], true);
	InfoSync_Clear(&cls.userinfosync);

	Con_Shutdown();
	COM_BiDi_Shutdown();
	Memory_DeInit();

#ifdef HAVE_SERVER
	SV_WipeServerState();
	memset(&svs, 0, sizeof(svs));
#endif
	Sys_Shutdown();
}

#ifndef HAVE_SERVER
void SV_EndRedirect (void)
{
}
#endif
