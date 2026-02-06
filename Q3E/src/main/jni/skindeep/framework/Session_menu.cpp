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

#include "sys/platform.h"
#include "idlib/LangDict.h"
#include "framework/async/AsyncNetwork.h"
#include "framework/FileSystem.h"
#include "framework/Console.h"
#include "framework/Game.h"
#include "sound/sound.h"
#include "ui/UserInterface.h"

#include "framework/Session_local.h"
#include "renderer/tr_local.h"
#include "Game_local.h"
#include "d3xp/Player.h"

#include "d3xp/platformutilties.h"

idCVar	idSessionLocal::gui_configServerRate( "gui_configServerRate", "0", CVAR_GUI | CVAR_ARCHIVE | CVAR_ROM | CVAR_INTEGER, "" );

// implements the setup for, and commands from, the main menu

/*
==============
idSessionLocal::GetActiveMenu
==============
*/
idUserInterface *idSessionLocal::GetActiveMenu( void ) {
	return guiActive;
}

/*
==============
idSessionLocal::StartMainMenu
==============
*/
void idSessionLocal::StartMenu( bool playIntro ) {
	if ( guiActive == guiMainMenu ) {
		return;
	}

	if ( readDemo ) {
		// if we're playing a demo, esc kills it
		UnloadMap();
	}

	// pause the game sound world
	if ( sw != NULL && !sw->IsPaused() ) {
		sw->Pause();
	}

	// start playing the menu sounds
	soundSystem->SetPlayingSoundWorld( menuSoundWorld );

	SetGUI( guiMainMenu, NULL );

	guiMainMenu->HandleNamedEvent("menustart");
	
	//BC 3-5-2025: re-enable the intro video logic.
	guiMainMenu->HandleNamedEvent( playIntro ? "playIntro" : "noIntro" );


	//if(fileSystem->HasD3XP()) {
	//	guiMainMenu->SetStateString("game_list", common->GetLanguageDict()->GetString( "#str_07202" ));
	//} else {
	//	guiMainMenu->SetStateString("game_list", common->GetLanguageDict()->GetString( "#str_07212" ));
	//}

	console->Close();

	cvarSystem->SetCVarBool("g_hideHudInternal", true); //blendo eric: hide hud in main menu
}

void idSessionLocal::LoadGameMenu()
{
	if (guiActive == guiMainMenu) {
		return;
	}

	// pause the game sound world
	if (sw != NULL && !sw->IsPaused()) {
		sw->Pause();
	}

	// start playing the menu sounds
	soundSystem->SetPlayingSoundWorld(menuSoundWorld);

	SetGUI(guiMainMenu, NULL);

	guiMainMenu->HandleNamedEvent("menustart");
	guiMainMenu->HandleNamedEvent("noIntro");
	guiMainMenu->HandleNamedEvent("showLoadMenu");

	console->Close();

	cvarSystem->SetCVarBool("g_hideHudInternal", true); //blendo eric: hide hud in main menu
}


/*
=================
idSessionLocal::SetGUI
=================
*/
void idSessionLocal::SetGUI( idUserInterface *gui, HandleGuiCommand_t handle ) {
	guiActive = gui;
	guiHandle = handle;
	if ( guiMsgRestore ) {
		common->DPrintf( "idSessionLocal::SetGUI: cleared an active message box\n" );
		guiMsgRestore = NULL;
	}
	if ( !guiActive ) {
		return;
	}

	if ( guiActive == guiMainMenu ) {
		SetSaveGameGuiVars();
		SetMainMenuGuiVars();
	} else if ( guiActive == guiRestartMenu ) {
		SetSaveGameGuiVars();
	}

	sysEvent_t  ev;
	memset( &ev, 0, sizeof( ev ) );
	ev.evType = SE_NONE;

	guiActive->HandleEvent( &ev, com_frameTime );
	guiActive->Activate( true, com_frameTime );
}

/*
===============
idSessionLocal::ExitMenu
===============
*/
void idSessionLocal::ExitMenu( void ) {
	guiActive = NULL;

	// go back to the game sounds
	soundSystem->SetPlayingSoundWorld( sw );

	// unpause the game sound world
	if ( sw != NULL && sw->IsPaused() ) {
		sw->UnPause();
	}
	cvarSystem->SetCVarBool("g_hideHudInternal", false); //blendo eric: hide hud in main menu

	// SM: Set this so that if you use B to exit the menu it doesn't crouch
	if (gameLocal.GetLocalPlayer())
	{
		gameLocal.GetLocalPlayer()->escapedFullScreenMenu = true;
	}
}

/*
===============
idListSaveGameCompare
===============
*/
ID_INLINE int idListSaveGameCompare( const fileTIME_T *a, const fileTIME_T *b ) {
	return b->timeStamp - a->timeStamp;
}

/*
===============
idSessionLocal::GetSaveGameList
===============
*/
void idSessionLocal::GetSaveGameList( idStrList &fileList, idList<fileTIME_T> &fileTimes ) {
	int i;
	idFileList *files;

	// NOTE: no fs_game_base for savegames
	idStr game = cvarSystem->GetCVarString( "fs_game" );
	if( game.Length() ) {
		files = fileSystem->ListFiles( "savegames", ".save", false, false, game );
	} else {
		files = fileSystem->ListFiles( "savegames", ".save" );
	}

	fileList = files->GetList();
	fileSystem->FreeFileList( files );

	for ( i = 0; i < fileList.Num(); i++ ) {

		// use save game name session id time if available
		ID_TIME_T timeStamp = ExtractTimeFromSaveGameUniqueID( ExtractSaveGameUniqueId( fileList[i] ) );

		if ( timeStamp <= 0 ) { // fallback to modified time
			fileSystem->ReadFile("savegames/" + fileList[i], NULL, &timeStamp);
		}

		fileList[i].StripLeading( '/' );
		fileList[i].StripFileExtension();

		fileTIME_T ft;
		ft.index = i;
		ft.timeStamp = timeStamp;
		fileTimes.Append( ft );
	}

	fileTimes.Sort( idListSaveGameCompare );
}

/*
===============
idSessionLocal::SetSaveGameGuiVars
===============
*/
//BC Set the information to appear in the 'loadgame' listDef, for the mainmenu load GUI.
void idSessionLocal::SetSaveGameGuiVars( void ) {
	int i;
	idStr name;
	idStrList fileList;
	idList<fileTIME_T> fileTimes;

	loadGameList.Clear();
	fileList.Clear();
	fileTimes.Clear();

	GetSaveGameList( fileList, fileTimes );

	loadGameList.SetNum( fileList.Num() );
	for ( i = 0; i < fileList.Num(); i++ ) {
		loadGameList[i] = fileList[fileTimes[i].index];

		idStr description, screenshot;//BC

		//BC Load the savegame TXT file. The TXT file has some info for the savegame.
		idLexer src(LEXFL_NOERRORS|LEXFL_NOSTRINGCONCAT);
		if ( src.LoadFile( va("savegames/%s.txt", loadGameList[i].c_str()) ) ) {
			idToken tok;
			src.ReadToken( &tok );
			name = tok; //BC 4-1-2025: note: this isn't used any more; we instead use the "description" field below to grab the localized level name.

			src.ReadToken(&tok);
			description = tok;

			//BC 4-1-2025: get level displayname.
			if (name.Find("autosave") != -1) {
				name = idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_save_autosave"), gameLocal.GetMapdisplaynameViaMapfile(description).c_str());
			} else {
				name = gameLocal.GetMapdisplaynameViaMapfile(description);
			}

			src.ReadToken(&tok);

			// SW 10th April 2025: Build number is now saved to the .txt, so we need to skip over its token to get to the screenshot path
			// The screenshot isn't used here anyway, but I don't want this to be a landmine for anyone else.
			src.ReadToken(&tok);
			screenshot = tok;
		}
		else
		{
			name = loadGameList[i];
		}

		name += "\t";

		//idStr date = Sys_TimeStampToStr( fileTimes[i].timeStamp );
		//name += date;

		idStr diffString;

		//BC don't show the date. Instead, print string of how long ago the save was relative to now.
		diffString = ParseSavetime(fileTimes[i].timeStamp);

		name += diffString; //Add the relative time elapsed to the savegame display name in the list.

		guiActive->SetStateString( va("loadgame_item_%i", i), name);

		//BC set up the restart.gui load carousel.
		//if (i <= 3) //Only for the first 3
		//{
		//	//Get the timestamp info.
		//	time_t		filetimestamp;
		//	time_t		today;
		//	time_t		timediff;
		//	struct tm	*timeinfo = nullptr;
		//	int			days;
		//	idStr		diffString;
		//
		//	filetimestamp = fileTimes[i].timeStamp;
		//	time(&today);
		//	timediff = (today - filetimestamp);
		//	timeinfo = gmtime(&timediff);			
		//
		//	days = timeinfo->tm_yday;
		//
		//	if (days > 0)
		//	{
		//		if (days > 1)
		//			diffString = va("%d days ago", days);
		//		else
		//			diffString = "Yesterday";
		//	}
		//	else
		//	{
		//		if (timeinfo->tm_hour > 0)
		//		{
		//			diffString = va("%d hours ago", timeinfo->tm_hour);
		//		}
		//		else if (timeinfo->tm_min > 0)
		//		{
		//			diffString = va("%d minutes ago", timeinfo->tm_min);
		//		}
		//		else
		//		{
		//			diffString = va("%d seconds ago", timeinfo->tm_sec);
		//		}
		//	}
		//
		//	guiActive->SetStateString(va("loadgame%i_time", i), diffString);
		//	guiActive->SetStateString(va("loadgame%i_shot", i), screenshot.c_str());
		//	guiActive->SetStateInt(va("loadgame%i_show", i), 1);
		//}
	}
	guiActive->DeleteStateVar( va("loadgame_item_%i", fileList.Num()) );

	guiActive->SetStateString( "loadgame_sel_0", "-1" );
	guiActive->SetStateString( "loadgame_shot", "guis/assets/teapot" );

}

//BC 5-6-2025: breaking this out into its own function because it now needs to get called from multiple places.
idStr idSessionLocal::ParseSavetime(time_t filetimestamp)
{
	ID_TIME_T curTime = time(nullptr);
	localtime(&curTime);

	ID_TIME_T timeDiff = difftime(curTime, filetimestamp);

	const ID_TIME_T dayLimit = 9999 * 24 * 60 * 60; // 9999 days in seconds
	if (timeDiff < 0) {
		timeDiff = 0;
	}
	else if (timeDiff > dayLimit) {
		timeDiff = dayLimit;
	}

	int days = timeDiff / (24 * 60 * 60);
	int hours = timeDiff / (60 * 60);
	int mins = timeDiff / (60);
	int seconds = timeDiff;

	if (days > 1)
	{
		return idStr::Format(common->GetLanguageDict()->GetString("#str_loadgame_daysago"), days);
	}
	else if (days == 1)
	{
		return common->GetLanguageDict()->GetString("#str_loadgame_yesterday");
	}
	else if (hours > 0)
	{
		return  idStr::Format(common->GetLanguageDict()->GetString("#str_loadgame_hoursago"), hours);
	}
	else if (mins > 0)
	{
		return idStr::Format(common->GetLanguageDict()->GetString("#str_loadgame_minutesago"), mins);
	}
	else if (seconds > 0)
	{
		return idStr::Format(common->GetLanguageDict()->GetString("#str_loadgame_secondsago"), seconds);
	}
	
	return "";

}

