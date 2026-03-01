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

#include "Game_local.h"

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

// List of all defs used by the player that will stay on the fast timeline
// BOYETTE NOTE: this is not used anymore - since everything is now on one timeline - we can delete it probably and remove all references to it.
static char* fastEntityList[] = {
		"player_icarus_officer",
		"weapon_chainsaw",
		"weapon_space_command_fists",
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

	portalSkyEnt			= NULL;
	portalSkyActive			= false;

	ResetSlowTimeVars();

	// boyette space command begin
	StoryRepositoryEntity = NULL;

	SpaceCommandViewscreenCamera = NULL;

		// space_command_dynamic_lights
	for ( int i = 0; i < MAX_SPACE_COMMAND_DYNAMIC_LIGHTS; i++ ) {
		space_command_dynamic_lights[i] = NULL;
	}
	// boyette space command end
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


	if(!g_xp_bind_run_once.GetBool()) {
		//The default config file contains remapped controls that support the XP weapons
		//We want to run this once after the base icarus config file has run so we can
		//have the correct xp binds
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "exec default.cfg\n" );
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "seta g_xp_bind_run_once 1\n" );
		cmdSystem->ExecuteCommandBuffer();
	}

	// load default scripts
	program.Startup( SCRIPT_DEFAULT );
	
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

	// BOYETTE STEAM INTEGRATION BEGIN
#ifdef STEAM_BUILD
	common->StoreSteamStats();
#endif
	// BOYETTE STEAM INTEGRATION END

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

	savegame.WriteInt( msec );

	savegame.WriteInt( vacuumAreaNum );

	savegame.WriteInt( entityDefBits );
	savegame.WriteBool( isServer );
	savegame.WriteBool( isClient );

	savegame.WriteInt( localClientNum );

	// snapshotEntities is used for multiplayer only

	savegame.WriteInt( realClientTime );
	savegame.WriteBool( isNewFrame );
	savegame.WriteFloat( clientSmoothing );

	portalSkyEnt.Save( &savegame );
	savegame.WriteBool( portalSkyActive );

	savegame.WriteInt( slowmoState );
	savegame.WriteFloat( slowmoMsec );
	savegame.WriteBool( quickSlowmoReset );

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

	// BOYETTE SAVE BEGIN
	for (int x = 1; x <= MAX_STARGRID_X_POSITIONS; x++) { // we can create constants called MAX_STARGRID_X_POSITIONS = 16 and MAX_STARGRID_Y_POSITIONS = 12
		for (int y = 1; y <= MAX_STARGRID_Y_POSITIONS; y++) { // we can create constants called MAX_STARGRID_X_POSITIONS = 16 and MAX_STARGRID_Y_POSITIONS = 12
			savegame.WriteBool( stargridPositionsVisitedByPlayer[x][y] );
		}
	}
	for (int x = 1; x <= MAX_STARGRID_X_POSITIONS; x++) { // we can create constants called MAX_STARGRID_X_POSITIONS = 16 and MAX_STARGRID_Y_POSITIONS = 12
		for (int y = 1; y <= MAX_STARGRID_Y_POSITIONS; y++) { // we can create constants called MAX_STARGRID_X_POSITIONS = 16 and MAX_STARGRID_Y_POSITIONS = 12
			savegame.WriteBool( stargridPositionsOffLimitsToShipAI[x][y] );
		}
	}

	savegame.WriteObject( StoryRepositoryEntity );

	savegame.WriteObject( SpaceCommandViewscreenCamera );

	for ( int i = 0; i < MAX_SPACE_COMMAND_DYNAMIC_LIGHTS ; i++ ) {
		savegame.WriteObject( space_command_dynamic_lights[i] );
	}
	// BOYETTE SAVE END

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

	// clear envirosuit sound fx
	gameSoundWorld->SetEnviroSuit( false );
	gameSoundWorld->SetSlowmo( false );

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

	portalSkyEnt			= NULL;
	portalSkyActive			= false;

	ResetSlowTimeVars();

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

	// boyette space command begin
	StoryRepositoryEntity = NULL;
	SpaceCommandViewscreenCamera = NULL;
	std::srand(randseed); // BOYETTE NOTE TODO: when we get to save games - this seed will have to be loaded again for save games.
	// boyette space command end
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
		// clear envirosuit sound fx
		gameSoundWorld->SetEnviroSuit( false );
		gameSoundWorld->SetSlowmo( false );
	}

	// the spawnCount is reset to zero temporarily to spawn the map entities with the same spawnId
	// if we don't do that, network clients are confused and don't show any map entities
	latchSpawnCount = spawnCount;
	spawnCount = INITIAL_SPAWN_COUNT;

	gamestate = GAMESTATE_STARTUP;

	// BOYETTE STEAM INTEGRATION BEGIN
#ifdef STEAM_BUILD
	common->StoreSteamStats();
#endif
	// BOYETTE STEAM INTEGRATION END

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

	if ( isMultiplayer && isServer ) {
		char buf[ MAX_STRING_CHARS ];
		idStr gametype;
		GetBestGameType( si_map.GetString(), si_gameType.GetString(), buf );
		gametype = buf;
		if ( gametype != si_gameType.GetString() ) {
			cvarSystem->SetCVarString( "si_gameType", gametype );
		}
	}


    
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

	// BOYETTE STEAM INTEGRATION BEGIN
#ifdef STEAM_BUILD
	common->StoreSteamStats();
#endif
	// BOYETTE STEAM INTEGRATION END

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

	// BOYETTE STEAM INTEGRATION BEGIN
#ifdef STEAM_BUILD
	common->StoreSteamStats();
#endif
	// BOYETTE STEAM INTEGRATION END

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
	FindEntityDef( "player_icarus_officer", false );

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

	savegame.ReadInt( msec );

	savegame.ReadInt( vacuumAreaNum );

	savegame.ReadInt( entityDefBits );
	savegame.ReadBool( isServer );
	savegame.ReadBool( isClient );

	savegame.ReadInt( localClientNum );

	// snapshotEntities is used for multiplayer only

	savegame.ReadInt( realClientTime );
	savegame.ReadBool( isNewFrame );
	savegame.ReadFloat( clientSmoothing );

	portalSkyEnt.Restore( &savegame );
	savegame.ReadBool( portalSkyActive );

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

	// BOYETTE RESTORE BEGIN
	for (int x = 1; x <= MAX_STARGRID_X_POSITIONS; x++) { // we can create constants called MAX_STARGRID_X_POSITIONS = 16 and MAX_STARGRID_Y_POSITIONS = 12
		for (int y = 1; y <= MAX_STARGRID_Y_POSITIONS; y++) { // we can create constants called MAX_STARGRID_X_POSITIONS = 16 and MAX_STARGRID_Y_POSITIONS = 12
			savegame.ReadBool( stargridPositionsVisitedByPlayer[x][y] );
		}
	}
	for (int x = 1; x <= MAX_STARGRID_X_POSITIONS; x++) { // we can create constants called MAX_STARGRID_X_POSITIONS = 16 and MAX_STARGRID_Y_POSITIONS = 12
		for (int y = 1; y <= MAX_STARGRID_Y_POSITIONS; y++) { // we can create constants called MAX_STARGRID_X_POSITIONS = 16 and MAX_STARGRID_Y_POSITIONS = 12
			savegame.ReadBool( stargridPositionsOffLimitsToShipAI[x][y] );
		}
	}

	savegame.ReadObject( reinterpret_cast<idClass *&>( StoryRepositoryEntity ) );

	savegame.ReadObject( reinterpret_cast<idClass *&>( SpaceCommandViewscreenCamera ) );

	for ( int i = 0; i < MAX_SPACE_COMMAND_DYNAMIC_LIGHTS ; i++ ) {
		savegame.ReadObject( reinterpret_cast<idClass *&>( space_command_dynamic_lights[i] ) );
	}
	// BOYETTE RESTORE END

	// spawnSpots
	// initialSpots
	// currentInitialSpot
	// newInfo
	// makingBuild
	// shakeSounds

	// Read out pending events
	idEvent::Restore( &savegame );

	savegame.RestoreObjects();

	// BOYETTE RESTORE BEGIN
	if ( GetLocalPlayer() && GetLocalPlayer()->PlayerShip ) {
		GetLocalPlayer()->SyncUpPlayerShipNameCVars();
	}
	// BOYETTE RESTORE END

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
}

/*
===========
idGameLocal::MapShutdown
============
*/
void idGameLocal::MapShutdown( void ) {
	Printf( "--------- Game Map Shutdown ----------\n" );
	
	gamestate = GAMESTATE_SHUTDOWN;

	// BOYETTE STEAM INTEGRATION BEGIN
#ifdef STEAM_BUILD
	common->StoreSteamStats();
#endif
	// BOYETTE STEAM INTEGRATION END

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
		args.Set( "classname", "player_icarus_officer_mp" );
	else if ( isMultiplayer && gameType == GAME_CTF )
		args.Set( "classname", "player_icarus_officer_ctf" );
	else
		args.Set( "classname", "player_icarus_officer" );
#else
	args.Set( "classname", isMultiplayer ? "player_icarus_officer_mp" : "player_icarus_officer" );
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

	// BOYETTE SPACE COMMAND BEGIN - this seems like a good place to organize any sbship entities and their module/console/crew etc. on the map - I think the player spawns last so all the other entities should be ready by now.
	for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( ent ) {
			ent->DoStuffAfterAllMapEntitiesHaveSpawned();
		}
	}

	StoryRepositoryEntity = gameLocal.FindEntityUsingDef( NULL, "stargrid_story_windows_repository" );

	GetSpaceCommandDynamicLights(); // BOYETTE NOTE TODO: might want to change these as well as the StoryRepositoryEntity to an idEntity pointer - that way if they are ever removed from the map it won't cause problems
	UpdateSpaceCommandDynamicLights();
	// SET THE STARGRID BACKGROUND BEGIN
	if ( StoryRepositoryEntity && GetLocalPlayer() && GetLocalPlayer()->CaptainGui ) {
		idStr StargridBackgroundMaterialName;
		StargridBackgroundMaterialName = StoryRepositoryEntity->spawnArgs.GetString("captaingui_stargrid_background","guis/steve_captain_display/star_grid_backgrounds/star_grid_background_default");
		GetLocalPlayer()->CaptainGui->SetStateString("stargrid_background",StargridBackgroundMaterialName);
		declManager->FindMaterial(StargridBackgroundMaterialName)->SetSort(SS_GUI);
	}
	// SET THE STARGRID BACKGROUND END

	GetStarGridPositionsThatAreOffLimitsToShipAI();
	RandomizeStarGridPositionsForCertainSpaceEntities();

	SetAllSkyPortalsOccupancyStatusToFalse();
	SetAllShipSkyPortalEntitiesToNull();
	AssignAllShipsAtCurrentLocToASkyPortalEntity();
	UpdateSpaceEntityVisibilities();

	for (int x = 1; x <= MAX_STARGRID_X_POSITIONS; x++) { // we can create constants called MAX_STARGRID_X_POSITIONS = 16 and MAX_STARGRID_Y_POSITIONS = 12
		for (int y = 1; y <= MAX_STARGRID_Y_POSITIONS; y++) { // we can create constants called MAX_STARGRID_X_POSITIONS = 16 and MAX_STARGRID_Y_POSITIONS = 12
			stargridPositionsVisitedByPlayer[x][y] = false; // this is not necessary because bools have a default initialized value of false - but we do it anyways for clarity.
		}
	}

	if ( GetLocalPlayer()->PlayerShip ) {
		GetLocalPlayer()->PlayerShip->MySkyPortalEnt = NULL;
		GetLocalPlayer()->PlayerShip->ClaimUnnoccupiedPlayerSkyPortalEntity();
		if ( GetLocalPlayer()->PlayerShip->MySkyPortalEnt ) {
			GetLocalPlayer()->PlayerShip->MySkyPortalEnt->Event_Activate(gameLocal.GetLocalPlayer());
		}
	} else {
		gameLocal.Warning( "There is no PlayerShip entity. The captain gui will not function properly." );
	}

	if ( GetLocalPlayer()->ShipOnBoard ) {
		GetLocalPlayer()->stargrid_positions_visited.push_back( idVec2(GetLocalPlayer()->ShipOnBoard->stargridpositionx,GetLocalPlayer()->ShipOnBoard->stargridpositiony) );
	} else if ( GetLocalPlayer()->PlayerShip ) {
		GetLocalPlayer()->stargrid_positions_visited.push_back( idVec2(GetLocalPlayer()->PlayerShip->stargridpositionx,GetLocalPlayer()->PlayerShip->stargridpositiony) );
	}

	GetLocalPlayer()->UpdateCaptainMenu();
	GetLocalPlayer()->UpdateStarGridShipPositions();
	GetLocalPlayer()->UpdateModulesPowerQueueOnCaptainGui();
	GetLocalPlayer()->UpdateWeaponsAndTorpedosTargetingOverlays();
	GetLocalPlayer()->UpdateWeaponsAndTorpedosQueuesOnCaptainGui();
	GetLocalPlayer()->UpdateCrewMemberPortraits();
	GetLocalPlayer()->UpdateReserveCrewMemberPortraits();
	GetLocalPlayer()->PopulateShipList();

	GetLocalPlayer()->SyncUpPlayerShipNameCVars();

	UpdateSunEntity();

	if ( GetLocalPlayer() ) {
		GetLocalPlayer()->ResetNotificationList();
	}

	SpaceCommandViewscreenCamera = gameLocal.FindEntityUsingDef( NULL, "space_command_viewscreen_camera" );

	// SET THE INITAL VIEWSCREEN POSITION/ANGLE BEGIN
	if ( SpaceCommandViewscreenCamera && GetLocalPlayer() && GetLocalPlayer()->PlayerShip && GetLocalPlayer()->PlayerShip->GetPhysics() ) {
		SpaceCommandViewscreenCamera->SetOrigin(GetLocalPlayer()->PlayerShip->GetPhysics()->GetOrigin() + idVec3(0,0,8));
		SpaceCommandViewscreenCamera->SetAxis(GetLocalPlayer()->PlayerShip->GetPhysics()->GetAxis());
	}
	// SET THE INITAL VIEWSCREEN POSITION/ANGLE END


	for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( ent ) {
			ent->DoStuffAfterPlayerHasSpawned();
		}
	}

	if ( GetLocalPlayer()->PlayerShip ) {
		GetLocalPlayer()->PlayerShip->Event_GetATargetShipInSpace();
		if ( GetLocalPlayer()->PlayerShip->TargetEntityInSpace ) {
			GetLocalPlayer()->SelectedEntityInSpace = GetLocalPlayer()->PlayerShip->TargetEntityInSpace;
		}
		GetLocalPlayer()->UpdateSelectedEntityInSpaceOnGuis();
	}
	// BOYETTE SPACE COMMAND END
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

