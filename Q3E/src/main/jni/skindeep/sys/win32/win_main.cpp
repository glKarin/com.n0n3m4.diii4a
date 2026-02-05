/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

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

#undef _WIN32_WINNT
#define  _WIN32_WINNT 0x0603

#include "sys/platform.h"
#include <thread>
#include "idlib/CmdArgs.h"
#include "framework/async/AsyncNetwork.h"
#include "framework/Licensee.h"
#include "framework/UsercmdGen.h"
#include "renderer/tr_local.h"
#include "sys/win32/rc/CreateResourceIDs.h"
#include "sys/sys_local.h"

#include "sys/win32/win_local.h"
#include "framework/Console.h"

#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#include <shellapi.h>
#include <shlobj.h>

#ifndef __MRC__
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include <SDL_main.h>
#include <SDL_cpuinfo.h>
#include "rc/resource.h"
#include <DbgHelp.h>
#include <winnt.h>
#include <commdlg.h>
#include <ctime>
#include "framework/miniz/miniz.h"
#include "framework/Session_local.h"

#include "framework/FileSystem.h"

idCVar Win32Vars_t::win_outputDebugString( "win_outputDebugString", "0", CVAR_SYSTEM | CVAR_BOOL, "" );
idCVar Win32Vars_t::win_outputEditString( "win_outputEditString", "1", CVAR_SYSTEM | CVAR_BOOL, "" );
idCVar Win32Vars_t::win_viewlog( "win_viewlog", "0", CVAR_SYSTEM | CVAR_INTEGER, "" );
idCVar Win32Vars_t::win_mindiskspace("win_mindiskspace", "1024", CVAR_SYSTEM | CVAR_ARCHIVE, "Minimum disk space (in MB) required to launch");
idCVar win_dumploglines("win_dumploglines", "50", CVAR_SYSTEM | CVAR_INTEGER | CVAR_ARCHIVE, "Number of recent log lines to show in crash dump");

Win32Vars_t	win32;

