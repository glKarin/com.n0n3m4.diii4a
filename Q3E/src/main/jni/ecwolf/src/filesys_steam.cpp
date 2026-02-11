/*
** filesys_steam.cpp
**
**---------------------------------------------------------------------------
** Copyright 2013 Braden Obrzut
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
#undef ERROR
#endif
#include <sys/stat.h>

#include "doomerrors.h"
#include "filesys.h"
#include "scanner.h"

namespace FileSys {

#ifdef _WIN32
#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & _S_IFMT) == _S_IFDIR)
#endif

/*
** Bits and pieces
**
**---------------------------------------------------------------------------
** Copyright 1998-2009 Randy Heit
** All rights reserved.
**
** License conditions same as BSD above.
**---------------------------------------------------------------------------
**
*/
//==========================================================================
//
// QueryPathKey
//
// Returns the value of a registry key into the output variable value.
//
//==========================================================================

static bool QueryPathKey(HKEY key, const char *keypath, const char *valname, FString &value)
{
	HKEY steamkey;
	DWORD pathtype;
	DWORD pathlen;
	LONG res;

	if(ERROR_SUCCESS == RegOpenKeyEx(key, keypath, 0, KEY_QUERY_VALUE, &steamkey))
	{
		if (ERROR_SUCCESS == RegQueryValueEx(steamkey, valname, 0, &pathtype, NULL, &pathlen) &&
			pathtype == REG_SZ && pathlen != 0)
		{
			// Don't include terminating null in count
			char *chars = value.LockNewBuffer(pathlen - 1);
			res = RegQueryValueEx(steamkey, valname, 0, NULL, (LPBYTE)chars, &pathlen);
			value.UnlockBuffer();
			if (res != ERROR_SUCCESS)
			{
				value = "";
			}
		}
		RegCloseKey(steamkey);
	}
	return value.IsNotEmpty();
}

// Previously Steam only allowed one steam library based on where Steam was installed
static FString QuerySteamappsPath()
{
	//==========================================================================
	//
	// I_GetSteamPath
	//
	// Check the registry for the path to Steam, so that we can search for
	// IWADs that were bought with Steam.
	//
	//==========================================================================

	FString path;
	if (!QueryPathKey(HKEY_CURRENT_USER, "Software\\Valve\\Steam", "SteamPath", path))
	{
		if (!QueryPathKey(HKEY_LOCAL_MACHINE, "Software\\Valve\\Steam", "InstallPath", path))
			path = "";
	}
	if (!path.IsEmpty())
		path += "\\SteamApps";

	return path;
}
#endif

static void PSR_FindEndBlock(Scanner &sc)
{
	int depth = 1;
	do
	{
		if(sc.CheckToken('}'))
			--depth;
		else if(sc.CheckToken('{'))
			++depth;
		else
			sc.GetNextToken();
	}
	while(depth);
}
static void PSR_SkipBlock(Scanner &sc)
{
	sc.MustGetToken('{');
	PSR_FindEndBlock(sc);
}
static bool PSR_FindAndEnterBlock(Scanner &sc, const char* keyword)
{
	// Finds a block with a given keyword and then enter it (opening brace)
	// Should be closed with PSR_FindEndBlock
	while(sc.TokensLeft())
	{
		if(sc.CheckToken('}'))
		{
			sc.Rewind();
			return false;
		}

		sc.MustGetToken(TK_StringConst);
		if(sc->str.CompareNoCase(keyword) != 0)
		{
			if(!sc.CheckToken(TK_StringConst))
				PSR_SkipBlock(sc);
		}
		else
		{
			sc.MustGetToken('{');
			return true;
		}
	}
	return false;
}

