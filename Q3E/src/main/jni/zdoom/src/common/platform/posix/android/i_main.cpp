/*
** i_main.cpp
** System-specific startup code. Eventually calls D_DoomMain.
**
**---------------------------------------------------------------------------
** Copyright 1998-2007 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

// HEADER FILES ------------------------------------------------------------

#include <csignal>
#include <locale.h>
#include <new>
#include <signal.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <unistd.h>

#include "c_console.h"
#include "cmdlib.h"
#include "engineerrors.h"
#include "i_interface.h"
#include "i_system.h"
#include "m_argv.h"
#include "printf.h"
#include "version.h"

// MACROS ------------------------------------------------------------------

// TYPES -------------------------------------------------------------------

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

extern "C" int cc_install_handlers(int, char**, int, int*, const char*, int(*)(char*, char*));

#include "sys_android.inc"

extern void Sys_SetArgs(int argc, const char** argv);

bool GLimp_CheckGLInitialized(void)
{
	return Q3E_CheckNativeWindowChanged();
}

#ifdef __linux__
void Linux_I_FatalError(const char* errortext);
#endif

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------
int GameMain();

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

// PUBLIC DATA DEFINITIONS -------------------------------------------------
FString sys_ostype;

// The command line arguments.
FArgs *Args;

// PRIVATE DATA DEFINITIONS ------------------------------------------------


// CODE --------------------------------------------------------------------



static int GetCrashInfo (char *buffer, char *end)
{
	if (sysCallbacks.CrashInfo) sysCallbacks.CrashInfo(buffer, end - buffer, "\n");
	return strlen(buffer);
}

void I_DetectOS()
{
	FString operatingSystem;

	const char *paths[] = {"/etc/os-release", "/usr/lib/os-release"};

	for (const char *path : paths)
	{
		struct stat dummy;

		if (stat(path, &dummy) != 0)
			continue;

		char cmdline[256];
		snprintf(cmdline, sizeof cmdline, ". %s && echo ${PRETTY_NAME}", path);

		FILE *proc = popen(cmdline, "r");

		if (proc == nullptr)
			continue;

		char distribution[256] = {};
		fread(distribution, sizeof distribution - 1, 1, proc);

		const size_t length = strlen(distribution);

		if (length > 1)
		{
			distribution[length - 1] = '\0';
			operatingSystem = distribution;
		}

		pclose(proc);
		break;
	}

	utsname unameInfo;

	if (uname(&unameInfo) == 0)
	{
		const char* const separator = operatingSystem.Len() > 0 ? ", " : "";
		operatingSystem.AppendFormat("%s%s %s on %s", separator, unameInfo.sysname, unameInfo.release, unameInfo.machine);
		sys_ostype.Format("%s %s on %s", unameInfo.sysname, unameInfo.release, unameInfo.machine);
	}

	if (operatingSystem.Len() > 0)
		Printf("OS: %s\n", operatingSystem.GetChars());
}

void I_StartupJoysticks();
//extern void ZMusic_SetDLLPath(const char *path);

// ZDOOM game main thread loop
int main(int argc, char **argv)
{
	//ZMusic_SetDLLPath(Sys_DLLDefaultPath());

	extern bool no_handle_signals;
	if(!no_handle_signals)
	{
		int s[4] = { SIGSEGV, SIGILL, SIGFPE, SIGBUS };
		cc_install_handlers(argc, argv, 4, s, GAMENAMELOWERCASE "-crash.log", GetCrashInfo);
	}

	printf(GAMENAME" %s - %s - SDL version\nCompiled on %s\n",
		GetVersionString(), GetGitTime(), __DATE__);

	seteuid (getuid ());
	// Set LC_NUMERIC environment variable in case some library decides to
	// clear the setlocale call at least this will be correct.
	// Note that the LANG environment variable is overridden by LC_*
	setenv ("LC_NUMERIC", "C", 1);

	setlocale (LC_ALL, "C");

	printf("\n");

	Args = new FArgs(argc, argv);

#define PROGDIR Sys_GameDataDefaultPath()
#ifdef PROGDIR
	progdir = PROGDIR;
#else
	char program[PATH_MAX];
	if (realpath (argv[0], program) == NULL)
		strcpy (program, argv[0]);
	char *slash = strrchr (program, '/');
	if (slash != NULL)
	{
		*(slash + 1) = '\0';
		progdir = program;
	}
	else
	{
		progdir = "./";
	}
#endif

	I_StartupJoysticks();

	const int result = GameMain();

	return result;
}

void ShutdownGame(void)
{
    if(q3e_running && gzdoomIsInitialized)
    {
		extern void D_DoomShutdown(void);
		D_DoomShutdown();
		NOTIFY_EXIT;
		q3e_running = false;
    }
}
