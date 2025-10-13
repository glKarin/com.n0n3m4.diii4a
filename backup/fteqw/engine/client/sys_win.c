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
// sys_win.h

#include "quakedef.h"

#include "winquake.h"
#include "resource.h"
#include "errno.h"
#include "fcntl.h"
#include <limits.h>
#include <conio.h>
#include <io.h>
#include <direct.h>
#include "pr_common.h"
#include "fs.h"

#ifdef _MSC_VER
#define MSVC_SEH
#endif

//#define RESTARTTEST

#ifdef MULTITHREAD
#include <process.h>
#endif

//exports that 3rd-party drivers can see in order to use descrete graphics cards over integrated ones, FOR MORE POWAH!
__declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;	//13.35+

static void Sys_InitClock(void);
static void Sys_ClockType_Changed(cvar_t *var, char *oldval);
static void Sys_ClockPrecision_Changed(cvar_t *var, char *oldval);

#ifdef WINRT	//you're going to need a different sys_ port.
qboolean isDedicated = false;
void VARGS Sys_Error (const char *error, ...){}	//eep
void VARGS Sys_Printf (char *fmt, ...){}		//safe, but not ideal (esp for debugging)
void Sys_SendKeyEvents (void){}					//safe, but not ideal
void Sys_ServerActivity(void){}					//empty is safe
void Sys_RecentServer(char *command, char *target, char *title, char *desc){}	//empty is safe
qboolean Sys_InitTerminal(void){return false;}	//failure will break 'setrenderer sv'
char *Sys_ConsoleInput (void){return NULL;}		//safe to stub
void Sys_CloseTerminal (void){}					//called when switching from dedicated->non-dedicated
dllhandle_t *Sys_LoadLibrary(const char *name, dllfunction_t *funcs){return NULL;}	//can just about get away with it
void *Sys_GetAddressForName(dllhandle_t *module, const char *exportname){return NULL;}
void Sys_CloseLibrary(dllhandle_t *lib){}		//safe, ish
void Sys_Init (void){}							//safe, stub is fine. used to register system-specific cvars/commands.
void Sys_Shutdown(void){}						//safe
qboolean Sys_RandomBytes(qbyte *string, int len){return false;}
qboolean Sys_GetDesktopParameters(int *width, int *height, int *bpp, int *refreshrate){return false;}
void INS_Move(void){}							//safe
void INS_Commands(void){}						//safe
void INS_Init(void){}							//safe. should be xinput2 I guess. nothing else is actually supported. touchscreens don't really count.
void INS_ReInit(void){}							//safe
void INS_Shutdown(void){}						//safe
void INS_UpdateGrabs(int fullscreen, int activeapp){}	//safe
void *RT_GetCoreWindow(int *width, int *height){return NULL;}	//I already wrote the d3d11 code, but it needs a window to attach to. you can override width+height by writing to them
void D3D11_DoResize(int newwidth, int newheight);	//already written, call if resized since getcorewindow

static char *clippy;
void Sys_Clipboard_PasteText(clipboardtype_t cbt, void (*callback)(void *cb, char *utf8), void *ctx)
{
	callback(ctx, clippy);
}
void Sys_SaveClipboard(clipboardtype_t cbt, char *text)
{
	if (cbt != CBT_CLIPBOARD)
		return;	//don't copy on mere selection. windows users won't expect it.
	Z_Free(clippy);
	clippy = Z_StrDup(text);
}

static LARGE_INTEGER timestart, timefreq;
static qboolean timeinited = false;
unsigned int Sys_Milliseconds(void)
{
	LARGE_INTEGER cur, diff;
	if (!timeinited)
	{
		timeinited = true;
		QueryPerformanceFrequency(&timefreq); 
		QueryPerformanceCounter(&timestart);
	}
	QueryPerformanceCounter(&cur);
	diff.QuadPart = cur.QuadPart - timestart.QuadPart;
	diff.QuadPart *= 1000;
	diff.QuadPart /= timefreq.QuadPart;
	return diff.QuadPart;
}
double Sys_DoubleTime (void)
{
	LARGE_INTEGER cur, diff;
	if (!timeinited)
	{
		timeinited = true;
		QueryPerformanceFrequency(&timefreq); 
		QueryPerformanceCounter(&timestart);
	}
	QueryPerformanceCounter(&cur);
	diff.QuadPart = cur.QuadPart - timestart.QuadPart;
	diff.QuadPart *= 1000;
	return (double)diff.QuadPart / (double)timefreq.QuadPart;	//I hope the timefreq doesn't get rounded and cause milliseconds and doubletime to start to drift apart.
}

void Sys_Quit (void)
{
	Host_Shutdown ();
	exit(EXIT_SUCCESS);
}

void Sys_mkdir (const char *path)
{
	wchar_t wide[MAX_OSPATH];
	widen(wide, sizeof(wide), path);
	CreateDirectoryW(wide, NULL);
}
qboolean Sys_rmdir (const char *path)
{
	RemoveDirectoryW(wide)

	if (rmdir (path) == 0)
		return true;
	if (errno == ENOENT)
		return true;
	return false;
}

qboolean Sys_remove (const char *path)
{
	wchar_t wide[MAX_OSPATH];
	widen(wide, sizeof(wide), path);
	if (DeleteFileW(wide))
		return true;	//success
	if (GetLastError() == ERROR_FILE_NOT_FOUND)
		return true;	//succeed when the file already didn't exist
	return false;		//other errors? panic
}

qboolean Sys_Rename (const char *oldfname, const char *newfname)
{
	wchar_t oldwide[MAX_OSPATH];
	wchar_t newwide[MAX_OSPATH];
	widen(oldwide, sizeof(oldwide), oldfname);
	widen(newwide, sizeof(newwide), newfname);
	return MoveFileExW(oldwide, newwide, MOVEFILE_REPLACE_EXISTING|MOVEFILE_COPY_ALLOWED);
}

//enumeratefiles is recursive for */* to work
static int Sys_EnumerateFiles2 (const char *match, int matchstart, int neststart, int (QDECL *func)(const char *fname, qofs_t fsize, time_t mtime, void *parm, searchpathfuncs_t *spath), void *parm, searchpathfuncs_t *spath)
{
	qboolean go;

	HANDLE r;
	WIN32_FIND_DATAW fd;
	int nest = neststart;	//neststart refers to just after a /
	qboolean wild = false;

	while(match[nest] && match[nest] != '/')
	{
		if (match[nest] == '?' || match[nest] == '*')
			wild = true;
		nest++;
	}
	if (match[nest] == '/')
	{
		char submatch[MAX_OSPATH];
		char tmproot[MAX_OSPATH];

		if (!wild)
			return Sys_EnumerateFiles2(match, matchstart, nest+1, func, parm, spath);

		if (nest-neststart+1> MAX_OSPATH)
			return 1;
		memcpy(submatch, match+neststart, nest - neststart);
		submatch[nest - neststart] = 0;
		nest++;

		if (neststart+4 > MAX_OSPATH)
			return 1;
		memcpy(tmproot, match, neststart);
		strcpy(tmproot+neststart, "*.*");

		{
			wchar_t wroot[MAX_OSPATH];
			r = FindFirstFileExW(widen(wroot, sizeof(wroot), tmproot), FindExInfoStandard, &fd, FindExSearchNameMatch, NULL, 0);
		}
		strcpy(tmproot+neststart, "");
		if (r==(HANDLE)-1)
			return 1;
		go = true;
		do
		{
			char utf8[MAX_OSPATH];
			char file[MAX_OSPATH];
			narrowen(utf8, sizeof(utf8), fd.cFileName);
			if (*utf8 == '.');	//don't ever find files with a name starting with '.'
			else if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)	//is a directory
			{
				if (wildcmp(submatch, utf8))
				{
					int newnest;
					if (strlen(tmproot) + strlen(utf8) + strlen(match+nest) + 2 < MAX_OSPATH)
					{
						Q_snprintfz(file, sizeof(file), "%s%s/", tmproot, utf8);
						newnest = strlen(file);
						strcpy(file+newnest, match+nest);
						go = Sys_EnumerateFiles2(file, matchstart, newnest, func, parm, spath);
					}
				}
			}
		} while(FindNextFileW(r, &fd) && go);
		FindClose(r);
	}
	else
	{
		const char *submatch = match + neststart;
		char tmproot[MAX_OSPATH];

		if (neststart+4 > MAX_OSPATH)
			return 1;
		memcpy(tmproot, match, neststart);
		strcpy(tmproot+neststart, "*.*");

		{
			wchar_t wroot[MAX_OSPATH];
			r = FindFirstFileExW(widen(wroot, sizeof(wroot), tmproot), FindExInfoStandard, &fd, FindExSearchNameMatch, NULL, 0);
		}
		strcpy(tmproot+neststart, "");
		if (r==(HANDLE)-1)
			return 1;
		go = true;
		do
		{
			char utf8[MAX_OSPATH];
			char file[MAX_OSPATH];

			narrowen(utf8, sizeof(utf8), fd.cFileName);
			if (*utf8 == '.')
				;	//don't ever find files with a name starting with '.' (includes .. and . directories, and unix hidden files)
			else if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)	//is a directory
			{
				if (wildcmp(submatch, utf8))
				{
					if (strlen(tmproot+matchstart) + strlen(utf8) + 2 < MAX_OSPATH)
					{
						Q_snprintfz(file, sizeof(file), "%s%s/", tmproot+matchstart, utf8);
						go = func(file, qofs_Make(fd.nFileSizeLow, fd.nFileSizeHigh), 0, parm, spath);
					}
				}
			}
			else
			{
				if (wildcmp(submatch, utf8))
				{
					if (strlen(tmproot+matchstart) + strlen(utf8) + 1 < MAX_OSPATH)
					{
						Q_snprintfz(file, sizeof(file), "%s%s", tmproot+matchstart, utf8);
						go = func(file, qofs_Make(fd.nFileSizeLow, fd.nFileSizeHigh), 0, parm, spath);
					}
				}
			}
		} while(FindNextFileW(r, &fd) && go);
		FindClose(r);
	}
	return go;
}
int Sys_EnumerateFiles (const char *gpath, const char *match, int (QDECL *func)(const char *fname, qofs_t fsize, void *parm, searchpathfuncs_t *spath), void *parm, searchpathfuncs_t *spath)
{
	char fullmatch[MAX_OSPATH];
	int start;
	if (strlen(gpath) + strlen(match) + 2 > MAX_OSPATH)
		return 1;

	strcpy(fullmatch, gpath);
	start = strlen(fullmatch);
	if (start && fullmatch[start-1] != '/')
		fullmatch[start++] = '/';
	fullmatch[start] = 0;
	strcat(fullmatch, match);
	return Sys_EnumerateFiles2(fullmatch, start, start, func, parm, spath);
}

#else

#if defined(GLQUAKE)
#define PRINTGLARRAYS
#endif

#if defined(_DEBUG) || defined(DEBUG)
#if !defined(_MSC_VER) || _MSC_VER > 1200
#define CATCHCRASH
#endif
#endif

#if !defined(CLIENTONLY) && !defined(SERVERONLY)
qboolean isDedicated = false;
#endif
extern int isPlugin;
qboolean debugout;
float gammapending;	//to cope with ATI. When it times out, v_gamma is reforced in order to correct/update gamma now the drivers think that they have won.

HWND sys_parentwindow;
unsigned int sys_parentleft;	//valid if sys_parentwindow is set
unsigned int sys_parenttop;
unsigned int sys_parentwidth;	//valid if sys_parentwindow is set
unsigned int sys_parentheight;

static struct
{
	int width;
	int height;
	int rate;
	int bpp;
} desktopsettings;
static void Sys_QueryDesktopParameters(void);

//used to do special things with awkward windows versions.
int qwinvermaj;
int qwinvermin;

char		*sys_argv[MAX_NUM_ARGVS];


