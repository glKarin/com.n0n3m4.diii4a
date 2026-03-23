/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 GPL Source Code (?Doom 3 Source Code?).  

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

#include "../idlib/precompiled.h"
#pragma hdrstop

#ifdef STEAM
#include "steamstats.h"
#include "steamremote.h"
#include "steamleaderboard.h"

#endif

#include "Game_local.h"



#ifdef STEAM
#define	STEAM_GAMEPAD_TEXTLENGTH	4096
#endif

#ifdef GAME_DLL

idSys *						sys = NULL;
idCommon *					common = NULL;
idCmdSystem *				cmdSystem = NULL;
idCVarSystem *				cvarSystem = NULL;
idFileSystem *				fileSystem = NULL;
idNetworkSystem *			networkSystem = NULL;
idRenderSystem *			renderSystem = NULL;
idSoundSystem *				soundSystem = NULL;
idRenderModelManager *		renderModelManager = NULL;
idUserInterfaceManager *	uiManager = NULL;
idDeclManager *				declManager = NULL;
idAASFileManager *			AASFileManager = NULL;
idCollisionModelManager *	collisionModelManager = NULL;
idCVar *					idCVar::staticVars = NULL;

idCVar com_forceGenericSIMD( "com_forceGenericSIMD", "0", CVAR_BOOL|CVAR_SYSTEM, "force generic platform independent SIMD" );

#endif

idRenderWorld *				gameRenderWorld = NULL;		// all drawing is done to this world
idSoundWorld *				gameSoundWorld = NULL;		// all audio goes to this world

static gameExport_t			gameExport;

// global animation lib
idAnimManager				animationLib;

// the rest of the engine will only reference the "game" variable, while all local aspects stay hidden
idGameLocal					gameLocal;
idGame *					game = &gameLocal;	// statically pointed at an idGameLocal

const char *idGameLocal::sufaceTypeNames[ MAX_SURFACE_TYPES ] = {
	"none",	"metal", "stone", "flesh", "wood", "cardboard", "liquid", "glass", "plastic",
	"ricochet", "surftype10", "surftype11", "surftype12", "surftype13", "surftype14", "surftype15"
};

#ifdef _D3XP
// List of all defs used by the player that will stay on the fast timeline
static char* fastEntityList[] = {
	"player_doommarine",
		"weapon_chainsaw",
		"weapon_fists",
		"weapon_flashlight",
		"weapon_rocketlauncher",
		"projectile_rocket",
		"weapon_machinegun",
		"projectile_bullet_machinegun",
		"weapon_pistol",
		"projectile_bullet_pistol",
		"weapon_handgrenade",
		"projectile_grenade",
		"weapon_bfg",
		"projectile_bfg",
		"weapon_chaingun",
		"projectile_chaingunbullet",
		"weapon_pda",
		"weapon_plasmagun",
		"projectile_plasmablast",
		"weapon_shotgun",
		"projectile_bullet_shotgun",
		"weapon_soulcube",
		"projectile_soulblast",
		"weapon_shotgun_double",
		"projectile_shotgunbullet_double",
		"weapon_grabber",
		"weapon_bloodstone_active1",
		"weapon_bloodstone_active2",
		"weapon_bloodstone_active3",
		"weapon_bloodstone_passive",
		NULL };
#endif
/*
===========
GetGameAPI
============
*/
#if __MWERKS__
#pragma export on
#endif
#if __GNUC__ >= 4
#pragma GCC visibility push(default)
#endif
extern "C" gameExport_t *GetGameAPI( gameImport_t *import ) {
#if __MWERKS__
#pragma export off
#endif

	if ( import->version == GAME_API_VERSION ) {

		// set interface pointers used by the game
		sys							= import->sys;
		common						= import->common;
		cmdSystem					= import->cmdSystem;
		cvarSystem					= import->cvarSystem;
		fileSystem					= import->fileSystem;
		networkSystem				= import->networkSystem;
		renderSystem				= import->renderSystem;
		soundSystem					= import->soundSystem;
		renderModelManager			= import->renderModelManager;
		uiManager					= import->uiManager;
		declManager					= import->declManager;
		AASFileManager				= import->AASFileManager;
		collisionModelManager		= import->collisionModelManager;
	}

	// set interface pointers used by idLib
	idLib::sys					= sys;
	idLib::common				= common;
	idLib::cvarSystem			= cvarSystem;
	idLib::fileSystem			= fileSystem;

	// setup export interface
	gameExport.version = GAME_API_VERSION;
	gameExport.game = game;
	gameExport.gameEdit = gameEdit;

	return &gameExport;
}
#if __GNUC__ >= 4
#pragma GCC visibility pop
#endif

/*
===========
TestGameAPI
============
*/
void TestGameAPI( void ) {
	gameImport_t testImport;
	gameExport_t testExport;

	testImport.version				= GAME_API_VERSION;
	testImport.sys						= ::sys;
	testImport.common					= ::common;
	testImport.cmdSystem				= ::cmdSystem;
	testImport.cvarSystem				= ::cvarSystem;
	testImport.fileSystem				= ::fileSystem;
	testImport.networkSystem			= ::networkSystem;
	testImport.renderSystem				= ::renderSystem;
	testImport.soundSystem				= ::soundSystem;
	testImport.renderModelManager		= ::renderModelManager;
	testImport.uiManager				= ::uiManager;
	testImport.declManager				= ::declManager;
	testImport.AASFileManager			= ::AASFileManager;
	testImport.collisionModelManager	= ::collisionModelManager;

	testExport = *GetGameAPI( &testImport );
}

/*
===========
idGameLocal::idGameLocal
============
*/
idGameLocal::idGameLocal()
#ifdef STEAM
	: m_SteamCallResultTextDismissed(this, &idGameLocal::OnTextDismissed)
#endif
{
	Clear();
}

/*
===========
idGameLocal::Clear
============
*/
void idGameLocal::Clear( void ) {
	int i;

	serverInfo.Clear();
	numClients = 0;
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		userInfo[i].Clear();
		persistentPlayerInfo[i].Clear();
	}
	memset( usercmds, 0, sizeof( usercmds ) );
	memset( entities, 0, sizeof( entities ) );
	memset( spawnIds, -1, sizeof( spawnIds ) );
	firstFreeIndex = 0;
	num_entities = 0;
	spawnedEntities.Clear();
	activeEntities.Clear();
	numEntitiesToDeactivate = 0;
	sortPushers = false;
	sortTeamMasters = false;
	persistentLevelInfo.Clear();
	memset( globalShaderParms, 0, sizeof( globalShaderParms ) );
	random.SetSeed( 0 );
	world = NULL;
	frameCommandThread = NULL;
	testmodel = NULL;
	testFx = NULL;
	clip.Shutdown();
	pvs.Shutdown();
	sessionCommand.Clear();
	locationEntities = NULL;
	smokeParticles = NULL;
	editEntities = NULL;
	entityHash.Clear( 1024, MAX_GENTITIES );
	inCinematic = false;
	cinematicSkipTime = 0;
	cinematicStopTime = 0;
	cinematicMaxSkipTime = 0;
	framenum = 0;
	previousTime = 0;
	time = 0;
	vacuumAreaNum = 0;
	mapFileName.Clear();
	mapFile = NULL;
	spawnCount = INITIAL_SPAWN_COUNT;
	mapSpawnCount = 0;
	camera = NULL;
	aasList.Clear();
	aasNames.Clear();
	lastAIAlertEntity = NULL;
	lastAIAlertTime = 0;
	spawnArgs.Clear();
	gravity.Set( 0, 0, -1 );
	playerPVS.h = -1;
	playerConnectedAreas.h = -1;
	gamestate = GAMESTATE_UNINITIALIZED;
	skipCinematic = false;
	influenceActive = false;

	localClientNum = 0;
	isMultiplayer = false;
	isServer = false;
	isClient = false;
	realClientTime = 0;
	isNewFrame = true;
	clientSmoothing = 0.1f;
	entityDefBits = 0;

	nextGibTime = 0;
	globalMaterial = NULL;
	newInfo.Clear();
	lastGUIEnt = NULL;
	lastGUI = 0;

	memset( clientEntityStates, 0, sizeof( clientEntityStates ) );
	memset( clientPVS, 0, sizeof( clientPVS ) );
	memset( clientSnapshots, 0, sizeof( clientSnapshots ) );

	eventQueue.Init();
	savedEventQueue.Init();

	memset( lagometer, 0, sizeof( lagometer ) );

#ifdef _D3XP
	portalSkyEnt			= NULL;
	portalSkyActive			= false;

	ResetSlowTimeVars();
#endif

#ifdef STEAM
	steamGotStickynotes		= false;
	textinputOn				= false;
#endif

	//bc
	inCutscene = false;
	
}

/*
===========
idGameLocal::Init

  initialize the game object, only happens once at startup, not each level load
============
*/
void idGameLocal::Init( void ) {
	const idDict *dict;
	idAAS *aas;

#ifndef GAME_DLL

	TestGameAPI();

#else

	// initialize idLib
	idLib::Init();

	// register static cvars declared in the game
	idCVar::RegisterStaticVars();

	// initialize processor specific SIMD
	idSIMD::InitProcessor( "game", com_forceGenericSIMD.GetBool() );

#endif

	Printf( "--------- Initializing Game ----------\n" );
	Printf( "gamename: %s\n", GAME_VERSION );
	Printf( "gamedate: %s\n", __DATE__ );

	// register game specific decl types
	declManager->RegisterDeclType( "model",				DECL_MODELDEF,		idDeclAllocator<idDeclModelDef> );
	declManager->RegisterDeclType( "export",			DECL_MODELEXPORT,	idDeclAllocator<idDecl> );

	// register game specific decl folders
	declManager->RegisterDeclFolder( "def",				".def",				DECL_ENTITYDEF );
	declManager->RegisterDeclFolder( "fx",				".fx",				DECL_FX );
	declManager->RegisterDeclFolder( "particles",		".prt",				DECL_PARTICLE );
	declManager->RegisterDeclFolder( "af",				".af",				DECL_AF );
	declManager->RegisterDeclFolder( "newpdas",			".pda",				DECL_PDA );

	cmdSystem->AddCommand( "listModelDefs", idListDecls_f<DECL_MODELDEF>, CMD_FL_SYSTEM|CMD_FL_GAME, "lists model defs" );
	cmdSystem->AddCommand( "printModelDefs", idPrintDecls_f<DECL_MODELDEF>, CMD_FL_SYSTEM|CMD_FL_GAME, "prints a model def", idCmdSystem::ArgCompletion_Decl<DECL_MODELDEF> );

	Clear();

	idEvent::Init();
	idClass::Init();

	InitConsoleCommands();


#ifdef _D3XP
	if(!g_xp_bind_run_once.GetBool()) {
		//The default config file contains remapped controls that support the XP weapons
		//We want to run this once after the base doom config file has run so we can
		//have the correct xp binds
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "exec default.cfg\n" );
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "seta g_xp_bind_run_once 1\n" );
		cmdSystem->ExecuteCommandBuffer();
	}
#endif

	// load default scripts
	program.Startup( SCRIPT_DEFAULT );
	
#ifdef _D3XP
	//BSM Nerve: Loads a game specific main script file
	idStr gamedir;
	int i;
	for ( i = 0; i < 2; i++ ) {
		if ( i == 0 ) {
			gamedir = cvarSystem->GetCVarString( "fs_game_base" );
		} else if ( i == 1 ) {
			gamedir = cvarSystem->GetCVarString( "fs_game" );
		}
		if( gamedir.Length() > 0 ) {		
			idStr scriptFile = va( "script/%s_main.script", gamedir.c_str() );
			if ( fileSystem->ReadFile( scriptFile.c_str(), NULL ) > 0 ) {
				program.CompileFile( scriptFile.c_str() );
				program.FinishCompilation();
			}
		}
	}
#endif

	smokeParticles = new idSmokeParticles;

	// set up the aas
	dict = FindEntityDefDict( "aas_types" );
	if ( !dict ) {
		Error( "Unable to find entityDef for 'aas_types'" );
	}

	// allocate space for the aas
	const idKeyValue *kv = dict->MatchPrefix( "type" );
	while( kv != NULL ) {
		aas = idAAS::Alloc();
		aasList.Append( aas );
		aasNames.Append( kv->GetValue() );
		kv = dict->MatchPrefix( "type", kv );
	}

	gamestate = GAMESTATE_NOMAP;

	Printf( "...%d aas types\n", aasList.Num() );
	Printf( "game initialized.\n" );
	Printf( "--------------------------------------\n" );


	mainGui = uiManager->FindGui( "guis/mainmenu.gui", true, false, true );



	//bc
	nextAutocompleteTime = 0;

	cvarSystem->SetCVarBool("savedone", true);

#ifdef STEAM

	//debug.
	putenv ("SteamTenfoot=1");

	m_SteamCallResultTextDismissed.Unregister();

	
	if (common->SteamInitSuccessful() )
	{
		m_SteamCallResultTextDismissed.Register(this, &idGameLocal::OnTextDismissed);

		steamInitialized = true;
		//SteamUtils()->SetOverlayNotificationPosition(k_EPositionTopRight);

		assert(!g_SteamStats && !g_SteamRemote && !g_SteamLeaderboard);
		g_SteamStats = new CSteamStats(g_Stats);
		g_SteamRemote = new CSteamRemote();
		g_SteamLeaderboard = new CSteamLeaderboard();

		

		cvarSystem->SetCVarString("ui_name", SteamFriends()->GetPersonaName() );
		
		
		workshopLegalGui = uiManager->FindGui( "guis/steam_workshopagree.gui", true, false, true );

		//Set up the main menu friend list.
		//if (mainGui)
		//{
			


			/*

			idList<idStr> people;
			int i;
			int friendCount;
			idList<CSteamID>	tempIDs;

	
			friendCount = SteamFriends()->GetFriendCount(k_EFriendFlagImmediate);
			friendIDs.Clear();
			tempIDs.Clear();

			//add local player first.
			people.Append(SteamFriends()->GetPersonaName());	
			tempIDs.Append(SteamUser()->GetSteamID());

			common->Printf("STEAM: found %d friend candidates.\n", friendCount);
			for (i = 0; i < friendCount; i++)
			{
				CSteamID id = SteamFriends()->GetFriendByIndex(i, k_EFriendFlagImmediate);
				tempIDs.Append(id);
				people.Append(SteamFriends()->GetFriendPersonaName(id));

				//common->Printf("STEAM: adding friend candidate: %s\n", SteamFriends()->GetFriendPersonaName(id));
			}

			const int numPeople = people.Num();
			idListGUI * guiList_people = uiManager->AllocListGUI();
			guiList_people = uiManager->AllocListGUI();
			guiList_people->Config( mainGui, "friendlist" );

			for ( int i = 0; i < numPeople; i++ )
			{
				//local info.
				if (i <= 0)
				{
					guiList_people->Push( people[i] );
					friendIDs.Append(tempIDs[i]);
					continue;
				}

				//Hack: because app ownership check is a little cumbersome, just check if player has ever started the game.
				if (g_SteamStats->RequestUserStats(tempIDs[i]))
				{
					int value;

					

					if (g_SteamStats->GetUserStat(tempIDs[i], "started", value))
					{
						if (value > 0)
						{
							guiList_people->Push( people[i] );
							friendIDs.Append(tempIDs[i]);

							common->Printf("STEAM: adding friend: %s\n", people[i].c_str());
						}
					}
					else
					{
						//common->Warning("STEAM: failed to get 'started' stat: %s\n", people[i].c_str());
					}
				}
			}	

			uiManager->FreeListGUI( guiList_people );	
			*/
		//}
		
		#define FLOTILLA			55000
		#define AIRFORTE			55020	
		#define ATOMZOMBIESMASHER	55040	
		#define TFOL				214700
	
		if (!cvarSystem->GetCVarBool("g_own_flotilla"))
			cvarSystem->SetCVarBool("g_own_flotilla", SteamApps()->BIsSubscribedApp(FLOTILLA));

		if (!cvarSystem->GetCVarBool("g_own_airforte"))
			cvarSystem->SetCVarBool("g_own_airforte", SteamApps()->BIsSubscribedApp(AIRFORTE));

		if (!cvarSystem->GetCVarBool("g_own_azs"))
			cvarSystem->SetCVarBool("g_own_azs", SteamApps()->BIsSubscribedApp(ATOMZOMBIESMASHER));

		if (!cvarSystem->GetCVarBool("g_own_tfol"))
			cvarSystem->SetCVarBool("g_own_tfol", SteamApps()->BIsSubscribedApp(TFOL));
		

		SteamFriends()->SetRichPresence("status", "Cybercracking.");
	}
	else
	{
		steamInitialized = false;
		common->Warning("STEAM: initialization error.\n");
	}

	workshopLegalOpen = false;
	


	if (steamInitialized)
	{
		/* obsolete. now uses leaderboard system.
		if (!g_SteamStats->SetStat("started", 1))
		{
			common->Warning("STEAM: unable to set start stat.\n");
		}
		*/

		if (mainGui)
		{
			g_SteamLeaderboard->GetFriendBoard();
		}
	}
	
	//bc this is only fired once at the beginning of the game.
	hasInitializedLeaderboardlist = false;

	g_SteamRemote->CloudLoad();
#endif

	// Populate the resolution list with supported display modes.
	if (mainGui)
	{
#ifdef USE_SDL
		idStr resChoiceString = "";
		{ // Generate a ';' seperated list of display modes from what the main display supports.
			bool has720 = false;
			// TODO: maybe support choosing non-main display modes.
			int displayModeIndex = 0;
			SDL_DisplayMode desktopDisplayMode;
			SDL_GetCurrentDisplayMode(displayModeIndex, &desktopDisplayMode);
			int displayModeCount = SDL_GetNumDisplayModes(displayModeIndex);
			SDL_DisplayMode currentDisplayMode;
			for (int i = (displayModeCount-1); i >= 0; --i)
			{
				SDL_GetDisplayMode(displayModeIndex, i, &currentDisplayMode);

				// Skip display modes that are not the same pixel-format as the current display mode
				if (currentDisplayMode.format != desktopDisplayMode.format)
					continue;

				// Skip any refresh rates that are not default.
				if (currentDisplayMode.refresh_rate != desktopDisplayMode.refresh_rate)
					continue;

				if (i < (displayModeCount-1) )
					resChoiceString += ";";

				resChoiceString += currentDisplayMode.w;
				resChoiceString += "x";
				resChoiceString += currentDisplayMode.h;

				has720 |= currentDisplayMode.w == 1280 && currentDisplayMode.h == 720;
			}
			if (!has720) {
				resChoiceString += ";1280x720";
			}
		}

		mainGui->SetStateString("resolution_choices", resChoiceString);
#else
		mainGui->SetStateString("resolution_choices", "640x480;800x600;856x480;960x720;1024x480;1024x768;1152x864;1280x720;1280x768;1280x800;1280x960;1280x1024;1360x768;1366x768;1400x1050;1440x900;1600x900;1600x1200;1680x1050;1920x1080;1920x1200;1920x1440;2048x1536;2560x1440;2560x1600;2880x1800;3200x1080;3840x1080");
#endif
	}
}

#ifdef STEAM
void idGameLocal::ShowWorkshopLegalGui( void )
{
	if (workshopLegalOpen || gameLocal.GetLocalPlayer()->viewlookActive)
	{
		return;
	}

	GetLocalPlayer()->StartSound( "snd_confirm", SND_CHANNEL_ANY, 0, false, NULL );

	workshopLegalOpen = true;
	workshopLegalGui->Activate(true, gameLocal.time);
}
#endif

/*
===========
idGameLocal::Shutdown

  shut down the entire game
============
*/
void idGameLocal::Shutdown( void ) {

	if ( !common ) {
		return;
	}

	Printf( "------------ Game Shutdown -----------\n" );

	mpGame.Shutdown();

	MapShutdown();

	aasList.DeleteContents( true );
	aasNames.Clear();

	idAI::FreeObstacleAvoidanceNodes();

	// shutdown the model exporter
	idModelExport::Shutdown();

	idEvent::Shutdown();

	delete[] locationEntities;
	locationEntities = NULL;

	delete smokeParticles;
	smokeParticles = NULL;

	idClass::Shutdown();

	// clear list with forces
	idForce::ClearForceList();

	// free the program data
	program.FreeData();

	// delete the .map file
	delete mapFile;
	mapFile = NULL;

	// free the collision map
	collisionModelManager->FreeMap();

	ShutdownConsoleCommands();

	// free memory allocated by class objects
	Clear();

	// shut down the animation manager
	animationLib.Shutdown();
#ifdef STEAM
	// Steam teardown
	delete g_SteamStats;
	g_SteamStats = NULL;
	delete g_SteamRemote;
	g_SteamRemote = NULL;
	delete g_SteamLeaderboard;
	g_SteamLeaderboard = NULL;

	m_SteamCallResultTextDismissed.Unregister();
#endif
	Printf( "--------------------------------------\n" );

#ifdef GAME_DLL

	// remove auto-completion function pointers pointing into this DLL
	cvarSystem->RemoveFlaggedAutoCompletion( CVAR_GAME );

	// enable leak test
	Mem_EnableLeakTest( "game" );

	// shutdown idLib
	idLib::ShutDown();

#endif
}

/*
===========
idGameLocal::SaveGame

save the current player state, level name, and level state
the session may have written some data to the file already
============
*/
void idGameLocal::SaveGame( idFile *f ) {
	int i;
	idEntity *ent;
	idEntity *link;



	idSaveGame savegame( f );

	if (g_flushSave.GetBool( ) == true ) { 
		// force flushing with each write... for tracking down
		// save game bugs.
		f->ForceFlush();
	}

	savegame.WriteBuildNumber( BUILD_NUMBER );

	// go through all entities and threads and add them to the object list
	for( i = 0; i < MAX_GENTITIES; i++ ) {
		ent = entities[i];

		if ( ent ) {
			if ( ent->GetTeamMaster() && ent->GetTeamMaster() != ent ) {
				continue;
			}
			for ( link = ent; link != NULL; link = link->GetNextTeamEntity() ) {
				savegame.AddObject( link );
			}
		}
	}

	idList<idThread *> threads;
	threads = idThread::GetThreads();

	for( i = 0; i < threads.Num(); i++ ) {
		savegame.AddObject( threads[i] );
	}

	// write out complete object list
	savegame.WriteObjectList();

	program.Save( &savegame );

	savegame.WriteInt( g_skill.GetInteger() );

	savegame.WriteDict( &serverInfo );

	savegame.WriteInt( numClients );
	for( i = 0; i < numClients; i++ ) {
		savegame.WriteDict( &userInfo[ i ] );
		savegame.WriteUsercmd( usercmds[ i ] );
		savegame.WriteDict( &persistentPlayerInfo[ i ] );
	}

	for( i = 0; i < MAX_GENTITIES; i++ ) {
		savegame.WriteObject( entities[ i ] );
		savegame.WriteInt( spawnIds[ i ] );
	}

	savegame.WriteInt( firstFreeIndex );
	savegame.WriteInt( num_entities );

	// enityHash is restored by idEntity::Restore setting the entity name.

	savegame.WriteObject( world );

	savegame.WriteInt( spawnedEntities.Num() );
	for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		savegame.WriteObject( ent );
	}

	savegame.WriteInt( activeEntities.Num() );
	for( ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() ) {
		savegame.WriteObject( ent );
	}

	savegame.WriteInt( numEntitiesToDeactivate );
	savegame.WriteBool( sortPushers );
	savegame.WriteBool( sortTeamMasters );
	savegame.WriteDict( &persistentLevelInfo );

	for( i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ ) {
		savegame.WriteFloat( globalShaderParms[ i ] );
	}

	savegame.WriteInt( random.GetSeed() );
	savegame.WriteObject( frameCommandThread );

	// clip
	// push
	// pvs

	testmodel = NULL;
	testFx = NULL;

	savegame.WriteString( sessionCommand );

	// FIXME: save smoke particles

	savegame.WriteInt( cinematicSkipTime );
	savegame.WriteInt( cinematicStopTime );
	savegame.WriteInt( cinematicMaxSkipTime );
	savegame.WriteBool( inCinematic );
	savegame.WriteBool( skipCinematic );

	savegame.WriteBool( isMultiplayer );
	savegame.WriteInt( gameType );

	savegame.WriteInt( framenum );
	savegame.WriteInt( previousTime );
	savegame.WriteInt( time );

#ifdef _D3XP
	savegame.WriteInt( msec );
#endif

	savegame.WriteInt( vacuumAreaNum );

	savegame.WriteInt( entityDefBits );
	savegame.WriteBool( isServer );
	savegame.WriteBool( isClient );

	savegame.WriteInt( localClientNum );

	// snapshotEntities is used for multiplayer only

	savegame.WriteInt( realClientTime );
	savegame.WriteBool( isNewFrame );
	savegame.WriteFloat( clientSmoothing );

#ifdef _D3XP
	portalSkyEnt.Save( &savegame );
	savegame.WriteBool( portalSkyActive );

	fast.Save( &savegame );
	slow.Save( &savegame );

	savegame.WriteInt( slowmoState );
	savegame.WriteFloat( slowmoMsec );
	savegame.WriteBool( quickSlowmoReset );
