// Copyright (C) 2007 Id Software, Inc.
//

#include "precompiled.h"
#pragma hdrstop

#if defined( _DEBUG ) && !defined( ID_REDIRECT_NEWDELETE )
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#if !defined( _XENON ) && !defined( MONOLITHIC )
idCVar* idCVar::staticVars;
#endif

#include "../framework/Licensee.h"
#include "../framework/LogitechLCDSystem.h"

#include "../bse/BSEInterface.h"
#include "../bse/BSE_Envelope.h"
#include "../bse/BSE_SpawnDomains.h"
#include "../bse/BSE_Particle.h"
#include "../bse/BSE.h"

#include "roles/Inventory.h"
#include "roles/FireTeams.h"
#include "roles/ObjectiveManager.h"
#include "roles/Tasks.h"
#include "roles/WayPointManager.h"
#include "structures/TeamManager.h"
#include "rules/GameRules.h"
#include "Player.h"
#include "../framework/BuildVersion.h"
#include "WorldSpawn.h"
#include "Misc.h"
#include "Camera.h"
#include "client/ClientEffect.h"
#include "client/ClientMoveable.h"
#include "gamesys/SysCmds.h"
#include "Trigger.h"
#include "structures/DeployMask.h"
#include "Projectile.h"
#include "vehicles/VehicleSuspension.h"
#include "vehicles/VehicleView.h"
#include "vehicles/Pathing.h"
#include "vehicles/Transport.h"
#include "vehicles/VehicleWeapon.h"
#include "vehicles/SoundControl.h"
#include "../framework/declManager.h"
#include "../framework/KeyInput.h"
#include "../decllib/DeclSurfaceType.h"
#include "structures/DeployRequest.h"
#include "CommandMapInfo.h"
#include "demos/DemoManager.h"
#include "anim/Anim_FrameCommands.h"
#include "rules/UserGroup.h"
#include "rules/AdminSystem.h"
#include "rules/VoteManager.h"
#include "Atmosphere.h"
#include "misc/Door.h"
#include "Light.h"
#include "Sound.h"
#include "guis/UserInterfaceManager.h"
#include "guis/UserInterfaceLocal.h"
#include "guis/GuiSurface.h"
#include "../decllib/declTypeHolder.h"
#include "./effects/Wakes.h"
#include "./effects/TireTread.h"
#include "./effects/FootPrints.h"
#include "ContentMask.h"
#include "proficiency/StatsTracker.h"
// Gordon: FIXME: Fix the case of this directory
#include "Waypoints/LocationMarker.h"

#include "script/Script_Helper.h"
#include "script/Script_Program.h"
#include "script/ScriptEntityHelpers.h"
#include "script/Script_ScriptObject.h"
#include "script/Script_DLL.h"

#include "../framework/AdManager.h"
#include "../framework/GraphManager.h"

#include "../idlib/PropertiesImpl.h"

#include "../renderer/Image.h"
#include "../renderer/DeviceContext.h"
#include "../decllib/declRenderBinding.h"

#ifdef _XENON
#include "../xenon/live/LiveManager.h"
#include "../xenon/live/LiveService.h"
#else
#include "../sdnet/SDNet.h"
#include "../sdnet/SDNetUser.h"
#include "../sdnet/SDNetProfile.h"
#endif

#include "botai/BotThread.h"
#include "botai/BotThreadData.h"
#include "botai/Bot.h"

#include "misc/ProfileHelper.h"
#include "AntiLag.h"

#if !defined( _XENON ) && !defined( MONOLITHIC )
idSys *						sys = NULL;
idCommon *					common = NULL;
idCmdSystem *				cmdSystem = NULL;
idCVarSystem *				cvarSystem = NULL;
idFileSystem *				fileSystem = NULL;
idNetworkSystem *			networkSystem = NULL;
idRenderSystem *			renderSystem = NULL;
sdDeviceContext *			deviceContext = NULL;
idSoundSystem *				soundSystem = NULL;
idRenderModelManager *		renderModelManager = NULL;
idDeclManager *				declManager = NULL;
idCollisionModelManager *	collisionModelManager = NULL;
idAASFileManager*			AASFileManager = NULL;
sdNetService*				networkService = NULL;
sdAdManager*				adManager = NULL;
sdKeyInputManager*			keyInputManager = NULL;
sdNotificationSystem*		notificationSystem = NULL;
sdGraphManager*				graphManager = NULL;
#else
extern sdNotificationSystem*	notificationSystem;
#endif

#if !defined( _XENON )
idRenderWorld *				gameRenderWorld = NULL;
idSoundWorld *				gameSoundWorld = NULL;
#endif

idList< surfaceProperties_t > idGameLocal::surfaceTypes;

#if !defined( _XENON ) && !defined( MONOLITHIC )
rvBSEManager *				bse = NULL;

idCVar com_forceGenericSIMD( "com_forceGenericSIMD", "0", CVAR_BOOL | CVAR_SYSTEM | CVAR_NOCHEAT, "force generic platform independent SIMD" );
idCVar com_timescale( "timescale", "1", CVAR_FLOAT | CVAR_SYSTEM | CVAR_NETWORKSYNC, "scales the time", 0.1f, 10.0f );
idCVar com_timeServer( "com_timeServer", "0", CVAR_SYSTEM | CVAR_INTEGER | CVAR_NOCHEAT, "" );
#else
extern idCVar com_forceGenericSIMD;
#endif
#ifndef MONOLITHIC
idCVar bse_simple( "bse_simple", "0", CVAR_BOOL | CVAR_ARCHIVE, "simple versions of effects" );
#endif

static gameExport_t			gameExport;

// global animation lib
idAnimManager				animationLib;

// the rest of the engine will only reference the "game" variable, while all local aspects stay hidden
idGameLocal					gameLocal;

#if !defined( MONOLITHIC )
idGame *					game = &gameLocal;	// statically pointed at an idGameLocal
#else
idGame *					game;	// statically pointed at an idGameLocal
#endif

#ifdef MONOLITHIC
#include "monolithic_dependencies.h"

volatile void ForceLink( void ) {
#include "monolithic_types.h"
}
#endif

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
		deviceContext				= import->deviceContext;
		soundSystem					= import->soundSystem;
		renderModelManager			= import->renderModelManager;
		declManager					= import->declManager;
		collisionModelManager		= import->collisionModelManager;
		bse							= import->bse;
#ifndef _XENON
		networkService				= import->networkService;
#endif
		adManager					= import->adManager;
		keyInputManager				= import->keyInputManager;
		AASFileManager				= import->AASFileManager;
		notificationSystem			= import->notificationSystem;
		graphManager				= import->graphManager;

#if !defined( _XENON ) && !defined( MONOLITHIC )
		idDict::SetGlobalPools( import->globalKeys, import->globalValues );
		idStr::SetStringAllocator( import->stringAllocator );
		idWStr::SetStringAllocator( import->wideStringAllocator );
#endif
	}

	// set interface pointers used by idLib
	idLib::sys					= sys;
	idLib::common				= common;
	idLib::cvarSystem			= cvarSystem;
	idLib::fileSystem			= fileSystem;

#if defined( _XENON ) || defined( MONOLITHIC )
	game = &gameLocal;
#endif

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
===============================================================================

	sdPlayZone

===============================================================================
*/

/*
================
sdPlayZone::sdPlayZone
================
*/
sdPlayZone::sdPlayZone( const idDict& info, const idBounds& bounds ) {
	_bounds		= bounds;
	_size		= _bounds.GetMaxs() - _bounds.GetMins();
	_priority	= info.GetInt( "priority", "0" );
	_flags		= 0;
	_target		= info.GetString( "target" );
	const char* titleName = info.GetString( "title" );

	if( titleName[ 0 ] != '\0' ) {
		_title = declHolder.declLocStrType.LocalFind( titleName, true );
	}
	if ( info.GetBool( "zone_commandmap" ) ) {
		_flags	|= PZF_COMMANDMAP;
	}
	if ( info.GetBool( "zone_playzone" ) ) {
		_flags	|= PZF_PLAYZONE;
	}
	if ( info.GetBool( "zone_deployment" ) ) {
		_flags	|= PZF_DEPLOYMENT;
	}
	if ( info.GetBool( "zone_paths" ) ) {
		_flags	|= PZF_PATH;
	}
	if ( info.GetBool( "zone_heightmap" ) ) {
		_flags	|= PZF_HEIGHTMAP;
	}
	if ( info.GetBool( "zone_vehicle_route" ) ) {
		_flags	|= PZF_VEHICLEROUTE;
	}	
	if ( info.GetBool( "zone_world" ) ) {
		_flags	|= PZF_WORLD;
	}
	if ( info.GetBool( "zone_choosable" ) ) {
		_flags	|= PZF_CHOOSABLE;
	}

	_commandmapMaterial			= gameLocal.declMaterialType[ info.GetString( "mtr_commandmap" ) ];

	const char* heightMapName = info.GetString( "hm_heightmap" );
	if ( *heightMapName != '\0' ) {
		_heightMap.Init( heightMapName, bounds );
	}

	const idKeyValue* kv;

	kv = NULL;
	while ( kv = info.MatchPrefix( "path_", kv ) ) {
		const idStr& key = kv->GetKey();

		const sdDeclVehiclePath* path = gameLocal.declVehiclePathType[ kv->GetValue().c_str() ];
		if ( !path ) {
			gameLocal.Warning( "Invalid Path '%s' on Playzone '%s'", kv->GetValue().c_str(), info.GetString( "name" ) );
			continue;
		}

		playzonePath_t& p = _paths.Alloc();
		p.first		= key.Right( key.Length() - 5 );
		p.second	= new sdVehiclePathGrid( path, bounds );
	}

	kv = NULL;
	while ( kv = info.MatchPrefix( "dm_", kv ) ) {
		qhandle_t handle = gameLocal.GetDeploymentMask( kv->GetKey() );
		if ( _masks.Num() <= handle ) {
			_masks.SetNum( handle + 1 );
		}
		_masks[ handle ].Init( kv->GetValue(), sdBounds2D( _bounds ) );
	}
}

/*
================
sdPlayZone::GetMask
================
*/
const sdDeployMaskInstance* sdPlayZone::GetMask( qhandle_t handle ) const {
	if ( handle < 0 || handle >= _masks.Num() ) {
		return NULL;
	}

	if ( !_masks[ handle ].IsValid() ) {
		return NULL;
	}

	return &_masks[ handle ];
}

/*
================
sdPlayZone::SaveMasks
================
*/
void sdPlayZone::SaveMasks( void ) {
	for ( int i = 0; i < _masks.Num(); i++ ) {
		sdDeployMaskInstance& mask = _masks[ i ];
		if ( !mask.IsValid() ) {
			continue;
		}

		mask.WriteTGA();
	}
}

/*
================
sdPlayZone::GetPath
================
*/
sdVehiclePathGrid* sdPlayZone::GetPath( const char* name ) const {
	for ( int i = 0; i < _paths.Num(); i++ ) {
		if ( _paths[ i ].first.Icmp( name ) != 0 ) {
			continue;
		}

		return _paths[ i ].second;
	}

	return NULL;
}

/*
================
sdPlayZone::sdPlayZone
================
*/
sdPlayZone::~sdPlayZone( void ) {
	for ( int i = 0; i < _paths.Num(); i++ ) {
		delete _paths[ i ].second;
	}
	_paths.Clear();
}

/*
===============================================================================

	sdLoggedTrace

===============================================================================
*/

extern const idEventDef EV_GetTraceBody;
extern const idEventDef EV_GetTraceFraction;
extern const idEventDef EV_GetTraceEndPos;
extern const idEventDef EV_GetTracePoint;
extern const idEventDef EV_GetTraceNormal;
extern const idEventDef EV_GetTraceEntity;
extern const idEventDef EV_GetTraceSurfaceFlags;
extern const idEventDef EV_GetTraceSurfaceType;
extern const idEventDef EV_GetTraceSurfaceColor;
extern const idEventDef EV_GetTraceJoint;

CLASS_DECLARATION( idClass, sdLoggedTrace )
	EVENT( EV_GetTraceFraction,			sdLoggedTrace::Event_GetTraceFraction )
	EVENT( EV_GetTraceEndPos,			sdLoggedTrace::Event_GetTraceEndPos )
	EVENT( EV_GetTracePoint,			sdLoggedTrace::Event_GetTracePoint )
	EVENT( EV_GetTraceNormal,			sdLoggedTrace::Event_GetTraceNormal )
	EVENT( EV_GetTraceEntity,			sdLoggedTrace::Event_GetTraceEntity )
	EVENT( EV_GetTraceSurfaceFlags,		sdLoggedTrace::Event_GetTraceSurfaceFlags )
	EVENT( EV_GetTraceSurfaceType,		sdLoggedTrace::Event_GetTraceSurfaceType )
	EVENT( EV_GetTraceSurfaceColor,		sdLoggedTrace::Event_GetTraceSurfaceColor )
	EVENT( EV_GetTraceJoint,			sdLoggedTrace::Event_GetTraceJoint )
	EVENT( EV_GetTraceBody,				sdLoggedTrace::Event_GetTraceBody )
END_CLASS

/*
================
sdLoggedTrace::sdLoggedTrace
================
*/
sdLoggedTrace::sdLoggedTrace( void ) {
	memset( &trace, 0, sizeof( trace ) );
	object = gameLocal.program->AllocScriptObject( this, gameLocal.program->GetDefaultType() );
}

/*
================
sdLoggedTrace::~sdLoggedTrace
================
*/
sdLoggedTrace::~sdLoggedTrace( void ) {
	gameLocal.program->FreeScriptObject( object );
}

/*
================
sdLoggedTrace::Init
================
*/
void sdLoggedTrace::Init( int _index, const trace_t& _trace ) {
	index	= _index;
	trace	= _trace;
}

/*
================
sdLoggedTrace::Event_GetTraceFraction
================
*/
void sdLoggedTrace::Event_GetTraceFraction( void ) {
	sdProgram::ReturnFloat( trace.fraction );
}

/*
================
sdLoggedTrace::Event_GetTraceEndPos
================
*/
void sdLoggedTrace::Event_GetTraceEndPos( void ) {
	sdProgram::ReturnVector( trace.endpos );
}

/*
================
sdLoggedTrace::Event_GetTracePoint
================
*/
void sdLoggedTrace::Event_GetTracePoint( void ) {
	sdProgram::ReturnVector( trace.c.point );
}

/*
================
sdLoggedTrace::Event_GetTraceNormal
================
*/
void sdLoggedTrace::Event_GetTraceNormal( void ) {
	if ( trace.fraction < 1.0f ) {
		sdProgram::ReturnVector( trace.c.normal );
	} else {
		sdProgram::ReturnVector( vec3_origin );
	}
}

/*
================
sdLoggedTrace::Event_GetTraceEntity
================
*/
void sdLoggedTrace::Event_GetTraceEntity( void ) {
	if ( trace.fraction < 1.0f ) {
		sdProgram::ReturnEntity( gameLocal.entities[ trace.c.entityNum ] );
	} else {
		sdProgram::ReturnEntity( NULL );
	}
}

/*
================
sdLoggedTrace::Event_GetTraceJoint
================
*/
void sdLoggedTrace::Event_GetTraceJoint( void ) {
	if ( trace.fraction < 1.0f && trace.c.id < 0 ) {
		idEntity* ent = gameLocal.entities[ trace.c.entityNum ];
		if ( ent->GetAnimator() != NULL ) {
			sdProgram::ReturnString( ent->GetAnimator()->GetJointName( CLIPMODEL_ID_TO_JOINT_HANDLE( trace.c.id ) ) );
			return;
		}
	}
	sdProgram::ReturnString( "" );
}

/*
================
sdLoggedTrace::Event_GetTraceBody
================
*/
void sdLoggedTrace::Event_GetTraceBody( void ) {
	if ( trace.fraction < 1.0f && trace.c.id < 0 ) {
		idAFEntity_Base *af = static_cast<idAFEntity_Base *>( gameLocal.entities[ trace.c.entityNum ] );
		if ( af && af->IsType( idAFEntity_Base::Type ) && af->IsActiveAF() ) {
			int bodyId = af->BodyForClipModelId( trace.c.id );
			idAFBody *body = af->GetAFPhysics()->GetBody( bodyId );
			if ( body ) {
				sdProgram::ReturnString( body->GetName() );
				return;
			}
		}
	}
	sdProgram::ReturnString( "" );
}

/*
================
sdLoggedTrace::Event_GetTraceSurfaceFlags
================
*/
void sdLoggedTrace::Event_GetTraceSurfaceFlags( void ) {
	if ( trace.fraction < 1.0f && trace.c.material != NULL ) {
		sdProgram::ReturnInteger( trace.c.material->GetSurfaceFlags() );
		return;
	}
	sdProgram::ReturnInteger( 0 );
}

/*
================
sdLoggedTrace::Event_GetTraceSurfaceType
================
*/
void sdLoggedTrace::Event_GetTraceSurfaceType( void ) {
	if ( trace.fraction < 1.0f && trace.c.surfaceType ) {
		sdProgram::ReturnString( trace.c.surfaceType->GetName() );
	} else {
		sdProgram::ReturnString( "" );
	}
}

/*
================
sdLoggedTrace::Event_GetTraceSurfaceColor
================
*/
void sdLoggedTrace::Event_GetTraceSurfaceColor( void ) {
	sdProgram::ReturnVector( trace.c.surfaceColor );
}


/*
===============================================================================

	sdEntityCollection

===============================================================================
*/

/*
================
sdEntityCollection::Remove
================
*/
void sdEntityCollection::Remove( idEntity* entity ) {
	if ( !Contains( entity ) ) {
		return;
	}
	for ( int i = 0; i < list.Num(); i++ ) {
		if ( list[ i ].GetEntity() == entity ) {
			list.RemoveIndex( i );
			break;
		}
	}
	mask.Clear( entity->entityNumber );
}


/*
===============================================================================

	sdEntityDebugInfo

===============================================================================
*/

/*
===========
sdEntityDebugInfo::sdEntityDebugInfo
============
*/
sdEntityDebugInfo::sdEntityDebugInfo( void ) {
	lastOperationTime		= 0;
	lastOperation			= OP_NONE;
	type					= NULL;
	name					= "";
}

/*
===========
sdEntityDebugInfo::OnCreate
============
*/
void sdEntityDebugInfo::OnCreate( idEntity* self ) {
	lastOperationTime		= gameLocal.time;
	lastOperation			= OP_CREATE;
	type					= self->GetType();
	name					= self->GetName();
}

/*
===========
sdEntityDebugInfo::OnDestroy
============
*/
void sdEntityDebugInfo::OnDestroy( idEntity* self ) {
	lastOperationTime		= gameLocal.time;
	lastOperation			= OP_DESTROY;
}

/*
===========
sdEntityDebugInfo::PrintStatus
============
*/
void sdEntityDebugInfo::PrintStatus( int index ) {
	gameLocal.Printf( "Current Time: %i\n", gameLocal.time );
	gameLocal.Printf( "Entity Number: %i\n", index );

	switch ( lastOperation ) {
		case OP_NONE:
			gameLocal.Printf( "No Information Found\n" );
			break;

		case OP_CREATE:
			gameLocal.Printf( "Created At: %i\n", lastOperationTime );
			gameLocal.Printf( "Type: %s\n", type->classname );
			gameLocal.Printf( "Name: %s\n", name.c_str() );
			break;

		case OP_DESTROY:
			gameLocal.Printf( "Destroyed At: %i\n", lastOperationTime );
			gameLocal.Printf( "Type: %s\n", type->classname );
			gameLocal.Printf( "Name: %s\n", name.c_str() );
			break;

	}
}


/*
===============================================================================

	idGameLocal

===============================================================================
*/

idCVar idGameLocal::g_cacheDictionaryMedia( "g_cacheDictionaryMedia", "1", CVAR_GAME | CVAR_BOOL | CVAR_NOCHEAT | CVAR_RANKLOCKED, "Precache all media from entity dictionaries" );

/*
===========
idGameLocal::idGameLocal
============
*/
idGameLocal::idGameLocal( void ) :
	systemNotifications( 1 ) {
	rulesStateObject.Init( this );

	proficiencyLog = NULL;
	networkLog = NULL;
	objectiveLog = NULL;
#if !defined( SD_DEMO_BUILD )
	clientStatsRequestTask = NULL;
#endif /* !SD_DEMO_BUILD */
	mapMetaDataList = NULL;
	campaignMetaDataList = NULL;

	memset( deployRequests, 0, sizeof( deployRequests ) );

	com_unlockFPS = NULL;

	Clear();
}

/*
===========
idGameLocal::Clear
============
*/
void idGameLocal::Clear( void ) {
	int i;

	msec = USERCMD_MSEC;

	ClearDeployRequests();

	serverInfo.Clear();
	numClients = 0;
	memset( usercmds, 0, sizeof( usercmds ) );
	memset( clientConnected, 0, sizeof( clientConnected ) );
	memset( clientRanks, 0, sizeof( clientRanks ) );
#if !defined( SD_DEMO_BUILD )
	clientStatsRequestIndex = -1;
	clientStatsRequestsPending = false;
#endif /* !SD_DEMO_BUILD */
	entities.Memset( 0 );
	memset( spawnIds, -1, sizeof( spawnIds ) );
	firstFreeIndex = 0;
	numEntities = 0;
	spawnedEntities.Clear();
	activeEntities.Clear();
	activeNetworkEntities.Clear();
	networkedEntities.Clear();
	nonNetworkedEntities.Clear();
	changedEntities.Clear();
	interpolateEntities.Clear();
	sortPushers = false;
	sortTeamMasters = false;
	random.SetSeed( 0 );
	world = NULL;
	frameCommandThread = NULL;
	testmodel = NULL;
	clip.Shutdown();
	pvs.Shutdown();
	editEntities = NULL;
	entityHash.Clear( 1024, MAX_GENTITIES );
	framenum = 0;
	previousTime = 0;
	time = 0;
	timeOffset = 0;
	localViewChangedTime = 0;
	nextTeamBalanceCheckTime = 0;
	damageLogFile = NULL;
	debugLogFile = NULL;

	insideExecuteMapChange = false;

	doingMapRestart = false;
	reloadingSameMap = true;
	mapFileName.Clear();
	mapFile = NULL;

    botMapFile = NULL;

	spawnCount = INITIAL_SPAWN_COUNT;
	camera = NULL;
	spawnArgs.Clear();
	gravity.Set( 0, 0, -1 );
	gamestate = GAMESTATE_UNINITIALIZED;
	rules = NULL;
	SetSnapShotPlayer( NULL );
	SetSnapShotClient( NULL );

	numEntityDefBits = 0;
	numDamageDeclBits = 0;
	numInvItemBits = 0;
	numSkinDeclBits = 0;
	numDeployObjectBits = 0;
	numPlayerClassBits = 0;
	numClientIndexBits = 0;

	localClientNum = ASYNC_DEMO_CLIENT_INDEX;
	isServer = false;
	isClient = false;
	isRepeater = false;
	serverIsRepeater = false;
	snapShotClientIsRepeater = false;
	repeaterClientFollowIndex = -1;
	realClientTime = 0;
	isNewFrame = true;
	isPaused = false;

	globalMaterial = NULL;
	newInfo.Clear();

	entityNetEventQueue.Clear();
	savedEntityNetEventQueue.Clear();

	loggedTraces.SetNum( MAX_LOGGED_TRACES );
	for ( i = 0; i < MAX_LOGGED_TRACES; i++ ) {
		loggedTraces[ i ] = NULL;
	}

	loggedDecals.SetNum( MAX_LOGGED_DECALS );
	for ( i = 0; i < MAX_LOGGED_DECALS; i++ ) {
		loggedDecals[ i ] = NULL;
	}

	clientSpawnedEntities.Clear();
	memset( clientEntities, 0, sizeof( clientEntities ) );
	for (int i=0; i<MAX_CENTITIES; i++) {
		clientSpawnIds[i] = -INITIAL_SPAWN_COUNT;
	}

	uiMainMenuHandle.Release();
	uiLevelLoadHandle.Release();
	pureWaitHandle.Release();
	uiSystemUIHandle.Release();

	mapMetaData		= NULL;
	mapInfo			= NULL;

	for ( int i = 0; i < MAX_BATTLESENSE_RANKS; i++ ) {
		battleSenseBonus[ i ] = NULL;
	}

	lagoMaterial = NULL;

	memset( &unlock, 0, sizeof( unlock ) );
	unlock.minAngles.Set( -180.0f, -180.0f, -180.0f );
	unlock.maxAngles.Set( 180.0f, 180.0f, 180.0f );

	nextBotPopulationCheck = 0;
	
	useSimpleEffect = bse_simple.GetBool();
}

/*
===============
idGameLocal::GetBotFPS
===============
*/
int idGameLocal::GetBotFPS( void ) const {
	return botThreadData.GetBotFPS();
}

/*
===============
idGameLocal::GetRandomBotName
===============
*/
bool idGameLocal::GetRandomBotName( int clientNum, idStr& botName ) {
	bool foundName = botThreadData.FindRandomBotName( clientNum, botName );
	return foundName;
}

/*
===============
idGameLocal::GetBotDebugInfo
===============
*/
botDebugInfo_t idGameLocal::GetBotDebugInfo( int clientNum ) {
	if ( !botThreadData.ClientIsValid( clientNum ) ) { //mal: someone goofed and try to pass an invalid client.
		botDebugInfo_t botInfo;
		memset( &botInfo, 0, sizeof( botInfo ) );
		return botInfo;
	}
	return botThreadData.GetBotDebugInfo( clientNum );
}

#if defined( SD_PUBLIC_BUILD )
idCVar g_useCompiledScript( "g_useCompiledScript", "1", CVAR_GAME | CVAR_BOOL | CVAR_ARCHIVE, "enable/disable native compiled scripts" );
#else
idCVar g_useCompiledScript( "g_useCompiledScript", "0", CVAR_GAME | CVAR_BOOL | CVAR_ARCHIVE, "enable/disable native compiled scripts" );
#endif // SD_PUBLIC_BUILD

/*
===========
idGameLocal::LoadScript
============
*/
void idGameLocal::LoadScript( void ) {
	if ( mapInfo == NULL ) {
		return;
	}

	// before scripts are loaded, shut down old threads, etc
	sdTeamManager::GetInstance().OnScriptChange();
	if ( rules ) {
		rules->OnScriptChange();
	}
	sdObjectiveManager::GetInstance().OnScriptChange();
	sdTaskManager::GetInstance().Init();
	sdTaskManager::GetInstance().OnScriptChange();

	if ( program != NULL && program->IsValid() ) {
		const sdProgram::sdFunction* func = program->FindFunction( "game_shutdown" );
		if ( func && frameCommandThread ) {
			frameCommandThread->CallFunction( func );
			if ( !frameCommandThread->Execute() ) {
				Error( "idGameLocal::LoadScript Shutdown Cannot be Blocking" );
			}
		}
	}

	if ( frameCommandThread != NULL ) {
		gameLocal.program->FreeThread( frameCommandThread );
		frameCommandThread	= NULL;
	}

	delete program;
	program = NULL;

	if ( g_useCompiledScript.GetBool() ) {
		program = new sdDLLProgram();
		if ( !program->Init() ) {
			delete program;
			program = NULL;
		}
	}

	if ( program == NULL ) {
		program = new idProgram();
		program->Init();
	}

	UpdateLevelLoadScreen( common->LocalizeText( "guis/mainmenu/loading/compiling_scripts" ).c_str() );

	InitScriptForMap();

	// after scripts are loaded, load new objects
	sdTeamManager::GetInstance().OnNewScriptLoad();
	if ( rules ) {
		rules->OnNewScriptLoad();
	}
	sdObjectiveManager::GetInstance().OnNewScriptLoad();
	sdTaskManager::GetInstance().OnNewScriptLoad();
}

// crash here so we can try to get a stack trace
void WinPureCallHandler( void ) {
	*( int* )( 0x00000000 ) = 7;
}

class idCVarCallback_TeamForceBalanceChanged_BotControl : public idCVarCallback {
public:	
	~idCVarCallback_TeamForceBalanceChanged_BotControl() {
		si_teamForceBalance.UnRegisterCallback( this );
	}

	void Register( void ) {
		si_teamForceBalance.RegisterCallback( this );
	}

	virtual void OnChanged( void ) {
		if ( !gameLocal.isClient && !networkSystem->IsRankedServer() ) {
			if( si_teamForceBalance.GetBool() ) {
				bot_uiNumStrogg.SetInteger( -1 );
				bot_uiNumGDF.SetInteger( -1 );
			}
		}
	}
} g_updateTeamForceBalance_BotControl;

class idCVarCallback_BotControl : public idCVarCallback {
public:
	idCVarCallback_BotControl( idCVar& watch ) {
		_watch = &watch;
	}

	~idCVarCallback_BotControl() {
		_watch->UnRegisterCallback( this );
	}

	void Register( void ) {
		_watch->RegisterCallback( this );
	}

	virtual void OnChanged( void ) {
		if ( !gameLocal.isClient ) {
			if( _watch->GetInteger() != -1 && !networkSystem->IsRankedServer() ) {
				si_teamForceBalance.SetBool( false );
			}
		}
	}

	idCVar* _watch;
}	g_updateNumGDF_BotControl( bot_uiNumGDF ), 
	g_updateNumStrogg_BotControl( bot_uiNumStrogg );

class idCVarCallback_ServerInfoDirectSet : public idCVarCallback {
public:
	idCVarCallback_ServerInfoDirectSet( idCVar& watch ) {
		_watch = &watch;
	}

	~idCVarCallback_ServerInfoDirectSet() {
		_watch->UnRegisterCallback( this );
	}

	void Register( void ) {
		_watch->RegisterCallback( this );
	}

	virtual void OnChanged( void ) {
		if ( !gameLocal.isClient ) {
			sys->SetServerInfo( _watch->GetName(), _watch->GetString() );
		}
	}

	idCVar* _watch;
}	g_updateTimeLimit( si_timeLimit ), 
	g_updateMaxPlayer( si_maxPlayers ), 
	g_updatePrivateClients( si_privateClients ), 
	g_updateTeamForceBalance( si_teamForceBalance ),
	g_updateAdminStart( si_adminStart ),
	g_updateMinPlayers( si_minPlayers ),
	g_updateReadyPercent( si_readyPercent ),
	g_updateDisableVoting( si_disableVoting ),
	g_updateServerName( si_name ),
	g_updateNoProficiency( si_noProficiency ),
	g_updateTeamDamage( si_teamDamage ),
	g_updateNeedPass( si_needPass ),
	g_updateDisableGlobalChat( si_disableGlobalChat ),
	g_updateGameReviewReadyWait( si_gameReviewReadyWait );


class idCVarCallback_CommandMapZoom : public idCVarCallback {
public:
	idCVarCallback_CommandMapZoom( idCVar& watch ) {
		_watch = &watch;
	}

	~idCVarCallback_CommandMapZoom() {
		_watch->UnRegisterCallback( this );
	}

	void Register( void ) {
		_watch->RegisterCallback( this );
	}

	virtual void OnChanged( void ) {
		idPlayer::SetupCommandMapZoom();
	}

	idCVar* _watch;
} g_commandMapZoomCvarCallback( g_commandMapZoom );

class idCVarCallback_BotMinClients : public idCVarCallback {
public:
	idCVarCallback_BotMinClients( idCVar& watch ) {
		_watch = &watch;
	}

	~idCVarCallback_BotMinClients() {
		_watch->UnRegisterCallback( this );
	}

	void Register( void ) {
		_watch->RegisterCallback( this );
	}

	virtual void OnChanged( void ) {
		if ( bot_minClients.GetInteger() > bot_minClientsMax.GetInteger() ) {
			bot_minClients.SetInteger( bot_minClientsMax.GetInteger() );
		}
	}

	idCVar* _watch;
} bot_minClientsCvarCallback( bot_minClients ),
	bot_minClientsMaxCvarCallback( bot_minClientsMax );

/*void ParseNetworkLog( void ) {
	idFile* file2 = fileSystem->OpenFileWrite( "network_dump.log" );

	idLexer parser( "network.log" );

	while ( true ) {
		if ( parser.EndOfFile() ) {
			break;
		}

		idStr str;
		parser.ParseCompleteLine( str );

		if ( str.Cmpn( "Entity Broadcast: ", 18 ) != 0 ) {
			continue;
		}

		file2->WriteFloatString( "%s", str.c_str() );
	}

	fileSystem->CloseFile( file2 );
}

void ParseNetworkLog( void ) {
	idFile* file2 = fileSystem->OpenFileWrite( "network_dump.log" );

	idLexer parser( "network_dump.csv" );

	struct entryData_t {
		int totalSize;
		int count;
		int type;
	};

	struct typeData_t {
		idList< entryData_t >	entries;
	};

	idHashMap< typeData_t* > types;

	while ( true ) {
		if ( parser.EndOfFile() ) {
			break;
		}

		idToken name;
		parser.ReadToken( &name );

		parser.ExpectTokenString( "," );

		idToken valueString;
		parser.ReadToken( &valueString );

		parser.ExpectTokenString( "," );

		idToken sizeString;
		parser.ReadToken( &sizeString );

		parser.SkipRestOfLine();

		typeData_t** dataPtr;
		typeData_t* data;
		if ( types.Get( name.c_str(), &dataPtr ) ) {
			data = *dataPtr;
		} else {
			data = new typeData_t;
			types.Set( name.c_str(), data );
		}

		int value = valueString.GetIntValue();
		int size = sizeString.GetIntValue();

		int i;
		for ( i = 0; i < data->entries.Num(); i++ ) {
			if ( data->entries[ i ].type == value ) {
				break;
			}
		}
		if ( i == data->entries.Num() ) {
			entryData_t& newEntry = data->entries.Alloc();
			newEntry.type = value;
			newEntry.count = 0;
			newEntry.totalSize = 0;
		}

		data->entries[ i ].count++;
		data->entries[ i ].totalSize += size;
	}

	for ( int i = 0; i < types.Num(); i++ ) {
		typeData_t** dataPtr = types.GetIndex( i );
		assert( dataPtr != NULL );
		typeData_t* data = *dataPtr;

		for ( int j = 0; j < data->entries.Num(); j++ ) {
			file2->WriteFloatString( "%s,%d,%d,%d\n", types.GetKey( i ).c_str(), data->entries[ j ].type, data->entries[ j ].count, data->entries[ j ].totalSize );
		}
	}

	fileSystem->CloseFile( file2 );
}*/

/*
===========
idGameLocal::LoadLifeStatsData
============
*/
void idGameLocal::LoadLifeStatsData( void ) {
	const sdDeclStringMap* lifeStataData = declStringMapType[ "life_stats_data" ];
	if ( lifeStataData == NULL ) {
		gameLocal.Warning( "No Life Stats Data Found" );
		return;
	}

	const idDict& info = lifeStataData->GetDict();

	const idKeyValue* kv = NULL;
	while ( ( kv = info.MatchPrefix( "life_stat_", kv ) ) != NULL ) {
		const char* statName = kv->GetValue().c_str();
		if ( *statName == '\0' ) {
			continue;
		}

		const char* textName = info.GetString( va( "text_%s", kv->GetKey().c_str() ) );
		const sdDeclLocStr* text = declHolder.declLocStrType[ textName ];
		if ( text == NULL ) {
			gameLocal.Warning( "sdDeclPlayerClass::ReadFromDict Bad Text '%s' for Life Stat %s", textName, kv->GetKey().c_str() );
			continue;
		}

		sdStringBuilder_Heap textNameLong( va( "%s_long", textName ) );
		const sdDeclLocStr* textLong = declHolder.declLocStrType[ textNameLong.c_str() ];
		if ( textLong == NULL ) {
			gameLocal.Warning( "sdDeclPlayerClass::ReadFromDict Bad Long Text '%s' for Life Stat %s", textNameLong.c_str(), kv->GetKey().c_str() );
			continue;
		}

		lifeStat_t& stat = lifeStats.Alloc();
		stat.stat = statName;
		stat.text = text;
		stat.textLong = textLong;
		stat.isTimeBased = info.GetBool( va( "timebased_%s", kv->GetKey().c_str() ) );
	}
}

