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
 * This file implements all system dependent generic functions.
 *
 * =======================================================================
 */

#include <conio.h>
#include <direct.h>
#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <io.h>
#include <shlobj.h>
#include <stdio.h>
#include <wchar.h>
#include <windows.h>

#include "../../common/header/common.h"
#include "header/resource.h"

// stdin and stdout handles
static HANDLE hinput, houtput;

// Game library handle
static HINSTANCE game_library;

// Config dir
char cfgdir[MAX_OSPATH] = CFGDIR;

// Buffer for the dedicated server console
static char console_text[256];
static size_t console_textlen;

/* ================================================================ */

void
Sys_Error(const char *error, ...)
{
	va_list argptr;
	char text[1024];

#ifndef DEDICATED_ONLY
	CL_Shutdown();
#endif

	Qcommon_Shutdown();

	va_start(argptr, error);
	vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);
	fprintf(stderr, "Error: %s\n", text);
	fprintf(stdout, "Error: %s\n", text);

	MessageBox(NULL, text, "Error", 0 /* MB_OK */);

	/* Close stdout and stderr */
#ifndef DEDICATED_ONLY
	fclose(stdout);
	fclose(stderr);
#endif

	exit(1);
}

void
Sys_Quit(void)
{
	const qboolean free_console = (dedicated && dedicated->value);
	timeEndPeriod(1);

#ifndef DEDICATED_ONLY
	CL_Shutdown();
#endif

	Qcommon_Shutdown();

	if (free_console)
	{
		FreeConsole();
	}

	printf( "------------------------------------\n" );

	/* Close stdout and stderr */
#ifndef DEDICATED_ONLY
	fclose(stdout);
	fclose(stderr);
#endif

	exit(0);
}

void
Sys_Init(void)
{
	OSVERSIONINFO vinfo;

	timeBeginPeriod(1);
	vinfo.dwOSVersionInfoSize = sizeof(vinfo);

	if (!GetVersionEx(&vinfo))
	{
		Sys_Error("Couldn't get OS info");
	}

	/* While Quake II should run on older versions,
	   limit Yamagi Quake II to Windows XP and
	   above. Testing older version would be a
	   PITA. */
	if (!((vinfo.dwMajorVersion > 5) ||
		((vinfo.dwMajorVersion == 5) &&
			(vinfo.dwMinorVersion >= 1))))
	{
		Sys_Error("Yamagi Quake II needs Windows XP or higher!\n");
	}


	if (dedicated->value)
	{
		AllocConsole();

		hinput = GetStdHandle(STD_INPUT_HANDLE);
		houtput = GetStdHandle(STD_OUTPUT_HANDLE);
	}
}

/* ================================================================ */

char *
Sys_ConsoleInput(void)
{
	INPUT_RECORD recs[1024];
	int ch;
   	DWORD dummy, numread, numevents;

	if (!dedicated || !dedicated->value)
	{
		return NULL;
	}

	for ( ; ; )
	{
		if (!GetNumberOfConsoleInputEvents(hinput, &numevents))
		{
			Sys_Error("Error getting # of console events");
		}

		if (numevents <= 0)
		{
			break;
		}

		if (!ReadConsoleInput(hinput, recs, 1, &numread))
		{
			Sys_Error("Error reading console input");
		}

		if (numread != 1)
		{
			Sys_Error("Couldn't read console input");
		}

		if (recs[0].EventType == KEY_EVENT)
		{
			if (!recs[0].Event.KeyEvent.bKeyDown)
			{
				ch = recs[0].Event.KeyEvent.uChar.AsciiChar;

				switch (ch)
				{
					case '\r':
						WriteFile(houtput, "\r\n", 2, &dummy, NULL);

						if (console_textlen)
						{
							console_text[console_textlen] = 0;
							console_textlen = 0;
							return console_text;
						}

						break;

					case '\b':

						if (console_textlen)
						{
							console_textlen--;
							WriteFile(houtput, "\b \b", 3, &dummy, NULL);
						}

						break;

					default:

						if (ch >= ' ')
						{
							if (console_textlen < sizeof(console_text) - 2)
							{
								WriteFile(houtput, &ch, 1, &dummy, NULL);
								console_text[console_textlen] = ch;
								console_textlen++;
							}
						}

						break;
				}
			}
		}
	}

	return NULL;
}