#endif

	savegame.WriteBool( mapCycleLoaded );
	savegame.WriteInt( spawnCount );

	if ( !locationEntities ) {
		savegame.WriteInt( 0 );
	} else {
		savegame.WriteInt( gameRenderWorld->NumAreas() );
		for( i = 0; i < gameRenderWorld->NumAreas(); i++ ) {
			savegame.WriteObject( locationEntities[ i ] );
		}
	}

	savegame.WriteObject( camera );

	savegame.WriteMaterial( globalMaterial );

	lastAIAlertEntity.Save( &savegame );
	savegame.WriteInt( lastAIAlertTime );

	savegame.WriteDict( &spawnArgs );

	savegame.WriteInt( playerPVS.i );
	savegame.WriteInt( playerPVS.h );
	savegame.WriteInt( playerConnectedAreas.i );
	savegame.WriteInt( playerConnectedAreas.h );

	savegame.WriteVec3( gravity );

	// gamestate

	savegame.WriteBool( influenceActive );
	savegame.WriteInt( nextGibTime );

	// spawnSpots
	// initialSpots
	// currentInitialSpot
	// newInfo
	// makingBuild
	// shakeSounds

	// write out pending events
	idEvent::Save( &savegame );

	savegame.Close();



	//common->Printf("SAVE FILE %s   %s\n", f->GetFullPath(), f->GetName());
	//g_SteamRemote->CloudSave(f);


	//bc 4-21-2016 save icon. Only show it a couple seconds in, to skip the level start autosave.
	if (this->time > 2000)
	{
		gameLocal.GetLocalPlayer()->hud->HandleNamedEvent("dosaveicon");
	}



	//BC 8-31-2015
	//talk to the save station.
	idEntity			*targetScreen;
	targetScreen = gameLocal.FindEntity("func_savestation_1");

	if (targetScreen)
	{
		idUserInterface *screenGui = targetScreen->GetRenderEntity()->gui[0]; //get the gui surface.

		if (screenGui)
		{
			screenGui->HandleNamedEvent("onsave");
		}
	}

	lastSavetime = gameLocal.time;
}

/*
===========
idGameLocal::GetPersistentPlayerInfo
============
*/
const idDict &idGameLocal::GetPersistentPlayerInfo( int clientNum ) {
	idEntity	*ent;

	persistentPlayerInfo[ clientNum ].Clear();
	ent = entities[ clientNum ];
	if ( ent && ent->IsType( idPlayer::Type ) ) {
		static_cast<idPlayer *>(ent)->SavePersistantInfo();
	}

	return persistentPlayerInfo[ clientNum ];
}

/*
===========
idGameLocal::SetPersistentPlayerInfo
============
*/
void idGameLocal::SetPersistentPlayerInfo( int clientNum, const idDict &playerInfo ) {
	persistentPlayerInfo[ clientNum ] = playerInfo;
}

/*
============
idGameLocal::Printf
============
*/
void idGameLocal::Printf( const char *fmt, ... ) const {
	va_list		argptr;
	char		text[MAX_STRING_CHARS];

	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	common->Printf( "%s", text );
}

/*
============
idGameLocal::DPrintf
============
*/
void idGameLocal::DPrintf( const char *fmt, ... ) const {
	va_list		argptr;
	char		text[MAX_STRING_CHARS];

	if ( !developer.GetBool() ) {
		return;
	}

	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	common->Printf( "%s", text );
}

/*
============
idGameLocal::Warning
============
*/
void idGameLocal::Warning( const char *fmt, ... ) const {
	va_list		argptr;
	char		text[MAX_STRING_CHARS];
	idThread *	thread;

	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	thread = idThread::CurrentThread();
	if ( thread ) {
		thread->Warning( "%s", text );
	} else {
		common->Warning( "%s", text );
	}
}

/*
============
idGameLocal::DWarning
============
*/
void idGameLocal::DWarning( const char *fmt, ... ) const {
	va_list		argptr;
	char		text[MAX_STRING_CHARS];
	idThread *	thread;

	if ( !developer.GetBool() ) {
		return;
	}

	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	thread = idThread::CurrentThread();
	if ( thread ) {
		thread->Warning( "%s", text );
	} else {
		common->DWarning( "%s", text );
	}
}

/*
============
idGameLocal::Error
============
*/
void idGameLocal::Error( const char *fmt, ... ) const {
	va_list		argptr;
	char		text[MAX_STRING_CHARS];
	idThread *	thread;

	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	thread = idThread::CurrentThread();
	if ( thread ) {
		thread->Error( "%s", text );
	} else {
		common->Error( "%s", text );
	}
}

/*
===============
gameError
===============
*/
void gameError( const char *fmt, ... ) {
	va_list		argptr;
	char		text[MAX_STRING_CHARS];

	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	gameLocal.Error( "%s", text );
}

/*
===========
idGameLocal::SetLocalClient
============
*/
void idGameLocal::SetLocalClient( int clientNum ) {
	localClientNum = clientNum;
}

/*
===========
idGameLocal::SetUserInfo
============
*/
const idDict* idGameLocal::SetUserInfo( int clientNum, const idDict &userInfo, bool isClient, bool canModify ) {
	int i;
	bool modifiedInfo = false;

	this->isClient = isClient;

	if ( clientNum >= 0 && clientNum < MAX_CLIENTS ) {
		idGameLocal::userInfo[ clientNum ] = userInfo;

		// server sanity
		if ( canModify ) {

			// don't let numeric nicknames, it can be exploited to go around kick and ban commands from the server
			if ( idStr::IsNumeric( this->userInfo[ clientNum ].GetString( "ui_name" ) ) ) {
				idGameLocal::userInfo[ clientNum ].Set( "ui_name", va( "%s_", idGameLocal::userInfo[ clientNum ].GetString( "ui_name" ) ) );
				modifiedInfo = true;
			}
		
			// don't allow dupe nicknames
			for ( i = 0; i < numClients; i++ ) {
				if ( i == clientNum ) {
					continue;
				}
				if ( entities[ i ] && entities[ i ]->IsType( idPlayer::Type ) ) {
					if ( !idStr::Icmp( idGameLocal::userInfo[ clientNum ].GetString( "ui_name" ), idGameLocal::userInfo[ i ].GetString( "ui_name" ) ) ) {
						idGameLocal::userInfo[ clientNum ].Set( "ui_name", va( "%s_", idGameLocal::userInfo[ clientNum ].GetString( "ui_name" ) ) );
						modifiedInfo = true;
						i = -1;	// rescan
						continue;
					}
				}
			}
		}

		if ( entities[ clientNum ] && entities[ clientNum ]->IsType( idPlayer::Type ) ) {
			modifiedInfo |= static_cast<idPlayer *>( entities[ clientNum ] )->UserInfoChanged( canModify );
		}

		if ( !isClient ) {
			// now mark this client in game
			mpGame.EnterGame( clientNum );
		}
	}

	if ( modifiedInfo ) {
		assert( canModify );
		newInfo = idGameLocal::userInfo[ clientNum ];
		return &newInfo;
	}
	return NULL;
}

/*
===========
idGameLocal::GetUserInfo
============
*/
const idDict* idGameLocal::GetUserInfo( int clientNum ) {
	if ( entities[ clientNum ] && entities[ clientNum ]->IsType( idPlayer::Type ) ) {
		return &userInfo[ clientNum ];
	}
	return NULL;
}

/*
===========
idGameLocal::SetServerInfo
============
*/
void idGameLocal::SetServerInfo( const idDict &_serverInfo ) {
	idBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];

	serverInfo = _serverInfo;
	UpdateServerInfoFlags();

	if ( !isClient ) {
		// Let our clients know the server info changed
		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.WriteByte( GAME_RELIABLE_MESSAGE_SERVERINFO );
		outMsg.WriteDeltaDict( gameLocal.serverInfo, NULL );
		networkSystem->ServerSendReliableMessage( -1, outMsg );
	}
}


/*
===================
idGameLocal::LoadMap

Initializes all map variables common to both save games and spawned games.
===================
*/
void idGameLocal::LoadMap( const char *mapName, int randseed ) {
	int i;
	bool sameMap = (mapFile && idStr::Icmp(mapFileName, mapName) == 0);

	// clear the sound system
	gameSoundWorld->ClearAllSoundEmitters();

#ifdef _D3XP
	// clear envirosuit sound fx
	gameSoundWorld->SetEnviroSuit( false );
	gameSoundWorld->SetSlowmo( false );
#endif

	InitAsyncNetwork();

	if ( !sameMap || ( mapFile && mapFile->NeedsReload() ) ) {
		// load the .map file
		if ( mapFile ) {
			delete mapFile;
		}
		mapFile = new idMapFile;
		if ( !mapFile->Parse( idStr( mapName ) + ".map" ) ) {
			delete mapFile;
			mapFile = NULL;
			Error( "Couldn't load %s", mapName );
		}
	}
	mapFileName = mapFile->GetName();

	// load the collision map
	collisionModelManager->LoadMap( mapFile );

	numClients = 0;

	// initialize all entities for this game
	memset( entities, 0, sizeof( entities ) );
	memset( usercmds, 0, sizeof( usercmds ) );
	memset( spawnIds, -1, sizeof( spawnIds ) );
	spawnCount = INITIAL_SPAWN_COUNT;
	
	spawnedEntities.Clear();
	activeEntities.Clear();
	numEntitiesToDeactivate = 0;
	sortTeamMasters = false;
	sortPushers = false;
	lastGUIEnt = NULL;
	lastGUI = 0;

	globalMaterial = NULL;

	memset( globalShaderParms, 0, sizeof( globalShaderParms ) );

	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	num_entities	= MAX_CLIENTS;
	firstFreeIndex	= MAX_CLIENTS;

	// reset the random number generator.
	random.SetSeed( isMultiplayer ? randseed : 0 );

	camera			= NULL;
	world			= NULL;
	testmodel		= NULL;
	testFx			= NULL;

	lastAIAlertEntity = NULL;
	lastAIAlertTime = 0;
	
	previousTime	= 0;
	time			= 0;
	framenum		= 0;
	sessionCommand = "";
	nextGibTime		= 0;

#ifdef _D3XP
	portalSkyEnt			= NULL;
	portalSkyActive			= false;

	ResetSlowTimeVars();
#endif

	vacuumAreaNum = -1;		// if an info_vacuum is spawned, it will set this

	if ( !editEntities ) {
		editEntities = new idEditEntities;
	}

	gravity.Set( 0, 0, -g_gravity.GetFloat() );

	spawnArgs.Clear();

	skipCinematic = false;
	inCinematic = false;
	cinematicSkipTime = 0;
	cinematicStopTime = 0;
	cinematicMaxSkipTime = 0;
	inCutscene = false;

	clip.Init();
	pvs.Init();
	playerPVS.i = -1;
	playerConnectedAreas.i = -1;

	// load navigation system for all the different monster sizes
	for( i = 0; i < aasNames.Num(); i++ ) {
		aasList[ i ]->Init( idStr( mapFileName ).SetFileExtension( aasNames[ i ] ).c_str(), mapFile->GetGeometryCRC() );
	}

	// clear the smoke particle free list
	smokeParticles->Init();

	// cache miscellanious media references
	FindEntityDef( "preCacheExtras", false );

	if ( !sameMap ) {
		mapFile->RemovePrimitiveData();
	}

	//bc
	if (mainGui)
	{
		idStr mapShortname;
		idStr outputName;

		mapShortname = mapName;
		mapShortname.StripPath();
		mapShortname.StripFileExtension();
		mapShortname.ToLower();

		const idDecl *mapDecl = declManager->FindType( DECL_MAPDEF, mapShortname, false );
		const idDeclEntityDef *mapDef = static_cast<const idDeclEntityDef *>( mapDecl );

		if ( mapDef )
		{
			outputName = common->GetLanguageDict()->GetString( mapDef->dict.GetString( "name", mapShortname ) );
		}
		else
		{
			outputName = mapShortname;
		}

		mainGui->SetStateString("mapname", outputName.c_str());
	}
}

/*
===================
idGameLocal::LocalMapRestart
===================
*/
void idGameLocal::LocalMapRestart( ) {
	int i, latchSpawnCount;

	Printf( "----------- Game Map Restart ------------\n" );

	gamestate = GAMESTATE_SHUTDOWN;

	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( entities[ i ] && entities[ i ]->IsType( idPlayer::Type ) ) {
			static_cast< idPlayer * >( entities[ i ] )->PrepareForRestart();
		}
	}

	eventQueue.Shutdown();
	savedEventQueue.Shutdown();

	MapClear( false );



	// clear the smoke particle free list
	smokeParticles->Init();

	// clear the sound system
	if ( gameSoundWorld ) {
		gameSoundWorld->ClearAllSoundEmitters();
#ifdef _D3XP
		// clear envirosuit sound fx
		gameSoundWorld->SetEnviroSuit( false );
		gameSoundWorld->SetSlowmo( false );
#endif
	}

	// the spawnCount is reset to zero temporarily to spawn the map entities with the same spawnId
	// if we don't do that, network clients are confused and don't show any map entities
	latchSpawnCount = spawnCount;
	spawnCount = INITIAL_SPAWN_COUNT;

	gamestate = GAMESTATE_STARTUP;

	program.Restart();

	InitScriptForMap();

	MapPopulate();

	// once the map is populated, set the spawnCount back to where it was so we don't risk any collision
	// (note that if there are no players in the game, we could just leave it at it's current value)
	spawnCount = latchSpawnCount;

	// setup the client entities again
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( entities[ i ] && entities[ i ]->IsType( idPlayer::Type ) ) {
			static_cast< idPlayer * >( entities[ i ] )->Restart();
		}
	}

	gamestate = GAMESTATE_ACTIVE;

	Printf( "--------------------------------------\n" );
	

}

/*
===================
idGameLocal::MapRestart
===================
*/
void idGameLocal::MapRestart( ) {
	idBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];
	idDict		newInfo;
	int			i;
	const idKeyValue *keyval, *keyval2;

#ifdef _D3XP
	if ( isMultiplayer && isServer ) {
		char buf[ MAX_STRING_CHARS ];
		idStr gametype;
		GetBestGameType( si_map.GetString(), si_gameType.GetString(), buf );
		gametype = buf;
		if ( gametype != si_gameType.GetString() ) {
			cvarSystem->SetCVarString( "si_gameType", gametype );
		}
	}
#endif


    
	if ( isClient ) {
		LocalMapRestart();
	} else {
		newInfo = *cvarSystem->MoveCVarsToDict( CVAR_SERVERINFO );
		for ( i = 0; i < newInfo.GetNumKeyVals(); i++ ) {
			keyval = newInfo.GetKeyVal( i );
			keyval2 = serverInfo.FindKey( keyval->GetKey() );
			if ( !keyval2 ) {
				break;
			}
			// a select set of si_ changes will cause a full restart of the server
			if ( keyval->GetValue().Cmp( keyval2->GetValue() ) &&
				( !keyval->GetKey().Cmp( "si_pure" ) || !keyval->GetKey().Cmp( "si_map" ) ) ) {
				break;
			}
		}
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "rescanSI" );

		if ( i != newInfo.GetNumKeyVals() ) {
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "nextMap" );
		} else {
			outMsg.Init( msgBuf, sizeof( msgBuf ) );
			outMsg.WriteByte( GAME_RELIABLE_MESSAGE_RESTART );
			outMsg.WriteBits( 1, 1 );
			outMsg.WriteDeltaDict( serverInfo, NULL );
			networkSystem->ServerSendReliableMessage( -1, outMsg );

			LocalMapRestart();
			mpGame.MapRestart();
		}
	}

#ifdef CTF
	if ( isMultiplayer ) {
		gameLocal.mpGame.ReloadScoreboard();
		//		gameLocal.mpGame.Reset();	// force reconstruct the GUIs when reloading maps, different gametypes have different GUIs
		//		gameLocal.mpGame.UpdateMainGui();
		//		gameLocal.mpGame.StartMenu();
		//		gameLocal.mpGame.DisableMenu();
		//		gameLocal.mpGame.Precache();
	}
#endif
}

/*
===================
idGameLocal::MapRestart_f
===================
*/
void idGameLocal::MapRestart_f( const idCmdArgs &args ) {
	if ( !gameLocal.isMultiplayer || gameLocal.isClient ) {
		common->Printf( "server is not running - use spawnServer\n" );
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "spawnServer\n" );
		return;
	}

	gameLocal.MapRestart( );
}

/*
===================
idGameLocal::NextMap
===================
*/
bool idGameLocal::NextMap( void ) {
	const function_t	*func;
	idThread			*thread;
	idDict				newInfo;
	const idKeyValue	*keyval, *keyval2;
	int					i;

	if ( !g_mapCycle.GetString()[0] ) {
		Printf( common->GetLanguageDict()->GetString( "#str_04294" ) );
		return false;
	}
	if ( fileSystem->ReadFile( g_mapCycle.GetString(), NULL, NULL ) < 0 ) {
		if ( fileSystem->ReadFile( va( "%s.scriptcfg", g_mapCycle.GetString() ), NULL, NULL ) < 0 ) {
			Printf( "map cycle script '%s': not found\n", g_mapCycle.GetString() );
			return false;
		} else {
			g_mapCycle.SetString( va( "%s.scriptcfg", g_mapCycle.GetString() ) );
		}
	}

	Printf( "map cycle script: '%s'\n", g_mapCycle.GetString() );
	func = program.FindFunction( "mapcycle::cycle" );
	if ( !func ) {
		program.CompileFile( g_mapCycle.GetString() );
		func = program.FindFunction( "mapcycle::cycle" );
	}
	if ( !func ) {
		Printf( "Couldn't find mapcycle::cycle\n" );
		return false;
	}
	thread = new idThread( func );
	thread->Start();
	delete thread;

	newInfo = *cvarSystem->MoveCVarsToDict( CVAR_SERVERINFO );
	for ( i = 0; i < newInfo.GetNumKeyVals(); i++ ) {
		keyval = newInfo.GetKeyVal( i );
		keyval2 = serverInfo.FindKey( keyval->GetKey() );
		if ( !keyval2 || keyval->GetValue().Cmp( keyval2->GetValue() ) ) {
			break;
		}
	}
	return ( i != newInfo.GetNumKeyVals() );
}

/*
===================
idGameLocal::NextMap_f
===================
*/
void idGameLocal::NextMap_f( const idCmdArgs &args ) {
	if ( !gameLocal.isMultiplayer || gameLocal.isClient ) {
		common->Printf( "server is not running\n" );
		return;
	}

	gameLocal.NextMap( );
	// next map was either voted for or triggered by a server command - always restart
	gameLocal.MapRestart( );
}

/*
===================
idGameLocal::MapPopulate
===================
*/
void idGameLocal::MapPopulate( void ) {

	if ( isMultiplayer ) {
		cvarSystem->SetCVarBool( "r_skipSpecular", false );
	}
	// parse the key/value pairs and spawn entities
	SpawnMapEntities();

	// mark location entities in all connected areas
	SpreadLocations();

	// prepare the list of randomized initial spawn spots
	RandomizeInitialSpawns();

	// spawnCount - 1 is the number of entities spawned into the map, their indexes started at MAX_CLIENTS (included)
	// mapSpawnCount is used as the max index of map entities, it's the first index of non-map entities
	mapSpawnCount = MAX_CLIENTS + spawnCount - 1;

	// execute pending events before the very first game frame
	// this makes sure the map script main() function is called
	// before the physics are run so entities can bind correctly
	Printf( "==== Processing events ====\n" );
	idEvent::ServiceEvents();
}

/*
===================
idGameLocal::InitFromNewMap
===================
*/
void idGameLocal::InitFromNewMap( const char *mapName, idRenderWorld *renderWorld, idSoundWorld *soundWorld, bool isServer, bool isClient, int randseed ) {

	this->isServer = isServer;
	this->isClient = isClient;
	this->isMultiplayer = isServer || isClient;

	if ( mapFileName.Length() ) {
		MapShutdown();
	}

	Printf( "----------- Game Map Init ------------\n" );

	gamestate = GAMESTATE_STARTUP;

	gameRenderWorld = renderWorld;
	gameSoundWorld = soundWorld;

	LoadMap( mapName, randseed );

	InitScriptForMap();

	MapPopulate();

	mpGame.Reset();

	mpGame.Precache();

	// free up any unused animations
	animationLib.FlushUnusedAnims();

	gamestate = GAMESTATE_ACTIVE;

	Printf( "--------------------------------------\n" );


}

/*
=================
idGameLocal::InitFromSaveGame
=================
*/
bool idGameLocal::InitFromSaveGame( const char *mapName, idRenderWorld *renderWorld, idSoundWorld *soundWorld, idFile *saveGameFile ) {
	int i;
	int num;
	idEntity *ent;
	idDict si;

	if ( mapFileName.Length() ) {
		MapShutdown();
	}

	Printf( "------- Game Map Init SaveGame -------\n" );

	gamestate = GAMESTATE_STARTUP;

	gameRenderWorld = renderWorld;
	gameSoundWorld = soundWorld;

	idRestoreGame savegame( saveGameFile );

	savegame.ReadBuildNumber();

	// Create the list of all objects in the game
	savegame.CreateObjects();

	// Load the idProgram, also checking to make sure scripting hasn't changed since the savegame
	if ( program.Restore( &savegame ) == false ) {

		// Abort the load process, and let the session know so that it can restart the level
		// with the player persistent data.
		savegame.DeleteObjects();
		program.Restart();

		return false;
	}

	// load the map needed for this savegame
	LoadMap( mapName, 0 );

	savegame.ReadInt( i );
	g_skill.SetInteger( i );

	// precache the player
	FindEntityDef( "player_doommarine", false );

	// precache any media specified in the map
	for ( i = 0; i < mapFile->GetNumEntities(); i++ ) {
		idMapEntity *mapEnt = mapFile->GetEntity( i );

		if ( !InhibitEntitySpawn( mapEnt->epairs ) ) {
			CacheDictionaryMedia( &mapEnt->epairs );
			const char *classname = mapEnt->epairs.GetString( "classname" );
			if ( *classname != '\0' ) {
				FindEntityDef( classname, false );
			}
		}
	}

	savegame.ReadDict( &si );
	SetServerInfo( si );

	savegame.ReadInt( numClients );
	for( i = 0; i < numClients; i++ ) {
		savegame.ReadDict( &userInfo[ i ] );
		savegame.ReadUsercmd( usercmds[ i ] );
		savegame.ReadDict( &persistentPlayerInfo[ i ] );
	}

	for( i = 0; i < MAX_GENTITIES; i++ ) {
		savegame.ReadObject( reinterpret_cast<idClass *&>( entities[ i ] ) );
		savegame.ReadInt( spawnIds[ i ] );

		// restore the entityNumber
		if ( entities[ i ] != NULL ) {
			entities[ i ]->entityNumber = i;
		}
	}

	savegame.ReadInt( firstFreeIndex );
	savegame.ReadInt( num_entities );

	// enityHash is restored by idEntity::Restore setting the entity name.

	savegame.ReadObject( reinterpret_cast<idClass *&>( world ) );

	savegame.ReadInt( num );
	for( i = 0; i < num; i++ ) {
		savegame.ReadObject( reinterpret_cast<idClass *&>( ent ) );
		assert( ent );
		if ( ent ) {
			ent->spawnNode.AddToEnd( spawnedEntities );
		}
	}

	savegame.ReadInt( num );
	for( i = 0; i < num; i++ ) {
		savegame.ReadObject( reinterpret_cast<idClass *&>( ent ) );
		assert( ent );
		if ( ent ) {
			ent->activeNode.AddToEnd( activeEntities );
		}
	}

	savegame.ReadInt( numEntitiesToDeactivate );
	savegame.ReadBool( sortPushers );
	savegame.ReadBool( sortTeamMasters );
	savegame.ReadDict( &persistentLevelInfo );

	for( i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ ) {
		savegame.ReadFloat( globalShaderParms[ i ] );
	}

	savegame.ReadInt( i );
	random.SetSeed( i );

	savegame.ReadObject( reinterpret_cast<idClass *&>( frameCommandThread ) );

	// clip
	// push
	// pvs

	// testmodel = "<NULL>"
	// testFx = "<NULL>"

	savegame.ReadString( sessionCommand );

	// FIXME: save smoke particles

	savegame.ReadInt( cinematicSkipTime );
	savegame.ReadInt( cinematicStopTime );
	savegame.ReadInt( cinematicMaxSkipTime );
	savegame.ReadBool( inCinematic );
	savegame.ReadBool( skipCinematic );

	savegame.ReadBool( isMultiplayer );
	savegame.ReadInt( (int &)gameType );

	savegame.ReadInt( framenum );
	savegame.ReadInt( previousTime );
	savegame.ReadInt( time );

#ifdef _D3XP
	savegame.ReadInt( msec );
#endif

	savegame.ReadInt( vacuumAreaNum );

	savegame.ReadInt( entityDefBits );
	savegame.ReadBool( isServer );
	savegame.ReadBool( isClient );

	savegame.ReadInt( localClientNum );

	// snapshotEntities is used for multiplayer only

	savegame.ReadInt( realClientTime );
	savegame.ReadBool( isNewFrame );
	savegame.ReadFloat( clientSmoothing );

#ifdef _D3XP
	portalSkyEnt.Restore( &savegame );
	savegame.ReadBool( portalSkyActive );

	fast.Restore( &savegame );
	slow.Restore( &savegame );

	int blah;
	savegame.ReadInt( blah );
	slowmoState = (slowmoState_t)blah;

	savegame.ReadFloat( slowmoMsec );
	savegame.ReadBool( quickSlowmoReset );

	if ( slowmoState == SLOWMO_STATE_OFF ) {
		if ( gameSoundWorld ) {
			gameSoundWorld->SetSlowmo( false );
		}
	}
	else {
		if ( gameSoundWorld ) {
			gameSoundWorld->SetSlowmo( true );
		}
	}
	if ( gameSoundWorld ) {
		gameSoundWorld->SetSlowmoSpeed( slowmoMsec / (float)USERCMD_MSEC );
	}
#endif

	savegame.ReadBool( mapCycleLoaded );
	savegame.ReadInt( spawnCount );

	savegame.ReadInt( num );
	if ( num ) {
		if ( num != gameRenderWorld->NumAreas() ) {
			savegame.Error( "idGameLocal::InitFromSaveGame: number of areas in map differs from save game." );
		}

		locationEntities = new idLocationEntity *[ num ];
		for( i = 0; i < num; i++ ) {
			savegame.ReadObject( reinterpret_cast<idClass *&>( locationEntities[ i ] ) );
		}
	}

	savegame.ReadObject( reinterpret_cast<idClass *&>( camera ) );

	savegame.ReadMaterial( globalMaterial );

	lastAIAlertEntity.Restore( &savegame );
	savegame.ReadInt( lastAIAlertTime );

	savegame.ReadDict( &spawnArgs );

	savegame.ReadInt( playerPVS.i );
	savegame.ReadInt( (int &)playerPVS.h );
	savegame.ReadInt( playerConnectedAreas.i );
	savegame.ReadInt( (int &)playerConnectedAreas.h );

	savegame.ReadVec3( gravity );

	// gamestate is restored after restoring everything else

	savegame.ReadBool( influenceActive );
	savegame.ReadInt( nextGibTime );

	// spawnSpots
	// initialSpots
	// currentInitialSpot
	// newInfo
	// makingBuild
	// shakeSounds

	// Read out pending events
	idEvent::Restore( &savegame );

	savegame.RestoreObjects();

	mpGame.Reset();

	mpGame.Precache();

	// free up any unused animations
	animationLib.FlushUnusedAnims();

	gamestate = GAMESTATE_ACTIVE;

	Printf( "--------------------------------------\n" );

	return true;
}