/*
===============
idSessionLocal::SetModsMenuGuiVars
===============
*/
void idSessionLocal::SetModsMenuGuiVars( void ) {
	int i;
	idModList *list = fileSystem->ListMods();

	bool foundActiveMod = false;
	int totalNumberOfMods = list->GetNumMods();
	

	//add steam workshop mods. This isn't actually adding it to list; this is just gathering the steam workshop infos...
	int workshopInstalledAmount = 0;
	idStrList modpaths;

	if (common->g_SteamUtilities)
	{
		if (common->g_SteamUtilities->IsSteamInitialized())
		{
			int workshopTotal = common->g_SteamUtilities->GetSteamWorkshopAmount();
			common->DPrintf("[steam] subscribed workshop items: %d\n", workshopTotal);

			for (i = 0; i < workshopTotal; i++)
			{
				idStr path = common->g_SteamUtilities->GetWorkshopPathAtIndex(i);

				if (path.Length() <= 0)
					continue;

				workshopInstalledAmount++;
				modpaths.Append(path);
			}

			common->DPrintf("[steam] installed workshop items:  %d\n", workshopInstalledAmount);
			totalNumberOfMods += workshopInstalledAmount;
		}
	}




	modsList.SetNum(totalNumberOfMods);

	// Build the local mods gui list
	for ( i = 0; i < list->GetNumMods(); i++ )
	{
		idStr path = list->GetMod(i);
		idStr modName = list->GetDescription(i);

		// SW 6th March 2025
		// Figure out the active mod for displaying at the top of the list
		if (!foundActiveMod && path.Icmp(cvarSystem->GetCVarString("fs_game")) == 0)
		{
			foundActiveMod = true;
			guiActive->SetStateString("activemodname", modName);
		}

		modName += "\t";
		modName += common->GetLanguageDict()->GetString("#str_gui_mainmenu_mod_local");
		guiActive->SetStateString(idStr::Format("modsList_item_%d", i).c_str(), modName.c_str());

		common->Printf("MODS: #%d local    path: '%s'\n", i, path.c_str());

		modsList[i] = list->GetMod(i);
	}


	
	//Build the steam workshop mod gui list.
	if (common->g_SteamUtilities && common->g_SteamUtilities->IsSteamInitialized())
	{
		for (i = 0; i < workshopInstalledAmount; i++)
		{
			idStr descPath = modpaths[i].c_str();

#ifdef _WIN32
#define PATH_SLASH "\\"
#else
#define PATH_SLASH "/"
#endif
			descPath.Append(PATH_SLASH"description.txt");

			const char searchPath[] = PATH_SLASH "steamapps" PATH_SLASH "workshop" PATH_SLASH "content" PATH_SLASH;
			
			// Get whatever comes after workshop/content
			int foldernameIndex = idStr::FindText(modpaths[i], searchPath, false);
			int rightSize = modpaths[i].Length() - foldernameIndex - sizeof(searchPath) + 1;
			idStr workshopStr = modpaths[i].Right(rightSize);
			
			// Find the point where there's a slash between the appid and the workshopid
			int workshopSlashIdx = idStr::FindText(workshopStr, PATH_SLASH, false);
			
			// Root directory is everything but the workshopid
			idStr rootDirStr = modpaths[i].Left(modpaths[i].Length() - workshopStr.Length() + workshopSlashIdx);
			
			// need just the final workshopid
			workshopStr = workshopStr.Right(workshopStr.Length() - workshopSlashIdx - 1);

			idStr modTitle = fileSystem->GetModDescription(descPath);

			// SW 6th March 2025
			// Figure out the active mod for displaying at the top of the list
			if (!foundActiveMod && workshopStr.Icmp(cvarSystem->GetCVarString("fs_game")) == 0)
			{
				foundActiveMod = true;
				guiActive->SetStateString("activemodname", modTitle);
			}

			idStr modName;
			modName = modTitle.c_str();
			modName += "\t";
			modName += common->GetLanguageDict()->GetString("#str_gui_mainmenu_mod_workshop");
			guiActive->SetStateString(idStr::Format("modsList_item_%d", i + list->GetNumMods()).c_str(), modName.c_str());

			common->Printf("MODS: #%d workshop root: %s id: %s\n", list->GetNumMods() + i, rootDirStr.c_str(), workshopStr.c_str());

			// Store this as a combinatoin of the fs_cdpath and fs_game, separated by '|'
			modsList[list->GetNumMods() + i] = rootDirStr + '|' + workshopStr;

#undef PATH_SLASH
		}

		for (i = 0; i < totalNumberOfMods; i++)
		{
			common->DPrintf("[steam] mod path %d: %s\n", i, modsList[i].c_str());
		}
	}

	// SW 6th March 2025:
	// We couldn't find the active mod. This is weird and shouldn't happen but let's try to be graceful about it.
	if (!foundActiveMod)
	{
		common->Warning("SetModsMenuGuiVars: could not determine the current active mod!");
		guiActive->SetStateString("activemodname", "#str_loc_unknown_00104");
	}


	//guiActive->DeleteStateVar( idStr::Format("modsList_item_%d", list->GetNumMods()).c_str() );
	guiActive->SetStateString( "modsList_sel_0", "-1" );

	fileSystem->FreeModList( list );

#ifdef STEAM
	guiMainMenu->SetStateString("allowWorkshop", "1");
#else
	guiMainMenu->SetStateString("allowWorkshop", "0");
#endif

	common->Printf("MODS: total amount=%d (local amount=%d ; steamworkshop amount=%d)\n", totalNumberOfMods, (totalNumberOfMods - workshopInstalledAmount), workshopInstalledAmount);
}


/*
===============
idSessionLocal::SetMainMenuSkin
===============
*/
void idSessionLocal::SetMainMenuSkin( void ) {
	// skins
	idStr str = cvarSystem->GetCVarString( "mod_validSkins" );
	idStr uiSkin = cvarSystem->GetCVarString( "ui_skin" );
	idStr skin;
	int skinId = 1;
	int count = 1;
	while ( str.Length() ) {
		int n = str.Find( ";" );
		if ( n >= 0 ) {
			skin = str.Left( n );
			str = str.Right( str.Length() - n - 1 );
		} else {
			skin = str;
			str = "";
		}
		if ( skin.Icmp( uiSkin ) == 0 ) {
			skinId = count;
		}
		count++;
	}

	for ( int i = 0; i < count; i++ ) {
		guiMainMenu->SetStateInt( va( "skin%i", i+1 ), 0 );
	}
	guiMainMenu->SetStateInt( va( "skin%i", skinId ), 1 );
}

/*
===============
idSessionLocal::SetPbMenuGuiVars
===============
*/
void idSessionLocal::SetPbMenuGuiVars( void ) {
}

/*
===============
idSessionLocal::SetMainMenuGuiVars
===============
*/
void idSessionLocal::SetMainMenuGuiVars( void ) {

	guiMainMenu->SetStateString( "serverlist_sel_0", "-1" );
	guiMainMenu->SetStateString( "serverlist_selid_0", "-1" );

	guiMainMenu->SetStateInt( "com_machineSpec", com_machineSpec.GetInteger() );

	// "inetGame" will hold a hand-typed inet address, which is not archived to a cvar
	guiMainMenu->SetStateString( "inetGame", "" );

	// key bind names
	guiMainMenu->SetKeyBindingNames();

	// flag for in-game menu
	if ( mapSpawned ) {
		guiMainMenu->SetStateString( "inGame", IsMultiplayer() ? "2" : "1" );
	} else {
		guiMainMenu->SetStateString( "inGame", "0" );
	}

	SetResumeButton();

	SetCDKeyGuiVars( );
	guiMainMenu->SetStateString( "nightmare", cvarSystem->GetCVarBool( "g_nightmare" ) ? "1" : "0" );
	guiMainMenu->SetStateString( "browser_levelshot", "guis/assets/splash/pdtempa" );

	SetMainMenuSkin();

#ifndef DEMO
	// Mods Menu
	SetModsMenuGuiVars();
#endif

	guiMsg->SetStateString( "visible_hasxp", fileSystem->HasD3XP() ? "1" : "0" );

	guiMainMenu->SetStateString( "driver_prompt", "0" );

	SetPbMenuGuiVars();

	guiMainMenu->SetStateString("builddate", cvarSystem->GetCVarString("g_versionShort"));
}

void idSessionLocal::SetResumeButton()
{
	//Create Resume Campaign button in main menu.
#ifndef DEMO

	//BC 4-1-2025: see if savegames exist in the savegame folder.
	idStrList fileList;
	idList<fileTIME_T> fileTimes;
	GetSaveGameList(fileList, fileTimes);

	if (fileList.Num() > 0) //If there is at least 1 file in the save folder...
	{
		guiMainMenu->SetStateString("hasProgressionSave", "1");

		idStrList loadGameList;
		loadGameList.Clear();
		loadGameList.SetNum(fileList.Num());

		for (int i = 0; i < fileList.Num(); i++)
		{
			loadGameList[i] = fileList[fileTimes[i].index]; //This sorts by date.

			idStr filename = idStr::Format("savegames/%s.txt", loadGameList[i].c_str());

			idStr savemapname = "";
			idLexer src(LEXFL_NOERRORS | LEXFL_NOSTRINGCONCAT);
			if (src.LoadFile(filename.c_str()))
			{
				idStr text;

				idToken tok;
				src.ReadToken(&tok);
				text = tok;

				src.ReadToken(&tok);
				savemapname = tok;
			}

			savemapname = gameLocal.GetMapdisplaynameViaMapfile(savemapname);

			if (savemapname.Length() > 0) //BC 4-3-2025: if there is a valid map name, then print it. Otherwise, leave it blank.
			{
				guiMainMenu->SetStateString("resumemapname", idStr::Format("%s: %s", common->GetLanguageDict()->GetString("#str_gui_mainmenu_chapter"), savemapname.c_str()).c_str());
			}
			else
			{
				guiMainMenu->SetStateString("resumemapname", "");
			}

			i = fileList.Num() + 100; //break out of the loop
		}


	}
	else
#endif // DEMO
	{
		guiMainMenu->SetStateString("hasProgressionSave", "0");
	}
}