#ifdef RESTARTTEST
jmp_buf restart_jmpbuf;
#endif
/*
================
Sys_RandomBytes
================
*/
#include <wincrypt.h>
qboolean Sys_RandomBytes(qbyte *string, int len)
{
	HCRYPTPROV  prov;

	if(!CryptAcquireContext( &prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
	{
		return false;
	}

	if(!CryptGenRandom(prov, len, (BYTE *)string))
	{
		CryptReleaseContext( prov, 0);
		return false;
	}
	CryptReleaseContext(prov, 0);
	return true;
}
#ifndef CALG_SHA_512
#define ALG_SID_SHA_256                 12
#define ALG_SID_SHA_384                 13
#define ALG_SID_SHA_512                 14
#define CALG_SHA_256            (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_SHA_256)
#define CALG_SHA_384            (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_SHA_384)
#define CALG_SHA_512            (ALG_CLASS_HASH | ALG_TYPE_ANY | ALG_SID_SHA_512)
#endif

//returns 0 on failure, otherwise returns the actual digest size and the digest (overallocate if lazy)
size_t HashCalculate(const char *hashtype, const void *data, size_t data_size, void *digest_out, size_t digest_size)
{
	HCRYPTPROV	prov;
	HCRYPTHASH	hash;
	ALG_ID		alg;

	if (!Q_strcasecmp(hashtype, "MD4"))
		alg = CALG_MD4;
	else if (!Q_strcasecmp(hashtype, "MD5"))
		alg = CALG_MD5;
	else if (!Q_strcasecmp(hashtype, "SHA1"))
		alg = CALG_SHA1;
#ifdef CALG_SHA_256
	else if (!Q_strcasecmp(hashtype, "SHA256"))
		alg = CALG_SHA_256;	//only on xp sp3+
#endif
#ifdef CALG_SHA_384
	else if (!Q_strcasecmp(hashtype, "SHA384"))
		alg = CALG_SHA_384;	//only on xp sp3+
#endif
#ifdef CALG_SHA_512
	else if (!Q_strcasecmp(hashtype, "SHA512"))
		alg = CALG_SHA_512;	//only on xp sp3+
#endif
	else
		return 0;

	memset(digest_out, 0, digest_size);

	if(CryptAcquireContext(&prov, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
	{
		if (CryptCreateHash(prov, alg, 0, 0, &hash))
		{
			if (CryptHashData(hash, data, (DWORD)data_size, 0))
			{
				DWORD grr = digest_size;
				if (CryptGetHashParam(hash, HP_HASHVAL, digest_out, &grr, 0))
				{
					CryptDestroyHash(hash);
					CryptReleaseContext(prov, 0);
					return grr;
				}
			}
			CryptDestroyHash(hash);
		}
		CryptReleaseContext(prov, 0);
	}
	return 0;
}

/*
=================
Library loading
=================
*/
void Sys_CloseLibrary(dllhandle_t *lib)
{
	FreeLibrary((HMODULE)lib);
}
HMODULE LoadLibraryU(const char *name)
{
	HMODULE ret;
	if (WinNT)
	{
		wchar_t wide[MAX_OSPATH];
		widen(wide, sizeof(wide), name);

		ret = LoadLibraryW(wide);
	}
	else
	{
		wchar_t wide[MAX_OSPATH];
		char ansi[MAX_OSPATH];
		widen(wide, sizeof(wide), name);
		ansi[WideCharToMultiByte(CP_ACP, 0, wide, wcslen(wide), ansi, sizeof(ansi)-1, NULL, NULL)] = 0;
		ret = LoadLibraryA(ansi);
	}
	return ret;
}
dllhandle_t *Sys_LoadLibrary(const char *name, dllfunction_t *funcs)
{
	int i;
	HMODULE lib;
	DWORD err;

	lib = LoadLibraryU(name);
	if (!lib)
	{
		err = GetLastError();
		switch(err)
		{
		case ERROR_MOD_NOT_FOUND:
			break;
		case ERROR_BAD_EXE_FORMAT:
			Con_Printf("Error ERROR_BAD_EXE_FORMAT loading %s\n", name);
			break;
		case ERROR_PROC_NOT_FOUND:
			Con_Printf("Error ERROR_PROC_NOT_FOUND loading %s\n", name);
			break;
		default:
			Con_Printf("Error %u loading %s\n", (unsigned)err, name);
			break;
		}

		if (!strstr(COM_SkipPath(name), ".dll"))
		{	//.dll implies that it is a system dll, or something that is otherwise windows-specific already.
			char libname[MAX_OSPATH];
#ifdef _WIN64
			Q_snprintfz(libname, sizeof(libname), "%s_64", name);
#elif defined(_WIN32)
			Q_snprintfz(libname, sizeof(libname), "%s_32", name);
#else
#error wut? not win32?
#endif
			lib = LoadLibraryU(libname);
		}
		if (!lib)
			return NULL;
	}

	if (funcs)
	{
		for (i = 0; funcs[i].name; i++)
		{
			*funcs[i].funcptr = GetProcAddress(lib, funcs[i].name);
			if (!*funcs[i].funcptr)
				break;
		}
		if (funcs[i].name)
		{
			Con_DPrintf("Missing export \"%s\" in \"%s\"\n", funcs[i].name, name);
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
	return GetProcAddress((HINSTANCE)module, exportname);
}
#ifdef HLSERVER
char *Sys_GetNameForAddress(dllhandle_t *module, void *address)
{
	//windows doesn't provide a function to do this, so we have to do it ourselves.
	//this isn't the fastest way...
	//halflife needs this function.
	char *base = (char *)module;

	IMAGE_DATA_DIRECTORY *datadir;
	IMAGE_EXPORT_DIRECTORY *block;
	IMAGE_NT_HEADERS *ntheader;
	IMAGE_DOS_HEADER *dosheader = (void*)base;

	int i, j;
	DWORD *funclist;
	DWORD *namelist;
	SHORT *ordilist;

	if (!dosheader || dosheader->e_magic != IMAGE_DOS_SIGNATURE)
		return NULL; //yeah, that wasn't an exe

	ntheader = (void*)(base + dosheader->e_lfanew);
	if (!dosheader->e_lfanew || ntheader->Signature != IMAGE_NT_SIGNATURE)
		return NULL;	//urm, wait, a 16bit dos exe?


	datadir = &ntheader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];

	block = (IMAGE_EXPORT_DIRECTORY *)(base + datadir->VirtualAddress);
	funclist = (DWORD*)(base+block->AddressOfFunctions);
	namelist = (DWORD*)(base+block->AddressOfNames);
	ordilist = (SHORT*)(base+block->AddressOfNameOrdinals);
	for (i = 0; i < block->NumberOfFunctions; i++)
	{
		if (base+funclist[i] == address)
		{
			for (j = 0; j < block->NumberOfNames; j++)
			{
				if (ordilist[j] == i)
				{
					return base+namelist[i];
				}
			}
			//it has no name. huh?
			return NULL;
		}
	}
	return NULL;
}
#endif

int		starttime;
qboolean Minimized;
qboolean	WinNT;	//NT has a) proper unicode support that does not unconditionally result in errors. b) a few different registry paths.

static HANDLE		hinput, houtput;

HANDLE		qwclsemaphore;

static HANDLE	tevent;

int VARGS Sys_DebugLog(char *file, char *fmt, ...)
{
	FILE *fd;
	va_list argptr;
	static char data[1024];

	va_start(argptr, fmt);
	vsnprintf(data, sizeof(data)-1, fmt, argptr);
	va_end(argptr);

#if defined(CRAZYDEBUGGING) && CRAZYDEBUGGING > 1
	{
		static int sock;
		if (!sock)
		{
			struct sockaddr_in sa;
			netadr_t na;
			int _true = true;
			int listip;
			listip = COM_CheckParm("-debugip");
			NET_StringToAdr(listip?com_argv[listip+1]:"127.0.0.1", &na);
			NetadrToSockadr(&na, (struct sockaddr_qstorage*)&sa);
			sa.sin_port = htons(10000);
			sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (-1==connect(sock, (struct sockaddr*)&sa, sizeof(sa)))
				Sys_Error("Couldn't send debug log lines\n");
			setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&_true, sizeof(_true));
		}
		send(sock, data, strlen(data), 0);
	}
#endif
	fd = fopen(file, "ab");
	if (fd)
	{
		fprintf(fd, "%s", data);
		fclose(fd);
		return 0;
	}

	return 1;
};

#ifdef CATCHCRASH
#include "dbghelp.h"
typedef BOOL (WINAPI *MINIDUMPWRITEDUMP) (
	HANDLE hProcess,
	DWORD ProcessId,
	HANDLE hFile,
	MINIDUMP_TYPE DumpType,
	PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
	PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
	PMINIDUMP_CALLBACK_INFORMATION CallbackParam
	);
void DumpGLState(void);
void *watchdogthread;
DWORD CrashExceptionHandler (qboolean iswatchdog, DWORD exceptionCode, LPEXCEPTION_POINTERS exceptionInfo)
{
	HANDLE hProc = GetCurrentProcess();
	HMODULE hKernel;
	BOOL (WINAPI *pIsDebuggerPresent)(void);
	DWORD (WINAPI *pSymSetOptions)(DWORD SymOptions);
	BOOL (WINAPI *pSymInitialize)(HANDLE hProcess, PSTR UserSearchPath, BOOL fInvadeProcess);
	BOOL (WINAPI *pSymFromAddr)(HANDLE hProcess, DWORD64 Address, PDWORD64 Displacement, PSYMBOL_INFO Symbol);

#ifdef _WIN64
#define DBGHELP_POSTFIX "64"
	BOOL (WINAPI *pStackWalkX)(DWORD MachineType, HANDLE hProcess, HANDLE hThread, LPSTACKFRAME64 StackFrame, PVOID ContextRecord, PREAD_PROCESS_MEMORY_ROUTINE64 ReadMemoryRoutine, PFUNCTION_TABLE_ACCESS_ROUTINE64 FunctionTableAccessRoutine, PGET_MODULE_BASE_ROUTINE64 GetModuleBaseRoutine, PTRANSLATE_ADDRESS_ROUTINE64 TranslateAddress);
	PVOID (WINAPI *pSymFunctionTableAccessX)(HANDLE hProcess, DWORD64 AddrBase);
	DWORD64 (WINAPI *pSymGetModuleBaseX)(HANDLE hProcess, DWORD64 qwAddr);
	BOOL (WINAPI *pSymGetLineFromAddrX)(HANDLE hProcess, DWORD64 qwAddr, PDWORD pdwDisplacement, PIMAGEHLP_LINE64 Line64);
	BOOL (WINAPI *pSymGetModuleInfoX)(HANDLE hProcess, DWORD64 qwAddr, PIMAGEHLP_MODULE64 ModuleInfo);
	#define STACKFRAMEX STACKFRAME64
	#define IMAGEHLP_LINEX IMAGEHLP_LINE64
	#define IMAGEHLP_MODULEX IMAGEHLP_MODULE64
#else
#define DBGHELP_POSTFIX ""
	BOOL (WINAPI *pStackWalkX)(DWORD MachineType, HANDLE hProcess, HANDLE hThread, LPSTACKFRAME StackFrame, PVOID ContextRecord, PREAD_PROCESS_MEMORY_ROUTINE ReadMemoryRoutine, PFUNCTION_TABLE_ACCESS_ROUTINE FunctionTableAccessRoutine, PGET_MODULE_BASE_ROUTINE GetModuleBaseRoutine, PTRANSLATE_ADDRESS_ROUTINE TranslateAddress);
	PVOID (WINAPI *pSymFunctionTableAccessX)(HANDLE hProcess, DWORD AddrBase);
	DWORD (WINAPI *pSymGetModuleBaseX)(HANDLE hProcess, DWORD dwAddr);
	BOOL (WINAPI *pSymGetLineFromAddrX)(HANDLE hProcess, DWORD dwAddr, PDWORD pdwDisplacement, PIMAGEHLP_LINE Line);
	BOOL (WINAPI *pSymGetModuleInfoX)(HANDLE hProcess, DWORD dwAddr, PIMAGEHLP_MODULE ModuleInfo);
	#define STACKFRAMEX STACKFRAME
	#define IMAGEHLP_LINEX IMAGEHLP_LINE
	#define IMAGEHLP_MODULEX IMAGEHLP_MODULE
#endif
	dllfunction_t debughelpfuncs[] =
	{
		{(void*)&pSymFromAddr,				"SymFromAddr"},
		{(void*)&pSymSetOptions,			"SymSetOptions"},
		{(void*)&pSymInitialize,			"SymInitialize"},
		{(void*)&pStackWalkX,				"StackWalk"DBGHELP_POSTFIX},
		{(void*)&pSymFunctionTableAccessX,	"SymFunctionTableAccess"DBGHELP_POSTFIX},
		{(void*)&pSymGetModuleBaseX,		"SymGetModuleBase"DBGHELP_POSTFIX},
		{(void*)&pSymGetLineFromAddrX,		"SymGetLineFromAddr"DBGHELP_POSTFIX},
		{(void*)&pSymGetModuleInfoX,		"SymGetModuleInfo"DBGHELP_POSTFIX},
		{NULL, NULL}
	};

	switch(exceptionCode)
	{
	case EXCEPTION_ACCESS_VIOLATION:
	case EXCEPTION_DATATYPE_MISALIGNMENT:
	case EXCEPTION_SINGLE_STEP:
	case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
	case EXCEPTION_FLT_DENORMAL_OPERAND:
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
	case EXCEPTION_FLT_INEXACT_RESULT:
	case EXCEPTION_FLT_INVALID_OPERATION:
	case EXCEPTION_FLT_OVERFLOW:
	case EXCEPTION_FLT_STACK_CHECK:
	case EXCEPTION_FLT_UNDERFLOW:
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
	case EXCEPTION_INT_OVERFLOW:
	case EXCEPTION_PRIV_INSTRUCTION:
	case EXCEPTION_IN_PAGE_ERROR:
	case EXCEPTION_ILLEGAL_INSTRUCTION:
	case EXCEPTION_NONCONTINUABLE_EXCEPTION:
	case EXCEPTION_STACK_OVERFLOW:
	case EXCEPTION_INVALID_DISPOSITION:
	case EXCEPTION_GUARD_PAGE:
	case EXCEPTION_INVALID_HANDLE:
//	case EXCEPTION_POSSIBLE_DEADLOCK:
		break;
	case EXCEPTION_BREAKPOINT:
		hKernel = LoadLibrary ("kernel32");
		pIsDebuggerPresent = (void*)GetProcAddress(hKernel, "IsDebuggerPresent");
		if (pIsDebuggerPresent && pIsDebuggerPresent())
			return EXCEPTION_CONTINUE_SEARCH;
		break;
		return EXCEPTION_CONTINUE_EXECUTION;
	default:
		//because windows is a steaming pile of shite, we have to ignore any software-generated exceptions, because most of them are not in fact fatal, *EVEN IF THEY CLAIM TO BE NON-CONTINUABLE*
		return EXCEPTION_CONTINUE_SEARCH;
	}

#ifdef PRINTGLARRAYS
	if (!iswatchdog && qrenderer == QR_OPENGL && Sys_IsMainThread())
		DumpGLState();
#endif

	hKernel = LoadLibrary ("kernel32");
	pIsDebuggerPresent = (void*)GetProcAddress(hKernel, "IsDebuggerPresent");

#ifdef GLQUAKE
	//restores gamma
	GLVID_Crashed();
#endif

	if (!iswatchdog && pIsDebuggerPresent && pIsDebuggerPresent ())
	{
		/*if we have a current window, minimize it to bring us out of fullscreen*/
/*		extern qboolean vid_initializing;
		qboolean oldval = vid_initializing;
		vid_initializing = true;
		ShowWindow(mainwindow, SW_MINIMIZE);
		vid_initializing = oldval;
*/
		return EXCEPTION_CONTINUE_SEARCH;
	}

	/*if we have a current window, kill it, so it can't steal input of handle window messages or anything risky like that*/
	if (iswatchdog)
	{
	}
	else
	{
		DestroyWindow(mainwindow);

		if (Sys_LoadLibrary("DBGHELP", debughelpfuncs))
		{
			STACKFRAMEX stack;
			CONTEXT *pcontext = exceptionInfo->ContextRecord;
			IMAGEHLP_LINEX line;
			IMAGEHLP_MODULEX module;
			struct
			{
				SYMBOL_INFO sym;
				char name[1024];
			} sym;
			int frameno;
			char stacklog[8192];
			int logpos, logstart;
			char *logline;

			stacklog[logpos=0] = 0;

			pSymInitialize(hProc, NULL, TRUE);
			pSymSetOptions(SYMOPT_LOAD_LINES);

			memset(&stack, 0, sizeof(stack));
#ifdef _WIN64
			#define IMAGE_FILE_MACHINE_THIS IMAGE_FILE_MACHINE_AMD64
			stack.AddrPC.Mode = AddrModeFlat;
			stack.AddrPC.Offset = pcontext->Rip;
			stack.AddrFrame.Mode = AddrModeFlat;
			stack.AddrFrame.Offset = pcontext->Rbp;
			stack.AddrStack.Mode = AddrModeFlat;
			stack.AddrStack.Offset = pcontext->Rsp;
#else
			#define IMAGE_FILE_MACHINE_THIS IMAGE_FILE_MACHINE_I386
			stack.AddrPC.Mode = AddrModeFlat;
			stack.AddrPC.Offset = pcontext->Eip;
			stack.AddrFrame.Mode = AddrModeFlat;
			stack.AddrFrame.Offset = pcontext->Ebp;
			stack.AddrStack.Mode = AddrModeFlat;
			stack.AddrStack.Offset = pcontext->Esp;
#endif

			Q_strncpyz(stacklog+logpos, FULLENGINENAME " or dependancy has crashed. The following stack dump been copied to your windows clipboard.\n"
#ifdef _MSC_VER
				"Would you like to generate a core dump too?\n"
#endif
				"\n", sizeof(stacklog)-logpos);
			logstart = logpos += strlen(stacklog+logpos);

			//so I know which one it is
#if defined(DEBUG) || defined(_DEBUG)
	#define BUILDDEBUGREL "Debug"
#else
	#define BUILDDEBUGREL "Optimised"
#endif
#ifdef MINIMAL
	#define BUILDMINIMAL "Min"
#else
	#define BUILDMINIMAL ""
#endif
#if defined(GLQUAKE) && !defined(D3DQUAKE) && !defined(VKQUAKE)
	#define BUILDTYPE "GL"
#elif !defined(GLQUAKE) && defined(D3DQUAKE) && !defined(VKQUAKE)
	#define BUILDTYPE "D3D"
#elif !defined(GLQUAKE) && !defined(D3DQUAKE) && defined(VKQUAKE)
	#define BUILDTYPE "VK"
#else
	#define BUILDTYPE "Merged"
#endif

			Q_snprintfz(stacklog+logpos, sizeof(stacklog)-logpos, "Build: %s %s %s: %s\r\n", BUILDDEBUGREL, PLATFORM, BUILDMINIMAL BUILDTYPE, version_string());
			logpos += strlen(stacklog+logpos);

			for(frameno = 0; ; frameno++)
			{
				DWORD64 symdisp;
				DWORD linedisp;
				DWORD_PTR symaddr;
				if (!pStackWalkX(IMAGE_FILE_MACHINE_THIS, hProc, GetCurrentThread(), &stack, pcontext, NULL, pSymFunctionTableAccessX, pSymGetModuleBaseX, NULL))
					break;
				memset(&module, 0, sizeof(module));
				module.SizeOfStruct = sizeof(module);
				pSymGetModuleInfoX(hProc, stack.AddrPC.Offset, &module);
				memset(&line, 0, sizeof(line));
				line.SizeOfStruct = sizeof(line);
				symdisp = 0;
				memset(&sym, 0, sizeof(sym));
				sym.sym.MaxNameLen = sizeof(sym.name);
				symaddr = stack.AddrPC.Offset;
				sym.sym.SizeOfStruct = sizeof(sym.sym);
				if (pSymFromAddr(hProc, symaddr, &symdisp, &sym.sym))
				{
					if (pSymGetLineFromAddrX(hProc, stack.AddrPC.Offset, &linedisp, &line))
						logline = va("%-20s - %s:%i (%s)\r\n", sym.sym.Name, line.FileName, (int)line.LineNumber, module.LoadedImageName);
					else
						logline = va("%-20s+%#x (%s)\r\n", sym.sym.Name, (unsigned int)symdisp, module.LoadedImageName);
				}
				else
					logline = va("0x%p (%s)\r\n", (void*)(DWORD_PTR)stack.AddrPC.Offset, module.LoadedImageName);
				Q_strncpyz(stacklog+logpos, logline, sizeof(stacklog)-logpos);
				logpos += strlen(stacklog+logpos);
				if (logpos+1 >= sizeof(stacklog))
					break;
			}
			Sys_SaveClipboard(CBT_CLIPBOARD, stacklog+logstart);
#ifdef _MSC_VER
			if (MessageBoxA(0, stacklog, "KABOOM!", MB_ICONSTOP|MB_YESNO) != IDYES)
			{
				if (pIsDebuggerPresent && pIsDebuggerPresent ())
				{
					//its possible someone attached a debugger while we were showing that message
					return EXCEPTION_CONTINUE_SEARCH;
				}
				return EXCEPTION_EXECUTE_HANDLER;
			}
#else
			MessageBox(0, stacklog, "KABOOM!", MB_ICONSTOP);
			return EXCEPTION_EXECUTE_HANDLER;
#endif
		}
		else
		{
			MessageBoxA(NULL, "We crashed.\nUnable to load dbghelp library. Stack info is not available", DISTRIBUTION " Sucks", MB_ICONSTOP);
			return EXCEPTION_EXECUTE_HANDLER;
		}
	}

	//generate a minidump, but only if we were compiled by something that used usable debugging info. its a bit pointless otherwise.
#ifdef _MSC_VER
	{
		char dumpPath[1024];
		char msg[1024];
		DWORD procid = GetCurrentProcessId();
		HANDLE dumpfile;
		HMODULE hDbgHelp;
		MINIDUMPWRITEDUMP fnMiniDumpWriteDump;

		hDbgHelp = LoadLibraryA ("DBGHELP");
		if (hDbgHelp)
			fnMiniDumpWriteDump = (MINIDUMPWRITEDUMP)GetProcAddress (hDbgHelp, "MiniDumpWriteDump");
		else
			fnMiniDumpWriteDump = NULL;

		if (fnMiniDumpWriteDump)
		{
			if (iswatchdog)
			{
				switch (MessageBoxA(NULL, "Fizzle... We hit an infinite loop! Or something is just really slow.\nBlame the monkey in the corner.\nI hope you saved your work.\nWould you like to take a dump now?\n(click cancel to wait a bit longer)", DISTRIBUTION " Sucks", MB_ICONSTOP|MB_YESNOCANCEL|MB_DEFBUTTON3))
				{
				case IDYES:
					break;	//take a dump.
				case IDNO:
					exit(EXIT_FAILURE);
				default:	//cancel = run the exception handler, which means we reset the watchdog.
					return EXCEPTION_EXECUTE_HANDLER;
				}
			}

			/*take a dump*/
			if (*com_homepath)
				Q_strncpyz(dumpPath, com_homepath, sizeof(dumpPath));
			else if (*com_gamepath)
				Q_strncpyz(dumpPath, com_gamepath, sizeof(dumpPath));
			else
				GetTempPathA (sizeof(dumpPath)-16, dumpPath);
			Q_strncatz(dumpPath, DISTRIBUTION"CrashDump.dmp", sizeof(dumpPath));
			dumpfile = CreateFileA (dumpPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (dumpfile)
			{
				MINIDUMP_EXCEPTION_INFORMATION crashinfo;
				crashinfo.ClientPointers = TRUE;
				crashinfo.ExceptionPointers = exceptionInfo;
				crashinfo.ThreadId = GetCurrentThreadId ();
				if (fnMiniDumpWriteDump(hProc, procid, dumpfile, MiniDumpWithIndirectlyReferencedMemory|MiniDumpWithDataSegs, &crashinfo, NULL, NULL))
				{
					CloseHandle(dumpfile);
					Q_snprintfz(msg, sizeof(msg), "You can find the crashdump at:\n%s\nPlease send this file to someone.\n\nWarning: sensitive information (like your current user name) might be present in the dump.\nYou will probably want to compress it.", dumpPath);
					MessageBoxA(NULL, msg, DISTRIBUTION " Sucks", 0);
				}
				else
					MessageBoxA(NULL, "MiniDumpWriteDump failed", "oh noes", 0);
			}
			else
			{
				Q_snprintfz(msg, sizeof(msg), "unable to open %s\nno dump created.", dumpPath);
				MessageBoxA(NULL, msg, "oh noes", 0);
			}
		}
		else
			MessageBoxA(NULL, "Kaboom! Sorry. No MiniDumpWriteDump function.", DISTRIBUTION " Sucks", 0);
	}
#endif
	return EXCEPTION_EXECUTE_HANDLER;
}

//most compilers do not support __try. perhaps we should avoid its use entirely?
LONG CALLBACK nonmsvc_CrashExceptionHandler(PEXCEPTION_POINTERS ExceptionInfo)
{
	DWORD foo = EXCEPTION_CONTINUE_SEARCH;
	foo = CrashExceptionHandler(false, ExceptionInfo->ExceptionRecord->ExceptionCode, ExceptionInfo);
	//we have no handler. thus we handle it by exiting.
	if (foo == EXCEPTION_EXECUTE_HANDLER)
		exit(EXIT_FAILURE);
	return foo;
}

volatile int watchdogframe;	//incremented each frame.
void *watchdogthread;
int watchdogthreadfunction(void *arg)
{
#ifdef _MSC_VER
	int oldframe = watchdogframe;
	int newframe;
	int secs = 0;
	while(1)
	{ 
		newframe = watchdogframe;
		if (oldframe != newframe)
		{
			oldframe = newframe;
			secs = 0;
		}
		else
		{
			secs++;
			if (secs > 10)
			{
				secs = 0;
				__try
				{
					*(int*)arg = -3;
				}
				__except (CrashExceptionHandler(true, GetExceptionCode(), GetExceptionInformation()))
				{
				}
			}
		}
		Sleep(1000);
	}
#endif
	return 0;
}
#endif

#ifndef SERVERONLY

#if (_WIN32_WINNT < 0x0400)
	#define LLKHF_ALTDOWN        0x00000020
	#define LLKHF_UP             0x00000080
	#define WH_KEYBOARD_LL     13
	typedef struct {
		DWORD vkCode;
		DWORD scanCode;
		DWORD flags;
		DWORD time;
		DWORD dwExtraInfo;
	} KBDLLHOOKSTRUCT;
#elif defined(MINGW)
	#ifndef LLKHF_UP
		#define LLKHF_UP             0x00000080
	#endif
#endif

static struct
{
	HHOOK llkeyboardhook;

	//windows hooks can be used for code injection etc.
	//hide these symbols from shitty exports scanners so we don't look like the keylogger that we aren't. Note the 'vid.activeapp' requirement below - we are not a keylogger, we only see a limited set of keys and only when we already have focus.
	HHOOK (WINAPI *pSetWindowsHookEx) (int idHook, HOOKPROC lpfn, HINSTANCE hmod, DWORD dwThreadId);	//W and A versions have the same signature.
	LRESULT (WINAPI *pCallNextHookEx) (HHOOK hhk, int nCode, WPARAM wParam, LPARAM lParam);
	BOOL (WINAPI *pUnhookWindowsHookEx) (HHOOK hhk);
} winkeys;

cvar_t	sys_disableWinKeys = CVAR("sys_disableWinKeys", "0");
cvar_t	sys_disableTaskSwitch = CVARF("sys_disableTaskSwitch", "0", CVAR_NOTFROMSERVER);	// please don't encourage people to use this...

LRESULT CALLBACK LowLevelKeyboardProc (INT nCode, WPARAM wParam, LPARAM lParam)
{
	KBDLLHOOKSTRUCT *pkbhs = (KBDLLHOOKSTRUCT *) lParam;
	if (vid.activeapp)
	switch (nCode)
	{
	case HC_ACTION:
		{
		//Trap the Left Windowskey
			if (pkbhs->vkCode == VK_SNAPSHOT)
			{
				IN_KeyEvent (0, !(pkbhs->flags & LLKHF_UP), K_PRINTSCREEN, 0);
				return 1;
			}
			if (sys_disableWinKeys.ival)
			{
				if (pkbhs->vkCode == VK_LWIN)
				{
					IN_KeyEvent (0, !(pkbhs->flags & LLKHF_UP), K_LWIN, 0);
					return 1;
				}
			//Trap the Right Windowskey
				if (pkbhs->vkCode == VK_RWIN)
				{
					IN_KeyEvent(0, !(pkbhs->flags & LLKHF_UP), K_RWIN, 0);
					return 1;
				}
			//Trap the Application Key (what a pointless key)
				if (pkbhs->vkCode == VK_APPS)
				{
					IN_KeyEvent (0, !(pkbhs->flags & LLKHF_UP), K_APP, 0);
					return 1;
				}
			}

		// Disable CTRL+ESC
			//this works, but we've got to give some way to tab out...
			if (sys_disableTaskSwitch.ival)
			{
				if (pkbhs->vkCode == VK_ESCAPE && GetAsyncKeyState (VK_CONTROL) >> ((sizeof(SHORT) * 8) - 1))
					return 1;
		// Disable ATL+TAB
				if (pkbhs->vkCode == VK_TAB && pkbhs->flags & LLKHF_ALTDOWN)
					return 1;
		// Disable ALT+ESC
				if (pkbhs->vkCode == VK_ESCAPE && pkbhs->flags & LLKHF_ALTDOWN)
					return 1;
			}

			break;
		}
	default:
		break;
	}
	return winkeys.pCallNextHookEx (winkeys.llkeyboardhook, nCode, wParam, lParam);
}

void SetHookState(qboolean state)
{
	if (!sys_disableTaskSwitch.ival && !sys_disableWinKeys.ival)
		state = false;

	if (!state == !winkeys.llkeyboardhook)	//not so types are comparable
		return;
	if (!winkeys.pSetWindowsHookEx)
	{
		HMODULE dll = LoadLibraryA("user32.dll");
		if (!dll)
			return;
		winkeys.pSetWindowsHookEx		= (void*)GetProcAddress(dll, WinNT?"SetWindowsHookExW":"SetWindowsHookExA");
		winkeys.pCallNextHookEx			= (void*)GetProcAddress(dll, "CallNextHookEx");
		winkeys.pUnhookWindowsHookEx	= (void*)GetProcAddress(dll, "UnhookWindowsHookEx");
		if (!winkeys.pSetWindowsHookEx)
			return;
	}

	if (winkeys.llkeyboardhook)
	{
		winkeys.pUnhookWindowsHookEx(winkeys.llkeyboardhook);
		winkeys.llkeyboardhook = NULL;
	}
	if (state)
		winkeys.llkeyboardhook = winkeys.pSetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
}

#endif

/*
===============================================================================

FILE IO

===============================================================================
*/

void Sys_mkdir (const char *path)
{
	if (WinNT)
	{
		wchar_t wide[MAX_OSPATH];
		widen(wide, sizeof(wide), path);
		CreateDirectoryW(wide, NULL);
	}
	else
		_mkdir (path);
}

qboolean Sys_rmdir (const char *path)
{
	if (WinNT)
	{
		wchar_t wide[MAX_OSPATH];
		widen(wide, sizeof(wide), path);
		return RemoveDirectoryW(wide);
	}
	else
		return 0==_mkdir (path);
}


qboolean Sys_remove (const char *path)
{
	if (WinNT)
	{
		wchar_t wide[MAX_OSPATH];
		DWORD err;
		widen(wide, sizeof(wide), path);
		if (DeleteFileW(wide))
			return true;	//success
		err = GetLastError();
		if (err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND)
			return true;	//succeed when the file already didn't exist
		if (err == ERROR_ACCESS_DENIED)
			return false;	//windows is shite. this may simply include that its open in another process that didn't include the SHARE_DELETE permission.
		return false;		//other errors? panic
	}
	else
	{
		if (remove (path) != 0)
		{
			int e = errno;
			if (e == ENOENT)
				return true;	//return success if it doesn't already exist.
			return false;
		}
		return true;
	}
}

qboolean Sys_Rename (const char *oldfname, const char *newfname)
{
	if (WinNT)
	{
		wchar_t oldwide[MAX_OSPATH];
		wchar_t newwide[MAX_OSPATH];
		widen(oldwide, sizeof(oldwide), oldfname);
		widen(newwide, sizeof(newwide), newfname);
		return MoveFileW(oldwide, newwide);
	}
	else
		return !rename(oldfname, newfname);
}

#ifdef _MSC_VER
#define ULL(x) x##ui64
#else
#define ULL(x) x##ull
#endif

static time_t Sys_FileTimeToTime(FILETIME ft)
{
	ULARGE_INTEGER ull;
	ull.LowPart = ft.dwLowDateTime;
	ull.HighPart = ft.dwHighDateTime;
	return ull.QuadPart / ULL(10000000) - ULL(11644473600);
}

static int Sys_EnumerateFiles_9x (const char *match, int matchstart, int neststart, int (QDECL *func)(const char *fname, qofs_t fsize, time_t mtime, void *parm, searchpathfuncs_t *spath), void *parm, searchpathfuncs_t *spath)
{
	qboolean go;
	HANDLE r;
	WIN32_FIND_DATAA fd;
	int nest = neststart;	//neststart refers to just after a /
	qboolean wild = false;

	while(match[nest] && match[nest] != '/')
	{
		if (match[nest] == '?' || match[nest] == '*')
			wild = true;
		nest++;
	}
	if (match[nest] == '/')
	{
		char submatch[MAX_OSPATH];
		char tmproot[MAX_OSPATH];
		char file[MAX_OSPATH];

		if (!wild)
			return Sys_EnumerateFiles_9x(match, matchstart, nest+1, func, parm, spath);

		if (nest-neststart+1> MAX_OSPATH)
			return 1;
		memcpy(submatch, match+neststart, nest - neststart);
		submatch[nest - neststart] = 0;
		nest++;

		if (neststart+4 > MAX_OSPATH)
			return 1;
		memcpy(tmproot, match, neststart);
		strcpy(tmproot+neststart, "*.*");

		r = FindFirstFileA(tmproot, &fd);
		strcpy(tmproot+neststart, "");
		if (r==(HANDLE)-1)
			return 1;
		go = true;
		do
		{
			if (*fd.cFileName == '.');	//don't ever find files with a name starting with '.'
			else if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)	//is a directory
			{
				if (wildcmp(submatch, fd.cFileName))
				{
					int newnest;
					if (strlen(tmproot) + strlen(fd.cFileName) + strlen(match+nest) + 2 < MAX_OSPATH)
					{
						Q_snprintfz(file, sizeof(file), "%s%s/", tmproot, fd.cFileName);
						newnest = strlen(file);
						strcpy(file+newnest, match+nest);
						go = Sys_EnumerateFiles_9x(file, matchstart, newnest, func, parm, spath);
					}
				}
			}
		} while(FindNextFileA(r, &fd) && go);
		FindClose(r);
	}
	else
	{
		const char *submatch = match + neststart;
		char tmproot[MAX_OSPATH];
		char file[MAX_OSPATH];

		if (neststart+4 > MAX_OSPATH)
			return 1;
		memcpy(tmproot, match, neststart);
		strcpy(tmproot+neststart, "*.*");

		r = FindFirstFileA(tmproot, &fd);
		strcpy(tmproot+neststart, "");
		if (r==(HANDLE)-1)
			return 1;
		go = true;
		do
		{
			if (*fd.cFileName == '.')
				;	//don't ever find files with a name starting with '.' (includes .. and . directories, and unix hidden files)
			else if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)	//is a directory
			{
				if (wildcmp(submatch, fd.cFileName))
				{
					if (strlen(tmproot+matchstart) + strlen(fd.cFileName) + 2 < MAX_OSPATH)
					{
						Q_snprintfz(file, sizeof(file), "%s%s/", tmproot+matchstart, fd.cFileName);
						go = func(file, qofs_Make(fd.nFileSizeLow, fd.nFileSizeHigh), Sys_FileTimeToTime(fd.ftLastWriteTime), parm, spath);
					}
				}
			}
			else
			{
				if (wildcmp(submatch, fd.cFileName))
				{
					if (strlen(tmproot+matchstart) + strlen(fd.cFileName) + 1 < MAX_OSPATH)
					{
						Q_snprintfz(file, sizeof(file), "%s%s", tmproot+matchstart, fd.cFileName);
						go = func(file, qofs_Make(fd.nFileSizeLow, fd.nFileSizeHigh), Sys_FileTimeToTime(fd.ftLastWriteTime), parm, spath);
					}
				}
			}
		} while(FindNextFileA(r, &fd) && go);
		FindClose(r);
	}
	return go;
}
static int Sys_EnumerateFiles_NT (const char *match, int matchstart, int neststart, int (QDECL *func)(const char *fname, qofs_t fsize, time_t mtime, void *parm, searchpathfuncs_t *spath), void *parm, searchpathfuncs_t *spath)
{
	qboolean go;
	HANDLE r;
	WIN32_FIND_DATAW fd;
	int nest = neststart;	//neststart refers to just after a /
	qboolean wild = false;
	char tmproot[MAX_OSPATH];
	char utf8[MAX_OSPATH];
	char file[MAX_OSPATH];

	for(;;)
	{
		while(match[nest] && match[nest] != '/')
		{
			if (match[nest] == '?' || match[nest] == '*')
				wild = true;
			nest++;
		}

		if (match[nest] == '/' && !wild)
		{
			nest = neststart = nest+1;
			wild = false;
		}
		else
			break;
	}
	if (match[nest] == '/')
	{
		char submatch[MAX_OSPATH];

		if (nest-neststart+1> MAX_OSPATH)
			return 1;
		memcpy(submatch, match+neststart, nest - neststart);
		submatch[nest - neststart] = 0;
		nest++;

		if (neststart+4 > MAX_OSPATH)
			return 1;
		memcpy(tmproot, match, neststart);
		strcpy(tmproot+neststart, "*.*");

		{
			wchar_t wroot[MAX_OSPATH];
			r = FindFirstFileW(widen(wroot, sizeof(wroot), tmproot), &fd);
		}
		strcpy(tmproot+neststart, "");
		if (r==(HANDLE)-1)
			return 1;
		go = true;
		do
		{
			narrowen(utf8, sizeof(utf8), fd.cFileName);
			if (*utf8 == '.');	//don't ever find files with a name starting with '.'
			else if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)	//is a directory
			{
				if (wildcmp(submatch, utf8))
				{
					int newnest;
					if (strlen(tmproot) + strlen(utf8) + strlen(match+nest) + 2 < MAX_OSPATH)
					{
						Q_snprintfz(file, sizeof(file), "%s%s/", tmproot, utf8);
						newnest = strlen(file);
						strcpy(file+newnest, match+nest);
						go = Sys_EnumerateFiles_NT(file, matchstart, newnest, func, parm, spath);
					}
				}
			}
		} while(FindNextFileW(r, &fd) && go);
		FindClose(r);
	}
	else
	{
		const char *submatch = match + neststart;
		char tmproot[MAX_OSPATH];

		if (neststart+4 > MAX_OSPATH)
			return 1;
		memcpy(tmproot, match, neststart);
		strcpy(tmproot+neststart, "*.*");

		{
			wchar_t wroot[MAX_OSPATH];
			r = FindFirstFileW(widen(wroot, sizeof(wroot), tmproot), &fd);
		}
		strcpy(tmproot+neststart, "");
		if (r==(HANDLE)-1)
			return 1;
		go = true;
		do
		{
			narrowen(utf8, sizeof(utf8), fd.cFileName);
			if (*utf8 == '.')
				;	//don't ever find files with a name starting with '.' (includes .. and . directories, and unix hidden files)
			else if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)	//is a directory
			{
				if (wildcmp(submatch, utf8))
				{
					if (strlen(tmproot+matchstart) + strlen(utf8) + 2 < MAX_OSPATH)
					{
						Q_snprintfz(file, sizeof(file), "%s%s/", tmproot+matchstart, utf8);
						go = func(file, qofs_Make(fd.nFileSizeLow, fd.nFileSizeHigh), Sys_FileTimeToTime(fd.ftLastWriteTime), parm, spath);
					}
				}
			}
			else
			{
				if (wildcmp(submatch, utf8))
				{
					if (strlen(tmproot+matchstart) + strlen(utf8) + 1 < MAX_OSPATH)
					{
						Q_snprintfz(file, sizeof(file), "%s%s", tmproot+matchstart, utf8);
						go = func(file, qofs_Make(fd.nFileSizeLow, fd.nFileSizeHigh), Sys_FileTimeToTime(fd.ftLastWriteTime), parm, spath);
					}
				}
			}
		} while(FindNextFileW(r, &fd) && go);
		FindClose(r);
	}
	return go;
}
int Sys_EnumerateFiles (const char *gpath, const char *match, int (QDECL *func)(const char *fname, qofs_t fsize, time_t mtime, void *parm, searchpathfuncs_t *spath), void *parm, searchpathfuncs_t *spath)
{
	char fullmatch[MAX_OSPATH];
	int start;
	if (!gpath)
		gpath = "";
	if (strlen(gpath) + strlen(match) + 2 > MAX_OSPATH)
		return 1;

	strcpy(fullmatch, gpath);
	start = strlen(fullmatch);
	if (start && fullmatch[start-1] != '/')
		fullmatch[start++] = '/';
	fullmatch[start] = 0;
	strcat(fullmatch, match);
	if (WinNT)
		return Sys_EnumerateFiles_NT(fullmatch, start, start, func, parm, spath);
	else
		return Sys_EnumerateFiles_9x(fullmatch, start, start, func, parm, spath);
}

//wide only. we let the windows api sort out the mess of file urls. system-wide consistancy.
qboolean Sys_ResolveFileURL(const char *inurl, int inlen, char *out, int outlen)
{
	char *cp;
	wchar_t wurl[MAX_PATH];
	wchar_t local[MAX_PATH];
	DWORD grr;
	static HRESULT (WINAPI *pPathCreateFromUrlW)(PCWSTR pszUrl, PWSTR pszPath, DWORD *pcchPath, DWORD dwFlags);
	if (!pPathCreateFromUrlW)
		pPathCreateFromUrlW = Sys_GetAddressForName(Sys_LoadLibrary("Shlwapi.dll", NULL), "PathCreateFromUrlW");
	if (!pPathCreateFromUrlW)
		return false;

	//need to make a copy, because we can't terminate the inurl easily.
	cp = malloc(inlen+1);
	memcpy(cp, inurl, inlen);
	cp[inlen] = 0;
	widen(wurl, sizeof(wurl), cp);
	free(cp);
	grr = sizeof(local)/sizeof(wchar_t);
	if (FAILED(pPathCreateFromUrlW(wurl, local, &grr, 0)))
		return false;
	narrowen(out, outlen, local);
	while(*out)
	{
		if (*out == '\\')
			*out = '/';
		out++;
	}
	return true;
}
/*
===============================================================================

SYSTEM IO

===============================================================================
*/

/*
================
Sys_MakeCodeWriteable
================
*/
#if 0
void Sys_MakeCodeWriteable (void *startaddr, unsigned long length)
{
	DWORD  flOldProtect;

//@@@ copy on write or just read-write?
	if (!VirtualProtect(startaddr, length, PAGE_EXECUTE_READWRITE, &flOldProtect))
	{
		char str[1024];

		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
						NULL,
						GetLastError(),
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
						str,
						sizeof(str),
						NULL);
		Sys_Error("Protection change failed!\nError %d: %s\n", (int)GetLastError(), str);
	}
}
#endif

