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
#include "Common.h"
#include "renderer/tr_local.h"
#include "renderer/backend/FrameBuffer.h"
#include "game/gamesys/SysCvar.h"
#include "game/Missions/MissionManager.h"

idCVar	idSessionLocal::com_showAngles( "com_showAngles", "0", CVAR_SYSTEM | CVAR_BOOL, "" );
idCVar	idSessionLocal::com_minTics( "com_minTics", "0", CVAR_SYSTEM, "" );
idCVar	idSessionLocal::com_showTics( "com_showTics", "0", CVAR_SYSTEM | CVAR_BOOL, "" );
idCVar	idSessionLocal::com_fixedTic("com_fixedTic", "0", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_INTEGER,
	" 0 -- game tics have fixed duration of 16 ms (stable physics but 60 FPS limit)\n"
	" 1 -- game tics can have shorter duration (removes 60 FPS limit)",
0, 1);
idCVar	idSessionLocal::com_maxTicTimestep("com_maxTicTimestep", "17", CVAR_SYSTEM | CVAR_INTEGER,
	"Timestep of a game tic must not exceed this number of milliseconds. "
	"If frame takes more time, then its duration is split into several game tics.\n"
	"Note: takes effect only when FPS is uncapped.",
1, 1000);
idCVar	idSessionLocal::com_maxTicsPerFrame("com_maxTicsPerFrame", "10", CVAR_SYSTEM | CVAR_INTEGER,
	"Never do more than this number of game tics per one frame. "
	"When frames take too much time, allow game time to run slower than astronomical time.",
1, 1000);
idCVar	idSessionLocal::com_useMinorTics("com_useMinorTics", "1", CVAR_SYSTEM | CVAR_BOOL,
	"If several game tics are modelled in one frame, all tics except the first one are declared \"minor\". "
	"Minor tics can enable various optimizations, f.i. alive AIs don't think in minor tics.",
1, 1000);
idCVar	idSessionLocal::com_maxFPS( "com_maxFPS", "300", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_INTEGER, "define the maximum FPS cap", 2, 1000 );
idCVar	idSessionLocal::com_showDemo("com_showDemo", "0", CVAR_SYSTEM | CVAR_BOOL, "");
idCVar	idSessionLocal::com_skipGameDraw( "com_skipGameDraw", "0", CVAR_SYSTEM | CVAR_BOOL, "" );
idCVar	idSessionLocal::com_aviDemoSamples( "com_aviDemoSamples", "16", CVAR_SYSTEM, "" );
idCVar	idSessionLocal::com_aviDemoWidth( "com_aviDemoWidth", "256", CVAR_SYSTEM, "" );
idCVar	idSessionLocal::com_aviDemoHeight( "com_aviDemoHeight", "256", CVAR_SYSTEM, "" );
idCVar	idSessionLocal::com_aviDemoTics( "com_aviDemoTics", "2", CVAR_SYSTEM | CVAR_INTEGER, "", 1, 60 );
idCVar	idSessionLocal::com_wipeSeconds( "com_wipeSeconds", "0.1", CVAR_SYSTEM, "" );
idCVar	idSessionLocal::com_guid( "com_guid", "", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_ROM, "" );

//Obsttorte
idCVar	idSessionLocal::saveGameName( "saveGameName", "", CVAR_GAME | CVAR_ROM, "");

// SteveL #4161: Support > 1 quicksave
idCVar	idSessionLocal::com_numQuickSaves( "com_numQuickSaves", "2", CVAR_GAME | CVAR_NOCHEAT | CVAR_INTEGER | CVAR_ARCHIVE, 
	"How many quicksaves to retain. Reducing the number won't delete any that you already have.", 1.0f, 100000.0f );

// stgatilov: allow choosing format for savegame previews
idCVar	com_savegame_preview_format( "com_savegame_preview_format", "jpg", CVAR_GAME | CVAR_ARCHIVE, "Image format used to store previews for game saves: tga/jpg." );

idSessionLocal		sessLocal;
idSession			*session = &sessLocal;

bool no_smp = false;

// these must be kept up to date with window Levelshot in guis/mainmenu.gui
const int PREVIEW_X = 211;
const int PREVIEW_Y = 31;
const int PREVIEW_WIDTH = 398;
const int PREVIEW_HEIGHT = 298;

// grayman #3763 - loading bar progress at key points

const float LOAD_KEY_START_PROGRESS = 0.02f;
const float LOAD_KEY_COLLISION_START_PROGRESS = 0.03f;
const float LOAD_KEY_COLLISION_DONE_PROGRESS = 0.04f;
const float LOAD_KEY_SPAWN_ENTITIES_START_PROGRESS = 0.05f;
const float LOAD_KEY_ROUTING_START_PROGRESS = 0.36f;
const float LOAD_KEY_ROUTING_DONE_PROGRESS = 0.43f;
const float LOAD_KEY_IMAGES_START_PROGRESS = 0.45f;
const float LOAD_KEY_DONE_PROGRESS = 1.00f;

void RandomizeStack( void ) {
	// attempt to force uninitialized stack memory bugs
	int		bytes = 4000000;
	byte	*buf = (byte *)_alloca( bytes );

	int	fill = rand()&255;
	for ( int i = 0 ; i < bytes ; i++ ) {
		buf[i] = fill;
	}
}

/*
=================
Session_RescanSI_f
=================
*/
void Session_RescanSI_f( const idCmdArgs &args ) {
	sessLocal.mapSpawnData.serverInfo = *cvarSystem->MoveCVarsToDict( CVAR_SERVERINFO );
}

/*
==================
Session_Map_f

Restart the server on a different map
==================
*/
static void Session_Map_f( const idCmdArgs &args ) {
	idStr		map, string;
	findFile_t	ff;
	idCmdArgs	rl_args;

	map = args.Argv(1);
	if ( !map.Length() ) {
		return;
	}
	map.StripFileExtension();

	// make sure the level exists before trying to change, so that
	// a typo at the server console won't end the game
	// handle addon packs through reloadEngine
	sprintf( string, "maps/%s.map", map.c_str() );
	ff = fileSystem->FindFile( string, true );
	switch ( ff ) {
	case FIND_NO:
		common->Printf( "Can't find map %s\n", string.c_str() );
		return;
	case FIND_ADDON:
		common->Printf( "map %s is in an addon pak - reloading\n", string.c_str() );
		rl_args.AppendArg( "map" );
		rl_args.AppendArg( map );
		cmdSystem->SetupReloadEngine( rl_args );
		return;
	default:
		break;
	}

	cvarSystem->SetCVarBool( "developer", false );
	sessLocal.StartNewGame( map, true );
}

/*
==================
Session_DevMap_f

Restart the server on a different map in developer mode
==================
*/
static void Session_DevMap_f( const idCmdArgs &args ) {
	idStr map, string;
	findFile_t	ff;
	idCmdArgs	rl_args;	

	map = args.Argv(1);
	if ( !map.Length() ) {
		return;
	}
	map.StripFileExtension();

	// make sure the level exists before trying to change, so that
	// a typo at the server console won't end the game
	// handle addon packs through reloadEngine
	sprintf( string, "maps/%s.map", map.c_str() );
	ff = fileSystem->FindFile( string, true );
	switch ( ff ) {
	case FIND_NO:
		common->Printf( "Can't find map %s\n", string.c_str() );
		return;
	case FIND_ADDON:
		common->Printf( "map %s is in an addon pak - reloading\n", string.c_str() );
		rl_args.AppendArg( "devmap" );
		rl_args.AppendArg( map );
		cmdSystem->SetupReloadEngine( rl_args );
		return;
	default:
		break;
	}

	cvarSystem->SetCVarBool( "developer", true );
	sessLocal.StartNewGame( map, true );
}

/*
==================
Session_TestMap_f
==================
*/
static void Session_TestMap_f( const idCmdArgs &args ) {
	idStr map, string;

	map = args.Argv(1);
	if ( !map.Length() ) {
		return;
	}
	map.StripFileExtension();

	cmdSystem->BufferCommandText( CMD_EXEC_NOW, "disconnect" );

	sprintf( string, "dmap maps/%s.map", map.c_str() );
	cmdSystem->BufferCommandText( CMD_EXEC_NOW, string );

	sprintf( string, "map %s", map.c_str() );
	cmdSystem->BufferCommandText( CMD_EXEC_NOW, string );
}

/*
==================
Session_TestDevmap_f
==================
*/
static void Session_TestDevmap_f( const idCmdArgs &args ) {
	idStr map, string;

	map = args.Argv(1);
	if ( !map.Length() ) {
		return;
	}
	map.StripFileExtension();

	cmdSystem->BufferCommandText( CMD_EXEC_NOW, "disconnect" );

	sprintf( string, "dmap maps/%s.map", map.c_str() );
	cmdSystem->BufferCommandText( CMD_EXEC_NOW, string );

	sprintf( string, "devmap %s", map.c_str() );
	cmdSystem->BufferCommandText( CMD_EXEC_NOW, string );
}

/*
==================
Sess_WritePrecache_f
==================
*/
static void Sess_WritePrecache_f( const idCmdArgs &args ) {
	if ( args.Argc() != 2 ) {
		common->Printf( "USAGE: writePrecache <execFile>\n" );
		return;
	}
	idStr	str = args.Argv(1);
	str.DefaultFileExtension( ".cfg" );
	idFile *f = fileSystem->OpenFileWrite( str );
	declManager->WritePrecacheCommands( f );
	renderModelManager->WritePrecacheCommands( f );
	uiManager->WritePrecacheCommands( f );

	fileSystem->CloseFile( f );
}

/*
===============================================================================

SESSION LOCAL
  
===============================================================================
*/

/*
===============
idSessionLocal::Clear
===============
*/
void idSessionLocal::Clear() {
	
	insideUpdateScreen = false;
	insideExecuteMapChange = false;

	savegameVersion = 0;

	currentMapName.Clear();
	aviDemoShortName.Clear();
	msgFireBack[ 0 ].Clear();
	msgFireBack[ 1 ].Clear();

	timeHitch = 0;

	rw = NULL;
	sw = NULL;
	menuSoundWorld = NULL;
	readDemo = NULL;
	writeDemo = NULL;
	renderdemoVersion = 0;
	cmdDemoFile = NULL;

	syncNextGameFrame = false;
	mapSpawned = false;
	mainMenuStartState = MMSS_MAINMENU;
	guiActive = NULL;
	aviCaptureMode = false;
	timeDemo = TD_NO;
	waitingOnBind = false;
	lastPacifierTime = 0;
	
	msgRunning = false;
	guiMsgRestore = NULL;
	msgIgnoreButtons = false;
	no_smp = false;

	//bytesNeededForMapLoad = 0; // grayman #3763 - no longer used

#if ID_CONSOLE_LOCK
	emptyDrawCount = 0;
#endif
	ClearWipe();

	loadGameList.Clear();
	modsList.Clear();

	authMsg.Clear();
}

/*
===============
idSessionLocal::idSessionLocal
===============
*/
idSessionLocal::idSessionLocal() {
	guiInGame = guiMainMenu = guiLoading = guiActive = guiTest = guiMsg = guiMsgRestore = NULL;	

	menuSoundWorld = NULL;
	
	Clear();
}

/*
===============
idSessionLocal::~idSessionLocal
===============
*/
idSessionLocal::~idSessionLocal() {
}

/*
===============
idSessionLocal::Stop

called on errors and game exits
===============
*/
void idSessionLocal::Stop() {
	ClearWipe();

	// clear mapSpawned and demo playing flags
	UnloadMap();
	// stgatilov: reset to default playerstart when "Quit Mission" is used (but not on restart)
	gameLocal.m_StartPosition = "";

	if ( sw ) {
		sw->StopAllSounds();
	}

	insideUpdateScreen = false;
	insideExecuteMapChange = false;

	// drop all guis
	SetGUI( NULL, NULL );
}

void idSessionLocal::TerminateFrontendThread() {
#if 0
	if (frontendThread.joinable()) {
		{  // lock scope
			std::lock_guard<std::mutex> lock( signalMutex );
			shutdownFrontend = true;
			signalFrontendThread.notify_one();
		}
		frontendThread.join();
	}
#else
	{
		std::lock_guard<std::mutex> lock( signalMutex );
		shutdownFrontend = true;
		signalFrontendThread.notify_one();
	}
	Sys_DestroyThread( frontendThread );
#endif
}

/*
===============
idSessionLocal::Shutdown
===============
*/
void idSessionLocal::Shutdown() {
	int i;

	TerminateFrontendThread();

	if (aviCaptureMode) {
		EndAVICapture();
	}

    if (timeDemo == TD_YES) {
        // else the game freezes when showing the timedemo results
        timeDemo = TD_YES_THEN_QUIT;
    }

	Stop();

	if ( rw ) {
		delete rw;
		rw = NULL;
	}

	if ( sw ) {
		delete sw;
		sw = NULL;
	}

	if ( menuSoundWorld ) {
		delete menuSoundWorld;
		menuSoundWorld = NULL;
	}
		
	mapSpawnData.serverInfo.ClearFree();
	mapSpawnData.syncedCVars.ClearFree();
	for ( i = 0; i < MAX_ASYNC_CLIENTS; i++ ) {
		mapSpawnData.userInfo[i].ClearFree();
		mapSpawnData.persistentPlayerInfo[i].ClearFree();
	}

	if ( guiMainMenu_MapList != NULL ) {
		guiMainMenu_MapList->Shutdown();
		uiManager->FreeListGUI( guiMainMenu_MapList );
		guiMainMenu_MapList = NULL;
	}

	//stgatilov: shut down main menu
	ResetMainMenu();

	Clear();
}