/*
==============
idSessionLocal::HandleSaveGameMenuCommands
==============
*/
bool idSessionLocal::HandleSaveGameMenuCommand( idCmdArgs &args, int &icmd ) {

	const char *cmd = args.Argv(icmd-1);

	//BC player clicked on Load Game button. Start the load.
	if ( !idStr::Icmp( cmd, "loadGame" ) ) {
		int choice = guiActive->State().GetInt("loadgame_sel_0");
		if ( choice >= 0 && choice < loadGameList.Num() ) {
			sessLocal.LoadGame( loadGameList[choice] );
		}
		return true;
	}

	if ( !idStr::Icmp( cmd, "saveGame" ) ) {
		const char *saveGameName = guiActive->State().GetString("saveGameName");
		if ( saveGameName && saveGameName[0] ) {

			// First see if the file already exists unless they pass '1' to authorize the overwrite
			if ( icmd == args.Argc() || atoi(args.Argv( icmd++ )) == 0 ) {
				idStr saveFileName = saveGameName;
				sessLocal.ScrubSaveGameFileName( saveFileName );
				saveFileName = "savegames/" + saveFileName;
				saveFileName.SetFileExtension(".save");

				idStr game = cvarSystem->GetCVarString( "fs_game" );
				idFile *file;
				if(game.Length()) {
					file = fileSystem->OpenFileRead( saveFileName, true, game );
				} else {
					file = fileSystem->OpenFileRead( saveFileName );
				}

				if ( file != NULL ) {
					fileSystem->CloseFile( file );

					// The file exists, see if it's an autosave
					saveFileName.SetFileExtension(".txt");
					idLexer src(LEXFL_NOERRORS|LEXFL_NOSTRINGCONCAT);
					if ( src.LoadFile( saveFileName ) ) {
						idToken tok;
						src.ReadToken( &tok ); // Name
						src.ReadToken( &tok ); // Map
						src.ReadToken( &tok ); // Screenshot
						if ( !tok.IsEmpty() ) {
							// NOTE: base/ gui doesn't handle that one
							guiActive->HandleNamedEvent( "autosaveOverwriteError" );
							return true;
						}
					}
					guiActive->HandleNamedEvent( "saveGameOverwrite" );
					return true;
				}
			}

			sessLocal.SaveGame( saveGameName );
			SetSaveGameGuiVars( );
			guiActive->StateChanged( com_frameTime );
		}
		return true;
	}

	//BC player clicked on delete button.
	if ( !idStr::Icmp( cmd, "deleteGame" ) )
	{
		int choice = guiActive->State().GetInt( "loadgame_sel_0" );
		if ( choice >= 0 && choice < loadGameList.Num() )
		{
			if (common->g_SteamUtilities)
			{
				idStr basegame = cvarSystem->GetCVarString("fs_game");
				if (basegame.IsEmpty())
				{
					basegame = "base";
				}

				common->g_SteamUtilities->SteamCloudDeleteFile(idStr::Format("%s/savegames/%s.save", basegame.c_str(), loadGameList[choice].c_str()).c_str());
				common->g_SteamUtilities->SteamCloudDeleteFile(idStr::Format("%s/savegames/%s.tga", basegame.c_str(), loadGameList[choice].c_str()).c_str());
				common->g_SteamUtilities->SteamCloudDeleteFile(idStr::Format("%s/savegames/%s.txt", basegame.c_str(), loadGameList[choice].c_str()).c_str());
			}


			fileSystem->RemoveFile( va("savegames/%s.save", loadGameList[choice].c_str()) );
			fileSystem->RemoveFile( va("savegames/%s.tga", loadGameList[choice].c_str()) );
			fileSystem->RemoveFile( va("savegames/%s.txt", loadGameList[choice].c_str()) );
			
			SetSaveGameGuiVars();

			//BC 4-3-2025: select item in list. This is so that after deleting a file, it auto selects the next item in list.
			if (loadGameList.Num() > 0)
			{
				int newselection = min(loadGameList.Num() - 1, choice);

				guiActive->SetStateInt("loadgame_sel_0", newselection); //Select item in list.
				ShowSavegameInfo(); //simulate clicking the selection so its info appears in the info panel.
			}
			else
			{
				guiActive->SetStateBool("loadbuttonsvisible", false);
			}
			
			guiActive->StateChanged( com_frameTime );

			SetResumeButton();
		}

		

		return true;
	}

	//BC Player clicked on a save file in the loadgame list. Display info for the selected savefile.
	if ( !idStr::Icmp( cmd, "updateSaveGameInfo" ) )
	{
		ShowSavegameInfo();
		return true;
	}

	return false;
}

void idSessionLocal::ShowSavegameInfo()
{
	int choice = guiActive->State().GetInt("loadgame_sel_0");
	if (choice >= 0 && choice < loadGameList.Num())
	{
		const idMaterial* material;

		idStr saveName, description, screenshot;
		idLexer src(LEXFL_NOERRORS | LEXFL_NOSTRINGCONCAT);

		if (src.LoadFile(va("savegames/%s.txt", loadGameList[choice].c_str())))
		{
			idToken tok;

			src.ReadToken(&tok);
			saveName = tok; //The savegame name.

			src.ReadToken(&tok);
			description = tok; //The internal map name.

			// SW 10th April 2025: Build number is now saved to the .txt, so we need to skip over its token to get to the screenshot path.
			// If you need to do anything with the build number, this is the place.
			src.ReadToken(&tok);
			src.ReadToken(&tok);
			screenshot = tok; //BC the map loading image.
		}
		else
		{
			saveName = loadGameList[choice];
			description = loadGameList[choice];
			screenshot = "";
		}

		if (screenshot.Length() <= 0)
		{
			screenshot = "guis/assets/teapot"; //BC fallback image.
		}

		material = declManager->FindMaterial(screenshot);
		if (material) {
			material->ReloadImages(false);
		}
		guiActive->SetStateString("loadgame_shot", screenshot);

		saveName.RemoveColors();
		guiActive->SetStateString("saveGameName", saveName);
		guiActive->SetStateString("saveGameDescription", description);

		int num = declManager->GetNumDecls(DECL_MAPDEF);
		for (int i = 0; i < num; i++)
		{
			const idDeclEntityDef* mapDef = static_cast<const idDeclEntityDef*>(declManager->DeclByIndex(DECL_MAPDEF, i));
			if (mapDef)
			{
				if (!idStr::Icmp(mapDef->GetName(), description))
				{
					guiActive->SetStateString("saveGameDisplayname", common->GetLanguageDict()->GetString(mapDef->dict.GetString("name")));
					guiActive->SetStateString("saveGameMapDescription", common->GetLanguageDict()->GetString(mapDef->dict.GetString("description")));
				}
			}
		}


		ID_TIME_T timeStamp;
		fileSystem->ReadFile(va("savegames/%s.save", loadGameList[choice].c_str()), NULL, &timeStamp);
		idStr date = Sys_TimeStampToStr(timeStamp);
		int tab = date.Find('\t');
		idStr time = date.Right(date.Length() - tab - 1);
		guiActive->SetStateString("saveGameDate", date.Left(tab));
		guiActive->SetStateString("saveGameTime", time);

		guiActive->SetStateBool("loadbuttonsvisible", true); //Show the Load/Delete buttons.
	}
	else
	{
		guiActive->SetStateBool("loadbuttonsvisible", false); //Hide the Load/Delete buttons.
	}
}

/*
==============
idSessionLocal::HandleRestartMenuCommands

Executes any commands returned by the gui
==============
*/
void idSessionLocal::HandleRestartMenuCommands( const char *menuCommand ) {
	// execute the command from the menu
	int icmd;
	idCmdArgs args;

	args.TokenizeString( menuCommand, false );

	for( icmd = 0; icmd < args.Argc(); ) {
		const char *cmd = args.Argv( icmd++ );

		if ( HandleSaveGameMenuCommand( args, icmd ) ) {
			continue;
		}

		if ( !idStr::Icmp( cmd, "restart" ) ) {
			if ( !LoadGame( GetAutoSaveName( mapSpawnData.serverInfo.GetString("si_map") ) ) ) {
				// If we can't load the autosave then just restart the map
				MoveToNewMap( mapSpawnData.serverInfo.GetString("si_map") );
			}
			continue;
		}

		if ( !idStr::Icmp( cmd, "quit" ) ) {
			ExitMenu();
			common->Quit();
			return;
		}

		if ( !idStr::Icmp ( cmd, "exec" ) ) {
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, args.Argv( icmd++ ) );
			continue;
		}

		if ( !idStr::Icmp( cmd, "play" ) ) {
			if ( args.Argc() - icmd >= 1 ) {
				idStr snd = args.Argv(icmd++);
				sw->PlayShaderDirectly(snd);
			}
			continue;
		}
	}
}

/*
==============
idSessionLocal::HandleIntroMenuCommands

Executes any commands returned by the gui
==============
*/
void idSessionLocal::HandleIntroMenuCommands( const char *menuCommand ) {
	// execute the command from the menu
	int i;
	idCmdArgs args;

	args.TokenizeString( menuCommand, false );

	for( i = 0; i < args.Argc(); ) {
		const char *cmd = args.Argv( i++ );

		if ( !idStr::Icmp( cmd, "startGame" ) ) {
			menuSoundWorld->ClearAllSoundEmitters();
			ExitMenu();
			continue;
		}

		if ( !idStr::Icmp( cmd, "play" ) ) {
			if ( args.Argc() - i >= 1 ) {
				idStr snd = args.Argv(i++);
				menuSoundWorld->PlayShaderDirectly(snd);
			}
			continue;
		}
	}
}

/*
==============
idSessionLocal::UpdateMPLevelShot
==============
*/
void idSessionLocal::UpdateMPLevelShot( void ) {
	char screenshot[ MAX_STRING_CHARS ];
	fileSystem->FindMapScreenshot( cvarSystem->GetCVarString( "si_map" ), screenshot, MAX_STRING_CHARS );
	guiMainMenu->SetStateString( "current_levelshot", screenshot );
}