void Sys_DoFileAssociations(int elevated, const char *scheme);
void Sys_Register_File_Associations_f(void)
{
	if (!Q_strcasecmp(Cmd_Argv(1), "quiet"))
		Sys_DoFileAssociations(2, fs_manifest->schemes);	//current user only.
	else
		Sys_DoFileAssociations(0, fs_manifest->schemes);	//user+machine(with elevation on failure)
}

static void QDECL Sys_Priority_Changed(cvar_t *var, char *oldval)
{
	HANDLE h = GetCurrentProcess();
	DWORD pc;

	if (var->ival >= 3)
		pc = REALTIME_PRIORITY_CLASS;
	else if (var->ival >= 2)
		pc = HIGH_PRIORITY_CLASS;
	else if (var->ival >= 1)
		pc = ABOVE_NORMAL_PRIORITY_CLASS;
	else if (var->ival >= 0)
		pc = NORMAL_PRIORITY_CLASS;
	else if (var->ival >= -1)
		pc = BELOW_NORMAL_PRIORITY_CLASS;
	else
		pc = IDLE_PRIORITY_CLASS;

	SetPriorityClass(h, pc);
}
static cvar_t sys_priority = CVARFCD("sys_highpriority", "0", CVAR_NOTFROMSERVER, Sys_Priority_Changed, "Controls the process priority");
static cvar_t sys_clocktype = CVARFCD("sys_clocktype", "", CVAR_NOTFROMSERVER, Sys_ClockType_Changed, "Controls which system clock to base timings from.\n0: auto\n1: timeGetTime (low precision).\n2: QueryPerformanceCounter (may drift, desync between cpu cores, or run fast with longer uptimes depending on cpu(s) and windows version).\n3: QueryPerformanceCounter-with-force-affinity (shouldn't drift, but may result in less cpu time available).");
static cvar_t sys_clockprecision = CVARFCD("sys_clockprecision", "1", CVAR_NOTFROMSERVER, Sys_ClockPrecision_Changed, "Attempts to control windows' interrupt interval, in milliseconds. This can cause windows to give better clock precision and shorter waits, but also more overhead from process rescheduling.");
/*
================
Sys_Init
================
*/
void Sys_Init (void)
{
//	LARGE_INTEGER	PerformanceFreq;
//	unsigned int	lowpart, highpart;

	Sys_QueryDesktopParameters();

	Cvar_Register(&sys_priority, "System vars");

	Cvar_Register(&sys_clocktype, "System vars");
	Cvar_Register(&sys_clockprecision, "System vars");
#ifndef SERVERONLY
	Cvar_Register(&sys_disableWinKeys, "System vars");
	Cvar_Register(&sys_disableTaskSwitch, "System vars");
	Cmd_AddCommandD("sys_register_file_associations", Sys_Register_File_Associations_f, "Register FTE as the system handler for .bsp .mvd .qwd .dem files. Also register the URL protocol. This command will probably trigger a UAC prompt in Windows Vista and up. Deny it for current-user-only asociations (will also prevent listing in windows' 'default programs' ui due to microsoft bugs/limitations).");

#ifdef QUAKESPYAPI
#ifndef CLIENTONLY
	if (!isDedicated && !COM_CheckParm("-nomutex"))
#else
	if (!COM_CheckParm("-nomutex"))	//we need to create a mutex to allow gamespy to realise that we're running, but it might not be desired as it prevents other clients from running too.
#endif
	{
		// allocate a named semaphore on the client so the
		// front end can tell if it is alive

		// mutex will fail if semephore already exists
		qwclsemaphore = CreateMutex(
			NULL,         // Security attributes
			0,            // owner
			"qwcl"); // Semaphore name
	//	if (!qwclsemaphore)
	//		Sys_Error ("QWCL is already running on this system");
		CloseHandle (qwclsemaphore);

		qwclsemaphore = CreateSemaphore(
			NULL,         // Security attributes
			0,            // Initial count
			1,            // Maximum count
			"qwcl"); // Semaphore name
	}
#endif
#endif

	Sys_InitClock();

#if _MSC_VER >= 1600 //msvc2010 runtime does not work on 9x any more. get rid of the deprecation warnings in later versions.
	WinNT = true;

	qwinvermaj = 6;	//Hack: assume 6.2 (aka win8). this will block 16bit colour depths.
	qwinvermin = 2;
#else
	{
		OSVERSIONINFO	vinfo;
		vinfo.dwOSVersionInfoSize = sizeof(vinfo);
		if (!GetVersionEx (&vinfo))
			Sys_Error ("Couldn't get OS info");

		if ((vinfo.dwMajorVersion < 4) ||
			(vinfo.dwPlatformId == VER_PLATFORM_WIN32s))
		{
			Sys_Error (FULLENGINENAME " requires at least Win95 or NT 4.0");
		}

		if (vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
			WinNT = true;
		else
			WinNT = false;

		qwinvermaj = vinfo.dwMajorVersion;
		qwinvermin = vinfo.dwMinorVersion;
	}
#endif
}


void Sys_Shutdown(void)
{
	int i;
	if (tevent)
		CloseHandle (tevent);
	tevent = NULL;

	if (qwclsemaphore)
		CloseHandle (qwclsemaphore);
	qwclsemaphore = NULL;

	for (i = 0; i < MAX_NUM_ARGVS; i++)
	{
		if (!sys_argv[i])
			break;
		free(sys_argv[i]);
		sys_argv[i] = NULL;
	}
}

void VARGS Sys_Error (const char *error, ...)
{
	va_list		argptr;
	char		text[1024];
//	DWORD		dummy;

 	va_start (argptr, error);
	vsnprintf (text, sizeof(text), error, argptr);
	va_end (argptr);

	COM_WorkerAbort(text);

#ifndef SERVERONLY
	SetHookState(false);
	Host_Shutdown ();
#else
	SV_Shutdown();
#endif

	{
		wchar_t		wtext[1024];
		widen(wtext, sizeof(wtext), text);
		MessageBoxW(NULL, wtext, L"Error", 0);
	}

#ifndef SERVERONLY
	CloseHandle (qwclsemaphore);
	SetHookState(false);
#endif

	TL_Shutdown();

#ifdef USE_MSVCRT_DEBUG
	if (_CrtDumpMemoryLeaks())
		OutputDebugStringA("Leaks detected\n");
#endif

	exit (EXIT_FAILURE);
}

void VARGS Sys_Printf (char *fmt, ...)
{
	va_list		argptr;
	char		text[4096];
	DWORD		dummy;

	conchar_t msg[4096], *end, *in;
	wchar_t wide[4096], *out;
	int wlen;

	if (!houtput && !debugout && !SSV_IsSubServer())
		return;

	va_start (argptr,fmt);
	vsnprintf (text, sizeof(text), fmt, argptr);
	va_end (argptr);

#ifdef SUBSERVERS
	if (SSV_IsSubServer())
	{
		SSV_PrintToMaster(text);
		return;
	}
#endif

	end = COM_ParseFunString(CON_WHITEMASK, text, msg, sizeof(msg), false);
	out = wide;
	in = msg;
	wlen = 0;
	for (in = msg; in < end && wlen+3 < countof(wide); )
	{
		unsigned int flags, cp;
		in = Font_Decode(in, &flags, &cp);
		if (!(flags & CON_HIDDEN))
		{
			cp = COM_DeQuake(cp);
			if (cp > 0xffff)
			{
				cp -= 0x10000;
				*out++ = 0xD800 | (cp>>10);
				*out++ = 0xDC00 | (cp&0x3ff);
				wlen += 2;
			}
			else
			{
				*out++ = cp;
				wlen++;
			}
		}
	}
	*out = 0;

	if (debugout)
		OutputDebugStringW(wide);
	if (houtput)
	{
		if (WinNT)
			WriteConsoleW(houtput, wide, wlen, &dummy, NULL);
		else
		{
			//win95 doesn't support wide chars *sigh*. blank consoles suck. this conversion might loose stuff if the multibytes are too long.
			WriteConsole(houtput, text, WideCharToMultiByte(CP_ACP, 0, wide, wlen, text, sizeof(text), NULL, NULL), &dummy, NULL);
		}
	}
}

static unsigned int sys_interrupt_freq;
static void Sys_ClockPrecision_Changed(cvar_t *var, char *oldval)
{
	if (sys_interrupt_freq)
		timeEndPeriod(sys_interrupt_freq);
	sys_interrupt_freq = 0;

	if (var && var->ival > 0)
	{
		sys_interrupt_freq = var->ival;
		if (TIMERR_NOERROR != timeBeginPeriod(sys_interrupt_freq) && oldval)
			Con_Printf(CON_ERROR"%s: timeBeginPeriod(%u) failed.\n", var->name, sys_interrupt_freq);
	}
}

static quint64_t timer_qpc_frequency;
static unsigned int timer_tgt_period;
static quint64_t timer_basetime;	//used by all clocks to bias them to starting at 0
static DWORD_PTR timer_threadaffinity;
static enum
{
	CLOCK_TGT = 1,
	CLOCK_QPC = 2,
	CLOCK_QPC_SINGLE = 3,
} timer_clocktype;
static quint64_t Sys_GetClock(quint64_t *freq)
{
	if (timer_clocktype == CLOCK_QPC || timer_clocktype == CLOCK_QPC_SINGLE)
	{
		static LARGE_INTEGER last;
		LARGE_INTEGER pc;
		QueryPerformanceCounter(&pc);
		*freq = timer_qpc_frequency;
		if (last.QuadPart <= pc.QuadPart)	//never let it go backwards. multiple cpus are bad. ignore it till it catches up.
			last.QuadPart = pc.QuadPart;
		return last.QuadPart - timer_basetime;
	}
	else //if (timer_clocktype == CLOCK_TGT)
	{
		static DWORD last;
		DWORD cur = timeGetTime();
		if (last > cur)
		{
			timer_basetime -= (quint64_t)1 << 32;	//if it wrapped then try to compensate with the 64bit var that we do use so that Sys_DoubleTime won't suddenly go backwards
			if (host_initialized)
				Con_Printf("Clock wrapped\n");
		}
		last = cur;
		*freq = 1000;
		return last - timer_basetime;
	}
}
static void Sys_ClockType_Changed(cvar_t *var, char *oldval)
{
	int newtype = var?var->ival:0;
	if (newtype <= 0)
		newtype = CLOCK_QPC;
	if ((newtype == CLOCK_QPC || newtype == CLOCK_QPC_SINGLE) && !timer_qpc_frequency)
		newtype = CLOCK_TGT;	//QueryPerformanceCounter can fail on older versions of windows.

	if (newtype != timer_clocktype)
	{
		quint64_t oldtime, oldfreq;
		quint64_t newtime, newfreq;


		oldtime = Sys_GetClock(&oldfreq);
		//kill old clock's evil global settings
		if (timer_clocktype == CLOCK_TGT)
			timeEndPeriod(timer_tgt_period);
		if (timer_clocktype == CLOCK_QPC_SINGLE)	//restore it.
			SetThreadAffinityMask(GetCurrentThread(), timer_threadaffinity);
		
		//override necessary global state for new clock mode
		if (newtype == CLOCK_QPC_SINGLE)			//lock it down to a single core
		{
			DWORD_PTR old = timer_threadaffinity, sys, m;
			if (!old)
			{	//try and get the default/existing value....
				GetProcessAffinityMask(GetCurrentProcess(), &old, &sys);	//wtf? there is no thread call!
				old &= sys;
			}
			for (m = 1; old > m; m<<=1)
				;	//scan to find the LAST cpu. First cpu often has overheads like interrupt handling, second is weird and unreliable (and often hyperthreads with the first).
			old = SetThreadAffinityMask(GetCurrentThread(), old);
			if (!timer_threadaffinity)
				timer_threadaffinity = old;
		}
		if (newtype == CLOCK_TGT)
		{
			timeBeginPeriod(timer_tgt_period);
			if (host_initialized && timer_tgt_period > 1)
				Con_Printf(CON_WARNING"System timer is limited to only %ums precision\n", timer_tgt_period);
		}

		//switch over internal state
		timer_clocktype = newtype;
		timer_basetime = 0;
		newtime = Sys_GetClock(&newfreq);

		//and fix the bias to avoid crazy stalls due to offsets/frequencies changing.
		timer_basetime = newtime - (newfreq * (oldtime) / oldfreq);
	}
}
static void Sys_InitClock(void)
{
	quint64_t freq;
	TIMECAPS tc;
	LARGE_INTEGER t;

	//QPC timer
	if (QueryPerformanceFrequency(&t))
		timer_qpc_frequency = t.QuadPart;
	else
		timer_qpc_frequency = 0;

	//TGT timer
	timeGetDevCaps(&tc, sizeof(tc));
	timer_tgt_period = max(1,tc.wPeriodMin);	//make sure its at least 1, because 0 is probably a bug...

	//calibrate it, and apply.
	timer_basetime = Sys_GetClock(&freq);
	Sys_ClockType_Changed(&sys_clocktype, NULL);

	Sys_ClockPrecision_Changed(&sys_clocktype, NULL);
}
double Sys_DoubleTime (void)
{
	quint64_t denum, num = Sys_GetClock(&denum);
	return num / (long double)denum;
}
unsigned int Sys_Milliseconds (void)
{
	quint64_t denum, num = Sys_GetClock(&denum);
	num *= 1000;
	return num / denum;
}

void Sys_Quit (void)
{
	Sys_ClockType_Changed(NULL, NULL);
	Sys_ClockPrecision_Changed(NULL, NULL);

#ifndef SERVERONLY
	SetHookState(false);

	Host_Shutdown ();

	SetHookState(false);
#else
	SV_Shutdown();
#endif

	TL_Shutdown();

#ifdef RESTARTTEST
	longjmp(restart_jmpbuf, 1);
#endif

#ifdef USE_MSVCRT_DEBUG
	if (_CrtDumpMemoryLeaks())
		OutputDebugStringA("Leaks detected\n");
#endif

	exit(EXIT_SUCCESS);
}


#if 0
/*
================
Sys_DoubleTime
================
*/
double Sys_DoubleTime (void)
{
	static int			first = 1;
	static LARGE_INTEGER		qpcfreq;
	LARGE_INTEGER		PerformanceCount;
	static LONGLONG			oldcall;
	static LONGLONG			firsttime;
	LONGLONG			diff;

	QueryPerformanceCounter (&PerformanceCount);
	if (first)
	{
		first = 0;
		QueryPerformanceFrequency(&qpcfreq);
		firsttime = PerformanceCount.QuadPart;
		diff = 0;
	}
	else
		diff = PerformanceCount.QuadPart - oldcall;
	if (diff >= 0)
		oldcall = PerformanceCount.QuadPart;
	return (oldcall - firsttime) / (double)qpcfreq.QuadPart;
}
unsigned int Sys_Milliseconds (void)
{
	return Sys_DoubleTime()*1000;
}
#elif 0
unsigned int Sys_Milliseconds (void)
{
	static DWORD starttime;
	static qboolean first = true;
	DWORD now;
//	double t;

	now = timeGetTime();

	if (first) {
		first = false;
		starttime = now;
		return 0.0;
	}
	/*
	if (now < starttime) // wrapped?
	{
		double r;
		r = (now) + (LONG_MAX - starttime);
		starttime = now;
		return r;
	}

	if (now - starttime == 0)
		return 0.0;
*/
	return (now - starttime);
}

double Sys_DoubleTime (void)
{
	return Sys_Milliseconds()/1000.f;
}
#endif



/////////////////////////////////////////////////////////////
//clipboard
void Sys_Clipboard_PasteText(clipboardtype_t cbt, void (*callback)(void *cb, const char *utf8), void *ctx)
{
	HANDLE	clipboardhandle;
	char *cliputf8;
	if (OpenClipboard(NULL))
	{
		//windows programs interpret CF_TEXT as ansi (aka: gibberish)
		//so grab utf-16 text and convert it to utf-8 if our console parsing is set to accept that.
		clipboardhandle = GetClipboardData(CF_UNICODETEXT);
		if (clipboardhandle)
		{
			unsigned short *clipWText = GlobalLock(clipboardhandle);
			if (clipWText)
			{
				unsigned int l, c;
				char *utf8;
				for (l = 0; clipWText[l]; l++)
					;
				l = l*4 + 1;
				utf8 = cliputf8 = malloc(l);
				while(*clipWText)
				{
					unsigned int cp = *clipWText++;
					if (cp == '\r' && *clipWText == '\n')	//bloomin microsoft.
						cp = *clipWText++;
					if (cp >= 0xD800u && cp <= 0xDBFFu)
					{	//handle utf-16 surrogates
						if (*clipWText >= 0xDC00u && *clipWText <= 0xDFFFu)
						{
							cp = (cp&0x3ff)<<10;
							cp |= *clipWText++ & 0x3ff;
							cp += 0x10000;
						}
						else
							cp = 0xFFFDu;
					}

					c = utf8_encode(utf8, cp, l);
					if (!c)
						break;
					l -= c;
					utf8 += c;
				}
				*utf8 = 0;
				callback(ctx, cliputf8);
				free(cliputf8);
				GlobalUnlock(clipboardhandle);
				CloseClipboard();
				return;
			}

			//failed at the last hurdle

			GlobalUnlock(clipboardhandle);
		}

		clipboardhandle = GetClipboardData(CF_TEXT);
		if (clipboardhandle)
		{
			char *clipText = GlobalLock(clipboardhandle);
			if (clipText)
			{
				unsigned int l, c;
				char *utf8;
				for (l = 0; clipText[l]; l++)
					;
				l = l*4 + 1;
				utf8 = cliputf8 = malloc(l);
				while(*clipText)
				{
					if (clipText[0] == '\r' && clipText[1] == '\n')	//bloomin microsoft.
						clipText++;
					c = utf8_encode(utf8, *clipText++, l);
					if (!c)
						break;
					l -= c;
					utf8 += c;
				}
				*utf8 = 0;
				callback(ctx, cliputf8);
				free(cliputf8);
			}

			//failed at the last hurdle

			GlobalUnlock(clipboardhandle);
		}
		CloseClipboard();
	}
}
void Sys_SaveClipboard(clipboardtype_t cbt, const char *text)
{
	HANDLE glob;
	char *temp;
	unsigned short *tempw;
	unsigned int codepoint;
	if (!OpenClipboard(NULL))
		return;
	EmptyClipboard();

	if (WinNT)
	{
		glob = GlobalAlloc(GMEM_MOVEABLE, (strlen(text) + 1)*4);
		if (glob)
		{
			tempw = GlobalLock(glob);
			if (tempw != NULL)
			{
				int error;
				while(*text)
				{
					codepoint = unicode_decode(&error, text, &text, false);
					if (codepoint == '\n')
					{	//windows is stupid and annoying.
						*tempw++ = '\r';
						*tempw++ = '\n';
					}
					else if (codepoint > 0xffff)
					{	//and badly designed, too.
						codepoint -= 0x10000;
						*tempw++ = 0xD800 | ((codepoint>>10)&0x3ff);
						*tempw++ = 0xDC00 | (codepoint&0x3ff);
					}
					else
						*tempw++ = codepoint;
				}
				*tempw = 0;
				GlobalUnlock(glob);
				SetClipboardData(CF_UNICODETEXT, glob);
			}
			else
				GlobalFree(glob);
		}
	}
	else
	{
		glob = GlobalAlloc(GMEM_MOVEABLE, strlen(text)*2 + 1);
		if (glob)
		{
			//yes, quake chars will get mangled horribly.
			temp = GlobalLock(glob);
			if (temp != NULL)
			{
				int error;
				while (*text)
				{
					codepoint = unicode_decode(&error, text, &text, false);
					if (codepoint == '\n')
						*temp++ = '\r';
					*temp++ = codepoint;
				}
				*temp = 0;
				strcpy(temp, text);
				GlobalUnlock(glob);
				SetClipboardData(CF_TEXT, glob);
			}
			else
				GlobalFree(glob);
		}
	}

	CloseClipboard();
}


//end of clipboard
/////////////////////////////////////////////////////////////




/////////////////////////////////////////
//the system console stuff

char *Sys_ConsoleInput (void)
{
	static char	text[256];
	static int		len;
	INPUT_RECORD	recs[1024];
//	int		count;
	int		ch;
	DWORD numevents, numread, dummy=0;
	HANDLE	th;
	char	*clipText, *textCopied;

#ifdef SUBSERVERS
	if (SSV_IsSubServer())
		return NULL;
#endif


	if (!hinput)
		return NULL;

	for ( ;; )
	{
		if (!GetNumberOfConsoleInputEvents (hinput, &numevents))
			Sys_Error ("Error getting # of console events");

		if (numevents <= 0)
			break;

		if (WinNT)
		{
			if (!ReadConsoleInputW(hinput, recs, 1, &numread))
				Sys_Error ("Error reading console input");
		}
		else
		{
			if (!ReadConsoleInputA(hinput, recs, 1, &numread))
				Sys_Error ("Error reading console input");
		}

		if (numread != 1)
			Sys_Error ("Couldn't read console input");

		if (recs[0].EventType == KEY_EVENT)
		{
			if (recs[0].Event.KeyEvent.bKeyDown)
			{
				ch = recs[0].Event.KeyEvent.uChar.UnicodeChar;

				switch (ch)
				{
					case '\r':
						WriteFile(houtput, "\r\n", 2, &dummy, NULL);

						if (len)
						{
							text[len] = 0;
							len = 0;
							return text;
						}
						break;

					case '\b':
						if (len)
						{
							len--;
							WriteFile(houtput, "\b \b", 3, &dummy, NULL);
						}
						break;

					default:
						if (((ch=='V' || ch=='v') && (recs[0].Event.KeyEvent.dwControlKeyState &
							(LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))) || ((recs[0].Event.KeyEvent.dwControlKeyState
							& SHIFT_PRESSED) && (recs[0].Event.KeyEvent.wVirtualKeyCode
							==VK_INSERT))) {
							if (OpenClipboard(NULL)) {
								th = GetClipboardData(CF_TEXT);
								if (th) {
									clipText = GlobalLock(th);
									if (clipText) {
										int i;
										textCopied = BZ_Malloc(GlobalSize(th)+1);
										strcpy(textCopied, clipText);
/* Substitutes a NULL for every token */strtok(textCopied, "\n\r\b");
										i = strlen(textCopied);
										if (i+len>=256)
											i=256-len;
										if (i>0) {
											textCopied[i]=0;
											text[len]=0;
											strcat(text, textCopied);
											len+=dummy;
											WriteFile(houtput, textCopied, i, &dummy, NULL);
										}
										BZ_Free(textCopied);
									}
									GlobalUnlock(th);
								}
								CloseClipboard();
							}
						} else if (ch >= ' ')
						{
							wchar_t wch = ch;
							if (WinNT)
								WriteConsoleW(houtput, &wch, 1, &dummy, NULL);
							else
							{
								char mb[8];	//hopefully ucs-2-only will be sufficient...
								WriteConsoleA(houtput, mb, WideCharToMultiByte(CP_ACP, 0, &wch, 1, mb, sizeof(mb), NULL, NULL), &dummy, NULL);
							}
							len += utf8_encode(text+len, ch, sizeof(text)-1-len);
						}

						break;

				}
			}
		}
	}

	return NULL;
}

BOOL WINAPI HandlerRoutine (DWORD dwCtrlType)
{
	switch (dwCtrlType)
	{
		case CTRL_C_EVENT:
		case CTRL_BREAK_EVENT:
		case CTRL_CLOSE_EVENT:
		case CTRL_LOGOFF_EVENT:
		case CTRL_SHUTDOWN_EVENT:
			Cbuf_AddText ("quit\n", RESTRICT_LOCAL);
			return true;
	}

	return false;
}

#ifndef CP_UTF8
#define CP_UTF8                   65001
#endif
qboolean Sys_InitTerminal (void)
{
	DWORD m;

	if (SSV_IsSubServer())
		return true;	//just pretend we did

	if (!AllocConsole())
		return false;

#ifndef SERVERONLY
	if (qwclsemaphore)
	{
		CloseHandle(qwclsemaphore);
		qwclsemaphore = NULL;
	}
#endif

	SetConsoleCtrlHandler (HandlerRoutine, TRUE);
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
	SetConsoleTitleW (_L(FULLENGINENAME) L" dedicated server");
	if (isPlugin)
	{
		hinput = CreateFileA("CONIN$",GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
		houtput = CreateFileA("CONOUT$",GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
	}
	else
	{
		hinput = GetStdHandle (STD_INPUT_HANDLE);
		houtput = GetStdHandle (STD_OUTPUT_HANDLE);
	}

	if (hinput)
	{
		GetConsoleMode(hinput, &m);
		SetConsoleMode(hinput, m | 0x40 | 0x80);
	}

	return true;
}
void Sys_CloseTerminal (void)
{
	FreeConsole();

	hinput = NULL;
	houtput = NULL;
}


//
////////////////////////////

qboolean QCExternalDebuggerCommand(char *text);
void Sys_SendKeyEvents (void)
{
    MSG        msg;

	if (isPlugin)
	{
		DWORD avail;
		static char	text[256], *nl;
		static int textpos = 0;

		HANDLE input = GetStdHandle(STD_INPUT_HANDLE);
		if (!PeekNamedPipe(input, NULL, 0, NULL, &avail, NULL))
		{
			wantquit = true;
			Cmd_ExecuteString("quit force", RESTRICT_LOCAL);
		}
		else if (avail)
		{
			if (avail > sizeof(text)-1-textpos)
				avail = sizeof(text)-1-textpos;
			if (ReadFile(input, text+textpos, avail, &avail, NULL))
			{
				textpos += avail;
				if (textpos > sizeof(text)-1)
					Sys_Error("No.");
			}
		}
		while (textpos)
		{
			text[textpos] = 0;
			nl = strchr(text, '\n');
			if (nl)
			{
				*nl++ = 0;
				if (qrenderer <= QR_NONE && !strncmp(text, "vid_recenter ", 13))
				{
					Cmd_TokenizeString(text, false, false);
					sys_parentleft = strtoul(Cmd_Argv(1), NULL, 0);
					sys_parenttop = strtoul(Cmd_Argv(2), NULL, 0);
					sys_parentwidth = strtoul(Cmd_Argv(3), NULL, 0);
					sys_parentheight = strtoul(Cmd_Argv(4), NULL, 0); 
					sys_parentwindow = (HWND)(qintptr_t)strtoull(Cmd_Argv(5), NULL, 16);
				}
#if !defined(CLIENTONLY) || defined(CSQC_DAT) || defined(MENU_DAT)
				else if (QCExternalDebuggerCommand(text))
					/*handled elsewhere*/;
#endif
				else
				{
					Cbuf_AddText(text, RESTRICT_LOCAL);
					Cbuf_AddText("\n", RESTRICT_LOCAL);
				}
				memmove(text, nl, textpos - (nl - text));
				textpos -= (nl - text);
			}
			else
				break;
		}
	}
	if (isDedicated)
	{
#ifndef CLIENTONLY
		SV_GetConsoleCommands ();
#endif
		return;
	}

	if (gammapending)
	{
		gammapending -= host_frametime;
		if (gammapending < host_frametime)
		{
			gammapending = 0;
			Cvar_ForceCallback(&v_gamma);
		}
	}

	while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
	{
	// we always update if there are any event, even if we're paused
		//if (!GetMessage (&msg, NULL, 0, 0))
		//	break;
//			Sys_Quit ();
//		if (TranslateMessage (&msg))
//			continue;
      	DispatchMessage (&msg);
	}
}


void Sys_ServerActivity(void)
{
#ifndef SERVERONLY
	if (GetActiveWindow() != mainwindow)
		FlashWindow(mainwindow, true);
#endif
}

/*
==============================================================================

 WINDOWS CRAP

==============================================================================
*/

/*
==================
WinMain
==================
*/
void SleepUntilInput (int time)
{

	MsgWaitForMultipleObjects(1, &tevent, FALSE, time, QS_ALLINPUT);
}








qboolean Sys_Startup_CheckMem(quakeparms_t *parms)
{
	return true;
}

/*
==================
WinMain
==================
*/
HINSTANCE	global_hInstance;
int			global_nCmdShow;
HWND		hwnd_dialog;

static const IID qIID_IShellLinkW	= {0x000214F9L, 0, 0, {0xc0,0,0,0,0,0,0,0x46}};

#include <shlobj.h>
#if defined(_MSC_VER) && _MSC_VER <= 1200
#define pSHBrowseForFolderW SHBrowseForFolderW
#define pSHGetPathFromIDListW SHGetPathFromIDListW
#define pSHGetSpecialFolderPathW SHGetSpecialFolderPathW
#define pShell_NotifyIconW Shell_NotifyIconW
void Win7_Init(void)
{
}
#else

typedef struct qSHARDAPPIDINFOLINK {
  IShellLinkW *psl;
  PCWSTR     pszAppID;
} qSHARDAPPIDINFOLINK;

#define qSHARD_APPIDINFOLINK 0x00000007

typedef struct {
  GUID  fmtid;
  DWORD pid;
} qPROPERTYKEY;

typedef struct qIPropertyStore qIPropertyStore;
struct qIPropertyStore
{
    CONST_VTBL struct
	{
		/*IUnknown*/
		HRESULT ( STDMETHODCALLTYPE *QueryInterface )(
				qIPropertyStore * This,
				REFIID riid,
				void **ppvObject);
		ULONG ( STDMETHODCALLTYPE *AddRef )(
				qIPropertyStore * This);
		ULONG ( STDMETHODCALLTYPE *Release )(
				qIPropertyStore * This);

		/*property store stuff*/
		HRESULT ( STDMETHODCALLTYPE *GetCount)(
				qIPropertyStore * This,
				ULONG *count);

		HRESULT  ( STDMETHODCALLTYPE *GetAt)(
				qIPropertyStore * This,
				DWORD prop,
				qPROPERTYKEY * key);

		HRESULT  ( STDMETHODCALLTYPE *GetValue)(
				qIPropertyStore * This,
				qPROPERTYKEY * key,
				PROPVARIANT * val);

		HRESULT  ( STDMETHODCALLTYPE *SetValue)(
				qIPropertyStore * This,
				qPROPERTYKEY * key,
				PROPVARIANT * val);

		HRESULT  ( STDMETHODCALLTYPE *Commit)(
				qIPropertyStore * This);
	} *lpVtbl;
};

static const IID qIID_IPropertyStore = {0x886d8eeb, 0x8cf2, 0x4446, {0x8d, 0x02, 0xcd, 0xba, 0x1d, 0xbd, 0xcf, 0x99}};

#define qIObjectArray IUnknown 
static const IID qIID_IObjectArray = {0x92ca9dcd, 0x5622, 0x4bba, {0xa8,0x05,0x5e,0x9f,0x54,0x1b,0xd8,0xc9}};

typedef struct qIObjectCollection
{
    struct qIObjectCollectionVtbl
	{
		HRESULT ( __stdcall *QueryInterface )(
			/* [in] IShellLink*/ void *This,
			/* [in] */ const GUID * const riid,
			/* [out] */ void **ppvObject);

		ULONG ( __stdcall *AddRef )(
			/* [in] IShellLink*/ void *This);

		ULONG ( __stdcall *Release )(
			/* [in] IShellLink*/ void *This);

		HRESULT ( __stdcall *GetCount )(
			/* [in] IShellLink*/ void *This,
			/* [out] */ UINT *pcObjects);

		HRESULT ( __stdcall *GetAt )(
			/* [in] IShellLink*/ void *This,
			/* [in] */ UINT uiIndex,
			/* [in] */ const GUID * const riid,
			/* [iid_is][out] */ void **ppv);

		HRESULT ( __stdcall *AddObject )(
			/* [in] IShellLink*/ void *This,
			/* [in] */ void *punk);

		HRESULT ( __stdcall *AddFromArray )(
			/* [in] IShellLink*/ void *This,
			/* [in] */ qIObjectArray *poaSource);

		HRESULT ( __stdcall *RemoveObjectAt )(
			/* [in] IShellLink*/ void *This,
			/* [in] */ UINT uiIndex);

		HRESULT ( __stdcall *Clear )(
			/* [in] IShellLink*/ void *This);
	} *lpVtbl;
} qIObjectCollection;
static const IID qIID_IObjectCollection = {0x5632b1a4, 0xe38a, 0x400a, {0x92,0x8a,0xd4,0xcd,0x63,0x23,0x02,0x95}};
static const CLSID qCLSID_EnumerableObjectCollection = {0x2d3468c1, 0x36a7, 0x43b6, {0xac,0x24,0xd3,0xf0,0x2f,0xd9,0x60,0x7a}};

typedef struct qICustomDestinationList
{
	struct qICustomDestinationListVtbl
	{
		HRESULT ( __stdcall *QueryInterface ) (
			/* [in] ICustomDestinationList*/ void *This,
			/* [in] */  const GUID * const riid,
			/* [out] */ void **ppvObject);

		ULONG ( __stdcall *AddRef )(
			/* [in] ICustomDestinationList*/ void *This);

		ULONG ( __stdcall *Release )(
			/* [in] ICustomDestinationList*/ void *This);

		HRESULT ( __stdcall *SetAppID )(
			/* [in] ICustomDestinationList*/ void *This,
			/* [string][in] */ LPCWSTR pszAppID);

		HRESULT ( __stdcall *BeginList )(
			/* [in] ICustomDestinationList*/ void *This,
			/* [out] */ UINT *pcMinSlots,
			/* [in] */  const GUID * const riid,
			/* [out] */ void **ppv);

		HRESULT ( __stdcall *AppendCategory )(
			/* [in] ICustomDestinationList*/ void *This,
			/* [string][in] */ LPCWSTR pszCategory,
			/* [in] IObjectArray*/ void *poa);

		HRESULT ( __stdcall *AppendKnownCategory )(
			/* [in] ICustomDestinationList*/ void *This,
			/* [in] KNOWNDESTCATEGORY*/ int category);

		HRESULT ( __stdcall *AddUserTasks )(
			/* [in] ICustomDestinationList*/ void *This,
			/* [in] IObjectArray*/ void *poa);

		HRESULT ( __stdcall *CommitList )(
			/* [in] ICustomDestinationList*/ void *This);

		HRESULT ( __stdcall *GetRemovedDestinations )(
			/* [in] ICustomDestinationList*/ void *This,
			/* [in] */ const IID * const riid,
			/* [out] */ void **ppv);

		HRESULT ( __stdcall *DeleteList )(
			/* [in] ICustomDestinationList*/ void *This,
			/* [string][unique][in] */ LPCWSTR pszAppID);

		HRESULT ( __stdcall *AbortList )(
			/* [in] ICustomDestinationList*/ void *This);

	} *lpVtbl;
} qICustomDestinationList;

static const IID qIID_ICustomDestinationList = {0x6332debf, 0x87b5, 0x4670, {0x90,0xc0,0x5e,0x57,0xb4,0x08,0xa4,0x9e}};
static const CLSID qCLSID_DestinationList = {0x77f10cf0, 0x3db5, 0x4966, {0xb5,0x20,0xb7,0xc5,0x4f,0xd3,0x5e,0xd6}};

#define WIN7_APPNAME L"FTEQuake"

static IShellLinkW *CreateShellLink(char *command, char *target, char *title, char *desc)
{
	HRESULT hr;
	IShellLinkW *link;
	qIPropertyStore *prop_store;

	WCHAR buf[1024];
	char tmp[1024], *s;

	// Get a pointer to the IShellLink interface.
	hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &qIID_IShellLinkW, (void**)&link);
	if (FAILED(hr))
		return NULL;

	GetModuleFileNameW(NULL, buf, countof(buf)-1);
	IShellLinkW_SetIconLocation(link, buf, 0);  /*grab the first icon from our exe*/
	IShellLinkW_SetPath(link, buf); /*program to run*/

	Q_strncpyz(tmp, com_gamepath, sizeof(tmp));
	/*normalize the gamedir, so we don't end up with the same thing multiple times*/
	for(s = tmp; *s; s++)
	{
		if (*s == '\\')
			*s = '/';
		else
			*s = tolower(*s);
	}
	IShellLinkW_SetArguments(link, widen(buf, sizeof(buf), va("%s \"%s\" -basedir \"%s\"", command, target, tmp))); /*args*/
	IShellLinkW_SetDescription(link, widen(buf, sizeof(buf), desc));  /*tooltip*/


	hr = IShellLinkW_QueryInterface(link, &qIID_IPropertyStore, (void**)&prop_store);

	if(SUCCEEDED(hr))
	{
		PROPVARIANT pv;
		qPROPERTYKEY PKEY_Title;
		pv.vt=VT_LPSTR;
		pv.pszVal=title; /*item text*/
		CLSIDFromString(L"{F29F85E0-4FF9-1068-AB91-08002B27B3D9}", &(PKEY_Title.fmtid));
		PKEY_Title.pid=2;
		hr = prop_store->lpVtbl->SetValue(prop_store, &PKEY_Title, &pv);
		hr = prop_store->lpVtbl->Commit(prop_store);
		prop_store->lpVtbl->Release(prop_store);
	}

	return link;
}

void Sys_RecentServer(char *command, char *target, char *title, char *desc)
{
	qSHARDAPPIDINFOLINK appinfo;
	IShellLinkW *link;

	link = CreateShellLink(command, target, title, desc);
	if (!link)
		return;

	appinfo.pszAppID=WIN7_APPNAME;
	appinfo.psl=link;
	SHAddToRecentDocs(qSHARD_APPIDINFOLINK, &appinfo);
	IShellLinkW_Release(link);
}


typedef struct {
  LPCWSTR            pcszFile;
  LPCWSTR            pcszClass;
  int oaifInFlags;
} qOPENASINFO;
HRESULT (WINAPI *pSHOpenWithDialog)(HWND hwndParent, const qOPENASINFO *poainfo);


LPITEMIDLIST (STDAPICALLTYPE *pSHBrowseForFolderW)(LPBROWSEINFOW lpbi);
BOOL (STDAPICALLTYPE *pSHGetPathFromIDListW)(LPCITEMIDLIST pidl, LPWSTR pszPath);
BOOL (STDAPICALLTYPE *pSHGetSpecialFolderPathW)(HWND hwnd, LPWSTR pszPath, int csidl, BOOL fCreate);
BOOL (STDAPICALLTYPE *pShell_NotifyIconW)(DWORD dwMessage, PNOTIFYICONDATAW lpData);
void Win7_Init(void)
{
	HANDLE h;
	HRESULT (WINAPI *pSetCurrentProcessExplicitAppUserModelID)(PCWSTR AppID);

	h = LoadLibraryU("shell32.dll");
	if (h)
	{
		pSHBrowseForFolderW			= (void*)GetProcAddress(h, "SHBrowseForFolderW");
		pSHGetPathFromIDListW		= (void*)GetProcAddress(h, "SHGetPathFromIDListW");
		pSHGetSpecialFolderPathW	= (void*)GetProcAddress(h, "SHGetSpecialFolderPathW");
		pShell_NotifyIconW			= (void*)GetProcAddress(h, "Shell_NotifyIconW");

		pSHOpenWithDialog = (void*)GetProcAddress(h, "SHOpenWithDialog");

		pSetCurrentProcessExplicitAppUserModelID = (void*)GetProcAddress(h, "SetCurrentProcessExplicitAppUserModelID");
		if (pSetCurrentProcessExplicitAppUserModelID)
			pSetCurrentProcessExplicitAppUserModelID(WIN7_APPNAME);
	}
}

void Win7_TaskListInit(void)
{
	qICustomDestinationList *cdl;
	qIObjectCollection *col;
	qIObjectArray *arr;
	IShellLinkW *link;
	CoInitialize(NULL);
	if (SUCCEEDED(CoCreateInstance(&qCLSID_DestinationList, NULL, CLSCTX_INPROC_SERVER, &qIID_ICustomDestinationList, (void**)&cdl)))
	{
		UINT minslots;
		IUnknown *removed;
		cdl->lpVtbl->BeginList(cdl, &minslots, &qIID_IObjectArray, (void**)&removed);

		if (SUCCEEDED(CoCreateInstance(&qCLSID_EnumerableObjectCollection, NULL, CLSCTX_INPROC_SERVER, &qIID_IObjectCollection, (void**)&col)))
		{

			switch(M_GameType())
			{
			case MGT_QUAKE1:
				link = CreateShellLink("+menu_servers", "", "Server List", "Pick a multiplayer server to join");
				if (link)
				{
					col->lpVtbl->AddObject(col, (IUnknown*)link);
					link->lpVtbl->Release(link);
				}
				link = CreateShellLink("+map start", "", "Start New Game (Quake)", "Begin a new single-player game");
				if (link)
				{
					col->lpVtbl->AddObject(col, (IUnknown*)link);
					link->lpVtbl->Release(link);
				}
				break;
			case MGT_QUAKE2:
				link = CreateShellLink("+menu_servers", "", "Quake2 Server List", "Pick a multiplayer server to join");
				if (link)
				{
					col->lpVtbl->AddObject(col, (IUnknown*)link);
					link->lpVtbl->Release(link);
				}
				link = CreateShellLink("+map unit1", "", "Start New Game (Quake2)", "Begin a new game");
				if (link)
				{
					col->lpVtbl->AddObject(col, (IUnknown*)link);
					link->lpVtbl->Release(link);
				}
				break;
#ifdef HEXEN2
			case MGT_HEXEN2:
				link = CreateShellLink("+menu_servers", "", "Hexen2 Server List", "Pick a multiplayer server to join");
				if (link)
				{
					col->lpVtbl->AddObject(col, (IUnknown*)link);
					link->lpVtbl->Release(link);
				}
				link = CreateShellLink("+map demo1", "", "Start New Game (Hexen2)", "Begin a new game");
				if (link)
				{
					col->lpVtbl->AddObject(col, (IUnknown*)link);
					link->lpVtbl->Release(link);
				}
				break;
#endif
			}

			if (SUCCEEDED(col->lpVtbl->QueryInterface(col, &qIID_IObjectArray, (void**)&arr)))
			{
				cdl->lpVtbl->AddUserTasks(cdl, arr);
				arr->lpVtbl->Release(arr);
			}
			col->lpVtbl->Release(col);
		}
		cdl->lpVtbl->AppendKnownCategory(cdl, 1);
		cdl->lpVtbl->CommitList(cdl);
		cdl->lpVtbl->Release(cdl);
	}
}
#endif

//using this like posix' access function, but with much more code, microsoftisms, and no errno codes/info
//no, I don't really have a clue why it needs to be so long.
//#include <svrapi.h>
#ifndef ACCESS_READ	
#define         ACCESS_READ     0x1
#define         ACCESS_WRITE    0x2
#endif
static BOOL microsoft_accessW(LPWSTR pszFolder, DWORD dwAccessDesired)
{
	HANDLE			hToken;
	PRIVILEGE_SET	PrivilegeSet;
	DWORD			dwPrivSetSize;
	DWORD			dwAccessGranted;
	BOOL			fAccessGranted = FALSE;
	GENERIC_MAPPING	GenericMapping;
	SECURITY_INFORMATION si = (SECURITY_INFORMATION)( OWNER_SECURITY_INFORMATION|GROUP_SECURITY_INFORMATION|DACL_SECURITY_INFORMATION);
	PSECURITY_DESCRIPTOR psdSD = NULL;
	DWORD			dwNeeded;
	GetFileSecurityW(pszFolder,si,NULL,0,&dwNeeded);
	psdSD = malloc(dwNeeded);
	GetFileSecurityW(pszFolder,si,psdSD,dwNeeded,&dwNeeded);
	ImpersonateSelf(SecurityImpersonation);
	OpenThreadToken(GetCurrentThread(), TOKEN_ALL_ACCESS, TRUE, &hToken);
	memset(&GenericMapping, 0xff, sizeof(GENERIC_MAPPING));
	GenericMapping.GenericRead = ACCESS_READ;
	GenericMapping.GenericWrite = ACCESS_WRITE;
	GenericMapping.GenericExecute = 0;
	GenericMapping.GenericAll = ACCESS_READ | ACCESS_WRITE;
	MapGenericMask(&dwAccessDesired, &GenericMapping);
	dwPrivSetSize = sizeof(PRIVILEGE_SET);
	AccessCheck(psdSD, hToken, dwAccessDesired, &GenericMapping, &PrivilegeSet, &dwPrivSetSize, &dwAccessGranted, &fAccessGranted);
	free(psdSD);
	return fAccessGranted;
}
static BOOL microsoft_accessU(LPCSTR pszFolder, DWORD dwAccessDesired)
{
	wchar_t			wpath[MAX_OSPATH];
	return microsoft_accessW(widen(wpath, sizeof(wpath), pszFolder), dwAccessDesired);
}


#ifndef SVNREVISION
	#if 0	//1 to debug engine update in msvc.
		#define SVNREVISION 1
	#else
		#define SVNREVISION -
	#endif
#endif
#define SVNREVISIONSTR STRINGIFY(SVNREVISION)

#if 0//ndef NOLEGACY
	#if defined(SVNREVISION) && !defined(MINIMAL)
		#if defined(OFFICIAL_RELEASE)
			#define UPD_BUILDTYPE "rel"
		#else
			#define UPD_BUILDTYPE "test"
			//WARNING: Security comes from the fact that the triptohell.info certificate is hardcoded in the tls code.
			//this will correctly detect insecure tls proxies also.
//			#define UPDATE_URL_ROOT		"https://triptohell.info/moodles/"
//			#define UPDATE_URL_TESTED	UPDATE_URL_ROOT "autoup/"
//			#define UPDATE_URL_NIGHTLY	UPDATE_URL_ROOT
//			#define UPDATE_URL_VERSION	"%sversion.txt"
/*			#ifdef NOLEGACY
				#ifdef _WIN64
					#define UPDATE_URL_BUILD "%snocompat64/fte" EXETYPE "64.exe"
				#else
					#define UPDATE_URL_BUILD "%snocompat/fte" EXETYPE ".exe"
				#endif
			#else
				#ifdef _WIN64
					#define UPDATE_URL_BUILD "%swin64/fte" EXETYPE "64.exe"
				#else
					#define UPDATE_URL_BUILD "%swin32/fte" EXETYPE ".exe"
				#endif
			#endif*/
		#endif

		#if defined(SERVERONLY)
			#define EXETYPE "qwsv"	//not gonna happen, but whatever.
		#elif defined(GLQUAKE) && defined(D3DQUAKE)
			#define EXETYPE "qw"
		#elif defined(GLQUAKE)
			#ifdef MINIMAL
				#define EXETYPE "minglqw"
			#else
				#define EXETYPE "glqw"
			#endif
		#elif defined(D3DQUAKE)
			#define EXETYPE "d3dqw"
		#elif defined(SWQUAKE)
			#define EXETYPE "swqw"
		#else
			//erm...
			#define EXETYPE "qw"
		#endif
	#endif
#endif

#ifdef HAVEAUTOUPDATE
//this is for legacy reasons. old builds stored a 'pending' name in the registry for the 'frontend' to rename+use
qboolean Sys_SetUpdatedBinary(const char *newbinary)
{
/* //legacy crap, completely redundant when we're replacing the original binary
#ifdef UPD_BUILDTYPE
	//downloads menu has provided a new binary to use
	MyRegSetValue(HKEY_CURRENT_USER, "Software\\"FULLENGINENAME, "pending" UPD_BUILDTYPE EXETYPE, REG_SZ, newbinary, strlen(newbinary)+1);
#endif
*/
	wchar_t newbinaryw[MAX_OSPATH];
	wchar_t enginebinary[MAX_OSPATH];
	wchar_t enginebinarybackup[MAX_OSPATH+4];
	size_t len;
	static qboolean alreadymoved = false;
	
	//update blocked via commandline. to be doubly sure its checked.
	if (COM_CheckParm("-noupdate") || COM_CheckParm("--noupdate") || COM_CheckParm("-noautoupdate") || COM_CheckParm("--noautoupdate"))
		return false;

	//windows is annoying. we can't delete a file that's in use (no orphaning)
	//we can normally rename it to something else before writing a new file with the original name.
	//then delete the old file later (sadly only on reboot)

	//because microsoft suck.
	widen(newbinaryw, sizeof(newbinaryw), newbinary);
	//get the binary name
	GetModuleFileNameW(NULL, enginebinary, countof(enginebinary)-1);
#ifdef HAVE_LEGACY
	//--fromfrontend was removed for r5596.
	//it had two args - launcher version and launcher filename, so that if the user manually updated then it'd use the more recent build.
	//replace the original launcher instead.
	{
		int arg = COM_CheckParm("--fromfrontend");
		if (arg)
			widen(enginebinary, sizeof(enginebinary), com_argv[arg+2]);
	}
#endif
	//generate the temp name
	memcpy(enginebinarybackup, enginebinary, sizeof(enginebinary));
	len = wcslen(enginebinarybackup);
	if (len > 4 && !_wcsicmp(enginebinarybackup+len-4, L".exe"))
		len -= 4;	//swap its extension over, if we can.
	wcscpy(enginebinarybackup+len, L".bak");

	//can fail if we don't have write access. can fail for a few other reasons.
	if (alreadymoved || MoveFileExW(enginebinary, enginebinarybackup, MOVEFILE_REPLACE_EXISTING))
	{
		//can fail if windows is bugging out again
		if (CopyFileW(newbinaryw, enginebinary, FALSE))
		{	//delete the old one eventually, when we can.
			if (!DeleteFileW(enginebinarybackup))	//if we can directly delete it... sadly this will normally fail but lets try it anyway. maybe it'll get opened with SHARE_DELETE some time!
				MoveFileExW(enginebinarybackup, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);	//schedule it to delete if we can't do it from the start.
			alreadymoved = true; //if we already moved it, don't try to delete-by-overwrite the current binary, just delete it...
			return true;	//yay! it worked!
		}
		if (!alreadymoved)
			MoveFileExW(enginebinarybackup, enginebinary, 0);
	}
	alreadymoved = false;
	return false;
}
qboolean Sys_EngineMayUpdate(void)
{
	wchar_t enginebinaryw[MAX_OSPATH];
	if (!COM_CheckParm("-allowupdate"))
	{
		char enginebinary[MAX_OSPATH*4];
		char *s;
		if (revision_number(true) <= 0)
			return false;

		GetModuleFileNameW(NULL, enginebinaryw, countof(enginebinaryw)-1);
		narrowen(enginebinary, sizeof(enginebinary), enginebinaryw);
		//if there's 3 consecutive digits or digit.digit then assume the user is doing their own versioning, and disallow engine updates (unless they use the -allowupdate arg).
		//(that or they ran one of our older builds directly)
        for (s=COM_SkipPath(enginebinary); *s; s++)
		{
			if ( s[0] >= '0' && s[0] <= '9')
			if ((s[1] >= '0' && s[1] <= '9') || s[1] == '.' || s[1] == '_' || s[1] == '-')
			if ( s[2] >= '0' && s[2] <= '9')
				return false;
		}
	}

	//update blocked via commandline
	if (COM_CheckParm("-noupdate") || COM_CheckParm("--noupdate") || COM_CheckParm("-noautoupdate") || COM_CheckParm("--noautoupdate"))
		return false;

	return true;
}
/*
qboolean Sys_EngineWasUpdated(char *newbinary)
{
	wchar_t launchbinary[2048];
	wchar_t launcherbinary[MAX_OSPATH];
	wchar_t wargs[8192];
	wchar_t rev[2048];
	PROCESS_INFORMATION childinfo;
	STARTUPINFOW startinfo = {sizeof(startinfo)};

	//if we were called from a frontend, then don't chain to another, because that would be recursive, and that would be bad.
	if (COM_CheckParm("--fromfrontend"))
		return false;
	//if we're not allowed for some other reason
	if (!Sys_EngineCanUpdate())
		return false;

	memset(&childinfo, 0, sizeof(childinfo));
	startinfo.dwFlags = STARTF_USESTDHANDLES;
	startinfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	startinfo.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	startinfo.hStdError = GetStdHandle(STD_ERROR_HANDLE);

	GetModuleFileNameW(NULL, launcherbinary, countof(launcherbinary)-1);
	widen(launchbinary, sizeof(launchbinary), newbinary);
	_snwprintf(wargs, countof(wargs), L"%s --fromfrontend \"%s\" \"%s\"", GetCommandLineW(), widen(rev, sizeof(rev), SVNREVISIONSTR), launcherbinary);
	if (CreateProcessW(launchbinary, wargs, NULL, NULL, TRUE, 0, NULL, NULL, &startinfo, &childinfo))
	{
		CloseHandle(childinfo.hProcess);
		CloseHandle(childinfo.hThread);
		return true;	//it started up, we need to die now.
	}

	return false;	//failure!
}
*/
#endif

#include "shellapi.h"
const GUID qIID_IApplicationAssociationRegistrationUI = {0x1f76a169,0xf994,0x40ac, {0x8f,0xc8,0x09,0x59,0xe8,0x87,0x47,0x10}};
const GUID qCLSID_ApplicationAssociationRegistrationUI = {0x1968106d,0xf3b5,0x44cf,{0x89,0x0e,0x11,0x6f,0xcb,0x9e,0xce,0xf1}};
struct qIApplicationAssociationRegistrationUI;
typedef struct qIApplicationAssociationRegistrationUI
{
	struct qIApplicationAssociationRegistrationUI_vtab
	{
		HRESULT  (WINAPI *QueryInterface)				(struct qIApplicationAssociationRegistrationUI *, const GUID *riid, void **ppvObject);
		HRESULT  (WINAPI *AddRef)						(struct qIApplicationAssociationRegistrationUI *);
		HRESULT  (WINAPI *Release)						(struct qIApplicationAssociationRegistrationUI *);
		HRESULT  (WINAPI *LaunchAdvancedAssociationUI)	(struct qIApplicationAssociationRegistrationUI *, LPCWSTR app);
	} *lpVtbl;
} qIApplicationAssociationRegistrationUI;

char *Sys_URIScheme_NeedsRegistering(void)
{   //just disables the prompts.
	HKEY root;
	char buffer[2048];
	char scheme[64];
	const char *s, *schemes = fs_manifest->schemes;
	char *exec, *me;
	size_t i;
	wchar_t enginebinaryw[MAX_OSPATH];
	char enginebinary[MAX_OSPATH*4];

	for (s = schemes; (s=COM_ParseOut(s, scheme, sizeof(scheme))); )
	{
		root = HKEY_CURRENT_USER;
		if (!MyRegGetStringValue(root, va("Software\\Classes\\%s", scheme), "", buffer, sizeof(buffer)))
		{
			root = HKEY_LOCAL_MACHINE;
			if (!MyRegGetStringValue(root, va("Software\\Classes\\%s", scheme), "", buffer, sizeof(buffer)))
				break;
		}

		//the scheme exists at least...
		if (!MyRegGetStringValue(root, va("Software\\Classes\\%s\\shell\\open\\command", scheme), "", buffer, sizeof(buffer)))
			break; //erk, missing.
		COM_Parse(buffer);
		if (!microsoft_accessU(com_token, ACCESS_READ))
			break;	//can't read it? doesn't exist?
		exec = COM_SkipPath(com_token);
		for (i = 0; exec[i]; i++)
			if (exec[i] == '_' || exec[i] == '.' || (exec[i] >= '0' && exec[i] <= '9'))
			{	//anything that looks like a revision number
				exec[i] = 0;
				break;
			}

		GetModuleFileNameW(NULL, enginebinaryw, countof(enginebinaryw)-1);
		narrowen(enginebinary, sizeof(enginebinary), enginebinaryw);
		me = COM_SkipPath(enginebinary);
		for (i = 0; me[i]; i++)
			if (me[i] == '_' || me[i] == '.' || (me[i] >= '0' && me[i] <= '9'))
			{	//anything that looks like a revision number
				me[i] = 0;
				break;
			}
		if (Q_strcasecmp(exec, me))
			break;	//looks like its set to something else.
	}

	if (s)
		return Z_StrDup(scheme);
    return NULL;
}
void Sys_DoFileAssociations(int elevated, const char *schemes)
{
	//elevated:
	//	0: console command
	//	1: running as an elevated/admin process
	//	2: register as current user only (do not show associations prompt).
	char command[1024];
	char scheme[64];
	const char *s;
	qboolean ok = true;
	HKEY root;

	//I'd do everything in current_user if I could, but windows sucks too much for that.
	//'registered applications' simply does not work in hkcu, we MUST use hklm for that.
	//if there's a registered application and we are not, we are unable to grab that association, ever.
	//thus we HAVE to do things to the local machine or we might as well not bother doing anything.
	//still, with a manifest not giving false success, if the user clicks 'no' to the UAC prompt, we'll write everything to the current user anyway, so if microsoft do ever fix things, then yay.
	//also, I hate the idea of creating a 'registered application' in globally without the file types it uses being local.

	//on xp, we use ONLY current user. no 'registered applications' means no 'registered applications bug', which means no need to use hklm at all.
	//in vista/7, we have to create stuff in local_machine. in which case we might as well put ALL associations in there. the ui stuff will allow user-specific settings, so this is not an issue other than the fact that it triggers uac.
	//in 8, we cannot programatically force ownership of our associations, so we might as well just use the ui method even for vista+7 instead of the ruder version.
	//in win10, the 'ui' stuff is just a quick popup to tell the user to configure defaults themselves. hopefully we can fall back on the regular associations for when the user didn't override anyting.
	if (qwinvermaj < 6)
		elevated = 2;

	root = (elevated>=2)?HKEY_CURRENT_USER:HKEY_LOCAL_MACHINE;

//	#define ASSOC_VERSION 2
#define ASSOCV "1"

	//register the basic demo class
	Q_snprintfz(command, sizeof(command), "Quake or QuakeWorld Demo");
	ok = ok & MyRegSetValue(root, "Software\\Classes\\"DISTRIBUTION"_DemoFile."ASSOCV, "", REG_SZ, command, strlen(command));
	Q_snprintfz(command, sizeof(command), "\"%s\",0", com_argv[0]);
	ok = ok & MyRegSetValue(root, "Software\\Classes\\"DISTRIBUTION"_DemoFile."ASSOCV"\\DefaultIcon", "", REG_SZ, command, strlen(command));
	Q_snprintfz(command, sizeof(command), "\"%s\" \"%%1\"", com_argv[0]);
	ok = ok & MyRegSetValue(root, "Software\\Classes\\"DISTRIBUTION"_DemoFile."ASSOCV"\\shell\\open\\command", "", REG_SZ, command, strlen(command));
	if (ok)
	{	//and now the extensions themselves...
		MyRegSetValue(root, "Software\\Classes\\.qtv", "", REG_SZ, DISTRIBUTION"_DemoFile."ASSOCV, strlen(DISTRIBUTION"_DemoFile."ASSOCV));
		MyRegSetValue(root, "Software\\Classes\\.mvd", "", REG_SZ, DISTRIBUTION"_DemoFile."ASSOCV, strlen(DISTRIBUTION"_DemoFile."ASSOCV));
		MyRegSetValue(root, "Software\\Classes\\.qwd", "", REG_SZ, DISTRIBUTION"_DemoFile."ASSOCV, strlen(DISTRIBUTION"_DemoFile."ASSOCV));
		MyRegSetValue(root, "Software\\Classes\\.dem", "", REG_SZ, DISTRIBUTION"_DemoFile."ASSOCV, strlen(DISTRIBUTION"_DemoFile."ASSOCV));
	}

	//register the basic map class. yeah, the command is the same as for demos. but the description is different!
	Q_snprintfz(command, sizeof(command), "Quake Map");
	ok = ok & MyRegSetValue(root, "Software\\Classes\\"DISTRIBUTION"_BSPFile."ASSOCV, "", REG_SZ, command, strlen(command));
	Q_snprintfz(command, sizeof(command), "\"%s\",0", com_argv[0]);
	ok = ok & MyRegSetValue(root, "Software\\Classes\\"DISTRIBUTION"_BSPFile."ASSOCV"\\DefaultIcon", "", REG_SZ, command, strlen(command));
	Q_snprintfz(command, sizeof(command), "\"%s\" \"%%1\"", com_argv[0]);
	ok = ok & MyRegSetValue(root, "Software\\Classes\\"DISTRIBUTION"_BSPFile."ASSOCV"\\shell\\open\\command", "", REG_SZ, command, strlen(command));
	if (ok)
	{	//and now the extensions themselves...
		MyRegSetValue(root, "Software\\Classes\\.bsp", "", REG_SZ, DISTRIBUTION"_BSPFile."ASSOCV, strlen(DISTRIBUTION"_BSPFile."ASSOCV));
		MyRegSetValue(root, "Software\\Classes\\.map", "", REG_SZ, DISTRIBUTION"_BSPFile."ASSOCV, strlen(DISTRIBUTION"_BSPFile."ASSOCV));
	}

	//register the basic uri scheme class
	Q_snprintfz(command, sizeof(command), "QuakeWorld Server");
	ok = ok & MyRegSetValue(root, "Software\\Classes\\"DISTRIBUTION"_Server."ASSOCV"", "", REG_SZ, command, strlen(command));
	ok = ok & MyRegSetValue(root, "Software\\Classes\\"DISTRIBUTION"_Server."ASSOCV"", "URL Protocol", REG_SZ, "", strlen(""));
	Q_snprintfz(command, sizeof(command), "\"%s\",0", com_argv[0]);
	ok = ok & MyRegSetValue(root, "Software\\Classes\\"DISTRIBUTION"_Server."ASSOCV"\\DefaultIcon", "", REG_SZ, command, strlen(command));
	Q_snprintfz(command, sizeof(command), "\"%s\" \"%%1\"", com_argv[0]);
	ok = ok & MyRegSetValue(root, "Software\\Classes\\"DISTRIBUTION"_Server."ASSOCV"\\shell\\open\\command", "", REG_SZ, command, strlen(command));
	if (ok)
	{	//and now the schemes themselves... (doesn't really use the same scheme stuff)
		for (s = schemes; (s=COM_ParseOut(s, scheme, sizeof(scheme))); )
		{
			Q_snprintfz(command, sizeof(command), "QuakeWorld Server");
			MyRegSetValue(root, va("Software\\Classes\\%s", scheme), "", REG_SZ, command, strlen(command));
			MyRegSetValue(root, va("Software\\Classes\\%s", scheme), "URL Protocol", REG_SZ, "", strlen(""));
			Q_snprintfz(command, sizeof(command), "\"%s\",0", com_argv[0]);
			MyRegSetValue(root, va("Software\\Classes\\%s\\DefaultIcon", scheme), "", REG_SZ, command, strlen(command));
			Q_snprintfz(command, sizeof(command), "\"%s\" \"%%1\"", com_argv[0]);
			MyRegSetValue(root, va("Software\\Classes\\%s\\shell\\open\\command", scheme), "", REG_SZ, command, strlen(command));
		}
	}


	//now try to get ourselves listed in windows' 'default programs' ui.
	Q_snprintfz(command, sizeof(command), "%s", FULLENGINENAME);
	ok = ok & MyRegSetValue(root, "Software\\"FULLENGINENAME"\\Capabilities", "ApplicationName", REG_SZ, command, strlen(command));
	Q_snprintfz(command, sizeof(command), "%s", FULLENGINENAME" is an awesome hybrid game engine able to run multiple Quake-compatible/derived games.");
	ok = ok & MyRegSetValue(root, "Software\\"FULLENGINENAME"\\Capabilities", "ApplicationDescription", REG_SZ, command, strlen(command));

	Q_snprintfz(command, sizeof(command), DISTRIBUTION"_DemoFile.1");
	ok = ok & MyRegSetValue(root, "Software\\"FULLENGINENAME"\\Capabilities\\FileAssociations", ".qtv", REG_SZ, command, strlen(command));
	ok = ok & MyRegSetValue(root, "Software\\"FULLENGINENAME"\\Capabilities\\FileAssociations", ".mvd", REG_SZ, command, strlen(command));
	ok = ok & MyRegSetValue(root, "Software\\"FULLENGINENAME"\\Capabilities\\FileAssociations", ".qwd", REG_SZ, command, strlen(command));
	ok = ok & MyRegSetValue(root, "Software\\"FULLENGINENAME"\\Capabilities\\FileAssociations", ".dem", REG_SZ, command, strlen(command));
//	ok = ok & MyRegSetValue(root, "Software\\"FULLENGINENAME"\\Capabilities\\FileAssociations", ".dm2", REG_SZ, command, strlen(command));

	Q_snprintfz(command, sizeof(command), DISTRIBUTION"_BSPFile.1");
	ok = ok & MyRegSetValue(root, "Software\\"FULLENGINENAME"\\Capabilities\\FileAssociations", ".bsp", REG_SZ, command, strlen(command));
	ok = ok & MyRegSetValue(root, "Software\\"FULLENGINENAME"\\Capabilities\\FileAssociations", ".map", REG_SZ, command, strlen(command));

//	ok = ok & MyRegSetValue(root, "Software\\"FULLENGINENAME"\\Capabilities\\FileAssociations", ".fmf", REG_SZ, DISTRIBUTION"_ManifestFile", strlen(DISTRIBUTION"_ManifestFile"));
//	ok = ok & MyRegSetValue(root, "Software\\"FULLENGINENAME"\\Capabilities\\MIMEAssociations", "application/x-ftemanifest", REG_SZ, DISTRIBUTION"_ManifestFile", strlen(DISTRIBUTION"_ManifestFile"));

	Q_snprintfz(command, sizeof(command), DISTRIBUTION"_Server."ASSOCV);
	for (s = schemes; (s=COM_ParseOut(s, scheme, sizeof(scheme))); )
	{
		ok = ok & MyRegSetValue(root, "Software\\"FULLENGINENAME "\\Capabilities\\UrlAssociations", scheme, REG_SZ, command, strlen(command));
	}
	
	Q_snprintfz(command, sizeof(command), "Software\\"FULLENGINENAME"\\Capabilities");
	ok = ok & MyRegSetValue(root, "Software\\RegisteredApplications", FULLENGINENAME, REG_SZ, command, strlen(command));

	//also try to add it to current user.
	if (root==HKEY_LOCAL_MACHINE)
		Sys_DoFileAssociations(2, schemes);

	//let the shell know that file associations changed (otherwise we might have to wait for a reboot)
	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

	if (!ok && root==HKEY_LOCAL_MACHINE)
	{
		ShellExecute(mainwindow, "runas", com_argv[0], va("-register_types \"%s\"", schemes), NULL, SW_SHOWNORMAL);
		return;
	}

	if (ok && root==HKEY_LOCAL_MACHINE)
	{
		//attempt to display the vista+ prompt (only way possible in win8, apparently)
		//note that in win10 this will supposedly just show a notification popup with the user required to configure it manually via control panel.
		qIApplicationAssociationRegistrationUI *aarui = NULL;

		CoInitialize(NULL);
		if (FAILED(CoCreateInstance(&qCLSID_ApplicationAssociationRegistrationUI, 0, CLSCTX_INPROC_SERVER, &qIID_IApplicationAssociationRegistrationUI, (LPVOID*)&aarui)))
			aarui = NULL;

		if (aarui)
		{
#define wideify2(a) L##a
#define wideify(a) wideify2(a)
			aarui->lpVtbl->LaunchAdvancedAssociationUI(aarui, wideify(FULLENGINENAME));
			aarui->lpVtbl->Release(aarui);
		}
		else
		{

/*
#define wideify2(a) L##a
#define wideify(a) wideify2(a)
			qOPENASINFO open_as_info = {0};
			open_as_info.pcszFile = L".mvd";
			open_as_info.pcszClass = wideify(DISTRIBUTION)L"_DemoFile.1";
			open_as_info.oaifInFlags = 8 | 2;//OAIF_FORCE_REGISTRATION | OAIF_REGISTER_EXT;
			if (pSHOpenWithDialog)
				pSHOpenWithDialog(NULL, &open_as_info);
			SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
*/
		}
	}
}

/*
#ifdef _MSC_VER
#include <signal.h>
void VARGS Signal_Error_Handler(int i)
{
	int *basepointer;
	__asm {mov basepointer,ebp};
	Sys_Error("Received signal, offset was 0x%8x", basepointer[73]);
}
#endif
*/

extern char sys_language[64];

static int Sys_ProcessCommandline(char **argv, int maxargc, char *argv0)
{
	int argc = 0, i;
	wchar_t *wc = GetCommandLineW();
	unsigned char utf8cmdline[4096], *cl = utf8cmdline;
	narrowen(utf8cmdline, sizeof(utf8cmdline), wc);

//	argv[argc] = argv0;
//	argc++;

	while (*cl && (argc < maxargc))
	{
		while (*cl && *cl <= 32)
			cl++;

		if (*cl)
		{
			if (*cl == '\"')
			{
				cl++;

				argv[argc] = cl;
				argc++;

				while (*cl && *cl != '\"')
					cl++;
			}
			else
			{
				argv[argc] = cl;
				argc++;


				while (*cl && *cl > 32)
					cl++;
			}

			if (*cl)
			{
				*cl = 0;
				cl++;
			}
		}
	}
	if (argc < 1)
	{
		argv[0] = argv0;
		argc = 1;
	}
	for (i = 0; i < argc; i++)
		argv[i] = strdup(argv[i]);
	return i;
}

int MessageBoxU(HWND hWnd, char *lpText, char *lpCaption, UINT uType)
{
	wchar_t widecaption[256];
	wchar_t widetext[2048];
	widen(widetext, sizeof(widetext), lpText);
	widen(widecaption, sizeof(widecaption), lpCaption);
	return MessageBoxW(hWnd, widetext, widecaption, uType);
}

#ifdef WEBCLIENT
#ifndef GWLP_WNDPROC
#define GWLP_WNDPROC GWL_WNDPROC
#define SetWindowLongPtr SetWindowLong
#define LONG_PTR LONG
#endif
#ifndef BIF_NEWDIALOGSTYLE
#define BIF_NEWDIALOGSTYLE	0x00000040		//v5
#define BFFM_SETOKTEXT		(WM_USER + 105)	//v6
#define BFFM_SETEXPANDED	(WM_USER + 106)	//v6
#endif
static const IID qIID_IPersistFile	= {0x0000010BL, 0, 0, {0xc0,0,0,0,0,0,0,0x46}};

static WNDPROC omgwtfwhyohwhy;
static LRESULT CALLBACK stoopidstoopidstoopid(HWND w, UINT m, WPARAM wp, LPARAM lp)
{
	switch (m)
	{
	case WM_NOTIFY:
		switch (((LPNMHDR)lp)->code)
		{
		case TVN_ENDLABELEDITW:
			{
				LRESULT r;
				NMTVDISPINFOW *fu = (NMTVDISPINFOW*)lp;
				NMTREEVIEWW gah;
				gah.action = TVC_UNKNOWN;
				gah.itemOld = fu->item;
				gah.itemNew = fu->item;
				gah.ptDrag.x = gah.ptDrag.y = 0;
				gah.hdr = fu->hdr;
				gah.hdr.code = TVN_SELCHANGEDW;
				r = CallWindowProcW(omgwtfwhyohwhy,w,m,wp,lp);
				CallWindowProcW(omgwtfwhyohwhy,w,WM_NOTIFY,wp,(LPARAM)&gah);
				return r;
			}
			break;
		case TVN_ENDLABELEDITA:
			{
				LRESULT r;
				NMTVDISPINFOA *fu = (NMTVDISPINFOA*)lp;
				NMTREEVIEWA gah;
				gah.action = TVC_UNKNOWN;
				gah.itemOld = fu->item;
				gah.itemNew = fu->item;
				gah.ptDrag.x = gah.ptDrag.y = 0;
				gah.hdr = fu->hdr;
				gah.hdr.code = TVN_SELCHANGEDA;
				r = CallWindowProcW(omgwtfwhyohwhy,w,m,wp,lp);
				CallWindowProcW(omgwtfwhyohwhy,w,WM_NOTIFY,wp,(LPARAM)&gah);
				return r;
			}
			break;
		case TVN_SELCHANGEDA:
		case TVN_SELCHANGEDW:
			break;
		}
		break;
	}
	return CallWindowProcW(omgwtfwhyohwhy,w,m,wp,lp);
}

struct egadsthisisretarded
{
	wchar_t title[MAX_OSPATH];
	char subdir[MAX_OSPATH];
	char parentdir[MAX_OSPATH];
	char statustext[MAX_OSPATH];
};

void FS_Directorize(char *fname, size_t fnamesize)
{
	size_t l = strlen(fname);
	if (!l)	//technically already a directory
		return;
	if (fname[l-1] == '\\' || fname[l-1] == '/')
		return;	//already a directory
	Q_strncatz(fname, "/", fnamesize);
}

static INT CALLBACK StupidBrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lp, LPARAM pDatafoo) 
{	//'stolen' from microsoft's knowledge base.
	//required to work around microsoft being annoying.
	struct egadsthisisretarded *pData = (struct egadsthisisretarded*)pDatafoo;
//	char *foo;
	HWND edit = FindWindowEx(hwnd, NULL, "EDIT", NULL);
	HWND list;
//	OutputDebugString(va("got %u (%u)\n", uMsg, lp));
	switch(uMsg)
	{
	case BFFM_INITIALIZED:
		//combat windows putting new windows behind everything else if it takes a while for UAC prompts to go away
		SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);

		//combat windows bug where renaming something doesn't update the dialog's path
		list = FindWindowExW(hwnd, NULL, L"SHBROWSEFORFOLDER SHELLNAMESPACE CONTROL", NULL);
		if (list)
			omgwtfwhyohwhy = (WNDPROC)SetWindowLongPtr(list, GWLP_WNDPROC, (LONG_PTR)stoopidstoopidstoopid);

		{
			wchar_t szDir[MAX_PATH];
#ifndef _DEBUG
			extern qboolean	com_homepathenabled;
			//the standard location iiuc
			if (com_homepathenabled && pSHGetSpecialFolderPathW(NULL, szDir, CSIDL_PROGRAM_FILES, TRUE) && microsoft_accessW(szDir, ACCESS_READ | ACCESS_WRITE))
				;
			else if (microsoft_accessU("C:\\Games\\", ACCESS_READ | ACCESS_WRITE))
				widen(szDir, sizeof(szDir), "C:\\Games\\");
			else if (microsoft_accessU("C:\\", ACCESS_READ | ACCESS_WRITE))
				widen(szDir, sizeof(szDir), "C:\\");
			//if we're not an admin, install it somewhere else.
			else if (pSHGetSpecialFolderPathW(NULL, szDir, CSIDL_LOCAL_APPDATA, TRUE) && microsoft_accessW(szDir, ACCESS_READ | ACCESS_WRITE))
				;
			else
#endif
				if (GetCurrentDirectoryW(countof(szDir), szDir))// && microsoft_access(szDir, ACCESS_READ | ACCESS_WRITE))
					;
			SendMessageW(hwnd, BFFM_SETSELECTIONW, TRUE, (LPARAM)szDir);
			SendMessageW(hwnd, BFFM_SETEXPANDED, TRUE, (LPARAM)szDir);
			SendMessageW(hwnd, BFFM_SETOKTEXT, TRUE, (LPARAM)widen(szDir, sizeof(szDir), "Install"));
		}
		break;
	case BFFM_VALIDATEFAILEDA:
	case BFFM_VALIDATEFAILEDW:
		if (!microsoft_accessU(pData->parentdir, ACCESS_READ | ACCESS_WRITE))
		{
			MessageBoxU(hwnd, va("%s is not writable.", pData->parentdir), fs_gamename.string, 0);
			return 1;
		}
		if (edit)
		{
			wchar_t wide[256];
			GetWindowTextW(edit, wide, countof(wide));
			narrowen(pData->subdir, sizeof(pData->subdir), wide);
		}

		if (microsoft_accessU(va("%s%s", pData->parentdir, pData->subdir), ACCESS_READ))
			return MessageBoxU(hwnd, va("%s%s already exists!\nThis installer will (generally) not overwrite existing data files.\nIf you want to re-install, you must manually uninstall it first.\n\nContinue?", pData->parentdir, pData->subdir), fs_gamename.string, MB_ICONWARNING|MB_OKCANCEL|MB_TOPMOST) == IDCANCEL;
		else
			return MessageBoxU(hwnd, va("Install to %s%s ?", pData->parentdir, pData->subdir), fs_gamename.string, MB_OKCANCEL) == IDCANCEL;
	case BFFM_SELCHANGED: 
		{
			wchar_t wide[MAX_PATH*2+2];
			char *foo;
			if (pSHGetPathFromIDListW((LPITEMIDLIST)lp, wide))
			{
				narrowen(pData->parentdir, sizeof(pData->parentdir), wide);
				FS_Directorize(pData->parentdir, sizeof(pData->parentdir));

				//this'll make microsoft happy.
				while((foo = strchr(pData->parentdir, '/')))
					*foo = '\\';

				if (edit)
				{
					wchar_t wide[128];
					SetWindowTextW(edit, widen(wide, sizeof(wide), fs_gamename.string));
					SendMessageW(hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)widen(wide, sizeof(wide), va("%s", pData->parentdir)));
				}
				else
					SendMessageW(hwnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)widen(wide, sizeof(wide), va("%s%s", pData->parentdir, fs_gamename.string)));
			}
		}
		break;
