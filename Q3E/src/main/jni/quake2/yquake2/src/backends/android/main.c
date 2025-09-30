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

extern void Qcommon_Mainloop(void);

#include "../android/sys_android.c"

qboolean GLimp_CheckGLInitialized(void)
{
	return Q3E_CheckNativeWindowChanged();
}

int
main(int argc, char **argv)
{
	// register signal handler
	registerHandler();

	// Setup FPU if necessary
	Sys_SetupFPU();

	// Implement command line options that the rather
	// crappy argument parser can't parse.
	for (int i = 0; i < argc; i++)
	{
		// Are we portable?
		if (strcmp(argv[i], "-portable") == 0)
		{
			is_portable = true;
		}

		// Inject a custom data dir.
		if (strcmp(argv[i], "-datadir") == 0)
		{
			// Mkay, did the user give us an argument?
			if (i != (argc - 1))
			{
				// Check if it exists.
				struct stat sb;

				if (stat(argv[i + 1], &sb) == 0)
				{
					if (!S_ISDIR(sb.st_mode))
					{
						printf("-datadir %s is not a directory\n", argv[i + 1]);
						return (void *)(intptr_t)1;
					}

					if(realpath(argv[i + 1], datadir) == NULL)
					{
						printf("realpath(datadir) failed: %s\n", strerror(errno));
						datadir[0] = '\0';
					}
				}
				else
				{
					printf("-datadir %s could not be found\n", argv[i + 1]);
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
		if (strcmp(argv[i], "-cfgdir") == 0)
		{
			// We need an argument.
			if (i != (argc - 1))
			{
				Q_strlcpy(cfgdir, argv[i + 1], sizeof(cfgdir));
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
	Qcommon_Init(argc, argv);

	Qcommon_Mainloop();

	return 0;
}

void ShutdownGame(void)
{
	if(q3e_running && IsInitialized)
	{
		q3e_running = false;
		NOTIFY_EXIT;
	}
}

void Sys_SyncState(void)
{
	//if (setState)
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

/* ------------------------------------------------------------------ */

void
IN_GetClipboardText(char *out, size_t n)
{
	char *s = Android_GetClipboardData();

	if (!s || *s == '\0')
	{
		*out = '\0';
		return;
	}

	Q_strlcpy(out, s, n - 1);

	free(s);
}

/* Copy string s to the clipboard.
   Returns 0 on success, 1 otherwise.
*/
int
IN_SetClipboardText(const char *s)
{
	Android_SetClipboardData(s);
	return 1;
}