/*
================
idSessionLocal::StartWipe

Draws and captures the current state, then starts a wipe with that image
================
*/
void idSessionLocal::StartWipe( const char *_wipeMaterial, bool hold ) {
	console->Close();

	// render the current screen into a texture for the wipe model
	renderSystem->CropRenderSize( 640, 480, true );

	Draw();
	vertexCache.EndFrame();

	renderSystem->CaptureRenderToImage( *globalImages->scratchImage );
	renderSystem->UnCrop();

	// stgatilov #6149: execute these commands now, including finishing SwapBuffers
	// otherwise they get concatenated with CompleteWipe into a single backend commands sequence
	// which causes broken rendering due to some kind of FBO state leak
	extern void R_IssueRenderCommands( frameData_t *frameData );
	R_IssueRenderCommands( backendFrameData );
	R_ToggleSmpFrame();

	wipeMaterial = declManager->FindMaterial( _wipeMaterial, false );

	wipeStartTic = com_ticNumber;
	wipeStopTic = wipeStartTic + 1000.0f / USERCMD_MSEC * com_wipeSeconds.GetFloat();
	wipeHold = hold;
}

/*
================
idSessionLocal::CompleteWipe
================
*/
void idSessionLocal::CompleteWipe() {
	if ( com_ticNumber == 0 ) {
		// if the async thread hasn't started, we would hang here
		wipeStopTic = 0;
		UpdateScreen( true );
		return;
	}
	do {
#if ID_CONSOLE_LOCK
		emptyDrawCount = 0;
#endif
		UpdateScreen( true );
	} while ( com_ticNumber < wipeStopTic );
}

/*
================
idSessionLocal::ShowLoadingGui
================
*/
void idSessionLocal::ShowLoadingGui() {
	if ( com_ticNumber == 0 ) {
		return;
	}
	console->Close();

	// grayman #4399 - duzenko's change to improve load time by 1s. This code runs before
	// the loading bar begins to climb from 0->100%.

/*
	// introduced in D3XP code. don't think it actually fixes anything, but doesn't hurt either
	// Try and prevent the while loop from being skipped over (long hitch on the main thread?)
	int stop = Sys_Milliseconds() + 1000;
	int force = 10;
	while ( Sys_Milliseconds() < stop || force-- > 0 )
	{*/
		com_frameTime = com_ticNumber * USERCMD_MSEC;
		session->Frame();
		session->UpdateScreen( false );
	//}
}



/*
================
idSessionLocal::ClearWipe
================
*/
void idSessionLocal::ClearWipe( void ) {
	wipeHold = false;
	wipeStopTic = 0;
	wipeStartTic = wipeStopTic + 1;
}

/*
================
Session_TestGUI_f
================
*/
static void Session_TestGUI_f( const idCmdArgs &args ) {
	sessLocal.TestGUI( args.Argv(1) );
}

/*
================
idSessionLocal::TestGUI
================
*/
void idSessionLocal::TestGUI( const char *guiName ) {
	if ( guiName && *guiName ) {
		guiTest = uiManager->FindGui( guiName, true, false, true );
	} else {
		guiTest = NULL;
	}
}

/*
================
FindUnusedFileName
================
*/
static idStr FindUnusedFileName( const char *format ) {
	int i;
	char	filename[1024];

	for ( i = 0 ; i < 999 ; i++ ) {
		sprintf( filename, format, i );
		int len = fileSystem->ReadFile( filename, NULL, NULL );
		if ( len <= 0 ) {
			return filename;	// file doesn't exist
		}
	}

	return filename;
}

/*
================
Session_DemoShot_f
================
*/
static void Session_DemoShot_f( const idCmdArgs &args ) {
	if ( args.Argc() != 2 ) {
		idStr filename = FindUnusedFileName( "demos/shot%03i.demo" );
		sessLocal.DemoShot( filename );
	} else {
		sessLocal.DemoShot( va( "demos/shot_%s.demo", args.Argv(1) ) );
	}
}

/*
================
Session_RecordDemo_f
================
*/
static void Session_RecordDemo_f( const idCmdArgs &args ) {
	if ( args.Argc() != 2 ) {
		idStr filename = FindUnusedFileName( "demos/demo%03i.demo" );
		sessLocal.StartRecordingRenderDemo( filename );
	} else {
		sessLocal.StartRecordingRenderDemo( va( "demos/%s.demo", args.Argv(1) ) );
	}
}

/*
================
Session_CompressDemo_f
================
*/
static void Session_CompressDemo_f( const idCmdArgs &args ) {
	if ( args.Argc() == 2 ) {
		sessLocal.CompressDemoFile( "2", args.Argv(1) );
	} else if ( args.Argc() == 3 ) {
		sessLocal.CompressDemoFile( args.Argv(2), args.Argv(1) );
	} else {
		common->Printf("use: CompressDemo <file> [scheme]\nscheme is the same as com_compressDemo, defaults to 2" );
	}
}

/*
================
Session_StopRecordingDemo_f
================
*/
static void Session_StopRecordingDemo_f( const idCmdArgs &args ) {
	sessLocal.StopRecordingRenderDemo();
}

/*
================
Session_PlayDemo_f
================
*/
static void Session_PlayDemo_f( const idCmdArgs &args ) {
	if ( args.Argc() >= 2 ) {
		sessLocal.StartPlayingRenderDemo( va( "demos/%s", args.Argv(1) ) );
	}
}

/*
================
Session_TimeDemo_f
================
*/
static void Session_TimeDemo_f( const idCmdArgs &args ) {
	if ( args.Argc() >= 2 ) {
		sessLocal.TimeRenderDemo( va( "demos/%s", args.Argv(1) ), ( args.Argc() > 2 ) );
	}
}

/*
================
Session_TimeDemoQuit_f
================
*/
static void Session_TimeDemoQuit_f( const idCmdArgs &args ) {
	sessLocal.TimeRenderDemo( va( "demos/%s", args.Argv(1) ) );
	if ( sessLocal.timeDemo == TD_YES ) {
		// this allows hardware vendors to automate some testing
		sessLocal.timeDemo = TD_YES_THEN_QUIT;
	}
}

/*
================
Session_AVIDemo_f
================
*/
static void Session_AVIDemo_f( const idCmdArgs &args ) {
	sessLocal.AVIRenderDemo( va( "demos/%s", args.Argv(1) ) );
}

/*
================
Session_AVIGame_f
================
*/
static void Session_AVIGame_f( const idCmdArgs &args ) {
	sessLocal.AVIGame( args.Argv(1) );
}

/*
================
Session_AVICmdDemo_f
================
*/
static void Session_AVICmdDemo_f( const idCmdArgs &args ) {
	sessLocal.AVICmdDemo( args.Argv(1) );
}

/*
================
Session_WriteCmdDemo_f
================
*/
static void Session_WriteCmdDemo_f( const idCmdArgs &args ) {
	if ( args.Argc() == 1 ) {
		idStr	filename = FindUnusedFileName( "demos/cmdDemo%03i.cdemo" );
		sessLocal.WriteCmdDemo( filename );
	} else if ( args.Argc() == 2 ) {
		sessLocal.WriteCmdDemo( va( "demos/%s.cdemo", args.Argv( 1 ) ) );
	} else {
		common->Printf( "usage: writeCmdDemo [demoName]\n" );
	}
}

/*
================
Session_PlayCmdDemo_f
================
*/
static void Session_PlayCmdDemo_f( const idCmdArgs &args ) {
	sessLocal.StartPlayingCmdDemo( args.Argv(1) );
}

/*
================
Session_TimeCmdDemo_f
================
*/
static void Session_TimeCmdDemo_f( const idCmdArgs &args ) {
	sessLocal.TimeCmdDemo( args.Argv(1) );
}

/*
================
Session_Disconnect_f
================
*/
static void Session_Disconnect_f( const idCmdArgs &args ) {
	sessLocal.Stop();
	sessLocal.StartMenu();
	if ( soundSystem ) {
		soundSystem->SetMute( false );
	}
}

/*
================
Session_ExitCmdDemo_f
================
*/
static void Session_ExitCmdDemo_f( const idCmdArgs &args ) {
	if ( !sessLocal.cmdDemoFile ) {
		common->Printf( "not reading from a cmdDemo\n" );
		return;
	}
	fileSystem->CloseFile( sessLocal.cmdDemoFile );
	common->Printf( "Command demo exited at logIndex %i\n", sessLocal.logIndex );
	sessLocal.cmdDemoFile = NULL;
}

/*
================
idSessionLocal::StartRecordingRenderDemo
================
*/
void idSessionLocal::StartRecordingRenderDemo( const char *demoName ) {
	if ( writeDemo ) {
		// allow it to act like a toggle
		StopRecordingRenderDemo();
		return;
	}

	if ( !demoName[0] ) {
		common->Printf( "idSessionLocal::StartRecordingRenderDemo: no name specified\n" );
		return;
	}

	console->Close();

	writeDemo = new idDemoFile;
	if ( !writeDemo->OpenForWriting( demoName ) ) {
		common->Printf( "error opening %s\n", demoName );
		delete writeDemo;
		writeDemo = NULL;
		return;
	}

	common->Printf( "recording to %s\n", writeDemo->GetName() );

	writeDemo->WriteInt( DS_VERSION );
	writeDemo->WriteInt( RENDERDEMO_VERSION );

	// if we are in a map already, dump the current state
	sw->StartWritingDemo( writeDemo );
	rw->StartWritingDemo( writeDemo );
}

/*
================
idSessionLocal::StopRecordingRenderDemo
================
*/
void idSessionLocal::StopRecordingRenderDemo() {
	if ( !writeDemo ) {
		common->Printf( "idSessionLocal::StopRecordingRenderDemo: not recording\n" );
		return;
	}
	sw->StopWritingDemo();
	rw->StopWritingDemo();

	writeDemo->Close();
	common->Printf( "stopped recording %s.\n", writeDemo->GetName() );
	delete writeDemo;
	writeDemo = NULL;
}

/*
================
idSessionLocal::StopPlayingRenderDemo

Reports timeDemo numbers and finishes any avi recording
================
*/
void idSessionLocal::StopPlayingRenderDemo() {
	if ( !readDemo ) {
		timeDemo = TD_NO;
		return;
	}

	// Record the stop time before doing anything that could be time consuming 
	int timeDemoStopTime = Sys_Milliseconds();

	EndAVICapture();

	readDemo->Close();

	sw->StopAllSounds();
	soundSystem->SetPlayingSoundWorld( menuSoundWorld );

	common->Printf( "stopped playing %s.\n", readDemo->GetName() );
	delete readDemo;
	readDemo = NULL;

	if ( timeDemo ) {
		// report the stats
		float	demoSeconds = ( timeDemoStopTime - timeDemoStartTime ) * 0.001f;
		float	demoFPS = numDemoFrames / demoSeconds;
		idStr	message = va( "%i frames rendered in %3.1f seconds = %3.1f fps\n", numDemoFrames, demoSeconds, demoFPS );

		common->Printf( "%s", message.c_str() );
		if ( timeDemo == TD_YES_THEN_QUIT ) {
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
		} else {
			soundSystem->SetMute( true );
			ShowMessageBox( MSG_OK, message, "Time Demo Results", true );
			soundSystem->SetMute( false );
		}
		timeDemo = TD_NO;
	}
}

/*
================
idSessionLocal::DemoShot

A demoShot is a single frame demo
================
*/
void idSessionLocal::DemoShot( const char *demoName ) {
	StartRecordingRenderDemo( demoName );

	// force draw one frame
	UpdateScreen();

	StopRecordingRenderDemo();
}

/*
================
idSessionLocal::StartPlayingRenderDemo
================
*/
void idSessionLocal::StartPlayingRenderDemo( idStr demoName ) {
	if ( !demoName[0] ) {
		common->Printf( "idSessionLocal::StartPlayingRenderDemo: no name specified\n" );
		return;
	}

	// make sure localSound / GUI intro music shuts up
	sw->StopAllSounds();
	sw->PlayShaderDirectly( "", 0 );	
	menuSoundWorld->StopAllSounds();
	menuSoundWorld->PlayShaderDirectly( "", 0 );

	// exit any current game
	Stop();

	// automatically put the console away
	console->Close();

	// bring up the loading screen manually, since demos won't
	// call ExecuteMapChange()
	guiLoading = uiManager->FindGui( "guis/map/loading.gui", true, false, true );
	guiLoading->SetStateString( "demo", common->Translate( "#str_02087" ) );
	readDemo = new idDemoFile;
	demoName.DefaultFileExtension( ".demo" );
	if ( !readDemo->OpenForReading( demoName ) ) {
		common->Printf( "Couldn't open %s\n", demoName.c_str() );
		delete readDemo;
		readDemo = NULL;
		Stop();
		StartMenu();
		soundSystem->SetMute( false );
		return;
	}

	insideExecuteMapChange = true;
	UpdateScreen();
	insideExecuteMapChange = false;
	guiLoading->SetStateString( "demo", "" );

	// setup default render demo settings
	// that's default for <= Doom3 v1.1
	renderdemoVersion = 1;
	savegameVersion = 16;

	AdvanceRenderDemo( true );

	numDemoFrames = 1;

	lastDemoTic = -1;
	timeDemoStartTime = Sys_Milliseconds();
}

/*
================
idSessionLocal::TimeRenderDemo
================
*/
void idSessionLocal::TimeRenderDemo( const char *demoName, bool twice ) {
	idStr demo = demoName;
	
	// no sound in time demos
	soundSystem->SetMute( true );

	StartPlayingRenderDemo( demo );
	
	if ( twice && readDemo ) {
		// cycle through once to precache everything
		guiLoading->SetStateString( "demo", common->Translate( "#str_04852" ) );
		guiLoading->StateChanged( com_frameTime );
		while ( readDemo ) {
			insideExecuteMapChange = true;
			UpdateScreen();
			insideExecuteMapChange = false;
			AdvanceRenderDemo( true );
		}
		guiLoading->SetStateString( "demo", "" );
		StartPlayingRenderDemo( demo );
	}
	

	if ( !readDemo ) {
		return;
	}

	timeDemo = TD_YES;
}