//	case BFFM_IUNKNOWN:
//		break;
	}
	return 0;
}

LRESULT CALLBACK NoCloseWindowProc(HWND w, UINT m, WPARAM wp, LPARAM lp)
{
	if (m == WM_CLOSE)
		return 0;
	return DefWindowProc(w, m, wp, lp);
}

BOOL CopyFileU(const char *src, const char *dst, BOOL bFailIfExists)
{
	wchar_t wide1[2048];
	wchar_t wide2[2048];
	return CopyFileW(widen(wide1, sizeof(wide1), src), widen(wide2, sizeof(wide2), dst), bFailIfExists);
}

#ifdef WEBCLIENT
static qboolean Sys_DoInstall(void)
{
	extern ftemanifest_t *fs_manifest;
	char exepath[MAX_OSPATH];
	char newexepath[MAX_OSPATH];
	wchar_t wide[MAX_PATH];
	wchar_t wide2[MAX_PATH];
	char resultpath[MAX_OSPATH];
	BROWSEINFOW bi;
	LPITEMIDLIST il;
	struct egadsthisisretarded diediedie;

	if (!pSHGetSpecialFolderPathW)
	{
		MessageBoxU(NULL, "SHGetSpecialFolderPathW is not supported\n", fs_gamename.string, 0);
		return TRUE;
	}

	if (fs_manifest && fs_manifest->eula)
	{
		if (MessageBoxU(NULL, fs_manifest->eula, fs_gamename.string, MB_OKCANCEL|MB_TOPMOST|MB_DEFBUTTON2) != IDOK)
			return TRUE;
	}

	Q_strncpyz(diediedie.subdir, fs_gamename.string, sizeof(diediedie.subdir));
	_snwprintf(diediedie.title, countof(diediedie.title), L"Where would you like to install %s to?", widen(wide, sizeof(wide), fs_gamename.string));
	GetCurrentDirectoryW(countof(wide)-1, wide);

	memset(&bi, 0, sizeof(bi));
	bi.hwndOwner = mainwindow; //note that this is usually still null
	bi.pidlRoot = NULL;
	bi.pszDisplayName = wide;
	bi.lpszTitle = diediedie.title;
	bi.ulFlags = BIF_RETURNONLYFSDIRS|BIF_STATUSTEXT | BIF_EDITBOX|BIF_NEWDIALOGSTYLE|BIF_VALIDATE;
	bi.lpfn = StupidBrowseCallbackProc;
	bi.lParam = (LPARAM)&diediedie;
	bi.iImage = 0;

	il = pSHBrowseForFolderW?pSHBrowseForFolderW(&bi):NULL;
	if (il)
	{
		pSHGetPathFromIDListW(il, wide);
		CoTaskMemFree(il);
	}
	else
		return true;
	narrowen(resultpath, sizeof(resultpath), wide);

	FS_Directorize(resultpath, sizeof(resultpath));
	if (*diediedie.subdir)
	{
		Q_strncatz(resultpath, diediedie.subdir, sizeof(resultpath));
		FS_Directorize(resultpath, sizeof(resultpath));
	}

	FS_CreateBasedir(resultpath);

	GetModuleFileNameW(NULL, wide, countof(wide));
	narrowen(exepath, sizeof(exepath), wide);
	FS_SystemPath(va("%s.exe", fs_gamename.string), FS_ROOT, newexepath, sizeof(newexepath));
	CopyFileU(exepath, newexepath, FALSE);

	/*the game can now be run (using regular autoupdate stuff), but most installers are expected to install the data instead of just more downloaders, so lets do that with a 'nice' progress box*/
	{
		HINSTANCE hInstance = NULL;
		HWND progress, label, wnd;
		WNDCLASS wc;
		RECT ca;
		int sh;
		int pct = -100;
		char fname[MAX_OSPATH];
		memset(&wc, 0, sizeof(wc));
		wc.style = 0;
		wc.lpfnWndProc		= NoCloseWindowProc;//Progress_Wnd;
		wc.hInstance		= hInstance;
		wc.hCursor			= LoadCursor (NULL,IDC_ARROW);
		wc.hbrBackground	= (void *)COLOR_WINDOW;
		wc.lpszClassName	= "FTEPROG";
		RegisterClass(&wc);

		ca.right = GetSystemMetrics(SM_CXSCREEN);
		ca.bottom = GetSystemMetrics(SM_CYSCREEN);

		mainwindow = wnd = CreateWindowEx(0, wc.lpszClassName, va("%s Installer",  fs_gamename.string), 0, (ca.right-320)/2, (ca.bottom-100)/2, 320, 100, NULL, NULL, hInstance, NULL);

		GetClientRect(wnd, &ca); 
		sh = GetSystemMetrics(SM_CYVSCROLL);

		Sys_LoadLibrary("comctl32.dll", NULL);
//		InitCommonControls();

		label = CreateWindow("STATIC","", WS_CHILD | WS_VISIBLE | SS_PATHELLIPSIS, sh, ((ca.bottom-ca.top-sh)/3), ca.right-ca.left-2*sh, sh, wnd, NULL, hInstance, NULL);
		progress = CreateWindowEx(0, PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE | PBS_SMOOTH, sh, ((ca.bottom-ca.top-sh)/3)*2, ca.right-ca.left-2*sh, sh, wnd, NULL, hInstance, NULL);

		ShowWindow(wnd, SW_NORMAL);
		SetWindowPos(wnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE);

		SendMessage(progress, PBM_SETRANGE32, 0, 10000);
		*fname = 0;
		HTTP_CL_Think(NULL, NULL);
		while(FS_DownloadingPackage())
		{
			MSG msg;
			char *cur = cls.download?COM_SkipPath(cls.download->localname):"Please Wait";
			int newpct = cls.download?cls.download->percent*100:0;

			if (cls.download && cls.download->sizeunknown)
			{
				//marquee needs manifest bollocks in order to work. so lets just not bother.
				float time = Sys_DoubleTime();
				newpct = 10000 * (time - (int)time);
				if ((int)time & 1)
					newpct = 10000 - newpct;
			}

			if (Q_strcmp(fname, cur))
			{
				Q_strncpyz(fname, cur, sizeof(fname));
				SetWindowText(label, fname);
			}
			if (pct != newpct)
			{
				SendMessage(progress, PBM_SETPOS, pct, 0);
				pct = newpct;
			}

			while (PeekMessage (&msg, NULL, 0, 0, PM_REMOVE))
				DispatchMessage (&msg);

			Sleep(10);
			HTTP_CL_Think(NULL, NULL);
		}
		DestroyWindow(progress);
		DestroyWindow(wnd);
		UnregisterClass("FTEPROG", hInstance);
		mainwindow = NULL;
	}

	/*create startmenu icon*/
	if (MessageBoxU(NULL, va("Create start-menu icon for %s?", fs_gamename.string), fs_gamename.string, MB_YESNO|MB_ICONQUESTION|MB_TOPMOST) == IDYES)
	{
		HRESULT hres;
		IShellLinkW *psl;
		hres = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, &qIID_IShellLinkW, (LPVOID*)&psl);
		if (SUCCEEDED(hres))
		{
			char startmenu[MAX_OSPATH];
			WCHAR wsz[MAX_PATH];
			IPersistFile *ppf;
			widen(wsz, sizeof(wsz), newexepath);
			psl->lpVtbl->SetPath(psl, wsz);
			widen(wsz, sizeof(wsz), resultpath);
			psl->lpVtbl->SetWorkingDirectory(psl, wsz);
			hres = psl->lpVtbl->QueryInterface(psl, &qIID_IPersistFile, (LPVOID*)&ppf);
			if (SUCCEEDED(hres) && pSHGetSpecialFolderPathW(NULL, wsz, CSIDL_COMMON_PROGRAMS, TRUE))
			{
				WCHAR wsz[MAX_PATH];
				narrowen(startmenu, sizeof(startmenu), wsz);
				widen(wsz, sizeof(wsz), va("%s/%s.lnk", startmenu, fs_gamename.string));
				hres = ppf->lpVtbl->Save(ppf, wsz, TRUE);
				if (hres == E_ACCESSDENIED && pSHGetSpecialFolderPathW(NULL, wsz, CSIDL_PROGRAMS, TRUE))
				{
					narrowen(startmenu, sizeof(startmenu), wsz);
					widen(wsz, sizeof(wsz), va("%s/%s.lnk", startmenu, fs_gamename.string));
					hres = ppf->lpVtbl->Save(ppf, wsz, TRUE);
				}
				ppf->lpVtbl->Release(ppf);
			}
			psl->lpVtbl->Release(psl);
		}
	}

	//now start it up properly.
	ShellExecuteW(mainwindow, L"open", widen(wide, sizeof(wide), newexepath), Q_strcasecmp(fs_manifest->installation, "quake")?L"":L"+sys_register_file_associations", widen(wide2, sizeof(wide2), resultpath), SW_SHOWNORMAL);
	return true;
}
qboolean Sys_RunInstaller(void)
{
	HINSTANCE ch;
	char exepath[MAX_OSPATH];
	if (COM_CheckParm("-doinstall"))
		return Sys_DoInstall();
	if (!com_installer)
		return false;
	if (MessageBoxU(NULL, va("%s is not installed. Install now?", fs_gamename.string), fs_gamename.string, MB_OKCANCEL|MB_ICONQUESTION|MB_TOPMOST) == IDOK)
	{
		GetModuleFileName(NULL, exepath, sizeof(exepath));
		ch = ShellExecute(mainwindow, "runas", com_argv[0], va("%s -doinstall", COM_Parse(GetCommandLine())), NULL, SW_SHOWNORMAL);
		if ((qintptr_t)ch > 32)
			return true;	//succeeded. should quit out.
		return Sys_DoInstall();	//if it failed, try doing it with the current privileges
	}
	return true;
}
#endif