/*
=============
Sys_Error

Show the early console as an error dialog
=============
*/
void Sys_Error( const char *error, ... ) {
	va_list		argptr;
	char		text[4096];
	MSG        msg;

	va_start( argptr, error );
	vsprintf( text, error, argptr );
	va_end( argptr);

	printf("%s", text);

	Conbuf_AppendText( text );
	Conbuf_AppendText( "\n" );

	Win_SetErrorText( text );
	Sys_ShowConsole( 1, true );

	timeEndPeriod( 1 );

	Sys_ShutdownInput();

	GLimp_Shutdown();

	// wait for the user to quit
	while ( 1 ) {
		if ( !GetMessage( &msg, NULL, 0, 0 ) ) {
			common->Quit();
		}
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	Sys_DestroyConsole();

	exit (1);
}

/*
==============
Sys_Quit
==============
*/
void Sys_Quit( void ) {
	timeEndPeriod( 1 );
	Sys_ShutdownInput();
	Sys_DestroyConsole();
	ExitProcess( 0 );
}


/*
==============
Sys_Printf
==============
*/
#define MAXPRINTMSG 4096
void Sys_Printf( const char *fmt, ... ) {
	char		msg[MAXPRINTMSG];

	va_list argptr;
	va_start(argptr, fmt);
	idStr::vsnPrintf( msg, MAXPRINTMSG-1, fmt, argptr );
	va_end(argptr);
	msg[sizeof(msg)-1] = '\0';

	printf("%s", msg);

	if ( win32.win_outputDebugString.GetBool() ) {
		OutputDebugString( msg );
	}
	if ( win32.win_outputEditString.GetBool() ) {
		Conbuf_AppendText( msg );
	}
}

/*
==============
Sys_DebugPrintf
==============
*/
#define MAXPRINTMSG 4096
void Sys_DebugPrintf( const char *fmt, ... ) {
	char msg[MAXPRINTMSG];

	va_list argptr;
	va_start( argptr, fmt );
	idStr::vsnPrintf( msg, MAXPRINTMSG-1, fmt, argptr );
	msg[ sizeof(msg)-1 ] = '\0';
	va_end( argptr );

	printf("%s", msg);

	OutputDebugString( msg );
}

/*
==============
Sys_DebugVPrintf
==============
*/
void Sys_DebugVPrintf( const char *fmt, va_list arg ) {
	char msg[MAXPRINTMSG];

	idStr::vsnPrintf( msg, MAXPRINTMSG-1, fmt, arg );
	msg[ sizeof(msg)-1 ] = '\0';

	printf("%s", msg);

	OutputDebugString( msg );
}

/*
==============
Sys_ShowWindow
==============
*/
void Sys_ShowWindow( bool show ) {
	::ShowWindow( win32.hWnd, show ? SW_SHOW : SW_HIDE );
}

/*
==============
Sys_IsWindowVisible
==============
*/
bool Sys_IsWindowVisible( void ) {
	return ( ::IsWindowVisible( win32.hWnd ) != 0 );
}

/*
==============
Sys_IsWindowActive
==============
*/
bool Sys_IsWindowActive( void ) {
	return GLimp_WindowActive();
}

/*
==============
Sys_Mkdir
==============
*/
int Sys_Mkdir( const char *path ) {
	return _mkdir (path);
}

/*
==============
Sys_Access
==============
*/
// modes: 00 Existence, 02 Write, 04 Read, 06 Read+write
int Sys_Access( const char *path, SYS_ACCESS_MODE mode ) {
	return _access(path, mode);
}

/*
=================
Sys_FileTimeStamp
=================
*/
ID_TIME_T Sys_FileTimeStamp( FILE *fp ) {
	struct _stat st;
	_fstat( _fileno( fp ), &st );
	return (long) st.st_mtime;
}

/*
==============
Sys_Cwd
==============
*/
const char *Sys_Cwd( void ) {
	static char cwd[MAX_OSPATH];

	_getcwd( cwd, sizeof( cwd ) - 1 );
	cwd[MAX_OSPATH-1] = 0;

	return cwd;
}

static int WPath2A(char *dst, size_t size, const WCHAR *src) {
	int len;
	BOOL default_char = FALSE;

	// test if we can convert lossless
	len = WideCharToMultiByte(CP_ACP, 0, src, -1, dst, size, NULL, &default_char);

	if (default_char) {
		/* The following lines implement a horrible
		   hack to connect the UTF-16 WinAPI to the
		   ASCII doom3 strings. While this should work in
		   most cases, it'll fail if the "Windows to
		   DOS filename translation" is switched off.
		   In that case the function will return NULL
		   and no homedir is used. */
		WCHAR w[MAX_OSPATH];
		len = GetShortPathNameW(src, w, sizeof(w));

		if (len == 0)
			return 0;

		/* Since the DOS path contains no UTF-16 characters, convert it to the system's default code page */
		len = WideCharToMultiByte(CP_ACP, 0, w, len, dst, size - 1, NULL, NULL);
	}

	if (len == 0)
		return 0;

	dst[len] = 0;
	/* Replace backslashes by slashes */
	for (int i = 0; i < len; ++i)
		if (dst[i] == '\\')
			dst[i] = '/';

	// cut trailing slash
	if (dst[len - 1] == '/') {
		dst[len - 1] = 0;
		len--;
	}

	return len;
}

/*
==============
Returns "My Documents"/My Games/dhewm3 directory (or equivalent - "CSIDL_PERSONAL").
To be used with Sys_DefaultSavePath(), so savegames, screenshots etc will be
saved to the users files instead of systemwide.

Based on (with kind permission) Yamagi Quake II's Sys_GetHomeDir()

Returns the number of characters written to dst
==============
 */
 // blendo eric: windows "known" folders for save sorted by priority
enum saveWindowsKnownFolders_t
{
	WKF_DEFAULT = 0,
	WKF_APPDATA_ROAMING = 0,
	WKF_MYDOCS,
	WKF_APPDATA_LOCAL,
	WKF_MAX
};
const int SAVE_WKF_WINIDS[3] = {
	CSIDL_APPDATA,
	CSIDL_PERSONAL,
	CSIDL_LOCAL_APPDATA
};
const char* SAVE_WKF_SUB_PATHS[3] = {
	"/skindeep",
	"/My Games/skindeep",
	"/skindeep"
};

static int GetHomeDir( char *dst, size_t size, saveWindowsKnownFolders_t knownFolder = WKF_DEFAULT )
{
	if (knownFolder >= WKF_MAX || knownFolder < 0) { return 0;  } // unknown folder, return length 0

	int len;
	WCHAR profile[MAX_OSPATH];
	SHGetFolderPathW(NULL, SAVE_WKF_WINIDS[knownFolder], NULL, 0, profile);

	len = WPath2A(dst, size, profile);
	if (len == 0)
		return 0;

	ULARGE_INTEGER minBytes;
	minBytes.QuadPart = win32.win_mindiskspace.GetInteger();
	minBytes.QuadPart *= 1024 * 1024;

	ULARGE_INTEGER bytesFree;
	GetDiskFreeSpaceEx(dst, &bytesFree, nullptr, nullptr);

	if (bytesFree.QuadPart < minBytes.QuadPart)
	{
		idStr errorMsg = idStr::Format("Insufficient disk space for My Documents path %s. Skin deep requires at least %d MB free.", dst, win32.win_mindiskspace.GetInteger());
		MessageBox(NULL, errorMsg.c_str(), "Fatal Error", MB_OK | MB_ICONERROR);
		exit(1);
	}

	idStr::Append( dst, size, SAVE_WKF_SUB_PATHS[knownFolder] );

#if STEAM
	idStr steamID = common->g_SteamUtilities->GetSteamID();
	if (steamID.Length() > 0)
	{
		idStr::Append(dst, size, "/");
		idStr::Append(dst, size, steamID.c_str());
	}
#endif
	return strlen(dst);
}

static int GetRegistryPath(char *dst, size_t size, const WCHAR *subkey, const WCHAR *name) {
	WCHAR w[MAX_OSPATH];
	DWORD len = sizeof(w);
	HKEY res;
	DWORD sam = KEY_QUERY_VALUE
#ifdef _WIN64
		| KEY_WOW64_32KEY
#endif
		;
	DWORD type;

	if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, subkey, 0, sam, &res) != ERROR_SUCCESS)
		return 0;

	if (RegQueryValueExW(res, name, NULL, &type, (LPBYTE)w, &len) != ERROR_SUCCESS) {
		RegCloseKey(res);
		return 0;
	}

	RegCloseKey(res);

	if (type != REG_SZ)
		return 0;

	return WPath2A(dst, size, w);
}

