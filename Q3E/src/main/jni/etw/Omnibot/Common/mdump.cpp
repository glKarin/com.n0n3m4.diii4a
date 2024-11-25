#include "common.h"
#include "mdump.h"

String		g_AppName;

#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOWINRES
#define NOWINRES
#endif
#ifndef NOSERVICE
#define NOSERVICE
#endif
#ifndef NOMCX
#define NOMCX
#endif
#ifndef NOIME
#define NOIME
#endif
#include <Windows.h>
#include <assert.h>
#include <tchar.h>
#include <stdio.h>

#if _MSC_VER < 1300
#define DECLSPEC_DEPRECATED
// VC6: change this path to your Platform SDK headers
#include "M:\\dev7\\vs\\devtools\\common\\win32sdk\\include\\dbghelp.h"			// must be XP version of file
#else
// VC7: ships with updated headers
#pragma warning(disable : 4091) // 'typedef ' : ignored on left of '' when no variable is declared
#include "dbghelp.h"
#endif

// based on dbghelp.h
typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(HANDLE hProcess, DWORD dwPid, HANDLE hFile, MINIDUMP_TYPE DumpType,
										 CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
										 CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
										 CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam
										 );

static LONG WINAPI TopLevelFilter( struct _EXCEPTION_POINTERS *pExceptionInfo )
{
	LONG retval = EXCEPTION_CONTINUE_SEARCH;
	//HWND hParent = NULL;						// find a better value for your app

	String DumpPath;
	bool EnableDump = false, EnableDumpDialog = false;
	Options::GetValue("Debug","DumpFileEnable",EnableDump);
	Options::GetValue("Debug","DumpFileDialog",EnableDumpDialog);

	if(!EnableDump)
		return retval;

	// firstly see if dbghelp.dll is around and has the function we need
	// look next to the EXE first, as the one in System32 might be old 
	// (e.g. Windows 2000)
	HMODULE hDll = NULL;
	char szDbgHelpPath[_MAX_PATH];

	if (GetModuleFileName( NULL, szDbgHelpPath, _MAX_PATH ))
	{
		char *pSlash = _tcsrchr( szDbgHelpPath, '\\' );
		if (pSlash)
		{
			_tcscpy( pSlash+1, "DBGHELP.DLL" );
			hDll = ::LoadLibrary( szDbgHelpPath );
		}
	}

	if (hDll==NULL)
	{
		// load any version we can
		hDll = ::LoadLibrary( "DBGHELP.DLL" );
	}

	LPCTSTR szResult = NULL;

	if (hDll)
	{
		MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)::GetProcAddress( hDll, "MiniDumpWriteDump" );
		if (pDump)
		{
			char szScratch[_MAX_PATH] = {0};
			char szDumpPath[_MAX_PATH] = {0};

			IGame *game = IGameManager::GetInstance()->GetGame();

			// Get System Time for filename
			SYSTEMTIME pTime;
			GetSystemTime( &pTime );
			sprintf( szDumpPath, "%s%s_%d.%d.%d.%d.%d.%d.v%s.dmp", 
				"", 
				g_AppName.c_str(), 
				pTime.wMonth, 
				pTime.wDay,
				pTime.wYear,
				pTime.wHour,
				pTime.wMinute,
				pTime.wSecond, 
				game ? game->GetVersion() : "");

			// ask the user if they want to save a dump file
			if (!EnableDumpDialog || ::MessageBox( NULL, 
				"Crash detected, would you like to save a dump file?", g_AppName.c_str(), MB_YESNO )==IDYES)
			{
				// create the file
				HANDLE hFile = ::CreateFile( szDumpPath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS,
					FILE_ATTRIBUTE_NORMAL, NULL );

				if (hFile!=INVALID_HANDLE_VALUE)
				{
					_MINIDUMP_EXCEPTION_INFORMATION ExInfo;

					ExInfo.ThreadId = ::GetCurrentThreadId();
					ExInfo.ExceptionPointers = pExceptionInfo;
					ExInfo.ClientPointers = NULL;

					// write the dump
					BOOL bOK = pDump( GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, NULL, NULL );
					if (bOK)
					{
						sprintf( szScratch, "Saved dump file to '%s'", szDumpPath );
						szResult = szScratch;
						retval = EXCEPTION_EXECUTE_HANDLER;
					}
					else
					{
						sprintf( szScratch, "Error saving dump file to '%s' (error %d)", szDumpPath, GetLastError() );
						szResult = szScratch;
					}
					::CloseHandle(hFile);
				}
				else
				{
					sprintf( szScratch, "Error creating dump file '%s' (error %d)", szDumpPath, GetLastError() );
					szResult = szScratch;
				}
			}
		}
		else
		{
			szResult = "DBGHELP.DLL too old";
		}
	}
	else
	{
		szResult = "DBGHELP.DLL not found";
	}

	if (EnableDumpDialog && szResult)
		::MessageBox( NULL, szResult, g_AppName.c_str(), MB_OK );

	return retval;
}

namespace MiniDumper
{
	void Init(const char *_appname)
	{
		assert( g_AppName.empty() );
		g_AppName = _appname ? _appname : "Application";

		::SetUnhandledExceptionFilter( TopLevelFilter );
	}

	void Shutdown()
	{
		g_AppName.clear();
	}
}

//////////////////////////////////////////////////////////////////////////

#else

namespace MiniDumper
{
	void Init(const char *_appname)
	{
		assert( g_AppName.empty() );
		g_AppName = _appname ? _appname : "Application";
	}

	void Shutdown()
	{
		g_AppName.clear();
	}
}

#endif