#define RESLANG MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_UK)
static const char *Sys_FindManifest(void)
{
	const char *fmf;
	HRSRC hdl = FindResource(NULL, MAKEINTRESOURCE(1), RT_RCDATA);
	HGLOBAL hgl = LoadResource(NULL, hdl);
	fmf = LockResource(hgl);
//	MessageBox(NULL, fmf, "Embedded manifest", 0);
	return fmf;
}

//size info that microsoft recommends
static const struct
{
	int width;
	int height;
	int bpp;
} icosizes[] = {
//	{96, 96, 32},
//	{48, 48, 32},
//	{32, 32, 32},
//	{16, 16, 32},
//	{16, 16, 4},
//	{48, 48, 4},
//	{32, 32, 4},
//	{16, 16, 1},
//	{48, 48, 1},
//	{32, 32, 1},
	{256, 256, 32}	//vista!
};
//dates back to 16bit windows. bah.
#pragma pack(push)
#pragma pack(2)
typedef struct
{
	WORD idReserved;
	WORD idType;
	WORD idCount;
	struct
	{
		BYTE  bWidth;
		BYTE  bHeight;
		BYTE  bColorCount;
		BYTE  bReserved;
		WORD  wPlanes;
		WORD  wBitCount;
		DWORD dwBytesInRes;
		WORD  nId;
	} idEntries[sizeof(icosizes)/sizeof(icosizes[0])];
} icon_group_t;
#pragma pack(pop)

