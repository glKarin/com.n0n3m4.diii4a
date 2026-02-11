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
#include <sys/types.h>
#include <sys/timeb.h>

#ifndef HAVE_CLIENT

#include <winsock.h>
#include <conio.h>
#include <direct.h>

#ifdef MULTITHREAD
#include <process.h>
#endif

#ifndef MINIMAL
//#define USESERVICE
#endif
#define SERVICENAME	DISTRIBUTION"SV"


static HANDLE hconsoleout;
extern int				isPlugin;	//if 2, we qcdebug to external program


qboolean	WinNT;	//if true, use utf-16 file paths. if false, hope that paths are in ascii.

#if defined(_DEBUG) || defined(DEBUG)
#define CATCHCRASH
#endif


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

DWORD CrashExceptionHandler (qboolean iswatchdog, DWORD exceptionCode, LPEXCEPTION_POINTERS exceptionInfo)
{
	char dumpPath[1024];
	HANDLE hProc = GetCurrentProcess();
	DWORD procid = GetCurrentProcessId();
	HANDLE dumpfile;
	HMODULE hDbgHelp;
	MINIDUMPWRITEDUMP fnMiniDumpWriteDump;
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
	case EXCEPTION_BREAKPOINT:
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
	default:
		//because windows is a steaming pile of shite, we have to ignore any software-generated exceptions, because most of them are not in fact fatal, *EVEN IF THEY CLAIM TO BE NON-CONTINUABLE*
		return exceptionCode;
	}

	hKernel = LoadLibrary ("kernel32");
	pIsDebuggerPresent = (void*)GetProcAddress(hKernel, "IsDebuggerPresent");

	if (pIsDebuggerPresent && pIsDebuggerPresent())
		return EXCEPTION_CONTINUE_SEARCH;
#if defined(HAVE_CLIENT) && defined(GLQUAKE)
	if (qrenderer == QR_OPENGL)
		GLVID_Crashed();
#endif

#if 1//ndef _MSC_VER
	{
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
#if defined(GLQUAKE) && !defined(D3DQUAKE)
	#define BUILDTYPE "GL"
#elif !defined(GLQUAKE) && defined(D3DQUAKE)
	#define BUILDTYPE "D3D"
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
			Sys_Printf("%s", stacklog+logstart);
			return EXCEPTION_EXECUTE_HANDLER;
		}
		else
		{
			Sys_Printf("We crashed.\nUnable to load dbghelp library. Stack info is not available\n");
			return EXCEPTION_EXECUTE_HANDLER;
		}
	}
#endif


	hDbgHelp = LoadLibrary ("DBGHELP");
	if (hDbgHelp)
		fnMiniDumpWriteDump = (MINIDUMPWRITEDUMP)GetProcAddress (hDbgHelp, "MiniDumpWriteDump");
	else
		fnMiniDumpWriteDump = NULL;

	if (fnMiniDumpWriteDump)
	{
		if (MessageBox(NULL, "KABOOM! We crashed!\nBlame the monkey in the corner.\nI hope you saved your work.\nWould you like to take a dump now?", DISTRIBUTION " Sucks", MB_ICONSTOP|MB_YESNO) != IDYES)
		{
			if (pIsDebuggerPresent ())
			{
				//its possible someone attached a debugger while we were showing that message
				return EXCEPTION_CONTINUE_SEARCH;
			}
			return EXCEPTION_EXECUTE_HANDLER;
		}

		/*take a dump*/
		GetTempPath (sizeof(dumpPath)-16, dumpPath);
		Q_strncatz(dumpPath, DISTRIBUTION"CrashDump.dmp", sizeof(dumpPath));
		dumpfile = CreateFile (dumpPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (dumpfile)
		{
			MINIDUMP_EXCEPTION_INFORMATION crashinfo;
			crashinfo.ClientPointers = TRUE;
			crashinfo.ExceptionPointers = exceptionInfo;
			crashinfo.ThreadId = GetCurrentThreadId ();
			if (fnMiniDumpWriteDump(hProc, procid, dumpfile, MiniDumpWithIndirectlyReferencedMemory|MiniDumpWithDataSegs, &crashinfo, NULL, NULL))
			{
				CloseHandle(dumpfile);
				MessageBox(NULL, va("You can find the crashdump at\n%s\nPlease send this file to someone.\n\nWarning: sensitive information (like your current user name) might be present in the dump.\nYou will probably want to compress it.", dumpPath), DISTRIBUTION " Sucks", 0);
				return EXCEPTION_EXECUTE_HANDLER;
			}
		}
	}
	else
		MessageBox(NULL, "Kaboom! Sorry. No MiniDumpWriteDump function.", FULLENGINENAME " Sucks", 0);
	return EXCEPTION_EXECUTE_HANDLER;
}

