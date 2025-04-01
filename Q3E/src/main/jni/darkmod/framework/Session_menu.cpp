/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop



#include "Session_local.h"
#include "../game/Missions/MissionManager.h"

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
idSessionLocal::ResetMainMenu
==============
*/
void idSessionLocal::ResetMainMenu() {
	if (guiMainMenu) {
		DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Destroying main menu GUI");
		if (guiActive == guiMainMenu) {
			SetGUI(NULL, NULL);
			assert(guiActive != guiMainMenu);
		}

		// stgatilov #6509: copy persistent info from main menu gui vars
		// make game source of truth
		if ( gameLocal.persistentLevelInfoLocation == PERSISTENT_LOCATION_MAINMENU ) {
			gameLocal.SyncPersistentInfoFromGui( sessLocal.guiMainMenu, true );
		}

		uiManager->DeAlloc(guiMainMenu);
		guiMainMenu = NULL;
	}
}

/*
==============
idSessionLocal::SetMainMenuStartAtBriefing
==============
*/
void idSessionLocal::SetMainMenuStartAtBriefing() {
	mainMenuStartState = MMSS_BRIEFING;
}

/*
==============
idSessionLocal::CreateMainMenu
==============
*/
void idSessionLocal::CreateMainMenu() {
	if (guiMainMenu) {
		//we won't recreate main menu GUI until someone calls ResetMainMenu
		return;
	}

	DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Recreating main menu GUI");

	idDict presetDefines;
	// flag for in-game menu
	presetDefines.SetBool("MM_INGAME", mapSpawned);
	int missionIdx = 0;
	assert(gameLocal.m_MissionManager);
	if (gameLocal.m_MissionManager)
		missionIdx = gameLocal.m_MissionManager->GetCurrentMissionIndex() + 1;
	presetDefines.SetInt("MM_CURRENTMISSION", missionIdx);
	guiMainMenu = uiManager->FindGui( "guis/mainmenu.gui", true, false, true, presetDefines );

	guiMainMenu->SetStateInt( "inGame", presetDefines.GetInt("MM_INGAME") );
	guiMainMenu->SetStateInt("CurrentMission", presetDefines.GetInt("MM_CURRENTMISSION") );
	// switch to initial state
	guiMainMenu->SetStateInt("mode", guiMainMenu->GetStateInt("#MM_STATE_NONE"));
	if ( mainMenuStartState == MMSS_MAINMENU ) {
		guiMainMenu->SetStateInt("targetmode", guiMainMenu->GetStateInt("#MM_STATE_MAINMENU"));
	} else if ( mainMenuStartState == MMSS_SUCCESS ) {
		// simulate switch forward into DEBRIEFING_VIDEO, so that state skipping works properly
		guiMainMenu->SetStateInt("mode", guiMainMenu->GetStateInt("#MM_STATE_FINISHED"));
		guiMainMenu->SetStateInt("targetmode", guiMainMenu->GetStateInt("#MM_STATE_FORWARD"));
	} else if ( mainMenuStartState == MMSS_FAILURE ) {
		guiMainMenu->SetStateInt("targetmode", guiMainMenu->GetStateInt("#MM_STATE_FAILURE"));
	} else if ( mainMenuStartState == MMSS_BRIEFING ) {
		guiMainMenu->SetStateInt("targetmode", guiMainMenu->GetStateInt("#MM_STATE_BRIEFING_VIDEO"));
	} else {
		assert(false);
	}
	// note: MainMenuStartUp takes the state and handles it
	mainMenuStartState = MMSS_MAINMENU;

	// stgatilov #6509: copy persistent info to main menu gui vars
	// unless this is "ingame" menu, make menu GUI source of truth
	if ( gameLocal.persistentLevelInfoLocation == PERSISTENT_LOCATION_GAME ) {
		gameLocal.SyncPersistentInfoToGui( sessLocal.guiMainMenu, gameLocal.GameState() != GAMESTATE_ACTIVE );
	}
}