bool Sys_GetPath(sysPath_t type, idStr &path) {
	char buf[MAX_OSPATH];
	struct _stat st;
	idStr s;

	switch(type) {
	case PATH_BASE:
		// try <path to exe>/base first
		if (Sys_GetPath(PATH_EXE, path)) {
			path.StripFilename();

			s = path;
#ifdef DEMO
			// We can't change BASE_GAMEDIR but in the demo we won't have a base
			s.AppendPath("basedemo");
#else
			s.AppendPath(BASE_GAMEDIR);
#endif
			if (_stat(s.c_str(), &st) != -1 && st.st_mode & _S_IFDIR)
				return true;

			common->Warning("base path '%s' does not exist", s.c_str());
		}

		//BC don't do fallbacks to retail/steam installations.

		// fallback to vanilla doom3 cd install
		//if (GetRegistryPath(buf, sizeof(buf), L"SOFTWARE\\id\\Doom 3", L"InstallPath") > 0) {
		//	path = buf;
		//	return true;
		//}
		//
		//// fallback to steam doom3 install
		//if (GetRegistryPath(buf, sizeof(buf), L"SOFTWARE\\Valve\\Steam", L"InstallPath") > 0) {
		//	path = buf;
		//	path.AppendPath("steamapps\\common\\doom 3");
		//
		//	if (_stat(path.c_str(), &st) != -1 && st.st_mode & _S_IFDIR)
		//		return true;
		//}

		common->Warning("vanilla doom3 path not found");

		return false;

	case PATH_CONFIG:
	case PATH_SAVE:

	#if 0
		// use only default save folder
		if (GetHomeDir(buf, sizeof(buf), knownFolder) < 1) {
			Sys_Error("ERROR: Couldn't get dir to home path");
			return false;
		}

		path = buf;
		return true;
	#else
		// blendo eric: test if the path is actually useable, or fallback to others
		// (a bit messy including filesystem in here, but it had the function needed)
		{
			for (int knownFolder = WKF_DEFAULT; knownFolder < WKF_MAX; knownFolder++) {
				if (GetHomeDir(buf, sizeof(buf), (saveWindowsKnownFolders_t)knownFolder) > 0) {
					idStr tempPath = idStr(buf);
					tempPath.ReplaceChar( '/', PATHSEPERATOR_CHAR );
					tempPath.StripTrailing( PATHSEPERATOR_CHAR );
					tempPath += PATHSEPERATOR_CHAR;
					if ( !fileSystem->CreateOSPath( tempPath.c_str() ) ) {
						common->Warning("Cannot access potential save folder: %s", buf);
						continue;
					}
					if (type == PATH_SAVE) {
						common->Printf("Using save folder: %s\n", buf);
					}
					path = buf;
					return true;
				}
			}
			Sys_Error("FATAL ERROR:\n\nSkin Deep was unable to access any potential save folder.\nThis may be because of anti-virus or firewall software. For solutions, please visit: https://blendogames.com/skindeep/support.htm");
			return false;
		}
	#endif

	case PATH_EXE:
		GetModuleFileName(NULL, buf, sizeof(buf) - 1);
		path = buf;
		path.BackSlashesToSlashes();
		return true;
	}

	return false;
}

/*
==============
Sys_ListFiles
==============
*/
int Sys_ListFiles( const char *directory, const char *extension, idStrList &list ) {
	idStr		search;
	struct _finddata_t findinfo;
	intptr_t	findhandle;
	int			flag;

	if ( !extension) {
		extension = "";
	}

	// passing a slash as extension will find directories
	if ( extension[0] == '/' && extension[1] == 0 ) {
		extension = "";
		flag = 0;
	} else {
		flag = _A_SUBDIR;
	}

	sprintf( search, "%s\\*%s", directory, extension );

	// search
	list.Clear();

	findhandle = _findfirst( search, &findinfo );
	if ( findhandle == -1 ) {
		return -1;
	}

	do {
		if ( flag ^ ( findinfo.attrib & _A_SUBDIR ) ) {
			list.Append( findinfo.name );
		}
	} while ( _findnext( findhandle, &findinfo ) != -1 );

	_findclose( findhandle );

	return list.Num();
}