/*
===========
idGameLocal::Init

  initialize the game object, only happens once at startup, not each level load
============
*/
void idGameLocal::Init( void ) {
#ifdef _WIN32
	_set_purecall_handler( WinPureCallHandler );
#endif // _WIN32

	// initialize idLib
	idLib::Init();

	// register static cvars declared in the game
	idCVar::RegisterStaticVars();

	g_updateTimeLimit.Register();
	g_updateMaxPlayer.Register();
	g_updatePrivateClients.Register();
	g_updateTeamForceBalance.Register();
	g_updateAdminStart.Register();
	g_updateDisableVoting.Register();
	g_updateMinPlayers.Register();
	g_updateReadyPercent.Register();
	g_updateServerName.Register();
	g_updateNoProficiency.Register();
	g_updateTeamDamage.Register();
	g_updateNeedPass.Register();
	g_updateGameReviewReadyWait.Register();
	g_updateDisableGlobalChat.Register();

	g_updateTeamForceBalance_BotControl.Register();
	g_updateNumGDF_BotControl.Register();
	g_updateNumStrogg_BotControl.Register();

	bot_minClientsCvarCallback.Register();
	bot_minClientsMaxCvarCallback.Register();

	idWeapon::RegisterCVarCallback();
	sdDeclAOR::InitCVars();

	sdAnimFrameCommand::Init();

	sdStatsTracker::Init();

	sdStatsTracker& statsTracker = sdGlobalStatsTracker::GetInstance();

	totalShotsFiredStat = statsTracker.GetStat( statsTracker.AllocStat( "total_shots_fired", sdNetStatKeyValue::SVT_INT ) );
	totalShotsHitStat	= statsTracker.GetStat( statsTracker.AllocStat( "total_shots_hit", sdNetStatKeyValue::SVT_INT ) );

	// initialize processor specific SIMD
	idSIMD::InitProcessor( "game", com_forceGenericSIMD.GetBool() );

	Printf( "--------- Initializing Game ----------\n" );
	Printf( "gamename: %s\n", GAME_VERSION );
	Printf( "gamedate: %s\n", __DATE__ );

	idStr versionString = "LOADING GAME:";
	common->PrintLoadingMessage( versionString + " REGISTERING DECLARATIONS" );

	// register game specific decl types
	declManager->RegisterDeclType( &declModelDefType );
	declManager->RegisterDeclType( &declExportDefType );
	declManager->RegisterDeclType( &declVehicleScriptDefType );
	declManager->RegisterDeclType( &declAmmoTypeType );
	declManager->RegisterDeclType( &declInvSlotType );
	declManager->RegisterDeclType( &declInvItemTypeType );
	declManager->RegisterDeclType( &declInvItemType );
	declManager->RegisterDeclType( &declItemPackageType );
	declManager->RegisterDeclType( &declStringMapType );
	declManager->RegisterDeclType( &declDamageType );
	declManager->RegisterDeclType( &declDamageFilterType );
	declManager->RegisterDeclType( &declCampaignType );
	declManager->RegisterDeclType( &declQuickChatType );
	declManager->RegisterDeclType( &declMapInfoType );
	declManager->RegisterDeclType( &declToolTipType );
	declManager->RegisterDeclType( &declTargetInfoType );
	declManager->RegisterDeclType( &declProficiencyTypeType );
	declManager->RegisterDeclType( &declProficiencyItemType );
	declManager->RegisterDeclType( &declRankType );
	declManager->RegisterDeclType( &declDeployableObjectType );
	declManager->RegisterDeclType( &declDeployableZoneType );
	declManager->RegisterDeclType( &declPlayerClassType );
	declManager->RegisterDeclType( &declGUIType );
	declManager->RegisterDeclType( &declTeamInfoType );
	declManager->RegisterDeclType( &declPlayerTaskType );
	declManager->RegisterDeclType( &declRequirementType );
	declManager->RegisterDeclType( &declGUIThemeType );
	declManager->RegisterDeclType( &declVehiclePathType );
	declManager->RegisterDeclType( &declKeyBindingType );
	declManager->RegisterDeclType( &declRadialMenuType );
	declManager->RegisterDeclType( &declAORType );
	declManager->RegisterDeclType( &declRatingType );
	declManager->RegisterDeclType( &declHeightMapType );
	declManager->RegisterDeclType( &declDeployMaskType );

	idDeclTypeInterface* declInterface = declManager->GetDeclType( declSurfaceTypeIdentifier );
	declInterface->RegisterPostParse( idGameLocal::SurfaceTypePostParse );

	declTableType.Init( declTableIdentifier );
	declMaterialType.Init( declMaterialIdentifier );
	declSkinType.Init( declSkinIdentifier );
	declSoundShaderType.Init( declSoundShaderIdentifier );
	declEntityDefType.Init( declEntityDefIdentifier );
	declAFType.Init( declAFIdentifier );
	declEffectsType.Init( declEffectsIdentifier );
	declAtmosphereType.Init( declAtmosphereIdentifier );
	declStuffTypeType.Init( declStuffTypeIdentifier );
	declDecalType.Init( declDecalIdentifier );
	declSurfaceTypeType.Init( declSurfaceTypeIdentifier );

	sdRequirementCheck::InitFactory();
	sdDeclItemPackage::InitConsumables();

	common->PrintLoadingMessage( versionString + " SCANNING DECLARATION FOLDERS" );

	// register game specific decl folders
	declManager->RegisterDeclFolder( "def",				".def" );
	declManager->RegisterDeclFolder( "af",				".af" );
	declManager->RegisterDeclFolder( "vehicles",		".vscript" );
	declManager->RegisterDeclFolder( "quickchat",		".qc" );
	declManager->RegisterDeclFolder( "mapinfo",			".txt" );
	declManager->RegisterDeclFolder( "mapinfo",			".md" );
	declManager->RegisterDeclFolder( "effects",			".effect" );
	declManager->RegisterDeclFolder( "guis",			".gui" );
	declManager->RegisterDeclFolder( "guis",			".guitheme" );
	declManager->RegisterDeclFolder( "bindings",		".binding" );
	declManager->RegisterDeclFolder( "decals",			".decal" );
	declManager->RegisterDeclFolder( "menus",			".radialmenu" );

	mapMetaDataList = fileSystem->ListAddonMetaData( "mapMetaData" );
	campaignMetaDataList = fileSystem->ListAddonMetaData( "campaignMetaData" );

	declManager->FinishedRegistering();

	cmdSystem->AddCommand( "listModelDefs",			idListGameDecls_f< DECLTYPE_MODEL >,			CMD_FL_SYSTEM | CMD_FL_GAME, "lists model defs" );
	cmdSystem->AddCommand( "listVehicleScripts",	idListGameDecls_f< DECLTYPE_VEHICLESCRIPT >,	CMD_FL_SYSTEM | CMD_FL_GAME, "lists vehicle scripts" );
	cmdSystem->AddCommand( "listGUIThemes",			idListGameDecls_f< DECLTYPE_GUITHEME >,			CMD_FL_SYSTEM | CMD_FL_GAME, "lists GUI themes" );
	cmdSystem->AddCommand( "listClientEntities",	idGameLocal::ListClientEntities_f,				CMD_FL_SYSTEM | CMD_FL_GAME, "lists Client Entities" );

	cmdSystem->AddCommand( "printModelDefs",		idPrintGameDecls_f< DECLTYPE_MODEL >,			CMD_FL_SYSTEM | CMD_FL_GAME, "prints a model def",		idArgCompletionGameDecl_f< DECLTYPE_MODEL > );
	cmdSystem->AddCommand( "printVehicleScripts",	idPrintGameDecls_f< DECLTYPE_VEHICLESCRIPT >,	CMD_FL_SYSTEM | CMD_FL_GAME, "prints a vehicle script",	idArgCompletionGameDecl_f< DECLTYPE_VEHICLESCRIPT > );

	cmdSystem->AddCommand( "testGUI",				idGameLocal::TestGUI_f,							CMD_FL_SYSTEM | CMD_FL_GAME, "Replace the main menu with a test gui.", idArgCompletionGameDecl_f< DECLTYPE_GUI > );

	sdDeclGUI::InitDefines();

	globalProperties.Init();
	limboProperties.Init();
	localPlayerProperties.Init();

	sdInventory::BuildSlotBankLookup();

	Clear();

	LoadLifeStatsData();

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		proficiencyTables[ i ].Init( i );
		proficiencyTables[ i ].Clear( true );
	}

	common->PrintLoadingMessage( versionString + " INIT CLASSES AND EVENTS" );

	for ( int i = 0; i < MAX_BATTLESENSE_RANKS; i++ ) {
		battleSenseBonus[ i ]	= gameLocal.declProficiencyItemType[ va( "pro_battlesense_rank%i", i + 1 ) ];
	}

	clip.AllocThread();

	idEvent::Init();
	idClass::InitClasses();

	InitConsoleCommands();

#ifdef _XENON
	liveManager->Initialize();
#else
	sdnet.Init();
	updateManager.Init();
#endif

	reservedClientSlots.Clear();

	sdDemoManager::GetInstance().Init();

	common->PrintLoadingMessage( versionString + " INIT UI" );

	uiManager->Init();

	common->PrintLoadingMessage( versionString + " GAMEPLAY SYSTEMS" );

	sdRequirementManager::GetInstance().Init();
	sdProficiencyManager::GetInstance().Init();
	sdUserGroupManager::GetInstance().Init();
	sdAdminSystem::GetInstance().Init();
	sdVoteManager::GetInstance().Init();

	common->PrintLoadingMessage( versionString + " INIT TEAMS" );
	sdTeamManager::GetInstance().Init();
	sdFireTeamManager::GetInstance().Init();
	gameLocal.aorManager.Init();

	sdCommandMapInfoManager::GetInstance().Init();

	sdVehicleSuspension::Startup();
	sdVehicleView::Startup();
	sdScriptedEntityHelper::Startup();
	sdVehicleSoundControlBase::Startup();

	mapLoadCount = 0;
	playerSpawnTime = 0;

	OnInputInit();

	gamestate = GAMESTATE_NOMAP;

	currentPartialLoadTeamAssets = NULL;

	if ( !networkSystem->IsDedicated() ) {
		common->PrintLoadingMessage( versionString + " LOAD UI" );

		lagoMaterial = declHolder.declMaterialType.LocalFind( LAGO_MATERIAL, false );

		uiScopes.Clear();
		uiScopes.Append( guiScope_t( "player", &localPlayerProperties ) );
		uiScopes.Append( guiScope_t( "globals", &globalProperties ) );
		uiScopes.Append( guiScope_t( "limbo", &limboProperties ) );
		uiScopes.Append( guiScope_t( "demo", &sdDemoManager::GetInstance().GetProperties() ) );
		uiScopes.Append( guiScope_t( "admin", &sdAdminSystem::GetInstance().GetProperties() ) );
#ifndef _XENON
		uiScopes.Append( guiScope_t( "sdnet", &sdnet.GetProperties() ) );
		uiScopes.Append( guiScope_t( "updates", &updateManager ) );
#endif
		sdDemoManager::GetInstance().InitGUIs();

		uiManager->RegisterListEnumerationCallback( "demoList",				CreateDemoList );
		uiManager->RegisterListEnumerationCallback( "lifeStatsList",		CreateLifeStatsList );
		uiManager->RegisterListEnumerationCallback( "predictedUpgradesList",CreatePredictedUpgradesList );
		uiManager->RegisterListEnumerationCallback( "reviewUpgradesList",	CreateUpgradesReviewList );
		uiManager->RegisterListEnumerationCallback( "modList",				CreateModList );
		uiManager->RegisterListEnumerationCallback( "crosshairs",			CreateCrosshairList );
		uiManager->RegisterListEnumerationCallback( "keyBindings",			CreateKeyBindingList );
		uiManager->RegisterListEnumerationCallback( "vehiclePlayerList",	CreateVehiclePlayerList );
		uiManager->RegisterListEnumerationCallback( "activeTaskList",		CreateActiveTaskList );
		uiManager->RegisterListEnumerationCallback( "fireTeamList",			CreateFireTeamList );
		uiManager->RegisterListEnumerationCallback( "inventoryList",		CreateInventoryList );
		uiManager->RegisterListEnumerationCallback( "scoreboardList",		CreateScoreboardList );	// input: string name of team to fill; return: average ping, average XP, number of active players
		uiManager->RegisterListEnumerationCallback( "playerAdminList",		CreatePlayerAdminList );
		uiManager->RegisterListEnumerationCallback( "userGroupList",		CreateUserGroupList );
		uiManager->RegisterListEnumerationCallback( "serverConfigList",		CreateServerConfigList );
		uiManager->RegisterListEnumerationCallback( "videoModeList",		CreateVideoModeList );
		uiManager->RegisterListEnumerationCallback( "campaignList",			CreateCampaignList );
		uiManager->RegisterListEnumerationCallback( "mapList",				CreateMapList );
		uiManager->RegisterListEnumerationCallback( "weaponSwitchList",		CreateWeaponSwitchList );
		uiManager->RegisterListEnumerationCallback( "callVoteList",			sdVoteManagerLocal::CreateCallVoteList );
		uiManager->RegisterListEnumerationCallback( "callVoteListOptions",	sdVoteManagerLocal::CreateCallVoteOptionList );
		uiManager->RegisterListEnumerationCallback( "colors",				CreateColorList );
		uiManager->RegisterListEnumerationCallback( "spawnLocations",		CreateSpawnLocationList );
		uiManager->RegisterListEnumerationCallback( "availableMSAA",		CreateMSAAList );
		uiManager->RegisterListEnumerationCallback( "soundOutput",			CreateSoundPlaybackList );
		uiManager->RegisterListEnumerationCallback( "soundInput",			CreateSoundCaptureList );


		sdDeclGUI::AddDefine( va( "MSG_OK %i",				MSG_OK ) );
		sdDeclGUI::AddDefine( va( "MSG_OKCANCEL %i",		MSG_OKCANCEL ) );
		sdDeclGUI::AddDefine( va( "MSG_YESNO %i",			MSG_YESNO ) );
		sdDeclGUI::AddDefine( va( "MSG_DOWNLOAD_YESNO %i",	MSG_DOWNLOAD_YESNO ) );
		sdDeclGUI::AddDefine( va( "MSG_NEED_PASSWORD %i",	MSG_NEED_PASSWORD ) );
		sdDeclGUI::AddDefine( va( "MSG_NEED_AUTH %i",		MSG_NEED_AUTH ) );
		sdDeclGUI::AddDefine( va( "MSG_ABORT %i",			MSG_ABORT ) );

		sdDeclGUI::AddDefine( va( "CHAT_MODE_MESSAGE %i",	sdGameRules::CHAT_MODE_MESSAGE ) );
		sdDeclGUI::AddDefine( va( "CHAT_MODE_OBITUARY %i",	sdGameRules::CHAT_MODE_OBITUARY ) );


		uiMainMenuHandle	= LoadUserInterface( "mainmenu",	false, true );
		uiSystemUIHandle	= LoadUserInterface( "system",		false, true );
		pureWaitHandle		= LoadUserInterface( "purewait",	false, true );		

		if( sdUserInterfaceLocal* systemUI = GetUserInterface( uiSystemUIHandle ) ) {
			systemUI->Activate();
		}

		LoadMainMenuPartialMedia( true );
	}

	g_commandMapZoomCvarCallback.Register();

	com_unlockFPS = cvarSystem->Find( "com_unlockFPS" );	

	// Ensure dynamic items are precached
	TouchMedia();

	sdProficiencyManagerLocal::ReadRankInfo( rankInfo );

	// cache rules
	for ( int i = 0; i < idClass::GetNumTypes(); i++ ) {
		idTypeInfo* type = idClass::GetType( i );
		if ( !sdGameRules::IsRuleType( *type ) ) {
			continue;
		}
		rulesCache.Set( type->classname, type->CreateInstance()->Cast< sdGameRules >() );
	}

	isAutorecording = false;
	hasTakenScoreShot = false;

	Printf( "game initialized.\n" );
	Printf( "--------------------------------------\n" );
}


/*
============
idGameLocal::TouchMedia
============
*/
void idGameLocal::TouchMedia() {
	// touch all crosshair defs
	int num = declStringMapType.Num();
	for( int i = 0; i < num; i++ ) {
		const sdDeclStringMap* decl = gameLocal.declStringMapType.LocalFindByIndex( i, false );
		if( idStr::Icmpn( decl->GetName(), "crosshairs", 10 ) != 0 ) {
			continue;
		}
		// ensure we're parsed
		gameLocal.declStringMapType.LocalFindByIndex( i, true );

		sdDeclGUI::CacheMaterialDictionary( "GameLocal::Init", decl->GetDict() );
	}

	// ensure levelshots are copied
	sdAddonMetaDataList* metaData = fileSystem->ListAddonMetaData( "mapMetaData" );
	for( int i = 0; i < metaData->GetNumMetaData(); i++ ) {
		const idDict& data = metaData->GetMetaData( i );
		if( !data.GetBool( "show_in_browser" ) ) {
			continue;
		}

		const char* imageName = data.GetString( "server_shot_thumb", "levelshots/thumbs/generic.tga" );
		sdFilePtr f( fileSystem->OpenFileRead( imageName, true ) );
		if( !f.IsValid() ) {
			Warning( "Could not touch '%s'", imageName );
		}
	}
	fileSystem->FreeAddonMetaDataList( metaData );
	metaData = NULL;

	for( int i = 0; i < gameLocal.campaignMetaDataList->GetNumMetaData(); i++ ) {
		const idDict& data = gameLocal.campaignMetaDataList->GetMetaData( i );
		if( !data.GetBool( "show_in_browser" ) ) {
			continue;
		}
		const idDict& dict = gameLocal.campaignMetaDataList->GetMetaData( i );
		const char* imageName = data.GetString( "server_shot_thumb", "levelshots/campaigns/thumbs/custom.tga" );
		sdFilePtr f( fileSystem->OpenFileRead( imageName, true ) );
		if( !f.IsValid() ) {
			Warning( "Could not touch '%s'", imageName );
		}
	}

//	ParseNetworkLog();
}

/*
================
idGameLocal::RegisterLoggedTrace
================
*/
sdLoggedTrace* idGameLocal::RegisterLoggedTrace( const trace_t& trace ) {
	for ( int i = 0; i < MAX_LOGGED_TRACES; i++ ) {
		if ( !loggedTraces[ i ] ) {
			loggedTraces[ i ] = new sdLoggedTrace();
			loggedTraces[ i ]->Init( i, trace );
			return loggedTraces[ i ];
		}
	}

	gameLocal.Warning( "idGameLocal::RegisterLoggedTrace No Free Traces" );
	return NULL;
}

/*
================
idGameLocal::FreeLoggedTrace
================
*/
void idGameLocal::FreeLoggedTrace( sdLoggedTrace* trace ) {
	if ( !trace ) {
		gameLocal.Warning( "idGameLocal::FreeLoggedTrace NULL trace" );
		assert( false );
		return;
	}

	int index = trace->GetIndex();
	delete trace;
	loggedTraces[ index ] = NULL;
}

/*
================
idGameLocal::RegisterLoggedDecal
================
*/
int idGameLocal::RegisterLoggedDecal( const idMaterial* material ) {
	if ( networkSystem->IsDedicated() ) {
		return -1;
	}

	int i;
	for ( i = 0; i < MAX_LOGGED_DECALS; i++ ) {
		if ( !loggedDecals[ i ] ) {
			gameDecalInfo_t* info = new gameDecalInfo_t;
			loggedDecals[ i ] = info;

			// init
			memset( &info->renderEntity, 0, sizeof( info->renderEntity ));
			info->renderEntity.hModel		= gameRenderWorld->CreateDecalModel();
			info->renderEntity.customShader = material;
			info->renderEntity.spawnID	= -1;
			info->renderEntity.axis.Identity();
			info->renderEntity.origin.Zero();
			info->renderEntity.shaderParms[ SHADERPARM_RED ]	= 1.0f;
			info->renderEntity.shaderParms[ SHADERPARM_GREEN ]	= 1.0f;
			info->renderEntity.shaderParms[ SHADERPARM_BLUE ]	= 1.0f;
			info->renderEntity.shaderParms[ SHADERPARM_ALPHA ]	= 1.0f;

			info->renderEntityHandle = gameRenderWorld->AddEntityDef( &info->renderEntity );

			return i;
		}
	}
	return -1;
}

/*
================
idGameLocal::GetLoggedDecal
================
*/
gameDecalInfo_t* idGameLocal::GetLoggedDecal( int index ) {
	if ( index < 0 || index >= MAX_LOGGED_DECALS ) {
		gameLocal.Warning( "idGameLocal::GetLoggedDecal : index '%d' out of bounds", index );
		assert( false );
		return NULL;
	}

	return loggedDecals[ index ];
}

/*
================
idGameLocal::ResetLoggedDecal
================
*/
void idGameLocal::ResetLoggedDecal( int index ) {
	if ( index < 0 || index >= MAX_LOGGED_DECALS ) {
		gameLocal.Warning( "idGameLocal::ResetLoggedDecal : index '%d' out of bounds", index );
		assert( false );
		return;
	}

	gameDecalInfo_t* info = loggedDecals[ index ];
	if( !info->renderEntity.hModel || !info->renderEntity.hModel->Surface( 0 )) {
		assert( 0 );
		return;
	}

	gameRenderWorld->ResetDecalModel( info->renderEntity.hModel );
}

/*
================
idGameLocal::FreeLoggedDecal
================
*/
void idGameLocal::FreeLoggedDecal( int index ) {
	if ( index < 0 || index >= MAX_LOGGED_DECALS ) {
		gameLocal.Warning( "idGameLocal::FreeLoggedDecal : index '%d' out of bounds", index );
		return;
	}

	gameDecalInfo_t* info = loggedDecals[ index ];
	assert( info );
	if( !info ) {
		return;
	}

	if( info->renderEntity.hModel != NULL ) {
		renderModelManager->FreeModel( info->renderEntity.hModel );
	}

	if ( gameRenderWorld ) {
		gameRenderWorld->FreeEntityDef( info->renderEntityHandle );
	}

	delete info;
	loggedDecals[ index ] = NULL;
}

/*
===========
idGameLocal::ClientsOnSameTeam
============
*/
bool idGameLocal::ClientsOnSameTeam( int clientNum1, int clientNum2, voiceMode_t voiceMode ) {
	idPlayer* player1 = GetClient( clientNum1 );
	idPlayer* player2 = GetClient( clientNum2 );

	if ( player1 == NULL || player2 == NULL ) {
		return false;
	}

	// Gordon: Client 1 is the one talking, so only check that mask
	if ( clientMuteMask[ clientNum1 ].Get( clientNum2 ) != 0 ) {
		return false;
	}

	switch ( voiceMode ) {
		case VO_TEAM:
			if ( !player2->userInfo.voipReceiveTeam ) {
				return false;
			}
			return player1->IsTeam( player2->GetTeam() );
		case VO_FIRETEAM: {
			if ( !player2->userInfo.voipReceiveFireTeam ) {
				return false;
			}

			if ( !player1->IsTeam( player2->GetTeam() ) ) {
				return false;
			}

			sdFireTeam* f1 = gameLocal.rules->GetPlayerFireTeam( clientNum1 );
			sdFireTeam* f2 = gameLocal.rules->GetPlayerFireTeam( clientNum2 );
			if ( f1 != NULL || f2 != NULL ) {
				return f1 == f2;
			}

			taskHandle_t t1 = player1->GetActiveTaskHandle();
			taskHandle_t t2 = player2->GetActiveTaskHandle();
			if ( t1.IsValid() || t2.IsValid() ) {
				return t1 == t2;
			}

			return false;
		}
		case VO_GLOBAL: {
			if ( !player2->userInfo.voipReceiveGlobal ) {
				return false;
			}
			return true;
		}
	}

	return false;
}

/*
===========
idGameLocal::AllowClientAudio
============
*/
bool idGameLocal::AllowClientAudio( int clientNum, voiceMode_t voiceTeam ) {
	idPlayer* player = GetClient( clientNum );
	if ( !player ) {
		return false;
	}

	if ( gameLocal.rules->MuteStatus( player ) & MF_AUDIO ) {
		const sdUserGroup& userGroup = sdUserGroupManager::GetInstance().GetGroup( player->GetUserGroup() );
		if ( !userGroup.HasPermission( PF_NO_MUTE_VOIP ) ) {
			return false;
		}
	}

	if ( voiceTeam == VO_GLOBAL ) {
		if ( g_disableGlobalAudio.GetBool() ) {
			const sdUserGroup& userGroup = sdUserGroupManager::GetInstance().GetGroup( player->GetUserGroup() );
			if ( !userGroup.HasPermission( PF_NO_MUTE_VOIP ) ) {
				return false;
			}
		}
	}

	return true;
}


/*
===========
idGameLocal::SetRules
============
*/
void idGameLocal::SetRules( idTypeInfo* type ) {
	delete rules;
	rules = type->CreateInstance()->Cast< sdGameRules >();
	if ( rules == NULL ) {
		Error( "idGameLocal::SetRules '%s' is not a valid rule type", type->classname );
	}

	if ( program && program->IsValid() ) {
		rules->OnNewScriptLoad();
	}

	if ( isServer ) {
		sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_RULES_DATA );
		msg.WriteLong( sdGameRules::EVENT_CREATE );
		msg.WriteLong( type->typeNum );
		msg.Send( sdReliableMessageClientInfoAll() );
	}

	ResetGameState( ENSM_RULES );

	// multiplayer commands
	rules->InitConsoleCommands();

	if( !networkSystem->IsDedicated() ) {
		UpdateCampaignStats( false );
	}
}

/*
===========
idGameLocal::KeyMove
============
*/
bool idGameLocal::KeyMove( char forward, char right, char up, usercmd_t& cmd ) {
	idPlayer* player = GetLocalPlayer();
	if ( !player ) {
		return false;
	}

	return player->KeyMove( forward, right, up, cmd );
}

/*
===========
idGameLocal::ControllerMove
============
*/
void idGameLocal::ControllerMove( bool doGameCallback, const int numControllers, const int* controllerNumbers, const float** controllerAxis, idVec3& viewAngles, usercmd_t& cmd ) {
	idPlayer* player = GetLocalPlayer();
	if ( !player ) {
		return;
	}

	sdHudModule* module = gameLocal.localPlayerProperties.GetActiveHudModule();
	if ( module != NULL && module->InhibitControllerMovement() ) {
		return;
	}

	player->ControllerMove( doGameCallback, numControllers, controllerNumbers, controllerAxis, viewAngles, cmd );
}

/*
===========
idGameLocal::MouseMove
============
*/
void idGameLocal::MouseMove( const idVec3& vecAngleBase, idVec3& vecAngleDelta ) {
	if ( IsPaused() ) {
		return;
	}

	idPlayer* player = GetLocalPlayer();
	if ( !player ) {
		return;
	}

	idAngles angleBase( vecAngleBase[ PITCH ], vecAngleBase[ YAW ], 0.0f );
	idAngles angleDelta( vecAngleDelta[ PITCH ], vecAngleDelta[ YAW ], 0.0f );
	player->MouseMove( angleBase, angleDelta );
	vecAngleDelta[ PITCH ] = angleDelta.pitch;
	vecAngleDelta[ YAW ] = angleDelta.yaw;
}

/*
===========
idGameLocal::GetSensitivity
============
*/
bool idGameLocal::GetSensitivity( float& scaleX, float& scaleY ) {
	if ( IsPaused() ) {
		return false;
	}

	idPlayer* player = GetLocalPlayer();
	if ( !player ) {
		return false;
	}

	return player->GetSensitivity( scaleX, scaleY );
}

/*
===========
idGameLocal::Shutdown

  shut down the entire game
============
*/
void idGameLocal::Shutdown( void ) {

	if ( common == NULL ) {
		return;
	}

	Printf( "------------ Game Shutdown -----------\n" );

	rulesCache.DeleteValues();
	rulesCache.Clear();

	if ( damageLogFile != NULL ) {
		fileSystem->CloseFile( damageLogFile );
		damageLogFile = NULL;
	}

	if ( debugLogFile != NULL ) {
		fileSystem->CloseFile( debugLogFile );
		debugLogFile = NULL;
	}

	if ( proficiencyLog != NULL ) {
		fileSystem->CloseFile( proficiencyLog );
		proficiencyLog = NULL;
	}

	if ( networkLog != NULL ) {
		fileSystem->CloseFile( networkLog );
		networkLog = NULL;
	}

	if ( objectiveLog != NULL ) {
		fileSystem->CloseFile( objectiveLog );
		objectiveLog = NULL;
	}

	if ( rules ) {
		rules->Shutdown();
	}

	MapShutdown();

	idWeapon::UnRegisterCVarCallback();
	sdDeclAOR::ShutdownCVars();

	sdRequirementCheck::ShutdownFactory();
	sdDeclItemPackage::ShutdownConsumables();
	sdDeclGUI::ClearDefines();

	idDeclTypeInterface* declInterface = declManager->GetDeclType( declSurfaceTypeIdentifier );
	declInterface->UnregisterPostParse( idGameLocal::SurfaceTypePostParse );

	fileSystem->FreeAddonMetaDataList( mapMetaDataList );
	mapMetaDataList = NULL;
	
	fileSystem->FreeAddonMetaDataList( campaignMetaDataList );
	campaignMetaDataList = NULL;

	// unregister game specific decl types
	declManager->UnregisterDeclType( &declModelDefType );
	declManager->UnregisterDeclType( &declExportDefType );
	declManager->UnregisterDeclType( &declVehicleScriptDefType );
	declManager->UnregisterDeclType( &declAmmoTypeType );
	declManager->UnregisterDeclType( &declInvSlotType );
	declManager->UnregisterDeclType( &declInvItemTypeType );
	declManager->UnregisterDeclType( &declInvItemType );
	declManager->UnregisterDeclType( &declItemPackageType );
	declManager->UnregisterDeclType( &declStringMapType );
	declManager->UnregisterDeclType( &declDamageType );
	declManager->UnregisterDeclType( &declDamageFilterType );
	declManager->UnregisterDeclType( &declCampaignType );
	declManager->UnregisterDeclType( &declQuickChatType );
	declManager->UnregisterDeclType( &declMapInfoType );
	declManager->UnregisterDeclType( &declToolTipType );
	declManager->UnregisterDeclType( &declTargetInfoType );
	declManager->UnregisterDeclType( &declProficiencyTypeType );
	declManager->UnregisterDeclType( &declProficiencyItemType );
	declManager->UnregisterDeclType( &declRankType );
	declManager->UnregisterDeclType( &declDeployableObjectType );
	declManager->UnregisterDeclType( &declDeployableZoneType );
	declManager->UnregisterDeclType( &declPlayerClassType );
	declManager->UnregisterDeclType( &declGUIType );
	declManager->UnregisterDeclType( &declTeamInfoType );
	declManager->UnregisterDeclType( &declPlayerTaskType );
	declManager->UnregisterDeclType( &declRequirementType );
	declManager->UnregisterDeclType( &declGUIThemeType );
	declManager->UnregisterDeclType( &declVehiclePathType );
	declManager->UnregisterDeclType( &declKeyBindingType );
	declManager->UnregisterDeclType( &declRadialMenuType );
	declManager->UnregisterDeclType( &declAORType );
	declManager->UnregisterDeclType( &declRatingType );
	declManager->UnregisterDeclType( &declHeightMapType );
	declManager->UnregisterDeclType( &declDeployMaskType );

	// register game specific decl folders
	declManager->UnregisterDeclFolder( "def",				".def" );
	declManager->UnregisterDeclFolder( "af",				".af" );
	declManager->UnregisterDeclFolder( "atmosphere",		".climate" );
	declManager->UnregisterDeclFolder( "atmosphere",		".forecast" );
	declManager->UnregisterDeclFolder( "vehicles",			".vscript" );
	declManager->UnregisterDeclFolder( "quickchat",			".qc" );
	declManager->UnregisterDeclFolder( "mapinfo",			".txt" );
	declManager->UnregisterDeclFolder( "mapinfo",			".md" );
	declManager->UnregisterDeclFolder( "effects",			".effect" );
	declManager->UnregisterDeclFolder( "guis",				".gui" );
	declManager->UnregisterDeclFolder( "guis",				".guitheme" );
	declManager->UnregisterDeclFolder( "bindings",			".binding" );
	declManager->UnregisterDeclFolder( "decals",			".decal" );
	declManager->UnregisterDeclFolder( "menus",				".radialmenu" );

#if defined( ID_ALLOW_TOOLS )
	// shutdown the model exporter
	idModelExport::Shutdown();
#endif /* ID_ALLOW_TOOLS */

	sdRequirementManager::DestroyInstance();
	sdProficiencyManager::DestroyInstance();
	sdTeamManager::DestroyInstance();

	int i;
	for ( i = 0; i < loggedTraces.Num(); i++ ) {
		if ( loggedTraces[ i ] ) {
			FreeLoggedTrace( loggedTraces[ i ] );
		}
	}

	for ( i = 0; i < loggedDecals.Num(); i++ ) {
		if ( loggedDecals[ i ] ) {
			FreeLoggedDecal( i );
		}
	}

	delete program;
	program = NULL;

	ClearPlayZones();

	entityCollections.DeleteContents( true );
	entityCollectionsHash.Clear();

	sdAnimFrameCommand::Shutdown();

	sdGlobalStatsTracker::DestroyInstance();

	idEvent::Shutdown();

	idClass::ShutdownClasses();

	// clear list with forces
	idForce::ClearForceList();

	// delete the .map file
	delete mapFile;
	mapFile = NULL;

//mal: delete the bot entities
	delete botMapFile;
	botMapFile = NULL;

	ShutdownConsoleCommands();

	clip.FreeThread();

	// free memory allocated by class objects
	Clear();

	// shut down the animation manager
	animationLib.Shutdown();

	playZoneAreas.DeleteContents( true );

	delete rules;
	rules = NULL;

	uiManager->Clear( true );
	uiManager->Shutdown();

	sdCommandMapInfoManager::GetInstance().Shutdown();
	sdCommandMapInfoManager::DestroyInstance();

	sdVehicleSuspension::Shutdown();
	sdVehicleView::Shutdown();
	sdScriptedEntityHelper::Shutdown();
	sdVehicleSoundControlBase::Shutdown();

#ifdef _XENON
	liveManager->Shutdown();
#else
	sdnet.Shutdown();
	updateManager.Shutdown();
#endif

	localPlayerProperties.Shutdown();
	globalProperties.Shutdown();
	limboProperties.Shutdown();

	sdDemoManager::GetInstance().Shutdown();
	sdDemoManager::DestroyInstance();

	Printf( "--------------------------------------\n" );

	// remove auto-completion function pointers pointing into this DLL
	cvarSystem->RemoveFlaggedAutoCompletion( CVAR_GAME );

	// enable leak test
	Mem_EnableLeakTest( "game" );

	// shutdown idLib
#if !defined( MONOLITHIC )
	idLib::ShutDown();
#endif
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

	if ( !IsDeveloper() ) {
		return;
	}

	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	common->DPrintf( "%s", text );
}

