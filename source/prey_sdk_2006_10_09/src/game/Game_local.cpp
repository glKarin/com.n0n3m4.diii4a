// Copyright (C) 2004 Id Software, Inc.
//

#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"
//HUMANHEAD: aob
#include "../prey/prey_local.h"
#include "../framework/BuildVersion.h" // HUMANHEAD mdl
//HUMANHEAD END

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

// HUMANHEAD pdm
#if INGAME_PROFILER_ENABLED
hhProfiler *				profiler = NULL;
#endif
// HUMANHEAD END

idCVar com_forceGenericSIMD( "com_forceGenericSIMD", "0", CVAR_BOOL|CVAR_SYSTEM, "force generic platform independent SIMD" );
idCVar g_printlocations( "g_printlocations", "0", CVAR_BOOL, "print area/location mappings");

#endif

idRenderWorld *				gameRenderWorld = NULL;		// all drawing is done to this world
idSoundWorld *				gameSoundWorld = NULL;		// all audio goes to this world

static gameExport_t			gameExport;

// global animation lib
idAnimManager				animationLib;

// the rest of the engine will only reference the "game" variable, while all local aspects stay hidden
// HUMANHEAD mdl:  Commented out the old gameLocal because it was messing up tagging
//#ifdef HUMANHEAD
hhGameLocal					gameLocal;
//#else
//idGameLocal					gameLocal;
//#endif
idGame *					game = &gameLocal;	// statically pointed at an idGameLocal

#ifdef HUMANHEAD
//NOTE: Duplicated in tr_rendertools and CamWnd, make any changes there also
const char *idGameLocal::sufaceTypeNames[ MAX_SURFACE_TYPES ] = {
	"none",	"metal", "stone", "flesh", "wood", "cardboard", "liquid", "glass", "tile",
	"wallwalk", "altmetal", "forcefield", "pipe", "spirit", "chaff"
};
#else
const char *idGameLocal::sufaceTypeNames[ MAX_SURFACE_TYPES ] = {
	"none",	"metal", "stone", "flesh", "wood", "cardboard", "liquid", "glass", "plastic",
	"ricochet", "surftype10", "surftype11", "surftype12", "surftype13", "surftype14", "surftype15"
};
#endif	// HUMANHEAD

/*
===========
GetGameAPI
============
*/
#if __MWERKS__
#pragma export on
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

		// HUMANHEAD pdm
#if INGAME_PROFILER_ENABLED
		profiler					= import->profiler;
#endif		
		// HUMANHEAD END
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

/*
===========
TestGameAPI
============
*/
void TestGameAPI( void ) {
	gameImport_t testImport;
	gameExport_t testExport;

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

	// HUMANHEAD pdm
#if INGAME_PROFILER_ENABLED
	testImport.profiler					= ::profiler;
#endif
	// HUMANHEAD END

	testExport = *GetGameAPI( &testImport );
}

/*
===========
idGameLocal::idGameLocal
============
*/
idGameLocal::idGameLocal() {
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
	//HUMANHEAD rww
	sortSnapshotPushers = false;
	sortSnapshotTeamMasters = false;
	//HUMANHEAD END
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
	//HUMANHEAD rww - for client entities.
	entityHash.Clear( 1024, MAX_GENTITIES+MAX_CENTITIES );
	//HUMANHEAD END
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

	// HUMANHEAD pdm:
	isCoop = false;
	// HUMANHEAD END

	localClientNum = 0;
	isMultiplayer = false;
	isServer = false;
	isClient = false;
	realClientTime = 0;
	isNewFrame = true;
	clientSmoothing = 0.1f;
	entityDefBits = 0;

	nextGibTime = 0;
	nextSteamTime = 0; // HUMANHEAD mdl
	globalMaterial = NULL;
	newInfo.Clear();
	lastGUIEnt = NULL;
	lastGUI = 0;

	//HUMANHEAD rww
	logitechLCDEnabled = false;
	logitechLCDDisplayAlt = false;
	logitechLCDButtonsLast = 0;
	logitechLCDUpdateTime = 0;
	//HUMANHEAD END

	memset( clientEntityStates, 0, sizeof( clientEntityStates ) );
	memset( clientPVS, 0, sizeof( clientPVS ) );
	memset( clientSnapshots, 0, sizeof( clientSnapshots ) );

	eventQueue.Init();
	savedEventQueue.Init();

	memset( lagometer, 0, sizeof( lagometer ) );

	//HUMANHEAD rww - for layeredSpawn
#if !GOLD
	spawnPopulating = false;
#endif
	// mdl:  Playtime
	playTime = 0;
	playTimeStart = -1;
	//HUMANHEAD END
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

	//HUMANHEAD rww
	ddaManager = new hhDDAManager;
	//HUMANHEAD END

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

#ifdef _HH_NET_DEBUGGING //HUMANHEAD rww
	cmdSystem->AddCommand( "networkStats", idClass::PrintHHNetStats_f, CMD_FL_SYSTEM|CMD_FL_GAME, "prints collective potential network optimizations" );
#endif //HUMANHEAD END

	Clear();

	idEvent::Init();
	idClass::Init();

	InitConsoleCommands();

	// load default scripts
	program.Startup( SCRIPT_DEFAULT );
	
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

	// HUMANHEAD pdm: Startup precaching for menu stuff
	FindEntityDef( "preCacheStartup", false );
	if (fileSystem->PerformingCopyFiles()) {
		FindEntityDef( "preCacheBuild", false );
	}
#if defined(_SYSTEM_BUILD_) || defined(ID_DEMO_BUILD)  //HUMANHEAD PCF mdl 05/26/06 - For precaching demo menu stuff.
	if (cvarSystem->GetCVarBool("com_makingDemoBuild")) {
		FindEntityDef( "preCacheDemoMainMenu", false );
	}
#endif
	// HUMANHEAD END

	gamestate = GAMESTATE_NOMAP;

	Printf( "...%d aas types\n", aasList.Num() );
	Printf( "game initialized.\n" );
	Printf( "--------------------------------------\n" );
}

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

	Printf( "--------------------------------------\n" );

	//HUMANHEAD rww
	delete ddaManager;
	ddaManager = NULL;
	//HUMANHEAD END

	//HUMANHEAD rww
	delete reactionHandler;
	reactionHandler = NULL;
	//HUMANHEAD END

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

#if DEATHWALK_AUTOLOAD // HUMANHEAD mdl
	savegame.WriteBool( bShouldAppend );
	if ( bShouldAppend ) {
		savegame.WriteString( serverInfo.GetString("deathwalkmap") );
	}
#endif // HUMANHEAD END

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

	// HUMANHEAD mdl:  Allows us to save objects that aren't spawned
	for( i = 0; i < uniqueObjects.Num(); i++ ) {
		savegame.AddObject( uniqueObjects[i] );
	}
	// HUMANHEAD END

	// write out complete object list
	savegame.WriteObjectList();

	program.Save( &savegame );

	Save( &savegame ); // HUMANHEAD Added to allow hhGameLocal to save it's data -mdl