/*
================
Sys_GetClipboardData
================
*/
char *Sys_GetClipboardData( void ) {
	char *data = NULL;
	char *cliptext;

	if ( OpenClipboard( NULL ) != 0 ) {
		HANDLE hClipboardData;

		if ( ( hClipboardData = GetClipboardData( CF_TEXT ) ) != 0 ) {
			if ( ( cliptext = (char *)GlobalLock( hClipboardData ) ) != 0 ) {
				data = (char *)Mem_Alloc( GlobalSize( hClipboardData ) + 1 );
				strcpy( data, cliptext );
				GlobalUnlock( hClipboardData );

				strtok( data, "\n\r\b" );
			}
		}
		CloseClipboard();
	}
	return data;
}

/*
================
Sys_SetClipboardData
================
*/
void Sys_SetClipboardData( const char *string ) {
	HGLOBAL HMem;
	char *PMem;

	// allocate memory block
	HMem = (char *)::GlobalAlloc( GMEM_MOVEABLE | GMEM_DDESHARE, strlen( string ) + 1 );
	if ( HMem == NULL ) {
		return;
	}
	// lock allocated memory and obtain a pointer
	PMem = (char *)::GlobalLock( HMem );
	if ( PMem == NULL ) {
		return;
	}
	// copy text into allocated memory block
	lstrcpy( PMem, string );
	// unlock allocated memory
	::GlobalUnlock( HMem );
	// open Clipboard
	if ( !OpenClipboard( 0 ) ) {
		::GlobalFree( HMem );
		return;
	}
	// remove current Clipboard contents
	EmptyClipboard();
	// supply the memory handle to the Clipboard
	SetClipboardData( CF_TEXT, HMem );
	HMem = 0;
	// close Clipboard
	CloseClipboard();
}

/*
========================================================================

DLL Loading

========================================================================
*/

/*
=====================
Sys_DLL_Load
=====================
*/
uintptr_t Sys_DLL_Load( const char *dllName ) {
	HINSTANCE	libHandle;
	libHandle = LoadLibrary( dllName );
	if ( libHandle ) {
		// since we can't have LoadLibrary load only from the specified path, check it did the right thing
		char loadedPath[ MAX_OSPATH ];
		GetModuleFileName( libHandle, loadedPath, sizeof( loadedPath ) - 1 );
		if ( idStr::IcmpPath( dllName, loadedPath ) ) {
			Sys_Printf( "ERROR: LoadLibrary '%s' wants to load '%s'\n", dllName, loadedPath );
			Sys_DLL_Unload( (uintptr_t)libHandle );
			return 0;
		}
	}
	return (uintptr_t)libHandle;
}

/*
=====================
Sys_DLL_GetProcAddress
=====================
*/
void *Sys_DLL_GetProcAddress( uintptr_t dllHandle, const char *procName ) {
	return (void *)GetProcAddress( (HINSTANCE)dllHandle, procName );
}

/*
=====================
Sys_DLL_Unload
=====================
*/
void Sys_DLL_Unload( uintptr_t dllHandle ) {
	if ( !dllHandle ) {
		return;
	}
	if ( FreeLibrary( (HINSTANCE)dllHandle ) == 0 ) {
		int lastError = GetLastError();
		LPVOID lpMsgBuf;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER,
		    NULL,
			lastError,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
			(LPTSTR) &lpMsgBuf,
			0,
			NULL
		);
		Sys_Error( "Sys_DLL_Unload: FreeLibrary failed - %s (%d)", lpMsgBuf, lastError );
	}
}

/*
================
Sys_Init

The cvar system must already be setup
================
*/
void Sys_Init( void ) {

	CoInitialize( NULL );

	// make sure the timer is high precision, otherwise
	// NT gets 18ms resolution
	timeBeginPeriod( 1 );

	// get WM_TIMER messages pumped every millisecond
//	SetTimer( NULL, 0, 100, NULL );

#ifdef DEBUG
	cmdSystem->AddCommand( "createResourceIDs", CreateResourceIDs_f, CMD_FL_TOOL, "assigns resource IDs in _resouce.h files" );
#endif
#if 0
	cmdSystem->AddCommand( "setAsyncSound", Sys_SetAsyncSound_f, CMD_FL_SYSTEM, "set the async sound option" );
#endif

	//
	// Windows version
	//
	win32.osversion.dwOSVersionInfoSize = sizeof( win32.osversion );

	if ( !GetVersionEx( (LPOSVERSIONINFO)&win32.osversion ) )
		Sys_Error( "Couldn't get OS info" );

	if ( win32.osversion.dwMajorVersion < 4 ) {
		Sys_Error( GAME_NAME " requires Windows version 4 (NT) or greater" );
	}
	if ( win32.osversion.dwPlatformId == VER_PLATFORM_WIN32s ) {
		Sys_Error( GAME_NAME " doesn't run on Win32s" );
	}

	common->Printf( "%d MB System Memory\n", Sys_GetSystemRam() );
}

/*
================
Sys_Shutdown
================
*/
void Sys_Shutdown( void ) {
	CoUninitialize();
}

//=======================================================================

//#define SET_THREAD_AFFINITY