/*
==============
idSessionLocal::StartMainMenu
==============
*/
void idSessionLocal::StartMenu( bool playIntro ) {
	if ( guiActive && guiActive == guiMainMenu ) {
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

	// make sure guiMainMenu is alive
	CreateMainMenu();

	SetGUI( guiMainMenu, NULL );
	guiMainMenu->HandleNamedEvent( playIntro ? "playIntro" : "noIntro" );

	guiMainMenu->SetStateString("game_list", common->Translate( "#str_07212" ));

	console->Close();
}

/*
=================
idSessionLocal::GetGUI
=================
*/
idUserInterface *idSessionLocal::GetGui(GuiType type) const {
	if (type == gtActive)
		return guiActive;
	if (type == gtMainMenu)
		return guiMainMenu;
	if (type == gtLoading)
		return guiLoading;
	//TODO: do we ever need other cases?
	return NULL;
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
	}

	sysEvent_t  ev;
	memset( &ev, 0, sizeof( ev ) );
	ev.evType = SE_NONE;

	guiActive->HandleEvent( &ev, com_frameTime );
	guiActive->Activate( true, com_frameTime );
}

bool idSessionLocal::RunGuiScript(const char *windowName, int scriptNum) {
	idUserInterface *ui = guiActive;
	if (!ui)
		return false;
	const char *command = ui->RunGuiScript(windowName, scriptNum);
	if (!command)
		return false;
	DispatchCommand(ui, command);
	return true;
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

	// NOTE: no fs_mod for savegames -- fan mission name stored in fs_currentfm
	idStr game = cvarSystem->GetCVarString( "fs_currentfm" );
	if( game.Length() ) {
		files = fileSystem->ListFiles( "savegames", ".save", false, false, game );
	} else {
		files = fileSystem->ListFiles( "savegames", ".save" );
	}
	
	fileList = files->GetList();
	fileSystem->FreeFileList( files );

	for ( i = 0; i < fileList.Num(); i++ ) {
		ID_TIME_T timeStamp;

		fileSystem->ReadFile( "savegames/" + fileList[i], NULL, &timeStamp );
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

		idLexer src(LEXFL_NOERRORS|LEXFL_NOSTRINGCONCAT);
		if ( src.LoadFile( va("savegames/%s.txt", loadGameList[i].c_str()) ) ) {
			idToken tok;
			src.ReadToken( &tok );
			name = tok;
		} else {
			name = loadGameList[i];
		}

		name += "\t";

		idStr date = Sys_TimeStampToStr( fileTimes[i].timeStamp );
		name += date;

		guiActive->SetStateString( va("loadgame_item_%i", i), name);
	}
	guiActive->DeleteStateVar( va("loadgame_item_%i", fileList.Num()) );

	guiActive->SetStateString( "loadgame_sel_0", "-1" );
	guiActive->SetStateString( "loadgame_shot", "guis/assets/blankLevelShot" );
}

/*
===============
idSessionLocal::SetModsMenuGuiVars
===============
*/
void idSessionLocal::SetModsMenuGuiVars( void ) {
	int i;
	idModList *list = fileSystem->ListMods();

	modsList.SetNum( list->GetNumMods() );

	// Build the gui list
	for ( i = 0; i < list->GetNumMods(); i++ ) {
		guiActive->SetStateString( va("modsList_item_%i", i), list->GetDescription( i ) );
		modsList[i] = list->GetMod( i );
	}
	guiActive->DeleteStateVar( va("modsList_item_%i", list->GetNumMods()) );
	guiActive->SetStateString( "modsList_sel_0", "-1" );

	fileSystem->FreeModList( list );
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

	//guiMainMenu->SetStateInt( "com_machineSpec", com_machineSpec.GetInteger() );

	// "inetGame" will hold a hand-typed inet address, which is not archived to a cvar
	guiMainMenu->SetStateString( "inetGame", "" );

	// key bind names
	guiMainMenu->SetKeyBindingNames();

	guiMainMenu->SetStateString( "browser_levelshot", "guis/assets/splash/pdtempa" );

	SetMainMenuSkin();
	// Mods Menu
	SetModsMenuGuiVars();

	guiMainMenu->SetStateString( "driver_prompt", "0" );

	SetPbMenuGuiVars();
}