/*
================
idGameLocal::RunTimeGroup2
================
*/
void idGameLocal::RunTimeGroup2() {
	idEntity *ent;
	int num = 0;

	//fast.Increment();
	//slow.Get( time, previousTime, msec, framenum, realClientTime );

	for( ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() ) {
		if ( ent->timeGroup != TIME_GROUP2 ) {
			continue;
		}

		ent->Think();
		num++;
	}

	//slow.Get( time, previousTime, msec, framenum, realClientTime );
}
 
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

	ComputeSlowMsec();

	//slow.Get( time, previousTime, msec, framenum, realClientTime );
	msec = slowmoMsec;

	if ( !isMultiplayer && g_stopTime.GetBool() ) {
		// clear any debug lines from a previous frame
		gameRenderWorld->DebugClearLines( time + 1 );

		// set the user commands for this frame
		memcpy( usercmds, clientCmds, numClients * sizeof( usercmds[ 0 ] ) );

		if ( player ) {
			player->Think();
		}

		// BOYETTE STOP TIME DOOR FIX BEGIN
		// BOYETTE NOTE: this was added 05 20 2016 - because the doors became stuck open when time was stopped and the doors were closing and then we tried to open them again. Think makes sure physics are run and some things are appropriately taken care of while time is stopped
		// BOYETTE TODO IMPORTANT: We MIGHT want to do this for all movers - they won't move because time is not changing - but it might prevent problems possibly by making sure things are updated correctly during stopped time.
		for( ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() ) {
			if ( ent && ent->IsType(idDoor::Type) ) {
				ent->Think();
			}
		}
		// BOYETTE STOP TIME DOOR FIX END

		 //BOYETTE SPACE COMMAND BEGIN // BOOGLA // this is necessary to have the gui overlay (hail gui and story guis) still perform "on time" animations. Although it might defeat the purpose of stop time since the time is updating. TESTED it and it doesn't seem to be a problem so far. The ship module charges don't charge and it seems the AI scripts don't run(or at least the AI doesn't move). So it should be fine to do this.
		// update the game time
		if ( g_stopTimeForceFrameNumUpdateDuring.GetBool() ) {
			framenum++;
			previousTime = time;
			time += msec;
			realClientTime = time;

			//slow.Set( time, previousTime, msec, framenum, realClientTime );
		}
		// so that gui videos play correct. Still need to test to see how the gui videos work during g_stopTime.
		if ( g_stopTimeForceRenderViewUpdateDuring.GetBool() ) {
			if ( player ) {
				// update the renderview so that any gui videos play from the right frame
				view = player->GetRenderView();
				if ( view ) {
					gameRenderWorld->SetRenderView( view );
				}

				// BOYETTE ADDED 03 29 15 BEGIN
				if ( gameLocal.portalSkyEnt.GetEntity() && g_enablePortalSky.GetBool() ) { // BOYETTE NOTE EXPLANATION: Along with SetupPlayerPVS,SortActiveEntityList, and FreePlayerPVS we only need to update a few enetities for the portalSky to update correctly
					// create a merged pvs for all players
					SetupPlayerPVS();
					// sort the active entity list
					SortActiveEntityList();

					/* // BOYETTE NOTE EXPLANATION: Along with SetupPlayerPVS,SortActiveEntityList, and FreePlayerPVS we only need to update a few enetities for the portalSky to update correctly
					num = 0;
					for( ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() ) {
						if ( ent->timeGroup != TIME_GROUP1 ) {
							continue;
						}
						ent->Think();
						num++;
					}
					*/

					gameLocal.world->Think();
					gameLocal.portalSkyEnt.GetEntity()->Think();
					gameLocal.GetLocalPlayer()->Think();
					// free the player pvs
					FreePlayerPVS();
				}
				// BOYETTE ADDED 03 29 15 END
			}
		}
		//BOYETTE SPACE COMMAND END // BOOGLA //
	} else do {
		// update the game time
		framenum++;
		previousTime = time;
		time += msec;
		realClientTime = time;

		//slow.Set( time, previousTime, msec, framenum, realClientTime );

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
					if ( ent->timeGroup != TIME_GROUP1 ) {
						continue;
					}
					ent->Think();
					num++;
				}
			}
		}

		RunTimeGroup2();

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

		// service pending fast events
		//fast.Get( time, previousTime, msec, framenum, realClientTime );
		idEvent::ServiceFastEvents();
		//slow.Get( time, previousTime, msec, framenum, realClientTime );

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

	// first, calculate the vertical fov based on a 1280x960 view
	x = 1280.0f / tan( base_fov / 360.0f * idMath::PI );
	y = atan2( 960.0f, x );
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
idGameLocal::HandleMainMenuCommands
================
*/
void idGameLocal::HandleMainMenuCommands( const char *menuCommand, idUserInterface *gui ) { }

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
		assert( bIsClientReadSnapshot );
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
returning false if not found // boyette - or the string equals "NULL" which will not produce a warning message. - this will allow you to change the dropdeathitem key/value to nothing after zombie death and not recieve a warning message.
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

	// BOYETTE LIST OF ENTITIES ATTEMPTED TO SPAWN BEGIN
	//idFile *file;	
	//file = fileSystem->OpenFileAppend("file_write_test.txt");
	//file->Printf( "Spawned Entity: %s \n", spawnArgs.GetString( "name", "") );
	//fileSystem->CloseFile( file );
	// BOYETTE LIST OF ENTITIES ATTEMPTED TO SPAWN END

	spawnArgs.GetString( "classname", NULL, &classname );

	const idDeclEntityDef *def = FindEntityDef( classname, false );

// boyette modify begin
	const char	*nullname;
	nullname = "0";
	if ( !def ) {
		if ( *classname == *nullname ) {
			return false;
		}
		else {
			Warning( "Unknown classname '%s'%s.", classname, error.c_str() );
			return false;
		}
	}
// boyette modify end

	spawnArgs.SetDefaults( &def->dict );

	if ( !spawnArgs.FindKey( "slowmo" ) ) {
		bool slowmo = true;


		// BOYETTE NOTE BEGIN: there is no fast timeline anymore - all entities are on the slow timeline - this was messing up the sound which we are still adjusting
		//for ( int i = 0; fastEntityList[i]; i++ ) {
			//if ( !idStr::Cmp( classname, fastEntityList[i] ) ) {
			//	slowmo = false;
			//	break;
			//}
		//}
		// BOYETTE NOTE END

		if ( !slowmo ) {
			spawnArgs.SetBool( "slowmo", slowmo );
		}
	}

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
		if ( !result && g_skill.GetInteger() == 3 ) {
			spawnArgs.GetBool( "not_nightmare", "0", result );
		}
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

		// BOYETTE ADDED 10/14/2016 TO SMOOTH LOADING BAR BEGIN
		common->PacifierUpdate();
		// BOYETTE ADDED 10/14/2016 TO SMOOTH LOADING BAR END
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

// boyette note - it looks like this could be used for any entity - not just targets - it seems to be a generalized function to create an index of entities that can be accessed from an entity.
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
/*
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
*/

