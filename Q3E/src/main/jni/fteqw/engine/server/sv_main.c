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
#include "quakedef.h"
#include "netinc.h"
#include "fs.h"	//for updates
#ifdef SQL
#include "sv_sql.h"
#endif
#include <sys/types.h>
#ifndef CLIENTONLY
#define Q2EDICT_NUM(i) (q2edict_t*)((char *)ge->edicts+(i)*ge->edict_size)

#define INVIS_CHAR1 12
#define INVIS_CHAR2 (char)138
#define INVIS_CHAR3 (char)160

client_t	*host_client;			// current client

void CvarPostfixKMG(cvar_t *v, char *oldval)
{
	double k = 1000;	//scientific units are in thousands. 10ki is 10*1024.
	char *end;
	double f = strtod(v->string, &end);
	if (*end && end[1]=='i')
		k = 1024;	//kibi
	if (*end == 'k' || *end == 'K')
		f *= k;
	if (*end == 'm' || *end == 'M')
		f *= k*k;
	if (*end == 'b' || *end == 'B' || *end == 'g' || *end == 'G')
		f *= k*k*k;
	v->ival = f;
	v->value = f;
}

// bound the size of the physics time tic
#ifdef SERVERONLY
cvar_t	sv_mintic					= CVARD("sv_mintic","0.013", "The minimum interval between running physics frames.");
#else
cvar_t	sv_mintic					= CVARD("sv_mintic","0", "The minimum interval between running physics frames.");	//client builds can think as often as they want.
#endif
cvar_t	sv_maxtic					= CVARD("sv_maxtic","0.1", "The maximum interval between running physics frames. If the value is too low, multiple physics interations might be run at a time (based upon sv_limittics). Set to 0 for fixed-interval ticks, which may be required if ODE is used.");//never run a tick slower than this
cvar_t	sv_limittics				= CVARD("sv_limittics","3", "The maximum number of ticks that may be run within a frame, to allow the server to catch up if it stalled or if sv_maxtic is too low.");//

cvar_t	sv_nailhack					= CVARD("sv_nailhack","1", "If set to 1, disables the nail entity networking optimisation. This hack was popularised by qizmo which recommends it for better compression. Also allows clients to interplate nail positions and add trails.");
cvar_t	sv_nopvs					= CVARD("sv_nopvs", "0", "Set to 1 to ignore pvs on the server. This can make wallhacks more dangerous, so should only be used for debugging.");
cvar_t	fraglog_public				= CVARD("fraglog_public", "1", "Enables support for connectionless fraglog requests");
cvar_t	fraglog_details				= CVARD("fraglog_details", "1", "Bitmask\n1: killer+killee names.\n2: killer+killee teams\n4:timestamp.\n8:killer weapon\n16:killer+killee guid.\nFor compatibility, use 1(vanilla) or 7(mvdsv).");

cvar_t	zombietime					= CVARD("zombietime", "2", "Client slots will not be reused for this number of seconds.");	// seconds to sink messages

cvar_t	sv_rconlim					= CVARFD("sv_rconlim", "4", CVAR_ARCHIVE, "Blocks repeated (invalid) rcon attempts.");
cvar_t	sv_crypt_rcon				= CVARFD("sv_crypt_rcon", "", CVAR_ARCHIVE, "Controls whether the rcon password must be hashed or not. Hashed passwords also partially prevent replay attacks, but does NOT prevent malicious actors from reading the commands/results.\n0: completely insecure. ONLY allows plain-text passwords. Do not use.\n1: Mandatory hashing (recommended).\nEmpty: Allow either, whether the password is secure or not is purely the client's responsibility/fault. Only use this for comptibility with old clients.");
cvar_t	sv_crypt_rcon_clockskew		= CVARFD("sv_timestamplen", "60", CVAR_ARCHIVE, "Limits clock skew to reduce (delayed) replay attacks");
#ifdef SERVERONLY
cvar_t	rcon_password				= CVARF("rcon_password", "", CVAR_NOUNSAFEEXPAND);	// password for remote server commands
cvar_t	password					= CVARF("password", "", CVAR_NOUNSAFEEXPAND);	// password for entering the game
#else
extern cvar_t	rcon_password;
extern cvar_t	password;
#endif
cvar_t	spectator_password			= CVARF("spectator_password", "", CVAR_NOUNSAFEEXPAND);	// password for entering as a sepctator

cvar_t	allow_download				= CVARAD("allow_download", "1", /*q3*/"sv_allowDownload", "If 1, permits downloading. Set to 0 to unconditionally block *ALL* downloads from this server. You may wish to set sv_dlURL if you wish clients to still be able to download content.");
cvar_t	allow_download_skins		= CVARD("allow_download_skins", "1", "0 blocks downloading of any file in the skins/ directory");
cvar_t	allow_download_models		= CVARD("allow_download_models", "1", "0 blocks downloading of any file in the progs/ or models/ directory");
cvar_t	allow_download_sounds		= CVARD("allow_download_sounds", "1", "0 blocks downloading of any file in the sound/ directory");
cvar_t	allow_download_particles	= CVARD("allow_download_particles", "1", "0 blocks downloading of any file in the particles/ directory");
cvar_t	allow_download_demos		= CVARD("allow_download_demos", "1", "0 blocks downloading of any file in the demos/ directory");
cvar_t	allow_download_maps			= CVARD("allow_download_maps", "1", "0 blocks downloading of any file in the maps/ directory");
cvar_t	allow_download_logs			= CVARFD("allow_download_logs", "0", CVAR_NOSET/*cmdline-only*/|CVAR_WARNONCHANGE, "1 permits downloading files with the extension .log\n"CON_ERROR"THIS IS DANGEROUS AS IT POTENTIALLY ALLOWS PEOPLE TO SEE PASSWORDS OR OTHER PRIVATE INFORMATION.\nNote that it can be switch on/off via rcon.");
cvar_t	allow_download_anymap		= CVARFD("allow_download_pakmaps", "0", CVAR_WARNONCHANGE, "0: clients may not download map files within the server's packages.\n1: clients may download such files so long as the package is not deemed copyrighted, for compat with old clients that do not support package downloads.\n2: client may download such files regardless of copyright state (WARNING! Should never be used!).");
cvar_t	allow_download_pakcontents	= CVARFD("allow_download_pakcontents", "0", CVAR_WARNONCHANGE, "0: clients may not download non-map files within the server's packages.\n1: clients may download such files so long as the package is not deemed copyrighted, for compat with old clients that do not support package downloads.\n2: client may download such files regardless of copyright state (WARNING! ONLY for consistency with vanilla QuakeWorld!).");
cvar_t	allow_download_root			= CVARFD("allow_download_root", "0", CVAR_WARNONCHANGE, "If set, enables downloading from the root of the gamedir (not the basedir). This setting has a lower priority than extension-based checks.");
cvar_t	allow_download_textures		= CVARD("allow_download_textures", "1", "0 blocks downloading of any file in the textures/ directory");
cvar_t	allow_download_packages		= CVARD("allow_download_packages", "1", "if 1, permits downloading files (from root directory or elsewhere) with known package extensions (eg: pak+pk3). Packages with a name starting 'pak' are covered by allow_download_copyrighted as well.");
cvar_t	allow_download_refpackages	= CVARD("allow_download_refpackages", "1", "If set to 1, packages that contain files needed during spawn functions will be become 'referenced' and automatically downloaded to clients.\nThis cvar should probably not be set if you have large packages that provide replacement pickup models on public servers.\nThe path command will show a '(ref)' tag next to packages which clients will automatically attempt to download.");
cvar_t	allow_download_wads			= CVARD("allow_download_wads", "1", "0 blocks downloading of any file in the wads/ directory, or is in the root directory with the extension .wad");
cvar_t	allow_download_configs		= CVARFD("allow_download_configs", "0", CVAR_WARNONCHANGE, "1 allows downloading of config files, either with the extension .cfg or in the subdir configs/.\n"CON_ERROR"THIS IS DANGEROUS AS IT CAN ALLOW PEOPLE TO READ YOUR RCON PASSWORD ETC.");
cvar_t	allow_download_locs			= CVARD("allow_download_locs", "1", "0 blocks downloading of any file in the locs/ directory");
cvar_t	allow_download_copyrighted	= CVARFD("allow_download_copyrighted", "0", CVAR_WARNONCHANGE, "0 blocks download of packages that are considered copyrighted. Specifically, this means packages with a leading 'pak' prefix on the filename.\nIf you take your copyrights seriously, you should also set allow_download_pakmaps 0 and allow_download_pakcontents 0.");
cvar_t	allow_download_other		= CVARD("allow_download_other", "0", "0 blocks downloading of any file that was not covered by any of the directory download blocks.");

extern cvar_t sv_allow_splitscreen;

#if defined(SUPPORT_ICE) || defined(FTE_TARGET_WEB)
static void QDECL SV_Public_Callback(struct cvar_s *var, char *oldvalue)
{
	char name[64], *e;
	COM_ParseOut(var->string, name, sizeof(name));
	strtol(name, &e, 0);
	if (*name&&e==name)	//failed to read any number out of it.
	{
		FTENET_AddToCollection(svs.sockets, var->name, va("/%s", (*name == '/')?name+1:name), NA_INVALID, NP_RTC_TLS);
		var->value = var->ival = 2;	//so other stuff sees us as holepunched.
	}
#ifdef FTE_TARGET_WEB
	else if (var->ival)	//any kind of public is webrtc public, browsers don't allow more.
#else
	else if (var->ival == 2)
#endif
		FTENET_AddToCollection(svs.sockets, var->name, "/", NA_INVALID, NP_RTC_TLS);
	else
		FTENET_AddToCollection(svs.sockets, var->name, "", NA_INVALID, NP_INVALID);
}
cvar_t sv_public			= CVARCD("sv_public", "0", SV_Public_Callback, "-1: Fully blocks all inbound connections.\n0: Disable subscribing to master servers (for private lan-only games).\n1: Subscribe to public master servers. Your IP address will be listed publicly. Make sure your Router/NAT+Firewall are set to allow inbound connections.\n2: Subscribe to a broker master, allowing firewall hole punching.");
#else
cvar_t sv_public			= CVARD("sv_public", "0", "-1: Fully blocks all inbound connections.\n0: Disable subscribing to master servers (for private lan-only games).\n1: Subscribe to public master servers. Your IP address will be listed publicly. Make sure your Router/NAT+Firewall are set to allow inbound connections.");
#endif

cvar_t sv_guidhash			= CVARD("sv_guidkey", "", "If set, clients will calculate their GUID values against this string instead of the server's IP address. This allows consistency between multiple servers (for stats tracking), but do NOT treat the client's GUID as something that is secure.");
cvar_t sv_serverip			= CVARD("sv_serverip", "", "Set this cvar to the server's public ip address if the server is behind a firewall and cannot detect its own public address. Providing a port is required if the firewall/nat remaps it, but is otherwise optional.");
cvar_t sv_listen_qw			= CVARAFD("sv_listen_qw", "1", "sv_listen", 0, "Specifies whether normal clients are allowed to connect.");
cvar_t sv_listen_nq			= CVARD("sv_listen_nq", "0", "Allow new (net)quake clients to connect to the server.\n0 = don't let them in.\n1 = allow them in (WARNING: this allows 'qsmurf' DOS attacks).\n2 = accept (net)quake clients by emulating a challenge (as secure as QW/Q2 but does not fully conform to the NQ protocol).\nYou may also need to set net_enable_dtls if you wish for the rerelease's client to connect.");
cvar_t sv_listen_dp			= CVARD("sv_listen_dp", "0", "Allows the server to respond with the DP-specific handshake protocol.\nWarning: this can potentially get confused with quake2, and results in race conditions with both vanilla netquake and quakeworld protocols.\nOn the plus side, DP clients can usually be identified correctly, enabling a model+sound limit boost.");
#ifdef QWOVERQ3
cvar_t sv_listen_q3			= CVAR("sv_listen_q3", "0");
#endif
cvar_t sv_reconnectlimit	= CVARD("sv_reconnectlimit", "0", "Blocks dupe connection within the specified length of time .");
cvar_t sv_use_dns			= CVARD("sv_use_dns", "", "Performs a reverse-dns lookup in order to report more info about where clients are connecting from.");
cvar_t sv_reportheartbeats	= CVARD("sv_reportheartbeats", "2", "Print a notice each time a heartbeat is sent to a master server. When set to 2, the message will be displayed once.");
cvar_t sv_heartbeat_interval = CVARD("sv_heartbeat_interval", "110", "Interval between heartbeats. Low values are abusive, high values may cause NAT/ghost issues.");
cvar_t sv_heartbeat_checks	= CVARD("sv_heartbeat_checks", "1", "Report when sv_public 1 fails due to PROBABLE router/NAT issues.");
cvar_t sv_highchars			= CVAR("sv_highchars", "1");
cvar_t sv_maxrate			= CVARCD("sv_maxrate", "50000", CvarPostfixKMG, "This controls the maximum number of bytes any indivual player may receive (when not downloading). The individual user's rate will also be controlled by the user's rate cvar.");
cvar_t sv_maxdrate			= CVARAFCD("sv_maxdrate", "500000",
									"sv_maxdownloadrate", 0, CvarPostfixKMG, "This cvar controls the maximum number of bytes sent to each player per second while that player is downloading.\nIf this cvar is set to 0, there will be NO CAP for download rates (if the user's drate is empty/0 too, then expect really fast+abusive downloads that could potentially be considered denial of service attacks)");
cvar_t sv_minping			= CVARFD("sv_minping", "", CVAR_SERVERINFO, "Simulate fake lag for any players with a ping under the value specified here. Value is in milliseconds.");

cvar_t sv_bigcoords			= CVARFD("sv_bigcoords", "1", 0, "Uses floats for coordinates instead of 16bit values.\nAlso boosts angle precision, so can be useful even on small maps.\nAffects clients thusly:\nQW: enforces a mandatory protocol extension\nDP: enables DPP7 protocol support\nNQ: uses RMQ protocol (protocol 999).");
cvar_t sv_calcphs			= CVARFD("sv_calcphs", "2", CVAR_MAPLATCH, "Enables culling of sound effects. 0=always skip phs. Sounds are globally broadcast. 1=always generate phs. Sounds are always culled. On large maps the phs will be dumped to disk. 2=On large single-player maps, generation of phs is skipped. Otherwise like option 1.");

cvar_t sv_showconnectionlessmessages	= CVARD("sv_showconnectionlessmessages", "0", "Display a line describing each connectionless message that arrives on the server. Primarily a debugging feature, but also potentially useful to admins.");
cvar_t sv_cullplayers_trace		= CVARFD("sv_cullplayers_trace", "", CVAR_SERVERINFO, "Attempt to cull player entities using tracelines as an anti-wallhack.");
cvar_t sv_cullentities_trace	= CVARFD("sv_cullentities_trace", "", CVAR_SERVERINFO, "Attempt to cull non-player entities using tracelines as an extreeme anti-wallhack.");
cvar_t sv_phs					= CVARD("sv_phs", "1", "If 1, do not use the phs. It is generally better to use sv_calcphs instead, and leave this as 1.");
cvar_t sv_resetparms			= CVAR("sv_resetparms", "0");
cvar_t sv_pupglow				= CVARFD("sv_pupglow", "", CVAR_SERVERINFO, "Instructs clients to enable hexen2-style powerup pulsing.");

#ifdef SV_MASTER
cvar_t sv_master				= CVAR("sv_master", "0");
#endif

cvar_t	sv_reliable_sound		= CVARFD("sv_reliable_sound", "0",  0, "Causes all sounds to be sent reliably, so they will not be missed due to packetloss. However, this will cause them to be delayed somewhat, and slightly bursty. This can be overriden using the 'rsnd' userinfo setting (either forced on or forced off). Note: this does not affect sounds attached to particle effects.");
cvar_t	sv_gamespeed		= CVARAF("sv_gamespeed", "1", "slowmo", 0);
cvar_t	sv_csqcdebug		= CVARD("sv_csqcdebug", "0", "Inject packet size information for data directed to csqc.");
cvar_t	sv_csqc_progname	= CVARAF("sv_csqc_progname", "csprogs.dat", /*dp*/"csqc_progname", 0);
cvar_t pausable				= CVAR("pausable", "");
cvar_t sv_banproxies		= CVARD("sv_banproxies", "0", "If enabled, anyone connecting via known proxy software will be refused entry. This should aid with blocking aimbots, but is only reliable for certain public proxies.");
cvar_t	sv_specprint		= CVARD("sv_specprint", "3",	"Bitfield that controls which player events spectators see when tracking that player.\n&1: spectators will see centerprints.\n&2: spectators will see sprints (pickup messages etc).\n&4: spectators will receive console commands, this is potentially risky.\nIndividual spectators can use 'setinfo sp foo' to limit this setting.");
cvar_t	sv_protocol				= CVARD("sv_protocol", "", "Specifies which protocol extensions to force. Clients which do not support the named protocols will be kicked. Recognised values: fte1 fte2 csqc.");

//
// game rules mirrored in svs.info
//
cvar_t	fraglimit		= CVARF("fraglimit",		"" ,	CVAR_SERVERINFO);
cvar_t	timelimit		= CVARF("timelimit",		"" ,	CVAR_SERVERINFO);
cvar_t	teamplay		= CVARF("teamplay",		"" ,	CVAR_SERVERINFO);
cvar_t	samelevel		= CVARF("samelevel",		"" ,	CVAR_SERVERINFO);
cvar_t	sv_playerslots	= CVARAD("sv_playerslots",	"",
								 "maxplayers",		"Specify maximum number of player/spectator/bot slots, new value takes effect on the next map (this may result in players getting kicked). This should generally be set to maxclients+maxspectators. Leave blank for a default value.\nMaximum value of "STRINGIFY(MAX_CLIENTS)". Values above 16 will result in issues with vanilla NQ clients. Effective values other than 32 will result in issues with vanilla QW clients.");
cvar_t	maxclients		= CVARAFD("maxclients",		"8",
								 "sv_maxclients",			CVAR_SERVERINFO, "Specify the maximum number of players allowed on the server at once. Can be changed mid-map.");
cvar_t	maxspectators	= CVARFD("maxspectators",	"8",	CVAR_SERVERINFO, "Specify the maximum number of spectators allowed on the server at once. Can be changed mid-map.");
#ifdef SERVERONLY
cvar_t	deathmatch		= CVARF("deathmatch",		"1",	CVAR_SERVERINFO);			// 0, 1, or 2
#else
cvar_t	deathmatch		= CVARF("deathmatch",		"",	CVAR_SERVERINFO);			// 0, 1, or 2
#endif
cvar_t	coop			= CVARF("coop",			"" ,	CVAR_SERVERINFO);
cvar_t	skill			= CVARF("skill",			"" ,	CVAR_SERVERINFO);			// 0, 1, 2 or 3
cvar_t	spawn			= CVARF("spawn",			"" ,	CVAR_SERVERINFO);
cvar_t	watervis		= CVARF("watervis",		"" ,	CVAR_SERVERINFO);
#pragma warningmsg("Remove this some time")
cvar_t	allow_skybox	= CVARFD("allow_skybox",	"",		CVAR_SERVERINFO, "This setting says whether clients should skip writing skybox depth when rendering skyboxes/skydomes. Skipping depth writes is required for halflife, quake2, and quake3 compat, but q1 content generally requires depth masking. Empty uses format-specific defaults.");
cvar_t	sv_allow_splitscreen = CVARFD("allow_splitscreen","",CVAR_SERVERINFO, "Specifies whether clients can use splitscreen extensions to dynamically add additional clients. This only affects remote clients and not the built-in client.\nClients may need to reconnect in order to add seats when this is changed.");
cvar_t	fbskins			= CVARF("fbskins",			"",	CVAR_SERVERINFO);	//to get rid of lame fuhquake fbskins

cvar_t	sv_motd[]		={	CVAR("sv_motd1",		""),
							CVAR("sv_motd2",		""),
							CVAR("sv_motd3",		""),
							CVAR("sv_motd4",		"")	};

cvar_t sv_compatiblehulls = CVAR("sv_compatiblehulls", "1");
cvar_t  dpcompat_stats = CVAR("dpcompat_stats", "0");

cvar_t	hostname = CVARF("hostname","unnamed", CVAR_SERVERINFO);

cvar_t	secure = CVARF("secure", "", CVAR_SERVERINFO);

extern cvar_t sv_nqplayerphysics;

char cvargroup_serverpermissions[] = "server permission variables";
char cvargroup_serverinfo[] = "serverinfo variables";
char cvargroup_serverphysics[] = "server physics variables";
char cvargroup_servercontrol[] = "server control variables";

vfsfile_t	*sv_fraglogfile;

void PRH2_SetPlayerClass(client_t *cl, int classnum, qboolean fromqc);
void SV_DeDupeName(const char *val, client_t *cl, char *newname, size_t newnamesize);

int	nextuserid;

//============================================================================

qboolean ServerPaused(void)
{
	return sv.paused;
}

/*
================
SV_Shutdown

Quake calls this before calling Sys_Quit or Sys_Error
================
*/
void SV_Shutdown (void)
{
	SV_Master_Shutdown ();
	if (sv_fraglogfile)
	{
		VFS_CLOSE (sv_fraglogfile);
		sv_fraglogfile = NULL;
	}

	SV_UnspawnServer();
#ifdef SUBSERVERS
	MSV_Shutdown();
#endif

#ifdef MVD_RECORDING
	if (sv.mvdrecording)
		SV_MVDStop (MVD_CLOSE_STOPPED, false);
#endif

	if (svs.entstatebuffer.entities)
	{
		BZ_Free(svs.entstatebuffer.entities);
		memset(&svs.entstatebuffer.entities, 0, sizeof(svs.entstatebuffer.entities));
	}
	if (sv_staticentities)
	{
		sv_max_staticentities = 0;
		sv.num_static_entities = 0;
		BZ_Free(sv_staticentities);
		sv_staticentities = NULL;
	}
	if (sv_staticsounds)
	{
		sv_max_staticsounds = 0;
		sv.num_static_sounds = 0;
		BZ_Free(sv_staticsounds);
		sv_staticsounds = NULL;
	}
	while (svs.free_lagged_packet)
	{
		laggedpacket_t *lp = svs.free_lagged_packet;
		svs.free_lagged_packet = lp->next;
		Z_Free(lp);
	}

#ifdef WEBCLIENT
	HTTP_CL_Terminate();
#endif

#ifdef HEXEN2
	T_FreeStrings();
#endif

	SV_GibFilterPurge();

	Log_ShutDown();

	NET_Shutdown ();

#ifdef PLUGINS
	Plug_Shutdown(true);
#endif
	Mod_Shutdown(true);
#ifdef PACKAGEMANAGER
	PM_Shutdown(false);
#endif
	COM_DestroyWorkerThread();
	FS_Shutdown();
#ifdef PLUGINS
	Plug_Shutdown(false);
#endif
	Cvar_Shutdown();
	Cmd_Shutdown();


	InfoBuf_Clear(&svs.info, true);
	InfoBuf_Clear(&svs.localinfo, true);

	COM_BiDi_Shutdown();
	TL_Shutdown();
	Memory_DeInit();
}

/*
================
SV_Error

Sends a datagram to all the clients informing them of the server crash,
then exits
================
*/
void VARGS SV_Error (const char *error, ...)
{
	va_list		argptr;
	static	char		string[1024];
	static	qboolean inerror = false;
	int i;

	if (inerror)
	{
		Sys_Error ("SV_Error: recursively entered (%s)", string);
	}

	inerror = true;

	va_start (argptr,error);
	vsnprintf (string,sizeof(string)-1, error,argptr);
	va_end (argptr);

	{
		extern cvar_t pr_ssqc_coreonerror;

		if (svprogfuncs && pr_ssqc_coreonerror.value && svprogfuncs->save_ents)
		{
			size_t size = 1024*1024*8;
			char *buffer = BZ_Malloc(size);
			svprogfuncs->save_ents(svprogfuncs, buffer, &size, size, 3);
			COM_WriteFile("ssqccore.txt", FS_GAMEONLY, buffer, size);
			BZ_Free(buffer);
		}
	}


	SV_EndRedirect();

	Con_Printf (CON_ERROR"SV_Error: %s\n",string);

	if (sv.state)
		SV_FinalMessage (va("server crashed: %s\n", string));

	//flag all players as unspawned, so gamecode doesn't recurse while already crashing. that sort of thing just results in more crashes.
	for (i = 0; i < sv.allocated_client_slots; i++)
		svs.clients[i].spawned = false;
	sv.spawned_client_slots = 0;
	sv.spawned_observer_slots = 0;


#ifndef SERVERONLY
	if (cls.state)
	{
		inerror = false;
		Host_EndGame("SV_Error: %s\n",string);
	}
	SCR_EndLoadingPlaque();

	if (!isDedicated)	//dedicated servers crash...
	{
		extern jmp_buf 	host_abort;
		SCR_EndLoadingPlaque();
		SV_UnspawnServer();
		CL_Disconnect(string);
		inerror=false;
		longjmp (host_abort, 1);
	}
#endif

	sys_nounload = true;
	SV_UnspawnServer();
	SV_Shutdown ();

	Sys_Error ("SV_Error: %s\n",string);
}

#ifdef SERVERONLY
void VARGS Host_Error (const char *error, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr,error);
	vsnprintf (string,sizeof(string)-1, error,argptr);
	va_end (argptr);

	SV_Error("%s", string);
}
#endif

#ifdef SERVERONLY
void VARGS Host_EndGame (const char *error, ...)
{
	va_list		argptr;
	char		string[1024];

	va_start (argptr,error);
	vsnprintf (string,sizeof(string)-1, error,argptr);
	va_end (argptr);

	SV_Error("%s", string);
}
#endif

/*
==================
SV_FinalMessage

Used by SV_Error and SV_Quit_f to send a final message to all connected
clients before the server goes down.  The messages are sent immediately,
not just stuck on the outgoing message list, because the server is going
to totally exit after returning from this function.
==================
*/
void SV_FinalMessage (char *message)
{
	int			i;
	client_t	*cl;
	sizebuf_t	buf;
	char		bufdata[1024];

	memset(&buf, 0, sizeof(buf));
	buf.data = bufdata;
	buf.maxsize = sizeof(bufdata);

	for (i=0, cl = svs.clients ; i<svs.allocated_client_slots ; i++, cl++)
		if (cl->state >= cs_spawned && !cl->controlled)
		{
			if (ISQWCLIENT(cl))
			{
				SZ_Clear (&buf);
				MSG_WriteByte (&buf, svc_print);
				MSG_WriteByte (&buf, PRINT_HIGH);
				MSG_WriteString (&buf, message);
				MSG_WriteByte (&buf, svc_disconnect);
			}
#ifdef NQPROT
			else if (ISNQCLIENT(cl))
			{
				SZ_Clear (&buf);
				MSG_WriteByte (&buf, svc_print);
				MSG_WriteString (&buf, message);
				MSG_WriteByte (&buf, svc_disconnect);
			}
#endif
			else
				continue;

			Netchan_Transmit (&cl->netchan, buf.cursize
					, buf.data, 10000);
		}
}



/*
=====================
SV_DropClient

Called when the player is totally leaving the server, either willingly
or unwillingly.  This is NOT called if the entire server is quiting
or crashing.
=====================
*/
void SV_DropClient (client_t *drop)
{
	unsigned int i;
	laggedpacket_t *lp;
	sizebuf_t termmsg;
	char termbuf[64];

	/*drop the first in the chain first*/
	if (drop->controller && drop->controller != drop)
	{
		SV_DropClient(drop->controller);
		return;
	}

	if (!drop->controller && drop->netchan.remote_address.type != NA_INVALID && drop->netchan.remote_address.type != NA_LOOPBACK)
	{
		// add the disconnect
		if (drop->state < cs_connected)
		{
			switch (drop->protocol)
			{
			case SCP_QUAKE2:
			case SCP_QUAKE2EX:
				MSG_WriteByte (&drop->netchan.message, svcq2_disconnect);
				break;
			case SCP_QUAKEWORLD:
			case SCP_NETQUAKE:
			case SCP_BJP3:
			case SCP_FITZ666:
			case SCP_DARKPLACES6:
			case SCP_DARKPLACES7:
				MSG_WriteByte (&drop->netchan.message, svc_disconnect);
				break;
			case SCP_BAD:
				break;
			case SCP_QUAKE3:
				break;
			}
		}
	}

	if (drop->state == cs_spawned)
	{
#ifdef SVRANKING
		if (drop->rankid)
		{
			int j;
			rankstats_t rs;
			if (Rank_GetPlayerStats(drop->rankid, &rs))
			{
				rs.timeonserver += realtime - drop->stats_started;
				drop->stats_started = realtime;
				rs.kills += drop->kills;
				rs.deaths += drop->deaths;

				rs.flags1 &= ~(RANK_CUFFED|RANK_MUTED|RANK_CRIPPLED);
//				if (drop->iscuffed==2)
//					rs.flags1 |= RANK_CUFFED;
//				if (drop->ismuted==2)
//					rs.flags1 |= RANK_MUTED;
//				if (drop->iscrippled==2)
//					rs.flags1 |= RANK_CRIPPLED;
				drop->kills=0;
				drop->deaths=0;
				pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, drop->edict);
				if (pr_global_ptrs->SetChangeParms)
					PR_ExecuteProgram (svprogfuncs, pr_global_struct->SetChangeParms);
				for (j=0 ; j<NUM_RANK_SPAWN_PARMS ; j++)
					if (pr_global_ptrs->spawnparamglobals[j])
						rs.parm[j] = *pr_global_ptrs->spawnparamglobals[j];
#if NUM_RANK_SPAWN_PARMS>32
				rs.lastseen = time(NULL);
#endif
				Rank_SetPlayerStats(drop->rankid, &rs);
			}
		}
#endif
	}
#ifdef SUBSERVERS
	SSV_SavePlayerStats(drop, (drop->redirect==2)?1:2);
#endif
#ifdef SVCHAT
	SV_WipeChat(drop);
#endif

	if (sv.world.worldmodel->loadstate != MLS_LOADED)
		Con_Printf(CON_WARNING "Warning: not notifying gamecode about client disconnection due to invalid worldmodel\n");
	else
	switch(svs.gametype)
	{
	case GT_MAX:
		break;
#ifdef VM_LUA
	case GT_LUA:
#endif
	case GT_Q1QVM:
	case GT_PROGS:
		if (svprogfuncs)
		{
			SV_DespawnClient(drop);
			drop->edict = NULL;

			if (drop->spawninfo)
				Z_Free(drop->spawninfo);
			drop->spawninfo = NULL;
		}
		break;
	case GT_QUAKE2:
#ifdef Q2SERVER
		if (ge)
			ge->ClientDisconnect(drop->q2edict);
#endif
		break;
	case GT_QUAKE3:
#ifdef Q3SERVER
		q3->sv.DropClient(drop);
#endif
		break;
	case GT_HALFLIFE:
#ifdef HLSERVER
		SVHL_DropClient(drop);
#endif
		break;
	}

	SV_Prompt_Clear(drop);
	if (drop->centerprintstring)
		Z_Free(drop->centerprintstring);
	drop->centerprintstring = NULL;

	if (!drop->redirect && drop->state > cs_zombie)
	{
		if (drop->spectator)
			Con_TPrintf ("Spectator \"%s\" removed\n",drop->name);
		else
			Con_TPrintf ("Client \"%s\" removed\n",drop->name);
	}

