/*
Copyright (C) 1996-1997 Id Software, Inc.
Copyright (C) 2000-2021 DarkPlaces contributors

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
// host.c -- coordinates spawning and killing of local servers

#include "quakedef.h"

#include <time.h>
#include "libcurl.h"
#include "taskqueue.h"
#include "utf8lib.h"

/*

A server can always be started, even if the system started out as a client
to a remote system.

A client can NOT be started if the system started as a dedicated server.

Memory is cleared / released when a server or client begins, not when they end.

*/

host_static_t host;

// pretend frames take this amount of time (in seconds), 0 = realtime
cvar_t host_framerate = {CF_CLIENT | CF_SERVER, "host_framerate","0", "locks frame timing to this value in seconds, 0.05 is 20fps for example, note that this can easily run too fast, use cl_maxfps if you want to limit your framerate instead, or sys_ticrate to limit server speed"};
// shows time used by certain subsystems
cvar_t host_speeds = {CF_CLIENT | CF_SERVER, "host_speeds","0", "reports how much time is used in server/graphics/sound"};

cvar_t developer = {CF_CLIENT | CF_SERVER | CF_ARCHIVE, "developer","0", "shows debugging messages and information (recommended for all developers and level designers); the value -1 also suppresses buffering and logging these messages"};
cvar_t developer_extra = {CF_CLIENT | CF_SERVER, "developer_extra", "0", "prints additional debugging messages, often very verbose!"};
cvar_t developer_insane = {CF_CLIENT | CF_SERVER, "developer_insane", "0", "prints huge streams of information about internal workings, entire contents of files being read/written, etc.  Not recommended!"};
cvar_t developer_loadfile = {CF_CLIENT | CF_SERVER, "developer_loadfile","0", "prints name and size of every file loaded via the FS_LoadFile function (which is almost everything)"};
cvar_t developer_loading = {CF_CLIENT | CF_SERVER, "developer_loading","0", "prints information about files as they are loaded or unloaded successfully"};
cvar_t developer_entityparsing = {CF_CLIENT, "developer_entityparsing", "0", "prints detailed network entities information each time a packet is received"};

cvar_t timestamps = {CF_CLIENT | CF_SERVER | CF_ARCHIVE, "timestamps", "0", "prints timestamps on console messages"};
cvar_t timeformat = {CF_CLIENT | CF_SERVER | CF_ARCHIVE, "timeformat", "[%Y-%m-%d %H:%M:%S] ", "time format to use on timestamped console messages"};

cvar_t sessionid = {CF_CLIENT | CF_SERVER | CF_READONLY, "sessionid", "", "ID of the current session (use the -sessionid parameter to set it); this is always either empty or begins with a dot (.)"};
cvar_t locksession = {CF_CLIENT | CF_SERVER, "locksession", "0", "Lock the session? 0 = no, 1 = yes and abort on failure, 2 = yes and continue on failure"};

cvar_t host_isclient = {CF_SHARED | CF_READONLY, "_host_isclient", "0", "If 1, clientside is active."};

/*
================
Host_AbortCurrentFrame

aborts the current host frame and goes on with the next one
================
*/
void Host_AbortCurrentFrame(void)
{
	// in case we were previously nice, make us mean again
	Sys_MakeProcessMean();

	longjmp (host.abortframe, 1);
}

/*
================
Host_Error

This shuts down both the client and server
================
*/
void Host_Error (const char *error, ...)
{
	static char hosterrorstring1[MAX_INPUTLINE]; // THREAD UNSAFE
	static char hosterrorstring2[MAX_INPUTLINE]; // THREAD UNSAFE
	static qbool hosterror = false;
	va_list argptr;
	int outfd = sys.outfd;

	// set output to stderr
	sys.outfd = fileno(stderr);

	// turn off rcon redirect if it was active when the crash occurred
	// to prevent loops when it is a networking problem
	Con_Rcon_Redirect_Abort();

	va_start (argptr,error);
	dpvsnprintf (hosterrorstring1,sizeof(hosterrorstring1),error,argptr);
	va_end (argptr);

	Con_Printf(CON_ERROR "Host_Error: %s\n", hosterrorstring1);

	// LadyHavoc: if crashing very early, or currently shutting down, do
	// Sys_Error instead
	if (host.framecount < 1 || host.state != host_active)
		Sys_Error ("Host_Error during %s: %s", host_state_str[host.state], hosterrorstring1);

	if (hosterror)
		Sys_Error ("Host_Error: recursively entered (original error was: %s    new error is: %s)", hosterrorstring2, hosterrorstring1);
	hosterror = true;

	dp_strlcpy(hosterrorstring2, hosterrorstring1, sizeof(hosterrorstring2));

	CL_Parse_DumpPacket();
	CL_Parse_ErrorCleanUp();

	// print out where the crash happened, if it was caused by QC (and do a cleanup)
	PRVM_Crash();

	if(host.hook.SV_Shutdown)
		host.hook.SV_Shutdown();

	if (cls.state == ca_dedicated)
		Sys_Error("Host_Error: %s", hosterrorstring1);        // dedicated servers exit

	// prevent an endless loop if the error was triggered by a command
	Cbuf_Clear(cmd_local->cbuf);

	CL_DisconnectEx(false, "Host_Error: %s", hosterrorstring1);
	cls.demonum = -1; // stop demo loop

	hosterror = false;

	// restore configured outfd
	sys.outfd = outfd;

	// can't abort a frame if we didn't start one yet, won't get here in that case (see above)
	Host_AbortCurrentFrame();
}