static void Sys_MakeInstaller(const char *name)
{
	vfsfile_t *filehandle;
	qbyte *filedata;
	unsigned int filelen;
	char *error = NULL;
	char *warn = NULL;
	HANDLE bin;
	char ourname[MAX_OSPATH];
	char newname[MAX_OSPATH];
	char tmpname[MAX_OSPATH];

	Q_snprintfz(newname, sizeof(newname), "%s.exe", name);
	Q_snprintfz(tmpname, sizeof(tmpname), "tmp.exe");

	GetModuleFileName(NULL, ourname, sizeof(ourname));

	if (!CopyFile(ourname, tmpname, FALSE))
		error = va("\"%s\" already exists or cannot be written", tmpname);

	if (!(bin = BeginUpdateResource(tmpname, FALSE)))
		error = "BeginUpdateResource failed";
	else
	{
		filehandle = VFSOS_Open(va("%s.png", name), "rb");
		if (filehandle)
		{
			icon_group_t icondata;
			qbyte *rgbadata;
			int imgwidth, imgheight;
			int iconid = 1;
			uploadfmt_t format;
			memset(&icondata, 0, sizeof(icondata));
			icondata.idType = 1;
			filelen = VFS_GETLEN(filehandle);
			filedata = BZ_Malloc(filelen);
			VFS_READ(filehandle, filedata, filelen);
			VFS_CLOSE(filehandle);

			//nuke existing icons.
			UpdateResource(bin, RT_GROUP_ICON, MAKEINTRESOURCE(1), RESLANG, NULL, 0);
			UpdateResource(bin, RT_GROUP_ICON, MAKEINTRESOURCE(2), RESLANG, NULL, 0);
//			UpdateResource(bin, RT_GROUP_ICON, MAKEINTRESOURCE(3), RESLANG, NULL, 0);

			rgbadata = ReadRawImageFile(filedata, filelen, &imgwidth, &imgheight, &format, true, va("%s.png", name));
			if (!rgbadata)
				error = "unable to read icon image";
			else
			{
				void *data = NULL;
				unsigned int datalen = 0;
				unsigned int i;
				extern cvar_t gl_lerpimages;
				gl_lerpimages.ival = 1;
				for (i = 0; i < sizeof(icosizes)/sizeof(icosizes[0]); i++)
				{
					unsigned int x,y;
					if (icosizes[i].width > imgwidth || icosizes[i].height > imgheight)
						continue;	//ignore icons if they're bigger than the original icon.

					if (icosizes[i].bpp == 32 && icosizes[i].width >= 128 && icosizes[i].height >= 128 && icosizes[i].width == imgwidth && icosizes[i].height == imgheight)
					{	//png compression. oh look. we originally loaded a png!
						data = filedata;
						datalen = filelen;
					}
					else
					{
						//generate the bitmap info
						BITMAPV4HEADER *bi;
						qbyte *out, *outmask;
						qbyte *in, *inrow;
						unsigned int outidx;

						bi = data = Z_Malloc(sizeof(*bi) + icosizes[i].width * icosizes[i].height * 5 + icosizes[i].height*4);
						memset(bi,0, sizeof(BITMAPINFOHEADER));
						bi->bV4Size				= sizeof(BITMAPINFOHEADER);
						bi->bV4Width			= icosizes[i].width;
						bi->bV4Height			= icosizes[i].height * 2;
						bi->bV4Planes			= 1;
						bi->bV4BitCount			= icosizes[i].bpp;
						bi->bV4V4Compression	= BI_RGB;
						bi->bV4ClrUsed			= (icosizes[i].bpp>=32?0:(1u<<icosizes[i].bpp));

						datalen = bi->bV4Size;
						out = (qbyte*)data + datalen;
						datalen += ((icosizes[i].width*icosizes[i].bpp/8+3)&~3) * icosizes[i].height;
						outmask = (qbyte*)data + datalen;
						datalen += ((icosizes[i].width+31)&~31)/8 * icosizes[i].height;

						in = Image_ResampleTexture(format, rgbadata, imgwidth, imgheight, NULL, icosizes[i].width, icosizes[i].height);

						inrow = in;
						outidx = 0;
						if (icosizes[i].bpp == 32)
						{
							for (y = 0; y < icosizes[i].height; y++)
							{
								inrow = in + 4*icosizes[i].width*(icosizes[i].height-1-y);
								for (x = 0; x < icosizes[i].width; x++)
								{
									if (inrow[3] == 0)	//transparent
										outmask[outidx>>3] |= 1u<<(outidx&7);
									else
									{
										out[0] = inrow[2];
										out[1] = inrow[1];
										out[2] = inrow[0];
									}
									out += 4;
									outidx++;
									inrow += 4;
								}
								if (x & 3)
									out += 4 - (x&3);
								outidx = (outidx + 31)&~31;
							}
						}
					}

					if (!error && !UpdateResource(bin, RT_ICON, MAKEINTRESOURCE(iconid), 0, data, datalen))
						error = "UpdateResource failed (icon data)";

					//and make a copy of it in the icon list
					icondata.idEntries[icondata.idCount].bWidth = (icosizes[i].width<256)?icosizes[i].width:0;
					icondata.idEntries[icondata.idCount].bHeight = (icosizes[i].height<256)?icosizes[i].height:0;
					icondata.idEntries[icondata.idCount].wBitCount = icosizes[i].bpp;
					icondata.idEntries[icondata.idCount].wPlanes = 1;
					icondata.idEntries[icondata.idCount].bColorCount = (icosizes[i].bpp>=8)?0:(1u<<icosizes[i].bpp);
					icondata.idEntries[icondata.idCount].dwBytesInRes = datalen;
					icondata.idEntries[icondata.idCount].nId = iconid++;
					icondata.idCount++;
				}
			}

			if (!error && !UpdateResource(bin, RT_GROUP_ICON, MAKEINTRESOURCE(IDI_ICON1), RESLANG, &icondata, (qbyte*)&icondata.idEntries[icondata.idCount] - (qbyte*)&icondata))
				error = "UpdateResource failed (icon group)";
			BZ_Free(filedata);
		}
		else
			warn = va("%s.png not found", name);

		filehandle = VFSOS_Open(va("%s.fmf", name), "rb");
		if (filehandle)
		{
			filelen = VFS_GETLEN(filehandle);
			filedata = BZ_Malloc(filelen+1);
			filedata[filelen] = 0;
			VFS_READ(filehandle, filedata, filelen);
			VFS_CLOSE(filehandle);
			if (!error && !UpdateResource(bin, RT_RCDATA, MAKEINTRESOURCE(1), 0, filedata, filelen+1))
				error = "UpdateResource failed (manicfest)";
			BZ_Free(filedata);
		}
		else
			error = va("%s.fmf not found in working directory", name);

		if (!EndUpdateResource(bin, !!error) && !error)
			error = "EndUpdateResource failed. Check access permissions.";

		DeleteFile(newname);
		MoveFile(tmpname, newname);
	}

	if (!error)
		error = warn;

	if (error)
	{
		Sys_Printf("%s", error);
		Sys_Error("%s", error);
	}
}
#endif