LONG CALLBACK nonmsvc_CrashExceptionHandler(PEXCEPTION_POINTERS ExceptionInfo)
{
	DWORD foo = EXCEPTION_CONTINUE_SEARCH;
	foo = CrashExceptionHandler(false, ExceptionInfo->ExceptionRecord->ExceptionCode, ExceptionInfo);
	//we have no handler. thus we handle it by exiting.
	if (foo == EXCEPTION_EXECUTE_HANDLER)
		exit(1);
	return foo;
}
#endif









void Sys_CloseLibrary(dllhandle_t *lib)
{
	FreeLibrary((HMODULE)lib);
}
dllhandle_t *Sys_LoadLibrary(const char *name, dllfunction_t *funcs)
{
	int i;
	HMODULE lib;

	lib = LoadLibrary(name);
	if (!lib)
		return NULL;

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




#include <fcntl.h>
#include <io.h>


#include <signal.h>

#include <shellapi.h>

#ifdef USESERVICE
qboolean asservice;
SERVICE_STATUS_HANDLE   ServerServiceStatusHandle;
SERVICE_STATUS          MyServiceStatus;
void CreateSampleService(qboolean create);
#endif

void PR_Deinit(void);

cvar_t	sys_nostdout = {"sys_nostdout","0"};
cvar_t	sys_colorconsole = {"sys_colorconsole", "1"};

HWND consolewindowhandle;
HWND hiddenwindowhandler;

int Sys_DebugLog(char *file, char *fmt, ...)
{
    va_list argptr;
    static char data[1024];
    int fd;

    va_start(argptr, fmt);
    vsnprintf(data, sizeof(data)-1, fmt, argptr);
    va_end(argptr);
    fd = open(file, O_WRONLY | O_CREAT | O_APPEND, 0666);
	if (fd)
	{
	    write(fd, data, strlen(data));
		close(fd);
		return 0;
	}
	return 1; // error
};

/*
================
Sys_FileTime
================
*/
int	Sys_FileTime (char *path)
{
	FILE	*f;

	f = fopen(path, "rb");
	if (f)
	{
		fclose(f);
		return 1;
	}

	return -1;
}


wchar_t *widen(wchar_t *out, size_t outlen, const char *utf8);
char *narrowen(char *out, size_t outlen, wchar_t *wide);

/*
================
Sys_mkdir
================
*/
void Sys_mkdir (const char *path)
{
	_mkdir(path);
}
qboolean Sys_rmdir (const char *path)
{
	return 0==_rmdir(path);
}

qboolean Sys_remove (const char *path)
{
	remove(path);

	return true;
}
qboolean Sys_Rename (const char *oldfname, const char *newfname)
{
	return !rename(oldfname, newfname);
}

static time_t Sys_FileTimeToTime(FILETIME ft)
{
	ULARGE_INTEGER ull;
	ull.LowPart = ft.dwLowDateTime;
	ull.HighPart = ft.dwHighDateTime;
	return ull.QuadPart / 10000000ULL - 11644473600ULL;
}

static int Sys_EnumerateFiles2 (const char *match, int matchstart, int neststart, int (QDECL *func)(const char *fname, qofs_t fsize, time_t mtime, void *parm, searchpathfuncs_t *spath), void *parm, searchpathfuncs_t *spath)
{
	qboolean go;
	if (!WinNT)
	{
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

			r = FindFirstFile(tmproot, &fd);
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
							go = Sys_EnumerateFiles2(file, matchstart, newnest, func, parm, spath);
						}
					}
				}
			} while(FindNextFile(r, &fd) && go);
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

			r = FindFirstFile(tmproot, &fd);
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
			} while(FindNextFile(r, &fd) && go);
			FindClose(r);
		}
	}
	else
	{
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
				r = FindFirstFileW(widen(wroot, sizeof(wroot), tmproot), &fd);
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
				r = FindFirstFileW(widen(wroot, sizeof(wroot), tmproot), &fd);
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
	}
	return go;
}
int Sys_EnumerateFiles (const char *gpath, const char *match, int (QDECL *func)(const char *fname, qofs_t fsize, time_t mtime, void *parm, searchpathfuncs_t *spath), void *parm, searchpathfuncs_t *spath)
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
================
Sys_Error
================
*/
#include <process.h>
void Sys_Error (const char *error, ...)
{
	va_list		argptr;
	char		text[1024];
	double end;
	STARTUPINFO startupinfo;
	PROCESS_INFORMATION processinfo;

	va_start (argptr,error);
	vsnprintf (text,sizeof(text)-1, error,argptr);
	va_end (argptr);
	COM_WorkerAbort(text);


//    MessageBox(NULL, text, "Error", 0 /* MB_OK */ );
	Sys_Printf ("ERROR: %s\n", text);

	Con_Log(text);

	NET_Shutdown();	//free sockets and stuff.

#ifdef USESERVICE
	if (asservice)
		Sys_Quit();
#endif

	if (COM_CheckParm("-noreset"))
	{
		Sys_Quit();
		exit(1);
	}

	Sys_Printf ("A new server will be started in 10 seconds unless you press a key\n");


	//check for a key press, quitting if we get one in 10 secs
	end = Sys_DoubleTime() + 10;
	while(Sys_DoubleTime() < end)
	{
		Sleep(500); // don't burn up CPU with polling
		if (_kbhit())
		{
			Sys_Quit();
			exit(1);
		}
	}

	Sys_Printf("\nLoading new instance of FTE...\n\n\n");
#ifdef HAVE_SERVER
	PR_Deinit();	//this takes a bit more mem
#ifdef SVRANKING
	Rank_Flush();
#endif
#endif
#ifndef MINGW
	fcloseall();	//make sure all files are written.
#endif

//	system("dqwsv.exe");	//spawn a new server to take over. This way, if debugging, then any key will quit, otherwise the server will just spawn a new one.

	memset(&startupinfo, 0, sizeof(startupinfo));
	memset(&processinfo, 0, sizeof(processinfo));

	CreateProcess(NULL,
		GetCommandLine(),
		NULL,
		NULL,
		false,
		0,
		NULL,
		NULL,
		&startupinfo,
		&processinfo);

	CloseHandle(processinfo.hProcess);
	CloseHandle(processinfo.hThread);

	Sys_Quit ();

	exit (1); // this function is NORETURN type, complains without this
}