#if defined(HAVE_LEGACY) && defined(MVD_RECORDING)
	SV_DownloadQueueClear(drop);
#endif
	if (drop->download)
	{
		VFS_CLOSE (drop->download);
		drop->download = NULL;
	}
	if (drop->upload)
	{
		VFS_CLOSE (drop->upload);
		drop->upload = NULL;
	}
	*drop->uploadfn = 0;

#ifdef HAVE_CLIENT
	if (drop->netchan.remote_address.type == NA_LOOPBACK)
	{
		if (drop->protocol != SCP_BAD)
			Netchan_Transmit(&drop->netchan, 0, "", SV_RateForClient(drop));
#ifdef warningmsg
#pragma warningmsg("This means that we may not see the reason we kicked ourselves.")
#endif
		drop->state = cs_free;	//don't do zombie stuff
		cls.state = ca_disconnected;
		CL_BeginServerReconnect();
	}
	else
#endif
	if (drop->protocol == SCP_BAD)
		drop->state = cs_free;	//skip zombie state for bots.
	else if (drop->state == cs_spawned || drop->istobeloaded)
	{
		drop->state = cs_zombie;		// become free in a few seconds
		drop->connection_started = realtime;	// for zombie timeout
	}
	else
		drop->state = cs_free;	//skip zombie state if qc couldn't access it anyway.
	drop->istobeloaded = false;

	drop->old_frags = 0;
#ifdef SVRANKING
	drop->kills = 0;
	drop->deaths = 0;
#endif
	drop->namebuf[0] = 0;
	drop->name = drop->namebuf;
	InfoBuf_Clear(&drop->userinfo, true);
	InfoSync_Clear(&drop->infosync);

	while ((lp = drop->laggedpacket))
	{
		drop->laggedpacket = lp->next;
		lp->next = svs.free_lagged_packet;
		svs.free_lagged_packet = lp;
	}
	drop->laggedpacket_last = NULL;

	drop->pendingdeltabits = NULL;
	drop->pendingcsqcbits = NULL;
	if (drop->frameunion.frames)	//union of the same sort of structure
	{
		Z_Free(drop->frameunion.frames);
		drop->frameunion.frames = NULL;
	}
	if (drop->sentents.entities)
	{
		Z_Free(drop->sentents.entities);
		memset(&drop->sentents.entities, 0, sizeof(drop->sentents.entities));
	}

	for (i = 0; i < MAX_CL_STATS; i++)
	{
		Z_Free(drop->statss[i]);
		drop->statss[i] = NULL;
	}

	drop->csqcactive = false;

	memset(&termmsg, 0, sizeof(termmsg));
	termmsg.data = termbuf;
	termmsg.maxsize = sizeof(termbuf);
	termmsg.cursize = 0;
	if (drop->netchan.remote_address.type == NA_LOOPBACK)
	{
	}
	else if (ISQWCLIENT(drop) || ISNQCLIENT(drop))
	{
		MSG_WriteByte(&termmsg, svc_disconnect);
	}
	else if (ISQ2CLIENT(drop))
	{
		MSG_WriteByte(&termmsg, svcq2_disconnect);
	}
	else if (ISQ3CLIENT(drop))
	{
	}
	if (drop->netchan.remote_address.type != NA_INVALID && drop->netchan.message.maxsize)
	{
#ifdef NQPROT
		if (drop->netchan.isnqprotocol && drop->netchan.nqunreliableonly != 1)
		{	//try and flush the reliables, so they can see why they were kicked.
			drop->netchan.nqunreliableonly = 3;	//may cause problems if they were loading content. not much we can do about that.
			while(Netchan_Transmit (&drop->netchan, 0, NULL, 10000))
				;
		}
#endif
		//send twice, to cover packetloss a little.
		Netchan_Transmit (&drop->netchan, termmsg.cursize, termmsg.data, 10000);
		Netchan_Transmit (&drop->netchan, termmsg.cursize, termmsg.data, 10000);

#ifdef HAVE_DTLS
		NET_DTLS_Disconnect(svs.sockets, &drop->netchan.remote_address);
#endif
	}

	if (svs.gametype == GT_PROGS || svs.gametype == GT_Q1QVM)	//gamecode should do it all for us.
	{
// send notification to all remaining clients
		SV_FullClientUpdate (drop, NULL);
#ifdef MVD_RECORDING
		SV_MVD_FullClientUpdate(NULL, drop);
#endif
	}

	if (drop->controlled)
	{
		drop->controlled->controller = NULL;
		drop->controlled->protocol = SCP_BAD;	//with the controller dead, make sure we don't try sending anything to it
		SV_DropClient(drop->controlled);
		drop->controlled = NULL;
	}
}

//called when someone's connection goes away.
void SV_DropClient_ByAddress (netadr_t *addr)
{	//just flag em, its easier on stack...
	int i;
	for (i = 0; i < svs.allocated_client_slots; i++)
	{
		if (!svs.clients[i].drop && svs.clients[i].state >= cs_connected)
			if (NET_CompareAdr(&svs.clients[i].netchan.remote_address, addr))
			{
				SV_BroadcastTPrintf (PRINT_HIGH, "%s lost connection\n", svs.clients[i].name);
				svs.clients[i].drop = true;
			}
	}
}


//====================================================================

typedef struct pinnedmessages_s {
	struct pinnedmessages_s *next;
	char setby[64];
	char message[1024];
} pinnedmessages_t;
static pinnedmessages_t *pinned;
static qboolean dopinnedload = true;
void PIN_DeleteOldestMessage(void);
void PIN_MakeMessage(char *from, char *msg);

void PIN_LoadMessages(void)
{
	char setby[64];
	char message[1024];

	int i;
	char *file, *end;
	char *lstart;
	int len;

	dopinnedload = false;

	while(pinned)
		PIN_DeleteOldestMessage();

	len = FS_LoadFile("pinned.txt", (void**)&file);
	if (!file)
		return;
	end = file+len;

	lstart = file;
	for(;;)
	{
		while (lstart<end && *lstart <= ' ')
			lstart++;

		for (i = 0; lstart<end && i < sizeof(message)-1; i++)
		{
			if (*lstart == '\n' || *lstart == '\r')
				break;
			message[i] = *lstart++;
		}
		message[i] = '\0';

		while (lstart<end && *lstart <= ' ')
			lstart++;

		for (i = 0; lstart<end && i < sizeof(setby)-1; i++)
		{
			if (*lstart == '\n' || *lstart == '\r')
				break;
			setby[i] = *lstart++;
		}
		setby[i] = '\0';

		if (!*setby)
			break;

		PIN_MakeMessage(setby, message);
	}

	FS_FreeFile(file);
}
void PIN_SaveMessages(void)
{
	pinnedmessages_t *p;
	vfsfile_t *f;

	f = FS_OpenVFS("pinned.txt", "wb", FS_GAMEONLY);
	if (!f)
	{
		Con_TPrintf(CON_ERROR "couldn't write to %s\n", "pinned.txt");
		return;
	}

	for (p = pinned; p; p = p->next)
		VFS_PRINTF(f, "%s\r\n\t%s\r\n\n", p->message, p->setby);

	VFS_CLOSE(f);
}
void PIN_DeleteOldestMessage(void)
{
	pinnedmessages_t *old;
	if (dopinnedload)
		PIN_LoadMessages();

	old = pinned;
	if (old)
	{
		pinned = pinned->next;
		Z_Free(old);
	}
}
void PIN_MakeMessage(char *from, char *msg)
{
	pinnedmessages_t *p;
	pinnedmessages_t *newp;

	if (dopinnedload)
		PIN_LoadMessages();

	newp = BZ_Malloc(sizeof(pinnedmessages_t));
	Q_strncpyz(newp->setby, from, sizeof(newp->setby));
	Q_strncpyz(newp->message, msg, sizeof(newp->message));
	newp->next = NULL;

	if (!pinned)
		pinned = newp;
	else
	{
		for (p = pinned; ; p = p->next)
		{
			if (!p->next)
			{
				p->next = newp;
				break;
			}
		}
	}
}
void PIN_ShowMessages(client_t *cl)
{
	pinnedmessages_t *p;
	if (dopinnedload)
		PIN_LoadMessages();

	if (!pinned)
		return;

	SV_ClientPrintf(cl, PRINT_HIGH, "\n\n\n");
	for (p = pinned; p; p = p->next)
	{
		SV_ClientPrintf(cl, PRINT_HIGH, "%s\n\n        %s\n", p->message, p->setby);
		SV_ClientPrintf(cl, PRINT_HIGH, "\n\n\n");
	}

}

//====================================================================

/*
===================
SV_CalcPing

===================
*/
int SV_CalcPing (client_t *cl, qboolean forcecalc)
{
	float		ping;
	int			i;
	int			count;

	if (cl->controller)
		cl = cl->controller;

	if (!cl->frameunion.frames)
		return 0;

	safeswitch (cl->protocol)
	{
	case SCP_QUAKE2:
	case SCP_QUAKE2EX:
#ifdef Q2SERVER
		{
			q2client_frame_t *frame;
			ping = 0;
			count = 0;
			for (frame = cl->frameunion.q2frames, i=0 ; i<Q2UPDATE_BACKUP ; i++, frame++)
			{
				if (frame->ping_time > 0)
				{
					ping += frame->ping_time;
					count++;
				}
			}
			if (!count)
				return 9999;
			ping /= count;

		}
		return ping;
#endif
	case SCP_QUAKE3:
		break;
	case SCP_DARKPLACES6:
	case SCP_DARKPLACES7:
	case SCP_NETQUAKE:
	case SCP_BJP3:
	case SCP_FITZ666:
	case SCP_QUAKEWORLD:
		{
			register	client_frame_t *frame;
			ping = 0;
			count = 0;
			for (frame = cl->frameunion.frames, i=0 ; i<UPDATE_BACKUP ; i++, frame++)
			{
				if (frame->ping_time > 0)
				{
					ping += frame->ping_time;
					count++;
				}
			}
			if (!count)
				return 9999;
			ping /= count;
		}
		return ping*1000;
	case SCP_BAD:
	safedefault:
		break;
	}
	return 0;
}

/*
===================
SV_FullClientUpdate

Writes all update values to client. use to=NULL to broadcast.
===================
*/
void SV_FullClientUpdate (client_t *client, client_t *to)
{
	int		i;
	char	info[EXTENDED_INFO_STRING];
	sizebuf_t *buf;

	if (!to)
	{
		for (i = 0; i < sv.allocated_client_slots; i++)
		{
			SV_FullClientUpdate(client, &svs.clients[i]);
		}
#ifdef MVD_RECORDING
		if (sv.mvdrecording)
			SV_FullClientUpdate(client, &demo.recorder);
#endif
		return;
	}

	if (to->controller && to->controller != to)
		return;

	i = client - svs.clients;

	if (i >= to->max_net_clients)
		return;	//most clients will crash if they see too high a player index. some even segfault.

//Sys_Printf("SV_FullClientUpdate:  Updated frags for client %d\n", i);

	if (ISQWCLIENT(to))
	{
		float onservertime;
		unsigned int pext = to->fteprotocolextensions;
		int ping = SV_CalcPing (client, false);
		if (ping > 0xffff)
			ping = 0xffff;

		buf = ClientReliable_StartWrite(to, 4);
			MSG_WriteByte(buf, svc_updatefrags);
			MSG_WriteByte(buf, i);
			MSG_WriteShort(buf, client->old_frags);
		ClientReliable_FinishWrite(to);

		buf = ClientReliable_StartWrite(to, 4);
			MSG_WriteByte(buf, svc_updateping);
			MSG_WriteByte(buf, i);
			MSG_WriteShort(buf, ping);
		ClientReliable_FinishWrite(to);

		buf = ClientReliable_StartWrite(to, 3);
			MSG_WriteByte(buf, svc_updatepl);
			MSG_WriteByte(buf, i);
			MSG_WriteByte(buf, client->lossage);
		ClientReliable_FinishWrite(to);

		onservertime = realtime - client->connection_started;
		if (onservertime > sv.time)
			onservertime = sv.time;
		buf = ClientReliable_StartWrite(to, 6);
			MSG_WriteByte(buf, svc_updateentertime);
			MSG_WriteByte(buf, i);
			MSG_WriteFloat(buf, onservertime);
		ClientReliable_FinishWrite(to);

		InfoBuf_ToString(&client->userinfo, info, (pext&PEXT_BIGUSERINFOS)?BASIC_INFO_STRING:sizeof(info), basicuserinfos, privateuserinfos, (pext&PEXT_BIGUSERINFOS)?NULL:basicuserinfos, &to->infosync, &client->userinfo);
		buf = ClientReliable_StartWrite(to, 7 + strlen(info));
			MSG_WriteByte(buf, svc_updateuserinfo);
			MSG_WriteByte(buf, i);
			MSG_WriteLong(buf, client->userid);
			MSG_WriteString(buf, info);
		ClientReliable_FinishWrite(to);
	}
	else if (ISNQCLIENT(to))
	{
		int top, bottom, playercolor;
		char *nam = InfoBuf_ValueForKey(&client->userinfo, "name");

		buf = ClientReliable_StartWrite(to, 4);
			MSG_WriteByte(buf, svc_updatefrags);
			MSG_WriteByte(buf, i);
			MSG_WriteShort(buf, client->old_frags);
		ClientReliable_FinishWrite(to);

		buf = ClientReliable_StartWrite(to, 3 + strlen(nam));
			MSG_WriteByte(buf, svc_updatename);
			MSG_WriteByte(buf, i);
			MSG_WriteString(buf, nam);
		ClientReliable_FinishWrite(to);


		top = atoi(InfoBuf_ValueForKey(&client->userinfo, "topcolor"));
		bottom = atoi(InfoBuf_ValueForKey(&client->userinfo, "bottomcolor"));
		top &= 15;
		if (top > 13)
			top = 13;
		bottom &= 15;
		if (bottom > 13)
			bottom = 13;
		playercolor = top*16 + bottom;

		buf = ClientReliable_StartWrite(to, 3);
			MSG_WriteByte(buf, svc_updatecolors);
			MSG_WriteByte(buf, i);
			MSG_WriteByte(buf, playercolor);
		ClientReliable_FinishWrite(to);

		if (to->qex)
		{
			unsigned int s1, s2;
			if (client->netchan.remote_address.type == NA_LOOPBACK)
				s1 = s2 = 0;	//host
			else
				s1 = s2 = -1;	//non-playfab connection
			MSG_WriteByte(buf, svcqex_updatesocial);
			MSG_WriteByte(buf, i);
			MSG_WriteLong(buf, s1);
			MSG_WriteLong(buf, s2);
		}

		if (to->fteprotocolextensions2 & PEXT2_PREDINFO)
		{
			char *s;
			InfoBuf_ToString(&client->userinfo, info, sizeof(info), basicuserinfos, privateuserinfos, NULL, NULL, NULL);
			s = va("//fui %i \"%s\"\n", i, info);
			buf = ClientReliable_StartWrite(to, 2 + strlen(s));
				ClientReliableWrite_Begin(to, svc_stufftext, 2+strlen(s));
				ClientReliableWrite_String(to, s);
			ClientReliable_FinishWrite(to);
		}
	}
}

/*
==============================================================================

CONNECTIONLESS COMMANDS

==============================================================================
*/

const char *SV_ProtocolNameForClient(client_t *cl)
{
	//okay, that failed...
	safeswitch (cl->protocol)
	{
	case SCP_QUAKEWORLD:
		if (cl->fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS)
			return "fteqw";	//changes enough to be significant. assumed to include csqc.
		return "qw";
	case SCP_BAD:
		return "bot";
	case SCP_QUAKE2:
		return "q2";
	case SCP_QUAKE2EX:
		return "q2ex";
	case SCP_QUAKE3:
		return "q3";
	case SCP_NETQUAKE:
		if (cl->fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS)
			return "ftenq";	//changes enough to be significant. assumed to include csqc.
		if (cl->qex)
			return "qex";
		if (cl->proquake_angles_hack)
			return "pq";
		return "nq";
	case SCP_BJP3:
		return "bjp3";
	case SCP_FITZ666:
		//this gets messy... probably we should distinguish more
		if (cl->fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS)
			return "ftenq";	//changes enough to be significant. assumed to include csqc.
		if (cl->qex)
			return "qex";
		if (cl->netchan.netprim.coordtype != COORDTYPE_FIXED_13_3 || cl->netchan.netprim.anglesize != 1)
			return "rmq";	//while fte tends not to care, most people consider them separate.
		return "fitz";
	case SCP_DARKPLACES6:
		return "dp6";
	case SCP_DARKPLACES7:
		return "dp7";
	safedefault:
		return "unk";
	}
}

char *SV_PlayerPublicAddress(client_t *cl)
{	//returns a string containing the client's IP address, as permitted for viewing by other clients.
	//if something useful is actually returned, it should be masked.
	//we hide it entirely out of private info caution. most nq clients expect a #.#.#.INVALID type address.
	//it should be fine to put other stuff here though, we put client version instead, if we know it.
	const char *ver = InfoBuf_ValueForKey(&cl->userinfo, "*ver");
	const char *prot = SV_ProtocolNameForClient(cl);

	return va("prot %s %s", prot, ver);	//something so they can't confuse ip parsing so easily nor pass them off as some other protocol.
}

/*
================
SVC_Status

Responds with all the info that qplug or qspy can see
This message can be up to around 5k with worst case string lengths.
================
*/
static void SVC_Status (void)
{
	int displayflags;
	int		i;
	client_t	*cl;
	char *name;
	int		ping;
	int		top, bottom;
	char frags[64];
	char *skin, *team, *botpre, *specpre;
	char junk[512];
	int jlen;

	int slots=0;

	displayflags = atoi(Cmd_Argv(1));
	if (displayflags == STATUS_OLDSTYLE)
		displayflags = STATUS_SERVERINFO|STATUS_PLAYERS;

	Cmd_TokenizeString ("status", false, false);
	SV_BeginRedirect (RD_PACKET, TL_FindLanguage(""));
	if (displayflags&STATUS_SERVERINFO)
	{
		char infostr[1024];	//FIXME: vanilla limit is 512. we should probably have a list of known cvars for lower priority sending.
		const char *ignorekeys[] = {"mapname", "*z_ext", NULL};	//ignore some pointless stuff
		const char *prioritykeys[] = {"hostname", "admin", "*gamedir", "*version", "deathmatch", "timelimit", "fraglimit", "maxclients", "maxspectators", "status", NULL}; //make sure we include these before we start overflowing
		InfoBuf_ToString(&svs.info, infostr, sizeof(infostr), prioritykeys, ignorekeys, NULL, NULL, NULL);
		Con_Printf ("%s\n", infostr);
	}
	for (i=0 ; i<svs.allocated_client_slots ; i++)
	{
		cl = &svs.clients[i];
		if ((cl->state == cs_connected || cl->state == cs_spawned || cl->name[0]) && ((cl->spectator && displayflags&STATUS_SPECTATORS) || (!cl->spectator && displayflags&STATUS_PLAYERS)))
		{
			top = atoi(InfoBuf_ValueForKey (&cl->userinfo, "topcolor"));
			bottom = atoi(InfoBuf_ValueForKey (&cl->userinfo, "bottomcolor"));
			top = (top < 0) ? 0 : ((top > 13) ? 13 : top);
			bottom = (bottom < 0) ? 0 : ((bottom > 13) ? 13 : bottom);
			ping = SV_CalcPing (cl, false);
			name = cl->name;

			skin = InfoBuf_ValueForKey (&cl->userinfo, "skin");
			team = InfoBuf_ValueForKey (&cl->userinfo, "team");

			if (!cl->state || cl->protocol == SCP_BAD)	//show bots differently. Just to be courteous.
				botpre = "BOT:";
			else
				botpre = "";

			specpre = "";
			if (cl->spectator)
			{	//silly mvdsv stuff
				if (displayflags & STATUS_SPECTATORS_AS_PLAYERS)
				{
					frags[0] = 'S';
					frags[1] = '\0';
				}
				else
				{
					ping = -ping;
					sprintf(frags, "%i", -9999);
					specpre = "\\s\\";
				}
			}
			else
				sprintf(frags, "%i", cl->old_frags);

			junk[jlen = 0] = 0;
			if ((displayflags & STATUS_SHOWTEAMS) && jlen+4<sizeof(junk))
			{
				junk[jlen++] = ' ';
				jlen += strlen(COM_QuotedString(team, junk+jlen, sizeof(junk)-jlen, false));
			}
			if ((displayflags & STATUS_LOGININFO) && jlen+4<sizeof(junk))
			{
				junk[jlen++] = ' ';
				jlen += strlen(COM_QuotedString("", junk+jlen, sizeof(junk)-jlen, false));
			}

			Con_Printf ("%i %s %i %i \"%s%s%s\" \"%s\" %i %i%s\n", cl->userid,
					frags, (int)(realtime - cl->connection_started)/60,
					ping, specpre, botpre, name, skin, top, bottom, junk);
		}
		else
			slots++;
	}
#ifdef MVD_RECORDING
	if (displayflags & STATUS_QTVLIST)
	{
		struct mvddest_s *d;
		for (d = demo.dest; d; d = d->nextdest)
		{
			if (d->desttype == DEST_STREAM)
				Con_Printf("qtv %d \"%s\" \"%s\" %d\n", d->id, d->simplename, d->filename, 0/*d->viewercount*/);
		}
	}
#endif

	SV_EndRedirect ();
}

#if 1//def NQPROT
const char *SV_GetProtocolVersionString(void)
{
	char *ret = va("%i", com_protocolversion.ival);	//for compat with DP, this is basically locked at 3. our pexts allow this to be mostly graceful.

	switch(svs.gametype)
	{
	case GT_PROGS:
	case GT_Q1QVM:
		if (sv_listen_qw.ival)
			Q_strncatz(ret, "w", 64);
#ifdef NQPROT
		if (progstype == PROG_H2)
			break;	//don't advertise nq protocols when they're blocked.
		if (sv_listen_nq.ival)
		{
			Q_strncatz(ret, "n", 64);
#ifdef HAVE_DTLS
			if (*dtls_psk_user.string)
				Q_strncatz(ret, "x", 64);
#endif
		}
		if (sv_listen_dp.ival)
			Q_strncatz(ret, "d", 64);
#endif
		break;
	default:
		break;	//these do their own thing, with their own protocols. don't be weird.
	}
	return ret;
}

void SV_GeneratePublicServerinfo(char *info, const char *endinfo)
{
	char *resp = info;
	const char *ignorekeys[] = {
		"maxclients", "map", "*gamedir", "*z_ext",	//this is a DP protocol query, so some QW fields are not needed
		"gamename", "modname", "protocol", "clients", "sv_maxclients", "mapname", "qcstatus", "challenge", NULL};	//and we need to add some
	const char *prioritykeys[] = {"hostname", NULL}; //make sure we include these before we start overflowing
	char protocolname[64];
	const char *gamestatus;

	extern cvar_t maxclients;
	int i;
	client_t *cl;
	int numclients = 0;
	for (i=0 ; i<svs.allocated_client_slots ; i++)
	{
		cl = &svs.clients[i];
		if ((cl->state == cs_connected || cl->state == cs_spawned || cl->name[0]) && !cl->spectator)
			numclients++;
	}

	//first line contains the serverinfo, or some form of it
	COM_ParseOut(com_protocolname.string, protocolname, sizeof(protocolname));	//we can only report one, so report the first.
	if (svprogfuncs)
	{
		eval_t *v = PR_FindGlobal(svprogfuncs, "worldstatus", PR_ANY, NULL);
		if (v)
			gamestatus = PR_GetString(svprogfuncs, v->string);
		else
			gamestatus = "";
	}
	else
		gamestatus = "";

	*resp = 0;
	Info_SetValueForKey(resp, "gamename", protocolname, endinfo - resp);//distinguishes it from other types of games
	Info_SetValueForKey(resp, "protocol", SV_GetProtocolVersionString(), endinfo - resp);
	Info_SetValueForKey(resp, "modname", FS_GetGamedir(true), endinfo - resp);
	Info_SetValueForKey(resp, "clients", va("%d", numclients), endinfo - resp);
	Info_SetValueForKey(resp, "sv_maxclients", maxclients.string, endinfo - resp);
	Info_SetValueForKey(resp, "mapname", InfoBuf_ValueForKey(&svs.info, "map"), endinfo - resp);
	resp += strlen(resp);
	//now include the full/regular serverinfo
	resp += InfoBuf_ToString(&svs.info, resp, endinfo - resp, prioritykeys, ignorekeys, NULL, NULL, NULL);
	*resp = 0;
	//and any possibly-long qc status string
	if (*gamestatus)
		Info_SetValueForKey(resp, "qcstatus", gamestatus, endinfo - resp);
	resp += strlen(resp);
	*resp++ = 0;
}

static void SVC_GetInfo (const char *challenge, int fullstatus)
{
	//dpmaster support
	char response[MAX_UDP_PACKET];
	int i;
	char *resp;

	resp = response;

	//response packet header
	*resp++ = 0xff;
	*resp++ = 0xff;
	*resp++ = 0xff;
	*resp++ = 0xff;
	if (fullstatus)
		Q_strncpyz(resp, "statusResponse", sizeof(response) - (resp-response) - 1);
	else
		Q_strncpyz(resp, "infoResponse", sizeof(response) - (resp-response) - 1);
	resp += strlen(resp);
	*resp++ = '\n';

	SV_GeneratePublicServerinfo(resp, response+sizeof(response));

	if (fullstatus)
	{
		client_t *cl;
		char *start = resp;

		if (resp != response+sizeof(response))
		{
			resp[-1] = '\n';	//replace the null terminator that we already wrote

			//on the following lines we have an entry for each client
			for (i=0 ; i<svs.allocated_client_slots ; i++)
			{
				cl = &svs.clients[i];
				if ((cl->state == cs_connected || cl->state == cs_spawned || cl->name[0]) && !cl->spectator)
				{
					Q_strncpyz(resp, va(
									"%d %d \"%s\" \"%s\"\n"
									,
									cl->old_frags,
									SV_CalcPing(cl, false),
									cl->team,
									cl->name
									), sizeof(response) - (resp-response));
					resp += strlen(resp);
				}
			}

			*resp++ = 0;	//this might not be a null
			if (resp == response+sizeof(response))
			{
				//we're at the end of the buffer, it's full. bummer
				//replace 12 bytes with infoResponse
				memcpy(response+4, "infoResponse", 12);
				//move down by len(statusResponse)-len(infoResponse) bytes
				memmove(response+4+12, response+4+14, resp-response-(4+14));
				start -= 14-12; //fix this pointer

				resp = start;
				resp[-1] = 0;	//reset the \n
			}
		}
	}

	NET_SendPacket (svs.sockets, resp-response, response, &net_from);
}
#endif

#ifdef Q2SERVER
static void SVC_InfoQ2 (void)
{
	char	string[128];
	int		i, count;
	int		version;

	if (maxclients.value == 1)
		return;		// ignore in single player

	version = atoi (Cmd_Argv(1));

	if (version != PROTOCOL_VERSION_Q2)
		snprintf (string, sizeof(string), "%s: wrong version\n", hostname.string);
	else
	{
		count = 0;
		for (i=0 ; i<svs.allocated_client_slots ; i++)
			if (svs.clients[i].state >= cs_connected)
				count++;

		snprintf (string, sizeof(string), "%16s %8s %2i/%2i\n", hostname.string, svs.name, count, (int)maxclients.value);
	}

	Netchan_OutOfBandPrint (NCF_SERVER, &net_from, "info\n%s", string);
}
#endif

#ifdef MVD_RECORDING
static void SVC_QTVUsers (void)
{
}
#endif

/*
===================
SV_CheckLog

===================
*/
#define	LOG_FLUSH		10*60
static void SV_CheckLog (void)
{
	sizebuf_t	*sz;

	sz = &svs.log[svs.logsequence&(FRAGLOG_BUFFERS-1)];

	// bump sequence ten minutes have passed and
	// there is something still sitting there
	//logfrag does the rotation for a full log.
	if (realtime - svs.logtime > LOG_FLUSH && sz->cursize)
	{
		// swap buffers and bump sequence
		svs.logtime = realtime;
		svs.logsequence++;
		sz = &svs.log[svs.logsequence&(FRAGLOG_BUFFERS-1)];
		sz->cursize = 0;
		Con_TPrintf ("beginning fraglog sequence %i\n", svs.logsequence);
	}

}

/*
================
SVC_Log

Responds with all the logged frags for ranking programs.
If a sequence number is passed as a parameter and it is
the same as the current sequence, an A2A_NACK will be returned
instead of the data.
================
*/
static void SVC_Log (void)
{
	unsigned int	seq;
	char	data[MAX_DATAGRAM+64];
	char	adr[MAX_ADR_SIZE];
	char *av;

	av = Cmd_Argv(1);
	if (*av)
	{
		seq = strtoul(av, NULL, 0);
		//seq is the last one that the client already has

		if (seq < svs.logsequence-(FRAGLOG_BUFFERS-1))
			seq = svs.logsequence-(FRAGLOG_BUFFERS-1);	//send them this sequence
		else if (seq == svs.logsequence)
		{	//current log isn't available as its not complete yet.
			data[0] = A2A_NACK;
			NET_SendPacket (svs.sockets, 1, data, &net_from);
			return;
		}
		else if (seq > svs.logsequence)	//future logs are not valid either. reply with the last that was. this is for compat, just in case.
			seq = svs.logsequence-1;
		else
			seq = seq+1;	//they will get the next sequence from the one they already have
	}
	else
		seq = svs.logsequence-1;

	if (!fraglog_public.ival)
	{	//frag logs are not public (for DoS protection perhaps?)
		data[0] = A2A_NACK;
		NET_SendPacket (svs.sockets, 1, data, &net_from);
		return;
	}

	Con_DPrintf ("sending log %i to %s\n", seq, NET_AdrToString(adr, sizeof(adr), &net_from));

	//cookie support, to avoid spoofing
	av = Cmd_Argv(2);
	if (*av)
		Q_snprintfz(data, sizeof(data), "stdlog %i %s\n%s", seq, av, (char *)svs.log_buf[seq&(FRAGLOG_BUFFERS-1)]);
	else
		Q_snprintfz(data, sizeof(data), "stdlog %i\n%s", seq, (char *)svs.log_buf[seq&(FRAGLOG_BUFFERS-1)]);
	NET_SendPacket (svs.sockets, strlen(data)+1, data, &net_from);
}

/*
================
SVC_Ping

Just responds with an acknowledgement
================
*/
static void SVC_Ping (void)
{
	char	data;

	data = A2A_ACK;

	NET_SendPacket (svs.sockets, 1, &data, &net_from);
}