#if defined(_MSC_VER) && defined(_AMD64_)
#pragma optimize( "", off)	//64bit msvc sucks and falls over when trying to inline Host_Frame
#endif
#ifdef __GNUC__
__attribute__((visibility("default")))
#endif
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
//    MSG				msg;
	quakeparms_t	parms;
	double			time, oldtime, newtime;
	char	cwd[1024], bindir[1024];
	const char *qtvfile = NULL;
	char lang[32];
	char ctry[32];
	int c;

	/* previous instances do not exist in Win32 */
	if (hPrevInstance)
		return EXIT_FAILURE;

	/* determine if we're on nt early, so we don't do the wrong thing when checking commandlines */
	{
		OSVERSIONINFOA vinfo;
		vinfo.dwOSVersionInfoSize = sizeof(vinfo);
		if (GetVersionExA (&vinfo))
			WinNT = vinfo.dwPlatformId == VER_PLATFORM_WIN32_NT;
	}

#if defined(_DEBUG) && defined(_MSC_VER) && defined(MULTITHREAD)
	Sys_SetThreadName(-1, "main thread");
#endif

	memset(&parms, 0, sizeof(parms));

	/*work around potentially serious windows flaw loading dlls from the current directory, by loading a dll...*/
	{
		BOOL (WINAPI *pSetDllDirectoryW)(LPCWSTR lpPathName);
		dllfunction_t ffsfuncs[] = {{(void*)&pSetDllDirectoryW, "SetDllDirectoryW"}, {NULL,NULL}};
		if (Sys_LoadLibrary("kernel32.dll", ffsfuncs))
			pSetDllDirectoryW(L"");	//disables it (null for 'use working directory')
	}

	Win7_Init();