/*
====================
Win_Frame
====================
*/
void Win_Frame( void ) {
	// if "viewlog" has been modified, show or hide the log console
	if ( win32.win_viewlog.IsModified() ) {
		if ( !com_skipRenderer.GetBool() && idAsyncNetwork::serverDedicated.GetInteger() != 1 ) {
			Sys_ShowConsole( win32.win_viewlog.GetInteger(), false );
		}
		win32.win_viewlog.ClearModified();
	}
}

// ==========================================================================================
// SM: Added code to catch exceptions and generate a stack trace in both the log and a dialog
static mz_bool AddFileToZip( mz_zip_archive* zip, const char* fileName )
{
	FILE* file = fopen( fileName, "rb" );
	if ( !file )
	{
		return MZ_FALSE;
	}

	fclose( file );
	return mz_zip_writer_add_file( zip, fileName, fileName, nullptr, 0, 9 );
}

idStr outputMsg;

static _EXCEPTION_POINTERS* crashExPtr = nullptr;
BOOL CALLBACK CrashHandlerProc( HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam )
{
	switch ( message )
	{
	case WM_INITDIALOG:
		{
			HWND hwndOwner;
			RECT rc, rcDlg, rcOwner;
			// Get the owner window and dialog box rectangles. 
			if ( ( hwndOwner = GetParent( hwndDlg ) ) == NULL )
			{
				hwndOwner = GetDesktopWindow();
			}

			GetWindowRect( hwndOwner, &rcOwner );
			GetWindowRect( hwndDlg, &rcDlg );
			CopyRect( &rc, &rcOwner );

			// Offset the owner and dialog box rectangles so that right and bottom 
			// values represent the width and height, and then offset the owner again 
			// to discard space taken up by the dialog box. 

			OffsetRect( &rcDlg, -rcDlg.left, -rcDlg.top );
			OffsetRect( &rc, -rc.left, -rc.top );
			OffsetRect( &rc, -rcDlg.right, -rcDlg.bottom );

			// The new position is the sum of half the remaining space and the owner's 
			// original position. 

			SetWindowPos( hwndDlg,
				HWND_TOP,
				rcOwner.left + ( rc.right / 2 ),
				rcOwner.top + ( rc.bottom / 2 ),
				0, 0,          // Ignores size arguments. 
				SWP_NOSIZE );

			// Setup the stack frame message
			idStr* inMsg = ( idStr* )lParam;
			inMsg->Replace( "\n", "\r\n" );
			SetDlgItemText( hwndDlg, IDC_STACK, inMsg->c_str() );

			FILE* crashFile = fopen( "crashinfo.txt", "w" );
			fprintf( crashFile, inMsg->c_str() );
			fclose( crashFile );

			SendDlgItemMessage(hwndDlg, IDC_ERRORICON, STM_SETICON, (WPARAM)LoadIcon( nullptr, IDI_ERROR ), 0);

			if (common && common->g_SteamUtilities && common->g_SteamUtilities->IsOnSteamDeck())
			{
				HWND breakButton = GetDlgItem(hwndDlg, IDC_BREAK);
				ShowWindow(breakButton, SW_HIDE);

				HWND dumpButton = GetDlgItem(hwndDlg, IDC_DUMP);
				ShowWindow(dumpButton, SW_HIDE);
			}
			return TRUE;
		}
	case WM_COMMAND:
		switch ( LOWORD( wParam ) )
		{
		case IDC_BREAK:
		{
			__debugbreak();
			return TRUE;
		}
		case IDC_COPY:
		{
			idStr crashStr = sessLocal.GetSanitizedURLArgument(outputMsg);
			idStr locationStr = sessLocal.GetPlayerLocationString();
			idStr URL = idStr::Format("https://docs.google.com/forms/d/e/1FAIpQLScejbJ0SFfkHb0iWhFagXAikLwnyHOSie-n7tHUtFheFQ6oiQ/viewform?usp=dialog&entry.561524408=%s&entry.1770058426=%s", locationStr.c_str(), crashStr.c_str());
			if (common && common->g_SteamUtilities && common->g_SteamUtilities->IsOnSteamDeck())
			{
				common->g_SteamUtilities->OpenSteamOverlaypage(URL.c_str());
			}
			else
			{
				ShellExecute(nullptr, nullptr, URL.c_str(), nullptr, nullptr, SW_SHOW);
			}

			return TRUE;
		}
		case IDC_DUMP:
		{
			// Create the crash dump file
			HANDLE dmpFile = CreateFile( "skindeep.dmp", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
				FILE_ATTRIBUTE_NORMAL, NULL );
			MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
			dumpInfo.ThreadId = GetCurrentThreadId();
			dumpInfo.ExceptionPointers = crashExPtr;
			dumpInfo.ClientPointers = TRUE;

			int dumpFlags = MiniDumpNormal
				| MiniDumpWithIndirectlyReferencedMemory
				| MiniDumpWithDataSegs
				| MiniDumpWithHandleData
				| MiniDumpWithUnloadedModules
				| MiniDumpWithProcessThreadData
				| MiniDumpWithThreadInfo
				| MiniDumpIgnoreInaccessibleMemory;
			
			MiniDumpWriteDump( GetCurrentProcess(), GetCurrentProcessId(), dmpFile, ( MINIDUMP_TYPE )dumpFlags,
				&dumpInfo, NULL, NULL );
			CloseHandle( dmpFile );

			// Get the current time
			time_t currTime = std::time( nullptr );
			char dateStr[256];
			strftime( dateStr, 256, "%F-%H-%M-%S", std::localtime( &currTime ) );

			// Have to save the original CWD as the path will change after the save file dialog
			char originalCWD[1024];
			GetCurrentDirectory( 1024, originalCWD );

			// Create a save file dialog
			const int MAX_ZIP_NAME = 1024;
			char fileName[MAX_ZIP_NAME];
			
			idStr::snPrintf( fileName, MAX_ZIP_NAME, "SkinDeep-Crash-%s.zip", dateStr );
			OPENFILENAME openInfo;
			ZeroMemory( &openInfo, sizeof( openInfo ) );
			openInfo.lStructSize = sizeof( OPENFILENAME );
			openInfo.hwndOwner = hwndDlg;
			openInfo.lpstrFilter = "ZIP file (*.zip)\0*.ZIP\0";
			openInfo.lpstrFile = fileName;
			openInfo.nMaxFile = MAX_ZIP_NAME;
			if ( GetSaveFileName( &openInfo ) )
			{
				IProgressDialog *pd;
				HRESULT hr;
				hr = CoCreateInstance( CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER,
					IID_IProgressDialog, ( LPVOID * )( &pd ) );
				pd->SetTitle( L"Saving crash dump..." );
				pd->StartProgressDialog( hwndDlg, NULL,
					PROGDLG_MODAL | PROGDLG_NOTIME | PROGDLG_NOMINIMIZE | PROGDLG_NOCANCEL | PROGDLG_MARQUEEPROGRESS,
					NULL );
				// Set back to original CWD
				SetCurrentDirectory( originalCWD );

				std::thread zipThread( [&openInfo, pd, hwndDlg]() {
					mz_zip_archive zip;
					memset( &zip, 0, sizeof( zip ) );
					mz_bool success = MZ_TRUE;
					idStr errorMsg;
					if ( mz_zip_writer_init_file( &zip, openInfo.lpstrFile, 0 ) )
					{
						success &= AddFileToZip( &zip, "crashinfo.txt" );
						if (!success && errorMsg.Length() == 0)
							errorMsg = "Failed to add crashinfo.txt to zip";
						
						success &= AddFileToZip( &zip, "skindeep.pdb" );
						if (!success && errorMsg.Length() == 0)
							errorMsg = "Failed to add skindeep.pdb to zip";
						
						success &= AddFileToZip( &zip, "skindeep.dmp" );
						if (!success && errorMsg.Length() == 0)
							errorMsg = "Failed to add skindeep.dmp to zip";
						
						success &= AddFileToZip( &zip, "skindeep.exe" );
						if (!success && errorMsg.Length() == 0)
							errorMsg = "Failed to add skindeep.exe to zip";
						
						success &= mz_zip_writer_finalize_archive( &zip );
						if (!success && errorMsg.Length() == 0)
							errorMsg = "Failed to finalize zip archive";
						
						success &= mz_zip_writer_end( &zip );
						if (!success && errorMsg.Length() == 0)
							errorMsg = "Failed to close zip file writer";
					}
					else
					{
						errorMsg = idStr::Format("Could not create zip file: '%s'", openInfo.lpstrFile);
						success = MZ_FALSE;
					}

					pd->StopProgressDialog();
					pd->Release();

					if (!success)
					{
						idStr msgBox = idStr::Format("Failed to save crash dump with error: %s. Try to SAVE CRASH DUMP again.", errorMsg.c_str());
						MessageBox(hwndDlg, msgBox.c_str(), NULL, MB_OK);
					}
				});

				zipThread.detach();
			}
			return TRUE;
		}
		case IDOK:
		case IDCANCEL:
			EndDialog( hwndDlg, wParam );
			return TRUE;
		}
	}
	return FALSE;
}