//BOYETTE NOTE: this might be a much way of doing FindEntity - prevents undefined behavior if name is NULL
idEntity *idGameLocal::FindEntity( const char *name ) const {
	if ( name ) {
		int hash, i;

		hash = entityHash.GenerateKey( name, true );
		for ( i = entityHash.First( hash ); i != -1; i = entityHash.Next( i ) ) {
			if ( entities[i] && entities[i]->name.Icmp( name ) == 0 ) {
				return entities[i];
			}
		}
	} else {
		return NULL;
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
	int timeGroup = 0;
	if ( lastAIAlertTime && lastAIAlertEntity.GetEntity() ) {
		timeGroup = lastAIAlertEntity.GetEntity()->timeGroup;
	}
	SetTimeState ts( timeGroup );

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
	gameRenderWorld->ProjectDecalOntoWorld( winding, projectionOrigin, parallel, depth * 0.5f, declManager->FindMaterial( material ), gameLocal.time /* _D3XP */ );
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
	//int i = 0;

	//if ( timeGroup ) {
		//slow.Get( time, previousTime, msec, framenum, realClientTime );
	//} else {
		//slow.Get( time, previousTime, msec, framenum, realClientTime );
	//}
}

/*
===========
idGameLocal::GetTimeGroupTime
============
*/
int idGameLocal::GetTimeGroupTime( int timeGroup ) {
	if ( timeGroup ) {
		return time;
	} else {
		return time;
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
	else if ( g_enableSlowmo.GetBool() || g_enableCaptainGuiSlowmo.GetBool() ) { // BOYETTE SPACE COMMAND NOTE: this was original just checking for g_enableSlowmo.GetBool() - we added g_enableCaptainGuiSlowmo.GetBool()
		powerupOn = true;
	}

	// determine proper slowmo state
	if ( powerupOn && slowmoState == SLOWMO_STATE_OFF ) {
		slowmoState = SLOWMO_STATE_RAMPUP;

		slowmoMsec = msec;
		if ( gameSoundWorld ) {
			gameSoundWorld->SetSlowmo( true );
			gameSoundWorld->SetSlowmoSpeed( slowmoMsec / (float)USERCMD_MSEC );
		}
	}
	else if ( !powerupOn && slowmoState == SLOWMO_STATE_ON ) {
		slowmoState = SLOWMO_STATE_RAMPDOWN;
		if ( GetLocalPlayer() && GetLocalPlayer()->ShipOnBoard && GetLocalPlayer()->ShipOnBoard->red_alert ) { // BOYETTE NOTE TODO: could maybe come up with a better solution than this - but this works fine.
			gameLocal.GetLocalPlayer()->StopSound(SND_CHANNEL_SHIP_ALARMS,false); // the sounds get unsynchronized during warp - this just prevents skipping - at the cost of missing a beat or two
			GetLocalPlayer()->ShipOnBoard->SynchronizeRedAlertSpecialFX();
		}

		// play the stop sound
		if ( player ) {
			player->PlayHelltimeStopSound();
		}
	}

	// do any necessary ramping
	if ( slowmoState == SLOWMO_STATE_RAMPUP ) {
		delta = 4 - slowmoMsec;

		if ( fabs( delta ) < g_slowmoStepRate.GetFloat() ) {
			slowmoMsec = 4;
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

	// BOYETTE NOTE: these aren't used anymore because we couldn't get slowmo to work consistently with entities in different times - expecially with the captaingui
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




// boyette space command begin

void idGameLocal::UpdateSpaceEntityVisibilities() {
	int			i;
	idEntity	*ent;
	sbShip*		sbShipToUpdate;

	// if there is no ShipOnBoard, we can't know what visibilities to show. So just leave it how it spawns.
	if ( !gameLocal.GetLocalPlayer()->ShipOnBoard ) {
		return;
	}

	int PlayerPosX = gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx;
	int PlayerPosY = gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony;

	for ( i = 0; i < gameLocal.num_entities ; i++ ) {
		ent = gameLocal.entities[ i ];
		if ( ent && ( ent->IsType(sbShip::Type) || ent->IsType(sbStationarySpaceEntity::Type) ) ) {
			if ( ent && ent->stargridpositionx == PlayerPosX && ent->stargridpositiony == PlayerPosY ) {
				sbShipToUpdate = dynamic_cast<sbShip*>( ent );
				sbShipToUpdate->Show();
				if ( sbShipToUpdate->GetPhysics() ) {
					sbShipToUpdate->GetPhysics()->SetContents( CONTENTS_SOLID ); // set solid
				}
				//gameLocal.Printf( "\n" );
				//gameLocal.Printf( "We are showing: " + ent->name + "\n" );
				sbShipToUpdate->UpdateVisuals();
				sbShipToUpdate->RunPhysics();
				sbShipToUpdate->Present();
				// Update whether the shield is hidden or not.
				if ( sbShipToUpdate && sbShipToUpdate->ShieldEntity && sbShipToUpdate->shieldStrength > 0 && sbShipToUpdate->ShieldEntity->IsHidden() ) {
					sbShipToUpdate->ShieldEntity->Show();
					if ( sbShipToUpdate->ShieldEntity->GetPhysics() ) {
						sbShipToUpdate->ShieldEntity->GetPhysics()->SetContents( CONTENTS_SOLID ); // set solid
					}
				}
				if ( sbShipToUpdate->should_warp_in_when_first_encountered && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->ShipOnBoard != sbShipToUpdate && !sbShipToUpdate->is_derelict ) {
					sbShipToUpdate->SetupWarpInVisualEffects( 5000 );
				}
			} else {
				sbShipToUpdate = dynamic_cast<sbShip*>( ent );
				sbShipToUpdate->Hide();
				if ( sbShipToUpdate->GetPhysics() ) {
					sbShipToUpdate->GetPhysics()->SetContents( 0 ); // set non-solid
				}
				sbShipToUpdate->UpdateVisuals();
				sbShipToUpdate->Present();
				// Hide the shield entity as well.
				if ( sbShipToUpdate && sbShipToUpdate->ShieldEntity && !sbShipToUpdate->ShieldEntity->IsHidden() ) {
					sbShipToUpdate->ShieldEntity->Hide();
					if ( sbShipToUpdate->ShieldEntity->GetPhysics() ) {
						sbShipToUpdate->ShieldEntity->GetPhysics()->SetContents( 0 ); // set non-solid
					}
					sbShipToUpdate->ShieldEntity->SetShaderParm(10,0.0f);
				}
				if ( sbShipToUpdate && sbShipToUpdate->projectile.GetEntity() ) {
					sbShipToUpdate->projectile.GetEntity()->Event_Remove(); // BOYETTE NOTE TODO: I think this will work ok. if the ship have a projectile fired at us when we are warping away
					sbShipToUpdate->projectile = NULL;
				}
			}
		}
	}

	if ( gameLocal.GetLocalPlayer()->ShipOnBoard ) {
		// we might want to do something here the shield entity visibility as well.
		gameLocal.GetLocalPlayer()->ShipOnBoard->should_warp_in_when_first_encountered = false;
		gameLocal.GetLocalPlayer()->ShipOnBoard->Hide(); // we don't want the ShipOnBoard getting in the way of the currently active skyportal.
		gameLocal.GetLocalPlayer()->ShipOnBoard->UpdateVisuals();
		gameLocal.GetLocalPlayer()->ShipOnBoard->RunPhysics();
		gameLocal.GetLocalPlayer()->ShipOnBoard->Present();
		//if ( gameLocal.GetLocalPlayer()->ShipOnBoard->GetPhysics() ) {
			// BOYETTE NOTE TODO: not sure if this is necessary. We don't want projectiles passing through the ship on board if its shields are down. TESTED - seems to not be necessary but we will see in the future.
			//gameLocal.GetLocalPlayer()->ShipOnBoard->GetPhysics()->SetContents( 0 ); // set non-solid
		//}
	}
	if ( gameLocal.GetLocalPlayer()->PlayerShip ) {
		gameLocal.GetLocalPlayer()->PlayerShip->should_warp_in_when_first_encountered = false;
	}
}

bool idGameLocal::CheckAllNonPlayerSkyPortalsForOccupancy() {
	int			i;
	idEntity	*ent;
	idPortalSky* PortalSkyToCheck;

	// if there is no ShipOnBoard, we can't know what visibilities to show. So just leave it how it spawns.
	if ( !gameLocal.GetLocalPlayer()->ShipOnBoard ) {
		return false;
	}

	int PlayerPosX = gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx;
	int PlayerPosY = gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony;

	for ( i = 0; i < gameLocal.num_entities ; i++ ) {
		ent = gameLocal.entities[ i ];
		if ( ent && ent->IsType( idPortalSky::Type )  ) {
			PortalSkyToCheck = dynamic_cast<idPortalSky*>( ent );
			if ( PortalSkyToCheck->occupied == false && !(idStr::FindText(ent->name.c_str(),"player",false)+1) ) {
				return true;
			}
		}
	}
	return false;
}

void idGameLocal::SetAllSkyPortalsOccupancyStatusToFalse() {
	int			i;
	idEntity	*ent;
	idPortalSky* PortalSkyToCheck;

	for ( i = 0; i < gameLocal.num_entities ; i++ ) {
		ent = gameLocal.entities[ i ];
		if ( ent && ent->IsType( idPortalSky::Type )  ) {
			PortalSkyToCheck = dynamic_cast<idPortalSky*>( ent );
			//if ( !idStr::FindText("player",PortalSkyToCheck->name,false,0,-1) ) {
				PortalSkyToCheck->occupied = false;
			//}
		}
	}
}

void idGameLocal::SetAllShipSkyPortalEntitiesToNull() {
	int			i;
	idEntity	*ent;

	sbShip*			sbShipToCheck;

	for ( i = 0; i < gameLocal.num_entities ; i++ ) {
		ent = gameLocal.entities[ i ];
		if ( ent && ent->IsType( sbShip::Type ) && !ent->IsType(sbStationarySpaceEntity::Type) ) {
			sbShipToCheck = dynamic_cast<sbShip*>( ent );
			sbShipToCheck->MySkyPortalEnt = NULL;
		}
	}
}


// BOYETTE UTILITY FUNCTIONS BEGIN:
bool Utility_Function_CompareEntityNamesForward (idEntity* i,idEntity* j) {
	if ( i && j ) {
		return ( i->name.c_str() < j->name.c_str() );
	} else {
		return true;
	}
}
bool Utility_Function_CompareEntityNamesReverse (idEntity* i,idEntity* j) {
	if ( i && j ) {
		return ( i->name.c_str() > j->name.c_str() );
	} else {
		return true;
	}
}
// BOYETTE UTILITY FUNCTIONS END
void idGameLocal::AssignAllShipsAtCurrentLocToASkyPortalEntity() {
	idEntity*		ent = NULL;
	sbShip*			sbShipToCheck = NULL;
	sbShip*			sbPlayerShipToCheck = NULL;
	idPortalSky*	PortalSkyToCheck = NULL;

	// if there is no ShipOnBoard, we can't know what visibilities to show. So just leave it how it spawns.
	if ( !gameLocal.GetLocalPlayer() || !gameLocal.GetLocalPlayer()->ShipOnBoard ) {
		return;
	}

	int PlayerPosX = gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx;
	int PlayerPosY = gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony;
	if ( gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->PlayerShip ) {
		sbPlayerShipToCheck = gameLocal.GetLocalPlayer()->PlayerShip;
	} else {
		sbPlayerShipToCheck = NULL;
	}

	idPortalSky*				PlayerSkyPortal = NULL;
	std::vector<idPortalSky*>	NonPlayerSkyPortals;
	std::vector<sbShip*>		NonPlayerShipsAtPlayerStargridPosition;

	for ( int i = 0; i < gameLocal.num_entities ; i++ ) {
		ent = gameLocal.entities[ i ];
		if ( ent && ent->IsType( sbShip::Type ) && ent->stargridpositionx == PlayerPosX && ent->stargridpositiony == PlayerPosY ) {
			sbShipToCheck = dynamic_cast<sbShip*>( ent );

			if ( sbShipToCheck->alway_snap_to_my_sky_portal_entity ) {
				sbShipToCheck->MySkyPortalEnt = dynamic_cast<idPortalSky*>( gameLocal.FindEntity( sbShipToCheck->spawnArgs.GetString( "my_sky_portal_entity" ) ) );
				if ( sbShipToCheck->MySkyPortalEnt && !sbShipToCheck->MySkyPortalEnt->occupied ) {//&& !(idStr::FindText(sbShipToCheck->MySkyPortalEnt->name.c_str(),"player",false)+1) && !(idStr::FindText(sbShipToCheck->MySkyPortalEnt->name.c_str(),"ship",false)+1) ) {
					sbShipToCheck->ClaimSpecifiedSkyPortalEntity(sbShipToCheck->MySkyPortalEnt);
				} else {
					gameLocal.Warning( sbShipToCheck->name + " was set to alway_snap_to_my_sky_portal_entity but my_sky_portal_entity is not available." );
				}
			} else if ( !sbShipToCheck->IsType(sbStationarySpaceEntity::Type) ) {
				if ( sbShipToCheck != sbPlayerShipToCheck ) { // BOYETTE NOTE: we exclude the playership because we will do it last.
					NonPlayerShipsAtPlayerStargridPosition.push_back(sbShipToCheck);
				}
			}
		} else if ( ent && ent->IsType( idPortalSky::Type )  ) {
			PortalSkyToCheck = dynamic_cast<idPortalSky*>( ent );
			if ( PortalSkyToCheck->occupied == false && !(idStr::FindText(PortalSkyToCheck->name.c_str(),"player",false)+1) && (idStr::FindText(PortalSkyToCheck->name.c_str(),"ship",false)+1) ) {
				NonPlayerSkyPortals.push_back(PortalSkyToCheck);
			} else if ( (idStr::FindText(PortalSkyToCheck->name.c_str(),"player",false)+1) ) {
				PlayerSkyPortal = PortalSkyToCheck;
			}
		}
	}
	std::sort (NonPlayerSkyPortals.begin(), NonPlayerSkyPortals.end(), Utility_Function_CompareEntityNamesForward); // BOYETTE NOTE: sorts the non-player sky portals by name e.g. info_portalsky_ship_1, info_portalsky_ship_2, info_portalsky_ship_3, etc. - make sure they are named appropriately for the sorting to work the way we want.
	/*
	gameLocal.Printf( "\nSky Portals List Begin:\n" );
	for ( int i = 0; i < NonPlayerSkyPortals.size(); i++ ) {
		gameLocal.Printf( NonPlayerSkyPortals[i]->name + "\n" );
	}
	gameLocal.Printf( "Sky Portals List END\n" );
	gameLocal.Printf( "\nShips List Begin:\n" );
	for ( int i = 0; i < NonPlayerShipsAtPlayerStargridPosition.size(); i++ ) {
		gameLocal.Printf( NonPlayerShipsAtPlayerStargridPosition[i]->name + "\n" );
	}
	gameLocal.Printf( "nShips List END\n" );
	*/
	for ( int i = 0; i < NonPlayerShipsAtPlayerStargridPosition.size(); i++ ) {
		idPortalSky* random_sky_portal = NULL;
		int attempts_to_be_random = NonPlayerSkyPortals.size() * 2;
		bool random_sky_portal_found = false;
		for ( int x = 0; x < attempts_to_be_random; x++ ) { // BOYETTE NOTE: If we don't want the skyportal for each ship to be random than comment out this for loop
			int random_portal_int = gameLocal.random.RandomInt(NonPlayerSkyPortals.size());
			if ( random_portal_int <= gameLocal.random.RandomInt(NonPlayerSkyPortals.size()) ) { // The higher the sky portal number goes, the less likely that it will be acceptable.
				random_sky_portal = NonPlayerSkyPortals[random_portal_int]; // might need to be -1 here on the size() - NOPE - this works fine how it is - gameLocal.random.RandomInt(8) will be a number from 0 to 7 - It will NOT ever be 8.
				if ( random_sky_portal && !random_sky_portal->occupied ) {
					NonPlayerShipsAtPlayerStargridPosition[i]->ClaimSpecifiedSkyPortalEntity(random_sky_portal);
					random_sky_portal_found = true;
					break;
				}
			}
		}
		if ( !random_sky_portal_found ) { // BOYETTE NOTE: If we don't want the skyportal for each ship to be random than only use this for loop
			for ( int x = 0; x < NonPlayerSkyPortals.size(); x++ ) {
				if ( NonPlayerSkyPortals[x] && !NonPlayerSkyPortals[x]->occupied ) {
					NonPlayerShipsAtPlayerStargridPosition[i]->ClaimSpecifiedSkyPortalEntity(NonPlayerSkyPortals[x]);
					break;
				}
			}
		}
	}
	if ( sbPlayerShipToCheck && sbPlayerShipToCheck->stargridpositionx == PlayerPosX && sbPlayerShipToCheck->stargridpositiony == PlayerPosY && PlayerSkyPortal ) { // BOYETTE NOTE: we do the playership here last.
		sbPlayerShipToCheck->ClaimSpecifiedSkyPortalEntity(PlayerSkyPortal);
	}
}
/* // BOYETTE NOTE: Old Version
void idGameLocal::AssignAllShipsAtCurrentLocToASkyPortalEntity() {
	int			i;
	idEntity	*ent;
// going to want to add this: if ( gameLocal.GetLocalPlayer()->PlayerShip = ent ) { ClaimUnnoccupiedPlayerSkyPortalEntity(); continue; }
	sbShip*			sbShipToCheck;
	sbShip*			sbPlayerShipToCheck;

	// if there is no ShipOnBoard, we can't know what visibilities to show. So just leave it how it spawns.
	if ( !gameLocal.GetLocalPlayer()->ShipOnBoard ) {
		return;
	}

	int PlayerPosX = gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx;
	int PlayerPosY = gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony;
	sbPlayerShipToCheck = gameLocal.GetLocalPlayer()->PlayerShip;

	for ( i = 0; i < gameLocal.num_entities ; i++ ) {
		ent = gameLocal.entities[ i ];
		if ( ent && ent->IsType( sbShip::Type ) && !ent->IsType(sbStationarySpaceEntity::Type) && ent->stargridpositionx == PlayerPosX && ent->stargridpositiony == PlayerPosY ) {
			sbShipToCheck = dynamic_cast<sbShip*>( ent );
			if ( sbPlayerShipToCheck == sbShipToCheck ) {
				sbShipToCheck->ClaimUnnoccupiedPlayerSkyPortalEntity();
				continue;
			}
			if ( gameLocal.GetLocalPlayer()->ShipOnBoard != sbShipToCheck ) {
				sbShipToCheck->ClaimUnnoccupiedSkyPortalEntity();
			}
		}
		// instead of relying on the seperate sbPlayerShip class - once we make the playershipbegin spawnarg and get that set up, we can change this to: if ( gameLocal.GetLocalPlayer()->PlayerShip = ent ) { ClaimUnnoccupiedPlayerSkyPortalEntity(); continue; }
	}
}
*/
int idGameLocal::GetNumberOfNonPlayerShipSkyPortalEntities() { // BOYETTE NOTE TODO: this only needs to be run once on map start.
	int			i;
	int			NumberOfEnts;
	idEntity	*ent;

	// if there is no ShipOnBoard, we can't know what visibilities to show. So just leave it how it spawns.
	if ( !gameLocal.GetLocalPlayer()->ShipOnBoard ) {
		return 0;
	}

	NumberOfEnts = 0;

	for ( i = 0; i < gameLocal.num_entities ; i++ ) {
		ent = gameLocal.entities[ i ];
		if ( ent && ent->IsType( idPortalSky::Type )  ) {
			if ( (idStr::FindText(ent->name.c_str(),"ship",false)+1) && !(idStr::FindText(ent->name.c_str(),"player",false)+1) ) {
				NumberOfEnts++;
			}
		}
	}
	return NumberOfEnts;
}

void idGameLocal::GetSpaceCommandDynamicLights() {
	idEntity	*ent;

	int counter = 0;

	// clear them first
	for ( int i = 0; i < MAX_SPACE_COMMAND_DYNAMIC_LIGHTS ; i++ ) {
		space_command_dynamic_lights[i] = NULL; //dynamic_cast<idLight*>( ent );
	}

	for ( int i = 0; i < gameLocal.num_entities && counter < MAX_SPACE_COMMAND_DYNAMIC_LIGHTS ; i++ ) {
		ent = gameLocal.entities[ i ];
		if ( ent && ent->IsType( idLight::Type )  ) {
			if ( idStr::FindText(ent->name.c_str(),"space_command_dynamic_light_",false) + 1 ) {
				space_command_dynamic_lights[counter] = ent; //dynamic_cast<idLight*>( ent );
				counter++;
			}
		}
	}
}
void idGameLocal::UpdateSpaceCommandDynamicLights() {
	if ( StoryRepositoryEntity && GetLocalPlayer() && GetLocalPlayer()->ShipOnBoard ) {
		// SET THE DEFAULT VALUES:
		idVec4 specified_light_color = StoryRepositoryEntity->spawnArgs.GetVec4( va("space_command_dynamic_light_color_x%i_y%i",GetLocalPlayer()->ShipOnBoard->stargridpositionx, GetLocalPlayer()->ShipOnBoard->stargridpositiony), "1 1 1 1" );
		const char* specified_light_shader = StoryRepositoryEntity->spawnArgs.GetString( va("space_command_dynamic_light_shader_x%i_y%i",GetLocalPlayer()->ShipOnBoard->stargridpositionx, GetLocalPlayer()->ShipOnBoard->stargridpositiony), "lights/squarelight1" );
		// CHECK FOR A LIGHT COLOR SPECIFIED ON ANY ENTITY IN LOCAL SPACE
		if ( specified_light_color == idVec4(1,1,1,1) ) {
			for ( int i = 0; i < GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position.size(); i++ ) {
				if ( GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position[i] ) {
					idVec4 color_specified_by_entity = GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position[i]->spawnArgs.GetVec4( "space_command_dynamic_light_color_to_display" , "1 1 1 1" );
					if ( color_specified_by_entity != idVec4(1,1,1,1) ) {
						specified_light_color = color_specified_by_entity;
						break;
					}
				}
			}
		}
		// CHECK FOR A LIGHT SHADER SPECIFIED ON ANY ENTITY IN LOCAL SPACE
		if ( idStr::Cmp(specified_light_shader, "lights/squarelight1") == 0 ) {
			for ( int i = 0; i < GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position.size(); i++ ) {
				if ( GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position[i] ) {
					const char* shader_specified_by_entity = GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position[i]->spawnArgs.GetString( "space_command_dynamic_light_shader_to_display" , "lights/squarelight1" );
					if ( idStr::Cmp(shader_specified_by_entity, "lights/squarelight1") != 0 ) {
						specified_light_shader = shader_specified_by_entity;
						break;
					}
				}
			}
		}
		// APPLY THEM TO THE LIGHTS
		for ( int i = 0; i < MAX_SPACE_COMMAND_DYNAMIC_LIGHTS ; i++ ) {
			if ( space_command_dynamic_lights[i] && space_command_dynamic_lights[i]->IsType( idLight::Type ) ) {
				if ( StoryRepositoryEntity && GetLocalPlayer() && GetLocalPlayer()->ShipOnBoard ) {
					dynamic_cast<idLight*>( space_command_dynamic_lights[i] )->SetColor( specified_light_color );
					if ( idStr::Cmp(specified_light_shader, "lights/squarelight1") == 0 ) {
						dynamic_cast<idLight*>( space_command_dynamic_lights[i] )->SetShader( space_command_dynamic_lights[i]->spawnArgs.GetString( "texture", "lights/squarelight1") );
					} else {
						dynamic_cast<idLight*>( space_command_dynamic_lights[i] )->SetShader( specified_light_shader );
					}
				}
			}
		}

	}

	/*
	for ( int i = 0; i < MAX_SPACE_COMMAND_DYNAMIC_LIGHTS ; i++ ) {
		if ( space_command_dynamic_lights[i] && space_command_dynamic_lights[i]->IsType( idLight::Type ) ) {
			if ( StoryRepositoryEntity && GetLocalPlayer() && GetLocalPlayer()->ShipOnBoard ) {
				//idVec4 light_color;
				//light_color = StoryRepositoryEntity->spawnArgs.GetVec4( va("space_command_dynamic_light_color_x%i_y%i",GetLocalPlayer()->ShipOnBoard->stargridpositionx, GetLocalPlayer()->ShipOnBoard->stargridpositiony), "1 1 1 1" );
				//dynamic_cast<idLight*>( space_command_dynamic_lights[i] )->SetLightParms(light_color.w,light_color.x,light_color.y,light_color.z); // red green blue alpha
				dynamic_cast<idLight*>( space_command_dynamic_lights[i] )->SetColor( StoryRepositoryEntity->spawnArgs.GetVec4( va("space_command_dynamic_light_color_x%i_y%i",GetLocalPlayer()->ShipOnBoard->stargridpositionx, GetLocalPlayer()->ShipOnBoard->stargridpositiony), "1 1 1 1" ) );
				dynamic_cast<idLight*>( space_command_dynamic_lights[i] )->SetShader( StoryRepositoryEntity->spawnArgs.GetString( va("space_command_dynamic_light_shader_x%i_y%i",GetLocalPlayer()->ShipOnBoard->stargridpositionx, GetLocalPlayer()->ShipOnBoard->stargridpositiony), space_command_dynamic_lights[i]->spawnArgs.GetString( "texture", "lights/squarelight1") ) );
				//Printf( space_command_dynamic_lights[i]->name + "had its color set to " + light_color.ToString() + "\n" );
			}
		}
	}
	*/
}
void idGameLocal::UpdateSpaceCommandViewscreenCamera() {
	if ( SpaceCommandViewscreenCamera && GetLocalPlayer() && GetLocalPlayer()->ShipOnBoard && GetLocalPlayer()->ShipOnBoard->ViewScreenEntity && GetLocalPlayer()->ShipOnBoard->GetPhysics() && SpaceCommandViewscreenCamera->IsType(idSecurityCamera::Type) ) {
		idSecurityCamera* camera = static_cast<idSecurityCamera*>(SpaceCommandViewscreenCamera);
		if ( GetLocalPlayer()->PlayerShip && GetLocalPlayer()->ShipOnBoard == GetLocalPlayer()->PlayerShip ) {

			camera->SetOrigin(GetLocalPlayer()->PlayerShip->GetPhysics()->GetOrigin() + idVec3(0,0,8));

			if ( GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position.size() > 0 ) {

				// find the average location of all the ships at the stargrid position
				float average_x = 0.0;
				float average_y = 0.0;
				float average_z = 0.0;
				for ( int x = 0; x < GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position.size(); x++ ) {
					average_x = average_x + GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position[x]->GetPhysics()->GetOrigin().x;
					average_y = average_y + GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position[x]->GetPhysics()->GetOrigin().y;
					average_z = average_z + GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position[x]->GetPhysics()->GetOrigin().z;
				}
				average_x = average_x / GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position.size();
				average_y = average_y / GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position.size();
				average_z = average_z / GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position.size();
				//original//camera->GetPhysics()->SetAxis((idVec3(average_x,average_y,average_z) - camera->GetPhysics()->GetOrigin()).ToAngles().ToForward().ToMat3());
				camera->GetPhysics()->SetAxis((idVec3(average_x,average_y,average_z) - camera->GetPhysics()->GetOrigin()).ToAngles().ToForward().ToMat3());
				//Printf( "The camera target is: X:" + idStr(idVec3(average_x,average_y,average_z).x) + " Y:" + idStr(idVec3(average_x,average_y,average_z).y) + " Z:" + idStr(idVec3(average_x,average_y,average_z).z) + "\n" );
				//Printf( "The camera location is: X:" + idStr(camera->GetPhysics()->GetOrigin().x) + " Y:" + idStr(camera->GetPhysics()->GetOrigin().y) + " Z:" + idStr(camera->GetPhysics()->GetOrigin().z) + "\n" );
				idAngles myangle = (idVec3(average_x,average_y,average_z) - camera->GetPhysics()->GetOrigin()).ToAngles().ToForward().ToMat3().ToAngles();
				//Printf( "The camera angle is: Roll:" + idStr(myangle.roll) + " Pitch:" + idStr(myangle.pitch) + " Yaw:" + idStr(myangle.yaw) + "\n" );

				camera->scanFov = 10;
				for ( int i = 0; i < 10; i++ ) {	

					camera->scanFov = camera->scanFov + 10.0;

					bool suitable_fov_found = true;
					for ( int x = 0; x < GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position.size(); x++ ) {
						if ( GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position[x] ) {
							if ( GetLocalPlayer()->ShipOnBoard->TargetEntityInSpace ) {
								if ( camera->CanSeeEntity(GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position[x]) == false ) {
									suitable_fov_found = false;
									break;
								}
							} else {
								if ( camera->CanSeeAllEntityBounds(GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position[x]) == false ) {
									suitable_fov_found = false;
									break;
								}
							}
						}
					}

					if ( suitable_fov_found ) {
						if ( GetLocalPlayer()->ShipOnBoard->TargetEntityInSpace ) {
							if ( camera->CanSeeAllEntityBounds(GetLocalPlayer()->ShipOnBoard->TargetEntityInSpace) ) { // just to make sure we can completely see the target entity
								//Printf( "A SUITABLE FOV WAS FOUND\n" ); 
								return;
							}
						} else {
							//Printf( "A SUITABLE FOV WAS FOUND\n" ); 
							//camera->scanFov = camera->scanFov - 20.0; // subtract an extra 20.0 just to make the ships seem a little bigger
							return;
						}
					}
				}
			} else {
				//Printf( "GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position.size() <= 0\n" ); 
				camera->GetPhysics()->SetAxis(GetLocalPlayer()->ShipOnBoard->GetPhysics()->GetAxis() * idMat3(0,1,0,-1,0,0,0,0,1)); // idMat3(0,1,0,-1,0,0,0,0,1)) is 90 degrees to the left - the camera uses a weird system for it orientation it seems
				camera->scanFov = 70.0; // 120.0;
			}
		} else if ( !GetLocalPlayer()->PlayerShip || ( GetLocalPlayer()->PlayerShip && GetLocalPlayer()->ShipOnBoard != GetLocalPlayer()->PlayerShip ) ) {

			camera->SetOrigin(GetLocalPlayer()->ShipOnBoard->GetPhysics()->GetOrigin() + idVec3(0,0,8));

			//if we are not aboard the playership - or there isn't one - zoom in as far as possible while keeping all the ships in view.
			if ( GetLocalPlayer()->ShipOnBoard->ship_that_just_fired_at_us && GetLocalPlayer()->ShipOnBoard->ship_that_just_fired_at_us->GetPhysics() ) {
				camera->GetPhysics()->SetAxis((GetLocalPlayer()->ShipOnBoard->ship_that_just_fired_at_us->GetPhysics()->GetOrigin() - camera->GetPhysics()->GetOrigin()).ToAngles().ToForward().ToMat3());
				camera->scanFov = 0;
				for ( int i = 0; i < 8; i++ ) {	
					camera->scanFov = camera->scanFov + 15.0;
					if ( camera->CanSeeAllEntityBounds(GetLocalPlayer()->ShipOnBoard->ship_that_just_fired_at_us) ) {
						GetLocalPlayer()->ShipOnBoard->ship_that_just_fired_at_us = NULL;
						// an event is scheduled on the firing ship(when it fires) to update the viewscreen again after a second or two
						break;
					}
				}
				GetLocalPlayer()->ShipOnBoard->ship_that_just_fired_at_us = NULL;
				// an event is scheduled on the firing ship(when it fires) to update the viewscreen again after a second or two
			} else if ( GetLocalPlayer()->ShipOnBoard->TargetEntityInSpace && GetLocalPlayer()->ShipOnBoard->TargetEntityInSpace->GetPhysics() ) {
				camera->GetPhysics()->SetAxis((GetLocalPlayer()->ShipOnBoard->TargetEntityInSpace->GetPhysics()->GetOrigin() - camera->GetPhysics()->GetOrigin()).ToAngles().ToForward().ToMat3());
				camera->scanFov = 0;
				for ( int i = 0; i < 8; i++ ) {	
					camera->scanFov = camera->scanFov + 15.0;
					if ( camera->CanSeeAllEntityBounds(GetLocalPlayer()->ShipOnBoard->TargetEntityInSpace) ) {
						break;
					}
				}
			} else if ( GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position.size() > 0 ) { // otherwise just pick the first ship that exists at out stargrid position
				for ( int x = 0; x < GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position.size(); x++ ) {
					if ( GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position[x] && GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position[x]->GetPhysics() ) {
						camera->GetPhysics()->SetAxis((GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position[x]->GetPhysics()->GetOrigin() - camera->GetPhysics()->GetOrigin()).ToAngles().ToForward().ToMat3());
					}
				}
			} else {
				camera->GetPhysics()->SetAxis(GetLocalPlayer()->ShipOnBoard->GetPhysics()->GetAxis() * idMat3(0,1,0,-1,0,0,0,0,1)); // idMat3(0,1,0,-1,0,0,0,0,1)) is 90 degrees to the left - the camera uses a weird system for it orientation it seems
			}
		}
	}
}

/*
=================
idGameLocal::KillBox

Finds a somewhat random position for the entity within the target bounds that is not inside other clip models.
=================
*/
idVec3 idGameLocal::GetSuitableTransporterPositionWithinBounds( idEntity* ent, const idBounds* target_bounds ) {
	bool		suitable_pos_found = true;
	int			num;
	idEntity *	hit;
	idClipModel *cm;
	idClipModel *clipModels[ MAX_GENTITIES ];
	idPhysics	*phys;

	if ( !ent || !target_bounds ) {
		return vec3_origin;
	}

	idVec3 suitable_pos = target_bounds->GetCenter();
	suitable_pos.z = (*target_bounds)[0][2]; // we don't want the AI to be up in the air.

	phys = ent->GetPhysics();
	if ( !phys->GetNumClipModels() ) {
		return suitable_pos;
	}

	for ( int ix = 0; ix < 17; ix++ ) {
		suitable_pos_found = true;
		idVec3 random_position_in_bounds;

			//left//1//suitable_torpedo_launchpoint_offset.Set(GetPhysics()->GetBounds()[0][0],0,0);
			//right//2//suitable_torpedo_launchpoint_offset.Set(GetPhysics()->GetBounds()[1][0],0,0);
			//down//3//suitable_torpedo_launchpoint_offset.Set(0,0,GetPhysics()->GetBounds()[0][2]);
			//up//4//suitable_torpedo_launchpoint_offset.Set(0,0,GetPhysics()->GetBounds()[1][2]);
			//back//5//suitable_torpedo_launchpoint_offset.Set(0,GetPhysics()->GetBounds()[0][1],0);
			//front//6//suitable_torpedo_launchpoint_offset.Set(0,GetPhysics()->GetBounds()[1][1],0);

			//target_bounds[0][0]
			//right//2//suitable_torpedo_launchpoint_offset.Set(GetPhysics()->GetBounds()[1][0],0,0);
			//down//3//suitable_torpedo_launchpoint_offset.Set(0,0,GetPhysics()->GetBounds()[0][2]);
			//up//4//suitable_torpedo_launchpoint_offset.Set(0,0,GetPhysics()->GetBounds()[1][2]);
			//back//5//suitable_torpedo_launchpoint_offset.Set(0,GetPhysics()->GetBounds()[0][1],0);
			//front//6//suitable_torpedo_launchpoint_offset.Set(0,GetPhysics()->GetBounds()[1][1],0);
		if ( ix > 0 ) { // on the first loop we will try the entity's current location.
			random_position_in_bounds.x = target_bounds->GetCenter().x + ( ( idMath::Fabs(( idMath::Fabs((*target_bounds)[0][0]) - idMath::Fabs((*target_bounds)[1][0]))) / 2.0f ) * random.CRandomFloat());
			random_position_in_bounds.y = target_bounds->GetCenter().y + ( ( idMath::Fabs(( idMath::Fabs((*target_bounds)[0][1]) - idMath::Fabs((*target_bounds)[1][1]))) / 2.0f ) * random.CRandomFloat());
			random_position_in_bounds.z = (*target_bounds)[0][2];

			gameLocal.Printf( "random_position_in_bounds.x = %f \n", random_position_in_bounds.x );
			gameLocal.Printf( "random_position_in_bounds.y = %f \n", random_position_in_bounds.y );
			gameLocal.Printf( "random_position_in_bounds.z = %f \n", random_position_in_bounds.z );

			ent->SetOrigin(random_position_in_bounds);
		}

		num = clip.ClipModelsTouchingBounds( phys->GetAbsBounds(), phys->GetClipMask(), clipModels, MAX_GENTITIES );
		//if ( num == 0 ) {
		//	return suitable_pos;
		//}
		for ( int i = 0; i < num; i++ ) {
			cm = clipModels[ i ];

			// don't check render entities
			if ( cm->IsRenderModel() ) {
				continue;
			}

			hit = cm->GetEntity();
			if ( ( hit == ent ) ) {
				continue;
			}

			if ( !phys->ClipContents( cm ) ) {
				continue;
			}

			if ( hit->IsType( idPlayer::Type ) || hit->IsType( idAI::Type ) ) {
				suitable_pos_found = false;
				break;
			}

			// nail it
			/*
			if ( hit->IsType( idPlayer::Type ) && static_cast< idPlayer * >( hit )->IsInTeleport() ) {
				static_cast< idPlayer * >( hit )->TeleportDeath( ent->entityNumber );
			}
			*/
			/*
			} else if ( !catch_teleport ) {
				hit->Damage( ent, ent, vec3_origin, "damage_telefrag", 1.0f, INVALID_JOINT );
			}
			*/
			/*
			if ( !gameLocal.isMultiplayer ) {
				// let the mapper know about it
				Warning( "'%s' telefragged '%s'", ent->name.c_str(), hit->name.c_str() );
			}
			*/
		}
		if ( suitable_pos_found ) {
			suitable_pos = random_position_in_bounds;
			break;
		}
	}

	if ( !suitable_pos_found ) {
		suitable_pos = target_bounds->GetCenter();
		suitable_pos.z = (*target_bounds)[0][2]; // we don't want the AI to be up in the air.
	}

	return suitable_pos;
}

int idGameLocal::GetNumberOfNonPlayerShipsAtStarGridPosition(int sgdestx,int sgdesty) {
	int			i;
	int			NumberOfEnts;
	idEntity	*ent;

	NumberOfEnts = 0;

	sbShip*			sbShipToCheck;
	sbPlayerShip*	sbPlayerShipToCheck;
	sbEnemyShip*	sbEnemyShipToCheck;
	sbFriendlyShip*	sbFriendlyShipToCheck;

	for ( i = 0; i < gameLocal.num_entities ; i++ ) {
		ent = gameLocal.entities[ i ];
		if ( ent && ent->IsType( sbShip::Type ) && !ent->IsType(sbStationarySpaceEntity::Type) && gameLocal.GetLocalPlayer() != ent ) {
			if ( ent && ent->stargridpositionx == sgdestx && ent->stargridpositiony == sgdesty ) {
				NumberOfEnts++;
			}
		}
	}
	return NumberOfEnts;
}

void idGameLocal::MaybeDisplayStarGridStoryWindow() {
	if ( !gameLocal.GetLocalPlayer() || !gameLocal.GetLocalPlayer()->ShipOnBoard ) {
		return;
	}

	bool story_window_displayed = false;

	idEntity	*ent;
	if ( StoryRepositoryEntity ) {
		ent = StoryRepositoryEntity;
	} else {
		ent = FindEntityUsingDef( NULL, "stargrid_story_windows_repository" );
	}

	/*
	if ( ent) {
		if ( ent->spawnArgs.GetString("story_window_x1_y1") ) {
			GetLocalPlayer()->SetOverlayGui( ent->spawnArgs.GetString("story_window_x1_y1") );
		}
	}
	*/
	int PlayerPosX = gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositionx;
	int PlayerPosY = gameLocal.GetLocalPlayer()->ShipOnBoard->stargridpositiony;

	if ( ent ) {
		bool something_still_wants_to_display_a_story_window = false;
		for ( int i = 0; i < GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position.size(); i++ ) {
			if ( GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position[i] && GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position[i]->spawnArgs.GetString( "story_window_to_display" , NULL ) != NULL && !GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position[i]->story_window_satisfied ) {
				something_still_wants_to_display_a_story_window = true;
				break;
			}
		}

		if ( GetLocalPlayer()->ShipOnBoard && ( !stargridPositionsVisitedByPlayer[PlayerPosX][PlayerPosY] || something_still_wants_to_display_a_story_window ) ) {
			stargridPositionsVisitedByPlayer[PlayerPosX][PlayerPosY] = true;

			const char* specified_story_window = ent->spawnArgs.GetString( va("story_window_x%i_y%i",PlayerPosX ,PlayerPosY), NULL );

			if ( specified_story_window == NULL ) {
				for ( int i = 0; i < GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position.size(); i++ ) {
					if ( GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position[i] && GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position[i]->spawnArgs.GetString( "story_window_to_display" , NULL ) != NULL ) {
						specified_story_window = GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position[i]->spawnArgs.GetString( "story_window_to_display" , NULL );
						break;
					}
				}
			}

			if ( specified_story_window != NULL ) {
				GetLocalPlayer()->SetOverlayGui( specified_story_window );
				story_window_displayed = true;

				// REMOTE RENDER IMAGE BEGIN // 08/27/15 - I decided we might use the generic viewscreen screenshot on predefined story guis on this date - but it was originally just below(and it is still there - duplicated exactly the same)
				const char* screenshot_filename = va("guis/assets/steve_captain_display/StargridStoryWindowImages/remote_renders/position_x%i_y%i.tga",PlayerPosX ,PlayerPosY);

				if ( SpaceCommandViewscreenCamera ) {
					SpaceCommandViewscreenCamera->TakeRemoteRenderViewScreenshot( screenshot_filename, 640, 480 );
				}
				if ( GetLocalPlayer() && GetLocalPlayer()->guiOverlay ) {
					common->ReloadMaterialImages(screenshot_filename);
					// ORIGINAL BEGIN
					GetLocalPlayer()->guiOverlay->SetStateString("default_story_window_image", screenshot_filename );
					declManager->FindMaterial( "_currentRender" )->SetSort(SS_GUI);
					// ORIGINAL END
				}
				// REMOTE RENDER IMAGE END

				//gameSoundWorld->ClearAllSoundEmitters();
				//gameSoundWorld->Pause
				gameSoundWorld->FadeSoundClasses(0,-100.0f,2); //  -100.0f // all entities have a soundclass of zero unless it is set otherwise. 0.0f db is the default level.
				gameSoundWorld->FadeSoundClasses(3,-100.0f,2); //  -100.0f // ship alarms are soundclass 3. 0.0f db is the default level.
				gameSoundWorld->FadeSoundClasses(1,-10.0f,2); //  -10.0f // the ship hum sound volumes are set to soundclass 1 - we will dim it a bit here. The music shader for the story gui will be set to soundclass 2 I think. We can add more soundclasses if we need them, right now the limit is 4 - look for SOUND_MAX_CLASSES.
				//gameSoundWorld->FadeSoundClasses(2,-100.0f,2);
				//gameSoundWorld->FadeSoundClasses(3,-100.0f,2);
				g_stopTime.SetBool(true);
				g_stopTimeForceFrameNumUpdateDuring.SetBool(true);
				g_stopTimeForceRenderViewUpdateDuring.SetBool(true);
				GetLocalPlayer()->currently_in_story_gui = true;

				g_enableSlowmo.SetBool( false );
				gameLocal.GetLocalPlayer()->blurEnabled = false;
				gameLocal.GetLocalPlayer()->desaturateEnabled = false;
				gameLocal.GetLocalPlayer()->bloomEnabled = false;
				gameLocal.GetLocalPlayer()->playerView.FreeWarp(0);
				gameLocal.GetLocalPlayer()->playerView.Fade(idVec4(0,0,0,0),700);

				gameLocal.GetLocalPlayer()->UpdateGuiOverlay();

			} else {
				if ( !ent->spawnArgs.GetBool( va("suppress_default_story_window_x%i_y%i",PlayerPosX ,PlayerPosY), "1" ) ) {


					// SET THE INITAL VIEWSCREEN POSITION/ANGLE BEGIN
					GetLocalPlayer()->SetOverlayGui( "guis/steve_space_command/stargrid_story_windows/stargrid_story_window_default.gui" );
					story_window_displayed = true;

					// REMOTE RENDER IMAGE BEGIN
					const char* screenshot_filename = va("guis/assets/steve_captain_display/StargridStoryWindowImages/remote_renders/position_x%i_y%i.tga",PlayerPosX ,PlayerPosY);

					if ( SpaceCommandViewscreenCamera ) {
						SpaceCommandViewscreenCamera->TakeRemoteRenderViewScreenshot( screenshot_filename, 640, 480 );
					}
					if ( GetLocalPlayer() && GetLocalPlayer()->guiOverlay ) {
						common->ReloadMaterialImages(screenshot_filename);
						// ORIGINAL BEGIN
						GetLocalPlayer()->guiOverlay->SetStateString("default_story_window_image", screenshot_filename );
						declManager->FindMaterial( screenshot_filename )->SetSort(SS_GUI);
						// ORIGINAL END
					}
					// REMOTE RENDER IMAGE END

					if ( GetLocalPlayer() && GetLocalPlayer()->guiOverlay ) {
						GetLocalPlayer()->guiOverlay->SetStateString("position_text", va("The vessel you are aboard has arrived at sector coordinates: ^5%i,%i^0",PlayerPosX ,PlayerPosY) );
						if ( GetLocalPlayer()->ShipOnBoard ) {
							if ( GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position.size() > 1 ) {
								GetLocalPlayer()->guiOverlay->SetStateString("entities_summary", va("There are ^5%i^0 other entities in local space.",GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position.size()) );
							} else if ( GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position.size() == 1 ) {
								GetLocalPlayer()->guiOverlay->SetStateString("entities_summary", va("There is ^5%i^0 other entity in local space.",GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position.size()) );
							} else if ( GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position.size() < 1 ) {
								GetLocalPlayer()->guiOverlay->SetStateString("entities_summary", "There are no other entities in local space." );
							}

							idStr friendly_text = "Friendly Entities: ";
							idStr neutral_text = "Neutral Entities: ";
							idStr hostile_text = "Hostile Entities: ";
							idStr derelict_text = "Derelict Entities: ";
							for ( int i = 0; i < GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position.size(); i++ ) {
								if ( GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position[i] ) {
									idStr ship_name = GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position[i]->original_name;
									if ( GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position[i]->team == GetLocalPlayer()->team ) {
										if ( friendly_text == "Friendly Entities: " ) {
											friendly_text = friendly_text + "^4" + ship_name;
										} else {
											friendly_text = friendly_text + "^0, ^4" + ship_name;
										}
										// friendly
									} else if ( GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position[i]->team != GetLocalPlayer()->team && GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position[i]->HasNeutralityWithTeam(GetLocalPlayer()->team) && !GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position[i]->is_derelict ) {
										if ( neutral_text == "Neutral Entities: " ) {
											neutral_text = neutral_text + "^8" + ship_name;
										} else {
											neutral_text = neutral_text + "^0, ^8" + ship_name;
										}
										// neutral
									} else if ( !GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position[i]->is_derelict ) {
										if ( hostile_text == "Hostile Entities: " ) {
											hostile_text = hostile_text + "^1" + ship_name;
										} else {
											hostile_text = hostile_text + "^0, ^1" + ship_name;
										}
										// hostile
									} else if ( GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position[i]->is_derelict ) {
										if ( derelict_text == "Derelict Entities: " ) {
											derelict_text = derelict_text + "^8" + ship_name;
										} else {
											derelict_text = derelict_text + "^0, ^8" + ship_name;
										}
										// derelict
									}
								}
							}

							if ( friendly_text == "Friendly Entities: " ) {
								friendly_text = "Friendly Entities: None";
							}
							if ( neutral_text == "Neutral Entities: " ) {
								neutral_text = "Neutral Entities: None";
							}
							if ( hostile_text == "Hostile Entities: " ) {
								hostile_text = "Hostile Entities: None";
							}
							if ( derelict_text == "Derelict Entities: " ) {
								derelict_text = "Derelict Entities: None";
							}

							GetLocalPlayer()->guiOverlay->SetStateString("friendly_entities", friendly_text );
							GetLocalPlayer()->guiOverlay->SetStateString("neutral_entities", neutral_text );
							GetLocalPlayer()->guiOverlay->SetStateString("hostile_entities", hostile_text );
							GetLocalPlayer()->guiOverlay->SetStateString("derelict_entities", derelict_text );

				//// SITUATION SUMMARY BEGIN ////
							int cumulative_friendly_shield_and_hull_strength = 0;
							int cumulative_enemy_shield_and_hull_strength = 0;

							float cumulative_friendly_dps = 0.0f;
							float cumulative_enemy_dps = 0.0f;

							float cumulative_friendly_shield_rps = 0.0f;
							float cumulative_enemy_shield_rps = 0.0f;

							int num_friendly_entities = 0;
							int num_neutral_entities = 0;
							int num_hostile_entities = 0;

							if ( GetLocalPlayer()->PlayerShip && GetLocalPlayer()->PlayerShip->stargridpositionx == GetLocalPlayer()->ShipOnBoard->stargridpositionx && GetLocalPlayer()->PlayerShip->stargridpositiony == GetLocalPlayer()->ShipOnBoard->stargridpositiony ) {
								for ( int i = 0; i < GetLocalPlayer()->PlayerShip->ships_at_my_stargrid_position.size(); i++ ) {
									sbShip* ShipToEvaluate = GetLocalPlayer()->PlayerShip->ships_at_my_stargrid_position[i];
									if ( ShipToEvaluate ) {

										float ship_shields_rps = 0.0f; //(rps is Repair Per Second)
										float ship_weapons_dps = 0.0f; //(dps is Damage Per Second)
										float ship_torpedos_dps = 0.0f; //(dps is Damage Per Second)
										if ( ShipToEvaluate->consoles[SHIELDSMODULEID] && ShipToEvaluate->consoles[SHIELDSMODULEID]->ControlledModule ) {
											sbModule* ShipShieldsModule = ShipToEvaluate->consoles[SHIELDSMODULEID]->ControlledModule;
											ship_shields_rps = ShipToEvaluate->shields_repair_per_cycle / ( ((ShipShieldsModule->max_charge_amount / ShipShieldsModule->charge_added_each_cycle) * ShipShieldsModule->ms_between_charge_cycles) / 1000.0f );
										}
										if ( ShipToEvaluate->consoles[WEAPONSMODULEID] && ShipToEvaluate->consoles[WEAPONSMODULEID]->ControlledModule ) {
											sbModule* ShipWeaponsModule = ShipToEvaluate->consoles[WEAPONSMODULEID]->ControlledModule;
											ship_weapons_dps = ShipToEvaluate->weapons_damage_modifier / ( ((ShipWeaponsModule->max_charge_amount / ShipWeaponsModule->charge_added_each_cycle) * ShipWeaponsModule->ms_between_charge_cycles) / 1000.0f );
										}
										if ( ShipToEvaluate->consoles[TORPEDOSMODULEID] && ShipToEvaluate->consoles[TORPEDOSMODULEID]->ControlledModule ) {
											sbModule* ShipTorpedosModule = ShipToEvaluate->consoles[TORPEDOSMODULEID]->ControlledModule;
											ship_torpedos_dps = ShipToEvaluate->weapons_damage_modifier / ( ((ShipTorpedosModule->max_charge_amount / ShipTorpedosModule->charge_added_each_cycle) * ShipTorpedosModule->ms_between_charge_cycles) / 1000.0f );
										}


										if ( ShipToEvaluate->team == GetLocalPlayer()->team ) {
											cumulative_friendly_shield_and_hull_strength = cumulative_friendly_shield_and_hull_strength + ShipToEvaluate->hullStrength + ShipToEvaluate->shieldStrength;
											cumulative_friendly_dps = cumulative_friendly_dps + ship_weapons_dps + ship_torpedos_dps;
											cumulative_friendly_shield_rps = cumulative_friendly_shield_rps + ship_shields_rps;
											num_friendly_entities++;
											// friendly
										} else if ( ShipToEvaluate->team != GetLocalPlayer()->team && ShipToEvaluate->HasNeutralityWithTeam(GetLocalPlayer()->team) && !ShipToEvaluate->is_derelict ) {
											num_neutral_entities++;
											// neutral
										} else if ( !ShipToEvaluate->is_derelict ) {
											cumulative_enemy_shield_and_hull_strength = cumulative_enemy_shield_and_hull_strength + ShipToEvaluate->hullStrength + ShipToEvaluate->shieldStrength;
											cumulative_enemy_dps = cumulative_enemy_dps + ship_weapons_dps + ship_torpedos_dps;
											cumulative_enemy_shield_rps = cumulative_enemy_shield_rps + ship_shields_rps;
											num_hostile_entities++;
											// hostile
										} else if ( ShipToEvaluate->is_derelict ) {
											
											// derelict
										}
									}
								}
								// Do the same for the PlayerShip:
								if ( GetLocalPlayer()->PlayerShip ) {

									float ship_shields_rps = 0.0f; //(rps is Repair Per Second)
									float ship_weapons_dps = 0.0f; //(dps is Damage Per Second)
									float ship_torpedos_dps = 0.0f; //(dps is Damage Per Second)
									if ( GetLocalPlayer()->PlayerShip->consoles[SHIELDSMODULEID] && GetLocalPlayer()->PlayerShip->consoles[SHIELDSMODULEID]->ControlledModule ) {
										sbModule* ShipShieldsModule = GetLocalPlayer()->PlayerShip->consoles[SHIELDSMODULEID]->ControlledModule;
										ship_shields_rps = GetLocalPlayer()->PlayerShip->shields_repair_per_cycle / ( ((ShipShieldsModule->max_charge_amount / ShipShieldsModule->charge_added_each_cycle) * ShipShieldsModule->ms_between_charge_cycles) / 1000.0f );
									}
									if ( GetLocalPlayer()->PlayerShip->consoles[WEAPONSMODULEID] && GetLocalPlayer()->PlayerShip->consoles[WEAPONSMODULEID]->ControlledModule ) {
										sbModule* ShipWeaponsModule = GetLocalPlayer()->PlayerShip->consoles[WEAPONSMODULEID]->ControlledModule;
										ship_weapons_dps = GetLocalPlayer()->PlayerShip->weapons_damage_modifier / ( ((ShipWeaponsModule->max_charge_amount / ShipWeaponsModule->charge_added_each_cycle) * ShipWeaponsModule->ms_between_charge_cycles) / 1000.0f );
									}
									if ( GetLocalPlayer()->PlayerShip->consoles[TORPEDOSMODULEID] && GetLocalPlayer()->PlayerShip->consoles[TORPEDOSMODULEID]->ControlledModule ) {
										sbModule* ShipTorpedosModule = GetLocalPlayer()->PlayerShip->consoles[TORPEDOSMODULEID]->ControlledModule;
										ship_torpedos_dps = GetLocalPlayer()->PlayerShip->weapons_damage_modifier / ( ((ShipTorpedosModule->max_charge_amount / ShipTorpedosModule->charge_added_each_cycle) * ShipTorpedosModule->ms_between_charge_cycles) / 1000.0f );
									}

									cumulative_friendly_shield_and_hull_strength = cumulative_friendly_shield_and_hull_strength + GetLocalPlayer()->PlayerShip->hullStrength + GetLocalPlayer()->PlayerShip->shieldStrength;
									cumulative_friendly_dps = cumulative_friendly_dps + ship_weapons_dps + ship_torpedos_dps;
									cumulative_friendly_shield_rps = cumulative_friendly_shield_rps + ship_shields_rps;
								}



								float friendly_to_enemy_shield_and_hull_ratio = 777.0f; // some really high number because the playership might be the only ship
								if ( cumulative_enemy_shield_and_hull_strength > 0 ) {
									friendly_to_enemy_shield_and_hull_ratio = (float)cumulative_friendly_shield_and_hull_strength / (float)cumulative_enemy_shield_and_hull_strength;
								}

								float friendly_to_enemy_dps_ratio = 777.0f; // some really high number because the playership might be the only ship
								if ( cumulative_enemy_dps > 0 ) {
									friendly_to_enemy_dps_ratio = (float)cumulative_friendly_dps / (float)cumulative_enemy_dps;
								}

								float friendly_to_enemy_shields_rps_ratio = 777.0f; // some really high number because the playership might be the only ship
								if ( cumulative_enemy_shield_rps > 0 ) {
									friendly_to_enemy_shields_rps_ratio = (float)cumulative_friendly_shield_rps / (float)cumulative_enemy_shield_rps;
								}

								// Now out of a total of 1 - we evaluate
								float friendly_to_enemy_overall_strength_ratio = (friendly_to_enemy_shield_and_hull_ratio * 0.45f) + (friendly_to_enemy_dps_ratio * 0.45f) + (friendly_to_enemy_shields_rps_ratio * 0.10f);

								idStr recommendations_text = "Recommendations: "; // Then we can append all recommendations.
								if ( GetLocalPlayer()->ShipOnBoard->ships_at_my_stargrid_position.size() <= 0 ) {
									GetLocalPlayer()->guiOverlay->SetStateString("situation_summary", "Situation: ^0Safe" );
									recommendations_text += "None. ";
								} else if ( hostile_text == "Hostile Entities: None" ) {
									GetLocalPlayer()->guiOverlay->SetStateString("situation_summary", "Situation: ^0Safe" );
									recommendations_text += "You might want to explore any nearby entities - as there are no hostile entities in local space. ";
								} else if ( friendly_to_enemy_overall_strength_ratio > 1.0f ) {
									GetLocalPlayer()->guiOverlay->SetStateString("situation_summary", "Situation: ^3Dangerous" );
									recommendations_text += "Disabling enemy shields will be a priority. If you are able, you should try to board and disable hostile entities. There may be salvageable items on board. ";
								} else if ( friendly_to_enemy_overall_strength_ratio <= 1.0f && friendly_to_enemy_overall_strength_ratio > 0.5f ) {
									GetLocalPlayer()->guiOverlay->SetStateString("situation_summary", "Situation: ^1Extremely Dangerous" );
									recommendations_text += "Disabling enemy weapons will be a priority. Fleeing might be the best course of action. ";
								} else if ( friendly_to_enemy_overall_strength_ratio <= 0.5f ) {
									GetLocalPlayer()->guiOverlay->SetStateString("situation_summary", "Situation: ^1Dire" );
									recommendations_text += "If fleeing is not an option, then you might want to consider abandoning ship. Perhaps you can board one of the attacking entities. ";
								}

								if ( friendly_to_enemy_shield_and_hull_ratio < 0.75f ) {
									recommendations_text += "\n\nYou will need to redirect power to your offensive systems in order to break through hostile defenses. ";
								}
								if ( friendly_to_enemy_shield_and_hull_ratio > 1.5f && num_hostile_entities > 0 ) {
									recommendations_text += "\n\nYour defenses are strong. Use them to your advantage. ";
								}
								if ( friendly_to_enemy_dps_ratio < 1.0f ) {
									recommendations_text += "\n\nAttempt to disable hostile offensive capability. ";
								}
								if ( friendly_to_enemy_dps_ratio > 1.0f && num_hostile_entities > 0 ) {
									recommendations_text += "\n\nKeeping your ship's weapons functioning properly will help to defeat hostile entities. ";
								}
								if ( friendly_to_enemy_shields_rps_ratio > 1.5f && num_hostile_entities > 0 ) {
									recommendations_text += "\n\nMaintaining shield integrity will help to prevent unnecessary damage to your ship. ";
								}
								if ( friendly_to_enemy_shields_rps_ratio < 0.75f ) {
									recommendations_text += "\n\nDisabling enemy shields will be a priority. ";
								}
								if ( num_friendly_entities == 1 && num_hostile_entities > 0 ) {
									recommendations_text += "\n\nThere is a friendly entity in local space. You should try to work with them in order to defeat hostile entities. ";
								}
								if ( num_friendly_entities > 1 && num_hostile_entities > 0 ) {
									recommendations_text += "\n\nThere are friendly entities. You should try to work with them in order to defeat hostile entities. ";
								}
								if ( num_hostile_entities > 2 ) {
									recommendations_text += "\n\nThere are multiple hostile entities. You should focus fire on one until it is unable to attack. ";
								}
								if ( num_neutral_entities == 1 ) {
									recommendations_text += "\n\nThere is a neutral entity. It is not a threat unless it is provoked. It can be an ally if you have the same enemy. ";
								}
								if ( num_neutral_entities > 1 ) {
									recommendations_text += "\n\nThere are neutral entities. They are not a threat unless they are provoked. They can be an ally if you have the same enemy. ";
								}


								GetLocalPlayer()->guiOverlay->SetStateString("recommendations", recommendations_text );

								//gameLocal.Printf( "The friendly_to_enemy_shield_and_hull_ratio is: " + idStr(friendly_to_enemy_shield_and_hull_ratio) + "\n");
								//gameLocal.Printf( "The friendly_to_enemy_dps_ratio is: " + idStr(friendly_to_enemy_dps_ratio) + "\n");
								//gameLocal.Printf( "The friendly_to_enemy_shields_rps_ratio is: " + idStr(friendly_to_enemy_shields_rps_ratio) + "\n");
								//gameLocal.Printf( "The friendly_to_enemy_overall_strength_ratio is: " + idStr(friendly_to_enemy_overall_strength_ratio) + "\n");

								//GetLocalPlayer()->guiOverlay->SetStateString("recommendations", "Recommendations: You might want to consider abandoning ship. Perhaps you can board one of the attacking entities." ); // "Recommendations: Disabling enemy weapons will be a priority." );
							} else {
								GetLocalPlayer()->guiOverlay->SetStateString("situation_summary", "Situation: ^8Unknown" );
								GetLocalPlayer()->guiOverlay->SetStateString("recommendations", "Recommendations: None" ); // "Recommendations: Disabling enemy weapons will be a priority." );
							}
				//// SITUATION SUMMARY END ////

						}
					}



					gameSoundWorld->FadeSoundClasses(0,-100.0f,2); //  -100.0f // all entities have a soundclass of zero unless it is set otherwise. 0.0f db is the default level.
					gameSoundWorld->FadeSoundClasses(3,-100.0f,2); //  -100.0f // ship alarms are soundclass 3. 0.0f db is the default level.
					gameSoundWorld->FadeSoundClasses(1,-10.0f,2); //  -10.0f // the ship hum sound volumes are set to soundclass 1 - we will dim it a bit here. The music shader for the story gui will be set to soundclass 2 I think. We can add more soundclasses if we need them, right now the limit is 4 - look for SOUND_MAX_CLASSES.
					g_stopTime.SetBool(true);
					g_stopTimeForceFrameNumUpdateDuring.SetBool(true);
					g_stopTimeForceRenderViewUpdateDuring.SetBool(true);
					GetLocalPlayer()->currently_in_story_gui = true;
					g_enableSlowmo.SetBool( false );
					gameLocal.GetLocalPlayer()->blurEnabled = false;
					gameLocal.GetLocalPlayer()->desaturateEnabled = false;
					gameLocal.GetLocalPlayer()->bloomEnabled = false;
					gameLocal.GetLocalPlayer()->playerView.FreeWarp(0);
					gameLocal.GetLocalPlayer()->playerView.Fade(idVec4(0,0,0,0),700);

					gameLocal.GetLocalPlayer()->UpdateGuiOverlay();
				}
				// display some generic gui give you maybe the star grid position or something even more generic maybe.
				//g_stopTime.SetBool(true); // we will want to pause the timje even for the generic story window.
				//g_stopTimeForceFrameNumUpdateDuring.SetBool(true);
				//g_stopTimeForceRenderViewUpdateDuring.SetBool(true);
				//GetLocalPlayer()->currently_in_story_gui = true;
			}
		}
	}

	if ( !story_window_displayed && gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->CaptainChairSeatedIn && gameLocal.GetLocalPlayer()->PlayerShip && gameLocal.GetLocalPlayer()->PlayerShip->TargetEntityInSpace ) {
		gameLocal.GetLocalPlayer()->SetViewAngles( gameLocal.GetLocalPlayer()->CaptainChairSeatedIn->GetPhysics()->GetAxis().ToAngles() );
		gameLocal.GetLocalPlayer()->SetAngles( gameLocal.GetLocalPlayer()->CaptainChairSeatedIn->GetPhysics()->GetAxis().ToAngles() );
		gameLocal.GetLocalPlayer()->SetOverlayCaptainGui();
		gameLocal.GetLocalPlayer()->CaptainGui->HandleNamedEvent( "GoToTacticalTab" );
	}

	return;
}

void idGameLocal::UpdateSunEntity() {
	idEntity	*ent;
	ent = FindEntityUsingDef( NULL, "stargrid_space_sun_entity" );

	if ( ent ) {
		idVec2 sun_entity_position(ent->spawnArgs.GetInt( "stargridposx", "0" ),ent->spawnArgs.GetInt( "stargridposy", "0" ));
		float max_distance = ( idVec2(0,0) - idVec2(MAX_STARGRID_X_POSITIONS,MAX_STARGRID_Y_POSITIONS) ).Length(); // 20.0f //idMath::Sqrt((MAX_STARGRID_X_POSITIONS^2) + (MAX_STARGRID_Y_POSITIONS^2));
		if ( gameLocal.GetLocalPlayer()->ShipOnBoard ) {
			idVec2 shiponboard_entity_position(GetLocalPlayer()->ShipOnBoard->stargridpositionx,GetLocalPlayer()->ShipOnBoard->stargridpositiony);
			if ( shiponboard_entity_position.x == sun_entity_position.x && shiponboard_entity_position.y == sun_entity_position.y ) {
				ent->Hide(); // if we are at the same position as the sun entity then hide it because we will have an sbShop phenomenon that will represent the sun there
			} else {
				ent->Show();
				float shiponboard_distance_to_sun_entity;
				shiponboard_distance_to_sun_entity = ( shiponboard_entity_position - sun_entity_position ).Length();
				if ( max_distance > 0 && shiponboard_distance_to_sun_entity > 0 ) {
					float size_modifier = idMath::ClampFloat(1,200,(max_distance / ent->spawnArgs.GetFloat( "max_distance_size_modifier", "2.0" )) * ((float)(shiponboard_distance_to_sun_entity/(float)max_distance))); // BOYETTE NOTE IMPORTANT: this spawnarg adjusts the ratio at which the sun size increases as we get closer to it. A smaller number means it will start smaller. A alarger number means it will start bigger. 1.0 would be natural.
					ent->SetShaderParm(11, size_modifier ); // display size // this works fine until we redo the skybox - with a skysphere.
					//ent->SetShaderParm(11, (max_distance / 1.0f) * ((float)(shiponboard_distance_to_sun_entity/(float)max_distance)) ); // display size // BOYETTE NOTE TODO: better this way eventually - or maybe not - once we increase sun entity size and increase the area of the space skybox.
					ent->SetShaderParm(10, ((float)(shiponboard_distance_to_sun_entity/(float)max_distance)) ); // distance percentage
				} else {
					ent->SetShaderParm(11, 1.0f );
					ent->SetShaderParm(10, 0.0f );
				}
				/*
				common->Printf( "The shiponboard_entity_position x is: %f \n", shiponboard_entity_position.x );
				common->Printf( "The shiponboard_entity_position y is: %f \n", shiponboard_entity_position.y );
				common->Printf( "The sun_entity_position x is: %f \n", sun_entity_position.x );
				common->Printf( "The sun_entity_position y is: %f \n", sun_entity_position.y );
				common->Printf( "The shiponboard_distance_to_sun_entity is: %f \n", shiponboard_distance_to_sun_entity );
				common->Printf( "The max_distance is: %f \n", max_distance );
				*/
			}
		}
	}

}

bool idGameLocal::CheckIfStarGridPositionIsOffLimitsToShipAI(int sgposx,int sgposy) {
	if ( stargridPositionsOffLimitsToShipAI[sgposx][sgposy] ) {
		return true;
	} else {
		return false;
	}
}

/* before adding "stargridstartpos_avoid_entities_of_same_class" spawnarg
void idGameLocal::RandomizeStarGridPositionsForCertainSpaceEntities() {
	idEntity*	ent = NULL;
	sbShip*		sbShipToCheck = NULL;

	std::vector<sbShip*> ships_at_sg_pos[MAX_STARGRID_X_POSITIONS + 1][MAX_STARGRID_Y_POSITIONS + 1];

	// boyette note: we need to first populate it with ships that aren't random and just have their sg pos set.
	for ( int i = 0; i < gameLocal.num_entities ; i++ ) {
		ent = gameLocal.entities[ i ];
		if ( ent && ent->IsType( sbShip::Type ) ) {
			sbShipToCheck = dynamic_cast<sbShip*>( ent );
			//if ( !dynamic_cast<sbStationarySpaceEntity*>(ent) ) { // BOYETTE NOTE IMPORTANT: this works fine for ignoring sbStationarySpaceEntitys - maybe we will put a spawn arg in to make this optional - but for now it is probably best to have "stargridstartpos_try_to_be_alone" relate to space stations and planets as well.
				if ( !sbShipToCheck->spawnArgs.GetBool("stargridstartpos_random", "0") && !sbShipToCheck->spawnArgs.GetBool("stargridstartposx_random", "0") && !sbShipToCheck->spawnArgs.GetBool("stargridstartposy_random", "0") ) { // if not a random location ship
					int sg_pos_x_set_in_spawnargs = idMath::ClampInt(1,MAX_STARGRID_X_POSITIONS + 1, sbShipToCheck->spawnArgs.GetInt("stargridstartposx") );
					int sg_pos_y_set_in_spawnargs = idMath::ClampInt(1,MAX_STARGRID_Y_POSITIONS + 1, sbShipToCheck->spawnArgs.GetInt("stargridstartposy")  );
					ships_at_sg_pos[sg_pos_x_set_in_spawnargs][sg_pos_y_set_in_spawnargs].push_back(sbShipToCheck);
				}
			//}
		}
	}

	////////std::srand(std::time(0)); // use current time as seed for random generator
    std::random_device rd;
    std::mt19937 gen(rd()); //mersenne_twister_engine is a random number engine based on Mersenne Twister algorithm. It produces high quality unsigned integer random numbers.
    //std::uniform_int_distribution<> dis_x(1, 6);
	//std::uniform_int_distribution<> dis_y(1, 6);
	////////gameLocal.random.SetSeed(std::rand());
	int num_sky_portal_entities = GetNumberOfNonPlayerShipSkyPortalEntities();

	// boyette note: then do the ships that should have random locations
	for ( int i = 0; i < gameLocal.num_entities ; i++ ) {
		ent = gameLocal.entities[ i ];
		if ( ent && ent->IsType( sbShip::Type ) ) {
			sbShipToCheck = dynamic_cast<sbShip*>( ent );
			if ( sbShipToCheck->spawnArgs.GetBool("stargridstartpos_random", "0") || sbShipToCheck->spawnArgs.GetBool("stargridstartposx_random", "0") || sbShipToCheck->spawnArgs.GetBool("stargridstartposy_random", "0") ) {
				if ( sbShipToCheck->spawnArgs.GetBool("stargridstartpos_random", "0") ) {
					sbShipToCheck->spawnArgs.SetBool("stargridstartposx_random", true);
					sbShipToCheck->spawnArgs.SetBool("stargridstartposy_random", true);
				}
				int num_stargrid_positions = MAX_STARGRID_X_POSITIONS * MAX_STARGRID_Y_POSITIONS;
				int attempts_to_be_random = num_stargrid_positions; // BOYETTE NOTE: mostly arbitrary - if it can't get to a suitable random stargrid position after this many attempts it will just increment x,y until it gets to a suitable stargrid position - if there are no suitable ones it will default to MAX_STARGRID_X_POSITIONS,MAX_STARGRID_Y_POSITIONS (which we hope is the farthest away from the player)
				bool found_suitable_stargrid_position = false;

				int specified_stargridpositionx = sbShipToCheck->spawnArgs.GetInt("stargridstartposx");
				int specified_stargridpositiony = sbShipToCheck->spawnArgs.GetInt("stargridstartposy");
				bool stargridstartposx_should_be_random = sbShipToCheck->spawnArgs.GetBool("stargridstartposx_random", "0");
				bool stargridstartposy_should_be_random = sbShipToCheck->spawnArgs.GetBool("stargridstartposy_random", "0");

				int stargridstartposx_random_max = sbShipToCheck->spawnArgs.GetInt("stargridstartposx_random_max", idStr(MAX_STARGRID_X_POSITIONS).c_str() );
				int stargridstartposy_random_max = sbShipToCheck->spawnArgs.GetInt("stargridstartposy_random_max", idStr(MAX_STARGRID_Y_POSITIONS).c_str() );
				int stargridstartposx_random_min = sbShipToCheck->spawnArgs.GetInt("stargridstartposx_random_min", "1");
				int stargridstartposy_random_min = sbShipToCheck->spawnArgs.GetInt("stargridstartposy_random_min", "1");

				if ( stargridstartposx_random_max < stargridstartposx_random_min ) {
					stargridstartposx_random_max = MAX_STARGRID_X_POSITIONS;
					stargridstartposx_random_min = 1;
				}
				if ( stargridstartposy_random_max < stargridstartposy_random_min ) {
					stargridstartposy_random_max = MAX_STARGRID_Y_POSITIONS;
					stargridstartposy_random_min = 1;
				}

				int x_random_range = stargridstartposx_random_max - stargridstartposx_random_min;
				int y_random_range = stargridstartposy_random_max - stargridstartposy_random_min;

				////////std::srand(std::time(0)); // use current time as seed for random generator
				std::uniform_int_distribution<> dis_x(stargridstartposx_random_min, stargridstartposx_random_max);
				std::uniform_int_distribution<> dis_y(stargridstartposy_random_min, stargridstartposy_random_max);
				////////gameLocal.random.SetSeed(std::rand());

				// first try to put it at a random stargrid position
				for ( int i = 0; i < attempts_to_be_random; i++ ) {
					////////gameLocal.random.SetSeed(std::rand() % 1000);
					////////gameLocal.random.SetSeed(std::rand() % 1000);
					
					if ( sbShipToCheck->stargridstartpos_random_team.Length() ) {
						bool sg_team_member_found = false;
						for (int x = 1; x <= MAX_STARGRID_X_POSITIONS; x++) {
							for (int y = 1; y <= MAX_STARGRID_Y_POSITIONS; y++) {
								for ( int ship_counter = 0; ship_counter < ships_at_sg_pos[x][y].size(); ship_counter++ ) {
									if ( !idStr::Icmp( ships_at_sg_pos[x][y][ship_counter]->stargridstartpos_random_team, sbShipToCheck->stargridstartpos_random_team ) && !CheckIfStarGridPositionIsOffLimitsToShipAI(x,y) && ships_at_sg_pos[x][y].size() < num_sky_portal_entities ) {
										sg_team_member_found = true;
										sbShipToCheck->stargridpositionx = x;
										sbShipToCheck->stargridpositiony = y;
										ships_at_sg_pos[x][y].push_back(sbShipToCheck);
										found_suitable_stargrid_position = true;
										break;
									}
								}
								if ( sg_team_member_found ) {
									break;
								}
							}
							if ( sg_team_member_found ) {
								break;
							}
						}
						if ( sg_team_member_found ) {
							break;
						}
					}
					

					if ( stargridstartposx_should_be_random ) {
						sbShipToCheck->stargridpositionx = idMath::ClampInt(1,MAX_STARGRID_X_POSITIONS + 1,dis_x(gen));//idMath::ClampInt(1,MAX_STARGRID_X_POSITIONS + 1,stargridstartposx_random_min + random.RandomInt( x_random_range + 1 ) );
					} else {
						sbShipToCheck->stargridpositionx = specified_stargridpositionx;
					}
					if ( stargridstartposy_should_be_random ) {
						sbShipToCheck->stargridpositiony = idMath::ClampInt(1,MAX_STARGRID_Y_POSITIONS + 1,dis_y(gen));//idMath::ClampInt(1,MAX_STARGRID_Y_POSITIONS + 1,stargridstartposy_random_min + random.RandomInt( y_random_range + 1 ) );
					} else {
						sbShipToCheck->stargridpositiony = specified_stargridpositiony;
					}

					if ( !CheckIfStarGridPositionIsOffLimitsToShipAI(sbShipToCheck->stargridpositionx,sbShipToCheck->stargridpositiony) && ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].size() < num_sky_portal_entities ) {
						if ( sbShipToCheck->stargridstartpos_try_to_be_alone ) { // if we want to be alone - check if there is already another ship at this stargrid position
							if ( ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].size() <= 0 ) {
								found_suitable_stargrid_position = true;
								ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].push_back(sbShipToCheck);
								break;
							}
						} else { // if we don't care if we are alone - then this stargrid position is suitable
							// check if there are any other ships at this stargrid position that are trying to be alone.
							bool is_suitable = true;
							for ( int ship_counter = 0; ship_counter < ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].size(); ship_counter++ ) {
								if ( ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony][ship_counter]->stargridstartpos_try_to_be_alone ) {
									is_suitable = false;
								}
							}
							if ( is_suitable ) {
								found_suitable_stargrid_position = true;
								ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].push_back(sbShipToCheck);
								break;
							}
						}
					}
				}

				// then if that doesn't work, just cycle through all the stargrid positions and try to find a suitable one
				if ( !found_suitable_stargrid_position ) {
					for (int x = 1; x <= MAX_STARGRID_X_POSITIONS; x++) {
						for (int y = 1; y <= MAX_STARGRID_Y_POSITIONS; y++) {
							sbShipToCheck->stargridpositionx = x;
							sbShipToCheck->stargridpositiony = y;
							if ( !CheckIfStarGridPositionIsOffLimitsToShipAI(sbShipToCheck->stargridpositionx,sbShipToCheck->stargridpositiony) && ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].size() < num_sky_portal_entities ) {
								if ( sbShipToCheck->stargridstartpos_try_to_be_alone ) { // if we want to be alone - check if there is already another ship at this stargrid position
									if ( ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].size() <= 0 ) {
										found_suitable_stargrid_position = true;
										ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].push_back(sbShipToCheck);
										break;
									}
								} else { // if we don't care if we are alone - then this stargrid position is suitable
									found_suitable_stargrid_position = true;
									ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].push_back(sbShipToCheck);
									break;
								}
							}
						}
						if ( found_suitable_stargrid_position ) {
							break;
						}
					}
				}

				// finally, if none of that worked - just put the sbShip at the maximum position.
				if ( !found_suitable_stargrid_position ) {
					sbShipToCheck->stargridpositionx = MAX_STARGRID_X_POSITIONS;
					sbShipToCheck->stargridpositiony = MAX_STARGRID_Y_POSITIONS;
					ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].push_back(sbShipToCheck);
				}
			}
		}
	
	}
}
*/

void idGameLocal::RandomizeStarGridPositionsForCertainSpaceEntities() {
	idEntity*	ent = NULL;
	sbShip*		sbShipToCheck = NULL;

	std::vector<sbShip*> ships_at_sg_pos[MAX_STARGRID_X_POSITIONS + 1][MAX_STARGRID_Y_POSITIONS + 1];

	// boyette note: we need to first populate it with ships that aren't random and just have their sg pos set.
	for ( int i = 0; i < gameLocal.num_entities ; i++ ) {
		ent = gameLocal.entities[ i ];
		if ( ent && ent->IsType( sbShip::Type ) ) {
			sbShipToCheck = dynamic_cast<sbShip*>( ent );
			//if ( !dynamic_cast<sbStationarySpaceEntity*>(ent) ) { // BOYETTE NOTE IMPORTANT: this works fine for ignoring sbStationarySpaceEntitys - maybe we will put a spawn arg in to make this optional - but for now it is probably best to have "stargridstartpos_try_to_be_alone" relate to space stations and planets as well.
				if ( !sbShipToCheck->spawnArgs.GetBool("stargridstartpos_random", "0") && !sbShipToCheck->spawnArgs.GetBool("stargridstartposx_random", "0") && !sbShipToCheck->spawnArgs.GetBool("stargridstartposy_random", "0") ) { // if not a random location ship
					int sg_pos_x_set_in_spawnargs = idMath::ClampInt(1,MAX_STARGRID_X_POSITIONS + 1, sbShipToCheck->spawnArgs.GetInt("stargridstartposx") );
					int sg_pos_y_set_in_spawnargs = idMath::ClampInt(1,MAX_STARGRID_Y_POSITIONS + 1, sbShipToCheck->spawnArgs.GetInt("stargridstartposy")  );
					ships_at_sg_pos[sg_pos_x_set_in_spawnargs][sg_pos_y_set_in_spawnargs].push_back(sbShipToCheck);
				}
			//}
		}
	}

	////////std::srand(std::time(0)); // use current time as seed for random generator
    std::random_device rd;
    std::mt19937 gen(rd()); //mersenne_twister_engine is a random number engine based on Mersenne Twister algorithm. It produces high quality unsigned integer random numbers.
    //std::uniform_int_distribution<> dis_x(1, 6);
	//std::uniform_int_distribution<> dis_y(1, 6);
	////////gameLocal.random.SetSeed(std::rand());
	int num_sky_portal_entities = GetNumberOfNonPlayerShipSkyPortalEntities();

	// boyette note: then do the ships that should have random locations
	for ( int i = 0; i < gameLocal.num_entities ; i++ ) {
		ent = gameLocal.entities[ i ];
		if ( ent && ent->IsType( sbShip::Type ) ) {
			sbShipToCheck = dynamic_cast<sbShip*>( ent );
			if ( sbShipToCheck->spawnArgs.GetBool("stargridstartpos_random", "0") || sbShipToCheck->spawnArgs.GetBool("stargridstartposx_random", "0") || sbShipToCheck->spawnArgs.GetBool("stargridstartposy_random", "0") ) {
				if ( sbShipToCheck->spawnArgs.GetBool("stargridstartpos_random", "0") ) {
					sbShipToCheck->spawnArgs.SetBool("stargridstartposx_random", true);
					sbShipToCheck->spawnArgs.SetBool("stargridstartposy_random", true);
				}
				int num_stargrid_positions = MAX_STARGRID_X_POSITIONS * MAX_STARGRID_Y_POSITIONS;
				int attempts_to_be_random = num_stargrid_positions; // BOYETTE NOTE: mostly arbitrary - if it can't get to a suitable random stargrid position after this many attempts it will just increment x,y until it gets to a suitable stargrid position - if there are no suitable ones it will default to MAX_STARGRID_X_POSITIONS,MAX_STARGRID_Y_POSITIONS (which we hope is the farthest away from the player)
				bool found_suitable_stargrid_position = false;

				int specified_stargridpositionx = sbShipToCheck->spawnArgs.GetInt("stargridstartposx");
				int specified_stargridpositiony = sbShipToCheck->spawnArgs.GetInt("stargridstartposy");
				bool stargridstartposx_should_be_random = sbShipToCheck->spawnArgs.GetBool("stargridstartposx_random", "0");
				bool stargridstartposy_should_be_random = sbShipToCheck->spawnArgs.GetBool("stargridstartposy_random", "0");

				int stargridstartposx_random_max = sbShipToCheck->spawnArgs.GetInt("stargridstartposx_random_max", idStr(MAX_STARGRID_X_POSITIONS).c_str() );
				int stargridstartposy_random_max = sbShipToCheck->spawnArgs.GetInt("stargridstartposy_random_max", idStr(MAX_STARGRID_Y_POSITIONS).c_str() );
				int stargridstartposx_random_min = sbShipToCheck->spawnArgs.GetInt("stargridstartposx_random_min", "1");
				int stargridstartposy_random_min = sbShipToCheck->spawnArgs.GetInt("stargridstartposy_random_min", "1");

				if ( stargridstartposx_random_max < stargridstartposx_random_min ) {
					stargridstartposx_random_max = MAX_STARGRID_X_POSITIONS;
					stargridstartposx_random_min = 1;
				}
				if ( stargridstartposy_random_max < stargridstartposy_random_min ) {
					stargridstartposy_random_max = MAX_STARGRID_Y_POSITIONS;
					stargridstartposy_random_min = 1;
				}

				int x_random_range = stargridstartposx_random_max - stargridstartposx_random_min;
				int y_random_range = stargridstartposy_random_max - stargridstartposy_random_min;

				////////std::srand(std::time(0)); // use current time as seed for random generator
				std::uniform_int_distribution<> dis_x(stargridstartposx_random_min, stargridstartposx_random_max);
				std::uniform_int_distribution<> dis_y(stargridstartposy_random_min, stargridstartposy_random_max);
				////////gameLocal.random.SetSeed(std::rand());

				// first try to put it at a random stargrid position
				for ( int i = 0; i < attempts_to_be_random; i++ ) {
					////////gameLocal.random.SetSeed(std::rand() % 1000);
					////////gameLocal.random.SetSeed(std::rand() % 1000);
					
					if ( sbShipToCheck->stargridstartpos_random_team.Length() ) {
						bool sg_team_member_found = false;
						for (int x = 1; x <= MAX_STARGRID_X_POSITIONS; x++) {
							for (int y = 1; y <= MAX_STARGRID_Y_POSITIONS; y++) {
								for ( int ship_counter = 0; ship_counter < ships_at_sg_pos[x][y].size(); ship_counter++ ) {
									if ( !idStr::Icmp( ships_at_sg_pos[x][y][ship_counter]->stargridstartpos_random_team, sbShipToCheck->stargridstartpos_random_team ) && ships_at_sg_pos[x][y][ship_counter]->stargridstartpos_random_team == sbShipToCheck->stargridstartpos_random_team && !CheckIfStarGridPositionIsOffLimitsToShipAI(x,y) && ships_at_sg_pos[x][y].size() < num_sky_portal_entities ) {
										sg_team_member_found = true;
										sbShipToCheck->stargridpositionx = x;
										sbShipToCheck->stargridpositiony = y;
										ships_at_sg_pos[x][y].push_back(sbShipToCheck);
										found_suitable_stargrid_position = true;
										break;
									}
								}
								if ( sg_team_member_found ) {
									break;
								}
							}
							if ( sg_team_member_found ) {
								break;
							}
						}
						if ( sg_team_member_found ) {
							break;
						}
					}
					

					if ( stargridstartposx_should_be_random ) {
						sbShipToCheck->stargridpositionx = idMath::ClampInt(1,MAX_STARGRID_X_POSITIONS + 1,dis_x(gen));//idMath::ClampInt(1,MAX_STARGRID_X_POSITIONS + 1,stargridstartposx_random_min + random.RandomInt( x_random_range + 1 ) );
					} else {
						sbShipToCheck->stargridpositionx = specified_stargridpositionx;
					}
					if ( stargridstartposy_should_be_random ) {
						sbShipToCheck->stargridpositiony = idMath::ClampInt(1,MAX_STARGRID_Y_POSITIONS + 1,dis_y(gen));//idMath::ClampInt(1,MAX_STARGRID_Y_POSITIONS + 1,stargridstartposy_random_min + random.RandomInt( y_random_range + 1 ) );
					} else {
						sbShipToCheck->stargridpositiony = specified_stargridpositiony;
					}

					if ( !CheckIfStarGridPositionIsOffLimitsToShipAI(sbShipToCheck->stargridpositionx,sbShipToCheck->stargridpositiony) && ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].size() < num_sky_portal_entities ) {
						if ( sbShipToCheck->stargridstartpos_try_to_be_alone ) { // if we want to be alone - check if there is already another ship at this stargrid position
							if ( ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].size() <= 0 ) {
								found_suitable_stargrid_position = true;
								ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].push_back(sbShipToCheck);
								//gameLocal.Printf( sbShipToCheck->original_name + " found a suitable try_to_be_alone stargrid position.\n" );
								break;
							}
						} else if ( sbShipToCheck->stargridstartpos_avoid_entities_of_same_class ) {
							if ( ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].size() <= 0 ) {
								found_suitable_stargrid_position = true;
								ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].push_back(sbShipToCheck);
								break;
							} else {
								bool no_entities_of_same_type = true; // no entities of the same type at this stargrid position
								bool is_suitable_try_to_be_alone = true;
								for ( int ship_counter = 0; ship_counter < ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].size(); ship_counter++ ) {
									if ( sbShipToCheck->GetType() == ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony][ship_counter]->GetType() ) {
										no_entities_of_same_type = false;
									}
									if ( ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony][ship_counter]->stargridstartpos_try_to_be_alone ) {
										is_suitable_try_to_be_alone = false;
									}
								}
								if ( no_entities_of_same_type && is_suitable_try_to_be_alone ) {
									found_suitable_stargrid_position = true;
									ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].push_back(sbShipToCheck);
									break;
								}
							}
						} else { // if we don't care if we are alone - then this stargrid position is suitable
							// check if there are any other ships at this stargrid position that are trying to be alone.
							bool is_suitable_try_to_be_alone = true;
							bool is_suitable_avoid_entities_of_same_class = true;
							for ( int ship_counter = 0; ship_counter < ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].size(); ship_counter++ ) {
								if ( ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony][ship_counter]->stargridstartpos_try_to_be_alone ) {
									is_suitable_try_to_be_alone = false;
								}
								if ( ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony][ship_counter]->stargridstartpos_avoid_entities_of_same_class && sbShipToCheck->GetType() == ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony][ship_counter]->GetType() ) {
									is_suitable_avoid_entities_of_same_class = false;
								}
							}
							if ( is_suitable_try_to_be_alone && is_suitable_avoid_entities_of_same_class ) {
								found_suitable_stargrid_position = true;
								ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].push_back(sbShipToCheck);
								break;
							}
						}
					}
				}

				// then if that doesn't work, just cycle through all the stargrid positions and try to find a suitable one
				if ( !found_suitable_stargrid_position ) {
					gameLocal.Printf( sbShipToCheck->original_name + " failed to find a random stargrid position. It will just try to find any suitable one now.\n" );
					for (int x = 1; x <= MAX_STARGRID_X_POSITIONS; x++) {
						for (int y = 1; y <= MAX_STARGRID_Y_POSITIONS; y++) {
							sbShipToCheck->stargridpositionx = x;
							sbShipToCheck->stargridpositiony = y;
							if ( !CheckIfStarGridPositionIsOffLimitsToShipAI(sbShipToCheck->stargridpositionx,sbShipToCheck->stargridpositiony) && ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].size() < num_sky_portal_entities ) {
								if ( sbShipToCheck->stargridstartpos_try_to_be_alone ) { // if we want to be alone - check if there is already another ship at this stargrid position
									if ( ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].size() <= 0 ) {
										found_suitable_stargrid_position = true;
										ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].push_back(sbShipToCheck);
										break;
									}
								} else if ( sbShipToCheck->stargridstartpos_avoid_entities_of_same_class ) {
									if ( ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].size() <= 0 ) {
										found_suitable_stargrid_position = true;
										ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].push_back(sbShipToCheck);
										break;
									} else {
										bool no_entities_of_same_type = true; // no entities of the same type at this stargrid position
										bool is_suitable_try_to_be_alone = true;
										for ( int ship_counter = 0; ship_counter < ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].size(); ship_counter++ ) {
											if ( sbShipToCheck->GetType() == ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony][ship_counter]->GetType() ) {
												no_entities_of_same_type = false;
											}
											if ( ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony][ship_counter]->stargridstartpos_try_to_be_alone ) {
												is_suitable_try_to_be_alone = false;
											}
										}
										if ( no_entities_of_same_type && is_suitable_try_to_be_alone ) {
											found_suitable_stargrid_position = true;
											ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].push_back(sbShipToCheck);
											break;
										}
									}
								} else { // one last attempt to get it at a suitable stargrid position
									bool is_suitable_try_to_be_alone = true;
									bool is_suitable_avoid_entities_of_same_class = true;
									for ( int ship_counter = 0; ship_counter < ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].size(); ship_counter++ ) {
										if ( ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony][ship_counter]->stargridstartpos_try_to_be_alone ) {
											is_suitable_try_to_be_alone = false;
										}
										if ( ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony][ship_counter]->stargridstartpos_avoid_entities_of_same_class && sbShipToCheck->GetType() == ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony][ship_counter]->GetType() ) {
											is_suitable_avoid_entities_of_same_class = false;
										}
									}
									if ( is_suitable_try_to_be_alone && is_suitable_avoid_entities_of_same_class ) {
										found_suitable_stargrid_position = true;
										ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].push_back(sbShipToCheck);
										break;
									}
								}
							}
						}
						if ( found_suitable_stargrid_position ) {
							break;
						}
					}
				}

				// finally, if none of that worked - just put the it at the the middle stargrid position.
				if ( !found_suitable_stargrid_position ) {
					gameLocal.Warning( sbShipToCheck->original_name + " could not find a suitable stargrid position - putting at middle stargrid position.\n" );
					sbShipToCheck->stargridpositionx = MAX_STARGRID_X_POSITIONS / 2;
					sbShipToCheck->stargridpositiony = MAX_STARGRID_Y_POSITIONS / 2;
					ships_at_sg_pos[sbShipToCheck->stargridpositionx][sbShipToCheck->stargridpositiony].push_back(sbShipToCheck);
				}
			}
		}
	
	}
}

