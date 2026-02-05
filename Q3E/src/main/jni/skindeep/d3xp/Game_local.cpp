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
#include "idlib/Timer.h"
#include "framework/async/NetworkSystem.h"
#include "framework/BuildVersion.h"
#include "framework/DeclEntityDef.h"
#include "framework/FileSystem.h"
#include "renderer/ModelManager.h"
#include "tools/compilers/aas/AASFileManager.h"
#include "ui/FeedAlertWindow.h"
#include "ui/UserInterfaceLocal.h"

#include "gamesys/SysCvar.h"
#include "gamesys/SysCmds.h"
#include "script/Script_Thread.h"
#include "ai/AI.h"
#include "anim/Anim_Testmodel.h"
#include "Camera.h"
#include "SmokeParticles.h"
#include "Player.h"
#include "WorldSpawn.h"
#include "Misc.h"
#include "Trigger.h"
#include "Moveable.h"
#include "bc_ventpeek.h"
#include "sw_skycontroller.h"

#include "SecurityCamera.h"
#include "BrittleFracture.h"

#include "Misc.h"

#include "bc_infostation.h"
#include "bc_trigger_deodorant.h"
#include "bc_trigger_sneeze.h"
#include "bc_meta.h"
#include "bc_ventdoor.h"

#include "framework/Session.h"

#include "Game_local.h"


const int NUM_RENDER_PORTAL_BITS	= idMath::BitsForInteger( PS_BLOCK_ALL );

const float DEFAULT_GRAVITY			= 1066.0f;
const idVec3 DEFAULT_GRAVITY_VEC3( 0, 0, -DEFAULT_GRAVITY );

const int	CINEMATIC_SKIP_DELAY	= SEC2MS( 2.0f );

//Teleport puck.
const int PHASE_MINDISTANCE = 32;
const int PHASE_MAXDISTANCE = 256;


const int SUBTITLE_MINTIME = 1600; //minimum time for subtitle. If subtitle is onscreen for less time, clamp it to this minimum.
const int SUBTITLE_POS_0 = 40;
const int SUBTITLE_POS_1 = 20;
const int SUBTITLE_POS_2 = 0;

const char* LOG_FEED_ALERT_WINDOW = "EventLogFeedAlertWindow";

// blendo eric combined game/dll means this is likely no longer needed
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

// Subtitles cvars
// For colors: uses the integer of a color in the color table in neo\idlib\Str.h (C_COLOR_RED, C_COLOR_GREEN, etc)
idCVar gui_subtitles("gui_subtitles", "1", CVAR_BOOL | CVAR_GUI | CVAR_ARCHIVE, "Enable subtitles.");
idCVar gui_subtitleShowSpeaker("gui_subtitleShowSpeaker", "1", CVAR_BOOL | CVAR_GUI | CVAR_ARCHIVE, "Show subtitle speaker name.");
idCVar gui_subtitleScale("gui_subtitleScale", "0.6", CVAR_FLOAT | CVAR_GUI | CVAR_ARCHIVE, "Scale of subtitles on screen");
idCVar gui_subtitleColor("gui_subtitleColor", "7", CVAR_INTEGER | CVAR_GUI | CVAR_ARCHIVE, "Color of subtitles (1-9 corresponding to idStr color table)", 1, 9);
idCVar gui_subtitleSpeakerColor("gui_subtitleSpeakerColor", "3", CVAR_INTEGER | CVAR_GUI | CVAR_ARCHIVE, "Color of speaker name for subtitles (1-9 corresponding to idStr color table)", 1, 9);
idCVar gui_subtitleBGOpacity("gui_subtitleBGOpacity", "0.5", CVAR_FLOAT | CVAR_GUI | CVAR_ARCHIVE, "Background opacity for subtitle display", 0.0f, 1.0f);
idCVar gui_subtitleWidth("gui_subtitleWidth", "240.0", CVAR_FLOAT | CVAR_GUI | CVAR_ARCHIVE, "Maximum width of subtitle display on screen (out of 640)", 200.0f, 640.0f);
idCVar gui_subtitleY("gui_subtitleY", "460.0", CVAR_FLOAT | CVAR_GUI | CVAR_ARCHIVE, "Default subtitle y position on screen (may move up to prevent going off-screen)", 0.0f, 480.0f);
idCVar gui_subtitleGap("gui_subtitleGap", "2.0", CVAR_FLOAT | CVAR_GUI | CVAR_ARCHIVE, "If there are multiple subtitles on screen, the size of the gap between them.");
idCVar gui_subtitleFadeTime("gui_subtitleFadeTime", "0.3", CVAR_FLOAT | CVAR_GUI | CVAR_ARCHIVE, "Duration of subtitle fade-out time.");
idCVar gui_subtitleTransitionTime("gui_subtitleTransitionTime", "0.2f", CVAR_FLOAT | CVAR_GUI | CVAR_ARCHIVE, "time subtitles move into target Y positon");

idRenderWorld *				gameRenderWorld = NULL;		// all drawing is done to this world
idSoundWorld *				gameSoundWorld = NULL;		// all audio goes to this world

static gameExport_t			gameExport;

// global animation lib
idAnimManager				animationLib;

// the rest of the engine will only reference the "game" variable, while all local aspects stay hidden
// blendo eric combined game projects/dll means this is likely always active
#ifndef __DOOM_DLL__
idGameLocal					gameLocal;
idGame *					game = &gameLocal;	// statically pointed at an idGameLocal
#endif

const char *idGameLocal::sufaceTypeNames[ MAX_SURFACE_TYPES ] = {
	"none",	"metal", "stone", "flesh", "wood", "cardboard", "liquid", "glass", "plastic",
	"ricochet", "fabric", "tile", "surftype12", "surftype13", "surftype14", "surftype15"
};


//BC vo manager
idVOManager				voManager;




#ifdef _D3XP
// List of all defs used by the player that will stay on the fast timeline
static const char* fastEntityList[] = {
		/*"player_doommarine",
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
		"weapon_grabber",*/
		NULL };
#endif
/*
===========
GetGameAPI
============
*/
extern "C" ID_GAME_API gameExport_t *GetGameAPI( gameImport_t *import ) {

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
	hudTime = 0;
	vacuumAreaNum = 0;
	mapFileName.Clear();
	mapFile = NULL;
	spawnCount = INITIAL_SPAWN_COUNT;
	mapSpawnCount = 0;
	camera = NULL;
	aasList.Clear();
	aasNames.Clear();
	//lastAIAlertEntity = NULL;
	//lastAIAlertTime = 0;
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

	//BC init bc reset
	lastSuspiciousNoiseTime = 0;
	suspiciousNoisePos = vec3_zero;
	suspiciousNoiseRadius = 0;
	menuSlowmo = false;
	lastSuspiciousNoisePriority = 0;
	
	bafflerEntities.Clear();
	interestEntities.Clear();
	idletaskEntities.Clear();
	searchnodeEntities.Clear();
	confinedEntities.Clear();
	ventdoorEntities.Clear();
	repairEntities.Clear();
	hatchEntities.Clear();
	turretEntities.Clear();
	petEntities.Clear();
	securitycameraEntities.Clear();
	airlockEntities.Clear();


	lastEditstate = false;

	gameEdit->editmenuActive = false;	

	lastExpensiveObservationCallTime = 0;	

	nextGrenadeTime = 0;
	nextPetTime = 0;

	//reset subtitles.
	ResetSubtitleSlots();

	loadCount = 0;

	lastDebugNoteIndex = -1;

	metaEnt = nullptr;

	eventlogGuiList = nullptr;
	eventLogAlerts = nullptr;
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

	Printf( "----- Initializing Game -----\n" );
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

	cmdSystem->AddCommand( "gamepad_rumble", GamePadFXSystem::Rumble_f, CMD_FL_SYSTEM|CMD_FL_GAME, "attempts add gamepad/controller rumble/vibrate, time excludes spin-up/down time" );
	cmdSystem->AddCommand( "gamepad_light", GamePadFXSystem::Light_f, CMD_FL_SYSTEM|CMD_FL_GAME, "attempts to add controller led light, excludes min light" );


	cmdSystem->AddCommand( "testfunc", idGameLocal::TestTimedFunc_f, CMD_FL_SYSTEM|CMD_FL_GAME, "developer only test func for random code" );

	Clear();

	LoadingContext loading = GetLoadingContext();

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


	{
		controllerButtonDicts[CT_XBOX360].Clear();
		idParser buttonParser("misc/controller_360.dict", LEXFL_ALLOWMULTICHARLITERALS);
		controllerButtonDicts[CT_XBOX360].Parse(buttonParser);
	}
	{
		controllerButtonDicts[CT_XBOXONE].Clear();
		idParser buttonParser("misc/controller_xboxone.dict", LEXFL_ALLOWMULTICHARLITERALS);
		controllerButtonDicts[CT_XBOXONE].Parse(buttonParser);
	}
	{
		controllerButtonDicts[CT_PS4].Clear();
		idParser buttonParser("misc/controller_ps4.dict", LEXFL_ALLOWMULTICHARLITERALS);
		controllerButtonDicts[CT_PS4].Parse(buttonParser);
	}
	{
		controllerButtonDicts[CT_PS5].Clear();
		idParser buttonParser("misc/controller_ps5.dict", LEXFL_ALLOWMULTICHARLITERALS);
		controllerButtonDicts[CT_PS5].Parse(buttonParser);
	}
	{
		controllerButtonDicts[CT_SWITCHPRO].Clear();
		idParser buttonParser("misc/controller_switch.dict", LEXFL_ALLOWMULTICHARLITERALS);
		controllerButtonDicts[CT_SWITCHPRO].Parse(buttonParser);
	}
	{
		controllerButtonDicts[CT_STEAMDECK].Clear();
		idParser buttonParser("misc/controller_steamdeck.dict", LEXFL_ALLOWMULTICHARLITERALS);
		controllerButtonDicts[CT_STEAMDECK].Parse(buttonParser);
	}
	{
		mouseButtonDict.Clear();
		idParser buttonParser("misc/mouse.dict", LEXFL_ALLOWMULTICHARLITERALS);
		mouseButtonDict.Parse(buttonParser);
	}
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

	Printf( "----- Game Shutdown -----\n" );

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

	if (g_flushSave.GetBool( ) == true || sg_debugchecks.GetBool() ) {
		// force flushing with each write... for tracking down
		// save game bugs.
		f->ForceFlush();
	}

	idSaveGame savegame = idSaveGame::Begin( f, SAVEGAME_VERSION, MAX_GENTITIES, idThread::GetThreads().Num() );

	savegame.WriteCheckString("post start");

	savegame.WriteInt(numClients);  // int numClients
	for (int i = 0; i < numClients; i++) {
		savegame.WriteDict(&userInfo[i]);  // idDict userInfo[MAX_CLIENTS]
		savegame.WriteDict(&persistentPlayerInfo[i]);  // idDict persistentPlayerInfo[MAX_CLIENTS]
	}

	savegame.WriteDict(&persistentLevelInfo); // idDict persistentLevelInfo

	savegame.WriteCheckString("post persistent");

	// SM: If this is an autosave, bail out immediately after persistent info is saved,
	// since that's all we need to write
	if (idStr::FindText(f->GetName(), "autosave") != -1) {
		return;
	}

	// go through all entities and threads and add them to the object list
	for( int idx = 0; idx < MAX_GENTITIES; idx++ ) { // idEntity * entities[MAX_GENTITIES] part 1
		idEntity * ent = entities[idx];

		if ( ent && !ent->IsRemoved() ) {
			if ( ent->GetTeamMaster() && ent->GetTeamMaster() != ent ) {
				continue; // will get added later
			}
			for ( idEntity *link = ent; link != NULL; link = link->GetNextTeamEntity() ) {
				int linkIdx = -1;
				for ( int idxFind = 0; idxFind < MAX_GENTITIES; idxFind++ ) {
					if ( entities[idxFind] == link ) {
						linkIdx = idxFind;
						break;
					}
				}
				savegame.AddObjectToList( link, linkIdx);
			}
		} else {
			savegame.AddObjectToList( nullptr, idx );
		}
	}

	idList<idThread *> threads = idThread::GetThreads();

	for( int i = 0; i < threads.Num(); i++ ) {
		savegame.AddObjectToList( threads[i], i+MAX_GENTITIES );
	}

	// write out complete object list
	savegame.WriteObjectListTypes();

	savegame.WriteCheckString("post objects");

	program.Save( &savegame ); // idProgram program

	savegame.WriteCheckString("post program");

	savegame.WriteInt( g_skill.GetInteger() );

	savegame.WriteDict( &serverInfo );  // idDict serverInfo

	savegame.WriteCheckString("post server");

	for( int i = 0; i < numClients; i++ ) {
		savegame.WriteUsercmd( usercmds[ i ] );  // usercmd_t usercmds[MAX_CLIENTS]
	}

	savegame.WriteCheckString("post user");

	for( int i = 0; i < MAX_GENTITIES; i++ ) {
		if (entities[i] && !entities[i]->IsRemoved()) { // blendo eric: checked for removed objs
			savegame.WriteObject( entities[ i ] ); // idEntity * entities[MAX_GENTITIES] part 2
			savegame.WriteInt( spawnIds[ i ] ); // int spawnIds[MAX_GENTITIES]
		} else {
			savegame.WriteObject( nullptr );
			savegame.WriteInt( -1 );
		}
	}

	savegame.WriteInt( firstFreeIndex );  // int firstFreeIndex
	savegame.WriteInt( num_entities );  // int num_entities
	// idHashIndex entityHash // blendo eric: gen'd on entity restore

	savegame.WriteCheckString("post entities");
	// enityHash is restored by idEntity::Restore setting the entity name.

	savegame.WriteObject( world ); // idWorldspawn * world

	savegame.WriteCheckString("post world");

	savegame.WriteInt( spawnedEntities.Num() );  // idLinkList<idEntity> spawnedEntities
	for( idEntity *ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		savegame.WriteObject( ent );
	}
	savegame.WriteCheckString("post spawned");

	savegame.WriteInt( activeEntities.Num() );  // idLinkList<idEntity> activeEntities
	for( idEntity *ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() ) {
		savegame.WriteObject( ent );
	}

	// idLinkList<idEntity> aimAssistEntities // blendo eric: gen'd by actors

	savegame.WriteCheckString("post active");

	savegame.WriteInt( numEntitiesToDeactivate );  // int numEntitiesToDeactivate
	savegame.WriteBool( sortPushers );  // bool sortPushers
	savegame.WriteBool( sortTeamMasters );  // bool sortTeamMasters

	for( int i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ ) { // float globalShaderParms[ MAX_GLOBAL_SHADER_PARMS ]
		savegame.WriteFloat( globalShaderParms[ i ] );
	}

	savegame.WriteCheckString("post shader");

	savegame.WriteInt( random.GetSeed() ); // idRandom random
	savegame.WriteObject( frameCommandThread ); // idThread * frameCommandThread


	//idClip clip; // regened on mapload
	//idPush push; // stack?
	//idPVS pvs; // map load

	testmodel = NULL; // idTestModel * testmodel // blendo eric: spawned by command
	testFx = NULL; // idEntityFx * testFx // blendo eric: spawned by command

	savegame.WriteString( sessionCommand ); // idString sessionCommand

	savegame.WriteCheckString("post commands");

	// FIXME: save smoke particles

	savegame.WriteInt( cinematicSkipTime ); // int cinematicSkipTime
	savegame.WriteInt( cinematicStopTime ); // int cinematicStopTime
	savegame.WriteInt( cinematicMaxSkipTime ); // int cinematicMaxSkipTime
	savegame.WriteBool( inCinematic ); // bool inCinematic
	savegame.WriteBool( skipCinematic ); // bool skipCinematic

	savegame.WriteBool( isMultiplayer ); // bool isMultiplayer
	savegame.WriteInt( gameType ); // gameType_t gameType


	savegame.WriteCheckString("pre frametime");

	savegame.WriteInt( framenum ); // int framenum
	savegame.WriteInt( previousTime ); // int previousTime
	savegame.WriteInt( time ); // int time

	savegame.WriteInt( msec ); // int msec

	savegame.WriteInt( hudTime ); // int hudTime

	savegame.WriteCheckString("post frametime");

	savegame.WriteInt( vacuumAreaNum ); // int vacuumAreaNum

	savegame.WriteInt( entityDefBits ); // int entityDefBits
	savegame.WriteBool( isServer ); // bool isServer
	savegame.WriteBool( isClient ); // bool isClient

	savegame.WriteInt( localClientNum ) ;// int localClientNum

	// idLinkList<idEntity> snapshotEntities // snapshotEntities is used for multiplayer only

	savegame.WriteInt( realClientTime ); // int realClientTime
	savegame.WriteBool( isNewFrame ); // bool isNewFrame
	savegame.WriteFloat( clientSmoothing ); // float clientSmoothing


	//savefile->WriteObject( lastGUIEnt ); // idEntityPtr<idEntity> lastGUIEnt // debug only?
	//savefile->WriteInt( lastGUI ); // int lastGUI


	portalSkyEnt.Save( &savegame ); // idEntityPtr<idEntity> portalSkyEnt
	savegame.WriteBool( portalSkyActive ); // bool portalSkyActive


	savegame.WriteCheckString("post portal");

	fast.Save( &savegame ); // timeState_t fast
	slow.Save( &savegame ); // timeState_t slow

	savegame.WriteInt( slowmoState ); // slowmoState_t slowmoState
	savegame.WriteFloat( slowmoMsec ); // float slowmoMsec

	// savefile->WriteInt( soundSlowmoHandle ); // int soundSlowmoHandle // blendo eric: sounds aren't restored?
	// savefile->WriteInt( soundSlowmoHandle ); // int soundSlowmoHandle // blendo eric: sounds aren't restored?
	// soundSlowmoActive = false;  // bool soundSlowmoActive // blendo eric: restored

	savegame.WriteBool( quickSlowmoReset ); // bool quickSlowmoReset

	savegame.WriteVec3( suspiciousNoisePos ); // idVec3 suspiciousNoisePos
	savegame.WriteInt( suspiciousNoiseRadius ); // int suspiciousNoiseRadius
	savegame.WriteInt( lastSuspiciousNoiseTime ); // int lastSuspiciousNoiseTime
	savegame.WriteInt( lastSuspiciousNoisePriority ); // int lastSuspiciousNoisePriority


	savegame.WriteObject( metaEnt ); // idEntityPtr<idEntity> metaEnt // blendo eric: set in player, but can be saved here

	savegame.WriteInt( lastExpensiveObservationCallTime ); // int lastExpensiveObservationCallTime


	// blendo eric: these should be linked upon entity creation (not spawn!)

	// idLinkList<idEntity> bafflerEntities
	// idLinkList<idEntity> interestEntities
	// idLinkList<idEntity> searchnodeEntities
	// idLinkList<idEntity> confinedEntities
	// idLinkList<idEntity> ventdoorEntities
	// idLinkList<idEntity> repairEntities
	// idLinkList<idEntity> hatchEntities
	// idLinkList<idEntity> idletaskEntities
	// idLinkList<idEntity> turretEntities
	// idLinkList<idEntity> petEntities
	// idLinkList<idEntity> securitycameraEntities
	// idLinkList<idEntity> airlockEntities
	// idLinkList<idEntity> catfriendsEntities
	// idLinkList<idEntity> landmineEntities
	// idLinkList<idEntity> skullsaverEntities
	// idLinkList<idEntity> catcageEntities
	// idLinkList<idEntity> spacenudgeEntities
	// idLinkList<idEntity> memorypalaceEntities
	// idLinkList<idEntity> dynatipEntities
	// idLinkList<idEntity> windowshutterEntities
	// idLinkList<idEntity> electricalboxEntities
	// idLinkList<idEntity> spectatenodeEntities


	// idStaticList<idEntity *, MAX_GENTITIES> repairpatrolEntities
	// idStaticList<idEntity *, MAX_GENTITIES> upgradecargoEntities
	// idStaticList<idEntity *, MAX_GENTITIES> healthstationEntities
	// idStaticList<idEntity *, MAX_GENTITIES> trashexitEntities

	savegame.WriteBool( menuPause ); // bool menuPause
	savegame.WriteBool( spectatePause ); // bool spectatePause
	savegame.WriteBool( requestPauseMenu ); // bool requestPauseMenu

	savegame.WriteInt( nextGrenadeTime ); // int nextGrenadeTime
	savegame.WriteInt( nextPetTime ); // int nextPetTime


	// blendo eric: this inited from file
	//savegame.WriteBool( eventlogGuiList != nullptr ); // idListGUI * eventlogGuiList
	//if (eventlogGuiList)
	//{
	//	eventlogGuiList->Save(&savegame);
	//}

	// idList<eventlog_t> eventLogList // blendo eric: TODO this should get synced with eventlogGuiList, but isn't?

	// idVOManager voManager // unnecessary

	// int lastDebugNoteIndex // debug only

	// idDict mouseButtonDict // regend
	// idDict controllerButtonDicts[CT_NUM] // regened

	// idString mapFileName // regened
	// idMapFile * mapFile // regened


	savegame.WriteBool( mapCycleLoaded ); // bool mapCycleLoaded
	savegame.WriteInt( spawnCount ); // int spawnCount
	// int mapSpawnCount // regened

	savegame.WriteCheckString("pre areas");

	if ( !locationEntities ) { // idLocationEntity ** locationEntities
		savegame.WriteInt( 0 );
	} else {
		savegame.WriteInt( gameRenderWorld->NumAreas() );
		for( int i = 0; i < gameRenderWorld->NumAreas(); i++ ) {
			savegame.WriteObject( locationEntities[ i ] );
		}
	}

	savegame.WriteCheckString("post areas");

	savegame.WriteObject( camera ); // idCamera * camera

	savegame.WriteMaterial( globalMaterial ); // const idMaterial * globalMaterial

	//lastAIAlertEntity.Save( &savegame );
	//savegame.WriteInt( lastAIAlertTime );

	// idList<idAAS *> aasList // regened
	// idStrList aasNames // regened

	savegame.WriteDict( &spawnArgs ); // idDict spawnArgs


	savegame.WriteCheckString("pre player");


	savegame.WriteInt( playerPVS.i ); // pvsHandle_t playerPVS
	savegame.WriteInt( playerPVS.h );
	savegame.WriteInt( playerConnectedAreas.i ); // pvsHandle_t playerConnectedAreas
	savegame.WriteInt( playerConnectedAreas.h );


	savegame.WriteCheckString("post player");

	savegame.WriteVec3( gravity ); // idVec3 gravity

	// gameState_t gamestate // blendo eric: set after restore

	savegame.WriteBool( influenceActive ); // bool influenceActive
	savegame.WriteInt( nextGibTime ); // int nextGibTime

	// network only
	//	idList<int> clientDeclRemap[MAX_CLIENTS][DECL_MAX_TYPES]
	// entityState_t * clientEntityStates[MAX_CLIENTS][MAX_GENTITIES]
	// int clientPVS[MAX_CLIENTS][ENTITY_PVS_SIZE]
	// snapshot_t * clientSnapshots[MAX_CLIENTS]
	// idBlockAlloc<entityState_t,256> entityStateAllocator
	// idBlockAlloc<snapshot_t,64> snapshotAllocator

	// more network?
	// idEventQueue eventQueue
	// idEventQueue savedEventQueue
	// idStaticList<spawnSpot_t, MAX_GENTITIES> spawnSpots
	// idStaticList<idEntity *, MAX_GENTITIES> initialSpots
	// int currentInitialSpot

	// idStaticList<spawnSpot_t, MAX_GENTITIES> teamSpawnSpots[2]
	// idStaticList<idEntity *, MAX_GENTITIES> teamInitialSpots[2]
	// int teamCurrentInitialSpot[2]

	// meta sys info
	// idDict newInfo
	// makingBuild
	// idStrList shakeSounds
	// byte lagometer[ LAGO_IMG_HEIGHT ][ LAGO_IMG_WIDTH ][ 4 ]

	savegame.WriteBool( menuSlowmo ); // bool menuSlowmo

	/// int lastEditstate // debug only

	savegame.WriteCheckString("pre events");

	// idFile* eventLogFile // regened

	// blendo eric: unnecessary to save out subtitles
	// int slotCircularIterator
	// idSubtitleItem subtitleItems[MAX_SUBTITLES];

	// int loadCount // meta?

	// write out pending events
	idEvent::Save( &savegame );

	savegame.WriteCheckString("post events");

	savegame.WriteObjectListData();
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
	gameSoundWorld->ClearSlowmoClients();
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
	// SM: Always set the seed to a random value even in single player
	random.SetSeed(randseed);
	//random.SetSeed( isMultiplayer ? randseed : 0 );

	camera			= NULL;
	world			= NULL;
	testmodel		= NULL;
	testFx			= NULL;

	//lastAIAlertEntity = NULL;
	//lastAIAlertTime = 0;

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

	SetGravityFromCVar();

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



	
	//BC THIS SECTION IS WHERE THINGS ARE RESET BETWEEN MAP RELOADS. Make sure this is zero so AI doesn't have leftover suspicious sounds from previous session
	suspiciousNoisePos = vec3_zero;

	menuSlowmo = false;
	slowmoState = SLOWMO_STATE_OFF;
	slowmoMsec = USERCMD_MSEC;
	soundSlowmoActive = false;
	cvarSystem->SetCVarBool("g_enableSlowmo", false);

	nextGrenadeTime = 0;
	nextPetTime = 0;

	//reset subtitles.
	ResetSubtitleSlots();

	cvarSystem->SetCVarBool("g_hideHudInternal", false);

	cmdSystem->BufferCommandText(CMD_EXEC_APPEND, "exec dev_mapstart.cfg\n");	 //BC run an exec file whenever a map is run.
}