/*
==================
Host_Quit_f
==================
*/
static void Host_Quit_f(cmd_state_t *cmd)
{
	if(host.state == host_shutdown)
		Con_Printf("shutting down already!\n");
	else
		host.state = host_shutdown;
}

static void Host_Version_f(cmd_state_t *cmd)
{
	Con_Printf("Version: %s\n", engineversion);
}

void Host_UpdateVersion(void)
{
	dpsnprintf(engineversion, sizeof(engineversion), "%s %s%s %s", gamename ? gamename : "DarkPlaces", DP_OS_NAME, cls.state == ca_dedicated ? " dedicated" : "", buildstring);
}

static void Host_Framerate_c(cvar_t *var)
{
	if (var->value < 0.00001 && var->value != 0)
		Cvar_SetValueQuick(var, 0);
}

// TODO: Find a better home for this.
static void SendCvar_f(cmd_state_t *cmd)
{
	if(cmd->source == src_local && host.hook.SV_SendCvar)
	{
		host.hook.SV_SendCvar(cmd);
		return;
	}
	if(cmd->source == src_client && host.hook.CL_SendCvar)
	{
		host.hook.CL_SendCvar(cmd);
		return;
	}
}

/*
===============
Host_SaveConfig_f

Writes key bindings and archived cvars to config.cfg
===============
*/
void Host_SaveConfig(const char *file)
{
	qfile_t *f;

// dedicated servers initialize the host but don't parse and set the
// config.cfg cvars
	// LadyHavoc: don't save a config if it crashed in startup
	if (host.framecount >= 3 && cls.state != ca_dedicated && !Sys_CheckParm("-benchmark") && !Sys_CheckParm("-capturedemo"))
	{
		f = FS_OpenRealFile(file, "wb", false);
		if (!f)
		{
			Con_Printf(CON_ERROR "Couldn't write %s\n", file);
			return;
		}
		else
			Con_Printf("Saving config to %s ...\n", file);

		Key_WriteBindings (f);
		Cvar_WriteVariables (&cvars_all, f);
#ifdef __EMSCRIPTEN__
		js_syncFS(false);
#endif
		FS_Close (f);
	}
}

static void Host_SaveConfig_f(cmd_state_t *cmd)
{
	const char *file = CONFIGFILENAME;

	if(Cmd_Argc(cmd) >= 2)
		file = Cmd_Argv(cmd, 1);

	Host_SaveConfig(file);
}

static void Host_AddConfigText(cmd_state_t *cmd)
{
	// set up the default startmap_sp and startmap_dm aliases (mods can
	// override these) and then execute the quake.rc startup script
	if (gamemode == GAME_NEHAHRA)
		Cbuf_InsertText(cmd, "alias startmap_sp \"map nehstart\"\nalias startmap_dm \"map nehstart\"\nexec " STARTCONFIGFILENAME "\n");
	else if (gamemode == GAME_TRANSFUSION)
		Cbuf_InsertText(cmd, "alias startmap_sp \"map e1m1\"\n""alias startmap_dm \"map bb1\"\nexec " STARTCONFIGFILENAME "\n");
	else if (gamemode == GAME_TEU)
		Cbuf_InsertText(cmd, "alias startmap_sp \"map start\"\nalias startmap_dm \"map start\"\nexec teu.rc\n");
	else
		Cbuf_InsertText(cmd, "alias startmap_sp \"map start\"\nalias startmap_dm \"map start\"\nexec " STARTCONFIGFILENAME "\n");

	// if quake.rc is missing, use default
	if (!FS_FileExists(STARTCONFIGFILENAME))
		Cbuf_InsertText(cmd, "exec default.cfg\nexec " CONFIGFILENAME "\nexec autoexec.cfg\n");
}

