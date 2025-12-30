/*
** filesys.cpp
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
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
**
*/

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define USE_WINDOWS_DWORD
#define USE_WINDOWS_BOOLEAN
#include <windows.h>
#include <direct.h>
#include <shlobj.h>
#else
#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#include <sys/attr.h>
#include <sys/mount.h>
#include <mach-o/dyld.h>
#endif
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#endif

#include <sys/stat.h>
#include <cstdio>
#include "filesys.h"
#include "version.h"
#include "zstring.h"

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

namespace FileSys {

static FString SpecialPaths[NUM_SPECIAL_DIRECTORIES];

FString GetDirectoryPath(ESpecialDirectory dir) { return SpecialPaths[dir]; }
void SetDirectoryPath(ESpecialDirectory dir, const FString &path) { SpecialPaths[dir] = path; }

#ifdef _WIN32
// Utility functions for converting to and from wchar_t since we want to use
// UTF8 so that the majority of our file system code is platform independent.
static inline void ConvertName(const char* filename, wchar_t* out, const int len=MAX_PATH)
{
	MultiByteToWideChar(CP_UTF8, 0, filename, -1, out, len);
}
static inline void ConvertName(const wchar_t* filename, char* out, const int len=MAX_PATH)
{
	WideCharToMultiByte(CP_UTF8, 0, filename, -1, out, len, NULL, NULL);
}

#ifdef _M_X64
static const bool IsWinNT = true;
#else
static bool IsWinNT = true;
#endif
#endif

// Converts a relative filename to an absolute name
static void FullFileName(const char* filename, char* dest)
{
#ifdef _WIN32
	wchar_t path[MAX_PATH], fullpath[MAX_PATH];
	ConvertName(filename, path);
	_wfullpath(fullpath, path, MAX_PATH);
	ConvertName(fullpath, dest);
#else
	if(realpath(filename, dest) == NULL)
		strncpy(dest, filename, MAX_PATH);
#endif
}

static bool CreateDirectoryIfNeeded(const char* path)
{
#ifdef _WIN32
	struct _stat dirStat;
	if(IsWinNT)
	{
		wchar_t wpath[MAX_PATH];
		ConvertName(path, wpath);
		if(_wstat(wpath, &dirStat) == -1)
		{
			if(_wmkdir(wpath) == -1)
				return false;
		}
	}
	else
	{
		if(_stat(path, &dirStat) == -1)
		{
			if(_mkdir(path) == -1)
				return false;
		}
	}
	return true;
#else
	struct stat dirStat;
	if(stat(path, &dirStat) == -1)
	{
		if(mkdir(path, S_IRWXU) == -1)
			return false;
	}
	return true;
#endif
}

#ifndef LIBRETRO
void SetupPaths(int argc, const char * const *argv)
{
	FString &progDir = SpecialPaths[DIR_Program];
	FString &configDir = SpecialPaths[DIR_Configuration];
	FString &saveDir = SpecialPaths[DIR_Saves];
	FString &appsupportDir = SpecialPaths[DIR_ApplicationSupport];
	FString &documentsDir = SpecialPaths[DIR_Documents];
	FString &screenshotsDir = SpecialPaths[DIR_Screenshots];

	// Setup platform specific folder location functions
#if defined(_WIN32)
	static const GUID gFOLDERID_RoamingAppData = {0x3EB685DBu, 0x65F9u, 0x4CF6u, {0xA0u,0x3Au,0xE3u,0xEFu,0x65u,0x72u,0x9Fu,0x3Du}};
	static const GUID gFOLDERID_SavedGames = {0x4C5C32FFu, 0xBB9Du, 0x43b0u, {0xB5u,0xB4u,0x2Du,0x72u,0xE5u,0x4Eu,0xAAu,0xA4u}};

	HRESULT (WINAPI* pSHGetKnownFolderPath)(const GUID*,DWORD,HANDLE,PWSTR*) = NULL;
	HRESULT (WINAPI* pSHGetFolderPathW)(HWND,int,HANDLE,DWORD,LPWSTR) = NULL;
	HMODULE shell32 = LoadLibrary ("shell32");
	if(shell32)
	{
		pSHGetKnownFolderPath = (HRESULT (WINAPI*)(const GUID*,DWORD,HANDLE,PWSTR*))GetProcAddress(shell32, "SHGetKnownFolderPath");
		pSHGetFolderPathW = (HRESULT (WINAPI*)(HWND,int,HANDLE,DWORD,LPWSTR))GetProcAddress(shell32, "SHGetFolderPathW");
	}

#ifndef _M_X64
	OSVERSIONINFO osVersion;
	ZeroMemory(&osVersion, sizeof(OSVERSIONINFO));
	osVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&osVersion);
	IsWinNT = osVersion.dwPlatformId > VER_PLATFORM_WIN32_WINDOWS;
#endif
#endif