/*
==============
idSessionLocal::HandleMainMenuCommands

Executes any commands returned by the gui
==============
*/
void idSessionLocal::HandleMainMenuCommands( const char *menuCommand ) {
	// execute the command from the menu
	int icmd;
	idCmdArgs args;

	args.TokenizeString( menuCommand, false );

	for( icmd = 0; icmd < args.Argc(); ) {
		const char *cmd = args.Argv( icmd++ );

		if ( HandleSaveGameMenuCommand( args, icmd ) ) {
			continue;
		}

		// always let the game know the command is being run
		if ( game ) {
			game->HandleMainMenuCommands( cmd, guiActive );
		}

		if ( !idStr::Icmp( cmd, "startGame" ) ) {
			cvarSystem->SetCVarInteger( "g_skill", guiMainMenu->State().GetInt( "skill" ) );
			if ( icmd < args.Argc() ) {
				StartNewGame( args.Argv( icmd++ ) );
			} else {
				StartNewGame( "game/mars_city1" );
			}
			// need to do this here to make sure com_frameTime is correct or the gui activates with a time that
			// is "however long map load took" time in the past
			common->GUIFrame( false, false );
			SetGUI( guiIntro, NULL );
			guiIntro->StateChanged( com_frameTime, true );
			// stop playing the game sounds
			soundSystem->SetPlayingSoundWorld( menuSoundWorld );

			continue;
		}

		if (!idStr::Icmp(cmd, "openstorepage"))
		{
			common->g_SteamUtilities->OpenSteamOverlaypageStore();
		}

		if (!idStr::Icmp(cmd, "openreportabug"))
		{
			//open reportabug overlay.


			//BC 5-8-2025: only allow bugreport if mod is not loaded.
			idStr basegame = cvarSystem->GetCVarString("fs_game");
			bool canDoReport = false;
			if (basegame.IsEmpty())
			{
				canDoReport = true;
			}
			else if (basegame.Icmp(BASE_GAMEDIR) == 0)
			{
				canDoReport = true;
			}

			if (canDoReport)
			{
				idStr infostring = GetPlayerLocationString();
				common->g_SteamUtilities->OpenSteamOverlaypage(idStr::Format("https://docs.google.com/forms/d/e/1FAIpQLScejbJ0SFfkHb0iWhFagXAikLwnyHOSie-n7tHUtFheFQ6oiQ/viewform?usp=dialog&entry.561524408=%s", infostring.c_str()).c_str());
			}
		}

		//BC 2-26-2025: open steam workshop page.
		if (!idStr::Icmp(cmd, "loadSteamWorkshop"))
		{
			common->g_SteamUtilities->OpenSteamOverlaypage("http://steamcommunity.com/app/301280/workshop");
		}

		//BC restart current mission.
		if (!idStr::Icmp(cmd, "restartmission"))
		{
			if (mapSpawned && currentMapName.Length() > 0)
			{
				idStr adjustedMapname = currentMapName.StripPath();
				MoveToNewMap(adjustedMapname.c_str());
			}	

			continue;
		}

		//BC start a new campaign
		if (!idStr::Icmp(cmd, "startGameViaIndex"))
		{
			//get the map with level index 1.
			int num = fileSystem->GetNumMaps();
			idStr levelnameToLoad = "";
			for (int i = 0; i < num; i++)
			{
				const idDeclEntityDef* mapDef = static_cast<const idDeclEntityDef*>(declManager->DeclByIndex(DECL_MAPDEF, i));
				if (mapDef && mapDef->dict.GetInt("levelindex") == 1)
				{
					levelnameToLoad = mapDef->GetName();
				}
			}

			if (levelnameToLoad.Length() <= 0)
			{
				common->Error("maps.def: failed to find level with levelindex 1");
				return;
			}

			// Reset email state in case it's not at the initial
			// (could happen if you were further along, go to main menu, start new game)
			int count = declManager->GetNumDecls(DECL_PDA); //total email count.
			for (int i = 0; i < count; i++)
			{
				const idDeclPDA* pda = static_cast<const idDeclPDA*>(declManager->DeclByIndex(DECL_PDA, i));
				pda->ResetEmails();
			}

			cvarSystem->SetCVarInteger("g_skill", guiMainMenu->State().GetInt("skill"));

			// SM 5/12/25: Fix for SD-603 where loading into another save then starting
			// a new game didn't reset progression
			gameLocal.persistentLevelInfo.Clear();
			gameLocal.persistentPlayerInfo[0].Clear();

			StartNewGame(levelnameToLoad.c_str());

			continue;
		}

		if (!idStr::Icmp(cmd, "loadCampaignProgression"))
		{
			//BC 4-2-2025: load latest save game.

			//First, ensure there are savegame files in the folder
			idStrList fileList;
			idList<fileTIME_T> fileTimes;
			GetSaveGameList(fileList, fileTimes);

			if (fileList.Num() <= 0)
			{
				common->Warning("loadCampaignProgression: savegame folder is empty.");
				continue; //folder is empty, so do early exit here.
			}

			idStrList loadGameList;
			loadGameList.Clear();
			loadGameList.SetNum(fileList.Num());

			//Iterate over the files until we find one that successfully loads.
			for (int i = 0; i < fileList.Num(); i++)
			{
				loadGameList[i] = fileList[fileTimes[i].index]; //This sorts by date.

				//idStr savefilename = idStr::Format("savegames/%s.txt", fileList[i].c_str());
				if (sessLocal.LoadGame(loadGameList[i]))
				{
					common->Printf("loadCampaignProgression: loading %s\n", loadGameList[i].c_str());
					i = fileList.Num() + 100; //break out of loop.
				}
			}

			continue;
		}

		//BC
		if (!idStr::Icmp(cmd, "startDebugGame")) {
			idStr si_map = cvarSystem->GetCVarString("si_map");
			if (si_map.IsEmpty())
				return;

			StartNewGame(si_map.c_str());
			// go back to the game sounds
			soundSystem->SetPlayingSoundWorld( sw );

			// unpause the game sound world
			if ( sw != NULL && sw->IsPaused() ) {
				sw->UnPause();
			}
			continue;
		}

		if ( !idStr::Icmp( cmd, "quit" ) ) {
			ExitMenu();
			common->Quit();
			return;
		}

#ifndef DEMO
		if ( !idStr::Icmp( cmd, "loadMod" ) ) {
			int choice = guiActive->State().GetInt( "modsList_sel_0" );
			if ( choice >= 0 && choice < modsList.Num() ) {
				// SM: This is a workshop mod if the text has a '|' in it
				// (Note: | is not a valid char in a windows path so should be ok)
				if (modsList[choice].Find('|') != -1) {
					idStrList workshopSplits = modsList[choice].Split('|');
					cvarSystem->SetCVarString("fs_cdpath", workshopSplits[0]);
					cvarSystem->SetCVarString("fs_game", workshopSplits[1]);
				} else {
					// SM: This is a local mod, so just set fs_game directly from the list
					// (and clear fs_cdpath just in case)
					cvarSystem->SetCVarString("fs_cdpath", "");
					cvarSystem->SetCVarString( "fs_game", modsList[ choice ] );
				}

				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "reloadEngine menu\n" );
			}
		}
#endif

		if ( !idStr::Icmp( cmd, "UpdateServers" ) ) {
			if ( guiActive->State().GetBool( "lanSet" ) ) {
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "LANScan" );
			} else {
				idAsyncNetwork::GetNETServers();
			}
			continue;
		}

		if ( !idStr::Icmp( cmd, "RefreshServers" ) ) {
			if ( guiActive->State().GetBool( "lanSet" ) ) {
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "LANScan" );
			} else {
				idAsyncNetwork::client.serverList.NetScan( );
			}
			continue;
		}

		if ( !idStr::Icmp( cmd, "FilterServers" ) ) {
			idAsyncNetwork::client.serverList.ApplyFilter( );
			continue;
		}

		if ( !idStr::Icmp( cmd, "sortServerName" ) ) {
			idAsyncNetwork::client.serverList.SetSorting( SORT_SERVERNAME );
			continue;
		}

		if ( !idStr::Icmp( cmd, "sortGame" ) ) {
			idAsyncNetwork::client.serverList.SetSorting( SORT_GAME );
			continue;
		}

		if ( !idStr::Icmp( cmd, "sortPlayers" ) ) {
			idAsyncNetwork::client.serverList.SetSorting( SORT_PLAYERS );
			continue;
		}

		if ( !idStr::Icmp( cmd, "sortPing" ) ) {
			idAsyncNetwork::client.serverList.SetSorting( SORT_PING );
			continue;
		}

		if ( !idStr::Icmp( cmd, "sortGameType" ) ) {
			idAsyncNetwork::client.serverList.SetSorting( SORT_GAMETYPE );
			continue;
		}

		if ( !idStr::Icmp( cmd, "sortMap" ) ) {
			idAsyncNetwork::client.serverList.SetSorting( SORT_MAP );
			continue;
		}

		if ( !idStr::Icmp( cmd, "serverList" ) ) {
			idAsyncNetwork::client.serverList.GUIUpdateSelected();
			continue;
		}

		if ( !idStr::Icmp( cmd, "LANConnect" ) ) {
			int sel = guiActive->State().GetInt( "serverList_selid_0" );
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "Connect %d\n", sel ) );
			return;
		}

		if ( !idStr::Icmp( cmd, "MAPScan" ) ) {
			const char *gametype = cvarSystem->GetCVarString( "si_gameType" );
			//if ( gametype == NULL || *gametype == 0 || idStr::Icmp( gametype, "singleplayer" ) == 0 )
			//{
			//	gametype = "Deathmatch";
			//}
			gametype = "singleplayer";

			int i, num;
			idStr si_map = cvarSystem->GetCVarString("si_map");
			const idDict *dict;

			guiMainMenu_MapList->Clear();
			guiMainMenu_MapList->SetSelection( 0 );
			num = fileSystem->GetNumMaps();
			for ( i = 0; i < num; i++ ) {
				dict = fileSystem->GetMapDecl( i );
				if ( dict && dict->GetBool( gametype ) ) {
					const char *mapName = dict->GetString( "name" );
					if ( mapName[ 0 ] == '\0' ) {
						mapName = dict->GetString( "path" );
					}
					mapName = common->GetLanguageDict()->GetString( mapName );
					guiMainMenu_MapList->Add( i, mapName );
					if ( !si_map.Icmp( dict->GetString( "path" ) ) ) {
						guiMainMenu_MapList->SetSelection( guiMainMenu_MapList->Num() - 1 );
					}
				}
			}
			i = guiMainMenu_MapList->GetSelection( NULL, 0 );
			if ( i >= 0 ) {
				dict = fileSystem->GetMapDecl( i);
			} else {
				dict = NULL;
			}
			cvarSystem->SetCVarString( "si_map", ( dict ? dict->GetString( "path" ) : "" ) );

			// set the current level shot
			UpdateMPLevelShot();
			continue;
		}

		if ( !idStr::Icmp( cmd, "click_mapList" ) ) {
			int mapNum = guiMainMenu_MapList->GetSelection( NULL, 0 );
			const idDict *dict = fileSystem->GetMapDecl( mapNum );
			if ( dict ) {
				cvarSystem->SetCVarString( "si_map", dict->GetString( "path" ) );

				//BC display map description from maps.def
				idStr mapdesc = dict->GetString("description");
				mapdesc = common->GetLanguageDict()->GetString(mapdesc);
				guiMainMenu->SetStateString("mapdescription", mapdesc.c_str());

				idStr mapname = dict->GetString("name");
				mapname = common->GetLanguageDict()->GetString(mapname);
				guiMainMenu->SetStateString("mapname", mapname.c_str());

			}
			UpdateMPLevelShot();
			continue;
		}

		//make the video card name display in video options menu.
		if(glConfig.renderer_short_string)
		{
			guiMainMenu->SetStateString("videocardname", glConfig.renderer_short_string);
		}


		if (soundSystem->soundcardname.Length() > 0)
		{
			guiMainMenu->SetStateString("soundcardname", soundSystem->soundcardname.c_str());
		}



		if ( !idStr::Icmp( cmd, "inetConnect" ) ) {
			const char	*s = guiMainMenu->State().GetString( "inetGame" );

			if ( !s || s[0] == 0 ) {
				// don't put the menu away if there isn't a valid selection
				continue;
			}

			cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "connect %s", s ) );
			return;
		}

		if ( !idStr::Icmp( cmd, "startMultiplayer" ) ) {
			int dedicated = guiActive->State().GetInt( "dedicated" );
			cvarSystem->SetCVarBool( "net_LANServer", guiActive->State().GetBool( "server_type" ) );
			if ( gui_configServerRate.GetInteger() > 0 ) {
				// guess the best rate for upstream, number of internet clients
				if ( gui_configServerRate.GetInteger() == 5 || cvarSystem->GetCVarBool( "net_LANServer" ) ) {
					cvarSystem->SetCVarInteger( "net_serverMaxClientRate", 25600 );
				} else {
					// internet players
					int n_clients = cvarSystem->GetCVarInteger( "si_maxPlayers" );
					if ( !dedicated ) {
						n_clients--;
					}
					int maxclients = 0;
					switch ( gui_configServerRate.GetInteger() ) {
						case 1:
							// 128 kbits
							cvarSystem->SetCVarInteger( "net_serverMaxClientRate", 8000 );
							maxclients = 2;
							break;
						case 2:
							// 256 kbits
							cvarSystem->SetCVarInteger( "net_serverMaxClientRate", 9500 );
							maxclients = 3;
							break;
						case 3:
							// 384 kbits
							cvarSystem->SetCVarInteger( "net_serverMaxClientRate", 10500 );
							maxclients = 4;
							break;
						case 4:
							// 512 and above..
							cvarSystem->SetCVarInteger( "net_serverMaxClientRate", 14000 );
							maxclients = 4;
							break;
					}
					if ( n_clients > maxclients ) {
						if ( MessageBox( MSG_OKCANCEL, va( common->GetLanguageDict()->GetString( "#str_04315" ), dedicated ? maxclients : Min( 8, maxclients + 1 ) ), common->GetLanguageDict()->GetString( "#str_04316" ), true, "OK" )[ 0 ] == '\0' ) {
							continue;
						}
						cvarSystem->SetCVarInteger( "si_maxPlayers", dedicated ? maxclients : Min( 8, maxclients + 1 ) );
					}
				}
			}

			/* BC remove this popup.
			if ( !dedicated && !cvarSystem->GetCVarBool( "net_LANServer" ) && cvarSystem->GetCVarInteger("si_maxPlayers") > 4 ) {
				// "Dedicated server mode is recommended for internet servers with more than 4 players. Continue in listen mode?"
				if ( !MessageBox( MSG_YESNO, common->GetLanguageDict()->GetString ( "#str_00100625" ), common->GetLanguageDict()->GetString ( "#str_00100626" ), true, "yes" )[ 0 ] ) {
					continue;
				}
			}
			*/

			if ( dedicated ) {
				cvarSystem->SetCVarInteger( "net_serverDedicated", 1 );
			} else {
				cvarSystem->SetCVarInteger( "net_serverDedicated", 0 );
			}



			ExitMenu();
			// may trigger a reloadEngine - APPEND
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "SpawnServer\n" );
			return;
		}

		if ( !idStr::Icmp( cmd, "mpSkin")) {
			idStr skin;
			if ( args.Argc() - icmd >= 1 ) {
				skin = args.Argv( icmd++ );
				cvarSystem->SetCVarString( "ui_skin", skin );
				SetMainMenuSkin();
			}
			continue;
		}

		if ( !idStr::Icmp( cmd, "close" ) ) {
			// if we aren't in a game, the menu can't be closed
			if ( mapSpawned ) {
				ExitMenu();
			}
			continue;
		}

		if ( !idStr::Icmp( cmd, "resetdefaults" ) ) {
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, "exec default.cfg" );
			guiMainMenu->SetKeyBindingNames();
			continue;
		}


		if ( !idStr::Icmp( cmd, "bind" ) ) {
			if ( args.Argc() - icmd >= 2 ) {
				int key = atoi( args.Argv( icmd++ ) );
				idStr bind = args.Argv( icmd++ );
				if ( idKeyInput::NumBinds( bind ) >= 2 && !idKeyInput::KeyIsBoundTo( key, bind ) ) {
					idKeyInput::UnbindBinding( bind );
				}
				idKeyInput::SetBinding( key, bind );
				guiMainMenu->SetKeyBindingNames();
			}
			continue;
		}

		if (!idStr::Icmp(cmd, "rebindKeyboard")) {
			if (args.Argc() - icmd >= 2) {
				int key = atoi(args.Argv(icmd++));
				idStr bind = args.Argv(icmd++);
				idKeyInput::UnbindKeyboardBinding( bind );
				idKeyInput::SetBinding(key, bind);
				guiMainMenu->SetKeyBindingNames();
			}
			continue;
		}

		if (!idStr::Icmp(cmd, "rebindGamepad")) {
			if (args.Argc() - icmd >= 2) {
				int key = atoi(args.Argv(icmd++));
				idStr bind = args.Argv(icmd++);
				idKeyInput::UnbindGamepadBinding(bind);
				idKeyInput::SetBinding(key, bind);
				guiMainMenu->SetKeyBindingNames();
			}
			continue;
		}

		if ( !idStr::Icmp( cmd, "play" ) ) {
			if ( args.Argc() - icmd >= 1 ) {
				idStr snd = args.Argv( icmd++ );
				int channel = SND_CHANNEL_BODY; // SW 18th Feb 2025: changing default GUI sound channel so it isn't affected by global voice volume scaling
				if ( snd.Length() == 1 ) {
					channel = atoi( snd );
					snd = args.Argv( icmd++ );
				}
				menuSoundWorld->PlayShaderDirectly( snd, channel );

			}
			continue;
		}

		if ( !idStr::Icmp( cmd, "music" ) ) {
			if ( args.Argc() - icmd >= 1 ) {
				idStr snd = args.Argv( icmd++ );
				menuSoundWorld->PlayShaderDirectly( snd, SCHANNEL_MUSIC);
			}
			continue;
		}

		// triggered from mainmenu or mpmain
		if ( !idStr::Icmp( cmd, "sound" ) ) {
			idStr vcmd;
			if ( args.Argc() - icmd >= 1 ) {
				vcmd = args.Argv( icmd++ );
			}
			if ( !vcmd.Length() || !vcmd.Icmp( "speakers" ) ) {
				int old = cvarSystem->GetCVarInteger( "s_numberOfSpeakers" );
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "s_restart\n" );
				if ( old != cvarSystem->GetCVarInteger( "s_numberOfSpeakers" ) ) {
#ifdef _WIN32
					MessageBox( MSG_OK, common->GetLanguageDict()->GetString( "#str_04142" ), common->GetLanguageDict()->GetString( "#str_04141" ), true );
#else
					// a message that doesn't mention the windows control panel
					MessageBox( MSG_OK, common->GetLanguageDict()->GetString( "#str_07230" ), common->GetLanguageDict()->GetString( "#str_04141" ), true );
#endif
				}
			}
			if ( !vcmd.Icmp( "eax" ) ) {
				if ( cvarSystem->GetCVarBool( "s_useEAXReverb" ) ) {
					int efx = soundSystem->IsEFXAvailable();
					switch ( efx ) {
					case 1:
						// when you restart
						MessageBox( MSG_OK, common->GetLanguageDict()->GetString( "#str_04137" ), common->GetLanguageDict()->GetString( "#str_07231" ), true );
						break;
					case -1:
						cvarSystem->SetCVarBool( "s_useEAXReverb", false );
						// disabled
						MessageBox( MSG_OK, common->GetLanguageDict()->GetString( "#str_07233" ), common->GetLanguageDict()->GetString( "#str_07231" ), true );
						break;
					case 0:
						cvarSystem->SetCVarBool( "s_useEAXReverb", false );
						// not available
						MessageBox( MSG_OK, common->GetLanguageDict()->GetString( "#str_07232" ), common->GetLanguageDict()->GetString( "#str_07231" ), true );
						break;
					}
				} else {
					// when you restart
					MessageBox( MSG_OK, common->GetLanguageDict()->GetString( "#str_04137" ), common->GetLanguageDict()->GetString( "#str_07231" ), true );
				}
			}
			if ( !vcmd.Icmp( "drivar" ) ) {
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "s_restart\n" );
			}
			continue;
		}

		if ( !idStr::Icmp( cmd, "video" ) ) {
			idStr vcmd;
			if ( args.Argc() - icmd >= 1 ) {
				vcmd = args.Argv( icmd++ );
			}

			int oldSpec = com_machineSpec.GetInteger();

			if ( idStr::Icmp( vcmd, "low" ) == 0 ) {
				com_machineSpec.SetInteger( 0 );
			} else if ( idStr::Icmp( vcmd, "medium" ) == 0 ) {
				com_machineSpec.SetInteger( 1 );
			} else  if ( idStr::Icmp( vcmd, "high" ) == 0 ) {
				com_machineSpec.SetInteger( 2 );
			} else  if ( idStr::Icmp( vcmd, "ultra" ) == 0 ) {
				com_machineSpec.SetInteger( 3 );
			} else if ( idStr::Icmp( vcmd, "recommended" ) == 0 ) {
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "setMachineSpec\n" );
			}

			if ( oldSpec != com_machineSpec.GetInteger() ) {
				guiActive->SetStateInt( "com_machineSpec", com_machineSpec.GetInteger() );
				guiActive->StateChanged( com_frameTime );
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "execMachineSpec\n" );
			}

			if ( idStr::Icmp( vcmd, "restart" )  == 0) {
				guiActive->HandleNamedEvent( "cvar write render" );
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "vid_restart partial\n" );
				guiActive->HandleNamedEvent( "resolution refresh" );
			}

			continue;
		}

		if ( !idStr::Icmp( cmd, "clearBind" ) ) {
			if ( args.Argc() - icmd >= 1 ) {
				idKeyInput::UnbindBinding( args.Argv( icmd++ ) );
				guiMainMenu->SetKeyBindingNames();
			}
			continue;
		}

		if (!idStr::Icmp(cmd, "clearKeyboardBind")) {
			if (args.Argc() - icmd >= 1) {
				idKeyInput::UnbindKeyboardBinding(args.Argv(icmd++));
				guiMainMenu->SetKeyBindingNames();
			}
			continue;
		}

		if (!idStr::Icmp(cmd, "clearGamepadBind")) {
			if (args.Argc() - icmd >= 1) {
				idKeyInput::UnbindGamepadBinding(args.Argv(icmd++));
				guiMainMenu->SetKeyBindingNames();
			}
			continue;
		}

		// FIXME: obsolete
		if ( !idStr::Icmp( cmd, "chatdone" ) ) {
			idStr temp = guiActive->State().GetString( "chattext" );
			temp += "\r";
			guiActive->SetStateString( "chattext", "" );
			continue;
		}

		if ( !idStr::Icmp ( cmd, "exec" ) ) {

			//Backup the language so we can restore it after defaults.
			idStr lang = cvarSystem->GetCVarString("sys_lang");

			cmdSystem->BufferCommandText( CMD_EXEC_NOW, args.Argv( icmd++ ) );
			if ( idStr::Icmp( "cvar_restart", args.Argv( icmd - 1 ) ) == 0 )
			{
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "exec default.cfg" );
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "setMachineSpec\n" );

				//Make sure that any r_brightness changes take effect
				float bright = cvarSystem->GetCVarFloat("r_brightness");
				cvarSystem->SetCVarFloat("r_brightness", 0.0f);
				cvarSystem->SetCVarFloat("r_brightness", bright);

				//Force user info modified after a reset to defaults
				cvarSystem->SetModifiedFlags(CVAR_USERINFO);

				guiActive->SetStateInt( "com_machineSpec", com_machineSpec.GetInteger() );

				//Restore the language
				cvarSystem->SetCVarString("sys_lang", lang);

			}
			continue;
		}

		if (!idStr::Icmp(cmd, "binds_reset" ) )
		{
			cmdSystem->BufferCommandText(CMD_EXEC_NOW, "exec default.cfg");
		}

		if (!idStr::Icmp(cmd, "binds_reset_keyboard"))
		{
			cmdSystem->BufferCommandText(CMD_EXEC_NOW, "exec keyboard.cfg");
		}


		if (!idStr::Icmp(cmd, "binds_reset_gamepad"))
		{
			cmdSystem->BufferCommandText(CMD_EXEC_NOW, "exec gamepad.cfg");
		}

		if ( !idStr::Icmp ( cmd, "loadBinds" ) ) {
			guiMainMenu->SetKeyBindingNames();
			continue;
		}

		//BC 2-24-2025
		if (!idStr::Icmp(cmd, "updategamepadpic"))
		{
			DisplayGamepadBind("joy1");
			DisplayGamepadBind("joy2");
			DisplayGamepadBind("joy3");
			DisplayGamepadBind("joy4");
			DisplayGamepadBind("joy5");
			DisplayGamepadBind("joy6");
			DisplayGamepadBind("joy7");
			DisplayGamepadBind("joy8");
			DisplayGamepadBind("joy9");
			DisplayGamepadBind("joy10");

			DisplayGamepadBind("joy_trigger1");
			DisplayGamepadBind("joy_trigger2");

			DisplayGamepadBind("joy_dpad_up");
			DisplayGamepadBind("joy_dpad_down");
			DisplayGamepadBind("joy_dpad_left");
			DisplayGamepadBind("joy_dpad_right");	

			//guiMainMenu->sets
			continue;
		}


		if ( !idStr::Icmp( cmd, "systemCvars" ) ) {
			guiActive->HandleNamedEvent( "cvar read render" );
			guiActive->HandleNamedEvent( "cvar read sound" );
			continue;
		}

		if ( !idStr::Icmp( cmd, "SetCDKey" ) ) {
			// we can't do this from inside the HandleMainMenuCommands code, otherwise the message box stuff gets confused
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "promptKey\n" );
			continue;
		}

		if ( !idStr::Icmp( cmd, "CheckUpdate" ) ) {
			idAsyncNetwork::client.SendVersionCheck();
			continue;
		}

		if ( !idStr::Icmp( cmd, "CheckUpdate2" ) ) {
			idAsyncNetwork::client.SendVersionCheck( true );
			continue;
		}

		if ( !idStr::Icmp( cmd, "checkKeys" ) ) {
#if ID_ENFORCE_KEY
			// not a strict check so you silently auth in the background without bugging the user
			if ( !session->CDKeysAreValid( false ) ) {
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "promptKey force" );
				cmdSystem->ExecuteCommandBuffer();
			}