/*
===========
idGameLocal::MapClear
===========
*/
void idGameLocal::MapClear( bool clearClients ) {
	int i;

	for( i = ( clearClients ? 0 : MAX_CLIENTS ); i < MAX_GENTITIES; i++ ) {
		delete entities[ i ];
		// ~idEntity is in charge of setting the pointer to NULL
		// it will also clear pending events for this entity
		assert( !entities[ i ] );
		spawnIds[ i ] = -1;
	}

	entityHash.Clear( 1024, MAX_GENTITIES );

	if ( !clearClients ) {
		// add back the hashes of the clients
		for ( i = 0; i < MAX_CLIENTS; i++ ) {
			if ( !entities[ i ] ) {
				continue;
			}
			entityHash.Add( entityHash.GenerateKey( entities[ i ]->name.c_str(), true ), i );
		}
	}

	delete frameCommandThread;
	frameCommandThread = NULL;

	if ( editEntities ) {
		delete editEntities;
		editEntities = NULL;
	}

	delete[] locationEntities;
	locationEntities = NULL;

#ifdef STEAM
	steamGotStickynotes = false;
	workshopLegalOpen = false;
#endif

	//BC reset timescale.
	cvarSystem->SetCVarInteger("timescale", 1);
}

/*
===========
idGameLocal::MapShutdown
============
*/
void idGameLocal::MapShutdown( void ) {
	Printf( "--------- Game Map Shutdown ----------\n" );	
	gamestate = GAMESTATE_SHUTDOWN;

#ifdef STEAM
	//sticky notes
	if (!mapFileName.IsEmpty())
	{
		if (gameLocal.GetLocalPlayer())
		{
			if (gameLocal.GetLocalPlayer()->stickynotePending)
			{
				//sticky notes are queued up.... send them immediately.
				g_SteamLeaderboard->SendStickynotes(gameLocal.GetMapName());
				gameLocal.GetLocalPlayer()->stickynotePending = false;
			}
		}
	}
#endif

	if ( gameRenderWorld ) {
		// clear any debug lines, text, and polygons
		gameRenderWorld->DebugClearLines( 0 );
		gameRenderWorld->DebugClearPolygons( 0 );
	}

	// clear out camera if we're in a cinematic
	if ( inCinematic ) {
		camera = NULL;
		inCinematic = false;
	}

	inCutscene = false;

	MapClear( true );

	// reset the script to the state it was before the map was started
	program.Restart();

	if ( smokeParticles ) {
		smokeParticles->Shutdown();
	}

	pvs.Shutdown();

	clip.Shutdown();
	idClipModel::ClearTraceModelCache();

	ShutdownAsyncNetwork();

	mapFileName.Clear();

	gameRenderWorld = NULL;
	gameSoundWorld = NULL;

	gamestate = GAMESTATE_NOMAP;

	Printf( "--------------------------------------\n" );
}

/*
===================
idGameLocal::DumpOggSounds
===================
*/
void idGameLocal::DumpOggSounds( void ) {
	int i, j, k, size, totalSize;
	idFile *file;
	idStrList oggSounds, weaponSounds;
	const idSoundShader *soundShader;
	const soundShaderParms_t *parms;
	idStr soundName;

	for ( i = 0; i < declManager->GetNumDecls( DECL_SOUND ); i++ ) {
		soundShader = static_cast<const idSoundShader *>(declManager->DeclByIndex( DECL_SOUND, i, false ));
		parms = soundShader->GetParms();

		if ( soundShader->EverReferenced() && soundShader->GetState() != DS_DEFAULTED ) {

			const_cast<idSoundShader *>(soundShader)->EnsureNotPurged();

			for ( j = 0; j < soundShader->GetNumSounds(); j++ ) {
				soundName = soundShader->GetSound( j );
				soundName.BackSlashesToSlashes();

#ifdef _D3XP
				// D3XP :: don't add sounds that are in Doom 3's pak files
				if ( fileSystem->FileIsInPAK( soundName ) ) {
					continue;
				} else {
					// Also check for a pre-ogg'd version in the pak file
					idStr testName = soundName;

					testName.SetFileExtension( ".ogg" );
					if ( fileSystem->FileIsInPAK( testName ) ) {
						continue;
					}
				}
#endif
				// don't OGG sounds that cause a shake because that would
				// cause continuous seeking on the OGG file which is expensive
				if ( parms->shakes != 0.0f ) {
					shakeSounds.AddUnique( soundName );
					continue;
				}

				// if not voice over or combat chatter
				if (	soundName.Find( "/vo/", false ) == -1 &&
						soundName.Find( "/combat_chatter/", false ) == -1 &&
						soundName.Find( "/bfgcarnage/", false ) == -1 &&
						soundName.Find( "/enpro/", false ) == - 1 &&
						soundName.Find( "/soulcube/energize_01.wav", false ) == -1 ) {
					// don't OGG weapon sounds
					if (	soundName.Find( "weapon", false ) != -1 ||
							soundName.Find( "gun", false ) != -1 ||
							soundName.Find( "bullet", false ) != -1 ||
							soundName.Find( "bfg", false ) != -1 ||
							soundName.Find( "plasma", false ) != -1 ) {
						weaponSounds.AddUnique( soundName );
						continue;
					}
				}

				for ( k = 0; k < shakeSounds.Num(); k++ ) {
					if ( shakeSounds[k].IcmpPath( soundName ) == 0 ) {
						break;
					}
				}
				if ( k < shakeSounds.Num() ) {
					continue;
				}

				oggSounds.AddUnique( soundName );
			}
		}
	}

	file = fileSystem->OpenFileWrite( "makeogg.bat", "fs_savepath" );
	if ( file == NULL ) {
		common->Warning( "Couldn't open makeogg.bat" );
		return;
	}

	// list all the shake sounds
	totalSize = 0;
	for ( i = 0; i < shakeSounds.Num(); i++ ) {
		size = fileSystem->ReadFile( shakeSounds[i], NULL, NULL );
		totalSize += size;
		shakeSounds[i].Replace( "/", "\\" );
		file->Printf( "echo \"%s\" (%d kB)\n", shakeSounds[i].c_str(), size >> 10 );
	}
	file->Printf( "echo %d kB in shake sounds\n\n\n", totalSize >> 10 );

	// list all the weapon sounds
	totalSize = 0;
	for ( i = 0; i < weaponSounds.Num(); i++ ) {
		size = fileSystem->ReadFile( weaponSounds[i], NULL, NULL );
		totalSize += size;
		weaponSounds[i].Replace( "/", "\\" );
		file->Printf( "echo \"%s\" (%d kB)\n", weaponSounds[i].c_str(), size >> 10 );
	}
	file->Printf( "echo %d kB in weapon sounds\n\n\n", totalSize >> 10 );

	// list commands to convert all other sounds to ogg
	totalSize = 0;
	for ( i = 0; i < oggSounds.Num(); i++ ) {
		size = fileSystem->ReadFile( oggSounds[i], NULL, NULL );
		totalSize += size;
		oggSounds[i].Replace( "/", "\\" );
		file->Printf( "z:\\d3xp\\ogg\\oggenc -q 0 \"%s\\d3xp\\%s\"\n", cvarSystem->GetCVarString( "fs_basepath" ), oggSounds[i].c_str() );
		file->Printf( "del \"%s\\d3xp\\%s\"\n", cvarSystem->GetCVarString( "fs_basepath" ), oggSounds[i].c_str() );
	}
	file->Printf( "\n\necho %d kB in OGG sounds\n\n\n", totalSize >> 10 );

	fileSystem->CloseFile( file );

	shakeSounds.Clear();
}

/*
===================
idGameLocal::GetShakeSounds
===================
*/
void idGameLocal::GetShakeSounds( const idDict *dict ) {
	const idSoundShader *soundShader;
	const char *soundShaderName;
	idStr soundName;

	soundShaderName = dict->GetString( "s_shader" );
	if ( *soundShaderName != '\0' && dict->GetFloat( "s_shakes" ) != 0.0f ) {
		soundShader = declManager->FindSound( soundShaderName );

		for ( int i = 0; i < soundShader->GetNumSounds(); i++ ) {
			soundName = soundShader->GetSound( i );
			soundName.BackSlashesToSlashes();

			shakeSounds.AddUnique( soundName );
		}
	}
}

/*
===================
idGameLocal::CacheDictionaryMedia

This is called after parsing an EntityDef and for each entity spawnArgs before
merging the entitydef.  It could be done post-merge, but that would
avoid the fast pre-cache check associated with each entityDef
===================
*/
void idGameLocal::CacheDictionaryMedia( const idDict *dict ) {
	const idKeyValue *kv;

	if ( dict == NULL ) {
		if ( cvarSystem->GetCVarBool( "com_makingBuild") ) {
			DumpOggSounds();
		}
		return;
	}

	if ( cvarSystem->GetCVarBool( "com_makingBuild" ) ) {
		GetShakeSounds( dict );
	}

	kv = dict->MatchPrefix( "model" );
	while( kv ) {
		if ( kv->GetValue().Length() ) {
			declManager->MediaPrint( "Precaching model %s\n", kv->GetValue().c_str() );
			// precache model/animations
			if ( declManager->FindType( DECL_MODELDEF, kv->GetValue(), false ) == NULL ) {
				// precache the render model
				renderModelManager->FindModel( kv->GetValue() );
				// precache .cm files only
				collisionModelManager->LoadModel( kv->GetValue(), true );
			}
		}
		kv = dict->MatchPrefix( "model", kv );
	}

	kv = dict->FindKey( "s_shader" );
	if ( kv && kv->GetValue().Length() ) {
		declManager->FindType( DECL_SOUND, kv->GetValue() );
	}

	kv = dict->MatchPrefix( "snd", NULL );
	while( kv ) {
		if ( kv->GetValue().Length() ) {
			declManager->FindType( DECL_SOUND, kv->GetValue() );
		}
		kv = dict->MatchPrefix( "snd", kv );
	}


	kv = dict->MatchPrefix( "gui", NULL );
	while( kv ) {
		if ( kv->GetValue().Length() ) {
			if ( !idStr::Icmp( kv->GetKey(), "gui_noninteractive" )
				|| !idStr::Icmpn( kv->GetKey(), "gui_parm", 8 )	
				|| !idStr::Icmp( kv->GetKey(), "gui_inventory" ) ) {
				// unfortunate flag names, they aren't actually a gui
			} else {
				declManager->MediaPrint( "Precaching gui %s\n", kv->GetValue().c_str() );
				idUserInterface *gui = uiManager->Alloc();
				if ( gui ) {
					gui->InitFromFile( kv->GetValue() );
					uiManager->DeAlloc( gui );
				}
			}
		}
		kv = dict->MatchPrefix( "gui", kv );
	}

	kv = dict->FindKey( "texture" );
	if ( kv && kv->GetValue().Length() ) {
		declManager->FindType( DECL_MATERIAL, kv->GetValue() );
	}

	kv = dict->MatchPrefix( "mtr", NULL );
	while( kv ) {
		if ( kv->GetValue().Length() ) {
			declManager->FindType( DECL_MATERIAL, kv->GetValue() );
		}
		kv = dict->MatchPrefix( "mtr", kv );
	}

	// handles hud icons
	kv = dict->MatchPrefix( "inv_icon", NULL );
	while ( kv ) {
		if ( kv->GetValue().Length() ) {
			declManager->FindType( DECL_MATERIAL, kv->GetValue() );
		}
		kv = dict->MatchPrefix( "inv_icon", kv );
	}

	// handles teleport fx.. this is not ideal but the actual decision on which fx to use
	// is handled by script code based on the teleport number
	kv = dict->MatchPrefix( "teleport", NULL );
	if ( kv && kv->GetValue().Length() ) {
		int teleportType = atoi( kv->GetValue() );
		const char *p = ( teleportType ) ? va( "fx/teleporter%i.fx", teleportType ) : "fx/teleporter.fx";
		declManager->FindType( DECL_FX, p );
	}

	kv = dict->MatchPrefix( "fx", NULL );
	while( kv ) {
		if ( kv->GetValue().Length() ) {
			declManager->MediaPrint( "Precaching fx %s\n", kv->GetValue().c_str() );
			declManager->FindType( DECL_FX, kv->GetValue() );
		}
		kv = dict->MatchPrefix( "fx", kv );
	}

	kv = dict->MatchPrefix( "smoke", NULL );
	while( kv ) {
		if ( kv->GetValue().Length() ) {
			idStr prtName = kv->GetValue();
			int dash = prtName.Find('-');
			if ( dash > 0 ) {
				prtName = prtName.Left( dash );
			}
			declManager->FindType( DECL_PARTICLE, prtName );
		}
		kv = dict->MatchPrefix( "smoke", kv );
	}

	kv = dict->MatchPrefix( "skin", NULL );
	while( kv ) {
		if ( kv->GetValue().Length() ) {
			declManager->MediaPrint( "Precaching skin %s\n", kv->GetValue().c_str() );
			declManager->FindType( DECL_SKIN, kv->GetValue() );
		}
		kv = dict->MatchPrefix( "skin", kv );
	}

	kv = dict->MatchPrefix( "def", NULL );
	while( kv ) {
		if ( kv->GetValue().Length() ) {
			FindEntityDef( kv->GetValue().c_str(), false );
		}
		kv = dict->MatchPrefix( "def", kv );
	}

	kv = dict->MatchPrefix( "pda_name", NULL );
	while( kv ) {
		if ( kv->GetValue().Length() ) {
			declManager->FindType( DECL_PDA, kv->GetValue().c_str(), false );
		}
		kv = dict->MatchPrefix( "pda_name", kv );
	}

	kv = dict->MatchPrefix( "video", NULL );
	while( kv ) {
		if ( kv->GetValue().Length() ) {
			declManager->FindType( DECL_VIDEO, kv->GetValue().c_str(), false );
		}
		kv = dict->MatchPrefix( "video", kv );
	}

	kv = dict->MatchPrefix( "audio", NULL );
	while( kv ) {
		if ( kv->GetValue().Length() ) {
			declManager->FindType( DECL_AUDIO, kv->GetValue().c_str(), false );
		}
		kv = dict->MatchPrefix( "audio", kv );
	}
}

/*
===========
idGameLocal::InitScriptForMap
============
*/
void idGameLocal::InitScriptForMap( void ) {
	// create a thread to run frame commands on
	frameCommandThread = new idThread();
	frameCommandThread->ManualDelete();
	frameCommandThread->SetThreadName( "frameCommands" );

	// run the main game script function (not the level specific main)
	const function_t *func = program.FindFunction( SCRIPT_DEFAULTFUNC );
	if ( func != NULL ) {
		idThread *thread = new idThread( func );
		if ( thread->Start() ) {
			// thread has finished executing, so delete it
			delete thread;
		}
	}
}

/*
===========
idGameLocal::SpawnPlayer
============
*/
void idGameLocal::SpawnPlayer( int clientNum ) {
	idEntity	*ent;
	idDict		args;

	// they can connect
	Printf( "SpawnPlayer: %i\n", clientNum );

	args.SetInt( "spawn_entnum", clientNum );
	args.Set( "name", va( "player%d", clientNum + 1 ) );
#ifdef CTF
	if ( isMultiplayer && gameType != GAME_CTF )
		args.Set( "classname", "player_doommarine_mp" );
	else if ( isMultiplayer && gameType == GAME_CTF )
		args.Set( "classname", "player_doommarine_ctf" );
	else
		args.Set( "classname", "player_doommarine" );
#else
	args.Set( "classname", isMultiplayer ? "player_doommarine_mp" : "player_doommarine" );
#endif
	if ( !SpawnEntityDef( args, &ent ) || !entities[ clientNum ] ) {
		Error( "Failed to spawn player as '%s'", args.GetString( "classname" ) );
	}

	// make sure it's a compatible class
	if ( !ent->IsType( idPlayer::Type ) ) {
		Error( "'%s' spawned the player as a '%s'.  Player spawnclass must be a subclass of idPlayer.", args.GetString( "classname" ), ent->GetClassname() );
	}

	if ( clientNum >= numClients ) {
		numClients = clientNum + 1;
	}

	mpGame.SpawnPlayer( clientNum );
}

/*
================
idGameLocal::GetClientByNum
================
*/
idPlayer *idGameLocal::GetClientByNum( int current ) const {
	if ( current < 0 || current >= numClients ) {
		current = 0;
	}
	if ( entities[current] ) {
		return static_cast<idPlayer *>( entities[ current ] );
	}
	return NULL;
}

/*
================
idGameLocal::GetClientByName
================
*/
idPlayer *idGameLocal::GetClientByName( const char *name ) const {
	int i;
	idEntity *ent;
	for ( i = 0 ; i < numClients ; i++ ) {
		ent = entities[ i ];
		if ( ent && ent->IsType( idPlayer::Type ) ) {
			if ( idStr::IcmpNoColor( name, userInfo[ i ].GetString( "ui_name" ) ) == 0 ) {
				return static_cast<idPlayer *>( ent );
			}
		}
	}
	return NULL;
}

/*
================
idGameLocal::GetClientByCmdArgs
================
*/
idPlayer *idGameLocal::GetClientByCmdArgs( const idCmdArgs &args ) const {
	idPlayer *player;
	idStr client = args.Argv( 1 );
	if ( !client.Length() ) {
		return NULL;
	}
	// we don't allow numeric ui_name so this can't go wrong
	if ( client.IsNumeric() ) {
		player = GetClientByNum( atoi( client.c_str() ) );
	} else {
		player = GetClientByName( client.c_str() );
	}
	if ( !player ) {
		common->Printf( "Player '%s' not found\n", client.c_str() );
	}
	return player;
}

/*
================
idGameLocal::GetNextClientNum
================
*/
int idGameLocal::GetNextClientNum( int _current ) const {
	int i, current;

	current = 0;
	for ( i = 0; i < numClients; i++) {
		current = ( _current + i + 1 ) % numClients;
		if ( entities[ current ] && entities[ current ]->IsType( idPlayer::Type ) ) {
			return current;
		}
	}

	return current;
}

/*
================
idGameLocal::GetLocalPlayer

Nothing in the game tic should EVER make a decision based on what the
local client number is, it shouldn't even be aware that there is a
draw phase even happening.  This just returns client 0, which will
be correct for single player.
================
*/
idPlayer *idGameLocal::GetLocalPlayer() const {
	if ( localClientNum < 0 ) {
		return NULL;
	}

	if ( !entities[ localClientNum ] || !entities[ localClientNum ]->IsType( idPlayer::Type ) ) {
		// not fully in game yet
		return NULL;
	}
	return static_cast<idPlayer *>( entities[ localClientNum ] );
}

/*
================
idGameLocal::SetupClientPVS
================
*/
pvsHandle_t idGameLocal::GetClientPVS( idPlayer *player, pvsType_t type ) {
	if ( player->GetPrivateCameraView() ) {
		return pvs.SetupCurrentPVS( player->GetPrivateCameraView()->GetPVSAreas(), player->GetPrivateCameraView()->GetNumPVSAreas() );
	} else if ( camera ) {
		return pvs.SetupCurrentPVS( camera->GetPVSAreas(), camera->GetNumPVSAreas() );
	} else {
		return pvs.SetupCurrentPVS( player->GetPVSAreas(), player->GetNumPVSAreas() );
	}
}

/*
================
idGameLocal::SetupPlayerPVS
================
*/
void idGameLocal::SetupPlayerPVS( void ) {
	int			i;
	idEntity *	ent;
	idPlayer *	player;
	pvsHandle_t	otherPVS, newPVS;

	playerPVS.i = -1;
	for ( i = 0; i < numClients; i++ ) {
		ent = entities[i];
		if ( !ent || !ent->IsType( idPlayer::Type ) ) {
			continue;
		}

		player = static_cast<idPlayer *>(ent);

		if ( playerPVS.i == -1 ) {
			playerPVS = GetClientPVS( player, PVS_NORMAL );
		} else {
			otherPVS = GetClientPVS( player, PVS_NORMAL );
			newPVS = pvs.MergeCurrentPVS( playerPVS, otherPVS );
			pvs.FreeCurrentPVS( playerPVS );
			pvs.FreeCurrentPVS( otherPVS );
			playerPVS = newPVS;
		}

		if ( playerConnectedAreas.i == -1 ) {
			playerConnectedAreas = GetClientPVS( player, PVS_CONNECTED_AREAS );
		} else {
			otherPVS = GetClientPVS( player, PVS_CONNECTED_AREAS );
			newPVS = pvs.MergeCurrentPVS( playerConnectedAreas, otherPVS );
			pvs.FreeCurrentPVS( playerConnectedAreas );
			pvs.FreeCurrentPVS( otherPVS );
			playerConnectedAreas = newPVS;
		}

#ifdef _D3XP
		// if portalSky is preset, then merge into pvs so we get rotating brushes, etc
		if ( portalSkyEnt.GetEntity() ) {
			idEntity *skyEnt = portalSkyEnt.GetEntity();

			otherPVS = pvs.SetupCurrentPVS( skyEnt->GetPVSAreas(), skyEnt->GetNumPVSAreas() );
			newPVS = pvs.MergeCurrentPVS( playerPVS, otherPVS );
			pvs.FreeCurrentPVS( playerPVS );
			pvs.FreeCurrentPVS( otherPVS );
			playerPVS = newPVS;

			otherPVS = pvs.SetupCurrentPVS( skyEnt->GetPVSAreas(), skyEnt->GetNumPVSAreas() );
			newPVS = pvs.MergeCurrentPVS( playerConnectedAreas, otherPVS );
			pvs.FreeCurrentPVS( playerConnectedAreas );
			pvs.FreeCurrentPVS( otherPVS );
			playerConnectedAreas = newPVS;
		}
#endif
	}
}

/*
================
idGameLocal::FreePlayerPVS
================
*/
void idGameLocal::FreePlayerPVS( void ) {
	if ( playerPVS.i != -1 ) {
		pvs.FreeCurrentPVS( playerPVS );
		playerPVS.i = -1;
	}
	if ( playerConnectedAreas.i != -1 ) {
		pvs.FreeCurrentPVS( playerConnectedAreas );
		playerConnectedAreas.i = -1;
	}
}

/*
================
idGameLocal::InPlayerPVS

  should only be called during entity thinking and event handling
================
*/
bool idGameLocal::InPlayerPVS( idEntity *ent ) const {
	if ( playerPVS.i == -1 ) {
		return false;
	}
    return pvs.InCurrentPVS( playerPVS, ent->GetPVSAreas(), ent->GetNumPVSAreas() );
}

/*
================
idGameLocal::InPlayerConnectedArea

  should only be called during entity thinking and event handling
================
*/
bool idGameLocal::InPlayerConnectedArea( idEntity *ent ) const {
	if ( playerConnectedAreas.i == -1 ) {
		return false;
	}
    return pvs.InCurrentPVS( playerConnectedAreas, ent->GetPVSAreas(), ent->GetNumPVSAreas() );
}

/*
================
idGameLocal::UpdateGravity
================
*/
void idGameLocal::UpdateGravity( void ) {
	idEntity *ent;

	if ( g_gravity.IsModified() ) {
		if ( g_gravity.GetFloat() == 0.0f ) {
			g_gravity.SetFloat( 1.0f );
		}
        gravity.Set( 0, 0, -g_gravity.GetFloat() );

		// update all physics objects
		for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
			if ( ent->IsType( idAFEntity_Generic::Type ) ) {
				idPhysics *phys = ent->GetPhysics();
				if ( phys ) {
					phys->SetGravity( gravity );
				}
			}
		}
		g_gravity.ClearModified();
	}
}