//from net_from
int SV_NewChallenge (void)
{
	int		i;
	int		oldest;
	int		oldestTime;

	oldest = 0;
	oldestTime = 0x7fffffff;

	// see if we already have a challenge for this ip
	for (i = 0 ; i < MAX_CHALLENGES ; i++)
	{
		if (NET_CompareBaseAdr (&net_from, &svs.challenges[i].adr))
		{
			svs.challenges[i].time = realtime;
			return svs.challenges[i].challenge;
		}
		if (svs.challenges[i].time < oldestTime)
		{
			oldestTime = svs.challenges[i].time;
			oldest = i;
		}
	}

	// overwrite the oldest
	svs.challenges[oldest].challenge = (rand() << 16) ^ rand();
	svs.challenges[oldest].adr = net_from;
	svs.challenges[oldest].time = realtime;

	return svs.challenges[oldest].challenge;
}

/*
=================
SVC_GetChallenge

Returns a challenge number that can be used
in a subsequent client_connect command.
We do this to prevent denial of service attacks that
flood the server with invalid connection IPs.  With a
challenge, they must give a valid IP address.
=================
*/
qboolean SVC_GetChallenge (qboolean respond_dp)
{
#ifdef HUFFNETWORK
	int compressioncrc;
#endif
	int		challenge;
	char *buf;
	int lng;
	char *over;

	qboolean respond_std = true;
#ifdef QWOVERQ3
	qboolean respond_qwoverq3 = true;
	respond_qwoverq3 &= !!sv_listen_q3.value;
#else
	const qboolean respond_qwoverq3 = false;
#endif

	int ioq3clchallenge = atoi(Cmd_Argv(1));
	const char *protocols = Cmd_Argv(2);
	if (*protocols)
	{
		const char *pname;
		char tprot[64], oprot[64];
		while ((protocols=COM_ParseOut(protocols, tprot,sizeof(tprot))))
		{
			pname = com_protocolname.string;
			while ((pname=COM_ParseOut(pname, oprot,sizeof(oprot))))
			{
				if (!strcmp(tprot, oprot))
					break;
			}
			if (pname)
				break;
		}

		if (!protocols)
		{
			COM_ParseOut(Cmd_Argv(2), tprot,sizeof(tprot));
			COM_ParseOut(com_protocolname.string, oprot,sizeof(oprot));
			pname = va("print\nGame mismatch: This is a %s server but you are using %s\n", oprot, tprot);
			Netchan_OutOfBand(NCF_SERVER, &net_from, strlen(pname), pname);
			return false;
		}
	}

	if (sv_listen_qw.value && !sv_listen_dp.value)
	{
		respond_std = true;
		respond_dp = false;
	}
	else if (sv_listen_dp.value && !sv_listen_qw.value)
	{
		respond_std = false;
		respond_dp = true;
	}
	else
	{
		respond_std &= !!sv_listen_qw.value;
		respond_dp &= !!sv_listen_dp.value;
	}

	if (progstype == PROG_H2)
		respond_dp = false;	//don't bother. dp doesn't support the maps anyway.
	//dp's connections result in race conditions or are ambiguous in certain regards
	//race: dp vs nq.
	//		the dp request will generally arrive first. we check if there was a recent challenge requested, and inhibit the nq response, ensuring that dp clients connect with a known protocol
	//race: dp vs qw.
	//		DP clients will just bindly respond to both with a connection request. sending the dp one usually means the server will see the dp connection request first
	//		FTE clients explicitly ignore dp challenges with the specific 'FTE' prefix so you get qw connections there.
	//conflict: dp vs q2. dp challenge responses USUALLY contain letters. vanilla q2 is always a 32bit int. FTE clients will check that before sending an appropriate response.
	//so:
	//		vanilla nq doesn't send getchallenge, its nq connect is not inhibited, and connects directly (we optionally hack a challenge over stuffcmds, as well as protocol extensions).
	//		dp gets a dp+qw challenge, its nq request is ignored due to packet ordering and a small timeout, the server sees the dp connection request first and ignores the qw connect.
	//		fte's nq request is treated as a getchallenge. fte clients ignore the dp challenge response (if qw protocols are still enabled). ends up with a qw/fte connection
	if (!(sv_listen_nq.value || sv_bigcoords.value || !respond_std))
		respond_dp = false;

#ifdef QWOVERQ3
	if (svs.gametype != GT_PROGS && svs.gametype != GT_Q1QVM)
		respond_qwoverq3 = false;	//should probably just nuke this feature.
#endif

	if (!respond_std && !respond_dp && !respond_qwoverq3)
		return false;

	if (svs.gametype != GT_PROGS && svs.gametype != GT_Q1QVM)
	{	//if we're running q2 or q3, just ignore the whole DP thing. its irrelevent in those game modes.
		respond_std |= true;
#ifdef QWOVERQ3
		respond_qwoverq3 = false;
#endif
		respond_dp = false;
	}
	if (respond_dp)
		respond_std = false;

	challenge = SV_NewChallenge();

	//different game modes require different types of responses
	switch(svs.gametype)
	{
#ifdef Q3SERVER
	case GT_QUAKE3:	//q3 servers
		buf = va("challengeResponse %i %i %i", challenge, ioq3clchallenge, com_protocolversion.ival);
		break;
#endif
#ifdef Q2SERVER
	case GT_QUAKE2:
		buf = va("challenge %i p="STRINGIFY(PROTOCOL_VERSION_Q2EX), challenge);	//quake 2 servers give a different challenge response
		break;
#endif
	default:
		buf = va("%c%i", S2C_CHALLENGE, challenge);	//quakeworld's response is a bit poo.
		break;
	}

	over = buf + strlen(buf) + 1;

	if (svs.gametype == GT_PROGS || svs.gametype == GT_Q1QVM)
	{
		unsigned int mask;
		//tell the client what fte extensions we support
		mask = Net_PextMask(PROTOCOL_VERSION_FTE1, false)&PEXT_SERVERADVERTISE;
		if (mask)
		{
			lng = LittleLong(PROTOCOL_VERSION_FTE1);
			memcpy(over, &lng, sizeof(lng));
			over+=sizeof(lng);

			lng = LittleLong(mask);
			memcpy(over, &lng, sizeof(lng));
			over+=sizeof(lng);
		}
		//tell the client what fte extensions we support
		mask = Net_PextMask(PROTOCOL_VERSION_FTE2, false)&PEXT2_SERVERADVERTISE;
		if (mask)
		{
			lng = LittleLong(PROTOCOL_VERSION_FTE2);
			memcpy(over, &lng, sizeof(lng));
			over+=sizeof(lng);

			lng = LittleLong(mask);
			memcpy(over, &lng, sizeof(lng));
			over+=sizeof(lng);
		}
		//tell the client what mvdsv/ezquake extensions we support
		mask = Net_PextMask(PROTOCOL_VERSION_EZQUAKE1, false)&EZPEXT1_SERVERADVERTISE;
		if (mask)
		{
			lng = LittleLong(PROTOCOL_VERSION_EZQUAKE1);
			memcpy(over, &lng, sizeof(lng));
			over+=sizeof(lng);

			lng = LittleLong(mask);
			memcpy(over, &lng, sizeof(lng));
			over+=sizeof(lng);
		}
	}
	if (svs.gametype == GT_PROGS || svs.gametype == GT_Q1QVM || svs.gametype == GT_QUAKE2)
	{
		unsigned int mask;
		//report the mtu
		if (*net_mtu.string)
			mask = net_mtu.ival&~7;
		else
			mask = 8192;
		if (mask > 64)
		{
			lng = LittleLong(PROTOCOL_VERSION_FRAGMENT);
			memcpy(over, &lng, sizeof(lng));
			over+=sizeof(lng);

			lng = LittleLong(mask);
			memcpy(over, &lng, sizeof(lng));
			over+=sizeof(lng);
		}

#ifdef HUFFNETWORK
		compressioncrc = Huff_PreferedCompressionCRC();
		if (compressioncrc)
		{
			lng = LittleLong(PROTOCOL_VERSION_HUFFMAN);
			memcpy(over, &lng, sizeof(lng));
			over+=sizeof(lng);

			lng = LittleLong(compressioncrc);
			memcpy(over, &lng, sizeof(lng));
			over+=sizeof(lng);
		}
#endif

#ifdef HAVE_DTLS
		if (net_enable_dtls.ival>0/* || !*net_enable_dtls.string*/ && svs.sockets->dtlsfuncs)
		{
			lng = LittleLong(PROTOCOL_VERSION_DTLSUPGRADE);
			memcpy(over, &lng, sizeof(lng));
			over+=sizeof(lng);

			if (net_enable_dtls.ival >= 3)
				lng = LittleLong(3);	//required
			else if (net_enable_dtls.ival >= 2)
				lng = LittleLong(2);	//encouraged
			else
				lng = LittleLong(1);	//supported
			memcpy(over, &lng, sizeof(lng));
			over+=sizeof(lng);
		}
#endif
		if (*sv_guidhash.string
#ifdef HAVE_DTLS
			&& (net_enable_dtls.ival < 3 || net_from.prot == NP_DTLS)
#endif
			)
		{
			lng = LittleLong(PROTOCOL_VERSION_VARLENGTH);
			memcpy(over, &lng, sizeof(lng));
			over+=sizeof(lng);
			lng = strlen(sv_guidhash.string);
			memcpy(over, &lng, sizeof(lng));
			over+=sizeof(lng);
			lng = LittleLong(PROTOCOL_INFO_GUID);
			memcpy(over, &lng, sizeof(lng));
			over+=sizeof(lng);
			memcpy(over, sv_guidhash.string, strlen(sv_guidhash.string));
			over+=strlen(sv_guidhash.string);
		}
	}

	if (respond_dp)
	{
		char *dp;
		if (sv_listen_qw.value)
			dp = va("challenge FTE%i", challenge);	//an FTE prefix will cause FTE clients to ignore the packet, to give preference to the qw challenge + protocols
		else
			dp = va("challenge %iDP", challenge);	//we still need to add a postfix to prevent it from being interpreted as a Q2 server
		Netchan_OutOfBand(NCF_SERVER, &net_from, strlen(dp)+1, dp);
	}

	if (respond_std)
		Netchan_OutOfBand(NCF_SERVER, &net_from, over-buf, buf);

#ifdef QWOVERQ3
	if (svs.gametype == GT_PROGS || svs.gametype == GT_Q1QVM)
	{
		if (respond_qwoverq3)
		{
			buf = va("challengeResponse %i", challenge);
			Netchan_OutOfBand(NS_SERVER, &net_from, strlen(buf), buf);
		}
	}
#endif
	return true;
}

#ifdef SVRANKING
static void VARGS SV_OutOfBandPrintf (int q2, netadr_t *adr, char *format, ...)
{
	va_list		argptr;
	char		string[8192];

	va_start (argptr, format);
	if (q2)
	{
		strcpy(string, "print\n");
		vsnprintf (string+6,sizeof(string)-1-6, format+1,argptr);
	}
	else
	{
		string[0] = A2C_PRINT;
		string[1] = '\n';
		vsnprintf (string+2,sizeof(string)-1-2, format,argptr);
	}
	va_end (argptr);


	Netchan_OutOfBand (NCF_SERVER, adr, strlen(string), (qbyte *)string);
}
static void VARGS SV_OutOfBandTPrintf (int q2, netadr_t *adr, int language, translation_t text, ...)
{
	va_list		argptr;
	char		string[8192];
	const char *format = langtext(text, language);

	va_start (argptr, text);
	if (q2)
	{
		strcpy(string, "print\n");
		vsnprintf (string+6,sizeof(string)-1-6, format+1,argptr);
	}
	else
	{
		string[0] = A2C_PRINT;
		vsnprintf (string+1,sizeof(string)-1-1, format,argptr);
	}
	va_end (argptr);


	Netchan_OutOfBand (NCF_SERVER, adr, strlen(string), (qbyte *)string);
}
#endif

qboolean SV_ChallengePasses(int challenge)
{
	int i;
	for (i=0 ; i<MAX_CHALLENGES ; i++)
	{	//one per ip.
		if (NET_CompareBaseAdr (&net_from, &svs.challenges[i].adr))
		{
			if (challenge == svs.challenges[i].challenge)
				return true;
			return false;
		}
	}
	return false;
}

#ifdef NQPROT
//DP sends us a getchallenge followed by a CCREQ_CONNECT at about the same time.
//this means that DP clients tend to connect as generic NQ clients.
//and because DP _REQUIRES_ sv_bigcoords, they tend to end up being given fitz/rmq protocols
//thus we don't respond to the connect if sv_listen_dp is 1, and we had a recent getchallenge request. recent is 2 secs.
qboolean SV_ChallengeRecent(void)
{
	int curtime = realtime;	//yeah, evil. sue me. consitent with challenges.
	int i;
	for (i=0 ; i<MAX_CHALLENGES ; i++)
	{	//one per ip.
		if (NET_CompareBaseAdr (&net_from, &svs.challenges[i].adr))
		{
			if (svs.challenges[i].time > curtime - 2)
				return true;
		}
	}
	return false;
}
#endif

void VARGS SV_RejectMessage(enum serverprotocols_e protocol, char *format, ...)
{
	va_list		argptr;
	char		string[8192];
	int len;

	va_start (argptr, format);
	switch(protocol)
	{
#ifdef NQPROT
	case SCP_NETQUAKE:
	case SCP_BJP3:
	case SCP_FITZ666:
		string[4] = CCREP_REJECT;
		vsnprintf (string+5,sizeof(string)-1-5, format,argptr);
		len = strlen(string+4)+1+4;
		*(int*)string = BigLong(NETFLAG_CTL|len);
		NET_SendPacket(svs.sockets, len, string, &net_from);
		return;
	case SCP_DARKPLACES6:
	case SCP_DARKPLACES7:
		strcpy(string, "reject ");
		vsnprintf (string+7,sizeof(string)-1-7, format,argptr);
		len = strlen(string);
		break;
#endif

	case SCP_QUAKE2:
	case SCP_QUAKE3:
	default:
		strcpy(string, "print\n");
		vsnprintf (string+6,sizeof(string)-1-6, format,argptr);
		len = strlen(string);
		break;

	case SCP_QUAKEWORLD:
		string[0] = A2C_PRINT;
		string[1] = '\n';
		vsnprintf (string+2,sizeof(string)-1-2, format,argptr);
		len = strlen(string);
		break;
	}
	va_end (argptr);

	Netchan_OutOfBand (NCF_SERVER, &net_from, len, (qbyte *)string);
}

void SV_AcceptMessage(client_t *newcl)
{
	char		string[8192];
	sizebuf_t	sb;
	int len;
#ifdef NQPROT
	netadr_t localaddr;
#endif

	memset(&sb, 0, sizeof(sb));
	sb.maxsize = sizeof(string);
	sb.data = string;

	safeswitch(newcl->protocol)
	{
#ifdef NQPROT
	case SCP_NETQUAKE:
	case SCP_BJP3:
	case SCP_FITZ666:
//		if (net_from.type != NA_LOOPBACK)
		{
			SZ_Clear(&sb);
			MSG_WriteLong(&sb, 0);
			MSG_WriteByte(&sb, CCREP_ACCEPT);
			if (newcl->qex)
				;	//skip any port info (as well as any proquake ident stuff).
			else
			{
				NET_LocalAddressForRemote(svs.sockets, &net_from, &localaddr, 0);
				if (net_from.prot == NP_DTLS
				#ifdef SUPPORT_ICE
						|| net_from.type == NA_ICE
				#endif
						)
					MSG_WriteLong(&sb, 0);	//send a port of 0 if we expect the client to be sane enough and/or otherwise problematic.
				else
					MSG_WriteLong(&sb, ShortSwap(localaddr.port));
				if (newcl->proquake_angles_hack)
				{
					MSG_WriteByte(&sb, MOD_PROQUAKE);
					MSG_WriteByte(&sb, MOD_PROQUAKE_VERSION);
					MSG_WriteByte(&sb, 0/*flags*/);
				}
			}
			*(int*)sb.data = BigLong(NETFLAG_CTL|sb.cursize);
			NET_SendPacket(svs.sockets, sb.cursize, sb.data, &net_from);
			return;
		}
	case SCP_DARKPLACES6:
	case SCP_DARKPLACES7:
		strcpy(string, "accept");
		len = strlen(string);
		break;
#else
	case SCP_NETQUAKE:
	case SCP_BJP3:
	case SCP_FITZ666:
	case SCP_DARKPLACES6:
	case SCP_DARKPLACES7:
		return;
#endif

	case SCP_QUAKE3:
		strcpy(string, "connectResponse");
		len = strlen(string);
		break;
	case SCP_QUAKE2:
	case SCP_QUAKE2EX:
		Q_snprintfz(string, sizeof(string), "client_connect%s%s\n",
			(newcl->protocol==SCP_QUAKE2EX)?" "STRINGIFY(PROTOCOL_VERSION_Q2EX):"",
			(*fs_dlURL.string?va(" dlserver=%s", fs_dlURL.string):"")	//q2pro's dlserver hint
			);
		len = strlen(string);
		break;

	safedefault:
	case SCP_BAD:
	case SCP_QUAKEWORLD:
		string[0] = S2C_CONNECTION;
		string[1] = '\n';
		string[2] = '\0';
		len = strlen(string);
		break;
	}

	Netchan_OutOfBand (NCF_SERVER, &net_from, len, (qbyte *)string);
}

#if !defined(_DEBUG) || defined(_WIN32) || defined(FTE_TARGET_WEB)
static void SV_CheckRecentCrashes(client_t *tellclient)
{
}
#else
#include <sys/stat.h>
static void SV_CheckRecentCrashes(client_t *tellclient)
{
	struct stat sb;
	if (-1 != stat("crash.log", &sb))
	{
		if ((time(NULL) - sb.st_mtime) > 2*24*60*60)
			return;	//after 2 days, we stop advertising that we once crashed.
		SV_ClientPrintf(tellclient, PRINT_HIGH, "\1WARNING: crash.log exists, dated %s\n", ctime(&sb.st_mtime));
	}
}
#endif


void SV_ClientProtocolExtensionsChanged(client_t *client)
{
	int i;
	int maxpacketentities;
	extern cvar_t pr_maxedicts;
	client_t *seat;

	extern cvar_t sv_protocol;
	char *s = sv_protocol.string;
	while ((s = COM_Parse(s)))
	{
		if (!strcasecmp(com_token, "fte2"))
		{	//fancy stuff
			client->fteprotocolextensions
					|= PEXT_CSQC				/*mods break without*/
					 | PEXT_CHUNKEDDOWNLOADS	/*much faster downloads+redirects*/
					 ;
			client->fteprotocolextensions2
					|= PEXT2_PRYDONCURSOR		/*mods might break without*/
//					 | PEXT2_VOICECHAT			/*entirely optional*/
					 | PEXT2_SETANGLEDELTA		/*mostly just nice to have*/
					 | PEXT2_REPLACEMENTDELTAS	/*carries quite a bit of extra info*/
					 | PEXT2_MAXPLAYERS			/*not supporting the extra players is bad*/
					 | PEXT2_PREDINFO			/*fixes some repdelta issues (especially for nq)*/
					 | PEXT2_NEWSIZEENCODING	/*more accurate sizes, for awkward mods*/
//					 | PEXT2_INFOBLOBS			/*allows mods to send infoblobs to csqc (for avatar images or whatever)*/
					 ;
		}
		if (!strcasecmp(com_token, "fte1"))
		{	//older stuff. most of this was replaced by replacementdeltas.
			client->fteprotocolextensions
					|= PEXT_SETVIEW
					 | PEXT_SCALE
					 | PEXT_TRANS
					 | PEXT_ACCURATETIMINGS
					 | PEXT_SOUNDDBL
					 | PEXT_MODELDBL
					 | PEXT_ENTITYDBL
					 | PEXT_ENTITYDBL2
					 | PEXT_FLOATCOORDS
					 | PEXT_COLOURMOD
					 | PEXT_SPAWNSTATIC2
					 | PEXT_256PACKETENTITIES
					 | PEXT_SETATTACHMENT
					 | PEXT_CHUNKEDDOWNLOADS
					 | PEXT_CSQC
					 | PEXT_DPFLAGS
					 ;
		}
		if (!strcasecmp(com_token, "csqc"))
		{	//JUST csqc.
			client->fteprotocolextensions
					|= PEXT_CSQC
					;
		}
	}

	client->fteprotocolextensions  &= Net_PextMask(PROTOCOL_VERSION_FTE1, ISNQCLIENT(client)) & PEXT_SERVERADVERTISE;
	client->fteprotocolextensions2 &= Net_PextMask(PROTOCOL_VERSION_FTE2, ISNQCLIENT(client)) & PEXT2_SERVERADVERTISE;
	client->ezprotocolextensions1  &= Net_PextMask(PROTOCOL_VERSION_EZQUAKE1, ISNQCLIENT(client)) & EZPEXT1_SERVERADVERTISE;
	client->zquake_extensions &= SERVER_SUPPORTED_Z_EXTENSIONS;

	//older versions of fte didn't understand any interactions between ez's limited float support and replacement deltas. so only activate both when vrinputs is also supported.
	if ((client->ezprotocolextensions1 & EZPEXT1_FLOATENTCOORDS) && (client->fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS) && !(client->fteprotocolextensions2 & PEXT2_VRINPUTS))
		client->ezprotocolextensions1 &= ~EZPEXT1_FLOATENTCOORDS;

	//some gamecode can't cope with some extensions for some reasons... and I'm too lazy to fix the code to cope.
	if (svs.gametype == GT_HALFLIFE)
		client->fteprotocolextensions2 &= ~PEXT2_REPLACEMENTDELTAS;	//baseline issues

#ifdef HAVE_LEGACY
	if (ISQWCLIENT(client))
	{
		//be prepared to recognise client versions, in order to block known-buggy extensions.
		const char *s;
		int ver;
		extern cvar_t pext_ezquake_nochunks;
		extern cvar_t pext_ezquake_verfortrans;
		s = InfoBuf_ValueForKey(&client->userinfo, "*client");
		if (!strncmp(s, "ezQuake", 7))
		{
			s = COM_Parse(s);	//skip name-of-fork
			COM_Parse(s);	//tokenize the version
			ver = atoi(com_token);

			//this should actually have been resolved now, but for future use...
			if ((client->fteprotocolextensions & PEXT_CHUNKEDDOWNLOADS) && pext_ezquake_nochunks.ival)
			{
				client->fteprotocolextensions &= ~PEXT_CHUNKEDDOWNLOADS;
				SV_PrintToClient(client, PRINT_HIGH, "ezQuake's implementation of chunked downloads is blocked on this server.\n");
			}

			//client fails to read the extra byte when PF_EXTRA_PFS is set, instead checking for the 18th bit in a 16-bit (signed) variable.
			if ((client->fteprotocolextensions & PEXT_TRANS) && ver < pext_ezquake_verfortrans.ival)
			{
				SV_PrintToClient(client, PRINT_HIGH, "ezQuake's implementation of PEXT_TRANS is buggy. Disabling.\n");
				client->fteprotocolextensions &= ~PEXT_TRANS;
			}
			//in order to simultaneously support PF_SOLID+Z_EXT_PF_SOLID and PF_HULLSIZE_Z+Z_EXT_PF_ONGROUND, I had to redefine the protocol when both were enabled.
			//ezquake does not understand the change.
			if ((client->zquake_extensions & (Z_EXT_PF_ONGROUND|Z_EXT_PF_SOLID)) && ver < pext_ezquake_verfortrans.ival)
			{
				if (client->fteprotocolextensions & PEXT_HULLSIZE)
					SV_PrintToClient(host_client, PRINT_HIGH, "ezQuake's implementation of PEXT_HULLSIZE conflicts with zquake extensions.\n");
				if (client->fteprotocolextensions & PEXT_SCALE)
					SV_PrintToClient(host_client, PRINT_HIGH, "ezQuake's implementation of PEXT_SCALE conflicts with zquake extensions.\n");
				if (client->fteprotocolextensions & PEXT_FATNESS)
					SV_PrintToClient(host_client, PRINT_HIGH, "ezQuake's implementation of PEXT_FATNESS conflicts with zquake extensions.\n");
				if (client->fteprotocolextensions & PEXT_TRANS)
					SV_PrintToClient(host_client, PRINT_HIGH, "ezQuake's implementation of PEXT_TRANS conflicts with zquake extensions.\n");
				client->fteprotocolextensions &= ~(PEXT_HULLSIZE|PEXT_TRANS|PEXT_SCALE|PEXT_FATNESS);
			}
		}

		//its not that I'm singling out ezquake or anything, but it has too many people using outdated versions that its hard to ignore.
	}
#endif

	//
	client->maxmodels = 256;
	if (client->fteprotocolextensions & PEXT_256PACKETENTITIES)
		maxpacketentities = MAX_EXTENDED_PACKET_ENTITIES;
	else
		maxpacketentities = MAX_STANDARD_PACKET_ENTITIES;	//true for qw,q2

	if (client->fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS)
	{
		client->max_net_clients = ISQWCLIENT(client)?QWMAX_CLIENTS:NQMAX_CLIENTS;
		client->max_net_staticents = ~0u;	//unlimited in both fte+qss.

		//you need to reconnect for this to update, of course. so make sure its not *too* low...
		client->max_net_ents =  bound(512, pr_maxedicts.ival, MAX_EDICTS);
		client->maxmodels = min(1u<<14, MAX_PRECACHE_MODELS);	//protocol limited to 14 bits.
	}
	else if (ISQWCLIENT(client))	//readd?
	{
		client->max_net_clients = QWMAX_CLIENTS;
		client->max_net_ents = 512;
		if (client->fteprotocolextensions & PEXT_ENTITYDBL)
			client->max_net_ents += 512;
		if (client->fteprotocolextensions & PEXT_ENTITYDBL2)
			client->max_net_ents += 1024;
		client->max_net_staticents = 512;	//the ezquake limit, too few people use vanilla to really care about that (it would be too unstable anyway). fodquake has no limit.

		if (client->fteprotocolextensions & PEXT_MODELDBL)
			client->maxmodels = 512;
	}
	else if (ISDPCLIENT(client))
	{
		client->max_net_clients = 255;
		client->max_net_ents = bound(512, pr_maxedicts.ival, 32768);
		client->max_net_staticents = 1024;			//its quite low, proportionally.
		client->maxmodels = MAX_PRECACHE_MODELS;	//protocol limit of 16 bits. 15 bits for late precaches. client limit of 1k

		client->datagram.maxsize = sizeof(host_client->datagram_buf);
	}
	else if (client->protocol == SCP_BJP3 || client->protocol == SCP_FITZ666)
	{
		client->max_net_clients = NQMAX_CLIENTS;
		client->max_net_ents = bound(512, pr_maxedicts.ival, 32768);	//fitzquake supports 65535, but our writeentity builtin works differently, which causes problems.
		client->max_net_staticents = 4096;	//quakespasm has 4k, more than 3k starts to have issues with the msg_init buffer size.
		client->maxmodels = MAX_PRECACHE_MODELS;
		maxpacketentities = client->max_net_ents;

		client->datagram.maxsize = sizeof(host_client->datagram_buf);
	}
	else if (client->qex)
	{
		client->max_net_clients = NQMAX_CLIENTS;
		client->datagram.maxsize = sizeof(host_client->datagram_buf);
		client->max_net_ents = bound(512, pr_maxedicts.ival, 32768);
		client->max_net_staticents = 4096;
	}
	else
	{
		client->max_net_clients = NQMAX_CLIENTS;
		client->datagram.maxsize = MAX_NQDATAGRAM;	//vanilla limit
		if (client->proquake_angles_hack)
			client->max_net_ents = bound(512, pr_maxedicts.ival, 8192);
		else
			client->max_net_ents = bound(512, pr_maxedicts.ival, 600);
		client->max_net_staticents = 128;	//yeah, its low.
	}

	if (client->fteprotocolextensions2 & PEXT2_MAXPLAYERS)
		client->max_net_clients = MAX_CLIENTS;

	client->max_net_clients = min(client->max_net_clients, MAX_CLIENTS);

	client->pendingdeltabits = NULL;
	client->pendingcsqcbits = NULL;

	//initialise the client's frames, based on that client's protocol
	switch(client->protocol)
	{
#ifdef Q3SERVER
	case SCP_QUAKE3:
		if (client->frameunion.q3frames)
			Z_Free(client->frameunion.q3frames);
		client->frameunion.q3frames = NULL;//Z_Malloc(Q3UPDATE_BACKUP*sizeof(*client->frameunion.q3frames));
		break;
#endif

#ifdef Q2SERVER
	case SCP_QUAKE2:
		// build a new connection
		// accept the new client
		// this is the only place a client_t is ever initialized
		client->frameunion.q2frames = client->frameunion.q2frames;	//don't touch these.
		if (client->frameunion.q2frames)
			Z_Free(client->frameunion.q2frames);

		client->frameunion.q2frames = Z_Malloc(sizeof(q2client_frame_t)*Q2UPDATE_BACKUP);
		break;
#endif

	default:
		if (client->frameunion.frames)
			Z_Free(client->frameunion.frames);

		if ((client->fteprotocolextensions2 & PEXT2_REPLACEMENTDELTAS) || ISDPCLIENT(client))
		{
#define round64(size) ((size+7)&~7)
			char *ptr;
			int maxents = maxpacketentities*4;	/*this is the max number of ents updated per frame. we can't track more, so...*/
			if (maxents > client->max_net_ents)
				maxents = client->max_net_ents;
			ptr = Z_Malloc(	round64(sizeof(client_frame_t)*UPDATE_BACKUP)+
							round64(sizeof(*client->pendingdeltabits)*client->max_net_ents)+
							round64(sizeof(*client->pendingcsqcbits)*client->max_net_ents)+
							round64(sizeof(*client->frameunion.frames[i].resend)*maxents)*UPDATE_BACKUP);
			client->frameunion.frames = (void*)ptr;
			ptr += round64(sizeof(*client->frameunion.frames)*UPDATE_BACKUP);
			client->pendingdeltabits = (void*)ptr;
			ptr += round64(sizeof(*client->pendingdeltabits)*client->max_net_ents);
			client->pendingcsqcbits = (void*)ptr;
			ptr += round64(sizeof(*client->pendingcsqcbits)*client->max_net_ents);
			for (i = 0; i < UPDATE_BACKUP; i++)
			{
				client->frameunion.frames[i].maxresend = maxents;
				client->frameunion.frames[i].resend = (void*)ptr;
				ptr += round64(sizeof(*client->frameunion.frames[i].resend)*maxents);
				client->frameunion.frames[i].senttime = realtime;
			}

			//make sure the reset is sent.
			client->pendingdeltabits[0] = UF_SV_REMOVE;
		}
		else if (ISNQCLIENT(client))
		{
			client->frameunion.frames = Z_Malloc((sizeof(client_frame_t))*UPDATE_BACKUP);
			for (i = 0; i < UPDATE_BACKUP; i++)
			{
				client->frameunion.frames[i].qwentities.max_entities = 0;
				client->frameunion.frames[i].qwentities.entities = NULL;
				client->frameunion.frames[i].senttime = realtime;
			}
		}
		else
		{
			client->frameunion.frames = Z_Malloc((sizeof(client_frame_t)+sizeof(entity_state_t)*maxpacketentities)*UPDATE_BACKUP);
			for (i = 0; i < UPDATE_BACKUP; i++)
			{
				client->frameunion.frames[i].qwentities.max_entities = maxpacketentities;
				client->frameunion.frames[i].qwentities.entities = (entity_state_t*)(client->frameunion.frames+UPDATE_BACKUP) + i*client->frameunion.frames[i].qwentities.max_entities;
				client->frameunion.frames[i].senttime = realtime;
			}
		}
		break;
	}

	//make sure we have the right limits for splitscreen clients too (mostly for viewmodel safety checks)
	for (seat = client->controlled; seat; seat = seat->controlled)
	{
		seat->max_net_clients = client->max_net_clients;
		seat->max_net_ents = client->max_net_ents;
		seat->maxmodels = client->maxmodels;
	}

	client->lastsequence_acknowledged = -2000000000;
}