/*
================
Sys_Milliseconds
================
*/
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

/*
================
Sys_DoubleTime
================
*/
double Sys_DoubleTime (void)
{
	double t;
    struct _timeb tstruct;
	static int	starttime;

	_ftime( &tstruct );

	if (!starttime)
		starttime = tstruct.time;
	t = (tstruct.time-starttime) + tstruct.millitm*0.001;

	return t;
}


/*
================
Sys_ConsoleInput
================
*/
void SV_GetNewSpawnParms(client_t *cl);
char	coninput_text[256];
int		coninput_len;
char *Sys_ConsoleInput (void)
{
	int		c;

	if (consolewindowhandle)
	{
		MSG msg;
		while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (!GetMessage (&msg, NULL, 0, 0))
				return NULL;
			TranslateMessage (&msg);
			DispatchMessage (&msg);
		}
		return NULL;
	}

#ifdef SUBSERVERS
	if (SSV_IsSubServer())
		return NULL;
#endif

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
				if (coninput_len)
				{
					putch ('\r');
					putch (']');
				}
				coninput_len = 0;
				Q_strncpyz(coninput_text, text, sizeof(coninput_text));
				memmove(text, nl, textpos - (nl - text));
				textpos -= (nl - text);
				return coninput_text;
			}
			else
				break;
		}
	}




	// read a line out
	while (_kbhit())
	{
		c = _getch();
		if (c == '\r')
		{
			coninput_text[coninput_len] = 0;
			putch ('\n');
			putch (']');
			coninput_len = 0;
			return coninput_text;
		}
		if (c == 8)
		{
			if (coninput_len)
			{
				putch (c);
				putch (' ');
				putch (c);
				coninput_len--;
				coninput_text[coninput_len] = 0;
			}
			continue;
		}
		if (c == '\t')
		{
			int i;
			char *s = Cmd_CompleteCommand(coninput_text, true, true, 0, NULL);
			if(s)
			{
				for (i = 0; i < coninput_len; i++)
					putch('\b');
				for (i = 0; i < coninput_len; i++)
					putch(' ');
				for (i = 0; i < coninput_len; i++)
					putch('\b');

				strcpy(coninput_text, s);
				coninput_len = strlen(coninput_text);
				printf("%s", coninput_text);
			}
			continue;
		}
		putch (c);
		coninput_text[coninput_len] = c;
		coninput_len++;
		coninput_text[coninput_len] = 0;
		if (coninput_len == sizeof(coninput_text))
			coninput_len = 0;
	}

	return NULL;
}

