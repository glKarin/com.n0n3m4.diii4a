/*
** wl_loadsave.cpp
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

#include <ctime>
#include "config.h"
#include "c_cvars.h"
#include "farchive.h"
#include "filesys.h"
#include "g_mapinfo.h"
#include "gamemap.h"
#include "id_ca.h"
#include "id_us.h"
#include "id_vh.h"
#include "language.h"
#include "m_classes.h"
#include "m_png.h"
#include "m_random.h"
#include "thinker.h"
#include "version.h"
#include "w_wad.h"
#include "wl_agent.h"
#include "wl_draw.h"
#include "wl_game.h"
#include "wl_inter.h"
#include "wl_iwad.h"
#include "wl_loadsave.h"
#include "wl_main.h"
#include "wl_menu.h"
#include "wl_net.h"
#include "wl_play.h"
#include "textures/textures.h"

void R_RenderView();
extern byte* vbuf;
extern unsigned vbufPitch;

namespace GameSave {

unsigned long long SaveVersion = GetSaveVersion();
DWORD SaveProdVersion = SAVEPRODVER;
bool param_foreginsave = false;

static const char* const NEW_SAVE = "    - NEW SAVE -";

struct SaveFile
{
	public:
		static TArray<SaveFile>	files;

		bool	hide;
		bool	hasFiles;
		bool	oldVersion;
		FString	name; // Displayed on the menu.
		FString	filename;
};
TArray<SaveFile> SaveFile::files;

static bool quickSaveLoad = false;

static inline FString GetFullSaveFileName(const FString &filename)
{
	FString dir = FileSys::GetDirectoryPath(FileSys::DIR_Saves);
	return dir + PATH_SEPARATOR + filename;
}
static inline FILE *OpenSaveFile(const FString &filename, const char* mode)
{
	return File(GetFullSaveFileName(filename)).open(mode);
}

#define MAX_SAVENAME 31
#define LSM_Y   55
#define LSM_W   175
#define LSM_X   (320-LSM_W-10)
#define LSM_H   10*13+10
#define LSA_X	96
#define LSA_Y	80
#define LSA_W	130
#define LSA_H	42

class SaveSlotMenuItem : public TextInputMenuItem
{
public:
	const unsigned int slotIndex;

	SaveSlotMenuItem(unsigned int slotIndex, const FString &text, unsigned int max, MENU_LISTENER_PROTOTYPE(preeditListener)=NULL, MENU_LISTENER_PROTOTYPE(posteditListener)=NULL, bool clearFirst=false)
		: TextInputMenuItem(text, max, preeditListener, posteditListener, clearFirst), slotIndex(slotIndex)
	{
	}

	void draw()
	{
		TextInputMenuItem::draw();

		// We use this instead of isSelected() so that the info is drawn during animations.
		if(menu->getIndex(menu->getCurrentPosition()) == this)
		{
			static const EColorRange textColor = gameinfo.FontColors[GameInfo::MENU_HIGHLIGHTSELECTION];
			static const unsigned int SAVEPICX = 10;
			static const unsigned int SAVEPICY = LSM_Y - 1;
			static const unsigned int SAVEPICW = 108;
			static const unsigned int SAVEPICH = 67;
			static const unsigned int INFOY = SAVEPICY + SAVEPICH + 5;
			static const unsigned int INFOH = 200 - INFOY - 11;

			DrawWindow(SAVEPICX - 1, SAVEPICY - 1, SAVEPICW + 2, SAVEPICH + 2, BKGDCOLOR);
			DrawWindow(SAVEPICX - 1, INFOY, SAVEPICW + 2, INFOH, BKGDCOLOR);

			int curPos = menu->getCurrentPosition();
			if((unsigned)menu->getNumItems() > SaveFile::files.Size())
				--curPos;

			if(curPos >= 0)
			{
				SaveFile &saveFile = SaveFile::files[slotIndex];

				if(saveFile.oldVersion)
				{
					word width, height;
					VW_MeasurePropString(SmallFont, language["MNU_DIFFVERSION"], width, height);

					px = SAVEPICX + (SAVEPICW - width)/2;
					py = SAVEPICY + (SAVEPICH - height)/2;
					VWB_DrawPropString(SmallFont, language["MNU_DIFFVERSION"], textColor);
				}
				else
				{
					PNGHandle *png;
					FILE *file = OpenSaveFile(saveFile.filename, "rb");
					if(file && (png = M_VerifyPNG(file)))
					{
						FTexture *savePicture = PNGTexture_CreateFromFile(png, GetFullSaveFileName(saveFile.filename));
						VWB_DrawGraphic(savePicture, SAVEPICX, SAVEPICY, SAVEPICW, SAVEPICH, MENU_CENTER);

						char* creationTime = M_GetPNGText(png, "Creation Time");
						char* comment = M_GetPNGText(png, "Comment");
						px = SAVEPICX;
						py = INFOY;
						if(creationTime)
						{
							VWB_DrawPropStringWrap(SAVEPICW, INFOH, SmallFont, creationTime, textColor);
							px = SAVEPICX;
							py += 5; // Space the strings a little.
							delete[] creationTime;
						}
						if(comment)
						{
							VWB_DrawPropStringWrap(SAVEPICW, INFOH, SmallFont, comment, textColor);
							delete[] comment;
						}

						delete savePicture;
						delete png;
					}
					if(file)
						fclose(file);
				}
			}
			else
			{
				word width, height;
				VW_MeasurePropString(SmallFont, language["MNU_NOPICTURE"], width, height);

				px = SAVEPICX + (SAVEPICW - width)/2;
				py = SAVEPICY + (SAVEPICH - height)/2;
				VWB_DrawPropString(SmallFont, language["MNU_NOPICTURE"], textColor);
			}
		}
	}
};

class LoadSaveMenu : public Menu
{
public:
	LoadSaveMenu(bool save) : Menu(LSM_X, LSM_Y, LSM_W, 24), save(save)
	{
	}

protected:
	void handleDelete()
	{
		if(save)
			return;

		SaveSlotMenuItem *item = static_cast<SaveSlotMenuItem *>(getIndex(getCurrentPosition()));
		SaveFile &saveFile = SaveFile::files[item->slotIndex];

		FString msg;
		msg.Format(language["DELETESVDGAME"], saveFile.name.GetChars());
		if(Confirm(msg))
		{
			File(GetFullSaveFileName(saveFile.filename)).remove();
			item->setVisible(false);
			setCurrentPosition(getCurrentPosition());
		}
	}

	bool save;
};

MENU_LISTENER(BeginEditSave);
MENU_LISTENER(LoadSaveGame);
MENU_LISTENER(PerformSaveGame);

static LoadSaveMenu loadGame(false), saveGame(true);
static MenuItem *loadItem, *saveItem;

Menu &GetLoadMenu() { return loadGame; }
Menu &GetSaveMenu() { return saveGame; }
MenuItem *GetLoadMenuItem() { return loadItem; }
MenuItem *GetSaveMenuItem() { return saveItem; }

//
// DRAW LOAD/SAVE IN PROGRESS
//
static void DrawLSAction (int which)
{
	DrawWindow (LSA_X, LSA_Y, LSA_W, LSA_H, MENUWIN_BACKGROUND);
	DrawOutline (LSA_X, LSA_Y, LSA_W, LSA_H, 0, MENUWIN_TOPBORDER);
	VWB_DrawGraphic (TexMan("M_LDING1"), LSA_X + 8, LSA_Y + 5, MENU_CENTER);

	PrintX = LSA_X + 46;
	PrintY = LSA_Y + 13;

	if (!which)
		US_Print (BigFont, language["STR_LOADING"]);
	else
		US_Print (BigFont, language["STR_SAVING"]);

	VW_UpdateScreen ();
}

// ECWolf <1.4.2 for Windows saved full filepaths due to ZDoom code expecting
// all file paths to have Unix path separators. Look for Windows path
// separators and strip the paths.
static const char* NormalizeSavedFilename(const char* filename)
{
	const char* backslash = strrchr(filename, '\\');
	return backslash ? backslash + 1 : filename;
}

////////////////////////////////////////////////////////////////////
//
// SEE WHICH SAVE GAME FILES ARE AVAILABLE & READ STRING IN
//
////////////////////////////////////////////////////////////////////
bool SetupSaveGames()
{
	bool canLoad = false;
	char title[MAX_SAVENAME+1];

	File saveDirectory(FileSys::GetDirectoryPath(FileSys::DIR_Saves));
	const TArray<FString> &files = saveDirectory.getFileList();

	for(unsigned int i = 0;i < files.Size();i++)
	{
		const FString &filename = files[i];
		if(filename.Len() < 5 ||
			filename.Mid(filename.Len()-4, 4).Compare(".ecs") != 0)
			continue; // Too short or incorrect name

		PNGHandle *png;
		FILE *file = OpenSaveFile(filename, "rb");
		if(file)
		{
			if((png = M_VerifyPNG(file)))
			{
				SaveFile sFile;
				sFile.filename = filename;
				sFile.hasFiles = true;

				char* savesig = M_GetPNGText(png, "ECWolf Save Version");
				if(savesig)
				{
					if(strncmp(savesig, GetSaveSignature(), 10) != 0) // Should be "ECWOLFSAVE"
						sFile.oldVersion = true;
					else
					{
						unsigned long long savever = atoll(savesig+10);
						char *prodver = M_GetPNGText(png, "ECWolf Save Product Version");
						// If the build was done in the revision control tree, then
						// savever should be used for better precision.  Otherwise
						// we must compare the version number itself.
						if((savever == SAVEVERUNDEFINED &&
							atoll(prodver) < MINSAVEPRODVER) ||
							savever < MINSAVEVER)
						{
							sFile.oldVersion = true;
						}
						else
							sFile.oldVersion = false;
						delete[] prodver;
					}
					delete[] savesig;
				}
				else
					sFile.oldVersion = true;

				char* checkFile = M_GetPNGText(png, "Map WAD");
				if(checkFile)
				{
					if(Wads.CheckIfWadLoaded(NormalizeSavedFilename(checkFile)) < 0 && !param_foreginsave)
						sFile.hasFiles = false;
					delete[] checkFile;
				}

				checkFile = M_GetPNGText(png, "Game WAD");
				sFile.hide = false;
				if(checkFile)
				{
					FString checkString(checkFile);
					int lastIndex = 0;
					int nextIndex = 0;
					int expectedIwads = IWad::GetNumIWads();
					do
					{
						nextIndex = checkString.IndexOf(';', lastIndex);
						if(Wads.CheckIfWadLoaded(NormalizeSavedFilename(checkString.Mid(lastIndex, nextIndex-lastIndex))) < 0 && !param_foreginsave)
						{
							sFile.hide = true;
							break;
						}
						else
							--expectedIwads;
						lastIndex = nextIndex + 1;
					}
					while(nextIndex != -1);

					// See if we don't have the right number of iwads loaded.
					if(expectedIwads != 0 && !param_foreginsave)
						sFile.hide = true;
					else
						canLoad = true;
					delete[] checkFile;
				}

				if(M_GetPNGText(png, "Title", title, MAX_SAVENAME))
				{
					sFile.name = title;
					SaveFile::files.Push(sFile);
				}

				delete png;
			}

			fclose(file);
		}
	}

	loadGame.clear();
	saveGame.clear();

	MenuItem *newSave = new SaveSlotMenuItem(0, NEW_SAVE, 31, NULL, PerformSaveGame, true);
	newSave->setHighlighted(true);
	saveGame.addItem(newSave);

	for(unsigned int i = 0;i < SaveFile::files.Size();i++)
	{
		MenuItem *item = new SaveSlotMenuItem(i, SaveFile::files[i].name, 31, LoadSaveGame);
		if(SaveFile::files[i].oldVersion || !SaveFile::files[i].hasFiles)
			item->setHighlighted(2);
		item->setVisible(!SaveFile::files[i].hide);
		loadGame.addItem(item);

		item = new SaveSlotMenuItem(i, SaveFile::files[i].name, 31, BeginEditSave, PerformSaveGame);
		if(SaveFile::files[i].oldVersion || !SaveFile::files[i].hasFiles)
			item->setHighlighted(2);
		item->setVisible(!SaveFile::files[i].hide);
		saveGame.addItem(item);
	}

	return canLoad;
}

MENU_LISTENER(BeginEditSave)
{
	bool ret = Confirm(language["GAMESVD"]);
	saveGame.draw();
	return ret;
}

MENU_LISTENER(PerformSaveGame)
{
	SaveFile file;

	// Copy the name
	file.name = static_cast<SaveSlotMenuItem *> (saveGame[which])->getValue();
	file.oldVersion = false;
	file.hasFiles = true;
	if(which == 0) // New
	{
		static_cast<SaveSlotMenuItem *> (saveGame[which])->setValue(NEW_SAVE);

		// Locate a available filename.  I don't want to assume savegamX.yza so this
		// might not be the fastest way to do things.
		bool nextSaveNumber = false;
		for(unsigned int i = 0;i < 10000;i++)
		{
			file.filename.Format("savegam%u.ecs", i);
			for(unsigned int j = 0;j < SaveFile::files.Size();j++)
			{
				if(stricmp(file.filename, SaveFile::files[j].filename) == 0)
				{
					nextSaveNumber = true;
					continue;
				}
			}
			if(nextSaveNumber)
			{
				nextSaveNumber = false;
				continue;
			}
			break;
		}

		SaveFile::files.Push(file);

		unsigned int slotIndex = loadGame.getNumItems();
		loadGame.addItem(new SaveSlotMenuItem(slotIndex, file.name, 31, LoadSaveGame));
		saveGame.addItem(new SaveSlotMenuItem(slotIndex, file.name, 31, BeginEditSave, PerformSaveGame));

		saveGame.setCurrentPosition(saveGame.getNumItems()-1);
		loadGame.setCurrentPosition(saveGame.getNumItems()-1);

		mainMenu[2]->setEnabled(Net::InitVars.mode == Net::MODE_SinglePlayer);
	}
	else
	{
		SaveSlotMenuItem *menuItem = static_cast<SaveSlotMenuItem *> (loadGame.getIndex(which-1));

		file.filename = SaveFile::files[menuItem->slotIndex].filename;
		SaveFile::files[menuItem->slotIndex] = file;
		loadGame.setCurrentPosition(which-1);
		menuItem->setValue(file.name);

		// Ungreen
		saveGame.getIndex(which)->setHighlighted(0);
		loadGame.getIndex(which-1)->setHighlighted(0);
	}

	Save(file.filename, file.name);

	if(!quickSaveLoad)
		Menu::closeMenus(true);

	return true;
}

MENU_LISTENER(LoadSaveGame)
{
	SaveSlotMenuItem *menuItem = static_cast<SaveSlotMenuItem *> (loadGame.getIndex(which));

	if(SaveFile::files[menuItem->slotIndex].oldVersion || !SaveFile::files[menuItem->slotIndex].hasFiles)
		return false;

	loadedgame = true;
	Load(SaveFile::files[menuItem->slotIndex].filename);

	if(!quickSaveLoad)
		Menu::closeMenus(true);
	else
		loadedgame = false;

	saveGame.setCurrentPosition(which+1);
	return false;
}

void InitMenus()
{
	bool canLoad = SetupSaveGames() && Net::InitVars.mode == Net::MODE_SinglePlayer;

	loadGame.setHeadPicture("M_LOADGM");
	saveGame.setHeadPicture("M_SAVEGM");

	loadItem = new MenuSwitcherMenuItem(language["STR_LG"], GameSave::GetLoadMenu());
	loadItem->setEnabled(canLoad);
	saveItem = new MenuSwitcherMenuItem(language["STR_SG"], GameSave::GetSaveMenu());
	saveItem->setEnabled(false);
}

void QuickLoadOrSave(bool load)
{
	if(saveGame.getCurrentPosition() != 0)
	{
		const SaveSlotMenuItem *menuItem = static_cast<SaveSlotMenuItem *> (saveGame.getIndex(saveGame.getCurrentPosition()));

		quickSaveLoad = true;
		FString string;
		string.Format("%s\"%s\"?", language[load ? "STR_LGC" : "STR_SGC"], SaveFile::files[menuItem->slotIndex].name.GetChars());
		if(Confirm(string))
			load ? LoadSaveGame(saveGame.getCurrentPosition()-1) : PerformSaveGame(saveGame.getCurrentPosition());
		quickSaveLoad = false;

		return;
	}

	ShowMenu(load ? loadGame : saveGame);
}

static void Serialize(FArchive &arc)
{
	short difficulty;
	if(arc.IsStoring())
	{
		difficulty = SkillInfo::GetSkillIndex(*gamestate.difficulty);
		arc << difficulty;
	}
	else
	{
		arc << difficulty;
		gamestate.difficulty = &SkillInfo::GetSkill(difficulty);
	}

	unsigned int maxPlayers = Net::InitVars.numPlayers;

	arc << gamestate.playerClass[0];
	if(SaveVersion >= 1599444347)
	{
		arc << maxPlayers;
		for(unsigned int i = 1;i < maxPlayers;++i)
			arc << gamestate.playerClass[i];
	}
	arc << gamestate.secretcount
		<< gamestate.treasurecount
		<< gamestate.killcount
		<< gamestate.secrettotal
		<< gamestate.treasuretotal
		<< gamestate.killtotal
		<< gamestate.TimeCount
		<< gamestate.victoryflag;
	if(SaveVersion >= 1393719642)
		arc << gamestate.fullmap;
	arc << LevelRatios.killratio
		<< LevelRatios.secretsratio
		<< LevelRatios.treasureratio
		<< LevelRatios.numLevels
		<< LevelRatios.time;
	if(SaveVersion > 1395865826)
		arc << LevelRatios.par;

	thinkerList.Serialize(arc);

	arc << map;

	for(unsigned int i = 0;i < (SaveVersion >= 1656330251 ? maxPlayers : 1);++i)
		players[i].Serialize(arc);
}

#define SNAP_ID MAKE_ID('s','n','A','p')

bool Load(const FString &filename)
{
	FILE *fileh = OpenSaveFile(filename, "rb");
	if(fileh == NULL)
	{
		Message(language["STR_FAILREAD"]);
		printf("Could not open %s for reading.\n", GetFullSaveFileName(filename).GetChars());
		IN_ClearKeysDown ();
		IN_Ack (ACK_Local);
		return false;
	}

	PNGHandle *png = M_VerifyPNG(fileh);
	if(png == NULL)
	{
		fclose(fileh);
		return false;
	}

	if(!quickSaveLoad)
		DrawLSAction(0);

	char* savesig = M_GetPNGText(png, "ECWolf Save Version");
	SaveVersion = atoll(savesig+10);
	delete[] savesig;

	char *prodver = M_GetPNGText(png, "ECWolf Save Product Version");
	SaveProdVersion = (DWORD)atoll(prodver);
	delete[] prodver;

	M_GetPNGText(png, "Current Map", gamestate.mapname, 8);
	SetupGameLevel();

	{
		unsigned int chunkLength = M_FindPNGChunk(png, SNAP_ID);
		FPNGChunkArchive arc(fileh, SNAP_ID, chunkLength);
		FCompressedMemFile snapshot;
		snapshot.Serialize(arc);
		snapshot.Reopen();
		FArchive snarc(snapshot);
		Serialize(snarc);
	}

	FRandom::StaticReadRNGState(png);
	// It is apparently possible to load a game at just the right time and end
	// up in the wrong play state.
	playstate = ex_stillplaying;

	delete png;
	fclose(fileh);
	return true;
}

void SaveScreenshot(FILE *file)
{
	static const int SAVEPICWIDTH = 216;
	static const int SAVEPICHEIGHT = 162;

	vbuf = new byte[SAVEPICHEIGHT*SAVEPICWIDTH];
	vbufPitch = SAVEPICWIDTH;

	int oldviewsize = viewsize;
	Aspect oldaspect = vid_aspect;

	vid_aspect = ASPECT_16_10;
	NewViewSize(21, SAVEPICWIDTH, SAVEPICHEIGHT);
	CalcProjection(players[ConsolePlayer].mo->radius);
	R_RenderView();

	M_CreatePNG(file, vbuf, GPalette.BaseColors, SS_PAL, SAVEPICWIDTH, SAVEPICHEIGHT, vbufPitch);

	delete[] vbuf;
	vbuf = NULL;

	vid_aspect = oldaspect;
	NewViewSize(oldviewsize); // Restore
}

bool Save(const FString &filename, const FString &title)
{
	FILE *fileh = OpenSaveFile(filename, "wb");
	if(fileh == NULL)
	{
		Message(language["STR_FAILWRITE"]);
		printf("Could not open %s for writing.\n", GetFullSaveFileName(filename).GetChars());
		IN_ClearKeysDown ();
		IN_Ack (ACK_Local);
		return false;
	}

	if(!quickSaveLoad)
		DrawLSAction(1);

	SaveVersion = GetSaveVersion();
	SaveProdVersion = SAVEPRODVER;

	// If we get hubs this will need to be moved so that we can have multiple of them
	FCompressedMemFile snapshot;
	snapshot.Open();
	{
		FArchive arc(snapshot);
		Serialize(arc);
	}

	SaveScreenshot(fileh);
	M_AppendPNGText(fileh, "Software", "ECWolf");
	M_AppendPNGText(fileh, "Engine", GAMESIG);
	M_AppendPNGText(fileh, "ECWolf Save Version", GetSaveSignature());
	{
		char saveprodver[11];
		mysnprintf(saveprodver, 11, "%u", SAVEPRODVER);
		M_AppendPNGText(fileh, "ECWolf Save Product Version", saveprodver);
	}
	M_AppendPNGText(fileh, "Title", title);
	M_AppendPNGText(fileh, "Current Map", levelInfo->MapName);

	FString comment;
	// Creation Time
	{
		time_t currentTime;
		struct tm *timeInfo;

		time(&currentTime);
		timeInfo = localtime(&currentTime);
		M_AppendPNGText(fileh, "Creation Time", asctime(timeInfo));
	}

	unsigned int gametime = gamestate.TimeCount/70;
	FString mapname = gamestate.mapname;
	mapname.ToUpper();
	comment.Format("%s - %s\nTime: %02d:%02d:%02d", mapname.GetChars(), levelInfo->GetName(map).GetChars(),
		gametime/3600, (gametime%3600)/60, gametime%60);
	M_AppendPNGText(fileh, "Comment", comment);

	FString gameWadString;
	for(unsigned int i = 0;i < IWad::GetNumIWads();++i)
	{
		if(i)
			gameWadString += ';';

		gameWadString += Wads.GetWadName(FWadCollection::IWAD_FILENUM + i);
	}
	M_AppendPNGText(fileh, "Game WAD", gameWadString);
	M_AppendPNGText(fileh, "Map WAD", Wads.GetWadName(Wads.GetLumpFile(map->GetMarketLumpNum())));

	FRandom::StaticWriteRNGState(fileh);

	{
		FPNGChunkArchive snapshotArc(fileh, SNAP_ID);
		snapshot.Serialize(snapshotArc);
	}

	M_FinishPNG(fileh);
	fclose(fileh);
	return true;
}

/* end namespace */ }