//void NET_AdrToStringResolve (netadr_t *adr, void (*resolved)(void *ctx, void *data, size_t a, size_t b), void *ctx, size_t a, size_t b);
static void SV_UserDNSResolved(void *ctx, void *data, size_t idx, size_t uid)
{
	if (idx < svs.allocated_client_slots)
	{
		client_t *cl = &svs.clients[idx];
		if (cl->userid == uid)
		{
			Z_Free(cl->reversedns);
			cl->reversedns = data;
			SV_LogPlayer(cl, va("dns %s", cl->reversedns));
			return;
		}
	}
	//client ran away before the dns completed
	Con_DPrintf("stale dns lookup result: %s\n", (char*)data);
	Z_Free(data);
}

client_t *SV_AddSplit(client_t *controller, char *info, int id)
{
	client_t *cl, *prev;
	int i, j;
	int curclients;
	qboolean loadgame;
//	const char *name;
	char newname[80];
	unsigned int clients = 0, spectators = 0;
	qboolean asspec;

	for (curclients = 0, prev = cl = controller; cl; cl = cl->controlled)
	{
		prev = cl;
		curclients++;
	}

	if (id && curclients != id)
		return NULL;	//this would be weird.

//	if (curclients >= 16)
//		return NULL;	//protocol limit on stats.
	if (curclients >= MAX_SPLITS)
		return NULL;
	//only allow splitscreen if its explicitly allowed. unless its the local client in which case its always allowed.
	//wouldn't it be awesome if we could always allow it for spectators? the join command makes that awkward, though I suppose we could just drop the extras in that case.
	if (!sv_allow_splitscreen.ival && controller->netchan.remote_address.type != NA_LOOPBACK)
		return NULL;	//FIXME: allow spectators to do this anyway?

/*		if (cl->state == cs_loadzombie)
		{
			if (!newcl)
			{
				if (((!strcmp(cl->name, name) || !*cl->name) && (!*cl->guid || !strcmp(guid, cl->guid))) || sv.allocated_client_slots <= 1)	//named, or first come first serve.
				{
					if (cl->istobeloaded)
						Con_DPrintf("%s:Using loadzombie\n", svs.name);
					else
						Con_DPrintf("%s:Using parmzombie\n", svs.name);
					newcl = cl;
					preserveparms = true;
					temp.istobeloaded = cl->istobeloaded;
					memcpy(temp.spawn_parms, cl->spawn_parms, sizeof(temp.spawn_parms));
					if (cl->userid)
						temp.userid = cl->userid;
					break;
				}
			}
		}
*/

	SV_DeDupeName(Info_ValueForKey(info, "name"), cl, newname, sizeof(newname));
	for (i=0,cl=svs.clients ; i<sv.allocated_client_slots ; i++,cl++)
	{
		if (cl->state == cs_loadzombie && !controller->spectator)
		{	//if this is a loadzombie with the same name as the new seat is trying to use then lets use that slot.
			if (!strcmp(cl->name, newname))
				break;
		}
	}
	if (i == sv.allocated_client_slots)
	{
		for (i=0,cl=svs.clients ; i<sv.allocated_client_slots ; i++,cl++)
		{
			if (cl->state == cs_free)
			{
				break;
			}
		}
		if (i == sv.allocated_client_slots)
		{
			SV_PrintToClient(controller, PRINT_HIGH, "not enough free player slots\n");
			return NULL;
		}
	}

	loadgame = (cl->state == cs_loadzombie);
	if (loadgame)
		asspec = cl->spectator;
	else
		asspec = !!atoi(Info_ValueForKey(info, "spectator"));
	for (j=0 ; j<sv.allocated_client_slots ; j++)
	{
		if (svs.clients[j].state == cs_free)
			continue;
		if (svs.clients[j].spectator && svs.clients[j].spectator!=2)
			spectators++;
		else
			clients++;
	}
	if (controller->netchan.remote_address.type != NA_LOOPBACK)	//ignore limits for the local client (they could just tweak the cvar instead, but that's a hassle)
	if ((asspec?spectators:clients) >= (asspec?maxspectators.ival:maxclients.ival))
	{
		SV_PrintToClient(controller, PRINT_HIGH, "Server full, cannot add new seat\n");
		return NULL;
	}


	{	//save off anything we might want to preserve
		qboolean tobeloaded = cl->istobeloaded;
		qboolean spawned = cl->spawned;
		unsigned int userid = cl->userid;
		float	spawn_parms[NUM_SPAWN_PARMS];
		memcpy(spawn_parms, cl->spawn_parms, sizeof(spawn_parms));

		//now actually wipe the player slot
		memset(cl, 0, sizeof(*cl));

		//and restore preserved stuff, if its appropriate.
		if (loadgame)
		{
			cl->istobeloaded = tobeloaded;
			cl->spawned = spawned;
			cl->userid = userid;
			memcpy(cl->spawn_parms, spawn_parms, sizeof(cl->spawn_parms));
		}
	}

	cl->spectator = asspec;
	cl->netchan.remote_address = controller->netchan.remote_address;
	cl->netchan.message.prim = controller->netchan.message.prim;
	cl->backbuf.prim = controller->backbuf.prim;
	cl->netchan.netprim = controller->netchan.netprim;
	cl->zquake_extensions = controller->zquake_extensions;
	cl->fteprotocolextensions = controller->fteprotocolextensions;
	cl->fteprotocolextensions2 = controller->fteprotocolextensions2;
	cl->ezprotocolextensions1 = controller->ezprotocolextensions1;
	cl->penalties = controller->penalties;
	cl->protocol = controller->protocol;
	cl->maxmodels = controller->maxmodels;
	cl->max_net_clients = controller->max_net_clients;
	cl->max_net_ents = controller->max_net_ents;

	if (*controller->guid)
		Q_snprintfz(cl->guid, sizeof(cl->guid), "%s:%i", controller->guid, curclients);
	else
		Q_strncpyz(cl->guid, "", sizeof(cl->guid));
	cl->name = cl->namebuf;
	cl->team = cl->teambuf;
	cl->userinfo.ChangeCB = svs.info.ChangeCB;
	cl->userinfo.ChangeCTX = &cl->userinfo;

	if (!cl->userid || !loadgame)
		cl->userid = ++nextuserid;

	cl->playerclass = 0;
	cl->pendingdeltabits = NULL;
	cl->pendingcsqcbits = NULL;
	cl->seat = curclients;

	cl->edict = NULL;
#ifdef Q2SERVER
	cl->q2edict = NULL;
#endif
	switch(svs.gametype)
	{
#ifdef Q2SERVER
	case GT_QUAKE2:
		cl->q2edict = Q2EDICT_NUM(i+1);

		if (!ge->ClientConnect(cl->q2edict, info))
		{
			const char *reject = Info_ValueForKey(info, "rejmsg");
			if (*reject)
				SV_ClientPrintf(controller, PRINT_HIGH, "Splitscreen Refused: %s\n", reject);
			else
				SV_ClientPrintf(controller, PRINT_HIGH, "Splitscreen Refused\n");
			Con_DPrintf ("Game rejected a connection.\n");
			return NULL;
		}

		ge->ClientUserinfoChanged(cl->q2edict, info);
		break;
#endif
	default:
		cl->edict = EDICT_NUM_PB(svprogfuncs, i+1);
		break;
	}

	prev->controlled = cl;
	prev = cl;
	cl->controller = controller;
	cl->controlled = NULL;

	InfoBuf_FromString(&cl->userinfo, info, false);
	InfoBuf_RemoveKey (&cl->userinfo, "spectator");
	//this is a hint rather than a game breaker should it fail.
	if (cl->spectator)
		InfoBuf_SetValueForStarKey (&cl->userinfo, "*spectator", va("%i", cl->spectator));
	cl->state = controller->state;
	cl->connection_started = realtime;

//	host_client = NULL;
//	sv_player = NULL;

	SV_ExtractFromUserinfo (cl, true);
	if (!loadgame)
		SV_GetNewSpawnParms(cl);

	if (cl->state >= cs_connected)
	{
		cl->sendinfo = true;
		if (svprogfuncs)
			SV_SetUpClientEdict(cl, cl->edict);
	}
	if (cl->state >= cs_spawned)
		SV_Begin_Core(cl);
	return cl;
}

void SV_DoDirectConnect(svconnectinfo_t *fte_restrict info)
{
	int			i;
	client_t	*cl, *newcl;
	client_t	temp;
	edict_t		*ent;
#ifdef Q2SERVER
	q2edict_t	*q2ent;
#endif
	int			edictnum;
	char		*s;
	int			clients, spectators;
	qboolean	spectator;
	char		basic[80];
	qboolean	redirect = false;
	qboolean	preserveparms = false;
	unsigned int	ncflags;

#ifdef NQPROT
	extern cvar_t sv_protocol_nq;
#endif


	char *name;
	char adrbuf[MAX_ADR_SIZE];

	net_from = info->adr; //SV_AcceptMessage+SV_RejectMessage are lame.


	// check for password or spectator_password
	if (svprogfuncs)
	{
		s = Info_ValueForKey (info->seat[0].info, "spectator");
		if (s[0] && strcmp(s, "0"))
		{
			if (spectator_password.string[0] &&
				stricmp(spectator_password.string, "none") &&
				strcmp(spectator_password.string, s) &&
				!NET_IsLoopBackAddress(&info->adr))
			{	// failed
				Con_TPrintf ("%s:spectator password failed\n", NET_AdrToString (adrbuf, sizeof(adrbuf), &info->adr));
				SV_RejectMessage (info->protocol, "requires a spectator password\n\n");
				return;
			}
			Info_RemoveKey (info->seat[0].info, "spectator"); // remove key
			Info_SetValueForStarKey (info->seat[0].info, "*spectator", "1", sizeof(info->seat[0].info));
			spectator = true;
		}
		else
		{
			if (!password.string[0] ||
				!stricmp(password.string, "none") ||
				NET_IsLoopBackAddress(&info->adr))
				;	//don't care, doesn't matter.
			else if (info->protocol == SCP_NETQUAKE)
			{	//if its a proquake client then use numeric passwords, which take a bit of processing
				char *e;
				int got = strtol(Info_ValueForKey (info->seat[0].info, "password"), NULL, 0);
				int need = strtol(password.string, &e, 0);
				if (*e)
					need = CalcHashInt(&hash_md4, password.string, strlen(password.string));
				if (got != need)
				{
					Con_TPrintf ("%s:password failed\n", NET_AdrToString (adrbuf, sizeof(adrbuf), &info->adr));
					SV_RejectMessage (info->protocol, "server requires a password\n\n");
					return;
				}
			}
			else
			{
				s = Info_ValueForKey (info->seat[0].info, "password");
				if (strcmp(password.string, s))
				{
					Con_TPrintf ("%s:password failed\n", NET_AdrToString (adrbuf, sizeof(adrbuf), &info->adr));
					SV_RejectMessage (info->protocol, "server requires a password\n\n");
					return;
				}
			}
			spectator = false;
			Info_RemoveKey (info->seat[0].info, "password"); // remove passwd
			Info_RemoveKey (info->seat[0].info, "*spectator"); // remove key
		}
	}
	else
		spectator = false;//q2 does all of it's checks internally, and deals with spectator ship too

	newcl = &temp;
	memset (newcl, 0, sizeof(client_t));

#ifdef NQPROT
	if (!info->supportedprotocols && info->protocol == SCP_NETQUAKE)
	{	//NQ protocols lack stuff like protocol extensions.
		//its the wild west where nothing is known about the client and everything breaks.
		//we defer the assumption to the sv_protocol_nq cvar (only for clients that don't report their known protocols)
		if (!strcmp(sv_protocol_nq.string, "fitz"))
			info->protocol = SCP_FITZ666;
		else if (!strcmp(sv_protocol_nq.string, "bjp") || !strcmp(sv_protocol_nq.string, "bjp3"))
			info->protocol = SCP_BJP3;
		else if (!strcmp(sv_protocol_nq.string, "dpp6") || !strcmp(sv_protocol_nq.string, "dp6"))
			info->protocol = SCP_DARKPLACES6;
		else if (!strcmp(sv_protocol_nq.string, "dpp7") || !strcmp(sv_protocol_nq.string, "dp7"))
			info->protocol = SCP_DARKPLACES7;
		else if (!strcmp(sv_protocol_nq.string, "id") || !strcmp(sv_protocol_nq.string, "vanilla"))
			info->protocol = SCP_NETQUAKE;
		else switch(sv_protocol_nq.ival)
		{
		case PROTOCOL_VERSION_RMQ:
		case PROTOCOL_VERSION_FITZ:
			info->protocol = SCP_FITZ666;
			break;
		case PROTOCOL_VERSION_BJP3:
			info->protocol = SCP_BJP3;
			break;
		case 15:
			info->protocol = SCP_NETQUAKE;
			break;
		case PROTOCOL_VERSION_DP6:
			info->protocol = SCP_DARKPLACES6;
			break;
		case PROTOCOL_VERSION_DP7:
			info->protocol = SCP_DARKPLACES7;
			break;
		default:
			Con_Printf("sv_protocol_nq set incorrectly\n");
		case 0:
			//change nothing
			break;
		}
	}
	newcl->supportedprotocols = info->supportedprotocols;
	newcl->proquake_angles_hack = info->proquakeanglehack;
	newcl->qex = info->isqex;
#endif

	newcl->userid = ++nextuserid;
	newcl->fteprotocolextensions = info->ftepext1;
	newcl->fteprotocolextensions2 = info->ftepext2;
	newcl->ezprotocolextensions1 = info->ezpext1;
	newcl->protocol = info->protocol;
	newcl->pextknown = info->ftepext1||info->ftepext2||info->ezpext1;
	Q_strncpyz(newcl->guid, info->guid, sizeof(newcl->guid));

//	Con_TPrintf("%s:%s:connect\n", sv.name, NET_AdrToString (adrbuf, sizeof(adrbuf), &adr));

	// if there is already a slot for this ip, drop it
	for (i=0,cl=svs.clients ; i<svs.allocated_client_slots ; i++,cl++)
	{
		if (cl->state == cs_free || cl->state == cs_loadzombie)
			continue;
		if (NET_CompareBaseAdr (&info->adr, &cl->netchan.remote_address)
			&& ((info->protocol == SCP_QUAKEWORLD && cl->netchan.qport == info->qport) || info->adr.port == cl->netchan.remote_address.port ))
		{
			if (realtime - cl->connection_started < sv_reconnectlimit.value)
			{
				Con_Printf ("%s:reconnect rejected: too soon\n", NET_AdrToString (adrbuf, sizeof(adrbuf), &info->adr));
				return;
			}

			if (cl->state == cs_connected)
			{
				if (cl->protocol != info->protocol)
				{
					Con_TPrintf("%s: diff prot connect\n", NET_AdrToString (adrbuf, sizeof(adrbuf), &info->adr));
					return;
				}
				else
					Con_TPrintf("%s:dup connect\n", NET_AdrToString (adrbuf, sizeof(adrbuf), &info->adr));
			}
			/*else if (cl->state == cs_zombie)
			{
				Con_Printf ("%s:reconnect\n", NET_AdrToString (adrbuf, sizeof(adrbuf), &adr));
			}*/
			else
				Con_TPrintf ("%s:%s:reconnect\n", svs.name, NET_AdrToString (adrbuf, sizeof(adrbuf), &info->adr));

#if 1
			//wait for timeout before the player can reuse that address:port combo. clients should pick a new port on reconnect if they want to connect instantly.
			return;
#else
			//silently drop the old connection, without causing the old client to get a disconnect or anything stupid like that.
			cl->protocol = SCP_BAD;
			SV_DropClient (cl);
			cl->protocol = info->protocol;
			break;
#endif
		}
	}

	name = Info_ValueForKey (info->seat[0].info, "name");

	/*
	if (sv.world.worldmodel && info->protocol == SCP_QUAKEWORLD &&!atoi(Info_ValueForKey (info->userinfo, "iknow")))
	{
		if (sv.world.worldmodel->fromgame == fg_halflife && !(newcl->fteprotocolextensions & PEXT_HLBSP))
		{
			if (atof(Info_ValueForKey (info->userinfo, "*FuhQuake")) < 0.3)
			{
				SV_RejectMessage (info->protocol, "The server is using a halflife level and we don't think your client supports this\nuse 'setinfo iknow 1' to ignore this check\nYou can go to "ENGINEWEBSITE" to get a compatible client\n\nYou may need to enable an option\n\n");
//				Con_Printf("player %s was dropped due to incompatible client\n", name);
//				return;
			}
		}
#ifdef PEXT_Q2BSP
		else if (sv.world.worldmodel->fromgame == fg_quake2 && !(newcl->fteprotocolextensions & PEXT_Q2BSP))
		{
			SV_RejectMessage (info->protocol, "The server is using a q2bsp-format level and we don't think your client supports this\nuse 'setinfo iknow 1' to ignore this check\nYou can go to "ENGINEWEBSITE" to get a compatible client\n\nYou may need to enable an option\n\n");
//			Con_Printf("player %s was dropped due to incompatible client\n", name);
//			return;
		}
#endif
#ifdef PEXT_Q3BSP
		else if (sv.world.worldmodel->fromgame == fg_quake3 && !(newcl->fteprotocolextensions & PEXT_Q3BSP))
		{
			SV_RejectMessage (info->protocol, "The server is using a q3bsp-format level and we don't think your client supports this\nuse 'setinfo iknow 1' to ignore this check\nYou can go to "ENGINEWEBSITE" to get a compatible client\n\nYou may need to enable an option\n\n");
//			Con_Printf("player %s was dropped due to incompatible client\n", name);
//			return;
		}
#endif
	}
	*/

	SV_FixupName(name, temp.namebuf, sizeof(temp.namebuf));
	name = temp.namebuf;

	deleetstring(basic, name);
	if (!*basic || strstr(basic, "console"))
		name = "unnamed";	//have fun dudes.

	// count up the clients and spectators
	clients = 0;
	spectators = 0;
	newcl = NULL;
	if (!sv.allocated_client_slots)
	{
		Con_Printf("Apparently, there are no client slots allocated. This shouldn't be happening\n");
		return;
	}
	for (i=0,cl=svs.clients ; i<sv.allocated_client_slots ; i++,cl++)
	{
		if (cl->state == cs_free)
			continue;
		if (cl->spectator && cl->spectator!=2)
			spectators++;
		else
			clients++;

		if (cl->state == cs_loadzombie)
		{	//only try if they actually match this one...
			if ((!strcmp(cl->name, name) || !*cl->name) && (!*cl->guid || !strcmp(info->guid, cl->guid)))
			{
				newcl = cl;
				break;
			}
			if (!newcl)	//just use any.
				newcl = cl;
		}
	}

	if (newcl)
	{	//client is reprising a loaded slot.
		if (newcl->istobeloaded)
		{
			SV_BroadcastTPrintf(PRINT_HIGH, "%s reprises %s\n", name, newcl->name);
			Con_DPrintf("%s:Using loadzombie\n", svs.name);
		}
		else
			Con_DPrintf("%s:Using parmzombie\n", svs.name);
		preserveparms = true;
		temp.istobeloaded = newcl->istobeloaded;
		temp.spawned = newcl->spawned;
		memcpy(temp.spawn_parms, newcl->spawn_parms, sizeof(temp.spawn_parms));
		if (newcl->userid)
			temp.userid = newcl->userid;
	}
	else 	//client has no existing slot.
	{
#ifdef SUBSERVERS
		if (SSV_IsSubServer())
		{
			if (1)
			{
				sizebuf_t s;
				qbyte send[8192];

				memset(&s, 0, sizeof(s));
				s.data = send;
				s.maxsize = sizeof(send);
				s.cursize = 2;

				MSG_WriteByte(&s, ccmd_foundplayer);
				MSG_WriteString(&s, name);
				MSG_WriteString(&s, NET_AdrToString (adrbuf, sizeof(adrbuf), &info->adr));
				MSG_WriteString(&s, info->guid);
				SSV_InstructMaster(&s);
				return;
			}
			else
			{
				SV_RejectMessage (info->protocol, "Direct connections are not permitted.\n");
				Con_TPrintf ("* rejected direct connection\n");
				return;
			}
		}
#endif

		/*single player logic*/
		if (sv.allocated_client_slots == 1 && info->adr.type == NA_LOOPBACK)
			if (svs.clients[0].state >= cs_connected)
			{
				Con_Printf("Kicking %s to make space for local client\n", svs.clients[0].name);
				SV_DropClient(svs.clients);
			}

		// if at server limits, refuse connection
		if ( maxclients.ival > MAX_CLIENTS )
			Cvar_SetValue (&maxclients, MAX_CLIENTS);
		if (maxspectators.ival > MAX_CLIENTS)
			Cvar_SetValue (&maxspectators, MAX_CLIENTS);

		// find a free client slot
		for (i=0; i<sv.allocated_client_slots ; i++)
		{
			cl=svs.clients+i;
			if (cl->state == cs_free)
			{
				newcl = cl;
				break;
			}
		}

		/*only q1/h2 has a maxclients/maxspectators separation. q2 or q3 the gamecode enforces any such clienttype limits*/
		if (svprogfuncs)
		{
			if (spectator && spectators >= maxspectators.ival)
				redirect = true;
			if (!spectator && clients >= maxclients.ival)
				redirect = true;
		}
		else
		{
			if (clients >= maxclients.ival)
				redirect = true;
		}

		if (redirect)
		{
			extern cvar_t sv_fullredirect;
			if (!*sv_fullredirect.string)
				newcl = NULL;
		}

		if (!newcl)
		{
			if (!svprogfuncs)
			{
				SV_RejectMessage (info->protocol, "\nserver is full\n\n");
				Con_TPrintf ("%s:full connect\n", NET_AdrToString (adrbuf, sizeof(adrbuf), &info->adr));
			}
			else
			{
				if (spectator && spectators >= maxspectators.ival)
				{
					SV_RejectMessage (info->protocol, "\nserver is full (%i of %i spectators)\n\n", spectators, maxspectators.ival);
					Con_TPrintf ("%s:full connect (spectators)\n", NET_AdrToString (adrbuf, sizeof(adrbuf), &info->adr));
				}
				else if (!spectator && clients >= maxclients.ival)
				{
					SV_RejectMessage (info->protocol, "\nserver is full (%i of %i players)\n\n", clients, maxclients.ival);
					Con_TPrintf ("%s:full connect (players)\n", NET_AdrToString (adrbuf, sizeof(adrbuf), &info->adr));
				}
				else
				{
					SV_RejectMessage (info->protocol, "\nserver is full (%i of %i connections)\n\n", clients+spectators, sv.allocated_client_slots);
					Con_TPrintf ("%s:full connect\n", NET_AdrToString (adrbuf, sizeof(adrbuf), &info->adr));
				}
			}
			return;
		}
	}

	//set up the gamecode for this player, optionally drop them here
	edictnum = (newcl-svs.clients)+1;
	switch (svs.gametype)
	{
#ifdef HLSERVER
		//fallthrough
	case GT_HALFLIFE:
		{
			char reject[128];
			if (!SVHL_ClientConnect(newcl, adr, reject))
			{
				SV_RejectMessage(protocol, "%s", reject);
				Con_TPrintf ("%s:gamecode reject\n", NET_AdrToString (adrbuf, sizeof(adrbuf), &adr));
				return NULL;
			}
			temp.hledict = newcl->hledict;
		}

		break;
#endif


#ifdef VM_Q1
	case GT_Q1QVM:
#endif
#ifdef VM_LUA
	case GT_LUA:
#endif
	case GT_PROGS:
		if (info->protocol == SCP_QUAKE2)
		{
			SV_RejectMessage(info->protocol, "This is a %s server.", fs_manifest->formalname);
			Con_DPrintf ("* Rejected q2 client.\n");
			return;
		}

		if (svprogfuncs)
			ent = EDICT_NUM_UB(svprogfuncs, edictnum);
		else
			ent = NULL;
#ifdef Q2SERVER
		temp.q2edict = NULL;
#endif
		temp.edict = ent;

		{
			const char *reject = SV_CheckRejectConnection(&info->adr, info->seat[0].info, info->protocol, info->ftepext1, info->ftepext2, info->ezpext1, info->guid);
			if (reject)
			{
				SV_RejectMessage(info->protocol, "%s", reject);
				Con_DPrintf ("* Game rejected a connection.\n");
				return;
			}
		}

		break;

#ifdef Q2SERVER
	case GT_QUAKE2:
		if (info->protocol != SCP_QUAKE2 && info->protocol != SCP_QUAKE2EX)
		{
			SV_RejectMessage(info->protocol, "This is a %s server.", fs_manifest->formalname);
			Con_DPrintf ("* Rejected non-q2 client.\n");
			return;
		}
		q2ent = Q2EDICT_NUM(edictnum);
		temp.edict = NULL;
		temp.q2edict = q2ent;

		if (!ge->ClientConnect(q2ent, info->seat[0].info))
		{
			const char *reject = Info_ValueForKey(info->seat[0].info, "rejmsg");
			if (*reject)
				SV_RejectMessage(info->protocol, "%s\nConnection Refused.", reject);
			else
				SV_RejectMessage(info->protocol, "Connection Refused.");
			Con_DPrintf ("Game rejected a connection.\n");
			return;
		}

		ge->ClientUserinfoChanged(q2ent, info->seat[0].info);


		break;
#endif
	default:
		Sys_Error("Bad svs.gametype in SVC_DirectConnect");
		break;
	}

	if (newcl->frameunion.frames)
	{
		Con_Printf("Client frame info was set\n");
		Z_Free(newcl->frameunion.frames);
	}

	temp.name = newcl->name;
	temp.team = newcl->team;

	InfoSync_Clear(&newcl->infosync);
	*newcl = temp;
	newcl->userinfo.ChangeCB = svs.info.ChangeCB;
	newcl->userinfo.ChangeCTX = &newcl->userinfo;
	InfoBuf_FromString(&newcl->userinfo, info->seat[0].info, false);

	newcl->challenge = info->challenge;
	newcl->zquake_extensions = atoi(InfoBuf_ValueForKey(&newcl->userinfo, "*z_ext"));
	InfoBuf_SetStarKey(&newcl->userinfo, "*z_ext", "");
	if (*InfoBuf_ValueForKey(&newcl->userinfo, "*fuhquake"))	//fuhquake doesn't claim to support z_ext but does look at our z_ext serverinfo key.
	{														//so switch on the bits that it should be sending.
		newcl->zquake_extensions |= Z_EXT_PM_TYPE|Z_EXT_PM_TYPE_NEW;
	}
	newcl->zquake_extensions &= SERVER_SUPPORTED_Z_EXTENSIONS;

	ncflags = NCF_SERVER;
	if (info->mtu)
		ncflags |= NCF_FRAGABLE;
	if (info->ftepext2&PEXT2_STUNAWARE)
		ncflags |= NCF_STUNAWARE;
	Netchan_Setup (ncflags, &newcl->netchan, &info->adr, info->qport,
					info->mtu?info->mtu:atoi(Info_ValueForKey (info->seat[0].info, "mtu")));

#ifdef HUFFNETWORK
	if (info->huffcrc)
		newcl->netchan.compresstable = Huff_CompressionCRC(info->huffcrc);
	else
#endif
		newcl->netchan.compresstable = NULL;

	newcl->protocol = info->protocol;
#ifdef NQPROT
	newcl->netchan.isnqprotocol = ISNQCLIENT(newcl);
#endif

	newcl->state = cs_connected;
	newcl->connection_started = realtime;

#ifdef Q3SERVER
	newcl->gamestatesequence = -1;
#endif
	newcl->datagram.allowoverflow = true;
	newcl->datagram.data = newcl->datagram_buf;
	if (newcl->netchan.flags&NCF_FRAGABLE)
		newcl->datagram.maxsize = sizeof(newcl->datagram_buf);
	else
		newcl->datagram.maxsize = MAX_DATAGRAM;

#ifdef Q2SERVER
	if (newcl->protocol == SCP_QUAKE2EX)
	{
		newcl->netchan.netprim =
		newcl->datagram.prim =
		newcl->backbuf.prim =
		newcl->netchan.message.prim = sv.q2multicast[1].prim;
	}
	else
#endif
	{
		newcl->netchan.netprim =
		newcl->datagram.prim =
		newcl->backbuf.prim =
		newcl->netchan.message.prim = svs.netprim;
	}

	SV_ClientProtocolExtensionsChanged(newcl);

	// spectator mode can ONLY be set at join time
	newcl->spectator = spectator;

	newcl->realip_ping = (((rand()^(rand()<<8) ^ *(int*)&realtime)&0xffffff)<<8) | (newcl-svs.clients);

#ifdef HEXEN2
	if (newcl->istobeloaded && newcl->edict)
		newcl->playerclass = newcl->edict->xv->playerclass;
#endif

	// parse some info from the info strings
	SV_ExtractFromUserinfo (newcl, true);

	// JACK: Init the floodprot stuff.
	newcl->floodprotmessage = 0.0;
	newcl->lastspoke = 0.0;
	newcl->lockedtill = 0;

#ifdef SVRANKING
//rankid is figured out in extract from user info
	if (!newcl->rankid)	//failed to get a userid
	{
		if (rank_needlogin.value)
		{
			SV_RejectMessage (info->protocol, "Bad password/username\nThis server requires logins. Please see the serverinfo for website and info on how to register.\n");
			newcl->state = cs_free;
			return;
		}

//			SV_OutOfBandPrintf (isquake2client, adr, "\nWARNING: You have not got a place on the ranking system, probably because a user with the same name has already connected and your pwds differ.\n\n");

		if (!preserveparms)
			SV_GetNewSpawnParms(newcl);
	}
	else
	{
		rankstats_t rs;
		if (!Rank_GetPlayerStats(newcl->rankid, &rs))
		{
			SV_RejectMessage (info->protocol, "Rankings/Account system failed\n");
			Con_TPrintf("banned player %s is trying to connect\n", newcl->name);
			newcl->name[0] = 0;
			InfoBuf_Clear(&newcl->userinfo, true);
			newcl->state = cs_free;
			return;
		}

		if (rs.flags1 & RANK_MUTED)
		{
			SV_BroadcastTPrintf(PRINT_MEDIUM, "%s is muted (still)\n", newcl->name);
		}
		if (rs.flags1 & RANK_CUFFED)
		{
			SV_BroadcastTPrintf(PRINT_LOW, "%s is now cuffed permanently\n", newcl->name);
		}
		if (rs.flags1 & RANK_CRIPPLED)
		{
			SV_BroadcastTPrintf(PRINT_HIGH, "%s is still crippled\n", newcl->name);
		}

		if (rs.timeonserver)
		{
			if (preserveparms)
			{	//do nothing.
			}
			else if (sv_resetparms.value)
			{
				SV_GetNewSpawnParms(newcl);
			}
			else
			{
				extern cvar_t rank_parms_first, rank_parms_last;
				for (i=0 ; i<NUM_SPAWN_PARMS ; i++)
				{
					if (i < NUM_RANK_SPAWN_PARMS && i >= rank_parms_first.ival && i <= rank_parms_last.ival)
						newcl->spawn_parms[i] = rs.parm[i];
					else
						newcl->spawn_parms[i] = 0;
				}
			}

			if (rs.timeonserver > 3*60)	//woo. Ages.
				s = va(langtext("Welcome back %s. You have previously spent %i:%i hours connected\n", newcl->language), newcl->name, (int)(rs.timeonserver/(60*60)), (int)((int)(rs.timeonserver/60)%(60)));
			else	//measure this guy in minuites.
				s = va(langtext("Welcome back %s. You have previously spent %i mins connected\n", newcl->language), newcl->name, (int)(rs.timeonserver/60));

			SV_OutOfBandPrintf (ISQ2CLIENT(info), &info->adr, s);
		}
		else if (!preserveparms)
		{
			SV_GetNewSpawnParms(newcl);

			SV_OutOfBandTPrintf (ISQ2CLIENT(info), &info->adr, newcl->language, "Welcome %s. Your time on this server is being logged and ranked\n", newcl->name, (int)rs.timeonserver);
		}
		//else loaded players already have their initial parms set
	}
#else
	// call the progs to get default spawn parms for the new client
	if (!preserveparms)
	{
		SV_GetNewSpawnParms(newcl);
	}
#endif


	SV_AcceptMessage (newcl);

	newcl->state = cs_free;
	if (redirect || preserveparms)
	{
	}
	else if (ISNQCLIENT(newcl))
	{
		//FIXME: we should delay this until we actually have a name, because right now they'll be called unnamed or unconnected or something
		SV_BroadcastPrintf(PRINT_LOW, "New client connected\n");
	}
	else if (newcl->spectator)
	{
		SV_BroadcastTPrintf(PRINT_LOW, "spectator %s connected\n", newcl->name);
//			Con_Printf ("Spectator %s connected\n", newcl->name);
	}
	else
	{
		SV_BroadcastTPrintf(PRINT_LOW, "client %s connected\n", newcl->name);
//			Con_DPrintf ("Client %s connected\n", newcl->name);
	}
	newcl->state = cs_connected;
	newcl->sendinfo = true;

	if (!redirect)
	{
		for (i = 0; i < sizeof(sv_motd)/sizeof(sv_motd[0]); i++)
		{
			if (*sv_motd[i].string)
				SV_ClientPrintf(newcl, PRINT_CHAT, "%s\n", sv_motd[i].string);
		}
	}

	SV_CheckRecentCrashes(newcl);

#ifdef VOICECHAT
	SV_VoiceInitClient(newcl);
#endif

	SV_EvaluatePenalties(newcl);

	if (newcl->penalties & BAN_SPECONLY)
	{
		if (spectators >= maxspectators.ival)
			newcl->drop = true;	//oops.
		newcl->spectator = spectator = true;
		InfoBuf_SetValueForStarKey (&cl->userinfo, "*spectator", "1");
	}

	//only advertise PEXT_SPLITSCREEN when splitscreen is allowed, to avoid spam. this might mean people need to reconnect after its enabled. oh well.
	if (!sv_allow_splitscreen.ival && newcl->netchan.remote_address.type != NA_LOOPBACK)
		newcl->fteprotocolextensions &= ~PEXT_SPLITSCREEN;
	newcl->controller = NULL;

	if (!redirect)
	{
		Sys_ServerActivity();

		PIN_ShowMessages(newcl);
	}

#ifdef NQPROT
	if (ISNQCLIENT(newcl))
	{
		newcl->netchan.message.maxsize = sizeof(newcl->netchan.message_buf);
		host_client = newcl;
		SVNQ_New_f();
	}
#endif

	newcl->redirect = redirect;

#ifdef SUBSERVERS
	SSV_SavePlayerStats(newcl, 0);
#endif

#ifdef IPLOG
	if (Q_strncasecmp(newcl->name, "unconnected", 11) && Q_strncasecmp(newcl->name, "connecting", 10))
		IPLog_Add(NET_AdrToString(adrbuf,sizeof(adrbuf), &newcl->netchan.remote_address), newcl->name);
#endif

#ifdef NQPROT
	newcl->netchan.incoming_reliable_sequence = info->expectedreliablesequence;
#endif

	if (sv_use_dns.ival)
		NET_AdrToStringResolve(&info->adr, SV_UserDNSResolved, NULL, newcl-svs.clients, newcl->userid);

	for (i = 1; i < info->seats; i++)
		SV_AddSplit(newcl, info->seat[i].info, i);
}