int SEH_Filter( _EXCEPTION_POINTERS* ex, bool isMainThread = true )
{
	static const int MAX_STACK_COUNT = 64;
	void* stack[MAX_STACK_COUNT];
	unsigned short frames;
	SYMBOL_INFO* symbol;
	HANDLE process;

	crashExPtr = ex;

	disableAssertPrintf = true;

	idCVar* versionCvar = cvarSystem->Find( "g_version" );
	outputMsg += "Build: ";
	outputMsg += versionCvar->GetString();
	outputMsg += '\n';

	outputMsg += "==================FATAL ERROR====================\n";
	outputMsg += idStr::Format( "UNHANDLED EXCEPTION: 0x%X\n", ex->ExceptionRecord->ExceptionCode );
	outputMsg += idStr::Format("OCCURRED AT: %s\n", Sys_TimeStampToStr(Sys_GetTime()));

	process = GetCurrentProcess();

	SymInitialize( process, NULL, TRUE );

	frames = CaptureStackBackTrace( 0, 100, stack, NULL );
	symbol = ( SYMBOL_INFO* )calloc( sizeof( SYMBOL_INFO ) + 256 * sizeof( char ), 1 );
	symbol->MaxNameLen = 255;
	symbol->SizeOfStruct = sizeof( SYMBOL_INFO );

	IMAGEHLP_LINE64* line = ( IMAGEHLP_LINE64* )malloc( sizeof( IMAGEHLP_LINE64 ) );
	DWORD displacement;

	outputMsg += idStr::Format( "===================CALL STACK====================\n" );
	idStrList stackFrames;
	for ( int i = 1; i < frames; i++ )
	{
		SymFromAddr( process, ( DWORD64 )( stack[i] ), 0, symbol );

		memset( line, 0, sizeof( IMAGEHLP_LINE64 ) );
		line->SizeOfStruct = sizeof( IMAGEHLP_LINE64 );
		if ( SymGetLineFromAddr64( process, ( DWORD64 )( stack[i] ), &displacement, line ) )
		{
			stackFrames.Append( idStr::Format( "%i: %s - %s:%lu\n", frames - i - 1, symbol->Name, line->FileName, line->LineNumber ));
		}
		else
		{
			stackFrames.Append( idStr::Format( "%i: %s - 0x%0llX\n", frames - i - 1, symbol->Name, symbol->Address ) );
		}
	}

	// Try to see if we can find the exception handler frame and omit everything above it
	int userExceptionIdx = -1;
	for ( int i = 0; i < stackFrames.Num(); i++ )
	{
		if ( stackFrames[i].Find( "KiUserExceptionDispatcher" ) != -1 )
		{
			userExceptionIdx = i;
			break;
		}
	}

	for ( int i = userExceptionIdx + 1; i < stackFrames.Num(); i++ )
	{
		outputMsg += stackFrames[i];
	}

	outputMsg += "===================RECENT LOG====================\n";

	outputMsg += console->GetLastLines(win_dumploglines.GetInteger(), true);

	outputMsg += "====================HARDWARE=====================\n";

	idStr cpuVendor, cpuBrand;
	idLib::sys->GetCPUInfo( cpuVendor, cpuBrand );

	outputMsg += idStr::Format( "CPU: %s; %s\n", cpuVendor.c_str(), cpuBrand.c_str() );

	outputMsg += idStr::Format( "Total Memory (MB): %d\n", SDL_GetSystemRAM() );

	outputMsg += idStr::Format( "GPU: %s; %s; %s\n", glConfig.vendor_string, glConfig.renderer_string, glConfig.version_string );

	outputMsg += "=================================================\n";

	// If this handler gets called from the non-main thread, using common->Printf may hang
	if ( isMainThread ) {
		common->Printf( outputMsg.c_str() );

		// Close the game window
		GLimp_Shutdown();
	}

	PlaySound( "SystemExclamation", NULL, SND_ALIAS );

	// Show dialog box
	DialogBoxParam( GetModuleHandle( NULL ), MAKEINTRESOURCE( IDD_CRASHHANDLER ), GetActiveWindow(), ( DLGPROC )CrashHandlerProc, ( LPARAM )&outputMsg );

	free( symbol );
	free( line );

	return EXCEPTION_EXECUTE_HANDLER;
}

