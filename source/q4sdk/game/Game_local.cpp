#include "../idlib/precompiled.h"
#pragma hdrstop

#include "Game_local.h"

// RAVEN BEGIN
#include "../bse/BSEInterface.h"
#include "Projectile.h"
#include "client/ClientEffect.h"
#include "ai/AI.h"
#include "ai/AI_Manager.h"
#include "ai/AAS_tactical.h"
#include "Game_Log.h"
// RAVEN END

//#define UI_DEBUG	1

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

// RAVEN BEGIN
// jscott: game interface to the fx system
rvBSEManager *				bse = NULL;
// RAVEN END

idCVar *					idCVar::staticVars = NULL;

// RAVEN BEGIN
// rjohnson: new help system for cvar ui
idCVarHelp *				idCVarHelp::staticCVarHelps = NULL;
idCVarHelp *				idCVarHelp::staticCVarHelpsTail = NULL;
// RAVEN END

idCVar com_forceGenericSIMD( "com_forceGenericSIMD", "0", CVAR_BOOL|CVAR_SYSTEM, "force generic platform independent SIMD" );

#endif

idRenderWorld *				gameRenderWorld = NULL;		// all drawing is done to this world

static gameExport_t			gameExport;

// global animation lib
// RAVEN BEGIN
// jsinger: changed to a pointer to prevent its constructor from allocating
//          memory before the unified allocator could be initialized
idAnimManager				*animationLib = NULL;
// RAVEN END

// the rest of the engine will only reference the "game" variable, while all local aspects stay hidden
idGameLocal					gameLocal;
idGame *					game = &gameLocal;	// statically pointed at an idGameLocal

const char *idGameLocal::sufaceTypeNames[ MAX_SURFACE_TYPES ] = {
	"none",	"metal", "stone", "flesh", "wood", "cardboard", "liquid", "glass", "plastic",
	"ricochet", "surftype10", "surftype11", "surftype12", "surftype13", "surftype14", "surftype15"
};

/*
===========
GetGameAPI
============
*/

#if __GNUC__ >= 4
#pragma GCC visibility push(default)
#endif
extern "C" gameExport_t *GetGameAPI( gameImport_t *import ) {

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
// RAVEN BEGIN
// jscott: import the fx system
		bse							= import->bse;
// RAVEN END

// RAVEN BEGIN
// dluetscher: import the memory system variables
#ifdef _RV_MEM_SYS_SUPPORT
		::currentHeapArena			= import->heapArena;
		rvSetAllSysHeaps( import->systemHeapArray );
#endif
// RAVEN END
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
// RAVEN BEGIN
// bdube: game log
	gameExport.gameLog = gameLog;
// RAVEN END	

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

// RAVEN BEGIN
// jscott: import the fx system
	testImport.bse						= ::bse;
// RAVEN END

	testExport = *GetGameAPI( &testImport );
}

/*
================
idGameLocal::BuildModList
================
*/
void idGameLocal::BuildModList( ) {
	int i;
	idStr currentMod;

	int numServers = networkSystem->GetNumScannedServers();

	if ( filterMod >= 0 && filterMod < modList.Num() ) {
		currentMod = modList[ filterMod ];
	} else {
		currentMod = "";
	}

	modList.Clear();
	for (i = 0; i < numServers; i++) {
		const scannedServer_t *server;
		idStr modname;

		server = networkSystem->GetScannedServerInfo( i );

		server->serverInfo.GetString( "fs_game", "", modname );
		modname.ToLower();
		modList.AddUnique( modname );
	}

	modList.Sort();

	if ( modList.Num() > 0 && (modList[ 0 ].Cmp( "" ) == 0) ) {
		modList.RemoveIndex( 0 );
	}

	filterMod = modList.Num();
	for (i = 0; i < modList.Num(); i++) {
		if ( modList[ i ].Icmp( currentMod ) == 0 ) {
			filterMod = i;
		}
	}
}

/*
================
FilterByMod
================
*/
static int FilterByMod( const int* serverIndex ) {
	const scannedServer_t *server;

	if ( gameLocal.filterMod < 0 || gameLocal.filterMod >= gameLocal.modList.Num() ) {
		return (int)false;
	}

	server = networkSystem->GetScannedServerInfo( *serverIndex );

	return (int)(gameLocal.modList[ gameLocal.filterMod ].Icmp( server->serverInfo.GetString( "fs_game" ) ) != 0);
}

static sortInfo_t filterByMod = {
	SC_ALL,
	NULL,
	FilterByMod,
	"#str_123006"
};

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
	usercmds = NULL;
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
// RAVEN BEGIN
// bdube: not using id effects
//	testFx = NULL;
// ddynerman: multiple clip worlds
	ShutdownInstances();
// mwhitlock: Dynamic memory consolidation
	clip.Clear();
// RAVEN END
	
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		clientsPVS[ i ].i = -1;
		clientsPVS[ i ].h = -1;
	}
	freePlayerPVS = false;
	pvs.Shutdown();
	sessionCommand.Clear();
	locationEntities = NULL;
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

// RAVEN BEGIN
// abahr
	gravityInfo.Clear();
	scriptObjectProxies.Clear();
// RAVEN END

	mapFileName.Clear();
// RAVEN BEGIN
// rjohnson: entity usage stats
	mapFileNameStripped.Clear();
// RAVEN END
	mapFile = NULL;
	spawnCount = INITIAL_SPAWN_COUNT;
	memset( isMapEntity, 0, sizeof( bool ) * MAX_GENTITIES );

	camera = NULL;

// RAVEN BEGIN
// jscott: for portal skies
	portalSky = NULL;
// RAVEN END

	aasList.Clear();
	aasNames.Clear();
// RAVEN BEGIN
// bdube: added
	lastAIAlertEntity = NULL;
	lastAIAlertEntityTime = 0;
	lastAIAlertActor = NULL;
	lastAIAlertActorTime = 0;
// RAVEN END
	spawnArgs.Clear();
	gravity.Set( 0, 0, -1 );
	playerPVS.i = -1;
	playerPVS.h = -1;
	playerConnectedAreas.i = -1;
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
	entityDefBits = 0;

	nextGibTime = 0;
// RITUAL BEGIN
// squirrel: added DeadZone multiplayer mode
	unFreezeTime = 0;
	isFrozen = false;
// RITUAL END
	globalMaterial = NULL;
	newInfo.Clear();
	lastGUIEnt = NULL;
	lastGUI = 0;
	
	memset( clientEntityStates, 0, sizeof( clientEntityStates ) );
	memset( clientPVS, 0, sizeof( clientPVS ) );
	memset( clientSnapshots, 0, sizeof( clientSnapshots ) );

	eventQueue.Init();

	clientSpawnCount = INITIAL_SPAWN_COUNT;
	clientSpawnedEntities.Clear();
	memset( clientEntities, 0, sizeof( clientEntities ) );
	memset( clientSpawnIds, -1, sizeof( clientSpawnIds ) );

	gameDebug.Shutdown();
	gameLogLocal.Shutdown();
	currentThinkingEntity = NULL;

	memset( lagometer, 0, sizeof( lagometer ) );

	demoState = DEMO_NONE;
	serverDemo = false;
	timeDemo = false;

	memset( &usercmd, 0, sizeof( usercmd ) );
	memset( &oldUsercmd, 0, sizeof( oldUsercmd ) );
	followPlayer = -1;	// start free flying
	demo_hud = NULL;
	demo_mphud = NULL;
	demo_cursor = NULL;

	demo_protocol = 0;

	instancesEntityIndexWatermarks.Clear();
	clientInstanceFirstFreeIndex = MAX_CLIENTS;
	minSpawnIndex = MAX_CLIENTS;

	modList.Clear();
	filterMod = -1;
	if ( networkSystem ) {
		networkSystem->UseSortFunction( filterByMod, false );
	}
}