#ifdef Q2SERVER
void Q2EFixupInfo(void *vctx, const char *key, const char *value)
{
	int seat = -1;
	svconnectinfo_t *ctx = vctx;
	int l = strlen(key);
	if (l > 2 && key[l-2] == '_' && key[l-1] >= '0' && key[l-1] < '0'+MAX_SPLITS)
	{
		seat = key[l-1]-'0';
		((char*)key)[l-2] = 0;	//strip out the annoying _0s
	}
	if (seat < 0)
	{
		for(seat = 0; seat < ctx->seats; seat++)
			Info_SetValueForStarKey(ctx->seat[seat].info, key, value, sizeof(ctx->seat[seat].info));
	}
	else
		Info_SetValueForStarKey(ctx->seat[seat].info, key, value, sizeof(ctx->seat[seat].info));
}
#endif

/*
==================
SVC_DirectConnect

A connection request that did not come from the master
==================
arguments must be tokenized first
Q3: connect "\key\val"
DP: connect\key\val
QW: connect $VER $QPORT $CHALLENGE "\key\val"
NQ: hacked to take the form of QW, but with protocol version 3.
UNSUPPORTED FTEQW/Splitscreen: connect2 $VER $QPORT $CHALLENGE "\key\val" "\key\val"
extension flags follow it.
*/
void SVC_DirectConnect(int expectedreliablesequence)
{
	int			version;
#ifdef HUFFNETWORK
	extern cvar_t net_compress;
#endif

	svconnectinfo_t info;
#ifdef NQPROT
	extern cvar_t sv_protocol_nq;
	info.proquakeanglehack = false;
	info.isqex = false;
	info.supportedprotocols = 0;
	info.expectedreliablesequence = expectedreliablesequence;
#endif

#ifdef HUFFNETWORK
	info.huffcrc = 0;
#endif
	info.mtu = 0;
	info.ftepext1 = 0;
	info.ftepext2 = 0;
	info.ezpext1 = 0;
	*info.guid = 0;
	info.seats = 1;
	if (*Cmd_Argv(1) == '\\')
	{	//q3: connect "\key\val"
#ifndef QWOVERQ3
		SV_RejectMessage (SCP_QUAKE3, "This is not a q3 server: %s\n", version_string());
		Con_TPrintf ("* rejected connect from q3 client\n");
		return;
#else
		const char		*s;
		//this is used by q3 (note, we already decrypted the huffman connection packet in a hack)
		if (!sv_listen_q3.ival)
		{
			SV_RejectMessage (SCP_QUAKE3, "Server is not accepting quake3 clients at this time: %s\n", version_string());
			Con_TPrintf ("* rejected connect from q3 client\n");
			return NULL;
		}
		numssclients = 1;
		protocol = SCP_QUAKE3;

		Q_strncpyz (userinfo, Cmd_Argv(1), sizeof(userinfo)-1);

		switch (atoi(Info_ValueForKey(userinfo, "protocol")))
		{
		case 68:	//regular q3 1.32
			break;
//		case 43:	//q3 1.11 (most 'recent' demo)
//			break;
		default:
			SV_RejectMessage (SCP_BAD, "Server is %s.\n", version_string());
			Con_TPrintf ("* rejected connect from incompatable client\n");
			return NULL;
		}

		s = Info_ValueForKey(userinfo, "challenge");
		challenge = atoi(s);

		s = Info_ValueForKey(userinfo, "qport");
		qport = atoi(s);

		s = Info_ValueForKey(userinfo, "name");
		if (!*s)
			Info_SetValueForKey(userinfo, "name", "UnnamedQ3", sizeof(userinfo));

#ifdef HUFFNETWORK
		info.huffcrc = HUFFCRC_QUAKE3;
#endif
#endif
	}
#ifdef NQPROT
	else if (*(Cmd_Argv(0)+7) == '\\')
	{	//DP has the userinfo attached directly to the end of the connect command
		const char		*s;
		if (!sv_listen_dp.value && net_from.type != NA_LOOPBACK)
		{
			if (!sv_listen_nq.value)
				SV_RejectMessage (SCP_DARKPLACES6, "Server is not accepting darkplaces clients at this time.\n", version_string());
			Con_TPrintf ("* rejected connect from dp client\n");
			return;
		}
		if (progstype == PROG_H2)
		{
			if (!sv_listen_nq.value)
				SV_RejectMessage (SCP_DARKPLACES6, "NQ protocols are not supported with hexen2 gamecode.\n", version_string());
			Con_TPrintf ("* rejected connect from dp client (because of hexen2)\n");
			return;
		}
		Q_strncpyz (info.seat[0].info, net_message.data + 11, sizeof(info.seat[0].info)-1);

		if (strcmp(Info_ValueForKey(info.seat[0].info, "protocol"), "darkplaces "STRINGIFY(NQ_NETCHAN_VERSION)))
		{
			SV_RejectMessage (SCP_BAD, "Server is %s.\n", version_string());
			Con_TPrintf ("* rejected connect from incompatible client\n");
			return;
		}
		//it's a darkplaces client.

		s = Info_ValueForKey(info.seat[0].info, "protocols");

		while(s && *s)
		{
			static const struct
			{
				char *name;
				unsigned int bits;
			} dpnames[] =
			{
				{"FITZ",			1u<<SCP_FITZ666},	//dp doesn't support this, but this is for potential compat if other engines use this handshake
				{"666",				1u<<SCP_FITZ666},	//dp doesn't support this, but this is for potential compat if other engines use this handshake
				{"RMQ",				1u<<SCP_FITZ666},	//fte doesn't distinguish, but assumes clients will support both
				{"999",				1u<<SCP_FITZ666},	//fte doesn't distinguish, but assumes clients will support both
				{"DP8",				0},	//unsupported, cloudwalk's attempt to fix some stuff (still unfinalised).
				{"DP7",				1u<<SCP_DARKPLACES7},
				{"DP6",				1u<<SCP_DARKPLACES6},
				{"DP5",				0},	//unsupported serverside
				{"DP4",				0},	//unsupported
				{"DP3",				0},	//unsupported
				{"DP2",				0},	//unsupported
				{"DP1",				0},	//unsupported
				{"QW",				0},	//mixing protocols doesn't make sense, and would just confuse the client.
				{"QUAKEDP",			1u<<SCP_NETQUAKE},
				{"QUAKE",			1u<<SCP_NETQUAKE},
				{"NEHAHRAMOVIE",	1u<<SCP_NETQUAKE},
				{"NEHAHRABJP",		0},
				{"NEHAHRABJP2",		0},
				{"NEHAHRABJP3",		1u<<SCP_BJP3},
				{"DP7DP6",			(1u<<SCP_DARKPLACES7)|(1u<<SCP_DARKPLACES6)},	//stupid shitty buggy crappy client
				{"DP8DP7",			/*(1u<<SCP_DARKPLACES8)|*/(1u<<SCP_DARKPLACES7)},	//stupid shitty buggy crappy client
			};
			int p;

			s = COM_Parse(s);
			for (p = 0; p < countof(dpnames); p++)
			{
				if (!Q_strcasecmp(dpnames[p].name, com_token))
				{
					info.supportedprotocols |= dpnames[p].bits;
					break;
				}
			}
			if (p == countof(dpnames))
				Con_DPrintf("'DP' client reporting unknown protocol \"%s\"\n", com_token);
		}

		info.protocol = SCP_DARKPLACES7;

		s = Info_ValueForKey(info.seat[0].info, "challenge");
		if (!strncmp(s, "FTE", strlen("FTE")))	//cope with our mangling of the challenge.
			info.challenge = atoi(s+strlen("FTE"));
		else
			info.challenge = atoi(s);

		Info_RemoveKey(info.seat[0].info, "protocol");
		Info_RemoveKey(info.seat[0].info, "protocols");
		Info_RemoveKey(info.seat[0].info, "challenge");

		s = Info_ValueForKey(info.seat[0].info, "name");
		if (!*s)
			Info_SetValueForKey(info.seat[0].info, "name", "CONNECTING", sizeof(info.seat[0].info));

		info.qport = 0;
		info.proquakeanglehack = false;	//NOTE: DP clients fuck up here due to a DP client bug.
									//DP clients will use 16bit angles if it has previously connected to a proquake-handshake server,
									//and 8bit angles otherwise (or a non-proquake/non-dp/non-qw server more recently than the proquake one).
	}
#endif
#ifdef Q2SERVER
	else if (atoi(Cmd_Argv(1)) == PROTOCOL_VERSION_Q2EX)
	{
		int i, numseats;
		info.seats = numseats = atoi(Cmd_Argv(2));
		version = atoi(Cmd_Argv(1));
		info.protocol = SCP_QUAKE2EX;
		info.seats = bound(1, numseats, countof(info.seat));
		if (info.seats < 1 || info.seats > MAX_SPLITS)
		{	//no splitscreen(yet)
			SV_RejectMessage (info.protocol, "bad number of seats\n", PROTOCOL_VERSION_Q2, version);
			Con_TPrintf ("* rejected connect from version %i\n", version);
			return;
		}
		Q_strncpyz(info.guid, Cmd_Argv(3), sizeof(info.guid));
		//Socials - Cmd_Argv(3 ... 3+numseats-1); fuck that spyware
		for (i = 0; i < info.seats; i++)
			*info.seat[i].info = 0;
		Info_Enumerate(va("\\%s", Cmd_Argv(3+numseats)), &info, Q2EFixupInfo);

		//hide a challenge in there, in case we're not using the lobby stuff.
		info.challenge = atoi(Info_ValueForKey(info.seat[0].info, "challenge"));
		info.qport = atoi(Info_ValueForKey(info.seat[0].info, "qport"));
		for (i = 0; i < info.seats; i++)
		{
			Info_RemoveKey(info.seat[i].info, "challenge");
			Info_RemoveKey(info.seat[i].info, "qport");
		}
	}
#endif
	else
	{
		//fte: connectN is no longer supported (multiple userinfos packed into a single packet was a bad idea when userinfos can be so large
		/*if (atoi(Cmd_Argv(0)+7))
		{
			int numssclients = atoi(Cmd_Argv(0)+7);
			if (numssclients!=1)
			{
				SV_RejectMessage (SCP_BAD, "Server is %s.\n", version_string());
				Con_TPrintf ("* rejected connect from old client\n");
				return;
			}
		}*/

		version = atoi(Cmd_Argv(1));
		if (version >= PROTOCOL_VERSION_Q2_MIN && version <= PROTOCOL_VERSION_Q2)
			info.protocol = SCP_QUAKE2;
#ifdef NQPROT
		else if (version == NQ_NETCHAN_VERSION)
		{
			info.protocol = SCP_NETQUAKE; //because we can
			switch(atoi(Info_ValueForKey(Cmd_Argv(4), "mod")))
			{
			case 1:
				info.proquakeanglehack = true;
				break;
			case PROTOCOL_VERSION_FITZ:
			case PROTOCOL_VERSION_RMQ:
				info.protocol = SCP_FITZ666;
				break;
			case PROTOCOL_VERSION_BJP3:
				info.protocol = SCP_BJP3;
				info.proquakeanglehack = true;
				break;
			}
		}
		else if (version == NQ_NETCHAN_VERSION_QEX)
		{	//rerelease...
			info.protocol = SCP_FITZ666;//NETQUAKE;
			info.isqex = true;
		}
#endif
		else if (version == PROTOCOL_VERSION_QW)
			info.protocol = SCP_QUAKEWORLD;
		else
		{
			SV_RejectMessage (SCP_BAD, "Server is protocol version %i, received %i\n", PROTOCOL_VERSION_QW, version);
			Con_TPrintf ("* rejected connect from version %i\n", version);
			return;
		}

		info.qport = atoi(Cmd_Argv(2));

		info.challenge = atoi(Cmd_Argv(3));

		// note an extra qbyte is needed to replace spectator key
		Q_strncpyz (info.seat[0].info, Cmd_Argv(4), sizeof(info.seat[0].info)-1);
		if (info.protocol >= SCP_NETQUAKE)
		{
			Info_RemoveKey(info.seat[0].info, "mod");	//its served its purpose.
			Info_RemoveKey(info.seat[0].info, "modver");	//its served its purpose.
			Info_RemoveKey(info.seat[0].info, "flags");	//its served its purpose.
		}
	}

#ifdef HAVE_DTLS
	if (net_enable_dtls.ival > 2 && (net_from.prot == NP_DGRAM || net_from.prot == NP_STREAM || net_from.prot == NP_WS) && net_from.type != NA_LOOPBACK && !NET_IsEncrypted(&net_from))
	{
		SV_RejectMessage (info.protocol, "This server requires the use of DTLS/TLS/WSS.\n");
		return;
	}
#endif

	{
		char *banreason = SV_BannedReason(&net_from);
		if (banreason)
		{
			if (*banreason)
				SV_RejectMessage (info.protocol, "You were banned.\nReason: %s\n", banreason);
			else
				SV_RejectMessage (info.protocol, "You were banned.\n");
			return;
		}
	}

	if (info.protocol == SCP_QUAKEWORLD)	//readd?
	{
		if (!sv_listen_qw.value && net_from.type != NA_LOOPBACK)
		{
			SV_RejectMessage (info.protocol, "QuakeWorld protocols are not permitted on this server.\n");
			Con_TPrintf ("* rejected connect from quakeworld\n");
			return;
		}
	}

	// see if the challenge is valid.
	if (net_from.type == NA_LOOPBACK)	//normal rules don't apply
		;
	else if (net_from.prot != NP_DGRAM)
		;	//challenge checks are irrelevant when we've alread passed a challenge in a lower network layer.
	else
	{
		if (!SV_ChallengePasses(info.challenge))
		{
			if (sv_listen_dp.ival && !info.challenge && info.protocol == SCP_QUAKEWORLD)
			{
				//dp replies with 'challenge'. which vanilla quakeworld interprets as: c<CHALLENGEID><ignored junk 'hallenge'>
				//so just silence that error.
				return;
			}
			SV_RejectMessage (info.protocol, "Bad challenge.\n");
			return;
		}
	}

	if (sv_banproxies.ival)
	{
		//FIXME: allow them to spectate but not join
		if (*Info_ValueForKey(info.seat[0].info, "*qwfwd"))
		{
			SV_RejectMessage (info.protocol, "Proxies are not permitted on this server.\n");
			Con_TPrintf ("* rejected connect from qwfwd proxy\n");
			return;
		}
		if (*Info_ValueForKey(info.seat[0].info, "Qizmo"))
		{
			SV_RejectMessage (info.protocol, "Proxies are not permitted on this server.\n");
			Con_TPrintf ("* rejected connect from qizmo proxy\n");
			return;
		}
		if (*Info_ValueForKey(info.seat[0].info, "*qtv"))
		{
			SV_RejectMessage (info.protocol, "Proxies are not permitted on this server.\n");
			Con_TPrintf ("* rejected connect from qtv proxy (udp)\n");
			return;
		}
	}

	while(!msg_badread)
	{
		Cmd_TokenizeString(MSG_ReadStringLine(), false, false);
		switch(Q_atoi(Cmd_Argv(0)))
		{
		case PROTOCOL_VERSION_FTE1:
			if (info.protocol == SCP_QUAKEWORLD || info.protocol == SCP_QUAKE2)
			{
				info.ftepext1 = Q_atoi(Cmd_Argv(1));
				Con_DPrintf("Client supports 0x%x fte extensions\n", info.ftepext1);
			}
			break;
		case PROTOCOL_VERSION_FTE2:
			if (info.protocol == SCP_QUAKEWORLD)
			{
				info.ftepext2 = Q_atoi(Cmd_Argv(1));
				Con_DPrintf("Client supports 0x%x fte2 extensions\n", info.ftepext2);
			}
			break;
		case PROTOCOL_VERSION_EZQUAKE1:
			if (info.protocol == SCP_QUAKEWORLD)
			{
				info.ezpext1 = Q_atoi(Cmd_Argv(1));
				Con_DPrintf("Client supports 0x%x ez1 extensions\n", info.ezpext1);
			}
			break;
		case PROTOCOL_VERSION_HUFFMAN:
#ifdef HUFFNETWORK
			info.huffcrc = Q_atoi(Cmd_Argv(1));
			Con_DPrintf("Client supports huffman compression. crc 0x%x\n", info.huffcrc);
			if (!net_compress.ival || !Huff_CompressionCRC(info.huffcrc))
			{
				SV_RejectMessage (info.protocol, "Compression should not have been enabled.\n");	//buggy/exploiting client. can also happen from timing when changing the setting, but whatever
				Con_TPrintf ("* rejected - bad compression state\n");
				return;
			}
#endif
			break;
		case PROTOCOL_VERSION_FRAGMENT:
			info.mtu = Q_atoi(Cmd_Argv(1)) & ~7;
			if (info.mtu < 64)
				info.mtu = 0;
			Con_DPrintf("Client supports fragmentation. mtu %i.\n", info.mtu);
			break;
		case PROTOCOL_INFO_GUID:
			Q_strncpyz(info.guid, Cmd_Argv(1), sizeof(info.guid));
			Con_DPrintf("GUID %s\n", Cmd_Argv(1));
			break;
		}
	}
	msg_badread=false;

	info.adr = net_from;
	if (MSV_ClusterLogin(&info))
		return;

	SV_DoDirectConnect(&info);
}

static int dehex(int i)
{
	if      (i >= '0' && i <= '9')
		return (i-'0');
	else if (i >= 'A' && i <= 'F')
		return (i-'A'+10);
	else
		return (i-'a'+10);
}
static qboolean Rcon_Validate (void)
{
	/*
	   The rcon protocol sucks.
	   1) vanilla sent it plain text
	   2) there's no challenge, so there's no way to block spoofed requests
	   3) the hashed version of the protocol still has no challenge
	*/
	const char *realpass = rcon_password.string;
	const char *pass = Cmd_Argv(1);
	if (!strlen (realpass))
		return false;

	if (!sv_crypt_rcon.ival || !*sv_crypt_rcon.string)
	{	//vanilla-compatible
		if (!strcmp (pass, realpass) )
			return true;
	}
	if (sv_crypt_rcon.ival || !*sv_crypt_rcon.string)
	{	//ezquake-compatible
		//the password arg is "[SHA1Digest][unixtime-in-hex]" where the digest is "[arg0] password[unixtime] arg0 arg1 argn "
		time_t clienttime = 0;
		time_t servertime = 0;
		intptr_t timediff;
		qbyte b;

		const hashfunc_t *hashfunc = &hash_sha1;
		void *hashctx = alloca(hashfunc->contextsize);

		const size_t digestsize = 20;
		size_t i, k;
		unsigned char digest[512];
		const unsigned char **tokens = alloca(sizeof(*tokens)*(Cmd_Argc()*2+5));	//overallocation in case argc is 0.
		if (strlen(pass) > digestsize*2)
		{
			for (i = 0; pass[digestsize*2+i] && i < sizeof(time_t)*2; i++)
			{	//mixed endian for compat with ezquake
				if (!(i & 1))
					clienttime |= dehex(pass[digestsize*2+i]) << (((i/2)*8)+4);
				else
					clienttime |= dehex(pass[digestsize*2+i]) << (((i/2)*8)+0);
			}
			time(&servertime);
			timediff = servertime-clienttime;
			//make sure the client's time is within our allowed bounds, to prevent (extreme) replay attacks.
			if (!sv_crypt_rcon_clockskew.value || (timediff >= -sv_crypt_rcon_clockskew.ival && timediff <= sv_crypt_rcon_clockskew.ival))
			{
				tokens[0] = Cmd_Argv(0);
				tokens[1] = " ";
				tokens[2] = realpass;
				tokens[3] = pass+digestsize*2;
				tokens[4] = " ";
				for (i = 0; i < Cmd_Argc()-2; i++)
				{
					tokens[5+i*2+0] = Cmd_Argv(i+2);
					tokens[5+i*2+1] = " ";	//a trailing space is required.
				}
				hashfunc->init(hashctx);
				for (k = 0; k < 5+i*2; k++)
					hashfunc->process(hashctx, tokens[k], strlen(tokens[k]));
				hashfunc->terminate(digest, hashctx);

				for (i = 0;;i++)
				{
					if (i == digestsize)
						return true;
					if (!pass[i*2+0] || !pass[i*2+1])
						break;	//premature termination
					b = dehex(pass[i*2+0])*16+dehex(pass[i*2+1]);
					if (b != digest[i])
						break;
				}
			}
		}
	}
	return false;
}

/*
===============
SVC_RemoteCommand

A client issued an rcon command.
Shift down the remaining args
Redirect all printfs
===============
*/
void SVC_RemoteCommand (void)
{
	int		i;
	char	remaining[1024];
	char	adr[MAX_ADR_SIZE];
	static unsigned int blockuntil;
	unsigned int curtime, inc = 1000/sv_rconlim.value;

	{
		char *br = SV_BannedReason(&net_from);
		if (br)
		{
			Con_TPrintf ("Rcon from banned ip %s: %s\n", NET_AdrToString (adr, sizeof(adr), &net_from), br);
			return;
		}
	}

	if (sv_rconlim.value > 0)
	{
		curtime = Sys_Milliseconds();
		if (1000 < curtime - blockuntil)
			blockuntil = curtime - 1000;
		if (inc > curtime-blockuntil)
			return;	//throttle
	}

	if (!Rcon_Validate())
	{
		blockuntil += inc;
/*
#ifdef SVRANKING
		if (cmd_allowaccess.value)	//try and find a username, match the numeric password
		{
			int rid;
			char *s = Cmd_Argv(1);
			char *colon=NULL, *c2;
			rankstats_t stats;
			c2=s;
			for(;;)
			{
				c2 = strchr(c2, ':');
				if (!c2)
					break;
				colon = c2;
				c2++;
			}
			if (colon)	//oh, could this be a specific username?
			{
				*colon = '\0';
				colon++;
				rid = Rank_GetPlayerID(NULL, s, atoi(colon), false, true);
				if (rid)
				{
					if (!Rank_GetPlayerStats(rid, &stats))
						return;


					Con_TPrintf ("Rcon from %s:\n%s\n"
						, NET_AdrToString (adr, sizeof(adr), &net_from), net_message.data+4);

					SV_BeginRedirect (RD_PACKET_LOG, svs.language);

					remaining[0] = 0;

					for (i=2 ; i<Cmd_Argc() ; i++)
					{
						if (strlen(remaining)+strlen(Cmd_Argv(i))>=sizeof(remaining)-2)
						{
							Con_TPrintf("Rcon was too long\n");
							SV_EndRedirect ();
							Con_TPrintf ("Rcon from %s:\n%s\n"
								, NET_AdrToString (adr, sizeof(adr), &net_from), "Was too long - possible buffer overflow attempt");
							return;
						}
						strcat (remaining, Cmd_Argv(i) );
						strcat (remaining, " ");
					}

					Cmd_ExecuteString (remaining, stats.trustlevel);

					SV_EndRedirect ();
					return;
				}
			}
		}
#endif
*/

		Log_String(LOG_RCON, va("Bad rcon from %s:\t%s\n"
			, NET_AdrToString (adr, sizeof(adr), &net_from), net_message.data+4));

		Con_TPrintf ("Bad rcon from %s:\t%s\n"
			, NET_AdrToString (adr, sizeof(adr), &net_from), net_message.data+4);

		if (1)
			return;

		SV_BeginRedirect (RD_PACKET, com_language);

		Con_TPrintf ("Bad rcon_password. Passwords might be logged. Be careful.\n");
	}
	else
	{
		//make sure stuff is flushed
		cmd_blockwait = true;
		Cbuf_ExecuteLevel(rcon_level.ival);
		cmd_blockwait = false;

		Log_String(LOG_RCON, va("\n\nRcon from %s:\t%s\n"
			, NET_AdrToString (adr, sizeof(adr), &net_from), net_message.data+4));

		Con_TPrintf ("Rcon from %s:\t%s\n"
			, NET_AdrToString (adr, sizeof(adr), &net_from), net_message.data+4);

		SV_BeginRedirect (RD_PACKET_LOG, com_language);

		remaining[0] = 0;

		for (i=2 ; i<Cmd_Argc() ; i++)
		{
			if (strlen(remaining)+strlen(Cmd_Argv(i))>=sizeof(remaining)-2)
			{
				Con_TPrintf("Rcon was too long\n");
				SV_EndRedirect ();
				Con_TPrintf ("Rcon from %s:\t%s\n"
					, NET_AdrToString (adr, sizeof(adr), &net_from), "Was too long - possible buffer overflow attempt");
				return;
			}
			Q_strncatz(remaining, Cmd_Argv(i), sizeof(remaining));
			Q_strncatz(remaining, " ", sizeof(remaining));
		}

		//make sure the wait command can't be used to fuck up our logs.
		cmd_blockwait = true;
		Cbuf_AddText(remaining, rcon_level.ival);
		Cbuf_ExecuteLevel(rcon_level.ival);
		cmd_blockwait = false;
	}

	SV_EndRedirect ();
}

void SVC_RealIP (void)
{
	unsigned int slotnum;
	int cookie;
	char *banreason;
	char adr[MAX_ADR_SIZE];

	slotnum = atoi(Cmd_Argv(1));
	cookie = atoi(Cmd_Argv(2));

	if (slotnum >= svs.allocated_client_slots)
	{
		//a malitious user
		return;
	}

	if (cookie != svs.clients[slotnum].realip_num)
	{
		//could be someone trying to kick someone else
		//so we can't kick, as much as we might like to.
		return;
	}

	if (svs.clients[slotnum].realip_status)
		return;

	if (NET_AddressSmellsFunny(&net_from))
	{
		Con_TPrintf("funny realip address: %s, ", NET_AdrToString(adr, sizeof(adr), &net_from));
		Con_TPrintf("proxy address: %s\n", NET_AdrToString(adr, sizeof(adr), &svs.clients[slotnum].netchan.remote_address));
		return;
	}

	banreason = SV_BannedReason(&net_from);
	if (banreason)
	{
		Con_TPrintf("%s has a banned realip\n", svs.clients[slotnum].name);
		if (*banreason)
			SV_ClientTPrintf(&svs.clients[slotnum], PRINT_CHAT, "You were banned.\nReason: %s\n", banreason);
		else
			SV_ClientTPrintf(&svs.clients[slotnum], PRINT_CHAT, "You were banned.\n");
		SV_DropClient(&svs.clients[slotnum]);
		return;
	}

	svs.clients[slotnum].realip_status = 1;
	svs.clients[slotnum].realip = net_from;
}