#ifndef _WIN32
static TArray<FString> PSR_ReadBaseInstalls(Scanner &sc)
{
	TArray<FString> result;

	// Get a list of possible install directories.
	while(sc.TokensLeft())
	{
		if(sc.CheckToken('}'))
			break;

		sc.MustGetToken(TK_StringConst);
		FString key(sc->str);
		if(key.Left(18).CompareNoCase("BaseInstallFolder_") == 0)
		{
			sc.MustGetToken(TK_StringConst);
			result.Push(sc->str + "/steamapps/common");
		}
		else
		{
			if(sc.CheckToken('{'))
				PSR_FindEndBlock(sc);
			else
				sc.MustGetToken(TK_StringConst);
		}
	}

	return result;
}
static TArray<FString> ParseSteamRegistry(const char* path)
{
	TArray<FString> dirs;

	char* data;
	long size;

	// Read registry data
	FILE* registry = fopen(path, "rb");
	if(!registry)
		return dirs;

	fseek(registry, 0, SEEK_END);
	size = ftell(registry);
	fseek(registry, 0, SEEK_SET);
	data = new char[size];
	fread(data, 1, size, registry);
	fclose(registry);

	Scanner sc(data, size);
	delete[] data;

	// Find the SteamApps listing
	if(PSR_FindAndEnterBlock(sc, "InstallConfigStore"))
	{
		if(PSR_FindAndEnterBlock(sc, "Software"))
		{
			if(PSR_FindAndEnterBlock(sc, "Valve"))
			{
				if(PSR_FindAndEnterBlock(sc, "Steam"))
				{
					dirs = PSR_ReadBaseInstalls(sc);
				}
				PSR_FindEndBlock(sc);
			}
			PSR_FindEndBlock(sc);
		}
		PSR_FindEndBlock(sc);
	}

	return dirs;
}
#else
static TArray<FString> ParseSteamLibraryFolders(const char* path)
{
	TArray<FString> dirs;

	char* data;
	long size;

	// Read registry data
	FILE* registry = fopen(path, "rb");
	if(!registry)
		return dirs;

	fseek(registry, 0, SEEK_END);
	size = ftell(registry);
	fseek(registry, 0, SEEK_SET);
	data = new char[size];
	fread(data, 1, size, registry);
	fclose(registry);

	Scanner sc(data, size);
	delete[] data;

	// Find the SteamApps listing
	if(PSR_FindAndEnterBlock(sc, "libraryfolders"))
	{
		while(!sc.CheckToken('}'))
		{
			sc.MustGetToken(TK_StringConst);
			sc.MustGetToken('{');
			while(sc.TokensLeft())
			{
				if(sc.CheckToken('}'))
					break;

				sc.MustGetToken(TK_StringConst);
				FString key(sc->str);
				if(key.CompareNoCase("path") == 0)
				{
					sc.MustGetToken(TK_StringConst);
					dirs.Push(sc->str + "\\steamapps\\common");
				}
				else
				{
					if(sc.CheckToken('{'))
						PSR_FindEndBlock(sc);
					else
						sc.MustGetToken(TK_StringConst);
				}
			}
		}
	}

	return dirs;
}
#endif

static TArray<FString> GetSteamLibraryFolders()
{
	TArray<FString> SteamInstallFolders;

#if defined(_WIN32)
	FString SteamappsPath = QuerySteamappsPath();
	if (SteamappsPath.IsEmpty())
		return TArray<FString>();

	try
	{
		SteamInstallFolders = ParseSteamLibraryFolders(SteamappsPath + "\\libraryfolders.vdf");
	}
	catch (class CDoomError& error)
	{
		// Ignore since we have a fallback
	}

	if (SteamInstallFolders.Size() == 0)
		SteamInstallFolders.Push(SteamappsPath + "\\common");
	return SteamInstallFolders;
#elif defined(__APPLE__)
	FString appSupportPath = OSX_FindFolder(DIR_ApplicationSupport);
	FString regPath = appSupportPath + "/Steam/config/config.vdf";
	try
	{

		SteamInstallFolders = ParseSteamRegistry(regPath);
	}
	catch (class CDoomError& error)
	{
		// If we can't parse for some reason just pretend we can't find anything.
		return TArray<FString>();
	}

	SteamInstallFolders.Push(appSupportPath + "/Steam/SteamApps/common");
#else
	char* home = getenv("HOME");
	if (home != NULL && *home != '\0')
	{
		FString regPath;
		regPath.Format("%s/.local/share/Steam/config/config.vdf", home);
		try
		{
			SteamInstallFolders = ParseSteamRegistry(regPath);
		}
		catch (class CDoomError& error)
		{
			// If we can't parse for some reason just pretend we can't find anything.
			return TArray<FString>();
		}

		regPath.Format("%s/.local/share/Steam/SteamApps/common", home);
		SteamInstallFolders.Push(regPath);
	}
#endif

	return SteamInstallFolders;
}

static TMap<int, FString> SteamAppInstallPath;
FString GetSteamPath(ESteamApp game)
{
	static struct SteamAppInfo
	{
		const char* const BasePath;
		const int AppID;
	} AppInfo[NUM_STEAM_APPS] =
	{
		{"Wolfenstein 3D", 2270},
		{"Spear of Destiny", 9000},
		{"The Apogee Throwback Pack", 238050},
		{"Super 3-D Noah's Ark", 371180},
		{"Blake Stone Aliens of Gold", 358190},
		{"Blake Stone Planet Strike", 358310},
		{"Rise of the Triad Dark War", 358410},
		{"Corridor 7", 1341890},
		{"Operation Body Count", 1627120}
	};

	// Linux and OS X actually allow the user to install to any location, so
	// we need to figure out on an app-by-app basis where the game is installed.
	// To do so, we read the virtual registry.
	if(SteamAppInstallPath.CountUsed() == 0)
	{
		TArray<FString> SteamInstallFolders = GetSteamLibraryFolders();

		for(unsigned int i = 0;i < SteamInstallFolders.Size();++i)
		{
			for(unsigned int app = 0;app < countof(AppInfo);++app)
			{
				struct stat st;
				FString candidate(SteamInstallFolders[i] + PATH_SEPARATOR + AppInfo[app].BasePath);
				if(stat(candidate, &st) == 0 && S_ISDIR(st.st_mode))
					SteamAppInstallPath[AppInfo[app].AppID] = candidate;
			}
		}
	}
	const FString *installPath = SteamAppInstallPath.CheckKey(AppInfo[game].AppID);
	if(installPath)
		return *installPath;
	return FString();
}