/*
============
idGameLocal::Warning
============
*/
void idGameLocal::Warning( const char *fmt, ... ) const {
	va_list		argptr;
	char		text[MAX_STRING_CHARS];

	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	sdProgramThread* thread = NULL;
	if ( gameLocal.program != NULL ) {
		thread = gameLocal.program->GetCurrentThread();
	}

	if ( thread != NULL ) {
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

	if ( !IsDeveloper() ) {
		return;
	}

	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	sdProgramThread* thread = NULL;
	if ( gameLocal.program != NULL ) {
		thread = gameLocal.program->GetCurrentThread();
	}

	if ( thread != NULL ) {
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

	va_start( argptr, fmt );
	idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
	va_end( argptr );

	assert( false );

	if ( program != NULL ) {
		if ( program->OnError( text ) ) {
			return;
		}
	}

	common->Error( "%s", text );
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
idGameLocal::ToggleNoclipMode
============
*/
void idGameLocal::ToggleNoclipMode( idPlayer* player ) const {
	if ( !player ) {
		return;
	}

	if ( !gameLocal.CheatsOk( false ) ) {
		return;
	}

	const wchar_t* msg;
	if ( player->GetNoClip() ) {
		player->SetNoClip( false );
		msg = L"noclip OFF";
	} else {
		player->SetNoClip( true );
		msg = L"noclip ON";
	}

	player->SendUnLocalisedMessage( msg );
}

/*
===========
idGameLocal::ToggleGodMode
============
*/
void idGameLocal::ToggleGodMode( idPlayer* player ) const {
	if ( !player ) {
		return;
	}

	if ( !gameLocal.CheatsOk( false ) ) {
		return;
	}

	const wchar_t* msg;
	if ( player->GetGodMode() ) {
		player->SetGodMode( false );
		msg = L"godmode OFF";
	} else {
		player->SetGodMode( true );
		msg = L"godmode ON";
	}

	player->SendUnLocalisedMessage( msg );
}

/*
===========
idGameLocal::NetworkSpawn
============
*/
void idGameLocal::NetworkSpawn( const char* classname, idPlayer* player ) {
	if ( !player ) {
		return;
	}

	if ( !gameLocal.CheatsOk( false ) ) {
		return;
	}

	float		yaw;
	idVec3		org;
	idDict		dict;

	yaw = player->viewAngles.yaw;

	dict.Set( "classname", classname );
	dict.SetFloat( "angle", yaw + 180 );

	org = player->GetPhysics()->GetOrigin() + idAngles( 0, yaw, 0 ).ToForward() * 80 + idVec3( 0, 0, 1 );
	dict.SetVector( "origin", org );

	gameLocal.SpawnEntityDef( dict, true );
}

/*
===========
idGameLocal::UserInfoChanged
============
*/
void idGameLocal::UserInfoChanged( int clientNum ) {
	idPlayer* player = GetClient( clientNum );
	if ( player != NULL ) {
		player->UserInfoChanged();
		rules->EnterGame( player );
	}
}

/*
===========
idGameLocal::CleanName
============
*/
void idGameLocal::CleanName( idStr& name ) {
	name.RemoveColors();
	name.StripTrailingWhiteSpace();
}

#define USERINFO_GET_NEW_NAME									\
				baseName = _userInfo.GetString( "ui_name" );	\
				name = baseName;								\
				CleanName( name );


/*
===========
idGameLocal::ValidateUserInfo
============
*/
bool idGameLocal::ValidateUserInfo( int clientNum, idDict& _userInfo ) {
	bool modifiedInfo = false;

	idStr baseName;
	idStr name;

	USERINFO_GET_NEW_NAME

	// don't let numeric nicknames, it can be exploited to go around kick and ban commands from the server
	if ( baseName.IsNumeric() ) {
		_userInfo.Set( "ui_name", va( "%s_", baseName.c_str() ) );
		modifiedInfo = true;

		USERINFO_GET_NEW_NAME
	}

	sdNetClientId clientId;
	networkSystem->ServerGetClientNetId( clientNum, clientId );

	bool demonwareAccountName = false;
	if ( clientId.IsValid() ) {
		demonwareAccountName = true;
	}

	// don't allow dupe nicknames - unless its a bot, in which case he'll change HIS name to defer to the human.
	for ( int i = 0; i < numClients; i++ ) {
		if ( i == clientNum ) {
			continue;
		}

		idPlayer* player = GetClient( i );
		if ( player ) {
			idStr cleanBaseName = player->userInfo.baseName;
			idGameLocal::CleanName( cleanBaseName );
			if ( !idStr::Icmp( name, cleanBaseName ) ) {
				if ( player->IsType( idBot::Type ) ) {
					botThreadData.ChangeBotName( player );
					i = -1; //rescan. There shouldn't be other bots with this name, but COULD be another human player.
				} else if ( !demonwareAccountName ) {
					_userInfo.Set( "ui_name", va( "%s_", baseName.c_str() ) );
					modifiedInfo = true;

					USERINFO_GET_NEW_NAME

					i = -1;	// rescan
					continue;
				}
			}
		}
	}

	// allow users to see bots
	idPlayer* p = GetClient( clientNum );
	if ( p != NULL ) {
		bool isBot = p->IsType( idBot::Type );
		if ( _userInfo.GetBool( "ui_bot" ) != isBot ) {
			_userInfo.SetBool( "ui_bot", isBot );
			modifiedInfo = true;
		}
	}

	return modifiedInfo;
}

/*
===========
idGameLocal::SetServerInfo
============
*/
void idGameLocal::SetServerInfo( const idDict& _serverInfo ) {
	serverInfo = _serverInfo;
	UpdateServerInfoFlags();
	ParseServerInfo();
#ifdef SD_SUPPORT_REPEATER
	UpdateRepeaterInfo();
#endif // SD_SUPPORT_REPEATER

	sdnet.UpdateGameSession( true, false );
}

/*
===========
idGameLocal::ParseServerInfo
============
*/
void idGameLocal::ParseServerInfo( void ) {
	serverInfoData.timeLimit			= Max( 0, MINS2MS( serverInfo.GetFloat( "si_timeLimit" ) ) );	
	serverInfoData.adminStart			= serverInfo.GetBool( "si_adminStart" );
	serverInfoData.noProficiency		= serverInfo.GetBool( "si_noProficiency" );
	serverInfoData.votingDisabled		= serverInfo.GetBool( "si_disableVoting" );
	serverInfoData.minPlayers			= serverInfo.GetInt( "si_minPlayers" );
	serverInfoData.readyPercent			= serverInfo.GetInt( "si_readyPercent" );
	serverInfoData.gameReviewReadyWait	= serverInfo.GetBool( "si_gameReviewReadyWait" );

	localPlayerProperties.OnServerInfoChanged();

	UpdateCampaignStats( false );
}

/*
===================
idGameLocal::PushChangedEntity
===================
*/
void idGameLocal::PushChangedEntity( idEntity* ent ) {
	changedEntities.Alloc() = ent;
}

/*
===================
idGameLocal::LoadMap

Initializes all map variables common to both save games and spawned games.
===================
*/
void idGameLocal::LoadMap( const char* mapName, int randSeed, int startTime ) {

	doingMapRestart = false;

	if ( !reloadingSameMap ) {
		useSimpleEffect = bse_simple.GetBool();
	}

	// regrab map/campaign meta data
	fileSystem->FreeAddonMetaDataList( mapMetaDataList );
	fileSystem->FreeAddonMetaDataList( campaignMetaDataList );

	mapMetaDataList = fileSystem->ListAddonMetaData( "mapMetaData" );
	campaignMetaDataList = fileSystem->ListAddonMetaData( "campaignMetaData" );

	SetupMapMetaData( mapName );

	gameLocal.declEntityDefType[ "player_edf" ];
	gameLocal.declEntityDefType[ "bot_edf" ];
	
	if ( !reloadingSameMap ) {
		OnNewMapLoad( mapName );
	} else {
		if ( program == NULL || !program->IsValid() ) {
			LoadScript();
		}
	}

	const char* zoneName = mapMetaData->GetString( "massive_zone_name" );
	if ( !*zoneName ) {
		zoneName = mapName;
	}
	adManager->SetAdZone( zoneName );

	// Gordon: not precaching this entry automatically on purpose, as it'll get hit outside of level load otherwise
	mapSkinPool = gameLocal.declStringMapType[ mapMetaData->GetString( "climate_skins", "climate_skins_temperate" ) ];
	if ( mapSkinPool != NULL ) {
		gameLocal.CacheDictionaryMedia( mapSkinPool->GetDict() );
	}

	// clear the sound system
	// jrad - don't do this, since we need to play during level loads
	//gameSoundWorld->ClearAllSoundEmitters();

	localPlayerProperties.CloseActiveHudModules();

	if ( !reloadingSameMap || mapFile == NULL || mapFile->NeedsReload() ) {
		delete mapFile;
		mapFile = new idMapFile;
		if ( !mapFile->Parse( ( idStr( mapName ) + "." ) + ENTITY_FILE_EXT ) ) {
			delete mapFile;
			mapFile = NULL;
			Error( "Couldn't load %s", mapName );
		}
	}

	if ( !reloadingSameMap || botMapFile == NULL || botMapFile->NeedsReload() ) {
        delete botMapFile;
		botMapFile = new idMapFile;
		if ( !botMapFile->ParseBotEntities( ( idStr( mapName ) + "." ) + BOT_ENTITY_FILE_EXT ) ) {
			delete botMapFile;
			botMapFile = NULL;
			Warning( "Couldn't load bot entities for  %s", mapName );
		}
	}

	mapFileName = mapName;

	if ( mapFile != NULL && gameLocal.isServer ) {
		gameLocal.Printf( "----------- Loading Map AAS ------------\n" );
		botThreadData.LoadMap( mapName, randSeed );
		botThreadData.InitAAS( mapFile ); //mal: load the AAS for this map, if the mapfile was loaded. 
	}

	// load the collision map
	collisionModelManager->LoadMap( mapName, false );

	numClients = 0;

	// initialize all entities for this game
	entities.Memset( 0 );
	memset( usercmds, 0, sizeof( usercmds ) );
	memset( spawnIds, -1, sizeof( spawnIds ) );
	spawnCount = INITIAL_SPAWN_COUNT;

	spawnedEntities.Clear();
	activeEntities.Clear();
	activeNetworkEntities.Clear();
	changedEntities.Clear();
	sortTeamMasters = false;
	sortPushers = false;

	clientSpawnedEntities.Clear ();
	for (int i=0; i<MAX_CENTITIES; i++) {
		clientSpawnIds[i] = -INITIAL_SPAWN_COUNT;
	}
	memset ( clientEntities, 0, sizeof(clientEntities) );
	firstFreeClientIndex = 0;

	globalMaterial = NULL;

	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	numEntities	= MAX_CLIENTS;
	firstFreeIndex	= MAX_CLIENTS;

	// reset the random number generator.
	random.SetSeed( randSeed );

	camera			= NULL;
	world			= NULL;
	testmodel		= NULL;
	//testFx			= NULL;

	this->startTime	= startTime;
	previousTime	= startTime;
	time			= startTime;
	timeOffset		= 0;
	nextBotPopulationCheck = startTime;

	framenum		= 0;

	if ( !editEntities ) {
		editEntities = new idEditEntities;
	}

	gravity.Set( 0, 0, -g_gravity.GetFloat() );

	spawnArgs.Clear();

	clip.Init();
	pvs.Init();
	aorManager.OnMapLoad();
	sdAntiLagManager::GetInstance().OnMapLoad();
	sdCommandMapInfoManager::GetInstance().Clear();

	if ( !reloadingSameMap ) {
		mapFile->RemovePrimitiveData(); 	// FIXME: Arnout/Jared: deprecated - we should remove this
	}

	CloseMessageBox();	// jrad - close any error/connecting message boxes in the main menu now that we're in

	if( sdUserInterfaceScope* scope = gameLocal.globalProperties.GetSubScope( "game" ) ) {
		if( sdProperties::sdProperty* property = scope->GetProperty( "isRunning", sdProperties::PT_FLOAT )) {
			*property->value.floatValue = GameState() > GAMESTATE_NOMAP ? 1.0f : 0.0f;
		}
	}
}

/*
===================
idGameLocal::OnLocalMapRestart
===================
*/
void idGameLocal::OnLocalMapRestart( void ) {
	rules->OnLocalMapRestart();
	sdObjectiveManager::GetInstance().OnLocalMapRestart();

	hasTakenScoreShot = false;
}

/*
===================
idGameLocal::LocalMapRestart
===================
*/
void idGameLocal::LocalMapRestart( void ) {
	doingMapRestart = true;

	renderSystem->LockThreads();

	if ( gameLocal.isServer ) {
		sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_MAP_RESTART );
		msg.Send( sdReliableMessageClientInfoAll() );
	}
	OnLocalMapRestart();

	botThread->StopThread();

	gamestate = GAMESTATE_SHUTDOWN;

	idStr currentSpawnPoints[ MAX_CLIENTS ];
	for ( int i = 0; i < numClients; i++ ) {
		idPlayer* player = GetClient( i );
		if ( player == NULL ) {
			continue;
		}

		idEntity* spawn = player->GetSpawnPoint();
		if ( spawn != NULL ) {
			currentSpawnPoints[ i ] = spawn->GetName();
		}
	}

	MapClear( false );

	gamestate = GAMESTATE_STARTUP;

	botThreadData.Clear( true );

	if ( mapFile != NULL && gameLocal.isServer ) {
		botThreadData.LoadMap( mapFile->GetName(), sys->Milliseconds() );
		botThreadData.InitAAS( mapFile ); //mal: load the AAS for this map, if the mapfile was loaded. 
	}

	OnPreMapStart();
	SpawnMapEntities();
	OnMapStart();
	
	if ( botMapFile != NULL ) {
 		botThreadData.LoadActions( botMapFile ); //mal: load any bot actions for this map, if the bot entities were loaded.

		if ( gameLocal.isServer ) {
			botThreadData.LoadRoutes( botMapFile ); //mal: load any bot routes for this map, if the bot entities were loaded.
			idStr navName = botMapFile->GetName();
			navName.SetFileExtension( "nav" );
			botThreadData.botVehicleNodes.LoadNodes( navName );
		}
	}

	// setup the client entities again
	for ( int i = 0; i < numClients; i++ ) {
		idPlayer* player = GetClient( i );
		if ( player == NULL ) {
			continue;
		}
		player->ClearDeathSound();
		player->Killed( NULL, NULL, 0, vec3_zero, -1, NULL );
		player->ServerForceRespawn( true );

		if ( currentSpawnPoints[ i ].Length() ) {
			idEntity* spawn = gameLocal.FindEntity( currentSpawnPoints[ i ] );
			if ( spawn != NULL ) {
				player->SetSpawnPoint( spawn );
			}
		}
	}

	gamestate = GAMESTATE_ACTIVE;

	botThreadData.Init();
	botThreadData.LoadBotNames();
	botThreadData.ResetClientsInfo();
	botThreadData.ResetBotsInfo();

	botThread->StartThread();
	renderSystem->UnlockThreads();

	doingMapRestart = false;
}

/*
===================
idGameLocal::MapRestart
===================
*/
void idGameLocal::MapRestart( void ) {
	if ( isClient ) {
		return;
	}

	LocalMapRestart();
	rules->MapRestart();
}

/*
===================
idGameLocal::MapRestart_f
===================
*/
void idGameLocal::MapRestart_f( const idCmdArgs &args ) {
	if ( !gameLocal.isServer ) {
		gameLocal.Printf( "server is not running - use spawnServer\n" );
		return;
	}

	gameLocal.MapRestart();
}

/*
===================
idGameLocal::NextMap_f
===================
*/
void idGameLocal::NextMap_f( const idCmdArgs &args ) {
	gameLocal.NextMap();
}

/*
===================
idGameLocal::StartDemos_f
===================
*/
void idGameLocal::StartDemos_f( const idCmdArgs &args ) {
	if ( gameLocal.isClient ) {
		return;
	}

	if ( gameLocal.isServer ) {
		sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_RECORD_DEMO );
		msg.Send( sdReliableMessageClientInfoAll() );
	}

	gameLocal.StartRecordingDemo();
}

/*
===================
idGameLocal::GetRulesType
===================
*/
idTypeInfo* idGameLocal::GetRulesType( bool errorOnFail ) {
	const char* ruleset;
	if ( networkSystem->IsRankedServer() ) {
		ruleset = "sdGameRulesCampaign";
	} else {
		ruleset = si_rules.GetString();
	}

	idTypeInfo* type = idClass::GetClass( ruleset );
	if ( type == NULL || !sdGameRules::IsRuleType( *type ) ) {
		if ( errorOnFail ) {
			gameLocal.Error( "idGameLocal::GetRulesType Invalid Rule Type '%s'", ruleset );
		}
		return NULL;
	}
	return type;
}

/*
===================
idGameLocal::MakeRules
===================
*/
void idGameLocal::MakeRules( void ) {
	if ( networkSystem->IsRankedServer() ) {
		si_rules.SetString( "sdGameRulesCampaign" );
	}

	idTypeInfo* type = GetRulesType( true );
	if ( rules != NULL ) {
		if ( rules->GetType() == type ) {
			return;
		}
	}

	bool playerStatesStored = false;
	sdGameRules::playerStateList_t playerStates;
	if ( gameLocal.rules != NULL ) {
		playerStatesStored = true;
		gameLocal.rules->SavePlayerStates( playerStates );
	}

	SetRules( type );

	if ( playerStatesStored ) {
		gameLocal.rules->RestorePlayerStates( playerStates );
	}
}

/*
===================
idGameLocal::OnPreMapStart
===================
*/
void idGameLocal::OnPreMapStart( void ) {
	localPlayerProperties.ShutdownGUIs();
	PurgeMainMenuPartialMedia();

	playzoneMask = gameLocal.GetDeploymentMask( "dm_playzone" );

	idDoor::OnNewMapLoad();
	idLight::OnNewMapLoad();
	idSound::OnNewMapLoad();
	sdLocationMarker::OnNewMapLoad();

	ClearDeployRequests();
	ClearPlayZones();
	ClearTargetTimers();
	envDefs.Clear();

	sdTaskManager::GetInstance().Init();
	sdObjectiveManager::GetInstance().Init();
	sdWayPointManager::GetInstance().Init();
	sdWakeManager::GetInstance().Init();
	tireTreadManager->Init();
	footPrintManager->Init();
	playerView.Init();

	sdDemoManager::GetInstance().StartDemo();	// FIXME: move to EndLevelLoad?

	spawnSpots.Clear();
	playZoneAreas.DeleteContents( true );

	int numAreas = gameRenderWorld->NumAreas();
	playZoneAreas.SetNum( numAreas );
	playZoneAreaNames.SetNum( numAreas );
	for ( int i = 0; i < numAreas; i++ ) {
		playZoneAreas[ i ] = NULL;
		playZoneAreaNames[ i ] = "";
	}
}

/*
===================
idGameLocal::OnNewMapLoad
===================
*/
void idGameLocal::OnNewMapLoad( const char* mapName ) {
	UpdateLevelLoadScreen( common->LocalizeText( "guis/mainmenu/loading/mapdata" ).c_str() );

	sdTeamManager::GetInstance().OnNewMapLoad();

	SetupMapMetaData( mapName );

	CacheDictionaryMedia( *mapMetaData );

	mapInfo = declMapInfoType.LocalFind( mapMetaData->GetString( "mapinfo", "default" ) );
	gameLocal.aorManager.Setup();

	// touch all the quickchat decls (they won't all get touched by the radial menu, since some are dynamic)
	for ( int i = 0; i < declQuickChatType.Num(); i++ ) {
		const idDecl* decl = declQuickChatType.FindByIndex( i );
	}

	UpdateLevelLoadScreen( common->LocalizeText(  "guis/mainmenu/loading/compiling_scripts" ).c_str() );
	LoadScript();
}


/*
============
idGameLocal::SetupMapMetaData
============
*/
void idGameLocal::SetupMapMetaData( const char* mapName ) {
	assert( mapMetaDataList != NULL );

	idStr strippedFileName = mapName;
	strippedFileName.StripFileExtension();
	mapMetaData = mapMetaDataList->FindMetaData( strippedFileName.c_str(), &defaultMetaData );
}

/*
===================
idGameLocal::OnMapStart
===================
*/
idCVar g_logDebugText( "g_logDebugText", "0", CVAR_BOOL | CVAR_GAME, "" );

void idGameLocal::OnMapStart( void ) {
	sdObjectiveManager::GetInstance().OnMapStart();
	sdTeamManager::GetInstance().OnMapStart();
	sdTaskManager::GetInstance().OnMapStart();

	sdLocationMarker::OnMapStart();
	LinkPlayZoneAreas();

	localPlayerProperties.InitGUIs();

	// free up any unused animations
	animationLib.FlushUnusedAnims();

	playerSpawnTime = time + 500; // Gordon: There is a slight delay to let entities settle down, have their teams set, etc

	// open and set up the debug logging file
	if ( !isClient && g_logDebugText.GetBool() ) {
		if ( debugLogFile != NULL ) {
			fileSystem->CloseFile( debugLogFile );
			debugLogFile = NULL;
		}
		idStr logFileName( "debugLog" );

		sysTime_t time;
		sys->RealTime( &time );

		logFileName += va( "_%d%02d%02d_%02d%02d%02d_", 1900 + time.tm_year, 1 + time.tm_mon, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec );
		idStr mapName = mapFileName;
		mapFileName.ExtractFileBase( mapName );
		logFileName += mapName;
		logFileName += ".txt";

		debugLogFile = fileSystem->OpenFileWrite( logFileName.c_str() );
		if ( debugLogFile != NULL ) {
			debugLogFile->Printf( "time, entityNumber, text\n" );
		}
	}
}

/*
===================
idGameLocal::InitFromNewMap
===================
*/
void idGameLocal::InitFromNewMap( const char* mapName, idRenderWorld *renderWorld, idSoundWorld *soundWorld, bool isServer, bool isClient, int randSeed, int startTime, bool isUserChange ) {
	mapLoadCount++;

	botThread->StopThread();

	this->isServer = isServer;
	this->isClient = isClient;
	localViewChangedTime = 0;
	nextTeamBalanceCheckTime = 0;

	if ( !gameLocal.isClient ) {
		SetPaused( false );
	}

	playerView.ClearEffects();
	playerView.ClearRepeaterView();
	repeaterClientFollowIndex = -1;

	if ( net_serverDownload.GetInteger() == 3 ) {
#if !defined( SD_PUBLIC_TOOLS )
		networkSystem->HTTPEnable( this->isServer || this->isRepeater );
#endif // !SD_PUBLIC_TOOLS
	}

	GetMDFExportTargets(); // Gordon: force this to be touched during level loads so that builds get it, etc

#ifndef _XENON
	if ( isServer && sdnet.NeedsGameSession() ) {
		if ( networkService->GetState() == sdNetService::SS_INITIALIZED ) {
			sdnet.Connect();
		} else if ( networkService->GetState() == sdNetService::SS_ONLINE ) {
			if ( networkService->GetDedicatedServerState() == sdNetService::DS_OFFLINE ) {
				sdnet.SignInDedicated();
			}
		}
	}
#endif

	Printf( "----------- Game Map Init ------------\n" );

	gamestate = GAMESTATE_STARTUP;
	gameRenderWorld = renderWorld;
	gameSoundWorld = soundWorld;

	sdVoteManager::GetInstance().FreeVotes();

	InitAsyncNetwork();

	if ( g_execMapConfigs.GetBool() ) {
		idStr configName = mapName;
		configName.SetFileExtension( "cfg" );
		if ( fileSystem->FindFile( configName.c_str() ) == FIND_NO ) {
			configName = "defaultmap.cfg";
		}
		idStr oldRules = si_rules.GetString();
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, va( "exec '%s'", configName.c_str() ) );
		if ( oldRules.Cmp( si_rules.GetString() ) != 0 ) {
			gameLocal.Warning( "Changing ruleset in the map config is not a good idea, ruleset has been reset." );
			si_rules.SetString( oldRules.c_str() );
		}
		sys->FlushServerInfo(); // Gordon: we may have changed timelimit or something
	}

	if ( rules != NULL ) {
		rules->NewMap( isUserChange );
	}

	UpdateCampaignStats( true );
	UpdateLevelLoadScreen( common->LocalizeText( "guis/mainmenu/loading/loading_map" ).c_str() );

	botThreadData.Clear( true );

	LoadMap( mapName, randSeed, startTime );

	OnPreMapStart();
	SpawnMapEntities();
	OnMapStart();

	if ( botMapFile != NULL ) {
        gameLocal.Printf( "----------- Loading Map Bot Actions ------------\n" );
		botThreadData.LoadActions( botMapFile ); //mal: load any bot actions for this map, if the bot entities were loaded.
		
		if ( gameLocal.isServer ) {
			botThreadData.LoadRoutes( botMapFile ); //mal: load any bot routes for this map, if the bot entities were loaded.

			idStr navName = botMapFile->GetName();
			navName.SetFileExtension( "nav" );
			botThreadData.botVehicleNodes.LoadNodes( navName );
		}
	}

	botThreadData.Init();
	botThreadData.LoadBotNames();
	botThreadData.LoadTrainingBotNames();

	gamestate = GAMESTATE_ACTIVE;

	botThread->StartThread();

	hasTakenScoreShot = false;

	Printf( "--------------------------------------\n" );

	guidFile.RemoveOldEntries();
}

/*
===========
idGameLocal::MapClear
===========
*/
void idGameLocal::MapClear( bool clearClients ) {
	idLight::OnMapClear();
	idSound::OnMapClear();
	idDoor::OnMapClear();
	sdLocationMarker::OnMapClear( clearClients );

	if ( gameRenderWorld ) {
		gameRenderWorld->ClearDecals();
	}

	sdWakeManager::GetInstance().Deinit();
	tireTreadManager->Deinit();
	footPrintManager->Deinit();

	int checkCount = 1000;
	bool listEmpty = false;
	while ( !listEmpty ) {
		checkCount--;
		if ( checkCount == 0 ) {
			gameLocal.Error( "idGameLocal::MapClear Failed to remove all entities, loading a new map will likely cause the game to crash" );
			break;
		}
		listEmpty = true;
		for( int i = ( clearClients ? 0 : MAX_CLIENTS ); i < MAX_GENTITIES; i++ ) {
			if ( !entities[ i ] ) {
				continue;
			}
			listEmpty = false;

			entities[ i ]->ProcessEvent( &EV_Remove );

			// ~idEntity is in charge of setting the pointer to NULL
			// it will also clear pending events for this entity
			assert( !entities[ i ] );
			spawnIds[ i ] = -1;
		}
	}

	entityHash.Clear( 1024, MAX_GENTITIES );

	if ( !clearClients ) {
		// add back the hashes of the clients
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			if ( !entities[ i ] ) {
				continue;
			}
			entityHash.Add( entityHash.GenerateKey( entities[ i ]->name.c_str(), true ), i );
		}
	}

	int entityStart = MAX_CLIENTS;
	if ( clearClients ) {
		entityStart = 0;
	}

	for ( int i = 0; i < entityCollections.Num(); i++ ) {
		for ( int j = entityStart; j < MAX_GENTITIES; j++ ) {
			if ( entityCollections[ i ]->Contains( j ) ) {
				assert( false );
			}
		}
	}

	if ( editEntities ) {
		delete editEntities;
		editEntities = NULL;
	}

	for( int i = 0; i < MAX_CENTITIES; i++ ) {
		delete clientEntities[ i ];
		assert( !clientEntities[ i ] );
		clientSpawnIds[ i ] = -INITIAL_SPAWN_COUNT;
	}

	if ( debugLogFile != NULL ) {
		fileSystem->CloseFile( debugLogFile );
		debugLogFile = NULL;
	}
}

/*
===========
idGameLocal::MapShutdown
============
*/
void idGameLocal::MapShutdown( void ) {
	if ( gamestate == GAMESTATE_NOMAP ) {
		return;
	}
	assert( gamestate != GAMESTATE_STARTUP );

	if( rules != NULL ) {
		rules->SetWinner( NULL );
	}

	Printf( "--------- Game Map Shutdown ----------\n" );

	gamestate = GAMESTATE_SHUTDOWN;

	botThread->StopThread();
	botThreadData.Clear( true );

#if !defined( SD_DEMO_BUILD )
	FreeClientStatsRequestTask();
	clientStatsRequestIndex = -1;
#endif /* !SD_DEMO_BUILD */

	if ( gameRenderWorld != NULL ) {
		// clear any debug lines, text, and polygons
		gameRenderWorld->DebugClearLines( 0 );
		gameRenderWorld->DebugClearPolygons( 0 );

		gameRenderWorld->ClearDecals();
		sdEffect::FreeDeadEffects();
	}


	sdWakeManager::GetInstance().Deinit();
	tireTreadManager->Deinit();
	footPrintManager->Deinit();

	adManager->SetAdZone( "" );

	sdDemoManager::GetInstance().EndDemo();

	MapClear( true );

	if ( gameRenderWorld != NULL ) {
		gameRenderWorld->FreeStoppedEffectDefs();
	}

	renderSystem->SyncRenderSystem();

	if ( gameSoundWorld != NULL ) {
		gameSoundWorld->PlayShaderDirectly( NULL, SND_PLAYER_TOOLTIP );
		gameSoundWorld->FadeSoundClasses( 0, 0, 0.5f );
		gameSoundWorld->PlaceListener( playerView.GetCurrentView().vieworg, playerView.GetCurrentView().viewaxis, -1, time );
	}

	sdObjectiveManager::GetInstance().OnMapShutdown();
	sdTaskManager::GetInstance().OnMapShutdown();
	sdAntiLagManager::GetInstance().OnMapShutdown();

	pvs.Shutdown();

	clip.Shutdown();

	traceModelCache.ClearTraceModelCache();

	// free the collision map
	collisionModelManager->PurgeModels();

	ShutdownAsyncNetwork();

	mapFileName.Clear();

	gameRenderWorld = NULL;
	gameSoundWorld = NULL;

	if( !networkSystem->IsDedicated() ) {
		localPlayerProperties.ShutdownGUIs();
	}

	gamestate = GAMESTATE_NOMAP;

	if ( gameLocal.program != NULL ) {
		gameLocal.program->PruneThreads();
	}

	LoadMainMenuPartialMedia( false );
	PurgeAndLoadTeamAssets( NULL );

	mapMetaData = NULL;

	if( sdUserInterfaceScope* scope = gameLocal.globalProperties.GetSubScope( "game" ) ) {
		if( sdProperties::sdProperty* property = scope->GetProperty( "isRunning", sdProperties::PT_FLOAT )) {
			*property->value.floatValue = GameState() > GAMESTATE_NOMAP ? 1.0f : 0.0f;
		}
	}

	sdProfileHelperManager::GetInstance().StopAll();

	Printf( "--------------------------------------\n" );
}

/*
===================
idGameLocal::DumpOggSounds
===================
*/
void idGameLocal::DumpOggSounds() {
	int i, j, k, size;
#if 0
	int totalSize;
	idFile *file, *delFile;
#endif
	idStrList oggSounds, highQualOggSounds, localizedVOSounds;
	const idSoundShader *soundShader;
	const soundShaderParms_t *parms;
	idStr soundName;

	for ( i = 0; i < declHolder.declSoundShaderType.Num(); i++ ) {
		soundShader = declHolder.FindSoundShaderByIndex( i, false );
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

				// use higher quality sounds for VO
				/*if ( soundName.Find( "sounds/vo/", false ) != idStr::INVALID_POSITION ) {
					highQualOggSounds.AddUnique( soundName );
					continue;
				}*/

				if ( soundName.Find( "localization/english/sounds/", false ) != idStr::INVALID_POSITION ) {
					// use a separate list for localized VO
					localizedVOSounds.AddUnique( soundName );
					continue;
				}

				if ( !soundShader->IsOGGCompressed() ) {
					continue;
				}

				for ( k = 0; k < shakeSounds.Num(); k++ ) {
					if ( shakeSounds[ k ].IcmpPath( soundName ) == 0 ) {
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

	idFile* pyFile = fileSystem->OpenFileWrite( "makeogg.py" );
	
	// write generic code
	pyFile->WriteFloatString( "#!/usr/bin/env python\n\n" );

	pyFile->WriteFloatString( "import os\n" );
	pyFile->WriteFloatString( "import sys\n" );
	pyFile->WriteFloatString( "import subprocess\n" );
	pyFile->WriteFloatString( "import time\n" );
	pyFile->WriteFloatString( "import traceback\n" );
	pyFile->WriteFloatString( "from optparse import OptionParser\n" );
	pyFile->WriteFloatString( "from threading import Thread, Lock\n\n" );

	pyFile->WriteFloatString( "def SoundFile( sound, language ):\n" );
	pyFile->WriteFloatString( "    sourceFile = sound\n" );
	pyFile->WriteFloatString( "    if language is not None:\n" );
	pyFile->WriteFloatString( "        sourceFile = sourceFile.replace( '$LANGUAGE$', language )\n" );
	pyFile->WriteFloatString( "    return sourceFile\n\n" );

	pyFile->WriteFloatString( "class OggEnc2Thread( Thread ):\n" );
	pyFile->WriteFloatString( "    def __init__( self, encodeSounds, language=None ):\n" );
	pyFile->WriteFloatString( "        Thread.__init__( self )\n\n" );

	pyFile->WriteFloatString( "        self.encodeSounds = encodeSounds\n" );
	pyFile->WriteFloatString( "        self.language = language\n" );
	pyFile->WriteFloatString( "        self.stop = False\n" );
	pyFile->WriteFloatString( "        self.error = False\n\n" );

	pyFile->WriteFloatString( "    def Stop( self ):\n" );
	pyFile->WriteFloatString( "        if self.isAlive():\n" );
	pyFile->WriteFloatString( "            self.stop = True\n\n" );

	pyFile->WriteFloatString( "    def OnCompleted( self ):\n" );
	pyFile->WriteFloatString( "        return\n\n" );

	pyFile->WriteFloatString( "    def run( self ):\n" );
	pyFile->WriteFloatString( "        self.stop = False\n" );
	pyFile->WriteFloatString( "        self.error = False\n\n" );

	pyFile->WriteFloatString( "        for sound in self.encodeSounds:\n" );
	pyFile->WriteFloatString( "            if self.stop:\n" );
	pyFile->WriteFloatString( "                return\n\n" );

	pyFile->WriteFloatString( "            sourceFile = SoundFile( sound[0], self.language )\n" );
	pyFile->WriteFloatString( "            destFile = SoundFile( sound[1], self.language )\n\n" );

	pyFile->WriteFloatString( "            try:\n" );
	pyFile->WriteFloatString( "                if not self.EncodeSound( sourceFile, destFile, sound[2] ):\n" );
	pyFile->WriteFloatString( "                    self.error = True\n" );
	pyFile->WriteFloatString( "                    return\n" );
	pyFile->WriteFloatString( "            except:\n" );
	pyFile->WriteFloatString( "                sys.stdout.write( traceback.format_exc() )\n" );
	pyFile->WriteFloatString( "                return\n\n" );

	pyFile->WriteFloatString( "    def OggEnc2( self, sourceFile, destFile, quality ):\n" );
	pyFile->WriteFloatString( "        returnCode = 1\n" );
	pyFile->WriteFloatString( "        stdoutLines = []\n" );
	pyFile->WriteFloatString( "        stderrLines = []\n" );
	pyFile->WriteFloatString( "        error = ''\n\n" );

	pyFile->WriteFloatString( "        # error if the source file doesn't exist\n" );
	pyFile->WriteFloatString( "        if not os.path.exists( sourceFile ):\n" );
	pyFile->WriteFloatString( "            sys.stdout.write( 'OggEnc2 failed: %%s (%%i)\\\\n' %% ( 'source file missing', 1 ) )\n" );
	pyFile->WriteFloatString( "            return False\n\n" );

	pyFile->WriteFloatString( "        try:\n" );
	pyFile->WriteFloatString( "            p = subprocess.Popen( 'oggenc2 -Q -q %%d -o \"%%s\" \"%%s\"' %% ( quality, destFile, sourceFile ), stdout=subprocess.PIPE, stderr=subprocess.PIPE )\n\n" );

	pyFile->WriteFloatString( "            stdoutLines = p.stdout.readlines()\n" );
	pyFile->WriteFloatString( "            stderrLines = []#p.stderr.readlines()\n\n" );

	pyFile->WriteFloatString( "            returnCode = p.wait()\n" );
	pyFile->WriteFloatString( "        except OSError, e:\n" );
	pyFile->WriteFloatString( "            error = e\n\n" );

	pyFile->WriteFloatString( "        if not os.path.exists( destFile ):\n" );
	pyFile->WriteFloatString( "            error = 'output file missing'\n" );
	pyFile->WriteFloatString( "            returnCode = 1\n\n" );

	pyFile->WriteFloatString( "        sys.stdout.writelines( stdoutLines )\n" );
	pyFile->WriteFloatString( "        sys.stderr.writelines( stderrLines )\n\n" );

	pyFile->WriteFloatString( "        if returnCode == 0:\n" );
	pyFile->WriteFloatString( "            return True\n" );
	pyFile->WriteFloatString( "        else:\n" );
	pyFile->WriteFloatString( "            sys.stdout.write( 'OggEnc2 failed: %%s (%%i)\\\\n' %% ( error, returnCode ) )\n" );
	pyFile->WriteFloatString( "            return False\n\n" );

	pyFile->WriteFloatString( "    def EncodeSound( self, sourceFile, destFile, quality ):\n" );
	pyFile->WriteFloatString( "        sourceFile = os.path.normpath( sourceFile )\n" );
	pyFile->WriteFloatString( "        destFile = os.path.normpath( destFile )\n\n" );

	pyFile->WriteFloatString( "        # skip already existing files\n" );
	pyFile->WriteFloatString( "        if os.path.exists( destFile ):\n" );
	pyFile->WriteFloatString( "            return True\n\n" );

	pyFile->WriteFloatString( "        sys.stdout.write( 'Encoding to %%s\\\\n' %% destFile )\n\n" );

	pyFile->WriteFloatString( "        destPath = os.path.dirname( destFile )\n" );
	pyFile->WriteFloatString( "        if not os.path.exists( destPath ):\n" );
	pyFile->WriteFloatString( "            sys.stdout.write( 'Output directory \\\\'%%s\\\\' does not exist\\\\n' %% destPath )\n" );
	pyFile->WriteFloatString( "            return False\n\n" );

	pyFile->WriteFloatString( "        if not self.OggEnc2( sourceFile, destFile, quality ):\n" );
	pyFile->WriteFloatString( "            return False\n\n" );

	pyFile->WriteFloatString( "        return True\n\n" );

	pyFile->WriteFloatString( "def CreateDirectories( encodeSounds, language=None ):\n" );
	pyFile->WriteFloatString( "    for sound in encodeSounds:\n" );
	pyFile->WriteFloatString( "        destFile = os.path.normpath( SoundFile( sound[1], language ) )\n" );
	pyFile->WriteFloatString( "        destPath = os.path.dirname( destFile )\n\n" );

	pyFile->WriteFloatString( "        if not os.path.exists( destPath ):\n" );
	pyFile->WriteFloatString( "            try:\n" );
	pyFile->WriteFloatString( "                os.makedirs( destPath )\n" );
	pyFile->WriteFloatString( "            except os.error, e:\n" );
	pyFile->WriteFloatString( "                sys.stdout.write( 'Directory creation failed: %%s (%%i)\\\\n' %% ( e.strerror, e.errno ) )\n" );
	pyFile->WriteFloatString( "                return False\n\n" );

	pyFile->WriteFloatString( "    return True\n\n" );

	cpuInfo_t cpuInfo;
	sys->GetCPUInfo( cpuInfo );

	pyFile->WriteFloatString( "cfg_number_of_hardware_threads = %d\n\n", cpuInfo.physicalNum );

	pyFile->WriteFloatString( "def RunThreads( threadClass, encodeSounds, language=None ):\n" );
	pyFile->WriteFloatString( "    threads = []\n\n" );

	pyFile->WriteFloatString( "    offset = 0\n" );
	pyFile->WriteFloatString( "    step = len( encodeSounds ) / cfg_number_of_hardware_threads\n" );
	pyFile->WriteFloatString( "    for core in xrange( 0, cfg_number_of_hardware_threads ):\n" );
	pyFile->WriteFloatString( "        if core == cfg_number_of_hardware_threads - 1:\n" );
	pyFile->WriteFloatString( "            start = offset\n" );
	pyFile->WriteFloatString( "            end = len( encodeSounds )\n" );
	pyFile->WriteFloatString( "        else:\n" );
	pyFile->WriteFloatString( "            start = offset\n" );
	pyFile->WriteFloatString( "            end = offset + step\n" );
	pyFile->WriteFloatString( "            offset = end\n\n" );

	pyFile->WriteFloatString( "        threads.append( threadClass( encodeSounds[start:end], language ) )\n\n" );

	pyFile->WriteFloatString( "    for thread in threads:\n" );
	pyFile->WriteFloatString( "        thread.start()\n\n" );

	pyFile->WriteFloatString( "    error = False\n" );
	pyFile->WriteFloatString( "    while True:  \n" );
	pyFile->WriteFloatString( "        time.sleep( 1 )\n" );
	pyFile->WriteFloatString( "        for thread in threads:\n" );
	pyFile->WriteFloatString( "            error |= thread.error\n\n" );

 	pyFile->WriteFloatString( "        if error:\n" );
	pyFile->WriteFloatString( "            for thread in threads:\n" );
	pyFile->WriteFloatString( "                if thread.isAlive():\n" );
	pyFile->WriteFloatString( "                    thread.Stop()\n" );
	pyFile->WriteFloatString( "                    thread.join()\n" );
 	pyFile->WriteFloatString( "            return 1\n\n" );

	pyFile->WriteFloatString( "        terminate = True\n" );
	pyFile->WriteFloatString( "        for thread in threads:\n" );
 	pyFile->WriteFloatString( "            terminate &= not thread.isAlive()\n\n" );

	pyFile->WriteFloatString( "        if terminate:\n" );
	pyFile->WriteFloatString( "            for thread in threads:\n" );
	pyFile->WriteFloatString( "                thread.OnCompleted()\n" );
	pyFile->WriteFloatString( "            return 0\n\n" );

	pyFile->WriteFloatString( "def vararg_strings( option, opt_str, value, parser ):\n" );
	pyFile->WriteFloatString( "    assert value is None\n" );
	pyFile->WriteFloatString( "    found = 0\n" );
	pyFile->WriteFloatString( "    value = []\n" );
	pyFile->WriteFloatString( "    rargs = parser.rargs\n" );
	pyFile->WriteFloatString( "    while rargs:\n" );
	pyFile->WriteFloatString( "        arg = rargs[0]\n\n" );

	pyFile->WriteFloatString( "        # Stop if we hit an arg like \"--foo\", \"-a\", \"-fx\", \"--file=f\", etc\n" );
	pyFile->WriteFloatString( "        if ( ( arg[:2] == '--' and len( arg ) > 2 ) or\n" );
	pyFile->WriteFloatString( "             ( arg[:1] == '-' and len( arg ) > 1 and arg[1] != '-' ) ):\n" );
	pyFile->WriteFloatString( "            break\n" );
	pyFile->WriteFloatString( "        else:\n" );
	pyFile->WriteFloatString( "            value.append( arg )\n" );
	pyFile->WriteFloatString( "            del rargs[0]\n\n" );

	pyFile->WriteFloatString( "        setattr( parser.values, option.dest, value )\n\n" );

	pyFile->WriteFloatString( "        found += 1\n\n" );

	pyFile->WriteFloatString( "    if found == 0:\n" );
	pyFile->WriteFloatString( "        raise optparse.OptionValueError( '%%s option requires an argument' %% opt_str )\n\n" );

	pyFile->WriteFloatString( "def main():\n" );
	pyFile->WriteFloatString( "    encodeSounds = [\n" );
	/*
	 highQualOggSounds
	*/
	for ( i = 0; i < highQualOggSounds.Num(); i++ ) {
		size = fileSystem->ReadFile( highQualOggSounds[i], NULL, NULL );

		idStr oggFileName = highQualOggSounds[i];
		oggFileName.SetFileExtension( ".ogg" );

		idStr outFile = va( "%s" PREGENERATED_BASEDIR "/ogg/%s", fileSystem->RelativePathToOSPath( "", "fs_savepath" ), oggFileName.c_str() );
		idStr inFile = fileSystem->RelativePathToOSPath( highQualOggSounds[i].c_str(), "fs_basepath" );

		outFile.BackSlashesToSlashes();
		inFile.BackSlashesToSlashes();

		pyFile->WriteFloatString( "        [ '%s', '%s', 4 ],\n", inFile.c_str(), outFile.c_str() );
	}

	/*
	 oggSounds
	*/
	for ( i = 0; i < oggSounds.Num(); i++ ) {
		size = fileSystem->ReadFile( oggSounds[i], NULL, NULL );

		idStr oggFileName = oggSounds[i];
		oggFileName.SetFileExtension( ".ogg" );

		idStr outFile = va( "%s" PREGENERATED_BASEDIR "/ogg/%s", fileSystem->RelativePathToOSPath( "", "fs_savepath" ), oggFileName.c_str() );
		idStr inFile = fileSystem->RelativePathToOSPath( oggSounds[i].c_str(), "fs_basepath" );

		outFile.BackSlashesToSlashes();
		inFile.BackSlashesToSlashes();

		pyFile->WriteFloatString( "        [ '%s', '%s', 0 ],\n", inFile.c_str(), outFile.c_str() );
	}

	/*
	 end
	*/
	pyFile->WriteFloatString( "    ]\n\n" );

	pyFile->WriteFloatString( "    localizedEncodeSounds = [\n" );
	/*
	 localizedVOSounds
	*/
	for ( i = 0; i < localizedVOSounds.Num(); i++ ) {
		size = fileSystem->ReadFile( localizedVOSounds[i], NULL, NULL );

		idStr oggFileName = localizedVOSounds[i];
		oggFileName.SetFileExtension( ".ogg" );

		idStr outFile = va( "%s" PREGENERATED_BASEDIR "/ogg/%s", fileSystem->RelativePathToOSPath( "", "fs_savepath" ), oggFileName.c_str() );
		idStr inFile = fileSystem->RelativePathToOSPath( localizedVOSounds[i].c_str(), "fs_cdpath" );

		outFile.BackSlashesToSlashes();
		inFile.BackSlashesToSlashes();

		outFile.Replace( "/localization/english/sounds/", "/localization/$LANGUAGE$/sounds/" );
		inFile.Replace( "/localization/english/sounds/", "/localization/$LANGUAGE$/sounds/" );

		pyFile->WriteFloatString( "        [ '%s', '%s', 4 ],\n", inFile.c_str(), outFile.c_str() );
	}

	/*
	 end
	*/
	pyFile->WriteFloatString( "    ]\n\n" );

	pyFile->WriteFloatString( "    parser = OptionParser( 'usage: %%prog [options]' )\n" );
	pyFile->WriteFloatString( "    parser.add_option( '-l', '--languages', dest='languages', help='VO Languages to encode', action='callback', callback=vararg_strings )\n\n" );

	pyFile->WriteFloatString( "    ( options, args ) = parser.parse_args()\n\n" );

	pyFile->WriteFloatString( "    if not CreateDirectories( encodeSounds ):\n" );
	pyFile->WriteFloatString( "        return 1\n\n" );

	pyFile->WriteFloatString( "    status = RunThreads( OggEnc2Thread, encodeSounds )\n" );
	pyFile->WriteFloatString( "    if status != 0:\n" );
	pyFile->WriteFloatString( "        return status\n\n" );

	pyFile->WriteFloatString( "    if options.languages is not None:\n" );
	pyFile->WriteFloatString( "        for language in options.languages:\n" );
	pyFile->WriteFloatString( "            if not CreateDirectories( localizedEncodeSounds, language ):\n" );
	pyFile->WriteFloatString( "                return 1\n\n" );

	pyFile->WriteFloatString( "            status = RunThreads( OggEnc2Thread, localizedEncodeSounds, language )\n" );
	pyFile->WriteFloatString( "            if status != 0:\n" );
	pyFile->WriteFloatString( "                return status\n\n" );

	pyFile->WriteFloatString( "    return 0\n\n" );

	pyFile->WriteFloatString( "if __name__ == '__main__':\n" );
	pyFile->WriteFloatString( "    sys.exit( main() )\n" );

	fileSystem->CloseFile( pyFile );

	shakeSounds.Clear();
}

/*
===================
idGameLocal::GetShakeSounds
===================
*/
void idGameLocal::GetShakeSounds( const idDict& dict ) {
	const idSoundShader*	soundShader;
	const char*				soundShaderName;
	idStr					soundName;

	soundShaderName = dict.GetString( "s_shader" );
	if ( soundShaderName != '\0' && dict.GetFloat( "s_shakes" ) != 0.0f ) {
		soundShader = declHolder.declSoundShaderType.LocalFind( soundShaderName );

		for ( int i = 0; i < soundShader->GetNumSounds(); i++ ) {
			soundName = soundShader->GetSound( i );
			soundName.BackSlashesToSlashes();

			shakeSounds.AddUnique( soundName );
		}
	}
}

/*
===================
idGameLocal::FinishBuild
===================
*/
void idGameLocal::FinishBuild( void ) {
	if ( !cvarSystem->GetCVarBool( "com_makingBuild") ) {
		return;
	}

	DumpOggSounds();
}

/*
===================
idGameLocal::CacheDictionaryMedia

This is called after parsing an EntityDef and for each entity spawnArgs before
merging the entitydef.  It could be done post-merge, but that would
avoid the fast pre-cache check associated with each entityDef
===================
*/
void idGameLocal::CacheDictionaryMedia( const idDict& dict ) {
	if ( doingMapRestart ) {
		return;
	}

	if ( !g_cacheDictionaryMedia.GetBool() ) {
		return;
	}

	if ( cvarSystem->GetCVarBool( "com_makingBuild" ) ) {
		GetShakeSounds( dict );
	}
	static int counter = 0;
	if( ( ( counter & 15 ) == 0 ) && !networkSystem->IsDedicated() ) {
		common->PacifierUpdate();
	}

	declManager->CacheFromDict( dict );
}

/*
===========
idGameLocal::InitScriptForMap
============
*/
void idGameLocal::InitScriptForMap( void ) {
	// create a thread to run frame commands on
	if ( !frameCommandThread ) {
		frameCommandThread = gameLocal.program->CreateThread();
		frameCommandThread->ManualDelete();
		frameCommandThread->SetName( "frameCommands" );
	}

	// run the main game script function (not the level specific main)
	const sdProgram::sdFunction* func = program->FindFunction( SCRIPT_DEFAULTFUNC );
	if ( func != NULL ) {
		frameCommandThread->CallFunction( func );
		if ( !frameCommandThread->Execute() ) {
			frameCommandThread->EndThread();
			gameLocal.Error( "idGameLocal::InitScriptForMap Startup Function May Not be Blocking" );
		}
	}
}

/*
===========
idGameLocal::SpawnPlayer
============
*/
void idGameLocal::SpawnPlayer( int clientNum, bool isBot ) {
	// they can connect
	Printf( "SpawnPlayer: %i\n", clientNum );

	idDict args;
	args.SetInt( "spawn_entnum", clientNum );

	if ( isBot ) {
		args.Set( "classname", "bot_edf" );
		botThreadData.SetupBotInfo( clientNum ); 
	} else {
		args.Set( "classname", "player_edf" );
		botThreadData.GetGameWorldState()->clientInfo[ clientNum ].isBot = false;
	}

	botThreadData.InitClientInfo( clientNum, true, false );

	if ( !SpawnEntityDef( args, true ) ) {
		Error( "Failed to spawn player as '%s'", args.GetString( "classname" ) );
	}
}

/*
================
idGameLocal::GetClientByName
================
*/
idPlayer* idGameLocal::GetClientByName( const char *name ) const {
	idStr temp( name );
	gameLocal.CleanName( temp );

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = GetClient( i );
		if ( !player ) {
			continue;
		}

		if ( temp.Icmp( player->userInfo.cleanName ) == 0 ) {
			return player;
		}
	}

	return NULL;
}

/*
================
idGameLocal::IsLocalViewPlayer
================
*/
bool idGameLocal::IsLocalViewPlayer( const idEntity* player ) const {
	return player == GetLocalViewPlayer();
}

/*
================
idGameLocal::GetLocalViewPlayer
================
*/
idPlayer* idGameLocal::GetLocalViewPlayer( void ) const {
	return GetActiveViewer();
}

/*
================
idGameLocal::DoClientSideStuff
================
*/
bool idGameLocal::DoClientSideStuff() const { 
	return !networkSystem->IsDedicated();
}

/*
================
idGameLocal::OnLocalViewPlayerChanged
================
*/
void idGameLocal::OnLocalViewPlayerChanged( void ) {
	UpdatePlayerShadows();
	sdObjectiveManager::GetInstance().OnLocalViewPlayerChanged();
	localViewChangedTime = realClientTime;
	playerView.ClearEffects();
	
	ResetTeamAssets();

	idPlayer* player = GetLocalViewPlayer();
	if ( player != NULL ) {
		player->ClearDamageDealt();
		localPlayerProperties.SetActiveCamera( player->GetRemoteCamera() );
		localPlayerProperties.SetActivePlayer( player );
		localPlayerProperties.SetActiveWeapon( player->GetWeapon() );
	} else {
		localPlayerProperties.SetActiveCamera( NULL );
		localPlayerProperties.SetActivePlayer( NULL );
		localPlayerProperties.SetActiveWeapon( NULL );
	}
}

/*
================
idGameLocal::OnLocalViewPlayerChanged
================
*/
void idGameLocal::OnLocalViewPlayerTeamChanged( void ) {
	sdObjectiveManager::GetInstance().OnLocalViewPlayerTeamChanged();
	ResetTeamAssets();
}

/*
================
idGameLocal::ResetTeamAssets
================
*/
void idGameLocal::ResetTeamAssets( void ) {
	idPlayer* player = GetLocalViewPlayer();
	if ( player == NULL ) {
		return;
	}

	sdTeamInfo* team = player->GetGameTeam();
	if ( team == NULL ) {
		return;
	}

	PurgeAndLoadTeamAssets( declStringMapType[ team->GetDict().GetString( "partial_load" ) ] );
}

int SortPlayersByXP( const idPlayer* a, const idPlayer* b ) {
	return ( int )( a->GetProficiencyTable().GetXP() - b->GetProficiencyTable().GetXP() );
}

/*
================
idGameLocal::FindUnbalancedTeam
================
*/
sdTeamInfo* idGameLocal::FindUnbalancedTeam( sdTeamInfo** lowest ) {
	const int TEAM_BALANCE_CHECK_THRESHOLD = 2;

	int numTeams = sdTeamManager::GetInstance().GetNumTeams();

	size_t arraySize = sizeof( int ) * numTeams;
	int* teamPlayerCounts = ( int* )_alloca( arraySize );
	memset( teamPlayerCounts, 0, arraySize );

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = GetClient( i );
		if ( player == NULL ) {
			continue;
		}

		sdTeamInfo* team = player->GetGameTeam();
		if ( team == NULL ) {
			continue;
		}

		teamPlayerCounts[ team->GetIndex() ]++;
	}

	int highestTeam = -1;
	int lowestTeam = -1;
	for ( int i = 0; i < numTeams; i++ ) {
		if ( highestTeam == -1 || ( teamPlayerCounts[ i ] > teamPlayerCounts[ highestTeam ] ) ) {
			highestTeam = i;
		}

		if ( lowestTeam == -1 || ( teamPlayerCounts[ i ] < teamPlayerCounts[ lowestTeam ] ) ) {
			lowestTeam = i;
		}
	}

	if ( ( teamPlayerCounts[ highestTeam ] - teamPlayerCounts[ lowestTeam ] ) < TEAM_BALANCE_CHECK_THRESHOLD ) {
		return NULL;
	}

	if ( lowest != NULL ) {
		*lowest = &sdTeamManager::GetInstance().GetTeamByIndex( lowestTeam );
	}
	return &sdTeamManager::GetInstance().GetTeamByIndex( highestTeam );
}

/*
================
idGameLocal::CheckTeamBalance
================
*/
void idGameLocal::CheckTeamBalance( void ) {
	bool valid = si_teamForceBalance.GetBool() && gameLocal.rules->GetState() == sdGameRules::GS_GAMEON && g_smartTeamBalance.GetBool() && g_smartTeamBalanceReward.GetInteger() > 0;

	class sdBalanceTeamSwitchFinalizer : public sdVoteFinalizer {
	public:
		sdBalanceTeamSwitchFinalizer( idPlayer* player, sdTeamInfo* team ) {
			this->player		= player;
			this->team			= player->GetGameTeam();
			this->switchTeam	= team;
		}

		void OnVoteCompleted( bool passed ) const {
			if ( !passed ) {
				return;
			}

			sdTeamInfo* switchTeam;
			sdTeamInfo* team = gameLocal.FindUnbalancedTeam( &switchTeam );
			if ( !CheckValid( team, switchTeam ) ) {
				return;
			}

			idPlayer* player = this->player;
			player->GiveClassProficiency( g_smartTeamBalanceReward.GetInteger(), "helped team balance" );
			gameLocal.rules->SetClientTeam( player, switchTeam->GetIndex() + 1, true, "" );
		}

		bool CheckValid( sdTeamInfo* team, sdTeamInfo* switchTeam ) const {
			idPlayer* player = this->player;
			if ( this->team != team || this->switchTeam != switchTeam || player == NULL || player->GetGameTeam() != this->team ) {
				return false;
			}
			return true;
		}

	private:
		idEntityPtr< idPlayer >		player;
		sdTeamInfo*					team;
		sdTeamInfo*					switchTeam;
	};

	sdTeamInfo* switchTeam;
	sdTeamInfo* team = FindUnbalancedTeam( &switchTeam );

	sdPlayerVote* vote = sdVoteManager::GetInstance().FindVote( VI_SWITCH_TEAM );
	if ( vote != NULL ) {
		bool cancel = false;

		sdBalanceTeamSwitchFinalizer* finalizer = ( sdBalanceTeamSwitchFinalizer* )vote->GetFinalizer();
		if ( valid && finalizer->CheckValid( team, switchTeam ) ) {
			return;
		}
		sdVoteManager::GetInstance().CancelVote( vote );
	}

	if ( !valid || team == NULL ) {
		return;
	}

	if ( gameLocal.time < nextTeamBalanceCheckTime ) {
		return;
	}
	nextTeamBalanceCheckTime = gameLocal.time + SEC2MS( 20 );

	idStaticList< idPlayer*, MAX_CLIENTS > sortedPlayers;
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = GetClient( i );
		if ( player == NULL ) {
			continue;
		}

		if ( player->GetGameTeam() != team ) {
			continue;
		}

		if ( player->userInfo.isBot ) {
			continue;
		}

		if ( player->GetNextTeamBalanceSwitchTime() > gameLocal.time ) {
			continue;
		}

		sortedPlayers.Append( player );
	}

	if ( sortedPlayers.Num() == 0 ) {
		return;
	}

	sortedPlayers.Sort( SortPlayersByXP );

	int index = ( int )idMath::Floor( sortedPlayers.Num() * 0.33f ); // Gordon: Aim for someone who has a decent amount of XP, but not right at the top, as they are less likely to switch

	vote = sdVoteManager::GetInstance().AllocVote();
	if ( vote != NULL ) {
		idPlayer* player = sortedPlayers[ index ];
		player->SetNextTeamBalanceSwitchTime( gameLocal.time + MINS2MS( 2 ) );

		vote->DisableFinishMessage();
		vote->MakePrivateVote( player );
		vote->Tag( VI_SWITCH_TEAM, player );
		vote->SetText( gameLocal.declToolTipType[ "unbalanced_teams_switch" ] );
		vote->AddTextParm( switchTeam->GetTitle() );
		vote->AddTextParm( va( L"%d", g_smartTeamBalanceReward.GetInteger() ) );
		vote->SetFinalizer( new sdBalanceTeamSwitchFinalizer( player, switchTeam ) );
		vote->Start();
	}
}