/*
===============
Host_LoadConfig_f

Resets key bindings and cvars to defaults and then reloads scripts
===============
*/
static void Host_LoadConfig_f(cmd_state_t *cmd)
{
#ifdef CONFIG_MENU
	// Xonotic QC complains/breaks if its cvars are deleted before its m_shutdown() is called
	if(MR_Shutdown)
		MR_Shutdown();
#endif
	Cmd_RestoreInitState();
#ifdef CONFIG_MENU
	// Must re-add menu.c commands or load menu.dat before executing quake.rc or handling events
	MR_Init();
#endif
	// exec startup scripts again
	Host_AddConfigText(cmd);
}

/*
=======================
Host_InitLocal
======================
*/
extern cvar_t r_texture_jpeg_fastpicmip;
static void Host_InitLocal (void)
{
	Cmd_AddCommand(CF_SHARED, "quit", Host_Quit_f, "quit the game");
	Cmd_AddCommand(CF_SHARED, "version", Host_Version_f, "print engine version");
	Cmd_AddCommand(CF_SHARED, "saveconfig", Host_SaveConfig_f, "save settings to config.cfg (or a specified filename) immediately (also automatic when quitting)");
	Cmd_AddCommand(CF_SHARED, "loadconfig", Host_LoadConfig_f, "reset everything and reload configs");
	Cmd_AddCommand(CF_SHARED, "sendcvar", SendCvar_f, "sends the value of a cvar to the server as a sentcvar command, for use by QuakeC");
	Cvar_RegisterVariable (&host_framerate);
	Cvar_RegisterCallback (&host_framerate, Host_Framerate_c);
	Cvar_RegisterVariable (&host_speeds);
	Cvar_RegisterVariable (&host_isclient);

	Cvar_RegisterVariable (&developer);
	Cvar_RegisterVariable (&developer_extra);
	Cvar_RegisterVariable (&developer_insane);
	Cvar_RegisterVariable (&developer_loadfile);
	Cvar_RegisterVariable (&developer_loading);
	Cvar_RegisterVariable (&developer_entityparsing);

	Cvar_RegisterVariable (&timestamps);
	Cvar_RegisterVariable (&timeformat);

	Cvar_RegisterVariable (&r_texture_jpeg_fastpicmip);
}

char engineversion[128]; ///< version string for the corner of the console, crash messages, status command, etc


static qfile_t *locksession_fh = NULL;
static qbool locksession_run = false;
static void Host_InitSession(void)
{
	int i;
	char *buf;
	Cvar_RegisterVariable(&sessionid);
	Cvar_RegisterVariable(&locksession);

	// load the session ID into the read-only cvar
	if ((i = Sys_CheckParm("-sessionid")) && (i + 1 < sys.argc))
	{
		if(sys.argv[i+1][0] == '.')
			Cvar_SetQuick(&sessionid, sys.argv[i+1]);
		else
		{
			buf = (char *)Z_Malloc(strlen(sys.argv[i+1]) + 2);
			dpsnprintf(buf, sizeof(buf), ".%s", sys.argv[i+1]);
			Cvar_SetQuick(&sessionid, buf);
		}
	}
}

void Host_LockSession(void)
{
	if(locksession_run)
		return;
	locksession_run = true;
	if(locksession.integer != 0 && !Sys_CheckParm("-readonly"))
	{
		char vabuf[1024];
		char *p = va(vabuf, sizeof(vabuf), "%slock%s", *fs_userdir ? fs_userdir : fs_basedir, sessionid.string);
		FS_CreatePath(p);
		locksession_fh = FS_SysOpen(p, "wl", false);
		// TODO maybe write the pid into the lockfile, while we are at it? may help server management tools
		if(!locksession_fh)
		{
			if(locksession.integer == 2)
			{
				Con_Printf(CON_WARN "WARNING: session lock %s could not be acquired. Please run with -sessionid and an unique session name. Continuing anyway.\n", p);
			}
			else
			{
				Sys_Error("session lock %s could not be acquired. Please run with -sessionid and an unique session name.\n", p);
			}
		}
	}
}