/*
================
idGameLocal::GetGravity
================
*/
const idVec3 &idGameLocal::GetGravity( void ) const {
	return gravity;
}

/*
================
idGameLocal::SortActiveEntityList

  Sorts the active entity list such that pushing entities come first,
  actors come next and physics team slaves appear after their master.
================
*/
void idGameLocal::SortActiveEntityList( void ) {
	idEntity *ent, *next_ent, *master, *part;

	// if the active entity list needs to be reordered to place physics team masters at the front
	if ( sortTeamMasters ) {
		for ( ent = activeEntities.Next(); ent != NULL; ent = next_ent ) {
			next_ent = ent->activeNode.Next();
			master = ent->GetTeamMaster();
			if ( master && master == ent ) {
				ent->activeNode.Remove();
				ent->activeNode.AddToFront( activeEntities );
			}
		}
	}

	// if the active entity list needs to be reordered to place pushers at the front
	if ( sortPushers ) {

		for ( ent = activeEntities.Next(); ent != NULL; ent = next_ent ) {
			next_ent = ent->activeNode.Next();
			master = ent->GetTeamMaster();
			if ( !master || master == ent ) {
				// check if there is an actor on the team
				for ( part = ent; part != NULL; part = part->GetNextTeamEntity() ) {
					if ( part->GetPhysics()->IsType( idPhysics_Actor::Type ) ) {
						break;
					}
				}
				// if there is an actor on the team
				if ( part ) {
					ent->activeNode.Remove();
					ent->activeNode.AddToFront( activeEntities );
				}
			}
		}

		for ( ent = activeEntities.Next(); ent != NULL; ent = next_ent ) {
			next_ent = ent->activeNode.Next();
			master = ent->GetTeamMaster();
			if ( !master || master == ent ) {
				// check if there is an entity on the team using parametric physics
				for ( part = ent; part != NULL; part = part->GetNextTeamEntity() ) {
					if ( part->GetPhysics()->IsType( idPhysics_Parametric::Type ) ) {
						break;
					}
				}
				// if there is an entity on the team using parametric physics
				if ( part ) {
					ent->activeNode.Remove();
					ent->activeNode.AddToFront( activeEntities );
				}
			}
		}
	}

	sortTeamMasters = false;
	sortPushers = false;
}

#ifdef _D3XP
/*
================
idGameLocal::RunTimeGroup2
================
*/
void idGameLocal::RunTimeGroup2() {
	idEntity *ent;
	int num = 0;

	fast.Increment();
	fast.Get( time, previousTime, msec, framenum, realClientTime );

	for( ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() ) {
		if ( ent->timeGroup != TIME_GROUP2 ) {
			continue;
		}

		ent->Think();
		num++;
	}

	slow.Get( time, previousTime, msec, framenum, realClientTime );
}
#endif
 
/*
================
idGameLocal::RunFrame
================
*/
gameReturn_t idGameLocal::RunFrame( const usercmd_t *clientCmds ) {
	idEntity *	ent;
	int			num;
	float		ms;
	idTimer		timer_think, timer_events, timer_singlethink;
	gameReturn_t ret;
	idPlayer	*player;
	const renderView_t *view;

#ifdef _DEBUG
	if ( isMultiplayer ) {
		assert( !isClient );
	}
#endif

	player = GetLocalPlayer();

#ifdef _D3XP
	ComputeSlowMsec();

	slow.Get( time, previousTime, msec, framenum, realClientTime );
	msec = slowmoMsec;
#endif

	if ( !isMultiplayer && g_stopTime.GetBool() ) {
		// clear any debug lines from a previous frame
		gameRenderWorld->DebugClearLines( time + 1 );

		// set the user commands for this frame
		memcpy( usercmds, clientCmds, numClients * sizeof( usercmds[ 0 ] ) );

		if ( player ) {
			player->Think();
		}
	} else do {
		// update the game time
		framenum++;
		previousTime = time;
		time += msec;
		realClientTime = time;

#ifdef _D3XP
		slow.Set( time, previousTime, msec, framenum, realClientTime );
#endif

#ifdef GAME_DLL
		// allow changing SIMD usage on the fly
		if ( com_forceGenericSIMD.IsModified() ) {
			idSIMD::InitProcessor( "game", com_forceGenericSIMD.GetBool() );
		}
#endif

		// make sure the random number counter is used each frame so random events
		// are influenced by the player's actions
		random.RandomInt();

		if ( player ) {
			// update the renderview so that any gui videos play from the right frame
			view = player->GetRenderView();
			if ( view ) {
				gameRenderWorld->SetRenderView( view );
			}
		}

		// clear any debug lines from a previous frame
		gameRenderWorld->DebugClearLines( time );

		// clear any debug polygons from a previous frame
		gameRenderWorld->DebugClearPolygons( time );

		// set the user commands for this frame
		memcpy( usercmds, clientCmds, numClients * sizeof( usercmds[ 0 ] ) );

		// free old smoke particles
		smokeParticles->FreeSmokes();

		// process events on the server
		ServerProcessEntityNetworkEventQueue();

		// update our gravity vector if needed.
		UpdateGravity();

		// create a merged pvs for all players
		SetupPlayerPVS();

		// sort the active entity list
		SortActiveEntityList();

		timer_think.Clear();
		timer_think.Start();

		// let entities think
		if ( g_timeentities.GetFloat() ) {
			num = 0;
			for( ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() ) {
				if ( g_cinematic.GetBool() && inCinematic && !ent->cinematic ) {
					ent->GetPhysics()->UpdateTime( time );
					continue;
				}
				timer_singlethink.Clear();
				timer_singlethink.Start();
				ent->Think();
				timer_singlethink.Stop();
				ms = timer_singlethink.Milliseconds();
				if ( ms >= g_timeentities.GetFloat() ) {
					Printf( "%d: entity '%s': %.1f ms\n", time, ent->name.c_str(), ms );
				}
				num++;
			}
		} else {
			if ( inCinematic ) {
				num = 0;
				for( ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() ) {
					if ( g_cinematic.GetBool() && !ent->cinematic ) {
						ent->GetPhysics()->UpdateTime( time );
						continue;
					}
					ent->Think();
					num++;
				}
			} else {
				num = 0;
				for( ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() ) {
#ifdef _D3XP
					if ( ent->timeGroup != TIME_GROUP1 ) {
						continue;
					}
#endif
					ent->Think();
					num++;
				}
			}
		}

#ifdef _D3XP
		RunTimeGroup2();
#endif

		// remove any entities that have stopped thinking
		if ( numEntitiesToDeactivate ) {
			idEntity *next_ent;
			int c = 0;
			for( ent = activeEntities.Next(); ent != NULL; ent = next_ent ) {
				next_ent = ent->activeNode.Next();
				if ( !ent->thinkFlags ) {
					ent->activeNode.Remove();
					c++;
				}
			}
			//assert( numEntitiesToDeactivate == c );
			numEntitiesToDeactivate = 0;
		}

		timer_think.Stop();
		timer_events.Clear();
		timer_events.Start();

		// service any pending events
		idEvent::ServiceEvents();

#ifdef _D3XP
		// service pending fast events
		fast.Get( time, previousTime, msec, framenum, realClientTime );
		idEvent::ServiceFastEvents();
		slow.Get( time, previousTime, msec, framenum, realClientTime );
#endif

		timer_events.Stop();

		// free the player pvs
		FreePlayerPVS();

		// do multiplayer related stuff
		if ( isMultiplayer ) {
			mpGame.Run();
		}

		// display how long it took to calculate the current game frame
		if ( g_frametime.GetBool() ) {
			Printf( "game %d: all:%.1f th:%.1f ev:%.1f %d ents \n",
				time, timer_think.Milliseconds() + timer_events.Milliseconds(),
				timer_think.Milliseconds(), timer_events.Milliseconds(), num );
		}

		// build the return value
		ret.consistencyHash = 0;
		ret.sessionCommand[0] = 0;

		if ( !isMultiplayer && player ) {
			ret.health = player->health;
			ret.heartRate = player->heartRate;
			ret.stamina = idMath::FtoiFast( player->stamina );
			// combat is a 0-100 value based on lastHitTime and lastDmgTime
			// each make up 50% of the time spread over 10 seconds
			ret.combat = 0;
			if ( player->lastDmgTime > 0 && time < player->lastDmgTime + 10000 ) {
				ret.combat += 50.0f * (float) ( time - player->lastDmgTime ) / 10000;
			}
			if ( player->lastHitTime > 0 && time < player->lastHitTime + 10000 ) {
				ret.combat += 50.0f * (float) ( time - player->lastHitTime ) / 10000;
			}
		}

		// see if a target_sessionCommand has forced a changelevel
		if ( sessionCommand.Length() ) {
			strncpy( ret.sessionCommand, sessionCommand, sizeof( ret.sessionCommand ) );
			break;
		}

		// make sure we don't loop forever when skipping a cinematic
		if ( skipCinematic && ( time > cinematicMaxSkipTime ) ) {
			Warning( "Exceeded maximum cinematic skip length.  Cinematic may be looping infinitely." );
			skipCinematic = false;
			break;
		}

#ifdef STEAM
	//CALLED ONCE PER MAP LOAD.
	if (steamInitialized && !steamGotStickynotes && time > 100)
	{
		//get stats.
		g_SteamStats->RequestGlobalStats();

		//get sticky notes.
		//common->Printf("zzz steam getting...\n");
		steamGotStickynotes = true;
		//g_SteamRemote->GetAllUserFiles();


		g_SteamLeaderboard->GetStickynotes(gameLocal.GetMapName());

		g_SteamLeaderboard->Initialize(gameLocal.GetMapName(), 3);
	}
#endif






	} while( ( inCinematic || ( time < cinematicStopTime ) ) && skipCinematic );

	ret.syncNextGameFrame = skipCinematic;
	if ( skipCinematic ) {
		soundSystem->SetMute( false );
		skipCinematic = false;		
	}

	// show any debug info for this frame
	RunDebugInfo();
	D_DrawDebugLines();

	return ret;
}


/*
======================================================================

  Game view drawing

======================================================================
*/

/*
====================
idGameLocal::CalcFov

Calculates the horizontal and vertical field of view based on a horizontal field of view and custom aspect ratio
====================
*/
void idGameLocal::CalcFov( float base_fov, float &fov_x, float &fov_y ) const {
	float	x;
	float	y;
	float	ratio_x;
	float	ratio_y;

	// first, calculate the vertical fov based on a 640x480 view
	x = 640.0f / tan( base_fov / 360.0f * idMath::PI );
	y = atan2( 480.0f, x );
	fov_y = y * 360.0f / idMath::PI;

	// FIXME: somehow, this is happening occasionally
	assert( fov_y > 0 );
	if ( fov_y <= 0 ) {
		Error( "idGameLocal::CalcFov: bad result, fov_y == %f, base_fov == %f", fov_y, base_fov );
	}

	switch( r_aspectRatio.GetInteger() ) {
	default :
	
	case 0 :
		// 4:3
		fov_x = base_fov;
		return;
		break;

	case 1 :
		// 16:9
		ratio_x = 16.0f;
		ratio_y = 9.0f;
		break;

	case 2 :
		// 16:10
		ratio_x = 16.0f;
		ratio_y = 10.0f;
		break;

	case 3 :
		// 16:10
		ratio_x = 21.0f;
		ratio_y = 9.0f;
		break;
	case 4 :
		// auto mode => use aspect ratio from resolution, assuming screen's pixels are squares
		ratio_x = renderSystem->GetScreenWidth();
		ratio_y = renderSystem->GetScreenHeight();
		if(ratio_x <= 0.0f || ratio_y <= 0.0f)
		{
			// for some reason (maybe this is a dedicated server?) GetScreenWidth()/Height()
			// returned 0. Assume default 4:3 to avoid assert()/Error() below.
			fov_x = base_fov;
			return;
		}
		break;
	}

	y = ratio_y / tan( fov_y / 360.0f * idMath::PI );
	fov_x = atan2( ratio_x, y ) * 360.0f / idMath::PI;

	if ( fov_x < base_fov ) {
		fov_x = base_fov;
		x = ratio_x / tan( fov_x / 360.0f * idMath::PI );
		fov_y = atan2( ratio_y, x ) * 360.0f / idMath::PI;
	}

	// FIXME: somehow, this is happening occasionally
	assert( ( fov_x > 0 ) && ( fov_y > 0 ) );
	if ( ( fov_y <= 0 ) || ( fov_x <= 0 ) ) {
		Error( "idGameLocal::CalcFov: bad result" );
	}
}

/*
================
idGameLocal::Draw

makes rendering and sound system calls
================
*/
bool idGameLocal::Draw( int clientNum ) {
	if ( isMultiplayer ) {
		return mpGame.Draw( clientNum );
	}

	idPlayer *player = static_cast<idPlayer *>(entities[ clientNum ]);

	if ( !player ) {
		return false;
	}

	// render the scene
	player->playerView.RenderPlayerView( player->hud );

	return true;
}

/*
================
idGameLocal::HandleESC
================
*/
escReply_t idGameLocal::HandleESC( idUserInterface **gui ) {
	if ( isMultiplayer ) {
		*gui = StartMenu();
		// we may set the gui back to NULL to hide it
		return ESC_GUI;
	}
	idPlayer *player = GetLocalPlayer();
	if ( player ) {
		if ( player->HandleESC() ) {
			return ESC_IGNORE;
		} else {
			return ESC_MAIN;
		}
	}
	return ESC_MAIN;
}

/*
================
idGameLocal::StartMenu
================
*/
idUserInterface* idGameLocal::StartMenu( void ) {
	
	//BC redirect the menu system to use this thing.
	if (GetLocalPlayer()->deckGui != NULL)
		return GetLocalPlayer()->deckGui;
	
	/*if ( !isMultiplayer )
	{
		return NULL;
	}*/

	return mpGame.StartMenu();
}

void trim(char * s) {
    char * p = s;
    int l = strlen(p);

    while(isspace(p[l - 1])) p[--l] = 0;
    while(* p && isspace(* p)) ++p, --l;

    memmove(s, p, l + 1);
}

const char* idGameLocal::ParseTimeMS(int value)
{
	float ret = value / 1000.0f;
	return ParseTime(ret);
}

const char* idGameLocal::ParseTime(float value)
{
	//value = total time, in seconds.
	int minutes = value / 60.0f;
	float seconds = value - (minutes * 60);
	int deciseconds = (seconds - (int)seconds) * 100.0f;
	
	return (va("%d:%02d.%02d", minutes, (int)(seconds), deciseconds ));
}

void idGameLocal::KillDeckThreads()
{
	//BC end whatever deck threads are running.
	idThread::KillThread("deckthread1");
}

void idGameLocal::KillCCTVs()
{
	idEntity *cctv = gameLocal.FindEntityUsingDef(NULL, "moveable_item_cctv");

	if (!cctv)
		return;

	const char *screen1name = cctv->spawnArgs.GetString("screen1_name");
	const char *screen2name = cctv->spawnArgs.GetString("screen2_name");

	idEntity *screen1 = gameLocal.FindEntity(screen1name);
	idEntity *screen2 = gameLocal.FindEntity(screen2name);
	
	if (screen1)
	{
		//screen1->Event_SetGui(1, "guis/video1.gui");
		screen1->Event_ClearCameraTarget();
		screen1->UpdateVisuals();
		screen1->Present();		
	}

	if (screen2)
	{
		//screen2->Event_SetGui(1, "guis/video2.gui");
		screen2->Event_ClearCameraTarget();
		screen2->UpdateVisuals();
		screen2->Present();		
	}

	cctv->Event_SetGui(1, "guis/video1.gui");
	cctv->Event_SetGui(2, "guis/video2.gui");

	//cctv->Event_SetGui(1, "guis/camera.gui");
	//cctv->Event_SetGui(2, "guis/camera2.gui");

	cctv->UpdateVisuals();
	cctv->Present();
}

void idGameLocal::CallMap( const char *funcname )
{
	idStr				callfunc;
	const function_t	*func;
	idThread			*thread;

	callfunc = gameLocal.GetMapName();

	//strip out the maps/ and .map
	callfunc.StripPath();
	callfunc.StripFileExtension();

	callfunc += "::";
	callfunc += funcname;

	if ( callfunc.Length() )
	{
		func = gameLocal.program.FindFunction( callfunc );
		
		if ( func != NULL )
		{
			thread = new idThread( func );
			thread->DelayedStart( 0 );

			idThread::ReturnInt(1);
			return;
		}
		else
		{
			//common->Warning("unable to find callmap function %s\n", funcname);
		}
	}
	else
	{
		idThread::ReturnInt(0);
		return;
	}

	callfunc = "map_" + callfunc;

	if ( callfunc.Length() )
	{
		func = gameLocal.program.FindFunction( callfunc );
		
		if ( func != NULL )
		{
			thread = new idThread( func );
			thread->DelayedStart( 0 );

			idThread::ReturnInt(1);
			return;
		}
		else
		{
			//common->Warning("unable to find callmap function %s\n", funcname);
		}
	}

	idThread::ReturnInt(0);
}

bool idGameLocal::RunDeckCommand( const char *rawCommand )
{
	idStr cleanCommand = rawCommand;

	cleanCommand.StripTrailingWhitespace(); //remove whitespace.
	cleanCommand.StripLeading( ' ' );		//remove whitespace.
	cleanCommand.ToLower();					//force lower case.

	rawCommand = cleanCommand.c_str();

	if ( rawCommand && rawCommand[0] )
	{
		//local echo.
		//GetLocalPlayer()->deckList->Push( va("> %s", rawCommand));
		const function_t *deckCall;
		deckCall = GetLocalPlayer()->deckScript.GetEntity()->scriptObject.GetFunction( "parse" );

		if ( deckCall )
		{
			// start a thread that will run immediately and be destroyed
			idThread * thread = new idThread( deckCall );
			thread->SetThreadName( "deckthread1" );
			thread->CallFunctionArgs(deckCall, true, "es", GetLocalPlayer()->deckScript.GetEntity(), rawCommand);
			thread->Execute();
			//delete thread; //comment out if you want to allow sys.wait script calls
		}
		else
		{
			common->Warning("Error: unable to find deckscript!");
		}

		//synthesize the entered command to base/sound/flite.wav, then play it. (Brian)
		//comment this out to remove speech synthesis entirely.

		/* bc flite
		soundSystem->SynthesizeSpeech( rawCommand );

		int length;
		GetLocalPlayer()->StartSoundShader( declManager->FindSound( "flite" ), SCHANNEL_ANY, 0, false, &length );*/

		return true;
	}
	else
	{
		
		return false;
	}
}