/*
================
idGameLocal::UpdateGravity
================
*/
void idGameLocal::UpdateGravity( void ) {
	if ( g_gravity.IsModified() ) {
		if ( g_gravity.GetFloat() == 0.0f ) {
			g_gravity.SetFloat( 1.0f );
		}
        gravity.Set( 0, 0, -g_gravity.GetFloat() );
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
	idEntity* nextEnt;

	// if the active entity list needs to be reordered to place physics team masters at the front
	if ( sortTeamMasters ) {
		for ( idEntity* ent = activeEntities.Next(); ent != NULL; ent = nextEnt ) {
			nextEnt = ent->activeNode.Next();

			idEntity* master = ent->GetTeamMaster();
			if ( master && master == ent ) {
				ent->activeNode.AddToFront( activeEntities );
			}
		}
	}

	// if the active entity list needs to be reordered to place pushers at the front
	if ( sortPushers ) {
		for ( idEntity* ent = activeEntities.Next(); ent != NULL; ent = nextEnt ) {
			nextEnt = ent->activeNode.Next();

			idEntity* master = ent->GetTeamMaster();
			if ( !master || master == ent ) {
				// check if there is an actor on the team
				idEntity* part;
				for ( part = ent; part != NULL; part = part->GetNextTeamEntity() ) {
					if ( part->GetPhysics()->IsType( idPhysics_Actor::Type ) ) {
						break;
					}
				}
				// if there is an actor on the team
				if ( part ) {
					ent->activeNode.AddToFront( activeEntities );
				}
			}
		}
	}

	sortTeamMasters = false;
	sortPushers = false;
}

/*
===============
idGameLocal::ControlBotPopulation
===============
*/
void idGameLocal::ControlBotPopulation( void ) {
	
	if( !gameLocal.isServer ) {
		return;
	}

	if ( networkSystem->IsRankedServer() ) {
		return;
	}

	if ( gamestate != GAMESTATE_ACTIVE ) {
		return;
	}

	int minBots = bot_minClients.GetInteger();

	// tick the bot population logic
	if ( bot_enable.GetBool() && bot_minClients.GetInteger() >= 0 && time > nextBotPopulationCheck ) {
		int iClient;
		int numClients = 0, numBots = 0;
		for ( iClient = 0; iClient < MAX_CLIENTS; iClient++ ) {
			if ( entities[iClient] != NULL ) {
				numClients++;
				if ( entities[iClient]->IsType( idBot::Type ) ) {
					numBots++;
				}
			}
		}

		int numGDF = botThreadData.GetNumClientsOnTeam( GDF );
		int numStrogg = botThreadData.GetNumClientsOnTeam( STROGG );
		int limitStrogg = bot_uiNumStrogg.GetInteger();
		int limitGDF = bot_uiNumGDF.GetInteger();
		int numStroggOverLimit = ( limitStrogg < 0 ) ? 0 : botThreadData.GetNumBotsOnTeam( STROGG ) - limitStrogg;
		int numGDFOverLimit = ( limitGDF < 0 ) ? 0 : botThreadData.GetNumBotsOnTeam( GDF ) - limitGDF;

		if ( ( numClients > bot_minClients.GetInteger() || ( numStroggOverLimit > 0 ) || ( numGDFOverLimit > 0 ) ) && numBots > 0 ) {
			playerTeamTypes_t team;

			if ( ( numStroggOverLimit <= 0 && numGDFOverLimit <= 0 && numStrogg > numGDF ) || ( numStroggOverLimit > 0 ) ) { //try to keep the teams even.
				team = STROGG;
			} else if ( ( numStroggOverLimit <= 0 && numGDFOverLimit <= 0 && numGDF > numStrogg ) || ( numGDFOverLimit > 0 ) ) {
				team = GDF;
			} else {
				team = ( random.RandomInt( 100 ) > 50 ) ? GDF : STROGG;
			}

			if ( team == STROGG ) {
				numBots = numStrogg;
			} else {
				numBots = numGDF;
			}

			// randomize a bot to be kicked
			numBots = random.RandomInt( numBots );
			for ( iClient = 0; iClient < MAX_CLIENTS; iClient++ ) {
				if ( entities[iClient] != NULL ) {
					if ( entities[iClient]->IsType( idBot::Type ) ) {
						if ( botThreadData.GetGameWorldState()->clientInfo[ iClient ].team != team ) {
							continue;
						}

						if ( numBots == 0 ) {
							// kick it!
							networkSystem->ServerKickClient( iClient, "", false );
							break;
						}
						numBots--;
					}
				}
			}
		} else if ( numClients < bot_minClients.GetInteger() && ( ( ( limitGDF < 0 ) || numGDF < limitGDF ) || ( ( limitStrogg < 0 ) || ( numStrogg < limitStrogg ) ) ) ) {
			cmdSystem->BufferCommandText( CMD_EXEC_NOW, "admin addbot\n" );
		} else if ( numClients == minBots && limitGDF == limitStrogg && numGDF != numStrogg && ( numGDF >= numStrogg + 2 || numStrogg >= numGDF + 2 ) ) { //mal: do some team balancing on the server when the numbers have settled, unless player wants uneven teams.
			playerTeamTypes_t team;

			if ( numGDF > numStrogg ) {
				team = GDF;
			} else {
				team = STROGG;
			}

			for ( int i = 0; i < MAX_CLIENTS; i++ ) {
				if ( entities[ i ] != NULL ) {
					if ( entities[ i ]->IsType( idBot::Type ) ) {
						if ( botThreadData.GetGameWorldState()->clientInfo[ i ].team != team ) {
							continue;
						}

						networkSystem->ServerKickClient( i, "", false );
						break;
					}
				}
			}
		}

		// next poll
		nextBotPopulationCheck = time + 3000;
	}
}

#ifdef GAME_FPU_EXCEPTIONS
extern idCVar g_fpuExceptions;
idCVar g_fpuExceptions( "g_fpuExceptions", "0", CVAR_BOOL | CVAR_GAME | CVAR_NOCHEAT, "enables fpu exception catching" );
#endif // GAME_FPU_EXCEPTIONS

idCVar g_debugStatsSetup( "g_debugStatsSetup", "0", CVAR_GAME | CVAR_BOOL | CVAR_NOCHEAT, "prints any problems with calculating a client rank when they connect to a server" );

/*
================
idGameLocal::RunFrame
================
*/
void idGameLocal::RunFrame( const usercmd_t *clientCmds, int elapsedTime ) {
	idPlayer* localPlayer = GetLocalPlayer();
	if ( localPlayer != NULL ) {
		unlock.canUnlockFrames = localPlayer->IsFPSUnlock();
	} else {
		unlock.canUnlockFrames = false;
	}

	unlock.unlockedDraw = false;
	interpolateEntities.Clear();



	idEntity*			ent;
	idTimer				timer_think, timer_events, timer_singlethink;
	idPlayer*			player;
	const renderView_t*	view;

#ifdef GAME_FPU_EXCEPTIONS
	if ( g_fpuExceptions.GetBool() ) {
		sys->FPU_EnableExceptions( FPU_EXCEPTION_DENORMALIZED_OPERAND );
	} else {
		sys->FPU_EnableExceptions( 0 );
	}
#endif // GAME_FPU_EXCEPTIONS

	assert( !isClient );

	player = GetLocalPlayer();

	isNewFrame = true;

	// update demo state
	sdDemoManagerLocal& demoManager = sdDemoManager::GetInstance();
	demoManager.RunDemoFrame( NULL );

	{

		// update the game time
		framenum++;

		if ( IsPaused() ) {
			msec = 0;
			previousTime = time;
			timeOffset += elapsedTime;
			time += 0;
		} else {
			msec = elapsedTime;
			previousTime = time;
			timeOffset += 0;
			time += elapsedTime;
		}
		realClientTime = ToGuiTime( time );

#ifdef GAME_DLL
		// allow changing SIMD usage on the fly
		if ( com_forceGenericSIMD.IsModified() ) {
			idSIMD::InitProcessor( "game", com_forceGenericSIMD.GetBool() );
		}
#endif

#ifdef CLIP_DEBUG_EXTREME
	clip.UpdateTraceLogging();
#endif
		sdProfileHelperManager::GetInstance().Update();

		// make sure the random number counter is used each frame so random events
		// are influenced by the player's actions
		random.RandomInt();

#ifndef _XENON
		UpdateGameSession();
#endif // _XENON

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

		UpdateDeploymentRequests();

		// update our gravity vector if needed.
		UpdateGravity();

		bse->StartFrame();

		timer_think.Clear();
		timer_think.Start();

		// update some of the world state for the bots
		botThreadData.UpdateState();

		// set latest bot output as current for the game
		botThreadData.SetCurrentGameOutputState();

		idEntity::ClearEntityCaches();

//mal: generate bot user cmds
		for( int i = 0; i < MAX_CLIENTS; i++ ) {
			idPlayer* botPlayer = GetClient( i );

			if ( botPlayer == NULL || botPlayer->userInfo.isBot == false ) {
				continue;
			}

			idBot* bot = botPlayer->Cast< idBot >();

			if ( bot == NULL ) {
				continue;
			}

			bot->Bot_InputToUserCmd();
		}

		for ( int i = 0; i < changedEntities.Num(); i++ ) {
			idEntity* ent = changedEntities[ i ];
			if ( !ent ) {
				continue;
			}

			if ( !ent->thinkFlags || ent->NoThink() ) {
				ent->activeNode.Remove();
			} else {
				if ( !ent->activeNode.InList() ) {
					ent->activeNode.AddToEnd( gameLocal.activeEntities );
				}
			}
		}
		changedEntities.SetNum( 0, false );

		// sort the active entity list
		SortActiveEntityList();

		int num = 0;
		for ( ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() ) {
			sdProfileHelper_ScopeTimer( "EntityThink", ent->spawnArgs.GetString( "classname" ), g_profileEntityThink.GetBool() );
			ent->Think();
			num++;
		}

		idPlayer* viewPlayer = GetLocalViewPlayer();
		if ( viewPlayer != NULL ) {
			playerView.UpdateProxyView( viewPlayer, false );
		}

		for( ent = postThinkEntities.Next(); ent; ent = ent->GetPostThinkNode()->Next() ) {
			ent->PostThink();
		}

		timer_think.Stop();
		timer_events.Clear();
		timer_events.Start();

		rvClientEntity* cent;
		for ( cent = clientSpawnedEntities.Next(); cent != NULL; cent = cent->spawnNode.Next() ) {
			cent->Think();
		}

		if ( gameLocal.localClientNum != ASYNC_DEMO_CLIENT_INDEX ) {
			localPlayerProperties.UpdateHudModules();
		}

		// Some will remove themselves
		sdEffect::UpdateDeadEffects();

		if ( viewPlayer != NULL ) {
			playerView.CalculateShake( viewPlayer );

			// fps unlock: record view position
			int index = framenum & 1;
			unlock.originlog[ index ] = viewPlayer->GetRenderView()->vieworg;
		}

		// service any pending events
		idEvent::ServiceEvents();

		timer_events.Stop();

		sdTeamManager::GetInstance().Think(); 
		sdTaskManager::GetInstance().Think();
		sdFireTeamManager::GetInstance().Think();
		sdObjectiveManager::GetInstance().Think();
		sdWayPointManager::GetInstance().Think();
		sdVoteManager::GetInstance().Think();
		sdWakeManager::GetInstance().Think();
		if ( !gameLocal.IsPaused() ) {
			sdAntiLagManager::GetInstance().Think();
		}
		tireTreadManager->Think();
		footPrintManager->Think();
		sdGlobalStatsTracker::GetInstance().UpdateStatsRequests();

		ControlBotPopulation();
		botThreadData.CheckBotClassSpread();

		// set latest world state as current for the bots
		botThreadData.SetCurrentGameWorldState();

		bse->EndFrame();

		UpdateLoggedDecals();

		// Run multiplayer game rules
		rules->Run();

		CheckTeamBalance();

		// decrease deleted clip model reference counts
		clip.ThreadDeleteClipModels();

		// delete clip models without references for real
		clip.ActuallyDeleteClipModels();

		// display how long it took to calculate the current game frame
		if ( g_frametime.GetBool() ) {
			Printf( "game %d: all:%.1f th:%.1f ev:%.1f %d ents \n",
				time, timer_think.Milliseconds() + timer_events.Milliseconds(),
				timer_think.Milliseconds(), timer_events.Milliseconds(), num );
		}
	}

	UpdateServerRankStats();

#ifdef GAME_FPU_EXCEPTIONS
	sys->FPU_EnableExceptions( 0 );
#endif // GAME_FPU_EXCEPTIONS

	demoManager.EndDemoFrame();

	// show any debug info for this frame
	RunDebugInfo();
	D_DrawDebugLines();
}

/*
============
idGameLocal::UpdateServerRankStats
============
*/
void idGameLocal::UpdateServerRankStats( void ) {
	if ( !clientStatsRequestsPending ) {
		return;
	}

	if ( !gameLocal.isServer ) {
		return;
	}

#if !defined( SD_DEMO_BUILD )
	if ( clientStatsRequestIndex != -1 ) {
		if ( !clientConnected[ clientStatsRequestIndex ] ) {
			FreeClientStatsRequestTask();
		} else {
			assert( clientStatsRequestTask != NULL );

			sdNetTask::taskStatus_e taskStatus = clientStatsRequestTask->GetState();
			if ( taskStatus == sdNetTask::TS_DONE ) {
				FinishClientStatsRequest();
				FreeClientStatsRequestTask();
				SetupFixedClientRank( clientStatsRequestIndex );
			} else {
				return;
			}
		}
	}

	if ( clientStatsRequestIndex == -1 ) {
		clientStatsRequestIndex = 0;
	}
#endif /* !SD_DEMO_BUILD */

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
#if !defined( SD_DEMO_BUILD )
		int clientIndex = ( clientStatsRequestIndex + i ) % MAX_CLIENTS;
#else
		int clientIndex = i;
#endif // SD_DEMO_BUILD
		if ( !clientConnected[ clientIndex ] ) {
			continue;
		}

		if ( clientRanks[ clientIndex ].calculated ) {
			continue;
		}

		if ( gameLocal.GetLocalPlayer() != NULL ) {
			if ( clientIndex == localClientNum ) {
				sdNetUser* activeUser = networkService->GetActiveUser();
				if ( activeUser != NULL ) {
					sdStatsTracker::ReadLocalUserStats( activeUser, clientStatsList[ clientIndex ] );
					CreateClientStatsHash( clientIndex );
				}
			}
			clientRanks[ clientIndex ].calculated = true;
		} else {
#if !defined( SD_DEMO_BUILD )
			if ( StartClientStatsRequest( clientIndex ) ) {
				clientStatsRequestIndex = clientIndex;
				return;
			} else {
				if ( g_debugStatsSetup.GetBool() ) {
					gameLocal.Printf( "Failed To Start Stats Request Task for client %d\n", clientIndex );
				}
				clientRanks[ clientIndex ].calculated = true;
				clientRanks[ clientIndex ].rank = -1;
			}
#else
			clientRanks[ clientIndex ].calculated = true;
#endif /* !SD_DEMO_BUILD */
		}
	}

	clientStatsRequestsPending = false;
#if !defined( SD_DEMO_BUILD )
	clientStatsRequestIndex = -1;
#endif /* !SD_DEMO_BUILD */
}

/*
============
idGameLocal::CreateClientStatsHash
============
*/
void idGameLocal::CreateClientStatsHash( int clientIndex ) {
	sdNetStatKeyValList& statsList = clientStatsList[ clientIndex ];
	idHashIndex& hashIndex = clientStatsHash[ clientIndex ];
	hashIndex.Clear();

	for ( int i = 0; i < statsList.Num(); i++ ) {
		int hashKey = hashIndex.GenerateKey( statsList[ i ].key->c_str(), false );
		hashIndex.Add( hashKey, i );
	}
}

#if !defined( SD_DEMO_BUILD )
/*
============
idGameLocal::FreeClientStatsRequestTask
============
*/
void idGameLocal::FreeClientStatsRequestTask( void ) {
	if ( clientStatsRequestTask == NULL ) {
		return;
	}

	networkService->FreeTask( clientStatsRequestTask );
	clientStatsRequestTask = NULL;
}

/*
============
idGameLocal::FinishClientStatsRequest
============
*/
void idGameLocal::FinishClientStatsRequest( void ) {
	assert( clientStatsRequestTask != NULL && clientStatsRequestIndex != -1 );

	clientRanks[ clientStatsRequestIndex ].calculated = true;

	sdNetErrorCode_e errorCode = clientStatsRequestTask->GetErrorCode();	
	if ( errorCode != SDNET_NO_ERROR ) {
		if ( g_debugStatsSetup.GetBool() ) {
			gameLocal.Printf( "Stats Request Task Failed for client %d\n", clientStatsRequestIndex );
		}
		clientRanks[ clientStatsRequestIndex ].rank = -1;
		return;
	}

	CreateClientStatsHash( clientStatsRequestIndex );

	rankInfo.CreateData( clientStatsHash[ clientStatsRequestIndex ], clientStatsList[ clientStatsRequestIndex ], rankScratchInfo );

	int rank = rankScratchInfo.completeTasks;
	if ( rank >= gameLocal.declRankType.Num() ) {
		rank = gameLocal.declRankType.Num() - 1;
	}
	clientRanks[ clientStatsRequestIndex ].rank = rank;
}

/*
============
idGameLocal::StartClientStatsRequest
============
*/
bool idGameLocal::StartClientStatsRequest( int clientIndex ) {
	sdNetClientId clientId;
	networkSystem->ServerGetClientNetId( clientIndex, clientId );	
	if ( clientId.IsValid() ) {
		clientStatsHash[ clientIndex ].Clear();
		clientStatsRequestTask = networkService->GetStatsManager().ReadDictionary( clientId, clientStatsList[ clientIndex ] );
	}

	return clientStatsRequestTask != NULL;
}

#endif /* !SD_DEMO_BUILD */

/*
============
idGameLocal::GetGlobalStatsValueMax
============
*/
void idGameLocal::GetGlobalStatsValueMax( int clientIndex, const char* name, sdPlayerStatEntry::statValue_t& value ) {
	sdNetStatKeyValList& statsList = clientStatsList[ clientIndex ];
	idHashIndex& hashIndex = clientStatsHash[ clientIndex ];

	int key = hashIndex.GenerateKey( name, false );
	for ( int index = hashIndex.GetFirst( key ); index != hashIndex.NULL_INDEX; index = hashIndex.GetNext( index ) ) {
		if ( idStr::Icmp( statsList[ index ].key->c_str(), name ) != 0 ) {
			continue;
		}

		switch ( statsList[ index ].type ) {
			case sdNetStatKeyValue::SVT_INT:
			case sdNetStatKeyValue::SVT_INT_MAX:
				if ( statsList[ index ].val.i > value.GetInt() ) {
					value = sdPlayerStatEntry::statValue_t( statsList[ index ].val.i );
				}
				break;
			case sdNetStatKeyValue::SVT_FLOAT:
			case sdNetStatKeyValue::SVT_FLOAT_MAX:
				if ( statsList[ index ].val.f > value.GetFloat() ) {
					value = sdPlayerStatEntry::statValue_t( statsList[ index ].val.f );
				}
				break;
		}
	}
}

/*
============
idGameLocal::SetupFixedClientRank
============
*/
void idGameLocal::SetupFixedClientRank( int clientIndex ) {
	if ( gameLocal.isClient ) {
		return;
	}

	idPlayer* player = gameLocal.GetClient( clientIndex );
	if ( player == NULL ) {
		return;
	}

	if ( clientRanks[ clientIndex ].rank == -1 ) {
		return;
	}

	const sdDeclRank* rank = FindRankForLevel( clientRanks[ clientIndex ].rank );
	if ( rank != NULL ) {
		player->GetProficiencyTable().SetFixedRank( rank );
	}
}

/*
============
idGameLocal::FindRankForLevel
============
*/
const sdDeclRank* idGameLocal::FindRankForLevel( int rankLevel ) {
	int highestRankLevel = -1;
	const sdDeclRank* highestRank = NULL;
	const sdDeclRank* rank = NULL;
	for ( int i = 0; i < gameLocal.declRankType.Num(); i++ ) {
		const sdDeclRank* testRank = gameLocal.declRankType[ i ];
		int level = gameLocal.declRankType[ i ]->GetLevel();
		if ( rankLevel == level ) {
			rank = testRank;
			break;
		}

		if ( level > highestRankLevel ) {
			highestRankLevel = level;
			highestRank = testRank;
		}
	}
	if ( rank == NULL ) {
		return highestRank;
	}
	return rank;
}