/*
================
idSessionLocal::BeginAVICapture
================
*/
void idSessionLocal::BeginAVICapture( const char *demoName ) {
	idStr name = demoName;
	name.ExtractFileBase( aviDemoShortName );
	aviCaptureMode = true;
	aviDemoFrameCount = 0;
	aviTicStart = 0;
	sw->AVIOpen( va( "demos/%s/", aviDemoShortName.c_str() ), aviDemoShortName.c_str() );
}

/*
================
idSessionLocal::EndAVICapture
================
*/
void idSessionLocal::EndAVICapture() {
	if ( !aviCaptureMode ) {
		return;
	}

	sw->AVIClose();

	// write a .roqParam file so the demo can be converted to a roq file
	idFile *f = fileSystem->OpenFileWrite( va( "demos/%s/%s.roqParam", 
		aviDemoShortName.c_str(), aviDemoShortName.c_str() ) );
	f->Printf( "INPUT_DIR demos/%s\n", aviDemoShortName.c_str() );
	f->Printf( "FILENAME demos/%s/%s.RoQ\n", aviDemoShortName.c_str(), aviDemoShortName.c_str() );
	f->Printf( "\nINPUT\n" );
	f->Printf( "%s_*.tga [00000-%05i]\n", aviDemoShortName.c_str(), (int)( aviDemoFrameCount-1 ) );
	f->Printf( "END_INPUT\n" );
	delete f;

	common->Printf( "captured %i frames for %s.\n", ( int )aviDemoFrameCount, aviDemoShortName.c_str() );

	aviCaptureMode = false;
}


/*
================
idSessionLocal::AVIRenderDemo
================
*/
void idSessionLocal::AVIRenderDemo( const char *_demoName ) {
	idStr	demoName = _demoName;	// copy off from va() buffer

	StartPlayingRenderDemo( demoName );
	if ( !readDemo ) {
		return;
	}

	BeginAVICapture( demoName.c_str() ) ;

	// I don't understand why I need to do this twice, something
	// strange with the nvidia swapbuffers?
	UpdateScreen();
}

/*
================
idSessionLocal::AVICmdDemo
================
*/
void idSessionLocal::AVICmdDemo( const char *demoName ) {
	StartPlayingCmdDemo( demoName );

	BeginAVICapture( demoName ) ;
}

/*
================
idSessionLocal::AVIGame

Start AVI recording the current game session
================
*/
void idSessionLocal::AVIGame( const char *demoName ) {
	if ( aviCaptureMode ) {
		EndAVICapture();
		return;
	}

	if ( !mapSpawned ) {
		common->Printf( "No map spawned.\n" );
	}

	if ( !demoName || !demoName[0] ) {
		idStr filename = FindUnusedFileName( "demos/game%03i.game" );
		demoName = filename.c_str();

		// write a one byte stub .game file just so the FindUnusedFileName works,
		fileSystem->WriteFile( demoName, demoName, 1 );
	}

	BeginAVICapture( demoName ) ;
}

/*
================
idSessionLocal::CompressDemoFile
================
*/
void idSessionLocal::CompressDemoFile( const char *scheme, const char *demoName ) {
	idStr	fullDemoName = "demos/";
	fullDemoName += demoName;
	fullDemoName.DefaultFileExtension( ".demo" );
	idStr compressedName = fullDemoName;
	compressedName.StripFileExtension();
	compressedName.Append( "_compressed.demo" );

	int savedCompression = cvarSystem->GetCVarInteger("com_compressDemos");
	bool savedPreload = cvarSystem->GetCVarBool("com_preloadDemos");
	cvarSystem->SetCVarBool( "com_preloadDemos", false );
	cvarSystem->SetCVarInteger("com_compressDemos", atoi(scheme) );

	idDemoFile demoread, demowrite;
	if ( !demoread.OpenForReading( fullDemoName ) ) {
		common->Printf( "Could not open %s for reading\n", fullDemoName.c_str() );
		return;
	}
	if ( !demowrite.OpenForWriting( compressedName ) ) {
		common->Printf( "Could not open %s for writing\n", compressedName.c_str() );
		demoread.Close();
		cvarSystem->SetCVarBool( "com_preloadDemos", savedPreload );
		cvarSystem->SetCVarInteger("com_compressDemos", savedCompression);
		return;
	}
	common->SetRefreshOnPrint( true );
	common->Printf( "Compressing %s to %s...\n", fullDemoName.c_str(), compressedName.c_str() );

	static const int bufferSize = 65535;
	char buffer[bufferSize];
	int bytesRead;
	while ( 0 != (bytesRead = demoread.Read( buffer, bufferSize ) ) ) {
		demowrite.Write( buffer, bytesRead );
		common->Printf( "." );
	}

	demoread.Close();
	demowrite.Close();

	cvarSystem->SetCVarBool( "com_preloadDemos", savedPreload );
	cvarSystem->SetCVarInteger("com_compressDemos", savedCompression);

	common->Printf( "Done\n" );
	common->SetRefreshOnPrint( false );

}


/*
===============
idSessionLocal::StartNewGame
===============
*/
void idSessionLocal::StartNewGame( const char *mapName, bool devmap ) {
	// clear the userInfo so the player starts out with the defaults
	mapSpawnData.userInfo[0].Clear();
	mapSpawnData.persistentPlayerInfo[0].Clear();
	mapSpawnData.userInfo[0] = *cvarSystem->MoveCVarsToDict( CVAR_USERINFO );

	mapSpawnData.serverInfo.Clear();
	mapSpawnData.serverInfo = *cvarSystem->MoveCVarsToDict( CVAR_SERVERINFO );
	mapSpawnData.serverInfo.Set( "si_gameType", "singleplayer" );

	// set the devmap key so any play testing items will be given at
	// spawn time to set approximately the right weapons and ammo
	if(devmap) {
		mapSpawnData.serverInfo.Set( "devmap", "1" );
	}

	mapSpawnData.syncedCVars.Clear();
	mapSpawnData.syncedCVars = *cvarSystem->MoveCVarsToDict( CVAR_NETWORKSYNC );

	MoveToNewMap( mapName );
}

/*
===============
idSessionLocal::GetAutoSaveName
===============
*/
idStr idSessionLocal::GetAutoSaveName( const char *mapName ) const {
	const idDecl *mapDecl = declManager->FindType( DECL_MAPDEF, mapName, false );
	const idDeclEntityDef *mapDef = static_cast<const idDeclEntityDef *>( mapDecl );
	if ( mapDef ) {
		mapName = common->Translate( mapDef->dict.GetString( "name", mapName ) );
	}
	// Fixme: Localization
	return va( "^3AutoSave:^0 %s", mapName );
}

/*
===============
idSessionLocal::MoveToNewMap

Leaves the existing userinfo and serverinfo
===============
*/
void idSessionLocal::MoveToNewMap( const char *mapName ) {
	mapSpawnData.serverInfo.Set( "si_map", mapName );

	ExecuteMapChange();

	SetGUI( NULL, NULL );
}

/*
==============
SaveCmdDemoFromFile
==============
*/
void idSessionLocal::SaveCmdDemoToFile( idFile *file ) {

	mapSpawnData.serverInfo.WriteToFileHandle( file );

	for ( int i = 0 ; i < MAX_ASYNC_CLIENTS ; i++ ) {
		mapSpawnData.userInfo[i].WriteToFileHandle( file );
		mapSpawnData.persistentPlayerInfo[i].WriteToFileHandle( file );
	}

	file->Write( &mapSpawnData.mapSpawnUsercmd, sizeof( mapSpawnData.mapSpawnUsercmd ) );

	if ( numClients < 1 ) {
		numClients = 1;
	}
	file->Write( loggedUsercmds, numClients * logIndex * sizeof( loggedUsercmds[0] ) );
}

/*
==============
idSessionLocal::LoadCmdDemoFromFile
==============
*/
void idSessionLocal::LoadCmdDemoFromFile( idFile *file ) {

	mapSpawnData.serverInfo.ReadFromFileHandle( file );

	for ( int i = 0 ; i < MAX_ASYNC_CLIENTS ; i++ ) {
		mapSpawnData.userInfo[i].ReadFromFileHandle( file );
		mapSpawnData.persistentPlayerInfo[i].ReadFromFileHandle( file );
	}
	file->Read( &mapSpawnData.mapSpawnUsercmd, sizeof( mapSpawnData.mapSpawnUsercmd ) );
}

/*
==============
idSessionLocal::WriteCmdDemo

Dumps the accumulated commands for the current level.
This should still work after disconnecting from a level
==============
*/
void idSessionLocal::WriteCmdDemo( const char *demoName, bool save ) {
	
	if ( !demoName[0] ) {
		common->Printf( "idSessionLocal::WriteCmdDemo: no name specified\n" );
		return;
	}
	if (com_fixedTic.GetInteger()) {
		common->Error( "Cmd demo is not compatible with uncapped FPS" );
	}

	idStr statsName;
	if (save) {
		statsName = demoName;
		statsName.StripFileExtension();
		statsName.DefaultFileExtension(".stats");
	}

	common->Printf( "writing save data to %s\n", demoName );

	idFile *cmdDemoFile = fileSystem->OpenFileWrite( demoName );
	if ( !cmdDemoFile ) {
		common->Printf( "Couldn't open for writing %s\n", demoName );
		return;
	}

	if ( save ) {
		cmdDemoFile->Write( &logIndex, sizeof( logIndex ) );
	}
	
	SaveCmdDemoToFile( cmdDemoFile );

	if ( save ) {
		idFile *statsFile = fileSystem->OpenFileWrite( statsName );
		if ( statsFile ) {
			statsFile->Write( &statIndex, sizeof( statIndex ) );
			statsFile->Write( loggedStats, numClients * statIndex * sizeof( loggedStats[0] ) );
			fileSystem->CloseFile( statsFile );
		}
	}

	fileSystem->CloseFile( cmdDemoFile );
}

/*
===============
idSessionLocal::FinishCmdLoad
===============
*/
void idSessionLocal::FinishCmdLoad() {
}

/*
===============
idSessionLocal::StartPlayingCmdDemo
===============
*/
void idSessionLocal::StartPlayingCmdDemo(const char *demoName) {
	if (com_fixedTic.GetInteger()) {
		common->Error( "Cmd demo is not compatible with uncapped FPS" );
	}
	// exit any current game
	Stop();

	idStr fullDemoName = "demos/";
	fullDemoName += demoName;
	fullDemoName.DefaultFileExtension( ".cdemo" );
	cmdDemoFile = fileSystem->OpenFileRead(fullDemoName);

	if ( cmdDemoFile == NULL ) {
		common->Printf( "Couldn't open %s\n", fullDemoName.c_str() );
		return;
	}

	guiLoading = uiManager->FindGui( "guis/map/loading.gui", true, false, true );
	//cmdDemoFile->Read(&loadGameTime, sizeof(loadGameTime));

	LoadCmdDemoFromFile(cmdDemoFile);

	// start the map
	ExecuteMapChange();

	cmdDemoFile = fileSystem->OpenFileRead(fullDemoName);

	// have to do this twice as the execmapchange clears the cmddemofile
	LoadCmdDemoFromFile(cmdDemoFile);

	// run one frame to get the view angles correct
	RunGameTic(USERCMD_MSEC, false);
}

/*
===============
idSessionLocal::TimeCmdDemo
===============
*/
void idSessionLocal::TimeCmdDemo( const char *demoName ) {
	StartPlayingCmdDemo( demoName );
	ClearWipe();
	UpdateScreen();

	int		startTime = Sys_Milliseconds();
	int		count = 0;
	int		minuteStart, minuteEnd;
	float	sec;

	// run all the frames in sequence
	minuteStart = startTime;

	while( cmdDemoFile ) {
		RunGameTic(USERCMD_MSEC, false);
		count++;

		if ( count / 3600 != ( count - 1 ) / 3600 ) {
			minuteEnd = Sys_Milliseconds();
			sec = ( minuteEnd - minuteStart ) / 1000.0;
			minuteStart = minuteEnd;
			common->Printf( "minute %i took %3.1f seconds\n", count / 3600, sec );
			UpdateScreen();
		}
	}

	int		endTime = Sys_Milliseconds();
	sec = ( endTime - startTime ) / 1000.0;
	common->Printf( "%i seconds of game, replayed in %5.1f seconds\n", count / 60, sec );
}

/*
===============
idSessionLocal::UnloadMap

Performs cleanup that needs to happen between maps, or when a
game is exited.
Exits with mapSpawned = false
===============
*/
void idSessionLocal::UnloadMap() {
	StopPlayingRenderDemo();

	// end the current map in the game
	if ( game ) {
		game->MapShutdown();
	}

	if ( cmdDemoFile ) {
		fileSystem->CloseFile( cmdDemoFile );
		cmdDemoFile = NULL;
	}

	if ( writeDemo ) {
		StopRecordingRenderDemo();
	}

	mapSpawned = false;
}

/*
===============
idSessionLocal::LoadLoadingGui
===============
*/
void idSessionLocal::LoadLoadingGui( const char *mapName ) {
	// load / program a gui to stay up on the screen while loading
	idStr stripped = mapName;
	stripped.StripFileExtension();
	stripped.StripPath();

	char guiMap[ MAX_STRING_CHARS ];
	strncpy( guiMap, va( "guis/map/%s.gui", stripped.c_str() ), MAX_STRING_CHARS );
	// give the gamecode a chance to override
	game->GetMapLoadingGUI( guiMap );

	if ( uiManager->CheckGui( guiMap ) )
	{
		guiLoading = uiManager->FindGui( guiMap, true, false, true );
	}
	else
	{
		guiLoading = uiManager->FindGui( "guis/map/loading.gui", true, false, true );

		// grayman #4594 - use different images based on aspect ratios
		switch (gameLocal.DetermineAspectRatio())
		{
		default:
		case 0:
			guiLoading->HandleNamedEvent("loadBackground_4x3");
			break;
		case 1:
			guiLoading->HandleNamedEvent("loadBackground_16x9");
			break;
		case 2:
			guiLoading->HandleNamedEvent("loadBackground_16x10");
			break;
		case 3:
			guiLoading->HandleNamedEvent("loadBackground_5x4");
			break;
		case 4:
			guiLoading->HandleNamedEvent("loadBackground_16x9tv");
			break;
		case 5:
			guiLoading->HandleNamedEvent("loadBackground_21x9");
		}
	}

	guiLoading->SetStateFloat( "map_loading", 0.0f );
}

