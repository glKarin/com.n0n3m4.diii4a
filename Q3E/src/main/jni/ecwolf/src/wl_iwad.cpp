/*
** wl_iwad.cpp
**
**---------------------------------------------------------------------------
** Copyright 2012 Braden Obrzut
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

#include "resourcefiles/resourcefile.h"
#include "config.h"
#include "filesys.h"
#include "lumpremap.h"
#include "scanner.h"
#include "tarray.h"
#include "tmemory.h"
#include "version.h"
#include "w_wad.h"
#include "wl_iwad.h"
#include "zstring.h"

bool queryiwad = true;
static bool showpreviewgames = false;

int I_PickIWad(WadStuff *wads, int numwads, bool showwin, int defaultiwad);
#ifndef _WIN32
#include "wl_iwad_picker.cpp"
#endif

namespace IWad {

#if defined(_WIN32) || defined(__APPLE__) || defined(__ANDROID__)
#define ISCASEINSENSITIVE 1
#else
#define ISCASEINSENSITIVE 0
#endif

struct LevelSet
{
	FString FileName;
	int Type;
};

static TArray<IWadData> iwadTypes;
static TArray<FString> iwadNames;
static TArray<LevelSet> levelSets;
static const IWadData *selectedGame;
static unsigned int NumIWads;

static bool CheckIWadNotYetFound(const TArray<WadStuff> &iwads, int type)
{
	for(unsigned int i = 0; i < iwads.Size(); ++i)
	{
		if(iwads[i].Type == type)
			return false;
	}
	return true;
}

// Insert Paths from one WadStuff into another when Required is satisfied
static void TransferWadStuffPaths(WadStuff &dest, const WadStuff &src)
{
	for(unsigned int i = src.Path.Size();i-- > 0;)
		dest.Path.Insert(0, src.Path[i]);
}

static bool SplitFilename(const FString &filename, FString &name, FString &extension)
{
	long extensionSep = filename.LastIndexOf('.');
	if(extensionSep == -1 || filename.Len() < 3)
		return false;
	name = filename.Left(extensionSep);
	extension = filename.Mid(extensionSep+1);
	return true;
}

static int CheckFileContents(FResourceFile *file, unsigned int* valid)
{
	for(unsigned int j = file->LumpCount();j-- > 0;)
	{
		FResourceLump *lump = file->GetLump(j);

		for(unsigned int k = 0;k < iwadTypes.Size();++k)
		{
			for(unsigned int l = iwadTypes[k].Ident.Size();l-- > 0;)
			{
				if(iwadTypes[k].Ident[l].CompareNoCase(lump->Name) == 0 ||
					(lump->FullName && (strnicmp(lump->FullName, "maps/", 5) == 0 &&
					iwadTypes[k].Ident[l].CompareNoCase(FString(lump->FullName.Mid(5).GetChars(), strcspn(lump->FullName.Mid(5), "."))))))
				{
					valid[k] |= 1<<l;
				}
			}
		}
	}

	for(unsigned int k = 0;k < iwadTypes.Size();++k)
	{
		if(!iwadTypes[k].LevelSet && valid[k] == static_cast<unsigned>((1<<iwadTypes[k].Ident.Size())-1))
			return k;
	}

	return -1;
}

static bool CheckHidden(const IWadData &data)
{
	return ((data.Flags & IWad::PREVIEW) && !showpreviewgames) ||
			!!(data.Flags & IWad::RESOURCE);
}

// Identifies the IWAD by examining the lumps against the possible requirements.
// Returns -1 if it isn't identifiable.
static int CheckData(WadStuff &wad)
{
	TUniquePtr<unsigned int[]> valid(new unsigned int[iwadTypes.Size()]);
	memset(valid.Get(), 0, sizeof(unsigned int)*iwadTypes.Size());

	for(unsigned int i = 0;i < wad.Path.Size();++i)
	{
		TUniquePtr<FResourceFile> file(FResourceFile::OpenResourceFile(wad.Path[i], NULL, true));
		if(file)
		{
			LumpRemapper::RemapAll(); // Fix lump names if needed

			int type;
			if((type = CheckFileContents(file, valid)) >= 0)
			{
				wad.Type = type;
				wad.Name = iwadTypes[type].Name;
				wad.Hidden = CheckHidden(iwadTypes[type]);
				// Don't break here since we might want to refine our selection.
			}
		}
	}

	return wad.Type;
}

static bool CheckStandalone(const char* directory, FString filename, FString extension, TArray<WadStuff> &iwads)
{
	WadStuff wad;
	for(unsigned int i = 0;i < iwadNames.Size();++i)
	{
		if(filename.CompareNoCase(iwadNames[i]) != 0)
			continue;

		FString path;
		path.Format("%s/%s", directory, filename.GetChars());
		TUniquePtr<FResourceFile> file(FResourceFile::OpenResourceFile(path, NULL, true));
		if(file)
		{
			TUniquePtr<unsigned int[]> valid(new unsigned int[iwadTypes.Size()]);
			memset(valid.Get(), 0, sizeof(unsigned int)*iwadTypes.Size());

			if((wad.Type = CheckFileContents(file, valid)) >= 0 && CheckIWadNotYetFound(iwads, wad.Type))
			{
				wad.Path.Push(path);
				wad.Extension = extension;
				wad.Name = iwadTypes[wad.Type].Name;
				wad.Hidden = CheckHidden(iwadTypes[wad.Type]);
				iwads.Push(wad);
				return true;
			}
		}
		break;
	}
	return false;
}

/**
 * Locate level sets based on what required data sets we have available
 */