#ifdef _MSC_VER
#if _M_IX86_FP >= 1
	{
		int idedx;
		char cpuname[13];
		/*I'm not going to check to make sure cpuid works.*/
		__asm
		{
			xor eax, eax
			cpuid
			mov dword ptr [cpuname+0],ebx
			mov dword ptr [cpuname+4],edx
			mov dword ptr [cpuname+8],ecx
		}
		cpuname[12] = 0;
		__asm
		{
			mov eax, 0x1
			cpuid
			mov idedx, edx
		}
#if _M_IX86_FP >= 2
		if (!(idedx&(1<<26)))
			MessageBox(NULL, "This is an SSE2 optimised build, and your cpu doesn't seem to support it", DISTRIBUTION, 0);
		else
#endif
			  if (!(idedx&(1<<25)))
				MessageBox(NULL, "This is an SSE optimised build, and your cpu doesn't seem to support it", DISTRIBUTION, 0);
	}
#endif
#endif

#ifdef CATCHCRASH
	LoadLibraryU ("DBGHELP");	//heap corruption can prevent loadlibrary from working properly, so do this in advance.
#ifdef MSVC_SEH
	__try
#else
	{
		PVOID (WINAPI *pAddVectoredExceptionHandler)(ULONG	FirstHandler,	PVECTORED_EXCEPTION_HANDLER VectoredHandler);
		dllfunction_t dbgfuncs[] = {{(void*)&pAddVectoredExceptionHandler, "AddVectoredExceptionHandler"}, {NULL,NULL}};
		if (Sys_LoadLibrary("kernel32.dll", dbgfuncs) && pAddVectoredExceptionHandler)
			pAddVectoredExceptionHandler(0, nonmsvc_CrashExceptionHandler);
	}
#endif
#endif
	{
/*
#ifndef _DEBUG
#ifdef _MSC_VER
		signal (SIGFPE,	Signal_Error_Handler);
		signal (SIGILL,	Signal_Error_Handler);
		signal (SIGSEGV,	Signal_Error_Handler);
#endif
#endif
*/
		global_hInstance = hInstance;
		global_nCmdShow = nCmdShow;

#ifdef RESTARTTEST
		setjmp (restart_jmpbuf);
#endif

		if (WinNT)
		{
			wchar_t widebindir[1024];
			GetModuleFileNameW(NULL, widebindir, sizeof(widebindir)/sizeof(widebindir[0])-1);
			narrowen(bindir, sizeof(bindir)-1, widebindir);
		}
		else
			GetModuleFileNameA(NULL, bindir, sizeof(bindir)-1);
		parms.argc = Sys_ProcessCommandline(sys_argv, MAX_NUM_ARGVS, bindir);
		*COM_SkipPath(bindir) = 0;
		parms.argv = (const char **)sys_argv;

		parms.binarydir = bindir;
		COM_InitArgv (parms.argc, parms.argv);

#ifdef PLUGINS
		c = COM_CheckParm("--plugwrapper");
		if (c)
		{
			int (QDECL *thefunc) (void);
			dllhandle_t *lib;
			host_parms = parms;//not really initialising, but the filesystem needs it
			lib = Sys_LoadLibrary(com_argv[c+1], NULL);
			if (lib)
			{
				thefunc = Sys_GetAddressForName(lib, com_argv[c+2]);
				if (thefunc)
					return thefunc();
			}
			MessageBox(NULL, "Unable to start up plugin wrapper", FULLENGINENAME, 0);
			return EXIT_FAILURE;
		}
#endif

		c = COM_CheckParm("-qcdebug");
		if (c)
			isPlugin = 3;
		else
		{
			c = COM_CheckParm("-plugin");
			if (c)
			{
				if (c < com_argc && !strcmp(com_argv[c+1], "qcdebug"))
					isPlugin = 2;
				else
					isPlugin = 1;
			}
			else
				isPlugin = 0;
		}

		c = COM_CheckParm("-register_types");
		if (c)
		{
			Sys_DoFileAssociations(1, (c+1 < com_argc)?com_argv[c+1]:NULL);
			return EXIT_SUCCESS;
		}
		/*
		else if (!isPlugin)
		{
			if (MyRegGetIntValue(HKEY_CURRENT_USER, "Software\\"FULLENGINENAME, "filetypes", -1) != ASSOC_VERSION)
			{
				DWORD dw = ASSOC_VERSION;
				if (IDYES == MessageBox(NULL, "Register file associations?", "FTE First Start", MB_YESNO))
					Sys_DoFileAssociations(0);
				MyRegSetValue(HKEY_CURRENT_USER, "Software\\"FULLENGINENAME, "filetypes", REG_DWORD, &dw, sizeof(dw)); 
			}
		}
		*/

#if defined(CATCHCRASH) && defined(MULTITHREAD)
#ifdef _MSC_VER
		if (COM_CheckParm("-watchdog"))
			watchdogthread = Sys_CreateThread("watchdog", watchdogthreadfunction, NULL, 0, 0); 
#endif
#endif

		if (isPlugin==1)
		{
			printf("status Starting up!\n");
			fflush(stdout);
		}

		if (COM_CheckParm("--version") || COM_CheckParm("-v"))
		{
			printf("version: %s\n", version_string());
			return EXIT_SUCCESS;
		}
		if (COM_CheckParm("-outputdebugstring"))
			debugout = true;

		if (WinNT)
		{
			wchar_t wcwd[MAX_OSPATH];
			if (!GetCurrentDirectoryW (sizeof(wcwd)/sizeof(wchar_t), wcwd))
				Sys_Error ("Couldn't determine current directory");
			narrowen(cwd, sizeof(cwd), wcwd);
		}
		else
		{
			if (!GetCurrentDirectoryA (sizeof(cwd), cwd))
				Sys_Error ("Couldn't determine current directory");
		}

#ifdef WEBCLIENT
		c = COM_CheckParm("-makeinstaller");
		if (c)
		{
			Sys_MakeInstaller(parms.argv[c+1]);
			return EXIT_SUCCESS;
		}
		parms.manifest = Sys_FindManifest();
#endif

		if (parms.argc >= 2)
		{
			if (*parms.argv[1] != '-' && *parms.argv[1] != '+')
			{
				char *e;

				if (parms.argc == 2 && !strchr(parms.argv[1], '\"') && !strchr(parms.argv[1], ';') && !strchr(parms.argv[1], '\n') && !strchr(parms.argv[1], '\r'))
				{
					HWND old;
					qtvfile = parms.argv[1];

					old = FindWindowW(L"FTEGLQuake", NULL);
					if (!old)
						old = FindWindowW(L"FTED3D11QUAKE", NULL);
					if (!old)
						old = FindWindowW(L"FTED3D9QUAKE", NULL);
					if (!old)
						old = FindWindowW(L"FTED3D8QUAKE", NULL);
					if (old)
					{
						COPYDATASTRUCT cds;
						cds.dwData = 0xdeadf11eu;
						cds.cbData = strlen(qtvfile);
						cds.lpData = (void*)qtvfile;
						if (SendMessage(old, WM_COPYDATA, (WPARAM)GetDesktopWindow(), (LPARAM)&cds))
						{
							Sleep(10*1000);	//sleep for 10 secs so the real engine has a chance to open it, if the program that gave it is watching to see if we quit.
							return EXIT_SUCCESS;	//message sent.
						}
					}
				}
				else
				{
					MessageBox(NULL, va("Invalid commandline:\n%s", lpCmdLine), FULLENGINENAME, 0);
					return EXIT_FAILURE;
				}

				GetModuleFileName(NULL, cwd, sizeof(cwd)-1);
				for (e = cwd+strlen(cwd)-1; e >= cwd; e--)
				{
					if (*e == '/' || *e == '\\')
					{
						*e = 0;
						break;
					}
				}
			}
		}

		//98+/nt4+
		if (GetLocaleInfo(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, lang, sizeof(lang)) > 0)
		{
			if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, ctry, sizeof(ctry)) > 0)
				Q_snprintfz(sys_language, sizeof(sys_language), "%s_%s", lang, ctry); 
			else
				Q_snprintfz(sys_language, sizeof(sys_language), "%s", lang); 
		}

		TL_InitLanguages(parms.binarydir);
		//tprints are now allowed

		if (*cwd && cwd[strlen(cwd)-1] != '\\' && cwd[strlen(cwd)-1] != '/')
			Q_strncatz(cwd, "/", sizeof(cwd));

		parms.basedir = cwd;

		parms.argc = com_argc;
		parms.argv = com_argv;

#if !defined(CLIENTONLY) && !defined(SERVERONLY)
		if (COM_CheckParm ("-dedicated"))
			isDedicated = true;
	#ifdef SUBSERVERS
		if (COM_CheckParm("-clusterslave"))
		{
			isDedicated = isClusterSlave = true;
#ifdef _DEBUG
			MessageBox(0, "New cluster slave starting\nAttach to process now, if desired.", "FTEQW Debug Build", 0);
#endif
			SSV_SetupControlPipe(Sys_GetStdInOutStream(), false);
		}
	#endif
#endif

		if (isDedicated)
		{
#if !defined(CLIENTONLY)
			if (!Sys_InitTerminal())
				Sys_Error ("Couldn't allocate dedicated server console");
#endif
		}

		if (!Sys_Startup_CheckMem(&parms))
			Sys_Error ("Not enough memory free; check disk space\n");

//		FS_ChangeGame(NULL, true, true);
//		if (Sys_CheckUpdated(bindir, sizeof(bindir)))
//			return true;


#ifndef CLIENTONLY
		if (isDedicated)	//compleate denial to switch to anything else - many of the client structures are not initialized.
		{
			float delay;

			SV_Init (&parms);

			delay = SV_Frame();

			while (1)
			{
				if (!isDedicated)
					Sys_Error("Dedicated was cleared");
				NET_Sleep(delay, false);
				delay = SV_Frame();
			}
			return EXIT_FAILURE;
		}
#endif

		tevent = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (!tevent)
			Sys_Error ("Couldn't create event");

#ifdef SERVERONLY
		Sys_Printf ("SV_Init\n");
		SV_Init(&parms);
#else
		Sys_Printf ("Host_Init\n");
		Host_Init (&parms);
#endif

		oldtime = Sys_DoubleTime ();

	//client console should now be initialized.

		#ifndef MINGW
		#if _MSC_VER > 1200
		Win7_TaskListInit();
		#endif
		#endif

		if (isPlugin==1)
		{
			printf("status Running!\n");
			fflush(stdout);
		}

		/* main window message loop */
		while (1)
		{
#ifdef CATCHCRASH
			watchdogframe++;
#endif
			if (isDedicated)
			{
	#ifndef CLIENTONLY
				float delay;

			// find time passed since last cycle
				newtime = Sys_DoubleTime ();
				time = newtime - oldtime;
				oldtime = newtime;

				delay = SV_Frame ();

				NET_Sleep(delay, false);
	#else
				Sys_Error("wut?");
	#endif
			}
			else
			{
	#ifndef SERVERONLY
				double sleeptime;
				newtime = Sys_DoubleTime ();
				time = newtime - oldtime;
				sleeptime = Host_Frame (time);
				oldtime = newtime;

				SetHookState(vid.activeapp);

				/*sleep if its not yet time for a frame*/
				if (sleeptime)
					Sys_Sleep(sleeptime);
	#else
				Sys_Error("wut?");
	#endif
			}
		}
	}
#ifdef CATCHCRASH
#ifdef MSVC_SEH
	__except (CrashExceptionHandler(false, GetExceptionCode(), GetExceptionInformation()))
	{
		return EXIT_FAILURE;
	}
#endif
#endif

	/* return success of application */
	return EXIT_FAILURE;
}
#ifdef _MSC_VER
#pragma optimize( "", on)	//revert back to default optimisations again.
#endif

#if 0	//define this if you're somehow getting windows' consle subsystem instead of the proper windows one
int __cdecl main(void)
{
	char *cmdline;
	FreeConsole();
	cmdline = GetCommandLine();
	while (*cmdline && *cmdline == ' ')
		cmdline++;
	if (*cmdline == '\"')
	{
		cmdline++;
		while (*cmdline && *cmdline != '\"')
			cmdline++;
		if (*cmdline == '\"')
			cmdline++;
	}
	else
	{
		while (*cmdline && *cmdline != ' ')
			cmdline++;
	}
	return WinMain(GetModuleHandle(NULL), NULL, cmdline, SW_NORMAL);
}
#endif 

//now queries at startup and then caches, to avoid mode changes from giving weird results.
qboolean Sys_GetDesktopParameters(int *width, int *height, int *bpp, int *refreshrate)
{
	*width = desktopsettings.width;
	*height = desktopsettings.height;
	*bpp = desktopsettings.bpp;
	*refreshrate = desktopsettings.rate;
	return true;
}

static void Sys_QueryDesktopParameters(void)
{
	HDC hdc;

	hdc = GetDC(NULL);

	desktopsettings.width = GetDeviceCaps(hdc, HORZRES);
	desktopsettings.height = GetDeviceCaps(hdc, VERTRES);
	desktopsettings.bpp = GetDeviceCaps(hdc, BITSPIXEL);
	desktopsettings.rate = GetDeviceCaps(hdc, VREFRESH);

	if (desktopsettings.rate == 1)
		desktopsettings.rate = 0;

	ReleaseDC(NULL, hdc);
}

void Sys_Sleep (double seconds)
{
	Sleep(seconds * 1000);
}




HCURSOR	hArrowCursor, hCustomCursor;
void *WIN_CreateCursor(const qbyte *imagedata, int width, int height, uploadfmt_t format, float hotx, float hoty, float scale)
{
	BITMAPV4HEADER bi;
	DWORD x,y;
	HCURSOR hAlphaCursor = NULL;
	ICONINFO ii;
	HDC maindc;

	const qbyte *rgbadata;
	qbyte *bgradata, *bgradata_start;
	void *scaled = NULL;
	if (!imagedata)
		return NULL;

	// FIXME: CreateIconIndirect does NOT understand DPI scaling, and will show a tiny cursor in such cases.
	// we should rescale scale by vid_conautoscale etc.
	if (scale != 1)
	{
		int nw,nh;
		qbyte *nd;
		nw = width * scale;
		nh = height * scale;
		if (nw <= 0 || nh <= 0 || nw > 128 || nh > 128)	//don't go crazy.
			return NULL;
		nd = Image_ResampleTexture(format, imagedata, width, height, NULL, nw, nh);
		width = nw;
		height = nh;
		imagedata = scaled = nd;
	}

	memset(&bi,0, sizeof(bi));
	bi.bV4Size			= sizeof(bi);
	bi.bV4Width			= width;
	bi.bV4Height		= height;
	bi.bV4Planes		= 1;
	bi.bV4BitCount		= 32;
	bi.bV4V4Compression	= BI_BITFIELDS;
	// The following mask specification specifies a supported 32 BPP
	// alpha format for Windows XP.
	//FIXME: can we not just specify it as RGBA? meh.
	bi.bV4RedMask		= 0x00FF0000;
	bi.bV4GreenMask		= 0x0000FF00;
	bi.bV4BlueMask		= 0x000000FF;
	bi.bV4AlphaMask		= 0xFF000000; 

	// Create the DIB section with an alpha channel.
	maindc = GetDC(mainwindow);
	ii.hbmColor = CreateDIBSection(maindc, (BITMAPINFO *)&bi, DIB_RGB_COLORS, (void **)&bgradata_start, NULL, 0);
	ReleaseDC(mainwindow, maindc);

	if (!ii.hbmColor)
	{
		BZ_Free(scaled);
		return NULL;
	}

	for (rgbadata=imagedata,y=0;y<height;y++)
	{
		bgradata = bgradata_start + (height-1-y)*width*4;
		for (x=0;x<width;x++)
		{
			bgradata[0] = rgbadata[2];
			bgradata[1] = rgbadata[1];
			bgradata[2] = rgbadata[0];
			bgradata[3] = rgbadata[3];
			bgradata+=4;
			rgbadata+=4;
		}
	}

	BZ_Free(scaled);

	ii.fIcon = FALSE;  // Change fIcon to TRUE to create an alpha icon
	ii.xHotspot = hotx;
	ii.yHotspot = hoty;
	ii.hbmMask = CreateBitmap(width,height,1,1,NULL);

	// Create the alpha cursor with the alpha DIB section.
	hAlphaCursor = CreateIconIndirect(&ii);

	DeleteObject(ii.hbmColor);
	DeleteObject(ii.hbmMask);

	return hAlphaCursor;
}

qboolean WIN_SetCursor(void *cursor)
{
	static POINT		current_pos;	//static to avoid bugs in vista(32) with largeaddressaware (this is fixed in win7). fixed exe base address prevents this from going above 2gb.

	hCustomCursor = cursor;

	//move the cursor to ensure the WM_SETCURSOR thing is invoked properly.
	//this ensures all the nastyness of random programs randomly setting the current global cursor is handled by microsoft's code instead of mine.
	//if you're using rawinput there'll be no lost inpuit problems, yay...
	GetCursorPos(&current_pos);
	SetCursorPos(current_pos.x, current_pos.y);
	return true;
}
void WIN_DestroyCursor(void *cursor)
{
	DestroyIcon(cursor);
}



/*
static HRESULT STDMETHODCALLTYPE DD_QueryInterface(IDropTarget *This, REFIID riid, void **ppvObject) {return E_NOINTERFACE;}
static ULONG STDMETHODCALLTYPE DD_AddRef(IDropTarget *This) {return 1;}
static ULONG STDMETHODCALLTYPE DD_Release(IDropTarget *This) {return 1;}
static HRESULT STDMETHODCALLTYPE DD_DragEnter(IDropTarget *This, IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
	*pdwEffect &= DROPEFFECT_COPY;
	return S_OK;
}
static HRESULT STDMETHODCALLTYPE DD_DragOver(IDropTarget *This, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
	*pdwEffect &= DROPEFFECT_COPY;
	return S_OK;
}
static HRESULT STDMETHODCALLTYPE DD_DragLeave(IDropTarget *This)
{
	return S_OK;
}
static HRESULT STDMETHODCALLTYPE DD_Drop(IDropTarget *This, IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
	*pdwEffect &= DROPEFFECT_COPY;
	return S_OK;
}
static struct IDropTargetVtbl MyDropTargetVtbl =
{
	DD_QueryInterface,
	DD_AddRef,
	DD_Release,
	DD_DragEnter,
	DD_DragOver,
	DD_DragLeave,
	DD_Drop
};
static IDropTarget MyDropTarget = {&MyDropTargetVtbl};*/
void WIN_WindowCreated(HWND window)
{
//	OleInitialize(NULL);
//	if (FAILED(RegisterDragDrop(window, &MyDropTarget)))
//		Con_Printf("RegisterDragDrop failed\n");
	DragAcceptFiles(window, TRUE);
}
#endif