void SVC_ACK (void)
{
	int slotnum;
	char adr[MAX_ADR_SIZE];

	for (slotnum = 0; slotnum < svs.allocated_client_slots; slotnum++)
	{
		if (svs.clients[slotnum].state)
		{
			if (svs.clients[slotnum].realip_status == 1 && NET_CompareAdr(&svs.clients[slotnum].realip, &net_from))
			{
				if (!*Cmd_Argv(1))
					svs.clients[slotnum].realip_status = 2;
				else if (atoi(Cmd_Argv(1)) == svs.clients[slotnum].realip_ping &&
						atoi(Cmd_Argv(2)) == svs.clients[slotnum].realip_num)
				{
					svs.clients[slotnum].realip_status = 3;
				}
				else
				{
					Netchan_OutOfBandPrint(NCF_SERVER, &net_from, "realip not accepted. Please stop hacking.\n");
				}
				return;
			}
		}
	}
	Con_TPrintf (S_COLOR_GRAY"A2A_ACK from %s\n", NET_AdrToString (adr, sizeof(adr), &net_from));
}

#ifdef SUPPORT_ICE
void SVC_ICE_Offer(void);
void SVC_ICE_Candidate(void);
#endif

//returns false to block replies
//this is to mitigate wasted bandwidth if we're used as a udp amplification
qboolean SVC_ThrottleInfo (void)
{
#define THROTTLE_PPS 20
	static unsigned int blockuntil;
	unsigned int curtime, inc = 1000/THROTTLE_PPS;

	if (SV_Master_AddressIsMaster(&net_from))
		return true; //allow it without contributing to any throttling.

	curtime = Sys_Milliseconds();

	/*don't go too far back*/
	//if (blockuntil < curtime - 1000)
	if (1000 < curtime - blockuntil)
		blockuntil = curtime - 1000;

	/*don't allow it to go beyond curtime or we get issues with the logic above*/
	if (inc > curtime-blockuntil)
		return false;

	blockuntil += inc;
	return true;
}

//more aggressive logic.
static struct attacker_s
{
	int af;
	qbyte addr[16];

	qboolean blocked;	//if we reach 10 status requests within 30 secs
	double timeout;
	int count;
} *dosattacker;
static size_t dosattacker_count;
static size_t dosattacker_max;
#define dosattacker_limit 15				//if we get X packets
#define dosattacker_period 30					//within Y secs
#define dosattacker_blocktime (60*60*24)	//block them for Z secs (24 hours).
static qboolean SV_DetectAmplificationDDOS (void)
{
	size_t at;
	double t = Sys_DoubleTime();
	int as;
	switch(net_from.type)	//trying to be efficient and avoiding net_comparebaseaddr
	{
	case NA_IP:		as = 4;	break;
	case NA_IPX:	as = 10;break;
	case NA_IPV6:	as = 16;break;
	default:		as = 0;	break;
	}
	if (as)
	{
		for (at = 0; at < dosattacker_count; at++)
		{	//look for an existing one
			if (net_from.type != dosattacker->af)
				continue;
			if (!memcmp(dosattacker[at].addr, &net_from.address, as))
			{	//a match.
				if (t > dosattacker[at].timeout)
				{	//they survived, for now...
					dosattacker[at].count = 0;
					dosattacker[at].timeout = t + dosattacker_period;
				}
				if (++dosattacker[at].count >= dosattacker_limit)
				{
					if (dosattacker[at].count == dosattacker_limit)
					{
						char buf[128];
						Con_Printf(CON_ERROR "%s: Presumed amplification ddos attack, blocking further status queries.\n", NET_BaseAdrToString(buf, sizeof(buf), &net_from));
						Q_snprintfz(buf, sizeof(buf), "\xff\xff\xff\xff%cProbable ddos amplification attack\n", A2C_PRINT);
						NET_SendPacket(svs.sockets, strlen(buf), buf, &net_from);

						dosattacker[at].timeout = t + dosattacker_blocktime;	//a 24 hour block.
					}
					else	//extend by a smidge...
						dosattacker[at].timeout += dosattacker_period/(double)dosattacker_limit;
					return false;
				}
				break;
			}
		}
		if (at == dosattacker_count)
		{	//didn't find one
			for (at = 0; at < dosattacker_count; at++)
			{	//try to find one to recycle
				if (t > dosattacker[at].timeout)
					break;
			}
			if (at == dosattacker_count)
			{
				if (at == dosattacker_max)
				{
					if (at > 4096)	//should we be using hash tables?...
						at--;	//stomp on the last.
					else
					{
						Z_ReallocElements((void**)&dosattacker, &dosattacker_max, max(16, dosattacker_count * 2), sizeof(dosattacker[0]));
					}
				}
				else
					at = dosattacker_count++;
			}
			dosattacker[at].af = net_from.type;
			memcpy(dosattacker[at].addr, &net_from.address, as);
			dosattacker[at].count = 0;
			dosattacker[at].timeout = t + dosattacker_period;
		}
	}

	if (SVC_ThrottleInfo())
		return true;
	return false;
}

/*
=================
SV_ConnectionlessPacket

A connectionless packet has four leading 0xff
characters to distinguish it from a game channel.
Clients that are in the game can still send
connectionless packets.
=================
*/
qboolean SV_ConnectionlessPacket (void)
{
	char	*s;
	char	*c;
	char	adr[MAX_ADR_SIZE];

	MSG_BeginReading (&net_message, svs.netprim);

	if (net_message.cursize >= MAX_QWMSGLEN)	//add a null term in message space
	{
		Con_TPrintf("Oversized packet from %s\n", NET_AdrToString (adr, sizeof(adr), &net_from));
		net_message.cursize=MAX_QWMSGLEN-1;
	}
	net_message.data[net_message.cursize] = '\0';	//terminate it properly. Just in case.

	MSG_ReadLong ();		// skip the -1 marker

	s = MSG_ReadStringLine ();

	Cmd_TokenizeString (s, false, false);

	c = Cmd_Argv(0);

	if (sv_showconnectionlessmessages.ival)
		Con_Printf(S_COLOR_GRAY"%s: %s\n", NET_AdrToString (adr, sizeof(adr), &net_from), s);

	if (!strcmp(c, "ping") || ( c[0] == A2A_PING && (c[1] == 0 || c[1] == '\n')) )
	{	//only continue respond to these if we're actually public. qwfwd likes spamming us endlessly even if we stop heartbeating (which leaves us discoverable to others, too).
		if (sv_public.ival >= 0)
			SVC_Ping ();
	}
	else if (c[0] == A2A_ACK && (c[1] == 0 || c[1] == '\n') )
		SVC_ACK ();
	else if (!strcmp(c,"status"))
	{
		if (sv_public.ival >= 0)
			if (SV_DetectAmplificationDDOS())
				SVC_Status ();
	}
	else if (!strcmp(c,"log"))
	{
		if (SVC_ThrottleInfo())
			SVC_Log ();
	}
#ifdef Q2SERVER
	else if (!strcmp(c, "info"))
	{
		if (sv_public.ival >= 0)
			if (SV_DetectAmplificationDDOS())
				SVC_InfoQ2 ();
	}
#endif
	else if (!strncmp(c,"connect", 7))
	{
#ifdef HAVE_DTLS
		if (net_from.prot == NP_DGRAM)
			NET_DTLS_Disconnect(svs.sockets, &net_from);
#endif

#ifdef Q3SERVER
		if (svs.gametype == GT_QUAKE3)
		{
			q3->sv.DirectConnect(&net_from, &net_message);
			return true;
		}

#ifdef QWOVERQ3
		if (sv_listen_q3.ival)
		{
			if (!strstr(s, "\\name\\"))
			{	//if name isn't in the string, assume they're q3
				//this isn't quite true though, hence the listen check. but users shouldn't be connecting with an empty name anyway. more fool them.
#ifdef HUFFNETWORK
				Huff_DecryptPacket(&net_message, 12);
#endif
				MSG_BeginReading(svs.netprim);
				MSG_ReadLong();
				s = MSG_ReadStringLine();
				Cmd_TokenizeString(s, false, false);
			}
		}
#endif
#endif

		if (secure.value)	//FIXME: possible problem for nq clients when enabled
		{
			Netchan_OutOfBandTPrintf (NCF_SERVER, &net_from, com_language, "%c\nThis server requires client validation.\nPlease use the "FULLENGINENAME" validation program\n", A2C_PRINT);
		}
		else
		{
			SVC_DirectConnect (0);
			return true;
		}
	}
	else if (!strcmp(c,"dtlsconnect"))
	{	//NOTE: redundant. if the server has dtls enabled then it'll respond to dtls hellos with its own stateless cookies
#ifdef HAVE_DTLS
		if (net_from.prot == NP_DGRAM && (net_enable_dtls.ival /*|| !*net_enable_dtls.ival*/))
		{
			if (*Cmd_Argv(2))
				SV_RejectMessage (SCP_QUAKEWORLD, "Proxying not enabled.\n");	//server would be expected to getchallenge+dtlsconnect the target server (or respond with a no-dtls challenge response...)
			else if (SV_ChallengePasses(atoi(Cmd_Argv(1))))
			{
				char *banreason = SV_BannedReason(&net_from);
				if (banreason)
				{
					if (*banreason)
						SV_RejectMessage (SCP_QUAKEWORLD, "You were banned.\nReason: %s\n", banreason);
					else
						SV_RejectMessage (SCP_QUAKEWORLD, "You were banned.\n");
				}
				else
				{
					//NET_DTLS_Disconnect(svs.sockets, &net_from);
					if (NET_DTLS_Create(svs.sockets, &net_from, NULL, false))
						Netchan_OutOfBandPrint(NCF_SERVER, &net_from, "dtlsopened");
					else
						SV_RejectMessage (SCP_QUAKEWORLD, "DTLS driver failure.\n");
				}
			}
			else
				SV_RejectMessage (SCP_QUAKEWORLD, "Bad challenge.\n");
		}
		return true;
#endif
	}
	/*else if (!strcmp(c,"\xad\xad\xad\xad""getchallenge"))
	{
		SVC_GetChallenge (false);
	}*/
	else if (!strcmp(c,"getchallenge"))
	{
		//qw+q2 sends "\xff\xff\xff\xffgetchallenge\n"
		//dp+q3 sends "\xff\xff\xff\xffgetchallenge"
		//ioq3 sends "\xff\xff\xff\xffgetchallenge <clientchallenge> <$com_gamename>"
		//its a subtle difference, but means we can avoid wasteful spam for real qw clients.
		SVC_GetChallenge ((net_message.cursize==16)?true:false);
	}
	else if (!strcmp(c, "getstatus"))
	{	//q3/dpmaster support
		if (sv_public.ival >= 0)
			if (SV_DetectAmplificationDDOS())
				SVC_GetInfo(Cmd_Args(), true);
	}
	else if (!strcmp(c, "getinfo"))
	{	//q3/dpmaster support
		if (sv_public.ival >= 0)
		{
			const char *chal = Cmd_Argv(1);
			SV_Master_HeartbeatResponse(&net_from, chal);

			if (SV_DetectAmplificationDDOS())
				SVC_GetInfo(chal, false);
		}
	}
	else if (!strcmp(c, "rcon"))
	{
		if (SV_DetectAmplificationDDOS())
			SVC_RemoteCommand ();
	}
	else if (!strcmp(c, "realip") || !strcmp(c, "ip"))
		SVC_RealIP ();

#ifdef SUPPORT_ICE
	else if (!strcmp(c, "ice_offer"))
		SVC_ICE_Offer();
	else if (!strcmp(c, "ice_ccand"))
		SVC_ICE_Candidate();
#endif
/*
	else if (!strcmp(c,"lastscores"))
	{
		if (SVC_ThrottleInfo())
			SVC_LastScores ();
	}
	else if (!strcmp(c,"dlist") || !strcmp(c,"demolist"))
	{
		if (SVC_ThrottleInfo())
			SVC_DemoList ();
	}
	else if (!strcmp(c,"dlistr") || !strcmp(c,"dlistregex") || !strcmp(c,"demolistr") || !strcmp(c,"demolistregex"))
	{
		if (SVC_ThrottleInfo())
			SVC_DemoListRegex ();
	}
*/
#ifdef MVD_RECORDING
	else if (!strcmp(c,"qtvusers"))
	{
		if (SVC_ThrottleInfo())
			SVC_QTVUsers ();
	}
#endif
	else if (!PR_GameCodePacket(net_message.data+4))
	{
		static unsigned int lt;
		unsigned int ct = Sys_Milliseconds();
		if (ct - lt > 5*1000)
		{
			Con_TPrintf ("bad connectionless packet from %s: \"%s\"\n", NET_AdrToString (adr, sizeof(adr), &net_from), c);
			lt = ct;
		}
	}

	return false;
}

#ifdef NQPROT
qboolean SVNQ_ConnectionlessPacket(void)
{
	sizebuf_t sb;
	int header;
	int length;
	int active, i, protver;
	int mod, modver, flags;
	unsigned int passwd;
	char *str;
	char buffer[256], buffer2[256];
	netadr_t localaddr;
	char *banreason;

	if (net_from.type == NA_LOOPBACK)
		return false;

	if (!sv_listen_nq.value || SSV_IsSubServer())
		return false;

	MSG_BeginReading(&net_message, svs.netprim);
	header = LongSwap(MSG_ReadLong());
	if (!(header & NETFLAG_CTL))
	{
		//this nasty chunk of code is to try to handle challenges with nq's challengeless protocol, by using stringcmds. woo. evil hacks.
		if (sv_listen_nq.ival == 2)
		if ((header & (NETFLAG_DATA|NETFLAG_EOM)) == (NETFLAG_DATA|NETFLAG_EOM))
		{
			int sequence;
			if (SV_BannedReason (&net_from))
				return false;	//just in case.
			sequence = LongSwap(MSG_ReadLong());
			if (sequence <= 1)
			{
				int numnops = 0;
				int numnonnops = 0;
				int c;
				/*make it at least robust enough to ignore any other stringcmds*/
				while(1)
				{
					switch(MSG_ReadByte())
					{
					case clc_nop:
						numnops++;
						continue;
					case clc_stringcmd:
						numnonnops++;
#define CCON "challengeconnect "
						for(i = 0; ; i++)
						{
							if (!CCON[i])
								c = -1;
							else
								c = MSG_ReadByte();
							if (c != CCON[i])
								break;
						}
						if (!CCON[i])
						{
							if (sv_showconnectionlessmessages.ival)
								Con_Printf(S_COLOR_GRAY"%s: CCREQ_CONNECT_COOKIE\n", NET_AdrToString (com_token, sizeof(com_token), &net_from));
							Cmd_TokenizeString(MSG_ReadStringLine(), false, false);
							/*okay, so this is a reliable packet from a client, containing a 'cmd challengeconnect $challenge' response*/
							str = va("connect %i %i %s \"\\name\\unconnected\\mod\\%s\\modver\\%s\\flags\\%s\\password\\%s\"", NQ_NETCHAN_VERSION, 0, Cmd_Argv(0), Cmd_Argv(1), Cmd_Argv(2), Cmd_Argv(3), Cmd_Argv(4));
							Cmd_TokenizeString (str, false, false);

							SVC_DirectConnect(sequence);

							/*if there is anything else in the packet, we don't actually care. its reliable, so they'll resend*/
							return true;
						}
						else if (c)	//handle any trailing stuff if we don't know what it was.
							MSG_ReadString();
						continue;
					case -1:
						break;
					default:
						numnonnops++;
						break;
					}
					break;
				}

				if (numnops && !numnonnops)
				{
					sb.maxsize = sizeof(buffer);
					sb.data = buffer;

					/*ack it, so dumb proquake clones can actually send the proper packet*/
					if (!sequence)
					{
						SZ_Clear(&sb);
						MSG_WriteLong(&sb, BigLong(NETFLAG_ACK | 8));
						MSG_WriteLong(&sb, sequence);
						NET_SendPacket(svs.sockets, sb.cursize, sb.data, &net_from);
					}


					/*resend the cmd request, cos if they didn't send it then it must have gotten dropped.
					at this point, we assume its a proquake clone and that it already thinks that we are proquake
					a vanilla client will not start spamming nops until it has received the server info, so its only the proquake clients+clones that will send a nop before a challengeconnect
					unfortunatly we don't know the modver+flags+password any more. I hope its not needed. if it does, server admins will be forced to use sv_listen_nq 1 instead of 2*/
					SZ_Clear(&sb);
					MSG_WriteLong(&sb, BigLong(0));
					MSG_WriteLong(&sb, BigLong(1));	//sequence 1, because 0 matches the old sequence, and thus might get dropped. hopefully the client will cope with dupes properly and ignore any regular (but unreliable) stuff.

					MSG_WriteByte(&sb, svc_stufftext);
					MSG_WriteString(&sb, va("cmd challengeconnect %i %i\n", SV_NewChallenge(), MOD_PROQUAKE));

					*(int*)sb.data = BigLong(NETFLAG_UNRELIABLE|sb.cursize);
					NET_SendPacket(svs.sockets, sb.cursize, sb.data, &net_from);

					return true;
				}
			}
		}
		return false;	//no idea what it is.
	}

	length = header & NETFLAG_LENGTH_MASK;
	if (length != net_message.cursize)
		return false;	//corrupt or not ours

	switch(MSG_ReadByte())
	{
	case CCREQ_CONNECT:
		str = MSG_ReadString();
		protver = MSG_ReadByte();

		if (sv_showconnectionlessmessages.ival)
			Con_Printf(S_COLOR_GRAY"%s: CCREQ_CONNECT (\"%s\" %i)\n", NET_AdrToString (com_token, sizeof(com_token), &net_from), str, protver);

		sb.maxsize = sizeof(buffer);
		sb.data = buffer;
		if (strcmp(str, NQ_NETCHAN_GAMENAME))
		{
			SZ_Clear(&sb);
			MSG_WriteLong(&sb, 0);
			MSG_WriteByte(&sb, CCREP_REJECT);
			MSG_WriteString(&sb, "Incorrect game\n");
			*(int*)sb.data = BigLong(NETFLAG_CTL+sb.cursize);
			NET_SendPacket(svs.sockets, sb.cursize, sb.data, &net_from);
			return true;	//not our game.
		}
		if (protver != NQ_NETCHAN_VERSION && protver != NQ_NETCHAN_VERSION_QEX)
		{
			SZ_Clear(&sb);
			MSG_WriteLong(&sb, 0);
			MSG_WriteByte(&sb, CCREP_REJECT);
			MSG_WriteString(&sb, "Incorrect version\n");
			*(int*)sb.data = BigLong(NETFLAG_CTL+sb.cursize);
			NET_SendPacket(svs.sockets, sb.cursize, sb.data, &net_from);
			return true;	//not our version...
		}

		banreason = SV_BannedReason (&net_from);
		if (banreason)
		{
			SZ_Clear(&sb);
			MSG_WriteLong(&sb, 0);
			MSG_WriteByte(&sb, CCREP_REJECT);
			MSG_WriteString(&sb, *banreason?va("You are banned: %s\n", banreason):"You are banned\n");
			*(int*)sb.data = BigLong(NETFLAG_CTL+sb.cursize);
			NET_SendPacket(svs.sockets, sb.cursize, sb.data, &net_from);
			return true;	//not our version...
		}

		//proquake's extensions
		mod = MSG_ReadByte();
		modver = MSG_ReadByte();
		flags = MSG_ReadByte();
		passwd = MSG_ReadLong();
		if (msg_badread)
			passwd = 0;

		if (SV_ChallengeRecent())
			return true;

		Cmd_TokenizeString (MSG_ReadString(), false, false);
		if (!strcmp(Cmd_Argv(0), "getchallenge") && (sv_listen_qw.ival || sv_listen_dp.ival))
		{
			/*dual-stack client, supporting either DP or QW protocols*/
			SVC_GetChallenge (false);
		}
#ifdef HAVE_DTLS
		else if (net_enable_dtls.ival > 2 && (net_from.prot == NP_DGRAM || net_from.prot == NP_STREAM || net_from.prot == NP_WS) && net_from.type != NA_LOOPBACK && !NET_IsEncrypted(&net_from))
		{
			SV_RejectMessage (SCP_NETQUAKE, "This server requires the use of DTLS/TLS/WSS.\n");
			return true;
		}
#endif
		else
		{	//legacy pure-nq (though often DP).
			if (progstype == PROG_H2)
			{
				SV_RejectMessage (SCP_NETQUAKE, "NQ clients are not supported with hexen2 gamecode\n");
				return true;	//not our version...
			}
			if (NET_WasSpecialPacket(svs.sockets))
				return true;
#ifdef HAVE_PACKET
			if (sv_listen_nq.ival == 2 && net_from.prot == NP_DGRAM
#ifdef SUPPORT_ICE
									   && net_from.type != NA_ICE
#endif
				)
			{
				if (password.string[0] &&
					stricmp(password.string, "none"))
				{	//make sure we don't get crippled because of being unable to specify the actual password with proquake's stuff.
					char *e;
					qintmax_t svpass = strtoll(password.string, &e, 0);
					if (*e)	//something ain't numeric... hash it so they have a chance of getting it right...
						svpass = CalcHashInt(&hash_md4, password.string, strlen(password.string));
					if (passwd != svpass)
					{
						Con_TPrintf ("%s:password failed (nq)\n", NET_AdrToString (buffer2, sizeof(buffer2), &net_from));
						SV_RejectMessage (SCP_NETQUAKE, "\x01this server requires a password\n\n");

						//and prevent them from spamming attempts. botnets might still get through fast though.
						SV_AutoBanSender(15, "password cooldown");
						return true;
					}
				}

				SZ_Clear(&sb);
				MSG_WriteLong(&sb, 0);
				MSG_WriteByte(&sb, CCREP_ACCEPT);
				NET_LocalAddressForRemote(svs.sockets, &net_from, &localaddr, 0);
				MSG_WriteLong(&sb, ShortSwap(localaddr.port));
				MSG_WriteByte(&sb, MOD_PROQUAKE);
				MSG_WriteByte(&sb, MOD_PROQUAKE_VERSION);
				*(int*)sb.data = BigLong(NETFLAG_CTL|sb.cursize);
				NET_SendPacket(svs.sockets, sb.cursize, sb.data, &net_from);


				SZ_Clear(&sb);
				MSG_WriteLong(&sb, BigLong(0));
				MSG_WriteLong(&sb, BigLong(1));	//sequence 1, because 0 matches the old sequence, and thus might get dropped. hopefully the client will cope with dupes properly and ignore any regular (but unreliable) stuff.

				MSG_WriteByte(&sb, svc_stufftext);
				MSG_WriteString(&sb, va("cmd challengeconnect %i %i %i %i %i\n", SV_NewChallenge(), mod, modver, flags, passwd));

				*(int*)sb.data = BigLong(NETFLAG_UNRELIABLE|sb.cursize);
				NET_SendPacket(svs.sockets, sb.cursize, sb.data, &net_from);
				/*don't worry about repeating, the nop case above will recover it*/
			}
			else
#endif
			{
				str = va("connect %i %i %i \"\\name\\unconnected\\mod\\%i\\modver\\%i\\flags\\%i\\password\\%i\"", protver, 0, SV_NewChallenge(), mod, modver, flags, passwd);
				Cmd_TokenizeString (str, false, false);

				SVC_DirectConnect(0);
			}
		}
		return true;
	case CCREQ_SERVER_INFO:
		if (sv_showconnectionlessmessages.ival)
			Con_Printf(S_COLOR_GRAY"%s: CCREQ_SERVER_INFO\n", NET_AdrToString (com_token, sizeof(com_token), &net_from));
		if (sv_public.ival < 0)
			return false;
		if (SV_BannedReason (&net_from))
			return false;
		if (Q_strcmp (MSG_ReadString(), NQ_NETCHAN_GAMENAME) != 0)
			return false;

		sb.maxsize = sizeof(buffer);
		sb.data = buffer;
		SZ_Clear (&sb);
		// save space for the header, filled in later
		MSG_WriteLong (&sb, 0);
		MSG_WriteByte (&sb, CCREP_SERVER_INFO);
		if (NET_LocalAddressForRemote(svs.sockets, &net_from, &localaddr, 0))
			MSG_WriteString (&sb, NET_AdrToString (buffer2, sizeof(buffer2), &localaddr));
		else
			MSG_WriteString (&sb, "unknown");
		active = 0;
		for (i = 0; i < sv.allocated_client_slots; i++)
			if (svs.clients[i].state)
				active++;
		MSG_WriteString (&sb, hostname.string);
		MSG_WriteString (&sb, svs.name);
		MSG_WriteByte (&sb, active);
		MSG_WriteByte (&sb, maxclients.value);
		MSG_WriteByte (&sb, NQ_NETCHAN_VERSION);
		*(int*)sb.data = BigLong(NETFLAG_CTL+sb.cursize);
		NET_SendPacket(svs.sockets, sb.cursize, sb.data, &net_from);
		return true;
	case CCREQ_PLAYER_INFO:
		if (sv_showconnectionlessmessages.ival)
			Con_Printf(S_COLOR_GRAY"%s: CCREQ_PLAYER_INFO\n", NET_AdrToString (com_token, sizeof(com_token), &net_from));
		if (sv_public.ival < 0)
			return false;
		if (SV_BannedReason (&net_from))
			return false;
		/*one request per player, ouch ouch ouch, what will it make of 32 players, I wonder*/
		sb.maxsize = sizeof(buffer);
		sb.data = buffer;
		// save space for the header, filled in later
		SZ_Clear (&sb);
		MSG_WriteLong (&sb, 0);
		{
			unsigned int pno;
			client_t *cl;
			pno = MSG_ReadByte();
			if (pno >= sv.allocated_client_slots)
				break;
			cl = &svs.clients[pno];
			if (cl->state <= cs_zombie)
				break;

			MSG_WriteByte (&sb, CCREP_PLAYER_INFO);
			MSG_WriteByte (&sb, pno);
			MSG_WriteString (&sb, cl->name);
			MSG_WriteLong (&sb, cl->playercolor);
			MSG_WriteLong (&sb, cl->old_frags);
			MSG_WriteLong (&sb, realtime - cl->connection_started);
			MSG_WriteString (&sb, SV_PlayerPublicAddress(cl));	/*player's address, leave blank, don't spam that info as it can result in personal attacks exploits*/
		}
		*(int*)sb.data = BigLong(NETFLAG_CTL+sb.cursize);
		NET_SendPacket(svs.sockets, sb.cursize, sb.data, &net_from);
		return true;
	case CCREQ_RULE_INFO:
		if (sv_showconnectionlessmessages.ival)
			Con_Printf(S_COLOR_GRAY"%s: CCREQ_RULE_INFO\n", NET_AdrToString (com_token, sizeof(com_token), &net_from));
		if (sv_public.ival < 0)
			return false;
		if (SV_BannedReason (&net_from))
			return false;

		/*lol, nq is evil*/
		sb.maxsize = sizeof(buffer);
		sb.data = buffer;
		// save space for the header, filled in later
		SZ_Clear (&sb);
		MSG_WriteLong (&sb, 0);
		{
			const char *rname, *rval, *kname;
			rname = MSG_ReadString();

			if (!*rname)
				rname = InfoBuf_KeyForNumber(&svs.info, 0);
			else
			{
				int i = 0;
				for(;;)
				{
					kname = InfoBuf_KeyForNumber(&svs.info, i);
					if (!kname)
					{
						rname = NULL;
						break;
					}
					i++;
					if (!strcmp(kname, rname))
					{
						rname = InfoBuf_KeyForNumber(&svs.info, i);
						break;
					}
				}
			}
			if (rname)
				rval = InfoBuf_ValueForKey(&svs.info, rname);
			else
				rval = rname = "";
			MSG_WriteByte (&sb, CCREP_RULE_INFO);
			MSG_WriteString (&sb, rname);
			MSG_WriteString (&sb, rval);
		}
		*(int*)sb.data = BigLong(NETFLAG_CTL+sb.cursize);
		NET_SendPacket(svs.sockets, sb.cursize, sb.data, &net_from);
		return true;
	}
	return false;
}
#endif

/*
==============================================================================

PACKET FILTERING


You can add or remove addresses from the filter list with:

addip <ip>
removeip <ip>

The ip address is specified in dot format, and any unspecified digits will match any value, so you can specify an entire class C network with "addip 192.246.40".

Removeip will only remove an address specified exactly the same way.  You cannot addip a subnet, then removeip a single host.

listip
Prints the current list of filters.

writeip
Dumps "addip <ip>" commands to listip.cfg so it can be execed at a later date.  The filter lists are not saved and restored by default, because I beleive it would cause too much confusion.

filterban <0 or 1>

If 1 (the default), then ip addresses matching the current list will be prohibited from entering the game.  This is the default setting.

If 0, then only addresses matching the list will be allowed.  This lets you easily set up a private game, or a game that only allows players from your local network.


==============================================================================
*/

cvar_t	filterban = CVARD("filterban", "1", "If 0, players will be kicked by default unless there is a rule that allows them. Also affects the default action of addip.");

//send a network packet to a new non-connected client.
//this is to combat tunneling
void SV_OpenRoute_f(void)
{
	netadr_t to;
	char data[64];

	if (NET_StringToAdr(Cmd_Argv(1), PORT_QWCLIENT, &to))
	{
		sprintf(data, "\xff\xff\xff\xff%c", S2C_CONNECTION);

		Netchan_OutOfBandPrint(NCF_SERVER, &to, "hello");
//		NET_SendPacket (strlen(data)+1, data, to);
	}
}