static void CheckForLevelSets(TArray<WadStuff> &iwads)
{
	for(unsigned int i = levelSets.Size();i-- > 0;)
	{
		const FString fileName = levelSets[i].FileName;
		const IWadData &lsType = iwadTypes[levelSets[i].Type];

		for(unsigned int r = 0;r < lsType.Required.Size();++r)
		{
			for(unsigned int j = 0;j < iwads.Size();++j)
			{
				if(iwadTypes[iwads[j].Type].Name.Compare(lsType.Required[r]) == 0)
				{
					// Find candidate in same directory as base data
					File baseFileDir(File(iwads[j].Path[0]).getDirectory());
					File candidateFile(baseFileDir, baseFileDir.getInsensitiveFile(fileName, false));
					if(!candidateFile.exists())
						continue;

					WadStuff wad;
					wad.Path.Push(candidateFile.getPath());
					TransferWadStuffPaths(wad, iwads[j]);

					FString dummy;
					SplitFilename(candidateFile.getPath(), dummy, wad.Extension);
					wad.Name = lsType.Name;
					wad.Type = levelSets[i].Type;
					wad.Hidden = CheckHidden(lsType);
					iwads.Push(wad);
					goto FinishLevelSet;
				}
			}
		}
	FinishLevelSet:;
	}
}

bool CheckGameFilter(FName filter)
{
	return selectedGame->Game == filter;
}

const IWadData &GetGame()
{
	return *selectedGame;
}

unsigned int GetNumIWads()
{
	return NumIWads;
}

enum
{
	FILE_AUDIOHED,
	FILE_AUDIOT,
	FILE_GAMEMAPS,
	FILE_MAPHEAD,
	FILE_VGADICT,
	FILE_VGAHEAD,
	FILE_VGAGRAPH,
	FILE_VSWAP,

	BASEFILES,

	FILE_REQMASK = (1<<BASEFILES)-1
};
struct BaseFile
{
	FString	extension;
	FString	filename[BASEFILES];
	BYTE	isValid;
};
/* Steam ships Spear of Destiny in the mission pack 3 state, so we need to go
 * and correct steam installs.
 *
 * Returns true if the install was fine (needs no correction)
 */