/*
============
idGameLocal::UpdateLoggedDecals
============
*/
void idGameLocal::UpdateLoggedDecals( void ) {
	for( int i = 0; i < MAX_LOGGED_DECALS; i++ ) {
		if( loggedDecals[ i ] == NULL ) {
			continue;
		}
		gameDecalInfo_t* info = loggedDecals[ i ];
		info->renderEntity.flags.forceUpdate = true;
		gameRenderWorld->UpdateEntityDef( info->renderEntityHandle, &info->renderEntity );
	}

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
void idGameLocal::CalcFov( float base_fov, float &fov_x, float &fov_y, const int width, const int height, const float correctYAspect ) const {
	float	x;
	float	y;
	float	ratio_x;
	float	ratio_y;

	// first, calculate the vertical fov based on a 640x480 view
	x = width / idMath::Tan( base_fov / 360.0f * idMath::PI );
	if ( correctYAspect > 0.f ) {
		x /= correctYAspect;
	}
	y = idMath::ATan( height, x );
	fov_y = y * 360.0f / idMath::PI;

	switch( r_aspectRatio.GetInteger() ) {
	default:
	case 0:
		// 4:3
		fov_x = base_fov;
		return;
	case 1:
		// 16:9
		ratio_x = 16.0f;
		ratio_y = 9.0f;
		break;

	case 2:
		// 16:10
		ratio_x = 16.0f;
		ratio_y = 10.0f;
		break;

	case 3:
		// 5:4
		ratio_x = 5.0f;
		ratio_y = 4.0f;
		break;

	case -1:
		// custom
		ratio_x = r_customAspectRatioH.GetFloat();
		ratio_y = r_customAspectRatioV.GetFloat();
		break;
	}

	y = ratio_y / idMath::Tan( fov_y / 360.0f * idMath::PI );
	if ( correctYAspect > 0.f ) {
		y *= (height/(float)width) * (SCREEN_WIDTH / (float)SCREEN_HEIGHT);
	}
	fov_x = idMath::ATan( ratio_x, y ) * 360.0f / idMath::PI;

	if ( fov_x < base_fov && correctYAspect == 0.0f ) {
		fov_x = base_fov;
		x = ratio_x / idMath::Tan( fov_x / 360.0f * idMath::PI );
		fov_y = idMath::ATan( ratio_y, x ) * 360.0f / idMath::PI;
	}
}

idCVar g_testSpectator( "g_testSpectator", "-1", CVAR_GAME | CVAR_INTEGER, "" );

/*
================
idGameLocal::GetActiveViewer
================
*/
idPlayer* idGameLocal::GetActiveViewer( void ) const {
	sdDemoManagerLocal& manager = sdDemoManager::GetInstance();
	if ( manager.InPlayBack() ) {
		if ( manager.NoClip() ) {
			return NULL;
		}
		if ( g_testSpectator.GetInteger() >= 0 && g_testSpectator.GetInteger() < MAX_CLIENTS ) {
			idPlayer* viewPlayer = GetClient( g_testSpectator.GetInteger() );
			if ( viewPlayer != NULL ) {
				return viewPlayer;
			}
		}
	}

	idPlayer* viewPlayer = NULL;
	if ( gameLocal.serverIsRepeater ) {
		int followClient = GetRepeaterFollowClientIndex();
		if ( followClient != -1 ) {
			viewPlayer = GetClient( followClient );
		}
	} else if ( localClientNum != ASYNC_DEMO_CLIENT_INDEX ) {
		viewPlayer = GetClient( localClientNum );
	}

	if ( viewPlayer != NULL ) {
		if ( viewPlayer->IsSpectating() ) {
			idPlayer* spectator = viewPlayer->GetSpectateClient();
			if ( spectator ) {
				viewPlayer = spectator;
			}
		}
	}

	return viewPlayer;
}

/*
================
idGameLocal::Draw

makes rendering and sound system calls
================
*/
bool idGameLocal::Draw( void ) {
	return gameLocal.playerView.RenderPlayerView( GetActiveViewer() );
}

/*
================
idGameLocal::Draw2D
================
*/
bool idGameLocal::Draw2D( void ) {
	return gameLocal.playerView.RenderPlayerView2D( GetActiveViewer() );
}

/*
===============
idGameLocal::ClientUpdateView
===============
*/
void idGameLocal::ClientUpdateView( const usercmd_t &cmd, int timeLeft ) {
	bool updateSight = false;

	unlock.unlockedDraw = true;

	if ( !unlock.canUnlockFrames ) {
		return;
	}

	idPlayer *p = GetActiveViewer();
	if ( p == NULL ) {
		return;
	}

	if ( p->IsBeingBriefed() ) {
		return;
	}

	// Aspyr's mac code indicated the possibility of interpFraction being 0 causing divide by zero in the angle unlock
	// this will catch it, still I'd like to confirm this can happen at all
	if ( timeLeft == USERCMD_MSEC ) {
		common->Warning( "idGameLocal::ClientUpdateView: timeLeft == USERCMD_MSEC abort" );
		return;
	}

	int effectiveTime = time + ( USERCMD_MSEC - timeLeft );
	gameRenderWorld->DebugClearLines( effectiveTime );
	gameRenderWorld->DebugClearPolygons( effectiveTime );

	// store reference positions and orientations at the beginning of a cycle of unlock frames
	if ( framenum != unlock.lastFullDrawFrame ) {
		unlock.refAngles = p->renderView.viewaxis.ToAngles();

		if ( g_unlock_updateViewpos.GetBool() && g_unlock_viewStyle.GetInteger() == 1 ) {
			// when view is interpolated, grab n - 1 as the reference view origin
			unlock.viewOrigin = unlock.originlog[ ( framenum + 1 ) & 1 ];
		} else {
			unlock.viewOrigin = p->renderView.vieworg;
		}

		if ( p->weapon != NULL ) {
			// NOTE: when interpolating, the origin here is already n-1 due to the adjustment done in RenderPlayerView
			unlock.weaponOrigin = p->weapon->GetRenderEntity()->origin;
			unlock.weaponAxis = p->weapon->GetRenderEntity()->axis;

			// look for a bound GUI and store it's original position
			// only ever expecting one, if there's more the reference position can be stored in sdGuiSurface directly
			bool gotOne = false;
			for ( rvClientEntity* cent = p->weapon->clientEntities.Next(); cent != NULL; cent = cent->bindNode.Next() ) {
				sdGuiSurface* guiSurface = cent->Cast< sdGuiSurface >();
				if ( guiSurface != NULL ) {
					if ( gotOne ) {
						common->Warning( "Multiple GUIs bound to player weapon in ClientUpdateView" );
					}
					guiSurface->GetRenderable().GetPosition( unlock.weaponGUIOrigin, unlock.weaponGUIAxis );
					gotOne = true;	// safe check single GUI
				}
			}	

			unlock.doWeapon = true;
		} else {
			unlock.doWeapon = false;
		}

		unlock.lastFullDrawFrame = framenum;
	}

	float interpFraction = float( USERCMD_MSEC - timeLeft ) / float( USERCMD_MSEC );

	idAngles currentAngles = unlock.refAngles;

	// Gordon: FIXME: This doesn't take all clamps/rate limits into account
	if ( g_unlock_updateAngles.GetBool() && !IsPaused() ) {
		// apply the latest angles to update the view axis
		for ( int i = 0; i < 3; i++ ) {
			currentAngles[i] = unlock.refAngles[i] + SHORT2ANGLE( cmd.angles[i] ) - SHORT2ANGLE( p->usercmd.angles[i] );
		}

		// HACK: let the player clamp the mouse move
		idAngles delta = ( currentAngles - unlock.refAngles ).Normalize180();
		delta /= interpFraction;
		p->MouseMove( unlock.refAngles, delta );
		delta *= interpFraction;
		currentAngles = unlock.refAngles + delta;

		// clamp to the limits
		currentAngles.Normalize180();
		currentAngles.Clamp( unlock.minAngles, unlock.maxAngles );

		p->renderView.viewaxis = currentAngles.ToMat3();
		updateSight = true;
	}


	idVec3 originDelta = vec3_origin;
	if ( g_unlock_updateViewpos.GetBool() ) {
		switch ( g_unlock_viewStyle.GetInteger() ) {
			case 0: {
				// extrapolate last speed, ignoring any newer data
				idVec3 current = unlock.originlog[ framenum & 1 ];
				idVec3 prev = unlock.originlog[ ( framenum + 1 ) & 1 ];
				originDelta = ( current - prev ) * interpFraction;
				p->renderView.vieworg = current + originDelta;
				break;
			}
			case 1: {
				// interpolate between two last positions
				// lags behind of one frame (32ms), but no misprediction hitches
				idVec3 current = unlock.originlog[ framenum & 1 ];
				idVec3 prev = unlock.originlog[ ( framenum + 1 ) & 1 ];
				originDelta = ( current - prev ) * interpFraction;
				p->renderView.vieworg = prev + originDelta;
				break;
			}
		}
		updateSight = true;
	}

	// sight model for the various scopes needs to be updated
	// conveniently uses the same origin as the view, so all computation is already done
	if ( updateSight ) {
		sdClientAnimated *sight = p->GetSight();
		if ( sight != NULL ) {
			float fovx, fovy;
			gameLocal.CalcFov( p->GetSightFOV(), fovx, fovy );
			sight->GetRenderEntity()->weaponDepthHackFOV_x = fovx;
			sight->GetRenderEntity()->weaponDepthHackFOV_y = fovy;

			sight->SetOrigin( p->renderView.vieworg );
			sight->SetAxis( p->renderView.viewaxis );
			sight->Present();
		}
	}

	// update the origin and axis of the weapon - tied to either position or angles update
	if ( unlock.doWeapon && ( g_unlock_updateViewpos.GetBool() || g_unlock_updateAngles.GetBool() ) ) {
		// compute the transformation matrix for the angle offset
		// get (cmd - ucmd) with angle clamping factored in
		idAngles a = currentAngles - unlock.refAngles;
		a.Normalize180();
		// get the transformation in world axis base
		idMat3 world;
		idAngles::YawToMat3( unlock.refAngles.yaw, world );
		idMat3 t = world.Transpose() * a.ToMat3() * world;

		idVec3 updatedOrigin = unlock.weaponOrigin - unlock.viewOrigin;
		updatedOrigin *= t;
		updatedOrigin += unlock.viewOrigin;
		idMat3 updatedAxis = unlock.weaponAxis * t;

		p->weapon->GetRenderEntity()->origin = updatedOrigin + originDelta;
		p->weapon->GetRenderEntity()->axis = updatedAxis;

		idVec3 oldOrigin = p->weapon->GetRenderEntity()->origin;

		p->weapon->BecomeActive( TH_UPDATEVISUALS );
		p->weapon->Present();

		// update bound GUIs as well
		for ( rvClientEntity* cent = p->weapon->clientEntities.Next(); cent != NULL; cent = cent->bindNode.Next() ) {
			sdGuiSurface* guiSurface;
			rvClientEffect* effect;
			if ( ( guiSurface = cent->Cast< sdGuiSurface >() ) != NULL ) {
				// same adjustment work as the weapon model
				updatedOrigin = unlock.weaponGUIOrigin - unlock.viewOrigin;
				updatedOrigin *= t;
				updatedOrigin += unlock.viewOrigin;
				updatedAxis = unlock.weaponGUIAxis * t;

				guiSurface->GetRenderable().SetPosition( updatedOrigin + originDelta, updatedAxis );
			} else if ( ( effect = cent->Cast< rvClientEffect >() ) != NULL ) {
				effect->ClientUpdateView();
			}
		}
 	}

	if ( g_unlock_interpolateMoving.GetBool() ) {
		// walk entities that need interpolation and present them to the renderer with updated origins
		idEntity *e;
		for ( e = interpolateEntities.Next(); e != NULL; e = e->interpolateNode.Next() ) {
			if ( e == p ) { // not on self (which you don't see in 1stp anyway so shouldn't matter)
				continue;
			}
			idVec3 A = e->interpolateHistory[ ( gameLocal.framenum + 1 ) & 1 ];
			idVec3 B = e->interpolateHistory[ gameLocal.framenum & 1 ];
			float ratio = interpFraction;
			e->GetRenderEntity()->origin = ( 1.0f - ratio ) * A + ratio * B;
			e->BecomeActive( TH_UPDATEVISUALS );
			e->Present();
		}
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
void idGameLocal::CallFrameCommand( const sdProgram::sdFunction *frameCommand ) {
	frameCommandThread->CallFunction( frameCommand );
	if ( !frameCommandThread->Execute() ) {
		gameLocal.Error( "idGameLocal::CallFrameCommand '%s' Cannot be Blocking", frameCommand->GetName() );
	}
}

/*
================
idGameLocal::CallFrameCommand
================
*/
void idGameLocal::CallFrameCommand( idScriptObject* object, const sdProgram::sdFunction *frameCommand ) {
	frameCommandThread->CallFunction( object, frameCommand );
	if ( !frameCommandThread->Execute() ) {
		gameLocal.Error( "idGameLocal::CallFrameCommand '%s' Cannot be Blocking", frameCommand->GetName() );
	}
}

/*
================
idGameLocal::CallObjectFrameCommand
================
*/
void idGameLocal::CallObjectFrameCommand( idScriptObject* object, const char *frameCommand, bool allowError ) {
	if ( !object ) {
		return;
	}

	const sdProgram::sdFunction* func = object->GetFunction( frameCommand );

	if ( !func ) {
		if ( allowError ) {
			Error( "Unknown function '%s' called for frame command on entity '%s'", frameCommand, object->GetTypeName() );
		}
	} else {
		frameCommandThread->CallFunction( object, func );
		if ( !frameCommandThread->Execute() ) {
			gameLocal.Error( "idGameLocal::CallFrameCommand '%s' Cannot be Blocking", frameCommand );
		}
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

idCVar g_showEntityInfoPrint( "g_showEntityInfoPrint", "0", CVAR_BOOL, "" );

/*
================
idGameLocal::RunDebugInfo
================
*/
void idGameLocal::RunDebugInfo( void ) {
	idEntity *ent;

	idPlayer* player = GetLocalPlayer();
	if ( !player ) {
		return;
	}

	renderView_t* view = &player->renderView;

	const idVec3& origin = view->vieworg;

	if ( g_showEntityInfo.GetBool() ) {
		idMat3		axis = player->viewAngles.ToMat3();
		idVec3		up = axis[ 2 ] * 5.0f;
		idBounds	viewTextBounds( origin );
		idBounds	viewBounds( origin );

		viewTextBounds.ExpandSelf( 128.0f );
		viewBounds.ExpandSelf( g_maxShowDistance.GetFloat() );
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
			if ( contents & CONTENTS_BODY || contents & CONTENTS_SLIDEMOVER ) {
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
				gameRenderWorld->DrawText( ent->name.c_str(), entBounds.GetCenter() - up, 0.1f, colorWhite, axis, 1 );
				gameRenderWorld->DrawText( ent->GetType()->classname, entBounds.GetCenter(), 0.1f, colorWhite, axis, 1 );
				gameRenderWorld->DrawText( va( "#%d", ent->entityNumber ), entBounds.GetCenter() + up, 0.1f, colorWhite, axis, 1 );
			}

			if ( g_showEntityInfoPrint.GetBool() ) {
				gameLocal.Printf( "%s\n", ent->name.c_str() );
			}
		}

		g_showEntityInfoPrint.SetBool( false );
	}

	// debug tool to draw bounding boxes around active entities
	if ( g_showActiveEntities.GetBool() ) {
		for( ent = activeEntities.Next(); ent != NULL; ent = ent->activeNode.Next() ) {
			idBounds	b = ent->GetPhysics()->GetBounds();
			if ( b.GetVolume() <= 0 ) {
				b[0][0] = b[0][1] = b[0][2] = -8;
				b[1][0] = b[1][1] = b[1][2] = 8;
			}
			gameRenderWorld->DebugBounds( colorGreen, b, ent->GetPhysics()->GetOrigin(), ent->GetPhysics()->GetAxis() );
		}
	}

	if ( g_showTargets.GetBool() ) {
		ShowTargets();
	}

	if ( g_showTriggers.GetBool() ) {
		idTrigger::DrawDebugInfo();
	}

	if ( g_editEntityMode.GetBool() && editEntities ) {
		editEntities->DisplayEntities();
	}

	if ( g_showCollisionWorld.GetInteger() ) {
		gameLocal.clip.DrawWorld( g_maxShowDistance.GetFloat() );
	}

	if ( g_showCollisionModels.GetBool() ) {
		clip.DrawClipModels( player->renderView.vieworg, player->renderView.viewaxis, g_maxShowDistance.GetFloat(), pm_thirdPerson.GetBool() ? NULL : player );
	}

	if ( g_showRenderModelBounds.GetBool() ) {
		for ( idEntity* ent = gameLocal.spawnedEntities.Next(); ent; ent = ent->spawnNode.Next() ) {
			const renderEntity_t* renderEnt = ent->GetRenderEntity();
			if ( !renderEnt ) {
				continue;
			}

			gameRenderWorld->DebugBounds( colorRed, renderEnt->bounds, renderEnt->origin, renderEnt->axis );
		}
	}

	if ( g_showClipSectors.GetBool() ) {
		clip.DrawClipSectors();
	}

	if ( g_showAreaClipSectors.GetFloat() ) {
		clip.DrawAreaClipSectors( g_showAreaClipSectors.GetFloat() );
	}

	if ( g_showCollisionTraces.GetBool() ) {
		clip.PrintStatistics();
	}

	if ( g_showPVS.GetInteger() ) {
		pvs.DrawPVS( origin, ( g_showPVS.GetInteger() == 2 ) ? PVS_ALL_PORTALS_OPEN : PVS_NORMAL );
	}

	if ( *g_debugMask.GetString() ) {
		qhandle_t handle = gameLocal.GetDeploymentMask( g_debugMask.GetString() );
		if ( handle != -1 ) {
			DebugDeploymentMask( handle );
		}
	}

	if ( g_showActiveDeployZones.GetBool() ) {
		sdInstanceCollector< sdDeployZone > deployZones( true );
		for ( int i = 0; i < deployZones.Num(); i++ ) {
			idPhysics* physics = deployZones[ i ]->GetPhysics();
			if ( deployZones[ i ]->IsActive() ) {
				physics->GetClipModel()->Draw( physics->GetOrigin(), physics->GetAxis() );
			}
		}
	}

	if ( g_debugLocations.GetBool() ) {
		sdLocationMarker::DebugDraw( player->renderView.vieworg );
	}

	// collision map debug output
	collisionModelManager->DebugOutput( player->renderView.vieworg, player->renderView.viewaxis );

	botThreadData.DebugOutput();
}

/*
==================
idGameLocal::CheatsOk
==================
*/
bool idGameLocal::CheatsOk( bool requirePlayer ) {
	idPlayer *player;

	if ( networkSystem->IsActive() ) {
		if ( !cvarSystem->GetCVarBool( "net_allowCheats" ) ) {
			return false;
		}
	} else {
		if ( !IsDeveloper() ) {
			return false;
		}
	}

	player = GetLocalPlayer();
	if ( !requirePlayer || ( player && ( player->GetHealth() > 0 ) ) ) {
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

	if ( !spawnArgs.GetInt( "spawn_entnum", "0", spawn_entnum ) ) {
		while( entities[firstFreeIndex] && firstFreeIndex < ENTITYNUM_MAX_NORMAL ) {
			firstFreeIndex++;
		}
		if ( firstFreeIndex >= ENTITYNUM_MAX_NORMAL ) {
			Error( "no free entities" );
		}
		spawn_entnum = firstFreeIndex++;
	}

	if ( spawn_entnum >= MAX_GENTITIES ) {
		Error( "idGameLocal::RegisterEntity: spawn count overflow" );
	}

	if ( entities[ spawn_entnum ] != NULL ) {
		Printf( "Index: %d\n", spawn_entnum );
		Printf( "Old Entity: '%s'\n", entities[ spawn_entnum ]->GetName() );
		Printf( "New Entity: '%s'\n", ent->GetName() );
		Error( "idGameLocal::RegisterEntity: entity spawned in existing entity slot" );
	}

	entities[ spawn_entnum ] = ent;
	spawnIds[ spawn_entnum ] = spawnCount++;
	ent->entityNumber = spawn_entnum;

	int id = GetSpawnId( ent );
	if( id == NETWORKEVENT_RULES_ID || id == NETWORKEVENT_OBJECTIVE_ID ) {
		Error( "Spawn ID equal to event magic number!" );
	}

	ent->spawnNode.AddToEnd( spawnedEntities );
	ent->spawnArgs.TransferKeyValues( spawnArgs );

	const idKeyValue* kv = NULL;
	while ( kv = ent->spawnArgs.MatchPrefix( "collection", kv ) ) {
		const char* collectionName = kv->GetValue();
		sdEntityCollection* collection = GetEntityCollection( collectionName, true );
		collection->Add( ent );
	}

	if ( spawn_entnum != ENTITYNUM_WORLD && spawn_entnum > numEntities ) {
		numEntities = spawn_entnum;
	}

	ent->ForceNetworkUpdate();

	entityDebugInfo[ spawn_entnum ].OnCreate( ent );
}

/*
===================
idGameLocal::ForceNetworkUpdate
===================
*/
void idGameLocal::ForceNetworkUpdate( idEntity *ent ) {
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		clientNetworkInfo[ i ].lastMarker[ ent->entityNumber ] = -1;
	}
	demoClientNetworkInfo.lastMarker[ ent->entityNumber ] = -1;

#ifdef SD_SUPPORT_REPEATER
	repeaterClientNetworkInfo.lastMarker[ ent->entityNumber ] = -1;
	for ( int i = 0; i < repeaterNetworkInfo.Num(); i++ ) {
		if ( repeaterNetworkInfo[ i ] == NULL ) {
			continue;
		}
		( *repeaterNetworkInfo[ i ] ).lastMarker[ ent->entityNumber ] = -1;
	}
#endif // SD_SUPPORT_REPEATER
}

/*
===================
idGameLocal::UnregisterEntity
===================
*/
void idGameLocal::UnregisterEntity( idEntity *ent ) {
	assert( ent );

	entityDebugInfo[ ent->entityNumber ].OnDestroy( ent );

#ifdef _DEBUG
	sdCommandMapInfoManager::GetInstance().OnEntityDeleted( ent );
#endif // _DEBUG

	FreeEntityNetworkEvents( ent, -1 );

	if ( editEntities ) {
		editEntities->RemoveSelectedEntity( ent );
	}

	for ( int i = 0; i < entityCollections.Num(); i++ ) {
		entityCollections[ i ]->Remove( ent );
	}

	if ( ( ent->entityNumber != ENTITYNUM_NONE ) && ( entities[ ent->entityNumber ] == ent ) ) {
		ent->spawnNode.Remove();
		ent->networkNode.Remove();
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
idEntity *idGameLocal::SpawnEntityType( const idTypeInfo &classdef, bool callPostMapSpawn, const idDict *args ) {
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
		obj->CallSpawn();
	}

	catch( idAllocError & ) {
		obj = NULL;
	}

	idEntity* ent = obj->Cast< idEntity >();
	if ( ent ) {
		if ( ent->StartSynced() ) {
			ent->SetNetworkSynced( true );
		}

		if ( callPostMapSpawn ) {
			ent->PostMapSpawn();
		}
	}

	spawnArgs.Clear();

	return ent;
}

/*
================
idGameLocal::CallSpawnFuncs
================
*/
void idGameLocal::CallSpawnFuncs( idEntity* entity, const idDict *args ) {
	if ( args ) {
		spawnArgs = *args;
	} else {
		spawnArgs.Clear();
	}
	entity->CallSpawn();
	spawnArgs.Clear();

	if ( entity->StartSynced() ) {
		entity->SetNetworkSynced( true );
	}
}

/*
================
idGameLocal::CreateEntityType
================
*/
idEntity *idGameLocal::CreateEntityType( const idTypeInfo &classdef ) {
	if ( !classdef.IsType( idEntity::Type ) ) {
		Error( "Attempted to spawn non-entity class '%s'", classdef.classname );
	}

	try {
		return reinterpret_cast< idEntity* >( classdef.CreateInstance() );
	}
	catch( idAllocError & ) {
		return NULL;
	}
}

/*
===================
idGameLocal::FindEntityDefDict
===================
*/
const idDict* idGameLocal::FindEntityDefDict( const char *name, bool makeDefault ) const {
	const idDeclEntityDef* def = declHolder.declEntityDefType.LocalFind( name, makeDefault );
	if ( !def ) {
		return NULL;
	}
	return &def->dict;
}

/*
===================
idGameLocal::SpawnEntityDef

Finds the spawn function for the entity and calls it,
returning false if not found
===================
*/
bool idGameLocal::SpawnEntityDef( const idDict &args, bool callPostMapSpawn, idEntity **ent, int entityNum, int mapSpawnId ) {
	if ( ent ) {
		*ent = NULL;
	}

	spawnArgs = args;

	idStr error;
	const char* name;
	if ( spawnArgs.GetString( "name", "", &name ) ) {
		sprintf( error, " on '%s'", name );
	}

	const char* classname = spawnArgs.GetString( "classname" );

	const idDeclEntityDef *def = declHolder.declEntityDefType.LocalFind( classname, false );
	if ( !def ) {
		Warning( "Unknown classname '%s'%s.", classname, error.c_str() );
		return false;
	}

	spawnArgs.SetDefaults( &def->dict );
	if ( entityNum != -1 ) {
		if ( InhibitEntitySpawn( spawnArgs ) ) {
			return false;
		}

		spawnArgs.SetInt( "spawn_entnum", entityNum );

		// precache any media specified in the map entity
		CacheDictionaryMedia( spawnArgs );
	}

	// check if we should spawn a class object
	const char* spawn = spawnArgs.GetString( "spawnclass" );
	if ( !*spawn ) {
		Warning( "%s doesn't include a spawnclass %s.", classname, error.c_str() );
		return false;
	}

	idTypeInfo* cls = idClass::GetClass( spawn );
	if ( cls == NULL ) {
		Warning( "Could not spawn '%s'.  Class '%s' not found%s.", classname, spawn, error.c_str() );
		return false;
	}

	if ( cls->InhibitSpawn( args ) ) {
		return false;
	}

	idClass* obj = cls->CreateInstance();
	if ( obj == NULL ) {
		Warning( "Could not spawn '%s'. Instance could not be created%s.", classname, error.c_str() );
		return false;
	}

	// setting the mapSpawnId on the dict so it is available in the Spawn function
	if ( mapSpawnId != -1 ) {
		spawnArgs.SetInt( "spawn_mapSpawnId", mapSpawnId );
	}

	obj->CallSpawn();

	idEntity* e = obj->Cast< idEntity >();
	if ( ent != NULL ) {
		*ent = e;
	}

	if ( e != NULL ) {
		e->mapSpawnId = mapSpawnId;

		if ( e->StartSynced() ) {
			e->SetNetworkSynced( true );
		}

		if ( callPostMapSpawn ) {
			e->PostMapSpawn();
		}
	}

	return true;
}

/*
===================
idGameLocal::SpawnClientEntityDef

Finds the spawn function for the entity and calls it,
returning false if not found
===================
*/
bool idGameLocal::SpawnClientEntityDef( const idDict &args, rvClientEntity **ent, int mapSpawnId ) {
	if ( !gameLocal.DoClientSideStuff() ) {
		return false;
	}

	if ( ent ) {
		*ent = NULL;
	}

	spawnArgs = args;

	idStr error;
	const char* name;
	if ( spawnArgs.GetString( "name", "", &name ) ) {
		sprintf( error, " on '%s'", name);
	}

	const char* classname = spawnArgs.GetString( "classname" );

	const idDeclEntityDef *def = declHolder.declEntityDefType.LocalFind( classname, false );
	if ( !def ) {
		Warning( "Unknown classname '%s'%s.", classname, error.c_str() );
		return false;
	}

	spawnArgs.SetDefaults( &def->dict );
	if ( mapSpawnId != -1 ) {
		if ( InhibitEntitySpawn( spawnArgs ) ) {
			return false;
		}

		spawnArgs.SetInt( "spawn_entnum", mapSpawnId );

		// precache any media specified in the map entity
		CacheDictionaryMedia( spawnArgs );
	}

	// check if we should spawn a class object
	const char* spawn = spawnArgs.GetString( "spawnclass" );
	if ( !*spawn ) {
		Warning( "%s doesn't include a spawnclass %s.", classname, error.c_str() );
		return false;
	}

	idTypeInfo* cls = idClass::GetClass( spawn );
	if ( !cls ) {
		Warning( "Could not spawn '%s'.  Class '%s' not found%s.", classname, spawn, error.c_str() );
		return false;
	}

	if ( cls->InhibitSpawn( args ) ) {
		return false;
	}

	idClass* obj = cls->CreateInstance();
	if ( !obj ) {
		Warning( "Could not spawn '%s'. Instance could not be created%s.", classname, error.c_str() );
		return false;
	}

	// setting the mapSpawnId on the dict so it is available in the Spawn function
	if ( mapSpawnId != -1 ) {
		spawnArgs.SetInt( "spawn_mapSpawnId", mapSpawnId );
	}

	obj->CallSpawn();

	rvClientEntity* e = obj->Cast< rvClientEntity >();
	if ( ent ) {
		*ent = e;
	}

	// If it's scripted make sure we also initialize the script object...
	// this is a bit backward but it's because the basic rvClientEntity doesn't have spawnargs
	sdClientScriptEntity *sc = obj->Cast< sdClientScriptEntity >();
	if ( sc != NULL ) {
		const char* scriptClass = spawnArgs.GetString( "scriptobject" );
		if ( !*scriptClass ) {
			Warning( "Could not spawn '%s'. No script object specified.", classname );
			return false;
		}
		sc->CreateByName( &spawnArgs, scriptClass );
	}

	return true;
}

/*
================
idGameLocal::InhibitEntitySpawn
================
*/
bool idGameLocal::InhibitEntitySpawn( idDict &spawnArgs ) {
	// allow game rules to inhibit an entity's spawn
	sdGameRules* tempRules = GetRulesInstance( serverInfo.GetString( "si_rules" ) );
	if ( tempRules->InhibitEntitySpawn( spawnArgs ) ) {
		return true;
	}

	return false;
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
	Printf( "Spawning entities\n" );

	if ( mapFile == NULL ) {
		Printf( "No mapfile present\n" );
		return;
	}

	int numMapEntities = mapFile->GetNumEntities();
	if ( numMapEntities == 0 ) {
		Error( "...no entities" );
	}

	int num = 0;
	int inhibit = 0;

	insideExecuteMapChange = true;

	bool showUpdate = !networkSystem->IsDedicated();
	idWStrList locArgs( 1 );
	locArgs.SetNum( 1 );

	locArgs[ 0 ] = L"0";
	UpdateLevelLoadScreen( common->LocalizeText( "guis/mainmenu/loading/spawning_entities", locArgs ).c_str() );

	for ( int i = 0; i < numMapEntities; i++ ) {
		idMapEntity* mapEnt = mapFile->GetEntity( i );

		int entNum = i == 0 ? ENTITYNUM_WORLD : ( MAX_CLIENTS + i - 1 );

		idEntity* ent;
		if ( SpawnEntityDef( mapEnt->epairs, false, &ent, entNum, i ) ) {
			num++;
		} else {
			if ( i == 0 ) {
				insideExecuteMapChange = false;
				Error( "Problem spawning world entity" );
			}
			inhibit++;
		}
		if( showUpdate && i % 25 == 0 ) {
			locArgs[ 0 ] = va( L"%0.f", ( static_cast< float >( i ) / numMapEntities ) * 100.0f );
			
			UpdateLevelLoadScreen( common->LocalizeText( "guis/mainmenu/loading/spawning_entities", locArgs ).c_str() );
			common->PacifierUpdate();
		}
	}
	// lock at 100%
	locArgs[ 0 ] = L"100";
	UpdateLevelLoadScreen( common->LocalizeText( "guis/mainmenu/loading/spawning_entities", locArgs ).c_str() );

	insideExecuteMapChange = false;

	for ( idEntity* ent = spawnedEntities.Next(); ent; ent = ent->spawnNode.Next() ) {
		ent->PostMapSpawn();
	}

	if ( gameLocal.isClient ) {
		// Remove any synced entities, as we won't get deletion events for ones from the map
		idEntity* nextNetEnt;
		for ( idEntity* netEnt = networkedEntities.Next(); netEnt; netEnt = nextNetEnt ) {
			nextNetEnt = netEnt->networkNode.Next();
			delete netEnt;
		}
	}

	idDict args;
	args.SetInt( "spawn_entnum", ENTITYNUM_CLIENT );
	if ( !SpawnEntityType( rvClientPhysics::Type, false, &args ) || !entities[ ENTITYNUM_CLIENT ] ) {
		Error( "Problem spawning client physics entity" );
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
	for ( i = entityHash.GetFirst( hash ); i != -1; i = entityHash.GetNext( i ) ) {
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

	refLength = idStr::Length( ref );
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
	if ( trace.fraction == 1.0f ) {
		return NULL;
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

	for( i = 0; i < gameLocal.numEntities; i++ ) {
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
	for ( i = entityHash.GetFirst( hash ); i != -1; i = entityHash.GetNext( i ) ) {
		if ( entities[i] && entities[i]->name.Icmp( name ) == 0 ) {
			return entities[i];
		}
	}

	return NULL;
}

/*
=============
idGameLocal::FindClassTypeT
=============
*/
template< typename T >
T *idGameLocal::FindClassTypeT( idEntity *from ) const {
	idEntity	*ent;

	if ( !from ) {
		ent = spawnedEntities.Next();
	} else {
		ent = from->spawnNode.Next();
	}

	for ( ; ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( ent->IsType( T::Type ) ) {
			return reinterpret_cast< T* >( ent );
		}
	}

	return NULL;
}

/*
=============
idGameLocal::FindEntityByType
=============
*/
idEntity *idGameLocal::FindEntityByType( idEntity *from, const idDeclEntityDef* type ) const {
	idEntity* ent = from ? from->spawnNode.Next() : spawnedEntities.Next();

	for ( ; ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( ent->entityDefNumber == type->Index() ) {
			return ent;
		}
	}

	return NULL;
}

/*
=============
idGameLocal::FindClassType
=============
*/
idEntity *idGameLocal::FindClassType( idEntity *from, const idTypeInfo& type, idEntity *ignore ) const {
	idEntity* ent = from ? from->spawnNode.Next() : spawnedEntities.Next();

	for ( ; ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( ent->IsType( type ) && ent != ignore ) {
			return ent;
		}
	}

	return NULL;
}

/*
=============
idGameLocal::FindClassTypeReverse
=============
*/
idEntity *idGameLocal::FindClassTypeReverse( idEntity *from, const idTypeInfo& type, idEntity *ignore ) const {
	idEntity* ent;

	if ( from ) {
		ent = from->spawnNode.Prev();
	} else {
		idEntity* lastEnt;

		lastEnt = ent = spawnedEntities.Next();
		for ( ; ent != NULL; ent = ent->spawnNode.Next() ) {
			lastEnt = ent;
		}
		ent = lastEnt;
	}

	for ( ; ent != NULL; ent = ent->spawnNode.Prev() ) {
		if ( ent->IsType( type ) && ent != ignore ) {
			return ent;
		}
	}

	return NULL;
}

/*
=============
idGameLocal::FindClassTypeInRadius
=============
*/
idEntity *idGameLocal::FindClassTypeInRadius( const idVec3& org, float radius, idEntity *from, const idTypeInfo& type, idEntity *ignore ) const {
	idEntity	*ent;

	if ( !from ) {
		ent = spawnedEntities.Next();
	} else {
		ent = from->spawnNode.Next();
	}

	float rSqr = Square( radius );
	idVec3 dist;
	for ( ; ent != NULL; ent = ent->spawnNode.Next() ) {
		assert( ent );
		if ( ent->IsType( type ) && ent != ignore ) {
			idPhysics* physics = ent->GetPhysics();
			if( !physics ) {
				continue;
			}

			dist = physics->GetOrigin() - org;
			if( dist.LengthSqr() > rSqr ) {
				continue;
			}

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
			entityList[ entCount++ ] = ent;
			if( entCount >= maxCount ) {
				return entCount;
			}
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
void idGameLocal::KillBox( idEntity *ent ) {
	int			i;
	int			num;
	idEntity*	hit;
	const idClipModel* cm;
	const idClipModel* clipModels[ MAX_GENTITIES ];
	idPhysics*	phys;

	phys = ent->GetPhysics();
	if ( !phys->GetNumClipModels() ) {
		return;
	}

	num = clip.ClipModelsTouchingBounds( CLIP_DEBUG_PARMS phys->GetAbsBounds(), phys->GetClipMask(), clipModels, MAX_GENTITIES, ent );
	for ( i = 0; i < num; i++ ) {
		cm = clipModels[ i ];

		// don't check render entities
		if ( cm->IsRenderModel() ) {
			continue;
		}

		hit = cm->GetEntity();
		if ( !hit->fl.takedamage ) {
			continue;
		}

		if ( !phys->ClipContents( cm ) ) {
			continue;
		}

		// nail it
		hit->Damage( ent, ent, vec3_origin, DAMAGE_FOR_NAME( "damage_telefrag" ), 1.0f, NULL );
	}
}

/*
============
idGameLocal::RadiusDamage
============
*/
void idGameLocal::RadiusDamage( const idVec3 &origin, idEntity *inflictor, idEntity *attacker, idEntity *ignoreDamage, idEntity *ignorePush, const sdDeclDamage* damageDecl, float dmgPower, float radiusScale ) {
	if ( damageDecl == NULL ) {
		gameLocal.Error( "idGameLocal::RadiusDamage NULL Damage" );
	}

	float radius	= Max( 1.f, damageDecl->GetRadius() * radiusScale );

	if ( !gameLocal.isClient ) {
		float		dist, damageScale;
		idEntity*	ent;
		idEntity*	entityList[ MAX_GENTITIES ];
		idBounds	entityBounds[ MAX_GENTITIES ];
		int			numListedEntities;
		idBounds	bounds;
		idVec3 		v, damagePoint, dir;
		int			i;

		assert( damageDecl );

		if ( !damageDecl ) {
			Warning( "idGameLocal::RadiusDamage NULL damage parm" );
			return;
		}

		bounds = idBounds( origin ).Expand( radius );

		// get all entities touching the bounds
		numListedEntities = clip.EntitiesTouchingBounds( bounds, -1, entityList, MAX_GENTITIES );

		// Gordon: Store the bounds as they were when we started, as the damage could cause shifts, due to players being knocked out of vehicles
		for ( int e = 0; e < numListedEntities; e++ ) {			
			entityBounds[ e ] = entityList[ e ]->GetPhysics()->GetAbsBounds();
		}

		for ( int pass = 0; pass < 2; pass++ ) {
			// apply damage to the entities
			for ( int e = 0; e < numListedEntities; e++ ) {
				ent = entityList[ e ];
				assert( ent );

				if ( pass == 0 ) {
					if ( ent->IsType( idPlayer::Type ) ) {
						continue;
					}
				} else {
					if ( !ent->IsType( idPlayer::Type ) ) {
						continue;
					}
				}

				if ( !ent->fl.takedamage || ent == ignoreDamage ) {
					continue;
				}

				const idBounds& absBounds = entityBounds[ e ];

				// find the distance from the edge of the bounding box
				for ( i = 0; i < 3; i++ ) {
					if ( origin[ i ] < absBounds[ 0 ][ i ] ) {
						v[ i ] = absBounds[ 0 ][ i ] - origin[ i ];
					} else if ( origin[ i ] > absBounds[ 1 ][ i ] ) {
						v[ i ] = origin[ i ] - absBounds[ 1 ][ i ];
					} else {
						v[ i ] = 0;
					}
				}

				dist = v.Length();
				if ( dist >= radius ) {
					continue;
				}

				trace_t tr;
				if ( damageDecl->GetNoTrace() || ent->CanDamage( origin, damagePoint, MASK_EXPLOSIONSOLID, ignoreDamage, &tr ) ) {
					// get the damage scale
					damageScale = dmgPower * ( 1.0f - dist / radius );
					dir = ent->GetPhysics()->GetOrigin() - inflictor->GetPhysics()->GetOrigin();
					dir.Normalize();
					ent->Damage( inflictor, attacker, dir, damageDecl, damageScale, &tr );
				}
			}
		}
	}

	// push physics objects
	RadiusPush( origin, radius, damageDecl, dmgPower, attacker, ignorePush, 0, true );
}


/*
==============
idGameLocal::RadiusPush
==============
*/
void idGameLocal::RadiusPush( const idVec3 &origin, float radius, const sdDeclDamage* damageDecl, float pushScale, const idEntity *inflictor, const idEntity *ignore, int flags, bool saveEvent ) {
	if ( isClient && saveEvent ) {
		new sdPhysicsEvent_RadiusPush( physicsEvents, origin, radius, damageDecl, pushScale, inflictor, ignore, flags );
	}

	idEntity* entityList[ MAX_GENTITIES ];

	idVec3 dir;
	dir.Set( 0.0f, 0.0f, 1.0f );

	idBounds bounds = idBounds( origin ).Expand( radius );

	// get all the entities touching the bounds
	int numListedEntities = clip.EntitiesTouchingBounds( bounds, -1, entityList, MAX_GENTITIES );

	// apply impact to all the entities
	for ( int i = 0; i < numListedEntities; i++ ) {
		idEntity* ent = entityList[ i ];
		if ( ent == ignore ) {
			continue;
		}

		if ( !ent->DoRadiusPush() ) {
			continue;
		}

		if ( flags & RP_GROUNDONLY ) {
			if ( !ent->GetPhysics()->HasGroundContacts() ) {
				continue;
			}
		}

		modelTrace_t result;
		idVec3 otherOrigin = ent->GetPhysics()->GetAbsBounds().GetCenter();
		if ( gameRenderWorld->FastWorldTrace( result, origin, otherOrigin ) ) {
			continue;
		}

		ent->ApplyRadiusPush( origin, otherOrigin, damageDecl, pushScale, radius );
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
		clipModel->GetEntity()->AddForce( world, clipModel->GetId(), clipModel->GetOrigin(), push * impulse );
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

		clipModel->GetEntity()->AddForce( world, clipModel->GetId(), center, impulse );
	}
}

/*
==============
idGameLocal::RadiusPushEntities
==============
*/
void idGameLocal::RadiusPushEntities( const idVec3& origin, float force, float radius ) {
	float rSquared = Square( radius );

	const int MAX_PUSH_ENTS = 32;
	idEntity* ents[ MAX_PUSH_ENTS ];

	int count = EntitiesWithinRadius( origin, radius, ents, MAX_PUSH_ENTS );
	int i;
	for( i = 0; i < count; i++ ) {
		idVec3 dist = origin - ents[ i ]->GetPhysics()->GetOrigin();

		float len = dist.LengthSqr();
		if( len > rSquared ) {
			continue;
		}

		len = dist.Normalize();
		len = len / radius;
		len *= len;

		idVec3 f = dist * force * len;

		ents[ i ]->GetPhysics()->AddForce( 0, origin, f );
	}
}

/*
===============
idGameLocal::ProjectDecal
===============
*/
void idGameLocal::ProjectDecal( const idVec3 &origin, const idVec3 &dir, float depth, bool parallel, float size, const idMaterial *material, float angle ) {
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
	idMath::SinCos16( DEG2RAD( angle ), s, c );

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
	gameRenderWorld->ProjectDecalOntoWorld( winding, projectionOrigin, parallel, depth * 0.5f, material, time, time );
}

/*
===============
idGameLocal::CreateProjectedDecal
===============
*/
void idGameLocal::CreateProjectedDecal( const idVec3 &origin, const idVec3 &dir, float depth, bool parallel, float width, float height, float angle, const idVec4& color, idRenderModel* model ) {
	if ( networkSystem->IsDedicated() ) {
		return;
	}

	float s, c;
	idMat3 axis, axistemp;
	idFixedWinding winding;
	idVec3 windingOrigin, projectionOrigin;

	if ( width <= 0.f || height <= 0.f ) {
		Warning( "idGameLocal::CreateProjectedDecal Invalid Size %f x %f", width, height );
		return;
	}

	width *= 0.5f;
	height *= 0.5f;
	idVec3 decalWinding[4] = {
		idVec3(  width,  height, 0.0f ),
		idVec3( -width,  height, 0.0f ),
		idVec3( -width, -height, 0.0f ),
		idVec3(  width, -height, 0.0f )
	};

	// randomly rotate the decal winding
	idMath::SinCos16( DEG2RAD( angle ), s, c );

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

	winding += idVec5( windingOrigin + ( axis * decalWinding[0] ), idVec2( 1, 1 ) );
	winding += idVec5( windingOrigin + ( axis * decalWinding[1] ), idVec2( 0, 1 ) );
	winding += idVec5( windingOrigin + ( axis * decalWinding[2] ), idVec2( 0, 0 ) );
	winding += idVec5( windingOrigin + ( axis * decalWinding[3] ), idVec2( 1, 0 ) );

	int spawnID = WORLD_SPAWN_ID;
	gameRenderWorld->AddToProjectedDecal( winding, projectionOrigin, parallel, color, model, spawnID );
}

/*
=============
idGameLocal::SetCamera
=============
*/
void idGameLocal::SetCamera( idCamera *cam ) {
	camera = cam;
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
======================
idGameLocal::StartViewEffect

For passing in an effect triggered view effect
======================
*/
void idGameLocal::StartViewEffect( int type, float time, float scale ) {
	switch( type ) {
	case VIEWEFFECT_SHAKE:
		gameLocal.playerView.SetShakeParms( time, scale );
		break;

	case VIEWEFFECT_TUNNEL:
		gameLocal.playerView.SetTunnelParms( time, scale );
		break;

	default:
		gameLocal.Warning( "Invalid view effect" );
		break;
	}
}

/*
======================
idGameLocal::GetPlayerView
======================
*/
void idGameLocal::GetPlayerView( idVec3& origin, idMat3& axis, float& fovx ) {
	idPlayer* player = gameLocal.GetActiveViewer();
	if ( player ) {
		renderView_t* view = player->GetRenderView();
		if ( view ) {
			origin	= view->vieworg;
			axis	= view->viewaxis;
			fovx	= view->fov_x;
			return;
		}
	}

	origin	= vec3_origin;
	axis	= mat3_identity;
	fovx	= 90.f;
}

/*
======================
idGameLocal::Translation

small portion of physics required for the effects system
======================
*/
void idGameLocal::Translation( trace_t &trace, const idVec3 &source, const idVec3 &dest, const idTraceModel &trm, int clipMask ) {
	idClipModel *cm = new idClipModel( trm, false );
	clip.Translation( CLIP_DEBUG_PARMS trace, source, dest, cm, mat3_identity, clipMask, NULL );
	gameLocal.clip.DeleteClipModel( cm );
}

/*
==============
idGameLocal::TracePoint
==============
*/
void idGameLocal::TracePoint( trace_t& trace, const idVec3 &source, const idVec3 &dest, int clipMask ) {
	clip.TracePoint( CLIP_DEBUG_PARMS trace, source, dest, clipMask, NULL );
}


/*
======================
idGameLocal::SpawnClientMoveable
======================
*/
rvClientMoveable* idGameLocal::SpawnClientMoveable ( const char* name, int lifetime, const idVec3& origin, const idMat3& axis, const idVec3& velocity, const idVec3& angular_velocity, int effectSet ) {
	if ( !gameLocal.DoClientSideStuff() ) {
		return NULL;
	}

	// Ensure client moveables never last forever
	if ( lifetime <= 0 ) {
		lifetime = 2500;
	}

	// find the debris def
	const idDict& args = *gameLocal.FindEntityDefDict( name, false );
	if ( &args == NULL ) {
		return NULL;
	}

	// Spawn the debris
	rvClientMoveable* cent = new rvClientMoveable;
	cent->SetOrigin( origin );
	cent->SetAxis( axis );
	cent->Spawn( &args, effectSet );

	renderEntity_t* renderEntity = cent->GetRenderEntity();
	assert( renderEntity != NULL );

	if ( gameLocal.mapSkinPool != NULL ) {
		const char* skinKey = args.GetString( "climate_skin_key" );
		if ( *skinKey != '\0' ) {
			const char* skinName = gameLocal.mapSkinPool->GetDict().GetString( va( "skin_%s", skinKey ) );
			if ( *skinName == '\0' ) {
				gameLocal.Warning( "idGameLocal::SpawnClientMoveable No Skin Set For '%s'", skinKey );
			} else {
				const idDeclSkin* skin = gameLocal.declSkinType[ skinName ];
				if ( skin == NULL ) {
					gameLocal.Warning( "idGameLocal::SpawnClientMoveable Skin '%s' Not Found", skinName );
				} else {
					renderEntity->customSkin = skin;
				}
			}
		}
	}

	cent->GetPhysics()->SetLinearVelocity ( velocity );
	cent->GetPhysics()->SetAngularVelocity ( angular_velocity );

	cent->PostEventMS( &CL_FadeOut, lifetime, 2500 );

	return cent;
}

/*
============
idGameLocal::SetPortalState
============
*/
void idGameLocal::SetPortalState( qhandle_t portal, int blockingBits ) {
	gameRenderWorld->SetPortalState( portal, blockingBits );
}

/*
===========
idGameLocal::SortSpawnsByAge
============
*/
int idGameLocal::SortSpawnsByAge( const void* a, const void* b ) {
	const sdSpawnPoint* spotA = *( const sdSpawnPoint** )( a );
	const sdSpawnPoint* spotB = *( const sdSpawnPoint** )( b );

	if ( spotA->GetLastUsedTime() > spotB->GetLastUsedTime() ) {
		return 1.f;
	}

	if ( spotB->GetLastUsedTime() > spotA->GetLastUsedTime() ) {
		return -1.f;
	}

	return 0.f;
}

/*
===========
idGameLocal::SelectInitialSpawnPointForRepeaterClient
============
*/
bool idGameLocal::SelectInitialSpawnPointForRepeaterClient( idVec3& outputOrg, idAngles& outputAngles ) {
	for ( int pass = 0; pass < 2; pass++ ) {
		for ( int i = 0; i < spawnSpots.Num(); i++ ) {
			const sdSpawnPoint& tempSpot = spawnSpots[ i ];

			idEntity* other = tempSpot.GetOwner();
			if ( pass == 0 ) {
				if ( other != NULL ) {
					sdRequirementContainer* requirements = other->GetSpawnRequirements();
					if ( requirements && requirements->HasRequirements() ) {
						continue;
					}
				}
			}

			if ( tempSpot.GetRelativePositioning() ) {
				if ( other != NULL ) {
					outputOrg	= other->GetPhysics()->GetOrigin() + ( other->GetPhysics()->GetAxis() * tempSpot.GetOffset() );
				} else {
					continue;
				}
			} else {
				outputOrg	= tempSpot.GetOffset();
			}

			// move up to make sure the player is at least an epsilon above the floor
			outputOrg[ 2 ] += 4.0f + CM_BOX_EPSILON;

			outputAngles	= tempSpot.GetAngles();
			return true;
		}
	}

	return false;
}

/*
===========
idGameLocal::SelectInitialSpawnPoint
============
*/
const sdSpawnPoint* idGameLocal::SelectInitialSpawnPoint( idPlayer *player, idVec3& outputOrg, idAngles& outputAngles ) {
	assert( !gameLocal.isClient );

	const int		MAX_AVAILABLE_SPAWN_POINTS = 256;

	const sdSpawnPoint* spots[ MAX_AVAILABLE_SPAWN_POINTS ];
	const sdSpawnPoint* blockedSpots[ MAX_AVAILABLE_SPAWN_POINTS ];
	int				availableSpots = 0;
	int				numBlockedSpots = 0;

	if ( player->IsType( idBot::Type ) ) {
		botThreadData.CheckBotSpawnLocation( player );
	}

	idEntity* chosenSpawnPoint = player->GetSpawnPoint();
	idEntity* teamSpawnPoint = NULL;

	sdInstanceCollector< idPlayer > players( true );
	sdInstanceCollector< sdTransport > transports( true );

	sdTeamInfo* team = player->GetGameTeam();
	if ( team != NULL ) {
		teamSpawnPoint = team->GetDefaultSpawn();
	}
	if ( chosenSpawnPoint == NULL ) {
		chosenSpawnPoint = teamSpawnPoint;
	}

	while ( true ) {
		for( int i = 0; i < spawnSpots.Num() && availableSpots < MAX_AVAILABLE_SPAWN_POINTS; i++ ) {
			const sdSpawnPoint& tempSpot = spawnSpots[ i ];

			if ( !tempSpot.InUse() ) {
				continue;
			}

			idEntity* other = tempSpot.GetOwner();

			if ( chosenSpawnPoint && chosenSpawnPoint != other ) {
				continue;
			}

			if ( other ) {
				sdRequirementContainer* requirements = other->GetSpawnRequirements();
				if ( requirements && !requirements->Check( player, other ) ) {
					continue;
				}
			}

			if ( !tempSpot.GetRequirements().Check( player ) ) {
				continue;
			}

			// check for obstructions
			if ( players.Num() > 0 ) {
				// calculate an approximate bounds for this spawn point based on the bounds of the first player
				// figure out origin first
				idEntity* owner = tempSpot.GetOwner();

				idVec3 origin;
				if ( owner && tempSpot.GetRelativePositioning() ) {
					origin = owner->GetPhysics()->GetOrigin() + ( owner->GetPhysics()->GetAxis() * tempSpot.GetOffset() );
				} else {
					origin = tempSpot.GetOffset();
				}

				idBounds playerBounds;
				idPhysics_Player::CalcNormalBounds( playerBounds );

				idBox spawnBox( playerBounds, origin, mat3_identity );

				bool blocked = false;
				// check for players
				// NOTE: could just check if the origin of the spawn is within the bounds of the other
				//		 for a faster & less accurate check. Probably accurate enough though.
				int j;
				for ( j = 0; j < players.Num(); j++ ) {
					idPlayer* player = players[ j ];

					idBox playerBox( player->GetPhysics()->GetBounds(), player->GetPhysics()->GetOrigin(), mat3_identity );
					if ( playerBox.IntersectsBox( spawnBox ) ) {
						blocked = true;
						break;
					}
				}

				// if someone is standing on our spawn point, knock them out the way
				sdDynamicSpawnPoint* spawnPoint = owner->Cast< sdDynamicSpawnPoint >();
				if ( blocked && spawnPoint != NULL ) {
					idPlayer* blockingPlayer = players[ j ];

					idVec3 forward = blockingPlayer->GetViewAngles().ToForward();
					forward.z = 0.f;
					forward.NormalizeFast();
					blockingPlayer->ApplyImpulse( gameLocal.world, -1, blockingPlayer->GetPhysics()->GetOrigin(), forward * 51200.f );

					blocked = false;
				}

				if ( !blocked ) {
					// check for transports
					for ( int j = 0; j < transports.Num(); j++ ) {
						sdTransport* transport = transports[ j ];

						idBox vehicleBox( transport->GetPhysics()->GetBounds(), transport->GetPhysics()->GetOrigin(), transport->GetPhysics()->GetAxis() );
						if ( vehicleBox.IntersectsBox( spawnBox ) ) {
							blocked = true;
							break;
						}
					}
				}

				if ( blocked ) {
					blockedSpots[ numBlockedSpots++ ] = &tempSpot;
					continue;
				}
			}

			spots[ availableSpots++ ] = &tempSpot;
		}

		if ( availableSpots == 0 ) {
			if ( chosenSpawnPoint != NULL && chosenSpawnPoint != teamSpawnPoint ) {
				chosenSpawnPoint = teamSpawnPoint;
				continue;
			}
		}

		break;
	}

	if ( availableSpots == 0 && numBlockedSpots == 0 ) {
		outputOrg.Zero();
		outputAngles.Zero();
		//Warning( "No valid idPlayerStart on map." );
		return NULL;
	} else if ( availableSpots == 0 ) {
		// the only available spots are blocked
		// copy them across into the available spots - OUT OF DESPERATION
		for ( int i = 0; i < numBlockedSpots; i++ ) {
			spots[ i ] = blockedSpots [ i ];
		}
		availableSpots = numBlockedSpots;
	}

	qsort( spots, availableSpots, sizeof( spots[ 0 ] ), SortSpawnsByAge );

	// New (hopefully improved) spawn point selection code
	const sdSpawnPoint* spot = NULL;
	if ( availableSpots > 2 ) {
		// use a "roulette wheel" selection method to find the next spawn point used
		// ie - weight the selection more heavily in favour of spawn points not used
		//		recently
		//		instead of randomly selecting from the least recent half, which could
		//		potentially cause telefrags if more than half are used in rapid
		//		succession
		int totalLastUsedTime = 0;
		const unsigned int halfSpots = availableSpots / 2;
		for ( unsigned int i = 0; i < halfSpots; i++ ) {
			totalLastUsedTime += spots[ i ]->GetLastUsedTime();
		}

		// now calculate the total inverse used time
		float totalInverseUsedTime = (float) ( ( halfSpots - 1 ) * totalLastUsedTime );

		// so that there are no divide by zeros
		if ( totalInverseUsedTime == 0.0f ) {
			totalInverseUsedTime = 1.0f;
		}

		// calculate the selection value
		// note this uses a power scale to shift the distribution towards the higher values
		const float randomSelection = idMath::Pow( random.RandomFloat(), 0.4f );

		// find the slice of the wheel that this fits into
		float cutoffUpto = 0.0f;
		for ( unsigned int i = halfSpots - 1; i >= 0; i-- ) {
			// calculate the cutoff value corresponding to this slice
			const float lastUsed = (float) ( totalLastUsedTime - spots[ i ]->GetLastUsedTime() );
			const float cutoff = lastUsed / totalInverseUsedTime + cutoffUpto;
			cutoffUpto = cutoff;

			// if the selection is greater than the last cutoff and less than this one
			// then it fits into this slice of the wheel
			if ( randomSelection < cutoff || i == 0 ) {
				spot = spots[i];
				break;
			}
		}
	} else {
		// two or less spots - the first spot will always be the best option
		spot = spots[ 0 ];
	}

	idEntity* owner = spot->GetOwner();
	if ( owner && spot->GetRelativePositioning() ) {
		outputOrg	= owner->GetPhysics()->GetOrigin() + ( owner->GetPhysics()->GetAxis() * spot->GetOffset() );
	} else {
		outputOrg	= spot->GetOffset();
	}

	// move up to make sure the player is at least an epsilon above the floor
	outputOrg[ 2 ] += 4.0f + CM_BOX_EPSILON;

	outputAngles	= spot->GetAngles();

	return spot;
}

/*
================
idGameLocal::UpdateServerInfoFlags
================
*/
void idGameLocal::UpdateServerInfoFlags( void ) {
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
idGameLocal::UsercommandCallback
================
*/
void idGameLocal::UsercommandCallback( usercmd_t& cmd ) {
	idPlayer* player = GetLocalPlayer();
	if ( !player ) {
		return;
	}

	player->UsercommandCallback( cmd );
}

idCVar g_nextMap( "g_nextMap", "", CVAR_GAME | CVAR_NOCHEAT, "commands to execute when the current map/campaign ends" );
/*
================
idGameLocal::NextMap
================
*/
bool idGameLocal::NextMap( void ) {
	idStr text = g_nextMap.GetString();
	g_nextMap.SetString( "" );
	int currentMapLoadCount = mapLoadCount;
	cmdSystem->BufferCommandText( CMD_EXEC_NOW, text.c_str() );
	return currentMapLoadCount != mapLoadCount;
}

/*
================
idGameLocal::BeginLevelLoad
================
*/
void idGameLocal::BeginLevelLoad() {
	reloadingSameMap = false;
	//sdDynamicBlockManagerBase::CompactPools();

	uiManager->Clear();
	uiManager->BeginLevelLoad();
}

/*
================
idGameLocal::EndLevelLoad
================
*/
void idGameLocal::EndLevelLoad() {
//	SetBytesNeededForMapLoad( GetMapName(), fileSystem->GetReadCount() );

	reloadingSameMap = true;
	uiManager->EndLevelLoad();
	UpdateLevelLoadScreen( L"" );
}

/*
================
idGameLocal::GetEffectHandle

Get the handle of the effect with the given name
================
*/
idCVar bse_projectileEffect( "bse_projectileEffect", "", CVAR_CHEAT, "this effect will replace projectile explosions" );

int idGameLocal::GetEffectHandle ( const idDict& args, const char* effectName, const char* materialType ) {
	//jrad - this is the least-nasty way to get the "gun fires effects" for development of particles
	if( bse_projectileEffect.GetString()[ 0 ] && !idStr::Icmp( bse_projectileEffect.GetString(), effectName )) {
		return( declHolder.declEffectsType.LocalFind( effectName )->Index() );
	}

	sdStringBuilder_Heap builder;
	builder = "effectchance ";
	builder += effectName;
	float chance = args.GetFloat ( builder.c_str(), "1" );
	if ( random.RandomFloat () > chance ) {
		return -1;
	}

	if( !effectName || !effectName[ 0 ] ) {
		return -1;
	}

	// we should ALWAYS be playing effects from the def.
	// hard-coded effects MUST be avoided at all times because they won't get pre-cached.
	if( idStr::Icmpn( effectName, "fx", 2 ) ) {
		Error( "Effects must be played via def file keys starting with 'fx'" );
		return -1;
	}

	if ( materialType && *materialType ) {
		sdStringBuilder_Heap		temp;
		const char*	result;

		temp = effectName;
		temp += "_";
		temp += materialType;

		// See if the given material effect is specified
		result = args.GetString( temp.c_str() );


		if ( result && *result ) {
			if ( useSimpleEffect ) {
				sdStringBuilder_Heap builder;
				builder = result;
				builder += "_simple";
				const rvDeclEffect *effect = declHolder.declEffectsType.LocalFind( builder.c_str(), false );
				if ( effect ) {
					return effect->Index();
				}
			}
			return( declHolder.declEffectsType.LocalFind( result )->Index() );
		}
	}

	// grab the non material effect name
	effectName = args.GetString( effectName );
	if ( !*effectName ) {
		return -1;
	}

	if ( useSimpleEffect ) {
		sdStringBuilder_Heap builder;
		builder = effectName;
		builder += "_simple";
		const rvDeclEffect *effect = declHolder.declEffectsType.LocalFind( builder.c_str(), false );
		if ( effect ) {
			return effect->Index();
		}
	}
	return( declHolder.declEffectsType.LocalFind( effectName )->Index() );
}

int idGameLocal::GetDecal ( const idDict& args, const char* decalName, const char* materialType ) {

	// we should ALWAYS be playing effects from the def.
	// hard-coded effects MUST be avoided at all times because they won't get pre-cached.
	if( idStr::Icmpn( decalName, "dec_", 4 ) ) {
		Error( "Decals must be played via def file keys starting with 'dec_'" );
		return -1;
	}

	if ( materialType && *materialType ) {
		sdStringBuilder_Heap		temp;
		const char*	result;

		temp = decalName;
		temp += "_";
		temp += materialType;

		// See if the given material effect is specified
		result = args.GetString( temp.c_str() );
		if ( result && *result ) {
			if ( *result == '_' ) {
				return -1;
			} else {
				return( declHolder.FindDecal( result )->Index() );
			}
		}
	}

	// grab the non material effect name
	decalName = args.GetString( decalName );
	if ( !*decalName ) {
		return -1;
	}

	return( declHolder.FindDecal( decalName )->Index() );
}


/*
================
idGameLocal::FindEffect
================
*/
const rvDeclEffect *idGameLocal::FindEffect( const char* name, bool makeDefault ) {
	if ( useSimpleEffect ) {
		sdStringBuilder_Heap builder;
		builder = name;
		builder += "_simple";
		const rvDeclEffect *effect = declHolder.declEffectsType.LocalFind( builder.c_str(), false );
		if ( effect != NULL ) {
			return effect;
		}
	}
	return declHolder.FindEffect( name, makeDefault );
}

/*
================
idGameLocal::PlayEffect

Plays an effect at the given origin using the given direction
================
*/
rvClientEffect* idGameLocal::PlayEffect( const int effectHandle, const idVec3& color, const idVec3& origin, const idMat3& axis, bool loop, const idVec3& endOrigin, float distanceOffset ) {
	if ( !gameLocal.DoClientSideStuff() ) {
		return NULL;
	}

	if ( !effectHandle ) {
		return NULL;
	}

	rvClientEffect* effect = new rvClientEffect( effectHandle );
	effect->SetOrigin( origin );
	effect->SetAxis( axis );
	effect->SetGravity( GetGravity() );
	effect->SetMaterialColor( color );
	effect->SetDistanceOffset( distanceOffset );
	if ( !effect->Play( gameLocal.time, loop, endOrigin ) ) {
		delete effect;
		return NULL;
	}

	return effect;
}

/*
===================
idGameLocal::RegisterClientEntity
===================
*/
void idGameLocal::RegisterClientEntity( rvClientEntity *cent ) {
	int entityNumber;

	sdScopedLock<true> scoped( clientEntLock );

	assert ( cent );

	// Find a free entity index to use
	while( clientEntities[firstFreeClientIndex] && firstFreeClientIndex < MAX_CENTITIES ) {
		firstFreeClientIndex++;
	}
	if( firstFreeClientIndex >= MAX_CENTITIES ) {
		for (int i=0; i<MAX_CENTITIES; i++) {
			const char *name = gameLocal.clientEntities[i]->GetName();
			common->Printf( "%d '%s'\n", i, name ? name : "<unknown>" );
		}
		cent->PostEventMS ( &EV_Remove, 0 );
		Error( "idGameLocal::RegisterClientEntity: no free client entities" );
		return;
	}

	entityNumber = firstFreeClientIndex++;

	// Add the client entity to the lists
	clientEntities[ entityNumber ] = cent;
	assert( clientSpawnIds[ entityNumber ] < 0 );
	int nextID = abs(clientSpawnIds[ entityNumber ]) + 1;
	if ( nextID >= ( 1 << ( 32 - CENTITYNUM_BITS ) ) ) {
		nextID = INITIAL_SPAWN_COUNT;
	}
	clientSpawnIds[ entityNumber ] = nextID;
	cent->entityNumber = entityNumber;
	cent->spawnNode.AddToEnd( clientSpawnedEntities );

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

	sdScopedLock<true> scoped( clientEntLock );

	// No entity number then it failed to register
	if( cent->entityNumber == -1 ) {
		return;
	}

	cent->spawnNode.Remove();
	cent->bindNode.Remove();

	if( cent->entityNumber != -1 && clientEntities [ cent->entityNumber ] == cent ) {
		cent->spawnNode.Remove();
		cent->bindNode.Remove();
		clientEntities[ cent->entityNumber ] = NULL;
		assert( clientSpawnIds[ cent->entityNumber ] > 0 );
		clientSpawnIds[ cent->entityNumber ] = -abs(clientSpawnIds[ cent->entityNumber ]);
		if( cent->entityNumber < firstFreeClientIndex ) {
			firstFreeClientIndex = cent->entityNumber;
		}
		cent->entityNumber = -1;
	}
}

/*
===================
idGameLocal::HandleNetworkMessage
===================
*/
void idGameLocal::HandleNetworkMessage( idPlayer* player, idEntity* entity, const char* message ) {
	if ( entity == NULL ) {
		gameLocal.Warning( "idGameLocal::HandleNetworkMessage entity is NULL" );
		return;
	}

	sdNetworkInterface* iface = entity->GetNetworkInterface();
	if ( !iface ) {
		return;
	}

	iface->HandleNetworkMessage( player, message );
}

/*
===================
idGameLocal::ParseClamp
===================
*/
void idGameLocal::ParseClamp( angleClamp_t& clamp, const char* prefix, const idDict& dict ) {
	clamp.flags.enabled = false;

	clamp.flags.enabled |= dict.GetFloat( va( "%s_min", prefix ), "0", clamp.extents[ 0 ] );
	clamp.flags.enabled |= dict.GetFloat( va( "%s_max", prefix ), "0", clamp.extents[ 1 ] );

	clamp.flags.limitRate = dict.GetFloat( va( "%s_rate", prefix ), "0", clamp.rate[ 0 ] );
	clamp.rate[ 1 ] = clamp.rate[ 0 ];
}

/*
===================
idGameLocal::HandleNetworkEvent
===================
*/
void idGameLocal::HandleNetworkEvent( idEntity* entity, const char* message ) {
	if ( !entity ) {
		return;
	}

	sdNetworkInterface* iface = entity->GetNetworkInterface();
	if ( !iface ) {
		return;
	}

	iface->HandleNetworkEvent( message );
}

/*
===================
idGameLocal::HandleGuiScriptMessage
===================
*/
void idGameLocal::HandleGuiScriptMessage( idPlayer* player, idEntity* entity, const char* message ) {
	if ( !entity ) {
		return;
	}

	sdGuiInterface* iface = entity->GetGuiInterface();
	if ( !iface ) {
		return;
	}

	iface->HandleGuiScriptMessage( player, message );
}

/*
============
idGameLocal::SurfaceTypePostParse
============
*/
void idGameLocal::SurfaceTypePostParse( idDecl* decl ) {
	sdDeclSurfaceType* surfaceDecl = static_cast< sdDeclSurfaceType* >( decl );
	assert( decl );
	int index = decl->Index();
	if( index >= surfaceTypes.Num() ) {
		surfaceTypes.AssureSize( index + 1 );
	}

	assert( index < surfaceTypes.Num() );
	surfaceTypes[ index ].friction = surfaceDecl->GetProperties().GetFloat( "friction", "1.0" );
}

/*
============
idGameLocal::GetSurfaceTypeForIndex
============
*/
surfaceProperties_t& idGameLocal::GetSurfaceTypeForIndex( int index ) {
	assert( index < surfaceTypes.Num() );
	return surfaceTypes[ index ];
}

/*
============
idGameLocal::SavePlayZoneMasks
============
*/
void idGameLocal::SavePlayZoneMasks( void ) {
	for ( int i = 0; i < playZones.Num(); i++ ) {
		playZones[ i ]->SaveMasks();
	}
}

/*
============
idGameLocal::ClearPlayZones
============
*/
void idGameLocal::ClearPlayZones( void ) {
	playZones.DeleteContents( true );
	worldPlayZones.Clear();
	choosablePlayZones.Clear();
}

/*
============
idGameLocal::ClearDeployRequests
============
*/
void idGameLocal::ClearDeployRequests( void ) {
	int i;
	for ( i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !deployRequests[ i ] ) {
			continue;
		}

		if ( !gameLocal.isClient ) {
			deployRequests[ i ]->WriteDestroyEvent( i );
		}
		delete deployRequests[ i ];
		deployRequests[ i ] = NULL;
	}
}

/*
============
idGameLocal::ClearDeployRequest
============
*/
void idGameLocal::ClearDeployRequest( int deployIndex ) {
	if ( deployIndex < 0 || deployIndex > MAX_CLIENTS ) {
		gameLocal.Warning( "idGameLocal::ClearDeployRequest Index out of Bounds '%i'", deployIndex );
		return;
	}

	if ( deployRequests[ deployIndex ] ) {
		if ( !gameLocal.isClient ) {
			deployRequests[ deployIndex ]->WriteDestroyEvent( deployIndex );
		}
		delete deployRequests[ deployIndex ];
		deployRequests[ deployIndex ] = NULL;
	}
}

/*
============
idGameLocal::GetDeploymentMask
============
*/
qhandle_t idGameLocal::GetDeploymentMask( const char* name ) {
	for ( int i = 0; i < deploymentMasks.Num(); i++ ) {
		if ( !deploymentMasks[ i ].Icmp( name ) ) {
			return i;
		}
	}

	qhandle_t handle = deploymentMasks.Num();
	deploymentMasks.Alloc() = name;

	return handle;
}

/*
============
idGameLocal::CreatePlayZone
============
*/
void idGameLocal::CreatePlayZone( const idDict& info, const idBounds& bounds ) {
	sdPlayZone* pz = new sdPlayZone( info, bounds );
	playZones.Alloc() = pz;
	if ( pz->GetFlags() & sdPlayZone::PZF_WORLD ) {
		worldPlayZones.Alloc() = pz;
	}

	if ( pz->GetFlags() & sdPlayZone::PZF_CHOOSABLE ) {
		choosablePlayZones.Alloc() = pz;
	}
}

/*
============
idGameLocal::SetPlayZoneAreaName
============
*/
void idGameLocal::SetPlayZoneAreaName( int index, const char* name ) {
	playZoneAreaNames[ index ] = name;
}

/*
============
idGameLocal::LinkPlayZoneAreas
============
*/
void idGameLocal::LinkPlayZoneAreas( void ) {
	for ( int i = 0; i < playZoneAreas.Num(); i++ ) {
		for ( int j = 0; j < playZones.Num(); j++ ) {
			if ( !*playZones[ j ]->GetTarget() ) {
				continue;
			}

			if ( playZoneAreaNames[ i ].Icmp( playZones[ j ]->GetTarget() ) ) {
				continue;
			}

			if ( playZoneAreas[ i ] == NULL ) {
				playZoneAreas[ i ] = new idList< sdPlayZone* >();
			}

			playZoneAreas[ i ]->Alloc() = playZones[ j ];
			break;
		}
	}
}

/*
============
idGameLocal::GetWorldPlayZoneIndex
============
*/
int idGameLocal::GetWorldPlayZoneIndex( const idVec3& point ) const {
	idVec2 point2d = point.ToVec2();

	int bestPlayZone = -1;

	for ( int i = 0; i < worldPlayZones.Num(); i++ ) {
		const sdPlayZone* playZone = worldPlayZones[ i ];

		if ( ( playZone->GetFlags() & sdPlayZone::PZF_WORLD ) == 0 ) {
			continue;
		}

		const sdBounds2D& bounds = playZone->GetBounds();
		if ( !bounds.ContainsPoint( point2d ) ) {
			continue;
		}

		bestPlayZone = i;
	}

	return bestPlayZone;
}

/*
============
idGameLocal::GetPlayZone
============
*/
const sdPlayZone* idGameLocal::GetPlayZone( const idVec3& point, int flags ) const {
	int areaNum = gameRenderWorld->PointInArea( point );
	if ( areaNum != -1 ) {
		if ( playZoneAreas[ areaNum ] != NULL ) {
			const idList< sdPlayZone* >& list = *playZoneAreas[ areaNum ];
			for ( int i = 0; i < list.Num(); i++ ) {
				if ( list[ i ]->GetFlags() & flags ) {
					return list[ i ];
				}
			}
		}
	}

	idVec2 point2d = point.ToVec2();

	int bestPlayZonePriority		= -1;
	const sdPlayZone* bestPlayZone	= NULL;

	for ( int i = 0; i < playZones.Num(); i++ ) {
		const sdPlayZone* playZone = playZones[ i ];

		if ( ( playZone->GetFlags() & flags ) == 0 ) {
			continue;
		}

		if ( playZone->GetPriority() < bestPlayZonePriority ) {
			continue;
		}

		const sdBounds2D& bounds = playZone->GetBounds();

		if ( !bounds.ContainsPoint( point2d ) ) {
			continue;
		}

		bestPlayZonePriority	= playZone->GetPriority();
		bestPlayZone			= playZone;
	}

	return bestPlayZone;
}

/*
============
idGameLocal::GetChoosablePlayZone
============
*/
const sdPlayZone* idGameLocal::GetChoosablePlayZone( int id ) const {
	if( id < 0 || id >= choosablePlayZones.Num() ) {
		return NULL;
	}
	return choosablePlayZones[ id ];
}

/*
============
idGameLocal::GetIndexForChoosablePlayZone
============
*/
int idGameLocal::GetIndexForChoosablePlayZone( const sdPlayZone* zone ) const {
	for( int i = 0; i < choosablePlayZones.Num(); i++ ) {
		if( choosablePlayZones[ i ] == zone ) {
			return i;
		}
	}
	return -1;
}

/*
============
idGameLocal::GetNumChoosablePlayZones
============
*/
int idGameLocal::GetNumChoosablePlayZones() const {
	return choosablePlayZones.Num();
}


/*
=========================
idGameLocal::ClosestPlayZone
=========================
*/
const sdPlayZone* idGameLocal::ClosestPlayZone( const idVec3& point, float& dist, int flags ) const {
	const idVec2 point2d = point.ToVec2();
	const sdPlayZone* bestPlayZone = NULL;
	float bestDist = -1.f;

	for ( int i = 0; i < playZones.Num(); i++ ) {
		sdPlayZone* pz = playZones[ i ];
		if ( ( pz->GetFlags() & flags ) == 0 ) {
			continue;
		}

		const sdBounds2D& bounds = pz->GetBounds();

		int sides = bounds.SideForPoint( point2d, sdBounds2D::SPACE_INCREASE_FROM_TOP );
		if ( sides == sdBounds2D::SIDE_INTERIOR ) {
			dist = 0.f;
			return pz;
		}

		float a = 0.f;
		if ( sides & sdBounds2D::SIDE_LEFT ) {
			a = bounds.GetLeft() - point.x;
		} else if ( sides & sdBounds2D::SIDE_RIGHT ) {
			a = point.x - bounds.GetRight();
		}

		float b = 0.f;
		if ( sides & sdBounds2D::SIDE_TOP ) {
			b = bounds.GetTop() - point.y;
		} else if ( sides & sdBounds2D::SIDE_BOTTOM ) {
			b = point.y - bounds.GetBottom();
		}

		float d = ( a * a ) + ( b * b );

		if ( bestDist < 0.f || d < bestDist ) {
			bestDist = d;
			bestPlayZone = pz;
		}
	}

	if ( bestDist == -1.f ) {
		dist = 0.f;
		return NULL;
	}

	dist = idMath::Sqrt( bestDist );
	return bestPlayZone;
}

/*
============
idGameLocal::DebugDeploymentMask
============
*/
void idGameLocal::DebugDeploymentMask( qhandle_t handle ) {
	for ( int i = 0; i < playZones.Num(); i++ ) {
		const sdDeployMaskInstance* mask = playZones[ i ]->GetMask( handle );
		if ( mask == NULL ) {
			continue;
		}

		mask->DebugDraw();
	}
}

/*
============
idGameLocal::RegisterTargetEntity
============
*/
void idGameLocal::RegisterTargetEntity( idLinkList< idEntity >& node ) {
	node.AddToEnd( targetEntities );
}

/*
============
idGameLocal::RegisterIconEntity
============
*/
void idGameLocal::RegisterIconEntity( idLinkList< idEntity >& node ) {
	node.AddToEnd( iconEntities );
}

/*
============
idGameLocal::RegisterSpawnPoint
============
*/
sdSpawnPoint& idGameLocal::RegisterSpawnPoint( idEntity* owner, const idVec3& offset, const idAngles& angles ) {
	sdSpawnPoint* spot = NULL;

	// see if theres a spawn point object available to reuse
	for ( int i = 0; i < spawnSpots.Num(); i++ ) {
		sdSpawnPoint& tempSpot = spawnSpots[ i ];
		if ( tempSpot.InUse() ) {
			continue;
		}

		spot = &tempSpot;
	}

	if ( !spot ) {
		// failed to reuse one, try to alloc a new one
		spot = spawnSpots.Alloc();
	}

	if ( !spot ) {
		Error( "idGameLocal::RegisterSpawnPoint No Free Spawn Points" );
	}

	// make sure everything from the last map is cleared out
	spot->Clear();

	spot->SetOwner( owner );
	if ( owner != NULL ) {
		spot->SetRelativePositioning( true );
	}
	spot->SetPosition( offset, angles );
	spot->SetInUse( true );

	return *spot;
}

/*
============
idGameLocal::UnRegisterSpawnPoint
============
*/
void idGameLocal::UnRegisterSpawnPoint( sdSpawnPoint* point ) {
	point->Clear();
}

/*
============
idGameLocal::CheckDeploymentRequestBlock
============
*/
deployResult_t idGameLocal::CheckDeploymentRequestBlock( const idBounds& bounds ) {
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !deployRequests[ i ] ) {
			continue;
		}

		if ( deployRequests[ i ]->CheckBlock( bounds ) ) {
			return DR_WARNING;
		}
	}
	return DR_CLEAR;
}


/*
============
idGameLocal::GetDeploymentRequest
============
*/
sdDeployRequest* idGameLocal::GetDeploymentRequest( idPlayer* player ) {
	if ( !player ) {
		return NULL;
	}

	return deployRequests[ player->entityNumber ];
}

/*
============
idGameLocal::RequestDeployment
============
*/
bool idGameLocal::RequestDeployment( idPlayer* player, const sdDeclDeployableObject* object, const idVec3& position, float rotation, int delayMS ) {
	if ( gameLocal.isClient ) {
		return false;
	}

	if ( gameLocal.rules->IsEndGame() ) {
		return false;
	}

	if ( !player || !object ) {
		return false;
	}

	if ( deployRequests[ player->entityNumber ] ) {
		return false;
	}

	// trace down to find the real position
	idVec3 start = position;

	// estimate where to start using the heightmap (if it exists)
	const sdPlayZone* playZoneHeight = gameLocal.GetPlayZone( start, sdPlayZone::PZF_HEIGHTMAP );
	if ( playZoneHeight != NULL ) {
		const sdHeightMapInstance& heightMap = playZoneHeight->GetHeightMap();
		if ( heightMap.IsValid() ) {
			start.z = heightMap.GetInterpolatedHeight( start );
		}
	}

	if ( start.z < position.z ) {
		start.z = position.z;
	}

	// trace down to the ground
	start.z += 64.0f;
	idVec3 end = start - idVec3( 0.0f, 0.0f, 2048.0f );
	trace_t trace;
	clip.TracePoint( CLIP_DEBUG_PARMS trace, start, end, CONTENTS_SOLID | CONTENTS_VEHICLECLIP | CONTENTS_BODY | CONTENTS_MONSTER, player );
	
	// if the endpos is significantly higher then the request position then 
	// it may be that the requester did it from under cover
	if ( trace.endpos.z > position.z + 64.0f ) {
		trace.endpos = position;
	}

	sdDeployRequest* request = new sdDeployRequest( object, player, trace.endpos, rotation, player->GetGameTeam(), delayMS );

	deployRequests[ player->entityNumber ] = request;
	request->WriteCreateEvent( player->entityNumber, sdReliableMessageClientInfoAll() );

	return true;
}

/*
============
idGameLocal::UpdateDeploymentRequests
============
*/
void idGameLocal::UpdateDeploymentRequests( void ) {
	idPlayer* player = gameLocal.GetLocalViewPlayer();

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		if ( !deployRequests[ i ] ) {
			continue;
		}
		if ( !deployRequests[ i ]->Update( player ) ) {
			ClearDeployRequest( i );
			continue;
		}
	}
}

/*
============
idGameLocal::RunFrame
============
*/
void idGameLocal::RunFrame() {
#ifdef _XENON
	liveManager->RunFrame();
#else
	
	// system notifications are in a static list that's reset to num 0 every frame
	// this ensures that the pointers are valid when the notifications are picked up below during sdnet.RunFrame
	sdnet.RunFrame();
	systemNotifications.SetNum( 0 );
#endif
}

#if 0
/*
===============
idGameLocal::GetBytesNeededForMapLoad
===============
*/
int idGameLocal::GetBytesNeededForMapLoad( const char* map ) {

	sdDeclStringMap* mapDef = const_cast< sdDeclStringMap* >( declMapMetaDataType.LocalFind( map ));

	int retVal = mapDef->GetDict().GetInt( va("size%d", cvarSystem->GetCVarInteger( "com_machineSpec" )));
	if( !retVal ) {
		if ( cvarSystem->GetCVarInteger( "com_machineSpec" ) < 2 ) { return 200 * 1024 * 1024; } else { return 400 * 1024 * 1024; }
	}

	return retVal;
}

/*
===============
idGameLocal::SetBytesNeededForMapLoad
===============
*/
void idGameLocal::SetBytesNeededForMapLoad( const char* map, int bytesNeeded ) {
	if( !cvarSystem->GetCVarBool( "com_updateLoadSize" )) {
		return;
	}

	sdDeclStringMap* mapDef = const_cast< sdDeclStringMap* >( declMapMetaDataType.LocalFind( GetMapName() ));

	if ( mapDef ) {
		mapDef->GetDict().SetInt( va("size%d", cvarSystem->GetCVarInteger( "com_machineSpec" )), bytesNeeded );
		mapDef->Save();
	}
}
#endif

/*
============
idGameLocal::GetEntityCollection
============
*/
sdEntityCollection* idGameLocal::GetEntityCollection( const char* name, bool allowCreate ) {
	int key = entityCollectionsHash.GenerateKey( name, false );
	for ( int index = entityCollectionsHash.GetFirst( key ); index != idHashIndex::NULL_INDEX; index = entityCollectionsHash.GetNext( index ) ) {
		if ( idStr::Icmp( entityCollections[ index ]->GetName(), name ) ) {
			continue;
		}
		return entityCollections[ index ];
	}

	if ( allowCreate ) {
		sdEntityCollection* collection = new sdEntityCollection();
		collection->SetName( name );
		int index = entityCollections.Num();
		entityCollectionsHash.Add( key, index );
		entityCollections.Alloc() = collection;
		return collection;
	}
	return NULL;
}

/*
================
idGameLocal::SetTargetTimer
================
*/
void idGameLocal::SetTargetTimer( qhandle_t handle, idPlayer* player, int t ) {
	if ( handle < 0 || handle >= targetTimers.Num() ) {
		gameLocal.Error( "idGameLocal::SetTargetTimer Invalid handle %d", handle );
	}
	targetTimers[ handle ].endTimes[ player->entityNumber ] = t;
}

/*
================
idGameLocal::AllocTargetTimer
================
*/
qhandle_t idGameLocal::AllocTargetTimer( const char* targetName ) {
	int i;

	for ( i = 0; i < targetTimers.Num(); i++ ) {
		if ( !idStr::Icmp( targetName, targetTimers[ i ].name ) ) {
			return i;
		}
	}

	i = targetTimers.Num();

	targetTimer_t& timer	= targetTimers.Alloc();
	timer.name				= targetName;
	timer.serverHandle		= -1;

	if ( !gameLocal.isClient ) {
		timer.serverHandle = i;
	}

	if ( gameLocal.isServer ) {
		sdReliableServerMessage outMsg( GAME_RELIABLE_SMESSAGE_CREATEPLAYERTARGETTIMER );
		outMsg.WriteString( targetName );
		outMsg.WriteShort( i );
		outMsg.Send( sdReliableMessageClientInfoAll() );
	}

	for ( int j = 0; j < MAX_CLIENTS; j++ ) {
		timer.endTimes[ j ] = -1;
	}

	return i;
}

/*
================
idGameLocal::GetTargetTimerForServerHandle
================
*/
qhandle_t idGameLocal::GetTargetTimerForServerHandle( qhandle_t timerHandle ) {
//	assert( timerHandle < targetTimerLookup.Num() );
	if ( timerHandle >= targetTimerLookup.Num() ) {
		return -1;
	}

	return targetTimerLookup[ timerHandle ];
}

/*
================
idGameLocal::GetTargetTimerValue
================
*/
int idGameLocal::GetTargetTimerValue( qhandle_t timerHandle, idPlayer* player ) {
	if ( timerHandle < 0 ) {
		return 0;
	}
	return targetTimers[ timerHandle ].endTimes[ player->entityNumber ];
}

/*
================
idGameLocal::ClearTargetTimers
================
*/
void idGameLocal::ClearTargetTimers() {
	for ( int i = 0; i < targetTimers.Num(); i++ ) {
		for ( int j = 0; j < MAX_CLIENTS; j++ ) {
			targetTimers[ i ].endTimes[ j ] = -1;
		}
	}
}

/*
================
idGameLocal::StartRecordingDemo
================
*/
void idGameLocal::StartRecordingDemo( void ) {
	idPlayer* localPlayer = GetLocalPlayer();
	if ( !localPlayer ) {
		return;
	}

	idStr playerName = localPlayer->userInfo.name;
	playerName.RemoveColors();

	sysTime_t time;
	sys->RealTime( &time );

	idStr timeStr = va( "%d%02d%02d_%02d%02d%02d", 1900 + time.tm_year, 1 + time.tm_mon, time.tm_mday, time.tm_hour, time.tm_min, time.tm_sec );

	idStr mapStr = mapFileName.c_str();
	mapStr.ReplaceChar( '/', '_' );
	mapStr.StripFileExtension();

	cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va( "recordNetDemo \"%s_%s_%s_build_%d_%d.ndm\"", timeStr.c_str(), mapStr.c_str(), playerName.c_str(), ENGINE_SRC_REVISION, ENGINE_MEDIA_REVISION ) );
}

/*
============
idGameLocal::TerritoryForPoint
============
*/
sdDeployZone* idGameLocal::TerritoryForPoint( const idVec3& point, sdTeamInfo* team, bool requireTeam, bool requireActive ) const {
	sdInstanceCollector< sdDeployZone > territories( false );

	int i;
	for ( i = 0; i < territories.Num(); i++ ) {
		sdDeployZone* territory = territories[ i ];

		if ( requireTeam ) {
			if ( territory->GetGameTeam() != team ) {
				continue;
			}
		}
		if ( requireActive ) {
			if ( !territory->IsActive() ) {
				continue;
			}
		}

		idClipModel* clip = territory->GetPhysics()->GetClipModel();
		const idVec3& origin = territory->GetPhysics()->GetOrigin();
		const idMat3& axes = territory->GetPhysics()->GetAxis();
		if ( !gameLocal.clip.ContentsModel( CLIP_DEBUG_PARMS point, NULL, mat3_identity, -1, clip, origin, axes ) ) {
			continue;
		}

		return territory;
	}
	return NULL;
}

/*
============
idGameLocal::AllocEntityState
============
*/
sdEntityState* idGameLocal::AllocEntityState( networkStateMode_t mode, idEntity* ent ) const {
	if ( ent->freeStates[ mode ] ) {
		sdEntityState* state = ent->freeStates[ mode ];
		ent->freeStates[ mode ] = state->next;
		state->next = NULL;
		return state;
	}
	return new sdEntityState( ent, mode );
}

/*
============
idGameLocal::AllocGameState
============
*/
sdGameState* idGameLocal::AllocGameState( const sdNetworkStateObject& object ) const {
	if ( object.freeStates ) {
		sdGameState* state = object.freeStates;
		object.freeStates = state->next;
		state->next = NULL;
		return state;
	}
	return new sdGameState( object );
}

/*
============
idGameLocal::OnUserStartMap
============
*/
userMapChangeResult_e idGameLocal::OnUserStartMap( const char* text, idStr& reason, idStr& mapName ) {
	sys->FlushServerInfo();
	MakeRules();
	return rules->OnUserStartMap( text, reason, mapName );
}

/*
============
idGameLocal::ArgCompletion_StartGame
============
*/
void idGameLocal::ArgCompletion_StartGame( const idCmdArgs& args, argCompletionCallback_t callback ) {
	idTypeInfo* type = GetRulesType( false );
	if ( type == NULL ) {
		return;
	}

	idClass* instance = type->CreateInstance();
	if ( instance == NULL ) {
		return;
	}

	sdGameRules* rules = instance->Cast< sdGameRules >();
	assert( rules != NULL );

	rules->ArgCompletion_StartGame( args, callback );

	delete rules;
}

/*
============
idGameLocal::ClassCount
============
*/
int idGameLocal::ClassCount( const sdDeclPlayerClass* pc, idPlayer* skip, sdTeamInfo* team ) {
	int count = 0;
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = GetClient( i );
		if ( !player || player == skip ) {
			continue;
		}

		if ( team && player->GetGameTeam() != team ) {
			continue;
		}

		const sdInventory& inv = player->GetInventory();
		if ( inv.GetClass() == pc || inv.GetCachedClass() == pc ) {
			count++;
		}
	}
	return count;
}

/*
============
idGameLocal::GetWindVector
============
*/
const idVec3& idGameLocal::GetWindVector( const idVec3 & origin ) const {
	static idVec3 wind( 100.f, 0.f, 0.f );
	return sdAtmosphere::GetAtmosphereInstance() ? sdAtmosphere::GetAtmosphereInstance()->GetWindVector() : wind;
}

/*
============
idGameLocal::AddCheapDecal
============
*/
void idGameLocal::AddCheapDecal( const idDict& decalDict, idEntity *attachTo, idVec3 &origin, idVec3 &normal, int jointIdx, int id, const char* decalName, const char* materialName ) {
	if ( isServer && networkSystem->IsDedicated() ) {
		return;
	}

	if ( !g_decals.GetBool() ) { //mal: allow players to turn decals off if desired.
		return;
	}

	// HACK: don't apply decals to things that have MASK_SHOT_BOUNDINGBOX contents
	//		 this includes players, flyer hives, repair drones, and icarii
	// FIND A BETTER WAY TO DO THIS POST-E3
	cheapDecalUsage_t attachmentType = ( attachTo ) ? attachTo->GetDecalUsage() : CDU_WORLD;

	if ( attachmentType == CDU_INHIBIT ) {
		return;
	}

	cheapDecalParameters_t params;
	params.origin = origin;
	params.normal = normal;
	params.jointIdx = jointIdx;
	int index = gameLocal.GetDecal( decalDict, decalName, materialName );
	if ( index < 0 ) {
		return;
	}
	params.decl = declHolder.FindDecalByIndex( index );

	if ( attachmentType == CDU_WORLD ) {
		gameRenderWorld->AddCheapDecal( -1, params, gameLocal.time );
	} else {
		gameRenderWorld->AddCheapDecal( attachTo->GetModelDefHandle( id ), params, gameLocal.time );
	}
}

/*
============
idGameLocal::SetSnapShotPlayer
============
*/
void idGameLocal::SetSnapShotPlayer( idPlayer* player ) {
	snapShotPlayer = player;
	aorManager.SetSnapShotPlayer( player );
}

/*
============
idGameLocal::SetSnapShotClient
============
*/
void idGameLocal::SetSnapShotClient( idPlayer* client ) {
	snapShotClient = client;
}

/*
============
idGameLocal::UpdatePlayerShadows
============
*/
void idGameLocal::UpdatePlayerShadows( void ) {
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = gameLocal.GetClient( i );
		if ( !player ) {
			continue;
		}

		player->UpdateShadows();
	}
}

/*
============
idGameLocal::LogProficiency
============
*/
void idGameLocal::LogProficiency( const char* message ) {
	if ( proficiencyLog == NULL ) {
		proficiencyLog = fileSystem->OpenFileWrite( "proficiency.log" );
	}
	if ( proficiencyLog == NULL ) {
		return;
	}

	proficiencyLog->WriteFloatString( "%s\n", message );
}

/*
============
idGameLocal::LogNetwork
============
*/
void idGameLocal::LogNetwork( const char* message ) {
	if ( networkLog == NULL ) {
		networkLog = fileSystem->OpenFileWrite( "network.log" );
	}
	if ( networkLog == NULL ) {
		return;
	}

	networkLog->Printf( message );
}

/*
============
idGameLocal::LogObjective
============
*/
void idGameLocal::LogObjective( const char* message ) {
	if ( objectiveLog == NULL ) {
		objectiveLog = fileSystem->OpenFileAppend( "objective.log" );
	}
	if ( objectiveLog == NULL ) {
		return;
	}

	objectiveLog->WriteFloatString( "%s", message );
}

/*
============
idGameLocal::LogDamage
============
*/
void idGameLocal::LogDamage( const char* message ) {
	if ( !rules || rules->GetState() != sdGameRules::GS_GAMEON ) {
		return;
	}

	if ( damageLogFile == NULL ) {
		int index = 0;
		while ( index < 1000 ) {
			idFile* temp = fileSystem->OpenFileRead( va( "damageLog%i.txt", index ) );
			if ( temp == NULL ) {
				damageLogFile = fileSystem->OpenFileWrite( va( "damageLog%i.txt", index ) );
				break;
			}
			index++;
		}
	}

	if ( !damageLogFile ) {
		Printf( "Damage Log: %s", message );
		Warning( "Failed To Log Damage to File" );
		return;
	}

	damageLogFile->WriteFloatString( "%s", message );
	damageLogFile->Flush();
}

/*
============
idGameLocal::LogDebugText
============
*/
void idGameLocal::LogDebugText( const idEntity* entityFrom, const char* fmt, ... ) {
	if ( !isClient && debugLogFile != NULL && g_logDebugText.GetBool() ) {
		va_list		argptr;
		char		text[MAX_STRING_CHARS];

		va_start( argptr, fmt );
		idStr::vsnPrintf( text, sizeof( text ), fmt, argptr );
		va_end( argptr );

		// add some interesting 
		debugLogFile->Printf( "%i, ", time );
		if ( entityFrom != NULL ) {
			debugLogFile->Printf( "%i, ", entityFrom->entityNumber );
		} else {
			debugLogFile->Printf( "-1, " );
		}
		
		debugLogFile->Printf( "\"%s\"\n", text );
	}
}

/*
================
idGameLocal::SetActionCommand
================
*/
void idGameLocal::SetActionCommand( const char* action ) {
	actionArgs.TokenizeString( action, false );
}

/*
================
idGameLocal::IsPaused
================
*/
bool idGameLocal::IsPaused( void ) {
	if ( gameLocal.isServer && gameLocal.time > gameLocal.playerSpawnTime ) { // Gordon: give the entities a little time to settle down before the menus will pause the game

		// don't do this if any clients are connected (development only, retail can't do this)
		bool hasClients = false;
		for ( int i = 0; i < MAX_CLIENTS; i++ ) {
			idPlayer* client = GetClient( i );
			if ( client != NULL && !client->IsType( idBot::Type ) && client != gameLocal.GetLocalPlayer() ) {
				hasClients = true;
				break;
			}
		}

		if ( !hasClients ) {
			sdLimboMenu* menu = gameLocal.localPlayerProperties.GetLimboMenu();
			if ( menu != NULL ) {
				if ( menu->Active() ) {
					return true;
				}
			}

			if ( gameLocal.IsMainMenuActive() ) {
				return true;
			}
		}
	}
	return isPaused;
}

/*
================
idGameLocal::SetPaused
================
*/
void idGameLocal::SetPaused( bool value ) {
	if ( isPaused == value ) {
		return;
	}
	isPaused = value;

	if ( gameLocal.isServer ) {
		SendPauseInfo( sdReliableMessageClientInfoAll() );
	}

	OnPausedChanged();
}

/*
================
idGameLocal::OnPausedChanged
================
*/
void idGameLocal::OnPausedChanged( void ) {
	if ( isPaused ) {
		pauseViewInited = false;
		pauseStartGuiTime = gameLocal.ToGuiTime( gameLocal.time );
	} else {
	}
}

/*
================
idGameLocal::GetPausedView
================
*/
void idGameLocal::GetPausedView( idVec3& origin, idMat3& axis ) {
	if ( !pauseViewInited ) {
		idPlayer* localPlayer = GetLocalPlayer();
		if ( localPlayer == NULL ) {
			origin.Zero();
			axis.Identity();
			return;
		}

		pauseViewOrg			= localPlayer->firstPersonViewOrigin;
		pauseViewAngles			= localPlayer->firstPersonViewAxis.ToAngles();
		pauseViewAngles.roll	= 0.f;
		pauseViewInited			= true;

		const usercmd_t& cmd = gameLocal.usercmds[ localPlayer->entityNumber ];
		for ( int i = 0; i < 3; i++ ) {
			pauseViewAnglesBase[ i ] = pauseViewAngles[ i ] - SHORT2ANGLE( cmd.angles[ i ] );
		}
	}

	origin	= pauseViewOrg;
	axis	= pauseViewAngles.ToMat3();
}

idCVar g_pauseNoClipSpeed( "g_pauseNoClipSpeed", "100", CVAR_GAME | CVAR_FLOAT, "speed to move when in pause noclip mode" );

/*
================
idGameLocal::UpdatePauseNoClip
================
*/
void idGameLocal::UpdatePauseNoClip( usercmd_t& cmd ) {
	const float speed = g_pauseNoClipSpeed.GetFloat();

	idVec3 dir;
	dir.x = ( cmd.forwardmove / 127.f ) * speed;
	dir.y = -( cmd.rightmove / 127.f ) * speed;
	dir.z = ( cmd.upmove / 127.f ) * speed;

	for ( int i = 0; i < 3; i++ ) {
		pauseViewAngles[ i ] = idMath::AngleNormalize180( pauseViewAnglesBase[ i ] + SHORT2ANGLE( cmd.angles[ i ] ) );
	}

	pauseViewOrg += pauseViewAngles.ToMat3() * dir;
}

/*
================
idGameLocal::SendPauseInfo
================
*/
void idGameLocal::SendPauseInfo( const sdReliableMessageClientInfoBase& info ) {
	sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_PAUSEINFO );
	msg.WriteBool( isPaused );
	msg.WriteLong( time );
	msg.WriteLong( timeOffset );
	msg.Send( info );
}

/*
================
idGameLocal::OnInputInit
================
*/
void idGameLocal::OnInputInit( void ) {
	defaultBindContext = keyInputManager->AllocBindContext( "default" );

	sdInstanceCollector< sdTransport > transports( true );
	for ( int i = 0; i < transports.Num(); i++ ) {
		transports[ i ]->OnInputInit();
	}
	for ( int i = 0; i < declPlayerClassType.Num(); i++ ) {
		const sdDeclPlayerClass* cls = declPlayerClassType.LocalFindByIndex( i, false );
		if ( !cls->IsValid() ) {
			continue;
		}

		cls->OnInputInit();
	}

	uiManager->OnInputInit();
}

/*
================
idGameLocal::OnInputShutdown
================
*/
void idGameLocal::OnInputShutdown( void ) {
	defaultBindContext = NULL;

	sdInstanceCollector< sdTransport > transports( true );
	for ( int i = 0; i < transports.Num(); i++ ) {
		transports[ i ]->OnInputShutdown();
	}
	for ( int i = 0; i < declPlayerClassType.Num(); i++ ) {
		const sdDeclPlayerClass* cls = declPlayerClassType.LocalFindByIndex( i, false );
		if ( !cls->IsValid() ) {
			continue;
		}

		cls->OnInputShutdown();
	}
	uiManager->OnInputShutdown();
}

/*
================
idGameLocal::OnLanguageInit
================
*/
void idGameLocal::OnLanguageInit() {
	uiManager->OnLanguageInit();
}
/*
================
idGameLocal::OnLanguageShutdown
================
*/
void idGameLocal::OnLanguageShutdown() {
	uiManager->OnLanguageShutdown();
}

/*
================
idGameLocal::Translate
================
*/
sdKeyCommand* idGameLocal::Translate( const idKey& key ) {
	sdKeyCommand* cmd;
	if ( TranslateGuiBind( key, &cmd ) ) {
		return cmd;
	}

	idPlayer* localPlayer = GetLocalPlayer();
	if ( localPlayer != NULL ) {
		sdBindContext* specialContext = NULL;

		idEntity* proxy = localPlayer->GetProxyEntity();
		if ( proxy != NULL ) {
			specialContext = proxy->GetBindContext();
		} else {
			if ( !localPlayer->IsSpectator() ) {
				const sdDeclPlayerClass* cls = localPlayer->GetInventory().GetClass();
				if ( cls != NULL ) {
					specialContext = cls->GetBindContext();
				}
			}
		}

		if ( specialContext != NULL ) {
			sdKeyCommand* cmd = keyInputManager->GetCommand( specialContext, key );
			if ( cmd != NULL ) {
				return cmd;
			}
		}
	}

	return keyInputManager->GetCommand( defaultBindContext, key );
}


/*
============
sdSpawnPoint::Clear
============
*/
void sdSpawnPoint::Clear( void ) {
	_angles.Zero();
	_lastUsedTime = 0;
	_offset.Zero();
	_owner = NULL;
	_relativePosition = false;
	_requirements.Clear();
	_parachute = false;
	_inUse = false;
}

typedef struct {
	const char*			string;
	usercmdbuttonType_t type;
	int					button;
} userCmdString_t;

userCmdString_t	userCmdStrings[] = {
	{ "_moveUp",			B_BUTTON,	UB_UP },
	{ "_moveDown",			B_BUTTON,	UB_DOWN },
	{ "_left",				B_BUTTON,	UB_LEFT },
	{ "_right",				B_BUTTON,	UB_RIGHT },
	{ "_forward",			B_BUTTON,	UB_FORWARD },
	{ "_back",				B_BUTTON,	UB_BACK },
	{ "_lookUp",			B_BUTTON,	UB_LOOKUP },
	{ "_lookDown",			B_BUTTON,	UB_LOOKDOWN },
	{ "_strafe",			B_BUTTON,	UB_STRAFE },
	{ "_moveLeft",			B_BUTTON,	UB_MOVELEFT },
	{ "_moveRight",			B_BUTTON,	UB_MOVERIGHT },

	{ "_attack",			B_BUTTON,	UB_ATTACK },
	{ "_modeswitch",		B_BUTTON,	UB_MODESWITCH },
	{ "_speed",				B_BUTTON,	UB_SPEED },
	{ "_sprint",			B_BUTTON,	UB_SPRINT },
	{ "_activate",			B_BUTTON,	UB_ACTIVATE },
	{ "_showScores",		B_BUTTON,	UB_SHOWSCORES },
	{ "_voice",				B_BUTTON,	UB_VOICE },
	{ "_teamVoice",			B_BUTTON,	UB_TEAMVOICE },
	{ "_fireteamVoice",		B_BUTTON,	UB_FIRETEAMVOICE },
	{ "_mlook",				B_BUTTON,	UB_MLOOK },
	{ "_altattack",			B_BUTTON,	UB_ALTATTACK },
	{ "_tophat",			B_BUTTON,	UB_TOPHAT },
	{ "_leanLeft",			B_BUTTON,	UB_LEANLEFT },
	{ "_leanRight",			B_BUTTON,	UB_LEANRIGHT },
	
	{ "_weapon0",			B_IMPULSE,	UCI_WEAP0 },
	{ "_weapon1",			B_IMPULSE,	UCI_WEAP1 },
	{ "_weapon2",			B_IMPULSE,	UCI_WEAP2 },
	{ "_weapon3",			B_IMPULSE,	UCI_WEAP3 },
	{ "_weapon4",			B_IMPULSE,	UCI_WEAP4 },
	{ "_weapon5",			B_IMPULSE,	UCI_WEAP5 },
	{ "_weapon6",			B_IMPULSE,	UCI_WEAP6 },
	{ "_weapon7",			B_IMPULSE,	UCI_WEAP7 },
	{ "_weapon8",			B_IMPULSE,	UCI_WEAP8 },
	{ "_weapon9",			B_IMPULSE,	UCI_WEAP9 },
	{ "_weapon10",			B_IMPULSE,	UCI_WEAP10 },
	{ "_weapon11",			B_IMPULSE,	UCI_WEAP11 },
	{ "_weapon12",			B_IMPULSE,	UCI_WEAP12 },
	{ "_reload",			B_IMPULSE,	UCI_RELOAD },
	{ "_weapnext",			B_IMPULSE,	UCI_WEAPNEXT },
	{ "_weapprev",			B_IMPULSE,	UCI_WEAPPREV },
	{ "_stroyup",			B_IMPULSE,	UCI_STROYUP },
	{ "_stroydown",			B_IMPULSE,	UCI_STROYDOWN },
	{ "_usevehicle",		B_IMPULSE,	UCI_USE_VEHICLE },
	{ "_vehiclecamera",		B_IMPULSE,	UCI_VEHICLE_CAMERA_MODE },
	{ "_prone",				B_IMPULSE,	UCI_PRONE },
	{ "_ready",				B_IMPULSE,	UCI_READY },

	{ "_admin",				B_LOCAL_IMPULSE,	ULI_ADMIN_MENU },
	{ "_votemenu",			B_LOCAL_IMPULSE,	ULI_VOTE_MENU },
	{ "_quickchat",			B_LOCAL_IMPULSE,	ULI_QUICKCHAT },
	{ "_limbomenu",			B_LOCAL_IMPULSE,	ULI_LIMBO_MENU },
	{ "_commandmap",		B_LOCAL_IMPULSE,	ULI_COMMAND_MAP },
	{ "_taskmenu",			B_LOCAL_IMPULSE,	ULI_TASK_MENU },
	{ "_fireteam",			B_LOCAL_IMPULSE,	ULI_FIRETEAM },
	{ "_menuClick",			B_LOCAL_IMPULSE,	ULI_MENU_EVENT_CLICK },
	{ "_menuContext",		B_LOCAL_IMPULSE,	ULI_MENU_EVENT_CONTEXT },
	{ "_menuNavForward",	B_LOCAL_IMPULSE,	ULI_MENU_EVENT_NAV_FORWARD },
	{ "_menuNavBackward",	B_LOCAL_IMPULSE,	ULI_MENU_EVENT_NAV_BACKWARD },
	{ "_menuAccept",		B_LOCAL_IMPULSE,	ULI_MENU_EVENT_ACCEPT },
	{ "_menuCancel",		B_LOCAL_IMPULSE,	ULI_MENU_EVENT_CANCEL },
	{ "_menuNewline",		B_LOCAL_IMPULSE,	ULI_MENU_NEWLINE },
	{ "_menuEvent1",		B_LOCAL_IMPULSE,	ULI_MENU_EVENT_GENERAL1 },
	{ "_menuEvent2",		B_LOCAL_IMPULSE,	ULI_MENU_EVENT_GENERAL2 },
	{ "_menuEvent3",		B_LOCAL_IMPULSE,	ULI_MENU_EVENT_GENERAL3 },
	{ "_menuEvent4",		B_LOCAL_IMPULSE,	ULI_MENU_EVENT_GENERAL4 },
	{ "_menuEvent5",		B_LOCAL_IMPULSE,	ULI_MENU_EVENT_GENERAL5 },
	{ "_menuEvent6",		B_LOCAL_IMPULSE,	ULI_MENU_EVENT_GENERAL6 },
	{ "_showLocations",		B_LOCAL_IMPULSE,	ULI_SHOWLOCATIONS },
	{ "_context",			B_LOCAL_IMPULSE,	ULI_CONTEXT },
	{ "_showwaypoints",		B_LOCAL_IMPULSE,	ULI_SHOW_WAYPOINTS },
	{ "_showFireTeam",		B_LOCAL_IMPULSE,	ULI_SHOW_FIRETEAM },
};

const int NUM_USERCMD_STRING = sizeof( userCmdStrings ) / sizeof( userCmdStrings[ 0 ] );

/*
================
idGameLocal::SetupBinding
================
*/
usercmdbuttonType_t idGameLocal::SetupBinding( const char* binding, int& action ) {
	for ( int i = 0; i < NUM_USERCMD_STRING; i++ ) {
		if ( idStr::Icmp( userCmdStrings[ i ].string, binding ) != 0 ) {
			continue;
		}

		action = userCmdStrings[ i ].button;
		return userCmdStrings[ i ].type;
	}

	action = -1;
	return B_COMMAND;
}

/*
================
idGameLocal::HandleLocalImpulse
================
*/
void idGameLocal::HandleLocalImpulse( int action, bool down ) {
	switch ( action ) {
		case ULI_COMMAND_MAP: {
			if ( down ) {
				localPlayerProperties.ToggleCommandMap();
			}
			break;
		}

		case ULI_TASK_MENU: {
			if ( down ) {
				if ( sdDemoManager::GetInstance().InPlayBack() ) {
					break;
				}
				idPlayer* player = gameLocal.GetLocalPlayer();
				if ( player != NULL ) {
					player->SelectNextTask();
				}
			}
			break;
		}

		case ULI_ADMIN_MENU: {
			if ( down ) {
				if ( sdDemoManager::GetInstance().InPlayBack() ) {
					break;
				}
				idPlayer* player = gameLocal.GetLocalPlayer();
				if ( player != NULL ) {
					if( sdUserInterfaceScope* scope = gameLocal.globalProperties.GetSubScope( "gameHud" ) ) {
						if( sdProperties::sdProperty* property = scope->GetProperty( "wantAdmin", sdProperties::PT_FLOAT ) ) {
							*property->value.floatValue = 1.0f;
						}
					} else {
						gameLocal.Warning( "idGameLocal::HandleLocalImpulse: Couldn't find global 'gameHud' scope in guiGlobals." );
					}
					sdLimboMenu* limboMenu = gameLocal.localPlayerProperties.GetLimboMenu();
					if ( !limboMenu->Enabled() ) {
						limboMenu->Enable( true, true );
					}
				}
			}
			break;
		}
		case ULI_VOTE_MENU: {
			if ( down ) {
				if ( sdDemoManager::GetInstance().InPlayBack() ) {
					break;
				}
				idPlayer* player = gameLocal.GetLocalPlayer();
				if ( player != NULL ) {
					if( sdUserInterfaceScope* scope = gameLocal.globalProperties.GetSubScope( "gameHud" ) ) {
						if( sdProperties::sdProperty* property = scope->GetProperty( "wantVote", sdProperties::PT_FLOAT ) ) {
							*property->value.floatValue = 1.0f;
						}
					} else {
						gameLocal.Warning( "idGameLocal::HandleLocalImpulse: Couldn't find global 'gameHud' scope in guiGlobals." );
					}
					sdLimboMenu* limboMenu = gameLocal.localPlayerProperties.GetLimboMenu();
					if ( !limboMenu->Enabled() ) {
						limboMenu->Enable( true, true );
					}
				}
			}
			break;
		}
		case ULI_LIMBO_MENU: {
			if ( down ) {
				if ( sdDemoManager::GetInstance().InPlayBack() ) {
					break;
				}

				if ( !networkSystem->IsDedicated() ) {
					if( sdUserInterfaceScope* scope = gameLocal.globalProperties.GetSubScope( "gameHud" ) ) {
						if( sdProperties::sdProperty* property = scope->GetProperty( "wantAdmin", sdProperties::PT_FLOAT ) ) {
							*property->value.floatValue = 0.0f;
						}
						if( sdProperties::sdProperty* property = scope->GetProperty( "wantVote", sdProperties::PT_FLOAT ) ) {
							*property->value.floatValue = 0.0f;
						}
					} else {
						gameLocal.Warning( "idGameLocal::HandleLocalImpulse: Couldn't find global 'gameHud' scope in guiGlobals." );
					}

					sdLimboMenu* limboMenu = gameLocal.localPlayerProperties.GetLimboMenu();
					if ( limboMenu->Enabled() ) {
						limboMenu->Enable( false );
					} else {
						limboMenu->Enable( true, true );
					}

					sdQuickChatMenu* contextMenu = localPlayerProperties.GetContextMenu();
					if ( contextMenu->Enabled() ) {
						contextMenu->Enable( false );
					}

					sdQuickChatMenu* quickChatMenu = localPlayerProperties.GetQuickChatMenu();
					if ( quickChatMenu->Enabled() ) {
						quickChatMenu->Enable( false );
					}
				}
			}
			break;
		}

		case ULI_QUICKCHAT: {
			if ( down ) {
				if ( sdDemoManager::GetInstance().InPlayBack() ) {
					break;
				}
				idPlayer* player = gameLocal.GetLocalPlayer();
				if ( player != NULL ) {
					if ( !player->IsSpectator() ) {
						sdQuickChatMenu* contextMenu = localPlayerProperties.GetContextMenu();
						if ( contextMenu->Enabled() ) {
							contextMenu->Enable( false );
						}

						sdWeaponSelectionMenu* weaponMenu = localPlayerProperties.GetWeaponSelectionMenu();
						if( weaponMenu->Enabled() ) {
							weaponMenu->Enable( false );
						}

						sdChatMenu* chatMenu = localPlayerProperties.GetChatMenu();
						if( chatMenu->Enabled() ) {
							chatMenu->Enable( false );
						}

						sdFireTeamMenu* fireTeamMenu = localPlayerProperties.GetFireTeamMenu();
						if ( fireTeamMenu->Enabled() ) {
							fireTeamMenu->Enable( false );
						}

						sdQuickChatMenu* quickChatMenu = localPlayerProperties.GetQuickChatMenu();
						if ( quickChatMenu->Enabled() ) {
							quickChatMenu->Enable( false );
						} else {
							quickChatMenu->Enable( true, true );
						}
					}
				}
			}
			break;
		}

		case ULI_CONTEXT: {
			if ( down ) {
				if ( sdDemoManager::GetInstance().InPlayBack() ) {
					break;
				}

				idPlayer* player = gameLocal.GetLocalPlayer();
				if ( player != NULL ) {
					if ( !player->IsSpectator() ) {
						sdQuickChatMenu* quickChatMenu = localPlayerProperties.GetQuickChatMenu();
						if ( quickChatMenu->Enabled() ) {
							quickChatMenu->Enable( false );
						}

						sdWeaponSelectionMenu* weaponMenu = localPlayerProperties.GetWeaponSelectionMenu();
						if( weaponMenu->Enabled() ) {
							weaponMenu->Enable( false );
						}

						sdChatMenu* chatMenu = localPlayerProperties.GetChatMenu();
						if( chatMenu->Enabled() ) {
							chatMenu->Enable( false );
						}
						sdFireTeamMenu* fireTeamMenu = localPlayerProperties.GetFireTeamMenu();
						if ( fireTeamMenu->Enabled() ) {
							fireTeamMenu->Enable( false );
						}

						sdQuickChatMenu* contextMenu = localPlayerProperties.GetContextMenu();
						if ( contextMenu->Enabled() ) {
							contextMenu->Enable( false );
						} else {
							contextMenu->Enable( true, true );							
						}
					}
				}
			}
			break;
		}

		case ULI_FIRETEAM: {
			if ( down ) {
				if ( sdDemoManager::GetInstance().InPlayBack() ) {
					break;
				}

				idPlayer* player = gameLocal.GetLocalPlayer();
				if ( player != NULL ) {
					if ( !player->IsSpectator() ) {
						sdFireTeamMenu* fireTeamMenu = localPlayerProperties.GetFireTeamMenu();
						if ( fireTeamMenu->Enabled() ) {
							fireTeamMenu->Enable( false );
						} else {
							fireTeamMenu->Enable( true, true );
						}
					}
					sdQuickChatMenu* quickChatMenu = localPlayerProperties.GetQuickChatMenu();
					if ( quickChatMenu->Enabled() ) {
						quickChatMenu->Enable( false );
					}
					sdQuickChatMenu* contextMenu = localPlayerProperties.GetContextMenu();
					if ( contextMenu->Enabled() ) {
						contextMenu->Enable( false );
					}
					sdWeaponSelectionMenu* weaponMenu = localPlayerProperties.GetWeaponSelectionMenu();
					if( weaponMenu->Enabled() ) {
						weaponMenu->Enable( false );
					}
				}
			}
			break;
		}

		case ULI_MENU_EVENT_CLICK: {
			const sdSysEvent* event = sys->GenerateMouseButtonEvent( 1, down );
			gameLocal.HandleGuiEvent( event );
			sys->FreeEvent( event );
			break;
		}

		case ULI_MENU_EVENT_CONTEXT: {
			const sdSysEvent* event = sys->GenerateMouseButtonEvent( 2, down );
			gameLocal.HandleGuiEvent( event );
			sys->FreeEvent( event );
			break;
		}

		case ULI_MENU_EVENT_GENERAL1:
		case ULI_MENU_EVENT_GENERAL2:
		case ULI_MENU_EVENT_GENERAL3:
		case ULI_MENU_EVENT_GENERAL4:
		case ULI_MENU_EVENT_GENERAL5:
		case ULI_MENU_EVENT_GENERAL6: 
		case ULI_MENU_EVENT_NAV_FORWARD:
		case ULI_MENU_EVENT_NAV_BACKWARD:
		case ULI_MENU_EVENT_ACCEPT:
		case ULI_MENU_EVENT_CANCEL:
		case ULI_MENU_NEWLINE: {
			if( down ) {
				const sdSysEvent* event = sys->GenerateGuiEvent( action );
				gameLocal.HandleGuiEvent( event );
				sys->FreeEvent( event );
			}			
			break;
		}

		case ULI_SHOWLOCATIONS: {
			sdLocationMarker::ShowLocations( down );
			break;
		}

		case ULI_SHOW_WAYPOINTS: {
			sdWayPointManager::GetInstance().ShowWayPoints( down );
			break;
		}
		case ULI_SHOW_FIRETEAM: {
			localPlayerProperties.SetShowFireTeam( down );
			break;
		 }
	}
}

/*
================
idGameLocal::GetCookieString
================
*/
const char* idGameLocal::GetCookieString( const char* key ) {
	sdNetUser* user = networkService->GetActiveUser();
	if ( user == NULL ) {
		return NULL;
	}

	return user->GetProfile().GetProperties().GetString( va( "cookie_%s", key ) );
}

/*
================
idGameLocal::GetCookieInt
================
*/
int idGameLocal::GetCookieInt( const char* key ) {
	sdNetUser* user = networkService->GetActiveUser();
	if ( user == NULL ) {
		return 0;
	}

	return user->GetProfile().GetProperties().GetInt( va( "cookie_%s", key ) );
}

/*
================
idGameLocal::SetCookieString
================
*/
void idGameLocal::SetCookieString( const char* key, const char* value ) {
	sdNetUser* user = networkService->GetActiveUser();
	if ( user == NULL ) {
		return;
	}

	user->GetProfile().GetProperties().Set( va( "cookie_%s", key ), value );
}

/*
================
idGameLocal::SetCookieInt
================
*/
void idGameLocal::SetCookieInt( const char* key, int value ) {
	sdNetUser* user = networkService->GetActiveUser();
	if ( user == NULL ) {
		return;
	}

	user->GetProfile().GetProperties().SetInt( va( "cookie_%s", key ), value );
}

/*
================
idGameLocal::PurgeAndLoadTeamAssets
================
*/
void idGameLocal::PurgeAndLoadTeamAssets( const sdDeclStringMap* newPartialLoadTeamAssets ) {
	if ( newPartialLoadTeamAssets == currentPartialLoadTeamAssets ) {
		return;
	}

	// unload old media
	if ( currentPartialLoadTeamAssets != NULL ) {
		const sdDeclStringMap* stringMap = currentPartialLoadTeamAssets;
		if ( stringMap != NULL ) {
			for ( int i = 0; i < stringMap->GetDict().GetNumKeyVals(); i++ ) {
				const idStr& value = stringMap->GetDict().GetKeyVal( i )->GetValue();

				idRenderModel* renderModel = NULL;

				// this does a for loop rather than by name, as we don't actually want to load declarations
				const idDeclModelDef* modelDef = NULL;
				for ( int j = 0; j < gameLocal.declModelDefType.Num(); j++ ) {
					modelDef = gameLocal.declModelDefType.LocalFindByIndex( j, false );
					if ( value.Icmp( modelDef->GetName() ) == 0 ) {
						break;
					}
					modelDef = NULL;
				}

				if ( modelDef != NULL ) {
					if ( modelDef->GetState() == DS_PARSED ) {
						renderModel = modelDef->ModelHandle();
					}
				} else {
					renderModel = renderModelManager->GetModel( value.c_str() );
				}

				if ( renderModel != NULL ) {
					renderModel->PurgePartialLoadableImages();
				}
			}
		}
	}

	// load new media
	if ( newPartialLoadTeamAssets != NULL ) {
		const sdDeclStringMap* stringMap = newPartialLoadTeamAssets;
		if ( stringMap != NULL ) {
			for ( int i = 0; i < stringMap->GetDict().GetNumKeyVals(); i++ ) {
				const idStr& value = stringMap->GetDict().GetKeyVal( i )->GetValue();

				idRenderModel* renderModel = NULL;

				// this does a for loop rather than by name, as we don't actually want to load declarations
				const idDeclModelDef* modelDef = NULL;
				for ( int j = 0; j < gameLocal.declModelDefType.Num(); j++ ) {
					modelDef = gameLocal.declModelDefType.LocalFindByIndex( j, false );
					if ( value.Icmp( modelDef->GetName() ) == 0 ) {
						break;
					}
					modelDef = NULL;
				}

				if ( modelDef != NULL ) {
					if ( modelDef->GetState() == DS_PARSED ) {
						renderModel = modelDef->ModelHandle();
					}
				} else {
					renderModel = renderModelManager->GetModel( value.c_str() );
				}

				if ( renderModel != NULL ) {
					renderModel->LoadPartialLoadableImages();
				}
			}
		}
	}

	currentPartialLoadTeamAssets = newPartialLoadTeamAssets;
}

/*
================
idGameLocal::AddEntityOcclusionQuery
================
*/
qhandle_t idGameLocal::AddEntityOcclusionQuery( idEntity *ent ) {
	if ( ent != NULL ) {
		if ( occlusionQueryList.FindIndex( ent ) < 0 ) {
			occlusionQueryList.Append( ent );
			return gameRenderWorld->AddOcclusionTestDef( &ent->GetOcclusionQueryInfo() );
		}
	}
	return -1;
}

/*
================
idGameLocal::FreeEntityOcclusionQuery
================
*/
void idGameLocal::FreeEntityOcclusionQuery( idEntity *ent ) {
	gameRenderWorld->FreeOcclusionTestDef( ent->GetOcclusionQueryHandle() );
	occlusionQueryList.Remove( ent );
}

/*
============
idGameLocal::MessageBox
============
*/
void idGameLocal::MessageBox( msgBoxType_t type, const wchar_t* message, const sdDeclLocStr* title ) {
	using namespace sdProperties;

	sdUserInterfaceScope* scope = gameLocal.globalProperties.GetSubScope( "messageBox" );
	if( scope == NULL ) {
		gameLocal.Warning( "idGameLocal::MessageBox: Couldn't find global 'messageBox' scope in guiGlobals." );
		return;
	}
			
	if( sdProperty* property = scope->GetProperty( "message", PT_WSTRING )) {
		*property->value.wstringValue = message;
	}
	if( sdProperty* property = scope->GetProperty( "title", PT_INT )) {
		*property->value.intValue = title->Index();
	}
	if( sdProperty* property = scope->GetProperty( "type", PT_FLOAT )) {
		*property->value.floatValue = type;
	}
	if( sdProperty* property = scope->GetProperty( "active", PT_FLOAT )) {
		*property->value.floatValue = 1.0f;
	}
}


/*
============
idGameLocal::CloseMessageBox
============
*/
void idGameLocal::CloseMessageBox() {
	using namespace sdProperties;

	sdUserInterfaceScope* scope = gameLocal.globalProperties.GetSubScope( "messageBox" );
	if( scope == NULL ) {
		gameLocal.Warning( "idGameLocal::MessageBox: Couldn't find global 'messageBox' scope in guiGlobals." );
		return;
	}

	if( sdProperty* property = scope->GetProperty( "active", PT_FLOAT )) {
		*property->value.floatValue = 0.0f;
	}
}


/*
==============
idGameLocal::EntitiesOfClass
==============
*/
int idGameLocal::EntitiesOfClass( const char *name, idList< idEntityPtr<idEntity> > &list ) const {
	int entCount = 0;

	for( idEntity* ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		idStr classname;
		if ( ent->spawnArgs.GetString( "classname", "", classname ) ) {
			if ( classname.Cmp( name ) == 0 ) {
				idEntityPtr<idEntity> &entityPtr = list.Alloc();
                entityPtr = ent;
			}
		}
	}

	return list.Num();
}

/*
============
idGameLocal::LogComplaint
============
*/
void idGameLocal::LogComplaint( idPlayer* player, idPlayer* attacker ) {
	const sdUserGroup& kickGroup = sdUserGroupManager::GetInstance().GetGroup( attacker->GetUserGroup() );
	if ( kickGroup.HasPermission( PF_NO_BAN ) || gameLocal.IsLocalPlayer( attacker ) ) {
		return;
	}

	assert( player != NULL && attacker != NULL );

	idWStrList parms;
	parms.Alloc() = player->userInfo.wideName;
	attacker->SendLocalisedMessage( declHolder.declLocStrType[ "game/complaint" ], parms );

	int clientNum = attacker->entityNumber;

	const char* reason = "engine/disconnect/toomanycomplaints";

	clientCompaintCount[ clientNum ]++;
	int maxComplaints = g_complaintLimit.GetInteger();
	if ( maxComplaints > 0 && clientCompaintCount[ clientNum ] >= maxComplaints ) {
		networkSystem->ServerKickClient( clientNum, reason, true );
		return;
	}

	maxComplaints = g_complaintGUIDLimit.GetInteger();
	if ( maxComplaints > 0 ) {
		if ( networkService->GetDedicatedServerState() == sdNetService::DS_ONLINE ) {
			sdNetClientId clientId;
			networkSystem->ServerGetClientNetId( player->entityNumber, clientId );

			if ( clientId.IsValid() ) {
				clientUniqueComplaints[ clientNum ].AddUnique( clientId );

				if ( clientUniqueComplaints[ clientNum ].Num() >= maxComplaints ) {
					sdPlayerStatEntry* stat = sdGlobalStatsTracker::GetInstance().GetStat( sdGlobalStatsTracker::GetInstance().AllocStat( "times_kicked_complaints", sdNetStatKeyValue::SVT_INT ) );
					stat->IncreaseValue( clientNum, 1 );

					networkSystem->ServerKickClient( clientNum, reason, true );
					return;
				}
			}
		}
	}
}

/*
============
idGameLocal::DoSkyCheck
============
*/
bool idGameLocal::DoSkyCheck( const idVec3& location ) const {
	idVec3 end = location;
	end.z += 65535.f;

	trace_t trace;
	memset( &trace, 0, sizeof( trace ) );

	gameLocal.clip.TranslationWorld( CLIP_DEBUG_PARMS trace, location, end, NULL, mat3_identity, MASK_SHOT_RENDERMODEL );

	if ( trace.c.material == NULL ) {
		return false;
	}

	return ( trace.c.material->GetSurfaceFlags() & SURF_NOIMPACT ) != 0;
/*
	// TWTODO: Check that this functions correctly
	
	const sdPlayZone* playZoneHeight = gameLocal.GetPlayZone( location, sdPlayZone::PZF_HEIGHTMAP );
	if ( playZoneHeight == NULL ) {
		return false;
	}

	const sdHeightMapInstance& heightMap = playZoneHeight->GetHeightMap();
	if ( !heightMap.IsValid() ) {
		return false;
	}

	if ( heightMap.GetHeight( location ) > location.z + 16.0f ) {
		return false;
	}

	return true;
*/
}


/*
============
idGameLocal::MutePlayerLocal
============
*/
void idGameLocal::MutePlayerLocal( idPlayer* player, int clientIndex ) {
	if ( clientIndex < 0 || clientIndex >= MAX_CLIENTS ) {
		return;
	}

	if ( clientIndex == player->entityNumber ) {
		return;
	}

	if ( gameLocal.isClient ) {
		sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_SETMUTESTATUS );
		msg.WriteBool( true );
		msg.WriteByte( clientIndex );
		msg.Send();
	} else {
		clientMuteMask[ clientIndex ].Set( player->entityNumber );
	}
}

/*
============
idGameLocal::UnMutePlayerLocal
============
*/
void idGameLocal::UnMutePlayerLocal( idPlayer* player, int clientIndex ) {
	if ( clientIndex < 0 || clientIndex >= MAX_CLIENTS ) {
		return;
	}

	if ( clientIndex == player->entityNumber ) {
		return;
	}

	if ( gameLocal.isClient ) {
		sdReliableClientMessage msg( GAME_RELIABLE_CMESSAGE_SETMUTESTATUS );
		msg.WriteBool( false );
		msg.WriteByte( clientIndex );
		msg.Send();
	} else {
		clientMuteMask[ clientIndex ].Clear( player->entityNumber );
	}
}


/*
============
idGameLocal::MutePlayerQuickChatLocal
============
*/
void idGameLocal::MutePlayerQuickChatLocal( int clientIndex ) {
	if ( clientIndex == localClientNum ) {
		return;
	}

	idPlayer* player = gameLocal.GetClient( clientIndex );
	if ( player == NULL ) {
		return;
	}

	int i;
	for ( i = 0; i < clientQuickChatMuteList.Num(); i++ ) {
		if ( clientQuickChatMuteList[ i ].name == player->userInfo.rawName ) {
			break;
		}
	}
	if ( i == clientQuickChatMuteList.Num() ) {
		quickChatMuteEntry_t& entry = clientQuickChatMuteList.Alloc();
		entry.name = player->userInfo.rawName;
	}
}

/*
============
idGameLocal::UnMutePlayerQuickChatLocal
============
*/
void idGameLocal::UnMutePlayerQuickChatLocal( int clientIndex ) {
	if ( clientIndex == localClientNum ) {
		return;
	}

	idPlayer* player = gameLocal.GetClient( clientIndex );
	if ( player == NULL ) {
		return;
	}

	for ( int i = 0; i < clientQuickChatMuteList.Num(); i++ ) {
		if ( clientQuickChatMuteList[ i ].name == player->userInfo.rawName ) {
			clientQuickChatMuteList.RemoveIndexFast( i );
			break;
		}
	}
}

/*
============
idGameLocal::IsClientQuickChatMuted
============
*/
bool idGameLocal::IsClientQuickChatMuted( idPlayer* player ) {
	if ( gameLocal.IsLocalPlayer( player ) ) {
		return false;
	}

	for ( int i = 0; i < clientQuickChatMuteList.Num(); i++ ) {
		if ( clientQuickChatMuteList[ i ].name == player->userInfo.rawName ) {
			return true;
		}
	}
	return false;
}

/*
============
idGameLocal::OnUserNameChanged
============
*/
void idGameLocal::OnUserNameChanged( idPlayer* player, idStr oldName, idStr newName ) {
	for ( int i = 0; i < clientQuickChatMuteList.Num(); i++ ) {
		if ( clientQuickChatMuteList[ i ].name == oldName ) {
			clientQuickChatMuteList[ i ].name = newName;
			return;
		}
	}
}

/*
============
idGameLocal::AllocEndGameStat
============
*/
int idGameLocal::AllocEndGameStat( void ) {
	int index = endGameStats.Num();
	savedPlayerStat_t& stat = endGameStats.Alloc();
	stat.name = "";
	stat.value = 0.f;
	stat.rank = NULL;
	stat.team = NULL;
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		stat.data[ i ] = -1.f;
	}
	return index;
}

/*
============
idGameLocal::SetEndGameStatValue
============
*/
void idGameLocal::SetEndGameStatValue( int statIndex, idPlayer* player, float value ) {
	assert( player != NULL );
	endGameStats[ statIndex ].data[ player->entityNumber ] = value;
}

/*
============
idGameLocal::SetEndGameStatWinner
============
*/
void idGameLocal::SetEndGameStatWinner( int statIndex, idPlayer* player ) {
	savedPlayerStat_t& stat = endGameStats[ statIndex ];
	if ( player == NULL ) {
		stat.name = "";
		stat.value = 0.f;
		stat.rank = NULL;
		stat.team = NULL;
	} else {
		stat.name = player->userInfo.name;
		stat.value = stat.data[ player->entityNumber ];
		stat.rank = player->GetProficiencyTable().GetRank();
		stat.team = player->GetTeam();
	}
}

/*
============
idGameLocal::SendEndGameStats
============
*/
void idGameLocal::SendEndGameStats( const sdReliableMessageClientInfoBase& target ) {
	if ( gameLocal.isClient ) {
		return;
	}

	if ( target.SendToClients() && target.SendToAll() ) {
		OnEndGameStatsReceived();
	}

	if ( endGameStats.Num() == 0 ) {
		return;
	}

	if ( gameLocal.isServer ) {
		if ( target.SendToClients() ) {
			if ( target.SendToAll() ) {
				for ( int i = 0; i < MAX_CLIENTS; i++ ) {
					if ( GetClient( i ) == NULL ) {
						continue;
					}

					sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_ENDGAMESTATS );
					msg.WriteLong( endGameStats.Num() );
					for ( int j = 0; j < endGameStats.Num(); j++ ) {
						savedPlayerStat_t& stat = endGameStats[ j ];

						msg.WriteLong( stat.rank == NULL ? -1 : stat.rank->Index() );
						msg.WriteString( stat.name.c_str() );
						msg.WriteFloat( stat.value );
						msg.WriteFloat( stat.data[ i ] );
					}
					msg.Send( sdReliableMessageClientInfo( i ) );
				}
			} else {
				sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_ENDGAMESTATS );
				msg.WriteLong( endGameStats.Num() );
				for ( int i = 0; i < endGameStats.Num(); i++ ) {
					savedPlayerStat_t& stat = endGameStats[ i ];

					msg.WriteLong( stat.rank == NULL ? -1 : stat.rank->Index() );
					msg.WriteString( stat.name.c_str() );
					msg.WriteFloat( stat.value );
					msg.WriteFloat( stat.data[ target.GetClientNum() ] );
				}
				msg.Send( sdReliableMessageClientInfo( target.GetClientNum() ) );
			}
		}

		if ( target.SendToRepeaterClients() ) {
			sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_ENDGAMESTATS );
			msg.WriteLong( endGameStats.Num() );
			for ( int j = 0; j < endGameStats.Num(); j++ ) {
				savedPlayerStat_t& stat = endGameStats[ j ];

				msg.WriteLong( stat.rank == NULL ? -1 : stat.rank->Index() );
				msg.WriteString( stat.name.c_str() );
				msg.WriteFloat( stat.value );
				msg.WriteFloat( -1.f );
			}
			msg.Send( sdReliableMessageClientInfoRepeater( target.GetClientNum() ) );
		}
	}
}

/*
============
idGameLocal::ClearEndGameStats
============
*/
void idGameLocal::ClearEndGameStats( void ) {
	if ( endGameStats.Num() == 0 ) {
		return;
	}

	endGameStats.SetNum( 0, false );

	if ( gameLocal.isServer ) {
		sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_ENDGAMESTATS );
		msg.WriteLong( 0 );
		msg.Send( sdReliableMessageClientInfoAll() );
	}
}