/*
================
idGameLocal::HandleGuiCommands
================
*/
const char* idGameLocal::HandleGuiCommands( const char *menuCommand ) 
{
	int				icmd;
	idCmdArgs		args;

	//BC player has entered a deck command & pressed ENTER.
	if ( !menuCommand[ 0 ] )
	{
		return "continue";
	}
	
	args.TokenizeString( menuCommand, false );

	for( icmd = 0; icmd < args.Argc(); )
	{
		const char *cmd = args.Argv( icmd++ );

		if ( !idStr::Icmp( cmd,	";"	) )
		{
			continue;
		}
		else if (	!idStr::Icmp( cmd, "deckcommand" ) || !idStr::Icmp( cmd, "deckcommand_echo" ) ) 
		{
			if ((GetLocalPlayer()->deckGui == NULL) || (GetLocalPlayer()->deckList == NULL))
				return NULL;

			GetLocalPlayer()->StartSound(  "snd_return" , SND_CHANNEL_ANY, 0, false, NULL );
			GetLocalPlayer()->lastMoveTime = gameLocal.time;

			//Local echo.
			const char *rawCommand= GetLocalPlayer()->deckGui->State().GetString("deckprompt");



			idEntity *recordEnt;
			recordEnt = gameLocal.FindEntity( "world1" );
			if (recordEnt)
			{
				static_cast<idWorldManager *>( recordEnt )->RecordDeck(rawCommand);
			}

			//bc 1-11-2016 if player did the right-click auto execute, echo to the hud.
			if (!idStr::Icmp( cmd, "deckcommand_echo" ))
			{
				idStr deckstr = rawCommand;
				deckstr.StripTrailingWhitespace(); //remove whitespace.
				deckstr.StripLeading( ' ' );		//remove whitespace.

				if (deckstr.Length())
				{
					gameLocal.GetLocalPlayer()->Event_hudMessage( va( common->GetLanguageDict()->GetString( "#str_02119" ), rawCommand ) );
				}
			}

			//run.
			if (!RunDeckCommand(rawCommand))
			{
				GetLocalPlayer()->deckList->Push( ">"); //empty line.
			}

			//clear the prompt.
			GetLocalPlayer()->deckGui->SetStateString("deckprompt", "");




#ifdef STEAM
			//bc stats
			g_SteamStats->SetStatDelta("stat_deckcommands", 1);
#endif

			return "continue";
		}
		else if ( !idStr::Icmp( cmd,	"scrollnext"	) )
		{
			if (GetLocalPlayer()->deckEnt.IsValid())
			{
				GetLocalPlayer()->UseFrob(  GetLocalPlayer()->deckEnt.GetEntity(),  "onScrollNext" );				
			}
			GetLocalPlayer()->lastMoveTime = gameLocal.time;
			return "continue";
		}
		else if ( !idStr::Icmp( cmd,	"scrollprev"	) )
		{
			if (GetLocalPlayer()->deckEnt.IsValid())
			{
				GetLocalPlayer()->UseFrob(  GetLocalPlayer()->deckEnt.GetEntity(),  "onScrollPrev" );
			}
			GetLocalPlayer()->lastMoveTime = gameLocal.time;
			return "continue";
		}
		/*
		//bc obsolete.
		else if ( !idStr::Icmp( cmd,	"scrollclick"	) )
		{
			if (GetLocalPlayer()->deckEnt.IsValid())
			{
				GetLocalPlayer()->UseFrob(  GetLocalPlayer()->deckEnt.GetEntity(),  "onScrollClick" );
			}
			GetLocalPlayer()->lastMoveTime = gameLocal.time;
			return "continue";
		}*/
		else if ( !idStr::Icmp( cmd, "decktab" ) )
		{

			//BC 3-30-2016 a timer between autocomplete calls.
			if (gameLocal.time < nextAutocompleteTime)
			{
				return "continue" ;
			}

			nextAutocompleteTime = gameLocal.time + 200;


			//AUTOCOMPLETE.
			GetLocalPlayer()->lastMoveTime = gameLocal.time;
			if (GetLocalPlayer()->deckEnt.IsValid())
			{
				idStr pristineCommand;
				const char *rawCommand = GetLocalPlayer()->deckGui->State().GetString("deckprompt");
				idStr cleanCommand = rawCommand;				
				cleanCommand.ToLower();					//force lower case.
				rawCommand = cleanCommand.c_str();
				pristineCommand = cleanCommand;

				


				if ( !rawCommand )
				{
					GetLocalPlayer()->StartSound( "snd_errorquiet", SND_CHANNEL_ANY, 0, false, NULL );
					return "continue";
				}
				
				int i,  k, idx;
				int stringLength = cleanCommand.Length() - 1;

				//there is a valid command in the prompt.

				if (rawCommand[stringLength] == ' ')
				{
					GetLocalPlayer()->StartSound( "snd_errorquiet", SND_CHANNEL_ANY, 0, false, NULL );
					return "continue";
				}

				
				//Find the word the cursor is currently on.
				int firstSpaceIndex, lastSpaceIndex;
				
				GetLocalPlayer()->deckGui->HandleNamedEvent("getCursorIndex"); //hack, get position of prompt cursor.
				int cursorIndex = cvarSystem->GetCVarInteger( "cursorindex" );

				
				//Determine where the starting space/semicolon is.
				firstSpaceIndex = -1;
				for (i = cursorIndex - 1; i >= 0; i--)
				{
					if (cleanCommand[i] == ' ' || cleanCommand[i] == ';')
					{
						firstSpaceIndex = i + 1;
						break;
					}
				}

				if (firstSpaceIndex < 0)
					firstSpaceIndex = 0;

				//Determine where the ending space/semicolon is.
				lastSpaceIndex = -1;
				for (i = cursorIndex ; i < stringLength; i++)
				{
					if (cleanCommand[i] == ' ' || cleanCommand[i] == ';')
					{
						lastSpaceIndex = i;
						break;
					}
				}

				if (lastSpaceIndex < 0)
					lastSpaceIndex = stringLength + 1;

				
				cleanCommand = cleanCommand.Mid(firstSpaceIndex , lastSpaceIndex - firstSpaceIndex );

				//GetLocalPlayer()->deckList->Push( va(" '%s'  %s  %d %d  cursor %d   length %d ", cleanCommand.c_str(), pristineCommand.c_str(), firstSpaceIndex, lastSpaceIndex, cursorIndex, stringLength ));

				
				//idStr finalCommand = pristineCommand.Left(firstSpaceIndex) + "bowzers" + pristineCommand.Mid(lastSpaceIndex, stringLength + 1 - lastSpaceIndex);
				

				
				//cleanCommand = the word we're operating on.
				//determine if there's a period in the word.
				

				int periodIndex = cleanCommand.Find(".", false, 0, -1);

				//TODO: check if there are multiple periods. If there are, then abort.

				if (periodIndex >= 0)
				{
					//There is a period. Therefore, we autocomplete the 'action' field.

					idStr actionString = cleanCommand.Mid(periodIndex + 1, cleanCommand.Length());
					idStr entityName = cleanCommand.Left(periodIndex);

					idEntity *targetEnt = gameLocal.FindEntity( entityName );

					//there is no entity with this name.
					if (!targetEnt)
					{
						GetLocalPlayer()->StartSound( "snd_errorquiet", SND_CHANNEL_ANY, 0, false, NULL );
						GetLocalPlayer()->deckList->Push( va("ERROR: Entity '%s' not found.", entityName.c_str()));
						return "continue";
					}


					//attempt to load the .AC autocomplete dictionary file.
					void *buffer;
					idStr rawDictionary;
					if (fileSystem->ReadFile( va("%s.ac", targetEnt->spawnArgs.GetString( "classname" )), &buffer) > 0)
					{
						rawDictionary = (char *) buffer;
						fileSystem->FreeFile( buffer );
					}
					else
					{
						//no file found.
						GetLocalPlayer()->StartSound( "snd_errorquiet", SND_CHANNEL_ANY, 0, false, NULL );
						return "continue";
					}
						
					rawDictionary.StripTrailingWhitespace(); //remove whitespace.
					rawDictionary.StripLeading( ' ' );		//remove whitespace.
					rawDictionary.ToLower();					//force lower case.

					//attempt to split the words via line breaks.
					idStrList candidates;
					for (i = rawDictionary.Length(); i >= 0; i--)
					{
						if (rawDictionary[i] == '\n' || rawDictionary[i] == '\r')
						{
							if (rawDictionary.Mid(i+1, rawDictionary.Length()).IsEmpty())
							{
								rawDictionary = rawDictionary.Left(i);
								continue;
							}

							candidates.AddUnique(rawDictionary.Mid(i+1, rawDictionary.Length()   ));

							rawDictionary = rawDictionary.Left(i); //strip.
						}

						if (i <= 0)
						{
							candidates.AddUnique(rawDictionary.Mid(0, rawDictionary.Length()   ));
						}
					}

					if (candidates.Num() <= 0)
					{
						GetLocalPlayer()->StartSound( "snd_errorquiet", SND_CHANNEL_ANY, 0, false, NULL );
						return "continue";
					}

					//Clean up the entries.
					for ( i = 0; i < candidates.Num(); i++)
					{
						candidates[i].StripLeading(' ');
						candidates[i].StripTrailingWhitespace();

						//GetLocalPlayer()->deckList->Push( va(" '%s'", candidates[i].c_str()  ));
					}

					//actionString is the action.
					idStr autocompleteString = GetAutocomplete(candidates, actionString);

					if (!autocompleteString)
					{
						GetLocalPlayer()->StartSound( "snd_errorquiet", SND_CHANNEL_ANY, 0, false, NULL );
						return "continue";
					}

					bool abort = true;
					bool addEndParentheses = false;
					if (autocompleteString.Length() <= 0)
					{
						//BC 11-4-2013
						//add end-parentheses semicolon autocomplete.

						int startParenthesesIndex = cleanCommand.Find("(", false, 0, -1);
						if (startParenthesesIndex >= 0)
						{
							//there is a start parentheses.
							//determine whether the player has entered a variable after it.
							if (cleanCommand.Length() > startParenthesesIndex
								&& cursorIndex >= pristineCommand.Length() )
							{
								autocompleteString = actionString;

								addEndParentheses = true;
								abort = false;
							}
						}

					}
					else
					{
						abort = false;
					}

					if (abort)
					{
						GetLocalPlayer()->StartSound( "snd_autocomplete1", SND_CHANNEL_ANY, 0, false, NULL );
						return "continue";
					}


					idStr finalCommand = pristineCommand.Left(firstSpaceIndex) + entityName + "." + autocompleteString;
					int desiredCursorPos = finalCommand.Length();
					finalCommand += pristineCommand.Mid(lastSpaceIndex, stringLength + 1 - lastSpaceIndex);

					
					if (addEndParentheses)
					{
						finalCommand += ");";
						desiredCursorPos += 2;
					}

					GetLocalPlayer()->deckGui->SetStateString("deckprompt", finalCommand.c_str());

					cvarSystem->SetCVarInteger( "desiredcursorpos", desiredCursorPos );
					GetLocalPlayer()->deckGui->HandleNamedEvent("movecursor");  //move cursor to end of line.

					GetLocalPlayer()->StartSound( "snd_autocomplete2", SND_CHANNEL_ANY, 0, false, NULL );					
				}
				/*
				else if (cleanCommand.Find("wait(") == 0)
				{

					idStr finalCommand = pristineCommand + ");";
					int desiredCursorPos = finalCommand.Length();
				

					GetLocalPlayer()->deckGui->SetStateString("deckprompt", finalCommand.c_str());

					cvarSystem->SetCVarInteger( "desiredcursorpos", desiredCursorPos );
					GetLocalPlayer()->deckGui->HandleNamedEvent("movecursor");  //move cursor to end of line.

					GetLocalPlayer()->StartSoundShader( declManager->FindSound( "click" ), SND_CHANNEL_ANY, 0, false, NULL );



				}*/
				else
				{
					//There is no period. Therefore, we autocomplete the name of an entity.
					idStrList candidates;

					GetLocalPlayer()->deckGui->HandleNamedEvent("getCursorIndex");
						

					//Go through all the entities.
					for ( i = 0; i < gameLocal.num_entities; i++ )
					{
						if ( !gameLocal.entities[ i ] )
							continue;

						//don't display things that are unhackable.
						if (gameLocal.entities[ i ]->spawnArgs.GetInt("unhackable") >= 1
							|| gameLocal.entities[ i ]->spawnArgs.GetInt("showname") <= 0)
							continue;

						//check if the classname matches.
						if (idStr::Icmp(  "func_door", gameLocal.entities[ i ]->spawnArgs.GetString( "classname" )) == 0
							|| idStr::Icmp(  "func_rotdoor", gameLocal.entities[ i ]->spawnArgs.GetString( "classname" )) == 0
							|| idStr::Icmp(  "func_securitycamera", gameLocal.entities[ i ]->spawnArgs.GetString( "classname" )) == 0
							|| idStr::Icmp(  "func_laserwire", gameLocal.entities[ i ]->spawnArgs.GetString( "classname" )) == 0)
						{
							candidates.AddUnique( gameLocal.entities[ i ]->GetName() );
						}
					}

					//wait command autocomplete.
					candidates.AddUnique("wait(");

			
						
					//Parse the autocompleted string.
					idStr autocompleteString = GetAutocomplete(candidates, cleanCommand);

					if (!autocompleteString)
					{
						GetLocalPlayer()->StartSound( "snd_errorquiet", SND_CHANNEL_ANY, 0, false, NULL );
						return "continue";
					}

					if (autocompleteString.Length() <= 0)
					{
						GetLocalPlayer()->StartSound( "snd_errorquiet", SND_CHANNEL_ANY, 0, false, NULL );
						return "continue";
					}


					
					//format the final output.					
					
					idStr finalCommand = pristineCommand.Left(firstSpaceIndex) + autocompleteString;
					int desiredCursorPos = finalCommand.Length();
					finalCommand += pristineCommand.Mid(lastSpaceIndex, stringLength + 1 - lastSpaceIndex);

					GetLocalPlayer()->deckGui->SetStateString("deckprompt", finalCommand.c_str());

					cvarSystem->SetCVarInteger( "desiredcursorpos", desiredCursorPos );
					GetLocalPlayer()->deckGui->HandleNamedEvent("movecursor");  //move cursor to end of line.

					GetLocalPlayer()->StartSound( "snd_autocomplete2", SND_CHANNEL_ANY, 0, false, NULL );					
				}				
			}
			return "continue";
		}
#ifdef STEAM
		else if ( !idStr::Icmp( cmd, "showtextinput" ) )
		{
			if (!steamInitialized || textinputOn)
			{
				return "continue";
			}

			if (SteamUtils()->ShowGamepadTextInput(k_EGamepadTextInputModeNormal, k_EGamepadTextInputLineModeSingleLine, "", STEAM_GAMEPAD_TEXTLENGTH, ""))
			{
				textinputOn = true;
			}

			return "continue";
		}
#endif
	}
	
	// ------- END BC -------

	if ( !isMultiplayer ) {
		return NULL;
	}
	return mpGame.HandleGuiCommands( menuCommand );
}

#ifdef STEAM
void idGameLocal::OnTextDismissed( GamepadTextInputDismissed_t  *pResult )
{
	textinputOn = false;

	if (!pResult->m_bSubmitted)
	{
		return;
	}

	int length = SteamUtils()->GetEnteredGamepadTextLength();

	char text[STEAM_GAMEPAD_TEXTLENGTH];
	if (!SteamUtils()->GetEnteredGamepadTextInput( text, pResult->m_unSubmittedText + 1 ))
	{
		return;
	}

	if (GetLocalPlayer()->deckGui)
	{
		GetLocalPlayer()->deckGui->SetStateString("deckprompt", text);

		//int desiredCursorPos =  pResult->m_unSubmittedText;
		//cvarSystem->SetCVarInteger( "desiredcursorpos", desiredCursorPos );
		//GetLocalPlayer()->deckGui->HandleNamedEvent("movecursor");  //move cursor to end of line.

		GetLocalPlayer()->Event_pressEnter();
	}
	
	//common->Printf("txt: %s      indeck: %d\n", text, gameLocal.GetLocalPlayer()->inDeck);
}
#endif



idStr idGameLocal::GetAutocomplete( idStrList candidates, idStr partialString )
{
	idStr autocompleteString;
	int i, k, idx;
	int maxStringLength = 1024;
	


	//Filter out candidates that don't match the player command.
	for ( i = candidates.Num() - 1; i >= 0; i-- )
	{
		if (candidates[i].Length() < partialString.Length())
		{
			//invalid.
			candidates.RemoveIndex(i);
			continue;
		}

		//compare the two strings. If there's a mismatch, then remove from candidate list.					
		for (k = 0; k < partialString.Length(); k++)
		{
			if (candidates[i][k] != partialString[k])
			{
				//invalid.
				candidates.RemoveIndex(i);
				break;
			}
		}
	}



	//get the maximum possible length of string.
	for ( i = candidates.Num() - 1; i >= 0; i-- )
	{
		if (candidates[i].Length() < maxStringLength)
			maxStringLength = candidates[i].Length();
	}

	//GetLocalPlayer()->deckList->Push( va(" %d", candidates.Num()  ));

	//if there's multiple candidates, then parse their data.						
	if (candidates.Num() > 1)
	{
		GetLocalPlayer()->deckList->Push( " ");
		GetLocalPlayer()->deckList->Push( va("    %d Matches:", candidates.Num()  ));

		//Compare the candidates with one another to find out the minimum match count. This determines what the autocomplete results.
		int matchCount = maxStringLength;
		int matchIndex = 0;
		for (i = 0; i < candidates.Num(); i++)
		{
			GetLocalPlayer()->deckList->Push( va("      %s", candidates[i].c_str()  ));

			for (k = 0; k < candidates.Num(); k++)
			{
				for (idx = 0; idx < maxStringLength; idx++)
				{
					if (candidates[i][idx] != candidates[k][idx])
					{
						if (idx < matchCount)
						{
							matchCount = idx;
							matchIndex = i;
						}

						

						break;
					}
				}
			}


		}

		GetLocalPlayer()->deckList->Push( " ");

		//if no matches are found.
		if (matchCount >= 1024)
		{
			return NULL;
		}

		autocompleteString = candidates[matchIndex].Left(matchCount).c_str();
	}
	else if (candidates.Num() > 0)
	{
		//only one match!
		return candidates[0];
	}
	else
	{
		//no matches.
		return NULL;
	}

	return autocompleteString;
}

idStr idGameLocal::ParseStickyLine(idStrList lines, idStr key)
{
	int i;

	for (i = 0; i < lines.Num(); i++)
	{
		if (lines[i].Find(key, false) < 0)
		{
			continue;
		}

		return lines[i].Mid(key.Length() + 1, lines[i].Length() - key.Length() - 1);
	}

	return "";
}

void idGameLocal::ParseStickynotes(const char *text, bool local)
{
	idStr raw = text;
	int charIndex = 0;

	while (true)
	{
		int i;
		int startbracketIdx = raw.Find("{", false, charIndex);
		charIndex = startbracketIdx + 1;

		if (startbracketIdx < 0)
		{
			//no more.
			break;
		}

		int endbracketIdx = raw.Find("}", false, charIndex);
		charIndex = endbracketIdx + 1;

		if (endbracketIdx < 0)
		{
			//no more.
			break;
		}

		idStr middle = raw.Mid(startbracketIdx + 1, endbracketIdx - startbracketIdx - 1);

		middle.StripTrailingWhitespace();
		middle.StripLeading( ' ' );

		if (middle.Length() <= 0)
		{
			continue;
		}

		//common->Printf("zzzz %s\n\n" , middle.c_str());

		idStrList candidates;
		
		//split into lines.
		idStr tempStr = "";
		for (i = 0; i < middle.Length(); i++)
		{
			if (middle[i] == '\n')
			{
				tempStr.StripTrailingWhitespace();
				tempStr.StripLeading(' ');

				if (tempStr.Length() > 0)
				{
					//common->Printf("LINE %s\n", tempStr.c_str());
					candidates.Append(tempStr);
				}

				tempStr = "";
			}
			else
			{
				if (middle[i] != '\r')
				{
					tempStr += middle[i];
				}
			}

			if (i >= middle.Length() - 1)
			{
				tempStr.StripTrailingWhitespace();
				tempStr.StripLeading(' ');

				if (tempStr.Length() > 0)
				{
					//common->Printf("LINE %s\n", tempStr.c_str());
					candidates.Append(tempStr);
				}

				//tempStr = "";
				break;
			}
		}

		//are now split into lines.
		//now parse the data.

		
		idStr nametag = ParseStickyLine(candidates, "nametag");
		idStr textmsg = ParseStickyLine(candidates, "text");
		idStr origin = ParseStickyLine(candidates, "origin");
		idStr rotation = ParseStickyLine(candidates, "rotation");
		idStr map = ParseStickyLine(candidates, "map");

		map.ToLower();

		if (idStr::Cmp( mapFileName, map ) != 0)
		{
			continue;
		}
		
		idDict args;

		args.Clear();
		args.Set( "origin", origin );
		args.Set( "model", "models/stickynote/tris.ase" );
		args.SetInt( "frobbable", 0 );
		args.SetInt( "solid", 0 );
		args.SetInt( "noGrab", 1 );
		args.SetBool( "local", local);
		args.SetInt( "stickynote", 1);
		args.Set("gui", "guis/stickynote/sticky000.gui");
		args.Set("gui_parm0", nametag );
		args.Set("gui_parm1",  textmsg );
		args.Set("rotation", rotation);
		idEntity *temp = gameLocal.SpawnEntityType( idStaticEntity::Type, &args );
		
	}

}


#ifdef STEAM
void idGameLocal::OnClickLeaderboard( void )
{
	if (steamLeaderboardNames.Num() <= 0)
		return;

	if (leaderboardList.Num() <= 0)
		return;

	int mapNum = leaderboardMaplist->GetSelection( NULL, 0 );

	if (mapNum < 0)
		return;

	gameLocal.mainmenuLeaderboardNames->Clear();
	gameLocal.mainmenuLeaderboardTimes->Clear();

	idStr boardname = leaderboardList[mapNum];

	int index = -1;

	for (int i = 0; i < steamLeaderboardNames.Num(); i++)
	{
		if (idStr::Icmp(boardname, steamLeaderboardNames[i]) == 0)
		{
			index = i;
			break;
		}
	}

	if (index < 0)
		return;		

	g_SteamLeaderboard->UpdateMainmenuLeaderboards(steamLeaderboardHandles[index]);

	//if (leaderboardMaplist->GetNumSelections() > 0)
	//	leaderboardMaplist->SetSelection(0);
}
#endif


/*
================
idGameLocal::HandleMainMenuCommands
================
*/
void idGameLocal::HandleMainMenuCommands( const char *menuCommand, idUserInterface *gui )
{

#ifdef STEAM
	int				icmd;
	idCmdArgs		args;

	
	

	args.TokenizeString( menuCommand, false );

	for( icmd = 0; icmd < args.Argc(); )
	{
		const char *cmd = args.Argv( icmd++ );




		//BC
		/*
		if ( !idStr::Icmp( cmd, "updatebugcount" ))
		{
			SteamUserStats()->GetStat("stat_bugreports", &lastbugreportcount );
			mainGui->SetStateInt("stat_bugreports", lastbugreportcount);

			continue;
		}

		if ( !idStr::Icmp( cmd, "sendbugreport" ) )
		{
			idStr rawText = mainGui->State().GetString("reportabug_text");
			rawText.StripTrailingWhitespace();
			rawText.StripLeading(' ');

			if (rawText.Length())
			{
				g_SteamStats->SetStatDelta("stat_bugreports", 1);

				//increase the number.
				lastbugreportcount++;
				mainGui->SetStateInt("stat_bugreports", lastbugreportcount);
			}
			continue;
		}
		*/



		if (!idStr::Icmp(cmd, "resetsteamstats") && steamInitialized)
		{
			common->Printf("STEAM: resetting stats.\n");
			g_SteamStats->ResetStats();
			continue;
		}

		if (!idStr::Icmp(cmd, "resetmissionprogress"))
		{
			for (int i = 0; i < 32; i++)
			{
				cvarSystem->SetCVarBool(va("g_leveldone_%d", i), false);
				mainGui->SetStateBool(va("donemark_%d", i), false);
			}

			continue;
		}


		//BC
		/*
		if (!idStr::Icmp(cmd, "resetsteamnotes"))
		{
			common->Printf("STEAM: resetting sticky notes.\n");
			continue;
		}
		*/

		//BC
		if (!idStr::Icmp(cmd, "leaderboardscan") && steamInitialized)
		{
			if (hasInitializedLeaderboardlist)
				continue;

			hasInitializedLeaderboardlist = true;

			//Generate the map list.			
			leaderboardMaplist = uiManager->AllocListGUI();
			leaderboardMaplist->Clear();
			leaderboardMaplist->Config( mainGui, "leaderboardmaplist" );
			leaderboardMaplist->SetSelection(0);


			void *buffer;
			idStr rawDictionary;

			if (fileSystem->ReadFile( "maplist.txt", &buffer) > 0)
			{
				rawDictionary = (char *) buffer;
				fileSystem->FreeFile( buffer );
			}
			else
			{
				common->Printf("ERROR: unable to load maplist.txt\n");
				continue;
			}

			rawDictionary.StripTrailingWhitespace(); //remove whitespace.
			rawDictionary.StripLeading( ' ' );		//remove whitespace.
			rawDictionary.ToUpper();

			int mapIndex = 0;

			leaderboardList.Clear();

			while ( 1 )
			{
				int carriageIdx = Max( rawDictionary.Find('\n',0), rawDictionary.Find('\r',0) );
				//common->Printf("%d %d\n", rawDictionary.Find('\n',0), rawDictionary.Find('\r',0));

				if (carriageIdx < 0)
				{
					carriageIdx = rawDictionary.Length();
				}

				idStr workStr = rawDictionary.Mid(0, carriageIdx);				
				
				idStr mapnameshort = workStr;
				mapnameshort.StripTrailingWhitespace(); //remove whitespace.
				mapnameshort.StripLeading( ' ' );		//remove whitespace.
				mapnameshort.ToLower();


				//get the names.
				const idDecl *mapDecl = declManager->FindType( DECL_MAPDEF, mapnameshort, false );
				const idDeclEntityDef *mapDef = static_cast<const idDeclEntityDef *>( mapDecl );

				if ( mapDef )
				{
					//found the map def.
					idStr boardCount = common->GetLanguageDict()->GetString(mapDef->dict.GetString( "boards" ));

					idStr displayName = common->GetLanguageDict()->GetString(mapDef->dict.GetString( "shortname" ));

					//common->Printf("mapshort %s   %s   \n", mapnameshort.c_str(), displayName.c_str());

					if (boardCount.IsEmpty())
					{
						//nothing here. move on.
					}	
					else if (idStr::Icmp(boardCount, "0") == 0)
					{
						//nothing here.
					}
					else if (idStr::Icmp(boardCount, "1") == 0)
					{
						leaderboardList.AddUnique(va("times_%s_0", mapnameshort.c_str()) );
						leaderboardMaplist->Add(mapIndex, displayName.c_str());

						//grab handle of leaderboard.
						//BC 7-15-2016
						g_SteamLeaderboard->GetMainmenuLeaderboards(mapIndex, va("times_%s_0", mapnameshort.c_str()));

						mapIndex++;
					}
					else
					{
						int amount = 0;

						if (idStr::Icmp(boardCount, "2") == 0)
						{
							amount = 2;
						}
						else if (idStr::Icmp(boardCount, "3") == 0)
						{
							amount = 3;
						}


						if (amount > 0)
						{
							for (int i = 0; i < amount; i++)
							{
								leaderboardList.AddUnique(va("times_%s_%d", mapnameshort.c_str(), i) );
								leaderboardMaplist->Add(mapIndex, va("%s-%d",displayName.c_str(), i ));

								//grab handle of leaderboard.
								g_SteamLeaderboard->GetMainmenuLeaderboards(mapIndex, va("times_%s_%d", mapnameshort.c_str(), i));

								mapIndex++;
							}
						}
					}
					
					rawDictionary = rawDictionary.Mid(carriageIdx + 1, rawDictionary.Length() - carriageIdx - 1); //strip

					if (rawDictionary.Length() <= 0)
						break;
				}				
			}

			//uiManager->FreeListGUI( leaderboardMaplist );



			mainmenuLeaderboardNames = uiManager->AllocListGUI();
			mainmenuLeaderboardNames->Clear();
			mainmenuLeaderboardNames->Config( mainGui, "leaderlist_names" );

			mainmenuLeaderboardTimes = uiManager->AllocListGUI();
			mainmenuLeaderboardTimes->Clear();
			mainmenuLeaderboardTimes->Config( mainGui, "leaderlist_times" );
		}




		if ( !idStr::Icmp( cmd, "click_leaderboardList" ) && steamInitialized)
		{
			OnClickLeaderboard();
			
			
			continue;
		}








		if ( !idStr::Icmp( cmd, "updateFriendlistInfo" ) && steamInitialized )
		{
			if (mainGui == NULL)
				break;

			int choice = mainGui->State().GetInt( "friendlist_sel_0" );

			if ( choice >= 0 && choice < friendIDs.Num() )
			{
				int i;
				const char *friendName = SteamFriends()->GetFriendPersonaName(friendIDs[choice]);
				mainGui->SetStateString( "statFriendname", friendName );

				if (g_SteamStats->RequestUserStats(friendIDs[choice]))
				{
					int numStats = g_SteamStats->GetNumStats();

					for (i = 0; i < numStats; i++)
					{
						//blank out the stat numbers.
						mainGui->SetStateString( g_Stats[i].m_pchStatName, "--");

						//Retrieve user stats.
					
						int value;
						if (g_SteamStats->GetUserStat(friendIDs[choice], g_Stats[i].m_pchStatName, value))
						{
							if (value <= 0)
							{
								mainGui->SetStateString( g_Stats[i].m_pchStatName, "--");
							}
							else
							{
								mainGui->SetStateString( g_Stats[i].m_pchStatName, va("%d", value));
							}
						}
						else
						{
							mainGui->SetStateString( g_Stats[i].m_pchStatName, "--");
						}					
					}
				}
			}
		}



		//end of mainmenu commands

	}


#endif

	//bc main menu save UI.
	if (	mainGui )
	{
		

		//lastSavetime
		int		diff;
		idStr	timeOutput;

		if (lastSavetime <= 0)
		{
			mainGui->SetStateString("lastsavetime", "");
			return;
		}

		diff = gameLocal.time - lastSavetime;

		if (diff < 5000)
			timeOutput = "just now.";
		else if (diff < 60000)
			timeOutput = "less than a minute ago.";
		else
		{
			int timeDigit = diff / 60000;
			if (timeDigit <= 1)
			{
				timeOutput = "a minute ago.";
			}
			else
			{
				timeOutput = va("%d minutes ago.", timeDigit);
			}
		}

		mainGui->SetStateString("lastsavetime", va("Last save:\n%s", timeOutput.c_str()));
	}
}

/*
================
idGameLocal::GetLevelMap

  should only be used for in-game level editing
================
*/
idMapFile *idGameLocal::GetLevelMap( void ) {
	if ( mapFile && mapFile->HasPrimitiveData()) {
		return mapFile;
	}
	if ( !mapFileName.Length() ) {
		return NULL;
	}

	if ( mapFile ) {
		delete mapFile;
	}

	mapFile = new idMapFile;
	if ( !mapFile->Parse( mapFileName ) ) {
		delete mapFile;
		mapFile = NULL;
	}

	return mapFile;
}

/*
================
idGameLocal::GetMapName
================
*/
const char *idGameLocal::GetMapName( void ) const {
	return mapFileName.c_str();
}

/*
================
idGameLocal::CallFrameCommand
================
*/
void idGameLocal::CallFrameCommand( idEntity *ent, const function_t *frameCommand ) {
	frameCommandThread->CallFunction( ent, frameCommand, true );
	frameCommandThread->Execute();
}

/*
================
idGameLocal::CallObjectFrameCommand
================
*/
void idGameLocal::CallObjectFrameCommand( idEntity *ent, const char *frameCommand ) {
	const function_t *func;

	func = ent->scriptObject.GetFunction( frameCommand );
	if ( !func ) {
		if ( !ent->IsType( idTestModel::Type ) ) {
			Error( "Unknown function '%s' called for frame command on entity '%s'", frameCommand, ent->name.c_str() );
		}
	} else {
		frameCommandThread->CallFunction( ent, func, true );
		frameCommandThread->Execute();
	}
}