static bool VerifySpearInstall(const char* directory)
{
	const FString MissionFiles[3] =
	{
		"gamemaps.",
		"maphead.",
		"vswap."
	};

	const File dir(directory);

	// Check for gamemaps.sd1, if it doesn't exist assume we're good
	if(!File(dir, dir.getInsensitiveFile("gamemaps.sd1", false)).exists())
		return true;

	// Try to find what mission that the .sod files are.
	// If everything is present then we just need to worry about the sd1 files.
	int currentMission = 1;
	if(!File(dir, dir.getInsensitiveFile("gamemaps.sd3", false)).exists())
		currentMission = 3;
	else if(!File(dir, dir.getInsensitiveFile("gamemaps.sd2", false)).exists())
		currentMission = 2;
	else if(File(dir, dir.getInsensitiveFile("gamemaps.sod", false)).exists())
		currentMission = -1;

	Printf("Spear of Destiny is not set to the original mission. Attempting remap for files in: %s\n", directory);
	for(unsigned int i = 0;i < 3;++i)
	{
		File srcFile(dir, dir.getInsensitiveFile(MissionFiles[i] + "sod", false));
		File sd1File(dir, dir.getInsensitiveFile(MissionFiles[i] + "sd1", false));

		if(currentMission > 0)
			srcFile.rename(MissionFiles[i] + "sd" + (char)('0' + currentMission));
		sd1File.rename(MissionFiles[i] + "sod");
	}

	return false;
}

/* Find valid game data.  Due to the nature of WOlf3D we must collect
 * information by extensions.  An extension is considered valid if it has all
 * files needed.  If the OS is case sensitive then the case sensitivity only
 * applies to the extension itself.
 */
static void LookForGameData(FResourceFile *res, TArray<WadStuff> &iwads, const char* directory)
{
	static const unsigned int LoadableBaseFiles[] = { FILE_AUDIOT, FILE_GAMEMAPS, FILE_VGAGRAPH, FILE_VSWAP, BASEFILES };
	static const char* const BaseFileNames[BASEFILES][3] = {
		{"audiohed", NULL}, {"audiot", NULL},
		{"gamemaps", "maptemp", NULL}, {"maphead", NULL},
		{"vgadict", NULL}, {"vgahead", NULL}, {"vgagraph", NULL},
		{"vswap", NULL}
	};
	TArray<BaseFile> foundFiles;

	File dir(directory);
	if(!dir.exists())
		return;

	if(!VerifySpearInstall(directory))
		dir = File(directory); // Repopulate the file list

	TArray<FString> files = dir.getFileList();
	for(unsigned int i = 0;i < files.Size();++i)
	{
		FString name, extension;
		if(!SplitFilename(files[i], name, extension))
			continue;

		if(CheckStandalone(directory, files[i], extension, iwads))
			continue;

		BaseFile *base = NULL;
		for(unsigned int j = 0;j < foundFiles.Size();++j)
		{
			#if ISCASEINSENSITIVE
			if(foundFiles[j].extension.CompareNoCase(extension) == 0)
			#else
			if(foundFiles[j].extension.Compare(extension) == 0)
			#endif
			{
				base = &foundFiles[j];
				break;
			}
		}
		if(!base)
		{
			int index = foundFiles.Push(BaseFile());
			base = &foundFiles[index];
			base->extension = extension;
			base->isValid = 0;
		}

		unsigned int baseName = 0;
		do
		{
			for(const char* const * nameCheck = BaseFileNames[baseName];*nameCheck;++nameCheck)
			{
				if(name.CompareNoCase(*nameCheck) == 0)
				{
					base->filename[baseName].Format("%s" PATH_SEPARATOR "%s", directory, files[i].GetChars());
					base->isValid |= 1<<baseName;

					baseName = BASEFILES;
					break;
				}
			}
		}
		while(++baseName < BASEFILES);
	}

	for(unsigned int i = 0;i < foundFiles.Size();++i)
	{
		if(!foundFiles[i].isValid)
			continue;

		WadStuff wadStuff;
		wadStuff.Extension = foundFiles[i].extension;
		for(unsigned int j = 0;LoadableBaseFiles[j] != BASEFILES;++j)
		{
			if((foundFiles[i].isValid & (1<<LoadableBaseFiles[j])))
				wadStuff.Path.Push(foundFiles[i].filename[LoadableBaseFiles[j]]);
		}

		// Before checking the data we must load the remap file.
		FString mapFile;
		mapFile.Format("%sMAP", foundFiles[i].extension.GetChars());
		for(unsigned int j = res->LumpCount();j-- > 0;)
		{
			FResourceLump *lump = res->GetLump(j);
			if(mapFile.CompareNoCase(lump->Name) == 0)
				LumpRemapper::LoadMap(foundFiles[i].extension, lump->FullName, (const char*) lump->CacheLump(), lump->LumpSize);
		}

		if(CheckData(wadStuff) > -1)
		{
			// Check to ensure there are no duplicates
			if(CheckIWadNotYetFound(iwads, wadStuff.Type))
			{
				if(iwadTypes[wadStuff.Type].Required.Size() > 0 ||
					(foundFiles[i].isValid & FILE_REQMASK) == FILE_REQMASK)
				{
					iwads.Push(wadStuff);
				}
			}
		}
	}

	LumpRemapper::ClearRemaps();
}

