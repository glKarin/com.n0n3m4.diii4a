/*
Copyright (C) 2006-2007 Mark Olsen

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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/random.h>

#include <dlfcn.h>

#include "quakedef.h"

#warning Find a better stack size

int __stack = 4*1024*1024;

struct Library *DynLoadBase;

extern struct Library *VorbisFileBase;

#ifndef CLIENTONLY
qboolean isDedicated;
#endif

void Sys_RecentServer(char *command, char *target, char *title, char *desc)
{
}

void Sys_Shutdown()
{
	if(DynLoadBase)
	{
		CloseLibrary(DynLoadBase);
		DynLoadBase = 0;
	}

	if (VorbisFileBase)
	{
		CloseLibrary(VorbisFileBase);
		VorbisFileBase = 0;
	}
}

void Sys_Quit (void)
{
	Host_Shutdown();

	Sys_Shutdown();

	exit(0);
}

static void ftevprintf(const char *fmt, va_list arg)
{
	char buf[4096];
	unsigned char *p;

	vsnprintf(buf, sizeof(buf), fmt, arg);

	for (p = (unsigned char *)buf; *p; p++)
		if ((*p > 128 || *p < 32) && *p != 10 && *p != 13 && *p != 9)
			printf("[%02x]", *p);
		else
			putc(*p, stdout);
}

void Sys_Printf(char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	ftevprintf(fmt, arg);
	va_end(arg);
}

void Sys_Error(const char *error, ...)
{
	va_list arg;

	printf("Error: ");
	va_start(arg, error);
	ftevprintf(error, arg);
	va_end(arg);

	Host_Shutdown ();
	exit (1);
}

void Sys_Warn(char *warning, ...)
{
	va_list arg;

	printf("Warning: ");
	va_start(arg, warning);
	ftevprintf(warning, arg);
	va_end(arg);
}

int Sys_DebugLog(char *file, char *fmt, ...)
{
	va_list arg;
	char buf[4096];
	BPTR fh;

	fh = Open(file, MODE_READWRITE);
	if (fh)
	{
		Seek(fh, OFFSET_END, 0);

		va_start(arg, fmt);
		vsnprintf(buf, sizeof(buf), fmt, arg);
		va_end(arg);

		Write(fh, buf, strlen(buf));

		Close(fh);

		return 0;
	}

	return 1;
}

int secbase;
unsigned int Sys_Milliseconds(void)
{
	struct timeval tp;
	struct timezone tzp;

	gettimeofday(&tp, &tzp);

	if (!secbase)
	{
		secbase = tp.tv_sec;
		return tp.tv_usec/1000;
	}

	return (tp.tv_sec - secbase)*1000 + tp.tv_usec/1000;
}

double Sys_DoubleTime(void)
{
	struct timeval tp;
	struct timezone tzp;

	gettimeofday(&tp, &tzp);

	if (!secbase)
	{
		secbase = tp.tv_sec;
		return tp.tv_usec/1000000.0;
	}

	return (tp.tv_sec - secbase) + tp.tv_usec/1000000.0;
}

/* FS stuff */

int Sys_FileTime(char *path)
{
	BPTR lock;
	struct FileInfoBlock fib;
	int ret = -1;

	if (path[0] == '.' && path[1] == '/')
		path+= 2;

	lock = Lock(path, ACCESS_READ);
	if (lock)
	{
		if (Examine(lock, &fib))
		{
			ret = ((fib.fib_Date.ds_Days+2922)*1440+fib.fib_Date.ds_Minute)*60+fib.fib_Date.ds_Tick/TICKS_PER_SECOND;
		}

		UnLock(lock);
	}

	return ret;
}

int Sys_EnumerateFiles(const char *gpath, const char *match, int (*func)(const char *, qofs_t, void *), void *parm)
{
	char *pattern;
	char pattrans[256];
	char finddir[256];
	char findpattern[514];
	char filename[256];
	int i, j;
	BPTR lock;
	struct FileInfoBlock fib;
	int ret = false;

	snprintf(finddir, sizeof(finddir), "%s/%s", gpath, match);

	pattern = strrchr(finddir, '/');
	if (pattern)
	{
		finddir[((unsigned int)(pattern-finddir))] = 0;
		pattern++;
	}

	for(i=0,j=0;i<sizeof(pattrans)-1 && *pattern;i++,j++)
	{
		if (pattern[j] == '*')
		{
			if (i < sizeof(pattrans)-2)
			{
				pattrans[i] = '#';
				pattrans[i+1] = '?';
				i++;
			}
			else
				pattrans[i] = 0;
		}
		else
			pattrans[i] = pattern[j];
	}

	pattrans[i] = 0;

	lock = Lock(finddir, ACCESS_READ);
	if (lock)
	{
		if (Examine(lock, &fib))
		{
			if (ParsePatternNoCase(pattrans, findpattern, sizeof(findpattern)) >= 0)
			{
				ret = true;

				while(ExNext(lock, &fib))
				{
					if (MatchPatternNoCase(findpattern, fib.fib_FileName))
					{
						snprintf(filename, sizeof(filename), "%s%s", fib.fib_FileName, fib.fib_DirEntryType>=0?"/":"");
						if (func(filename, fib.fib_Size, parm) == 0)
						{
							ret = false;
							break;
						}
					}
				}
			}
		}

		UnLock(lock);
	}

	return ret;
}