/*
===============
idSessionLocal::SetBytesNeededForMapLoad
===============
*/
void idSessionLocal::SetBytesNeededForMapLoad( const char *mapName, int bytesNeeded ) {
	idDecl *mapDecl = const_cast<idDecl *>(declManager->FindType( DECL_MAPDEF, mapName, false ));
	idDeclEntityDef *mapDef = static_cast<idDeclEntityDef *>( mapDecl );

	if ( com_updateLoadSize.GetBool() && mapDef ) {
		// we assume that if com_updateLoadSize is true then the file is writable

		mapDef->dict.SetInt( "size0", bytesNeeded );

		idStr declText = "\nmapDef ";
		declText += mapDef->GetName();
		declText += " {\n";
		for (int i=0; i<mapDef->dict.GetNumKeyVals(); i++) {
			const idKeyValue *kv = mapDef->dict.GetKeyVal( i );
			if ( kv && (kv->GetKey().Cmp("classname") != 0 ) ) {
				declText += "\t\"" + kv->GetKey() + "\"\t\t\"" + kv->GetValue() + "\"\n";
			}
		}
		declText += "}";
		mapDef->SetText( declText );
		mapDef->ReplaceSourceFileText();
	}
}

/*
===============
idSessionLocal::ExecuteMapChange

Performs the initialization of a game based on mapSpawnData, used for both single
player and multiplayer, but not for renderDemos, which don't
create a game at all.
Exits with mapSpawned = true
===============
*/
bool idSessionLocal::ExecuteMapChange(idFile* savegameFile, bool noFadeWipe ) {
	int		i;
	bool	reloadingSameMap;

	// extract the map name from serverinfo
	idStr mapString = mapSpawnData.serverInfo.GetString( "si_map" );

	idStr fullMapName = "maps/";
	fullMapName += mapString;
	fullMapName.StripFileExtension();

	// don't do the deferred caching if we are reloading the same map
	if ( fullMapName == currentMapName ) {
		reloadingSameMap = true;
	} else {
		reloadingSameMap = false;
		currentMapName = fullMapName;
	}

	idStr traceText = va("map %s", mapString.c_str());
	if (savegameFile)
		traceText += va("\nload %s", savegameFile->GetName());
	if (reloadingSameMap)
		traceText += "\n(samemap)";
	TRACE_CPU_SCOPE_STR("idSessionLocal::ExecuteMapChange", traceText);

	// close console and remove any prints from the notify lines
	console->Close();

	// mute sound
	soundSystem->SetMute( true );

	// clear all menu sounds
	menuSoundWorld->ClearAllSoundEmitters();

	// unpause the game sound world
	// NOTE: we UnPause again later down. not sure this is needed
	if ( sw->IsPaused() ) {
		sw->UnPause();
	}

	if ( !noFadeWipe ) {
		// capture the current screen and start a wipe
		StartWipe( "wipeMaterial", true );

		// immediately complete the wipe to fade out the level transition
		// run the wipe to completion
		CompleteWipe();
	}

	// shut down the existing game if it is running
	UnloadMap();

	R_ToggleSmpFrame(); // duzenko 4848: FIXME find a better place to clear the "next frame" data
	R_ToggleSmpFrame();	// duzenko 5065: apparently R_ToggleSmpFrame does not like being called once

	// note which media we are going to need to load
	if ( !reloadingSameMap ) {
		declManager->BeginLevelLoad();
		renderSystem->BeginLevelLoad();
		soundSystem->BeginLevelLoad();
	}

	uiManager->BeginLevelLoad();
	uiManager->Reload( true );

	// set the loading gui that we will wipe to
	LoadLoadingGui( mapString );

	// cause prints to force screen updates as a pacifier,
	// and draw the loading gui instead of game draws
	insideExecuteMapChange = true;

	/* grayman #3763 - no longer used
	// if this works out we will probably want all the sizes in a def file although this solution will 
	// work for new maps etc. after the first load. we can also drop the sizes into the default.cfg
	fileSystem->ResetReadCount();
	if ( !reloadingSameMap  ) {
		bytesNeededForMapLoad = GetBytesNeededForMapLoad( mapString.c_str() );
	} else {
		bytesNeededForMapLoad = 30 * 1024 * 1024;
	}
	*/

	ClearWipe();

	// let the loading gui spin for 1 second to animate out
	ShowLoadingGui();

	// note any warning prints that happen during the load process
	common->ClearWarnings( mapString );

	// release the mouse cursor
	// before we do this potentially long operation
	Sys_GrabMouseCursor( false );

	{
		numClients = 1;
	} 
	
	int start = Sys_Milliseconds();

	common->Printf( "--------- Map Initialization ---------\n" );
	common->Printf( "Map: %s\n", mapString.c_str() );

	// let the renderSystem load all the geometry
	if ( !rw->InitFromMap( fullMapName ) ) {
		common->Error( "Couldn't load %s", fullMapName.c_str() );
	}

	// for the synchronous networking we needed to roll the angles over from
	// level to level, but now we can just clear everything
	usercmdGen->InitForNewMap();
	memset( &mapSpawnData.mapSpawnUsercmd, 0, sizeof( mapSpawnData.mapSpawnUsercmd ) );

	// set the user info
	for ( i = 0; i < numClients; i++ ) {
		game->SetUserInfo( i, mapSpawnData.userInfo[i], false, false );
		game->SetPersistentPlayerInfo( i, mapSpawnData.persistentPlayerInfo[i] );
	}

	// load and spawn all other entities ( from a savegame possibly )
	if ( savegameFile ) {
		if ( game->InitFromSaveGame( fullMapName + ".map", rw, sw, savegameFile ) == false ) {
			
			// Loadgame failed
			// STiFU #4531: We used to do an initialized load of the map at this point. 
			// This is now controlled from the outside, however.
			return false;
			
			/*		
			game->SetServerInfo( mapSpawnData.serverInfo );
			game->InitFromNewMap( fullMapName + ".map", rw, sw, false, false, Sys_Milliseconds() );*/
		}
	} else {
		game->SetServerInfo( mapSpawnData.serverInfo );
		game->InitFromNewMap( fullMapName + ".map", rw, sw, false, false, Sys_Milliseconds() );
	}

	if ( !savegameFile) {
		// spawn players
		for ( i = 0; i < numClients; i++ ) {
			game->SpawnPlayer( i );
		}
	}

	// actually purge/load the media
	if ( !reloadingSameMap ) {
		renderSystem->EndLevelLoad();
		soundSystem->EndLevelLoad( mapString.c_str() );
		declManager->EndLevelLoad();
		SetBytesNeededForMapLoad( mapString.c_str(), fileSystem->GetReadCount() );
	}
	uiManager->EndLevelLoad();

	if (!savegameFile) {
		// run a few frames to allow everything to settle
		for ( i = 0; i < 10; i++ ) {
			game->RunFrame( mapSpawnData.mapSpawnUsercmd );
		}
	}

	common->Printf ("-----------------------------------\n");

	int	msec = Sys_Milliseconds() - start;
	common->Printf( "%6d msec to load %s\n", msec, mapString.c_str() );

	// let the renderSystem generate interactions now that everything is spawned
	rw->GenerateAllInteractions();

	common->PrintWarnings();

	if ( guiLoading /*&& bytesNeededForMapLoad*/ ) {
		float pct = guiLoading->State().GetFloat( "map_loading" );
		if ( pct < 0.0f ) {
			pct = 0.0f;
		}
		while ( pct < 1.0f ) {
			guiLoading->SetStateFloat( "map_loading", pct );
			guiLoading->StateChanged( com_frameTime );
			Sys_GenerateEvents();
			UpdateScreen();
			pct += 0.05f;
		}
	}

	// capture the current screen and start a wipe
	StartWipe( "wipe2Material" );

	usercmdGen->Clear();

	// start saving commands for possible writeCmdDemo usage
	logIndex = 0;
	statIndex = 0;
	lastSaveIndex = 0;

	// don't bother spinning over all the tics we spent loading
	lastGameTic = latchedTicNumber = com_ticNumber;

	// remove any prints from the notify lines
	console->ClearNotifyLines();

	// stop drawing the laoding screen
	insideExecuteMapChange = false;

	Sys_SetPhysicalWorkMemory( -1, -1 );

	// set the game sound world for playback
	soundSystem->SetPlayingSoundWorld( sw );

	// when loading a save game the sound is paused
	if ( sw->IsPaused() ) {
		// unpause the game sound world
		sw->UnPause();
	}

	// restart entity sound playback
	soundSystem->SetMute( false );

	// we are valid for game draws now
	mapSpawned = true;
	Sys_ClearEvents();

	return true;
}

/*
===============
GetNextQuicksaveFilename

If all quicksave slots are used, return the oldest filename for overwriting.
If not all quicksave slots are used, return a new filename. SteveL #4191
===============
*/
const idStr GetNextQuicksaveFilename()
{
	// Get the list of existing save games
	idStrList fileList;
	idList<fileTIME_T> fileTimes;
	sessLocal.GetSaveGameList( fileList, fileTimes ); // fileTimes is sorted, most recent first

	// Count the number of quicksaves and remember the oldest one we saw
	idStr oldestFile;
	int quicksaveCounter = 0;
	idStr quicksaveName = common->Translate( "#str_07178" );
	quicksaveName.Replace( " ", "_" ); // grayman #4398

	for ( int i = 0; i < fileList.Num(); i++ )
	{
		idStr filename = fileList[fileTimes[i].index];
		if ( filename.IcmpPrefix(quicksaveName) == 0 )
		{
			oldestFile = filename;
			++quicksaveCounter;
		}
	}

	// Choose the save file name
	idStr result;
	if ( quicksaveCounter < idSessionLocal::com_numQuickSaves.GetInteger() ) {
		result = FindUnusedFileName( ("savegames/" + quicksaveName + "_%d.save").c_str() );
		result.StripLeading( "savegames/" );
		result.StripTrailing( ".save" );
	} else
		result = oldestFile;
	return result;
}

/*
===============
GetNextQuicksaveFilename

SteveL #4191, support multiple quicksaves
===============
*/
const idStr GetMostRecentQuicksaveFilename()
{
	// Get the list of existing save games
	idStrList fileList;
	idList<fileTIME_T> fileTimes;
	sessLocal.GetSaveGameList( fileList, fileTimes ); 

	// fileTimes is sorted, most recent first. Find the first quick save
	idStr filename;
	idStr quicksaveName = common->Translate( "#str_07178" );
	quicksaveName.Replace( " ", "_" ); // grayman #4398
	
	int idx = 0;
	for ( ; idx < fileList.Num(); idx++ )
	{
		filename = fileList[fileTimes[idx].index];
		if ( filename.IcmpPrefix(quicksaveName) == 0 )
		{
			break;
		}
	}

	// Choose the save file name
	idStr result;
	if ( idx == fileList.Num() )	// No quicksaves
	{
		result = quicksaveName;		// Let the user see the same console error as before, can't find quicksave
	}
	else
	{
		result = filename;
	}
	return result;
}

/*
===============
LoadGame_f
===============
*/
void LoadGame_f( const idCmdArgs &args ) {
	console->Close();
	if ( args.Argc() < 2 || idStr::Icmp(args.Argv(1), "quick" ) == 0 ) {
		idStr saveName = GetMostRecentQuicksaveFilename();
		sessLocal.LoadGame( saveName );
	} else {
		sessLocal.LoadGame( args.Argv(1) );
	}
}

/*
===============
SaveGame_f
===============
*/
void SaveGame_f( const idCmdArgs &args ) {
	if ( args.Argc() < 2 || idStr::Icmp( args.Argv(1), "quick" ) == 0 ) {
		idStr saveName = GetNextQuicksaveFilename();
		if ( sessLocal.SaveGame( saveName ) ) {
			common->Printf( "%s\n", saveName.c_str() );
		}
	} else {
		if ( sessLocal.SaveGame( args.Argv(1) ) ) {
			common->Printf( "Saved %s\n", args.Argv(1) );
		}
	}
	qglFinish();
}

/*
===============
Session_Hitch_f
===============
*/
void Session_Hitch_f( const idCmdArgs &args ) {
	idSoundWorld *sw = soundSystem->GetPlayingSoundWorld();
	if ( sw ) {
		soundSystem->SetMute(true);
		sw->Pause();
		Sys_EnterCriticalSection();
	}
	if ( args.Argc() == 2 ) {
		Sys_Sleep( atoi(args.Argv(1)) );
	} else {
		Sys_Sleep( 100 );
	}
	if ( sw ) {
		Sys_LeaveCriticalSection();
		sw->UnPause();
		soundSystem->SetMute(false);
	}
}

/*
====================
SimulateEscape_f
====================
*/
void SimulateEscape_f( const idCmdArgs & ) {
	Sys_QueEvent( 0, SE_KEY, K_ESCAPE, 1, 0, nullptr );
}

/*
===============
idSessionLocal::ScrubSaveGameFileName

Turns a bad file name into a good one or your money back
===============
*/
void idSessionLocal::ScrubSaveGameFileName( idStr &saveFileName ) const {

	saveFileName.RemoveColors();
	// Tels: fix 02682: Just strip ".save", but keep potentially other "extensions" like
	//	 .01 so we turn "test_1.03.save" into "test_1_03" and not "test_1".
	saveFileName.Remove(".save");

	// we just modify characters, but keep the length
	int len = saveFileName.Length();
	for ( int i = 0; i < len; i++ ) {
		const unsigned char c = (const unsigned char)saveFileName[i]; // grayman #4737
		// random junk or a control character including space
		if ( ( c <= 32  ) ||
			 (( c >= 127) && (c <= 159)) || // grayman #4737 - replace special chars, but allow UTF chars
		     ( strchr( ",.~!@#$%^&*()[]{}<>\\|/=?+;:-'\"", saveFileName[i] ) ) ) {
			saveFileName[i] = '_';
		}
		// else keep character
	}
	if (len == 0)
	{
		// disallow empty file names (if the user uses ".save", or ".save.save" etc)
		saveFileName = "default";
	}
}