/*
============
idGameLocal::OnEndGameStatsReceived
============
*/
void idGameLocal::OnEndGameStatsReceived( void ) {
	if ( endGameStats.Num() == 0 ) {
		if( sdUserInterfaceScope* scope = gameLocal.globalProperties.GetSubScope( "mapInfo" ) ) {
			if( sdProperties::sdProperty* property = scope->GetProperty( "statsReady", sdProperties::PT_FLOAT ) ) {
				*property->value.floatValue = 0.0f;
			}
		} else {
			gameLocal.Warning( "idGameLocal::OnEndGameStatsReceived: Couldn't find global 'mapInfo' scope in guiGlobals." );
		}
		return;
	}

	// normal end of game stuff
	if( sdUserInterfaceScope* scope = gameLocal.globalProperties.GetSubScope( "mapInfo" ) ) {
		if( sdProperties::sdProperty* property = scope->GetProperty( "statsReady", sdProperties::PT_FLOAT ) ) {
			// force a signal
			*property->value.floatValue = 0.0f;
			*property->value.floatValue = 1.0f;
		}
	} else {
		gameLocal.Warning( "idGameLocal::OnEndGameStatsReceived: Couldn't find global 'mapInfo' scope in guiGlobals." );
	}
}


/*
============
idGameLocal::ListClientEntities_f
============
*/
void idGameLocal::ListClientEntities_f(class idCmdArgs const & args ) {
	common->Printf( "-------------------------\n" );
	for (int i=0; i<MAX_CENTITIES; i++) {
		if ( gameLocal.clientEntities[i] ) {
			const char *name = gameLocal.clientEntities[i]->GetName();
			common->Printf( "%d '%s'\n", i, name ? name : "<unknown>" );
		}
	}
	common->Printf( "-------------------------\n" );
}