/*
================
idGameLocal::ShowTargets
================
*/
void idGameLocal::ShowTargets( void ) {
	idMat3		axis = GetLocalPlayer()->viewAngles.ToMat3();
	idVec3		up = axis[ 2 ] * 5.0f;
	const idVec3 &viewPos = GetLocalPlayer()->GetPhysics()->GetOrigin();
	idBounds	viewTextBounds( viewPos );
	idBounds	viewBounds( viewPos );
	idBounds	box( idVec3( -4.0f, -4.0f, -4.0f ), idVec3( 4.0f, 4.0f, 4.0f ) );
	idEntity	*ent;
	idEntity	*target;
	int			i;
	idBounds	totalBounds;

	viewTextBounds.ExpandSelf( 128.0f );
	viewBounds.ExpandSelf( 512.0f );
	for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		totalBounds = ent->GetPhysics()->GetAbsBounds();
		for( i = 0; i < ent->targets.Num(); i++ ) {
			target = ent->targets[ i ].GetEntity();
			if ( target ) {
				totalBounds.AddBounds( target->GetPhysics()->GetAbsBounds() );
			}
		}

		if ( !viewBounds.IntersectsBounds( totalBounds ) ) {
			continue;
		}

		float dist;
		idVec3 dir = totalBounds.GetCenter() - viewPos;
		dir.NormalizeFast();
		totalBounds.RayIntersection( viewPos, dir, dist );
		float frac = ( 512.0f - dist ) / 512.0f;
		if ( frac < 0.0f ) {
			continue;
		}

		gameRenderWorld->DebugBounds( ( ent->IsHidden() ? colorLtGrey : colorOrange ) * frac, ent->GetPhysics()->GetAbsBounds() );
		if ( viewTextBounds.IntersectsBounds( ent->GetPhysics()->GetAbsBounds() ) ) {
			idVec3 center = ent->GetPhysics()->GetAbsBounds().GetCenter();
			gameRenderWorld->DrawText( ent->name.c_str(), center - up, 0.1f, colorWhite * frac, axis, 1 );
			gameRenderWorld->DrawText( ent->GetEntityDefName(), center, 0.1f, colorWhite * frac, axis, 1 );
			gameRenderWorld->DrawText( va( "#%d", ent->entityNumber ), center + up, 0.1f, colorWhite * frac, axis, 1 );
		}

		for( i = 0; i < ent->targets.Num(); i++ ) {
			target = ent->targets[ i ].GetEntity();
			if ( target ) {
				gameRenderWorld->DebugArrow( colorYellow * frac, ent->GetPhysics()->GetAbsBounds().GetCenter(), target->GetPhysics()->GetOrigin(), 10, 0 );
				gameRenderWorld->DebugBounds( colorGreen * frac, box, target->GetPhysics()->GetOrigin() );
			}
		}
	}
}

/*
================
idGameLocal::RunDebugInfo
================
*/
void idGameLocal::RunDebugInfo( void ) {
	idEntity *ent;
	idPlayer *player;

	player = GetLocalPlayer();
	if ( !player ) {
		return;
	}

	const idVec3 &origin = player->GetPhysics()->GetOrigin();

	//bc
	if (g_showModelNames.GetBool())
	{
		idMat3		axis = player->viewAngles.ToMat3();
		idVec3		up = axis[ 2 ] * 5.0f;
		idBounds	viewTextBounds( origin );
		idBounds	viewBounds( origin );

		float maxDistance = 320.0f;

		viewTextBounds.ExpandSelf( maxDistance );
		viewBounds.ExpandSelf( maxDistance );
		for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
		{
			// don't draw the worldspawn
			if ( ent == world ) {
				continue;
			}

			if (!ent->spawnArgs.GetString( "model" ))
				continue;

			if ( !viewBounds.IntersectsBounds( ent->GetPhysics()->GetAbsBounds() ) )
			{
				continue;
			}

			const idBounds &entBounds = ent->GetPhysics()->GetAbsBounds();
			
			
			if ( viewTextBounds.IntersectsBounds( entBounds ) )
			{

				gameRenderWorld->DrawText( ent->spawnArgs.GetString( "model" ), entBounds.GetCenter(), 0.1f  , idVec4(1,1,1,1), axis, 1 );
			}
		}
	}






	//modified version of g_showEntityInfo.
	if ( g_showEntityNames.GetBool() )
	{
		idMat3		axis = player->viewAngles.ToMat3();
		idVec3		up = axis[ 2 ] * 5.0f;
		idBounds	viewTextBounds( origin );
		idBounds	viewBounds( origin );

		float maxDistance = 640.0f;


		/*
		bool isCasing = false;
		idEntity *recordEnt;
		recordEnt = gameLocal.FindEntity( "world1" );
		if (recordEnt)
		{
			if (static_cast<idWorldManager *>( recordEnt )->index < 0)
			{
				isCasing = true;
			}
		}
		*/

		viewTextBounds.ExpandSelf( maxDistance );
		viewBounds.ExpandSelf( maxDistance );
		for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
		{
			// don't draw the worldspawn
			if ( ent == world ) {
				continue;
			}

			/*
			int roleNumber = ent->spawnArgs.GetInt("role", "-1");
			if (roleNumber >= 0 && isCasing)
			{
				idStr rolename = "REQUIRED:\n";
				switch (roleNumber)
				{
					case 0:	rolename += common->GetLanguageDict()->GetString( "#str_02000" ); break;
					case 1:	rolename += common->GetLanguageDict()->GetString( "#str_02001" ); break;
					case 2:	rolename += common->GetLanguageDict()->GetString( "#str_02002" ); break;
					default: rolename = "???"; break;
				}

				gameRenderWorld->DrawText(rolename, ent->GetPhysics()->GetOrigin(), 0.25f, idVec4(1,1,0,1), axis, 1);
			}
			*/

			if (!ent->spawnArgs.GetBool( "showname", "0" ))
				continue;



			// draw the objective markers.
			if ( !idStr::Cmp( ent->spawnArgs.GetString( "classname" ), "info_objective" ) )
			{
				int size;

				//blink speed.
				if ((int)(gameLocal.time * 0.004) % 2 == 0)
					size = 8;
				else
					size = 7;
				
				const idBounds &objBounds = idBounds(
					idVec3(ent->GetPhysics()->GetOrigin()[0]-size,ent->GetPhysics()->GetOrigin()[1]-size,ent->GetPhysics()->GetOrigin()[2]-size),
					idVec3(ent->GetPhysics()->GetOrigin()[0]+size,ent->GetPhysics()->GetOrigin()[1]+size,ent->GetPhysics()->GetOrigin()[2]+size));
				
				gameRenderWorld->DebugBounds( idVec4( 1.00f, 0.55f, 0.00f, 1.00f ), objBounds );
				
				continue;
			}			

			// skip if the entity is very far away
			if ( !viewBounds.IntersectsBounds( ent->GetPhysics()->GetAbsBounds() ) )
			{
				continue;
			}

			const idBounds &entBounds = ent->GetPhysics()->GetAbsBounds();
			
			
			if ( viewTextBounds.IntersectsBounds( entBounds ) )
			{
				int contents = ent->GetPhysics()->GetContents();
				if ( contents & CONTENTS_SOLID )
				{
					idVec4 linecolor;
					idVec4 textcolor;

					if ( gameLocal.time < ent->uiAlarmTimer )
					{
						linecolor = idVec4(1,0,0,1);						
						textcolor = linecolor;
					}
					else if ( gameLocal.time < ent->uiHackTimer )
					{
						linecolor = idVec4(1,0.9f,0,1);
						textcolor = linecolor;
					}
					else
					{
						linecolor = idVec4(0,1,0,1);
						textcolor = idVec4(1,1,1,1);
					}

					gameRenderWorld->DrawText( ent->name.c_str(), entBounds.GetCenter(), 0.4f  /*textsize*/, textcolor, axis, 1 );

					collisionModelManager->DrawModel(
						ent->GetPhysics()->GetClipModel()->Handle(),
						ent->GetPhysics()->GetClipModel()->GetOrigin(), ent->GetPhysics()->GetClipModel()->GetAxis(),
						player->GetEyePosition(), maxDistance, linecolor );		
				}
			}
		}
	}

	if ( g_showEntityInfo.GetBool() ) {
		idMat3		axis = player->viewAngles.ToMat3();
		idVec3		up = axis[ 2 ] * 5.0f;
		idBounds	viewTextBounds( origin );
		idBounds	viewBounds( origin );

		viewTextBounds.ExpandSelf( 128.0f );
		viewBounds.ExpandSelf( 512.0f );
		for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
			// don't draw the worldspawn
			if ( ent == world ) {
				continue;
			}

			// skip if the entity is very far away
			if ( !viewBounds.IntersectsBounds( ent->GetPhysics()->GetAbsBounds() ) ) {
				continue;
			}

			const idBounds &entBounds = ent->GetPhysics()->GetAbsBounds();
			int contents = ent->GetPhysics()->GetContents();
			if ( contents & CONTENTS_BODY ) {
				gameRenderWorld->DebugBounds( colorCyan, entBounds );
			} else if ( contents & CONTENTS_TRIGGER ) {
				gameRenderWorld->DebugBounds( colorOrange, entBounds );
			} else if ( contents & CONTENTS_SOLID ) {
				gameRenderWorld->DebugBounds( colorGreen, entBounds );
			} else {
				if ( !entBounds.GetVolume() ) {
					gameRenderWorld->DebugBounds( colorMdGrey, entBounds.Expand( 8.0f ) );
				} else {
					gameRenderWorld->DebugBounds( colorMdGrey, entBounds );
				}
			}
			if ( viewTextBounds.IntersectsBounds( entBounds ) ) {
				gameRenderWorld->DrawText( ent->name.c_str(), entBounds.GetCenter(), 0.1f, colorWhite, axis, 1 );
				gameRenderWorld->DrawText( va( "#%d", ent->entityNumber ), entBounds.GetCenter() + up, 0.1f, colorWhite, axis, 1 );
			}
		}
	}

	// debug tool to draw bounding boxes around active entities
	if ( g_showActiveEntities.GetBool() ) {
		for( ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() ) {
			idBounds	b = ent->GetPhysics()->GetBounds();
			if ( b.GetVolume() <= 0 ) {
				b[0][0] = b[0][1] = b[0][2] = -8;
				b[1][0] = b[1][1] = b[1][2] = 8;
			}
			if ( ent->fl.isDormant ) {
				gameRenderWorld->DebugBounds( colorYellow, b, ent->GetPhysics()->GetOrigin() );
			} else {
				gameRenderWorld->DebugBounds( colorGreen, b, ent->GetPhysics()->GetOrigin() );
			}
		}
	}

	if ( g_showTargets.GetBool() ) {
		ShowTargets();
	}

	if ( g_showTriggers.GetBool() ) {
		idTrigger::DrawDebugInfo();
	}

	if ( ai_showCombatNodes.GetBool() ) {
		idCombatNode::DrawDebugInfo();
	}

	if ( ai_showPaths.GetBool() ) {
		idPathCorner::DrawDebugInfo();
	}

	if ( g_editEntityMode.GetBool() ) {
		editEntities->DisplayEntities();
	}

	if ( g_showCollisionWorld.GetBool() ) {
		collisionModelManager->DrawModel( 0, vec3_origin, mat3_identity, origin, 128.0f, idVec4(0,1,0,1) );
	}

	if ( g_showCollisionModels.GetBool() ) {
		clip.DrawClipModels( player->GetEyePosition(), g_maxShowDistance.GetFloat(), pm_thirdPerson.GetBool() ? NULL : player );
	}

	if ( g_showCollisionTraces.GetBool() ) {
		clip.PrintStatistics();
	}

	if ( g_showPVS.GetInteger() ) {
		pvs.DrawPVS( origin, ( g_showPVS.GetInteger() == 2 ) ? PVS_ALL_PORTALS_OPEN : PVS_NORMAL );
	}

	if ( aas_test.GetInteger() >= 0 ) {
		idAAS *aas = GetAAS( aas_test.GetInteger() );
		if ( aas ) {
			aas->Test( origin );
			if ( ai_testPredictPath.GetBool() ) {
				idVec3 velocity;
				predictedPath_t path;

				velocity.x = cos( DEG2RAD( player->viewAngles.yaw ) ) * 100.0f;
				velocity.y = sin( DEG2RAD( player->viewAngles.yaw ) ) * 100.0f;
				velocity.z = 0.0f;
				idAI::PredictPath( player, aas, origin, velocity, 1000, 100, SE_ENTER_OBSTACLE | SE_BLOCKED | SE_ENTER_LEDGE_AREA, path );
			}
		}
	}

	if ( ai_showObstacleAvoidance.GetInteger() == 2 ) {
		idAAS *aas = GetAAS( 0 );
		if ( aas ) {
			idVec3 seekPos;
			obstaclePath_t path;

			seekPos = player->GetPhysics()->GetOrigin() + player->viewAxis[0] * 200.0f;
			idAI::FindPathAroundObstacles( player->GetPhysics(), aas, NULL, player->GetPhysics()->GetOrigin(), seekPos, path );
		}
	}

	// collision map debug output
	collisionModelManager->DebugOutput( player->GetEyePosition() );
}

/*
==================
idGameLocal::NumAAS
==================
*/
int	idGameLocal::NumAAS( void ) const {
	return aasList.Num();
}

/*
==================
idGameLocal::GetAAS
==================
*/
idAAS *idGameLocal::GetAAS( int num ) const {
	if ( ( num >= 0 ) && ( num < aasList.Num() ) ) {
		if ( aasList[ num ] && aasList[ num ]->GetSettings() ) {
			return aasList[ num ];
		}
	}
	return NULL;
}

/*
==================
idGameLocal::GetAAS
==================
*/
idAAS *idGameLocal::GetAAS( const char *name ) const {
	int i;

	for ( i = 0; i < aasNames.Num(); i++ ) {
		if ( aasNames[ i ] == name ) {
			if ( !aasList[ i ]->GetSettings() ) {
				return NULL;
			} else {
				return aasList[ i ];
			}
		}
	}
	return NULL;
}

/*
==================
idGameLocal::SetAASAreaState
==================
*/
void idGameLocal::SetAASAreaState( const idBounds &bounds, const int areaContents, bool closed ) {
	int i;

	for( i = 0; i < aasList.Num(); i++ ) {
		aasList[ i ]->SetAreaState( bounds, areaContents, closed );
	}
}

/*
==================
idGameLocal::AddAASObstacle
==================
*/
aasHandle_t idGameLocal::AddAASObstacle( const idBounds &bounds ) {
	int i;
	aasHandle_t obstacle;
	aasHandle_t check;

	if ( !aasList.Num() ) {
		return -1;
	}

	obstacle = aasList[ 0 ]->AddObstacle( bounds );
	for( i = 1; i < aasList.Num(); i++ ) {
		check = aasList[ i ]->AddObstacle( bounds );
		assert( check == obstacle );
	}

	return obstacle;
}

/*
==================
idGameLocal::RemoveAASObstacle
==================
*/
void idGameLocal::RemoveAASObstacle( const aasHandle_t handle ) {
	int i;

	for( i = 0; i < aasList.Num(); i++ ) {
		aasList[ i ]->RemoveObstacle( handle );
	}
}

/*
==================
idGameLocal::RemoveAllAASObstacles
==================
*/
void idGameLocal::RemoveAllAASObstacles( void ) {
	int i;

	for( i = 0; i < aasList.Num(); i++ ) {
		aasList[ i ]->RemoveAllObstacles();
	}
}

/*
==================
idGameLocal::CheatsOk
==================
*/
bool idGameLocal::CheatsOk( bool requirePlayer ) {
	idPlayer *player;

	if ( isMultiplayer && !cvarSystem->GetCVarBool( "net_allowCheats" ) ) {
		Printf( "Not allowed in multiplayer.\n" );
		return false;
	}

	if ( developer.GetBool() ) {
		return true;
	}

	player = GetLocalPlayer();
	if ( !requirePlayer || ( player && ( player->health > 0 ) ) ) {
		return true;
	}

	Printf( "You must be alive to use this command.\n" );

	return false;
}

/*
===================
idGameLocal::RegisterEntity
===================
*/
void idGameLocal::RegisterEntity( idEntity *ent ) {
	int spawn_entnum;

	if ( spawnCount >= ( 1 << ( 32 - GENTITYNUM_BITS ) ) ) {
		Error( "idGameLocal::RegisterEntity: spawn count overflow" );
	}

	if ( !spawnArgs.GetInt( "spawn_entnum", "0", spawn_entnum ) ) {
		while( entities[firstFreeIndex] && firstFreeIndex < ENTITYNUM_MAX_NORMAL ) {
			firstFreeIndex++;
		}
		if ( firstFreeIndex >= ENTITYNUM_MAX_NORMAL ) {
			Error( "no free entities" );
		}
		spawn_entnum = firstFreeIndex++;
	}

	entities[ spawn_entnum ] = ent;
	spawnIds[ spawn_entnum ] = spawnCount++;
	ent->entityNumber = spawn_entnum;
	ent->spawnNode.AddToEnd( spawnedEntities );
	ent->spawnArgs.TransferKeyValues( spawnArgs );

	if ( spawn_entnum >= num_entities ) {
		num_entities++;
	}
}

/*
===================
idGameLocal::UnregisterEntity
===================
*/
void idGameLocal::UnregisterEntity( idEntity *ent ) {
	assert( ent );

	if ( editEntities ) {
		editEntities->RemoveSelectedEntity( ent );
	}

	if ( ( ent->entityNumber != ENTITYNUM_NONE ) && ( entities[ ent->entityNumber ] == ent ) ) {
		ent->spawnNode.Remove();
		entities[ ent->entityNumber ] = NULL;
		spawnIds[ ent->entityNumber ] = -1;
		if ( ent->entityNumber >= MAX_CLIENTS && ent->entityNumber < firstFreeIndex ) {
			firstFreeIndex = ent->entityNumber;
		}
		ent->entityNumber = ENTITYNUM_NONE;
	}
}

/*
================
idGameLocal::SpawnEntityType
================
*/
idEntity *idGameLocal::SpawnEntityType( const idTypeInfo &classdef, const idDict *args, bool bIsClientReadSnapshot ) {
	idClass *obj;

#if _DEBUG
	if ( isClient ) {
	//	assert( bIsClientReadSnapshot ); BC
	}
#endif

	if ( !classdef.IsType( idEntity::Type ) ) {
		Error( "Attempted to spawn non-entity class '%s'", classdef.classname );
	}

	try {
		if ( args ) {
			spawnArgs = *args;
		} else {
			spawnArgs.Clear();
		}
		obj = classdef.CreateInstance();
		obj->CallSpawn();
	}
	
	catch( idAllocError & ) {
		obj = NULL;
	}
	spawnArgs.Clear();

	return static_cast<idEntity *>(obj);
}

/*
===================
idGameLocal::SpawnEntityDef

Finds the spawn function for the entity and calls it,
returning false if not found
===================
*/
bool idGameLocal::SpawnEntityDef( const idDict &args, idEntity **ent, bool setDefaults ) {
	const char	*classname;
	const char	*spawn;
	idTypeInfo	*cls;
	idClass		*obj;
	idStr		error;
	const char  *name;

	if ( ent ) {
		*ent = NULL;
	}

	spawnArgs = args;

	if ( spawnArgs.GetString( "name", "", &name ) ) {
		sprintf( error, " on '%s'", name);
	}

	spawnArgs.GetString( "classname", NULL, &classname );

	const idDeclEntityDef *def = FindEntityDef( classname, false );

	if ( !def ) {
		Warning( "Unknown classname '%s'%s.", classname, error.c_str() );
		return false;
	}

	spawnArgs.SetDefaults( &def->dict );

#ifdef _D3XP
	if ( !spawnArgs.FindKey( "slowmo" ) ) {
		bool slowmo = true;

		for ( int i = 0; fastEntityList[i]; i++ ) {
			if ( !idStr::Cmp( classname, fastEntityList[i] ) ) {
				slowmo = false;
				break;
			}
		}

		if ( !slowmo ) {
			spawnArgs.SetBool( "slowmo", slowmo );
		}
	}
#endif

	// check if we should spawn a class object
	spawnArgs.GetString( "spawnclass", NULL, &spawn );
	if ( spawn ) {

		cls = idClass::GetClass( spawn );
		if ( !cls ) {
			Warning( "Could not spawn '%s'.  Class '%s' not found %s.", classname, spawn, error.c_str() );
			return false;
		}

		obj = cls->CreateInstance();
		if ( !obj ) {
			Warning( "Could not spawn '%s'. Instance could not be created %s.", classname, error.c_str() );
			return false;
		}

		obj->CallSpawn();

		if ( ent && obj->IsType( idEntity::Type ) ) {
			*ent = static_cast<idEntity *>(obj);
		}

		return true;
	}

	// check if we should call a script function to spawn
	spawnArgs.GetString( "spawnfunc", NULL, &spawn );
	if ( spawn ) {
		const function_t *func = program.FindFunction( spawn );
		if ( !func ) {
			Warning( "Could not spawn '%s'.  Script function '%s' not found%s.", classname, spawn, error.c_str() );
			return false;
		}
		idThread *thread = new idThread( func );
		thread->DelayedStart( 0 );
		return true;
	}

	Warning( "%s doesn't include a spawnfunc or spawnclass%s.", classname, error.c_str() );
	return false;
}

/*
================
idGameLocal::FindEntityDef
================
*/
const idDeclEntityDef *idGameLocal::FindEntityDef( const char *name, bool makeDefault ) const {
	const idDecl *decl = NULL;
	if ( isMultiplayer ) {
		decl = declManager->FindType( DECL_ENTITYDEF, va( "%s_mp", name ), false );
	}
	if ( !decl ) {
		decl = declManager->FindType( DECL_ENTITYDEF, name, makeDefault );
	}
	return static_cast<const idDeclEntityDef *>( decl );
}

/*
================
idGameLocal::FindEntityDefDict
================
*/
const idDict *idGameLocal::FindEntityDefDict( const char *name, bool makeDefault ) const {
	const idDeclEntityDef *decl = FindEntityDef( name, makeDefault );
	return decl ? &decl->dict : NULL;
}

/*
================
idGameLocal::InhibitEntitySpawn
================
*/
bool idGameLocal::InhibitEntitySpawn( idDict &spawnArgs ) {
	
	bool result = false;

	if ( isMultiplayer ) {
		spawnArgs.GetBool( "not_multiplayer", "0", result );
	} else if ( g_skill.GetInteger() == 0 ) {
		spawnArgs.GetBool( "not_easy", "0", result );
	} else if ( g_skill.GetInteger() == 1 ) {
		spawnArgs.GetBool( "not_medium", "0", result );
	} else {
		spawnArgs.GetBool( "not_hard", "0", result );
#ifdef _D3XP
		if ( !result && g_skill.GetInteger() == 3 ) {
			spawnArgs.GetBool( "not_nightmare", "0", result );
		}
#endif
	}


	const char *name;
#ifndef ID_DEMO_BUILD
	if ( g_skill.GetInteger() == 3 ) { 
		name = spawnArgs.GetString( "classname" );
		// _D3XP :: remove moveable medkit packs also
		if ( idStr::Icmp( name, "item_medkit" ) == 0 || idStr::Icmp( name, "item_medkit_small" ) == 0 ||
			 idStr::Icmp( name, "moveable_item_medkit" ) == 0 || idStr::Icmp( name, "moveable_item_medkit_small" ) == 0 ) {

			result = true;
		}
	}
#endif

	if ( gameLocal.isMultiplayer ) {
		name = spawnArgs.GetString( "classname" );
		if ( idStr::Icmp( name, "weapon_bfg" ) == 0 || idStr::Icmp( name, "weapon_soulcube" ) == 0 ) {
			result = true;
		}
	}

	return result;
}

/*
================
idGameLocal::SetSkill
================
*/
void idGameLocal::SetSkill( int value ) {
	int skill_level;

	if ( value < 0 ) {
		skill_level = 0;
	} else if ( value > 3 ) {
		skill_level = 3;
	} else {
		skill_level = value;
	}

	g_skill.SetInteger( skill_level );
}

/*
==============
idGameLocal::GameState

Used to allow entities to know if they're being spawned during the initial spawn.
==============
*/
gameState_t	idGameLocal::GameState( void ) const {
	return gamestate;
}

/*
==============
idGameLocal::SpawnMapEntities

Parses textual entity definitions out of an entstring and spawns gentities.
==============
*/
void idGameLocal::SpawnMapEntities( void ) {
	int			i;
	int			num;
	int			inhibit;
	idMapEntity	*mapEnt;
	int			numEntities;
	idDict		args;

	Printf( "Spawning entities\n" );

	if ( mapFile == NULL ) {
		Printf("No mapfile present\n");
		return;
	}

	SetSkill( g_skill.GetInteger() );

	numEntities = mapFile->GetNumEntities();
	if ( numEntities == 0 ) {
		Error( "...no entities" );
	}

	// the worldspawn is a special that performs any global setup
	// needed by a level
	mapEnt = mapFile->GetEntity( 0 );
	args = mapEnt->epairs;
	args.SetInt( "spawn_entnum", ENTITYNUM_WORLD );
	if ( !SpawnEntityDef( args ) || !entities[ ENTITYNUM_WORLD ] || !entities[ ENTITYNUM_WORLD ]->IsType( idWorldspawn::Type ) ) {
		Error( "Problem spawning world entity" );
	}

	num = 1;
	inhibit = 0;

	for ( i = 1 ; i < numEntities ; i++ ) {
		mapEnt = mapFile->GetEntity( i );
		args = mapEnt->epairs;

		if ( !InhibitEntitySpawn( args ) ) {
			// precache any media specified in the map entity
			CacheDictionaryMedia( &args );

			SpawnEntityDef( args );
			num++;
		} else {
			inhibit++;
		}
	}

	Printf( "...%i entities spawned, %i inhibited\n\n", num, inhibit );
}