/*
===============
idSessionLocal::SaveGame
===============
*/
bool idSessionLocal::SaveGame( const char *saveName, bool autosave, bool skipCheck ) {
	int i;
	idStr gameFile, previewFile, descriptionFile, mapName;

	gameFile = saveName; // Obsttorte: moved upwards as needed earlier

	if ( !mapSpawned ) {
		common->Printf( "Not playing a game.\n" );
		return false;
	}

	if ( game->GetPersistentPlayerInfo( 0 ).GetInt( "health" ) <= 0 ) {
		// "Must be alive" and "Unable to save"
		ShowMessageBox( MSG_OK, common->Translate ( "#str_02012" ), common->Translate ( "#str_02013" ), true );
		common->Printf( "You must be alive to save the game\n" );
		return false;
	}

	if ( !game->PlayerReady() ) {
		common->Printf( "Can't save until you start the map.\n" );
		return false;
	}

	if (!skipCheck)
	{
		if (game->savegamesDisallowed())
		{
			common->Printf("Manual saving is disabled!\n");
			return false;
		}

		// grayman #4398 - account for multi-word I18N replacements for "Quicksave"
		idStr s = common->Translate("#str_07178");
		s.Replace( " ", "_" );

		if ( game->quicksavesDisallowed() && ( gameFile.IcmpPrefix(s) == 0 ) ) // SteveL tweaked to use l18n while working on #4191
		{
			common->Printf("Quicksaves disabled!\n");
			return false;
		}
	}
	if ( Sys_GetDriveFreeSpace( cvarSystem->GetCVarString( "fs_savepath" ) ) < 25 ) {
		// "Not enough space" and "Unable to save"
		ShowMessageBox( MSG_OK, common->Translate ( "#str_02014" ), common->Translate ( "#str_02013" ), true );
		common->Printf( "Not enough drive space to save the game\n" );
		return false;
	}

	idSoundWorld *pauseWorld = soundSystem->GetPlayingSoundWorld();
	if ( pauseWorld ) {
		pauseWorld->Pause();
		soundSystem->SetPlayingSoundWorld( NULL );
	}

	// setup up paths
	
	ScrubSaveGameFileName( gameFile );

	gameFile = "savegames/" + gameFile;
	gameFile.SetFileExtension( ".save" );

	idStr previewExtension = com_savegame_preview_format.GetString();
	if ( !(previewExtension == "jpg" || previewExtension == "tga") ) {
		common->Warning( "Unknown preview image extension %s, falling back to default.", previewExtension.c_str() );
		previewExtension = "tga";
	}
	previewFile = gameFile;
	previewFile.SetFileExtension( previewExtension.c_str() );

	descriptionFile = gameFile;
	descriptionFile.SetFileExtension( ".txt" );

	// Open savegame file
	idFile *fileOut = fileSystem->OpenFileWrite( gameFile );
	if ( fileOut == NULL ) {
		common->Warning( "Failed to open save file '%s'", gameFile.c_str() );
		if ( pauseWorld ) {
			soundSystem->SetPlayingSoundWorld( pauseWorld );
			pauseWorld->UnPause();
		}
		return false;
	}

	// Write SaveGame Header: 
	// Game Name / Version / Map Name / Persistant Player Info

	// game
	const char *gamename = GAME_NAME;
	fileOut->WriteString( gamename );

	// version
	fileOut->WriteInt( SAVEGAME_VERSION );

	// map
	mapName = mapSpawnData.serverInfo.GetString( "si_map" );
	fileOut->WriteString( mapName );

	// persistent player info
	for ( i = 0; i < MAX_ASYNC_CLIENTS; i++ ) {
		mapSpawnData.persistentPlayerInfo[i] = game->GetPersistentPlayerInfo( i );
		mapSpawnData.persistentPlayerInfo[i].WriteToFileHandle( fileOut );
	}

	// let the game save its state
	game->SaveGame( fileOut );

	// close the sava game file
	fileSystem->CloseFile( fileOut );

	//stgatilov: clear old screenshots with this name (if present)
	{
		idStr tmpName = previewFile;
		tmpName.SetFileExtension("tga");
		fileSystem->RemoveFile(tmpName.c_str());
		tmpName.SetFileExtension("jpg");
		fileSystem->RemoveFile(tmpName.c_str());
	}

	// Write screenshot
	if ( !autosave ) {
		qglFinish();
		game->Draw( 0 );
		// need to make the changes to the vertex cache accessible to the backend
		vertexCache.EndFrame();

		// stgatilov: render image to buffer
		int width, height;
		renderSystem->GetCurrentRenderCropSize(width, height);
#ifdef __ANDROID__ //karin: RGBA
		byte *imgData = (byte*)Mem_Alloc(height * width * 4);
		renderSystem->CaptureRenderToBuffer(imgData);
#else
        byte *imgData = (byte*)Mem_Alloc(height * width * 3);
		renderSystem->CaptureRenderToBuffer(imgData);

		{ // convert to RGBA since image processing functions use that
			byte *newImg = (byte*)Mem_Alloc(height * width * 4);
			bool ok = SIMDProcessor->ConvertRowToRGBA8(imgData, width * height, 24, false, newImg);
			assert(ok);
			Mem_Free(imgData);
			imgData = newImg;
		}
#endif
		// downsample the image to reduce file size
		while ( width > 480 && height > 270 ) {
			imgData = R_MipMap( imgData, width, height );
			width >>= 1;
			height >>= 1;
		}

		// save image to file
		idImageWriter wr;
		wr.Source(imgData, width, height, 4);
		wr.Dest(fileSystem->OpenFileWrite(previewFile.c_str(), "fs_modSavePath"));
		wr.Flip();
		wr.WriteExtension(previewExtension.c_str());
		Mem_Free(imgData);

		R_ClearCommandChain( frameData );
		qglFinish();
	}

	// Write description, which is just a text file with
	// the unclean save name on line 1, map name on line 2, screenshot on line 3
	idFile *fileDesc = fileSystem->OpenFileWrite( descriptionFile );
	if ( fileDesc == NULL ) {
		common->Warning( "Failed to open description file '%s'", descriptionFile.c_str() );
		if ( pauseWorld ) {
			soundSystem->SetPlayingSoundWorld( pauseWorld );
			pauseWorld->UnPause();
		}
		return false;
	}

	idStr description = saveName;
	description.Replace( "\\", "\\\\" );
	description.Replace( "\"", "\\\"" );

	const idDeclEntityDef *mapDef = static_cast<const idDeclEntityDef *>(declManager->FindType( DECL_MAPDEF, mapName, false ));
	if ( mapDef ) {
		mapName = common->Translate( mapDef->dict.GetString( "name", mapName ) );
	}

	fileDesc->Printf( "\"%s\"\n", description.c_str() );
	fileDesc->Printf( "\"%s\"\n", mapName.c_str());

	if ( autosave ) {
		idStr sshot = mapSpawnData.serverInfo.GetString( "si_map" );
		sshot.StripPath();
		sshot.StripFileExtension();
		fileDesc->Printf( "\"guis/assets/autosave/%s\"\n", sshot.c_str() );
	} else {
		fileDesc->Printf( "\"\"\n" );
	}

	fileSystem->CloseFile( fileDesc );

	if ( pauseWorld ) {
		soundSystem->SetPlayingSoundWorld( pauseWorld );
		pauseWorld->UnPause();
	}

	syncNextGameFrame = true;
	qglFinish();


	return true;
}


bool idSessionLocal::LoadGame(const char *saveName, eSaveConflictHandling conflictHandling)
{
	if (strlen(saveName) == 0)
		saveName = lastSaveName;
	else
		lastSaveName = saveName;

	bool error = false;

	const bool checkVersion = conflictHandling == eSaveConflictHandling_QueryUser;
	if (checkVersion)
	{
		int savegameRevision = 0;
		const SavegameValidity val = IsSavegameValid(saveName, &savegameRevision);
		if (val == savegame_invalid)
			error = true;
		else if (val == savegame_versionMismatch)
		{
			// Version conflict. Ask what to do.

			// If not yet active, open the main menu		
			if (!guiActive)
			{
				console->Close();
				StartMenu();
			}

			GuiMessage msg;
			msg.type = GuiMessage::MSG_CUSTOM;
			msg.title = common->Translate("#str_10180"); // "Savegame Version Mismatch"

			msg.positiveCmd = "close_msg_box;LoadGameInitialized;";
			msg.positiveLabel = common->Translate("#str_10183"); // "Mapstart";

			msg.negativeCmd = "close_msg_box;";
			msg.negativeLabel = common->Translate("#str_07203"); // "Cancel";

			// Check for mixed revisions
			RevisionTracker& RevTracker = RevisionTracker::Instance();
			idStr gameRevision;
			gameRevision = va("%d", RevTracker.GetHighestRevision());

			if (cv_force_savegame_load.GetBool())
			{
				msg.message = va(
					common->Translate("#str_10182"), // "You are running TDM revision %s, but this savegame has been created with TDM revision %d. You can cancel loading (highly recommended!), try to force load (crash to desktop likely) or load to mapstart (issues likely to occur)."
					gameRevision.c_str(), savegameRevision
				);

				// Add third button for force-load
				msg.okCmd = "close_msg_box;LoadGameForced;";
				msg.okLabel = common->Translate("#str_10184"); // "Force-Load";
			}
			else
			{
				msg.message = va(
					common->Translate("#str_10181"), // "You are running TDM revision %s, but this savegame has been created with TDM revision %d. You can cancel loading (highly recommended!) or load to mapstart (issues likely to occur)."
					gameRevision.c_str(), savegameRevision
				);
			}

			gameLocal.AddMainMenuMessage(msg);

			return false;
		}
	}

	if (!error)
	{
		// Savegame is valid, so load it
		const bool doInitializedLoad = conflictHandling == eSaveConflictHandling_LoadMapStart;
		if (!DoLoadGame(saveName, doInitializedLoad))
		{
			error = true;
		}
	}

	if (error)
	{
		// If not yet active, open the main menu		
		if (!guiActive)
		{
			console->Close();
			StartMenu();
		}

		// Show error message
		GuiMessage msg;
		msg.type = GuiMessage::MSG_OK;
		msg.title = common->Translate("#str_02000"); //  "Error";
		msg.message = common->Translate("#str_10185"); // "Failed to load savegame.";
		msg.okCmd = "close_msg_box;";

		gameLocal.AddMainMenuMessage(msg);
		return false;
	}

	return true;
}

/*
===============
idSessionLocal::DoLoadGame
===============
*/
bool idSessionLocal::DoLoadGame( const char *saveName, const bool initializedLoad) {
	//Hide the dialog box if it is up.
	StopBox();

	idFile* savegameFile = NULL;
	idStr saveMap;
	if (!ParseSavegamePreamble(saveName, &savegameFile, &saveMap))
	{
		fileSystem->CloseFile(savegameFile);
		savegameFile = NULL;

		return false;
	}

	if (initializedLoad)
	{
		fileSystem->CloseFile(savegameFile);
		savegameFile = NULL;
		common->DPrintf("Doing initialized load instead of loading a v%d savegame\n", savegameVersion);
	}
	else
	{
		common->DPrintf("loading a v%d savegame\n", savegameVersion);
	}

	// Start loading map
	mapSpawnData.serverInfo.Clear();

	mapSpawnData.serverInfo = *cvarSystem->MoveCVarsToDict( CVAR_SERVERINFO );
	mapSpawnData.serverInfo.Set( "si_gameType", "singleplayer" );

	mapSpawnData.serverInfo.Set( "si_map", saveMap );

	mapSpawnData.syncedCVars.Clear();
	mapSpawnData.syncedCVars = *cvarSystem->MoveCVarsToDict( CVAR_NETWORKSYNC );

	mapSpawnData.mapSpawnUsercmd[0] = usercmdGen->TicCmd( latchedTicNumber );
	// make sure no buttons are pressed
	mapSpawnData.mapSpawnUsercmd[0].buttons = 0;

	bool success = ExecuteMapChange(savegameFile);
	if (success)
		SetGUI( NULL, NULL );

	if (savegameFile) {
		fileSystem->CloseFile( savegameFile );
		savegameFile = NULL;
	}

	return success;
}

// STiFU #4531: TDM savegame version check
idSessionLocal::SavegameValidity idSessionLocal::IsSavegameValid(const char *saveName, int* savegameRevision)
{
	if (!saveName)
		return savegame_invalid;

	SavegameValidity retVal = savegame_valid;
	idStr saveMap;
	idFile* savegameFile = NULL;
	if (!ParseSavegamePreamble(saveName, &savegameFile, &saveMap))
	{
		retVal = savegame_invalid;
		if (savegameRevision)
			*savegameRevision = -1;
	}

	if (retVal == savegame_valid)
	{
		idRestoreGame savegame(savegameFile);
		savegame.ReadHeader();
		if (savegameRevision)
			*savegameRevision = savegame.GetCodeRevision();

		RevisionTracker& RevTracker = RevisionTracker::Instance();

		if (savegameVersion != SAVEGAME_VERSION || savegame.GetCodeRevision() != RevTracker.GetSavegameRevision())
		{
			common->Warning("Savegame Version mismatch!");
			retVal = savegame_versionMismatch;
		}
	}

	if (savegameFile)
	{
		fileSystem->CloseFile(savegameFile);
		savegameFile = NULL;
	}

	return retVal;
}