//	savegame.WriteInt( g_skill.GetInteger() );	// HUMANHEAD pdm: not used
	savegame.WriteInt( g_wicked.GetInteger() );	// HUMANHEAD pdm
	savegame.WriteInt( g_casino.GetInteger() );	// HUMANHEAD pdm

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
	//HUMANHEAD rww
	savegame.WriteBool( sortSnapshotPushers );
	savegame.WriteBool( sortSnapshotTeamMasters );
	//HUMANHEAD END
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

	savegame.WriteInt( vacuumAreaNum );

	savegame.WriteInt( entityDefBits );
	savegame.WriteBool( isServer );
	savegame.WriteBool( isClient );

	savegame.WriteInt( localClientNum );

	// snapshotEntities is used for multiplayer only

	savegame.WriteInt( realClientTime );
	savegame.WriteBool( isNewFrame );
	savegame.WriteFloat( clientSmoothing );

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
	// HUMANHEAD mdl
	savegame.WriteInt( nextSteamTime );
	savegame.WriteBool( isCoop );
	savegame.WriteInt( GetTimePlayed() );
	// HUMANHEAD END

	// spawnSpots
	// initialSpots
	// currentInitialSpot
	// newInfo
	// makingBuild
	// shakeSounds

	// write out pending events
	idEvent::Save( &savegame );

	savegame.Close();
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

	// HUMANHEAD pdm
	SpiritWalkSoundMode( false );
	DialogSoundMode( false );
	// HUMANHEAD END

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

// HUMANHEAD pdm: Support for level appending
#if DEATHWALK_AUTOLOAD
	bool bUseLoaded = collisionModelManager->WillUseAlreadyLoadedCollisionMap(mapFile);
#endif
// HUMANHEAD END

	// load the collision map
	collisionModelManager->LoadMap( mapFile );

// HUMANHEAD pdm: Support for level appending
#if DEATHWALK_AUTOLOAD
	if (bShouldAppend && !bUseLoaded) {
		// load the collision map for dw
		collisionModelManager->AppendMap( additionalMapFile );
	}
#endif
// HUMANHEAD END

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
	//HUMANHEAD rww
	sortSnapshotPushers = false;
	sortSnapshotTeamMasters = false;
	//HUMANHEAD END
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
	nextSteamTime	= 0; // HUMANHEAD mdl

	//HUMANHEAD rww - check lglcd validity and reset values
	logitechLCDEnabled = sys->LGLCD_Valid();
	logitechLCDDisplayAlt = false;
	logitechLCDButtonsLast = 0;
	logitechLCDUpdateTime = 0;
	//HUMANHEAD END

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

	// HUMANHEAD pdm: Register our game portals so they can be used in PVS below
#if GAMEPORTAL_PVS
	gameRenderWorld->RegisterGamePortals(mapFile);
#endif
#if GAMEPORTAL_SOUND
	gameRenderWorld->LevelInitSoundAreas();
#endif
	// HUMANHEAD END

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

		// HUMANHEAD pdm
		SpiritWalkSoundMode( false );
		DialogSoundMode( false );
		// HUMANHEAD END
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

	// HUMANHEAD pdm: restart current map if cycling is disabled
	if ( !g_runMapCycle.GetBool() ) {
		return true;
	}

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
	//HUMANHEAD rww - keep track of layered spawning
#if !GOLD
	spawnPopulating = true;
	layeredSpawn = 0;
#endif
	//HUMANHEAD END

	if ( isMultiplayer ) {
		cvarSystem->SetCVarBool( "r_skipSpecular", false );
	}
	// parse the key/value pairs and spawn entities
	SpawnMapEntities();

// HUMANHEAD pdm: Support for level appending
#if DEATHWALK_AUTOLOAD
	if (bShouldAppend) {
		SpawnAppendedMapEntities();
	}
#endif
// HUMANHEAD END

	// mark location entities in all connected areas
	SpreadLocations();

	// prepare the list of randomized initial spawn spots
	RandomizeInitialSpawns( );

	mapSpawnCount = spawnCount;

	// execute pending events before the very first game frame
	// this makes sure the map script main() function is called
	// before the physics are run so entities can bind correctly
	Printf( "==== Processing events ====\n" );
	idEvent::ServiceEvents();

	//HUMANHEAD rww - keep track of layered spawning
#if !GOLD
	if (layeredSpawn) {
		gameLocal.Error("Uneven layeredSpawn at the end of idGameLocal::MapPopulate!\n");
	}
	spawnPopulating = false;
#endif
	//HUMANHEAD END
}