// SM: End code for stack traces/catch exceptions
// ==========================================================================================

// SM: High DPI aware code from newer version of dhewm3
typedef enum D3_PROCESS_DPI_AWARENESS {
	D3_PROCESS_DPI_UNAWARE = 0,
	D3_PROCESS_SYSTEM_DPI_AWARE = 1,
	D3_PROCESS_PER_MONITOR_DPI_AWARE = 2
} D3_PROCESS_DPI_AWARENESS;


static void setHighDPIMode(void)
{
	/* For Vista, Win7 and Win8 */
	BOOL(WINAPI * SetProcessDPIAware)(void) = NULL;


	/* Win8.1 and later */
	HRESULT(WINAPI * SetProcessDpiAwareness)(D3_PROCESS_DPI_AWARENESS dpiAwareness) = NULL;


	HINSTANCE userDLL = LoadLibrary("USER32.DLL");


	if (userDLL)
	{
		SetProcessDPIAware = (BOOL(WINAPI*)(void)) GetProcAddress(userDLL, "SetProcessDPIAware");
	}


	HINSTANCE shcoreDLL = LoadLibrary("SHCORE.DLL");


	if (shcoreDLL)
	{
		SetProcessDpiAwareness = (HRESULT(WINAPI*)(D3_PROCESS_DPI_AWARENESS))
			GetProcAddress(shcoreDLL, "SetProcessDpiAwareness");
	}


	if (SetProcessDpiAwareness) {
		SetProcessDpiAwareness(D3_PROCESS_PER_MONITOR_DPI_AWARE);
	}
	else if (SetProcessDPIAware) {
		SetProcessDPIAware();
	}
}