/**
 * Remove any iwads which depend on another, but the other isn't present.
 */
static void CheckForExpansionRequirements(TArray<WadStuff> &iwads)
{
	for(unsigned int i = iwads.Size();i-- > 0;)
	{
		if(iwadTypes[iwads[i].Type].Required.Size() == 0)
			continue;

		bool reqSatisfied = false;
		for(unsigned int j = 0;!reqSatisfied && j < iwadTypes[iwads[i].Type].Required.Size();++j)
		{
			const FString &req = iwadTypes[iwads[i].Type].Required[j];
			if(req.IsNotEmpty())
			{
				for(unsigned int k = 0;k < iwads.Size();++k)
				{
					if(iwadTypes[iwads[k].Type].Name.Compare(req) == 0)
					{
						TransferWadStuffPaths(iwads[i], iwads[k]);
						reqSatisfied = true;
						break;
					}
				}
			}
		}

		if(!reqSatisfied)
			iwads.Delete(i);
	}
}

static IWadData ParseIWad(Scanner &sc)
{
	IWadData iwad = {};

	sc.MustGetToken('{');
	while(!sc.CheckToken('}'))
	{
		sc.MustGetToken(TK_Identifier);
		FString key = sc->str;
		sc.MustGetToken('=');
		if(key.CompareNoCase("Flags") == 0)
		{
			do
			{
				sc.MustGetToken(TK_Identifier);
				if(sc->str.CompareNoCase("HelpHack") == 0)
					iwad.Flags |= IWad::HELPHACK;
				else if(sc->str.CompareNoCase("Registered") == 0)
					iwad.Flags |= IWad::REGISTERED;
				else if(sc->str.CompareNoCase("Preview") == 0)
					iwad.Flags |= IWad::PREVIEW;
				else if(sc->str.CompareNoCase("Resource") == 0)
					iwad.Flags |= IWad::RESOURCE;
				else
					sc.ScriptMessage(Scanner::ERROR, "Unknown flag %s.", sc->str.GetChars());
			}
			while(sc.CheckToken(','));
		}
		else if(key.CompareNoCase("Game") == 0)
		{
			// This specifies a filter to be used for switching between things
			// like environment sounds.
			sc.MustGetToken(TK_StringConst);
			iwad.Game = sc->str;
		}
		else if(key.CompareNoCase("Name") == 0)
		{
			sc.MustGetToken(TK_StringConst);
			iwad.Name = sc->str;
		}
		else if(key.CompareNoCase("Autoname") == 0)
		{
			sc.MustGetToken(TK_StringConst);
			iwad.Autoname = sc->str;
		}
		else if(key.CompareNoCase("Mapinfo") == 0)
		{
			sc.MustGetToken(TK_StringConst);
			iwad.Mapinfo = sc->str;
		}
		else if(key.CompareNoCase("MustContain") == 0)
		{
			do
			{
				sc.MustGetToken(TK_StringConst);
				iwad.Ident.Push(sc->str);
			}
			while(sc.CheckToken(','));
		}
		else if(key.CompareNoCase("Required") == 0)
		{
			do
			{
				sc.MustGetToken(TK_StringConst);
				iwad.Required.Push(sc->str);
			}
			while(sc.CheckToken(','));
		}
	}

	return iwad;
}
static void ParseIWadInfo(FResourceFile *res)
{
	for(unsigned int i = res->LumpCount();i-- > 0;)
	{
		FResourceLump *lump = res->GetLump(i);

		if(lump->Namespace == ns_global && stricmp(lump->Name, "IWADINFO") == 0)
		{
			Scanner sc((const char*)lump->CacheLump(), lump->LumpSize);
			sc.SetScriptIdentifier(lump->FullName);

			while(sc.TokensLeft())
			{
				sc.MustGetToken(TK_Identifier);
				if(sc->str.CompareNoCase("IWad") == 0)
				{
					iwadTypes.Push(ParseIWad(sc));
				}
				else if(sc->str.CompareNoCase("LevelSet") == 0)
				{
					sc.MustGetToken(TK_StringConst);

					LevelSet ls;
					ls.FileName = sc->str;
					ls.Type = iwadTypes.Size();
					levelSets.Push(ls);

					IWadData iwad = ParseIWad(sc);
					iwad.LevelSet = true;
					iwadTypes.Push(iwad);
				}
				else if(sc->str.CompareNoCase("Names") == 0)
				{
					sc.MustGetToken('{');
					do
					{
						sc.MustGetToken(TK_StringConst);
						iwadNames.Push(sc->str);
					}
					while(!sc.CheckToken('}'));
				}
				else
					sc.ScriptMessage(Scanner::ERROR, "Unknown IWADINFO block '%s'.", sc->str.GetChars());
			}
			break;
		}
	}
}