/*
===========
idGameLocal::Init

  initialize the game object, only happens once at startup, not each level load
============
*/
// RAVEN BEGIN
// jsinger: attempt to eliminate cross-DLL allocation issues
extern idHashTable<rvViseme> *visemeTable100;
extern idHashTable<rvViseme> *visemeTable66;
extern idHashTable<rvViseme> *visemeTable33;
#ifdef RV_UNIFIED_ALLOCATOR
void idGameLocal::Init( void *(*allocator)(size_t size), void (*deallocator)( void *ptr ), size_t (*msize)(void *ptr) ) {
#else
void idGameLocal::Init( void ) {
#endif
// RAVEN END
	const idDict *dict;
	idAAS *aas;

#ifndef GAME_DLL

	TestGameAPI();

#else

	mHz = common->GetUserCmdHz();
	msec = common->GetUserCmdMSec();

// RAVEN BEGIN
// jsinger: attempt to eliminate cross-DLL allocation issues
#ifdef RV_UNIFIED_ALLOCATOR
	Memory::InitAllocator(allocator, deallocator, msize);
#endif
// RAVEN END
	// initialize idLib
	idLib::Init();

	// register static cvars declared in the game
	idCVar::RegisterStaticVars();

	// initialize processor specific SIMD
	idSIMD::InitProcessor( "game", com_forceGenericSIMD.GetBool() );

#endif
// RAVEN BEGIN
// jsinger: these need to be initialized after the InitAllocator call above in order
//          to avoid crashes when the allocator is used.
	animationLib = new idAnimManager();
	visemeTable100 = new idHashTable<rvViseme>;
	visemeTable66 = new idHashTable<rvViseme>;
	visemeTable33 = new idHashTable<rvViseme>;
// RAVEN END

	Printf( "------------- Initializing Game -------------\n" );
	Printf( "gamename: %s\n", GAME_VERSION );
	Printf( "gamedate: %s\n", __DATE__ );

// RAVEN BEGIN
// rjohnson: new help system for cvar ui
	idCVarHelp::RegisterStatics();

// jsinger: added to support serialization/deserialization of binary decls
#ifdef RV_BINARYDECLS
	idStr prefix="";
	if(cvarSystem->GetCVarBool("com_binaryDeclRead"))
	{
		prefix = "binary/";
	}
	declManager->RegisterDeclType( "model",				DECL_MODELDEF,		idDeclAllocator<idDeclModelDef>, idDeclStreamAllocator<idDeclModelDef> );
	//declManager->RegisterDeclType( "export",			DECL_MODELEXPORT,	idDeclAllocator<idDecl>, idDeclStreamAllocator<idDecl> );
	declManager->RegisterDeclType( "camera",			DECL_CAMERADEF,		idDeclAllocator<idDeclCameraDef>, idDeclStreamAllocator<idDeclCameraDef> );
	declManager->RegisterDeclFolderWrapper( prefix + "def",			".def",			DECL_ENTITYDEF );
	declManager->RegisterDeclFolderWrapper( prefix + "af",			".af",			DECL_AF );
#else
// RAVEN END
	// register game specific decl types
	declManager->RegisterDeclType( "model",				DECL_MODELDEF,		idDeclAllocator<idDeclModelDef> );
	declManager->RegisterDeclType( "export",			DECL_MODELEXPORT,	idDeclAllocator<idDecl> );

// RAVEN BEGIN
// rjohnson: camera is now contained in a def for frame commands
	declManager->RegisterDeclType( "camera",			DECL_CAMERADEF,		idDeclAllocator<idDeclCameraDef> );
// RAVEN END
	// register game specific decl folders
// RAVEN BEGIN
#ifndef RV_SINGLE_DECL_FILE
	declManager->RegisterDeclFolderWrapper( "def",			".def",			DECL_ENTITYDEF );
// bdube: not used in quake 4
//	declManager->RegisterDeclFolder( "fx",					".fx",			DECL_FX );
//	declManager->RegisterDeclFolder( "particles",			".prt",			DECL_PARTICLE );
	declManager->RegisterDeclFolderWrapper( "af",			".af",			DECL_AF );
//	declManager->RegisterDeclFolderWrapper( "newpdas",		".pda",			DECL_PDA );
#else
	if(!cvarSystem->GetCVarBool("com_SingleDeclFile"))
	{
		declManager->RegisterDeclFolderWrapper( "def",			".def",			DECL_ENTITYDEF );
		declManager->RegisterDeclFolderWrapper( "af",			".af",			DECL_AF );
	}
	else
	{
		// loads the second set of decls from the file which will contain
		// modles, cameras
		declManager->LoadDeclsFromFile();
		declManager->FinishLoadingDecls();
	}
#endif
#endif  // RV_BINARYDECLS
// RAVEN END

	cmdSystem->AddCommand( "listModelDefs", idListDecls_f<DECL_MODELDEF>, CMD_FL_SYSTEM|CMD_FL_GAME, "lists model defs" );
	cmdSystem->AddCommand( "printModelDefs", idPrintDecls_f<DECL_MODELDEF>, CMD_FL_SYSTEM|CMD_FL_GAME, "prints a model def", idCmdSystem::ArgCompletion_Decl<DECL_MODELDEF> );

	Clear();

	idEvent::Init();
// RAVEN BEGIN
// jnewquist: Register subclasses explicitly so they aren't dead-stripped
	idClass::RegisterClasses();
// RAVEN END
	idClass::Init();

	InitConsoleCommands();
	// load default scripts
	program.Startup( SCRIPT_DEFAULT );
	
	// set up the aas
// RAVEN BEGIN
// ddynerman: added false as 2nd parameter, otherwise def will be created
	dict = FindEntityDefDict( "aas_types", false );
// RAVEN END
	if ( !dict ) {
		Error( "Unable to find entityDef for 'aas_types'" );
	}

	// allocate space for the aas
	const idKeyValue *kv = dict->MatchPrefix( "type" );
	while( kv != NULL ) {
// RAVEN BEGIN
// jnewquist: Tag scope and callees to track allocations using "new".
		MEM_SCOPED_TAG(tag,MA_AAS);
// RAVEN END
		aas = idAAS::Alloc();
		aasList.Append( aas );
		aasNames.Append( kv->GetValue() );
		kv = dict->MatchPrefix( "type", kv );
	}

	gamestate = GAMESTATE_NOMAP;

	Printf( "...%d aas types\n", aasList.Num() );
	Printf( "game initialized.\n" );
	Printf( "---------------------------------------------\n" );
	
// RAVEN BEGIN
// bdube: debug stuff
	gameDebug.Init();
	gameLogLocal.Init();

// jscott: facial animation init
	if( !FAS_Init( "annosoft" ) ) {
		common->Warning( "Failed to load viseme file" );
	}

// jnewquist: Tag scope and callees to track allocations using "new".
	MEM_SCOPED_TAG(tag,MA_RENDER);
// shouchard:  make sure ban list starts out in a known state
	banListLoaded = false;
	banListChanged = false;
	memset( clientGuids, 0, sizeof( clientGuids ) );
// RAVEN END

// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
#if defined(_RV_MEM_SYS_SUPPORT)
	for(int i=0;i<MAX_CLIENTS;i++)
	{
		persistentPlayerInfo[i].SetAllocatorHeap(rvGetSysHeap(RV_HEAP_ID_PERMANENT));
	}
	entityHash.SetAllocatorHeap(rvGetSysHeap(RV_HEAP_ID_LEVEL));
	gravityInfo.SetAllocatorHeap(rvGetSysHeap(RV_HEAP_ID_LEVEL));
	scriptObjectProxies.SetAllocatorHeap(rvGetSysHeap(RV_HEAP_ID_LEVEL));
	entityUsageList.SetAllocatorHeap(rvGetSysHeap(RV_HEAP_ID_LEVEL));
	ambientLights.SetAllocatorHeap(rvGetSysHeap(RV_HEAP_ID_LEVEL));
	instances.SetAllocatorHeap(rvGetSysHeap(RV_HEAP_ID_LEVEL));
	clip.SetAllocatorHeap(rvGetSysHeap(RV_HEAP_ID_LEVEL));
#endif
// RAVEN END

	networkSystem->AddSortFunction( filterByMod );
}

/*
===========
idGameLocal::Shutdown

  shut down the entire game
============
*/
void idGameLocal::Shutdown( void ) {

	int		i;

	if ( !common ) {
		return;
	}

// RAVEN BEGIN
// jscott: FAS
	FAS_Shutdown();
// shouchard:  clean up ban list stuff
	SaveBanList();
	FlushBanList();
// RAVEN END

	Printf( "--------------- Game Shutdown ---------------\n" );

	networkSystem->RemoveSortFunction( filterByMod );

	mpGame.Shutdown();

	MapShutdown();

	aasList.DeleteContents( true );
	aasNames.Clear();

	idAI::FreeObstacleAvoidanceNodes();

	// shutdown the model exporter
	idModelExport::Shutdown();

	idEvent::Shutdown();

	program.Shutdown();

	idClass::Shutdown();

	// clear list with forces
	idForce::ClearForceList();

	// free the program data
	program.FreeData();

	// delete the .map file
	delete mapFile;
	mapFile = NULL;

	// free the collision map
	collisionModelManager->FreeMap( GetMapName() );

// RAVEN BEGIN
// jscott: free up static objects
	for( i = 0; i < MAX_CLIENTS; i++ ) {
		userInfo[i].Clear();
		persistentPlayerInfo[i].Clear();
	}

	for( i = 0; i < entityUsageList.Num(); i++ ) {
		entityUsageList[i].Clear();
	}

	serverInfo.Clear();
	persistentLevelInfo.Clear();
	sessionCommand.Clear();
	mapFileName.Clear();
	mapFileNameStripped.Clear();
	entityUsageList.Clear();

	newInfo.Clear();
	spawnArgs.Clear();
	shakeSounds.Clear();
	aiManager.Clear();
// RAVEN END

	ShutdownConsoleCommands();

	// free memory allocated by class objects
	Clear();

	// shut down the animation manager
// RAVEN BEGIN
// jsinger: animationLib changed to a pointer
	animationLib->Shutdown();
// RAVEN END

// RAVEN BEGIN
// rjohnson: entity usage stats
	entityUsageList.Clear();
// RAVEN END

	freeView.Shutdown();

	Printf( "---------------------------------------------\n" );

	instances.DeleteContents( true );

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
// RAVEN BEGIN
// mekberg: added saveTypes
void idGameLocal::SaveGame( idFile *f, saveType_t saveType ) {
// RAVEN END
	int i;
	idEntity *ent;
	idEntity *link;

	//remove weapon effect entites from the world
	if(	GetLocalPlayer() && 
		!GetLocalPlayer()->IsInVehicle() &&	
		GetLocalPlayer()->weapon )	{
		
		GetLocalPlayer()->weapon->PreSave();
	}

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

// RAVEN BEGIN
// abahr: saving clientEntities
	rvClientEntity* clientEnt = NULL;
	for( i = 0; i < MAX_CENTITIES; ++i ) {
		clientEnt = clientEntities[ i ];
		if( !clientEnt ) {
			continue;
		}
//		if( clientEnt->IsType( rvClientEffect::GetClassType() )){
//			continue;
//		}
		savegame.AddObject( clientEnt );
	}
// RAVEN END

	// write out complete object list
	savegame.WriteObjectList();

	program.Save( &savegame );

	savegame.WriteInt( g_skill.GetInteger() );

	savegame.WriteDict( &serverInfo );

	savegame.WriteInt( numClients );
	for( i = 0; i < numClients; i++ ) {
// RAVEN BEGIN
// mekberg: don't write out userinfo. Grab from cvars
//		savegame.WriteDict( &userInfo[ i ] );
// RAVEN END
//		savegame.WriteUsercmd( usercmds[ i ] );
		savegame.WriteDict( &persistentPlayerInfo[ i ] );
	}

	for( i = 0; i < MAX_GENTITIES; i++ ) {
		savegame.WriteObject( entities[ i ] );
		savegame.WriteInt( spawnIds[ i ] );
	}

// RAVEN BEGIN
// abahr: more clientEntities saving
	for( i = 0; i < MAX_CENTITIES; i++ ) {
//		if( clientEntities[ i ] && clientEntities[ i ]->IsType( rvClientEffect::GetClassType() )){
//			continue;
//		}
		savegame.WriteObject( clientEntities[ i ] );
		savegame.WriteInt( clientSpawnIds[ i ] );
	}
// RAVEN END

	savegame.WriteInt( firstFreeIndex );
	savegame.WriteInt( num_entities );

	// enityHash is restored by idEntity::Restore setting the entity name.

	savegame.WriteObject( world );

	savegame.WriteInt( spawnedEntities.Num() );
	for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		savegame.WriteObject( ent );
	}

	savegame.WriteInt( scriptObjectProxies.Num() );
	for( i = 0; i < scriptObjectProxies.Num(); ++i ) {
		scriptObjectProxies[i].Save( &savegame );
	}
	// used to write clientSpawnedEntities.Num() then iterate through the spawn nodes
	// this is fragile however, as some elsewhere get those out of sync (mantis #70)
	int countClientSpawnedEntities = 0;
	for ( clientEnt = clientSpawnedEntities.Next(); clientEnt != NULL; clientEnt = clientEnt->spawnNode.Next() ) {
		countClientSpawnedEntities++;
	}
	if ( countClientSpawnedEntities != clientSpawnedEntities.Num() ) {
		common->Warning( "countClientSpawnedEntities %d != clientSpawnedEntities.Num() %d\n", countClientSpawnedEntities, clientSpawnedEntities.Num() );
	}
	savegame.WriteInt( countClientSpawnedEntities );
	for ( clientEnt = clientSpawnedEntities.Next(); clientEnt != NULL; clientEnt = clientEnt->spawnNode.Next() ) {
		savegame.WriteObject( clientEnt );
	}
	// same as above. haven't seen a problem with that one but I can't trust it either
	int countActiveEntities = 0;
	for ( ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() ) {
		countActiveEntities++;
	}
	if ( countActiveEntities != activeEntities.Num() ) {
		common->Warning( "countActiveEntities %d != activeEntities.Num() %d\n", countActiveEntities, activeEntities.Num() );
	}
	savegame.WriteInt( countActiveEntities );
	for ( ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() ) {
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
// RAVEN BEGIN
// bdube: no test fx
//	testFx = NULL;
// RAVEN END

	savegame.WriteString( sessionCommand );
// RAVEN BEGIN
// nmckenzie: Let the AI system save itself too.
	aiManager.Save( &savegame );
// RAVEN END

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
// RAVEN BEGIN
	savegame.WriteBool( isListenServer );
// RAVEN END

	savegame.WriteInt( localClientNum );

	// snapshotEntities is used for multiplayer only

	savegame.WriteInt( realClientTime );
	savegame.WriteBool( isNewFrame );

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

// RAVEN BEGIN
// bdube: added
	lastAIAlertActor.Save( &savegame );
	lastAIAlertEntity.Save( &savegame );
	savegame.WriteInt( lastAIAlertEntityTime );
	savegame.WriteInt( lastAIAlertActorTime );
// RAVEN END

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

// RAVEN BEGIN
// mekberg: added saveTypes and wrapped saveMessage
	if ( saveType != ST_AUTO && GetLocalPlayer() && GetLocalPlayer()->GetHud() ) {
		GetLocalPlayer()->SaveMessage();
	}

// jshepard: resume weapon operation
	if(	GetLocalPlayer() && 
		!GetLocalPlayer()->IsInVehicle() &&	
		GetLocalPlayer()->weapon )	{
		GetLocalPlayer()->weapon->PostSave();
	}
// RAVEN END
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
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	if ( ent && ent->IsType( idPlayer::GetClassType() ) ) {
// RAVEN END
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
	TIME_THIS_SCOPE( __FUNCLINE__);
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
// MERGE:FIXME - the following now gets into a recursive stack overflow when enemies are killed.  Not sure why.
// nmckenzie:
/*	if ( thread ) {
		thread->Warning( "%s", text );
	} else {*/
		common->Warning( "%s", text );
//	}
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

// RAVEN BEGIN
// scork: some model errors arrive here during validation which kills the whole process, so let's just warn about them instead...
	if ( common->DoingDeclValidation() ) {
		this->Warning( "%s", text );
		return;
	}
// RAVEN END

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
const idDict* idGameLocal::SetUserInfo( int clientNum, const idDict &userInfo, bool isClient ) {
	
	TIME_THIS_SCOPE( __FUNCLINE__);
	
	int i;
	bool modifiedInfo = false;

	this->isClient = isClient;

	if ( clientNum >= 0 && clientNum < MAX_CLIENTS ) {
		idGameLocal::userInfo[ clientNum ] = userInfo;

		// server sanity
		if ( !isClient ) {

			// don't let numeric nicknames, it can be exploited to go around kick and ban commands from the server
			if ( idStr::IsNumeric( this->userInfo[ clientNum ].GetString( "ui_name" ) ) ) {
#if UI_DEBUG
				common->Printf( "SetUserInfo: client %d changed name from %s to %s_\n", 
					clientNum, idGameLocal::userInfo[ clientNum ].GetString( "ui_name" ), idGameLocal::userInfo[ clientNum ].GetString( "ui_name" ) );
#endif
				idGameLocal::userInfo[ clientNum ].Set( "ui_name", va( "%s_", idGameLocal::userInfo[ clientNum ].GetString( "ui_name" ) ) );
				modifiedInfo = true;
			}
		
			// don't allow dupe nicknames
			for ( i = 0; i < numClients; i++ ) {
				if ( i == clientNum ) {
					continue;
				}
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
				if ( entities[ i ] && entities[ i ]->IsType( idPlayer::GetClassType() ) ) {
// RAVEN END
					if ( !idStr::Icmp( idGameLocal::userInfo[ clientNum ].GetString( "ui_name" ), idGameLocal::userInfo[ i ].GetString( "ui_name" ) ) ) {
#if UI_DEBUG
						common->Printf( "SetUserInfo: client %d changed name from %s to %s_ because of client %d\n", 
							clientNum, idGameLocal::userInfo[ clientNum ].GetString( "ui_name" ), idGameLocal::userInfo[ clientNum ].GetString( "ui_name" ), i );
#endif
						idGameLocal::userInfo[ clientNum ].Set( "ui_name", va( "%s_", idGameLocal::userInfo[ clientNum ].GetString( "ui_name" ) ) );
						modifiedInfo = true;
						i = -1;	// rescan
						continue;
					}
				}
			}
		}

// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( entities[ clientNum ] && entities[ clientNum ]->IsType( idPlayer::GetClassType() ) ) {
// RAVEN END
			modifiedInfo |= static_cast<idPlayer *>( entities[ clientNum ] )->UserInfoChanged();
		}

		if ( !isClient ) {
			// now mark this client in game
			mpGame.EnterGame( clientNum );
		}
	}

	if ( modifiedInfo ) {
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
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	if ( entities[ clientNum ] && entities[ clientNum ]->IsType( idPlayer::GetClassType() ) ) {
// RAVEN END
		return &userInfo[ clientNum ];
	}
	return NULL;
}

/*
===========
idGameLocal::IsClientActive
============
*/
bool idGameLocal::IsClientActive( int clientNum ) {
	if ( entities[ clientNum ] && entities[ clientNum ]->IsType( idPlayer::GetClassType() ) ) {
		return true;
	}

	return false;
}

/*
===========
idGameLocal::SetServerInfo
============
*/
void idGameLocal::SetServerInfo( const idDict &_serverInfo ) {
	TIME_THIS_SCOPE( __FUNCLINE__);
	
	idBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];
	bool timeLimitChanged = false;

// RAVEN BEGIN
// mekberg: clear announcer and reschedule time announcements
	if ( ( isClient || isListenServer ) && mpGame.GetGameState( ) && mpGame.GetGameState( )->GetMPGameState( ) == GAMEON &&
		serverInfo.GetInt( "si_timelimit" ) != _serverInfo.GetInt( "si_timelimit" ) ) {
		timeLimitChanged = true;
	}

	serverInfo = _serverInfo;

	if ( timeLimitChanged ) {
		mpGame.ClearAnnouncerSounds( );
		mpGame.ScheduleTimeAnnouncements( );
	}
// RAVEN END

	if ( isServer ) {
		
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
// RAVEN BEGIN
// jnewquist: Tag scope and callees to track allocations using "new".
	MEM_SCOPED_TAG(tag,MA_PARSER);
// RAVEN END
	bool sameMap = (mapFile && idStr::Icmp(mapFile->GetName(), mapName) == 0);

	networkSystem->SetLoadingText( mapName );

	InitAsyncNetwork();

	// these can changed based upon sp / mp
	mHz = common->GetUserCmdHz();
	msec = common->GetUserCmdMSec();

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
// RAVEN BEGIN
// rjohnson: added resolve for handling func_groups and other aspects.  Before, radiant would do this processing on a map destroying the original data
		mapFile->Resolve();
// RAVEN END
	}
	mapFileName = mapFile->GetName();
	
	assert(!idStr::Cmp(mapFileName, mapFile->GetName()));
	
// RAVEN BEGIN
// rjohnson: entity usage stats
	mapFileNameStripped = mapFileName;
	mapFileNameStripped.StripFileExtension();
	mapFileNameStripped.StripPath();

	const char*	entityFilter;

	gameLocal.serverInfo.GetString( "si_entityFilter", "", &entityFilter );
	if ( entityFilter && *entityFilter ) {
		mapFileNameStripped += " ";
		mapFileNameStripped += entityFilter;
	}
// RAVEN END

	// load the collision map
	networkSystem->SetLoadingText( common->GetLocalizedString( "#str_107668" ) );
	collisionModelManager->LoadMap( mapFile, false );

	numClients = 0;

	// initialize all entities for this game
	memset( entities, 0, sizeof( entities ) );
	usercmds = NULL;
	memset( spawnIds, -1, sizeof( spawnIds ) );
	spawnCount = INITIAL_SPAWN_COUNT;
	
	spawnedEntities.Clear();
	activeEntities.Clear();
	numEntitiesToDeactivate = 0;
	sortTeamMasters = false;
	sortPushers = false;
	lastGUIEnt = NULL;
	lastGUI = 0;

// RAVEN BEGIN
// bdube: client entities
	clientSpawnCount = INITIAL_SPAWN_COUNT;
	clientSpawnedEntities.Clear();
	memset ( clientSpawnIds, -1, sizeof(clientSpawnIds ) );
	memset ( clientEntities, 0, sizeof(clientEntities) );
	firstFreeClientIndex = 0;
// RAVEN END

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
// RAVEN BEGIN
// bdube: not using id effects
//	testFx			= NULL;
// RAVEN END

// RAVEN BEGIN
// bdube: merged
	lastAIAlertEntity = NULL;
	lastAIAlertEntityTime = 0;
	lastAIAlertActor = NULL;
	lastAIAlertActorTime = 0;
// RAVEN END
	
	previousTime	= 0;
	time			= 0;
	framenum		= 0;
	sessionCommand = "";
	nextGibTime		= 0;
// RITUAL BEGIN
// squirrel: added DeadZone multiplayer mode
	unFreezeTime	= 0;
	isFrozen		= 0;
// RITUAL END

	vacuumAreaNum = -1;		// if an info_vacuum is spawned, it will set this

// RAVEN BEGIN
// abahr
	gravityInfo.Clear();
	scriptObjectProxies.Clear();
// RAVEN END

	if ( !editEntities ) {
		editEntities = new idEditEntities;
	}

	if ( gameLocal.isMultiplayer ) {
		gravity.Set( 0, 0, -g_mp_gravity.GetFloat() );
	} else {
		gravity.Set( 0, 0, -g_gravity.GetFloat() );
	}

	spawnArgs.Clear();

// RAVEN BEGIN
// nmckenzie:
	//make sure we clear all reachabilities we marked as blocked!
	aiManager.UnMarkAllReachBlocked();
	aiManager.Clear();
// RAVEN END

	skipCinematic = false;
	inCinematic = false;
	cinematicSkipTime = 0;
	cinematicStopTime = 0;
	cinematicMaxSkipTime = 0;

// RAVEN BEGIN
// ddynerman: main world instance
	PACIFIER_UPDATE;
	AddInstance( 0, true );
	assert( instances.Num() == 1 && instances[ 0 ]->GetInstanceID() == 0 );
// RAVEN END
	pvs.Init();
// RAVEN BEGIN
// mwhitlock: Xenon texture streaming
#if defined(_XENON)
	// Experimental use of game's PVS for reducing amount of stuff streamed. Will
	// do this a bit more cleanly if I decide to keep this.
	extern idPVS* pvsForStreaming;
	pvsForStreaming=&pvs;
#endif
// RAVEN END

	playerPVS.i = -1;
	playerConnectedAreas.i = -1;

	// load navigation system for all the different monster sizes
	for( i = 0; i < aasNames.Num(); i++ ) {
		aasList[ i ]->Init( idStr( mapFileName ).SetFileExtension( aasNames[ i ] ).c_str(), mapFile->GetGeometryCRC() );
	}

// RAVEN BEGIN
// cdr: Obstacle Avoidance
	AI_MoveInitialize();
// RAVEN END

	FindEntityDef( "preCacheExtras", false );

	if ( !sameMap ) {
		mapFile->RemovePrimitiveData();
	}

// RAVEN BEGIN
// ddynerman: ambient light list
	ambientLights.Clear();
// RAVEN END
}

/*
===================
idGameLocal::LocalMapRestart
===================
*/
void idGameLocal::LocalMapRestart( int instance ) {
	int i, latchSpawnCount;

	Printf( "----------- Game Map Restart (%s) ------------\n", instance == -1 ? "all instances" : va( "instance %d", instance ) );

	// client always respawns everything, so make sure it picks up the right map entities
	if( instance == -1 || isClient ) {
		memset( isMapEntity, 0, sizeof(bool) * MAX_GENTITIES );
	} else {
		assert( instance >= 0 && instance < instances.Num() );

		for( int i = 0; i < instances[ instance ]->GetNumMapEntities(); i++ ) {
			if ( instances[ instance ]->GetMapEntityNumber( i ) >= 0 && instances[ instance ]->GetMapEntityNumber( i ) < MAX_GENTITIES ) {
				isMapEntity[ instances[ instance ]->GetMapEntityNumber( i ) ] = false;
			}
		}
	}

	gamestate = GAMESTATE_SHUTDOWN;

	for ( i = 0; i < MAX_CLIENTS; i++ ) {
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( entities[ i ] && entities[ i ]->IsType( idPlayer::GetClassType() ) && (isClient || instance == -1 || entities[ i ]->GetInstance() == instance ) ) {
// RAVEN END
			static_cast< idPlayer * >( entities[ i ] )->PrepareForRestart();
		}
	}

	eventQueue.Shutdown();

	MapClear( false, instance );

	// clear the sound system
	soundSystem->StopAllSounds( SOUNDWORLD_GAME );

	// clear icons
	iconManager->Shutdown();

	// the spawnCount is reset to zero temporarily to spawn the map entities with the same spawnId
	// if we don't do that, network clients are confused and don't show any map entities
	latchSpawnCount = spawnCount;
	spawnCount = INITIAL_SPAWN_COUNT;

	gamestate = GAMESTATE_STARTUP;

	program.Restart();

	InitScriptForMap();

	MapPopulate( instance );

	// once the map is populated, set the spawnCount back to where it was so we don't risk any collision
	// (note that if there are no players in the game, we could just leave it at it's current value)
	spawnCount = latchSpawnCount;

	// setup the client entities again
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( entities[ i ] && entities[ i ]->IsType( idPlayer::GetClassType() ) && (isClient || instance == -1 || entities[ i ]->GetInstance() == instance ) ) {
// RAVEN END
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
void idGameLocal::MapRestart( int instance ) {
	idBitMsg	outMsg;
	byte		msgBuf[MAX_GAME_MESSAGE_SIZE];
	idDict		newInfo;
	int			i;
	const idKeyValue *keyval, *keyval2;

	if ( isClient ) {
// RAVEN BEGIN
// ddynerman: check if gametype changed
		SetGameType();
// RAVEN END
		LocalMapRestart( instance );
	} else {

		if ( mpGame.PickMap( "", true ) ) {
			common->Warning( "map %s and gametype %s are incompatible, aborting map change", si_map.GetString(), si_gameType.GetString() );
			return;
		}

		newInfo = *cvarSystem->MoveCVarsToDict( CVAR_SERVERINFO );
		// this has to be after the cvars are moved to the dict
		for ( i = 0; i < newInfo.GetNumKeyVals(); i++ ) {

			keyval = newInfo.GetKeyVal( i );
			keyval2 = serverInfo.FindKey( keyval->GetKey() );
			if ( !keyval2 ) {
				break;
			}
			// a select set of si_ changes will cause a full restart of the server
			if ( keyval->GetValue().Icmp( keyval2->GetValue() ) &&
				( !keyval->GetKey().Icmp( "si_pure" ) || !keyval->GetKey().Icmp( "si_map" ) ) ) {
				break;
			}
//RAVEN BEGIN
			//asalmon: need to restart if the game type has changed but the map has not cause someone could be connecting
			if(  keyval->GetValue().Icmp( keyval2->GetValue() ) && (!keyval->GetKey().Icmp("si_gametype")))
			{
				break;
			}
//RAVEN END
		}
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "rescanSI" " " __FILE__ " " __LINESTR__ );

		SetGameType();

		mpGame.isBuyingAllowedRightNow = false;

		if ( i != newInfo.GetNumKeyVals() ) {
			gameLocal.sessionCommand = "nextMap";
		} else {
			outMsg.Init( msgBuf, sizeof( msgBuf ) );
			outMsg.WriteByte( GAME_RELIABLE_MESSAGE_RESTART );
			outMsg.WriteBits( 1, 1 );
			outMsg.WriteDeltaDict( serverInfo, NULL );
			networkSystem->ServerSendReliableMessage( -1, outMsg );

			LocalMapRestart( instance );
			mpGame.MapRestart();
		}
	}
}

/*
===================
idGameLocal::VerifyServerSettings_f
===================
*/
void idGameLocal::VerifyServerSettings_f( const idCmdArgs &args ) {
	gameLocal.mpGame.PickMap( si_gameType.GetString() );
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
	
	int instance = -1;
	if( args.Argc() > 1 ) {
		instance = atoi( args.Args( 1 ) );
		if( instance < 0 || instance >= gameLocal.GetNumInstances() ) {
			common->Warning( "idGameLocal::MapRestart_f() - Invalid instance '%d' specified\n", instance );
			return;
		}
		gameLocal.LocalMapRestart( instance );
		return;
	}

	gameLocal.MapRestart( instance );
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
	const char			*mapCycleList, *currentMap;

// RAVEN BEGIN
// rjohnson: traditional map cycle
//		si_mapCycle "mp/q4dm4;mp/q4dm5;mp/q4dm6"
	mapCycleList = si_mapCycle.GetString();
	if ( mapCycleList && strlen( mapCycleList ) ) {
		idLexer src;
		idToken token, firstFound;
		int		numMaps = 0;
		bool	foundMap = false;

		src.FreeSource();
		src.SetFlags( LEXFL_NOFATALERRORS | LEXFL_ALLOWPATHNAMES | LEXFL_ALLOWMULTICHARLITERALS | LEXFL_ALLOWBACKSLASHSTRINGCONCAT );
		src.LoadMemory( mapCycleList, strlen( mapCycleList ), "idGameLocal::NextMap" );
		if ( src.IsLoaded() ) {

			currentMap = si_map.GetString();
			while( src.ReadToken( &token ) ) {
				if ( token == ";" ) {
					continue;
				}
				numMaps++;

				if ( numMaps == 1 ) {
					// guarantee that we use a map no matter what ( or when we wrap )
					firstFound = token;
				}
				if ( foundMap ) {
					firstFound = token;
					break;
				}
				
				if ( idStr::Icmp( token, currentMap ) == 0 ) {
					foundMap = true;
				}
			}

			if ( firstFound != "" ) {
				si_map.SetString( firstFound );
				return true;
			}
		}

		return false;
	}
// RAVEN END

	if ( !g_mapCycle.GetString()[0] ) {
		Printf( common->GetLocalizedString( "#str_104294" ) );
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
		if ( !keyval2 || keyval->GetValue().Icmp( keyval2->GetValue() ) ) {
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
===============
idGameLocal::GetStartingIndexForInstance
===============
*/
int idGameLocal::GetStartingIndexForInstance( int instanceID ) {
	if ( isServer ) {
		assert( instancesEntityIndexWatermarks.Num() >= instanceID );
		if ( instanceID == 0 ) {
			return MAX_CLIENTS;
		} else {
			// the high watermark of the previous instance is the starting index of the next one
			return instancesEntityIndexWatermarks[ instanceID - 1 ];
		}
	} else {
		assert( instanceID == 0 );
		return clientInstanceFirstFreeIndex;
	}
}

/*
===============
idGameLocal::ServerSetEntityIndexWatermark
keep track of the entity layout at the server - specially when there are multiple instances ( tourney )
===============
*/
void idGameLocal::ServerSetEntityIndexWatermark( int instanceID ) {
	if ( isClient ) {
		return;
	}
	instancesEntityIndexWatermarks.AssureSize( instanceID + 1, MAX_CLIENTS );
	// make sure there is no drift. if a value was already set it has to match
	// otherwise that means the server is repopulating with different indexes, and that would likely lead to net corruption
	assert( instancesEntityIndexWatermarks[ instanceID ] == MAX_CLIENTS || instancesEntityIndexWatermarks[ instanceID ] == firstFreeIndex );
	instancesEntityIndexWatermarks[ instanceID ] = firstFreeIndex;
}

/*
===============
idGameLocal::ServerSetMinSpawnIndex
===============
*/
void idGameLocal::ServerSetMinSpawnIndex( void ) {
	if ( isClient ) {
		return;
	}
	// setup minSpawnIndex with enough headroom so gameplay entities don't cause bad offsets
	// only needed on server, clients are completely slaved up to server entity layout
	if ( !idStr::Icmp( serverInfo.GetString( "si_gameType" ), "Tourney" ) ) {
		minSpawnIndex = MAX_CLIENTS + GetNumMapEntities() * MAX_INSTANCES;
	}
}

/*
===================
idGameLocal::MapPopulate
===================
*/
void idGameLocal::MapPopulate( int instance ) {

// RAVEN BEGIN
// jnewquist: Tag scope and callees to track allocations using "new".
	MEM_SCOPED_TAG(tag,MA_ENTITY);
// RAVEN END

	if ( isMultiplayer ) {
		cvarSystem->SetCVarBool( "r_skipSpecular", false );
	}

	minSpawnIndex = MAX_CLIENTS;

	// parse the key/value pairs and spawn entities
// RAVEN BEGIN
// ddynerman: instance code
	// reload the instances
	if ( instance == -1 ) {
		int i;
		firstFreeIndex = MAX_CLIENTS;
		for ( i = 0; i < instances.Num(); i++ ) {
			if ( instances[ i ] ) {
				instances[ i ]->Restart();

				ServerSetEntityIndexWatermark( i );
			}
		}
	} else {
		assert( instance >= 0 && instance < instances.Num() );
		instances[ instance ]->Restart();
	}
// RAVEN END

	ServerSetMinSpawnIndex();

	// mark location entities in all connected areas
	SpreadLocations();

	// RAVEN BEGIN
	// ddynerman: prepare the list of spawn spots
	InitializeSpawns();
	// RAVEN END

	// execute pending events before the very first game frame
	// this makes sure the map script main() function is called
	// before the physics are run so entities can bind correctly
	Printf( "------------ Processing events --------------\n" );
	idEvent::ServiceEvents();
}

/*
===================
idGameLocal::InitFromNewMap
===================
*/
void idGameLocal::InitFromNewMap( const char *mapName, idRenderWorld *renderWorld, bool isServer, bool isClient, int randseed ) {

	TIME_THIS_SCOPE( __FUNCLINE__);
	
	this->isServer = isServer;
	this->isClient = isClient;
// RAVEN BEGIN
// ddynerman: listen server
	this->isListenServer = isServer && !cvarSystem->GetCVarBool( "net_serverDedicated" );
// RAVEN END
	this->isMultiplayer = isServer || isClient;

	if ( this->isMultiplayer )
		gameLocal.Error( "This mod is for singleplayer only" );

	PACIFIER_UPDATE;

//RAVEN BEGIN
//asalmon: stats for single player
	if (!this->isMultiplayer) {
#ifdef _MPBETA
		return;
#else
		statManager->EndGame();
#ifdef _XENON
		Live()->DeleteSPSession(true);
#endif
		statManager->Shutdown();
		statManager->Init();
		statManager->BeginGame();
		statManager->ClientConnect(0);
#ifdef _XENON
		Live()->CreateSPSession();
#endif
#endif // _MPBETA
	}
//RAVEN END

	if ( mapFileName.Length() ) {
		MapShutdown();
	}

	Printf( "-------------- Game Map Init ----------------\n" );

	gamestate = GAMESTATE_STARTUP;

	gameRenderWorld = renderWorld;

// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
#if defined(_RV_MEM_SYS_SUPPORT)
	animationLib->BeginLevelLoad();
#endif
	
// ddynerman: set gametype
	SetGameType();
// RAVEN END

	LoadMap( mapName, randseed );

	InitScriptForMap();

	MapPopulate();

	mpGame.Reset();

	mpGame.Precache();

// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
#if defined(_RV_MEM_SYS_SUPPORT)
	animationLib->EndLevelLoad();
#endif
// RAVEN END

	// free up any unused animations
// RAVEN BEGIN
// jsinger: animationLib changed to a pointer
	animationLib->FlushUnusedAnims();
// RAVEN END

	gamestate = GAMESTATE_ACTIVE;

	Printf( "---------------------------------------------\n" );
}

/*
=================
idGameLocal::InitFromSaveGame
=================
*/
bool idGameLocal::InitFromSaveGame( const char *mapName, idRenderWorld *renderWorld, idFile *saveGameFile ) {
	TIME_THIS_SCOPE( __FUNCLINE__);
	
	int i;
	int num;
	idEntity *ent;
	idDict si;

	if ( mapFileName.Length() ) {
		MapShutdown();
	}

	Printf( "---------- Game Map Init SaveGame -----------\n" );

	gamestate = GAMESTATE_STARTUP;

	gameRenderWorld = renderWorld;

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

	// precache any media specified in the map
	for ( i = 0; i < mapFile->GetNumEntities(); i++ ) {
		idMapEntity *mapEnt = mapFile->GetEntity( i );

		if ( !InhibitEntitySpawn( mapEnt->epairs ) ) {
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
// RAVEN BEGIN
// mekberg: don't read in userinfo. Grab from cvars
//		savegame.ReadDict( &userInfo[ i ] );
// RAVEN END
//		savegame.ReadUsercmd( usercmds[ i ] );
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

	// Precache the player
// RAVEN BEGIN
// bdube: changed so we actually cache stuff
	FindEntityDef( idPlayer::GetSpawnClassname() );

// abahr: saving clientEntities
	for( i = 0; i < MAX_CENTITIES; i++ ) {
		savegame.ReadObject( reinterpret_cast<idClass *&>( clientEntities[ i ] ) );
		savegame.ReadInt( clientSpawnIds[ i ] );

		// restore the entityNumber
		if ( clientEntities[ i ] != NULL ) {
			clientEntities[ i ]->entityNumber = i;
		}
	}
// RAVEN END

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

// RAVEN BEGIN
// abahr: save scriptObject proxies
	savegame.ReadInt( num );
	scriptObjectProxies.SetNum( num );
	for( i = 0; i < num; ++i ) {
		scriptObjectProxies[i].Restore( &savegame );
	}
// abahr: save client entity stuff
	rvClientEntity* clientEnt = NULL;
	savegame.ReadInt( num );
	for( i = 0; i < num; ++i ) {
		savegame.ReadObject( reinterpret_cast<idClass *&>( clientEnt ) );
		assert( clientEnt );
		if ( clientEnt ) {
			clientEnt->spawnNode.AddToEnd( clientSpawnedEntities );
		}
	}
// RAVEN END

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
// RAVEN BEGIN
// nmckenzie: Let the AI system load itself too.
	aiManager.Restore( &savegame );
// RAVEN END

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
// RAVEN BEGIN
	savegame.ReadBool( isListenServer );
// RAVEN END

	savegame.ReadInt( localClientNum );

	// snapshotEntities is used for multiplayer only

	savegame.ReadInt( realClientTime );
	savegame.ReadBool( isNewFrame );

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

// RAVEN BEGIN
// bdube: added
	lastAIAlertEntity.Restore( &savegame );
	savegame.ReadInt( lastAIAlertEntityTime );
	lastAIAlertActor.Restore( &savegame );
	savegame.ReadInt( lastAIAlertActorTime );
// RAVEN END

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

	// call the restore functions to read out all the data from the entities
	savegame.RestoreObjects();

	mpGame.Reset();

	mpGame.Precache();

	// free up any unused animations
// RAVEN BEGIN
// jsinger: animationLib changed to a pointer
	animationLib->FlushUnusedAnims();
// RAVEN END

	gamestate = GAMESTATE_ACTIVE;

	Printf( "--------------------------------------\n" );

	return true;
}

/*
===========
idGameLocal::MapClear
===========
*/
// RAVEN BEGIN
// ddynerman: multiple instances 
void idGameLocal::MapClear( bool clearClients, int instance ) {
// RAVEN END
	int i;

// RAVEN BEGIN
// bdube: delete client entities first since they reference real entities
	for( i = 0; i < MAX_CENTITIES; i++ ) {
		// on the server we need to keep around client entities bound to entities in our instance2
		if( gameLocal.isServer && gameLocal.GetLocalPlayer() && instance != -1 &&
			instance != gameLocal.GetLocalPlayer()->GetInstance() && 
			clientEntities[ i ] && clientEntities[ i ]->GetBindMaster() && 
			clientEntities[ i ]->GetBindMaster()->GetInstance() == gameLocal.GetLocalPlayer()->GetInstance() ) {
			continue;
		}
		delete clientEntities[ i ];
		clientEntities[ i ] = NULL;
		clientSpawnIds[ i ] = -1;
	}
// RAVEN END

	for( i = ( clearClients ? 0 : MAX_CLIENTS ); i < MAX_GENTITIES; i++ ) {
		if( instance >= 0 && entities[ i ] && entities[ i ]->GetInstance() != instance ) {
			continue;
		}

		delete entities[ i ];
		// ~idEntity is in charge of setting the pointer to NULL
		// it will also clear pending events for this entity
		assert( !entities[ i ] );
// RAVEN BEGIN
// see FIXME in idRestoreGame::Error
		entities[ i ] = NULL;
// RAVEN END
		spawnIds[ i ] = -1;
	}

	entityHash.Clear( 1024, MAX_GENTITIES );
// RAVEN BEGIN
// rjohnson: reset spawnedEntities during clear to ensure no left over pieces that get remapped to a new id ( causing bad snapshot reading )
	if ( instance == -1 ) {
		spawnedEntities.Clear();
	}
// RAVEN END

	if ( !clearClients ) {
		// add back the hashes of the clients/stuff in other instances
		for ( i = 0; i < MAX_GENTITIES; i++ ) {
			if ( !entities[ i ] ) {
				continue;
			}
			entityHash.Add( entityHash.GenerateKey( entities[ i ]->name.c_str(), true ), i );
// RAVEN BEGIN
// rjohnson: reset spawnedEntities during clear to ensure no left over pieces that get remapped to a new id ( causing bad snapshot reading )
			if ( instance == -1 ) {
				entities[ i ]->spawnNode.AddToEnd( spawnedEntities );
			}
// RAVEN END
		}
	}

// RAVEN BEGIN
// jscott: clear out portal skies
	portalSky = NULL;
// abahr:
	gravityInfo.Clear();
	scriptObjectProxies.Clear();
// RAVEN END

	delete frameCommandThread;
	frameCommandThread = NULL;

	if ( editEntities ) {
		delete editEntities;
		editEntities = NULL;
	}

	delete[] locationEntities;
	locationEntities = NULL;
	
// RAVEN BEGIN
// ddynerman: mp clear
	if( gameLocal.isMultiplayer ) {
		ClearForwardSpawns();
		for( i = 0; i < TEAM_MAX; i++ ) {
			teamSpawnSpots[ i ].Clear();
		}
		mpGame.ClearMap();
	}
	ambientLights.Clear();
// RAVEN END

	// set the free index back at MAX_CLIENTS for registering entities again
	// the deletion of map entities causes the index to go down, but it may not have been left exactly at 32
	// under such conditions, the map populate that will follow may be offset
	firstFreeIndex = MAX_CLIENTS;
}

// RAVEN BEGIN
// ddynerman: instance-specific clear
void idGameLocal::InstanceClear( void ) {
	// note: clears all ents EXCEPT those in the instance
	int i;

	for( i = 0; i < MAX_CENTITIES; i++ ) {
		delete clientEntities[ i ];
		assert( !clientEntities[ i ] );
		clientSpawnIds[ i ] = -1;
	}

	for( i = MAX_CLIENTS; i < MAX_GENTITIES; i++ ) {
		if( i == ENTITYNUM_CLIENT || i == ENTITYNUM_WORLD ) {
			continue;
		}

		if( entities[ i ] && entities[ i ]->fl.persistAcrossInstances ) {
			//common->DPrintf( "Instance %d: persistant: excluding ent from clear: %s (%s)\n", instance, entities[ i ]->name.c_str(), entities[ i ]->GetClassname() );
			continue;
		}

		//if( entities[ i ] ) {
			//Printf( "Instance %d: CLEARING ent from clear: %s (%s)\n", instance, entities[ i ]->name.c_str(), entities[ i ]->GetClassname() );
		//}
		
		delete entities[ i ];
		// ~idEntity is in charge of setting the pointer to NULL
		// it will also clear pending events for this entity
		assert( !entities[ i ] );
		// RAVEN BEGIN
		// see FIXME in idRestoreGame::Error
		entities[ i ] = NULL;
		// RAVEN END
		spawnIds[ i ] = -1;
	}

	entityHash.Clear( 1024, MAX_GENTITIES );

	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !entities[ i ] ) {
			continue;
		}
		entityHash.Add( entityHash.GenerateKey( entities[ i ]->name.c_str(), true ), i );
	}

// RAVEN BEGIN
// jscott: clear out portal skies
	portalSky = NULL;
	// abahr:
	gravityInfo.Clear();
	scriptObjectProxies.Clear();
// RAVEN END

	delete frameCommandThread;
	frameCommandThread = NULL;

	if ( editEntities ) {
		delete editEntities;
		editEntities = NULL;
	}

	delete[] locationEntities;
	locationEntities = NULL;

	// RAVEN BEGIN
	// ddynerman: mp clear
	if( gameLocal.isMultiplayer ) {
		ClearForwardSpawns();
		for( i = 0; i < TEAM_MAX; i++ ) {
			teamSpawnSpots[ i ].Clear();
		}
		mpGame.ClearMap();
	}
	ambientLights.Clear();

	
}
// RAVEN END

/*
===========
idGameLocal::MapShutdown
============
*/
void idGameLocal::MapShutdown( void ) {
	Printf( "------------ Game Map Shutdown --------------\n" );
	
	gamestate = GAMESTATE_SHUTDOWN;

	if ( soundSystem ) {
		soundSystem->ResetListener();
	}

// RAVEN BEGIN
// rjohnson: new blur special effect
	renderSystem->ShutdownSpecialEffects();
// RAVEN END

	// clear out camera if we're in a cinematic
	if ( inCinematic ) {
		camera = NULL;
		inCinematic = false;
	}

// RAVEN BEGIN
// jscott: cleanup playbacks
	gameEdit->ShutdownPlaybacks();
// RAVEN END

	MapClear( true );

	instancesEntityIndexWatermarks.Clear();
	clientInstanceFirstFreeIndex = MAX_CLIENTS;	

// RAVEN BEGIN
// jscott: make sure any spurious events are killed
	idEvent::ClearEventList();

	// reset the script to the state it was before the map was started
	program.Restart();

// bdube: game debug
	gameDebug.Shutdown( );
	gameLogLocal.Shutdown( );
// RAVEN END

	iconManager->Shutdown();

	pvs.Shutdown();

// RAVEN BEGIN
// ddynerman: MP multiple instances
	ShutdownInstances();
// mwhitlock: Dynamic memory consolidation
	clip.Clear();
// RAVEN END
	idClipModel::ClearTraceModelCache();

// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
#if defined(_RV_MEM_SYS_SUPPORT)
	idForce::ClearForceList();
#endif
// RAVEN END

	ShutdownAsyncNetwork();

	mapFileName.Clear();

	gameRenderWorld = NULL;

	gamestate = GAMESTATE_NOMAP;

	Printf( "---------------------------------------------\n" );
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
// RAVEN BEGIN
// rjohnson: changed path to raven's directories
// jnewquist: Use filesystem search path to get absolute file path
		idStr	tempFile;
		idFile *f = fileSystem->OpenFileRead( oggSounds[i] );
		if ( !f ) {
			continue;
		}
		size = f->Length();
		totalSize += size;
		tempFile = f->GetFullPath();
		fileSystem->CloseFile(f);
		f = NULL;
		tempFile.Replace( "/", "\\" );

// jnewquist: prevent alterations to files relative to cdpath
		const char *cdPath = cvarSystem->GetCVarString("fs_cdpath");
		const int cdPathLen = idStr::Length(cdPath);
		if ( cdPathLen > 0 && idStr::Icmpn(cdPath, tempFile, cdPathLen) == 0 ) {
			file->Printf( "rem Ignored file from CD path: %s\n", tempFile.c_str() );
			continue;
		}

		file->Printf( "echo %d / %d\n", i, oggSounds.Num() );
		file->Printf( "k:\\utility\\oggenc -q 0 \"%s\"\n", tempFile.c_str() );
		file->Printf( "del \"%s\"\n", tempFile.c_str() );
// RAVEN END
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
	idDict spawnerArgs;
	
	TIME_THIS_SCOPE( __FUNCLINE__);

	if ( dict == NULL ) {
#ifndef _CONSOLE
		if ( cvarSystem->GetCVarBool( "com_makingBuild") ) {
			DumpOggSounds();
		}
#endif
		return;
	}

#ifndef _CONSOLE
	if ( cvarSystem->GetCVarBool( "com_makingBuild" ) ) {
		GetShakeSounds( dict );
	}
#endif

	int numVals = dict->GetNumKeyVals();
	
	for ( int i = 0; i < numVals; ++i ) {
		const idKeyValue *kv = dict->GetKeyVal( i );
		
		#define MATCH(s) \
			(!kv->GetKey().Icmpn( s, strlen(s) ))
		/**/
		
		if ( !kv || !kv->GetValue().Length() ) {
			continue;
		}
		
		if ( MATCH( "model" ) ) {
			
			TIME_THIS_SCOPE( __FUNCLINE__);
			
			MEM_SCOPED_TAG(tag,MA_MODEL);
			declManager->MediaPrint( "Precaching model %s\n", kv->GetValue().c_str() );
			// precache model/animations
			if ( declManager->FindType( DECL_MODELDEF, kv->GetValue(), false ) == NULL ) {
				// precache the render model
				renderModelManager->FindModel( kv->GetValue() );
				// precache .cm files only
				collisionModelManager->PreCacheModel( GetMapName(), kv->GetValue() );
			}
		} else if ( MATCH( "s_shader" ) ) {
			
			TIME_THIS_SCOPE( __FUNCLINE__);
			
			MEM_SCOPED_TAG(tag,MA_SOUND);
			declManager->FindType( DECL_SOUND, kv->GetValue() );
		} else if ( MATCH( "snd_" ) ) {
			
			TIME_THIS_SCOPE( __FUNCLINE__);
			
			MEM_SCOPED_TAG(tag,MA_SOUND);
			declManager->FindType( DECL_SOUND, kv->GetValue() );
		} else if ( MATCH( "gui_" ) ) {
			
			TIME_THIS_SCOPE( __FUNCLINE__);
			
			MEM_SCOPED_TAG(tag,MA_GUI);
			if ( !idStr::Icmp( kv->GetKey(), "gui_noninteractive" )
				|| !idStr::Icmpn( kv->GetKey(), "gui_parm", 8 )	
				|| !idStr::Icmp( kv->GetKey(), "gui_inventory" ) ) {
				// unfortunate flag names, they aren't actually a gui
			} else {
				declManager->MediaPrint( "Precaching gui %s\n", kv->GetValue().c_str() );
				uiManager->FindGui( kv->GetValue().c_str(), true );
			}
		} else if ( MATCH( "texture" ) ) {
			
			TIME_THIS_SCOPE( __FUNCLINE__);
			
			MEM_SCOPED_TAG(tag,MA_MATERIAL);
			declManager->FindType( DECL_MATERIAL, kv->GetValue() );
		} else if ( MATCH( "mtr_" ) ) {
			
			TIME_THIS_SCOPE( __FUNCLINE__);
			
			MEM_SCOPED_TAG(tag,MA_MATERIAL);
			declManager->FindType( DECL_MATERIAL, kv->GetValue() );
		} else if ( MATCH( "screenShot" ) ) {
		
			TIME_THIS_SCOPE( __FUNCLINE__);
			
			MEM_SCOPED_TAG(tag,MA_MATERIAL);
			declManager->FindType( DECL_MATERIAL, kv->GetValue() );
		} else if ( MATCH( "inv_icon" ) ) {
			
			TIME_THIS_SCOPE( __FUNCLINE__);
			
			MEM_SCOPED_TAG(tag,MA_MATERIAL);
			declManager->FindType( DECL_MATERIAL, kv->GetValue() );
		} else if ( MATCH( "teleport" ) ) {
			
			TIME_THIS_SCOPE( __FUNCLINE__);
			
			MEM_SCOPED_TAG(tag,MA_EFFECT);
			int teleportType = atoi( kv->GetValue() );
			const char *p = ( teleportType ) ? va( "effects/teleporter%i.fx", teleportType ) : "effects/teleporter.fx";
			declManager->FindType( DECL_EFFECT, p );
		} else if ( MATCH( "fx_" ) ) {
			
			TIME_THIS_SCOPE( __FUNCLINE__);
			
			MEM_SCOPED_TAG(tag,MA_EFFECT);
			declManager->MediaPrint( "Precaching fx %s\n", kv->GetValue().c_str() );
			declManager->FindEffect( kv->GetValue() );
		} else if ( MATCH( "skin" ) ) {
			
			TIME_THIS_SCOPE( __FUNCLINE__);
			
			MEM_SCOPED_TAG(tag,MA_MATERIAL);
			declManager->MediaPrint( "Precaching skin %s\n", kv->GetValue().c_str() );
			declManager->FindType( DECL_SKIN, kv->GetValue() );
		} else if ( MATCH( "def_" ) ) {
			
			TIME_THIS_SCOPE( __FUNCLINE__);
			
			MEM_SCOPED_TAG(tag,MA_DECL);
			FindEntityDef( kv->GetValue().c_str(), false );
		} else if ( MATCH( "playback_" ) ) {
			
			TIME_THIS_SCOPE( __FUNCLINE__);
			
			MEM_SCOPED_TAG(tag,MA_ANIM);
			declManager->MediaPrint( "Precaching playback %s\n", kv->GetValue().c_str() );
			declManager->FindPlayback( kv->GetValue() );
		} else if ( MATCH( "lipsync_" ) ) {
			
			TIME_THIS_SCOPE( __FUNCLINE__);
			
			MEM_SCOPED_TAG(tag,MA_ANIM);
			declManager->MediaPrint( "Precaching lipsync %s\n", kv->GetValue().c_str() );
			declManager->FindLipSync( kv->GetValue() );
			declManager->FindSound ( kv->GetValue() );
		} else if ( MATCH( "icon " ) ) {
			
			TIME_THIS_SCOPE( __FUNCLINE__);
			
			MEM_SCOPED_TAG(tag,MA_MATERIAL);
			idLexer  src ( LEXFL_ALLOWPATHNAMES );
			idToken  token;
			idToken	 token2;
			src.LoadMemory( kv->GetValue(), kv->GetValue().Length(), "icon" );
			
			src.ReadToken ( &token );
			if ( src.CheckTokenString ( "," ) ) {
				int x, y, w, h;
				src.ReadToken ( &token2 ) ;
				x = token2.GetIntValue ( );
				src.ExpectTokenString ( "," );
				
				src.ReadToken ( &token2 ) ;
				y = token2.GetIntValue ( );
				src.ExpectTokenString ( "," );

				src.ReadToken ( &token2 ) ;
				w = token2.GetIntValue ( );
				src.ExpectTokenString ( "," );

				src.ReadToken ( &token2 ) ;
				h = token2.GetIntValue ( );
				
				uiManager->RegisterIcon ( kv->GetKey ( ).c_str() + 5, token, x, y, w, h );
			} else { 
				uiManager->RegisterIcon ( kv->GetKey ( ).c_str() + 5, token );
			}
		} else if ( MATCH( "spawn_" ) ) {
			
			TIME_THIS_SCOPE( __FUNCLINE__);
			
			MEM_SCOPED_TAG(tag,MA_DECL);
			spawnerArgs.Set ( kv->GetKey ( ).c_str() + 6, kv->GetValue ( ) );
		}
		
		#undef MATCH
	}

	if ( spawnerArgs.GetNumKeyVals() ) {
		CacheDictionaryMedia ( &spawnerArgs );
	}
// RAVEN END
}

/*
===========
idGameLocal::InitScriptForMap
============
*/
void idGameLocal::InitScriptForMap( void ) {

// RAVEN BEGIN
// jnewquist: Tag scope and callees to track allocations using "new".
	MEM_SCOPED_TAG(tag,MA_DEFAULT);
// RAVEN END
	// create a thread to run frame commands on
	frameCommandThread = new idThread();
	frameCommandThread->ManualDelete();
	frameCommandThread->SetThreadName( "frameCommands" );


// jnewquist: Tag scope and callees to track allocations using "new".
	MEM_SCOPED_TAG_SET(tag,MA_SCRIPT);

	// run the main game script function (not the level specific main)
	const function_t *func = program.FindFunction( SCRIPT_DEFAULTFUNC );


// jnewquist: Tag scope and callees to track allocations using "new".
	MEM_SCOPED_TAG_SET(tag,MA_DEFAULT);

	if ( func != NULL ) {
		idThread *thread = new idThread( func );
		if ( thread->Start() ) {
			// thread has finished executing, so delete it
			delete thread;
		}
	}

// RAVEN END

}

/*
===========
idGameLocal::SpawnPlayer
============
*/
void idGameLocal::SpawnPlayer( int clientNum ) {

	TIME_THIS_SCOPE( __FUNCLINE__);

	idEntity	*ent;
	idDict		args;
// RAVEN BEGIN
// jnewquist: Tag scope and callees to track allocations using "new".
	MEM_SCOPED_TAG(tag,MA_ENTITY);
// RAVEN END

	// they can connect
	common->DPrintf( "SpawnPlayer: %i\n", clientNum );

	args.SetInt( "spawn_entnum", clientNum );
	args.Set( "name", va( "player%d", clientNum + 1 ) );
// RAVEN BEGIN
// bdube: changed marine class
	args.Set( "classname", idPlayer::GetSpawnClassname() );
// RAVEN END
	
	// This takes a really long time.
	PACIFIER_UPDATE;
	if ( !SpawnEntityDef( args, &ent ) || !entities[ clientNum ] ) {
		Error( "Failed to spawn player as '%s'", args.GetString( "classname" ) );
	}

	// make sure it's a compatible class
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	if ( !ent->IsType( idPlayer::GetClassType() ) ) {
// RAVEN END
		Error( "'%s' spawned the player as a '%s'.  Player spawnclass must be a subclass of idPlayer.", args.GetString( "classname" ), ent->GetClassname() );
	}

	if ( clientNum >= numClients ) {
		numClients = clientNum + 1;
	}

	PACIFIER_UPDATE;
	mpGame.SpawnPlayer( clientNum );
}

/*
================
idGameLocal::GetClientByNum
================
*/
idPlayer *idGameLocal::GetClientByNum( int current ) const {
	if ( current == MAX_CLIENTS ) {
		return NULL;
	}
	if ( current < 0 || current >= numClients ) {
		// that's a bit nasty but I suppose it has it's use
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
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( ent && ent->IsType( idPlayer::GetClassType() ) ) {
// RAVEN END
// RAVEN BEGIN
// bdube: escape codes
			if ( idStr::IcmpNoEscape( name, userInfo[ i ].GetString( "ui_name" ) ) == 0 ) {
// RAVEN END			
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
idGameLocal::GetClientNumByName
================
*/
int idGameLocal::GetClientNumByName( const char *name ) const {
	int i;
	idEntity *ent;
	for ( i = 0 ; i < numClients ; i++ ) {
		ent = entities[ i ];

		if ( ent && ent->IsType( idPlayer::GetClassType() ) ) {
			if ( idStr::IcmpNoEscape( name, userInfo[ i ].GetString( "ui_name" ) ) == 0 ) {
				return i;
			}
		}
	}
	return -1;
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
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( entities[ current ] && entities[ current ]->IsType( idPlayer::GetClassType() ) ) {
// RAVEN END
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
draw phase even happening.

localClientNum == MAX_CLIENTS when playing server demos
don't send back the wrong entity obviously
================
*/
idPlayer *idGameLocal::GetLocalPlayer() const {
	if ( localClientNum < 0 || localClientNum == MAX_CLIENTS ) {
		return NULL;
	}

	if ( !entities[ localClientNum ] || !entities[ localClientNum ]->IsType( idPlayer::GetClassType() ) ) {
		// not fully in game yet
		return NULL;
	}
	
	// through idGameLocal::MapShutdown, deleting player ents, calls idEntity::StopSound
	// calls in here and triggers the assert because proper type info is already gone
	//assert( gamestate == GAMESTATE_SHUTDOWN || entities[ localClientNum ]->IsType( idPlayer::GetClassType() ) );
	
	return static_cast<idPlayer *>( entities[ localClientNum ] );
}

/*
================
idGameLocal::SetupClientPVS
for client spectating others, get the pvs of spectated
================
*/
pvsHandle_t idGameLocal::GetClientPVS( idPlayer *player, pvsType_t type ) {
	if ( player->GetPrivateCameraView() ) {
		return pvs.SetupCurrentPVS( player->GetPrivateCameraView()->GetPVSAreas(), player->GetPrivateCameraView()->GetNumPVSAreas() );
	} else if ( camera ) {
		return pvs.SetupCurrentPVS( camera->GetPVSAreas(), camera->GetNumPVSAreas() );
	} else {
		if ( player->spectating && player->spectator != player->entityNumber && entities[ player->spectator ] ) {
			player = static_cast<idPlayer*>( entities[ player->spectator ] );
		}
		return pvs.SetupCurrentPVS( player->GetPVSAreas(), player->GetNumPVSAreas() );
	}
}

// RAVEN BEGIN
// jscott: for portal skies
/*
================
idGameLocal::GetSpawnCount
================
*/
int idGameLocal::GetSpawnCount ( void ) const {
	return spawnCount;
}

/*
================
idGameLocal::SetupPortalSkyPVS
================
*/
bool idGameLocal::SetupPortalSkyPVS( idPlayer *player ) {

	int			i, count, numAreas;
	const int	*areaNums;
	bool		*visibleAreas;

	if( !portalSky ) {

		return( false );
	}

	// Allocate room for the area flags
	numAreas = gameRenderWorld->NumAreas();
	visibleAreas = ( bool * )_alloca( numAreas );
	memset( visibleAreas, 0, numAreas );

	// Grab the areas the player can see....
	count = player->GetNumPVSAreas();
	areaNums = player->GetPVSAreas();
	for( i = 0; i < count; i++ ) {

		// Work out the referenced areas
		gameRenderWorld->FindVisibleAreas( player->GetPhysics()->GetOrigin(), areaNums[i], visibleAreas );
	}

	// Do any of the visible areas have a skybox?
	for( i = 0; i < numAreas; i++ ) {

		if( !visibleAreas[i] ) {

			continue;
		}

		if( gameRenderWorld->HasSkybox( i ) ) {

			break;
		}
	}

	// .. if any one has a skybox component, then merge in the portal sky
	return ( i != numAreas );
}
// RAVEN END

/*
===============
idGameLocal::UpdateClientsPVS
===============
*/
void idGameLocal::UpdateClientsPVS( void ) {
	int i;
	for ( i = 0; i < numClients; i++ ) {
		if ( !entities[ i ] ) {
			continue;
		}
		assert( clientsPVS[ i ].i == -1 );
		clientsPVS[ i ] = GetClientPVS( static_cast< idPlayer * >( entities[ i ] ), PVS_NORMAL );
	}
}

/*
================
idGameLocal::SetupPlayerPVS
================
*/
void idGameLocal::SetupPlayerPVS( void ) {
	int			i = 0;
	idPlayer *	player = NULL;
	pvsHandle_t	otherPVS, newPVS;

	UpdateClientsPVS( );

	playerPVS.i = -1;
	for ( i = 0; i < numClients; i++ ) {
		if ( !entities[i] ) {
			return;
		}
		assert( entities[i]->IsType( idPlayer::GetClassType() ) );

		player = static_cast<idPlayer *>( entities[ i ] );

		if ( playerPVS.i == -1 ) {
			playerPVS = clientsPVS[ i ];
			freePlayerPVS = false;	// don't try to free it as long as we stick to the client PVS
		} else {
			otherPVS = clientsPVS[ i ];
			newPVS = pvs.MergeCurrentPVS( playerPVS, otherPVS );
			if ( freePlayerPVS ) {
				pvs.FreeCurrentPVS( playerPVS );
				freePlayerPVS = false;
			}
			playerPVS = newPVS;
			freePlayerPVS = true;	// that merged one will need to be freed
		}

// RAVEN BEGIN
// jscott: for portal skies
		portalSkyVisible = SetupPortalSkyPVS( player );
		if ( portalSkyVisible ) {

			otherPVS = pvs.SetupCurrentPVS( portalSky->GetPVSAreas(), portalSky->GetNumPVSAreas() );
			newPVS = pvs.MergeCurrentPVS( playerPVS, otherPVS );

			if ( freePlayerPVS ) {
				pvs.FreeCurrentPVS( playerPVS );
				freePlayerPVS = false;
			}
			pvs.FreeCurrentPVS( otherPVS );
			playerPVS = newPVS;
			freePlayerPVS = true;
		}
// RAVEN END

		if ( playerConnectedAreas.i == -1 ) {
			playerConnectedAreas = GetClientPVS( player, PVS_CONNECTED_AREAS );
		} else {
			otherPVS = GetClientPVS( player, PVS_CONNECTED_AREAS );
			newPVS = pvs.MergeCurrentPVS( playerConnectedAreas, otherPVS );
			pvs.FreeCurrentPVS( playerConnectedAreas );
			pvs.FreeCurrentPVS( otherPVS );
			playerConnectedAreas = newPVS;
		}
	}
}

/*
================
idGameLocal::FreePlayerPVS
================
*/
void idGameLocal::FreePlayerPVS( void ) {
	int i;

	// only clear playerPVS if it's a different handle than the one in clientsPVS
	if ( freePlayerPVS && playerPVS.i != -1 ) {
		pvs.FreeCurrentPVS( playerPVS );
		freePlayerPVS = false;
	}
	playerPVS.i = -1;
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( clientsPVS[ i ].i >= 0 ) {
			pvs.FreeCurrentPVS( clientsPVS[ i ] );
			clientsPVS[i].i = -1;
		}
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
	
	idCVar* gravityCVar = NULL;

	if( gameLocal.isMultiplayer ) {
		gravityCVar = &g_mp_gravity;
	} else {
		gravityCVar = &g_gravity;
	}

	if ( gravityCVar->IsModified() ) {
		if ( gravityCVar->GetFloat() == 0.0f ) {
			gravityCVar->SetFloat( 1.0f );
		}
        gravity.Set( 0, 0, -gravityCVar->GetFloat() );

		// update all physics objects
		for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
			if ( ent->IsType( idAFEntity_Generic::GetClassType() ) ) {
// RAVEN END
				idPhysics *phys = ent->GetPhysics();
				if ( phys ) {
					phys->SetGravity( gravity );
				}
// RAVEN BEGIN
// ddynerman: jump pads
			} else if ( ent->IsType( rvJumpPad::GetClassType() ) ) {
				ent->PostEventMS( &EV_FindTargets, 0 );
			}
// RAVEN END
		}
		gravityCVar->ClearModified();
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
		// Sort bind masters first
		for ( ent = activeEntities.Next(); ent != NULL; ent = next_ent ) {
			next_ent = ent->activeNode.Next();
			for ( part = ent->GetBindMaster ( ); part; part = part->GetBindMaster ( ) ) {
				// Ensure we dont rerun the whole active entity list if our cached next_ent is one 
				// of the entities we are moving
				if ( next_ent == part ) {
					next_ent = next_ent->activeNode.Next();
					part = ent->GetBindMaster ( );
					continue;
				}
				part->activeNode.Remove();
				part->activeNode.AddToFront( activeEntities );												
			}
		}				
	
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
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
					if ( part->GetPhysics()->IsType( idPhysics_Actor::GetClassType() ) ) {
// RAVEN END
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
					if ( part->GetPhysics()->IsType( idPhysics_Parametric::GetClassType() ) ) {
						break;
					}
					if ( part->GetPhysics()->IsType( rvPhysics_Spline::GetClassType() ) ) {
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

/*
================
idGameLocal::MenuFrame
Called each session frame when a map is not running (e.g. usually in the main menu)
================
*/
void idGameLocal::MenuFrame( void ) { }

/*
================
idGameLocal::RunFrame
================
*/
// RAVEN BEGIN
 gameReturn_t idGameLocal::RunFrame( const usercmd_t *clientCmds, int activeEditors, bool lastCatchupFrame, int serverGameFrame ) {
	idEntity *	ent;
	int			num;
	float		ms;
	idTimer		timer_think, timer_events, timer_singlethink;
	idTimer		timer_misc, timer_misc2;
	gameReturn_t ret;
	idPlayer	*player;
	const renderView_t *view;

	editors = activeEditors;
	isLastPredictFrame = lastCatchupFrame;

	assert( !isClient );

	player = GetLocalPlayer();

	if ( !isMultiplayer && g_stopTime.GetBool() ) {

		// set the user commands for this frame
		usercmds = clientCmds;

		if ( player ) {
			// ddynerman: save the current thinking entity for instance-dependent
			currentThinkingEntity = player;
			player->Think();
			currentThinkingEntity = NULL;
		}
	} else do {
		// update the game time
		framenum++;
		previousTime = time;
		// bdube: use GetMSec access rather than USERCMD_TIME
		time += GetMSec();

		realClientTime = time;
		{
TIME_THIS_SCOPE("idGameLocal::RunFrame - gameDebug.BeginFrame()");
		// bdube: added advanced debug support
		gameDebug.BeginFrame( );
		gameLogLocal.BeginFrame( time );
		}

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

		// If modview is running then let it think
		common->ModViewThink( );	

		// rjohnson: added option for guis to always think
		common->RunAlwaysThinkGUIs( time );

		// nmckenzie: Let AI System stuff update itself.
		if ( !isMultiplayer ) {
#ifndef _MPBETA
			aiManager.RunFrame();
#endif // !_MPBETA
		}

		timer_misc.Start();
		
		// set the user commands for this frame
		usercmds = clientCmds;

		// create a merged pvs for all players
		// do this before we process events, which may rely on PVS info
		SetupPlayerPVS();

		// process events on the server
		ServerProcessEntityNetworkEventQueue();

		// update our gravity vector if needed.
		UpdateGravity();

		if ( isLastPredictFrame ) {
			// jscott: effect system uses gravity and the player PVS
			bse->StartFrame();
		}

		// sort the active entity list
		SortActiveEntityList();

		timer_think.Clear();
		timer_think.Start();

		// jscott: for timing and effect handling
		timer_misc2.Start();
		timer_misc.Stop();

		// let entities think
		if ( g_timeentities.GetFloat() ) {
			// rjohnson: will now draw entity info for long thinkers
			idPlayer *player;
			idVec3 origin;

			player = GetLocalPlayer();
			if ( player ) {
				origin = player->GetPhysics()->GetOrigin();
			}

			idBounds	viewTextBounds( origin );
			viewTextBounds.ExpandSelf( 128.0f );

			num = 0;

			for( ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() ) {
				if ( g_cinematic.GetBool() && inCinematic && !ent->cinematic ) {
					ent->GetPhysics()->UpdateTime( time );
					continue;
				}
				timer_singlethink.Clear();
				timer_singlethink.Start();
				// ddynerman: save the current thinking entity for instance-dependent
				currentThinkingEntity = ent;
				ent->Think();
				currentThinkingEntity = NULL;
				timer_singlethink.Stop();
				ms = timer_singlethink.Milliseconds();
				if ( ms >= g_timeentities.GetFloat() ) {
					// rjohnson: will now draw entity info for long thinkers
					Printf( "%d: entity '%s' [%s]: %.1f ms\n", time, ent->name.c_str(), ent->GetPhysics()->GetOrigin().ToString(), ms );

					if ( ms >= g_timeentities.GetFloat() * 3.0f )
					{
						ent->mLastLongThinkColor = colorRed;
					}
					else
					{
						ent->mLastLongThinkColor[0] = 1.0f;
						ent->mLastLongThinkColor[1] = 2.0f - (( ms - g_timeentities.GetFloat()) / g_timeentities.GetFloat() );
						ent->mLastLongThinkColor[2] = 0.0f;
						ent->mLastLongThinkColor[3] = 1.0f;
					}
					ent->DrawDebugEntityInfo( 0, &viewTextBounds, &ent->mLastLongThinkColor );
					ent->mLastLongThinkTime = time + 2000;
				}
				else if ( ent->mLastLongThinkTime > time )
				{
					ent->DrawDebugEntityInfo( 0, &viewTextBounds, &ent->mLastLongThinkColor );
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
					// ddynerman: save the current thinking entity for instance-dependent
					currentThinkingEntity = ent;
					ent->Think();
					currentThinkingEntity = NULL;
					num++;
				}
			} else {
				num = 0;
				for( ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() ) {
					// ddynerman: save the current thinking entity for instance-dependent
					currentThinkingEntity = ent;
					ent->Think();
					currentThinkingEntity = NULL;
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

		if ( isLastPredictFrame ) {
			// bdube: client entities
			rvClientEntity* cent;
			for( cent = clientSpawnedEntities.Next(); cent != NULL; cent = cent->spawnNode.Next() ) {
				cent->Think();			
			}	
		}

		// service any pending events
		idEvent::ServiceEvents();

		// nrausch: player could have been deleted in an event
		player = GetLocalPlayer();

		timer_events.Stop();

		if ( isLastPredictFrame ) {
			// jscott: effect system uses gravity and the player PVS
			bse->EndFrame();
		}

		// do multiplayer related stuff
		if ( isMultiplayer ) {
			mpGame.Run();
		}

		// free the player pvs
		FreePlayerPVS();

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
			ret.heartRate = 0.0f;
			ret.stamina = 0.0f;
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
			sessionCommand = "";
			break;
		}

		// make sure we don't loop forever when skipping a cinematic
		if ( skipCinematic && ( time > cinematicMaxSkipTime ) ) {
			Warning( "Exceeded maximum cinematic skip length.  Cinematic may be looping infinitely." );
			skipCinematic = false;
			break;
		}

		// jscott: additional timings
		timer_misc2.Stop();

		if ( g_frametime.GetInteger() > 1 )  {
			gameLocal.Printf( "misc:%.1f misc2:%.1f\n", timer_misc.Milliseconds(), timer_misc2.Milliseconds() );
		}
		// bdube: let gameDebug know that its not in a game frame anymore
		gameDebug.EndFrame( );
		gameLogLocal.EndFrame( );

	} while( ( inCinematic || ( time < cinematicStopTime ) ) && skipCinematic );

	ret.syncNextGameFrame = skipCinematic;
	if ( skipCinematic ) {
		soundSystem->EndCinematic();
		soundSystem->SetMute( false );
		skipCinematic = false;		
	}

	// show any debug info for this frame
	RunDebugInfo();
	D_DrawDebugLines();

	g_simpleItems.ClearModified();
	return ret;
}
// RAVEN END


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

// RAVEN BEGIN
// jnewquist: Option to adjust vertical fov instead of horizontal for non 4:3 modes
	if ( g_fixedHorizFOV.GetBool() ) {
		int aspectChoice = cvarSystem->GetCVarInteger( "r_aspectRatio" );
		switch( aspectChoice ) {
		default :
		case 0 :
			// 4:3
			ratio_x = 4.0f;
			ratio_y = 3.0f;
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
		x = ratio_x / idMath::Tan( base_fov / 360.0f * idMath::PI );
		y = idMath::ATan( ratio_y, x );
		fov_y = y * 360.0f / idMath::PI;
		fov_x = base_fov;
		return;
	}
// RAVEN END

	// first, calculate the vertical fov based on a 640x480 view
	x = 640.0f / idMath::Tan( base_fov / 360.0f * idMath::PI );
	y = idMath::ATan( 480.0f, x );
	fov_y = y * 360.0f / idMath::PI;

	// FIXME: somehow, this is happening occasionally
	assert( fov_y > 0 );
	if ( fov_y <= 0 ) {
		Printf( sys->FPU_GetState() );
		Error( "idGameLocal::CalcFov: bad result" );
	}

	int aspectChoice = cvarSystem->GetCVarInteger( "r_aspectRatio" );
	switch( aspectChoice ) {
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

	y = ratio_y / idMath::Tan( fov_y / 360.0f * idMath::PI );
	fov_x = idMath::ATan( ratio_x, y ) * 360.0f / idMath::PI;

	if ( fov_x < base_fov ) {
		fov_x = base_fov;
		x = ratio_x / idMath::Tan( fov_x / 360.0f * idMath::PI );
		fov_y = idMath::ATan( ratio_y, x ) * 360.0f / idMath::PI;
	}

	// FIXME: somehow, this is happening occasionally
	assert( ( fov_x > 0 ) && ( fov_y > 0 ) );
	if ( ( fov_y <= 0 ) || ( fov_x <= 0 ) ) {
		Printf( sys->FPU_GetState() );
		Error( "idGameLocal::CalcFov: bad result" );
	}
}

void ClearClipProfile( void );
void DisplayClipProfile( void );

/*
================
idGameLocal::Draw

makes rendering and sound system calls
================
*/
bool idGameLocal::Draw( int clientNum ) {
//	DisplayClipProfile( );
//	ClearClipProfile( );

	if ( isMultiplayer ) {
		return mpGame.Draw( clientNum );
	}

	idPlayer *player = static_cast<idPlayer *>(entities[ clientNum ]);

	if ( !player ) {
		return false;
	}

// RAVEN BEGIN
// mwhitlock: Xenon texture streaming.
#if defined(_XENON)
	renderView_t *view = player->GetRenderView();
	// nrausch: view doesn't necessarily exist yet
	if ( !view ) {
		player->CalculateRenderView();
		view = player->GetRenderView();
	}
#endif
// RAVEN END
	// render the scene
	player->playerView.RenderPlayerView( player->hud );

// RAVEN BEGIN
// bdube: debugging HUD
	gameDebug.DrawHud( );
// RAVEN END

	return true;
}

/*
================
idGameLocal::HandleESC
================
*/
escReply_t idGameLocal::HandleESC( idUserInterface **gui ) {

//RAVEN BEGIN
//asalmon: xbox dedicated server needs to bring up a special menu
#ifdef _XBOX
	if( cvarSystem->GetCVarBool( "net_serverDedicated" ))
	{
		return ESC_MAIN;
	}
#endif
//RAVEN END

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
idGameLocal::UpdatePlayerPostMainMenu
================
*/
void idGameLocal::UpdatePlayerPostMainMenu()	{
	idPlayer* player = GetLocalPlayer();

	//dedicated server?
	if( !player)	{
		return;
	}	
	
	//crosshair may have changed
	player->UpdateHudWeapon();
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

	if ( !player ) {
		return;
	}

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
idGameLocal::HandleMainMenuCommands
================
*/
void idGameLocal::HandleMainMenuCommands( const char *menuCommand, idUserInterface *gui ) {
	if ( !idStr::Icmp( menuCommand, "initCreateServerSettings" ) ) {
		int	guiValue = 0;

		switch ( mpGame.GameTypeToVote( si_gameType.GetString() ) ) {
			case idMultiplayerGame::VOTE_GAMETYPE_DM:
				guiValue = 0;
				break;
			case idMultiplayerGame::VOTE_GAMETYPE_TOURNEY:
				guiValue = 1;
				break;
			case idMultiplayerGame::VOTE_GAMETYPE_TDM:
				guiValue = 2;
				break;
			case idMultiplayerGame::VOTE_GAMETYPE_CTF:
				guiValue = 3;
				break;
			case idMultiplayerGame::VOTE_GAMETYPE_ARENA_CTF:
				guiValue = 4;
				break;
			case idMultiplayerGame::VOTE_GAMETYPE_DEADZONE:
				guiValue = 5;
				break;
		}

		gui->SetStateInt( "currentGametype", guiValue );
	} else if ( !idStr::Icmp( menuCommand, "filterByNextMod" ) ) {
		BuildModList();

		if ( modList.Num() > 0 && (filterMod < 0 || filterMod >= modList.Num()) ) {
			filterMod = 0;
			networkSystem->UseSortFunction( filterByMod, true );
			gui->SetStateString( "filterMod", modList[ filterMod ].c_str() );
		} else {
			++filterMod;
			if ( filterMod < modList.Num() ) {
				networkSystem->UseSortFunction( filterByMod, true );
				gui->SetStateString( "filterMod", modList[ filterMod ].c_str() );
			} else {
				filterMod = -1;
				networkSystem->UseSortFunction( filterByMod, false );
				gui->SetStateString( "filterMod", common->GetLocalizedString( "#str_123008" ) );
			}
		}
	} else if ( !idStr::Icmp( menuCommand, "filterByPrevMod" ) ) {
		BuildModList();

		if ( modList.Num() > 0 && (filterMod < 0 || filterMod >= modList.Num()) ) {
			filterMod = modList.Num() - 1;
			networkSystem->UseSortFunction( filterByMod, true );
			gui->SetStateString( "filterMod", modList[ filterMod ].c_str() );
		} else {
			--filterMod;
			if ( filterMod >= 0 ) {
				networkSystem->UseSortFunction( filterByMod, true );
				gui->SetStateString( "filterMod", modList[ filterMod ].c_str() );
			} else {
				filterMod = -1;
				networkSystem->UseSortFunction( filterByMod, false );
				gui->SetStateString( "filterMod", common->GetLocalizedString( "#str_123008" ) );
			}
		}
	} else if ( !idStr::Icmp( menuCommand, "updateFilterByMod" ) ) {
		if ( filterMod < 0 || filterMod >= modList.Num() ) {
			gui->SetStateString( "filterMod", common->GetLocalizedString( "#str_123008" ) );
		} else {
			gui->SetStateString( "filterMod", modList[ filterMod ].c_str() );
		}
	} else if ( !idStr::Icmp( menuCommand, "server_clearSort" ) ) {
		filterMod = -1;
		gui->SetStateString( "filterMod", common->GetLocalizedString( "#str_123008" ) );
	}

	return;
}

/*
================
idGameLocal::GetLevelMap

  should only be used for in-game level editing
================
*/
idMapFile *idGameLocal::GetLevelMap( void ) {
// RAVEN BEGIN
// rhummer: Added the HasBeenResolved check, if resolve has been run then it doesn't have func_groups.
//			Which we probably don't want, so force the map to be reloaded.
	if ( mapFile && mapFile->HasPrimitiveData() && !mapFile->HasBeenResloved() ) {
// RAVEN END
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
void idGameLocal::CallFrameCommand( idEntity* ent, const char* frameCommand ) {
	const function_t *func = program.FindFunction( frameCommand );
	if ( !func ) {
		Warning( "Script function '%s' not found.", frameCommand );
		return;
	}
	CallFrameCommand ( ent, func );
}

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
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( !ent->IsType( idTestModel::GetClassType() ) ) {
// RAVEN END
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

	if ( g_showEntityInfo.GetBool() ) {
		idBounds	viewTextBounds( origin );
		idBounds	viewBounds( origin );

		viewTextBounds.ExpandSelf( 128.0f );
		viewBounds.ExpandSelf( 512.0f );
		for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
			// don't draw the worldspawn
			if ( ent == world ) {
				continue;
			}

// RAVEN BEGIN
// rjohnson: moved entity info out of idGameLocal into its own function
			ent->DrawDebugEntityInfo(&viewBounds, &viewTextBounds);
// RAVEN END
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

// RAVEN BEGIN
// bdube: client entities
	if ( cl_showEntityInfo.GetBool ( ) ) {
		rvClientEntity* cent;
		for( cent = clientSpawnedEntities.Next(); cent != NULL; cent = cent->spawnNode.Next() ) {
			cent->DrawDebugInfo ( );
		}				
	}
// RAVEN END

	if ( g_showTargets.GetBool() ) {
		ShowTargets();
	}

	if ( g_showTriggers.GetBool() ) {
		idTrigger::DrawDebugInfo();
	}

	if ( ai_showCombatNodes.GetBool() ) {
//		idCombatNode::DrawDebugInfo();
	}

	if ( ai_showPaths.GetBool() ) {
		idPathCorner::DrawDebugInfo();
	}

	if ( g_editEntityMode.GetBool() ) {
		if ( gameLocal.isMultiplayer ) 	{
			g_editEntityMode.SetInteger(0);
			Printf( "Not allowed in multiplayer.\n" );
		} else {
			editEntities->DisplayEntities();
		}
	}

	if ( g_showCollisionWorld.GetBool() ) {
		// use g_maxShowDistance value instead of 128.0f
		collisionModelManager->DrawModel( clip[0]->GetWorldCollisionModel(), vec3_origin, mat3_identity, origin, mat3_identity, g_maxShowDistance.GetFloat() );
	}

	if ( g_showCollisionModels.GetBool() ) {
		if( g_showCollisionModels.GetInteger() == 2 ) {
			clip[ 0 ]->DrawClipModels( player->GetEyePosition(), g_maxShowDistance.GetFloat(), pm_thirdPerson.GetBool() ? NULL : player, &idPlayer::GetClassType() );
		} else {
			clip[ 0 ]->DrawClipModels( player->GetEyePosition(), g_maxShowDistance.GetFloat(), pm_thirdPerson.GetBool() ? NULL : player );
		}
		
	}

	if ( g_showCollisionTraces.GetBool() ) {
		clip[ 0 ]->PrintStatistics();
	}

	if ( g_showPVS.GetInteger() ) {
		pvs.DrawPVS( origin, ( g_showPVS.GetInteger() == 2 ) ? PVS_ALL_PORTALS_OPEN : PVS_NORMAL );
	}

// RAVEN BEGIN
// rjohnson: added debug hud
	if ( gameDebug.IsHudActive ( DBGHUD_PHYSICS ) )	{
		clip[ 0 ]->DebugHudStatistics();
	}

	clip[ 0 ]->ClearStatistics();
// RAVEN END

	if ( aas_test.GetInteger() >= 0 ) {
		idAAS *aas = GetAAS( aas_test.GetInteger() );
		if ( aas ) {
			aas->Test( origin );
			if ( ai_testPredictPath.GetBool() ) {
				idVec3 velocity;
				predictedPath_t path;

				velocity.x = idMath::Cos( DEG2RAD( player->viewAngles.yaw ) ) * 100.0f;
				velocity.y = idMath::Sin( DEG2RAD( player->viewAngles.yaw ) ) * 100.0f;
				velocity.z = 0.0f;
				idAI::PredictPath( player, aas, origin, velocity, 1000, 100, SE_ENTER_OBSTACLE | SE_BLOCKED | SE_ENTER_LEDGE_AREA, path );
			}
		}
// RAVEN BEGIN 
// rjohnson: added more debug drawing
		if ( aas_showAreas.GetInteger() == 3 ) {
			for( int i=0; i<aasNames.Num(); i++ ) {
				aas = GetAAS( i );
				if ( !aas || !aas->IsValid() ) {
					continue;
				}

				if ( aas->GetSettings()->debugDraw ) {
					aas->ShowAreas( origin );
				}
			}
		}
		if ( aas_showProblemAreas.GetInteger() == 3 ) {
			for( int i=0; i<aasNames.Num(); i++ ) {
				aas = GetAAS( i );
				if ( !aas || !aas->IsValid() ) {
					continue;
				}

				if ( aas->GetSettings()->debugDraw ) {
					aas->ShowAreas( origin, true );
				}
			}
		}
// RAVEN END
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
	collisionModelManager->DebugOutput( player->GetEyePosition(), mat3_identity );

// RAVEN BEGIN
// jscott: for debugging playbacks
	if( g_showPlayback.GetInteger() ) {
		gameEdit->DrawPlaybackDebugInfo();
	}
// RAVEN END

// RAVEN BEGIN
// ddynerman: SD's clip sector code
	if ( g_showClipSectors.GetBool() ) {
		clip[ 0 ]->DrawClipSectors();
	}

	if ( g_showAreaClipSectors.GetFloat() ) {
		clip[ 0 ]->DrawAreaClipSectors( g_showAreaClipSectors.GetFloat() );
	}
// RAVEN END
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

// RAVEN BEGIN
// mwhitlock: added entity memory usage stuff.
/*
==================
idGameLocal::GetEntityMemoryUsage

Compute combined total memory footprint of server and client entity storage.
==================
*/

size_t idGameLocal::GetEntityMemoryUsage ( void ) const {

	// Server ents.
	size_t serverEntitiesSize = 0;
	for( idEntity *ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		 serverEntitiesSize += sizeof( idEntity );
	}
	
	// Client ents.
	size_t clientEntitiesSize = 0;
	for( rvClientEntity *ent = gameLocal.clientSpawnedEntities.Next() ; ent != NULL; ent = ent->spawnNode.Next() ) {
		 clientEntitiesSize += ent->Size();
	}
	
	return serverEntitiesSize + clientEntitiesSize;
}
// RAVEN END

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

	firstFreeIndex = Max( minSpawnIndex, firstFreeIndex );

	if ( !spawnArgs.GetInt( "spawn_entnum", "0", spawn_entnum ) ) {
		while ( entities[firstFreeIndex] && firstFreeIndex < ENTITYNUM_MAX_NORMAL ) {
			firstFreeIndex++;
		}
		if ( firstFreeIndex >= ENTITYNUM_MAX_NORMAL ) {
			Error( "no free entities" );
		}
		spawn_entnum = firstFreeIndex++;
	} else {
		assert( spawn_entnum < MAX_CLIENTS || spawn_entnum >= minSpawnIndex );
	}

	entities[ spawn_entnum ] = ent;
	spawnIds[ spawn_entnum ] = spawnCount++;
	ent->entityNumber = spawn_entnum;
	ent->spawnNode.AddToEnd( spawnedEntities );
	ent->spawnArgs.TransferKeyValues( spawnArgs );

	if ( spawn_entnum >= num_entities ) {
		num_entities++;
	}

// RAVEN BEGIN
// bdube: keep track of last time an entity was registered	
	entityRegisterTime = time;
// RAVEN END
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

// RAVEN BEGIN
// bdube: keep track of last time an entity was registered	
	entityRegisterTime = time;
// RAVEN END
}

/*
===============
idGameLocal::SkipEntityIndex
===============
*/
void idGameLocal::SkipEntityIndex( void ) {
	assert( entities[ firstFreeIndex ] == NULL );
	firstFreeIndex++;
	if ( firstFreeIndex >= ENTITYNUM_MAX_NORMAL ) {
		Error( "no free entities" );
	}
}

/*
================
idGameLocal::SpawnEntityType
================
*/
idEntity *idGameLocal::SpawnEntityType( const idTypeInfo &classdef, const idDict *args, bool bIsClientReadSnapshot ) {
	idClass *obj;

#ifdef _DEBUG
	if ( isClient ) {
		assert( bIsClientReadSnapshot );
	}
#endif

// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
	if ( !classdef.IsType( idEntity::GetClassType() ) ) {
// RAVEN END
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
idGameLocal::SpawnClientEntityDef

Finds the spawn function for the client entity and calls it,
returning false if not found
===================
*/
bool idGameLocal::SpawnClientEntityDef( const idDict &args, rvClientEntity **cent, bool setDefaults, const char* spawn ) {
	const char	*classname;
	idTypeInfo	*cls;
	idClass		*obj;
	idStr		error;
	const char  *name;

	if ( cent ) {
		*cent = NULL;
	}

	spawnArgs = args;

	if ( spawnArgs.GetBool( "nospawn" ) ){
		//not meant to actually spawn, just there for some compiling process
		return false;
	}

	if ( spawnArgs.GetString( "name", "", &name ) ) {
		error = va( " on '%s'", name );
	}

	spawnArgs.GetString( "classname", NULL, &classname );

	const idDeclEntityDef *def = FindEntityDef( classname, false );
	if ( !def ) {
		// RAVEN BEGIN
		// jscott: a NULL classname would crash Warning()
		if( classname ) {
			Warning( "Unknown classname '%s'%s.", classname, error.c_str() );
		}
		// RAVEN END
		return false;
	}

	spawnArgs.SetDefaults( &def->dict );

	// check if we should spawn a class object
	if( spawn == NULL ) {
		spawnArgs.GetString( "spawnclass", NULL, &spawn );
	}

	if ( spawn ) {

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

		obj->CallSpawn();

		if ( cent && obj->IsType( rvClientEntity::GetClassType() ) ) {
			*cent = static_cast<rvClientEntity*>(obj);
		}

		return true;
	}

	Warning( "%s doesn't include a spawnfunc%s.", classname, error.c_str() );
	return false;
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

	TIME_THIS_SCOPE( __FUNCLINE__);
	
	if ( ent ) {
		*ent = NULL;
	}

	spawnArgs = args;

	if ( spawnArgs.GetBool( "nospawn" ) )
	{//not meant to actually spawn, just there for some compiling process
		return false;
	}

	if ( spawnArgs.GetString( "name", "", &name ) ) {
// RAVEN BEGIN
// jscott: fixed sprintf to idStr
		error = va( " on '%s'", name );
// RAVEN END
	}
	spawnArgs.GetString( "classname", NULL, &classname );

	const idDeclEntityDef *def = FindEntityDef( classname, false );
	if ( !def ) {
// RAVEN BEGIN
// jscott: a NULL classname would crash Warning()
		if( classname ) {
			Warning( "Unknown classname '%s'%s.", classname, error.c_str() );
		}
// RAVEN END
		return false;
	}

	spawnArgs.SetDefaults( &def->dict );

// RAVEN BEGIN
// rjohnson: entity usage stats
	if ( g_keepEntityStats.GetBool() ) {
		if ( idStr::Icmp( classname, "func_spawner" ) == 0 || 
			 idStr::Icmp( classname, "func_spawner_enemy" ) == 0 ) {
			// special case for spawners
			for( int i = 1; ; i++ ) {
				char		tempSpawn[128];
				const char	*tempClassname;

				sprintf( tempSpawn, "def_spawn_type%d", i );
				tempClassname = spawnArgs.GetString( tempSpawn, NULL );
				if ( tempClassname ) {
					const idDeclEntityDef *tempDef = FindEntityDef( tempClassname, false );

					if ( tempDef ) {
						idDict	tempArgs = tempDef->dict;

						tempArgs.Set( "mapFileName", mapFileNameStripped );
						entityUsageList.Insert( tempArgs );
					}
				} else {
					break;
				}
			}
		}
		else if ( def->dict.GetBool( "report_stats" ) ) {
			idDict	tempArgs = spawnArgs;
			tempArgs.Set( "mapFileName", mapFileNameStripped );
			entityUsageList.Insert( tempArgs );
		}
	}
// RAVEN END

	// check if we should spawn a class object
	spawnArgs.GetString( "spawnclass", NULL, &spawn );
	if ( spawn ) {

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

		obj->CallSpawn();

// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( ent && obj->IsType( idEntity::GetClassType() ) ) {
// RAVEN END
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

// abahr:
idEntity* idGameLocal::SpawnEntityDef( const char* entityDefName, const idDict* additionalArgs ) {
	idDict			finalArgs;
	const idDict*	entityDict = NULL;
	idEntity*		entity = NULL;
	
	TIME_THIS_SCOPE( __FUNCLINE__);

	if( !entityDefName ) {
		return NULL;
	}

	entityDict = FindEntityDefDict( entityDefName, false );
	if( !entityDict ) {
		return NULL;
	}

	if( !additionalArgs ) {
		SpawnEntityDef( *entityDict, &entity );
	} else {
		finalArgs.Copy( *entityDict );
		finalArgs.Copy( *additionalArgs );
		SpawnEntityDef( finalArgs, &entity );
	}

	return entity;
}
// RAVEN END

/*
================
idGameLocal::FindEntityDef
================
*/
const idDeclEntityDef *idGameLocal::FindEntityDef( const char *name, bool makeDefault ) const {
	TIME_THIS_SCOPE( __FUNCLINE__);
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
	TIME_THIS_SCOPE( __FUNCLINE__);
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

// RAVEN BEGIN
// bdube: suppress ents that don't match the entity filter
	const char* entityFilter;
	if ( serverInfo.GetString( "si_entityFilter", "", &entityFilter ) && *entityFilter ) {
		if ( spawnArgs.MatchPrefix ( "filter_" ) && !spawnArgs.GetBool ( va("filter_%s", entityFilter) ) ) {
			return true;
		}
	}
// RAVEN END

// RITUAL BEGIN
// squirrel: suppress ents that aren't supported in Buying modes (if that's the mode we're in)
	if ( mpGame.IsBuyingAllowedInTheCurrentGameMode() ) {
		if ( spawnArgs.GetBool( "disableSpawnInBuying", "0" ) ) {
			return true;
		}

		/// Don't spawn weapons or armor vests or ammo in Buying modes
		idStr classname = spawnArgs.GetString( "classname" );
		if( idStr::FindText( classname, "weapon_" ) == 0 || 
			idStr::FindText( classname, "item_armor_small" ) == 0 ||
			idStr::FindText( classname, "ammo_" ) == 0 ||
			idStr::FindText( classname, "item_armor_large" ) == 0 )
		{
			return true;
		}
	}
// RITUAL END

	// suppress deadzone triggers if we're not running DZ
	if ( idStr::Icmp( serverInfo.GetString( "si_gameType" ), "DeadZone" ) != 0 ) {
		if ( idStr::Icmp( spawnArgs.GetString( "classname" ), "trigger_controlzone" ) == 0 ) {
			return true;
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
// RAVEN BEGIN
// ddynerman: multiple game instances
void idGameLocal::SpawnMapEntities( int instance, unsigned short* entityNumIn, unsigned short* entityNumOut, int* startSpawnCount ) {
// RAVEN END
	int			i;
	int			num;
	int			inhibit;
	int			latchedSpawnCount = -1;
	idMapEntity	*mapEnt;
	int			numEntities;
	idDict		args;
	idDict		items;

	bool		proto69 = ( GetCurrentDemoProtocol() == 69 );

	Printf( "Spawning entities\n" );
	
	TIME_THIS_SCOPE( __FUNCLINE__);

	if ( mapFile == NULL ) {
		Printf("No mapfile present\n");
		return;
	}

	SetSkill( g_skill.GetInteger() );

	numEntities = mapFile->GetNumEntities();
	if ( numEntities == 0 ) {
		Error( "...no entities" );
	}

// RAVEN BEGIN
// ddynerman:	on the server, perform world-initialization when spawning instance 0 only and while starting up
//				on the client, perform world-init when spawning while starting up, as client main instance may not be ID 0
//				always spawn world-init in single-player
	num = inhibit = 0;

	// the worldspawn is a special that performs any global setup
	// needed by a level
	if ( ( gameLocal.isServer && instance == 0 && gamestate == GAMESTATE_STARTUP ) || ( gameLocal.isClient && gamestate == GAMESTATE_STARTUP ) || !gameLocal.isMultiplayer ) { 
		mapEnt = mapFile->GetEntity( 0 );
		args = mapEnt->epairs;
		args.SetInt( "spawn_entnum", ENTITYNUM_WORLD );
		
		// on the client, spawnCount won't always start at INITIAL_SPAWN_COUNT (see rvInstance::Populate())
		// make sure the world and physics ent get the right spawn id
		if( gameLocal.isClient && spawnCount != INITIAL_SPAWN_COUNT ) {
			latchedSpawnCount = spawnCount;
			spawnCount = INITIAL_SPAWN_COUNT;
		}

// abahr: precache stuff on worldSpawn like player def and other misc things
		CacheDictionaryMedia( &args );
// jnewquist: Use accessor for static class type
		if ( !SpawnEntityDef( args ) || !entities[ ENTITYNUM_WORLD ] || !entities[ ENTITYNUM_WORLD ]->IsType( idWorldspawn::GetClassType() ) ) {
			Error( "Problem spawning world entity" );
		}
		num++;

// bdube: dummy entity for client entities with physics
		args.Clear();
		args.SetInt( "spawn_entnum", ENTITYNUM_CLIENT );
// jnewquist: Use accessor for static class type 
		if ( !SpawnEntityType( rvClientPhysics::GetClassType(), &args, true ) || !entities[ ENTITYNUM_CLIENT ] ) {	
			Error( "Problem spawning client physics entity" );
		}

		if( gameLocal.isClient && latchedSpawnCount != -1 ) {
			spawnCount = latchedSpawnCount;
		}

		isMapEntity[ ENTITYNUM_CLIENT ] = true;
		isMapEntity[ ENTITYNUM_WORLD ] = true;
// RAVEN END
	}

	// capture spawn count of start of map entities (after we've spawned in the physics ents)
	if ( startSpawnCount ) {
		(*startSpawnCount) = spawnCount;
	}

	for ( i = 1 ; i < numEntities ; i++ ) {

		mapEnt = mapFile->GetEntity( i );
		args = mapEnt->epairs;

// RAVEN BEGIN
// ddynerman: merge the dicts ahead of SpawnEntityDef() so we can inhibit using merged info
		const idDeclEntityDef* entityDef = FindEntityDef( args.GetString( "classname" ), false );
		
		if( entityDef == NULL ) {
			gameLocal.Error( "idGameLocal::SpawnMapEntities() - Unknown entity classname '%s'\n", args.GetString( "classname" ) );
			return;
		}
		args.SetDefaults( &(entityDef->dict) );
// RAVEN END

		if ( !InhibitEntitySpawn( args ) ) {

			if( args.GetBool( "inv_item" ) ) {
				if( !items.GetBool( args.GetString( "inv_icon" ) ) ) {
					networkSystem->AddLoadingIcon( args.GetString( "inv_icon" ) );
					networkSystem->SetLoadingText( common->GetLocalizedString( args.GetString( "inv_name" ) ) );
					items.SetBool( args.GetString( "inv_icon" ), true );
				}
			}

			// precache any media specified in the map entity
			CacheDictionaryMedia( &args );
// RAVEN BEGIN
			if ( instance != 0 ) {
// ddynerman: allow this function to be called multiple-times to respawn map entities in other instances
				args.SetInt( "instance", instance );
				args.Set( "name", va( "%s_instance%d", args.GetString( "name" ), instance ) );
				//gameLocal.Printf( "Instance %d: Spawning %s (class: %s)\n", instance, args.GetString( "name" ), args.GetString( "classname" ) ); 
				
				// need a better way to do this - map entities that target other map entities need
				// to target the ones in the correct instance.  
				idStr target;
				if( args.GetString( "target", "", target ) ) {
					args.Set( "target", va( "%s_instance%d", target.c_str(), instance ) );
				}

				// and also udpate binds on a per-instance bases
				idStr bind;
				if( args.GetString( "bind", "", bind ) ) {
					args.Set( "bind", va( "%s_instance%d", bind.c_str(), instance ) );
				}

				// and also door triggers
				idStr triggerOnOpen;
				if( args.GetString( "triggerOnOpen", "", triggerOnOpen ) ) {
					args.Set( "triggerOnOpen", va( "%s_instance%d", triggerOnOpen.c_str(), instance ) );
				}

				idStr triggerOpened;
				if( args.GetString( "triggerOpened", "", triggerOpened ) ) {
					args.Set( "triggerOpened", va( "%s_instance%d", triggerOpened.c_str(), instance ) );
				}
			}

			// backward compatible 1.2 playback: proto69 reads the entity mapping list from the server
			// only 1.2 backward replays should get passed a != NULL entityNumIn
			assert( entityNumIn == NULL || proto69 );
			if ( proto69 && entityNumIn ) {
				assert( gameLocal.isClient );
				if ( entityNumIn[ i ] < 0 || entityNumIn[ i ] >= MAX_GENTITIES ) {
					// this entity was not spawned in on the server, ignore it here
					// this is one of the symptoms of net corruption <= 1.2 - fix here acted as a band aid
					// if you replay a 1.2 netdemo and hit this, it's likely other things will go wrong in the replay
					common->Warning( "entity inhibited on server not properly inhibited on client - map ent %d ( %s )", i, args.GetString( "name" ) );
					inhibit++;
					continue;
				}
				args.SetInt( "spawn_entnum", entityNumIn[ i ] );
			}

			idEntity* ent = NULL;
			SpawnEntityDef( args, &ent );
			//common->Printf( "pop: spawn map ent %d at %d ( %s )\n", i, ent->entityNumber, args.GetString( "name" ) );
	
			if ( ent && entityNumOut ) {
				entityNumOut[ i ] = ent->entityNumber;
			}

			if ( gameLocal.GetLocalPlayer() && ent && gameLocal.isServer && instance != gameLocal.GetLocalPlayer()->GetInstance() ) {
				// if we're spawning entities not in our instance, tell them not to draw
				ent->BecomeInactive( TH_UPDATEVISUALS );
			}

			if ( ent ) {
				isMapEntity[ ent->entityNumber ] = true;
			}
// RAVEN END
			num++;
		} else {
			if ( !proto69 ) {
				// keep counting and leave an empty slot in the entity list for inhibited entities
				// this so we maintain the same layout as the server and don't change it across restarts with different inhibit schemes
				//common->Printf( "pop: skip map ent %d at index %d ( %s )\n", i, firstFreeIndex, args.GetString( "name" ) );
				SkipEntityIndex();
			} else {
				// backward 1.2 netdemo replay compatibility - no skipping
				// they might net corrupt but there's no fix client side
				assert( isClient );
			}
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

/*
=============
idGameLocal::ArgCompletion_AIName

Argument completion for idAI entity names
=============
*/
void idGameLocal::ArgCompletion_AIName( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	int i;

	for( i = 0; i < gameLocal.num_entities; i++ ) {
		if ( gameLocal.entities[ i ] && gameLocal.entities[ i ]->IsType( idAI::GetClassType() ) ) {
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

// RAVEN BEGIN
// ddynerman: multiple clip worlds
	num = ClipModelsTouchingBounds( ent, phys->GetAbsBounds(), phys->GetClipMask(), clipModels, MAX_GENTITIES );
// RAVEN END
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
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( hit->IsType( idPlayer::GetClassType() ) && static_cast< idPlayer * >( hit )->IsInTeleport() ) {
// RAVEN END
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
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( activator->IsType( idPlayer::GetClassType() ) ) {
// RAVEN END
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
================
idGameLocal::IsWeaponsStayOn
================
*/
bool idGameLocal::IsWeaponsStayOn( void ) {
	/// Override weapons stay when buying is active
	if( isMultiplayer && mpGame.IsBuyingAllowedInTheCurrentGameMode() ) {
		return false;
	}

	return serverInfo.GetBool( "si_weaponStay" );
}


/*
============
idGameLocal::AlertAI
============
*/
void idGameLocal::AlertAI( idEntity *ent ) {
// RAVEN BEGIN
// bdube: merged
	if ( ent ) {
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( ent->IsType( idActor::GetClassType() ) ) {
// RAVEN END
			// alert them for the next frame
			lastAIAlertActorTime = time + GetMSec();
			lastAIAlertActor = static_cast<idActor *>( ent );
		} else {
			lastAIAlertEntityTime = time + GetMSec();
			lastAIAlertEntity = ent;
		}
	} else {
		lastAIAlertEntityTime = 0;
		lastAIAlertActorTime = 0;
		lastAIAlertEntity = NULL;
		lastAIAlertActor = NULL;
	}
// RAVEN END
}

// RAVEN BEGIN
// bdube: alert entity returns an entity, alert actor a an actor
/*
============
idGameLocal::GetAlertEntity
============
*/
idEntity *idGameLocal::GetAlertEntity( void ) {
	if ( lastAIAlertEntityTime >= time ) {
		return lastAIAlertEntity.GetEntity();
	}

	return NULL;
}

/*
============
idGameLocal::GetAlertActor
============
*/
idActor *idGameLocal::GetAlertActor( void ) {
	if ( lastAIAlertActorTime >= time ) {
		return lastAIAlertActor.GetEntity();
	}

	return NULL;
}
// RAVEN END

/*
============
SortClipModelsByEntity
============
*/
static int SortClipModelsByEntity( const void* left, const void* right ) {
	idEntity* leftEntity  = left  ? (*((const idClipModel**)left))->GetEntity() : NULL;
	idEntity* rightEntity = right ? (*((const idClipModel**)right))->GetEntity() : NULL;

	int entityNumLeft = (leftEntity) ? leftEntity->entityNumber : 0;
	int entityNumRight = (rightEntity) ? rightEntity->entityNumber : 0;

	return entityNumLeft - entityNumRight;
}

// RAVEN BEGIN
/*
============
idGameLocal::RadiusDamage
Returns the number of actors damaged
============
*/
// abahr: changed to work with deathPush
void idGameLocal::RadiusDamage( const idVec3 &origin, idEntity *inflictor, idEntity *attacker, idEntity *ignoreDamage, idEntity *ignorePush, const char *damageDefName, float dmgPower, int* hitCount ) {
	float		dist, damageScale, attackerDamageScale, attackerPushScale;
	idEntity*	ent = NULL;
	idEntity*	lastEnt = NULL;
	idClipModel* clipModel = NULL;
	idClipModel* clipModelList[ MAX_GENTITIES ];
	int			numListedClipModels;
	modelTrace_t	result;
	idVec3 		v, damagePoint, dir;
	int			i, damage, radius, push;

	const idDict *damageDef = FindEntityDefDict( damageDefName, false );
	if ( !damageDef ) {
		Warning( "Unknown damageDef '%s'", damageDefName );
		return;
	}

	damageDef->GetInt( "damage", "20", damage );
	damageDef->GetInt( "radius", "50", radius );
	damageDef->GetInt( "push", va( "%d", damage * 100 ), push );
	damageDef->GetFloat( "attackerDamageScale", "0.5", attackerDamageScale );
	if( gameLocal.isMultiplayer ) {
		damageDef->GetFloat( "attackerPushScale", "2", attackerPushScale );
	} else {
		damageDef->GetFloat( "attackerPushScale", "0", attackerPushScale );
	}

	if ( radius < 1 ) {
		radius = 1;
	}


// ddynerman: multiple clip worlds
	numListedClipModels = ClipModelsTouchingBounds( inflictor, idBounds(origin).Expand(radius), MASK_ALL, clipModelList, MAX_GENTITIES );
	if( numListedClipModels > 0 ) {
		//Sort list by unique entities for easier searching
		qsort( clipModelList, numListedClipModels, sizeof(clipModelList[0]), SortClipModelsByEntity );
	}

	if ( inflictor ) {
		inflictor = inflictor->GetDamageEntity ( );
	}
	if ( attacker ) {
		attacker = attacker->GetDamageEntity ( );
	}
	if ( ignoreDamage ) {
		ignoreDamage = ignoreDamage->GetDamageEntity ( );
	}

	for( int c = 0; c < numListedClipModels; ++c ) {
		clipModel = clipModelList[ c ];
		assert( clipModel );

		ent = clipModel->GetEntity();
		
		// Skip all entitys that arent primary damage entities
		if ( !ent || ent != ent->GetDamageEntity ( ) ) {
			continue;
		}

		// Dont damage inflictor or the ignore entity
		if( ent == inflictor || ent == ignoreDamage ) {
			continue;
		}

		idBounds absBounds = clipModel->GetAbsBounds();

		// find the distance from the edge of the bounding box
		for ( i = 0; i < 3; i++ ) {
			if ( origin[ i ] < absBounds[0][ i ] ) {
				v[ i ] = absBounds[0][ i ] - origin[ i ];
			} else if ( origin[ i ] > absBounds[1][ i ] ) {
				v[ i ] = origin[ i ] - absBounds[1][ i ];
			} else {
				v[ i ] = 0;
			}
		}

		dist = v.Length();
		if ( dist >= radius ) {
			continue;
		}

		if( gameRenderWorld->FastWorldTrace(result, origin, absBounds.GetCenter()) ) {
			continue;
		}
		
		RadiusPushClipModel ( inflictor, origin, push, clipModel );

		// Only damage unique entities.  This works because we have a sorted list
		if( lastEnt == ent ) {
			continue;
		}

		lastEnt = ent;

		if ( ent->CanDamage( origin, damagePoint, ignoreDamage ) ) {						
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			if( gameLocal.isMultiplayer ) {
				// fudge the direction in MP to account for player height difference and origin shift
				// 31.875 = origin is 23.875 units lower in Q4 than Q3 + player is 8 units taller in Q4
				dir = ( ent->GetPhysics()->GetOrigin() + idVec3( 0.0f, 0.0f, 31.875f ) ) - origin;
			} else {
				dir = ent->GetPhysics()->GetOrigin() - origin;
			}		
			
			dir[2] += 24;
 
 			// get the damage scale
			damageScale = dmgPower * ( 1.0f - dist / radius );
			
			if ( ent == attacker ) {
				damageScale *= attackerDamageScale;
			}

			dir.Normalize();
			ent->Damage( inflictor, attacker, dir, damageDefName, damageScale, CLIPMODEL_ID_TO_JOINT_HANDLE(ent->GetPhysics()->GetClipModel()->GetId()) );

			// for stats, count the first 
			if( attacker && attacker->IsType( idPlayer::GetClassType() ) && inflictor && inflictor->IsType( idProjectile::GetClassType() ) && ent->IsType( idPlayer::GetClassType() ) && hitCount ) {
				// with splash damage projectiles, one projectile fire can damage multiple people.  If anyone is hit, 
				// the shot counts as a hit but only report accuracy on the first one to avoid accuracies > 100%
				statManager->WeaponHit( (const idActor*)attacker, ent, ((idProjectile*)inflictor)->methodOfDeath, (*hitCount) == 0 );
				(*hitCount)++;
			}
		} 
	}
}
// RAVEN END

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

// RAVEN BEGIN
// abahr: need to use gravity instead of assuming z is up
	dir = -GetCurrentGravity( const_cast<idEntity*>(inflictor) ).ToNormal();
// RAVEN END

	bounds = idBounds( origin ).Expand( radius );

	// get all clip models touching the bounds
// RAVEN BEGIN
// ddynerman: multiple clip worlds
	numListedClipModels = ClipModelsTouchingBounds( inflictor, bounds, -1, clipModelList, MAX_GENTITIES );

// bdube: getdamageentity
	if ( inflictor ) {
		inflictor = ((idEntity*)inflictor)->GetDamageEntity ( );
	}
	if ( ignore ) {
		ignore = ((idEntity*)ignore)->GetDamageEntity ( );
	}
// RAVEN END

	// apply impact to all the clip models through their associated physics objects
	for ( i = 0; i < numListedClipModels; i++ ) {

		clipModel = clipModelList[i];

		// never push render models
		if ( clipModel->IsRenderModel() ) {
			continue;
		}

		ent = clipModel->GetEntity();
// RAVEN BEGIN
// bdube: damage entity		
		if ( !ent || ent != ent->GetDamageEntity ( ) ) {
			continue;
		}
// RAVEN END
		
		// never push projectiles
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( ent->IsType( idProjectile::GetClassType() ) ) {
// RAVEN END
			continue;
		}

		// players use "knockback" in idPlayer::Damage
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( ent->IsType( idPlayer::GetClassType() ) && !quake ) {
// RAVEN END
			continue;
		}

		// don't push the ignore entity
		if ( ent == ignore ) {
			continue;
		}

		if ( gameRenderWorld->FastWorldTrace( result, origin, clipModel->GetOrigin() ) ) {
			continue;
		}

		// scale the push for the inflictor
		if ( ent == inflictor ) {
			scale = inflictorScale;
		} else {
			scale = 1.0f;
		}

		if ( quake ) {
			clipModel->GetEntity()->ApplyImpulse( world, clipModel->GetId(), clipModel->GetOrigin(), scale * push * dir );
		} else {
// RAVEN BEGIN
// bdube: inflictor
			RadiusPushClipModel( (idEntity*)inflictor, origin, scale * push, clipModel );
// RAVEN END
		}
	}
}

/*
==============
idGameLocal::RadiusPushClipModel
==============
*/
// RAVEN BEGIN
// bdube: inflictor
void idGameLocal::RadiusPushClipModel( idEntity* inflictor, const idVec3 &origin, const float push, const idClipModel *clipModel ) {
// RAVEN END
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
// RAVEN BEGIN
// abahr: removed because z isn't always up
		//impulse.z += 1.0f;
		//impulse = idVec3( 0.0, 0.0, 1.0 );
		clipModel->GetEntity()->ApplyImpulse( inflictor, clipModel->GetId(), clipModel->GetOrigin(), push * impulse, true );
// RAVEN END
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

// RAVEN BEGIN
// bdube: inflictor
		clipModel->GetEntity()->ApplyImpulse( inflictor, clipModel->GetId(), center, impulse );
// RAVEN END
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

// RAVEN BEGIN
// rjohnson: no decals on dedicated server
	if ( isMultiplayer && !isClient && !isListenServer ) {
		// no decals on dedicated server
		return;
	}
// RAVEN END

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
	gameRenderWorld->ProjectDecalOntoWorld( winding, projectionOrigin, parallel, depth * 0.5f, declManager->FindMaterial( material ), time );
}

/*
==============
idGameLocal::BloodSplat
==============
*/
// RAVEN BEGIN
// ddynerman: multiple collision models
void idGameLocal::BloodSplat( const idEntity* ent, const idVec3 &origin, const idVec3 &dirArg, float size, const char *material ) {

	float halfSize = size * 0.5f;
	idVec3 verts[] = {	idVec3( 0.0f, +halfSize, +halfSize ),
						idVec3( 0.0f, +halfSize, -halfSize ),
						idVec3( 0.0f, -halfSize, -halfSize ),
						idVec3( 0.0f, -halfSize, +halfSize ) };
	idVec3 dir = dirArg;
	idTraceModel trm;
	idClipModel mdl;
	trace_t results;

// RAVEN BEGIN
// mekberg: changed from g_bloodEffects to g_decals
	if ( !g_decals.GetBool() ) {
		return;
	}
// RAVEN END

	size = halfSize + random.RandomFloat() * halfSize;
	trm.SetupPolygon( verts, 4 );
	mdl.LoadModel( trm, NULL );

	// I don't want dir to be axis aligned, as it is more likely to streak them (because most architecture is axis aligned
	dir.Set( dirArg.x*.1f, dirArg.y*.1f, -1 );
	dir.Normalize();

// RAVEN BEGIN
// ddynerman: multiple clip worlds
	Translation( ent, results, origin, origin + dir * 72.0f, &mdl, mat3_identity, CONTENTS_SOLID, NULL );
// RAVEN END
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
	if ( client->health <= 0 || client->pfl.dead ) {
		return;
	}

	camera = cam;
	if ( camera ) {
// RAVEN BEGIN
// bdube: tool support
		inCinematic = false;
		if( !( gameLocal.editors & ( EDITOR_MODVIEW | EDITOR_PLAYBACKS ) ) ) {
			inCinematic = true;
		}
// RAVEN END

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
				
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
				if ( ent->IsType( idAI::GetClassType() ) ) {
// RAVEN END
					ai = static_cast<idAI *>( ent );
					if ( !ai->GetEnemy() || !ai->IsActive() ) {
						// no enemy, or inactive, so probably safe to ignore
						continue;
					}
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
				} else if ( ent->IsType( idProjectile::GetClassType() ) ) {
// RAVEN END
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

// RAVEN BEGIN
// jscott: for portal skies
/*
=============
idGameLocal::GetPortalSky
=============
*/
idCamera *idGameLocal::GetPortalSky( void ) const
{
	if( !portalSkyVisible ) {

		return( NULL );
	}
	return( portalSky );
}
/*
=============
idGameLocal::SetPortalSky
=============
*/
void idGameLocal::SetPortalSky( idCamera *cam ) 
{
	portalSky = cam;
}
// RAVEN END

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

// RAVEN BEGIN
/*
======================
idGameLocal::StartViewEffect

For passing in an effect triggered view effect
======================
*/
void idGameLocal::StartViewEffect( int type, float time, float scale )
{
	idPlayer		*player;
	idPlayerView	*view;

	player = GetLocalPlayer();
	if( player )
	{
		view = &player->playerView;

		switch( type )
		{
		case VIEWEFFECT_DOUBLEVISION:
			view->SetDoubleVisionParms( time, scale );
			break;

		case VIEWEFFECT_SHAKE:
			if( !gameLocal.isMultiplayer ) {
				view->SetShakeParms( time, scale );
			}
			break;

		case VIEWEFFECT_TUNNEL:
			view->SetTunnelParms( time, scale );
			break;

		default:
			gameLocal.Warning( "Invalid view effect" );
			break;
		}
	}
}

/*
======================
idGameLocal::GetPlayerView
======================
*/
void idGameLocal::GetPlayerView( idVec3 &origin, idMat3 &axis )
{
	idPlayer		*player;
	renderView_t	*view;

	player = GetLocalPlayer();
	if( player )
	{
		view = player->GetRenderView();
		origin = view->vieworg;
		axis = view->viewaxis;
	}
	else
	{
		origin = vec3_origin;
		axis = mat3_identity;
	}
}

/*
======================
idGameLocal::Translation

small portion of physics required for the effects system
======================
*/
void idGameLocal::Translation( trace_t &trace, idVec3 &source, idVec3 &dest, idTraceModel *trm, int clipMask ) {	

	if( !trm ) {
		// HACK
		clip[0]->Translation( trace, source, dest, NULL, mat3_identity, clipMask, NULL, NULL );
	}
	else {
		idClipModel		cm;
	
		cm.LoadModel( *trm, NULL );
		// HACK
		clip[0]->Translation( trace, source, dest, &cm, mat3_identity, clipMask, NULL, NULL );
	}
}

/*
======================
idGameLocal::SpawnClientMoveable
======================
*/
void idGameLocal::SpawnClientMoveable( const char* name, int lifetime, const idVec3& origin, const idMat3& axis, const idVec3& velocity, const idVec3& angular_velocity ) {
	// find the debris def
	const idDict* args = gameLocal.FindEntityDefDict( name, false );
	if ( !args ) {
		return;
	}

	// Ensure client moveables never last forever
	if ( lifetime <= 0 ) {
		lifetime = SEC2MS(args->GetFloat( "duration", "5" ));
	}
	int burn_time = idMath::ClampInt( 0, lifetime, SEC2MS(args->GetFloat( "burn_time", "2.5" )) );
	
	// Spawn the debris
	
	rvClientMoveable* cent = NULL;
	// force the args to spawn a rvClientMoveable
	SpawnClientEntityDef( *args, (rvClientEntity**)(&cent), false, "rvClientMoveable" );
	
	if( !cent ) {
		return;
	}
 
	cent->SetOrigin( origin );
	cent->SetAxis( axis );
		
	cent->GetPhysics()->SetLinearVelocity( velocity );
	cent->GetPhysics()->SetAngularVelocity( angular_velocity );

	if ( !burn_time ) {
		//just disappear
		cent->PostEventMS( &EV_Remove, lifetime );	
	} else {
		cent->PostEventMS( &CL_FadeOut, lifetime-burn_time, burn_time );
	}
}

/*
======================
idGameLocal::DebugSet
======================
*/
void idGameLocal::DebugSetString( const char* name, const char* value ) {
	gameDebug.SetString( name, value );
}
void idGameLocal::DebugSetFloat( const char* name, float value ) {
	gameDebug.SetFloat( name, value );
}
void idGameLocal::DebugSetInt( const char* name, int value ) {
	gameDebug.SetInt( name, value );
}

/*
======================
idGameLocal::DebugGetStat
======================
*/
const char* idGameLocal::DebugGetStatString ( const char* name ) {
	return gameDebug.GetStatString( name );
}

int idGameLocal::DebugGetStatInt ( const char* name ) {
	return gameDebug.GetStatInt( name );
}

float idGameLocal::DebugGetStatFloat ( const char* name ) {
	return gameDebug.GetStatFloat( name );
}

/*
======================
idGameLocal::IsDebugHudActive
======================
*/
bool idGameLocal::IsDebugHudActive ( void ) const {
	return gameDebug.IsHudActive( DBGHUD_ANY );
}


// rjohnson: added player info support for note taking system
/*
======================
idGameLocal::GetPlayerInfo
======================
*/
bool idGameLocal::GetPlayerInfo( idVec3 &origin, idMat3 &axis, int PlayerNum, idAngles *deltaViewAngles, int reqClientNum ) {
	idPlayer	*player;

	if ( PlayerNum == -1 ) {
		player = GetLocalPlayer();
	} else {
		player = GetClientByNum( PlayerNum );
	}

	if( reqClientNum != -1 ) {
		idPlayer* reqClient = GetClientByNum( reqClientNum );
		if( reqClient && player ) {
			if( reqClient->GetInstance() != player->GetInstance() ) {
				return false;
			}
		}
	}

	if ( !player ) {
		return false;
	}

	player->GetViewPos( origin, axis );
	origin = player->GetPhysics()->GetOrigin();

	if ( deltaViewAngles ) {
		*deltaViewAngles = player->GetDeltaViewAngles();
	}

	return true;
};

/*
======================
idGameLocal::SetCurrentPlayerInfo
======================
*/
void idGameLocal::SetPlayerInfo( idVec3 &origin, idMat3 &axis, int PlayerNum ) {
	idPlayer	*player;

	if ( PlayerNum == -1 ) {
		player = GetLocalPlayer();
	} else {
		player = GetClientByNum( PlayerNum );
	}

	if ( !player ) {
		return;
	}

	player->Teleport( origin, axis.ToAngles(), NULL );
// RAVEN BEGIN
// ddynerman: save the current thinking entity for instance-dependent
	currentThinkingEntity = player;
	player->CalculateFirstPersonView();
	player->CalculateRenderView();
	currentThinkingEntity = NULL;
// RAVEN END

	return;
};

bool idGameLocal::PlayerChatDisabled( int clientNum ) {
	if( clientNum < 0 || clientNum >= MAX_CLIENTS || !entities[ clientNum ] ) {
		return false;
	}

	return !( ((idPlayer*)entities[ clientNum ])->isChatting || ((idPlayer*)entities[ clientNum ])->pfl.dead );
}

void idGameLocal::SetViewComments( const char *text ) {
	idPlayer	*player;

	player = GetLocalPlayer();

	if ( !player ) {
		return;
	}

	if ( text ) {
		player->hud->SetStateString( "viewcomments", text );
		player->hud->HandleNamedEvent( "showViewComments" );
	}
	else {
		player->hud->SetStateString( "viewcomments", "" );
		player->hud->HandleNamedEvent( "hideViewComments" );
	}
}

/*
===================
idGameLocal::GetNumGravityAreas
===================
*/
int	idGameLocal::GetNumGravityAreas() const {
	return gravityInfo.Num();
}

/*
===================
idGameLocal::GetGravityInfo
===================
*/
const rvGravityArea* idGameLocal::GetGravityInfo( int index ) const {
	return gravityInfo[ index ];
}

/*
===================
idGameLocal::SetGravityArea
===================
*/
void idGameLocal::SetGravityInfo( int index, rvGravityArea* info ) {
	gravityInfo[ index ] = info;
}

/*
===================
idGameLocal::AddUniqueGravityInfo
===================
*/
void idGameLocal::AddUniqueGravityInfo( rvGravityArea* info ) {
	gravityInfo.AddUnique( info );
}

/*
===================
idGameLocal::GetCurrentGravityInfoIndex
===================
*/
int idGameLocal::GetCurrentGravityInfoIndex( const idVec3& origin ) const {
	int numGravityAreas = GetNumGravityAreas();
	if( !numGravityAreas ) {
		return -1;
	}

	int areaNum = gameRenderWorld->PointInArea( origin );

	for( int ix = 0; ix < numGravityAreas; ++ix ) {
		if( !gameRenderWorld->AreasAreConnected(GetGravityInfo(ix)->GetArea(), areaNum, PS_BLOCK_GRAVITY) ) {
			continue;
		}
		 
		return ix;
	}

	return -1;
}

/*
===================
idGameLocal::InGravityArea
===================
*/
bool idGameLocal::InGravityArea( idEntity* entity ) const {
	return GetCurrentGravityInfoIndex( entity ) >= 0;
}

/*
===================
idGameLocal::GetCurrentGravityInfoIndex
===================
*/
int idGameLocal::GetCurrentGravityInfoIndex( idEntity* entity ) const {
	return GetCurrentGravityInfoIndex( entity->GetPhysics()->GetOrigin() );
}

/*
===================
idGameLocal::GetCurrentGravity
===================
*/
const idVec3 idGameLocal::GetCurrentGravity( idEntity* entity ) const {
	int index = GetCurrentGravityInfoIndex( entity );
	return (index >= 0) ? gravityInfo[ index ]->GetGravity(entity) : GetGravity();
}

/*
===================
idGameLocal::GetCurrentGravity
===================
*/
const idVec3 idGameLocal::GetCurrentGravity( const idVec3& origin, const idMat3& axis ) const {
	int index = GetCurrentGravityInfoIndex( origin );
	return (index >= 0) ? gravityInfo[ index ]->GetGravity(origin, axis, MASK_SOLID, NULL) : GetGravity();
}

/*
===================
idGameLocal::InGravityArea
===================
*/
bool idGameLocal::InGravityArea( rvClientEntity* entity ) const {
	return GetCurrentGravityInfoIndex( entity ) >= 0;
}

/*
===================
idGameLocal::GetCurrentGravityInfoIndex
===================
*/
int idGameLocal::GetCurrentGravityInfoIndex( rvClientEntity* entity ) const {
	return GetCurrentGravityInfoIndex( entity->GetPhysics()->GetOrigin() );
}

/*
===================
idGameLocal::GetCurrentGravity
===================
*/
const idVec3 idGameLocal::GetCurrentGravity( rvClientEntity* entity ) const {
	int index = GetCurrentGravityInfoIndex( entity );
	return (index >= 0) ? gravityInfo[ index ]->GetGravity(entity) : GetGravity();
}

/*
===================
idGameLocal::ReferenceScriptObjectProxy
===================
*/
idEntity* idGameLocal::ReferenceScriptObjectProxy( const char* scriptObjectName ) {
	idEntity*		proxy = NULL;
	idEntityPtr<idEntity> safeProxy;
	idDict			args;
	idScriptObject* object = NULL;

	for( int ix = 0; ix < scriptObjectProxies.Num(); ++ix ) {
		proxy = scriptObjectProxies[ ix ].GetEntity();
		assert( proxy );
		
		object = &proxy->scriptObject;
		if( !object->data ) {
			object->SetType( scriptObjectName );
			proxy->ConstructScriptObject();
			return proxy;
		}
	}

	args.Set( "classname", "func_static" );
	args.Set( "scriptobject", scriptObjectName );
	args.SetBool( "noclipmodel", true );
	bool spawned = SpawnEntityDef(args, &proxy);
	if ( !spawned ) {
		assert( 0 );
	}
	safeProxy = proxy;
	scriptObjectProxies.AddUnique( safeProxy );
	return proxy;
}

/*
===================
idGameLocal::ReleaseScriptObjectProxy
===================
*/
void idGameLocal::ReleaseScriptObjectProxy( const char* proxyName ) {
	idScriptObject* object = NULL;
	idEntity*		entity = NULL;

	for( int ix = 0; ix < scriptObjectProxies.Num(); ++ix ) {
		entity = scriptObjectProxies[ ix ].GetEntity();
		if( entity && !idStr::Icmp(entity->GetName(), proxyName) ) {
			object = &entity->scriptObject;
			if( !object ) {
				continue;
			}
			
			entity->DeconstructScriptObject();
			object->Free();
		}
	}
}

// RAVEN BEGIN
// rjohnson: entity usage stats
void idGameLocal::ListEntityStats( const idCmdArgs &args ) {
	int				i, j;
	idStr			currentMap;
	idList<idStr>	uniqueMapNames;


	for( i = 1; i < args.Argc(); i++ ) {
		if ( idStr::Icmp( args.Argv( i ), "clear" ) == 0 ) {
			entityUsageList.Clear();
			common->Printf("Entity stats cleared.\n");
			return;
		}
	}

	for( i = 0; i < entityUsageList.Num(); i++ ) {
		entityUsageList[ i ].SetInt( "reported_stat", false );
	}

	for( i = 0; i < entityUsageList.Num(); i++ ) {
		idStr	mapFileName, className;
		int		count;

		if ( entityUsageList[ i ].GetInt( "reported_stat" ) ) {
			continue;
		}

		entityUsageList[ i ].GetString( "mapFileName", "none", mapFileName );
		if ( currentMap != mapFileName )
		{
			if ( i ) {
				common->Printf( "\n" );
			}
			common->Printf( "================ %s ================\n", mapFileName.c_str() );
			currentMap = mapFileName;
			uniqueMapNames.Insert( mapFileName );
		}

		entityUsageList[ i ].GetString( "classname", "none", className );
		count = 0;

		for( j = i; j < entityUsageList.Num(); j++ ) {
			idStr	checkMapFileName, checkClassName;

			entityUsageList[ j ].GetString( "mapFileName", "none", checkMapFileName );
			if ( checkMapFileName != mapFileName ) {
				break;
			}

			entityUsageList[ j ].GetString( "classname", "none", checkClassName );

			if ( checkClassName == className ) {
				entityUsageList[ j ].SetInt( "reported_stat", 1 );
				count++;
			}
		}

		common->Printf("%d\t%s\n", count, className.c_str() );
	}

	common->Printf( "\n" );
	common->Printf( "\n" );
	common->Printf( "================ OVERALL ================\n" );

	for( i = 0; i < entityUsageList.Num(); i++ ) {
		idStr	mapFileName, className;
		int		count;

		if ( entityUsageList[ i ].GetInt( "reported_stat" ) == 2 ) {
			continue;
		}

		entityUsageList[ i ].GetString( "classname", "none", className );
		count = 0;

		for( j = i; j < entityUsageList.Num(); j++ ) {
			idStr	checkClassName;

			entityUsageList[ j ].GetString( "classname", "none", checkClassName );

			if ( checkClassName == className ) {
				entityUsageList[ j ].SetInt( "reported_stat", 2 );
				count++;
			}
		}

		common->Printf("%d\t%s\n", count, className.c_str() );
	}

	idFile *FH = fileSystem->OpenFileWrite( "EntityStats.csv" );
	if ( FH ) {
		int	size = sizeof( int ) * uniqueMapNames.Num();
		int	*count = ( int * )_alloca( size );

		FH->Printf("\"Definition\"");
		for( i = 0; i < uniqueMapNames.Num(); i++ ) {
			FH->Printf( ",\"%s\"", uniqueMapNames[ i ].c_str() );
		}
		FH->Printf(",Total\n");

		for( i = 0; i < entityUsageList.Num(); i++ ) {
			idStr	className;
			int		total;

			if ( entityUsageList[ i ].GetInt( "reported_stat" ) == 3 ) {
				continue;
			}

			entityUsageList[ i ].GetString( "classname", "none", className );

			memset( count, 0, size );
			for( j = i; j < entityUsageList.Num(); j++ )
			{
				idStr	checkMapFileName, checkClassName;

				entityUsageList[ j ].GetString( "classname", "none", checkClassName );

				if ( checkClassName == className ) {
					entityUsageList[ j ].SetInt( "reported_stat", 3 );
					entityUsageList[ j ].GetString( "mapFileName", "none", checkMapFileName );

					int loc = uniqueMapNames.FindIndex( checkMapFileName );
					if ( loc >= 0 ) {
						count[ loc ]++;
					}
				}
			}

			total = 0;
			FH->Printf( "\"%s\"", className.c_str() );
			for( j = 0; j < uniqueMapNames.Num(); j++ ) {
				FH->Printf( ",%d", count[ j ] );
				total += count[ j ];
			}
			FH->Printf( ",%d\n", total );
		}

		fileSystem->CloseFile( FH );
	}
}
// RAVEN END

/*
======================
idGameLocal::SpreadLocations

Now that everything has been spawned, associate areas with location entities
======================
*/
void idGameLocal::SpreadLocations() {
	idEntity *ent;

// RAVEN BEGIN
	if( !gameRenderWorld ) {
		common->Error( "GameRenderWorld is NULL!" );
	}
// RAVEN END

	// allocate the area table
	int	numAreas = gameRenderWorld->NumAreas();
	locationEntities = new idLocationEntity *[ numAreas ];
	memset( locationEntities, 0, numAreas * sizeof( *locationEntities ) );

	// for each location entity, make pointers from every area it touches
	for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if ( !ent->IsType( idLocationEntity::GetClassType() ) ) {
// RAVEN END
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
idGameLocal::AddLocation
===================
*/
idLocationEntity* idGameLocal::AddLocation( const idVec3& point, const char* name ) {
	int areaNum = gameRenderWorld->PointInArea( point );
	if ( areaNum < 0 ) {
		Warning ( "idGameLocal::AddLocation: cannot add location entity '%s' at '%g %g %g'\n", name, point.x, point.y, point.z );
		return NULL;
	}
	if ( areaNum >= gameRenderWorld->NumAreas() ) {
		Error( "idGameLocal::AddLocation: areaNum >= gameRenderWorld->NumAreas()" );
	}
	if ( locationEntities[areaNum] ) {
		Warning ( "idGameLocal::AddLocation: location '%s' already exists at '%g %g %g'\n", locationEntities[areaNum]->GetName(), point.x, point.y, point.z );
		return NULL;
	}

	// Spawn the new location entity
	idDict args;
	args.Set ( "location", name );
	locationEntities[areaNum] = static_cast<idLocationEntity*>(SpawnEntityType ( idLocationEntity::GetClassType(), &args ));

	// spread to all other connected areas
	for ( int i = gameRenderWorld->NumAreas() - 1 ; i >= 0 ; i-- ) {
		if ( i == areaNum ) {
			continue;
		}
		if ( gameRenderWorld->AreasAreConnected( areaNum, i, PS_BLOCK_LOCATION ) ) {
			locationEntities[i] = static_cast<idLocationEntity *>(locationEntities[areaNum]);
		}
	}
	
	return locationEntities[areaNum];
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

// RAVEN BEGIN
// ddynerman: new gametype specific spawn code
// TODO this should be moved to idMultiplayerGame
/*
===========
idGameLocal::InitializeSpawns
randomize the order of the initial spawns
prepare for a sequence of initial player spawns
============
*/
void idGameLocal::InitializeSpawns( void ) {
	idEntity* spot = NULL;

	// initialize the spawns for clients as well, need them for free fly demo replays
	if ( !isMultiplayer ) {
		return;
	}

	spawnSpots.Clear();

	for( int i = 0; i < TEAM_MAX; i++ ) {
		teamSpawnSpots[i].Clear();
	}
	
	spot = FindEntityUsingDef( NULL, "info_player_team" );
	while( spot ) {
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if( spot->IsType ( idPlayerStart::GetClassType() ) ) {
// RAVEN END
			if( !idStr::Icmp(spot->spawnArgs.GetString("team"), "strogg") ) {
				teamSpawnSpots[TEAM_STROGG].Append( static_cast<idPlayerStart*>(spot) );
			} else if( !idStr::Icmp(spot->spawnArgs.GetString("team"), "marine") ) {
				teamSpawnSpots[TEAM_MARINE].Append( static_cast<idPlayerStart*>(spot) );
			}

			// spawnSpots contains info_player_team as well as info_player_deathmatch
			spawnSpots.Append ( static_cast<idPlayerStart*>(spot) );
			
		}

		spot = FindEntityUsingDef( spot, "info_player_team" );
	}

	spot = FindEntityUsingDef( NULL, "info_player_deathmatch" );
	while( spot ) {
// RAVEN BEGIN
// jnewquist: Use accessor for static class type 
		if( spot->IsType ( idPlayerStart::GetClassType() ) ) {
// RAVEN END
			spawnSpots.Append ( static_cast<idPlayerStart*>(spot) );
		}
		spot = FindEntityUsingDef( spot, "info_player_deathmatch" );
	}

	while( spot ) {
		// RAVEN BEGIN
		// jnewquist: Use accessor for static class type 
		if( spot->IsType ( idPlayerStart::GetClassType() ) ) {
			// RAVEN END
			if( !idStr::Icmp(spot->spawnArgs.GetString("team"), "strogg") ) {
				teamSpawnSpots[TEAM_STROGG].Append( static_cast<idPlayerStart*>(spot) );
			} else if( !idStr::Icmp(spot->spawnArgs.GetString("team"), "marine") ) {
				teamSpawnSpots[TEAM_MARINE].Append( static_cast<idPlayerStart*>(spot) );
			}

			// spawnSpots contains info_player_team as well as info_player_deathmatch
			spawnSpots.Append ( static_cast<idPlayerStart*>(spot) );

		}

		spot = FindEntityUsingDef( spot, "info_player_team" );
	}

	if( IsFlagGameType() && ( teamSpawnSpots[ TEAM_STROGG ].Num() == 0 || teamSpawnSpots[ TEAM_MARINE ].Num() == 0 ) ) {
		Error( "InitializeSpawns() - Map must have at least one Marine and one Strogg spawn for CTF gametype.");
	}

	if( spawnSpots.Num() == 0 ) {
		Error( "InitializeSpawns() - Map must have a spawn spot." );
	}

	common->Printf( "%d general spawns\n", spawnSpots.Num() );
	common->Printf( "%d team spawns (%d strogg/%d marine)\n", teamSpawnSpots[TEAM_STROGG].Num() + teamSpawnSpots[TEAM_MARINE].Num(),
														 teamSpawnSpots[TEAM_STROGG].Num(), teamSpawnSpots[TEAM_MARINE].Num());
}

/*
===========
idGameLocal::UpdateForwardSpawn
ddynerman: Updates forward spawn lists
===========
*/
void idGameLocal::UpdateForwardSpawns( rvCTFAssaultPlayerStart* point, int team ) {
	teamForwardSpawnSpots[ team ].Append( point );
}

/*
===========
idGameLocal::ClearForwardSpawn
ddynerman: Clears forward spawn lists
===========
*/
void idGameLocal::ClearForwardSpawns( void ) {
	for( int i = 0; i < TEAM_MAX; i++ ) {
		teamForwardSpawnSpots[ i ].Clear();
	}
}

/*
===========
idGameLocal::SpotWouldTelefrag
===========
*/
bool idGameLocal::SpotWouldTelefrag( idPlayer* player, idPlayerStart* spawn ) {
	idPlayer*	playerList[ MAX_CLIENTS ];
	idBounds	bound = player->GetPhysics()->GetBounds();

	bound.TranslateSelf( spawn->GetPhysics()->GetOrigin() );
	int numEntities = PlayersTouchingBounds( player, bound, CONTENTS_BODY, playerList, MAX_CLIENTS );

	return !( numEntities == 0 );
}

/*
===========
idGameLocal::SelectSpawnSpot
ddynerman: Selects a spawn spot randomly from spots furthest from the player
			This is taken from q3
===========
*/
idEntity* idGameLocal::SelectSpawnPoint( idPlayer* player ) {
	if( !isMultiplayer ) {
		idEntity* ent = FindEntityUsingDef( NULL, "info_player_start" );
		if ( !ent ) {
			Error( "No info_player_start on map.\n" );
		}
		return ent;
	}

	if ( player == NULL ) {
		return NULL;
	}

	// Give spectators any old random spot
	if ( player->team < 0 || player->team >= TEAM_MAX || player->spectating ) {
		common->DPrintf("Returning a random spot\n");
		return spawnSpots[ random.RandomInt( spawnSpots.Num() ) ];
	}

	idList<spawnSpot_t> weightedSpawns;
	idList<idPlayerStart*>* spawnArray = NULL;

	// Pick which spawns to use based on gametype
// RITUAL BEGIN
// squirrel: added DeadZone multiplayer mode
	if( gameLocal.gameType == GAME_DM || gameLocal.gameType == GAME_TDM || gameLocal.gameType == GAME_TOURNEY ) {
		spawnArray = &spawnSpots;
	} 
	else if( IsFlagGameType() || gameLocal.gameType == GAME_DEADZONE ) {	
		if( teamForwardSpawnSpots[ player->team ].Num() ) {
			spawnArray = &teamForwardSpawnSpots[ player->team ];
		} else {
			spawnArray = &teamSpawnSpots[ player->team ];
		}
	}
// RITUAL END

	if ( spawnArray == NULL ) {
		Error( "SelectSpawnPoint() - invalid spawn list." );
		return NULL;
	}

	idVec3 refPos;
	if ( player->lastKiller != NULL && !player->lastKiller->spectating && player->lastKiller->GetInstance() == player->GetInstance() ) {
		refPos = player->lastKiller->GetPhysics()->GetOrigin();
	} else {
		refPos = player->GetPhysics()->GetOrigin();
	}

	for ( int i = 0; i < spawnArray->Num(); i++ ) {
		idPlayerStart* spot = (*spawnArray)[i];

		if ( spot->GetInstance() != player->GetInstance() || SpotWouldTelefrag( player, spot ) ) {
			continue;
		}

		idVec3	pos = spot->GetPhysics()->GetOrigin();
		float	dist = ( pos - refPos ).LengthSqr();

		spawnSpot_t	newSpot;
		
		newSpot.dist = dist;
		newSpot.ent = (*spawnArray)[ i ];
		weightedSpawns.Append( newSpot );
	}

	if ( weightedSpawns.Num() == 0 ) {
		// no spawns avaialable, spawn randomly
		common->DPrintf("no spawns avaialable, spawn randomly\n");
		return (*spawnArray)[ random.RandomInt( spawnArray->Num() ) ];
	}

	qsort( ( void * )weightedSpawns.Ptr(), weightedSpawns.Num(), sizeof( spawnSpot_t ), ( int (*)(const void *, const void *) )sortSpawnPoints );

	int rnd = rvRandom::flrand( 0.0, 1.0 ) * (weightedSpawns.Num() / 2);
	return weightedSpawns[ rnd ].ent;
}
/*
================
idGameLocal::UpdateServerInfoFlags
================
*/
// RAVEN BEGIN
// ddynerman: new gametype strings
void idGameLocal::SetGameType( void ) {
	gameType = GAME_SP;

	if ( idStr::Icmp( serverInfo.GetString( "si_gameType" ), "singleplayer" ) ) {
		mpGame.SetGameType();
	}
}
// RAVEN END

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

/*
===========
idGameLocal::ValidateServerSettings
============
*/
bool idGameLocal::ValidateServerSettings( const char* map, const char* gametype ) {
	// PickMap uses si_map directly
	// PickMap returns wether we would have to change the maps, which means settings are invalid
	assert( !idStr::Icmp( si_map.GetString(), map ) );
	if ( mpGame.PickMap( gametype, true ) ) {
		common->Printf( "map '%s' and gametype '%s' are not compatible\n", map, gametype );
		return false;
	}
	return true;
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
		if ( keyval->GetValue().Icmp( keyval2->GetValue() ) && ( !keyval->GetKey().Icmp( "si_pure" ) || !keyval->GetKey().Icmp( "si_map" ) ) ) {
			return true;
		}
	}
	return false;
}

// RAVEN BEGIN
// jshepard: update player hud to alert to end of level
/*
===================
idGameLocal::UpdateEndLevel
===================
*/
void idGameLocal::UpdateEndLevel()	{
	idPlayer * player = GetLocalPlayer();

	if( player && player->GetHud() ) {
		player->GetHud()->HandleNamedEvent( "showExit" );
	}
}


// bdube: added
/*
================
idGameLocal::GetEffect

Get the handle of the effect with the given name
================
*/
const idDecl *idGameLocal::GetEffect ( const idDict& args, const char* effectName, const rvDeclMatType* materialType ) {
	const char *effectFile = NULL;

	float chance = args.GetFloat ( idStr("effectchance ") + effectName, "1" );	
	if ( random.RandomFloat ( ) > chance ) {
		return NULL;
	}

	// we should ALWAYS be playing sounds from the def.
	// hardcoded sounds MUST be avoided at all times because they won't get precached.
	assert( !idStr::Icmpn( effectName, "fx_", 3 ) );

	if ( materialType )	{
		idStr		temp;
		const char*	result = NULL;
		
		temp = effectName;
		temp += "_";
		temp += materialType->GetName();
	
		// See if the given material effect is specified
		if ( isMultiplayer ) {
			idStr	testMP = temp;
			testMP += "_mp";

			result = args.GetString( testMP );
		}
		if ( !result || !*result ) {
			result = args.GetString( temp );
		}
		if ( result && *result) {
			return( ( const idDecl * )declManager->FindEffect( result ) );
		}
	}	

	// grab the non material effect name
	if ( isMultiplayer ) {
		idStr	testMP = effectName;
		testMP += "_mp";

		effectFile = args.GetString( testMP );
	}

	if ( !effectFile || !*effectFile ) {
		effectFile = args.GetString( effectName );
	}

	if ( !effectFile || !*effectFile ) {
		return NULL;
	}

	return( ( const idDecl * )declManager->FindEffect( effectFile ) );
}

/*
================
idGameLocal::PlayEffect

Plays an effect at the given origin using the given direction
================
*/
rvClientEffect* idGameLocal::PlayEffect( 
	const idDecl			*effect, 
	const idVec3&			origin, 
	const idMat3&			axis, 
	bool					loop, 
	const idVec3&			endOrigin, 
	bool					broadcast,
	bool					predictBit,
	effectCategory_t		category,
	const idVec4&			effectTint ) {

	if ( !effect ) {
		return NULL;
	}

	if ( !gameLocal.isNewFrame ) {
		return NULL;
	}

	if ( isServer && broadcast ) {
		idBitMsg	msg;
		byte		msgBuf[MAX_GAME_MESSAGE_SIZE];
		idCQuat		quat;

		quat = axis.ToCQuat();
		msg.Init( msgBuf, sizeof( msgBuf ) );
		msg.BeginWriting();
		msg.WriteByte( GAME_UNRELIABLE_MESSAGE_EFFECT );
		idGameLocal::WriteDecl( msg, effect );
		msg.WriteFloat( origin.x );
		msg.WriteFloat( origin.y );
		msg.WriteFloat( origin.z );
		msg.WriteFloat( quat.x );
		msg.WriteFloat( quat.y );
		msg.WriteFloat( quat.z );
		msg.WriteBits( loop, 1 );
		msg.WriteFloat( endOrigin.x );
		msg.WriteFloat( endOrigin.y );
		msg.WriteFloat( endOrigin.z );
		msg.WriteByte( category );

		// send to everyone who has start or end in it's PVS
		SendUnreliableMessagePVS( msg, currentThinkingEntity, pvs.GetPVSArea( origin ), pvs.GetPVSArea( endOrigin ) );
	}

	if ( isServer && localClientNum < 0 ) {
		// no effects on dedicated server
		return NULL;
	}

	if ( bse->Filtered( effect->GetName(), category ) ) {
		// Effect filtered out
		return NULL;
	}
// RAVEN END

	if ( gameLocal.isListenServer && currentThinkingEntity && gameLocal.GetLocalPlayer() ) {
		if ( currentThinkingEntity->GetInstance() != gameLocal.GetLocalPlayer()->GetInstance() ) {
			return NULL;
		}
	}
	
	// mwhitlock: Dynamic memory consolidation
	RV_PUSH_SYS_HEAP_ID(RV_HEAP_ID_MULTIPLE_FRAME);
	rvClientEffect* clientEffect = new rvClientEffect( effect );
	RV_POP_HEAP();

	if( !clientEffect ) {
		common->Warning( "Failed to create effect \'%s\'\n", effect->GetName() );
		return NULL;
	}

	if( clientEffect->entityNumber == -1 ) {
		common->Warning( "Failed to spawn effect \'%s\'\n", effect->GetName() );
		delete clientEffect;
		return NULL;
	}

	clientEffect->SetOrigin( origin );
	clientEffect->SetAxis( axis );
	clientEffect->SetGravity( GetCurrentGravity( origin, axis ) );
	if ( !clientEffect->Play( gameLocal.time, loop, endOrigin ) ) {
		delete clientEffect;
		return NULL;
	}

	clientEffect->GetRenderEffect()->shaderParms[ SHADERPARM_RED ]		= effectTint[ 0 ];
	clientEffect->GetRenderEffect()->shaderParms[ SHADERPARM_GREEN ]	= effectTint[ 1 ];
	clientEffect->GetRenderEffect()->shaderParms[ SHADERPARM_BLUE ]		= effectTint[ 2 ];
	clientEffect->GetRenderEffect()->shaderParms[ SHADERPARM_ALPHA ]	= effectTint[ 3 ];

	return clientEffect;
}

void idGameLocal::CheckPlayerWhizzBy( idVec3 start, idVec3 end, idEntity* hitEnt, idEntity *attacker )
{
	//FIXME: make this client-side?  Work in MP?
	if ( gameLocal.isMultiplayer ) {
		return;
	}
	idPlayer* player = gameLocal.GetLocalPlayer();
	if ( !player ) {
		return;
	}
	if ( player->IsHidden() ) {
		return;
	}
	if ( player == attacker ) {
		return;
	}
	if ( player == hitEnt ) {
		return;
	}
	idVec3 center = player->firstPersonViewOrigin;
	idVec3 diff = end-center;
	if ( diff.Length() < 64.0f ) {
		//hit too close - didn't actually pass by, will hear impact sound instead
		return;
	}
	idVec3 closestPoint = player->firstPersonViewOrigin;
	if ( closestPoint.ProjectToLineSeg( start, end ) ) {
		//on line seg
		diff = closestPoint-center;
		if ( diff.Length() < 48.0f ) {
			//close enough to hear whizz-by
			idVec3 dir = end-start;
			dir.Normalize();
			idVec3 fxStart = closestPoint+dir*-32.0f;
			idVec3 fxEnd = closestPoint+dir*32.0f;
			player->PlayEffect( "fx_whizby", fxStart, player->firstPersonViewAxis, false, fxEnd );
		}
	}
}

/*
================
idGameLocal::HitScan

Run a hitscan trace from the given origin and direction
================
*/
idEntity* idGameLocal::HitScan( 
	const idDict&	hitscanDict, 
	const idVec3&	origOrigin, 
	const idVec3&	origDir, 
	const idVec3&	origFxOrigin, 
	idEntity*		owner, 
	bool			noFX,
	float			damageScale,
// twhitaker: added additionalIgnore parameter
	idEntity*		additionalIgnore,
	int				areas[ 2 ]
	) {

	idVec3		dir;
	idVec3		origin;
	idVec3		fxOrigin;
	idVec3		fxDir;
	idVec3		impulse;
	idVec4		hitscanTint( 1.0f, 1.0f, 1.0f, 1.0f );
	int			reflect;
	float		tracerChance;
	idEntity*	ignore;
	float		penetrate;

	if ( areas ) {
		areas[ 0 ] = pvs.GetPVSArea( origFxOrigin );
		areas[ 1 ] = -1;
	}

	ignore    = owner;
	penetrate = hitscanDict.GetFloat( "penetrate" );

	if( hitscanDict.GetBool( "hitscanTint" ) && owner->IsType( idPlayer::GetClassType() ) ) {
		hitscanTint = ((idPlayer*)owner)->GetHitscanTint();
	}

	// twhitaker: additionalIgnore parameter
	if ( !additionalIgnore ) {
		additionalIgnore = ignore;
	}	

	origin		 = origOrigin;
	fxOrigin	 = origFxOrigin;
	dir			 = origDir;
	tracerChance = ((g_perfTest_weaponNoFX.GetBool())?0:hitscanDict.GetFloat( "tracerchance", "0" ));

	// Apply player powerups
	if ( owner && owner->IsType( idPlayer::GetClassType() ) ) {
		damageScale *= static_cast<idPlayer*>(owner)->PowerUpModifier(PMOD_PROJECTILE_DAMAGE);
	}
	
	// Run reflections
	for ( reflect = hitscanDict.GetFloat( "reflect", "0" ); reflect >= 0; reflect-- ) {	
		idVec3		start;
		idVec3		end;
		idEntity*	ent;
		idEntity*	actualHitEnt;
		trace_t		tr;
		int			contents;
		int			collisionArea;
		idVec3		collisionPoint;
		bool		tracer;
		
		// Calculate the end point of the trace
		start    = origin;
		if ( g_perfTest_hitscanShort.GetBool() ) {
			end		 = start + (dir.ToMat3() * idVec3(idMath::ClampFloat(0,2048,hitscanDict.GetFloat ( "range", "2048" )),0,0));
		} else {
			end		 = start + (dir.ToMat3() * idVec3(hitscanDict.GetFloat ( "range", "40000" ),0,0));
		}
		if ( g_perfTest_hitscanBBox.GetBool() ) {
			contents = MASK_SHOT_BOUNDINGBOX|CONTENTS_PROJECTILE;
		} else {
			contents = MASK_SHOT_RENDERMODEL|CONTENTS_WATER|CONTENTS_PROJECTILE;
		}
		
		// Loop the traces to handle cases where something can be shot through
		while ( 1 ) {				
			// Trace to see if we hit any entities
// RAVEN BEGIN
// ddynerman: multiple clip worlds
			if ( hitscanDict.GetFloat( "trace_size", "0" ) > 0.0f )
			{
				float range = hitscanDict.GetFloat ( "range", "1024" );
				if ( range > 4096.0f )
				{
					assert( !(range > 4096.0f) );
					Warning( "idGameLocal::HitScan: hitscan def (%s) with trace_size must have max range of 4096!", hitscanDict.GetString( "classname" ) );
					range = idMath::ClampFloat( 0.0f, 4096.0f, range );
				}
				end	= start + (dir * range);

				idBounds traceBounds;
				traceBounds.Zero();
				traceBounds.ExpandSelf( hitscanDict.GetFloat( "trace_size", "0" ) );
				// twhitaker: additionalIgnore parameter
				TraceBounds( owner, tr, start, end, traceBounds, contents, additionalIgnore );
			}
			else
			{
				// twhitaker: additionalIgnore parameter
				TracePoint( owner, tr, start, end, contents, additionalIgnore );
			}
			//gameRenderWorld->DebugArrow( colorRed, start, end, 10, 5000 );
// RAVEN END
			
			// If the hitscan hit a no impact surface we can just return out
			//assert( tr.c.material );
			if ( tr.fraction >= 1.0f || (tr.c.material && tr.c.material->GetSurfaceFlags() & SURF_NOIMPACT) ) {					
				PlayEffect( hitscanDict, "fx_path", fxOrigin, dir.ToMat3(), false, tr.endpos, false, EC_IGNORE, hitscanTint );	
				if ( random.RandomFloat( ) < tracerChance ) {
					PlayEffect( hitscanDict, "fx_tracer", fxOrigin, dir.ToMat3(), false, tr.endpos );
					tracer = true;
				} else {
					tracer = false;
				}

				if ( areas ) {
					collisionArea = pvs.GetPVSArea( tr.endpos );
					if ( collisionArea != areas[0] ) {
						areas[1] = collisionArea;
					}
				}

				return NULL;
			}

			// computing the collisionArea from the collisionPoint fails sometimes
			if ( areas ) {
				collisionArea  = pvs.GetPVSArea( tr.c.point );
				if ( collisionArea != areas[0] ) {
					areas[1] = collisionArea;
				}
			}
			collisionPoint = tr.c.point - ( tr.c.normal * tr.c.point - tr.c.dist ) * tr.c.normal;
			ent			   = entities[ tr.c.entityNum ];
			actualHitEnt   = NULL;
			start		   = collisionPoint;

			// Keep tracing if we hit water
			if ( (ent->GetPhysics()->GetContents() & CONTENTS_WATER) || (tr.c.material && (tr.c.material->GetContentFlags() & CONTENTS_WATER)) ) {
				// Apply force to the water entity that was hit
				ent->ApplyImpulse( owner, tr.c.id, tr.c.point, -(hitscanDict.GetFloat( "push", "5000" )) * tr.c.normal );
				// Continue on excluding water
				contents &= (~CONTENTS_WATER);

				if ( !g_perfTest_weaponNoFX.GetBool() ) {
					if ( ent->CanPlayImpactEffect( owner, ent ) ) {
						if ( ent->IsType( idMover::GetClassType( ) ) ) {
							ent->PlayEffect( GetEffect( hitscanDict, "fx_impact", tr.c.materialType ), collisionPoint, tr.c.normal.ToMat3(), false, vec3_origin, false, EC_IMPACT, hitscanTint );
						} else {
							gameLocal.PlayEffect( GetEffect( hitscanDict, "fx_impact", tr.c.materialType ), collisionPoint, tr.c.normal.ToMat3(), false, vec3_origin, false, false, EC_IMPACT, hitscanTint );
						}
					}
				}

				continue;
			// Reflect off a bounce target?
			} else if ( (tr.c.material->GetSurfaceFlags ( ) & SURF_BOUNCE) && !hitscanDict.GetBool ( "noBounce" ) ) {
				reflect++;
			}

			// If the hit entity is bound to an actor use the actor instead
			if ( ent->fl.takedamage && ent->GetTeamMaster( ) && ent->GetTeamMaster( )->IsType ( idActor::GetClassType() ) ) {
				actualHitEnt = ent;
				ent = ent->GetTeamMaster( );
			}

			if ( !gameLocal.isClient ) {

				// Apply force to the entity that was hit
				ent->ApplyImpulse( owner, tr.c.id, tr.c.point, -tr.c.normal, &hitscanDict );

				// Handle damage to the entity
				if ( ent->fl.takedamage && !(( tr.c.material != NULL ) && ( tr.c.material->GetSurfaceFlags() & SURF_NODAMAGE )) ) {		
					const char*	damage;
				
					damage    = NULL;

					// RAVEN BEGIN
					// jdischler: code from the other project..to ensure that if an attached head is hit, the body will use the head joint
					//	otherwise damage zones for head attachments no-worky
					int hitJoint = CLIPMODEL_ID_TO_JOINT_HANDLE(tr.c.id);
					if ( ent->IsType(idActor::GetClassType()) )
						{
							idActor* entActor = static_cast<idActor*>(ent);
							if ( entActor && entActor->GetHead() && entActor->GetHead()->IsType(idAFAttachment::GetClassType()) )
								{
									idAFAttachment* headEnt = static_cast<idAFAttachment*>(entActor->GetHead());
									if ( headEnt && headEnt->entityNumber == tr.c.entityNum )
										{//hit ent's head, get the proper joint for the head
											hitJoint = entActor->GetAnimator()->GetJointHandle("head");
										}
								}
						}	
					// RAVEN END
					// Inflict damage
					if ( tr.c.materialType ) {
						damage = hitscanDict.GetString( va("def_damage_%s", tr.c.materialType->GetName()) );
					}
					if ( !damage || !*damage ) {
						damage = hitscanDict.GetString ( "def_damage" );
					}

					if ( damage && damage[0] ) {
						// RAVEN BEGIN
						// ddynerman: stats
						if( owner->IsType( idPlayer::GetClassType() ) && ent->IsType( idActor::GetClassType() ) && ent != owner && !((idPlayer*)owner)->pfl.dead ) {
							statManager->WeaponHit( (idActor*)owner, ent, ((idPlayer*)owner)->GetCurrentWeapon() );
						}
						// RAVEN END
						ent->Damage( owner, owner, dir, damage, damageScale, hitJoint );
					}

					// Let the entity add its own damage effect
					if ( !g_perfTest_weaponNoFX.GetBool() ) {
						ent->AddDamageEffect ( tr, dir, damage, owner );
					}
				} else { 
					if ( actualHitEnt
						 && actualHitEnt != ent
						 && (tr.c.material->GetSurfaceFlags ( ) & SURF_BOUNCE)
						 && actualHitEnt->spawnArgs.GetBool( "takeBounceDamage" ) )
						{//bleh...
							const char*	damage = NULL;
							// Inflict damage
							if ( tr.c.materialType ) {
								damage = hitscanDict.GetString( va("def_damage_%s", tr.c.materialType->GetName()) );
							}
							if ( !damage || !*damage ) {
								damage = hitscanDict.GetString ( "def_damage" );
							}
							if ( damage && damage[0] ) {
								actualHitEnt->Damage( owner, owner, dir, damage, damageScale, CLIPMODEL_ID_TO_JOINT_HANDLE( tr.c.id ) );
							}
						}
					if ( !g_perfTest_weaponNoFX.GetBool() ) {
						ent->AddDamageEffect( tr, dir, hitscanDict.GetString ( "def_damage" ), owner );
					}
				}
			}


			// Pass through actors
			if ( ent->IsType ( idActor::GetClassType() ) && penetrate > 0.0f ) {			
				start = collisionPoint;
				additionalIgnore = ent;
				damageScale *= penetrate;
				continue;
			}
			break;
		}
			
		// Path effect 
		fxDir = collisionPoint - fxOrigin;
		fxDir.Normalize( );
		PlayEffect( hitscanDict, "fx_path", fxOrigin, fxDir.ToMat3(), false, collisionPoint, false, EC_IGNORE, hitscanTint );	
		if ( !ent->fl.takedamage && random.RandomFloat ( ) < tracerChance ) {
			PlayEffect( hitscanDict, "fx_tracer", fxOrigin, fxDir.ToMat3(), false, collisionPoint );
			tracer = true;
		} else {
			tracer = false;
		}

		if ( !reflect ) {
			//on initial trace only
			if ( hitscanDict.GetBool( "doWhizz" ) ) {
				//play whizz-by sound if trace is close to player's head
				CheckPlayerWhizzBy( origin, collisionPoint, ent, owner );
			}
		}

		// Play a different effect when reflecting
		if ( !reflect || ent->fl.takedamage ) {
			idMat3 axis;
			
			// Effect axis when hitting actors is along the direction of impact because actor models are 
			// very detailed.
			if ( ent->IsType ( idActor::GetClassType() ) ) {
				axis = ((-dir + tr.c.normal) * 0.5f).ToMat3();
			} else {
				axis = tr.c.normal.ToMat3();
			}
			
			if ( !g_perfTest_weaponNoFX.GetBool() ) {
				if ( ent->CanPlayImpactEffect( owner, ent ) ) {
					if ( ent->IsType( idMover::GetClassType( ) ) ) {
						ent->PlayEffect( GetEffect( hitscanDict, "fx_impact", tr.c.materialType ), collisionPoint, axis, false, vec3_origin, false, EC_IMPACT, hitscanTint );					
					} else {
						gameLocal.PlayEffect( GetEffect( hitscanDict, "fx_impact", tr.c.materialType ), collisionPoint, axis, false, vec3_origin, false, false, EC_IMPACT, hitscanTint );
					}
				}
			}
			
			// End of reflection
			return ent;
		} else {
			PlayEffect( GetEffect( hitscanDict, "fx_reflect", tr.c.materialType ), collisionPoint, tr.c.normal.ToMat3() );
		}
		
		// Calc new diretion based on bounce
		origin = start;
		fxOrigin = start;
		dir = ( dir - ( 2.0f * DotProduct( dir, tr.c.normal ) * tr.c.normal ) );
		dir.Normalize( );

		// Increase damage scale on reflect		
		damageScale += hitscanDict.GetFloat( "reflect_powerup", "0" );
	}	
	
	assert( false );
	
	return NULL;
}

/*
===================
idGameLocal::RegisterClientEntity
===================
*/
void idGameLocal::RegisterClientEntity( rvClientEntity *cent ) {
	int entityNumber;

	assert ( cent );

	if ( clientSpawnCount >= ( 1 << ( 32 - CENTITYNUM_BITS ) ) ) {
//		Error( "idGameLocal::RegisterClientEntity: spawn count overflow" );
		clientSpawnCount = INITIAL_SPAWN_COUNT;
	}

	// Find a free entity index to use
	while( clientEntities[firstFreeClientIndex] && firstFreeClientIndex < MAX_CENTITIES ) {
		firstFreeClientIndex++;
	}

	if ( firstFreeClientIndex >= MAX_CENTITIES ) {
		cent->PostEventMS ( &EV_Remove, 0 );
		Warning( "idGameLocal::RegisterClientEntity: no free client entities" );
		return;
	}

	entityNumber = firstFreeClientIndex++;

	// Add the client entity to the lists
	clientEntities[ entityNumber ] = cent;
	clientSpawnIds[ entityNumber ] = clientSpawnCount++;
	cent->entityNumber = entityNumber;
	cent->spawnNode.AddToEnd( clientSpawnedEntities );
	cent->spawnArgs.TransferKeyValues( spawnArgs );

	if ( entityNumber >= num_clientEntities ) {
		num_clientEntities++;
	}
}

/*
===================
idGameLocal::UnregisterClientEntity
===================
*/
void idGameLocal::UnregisterClientEntity( rvClientEntity* cent ) {
	assert( cent );
	
	// No entity number then it failed to register
	if ( cent->entityNumber == -1 ) {
		return;
	}
	
	cent->spawnNode.Remove ( );
	cent->bindNode.Remove ( );

	if ( clientEntities [ cent->entityNumber ] == cent ) {
		clientEntities [ cent->entityNumber ] = NULL;
		clientSpawnIds[ cent->entityNumber ] = -1;
		if ( cent->entityNumber < firstFreeClientIndex ) {
			firstFreeClientIndex = cent->entityNumber;
		}
		cent->entityNumber = -1;
	}
}

// RAVEN BEGIN
// ddynerman: idClip wrapper functions

/*
===================
idGameLocal::Translation
===================
*/
bool idGameLocal::Translation( const idEntity* ent, trace_t &results, const idVec3 &start, const idVec3 &end, const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, const idEntity *passEntity2 ) {
	idClip* clipWorld = GetEntityClipWorld( ent );
	
	if( clipWorld ) {
		return clipWorld->Translation( results, start, end, mdl, trmAxis, contentMask, passEntity, passEntity2 );
	} 
	
	return false;
}

/*
===================
idGameLocal::Rotation
===================
*/
bool idGameLocal::Rotation( const idEntity* ent, trace_t &results, const idVec3 &start, const idRotation &rotation, const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity ) {
	idClip* clipWorld = GetEntityClipWorld( ent );
	
	if( clipWorld ) {
		return clipWorld->Rotation( results, start, rotation, mdl, trmAxis, contentMask, passEntity );
	}

	return false;
}

/*
===================
idGameLocal::Motion
===================
*/
bool idGameLocal::Motion( const idEntity* ent, trace_t &results, const idVec3 &start, const idVec3 &end, const idRotation &rotation, const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity ) {
	idClip* clipWorld = GetEntityClipWorld( ent );
	
	if( clipWorld ) {
		return clipWorld->Motion( results, start, end, rotation, mdl, trmAxis, contentMask, passEntity );
	}

	return false;
}


/*
===================
idGameLocal::Contacts
===================
*/
int idGameLocal::Contacts( const idEntity* ent, contactInfo_t *contacts, const int maxContacts, const idVec3 &start, const idVec6 &dir, const float depth, const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity ) {
	idClip* clipWorld = GetEntityClipWorld( ent );
	
	if( clipWorld ) {
		return clipWorld->Contacts( contacts, maxContacts, start, dir, depth, mdl, trmAxis, contentMask, passEntity );
	}

	return 0;
}

/*
===================
idGameLocal::Contents
===================
*/
int idGameLocal::Contents( const idEntity* ent, const idVec3 &start, const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, idEntity **touchedEntity ) {
	idClip* clipWorld = GetEntityClipWorld( ent );
	
	if( clipWorld ) {
		return clipWorld->Contents( start, mdl, trmAxis, contentMask, passEntity, touchedEntity );
	}

	return 0;
}

/*
===================
idGameLocal::TracePoint
===================
*/
bool idGameLocal::TracePoint( const idEntity* ent, trace_t &results, const idVec3 &start, const idVec3 &end, int contentMask, const idEntity *passEntity ) {
	idClip* clipWorld = GetEntityClipWorld( ent );
	
	if( clipWorld ) {
		return clipWorld->TracePoint( results, start, end, contentMask, passEntity );
	}

	return false;
}

/*
===================
idGameLocal::TraceBounds
===================
*/
bool idGameLocal::TraceBounds( const idEntity* ent, trace_t &results, const idVec3 &start, const idVec3 &end, const idBounds &bounds, int contentMask, const idEntity *passEntity ) {
	idClip* clipWorld = GetEntityClipWorld( ent );
	
	if( clipWorld ) {
		return clipWorld->TraceBounds( results, start, end, bounds, contentMask, passEntity );
	}

	return false;
}

/*
===================
idGameLocal::TranslationModel
===================
*/
void idGameLocal::TranslationModel( const idEntity* ent, trace_t &results, const idVec3 &start, const idVec3 &end, const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, idCollisionModel *model, const idVec3 &modelOrigin, const idMat3 &modelAxis ) {
	idClip* clipWorld = GetEntityClipWorld( ent );
	
	if( clipWorld ) {
		clipWorld->TranslationModel( results, start, end, mdl, trmAxis, contentMask, model, modelOrigin, modelAxis );
	}
}

/*
===================
idGameLocal::RotationModel
===================
*/
void idGameLocal::RotationModel( const idEntity* ent, trace_t &results, const idVec3 &start, const idRotation &rotation, const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, idCollisionModel *model, const idVec3 &modelOrigin, const idMat3 &modelAxis ) {
	idClip* clipWorld = GetEntityClipWorld( ent );
	
	if( clipWorld ) {
		clipWorld->RotationModel( results, start, rotation, mdl, trmAxis, contentMask, model, modelOrigin, modelAxis );
	}
}

/*
===================
idGameLocal::ContactsModel
===================
*/
int idGameLocal::ContactsModel( const idEntity* ent, contactInfo_t *contacts, const int maxContacts, const idVec3 &start, const idVec6 &dir, const float depth, const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, idCollisionModel *model, const idVec3 &modelOrigin, const idMat3 &modelAxis ) {
	idClip* clipWorld = GetEntityClipWorld( ent );
	
	if( clipWorld ) {
		return clipWorld->ContactsModel( contacts, maxContacts, start, dir, depth, mdl, trmAxis, contentMask, model, modelOrigin, modelAxis );
	}

	return 0;
}

/*
===================
idGameLocal::ContentsModel
===================
*/
int idGameLocal::ContentsModel( const idEntity* ent, const idVec3 &start, const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, idCollisionModel *model, const idVec3 &modelOrigin, const idMat3 &modelAxis ) {
	idClip* clipWorld = GetEntityClipWorld( ent );
	
	if( clipWorld ) {
		return clipWorld->ContentsModel( start, mdl, trmAxis, contentMask, model, modelOrigin, modelAxis );
	}

	return 0;
}

/*
===================
idGameLocal::TranslationEntities
===================
*/	
void idGameLocal::TranslationEntities( const idEntity* ent, trace_t &results, const idVec3 &start, const idVec3 &end, const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, const idEntity *passEntity2 ) {
	idClip* clipWorld = GetEntityClipWorld( ent );
	
	if( clipWorld ) {
		clipWorld->TranslationEntities( results, start, end, mdl, trmAxis, contentMask, passEntity, passEntity2 );
	}
}

/*
===================
idGameLocal::GetModelContactFeature
===================
*/
bool idGameLocal::GetModelContactFeature( const idEntity* ent, const contactInfo_t &contact, const idClipModel *clipModel, idFixedWinding &winding ) const {
	const idClip* clipWorld = GetEntityClipWorld( ent );
	
	if( clipWorld ) {
		return clipWorld->GetModelContactFeature( contact, clipModel, winding );
	}

	return false;
}

/*
===================
idGameLocal::EntitiesTouchingBounds
===================
*/
int idGameLocal::EntitiesTouchingBounds	( const idEntity* ent, const idBounds &bounds, int contentMask, idEntity **entityList, int maxCount ) const {
	const idClip* clipWorld = GetEntityClipWorld( ent );
	
	if( clipWorld ) {
		return clipWorld->EntitiesTouchingBounds( bounds, contentMask, entityList, maxCount );
	}

	return 0;
}

/*
===================
idGameLocal::ClipModelsTouchingBounds
===================
*/
int idGameLocal::ClipModelsTouchingBounds( const idEntity* ent, const idBounds &bounds, int contentMask, idClipModel **clipModelList, int maxCount ) const {
	const idClip* clipWorld = GetEntityClipWorld( ent );
	
	if( clipWorld ) {
		return clipWorld->ClipModelsTouchingBounds( bounds, contentMask, clipModelList, maxCount );
	}

	return 0;
}

/*
===================
idGameLocal::PlayersTouchingBounds
===================
*/
int idGameLocal::PlayersTouchingBounds	( const idEntity* ent, const idBounds &bounds, int contentMask, idPlayer **entityList, int maxCount ) const {
	const idClip* clipWorld = GetEntityClipWorld( ent );
	
	if( clipWorld ) {
		return clipWorld->PlayersTouchingBounds( bounds, contentMask, entityList, maxCount );
	}

	return 0;
}

/*
===================
idGameLocal::GetWorldBounds
===================
*/
const idBounds& idGameLocal::GetWorldBounds( const idEntity* ent ) const {
	const idClip* clipWorld = GetEntityClipWorld( ent );
	
	if ( clipWorld ) {
		return clipWorld->GetWorldBounds();
	}

	return clip[ 0 ]->GetWorldBounds();
}

/*
===================
idGameLocal::GetEntityClipWorld
===================
*/
idClip* idGameLocal::GetEntityClipWorld( const idEntity* ent ) {
	if( ent == NULL ) {
		return clip[ 0 ];
	}

	if( ent->GetClipWorld() < 0 || ent->GetClipWorld() >= clip.Num() ) {
		Warning( "idGameLocal::GetEntityClipWorld() - invalid clip world %d on entity %s (valid range: 0 - %d)\n", ent->GetClipWorld(), ent->GetClassname(), clip.Num() - 1 );
		return NULL;
	}
	return clip[ ent->GetClipWorld() ];
}

/*
===================
idGameLocal::GetEntityClipWorld
===================
*/
const idClip* idGameLocal::GetEntityClipWorld( const idEntity* ent ) const {
	if( ent == NULL ) {
		return clip[ 0 ];
	}

	if( ent->GetClipWorld() < 0 || ent->GetClipWorld() >= clip.Num() ) {
		Warning( "idGameLocal::GetEntityClipWorld() - invalid clip world %d on entity %s (valid range: 0 - %d)\n", ent->GetClipWorld(), ent->GetClassname(), clip.Num() - 1  );
		return NULL;
	}
	return clip[ ent->GetClipWorld() ];
}

/*
===================
idGameLocal::AddClipWorld
===================
*/
int idGameLocal::AddClipWorld( int id ) {
	if( id >= clip.Num() ) {
		// if we want an index higher in the list, fill the intermediate indices with empties
		for( int i = clip.Num(); i <= id; i++ ) {
			clip.Append( NULL );
		}
	}

	if( clip[ id ] == NULL ) {
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
		RV_PUSH_SYS_HEAP_ID(RV_HEAP_ID_LEVEL);
// RAVEN END
		clip[ id ] = new idClip();
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
		RV_POP_HEAP();
// RAVEN END
		clip[ id ]->Init();
	}
	return id;
}

/*
===================
idGameLocal::RemoveClipWorld
===================
*/
void idGameLocal::RemoveClipWorld( int id ) {
	assert( id >= 0 && id < clip.Num() );

	clip[ id ]->Shutdown();
	delete clip[ id ];
	clip[ id ] = NULL;
}

/*
===================
idGameLocal::ShutdownInstances
===================
*/
void idGameLocal::ShutdownInstances( void ) {
	if( gamestate == GAMESTATE_UNINITIALIZED ) {
		return;
	}

	instances.DeleteContents( true );

	// free the trace model used for the defaultClipModel
	idClip::FreeDefaultClipModel();
}

/*
===================
idGameLocal::AddInstance
===================
*/
int idGameLocal::AddInstance( int id, bool deferPopulate ) {
	if ( id == -1 ) {
		id = instances.Num();
	}

	if ( id >= instances.Num() ) {
		// if we want an index higher in the list, fill the intermediate indices with empties
		for( int i = instances.Num(); i <= id; i++ ) {
			instances.Append( NULL );
		}
	}

	if ( instances[ id ] == NULL ) {
// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
		RV_PUSH_SYS_HEAP_ID(RV_HEAP_ID_LEVEL);
		instances[ id ] = new rvInstance( id, deferPopulate );
		RV_POP_HEAP();
// RAVEN END

		if ( !deferPopulate ) {
			// keep track of the high watermark
			ServerSetEntityIndexWatermark( id );
		}

		common->DPrintf( "idGameLocal::AddInstance(): Adding instance %d\n", instances[ id ]->GetInstanceID() );
	} else {
		common->DPrintf( "idGameLocal::AddInstance(): Instance %d already exists\n", instances[ id ]->GetInstanceID() );
	}
	
	// keep the min spawn index correctly set
	ServerSetMinSpawnIndex();
	
	return instances[ id ]->GetInstanceID();
}

/*
===================
idGameLocal::RemoveInstance
===================
*/
void idGameLocal::RemoveInstance( int id ) {
	delete instances[ id ];
	instances[ id ] = NULL;
}

/*
===================
idGameLocal::GetPlayerName
Returns the specified player name, max of 64 chars
===================
*/
void idGameLocal::GetPlayerName( int clientNum, char* name ) {
	if( !gameLocal.entities[ clientNum ] ) {
		return;
	}

	strncpy( name, gameLocal.GetUserInfo( clientNum )->GetString( "ui_name" ), 64 );
	name[ 63 ] = 0;
}

/*
===================
idGameLocal::GetPlayerClan
Returns the specified player clan, max of 64 chars
===================
*/
void idGameLocal::GetPlayerClan( int clientNum, char* clan ) {
	if( !gameLocal.entities[ clientNum ] ) {
		return;
	}

	strncpy( clan, gameLocal.GetUserInfo( clientNum )->GetString( "ui_clan" ), 64 );
	clan[ 63 ] = 0;
}

/*
===================
idGameLocal::SetFriend
===================
*/
void idGameLocal::SetFriend( int clientNum, bool isFriend ) {
	if( !gameLocal.GetLocalPlayer() ) {
		Warning( "idGameLocal::SetFriend() - SetFriend() called with NULL local player\n" );
		return;
	}

	gameLocal.GetLocalPlayer()->SetFriend( clientNum, isFriend );
}

/*
===================
idGameLocal::GetLongGametypeName
===================
*/
const char*	idGameLocal::GetLongGametypeName( const char* gametype ) {
	return mpGame.GetLongGametypeName( gametype );
}

void idGameLocal::Cmd_PrintMapEntityNumbers_f( const idCmdArgs& args ) {
	int instance = 0;

	if ( args.Argc() > 1 ) {
		instance = atoi( args.Argv( 1 ) );
	} 

	if( gameLocal.instances[ instance ] ) {
		gameLocal.instances[ instance ]->PrintMapNumbers();
	}
}

void idGameLocal::Cmd_PrintSpawnIds_f( const idCmdArgs& args ) {
	for( int i = 0; i < MAX_GENTITIES; i++ ) {
		if( gameLocal.entities[ i ] ) {
			gameLocal.Printf( "Spawn id %d: %d\n", i, gameLocal.spawnIds[ i ] );
		}
	}
}

/*
===============
idGameLocal::GetDemoHud
===============
*/
idUserInterface	*idGameLocal::GetDemoHud( void ) {
	if ( !demo_hud ) {
		demo_hud = uiManager->FindGui( "guis/hud.gui", true, false, true );
		assert( demo_hud );
	}
	return demo_hud;
}

/*
===============
idGameLocal::GetDemoMphud
===============
*/
idUserInterface	*idGameLocal::GetDemoMphud( void ) {
	if ( !demo_mphud ) {
		demo_mphud = uiManager->FindGui( "guis/mphud.gui", true, false, true );
		assert( demo_mphud );
	}
	return demo_mphud;
}

/*
===============
idGameLocal::GetDemoCursor
===============
*/
idUserInterface *idGameLocal::GetDemoCursor( void ) {
	if ( !demo_cursor ) {
		demo_cursor = uiManager->FindGui( "guis/cursor.gui", true, false, true );	   
		assert( demo_cursor );
	}
	return demo_cursor;
}

/*
===============
idGameLocal::IsTeamPowerups
===============
*/
bool idGameLocal::IsTeamPowerups( void ) {
	if ( !serverInfo.GetBool( "si_isBuyingEnabled" ) ) {
		return false;
	}
	if ( !IsTeamGameType() ) {
		return false;
	}
	return ( gameType != GAME_ARENA_CTF );
}

// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
#if defined(_RV_MEM_SYS_SUPPORT)
/*
===================
idGameLocal::FlushBeforelevelLoad
===================
*/
void idGameLocal::FlushBeforelevelLoad( void )
{
	TIME_THIS_SCOPE( __FUNCLINE__);

#ifndef _XENON
	MapShutdown();
#else
	mpGame.Clear();
#endif
	for(int i = 0; i < aasNames.Num(); i++)
	{
		aasList[i]->Shutdown();
	}
}
#endif
// RAVEN END

// dluetscher: moved the overloaded new/delete to sys_local.cpp and Game_local.cpp (from Heap.h)
//			   so that the tools.dll will link.
#if !defined(_XBOX) && (defined(ID_REDIRECT_NEWDELETE) || defined(_RV_MEM_SYS_SUPPORT))

#undef new
#undef delete
#undef Mem_Alloc
#undef Mem_Free

#ifdef ID_DEBUG_MEMORY
void *operator new( size_t s, int t1, int t2, char *fileName, int lineNumber ) {
	return Mem_Alloc( s, fileName, lineNumber, MemScopedTag_GetTopTag() );
}

void operator delete( void *p, int t1, int t2, char *fileName, int lineNumber ) {
	Mem_Free( p, fileName, lineNumber );
}

void *operator new[]( size_t s, int t1, int t2, char *fileName, int lineNumber ) {
	return Mem_Alloc( s, fileName, lineNumber, MemScopedTag_GetTopTag() );
}

void operator delete[]( void *p, int t1, int t2, char *fileName, int lineNumber ) {
	Mem_Free( p, fileName, lineNumber );
}

void *operator new( size_t s ) {
	return Mem_Alloc( s, "", 0, MemScopedTag_GetTopTag() );
}

void operator delete( void *p ) {
	Mem_Free( p, "", 0 );
}

void *operator new[]( size_t s ) {
	return Mem_Alloc( s, "", 0, MemScopedTag_GetTopTag() );
}

void operator delete[]( void *p ) {
	Mem_Free( p, "", 0 );
}

#else	// #ifdef ID_DEBUG_MEMORY

void *operator new( size_t s ) {
	return Mem_Alloc( s, MemScopedTag_GetTopTag() );
}

void operator delete( void *p ) {
	Mem_Free( p );
}

void *operator new[]( size_t s ) {
	return Mem_Alloc( s, MemScopedTag_GetTopTag() );
}

void operator delete[]( void *p ) {
	Mem_Free( p );
}
#endif	// #else #ifdef ID_DEBUG_MEMORY
#endif	// #if defined(ID_REDIRECT_NEWDELETE) || defined(_RV_MEM_SYS_SUPPORT)
// RAVEN END