//============================================================================
static int inboundsequence;	//so we can detect frames when we didn't get any packets, even when packets come from epoll
void SV_ReadPacket(void)
{
	int			i;
	client_t	*cl;
	int			qport;
	char		*banreason;

	// check for connectionless packet (0xffffffff) first
	if (*(unsigned int *)net_message.data == ~0)
	{
		banreason = SV_BannedReason (&net_from);
		if (banreason)
		{
			static unsigned int lt;
			unsigned int ct = Sys_Milliseconds();
			if (ct - lt > 5*1000)
			{
				if (*banreason)
					Netchan_OutOfBandTPrintf(NCF_SERVER, &net_from, com_language, "You are banned: %s\n", banreason);
				else
					Netchan_OutOfBandTPrintf(NCF_SERVER, &net_from, com_language, "You are banned\n");
			}
			return;
		}

		if (NET_WasSpecialPacket(svs.sockets))	//fixes up IP->ICE addresses (so we don't break when routes change).
			return;
		SV_ConnectionlessPacket();
		return;
	}
#ifdef HAVE_DTLS
	else
	{
		if (NET_DTLS_Decode(svs.sockets))
		{
			if (!net_message.cursize)
				return;
			if (*(unsigned int *)net_message.data == ~0)
			{
				SV_ConnectionlessPacket();
				return;
			}
		}
	}
#endif

#ifdef Q3SERVER
	if (svs.gametype == GT_QUAKE3)
	{
		if (q3->sv.HandleClient(&net_from, &net_message))
			inboundsequence++;
		else if (NET_WasSpecialPacket(svs.sockets))
			return;
		return;
	}
#endif

	// read the qport out of the message so we can fix up
	// stupid address translating routers
	MSG_BeginReading (&net_message, svs.netprim);
	MSG_ReadLong ();		// sequence number
	MSG_ReadLong ();		// sequence number
	qport = MSG_ReadShort () & 0xffff;

	// check for packets from connected clients
	for (i=0, cl=svs.clients ; i<svs.allocated_client_slots ; i++,cl++)
	{
		if (cl->state == cs_free || cl->controller)
			continue;
		if (!NET_CompareBaseAdr (&net_from, &cl->netchan.remote_address))
			continue;
#ifdef NQPROT
		if (ISNQCLIENT(cl) && cl->netchan.remote_address.port == net_from.port)
		{
			if (cl->state >= cs_connected)
			{
				if (cl->delay > 0)
					goto dominping;

				switch(NQNetChan_Process(&cl->netchan))
				{
				case NQNC_IGNORED:
					break;
				case NQNC_ACK:
					if (cl->netchan.message.cursize)
						inboundsequence++;	//we need to wake up...
					if (cl->netchan.reliable_length > cl->netchan.reliable_start)
						inboundsequence++;	//we need to wake up...
					break;
				case NQNC_RELIABLE:
				case NQNC_UNRELIABLE:
					inboundsequence++;
					svs.stats.packets++;
					SVNQ_ExecuteClientMessage(cl);
					break;
				}
			}
			break;
		}
#endif

#ifdef Q3SERVER
		if (ISQ3CLIENT(cl))
			continue;
#endif

		if (cl->netchan.qportsize == 0)
		{	//no qports... use the actual port.
			if (cl->netchan.remote_address.port != net_from.port)
				continue;
		}
		else
		{
			if (cl->netchan.qport != qport)
				continue;
		}
		if (cl->netchan.remote_address.port != net_from.port)
		{
			Con_DPrintf ("SV_ReadPackets: fixing up a translated port\n");
			cl->netchan.remote_address.port = net_from.port;
		}

		if (cl->delay > 0)
		{
#ifdef NQPROT
dominping:
#endif
			if (cl->state < cs_connected)
				break;
			if (net_message.cursize > sizeof(svs.free_lagged_packet->data))
			{
				Con_Printf("packet too large for minping\n");
				cl->delay -= 0.001;
				break;	//drop this packet
			}

			if (!svs.free_lagged_packet)	//kinda nasty
				svs.free_lagged_packet = Z_Malloc(sizeof(*svs.free_lagged_packet));

			if (!cl->laggedpacket)
				cl->laggedpacket_last = cl->laggedpacket = svs.free_lagged_packet;
			else
			{
				cl->laggedpacket_last->next = svs.free_lagged_packet;
				cl->laggedpacket_last = cl->laggedpacket_last->next;
			}
			svs.free_lagged_packet = svs.free_lagged_packet->next;
			cl->laggedpacket_last->next = NULL;

			cl->laggedpacket_last->time = realtime + cl->delay;
			memcpy(cl->laggedpacket_last->data, net_message.data, net_message.cursize);
			cl->laggedpacket_last->length = net_message.cursize;
			break;
		}


		if (Netchan_Process(&cl->netchan))
		{	// this is a valid, sequenced packet, so process it
			inboundsequence++;
			svs.stats.packets++;
			if (cl->state >= cs_connected)
			{
				if (cl->send_message)
					cl->chokecount++;
				else
					cl->send_message = true;	// reply at end of frame

#ifdef Q2SERVER
				if (ISQ2CLIENT(cl))
					SVQ2_ExecuteClientMessage(cl);
				else
#endif
					SV_ExecuteClientMessage (cl);
			}
		}
		break;
	}

	if (i != svs.allocated_client_slots)
		return;

#ifdef QWOVERQ3
	if (sv_listen_q3.ival && q3->sv.HandleClient())
	{
		received++;
		continue;
	}
#endif

#ifdef NQPROT
	if (SVNQ_ConnectionlessPacket())
		return;
#endif
	if (SV_BannedReason (&net_from))
		return;

	if (NET_WasSpecialPacket(svs.sockets))
		return;

	// packet is not from a known client
	if (sv_showconnectionlessmessages.ival)
		Con_Printf (S_COLOR_GRAY "%s:sequenced packet without connection (%i bytes)\n", NET_AdrToString (com_token, sizeof(com_token), &net_from), net_message.cursize);	//hack: com_token cos we need some random temp buffer.
}

/*
=================
SV_ReadPackets
=================
*/
void SV_KillExpiredBans(void);
#ifdef SERVER_DEMO_PLAYBACK
//FIMXE: move to header
qboolean SV_GetPacket (void);
#endif
qboolean SV_ReadPackets (float *delay)
{
	int			i;
	client_t	*cl;
	laggedpacket_t *lp;

	static int oldinboundsequence;

	SV_KillExpiredBans();

	for (i = 0; i < svs.allocated_client_slots; i++)	//fixme: shouldn't we be using svs.allocated_client_slots ?
	{
		cl = &svs.clients[i];
		while (cl->laggedpacket)
		{
			//schedule a wakeup so minping is more consistant
			if (cl->laggedpacket->time > realtime)
			{
				if (*delay > cl->laggedpacket->time - realtime)
					*delay = cl->laggedpacket->time - realtime;
				break;
			}
			else
			{
				lp = cl->laggedpacket;
				cl->laggedpacket = lp->next;
				if (cl->laggedpacket_last == lp)
					cl->laggedpacket_last = lp->next;

				lp->next = svs.free_lagged_packet;
				svs.free_lagged_packet = lp;

				SZ_Clear(&net_message);
				memcpy(net_message.data, lp->data, lp->length);
				net_message.cursize = lp->length;

				net_from = cl->netchan.remote_address;	//not sure if anything depends on this, but lets not screw them up willynilly

#ifdef NQPROT
				if (ISNQCLIENT(cl))
				{
					if (cl->state >= cs_connected)
					{
						if (NQNetChan_Process(&cl->netchan))
						{
							inboundsequence++;
							svs.stats.packets++;
							SVNQ_ExecuteClientMessage(cl);
						}
					}
				}
				else
#endif
				{
					/*QW*/
					if (Netchan_Process(&cl->netchan))
					{	// this is a valid, sequenced packet, so process it
						inboundsequence++;
						svs.stats.packets++;
						if (cl->state >= cs_connected)
						{	//make sure they didn't already disconnect
							if (cl->send_message)
								cl->chokecount++;
							else
								cl->send_message = true;	// reply at end of frame

		#ifdef Q2SERVER
							if (ISQ2CLIENT(cl))
								SVQ2_ExecuteClientMessage(cl);
							else
		#endif
								SV_ExecuteClientMessage (cl);
						}
					}
				}
			}
		}
	}

	NET_ReadPackets(svs.sockets);

	if (inboundsequence == oldinboundsequence)
		return false;	//nothing new.
	oldinboundsequence = inboundsequence;
	return true;
}

/*
==================
SV_CheckTimeouts

If a packet has not been received from a client in timeout.value
seconds, drop the conneciton.

When a client is normally dropped, the client_t goes into a zombie state
for a few seconds to make sure any final reliable message gets resent
if necessary
==================
*/
void SV_CheckTimeouts (void)
{
	int		i;
	client_t	*cl, *cont;
	float	droptime;
	int	nclients;

	droptime = realtime - timeout.value;
	nclients = 0;

	for (i=0,cl=svs.clients ; i<svs.allocated_client_slots ; i++,cl++)
	{
		if (cl->state == cs_connected || cl->state == cs_spawned)
		{
			if (!cl->spectator)
				nclients++;
			cont = cl;
			if (cont->controller)
				cont = cont->controller;
			if (cont->netchan.last_received < droptime && cl->netchan.remote_address.type != NA_LOOPBACK && cl->protocol != SCP_BAD)
			{
				SV_BroadcastTPrintf (PRINT_HIGH, "Client %s timed out\n", cl->name);
				SV_DropClient (cl);
				cl->state = cs_free;	// don't bother with zombie state for local player.
				if (cl->netchan.remote_address.type != NA_INVALID)
					NET_TerminateRoute(svs.sockets, &cl->netchan.remote_address);
				cl->netchan.remote_address.type = NA_INVALID;
			}
		}

		if (cl->state == cs_zombie && realtime - cl->connection_started > zombietime.value)
		{
			cl->state = cs_free;	// can now be reused
			if (cl->netchan.remote_address.type != NA_INVALID)
				NET_TerminateRoute(svs.sockets, &cl->netchan.remote_address);
			cl->netchan.remote_address.type = NA_INVALID;
		}

		if (cl->state == cs_loadzombie && realtime - cl->connection_started > zombietime.value)
		{
			if (cl->istobeloaded)
			{
				if (1)//svs.gametype != GT_PROGS)
				{
					cl->netchan.remote_address.type = NA_INVALID;	//make it look like a bot.
					if (cl->istobeloaded == 1)
						cl->state = cs_spawned;		//client has an entity, apparently.
					else
						cl->state = cs_connected;	//not actually on yet
					cl->istobeloaded = false;
					if (*cl->name)
						SV_BroadcastTPrintf (PRINT_HIGH, "LoadZombie %s timed out\n", cl->name);
					else
						SV_BroadcastTPrintf (PRINT_HIGH, "LoadZombie timed out\n");
					SV_DropClient (cl);
				}
				else
				{
					if (cl->istobeloaded == 1)
					{
						pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, cl->edict);
						PR_ExecuteProgram (svprogfuncs, pr_global_struct->ClientDisconnect);
						if (*cl->name)
							SV_BroadcastTPrintf (PRINT_HIGH, "LoadZombie %s timed out\n", cl->name);
						else
							SV_BroadcastTPrintf (PRINT_HIGH, "LoadZombie timed out\n");
					}
					sv.spawned_client_slots--;

					cl->istobeloaded=false;

					//must go through a zombie phase for 2 secs when the zombie gets removed.
					cl->state = cs_zombie;	// the real zombieness starts now
					cl->connection_started = realtime;
				}
			}
			else
			{
				//no entity, just free them.
#ifdef SUBSERVERS
				SSV_SavePlayerStats(cl, 3);
#endif
				SV_BroadcastTPrintf (PRINT_HIGH, "TransferZombie %s timed out\n", cl->name);
				cl->state = cs_free;
				*cl->name = 0;
			}
			cl->netchan.remote_address.type = NA_INVALID;	//don't mess up from not knowing their address.
		}
	}
	if ((sv.paused&PAUSE_EXPLICIT) && !nclients)
	{
		// nobody left, unpause the server
		if (SV_TogglePause(NULL))
			SV_BroadcastTPrintf(PRINT_HIGH, "pause released due to empty server\n");
	}
}

/*
===================
SV_GetConsoleCommands

Add them exactly as if they had been typed at the console
===================
*/
qboolean QCExternalDebuggerCommand(char *text);
void SV_GetConsoleCommands (void)
{
	char	*cmd;

	while (1)
	{
		cmd = Sys_ConsoleInput ();
		if (!cmd)
			break;
		if (QCExternalDebuggerCommand(cmd))
			continue;
		Log_String(LOG_CONSOLE, cmd);
		Cbuf_AddText (cmd, RESTRICT_LOCAL);
		Cbuf_AddText ("\n", RESTRICT_LOCAL);
	}
}

#define MINDRATE 4096
#define MINRATE 500
int SV_RateForClient(client_t *cl)
{
	int rate;
	if (cl->download)
	{
		rate = cl->drate;
		if (sv_maxdrate.ival)
		{
			if (!rate || rate > sv_maxdrate.value)
				rate = sv_maxdrate.value;
			else if (rate < MINDRATE)
				rate = MINDRATE;
		}
		else if (rate != 0 && rate < MINDRATE)
			rate = MINDRATE;
	}
	else
	{
		rate = cl->rate;
		if (sv_maxrate.ival)
		{
			if (!rate || rate > sv_maxrate.value)
				rate = sv_maxrate.value;
			else if (rate < MINRATE)
				rate = MINRATE;
		}
		else if (rate != 0 && rate < MINRATE)
			rate = MINRATE;
	}

	return rate;
}
/*
===================
SV_CheckVars

===================
*/
void SV_CheckVars (void)
{
	static char *pw, *spw;
	int			v;

	if (password.string == pw && spectator_password.string == spw)
		return;
	pw = password.string;
	spw = spectator_password.string;

	v = 0;
	if (pw && pw[0] && strcmp(pw, "none"))
		v |= 1;
	if (spw && spw[0] && strcmp(spw, "none"))
		v |= 2;

	Con_DPrintf ("Updated needpass.\n");
	if (!v)
		InfoBuf_SetValueForKey (&svs.info, "needpass", "");
	else
		InfoBuf_SetValueForKey (&svs.info, "needpass", va("%i",v));
}

#ifdef Q2SERVER
void SVQ2_ClearEvents(void)
{
	q2edict_t	*ent;
	int		i;

	for (i=0 ; i<ge->num_edicts ; i++, ent++)
	{
		ent = Q2EDICT_NUM(i);
		// events only last for a single message
		ent->s.event = 0;
	}
}
#endif


/*
==================
SV_Impulse_f

Spawns a client, uses an impulse, uses that clients think then removes the client.
==================
*/
void SV_Impulse_f (void)
{
	int i;
	if (svs.gametype != GT_PROGS)
	{
		Con_Printf("Not supported in this game type\n");
		return;
	}

	for (i = 0; i < sv.allocated_client_slots; i++)
	{
		if (svs.clients[i].state == cs_free)
		{
			break;
		}
	}

	if (i == sv.allocated_client_slots)
	{
		Con_Printf("No empty player slots\n");
		return;
	}
	if (!svprogfuncs)
		return;

	pr_global_struct->time = sv.world.physicstime;

	svs.clients[i].state = cs_connected;
	svs.clients[i].connection_started = realtime;

	SV_SetUpClientEdict(&svs.clients[i], svs.clients[i].edict);

	svprogfuncs->SetStringField(svprogfuncs, svs.clients[i].edict, &svs.clients[i].edict->v->netname, "Console", true);

	sv.skipbprintclient = &svs.clients[i];
	pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, svs.clients[i].edict);
	PR_ExecuteProgram (svprogfuncs, pr_global_struct->ClientConnect);
	sv.skipbprintclient = NULL;

	pr_global_struct->time = sv.world.physicstime;
	pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, svs.clients[i].edict);
	PR_ExecuteProgram (svprogfuncs, pr_global_struct->PutClientInServer);
	sv.spawned_client_slots++;

	pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, svs.clients[i].edict);
	PR_ExecuteProgram (svprogfuncs, pr_global_struct->PlayerPreThink);
	pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, svs.clients[i].edict);
	PR_ExecuteProgram (svprogfuncs, svs.clients[i].edict->v->think);
	pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, svs.clients[i].edict);
	PR_ExecuteProgram (svprogfuncs, pr_global_struct->PlayerPostThink);

	svs.clients[i].edict->v->impulse = atoi(Cmd_Argv(1));

	pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, svs.clients[i].edict);
	PR_ExecuteProgram (svprogfuncs, pr_global_struct->PlayerPreThink);
	pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, svs.clients[i].edict);
	PR_ExecuteProgram (svprogfuncs, svs.clients[i].edict->v->think);
	pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, svs.clients[i].edict);
	PR_ExecuteProgram (svprogfuncs, pr_global_struct->PlayerPostThink);

	pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, svs.clients[i].edict);
	PR_ExecuteProgram (svprogfuncs, pr_global_struct->ClientDisconnect);
	sv.spawned_client_slots--;

	svs.clients[i].state = cs_free;
}

static void SV_PauseChanged(void)
{
	int i;
	client_t *cl;
	// send notification to all clients
	for (i=0, cl = svs.clients ; i<svs.allocated_client_slots ; i++, cl++)
	{
		if (!cl->state)
			continue;
		if ((ISQWCLIENT(cl) || ISNQCLIENT(cl)) && !cl->controller)
		{
			ClientReliableWrite_Begin (cl, svc_setpause, 2);
			ClientReliableWrite_Byte (cl, sv.paused!=0);
		}
	}
#ifdef MVD_RECORDING
	if (sv.mvdrecording)
	{
		ClientReliableWrite_Begin (&demo.recorder, svc_setpause, 2);
		ClientReliableWrite_Byte (&demo.recorder, sv.paused!=0);
	}
#endif
}

double server_frametime;
/*
==================
SV_Frame

==================
*/
float SV_Frame (void)
{
	extern cvar_t pr_imitatemvdsv;
	static double	start, end, idletime;
	static int oldpackets;
	float oldtime;
	qboolean isidle;
	float timedelta;
	float delay;

	start = Sys_DoubleTime ();
	svs.stats.idle += start - end;
	end = start;

	COM_MainThreadWork();

	//qw qc uses this for newmis handling
	svs.framenum++;
	if (svs.framenum > 0x10000)
		svs.framenum = 0;

	delay = sv_maxtic.value;
	if (delay < sv_mintic.value)
		delay = sv_mintic.value;
	if (isDedicated && sv.allocated_client_slots == 0)
		delay = max(delay, 1);	//when idle, don't keep waking up for no reason

// keep the random time dependent
	rand ();

	if (!sv.gamespeed)
		sv.gamespeed = 1;

#ifdef WEBCLIENT
	if (isDedicated)
	{
//		FTP_ClientThink();
		HTTP_CL_Think(NULL, NULL);
	}
#endif

#ifdef HAVE_CLIENT
	isidle = !isDedicated && sv.allocated_client_slots == 1 && (Key_Dest_Has(~kdm_game)
#ifdef QUAKESTATS
		|| IN_WeaponWheelIsShown()
#endif
		|| cl.implicitpause) && cls.state == ca_active;
	/*server is effectively paused in SP/coop if there are no clients/spectators*/
	if (sv.spawned_client_slots == 0 && sv.spawned_observer_slots == 0 && !deathmatch.ival)
		isidle = true;
	if ((sv.paused & PAUSE_AUTO) != ((isidle)?PAUSE_AUTO:0))
		sv.paused ^= PAUSE_AUTO;
#endif

	if (sv.oldpaused != sv.paused)
	{
		sv.oldpaused = sv.paused;
		SV_PauseChanged();
	}


	//work out the gamespeed
	if (sv.gamespeed != sv_gamespeed.value)
	{
		char *newspeed;
		sv.gamespeed = sv_gamespeed.value;
		if (sv.gamespeed < 0.1 || sv.gamespeed == 1)
			sv_gamespeed.value = sv.gamespeed = 1;

		if (sv.gamespeed == 1)
			newspeed = "";
		else
			newspeed = va("%g", sv.gamespeed*100);
		InfoBuf_SetValueForStarKey(&svs.info, "*gamespeed", newspeed);

		//correct sv.starttime
		sv.starttime = Sys_DoubleTime() - (sv.time/sv.gamespeed);
	}


// decide the simulation time
	{
		oldtime = sv.time;
		sv.time = (Sys_DoubleTime() - sv.starttime)*sv.gamespeed;
		timedelta = sv.time - oldtime;
		if (sv.time < oldtime)
		{
			sv.time = oldtime;	//urm
			timedelta = 0;
		}

		if (isDedicated)
			realtime += sv.time - oldtime;

		if (sv.paused && sv.time > 1.5)
		{
			sv.starttime += (sv.time - oldtime)/sv.gamespeed;	//move the offset
			sv.time = oldtime;	//and keep time as it was.
		}
	}

#ifdef SV_MASTER
	if (sv_master.ival)
		SVM_Think();
#endif

#ifdef PLUGINS
	if (isDedicated)
		Plug_Tick();
#endif

#ifdef SUBSERVERS
	MSV_PollSlaves();
#endif

#ifdef SQL
	SQL_ServerCycle();
#endif

	if (sv.state < ss_active || !sv.world.worldmodel)
	{
#ifdef SUBSERVERS
		if (sv.state == ss_clustermode)
		{
			isidle = !SV_ReadPackets (&delay);
			SV_SendClientMessages ();
		}
#endif
#ifndef SERVERONLY
	// check for commands typed to the host
		if (isDedicated)
#endif
		{
			SV_GetConsoleCommands ();
			Cbuf_Execute ();
		}
		return delay;
	}

// check timeouts
	SV_CheckTimeouts ();

	SV_CheckTimer ();

// toggle the log buffer if full
	SV_CheckLog ();

// get packets
	isidle = !SV_ReadPackets (&delay);
	if (isDedicated)
		NET_Tick();

	if (pr_imitatemvdsv.ival || dpcompat_nopreparse.ival)
	{
		Cbuf_Execute ();
		if (sv.state < ss_active)	//whoops...
			return delay;
	}

	if (sv.multicast.cursize)
	{
		Con_TPrintf("Unterminated multicast\n");
		sv.multicast.cursize=0;
	}

	// move autonomous things around if enough time has passed
	if (!sv.paused || (sv.world.physicstime < 1 && sv.spawned_client_slots))
	{
#ifdef Q2SERVER
		//q2 is idle even if clients sent packets.
		if (svs.gametype == GT_QUAKE2)
			isidle = true;
#endif
		if (SV_Physics ())
		{
			isidle = false;

#ifdef SAVEDGAMES
			if (sv.time > sv.autosave_time)
				SV_AutoSave();
#endif
		}
	}
	else
	{
		isidle = idletime < 0.1;
#ifdef VM_Q1
		if (svs.gametype == GT_Q1QVM)
		{
			Q1QVM_GameCodePausedTic(Sys_DoubleTime() - sv.pausedstart);
		}
		else
#endif
		{
			PR_GameCodePausedTic(Sys_DoubleTime() - sv.pausedstart);
		}
	}

	//run any end-of-frame tasks that were set up
	while (svs.frameendtasks)
	{
		struct frameendtasks_s *t = svs.frameendtasks;
		svs.frameendtasks = t->next;
		t->callback(t);
		Z_Free(t);
	}

	if (!isidle || idletime > 0.15)
	{
		//this is the q2 frame number found in the q2 protocol. each packet should contain a new frame or interpolation gets confused
		sv.framenum++;

#ifdef SERVER_DEMO_PLAYBACK
		while(SV_ReadMVD());
#endif

		if (sv.multicast.cursize)
		{
			Con_TPrintf("Unterminated multicast\n");
			sv.multicast.cursize=0;
		}

#ifndef SERVERONLY
// check for commands typed to the host
		if (isDedicated)
#endif
		{
			if (sv.framenum != 1)
			{
#ifndef SERVERONLY
				Sys_SendKeyEvents();
#else
				SV_GetConsoleCommands ();
#endif
			}

// process console commands
			if (!(pr_imitatemvdsv.value || dpcompat_nopreparse.ival))
				Cbuf_Execute ();
		}

		if (sv.state < ss_active)	//whoops...
			return delay;

		//make sure the worldmodel is actually valid...
		if (sv.world.worldmodel && sv.world.worldmodel->loadstate != MLS_LOADED)
			Mod_LoadModel(sv.world.worldmodel, MLV_ERROR);

		SV_CheckVars ();

// send messages back to the clients that had packets read this frame
		SV_SendClientMessages ();

#ifdef MVD_RECORDING
		SV_SendMVDMessage();
#endif

// send a heartbeat to the master if needed
		SV_Master_Heartbeat ();


#ifdef Q2SERVER
		if (ge && ge->edicts)
			SVQ2_ClearEvents();
#endif
		idletime = 0;
	}
	idletime += timedelta;

// collect timing statistics
	end = Sys_DoubleTime ();
	svs.stats.active += end-start;
	if (svs.stats.maxresponse < end-start)
		svs.stats.maxresponse = end-start;
	if (svs.stats.maxpackets < svs.stats.packets-oldpackets)
		svs.stats.maxpackets = svs.stats.packets-oldpackets;
	svs.stats.count++;
	if (svs.stats.latched_time < end)
	{
		svs.stats.latched_active = svs.stats.active;
		svs.stats.latched_idle = svs.stats.idle;
		svs.stats.latched_packets = svs.stats.packets;
		svs.stats.latched_count = svs.stats.count;
		svs.stats.latched_maxpackets = svs.stats.maxpackets;
		svs.stats.latched_maxresponse = svs.stats.maxresponse;
		svs.stats.latched_time = end + SVSTATS_PERIOD;
		svs.stats.active = 0;
		svs.stats.idle = 0;
		svs.stats.packets = 0;
		svs.stats.count = 0;
		svs.stats.maxresponse = 0;
		svs.stats.maxpackets = 0;
	}
	oldpackets = svs.stats.packets;

	server_frametime += end-start;
	return delay;
}

static void SV_InfoChanged(void *context, const char *key)
{
	size_t i;

#ifdef Q3SERVER
	if (q3)
		q3->sv.ServerinfoChanged(key);
#endif

	if (context != &svs.info && *key == '_')
		return;	//these keys are considered private to originating client/server, and are not broadcast to anyone else

#ifdef MVD_RECORDING
	if (sv.mvdrecording)
		InfoSync_Add(&demo.recorder.infosync, context, key);	//make sure it gets written into mvds too.
#endif
	for (i = 0; i < svs.allocated_client_slots; i++)
	{
		if (svs.clients[i].state >= cs_connected && !svs.clients[i].controller)
		{
			InfoSync_Add(&svs.clients[i].infosync, context, key);
		}
	}
}

/*
===============
SV_InitLocal
===============
*/
extern void Log_Init (void);

