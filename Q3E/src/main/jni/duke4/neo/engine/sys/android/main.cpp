/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012 Krzysztof Klinikowski <kkszysiu@gmail.com>
Copyright (C) 2012 Havlena Petr <havlenapetr@gmail.com>

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#include "../../../idlib/precompiled.h"
#include "../posix/posix_public.h"
#include "../sys_local.h"
#include "local.h"

#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#ifdef ID_MCHECK
#include <mcheck.h>
#endif

static idStr	basepath;
static idStr	savepath;

extern void Posix_EarlyInit();
extern void Posix_LateInit();
extern bool Posix_AddMousePollEvent(int action, int value);
extern void Posix_QueEvent(sysEventType_t type, int value, int value2, int ptrLength, void *ptr);

/*
===========
Sys_InitScanTable
===========
*/

void * __attribute__((weak)) __dso_handle=0;

const char *workdir()
{
static char wd[256];
getcwd(wd,256);
return wd;
}

void Sys_InitScanTable(void)
{
	common->DPrintf("TODO: Sys_InitScanTable\n");
}

/*
=================
Sys_AsyncThread
=================
*/
void *Sys_AsyncThread(void *p)
{
#if 0
	int now;
	int start, end;
	int ticked, to_ticked;

#if 1 // Android
	xthreadInfo *threadInfo = static_cast<xthreadInfo *>(p);
	assert(threadInfo);
#endif

	start = Sys_Milliseconds();
	ticked = start >> 4;

	while (1) {
		start = Sys_Milliseconds();
		to_ticked = start >> 4;

		while (ticked < to_ticked) {
			common->Async();
			ticked++;
			Sys_TriggerEvent(TRIGGER_EVENT_ONE);
		}

		// sleep
		end = Sys_Milliseconds() - start;
		if (end < 16) {
			usleep(16 - end);
		}

		// thread exit
#ifdef _NO_PTHREAD_CANCEL
		if (threadInfo->threadCancel) {
			break;
		}
#else
		pthread_testcancel();
#endif
	}
#endif
	return NULL;
}

/*
===============
Sys_GetConsoleKey
===============
*/
unsigned char Sys_GetConsoleKey(bool shifted)
{
	return shifted ? '~' : '`';
}

/*
===============
Sys_FPU_EnableExceptions
===============
*/
void Sys_FPU_EnableExceptions(int exceptions)
{
}

/*
================
Sys_GetSystemRam
returns in megabytes
================
*/
int Sys_GetSystemRam(void)
{
	long	count, page_size;
	int		mb;

	count = sysconf(_SC_PHYS_PAGES);

	if (count == -1) {
		common->Printf("GetSystemRam: sysconf _SC_PHYS_PAGES failed\n");
		return 512;
	}

	page_size = sysconf(_SC_PAGE_SIZE);

	if (page_size == -1) {
		common->Printf("GetSystemRam: sysconf _SC_PAGE_SIZE failed\n");
		return 512;
	}

	mb= (int)((double)count * (double)page_size / (1024 * 1024));
	// round to the nearest 16Mb
	mb = (mb + 8) & ~15;
	return mb;
}

/*
================
Sys_FPU_SetDAZ
================
*/
void Sys_FPU_SetDAZ(bool enable)
{
}

/*
================
Sys_FPU_SetFTZ
================
*/
void Sys_FPU_SetFTZ(bool enable)
{
}

/*
===============
mem consistency stuff
===============
*/

#ifdef ID_MCHECK

const char *mcheckstrings[] = {
	"MCHECK_DISABLED",
	"MCHECK_OK",
	"MCHECK_FREE",	// block freed twice
	"MCHECK_HEAD",	// memory before the block was clobbered
	"MCHECK_TAIL"	// memory after the block was clobbered
};

void abrt_func(mcheck_status status)
{
	Sys_Printf("memory consistency failure: %s\n", mcheckstrings[ status + 1 ]);
	Posix_SetExit(EXIT_FAILURE);
	common->Quit();
}

#endif

/* Android */

static void * game_main(int argc, char **argv);

#include "sys_android.inc"

void GLimp_CheckGLInitialized(void)
{
	Q3E_CheckNativeWindowChanged();
}

// DNF game main thread loop
void * game_main(int argc, char **argv)
{
	attach_thread(); // attach current to JNI for call Android code

#ifdef ID_MCHECK
	// must have -lmcheck linkage
	mcheck(abrt_func);
	Sys_Printf("memory consistency checking enabled\n");
#endif
	Q3E_Start();

	Posix_EarlyInit();
	Sys_Printf("[Harmattan]: Enter doom3 main thread -> %lu\n", pthread_self());

	if (argc > 1) {
		common->Init(argc-1, (const char **)&argv[1], NULL);
	} else {
		common->Init(0, NULL, NULL);
	}

	Posix_LateInit();

	Q3E_FreeArgs();

	while (1) {
		if(!q3e_running) // exit
			break;
		common->Frame();
	}

	common->Quit();
	Q3E_End();
	main_thread = 0;
	Sys_Printf("[Harmattan]: Leave " Q3E_GAME_NAME " main thread.\n");
	return 0;
}

void ShutdownGame(void)
{
    if(common->IsInitialized())
    {
        TRIGGER_WINDOW_CREATED; // if doom3 main thread is waiting new window
        Q3E_ShutdownGameMainThread();
        common->Quit();
    }
}

static void doom3_exit(void)
{
	Sys_Printf("[Harmattan]: doom3 exit.\n");

	Q3E_FreeArgs();

	Q3E_CloseRedirectOutput();
}

int main(int argc, char **argv)
{
	Q3E_DumpArgs(argc, argv);

	Q3E_RedirectOutput();

	Q3E_PrintInitialContext(argc, argv);

	Q3E_StartGameMainThread();

	atexit(doom3_exit);

	return 0;
}

void idSysLocal::ShowGameWindow(bool show) {
}

void idSysLocal::ShowSplashScreen(bool show) {
}