	// Find the program directory.
#if defined(_WIN32)
	if(IsWinNT)
	{
		char tempCName[MAX_PATH];
		wchar_t tempName[MAX_PATH];
		memset(tempCName, 0, MAX_PATH);
		memset(tempName, 0, MAX_PATH*sizeof(wchar_t));
		if(SUCCEEDED(GetModuleFileNameW(NULL, tempName, MAX_PATH)))
		{
			ConvertName(tempName, tempCName);
			progDir = tempCName;
		}
		else
			progDir = argv[0];
	}
	else
	{
		char tempName[MAX_PATH];
		memset(tempName, 0, MAX_PATH);
		if(SUCCEEDED(GetModuleFileNameA(NULL, tempName, MAX_PATH)))
			progDir = tempName;
		else
			progDir = argv[0];
	}
#elif defined(__APPLE__)
	{
		char binpath[MAX_PATH];
		char binrealpath[MAX_PATH];
		uint32_t binlen = MAX_PATH;
		if(_NSGetExecutablePath(binpath, &binlen) == 0 && realpath(binpath, binrealpath) != NULL)
			progDir = binrealpath;
		else
			progDir = argv[0];
	}
#elif defined(_DIII4A) //karin: using cwd as HOME
    char home[1024];
    getcwd(home, sizeof(home));
    progDir = home;
#else
	{
		char linkbuf[MAX_PATH];
		ssize_t linklen;
		if((linklen = readlink("/proc/self/exe", linkbuf, MAX_PATH)) > 0)
		{
			linkbuf[linklen] = 0;
			progDir = linkbuf;
		}
		else
			progDir = argv[0];
	}
#endif
	int pos = progDir.LastIndexOfAny("/\\");
	if(pos != -1)
		progDir = progDir.Mid(0, pos);
	else
		progDir = ".";

	// Configuration directory
#if defined(_WIN32)
	if(IsWinNT)
	{
		if(pSHGetKnownFolderPath) // Vista+
		{
			char tempCPath[MAX_PATH];
			memset(tempCPath, 0, MAX_PATH);
			PWSTR tempPath = NULL;
			if(SUCCEEDED(pSHGetKnownFolderPath(&gFOLDERID_RoamingAppData, 0x00008000, NULL, &tempPath)))
			{
				ConvertName(tempPath, tempCPath);
				configDir.Format("%s\\" GAME_DIR, (const char*)tempCPath);
				CoTaskMemFree(tempPath);
			}
		}
		if(configDir.IsEmpty() && pSHGetFolderPathW)
		{
			// Other Windows NT
			char tempCPath[MAX_PATH];
			wchar_t tempPath[MAX_PATH];
			memset(tempCPath, 0, MAX_PATH);
			memset(tempPath, 0, MAX_PATH*sizeof(wchar_t));
			if(SUCCEEDED(pSHGetFolderPathW(NULL, CSIDL_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, tempPath)))
			{
				ConvertName(tempPath, tempCPath);
				configDir.Format("%s\\" GAME_DIR, (const char*)tempCPath);
				CoTaskMemFree(tempPath);
			}
		}
	}
	if(configDir.IsEmpty())
	{
		wchar_t *home = _wgetenv(L"APPDATA");
		if(home == NULL || *home == '\0')
		{
			// No APPDATA then use the program directory (Should mean Windows 9x)
			printf("APPDATA environment variable not set, falling back.\n");
			configDir = progDir;
		}
		else
		{
			char* chome = new char[wcslen(home)];
			ConvertName(home, chome, (int)wcslen(home));
			configDir.Format("%s\\" GAME_DIR, chome);
			delete[] chome;
		}
	}
#elif defined(__APPLE__)
	FString osxDir = OSX_FindFolder(DIR_Configuration);
	if(osxDir.IsEmpty())
	{
		I_Error("Could not create your preferences files.\n");
	}
	configDir = osxDir;
#elif defined(_DIII4A) //karin: using cwd as HOME
    configDir.Format("%s/" GAME_DIR, home);
#else
	char *home = getenv("HOME");
	char *xdg_config = getenv("XDG_CONFIG_HOME");
	if(xdg_config == NULL || *xdg_config == '\0')
	{	
		if(home == NULL || *home == '\0')
		{
			I_Error("Please set your HOME environment variable.\n");
		}
		configDir.Format("%s/.config/" GAME_DIR, home);
	}
	else
		configDir.Format("%s/" GAME_DIR, xdg_config);
#endif

	if(!CreateDirectoryIfNeeded(configDir))
		printf("Could not create settings directory, configuration will not be saved.\n");