/*
===================
idGameLocal::InitFromNewMap
===================
*/
void idGameLocal::InitFromNewMap( const char *mapName, idRenderWorld *renderWorld, idSoundWorld *soundWorld, bool isServer, bool isClient, int randseed ) {

	uniqueObjects.Clear(); // HUMANHEAD mdl

	this->isServer = isServer;
	this->isClient = isClient;
	this->isMultiplayer = isServer || isClient;

	// HUMANHEAD mdl:  Playtime
	if ( playTimeStart != -1 ) {
		if ( isMultiplayer ) {
			playTime = 0; // No playtime for MP
		} else {
			playTime += gameLocal.time - playTimeStart;
		}
		playTimeStart = -1;
	}
	// HUMANHEAD END

	if ( mapFileName.Length() ) {
		MapShutdown();
	}

	Printf( "----------- Game Map Init ------------\n" );

	gamestate = GAMESTATE_STARTUP;

	gameRenderWorld = renderWorld;
	gameSoundWorld = soundWorld;

// HUMANHEAD pdm: Support for level appending
#if DEATHWALK_AUTOLOAD
	bShouldAppend = serverInfo.GetBool("shouldappendlevel");

	additionalMapFile = NULL;
	if (bShouldAppend) {
		// Load the deathwalk map
		idStr additionalMapName = serverInfo.GetString("deathwalkmap");
		additionalMapName.SetFileExtension( "map" );
		additionalMapFile = new idMapFile;
		if ( !additionalMapFile->Parse( additionalMapName.c_str() ) ) {
			delete additionalMapFile;
			additionalMapFile = NULL;
		}
	}
#endif

	LoadMap( mapName, randseed );

	InitScriptForMap();

	MapPopulate();

	// HUMANHEAD pdm: tell sound system about our location names
	RegisterLocationsWithSoundWorld();

	mpGame.Reset();

	mpGame.Precache();

	// free up any unused animations
	animationLib.FlushUnusedAnims();

	gamestate = GAMESTATE_ACTIVE;

	// HUMANHEAD mdl:  Start session
	if ( !isMultiplayer && playTimeStart == ( unsigned int ) -1 ) {
		playTimeStart = gameLocal.time;
	}
	// HUMANHEAD END


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

	uniqueObjects.Clear(); // HUMANHEAD mdl

	idRestoreGame savegame( saveGameFile );

	savegame.ReadBuildNumber();

	// HUMANHEAD pdm: Support for level appending
#if DEATHWALK_AUTOLOAD
	savegame.ReadBool( bShouldAppend );

	additionalMapFile = NULL;
	if (bShouldAppend) {
		// Load the deathwalk map
		idStr additionalMapName;
		savegame.ReadString( additionalMapName );
		additionalMapName.SetFileExtension( "map" );
		additionalMapFile = new idMapFile;
		if ( !additionalMapFile->Parse( additionalMapName.c_str() ) ) {
			delete additionalMapFile;
			additionalMapFile = NULL;
		}
	}
#endif

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

	Restore( &savegame ); // HUMANHEAD Added to allow hhGameLocal to restore it's data -mdl

//	savegame.ReadInt( i );		// HUMANHEAD pdm: not used
//	g_skill.SetInteger( i );	// HUMANHEAD pdm: not used

	savegame.ReadInt( i );		// HUMANHEAD pdm
	g_wicked.SetInteger( i );

	savegame.ReadInt( i );		// HUMANHEAD pdm
	g_casino.SetInteger( i );

	// precache the player
	FindEntityDef( GAME_PLAYERDEFNAME, false );	// HUMANHEAD pdm: removed hardcoded name

	// precache any media specified in the map
	for ( i = 0; i < mapFile->GetNumEntities(); i++ ) {
		idMapEntity *mapEnt = mapFile->GetEntity( i );

		if ( !idGameLocal::InhibitEntitySpawn( mapEnt->epairs ) ) { // HUMANHEAD mdl:  Need this to call the original function
			CacheDictionaryMedia( &mapEnt->epairs );
			const char *classname = mapEnt->epairs.GetString( "classname" );
			if ( classname != '\0' ) {
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
	//HUMANHEAD rww
	savegame.ReadBool( sortSnapshotPushers );
	savegame.ReadBool( sortSnapshotTeamMasters );
	//HUMANHEAD END
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

	savegame.ReadInt( vacuumAreaNum );

	savegame.ReadInt( entityDefBits );
	savegame.ReadBool( isServer );
	savegame.ReadBool( isClient );

	savegame.ReadInt( localClientNum );

	// snapshotEntities is used for multiplayer only

	savegame.ReadInt( realClientTime );
	savegame.ReadBool( isNewFrame );
	savegame.ReadFloat( clientSmoothing );

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

	// HUMANHEAD mdl
	savegame.ReadInt( nextSteamTime );
	savegame.ReadBool( isCoop );
	savegame.ReadInt( reinterpret_cast<int &> ( playTime ) );
	// HUMANHEAD END

	// spawnSpots
	// initialSpots
	// currentInitialSpot
	// newInfo
	// makingBuild
	// shakeSounds

	// Read out pending events
	idEvent::Restore( &savegame );

	// HUMANHEAD PCF rww 05/12/06 - fix for gravity being reset on physics objects after load
	// update our gravity vector if needed
	UpdateGravity();
	// HUMANHEAD END

	savegame.RestoreObjects();

	mpGame.Reset();

	mpGame.Precache();

	// free up any unused animations
	animationLib.FlushUnusedAnims();

	gamestate = GAMESTATE_ACTIVE;

	playTimeStart = gameLocal.time; // HUMANHEAD mdl:  Start new session

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

	//HUMANHEAD rww - for client entities.
	//for( i = ( clearClients ? 0 : MAX_CLIENTS ); i < MAX_GENTITIES; i++ ) {
	for( i = ( clearClients ? 0 : MAX_CLIENTS ); i < (MAX_GENTITIES+MAX_CENTITIES); i++ ) {
	//HUMANHEAD END
		delete entities[ i ];
		// ~idEntity is in charge of setting the pointer to NULL
		// it will also clear pending events for this entity
		assert( !entities[ i ] );
		spawnIds[ i ] = -1;
	}

	//HUMANHEAD rww - for client entities.
	//entityHash.Clear( 1024, MAX_GENTITIES );
	entityHash.Clear( 1024, MAX_GENTITIES+MAX_CENTITIES );
	//HUMANHEAD END

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
}

/*
===========
idGameLocal::MapShutdown
============
*/
void idGameLocal::MapShutdown( void ) {
	Printf( "--------- Game Map Shutdown ----------\n" );
	
	gamestate = GAMESTATE_SHUTDOWN;

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

	MapClear( true );

	// reset the script to the state it was before the map was started
	program.Restart();

	if ( smokeParticles ) {
		smokeParticles->Shutdown();
	}

	// HUMANHEAD pdm: clear the sound system.  Fixes bug where a looper playing when you "really die" would keep stuttering during restart gui
	if ( gameSoundWorld ) {
		gameSoundWorld->ClearAllSoundEmitters();
		SpiritWalkSoundMode( false );
		DialogSoundMode( false );
	}
	// HUMANHEAD END

#if GAMEPORTAL_SOUND
	if (gameRenderWorld) {
		gameRenderWorld->LevelShutdownSoundAreas();
	}
#endif

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
	idStrList oggSounds, weaponSounds, oggOnlySounds;
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

				// HUMANHEAD mdl:  Added a check to make sure we don't try to convert OGG files to OGG files.
				if ( !idStr::Icmp(soundName.Right( 4 ), ".ogg" ) ) {
					oggOnlySounds.AddUnique( soundName );
					continue;
				}
				// HUMANHEAD END

				// don't OGG sounds that cause a shake because that would
				// cause continuous seeking on the OGG file which is expensive
				if ( parms->shakes != 0.0f || (parms->soundShaderFlags & SSF_VOICEAMPLITUDE)) {
					shakeSounds.AddUnique( soundName );
					continue;
				}

#if HUMANHEAD	// HUMANHEAD mdc - our version of skip logic (to account for our naming conventions)

				// always ogg vo, music, elements
				if(		soundName.Find( "/elements/", false )	== -1 &&
						soundName.Find( "/music/", false )	== -1 &&
						soundName.Find( "/vo/", false )			== -1 ) {

						//don't ogg these
						if(		soundName.Find("/weapons/", false)			!= -1 ||
								soundName.Find("/lights/", false )			!= -1 ||
								(soundName.Find("/monsters/", false) != -1 && soundName.Find("fire", false) != -1) ||
								soundName.Find("foot", false)				!= -1 ||
								soundName.Find("pain", false)				!= -1 ) {

						weaponSounds.AddUnique( soundName );
						continue;

					}
				}
#else
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
#endif

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
#if HUMANHEAD
	file->Printf( "echo %d kB in miscellaneous skipped sounds\n\n\n", totalSize >> 10 );
#else
	file->Printf( "echo %d kB in weapon sounds\n\n\n", totalSize >> 10 );
#endif

	// list all the existing ogg sounds
	totalSize = 0;
	for ( i = 0; i < oggOnlySounds.Num(); i++ ) {
		size = fileSystem->ReadFile( oggOnlySounds[i], NULL, NULL );
		totalSize += size;
		oggOnlySounds[i].Replace( "/", "\\" );
		file->Printf( "echo \"%s\" (%d kB)\n", oggOnlySounds[i].c_str(), size >> 10 );
	}
	file->Printf( "echo %d kB in pre-existing OGG sounds\n\n\n", totalSize >> 10 );

	// list commands to convert all other sounds to ogg
	totalSize = 0;
	for ( i = 0; i < oggSounds.Num(); i++ ) {
		size = fileSystem->ReadFile( oggSounds[i], NULL, NULL );
		totalSize += size;
		oggSounds[i].Replace( "/", "\\" );
#if HUMANHEAD
#ifdef _SYSTEM_BUILD_
		for (int shakeIndex = 0; shakeIndex < shakeSounds.Num(); shakeIndex++) {
			if ( !shakeSounds[shakeIndex].IcmpPath( oggSounds[i] ) ) {
				file->Printf( "echo \"WARNING:  Shake sound found in ogg conversion:  %s\"\n", oggSounds[i].c_str() );
				break;
			}
		}
#endif // _SYSTEM_BUILD_  
//HUMANHEAD mdc - modified so this will write out the directory using the fs_savepath, so we don't delete the sounds from our source
		file->Printf( "p:\\tools\\ogg\\oggenc -q 2 \"%s\\base\\%s\"\n", cvarSystem->GetCVarString("fs_savepath"), oggSounds[i].c_str() );
		file->Printf( "del \"%s\\base\\%s\"\n", cvarSystem->GetCVarString("fs_savepath"), oggSounds[i].c_str() );
#else
		file->Printf( "w:\\doom\\ogg\\oggenc -q 0 \"c:\\doom\\base\\%s\"\n", oggSounds[i].c_str() );
		file->Printf( "del \"c:\\doom\\base\\%s\"\n", oggSounds[i].c_str() );
#endif
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
	if ( soundShaderName != '\0' && dict->GetFloat( "s_shakes" ) != 0.0f ) {
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

//HUMANHEAD mdc - skip caching if we are in development mode and not making a build
	if( !g_precache.GetBool() && !sys_forceCache.GetBool() && !cvarSystem->GetCVarBool("com_makingBuild") ) {
		return;
	}
//HUMANHEAD END

	//HUMANHEAD: aob
	idList<idStr> strList;
	//HUMANHEAD END

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
#if 1 //HUMANHEAD rww - keep the gui around, so that we don't hit the disk again when we go to use it
				uiManager->FindGui(kv->GetValue().c_str(), true, false, false);
#else
				idUserInterface *gui = uiManager->Alloc();
				if ( gui ) {
					gui->InitFromFile( kv->GetValue() );
					uiManager->DeAlloc( gui );
				}
#endif
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
			//HUMANHEAD: aob - this is a temp test to allow bowman to have different fade times on his materials
			strList.Clear();
			hhUtils::SplitString( kv->GetValue(), strList );
			for( int ix = strList.Num() - 1; ix >= 0; --ix ) {
				declManager->FindType( DECL_MATERIAL, strList[ix] );
			}
			//HUMANHEAD END
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

/*	HUMANHEAD pdm: not used
	// handles teleport fx.. this is not ideal but the actual decision on which fx to use
	// is handled by script code based on the teleport number
	kv = dict->MatchPrefix( "teleport", NULL );
	if ( kv && kv->GetValue().Length() ) {
		int teleportType = atoi( kv->GetValue() );
		const char *p = ( teleportType ) ? va( "fx/teleporter%i.fx", teleportType ) : "fx/teleporter.fx";
		declManager->FindType( DECL_FX, p );
	}
*/

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

/*	HUMANHEAD pdm: not used
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
*/
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
#if HUMANHEAD	// HUMANHEAD pdm: removed hardcoded name
	args.Set( "classname", isMultiplayer ? GAME_PLAYERDEFNAME_MP : GAME_PLAYERDEFNAME );

	// HUMANHEAD mdl:  Check for map specific player health
	bool mapHealth = false;
	int playerHealth;
	const char *mapName = gameLocal.serverInfo.GetString( "si_map" );
	const idDecl *mapDecl = declManager->FindType(DECL_MAPDEF, mapName, false );
	if ( mapDecl ) {
		const idDeclEntityDef *mapInfo = static_cast<const idDeclEntityDef *>(mapDecl);
		playerHealth = mapInfo->dict.GetInt( "playerHealth" );
		if ( playerHealth > 0 ) {
			// Optionally store the old health value in storedHealth
			if (mapInfo->dict.GetBool( "storePlayerHealth", "0" )) {
				persistentPlayerInfo[ clientNum ].SetInt( "storedHealth", persistentPlayerInfo[ clientNum ].GetInt( "health", "100" ));
			}
			persistentPlayerInfo[ clientNum ].SetInt( "health", playerHealth );
			mapHealth = true;
		}
		int playerSpirit = mapInfo->dict.GetInt( "playerSpirit" );
		if ( playerSpirit > 0 ) {
			args.SetInt( "ammo_spiritpower", playerSpirit );
		}
	}

	// See if we need to restore a previously saved health value
	playerHealth = persistentPlayerInfo[ clientNum ].GetInt( "storedHealth", "0" );
	if ( !mapHealth && playerHealth > 0 ) {
		persistentPlayerInfo[ clientNum ].SetInt( "health", playerHealth );
		persistentPlayerInfo[ clientNum ].SetInt( "storedHealth", 0 );
	}
#else
	args.Set( "classname", isMultiplayer ? "player_doommarine_mp" : "player_doommarine" );
#endif
	//HUMANHEAD rww - ok on client
	if ( !SpawnEntityDef( args, &ent, true, false, true ) || !entities[ clientNum ] ) {
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
=================
idGameLocal::AmLocalClient
HUMANHEAD nla - Function that was removed
=================
*/
bool idGameLocal::AmLocalClient( idPlayer *player ) {

	if ( !isClient || player->entityNumber == localClientNum ) {
		return( true );
	}

	return( false );
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
	//HUMANHEAD rww
	int			startIndex = 0;
	//HUMANHEAD END
	idEntity *	ent;
	idPlayer *	player;
	pvsHandle_t	otherPVS, newPVS;

	playerPVS.i = -1;
	//HUMANHEAD rww
	if (gameLocal.isClient) {
		startIndex = gameLocal.localClientNum;

		if (gameLocal.localClientNum != -1 && gameLocal.entities[gameLocal.localClientNum] && gameLocal.entities[gameLocal.localClientNum]->IsType(hhPlayer::Type)) {
			hhPlayer *plClient = static_cast<hhPlayer *>(gameLocal.entities[gameLocal.localClientNum]);
			if (plClient->spectating && plClient->spectator >= 0 && plClient->spectator < MAX_CLIENTS) {
				startIndex = plClient->spectator; //build a pvs for the player i am spectating
			}
		}
	}
	for ( i = startIndex; i < numClients; i++ ) {
	//HUMANHEAD END
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

		//HUMANHEAD rww
		if (gameLocal.isClient) {
			break;
		}
		//HUMANHEAD END
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
				//HUMANHEAD PCF rww 05/27/06 - force pusher sort
				//hack fix to prevent elevator skipping on harvesterb
				if (!ent->fl.forcePusherSort) {
				//HUMANHEAD END
					ent->activeNode.Remove();
					ent->activeNode.AddToFront( activeEntities );
				}
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

//HUMANHEAD rww
/*
================
idGameLocal::SortSnapshotEntityList

  Sorts the snapshot entity list such that pushing entities come first,
  actors come next and physics team slaves appear after their master.
================
*/
void idGameLocal::SortSnapshotEntityList( void ) {
	idEntity *ent, *next_ent, *master, *part;

	// if the snapshot entity list needs to be reordered to place physics team masters at the front
	if ( sortSnapshotTeamMasters ) {
		for ( ent = snapshotEntities.Next(); ent != NULL; ent = next_ent ) {
			next_ent = ent->snapshotNode.Next();
			master = ent->GetTeamMaster();
			if ( master && master == ent ) {
				ent->snapshotNode.Remove();
				ent->snapshotNode.AddToFront( snapshotEntities );
			}
		}
	}

	// if the snapshot entity list needs to be reordered to place pushers at the front
	if ( sortSnapshotPushers ) {

		for ( ent = snapshotEntities.Next(); ent != NULL; ent = next_ent ) {
			next_ent = ent->snapshotNode.Next();
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
					ent->snapshotNode.Remove();
					ent->snapshotNode.AddToFront( snapshotEntities );
				}
			}
		}

		for ( ent = snapshotEntities.Next(); ent != NULL; ent = next_ent ) {
			next_ent = ent->snapshotNode.Next();
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
					ent->snapshotNode.Remove();
					ent->snapshotNode.AddToFront( activeEntities );
				}
			}
		}
	}

	sortSnapshotTeamMasters = false;
	sortSnapshotPushers = false;
}
//HUMANHEAD END

/*
================
idGameLocal::RunFrame
================
*/
gameReturn_t idGameLocal::RunFrame( const usercmd_t *clientCmds ) {
/*	HUMANHEAD pdm: OVERRIDDEN
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
					ent->Think();
					num++;
				}
			}
		}

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
*/
	gameReturn_t ret;
	ret.consistencyHash = 0;
	ret.sessionCommand[0] = 0;
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
	
	if ( !sys->FPU_StackIsEmpty() ) {
		Printf( sys->FPU_GetState() );
		Error( "idGameLocal::CalcFov: FPU stack not empty" );
	}

	// first, calculate the vertical fov based on a 640x480 view
	x = 640.0f / tan( base_fov / 360.0f * idMath::PI );
	y = atan2( 480.0f, x );
	fov_y = y * 360.0f / idMath::PI;

	// FIXME: somehow, this is happening occasionally
	assert( fov_y > 0 );
	if ( fov_y <= 0 ) {
		Printf( sys->FPU_GetState() );
		Error( "idGameLocal::CalcFov: bad result" );
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
		Printf( sys->FPU_GetState() );
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
/*	HUMANHEAD pdm: OVERRIDDEN
	if ( isMultiplayer ) {
		return mpGame.Draw( clientNum );
	}

	idPlayer *player = static_cast<idPlayer *>(entities[ clientNum ]);

	if ( !player ) {
		return false;
	}

	// render the scene
	player->playerView.RenderPlayerView( player->hud );
*/
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
	if ( !isMultiplayer ) {
		return NULL;
	}
	return mpGame.StartMenu();
}

/*
================
idGameLocal::HandleGuiCommands
================
*/
const char* idGameLocal::HandleGuiCommands( const char *menuCommand ) {
	if ( !isMultiplayer ) {
		return NULL;
	}
	return mpGame.HandleGuiCommands( menuCommand );
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

//HUMANHEAD rww
void idGameLocal::GetAPUserInfo(idDict &dict, int clientNum) {
	static const char *apNames[8] = {
		"i have spaces in my name",
		"DIX",
		"j0sh",
		"mpl",
		"Boris",
		"Big H",
		"fr3d",
		"sho"
	};
	if (clientNum >= 0 && clientNum < 8) {
		dict.Set("ui_name", apNames[clientNum]);
	}
	else {
		dict.Set("ui_name", va("Johnny McMuffins %i", clientNum));
	}
	dict.Set("ui_ready", "Ready");
	dict.Set("ui_spectate", "Play");
	dict.SetInt("ui_modelNum", clientNum);
}

void idGameLocal::SpawnArtificialPlayer(void) {
	idEntity	*ent;
	idDict		dict;
	idDict		apUI;
	const int	maxClients = serverInfo.GetInt( "si_maxPlayers" );
	const char	*classname = "player_artificial_mp";
	int			clientNum;

	for (clientNum = 0; clientNum < maxClients; clientNum++) {
		if (!entities[clientNum]) {
			break;
		}
	}
	if (clientNum == maxClients) {
		Printf("No free player slots.\n");
		return;
	}

	dict.Set( "classname", "player_artificial_mp" );
	dict.SetInt( "spawn_entnum", clientNum );
	dict.Set( "name", va( "player%d", clientNum + 1 ) );

	if (!SpawnEntityDef( dict, &ent )) {
		Warning("Couldn't spawn entityDef '%s' for artificial player.\n", classname);
		return;
	}

	if ( clientNum >= numClients ) {
		numClients = clientNum + 1;
	}

	GetAPUserInfo(apUI, clientNum);
	SetUserInfo(clientNum, apUI, false, false);
	mpGame.SpawnPlayer( clientNum );
}
//HUMANHEAD END

/*
================
idGameLocal::RunDebugInfo
================
*/
void idGameLocal::RunDebugInfo( void ) {
#if GOLD //HUMANHEAD rww
	if (!developer.GetBool()) {
		return;
	}
#endif //HUMANHEAD END

	//HUMANHEAD rww
	if (isMultiplayer && isServer && g_artificialPlayerCount.GetInteger() > 0) {
		int numExistingAP = 0;
		int numClients = 0;
		bool anyNonAP = false;
		const int maxClients = serverInfo.GetInt( "si_maxPlayers" );

		for (int i = 0; i < maxClients; i++) {
			if (entities[i]) {
				if (entities[i]->IsType(hhPlayer::Type)) {
					if (entities[i]->IsType(hhArtificialPlayer::Type)) {
						numExistingAP++;
					}
					else {
						anyNonAP = true;
					}
					numClients++;
				}
			}
		}

		if (anyNonAP && numClients < maxClients && numExistingAP < g_artificialPlayerCount.GetInteger()) { //if we have room and want more, spawn one this frame
			SpawnArtificialPlayer();
		}
	}
	//HUMANHEAD END

	idEntity *ent;
	idPlayer *player;

	player = GetLocalPlayer();
	if ( !player ) {
		return;
	}

	const idVec3 &origin = player->GetPhysics()->GetOrigin();

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

	if ( g_showCollisionWorld.GetBool() || ai_debugPath.GetInteger() == 1 ) {
		collisionModelManager->DrawModel( 0, vec3_origin, mat3_identity, origin, 128.0f );
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

	// HUMANHEAD pdm: game portal support
#if GAMEPORTAL_PVS
	if ( g_showGamePortals.GetInteger() ) {
		gameRenderWorld->DrawGamePortals(g_showGamePortals.GetInteger(), gameLocal.GetLocalPlayer()->viewAngles.ToMat3());
	}
#endif
#if GAMEPORTAL_SOUND
	if ( g_showValidSoundAreas.GetInteger() ) {
		gameRenderWorld->DrawValidSoundAreas(origin, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());
	}
#endif

	if (g_showEntityCount.GetBool() || g_maxEntitiesWarning.GetInteger()) {
		int count = 0;
		for (int ix=0; ix<MAX_GENTITIES; ix++) {
			if (entities[ix]) {
				count++;
			}
		}
		// Display warning if over the entity limit
		if (g_maxEntitiesWarning.GetInteger() > 0 && count > g_maxEntitiesWarning.GetInteger()) {
			Warning("%d entity slots used!  g_maxEntitiesWarning=%d\n", count, g_maxEntitiesWarning.GetInteger());
		}
		else if (g_showEntityCount.GetBool()) {
			Printf("%d entity slots used\n", count);
		}
	}
	// HUMANHEAD END

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
	if ( !requirePlayer || ( player && !player->IsDead() ) ) { // HUMANHEAD cjr: allow cheats when deathwalking
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
extern void Cmd_EntityList(const idStr &match); //HUMANHEAD rww
void idGameLocal::RegisterEntity( idEntity *ent ) {
	int spawn_entnum;

	if ( spawnCount >= ( 1 << ( 32 - GENTITYNUM_BITS_PLUSCENT ) ) ) { //HUMANHEAD rww cent bits
		//HUMANHEAD rww
		Error( "idGameLocal::RegisterEntity: spawn count overflow" );
		//this should be a safe way to handle this, since entities will still be in different slots,
		//even if they happen to share the same spawn id.
		//in theory you should need a server up for a very long time for this to happen, but with enough
		//fx and projectiles it is conceivable.
		//spawnCount = gameLocal.GetMapSpawnCount()+1;
		//addendum - actually, seems like this may be causing trouble with really damn old safe entity pointers.
		//HUMANHEAD END
	}

	//HUMANHEAD rww - client entities
	if (ent->fl.clientEntity) {
		int clientIndex = MAX_GENTITIES;
		if ( !spawnArgs.GetInt( "spawn_entnum", "0", spawn_entnum ) ) {
			while( entities[clientIndex] && clientIndex < (MAX_GENTITIES+MAX_CENTITIES) ) {
				clientIndex++;
			}
			if ( clientIndex >= (MAX_GENTITIES+MAX_CENTITIES) ) {
				Error( "no free client entities" );
			}
			spawn_entnum = clientIndex;
		}
		else {
			spawn_entnum += MAX_GENTITIES;
		}
	}
	else
	{
	//END HUMANHEAD
		if ( !spawnArgs.GetInt( "spawn_entnum", "0", spawn_entnum ) ) {
			while( entities[firstFreeIndex] && firstFreeIndex < ENTITYNUM_MAX_NORMAL ) {
				firstFreeIndex++;
			}
			if ( firstFreeIndex >= ENTITYNUM_MAX_NORMAL ) {
				Cmd_EntityList(""); //HUMANHEAD rww
				Error( "no free entities" );
			}
			spawn_entnum = firstFreeIndex++;
		}
	}

	assert(!entities[ spawn_entnum ]); //HUMANHEAD rww - this can happen if something decides to do something dumb like spawn a new entity with an existing dict that includes a spawn_entnum
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
		//ent->snapshotNode.Remove(); //HUMANHEAD rww - testing to see if this fixes mysterious prediction crash
		entities[ ent->entityNumber ] = NULL;
		spawnIds[ ent->entityNumber ] = -1;
		//HUMANHEAD rww - don't set this for client ents
		//if ( ent->entityNumber >= MAX_CLIENTS && ent->entityNumber < firstFreeIndex ) {
		if ( ent->entityNumber >= MAX_CLIENTS && ent->entityNumber < firstFreeIndex && ent->entityNumber < MAX_GENTITIES ) {
		//HUMANHEAD END
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

//HUMANHEAD rww
#if !GOLD
	if (isClient && !bIsClientReadSnapshot) {
		Error( "Attempted to spawn non-client entity '%s' on client", classdef.classname );
	}
#endif
	/*
#if _DEBUG
	if ( isClient ) {
		assert( bIsClientReadSnapshot );
	}
#endif
	*/
//HUMANHEAD END

	if ( !classdef.IsType( idEntity::Type ) ) {
		Error( "Attempted to spawn non-entity class '%s'", classdef.classname );
	}

	//HUMANHEAD rww - keep track of layered spawning
#if !GOLD
	layeredSpawn++;
	if (gameLocal.isMultiplayer) {
		if (spawnPopulating && layeredSpawn > 1) {
			const char *typeName = classdef.classname;
			if (!typeName) {
				typeName = "unknown";
			}
			gameLocal.Error("layeredSpawn says an entity is spawning an entity (type '%s') in idGameLocal::SpawnEntityType!\n", typeName);
		}
	}
#endif
	//HUMANHEAD END

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

	//HUMANHEAD rww - keep track of layered spawning
#if !GOLD
	layeredSpawn--;
#endif
	//HUMANHEAD END

	return static_cast<idEntity *>(obj);
}

//HUMANHEAD rww
/*
================
idGameLocal::SpawnEntityTypeClient
================
*/
idEntity *idGameLocal::SpawnEntityTypeClient( const idTypeInfo &classdef, const idDict *args ) {
	idClass *obj;

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

		if (gameLocal.isClient) {
			idEntity *objEnt = static_cast<idEntity *>(obj);
			objEnt->fl.clientEntity = true;
		}

		obj->CallSpawn();
	}

	catch( idAllocError & ) {
		obj = NULL;
	}
	spawnArgs.Clear();

	return static_cast<idEntity *>(obj);
}
//HUMANHEAD END

/*
===================
idGameLocal::SpawnEntityDef

Finds the spawn function for the entity and calls it,
returning false if not found
===================
*/
bool idGameLocal::SpawnEntityDef( const idDict &args, idEntity **ent, bool setDefaults, bool clientEntity, bool bIsClientReadSnapshot ) {
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

	// check if we should spawn a class object
	spawnArgs.GetString( "spawnclass", NULL, &spawn );
	if ( spawn ) {

		//HUMANHEAD rww
#if !GOLD
		if (isClient && !clientEntity && !bIsClientReadSnapshot) {
			const char *typeName = classname;
			if (!typeName) {
				typeName = "unknown";
			}
			Error("idGameLocal::SpawnEntityDef: '%s' trying to spawn on client but not a client ent", typeName);
		}
#endif
		//HUMANHEAD END

		cls = idClass::GetClass( spawn );
		if ( !cls ) {
			Warning( "Could not spawn '%s'.  Class '%s' not found%s.", classname, spawn, error.c_str() );
			return false;
		}

		obj = cls->CreateInstance();
		if ( !obj ) {
			Warning( "Could not spawn '%s'. Instance could not be created%s.", classname, error.c_str() );
			return false;
		}

		//HUMANHEAD rww - client entities
		if (clientEntity) {
			idEntity *objEnt = static_cast<idEntity *>(obj);
			objEnt->fl.clientEntity = true;
		}
		//END HUMANHEAD

		//HUMANHEAD rww - keep track of layered spawning
#if !GOLD
		layeredSpawn++;
		if (gameLocal.isMultiplayer) {
			if (spawnPopulating && layeredSpawn > 1) {
				const char *typeName = classname;
				if (!typeName) {
					typeName = "unknown";
				}
				gameLocal.Error("layeredSpawn says an entity is spawning an entity (type '%s') in idGameLocal::SpawnEntityDef!\n", typeName);
			}
		}
#endif
		//HUMANHEAD END

		obj->CallSpawn();

		//HUMANHEAD rww - keep track of layered spawning
#if !GOLD
		layeredSpawn--;
#endif
		//HUMANHEAD END

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
#if GERMAN_VERSION
		decl = declManager->FindType( DECL_ENTITYDEF, va( "%s_mp_nogore", name ), false );
		if ( !decl ) // Fall through to normal mp decls below
#else if !GOLD // For testing the german defs
		if ( g_nogore.GetBool() ) {
			decl = declManager->FindType( DECL_ENTITYDEF, va( "%s_mp_nogore", name ), false );
		}
		if ( !decl ) // Fall through to normal mp decls below
#endif 
			decl = declManager->FindType( DECL_ENTITYDEF, va( "%s_mp", name ), false );
	}
	if ( !decl ) {
#if GERMAN_VERSION
		decl = declManager->FindType( DECL_ENTITYDEF, va( "%s_nogore", name ), false );
		if ( !decl )
#else if !GOLD // For testing the german defs
		if ( g_nogore.GetBool() ) {
			decl = declManager->FindType( DECL_ENTITYDEF, va( "%s_nogore", name ), false );
		}
		if ( !decl ) // Fall through to normal mp decls below
#endif // Fall through to normal decls
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
	}
/* HUMANHEAD pdm: not used
	else if ( g_skill.GetInteger() == 0 ) {
		spawnArgs.GetBool( "not_easy", "0", result );
	} else if ( g_skill.GetInteger() == 1 ) {
		spawnArgs.GetBool( "not_medium", "0", result );
	} else {
		spawnArgs.GetBool( "not_hard", "0", result );
	}

	const char *name;
#ifndef ID_DEMO_BUILD
	if ( g_skill.GetInteger() == 3 ) { 
		name = spawnArgs.GetString( "classname" );
		if ( idStr::Icmp( name, "item_medkit" ) == 0 || idStr::Icmp( name, "item_medkit_small" ) == 0 ) {
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
*/

	return result;
}

/*
================
idGameLocal::SetSkill
================
*/
/*	HUMANHEAD pdm: not used
void idGameLocal::SetSkill( int value ) {
	if (gameLocal.isMultiplayer) {
		return;
	}

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
*/

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

//	SetSkill( g_skill.GetInteger() );		// HUMANHEAD pdm: not used

	numEntities = mapFile->GetNumEntities();
	if ( numEntities == 0 ) {
		Error( "...no entities" );
	}

	// the worldspawn is a special that performs any global setup
	// needed by a level
	mapEnt = mapFile->GetEntity( 0 );
	args = mapEnt->epairs;
	args.SetInt( "spawn_entnum", ENTITYNUM_WORLD );
	//HUMANHEAD rww - ok on client
	if ( !SpawnEntityDef( args, NULL, true, false, true ) || !entities[ ENTITYNUM_WORLD ] || !entities[ ENTITYNUM_WORLD ]->IsType( idWorldspawn::Type ) ) {
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

			//HUMANHEAD rww - ok on client
			SpawnEntityDef( args, NULL, true, false, true );
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
		Error( "Multiple entities named '%s'", name );
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

// HUMANHEAD pdm
void idGameLocal::ArgCompletion_ClassName( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	int i;
	idTypeInfo *type;

	int numTypes = idClass::GetNumTypes();
	for( i = 0; i < numTypes; i++ ) {
		type = idClass::GetType(i);
		callback( va( "%s %s", args.Argv( 0 ), type->classname ) );
	}
}

// HUMANHEAD pdm
void idGameLocal::ArgCompletion_EntityOrClassName( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	int i;
	idTypeInfo *type;

	int numTypes = idClass::GetNumTypes();
	for( i = 0; i < numTypes; i++ ) {
		type = idClass::GetType(i);
		callback( va( "%s %s", args.Argv( 0 ), type->classname ) );
	}
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
			// HUMANHEAD mdl:  maxCount was being ignored
			if ( entCount == maxCount ) {
				return entCount;
			}
			// HUMANHEAD END
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
			hit->SquishedByDoor( ent ); // HUMANHEAD CJR: Squish certain objects when a KillBox is applied
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

//HUMANHEAD PCF rww 05/15/06 - use a custom clipmask killbox (seperated due to pcf paranoia)
/*
================
idGameLocal::KillBoxMasked
================
*/
void idGameLocal::KillBoxMasked( idEntity *ent, int clipMask, bool catch_teleport ) {
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

	num = clip.ClipModelsTouchingBounds( phys->GetAbsBounds(), clipMask, clipModels, MAX_GENTITIES );
	for ( i = 0; i < num; i++ ) {
		cm = clipModels[ i ];

		// don't check render entities
		if ( cm->IsRenderModel() ) {
			continue;
		}

		hit = cm->GetEntity();
		if ( ( hit == ent ) || !hit->fl.takedamage ) {
			hit->SquishedByDoor( ent ); // HUMANHEAD CJR: Squish certain objects when a KillBox is applied
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
//HUMANHEAD END

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
/*	HUMANHEAD pdm: OVERRIDDEN
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
*/
}

/*
==============
idGameLocal::RadiusPush
==============
*/
void idGameLocal::RadiusPush( const idVec3 &origin, const float radius, const float push, const idEntity *inflictor, const idEntity *ignore, float inflictorScale, const bool quake ) {
/*	HUMANHEAD pdm: OVERRIDDEN
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
*/
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

		if (g_debugImpulse.GetBool()) {
			gameRenderWorld->DebugArrow(colorRed, center, center + impulse, 25, 2000);
		}

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

	// HUMANHEAD pdm
	if (g_debugProjections.GetBool()) {
		idVec4 color = parallel ? colorRed : colorYellow;
		gameRenderWorld->DebugWinding(color, winding, vec3_origin, mat3_identity, 3000, false);
		gameRenderWorld->DebugArrow(color, projectionOrigin, winding.GetCenter(), 5, 3000);
	}
	// HUMANHEAD END

	gameRenderWorld->ProjectDecalOntoWorld( winding, projectionOrigin, parallel, depth * 0.5f, declManager->FindMaterial( material ), time );
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

	// HUMANHEAD pdm: print area -> location mappings for efx integration
	if ( g_printlocations.GetBool() ) {
		idLocationEntity *ent;
#if 1
		idFile *areaFile = fileSystem->OpenFileWrite( va("%s_areas.txt", gameLocal.GetMapName()) );
		if (areaFile) {
			for (int ix=0; ix<numAreas; ix++) {
				ent = locationEntities[ix];
				areaFile->Printf( "%4d: %s\n", ix, ent ? ent->GetLocation() : "<No Location>" );
			}
			fileSystem->CloseFile( areaFile );
		}
#else
		idFile *areaFile = fileSystem->OpenFileWrite( va("%s_areas.csv", gameLocal.GetMapName()) );
		if (areaFile) {
			for (int ix=0; ix<numAreas; ix++) {
				ent = locationEntities[ix];
				idStr out = va("%d,%s\n", ix, ent ? ent->GetLocation() : "<No Location>");
				areaFile->Write( out.c_str(), out.Length() );
			}
			fileSystem->CloseFile( areaFile );
		}
#endif
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
	idEntity *ent;

	if ( !isMultiplayer || isClient ) {
		return;
	}
	spawnSpots.Clear();
	initialSpots.Clear();
	spot.dist = 0;

	// HUMANHEAD pdm: Coop
	if (isMultiplayer && IsCooperative()) {
		spot.ent = FindEntityUsingDef( NULL, "info_player_coop" );
		while( spot.ent ) {
			spawnSpots.Append( spot );
			if ( spot.ent->spawnArgs.GetBool( "initial" ) ) {
				initialSpots.Append( spot.ent );
			}
			spot.ent = FindEntityUsingDef( spot.ent, "info_player_coop" );
		}
	}
	else {
	// HUMANHEAD END

	spot.ent = FindEntityUsingDef( NULL, "info_player_deathmatch" );
	while( spot.ent ) {
		spawnSpots.Append( spot );
		if ( spot.ent->spawnArgs.GetBool( "initial" ) ) {
			initialSpots.Append( spot.ent );
		}
		spot.ent = FindEntityUsingDef( spot.ent, "info_player_deathmatch" );
	}

	}	// HUMANHEAD PDM
	if ( !spawnSpots.Num() ) {
		common->Warning( "no info_player_deathmatch in map" );
		return;
	}
	common->Printf( "%d spawns (%d initials)\n", spawnSpots.Num(), initialSpots.Num() );
	// if there are no initial spots in the map, consider they can all be used as initial
	if ( !initialSpots.Num() ) {
		//HUMANHEAD rww - we don't actually care.
		//common->Warning( "no info_player_deathmatch entities marked initial in map" );
		for ( i = 0; i < spawnSpots.Num(); i++ ) {
			initialSpots.Append( spawnSpots[ i ].ent );
		}
	}
	for ( i = 0; i < initialSpots.Num(); i++ ) {
		j = random.RandomInt( initialSpots.Num() );
		ent = initialSpots[ i ];
		initialSpots[ i ] = initialSpots[ j ];
		initialSpots[ j ] = ent;
	}
	// reset the counter
	currentInitialSpot = 0;
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

	if ( !isMultiplayer || !spawnSpots.Num() ) {
		spot.ent = FindEntityUsingDef( NULL, "info_player_start" );
		if ( !spot.ent ) {
			Error( "No info_player_start on map.\n" );
		}
		player->MPLastSpawnSpot = NULL; //HUMANHEAD rww
		return spot.ent;
	}
	if ( player->spectating ) {
		// plain random spot, don't bother
		player->MPLastSpawnSpot = NULL; //HUMANHEAD rww
		return spawnSpots[ random.RandomInt( spawnSpots.Num() ) ].ent;
	} else if ( player->useInitialSpawns && currentInitialSpot < initialSpots.Num() ) {
		player->MPLastSpawnSpot = NULL; //HUMANHEAD rww
		return initialSpots[ currentInitialSpot++ ];
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
			// don't do distance-based
			player->MPLastSpawnSpot = NULL; //HUMANHEAD rww
			return spawnSpots[ random.RandomInt( spawnSpots.Num() ) ].ent;
		}

		//HUMANHEAD rww
		idStaticList<spawnSpot_t *, 32> distantSpawnSpots;
		//HUMANHEAD END

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

			//HUMANHEAD rww
			if ((!player->MPLastSpawnSpot.IsValid() || player->MPLastSpawnSpot.GetEntity() != spawnSpots[i].ent) && spawnSpots[i].dist > (128.0f*128.0f) && distantSpawnSpots.Num() < 32) {
				distantSpawnSpots.Append(&spawnSpots[i]);
			}
			//HUMANHEAD END
		}

		//HUMANHEAD rww - take all spots > 128 units from another player and just pick a completely random one out of those
		if (distantSpawnSpots.Num() > 0) {
			spot = *distantSpawnSpots[random.RandomInt(distantSpawnSpots.Num())];
		}
		else {
		//HUMANHEAD END
			// sort the list
			qsort( ( void * )spawnSpots.Ptr(), spawnSpots.Num(), sizeof( spawnSpot_t ), ( int (*)(const void *, const void *) )sortSpawnPoints );

	#if 0
			// choose a random one in the top half
			which = random.RandomInt( spawnSpots.Num() / 2 );
	#else //HUMANHEAD rww - always pick the absolute farthest
			which = 0;
	#endif //HUMANHEAD END
			spot = spawnSpots[ which ];
		}
	}
	player->MPLastSpawnSpot = spot.ent; //HUMANHEAD rww
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
	} else if ( ( idStr::Icmp( serverInfo.GetString( "si_gameType" ), "notsupported1" ) == 0 ) ) { //HUMANHEAD rww
		gameType = GAME_TOURNEY;
	} else if ( ( idStr::Icmp( serverInfo.GetString( "si_gameType" ), "Team DM" ) == 0 ) ) {
		gameType = GAME_TDM;
	} else if ( ( idStr::Icmp( serverInfo.GetString( "si_gameType" ), "notsupported2" ) == 0 ) ) {  //HUMANHEAD rww
		gameType = GAME_LASTMAN;
	}
	// HUMANHEAD pdm
	else if ( ( idStr::Icmp( serverInfo.GetString( "si_gameType" ), "notsupported3" ) == 0 ) ) {  //HUMANHEAD rww (was coop)
		isCoop = idStr::Icmp( serverInfo.GetString( "si_gameType" ), "notsupported3" ) == 0;
	}
	// HUMANHEAD END

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
	//HUMANHEAD rww - take cent bits into account
	return ( gameLocal.spawnIds[ ent->entityNumber ] << GENTITYNUM_BITS_PLUSCENT ) | ent->entityNumber;
}

/*
================
idGameLocal::ThrottleUserInfo
================
*/
void idGameLocal::ThrottleUserInfo( void ) {
	mpGame.ThrottleUserInfo();
}

/*
===========
idGameLocal::SelectTimeGroup
============
*/
void idGameLocal::SelectTimeGroup( int timeGroup ) { }

/*
===========
idGameLocal::GetTimeGroupTime
============
*/
int idGameLocal::GetTimeGroupTime( int timeGroup ) {
	return gameLocal.time;
}

/*
===========
idGameLocal::GetBestGameType
============
*/
idStr idGameLocal::GetBestGameType( const char* map, const char* gametype ) {
	return gametype;
}

/*
===========
idGameLocal::NeedRestart
============
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
		//HUMANHEAD rww - seems appropriate to restart for spectator toggle too
		if ( keyval->GetValue().Cmp( keyval2->GetValue() ) && ( !keyval->GetKey().Cmp( "si_pure" ) || !keyval->GetKey().Cmp( "si_map" ) || !keyval->GetKey().Cmp( "si_spectators" ) ) ) {
			return true;
		}
	}
	return false;
}

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
	player = clientNum >= 0 ? static_cast<idPlayer *>( gameLocal.entities[ clientNum ] ) : NULL;

	if ( !player )
		return;

	int oldTeam = player->team;

	// Put in spectator mode
	if ( team == -1 ) {
		static_cast< idPlayer * >( entities[ clientNum ] )->Spectate( true );
	}
	// Switch to a team
	else {
		mpGame.SwitchToTeam ( clientNum, oldTeam, team );
	}
}


// HUMANHEAD mdl
void idGameLocal::RegisterUniqueObject( idClass *object ) {
	HH_ASSERT( uniqueObjects.FindIndex( object ) < 0 );
	HH_ASSERT( object->GetType() != &idClass::Type );
	uniqueObjects.Append( object );
}

void idGameLocal::UnregisterUniqueObject( idClass *object ) {
	if( !uniqueObjects.Remove( object ) ) {
		Warning("idGameLocal::UnregisterUniqueObject:  Object not found.");
	}
}
//HUMANHEAD END

//HUMANHEAD rww - if a gui is actually getting deleted, make sure no player has it in focus.
void idGameLocal::FocusGUICleanup(idUserInterface *gui) {
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (entities[i] && entities[i]->IsType(hhPlayer::Type)) {
			hhPlayer *pl = static_cast<hhPlayer *>(entities[i]);
			if (pl->ActiveGui() == gui) {
				pl->ClearFocus();
			}
		}
	}
}
//HUMANHEAD END

// HUMANHEAD pdm: print game side memory statistics
void idGameLocal::PrintMemInfo( MemInfo_t *mi ) {
	animationLib.PrintMemInfo( mi );

	//TODO: Add collision models, AAS
}

// HUMANHEAD pdm: Register locations for eax
void idGameLocal::RegisterLocationsWithSoundWorld() {
	int numAreas = gameRenderWorld->NumAreas();
	gameSoundWorld->ClearAreaLocations();
	for (int ix=0; ix<numAreas; ix++) {
		if (locationEntities[ix]) {
			gameSoundWorld->RegisterLocation(ix, locationEntities[ix]->GetLocation());
		}
	}
}

// HUMANHEAD pdm: call sound system to set the spiritwalk effects
void idGameLocal::SpiritWalkSoundMode(bool active) {
	if (gameSoundWorld) {
		if (active) {
			gameSoundWorld->FadeSoundClasses( SOUNDCLASS_NORMAL, -8.0f, 1.0f );
			gameSoundWorld->SetSpiritWalkEffect( true );
		}
		else {
			gameSoundWorld->FadeSoundClasses( SOUNDCLASS_NORMAL, 0.0f, 0.5f );
			gameSoundWorld->SetSpiritWalkEffect( false );
		}
	}
}

void idGameLocal::DialogSoundMode(bool active) {
	if (gameSoundWorld) {
		gameSoundWorld->SetVoiceDucker(active);
	}
}

bool idGameLocal::PlayerIsDeathwalking(void) {
	return false;
}

unsigned int idGameLocal::GetTimePlayed(void) {
	if (playTimeStart == (unsigned int) -1) {
		return playTime;
	} else {
		return playTime + (gameLocal.time - playTimeStart);
	}
}

void idGameLocal::ClearTimePlayed( void ) {
	playTime = 0;
	playTimeStart = -1;
}
// HUMANHEAD END