void ApplyColour(unsigned int chr)
{
	static int oldchar = CON_WHITEMASK;
	chr &= CON_FLAGSMASK;

	if (oldchar == chr)
		return;
	oldchar = chr;

	if (hconsoleout)
	{
		unsigned short val = 0;

		// bits 28-31 of the console chars match up to the attributes for
		// the CHAR_INFO struct exactly
		if (chr & CON_NONCLEARBG)
			val = ((chr & (CON_FGMASK|CON_BGMASK)) >> CON_FGSHIFT);
		else
		{
			int fg = (chr & CON_FGMASK) >> CON_FGSHIFT;

			switch (fg)
			{
			case COLOR_BLACK: // reverse ^0 like the Linux version
				val = BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE;
				break;
			case COLOR_WHITE: // reset to defaults?
				val = FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE; // use grey
				break;
			case COLOR_GREY:
				val = FOREGROUND_INTENSITY; // color light grey as dark grey
				break;
			default:
				val = fg; // send RGBI value as is
				break;
			}
		}

		if ((chr & CON_HALFALPHA) && (val & ~FOREGROUND_INTENSITY))
			val &= ~FOREGROUND_INTENSITY; // strip intensity to fake alpha

		SetConsoleTextAttribute(hconsoleout, val);
	}
}

//this could be much more efficient.
static void Sys_PrintColouredChars(conchar_t *start, conchar_t *end)
{
	wchar_t wc[256];
	int l;
	DWORD dummy;
	unsigned int cp, flags, m;

	m = CON_WHITEMASK;
	l = 0;

	while(start < end)
	{
		start = Font_Decode(start, &flags, &cp);

		if (l+2 >= countof(wc) || flags != m)
		{
			ApplyColour(m);
			if (WinNT)
				WriteConsoleW(hconsoleout, wc, l, &dummy, NULL);
			else
			{
				//win95 doesn't support wide chars *sigh*. blank consoles suck.
				char ac[256];
				l = WideCharToMultiByte(CP_ACP, 0, wc, l, ac, sizeof(ac), NULL, NULL);
				WriteConsole(hconsoleout, ac, l, &dummy, NULL);
			}
			l = 0;
		}

		if (!(flags & CON_HIDDEN))
		{
			if (cp >= 0xe000 && cp < 0xe100)
			{
				cp -= 0xe000;
				if (cp >= 0x80)
				{
					char c1[32] = "---..........>  "   "[]0123456789.---";
					cp -= 0x80;
					if (cp <= countof(c1))
						cp = c1[cp];
				}
			}
			if (cp > 0xffff)
				cp = '?';	//too lazy for utf-16 crap when its mostly smilies anyway.
			wc[l++] = cp;
		}
	}

	//and flush it.
	if (l)
	{
		ApplyColour(m);
		if (WinNT)
			WriteConsoleW(hconsoleout, wc, l, &dummy, NULL);
		else
		{
			//win95 doesn't support wide chars *sigh*. blank consoles suck.
			char ac[256];
			l = WideCharToMultiByte(CP_ACP, 0, wc, l, ac, sizeof(ac), NULL, NULL);
			WriteConsole(hconsoleout, ac, l, &dummy, NULL);
		}
	}

	ApplyColour(CON_WHITEMASK);
}