/*
==============
idSessionLocal::HandleSaveGameMenuCommands
==============
*/
bool idSessionLocal::HandleSaveGameMenuCommand( idCmdArgs &args, int &icmd ) {

	const char *cmd = args.Argv(icmd-1);

	if ( !idStr::Icmp( cmd, "loadGame" ) ) {
		int choice = guiActive->State().GetInt("loadgame_sel_0");
		if ( choice >= 0 && choice < loadGameList.Num() ) {
			sessLocal.LoadGame( loadGameList[choice] );
		}
		return true;
	}
	
	if (!idStr::Icmp(cmd, "loadgameforced")) {
		sessLocal.LoadGame("", idSessionLocal::eSaveConflictHandling_Ignore);
		return true;
	}
	
	if (!idStr::Icmp(cmd, "loadgameinitialized")) {
		sessLocal.LoadGame("", idSessionLocal::eSaveConflictHandling_LoadMapStart);
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

				idStr game = cvarSystem->GetCVarString( "fs_currentfm" );
				idFile *file;
				if(game.Length()) {
                    file = fileSystem->OpenFileRead( saveFileName, game );
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

	if ( !idStr::Icmp( cmd, "deleteGame" ) ) {
		int choice = guiActive->State().GetInt( "loadgame_sel_0" );
		if ( choice >= 0 && choice < loadGameList.Num() ) {
			fileSystem->RemoveFile( va("savegames/%s.save", loadGameList[choice].c_str()) );
			fileSystem->RemoveFile( va("savegames/%s.tga", loadGameList[choice].c_str()) );
			fileSystem->RemoveFile( va("savegames/%s.jpg", loadGameList[choice].c_str()) );
			fileSystem->RemoveFile( va("savegames/%s.txt", loadGameList[choice].c_str()) );
			SetSaveGameGuiVars( );
			guiActive->StateChanged( com_frameTime );
		}
		return true;
	}
	
	if ( !idStr::Icmp( cmd, "updateSaveGameInfo" ) ) {
		int choice = guiActive->State().GetInt( "loadgame_sel_0" );
		if ( choice >= 0 && choice < loadGameList.Num() ) {
			const idMaterial *material;

			idStr saveName, description, screenshot;
			idLexer src(LEXFL_NOERRORS|LEXFL_NOSTRINGCONCAT);
			if ( src.LoadFile( va("savegames/%s.txt", loadGameList[choice].c_str()) ) ) {
				idToken tok;

				src.ReadToken( &tok );
				saveName = tok;

				src.ReadToken( &tok );
				description = tok;

				src.ReadToken( &tok );
				screenshot = tok;

			} else {
				saveName = loadGameList[choice];
				description = loadGameList[choice];
				screenshot = "";
			}
			if ( screenshot.Length() == 0 ) {
				screenshot = va("savegames/%s.tga", loadGameList[choice].c_str());
			}
			material = declManager->FindMaterial( screenshot );
			if ( material ) {
				material->ReloadImages( false );
			}
			guiActive->SetStateString( "loadgame_shot",  screenshot );

			saveName.RemoveColors();
			guiActive->SetStateString( "saveGameName", saveName );
			guiActive->SetStateString( "saveGameDescription", description );

			ID_TIME_T timeStamp;
			fileSystem->ReadFile( va("savegames/%s.save", loadGameList[choice].c_str()), NULL, &timeStamp );
			idStr date = Sys_TimeStampToStr(timeStamp);
			int tab = date.Find( '\t' );
			idStr time = date.Right( date.Length() - tab - 1);
			guiActive->SetStateString( "saveGameDate", date.Left( tab ) );
			guiActive->SetStateString( "saveGameTime", time );
		}
		return true;
	}

	return false;
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

		//if ( !idStr::Icmp( cmd, "restart" ) ) {
		//	if ( !LoadGame( GetAutoSaveName( mapSpawnData.serverInfo.GetString("si_map") ) ) ) {
				// If we can't load the autosave then just restart the map
		//		MoveToNewMap( mapSpawnData.serverInfo.GetString("si_map") );
		//	}
		//	continue;
		//}
		
		if ( !idStr::Icmp( cmd, "restart" ) ) {
		//	if ( !LoadGame( GetAutoSaveName( mapSpawnData.serverInfo.GetString("si_map") ) ) ) {
		//		// If we can't load the autosave then just restart the map
			MoveToNewMap( mapSpawnData.serverInfo.GetString("si_map") );
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
		
		if ( !idStr::Icmp( cmd, "quit" ) ) {
			ExitMenu();
			common->Quit();
			return;
		}

		if ( !idStr::Icmp( cmd, "loadMod" ) ) {
			int choice = guiActive->State().GetInt( "modsList_sel_0" );
			if ( choice >= 0 && choice < modsList.Num() ) {
				cvarSystem->SetCVarString( "fs_mod", modsList[ choice ] );
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "reloadEngine menu\n" );
			}
		}

		if ( !idStr::Icmp( cmd, "UpdateServers" ) ) {
			if ( guiActive->State().GetBool( "lanSet" ) ) {
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "LANScan" );
			}
			continue;
		}

		if ( !idStr::Icmp( cmd, "RefreshServers" ) ) {
			if ( guiActive->State().GetBool( "lanSet" ) ) {
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "LANScan" );
			}
			continue;
		}

		if (!idStr::Icmp( cmd, "LANConnect" )) {
			int sel = guiActive->State().GetInt( "serverList_selid_0" ); 
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "Connect %d\n", sel ) );
			return;
		}

		if ( !idStr::Icmp( cmd, "MAPScan" ) ) {
			const char *gametype = cvarSystem->GetCVarString( "si_gameType" );
			if ( gametype == NULL || *gametype == 0 || idStr::Icmp( gametype, "singleplayer" ) == 0 ) {
				gametype = "Deathmatch";
			}

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
					mapName = common->Translate( mapName );
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
			}
			UpdateMPLevelShot();
			continue;
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
						if ( ShowMessageBox( MSG_OKCANCEL, va( common->Translate( "#str_04315" ), dedicated ? maxclients : Min( 8, maxclients + 1 ) ), common->Translate( "#str_04316" ), true, "OK" )[ 0 ] == '\0' ) {
							continue;
						}
						cvarSystem->SetCVarInteger( "si_maxPlayers", dedicated ? maxclients : Min( 8, maxclients + 1 ) );
					}
				}
			}

			if ( !dedicated && !cvarSystem->GetCVarBool( "net_LANServer" ) && cvarSystem->GetCVarInteger("si_maxPlayers") > 4 ) {
				// "Dedicated server mode is recommended for internet servers with more than 4 players. Continue in listen mode?"
				if ( !ShowMessageBox( MSG_YESNO, common->Translate( "#str_00100625" ), common->Translate ( "#str_00100626" ), true, "yes" )[ 0 ] ) {
					continue;
				}
			}

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
		if ( !idStr::Icmp( cmd, "restart" ) ) {
		//	if ( !LoadGame( GetAutoSaveName( mapSpawnData.serverInfo.GetString("si_map") ) ) ) {
		//		// If we can't load the autosave then just restart the map
			MoveToNewMap( mapSpawnData.serverInfo.GetString("si_map") );
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

		if ( !idStr::Icmp( cmd, "play" ) ) {
			if ( args.Argc() - icmd >= 1 ) {
				idStr snd = args.Argv( icmd++ );
				int channel = 1;
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
				menuSoundWorld->PlayShaderDirectly( snd, 2 );
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
					ShowMessageBox( MSG_OK, common->Translate( "#str_07236" ), common->Translate( "#str_07235" ), true );
#else
					// a message that doesn't mention the windows control panel
					ShowMessageBox( MSG_OK, common->Translate( "#str_07234" ), common->Translate( "#str_07235" ), true );
#endif
				}
			}
			if ( !vcmd.Icmp( "eax" ) ) {
				if ( cvarSystem->GetCVarBool( "s_useEAXReverb" ) ) {
                    int efx = soundSystem->IsEFXAvailable();
                    switch (efx) {
					case 1:
						// when you restart
						ShowMessageBox( MSG_OK, common->Translate( "#str_04137" ), common->Translate( "#str_07231" ), true );
						break;
					case -1:
						cvarSystem->SetCVarBool( "s_useEAXReverb", false );
						// disabled
						ShowMessageBox( MSG_OK, common->Translate( "#str_07233" ), common->Translate( "#str_07231" ), true );
						break;
					case 0:
						cvarSystem->SetCVarBool( "s_useEAXReverb", false );
						// not available
						ShowMessageBox( MSG_OK, common->Translate( "#str_07232" ), common->Translate( "#str_07231" ), true );
						break;
					}
				} else {
					// also turn off OpenAL so we fully go back to legacy mixer
					cvarSystem->SetCVarBool( "s_useOpenAL", false );
					// when you restart
					ShowMessageBox( MSG_OK, common->Translate( "#str_04137" ), common->Translate( "#str_07231" ), true );
				}
			}
			if ( !vcmd.Icmp( "drivar" ) ) {
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "s_restart\n" );				
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
			if ( idStr::Icmp( "cvar_restart", args.Argv( icmd - 1 ) ) == 0 ) {
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "exec default.cfg" );
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "setMachineSpec\n" );

				//Make sure that any r_brightness changes take effect
				float bright = cvarSystem->GetCVarFloat("r_brightness");
				cvarSystem->SetCVarFloat("r_brightness", 0.0f);
				cvarSystem->SetCVarFloat("r_brightness", bright);

				//guiActive->SetStateInt( "com_machineSpec", com_machineSpec.GetInteger() );

				//Restore the language
				cvarSystem->SetCVarString("sys_lang", lang);

			}
			continue;
		}

		if ( !idStr::Icmp ( cmd, "loadBinds" ) ) {
			guiMainMenu->SetKeyBindingNames();
			continue;
		}
		
		if ( !idStr::Icmp( cmd, "systemCvars" ) ) {
			guiActive->HandleNamedEvent( "cvar read render" );
			guiActive->HandleNamedEvent( "cvar read sound" );
			continue;
		}
	}
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
		//TODO on restart screen: HandleRestartMenuCommands( menuCommand ); 
		return;
	} else if ( gui == guiMsg ) {
		HandleMsgCommands( menuCommand );
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
idSessionLocal::ShowMessageBox
=================
*/
const char* idSessionLocal::ShowMessageBox( msgBoxType_t type, const char *message, const char *title, bool wait, const char *fire_yes, const char *fire_no, bool network ) {
	
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
	switch ( type ) {
		case MSG_INFO:
			guiMsg->SetStateString( "mid", "" );
			guiMsg->SetStateString( "visible_mid", "0" );
			guiMsg->SetStateString( "visible_left", "0" );
			guiMsg->SetStateString( "visible_right", "0" );
			break;
		case MSG_OK:
			guiMsg->SetStateString( "mid", common->Translate( "#str_07188" ) );
			guiMsg->SetStateString( "visible_mid", "1" );
			guiMsg->SetStateString( "visible_left", "0" );
			guiMsg->SetStateString( "visible_right", "0" );
			break;
		case MSG_ABORT:
			guiMsg->SetStateString( "mid", common->Translate( "#str_04340" ) );
			guiMsg->SetStateString( "visible_mid", "1" );
			guiMsg->SetStateString( "visible_left", "0" );
			guiMsg->SetStateString( "visible_right", "0" );
			break;
		case MSG_OKCANCEL:
			guiMsg->SetStateString( "left", common->Translate( "#str_07188" ) );
			guiMsg->SetStateString( "right", common->Translate( "#str_04340" ) );
			guiMsg->SetStateString( "visible_mid", "0" );
			guiMsg->SetStateString( "visible_left", "1" );
			guiMsg->SetStateString( "visible_right", "1" );
			break;
		case MSG_YESNO:
			guiMsg->SetStateString( "left", common->Translate( "#str_04341" ) );
			guiMsg->SetStateString( "right", common->Translate( "#str_04342" ) );
			guiMsg->SetStateString( "visible_mid", "0" );
			guiMsg->SetStateString( "visible_left", "1" );
			guiMsg->SetStateString( "visible_right", "1" );
			break;
		case MSG_PROMPT:
			guiMsg->SetStateString( "left", common->Translate( "#str_07188" ) );
			guiMsg->SetStateString( "right", common->Translate( "#str_04340" ) );
			guiMsg->SetStateString( "visible_mid", "0" );
			guiMsg->SetStateString( "visible_left", "1" );
			guiMsg->SetStateString( "visible_right", "1" );
			guiMsg->SetStateString( "visible_entry", "1" );			
			guiMsg->HandleNamedEvent( "Prompt" );
			break;
		case MSG_WAIT:
			break;
		default:
			common->Printf( "idSessionLocal::MessageBox: unknown msg box type\n" );
	}
	msgFireBack[ 0 ] = fire_yes ? fire_yes : "";
	msgFireBack[ 1 ] = fire_no ? fire_no : "";
	guiMsgRestore = guiActive;
	// 4725: Hide the cursor behind the prompt (Obsttorte)
	if (guiMsgRestore)
		guiMsgRestore->SetCursor(325,290);

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
		}
		else {
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