#endif
			continue;
		}

		// triggered from mainmenu or mpmain
		if ( !idStr::Icmp( cmd, "punkbuster" ) ) {
			idStr vcmd;
			if ( args.Argc() - icmd >= 1 ) {
				vcmd = args.Argv( icmd++ );
			}
			// filtering PB based on enabled/disabled
			idAsyncNetwork::client.serverList.ApplyFilter( );
			SetPbMenuGuiVars();
			continue;
		}

		//-- start reportabug

		if (!idStr::Icmp(cmd, "sendbugreport"))
		{
			idStr rawText = guiMainMenu->State().GetString("reportabug_text");

			rawText = GetSanitizedURLArgument(rawText);

			if (rawText.Length())
			{
				//only send string if it has characters.
				idAsyncNetwork::client.SendBugreport(rawText.c_str());
			}

			continue;
		}


		//-- end reportabug


		//BC 5-6-2025: show last savegame time.
		if (!idStr::Icmp(cmd, "showlastsave"))
		{
			idStr lastSaveStr = GetLastSavetime();

			if (lastSaveStr.Length() > 0)
			{
				guiMainMenu->SetStateBool("lastsavevisible", true);
				guiMainMenu->SetStateString("lastsavetime", lastSaveStr.c_str());				
			}
			else
			{
				guiMainMenu->SetStateBool("lastsavevisible", false);
			}
		}
	}
}

