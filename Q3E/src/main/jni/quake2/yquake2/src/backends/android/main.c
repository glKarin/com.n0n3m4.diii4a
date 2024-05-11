/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * This file is the starting point of the program. It does some platform
 * specific initialization stuff and calls the common initialization code.
 *
 * =======================================================================
 */

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/stat.h>
#ifndef FNDELAY
#define FNDELAY O_NDELAY
#endif

#include "../../common/header/common.h"

#include "../../client/input/header/input.h"
#include "../../client/header/keyboard.h"
#include "../../client/header/client.h"

void registerHandler(void);

qboolean IsInitialized = false;

/* Android */

void Qcommon_Mainloop(void);

#include "../android/sys_android.c"

// command line arguments
static int q3e_argc = 0;
static char **q3e_argv = NULL;

// game main thread
static pthread_t				main_thread;

// app exit
volatile qboolean q3e_running = false;

void GLimp_CheckGLInitialized(void)
{
	Q3E_CheckNativeWindowChanged();
}

static void Q3E_DumpArgs(int argc, const char **argv)
{
	q3e_argc = argc;
	q3e_argv = (char **) malloc(sizeof(char *) * argc);
	for (int i = 0; i < argc; i++)
	{
		q3e_argv[i] = strdup(argv[i]);
	}
}

static void Q3E_FreeArgs(void)
{
	for(int i = 0; i < q3e_argc; i++)
	{
		free(q3e_argv[i]);
	}
	free(q3e_argv);
}

// quake2 game main thread loop
static void * quake2_main(void *data)
{
	attach_thread(); // attach current to JNI for call Android code

	Q3E_Start();

	Com_Printf("[Harmattan]: Enter quake2 main thread -> %s\n", "main");
	// register signal handler
	registerHandler();

	// Setup FPU if necessary
	Sys_SetupFPU();

	// Implement command line options that the rather
	// crappy argument parser can't parse.
	for (int i = 0; i < q3e_argc; i++)
	{
		// Are we portable?
		if (strcmp(q3e_argv[i], "-portable") == 0)
		{
			is_portable = true;
		}

		// Inject a custom data dir.
		if (strcmp(q3e_argv[i], "-datadir") == 0)
		{
			// Mkay, did the user give us an argument?
			if (i != (q3e_argc - 1))
			{
				// Check if it exists.
				struct stat sb;

				if (stat(q3e_argv[i + 1], &sb) == 0)
				{
					if (!S_ISDIR(sb.st_mode))
					{
						printf("-datadir %s is not a directory\n", q3e_argv[i + 1]);
						return (void *)(intptr_t)1;
					}

					if(realpath(q3e_argv[i + 1], datadir) == NULL)
					{
						printf("realpath(datadir) failed: %s\n", strerror(errno));
						datadir[0] = '\0';
					}
				}
				else
				{
					printf("-datadir %s could not be found\n", q3e_argv[i + 1]);
					return (void *)(intptr_t)1;
				}
			}
			else
			{
				printf("-datadir needs an argument\n");
				return (void *)(intptr_t)1;
			}
		}

		// Inject a custom config dir.
		if (strcmp(q3e_argv[i], "-cfgdir") == 0)
		{
			// We need an argument.
			if (i != (q3e_argc - 1))
			{
				Q_strlcpy(cfgdir, q3e_argv[i + 1], sizeof(cfgdir));
			}
			else
			{
				printf("-cfgdir needs an argument\n");
				return (void *)(intptr_t)1;
			}

		}
	}

	// Enforce the real UID to prevent setuid crap
	if (getuid() != geteuid())
	{
		printf("The effective UID is not the real UID! Your binary is probably marked\n");
		printf("'setuid'. That is not good idea, please fix it :) If you really know\n");
		printf("what you're doing edit src/unix/main.c and remove this check. Don't\n");
		printf("complain if Quake II eats your dog afterwards!\n");

		return (void *)(intptr_t)1;
	}

	// enforce C locale
	setenv("LC_ALL", "C", 1);

	/// Do not delay reads on stdin
	fcntl(fileno(stdin), F_SETFL, fcntl(fileno(stdin), F_GETFL, NULL) | FNDELAY);

	// Initialize the game.
	// Never returns.
	Qcommon_Init(q3e_argc, q3e_argv);

	Q3E_FreeArgs();

	Qcommon_Mainloop();

	Q3E_End();
	main_thread = 0;
	IsInitialized = false;
	Com_Printf("[Harmattan]: Leave quake2 main thread.\n");
	return 0;
}

// start game main thread from Android Surface thread
static void Q3E_StartGameMainThread(void)
{
	if(main_thread)
		return;

	pthread_attr_t attr;
	pthread_attr_init(&attr);

	if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE) != 0) {
		Com_Printf("[Harmattan]: ERROR: pthread_attr_setdetachstate quake2 main thread failed\n");
		exit(1);
	}

	if (pthread_create((pthread_t *)&main_thread, &attr, quake2_main, NULL) != 0) {
		Com_Printf("[Harmattan]: ERROR: pthread_create quake2 main thread failed\n");
		exit(1);
	}

	pthread_attr_destroy(&attr);

	q3e_running = true;
	Com_Printf("[Harmattan]: quake2 main thread start.\n");
}

// shutdown game main thread
static void Q3E_ShutdownGameMainThread(void)
{
	if(!main_thread)
		return;

	q3e_running = false;
	if (pthread_join(main_thread, NULL) != 0) {
		Com_Printf("[Harmattan]: ERROR: pthread_join quake2 main thread failed\n");
	}
	main_thread = 0;
	Com_Printf("[Harmattan]: quake2 main thread quit.\n");
}

void ShutdownGame(void)
{
	if(IsInitialized)
	{
		Sys_TriggerEvent(TRIGGER_EVENT_WINDOW_CREATED); // if quake2 main thread is waiting new window
		Q3E_ShutdownGameMainThread();
		//common->Quit();
	}
}

static void game_exit(void)
{
	Com_Printf("[Harmattan]: quake2 exit.\n");

	Q3E_CloseRedirectOutput();
}

void Sys_SyncState(void)
{
	if (setState)
	{
		static int prev_state = -1;
		static int state = -1;
		state = (cls.key_dest == key_game) << 1;

		if (state != prev_state)
		{
			(*setState)(state);
			prev_state = state;
		}
	}
}

int
main(int argc, char **argv)
{
	Q3E_DumpArgs(argc, (const char **)argv);

	Q3E_RedirectOutput();

	Q3E_PrintInitialContext(argc, (const char **)argv);

	Sys_InitThreads();

	Q3E_StartGameMainThread();

	atexit(game_exit);

	IsInitialized = true;

	return 0;
}