/*
================
Sys_Printf
================
*/
#define	MAXPRINTMSG	4096
void Sys_Printf (char *fmt, ...)
{
	va_list		argptr;

	if (sys_nostdout.value)
		return;

	if (1)
	{
		char		msg[MAXPRINTMSG];
		unsigned char *t;

		va_start (argptr,fmt);
		vsnprintf (msg,sizeof(msg)-1, fmt,argptr);
		va_end (argptr);

#ifdef SUBSERVERS
		if (SSV_IsSubServer())
		{
			SSV_PrintToMaster(msg);
			return;
		}
#endif

		{
			int i;

			for (i = 0; i < coninput_len; i++)
				putch('\b');
			putch('\b');
			for (i = 0; i < coninput_len; i++)
				putch(' ');
			putch(' ');
			for (i = 0; i < coninput_len; i++)
				putch('\b');
			putch('\b');
		}

		if (sys_colorconsole.value && hconsoleout)
		{
			conchar_t out[MAXPRINTMSG], *end;
			end = COM_ParseFunString(CON_WHITEMASK, msg, out, sizeof(out), false);
			Sys_PrintColouredChars (out, end);
		}
		else
		{
			for (t = (unsigned char*)msg; *t; t++)
			{
				if (*t >= 146 && *t < 156)
					*t = *t - 146 + '0';
				if (*t >= 0x12 && *t <= 0x1b)
					*t = *t - 0x12 + '0';
				if (*t == 143)
					*t = '.';
				if (*t == 157 || *t == 158 || *t == 159)
					*t = '-';
				if (*t >= 128)
					*t -= 128;
				if (*t == 16)
					*t = '[';
				if (*t == 17)
					*t = ']';
				if (*t == 0x1c)
					*t = 249;
			}
			printf("%s", msg);
		}


		if (coninput_len)
			printf("]%s", coninput_text);
		else
			putch(']');
	}
	else
	{
		va_start (argptr,fmt);
		vprintf (fmt,argptr);
		va_end (argptr);
	}
}

/*
================
Sys_Quit
================
*/

void Sys_Quit (void)
{
#ifdef USESERVICE
	if (asservice)
	{
		MyServiceStatus.dwCurrentState       = SERVICE_STOPPED;
		MyServiceStatus.dwCheckPoint         = 0;
		MyServiceStatus.dwWaitHint           = 0;
		MyServiceStatus.dwWin32ExitCode      = 0;
		MyServiceStatus.dwServiceSpecificExitCode = 0;

		SetServiceStatus (ServerServiceStatusHandle, &MyServiceStatus);
	}
#endif
	exit (0);
}

int restorecode;

LRESULT (CALLBACK Sys_WindowHandler)(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_USER)
	{
		if (lParam & 1)
		{
		}
		else if ((lParam & 2 && restorecode == 0) ||
			(lParam & 4 && restorecode == 1) ||
			(lParam & 4 && restorecode == 2) )
		{
// 			MessageBox(NULL, "Hello", "", 0);
			restorecode++;
		}
		else if (lParam & 2 && restorecode == 3)
		{
			DestroyWindow(hWnd);
			ShowWindow(consolewindowhandle, SW_SHOWNORMAL);
			consolewindowhandle = NULL;

			Cbuf_AddText("status\n", RESTRICT_LOCAL);
		}
		else if (lParam & 6)
		{
			restorecode = (lParam & 2)>0;
		}

		return 0;
	}
	return DefWindowProc (hWnd, uMsg, wParam, lParam);
}
void Sys_HideConsole(void)
{
	HMODULE kernel32dll;
	HWND (WINAPI *GetConsoleWindow)(void);

	if (consolewindowhandle)
		return;	//err... already hidden... ?

	restorecode = 0;

	GetConsoleWindow = NULL;
	kernel32dll = LoadLibrary("kernel32.dll");
	consolewindowhandle = NULL;
	if (kernel32dll)
	{
		GetConsoleWindow = (void*)GetProcAddress(kernel32dll, "GetConsoleWindow");
		if (GetConsoleWindow)
			consolewindowhandle = GetConsoleWindow();

		FreeModule(kernel32dll);	//works because the underlying code uses kernel32, so this decreases the reference count rather than closing it.
	}

	if (!consolewindowhandle)
	{
		char old[512];
#define STRINGH	"Trying to hide"	//msvc sucks
		GetConsoleTitle(old, sizeof(old));
		SetConsoleTitle(STRINGH);
		consolewindowhandle = FindWindow(NULL, STRINGH);
		SetConsoleTitle(old);
#undef STRINGH
	}

	if (consolewindowhandle)
	{
		WNDCLASS wc;
		NOTIFYICONDATA d;

			/* Register the frame class */
		memset(&wc, 0, sizeof(wc));
		wc.style         = 0;
		wc.lpfnWndProc   = Sys_WindowHandler;
		wc.cbClsExtra    = 0;
		wc.cbWndExtra    = 0;
		wc.hInstance     = GetModuleHandle(NULL);
		wc.hIcon         = 0;
		wc.hCursor       = LoadCursor (NULL,IDC_ARROW);
		wc.hbrBackground = NULL;
		wc.lpszMenuName  = 0;
		wc.lpszClassName = "DeadQuake";

		RegisterClass (&wc);

		hiddenwindowhandler = CreateWindow(wc.lpszClassName, "DeadQuake", 0, 0, 0, 16, 16, NULL, NULL, GetModuleHandle(NULL), NULL);
		if (!hiddenwindowhandler)
		{
			Con_Printf("Failed to create window\n");
			return;
		}
		ShowWindow(consolewindowhandle, SW_HIDE);

		d.cbSize = sizeof(NOTIFYICONDATA);
		d.hWnd = hiddenwindowhandler;
		d.uFlags = NIF_ICON | NIF_TIP | NIF_MESSAGE;
		d.hIcon = NULL;
		d.uCallbackMessage = WM_USER;
		d.uID = 0;
		strcpy(d.szTip, "");
		Shell_NotifyIcon(NIM_ADD, &d);
	}
	else
		Con_Printf("Your OS doesn't seem to properly support the way this was implemented\n");
}