bool idSessionLocal::ParseSavegamePreamble(const char * saveName, idFile** savegameFile, idStr* pSaveMap)
{
	if (!saveName || !pSaveMap || !savegameFile)
		return false;

	idStr in, loadFile, gamename;

	loadFile = saveName;
	ScrubSaveGameFileName(loadFile);
	loadFile.SetFileExtension(".save");

	in = "savegames/";
	in += loadFile;

	// Open savegame file
	// only allow loads from the game directory because we don't want a base game to load
	idStr game = cvarSystem->GetCVarString("fs_currentfm");
	*savegameFile = fileSystem->OpenFileRead(in, game.Length() ? game : NULL);

	if (*savegameFile == NULL) {
		common->Warning("Couldn't open savegame file %s", in.c_str());
		return false;
	}

	// Read in save game header
	// Game Name / Version / Map Name / Persistant Player Info

	// game
	(*savegameFile)->ReadString(gamename);

	// if this isn't a savegame for the correct game, abort loadgame
	if (gamename != GAME_NAME) {
		common->Warning("Attempted to load an invalid savegame: %s", in.c_str());
		return false;
	}

	// version
	(*savegameFile)->ReadInt(savegameVersion);

	// map
	(*savegameFile)->ReadString(*pSaveMap);

	// persistent player info
	for (int i = 0; i < MAX_ASYNC_CLIENTS; i++) {
		mapSpawnData.persistentPlayerInfo[i].ReadFromFileHandle(*savegameFile);
	}

	return pSaveMap->Length() > 0;
}

/*
===============
idSessionLocal::ProcessEvent
===============
*/
bool idSessionLocal::ProcessEvent( const sysEvent_t *event ) {
	// hitting escape anywhere brings up the menu
	if ( !guiActive && event->evType == SE_KEY && event->evValue2 == 1 && event->evValue == K_ESCAPE ) {
		console->Close();
		if ( game ) {
			idUserInterface	*gui = NULL;
			escReply_t		op;
			op = game->HandleESC( &gui );
			if ( op == ESC_IGNORE ) {
				return true;
			} else if ( op == ESC_GUI ) {
				SetGUI( gui, NULL );
				return true;
			}
		}
		StartMenu();
		return true;
	}

	// let the pull-down console take it if desired
	if ( console->ProcessEvent( event, false ) ) {
		return true;
	}

	// if we are testing a GUI, send all events to it
	if ( guiTest ) {
		// hitting escape exits the testgui
		if ( event->evType == SE_KEY && event->evValue2 == 1 && event->evValue == K_ESCAPE ) {
			guiTest = NULL;
			return true;
		}
		
		static const char *cmd;
		cmd = guiTest->HandleEvent( event, com_frameTime );
		if ( cmd && cmd[0] ) {
			common->Printf( "testGui event returned: '%s'\n", cmd );
		}
		return true;
	}

	// menus / etc
	if ( guiActive ) {
		MenuEvent( event );
		return true;
	}

	// if we aren't in a game, force the console to take it
	if ( !mapSpawned ) {
		console->ProcessEvent( event, true );
		return true;
	}

	// in game, exec bindings for all key downs
	if ( event->evType == SE_KEY && event->evValue2 == 1 ) {
		idKeyInput::ExecKeyBinding( event->evValue );
		return true;
	}

	return false;
}

/*
===============
idSessionLocal::DrawWipeModel

Draw the fade material over everything that has been drawn
===============
*/
void	idSessionLocal::DrawWipeModel() {
	int		latchedTic = com_ticNumber;

	if (  wipeStartTic >= wipeStopTic ) {
		return;
	}

	if ( !wipeHold && latchedTic >= wipeStopTic ) {
		return;
	}

	float fade = ( float )( latchedTic - wipeStartTic ) / ( wipeStopTic - wipeStartTic );
	renderSystem->SetColor4( 1, 1, 1, fade );
	renderSystem->DrawStretchPic( 0, 0, 640, 480, 0, 0, 1, 1, wipeMaterial );
}

/*
===============
idSessionLocal::AdvanceRenderDemo
===============
*/
void idSessionLocal::AdvanceRenderDemo( bool singleFrameOnly ) {
	if ( lastDemoTic == -1 ) {
		lastDemoTic = latchedTicNumber - 1;
	}

	int skipFrames = 0;

	if ( !aviCaptureMode && !timeDemo && !singleFrameOnly ) {
		skipFrames = ( (latchedTicNumber - lastDemoTic) / USERCMD_PER_DEMO_FRAME ) - 1;
		// never skip too many frames, just let it go into slightly slow motion
		if ( skipFrames > 4 ) {
			skipFrames = 4;
		}
		lastDemoTic = latchedTicNumber - latchedTicNumber % USERCMD_PER_DEMO_FRAME;
	} else {
		// always advance a single frame with avidemo and timedemo
		lastDemoTic = latchedTicNumber; 
	}

	while( skipFrames > -1 ) {
		int		ds = DS_FINISHED;

		readDemo->ReadInt( ds );
		if ( ds == DS_FINISHED ) {
			if ( numDemoFrames != 1 ) {
				// if the demo has a single frame (a demoShot), continuously replay
				// the renderView that has already been read
				Stop();
				StartMenu();
			}
			break;
		}
		if ( ds == DS_RENDER ) {
			if ( rw->ProcessDemoCommand( readDemo, &currentDemoRenderView, &demoTimeOffset ) ) {
				// a view is ready to render
				skipFrames--;
				numDemoFrames++;
			}
			continue;
		}
		if ( ds == DS_SOUND ) {
			sw->ProcessDemoCommand( readDemo );
			continue;
		}
		// appears in v1.2, with savegame format 17
		if ( ds == DS_VERSION ) {
			readDemo->ReadInt( renderdemoVersion );
			common->Printf( "reading a v%d render demo\n", renderdemoVersion );
			// set the savegameVersion to current for render demo paths that share the savegame paths
			savegameVersion = SAVEGAME_VERSION;
			continue;
		}
		common->Error( "Bad render demo token" );
	}

	if ( com_showDemo.GetBool() ) {
		common->Printf( "frame:%i DemoTic:%i latched:%i skip:%i\n", numDemoFrames, lastDemoTic, latchedTicNumber, skipFrames );
	}

}

/*
===============
idSessionLocal::DrawCmdGraph

Graphs yaw angle for testing smoothness
===============
*/
static const int	ANGLE_GRAPH_HEIGHT = 128;
static const int	ANGLE_GRAPH_STRETCH = 3;
void idSessionLocal::DrawCmdGraph() {
	if ( !com_showAngles.GetBool() ) {
		return;
	}
	renderSystem->SetColor4( 0.1f, 0.1f, 0.1f, 1.0f );
	renderSystem->DrawStretchPic( 0, 480-ANGLE_GRAPH_HEIGHT, MAX_BUFFERED_USERCMD*ANGLE_GRAPH_STRETCH, ANGLE_GRAPH_HEIGHT, 0, 0, 1, 1, whiteMaterial );
	renderSystem->SetColor4( 0.9f, 0.9f, 0.9f, 1.0f );
	for ( int i = 0 ; i < MAX_BUFFERED_USERCMD-4 ; i++ ) {
		usercmd_t	cmd = usercmdGen->TicCmd( latchedTicNumber - (MAX_BUFFERED_USERCMD-4) + i );
		int h = cmd.angles[1];
		h >>= 8;
		h &= (ANGLE_GRAPH_HEIGHT-1);
		renderSystem->DrawStretchPic( i* ANGLE_GRAPH_STRETCH, 480-h, 1, h, 0, 0, 1, 1, whiteMaterial );
	}
}

/*
===============
idSessionLocal::PacifierUpdate
===============
*/
void idSessionLocal::PacifierUpdate(loadkey_t key, int count) // grayman #3763
{
	/* 'count' is used as follows:
	  
	   - for key points, it is either '0' (not used), or the
	     total number of expected interim 'sub-key' points, which
	     is used to determine the incremental amount to be added to
	     the loading percentage as each 'sub-key' point is reported.
	  
	   - for 'sub-key' points, it is the number of objects processed,
	     but it's not used
	*/

	if ( !insideExecuteMapChange )
	{
		return;
	}

	// never do pacifier screen updates while inside the
	// drawing code, or we can have various recursive problems
	if ( insideUpdateScreen )
	{
		return;
	}

	if ( guiLoading /*&& bytesNeededForMapLoad*/ )
	{
		// grayman #3763 - new way
		// 'bytesNeededForMapLoad' is a constant,
		// so it can't reflect the varying amounts of data needed
		// to load a map. Small maps will be more accurate, and
		// large maps will be way off the mark, relegating most of
		// the map load to the time after map_loading == 100%. The
		// only accurate way to display the loading bar is to know
		// ahead of time how many bytes will need to be read, but
		// we can't know that until after the first time the map is loaded.
		//
		// Replaced with a method that determines average "% complete"
		// settings at key loading points, and
		// bump the loading bar to those settings as the load progresses.
		//
		// Leave a short time (~5s) after 100% is reached so that the loading
		// messages that replace the bar can be displayed for a short time.
		// This is done by reducing the number of textures that need to be
		// loaded, which increases the amount of progress shown as each is
		// loaded. Once pct reaches 1.00, the loading bar shows 100% and
		// switches what's painted based on what the map author has the
		// gui doing. Meanwhile, the extra textures continue to load in the
		// background until all are loaded. At that point, it's only a few
		// ms before the "Mission Loaded / Press Attack" screen is painted
		// and the player can start the mission.

		int	time = eventLoop->Milliseconds();

		switch (key)
		{
		case LOAD_KEY_START: // Start loading map
			pct = LOAD_KEY_START_PROGRESS;
			break;
		case LOAD_KEY_COLLISION_START: // Start loading collision data
			pct = LOAD_KEY_COLLISION_START_PROGRESS;
			break;
		case LOAD_KEY_COLLISION_DONE: // Collision data loaded, start spawning player
			pct = LOAD_KEY_COLLISION_DONE_PROGRESS;
			break;
		case LOAD_KEY_SPAWN_ENTITIES_START: // Player spawned, start spawning entities
			pct = LOAD_KEY_SPAWN_ENTITIES_START_PROGRESS;
			pct_delta = (LOAD_KEY_ROUTING_START_PROGRESS - LOAD_KEY_SPAWN_ENTITIES_START_PROGRESS) / idMath::Fmax(count, 1.0f);
			break;
		case LOAD_KEY_SPAWN_ENTITIES_INTERIM: // spawning entities (finer granularity)
			pct += pct_delta;
			if ( time - lastPacifierTime < 500 )
			{
				return;
			}
			break;
		case LOAD_KEY_ROUTING_START: // entities spawned, start compiling routing data
			pct = LOAD_KEY_ROUTING_START_PROGRESS;
			pct_delta = (LOAD_KEY_ROUTING_DONE_PROGRESS - LOAD_KEY_ROUTING_START_PROGRESS) / idMath::Fmax(count, 1.0f);
			break;
		case LOAD_KEY_ROUTING_INTERIM: // compiling routing data (finer granularity)
			pct += pct_delta;
			if ( time - lastPacifierTime < 500 )
			{
				return;
			}
			break;
		case LOAD_KEY_ROUTING_DONE: // routing data compiled, spawn player
			pct = LOAD_KEY_ROUTING_DONE_PROGRESS;
			break;
		case LOAD_KEY_IMAGES_START: // player spawned, start loading textures
			pct = LOAD_KEY_IMAGES_START_PROGRESS;
			
			// the -35 below guarantees there will be
			// some time between the loading bar
			// hitting 100% and the "Mission Loaded / Press Attack" screen
			pct_delta = (LOAD_KEY_DONE_PROGRESS - LOAD_KEY_IMAGES_START_PROGRESS) / idMath::Fmax(idMath::Fmax(count - 35, count/2.0f), 1.0f);
			break;
		case LOAD_KEY_IMAGES_INTERIM: // loading textures (finer granularity)
			pct += pct_delta;
			if ( time - lastPacifierTime < 500 )
			{
				return;
			}
			break;
		case LOAD_KEY_DONE: // textures loaded, mission done loading
			// send the loading gui the final pct
			break;
		default:
			break;
		}

		lastPacifierTime = time;

		guiLoading->SetStateFloat( "map_loading", pct );
		guiLoading->StateChanged( com_frameTime );
		// end of new way

		/* grayman #3763 - old way
		float n = fileSystem->GetReadCount();
		float pct = ( n / bytesNeededForMapLoad );
		guiLoading->SetStateFloat( "map_loading", pct );
		guiLoading->StateChanged( com_frameTime );
		// end of old way
		*/
	}

	Sys_GenerateEvents();

	UpdateScreen();
}

