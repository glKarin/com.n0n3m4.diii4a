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
 * This file is the starting point of the program and implements
 * the main loop
 *
 * =======================================================================
 */

#include <fcntl.h>
#include <locale.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../../common/header/common.h"
#include "header/unix.h"

#if defined(__APPLE__) && !defined(DEDICATED_ONLY)
#include <SDL/SDL.h>
#endif

qboolean IsInitialized = false;

extern void Android_PollInput(void);
extern void Q3E_CheckNativeWindowChanged(void);
int main_time, main_oldtime, main_newtime;
void main_frame()
{
	do
	{
		main_newtime = Sys_Milliseconds();
		main_time = main_newtime - main_oldtime;
	}
	while (main_time < 1);
	Android_PollInput();
	Q3E_CheckNativeWindowChanged();
	Qcommon_Frame(main_time);
	main_oldtime = main_newtime;
}

/* Android */

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

	//Q3E_Start();

	Com_Printf("[Harmattan]: Enter quake2 main thread -> %s\n", "main");

	/* register signal handler */
	registerHandler();

	/* enforce C locale */
	setenv("LC_ALL", "C", 1);

	printf("\nYamagi Quake II v%s\n", VERSION);
	printf("=====================\n\n");

#ifndef DEDICATED_ONLY
	printf("Client build options:\n");
#ifdef CDA
	printf(" + CD audio\n");
#else
	printf(" - CD audio\n");
#endif
#ifdef OGG
	printf(" + OGG/Vorbis\n");
#else
	printf(" - OGG/Vorbis\n");
#endif
#ifdef USE_OPENAL
	printf(" + OpenAL audio\n");
#else
	printf(" - OpenAL audio\n");
#endif
#ifdef ZIP
	printf(" + Zip file support\n");
#else
	printf(" - Zip file support\n");
#endif
#endif

	printf("Platform: %s\n", BUILDSTRING);
	printf("Architecture: %s\n", CPUSTRING);

	/* Seed PRNG */
	randk_seed();

	/* Initialze the game */
	Qcommon_Init(q3e_argc, q3e_argv);

	Q3E_FreeArgs();

	main_oldtime = Sys_Milliseconds();

	while (1) {
		if(!q3e_running) // exit
			break;
		main_frame();
	}

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