void Host_UnlockSession(void)
{
	if(!locksession_run)
		return;
	locksession_run = false;

	if(locksession_fh)
	{
		FS_Close(locksession_fh);
		// NOTE: we can NOT unlink the lock here, as doing so would
		// create a race condition if another process created it
		// between our close and our unlink
		locksession_fh = NULL;
	}
}

/*
====================
Host_Init
====================
*/
void Host_Init (void)
{
	int i;
	char vabuf[1024];

	Sys_SDL_Init();

	Memory_Init();

	host.hook.ConnectLocal = NULL;
	host.hook.Disconnect = NULL;
	host.hook.ToggleMenu = NULL;
	host.hook.SV_Shutdown = NULL;

	host.state = host_init;

	host.realtime = 0;
	host.dirtytime = Sys_DirtyTime();

	if (Sys_CheckParm("-profilegameonly"))
		Sys_AllowProfiling(false);

	// LadyHavoc: quake never seeded the random number generator before... heh
	if (Sys_CheckParm("-benchmark"))
		srand(0); // predictable random sequence for -benchmark
	else
		srand((unsigned int)time(NULL));

	// FIXME: this is evil, but possibly temporary
	// LadyHavoc: doesn't seem very temporary...
	// LadyHavoc: made this a saved cvar
// COMMANDLINEOPTION: Console: -developer enables warnings and other notices (RECOMMENDED for mod developers)
	if (Sys_CheckParm("-developer"))
	{
		developer.value = developer.integer = 1;
		developer.string = "1";
	}

	if (Sys_CheckParm("-developer2") || Sys_CheckParm("-developer3"))
	{
		developer.value = developer.integer = 1;
		developer.string = "1";
		developer_extra.value = developer_extra.integer = 1;
		developer_extra.string = "1";
		developer_insane.value = developer_insane.integer = 1;
		developer_insane.string = "1";
		developer_memory.value = developer_memory.integer = 1;
		developer_memory.string = "1";
		developer_memorydebug.value = developer_memorydebug.integer = 1;
		developer_memorydebug.string = "1";
	}

	if (Sys_CheckParm("-developer3"))
	{
		gl_paranoid.integer = 1;gl_paranoid.string = "1";
		gl_printcheckerror.integer = 1;gl_printcheckerror.string = "1";
	}

	// -dedicated is checked in SV_ServerOptions() but that's too late for Cvar_RegisterVariable() to skip all the client-only cvars
	if (Sys_CheckParm ("-dedicated") || !cl_available)
		cls.state = ca_dedicated;

	// set and print initial version string (will be updated when gamename is changed)
	Host_UpdateVersion(); // checks for cls.state == ca_dedicated
	Con_Printf("%s\n", engineversion);

	// initialize console command/cvar/alias/command execution systems
	Cmd_Init();

	// initialize memory subsystem cvars/commands
	Memory_Init_Commands();

	// initialize console and logging and its cvars/commands
	// this enables Con_Printf() messages to be coloured
	Con_Init();

	// initialize various cvars that could not be initialized earlier
	u8_Init();
	Curl_Init_Commands();
	Sys_Init_Commands();
	COM_Init_Commands();

	// initialize filesystem (including fs_basedir, fs_gamedir, -game, scr_screenshot_name, gamename)
	FS_Init();

	// initialize process nice level
	Sys_InitProcessNice();

	// initialize ixtable
	Mathlib_Init();

	// register the cvars for session locking
	Host_InitSession();

	// must be after FS_Init
	Crypto_Init();
	Crypto_Init_Commands();

	NetConn_Init();
	Curl_Init();
	PRVM_Init();
	Mod_Init();
	World_Init();
	SV_Init();
	Host_InitLocal();

	Thread_Init();
	TaskQueue_Init();

	CL_Init();

	// save off current state of aliases, commands and cvars for later restore if FS_GameDir_f is called
	// NOTE: menu commands are freed by Cmd_RestoreInitState
	Cmd_SaveInitState();

	// here comes the not so critical stuff

	Host_AddConfigText(cmd_local);
	Cbuf_Execute(cmd_local->cbuf); // cannot be in Host_AddConfigText as that would cause Host_LoadConfig_f to loop!

	CL_StartVideo();

	Log_Start();

	if (cls.state != ca_dedicated)
	{
		// put up the loading image so the user doesn't stare at a black screen...
		SCR_BeginLoadingPlaque(true);
		S_Startup();
#ifdef CONFIG_MENU
		MR_Init();
#endif
	}

	// check for special benchmark mode
// COMMANDLINEOPTION: Client: -benchmark <demoname> runs a timedemo and quits, results of any timedemo can be found in gamedir/benchmark.log (for example id1/benchmark.log)
	i = Sys_CheckParm("-benchmark");
	if (i && i + 1 < sys.argc)
	if (!sv.active && !cls.demoplayback && !cls.connect_trying)
	{
		Cbuf_AddText(cmd_local, va(vabuf, sizeof(vabuf), "timedemo %s\n", sys.argv[i + 1]));
		Cbuf_Execute(cmd_local->cbuf);
	}

	// check for special demo mode
// COMMANDLINEOPTION: Client: -demo <demoname> runs a playdemo and quits
	i = Sys_CheckParm("-demo");
	if (i && i + 1 < sys.argc)
	if (!sv.active && !cls.demoplayback && !cls.connect_trying)
	{
		Cbuf_AddText(cmd_local, va(vabuf, sizeof(vabuf), "playdemo %s\n", sys.argv[i + 1]));
		Cbuf_Execute(cmd_local->cbuf);
	}

#ifdef CONFIG_VIDEO_CAPTURE
// COMMANDLINEOPTION: Client: -capturedemo <demoname> captures a playdemo and quits
	i = Sys_CheckParm("-capturedemo");
	if (i && i + 1 < sys.argc)
	if (!sv.active && !cls.demoplayback && !cls.connect_trying)
	{
		Cbuf_AddText(cmd_local, va(vabuf, sizeof(vabuf), "playdemo %s\ncl_capturevideo 1\n", sys.argv[i + 1]));
		Cbuf_Execute((cmd_local)->cbuf);
	}
#endif

	if (cls.state == ca_dedicated || Sys_CheckParm("-listen"))
	if (!sv.active && !cls.demoplayback && !cls.connect_trying)
	{
		Cbuf_AddText(cmd_local, "startmap_dm\n");
		Cbuf_Execute(cmd_local->cbuf);
	}

	if (!sv.active && !cls.demoplayback && !cls.connect_trying)
	{
#ifdef CONFIG_MENU
		Cbuf_AddText(cmd_local, "togglemenu 1\n");
#endif
		Cbuf_Execute(cmd_local->cbuf);
	}

	Con_DPrint("========Initialized=========\n");
	host.state = host_active;

	if (cls.state != ca_dedicated)
		SV_StartThread();
}