void SelectGame(TArray<FString> &wadfiles, const char* iwad, const char* datawad, const FString &progdir)
{
	config.CreateSetting("DefaultIWad", 0);
	config.CreateSetting("ShowIWadPicker", 1);
	bool showPicker = config.GetSetting("ShowIWadPicker")->GetInteger() != 0;
	int defaultIWad = config.GetSetting("DefaultIWad")->GetInteger();

	if(config.GetSetting("ShowPreviewGames"))
		showpreviewgames = config.GetSetting("ShowPreviewGames")->GetInteger() != 0;

	FString datawadDir;
	FResourceFile *datawadRes = FResourceFile::OpenResourceFile(datawad, NULL, true);
	if(!datawadRes)
	{
		if((datawadRes = FResourceFile::OpenResourceFile(progdir + PATH_SEPARATOR + datawad, NULL, true)))
			datawadDir = progdir + PATH_SEPARATOR;
#if !defined(__APPLE__) && !defined(_WIN32)
		else if((datawadRes = FResourceFile::OpenResourceFile(FString(INSTALL_PREFIX "/share/" BINNAME "/") + datawad, NULL, true)))
			datawadDir = FString(INSTALL_PREFIX "/share/" BINNAME "/");
#endif
	}
	if(!datawadRes)
		I_Error("Could not open %s!", datawad);

	ParseIWadInfo(datawadRes);

	// Get a list of potential data paths
	FString dataPaths;
	if(config.GetSetting("BaseDataPaths") == NULL)
	{
		FString configDir = FileSys::GetDirectoryPath(FileSys::DIR_Configuration);
		dataPaths = ".;$PROGDIR";

		// On OS X our default config directory is ~/Library/Preferences which isn't a good place to put data at all.
#if !defined(__APPLE__)
		dataPaths += FString(";") + configDir;
#endif

		// Add documents and application support directories if they're not mapped to the config directory.
		FString tmp;
		if((tmp = FileSys::GetDirectoryPath(FileSys::DIR_Documents)).Compare(configDir) != 0)
			dataPaths += FString(";") + tmp;
		if((tmp = FileSys::GetDirectoryPath(FileSys::DIR_ApplicationSupport)).Compare(configDir) != 0)
			dataPaths += FString(";") + tmp;

		config.CreateSetting("BaseDataPaths", dataPaths);
	}
	dataPaths = config.GetSetting("BaseDataPaths")->GetString();

	TArray<WadStuff> basefiles;
	long split = 0;
	do
	{
		long newSplit = dataPaths.IndexOf(';', split);
		FString path = dataPaths.Mid(split, newSplit-split);
		// Check for environment variable
		if(path[0] == '$')
		{
			FString envvar = path.Mid(1);
			if(envvar.CompareNoCase("PROGDIR") == 0)
				path = progdir;
			else
			{
				char* value = getenv(envvar);
				if(value)
					path = value;
				else
					path = "";
			}
		}

		// Skip empty paths
		if(!path.IsEmpty())
			LookForGameData(datawadRes, basefiles, path);
		split = newSplit+1;
	}
	while(split != 0);

#if !defined(__APPLE__) && !defined(_WIN32)
	LookForGameData(datawadRes, basefiles, "/usr/share/games/wolf3d");
	LookForGameData(datawadRes, basefiles, "/usr/local/share/games/wolf3d");
#endif

	// Look for a steam install. (Basically from ZDoom)
	{
		struct CommercialGameDir
		{
			FileSys::ESteamApp app;
			const char* dir;
		};

		static const CommercialGameDir steamDirs[] =
		{
			{FileSys::APP_Wolfenstein3D, PATH_SEPARATOR "base"},
			{FileSys::APP_Wolfenstein3D, PATH_SEPARATOR "base" PATH_SEPARATOR "m1"},
			{FileSys::APP_SpearOfDestiny, PATH_SEPARATOR "base"},
			{FileSys::APP_NoahsArk, ""},
#if defined(__APPLE__)
			{FileSys::APP_ThrowbackPack, PATH_SEPARATOR "Blake Stone AOG.app/Contents/Resources/BlakestoneAOG"},
			{FileSys::APP_ThrowbackPack, PATH_SEPARATOR "Blake Stone PS.app/Contents/Resources/BlakestonePS"},
			// Note: There's also a Rise of the triad EX app but the DARKWAR.RTL is different in it, both have Extreme Rise of the Triad data in them anyway
			{FileSys::APP_ThrowbackPack, PATH_SEPARATOR "Rise of the triad.app/Contents/Resources/ROTT"},
			{FileSys::APP_AliensOfGold, PATH_SEPARATOR "Blake Stone AOG.app/Contents/Resources/BlakestoneAOG"},
			{FileSys::APP_PlanetStrike, PATH_SEPARATOR "Blake Stone PS.app/Contents/Resources/BlakestonePS"},
			{FileSys::APP_RiseOfTheTriad, PATH_SEPARATOR "Rise of the triad.app/Contents/Resources/ROTT"},
#else
			{FileSys::APP_ThrowbackPack, PATH_SEPARATOR "Blake Stone"},
			{FileSys::APP_ThrowbackPack, PATH_SEPARATOR "Planet Strike"},
			{FileSys::APP_ThrowbackPack, PATH_SEPARATOR "Rise of the Triad"},
			{FileSys::APP_AliensOfGold, PATH_SEPARATOR "Blake Stone - Aliens of Gold"},
			{FileSys::APP_PlanetStrike, PATH_SEPARATOR "Blake Stone - Planet Strike"},
			{FileSys::APP_RiseOfTheTriad, PATH_SEPARATOR "Rise of the Triad - Dark War"},
#endif
			// TODO: Corridor 7 isn't unpacked
			//{FileSys::APP_Corridor7, PATH_SEPARATOR "cd"},
			{FileSys::APP_OperationBodyCount, PATH_SEPARATOR "C" PATH_SEPARATOR "BCCD"}
		};
		for(unsigned int i = 0;i < countof(steamDirs);++i)
			LookForGameData(datawadRes, basefiles, FileSys::GetSteamPath(steamDirs[i].app) + steamDirs[i].dir);

		static const CommercialGameDir gogDirs[] = 
		{
			{FileSys::APP_Wolfenstein3D, ""},
			{FileSys::APP_Wolfenstein3D, PATH_SEPARATOR "m1"},
			{FileSys::APP_SpearOfDestiny, PATH_SEPARATOR "M1"},
			{FileSys::APP_NoahsArk, ""},
#if defined(_WIN32)
			{FileSys::APP_AliensOfGold, ""},
			{FileSys::APP_PlanetStrike, ""},
			{FileSys::APP_RiseOfTheTriad, ""},
#elif defined(__APPLE__)
			{FileSys::APP_AliensOfGold, PATH_SEPARATOR "Contents/Resources/game/Blake Stone Aliens of Gold.app/Contents/Resources/Blake Stone Aliens of Gold.boxer/C Blake Stone Aliens of Gold.harddisk"},
			{FileSys::APP_PlanetStrike, PATH_SEPARATOR "Contents/Resources/game/Blake Stone Planet Strike.app/Contents/Resources/Blake Stone Planet Strike.boxer/C Blake Stone Planet Strike.harddisk"},
			{FileSys::APP_RiseOfTheTriad, PATH_SEPARATOR "Contents/Resources/game/Rise of the Triad Dark War.app/Contents/Resources/Rise of the Triad Dark-War.boxer/C Rise of The Triad.harddisk"},
#else
			{FileSys::APP_AliensOfGold, PATH_SEPARATOR "data"},
			{FileSys::APP_PlanetStrike, PATH_SEPARATOR "data"},
			{FileSys::APP_RiseOfTheTriad, PATH_SEPARATOR "data"},
#endif
			// TODO: Corridor 7 isn't unpacked
			//{FileSys::APP_Corridor7, PATH_SEPARATOR "cd"},
			{FileSys::APP_OperationBodyCount, PATH_SEPARATOR "C" PATH_SEPARATOR "BCCD"}
		};
		for(unsigned int i = 0;i < countof(gogDirs);++i)
		{
			FString path = FileSys::GetGOGPath(gogDirs[i].app) + gogDirs[i].dir;
			LookForGameData(datawadRes, basefiles, path);

			// Find mission packs which GOG was so kind to remove the hack for.
			if(!path.IsEmpty() && gogDirs[i].app == FileSys::APP_SpearOfDestiny)
			{
				for(unsigned int mp = 2;mp <= 3;++mp)
				{
					File dir(path.Left(path.Len()-1) + char('0' + mp));
					TArray<FString> files = dir.getFileList();
					for(unsigned int f = 0;f < files.Size();++f)
					{
						if(files[f].Right(4).CompareNoCase(".SOD") == 0)
							File(dir, files[f]).rename(files[f].Left(files[f].Len()-4) + ".sd" + char('0' + mp));
					}
					LookForGameData(datawadRes, basefiles, dir.getDirectory());
				}
			}
		}
	}

	delete datawadRes;

	// Check requirements now as opposed to with LookForGameData so that reqs
	// don't get loaded multiple times.
	CheckForExpansionRequirements(basefiles);

	// Now search for any applicable level sets
	CheckForLevelSets(basefiles);

	// Remove hidden options
	for(unsigned int i = basefiles.Size();i-- > 0;)
	{
		if(basefiles[i].Hidden)
			basefiles.Delete(i);
	}

	if(basefiles.Size() == 0)
	{
		I_Error("Can not find base game data. (*.wl6, *.wl1, *.sdm, *.sod, *.n3d)");
	}

	int pick = -1;
	if(iwad)
	{
		for(unsigned int i = 0;i < basefiles.Size();++i)
		{
			#if ISCASEINSENSITIVE
			if(basefiles[i].Extension.CompareNoCase(iwad) == 0)
			#else
			if(basefiles[i].Extension.Compare(iwad) == 0)
			#endif
			{
				pick = i;
				break;
			}
		}
	}
	if(pick < 0)
	{
		if(basefiles.Size() > 1)
		{
			pick = I_PickIWad(&basefiles[0], basefiles.Size(), showPicker, defaultIWad);
			if(pick != -1 && (unsigned int) pick >= basefiles.Size()) // keep the pick within bounds
				pick = basefiles.Size()-1;
			config.GetSetting("ShowIWadPicker")->SetValue(queryiwad);
		}
		else
			pick = 0;
	}
	if(pick < 0)
		Quit();

	config.GetSetting("DefaultIWad")->SetValue(pick);

	WadStuff &base = basefiles[pick];
	selectedGame = &iwadTypes[base.Type];

	wadfiles.Push(datawadDir + datawad);
	for(unsigned int i = 0;i < base.Path.Size();++i)
	{
		wadfiles.Push(base.Path[i]);
	}

	NumIWads = base.Path.Size();

	// Load in config autoloads
	FString autoloadkey = FString("Autoload") + selectedGame->Autoname;
	for(long dot = 0, dotterm;dot < (long)autoloadkey.Len();dot = dotterm+1)
	{
		dotterm = autoloadkey.IndexOf('.', dot);
		if(dotterm == -1)
			dotterm = (long)autoloadkey.Len();

		FString autoload = autoloadkey.Mid(0, dotterm);
		autoload.StripChars('.');
		config.CreateSetting(autoload, "");
		autoload = config.GetSetting(autoload)->GetString();
		for(long i = 0, term;i < (long)autoload.Len();i = term+1)
		{
			term = autoload.IndexOf(';', i);
			if(term == -1)
				term = (long)autoload.Len();

			FString fname = autoload.Mid(i, term-i);
			wadfiles.Push(fname);
		}
	}
}

}