/*
================
idGameLocal::AddEntityToHash
================
*/
void idGameLocal::AddEntityToHash( const char *name, idEntity *ent ) {
	if ( FindEntity( name ) ) {
		Error( "Multiple entities named '%s' (at: %f %f %f)", name, ent->spawnArgs.GetVector("origin").x, ent->spawnArgs.GetVector("origin").y, ent->spawnArgs.GetVector("origin").z );
	}
	entityHash.Add( entityHash.GenerateKey( name, true ), ent->entityNumber );
}

/*
================
idGameLocal::RemoveEntityFromHash
================
*/
bool idGameLocal::RemoveEntityFromHash( const char *name, idEntity *ent ) {
	int hash, i;

	hash = entityHash.GenerateKey( name, true );
	for ( i = entityHash.First( hash ); i != -1; i = entityHash.Next( i ) ) {
		if ( entities[i] && entities[i] == ent && entities[i]->name.Icmp( name ) == 0 ) {
			entityHash.Remove( hash, i );
			return true;
		}
	}
	return false;
}

/*
================
idGameLocal::GetTargets
================
*/
int idGameLocal::GetTargets( const idDict &args, idList< idEntityPtr<idEntity> > &list, const char *ref ) const {
	int i, num, refLength;
	const idKeyValue *arg;
	idEntity *ent;

	list.Clear();

	refLength = strlen( ref );
	num = args.GetNumKeyVals();
	for( i = 0; i < num; i++ ) {

		arg = args.GetKeyVal( i );
		if ( arg->GetKey().Icmpn( ref, refLength ) == 0 ) {

			ent = FindEntity( arg->GetValue() );
			if ( ent ) {
				idEntityPtr<idEntity> &entityPtr = list.Alloc();
                entityPtr = ent;
			}
		}
	}

	return list.Num();
}

/*
=============
idGameLocal::GetTraceEntity

returns the master entity of a trace.  for example, if the trace entity is the player's head, it will return the player.
=============
*/
idEntity *idGameLocal::GetTraceEntity( const trace_t &trace ) const {
	idEntity *master;

	if ( !entities[ trace.c.entityNum ] ) {
		return NULL;
	}
	master = entities[ trace.c.entityNum ]->GetBindMaster();
	if ( master ) {
		return master;
	}
	return entities[ trace.c.entityNum ];
}

/*
=============
idGameLocal::ArgCompletion_EntityName

Argument completion for entity names
=============
*/
void idGameLocal::ArgCompletion_EntityName( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	int i;

	for( i = 0; i < gameLocal.num_entities; i++ ) {
		if ( gameLocal.entities[ i ] ) {
			callback( va( "%s %s", args.Argv( 0 ), gameLocal.entities[ i ]->name.c_str() ) );
		}
	}
}

/*
=============
idGameLocal::FindEntity

Returns the entity whose name matches the specified string.
=============
*/
idEntity *idGameLocal::FindEntity( const char *name ) const {
	int hash, i;

	hash = entityHash.GenerateKey( name, true );
	for ( i = entityHash.First( hash ); i != -1; i = entityHash.Next( i ) ) {
		if ( entities[i] && entities[i]->name.Icmp( name ) == 0 ) {
			return entities[i];
		}
	}

	return NULL;
}

/*
=============
idGameLocal::FindEntityUsingDef

Searches all active entities for the next one using the specified entityDef.

Searches beginning at the entity after from, or the beginning if NULL
NULL will be returned if the end of the list is reached.
=============
*/
idEntity *idGameLocal::FindEntityUsingDef( idEntity *from, const char *match ) const {
	idEntity	*ent;

	if ( !from ) {
		ent = spawnedEntities.Next();
	} else {
		ent = from->spawnNode.Next();
	}

	for ( ; ent != NULL; ent = ent->spawnNode.Next() ) {
		assert( ent );
		if ( idStr::Icmp( ent->GetEntityDefName(), match ) == 0 ) {
			return ent;
		}
	}

	return NULL;
}

/*
=============
idGameLocal::FindTraceEntity

Searches all active entities for the closest ( to start ) match that intersects
the line start,end
=============
*/
idEntity *idGameLocal::FindTraceEntity( idVec3 start, idVec3 end, const idTypeInfo &c, const idEntity *skip ) const {
	idEntity *ent;
	idEntity *bestEnt;
	float scale;
	float bestScale;
	idBounds b;

	bestEnt = NULL;
	bestScale = 1.0f;
	for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( ent->IsType( c ) && ent != skip ) {
			b = ent->GetPhysics()->GetAbsBounds().Expand( 16 );
			if ( b.RayIntersection( start, end-start, scale ) ) {
				if ( scale >= 0.0f && scale < bestScale ) {
					bestEnt = ent;
					bestScale = scale;
				}
			}
		}
	}

	return bestEnt;
}

/*
================
idGameLocal::EntitiesWithinRadius
================
*/
int idGameLocal::EntitiesWithinRadius( const idVec3 org, float radius, idEntity **entityList, int maxCount ) const {
	idEntity *ent;
	idBounds bo( org );
	int entCount = 0;

	bo.ExpandSelf( radius );
	for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( ent->GetPhysics()->GetAbsBounds().IntersectsBounds( bo ) ) {
			entityList[entCount++] = ent;
		}
	}

	return entCount;
}

/*
=================
idGameLocal::KillBox

Kills all entities that would touch the proposed new positioning of ent. The ent itself will not being killed.
Checks if player entities are in the teleporter, and marks them to die at teleport exit instead of immediately.
If catch_teleport, this only marks teleport players for death on exit
=================
*/
void idGameLocal::KillBox( idEntity *ent, bool catch_teleport ) {
	int			i;
	int			num;
	idEntity *	hit;
	idClipModel *cm;
	idClipModel *clipModels[ MAX_GENTITIES ];
	idPhysics	*phys;

	phys = ent->GetPhysics();
	if ( !phys->GetNumClipModels() ) {
		return;
	}

	num = clip.ClipModelsTouchingBounds( phys->GetAbsBounds(), phys->GetClipMask(), clipModels, MAX_GENTITIES );
	for ( i = 0; i < num; i++ ) {
		cm = clipModels[ i ];

		// don't check render entities
		if ( cm->IsRenderModel() ) {
			continue;
		}

		hit = cm->GetEntity();
		if ( ( hit == ent ) || !hit->fl.takedamage ) {
			continue;
		}

		if ( !phys->ClipContents( cm ) ) {
			continue;
		}

		// nail it
		if ( hit->IsType( idPlayer::Type ) && static_cast< idPlayer * >( hit )->IsInTeleport() ) {
			static_cast< idPlayer * >( hit )->TeleportDeath( ent->entityNumber );
		} else if ( !catch_teleport ) {
			hit->Damage( ent, ent, vec3_origin, "damage_telefrag", 1.0f, INVALID_JOINT );
		}

		if ( !gameLocal.isMultiplayer ) {
			// let the mapper know about it
			Warning( "'%s' telefragged '%s'", ent->name.c_str(), hit->name.c_str() );
		}
	}
}

/*
================
idGameLocal::RequirementMet
================
*/
bool idGameLocal::RequirementMet( idEntity *activator, const idStr &requires, int removeItem ) {
	if ( requires.Length() ) {
		if ( activator->IsType( idPlayer::Type ) ) {
			idPlayer *player = static_cast<idPlayer *>(activator);
			idDict *item = player->FindInventoryItem( requires );
			if ( item ) {
				if ( removeItem ) {
					player->RemoveInventoryItem( item );
				}
				return true;
			} else {
				return false;
			}
		}
	}

	return true;
}

/*
============
idGameLocal::AlertAI
============
*/
void idGameLocal::AlertAI( idEntity *ent ) {
	if ( ent && ent->IsType( idActor::Type ) ) {
		// alert them for the next frame
		lastAIAlertTime = time + msec;
		lastAIAlertEntity = static_cast<idActor *>( ent );
	}
}

/*
============
idGameLocal::GetAlertEntity
============
*/
idActor *idGameLocal::GetAlertEntity( void ) {
#ifdef _D3XP
	int timeGroup = 0;
	if ( lastAIAlertTime && lastAIAlertEntity.GetEntity() ) {
		timeGroup = lastAIAlertEntity.GetEntity()->timeGroup;
	}
	SetTimeState ts( timeGroup );
#endif

	if ( lastAIAlertTime >= time ) {
		return lastAIAlertEntity.GetEntity();
	}

	return NULL;
}

/*
============
idGameLocal::RadiusDamage
============
*/
void idGameLocal::RadiusDamage( const idVec3 &origin, idEntity *inflictor, idEntity *attacker, idEntity *ignoreDamage, idEntity *ignorePush, const char *damageDefName, float dmgPower ) {
	float		dist, damageScale, attackerDamageScale, attackerPushScale;
	idEntity *	ent;
	idEntity *	entityList[ MAX_GENTITIES ];
	int			numListedEntities;
	idBounds	bounds;
	idVec3 		v, damagePoint, dir;
	int			i, e, damage, radius, push;

	const idDict *damageDef = FindEntityDefDict( damageDefName, false );
	if ( !damageDef ) {
		Warning( "Unknown damageDef '%s'", damageDefName );
		return;
	}

	damageDef->GetInt( "damage", "20", damage );
	damageDef->GetInt( "radius", "50", radius );
	damageDef->GetInt( "push", va( "%d", damage * 100 ), push );
	damageDef->GetFloat( "attackerDamageScale", "0.5", attackerDamageScale );
	damageDef->GetFloat( "attackerPushScale", "0", attackerPushScale );

	if ( radius < 1 ) {
		radius = 1;
	}

	bounds = idBounds( origin ).Expand( radius );

	// get all entities touching the bounds
	numListedEntities = clip.EntitiesTouchingBounds( bounds, -1, entityList, MAX_GENTITIES );

	if ( inflictor && inflictor->IsType( idAFAttachment::Type ) ) {
		inflictor = static_cast<idAFAttachment*>(inflictor)->GetBody();
	}
	if ( attacker && attacker->IsType( idAFAttachment::Type ) ) {
		attacker = static_cast<idAFAttachment*>(attacker)->GetBody();
	}
	if ( ignoreDamage && ignoreDamage->IsType( idAFAttachment::Type ) ) {
		ignoreDamage = static_cast<idAFAttachment*>(ignoreDamage)->GetBody();
	}

	// apply damage to the entities
	for ( e = 0; e < numListedEntities; e++ ) {
		ent = entityList[ e ];
		assert( ent );

		if ( !ent->fl.takedamage ) {
			continue;
		}

		if ( ent == inflictor || ( ent->IsType( idAFAttachment::Type ) && static_cast<idAFAttachment*>(ent)->GetBody() == inflictor ) ) {
			continue;
		}

		if ( ent == ignoreDamage || ( ent->IsType( idAFAttachment::Type ) && static_cast<idAFAttachment*>(ent)->GetBody() == ignoreDamage ) ) {
			continue;
		}

		// don't damage a dead player
		if ( isMultiplayer && ent->entityNumber < MAX_CLIENTS && ent->IsType( idPlayer::Type ) && static_cast< idPlayer * >( ent )->health < 0 ) {
			continue;
		}

		// find the distance from the edge of the bounding box
		for ( i = 0; i < 3; i++ ) {
			if ( origin[ i ] < ent->GetPhysics()->GetAbsBounds()[0][ i ] ) {
				v[ i ] = ent->GetPhysics()->GetAbsBounds()[0][ i ] - origin[ i ];
			} else if ( origin[ i ] > ent->GetPhysics()->GetAbsBounds()[1][ i ] ) {
				v[ i ] = origin[ i ] - ent->GetPhysics()->GetAbsBounds()[1][ i ];
			} else {
				v[ i ] = 0;
			}
		}

		dist = v.Length();
		if ( dist >= radius ) {
			continue;
		}

		if ( ent->CanDamage( origin, damagePoint ) ) {
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir = ent->GetPhysics()->GetOrigin() - origin;
			dir[ 2 ] += 24;

			// get the damage scale
			damageScale = dmgPower * ( 1.0f - dist / radius );
			if ( ent == attacker || ( ent->IsType( idAFAttachment::Type ) && static_cast<idAFAttachment*>(ent)->GetBody() == attacker ) ) {
				damageScale *= attackerDamageScale;
			}

			ent->Damage( inflictor, attacker, dir, damageDefName, damageScale, INVALID_JOINT );
		} 
	}

	// push physics objects
	if ( push ) {
		RadiusPush( origin, radius, push * dmgPower, attacker, ignorePush, attackerPushScale, false );
	}
}

/*
==============
idGameLocal::RadiusPush
==============
*/
void idGameLocal::RadiusPush( const idVec3 &origin, const float radius, const float push, const idEntity *inflictor, const idEntity *ignore, float inflictorScale, const bool quake ) {
	int i, numListedClipModels;
	idClipModel *clipModel;
	idClipModel *clipModelList[ MAX_GENTITIES ];
	idVec3 dir;
	idBounds bounds;
	modelTrace_t result;
	idEntity *ent;
	float scale;

	dir.Set( 0.0f, 0.0f, 1.0f );

	bounds = idBounds( origin ).Expand( radius );

	// get all clip models touching the bounds
	numListedClipModels = clip.ClipModelsTouchingBounds( bounds, -1, clipModelList, MAX_GENTITIES );

	if ( inflictor && inflictor->IsType( idAFAttachment::Type ) ) {
		inflictor = static_cast<const idAFAttachment*>(inflictor)->GetBody();
	}
	if ( ignore && ignore->IsType( idAFAttachment::Type ) ) {
		ignore = static_cast<const idAFAttachment*>(ignore)->GetBody();
	}

	// apply impact to all the clip models through their associated physics objects
	for ( i = 0; i < numListedClipModels; i++ ) {

		clipModel = clipModelList[i];

		// never push render models
		if ( clipModel->IsRenderModel() ) {
			continue;
		}

		ent = clipModel->GetEntity();

		// never push projectiles
		if ( ent->IsType( idProjectile::Type ) ) {
			continue;
		}

		// players use "knockback" in idPlayer::Damage
		if ( ent->IsType( idPlayer::Type ) && !quake ) {
			continue;
		}

		// don't push the ignore entity
		if ( ent == ignore || ( ent->IsType( idAFAttachment::Type ) && static_cast<idAFAttachment*>(ent)->GetBody() == ignore ) ) {
			continue;
		}

		if ( gameRenderWorld->FastWorldTrace( result, origin, clipModel->GetOrigin() ) ) {
			continue;
		}

		// scale the push for the inflictor
		if ( ent == inflictor || ( ent->IsType( idAFAttachment::Type ) && static_cast<idAFAttachment*>(ent)->GetBody() == inflictor ) ) {
			scale = inflictorScale;
		} else {
			scale = 1.0f;
		}

		if ( quake ) {
			clipModel->GetEntity()->ApplyImpulse( world, clipModel->GetId(), clipModel->GetOrigin(), scale * push * dir );
		} else {
			RadiusPushClipModel( origin, scale * push, clipModel );
		}
	}
}

/*
==============
idGameLocal::RadiusPushClipModel
==============
*/
void idGameLocal::RadiusPushClipModel( const idVec3 &origin, const float push, const idClipModel *clipModel ) {
	int i, j;
	float dot, dist, area;
	const idTraceModel *trm;
	const traceModelPoly_t *poly;
	idFixedWinding w;
	idVec3 v, localOrigin, center, impulse;

	trm = clipModel->GetTraceModel();
	if ( !trm || 1 ) {
		impulse = clipModel->GetAbsBounds().GetCenter() - origin;
		impulse.Normalize();
		impulse.z += 1.0f;
		clipModel->GetEntity()->ApplyImpulse( world, clipModel->GetId(), clipModel->GetOrigin(), push * impulse );
		return;
	}

	localOrigin = ( origin - clipModel->GetOrigin() ) * clipModel->GetAxis().Transpose();
	for ( i = 0; i < trm->numPolys; i++ ) {
		poly = &trm->polys[i];

		center.Zero();
		for ( j = 0; j < poly->numEdges; j++ ) {
			v = trm->verts[ trm->edges[ abs(poly->edges[j]) ].v[ INTSIGNBITSET( poly->edges[j] ) ] ];
			center += v;
			v -= localOrigin;
			v.NormalizeFast();	// project point on a unit sphere
			w.AddPoint( v );
		}
		center /= poly->numEdges;
		v = center - localOrigin;
		dist = v.NormalizeFast();
		dot = v * poly->normal;
		if ( dot > 0.0f ) {
			continue;
		}
		area = w.GetArea();
		// impulse in polygon normal direction
		impulse = poly->normal * clipModel->GetAxis();
		// always push up for nicer effect
		impulse.z -= 1.0f;
		// scale impulse based on visible surface area and polygon angle
		impulse *= push * ( dot * area * ( 1.0f / ( 4.0f * idMath::PI ) ) );
		// scale away distance for nicer effect
		impulse *= ( dist * 2.0f );
		// impulse is applied to the center of the polygon
		center = clipModel->GetOrigin() + center * clipModel->GetAxis();

		clipModel->GetEntity()->ApplyImpulse( world, clipModel->GetId(), center, impulse );
	}
}

/*
===============
idGameLocal::ProjectDecal
===============
*/
void idGameLocal::ProjectDecal( const idVec3 &origin, const idVec3 &dir, float depth, bool parallel, float size, const char *material, float angle ) {
	float s, c;
	idMat3 axis, axistemp;
	idFixedWinding winding;
	idVec3 windingOrigin, projectionOrigin;

	static idVec3 decalWinding[4] = {
		idVec3(  1.0f,  1.0f, 0.0f ),
		idVec3( -1.0f,  1.0f, 0.0f ),
		idVec3( -1.0f, -1.0f, 0.0f ),
		idVec3(  1.0f, -1.0f, 0.0f )
	};

	if ( !g_decals.GetBool() ) {
		return;
	}

	// randomly rotate the decal winding
	idMath::SinCos16( ( angle ) ? angle : random.RandomFloat() * idMath::TWO_PI, s, c );

	// winding orientation
	axis[2] = dir;
	axis[2].Normalize();
	axis[2].NormalVectors( axistemp[0], axistemp[1] );
	axis[0] = axistemp[ 0 ] * c + axistemp[ 1 ] * -s;
	axis[1] = axistemp[ 0 ] * -s + axistemp[ 1 ] * -c;

	windingOrigin = origin + depth * axis[2];
	if ( parallel ) {
		projectionOrigin = origin - depth * axis[2];
	} else {
		projectionOrigin = origin;
	}

	size *= 0.5f;

	winding.Clear();
	winding += idVec5( windingOrigin + ( axis * decalWinding[0] ) * size, idVec2( 1, 1 ) );
	winding += idVec5( windingOrigin + ( axis * decalWinding[1] ) * size, idVec2( 0, 1 ) );
	winding += idVec5( windingOrigin + ( axis * decalWinding[2] ) * size, idVec2( 0, 0 ) );
	winding += idVec5( windingOrigin + ( axis * decalWinding[3] ) * size, idVec2( 1, 0 ) );
	gameRenderWorld->ProjectDecalOntoWorld( winding, projectionOrigin, parallel, depth * 0.5f, declManager->FindMaterial( material ), gameLocal.slow.time /* _D3XP */ );
}

/*
==============
idGameLocal::BloodSplat
==============
*/
void idGameLocal::BloodSplat( const idVec3 &origin, const idVec3 &dir, float size, const char *material ) {
	float halfSize = size * 0.5f;
	idVec3 verts[] = {	idVec3( 0.0f, +halfSize, +halfSize ),
						idVec3( 0.0f, +halfSize, -halfSize ),
						idVec3( 0.0f, -halfSize, -halfSize ),
						idVec3( 0.0f, -halfSize, +halfSize ) };
	idTraceModel trm;
	idClipModel mdl;
	trace_t results;

	// FIXME: get from damage def
	if ( !g_bloodEffects.GetBool() ) {
		return;
	}

	size = halfSize + random.RandomFloat() * halfSize;
	trm.SetupPolygon( verts, 4 );
	mdl.LoadModel( trm );
	clip.Translation( results, origin, origin + dir * 64.0f, &mdl, mat3_identity, CONTENTS_SOLID, NULL );
	ProjectDecal( results.endpos, dir, 2.0f * size, true, size, material );
}

/*
=============
idGameLocal::SetCamera
=============
*/
void idGameLocal::SetCamera( idCamera *cam ) {
	int i;
	idEntity *ent;
	idAI *ai;

	// this should fix going into a cinematic when dead.. rare but happens
	idPlayer *client = GetLocalPlayer();
	if ( client->health <= 0 || client->AI_DEAD ) {
		return;
	}

	camera = cam;
	if ( camera ) {
		inCinematic = true;

		if ( skipCinematic && camera->spawnArgs.GetBool( "disconnect" ) ) {
			camera->spawnArgs.SetBool( "disconnect", false );
			cvarSystem->SetCVarFloat( "r_znear", 3.0f );
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "disconnect\n" );
			skipCinematic = false;
			return;
		}

		if ( time > cinematicStopTime ) {
			cinematicSkipTime = time + CINEMATIC_SKIP_DELAY;
		}

		// set r_znear so that transitioning into/out of the player's head doesn't clip through the view
		cvarSystem->SetCVarFloat( "r_znear", 1.0f );
		
		// hide all the player models
		for( i = 0; i < numClients; i++ ) {
			if ( entities[ i ] ) {
				client = static_cast< idPlayer* >( entities[ i ] );
				client->EnterCinematic();
			}
		}

		if ( !cam->spawnArgs.GetBool( "ignore_enemies" ) ) {
			// kill any active monsters that are enemies of the player
			for ( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
				if ( ent->cinematic || ent->fl.isDormant ) {
					// only kill entities that aren't needed for cinematics and aren't dormant
					continue;
				}
				
				if ( ent->IsType( idAI::Type ) ) {
					ai = static_cast<idAI *>( ent );
					if ( !ai->GetEnemy() || !ai->IsActive() ) {
						// no enemy, or inactive, so probably safe to ignore
						continue;
					}
				} else if ( ent->IsType( idProjectile::Type ) ) {
					// remove all projectiles
				} else if ( ent->spawnArgs.GetBool( "cinematic_remove" ) ) {
					// remove anything marked to be removed during cinematics
				} else {
					// ignore everything else
					continue;
				}

				// remove it
				DPrintf( "removing '%s' for cinematic\n", ent->GetName() );
				ent->PostEventMS( &EV_Remove, 0 );
			}
		}

	} else {
		inCinematic = false;
		cinematicStopTime = time + msec;

		// restore r_znear
		cvarSystem->SetCVarFloat( "r_znear", 3.0f );

		// show all the player models
		for( i = 0; i < numClients; i++ ) {
			if ( entities[ i ] ) {
				idPlayer *client = static_cast< idPlayer* >( entities[ i ] );
				client->ExitCinematic();
			}
		}
	}
}

/*
=============
idGameLocal::GetCamera
=============
*/
idCamera *idGameLocal::GetCamera( void ) const {
	return camera;
}

/*
=============
idGameLocal::SkipCinematic
=============
*/
bool idGameLocal::SkipCinematic( void ) {
	if ( camera ) {
		if ( camera->spawnArgs.GetBool( "disconnect" ) ) {
			camera->spawnArgs.SetBool( "disconnect", false );
			cvarSystem->SetCVarFloat( "r_znear", 3.0f );
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "disconnect\n" );
			skipCinematic = false;
			return false;
		}

		if ( camera->spawnArgs.GetBool( "instantSkip" ) ) {
			camera->Stop();
			return false;
		}
	}

	soundSystem->SetMute( true );
	if ( !skipCinematic ) {
		skipCinematic = true;
		cinematicMaxSkipTime = gameLocal.time + SEC2MS( g_cinematicMaxSkipTime.GetFloat() );
	}

	return true;
}


/*
======================
idGameLocal::SpreadLocations

Now that everything has been spawned, associate areas with location entities
======================
*/
void idGameLocal::SpreadLocations() {
	idEntity *ent;

	// allocate the area table
	int	numAreas = gameRenderWorld->NumAreas();
	locationEntities = new idLocationEntity *[ numAreas ];
	memset( locationEntities, 0, numAreas * sizeof( *locationEntities ) );

	// for each location entity, make pointers from every area it touches
	for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( !ent->IsType( idLocationEntity::Type ) ) {
			continue;
		}
		idVec3	point = ent->spawnArgs.GetVector( "origin" );
		int areaNum = gameRenderWorld->PointInArea( point );
		if ( areaNum < 0 ) {
			Printf( "SpreadLocations: location '%s' is not in a valid area\n", ent->spawnArgs.GetString( "name" ) );
			continue;
		}
		if ( areaNum >= numAreas ) {
			Error( "idGameLocal::SpreadLocations: areaNum >= gameRenderWorld->NumAreas()" );
		}
		if ( locationEntities[areaNum] ) {
			Warning( "location entity '%s' overlaps '%s'", ent->spawnArgs.GetString( "name" ),
				locationEntities[areaNum]->spawnArgs.GetString( "name" ) );
			continue;
		}
		locationEntities[areaNum] = static_cast<idLocationEntity *>(ent);

		// spread to all other connected areas
		for ( int i = 0 ; i < numAreas ; i++ ) {
			if ( i == areaNum ) {
				continue;
			}
			if ( gameRenderWorld->AreasAreConnected( areaNum, i, PS_BLOCK_LOCATION ) ) {
				locationEntities[i] = static_cast<idLocationEntity *>(ent);
			}
		}
	}
}