/*
============
idGameLocal::LoadMainMenuPartialMedia
============
*/
void idGameLocal::LoadMainMenuPartialMedia( bool blocking ) {
	if( networkSystem->IsDedicated() ) {
		return;
	}
	if( sdUserInterfaceLocal* mainMenu = GetUserInterface( uiMainMenuHandle ) ) {
		if( mainMenu->GetDecl() == NULL ) {
			assert( 0 );
			return;
		}

		const idStrList& models = mainMenu->GetDecl()->GetPartialLoadModels();
		for( int i = 0; i < models.Num(); i++ ) {
			idRenderModel* renderModel = NULL;

			const idDeclModelDef* modelDef = gameLocal.declModelDefType.LocalFind( models[ i ].c_str(), false );
			if ( modelDef != NULL ) {
				renderModel = modelDef->ModelHandle();
			} else {
				if( blocking ) {
					renderModel = renderModelManager->FindModel( models[ i ].c_str() );
				} else {
					renderModel = renderModelManager->GetModel( models[ i ].c_str() );
				}
			}

			if ( renderModel != NULL ) {
				renderModel->LoadPartialLoadableImages( blocking );
			}
		}
	}
}


/*
============
idGameLocal::PurgeMainMenuPartialMedia
============
*/
void idGameLocal::PurgeMainMenuPartialMedia() {
	if( networkSystem->IsDedicated() ) {
		return;
	}
	if( sdUserInterfaceLocal* mainMenu = GetUserInterface( uiMainMenuHandle ) ) {
		const idStrList& models = mainMenu->GetDecl()->GetPartialLoadModels();
		for( int i = 0; i < models.Num(); i++ ) {
			idRenderModel* renderModel = NULL;

			const idDeclModelDef* modelDef = gameLocal.declModelDefType.LocalFind( models[ i ].c_str(), false );
			if ( modelDef != NULL ) {
				renderModel = modelDef->ModelHandle();
			} else {
				renderModel = renderModelManager->GetModel( models[ i ].c_str() );
			}

			if ( renderModel != NULL ) {
				renderModel->PurgePartialLoadableImages();
			}
		}
	}
}