//BC 5-6-2025: show last savegame time.
idStr idSessionLocal::GetLastSavetime()
{
	idStrList fileList;
	idList<fileTIME_T> fileTimes;
	GetSaveGameList(fileList, fileTimes);

	if (fileList.Num() > 0) //If there is at least 1 file in the save folder...
	{
		//Search for the highest timestamp. The highest timestamp is the most-recent savegame.
		int64 highestTimestamp = 0;
		for (int i = 0; i < fileList.Num(); i++)
		{
			if (fileTimes[i].timeStamp > highestTimestamp)
			{
				highestTimestamp = fileTimes[i].timeStamp;
			}
		}

		if (highestTimestamp > 0)
		{
			idStr diffString = ParseSavetime(highestTimestamp);
			if (diffString.Length() <= 0)
			{
				return "";
			}

			return diffString;
		}		
	}
	
	//no files. return empty.
	return "";
}


//BC 2-25-2025: make the gamepad display show what the current binds are.
void idSessionLocal::DisplayGamepadBind(idStr key)
{
	idStr guiVar = idStr::Format("gp_%s", key.c_str());
	idStr rawbind = common->BindingFromKey(key);
	if (rawbind.Length() > 0)
	{
		idStr locstr = gameLocal.GetBindingnameViaBind(rawbind);
		if (locstr.Length() > 0)
		{
			idStr displaybind = common->GetLanguageDict()->GetString(locstr);
			if (displaybind.Length() > 0)
			{
				guiMainMenu->SetStateString(guiVar.c_str(), displaybind.c_str());
				return;
			}
		}
	}

	guiMainMenu->SetStateString(guiVar.c_str(), " ");
}

idStr idSessionLocal::GetSanitizedURLArgument(idStr rawText)
{
	rawText.StripTrailingWhitespace();
	rawText.StripLeading(' ');

	//bc parse the text. This is pretty ugly! don't judge me!
	rawText.Replace(" ", "+");
	rawText.Replace("\"", "%22");
	rawText.Replace("#", "%23");
	rawText.Replace("&", "%26");
	rawText.Replace("'", "%27");
	rawText.Replace("`", "%27");
	rawText.Replace("(", "%28");
	rawText.Replace(")", "%29");

	rawText.Replace("*", "%2a");
	rawText.Replace(",", "%2c");
	rawText.Replace("-", "%2d");
	rawText.Replace(".", "%2e");
	rawText.Replace("/", "%2f");
	rawText.Replace(":", "%3a");
	rawText.Replace(";", "%3b");
	rawText.Replace("?", "%3f");
	rawText.Replace("@", "%40");
	rawText.Replace("[", "%5b");
	rawText.Replace("\\", "%5c");
	rawText.Replace("]", "%5d");

	rawText.Replace("^", "%5e");
	rawText.Replace("_", "%5f");
	rawText.Replace("{", "%7b");
	rawText.Replace("|", "%7c");
	rawText.Replace("}", "%7d");
	rawText.Replace("~", "%7e");

	rawText.Replace("=", "%3d");

	rawText.Replace("\"", "%93");
	rawText.Replace("\"", "%94");
	rawText.Replace("-", "%97");
	rawText.Replace("\n", "%0A");

	return rawText;
}


idStr idSessionLocal::GetPlayerLocationString()
{
	idStr infostring = "";
	bool foundLevel = false;
	if (session->GetCurrentMapName() && strlen(session->GetCurrentMapName()))
	{
		infostring += idStr::Format("%s ", session->GetCurrentMapName());
		foundLevel = true;
	}

	if (!foundLevel)
	{
		infostring += idStr::Format("mainmenu ");
	}

	if (gameLocal.GetLocalPlayer())
	{
		const renderView_t* view = gameLocal.GetLocalPlayer()->GetRenderView();
		if (view) {
			infostring += idStr::Format("View: %.1f %.1f %.1f  %.0f -%.0f\n", view->vieworg.x, view->vieworg.y, view->vieworg.z, view->viewaxis[0].ToYaw(), view->viewaxis[0].ToPitch());
		}
		else
		{
			idVec3		origin;
			idMat3		axis;
			gameLocal.GetLocalPlayer()->GetViewPos(origin, axis);
			infostring += idStr::Format("View: %.1f %.1f %.1f  %.0f -%.0f\n", origin.x, origin.y, origin.z, axis[0].ToYaw(), axis[0].ToPitch());
		}
	}

	infostring = GetSanitizedURLArgument(infostring);
	return infostring;
}

/*
==============
idSessionLocal::HandleChatMenuCommands

Executes any commands returned by the gui
==============
*/
void idSessionLocal::HandleChatMenuCommands( const char *menuCommand ) {
	// execute the command from the menu
	int i;
	idCmdArgs args;

	args.TokenizeString( menuCommand, false );

	for ( i = 0; i < args.Argc(); ) {
		const char *cmd = args.Argv( i++ );

		if ( idStr::Icmp( cmd, "chatactive" ) == 0 ) {
			//chat.chatMode = CHAT_GLOBAL;
			continue;
		}
		if ( idStr::Icmp( cmd, "chatabort" ) == 0 ) {
			//chat.chatMode = CHAT_NONE;
			continue;
		}
		if ( idStr::Icmp( cmd, "netready" ) == 0 ) {
			bool b = cvarSystem->GetCVarBool( "ui_ready" );
			cvarSystem->SetCVarBool( "ui_ready", !b );
			continue;
		}
		if ( idStr::Icmp( cmd, "netstart" ) == 0 ) {
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, "netcommand start\n" );
			continue;
		}
	}
}