/*
===================
idGameLocal::LocalMapRestart
===================
*/
void idGameLocal::LocalMapRestart( ) {
	int i, latchSpawnCount;

	Printf( "----- Game Map Restart -----\n" );

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
		gameSoundWorld->ClearSlowmoClients();
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

	Printf( "----- Game Map Init -----\n" );

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
}

/*
=================
idGameLocal::InitFromSaveGame
=================
*/
bool idGameLocal::InitFromSaveGame( const char *mapName, idRenderWorld *renderWorld, idSoundWorld *soundWorld, idFile *saveGameFile ) {
	if ( mapFileName.Length() ) {
		MapShutdown();
	}

	Printf( "----- Game Map Init SaveGame -----\n" );

	gamestate = GAMESTATE_STARTUP;

	gameRenderWorld = renderWorld;
	gameSoundWorld = soundWorld;

	idRestoreGame savegame = idRestoreGame::Begin( saveGameFile );

	if (!savegame.ReadCheckString("post start")) { return false; }

	savegame.ReadInt(numClients);   // int numClients
	int numClientsFromSave = numClients;
	for (int i = 0; i < numClients; i++) {
		savegame.ReadDict(&userInfo[i]); // idDict userInfo[MAX_CLIENTS]
		savegame.ReadDict(&persistentPlayerInfo[i]); // idDict persistentPlayerInfo[MAX_CLIENTS]
	}

	savegame.ReadDict(&persistentLevelInfo); // persistentLevelInfo
	if (!savegame.ReadCheckString("post persistent")) { return false; }

	// SM: If this is an autosave, bail out immediately after persistent info is loaded
	// to force full level load with updated persistent info
	if (savegame.GetFileName().Find("autosave") != -1) {
		return false;
	}

	// SM: If the save game version is invalidated, also force full reload
	if (session->GetSaveGameVersion() <= SAVEGAME_VERSION_INVALID) {
		return false;
	}

	// Create the list of all objects in the game
	savegame.ReadAndCreateObjectsListTypes(); // idEntity * entities[MAX_GENTITIES] part 1

	if (!savegame.ReadCheckString("post objects")) { return false; }

	// Load the idProgram, also checking to make sure scripting hasn't changed since the savegame
	bool programOK = program.Restore(&savegame);  // idProgram program
	if ( !programOK ) {

		// Abort the load process, and let the session know so that it can restart the level
		// with the player persistent data.
		savegame.DeleteObjects();
		program.Restart();

		gameLocal.Error("save game script program did not match\n\nAs a workaround fix, please:\n1. Type: disconnect\n2. Load a savegame that has the word 'AutoSave' in its name.");

		return false;
	}

	if (!savegame.ReadCheckString("post program")) { return false; }

	// load the map needed for this savegame
	LoadMap( mapName, 0 );

	int num;
	savegame.ReadInt( num );
	g_skill.SetInteger( num );

	// precache the player
	FindEntityDef( "player_doommarine", false );

	// precache any media specified in the map
	for ( int i = 0; i < mapFile->GetNumEntities(); i++ ) {
		idMapEntity *mapEnt = mapFile->GetEntity( i );

		if ( !InhibitEntitySpawn( mapEnt->epairs ) ) {
			CacheDictionaryMedia( &mapEnt->epairs );
			const char *classname;
			if ( mapEnt->epairs.GetString( "classname", "", &classname ) ) {
				FindEntityDef( classname, false );
			}
		}
	}

	idDict tempDict;
	savegame.ReadDict( &tempDict );   // idDict serverInfo
	SetServerInfo( tempDict );

	if (!savegame.ReadCheckString("post server")) { return false; }

	numClients = numClientsFromSave; // SM: Need to restore this because map change nixed it
	for( int i = 0; i < numClients; i++ ) {
		savegame.ReadUsercmd( usercmds[ i ] ); // usercmd_t usercmds[MAX_CLIENTS]
	}

	if (!savegame.ReadCheckString("post user")) { return false; }

	for (int i = 0; i < MAX_GENTITIES; i++) {
		savegame.ReadObject(reinterpret_cast<idClass*&>(entities[i]));  // idEntity * entities[MAX_GENTITIES] part 2
		savegame.ReadInt(spawnIds[i]); // int spawnIds[MAX_GENTITIES]

		// restore the entityNumber
		if (entities[i] != NULL) {
			entities[i]->entityNumber = i;
		} else if( spawnIds[ i ] != -1 ) {
			Warning( "idGameLocal::InitFromSaveGame() null entity %d had incorrect spawn type", i);
			spawnIds[ i ] = -1;
		}
	}

	savegame.ReadInt( firstFreeIndex ); // int firstFreeIndex
	savegame.ReadInt( num_entities ); // int num_entities

	if (!savegame.ReadCheckString("post entities")) { return false; }

	// idHashIndex entityHash // enityHash is restored by idEntity::Restore setting the entity name.

	savegame.ReadObject( reinterpret_cast<idClass *&>( world ) );  // idWorldspawn * world

	if (!savegame.ReadCheckString("post world")) { return false; }

	assert(spawnedEntities.Num() == 0);
	savegame.ReadInt( num );  // idLinkList<idEntity> spawnedEntities
	for( int i = 0; i < num; i++ ) { 
		idEntity* ent = nullptr;
		savegame.ReadObject( reinterpret_cast<idClass *&>( ent ) );
		assert( ent );
		if ( ent ) {
			ent->spawnNode.AddToEnd(spawnedEntities);
		}
	}

	if (!savegame.ReadCheckString("post spawned")) { return false; }

	assert(activeEntities.Num() == 0);
	savegame.ReadInt( num );  // idLinkList<idEntity> activeEntities
	for( int i = 0; i < num; i++ ) {
		idEntity* ent = nullptr;
		savegame.ReadObject( reinterpret_cast<idClass *&>( ent ) );
		assert( ent );
		if ( ent ) {
			ent->activeNode.AddToEnd( activeEntities );
		}
	}

	// idLinkList<idEntity> aimAssistEntities // blendo eric: gen'd by actors

	if (!savegame.ReadCheckString("post active")) { return false; }

	savegame.ReadInt( numEntitiesToDeactivate ); // int numEntitiesToDeactivate
	savegame.ReadBool( sortPushers ); // bool sortPushers
	savegame.ReadBool( sortTeamMasters ); // bool sortTeamMasters

	for( int i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ ) { // // float globalShaderParms[ MAX_GLOBAL_SHADER_PARMS ]
		savegame.ReadFloat( globalShaderParms[ i ] );
	}

	if (!savegame.ReadCheckString("post shader")) { return false; }

	savegame.ReadInt( num );
	random.SetSeed( num ); // idRandom random

	savegame.ReadObject( reinterpret_cast<idClass *&>( frameCommandThread ) ); // idThread * frameCommandThread

	//idClip clip; // regened on mapload
	//idPush push; // stack?
	//idPVS pvs; // map load

	testmodel = NULL; // idTestModel * testmodel // blendo eric: spawned by command
	testFx = NULL; // idEntityFx * testFx // blendo eric: spawned by command

	savegame.ReadString( sessionCommand ); // idString sessionCommand

	if (!savegame.ReadCheckString("post commands")) { return false; }

	// FIXME: save smoke particles // blendo eric: probably isn't necessary to restore smoke

	savegame.ReadInt( cinematicSkipTime ); // int cinematicSkipTime
	savegame.ReadInt( cinematicStopTime ); // int cinematicStopTime
	savegame.ReadInt( cinematicMaxSkipTime ); // int cinematicMaxSkipTime
	savegame.ReadBool( inCinematic ); // bool inCinematic
	savegame.ReadBool( skipCinematic ); // bool skipCinematic

	savegame.ReadBool( isMultiplayer ); // bool isMultiplayer
	savegame.ReadInt( (int &)gameType ); // gameType_t gameType


	if (!savegame.ReadCheckString("pre frametime")) { return false; }

	savegame.ReadInt( framenum ); // int framenum
	savegame.ReadInt( previousTime ); // int previousTime
	savegame.ReadInt( time ); // int time


	savegame.ReadInt( msec ); // int msec


	savegame.ReadInt( hudTime ); // int hudTime // blendo eric: possibly error prone?


	if (!savegame.ReadCheckString("post frametime")) { return false; }

	savegame.ReadInt( vacuumAreaNum ); // int vacuumAreaNum

	savegame.ReadInt( entityDefBits ); // int entityDefBits
	savegame.ReadBool( isServer ); // bool isServer
	savegame.ReadBool( isClient ); // bool isClient

	savegame.ReadInt( localClientNum );// int localClientNum

	// idLinkList<idEntity> snapshotEntities // snapshotEntities is used for multiplayer only

	savegame.ReadInt( realClientTime ); // int realClientTime
	savegame.ReadBool( isNewFrame );// bool isNewFrame
	savegame.ReadFloat( clientSmoothing ); // float clientSmoothing

	//savefile->WriteObject( lastGUIEnt ); // idEntityPtr<idEntity> lastGUIEnt  // debug only?
	//savefile->WriteInt( lastGUI ); // int lastGUI

	portalSkyEnt.Restore( &savegame ); // idEntityPtr<idEntity> portalSkyEnt
	savegame.ReadBool( portalSkyActive ); // bool portalSkyActive


	if (!savegame.ReadCheckString("post portal")) { return false; }

	fast.Restore( &savegame ); // timeState_t fast
	slow.Restore( &savegame ); // timeState_t slow

	int blah;
	savegame.ReadInt( blah );  // slowmoState_t slowmoState
	slowmoState = (slowmoState_t)blah;

	savegame.ReadFloat( slowmoMsec ); // float slowmoMsec

	// savefile->WriteInt( soundSlowmoHandle ); // int soundSlowmoHandle // blendo eric: sounds aren't restored?
	// savefile->WriteInt( soundSlowmoHandle ); // int soundSlowmoHandle // blendo eric: sounds aren't restored?
	soundSlowmoActive = false;  // bool soundSlowmoActive // blendo eric: should cause regen of slowmo sound

	savegame.ReadBool( quickSlowmoReset ); // bool quickSlowmoReset

	savegame.ReadVec3( suspiciousNoisePos ); // idVec3 suspiciousNoisePos
	savegame.ReadInt( suspiciousNoiseRadius ); // int suspiciousNoiseRadius
	savegame.ReadInt( lastSuspiciousNoiseTime ); // int lastSuspiciousNoiseTime
	savegame.ReadInt( lastSuspiciousNoisePriority ); // int lastSuspiciousNoisePriority

	savegame.ReadObject( metaEnt ); // idEntityPtr<idEntity> metaEnt // blendo eric: set in player, but can be saved here

	savegame.ReadInt( lastExpensiveObservationCallTime ); // int lastExpensiveObservationCallTime

	// blendo eric: these should be linked upon entity creation (not spawn!)

	// idLinkList<idEntity> bafflerEntities
	// idLinkList<idEntity> interestEntities
	// idLinkList<idEntity> searchnodeEntities
	// idLinkList<idEntity> confinedEntities
	// idLinkList<idEntity> ventdoorEntities
	// idLinkList<idEntity> repairEntities
	// idLinkList<idEntity> hatchEntities
	// idLinkList<idEntity> idletaskEntities
	// idLinkList<idEntity> turretEntities
	// idLinkList<idEntity> petEntities
	// idLinkList<idEntity> securitycameraEntities
	// idLinkList<idEntity> airlockEntities
	// idLinkList<idEntity> catfriendsEntities
	// idLinkList<idEntity> landmineEntities
	// idLinkList<idEntity> skullsaverEntities
	// idLinkList<idEntity> catcageEntities
	// idLinkList<idEntity> spacenudgeEntities
	// idLinkList<idEntity> memorypalaceEntities
	// idLinkList<idEntity> dynatipEntities
	// idLinkList<idEntity> windowshutterEntities
	// idLinkList<idEntity> electricalboxEntities
	// idLinkList<idEntity> spectatenodeEntities


	// idStaticList<idEntity *, MAX_GENTITIES> repairpatrolEntities
	// idStaticList<idEntity *, MAX_GENTITIES> upgradecargoEntities
	// idStaticList<idEntity *, MAX_GENTITIES> healthstationEntities
	// idStaticList<idEntity *, MAX_GENTITIES> trashexitEntities


	savegame.ReadBool( menuPause ); // bool menuPause
	savegame.ReadBool( spectatePause ); // bool spectatePause
	savegame.ReadBool( requestPauseMenu ); // bool requestPauseMenu

	savegame.ReadInt( nextGrenadeTime ); // int nextGrenadeTime
	savegame.ReadInt( nextPetTime ); // int nextPetTime


	// blendo eric: this inited from file
	// idListGUI * eventlogGuiList

	// idList<eventlog_t> eventLogList // blendo eric: TODO this should get synced with eventlogGuiList, but isn't?

	// idVOManager voManager // unnecessary

	// int lastDebugNoteIndex // debug only

	// idDict mouseButtonDict // regend
	// idDict controllerButtonDicts[CT_NUM] // regened

	// idString mapFileName // regened
	// idMapFile * mapFile // regened

	savegame.ReadBool( mapCycleLoaded ); // bool mapCycleLoaded
	savegame.ReadInt( spawnCount ); // int spawnCount
	// int mapSpawnCount // regened

	if (!savegame.ReadCheckString("pre areas")) { return false; }

	savegame.ReadInt( num );
	if ( num ) {  // idLocationEntity ** locationEntities
		if ( num != gameRenderWorld->NumAreas() ) {
			savegame.Error( "idGameLocal::InitFromSaveGame: number of areas in map differs from save game." );
		}

		locationEntities = new idLocationEntity *[ num ];
		memset( locationEntities, 0, sizeof(idLocationEntity*)*num );
		for( int i = 0; i < num; i++ ) {
			savegame.ReadObject( reinterpret_cast<idClass *&>( locationEntities[ i ] ) );
		}
	}

	if (!savegame.ReadCheckString("post areas")) { return false; }

	savegame.ReadObject( reinterpret_cast<idClass *&>( camera ) ); // idCamera * camera

	savegame.ReadMaterial( globalMaterial ); // const idMaterial * globalMaterial

	//lastAIAlertEntity.Restore( &savegame );
	//savegame.ReadInt( lastAIAlertTime );

	savegame.ReadDict( &spawnArgs ); // idDict spawnArgs


	if (!savegame.ReadCheckString("pre player")) { return false; }

	savegame.ReadInt( playerPVS.i ); // pvsHandle_t playerPVS
	savegame.ReadInt( (int &)playerPVS.h );
	savegame.ReadInt( playerConnectedAreas.i ); // pvsHandle_t playerConnectedAreas
	savegame.ReadInt( (int &)playerConnectedAreas.h );

	if (!savegame.ReadCheckString("post player")) { return false; }

	savegame.ReadVec3( gravity ); // idVec3 gravity

	// gamestate is restored after restoring everything else

	savegame.ReadBool( influenceActive ); // bool influenceActive
	savegame.ReadInt( nextGibTime ); // int nextGibTime

	// network only
	//	idList<int> clientDeclRemap[MAX_CLIENTS][DECL_MAX_TYPES]
	// entityState_t * clientEntityStates[MAX_CLIENTS][MAX_GENTITIES]
	// int clientPVS[MAX_CLIENTS][ENTITY_PVS_SIZE]
	// snapshot_t * clientSnapshots[MAX_CLIENTS]
	// idBlockAlloc<entityState_t,256> entityStateAllocator
	// idBlockAlloc<snapshot_t,64> snapshotAllocator

	// more network?
	// idEventQueue eventQueue
	// idEventQueue savedEventQueue
	// idStaticList<spawnSpot_t, MAX_GENTITIES> spawnSpots
	// idStaticList<idEntity *, MAX_GENTITIES> initialSpots
	// int currentInitialSpot

	// idStaticList<spawnSpot_t, MAX_GENTITIES> teamSpawnSpots[2]
	// idStaticList<idEntity *, MAX_GENTITIES> teamInitialSpots[2]
	// int teamCurrentInitialSpot[2]

	// meta sys info
	// idDict newInfo
	// makingBuild
	// idStrList shakeSounds
	// byte lagometer[ LAGO_IMG_HEIGHT ][ LAGO_IMG_WIDTH ][ 4 ]

	savegame.ReadBool( menuSlowmo ); // bool menuSlowmo

	/// int lastEditstate // debug only


	if (!savegame.ReadCheckString("pre events")) { return false; }

	// idFile* eventLogFile // regened

	// blendo eric: unnecessary to save out subtitles
	// int slotCircularIterator
	// idSubtitleItem subtitleItems[MAX_SUBTITLES];

	// int loadCount

	// Read out pending events
	idEvent::Restore( &savegame );

	if (!savegame.ReadCheckString("post events")) { return false; }

	bool restoreOK = savegame.ReadAndRestoreObjectsListData();

	if(!restoreOK) {
		savegame.Error( "idGameLocal::InitFromSaveGame: restore objects failed" );
		return false;
	}

	// blendo eric: added for post setup, similar to spawn
	for (int idx = 0; idx < MAX_GENTITIES; idx++) {
		if (entities[idx] != NULL) {
			entities[idx]->PostSaveRestore( &savegame );
		}
	}

	mpGame.Reset();

	mpGame.Precache();

	// free up any unused animations
	animationLib.FlushUnusedAnims();

	gamestate = GAMESTATE_ACTIVE;

	return true;
}

/*
===========
idGameLocal::MapClear
===========
*/
void idGameLocal::MapClear( bool clearClients ) {
	int i;

	// blendo eric: clear all nodes
	//bafflerEntities.Clear();
	//interestEntities.Clear();
	//searchnodeEntities.Clear();
	//confinedEntities.Clear();
	//ventdoorEntities.Clear();
	//repairEntities.Clear();
	//hatchEntities.Clear();
	//idletaskEntities.Clear();
	//turretEntities.Clear();
	//petEntities.Clear();
	//securitycameraEntities.Clear();
	//airlockEntities.Clear();
	//catfriendsEntities.Clear();
	//landmineEntities.Clear();
	//skullsaverEntities.Clear();
	//catcageEntities.Clear();
	//spacenudgeEntities.Clear();
	//memorypalaceEntities.Clear();
	//dynatipEntities.Clear();
	//windowshutterEntities.Clear();
	//electricalboxEntities.Clear();
	//spectatenodeEntities.Clear();

	repairpatrolEntities.Clear();
	upgradecargoEntities.Clear();
	healthstationEntities.Clear();
	trashexitEntities.Clear();


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

	//This is stuff we initialize on every map load.

	
	menuPause = false;
	spectatePause = false;
	requestPauseMenu = false;

	eventLogAlerts = nullptr;
}