void
Sys_ConsoleOutput(char *string)
{
	char text[256];
	DWORD dummy;

	if ((string[0] == 0x01) || (string[0] == 0x02))
	{
		// remove color marker
		string[0] = ' ';
	}

	if (!dedicated || !dedicated->value)
	{
		fputs(string, stdout);
	}
	else
	{
		if (console_textlen)
		{
			text[0] = '\r';
			memset(&text[1], ' ', console_textlen);
			text[console_textlen + 1] = '\r';
			text[console_textlen + 2] = 0;
			WriteFile(houtput, text, console_textlen + 2, &dummy, NULL);
		}

		WriteFile(houtput, string, strlen(string), &dummy, NULL);

		if (console_textlen)
		{
			WriteFile(houtput, console_text, console_textlen, &dummy, NULL);
		}
	}
}

/* ================================================================ */

long long
Sys_Microseconds(void)
{
	static LARGE_INTEGER freq = { 0 };
	static LARGE_INTEGER base = { 0 };

    if (!freq.QuadPart)
	{
		QueryPerformanceFrequency(&freq);
	}

	if (!base.QuadPart)
	{
		QueryPerformanceCounter(&base);
		base.QuadPart -= 1001;
	}

	LARGE_INTEGER cur;
	QueryPerformanceCounter(&cur);

	return (cur.QuadPart - base.QuadPart) * 1000000 / freq.QuadPart;
}

int
Sys_Milliseconds(void)
{
	return (int)(Sys_Microseconds()/1000ll);
}

void
Sys_Nanosleep(int nanosec)
{
	HANDLE timer;
	LARGE_INTEGER li;

	timer = CreateWaitableTimer(NULL, TRUE, NULL);

	// Windows has a max. resolution of 100ns.
	li.QuadPart = -nanosec / 100;

	SetWaitableTimer(timer, &li, 0, NULL, NULL, FALSE);
	WaitForSingleObject(timer, INFINITE);

	CloseHandle(timer);
}

/* ================================================================ */

/* The musthave and canhave arguments are unused in YQ2. We
   can't remove them since Sys_FindFirst() and Sys_FindNext()
   are defined in shared.h and may be used in custom game DLLs. */

// File searching
static char findbase[MAX_OSPATH];
static char findpath[MAX_OSPATH];
static HANDLE findhandle;

char *
Sys_FindFirst(const char *path, unsigned musthave, unsigned canthave)
{
	if (findhandle)
	{
		Sys_Error("Sys_BeginFind without close");
	}

	COM_FilePath(path, findbase);

	WCHAR wpath[MAX_OSPATH] = {0};
	MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, MAX_OSPATH);

	WIN32_FIND_DATAW findinfo;
	findhandle = FindFirstFileW(wpath, &findinfo);

	if (findhandle == INVALID_HANDLE_VALUE)
	{
		return NULL;
	}

	CHAR cFileName[MAX_OSPATH];
	WideCharToMultiByte(CP_UTF8, 0, findinfo.cFileName, -1, cFileName, MAX_OSPATH, NULL, NULL);

	Com_sprintf(findpath, sizeof(findpath), "%s/%s", findbase, cFileName);
	return findpath;
}

char *
Sys_FindNext(unsigned musthave, unsigned canthave)
{
	WIN32_FIND_DATAW findinfo;

	if (findhandle == INVALID_HANDLE_VALUE)
	{
		return NULL;
	}

	if (!FindNextFileW(findhandle, &findinfo))
	{
		return NULL;
	}

	CHAR cFileName[MAX_OSPATH];
	WideCharToMultiByte(CP_UTF8, 0, findinfo.cFileName, -1, cFileName, MAX_OSPATH, NULL, NULL);

	Com_sprintf(findpath, sizeof(findpath), "%s/%s", findbase, cFileName);
	return findpath;
}