/*
==============
idSessionLocal::HandleInGameCommands

Executes any commands returned by the gui
==============
*/
void idSessionLocal::HandleInGameCommands( const char *menuCommand ) {
	idPlayer* player = gameLocal.GetLocalPlayer();
	// execute the command from the menu
	idCmdArgs args;

	args.TokenizeString( menuCommand, false );

	const char *cmd = args.Argv( 0 );
	if ( !idStr::Icmp( cmd, "close" ) ) {
		if ( guiActive ) {
			sysEvent_t  ev;
			ev.evType = SE_NONE;
			guiActive->HandleEvent( &ev, com_frameTime );
			guiActive->Activate( false, com_frameTime );
			guiActive = NULL;
		}
	}
	// SM: If the player is in a fullscreen UI, we need to reroute the command to the player
	else if (player && player->IsFullscreenUIActive())
	{
		idLexer lexer;
		lexer.LoadMemory(menuCommand, idStr::Length(menuCommand), "cmd", 0);
		player->HandleSingleGuiCommand(nullptr, &lexer);
	}
}

/*
==============
idSessionLocal::DispatchCommand
==============
*/
void idSessionLocal::DispatchCommand( idUserInterface *gui, const char *menuCommand, bool doIngame ) {

	if ( !gui ) {
		gui = guiActive;
	}

	if ( gui == guiMainMenu ) {
		HandleMainMenuCommands( menuCommand );
		return;
	} else if ( gui == guiIntro) {
		HandleIntroMenuCommands( menuCommand );
	} else if ( gui == guiMsg ) {
		HandleMsgCommands( menuCommand );
	} else if ( gui == guiTakeNotes ) {
		HandleNoteCommands( menuCommand );
	} else if ( gui == guiRestartMenu ) {
		HandleRestartMenuCommands( menuCommand );
	} else if ( game && guiActive && guiActive->State().GetBool( "gameDraw" ) ) {
		const char *cmd = game->HandleGuiCommands( menuCommand );
		if ( !cmd ) {
			guiActive = NULL;
		} else if ( idStr::Icmp( cmd, "main" ) == 0 ) {
			StartMenu();
		} else if ( strstr( cmd, "sound " ) == cmd ) {
			// pipe the GUI sound commands not handled by the game to the main menu code
			HandleMainMenuCommands( cmd );
		}
	} else if ( guiHandle ) {
		if ( (*guiHandle)( menuCommand ) ) {
			return;
		}
	} else if ( !doIngame ) {
		common->DPrintf( "idSessionLocal::DispatchCommand: no dispatch found for command '%s'\n", menuCommand );
	}

	if ( doIngame ) {
		HandleInGameCommands( menuCommand );
	}
}


/*
==============
idSessionLocal::MenuEvent

Executes any commands returned by the gui
==============
*/
void idSessionLocal::MenuEvent( const sysEvent_t *event ) {
	const char	*menuCommand;

	if ( guiActive == NULL ) {
		return;
	}

	menuCommand = guiActive->HandleEvent( event, com_frameTime );

	if ( !menuCommand || !menuCommand[0] ) {
		// If the menu didn't handle the event, and it's a key down event for an F key, run the bind
		if ( event->evType == SE_KEY && event->evValue2 == 1 && event->evValue >= K_F1 && event->evValue <= K_F12 ) {
			idKeyInput::ExecKeyBinding( event->evValue );
		}
		return;
	}

	DispatchCommand( guiActive, menuCommand );
}

/*
=================
idSessionLocal::GuiFrameEvents
=================
*/
void idSessionLocal::GuiFrameEvents() {
	const char	*cmd;
	sysEvent_t  ev;
	idUserInterface	*gui;

	// stop generating move and button commands when a local console or menu is active
	// running here so SP, async networking and no game all go through it
	if ( console->Active() || guiActive ) {
		usercmdGen->InhibitUsercmd( INHIBIT_SESSION, true );
	} else {
		usercmdGen->InhibitUsercmd( INHIBIT_SESSION, false );
	}

	if ( guiTest ) {
		gui = guiTest;
	} else if ( guiActive ) {
		gui = guiActive;
	} else {
		return;
	}

	memset( &ev, 0, sizeof( ev ) );

	ev.evType = SE_NONE;
	cmd = gui->HandleEvent( &ev, com_frameTime );
	if ( cmd && cmd[0] ) {
		DispatchCommand( guiActive, cmd );
	}
}

/*
=================
idSessionLocal::BoxDialogSanityCheck
=================
*/
bool idSessionLocal::BoxDialogSanityCheck( void ) {
	if ( !common->IsInitialized() ) {
		common->DPrintf( "message box sanity check: !common->IsInitialized()\n" );
		return false;
	}
	if ( !guiMsg ) {
		return false;
	}
	if ( guiMsgRestore ) {
		common->DPrintf( "message box sanity check: recursed\n" );
		return false;
	}
	if ( cvarSystem->GetCVarInteger( "net_serverDedicated" ) ) {
		common->DPrintf( "message box sanity check: not compatible with dedicated server\n" );
		return false;
	}
	return true;
}

/*
=================
idSessionLocal::MessageBox
=================
*/
const char* idSessionLocal::MessageBox( msgBoxType_t type, const char *message, const char *title, bool wait, const char *fire_yes, const char *fire_no, bool network ) {

	common->DPrintf( "MessageBox: %s - %s\n", title ? title : "", message ? message : "" );

	if ( !BoxDialogSanityCheck() ) {
		return NULL;
	}

	guiMsg->SetStateString( "title", title ? title : "" );
	guiMsg->SetStateString( "message", message ? message : "" );
	if ( type == MSG_WAIT ) {
		guiMsg->SetStateString( "visible_msgbox", "0" );
		guiMsg->SetStateString( "visible_waitbox", "1" );
	} else {
		guiMsg->SetStateString( "visible_msgbox", "1" );
		guiMsg->SetStateString( "visible_waitbox", "0" );
	}

	guiMsg->SetStateString( "visible_entry", "0" );
	guiMsg->SetStateString( "visible_cdkey", "0" );
	switch ( type ) {
		case MSG_INFO:
			guiMsg->SetStateString( "mid", "" );
			guiMsg->SetStateString( "visible_mid", "0" );
			guiMsg->SetStateString( "visible_left", "0" );
			guiMsg->SetStateString( "visible_right", "0" );
			break;
		case MSG_OK:
			guiMsg->SetStateString( "mid", common->GetLanguageDict()->GetString( "#str_04339" ) );
			guiMsg->SetStateString( "visible_mid", "1" );
			guiMsg->SetStateString( "visible_left", "0" );
			guiMsg->SetStateString( "visible_right", "0" );
			break;
		case MSG_ABORT:
			guiMsg->SetStateString( "mid", common->GetLanguageDict()->GetString( "#str_04340" ) );
			guiMsg->SetStateString( "visible_mid", "1" );
			guiMsg->SetStateString( "visible_left", "0" );
			guiMsg->SetStateString( "visible_right", "0" );
			break;
		case MSG_OKCANCEL:
			guiMsg->SetStateString( "left", common->GetLanguageDict()->GetString( "#str_04339" ) );
			guiMsg->SetStateString( "right", common->GetLanguageDict()->GetString( "#str_04340" ) );
			guiMsg->SetStateString( "visible_mid", "0" );
			guiMsg->SetStateString( "visible_left", "1" );
			guiMsg->SetStateString( "visible_right", "1" );
			break;
		case MSG_YESNO:
			guiMsg->SetStateString( "left", common->GetLanguageDict()->GetString( "#str_04341" ) );
			guiMsg->SetStateString( "right", common->GetLanguageDict()->GetString( "#str_04342" ) );
			guiMsg->SetStateString( "visible_mid", "0" );
			guiMsg->SetStateString( "visible_left", "1" );
			guiMsg->SetStateString( "visible_right", "1" );
			break;
		case MSG_PROMPT:
			guiMsg->SetStateString( "left", common->GetLanguageDict()->GetString( "#str_04339" ) );
			guiMsg->SetStateString( "right", common->GetLanguageDict()->GetString( "#str_04340" ) );
			guiMsg->SetStateString( "visible_mid", "0" );
			guiMsg->SetStateString( "visible_left", "1" );
			guiMsg->SetStateString( "visible_right", "1" );
			guiMsg->SetStateString( "visible_entry", "1" );
			guiMsg->HandleNamedEvent( "Prompt" );
			break;
		case MSG_CDKEY:
			guiMsg->SetStateString( "left", common->GetLanguageDict()->GetString( "#str_04339" ) );
			guiMsg->SetStateString( "right", common->GetLanguageDict()->GetString( "#str_04340" ) );
			guiMsg->SetStateString( "visible_msgbox", "0" );
			guiMsg->SetStateString( "visible_cdkey", "1" );
			guiMsg->SetStateString( "visible_hasxp", fileSystem->HasD3XP() ? "1" : "0" );
			// the current cdkey / xpkey values may have bad/random data in them
			// it's best to avoid printing them completely, unless the key is good
			if ( cdkey_state == CDKEY_OK ) {
				guiMsg->SetStateString( "str_cdkey", cdkey );
				guiMsg->SetStateString( "visible_cdchk", "0" );
			} else {
				guiMsg->SetStateString( "str_cdkey", "" );
				guiMsg->SetStateString( "visible_cdchk", "1" );
			}
			guiMsg->SetStateString( "str_cdchk", "" );
			if ( xpkey_state == CDKEY_OK ) {
				guiMsg->SetStateString( "str_xpkey", xpkey );
				guiMsg->SetStateString( "visible_xpchk", "0" );
			} else {
				guiMsg->SetStateString( "str_xpkey", "" );
				guiMsg->SetStateString( "visible_xpchk", "1" );
			}
			guiMsg->SetStateString( "str_xpchk", "" );
			guiMsg->HandleNamedEvent( "CDKey" );
			break;
		case MSG_WAIT:
			break;
		default:
			common->Printf( "idSessionLocal::MessageBox: unknown msg box type\n" );
	}
	msgFireBack[ 0 ] = fire_yes ? fire_yes : "";
	msgFireBack[ 1 ] = fire_no ? fire_no : "";
	guiMsgRestore = guiActive;
	guiActive = guiMsg;
	guiMsg->SetCursor( 325, 290 );
	guiActive->Activate( true, com_frameTime );
	msgRunning = true;
	msgRetIndex = -1;

	if ( wait ) {
		// play one frame ignoring events so we don't get confused by parasite button releases
		msgIgnoreButtons = true;
		common->GUIFrame( true, network );
		msgIgnoreButtons = false;
		while ( msgRunning ) {
			common->GUIFrame( true, network );
		}
		if ( msgRetIndex < 0 ) {
			// MSG_WAIT and other StopBox calls
			return NULL;
		}
		if ( type == MSG_PROMPT ) {
			if ( msgRetIndex == 0 ) {
				guiMsg->State().GetString( "str_entry", "", msgFireBack[ 0 ] );
				return msgFireBack[ 0 ].c_str();
			} else {
				return NULL;
			}
		} else if ( type == MSG_CDKEY ) {
			if ( msgRetIndex == 0 ) {
				// the visible_ values distinguish looking at a valid key, or editing it
				sprintf( msgFireBack[ 0 ], "%1s;%16s;%2s;%1s;%16s;%2s",
						 guiMsg->State().GetString( "visible_cdchk" ),
						 guiMsg->State().GetString( "str_cdkey" ),
						 guiMsg->State().GetString( "str_cdchk" ),
						 guiMsg->State().GetString( "visible_xpchk" ),
						 guiMsg->State().GetString( "str_xpkey" ),
						 guiMsg->State().GetString( "str_xpchk" ) );
				return msgFireBack[ 0 ].c_str();
			} else {
				return NULL;
			}
		} else {
			return msgFireBack[ msgRetIndex ].c_str();
		}
	}
	return NULL;
}