void Sys_mkdir(const char *path)
{
	BPTR lock;

	if (path[0] == '.' && path[1] == '/')
		path+= 2;

	lock = CreateDir(path);
	if (lock)
	{
		UnLock(lock);
	}
}

qboolean Sys_rmdir (const char *path)
{
	return false;
}

qboolean Sys_remove(const char *path)
{
	if (path[0] == '.' && path[1] == '/')
		path+= 2;

	return DeleteFile(path);
}
qboolean Sys_Rename (const char *oldfname, const char *newfname)
{
	return !rename(oldfname, newfname);
}

void Sys_CloseLibrary(dllhandle_t *lib)
{
	dlclose((void*)lib);
}

dllhandle_t *Sys_LoadLibrary(const char *name, dllfunction_t *funcs)
{
	int i;
	dllhandle_t lib;

	lib = dlopen (name, RTLD_LAZY);
	if (!lib)
		return NULL;

	if (funcs)
	{
		for (i = 0; funcs[i].name; i++)
		{
			*funcs[i].funcptr = dlsym(lib, funcs[i].name);
			if (!*funcs[i].funcptr)
				break;
		}
		if (funcs[i].name)
		{
			Sys_CloseLibrary((dllhandle_t*)lib);
			lib = NULL;
		}
	}

	return (dllhandle_t*)lib;
}

void *Sys_GetAddressForName(dllhandle_t *module, const char *exportname)
{
	if (!module)
		return NULL;
	return dlsym(module, exportname);
}

int main(int argc, char **argv)
{
	double oldtime, newtime;
	quakeparms_t parms;
	int i;

	memset(&parms, 0, sizeof(parms));

	COM_InitArgv(argc, argv);
	TL_InitLanguages("");

	parms.basedir = "";
	parms.argc = argc;
	parms.argv = argv;

	DynLoadBase = OpenLibrary("dynload.library", 0);

	Host_Init(&parms);

	oldtime = Sys_DoubleTime ();
	while(!(SetSignal(0, 0)&SIGBREAKF_CTRL_C))
	{
		double sleeptime;

		newtime = Sys_DoubleTime ();
		sleeptime = Host_Frame(newtime - oldtime);
		oldtime = newtime;

		Sys_Sleep(sleeptime);
	}
}

char *Sys_URIScheme_NeedsRegistering(void)
{	//no support, report something that'll disable annoying prompts.
    return NULL;
}
void Sys_Init()
{
}

#define SYS_CLIPBOARD_SIZE  256
static char clipboard_buffer[SYS_CLIPBOARD_SIZE] = {0};
void Sys_Clipboard_PasteText(clipboardtype_t cbt, void (*callback)(void *cb, char *utf8), void *ctx)
{
	callback(ctx, clipboard_buffer);
}
void Sys_SaveClipboard(clipboardtype_t cbt, char *text)
{
 	Q_strncpyz(clipboard_buffer, text, SYS_CLIPBOARD_SIZE);
}

qboolean Sys_InitTerminal()
{
	return false;
}
void Sys_CloseTerminal()
{
}

char *Sys_ConsoleInput()
{
	return 0;
}

void Sys_ServerActivity(void)
{
}

qboolean Sys_GetDesktopParameters(int *width, int *height, int *bpp, int *refreshrate)
{
	return false;
}

qboolean Sys_RandomBytes(qbyte *string, int len)
{
	while(len--)
		*string++ = RandomByte();

	return true;
}

#ifdef MULTITHREAD
/* Everything here is stubbed because I don't know MorphOS */
/* Thread creation calls */
void *Sys_CreateThread(char *name, int (*func)(void *), void *args, int priority, int stacksize) { return NULL; }
void Sys_WaitOnThread(void *thread) {}
/* Mutex calls */
void *Sys_CreateMutex(void) { return NULL; }
qboolean Sys_TryLockMutex(void *mutex) { return false; }
qboolean Sys_LockMutex(void *mutex) { return false; }
qboolean Sys_UnlockMutex(void *mutex) { return false; }
void Sys_DestroyMutex(void *mutex) {}
/* Conditional wait calls */
void *Sys_CreateConditional(void) { return NULL; }
qboolean Sys_LockConditional(void *condv) { return false; }
qboolean Sys_UnlockConditional(void *condv) { return false; }
qboolean Sys_ConditionWait(void *condv) { return false; }
qboolean Sys_ConditionSignal(void *condv) { return false; }
qboolean Sys_ConditionBroadcast(void *condv) { return false; }
void Sys_DestroyConditional(void *condv) {}
#endif

void Sys_Sleep(double seconds) {}