void Sys_ServerActivity(void)
{
	HMODULE kernel32dll;
	HWND (WINAPI *GetConsoleWindow)(void);
	HWND wnd;

	restorecode = 0;

	GetConsoleWindow = NULL;
	kernel32dll = LoadLibrary("kernel32.dll");
	wnd = NULL;
	if (kernel32dll)
	{
		GetConsoleWindow = (void*)GetProcAddress(kernel32dll, "GetConsoleWindow");
		if (GetConsoleWindow)
			wnd = GetConsoleWindow();

		FreeModule(kernel32dll);	//works because the underlying code uses kernel32, so this decreases the reference count rather than closing it.
	}

	if (!wnd)
	{
		char old[512];
#define STRINGF	"About To Flash"	//msvc sucks
		GetConsoleTitle(old, sizeof(old));
		SetConsoleTitle(STRINGF);
		wnd = FindWindow(NULL, STRINGF);
		SetConsoleTitle(old);
#undef STRINGF
	}

	if (wnd && GetActiveWindow() != wnd)
		FlashWindow(wnd, true);
}

/*
=============
Sys_Init

Quake calls this so the system can register variables before host_hunklevel
is marked
=============
*/
void Sys_Init (void)
{
	Cvar_Register (&sys_nostdout, "System controls");
	Cvar_Register (&sys_colorconsole, "System controls");

	Cmd_AddCommand("hide", Sys_HideConsole);

	if (isPlugin)
		hconsoleout = CreateFileA("CONOUT$",GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,0,OPEN_EXISTING,0,0);
	else
		hconsoleout = GetStdHandle(STD_OUTPUT_HANDLE);

//	SetConsoleCP(CP_UTF8);
//	SetConsoleOutputCP(CP_UTF8);
}

void Sys_Shutdown (void)
{
}

/*
==================
main

==================
*/
char	*newargv[256];

void Signal_Error_Handler (int sig)
{
	Sys_Error("Illegal error occured");
}


void StartQuakeServer(void)
{
	quakeparms_t	parms;
	static	char	bindir[MAX_OSPATH];
	int c;

	memset(&parms, 0, sizeof(parms));

	parms.argc = com_argc;
	parms.argv = com_argv;

	GetModuleFileName(NULL, bindir, sizeof(bindir)-1);
	*COM_SkipPath(bindir) = 0;
	parms.binarydir = bindir;

	parms.basedir = "./";

	TL_InitLanguages(parms.basedir);

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

	SV_Init (&parms);

// run one frame immediately for first heartbeat
	SV_Frame ();
}