/*
=================
idSessionLocal::DownloadProgressBox
=================
*/
void idSessionLocal::DownloadProgressBox( backgroundDownload_t *bgl, const char *title, int progress_start, int progress_end ) {
	int dlnow = 0, dltotal = 0;
	int startTime = Sys_Milliseconds();
	int lapsed;
	idStr sNow, sTotal, sBW, sETA, sMsg;

	if ( !BoxDialogSanityCheck() ) {
		return;
	}

	guiMsg->SetStateString( "visible_msgbox", "1" );
	guiMsg->SetStateString( "visible_waitbox", "0" );

	guiMsg->SetStateString( "visible_entry", "0" );
	guiMsg->SetStateString( "visible_cdkey", "0" );

	guiMsg->SetStateString( "mid", "Cancel" );
	guiMsg->SetStateString( "visible_mid", "1" );
	guiMsg->SetStateString( "visible_left", "0" );
	guiMsg->SetStateString( "visible_right", "0" );

	guiMsg->SetStateString( "title", title );
	guiMsg->SetStateString( "message", "Connecting.." );

	guiMsgRestore = guiActive;
	guiActive = guiMsg;
	msgRunning = true;

	while ( 1 ) {
		while ( msgRunning ) {
			common->GUIFrame( true, false );
			if ( bgl->completed ) {
				guiActive = guiMsgRestore;
				guiMsgRestore = NULL;
				return;
			} else if ( bgl->url.dltotal != dltotal || bgl->url.dlnow != dlnow ) {
				dltotal = bgl->url.dltotal;
				dlnow = bgl->url.dlnow;
				lapsed = Sys_Milliseconds() - startTime;
				sNow.BestUnit( "%.2f", dlnow, MEASURE_SIZE );
				if ( lapsed > 2000 ) {
					sBW.BestUnit( "%.1f", ( 1000.0f * dlnow ) / lapsed, MEASURE_BANDWIDTH );
				} else {
					sBW = "-- KB/s";
				}
				if ( dltotal ) {
					sTotal.BestUnit( "%.2f", dltotal, MEASURE_SIZE );
					if ( lapsed < 2000 ) {
						sprintf( sMsg, "%s / %s", sNow.c_str(), sTotal.c_str() );
					} else {
						sprintf( sETA, "%.0f sec", ( (float)dltotal / (float)dlnow - 1.0f ) * lapsed / 1000 );
						sprintf( sMsg, "%s / %s ( %s - %s )", sNow.c_str(), sTotal.c_str(), sBW.c_str(), sETA.c_str() );
					}
				} else {
					if ( lapsed < 2000 ) {
						sMsg = sNow;
					} else {
						sprintf( sMsg, "%s - %s", sNow.c_str(), sBW.c_str() );
					}
				}
				if ( dltotal ) {
					guiMsg->SetStateString( "progress", va( "%d", progress_start + dlnow * ( progress_end - progress_start ) / dltotal ) );
				} else {
					guiMsg->SetStateString( "progress", "0" );
				}
				guiMsg->SetStateString( "message", sMsg.c_str() );
			}
		}
		// abort was used - tell the downloader and wait till final stop
		bgl->url.status = DL_ABORTING;
		guiMsg->SetStateString( "title", "Aborting.." );
		guiMsg->SetStateString( "visible_mid", "0" );
		// continue looping
		guiMsgRestore = guiActive;
		guiActive = guiMsg;
		msgRunning = true;
	}
}

/*
=================
idSessionLocal::StopBox
=================
*/
void idSessionLocal::StopBox() {
	if ( guiActive == guiMsg ) {
		HandleMsgCommands( "stop" );
	}
}

/*
=================
idSessionLocal::HandleMsgCommands
=================
*/
void idSessionLocal::HandleMsgCommands( const char *menuCommand ) {
	assert( guiActive == guiMsg );
	// "stop" works even on first frame
	if ( idStr::Icmp( menuCommand, "stop" ) == 0 ) {
		// force hiding the current dialog
		guiActive = guiMsgRestore;
		guiMsgRestore = NULL;
		msgRunning = false;
		msgRetIndex = -1;
	}
	if ( msgIgnoreButtons ) {
		common->DPrintf( "MessageBox HandleMsgCommands 1st frame ignore\n" );
		return;
	}
	if ( idStr::Icmp( menuCommand, "mid" ) == 0 || idStr::Icmp( menuCommand, "left" ) == 0 ) {
		guiActive = guiMsgRestore;
		guiMsgRestore = NULL;
		msgRunning = false;
		msgRetIndex = 0;
		DispatchCommand( guiActive, msgFireBack[ 0 ].c_str() );
	} else if ( idStr::Icmp( menuCommand, "right" ) == 0 ) {
		guiActive = guiMsgRestore;
		guiMsgRestore = NULL;
		msgRunning = false;
		msgRetIndex = 1;
		DispatchCommand( guiActive, msgFireBack[ 1 ].c_str() );
	}
}

/*
=================
idSessionLocal::HandleNoteCommands
=================
*/
#define NOTEDATFILE "C:/notenumber.dat"

void idSessionLocal::HandleNoteCommands( const char *menuCommand ) {
	guiActive = NULL;

	if ( idStr::Icmp( menuCommand,  "note" ) == 0 && mapSpawned )
	{
		idFile *file = NULL;
		for ( int tries = 0; tries < 10; tries++ ) {
			file = fileSystem->OpenExplicitFileRead( NOTEDATFILE );
			if ( file != NULL ) {
				break;
			}
			Sys_Sleep( 100 );
		}
		int noteNumber = 1000;
		if ( file ) {
			file->Read( &noteNumber, 4 );
			fileSystem->CloseFile( file );
		}

		int i;
		idStr str, noteNum, shotName, workName, fileName = "viewnotes/";
		idStrList fileList;

		const char *severity = NULL;
		const char *p = guiTakeNotes->State().GetString( "notefile" );
		if ( p == NULL || *p == '\0' ) {
			p = cvarSystem->GetCVarString( "ui_name" );
		}

		//bool extended = guiTakeNotes->State().GetBool( "extended" );
		//if ( extended ) {
		//	if ( guiTakeNotes->State().GetInt( "severity" ) == 1 ) {
		//		severity = "WishList_Viewnotes/";
		//	} else {
		//		severity = "MustFix_Viewnotes/";
		//	}
		//	fileName += severity;
		//
		//	const idDecl *mapDecl = declManager->FindType(DECL_ENTITYDEF, mapSpawnData.serverInfo.GetString( "si_map" ), false );
		//	const idDeclEntityDef *mapInfo = static_cast<const idDeclEntityDef *>(mapDecl);
		//
		//	if ( mapInfo ) {
		//		fileName += mapInfo->dict.GetString( "devname" );
		//	} else {
		//		fileName += mapSpawnData.serverInfo.GetString( "si_map" );
		//		fileName.StripFileExtension();
		//	}
		//
		//	int count = guiTakeNotes->State().GetInt( "person_numsel" );
		//	if ( count == 0 ) {
		//		fileList.Append( fileName + "/Nobody" );
		//	} else {
		//		for ( i = 0; i < count; i++ ) {
		//			int person = guiTakeNotes->State().GetInt( va( "person_sel_%i", i ) );
		//			workName = fileName + "/";
		//			workName += guiTakeNotes->State().GetString( va( "person_item_%i", person ), "Nobody" );
		//			fileList.Append( workName );
		//		}
		//	}
		//}
		//else
		{
			fileName += "maps/";
			fileName += mapSpawnData.serverInfo.GetString( "si_map" );
			fileName.StripFileExtension();
			fileList.Append( fileName );
		}

		bool bCon = cvarSystem->GetCVarBool( "con_noPrint" );
		cvarSystem->SetCVarBool( "con_noPrint", true );
		for ( i = 0; i < fileList.Num(); i++ ) {
			workName = fileList[i];
			workName += "/";
			workName += p;
			int workNote = noteNumber;
			R_ScreenshotFilename( workNote, workName, shotName );

			noteNum = shotName;
			noteNum.StripPath();
			noteNum.StripFileExtension();

			if ( severity && *severity ) {
				workName = severity;
				workName += "viewNotes";
			}

			sprintf( str, "recordViewNotes \"%s\" \"%s\" \"%s\"\n", workName.c_str(), noteNum.c_str(), guiTakeNotes->State().GetString( "note" ) );

			cmdSystem->BufferCommandText( CMD_EXEC_NOW, str );
			cmdSystem->ExecuteCommandBuffer();

			UpdateScreen();
			renderSystem->TakeScreenshot( renderSystem->GetScreenWidth(), renderSystem->GetScreenHeight(), shotName, 1, NULL );
		}
		noteNumber++;

		for ( int tries = 0; tries < 10; tries++ ) {
			file = fileSystem->OpenExplicitFileWrite( "c:/viewnotes/notenumber.dat" );
			if ( file != NULL ) {
				break;
			}
			Sys_Sleep( 100 );
		}
		if ( file ) {
			file->Write( &noteNumber, 4 );
			fileSystem->CloseFile( file );
		}

		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "closeViewNotes\n" );
		cvarSystem->SetCVarBool( "con_noPrint", bCon );
	}
}

/*
===============
idSessionLocal::SetCDKeyGuiVars
===============
*/
void idSessionLocal::SetCDKeyGuiVars( void ) {
	if ( !guiMainMenu ) {
		return;
	}
	//guiMainMenu->SetStateString( "str_d3key_state", common->GetLanguageDict()->GetString( va( "#str_071%d", 86 + cdkey_state ) ) );
	//guiMainMenu->SetStateString( "str_xpkey_state", common->GetLanguageDict()->GetString( va( "#str_071%d", 86 + xpkey_state ) ) );
}