	// Documents
#if defined(_WIN32)
	documentsDir = configDir;
#elif defined(__APPLE__)
	osxDir = OSX_FindFolder(DIR_Documents);
	if(!osxDir.IsEmpty())
		documentsDir = osxDir + "/" GAME_DIR;
#elif defined(_DIII4A) //karin: using cwd as HOME
    documentsDir.Format("%s/" GAME_DIR, home);
#else
	char *xdg_data = getenv("XDG_DATA_HOME");
	if(xdg_data == NULL || *xdg_data == '\0')
	{
		if(home == NULL || *home == '\0')
		{
			I_Error("Please set your HOME environment variable.\n");
		}
		documentsDir.Format("%s/.local/share/" GAME_DIR, home);
	}
	else
		documentsDir.Format("%s/" GAME_DIR, xdg_data);
#endif

	if(!CreateDirectoryIfNeeded(documentsDir))
		documentsDir = configDir;

	// Saved games
#if defined(_WIN32)
	if(pSHGetKnownFolderPath) // Vista+
	{
		char tempCPath[MAX_PATH];
		PWSTR tempPath = NULL;
		if(SUCCEEDED(pSHGetKnownFolderPath(&gFOLDERID_SavedGames, 0x00008000, NULL, &tempPath)))
		{
			ConvertName(tempPath, tempCPath);
			saveDir.Format("%s\\" GAME_DIR, (const char*)tempCPath);
			CoTaskMemFree(tempPath);
		}
	}
	if(saveDir.IsEmpty())
		saveDir = configDir;
#elif defined(__APPLE__)
	saveDir = documentsDir + "/Savegames";
#else
	saveDir = documentsDir + "/savegames";
#endif

	if(!CreateDirectoryIfNeeded(saveDir))
		saveDir = configDir; // Can't make saved games directory? Fall back to config directory.

	// Application support directory
#if defined(__APPLE__)
	osxDir = OSX_FindFolder(DIR_ApplicationSupport);
	if(!osxDir.IsEmpty())
		appsupportDir = osxDir + "/" GAME_DIR;
#else
	appsupportDir = progDir;
#endif

	if(!CreateDirectoryIfNeeded(appsupportDir))
		appsupportDir = progDir;

	// Screenshots directory
#if defined(_WIN32)
	screenshotsDir = configDir;
#elif defined(__APPLE__)
	screenshotsDir = documentsDir + "/Screenshots";
#else
	screenshotsDir = documentsDir + "/screenshots";
#endif

	if(!CreateDirectoryIfNeeded(screenshotsDir))
		screenshotsDir = configDir;
}
#endif

}

static TMap<unsigned int, FString> VirtualRenameTable;
// The reverse table was setup for the GOG version of Spear of Destiny.
// It seems awfully fragile in comparison to the above.
static TMap<unsigned int, FString> VirtualRenameReverse;

File::File(const FString &filename)
{
	init(filename);
}

File::File(const File &dir, const FString &filename)
{
	init(dir.getDirectory() + PATH_SEPARATOR + filename);
}

void File::init(FString filename)
{
	this->filename = filename;
	directory = false;
	existing = false;
	writable = false;

	// Are we trying to reference a renamed file?
	if(FString *fname = VirtualRenameTable.CheckKey(MakeKey(filename)))
		filename = *fname;

#ifdef _WIN32
	if(FileSys::IsWinNT)
	{
		wchar_t wname[MAX_PATH];
		FileSys::ConvertName(filename, wname);
		DWORD fAttributes = GetFileAttributesW(wname);
		if(fAttributes != INVALID_FILE_ATTRIBUTES)
		{
			existing = true;
			if(fAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				directory = true;
				WIN32_FIND_DATAW fdata;
				HANDLE hnd = INVALID_HANDLE_VALUE;
				FileSys::ConvertName(filename + "\\*", wname);
				hnd = FindFirstFileW(wname, &fdata);
				if(hnd != INVALID_HANDLE_VALUE)
				{
					do
					{
						char fname[MAX_PATH];
						FileSys::ConvertName(fdata.cFileName, fname);
						files.Push(fname);
					}
					while(FindNextFileW(hnd, &fdata) != 0);
				}
			}
		}
	}
	else
	{
		DWORD fAttributes = GetFileAttributesA(filename);
		if(fAttributes != INVALID_FILE_ATTRIBUTES)
		{
			existing = true;
			if(fAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				directory = true;
				WIN32_FIND_DATA fdata;
				HANDLE hnd = INVALID_HANDLE_VALUE;
				hnd = FindFirstFileA((filename + "\\*").GetChars(), &fdata);
				if(hnd != INVALID_HANDLE_VALUE)
				{
					do
					{
						files.Push(fdata.cFileName);
					}
					while(FindNextFileA(hnd, &fdata) != 0);
				}
			}
		}
	}

	// Can't find an easy way to test writability on Windows so
	writable = true;
#else
	struct stat statRet;
	if(stat(filename, &statRet) == 0)
		existing = true;

	if(existing)
	{
		if((statRet.st_mode & S_IFDIR))
		{
			directory = true;

			// Populate a base list.
			DIR *direct = opendir(filename);
			if(direct != NULL)
			{
				dirent *file = NULL;
				while((file = readdir(direct)) != NULL)
					files.Push(file->d_name);
			}
			closedir(direct);
		}

		// Check writable
		if(access(filename, W_OK) == 0)
			writable = true;
	}
#endif

	// Check for any virtual renames
	for(unsigned int i = 0;i < files.Size();++i)
	{
		FString *renamed = VirtualRenameReverse.CheckKey(MakeKey(getDirectory() + PATH_SEPARATOR + files[i]));
		if(renamed)
			files[i] = *renamed;
	}
}