void SV_InitLocal (void)
{
	int		i;
	extern	cvar_t	pr_allowbutton1;
	extern	cvar_t	sv_aim;

	extern	cvar_t	pm_bunnyspeedcap;
	extern	cvar_t	pm_bunnyfriction;
	extern	cvar_t	pm_ktjump;
	extern	cvar_t	pm_slidefix;
	extern	cvar_t	pm_airstep;
	extern	cvar_t	pm_pground;
	extern	cvar_t	pm_stepdown;
	extern	cvar_t	pm_walljump;
	extern	cvar_t	pm_slidyslopes;
	extern	cvar_t	pm_autobunny;
	extern	cvar_t	pm_watersinkspeed;
	extern	cvar_t	pm_flyfriction;
	extern	cvar_t	pm_edgefriction;

#ifdef VM_Q1	//cvars for pimping ourselves to ktx...
	static cvar_t qws_name		= CVARF("qws_name",		DISTRIBUTION,			CVAR_NOSET );
	static cvar_t qws_fullname	= CVARF("qws_fullname", FULLENGINENAME,			CVAR_NOSET );
	static cvar_t qws_version	= CVARF("qws_version",	STRINGIFY(FTE_VER_MAJOR)"."STRINGIFY(FTE_VER_MINOR),CVAR_NOSET );
	static cvar_t qws_buildnum	= CVARF("qws_buildnum",	STRINGIFY(SVNREVISION),	CVAR_NOSET );
#ifdef FTE_TARGET_WEB
	static cvar_t qws_platform	= CVARF("qws_platform",	PLATFORM,				CVAR_NOSET );
#else
	static cvar_t qws_platform	= CVARF("qws_platform",	PLATFORM "-" ARCH_CPU_POSTFIX,				CVAR_NOSET );
#endif
	static cvar_t qws_builddate	= CVARF("qws_builddate",STRINGIFY(SVNDATE),		CVAR_NOSET );
	static cvar_t qws_homepage	= CVARF("qws_homepage",	ENGINEWEBSITE,		CVAR_NOSET );
	Cvar_Register(&qws_name,		"Server Info");
	Cvar_Register(&qws_fullname,	"Server Info");
	Cvar_Register(&qws_version,		"Server Info");
	Cvar_Register(&qws_buildnum,	"Server Info");
	Cvar_Register(&qws_platform,	"Server Info");
	Cvar_Register(&qws_builddate,	"Server Info");
	Cvar_Register(&qws_homepage,	"Server Info");
#endif

	SV_InitOperatorCommands	();
	SV_UserInit ();

#ifndef SERVERONLY
	if (isDedicated)
#endif
	{
		Cvar_Register (&password,	cvargroup_servercontrol);
		Cvar_Register (&rcon_password,	cvargroup_servercontrol);

		Log_Init();
	}
	rcon_password.restriction = RESTRICT_MAX;	//no cheatie rconers changing rcon passwords...
	Cvar_Register (&sv_rconlim,	cvargroup_servercontrol);
	Cvar_Register (&sv_crypt_rcon,	cvargroup_servercontrol);
	Cvar_Register (&spectator_password,	cvargroup_servercontrol);

	Cvar_Register (&sv_mintic,	cvargroup_servercontrol);
	Cvar_Register (&sv_maxtic,	cvargroup_servercontrol);
	Cvar_Register (&sv_limittics,	cvargroup_servercontrol);

	Cvar_Register (&fraglimit,	cvargroup_serverinfo);
	Cvar_Register (&timelimit,	cvargroup_serverinfo);
	Cvar_Register (&teamplay,	cvargroup_serverinfo);
	Cvar_Register (&coop,	cvargroup_serverinfo);
	Cvar_Register (&skill,	cvargroup_serverinfo);
	Cvar_Register (&samelevel,	cvargroup_serverinfo);
	Cvar_Register (&maxclients,	cvargroup_serverinfo);
	Cvar_Register (&maxspectators,	cvargroup_serverinfo);
	Cvar_Register (&sv_playerslots,	cvargroup_serverinfo);
	Cvar_Register (&hostname,	cvargroup_serverinfo);
	Cvar_Register (&deathmatch,	cvargroup_serverinfo);
	Cvar_Register (&spawn,	cvargroup_servercontrol);

	//arguably cheats. Must be switched on to use.
	Cvar_Register (&watervis,	cvargroup_serverinfo);
	Cvar_Register (&allow_skybox,	cvargroup_serverinfo);
	Cvar_Register (&sv_allow_splitscreen,	cvargroup_serverinfo);
	Cvar_Register (&fbskins,	cvargroup_serverinfo);

	Cvar_Register (&zombietime,	cvargroup_servercontrol);

	Cvar_Register (&sv_pupglow,	cvargroup_serverinfo);

	Cvar_Register (&sv_bigcoords,			cvargroup_serverphysics);

	Cvar_Register (&pm_bunnyspeedcap,		cvargroup_serverphysics);
	Cvar_Register (&pm_bunnyfriction,		cvargroup_serverphysics);
	Cvar_Register (&pm_watersinkspeed,		cvargroup_serverphysics);
	Cvar_Register (&pm_flyfriction,			cvargroup_serverphysics);
	Cvar_Register (&pm_ktjump,				cvargroup_serverphysics);
	Cvar_Register (&pm_slidefix,			cvargroup_serverphysics);
	Cvar_Register (&pm_slidyslopes,			cvargroup_serverphysics);
	Cvar_Register (&pm_autobunny,			cvargroup_serverphysics);
	Cvar_Register (&pm_airstep,				cvargroup_serverphysics);
	Cvar_Register (&pm_pground,				cvargroup_serverphysics);
	Cvar_Register (&pm_stepdown,			cvargroup_serverphysics);
	Cvar_Register (&pm_walljump,			cvargroup_serverphysics);
	Cvar_Register (&pm_edgefriction,		cvargroup_serverphysics);

	Cvar_Register (&sv_compatiblehulls,		cvargroup_serverphysics);
	Cvar_Register (&dpcompat_stats,			"Darkplaces compatibility");

	for (i = 0; i < sizeof(sv_motd)/sizeof(sv_motd[0]); i++)
		Cvar_Register(&sv_motd[i],	cvargroup_serverinfo);

	Cvar_Register (&sv_aim,	cvargroup_servercontrol);

	Cvar_Register (&sv_resetparms,	cvargroup_servercontrol);

	if (isDedicated)
		sv_public.enginevalue = "1";

	Cvar_Register (&sv_protocol,	cvargroup_servercontrol);
	Cvar_Register (&sv_guidhash,	cvargroup_servercontrol);
	Cvar_Register (&sv_serverip,	cvargroup_servercontrol);
	Cvar_Register (&sv_public,	cvargroup_servercontrol);
	sv_public.restriction = RESTRICT_MAX;	//no disabling this over rcon.
	Cvar_Register (&sv_listen_qw,	cvargroup_servercontrol);
	Cvar_Register (&sv_listen_nq,	cvargroup_servercontrol);
	Cvar_Register (&sv_listen_dp,	cvargroup_servercontrol);
#ifdef QWOVERQ3
	Cvar_Register (&sv_listen_q3,	cvargroup_servercontrol);
#endif
	sv_listen_qw.restriction = RESTRICT_MAX;	//no disabling this over rcon.
	Cvar_Register (&sv_reconnectlimit,	cvargroup_servercontrol);
	Cvar_Register (&sv_use_dns, cvargroup_servercontrol);
	Cvar_Register (&fraglog_public,	cvargroup_servercontrol);

	SVNET_RegisterCvars();

	Cvar_Register (&sv_reportheartbeats, cvargroup_servercontrol);
	Cvar_Register (&sv_heartbeat_interval, cvargroup_servercontrol);
	Cvar_Register (&sv_heartbeat_checks, cvargroup_servercontrol);

	Cvar_Register (&sv_showconnectionlessmessages, cvargroup_servercontrol);
	Cvar_Register (&sv_banproxies, cvargroup_serverpermissions);
#ifdef SV_MASTER
	Cvar_Register (&sv_master,	cvargroup_servercontrol);
#endif

	Cvar_Register (&filterban,	cvargroup_servercontrol);

	Cvar_Register (&allow_download,	cvargroup_serverpermissions);
	Cvar_Register (&allow_download_skins,	cvargroup_serverpermissions);
	Cvar_Register (&allow_download_models,	cvargroup_serverpermissions);
	Cvar_Register (&allow_download_sounds,	cvargroup_serverpermissions);
	Cvar_Register (&allow_download_particles,	cvargroup_serverpermissions);
	Cvar_Register (&allow_download_maps,	cvargroup_serverpermissions);
	Cvar_Register (&allow_download_logs,	cvargroup_serverpermissions);
	Cvar_Register (&allow_download_demos,	cvargroup_serverpermissions);
	Cvar_Register (&allow_download_anymap,	cvargroup_serverpermissions);
	Cvar_Register (&allow_download_pakcontents,	cvargroup_serverpermissions);
	Cvar_Register (&allow_download_textures,cvargroup_serverpermissions);
	Cvar_Register (&allow_download_configs,	cvargroup_serverpermissions);
	Cvar_Register (&allow_download_locs,	cvargroup_serverpermissions);
	Cvar_Register (&allow_download_packages,cvargroup_serverpermissions);
	Cvar_Register (&allow_download_refpackages,cvargroup_serverpermissions);
	Cvar_Register (&allow_download_wads,	cvargroup_serverpermissions);
	Cvar_Register (&allow_download_root,	cvargroup_serverpermissions);
	Cvar_Register (&allow_download_copyrighted,	cvargroup_serverpermissions);
	Cvar_Register (&allow_download_other,	cvargroup_serverpermissions);
	Cvar_Register (&secure,	cvargroup_serverpermissions);

	Cvar_Register (&sv_highchars,	cvargroup_servercontrol);
	Cvar_Register (&sv_calcphs,	cvargroup_servercontrol);
	Cvar_Register (&sv_phs,	cvargroup_servercontrol);
	Cvar_Register (&sv_cullplayers_trace, cvargroup_servercontrol);
	Cvar_Register (&sv_cullentities_trace, cvargroup_servercontrol);

	Cvar_Register (&sv_csqc_progname,	cvargroup_servercontrol);
	Cvar_Register (&sv_csqcdebug, cvargroup_servercontrol);
	Cvar_Register (&sv_specprint, cvargroup_serverpermissions);

	Cvar_Register (&sv_reliable_sound, cvargroup_serverphysics);
	Cvar_Register (&sv_gamespeed, cvargroup_serverphysics);
	Cvar_Register (&sv_nqplayerphysics,	cvargroup_serverphysics);
	Cvar_Register (&pr_allowbutton1, cvargroup_servercontrol);

	Cvar_Register (&pausable,	cvargroup_servercontrol);

	Cvar_Register (&sv_maxrate, cvargroup_servercontrol);
	Cvar_Register (&sv_maxdrate, cvargroup_servercontrol);
	Cvar_ForceCallback(&sv_maxrate);
	Cvar_ForceCallback(&sv_maxdrate);
	Cvar_Register (&sv_minping, cvargroup_servercontrol);

	Cvar_Register (&sv_nailhack, cvargroup_servercontrol);
	Cvar_Register (&sv_nopvs, cvargroup_servercontrol);

	Cmd_AddCommand ("sv_impulse", SV_Impulse_f);

	Cmd_AddCommand ("openroute", SV_OpenRoute_f);

#ifdef SAVEDGAMES
#if !defined(NOBUILTINMENUS) && !defined(SERVERONLY)
	Cvar_Register(&sv_autosave, cvargroup_servercontrol);
#endif
	Cvar_Register(&sv_savefmt, cvargroup_servercontrol);
#ifndef QUAKETC
	Cmd_AddCommandAD ("savegame_legacy", SV_Savegame_f, SV_Savegame_c, "Saves the game in a format compatible with vanilla Quake. Anything not supported by that format will be lost.");
#endif
	Cmd_AddCommandAD ("savegame", SV_Savegame_f, SV_Savegame_c, "Saves the game to the named location.");
	Cmd_AddCommandAD ("loadgame", SV_Loadgame_f, SV_Savegame_c, "Loads an existing saved game.");
	Cmd_AddCommandAD ("save", SV_Savegame_f, SV_Savegame_c, "Saves the game to the named location.");
	Cmd_AddCommandAD ("load", SV_Loadgame_f, SV_Savegame_c, "Loads an existing saved game.");
	Cmd_AddCommandAD ("unsavegame", SV_DeleteSavegame_f, SV_Savegame_c, "Wipes an existing saved game from disk.");
#endif

#ifdef MVD_RECORDING
	SV_MVDInit();
#endif

	svs.info.ChangeCB = SV_InfoChanged;
	svs.info.ChangeCTX = &svs.info;
	InfoBuf_SetValueForStarKey (&svs.info, "*version", version_string());
	InfoBuf_SetValueForStarKey (&svs.info, "*z_ext", va("%i", SERVER_SUPPORTED_Z_EXTENSIONS));

	// init fraglog stuff
	svs.logsequence = 1;
	svs.logtime = realtime;
	for (i = 0; i < FRAGLOG_BUFFERS; i++)
	{
		svs.log[i].data = svs.log_buf[i];
		svs.log[i].maxsize = sizeof(svs.log_buf[i]);
		svs.log[i].cursize = 0;
		svs.log[i].allowoverflow = true;
	}

	svs.free_lagged_packet = NULL;
}

#define iswhite(c) ((c) == ' ' || (unsigned char)(c) == (unsigned char)INVIS_CHAR1 || (unsigned char)(c) == (unsigned char)INVIS_CHAR2 || (unsigned char)(c) == (unsigned char)INVIS_CHAR3)
#define isinvalid(c) ((c) == ':' || (c) == '\\' || (c) == '$' || (c) == '\r' || (c) == '\n' || (unsigned char)(c) == (unsigned char)0xff || (c) == '\"')
//colon is so clients can't get confused while parsing chats (eg frag messages)
//255 is so fuhquake/ezquake don't end up with nameless players (and general MSG_ReadString bugs)
//" is so mods that use player names in tokenizing/frik_files don't mess up. mods are still expected to be able to cope with space.
//\ is blocked because it messes up our ^[NAME\player\NUM^] links, and because vanilla would hate it.
//$ is blocked because of potential internationalisation escapes.

//is allowed to shorten, out must be as long as in and min of "unnamed"+1
void SV_FixupName(const char *in, char *out, unsigned int outlen)
{
	char *s, *p;
	unsigned int len, codepoint, codeflags;
	conchar_t testbuf[1024], *t, *n, *e;

	if (outlen == 0)
		return;

	len = outlen;

	s = out;
	while(iswhite(*in) || isinvalid(*in) || *in == '\1' || *in == '\2')	//1 and 2 are to stop clients from printing the entire line as chat. only do that for the leading charater.
		in++;
	while(*in && len > 1)
	{
		if (isinvalid(*in))
		{	//chars that cause a problem.
			in++;
			continue;
		}
		*s++ = *in++;
		len--;
	}
	*s = '\0';

	/*note: clients are not guarenteed to parse things the same as the server. utf-8 surrogates may be awkward here*/
	e = COM_ParseFunString(CON_WHITEMASK, out, testbuf, sizeof(testbuf), false);
	for (t = testbuf; t < e; t = n)
	{
		n = Font_Decode(t, &codeflags, &codepoint);
		/*reject anything hidden in names*/
		if ((codeflags & CON_HIDDEN) || (codeflags&(CON_LINKSPECIAL|CON_RICHFORECOLOUR))==CON_LINKSPECIAL)
			break;
		/*reject pictograms*/
		if (codepoint >= 0xe100 && codepoint < 0xe200)
			break;
		if (!sv_highchars.ival && (codeflags & CON_2NDCHARSETTEXT))
			break;
		/*FIXME: should we try to ensure that the chars are in most fonts? that might annoy speakers of more exotic languages I suppose. cvar it?*/
	}
	//and spit it out again, which makes sure there's no weird markup that might screw up other strings.
	COM_DeFunString(testbuf, t, out, outlen, false, false);

	if (!*out || (t < e) || e == testbuf)
	{	//reached end and it was all whitespace
		//white space only
		strncpy(out, "unnamed", outlen);
		out[outlen-1] = 0;
		p = out;
	}

	for (p = out + strlen(out) - 1; p != out && iswhite(*p) ; p--)
		;
	p[1] = 0;
}

void SV_DeDupeName(const char *val, client_t *cl, char *newname, size_t newnamesize)
{
	int		i;
	client_t	*client;
	int		dupc = 1;
	char	basic[80];
	const char *p;
	if (!val)
		val = "";
	//we block large names here because a) they're unwieldly. b) they might cause players to be invisible to older clients/server browsers/etc.
	//bots with no name skip the fixup, to avoid default names(they're expected to be given a name eventually, so are allowed to be invisible for now)
	if (cl && cl->protocol == SCP_BAD && !*val)
		newname[0] = 0;
	else
	{
		SV_FixupName(val, newname, newnamesize);
		if (strlen(newname) > 40)
			newname[40] = 0;
	}

	//strip weirdness
	deleetstring(basic, newname);


	if (!cl || cl->protocol != SCP_BAD)
	{	//don't bother validating bot names. The gamecode is expected to not be stupid.
		if (!basic[0] || strstr(basic, "console"))
			strcpy(newname, "unnamed");

		// check to see if another user by the same name exists
		while (1)
		{
			for (i=0, client = svs.clients ; i<svs.allocated_client_slots ; i++, client++)
			{
				if (client->state < cs_connected || client == cl)
					continue;
				if (!stricmp(client->name, newname))
					break;
			}
			if (i != svs.allocated_client_slots)
			{ // dup name
				char tmpname[80];
				if (strlen(newname) > sizeof(cl->namebuf) - 1)
					newname[sizeof(cl->namebuf) - 4] = 0;
				p = newname;

				if (newname[0] == '(')
				{
					if (newname[1]>='0'&&newname[1]<='9')
					{
						if (newname[2] == ')')
							p = newname + 3;
						else if (newname[2]>='0'&&newname[2]<='9')
						{
							if (newname[3] == ')')
								p = newname + 4;
							else if (newname[3]>='0'&&newname[3]<='9')
							{
								if (newname[4] == ')')
									p = newname + 5;
							}
						}
					}
				}

				memcpy(tmpname, p, strlen(p)+1);

				Q_snprintfz(newname, newnamesize, "(%d)%-.40s", dupc++, tmpname);
			}
			else
				break;
		}
	}
}


qboolean ReloadRanking(client_t *cl, const char *newname)
{
#ifdef SVRANKING
	int newid;
	int j;
	rankstats_t rs;
	newid = Rank_GetPlayerID(cl->guid, newname, atoi(InfoBuf_ValueForKey (&cl->userinfo, "_pwd")), true, false);	//'_' keys are always stripped. On any server. So try and use that so persistant data won't give out the password when connecting to a different server
	if (!newid)
		newid = Rank_GetPlayerID(cl->guid, newname, atoi(InfoBuf_ValueForKey (&cl->userinfo, "password")), true, false);
	if (newid)
	{
		if (cl->rankid && cl->state >= cs_spawned)//apply current stats
		{
			if (!Rank_GetPlayerStats(cl->rankid, &rs))
				return false;
			rs.timeonserver += realtime - cl->stats_started;
			cl->stats_started = realtime;
			rs.kills += cl->kills;
			rs.deaths += cl->deaths;

			rs.flags1 &= ~(RANK_CUFFED|RANK_MUTED|RANK_CRIPPLED);
//			if (cl->iscuffed==2)
//				rs.flags1 |= RANK_CUFFED;
//			if (cl->ismuted==2)
//				rs.flags1 |= RANK_MUTED;
//			if (cl->iscrippled==2)
//				rs.flags1 |= RANK_CRIPPLED;
			cl->kills=0;
			cl->deaths=0;
			pr_global_struct->self = EDICT_TO_PROG(svprogfuncs, cl->edict);
			if (pr_global_ptrs->SetChangeParms)
				PR_ExecuteProgram (svprogfuncs, pr_global_struct->SetChangeParms);
			for (j=0 ; j<NUM_RANK_SPAWN_PARMS ; j++)
				if (pr_global_ptrs->spawnparamglobals[j])
					rs.parm[j] = *pr_global_ptrs->spawnparamglobals[j];
			Rank_SetPlayerStats(cl->rankid, &rs);
			cl->rankid = 0;
		}
		if (!Rank_GetPlayerStats(newid, &rs))
			return false;
		cl->rankid = newid;
		if (rs.flags1 & RANK_CUFFED)
			cl->penalties |= BAN_CUFF;
		if (rs.flags1 & RANK_MUTED)
			cl->penalties |= BAN_MUTE;
		if (rs.flags1 & RANK_CRIPPLED)
			cl->penalties |= BAN_CRIPPLED;

		cl->trustlevel = rs.trustlevel;
		return true;
	}
#endif
	return false;
}
/*
=================
SV_ExtractFromUserinfo

Pull specific info from a newly changed userinfo string
into a more C freindly form.
=================
*/
void SV_ExtractFromUserinfo (client_t *cl, qboolean verbose)
{
	const char	*val, *p;
	char	newname[80];
#ifdef SVRANKING
	extern cvar_t rank_filename;
#endif
	size_t blobsize;
	qboolean large;

	int bottom = atoi(InfoBuf_ValueForKey(&cl->userinfo, "bottomcolor"));

	if (progstype == PROG_NQ)
		p = va("t%u", bottom);
	else
		p = InfoBuf_ValueForKey(&svs.localinfo, va("team%u", bottom));
	val = InfoBuf_ValueForKey (&cl->userinfo, "team");
	if (*p && strcmp(p, val))
	{
		InfoBuf_SetValueForKey(&cl->userinfo, "team", p);
		if (verbose)
			SV_BroadcastUserinfoChange(cl, true, "team", p);
	}
	Q_strncpyz (cl->team, val, sizeof(cl->teambuf));

	// name for C code
	val = InfoBuf_BlobForKey (&cl->userinfo, "name", &blobsize, &large);
	if (large)	//don't allow it if there's even anything weird in there (simplies the necessary qc).
		val = "";
	//fixup and dedupe
	SV_DeDupeName(val, cl, newname, sizeof(newname));

	if (!cl->drop && strncmp(newname, cl->name, sizeof(cl->namebuf)-1))
	{
		if ((cl->penalties & BAN_MUTE) && *cl->name && verbose)	//!verbose is a gamecode-forced update, where the gamecode is expected to know what its doing.
		{
			if (!(cl->penalties & BAN_STEALTH))
				SV_ClientTPrintf (cl, PRINT_HIGH, "Muted players may not change their names\n");

			Q_strncpyz (newname, cl->name, sizeof(newname));
		}

		if (!sv.paused && *cl->name)
		{
			if (!cl->lastnametime || realtime - cl->lastnametime > 5)
			{
				cl->lastnamecount = 0;
				cl->lastnametime = realtime;
			}
			else if (cl->lastnamecount++ > 4 && verbose)
			{
				SV_AutoAddPenalty (cl, BAN_MUTE, 60*5, "Muted for name spam");
				Q_strncpyz (newname, cl->name, sizeof(newname));
			}
		}

		if (!cl->drop && strncmp(newname, cl->name, sizeof(cl->namebuf)-1) && cl->state > cs_zombie)
		{
			if (*cl->name && cl->state >= cs_spawned && !cl->spectator && verbose)
			{
				SV_BroadcastTPrintf (PRINT_HIGH, "%s changed their name to %s\n", cl->name, newname);
			}
			Q_strncpyz (cl->name, newname, sizeof(cl->namebuf));

			if (svprogfuncs)
				svprogfuncs->SetStringField(svprogfuncs, cl->edict, &cl->edict->v->netname, cl->name, true);

#ifdef SVRANKING
			if (ReloadRanking(cl, newname))
			{
			}
			else if (cl->state >= cs_spawned && *rank_filename.string && verbose)
				SV_ClientTPrintf(cl, PRINT_HIGH, "Your rankings name has not been changed\n");
#endif
		}
	}

	//make CERTAIN that the name we think they're using is actually the name everyone else sees too.
	//bots are allowed empty names. this gives the gamecode a chance to actually assign one.
	if (cl->protocol != SCP_BAD)
	{
		InfoBuf_SetValueForKey (&cl->userinfo, "name", newname);
		val = InfoBuf_ValueForKey (&cl->userinfo, "name");
		if (!*val)
		{
			SV_BroadcastTPrintf (PRINT_HIGH, "corrupt userinfo for player %s\n", cl->name);
			cl->drop = true;
		}
	}

	val = InfoBuf_ValueForKey (&cl->userinfo, "lang");
	cl->language = *val?TL_FindLanguage(val):com_language;

	val = InfoBuf_ValueForKey (&cl->userinfo, "nogib");
	cl->gibfilter = !!atoi(val);

	// rate command
	val = InfoBuf_ValueForKey (&cl->userinfo, "rate");
	if (strlen(val))
		cl->rate = atoi(val);
	else
		cl->rate = 0;//0 means no specific limit, limited only by sv_maxrate.

	val = InfoBuf_ValueForKey (&cl->userinfo, "dupe");
	cl->netchan.dupe = bound(0, atoi(val), 5);

	val = InfoBuf_ValueForKey (&cl->userinfo, "drate");
	if (strlen(val))
		cl->drate = atoi(val);
	else
		cl->drate = 0;	//0 disables rate limiting while downloading

#ifdef HEXEN2
	val = InfoBuf_ValueForKey (&cl->userinfo, "cl_playerclass");
	if (val)
	{
		PRH2_SetPlayerClass(cl, atoi(val), false);
	}
#endif

	val = InfoBuf_ValueForKey (&cl->userinfo, "noaim");
	if (atoi(val) > 0)
		cl->autoaimdot = 2; //disable, ignoring sv_aim
	else
	{
		val = InfoBuf_ValueForKey (&cl->userinfo, "aim");
		if (*val)
		{
			cl->autoaimdot = atof(val);
			if (cl->autoaimdot > 1)
				cl->autoaimdot = cos(cl->autoaimdot * M_PI/180);//interpret it as an accepted angle in degrees
		}
		else
			cl->autoaimdot = sv_aim.value;
	}

	// msg command
	val = InfoBuf_ValueForKey (&cl->userinfo, "msg");
	if (strlen(val))
	{
		cl->messagelevel = atoi(val);
	}

	val = InfoBuf_ValueForKey (&cl->userinfo, "sp");	//naming for compat with mvdsv
	if (*val)
		cl->spec_print = atoi(val);
	else
		cl->spec_print = ~0;	//if unspecified, default to server setting (even if the cvar is changed mid-map).

#ifdef NQPROT
	{
		int top = atoi(InfoBuf_ValueForKey(&cl->userinfo, "topcolor"));
		top &= 15;
		if (top > 13)
			top = 13;
		bottom &= 15;
		if (bottom > 13)
			bottom = 13;
		if (cl->playercolor != top*16 + bottom)
		{
			cl->playercolor = top*16 + bottom;
			if (svs.gametype == GT_PROGS || svs.gametype == GT_Q1QVM)
			{
#ifdef HAVE_LEGACY
				if (cl->edict)
					cl->edict->xv->clientcolors = cl->playercolor;
#endif
				MSG_WriteByte (&sv.nqreliable_datagram, svc_updatecolors);
				MSG_WriteByte (&sv.nqreliable_datagram, cl-svs.clients);
				MSG_WriteByte (&sv.nqreliable_datagram, cl->playercolor);
			}
		}
	}
#endif
}

//============================================================================

/*
====================
SV_InitNet
====================
*/
void SV_InitNet (void)
{
#ifndef SERVERONLY
	if (isDedicated)
#endif
	{
		Netchan_Init ();
	}

	SV_Master_ReResolve();

//	NET_StringToAdr ("192.246.40.70:27000", &idmaster_adr);
}

/*
====================
SV_Init
====================
*/
#ifdef SERVER_DEMO_PLAYBACK
//FIXME: move to header
void SV_Demo_Init(void);
#endif

void SV_ArgumentOverrides(void)
{
	int p;
	// parse params for cvars
	p = COM_CheckParm ("-svport");
	if (!p)
		p = COM_CheckParm ("-port");
	if (p && p < com_argc)
	{
		Cvar_Set(Cvar_FindVar("sv_port"), com_argv[p+1]);
		Cvar_Set(Cvar_FindVar("sv_port_tcp"), com_argv[p+1]);
	}
}

void SV_ExecInitialConfigs(char *defaultexec)
{
	Cbuf_AddText("cvar_purgedefaults\n", RESTRICT_LOCAL);	//reset cvar defaults to their engine-specified values. the tail end of 'exec default.cfg' will update non-cheat defaults to mod-specified values.
	Cbuf_AddText("cvarreset *\n", RESTRICT_LOCAL);			//reset all cvars to their current (engine) defaults
	Cbuf_AddText("alias restart \"map_restart\"\n",RESTRICT_LOCAL);

	Cbuf_AddText(va("sv_gamedir \"%s\"\n", FS_GetGamedir(true)), RESTRICT_LOCAL);

	Cbuf_AddText("cl_warncmd 0\n", RESTRICT_LOCAL);
	Cbuf_AddText(defaultexec, RESTRICT_LOCAL);
	Cbuf_AddText("cl_warncmd 1\n", RESTRICT_LOCAL);
	Cbuf_AddText("\n", RESTRICT_LOCAL);
	//make sure +set args override fmf/engine defaults (redundant when there's no map/etc command in configs)
	COM_ParsePlusSets(true);

	if (COM_FileSize("server.cfg") != -1)
		Cbuf_AddText ("cl_warncmd 1\nexec server.cfg\nexec ftesrv.cfg\n", RESTRICT_LOCAL);
	else if (COM_FileSize("quake.rc") != -1)
		Cbuf_AddText ("cl_warncmd 0\nexec quake.rc\ncl_warncmd 1\nexec ftesrv.cfg\n", RESTRICT_LOCAL);
#ifdef HEXEN2
	else if (COM_FileSize("hexen.rc") != -1)	//fixme: some kind of priority thing.
		Cbuf_AddText ("cl_warncmd 0\nexec hexen.rc\ncl_warncmd 1\nexec ftesrv.cfg\n", RESTRICT_LOCAL);
#endif
	else
		Cbuf_AddText ("cl_warncmd 0\nexec default.cfg\ncl_warncmd 1\nexec ftesrv.cfg\n", RESTRICT_LOCAL);

	//make sure +set stuff still applies...
	COM_ParsePlusSets(true);

// process command line arguments
	Cbuf_Execute ();

	SV_ArgumentOverrides();
}

static void SV_StartInitialMap(void)
{
	// if a map wasn't specified on the command line, spawn start.map
	//aliases require that we flush the cbuf in order to actually see the results.
	if (sv.state == ss_dead && Cmd_AliasExist("dedicated_start", RESTRICT_LOCAL))
	{
		Cbuf_AddText("dedicated_start", RESTRICT_LOCAL);	//Q2 feature
		Cbuf_Execute();
	}
	if (sv.state == ss_dead && Cmd_AliasExist("startmap_dm", RESTRICT_LOCAL))
	{
		Cbuf_AddText("startmap_dm", RESTRICT_LOCAL);	//DP extension
		Cbuf_Execute();
	}
	if (sv.state == ss_dead && Cmd_AliasExist("startmap_sp", RESTRICT_LOCAL))
	{
		Cbuf_AddText("startmap_sp", RESTRICT_LOCAL);	//DP extension
		Cbuf_Execute();
	}
	if (sv.state == ss_dead && COM_FCheckExists("maps/start.bsp"))
		Cmd_ExecuteString ("map start", RESTRICT_LOCAL);	//regular q1
#ifdef HEXEN2
	if (sv.state == ss_dead && COM_FCheckExists("maps/demo1.bsp"))
		Cmd_ExecuteString ("map demo1", RESTRICT_LOCAL);	//regular h2 sp
#endif
#ifdef Q2SERVER
	if (sv.state == ss_dead && COM_FCheckExists("maps/base1.bsp"))
		Cmd_ExecuteString ("map base1", RESTRICT_LOCAL);	//regular q2 sp
#endif
#ifdef Q3SERVER
	if (sv.state == ss_dead && COM_FCheckExists("maps/q3dm1.bsp"))
		Cmd_ExecuteString ("map q3dm1", RESTRICT_LOCAL);	//regular q3 'sp'
#endif
#ifdef HLSERVER
	if (sv.state == ss_dead && COM_FCheckExists("maps/c0a0.bsp"))
		Cmd_ExecuteString ("map c0a0", RESTRICT_LOCAL);	//regular hl sp
#endif
}

static void SV_CheckMapless(int iarg, void *data)
{	//kills the server if we're dead
	static int wtf;
	if (sv.state != ss_dead)
		return;	//yay, the situation got resolved!
	else if (HTTP_CL_GetActiveDownloads())
		;	//we still have hope. check again later.
	else
	{
#ifdef PACKAGEMANAGER
		if (!FS_GameIsInitialised())
			wtf = -1;
		if (wtf == 0)
		{
			Cmd_ExecuteString("pkg listenabledsources", RESTRICT_LOCAL);
			Cmd_ExecuteString("pkg refresh", RESTRICT_LOCAL);
		}
		else if (wtf == 1)
		{
			Cmd_ExecuteString("pkg upgrade", RESTRICT_LOCAL);
			Cmd_ExecuteString("pkg apply", RESTRICT_LOCAL);
		}
		else if (wtf == 2)
			Cmd_ExecuteString("fs_restart", RESTRICT_LOCAL);	//just in case...
		else if (wtf == 3)
			SV_StartInitialMap();	//we should be fully up and running, so try starting that initial map again.
		else
#endif
		{
			Cmd_ExecuteString("path", RESTRICT_LOCAL);
			if (COM_CheckParm("-allowmapless"))
			{	//this is risky and mustn't be on by default. there's a load of scripts that check for success or w/e, so best to not make them arbitrarily fail.
				Con_Printf(CON_ERROR"Couldn't load a map. You may need to use the -basedir argument or to install some packages.\n");
				Con_Printf(CON_ERROR"Use the ^[/fs_changegame^] to select the proper basedir/game, or use the ^[/pkg^] command to install required packages, or ^[/quit^] out\n");
			}
			else
				SV_Error (CON_ERROR"Couldn't load a map. You may need to use the -basedir argument.");
			return;
		}
		wtf++;
	}
	Cmd_AddTimer(0.1, SV_CheckMapless, 0, NULL,0);	//we still have hope. check again later.
}

void SV_Init (quakeparms_t *parms)
{
	if (isDedicated)
	{
		COM_InitArgv (parms->argc, parms->argv);


		host_parms = *parms;

		Cvar_Init();
		Memory_Init();

		Sys_Init();

		COM_ParsePlusSets(false);

		Cbuf_Init ();
		Cmd_Init ();
#ifndef SERVERONLY
		R_SetRenderer(NULL);
#endif
		NET_Init ();
		COM_Init ();
#if defined(Q2BSPS) || defined(Q3BSPS)
		CM_Init();
#endif
#ifdef TERRAIN
		Terr_Init();
#endif
		Mod_Init (true);
		Mod_Init (false);

		PF_Common_RegisterCvars();
	}
	else
	{
#if defined(SERVERONLY) || !(defined(CSQC_DAT) || defined(MENU_DAT))
		PF_Common_RegisterCvars();
#endif
	}

	PR_Init ();

	SV_InitNet ();

	SV_InitLocal ();

#ifdef SERVER_DEMO_PLAYBACK
	SV_Demo_Init();
#endif

#ifdef SVRANKING
	Rank_RegisterCommands();
#endif

#ifndef SERVERONLY
	if (isDedicated)
#endif
	{
		int manarg;
		PM_Init ();

#ifdef PLUGINS
		Plug_Initialise(true);
#endif

		Cvar_ParseWatches();
		host_initialized = true;


		manarg = COM_CheckParm("-manifest");
		if (manarg && manarg < com_argc-1 && com_argv[manarg+1])
			FS_ChangeGame(FS_Manifest_ReadSystem(com_argv[manarg+1], NULL), true, true);
		else
			FS_ChangeGame(NULL, true, true);

		Cmd_StuffCmds();

		Menu_Download_Update();

#ifdef MANIFESTDOWNLOADS
		if (Sys_RunInstaller())
			Sys_Quit();
#endif

		Con_Printf ("Exe: %s\n", version_string());

		Con_TPrintf ("======== %s Initialized ========\n", *fs_gamename.string?fs_gamename.string:"Nothing");

#ifdef SUBSERVERS
		if (SSV_IsSubServer())
		{
			NET_InitServer();
			return;
		}
#endif

#ifdef IPLOG
		IPLog_Merge_File("iplog.txt");	//should be compatible with DP's take on the feature.
		IPLog_Merge_File("iplog.dat");	//legacy crap, for compat with proquake
#endif

		SV_StartInitialMap();
		SV_CheckMapless(0,NULL);	//restarts a timer, kill the server only when the pending downloads have ended.
	}
}

#endif