/*
============
idGameLocal::SetUpdateAvailability
============
*/
void idGameLocal::SetUpdateAvailability( updateAvailability_t type ) {
	if( !networkSystem->IsDedicated() ) {
		updateManager.SetAvailability( type );
	}
}

/*
============
idGameLocal::GetUpdateResponse
============
*/
guiUpdateResponse_t idGameLocal::GetUpdateResponse() {
	return updateManager.GetResponse();
}

/*
============
idGameLocal::SetUpdateProgress
============
*/
void idGameLocal::SetUpdateProgress( float percent ) {
	if( !networkSystem->IsDedicated() ) {
		updateManager.SetUpdateProgress( percent );
	}
}

/*
============
idGameLocal::SetUpdateState
============
*/
void idGameLocal::SetUpdateState( updateState_t state ) {
	if( !networkSystem->IsDedicated() ) {
		updateManager.SetUpdateState( state );
	}
}

/*
============
idGameLocal::SetUpdateFromServer
============
*/
void idGameLocal::SetUpdateFromServer( bool fromServer ) {
	if( !networkSystem->IsDedicated() ) {
		updateManager.SetUpdateFromServer( fromServer );
	}
}

/*
============
idGameLocal::AddSystemNotification
============
*/
void idGameLocal::AddSystemNotification( const wchar_t* text ) {
	if( !networkSystem->IsDedicated() && systemNotifications.Num() < ( sdNotificationSystem::MAX_NOTIFICATIONS - notificationSystem->GetNumNotifications() ) ) {
		systemNotifications.Append( idWStr( text ) );
	}
}

/*
============
idGameLocal::SetUpdateMessage
============
*/
void idGameLocal::SetUpdateMessage( const wchar_t* text ) {
	if( !networkSystem->IsDedicated() ) {
		updateManager.SetUpdateMessage( text );
	}
}

/*
============
idGameLocal::DrawLCD
============
*/
void idGameLocal::DrawLCD( sdLogitechLCDSystem* lcd ) {
	bool drawSplash = false;

	idPlayer* player = GetLocalPlayer();
	const sdDeclPlayerClass* cls = NULL;

	if ( player == NULL ) {
		drawSplash = true;
	} else if ( player->IsSpectator() ) {
		drawSplash = true;
	} else {
		cls = player->GetInventory().GetClass();
		if ( cls == NULL ) {
			drawSplash = true;
		}
	}

	if ( drawSplash ) {
		sdLogitechLCDSystem::objectHandle_t texture = lcd->GetImageHandle( "textures/lcd/etqw_logo.tga" );
		lcd->DrawImageHandle( texture, 0, 0 );
		return;
	}

	int offset[ 4 ][ 2 ] = {
		{ 0, 0 },
		{ 81, 0 },
		{ 0, 22 },
		{ 81, 22 },
	};

	sdLogitechLCDSystem::objectHandle_t starTexture = lcd->GetImageHandle( "textures/lcd/star.tga" );

	int count = Min( 4, cls->GetNumProficiencies() );
	for ( int i = 0; i < count; i++ ) {
		int profIndex = cls->GetProficiency( i ).index;
		const sdDeclProficiencyType* prof = gameLocal.declProficiencyTypeType[ profIndex ];

		const char* fileName = va( "textures/lcd/skill_%s.tga", prof->GetName() );

		sdLogitechLCDSystem::objectHandle_t texture = lcd->GetImageHandle( fileName );
		lcd->DrawImageHandle( texture, offset[ i ][ 0 ], offset[ i ][ 1 ] );

		sdProficiencyTable& table = player->GetProficiencyTable();

		int level = Min( 4, table.GetLevel( profIndex ) );
		for ( int j = 0; j < level; j++ ) {
			lcd->DrawImageHandle( starTexture, offset[ i ][ 0 ] + 17 + ( j * 15 ), offset[ i ][ 1 ] );
		}

		lcd->DrawFilledRect( offset[ i ][ 0 ] + 17, offset[ i ][ 1 ] + 15, 60, 6, sdLogitechLCDSystem::BC_WHITE );
		lcd->DrawFilledRect( offset[ i ][ 0 ] + 18, offset[ i ][ 1 ] + 16, 58, 4, sdLogitechLCDSystem::BC_BLACK );
		lcd->DrawFilledRect( offset[ i ][ 0 ] + 19, offset[ i ][ 1 ] + 17, 56 * table.GetPercent( profIndex ), 2, sdLogitechLCDSystem::BC_WHITE );
	}
}

/*
============
idGameLocal::AddChatLine
============
*/
void idGameLocal::AddChatLine( const wchar_t* text ) {
	if ( rules == NULL ) {
		return;
	}
	rules->AddChatLine( sdGameRules::CHAT_MODE_MESSAGE, colorWhite, L"%ls", text );
}

/*
============
idGameLocal::CreateStatusResponseDict
============
*/
void idGameLocal::CreateStatusResponseDict( const idDict& serverInfo, idDict& statusResponseDict ) {
	statusResponseDict.SetBool( "si_teamDamage", serverInfo.GetBool( "si_teamDamage" ) );
	statusResponseDict.SetBool( "si_needPass", serverInfo.GetBool( "si_needPass" ) );
	statusResponseDict.Set( "si_rules", serverInfo.GetString( "si_rules" ) );
	statusResponseDict.SetBool( "si_teamForceBalance", serverInfo.GetBool( "si_teamForceBalance" ) );
	statusResponseDict.SetBool( "si_allowLateJoin", serverInfo.GetBool( "si_allowLateJoin" ) );
}

/*
============
idGameLocal::WriteExtendedProbeData
============
*/
void idGameLocal::WriteExtendedProbeData( idBitMsg& msg ) {
	sdStatsTracker& tracker = sdGlobalStatsTracker::GetInstance();
	sdPlayerStatEntry* totalKillsStat = tracker.GetStat( tracker.AllocStat( "total_kills", sdNetStatKeyValue::SVT_INT ) );
	sdPlayerStatEntry* totalDeathsStat = tracker.GetStat( tracker.AllocStat( "total_deaths", sdNetStatKeyValue::SVT_INT ) );

	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = GetClient( i );
		if ( player == NULL ) {
			continue;
		}

		sdTeamInfo* team = player->GetGameTeam();

		msg.WriteByte( i );
		msg.WriteFloat( player->GetProficiencyTable().GetXP() );
		msg.WriteString( team != NULL ? team->GetLookupName() : "" );
		msg.WriteLong( totalKillsStat->GetValue( i ).GetInt() );
		msg.WriteLong( totalDeathsStat->GetValue( i ).GetInt() );
	}
	msg.WriteByte( MAX_CLIENTS );
}

/*
============
idGameLocal::GetProbeTime
============
*/
int idGameLocal::GetProbeTime() const {
	if( rules == NULL ) {
		return 0;
	}
	return rules->GetGameTime();
}

/*
============
idGameLocal::GetProbeState
============
*/
byte idGameLocal::GetProbeState() const {
	if( rules == NULL ) {
		return false;
	}
	return rules->GetProbeState();
}

/*
============
idGameLocal::GetRulesInstance
============
*/
sdGameRules* idGameLocal::GetRulesInstance( const char* type ) const {
	rulesMap_t::ConstIterator iter = rulesCache.Find( type );
	if( iter == rulesCache.End() ) {
		return NULL;
	}
	return iter->second;
}

/*
===============
idGameLocal::DownloadRequest
===============
*/
bool idGameLocal::DownloadRequest( const char* IP, const char* guid, const char* paks, char urls[ MAX_STRING_CHARS ] ) {
	if ( net_serverDownload.GetInteger() == 0 ) {
		return false;
	}

	if ( net_serverDownload.GetInteger() == 1 ) {
		const char* serverURL = si_serverURL.GetString();
		// 1: single URL redirect
		if ( idStr::Length( serverURL ) == 0 ) {
			common->Warning( "si_serverURL not set" );
			return false;
		}
		idStr::snPrintf( urls, MAX_STRING_CHARS, "1;%s", serverURL );
		return true;
	} else {
		// 2: table of pak URLs
		// 3: table of pak URLs with built-in http server
		// first token is the game pak if requested, empty if not requested by the client
		// there may be empty tokens for paks the server couldn't pinpoint - the order matters
		idStr 		reply = "2;";
		idStrList	dlTable, pakList;
		bool		matchAll = false;
		int			i, j;

		if ( !idStr::Cmp( net_serverDlTable.GetString(), "*" ) ) {
			matchAll = true;
		} else {
			idSplitStringIntoList( dlTable, net_serverDlTable.GetString(), ";" );
		}
		idSplitStringIntoList( pakList, paks, ";" );

		for ( i = 0; i < pakList.Num(); i++ ) {
			if ( i > 0 ) {
				reply += ";";
			}
			if ( pakList[ i ][ 0 ] == '\0' ) {
				if ( i == 0 ) {
					// pak 0 will always miss when client doesn't ask for game bin
					common->DPrintf( "no game pak request\n" );
				} else {
					common->DPrintf( "no pak %d\n", i );
				}
				continue;
			}

			idStr url = net_serverDlBaseURL.GetString();
			if ( url.Length() == 0 ) {
				if ( net_serverDownload.GetInteger() == 2 ) {
					common->Warning( "net_serverDownload == 2 and net_serverDlBaseURL not set" );
				} else {
					url = cvarSystem->GetCVarString( "net_httpServerBaseURL" );
				}
			}
			
			if ( matchAll ) {
				url.AppendPath( pakList[ i ] );
				reply += url;
				common->Printf( "download for %s: %s\n", IP, url.c_str() );
			} else {
				for ( j = 0; j < dlTable.Num(); j++ ) {
					if ( !pakList[ i ].Icmp( dlTable[ j ] ) ) {
						break;
					}
				}
				if ( j == dlTable.Num() ) {
					common->Printf( "download for %s: pak not matched: %s\n", IP, pakList[ i ].c_str() );
				} else {
					url.AppendPath( dlTable[ j ] );
					reply += url;
					common->Printf( "download for %s: %s\n", IP, url.c_str() );
				}
			}
		}

		idStr::Copynz( urls, reply, MAX_STRING_CHARS );
		return true;
	}
}

/*
===============
idGameLocal::HTTPRequest
===============
*/
bool idGameLocal::HTTPRequest( const char *IP, const char *file, bool isGamePak ) {
	idStrList	dlTable;
	int			i;

	if ( !idStr::Cmp( net_serverDlTable.GetString(), "*" ) ) {
		return true;
	}

	idSplitStringIntoList( dlTable, net_serverDlTable.GetString(), ";" );
	while ( *file == '/' ) ++file; // net_serverDlTable doesn't include the initial /

	for ( i = 0; i < dlTable.Num(); i++ ) {
		if ( !dlTable[ i ].Icmp( file ) ) {
			return true;
		}
	}

	return false;
}

/*
============
idGameLocal::EnablePlayerHeadModels
============
*/
void idGameLocal::EnablePlayerHeadModels( void ) {
	// enable the relevant player head models
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = GetClient( i );
		if ( player == NULL ) {
			continue;
		}
		bool enable = true;
		if ( player->IsSpectator() ) {
			enable = false;
		}
		if ( player->GetHealth() <= 0 ) {
			enable = false;
		}
		idEntity* proxy = player->GetProxyEntity();
		if ( proxy != NULL ) {
			if ( !proxy->GetUsableInterface()->GetAllowPlayerDamage( player ) ) {
				enable = false;
			}
		}
		if ( enable ) {
			player->GetPlayerPhysics().EnableHeadClipModel();
		} else {
			player->GetPlayerPhysics().DisableHeadClipModel();
		}
	}
}

/*
============
idGameLocal::DisablePlayerHeadModels
============
*/
void idGameLocal::DisablePlayerHeadModels( void ) {
	// disable the player head models
	for ( int i = 0; i < MAX_CLIENTS; i++ ) {
		idPlayer* player = GetClient( i );
		if ( player == NULL ) {
			continue;
		}
		player->GetPlayerPhysics().DisableHeadClipModel();
	}
}

/*
================
idGameLocal::GetDemoName
================
*/
idCVar g_autoDemoNameFormat( "g_autoDemoNameFormat", "$year$$month$$day$_$hour$$min$$sec$_$map$_$rules$_$name$_build_$srcrev$_$mediarev$.ndm", CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE, "demo name format: date - $year$, $month$, $day$   time - $hour$, $min$, $sec$   map name - $map$   ruleset info - $rules$   player name - $name$    versions - $srcrev$ $mediarev$" );

void idGameLocal::GetDemoName( idStr& output ) {
	idStr name = "na";
	if ( gameLocal.isClient ) {
		idPlayer* localPlayer = gameLocal.GetLocalPlayer();
		if ( localPlayer != NULL ) {
			name = localPlayer->userInfo.name;
			name.RemoveColors();
		}
	} else {
		name = "server";
	}

	// just in case
	name.ReplaceChar( '/', '_' );
	name.ReplaceChar( '\\', '_' );

	sysTime_t time;
	sys->RealTime( &time );

	idStr mapStr = mapFileName.c_str();
	mapStr.ReplaceChar( '/', '_' );
	mapStr.ReplaceFirst( "maps_", "" );		// the maps tag is boring. die.
	mapStr.StripFileExtension();
	

	output = g_autoDemoNameFormat.GetString();

	// date
	output.Replace( "$year$", va( "%d", 1900 + time.tm_year ) );
	output.Replace( "$month$", va( "%02d", 1 + time.tm_mon ) );
	output.Replace( "$day$", va( "%02d", time.tm_mday ) );

	// time
	output.Replace( "$hour$", va( "%02d", time.tm_hour ) );
	output.Replace( "$min$", va( "%02d", time.tm_min ) );
	output.Replace( "$sec$", va( "%02d", time.tm_sec ) );

	// other
	output.Replace( "$map$", mapStr.c_str() );
	output.Replace( "$rules$", rules->GetDemoNameInfo() );
	output.Replace( "$name$", name.c_str() );
	output.Replace( "$srcrev$", va( "%d", ENGINE_SRC_REVISION ) );
	output.Replace( "$mediarev$", va( "%d", ENGINE_MEDIA_REVISION ) );
}

/*
============
idGameLocal::StartAutorecording
============
*/
idCVar g_autoRecordDemos( "g_autoRecordDemos", "0", CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE, "automatically start & stop demos at the start & end of a map" );

void idGameLocal::StartAutorecording() {
	if ( sdDemoManager::GetInstance().InPlayBack() ) {
		return;
	}

	if ( g_autoRecordDemos.GetBool() && !isAutorecording ) {
		idStr demoName = "";
		gameLocal.GetDemoName( demoName );

		if ( demoName.Length() > 0 ) {
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va( "recordNetDemo \"%s\"\n", demoName.c_str() ) );
			isAutorecording = true;
		}
	}
}

/*
============
idGameLocal::StopAutorecording
============
*/
void idGameLocal::StopAutorecording() {
	if ( sdDemoManager::GetInstance().InPlayBack() ) {
		return;
	}

	if ( isAutorecording ) {
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "stopNetDemo\n" );
		isAutorecording = false;
	}
}

/*
================
idGameLocal::GetScoreboardShotName
================
*/
idCVar g_autoScreenshotNameFormat( "g_autoScreenshotNameFormat", "screenshots/scoreboard_$year$$month$$day$_$hour$$min$$sec$_$map$_$rules$_$name$_build_$srcrev$_$mediarev$.tga", CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE, "auto screenshot name format: date - $year$, $month$, $day$   time - $hour$, $min$, $sec$   map name - $map$   ruleset info - $rules$   player name - $name$    versions - $srcrev$ $mediarev$" );

void idGameLocal::GetScoreboardShotName( idStr& output ) {
	idStr name = "na";
	if ( gameLocal.isClient ) {
		idPlayer* localPlayer = gameLocal.GetLocalPlayer();
		if ( localPlayer != NULL ) {
			name = localPlayer->userInfo.name;
			name.RemoveColors();
		}
	} else {
		name = "server";
	}
	
	// just in case
	name.ReplaceChar( '/', '_' );
	name.ReplaceChar( '\\', '_' );

	sysTime_t time;
	sys->RealTime( &time );

	idStr mapStr = mapFileName.c_str();
	mapStr.ReplaceFirst( "maps_", "" );		// the maps tag is boring. die.
	mapStr.ReplaceChar( '/', '_' );
	mapStr.StripFileExtension();
	

	output = g_autoScreenshotNameFormat.GetString();

	// date
	output.Replace( "$year$", va( "%d", 1900 + time.tm_year ) );
	output.Replace( "$month$", va( "%02d", 1 + time.tm_mon ) );
	output.Replace( "$day$", va( "%02d", time.tm_mday ) );

	// time
	output.Replace( "$hour$", va( "%02d", time.tm_hour ) );
	output.Replace( "$min$", va( "%02d", time.tm_min ) );
	output.Replace( "$sec$", va( "%02d", time.tm_sec ) );

	// other
	output.Replace( "$map$", mapStr.c_str() );
	output.Replace( "$rules$", rules->GetDemoNameInfo() );
	output.Replace( "$name$", name.c_str() );
	output.Replace( "$srcrev$", va( "%d", ENGINE_SRC_REVISION ) );
	output.Replace( "$mediarev$", va( "%d", ENGINE_MEDIA_REVISION ) );
}

/*
============
idGameLocal::OnEndGameScoreboardActive
============
*/
idCVar g_autoScreenshot( "g_autoScreenshot", "0", CVAR_BOOL | CVAR_GAME | CVAR_PROFILE | CVAR_ARCHIVE, "automatically take a screenshot of the scoreboard at the end of a map" );

void idGameLocal::OnEndGameScoreboardActive() {
	if ( !hasTakenScoreShot && g_autoScreenshot.GetBool() ) {
		idStr shotName;
		GetScoreboardShotName( shotName );
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va( "screenshot \"%s\"\n", shotName.c_str() ) );
		hasTakenScoreShot = true;
	}
}

/*
============
idGameLocal::IsMultiPlayer
============
*/
bool idGameLocal::IsMultiPlayer( void ) {
	return isClient || networkSystem->IsDedicated();
}

/*
============
idGameLocal::IsMetaDataValidForPlay
============
*/
bool idGameLocal::IsMetaDataValidForPlay( const metaDataContext_t& context, bool checkBrowserStatus ) {
	const idDict& dict = *context.meta;
	if ( checkBrowserStatus ) {
		if ( !dict.GetBool( "show_in_browser" ) ) {
			return false;
		}
	}

	if ( gameLocal.IsMultiPlayer() ) {
		if ( dict.GetBool( "sp_only" ) ) {
			return false;
		}
	} else {
		if ( dict.GetBool( "mp_only" ) ) {
			return false;
		}
	}

	return true;
}

/*
============
idGameLocal::ChangeLocalSpectateClient
============
*/
void idGameLocal::ChangeLocalSpectateClient( int spectateeNum ) {
	if ( serverIsRepeater ) {
		repeaterClientFollowIndex = spectateeNum;
		idWeapon::UpdateWeaponVisibility();
		OnLocalViewPlayerChanged();
	} else if ( isClient ) {
		sdReliableClientMessage outMsg( GAME_RELIABLE_CMESSAGE_SETSPECTATECLIENT );
		outMsg.WriteByte( spectateeNum );
		outMsg.Send();
	} else {
		idPlayer* localPlayer = GetLocalPlayer();
		if ( localPlayer != NULL ) {
			localPlayer->OnSetClientSpectatee( GetClient( spectateeNum ) );
		}
	}
}

/*
============
idGameLocal::GetRepeaterFollowClientIndex
============
*/
int idGameLocal::GetRepeaterFollowClientIndex( void ) const {
	if ( repeaterClientFollowIndex < -1 || repeaterClientFollowIndex >= MAX_ASYNC_CLIENTS ) {
		repeaterClientFollowIndex = -1;
		idWeapon::UpdateWeaponVisibility();
	}
	if ( repeaterClientFollowIndex != -1 ) {
		if ( GetClient( repeaterClientFollowIndex ) == NULL ) {
			repeaterClientFollowIndex = -1;
			idWeapon::UpdateWeaponVisibility();
		}
	}
	return repeaterClientFollowIndex;
}

#ifndef _XENON
/*
============
idGameLocal::UpdateGameSession
============
*/
void idGameLocal::UpdateGameSession( void ) {
	// handle game session advertising
	if ( gameLocal.GetLocalPlayer() == NULL && sdnet.NeedsGameSession() ) {
		if ( networkService->GetState() == sdNetService::SS_INITIALIZED ) {
			sdnet.Connect();
		} else if ( networkService->GetState() == sdNetService::SS_ONLINE ) {
			if ( networkService->GetDedicatedServerState() == sdNetService::DS_OFFLINE ) {
				// if ranked, don't automatically try to connect anymore if we've had a duplicate auth
				// for dedicated servers using the key distribution, report and request a different key (TTimo)
				if ( !networkSystem->IsRankedServer() || networkService->GetDisconnectReason() != sdNetService::DR_DUPLICATE_AUTH ) {
					sdnet.SignInDedicated();
				}
			} else if ( networkService->GetDedicatedServerState() == sdNetService::DS_ONLINE ) {
				if ( !sdnet.HasGameSession() ) {
					reservedClientSlots.Clear(); // the session ID will change, so old invites aren't valid anymore anyway
					sdnet.StartGameSession();
				} else {
					sdnet.UpdateGameSession( false, true );
				}
			}
		}
	}
}
#endif // _XENON

/*
============
idGameLocal::StartSendingBanList
============
*/
void idGameLocal::StartSendingBanList( idPlayer* player ) {
	if ( player == NULL || gameLocal.IsLocalPlayer( player ) ) {
		guidFile.ListBans();
		return;
	}

	clientLastBanIndexReceived[ player->entityNumber ] = -1;

	SendBanList( player );
}

/*
============
idGameLocal::SendBanList
============
*/
void idGameLocal::SendBanList( idPlayer* player ) {
	assert( player != NULL );

	sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_BANLISTMESSAGE );
	bool done = guidFile.WriteBans( clientLastBanIndexReceived[ player->entityNumber ], msg );
	msg.Send( sdReliableMessageClientInfo( player->entityNumber ) );
	if ( done ) {
		clientLastBanIndexReceived[ player->entityNumber ] = -1;

		sdReliableServerMessage msg( GAME_RELIABLE_SMESSAGE_BANLISTFINISHED );
		msg.Send( sdReliableMessageClientInfo( player->entityNumber ) );
	}
}