/*
===========
idGameLocal::MapShutdown
============
*/
void idGameLocal::MapShutdown( void ) {
	Printf( "----- Game Map Shutdown -----\n" );

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

	pvs.Shutdown();

	clip.Shutdown();
	idClipModel::ClearTraceModelCache();

	ShutdownAsyncNetwork();

	mapFileName.Clear();

	gameRenderWorld = NULL;
	gameSoundWorld = NULL;

	gamestate = GAMESTATE_NOMAP;

	if (eventLogFile)
	{
		CloseEventLogFile();
	}
	ShutdownEventLog();

	// Reenable blendo ambience if it's disabled
	renderSystem->SetUseBlendoAmbience( true );

	// Reset all cvars which were script-modified
	cvarSystem->ResetFlaggedVariables( CVAR_SCRIPTMODIFIED );
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

	if ( dict->GetString( "s_shader", "", &soundShaderName )
		 && dict->GetFloat( "s_shakes" ) != 0.0f )
	{
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
	if ( kv && kv->GetValue().Length() )
	{
		//BC sanity check
		if (kv->GetValue()[0] == '\0' || !idStr::Icmp(kv->GetValue(), " "))
		{
			const char *badsound = va("Empty s_shader value in '%s' entity at '%f %f %f'\n", dict->GetString("classname"), dict->GetVector("origin").x, dict->GetVector("origin").y, dict->GetVector("origin").z);			
			common->Warning(badsound);
		}

		declManager->FindType( DECL_SOUND, kv->GetValue() );
	}

	kv = dict->MatchPrefix( "snd", NULL );
	while( kv ) {
		if ( kv->GetValue().Length() > 1)
		{
			declManager->FindType( DECL_SOUND, kv->GetValue() );
		}
		kv = dict->MatchPrefix( "snd", kv );
	}


	kv = dict->MatchPrefix( "gui", NULL );
	while( kv ) {
		if ( kv->GetValue().Length() ) {
			if ( !idStr::Icmp( kv->GetKey(), "gui_noninteractive" )
				|| !idStr::Icmpn( kv->GetKey(), "gui_parm", 8 )
				|| !idStr::Icmp( kv->GetKey(), "gui_inventory" )

				//BC  don't precache these; these are gui events when the entity is repaired or broken
				|| !idStr::Icmp(kv->GetKey(), "gui_onbreak")
				|| !idStr::Icmp(kv->GetKey(), "gui_onrepaired")
				)
			{
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
	if (developer.GetBool())
	{
		Printf("SpawnPlayer: %i\n", clientNum);
	}

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
		// SW: Player is peeking, merge the PVS and connected areas of wherever we're peeking into.
		// We could potentially just *replace* the PVS with the area we're peeking into,
		// but this might have weird consequences.
		// (What happens if the player's PVS doesn't currently include the player?)
		if (player->peekObject.IsValid())
		{
			idEntityPtr<idEntity> ventPeekTarget = static_cast<idVentpeek*>(player->peekObject.GetEntity())->peekEnt;
			if (ventPeekTarget.IsValid())
			{
				// The engine supports a 'private camera' view which would normally make this automatic,
				// but in ventpeeks we're actually just moving the player's eyeball vector to a target entity.
				// So we'll need to grab hold of that instead.
				otherPVS = pvs.SetupCurrentPVS(ventPeekTarget.GetEntity()->GetPVSAreas(), ventPeekTarget.GetEntity()->GetNumPVSAreas());
				newPVS = pvs.MergeCurrentPVS(playerPVS, otherPVS);
				pvs.FreeCurrentPVS(playerPVS);
				playerPVS = newPVS;

				newPVS = pvs.MergeCurrentPVS(playerConnectedAreas, otherPVS);
				pvs.FreeCurrentPVS(playerConnectedAreas);
				pvs.FreeCurrentPVS(otherPVS);
				playerConnectedAreas = newPVS;
			}
			
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


/* blendo eric
================
idGameLocal::InPlayerAreaRecalculated

  will recalculate playerpvsarea if outside of think
================
*/
bool idGameLocal::InPlayerAreasRecalculated(idEntity* ent) {
	bool pvsRecreate = playerConnectedAreas.i == -1;
	if (pvsRecreate) {
		SetupPlayerPVS();
	}
	if (playerConnectedAreas.i == -1) {
		pvsRecreate = true;
		FreePlayerPVS();
		return false;
	}
	bool found = pvs.InCurrentPVS(playerConnectedAreas, ent->GetPVSAreas(), ent->GetNumPVSAreas());
	if (pvsRecreate) {
		FreePlayerPVS();
	}
	return found;
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

	// SM: Make sure players are always first in the active entity list
	// this fixes carryables sometimes jittering
	for ( ent = activeEntities.Next(); ent != NULL; ent = next_ent ) {
		next_ent = ent->activeNode.Next();
		if ( ent->IsType( idPlayer::Type ) && ent->GetTeamMaster() == ent ) {
			ent->activeNode.Remove();
			ent->activeNode.AddToFront( activeEntities );
		}
	}
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

		if ( !ent->IsFrozen() )
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

	// SM: This is intentionally static because we DO NOT want to save/load this since
	// common->FrameTime() is session-specific. So instead we need to know the diff between
	// previous frame time and the current, and add that to hudTime (which is save/loaded)
	static int s_prevFrameTime = common->FrameTime();
	// SW: HUD time runs independent of slow-mo or even stopped time, so we can still update it even when the player is in the context menu, etc
	int currentFrameTime = common->FrameTime();
	hudTime += currentFrameTime - s_prevFrameTime;
	s_prevFrameTime = currentFrameTime;

#ifdef _D3XP
	ComputeSlowMsec();

	slow.Get( time, previousTime, msec, framenum, realClientTime );
	msec = slowmoMsec;
#endif

	if ( !isMultiplayer && (g_stopTime.GetBool()  || gameLocal.menuPause || (gameLocal.spectatePause && slowmoState == SLOWMO_STATE_OFF) ))
	{
		// clear any debug lines from a previous frame
		gameRenderWorld->DebugClearLines( time + 1 );

		// set the user commands for this frame
		memcpy( usercmds, clientCmds, numClients * sizeof( usercmds[ 0 ] ) );

		if ( player && !gameLocal.InLoadingContext() )
		{
			SetupPlayerPVS(); // setup pvs so portalsky is still functional even when paused / end mission
			player->Think(); //BC this is what allows the player to think during pause sequences (including spectatorcam)
			FreePlayerPVS();
		}
	}
	else do
	{
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

		GamePadFXSystem::GetSys().Update(msec);

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
				if ( !ent->IsFrozen() )
					ent->Think();
				timer_singlethink.Stop();
				ms = timer_singlethink.Milliseconds();
				if ( ms >= g_timeentities.GetFloat() ) {
					Printf( "%d: entity '%s': %f ms\n", time, ent->name.c_str(), ms );
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
					if ( !ent->IsFrozen() )
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
					if ( !ent->IsFrozen() )
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
			Printf( "game %d: all:%u th:%u ev:%u %d ents \n",
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
			// Combat Intensity
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

	if ((gameLocal.time > this->lastSuspiciousNoiseTime + 100) && suspiciousNoiseRadius > 0)
	{
		suspiciousNoiseRadius = 0;
		suspiciousNoisePos = vec3_zero;
	}

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
	case -1 :
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

	//BC if in edit mode. Make ESC exit editmode.
	if (g_editEntityMode.GetInteger() > 0)
	{
		if (gameEdit->editmenuActive)
		{
			gameEdit->editmenuActive = false;
			gameEdit->lighteditMenu->HandleNamedEvent("closeEditmenu");
		}
		else
		{
			cvarSystem->SetCVarInteger("g_editEntityMode", 0);
		}
		return ESC_IGNORE;
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

//BC return just the map name without file extension or filepath
idStr idGameLocal::GetMapNameStripped(void) const {
	idStr strippedName = mapFileName;
	strippedName.StripPath();
	strippedName.StripFileExtension();
	strippedName.ToLower();

    return strippedName;
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

	if (g_showEntityNumber.GetInteger() >= 0)
	{
		idMat3		axis = player->viewAngles.ToMat3();
		idVec3		up = axis[2] * 5.0f;
		idBounds	viewTextBounds(origin);
		idBounds	viewBounds(origin);

		viewTextBounds.ExpandSelf(128.0f);
		viewBounds.ExpandSelf(512.0f);

		for (ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
		{
			if (ent->entityNumber != g_showEntityNumber.GetInteger())
				continue;

			DrawEntityDebug(ent, viewTextBounds, axis, up);
		}
	}

	if (g_showParticleFX.GetBool())
	{
		idBounds	viewTextBounds(origin);
		idBounds	viewBounds(origin);
		idMat3		axis = player->viewAngles.ToMat3();

		viewTextBounds.ExpandSelf(128.0f);
		viewBounds.ExpandSelf(512.0f);

		for (ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
		{
			if (ent == world || !ent->IsType(idFuncEmitter::Type))
			{
				continue;
			}

			// skip if the entity is very far away
			if (!viewBounds.IntersectsBounds(ent->GetPhysics()->GetAbsBounds())) {
				continue;
			}

			const idBounds &entBounds = ent->GetPhysics()->GetAbsBounds();
			if (viewTextBounds.IntersectsBounds(entBounds))
			{
				#define	BOUNDSIZE .4f
				gameRenderWorld->DebugBounds(colorGreen, idBounds(idVec3(-BOUNDSIZE, -BOUNDSIZE, -BOUNDSIZE), idVec3(BOUNDSIZE, BOUNDSIZE, BOUNDSIZE)), ent->GetPhysics()->GetOrigin(), 10);
				
				//arrow.
				gameRenderWorld->DebugArrow(colorGreen, ent->GetPhysics()->GetOrigin(), ent->GetPhysics()->GetOrigin() + (ent->GetPhysics()->GetAxis().ToAngles().ToForward() * 3), 1, 10);

				gameRenderWorld->DrawText(va("%s", ent->spawnArgs.GetString("model")), entBounds.GetCenter() + idVec3(0,0,3), 0.15f, colorWhite, axis, 1);
			}
		}
	}

	if (g_showEntityHealth.GetBool())
	{
		idBounds	viewTextBounds(origin);
		idBounds	viewBounds(origin);
		idMat3		axis = player->viewAngles.ToMat3();

		viewTextBounds.ExpandSelf(128.0f);
		viewBounds.ExpandSelf(512.0f);

		for (ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
		{
			// don't draw the worldspawn
			if (ent == world)
			{
				continue;
			}

			// skip if the entity is very far away
			if (!viewBounds.IntersectsBounds(ent->GetPhysics()->GetAbsBounds())) {
				continue;
			}

			const idBounds &entBounds = ent->GetPhysics()->GetAbsBounds();
			if (viewTextBounds.IntersectsBounds(entBounds))
			{
				gameRenderWorld->DrawText(va("%d",(int)ent->health), entBounds.GetCenter(), 0.1f, colorWhite, axis, 1);
			}
		}
	}

	// show entity detail larger than 1 changes info distance
	if (g_showEntityDetail.GetInteger() > 1) {
		idMat3		axis = player->viewAngles.ToMat3();
		idVec3		up = axis[2] * 5.0f;
		idBounds	viewTextBounds(player->GetEyePosition());
		idBounds	viewBounds(player->GetEyePosition());

		float viewDist = (float)g_showEntityDetail.GetInteger();
		viewTextBounds.ExpandSelf(512.0f);
		viewBounds.ExpandSelf(512.0f);
		for (ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next()) {
			// don't draw the worldspawn
			if (ent == world) {
				continue;
			}

			//BC skip player stuff.
			if ((gameLocal.GetLocalPlayer() == ent || !ent->name.CmpPrefix("player1_") || ent->GetBindMaster() == gameLocal.GetLocalPlayer()) && g_showEntityInfo.GetInteger() <= 1)
				continue;

			auto entRadiusSqr = ent->GetPhysics()->GetBounds().GetRadius();
			entRadiusSqr *= entRadiusSqr;

			auto dirToEnt = ent->GetPhysics()->GetOrigin() - player->GetEyePosition();
			auto distToEntSqr = dirToEnt.LengthSqr();

			if (distToEntSqr < viewDist*viewDist && distToEntSqr > 1.0f) {
				bool inBounds = distToEntSqr < entRadiusSqr;

				if (!inBounds) {
					auto forwardDir = player->viewAngles.ToForward().Normalized();
					if (DotProduct(forwardDir, dirToEnt) > 0.0f) {
						auto pointProj = DotProduct(forwardDir, dirToEnt)*forwardDir;
						auto distRadSqr = (pointProj - dirToEnt).LengthSqr();
						dirToEnt.Normalize();
						auto dotAng = DotProduct(forwardDir, dirToEnt);
						if (distRadSqr <= entRadiusSqr || idMath::ACos(dotAng)*idMath::M_RAD2DEG < 20.0f) {
							inBounds = true;
						}
					}
				}

				if (inBounds) {
					auto distAlpha = 1.0f - ((idMath::Sqrt(distToEntSqr) - 100.0f) / (viewDist));
					distAlpha = idMath::ClampFloat(0.0f, 1.0f, distAlpha);
					DrawEntityDebug(ent, viewTextBounds, axis, up, distAlpha, true);
				}
			}
		}
	} else if (g_showEntityInfo.GetBool()) {
		idMat3		axis = player->viewAngles.ToMat3();
		idVec3		up = axis[2] * 5.0f;
		idBounds	viewTextBounds(origin);
		idBounds	viewBounds(origin);

		viewTextBounds.ExpandSelf(128.0f);
		viewBounds.ExpandSelf(512.0f);
		for (ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next()) {
			// don't draw the worldspawn
			if (ent == world) {
				continue;
			}

			// skip if the entity is very far away
			if ( !viewBounds.IntersectsBounds( ent->GetPhysics()->GetAbsBounds() ) ) {
				continue;
			}

			//BC skip player stuff.
			if ((gameLocal.GetLocalPlayer() == ent || !ent->name.CmpPrefix("player1_") || ent->GetBindMaster() == gameLocal.GetLocalPlayer()) && g_showEntityInfo.GetInteger() <= 1)
				continue;

			if (g_showEntityInfo.GetInteger() >= 2)
			{
				//skip func static.
				if ( ent->name.CmpPrefix("func_static") == 0 || ent->name.CmpPrefix("idTrigger_") == 0)
				{
					continue;
				}
			}

			DrawEntityDebug(ent, viewTextBounds, axis, up);
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

	//BC show model names.
	if (g_showModelNames.GetBool())
	{
		idMat3		axis = player->viewAngles.ToMat3();
		idVec3		up = axis[2] * 5.0f;
		idBounds	viewTextBounds(origin);
		idBounds	viewBounds(origin);

		float maxDistance = 320.0f;

		viewTextBounds.ExpandSelf(maxDistance);
		viewBounds.ExpandSelf(maxDistance);
		for (ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
		{
			// don't draw the worldspawn
			if (ent == world) {
				continue;
			}

			if (!ent->spawnArgs.GetString("model"))
				continue;

			if (!viewBounds.IntersectsBounds(ent->GetPhysics()->GetAbsBounds()))
			{
				continue;
			}

			const idBounds& entBounds = ent->GetPhysics()->GetAbsBounds();


			if (viewTextBounds.IntersectsBounds(entBounds))
			{

				gameRenderWorld->DrawText(ent->spawnArgs.GetString("model"), entBounds.GetCenter(), 0.1f, idVec4(1, 1, 1, 1), axis, 1);
			}
		}
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
			idAI::FindPathAroundObstacles(NULL, player->GetPhysics(), aas, NULL, player->GetPhysics()->GetOrigin(), seekPos, path );
		}
	}

	// collision map debug output
	collisionModelManager->DebugOutput( player->GetEyePosition() );

	//BC
	if (g_showmaterial.GetBool())
	{
		//print material name
		trace_t tr;

		gameLocal.clip.TracePoint(tr, gameLocal.GetLocalPlayer()->GetEyePosition(),
			gameLocal.GetLocalPlayer()->GetEyePosition() + (gameLocal.GetLocalPlayer()->firstPersonViewAxis[0] * 1024),
			MASK_SHOT_RENDERMODEL, gameLocal.GetLocalPlayer());

		if (tr.fraction < 1)
		{
			gameRenderWorld->DebugLine(colorWhite, tr.endpos + (tr.c.normal * 16), tr.endpos);
			gameRenderWorld->DebugCircle(colorWhite, tr.endpos, tr.c.normal, 3, 7);
			gameRenderWorld->DebugCircle(colorBlack, tr.endpos, tr.c.normal, 4, 7);

			if (tr.c.material != NULL)
				gameRenderWorld->DrawText(tr.c.material->GetName(), tr.endpos + idVec3(0, 0, 24), .3f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());

			if (tr.c.entityNum > 0 && tr.c.entityNum < MAX_GENTITIES - 2)
			{
				if (gameLocal.entities[tr.c.entityNum])
				{
					idStr skinname = NULL;
					skinname = gameLocal.entities[tr.c.entityNum]->spawnArgs.GetString("skin");

					if (skinname.Length() > 0)
					{
						gameRenderWorld->DrawText(va("SKIN: %s", skinname.c_str()), tr.endpos + idVec3(0, 0, 14), .2f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());
					}
				}
			}
	
		}
	}

	if (g_showmodel.GetBool())
	{
		//print model name
		trace_t tr;

		gameLocal.clip.TracePoint(tr, gameLocal.GetLocalPlayer()->GetEyePosition(),
			gameLocal.GetLocalPlayer()->GetEyePosition() + (gameLocal.GetLocalPlayer()->firstPersonViewAxis[0] * 1024),
			MASK_SHOT_RENDERMODEL, gameLocal.GetLocalPlayer());

		if (tr.fraction < 1)
		{
			gameRenderWorld->DebugLine(colorWhite, tr.endpos + (tr.c.normal * 16), tr.endpos);
			gameRenderWorld->DebugCircle(colorWhite, tr.endpos, tr.c.normal, 3, 7);
			gameRenderWorld->DebugCircle(colorBlack, tr.endpos, tr.c.normal, 4, 7);

			if (gameLocal.entities[tr.c.entityNum])
				gameRenderWorld->DrawText(va("%s\n", gameLocal.entities[tr.c.entityNum]->spawnArgs.GetString("model", "")), tr.endpos + idVec3(0, 0, 16), .2f, colorWhite, gameLocal.GetLocalPlayer()->viewAngles.ToMat3());

			
		}
	}

	if (g_editEntityMode.GetInteger() > 0)
	{
		if (lastEditstate != g_editEntityMode.GetInteger())
		{
			//BC player just entered editmode. Activate noclip mode and unarmed weapon. This is fired ONCE whenever the editmenu is opened.
			lastEditstate = true;
			gameLocal.GetLocalPlayer()->noclip = true;
			gameLocal.GetLocalPlayer()->SelectWeapon(0, 0, true);
			gameEdit->lighteditMenu = uiManager->FindGui("guis/edit_light.gui", true, false, true);
			gameEdit->lighteditMenu->Activate(true, gameLocal.time);

			gameEdit->lighteditMenu->SetStateString("entName", "<no light selected>");
			gameEdit->ClearEntitySelection();
		}
	}
	else if (lastEditstate > 0)
	{		
		//BC exited editmode.
		lastEditstate = 0;
		gameEdit->ClearEntitySelection();
	}
}

idVec4 Vec4Alpha(idVec4 a, float alpha) {
	return idVec4(a.x*alpha, a.y*alpha, a.z*alpha, alpha);
}

void idGameLocal::DrawEntityDebug(idEntity *ent, idBounds viewTextBounds, idMat3 axis, idVec3 up, float distAlpha, bool fullInfo) {
	const idBounds &entBounds = ent->GetPhysics()->GetAbsBounds();
	int contents = ent->GetPhysics()->GetContents();
	if (contents & CONTENTS_BODY) {
		gameRenderWorld->DebugBounds(Vec4Alpha(colorCyan, distAlpha), entBounds);
	}
	else if (contents & CONTENTS_TRIGGER) {
		gameRenderWorld->DebugBounds(Vec4Alpha(colorOrange, distAlpha), entBounds);
	}
	else if (contents & CONTENTS_SOLID) {
		gameRenderWorld->DebugBounds(Vec4Alpha(colorGreen, distAlpha), entBounds);
	}
	else {
		if (!entBounds.GetVolume()) {
			gameRenderWorld->DebugBounds(Vec4Alpha(colorMdGrey, distAlpha), entBounds.Expand(8.0f));
		}
		else {
			gameRenderWorld->DebugBounds(Vec4Alpha(colorMdGrey, distAlpha), entBounds);
		}
	}

	if (viewTextBounds.IntersectsBounds(entBounds)) {
		if (fullInfo || g_showEntityDetail.GetInteger() > 0) {
			auto textPos = entBounds.GetCenter() + up;
			gameRenderWorld->DrawText(va("#%d (%s)", ent->entityNumber, ent->GetClassname()), textPos, 0.1f, Vec4Alpha(colorWhite, distAlpha), axis, 1);
			textPos -= up;
			gameRenderWorld->DrawText(ent->name.c_str(), textPos, 0.1f, Vec4Alpha(colorWhite, distAlpha), axis, 1);
			textPos -= up;
			if (ent->GetRenderEntity() && ent->GetRenderEntity()->hModel) {
				auto modelName = ent->GetRenderEntity()->hModel->Name();
				while (strchr(modelName, '/') && strlen(modelName) > 2) {
					modelName = strchr(modelName, '/') + 1;
				}
				gameRenderWorld->DrawText(modelName, textPos, 0.1f, Vec4Alpha(colorWhite, distAlpha), axis, 1);
				textPos -= up;
			}
			if (ent->maxHealth > 0) {
				gameRenderWorld->DrawText(va("%d HP", ent->health), textPos, 0.1f, Vec4Alpha(colorWhite, distAlpha), axis, 1);
				textPos -= up;
			}
			auto pos = ent->GetPhysics()->GetOrigin();
			gameRenderWorld->DrawText(va("%.2f / %.2f / %.2f", pos.x, pos.y, pos.z), textPos, 0.07f, Vec4Alpha(colorWhite, distAlpha), axis, 1);
		}
		else {
			gameRenderWorld->DrawText(ent->name.c_str(), entBounds.GetCenter(), 0.1f, Vec4Alpha(colorWhite, distAlpha), axis, 1);
			gameRenderWorld->DrawText(va("#%d", ent->entityNumber), entBounds.GetCenter() + up, 0.1f, Vec4Alpha(colorWhite, distAlpha), axis, 1);
		}
	}
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
	aasHandle_t check id_attribute((unused));

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

	// SW: By increasing GENTITYNUM_BITS, we've technically halved this cap.
	// It shouldn't be a problem (the usual rate of increase is pretty leisurely)
	// but we're not sure if it could become more of a risk with very long/complex sessions.
	// Worth keeping an eye on (and potentially finding out *why* this is capped like this)
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

		if ( ent && obj->IsType( idEntity::Type ) )
		{
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

	Warning( "'%s' doesn't include a spawnfunc or spawnclass%s.", classname, error.c_str() );
	return false;
}

/*
================
idGameLocal::FindEntityDef
================
*/
const idDeclEntityDef *idGameLocal::FindEntityDef( const char *name, bool makeDefault ) const {

	//BC possible crash fix when def name is invalid....
	if (name == NULL)
		return NULL;

	if (name[0] == '\0') //if empty.
		return NULL;

	const idDecl *decl = NULL;
	if ( isMultiplayer )
	{
		decl = declManager->FindType( DECL_ENTITYDEF, va( "%s_mp", name ), false );
	}
	
	if ( !decl )
	{
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
//Return TRUE if we DON'T want entity to spawn.
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
	if ( g_skill.GetInteger() == 3 ) {
		name = spawnArgs.GetString( "classname" );
		// _D3XP :: remove moveable medkit packs also
		if ( idStr::Icmp( name, "item_medkit" ) == 0 || idStr::Icmp( name, "item_medkit_small" ) == 0 ||
			 idStr::Icmp( name, "moveable_item_medkit" ) == 0 || idStr::Icmp( name, "moveable_item_medkit_small" ) == 0 ) {

			result = true;
		}
	}

	if ( gameLocal.isMultiplayer ) {
		name = spawnArgs.GetString( "classname" );
		if ( idStr::Icmp( name, "weapon_bfg" ) == 0 || idStr::Icmp( name, "weapon_soulcube" ) == 0 ) {
			result = true;
		}
	}

	
	//BC spawn filter logic. Inhibit certain entity types from spawning.
	if (g_spawnfilter.GetString()[0] && gameLocal.world != NULL)
	{
		idStrList* itemlist = NULL;
		gameLocal.world->spawnfilterTable.Get(g_spawnfilter.GetString(), &itemlist);		

		if (itemlist == NULL)
		{
			gameLocal.Error("g_spawnfilter: unknown filter type '%s'", g_spawnfilter.GetString());
			return false;
		}

		if (itemlist->Num() > 0)
		{
			//Verify if the item we're spawning is in this filter list.
			name = spawnArgs.GetString("classname");
			
			for (int i = 0; i < itemlist->Num(); i++)
			{
				idStr inhibitedName = itemlist[0][i];
				if (idStr::Icmp(name, inhibitedName.c_str()) == 0)
				{
					result = true; //found a match in the filter list. Prohibit it.
					i = 99999999;
				}
			}
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

	Printf( "----- Spawning Entities -----\n" );

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

	DPrintf( "...%i entities spawned, %i inhibited\n\n", num, inhibit );
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
	for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() )
	{
		//BC this hack is to prevent light edit mode from picking the light bound to player.....
		if (g_editEntityMode.GetInteger() == 1)
		{
			idEntity *parent;
			parent = ent->GetBindMaster();
			if (parent == gameLocal.GetLocalPlayer())
			{
				continue;
			}
		}

		if ( ent->IsType( c ) && ent != skip )
		{		

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
	for( ent = spawnedEntities.Next(); ent != NULL && entCount < maxCount; ent = ent->spawnNode.Next() ) {
		if ( ent->GetPhysics()->GetAbsBounds().IntersectsBounds( bo ) ) {
			entityList[entCount++] = ent;
		}
	}

	return entCount;
}

//BC get entities within a big ol box.
int idGameLocal::EntitiesWithinAbsBoundingbox(const idBounds bounds, idEntity **entityList, int maxCount) const {

	idEntity *ent;
	int entCount = 0;

	for (ent = spawnedEntities.Next(); ent != NULL && entCount < maxCount; ent = ent->spawnNode.Next())
	{
		if (ent->GetPhysics()->GetAbsBounds().IntersectsBounds(bounds))
		{
			entityList[entCount++] = ent;
		}
	}

	return entCount;
}

int idGameLocal::EntitiesWithinBoundingbox(const idBounds bounds, idVec3 boxPosition, idEntity **entityList, int maxCount) const {

	idEntity *ent;
	int entCount = 0;

	idBounds adjustedBox = bounds;
	adjustedBox += boxPosition;

	for (ent = spawnedEntities.Next(); ent != NULL && entCount < maxCount; ent = ent->spawnNode.Next())
	{
		if (ent->GetPhysics()->GetAbsBounds().IntersectsBounds(adjustedBox))
		{
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
bool idGameLocal::RequirementMet( idEntity *activator, const idStr &_requires, int removeItem ) {
	if ( _requires.Length() ) {
		if ( activator->IsType( idPlayer::Type ) )
		{
			idPlayer *player = static_cast<idPlayer *>(activator);
			idDict *item = player->FindInventoryItem(_requires);
			if ( item ) {
				if ( removeItem ) {
					player->RemoveInventoryItem( item );
				}
				return true;
			} else {
				return false;
			}
		}
		else
		{
			return false; //only player can meet requirements.
		}
	}

	return true;
}

//This is a variation that checks the player's hotbar inventory items.
bool idGameLocal::RequirementMet_Inventory(idEntity* activator, const idStr& _requires, int removeItem)
{
	if (_requires.Length() <= 0)
		return true;

	if (activator == NULL)
		return false;

	if (idStr::Icmp(activator->spawnArgs.GetString("inv_name"), _requires.c_str()) == 0)
	{
		//is the object itself the required item? if so, return true

		if (removeItem > 0)
		{
			activator->Hide();
			activator->PostEventMS(&EV_Remove, 0);
		}

		return true;
	}

	if (!activator->IsType(idPlayer::Type))
		return false; //Only player can do this.

	idPlayer* player = static_cast<idPlayer*>(activator);
	idEntity *inventoryItem = player->HasEntityInCarryableInventory_ViaInvName(_requires);
	if (inventoryItem == NULL)
	{
		//player doesn't have it. Exit.
		return false;
	}
	
	//Player DOES have it.
	if (removeItem > 0)
	{
		//remove the item from inventory.
		player->RemoveCarryableFromInventory(inventoryItem);
		inventoryItem->Hide();
		inventoryItem->PostEventMS(&EV_Remove, 0);
	}

	return true;
}

/*
============
idGameLocal::AlertAI
============
*/
/*
void idGameLocal::AlertAI( idEntity *ent ) {
	if ( ent && ent->IsType( idActor::Type ) ) {
		// alert them for the next frame
		lastAIAlertTime = time + msec;
		lastAIAlertEntity = static_cast<idActor *>( ent );
	}
}
*/

/*
============
idGameLocal::GetAlertEntity
============
*/
/*
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
*/

void idGameLocal::ThrowShrapnel(const idVec3 &origin, const char *defName, const idEntity *passEntity)
{
	const idDict *	projectileDef;	
	int				i;
	
	projectileDef = gameLocal.FindEntityDefDict(defName, false);

	if (!projectileDef)
	{
		gameLocal.Warning("Throwshrapnel unable to find projectile def: %s", defName);
		return;
	}

	for (i = 0; i < 20; i++)
	{
		idVec3			dir;
		idEntity *		ent;
		idProjectile *	projectile;
		idVec3			adjustedOrigin;
		int				penetrationContents;		


		dir.x = gameLocal.random.CRandomFloat();
		dir.y = gameLocal.random.CRandomFloat();

		if (i <= 16)
		{
			dir.z = gameLocal.random.CRandomFloat() * .2f;
		}
		else
		{
			dir.z = gameLocal.random.RandomFloat() * 5;
			
			if (gameLocal.random.RandomInt(10) > 5)
				dir.z *= -1.0f;
		}

		dir.Normalize();


		
		adjustedOrigin = origin + (dir * 32);

		

		if (passEntity != NULL)
		{
			penetrationContents = gameLocal.clip.Contents(adjustedOrigin, NULL, mat3_identity, CONTENTS_SOLID, NULL);

			if (penetrationContents & MASK_SOLID)
			{
				//If shrapnel spawnpoint starts in solid, then ignore.
				continue;
			}
		}

		gameLocal.SpawnEntityDef(*projectileDef, &ent, false);

		if (!ent || !ent->IsType(idProjectile::Type))
		{
			gameLocal.Error("Throwshrapnel unable to spawn projectile def: %s", defName);
		}

		projectile = (idProjectile *)ent;
		projectile->Create(NULL, adjustedOrigin, dir);
		projectile->Launch(adjustedOrigin, dir, vec3_origin);

		//gameRenderWorld->DebugArrow(colorGreen, adjustedOrigin, adjustedOrigin + (dir * 64), 8, 30000);
	}
}


//Returns an array of equidistant points on a sphere. Sphere radius is 1.0, sphere diameter is 2.0
idVec3* idGameLocal::GetPointsOnSphere(int nPoints)
{
	float fPoints = (float)nPoints;

	idVec3* points = new idVec3[nPoints];

	float inc = (float)(3.14159f * (3 - idMath::Sqrt(5)));
	float off = 2 / fPoints;

	for (int k = 0; k < nPoints; k++)
	{
		float y = k * off - 1 + (off / 2);
		float r = (float)idMath::Sqrt(1 - y * y);
		float phi = k * inc;

		points[k].x = (float)(idMath::Cos(phi) * r);
		points[k].y = y;
		points[k].z = (float)(idMath::Sin(phi) * r);

		/*
		points[k] = new idVec3(
			(float)(idMath::Cos(phi) * r),
			y,
			(float)(idMath::Sin(phi) * r));*/
	}

	return points;
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
	idVec3		v, damagePoint, dir;
	int			i, e, damage, radius, push, innerRadius;

	const idDict *damageDef = FindEntityDefDict( damageDefName, false );
	if ( !damageDef ) {
		Warning( "Unknown damageDef '%s'", damageDefName );
		return;
	}

	damageDef->GetInt("innerradius", "20", innerRadius);
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


		//BC
		if (!ent || ent->IsHidden() || ent->entityNumber < 0 || ent->entityNumber >= MAX_GENTITIES - 1 || ent->entityDefNumber <= 0 || !ent->name || !ent->GetPhysics() || ent->name.Length() <= 0)
			continue;

		if (ent->entityNumber >= MAX_GENTITIES - 1 || ent->entityNumber < 0) //bc fix crash
			continue;

		if ( !ent->fl.takedamage ) { //BC CRASH HERE: ent id 0
			continue;
		}

		if ( ent == inflictor || ( ent->IsType( idAFAttachment::Type ) && static_cast<idAFAttachment*>(ent)->GetBody() == inflictor ) ) {
			continue;
		}

		if ( ent == ignoreDamage || ( ent->IsType( idAFAttachment::Type ) && static_cast<idAFAttachment*>(ent)->GetBody() == ignoreDamage ) ) {
			continue;
		}

		//if (ent->IsType(idProjectile::Type) && damageDef->GetBool("affects_projectiles", "0"))
		//	continue;

		// don't damage a dead player
		if ( isMultiplayer && ent->entityNumber < MAX_CLIENTS && ent->IsType( idPlayer::Type ) && static_cast< idPlayer * >( ent )->health < 0 ) {
			continue;
		}
		

		// find the distance from the edge of the bounding box
		for ( i = 0; i < 3; i++ ) {
			if ( origin[ i ] < ent->GetPhysics()->GetAbsBounds()[0][ i ] ) { //BC CRASH HERE = entitynumber 4095. Happens randomly... can also try attaching TNT to a friendly mech.
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

		if ( ent->CanDamage( origin, damagePoint ) )
		{
			// push the center of mass higher than the origin so players
			// get knocked into the air more
			dir = ent->GetPhysics()->GetOrigin() - origin;
			dir[ 2 ] += 24;

			// get the damage scale

			//BC do an innerRadius damage effect where things caught in this radius get 100% damage.

			if (dist <= innerRadius)
			{
				damageScale = dmgPower * 1.0f;
			}
			else
			{
				//Scale the damage from the innerRadius
				damageScale = dmgPower * (1.0f - (dist - innerRadius) / (radius - innerRadius));
			}

			if ( ent == attacker || ( ent->IsType( idAFAttachment::Type ) && static_cast<idAFAttachment*>(ent)->GetBody() == attacker ) ) {
				damageScale *= attackerDamageScale;
			}

			ent->Damage( inflictor, attacker, dir, damageDefName, damageScale, INVALID_JOINT );
		}
	}

	// push physics objects
	if ( push )
	{
		int pushRadius = Max(damageDef->GetInt("pushradius"), radius);
		RadiusPush( origin, pushRadius, push * dmgPower, attacker, ignorePush, attackerPushScale, false );
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
	for ( i = 0; i < numListedClipModels; i++ )
	{

		clipModel = clipModelList[i];

		// never push render models
		if ( clipModel->IsRenderModel() ) {
			continue;
		}

		ent = clipModel->GetEntity();

		//BC CRASH
		if (!ent || ent->IsHidden() || ent->entityNumber <= 0 || ent->entityNumber >= MAX_GENTITIES - 1 || ent->entityDefNumber < -1 || !ent->name || !ent->GetPhysics() || ent->name.Length() <= 0 || ent->entityDefNumber <= 0)
		{
			continue;
		}

		// never push projectiles
		//if ( ent->IsType( idProjectile::Type ) ) {
		//	continue;
		//}

		//BC only allow certain things.
		if (ent->IsType(idItem::Type) || ent->IsType(idActor::Type) || ent->IsType(idBrittleFracture::Type))
		{
			if ( ent->IsType( idMoveableItem::Type ) )
			{
				static_cast< idMoveableItem* >( ent )->canDealEntDamage = true;
			}
		}
		else
		{
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
		}
		else
		{
			//Push the object.
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

	//if ( !trm || 1 ) //BC uhhhhhhhhhhhhhhhhh. I don't know understand the "1" condition here.
	if (!trm) 
	{
		impulse = clipModel->GetAbsBounds().GetCenter() - origin;
		impulse.Normalize();
		impulse.z += 1.0f;
		clipModel->GetEntity()->ApplyImpulse( world, clipModel->GetId(), clipModel->GetOrigin(), push * impulse );

		//gameRenderWorld->DebugLine(colorRed, origin, origin + idVec3(0, 0, 32), 10000);

		return;
	}

	localOrigin = ( origin - clipModel->GetOrigin() ) * clipModel->GetAxis().Transpose();
	for ( i = 0; i < trm->numPolys; i++ )
	{
		int impulseMultiplier = 1;

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
		//impulse *= ( dist * 2.0f ); //BC remove this because it was causing massive translations.

		// impulse is applied to the center of the polygon
		center = clipModel->GetOrigin() + center * clipModel->GetAxis();

		//gameRenderWorld->DebugArrow(colorGreen, center, center + impulse, 4, 10000);
		//gameRenderWorld->DrawTextA(va("%f %f %f", impulse.x, impulse.y, impulse.z), clipModel->GetOrigin(), .15f, idVec4(1, 1, 1, 1), gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, 10000);


		
		if (clipModel->GetEntity()->IsType(idMoveable::Type))
		{
			impulseMultiplier = 100; //BC TODO fix this????
		}
		else
		{
			//BC limit speed of pushing moveableItems to workaround the "huge translation" errors.
			float impulseLength = impulse.Length();

			if (impulseLength >= 300)
			{
				idVec3 impulseDir = impulse;
				impulseDir.Normalize();

				impulse = impulseDir * 300;
				//common->Printf("hi  %s   speed %f    %f %f %f\n", clipModel->GetEntity()->GetName(), impulseLength, impulse.x, impulse.y, impulse.z);
			}
		}

		clipModel->GetEntity()->ApplyImpulse( world, clipModel->GetId(), center, impulse * impulseMultiplier);
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
		if ( areaNum < 0 )
		{
			Warning( "SpreadLocations: location '%s' is not in a valid area\n", ent->spawnArgs.GetString( "name" ) );
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

// SM
/*
===================
idGameLocal::LocationForEntity

Try to find the location for the specified entity
May return NULL
===================
*/
idLocationEntity *idGameLocal::LocationForEntity( const idEntity *ent )
{
	if ( !locationEntities || !ent || !ent->GetPhysics() ) {
		if ( ent && !ent->GetPhysics() ) {
			Warning( "idGameLocal::LocationForEntity: %s does not have physics, this function won't work", ent->GetName() );
		}
		return NULL;
	}

	int areaNum = -1;
	const int MAX_LOCATIONS = 4;
	int potentialAreas[MAX_LOCATIONS];
	int numAreas = gameRenderWorld->BoundsInAreas( ent->GetPhysics()->GetAbsBounds(), potentialAreas, MAX_LOCATIONS );
	if ( numAreas == 1 ) {
		// Simple case - Bounds intersect with only one area
		areaNum = potentialAreas[0];
	} else if ( numAreas > 1 ) {
		// Complex case - Bounds intersect with multiple areas
		// The best guess is the one that contains the origin of the entity, so use LocationForPoint

		//BC 3-3-2025: do a fallback failsafe in case the location is null.
		idLocationEntity* locEnt = LocationForPoint( ent->GetPhysics()->GetOrigin() );
		if (locEnt != nullptr)
		{
			return locEnt;
		}

		//BC 3-3-2025 fallback check: 1 unit upward.
		locEnt = LocationForPoint(ent->GetPhysics()->GetOrigin() + idVec3(0,0,1)); 
		if (locEnt != nullptr)
		{
			return locEnt;
		}

		//BC 3-3-2025 fallback check: 1 unit forwards.
		idVec3 forward;
		ent->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, NULL, NULL);
		locEnt = LocationForPoint(ent->GetPhysics()->GetOrigin() + (forward * 1));
		//if (locEnt == nullptr)
		//{
		//	gameLocal.Warning("Failed to get locationEnt for '%s'\n", ent->GetName());
		//}

		return locEnt;
	}

	if ( areaNum < 0 ) {
		return NULL;
	}
	if ( areaNum >= gameRenderWorld->NumAreas() ) {
		Error( "idGameLocal::LocationForEntity: areaNum >= gameRenderWorld->NumAreas()" );
	}

	return locationEntities[areaNum];
}

// SM
/*
===================
idGameLocal::AreaNumForEntity

Try to find the area number for the specified entity
Returns -1 if none
===================
*/
int	idGameLocal::AreaNumForEntity( const idEntity *ent ) {
	if ( !locationEntities || !ent || !ent->GetPhysics() ) {
		if ( ent && !ent->GetPhysics() ) {
			Warning( "idGameLocal::AreaNumForEntity: %s does not have physics, this function won't work", ent->GetName() );
		}
		return -1;
	}

	int areaNum = -1;
	const int MAX_LOCATIONS = 4;
	int potentialAreas[MAX_LOCATIONS];
	int numAreas = gameRenderWorld->BoundsInAreas( ent->GetPhysics()->GetAbsBounds(), potentialAreas, MAX_LOCATIONS );
	if ( numAreas == 1 ) {
		// Simple case - Bounds intersect with only one area
		areaNum = potentialAreas[0];
	} else if ( numAreas > 1 ) {
		// Complex case - Bounds intersect with multiple areas
		// The best guess is the one that contains the origin of the entity
		areaNum = gameRenderWorld->PointInArea( ent->GetPhysics()->GetOrigin() );
	}

	return areaNum;
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
		outMsg.WriteInt( portal );
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
int idGameLocal::sortSpawnPoints_Farthest( const void *ptr1, const void *ptr2 ) {
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

int idGameLocal::sortSpawnPoints_Nearest(const void *ptr1, const void *ptr2) {
	const spawnSpot_t *spot1 = static_cast<const spawnSpot_t *>(ptr1);
	const spawnSpot_t *spot2 = static_cast<const spawnSpot_t *>(ptr2);
	float diff;

	diff = spot1->dist - spot2->dist;
	if (diff > 0.0f) {
		return 1;
	}
	else if (diff < 0.0f) {
		return -1;
	}
	else {
		return 0;
	}
}

//BC this is used to sort the wiregrenade points to be the longest wires possible.
int idGameLocal::sortVecPoints_Farthest(const void *ptr1, const void *ptr2) {
	const vecSpot_t *spot1 = static_cast<const vecSpot_t *>(ptr1);
	const vecSpot_t *spot2 = static_cast<const vecSpot_t *>(ptr2);
	float diff;

	diff = spot1->distance - spot2->distance;
	if (diff < 0.0f) {
		return 1;
	}
	else if (diff > 0.0f) {
		return -1;
	}
	else {
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
			qsort( ( void * )teamSpawnSpots[ team ].Ptr(), teamSpawnSpots[ team ].Num(), sizeof( spawnSpot_t ), ( int (*)(const void *, const void *) )sortSpawnPoints_Farthest );

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
		qsort( ( void * )spawnSpots.Ptr(), spawnSpots.Num(), sizeof( spawnSpot_t ), ( int (*)(const void *, const void *) )sortSpawnPoints_Farthest );

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

	if ( gameType == GAME_LASTMAN )
	{
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
	if ( quickSlowmoReset )
	{
		quickSlowmoReset = false;

		// stop the sounds
		if ( gameSoundWorld && soundSlowmoActive )
		{
			gameSoundWorld->ExitSlowmo(soundSlowmoHandle);
			soundSlowmoActive = false;
		}

		// stop the state
		slowmoState = SLOWMO_STATE_OFF;
		slowmoMsec = USERCMD_MSEC;
	}

	// check the player state
	player = GetLocalPlayer();
	powerupOn = false;

	
	if ( (player && player->PowerUpActive( HELLTIME )) || g_enableSlowmo.GetBool() || menuSlowmo)
	{
		powerupOn = true;
	}
	

	

	// determine proper slowmo state
	if ( powerupOn && (slowmoState == SLOWMO_STATE_OFF || slowmoState == SLOWMO_STATE_RAMPDOWN) )
	{
		//Called when powerup is activated.
		//common->Printf("ENTERING SLOWMO\n");
		slowmoState = SLOWMO_STATE_RAMPUP;

		slowmoMsec = msec;
		if ( gameSoundWorld && slowmoMsec / (float)USERCMD_MSEC < 1.0f)
		{
			if (!soundSlowmoActive)
			{
				soundSlowmoHandle = gameSoundWorld->EnterSlowmo();
				soundSlowmoActive = true;
			}
			gameSoundWorld->SetSlowmoSpeed( slowmoMsec / (float)USERCMD_MSEC, soundSlowmoHandle );
		}
	}
	else if ( !powerupOn && slowmoState == SLOWMO_STATE_ON )
	{
		//Called when slowmo period has ended, just before the ramp to normal time.
		//common->Printf("EXITING SLOWMO\n");
		slowmoState = SLOWMO_STATE_RAMPDOWN;

		// play the stop sound
		if ( player )
		{
			player->PlayHelltimeStopSound();
		}
	}

	// do any necessary ramping
	if ( slowmoState == SLOWMO_STATE_RAMPUP )
	{
		float enterRate = .1f;

		//Entering slowmo state.
		delta = 2 - slowmoMsec;

		if ( fabs( delta ) < enterRate /*g_slowmoStepRate.GetFloat()*/ )
		{
			slowmoMsec = 2;
			slowmoState = SLOWMO_STATE_ON;
		}
		else
		{
			slowmoMsec += delta * enterRate /*g_slowmoStepRate.GetFloat()*/;
		}

		if ( gameSoundWorld && slowmoMsec / (float)USERCMD_MSEC < 1.0f)
		{
			if (!soundSlowmoActive)
			{
				soundSlowmoHandle = gameSoundWorld->EnterSlowmo();
				soundSlowmoActive = true;
			}
			gameSoundWorld->SetSlowmoSpeed( slowmoMsec / (float)USERCMD_MSEC, soundSlowmoHandle );
		}
	}
	else if ( slowmoState == SLOWMO_STATE_RAMPDOWN )
	{
		//When slowmo is ramping back to normal time.
		delta = 16 - slowmoMsec;

		if ( fabs( delta ) < g_slowmoStepRate.GetFloat() )
		{
			slowmoMsec = 16;
			slowmoState = SLOWMO_STATE_OFF;
			if ( gameSoundWorld && soundSlowmoActive )
			{
				gameSoundWorld->ExitSlowmo(soundSlowmoHandle);
				soundSlowmoActive = false;
			}
		}
		else
		{
			slowmoMsec += delta * g_slowmoStepRate.GetFloat();
		}

		if ( gameSoundWorld && slowmoMsec / (float)USERCMD_MSEC < 1.0f && soundSlowmoActive)
		{
			gameSoundWorld->SetSlowmoSpeed( slowmoMsec / (float)USERCMD_MSEC, soundSlowmoHandle );
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
	soundSlowmoActive = false;

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


idStr idGameLocal::GetDLLVersionString()
{
	return idStr::Format( "%s %s %s", CMAKE_INTDIR, __DATE__, __TIME__ );
}

bool idGameLocal::PrecacheModel(idEntity *ent, const char *modelDef)
{
	const char *defValue = ent->spawnArgs.GetString(modelDef);
	if (defValue[0])
	{
		idTraceModel trm;

		// load the trace model
		return collisionModelManager->TrmFromModel(defValue, trm);
	}

	return false;
}

bool idGameLocal::PrecacheDef(idEntity *ent, const char *defName)
{
	const char *defValue = ent->spawnArgs.GetString(defName);
	if (defValue[0])
	{
		idTraceModel trm;
		const char *clipModelName;
		const idDeclEntityDef *entityDef = gameLocal.FindEntityDef(defValue, false);

		if (entityDef)
		{
			entityDef->dict.GetString("clipmodel", "", &clipModelName);

			if (!clipModelName[0])
			{
				clipModelName = entityDef->dict.GetString("model"); //attempt to use the visual model
			}

			// load the trace model
			collisionModelManager->TrmFromModel(clipModelName, trm);
		}
	}

	return false;
}

//Return TRUE if sound should be muted/baffled/silenced. Is overriden by soundclass 1 soundshader parameter.
int idGameLocal::IsBaffled(idVec3 soundPosition)
{
	if (static_cast<idMeta*>(this->metaEnt.GetEntity())->GetWorldBaffled())
		return true;

	int returnvalue = 0;
	for (idEntity* entity = gameLocal.bafflerEntities.Next(); entity != NULL; entity = entity->bafflerNode.Next())
	{
		int  baffleRange;

		if (!entity)
			continue;

		if (entity->IsHidden() || entity->spawnArgs.GetInt("baffleactive") <= 0)
			continue;

		baffleRange = entity->spawnArgs.GetInt("baffle_range", "256");

		//Do distance check.
		if ((entity->GetPhysics()->GetOrigin() - soundPosition).LengthSqr() <= (baffleRange * baffleRange)) //Distance squared.
		{
			returnvalue = Max(returnvalue, entity->spawnArgs.GetInt("baffleactive"));
		}
	}

	return returnvalue;
}

void idGameLocal::SetSuspiciousNoise(idEntity *ent, idVec3 position, int radius, int priority)
{
	if (IsBaffled(position))
		return;

	if (gameLocal.time + 3000 >= lastSuspiciousNoiseTime && priority < lastSuspiciousNoisePriority)
		return;

	if (ent->IsType(idPlayer::Type))
	{
		//If player makes suspicious noise and they're inside a vent, then player loses confinedstealth status.
		if (gameLocal.GetLocalPlayer()->inConfinedState /*&& static_cast<idMeta *>(gameLocal.metaEnt.GetEntity())->combatMetastate != COMBATSTATE_IDLE*/)
		{
			gameLocal.GetLocalPlayer()->confinedStealthActive = false;
		}
	}

	this->suspiciousNoisePos = position;
	this->suspiciousNoiseRadius = radius;
	this->lastSuspiciousNoiseTime = gameLocal.time;
	this->lastSuspiciousNoisePriority = priority;

	if (cvarSystem->GetCVarBool("s_debug"))
	{
		const int SUSP_DISPLAYTIME = 3000;
		gameRenderWorld->DrawText("SuspiciousNoise", position + idVec3(0,0,-4), 0.2f, colorGreen, gameLocal.GetLocalPlayer()->viewAngles.ToMat3(), 1, SUSP_DISPLAYTIME);
		//gameRenderWorld->DebugBox(colorGreen, idBox(position, idVec3(radius, radius, radius), mat3_identity), 10000);
		gameRenderWorld->DebugCircle(colorGreen, position, idVec3(0, 0, 1), radius, 16, SUSP_DISPLAYTIME, false);
		gameRenderWorld->DebugCircle(colorGreen, position, idVec3(0, 1, 0), radius, 16, SUSP_DISPLAYTIME, false);
		gameRenderWorld->DebugCircle(colorGreen, position, idVec3(1, 0, 0), radius, 16, SUSP_DISPLAYTIME, false);		
	}


}


//value = enable or disable.
void idGameLocal::SetSlowmo(bool value, bool quickReset)
{
	menuSlowmo = value;

	if (quickReset)
	{
		menuSlowmo = false;
		quickSlowmoReset = true;
	}

	if (value)
	{
		//turn on.
		gameLocal.GetLocalPlayer()->StartSound("snd_slowmo", SND_CHANNEL_ANY);
	}
}

//Get keyboard/mouse control bound to a game action.
idStr idGameLocal::GetKeyFromBinding(const char *bindname, bool joystick)
{
	//BC tweak this so it only shows the first bind.

	// SM: If the binding doesn't have an _ in front of it, assume this is the raw key
	if (bindname[0] != '_')
	{
		return bindname;
	}

	idStr rawkey = cmdSystem->GetKeyFromBinding(bindname, joystick);
	int spaceIdx = rawkey.Find(' ', false);

	if (spaceIdx < 0)
	{
		return rawkey; //No spaces. So just return the whole thing.
	}

	//There is a space in the keybind. Return just the first element.	
	return rawkey.Mid(0, spaceIdx);
}

//Convert time to a readable string.
//value = milliseconds.
const char* idGameLocal::ParseTimeMS(int value)
{
	float ret = value / 1000.0f;
	return ParseTime(ret);
}

//Convert time to a readable string. Returns: MM:SS
//value = seconds.
const char* idGameLocal::ParseTime(float value)
{
	//value = total time, in seconds.
	int minutes = value / 60.0f;
	float seconds = value - (minutes * 60);
	//int deciseconds = (seconds - (int)seconds) * 100.0f;

	//return (va("%d:%02d.%02d", minutes, (int)(seconds), deciseconds));
	return (va("%02d:%02d", minutes, (int)(seconds + .2f) ));
}


//Returns MM:SS:milliseconds
const char* idGameLocal::ParseTimeDetailedMS(int durationMS)
{
	float value = durationMS / 1000.0f;

	//value = total time, in seconds.
	int minutes = value / 60.0f;
	float seconds = value - (minutes * 60);
	int deciseconds = (seconds - (int)seconds) * 100.0f;

	//return (va("%d:%02d.%02d", minutes, (int)(seconds), deciseconds));
	return (va("%02d:%02d.%02d", minutes, (int)(seconds + .2f), deciseconds));
}

//Returns SS:milliseconds (one decimal place)
const char* idGameLocal::ParseTimeMS_SecAndDecisec(int durationMS)
{
	float value = durationMS / 1000.0f;
	return (va("%.1f",  value));
}

//Returns MMm SSs
idStr idGameLocal::ParseTimeVerbose(int durationMS)
{
	float value = durationMS / 1000.0f;

	//value = total time, in seconds.
	int minutes = value / 60.0f;
	float seconds = value - (minutes * 60);
	
	int deciseconds = (seconds - (int)seconds) * 100.0f;
	idStr deciStr = idStr::Format("%d", deciseconds);
	if (deciStr.Length() <= 1)
	{
		deciStr = idStr::Format("0%s", deciStr.c_str());
	}

	return idStr::Format("%d %s %d.%s %s", minutes, common->GetLanguageDict()->GetString("#str_def_gameplay_minutes"), (int)(seconds ), deciStr.c_str(), common->GetLanguageDict()->GetString("#str_def_gameplay_seconds"));
}

void idGameLocal::SetDebrisBurst(const char *defName, idVec3 position, int count, float radius, float speed, idVec3 direction)
{
	const idDeclEntityDef *entDef;
	entDef = gameLocal.FindEntityDef(defName, false);

	if (entDef == NULL)
	{
		gameLocal.Error("SetDebrisBurst: failed to find def for '%s'\n", defName);
		return;
	}

	int i;
	
	bool isAirless = GetAirlessAtPoint(position);
	
	for (i = 0; i < count; i++)
	{
		idEntity *newEnt;
		idDebris *debrisEnt;
		idVec3 flyDir, spawnPos;

		gameLocal.SpawnEntityDef(entDef->dict, &newEnt, false);

		if (!newEnt || !newEnt->IsType(idDebris::Type))
		{
			gameLocal.Error("SetDebrisBurst: couldn't spawn '%s'\n", defName);
			return;
		}

		spawnPos = position;

		spawnPos.x += -radius + gameLocal.random.RandomInt(radius*2);
		spawnPos.y += -radius + gameLocal.random.RandomInt(radius*2);
		spawnPos.z += -radius + gameLocal.random.RandomInt(radius*2);


		if (direction == vec3_zero)
		{
			//make it fly outward. Bias it upward.
			flyDir = spawnPos - (position + idVec3(0,0,-4));
			flyDir.NormalizeFast();
		}
		else
		{
			//make it all go in the same direction.
			flyDir = direction;
		}


		if (isAirless)
		{
			flyDir.x += -.2f + gameLocal.random.RandomFloat() * .4f;
			flyDir.y += -.2f + gameLocal.random.RandomFloat() * .4f;
			flyDir.z += -.2f + gameLocal.random.RandomFloat() * .4f;			

			flyDir *= (8 + gameLocal.random.RandomInt(speed)); //Make a big "trail" of debris.
		}
		else
		{
			flyDir *= (speed + gameLocal.random.RandomInt(32));
		}


		debrisEnt = static_cast<idDebris *>(newEnt);
		debrisEnt->Create(NULL, spawnPos, mat3_identity);
		debrisEnt->Launch();
		debrisEnt->GetPhysics()->SetLinearVelocity( flyDir );
		debrisEnt->GetPhysics()->SetAngularVelocity(idVec3(gameLocal.random.CRandomFloat() * 16, gameLocal.random.CRandomFloat() * 16, gameLocal.random.CRandomFloat() * 16));

		if (isAirless)
		{
			debrisEnt->GetPhysics()->SetGravity(vec3_zero);
		}
		else
		{
			idVec3 debrisGravity = idVec3(0,0, debrisEnt->spawnArgs.GetFloat("gravity", "-1066"));

			debrisEnt->GetPhysics()->SetGravity(debrisGravity);
		}
	}

}


idEntity* idGameLocal::DoParticleAng(const char* particleName, idVec3 position, idAngles angle, bool autoRemove, bool airlessGravity, idVec3 color)
{
	idDict args;
	idFuncEmitter* idleSmoke;

	if (strlen(particleName) <= 0)
		return NULL;

	args.Clear();
	args.Set("model", particleName);
	args.SetVector("origin", position);
	args.SetMatrix("rotation", angle.ToMat3());
	args.SetBool("start_off", true);
	args.SetBool("airlessGravity", airlessGravity);
	args.SetVector("_color", color);
	idleSmoke = static_cast<idFuncEmitter*>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));
	if (idleSmoke)
	{
		if (autoRemove)
		{
			idleSmoke->PostEventMS(&EV_Remove, 5000);
		}

		idleSmoke->PostEventMS(&EV_Activate, 0, (idEntity *)NULL);
		return idleSmoke;
	}

	return NULL;
}

//the angle is actually an idAngle (pitch, yaw, roll) but we read in an idvec3.
idEntity * idGameLocal::DoParticle(const char *particleName, idVec3 position, idVec3 angle, bool autoRemove, bool airlessGravity, idVec3 color)
{
	idDict args;
	idFuncEmitter			*idleSmoke;

	if (strlen(particleName) <= 0)
		return NULL;

	args.Clear();
	args.Set("model", particleName);
	args.SetVector("origin", position);
	args.SetMatrix("rotation", angle.ToAngles().ToMat3());
	args.SetBool("start_off", true);
	args.SetBool("airlessGravity", airlessGravity);
	args.SetVector("_color", color);
	idleSmoke = static_cast<idFuncEmitter *>(gameLocal.SpawnEntityType(idFuncEmitter::Type, &args));	
	if (idleSmoke)
	{
		//idleSmoke->SetOrigin(position);

		//idleSmoke->SetAxis(angle.ToAngles().ToMat3());

		if (autoRemove)
		{
			idleSmoke->PostEventMS(&EV_Remove, 5000);
		}

		idleSmoke->PostEventMS(&EV_Activate, 0, (idEntity *)NULL);
		return idleSmoke;
	}

	return NULL;
}

//HotReload from DARKMOD
void idGameLocal::HotReloadMap() {
	if (!mapFile) {
		common->Warning("HotReload: idGameLocal has no mapFile stored");
		return;
	}

	idMapReloadInfo info = mapFile->TryReload();

	if (info.cannotReload || info.mapInvalid)
		return;

	//spawn added entities
	for (int i = 0; i < info.addedEntities.Num(); i++) {
		const char* name = info.addedEntities[i];
		idMapEntity* mapEnt = mapFile->FindEntity(info.addedEntities[i]);
		idDict args = mapEnt->epairs;
		if (idEntity* exEnt = gameEdit->FindEntity(name)) {
			//note: this should not happen actually...
			common->Warning("HotReload: multiple entities with name %s: rename the new one!");
			continue;
		}
		gameEdit->SpawnEntityDef(args, NULL);
	}

	//kill removed entities
	for (int i = 0; i < info.removedEntities.Num(); i++) {
		const char* name = info.removedEntities[i];

		idEntity* ent = gameEdit->FindEntity(name);
		if (!ent)
			continue;
		gameEdit->EntityDelete(ent);
	}

	//Update modified entities.
	for (int i = 0; i < info.modifiedEntities.Num(); i++)
	{
		const char* name = info.modifiedEntities[i];
		idMapEntity* newMapEnt = mapFile->FindEntity(name);
		idDict newArgs = newMapEnt->epairs;

		idEntity* ent = FindEntity(name);
		if (!ent)
		{
			continue;
		}

		gameEdit->EntityChangeSpawnArgs(ent, &newArgs);
		gameEdit->EntityUpdateChangeableSpawnArgs(ent, NULL);

		//if (1)
		//{
		//	idStr newModel = newArgs.GetString("model");
		//	gameEdit->EntitySetModel(ent, newModel);
		//}
		//
		//if (1)
		//{
		//	idStr newSkin = newArgs.GetString("skin");
		//
		//	if (newSkin.Length() > 0)
		//	{
		//		ent->PostEventMS(&EV_SetSkin, 0, newSkin);
		//	}
		//}

		if (1)
		{
			idVec3 newOrigin = newArgs.GetVector("origin");
			gameEdit->EntitySetOrigin(ent, newOrigin);
		}

		if (1)
		{
			idMat3 newAxis = newArgs.GetMatrix("rotation");
			gameEdit->EntitySetAxis(ent, newAxis);
		}

		if (1)
		{
			idVec3 newColor = newArgs.GetVector("_color", "1 1 1");
			gameEdit->EntitySetColor(ent, newColor);
		}

		if (1)
		{
			idVec3 newZoomAngle = newArgs.GetVector("zoominspect_angle");
			ent->spawnArgs.SetVector("zoominspect_angle", newZoomAngle);
		}

		if (1)
		{
			idVec3 newZoomPos = newArgs.GetVector("zoominspect_campos");
			ent->spawnArgs.SetVector("zoominspect_campos", newZoomPos);
		}

		if (1)
		{
			float newZoomSize = newArgs.GetFloat("zoominspect_size");
			ent->spawnArgs.SetFloat("zoominspect_size", newZoomSize);
		}

		if (1)
		{
			bool newZoom = newArgs.GetBool("zoominspect");
			ent->spawnArgs.SetBool("zoominspect", newZoom);
		}

		gameEdit->EntityUpdateVisuals(ent);

		if (ent->IsType(idSecurityCamera::Type))
		{
			static_cast<idSecurityCamera *>(ent)->HotReload();
		}
	}

	//Update the text on a note.
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (ent->spawnArgs.GetBool("is_note"))
		{
			idStr newGuiText = common->GetLanguageDict()->GetString(ent->locID);
			//ent->spawnArgs.Set("gui_parm0", newGuiText.c_str());
			ent->Event_SetGuiParm("gui_parm0", newGuiText.c_str());


			//gameRenderWorld->DebugArrowSimple(ent->GetPhysics()->GetOrigin());
		}
	}

	if (this->metaEnt.IsValid())
	{
		idMeta* meta = static_cast<idMeta*>(this->metaEnt.GetEntity());
		if (meta->skyController.IsValid())
		{
			idSkyController* skyController = static_cast<idSkyController*>(meta->skyController.GetEntity());
			const idMaterial* sky = skyController->GetCurrentSky();
			if (sky)
				skyController->ReloadSky(); // Should also update any dynamic sky lights we've changed
		}
	}

	//update wristwatch room labels
	if (gameLocal.GetLocalPlayer() != nullptr)
	{
		gameLocal.GetLocalPlayer()->SetupArmstatRoomLabels(); //reload the map labels.
	}

	//update infostation room labels
	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (ent->IsType(idInfoStation::Type))
		{
			static_cast<idInfoStation*>(ent)->SetupRoomLabels();
		}
	}

	if (info.modifiedEntities.Num() <= 0 && info.addedEntities.Num() <= 0 && info.removedEntities.Num() <= 0)
		common->Printf("Hot reload: no changes found.\n");
	else
		common->Printf("Hot reload done. (updated: %d  added: %d  removed: %d)\n", info.modifiedEntities.Num(), info.addedEntities.Num(), info.removedEntities.Num());
}



void idGameLocal::ReloadLights()
{
	if (!mapFile)
	{
		common->Warning("HotReload: idGameLocal has no mapFile stored");
		return;
	}

	idMapReloadInfo info = mapFile->TryReload();

	if (info.cannotReload || info.mapInvalid)
		return;	

	int count = 0;

	//Load the lights from the map file.
	for (int i = 0; i < gameLocal.num_entities; i++)
	{
		if (!gameLocal.entities[i])
			continue;

		if (!gameLocal.entities[i]->IsType(idLight::Type))
			continue;

		const char *name = gameLocal.entities[i]->GetName();
		idMapEntity *newMapEnt = mapFile->FindEntity(name);

		if (newMapEnt == NULL)
			continue;

		idDict newArgs = newMapEnt->epairs;
		idEntity *ent = FindEntity(name);
		if (!ent)
		{
			continue;
		}

		gameEdit->EntityChangeSpawnArgs(ent, &newArgs);
		gameEdit->EntityUpdateChangeableSpawnArgs(ent, NULL);

		//if (1)
		//{
		//	idStr newModel = newArgs.GetString("model");
		//	gameEdit->EntitySetModel(ent, newModel);
		//}
		//
		//if (1)
		//{
		//	idStr newSkin = newArgs.GetString("skin");
		//
		//	if (newSkin.Length() > 0)
		//	{
		//		ent->PostEventMS(&EV_SetSkin, 0, newSkin);
		//	}
		//}

		if (1)
		{
			idVec3 newOrigin = newArgs.GetVector("origin");
			gameEdit->EntitySetOrigin(ent, newOrigin);
		}

		if (1)
		{
			idMat3 newAxis = newArgs.GetMatrix("rotation");
			gameEdit->EntitySetAxis(ent, newAxis);
		}

		if (1)
		{
			idVec3 newColor = newArgs.GetVector("_color");
			gameEdit->EntitySetColor(ent, newColor);
		}

		gameEdit->EntityUpdateVisuals(ent);
		count++;
	}

	if (info.modifiedEntities.Num() <= 0 && info.addedEntities.Num() <= 0 && info.removedEntities.Num() <= 0)
		common->Printf("Reload: no changes found.\n");
	else
		common->Printf("Reload done. (updated: %d)\n", count);
}


//This gets called when airlocks are opened, or vacuumseperators are toggled, or doors are opened.
void idGameLocal::DoGravityCheck()
{
	int i;
	idEntity *ent;	

	for (i = 0; i < MAX_GENTITIES; i++)
	{
		ent = entities[i];

		if (!ent)
			continue;

		if (ent->IsType(idPlayer::Type))
			continue;

		if ((ent->IsType(idItem::Type) || ent->IsType(idActor::Type) || ent->IsType(idDebris::Type) || ent->IsType(idProjectile::Type) || ent->IsType(idAnimated::Type)) && ent->spawnArgs.GetBool("zerog", "0"))
		{
			//these are floatable things. do the gravity check.
			ent->UpdateGravity();
		}
	}
}

//return TRUE if airless (outer space).
bool idGameLocal::GetAirlessAtPoint(idVec3 point)
{
	bool	newAirless = false;

	if (gameLocal.vacuumAreaNum != -1)
	{
		int		areaNum;		
		areaNum = gameRenderWorld->PointInArea(point);		

		if (areaNum < 0)
			return false;

		newAirless = gameRenderWorld->AreasAreConnected(gameLocal.vacuumAreaNum, areaNum, PS_BLOCK_AIR);
	}

	return newAirless;
}

void idGameLocal::ClearInterestPoints(void)
{
	static_cast<idMeta*>(metaEnt.GetEntity())->ClearInterestPoints();
}

bool idGameLocal::IsInEndGame()
{
	// SM: Super hack
	return persistentLevelInfo.GetInt("levelProgressIndex", "0") >= 22;
}

idEntity * idGameLocal::SpawnInterestPoint(idEntity *ownerEnt, idVec3 position, const char *interestDef)
{
	if (interestDef[0] == '\0' || gameLocal.time <= 500) //ignore interestpoints that happen immediately at game start. ignore if it has no definition name.
	{
		return NULL;
	}

	return static_cast<idMeta *>(metaEnt.GetEntity())->SpawnInterestPoint(ownerEnt, position, interestDef);
}

int idGameLocal::GetAmountEnemiesSeePlayer(bool onlyAlerted)
{
	int amountWhoCanSeePlayer = 0;

	//Iterate through all ai and see if all or none have line of sight. We piggyback onto the auto aim system.
	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (!entity->IsActive() || entity->IsHidden())
		{
			continue;
		}

		//Do not target friendlies.
		if (!entity->IsType(idActor::Type))
			continue;

		if (static_cast<idActor*>(entity)->team != 1)
			continue; //if you're not a bad guy, then we're SKIPPING you.


		//ok, we have a bad guy now.
		if (entity->IsType(idAI::Type) && entity->health > 0)
		{
			if (static_cast<idAI*>(entity)->lastFovCheck)
			{
				if (onlyAlerted && !static_cast<idAI*>(entity)->combatState)
					continue;

				amountWhoCanSeePlayer++;				
			}
		}		
	}

	return amountWhoCanSeePlayer;
}


//bc this gets called after everything has spawned and all systems are initialized. Called on every map load.
void idGameLocal::OnMapChange()
{
	int i;

	for (i = 0; i < MAX_GENTITIES; i++)
	{
		if (entities[i] && entities[i]->IsType(idLocationEntity::Type))
		{
			const char *reverbName = entities[i]->spawnArgs.GetString("reverb", "");

			if (reverbName[0] != 0)
			{			
				if (!gameSoundWorld->ReverbCheck(reverbName))
				{
					common->Error("info_location '%s' cannot find reverb setting: '%s'", entities[i]->GetName(), entities[i]->spawnArgs.GetString("reverb", ""));
				}
			}
		}
	}
	InitEventLog();
	GetLocalPlayer()->UpdatePDAInfo( false );
}

void idGameLocal::CloseEventLogFile(void)
{
	// Always try to close the file, but don't worry too much if it's missing.
	// (Remember that we may have toggled the cvar mid-session).
	if (eventLogFile)
	{
		eventLogFile->Printf("\nClosing log file...");
		eventLogFile->Flush();
		fileSystem->CloseFile(eventLogFile);
		eventLogFile = NULL;
	}
}

void idGameLocal::InitEventLog(void)
{
	eventlogGuiList = uiManager->AllocListGUI();
	eventlogGuiList->Clear();
	eventlogGuiList->Config(gameLocal.GetLocalPlayer()->eventlogMenu, "eventlist");
	eventlogGuiList->SetSelection(0);

	if (g_eventLog_logToFile.GetBool())
		InitEventLogFile(true);


	idWindow* newHud = static_cast<idUserInterfaceLocal*>(gameLocal.GetLocalPlayer()->hud)->GetDesktop();
	if (drawWin_t* alertDef = newHud->FindChildByName(LOG_FEED_ALERT_WINDOW))
	{
		eventLogAlerts = static_cast<idFeedAlertWindow*>(alertDef->win);
		assert(!eventLogAlerts->WasDisposed());
	} else {
		Warning("Missing LOG_FEED_ALERT_WINDOW: \"%s\" ", LOG_FEED_ALERT_WINDOW);
	}
}

void idGameLocal::ShutdownEventLog()
{
	if (eventlogGuiList)
	{
		eventLogList.Clear();
		uiManager->FreeListGUI(eventlogGuiList);
		eventlogGuiList = nullptr;
	}
}

void idGameLocal::InitEventLogFile(bool startOfSession)
{
	if (eventLogFile)
	{
		Warning("idGameLocal::InitEventLog: Previous event log was not closed properly. Closing now...");
		CloseEventLogFile();
	}

	struct tm* newtime;
	ID_TIME_T aclock;
	::time(&aclock);
	newtime = localtime(&aclock);

	idStr mapname = this->mapFileName;
	mapname.StripPath().StripFileExtension();

	idStr timestamp = TimeStampForEventLog(newtime);

	char* filename = va("eventlogs/eventlog_%s_%s.log", mapname.c_str(), timestamp.c_str());
	eventLogFile = fileSystem->OpenFileWrite(filename, "fs_configpath");

	eventLogFile->Printf("Started logging at %s on map %s\n\n", timestamp.c_str(), mapname.c_str());

	if (!startOfSession)
	{
		eventLogFile->Printf("Event log file was not opened at start of session -- playing back existing events...\n\n");
		if (eventlogGuiList)
		{
			if (eventlogGuiList->Num() > 0)
			{
				for (int i = 0; i < eventlogGuiList->Num(); i++)
				{
					// This is stupid, but in order to get elements out of the list, we need to select each one in turn
					char listItem[2048];
					eventlogGuiList->SetSelection(i);
					eventlogGuiList->GetSelection(listItem, 2048);
					eventLogFile->Printf(va("%s\n", listItem));
				}

				eventLogFile->Printf("\nFinished playback, resuming normal operation.\n\n");
			}
			else
			{
				eventLogFile->Printf("Event log list is empty -- nothing to record.\n");
			}
		}
		else
		{
			eventLogFile->Printf("Event log list not instantiated -- nothing to record.\n");
		}
	}
}

idStr idGameLocal::TimeStampForEventLog(tm* time)
{
	idStr out;
	out = va("%02d", time->tm_mday);
	out += "-";
	out += va("%02d", time->tm_mon + 1);
	out += "-";
	out += va("%d", time->tm_year + 1900);
	out += "_(";
	out += va("%02d", time->tm_hour);
	out += "-";
	out += va("%02d", time->tm_min);
	out += "-";
	out += va("%02d)", time->tm_sec);
	return out;
}

void idGameLocal::DisplayEventLogAlert(const char *text, const char * icon, idVec4 * textColor, idVec4 * bgColor, idVec4 * iconColor, float durationSeconds, bool allowDupes)
{
	idPlayer * localPlayer = gameLocal.GetLocalPlayer();
	if( !localPlayer )
	{
#ifdef _DEBUG
		Warning("Suppressed log feed alert, missing player: %s", text );
#endif
		return;
	}
	else if( !localPlayer->hud )
	{
#ifdef _DEBUG
		Warning("Suppressed log feed, missing hud: %s", text );
#endif
		return;
	}
	else if(!gameLocal.world->showEventLogInfoFeed || localPlayer->isInVignette)
	{
#ifdef _DEBUG
		gameLocal.Printf("Suppressed eventlog: '%s'\n", text );
#endif
		return;
	}

	if( eventLogAlerts && !eventLogAlerts->WasDisposed() )
	{
		if( idStr::Cmpn(text, STRTABLE_ID, STRTABLE_ID_LENGTH) == 0 )
		{
			text = common->GetLanguageDict()->GetString(text);
		}
		eventLogAlerts->DisplayAlert(text, icon, textColor, bgColor, iconColor, durationSeconds, allowDupes);
	}
}

void idGameLocal::AddEventLog(const char *text, idVec3 _position, bool showInfoFeed, int eventType, bool showDupes)
{
	if (gameLocal.GetLocalPlayer()->spectating)
		return;

	//This is the list of events for the spectate mode.
	eventlog_t newEvent;
	newEvent.name = common->GetLanguageDict()->GetString(text);
	newEvent.timestamp = gameLocal.time;
	newEvent.position = _position;
	newEvent.eventType = eventType;
	eventLogList.Append(newEvent);

	if (eventlogGuiList == NULL)
		return;

	const char *timeStr = ParseTimeDetailedMS(gameLocal.time);
	const char* entry = va("%s %s", timeStr, common->GetLanguageDict()->GetString(text));
	eventlogGuiList->Push(entry);

	if (showInfoFeed)
	{
		idStr iconName = "";
		idVec4 bgColor = idVec4(0, 0, 0, 1);
		idVec4 textColor = idVec4(1, 1, 1, 1);

		if (eventType == EL_INTERESTPOINT)
		{
			//interestpoint event.
			bgColor = idVec4(1, 1, 1, 1); //white
			textColor = idVec4(0, 0, 0, 1);
			iconName = "guis/assets/interestpoint_iconsquare";
			showDupes = false;
		}
		else if (eventType == EL_DEATH)
		{
			//death event.
			bgColor = idVec4(.9, 0, 0, 1); //red
			textColor = idVec4(1, 1, 1, 1);
			iconName = "guis/assets/eventlog_skull";
		}
		else if (eventType == EL_DESTROYED)
		{
			//death event.
			bgColor = idVec4(.9, 0, 0, 1); //red
			textColor = idVec4(1, 1, 1, 1);
			iconName = "guis/assets/eventlog_destroy";
		}
		else if (eventType == EL_DAMAGE)
		{
			//damage event.
			bgColor = idVec4(1, .6f, 0, 1); //orange
			textColor = idVec4(0, 0, 0, 1);
			iconName = "guis/assets/eventlog_damage";
		}
		else if (eventType == EL_HEAL)
		{
			//heal event.
			bgColor = idVec4(0, .5f, 0, 1); //green
			textColor = idVec4(1, 1, 1, 1);
			iconName = "guis/assets/eventlog_heal";
		}
		else if (eventType == EL_ALERT)
		{
			//interestpoint event.
			bgColor = idVec4(1, .8, 0, 1); //yellow
			textColor = idVec4(0, 0, 0, 1);
			iconName = "guis/assets/eventlog_alarm_wiggle";
		}
		
		// SW 19th March 2025: cutting down on interestpoint spam by adding showDupes arg
		DisplayEventLogAlert(common->GetLanguageDict()->GetString(text), iconName.c_str(), &textColor, &bgColor, &textColor, 0.0f, showDupes); 
	}

	if (g_eventLog_logToFile.GetBool())
	{
		if (!eventLogFile)
		{
			Warning("idGameLocal::AddEventLog: Tried to write to event log file before it was opened. Opening now...");
			InitEventLogFile(false);
		}
		else
		{
			eventLogFile->Printf("%s\n", entry);
		}
	}
}

//inflictor = the OWNER of the entity. i.e. the shooter of the bullet.
//attackerEnt = the entity that caused the damage. i.e. a bullet.
void idGameLocal::AddEventlogDamage(idEntity *target, int damage, idEntity *inflictor, idEntity *attackerEnt, const char *damageDefname)
{
	idStr attackerName = GetLogAttackerName(inflictor, attackerEnt, damageDefname);
	
	idStr myName;
	if (target->displayName.Length() > 0)
		myName = target->displayName;
	else
		myName = target->GetName();

	//Allow damage type to skip showing the damage event in the log.
	bool showEventLog = false;
	if (damageDefname != NULL)
	{
		if (damageDefname[0] != '\0')
		{
			const idDict* damageDef = gameLocal.FindEntityDefDict(damageDefname);
			if (damageDef)
			{
				showEventLog = damageDef->GetBool("showeventlog", "0");
			}
		}
	}

	AddEventLog(idStr::Format2(common->GetLanguageDict()->GetString("#str_def_gameplay_damagedby"),
		common->GetLanguageDict()->GetString(myName.c_str()),
		common->GetLanguageDict()->GetString(attackerName.c_str())), target->GetPhysics()->GetOrigin(), showEventLog, EL_DAMAGE);
}

void idGameLocal::AddEventlogDeath(idEntity *target, int damage, idEntity *inflictor, idEntity *attackerEnt, const char *damageDefname, int eventType)
{
	idStr attackerName = GetLogAttackerName(inflictor, attackerEnt, damageDefname);		

	idStr myName;
	if (target->displayName.Length() > 0)
		myName = target->displayName;
	else
		myName = target->GetName();

	if (attackerName == "???")
	{
		AddEventLog(idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_destroyed"), myName.c_str()), target->GetPhysics()->GetOrigin(), true, eventType);
		return;
	}
	else
	{
		
		AddEventLog(idStr::Format2(common->GetLanguageDict()->GetString("#str_def_gameplay_destroyedby"), myName.c_str(), common->GetLanguageDict()->GetString(attackerName.c_str())),
			target->GetPhysics()->GetOrigin(), true, eventType);
	}
}

//inflictor = the OWNER of the entity. i.e. the shooter of the bullet.
//attackerEnt = the entity that caused the damage. i.e. a bullet.
idStr idGameLocal::GetLogAttackerName(idEntity *inflictor, idEntity *attackerEnt, const char *damageDefname)
{
	//Try to find the best string for attacker.
	//Try to get a sanitized display-friendly name of attacker.

	
	//Fail, so fallback. Get name of the inflictor . Shotgun shell, conduit, etc.
	idStr inflictorName = "";
	if (inflictor != NULL)
	{
		if (inflictor->entityNumber != ENTITYNUM_WORLD && !inflictor->IsType(idActor::Type))
		{
			if (inflictor->displayName.Length() > 0)
				inflictorName = inflictor->displayName;
			else
				inflictorName = inflictor->GetName();
		}
	}

	if (inflictorName.Length())
	{
		//Inflictor Name
		return inflictorName.c_str();
	}





	//Get name of damage definition. Fire damage, Electric shock, etc.
	idStr damageName = "";
	if (damageDefname != NULL)
	{
		//Fail. Fall back to damagedef.
		if (damageDefname[0] != '\0')
		{
			const idDict *damageDef = gameLocal.FindEntityDefDict(damageDefname);
			if (damageDef)
			{
				idStr damageDisplayname = damageDef->GetString("displayname");
				if (damageDisplayname.Length() > 0)
				{
					damageName = damageDisplayname;
				}
				//BC 4-7-2025: do not fall back to the damage def name. We only want to display localized text.
				//else
				//{
				//	damageName = damageDefname;
				//}
			}
		}
	}

	if (damageName.Length())
	{
		//Damage Name
		return damageName.c_str();
	}


	//Get name of attack owner. Nina, Topaz, etc.
	idStr attackEntName = "";
	if (attackerEnt != NULL)
	{
		if (attackerEnt->entityNumber != ENTITYNUM_WORLD)
		{
			if (attackerEnt->displayName.Length() > 0)
			{
				attackEntName = attackerEnt->displayName;
			}
			else
			{
				attackEntName = attackerEnt->GetName();
			}
		}
	}

	if (attackEntName.Length())
	{
		//Attack Ent Name
		return attackEntName.c_str();
	}


	//Done gathering info.
	//Now we parse this info. Try to send the most useful display string.
	//Start at highest priority and continue on to lower priorities.

	//if (attackEntName.Length() > 0 && inflictorName.Length() > 0)
	//{
	//	//Attack Ent Name (Inflictor Name)
	//	return idStr::Format("%s (%s)", attackEntName.c_str(), inflictorName.c_str());
	//}
	//else if (damageName.Length() > 0 && inflictorName.Length() > 0)
	//{
	//	//Damage Name (Inflictor Name)
	//	return idStr::Format("%s (%s)", damageName.c_str(), inflictorName.c_str());
	//}
	//else if (attackEntName.Length())



	//Revisit inflictor again, but this time don't do the "ignore actor" check.
	if (inflictor != NULL)
	{
		if (inflictor->entityNumber != ENTITYNUM_WORLD)
		{
			if (inflictor->displayName.Length() > 0)
				inflictorName = inflictor->displayName;
			else
				inflictorName = inflictor->GetName();
		}
	}

	if (inflictorName.Length())
	{
		//Inflictor Name
		return inflictorName.c_str();
	}

	


	//Fail.
	return "???";
}

void idGameLocal::TestEventLogFeed()
{
	//test the infofeed message system.
	static int testLogIndex = 0;
	idStr sampleMessage = idStr::Format("sample message %d %d", testLogIndex, gameLocal.time);

	//swap between a default event log alert, and a special alert using various colors, icons, etc
	bool specialAlert = (testLogIndex % 2) == 0;
	gameLocal.AddEventLog(sampleMessage, gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin(), !specialAlert);
	if(specialAlert)
	{
		idStr randomMessage = idStr("Start ");
		static idRandom rand = idRandom(gameLocal.time+testLogIndex);
		int charExtra = rand.RandomInt(70);
		idStr charRand = idStr("      abcdefghijklmnopqrstuvwxyz");
		for( int i = 0 ; i < charExtra; i++)
		{
			randomMessage.Append( charRand[rand.RandomInt(charRand.Length()-1)] );
		}

		randomMessage.Replace("  ", " ");

		float randomTime = 15.0f;// 6.0f*rand.RandomFloat()+ 4.0f;
		randomMessage.Append( idStr::Format(" %.2fsec %d %d", randomTime, testLogIndex, gameLocal.time) );

		const char * bgs [] = { "guis/assets/buttonprompt", "guis/assets/circlecross_colored", "guis/assets/circle02"};
		idVec4 randColor1 = idVec4(rand.RandomFloat(),rand.RandomFloat(),rand.RandomFloat(),1.0f);
		idVec4 randColor2 = idVec4(rand.RandomFloat(),rand.RandomFloat(),rand.RandomFloat(),rand.RandomFloat());
		idVec4 randColor3 = idVec4(rand.RandomFloat(),rand.RandomFloat(),rand.RandomFloat(),rand.RandomFloat());

		gameLocal.DisplayEventLogAlert(randomMessage, bgs[testLogIndex%3],&randColor1,&randColor2,&randColor3,randomTime,false);
	}

	testLogIndex++;
}

bool idGameLocal::GetPositionIsInsideConfinedspace(idVec3 _targetPosition)
{
	for (idEntity* confinedTrigger = gameLocal.confinedEntities.Next(); confinedTrigger != NULL; confinedTrigger = confinedTrigger->confinedNode.Next())
	{
		if (!confinedTrigger)
			continue;

		if (!confinedTrigger->spawnArgs.GetBool("purge")) //ignore the Hide Triggers.
			continue;

		if (confinedTrigger->GetPhysics()->GetAbsBounds().ContainsPoint(_targetPosition))
		{
			return true;
		}
	}

	return false;
}

idEntity* idGameLocal::GetConfinedTriggerAtPos( const idVec3& targetPosition )
{
	for ( idEntity* confinedTrigger = gameLocal.confinedEntities.Next(); confinedTrigger != NULL; confinedTrigger = confinedTrigger->confinedNode.Next() )
	{
		if ( !confinedTrigger )
			continue;

		//if (!entity->spawnArgs.GetBool("purge")) //TODO: is this flag obsolete?
		//	continue;

		if ( confinedTrigger->GetPhysics()->GetAbsBounds().ContainsPoint( targetPosition ) )
		{
			return confinedTrigger;
		}
	}

	return NULL;
}

idEntity *idGameLocal::LaunchProjectile(idEntity *owner, const char *defName, idVec3 projectileSpawnPos, idVec3 projectileTrajectory)
{
	if (defName[0] == '\0')
	{
		return NULL;
	}

	idProjectile	*projectile;

	idEntity *		projectileEnt;
	idVec3			projectileVelocity;

	//Get the projectile definition.
	const idDict *	projectileDef;
	projectileDef = gameLocal.FindEntityDefDict(defName, false);
	if (projectileDef == NULL)
	{
		common->Warning("LaunchProjectile: failed to find def '%s'\n", defName);
		return NULL;
	}

	projectileVelocity = projectileDef->GetVector("velocity");
	gameLocal.SpawnEntityDef(*projectileDef, &projectileEnt, false);
	if (projectileEnt == NULL)
	{
		common->Warning("LaunchProjectile: failed to spawn entity '%s'\n", defName);
		return NULL;
	}

	//Spawn the projectile.
	projectile = (idProjectile *)projectileEnt;
	if (projectile == NULL)
	{
		common->Warning("LaunchProjectile: failed to cast idProjectile '%s'\n", defName);
		return NULL;
	}

	//Shoot the projectile.
	projectileTrajectory.Normalize();
	projectile->Create(owner, projectileSpawnPos, projectileTrajectory);
	projectile->Launch(projectileSpawnPos, projectileTrajectory, projectileTrajectory * projectileVelocity.x);
	return projectile;
}

//Check if the origin point is inside the entity's boundaries. This is mostly for brush-based entities; if the origin is outside the bounds then it sometimes creates problems where
//the game thinks an empty space is playable space (i.e. any empty space between a ship interior and hull)
bool idGameLocal::DoOriginContainmentCheck(idEntity *ent)
{
	if (!ent->GetPhysics()->GetAbsBounds().ContainsPoint(ent->GetPhysics()->GetOrigin()))
	{
#ifdef _DEBUG
		gameRenderWorld->DebugArrow(colorRed, ent->GetPhysics()->GetOrigin(), ent->GetPhysics()->GetAbsBounds().GetCenter(), 4, 60000);
		gameRenderWorld->DrawText(va("%s", ent->GetName()), ent->GetPhysics()->GetAbsBounds().GetCenter() + idVec3(0, 0, 6), .12f, colorRed, mat3_default, 1, 60000);
		gameRenderWorld->DrawText("origin is outside of brush.", ent->GetPhysics()->GetAbsBounds().GetCenter() + idVec3(0, 0, 2), .12f, colorRed, mat3_default, 1, 60000);
#endif
		return false;
	}

	return true;
}

//This is the code that tries to vacuum-pull actors out of a broken window.
void idGameLocal::DoVacuumSuctionActors(idVec3 suctionInteriorPosition, idVec3 suctionExteriorPosition, const idAngles& angles, bool pullPlayer)
{
	idVec3 suctionDirection = suctionExteriorPosition - suctionInteriorPosition;
	suctionDirection.Normalize();

	for (idEntity* entity = gameLocal.aimAssistEntities.Next(); entity != NULL; entity = entity->aimAssistNode.Next())
	{
		if (!entity)
			continue;

		if (!entity->spawnArgs.GetBool("zerog", "0"))
			continue;

		if (entity == gameLocal.GetLocalPlayer() && gameLocal.GetLocalPlayer()->noclip)
		{
			continue; //don't pull noclip'd player
		}

		//check if actor is in FRONT or BEHIND the suction point.
		idVec3 dirToEntity = suctionInteriorPosition - entity->GetPhysics()->GetOrigin();
		dirToEntity.Normalize();
		float facingResult = DotProduct(dirToEntity, suctionDirection);
		if (facingResult < 0 || entity->isInOuterSpace())
		{
			//Check if actor has LOS to exterior suction point. This is to handle situation where actor is in interior, but is pressed right up against the breaking glass.
			idVec3 dirToExteriorPosition = suctionExteriorPosition - entity->GetPhysics()->GetOrigin();
			dirToExteriorPosition.Normalize();
			float exteriorFacingResult = DotProduct(dirToExteriorPosition, suctionDirection);
			if (exteriorFacingResult > 0)
			{
				#define PROXIMITY_DISTANCE 160

				//do distance check.
				float distToInterior = (suctionInteriorPosition - entity->GetPhysics()->GetOrigin()).Length();
				float distToExterior = (suctionExteriorPosition - entity->GetPhysics()->GetOrigin()).Length();

				if (distToInterior < PROXIMITY_DISTANCE || distToExterior < PROXIMITY_DISTANCE)
				{
					float suctionForce = 256; //push force for: npc actor

					if (entity == gameLocal.GetLocalPlayer())
					{
						if (entity->isInOuterSpace())
							suctionForce = 512;		//push force for: player in outer space
						else
							suctionForce = 1024;	//push force for: player in interior
					}

					//between the suctionInterior spot and suctionExterior spot. Blow actor out the window.
					entity->ApplyImpulse(entity, 0, entity->GetPhysics()->GetAbsBounds().GetCenter(), suctionDirection * suctionForce * entity->GetPhysics()->GetMass());
				}
			}

			continue;
		}


		bool hasInitialBend = false;
		idVec3 initialBendPoint;
		//check if suction point has LOS to actor. If no LOS, then ignore.
		trace_t losTr;
		trace_t secondTr;
		gameLocal.clip.TracePoint(losTr, suctionInteriorPosition, entity->GetPhysics()->GetOrigin() + idVec3(0, 0, 1), MASK_SOLID, entity);
		if (losTr.fraction < 1)
		{
			//gameRenderWorld->DebugLine(colorRed, entity->GetPhysics()->GetOrigin(), losTr.endpos, 10000);
			// Try some extra spline point options:
			// An extra point in the direction parallel to suction direction
			idVec3 entityToInterior = suctionInteriorPosition - entity->GetPhysics()->GetOrigin();
			float scalarProject = DotProduct( suctionDirection, entityToInterior );
			initialBendPoint = entity->GetPhysics()->GetOrigin() + idVec3( 0, 0, 1 ) + suctionDirection * scalarProject;
			gameLocal.clip.TracePoint( losTr, entity->GetPhysics()->GetOrigin() + idVec3( 0, 0, 1 ), initialBendPoint, MASK_SOLID, entity );
			gameLocal.clip.TracePoint( secondTr, suctionInteriorPosition, initialBendPoint, MASK_SOLID, entity);
			if ( losTr.fraction < 1 || secondTr.fraction < 1 )
			{
				// Try one point perpendicular to the suction direction
				idVec3 right;
				angles.ToVectors( nullptr, &right, nullptr );
				if ( right.z > 0.5f )
				{
					// Sometimes the right vector is actually the up
					angles.ToVectors( nullptr, nullptr, &right );
				}
				initialBendPoint = entity->GetPhysics()->GetOrigin() + idVec3( 0, 0, 1 ) + right * 32.0f;
				gameLocal.clip.TracePoint( losTr, entity->GetPhysics()->GetOrigin() + idVec3( 0, 0, 1 ), initialBendPoint, MASK_SOLID, entity );
				gameLocal.clip.TracePoint( secondTr, suctionInteriorPosition, initialBendPoint, MASK_SOLID, entity );
				if ( losTr.fraction < 1 || secondTr.fraction < 1 )
				{
					// Try the other point perpendicular to the suction direction
					initialBendPoint = entity->GetPhysics()->GetOrigin() + idVec3( 0, 0, 1 ) + right * -32.0f;
					gameLocal.clip.TracePoint( losTr, entity->GetPhysics()->GetOrigin() + idVec3( 0, 0, 1 ), initialBendPoint, MASK_SOLID, entity );
					gameLocal.clip.TracePoint( secondTr, suctionInteriorPosition, initialBendPoint, MASK_SOLID, entity );
					if ( losTr.fraction < 1 || secondTr.fraction < 1 )
					{
						// If none of these options work, give up
						continue;
					}
				}
			}
			hasInitialBend = true;
		}

		if (entity == gameLocal.GetLocalPlayer() && !pullPlayer)
		{
			//if (pullPlayer)
			//{
			//	//Player is treated differently. Give player a gentle pull.
			//	dirToEntity.Normalize();
			//	gameLocal.GetLocalPlayer()->ApplyImpulse(0, 0, gameLocal.GetLocalPlayer()->GetPhysics()->GetOrigin() + idVec3(0, 0, 32), dirToEntity * 32768);
			//}
			continue;
		}

		// SW 17th Feb 2025: Normal actors are bound to the mover, so this condition would prevent movers fighting over actors.
		// However, the player *isn't* bound to the mover (to prevent clipping into walls) so we need to call the bespoke check here)
		if (entity == gameLocal.GetLocalPlayer() ? !gameLocal.GetLocalPlayer()->GetBeingVacuumSplined() : entity->GetBindMaster() == NULL)
		{
			//TODO: the -32 height offset assumes the aperture is NOT on ceiling or floor. Detect suction direction and adjust this accordingly

			//Entity is not bound to anything.
			//Do the vacuum spline stuff. Spawn the ent.			
			idEntity *vacuumEnt;
			idDict args;
			args.Set("classname", "func_vacuumspline");
			if ( hasInitialBend )
			{
				args.SetVector( "initialbend", initialBendPoint + idVec3( 0, 0, 32 ) ); //parallel to suction direction point
				//gameRenderWorld->DebugLine( colorGreen, entity->GetPhysics()->GetOrigin(), initialBendPoint + idVec3( 0, 0, 32 ), 50000 );
				//gameRenderWorld->DebugLine( colorGreen, initialBendPoint + idVec3( 0, 0, 32 ), suctionInteriorPosition + idVec3( 0, 0, -32 ), 50000 );
				//gameRenderWorld->DebugLine( colorGreen, suctionInteriorPosition + idVec3( 0, 0, -32 ), suctionExteriorPosition + idVec3( 0, 0, -32 ), 50000 );
			}
			args.SetVector("midpoint", suctionInteriorPosition  + (idVec3(0,0,-32))); //inner position (in interior)
			args.SetVector("endpoint", suctionExteriorPosition  + (idVec3(0,0,-32))); //outer position (in space)
			args.Set("actor", entity->GetName());

			if (entity == gameLocal.GetLocalPlayer())
			{
				args.SetBool("is_player", true);
			}

			gameLocal.SpawnEntityDef(args, &vacuumEnt);
		}
	}
}

void idGameLocal::DoVacuumSuctionItems(idVec3 suctionInteriorPosition, idVec3 suctionExteriorPosition)
{
	//Suck world items out.
	idLocationEntity *innerLocEnt = gameLocal.LocationForPoint(suctionInteriorPosition);
	if (innerLocEnt == NULL)
		return;

	idVec3 suctionDirection = suctionExteriorPosition - suctionInteriorPosition;
	suctionDirection.Normalize();

	for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent)
			continue;

		if (ent->IsType(idActor::Type) || ent->IsHidden() || !ent->spawnArgs.GetBool("zerog", "0"))
			continue;

		//Check if object is in the same room.
		idLocationEntity *entLoc = gameLocal.LocationForEntity(ent);
		if (entLoc == NULL)
			continue;

		if (entLoc->entityNumber != innerLocEnt->entityNumber)
			continue;

		//If it's behind the suctionInterior point, then ignore it.
		idVec3 dirToEntity = suctionInteriorPosition - ent->GetPhysics()->GetOrigin();
		dirToEntity.Normalize();
		float facingResult = DotProduct(dirToEntity, suctionDirection);
		if (facingResult < 0)
			continue;

		//entity is in the room.
		trace_t trInterior, trExterior;
		idVec3 pointToPullTo;
		gameLocal.clip.TracePoint(trInterior, ent->GetPhysics()->GetOrigin(), suctionInteriorPosition, MASK_SOLID, NULL);
		gameLocal.clip.TracePoint(trExterior, ent->GetPhysics()->GetOrigin(), suctionExteriorPosition, MASK_SOLID, NULL);
		if (trExterior.fraction < 1)
			pointToPullTo = suctionExteriorPosition;
		else if (trInterior.fraction < 1)
			pointToPullTo = suctionExteriorPosition;
		else
			continue; //no LOS, exit.

		//Apply physics impulse.
		#define	VACUUM_OBJECT_NORMALFORCE 128
		idVec3 entityCenter = ent->GetPhysics()->GetAbsBounds().GetCenter();
		idVec3 force = pointToPullTo - entityCenter;
		force.Normalize();
		ent->ApplyImpulse(ent, 0, entityCenter, force * VACUUM_OBJECT_NORMALFORCE * ent->GetPhysics()->GetMass());
	}
}

//For the teleport puck: do clearance checks and find a safe position for player to teleport to through wall.
idVec3 idGameLocal::GetPhasedDestination(trace_t collisionInfo, idVec3 startPosition)
{
    idVec3 collisionPos = collisionInfo.endpos;
    idVec3 throwDirection = collisionPos - startPosition;
    throwDirection.NormalizeFast();

    idBounds	playerbounds;
    playerbounds = gameLocal.GetLocalPlayer()->GetPhysics()->GetBounds();
    playerbounds[1].z = pm_crouchheight.GetFloat();

    idVec3 destinationPos = vec3_zero;

	// If the collision is a vent door, the destination is the entry point into the vent
	int entityNum = collisionInfo.c.entityNum;
	if ( entityNum > 0 && entityNum <= MAX_GENTITIES - 1 && entities[entityNum] && entities[entityNum]->IsType( idVentdoor::Type ) ) {
		destinationPos = static_cast< idVentdoor* >( entities[entityNum] )->GetPlayerDestinationPos();
		if ( destinationPos != vec3_zero ) {
			destinationPos.z += 3.0f;
			return destinationPos;
		}
	}

    for (int i = PHASE_MINDISTANCE; i < PHASE_MAXDISTANCE; i += 16)
    {
        idVec3 candidatePos = collisionPos + (throwDirection * i);

        //Is it in a solid.
        idLocationEntity *locationEntity = gameLocal.LocationForPoint(candidatePos);
        if (locationEntity == NULL)
            continue; //not a valid place...

		// If the candidate pos is in a constrained volume, we want to teleport to the center of the volume
		idEntity* confinedTrigger = GetConfinedTriggerAtPos( candidatePos );
		if ( GetPositionIsInsideConfinedspace( candidatePos ) ) {
			destinationPos = confinedTrigger->GetPhysics()->GetOrigin();
			destinationPos.z -= playerbounds[1].z / 2.0f;
			break;
		}

        trace_t boundTr;
        gameLocal.clip.TraceBounds(boundTr, candidatePos, candidatePos, playerbounds, MASK_SOLID, NULL);
        if (boundTr.fraction >= 1.0f)
        {
            //gameRenderWorld->DebugBounds(colorGreen, playerbounds, candidatePos, 10000);
            destinationPos = candidatePos;
            break;
        }
    }

    return destinationPos;
}

bool idGameLocal::InPVS_Entity(idEntity *ent, idVec3 targetPos) const
{
	bool			returnValue;
	pvsHandle_t		pvs;
	int				localPvsArea;

	localPvsArea = gameLocal.pvs.GetPVSArea(targetPos);
	pvs = gameLocal.pvs.SetupCurrentPVS(localPvsArea);
	returnValue = gameLocal.pvs.InCurrentPVS(pvs, ent->GetPVSAreas(), ent->GetNumPVSAreas());
	gameLocal.pvs.FreeCurrentPVS(pvs);

	return returnValue;
}

bool idGameLocal::InPVS_Pos(idVec3 observerPos, idVec3 targetPos) const
{
	bool			returnValue;
	pvsHandle_t		pvs;
	int				localPvsArea;

	localPvsArea = gameLocal.pvs.GetPVSArea(targetPos);
	pvs = gameLocal.pvs.SetupCurrentPVS(localPvsArea);
	returnValue = gameLocal.pvs.InCurrentPVS(pvs, observerPos);
	gameLocal.pvs.FreeCurrentPVS(pvs);

	return returnValue;
}



// ====================================== SUBTITLES ======================================

//The way this system works:
//Subtitles display in a list of slots. 
// Slot #2 is on top of the subtitle window.
// Slot #1
// Slot #0 is on bottom of the subtitle window.

//Subtitles try to appear in the highest-numbered available slot.
// blendo eric: use circle buffer / queue for subtitles,
//	newest overwrites oldest subtitles

void ResetSubtitleItem(idSubtitleItem& item)
{
	item.displayText = "";
	item.startTime = 0;
	item.expirationTime = 0;
	item.transitioned = false;
	item.rectHeight = 0.0f;
}

void idGameLocal::ResetSubtitleSlots()
{
	slotCircularIterator = 0;
	for (int i = 0; i < MAX_SUBTITLES; i++)
	{
		ResetSubtitleItem(subtitleItems[i]);
	}
}

idSubtitleItem& idGameLocal::GetSubtitleSlot(int slot)
{
	return subtitleItems[(slotCircularIterator + slot) % MAX_SUBTITLES];
}

idSubtitleItem& idGameLocal::QueueSubtitleSlot()
{
	// slotCircularIterator is considered to the be the first slot
	// new slots are always at the first position
	slotCircularIterator = slotCircularIterator - 1;
	if (slotCircularIterator < 0)
	{
		slotCircularIterator = MAX_SUBTITLES - 1;
	}
	idSubtitleItem& newItem = GetSubtitleSlot(0);
	ResetSubtitleItem(newItem);
	return newItem;
}

//Get how many subtitles are currently on-screen.
int idGameLocal::GetSubtitleCount()
{
	int amountUsed = 0;
	for (int i = 0; i < MAX_SUBTITLES; i++)
	{
		if (gameLocal.time > GetSubtitleSlot(i).expirationTime)
		{
			break;
		}

		amountUsed++;
	}

	return amountUsed;
}

//Call this when you want you want a subtitle to appear.
void idGameLocal::AddSubtitle( idStr speakerName, idStr text, int durationMS)
{
	//BC 3-14-2025: if subtitle is empty, don't display it.
	if (text.Length() <= 0)
	{
		if (speakerName.Length() <= 0)
		{
			gameLocal.Warning("Subtitle text is empty. Skipping.\n");
		}
		else
		{
			gameLocal.Warning("Subtitle text is empty. Speakername = '%s'\n", speakerName.c_str());
		}

		return;
	}

	// SW 26th Feb 2025:
	// Sometimes we get a large number of identical subtitles doubling-up (e.g. when a large number of wallspeakers all announce the end of the combat alert at once)
	// In situations like this, we check for a match and skip the subtitle if a match is found.
	for (int i = 0; i < GetSubtitleCount(); i++)
	{
		idSubtitleItem existingSub = GetSubtitleSlot(i);
		if (existingSub.displayText == text
			&& existingSub.speakerName == speakerName
			&& gameLocal.time - existingSub.startTime < 500)
		{
			// Subtitle has the same speaker and contents, and occurred at approximately the same time.
			// We shouldn't try to add a duplicate.
			return;
		}
	}

	durationMS = Max(durationMS, SUBTITLE_MINTIME); //clamp the duration to a minimum value. So that we don't end up with weird situations where a subtitle blips onscreen for just several frames.

	int fadeTimeMS = SEC2MS(gui_subtitleFadeTime.GetFloat());

	// if previous sub is currently fading out, just "remove" it so we're not transitioning into an invisible line
	if (GetSubtitleCount() == 1)
	{
		idSubtitleItem& prevSub = GetSubtitleSlot(0);
		if (prevSub.expirationTime - gameLocal.time < fadeTimeMS)
		{
			ResetSubtitleItem(prevSub);
		}
	}
	
	// Subtitles will queue into the first (slot 0) position, the lowest on-screen
	idSubtitleItem & newSub = QueueSubtitleSlot();
	newSub.speakerName = speakerName;
	newSub.displayText = text;
	newSub.startTime = gameLocal.time;
	newSub.expirationTime = gameLocal.time + durationMS;
	newSub.transitioned = false;
	newSub.rectHeight = 0.0f;

	int subCount = GetSubtitleCount();

	// if this is the only subtitle, no need to transition
	if (subCount == 1)
	{
		newSub.transitioned = true;
	}

	//Do some logic on the timers. We clamp the newly-added subtitle to be at least as long as the existing subtitles.
	//We do this in order to prevent weirdness where subtitles disappear in an order that feels weird / doesn't make sense.
	//For example:
	//	   VERY LONG SUBTITLE LINE
	//	   SHORT SUBTITLE LINE
	//We don't want the shorter line to disappear before the longer one, as it looks funky when that happens.
	// include extra fadeTime to the newest so both subs don't all fade in a big chunk

	if (subCount > 1)
	{
		idSubtitleItem& otherItem = GetSubtitleSlot(1);
		newSub.expirationTime = max(newSub.expirationTime, otherItem.expirationTime + fadeTimeMS);
	}

	//gameLocal.GetLocalPlayer()->hud->SetStateString(va("subtitle%d_text", availableIdx), text);
	//gameLocal.GetLocalPlayer()->hud->SetStateBool(va("subtitle%d_visible", availableIdx), true);
}

void idGameLocal::DrawSubtitles( idUserInterface* hud )
{
	if (!gui_subtitles.GetBool()) //if subtitles aren't on, then exit here.
		return;

	float scale = gui_subtitleScale.GetFloat();
	idVec4 color = idStr::ColorForIndex( gui_subtitleColor.GetInteger() );
	int speakerColorIdx = gui_subtitleSpeakerColor.GetInteger();
	float opacity = gui_subtitleBGOpacity.GetFloat();
	float width = gui_subtitleWidth.GetFloat();
	float targetYpos = gui_subtitleY.GetFloat();
	float maxY = 1000.0f;
	float gap = gui_subtitleGap.GetFloat();
	float fadeTime = gui_subtitleFadeTime.GetFloat()+0.00001f; // avoid div by 0
	float transitTime = gui_subtitleTransitionTime.GetFloat()+0.00001f; // avoid div by 0
	bool showSpeaker = gui_subtitleShowSpeaker.GetBool();

	int subCount = GetSubtitleCount();
	for (int i = 0; i < subCount; i++)
	{
		idSubtitleItem& subItem = GetSubtitleSlot(i);

		idStr displayText = (subItem.speakerName.Length() > 0 && showSpeaker) ?
			idStr::Format("^%d%s:^0 %s", speakerColorIdx, subItem.speakerName.c_str(), subItem.displayText.c_str()) //include speaker name.
			: idStr::Format("%s", subItem.displayText.c_str()); //do not include speaker name.


		// precalculate rect size if haven't yet
		if (subItem.rectHeight == 0.0f)
		{
			subItem.rectHeight = hud->DrawSubtitleText(displayText, scale, color, 320.0f, targetYpos, width, maxY, 1.0f, "sofia", true).h;
		}

		float transitionInterp = 1.0f;
		if( !subItem.transitioned )
		{
			transitionInterp = idMath::ClampFloat(0.0f, 1.0f, MS2SEC(gameLocal.time - subItem.startTime) / transitTime); // avoid div by 0
			if (transitionInterp > 0.9999f)
			{
				subItem.transitioned = true;
			}
		}
		// transitions rect from below the yPos to above it
		float curYPos = idMath::Lerp(targetYpos + gap, targetYpos - subItem.rectHeight, transitionInterp);

		float fade = idMath::ClampFloat(0.0f, 1.0f, MS2SEC(subItem.expirationTime - gameLocal.time) / fadeTime); // avoid div by 0
		fade *= transitionInterp;
		idVec4 colorFaded = color;
		colorFaded.w *= fade;
		float bgOpacity = opacity * fade;

		idRectangle rect = hud->DrawSubtitleText( displayText, scale, colorFaded, 320.0f, curYPos, width, maxY, bgOpacity, "sofia" );
		maxY = rect.y - gap; // next subtitle above should not pass this point, i.e. this value pushes next subtitle upwards (decreasing value of Y)
	}
}

bool idGameLocal::RunMapScript(idStr functionName)
{
	idCmdArgs args = idCmdArgs();
	return this->RunMapScript(functionName, args);
}

//Run a script function in the currently-loaded map script.
bool idGameLocal::RunMapScript(idStr functionName, const idCmdArgs& args)
{
	idStr scriptName = idStr::Format("%s::%s", GetMapNameStripped().c_str(), functionName.c_str());

	const function_t	*scriptFunction;
	scriptFunction = gameLocal.program.FindFunction(scriptName);
	if (scriptFunction)
	{
		idThread* thread = new idThread();
		for (int i = 2; i < args.Argc(); i++)
		{
			// SW: Try to push entities onto the stack, if there are any.
			// I'd love to support all argument types, but that's a rabbit hole.
			const char* arg = args.Argv(i);
			idEntity* ent = gameLocal.FindEntity(arg);
			if (ent != NULL)
			{
				thread->PushArg(ent);
			}
			else
			{
				gameLocal.Warning("Could not find entity arg '%s'\n", arg);
			}
		}
		thread->CallFunction(scriptFunction, false);
		thread->Start();

		return true;
	}

    gameLocal.Warning("Failed to find script function '%s'\n", scriptName.c_str());
	return false;
}

//This calls a map script and returns the 
bool idGameLocal::RunMapScriptArgs(idStr functionName, idEntity *activator, idEntity *callee)
{
	if (functionName.Length() <= 0)
		return false;

	const function_t	*scriptFunction;
	scriptFunction = gameLocal.program.FindFunction(functionName);
	if (scriptFunction)
	{
		assert(scriptFunction->parmSize.Num() <= 2);
		idThread *thread = new idThread();

		// 1 or 0 args always pushes activator (this preserves the old behavior)
		thread->PushArg(activator);

		// 2 args pushes self as well
		if (scriptFunction->parmSize.Num() == 2)
		{
			thread->PushArg(callee);
		}

		thread->CallFunction(scriptFunction, false);
		thread->DelayedStart(0);

		return true;
	}	

	return false;
}

void idGameLocal::SetGravityFromCVar()
{
	gravity.Set( 0, 0, -g_gravity.GetFloat() );
}

LoadingContext idGameLocal::GetLoadingContext()
{
	return LoadingContext( this );
}

void idGameLocal::AddLoadingContext()
{
	loadCount++;
}

void idGameLocal::RemoveLoadingContext()
{
	loadCount--;
	loadCount = Max( 0, loadCount );
}

//BC
void idGameLocal::CreateSparkObject(idVec3 position, idVec3 normal)
{
	//This creates a spark object, to ignite gas clouds.

	idEntity *ent;
	idDict args;
	args.Clear();
	args.SetVector("origin", position);	
	args.Set("classname", "env_spark");
	gameLocal.SpawnEntityDef(args, &ent);

	if (ent)
	{
		ent->GetPhysics()->SetGravity(idVec3(0, 0, -200)); //lower gravity.
		ent->GetPhysics()->SetLinearVelocity(normal * 96);
		static_cast<idMoveableItem *>(ent)->SetSparking();

		ent->PostEventMS(&EV_Remove, 500); //only exists for a short time in the world.
	}
}

//Determines if a collision will generate a spark.
bool idGameLocal::IsCollisionSparkable(const trace_t &collision)
{
	int			surfaceType;
	surfaceType = collision.c.material != NULL ? collision.c.material->GetSurfaceType() : SURFTYPE_NONE;
	return (surfaceType == SURFTYPE_METAL || surfaceType == SURFTYPE_STONE || surfaceType == SURFTYPE_RICOCHET || surfaceType == SURFTYPE_TILE); //These material types create sparks
}

bool idGameLocal::HasJockeyLOSToEnt(idEntity *ent)
{	
	//we're in third-person jockey mode and need to check if something is visible to the player.

	//first do a traceline from player camera.
	//Note: because we're in thirdperson, this is NOT from the player model eye. It's from the camera.
	
	idVec3 offset = ent->spawnArgs.GetVector("jockey_offset");
	idVec3 diamondWorldPos;

	if (offset.x != 0 || offset.y != 0 || offset.z != 0)
	{
		idVec3 forward, right, up;
		ent->GetPhysics()->GetAxis().ToAngles().ToVectors(&forward, &right, &up);
		diamondWorldPos = ent->GetPhysics()->GetOrigin() + (forward * offset[0]) + (right * offset[1]) + (up * offset[2]);
	}
	else
	{
		diamondWorldPos = ent->GetPhysics()->GetOrigin();
	}

	trace_t tr;
	gameLocal.clip.TracePoint(tr, gameLocal.GetLocalPlayer()->firstPersonViewOrigin, diamondWorldPos, MASK_SOLID, NULL);

	if (tr.c.entityNum == ent->entityNumber) //looking at the object.
		return true;

	//check if the trace check is close enough...
	float distToDiamond = (tr.endpos - diamondWorldPos).Length();
	if (distToDiamond <= 2)
		return true;

	return false;
}

idStr idGameLocal::GetShipName()
{
	idStr shipname = gameLocal.world->spawnArgs.GetString("shipname");
	if (shipname.Length() <= 0)
	{
		shipname = "#str_loc_unknown_00104";
		gameLocal.Warning("Can't find worldspawn 'shipname' value.\n");
	}
	return shipname;
}

idStr idGameLocal::GetShipDesc()
{
	idStr shipdesc = gameLocal.world->spawnArgs.GetString("shipdesc");
	if (shipdesc.Length() <= 0)
	{
		shipdesc = "#str_loc_unknown_00104";
		gameLocal.Warning("Can't find worldspawn 'shipdesc' value.\n");
	}
	return shipdesc;
}

idStr idGameLocal::GetMilestonenameViaLevelIndex(int levelIndex, int milestoneIndex)
{
	idStr levelInternalname = "";

	int num = declManager->GetNumDecls(DECL_MAPDEF);
	for (int i = 0; i < num; i++)
	{
		const idDeclEntityDef* mapDef = static_cast<const idDeclEntityDef*>(declManager->DeclByIndex(DECL_MAPDEF, i));
		if (!mapDef)
			continue;

		int candidateIndex = mapDef->dict.GetInt("levelindex", "-1");

		if (candidateIndex != levelIndex)		
			continue;
		
		//We found it. Great.
		levelInternalname = mapDef->dict.GetString("internalname");
		break; //exit loop.
	}

	if (levelInternalname.Length() <= 0)
	{
		//If the map def doesn't have an internal name.
		gameLocal.Warning("Map def for levelIndex '%d' is missing 'internalname'? (check maps.def)", levelIndex);
		return "";
	}


	//Contruct the milestone name.
	idStr milestoneName = idStr::Format("ms_%s_%d", levelInternalname.c_str(), milestoneIndex);
	return milestoneName;
}

void idGameLocal::DoSpewBurst(idEntity *ent, int spewType)
{
	//Spew.		
	idVec3 itemCenter = ent->GetPhysics()->GetAbsBounds().GetCenter();
	int radius = ent->spawnArgs.GetInt("spewRadius", "64");

	idDict args;
	args.Clear();
	args.SetVector("origin", itemCenter);
	args.SetVector("mins", idVec3(-radius, -radius, -radius));
	args.SetVector("maxs", idVec3(radius, radius, radius));

	idTrigger_Multi* spewTrigger;
	args.Set("spewParticle", ent->spawnArgs.GetString("spewParticle", "pepperburst01.prt"));
	args.SetInt("spewLifetime", ent->spawnArgs.GetInt("spewLifetime", "9000"));
	if (spewType == SPEWTYPE_PEPPER)
	{
		//Spawn pepper cloud.
		gameLocal.AddEventLog(idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_pepper_created"), ent->displayName.c_str()), ent->GetPhysics()->GetOrigin(), true, 0, false); // SW 6th May 2025: Adding showDupes = false
		args.SetFloat("multiplier", ent->spawnArgs.GetFloat("sneezemultiplier", "1"));
		args.Set("classname", "trigger_cloud_sneeze");
		spewTrigger = (idTrigger_sneeze*)gameLocal.SpawnEntityDef(args);
	}
	else if (spewType == SPEWTYPE_DEODORANT)
	{
		//Spawn a flammable cloud.		
		gameLocal.AddEventLog(idStr::Format(common->GetLanguageDict()->GetString("#str_def_gameplay_cloud_created"), ent->displayName.c_str()), ent->GetPhysics()->GetOrigin(), true, 0, false);
		args.Set("classname", "trigger_cloud_deodorant");
		spewTrigger = (idTrigger_deodorant*)gameLocal.SpawnEntityDef(args);
	}
	else
	{
		gameLocal.Error("Missing spew spawn logic in '%s'\n", ent->GetName());
	}
}

idStr idGameLocal::GetMapdisplaynameViaMapfile(idStr mapfilename)
{
	int num = declManager->GetNumDecls(DECL_MAPDEF);
	for (int i = 0; i < num; i++)
	{
		const idDeclEntityDef* mapDef = static_cast<const idDeclEntityDef*>(declManager->DeclByIndex(DECL_MAPDEF, i));
		if (mapDef && idStr::Icmp(mapDef->GetName(), mapfilename.c_str()) == 0)
		{
			//Found it.
			idStr output = common->GetLanguageDict()->GetString(mapDef->dict.GetString("name"));
			return output.c_str();
		}
	}

	return mapfilename;
}

idStr idGameLocal::GetMapdisplaynameViaProgressIndex(int index)
{
	int num = declManager->GetNumDecls(DECL_MAPDEF);
	for (int i = 0; i < num; i++)
	{
		const idDeclEntityDef* mapDef = static_cast<const idDeclEntityDef*>(declManager->DeclByIndex(DECL_MAPDEF, i));
		if (mapDef && mapDef->dict.GetInt("levelindex") == index)
		{
			//Found it.
			idStr output = common->GetLanguageDict()->GetString(mapDef->dict.GetString("name"));
			return output.c_str();
		}
	}

	return "???";
}

void idGameLocal::BindStickyItemViaTrace(idEntity* stickyItem, trace_t tr)
{
	idEntity* hitEnt;	
	idMat3 _axis;
	idVec3 _pos;

	if (tr.c.entityNum < 0 || tr.c.entityNum == ENTITYNUM_WORLD || tr.c.entityNum >= MAX_GENTITIES - 2)
		return;

	hitEnt = gameLocal.entities[tr.c.entityNum];

	if (!hitEnt)
		return;

	if (hitEnt->IsType(idWorldspawn::Type)) //idAFEntity_Base::Type
	{
		return;
	}

	if (hitEnt->IsType(idAI::Type)) //idAFEntity_Base::Type
	{
		jointHandle_t monsterJoint;
		monsterJoint = CLIPMODEL_ID_TO_JOINT_HANDLE(tr.c.id);

		if (monsterJoint == INVALID_JOINT)		
			return;

		stickyItem->BindToJoint(hitEnt, monsterJoint, true);
		return;
	}

	//BC 2-21-2025: bind/affix to entities.
	if (hitEnt->IsType(idMover::Type) || hitEnt->IsType(idMover_Binary::Type))
	{
		stickyItem->Bind(hitEnt, true);
		return;
	}

	gameRenderWorld->DebugBounds(colorGreen, hitEnt->GetPhysics()->GetAbsBounds());
	common->Printf("hit %s\n", hitEnt->GetName());
}


//bc going to do a game crime... just hard code these values
idStr idGameLocal::GetCatViaModel(idStr modelname, int* voiceprint)
{
	*voiceprint = 1;
	if (idStr::FindText(modelname.c_str(), "renfaire", false) >= 0)
	{
		*voiceprint = 0;
		return "#str_emailname_renfaire";
	}
	else if (idStr::FindText(modelname.c_str(), "chef", false) >= 0)
	{
		return "#str_emailname_chef";
	}
	else if (idStr::FindText(modelname.c_str(), "beatpoet", false) >= 0)
	{
		return "#str_emailname_poet";
	}
	else if (idStr::FindText(modelname.c_str(), "occultrockmusic", false) >= 0)
	{
		return "#str_emailname_metal";
	}
	else if (idStr::FindText(modelname.c_str(), "lovingcaringmom", false) >= 0)
	{
		*voiceprint = 0;
		return "#str_emailname_mom";
	}
	else if (idStr::FindText(modelname.c_str(), "skateboarder", false) >= 0)
	{
		*voiceprint = 0;
		return "#str_emailname_skateboard";
	}
	else if (idStr::FindText(modelname.c_str(), "stagemagician", false) >= 0)
	{
		return "#str_emailname_magician";
	}
	else if (idStr::FindText(modelname.c_str(), "cowboy", false) >= 0)
	{
		return "#str_emailname_cowboy";
	}
	else if (idStr::FindText(modelname.c_str(), "scubadiving", false) >= 0)
	{
		return "#str_emailname_scuba";
	}
	else if (idStr::FindText(modelname.c_str(), "writingyanovel", false) >= 0)
	{
		*voiceprint = 0;
		return "#str_emailname_yanovel";
	}
	else if (idStr::FindText(modelname.c_str(), "horrorfilmbuff", false) >= 0)
	{
		*voiceprint = 0;
		return "#str_emailname_horror";
	}
	else if (idStr::FindText(modelname.c_str(), "farmer", false) >= 0)
	{
		*voiceprint = 0;
		return "#str_emailname_farmer";
	}

	gameLocal.Warning("INVALID CAT MODEL? '%s'", modelname.c_str());
	return "";
}

idStr idGameLocal::GetBindingnameViaBind(idStr bindname)
{
	if (idStr::Cmp(bindname, "_moveup") == 0)
	{
		return "#str_gui_mainmenu_100425"; //Jump/dash/climb
	}
	else if (idStr::Cmp(bindname, "_movedown") == 0)
	{
		return "#str_gui_sign_crouch_100193"; //crouch
	}
	else if (idStr::Cmp(bindname, "_frob") == 0)
	{
		return "#str_gui_mainmenu_100427"; //interact
	}
	else if (idStr::Cmp(bindname, "_bash") == 0)
	{
		return "#str_gui_hud_100359"; //bash
	}
	else if (idStr::Cmp(bindname, "_zoom") == 0)
	{
		return "#str_gui_cryo_zoom_100035"; //zoom lens
	}
	else if (idStr::Cmp(bindname, "_lean") == 0)
	{
		return "#str_gui_mainmenu_100426"; //lean
	}
	else if (idStr::Cmp(bindname, "_quickthrow") == 0)
	{
		return "#str_gui_hud_100361"; //throw
	}
	else if (idStr::Cmp(bindname, "_attack") == 0)
	{
		return "#str_gui_mainmenu_100428"; //attack/use item
	}
	else if (idStr::Cmp(bindname, "_impulse15") == 0)
	{
		return "#str_gui_sign_switchitem_100208"; //previous item
	}
	else if (idStr::Cmp(bindname, "_impulse14") == 0)
	{
		return "#str_gui_sign_switchitem_100209"; //next item
	}
	else if (idStr::Cmp(bindname, "_reload") == 0)
	{
		return "#str_gui_mainmenu_100429"; //reload weapon
	}
	else if (idStr::Cmp(bindname, "_rackslide") == 0)
	{
		return "#str_gui_mainmenu_100430"; //rack weapon
	}
	else if (idStr::Cmp(bindname, "_contextmenu") == 0)
	{
		return "#str_gui_mainmenu_100431"; //context menu
	}

	gameLocal.Warning("GetBindingnameViaBind: unknown bind '%s'\n", bindname.c_str());
	return "";
}


// ------------- GAMEPAD FX / RUMBLE START -------------
//
const int GAMEPADFX_EVENT_MAX = 20; // max simultaneous gamepad fx
const float GAMEPADFX_SPIN_UP_TIME = 66; // time it takes to spin up motors to actually feel anything
const idVec3 GAMEPADFX_LIGHT_DEFAULT = idVec3(0.5f,0.5f,0.5f); // what LED lights defaults to when reset
const idVec3 GAMEPADFX_LIGHT_MIN = idVec3(0.012f,0.012f,0.012f); // minimum amount of light (so player knows controller is powered)
const float GAMEPADFX_RUMBLE_MIN = 0.0f;//0.0001f; // minimum amount of rumble, this can make the motor more responsive, minimizing spin up

GamePadFXSystem GamePadFXSystem::gamePadFXSys = GamePadFXSystem();

void GamePadFXSystem::Rumble_f(const idCmdArgs &args)
{
	if(args.Argc() < 3)
	{
		common->Printf("usage: rumble timeInMs[0,10000] lowFreqPct[0,1.0](intensity of thumpy motor) highFreqPct[0,1.0](default = matches low freq, buzzy motor)\n");
		common->Printf("\t\t(optional:) triggerLeftPct[def=0,1.0] triggerRightPct[def=0,1.0] \n");
		common->Printf("\t\t\tsineWavePeriodInMs[def=0,](starts at max, then reaches min at given val) channelId[-1,](0 = no replace, >0 = replace same id, -1 = replace all)\n");
		return;
	}

	if(args.Argc() < 4)
	{
		GetSys().AddRumble(atof(args.Argv(2)),atoi(args.Argv(1)));
		return;
	}

	gamePadFXEvent_t fxEvent = {};
	fxEvent.fxTypeFlag = TYPE_RUMBLE;
	fxEvent.timeLeftMs = atoi(args.Argv(1));
	fxEvent.rumblePctFreqLow = atof(args.Argv(2));
	fxEvent.rumblePctFreqHigh = atof(args.Argv(3));
	fxEvent.rumblePctTriggerLeft = atof(args.Argv(4));
	fxEvent.rumblePctTriggerRight = atof(args.Argv(5));
	fxEvent.timePeriodMs = atof(args.Argv(6));
	GetSys().AddFX(fxEvent);
}

void GamePadFXSystem::Light_f(const idCmdArgs &args)
{
	if(args.Argc() < 5)
	{
		common->Printf("usage: light timeInMs[0,10000] red[0,1] green[0,1] blue[0,1]\n");
		common->Printf("\t\t(optional:) sineWavePeriodInMs[def=0,](starts at max, then reaches min at given val) channelId[-1;0=def,](0 = no replace, >0 = replace same id, -1 = replace all)\n");
		return;
	}

	GetSys().AddLight(idVec3(atof(args.Argv(2)),atof(args.Argv(3)),atof(args.Argv(4))),atoi(args.Argv(1)));
}

void GamePadFXSystem::AddRumble( float intensityPct, int timeInMs )
{
	if (gamepad_rumble_enable.GetBool() == false)
		return;

	gamePadFXEvent_t fxEvent = {};
	fxEvent.fxTypeFlag = TYPE_RUMBLE;
	fxEvent.timeLeftMs = timeInMs;
	fxEvent.rumblePctFreqLow = intensityPct;
	fxEvent.rumblePctFreqHigh = intensityPct;
	AddFX(fxEvent);
}

void GamePadFXSystem::AddLight( idVec3 lightRGB, int timeInMs )
{
	gamePadFXEvent_t fxEvent = {};
	fxEvent.fxTypeFlag = TYPE_LIGHT;
	fxEvent.timeLeftMs = timeInMs;
	fxEvent.ledLight = lightRGB;
	AddFX(fxEvent);
}

void GamePadFXSystem::AddFX( gamePadFXEvent_t & inFX )
{
	if (gamepad_rumble_enable.GetBool() == false || !usercmdGen->IsUsingJoystick()) //BC 3-2-2025: only do rumble if joystick is active.
		return;

	int replaceIdx = -1;
	int aliveTimeUnique = 0;

	// replace unique fx if available
	if( inFX.uniqueID == ID_REPLACE_NONE ) { }
	else if( inFX.uniqueID == ID_REPLACE_ALL )
	{
		for(int idx = fxEventList.Num(); idx >= 0; idx--)
		{
			if( (fxEventList[idx].fxTypeFlag & inFX.fxTypeFlag) )
			{
				fxEventList.RemoveIndex(idx);
			}
		}
	}
	else
	{
		for( int idx = 0; idx < fxEventList.Num(); idx++ )
		{
			if( fxEventList[idx].uniqueID == inFX.uniqueID
				&& (fxEventList[idx].fxTypeFlag & inFX.fxTypeFlag) )
			{
				replaceIdx = idx;
				aliveTimeUnique = fxEventList[idx].timeAliveMs; // continue from this time
				break;
			}
		}
	}

	if(replaceIdx == -1)
	{
		if(fxEventList.Num() >= GAMEPADFX_EVENT_MAX)
		{ // remove oldest if at capacity
			replaceIdx = 0;
			float smallestTime = fxEventList[0].timeLeftMs;
			for(int idx = 1; idx < fxEventList.Num(); idx++)
			{
				if(fxEventList[idx].timeLeftMs < smallestTime)
				{
					replaceIdx = idx;
					smallestTime = smallestTime;
				}
			}
		}
		else
		{ // alloc new
			fxEventList.Append(gamePadFXEvent_t());
			replaceIdx = fxEventList.Num()-1;
		}
	}

	gamePadFXEvent_t newFX = inFX;
	newFX.timeAliveMs = inFX.timeAliveMs != 0 ? inFX.timeAliveMs : aliveTimeUnique;

	if( newFX.timeLeftMs < 0.0f )
	{
		gameLocal.Warning("gamepad fx time out of range: %d", newFX.timeLeftMs);
		newFX.timeLeftMs = 0.0f;
	}

	if( newFX.rumblePctFreqLow != 0.0f  || newFX.rumblePctFreqHigh != 0.0f  || newFX.rumblePctTriggerLeft  != 0.0f || newFX.rumblePctTriggerRight != 0.0f )
	{ // check if it has rumble
		newFX.rumblePctFreqLow = idMath::ClampFloat( 0.0f, 1.0f, newFX.rumblePctFreqLow );
		newFX.rumblePctFreqHigh = idMath::ClampFloat( 0.0f, 1.0f, newFX.rumblePctFreqHigh);
		newFX.rumblePctTriggerLeft = idMath::ClampFloat( 0.0f, 1.0f, newFX.rumblePctTriggerLeft);
		newFX.rumblePctTriggerRight = idMath::ClampFloat( 0.0f, 1.0f, newFX.rumblePctTriggerRight);
		newFX.rumblePctTriggerRight = idMath::ClampFloat( 0.0f, 1.0f, newFX.rumblePctTriggerRight);

		if( newFX.timeLeftMs > 10000 )
		{
			gameLocal.Warning("gamepad excessive rumble time: %d", newFX.timeLeftMs);
		}
		// adjust small time values for spin up time
		newFX.timeLeftMs = GAMEPADFX_SPIN_UP_TIME + newFX.timeLeftMs;
	}

	fxEventList[replaceIdx] = newFX;
}

void GamePadFXSystem::Reset()
{
	fxEventList.SetNum(0, false);
	Sys_SetGamepadRumble();
	Sys_SetGamepadLED(GAMEPADFX_LIGHT_DEFAULT.x,GAMEPADFX_LIGHT_DEFAULT.y,GAMEPADFX_LIGHT_DEFAULT.z);
}

void GamePadFXSystem::Update(int frameTimeMs)
{
	// max value per inFX event (total can add up to more)
	// this allows multiple heavy rumbles to be felt
	const float ATTENUATION_LIMIT = 0.75f;

	gamePadFXEvent_t totalFX = {};

	{ // get screen shake vals
		idPlayer * localPlayer = gameLocal.GetLocalPlayer();
		float shakeVolume = gameSoundWorld->CurrentShakeAmplitudeForPosition( gameLocal.slow.time, localPlayer->firstPersonViewOrigin );
		shakeVolume = idMath::ClampFloat(0.0f,1.0f,shakeVolume);
		float kickIntensity = idMath::Abs(localPlayer->playerView.AngleOffset().ToRotation().GetAngle());
		kickIntensity = idMath::ClampFloat(0.0f,1.0f,kickIntensity);

		float shakeCombined = shakeVolume + kickIntensity*(1.0f-shakeVolume);

		// smooth it out a bit, for shakes that are too quick
		static float shakeCombinedAccum = 0.0f;
		shakeCombined = Max( shakeCombined, shakeCombinedAccum );
		shakeCombinedAccum = Max( shakeCombined - frameTimeMs*2.0f, 0.0f );

		float rumbleFromScreenShake = GAMEPADFX_RUMBLE_MIN + (1.0f-GAMEPADFX_RUMBLE_MIN) * shakeCombined*gamepad_screenshake_rumble.GetFloat();
		totalFX.rumblePctFreqHigh = rumbleFromScreenShake;
		totalFX.rumblePctFreqLow = rumbleFromScreenShake;
		totalFX.ledLight = GAMEPADFX_LIGHT_MIN + (idVec3(1.0f,1.0f,1.0f)-GAMEPADFX_LIGHT_MIN) * shakeCombined * gamepad_screenshake_light.GetFloat();
	}

	bool active = (gamepad_fx_background.GetBool() || Sys_IsWindowActive()) && usercmdGen->IsUsingJoystick(); //BC 3-2-2025: only do rumble if joystick is active.
	if( active )
	{
		for(int idx = 0; idx < fxEventList.Num();)
		{
			gamePadFXEvent_t & curFx = fxEventList[idx];

			float attenValue = ATTENUATION_LIMIT;

			if(curFx.timePeriodMs > USERCMD_MSEC)
			{ //fluctuate intensity with sin
				float sinWave = idMath::Cos( (idMath::PI*(curFx.timeAliveMs-GAMEPADFX_SPIN_UP_TIME)) / curFx.timePeriodMs );
				attenValue *= sinWave*0.45f + 0.55f; // min sine intensity 0.1, to keep rumble motors from stalling
			}

			// attenuate intensity
			float attenRumble = attenValue * gamepad_rumble_intensity.GetFloat();
			totalFX.rumblePctFreqLow += (1.0f - totalFX.rumblePctFreqLow ) * curFx.rumblePctFreqLow*attenRumble;
			totalFX.rumblePctFreqHigh += (1.0f - totalFX.rumblePctFreqHigh ) * curFx.rumblePctFreqHigh*attenRumble;
			totalFX.rumblePctTriggerLeft += (1.0f - totalFX.rumblePctTriggerLeft) * curFx.rumblePctTriggerLeft*attenRumble;
			totalFX.rumblePctTriggerRight += (1.0f - totalFX.rumblePctTriggerRight) * curFx.rumblePctTriggerRight*attenRumble;

			idVec3 attenLight = (idVec3(1.0f,1.0f,1.0f) - totalFX.ledLight)* attenValue * gamepad_light_intensity.GetFloat();
			totalFX.ledLight += idVec3(attenLight.x*curFx.ledLight.x, attenLight.y*curFx.ledLight.y, attenLight.z*curFx.ledLight.z);

			int newTimeLeft = curFx.timeLeftMs - frameTimeMs;
			if(newTimeLeft > 0)
			{
				curFx.timeLeftMs = newTimeLeft;
				curFx.timeAliveMs += frameTimeMs;
				idx++;
			}
			else
			{ // end of life, swap with last event, then remove
				int lastIdx = fxEventList.Num() - 1;
				curFx = fxEventList[lastIdx];
				fxEventList.RemoveIndex(lastIdx);
			}
		}
	}
	else
	{
		totalFX.ledLight = GAMEPADFX_LIGHT_DEFAULT;
	}

	Sys_SetGamepadRumble(totalFX.rumblePctFreqLow,totalFX.rumblePctFreqHigh,totalFX.rumblePctTriggerLeft,totalFX.rumblePctTriggerRight);
	Sys_SetGamepadLED(totalFX.ledLight.x,totalFX.ledLight.y,totalFX.ledLight.z);

	if( gamepad_fx_debug.GetBool() )
	{
		gameLocal.Printf("rumble: %.4f low, %.4f hi, %.4f trigl, %.4f trigr\nlight: %.4f,%.4f,%.4f\n %s events: %d\n",
			totalFX.rumblePctFreqLow,totalFX.rumblePctFreqHigh,totalFX.rumblePctTriggerLeft,totalFX.rumblePctTriggerRight,
			totalFX.ledLight.x,totalFX.ledLight.y,totalFX.ledLight.z,
			active ? "active" : "inactive", fxEventList.Num());
	}
}


//
//------------- GAMEPAD FX / RUMBLE END -------------


// ------------- TEST FUNC / PROFILING  -------------
// blendo eric: this is just a helper area I use to test bits of code
#define USE_TEST_FUNC 0
#define USE_QUERYPERFORMANCECOUNTER 1

#if USE_TEST_FUNC
#if USE_QUERYPERFORMANCECOUNTER

	#include "profileapi.h"
	typedef int64 TESTFUNC_TIME;

	LARGE_INTEGER TF_LI_START; 
	LARGE_INTEGER TF_LI_END; 

	#define StartTimer() { \
			QueryPerformanceCounter(&TF_LI_START); \
		}

	#define StopTimer(timer) { \
			QueryPerformanceCounter( &TF_LI_END );	\
			timer += (TESTFUNC_TIME) (TF_LI_END.QuadPart - TF_LI_START.QuadPart); \
		}

#else

	#include <ctime>
	typedef std::clock_t TESTFUNC_TIME;
	TESTFUNC_TIME TF_TIME_START = 0;
	TESTFUNC_TIME TF_TIME_END = 0;
	#define StartTimer( )			\
		TF_TIME_START = clock();
	#define StopTimer(timer)				\
		TF_TIME_END = clock(); timer += TF_TIME_END-TF_TIME_START;

#endif

	void TestFuncPrint(const char* desc, TESTFUNC_TIME timer, TESTFUNC_TIME timer2 = (TESTFUNC_TIME)0)
	{

#if 0
#if USE_QUERYPERFORMANCECOUNTER
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		timer = (1000 * timer) / freq.QuadPart; // convert to ms
#endif
#endif
	if (timer2 != 0)
	{
		idLib::common->Printf("[test] %10s : %10d | %10d | %6d \n", desc, timer, timer2, (timer+timer2)/1000);
	}
	else
	{
		idLib::common->Printf("[test] %10s : %10d \n", desc, timer);
	}
}

#define TESTUNROLL100X(uf) { \
		uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; \
		uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; \
		uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; \
		uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; \
		uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; uf; \
	} \

void idGameLocal::TestTimedFunc_f(const idCmdArgs& args)
{
		gameLocal.TestTimedFunc(args);

		cmdSystem->BufferCommandText(CMD_EXEC_APPEND, "wait;wait;wait;wait;testfunc");

}
void idGameLocal::TestTimedFunc(const idCmdArgs& args)
{
	const int ExecTimes = 10;
	static TESTFUNC_TIME timerSum[20] = {};
	static int collectAll = 0;
	int collect = 0; // make sure code isn't optimized out by "collecting" discarded results

}
#else
void idGameLocal::TestTimedFunc_f(const idCmdArgs& args) {}
#endif

//
//  ------------- TEST FUNC / PROFILING  -------------