FString File::getDirectory() const
{
	if(directory)
	{
		if(filename[filename.Len()-1] == '\\' || filename[filename.Len()-1] == '/')
			return filename.Left(filename.Len()-1);
		return filename;
	}

	long dirSepPos = filename.LastIndexOfAny("/\\");
	if(dirSepPos != -1)
		return filename.Left(dirSepPos);
	return FString(".");
}

FString File::getFileName() const
{
	if(directory)
		return FString();

	long dirSepPos = filename.LastIndexOfAny("/\\");
	if(dirSepPos != -1)
		return filename.Mid(dirSepPos+1);
	return filename;
}

FString File::getInsensitiveFile(const FString &filename, bool sensitiveExtension) const
{
#ifdef _WIN32
	// Insensitive filesystem, so just return the filename
	return filename;
#else
#ifdef __APPLE__
	{
		// Mac supports both case insensitive and sensitive file systems
		attrlist pathAttr = {
			ATTR_BIT_MAP_COUNT, 0,
			0, ATTR_VOL_CAPABILITIES, 0, 0, 0
		};
		struct statfs fs;
		struct
		{
			u_int32_t length;
			vol_capabilities_attr_t cap;
		} __attribute__((aligned(4), packed)) vol;

		int attrError = -1;
		if(statfs(this->filename, &fs) == 0)
			attrError = getattrlist(fs.f_mntonname, &pathAttr, &vol, sizeof(vol), 0);
		if(attrError || !(vol.cap.capabilities[0] & vol.cap.valid[0] & VOL_CAP_FMT_CASE_SENSITIVE))
		{
			// We assume case insensitive (since it's the default) unless the volcap tells us otherwise
			return filename;
		}
	}
#endif

	const TArray<FString> &files = getFileList();
	FString extension = filename.Mid(filename.LastIndexOf('.')+1);

	for(unsigned int i = 0;i < files.Size();++i)
	{
		if(files[i].CompareNoCase(filename) == 0)
		{
			if(!sensitiveExtension || files[i].Mid(files[i].LastIndexOf('.')+1).Compare(extension) == 0)
				return files[i];
		}
	}
	return filename;
#endif
}

/**
 * Open a file while handling any non-English character sets and
 * respecting our virtual renaming table.
 */
FILE *File::open(const char* mode) const
{
	char path[MAX_PATH];
	FileSys::FullFileName(filename, path);
	FString *renamed = VirtualRenameTable.CheckKey(MakeKey(path));
	FString fn = renamed ? *renamed : filename;

#ifdef _WIN32
	if(FileSys::IsWinNT)
	{
		wchar_t wname[MAX_PATH], wmode[MAX_PATH];
		FileSys::ConvertName(fn, wname);
		FileSys::ConvertName(mode, wmode);
		return _wfopen(wname, wmode);
	}
#endif
	return fopen(fn, mode);
}

/**
 * Perform a virtual rename on the file. This function is designed to handle
 * renaming improperly set copies of Spear of Destiny. Since we can't assume
 * that the user can physically rename the file, we need to do the remapping
 * in memory. We could just load the SD1 files, but that may be undesirable
 * for save compatibility and/or multiplayer.
 */
void File::rename(const FString &newname)
{
	char path[MAX_PATH];
	FileSys::FullFileName(filename, path);
	filename = path;

	VirtualRenameReverse[MakeKey(filename)] = newname;
	VirtualRenameTable[MakeKey(getDirectory() + PATH_SEPARATOR + newname)] = filename;
}

bool File::remove()
{
	char path[MAX_PATH];
	FileSys::FullFileName(filename, path);
	FString *renamed = VirtualRenameTable.CheckKey(MakeKey(path));
	FString fn = renamed ? *renamed : filename;
#ifdef _WIN32
	if(FileSys::IsWinNT)
	{
		wchar_t wname[MAX_PATH];
		FileSys::ConvertName(fn, wname);
		return DeleteFileW(wname) != 0;
	}
	else
		return DeleteFileA(fn) != 0;
#else
	return unlink(fn) == 0;
#endif
}