void
Sys_FindClose(void)
{
	if (findhandle != INVALID_HANDLE_VALUE)
	{
		FindClose(findhandle);
	}

	findhandle = 0;
}

/* ================================================================ */

void
Sys_UnloadGame(void)
{
	if (!FreeLibrary(game_library))
	{
		Com_Error(ERR_FATAL, "FreeLibrary failed for game library");
	}

	game_library = NULL;
}

void *
Sys_GetGameAPI(void *parms)
{
	void *(*GetGameAPI)(void *);
	char name[MAX_OSPATH];
	WCHAR wname[MAX_OSPATH];
	char *path = NULL;

	if (game_library)
	{
		Com_Error(ERR_FATAL, "Sys_GetGameAPI without Sys_UnloadingGame");
	}

	/* now run through the search paths */
	path = NULL;

	while (1)
	{
		path = FS_NextPath(path);

		if (!path)
		{
			return NULL; /* couldn't find one anywhere */
		}

		/* Try game.dll */
		Com_sprintf(name, sizeof(name), "%s/%s", path, "game.dll");
		MultiByteToWideChar(CP_UTF8, 0, name, -1, wname, MAX_OSPATH);
		game_library = LoadLibraryW(wname);

		if (game_library)
		{
			Com_DPrintf("Loading library: %s\n", name);
			break;
		}

		/* Try gamex86.dll as fallback */
 		Com_sprintf(name, sizeof(name), "%s/%s", path, "gamex86.dll");
		MultiByteToWideChar(CP_UTF8, 0, name, -1, wname, MAX_OSPATH);
		game_library = LoadLibraryW(wname);

		if (game_library)
		{
			Com_DPrintf("Loading library: %s\n", name);
			break;
		}
	}

	GetGameAPI = (void *)GetProcAddress(game_library, "GetGameAPI");

	if (!GetGameAPI)
	{
		Sys_UnloadGame();
		return NULL;
	}

	return GetGameAPI(parms);
}

/* ======================================================================= */

void
Sys_Mkdir(const char *path)
{
	if (!Sys_IsDir(path))
	{
		WCHAR wpath[MAX_OSPATH] = {0};
		MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, MAX_OSPATH);

		if (CreateDirectoryW(wpath, NULL) == 0)
		{
			Com_Error(ERR_FATAL, "Couldn't create dir %s\n", path);
		}
	}
}

qboolean
Sys_IsDir(const char *path)
{
	WCHAR wpath[MAX_OSPATH] = {0};
	MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, MAX_OSPATH);

	DWORD fileAttributes = GetFileAttributesW(wpath);
	if (fileAttributes == INVALID_FILE_ATTRIBUTES)
	{
		return false;
	}

	return (fileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

qboolean
Sys_IsFile(const char *path)
{
	WCHAR wpath[MAX_OSPATH] = {0};
	MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, MAX_OSPATH);

	DWORD fileAttributes = GetFileAttributesW(wpath);
	if (fileAttributes == INVALID_FILE_ATTRIBUTES)
	{
		return false;
	}

	// I guess the assumption that if it's not a file or device
	// then it's a directory is good enough for us?
	return (fileAttributes & (FILE_ATTRIBUTE_DIRECTORY|FILE_ATTRIBUTE_DEVICE)) == 0;
}

char *
Sys_GetHomeDir(void)
{
	char *cur;
	char *old;
	char profile[MAX_PATH];
	static char gdir[MAX_OSPATH];
	WCHAR uprofile[MAX_PATH];

	/* Get the path to "My Documents" directory */
	SHGetFolderPathW(NULL, CSIDL_PERSONAL, NULL, 0, uprofile);

	if (WideCharToMultiByte(CP_UTF8, 0, uprofile, -1, profile, sizeof(profile), NULL, NULL) == 0)
	{
		return NULL;
	}

	/* Replace backslashes by slashes */
	cur = old = profile;

	if (strstr(cur, "\\") != NULL)
	{
		while (cur != NULL)
		{
			if ((cur - old) > 1)
			{
				*cur = '/';

			}

			old = cur;
			cur = strchr(old + 1, '\\');
		}
	}

	snprintf(gdir, sizeof(gdir), "%s/%s/", profile, cfgdir);
	Sys_Mkdir(gdir);

	return gdir;
}