/*
===================
idGameLocal::LocationForPoint

The player checks the location each frame to update the HUD text display
May return NULL
===================
*/
idLocationEntity *idGameLocal::LocationForPoint( const idVec3 &point ) {
	if ( !locationEntities ) {
		// before SpreadLocations() has been called
		return NULL;
	}

	int areaNum = gameRenderWorld->PointInArea( point );
	if ( areaNum < 0 ) {
		return NULL;
	}
	if ( areaNum >= gameRenderWorld->NumAreas() ) {
		Error( "idGameLocal::LocationForPoint: areaNum >= gameRenderWorld->NumAreas()" );
	}

	return locationEntities[ areaNum ];
}

/*
============
idGameLocal::SetPortalState
============
*/
void idGameLocal::SetPortalState( qhandle_t portal, int blockingBits ) {
	idBitMsg outMsg;
	byte msgBuf[ MAX_GAME_MESSAGE_SIZE ];

	if ( !gameLocal.isClient ) {
		outMsg.Init( msgBuf, sizeof( msgBuf ) );
		outMsg.WriteByte( GAME_RELIABLE_MESSAGE_PORTAL );
		outMsg.WriteLong( portal );
		outMsg.WriteBits( blockingBits, NUM_RENDER_PORTAL_BITS );
		networkSystem->ServerSendReliableMessage( -1, outMsg );
	}
	gameRenderWorld->SetPortalState( portal, blockingBits );
}

/*
============
idGameLocal::sortSpawnPoints
============
*/
int idGameLocal::sortSpawnPoints( const void *ptr1, const void *ptr2 ) {
	const spawnSpot_t *spot1 = static_cast<const spawnSpot_t *>( ptr1 );
	const spawnSpot_t *spot2 = static_cast<const spawnSpot_t *>( ptr2 );
	float diff;

	diff = spot1->dist - spot2->dist;
	if ( diff < 0.0f ) {
		return 1;
	} else if ( diff > 0.0f ) {
		return -1;
	} else {
		return 0;
	}
}

/*
===========
idGameLocal::RandomizeInitialSpawns
randomize the order of the initial spawns
prepare for a sequence of initial player spawns
============
*/
void idGameLocal::RandomizeInitialSpawns( void ) {
	spawnSpot_t	spot;
	int i, j;
#ifdef CTF
	int k;
#endif
    
	idEntity *ent;

	if ( !isMultiplayer || isClient ) {
		return;
	}
	spawnSpots.Clear();
	initialSpots.Clear();
#ifdef CTF
	teamSpawnSpots[0].Clear();
	teamSpawnSpots[1].Clear();
	teamInitialSpots[0].Clear();
	teamInitialSpots[1].Clear();
#endif
    
	spot.dist = 0;
	spot.ent = FindEntityUsingDef( NULL, "info_player_deathmatch" );
	while( spot.ent ) {
#ifdef CTF
		spot.ent->spawnArgs.GetInt( "team", "-1", spot.team );

		if ( mpGame.IsGametypeFlagBased() ) /* CTF */
		{
			if ( spot.team == 0 || spot.team == 1 )
				teamSpawnSpots[spot.team].Append( spot );
			else
				common->Warning( "info_player_deathmatch : invalid or no team attached to spawn point\n");
		}
#endif        
		spawnSpots.Append( spot );
		if ( spot.ent->spawnArgs.GetBool( "initial" ) ) {
#ifdef CTF
			if ( mpGame.IsGametypeFlagBased() ) /* CTF */
			{
				assert( spot.team == 0 || spot.team == 1 );
				teamInitialSpots[ spot.team ].Append( spot.ent );
			}
#endif
            
			initialSpots.Append( spot.ent );
		}
		spot.ent = FindEntityUsingDef( spot.ent, "info_player_deathmatch" );
	}

#ifdef CTF
	if ( mpGame.IsGametypeFlagBased() ) /* CTF */
	{
		if ( !teamSpawnSpots[0].Num() )
			common->Warning( "red team : no info_player_deathmatch in map" );
		if ( !teamSpawnSpots[1].Num() )
			common->Warning( "blue team : no info_player_deathmatch in map" );

		if ( !teamSpawnSpots[0].Num() || !teamSpawnSpots[1].Num() )
			return;
	}
#endif
    
	if ( !spawnSpots.Num() ) {
		common->Warning( "no info_player_deathmatch in map" );
		return;
	}

#ifdef CTF
	if ( mpGame.IsGametypeFlagBased() ) /* CTF */
	{
		common->Printf( "red team : %d spawns (%d initials)\n", teamSpawnSpots[ 0 ].Num(), teamInitialSpots[ 0 ].Num() );
		// if there are no initial spots in the map, consider they can all be used as initial
		if ( !teamInitialSpots[ 0 ].Num() ) {
			common->Warning( "red team : no info_player_deathmatch entities marked initial in map" );
			for ( i = 0; i < teamSpawnSpots[ 0 ].Num(); i++ ) {
				teamInitialSpots[ 0 ].Append( teamSpawnSpots[ 0 ][ i ].ent );
			}
		}

		common->Printf( "blue team : %d spawns (%d initials)\n", teamSpawnSpots[ 1 ].Num(), teamInitialSpots[ 1 ].Num() );
		// if there are no initial spots in the map, consider they can all be used as initial
		if ( !teamInitialSpots[ 1 ].Num() ) {
			common->Warning( "blue team : no info_player_deathmatch entities marked initial in map" );
			for ( i = 0; i < teamSpawnSpots[ 1 ].Num(); i++ ) {
				teamInitialSpots[ 1 ].Append( teamSpawnSpots[ 1 ][ i ].ent );
			}
		}
	}
#endif

    
	common->Printf( "%d spawns (%d initials)\n", spawnSpots.Num(), initialSpots.Num() );
	// if there are no initial spots in the map, consider they can all be used as initial
	if ( !initialSpots.Num() ) {
		common->Warning( "no info_player_deathmatch entities marked initial in map" );
		for ( i = 0; i < spawnSpots.Num(); i++ ) {
			initialSpots.Append( spawnSpots[ i ].ent );
		}
	}

#ifdef CTF
	for ( k = 0; k < 2; k++ )
	for ( i = 0; i < teamInitialSpots[ k ].Num(); i++ ) {
		j = random.RandomInt( teamInitialSpots[ k ].Num() );
		ent = teamInitialSpots[ k ][ i ];
		teamInitialSpots[ k ][ i ] = teamInitialSpots[ k ][ j ];
		teamInitialSpots[ k ][ j ] = ent;
	}
#endif
    
	for ( i = 0; i < initialSpots.Num(); i++ ) {
		j = random.RandomInt( initialSpots.Num() );
		ent = initialSpots[ i ];
		initialSpots[ i ] = initialSpots[ j ];
		initialSpots[ j ] = ent;
	}
	// reset the counter
	currentInitialSpot = 0;

#ifdef CTF
	teamCurrentInitialSpot[0] = 0;
	teamCurrentInitialSpot[1] = 0;
#endif
}

/*
===========
idGameLocal::SelectInitialSpawnPoint
spectators are spawned randomly anywhere
in-game clients are spawned based on distance to active players (randomized on the first half)
upon map restart, initial spawns are used (randomized ordered list of spawns flagged "initial")
  if there are more players than initial spots, overflow to regular spawning
============
*/
idEntity *idGameLocal::SelectInitialSpawnPoint( idPlayer *player ) {
	int				i, j, which;
	spawnSpot_t		spot;
	idVec3			pos;
	float			dist;
	bool			alone;

#ifdef CTF
	if ( !isMultiplayer || !spawnSpots.Num() || ( mpGame.IsGametypeFlagBased() && ( !teamSpawnSpots[0].Num() || !teamSpawnSpots[1].Num() ) ) ) { /* CTF */
#else
	if ( !isMultiplayer || !spawnSpots.Num() ) {
#endif
		spot.ent = FindEntityUsingDef( NULL, "info_player_start" );
		if ( !spot.ent ) {
			Error( "No info_player_start on map.\n" );
		}
		return spot.ent;
	}

#ifdef CTF
	bool useInitialSpots = false;
	if ( mpGame.IsGametypeFlagBased() ) { /* CTF */
		assert( player->team == 0 || player->team == 1 );
		useInitialSpots = player->useInitialSpawns && teamCurrentInitialSpot[ player->team ] < teamInitialSpots[ player->team ].Num();
	} else {
		useInitialSpots = player->useInitialSpawns && currentInitialSpot < initialSpots.Num();
	}
#endif
    
	if ( player->spectating ) {
		// plain random spot, don't bother
		return spawnSpots[ random.RandomInt( spawnSpots.Num() ) ].ent;
#ifdef CTF
	} else if ( useInitialSpots ) {
		if ( mpGame.IsGametypeFlagBased() ) { /* CTF */
			assert( player->team == 0 || player->team == 1 );
			player->useInitialSpawns = false;							// only use the initial spawn once
			return teamInitialSpots[ player->team ][ teamCurrentInitialSpot[ player->team ]++ ];
		}
		return initialSpots[ currentInitialSpot++ ];
#else
	} else if ( player->useInitialSpawns && currentInitialSpot < initialSpots.Num() ) {
		return initialSpots[ currentInitialSpot++ ];
#endif
	} else {
		// check if we are alone in map
		alone = true;
		for ( j = 0; j < MAX_CLIENTS; j++ ) {
			if ( entities[ j ] && entities[ j ] != player ) {
				alone = false;
				break;
			}
		}
		if ( alone ) {
#ifdef CTF
			if ( mpGame.IsGametypeFlagBased() ) /* CTF */
			{
				assert( player->team == 0 || player->team == 1 );
				return teamSpawnSpots[ player->team ][ random.RandomInt( teamSpawnSpots[ player->team ].Num() ) ].ent;
			}
#endif
			// don't do distance-based
			return spawnSpots[ random.RandomInt( spawnSpots.Num() ) ].ent;
		}

#ifdef CTF
		if ( mpGame.IsGametypeFlagBased() ) /* CTF */
		{
			// TODO : make as reusable method, same code as below
			int team = player->team;
			assert( team == 0 || team == 1 );

			// find the distance to the closest active player for each spawn spot
			for( i = 0; i < teamSpawnSpots[ team ].Num(); i++ ) {
				pos = teamSpawnSpots[ team ][ i ].ent->GetPhysics()->GetOrigin();

				// skip initial spawn points for CTF
				if ( teamSpawnSpots[ team ][ i ].ent->spawnArgs.GetBool("initial") ) {
					teamSpawnSpots[ team ][ i ].dist = 0x0;
					continue;
				}

				teamSpawnSpots[ team ][ i ].dist = 0x7fffffff;

				for( j = 0; j < MAX_CLIENTS; j++ ) {
					if ( !entities[ j ] || !entities[ j ]->IsType( idPlayer::Type )
						|| entities[ j ] == player
						|| static_cast< idPlayer * >( entities[ j ] )->spectating ) {
						continue;
					}
					
					dist = ( pos - entities[ j ]->GetPhysics()->GetOrigin() ).LengthSqr();
					if ( dist < teamSpawnSpots[ team ][ i ].dist ) {
						teamSpawnSpots[ team ][ i ].dist = dist;
					}
				}
			}

			// sort the list
			qsort( ( void * )teamSpawnSpots[ team ].Ptr(), teamSpawnSpots[ team ].Num(), sizeof( spawnSpot_t ), ( int (*)(const void *, const void *) )sortSpawnPoints );

			// choose a random one in the top half
			which = random.RandomInt( teamSpawnSpots[ team ].Num() / 2 );
			spot = teamSpawnSpots[ team ][ which ];
//			assert( teamSpawnSpots[ team ][ which ].dist != 0 );

			return spot.ent;
		}
#endif
        
		// find the distance to the closest active player for each spawn spot
		for( i = 0; i < spawnSpots.Num(); i++ ) {
			pos = spawnSpots[ i ].ent->GetPhysics()->GetOrigin();
			spawnSpots[ i ].dist = 0x7fffffff;
			for( j = 0; j < MAX_CLIENTS; j++ ) {
				if ( !entities[ j ] || !entities[ j ]->IsType( idPlayer::Type )
					|| entities[ j ] == player
					|| static_cast< idPlayer * >( entities[ j ] )->spectating ) {
					continue;
				}
				
				dist = ( pos - entities[ j ]->GetPhysics()->GetOrigin() ).LengthSqr();
				if ( dist < spawnSpots[ i ].dist ) {
					spawnSpots[ i ].dist = dist;
				}
			}
		}

		// sort the list
		qsort( ( void * )spawnSpots.Ptr(), spawnSpots.Num(), sizeof( spawnSpot_t ), ( int (*)(const void *, const void *) )sortSpawnPoints );

		// choose a random one in the top half
		which = random.RandomInt( spawnSpots.Num() / 2 );
		spot = spawnSpots[ which ];
	}
	return spot.ent;
}

/*
================
idGameLocal::UpdateServerInfoFlags
================
*/
void idGameLocal::UpdateServerInfoFlags() {
	gameType = GAME_SP;
	if ( ( idStr::Icmp( serverInfo.GetString( "si_gameType" ), "deathmatch" ) == 0 ) ) {
		gameType = GAME_DM;
	} else if ( ( idStr::Icmp( serverInfo.GetString( "si_gameType" ), "Tourney" ) == 0 ) ) {
		gameType = GAME_TOURNEY;
	} else if ( ( idStr::Icmp( serverInfo.GetString( "si_gameType" ), "Team DM" ) == 0 ) ) {
		gameType = GAME_TDM;
	} else if ( ( idStr::Icmp( serverInfo.GetString( "si_gameType" ), "Last Man" ) == 0 ) ) {
		gameType = GAME_LASTMAN;
	}
#ifdef CTF
	else if ( ( idStr::Icmp( serverInfo.GetString( "si_gameType" ), "CTF" ) == 0 ) ) {
		gameType = GAME_CTF;
	}
#endif
    
	if ( gameType == GAME_LASTMAN ) {
		if ( !serverInfo.GetInt( "si_warmup" ) ) {
			common->Warning( "Last Man Standing - forcing warmup on" );
			serverInfo.SetInt( "si_warmup", 1 );
		}
		if ( serverInfo.GetInt( "si_fraglimit" ) <= 0 ) {
			common->Warning( "Last Man Standing - setting fraglimit 1" );
			serverInfo.SetInt( "si_fraglimit", 1 );
		}
	}
}


/*
================
idGameLocal::SetGlobalMaterial
================
*/
void idGameLocal::SetGlobalMaterial( const idMaterial *mat ) {
	globalMaterial = mat;
}

/*
================
idGameLocal::GetGlobalMaterial
================
*/
const idMaterial *idGameLocal::GetGlobalMaterial() {
	return globalMaterial;
}

/*
================
idGameLocal::GetSpawnId
================
*/
int idGameLocal::GetSpawnId( const idEntity* ent ) const {
	return ( gameLocal.spawnIds[ ent->entityNumber ] << GENTITYNUM_BITS ) | ent->entityNumber;
}

/*
================
idGameLocal::ThrottleUserInfo
================
*/
void idGameLocal::ThrottleUserInfo( void ) {
	mpGame.ThrottleUserInfo();
}

#ifdef _D3XP
/*
=================
idPlayer::SetPortalSkyEnt
=================
*/
void idGameLocal::SetPortalSkyEnt( idEntity *ent ) {
	portalSkyEnt = ent;
}

/*
=================
idPlayer::IsPortalSkyAcive
=================
*/
bool idGameLocal::IsPortalSkyAcive() {
	return portalSkyActive;
}

/*
===========
idGameLocal::SelectTimeGroup
============
*/
void idGameLocal::SelectTimeGroup( int timeGroup ) {
	int i = 0;

	if ( timeGroup ) {
		fast.Get( time, previousTime, msec, framenum, realClientTime );
	} else {
		slow.Get( time, previousTime, msec, framenum, realClientTime );
	}
}

/*
===========
idGameLocal::GetTimeGroupTime
============
*/
int idGameLocal::GetTimeGroupTime( int timeGroup ) {
	if ( timeGroup ) {
		return fast.time;
	} else {
		return slow.time;
	}
}

/*
===============
idGameLocal::GetBestGameType
===============
*/
void idGameLocal::GetBestGameType( const char* map, const char* gametype, char buf[ MAX_STRING_CHARS ] ) {
	idStr aux = mpGame.GetBestGametype( map, gametype );
	strncpy( buf, aux.c_str(), MAX_STRING_CHARS );
	buf[ MAX_STRING_CHARS - 1 ] = '\0';
}

/*
===========
idGameLocal::ComputeSlowMsec
============
*/
void idGameLocal::ComputeSlowMsec() {
	idPlayer *player;
	bool powerupOn;
	float delta;

	// check if we need to do a quick reset
	if ( quickSlowmoReset ) {
		quickSlowmoReset = false;

		// stop the sounds
		if ( gameSoundWorld ) {
			gameSoundWorld->SetSlowmo( false );
			gameSoundWorld->SetSlowmoSpeed( 1 );
		}

		// stop the state
		slowmoState = SLOWMO_STATE_OFF;
		slowmoMsec = USERCMD_MSEC;
	}

	// check the player state
	player = GetLocalPlayer();
	powerupOn = false;

	if ( player && player->PowerUpActive( HELLTIME ) ) {
		powerupOn = true;
	}
	else if ( g_enableSlowmo.GetBool() ) {
		powerupOn = true;
	}

	// determine proper slowmo state
	if ( powerupOn && slowmoState == SLOWMO_STATE_OFF ) {
		//triggered when slowmo is activated.
		slowmoState = SLOWMO_STATE_RAMPUP;

		slowmoMsec = msec;
		if ( gameSoundWorld ) {
			gameSoundWorld->SetSlowmo( true );
			gameSoundWorld->SetSlowmoSpeed( slowmoMsec / (float)USERCMD_MSEC );
		}
	}
	else if ( !powerupOn && slowmoState == SLOWMO_STATE_ON ) {
		slowmoState = SLOWMO_STATE_RAMPDOWN;

		// play the stop sound
		if ( player ) {
			player->PlayHelltimeStopSound();
		}
	}

	// do any necessary ramping
	if ( slowmoState == SLOWMO_STATE_RAMPUP )
	{
		//BC slow mo rate.
		const float SLOWRATE = 2; //bc this value seems to be hardcoded to 4 (?)

		//triggered when slowmo is activating (entering slow state).
		delta = SLOWRATE - slowmoMsec;

		if ( fabs( delta ) < g_slowmoStepRate.GetFloat() ) {
			slowmoMsec = SLOWRATE;
			slowmoState = SLOWMO_STATE_ON;
		}
		else {
			slowmoMsec += delta * g_slowmoStepRate.GetFloat();
		}

		if ( gameSoundWorld ) {
			gameSoundWorld->SetSlowmoSpeed( slowmoMsec / (float)USERCMD_MSEC );
		}
	}
	else if ( slowmoState == SLOWMO_STATE_RAMPDOWN ) {
		delta = 16 - slowmoMsec;

		if ( fabs( delta ) < g_slowmoStepRate.GetFloat() ) {
			slowmoMsec = 16;
			slowmoState = SLOWMO_STATE_OFF;
			if ( gameSoundWorld ) {
				gameSoundWorld->SetSlowmo( false );
			}
		}
		else {
			slowmoMsec += delta * g_slowmoStepRate.GetFloat();
		}

		if ( gameSoundWorld ) {
			gameSoundWorld->SetSlowmoSpeed( slowmoMsec / (float)USERCMD_MSEC );
		}
	}
}

/*
===========
idGameLocal::ResetSlowTimeVars
============
*/
void idGameLocal::ResetSlowTimeVars() {
	msec				= USERCMD_MSEC;
	slowmoMsec			= USERCMD_MSEC;
	slowmoState			= SLOWMO_STATE_OFF;

	fast.framenum		= 0;
	fast.previousTime	= 0;
	fast.time			= 0;
	fast.msec			= USERCMD_MSEC;

	slow.framenum		= 0;
	slow.previousTime	= 0;
	slow.time			= 0;
	slow.msec			= USERCMD_MSEC;
}

/*
===========
idGameLocal::QuickSlowmoReset
============
*/
void idGameLocal::QuickSlowmoReset() {
	quickSlowmoReset = true;
}

/*
===============
idGameLocal::NeedRestart
===============
*/
bool idGameLocal::NeedRestart() {
	
	idDict		newInfo;
	const idKeyValue *keyval, *keyval2;

	newInfo = *cvarSystem->MoveCVarsToDict( CVAR_SERVERINFO );
	
	for ( int i = 0; i < newInfo.GetNumKeyVals(); i++ ) {
		keyval = newInfo.GetKeyVal( i );
		keyval2 = serverInfo.FindKey( keyval->GetKey() );
		if ( !keyval2 ) {
			return true;
		}
		// a select set of si_ changes will cause a full restart of the server
		if ( keyval->GetValue().Cmp( keyval2->GetValue() ) && ( !keyval->GetKey().Cmp( "si_pure" ) || !keyval->GetKey().Cmp( "si_map" ) ) ) {
			return true;
		}
	}
	return false;
}

#endif

/*
================
idGameLocal::GetClientStats
================
*/
void idGameLocal::GetClientStats( int clientNum, char *data, const int len ) {
	mpGame.PlayerStats( clientNum, data, len );
}


/*
================
idGameLocal::SwitchTeam
================
*/
void idGameLocal::SwitchTeam( int clientNum, int team ) {

	idPlayer *   player;
	player = static_cast< idPlayer * >( entities[ clientNum ] );
	int oldTeam = player->team ;

	// Put in spectator mode
	if ( team == -1 ) {
		static_cast< idPlayer * >( entities[ clientNum ] )->Spectate( true );
	}
	// Switch to a team
	else {
		mpGame.SwitchToTeam ( clientNum, oldTeam, team );
	}
	player->forceRespawn = true ;
}

/*
===============
idGameLocal::GetMapLoadingGUI
===============
*/
void idGameLocal::GetMapLoadingGUI( char gui[ MAX_STRING_CHARS ] ) { }

idStr idGameLocal::GetMapIndex( void )
{
	idStr mapname;
	mapname = gameLocal.GetMapName();
	mapname.StripPath();
	mapname.StripFileExtension();
	mapname.ToLower();

	const idDecl *mapDecl = declManager->FindType( DECL_MAPDEF, mapname, false );
	const idDeclEntityDef *mapDef = static_cast<const idDeclEntityDef *>( mapDecl );

	if ( mapDef )
	{
		//found the map def.
		idStr levelIndex = common->GetLanguageDict()->GetString(mapDef->dict.GetString( "levelindex" ));
		return levelIndex;
	}

	return "";
}

bool idGameLocal::GetMapInterstitial( void )
{
	idStr mapname;
	mapname = gameLocal.GetMapName();
	mapname.StripPath();
	mapname.StripFileExtension();
	mapname.ToLower();

	const idDecl *mapDecl = declManager->FindType( DECL_MAPDEF, mapname, false );
	const idDeclEntityDef *mapDef = static_cast<const idDeclEntityDef *>( mapDecl );

	if ( mapDef )
	{
		//found the map def.
		return mapDef->dict.GetBool( "interstitial" );
	}

	return false;
}

//bc note that when adding new game.h functions, be sure to retain the same order in all .h files.
bool idGameLocal::CanSave( void )
{
	bool cansave = gameLocal.world->spawnArgs.GetBool("cansave", "1");

	if (!cansave && gameLocal.time > 1000 /* bc 3-8-2016 hack so that this save-message doesnt appear at level start*/ )
	{
		gameLocal.GetLocalPlayer()->Event_hudMessage("SAVING DISABLED IN THIS MAP.");
	}

	return cansave;
}


#ifdef STEAM

int idGameLocal::GetWorkshopAmount()
{
	if (!steamInitialized)
		return 0;

	uint total = SteamUGC()->GetNumSubscribedItems();
	
	if (total <= 0)
		return 0;

	if (total > MAX_WORKSHOP_ITEMS)
	{
		common->Printf("STEAM: warning: number of subscribed items (%d) exceeds MAX_WORKSHOP_ITEMS.\n", total);
	}

	SteamUGC()->GetSubscribedItems(modFileIDs, total);
	
	return total;
}

const char* idGameLocal::GetWorkshopPathAtIndex(int index)
{
	static char szItemFolder[1024] = { 0 };
	szItemFolder[0] = '\0'; 
	if (steamInitialized) {
		uint32 unItemState = SteamUGC()->GetItemState(modFileIDs[index]);

		if (unItemState && k_EItemStateSubscribed) {
			uint32 unTimeStamp = 0;
			uint64 unSizeOnDisk = 0;
	
			if ( !SteamUGC()->GetItemInstallInfo( modFileIDs[index], &unSizeOnDisk, szItemFolder, sizeof(szItemFolder), &unTimeStamp ) )
				szItemFolder[0] = '\0';
		}
	}

    return szItemFolder;
}

bool idGameLocal::GetSteamInitialized()
{
	return steamInitialized;
}
#endif