/*
===============
idSessionLocal::Draw
===============
*/
void idSessionLocal::Draw() {
	bool fullConsole = false;

	if ( insideExecuteMapChange ) {
		if ( guiLoading ) {
			guiLoading->Redraw( com_frameTime );
		}
		if ( guiActive == guiMsg ) {
			guiMsg->Redraw( com_frameTime );
		} 
	} else if ( guiTest ) {
		// if testing a gui, clear the screen and draw it
		// clear the background, in case the tested gui is transparent
		// NOTE that you can't use this for aviGame recording, it will tick at real com_frameTime between screenshots..
		renderSystem->SetColor( colorBlack );
		renderSystem->DrawStretchPic( 0, 0, 640, 480, 0, 0, 1, 1, declManager->FindMaterial( "_white" ) );
		guiTest->Redraw( com_frameTime );
	} else if ( guiActive && !guiActive->State().GetBool( "gameDraw" ) ) {
		
		// draw the frozen gui in the background
		if ( guiActive == guiMsg && guiMsgRestore ) {
			guiMsgRestore->Redraw( com_frameTime );
		}

		guiActive->UpdateSubtitles();	//stgatilov #2454
		guiActive->Redraw( com_frameTime );
	} else if ( readDemo ) {
		rw->RenderScene( currentDemoRenderView );
		renderSystem->DrawDemoPics();
	} else if ( mapSpawned ) {
		bool gameDraw = false;
		// normal drawing for both single and multi player
		if ( !com_skipGameDraw.GetBool() && GetLocalClientNum() >= 0 ) {
			// draw the game view
			int	start = Sys_Milliseconds();
			gameDraw = game->Draw( GetLocalClientNum() );
			int end = Sys_Milliseconds();
			time_gameDraw += ( end - start );	// note time used for com_speeds
		}
		if ( !gameDraw ) {
			renderSystem->SetColor( colorBlack );
			renderSystem->DrawStretchPic( 0, 0, 640, 480, 0, 0, 1, 1, declManager->FindMaterial( "_white" ) );
		}

		// save off the 2D drawing from the game
		if ( writeDemo ) {
			renderSystem->WriteDemoPics();
		}
	} else {
#if ID_CONSOLE_LOCK
		if ( com_allowConsole.GetBool() ) {
			console->Draw( true );
		} else {
			emptyDrawCount++;
			if ( emptyDrawCount > 5 ) {
				// it's best if you can avoid triggering the watchgod by doing the right thing somewhere else
				assert( false );
				common->Warning( "idSession: triggering mainmenu watchdog" );
				emptyDrawCount = 0;
				StartMenu();
			}
			renderSystem->SetColor4( 0, 0, 0, 1 );
			renderSystem->DrawStretchPic( 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 1, 1, declManager->FindMaterial( "_white" ) );
		}
#else
		// draw the console full screen - this should only ever happen in developer builds
		console->Draw( true );
#endif
		fullConsole = true;
	}

#if ID_CONSOLE_LOCK
	if ( !fullConsole && emptyDrawCount ) {
		common->DPrintf( "idSession: %d empty frame draws\n", emptyDrawCount );
		emptyDrawCount = 0;
	}
	fullConsole = false;
#endif

	// draw the wipe material on top of this if it hasn't completed yet
	DrawWipeModel();
	
	// draw debug graphs
	DrawCmdGraph();

	// draw the half console / notify console on top of everything
	if ( !fullConsole ) {
		console->Draw( false );
	}

#ifdef __ANDROID__ //karin: sync session state to Q3E
    Sys_SyncState();
#endif
}

/*
===============
idSessionLocal::UpdateScreen
===============
*/
void idSessionLocal::UpdateScreen( bool outOfSequence ) {

#ifdef _WIN32

	if ( com_editors ) {
		if ( !Sys_IsWindowVisible() ) {
			return;
		}
	}
#endif

	if ( insideUpdateScreen ) {
		return;
//		common->FatalError( "idSessionLocal::UpdateScreen: recursively called" );
	}

	insideUpdateScreen = true;

	// if this is a long-operation update and we are in windowed mode,
	// release the mouse capture back to the desktop
	if ( outOfSequence ) {
		Sys_GrabMouseCursor( false );
	}
	
	renderSystem->BeginFrame( renderSystem->GetScreenWidth(), renderSystem->GetScreenHeight() );

	if (com_speeds.GetBool()) {
		time_backendLast = backEnd.pc.msecLast;
		time_frontendLast = tr.pc.frontEndMsecLast;
		renderSystem->EndFrame(&time_frontend, &time_backend);
	} else {
		renderSystem->EndFrame( NULL, NULL );
	}

	insideUpdateScreen = false;
}

/*
===============
idSessionLocal::Frame
===============
*/
void idSessionLocal::Frame() {

	if ( com_asyncSound.GetInteger() == 0 ) {
		soundSystem->AsyncUpdate( Sys_Milliseconds() );
	}

	// Editors that completely take over the game
	if ( com_editorActive && ( com_editors & ( EDITOR_RADIANT | EDITOR_GUI ) ) ) {
		return;
	}

	// if the console is down, we don't need to hold
	// the mouse cursor
	if ( console->Active() || com_editorActive ) {
		Sys_GrabMouseCursor( false );
	} else {
		Sys_GrabMouseCursor( true );
	}
	
	//nbohr1more: disable SMP for debug render tools
	if (
		r_showSurfaceInfo.GetBool() ||
		r_showDepth.GetBool() ||
		r_showViewEntitys.GetBool() || // frontend may invalidate viewEntity pointers, e.g. when LOD model changes
		r_materialOverride.GetString()[0] != '\0'
	) {
		no_smp = true;
	} else {
		no_smp = false;
	}

	// save the screenshot and audio from the last draw if needed
	if ( aviCaptureMode ) {
		idStr	name;

		name = va("demos/%s/%s_%05i.tga", aviDemoShortName.c_str(), aviDemoShortName.c_str(), aviTicStart );

		float ratio = 30.0f / ( 1000.0f / USERCMD_MSEC / com_aviDemoTics.GetInteger() );
		aviDemoFrameCount += ratio;
		if ( aviTicStart + 1 != ( int )aviDemoFrameCount ) {
			// skipped frames so write them out
			int c = aviDemoFrameCount - aviTicStart;
			while ( c-- ) {
				renderSystem->TakeScreenshot( com_aviDemoWidth.GetInteger(), com_aviDemoHeight.GetInteger(), name, com_aviDemoSamples.GetInteger(), NULL );
				name = va("demos/%s/%s_%05i.tga", aviDemoShortName.c_str(), aviDemoShortName.c_str(), ++aviTicStart );
			}
		}
		aviTicStart = aviDemoFrameCount;

		// remove any printed lines at the top before taking the screenshot
		console->ClearNotifyLines();

		// this will call Draw, possibly multiple times if com_aviDemoSamples is > 1
		renderSystem->TakeScreenshot( com_aviDemoWidth.GetInteger(), com_aviDemoHeight.GetInteger(), name, com_aviDemoSamples.GetInteger(), NULL );
	}

	// at startup, we may be backwards
	if (latchedTicNumber > com_ticNumber) {
		latchedTicNumber = com_ticNumber;
	}

	// see which async tic should happen before continuing
	int	minTic;
	if ( com_fixedTic.GetInteger() ) {
		// stgatilov: don't wait for async tics, just model & render as fast as we can
		minTic = latchedTicNumber;
	}
	else {
		// stgatilov: don't do anything until at least one async tic has passed
		// that's because we tie game ticks to async tics
		minTic = latchedTicNumber + 1;
	}

	if (com_minTics.GetInteger() > 1) {
		// stgatilov: looks like some rarely used debug setting
		minTic = lastGameTic + com_minTics.GetInteger();
	}

	if (readDemo) {
		if (!timeDemo && numDemoFrames != 1) {
			minTic = lastDemoTic + USERCMD_PER_DEMO_FRAME;
		} else {
			// timedemos and demoshots will run as fast as they can, other demos
			// will not run more than 30 hz
			minTic = latchedTicNumber;
		}
	} else if (writeDemo) {
		minTic = lastGameTic + USERCMD_PER_DEMO_FRAME;		// demos are recorded at 30 hz
	}

	// Spin in place if needed when frame cap is active. 
	// The game should yield the cpu if it is running over 60 hz, 
	// because there is fundamentally nothing useful for it to do.
	while (true) {
		latchedTicNumber = com_ticNumber;
		if (latchedTicNumber >= minTic) {
			break;
		}
		Sys_WaitForEvent( TRIGGER_EVENT_ONE );	//wait for event from async thread
	}

	// send frame and mouse events to active guis
	GuiFrameEvents();

	// advance demos
	if ( readDemo ) {
		AdvanceRenderDemo( false );
		return;
	}

	//------------ single player game tics --------------

	gameTicsToRun = 0;
	
	if ( !mapSpawned || guiActive ) {
		if ( !com_asyncInput.GetBool() ) {
			// early exit, won't do RunGameTic .. but still need to update mouse position for GUIs
			usercmdGen->GetDirectUsercmd();
		}
	}

	if ( !mapSpawned ) {
		return;
	}

	if ( guiActive ) {
		lastGameTic = latchedTicNumber;
		return;
	}

	// check for user info changes
	if ( cvarSystem->GetModifiedFlags() & CVAR_USERINFO ) {
		mapSpawnData.userInfo[0] = *cvarSystem->MoveCVarsToDict( CVAR_USERINFO );
		game->SetUserInfo( 0, mapSpawnData.userInfo[0], false, false );
		cvarSystem->ClearModifiedFlags( CVAR_USERINFO );
	}

	// see how many usercmds we are going to run
	int	numCmdsToRun = latchedTicNumber - lastGameTic;

	// don't let a long onDemand sound load unsync everything
	if ( timeHitch ) {
		int	skip = timeHitch / USERCMD_MSEC;
		lastGameTic += skip;
		numCmdsToRun -= skip;
		timeHitch = 0;
	}

	// don't get too far behind after a hitch
	if ( numCmdsToRun > com_maxTicsPerFrame.GetInteger() ) {
		lastGameTic = latchedTicNumber - com_maxTicsPerFrame.GetInteger();
	}

	// never use more than USERCMD_PER_DEMO_FRAME,
	// which makes it go into slow motion when recording
	if ( writeDemo ) {
		int fixedTic = USERCMD_PER_DEMO_FRAME;
		// we should have waited long enough
		if ( numCmdsToRun < fixedTic ) {
			common->Error( "idSessionLocal::Frame: numCmdsToRun < fixedTic" );
		}
		// we may need to dump older commands
		lastGameTic = latchedTicNumber - fixedTic;
	} else if ( (com_fixedTic.GetInteger() > 0) && com_timescale.GetFloat() == 1 ) {
		// this may cause commands run in a previous frame to
		// be run again if we are going at above the real time rate
		lastGameTic = latchedTicNumber - com_fixedTic.GetInteger();
	} else if (	aviCaptureMode ) {
		lastGameTic = latchedTicNumber - com_aviDemoTics.GetInteger();
	}

	// force only one game frame update this frame.  the game code requests this after skipping cinematics
	// so we come back immediately after the cinematic is done instead of a few frames later which can
	// cause sounds played right after the cinematic to not play.
	if ( syncNextGameFrame ) {
		lastGameTic = latchedTicNumber - 1;
		syncNextGameFrame = false;
	}

	if (com_fixedTic.GetInteger() > 0) {
		gameTimestepTotal = com_frameDelta;
		//stgatilov #4924: if too much time passed since last frame,
		//then split this game tic into many short tics
		//long tics easily make physics unstable, so game tic duration should be under control
		gameTicsToRun = (gameTimestepTotal - 1) / com_maxTicTimestep.GetInteger() + 1;	//divide by 17 ms, rounding up
		if (gameTicsToRun > com_maxTicsPerFrame.GetInteger()) {
			//if everything is too bad, slow game down instead of modeling insane number of ticks per frame
			gameTicsToRun = com_maxTicsPerFrame.GetInteger();
			gameTimestepTotal = USERCMD_MSEC * gameTicsToRun;
		}
		lastGameTic = latchedTicNumber - gameTicsToRun;
	}
	else {
		gameTicsToRun = latchedTicNumber - lastGameTic;
		gameTimestepTotal = USERCMD_MSEC * gameTicsToRun;
	}

	// create client commands, which will be sent directly
	// to the game
	if ( com_showTics.GetBool() ) {
		common->Printf( "%i ", gameTicsToRun );
	}
}

/*
================
idSessionLocal::RunGameTic
================
*/
void idSessionLocal::RunGameTic(int timestepMs, bool minorTic) {
	logCmd_t	logCmd;
	usercmd_t	cmd;

	TRACE_CPU_SCOPE( "RunGameTic" )

	// if we are doing a command demo, read or write from the file
	if ( cmdDemoFile ) {
		if ( !cmdDemoFile->Read( &logCmd, sizeof( logCmd ) ) ) {
			common->Printf( "Command demo completed at logIndex %i\n", logIndex );
			fileSystem->CloseFile( cmdDemoFile );
			cmdDemoFile = NULL;
			if ( aviCaptureMode ) {
				EndAVICapture();
				Shutdown();
			}
			// we fall out of the demo to normal commands
			// the impulse and chat character toggles may not be correct, and the view
			// angle will definitely be wrong
		} else {
			cmd = logCmd.cmd;
			cmd.ByteSwap();
			logCmd.consistencyHash = LittleInt( logCmd.consistencyHash );
		}
	}
	
	// if we didn't get one from the file, get it locally
	if ( !cmdDemoFile ) {
		// get a locally created command
		if ( com_asyncInput.GetBool() ) {
			cmd = usercmdGen->TicCmd( lastGameTic );
		} else {
			cmd = usercmdGen->GetDirectUsercmd();
		}
	}
	lastGameTic++;

	// stgatilov: allow automation to intercept gameplay controls
	if (com_automation.GetBool()) {
		bool automationRules = Auto_GetUsercmd(cmd);
	}

	// Obsttorte - check if we should save the game

	idStr saveGameName = game->triggeredSave();
	if (!saveGameName.IsEmpty())
	{
		if (cvarSystem->GetCVarBool("tdm_nosave"))
		{
			cvarSystem->SetCVarBool("tdm_nosave",false);
			SaveGame(saveGameName.c_str());
			cvarSystem->SetCVarBool("tdm_nosave",true);
		}
		else
		{
			SaveGame(saveGameName.c_str(), true, true);
		}
	}

	// run the game logic every player move
	int	start = Sys_Milliseconds();
	gameReturn_t	ret = game->RunFrame( &cmd, timestepMs, minorTic );
	int end = Sys_Milliseconds();
	time_gameFrame += end - start;	// note time used for com_speeds

	// check for constency failure from a recorded command
	if ( cmdDemoFile ) {
		if ( ret.consistencyHash != logCmd.consistencyHash ) {
			common->Printf( "Consistency failure on logIndex %i\n", logIndex );
			Stop();
			return;
		}
	}

	// save the cmd for cmdDemo archiving
	if ( logIndex < MAX_LOGGED_USERCMDS ) {
		loggedUsercmds[logIndex].cmd = cmd;
		// save the consistencyHash for demo playback verification
		loggedUsercmds[logIndex].consistencyHash = ret.consistencyHash;
		if (logIndex % 30 == 0 && statIndex < MAX_LOGGED_STATS) {
			loggedStats[statIndex].health = ret.health;
			loggedStats[statIndex].heartRate = ret.heartRate;
			loggedStats[statIndex].stamina = ret.stamina;
			loggedStats[statIndex].combat = ret.combat;
			statIndex++;
		}
		logIndex++;
	}

	syncNextGameFrame = ret.syncNextGameFrame;

	if ( ret.sessionCommand[0] ) {
		// execute commands from backend, most importantly map change
		ExecuteFrameCommand(ret.sessionCommand, true);
	}
}