/*
==================
WinMain
==================
*/
int main(int argc, char *argv[]) {
__try{ // SM: Use __try/__except to catch unhandled exceptions
	//const HCURSOR hcurSave = ::SetCursor( LoadCursor( 0, IDC_WAIT ) );

#ifdef ID_DEDICATED
	MSG msg;
#else
	setHighDPIMode();
#endif

	Sys_SetPhysicalWorkMemory( 192 << 20, 1024 << 20 );

	win32.hInstance = GetModuleHandle(NULL);

	// done before Com/Sys_Init since we need this for error output
	Sys_CreateConsole();

	// no abort/retry/fail errors
	SetErrorMode( SEM_FAILCRITICALERRORS );

#ifdef DEBUG
	// disable the painfully slow MS heap check every 1024 allocs
	_CrtSetDbgFlag( 0 );
#endif

	if ( argc > 1 ) {
		common->Init( argc-1, &argv[1] );
	} else {
		common->Init( 0, NULL );
	}

	// hide or show the early console as necessary
	if ( win32.win_viewlog.GetInteger() || com_skipRenderer.GetBool() || idAsyncNetwork::serverDedicated.GetInteger() ) {
		Sys_ShowConsole( 1, true );
	} else {
		Sys_ShowConsole( 0, false );
	}

#ifdef SET_THREAD_AFFINITY
	// give the main thread an affinity for the first cpu
	SetThreadAffinityMask( GetCurrentThread(), 1 );
#endif

	// ::SetCursor( hcurSave ); // DG: I think SDL handles the cursor fine..

	// Launch the script debugger
	if ( strstr( GetCommandLine(), "+debugger" ) ) {
		// DebuggerClientInit( lpCmdLine );
		return 0;
	}

	// ::SetFocus( win32.hWnd ); // DG: let SDL handle focus, otherwise input is fucked up! (#100)

	// main game loop
	while( 1 ) {
#if ID_DEDICATED
		// Since this is a Dedicated Server, process all Windowing Messages
		// Now.
		while(PeekMessage (&msg, NULL, 0, 0, PM_REMOVE)){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// Give the OS a little time to recuperate.
		Sleep(10);
#endif

		Win_Frame();

#ifdef ID_ALLOW_TOOLS
		if ( com_editors ) {
			if ( com_editors & EDITOR_GUI ) {
				// GUI editor
				GUIEditorRun();
			} else if ( com_editors & EDITOR_RADIANT ) {
				// Level Editor
				RadiantRun();
			}
			else if (com_editors & EDITOR_MATERIAL ) {
				//BSM Nerve: Add support for the material editor
				MaterialEditorRun();
			}
			else {
				if ( com_editors & EDITOR_LIGHT ) {
					// in-game Light Editor
					LightEditorRun();
				}
				if ( com_editors & EDITOR_SOUND ) {
					// in-game Sound Editor
					SoundEditorRun();
				}
				if ( com_editors & EDITOR_DECL ) {
					// in-game Declaration Browser
					DeclBrowserRun();
				}
				if ( com_editors & EDITOR_AF ) {
					// in-game Articulated Figure Editor
					AFEditorRun();
				}
				if ( com_editors & EDITOR_PARTICLE ) {
					// in-game Particle Editor
					ParticleEditorRun();
				}
				if ( com_editors & EDITOR_SCRIPT ) {
					// in-game Script Editor
					ScriptEditorRun();
				}
				if ( com_editors & EDITOR_PDA ) {
					// in-game PDA Editor
					PDAEditorRun();
				}
			}
		}
#endif
		// run the game
		common->Frame();
	}
}
__except( SEH_Filter( GetExceptionInformation() ) ) // SM: Use __try/__except to catch unhandled exceptions
{
	__debugbreak();
}
	// never gets here
	return 0;
}

/*
==================
idSysLocal::OpenURL
==================
*/
void idSysLocal::OpenURL( const char *url, bool doexit ) {
	static bool doexit_spamguard = false;
	HWND wnd;

	if (doexit_spamguard) {
		common->DPrintf( "OpenURL: already in an exit sequence, ignoring %s\n", url );
		return;
	}

	common->Printf("Open URL: %s\n", url);

	if ( !ShellExecute( NULL, "open", url, NULL, NULL, SW_RESTORE ) ) {
		common->Error( "Could not open url: '%s' ", url );
		return;
	}

	wnd = GetForegroundWindow();
	if ( wnd ) {
		ShowWindow( wnd, SW_MAXIMIZE );
	}

	if ( doexit ) {
		doexit_spamguard = true;
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
	}
}

/*
==================
idSysLocal::StartProcess
==================
*/
void idSysLocal::StartProcess( const char *exePath, bool doexit ) {
	TCHAR				szPathOrig[_MAX_PATH];
	STARTUPINFO			si;
	PROCESS_INFORMATION	pi;

	ZeroMemory( &si, sizeof(si) );
	si.cb = sizeof(si);

	strncpy( szPathOrig, exePath, _MAX_PATH );

	if( !CreateProcess( NULL, szPathOrig, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi ) ) {
		common->Error( "Could not start process: '%s' ", szPathOrig );
	    return;
	}

	if ( doexit ) {
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
	}
}