FString GetGOGPath(ESteamApp game)
{
	static struct SteamAppInfo
	{
		const char* const AppID[3]; // NULL terminated list of up to two ids
		const char* const MacFolder;
		const char* const LinuxFolder;
		const char* const LinuxXdgApp;
	} AppInfo[NUM_STEAM_APPS] =
	{
		// Wolfenstein 3D
		// 1778410505 = New release with Spear of Destiny
		// 1441705046 = Old stand-alone release
		{{"1778420505", "1441705046"}, NULL, NULL, NULL},
		{{"1441705126"}, NULL, NULL, NULL}, // Spear of Destiny
		{{NULL}, NULL, NULL, NULL}, // Throwback Pack
		{{"1672565562"}, NULL, NULL, NULL}, // Super 3D Noah's Ark
		{{"1207658728"}, "Blake Stone - Aliens of Gold.app", "Blake Stone Aliens of Gold", "gog_com-Blake_Stone_Aliens_of_Gold_1.desktop"}, // Blake Stone: Aliens of Gold
		{{"1207658729"}, "Blake Stone Planet Strike.app", "Blake Stone Planet Strike", "gog_com-Blake_Stone_Planet_Strike_1.desktop"}, // Blake Stone: Planet Strike
		{{"1207658732"}, "Rise of the Triad Dark War.app", "Rise of the Triad Dark War", "gog_com-Rise_of_the_Triad_Dark_War_1.desktop"}, // Rise of the Triad: Dark War
		{{"2147483140"}, "Corridor 7 Alien Invasion", NULL, NULL}, // Corridor 7: Alien Invasion
		{{"1718217391"}, "Operation Body Count", NULL, NULL} // Operation Body Count
	};

	if(AppInfo[game].AppID[0] == NULL)
		return FString();

#if defined(_WIN32)
	FString path;


	//==========================================================================
	//
	// I_GetGogPaths
	//
	// Check the registry for GOG installation paths, so we can search for IWADs
	// that were bought from GOG.com. This is a bit different from the Steam
	// version because each game has its own independent installation path, no
	// such thing as <steamdir>/SteamApps/common/<GameName>.
	//
	//==========================================================================

#ifdef _WIN64
	FString gogregistrypath = "Software\\Wow6432Node\\GOG.com\\Games";
#else
	// If a 32-bit ZDoom runs on a 64-bit Windows, this will be transparently and
	// automatically redirected to the Wow6432Node address instead, so this address
	// should be safe to use in all cases.
	FString gogregistrypath = "Software\\GOG.com\\Games";
#endif

	for(const char* const* id = AppInfo[game].AppID; *id; ++id)
	{
		if(QueryPathKey(HKEY_LOCAL_MACHINE, gogregistrypath + PATH_SEPARATOR + *id, "Path", path))
			return path;
	}
	return FString();
#elif defined(__APPLE__)
	/* The GOG macOS installers don't register themselves with pkgutil and they
	 * prompt the user to pick an install location in the postintall script. The
	 * only realistic way to find them would be to look in ~/Documents (the
	 * default location it prompts to) and /Applications.
	 *
	 * If the user is using GOG Galaxy, the installation information is stored in
	 * a sqlite database. /Users/Shared/GOG.com/Galaxy/config.json has the
	 * storagePath key and the galaxy-2.0.db file has the application id and path
	 * in the InstalledBaseProducts table.
	 *
	 * This is not yet implemented as the games available on macOS are not yet
	 * supported so not worth writing the detection code at this time.
	 */
	return FString();
#else
	/* On Linux GOG uses MojoSetup which doesn't have any central database. The
	 * default install location is "~/GOG Games". If the user chooses to install
	 * the menu item, then the XDG desktop file will be installed in the user's
	 * XDG applications directory. This could be used to trace a non-default
	 * location.
	 *
	 * This is not yet implemented as the games available for Linux are not yet
	 * supported so not worth writing the detection code at this time.
	 */
	return FString();
#endif
}

}