void idSessionLocal::RunGameTics() {
	// run game tics
	for (int i = 0; i < gameTicsToRun; ++i) {
		int deltaMs = gameTimestepTotal * (i+1) / gameTicsToRun - gameTimestepTotal * i / gameTicsToRun;
		if (com_fixedTic.GetInteger() == 0) 
			assert(deltaMs == USERCMD_MSEC);

		// stgatilov #5992: optimize all tics except for the first one
		bool minorTic = com_useMinorTics.GetBool() && (i > 0);

		RunGameTic(deltaMs, minorTic);
		if (!mapSpawned || syncNextGameFrame) {
			break;
		}
	}
}

void idSessionLocal::DrawFrame() {
	// draw lightgem
	if (mapSpawned && !com_skipGameDraw.GetBool() && GetLocalClientNum() >= 0) {
		game->DrawLightgem(GetLocalClientNum());
	}

	// render next frame
	Draw();
	// close any gui drawing
	tr.guiModel->EmitFullScreen();
	tr.guiModel->Clear();
	// add the swapbuffers command
	emptyCommand_t *cmd = (emptyCommand_t *)R_GetCommandBuffer( sizeof( *cmd ) );
	cmd->commandId = RC_TONEMAP;
}

/*
===============
idSessionLocal::FrontendThreadFunction

Runs game tics and draw call creation in a background thread.
===============
*/
void idSessionLocal::FrontendThreadFunction() {
	// stgatilov #4550: set FPU props (FTZ + DAZ, etc.)
	sys->ThreadStartup();

	while( true ) {
		// stgatilov #4550: update FPU props (e.g. NaN exceptions)
		sys->ThreadHeartbeat( "Frontend" );

		{ // lock scope
			TRACE_CPU_SCOPE_COLOR( "Frontend::Wait", TRACE_COLOR_IDLE )
			std::unique_lock< std::mutex > lock( signalMutex );
			// wait for render thread
			while( !frontendActive && !shutdownFrontend ) {
				signalFrontendThread.wait( lock );
			}
			if( shutdownFrontend ) {
				return;
			}
		}
		try {
			RunGameTics();
			DrawFrame();
		} catch( std::shared_ptr< ErrorReportedException > e ) {
			frontendException = e;
		} 
		
		{ // lock scope - signal render thread
			std::unique_lock< std::mutex > lock( signalMutex );
			frontendActive = false;
			signalMainThread.notify_one();
		}
	}
}

bool idSessionLocal::IsFrontend() const {
#if 0	
	return std::this_thread::get_id() == frontendThread.get_id();
#else
#if WIN32
	return Sys_GetCurrentThreadID() == GetThreadId( (HANDLE)frontendThread );
#else
	return Sys_GetCurrentThreadID() == frontendThread;
#endif
#endif
}

/*
===============
idSessionLocal::ActivateFrontend

Called before the rendering backend starts working.
Activates game tic and frontend rendering on a separate thread.
===============
*/
void idSessionLocal::ActivateFrontend() {
	if( com_smp.GetBool() && !guiActive && !no_smp ) {
		std::unique_lock<std::mutex> lock( signalMutex );
		frontendActive = true;
		signalFrontendThread.notify_one();
	} else {
		// run game tics and frontend drawing serially
		RunGameTics();
		DrawFrame();
	}
}

/*
===============
idSessionLocal::WaitForFrontendCompletion

Called after the rendering backend finishes.
Waits for the frontend to finish preparing the next frame.
===============
*/
void idSessionLocal::WaitForFrontendCompletion() {
	if( com_smp.GetBool() ) {
		TRACE_CPU_SCOPE_COLOR( "WaitForFrontend", TRACE_COLOR_IDLE );
		std::unique_lock<std::mutex> lock( signalMutex );
		if( r_showSmp.GetBool() )
			backEnd.pc.waitedFor = frontendActive ? 'F' : '.';
		while( frontendActive ) {
			signalMainThread.wait( lock );
		}

		if( frontendException ) {
			std::shared_ptr<ErrorReportedException> e = frontendException;
			frontendException.reset();
			throw e;
		}
	}
}

void idSessionLocal::StartFrontendThread() {
	frontendActive = shutdownFrontend = false;
	auto func = []( void *x ) -> unsigned int {
		idSessionLocal* s = (idSessionLocal*)x;
		s->FrontendThreadFunction();
		return 0; 
	};
	frontendThread = Sys_CreateThread( (xthread_t)func, this, THREAD_NORMAL, "Frontend" );
}


void idSessionLocal::ExecuteFrameCommand(const char *command, bool delayed) {
	if (delayed) {
		delayedFrameCommands.Append(command);
		return;
	}

	idCmdArgs args;
	args.TokenizeString( command, false );

	if ( !idStr::Icmp( args.Argv(0), "map" ) ) {
		// get current player states
		for ( int i = 0 ; i < numClients ; i++ ) {
			mapSpawnData.persistentPlayerInfo[i] = game->GetPersistentPlayerInfo( i );
		}
		// clear the devmap key on serverinfo, so player spawns
		// won't get the map testing items
		mapSpawnData.serverInfo.Delete( "devmap" );

		// go to the next map
		MoveToNewMap( args.Argv(1) );

		// do not process any additional game tics this frame
		syncNextGameFrame = true;
	} else if ( !idStr::Icmp( args.Argv(0), "devmap" ) ) {
		mapSpawnData.serverInfo.Set( "devmap", "1" );
		MoveToNewMap( args.Argv(1) );

		// do not process any additional game tics this frame
		syncNextGameFrame = true;
	} else if ( !idStr::Icmp( args.Argv(0), "died" ) ) {
		// restart on the same map
		mainMenuStartState = MMSS_FAILURE;
		UnloadMap();
		StartMenu();

		// do not process any additional game tics this frame
		syncNextGameFrame = true;
	} else if ( !idStr::Icmp( args.Argv(0), "disconnect" ) ) {
		mainMenuStartState = MMSS_SUCCESS;
		cmdSystem->BufferCommandText( CMD_EXEC_INSERT, "stoprecording ; disconnect" );
		// Check for final save trigger - the player PVS is freed at this point, so we can go ahead and save the game
		if( gameLocal.m_TriggerFinalSave ) {
			gameLocal.m_TriggerFinalSave = false;

			idStr savegameName = va( "Mission %d Final", gameLocal.m_MissionManager->GetCurrentMissionIndex() + 1 );
			cmdSystem->BufferCommandText( CMD_EXEC_INSERT, va( "savegame '%s'", savegameName.c_str() ) );
		}
	}
}

void idSessionLocal::ExecuteDelayedFrameCommands() {
	if (delayedFrameCommands.Num() == 0)
		return;
	for (int i = 0; i < delayedFrameCommands.Num(); i++)
		ExecuteFrameCommand(delayedFrameCommands[i], false);
	delayedFrameCommands.SetNum(0);
}

/*
===============
idSessionLocal::Init

Called in an orderly fashion at system startup,
so commands, cvars, files, etc are all available
===============
*/
void idSessionLocal::Init() {

	common->Printf( "-------- Initializing Session --------\n" );

	cmdSystem->AddCommand( "writePrecache", Sess_WritePrecache_f, CMD_FL_SYSTEM|CMD_FL_CHEAT, "writes precache commands" );

	cmdSystem->AddCommand( "map", Session_Map_f, CMD_FL_SYSTEM, "loads a map", idCmdSystem::ArgCompletion_MapName );
	cmdSystem->AddCommand( "devmap", Session_DevMap_f, CMD_FL_SYSTEM, "loads a map in developer mode", idCmdSystem::ArgCompletion_MapName );
	cmdSystem->AddCommand( "testmap", Session_TestMap_f, CMD_FL_SYSTEM, "tests a map", idCmdSystem::ArgCompletion_MapName );
	cmdSystem->AddCommand( "testdevmap", Session_TestDevmap_f, CMD_FL_SYSTEM, "tests a map in developer mode", idCmdSystem::ArgCompletion_MapName );

	cmdSystem->AddCommand( "writeCmdDemo", Session_WriteCmdDemo_f, CMD_FL_SYSTEM, "writes a command demo" );
	cmdSystem->AddCommand( "playCmdDemo", Session_PlayCmdDemo_f, CMD_FL_SYSTEM, "plays back a command demo" );
	cmdSystem->AddCommand( "timeCmdDemo", Session_TimeCmdDemo_f, CMD_FL_SYSTEM, "times a command demo" );
	cmdSystem->AddCommand( "exitCmdDemo", Session_ExitCmdDemo_f, CMD_FL_SYSTEM, "exits a command demo" );
	cmdSystem->AddCommand( "aviCmdDemo", Session_AVICmdDemo_f, CMD_FL_SYSTEM, "writes AVIs for a command demo" );
	cmdSystem->AddCommand( "aviGame", Session_AVIGame_f, CMD_FL_SYSTEM, "writes AVIs for the current game" );

	cmdSystem->AddCommand( "recordDemo", Session_RecordDemo_f, CMD_FL_SYSTEM, "records a demo" );
	cmdSystem->AddCommand( "stopRecording", Session_StopRecordingDemo_f, CMD_FL_SYSTEM, "stops demo recording" );
	cmdSystem->AddCommand( "playDemo", Session_PlayDemo_f, CMD_FL_SYSTEM, "plays back a demo", idCmdSystem::ArgCompletion_DemoName );
	cmdSystem->AddCommand( "timeDemo", Session_TimeDemo_f, CMD_FL_SYSTEM, "times a demo", idCmdSystem::ArgCompletion_DemoName );
	cmdSystem->AddCommand( "timeDemoQuit", Session_TimeDemoQuit_f, CMD_FL_SYSTEM, "times a demo and quits", idCmdSystem::ArgCompletion_DemoName );
	cmdSystem->AddCommand( "aviDemo", Session_AVIDemo_f, CMD_FL_SYSTEM, "writes AVIs for a demo", idCmdSystem::ArgCompletion_DemoName );
	cmdSystem->AddCommand( "compressDemo", Session_CompressDemo_f, CMD_FL_SYSTEM, "compresses a demo file", idCmdSystem::ArgCompletion_DemoName );

	cmdSystem->AddCommand( "disconnect", Session_Disconnect_f, CMD_FL_SYSTEM, "disconnects from a game" );

	cmdSystem->AddCommand( "demoShot", Session_DemoShot_f, CMD_FL_SYSTEM, "writes a screenshot for a demo" );
	cmdSystem->AddCommand( "testGUI", Session_TestGUI_f, CMD_FL_SYSTEM, "tests a gui" );

	cmdSystem->AddCommand( "saveGame", SaveGame_f, CMD_FL_SYSTEM|CMD_FL_CHEAT, "saves a game" );
	cmdSystem->AddCommand( "loadGame", LoadGame_f, CMD_FL_SYSTEM|CMD_FL_CHEAT, "loads a game", idCmdSystem::ArgCompletion_SaveGame );

	cmdSystem->AddCommand( "rescanSI", Session_RescanSI_f, CMD_FL_SYSTEM, "internal - rescan serverinfo cvars and tell game" );

	cmdSystem->AddCommand( "hitch", Session_Hitch_f, CMD_FL_SYSTEM|CMD_FL_CHEAT, "hitches the game" );

	cmdSystem->AddCommand( "escape", SimulateEscape_f, CMD_FL_GAME, "simulate a press of the ESC key" );

	// the same idRenderWorld will be used for all games
	// and demos, insuring that level specific models
	// will be freed
	rw = renderSystem->AllocRenderWorld();
	sw = soundSystem->AllocSoundWorld( rw );

	menuSoundWorld = soundSystem->AllocSoundWorld( rw );

	// we have a single instance of the main menu
	ResetMainMenu();
	guiMainMenu_MapList = uiManager->AllocListGUI();
	guiMainMenu_MapList->Config( guiMainMenu, "mapList" );
	guiMsg = uiManager->FindGui( "guis/msg.gui", true, false, true );

	whiteMaterial = declManager->FindMaterial( "_white" );

	guiInGame = NULL;
	guiTest = NULL;

	guiActive = NULL;
	guiHandle = NULL;

	StartFrontendThread();
	
	common->Printf( "session initialized\n" );
	common->Printf( "--------------------------------------\n" );
}

/*
===============
idSessionLocal::GetLocalClientNum
===============
*/
int idSessionLocal::GetLocalClientNum() {
	return 0;
}

/*
===============
idSessionLocal::SetPlayingSoundWorld
===============
*/
void idSessionLocal::SetPlayingSoundWorld() {
	if ( guiActive && ( guiActive == guiMainMenu || guiActive == guiLoading || ( guiActive == guiMsg && !mapSpawned ) ) ) {
		soundSystem->SetPlayingSoundWorld( menuSoundWorld );
	} else {
		soundSystem->SetPlayingSoundWorld( sw );
	}
}

/*
===============
idSessionLocal::TimeHitch

this is used by the sound system when an OnDemand sound is loaded, so the game action
doesn't advance and get things out of sync
===============
*/
void idSessionLocal::TimeHitch( int msec ) {
	timeHitch += msec;
}

/*
===============
idSessionLocal::GetCurrentMapName
===============
*/
const char *idSessionLocal::GetCurrentMapName() {
	return currentMapName.c_str();
}

/*
===============
idSessionLocal::GetSaveGameVersion
===============
*/
int idSessionLocal::GetSaveGameVersion( void ) {
	// TODO STiFU: Possibly refactor to use proper TDM revision instead of old id savegameVersion
	return savegameVersion;
}