#ifdef USESERVICE
int servicecontrol;
#endif
void ServerMainLoop(void)
{
	float delay = 0.001;
//
// main loop
//
	while (1)
	{
		NET_Sleep(delay, false);

	// find time passed since last cycle
		delay = SV_Frame();


#ifdef USESERVICE
		switch(servicecontrol)
		{
		case SERVICE_CONTROL_PAUSE:
			// Initialization complete - report running status.
			MyServiceStatus.dwCurrentState       = SERVICE_PAUSED;
			MyServiceStatus.dwCheckPoint         = 0;
			MyServiceStatus.dwWaitHint           = 0;

			SetServiceStatus (ServerServiceStatusHandle, &MyServiceStatus);
			sv.paused |= PAUSE_SERVICE;
			break;
		case SERVICE_CONTROL_CONTINUE:
			// Initialization complete - report running status.
			MyServiceStatus.dwCurrentState       = SERVICE_RUNNING;
			MyServiceStatus.dwCheckPoint         = 0;
			MyServiceStatus.dwWaitHint           = 0;

			SetServiceStatus (ServerServiceStatusHandle, &MyServiceStatus);

			sv.paused &= ~PAUSE_SERVICE;
			break;
		case SERVICE_CONTROL_STOP:	//leave the loop
			return;
		default:
			break;
		}
#endif
	}
}
#ifdef USESERVICE

VOID WINAPI MyServiceCtrlHandler(DWORD    dwControl)
{
	servicecontrol = dwControl;
}

void WINAPI StartQuakeServerService	(DWORD argc, LPTSTR *argv)
{
	HKEY hk;
	char path[MAX_OSPATH];
	DWORD pathlen;
	DWORD type;

	asservice = true;

	MyServiceStatus.dwServiceType        = SERVICE_WIN32|SERVICE_INTERACTIVE_PROCESS;
	MyServiceStatus.dwCurrentState       = SERVICE_START_PENDING;
	MyServiceStatus.dwControlsAccepted   = SERVICE_ACCEPT_STOP |
		SERVICE_ACCEPT_PAUSE_CONTINUE;
	MyServiceStatus.dwWin32ExitCode      = 0;
	MyServiceStatus.dwServiceSpecificExitCode = 0;
	MyServiceStatus.dwCheckPoint         = 0;
	MyServiceStatus.dwWaitHint           = 0;

	ServerServiceStatusHandle = RegisterServiceCtrlHandler(
		SERVICENAME,
		MyServiceCtrlHandler);

	if (ServerServiceStatusHandle == (SERVICE_STATUS_HANDLE)0)
	{
		printf(" [MY_SERVICE] RegisterServiceCtrlHandler failed %d\n", GetLastError());
		return;
	}


	RegOpenKey(HKEY_LOCAL_MACHINE, "Software\\"DISTRIBUTIONLONG"\\"FULLENGINENAME, &hk);
	RegQueryValueEx(hk, "servicepath", 0, &type, NULL, &pathlen);
	if (type == REG_SZ && pathlen < sizeof(path))
		RegQueryValueEx(hk, "servicepath", 0, NULL, path, &pathlen);
	RegCloseKey(hk);

	SetCurrentDirectory(path);


	COM_InitArgv (argc, argv);
	StartQuakeServer();


	// Handle error condition
	if (!sv.state)
	{
		MyServiceStatus.dwCurrentState       = SERVICE_STOPPED;
		MyServiceStatus.dwCheckPoint         = 0;
		MyServiceStatus.dwWaitHint           = 0;
		MyServiceStatus.dwWin32ExitCode      = 0;
		MyServiceStatus.dwServiceSpecificExitCode = 0;

		SetServiceStatus (ServerServiceStatusHandle, &MyServiceStatus);
		return;
	}

	// Initialization complete - report running status.
	MyServiceStatus.dwCurrentState       = SERVICE_RUNNING;
	MyServiceStatus.dwCheckPoint         = 0;
	MyServiceStatus.dwWaitHint           = 0;

	if (!SetServiceStatus (ServerServiceStatusHandle, &MyServiceStatus))
	{
		printf(" [MY_SERVICE] SetServiceStatus error %ld\n",GetLastError());
	}

	ServerMainLoop();

	MyServiceStatus.dwCurrentState       = SERVICE_STOPPED;
	MyServiceStatus.dwCheckPoint         = 0;
	MyServiceStatus.dwWaitHint           = 0;
	MyServiceStatus.dwWin32ExitCode      = 0;
	MyServiceStatus.dwServiceSpecificExitCode = 0;

	SetServiceStatus (ServerServiceStatusHandle, &MyServiceStatus);

	return;
}

SERVICE_TABLE_ENTRY   DispatchTable[] =
{
{ SERVICENAME, StartQuakeServerService      },
{ NULL,              NULL          }
};
#endif