/*
===============
Host_Shutdown

Cleanly shuts down, Host_Frame() must not be called again after this.
===============
*/
void Host_Shutdown(void)
{
	if (Sys_CheckParm("-profilegameonly"))
		Sys_AllowProfiling(false);

	if(cls.state != ca_dedicated)
		CL_Shutdown();

	// end the server thread
	if (svs.threaded)
		SV_StopThread();

	// shut down local server if active
	if(host.hook.SV_Shutdown)
		host.hook.SV_Shutdown();

	// AK shutdown PRVM
	// AK hmm, no PRVM_Shutdown(); yet

	Host_SaveConfig(CONFIGFILENAME);

	Curl_Shutdown ();
	NetConn_Shutdown ();

	SV_StopThread();
	TaskQueue_Shutdown();
	Thread_Shutdown();
	Cmd_Shutdown();
	Sys_SDL_Shutdown();
	Log_Close();
	Crypto_Shutdown();

	Host_UnlockSession();

	Con_Shutdown();
	Memory_Shutdown();
}

//============================================================================

/*
==================
Host_Frame

Runs all active servers
==================
*/
double Host_Frame(double time)
{
	double cl_wait, sv_wait;

	++host.framecount;

	TaskQueue_Frame(false);

	// keep the random time dependent, but not when playing demos/benchmarking
	if(!*sv_random_seed.string && !host.restless)
		rand();

	NetConn_UpdateSockets();

	Log_DestBuffer_Flush();

	// Run any downloads
	Curl_Frame();

	// get new SDL events and add commands from keybindings to the cbuf
	Sys_SDL_HandleEvents();

	// process console commands
	Cbuf_Frame(host.cbuf);

	R_TimeReport("---");

	// if the accumulators haven't become positive yet, wait a while
	sv_wait = - SV_Frame(time);
	cl_wait = - CL_Frame(time);

	Mem_CheckSentinelsGlobal();

	if (cls.state == ca_dedicated)
		return sv_wait; // dedicated
	else if (!sv.active || svs.threaded)
		return cl_wait; // connected to server, main menu, or server is on different thread
	else
		return min(cl_wait, sv_wait); // listen server or singleplayer
}