void
Sys_Remove(const char *path)
{
	WCHAR wpath[MAX_OSPATH] = {0};
	MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, MAX_OSPATH);

	_wremove(wpath);
}

int
Sys_Rename(const char *from, const char *to)
{
	WCHAR wfrom[MAX_OSPATH] = {0};
	MultiByteToWideChar(CP_UTF8, 0, from, -1, wfrom, MAX_OSPATH);

	WCHAR wto[MAX_OSPATH] = {0};
	MultiByteToWideChar(CP_UTF8, 0, to, -1, wto, MAX_OSPATH);

	return _wrename(wfrom, wto);
}

void
Sys_RemoveDir(const char *path)
{
	WCHAR wpath[MAX_OSPATH] = {0};
	WCHAR wpathwithwildcard[MAX_OSPATH] = {0};
	WCHAR wpathwithfilename[MAX_OSPATH] = {0};
	WIN32_FIND_DATAW fd;

	if (MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, MAX_OSPATH) >= MAX_QPATH)
	{
		/* This is hopefully never reached, because in a good
		   world path has a maximum length of MAX_QPATH. Since
		   we can't use wcscat_s() below because it would break
		   compat with Win XP, nevertheless do this check. */
		return;
	}

	wcsncpy(wpathwithwildcard, wpath, MAX_OSPATH);
	wcscat(wpathwithwildcard, L"\\*.*");

	HANDLE hFind = FindFirstFileW(wpathwithwildcard, &fd);
	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (wcslen(wpath) + wcslen(fd.cFileName) >= MAX_QPATH)
			{
				// Same as above.
				continue;
			}

			wmemset(wpathwithfilename, 0, MAX_OSPATH);
			wcscat(wpathwithfilename, wpath);
			wcscat(wpathwithfilename, fd.cFileName);

			DeleteFileW(wpathwithfilename);
		}
		while (FindNextFileW(hFind, &fd));
		FindClose(hFind);
	}

	RemoveDirectoryW(wpath);
}

qboolean
Sys_Realpath(const char *in, char *out, size_t size)
{
	WCHAR win[MAX_OSPATH] = {0};
	WCHAR wconverted[MAX_OSPATH] = {0};

	MultiByteToWideChar(CP_UTF8, 0, in, -1, win, sizeof(win)/sizeof(win[0]));

	if (_wfullpath(wconverted, win, size) == NULL)
	{
		Com_Printf("Couldn't get realpath for %s\n", in);
		return false;
	}

	WideCharToMultiByte(CP_UTF8, 0, wconverted, -1, out, size, NULL, NULL);
	return true;
}

/* ======================================================================= */

void *
Sys_GetProcAddress(void *handle, const char *sym)
{
	return GetProcAddress(handle, sym);
}

void
Sys_FreeLibrary(void *handle)
{
	if (!handle)
	{
		return;
	}

	if (!FreeLibrary(handle))
	{
		Com_Error(ERR_FATAL, "FreeLibrary failed on %p", handle);
	}
}

void *
Sys_LoadLibrary(const char *path, const char *sym, void **handle)
{
	HMODULE module;
	void *entry;
	WCHAR wpath[MAX_OSPATH];

	*handle = NULL;

	MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, MAX_OSPATH);
	module = LoadLibraryW(wpath);

	if (!module)
	{
		Com_Printf("%s failed: LoadLibrary returned %lu on %s\n",
		           __func__, GetLastError(), path);
		return NULL;
	}

	if (sym)
	{
		entry = GetProcAddress(module, sym);

		if (!entry)
		{
			Com_Printf("%s failed: GetProcAddress returned %lu on %s\n",
			           __func__, GetLastError(), path);
			FreeLibrary(module);
			return NULL;
		}
	}
	else
	{
		entry = NULL;
	}

	*handle = module;

	Com_DPrintf("%s succeeded: %s\n", __func__, path);

	return entry;
}