int main (int argc, char **argv)
{
#ifdef CATCHCRASH
	LoadLibrary ("DBGHELP");	//heap corruption can prevent loadlibrary from working properly, so do this in advance.
#ifdef _MSC_VER
	__try
#else
	AddVectoredExceptionHandler(true, nonmsvc_CrashExceptionHandler);
#endif
#endif
	{
		COM_InitArgv (argc, (const char **)argv);

#ifdef SUBSERVERS
		isClusterSlave = COM_CheckParm("-clusterslave");
		if (isClusterSlave)
            SSV_SetupControlPipe(Sys_GetStdInOutStream(), false);
#endif
#ifdef USESERVICE
		if (!SSV_IsSubServer() && StartServiceCtrlDispatcher( DispatchTable))
		{
			return true;
		}
#endif

	#ifdef USESERVICE
		if (COM_CheckParm("-register"))
		{
			CreateSampleService(1);
			return true;
		}
		if (COM_CheckParm("-unregister"))
		{
			CreateSampleService(0);
			return true;
		}
	#endif

	#ifndef _DEBUG
		if (COM_CheckParm("-noreset"))
		{
			signal (SIGFPE,	Signal_Error_Handler);
			signal (SIGILL,	Signal_Error_Handler);
			signal (SIGSEGV,	Signal_Error_Handler);
		}
	#endif

		StartQuakeServer();

		ServerMainLoop();
	}
#ifdef CATCHCRASH
#ifdef _MSC_VER
	__except (CrashExceptionHandler(false, GetExceptionCode(), GetExceptionInformation()))
	{
		return 1;
	}
#endif
#endif

	return true;
}

#ifdef USESERVICE
void CreateSampleService(qboolean create)
{
	BOOL deleted;
	char path[MAX_OSPATH];
	char exe[MAX_OSPATH];
	SC_HANDLE schService;

	SC_HANDLE schSCManager;

// Open a handle to the SC Manager database.
	schSCManager = OpenSCManager(
		NULL,                    // local machine
		NULL,                    // ServicesActive database
		SC_MANAGER_ALL_ACCESS);  // full access rights

	if (NULL == schSCManager)
	{
		Con_Printf("Failed to open SCManager (%d)\n", GetLastError());
		return;
	}

	if (!GetModuleFileName(NULL, exe+1, sizeof(exe)-2))
	{
		Con_Printf("Path too long\n");
		return;
	}
	GetCurrentDirectory(sizeof(path), path);
	exe[0] = '\"';
	exe[strlen(path)+1] = '\0';
	exe[strlen(path)] = '\"';

	if (!create)
	{
		schService = OpenServiceA(schSCManager, SERVICENAME, SERVICE_ALL_ACCESS);
		if (schService)
		{
			deleted = DeleteService(schService);
		}
	}
	else
	{
		HKEY hk;
		RegOpenKey(HKEY_LOCAL_MACHINE, "Software\\"DISTRIBUTIONLONG"\\"FULLENGINENAME, &hk);
		if (!hk)RegCreateKey(HKEY_LOCAL_MACHINE, "Software\\"DISTRIBUTIONLONG"\\"FULLENGINENAME, &hk);
		RegSetValueEx(hk, "servicepath", 0, REG_SZ, path, strlen(path));
		RegCloseKey(hk);

		schService = CreateService(
			schSCManager,				// SCManager database
			SERVICENAME,				// name of service
			FULLENGINENAME" Server",	// service name to display
			SERVICE_ALL_ACCESS,			// desired access
			SERVICE_WIN32_OWN_PROCESS|SERVICE_INTERACTIVE_PROCESS,	// service type
			SERVICE_AUTO_START,			// start type
			SERVICE_ERROR_NORMAL,		// error control type
			exe,						// service's binary
			NULL,						// no load ordering group
			NULL,						// no tag identifier
			NULL,						// no dependencies
			NULL,						// LocalSystem account
			NULL);						// no password
	}

    if (schService == NULL)
    {
        Con_Printf("CreateService failed.\n");
        return;
    }
    else
    {
        CloseServiceHandle(schService);
        return;
    }
}
#endif

void Sys_Sleep (double seconds)
{
	Sleep(seconds * 1000);
}

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


#ifdef HAVEAUTOUPDATE
int Sys_GetAutoUpdateSetting(void)
{
	return -1;
}
void Sys_SetAutoUpdateSetting(int newval)
{
}
#endif

#ifdef WEBCLIENT
qboolean Sys_RunInstaller(void)
{	//not implemented
	return false;
}
#endif
#endif