void idGameLocal::GetStarGridPositionsThatAreOffLimitsToShipAI() {
	for (int x = 1; x <= MAX_STARGRID_X_POSITIONS; x++) {
		for (int y = 1; y <= MAX_STARGRID_Y_POSITIONS; y++) {
			stargridPositionsOffLimitsToShipAI[x][y] = false; // this is not necessary because bools have a default initialized value of false - but we do it anyways for clarity.
		}
	}

	if ( StoryRepositoryEntity ) {
		for (int x = 1; x <= MAX_STARGRID_X_POSITIONS; x++) {
			for (int y = 1; y <= MAX_STARGRID_Y_POSITIONS; y++) {
				if ( StoryRepositoryEntity->spawnArgs.GetBool( va("stargrid_pos_off_limits_to_ship_AI_x%i_y%i",x ,y), "0" ) ) {
					stargridPositionsOffLimitsToShipAI[x][y] = true;
				}
			}
		}
	}
}


// BOYETTE FULLSCREEN GUI BEGIN
/*
================
idGameLocal::PlayerGUIOverlay
================
*/
idUserInterface* idGameLocal::PlayerGUIOverlay( void ) {
	if ( !isMultiplayer && GetLocalPlayer() && GetLocalPlayer()->guiOverlay ) {
		return GetLocalPlayer()->guiOverlay;
	} else {
		return NULL;
	}
}

/*
================
idGameLocal::HandleGuiCommands
================
*/
bool idGameLocal::HandleGuiCommandsOnPlayer( const char *cmds ) {
	if ( !isMultiplayer && GetLocalPlayer() && GetLocalPlayer()->guiOverlay ) {
		return GetLocalPlayer()->HandleGuiCommands( GetLocalPlayer(), cmds );
	} else {
		return false;
	}
}
// BOYETTE FULLSCREEN GUI END

// boyette space command end