/* ======================================================================= */

void
Sys_GetWorkDir(char *buffer, size_t len)
{
	WCHAR wbuffer[MAX_OSPATH];

	if (GetCurrentDirectoryW(sizeof(wbuffer)/sizeof(wbuffer[0]), wbuffer) != 0)
	{
		WideCharToMultiByte(CP_UTF8, 0, wbuffer, -1, buffer, len, NULL, NULL);
		return;
	}

	buffer[0] = '\0';
}

qboolean
Sys_SetWorkDir(char *path)
{
	WCHAR wpath[MAX_OSPATH];

	MultiByteToWideChar(CP_UTF8, 0, path, -1, wpath, sizeof(wpath)/sizeof(wpath[0]));

	if (SetCurrentDirectoryW(wpath) != 0)
	{
		return true;
	}

	return false;
}

/* ======================================================================= */

// This one is Windows specific.

void
Sys_RedirectStdout(void)
{
	char dir[MAX_OSPATH];
	char path_stdout[MAX_OSPATH];
	char path_stderr[MAX_OSPATH];
	WCHAR wpath_stdout[MAX_OSPATH];
	WCHAR wpath_stderr[MAX_OSPATH];
	const char *tmp;

	if (is_portable) {
		tmp = Sys_GetBinaryDir();
		Q_strlcpy(dir, tmp, sizeof(dir));
	} else {
		tmp = Sys_GetHomeDir();
		Q_strlcpy(dir, tmp, sizeof(dir));
	}

	if (dir[0] == '\0')
	{
		return;
	}

	snprintf(path_stdout, sizeof(path_stdout), "%s/%s", dir, "stdout.txt");
	snprintf(path_stderr, sizeof(path_stderr), "%s/%s", dir, "stderr.txt");

	MultiByteToWideChar(CP_UTF8, 0, path_stdout, -1, wpath_stdout, sizeof(wpath_stdout)/sizeof(wpath_stdout[0]));
	MultiByteToWideChar(CP_UTF8, 0, path_stderr, -1, wpath_stderr, sizeof(wpath_stderr)/sizeof( wpath_stderr[0] ) );

	_wfreopen(wpath_stdout, L"w", stdout);
	_wfreopen(wpath_stderr, L"w", stderr);

	setvbuf(stdout, (char *)NULL, _IOLBF, BUFSIZ);
	setvbuf(stderr, (char *)NULL, _IOLBF, BUFSIZ);
}

/* ======================================================================= */

// This one is windows specific.

typedef enum YQ2_PROCESS_DPI_AWARENESS {
	YQ2_PROCESS_DPI_UNAWARE = 0,
	YQ2_PROCESS_SYSTEM_DPI_AWARE = 1,
	YQ2_PROCESS_PER_MONITOR_DPI_AWARE = 2
} YQ2_PROCESS_DPI_AWARENESS;

void
Sys_SetHighDPIMode(void)
{
	/* For Vista, Win7 and Win8 */
	BOOL(WINAPI *SetProcessDPIAware)(void) = NULL;

	/* Win8.1 and later */
	HRESULT(WINAPI *SetProcessDpiAwareness)(YQ2_PROCESS_DPI_AWARENESS dpiAwareness) = NULL;


	HINSTANCE userDLL = LoadLibrary("USER32.DLL");

	if (userDLL)
	{
		SetProcessDPIAware = (BOOL(WINAPI *)(void)) GetProcAddress(userDLL,
				"SetProcessDPIAware");
	}


	HINSTANCE shcoreDLL = LoadLibrary("SHCORE.DLL");

	if (shcoreDLL)
	{
		SetProcessDpiAwareness = (HRESULT(WINAPI *)(YQ2_PROCESS_DPI_AWARENESS))
			GetProcAddress(shcoreDLL, "SetProcessDpiAwareness");
	}


	if (SetProcessDpiAwareness) {
		SetProcessDpiAwareness(YQ2_PROCESS_PER_MONITOR_DPI_AWARE);
	}
	else if (SetProcessDPIAware) {
		SetProcessDPIAware();
	}
}
