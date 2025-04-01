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

#pragma warning(disable : 4127 4996 4805 4800)



#include "Game_local.h"
#include "DarkModGlobals.h"
#include "darkModLAS.h"
#include "declxdata.h"
#include "Grabber.h"
#include "Relations.h"
#include "Inventory/Inventory.h"
#include "SndProp.h"
#include "ai/AAS_local.h"
#include "StimResponse/StimResponseCollection.h"
#include "Objectives/MissionData.h"
#include "Objectives/CampaignStatistics.h"
#include "MultiStateMover.h"
#include "Func_Shooter.h"
#include "Shop/Shop.h"
#include "EscapePointManager.h"
#include "DownloadMenu.h"
#include "TimerManager.h"
#include "ai/Conversation/ConversationSystem.h"
#include "../idlib/RevisionTracker.h"
#include "Missions/MissionManager.h"
#include "Missions/DownloadManager.h"
#include "Http/HttpConnection.h"
#include "Http/HttpRequest.h"
#include "StimResponse/StimType.h" // grayman #2721
#include "StdString.h"
#include "framework/Session_local.h"
#include "framework/Common.h"
#include "LightEstimateSystem.h"

#include <chrono>
#include <iostream>

#ifdef max
#undef max
#endif

CGlobal g_Global;

extern CMissionData		g_MissionData;
extern CsndPropLoader	g_SoundPropLoader;
extern CsndProp			g_SoundProp;

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

#endif

idRenderWorld *				gameRenderWorld = NULL;		// all drawing is done to this world
idSoundWorld *				gameSoundWorld = NULL;		// all audio goes to this world
idSoundWorld *				gameSoundWorldBuf = NULL;

static gameExport_t			gameExport;

// global animation lib
idAnimManager				animationLib;

// the rest of the engine will only reference the "game" variable, while all local aspects stay hidden
idGameLocal					gameLocal;
idGame *					game = &gameLocal;	// statically pointed at an idGameLocal

const char *idGameLocal::surfaceTypeNames[ MAX_SURFACE_TYPES ] = {
	"none",	"metal", "stone", "flesh", "wood", "cardboard", "liquid", "glass", "plastic",
	"ricochet", "surftype10", "surftype11", "surftype12", "surftype13", "surftype14", "surftype15"
};

/* This list isn't actually used by the code, it's here just for reference. The code
   accepts any first word in the description as the surface type name: */
// grayman - #1421/1422 - added "hardwood"
const char *idGameLocal::m_NewSurfaceTypes[ MAX_SURFACE_TYPES * 2 + 1] = {
	"tile", "carpet", "dirt", "gravel", "grass", "rock", "twigs", "foliage", "sand", "mud",
	"brokeglass", "snow", "ice", "squeakboard", "puddle", "moss", "cloth", "ceramic", "slate",
	"straw", "armor_leath", "armor_chain", "armor_plate", "climbable", "paper","hardwood"
};

void PrintMessage( int x, int y, const char *szMessage, idVec4 colour, fontInfoEx_t &font )
{
      renderSystem->SetColor( colour );

      for( const char *p = szMessage; *p; p++ )
      {
            glyphInfo_t &glyph = font.fontInfoSmall.glyphs[int(*p)];

            renderSystem->DrawStretchPic( x, y - glyph.top,
                  glyph.imageWidth, glyph.imageHeight,
                  glyph.s, glyph.t, glyph.s2, glyph.t2,
                  glyph.glyph );

            x += glyph.xSkip;
      }
}

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
		renderSystem				= import->renderSystem;
		soundSystem					= import->soundSystem;
		renderModelManager			= import->renderModelManager;
		uiManager					= import->uiManager;
		declManager					= import->declManager;
		AASFileManager				= import->AASFileManager;
		collisionModelManager		= import->collisionModelManager;
	}
	else {
		// Wrong game version, throw a meaningful error rather than leaving
		// stuff initialised and getting segfaults.
		// The old dll (pre v1.08) will output something like:
		// "Ensure the correct Doom 3 patches are installed." which is misleading.
		std::cerr << "FATAL: Incorrect game version: required " 
			<< GAME_API_VERSION << ", got " << import->version << "\n"
			<< "Use the latest TheDarkMod executable with the latest game DLL." << std::endl;
		abort();
	}

	// set interface pointers used by idLib
	idLib::sys				= sys;
	idLib::common				= common;
	idLib::cvarSystem			= cvarSystem;
	idLib::fileSystem			= fileSystem;

	// setup export interface
	gameExport.version = GAME_API_VERSION;
	gameExport.game = game;
	gameExport.gameEdit = gameEdit;

	// Initialize logging and all the global stuff for darkmod
	g_Global.Init();

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

	testImport.version = GAME_API_VERSION;
	testImport.sys						= ::sys;
	testImport.common					= ::common;
	testImport.cmdSystem				= ::cmdSystem;
	testImport.cvarSystem				= ::cvarSystem;
	testImport.fileSystem				= ::fileSystem;
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
idGameLocal::idGameLocal() :
	postMissionScreenActive(false),
	briefingVideoInfoLoaded(false),
	curBriefingVideoPart(-1),
	m_MissionResult(MISSION_NOTEVENSTARTED),
	m_HighestSRId(0),
	m_searchManager(NULL), // grayman #3857
	activeEntities(&idEntity::activeIdx)
{
	entities.SetNum( MAX_GENTITIES );
	spawnIds.SetNum( MAX_GENTITIES );
	Clear();
}

/*
===========
idGameLocal::~idGameLocal
============
*/
idGameLocal::~idGameLocal() 
{
}

/*
===========
idGameLocal::Clear
============
*/
void idGameLocal::Clear( void )
{
	m_HighestSRId = 0;
	m_StimTimer.Clear();
	m_Timer.Clear();
	m_StimEntity.Clear();
	m_RespEntity.Clear();

	m_sndPropLoader = &g_SoundPropLoader;
	m_sndProp = &g_SoundProp;
	m_RelationsManager = CRelationsPtr();
	m_MissionData.reset();

	mainMenuExited = false;
	briefingVideo.Clear();
	debriefingVideo.Clear();

	m_Grabber = NULL;
	m_DifficultyManager.Clear();

	m_ModMenu.reset();
	m_DownloadMenu.reset();
	m_DownloadManager.reset();
	m_Shop.reset();

	m_TriggerFinalSave = false;

	m_GUICommandStack.Clear();
	m_GUICommandArgs = 0;

	m_guiError.Clear();

	m_AreaManager.Clear();
	m_ConversationSystem.reset();

	if (m_ModelGenerator)
	{
		m_ModelGenerator->Clear();
	}
	if (m_LightController)
	{
		m_LightController->Clear();
	}
	
#ifdef TIMING_BUILD
	debugtools::TimerManager::Instance().Clear();
#endif

	m_EscapePointManager = CEscapePointManager::Instance();
	m_EscapePointManager->Clear();

	m_searchManager = CSearchManager::Instance(); // grayman #3857
	m_searchManager->Clear(); // grayman #3857
	
	m_Interleave = 0;

	m_lightGem.Clear();

	m_uniqueMessageTag = 0; // grayman #3355

	m_spyglassOverlay = 0; // grayman #3807 - no need to save/restore

	m_peekOverlay = 0; // grayman #4882 - no need to save/restore

	m_InterMissionTriggers.Clear();
	
	serverInfo.Clear();
	numClients = 0;
		userInfo.Clear();
		persistentPlayerInfo.Clear();
	memset( &usercmds, 0, sizeof( usercmds ) );
	memset( entities.Ptr(), 0, entities.MemoryUsed() );
	memset( spawnIds.Ptr(), -1, spawnIds.MemoryUsed() );
	firstFreeIndex = 0;
	num_entities = 0;
	spawnedEntities.Clear();
	activeEntities.Clear();
	spawnedAI.Clear();
	numEntitiesToDeactivate = 0;
	persistentLevelInfo.Clear();
	persistentLevelInfoLocation = PERSISTENT_LOCATION_NOWHERE;
	persistentPlayerInventory.reset();
	campaignInfoEntities.Clear();

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
	entityHash.ClearFree( 1024, MAX_GENTITIES );
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
	spawnArgs.Clear();
	gravity.Set( 0, 0, -1 );
	playerPVS.h = (unsigned int)-1;
	playerConnectedAreas.h = (unsigned int)-1;
	gamestate = GAMESTATE_UNINITIALIZED;
	skipCinematic = false;
	influenceActive = false;

	localClientNum = 0;
	realClientTime = 0;
	isNewFrame = true;
	clientSmoothing = 0.1f;
	entityDefBits = 0;

	nextGibTime = 0;
	globalMaterial = NULL;
	newInfo.Clear();
	lastGUIEnt = NULL;
	lastGUI = 0;

	/*memset( clientEntityStates, 0, sizeof( clientEntityStates ) );
	memset( clientPVS, 0, sizeof( clientPVS ) );
	memset( clientSnapshots, 0, sizeof( clientSnapshots ) );*/

	memset( lagometer, 0, sizeof( lagometer ) );

	portalSkyEnt			= NULL;
	portalSkyActive			= false;
	playerOldEyePos.Zero();			 // grayman #3108 - contributed by 7318
	globalPortalSky			= false; // grayman #3108 - contributed by 7318

	//	ResetSlowTimeVars();

	m_GamePlayTimer.Clear();

	musicSpeakers.Clear();

	m_suspiciousEvents.Clear(); // grayman #3424

	m_ambientLights.Clear();	// grayman #3584

}

// grayman #3807
int idGameLocal::DetermineAspectRatio()
{
	int viewX = r_customWidth.GetInteger();
	int viewY = r_customHeight.GetInteger();
	if (viewX <= 0 || viewY <= 0)
		Error( "idGameLocal::CalcFov: screen dimensions are %d x %d <= 0", viewX, viewY );
	float ratio_fov = float(viewX) / float(viewY);

	// There are several overlays, one for each of these aspect ratios: 5:4, 4:3, 16:10, 16:9 TV, 16:9, 21:9.
	// Converting those to a single ratio gives us: 1.25, 1.333333, 1.6, 1.770833, 1.777778, 2.333333
	// To match an overlay to a given 'ratio_fov', we'll assume that halfway between two
	// ratios, we'll switch from the lower aspect ratio to the higher.
	int result = 0;
	if (ratio_fov < (1.25 + (1.333333 - 1.25) / 2)) //1.2916665
		result = 3; // 5:4
	else if (ratio_fov < (1.333333 + (1.6 - 1.333333) / 2)) //1.4666665
		result = 0; // 4:3
	else if (ratio_fov < (1.6 + (1.770833 - 1.6) / 2)) //1.6854165
		result = 2; // 16:10
	else if (ratio_fov < (1.770833 + (1.777778 - 1.770833) / 2)) //1.770833
		result = 4; // 16:9 TV
	else if (ratio_fov < (1.777778 + (2.333333 - 1.777778) / 2)) //2.0555555
		result = 1; // 16:9
	else
		result = 5; // 21:9

	return result;
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

	// BluePill #4539 - show whether this is a 32-bit or 64-bit binary
	Printf( "%s %d.%02d/%zu, %s, code revision %d\n", 
 		GAME_VERSION, 
		TDM_VERSION_MAJOR,
		TDM_VERSION_MINOR,
		sizeof(void*) * 8,
		BUILD_STRING,
		RevisionTracker::Instance().GetHighestRevision() 
	);
	Printf( "Build date: %s\n", __DATE__ );
	
	// register game specific decl types
	declManager->RegisterDeclType( "model",				DECL_MODELDEF,		idDeclAllocator<idDeclModelDef> );
	declManager->RegisterDeclType( "export",			DECL_MODELEXPORT,	idDeclAllocator<idDecl> );
	// TDM specific DECLs
	declManager->RegisterDeclType( "xdata",				DECL_XDATA,			idDeclAllocator<tdmDeclXData> );

	// register game specific decl folders
	declManager->RegisterDeclFolder( "def",				".def",				DECL_ENTITYDEF );
	declManager->RegisterDeclFolder( "fx",				".fx",				DECL_FX );
	declManager->RegisterDeclFolder( "particles",		".prt",				DECL_PARTICLE );
	declManager->RegisterDeclFolder( "af",				".af",				DECL_AF );
	// TDM specific DECLs
	declManager->RegisterDeclFolder( "xdata",			".xd",				DECL_XDATA );

	cmdSystem->AddCommand( "listModelDefs", idListDecls_f<DECL_MODELDEF>, CMD_FL_SYSTEM|CMD_FL_GAME, "lists model defs" );
	cmdSystem->AddCommand( "printModelDefs", idPrintDecls_f<DECL_MODELDEF>, CMD_FL_SYSTEM|CMD_FL_GAME, "prints a model def", idCmdSystem::ArgCompletion_Decl<DECL_MODELDEF> );

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

	gamestate = GAMESTATE_NOMAP;

	Printf( "...%d aas types\n", aasList.Num() );
	Printf( "game initialized.\n" );
	Printf( "--------------------------------------\n" );
	Printf( "Parsing material files\n" );

	m_MissionData = CMissionDataPtr(new CMissionData);
	m_CampaignStats = CampaignStatsPtr(new CampaignStats);
	m_RelationsManager = CRelationsPtr(new CRelations);
	m_ModMenu = CModMenuPtr(new CModMenu);
	m_DownloadMenu = CDownloadMenuPtr(new CDownloadMenu);
	m_DownloadManager = CDownloadManagerPtr(new CDownloadManager);
	m_ConversationSystem = ai::ConversationSystemPtr(new ai::ConversationSystem);
	
	// load the soundprop globals from the def file
	m_sndPropLoader->GlobalsFromDef();

	//FIX: pm_walkspeed keeps getting reset whenever a map loads.
	// Copy the old value here and set it when the map starts up.
	m_walkSpeed = pm_walkspeed.GetFloat();

	// grayman #3355 - Initialize the AI message tag
	m_uniqueMessageTag = 0;

	// Initialise the mission manager
	m_MissionManager = CMissionManagerPtr(new CMissionManager);
	m_MissionManager->Init();

	// Initialise the model generator
	m_ModelGenerator = CModelGeneratorPtr(new CModelGenerator);
	m_ModelGenerator->Init();

	// Initialise the light controller
	m_LightController = CLightControllerPtr(new CLightController);
	m_LightController->Init();

	m_LightEstimateSystem = new LightEstimateSystem();

	// greebo: Create the persistent inventory - will be handled by game state changing code
	persistentPlayerInventory.reset(new CInventory);

	m_Shop = CShopPtr(new CShop);
	m_Shop->Init();

	// Construct a new http connection object
	if (cv_tdm_allow_http_access.GetBool())
	{
		m_HttpConnection = CHttpConnectionPtr(new CHttpConnection);
	}

	// grayman #3807 - set spyglass overlay per aspect ratio
	int ar = DetermineAspectRatio();
	m_spyglassOverlay = ar;

	// grayman #4882 - set peeking overlay per aspect ratio
	m_peekOverlay = ar;
}

const idStr& idGameLocal::GetMapFileName() const
{
	return mapFileName;
}

void idGameLocal::AddInterMissionTrigger(int missionNum, const idStr& activatorName, const idStr& targetName)
{
	InterMissionTrigger& trigger = m_InterMissionTriggers.Alloc();

	trigger.missionNum = missionNum;
	trigger.activatorName = activatorName;
	trigger.targetName = targetName;
}

#if 0
void idGameLocal::CheckTDMVersion()
{
	GuiMessage msg;
	msg.type = GuiMessage::MSG_OK;
	msg.okCmd = "close_msg_box";

	if (m_HttpConnection == NULL) 
	{
		Printf("HTTP requests disabled, skipping TDM version check.\n");

		msg.title = common->Translate( "#str_02136" );
		msg.message = common->Translate( "#str_02141" );	// HTTP Requests have been disabled,\n cannot check for updates.

		AddMainMenuMessage(msg);
		return;
	}

	idStr url = cv_tdm_version_check_url.GetString();
	Printf("Checking %s\n", url.c_str() );
	CHttpRequestPtr req = m_HttpConnection->CreateRequest( url.c_str() );

	req->Perform();

	// Check Request Status
	if (req->GetStatus() != CHttpRequest::OK)
	{
		Printf("%s\n", common->Translate( "#str_02002") );	// Connection Error.

		msg.title = common->Translate( "#str_02136" );		// Version Check Failed
		msg.message = common->Translate( "#str_02007" );	// Cannot connect to server.

		AddMainMenuMessage(msg);
		return;
	}

	XmlDocumentPtr doc = req->GetResultXml();

	pugi::xpath_node node = doc->select_node("//tdm/currentVersion");

	if (node)
	{
		int major = node.node().attribute("major").as_int();
		int minor = node.node().attribute("minor").as_int();

		msg.title = va( common->Translate( "#str_02132" ), major, minor );	// Most recent version is: 

		switch (CompareVersion(TDM_VERSION_MAJOR, TDM_VERSION_MINOR, major, minor))
		{
		case EQUAL:
			// "Your version %d.%02d is up to date."
			msg.message = va( common->Translate( "#str_02133"), TDM_VERSION_MAJOR, TDM_VERSION_MINOR);
			break;
		case OLDER:
			// "Your version %d.%02d needs updating."
			msg.message = va( common->Translate( "#str_02134"), TDM_VERSION_MAJOR, TDM_VERSION_MINOR);
			break;
		case NEWER:
			// "Your version %d.%02d is newer than the most recently published one."
			msg.message = va( common->Translate( "#str_02135"), TDM_VERSION_MAJOR, TDM_VERSION_MINOR);
			break;
		};
	}
	else
	{
		msg.title = common->Translate( "#str_02136" );	// "Version Check Failed"
		msg.message = common->Translate( "#str_02137" );	// "Couldn't find current version tag."

	}

	AddMainMenuMessage(msg);
}
#endif

void idGameLocal::AddMainMenuMessage(const GuiMessage& message)
{
	m_GuiMessages.Append(message);
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
	
	MapShutdown();

	// greebo: De-allocate the missiondata singleton, this is not 
	// done in MapShutdown() (needed for mission statistics)
	m_MissionData.reset();
	m_CampaignStats.reset();

	// Destroy the conversation system
	m_ConversationSystem.reset();

	// Destroy the mission manager
	m_MissionManager.reset();

	// Destroy the model generator
	m_ModelGenerator.reset();

	// Destroy the light controller
	m_LightController.reset();

	delete m_LightEstimateSystem;
	m_LightEstimateSystem = nullptr;

	// Clear http connection
	m_HttpConnection.reset();
	m_GuiMessages.ClearFree();

	aasList.DeleteContents( true );
	aasNames.ClearFree();

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

	// shut down globals (make sure to remove images from there)
	g_Global.Shutdown();

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

	idSaveGame savegame( f );

	if (g_flushSave.GetBool( ) == true ) { 
		// force flushing with each write... for tracking down
		// save game bugs.
		f->ForceFlush();
	}

	savegame.WriteHeader();

	// #5453: save mission overrides of all cvars
	idDict cvarOverrides = cvarSystem->GetMissionOverrides();
	savegame.WriteDict( &cvarOverrides );

	// go through all entities and threads and add them to the object list
	for (i = 0; i < MAX_GENTITIES; i++)
	{
		idEntity* ent = entities[i];

		if (ent == NULL) continue;

		// greebo: Give all entities a chance to register sub-objects (to resolve issue #2502)
		ent->AddObjectsToSaveGame(&savegame);

		if (ent->GetTeamMaster() && ent->GetTeamMaster() != ent)
		{
			continue; // skip bindslaves, these are added by the team master
		}

		for (idEntity* link = ent; link != NULL; link = link->GetNextTeamEntity())
		{
			savegame.AddObject( link );
		}
	}

	idList<idThread *> threads;
	threads = idThread::GetThreads();

	for( i = 0; i < threads.Num(); i++ ) {
		savegame.AddObject( threads[i] );
	}

	// DarkMod: Add darkmod specific objects here:

	savegame.AddObject( m_Grabber );

	// write out complete object list
	savegame.WriteObjectList();

	program.Save( &savegame );

	// Save the global hiding spot search collection
	CHidingSpotSearchCollection::Instance().Save(&savegame);

	// Save our grabber pointer
	savegame.WriteObject(m_Grabber);

	// Save whatever the model generator needs
	m_ModelGenerator->Save(&savegame);

	// Save whatever the light controller needs
	m_LightController->Save(&savegame);

	m_DifficultyManager.Save(&savegame);

	for ( i = 0; i < NumAAS(); i++)
	{
		idAAS* aas = GetAAS(i);
		if (aas != NULL)
		{
			aas->Save(&savegame);
		}
	}

	m_GamePlayTimer.Save(&savegame);
	m_AreaManager.Save(&savegame);
	m_ConversationSystem->Save(&savegame);
	m_RelationsManager->Save(&savegame);
	m_Shop->Save(&savegame);
	LAS.Save(&savegame);
	m_MissionManager->Save(&savegame);

#ifdef TIMING_BUILD
	debugtools::TimerManager::Instance().Save(&savegame);
#endif

	savegame.WriteDict( &serverInfo );

	savegame.WriteInt( numClients );
		savegame.WriteDict( &userInfo );
		savegame.WriteUsercmd( usercmds );
		savegame.WriteDict( &persistentPlayerInfo );

	for( i = 0; i < MAX_GENTITIES; i++ ) {
		savegame.WriteObject( entities[ i ] );
		savegame.WriteInt( spawnIds[ i ] );
	}

	savegame.WriteInt( firstFreeIndex );
	savegame.WriteInt( num_entities );

	// enityHash is restored by idEntity::Restore setting the entity name.

	savegame.WriteObject( world );

	savegame.WriteInt( spawnedEntities.Num() );
	for (idEntity* ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		savegame.WriteObject( ent );
	}

	activeEntities.Save( savegame );

	lodSystem.Save( savegame );

	m_LightEstimateSystem->Save(&savegame);

	// tels: save the list of music speakers
	savegame.WriteInt( musicSpeakers.Num() );
	for( i = 0; i < musicSpeakers.Num(); i++ ) {
		savegame.WriteInt( musicSpeakers[ i ] );
	}

	// grayman #3424 - save the list of suspicious events
	savegame.WriteInt(m_suspiciousEvents.Num());
	for ( int i = 0 ; i < m_suspiciousEvents.Num() ; i++ )
	{
		SuspiciousEvent se = m_suspiciousEvents[i];

		savegame.WriteInt(static_cast<int>(se.type));
		savegame.WriteVec3(se.location);
		se.entity.Save(&savegame);
		savegame.WriteInt(se.time); // grayman #3857
	}

	// grayman #3584

	savegame.WriteInt( m_ambientLights.Num() );
	for ( int i = 0 ; i < m_ambientLights.Num() ; i++ )
	{
		m_ambientLights[i].Save(&savegame);
	}
		
	savegame.WriteInt(spawnedAI.Num());
	for (idAI* ai = spawnedAI.Next(); ai != NULL; ai = ai->aiNode.Next()) {
		savegame.WriteObject(ai);
	}

	savegame.WriteInt( numEntitiesToDeactivate );
	savegame.WriteDict( &persistentLevelInfo );

	persistentPlayerInventory->Save(&savegame);

	savegame.WriteInt(campaignInfoEntities.Num());
	for (i = 0; i < campaignInfoEntities.Num(); ++i)
	{
		savegame.WriteObject(campaignInfoEntities[i]);
	}
	
	for( i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ ) {
		savegame.WriteFloat( globalShaderParms[ i ] );
	}

	savegame.WriteBool(postMissionScreenActive);

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

	savegame.WriteInt( framenum );
	savegame.WriteInt( previousTime );
	savegame.WriteInt( time );

	savegame.WriteInt( vacuumAreaNum );

	savegame.WriteInt( entityDefBits );

	savegame.WriteInt( localClientNum );

	// snapshotEntities is used for multiplayer only

	savegame.WriteInt( realClientTime );
	savegame.WriteBool( isNewFrame );
	savegame.WriteFloat( clientSmoothing );

	portalSkyEnt.Save( &savegame );

	savegame.WriteBool( portalSkyActive );

	// grayman #3108 - contributed by 7318
	savegame.WriteBool( globalPortalSky );
	savegame.WriteInt( portalSkyScale ); // grayman #3582
	savegame.WriteInt( currentPortalSkyType );
	savegame.WriteVec3( playerOldEyePos );
	savegame.WriteVec3( portalSkyGlobalOrigin );
	savegame.WriteVec3( portalSkyOrigin );
	// end 7318

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

	savegame.WriteDict( &spawnArgs );

	savegame.WriteInt( playerPVS.i );
	savegame.WriteInt( playerPVS.h );
	savegame.WriteInt( playerConnectedAreas.i );
	savegame.WriteInt( playerConnectedAreas.h );

	savegame.WriteVec3( gravity );

	// gamestate is not saved, instead if is just set to ACTIVE upon Restore()

	savegame.WriteBool( influenceActive );
	savegame.WriteInt( nextGibTime );

	//Save LightGem - J.C.Denton
	m_lightGem.Save( savegame );

	savegame.WriteInt( m_uniqueMessageTag ); // grayman #3355

	savegame.WriteInt(m_InterMissionTriggers.Num());
	for (int i = 0; i < m_InterMissionTriggers.Num(); ++i)
	{
		savegame.WriteInt(m_InterMissionTriggers[i].missionNum);
		savegame.WriteString(m_InterMissionTriggers[i].activatorName);
		savegame.WriteString(m_InterMissionTriggers[i].targetName);
	}

	m_sndProp->Save(&savegame);
	m_MissionData->Save(&savegame);
	savegame.WriteInt(static_cast<int>(m_MissionResult));

	m_CampaignStats->Save(&savegame);

	savegame.WriteInt(m_HighestSRId);

	savegame.WriteInt(m_Timer.Num());
	for (int i = 0; i < m_Timer.Num(); i++)
	{
		m_Timer[i]->Save(&savegame);
	}

	savegame.WriteInt(m_StimTimer.Num());
	for (int i = 0; i < m_StimTimer.Num(); i++)
	{
		savegame.WriteInt(m_StimTimer[i]->GetUniqueId());
	}

	savegame.WriteInt(m_StimEntity.Num());
	for (int i = 0; i < m_StimEntity.Num(); i++)
	{
		m_StimEntity[i].Save(&savegame);
	}

	savegame.WriteInt(m_RespEntity.Num());
	for (int i = 0; i < m_RespEntity.Num(); i++)
	{
		m_RespEntity[i].Save(&savegame);
	}

	m_EscapePointManager->Save(&savegame);

	m_searchManager->Save(&savegame); // grayman #3857

	// greebo: Save the maximum frob distance
	savegame.WriteFloat(g_Global.m_MaxFrobDistance);

	// spawnSpots
	// initialSpots
	// currentInitialSpot
	// newInfo
	// makingBuild
	// shakeSounds

	// write out pending events
	idEvent::Save( &savegame );

	// Tels: Save the GUI command stack
	savegame.WriteInt( m_GUICommandArgs );
	int cmds = m_GUICommandStack.Num();
	savegame.WriteInt( cmds );
	for( i = 0; i < cmds; i++ ) {
		savegame.WriteString( m_GUICommandStack[i] );
	}
	
	// greebo: Close the savegame, this will invoke a recursive Save on all registered objects
	savegame.Close();

	// save all accumulated cache to file
	savegame.FinalizeCache();

	// Send a message to the HUD
	GetLocalPlayer()->SendHUDMessage("#str_02916");	// "Game Saved"
}

/*
===========
idGameLocal::GetPersistentPlayerInfo
============
*/
const idDict &idGameLocal::GetPersistentPlayerInfo( int clientNum ) {
	idEntity	*ent;

	persistentPlayerInfo.Clear();
	ent = entities[ clientNum ];
	if ( ent && ent->IsType( idPlayer::Type ) ) {
		static_cast<idPlayer *>(ent)->SavePersistentInfo();
	}

	return persistentPlayerInfo;
}

/*
===========
idGameLocal::SetPersistentPlayerInfo
============
*/
void idGameLocal::SetPersistentPlayerInfo( int clientNum, const idDict &playerInfo ) {
	persistentPlayerInfo = playerInfo;
}

/*
===========
idGameLocal::triggeredSave
===========
*/

idStr idGameLocal::triggeredSave()
{
	idStr sgn = cvarSystem->GetCVarString("saveGameName");
	cvarSystem->SetCVarString("saveGameName","");
	return sgn;	
}

/*
=========
idGameLocal::saveGamesDisallowed
=========
*/
bool idGameLocal::savegamesDisallowed()
{
	idPlayer* player = GetLocalPlayer();
	return player->savePermissions == 2;
}

/*
=========
idGameLocal::quicksavesDisallowed
=========
*/
bool idGameLocal::quicksavesDisallowed()
{
	idPlayer* player = GetLocalPlayer();
	return player->savePermissions == 1;
}

/*
=========
idGameLocal::PlayerReady
=========
*/
bool idGameLocal::PlayerReady()
{
	idPlayer* player = GetLocalPlayer();
	if ( player == NULL ) // duzenko: game has not started yet, better this than CTD
		return false;
	return player->IsReady();
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

	if ( !com_developer.GetBool() ) {
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

	if ( !com_developer.GetBool() ) {
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

	// Send this error message to the GUI queue
	m_guiError = text;

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

	if ( clientNum >= 0 && clientNum < 1 ) {
		idGameLocal::userInfo = userInfo;

		// server sanity
		if ( canModify  ) {

			// don't let numeric nicknames, it can be exploited to go around kick and ban commands from the server
			if ( idStr::IsNumeric( this->userInfo.GetString( "ui_name" ) ) ) {
				idGameLocal::userInfo.Set( "ui_name", va( "%s_", idGameLocal::userInfo.GetString( "ui_name" ) ) );
				modifiedInfo = true;
			}
		
			// don't allow dupe nicknames
			for ( i = 0; i < numClients; i++ ) {
				if ( i == clientNum ) {
					continue;
				}
				if ( entities[ i ] && entities[ i ]->IsType( idPlayer::Type ) ) {
					if ( !idStr::Icmp( idGameLocal::userInfo.GetString( "ui_name" ), idGameLocal::userInfo.GetString( "ui_name" ) ) ) {
						idGameLocal::userInfo.Set( "ui_name", va( "%s_", idGameLocal::userInfo.GetString( "ui_name" ) ) );
						modifiedInfo = true;
						i = -1;	// rescan
						continue;
					}
				}
			}
		}

		if ( entities[ clientNum ] && entities[ clientNum ]->IsType( idPlayer::Type ) ) {
			modifiedInfo |= static_cast<idPlayer *>( entities[ clientNum ] )->UserInfoChanged(canModify);
		}
	}

	if ( modifiedInfo ) {
		assert( canModify );

		newInfo = idGameLocal::userInfo;
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
		return &userInfo;
	}
	return NULL;
}

/*
===========
idGameLocal::SetServerInfo
============
*/
void idGameLocal::SetServerInfo( const idDict &_serverInfo ) {
	serverInfo = _serverInfo;
}

/*
===================
idGameLocal::LoadMap

Initializes all map variables common to both save games and spawned games.
===================
*/
void idGameLocal::LoadMap( const char *mapName, int randseed ) {
	int i;
	// A couple of flags to track whether we are reloading the same map. "bool sameMap" has been around for a while, 
	// and checks for reloading the currently active map. It'll usually be false, as the map will have been shut 
	// down even when doing a quick load. "mapFileName" acts as a flag: active vs shut down.
	// "sameAsPrevMap" checks for a quickload. Added for #2807: Random GUI quotes on loading screen -- SteveL TDM2.04
	bool sameMap = (mapFile && idStr::Icmp(mapFileName, mapName) == 0); 
	bool sameAsPrevMap = mapFile && idStr::Icmp(mapFile->GetName(), mapName) == 0;

	// Still with #2807: Random GUI quotes on loading screen. Next code block moved here from 
	// InitFromNewMap() so that loading of saved games can show tips too.
	idStr mapNameStr = mapName;
	mapNameStr.StripLeadingOnce("maps/");
	mapNameStr.StripFileExtension();
	idUserInterface* loadingGUI = uiManager->FindGui(va("guis/map/%s.gui", mapNameStr.c_str()), false, false, false);
	if (loadingGUI != NULL )
	{
		// Use our own randomizer, the gameLocal.random one is not yet initialised
		idRandom simpleRnd(randseed);
		float random_value = simpleRnd.RandomFloat();
		loadingGUI->SetStateFloat("random_value", random_value);
		// #2807: Allow GUI scripts to distinguish between a quickload and a full load, so 
		// they can choose to show only short messages. 
		loadingGUI->SetStateBool("quickloading", sameAsPrevMap);
		// Activate any GUI code that depends on this random value (which includes random text)
		loadingGUI->HandleNamedEvent("OnRandomValueInitialised");
	}

	session->ResetMainMenu();

	// clear the sound system
	gameSoundWorld->ClearAllSoundEmitters();

	musicSpeakers.Clear();			// Tels: Clear the list even on reload to account for dynamic changes
	cv_music_volume.SetModified();	// SnoopJeDi: we want to fade on level start

	realClientTime = 0; 

	if ( !sameMap || ( mapFile && mapFile->NeedsReload() ) ) {
		// load the .map file
		if ( mapFile ) {
			delete mapFile;
		}
		session->UpdateLoadingProgressBar( PROGRESS_STAGE_MAPFILE, 0.0f );
		mapFile = new idMapFile;
		if ( !mapFile->Parse( idStr( mapName ) + ".map" ) ) {
			delete mapFile;
			mapFile = NULL;
			Error( "Couldn't load %s", mapName );
		}
		session->UpdateLoadingProgressBar( PROGRESS_STAGE_MAPFILE, 1.0f );
	}
	mapFileName = mapFile->GetName();

	// load the collision map
	collisionModelManager->LoadMap( mapFile );

	numClients = 0;

	// initialize all entities for this game
	memset( entities.Ptr(), 0, entities.MemoryUsed() );
	memset( &usercmds, 0, sizeof( usercmds ) );
	memset( spawnIds.Ptr(), -1, spawnIds.MemoryUsed() );
	spawnCount = INITIAL_SPAWN_COUNT;
	
	spawnedEntities.Clear();
	activeEntities.Clear();
	spawnedAI.Clear();
	lodSystem.Clear();
	numEntitiesToDeactivate = 0;
	lastGUIEnt = NULL;
	lastGUI = 0;

	globalMaterial = NULL;

	memset( globalShaderParms, 0, sizeof( globalShaderParms ) );

	// always leave room for the max number of clients,
	// even if they aren't all used, so numbers inside that
	// range are NEVER anything but clients
	num_entities	= 1;
	firstFreeIndex	= 1;

	// reset the random number generator.
	// Tels: use a random seed for single-player, too, otherwise map content can't be random
	//random.SetSeed( isMultiplayer ? randseed : 0 );
	random.SetSeed( randseed );

	camera			= NULL;
	world			= NULL;
	testmodel		= NULL;
	testFx			= NULL;

	previousTime	= 0;
	time			= 0;
	framenum		= 0;
	sessionCommand = "";
	nextGibTime		= 0;

	portalSkyEnt			= NULL;

	portalSkyActive			= false;

	playerOldEyePos.Zero();			 // grayman #3108 - contributed by 7318
	globalPortalSky			= false; // grayman #3108 - contributed by 7318

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


	// this will always fail for now, have not yet written the map compile
	m_sndPropLoader->CompileMap( mapFile );

	playerPVS.i = -1;
	playerConnectedAreas.i = -1;

	// load navigation system for all the different monster sizes
	for( i = 0; i < aasNames.Num(); i++ ) {
		aasList[ i ]->Init( idStr( mapFileName ).SetFileExtension( aasNames[ i ] ).c_str(), mapFile->GetGeometryCRC() );
	}

	/*!
	* The Dark Mod LAS: Init the Light Awareness System
	* This must occur AFTER the AAS list is loaded
	*/
	LAS.initialize();

	// clear the smoke particle free list
	smokeParticles->Init();

	// cache miscellanious media references
	FindEntityDef( "preCacheExtras", false );

	if ( !sameMap ) {
		mapFile->RemovePrimitiveData();
	}

	// greebo: Reset the flag. When a map is loaded, the success screen is definitely not shown anymore.
	// This is meant to catch cases where the player is reloading a map from the console without clicking
	// the "Continue" button on the success GUI. I can't stop him, so I need to track this here.
	postMissionScreenActive = false;

	if (m_MissionData != NULL)
	{
		m_MissionData->ClearGUIState();
	}

	m_strMainAmbientLightName = "ambient_world"; // Default name for main ambient light. J.C.Denton.

	m_suspiciousEvents.Clear(); // grayman #3424
	m_ambientLights.Clear();	// grayman #3584
	m_searchManager->Clear(); // grayman #3857
}

static idDict ComputeSpawnargsDifference(const idDict &newArgs, const idDict &oldArgs) {
	idDict res;
	//find added or modified args
	for (int i = 0; i < newArgs.GetNumKeyVals(); i++) {
		const idKeyValue *newKv = newArgs.GetKeyVal(i);
		if (const idKeyValue *oldKv = oldArgs.FindKey(newKv->GetKey())) {
			if (oldKv->GetValue() == newKv->GetValue())
				continue;
		}
		res.Set(newKv->GetKey(), newKv->GetValue());
	}
	//find removed args
	const char *classname = newArgs.GetString("classname");
	const idDeclEntityDef *def = gameLocal.FindEntityDef(classname, false);
	for (int i = 0; i < oldArgs.GetNumKeyVals(); i++) {
		const idKeyValue *oldKv = oldArgs.GetKeyVal(i);
		const char *key = oldKv->GetKey();
		if (!newArgs.FindKey(key)) {
			//removal detected
			const char *newVal = "";
			if (def) {
				//lookup entitydef: use its value if present
				newVal = def->dict.GetString(key);
			}
			res.Set(oldKv->GetKey(), newVal);
		}
	}
	return res;
}
void idGameLocal::HotReloadMap(const char *mapDiff, bool skipTimestampCheck) {
	if (!mapFile) {
		common->Warning("HotReload: idGameLocal has no mapFile stored");
		return;
	}

	idMapReloadInfo info;
	if (mapDiff) {
		info = mapFile->ApplyDiff(mapDiff);
	}
	else {
		if (!skipTimestampCheck && !mapFile->NeedsReload()) {
			common->DPrintf("HotReload: mapFile does not need reload");
			return;
		}

		//reload .map file
		info = mapFile->TryReload();
	}

	if (info.cannotReload || info.mapInvalid)
		return;

	//spawn added entities
	for (int i = 0; i < info.addedEntities.Num(); i++) {
		const char *name = info.addedEntities[i].name;
		idMapEntity *mapEnt = mapFile->GetEntity(info.addedEntities[i].idx);
		idDict args = mapEnt->epairs;
		if (idEntity *exEnt = gameEdit->FindEntity(name)) {
			//note: this should not happen actually...
			common->Warning("HotReload: multiple entities with name %s: rename the new one!", name);
			continue;
		}
		gameEdit->SpawnEntityDef(args, NULL, idGameEdit::sedRespectInhibit);
	}

	//kill removed entities
	for (int i = 0; i < info.removedEntities.Num(); i++) {
		const char *name = info.removedEntities[i].name;
		if (idStr::Cmp(name, DARKMOD_LG_ENTITY_NAME) == 0)
			continue;	//lightgem is added dynamically, so it is always "removed"
		idEntity *ent = gameEdit->FindEntity(name);
		if (!ent)
			continue;
		gameEdit->EntityDelete(ent, true);
	}

	//update modified entities
	int modNum = info.modifiedEntities.Num();
	int respNum = info.respawnEntities.Num();
	for (int i = 0; i < modNum + respNum; i++) {
		const char *name = (i < modNum ? info.modifiedEntities[i].name : info.respawnEntities[i - modNum].name);
		bool respawn = (i >= modNum);
		if (idStr::Cmp(name, "worldspawn") == 0)
			continue;	//world not updateable yet
		idMapEntity *newMapEnt = mapFile->FindEntity(name);
		idMapEntity *oldMapEnt = info.oldMap->FindEntity(name);
		assert(newMapEnt && oldMapEnt);

		idDict newArgs = newMapEnt->epairs;
		idDict oldArgs = oldMapEnt->epairs;
		idDict diffArgs = ComputeSpawnargsDifference(newArgs, oldArgs);

		idEntity *ent = FindEntity(name);
		if (!ent) {
			gameEdit->SpawnEntityDef(newArgs, NULL, idGameEdit::sedRespectInhibit);
			continue;
		}

		bool lodChanged = false;
		for (int i = 0; i < diffArgs.GetNumKeyVals(); i++) {
			const idKeyValue *kv = diffArgs.GetKeyVal(i);
			if (
				kv->GetKey().Icmp("spawnclass") == 0 || 
				kv->GetKey().Icmp("classname") == 0 || 
				kv->GetKey().IcmpPrefix("def_attach") == 0 || 
				kv->GetKey().IcmpPrefix("pos_attach") == 0 || 
				kv->GetKey().IcmpPrefix("name_attach") == 0 || 
				kv->GetKey().IcmpPrefix("set ") == 0 ||
				kv->GetKey().IcmpPrefix("add_link") == 0 ||
				kv->GetKey().IcmpPrefix("s_") == 0 ||		//should we try to update sound properties?...
			0) {
				respawn = true;
			}
			if (idStr::FindText(kv->GetKey(), "lod_") >= 0 ||
				idStr::FindText(kv->GetKey(), "_lod") >= 0 ||
				kv->GetKey().IcmpPrefix("dist_check") == 0 ||
				kv->GetKey().Icmp("hide_distance") == 0 ||
			0) {
				lodChanged = true;
			}
		}
		if (respawn) {
			gameEdit->SpawnEntityDef(newArgs, &ent, idGameEdit::sedRespectInhibit | idGameEdit::sedRespawn);
			continue;
		}

		//update spawnargs dict in entity
		gameEdit->EntityChangeSpawnArgs( ent, &diffArgs );

		//compute new spawnargs with inherited props
		const char *newClassname = newArgs.GetString("classname");
		const idDeclEntityDef *newEntityDef = gameLocal.FindEntityDef(newClassname, false);
		idDict newArgsInherited = newArgs;
		newArgsInherited.SetDefaults(&newEntityDef->dict, idStr("editor_"));

		bool originChanged = diffArgs.FindKey("origin") != NULL;
		bool axisChanged = diffArgs.FindKey("rotation") || diffArgs.FindKey("angle");

		//check if entity is moved while bound to master
		idVec3 masterOrigin;	masterOrigin.Zero();
		idMat3 masterAxis;		masterAxis.Identity();
		if (originChanged || axisChanged) {
			//find coordinate system of the bind master according to new spawnargs
			//if position of bound entity is changed, then we will adjust it relative to that
			if (idEntity *gameMasterEnt = ent->GetBindMaster()) {
				const char *gameMasterName = gameMasterEnt->name.c_str();
				const char *newMasterName = newArgsInherited.GetString("bind");
				idMapEntity *newMasterMapEnt = mapFile->FindEntity(newMasterName);

				//note: we only support simple binds here, no "bindToJoint" or "bindToBody"
				bool complexBind = (gameMasterEnt->GetBindBody() >= 0 || gameMasterEnt->GetBindJoint());

				if (idStr::Cmp(gameMasterName, newMasterName) == 0 && newMasterMapEnt && !complexBind) {
					//still has same bind master as in .map file
					masterOrigin = newMasterMapEnt->epairs.GetVector("origin");
					gameEdit->ParseSpawnArgsToAxis(&newMasterMapEnt->epairs, masterAxis);
				}
				else {
					//different master / master missing in .map / unsupported bind type
					//unbind entity and move in absolute coords
					ent->Unbind();
				}
			}
		}

		if (diffArgs.FindKey("rotate") || diffArgs.FindKey("translate"))
			if (ent->IsType(CBinaryFrobMover::Type)) {
				((CBinaryFrobMover*)ent)->SetFractionalPosition(0.0, true);
				((CBinaryFrobMover*)ent)->Event_PostSpawn();
			}
		if (diffArgs.FindKey("model")) {
			idStr newModel = newArgsInherited.GetString("model");
			gameEdit->EntitySetModel(ent, newModel);
		}
		if (diffArgs.FindKey("skin")) {
			idStr newSkin = newArgsInherited.GetString("skin");
			ent->Event_SetSkin(newSkin);
		}
		if (diffArgs.FindKey("noshadows")) {
			bool newNoShadows = newArgsInherited.GetBool("noshadows");
			ent->Event_noShadows(newNoShadows);
		}
		if (lodChanged) {
			gameEdit->EntityUpdateLOD(ent);
		}
		if (originChanged) {
			idVec3 newOrigin = newArgsInherited.GetVector("origin");
			newOrigin = (newOrigin - masterOrigin) * masterAxis.Inverse();
			gameEdit->EntitySetOrigin(ent, newOrigin);
		}
		if (axisChanged) {
			idMat3 newAxis;
			gameEdit->ParseSpawnArgsToAxis(&newArgsInherited, newAxis);
			newAxis = newAxis * masterAxis.Inverse();
			gameEdit->EntitySetAxis(ent, newAxis);
		}
		if (diffArgs.FindKey("_color") || diffArgs.MatchPrefix("shaderParm")) {
			gameEdit->EntityUpdateShaderParms(ent);
		}
		if (ent->IsType(idLight::Type)) {
			gameEdit->ParseSpawnArgsToRenderLight(&newArgsInherited, ((idLight*)ent)->GetRenderLight());
		}
		if (diffArgs.MatchPrefix("target")) {
			//note: rebuild list of targets on NEXT frame
			//in case we will spawn some targeted entity in this diff bundle
			ent->PostEventMS(&EV_FindTargets, 0);
		}

		gameEdit->EntityUpdateVisuals( ent );
	}

	common->Printf("HotReload: SUCCESS\n");
}

/*
===================
idGameLocal::LocalMapRestart
===================
*/
void idGameLocal::LocalMapRestart( ) {
	int latchSpawnCount;

	Printf( "----------- Game Map Restart ------------\n" );

	gamestate = GAMESTATE_SHUTDOWN;

		if ( entities[ 0 ] && entities[ 0 ]->IsType( idPlayer::Type ) ) {
			static_cast< idPlayer * >( entities[ 0 ] )->PrepareForRestart();
		}

	MapClear( false );

	// clear the smoke particle free list
	smokeParticles->Init();

	// clear the sound system
	if ( gameSoundWorld ) {
		gameSoundWorld->ClearAllSoundEmitters();
	}

	/*!
	* The Dark Mod LAS: Init the LAS
	*/
	LAS.initialize();

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
		if ( entities[ 0 ] && entities[ 0 ]->IsType( idPlayer::Type ) ) {
			static_cast< idPlayer * >( entities[ 0 ] )->Restart();
		}

	gamestate = GAMESTATE_ACTIVE;
	m_MissionResult = MISSION_INPROGRESS;

	Printf( "--------------------------------------\n" );
}

/*
===================
idGameLocal::MapRestart
===================
*/
void idGameLocal::MapRestart( ) {
	idDict		newInfo;
	int			i;
	const idKeyValue *keyval, *keyval2;

	{
		newInfo = cvarSystem->MoveCVarsToDict( CVAR_SERVERINFO );
		for ( i = 0; i < newInfo.GetNumKeyVals(); i++ ) {
			keyval = newInfo.GetKeyVal( i );
			keyval2 = serverInfo.FindKey( keyval->GetKey() );
			if ( !keyval2 ) {
				break;
			}
			// a select set of si_ changes will cause a full restart of the server
			if ( keyval->GetValue().Cmp( keyval2->GetValue() ) && ( !keyval->GetKey().Cmp( "si_map" ) ) ) {
				break;
			}
		}
		cmdSystem->BufferCommandText( CMD_EXEC_NOW, "rescanSI" );
		if ( i != newInfo.GetNumKeyVals() ) {
			cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "nextMap\n" );
		} else {
			LocalMapRestart();
		}
	}
}

/*
===================
idGameLocal::MapPopulate
Dark Mod: Sound prop initialization added
===================
*/
void idGameLocal::MapPopulate( void ) {

	// parse the key/value pairs and spawn entities
	SpawnMapEntities();

	// mark location entities in all connected areas
	SpreadLocations();

	// spawnCount - 1 is the number of entities spawned into the map, their indexes started at MAX_CLIENTS (included)
	// mapSpawnCount is used as the max index of map entities, it's the first index of non-map entities
	mapSpawnCount = 1 + spawnCount - 1;

	// read in the soundprop data for various locations
	m_sndPropLoader->FillLocationData();

	// Transfer sound prop data from loader to gameplay object

	// grayman #3042 - A bit of explanation here. Prior to this point,
	// sound data is stored in m_sndPropLoader. After this point, sound
	// data needs to go in m_sndProp. This is important for sound-related
	// spawnargs that don't get set up until the PostSpawn() routines are
	// run for certain entities.
	m_sndProp->SetupFromLoader( m_sndPropLoader );

	m_sndPropLoader->Shutdown(); // And here's why you can't use m_sndPropLoader after this point
	
	// Initialise the escape point manager after all the entities have been spawned.
	m_EscapePointManager->InitAAS();

	// grayman #4220 - Clear the search manager
	m_searchManager->Clear();

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

	if ( mapFileName.Length() ) {
		MapShutdown();
	}
	
	// greebo: Clear the mission data, it might have been filled during the objectives screen display
	m_MissionData->Clear();

	Printf( "----------- Game Map Init ------------\n" );

	gamestate = GAMESTATE_STARTUP;

	// #5453: reset all mission overrides
	cvarSystem->SetMissionOverrides();

	gameRenderWorld = renderWorld;
	gameSoundWorld = soundWorld;

	LoadMap( mapName, randseed );

	// Instantiate our grabber entity
	m_Grabber = static_cast<CGrabber*>(CGrabber::Type.CreateInstance());

	// greebo: Initialize the Difficulty Manager, before any entities are spawned
	m_DifficultyManager.Init(mapFile);
	m_ConversationSystem->Init(mapFile);

	// Immediately apply the CVAR difficulty settings
	m_DifficultyManager.ApplyCVARDifficultySettings();
	
	InitScriptForMap();

	// Initialize the AI relationships
	// greebo: Do this before spawning the rest of the map entities to give them a chance
	// to override the default settings found on the entityDef
	const idDict* defaultRelationsDict = FindEntityDefDict(cv_tdm_default_relations_def.GetString());

	if (defaultRelationsDict != NULL)
	{
		m_RelationsManager->SetFromArgs(*defaultRelationsDict);
	}

	MapPopulate();

	// ishtvan: Set the player variable on the grabber
	//m_Grabber->SetPlayer( GetLocalPlayer() ); // greebo: GetLocalPlayer() returns NULL, moved to idPlayer::Spawn()

	// greebo: Add the elevator reachabilities to the AAS
	SetupEAS();

	// Then, apply the worldspawn settings
	m_RelationsManager->SetFromArgs( world->spawnArgs );

	// free up any unused animations
	animationLib.FlushUnusedAnims();

	gamestate = GAMESTATE_ACTIVE;
	m_MissionResult = MISSION_INPROGRESS;

	// Let the mission database know that we start playing
	m_MissionManager->OnMissionStart();

	// We need an objectives update now that we've loaded the map
	m_MissionData->ClearGUIState();

	// stgatilov: for some reason, many clipmodels are not in their proper place yet
	// so this optimization is incomplete...
	// hopefully, player will save/load game, which would trigger full reconstruction of clip octree
	clip.Optimize();

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

	savegame.ReadHeader();

	// STiFU #4531: All version checking has already been done in idSessionLocal::IsSavegameValid()
// 	if (!cv_force_savegame_load.GetBool() && savegame.GetCodeRevision() != RevisionTracker::Instance().GetHighestRevision())
// 	{
// 		gameLocal.Printf("Can't load this savegame, was saved with an old revision %d\n", savegame.GetCodeRevision());
// 		return false;
// 	}

	// Read and initialize cache from file
	savegame.InitializeCache();

	// #5453: restore mission overrides of all cvars
	idDict cvarOverrides;
	savegame.ReadDict( &cvarOverrides );
	cvarSystem->SetMissionOverrides( cvarOverrides );

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
	LoadMap( mapName, Sys_Milliseconds() );

	// Restore the global hiding spot search collection
	CHidingSpotSearchCollection::Instance().Restore(&savegame);

	// Restore our grabber pointer
	savegame.ReadObject( reinterpret_cast<idClass*&>(m_Grabber) );

	m_ModelGenerator->Restore(&savegame);
	m_LightController->Restore(&savegame);

	m_DifficultyManager.Restore(&savegame);

	for ( i = 0; i < NumAAS(); i++)
	{
		idAAS* aas = GetAAS(i);
		if (aas != NULL)
		{
			aas->Restore(&savegame);
		}
	}

	m_GamePlayTimer.Restore(&savegame);
	m_GamePlayTimer.SetEnabled(false);
	m_AreaManager.Restore(&savegame);
	m_ConversationSystem->Restore(&savegame);
	m_RelationsManager->Restore(&savegame);
	m_Shop->Restore(&savegame);
	LAS.Restore(&savegame);
	m_MissionManager->Restore(&savegame);

#ifdef TIMING_BUILD
	debugtools::TimerManager::Instance().Restore(&savegame);
#endif

	// precache the player
	FindEntityDef( cv_player_spawnclass.GetString(), false );

	// precache the empty model (used by idEntity::m_renderTrigger)
	renderModelManager->FindModel( cv_empty_model.GetString() );

	m_lightGem.SpawnLightGemEntity( mapFile );

	// precache any media specified in the map
	session->UpdateLoadingProgressBar( PROGRESS_STAGE_ENTITIES, 0.0f );
	for ( i = 0; i < mapFile->GetNumEntities(); i++ ) {
		idMapEntity *mapEnt = mapFile->GetEntity( i );
		const char *entName = mapEnt->epairs.GetString("name");

		if ( !InhibitEntitySpawn( mapEnt->epairs ) ) {
			TRACE_CPU_SCOPE_TEXT("Entity:Spawn", entName)
			declManager->BeginEntityLoad(mapEnt);
			CacheDictionaryMedia( &mapEnt->epairs );
			const char *classname = mapEnt->epairs.GetString( "classname" );
			if ( classname != NULL ) {
				FindEntityDef( classname, false );
			}
			declManager->EndEntityLoad(mapEnt);
		}

		session->UpdateLoadingProgressBar( PROGRESS_STAGE_ENTITIES, float(i + 1) / mapFile->GetNumEntities() );
	}
	session->UpdateLoadingProgressBar( PROGRESS_STAGE_ENTITIES, 1.0f );

	savegame.ReadDict( &si );
	SetServerInfo( si );

	savegame.ReadInt( numClients );
	savegame.ReadDict( &userInfo );
	savegame.ReadUsercmd( usercmds );
	savegame.ReadDict( &persistentPlayerInfo );

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

	activeEntities.Restore( savegame );

	lodSystem.Restore( savegame );

	m_LightEstimateSystem->Restore(&savegame);

	// tels: restore the list of music speakers
	savegame.ReadInt( num );
	musicSpeakers.Clear();
	for( i = 0; i < num; i++ ) {
		int id;
		savegame.ReadInt( id );
		musicSpeakers.Append( id );
	}

	// grayman #3424
	savegame.ReadInt(num);
	m_suspiciousEvents.Clear();
	m_suspiciousEvents.SetNum(num);
	for ( int i = 0 ; i < num ; i++ )
	{
		int type;
		savegame.ReadInt(type);
		m_suspiciousEvents[i].type = static_cast<EventType>(type);
		savegame.ReadVec3(m_suspiciousEvents[i].location);
		m_suspiciousEvents[i].entity.Restore(&savegame);
		savegame.ReadInt(m_suspiciousEvents[i].time); // grayman #3857
	}

	m_ambientLights.Clear();
	savegame.ReadInt( num );
	m_ambientLights.SetNum( num );
	for ( int i = 0 ; i < num ; i++ )
	{
		m_ambientLights[i].Restore(&savegame);
	}


	savegame.ReadInt( num );
	for( i = 0; i < num; i++ )
	{
		idAI* ai(NULL);
		savegame.ReadObject(reinterpret_cast<idClass *&>(ai));
		assert(ai != NULL);

		if (ai != NULL)
		{
			ai->aiNode.AddToEnd(spawnedAI);
		}
	}

	savegame.ReadInt( numEntitiesToDeactivate );
	savegame.ReadDict( &persistentLevelInfo );
	persistentLevelInfoLocation = PERSISTENT_LOCATION_GAME;

	persistentPlayerInventory->Restore(&savegame);

	savegame.ReadInt(num);
	campaignInfoEntities.SetNum(num);
	for (i = 0; i < num; ++i)
	{
		savegame.ReadObject(reinterpret_cast<idClass*&>(campaignInfoEntities[i]));
	}

	for( i = 0; i < MAX_GLOBAL_SHADER_PARMS; i++ ) {
		savegame.ReadFloat( globalShaderParms[ i ] );
	}

	savegame.ReadBool(postMissionScreenActive);

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

	savegame.ReadInt( framenum );
	savegame.ReadInt( previousTime );
	savegame.ReadInt( time );

	savegame.ReadInt( vacuumAreaNum );

	savegame.ReadInt( entityDefBits );

	savegame.ReadInt( localClientNum );

	// snapshotEntities is used for multiplayer only

	savegame.ReadInt( realClientTime );
	savegame.ReadBool( isNewFrame );
	savegame.ReadFloat( clientSmoothing );

	portalSkyEnt.Restore( &savegame );

	savegame.ReadBool( portalSkyActive );

	// grayman #3108 - contributed by 7318
	savegame.ReadBool( globalPortalSky );
	savegame.ReadInt( portalSkyScale ); // grayman #3582
	savegame.ReadInt( currentPortalSkyType );
	savegame.ReadVec3( playerOldEyePos );
	savegame.ReadVec3( portalSkyGlobalOrigin );
	savegame.ReadVec3( portalSkyOrigin );
	// end 7318

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

	savegame.ReadDict( &spawnArgs );

	savegame.ReadInt( playerPVS.i );
	savegame.ReadInt( (int &)playerPVS.h );
	savegame.ReadInt( playerConnectedAreas.i );
	savegame.ReadInt( (int &)playerConnectedAreas.h );

	savegame.ReadVec3( gravity );

	// gamestate is restored after restoring everything else

	savegame.ReadBool( influenceActive );
	savegame.ReadInt( nextGibTime );

	// Restore LightGem				- J.C.Denton
	m_lightGem.Restore( savegame );

	savegame.ReadInt( m_uniqueMessageTag ); // grayman #3355

	savegame.ReadInt(num);
	m_InterMissionTriggers.SetNum(num);
	for (int i = 0; i < num; ++i)
	{
		savegame.ReadInt(m_InterMissionTriggers[i].missionNum);
		savegame.ReadString(m_InterMissionTriggers[i].activatorName);
		savegame.ReadString(m_InterMissionTriggers[i].targetName);
	}

	m_sndProp->Restore(&savegame);
	m_MissionData->Restore(&savegame);

	int missResult;
	savegame.ReadInt(missResult);
	m_MissionResult = static_cast<EMissionResult>(missResult);

	m_CampaignStats->Restore(&savegame);

	savegame.ReadInt(m_HighestSRId);

	savegame.ReadInt(num);
	m_Timer.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		m_Timer[i] = new CStimResponseTimer;
		m_Timer[i]->Restore(&savegame);
	}

	// The list to take all the values, they will be restored later on
	idList<int> tempStimTimerIdList;
	savegame.ReadInt(num);
	tempStimTimerIdList.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		savegame.ReadInt(tempStimTimerIdList[i]);
	}

	savegame.ReadInt(num);
	m_StimEntity.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		m_StimEntity[i].Restore(&savegame);
	}

	savegame.ReadInt(num);
	m_RespEntity.SetNum(num);
	for (int i = 0; i < num; i++)
	{
		m_RespEntity[i].Restore(&savegame);
	}

	m_EscapePointManager->Restore(&savegame);

	m_searchManager->Restore(&savegame); // grayman #3857

	// greebo: Restore the maximum frob distance
	savegame.ReadFloat(g_Global.m_MaxFrobDistance);

	// spawnSpots
	// initialSpots
	// currentInitialSpot
	// newInfo
	// makingBuild
	// shakeSounds

	// Read out pending events
	idEvent::Restore( &savegame );

	// Tels: Restore the GUI command stack
	int cmds; 
	savegame.ReadInt( m_GUICommandArgs );
	savegame.ReadInt( cmds );
	m_GUICommandStack.Clear();
	m_GUICommandStack.SetNum( cmds );
	for( i = 0; i < cmds; i++ ) {
		savegame.ReadString( m_GUICommandStack[i] );
	}
	savegame.RestoreObjects();

	// free up any unused animations
	animationLib.FlushUnusedAnims();

	gamestate = GAMESTATE_ACTIVE;

	//FIX: Set the walkspeed back to the stored value.
	pm_walkspeed.SetFloat( m_walkSpeed );

	// Restore the physics pointer in the grabber.
	gameLocal.m_Grabber->SetPhysicsFromDragEntity();

	// Restore the CStim* pointers in the m_StimTimer list
	m_StimTimer.SetNum(tempStimTimerIdList.Num());
	for (int i = 0; i < tempStimTimerIdList.Num(); i++)
	{
		m_StimTimer[i] = static_cast<CStim*>(FindStimResponse(tempStimTimerIdList[i]).get());
	}

	// Let the mission database know that we start playing
	m_MissionManager->OnMissionStart();

	clip.Optimize();

	Printf( "--------------------------------------\n" );

	// Restart the timer
	m_GamePlayTimer.Start();

	return true;
}

/*
===========
idGameLocal::MapClear
===========
*/
void idGameLocal::MapClear( bool clearClients ) {
	int i;

	for( i = ( clearClients ? 0 : 1 ); i < MAX_GENTITIES; i++ ) {
		delete entities[ i ];
		// ~idEntity is in charge of setting the pointer to NULL
		// it will also clear pending events for this entity
		assert( !entities[ i ] );
		spawnIds[ i ] = -1;
	}

	entityHash.ClearFree( 1024, MAX_GENTITIES );

	if ( !clearClients ) {
		// add back the hashes of the clients
		for ( i = 0; i < 1; i++ ) {
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
	if (gamestate == GAMESTATE_NOMAP)
	{
		// tels: #3287 don't shut down everything twice
		return;
	}
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

	// Run the grabber->clear() method before the entities get deleted from the map
	if (m_Grabber != NULL)
	{
		m_Grabber->Clear();
	}
	
	MapClear( true );

	session->ResetMainMenu();

	// reset the script to the state it was before the map was started
	program.Restart();

	if ( smokeParticles ) {
		smokeParticles->Shutdown();
	}

	/*!
	* The Dark Mod LAS: shut down the LAS
	*/
	LAS.shutDown();

	if (m_LightEstimateSystem)
	{
		m_LightEstimateSystem->Clear();
	}

	pvs.Shutdown();

	// Remove the grabber entity itself (note that it's safe to pass NULL pointers to delete)
	delete m_Grabber;
	m_Grabber = NULL;

	m_sndProp->Clear();
	if (m_RelationsManager != NULL)
	{
		m_RelationsManager->Clear();
	}
	if (m_ConversationSystem != NULL)
	{
		m_ConversationSystem->Clear();
	}

	m_DifficultyManager.Clear();

	if (m_ModelGenerator != NULL)
	{
		m_ModelGenerator->Print();
		m_ModelGenerator->Clear();
	}
	
	if (m_LightController != NULL)
	{
		m_LightController->Clear();
	}

	// greebo: Don't clear the shop - MapShutdown() is called right before loading a map
	// m_Shop->Clear();

	clip.Shutdown();
	idClipModel::ClearTraceModelCache();

	mapFileName.Clear();

	gameRenderWorld = NULL;
	gameSoundWorld = NULL;

	// #5453: reset all mission overrides
	cvarSystem->SetMissionOverrides();

	gamestate = GAMESTATE_NOMAP;

	// Save the current walkspeed
	m_walkSpeed = pm_walkspeed.GetFloat();

	Printf( "--------- Game Map Shutdown done -----\n" );
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
						soundName.Find( "/enpro/", false ) == - 1 ) {
					// don't OGG weapon sounds
					if (	soundName.Find( "weapon", false ) != -1 ||
							soundName.Find( "gun", false ) != -1 ||
							soundName.Find( "bullet", false ) != -1 ||
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
		file->Printf( "w:\\doom\\ogg\\oggenc -q 0 \"c:\\doom\\base\\%s\"\n", oggSounds[i].c_str() );
		file->Printf( "del \"c:\\doom\\base\\%s\"\n", oggSounds[i].c_str() );
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
	if ( soundShaderName != NULL && dict->GetFloat( "s_shakes" ) != 0.0f ) {
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
avoid the fast pre-cache check associated with each entityDef.
===================
*/
void idGameLocal::CacheDictionaryMedia( const idDict *dict ) {
	const idKeyValue *kv;
	const char *name = dict->GetString("name");

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
		const char *modelName = kv->GetValue();
		if ( modelName[0] ) {
			declManager->MediaPrint( "CacheDictionaryMedia - Precaching model %s\n", modelName );
			// precache model/animations
			if ( declManager->FindType( DECL_MODELDEF, modelName, false ) == NULL ) {
				// precache the render model
				idRenderModel *renderModel = renderModelManager->FindModel( modelName );
				// precache .cm files only
				cmHandle_t cmHandle = collisionModelManager->LoadModel( modelName, true );
			}
		}
		kv = dict->MatchPrefix( "model", kv );
	}

	kv = dict->FindKey( "s_shader" );
	if ( kv && kv->GetValue().Length() ) {
		declManager->MediaPrint( "CacheDictionaryMedia - Precaching s_shader %s\n", kv->GetValue().c_str() );
		declManager->FindType( DECL_SOUND, kv->GetValue() );
	}

	kv = dict->MatchPrefix( "snd", NULL );
	while( kv ) {
		if ( kv->GetValue().Length() ) {
			declManager->MediaPrint( "CacheDictionaryMedia - Precaching snd %s\n", kv->GetValue().c_str() );
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
				declManager->MediaPrint( "CacheDictionaryMedia - Precaching gui %s\n", kv->GetValue().c_str() );
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
		declManager->MediaPrint( "CacheDictionaryMedia - Precaching texture %s\n", kv->GetValue().c_str() );
		declManager->FindType( DECL_MATERIAL, kv->GetValue() );
	}

	kv = dict->MatchPrefix( "mtr", NULL );
	while( kv ) {
		if ( kv->GetValue().Length() ) {
			declManager->MediaPrint( "CacheDictionaryMedia - Precaching mtr %s\n", kv->GetValue().c_str() );
			declManager->FindType( DECL_MATERIAL, kv->GetValue() );
		}
		kv = dict->MatchPrefix( "mtr", kv );
	}

	// handles hud icons
	kv = dict->MatchPrefix( "inv_icon", NULL );
	while ( kv ) {
		if ( kv->GetValue().Length() ) {
			declManager->MediaPrint( "CacheDictionaryMedia - Precaching inv_icon %s\n", kv->GetValue().c_str() );
			declManager->FindType( DECL_MATERIAL, kv->GetValue() );
		}
		kv = dict->MatchPrefix( "inv_icon", kv );
	}

	kv = dict->MatchPrefix( "fx", NULL );
	while( kv ) {
		if ( kv->GetValue().Length() ) {
			declManager->MediaPrint( "CacheDictionaryMedia - Precaching fx %s\n", kv->GetValue().c_str() );
			declManager->MediaPrint( "Precaching fx %s\n", kv->GetValue().c_str() );
			declManager->FindType( DECL_FX, kv->GetValue() );
		}
		kv = dict->MatchPrefix( "fx", kv );
	}

	kv = dict->MatchPrefix( "smoke", NULL );
	while( kv ) {
		if ( kv->GetValue().Length() ) {
			declManager->MediaPrint( "CacheDictionaryMedia - Precaching smoke %s\n", kv->GetValue().c_str() );
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
			declManager->MediaPrint( "CacheDictionaryMedia - Precaching skin %s\n", kv->GetValue().c_str() );
			declManager->FindType( DECL_SKIN, kv->GetValue() );
		}
		kv = dict->MatchPrefix( "skin", kv );
	}

	kv = dict->MatchPrefix( "def", NULL );
	while( kv ) {
		if ( kv->GetValue().Length() ) {
			declManager->MediaPrint( "CacheDictionaryMedia - Precaching def %s\n", kv->GetValue().c_str() );
			FindEntityDef( kv->GetValue().c_str(), false );
		}
		kv = dict->MatchPrefix( "def", kv );
	}

	kv = dict->MatchPrefix( "xdata", NULL );
	while( kv ) {
		if ( kv->GetValue().Length() ) {
			declManager->MediaPrint( "CacheDictionaryMedia - Precaching xdata %s\n", kv->GetValue().c_str() );
			declManager->FindType( DECL_XDATA, kv->GetValue().c_str(), false );
		}
		kv = dict->MatchPrefix( "xdata", kv );
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
void idGameLocal::SpawnPlayer( int clientNum )
{
	// they can connect
	Printf( "SpawnPlayer: %i\n", clientNum );

	idStr playerClass = cv_player_spawnclass.GetString();

	// greebo: Allow worldspawn to specify a different player classname
	if ( world != NULL && world->spawnArgs.FindKey("player_classname") != NULL)
	{
		playerClass = world->spawnArgs.GetString("player_classname", cv_player_spawnclass.GetString());
	}

	idDict		args;
	args.SetInt( "spawn_entnum", clientNum );
	args.Set( "name", va( "player%d", clientNum + 1 ) );
	args.Set( "classname", playerClass );

	idEntity	*ent;
	if ( !SpawnEntityDef( args, &ent ) || !entities[ clientNum ] )
	{
		Error( "Failed to spawn player as '%s'", args.GetString( "classname" ) );
	}

	// make sure it's a compatible class
	if ( !ent->IsType( idPlayer::Type ) ) {
		Error( "'%s' spawned the player as a '%s'.  Player spawnclass must be a subclass of idPlayer.", args.GetString( "classname" ), ent->GetClassname() );
	}

	if ( clientNum >= numClients ) {
		numClients = clientNum + 1;
	}

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
			if ( idStr::IcmpNoColor( name, userInfo.GetString( "ui_name" ) ) == 0 ) {
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
idGameLocal::GetViewPos_Cmd
================
*/
bool idGameLocal::GetViewPos_Cmd(idVec3 &origin, idMat3 &axis) const {
	origin.Zero();
	axis.Zero();

	idPlayer *player = gameLocal.GetLocalPlayer();
	if ( !player ) {
		return false;
	}

	const renderView_t *view = player->GetRenderView();
	if ( view ) {
		origin = view->vieworg;
		axis = view->viewaxis;
	} else {
		player->GetViewPos( origin, axis );
	}
	return true;
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

		// if portalSky is present, then merge into pvs so we get rotating brushes, etc

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
	TRACE_CPU_SCOPE( "SortActive" )
	idEntity *ent, *master, *part;

	static idList<idEntity*> buckets[6];

	for ( auto iter = activeEntities.Begin(); iter; activeEntities.Next(iter) ) {
		ent = iter.entity;
		master = ent->GetTeamMaster();

		bool isTeamMaster = ( master && master == ent );
		bool hasActor = false;
		bool hasParametric = false;

		if ( !master || master == ent ) {
			// check if there is an actor on the team,
			// or an entity with parametric physics (?pusher?)
			for ( part = ent; part != NULL; part = part->GetNextTeamEntity() ) {
				if ( part->GetPhysics()->IsType( idPhysics_Actor::Type ) )
					hasActor = true;
				if ( part->GetPhysics()->IsType( idPhysics_Parametric::Type ) )
					hasParametric = true;
			}
		}

		int group = (
			hasParametric ? 0 :		// pushers first
			hasActor ? 1 :			// actors next
			2						// finally, all the rest 
		);
		// without eah group, put team masters first
		group = 2 * group + !isTeamMaster;

		buckets[group].AddGrow( ent );
	}

	// concatenate buckets
	int num = 0;
	for ( int b = 0; b < 6; b++ )
		num += buckets[b].Num();
	idList<idEntity*> newOrder;
	newOrder.SetNum(num);
	num = 0;
	for ( int b = 0; b < 6; b++ ) {
		memcpy( newOrder.Ptr() + num, buckets[b].Ptr(), buckets[b].MemoryUsed() );
		num += buckets[b].Num();
		buckets[b].Clear();
	}

	activeEntities.FromList( newOrder );
}

/*
================
idGameLocal::RunFrame
================
*/
gameReturn_t idGameLocal::RunFrame( const usercmd_t *clientCmds, int timestepMs, bool minorTic ) {
	idEntity *	ent;
	int			num(-1);
	idTimer		timer_think, timer_events, timer_singlethink;
	gameReturn_t ret;
	idPlayer	*player;
	const renderView_t *view;
	int curframe = framenum;

	memset(&ret, 0, sizeof(ret));
	ret.sessionCommand[0] = 0; // grayman #3139 - must be cleared here, to handle the "player waiting" time
	g_Global.m_Frame = curframe;
	DM_LOG(LC_FRAME, LT_INFO)LOGSTRING("Frame start\r");

	player = GetLocalPlayer();
	if (!player) {
		//stgatilov #4670: if player is absent, the game is most likely being shutdown
		//so we should not compute anything in Game to avoid crashes
		return ret;
	}

	// Handle any mission downloads in progress
	m_DownloadManager->ProcessDownloads();

	if (framenum == 0 && player != NULL && !player->IsReady())
	{
		// greebo: This is the first game frame, handle the "click to start GUI"
		if (m_GamePlayTimer.IsEnabled())
		{
			m_GamePlayTimer.Stop();
		}

		// set the user commands for this frame
		memcpy(&usercmds, clientCmds, sizeof(usercmds));

		if ( player->WaitUntilReady() ) // grayman #3763
		{
			// Player got ready this frame, start timer
			m_GamePlayTimer.Start();
		}
	}
	else
	{
		// Update the gameplay timer
		m_GamePlayTimer.Update();

		if ( g_stopTime.GetBool() ) {
			// clear any debug lines from a previous frame
			gameRenderWorld->DebugClearLines( time + 1 );

			// set the user commands for this frame
			memcpy( &usercmds, clientCmds, sizeof( usercmds ) );

			if ( player ) {
				player->Think();
			}
		}
		else do
		{
			// update the game time
			framenum++;
			previousTime = time;
			time += idMath::Imax(int(timestepMs * g_timeModifier.GetFloat()), 1);
			realClientTime = time;
			this->minorTic = minorTic;

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
			memcpy( &usercmds, clientCmds, sizeof( usercmds ) );

			// free old smoke particles
			smokeParticles->FreeSmokes();

			// update our gravity vector if needed.
			UpdateGravity();

			// create a merged pvs for all players
			SetupPlayerPVS();

			{
				TRACE_CPU_SCOPE( "Update:LAS" )
				// The Dark Mod
				// 10/9/2005: SophisticatedZombie
				// Update the Light Awareness System
				LAS.updateLASState();
			}

			unsigned int ticks = static_cast<unsigned int>(sys->GetClockTicks());

			// Tick the timers. Should be done before stim/response, just to be safe. :)
			ProcessTimer(ticks);

			// TDM: Work through the active stims/responses
			ProcessStimResponse(ticks);

			// TDM: Update objective system
			m_MissionData->UpdateObjectives();

			// sort the active entity list
			SortActiveEntityList();

			// check and possibly switch LOD levels 
			lodSystem.ThinkAllLod();

			timer_think.Clear();
			timer_think.Start();

			{ // let entities think
				TRACE_CPU_SCOPE( "ThinkAllEntities" )
				num = 0;
				bool timeentities = (g_timeentities.GetFloat() > 0.0);
				for ( auto iter = activeEntities.Begin(); iter; activeEntities.Next(iter) ) {
					ent = iter.entity;
					if ( inCinematic && g_cinematic.GetBool() && !ent->cinematic ) {
						ent->GetPhysics()->UpdateTime( time );
						// grayman #2654 - update m_lastThinkTime to keep non-cinematic AI from dying at CrashLand()
						if (ent->IsType(idAI::Type)) {
							static_cast<idAI*>(ent)->m_lastThinkTime = time;
						}
						continue;
					}

					if ( timeentities ) {
						timer_singlethink.Clear();
						timer_singlethink.Start();
					}

					{
						TRACE_CPU_SCOPE_STR("Entity:Think", ent->name)
						ent->Think();
						num++;
					}

					if ( timeentities ) {
						timer_singlethink.Stop();
						float ms = timer_singlethink.Milliseconds();
						if ( ms >= g_timeentities.GetFloat() ) {
							Printf( "%d: entity '%s': %.1f ms\n", time, ent->name.c_str(), ms );
							DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("%d: entity '%s': %.3f ms\r", time, ent->name.c_str(), ms );
						}
					}
				}
			}

			// remove any entities that have stopped thinking
			if ( numEntitiesToDeactivate ) {
				TRACE_CPU_SCOPE( "DeactivateEntities" )
				int c = 0;
				for ( auto iter = activeEntities.Begin(); iter; activeEntities.Next(iter) ) {
					ent = iter.entity;
					if ( !ent->thinkFlags ) {
						activeEntities.Remove( ent );
						c++;
					}
				}
				//assert( numEntitiesToDeactivate == c );
				numEntitiesToDeactivate = 0;
			}

			timer_think.Stop();
		
			//DM_LOG(LC_ENTITY, LT_INFO)LOGSTRING("Thinking timer: %lfms\r", timer_think.Milliseconds());

			timer_events.Clear();
			timer_events.Start();

			// service any pending events
			idEvent::ServiceEvents();

			timer_events.Stop();

			// Process the active AI conversations
			m_ConversationSystem->ProcessConversations();

			// grayman #3857 - Process the active searches
			m_searchManager->ProcessSearches();

			// free the player pvs
			FreePlayerPVS();

			// stgatilov #6546: work on light queries from game
			m_LightEstimateSystem->Think();
			if ( idMath::Abs( g_showLightQuotient.GetInteger() ) == 3 )
				m_LightEstimateSystem->DebugVisualize();

			if ( cv_music_volume.IsModified() ) {  //SnoopJeDi, fade that sound!
				float music_vol = cv_music_volume.GetFloat();
				for ( int i = 0; i < musicSpeakers.Num(); i++ ) {
					idSound* ent = static_cast<idSound *>(entities[ musicSpeakers[ i ] ]);
					// Printf( " Fading speaker %s (index %i) to %f\n", ent->name.c_str(), i, music_vol);
					if (ent)
						ent->Event_FadeSound( 0, music_vol, 0.5 );
				}
				cv_music_volume.ClearModified();
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

			if ( player ) {
				ret.health = player->health;

#ifdef PLAYER_HEARTBEAT
				ret.heartRate = player->heartRate;
#else
				ret.heartRate = 0; // not used
#endif // PLAYER_HEARTBEAT
				ret.stamina = idMath::FtoiRound( player->stamina );
				// combat is a 0-100 value based on lastHitTime and lastDmgTime
				// each make up 50% of the time spread over 10 seconds
				ret.combat = 0;
				if ( player->lastDmgTime > 0 && time < player->lastDmgTime + 10000 ) {
					ret.combat += int(50.0f * (float) ( time - player->lastDmgTime ) / 10000);
				}
				if ( player->lastHitTime > 0 && time < player->lastHitTime + 10000 ) {
					ret.combat += int(50.0f * (float) ( time - player->lastHitTime ) / 10000);
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

			/*if (m_DoLightgem) // duzenko #4408 - move this to idGameLocal::Draw where the second half of this already is
			{					// because 1) game tic could be on background thread and 2) it makes more sense 
				player->ProcessLightgem(cv_lg_hud.GetInteger() == 0);
				m_DoLightgem = false;
			}*/

		} while( ( inCinematic || ( time < cinematicStopTime ) ) && skipCinematic );
	}

	ret.syncNextGameFrame = skipCinematic;
	if ( skipCinematic ) {
		soundSystem->SetMute( false );
		skipCinematic = false;		
	}

	// show any debug info for this frame
	RunDebugInfo();
	D_DrawDebugLines();

	DM_LOG(LC_FRAME, LT_INFO)LOGSTRING("Frame end %d - %d: all:%.1f th:%.1f ev:%.1f %d ents \r", 
		time, timer_think.Milliseconds() + timer_events.Milliseconds(),
		timer_think.Milliseconds(), timer_events.Milliseconds(), num );

	g_Global.m_Frame = 0;

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
	int viewX = r_customWidth.GetInteger();
	int viewY = r_customHeight.GetInteger();

	if (viewX <= 0 || viewY <= 0)
		Error( "idGameLocal::CalcFov: screen dimensions are %d x %d <= 0", viewX, viewY );
	if (base_fov <= 1e-3f || base_fov >= 180.0f - 1e-3f)
		Error( "idGameLocal::CalcFov: bad base value %f", base_fov );

	//stgatilov: simulating the formula of TDM 2.07 and earlier
	//g_fov specifies horizontal FOV or a virtual 4:3 screen inscribed into the real screen
	float cvarFov = DEG2RAD(base_fov);
	float fovY = idMath::ATan(idMath::Tan(cvarFov * 0.5f) * 0.75) * 2.0f;
	//float fovY = DEG2RAD(base_fov);

	float aspect = float(viewX) / float(viewY);
	float ratioY = idMath::Tan(fovY * 0.5f);
	float ratioX = ratioY * aspect;
	float fovX = idMath::ATan(ratioX) * 2.0f;

	fov_x = RAD2DEG(fovX);
	fov_y = RAD2DEG(fovY);
}

/*
================
idGameLocal::Draw

makes rendering and sound system calls
================
*/
bool idGameLocal::Draw( int clientNum )
{
	idPlayer *player = static_cast<idPlayer *>(entities[ clientNum ]);

	if ( !player ) {
		return false;
	}

	// render the scene
	player->playerView.RenderPlayerView(player->hud);

	return true;
}

void idGameLocal::DrawLightgem( int clientNum ) {
	idPlayer *player = static_cast<idPlayer *>(entities[clientNum]);
	if (!player) {
		return;
	}
	player->ProcessLightgem( true );
}

/*
================
idGameLocal::HandleESC
================
*/
escReply_t idGameLocal::HandleESC( idUserInterface **gui ) {

	// If we're in the process of ending the mission, ignore all ESC keys.
	if (GameState() == GAMESTATE_COMPLETED) {
		return ESC_IGNORE;
	}

	// greebo: Hitting the ESC key means that the main menu is about to be entered => stop the timer
	m_GamePlayTimer.Stop();

	idPlayer *player = GetLocalPlayer();
	if ( player ) {
		if ( player->HandleESC() ) {
			m_GamePlayTimer.SetEnabled(true);
			return ESC_IGNORE;
		} else {
			return ESC_MAIN;
		}
	}
	return ESC_MAIN;
}

/*
================
idGameLocal::HandleGuiCommands
================
*/
const char* idGameLocal::HandleGuiCommands( const char *menuCommand ) {
	return NULL;
}

int idGameLocal::LoadVideosFromString(const char* videosStr, const char* lengthStr, idList<BriefingVideoPart>& targetList)
{
	targetList.Clear();

	std::string videos = videosStr;
	std::string lengths = lengthStr;

	// Remove leading and trailing semicolons
	stdext::trim(videos, ";");
	stdext::trim(lengths, ";");

	// Split strings into parts
	std::vector<std::string> vidParts;
	std::vector<std::string> lengthParts;

	stdext::split(vidParts, videos, ";");
	stdext::split(lengthParts, lengths, ";");

	if (vidParts.size() != lengthParts.size())
	{
		gameLocal.Warning("The video string array '%s' does not have the same number of elements as the length array '%s'!", videosStr, lengthStr);
		return 0;
	}

	if (vidParts.size() == 0)
	{
		return 0;
	}

	int totalLength = 0;

	for (std::size_t i = 0; i < vidParts.size(); ++i)
	{
		BriefingVideoPart& part = targetList.Alloc();

		part.material = vidParts[i].c_str();
		part.lengthMsec = atoi(lengthParts[i].c_str());

		if (part.lengthMsec <= 0)
		{
			gameLocal.Warning("The video length %s is invalid or not numeric, must be an integer > 0", lengthParts[i].c_str());
			part.lengthMsec = 0;
		}

		totalLength += part.lengthMsec;
	}

	return totalLength;
}

/* for aspect ratios, see mainmenu_settings_video.gui:
choiceDef Screensize4to3	0
choiceDef Screensize16to9	1
choiceDef Screensize16to10	2
choiceDef Screensize5to4	3
choiceDef ScreensizeTV16to9	4
*/
struct VideoMode {
	int aspect;				//r_aspectRatio number
	int width, height;		//r_customWidth, r_customHeight
};
//note: indexed by cv_tdm_widescreenmode
VideoMode VideoModes[] = {
	{2,  1024,  600 },  //0
	{2,  1280,  800 },  //1
	{2,  1440,  900 },  //2
	{2,  1680,  1050 }, //3
	{2,  1920,  1200 }, //4
	{1,  1366,  768 },	//5
	{1,  1280,  720 },	//6
	{1,  1920,	1080 },	//7
	{1,  2560,	1440 },	//8
	{2,  2560,	1600 },	//9
	{3,  1280,	1024 },	//10
	{3,  1800,	1440 },	//11
	{3,  2560,	2048 },	//12
	{4,  1360,	768  },	//13
	{1,  1600,	900  },	//14
	{2,  3280,	2048 },	//15
	{2,  3360,	2100 },	//16
	{1,  3840,	2160 },	//17
	{2,  3840,	2400 },	//18
	{-1,  320,	200  },	//19
	{-1,  400,	300  },	//20
	{-1,  512,	384  },	//21
	{0,  640,	480  },	//22
	{0,  800,	600  },	//23
	{0,  1024,	768  },	//24
	{0,  1152,	864  },	//25
	{0,  1280,	960  },	//26
	{0,  1600,	1200 },	//27
	{5,  2560,	1080 },	//28
	{5,  3440,	1440 },	//29
	{5,  3840,	1600 }, //30
	{5,  5120,	2160 },	//31
};
static int VideoModesNum = ( sizeof(VideoModes) / sizeof(VideoModes[0]) );

void idGameLocal::UpdateScreenResolutionFromGUI(idUserInterface* gui)
{
	// Set the custom height and width
	int mode = cv_tdm_widescreenmode.GetInteger();
	if (mode < 0 || mode >= VideoModesNum) {
		Printf("widescreenmode %i: out of range\n", mode);
		return;
	}
	auto vm = VideoModes[mode];
	int width = vm.width;
	int height = vm.height;
	int aspect = vm.aspect;

	Printf("widescreenmode %i: setting r_customWidth|r_customHeight = %ix%i, r_aspectRatio = %i\n", mode, width, height, aspect);
	r_customWidth.SetInteger(width);
	r_customHeight.SetInteger(height);
	r_aspectRatio.SetInteger(aspect);
}

void idGameLocal::UpdateWidescreenModeFromScreenResolution(idUserInterface* gui)
{
	int width = r_customWidth.GetInteger();
	int height = r_customHeight.GetInteger();

	int mode = -1;
	for (int m = 0; m < VideoModesNum; m++) {
		auto vm = VideoModes[m];
		if (width == vm.width && height == vm.height)
			mode = m;
	}
	int aspect = (mode < 0 ? 0 : VideoModes[mode].aspect);

	if (cv_tdm_widescreenmode.GetInteger() != mode || r_aspectRatio.GetInteger() != aspect)
		Printf("r_customWidth|Height = %ix%i => widescreenmode = %i, r_aspectRatio = %i\n", width, height, mode, aspect );
	cv_tdm_widescreenmode.SetInteger(mode);
	r_aspectRatio.SetInteger(aspect);
}

void idGameLocal::HandleGuiMessages(idUserInterface* ui)
{
	//stgatilov #5661: don't drop message until previous one is closed
	if (ui->GetStateBool("MsgBoxVisible"))
		return;
	if (m_GuiMessages.Num() == 0)
		return;

	const GuiMessage& msg = m_GuiMessages[0];

	ui->SetStateBool("MsgBoxVisible", true);

	ui->SetStateString("MsgBoxTitle", msg.title);
	ui->SetStateString("MsgBoxText", msg.message);

	switch (msg.type)
	{
	case GuiMessage::MSG_OK:
		ui->SetStateBool("MsgBoxLeftButtonVisible", false);
		ui->SetStateBool("MsgBoxRightButtonVisible", false);
		ui->SetStateBool("MsgBoxMiddleButtonVisible", true);
		ui->SetStateString("MsgBoxMiddleButtonText", common->Translate("#str_07188"));	// OK
		break;
	case GuiMessage::MSG_OK_CANCEL:
		ui->SetStateBool("MsgBoxLeftButtonVisible", true);
		ui->SetStateBool("MsgBoxRightButtonVisible", true);
		ui->SetStateBool("MsgBoxMiddleButtonVisible", false);

		ui->SetStateString("MsgBoxLeftButtonText", common->Translate("#str_07188"));	// OK
		ui->SetStateString("MsgBoxRightButtonText", common->Translate("#str_07203"));	// Cancel
		break;
	case GuiMessage::MSG_YES_NO:
		ui->SetStateBool("MsgBoxLeftButtonVisible", true);
		ui->SetStateBool("MsgBoxRightButtonVisible", true);
		ui->SetStateBool("MsgBoxMiddleButtonVisible", false);

		ui->SetStateString("MsgBoxLeftButtonText", common->Translate("#str_02501"));	// Yes
		ui->SetStateString("MsgBoxRightButtonText", common->Translate("#str_02502"));	// No
		break;
	case GuiMessage::MSG_CUSTOM:
		// Left button
		if (!msg.positiveLabel.IsEmpty())
		{
			ui->SetStateBool("MsgBoxLeftButtonVisible", true);
			ui->SetStateString("MsgBoxLeftButtonText", msg.positiveLabel);
		} else
			ui->SetStateBool("MsgBoxLeftButtonVisible", false);
		// Middle button
		if (!msg.okLabel.IsEmpty())
		{
			ui->SetStateBool("MsgBoxMiddleButtonVisible", true);
			ui->SetStateString("MsgBoxMiddleButtonText", msg.okLabel);
		} else
			ui->SetStateBool("MsgBoxMiddleButtonVisible", false);
		// Right button
		if (!msg.negativeLabel.IsEmpty())
		{
			ui->SetStateBool("MsgBoxRightButtonVisible", true);
			ui->SetStateString("MsgBoxRightButtonText", msg.negativeLabel);
		} else
			ui->SetStateBool("MsgBoxRightButtonVisible", false);
		break;
	};

	ui->SetStateString("MsgBoxRightButtonCmd", msg.negativeCmd);
	ui->SetStateString("MsgBoxLeftButtonCmd", msg.positiveCmd);
	ui->SetStateString("MsgBoxMiddleButtonCmd", msg.okCmd);

	m_GuiMessages.RemoveIndex(0);
}

/*
================
idGameLocal::UpdateGUIScaling
================
*/
void idGameLocal::UpdateGUIScaling( idUserInterface *gui )
{
	
	float wobble = 1.0f;
	// this could be turned into a warble-wobble effect f.i. for player poisoned etc.
	//float wobble = random.RandomFloat() * 0.02 + 0.98;

	float x_mul = cvarSystem->GetCVarFloat("gui_Width") * wobble;
	if (x_mul < 0.1f) { x_mul = 0.1f; }
	if (x_mul > 2.0f) { x_mul = 2.0f; }
	float y_mul = cvarSystem->GetCVarFloat("gui_Height") * wobble;
	if (y_mul < 0.1f) { y_mul = 0.1f; }
	if (y_mul > 2.0f) { y_mul = 2.0f; }
	float x_shift = cvarSystem->GetCVarFloat("gui_CenterX") * 640 * wobble;
	float y_shift = cvarSystem->GetCVarFloat("gui_CenterY") * 480 * wobble;
//	Printf("UpdateGUIScaling: width %0.2f height %0.2f centerX %0.02ff centerY %0.02f\n (%p)", x_mul, y_mul, x_shift, y_shift, gui);
	gui->SetStateFloat("LEFT", x_shift - x_mul * 320);
	gui->SetStateFloat("CENTERX", x_shift);
	gui->SetStateFloat("TOP", y_shift - y_mul * 240);
	gui->SetStateFloat("CENTERY", y_shift);
	gui->SetStateFloat("WIDTH", x_mul);
	gui->SetStateFloat("HEIGHT", y_mul);
	// We have only one scaling factor to scale text, so use the average of X and Y
	// This will have odd effects if W and H differ greatly, but is better than to not scale the text
	gui->SetStateFloat("SCALE", (y_mul + x_mul) / 2);	
	
	/*
	gui->SetStateFloat("iconSize", cvarSystem->GetCVarFloat("gui_iconSize"));
	gui->SetStateFloat("smallTextSize", cvarSystem->GetCVarFloat("gui_smallTextSize"));
	gui->SetStateFloat("bigTextSize", cvarSystem->GetCVarFloat("gui_bigTextSize"));
	gui->SetStateFloat("lightgemSize", cvarSystem->GetCVarFloat("gui_lightgemSize"));
	gui->SetStateFloat("barSize", cvarSystem->GetCVarFloat("gui_barSize"));
	*/
}

/*
================
idGameLocal::HandleMainMenuCommands
================
*/
void idGameLocal::HandleMainMenuCommands( const char *menuCommand, idUserInterface *gui )
{
	/* Tels: This routine here is called for every "command" in the menu GUI. Unfortunately,
	   		 the interpretation of what is a command works a bit strange (as almost everything
			 else does in idTech4 :)
		
		In the following examples, there is usually another call to this routine with ";"
		as the "menuCommand" param. But sometimes it does not appear, depending on whoknows.
		(stgatilov 2.09: outer code breaks full cmd string into tokens and passes every token here)

		The following will cause this routine called twice (!), once with "mainmenu_heartbeat"
		and once with ";" as command (if you add ";" before the last double quote, it will
		still be called only twice):

	   		set "cmd" "mainmenu_heartbeat";

		This will call it twice (or three times counting ";") with "mycommand" "myarg" and ";":

			set "cmd" "mycommand 'test'";

		This calls it only once (!) (two times counting ";"), with "play":

			Set "play mymusic";

		This calls it twice (three times counting ";"), with "notime" and then "1":

			Set "noTime" "1";

		Note that the command buffer has only a certain, unknown length, and if too many
		commands are issued in the same timeframe (onTime X..), then the buffer will overflow,
		and certain commands might get cut-off. Usually this means that you see a ";" instead
		of an expected argument, or that commands run together like "initChoicelog". When this
		happens, the GUI must be adjusted to issue less commands (or issue them later).
		To work around this issue, we call ExecuteCommandBuffer( void ) whenever we see an
		initChoice command, in the hopes that the buffer won't overflow until the next one.

		(stgatilov 2.09: the sporadic words contatentation was caused by a bug in cmd merging code.
		Among several places which merge command strings, most put a semicolon and spaces in-between.
		But one piece of code did not have it --- end of idWindow::Time)

		The old code simply watched for certain words, and then issued the command. It was not
		able to distinguish between commands and arguments.
		(stgatilov 2.09: unfortunately, the outer code still does it now for builtin keywords)

		This routine now watches for the first command, ignoring any stray ";". If it sees
		a command, it deduces the number of following arguments, and then collects them until
		it has all of them, warning if there is a ";" coming unexpectedly.

		Once all arguments are collected, the command is finally handled, or silently ignored.
	*/
	if (!menuCommand || menuCommand[0] == 0x00)
	{
		// silently ignore
		Warning("Seen NULL MainMenu cmd.");
		return;
	}

	int numArgs = m_GUICommandStack.Num();

	if (numArgs == 0)
	{
		if ( menuCommand[0] == ';' && menuCommand[1] == 0x0 )
		{
			// ignore stray any ";"
			return;
		}

		// have not seen anything?
		idStr cmd = menuCommand;
		// commands can appear in any case (like "notime", "noTime" etc)
		cmd.ToLower();
		m_GUICommandStack.Append(cmd);

		// set the number of wanted args, default is none
		m_GUICommandArgs = 0;

		// "log" and "notime" take one argument
		if ( cmd == "log" || cmd == "notime")
		{
			m_GUICommandArgs = 1;
		}

		// this takes one argument
		if ( cmd == "setlang")
		{
			m_GUICommandArgs = 1;
		}

		//if ( cmd != "log" && cmd != "mainmenu_heartbeat")
		//{
			//Printf("Seen command '%s' ('%s'), takes %i args.\n", menuCommand, cmd.c_str(), m_GUICommandArgs );
		//}
	}
	else
	{
		// append the current argument to the stack (but not if it is ";")
		if ( menuCommand[0] != ';' || menuCommand[1] != 0x0 )
		{
			m_GUICommandStack.Alloc() = menuCommand;
			//Printf("  Seen argument '%s' for '%s'\n", menuCommand, m_GUICommandStack[0].c_str() );
		}
		else
		{
			// seen a ";" even tho we have not yet as many arguments as we wanted?
			if (numArgs - 1 < m_GUICommandArgs)
			{
				//Warning("MainMenu command %s takes %i arguments, but has only %i (last arg %s).\n", m_GUICommandStack[0].c_str(), m_GUICommandArgs, numArgs - 1, menuCommand);
				Warning("Seen stray ';' - command %s takes %i arguments, but has only %i (last arg %s).\n", m_GUICommandStack[0].c_str(), m_GUICommandArgs, numArgs - 1, menuCommand);
				// Ignore any stray ";" as they can be injected anywhere :(
				numArgs = m_GUICommandArgs;
				return;
			}
		}
	}

//	 Printf(" have %i vs wanted %i\n", numArgs, m_GUICommandArgs);
	// do we have already as much arguments as we want?
	if (numArgs < m_GUICommandArgs)
	{
		// no, wait for the next argument
		return;
	}

	// the command
	const idStr& cmd = m_GUICommandStack[0];

	// seen the command and its arguments, now handle it

	/*
	if ( ( cmd != "mainmenu_heartbeat" ) && ( cmd != "briefingvideoheartbeat" ) && ( cmd != "log" ) )
	{
		// debug output, but ignore the frequent ones
		Printf("MainMenu cmd: %s (%i args)\n", cmd.c_str(), numArgs);
	}
	*/

	// Watch out for objectives GUI-related commands
	// TODO: Handle here commands with arguments, too.
	m_MissionData->HandleMainMenuCommands(cmd, gui);

	if (cmd == "mainmenu_heartbeat")
	{
		// greebo: Stop the timer, this is already done in HandleESC, but just to make sure...
		m_GamePlayTimer.Stop();

		if (GetMissionResult() == MISSION_COMPLETE)
		{
			/*
			// Check if we should show the success screen (also check the member variable
			// to catch cases where the player reloaded the map via the console)
			if (!gui->GetStateBool("PostMissionScreenActive") || !postMissionScreenActive)
			{
				// Check if this mission has a debriefing video
				int missionNum = m_MissionManager->GetCurrentMissionIndex() + 1; // GUI mission numbers are 1-based

				const char* videoMaterials = gui->GetStateString(va("DebriefingVideoMaterials%d", missionNum));
				const char* videoLengths = gui->GetStateString(va("DebriefingVideoLengths%d", missionNum));
				const char* videoSoundCmd = gui->GetStateString(va("DebriefingVideoSoundCmd%d", missionNum));

				// Calculate the total length of the video
				int videoLengthMsec = LoadVideosFromString(videoMaterials, videoLengths, debriefingVideo);

				gui->SetStateInt("DebriefingVideoLength", videoLengthMsec);
				gui->SetStateString("DebriefingVideoSoundCmd", videoSoundCmd);
				
				// Let the GUI know whether we have a debriefing video
				gui->SetStateBool("HasDebriefingVideo", debriefingVideo.Num() > 0);

		 		// Show the post-mission GUI (might be a debriefing video or just the success screen)
				gui->HandleNamedEvent("ShowPostMissionScreen");

				// Avoid duplicate triggering
				gui->SetStateBool("PostMissionScreenActive", true);
				postMissionScreenActive = true;
			}*/
			
			// tels: We handled this command, so clear the command stack
			m_GUICommandStack.Clear();
			m_GUICommandArgs = 0;
			return;
		}

		// Check if any errors are in the queue
		if (!m_guiError.IsEmpty())
		{
			// Send the error to the GUI
			gui->SetStateString("gameErrorMsg", m_guiError);
			gui->HandleNamedEvent("OnGameError");

			// Clear the string again
			m_guiError.Clear();
		}

		// Check messages
		if (m_GuiMessages.Num() > 0)
		{
			HandleGuiMessages(gui);
		}

		// Make sure we have a valid mission number in the GUI, the first mission is always 1
		int curMissionNum = gui->GetStateInt("CurrentMission");

		if (curMissionNum <= 0)
		{
			gui->SetStateInt("CurrentMission", 1);
		}

		// Handle downloads in progress
		m_DownloadManager->ProcessDownloads();

		// Propagate the mission list CVARs to the GUI
		if (cvarSystem->GetCVarInteger("tdm_mission_list_sort_direction") == 0) {
			gui->SetStateString("mission_list_direction", "guis/assets/mainmenu/sort_down");
		} else {
			gui->SetStateString("mission_list_direction", "guis/assets/mainmenu/sort_up");
		}
		if (cvarSystem->GetCVarInteger("tdm_download_list_sort_direction") == 0) {
			gui->SetStateString("download_list_direction", "guis/assets/mainmenu/sort_down");
		} else {
			gui->SetStateString("download_list_direction", "guis/assets/mainmenu/sort_up");
		}

		// Propagate the video CVARs to the GUI
		gui->SetStateInt("video_aspectratio", cvarSystem->GetCVarInteger("r_aspectRatio"));
		gui->SetStateBool("confirmQuit", cv_mainmenu_confirmquit.GetBool());
		gui->SetStateBool("menu_bg_music", cv_tdm_menu_music.GetBool());
		// Obsttorte
		gui->SetStateFloat("iconSize", cv_gui_iconSize.GetFloat());
		gui->SetStateFloat("smallTextSize", cv_gui_smallTextSize.GetFloat());
		gui->SetStateFloat("bigTextSize", cv_gui_bigTextSize.GetFloat());
		gui->SetStateFloat("lightgemSize", cv_gui_lightgemSize.GetFloat());
		gui->SetStateFloat("barSize", cv_gui_barSize.GetFloat());
		gui->SetStateFloat("objectiveTextSize", cv_gui_objectiveTextSize.GetFloat());
		gui->SetStateFloat("HUD_Opacity", cv_tdm_hud_opacity.GetFloat());
		if (cv_tdm_menu_music.IsModified())
		{
			gui->HandleNamedEvent("OnMenuMusicSettingChanged");

			cv_tdm_menu_music.ClearModified();
		}
	}
	else if (cmd == "mainmenumodeselect")
	{
		struct MainMenuStateInfo {
			idStr name;				//e.g. BRIEFING   (MM_STATE_ is prepended)
			idStr stateToggle;		//e.g. BriefingState -> BriefingStateInit + BriefingStateEnd
			idStr backgrounds;		//e.g. MAINMENI_NOTINGAME -> MM_BACKGROUNDS_MAINMENU_NOTINGAME
			idStr music;			//e.g. MusicIngame   (MainMenu is prepended)
		};
		struct MainMenuTransition {
			idStr from;				//e.g. BRIEFING
			idStr action;			//e.g. FORWARD
			idStr to;				//e.g. DIFF_SELECT
		};
		static const MainMenuStateInfo STATES[] = {
			{"NONE", NULL, NULL, NULL},
			{"MAINMENU", NULL, NULL, NULL},
			{"START_GAME", NULL, NULL, NULL},
			{"END_GAME", NULL, NULL, NULL},
			{"FINISHED", NULL, NULL, NULL},
			{"FORWARD", NULL, NULL, NULL},
			{"BACKWARD", NULL, NULL, NULL},
			{"MAINMENU_INGAME", "MainMenuInGameState", "MAINMENU_INGAME", "MusicIngame"},
			{"MAINMENU_NOTINGAME", "MainMenuState", "MAINMENU_NOTINGAME", "Music"},
			{"QUITGAME", "QuitGameState", "MAINMENU_%INGAME%", "Music%INGAME%"},
			{"CREDITS", "CreditsMenuState", "CREDITS", "MusicCredits"},
			{"LOAD_SAVE_GAME", "LoadSaveGameMenuState", "MAINMENU_%INGAME%", "Music%INGAME%"},
			{"FAILURE", "FailureMenuState", "FAILURE", "MusicMissionFailure"},
			{"SUCCESS", "SuccessScreenState", "SUCCESS", "MusicMissionSuccess"},
			{"BRIEFING", "BriefingState", "BRIEFING", "MusicBriefing"},
			{"BRIEFING_VIDEO", "BriefingVidState", "", "MusicBriefingVideo"},
			{"OBJECTIVES", "ObjectivesState", "EXTRAMENU_INGAME", "Music%INGAME%"},
			{"SHOP", "ShopMenuState", "SHOP", "MusicBriefing"},
			{"SETTINGS", "SettingsMenuState", "MAINMENU_%INGAME%", "Music%INGAME%"},
			{"SELECT_LANGUAGE", "SelectLanguageState", "MAINMENU_%INGAME%", "Music%INGAME%"},
			{"DOWNLOAD", "DownloadMissionsMenuState", "EXTRAMENU_NOTINGAME", "Music%INGAME%"},
			{"DEBRIEFING", "DebriefingState", "DEBRIEFING", "MusicDebriefing"},
			{"DEBRIEFING_VIDEO", "DebriefingVidState", "", "MusicDebriefingVideo"},
			{"GUISIZE", "SettingsGuiSizeState", "", "Music%INGAME%"},
			{"MOD_SELECT", "NewGameMenuState", "EXTRAMENU_NOTINGAME", "Music%INGAME%"},
			{"DIFF_SELECT", "ObjectivesState", "DIFFSELECT", "MusicBriefing"},
		};
		static const MainMenuTransition TRANSITIONS[] = {
			//standard FM-customized sequence: starting new game
			{"MAINMENU_NOTINGAME", "FORWARD", "BRIEFING_VIDEO"},	{"BRIEFING_VIDEO", "BACKWARD", "MAINMENU"},
			{"BRIEFING_VIDEO", "FORWARD", "BRIEFING"},				{"BRIEFING", "BACKWARD", "MAINMENU"},
			{"BRIEFING", "FORWARD", "DIFF_SELECT"},					{"DIFF_SELECT", "BACKWARD", "BRIEFING"},
			{"DIFF_SELECT", "FORWARD", "SHOP"},						{"SHOP", "BACKWARD", "DIFF_SELECT"},
			{"SHOP", "FORWARD", "START_GAME"},
			//standard FM-customized sequence: game finished successfully
			{"FINISHED", "FORWARD", "DEBRIEFING_VIDEO"},
			{"DEBRIEFING_VIDEO", "FORWARD", "DEBRIEFING"},		{"DEBRIEFING", "BACKWARD", "DEBRIEFING_VIDEO"},
			{"DEBRIEFING", "FORWARD", "SUCCESS"},				{"SUCCESS", "BACKWARD", "DEBRIEFING"},
			{"SUCCESS", "FORWARD", "MAINMENU"},
		};

		static const int STATENUM = sizeof(STATES) / sizeof(STATES[0]);
		static const int TRANSITIONNUM = sizeof(TRANSITIONS) / sizeof(TRANSITIONS[0]);
		auto FindStateByValue = [gui](int value) -> const MainMenuStateInfo* {
			for (int i = 0; i < STATENUM; i++) {
				idStr name = "#MM_STATE_" + STATES[i].name;
				if (value == gui->GetStateInt(name))
					return &STATES[i];
			}
			return nullptr;
		};
		auto FindStateByName = [gui](const char *name) -> const MainMenuStateInfo* {
			for (int i = 0; i < STATENUM; i++) {
				if (STATES[i].name == name)
					return &STATES[i];
			}
			return nullptr;
		};
		auto ShouldSkipState = [gui](const char *name) -> bool {
			idStr defName = idStr("#ENABLE_MAINMENU_") + name;
			int value = gui->GetStateInt(defName, "-1");
			if (value == 0)
				return true;		//#define ENABLE_MAINMENU_SHOP 0  (set by mapper)
			if (idStr::Icmp(name, "SHOP") == 0 && gui->GetStateInt("SkipShop") != 0)
				return true;		//gui::SkipShop = 1    (set by C++ code)
			return false;
		};
		auto FindTransition = [&](const char *from, const char *action) -> const char* {
			for (int i = 0; i < TRANSITIONNUM; i++) {
				if (TRANSITIONS[i].from == from && TRANSITIONS[i].action == action)
					return TRANSITIONS[i].to.c_str();
			}
			return nullptr;
		};

		int modeValue = gui->GetStateInt("mode");
		int targetValue = gui->GetStateInt("targetmode");
		auto modeState = FindStateByValue(modeValue);
		auto targetState = FindStateByValue(targetValue);
		if (!modeState) {
			DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Unknown current state %d, setting NONE", modeValue);
			modeState = &STATES[0];
		}
		if (!targetState) {
			DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Unknown target state %d, setting NONE", modeValue);
			targetState = &STATES[0];
		}

		if (const char *dest = FindTransition(modeState->name, targetState->name)) {
			//this is "action" of transition, i.e. not a "target" state, but more like a "direction" of movement
			DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Detected transition %s from %s", targetState->name.c_str(), modeState->name.c_str());
			while (dest && ShouldSkipState(dest)) {
				DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Skipped disabled state %s", dest);
				dest = FindTransition(dest, targetState->name);
			}
			if (dest) {
				DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Transition ends at %s", dest);
			} else {
				DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("All states in transition chain are disabled, redirect to MAINMENU");
				dest = "MAINMENU";
			}
			targetState = FindStateByName(dest);
		}

		auto Redirect = [&](const char *newTargetState) {
			DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Target state %s, redirecting to %s", targetState->name.c_str(), newTargetState);
			targetState = FindStateByName(newTargetState);
			assert(targetState);
		};

		if (ShouldSkipState(targetState->name)) {
			DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Target state %s is disabled, redirecting to MAINMENU", targetState->name.c_str());
			targetState = FindStateByName("MAINMENU");
		}
		if (targetState->name == "NONE")
			Redirect("MAINMENU");
		if (targetState->name == "MAINMENU") {
			if (gui->GetStateInt("inGame"))
				Redirect("MAINMENU_INGAME");
			else
				Redirect("MAINMENU_NOTINGAME");
		}

		if (targetState != modeState) {
			if (targetState->name == "START_GAME") {
				DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Starting game");
				//stgatilov #6509: transfer persistent info early, so that we can read map name from it now
				SyncPersistentInfoFromGui(gui, true);
				//select which map to load
				idStr customMapName = persistentLevelInfo.GetString("builtin_startMap");
				idStr mapName;
				if (customMapName[0]) {
					mapName = customMapName;
				} else {
					mapName = m_MissionManager->GetCurrentStartingMap();
				}
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va("map %s\n", mapName.c_str()) );
				//note: it seems that target state does not matter
				//map start resets "mode" to NONE anyway (see ClearMainMenuMode)
			}
			if (targetState->name == "END_GAME") {
				DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Ending game");
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, va("disconnect\n") );
				Redirect("MAINMENU_NOTINGAME");
			}

			idStr modeMusicState = gui->GetStateString("MusicLastState");
			idStr targetMusicState = targetState->music;
			if (targetMusicState == "Music%INGAME%") {
				//special syntax to enable MENU/MENU_INGAME
				if (gui->GetStateInt("inGame"))
					targetMusicState = FindStateByName("MAINMENU_INGAME")->music;
				else
					targetMusicState = FindStateByName("MAINMENU_NOTINGAME")->music;
			}
			if (gui->GetStateInt("menu_bg_music") == 0 && (targetMusicState == FindStateByName("MAINMENU_NOTINGAME")->music || targetMusicState == FindStateByName("MAINMENU_INGAME")->music))
				targetMusicState = "";				//disable music
			if (targetMusicState == "")
				targetMusicState = "MusicNone";		//no music requested
			if (modeMusicState != targetMusicState) {
				DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Starting music state %s", targetMusicState.c_str() );
				gui->ResetWindowTime("MainMenu" + targetMusicState);
				gui->SetStateString("MusicLastState", targetMusicState);
			}
			else {
				DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("No music change: state %s", targetMusicState.c_str() );
			}

			idStr targetBackgroundsMacro = targetState->backgrounds;
			if (targetBackgroundsMacro == "MAINMENU_%INGAME%") {
				//special syntax to enable MENU/MENU_INGAME
				if (gui->GetStateInt("inGame"))
					targetBackgroundsMacro = FindStateByName("MAINMENU_INGAME")->backgrounds;
				else
					targetBackgroundsMacro = FindStateByName("MAINMENU_NOTINGAME")->backgrounds;
			}
			idStr targetBackgroundsLayersStr = gui->GetStateString("#MM_BACKGROUNDS_" + targetBackgroundsMacro, "");
			while (targetBackgroundsLayersStr.CmpPrefix("MM_BACKGROUNDS_") == 0) {
				//usually we only read plain values of macros, but this limitation is a problem for backgrounds meta states
				//so we allow setting name of another backgrounds macro as value
				targetBackgroundsLayersStr = gui->GetStateString("#" + targetBackgroundsLayersStr, "");
			}
			idStr backgroundsLayersStr = gui->GetStateString("backgrounds");
			if (backgroundsLayersStr != targetBackgroundsLayersStr) {
				idStrList backgroundsLayers = backgroundsLayersStr.Split(",", true);
				idStrList targetBackgroundsLayers = targetBackgroundsLayersStr.Split(",", true);
				for (int i = 0; i < backgroundsLayers.Num(); i++) {
					if (!targetBackgroundsLayers.Find(backgroundsLayers[i])) {
						DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Ending background layer %s", backgroundsLayers[i].c_str() );
						gui->ResetWindowTime(backgroundsLayers[i] + "End", 0);
					}
					else
						DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("No change: background layer %s", backgroundsLayers[i].c_str() );
				}
				for (int i = 0; i < targetBackgroundsLayers.Num(); i++) {
					if (!backgroundsLayers.Find(targetBackgroundsLayers[i])) {
						DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Starting background layer %s", targetBackgroundsLayers[i].c_str() );
						gui->ResetWindowTime(targetBackgroundsLayers[i] + "Init", 0);
					}
				}
				gui->SetStateString("backgrounds", targetBackgroundsLayersStr);
			}

			DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Ending state %s", modeState->name.c_str() );
			if (modeState->stateToggle)
				gui->ResetWindowTime(modeState->stateToggle + "End", 0);
			DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("Starting state %s", targetState->name.c_str() );
			if (targetState->stateToggle)
				gui->ResetWindowTime(targetState->stateToggle + "Init", 0);

			targetValue = gui->GetStateInt("#MM_STATE_" + targetState->name);
			gui->SetStateInt("mode", targetValue);
		}
	}
	else if (cmd == "setvideoreswidescreen")
	{
		// Called when "screen size" is changed (i.e. cv_tdm_widescreenmode)
		// Update resolution and aspect cvars accordingly
		UpdateScreenResolutionFromGUI(gui);
		UpdateGUIScaling(gui);
	}
	else if (cmd == "aspectratiochanged")
	{
		// Called when "aspect ratio" is changed
		// Enable any mode with such ratio
		int aspect = r_aspectRatio.GetInteger();
		for (int m = 0; m < VideoModesNum; m++) {
			auto vm = VideoModes[m];
			if (aspect == vm.aspect) {
				cv_tdm_widescreenmode.SetInteger(m);
				break;
			}
		}
		UpdateScreenResolutionFromGUI(gui);
		UpdateGUIScaling(gui);
	}
	else if (cmd == "loadcustomvideoresolution")
	{
		// Called during engine initialization
		// Take r_customWidth x r_customHeight, and find mode and aspect ratio
		UpdateWidescreenModeFromScreenResolution(gui);
		UpdateGUIScaling(gui);
	}
	// greebo: the "log" command is used to write stuff to the console
	else if (cmd == "log")
	{
		// We should log the given argument
		if (cv_debug_mainmenu.GetBool())
		{
			Printf("MainMenu: %s\n", m_GUICommandStack[1].c_str() );
		}

		DM_LOG(LC_MAINMENU, LT_INFO)LOGSTRING("%s\r", m_GUICommandStack[1].c_str() );
	}	
	else if (cmd == "close") 
	{
		// Solarsplace: fix for #2424:
		mainMenuExited = true;

		// Start the timer again, we're closing the menu
		m_GamePlayTimer.Start();
	}
	else if (cmd == "loadvideodefinitions")
	{
		// Check if we've set up the briefing video 
		if (!briefingVideoInfoLoaded)
		{
			briefingVideoInfoLoaded = true;

			// Tell the briefing video GUI to load all video materials into the state dict
			gui->HandleNamedEvent("LoadVideoDefinitions");
			gui->HandleNamedEvent("LoadDebriefingVideoDefinitions");
		}
	}
	else if (cmd == "preparebriefingvideo")
	{
		// Ensure we've set up the briefing video (should already be done in "loadVideoDefinitions")
		assert(briefingVideoInfoLoaded);

		// Check the video defs
		int missionNum = m_MissionManager->GetCurrentMissionIndex() + 1;

		const char* videoMaterials = gui->GetStateString(va("BriefingVideoMaterials%d", missionNum));
		const char* videoLengths = gui->GetStateString(va("BriefingVideoLengths%d", missionNum));
		const char* videoSoundCmd = gui->GetStateString(va("BriefingVideoSoundCmd%d", missionNum));

		// Calculate the total length of the video
		int videoLengthMsec = LoadVideosFromString(videoMaterials, videoLengths, briefingVideo);

		gui->SetStateInt("BriefingVideoLength", videoLengthMsec);
		gui->SetStateString("BriefingVideoSoundCmd", videoSoundCmd);
		
		// We start with the first part
		curBriefingVideoPart = 0;
	}
	else if (cmd == "startbriefingvideo")
	{
		if (curBriefingVideoPart >= 0 && curBriefingVideoPart < briefingVideo.Num())
		{
			gui->SetStateString("BriefingVideoMaterial", briefingVideo[curBriefingVideoPart].material);

			// Let the video windowDef reset its cinematics
			gui->HandleNamedEvent("OnBriefingVideoPartChanged");
		}
		else
		{
			// No video
			gui->HandleNamedEvent("OnBriefingVideoFinished");
		}
	}
	else if (cmd == "briefingvideoheartbeat")
	{
		if (curBriefingVideoPart >= 0 && curBriefingVideoPart < briefingVideo.Num())
		{
			int videoTime = gui->GetStateInt("briefingVideoTime");

			videoTime += 16;

			if (videoTime >= briefingVideo[curBriefingVideoPart].lengthMsec)
			{
				curBriefingVideoPart++;

				// Briefing video part time exceeded
				if (curBriefingVideoPart < briefingVideo.Num())
				{
					// Switch to the next
					gui->SetStateString("BriefingVideoMaterial", briefingVideo[curBriefingVideoPart].material);
					gui->HandleNamedEvent("OnBriefingVideoPartChanged");

					// Each part starts at 0 again
					videoTime = 0;
				}
				else
				{
					// We're done with the video
					gui->HandleNamedEvent("OnBriefingVideoFinished");
				}
			}

			gui->SetStateInt("briefingVideoTime", videoTime);	
		}
	}
	else if (cmd == "preparedebriefingvideo")
	{
		// Ensure we've set up the debriefing video (should already be done in "loadVideoDefinitions")
		assert(briefingVideoInfoLoaded);
		
		// We start with the first part
		curDebriefingVideoPart = 0;
	}
	else if (cmd == "startdebriefingvideo")
	{
		if (curDebriefingVideoPart >= 0 && curDebriefingVideoPart < debriefingVideo.Num())
		{
			gui->SetStateString("DebriefingVideoMaterial", debriefingVideo[curDebriefingVideoPart].material);

			// Let the video windowDef reset its cinematics
			gui->HandleNamedEvent("OnDebriefingVideoPartChanged");
		}
		else
		{
			// No video
			gui->HandleNamedEvent("OnDebriefingVideoFinished");
		}
	}
	else if (cmd == "debriefingvideoheartbeat")
	{
		if (curDebriefingVideoPart >= 0 && curDebriefingVideoPart < debriefingVideo.Num())
		{
			int videoTime = gui->GetStateInt("debriefingVideoTime");

			videoTime += 16;

			if (videoTime >= debriefingVideo[curDebriefingVideoPart].lengthMsec)
			{
				curDebriefingVideoPart++;

				// Briefing video part time exceeded
				if (curDebriefingVideoPart < debriefingVideo.Num())
				{
					// Switch to the next
					gui->SetStateString("DebriefingVideoMaterial", debriefingVideo[curDebriefingVideoPart].material);
					gui->HandleNamedEvent("OnDebriefingVideoPartChanged");

					// Each part starts at 0 again
					videoTime = 0;
				}
				else
				{
					// We're done with the video
					gui->HandleNamedEvent("OnDebriefingVideoFinished");
				}
			}

			gui->SetStateInt("debriefingVideoTime", videoTime);	
		}
	}
	else if (cmd == "onsuccessscreencontinueclicked")
	{
		// Clear the mission result flag
		SetMissionResult(MISSION_NOTEVENSTARTED);

		// Set the boolean back to false for the next map start
		gui->SetStateBool("PostMissionScreenActive", false);
		postMissionScreenActive = false;

		// Switch to the next mission if there is one
		if (m_MissionManager->NextMissionAvailable())
		{
			m_MissionManager->ProceedToNextMission();

			// Recreate main menu GUI
			session->ResetMainMenu();
			session->SetMainMenuStartAtBriefing();
			session->StartMenu();
		}
		else
		{
			// Switch back to the main menu
			gui->HandleNamedEvent("SuccessGoBackToMainMenu");
		}
	}
	else if (cmd == "setlpdifficulty")
	{
		// Lockpicking difficulty setting changed, update CVARs
		int setting = gui->GetStateInt("lp_difficulty", "-1");
		
		switch (setting)
		{
		case 0: // Trainer
			cv_lp_auto_pick.SetBool(false);
			cv_lp_pick_timeout.SetInteger(750);
			break;
		case 1: // Average
			cv_lp_auto_pick.SetBool(false);
			cv_lp_pick_timeout.SetInteger(500);
			break;
		case 2: // Hard
			cv_lp_auto_pick.SetBool(false);
			cv_lp_pick_timeout.SetInteger(400);
			break;
		case 3: // Expert
			cv_lp_auto_pick.SetBool(false);
			cv_lp_pick_timeout.SetInteger(300);
			break;
		case 4: // Automatic
			cv_lp_auto_pick.SetBool(true);
			cv_lp_pick_timeout.SetInteger(300); // auto-LP is using expert timeout
			break;
		default:
			gameLocal.Warning("Unknown value for lockpicking difficulty encountered!");
		};
	}
	else if (cmd == "setdefaultspeed") // Daft Mugi #6311
	{
		int defaultSpeed = gui->GetStateInt("defaultspeed", "-1");
		switch (defaultSpeed)
		{
		case 0: // Walk
			cvarSystem->SetCVarBool("in_alwaysRun", false);
			cvarSystem->SetCVarBool("in_toggleRun", false);
			break;
		case 1: // Run
			cvarSystem->SetCVarBool("in_alwaysRun", true);
			cvarSystem->SetCVarBool("in_toggleRun", false);
			break;
		case 2: // Toggle
			cvarSystem->SetCVarBool("in_alwaysRun", false);
			cvarSystem->SetCVarBool("in_toggleRun", true);
			break;
		default:
			gameLocal.Warning("Unknown value for defaultspeed encountered!");
		};
	}
	else if (cmd == "set_frob_helper")
	{
		const int frobHelperPreset = gui->GetStateInt("frob_helper_preset", "-1");
		switch (frobHelperPreset)
		{
		case 0: // Off
			cvarSystem->SetCVarBool("tdm_frobhelper_active", false);
			cvarSystem->SetCVarBool("tdm_frobhelper_alwaysVisible", false);
			cvarSystem->SetCVarInteger("tdm_frobhelper_fadein_delay", 500);     // default value
			cvarSystem->SetCVarInteger("tdm_frobhelper_fadein_duration", 1500); // default value
			break;
		case 1: // Always
			cvarSystem->SetCVarBool("tdm_frobhelper_active", true);
			cvarSystem->SetCVarBool("tdm_frobhelper_alwaysVisible", true);
			cvarSystem->SetCVarInteger("tdm_frobhelper_fadein_delay", 500);     // default value
			cvarSystem->SetCVarInteger("tdm_frobhelper_fadein_duration", 1500); // default value
			break;
		case 2: // Hover
			cvarSystem->SetCVarBool("tdm_frobhelper_active", true);
			cvarSystem->SetCVarBool("tdm_frobhelper_alwaysVisible", false);
			cvarSystem->SetCVarInteger("tdm_frobhelper_fadein_delay", 0);
			cvarSystem->SetCVarInteger("tdm_frobhelper_fadein_duration", 0);
			break;
		case 3: // Fade In
			cvarSystem->SetCVarBool("tdm_frobhelper_active", true);
			cvarSystem->SetCVarBool("tdm_frobhelper_alwaysVisible", false);
			cvarSystem->SetCVarInteger("tdm_frobhelper_fadein_delay", 500);     // default value
			cvarSystem->SetCVarInteger("tdm_frobhelper_fadein_duration", 1500); // default value
			break;
		case 4: // Fade In Fast
			cvarSystem->SetCVarBool("tdm_frobhelper_active", true);
			cvarSystem->SetCVarBool("tdm_frobhelper_alwaysVisible", false);
			cvarSystem->SetCVarInteger("tdm_frobhelper_fadein_delay", 250);
			cvarSystem->SetCVarInteger("tdm_frobhelper_fadein_duration", 500);
			break;
		default:
			gameLocal.Warning("Unknown value for frob_helper_preset encountered!");
		};
	}
	// load various game play settings, and update the GUI strings in one go
	else if (cmd == "loadsettings")
	{
		int setting = 0;
		if (cv_lp_auto_pick.GetBool())
		{
			setting = 4; // automatic
		}
		else // auto-pick is false
		{
			if (cv_lp_pick_timeout.GetInteger() >= 750)
			{
				setting = 0; // Trainer
			}
			else if (cv_lp_pick_timeout.GetInteger() >= 500)
			{
				setting = 1; // Average
			}
			else if (cv_lp_pick_timeout.GetInteger() >= 400)
			{
				setting = 2; // Hard
			}
			else // if (cv_lp_pick_timeout.GetInteger() >= 300)
			{
				setting = 3; // Expert (default)
			}
		}
		gui->SetStateInt("lp_difficulty", setting);

		int defaultSpeed = 0; // Walk
		if (cvarSystem->GetCVarBool("in_toggleRun"))
			defaultSpeed = 2; // Toggle (has precedence; works when in_alwaysRun true)
		else if (cvarSystem->GetCVarBool("in_alwaysRun"))
			defaultSpeed = 1; // Run
		gui->SetStateInt("defaultspeed", defaultSpeed);

		int frobHelperPreset = 0; // Off
		if (cvarSystem->GetCVarBool("tdm_frobhelper_active"))
		{
			if (cvarSystem->GetCVarBool("tdm_frobhelper_alwaysVisible"))
			{
				frobHelperPreset = 1; // Always
			}
			else if (cvarSystem->GetCVarInteger("tdm_frobhelper_fadein_delay") == 0 &&
			         cvarSystem->GetCVarInteger("tdm_frobhelper_fadein_duration") == 0)
			{
				frobHelperPreset = 2; // Hover
			}
			else
			{
				frobHelperPreset = 3; // Fade In
			}
		}
		gui->SetStateInt("frob_helper_preset", frobHelperPreset);

		idStr diffString = cv_melee_difficulty.GetString();
		bool bForbidAuto = cv_melee_forbid_auto_parry.GetBool();

		if( diffString == "hard" )
		{
			setting = 1; // Hard
		}
		else if( diffString == "expert" && !bForbidAuto )
		{
			setting = 2; // Expert
		}
		else if( diffString == "expert" && bForbidAuto )
		{
			setting = 3; // Master
		}
		else
		{
			setting = 0; // Normal by default
		}
		gui->SetStateInt("melee_difficulty", setting);

		gui->HandleNamedEvent("UpdateAutoParryOption");

		// grayman #3492 - AI Vision
		setting = cv_ai_vision.GetInteger(); // returns one a number from 0 -> 3
		gui->SetStateInt("ai_vision",setting);

		// grayman #3682 - AI Hearing
		setting = cv_ai_hearing.GetInteger(); // returns one a number from 0 -> 3
		gui->SetStateInt("ai_hearing",setting);
	}
	else if (cmd == "updateaivision") // grayman #3492 - AI vision
	{
		// AI Vision setting changed, update CVARs
		int setting = gui->GetStateInt("ai_vision", "-1");
		if ( ( setting >= 0 ) && ( setting <= 3 ) )
		{
			cv_ai_vision.SetInteger(setting);
		}
		else
		{
			gameLocal.Warning("Unknown value for AI Vision encountered!");
		}
	}
	else if (cmd == "updateaihearing") // grayman #3682 - AI hearing
	{
		// AI Hearing setting changed, update CVARs
		int setting = gui->GetStateInt("ai_hearing", "-1");
		if ( ( setting >= 0 ) && ( setting <= 3 ) )
		{
			cv_ai_hearing.SetInteger(setting);
		}
		else
		{
			gameLocal.Warning("Unknown value for AI Hearing encountered!");
		}
	}
	else if (cmd == "updatemeleedifficulty")
	{
		// Melee difficulty setting changed, update CVARs
		int setting = gui->GetStateInt("melee_difficulty", "-1");
		
		switch (setting)
		{
		case 0: // Normal
			cv_melee_difficulty.SetString("normal");
			cv_melee_forbid_auto_parry.SetBool(false);
			break;
		case 1: // Hard
			cv_melee_difficulty.SetString("hard");
			cv_melee_forbid_auto_parry.SetBool(false);
			break;
		case 2: // Expert
			cv_melee_difficulty.SetString("expert");
			cv_melee_forbid_auto_parry.SetBool(false);
			break;
		case 3: // Master, same as expert but forces manual parry to be enabled
			cv_melee_difficulty.SetString("expert");
			cv_melee_auto_parry.SetBool(false);
			cv_melee_forbid_auto_parry.SetBool(true);
			break;
		default:
			gameLocal.Warning("Unknown value for melee difficulty encountered!");
		};

		// Trigger an update for the auto-parry option when melee difficulty changes
		gui->HandleNamedEvent("UpdateAutoParryOption");
	}
	else if (cmd == "mainmenu_init")
	{
		gui->SetStateString("tdmversiontext", va("%s #%d", ENGINE_VERSION, RevisionTracker::Instance().GetHighestRevision()));
		UpdateGUIScaling(gui);
		gui->SetStateString( "tdm_lang", common->GetI18N()->GetCurrentLanguage().c_str() );
		idStr gui_lang = "lang_";
		gui_lang += common->GetI18N()->GetCurrentLanguage().c_str();
		gui->SetStateInt( gui_lang, 1 );
		idStr modName = m_MissionManager->GetCurrentModName();
		CModInfoPtr info = m_MissionManager->GetModInfo(modName);

		// grayman #3733 - provide campaign and mission titles to the main menu
		if (info->_missionTitles.Num() > 1)
		{
			idStr campaignTitle = info->_missionTitles[0];
			campaignTitle += ":";
			idStr campaignMissionTitle = info->_missionTitles[m_MissionManager->GetCurrentMissionIndex() + 1];
			gui->SetStateString("CampaignTitleText",common->Translate(campaignTitle.c_str())); // grayman #3733
			gui->SetStateString("CampaignMissionTitleText",common->Translate(campaignMissionTitle.c_str())); // grayman #3733
		}
		else if (info->_missionTitles.Num() == 1)
		{
			idStr missionTitle = info->_missionTitles[0];
			gui->SetStateString("MissionTitleText",common->Translate(missionTitle.c_str())); // grayman #3733
		}
		//stgatilov #4724: resolution is changed on start on Windows + fullscreen
		//update gui variables here so that correct resolution is shown to player
		UpdateWidescreenModeFromScreenResolution(gui);
	}
	else if (cmd == "mainmenuingame_init") // grayman #3733
	{
		gui->SetStateString("tdmversiontext", va("%s #%d", ENGINE_VERSION, RevisionTracker::Instance().GetHighestRevision()));
		idStr modName = m_MissionManager->GetCurrentModName();
		CModInfoPtr info = m_MissionManager->GetModInfo(modName);

		// grayman #3733 - provide campaign and mission titles to the main in-game menu
		if (info->_missionTitles.Num() > 1)
		{
			idStr campaignTitle = info->_missionTitles[0];
			campaignTitle += ":";
			idStr campaignMissionTitle = info->_missionTitles[m_MissionManager->GetCurrentMissionIndex() + 1];
			gui->SetStateString("MMIGCampaignTitleText",common->Translate(campaignTitle.c_str())); // grayman #3733
			gui->SetStateString("MMIGCampaignMissionTitleText",common->Translate(campaignMissionTitle.c_str())); // grayman #3733
		}
		else if (info->_missionTitles.Num() == 1)
		{
			idStr missionTitle = info->_missionTitles[0];
			gui->SetStateString("MMIGMissionTitleText",common->Translate(missionTitle.c_str())); // grayman #3733
		}
	}
	else if (cmd == "loadsavemenu_init") // grayman #3733
	{
		idStr modName = m_MissionManager->GetCurrentModName();
		CModInfoPtr info = m_MissionManager->GetModInfo(modName);

		// grayman #3733 - provide campaign and mission titles to the load/save menu
		if (info->_missionTitles.Num() > 1)
		{
			idStr campaignTitle = info->_missionTitles[0];
			campaignTitle += ":";
			idStr campaignMissionTitle = info->_missionTitles[m_MissionManager->GetCurrentMissionIndex() + 1];
			gui->SetStateString("LSCampaignTitleText",common->Translate(campaignTitle.c_str())); // grayman #3733
			gui->SetStateString("LSCampaignMissionTitleText",common->Translate(campaignMissionTitle.c_str())); // grayman #3733
		}
		else if (info->_missionTitles.Num() == 1)
		{
			idStr missionTitle = info->_missionTitles[0];
			gui->SetStateString("LSMissionTitleText",common->Translate(missionTitle.c_str())); // grayman #3733
		}
	}
/*	else if (cmd == "check_tdm_version")
	{
		CheckTDMVersion();
	}*/
	else if (cmd == "close_msg_box")
	{
		gui->SetStateBool("MsgBoxVisible", false);
	}
	else if (cmd == "lod_bias_changed")		// Adding a way to update cooked data from menu - J.C.Denton
	{
		// Add the command to buffer, but no need to issue it immediately. 
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "tdm_lod_bias_changed\n" );
	}
	else if (cmd == "shadowimplchanged")	// We have to call "reloadModels" to make sure shadows implementations toggle correctly
	{										// stgatilov: added for 2.07
		// Add the command to buffer, but no need to issue it immediately. 
		cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "reloadModels\n" );
	}
	else if (cmd == "resetbrightness")
	{
		idCVar * cvar = cvarSystem->Find( "r_postprocess_brightness" );
		cvar ? cvar->SetFloat( 1.0f ) :	Warning("Cannot find CVAR r_postprocess_brightness.");
	}
	else if (cmd == "resetgamma")
	{
		idCVar * cvar = cvarSystem->Find( "r_postprocess_gamma" );
		cvar ? cvar->SetFloat( 1.2f ) :	Warning("Cannot find CVAR r_postprocess_gamma.");
	}
	else if (cmd == "resethud") // Obsttorte
	{
		idCVar * cvar = cvarSystem->Find("gui_iconSize");
		cvar ? cvar->SetFloat(1.0f) : Warning("Cannot find CVAR gui_iconSize.");
		cvar = cvarSystem->Find("gui_smallTextSize");
		cvar ? cvar->SetFloat(1.0f) : Warning("Cannot find CVAR gui_smallTextSize.");
		cvar = cvarSystem->Find("gui_lightgemSize");
		cvar ? cvar->SetFloat(1.0f) : Warning("Cannot find CVAR gui_lightgemSize.");
		cvar = cvarSystem->Find("gui_bigTextSize");
		cvar ? cvar->SetFloat(1.0f) : Warning("Cannot find CVAR gui_bigTextSize.");
		cvar = cvarSystem->Find("gui_barSize");
		cvar ? cvar->SetFloat(1.0f) : Warning("Cannot find CVAR gui_barSize.");
		cvar = cvarSystem->Find("gui_objectiveTextSize");
		cvar ? cvar->SetFloat(1.0f) : Warning("Cannot find CVAR gui_objectiveTextSize.");
	}
	else if (cmd == "onstartmissionclicked")
	{
		// First mission to be started, reset index
		m_MissionManager->SetCurrentMissionIndex(0);
		gui->SetStateInt("CurrentMission", 1);

		// Let the GUI know which map to load
		idStr mapToStart = m_MissionManager->GetCurrentStartingMap();
		gui->SetStateString("mapStartCmd", va("exec 'map %s'", mapToStart.c_str()));

		// stgatilov #6509: set persistent info to empty when starting mission or campaign from scratch
		ClearPersistentInfo();
		ClearPersistentInfoInGui(gui);
		gameLocal.persistentLevelInfoLocation = PERSISTENT_LOCATION_MAINMENU;
	}
	else if (cmd == "setlang")
	{
		const idStr& oldLang = common->GetI18N()->GetCurrentLanguage();
		idStr newLang = m_GUICommandStack[1];

		// reset lang_oldname and enable lang_newname
		idStr gui_lang = "lang_"; gui_lang += newLang;
//		Printf ("Setting %s to 1.\n", gui_lang.c_str() );
		gui->SetStateInt( gui_lang, 1 );
		gui_lang = "lang_"; gui_lang += oldLang.c_str();
//		Printf ("Setting %s to 0.\n", gui_lang.c_str() );
		gui->SetStateInt( gui_lang, 0 );

		// do this after the step above, or oldLang will be incorrect:
		Printf("GUI: Language changed to %s.\n", newLang.c_str() );
		// set the new language and store it, will also reload the GUI
		gui->SetStateString( "tdm_lang", common->GetI18N()->GetCurrentLanguage().c_str() );
		if (common->GetI18N()->SetLanguage( newLang.c_str() ))
		{
			// Tels: #3193: Cycle through all active entities and call "onLanguageChanged" on them
			// some scriptobjects (like for readables) may implement this function to react on language switching
			for (idEntity* ent = gameLocal.spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
			{
				idThread* thread = ent->CallScriptFunctionArgs("onLanguageChanged", true, 0, "e", ent);
				if (thread != NULL)
				{
					thread->Execute();
				}
			}
		}
	}
//	else
//	{
//		Warning("Unknown main menu command '%s'.\n", cmd.c_str());
//	}

	// TODO: These routines might want to handle arguments to commands, too, so
	//		 add support for this here. E.g. instead of passing ("SomeCommand", gui), pass
	//		 (m_GUICommandStack, gui) to them:
	if (cmd != "log")
	{
		m_Shop->HandleCommands( menuCommand, gui);
		m_ModMenu->HandleCommands( menuCommand, gui);
		m_DownloadMenu->HandleCommands( cmd, gui);		// expects commands in lowercase
	}

	/*if (cv_debug_mainmenu.GetBool())
	{
		const idDict& state = gui->State();
		
		for (int i = 0; i < state.GetNumKeyVals(); ++i)
		{
			const idKeyValue* kv = state.GetKeyVal(i);

			const idStr key = kv->GetKey();
			const idStr value = kv->GetValue();

			// Force the log file to write its stuff
			g_Global.m_ClassArray[LC_MISC] = true;
			g_Global.m_LogArray[LT_INFO] = true;

			DM_LOG(LC_MISC, LT_INFO)LOGSTRING("Mainmenu GUI State %s = %s\r", key.c_str(), value.c_str());
		}
	}*/

	// tels: We handled (or ignored) this command, so clear the command stack
	m_GUICommandStack.Clear();
	m_GUICommandArgs = 0;
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
			gameRenderWorld->DebugText( ent->name.c_str(), center - up, 0.1f, colorWhite * frac, axis, 1 );
			gameRenderWorld->DebugText( ent->GetEntityDefName(), center, 0.1f, colorWhite * frac, axis, 1 );
			gameRenderWorld->DebugText( va( "#%d", ent->entityNumber ), center + up, 0.1f, colorWhite * frac, axis, 1 );
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
				gameRenderWorld->DebugText( ent->name.c_str(), entBounds.GetCenter(), 0.1f, colorWhite, axis, 1 );
				gameRenderWorld->DebugText( va( "#%d", ent->entityNumber ), entBounds.GetCenter() + up, 0.1f, colorWhite, axis, 1 );
				gameRenderWorld->DebugText( 
					va( "%d / %d", ent->GetPhysics()->GetContents(), ent->GetPhysics()->GetClipMask() ), 
					entBounds.GetCenter() - up, 0.1f, 
					colorWhite, 
					axis, 1 
				);
			}
		}
	}

	// debug tool to draw bounding boxes around active entities
	if ( g_showActiveEntities.GetBool() ) {
		for ( auto iter = activeEntities.Begin(); iter; activeEntities.Next(iter) ) {
			ent = iter.entity;
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

	if ( cv_show_health.GetBool() ) {
		idMat3		axis = player->viewAngles.ToMat3();
		idBounds	viewBounds( origin );
		viewBounds.ExpandSelf( 512.0f );
		for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
			// don't draw the worldspawn or "dead" entities
			if ( ent == world || !ent->health) {
				continue;
			}

			// skip if the entity is very far away
			if ( !viewBounds.IntersectsBounds( ent->GetPhysics()->GetAbsBounds() ) ) {
				continue;
			}
			
			gameRenderWorld->DebugText(va("Health: %d", ent->health), ent->GetPhysics()->GetOrigin(), 0.2f, colorGreen, axis);
		}
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

	if ( int contentsMask = g_showCollisionAlongView.GetInteger() ) {
		trace_t results;
		idVec3 eye;
		idMat3 matr;
		player->GetViewPos(eye, matr);
		idVec3 dir = matr[0];
		idVec3 end = eye + dir * CM_MAX_TRACE_DIST * 0.9f;
		if (clip.Translation(results, eye, end, 0, idMat3(), contentsMask, player)) {
			int idx = results.c.entityNum;
			if (idx >= 0) {
				idEntity *ent = entities[idx];
				if (ent) {
					auto mdl = ent->GetPhysics()->GetClipModel();
					if (ent != world)
						clip.DrawClipModel(mdl, eye, 0.0f);
					idBox box(results.c.point, idVec3(1.0f), matr);
					gameRenderWorld->DebugBox(idVec4(0, 1, 1, 1), box);
					static int64 lastDump = 0;
					int64 nowTime = Sys_GetTimeMicroseconds();
					if (nowTime >= lastDump + 1000000) {
						float dist = (results.endpos - eye).Length();
						Printf("Clipped along view direction: \"%s\" at distance %0.1f\n", ent->GetName(), dist);
						lastDump = nowTime;
					}
				}
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
		idAAS *aas = GetAAS("aas32");
		if ( aas ) {
			idVec3 seekPos;
			obstaclePath_t path;

			seekPos = player->GetPhysics()->GetOrigin() + player->viewAxis[0] * 200.0f;
			idAI::FindPathAroundObstacles( player->GetPhysics(), aas, NULL, player->GetPhysics()->GetOrigin(), seekPos, path, player );
		}
	}

	if ( g_showLightQuotient.GetInteger() != 0 ) {
		idBounds viewBounds( origin );
		viewBounds.ExpandSelf( 256.0f );
		idMat3 axis = player->viewAngles.ToMat3();

		for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
			if ( m_LightEstimateSystem->DebugIgnorePlayer(ent) )
				continue;
			if ( ent->GetModelDefHandle() < 0 )
				continue;	// no visual representation

			const idBounds &entBounds = ent->GetPhysics()->GetAbsBounds();
			if ( !viewBounds.IntersectsBounds( entBounds ) )
				continue;	// too far away

			bool tracked;
			float value;
			if ( g_showLightQuotient.GetInteger() < 0 ) {
				tracked = ent->DebugGetLightQuotient( value );
			} else {
				tracked = true;
				value = ent->GetLightQuotient();
			}
			if (!tracked)
				continue;

			idStr text = va("%0.3f", value);

			int mode = idMath::Abs( g_showLightQuotient.GetInteger() );
			if ( mode != 3 ) {
				gameRenderWorld->DebugBounds( colorCyan, entBounds );
			}
			if ( mode == 2 ) {
				gameRenderWorld->DebugText( va( "%s:\n%s", ent->name.c_str(), text.c_str() ), entBounds.GetCenter(), 0.1f, colorWhite, axis, 1 );
			} else {
				gameRenderWorld->DebugText( va( "%s", text.c_str() ), entBounds.GetCenter(), 0.1f, colorWhite, axis, 1 );
			}
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
idGameLocal::GetAASId (TDM)
==================
*/
int	idGameLocal::GetAASId( idAAS* aas ) const
{
	// Do a reverse lookup in the aasList array
	return aasList.FindIndex(aas);
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

void idGameLocal::SetupEAS()
{
	// Cycle through the entities and find all elevators
	for (idEntity* ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (!ent->IsType(CMultiStateMover::Type))
		{
			continue;
		}

		// Get all position entities of that mover
		CMultiStateMover* mover = static_cast<CMultiStateMover*>(ent);

		for (int aasNum = 0; aasNum < NumAAS(); aasNum++)
		{
			idAASLocal* aas = dynamic_cast<idAASLocal*>(GetAAS(aasNum));
			if (aas == NULL) continue;

			aas->AddElevator(mover);
		}
	}

	// All elevators registered, compile the routing information

	// grayman #3763 - calculate the total cluster count for all aas areas.

	int clusterCount = 0;
	for (int aasNum = 0; aasNum < NumAAS(); aasNum++)
	{
		idAASLocal* aas = dynamic_cast<idAASLocal*>(GetAAS(aasNum));
		if (aas != NULL)
		{
			clusterCount += aas->GetClusterSize();
		}
	}

	session->UpdateLoadingProgressBar(PROGRESS_STAGE_ROUTING, 0.0f);

	for (int aasNum = 0; aasNum < NumAAS(); aasNum++)
	{
		idAASLocal* aas = dynamic_cast<idAASLocal*>(GetAAS(aasNum));
		if (aas != NULL)
		{
			aas->CompileEAS(); // will also update the Loading Bar
		}
	}

	session->UpdateLoadingProgressBar(PROGRESS_STAGE_ROUTING, 1.0f);
}


// grayman #3857
void idGameLocal::GetPortals(Search* search, idAI* ai)
{
	idAASLocal* aasLocal = dynamic_cast<idAASLocal*>(GetAAS("aas32")); // "aas32" is used by humanoids, which are the only searchers/guards
	int areaNum = ai->PointReachableAreaNum(search->_origin);
	aasLocal->GetPortals(areaNum,search->_guardSpots,search->_limits, ai); // grayman #4238
}

/*
==================
idGameLocal::CheatsOk
==================
*/
bool idGameLocal::CheatsOk( bool requirePlayer ) {
	idPlayer *player;

	if ( com_developer.GetBool() ) {
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

	if ( spawnCount >= ( 1 << 30 ) ) {
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

	// this will also have the effect of spawnArgs.Clear() at the same time:
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
		if ( ent->entityNumber >= 1 && ent->entityNumber < firstFreeIndex ) {
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
	TRACE_CPU_SCOPE_TEXT("EntDef:Spawn", name);

	spawnArgs.GetString( "classname", NULL, &classname );
	const idDeclEntityDef *def = FindEntityDef( classname, false );

	if ( !def ) {
		Warning( "Unknown classname '%s'%s.\n", classname, error.c_str() );
		return false;
	}

	// Tels: SetDefaults(), but without the "editor_" spawnargs
	spawnArgs.SetDefaults( &def->dict, idStr("editor_") );

	// greebo: Apply the difficulty settings before any values get filled from the spawnarg data
	m_DifficultyManager.ApplyDifficultySettings(spawnArgs);

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

		if ( ent && obj->IsType( idEntity::Type ) ) {
			*ent = static_cast<idEntity *>(obj);
		}

		// Check for campaign info ents
		if (idStr::Cmp(classname, "atdm:campaign_info") == 0)
		{
			assert(obj->IsType(idEntity::Type));
			campaignInfoEntities.Append(static_cast<idEntity*>(obj));
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
		idThread *thread = new idThread(func);
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
const idDict *idGameLocal::FindEntityDefDict( const char *name, bool makeDefault ) const
{
	const idDeclEntityDef *decl = FindEntityDef( name, makeDefault );
	return decl ? &decl->dict : NULL;
}

/*
================
idGameLocal::InhibitEntitySpawn
================
*/
bool idGameLocal::InhibitEntitySpawn( const idDict &spawnArgs ) {
	bool inhibit_spawn = false;

	// #3933 prevent inlined FS from spawning
	if ( spawnArgs.GetBool("inline") && idStr("func_static") == spawnArgs.GetString("classname") ) 
	{
		inhibit_spawn = true;
	}

	// #4300: Prevent func_groups from attempting to spawn
	if ( spawnArgs.GetString("classname") == idStr("func_group") )
	{
		inhibit_spawn = true;
	}
	
	if ( m_DifficultyManager.InhibitEntitySpawn(spawnArgs) )
	{
		inhibit_spawn = true;
	}

	// Tels: #3223: See if this entity should spawn this time
	// Moved from DifficultyManager.cpp in #3933
	float random_remove = spawnArgs.GetFloat( "random_remove", "1.1" );
	float random_value = gameLocal.random.RandomFloat();
	if ( random_remove < random_value )
	{
		inhibit_spawn = true;
		DM_LOG( LC_ENTITY, LT_INFO )LOGSTRING( "Removing entity %s due to random_remove %f < %f.\r", spawnArgs.GetString( "name" ), random_remove, random_value );
	}

	return inhibit_spawn;
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

void idGameLocal::PrepareForMissionEnd()
{
	// Raise the gamestate to "Completed"
	gamestate = GAMESTATE_COMPLETED;

	// Remove mission triggers now that we're done here
	for (int i = 0; i < m_InterMissionTriggers.Num(); )
	{
		if (m_InterMissionTriggers[i].missionNum == m_MissionManager->GetCurrentMissionIndex())
		{
			m_InterMissionTriggers.RemoveIndex(i);
		}
		else
		{
			++i;
		}
	}
}

/*
==============
idGameLocal::SpawnMapEntities

Parses textual entity definitions out of an entstring and spawns gentities.
==============
*/
void idGameLocal::SpawnMapEntities( void )
{
	int			i;
	int			num;
	int			inhibit;
	idMapEntity	*mapEnt;
	int			numEntities;
	idDict		args;

	if ( mapFile == NULL )
	{
		Printf("No mapfile present\n");
		return;
	}

	int start = Sys_Milliseconds();

	// Add the lightgem to the map before anything else happened
	// so it will be included as if it were a regular map entity.
	m_lightGem.SpawnLightGemEntity( mapFile );

	numEntities = mapFile->GetNumEntities();
	if ( numEntities == 0 )
	{
		Error( "...no entities" );
	}

	Printf("Spawning entities\n");

	// the worldspawn is a special that performs any global setup
	// needed by a level
	mapEnt = mapFile->GetEntity( 0 );
	args = mapEnt->epairs;
	args.SetInt( "spawn_entnum", ENTITYNUM_WORLD );
	if ( !SpawnEntityDef( args ) || !entities[ ENTITYNUM_WORLD ] || !entities[ ENTITYNUM_WORLD ]->IsType( idWorldspawn::Type ) )
	{
		Error( "Problem spawning world entity" );
	}

	num = 1;
	inhibit = 0;

	// Clear out the campaign info cache
	campaignInfoEntities.Clear();

	session->UpdateLoadingProgressBar(PROGRESS_STAGE_ENTITIES, 0.0f);
	for ( i = 1 ; i < numEntities ; i++ )
	{
		mapEnt = mapFile->GetEntity( i );
		args = mapEnt->epairs;
		const char *entName = args.GetString("name");

#ifdef LOGBUILD
		// Tels: This might need a lot of time, even when the logging is disabled, so make
		//		 sure we execute this loop only if we need to log:
		if(g_Global.m_ClassArray[LC_ENTITY] == true && g_Global.m_LogArray[LT_DEBUG] == true)
		{
			int n = args.GetNumKeyVals();
			for(int x = 0; x < n; x++)
			{
				const idKeyValue *p = args.GetKeyVal(x);
				const idStr k = p->GetKey();
				const idStr v = p->GetValue();
				DM_LOG(LC_ENTITY, LT_DEBUG)LOGSTRING("Entity[%u] Key:[%s] = [%s]\r", i, k.c_str(), v.c_str());
			}
		}
#endif

		if (!InhibitEntitySpawn(args))
		{
			TRACE_CPU_SCOPE_TEXT("Entity:Spawn", entName)
			declManager->BeginEntityLoad(mapEnt);
			// precache any media specified in the map entity
			CacheDictionaryMedia(&args);
			idEntity *ent;
			SpawnEntityDef(args, &ent);
			if (ent)
				ent->fromMapFile = true;
			declManager->EndEntityLoad(mapEnt);
			num++;
		}
		else
		{
			inhibit++;
		}

		session->UpdateLoadingProgressBar(PROGRESS_STAGE_ENTITIES, float(i + 1) / numEntities);
	}
	session->UpdateLoadingProgressBar(PROGRESS_STAGE_ENTITIES, 1.0f);

	m_lightGem.InitializeLightGemEntity();

	Printf( "... %i entities spawned, %i inhibited in %5.1f seconds\n\n", num, inhibit, (Sys_Milliseconds() - start) * 0.001f );
	DM_LOG(LC_LIGHT, LT_DEBUG)LOGSTRING("... %i entities spawned, %i inhibited\r", num, inhibit);
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

	refLength = static_cast<int>(strlen( ref ));
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
================
idGameLocal::GetRelights - grayman #2603 - retrieve relight entities and add them to the target list
================
*/
int idGameLocal::GetRelights( const idDict &args, idList< idEntityPtr<idEntity> > &list, const char *ref ) const
{
	int i,num,refLength;
	const idKeyValue *arg;
	idEntity *ent;

    refLength = static_cast<int>(strlen(ref));
	num = args.GetNumKeyVals();
	for (i = 0 ; i < num ; i++)
	{
		arg = args.GetKeyVal(i);
		if (arg->GetKey().Icmpn(ref,refLength) == 0)
		{
			const idStr name = arg->GetValue();
			ent = FindEntity(name);
			if (ent)
			{
				idEntityPtr<idEntity> &entityPtr = list.Alloc();
				entityPtr = ent;
			}
		}
	}

	return list.Num();
}

/*
=============
idGameLocal::GetAmbientIllumination

grayman #3132 - returns the ambient illumination at a point
grayman #3584 - rewritten
=============
*/

float idGameLocal::GetAmbientIllumination( const idVec3 point ) const
{
	idVec3 colorSum(0,0,0);
	idLight* mainAmbientLight = gameLocal.FindMainAmbientLight(false);

	// If the point is in a location that defines "ambient_light",
	// use that color. That color is applied to the main ambient light,
	// which has a boundary. If the point is inside the boundary, the
	// location's color is applied. If the point is outside the
	// boundary, then neither the location color nor the main ambient
	// light's color contributes any illumination.

	if ( mainAmbientLight != NULL )
	{
		// is the test point inside the ambient light's volume?

		if ( mainAmbientLight->GetBounds().ContainsPoint(point) )
		{
			idLocationEntity* location = gameLocal.LocationForPoint(point);
			if ( location != NULL )
			{
				// Use the location entity's ambient_light setting, since that
				// is what the main ambient light would be set to in this location.

				idVec3 color;
				location->spawnArgs.GetVector( "ambient_light", "0 0 0", color );
				colorSum += color;
			}
			else
			{
				// This point is not inside a location, so the current value
				// of the main ambient would be used at this point.

				colorSum += mainAmbientLight->GetBaseColor();
			}
		}
	}

	// Any ambient illumination just contributed from a location entity replaces
	// only the value of the ambient_world light (or whatever light is playing
	// that role). There might be other ambient lights inside the location, and
	// they need to be considered.

	// Check the list of ambient lights to
	// see which are contributing light at this point. Ignore the main ambient light
	// because it's already made a contribution, either via the location color or
	// the color of the light itself. (Location color works by setting the main ambient's
	// color to the location's ambient color.)

	// For now, we'll assume that ambient lights apply a uniform illumination
	// througout their boundary. This isn't correct for lights that use a
	// falloff texture, but atm we don't have access to the falloff textures.
	// For simplicity, we'll assume uniform illumination.

	for ( int i = 0 ; i < m_ambientLights.Num() ; i++ )
	{
		idLight *light = m_ambientLights[i].GetEntity();
		if ( ( light != NULL ) && ( light != mainAmbientLight ) )
		{
			if ( light->GetBounds().Translate(light->GetPhysics()->GetOrigin()).ContainsPoint(point) )
			{
				colorSum += light->GetBaseColor(); // gets the current light color
			}
		}
	}

	float illumination = (colorSum.x * DARKMOD_LG_RED + colorSum.y * DARKMOD_LG_GREEN + colorSum.z * DARKMOD_LG_BLUE);

	return illumination;
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

//stgatilov: this searches over entity names in the .map file
//it excludes dynamically spawned entities (e.g. arrows), but includes dead entities already removed from game
void idGameLocal::ArgCompletion_MapEntityName( const idCmdArgs &args, void(*callback)( const char *s ) ) {
	if (idMapFile *mf = gameLocal.mapFile) {
		for (int i = 0; i < mf->GetNumEntities(); i++) {
			if (idMapEntity *me = mf->GetEntity(i)) {
				if (const idKeyValue *kv = me->epairs.FindKey("name")) {
					callback( va( "%s %s", args.Argv( 0 ), kv->GetValue().c_str() ) );
				}
			}
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
int idGameLocal::EntitiesWithinRadius( const idVec3 org, float radius, idClip_EntityList &entityList ) const {
	idEntity *ent;
	idBounds bo( org );
	entityList.SetNum( 0 );

	bo.ExpandSelf( radius );
	for( ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next() ) {
		if ( ent->GetPhysics()->GetAbsBounds().IntersectsBounds( bo ) ) {
			entityList.AddGrow(ent);
		}
	}

	return entityList.Num();
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
	idPhysics	*phys;

	phys = ent->GetPhysics();
	if ( !phys->GetNumClipModels() ) {
		return;
	}

	idClip_ClipModelList clipModels;
	num = clip.ClipModelsTouchingBounds( phys->GetAbsBounds(), phys->GetClipMask(), clipModels );
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

		{
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
			//idPlayer *player = static_cast<idPlayer *>(activator);
			idDict *item = NULL;//player->FindInventoryItem( requires );
			if ( item ) {
				if ( removeItem ) {
					//player->RemoveInventoryItem( item );
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
idGameLocal::RadiusDamage
============
*/
void idGameLocal::RadiusDamage( const idVec3 &origin, idEntity *inflictor, idEntity *attacker, idEntity *ignoreDamage, idEntity *ignorePush, const char *damageDefName, float dmgPower ) {
	float		dist, damageScale, attackerDamageScale, attackerPushScale;
	idEntity *	ent;
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
	idClip_EntityList entityList;
	numListedEntities = clip.EntitiesTouchingBounds( bounds, -1, entityList );

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

	// grayman #3857 - douse nearby lights
	if( damageDef->GetInt("douse", "1") == 1 )
	{
		RadiusDouse( origin, radius, true );
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
	idVec3 dir;
	idBounds bounds;
	modelTrace_t result;
	idEntity *ent;
	float scale;

	dir.Set( 0.0f, 0.0f, 1.0f );

	bounds = idBounds( origin ).Expand( radius );

	// get all clip models touching the bounds
	idClip_ClipModelList clipModelList;
	numListedClipModels = clip.ClipModelsTouchingBounds( bounds, -1, clipModelList );

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

// grayman #3857 - douse nearby flame lights

void idGameLocal::RadiusDouse( const idVec3 &origin, const float radius, const bool checkSpawnarg )
{
	idEntity *ent;
	int		  numListedEntities;

	idBounds bounds = idBounds(origin).Expand(radius);

	// get all entities touching the bounds
	idClip_EntityList entityList;
	numListedEntities = clip.EntitiesTouchingBounds( bounds, -1, entityList );

	// douse all flames that have a LOS from them to the origin
	for ( int i = 0 ; i < numListedEntities ; i++ )
	{
		ent = entityList[i];

		if (ent && ent->IsType(idLight::Type))
		{
			idStr lightType = ent->spawnArgs.GetString(AIUSE_LIGHTTYPE_KEY);

			// douse only flames and gas lamps
			if ((lightType == AIUSE_LIGHTTYPE_TORCH) || (lightType == AIUSE_LIGHTTYPE_GASLAMP))
			{
				idLight* light = static_cast<idLight*>(ent);

				// ignore blend and fog lights

				if (light->IsBlend() || light->IsFog())
				{
					continue;
				}

				// Make it spawnarg-dependent #4201
				if ( checkSpawnarg && !light->spawnArgs.GetBool("canBeBlownOut") )
				{
					continue;
				}

				// Only douse lit lights

				if (light->GetLightLevel() > 0)
				{
					// Find the bindMaster for lights with light holders. Only go
					// up one level in the family chain, since we're only interested
					// in ignoring the entity the light is bound to, if anything.

					idEntity* ignoreMe = light;
					idEntity* bindMaster = light->GetBindMaster();
					if (bindMaster)
					{
						ignoreMe = bindMaster;
					}

					// test LOS between origin of force and light origin

					// Light center is not just the light origin. There's an offset called "light_center", and there's orientation.
					idVec3 trueOrigin = light->GetPhysics()->GetOrigin() + light->GetPhysics()->GetAxis()*light->GetRenderLight()->lightCenter;
					trace_t result;
					if ( !clip.TracePoint(result, origin, trueOrigin, MASK_OPAQUE, ignoreMe) )
					{
						// trace completed, so LOS exists
						light->CallScriptFunctionArgs("frob_extinguish", true, 0, "e", light);
					}
				}
			}
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
			int edgeNum = trm->edgeUses[poly->firstEdge + j];
			v = trm->verts[ trm->edges[ abs(edgeNum) ].v[ INTSIGNBITSET( edgeNum ) ] ];
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
void idGameLocal::ProjectDecal( const idVec3 &origin, const idVec3 &dir, float depth, bool parallel, float size, 
								const char *material, float angle, idEntity* target, bool save, int starttime, bool allowRandomAngle )
{
	float s, c;
	idMat3 axis, axistemp;
	idFixedWinding winding;
	idVec3 windingOrigin, projectionOrigin;

	if ( starttime == -1 )
	{
		starttime = time; // Optional param defaults to -1 => gameLocal.time -- SteveL #3817
	}

	if ( target && save )
	{
		target->SaveDecalInfo( origin, dir, depth, parallel, size, material, angle ); // Save for reapplication after LOD switches -- SteveL #3817
	}

	static idVec3 decalWinding[4] = {
		idVec3(  1.0f,  1.0f, 0.0f ),
		idVec3( -1.0f,  1.0f, 0.0f ),
		idVec3( -1.0f, -1.0f, 0.0f ),
		idVec3(  1.0f, -1.0f, 0.0f )
	};

	if ( !g_decals.GetBool() ) {
		return;
	}

	// randomly rotate the decal winding if angle = 0 and random angles are allowed
	if( angle == 0 && allowRandomAngle )
	{
		angle = random.RandomFloat() * idMath::TWO_PI;
	}

	idMath::SinCos16( angle, s, c );

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
	gameRenderWorld->ProjectDecalOntoWorld( winding, projectionOrigin, parallel, depth * 0.5f, declManager->FindMaterial( material ), starttime );
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

	// grayman #3394 - normalize dir so splats aren't thrown
	// onto faraway surfaces
	idVec3 direction = dir;
	direction.NormalizeFast();

	if ( clip.Translation( results, origin, origin + direction * 64.0f, &mdl, mat3_identity, CONTENTS_SOLID, NULL ) )
	{
		ProjectDecal( results.endpos, dir, 2.0f * size, true, size, material );
	}
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
		cinematicStopTime = time + USERCMD_MSEC;

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
		int areaNum = gameRenderWorld->GetAreaAtPoint( point );
		if ( areaNum < 0 ) {
			Printf( "SpreadLocations: location '%s' is not in a valid area\n", ent->spawnArgs.GetString( "name" ) );
			continue;
		}
		if ( areaNum >= numAreas ) {
			Error( "idGameLocal::SpreadLocations: areaNum >= gameRenderWorld->NumAreas()" );
		}
		if ( locationEntities[areaNum] ) {
			Warning( "location entity '%s' overlaps '%s' in area %d", ent->spawnArgs.GetString( "name" ),
				locationEntities[areaNum]->spawnArgs.GetString( "name" ),areaNum );
			continue;
		}
		locationEntities[areaNum] = static_cast<idLocationEntity *>(ent);

		// spread to all other connected areas
		for ( int i = 0 ; i < numAreas ; i++ ) {
			if ( i == areaNum ) {
				continue;
			}
			if ( gameRenderWorld->AreasAreConnected( areaNum, i, PS_BLOCK_LOCATION ) )
			{
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

	int areaNum = gameRenderWorld->GetAreaAtPoint( point );
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
idGameLocal::SelectInitialSpawnPoint
============
*/
idEntity *idGameLocal::SelectInitialSpawnPoint( idPlayer *player ) {
	spawnSpot_t		spot;

	// grayman #2933 - Did the player specify
	// a starting point in the briefing?
	// stgatilov #6509: now passed via persistent data

	bool foundSpot = false;
	spot.ent = NULL;

	const char *startEntityName = persistentLevelInfo.GetString("builtin_playerStartEntity");
	if ( startEntityName[0] != '\0' )
	{
		spot.ent = FindEntity( startEntityName );
		if ( spot.ent != NULL )
		{
			foundSpot = true;
		}
	}
	
	if ( !foundSpot )
	{
		spot.ent = FindEntityUsingDef( NULL, "info_player_start" );
		if ( !spot.ent )
		{
			Error( "No info_player_start on map.\n" );
		}
	}
	return spot.ent;
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
=================
idGameLocal::SetPortalSkyEnt
=================
*/

void idGameLocal::SetPortalSkyEnt( idEntity *ent ) {
	portalSkyEnt = ent;
}

/*
=================
idGameLocal::IsPortalSkyActive
=================
*/
bool idGameLocal::IsPortalSkyActive() {
	return portalSkyActive;
}

// grayman #3108 - contributed by neuro & 7318

/*
=================
idGameLocal::CheckGlobalPortalSky
=================
*/
bool idGameLocal::CheckGlobalPortalSky() {
	return globalPortalSky;
}

/*
=================
idGameLocal::SetGlobalPortalSky			// 7318
=================
*/
void idGameLocal::SetGlobalPortalSky( const char *name ) {

	if ( CheckGlobalPortalSky() ) {
		Error( "There is more than one global portalSky.\nDelete them until you have just one.\nportalSky '%s' causes it.", name );
	}
	else {
		globalPortalSky = true;
	}
}

/*
=================
idGameLocal::SetCurrentPortalSkyType		// 7318
=================
*/
void idGameLocal::SetCurrentPortalSkyType( int type ) {
	currentPortalSkyType = type;
}

/*
=================
idGameLocal::GetCurrentPortalSkyType		// 7318
=================
*/
int idGameLocal::GetCurrentPortalSkyType() {
	return currentPortalSkyType;
}

// end neuro & 7318

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
void idGameLocal::GetBestGameType( const char* map, const char* gametype, char buf[ MAX_STRING_CHARS ] ) {
	strncpy( buf, gametype, MAX_STRING_CHARS );
	buf[ MAX_STRING_CHARS - 1 ] = '\0';
}

/*
===========
idGameLocal::NeedRestart
============
*/
bool idGameLocal::NeedRestart() {
	idDict		newInfo;
	const idKeyValue *keyval, *keyval2;

	newInfo = cvarSystem->MoveCVarsToDict( CVAR_SERVERINFO );

	for ( int i = 0; i < newInfo.GetNumKeyVals(); i++ ) {
		keyval = newInfo.GetKeyVal( i );
		keyval2 = serverInfo.FindKey( keyval->GetKey() );
		if ( !keyval2 ) {
			return true;
		}

		// a select set of si_ changes will cause a full restart of the server
		if ( keyval->GetValue().Cmp( keyval2->GetValue() ) && ( !keyval->GetKey().Cmp( "si_map" ) ) ) {
			return true;
		}
	}
	return false;
}

void idGameLocal::GetMapLoadingGUI( char gui[ MAX_STRING_CHARS ] )
{
}

int idGameLocal::CheckStimResponse(idList< idEntityPtr<idEntity> > &list, idEntity *e)
{
	// Construct an idEntityPtr with the given entity
	return list.FindIndex(e);
}

void idGameLocal::LinkStimEntity(idEntity* ent)
{
	if (ent != NULL && ent->GetStimResponseCollection()->HasStim())
	{
		AddStim(ent);
	}
}

void idGameLocal::UnlinkStimEntity(idEntity* ent)
{
	RemoveStim(ent);
}

bool idGameLocal::AddStim(idEntity *e)
{
	bool rc = true;

	if (CheckStimResponse(m_StimEntity, e) == -1)
	{
		m_StimEntity.Append(e);
	}

	return rc;
}

void idGameLocal::RemoveStim(idEntity *e)
{
	int i = CheckStimResponse(m_StimEntity, e);

	if (i != -1)
	{
		m_StimEntity.RemoveIndex(i);
	}
}

bool idGameLocal::AddResponse(idEntity *e)
{
	bool rc = true;

	if(CheckStimResponse(m_RespEntity, e) == -1)
	{
		m_RespEntity.Append(e);
	}

	return rc;
}


void idGameLocal::RemoveResponse(idEntity *e)
{
	int i = CheckStimResponse(m_RespEntity, e);

	if (i != -1)
	{
		m_RespEntity.RemoveIndex(i);
	}
}

// grayman #1104 - DoesOpeningExist() looks for any opening along the axis of the
// normal of the surface the original trace impacted. It creates a grid of points
// to test from, and applies a randomized jitter to the grid, to minimize testing
// the same points in future fires of the gas stim.

#define GAS_GRID_SIZE     16.0f // grid spacing on the trace plane
#define GAS_PLANE_OFFSET  10.0f // how far to back up from the impacted plane when setting a plane to trace from
#define GAS_MAX_TRACES	  32    // max number of traces to try
#define GAS_JITTER		  (GAS_GRID_SIZE/2) // max jitter randomness to add at start of each session

#define GAS_DOWN  1
#define GAS_LEFT  2
#define GAS_UP    3
#define GAS_RIGHT 4

bool idGameLocal::DoesOpeningExist( const idVec3 origin, const idVec3 target, const float radius, const idVec3 normal, idEntity* ent )
{
	float dist2Target = (target - origin).LengthFast();

	// Trace from target to origin to determine rough barrier depth
	trace_t result;
	gameLocal.clip.TracePoint(result, target, origin, (CONTENTS_WATER|CONTENTS_SOLID), ent);
	
	float barrierDepth = dist2Target*(1 - result.fraction);
	idVec3 forwardVector = (GAS_PLANE_OFFSET + barrierDepth + GAS_PLANE_OFFSET)*(-normal); // use this at testPoint to find endPoint

	idVec3 planeOrigin = origin + GAS_PLANE_OFFSET*normal; // origin of plane from which we'll launch the traces
	idVec3 normalRight;
	idVec3 normalUp;
	normal.OrthogonalBasis(normalRight,normalUp);
	int divisions; // (divisions+1) test points per side ((divisions+1) x (divisions+1))
	idVec3 testPoint;
	idVec3 deltaRight; // vector when moving right or left
	idVec3 deltaUp; // vector when moving up or down
	if ( radius < GAS_GRID_SIZE )
	{
		divisions = 2; // (spotCount+1) test points per side ((spotCount+1) x (spotCount+1))
		deltaRight = radius*normalRight;
		deltaUp = radius*normalUp;
	}
	else
	{
		divisions = (int)(2*radius/GAS_GRID_SIZE);
		deltaRight = GAS_GRID_SIZE*normalRight;
		deltaUp = GAS_GRID_SIZE*normalUp;
	}

	idVec3 jitter = GAS_JITTER*(gameLocal.random.RandomFloat() - 0.5)*normalRight + GAS_JITTER*(gameLocal.random.RandomFloat() - 0.5)*normalUp;
	testPoint = planeOrigin + jitter + radius*normalRight + radius*normalUp;
	int counter = 0;

	int state = GAS_DOWN;
	int n = 0;
	int end = divisions;

	while ( end >= 0 )
	{
		// Only process testPoint if the total travel distance through it
		// to the target is less than the radius. If that's the case, the gas
		// can't reach the target, and we can move on to the next testPoint.
		// For this calculation, assume the distance from testPoint to endPoint
		// is barrierDepth, ignoring GAS_PLANE_OFFSET.

		idVec3 endPoint = testPoint + forwardVector;
		float dist2TestPoint = (testPoint - planeOrigin).LengthFast(); // first leg
		// barrierDepth = second leg
		float dist2Target = (target - endPoint).LengthFast(); // third leg
		if ( (dist2TestPoint + barrierDepth + dist2Target) <= radius )
		{
			// Only consider testPoint if there's a clear path
			// to the origin. This keeps us from considering points
			// in neighboring rooms.

			counter++;
			if ( !gameLocal.clip.TracePoint(result, origin, testPoint, (CONTENTS_WATER|CONTENTS_SOLID), NULL) )
			{
				// Trace from the test point forward (forwardVector) to see if there's an opening through the barrier

				counter++;
				if ( !gameLocal.clip.TracePoint(result, testPoint, endPoint, (CONTENTS_WATER|CONTENTS_SOLID), ent) )
				{
					// One last trace to perform: can endPoint see the target? This covers the case where we've
					// punched through, but not to the room where target lives.

					counter++;
					idVec3 normal;

					int traceResult = TraceGasPath( testPoint, endPoint, ent, normal );

					if ( traceResult == 0 )
					{
						return true; // reached the target
					}

					return false; // didn't reach the target
				}

				if ( counter >= GAS_MAX_TRACES )
				{
					return false; // quit early
				}
			}
		}

		// Go to next test point. To reduce the amount of repeat traces close
		// to the origin, test points around the perimeter first, and spiral in
		// toward the origin. As the radius expands, if previous radii haven't
		// found any openings, openings found with the expanded radius are
		// most likely to be found around the perimeter, in space untested before
		// now. We have to spiral all the way to the origin, though, in case an
		// opening appeared near the origin (i.e. a window or door gets opened)
		// since the previous smaller radius was tested.

		n++;
		switch (state)
		{
		case GAS_DOWN:
			if ( n > end )
			{
				end--;
				n = 0;
				testPoint -= deltaRight;
				state = GAS_LEFT;
			}
			else
			{
				testPoint -= deltaUp;
			}
			break;
		case GAS_LEFT:
			if ( n > end )
			{
				n = 0;
				testPoint += deltaUp;
				state = GAS_UP;
			}
			else
			{
				testPoint -= deltaRight;
			}
			break;
		case GAS_UP:
			if ( n > end )
			{
				end--;
				n = 0;
				testPoint += deltaRight;
				state = GAS_RIGHT;
			}
			else
			{
				testPoint += deltaUp;
			}
			break;
		case GAS_RIGHT:
			if ( n > end )
			{
				n = 0;
				testPoint -= deltaUp;
				state = GAS_DOWN;
			}
			else
			{
				testPoint += deltaRight;
			}
			break;
		}
	}

	return false;
}

// grayman #1104 - check what's between two points
// return 0 = completed the path
//        1 = didn't complete the path, seek an opening
//        2 = didn't complete the path, don't seek an opening

int idGameLocal::TraceGasPath( idVec3 from, idVec3 to, idEntity* ignore, idVec3& normal )
{
	trace_t trace;

	int result = 0; // completed the path
	int traceCount = 1; // which trace # we're on

	while ( true )
	{
		gameLocal.clip.TracePoint( trace, from, to, MASK_SOLID, ignore );
		if ( trace.fraction == 1.0f )
		{
			// completed the path
			break;
		}

		// prevent infinite loops where we get stuck inside the intersection of 2 entities

		if ( ( traceCount > 1 ) && ( trace.fraction < VECTOR_EPSILON ) )
		{
			// see if there's an opening to leak through
			normal = trace.c.normal;
			result = 1;
			break;
		}

		// End the trace if we hit the world or a door

		idEntity* entHit = gameLocal.entities[trace.c.entityNum];

		if ( entHit == gameLocal.world )
		{
			// see if there's an opening to leak through
			normal = trace.c.normal;
			result = 1;
			break;
		}

		if ( entHit->IsType(CFrobDoor::Type) )
		{
			// ignore an open door, but a closed door stops gas
			if ( !static_cast<CFrobDoor*>(entHit)->IsOpen() )
			{
				// door is closed, don't seek an opening
				result = 2;
				break;
			}
		}

		// Continue the trace from the struck point

		from = trace.endpos;
		ignore = entHit; // this time, ignore the entity we struck
		traceCount++;
	}

	return result;
}


int idGameLocal::DoResponseAction(const CStimPtr& stim, const idClip_EntityList &srEntities, idEntity* originator, const idVec3& stimOrigin )
{
	int numEntities = srEntities.Num();
	int numResponses = 0;
	for ( int i = 0 ; i < numEntities ; i++ )
	{
		// ignore the original entity because an entity shouldn't respond 
		// to its own stims.
		if ( ( srEntities[i] == originator ) || ( srEntities[i]->GetResponseEntity() == originator ) )
		{
			continue;
		}

		// Check if the radius is really fitting. EntitiesTouchingBounds is using a rectangular volume
		// greebo: Be sure to use this check only if "use bounds" is set to false
		if (!stim->m_bCollisionBased && !stim->m_bUseEntBounds)
		{
			// take the square radius, is faster
			float radiusSqr = stim->GetRadius();
			radiusSqr *= radiusSqr;

			idEntity *ent = srEntities[i];
			idVec3 entitySpot = ent->GetPhysics()->GetOrigin();

			if ( stim->m_StimTypeId == ST_GAS )
			{
				// grayman #1104 - doused lights don't turn their gas stim responses
				// off, so they're still queried for their response. To reduce the
				// potential of running traces when a barrier is present, ignore
				// doused lights.

				if ( ent->IsType(idLight::Type) )
				{
					if ( static_cast<idLight*>(ent)->GetLightLevel() <= 0 )
					{
						continue;
					}
				}
				else // not a light
				{
					// To minimize tracing, check if AI are already dead or knocked out. No point
					// in passing a gas stim reponse to them if they aren't going to use it.

					idAFAttachment* head = NULL;
					idEntity* body = NULL;
					if ( ent->IsType(idAFAttachment::Type) ) // is this an attached head?
					{
						head = static_cast<idAFAttachment*>(ent);
						// this is an AI's head, so get his body
						body = static_cast<idAFAttachment*>(ent)->GetBody();
					}
					else if (ent->IsType(idAI::Type))
					{
						// this is an AI w/a built-in head
						body = ent;
					}

					if ( body && body->IsType(idAI::Type))
					{
						idAI* ai = static_cast<idAI*>(body);
						if ( ai->AI_KNOCKEDOUT || ai->AI_DEAD )
						{
							continue;
						}
					}

					if ( head == NULL ) // is this an attached head?
					{
						// not an attached head, so is this an AI with a built-in head,
						// or perhaps the body of an AI with an attached head?

						if ( ent->IsType(idActor::Type) )
						{
							idActor* entActor = static_cast<idActor*>(ent);
							if ( entActor->GetHead() )
							{
								continue; // skip the body because the head is processed
										  // separately, and will provide the response
							}

							// No head attachment, so obtain the mouth position from the body,
							// given as an offset from the eyes.

							idMat3 viewaxis;
							entActor->GetViewPos(entitySpot,viewaxis); // get eye position and orientation
							entitySpot += viewaxis * entActor->m_MouthOffset;
						}
					}
					else
					{
						// This is a separate head. Add in the mouth offset oriented by head axis.
						// The mouth offset is relative to the head's origin.
						if ( body && body->IsType(idActor::Type))
						{
							entitySpot += head->GetPhysics()->GetAxis() * static_cast<idActor*>(body)->m_MouthOffset;
						}
					}
				}
			}

			if ((entitySpot - stimOrigin).LengthSqr() > radiusSqr)
			{
				// Too far away
				continue;
			}

			// grayman #1104 - for a gas stim, see if there's a clear path
			// between the stim origin and the entity being stimmed

			if ( stim->m_StimTypeId == ST_GAS )
			{
				// Test LOS between stim origin and entitySpot. If it exists,
				// the gas reached the target. If it doesn't, probe the
				// area between the stim origin and the target to see if there's
				// an opening the gas could leak through.

				idVec3 normal;

				int pathResult = TraceGasPath( stimOrigin, entitySpot, ent, normal );
				if ( pathResult == 0 )
				{
					// path to target exists, gas it
				}
				else if ( pathResult == 1 )
				{
					// didn't complete the path, but there might be an opening to leak through
					if (!DoesOpeningExist( stimOrigin, entitySpot, stim->GetRadius(), normal, ent ) )
					{
						continue; // there's no path to the other side
					}
				}
				else // pathResult == 2
				{
					continue; // there's no path to the other side
				}
			}
		}

		// Check for a shooter entity, these don't need to have a response
		if (srEntities[i]->IsType(tdmFuncShooter::Type))
		{
			static_cast<tdmFuncShooter*>(srEntities[i])->stimulate(static_cast<StimType>(stim->m_StimTypeId));
		}

		CResponsePtr response = srEntities[i]->GetStimResponseCollection()->GetResponseByType(stim->m_StimTypeId);

		if (response != NULL)
		{
			if (response->m_State == SS_ENABLED && stim->CheckResponseIgnore(srEntities[i]) == false)
			{
				// Check duration, disable if past duration
				if (response->m_Duration && (gameLocal.time - response->m_EnabledTimeStamp) > response->m_Duration )
				{
					response->Disable();
					continue;
				}

				if (cv_sr_show.GetInteger() > 0)
				{
					// Show successful S/R
					gameRenderWorld->DebugArrow( colorGreen, stimOrigin, srEntities[i]->GetPhysics()->GetOrigin(), 1, 4 * USERCMD_MSEC );
				}

				// Fire the response and pass the originating entity plus the stim object itself
				// The stim object can be queried for values like magnitude, falloff and such.
				response->TriggerResponse(originator, stim);
				numResponses++;
			}
		}
	}

	// Return number of responses triggered
	return numResponses;
}

void idGameLocal::ProcessTimer(unsigned int ticks)
{
	int i, n;
	CStimResponseTimer *t;

	n = m_Timer.Num();
	for(i = 0; i < n; i++)
	{
		t = m_Timer[i];

		// We just let the timers tick. Querying them must be done by the respective owner,
		// because we can not know from where it was called, and it doesn't make sense
		// to use a callback, because the timer user still has to wait until it is expired.
		// So the owner has to check it from time to time anyway.
		if(t->GetState() == CStimResponseTimer::SRTS_RUNNING)
		{
			t = m_Timer[i];
			t->Tick(ticks);
		}
	}
}

CStimResponsePtr idGameLocal::FindStimResponse(int uniqueId)
{
	for (idEntity* ent = spawnedEntities.Next(); ent != NULL; ent = ent->spawnNode.Next())
	{
		if (ent->GetStimResponseCollection() != NULL)
		{
			CStimResponsePtr candidate = ent->GetStimResponseCollection()->FindStimResponse(uniqueId);

			if (candidate != NULL)
			{
				// We found the stim/response, return it
				DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("StimResponse with Id %d found.\r", uniqueId);
				return candidate;
			}
		}
	}

	DM_LOG(LC_STIM_RESPONSE, LT_DEBUG)LOGSTRING("Warning: StimResponse with Id %d NOT found.\r", uniqueId);

	// Search did not produce any results
	return CStimResponsePtr();
}

void idGameLocal::ProcessStimResponse(unsigned int ticks)
{
	if (cv_sr_disable.GetBool())
	{
		return; // S/R disabled, skip this
	}

	TRACE_CPU_SCOPE( "ProcessStimResponse" )
	idTimer srTimer;
	srTimer.Clear();
	srTimer.Start();

	// Check the timed stims first.
	for (int i = 0; i < m_StimTimer.Num(); i++)
	{
		CStim* stim = m_StimTimer[i];

		if (stim == NULL) continue;

		// Only advance the timer if the stim can be fired in the first place
		if (stim->m_MaxFireCount > 0 || stim->m_MaxFireCount == -1)
		{
			// Advance the timer
			CStimResponseTimer* timer = stim->GetTimer();

			if (timer->GetState() == CStimResponseTimer::SRTS_RUNNING)
			{
				if (timer->Tick(ticks))
				{
					//gameLocal.Printf("Stim timer elapsed! ProcessStimResponse - firing stim\n");
					// Enable the stim when the timer has expired

					stim->Enable();
				}
			}
		}
	}

	int n;
	idClip_EntityList srEntities;

	// Now check the rest of the stims.
	for (int i = 0; i < m_StimEntity.Num(); i++)
	{
		idEntity* entity = m_StimEntity[i].GetEntity();

		if (entity == NULL) continue;

		TRACE_CPU_SCOPE_STR( "Process:Stim", entity->name );

		// greebo: Get the S/R collection, this is always non-NULL
		CStimResponseCollection* srColl = entity->GetStimResponseCollection();
		assert(srColl != NULL);

		idVec3 entityOrigin = entity->GetPhysics()->GetOrigin();

		int numStims = srColl->GetNumStims();

		// Traverse through all the stims on this entity
		for (int stimIdx = 0; stimIdx < numStims; ++stimIdx)
		{
			const CStimPtr& stim = srColl->GetStim(stimIdx);

			if (stim->m_MaxFireCount == 0)
			{
				// Maximum number of firings reached, do not process this stim
				continue;
			}

			if (stim->m_State != SS_ENABLED)
			{
				continue; // Stim is not enabled
			}

			if (stim->m_bCollisionBased && !stim->m_bCollisionFired)
			{
				continue; // Collision-based stim that did not fire
			}
			if (stim->m_bScriptBased && !stim->m_bScriptFired)
			{
				continue; // Script-driven stim that did not fire
			}

			// Check the interleaving timer and don't eval stim if it's not up yet
			if ( gameLocal.time - stim->m_TimeInterleaveStamp < stim->m_TimeInterleave )
			{
				continue;
			}

			// If stim has a finite duration, check if it expired. If so, disable the stim.
			if (stim->m_Duration != 0 && (gameLocal.time - stim->m_EnabledTimeStamp) > stim->m_Duration )
			{
				stim->Disable();
				continue;
			}

			// Initial checks passed, decrease the fire count
			if (stim->m_MaxFireCount > 0)
			{
				stim->m_MaxFireCount--;
			}

			idVec3 origin = entityOrigin;
			if (stim->m_bScriptBased && stim->m_ScriptPositionOverride != vec3_zero) {
				// stgatilov: script-based stim with overriden position
				origin = stim->m_ScriptPositionOverride;
			}

			// Check if a stim velocity has been specified
			if (stim->m_Velocity != idVec3(0,0,0)) {
				// The velocity mutliplied by the time gives the translation
				// Updates the location of this stim relative to the time it last fired.
				origin += stim->m_Velocity * (gameLocal.time - stim->m_EnabledTimeStamp)/1000;
			}

			if (stim->m_TimeInterleave > 0) {
				// Save the current timestamp into the stim, so that we know when it was last fired
				// stgatilov: save exact offset modulo m_TimeInterleave to save even distribution
				while ( gameLocal.time - stim->m_TimeInterleaveStamp >= stim->m_TimeInterleave)
					stim->m_TimeInterleaveStamp += stim->m_TimeInterleave;
			}
			else {
				stim->m_TimeInterleaveStamp = gameLocal.time;
			}

			// greebo: Check if the stim passes the "chance" test
			// Do this AFTER the m_TimeInterleaveStamp has been set to avoid the stim
			// from being re-evaluated in the very next frame in case it failed the test.
			if (!stim->CheckChance())
			{
				continue;
			}

			// If stim is not disabled
			if (stim->m_State == SS_DISABLED)
			{
				continue;
			}

			float radius = stim->GetRadius();

			if (radius != 0.0 || stim->m_bCollisionBased ||
				stim->m_bUseEntBounds || stim->m_Bounds.GetVolume() > 0)
			{
				int numResponses = 0;

				idBounds bounds;
				// Check if we have fixed bounds to work with (sr_bounds_mins & maxs set)
				if (stim->m_Bounds.GetVolume() > 0) {
					bounds = idBounds(stim->m_Bounds[0] + origin, stim->m_Bounds[1] + origin);
				}
				else {
					// No bounds set, check for radius and useBounds

					// Find entities in the radius of the stim
					if (stim->m_bUseEntBounds )
					{
						bounds = entity->GetPhysics()->GetAbsBounds();
					}
					else
					{
						bounds = idBounds(origin);
					}

					bounds.ExpandSelf(radius);
				}

				// Collision-based stims
				if (stim->m_bCollisionBased)
				{
					n = stim->m_CollisionEnts.Num();

					srEntities.SetNum(n);
					for (int n2 = 0; n2 < n; n2++)
					{
						srEntities[n2] = stim->m_CollisionEnts[n2];
					}

					// clear the collision vars for the next frame
					stim->m_bCollisionFired = false;
					stim->m_CollisionEnts.Clear();
				}
				else 
				{
					// Radius based stims
					n = clip.EntitiesTouchingBounds(bounds, CONTENTS_RESPONSE, srEntities);
					//DM_LOG(LC_STIM_RESPONSE, LT_INFO)LOGSTRING("Entities touching bounds: %d\r", n);
				}
				
				if (n > 0)
				{
					if (cv_sr_show.GetInteger() > 1)
					{
						for (int n2 = 0; n2 < n; ++n2)
						{
							// Show failed S/R
							gameRenderWorld->DebugArrow( colorRed, bounds.GetCenter(), srEntities[n2]->GetPhysics()->GetOrigin(), 1, 4 * USERCMD_MSEC );
						}
					}

					// Do responses for entities within the radius of the stim
					numResponses = DoResponseAction(stim, srEntities, entity, origin);
				}

				// The stim has fired, let it do any post-firing activity it may have
				stim->PostFired(numResponses);

				if (stim->m_bScriptBased)
				{
					// stgatilov: clear fired state of script-driven stim
					// this should be done AFTER stim is processed, since overrides are during response processing
					stim->m_bScriptFired = false;
					stim->m_ScriptRadiusOverride = -1.0f;
					stim->m_ScriptPositionOverride = vec3_zero;
				}
			}
		}
	}

	srTimer.Stop();
	DM_LOG(LC_STIM_RESPONSE, LT_INFO)LOGSTRING("Processing S/R took %lf\r", srTimer.Milliseconds());
}

/*
===================
Dark Mod:
idGameLocal::LocationForArea
===================
*/
idLocationEntity *idGameLocal::LocationForArea( const int areaNum ) 
{
	idLocationEntity *pReturnval( NULL );
	
	if ( !locationEntities || areaNum < 0 || areaNum >= gameRenderWorld->NumAreas() ) 
	{
		pReturnval = NULL;
		goto Quit;
	}

	pReturnval = locationEntities[ areaNum ];

Quit:
	return pReturnval;
}

/*
===================
Dark Mod:
idGameLocal::PlayerTraceEntity
===================
*/
idEntity *idGameLocal::PlayerTraceEntity( void )
{
	idVec3		start, end;
	idEntity	*returnEnt = NULL;
	idPlayer	*player = NULL;
	trace_t		trace;
	int			cm = 0;

	player = gameLocal.GetLocalPlayer();
	if( ! player )
		goto Quit;

	start = player->GetEyePosition();
	// do a trace 512 doom units ahead
	end = start + 512.0f * (player->viewAngles.ToForward());

	cm = CONTENTS_SOLID | CONTENTS_BODY | CONTENTS_CORPSE | CONTENTS_TRIGGER;
	gameLocal.clip.TracePoint( trace, start, end, cm, player );
	if( trace.fraction < 1.0f )
	{
		returnEnt = gameLocal.entities[ trace.c.entityNum ];
		// don't return the world
		if( returnEnt == gameLocal.world )
			returnEnt = NULL;
	}
	
Quit:
	return returnEnt;
}

void idGameLocal::PauseGame( bool bPauseState )
{
	if( bPauseState )
	{
		gameSoundWorldBuf = soundSystem->GetPlayingSoundWorld();
		gameSoundWorldBuf->Pause();
		soundSystem->SetPlayingSoundWorld( NULL );
	
		g_stopTime.SetBool( true );
	}
	else
	{
		soundSystem->SetPlayingSoundWorld( gameSoundWorldBuf );
		soundSystem->GetPlayingSoundWorld()->UnPause();
		
		g_stopTime.SetBool( false );
	}
}

void idGameLocal::SetMissionResult(EMissionResult result)
{
	m_MissionResult = result;
}

EMissionResult idGameLocal::GetMissionResult() const 
{
	return m_MissionResult;
}

float idGameLocal::CalcLightgem( idPlayer *a_pPlayer )
{
	return m_lightGem.Calculate( a_pPlayer );
}
/*
===================
Dark Mod:
idGameLocal::FindMainAmbientLight
Author: J.C.Denton 
Usage:	Finds main ambient light by the name "ambient_world". 
		If it can't be found, finds ambient light with largest radius and rename it to ambient_world before returning pointer to it.
		grayman - The renaming isn't done. The found light's name is saved as the main ambient light.
===================
*/

idLight* idGameLocal::FindMainAmbientLight( bool a_bCreateNewIfNotFound /*= false */ )
{
	int hash, i;
	idEntity *pEntMainAmbientLight = NULL;

	hash = entityHash.GenerateKey( m_strMainAmbientLightName, true );
	for ( i = entityHash.First( hash ) ; i != -1 ; i = entityHash.Next( i ) )
	{
		if ( entities[i] && entities[i]->name.Icmp( m_strMainAmbientLightName ) == 0 )
		{
			pEntMainAmbientLight =  entities[i];
			break;
		}
	}

	if ( ( NULL != pEntMainAmbientLight ) && pEntMainAmbientLight->IsType(idLight::Type) )
	{
		return static_cast<idLight *>( pEntMainAmbientLight );
	}

	if ( !a_bCreateNewIfNotFound )
	{
		return NULL;
	}

	gameLocal.Printf("Ambient light 'ambient_world' not found, attempting to create a new one.\n"); 

	idLight *pLightEntMainAmbient = NULL;
	float fMaxRadius = 0.0f;

	// Find the ambient light with greatest radius.
	// grayman #3584 - use the list of ambient lights

	for ( i = 0 ; i < m_ambientLights.Num() ; i++ )
	{
		idLight *plight = m_ambientLights[i].GetEntity();
		if ( plight != NULL )
		{
			idVec3 vec3LightRadius; 
			plight->GetRadius( vec3LightRadius );
			float fRadius = vec3LightRadius.LengthFast();

			if ( fRadius > fMaxRadius )
			{
				fMaxRadius = fRadius;
				pLightEntMainAmbient = plight;
			}
		}
	}

	if ( pLightEntMainAmbient )
	{
		m_strMainAmbientLightName = pLightEntMainAmbient->GetName();
		gameLocal.Printf( "Found light '%s' and it's now set as the main ambient light.\n", m_strMainAmbientLightName.c_str() ); 
	}

	return pLightEntMainAmbient;
}

void idGameLocal::ClearPersistentInfo()
{
	m_CampaignStats.reset(new CampaignStats);
	m_InterMissionTriggers.Clear();
	persistentPlayerInventory->Clear();

	persistentLevelInfo.Clear();
	if (persistentLevelInfoLocation == PERSISTENT_LOCATION_GAME)
		persistentLevelInfoLocation = PERSISTENT_LOCATION_NOWHERE;
}

void idGameLocal::ClearPersistentInfoInGui(idUserInterface *gui)
{
	const idDict &allGuiVars = gui->State();
	idStrList keys;
	for (const idKeyValue *kv = allGuiVars.MatchPrefix("persistent_"); kv; kv = allGuiVars.MatchPrefix("persistent_", kv))
		keys.Append(kv->GetKey());

	for (idStr key : keys)
		gui->DeleteStateVar(key);

	if (persistentLevelInfoLocation == PERSISTENT_LOCATION_MAINMENU)
		persistentLevelInfoLocation = PERSISTENT_LOCATION_NOWHERE;
}

void idGameLocal::SyncPersistentInfoToGui(idUserInterface *gui, bool moveOwnership)
{
	assert(persistentLevelInfoLocation == PERSISTENT_LOCATION_GAME);
	ClearPersistentInfoInGui(gui);

	for (const idKeyValue *kv = persistentLevelInfo.MatchPrefix(""); kv; kv = persistentLevelInfo.MatchPrefix("", kv)) {
		idStr key = "persistent_" + kv->GetKey();
		gui->SetStateString(key, kv->GetValue());
	}

	// mark main menu GUI vars as the source of truth from now on
	if (moveOwnership)
		persistentLevelInfoLocation = PERSISTENT_LOCATION_MAINMENU;
}

void idGameLocal::SyncPersistentInfoFromGui(const idUserInterface *gui, bool moveOwnership)
{
	assert(persistentLevelInfoLocation == PERSISTENT_LOCATION_MAINMENU);
	persistentLevelInfo.Clear();

	const idDict &allGuiVars = gui->State();
	for (const idKeyValue *kv = allGuiVars.MatchPrefix("persistent_"); kv; kv = allGuiVars.MatchPrefix("persistent_", kv)) {
		idStr key = kv->GetKey().c_str() + strlen("persistent_");
		persistentLevelInfo.Set(key, kv->GetValue());
	}

	// mark game as the source of truth from now on
	if (moveOwnership)
		persistentLevelInfoLocation = PERSISTENT_LOCATION_GAME;
}

void idGameLocal::ProcessInterMissionTriggers()
{
	for (int i = 0; i < m_InterMissionTriggers.Num(); ++i)
	{
		const InterMissionTrigger& trigger = m_InterMissionTriggers[i];

		if (trigger.missionNum == m_MissionManager->GetCurrentMissionIndex())
		{
			idEntity* target = FindEntity(trigger.targetName);
			idEntity* activator = !trigger.activatorName.IsEmpty() ? FindEntity(trigger.activatorName) : GetLocalPlayer();

			if (target != NULL)
			{
				target->Activate(activator);
			}
			else
			{
				gameLocal.Warning("Cannot find intermission trigger target entity %s", trigger.targetName.c_str());
			}

			// Don't remove the triggers yet, we might need them again when restarting the map
		}
	}
}

void idGameLocal::OnReloadImages()
{
	idPlayer* player = GetLocalPlayer();

	if (player != NULL)
	{
		player->GetPlayerView().OnReloadImages();
	}
}

void idGameLocal::OnVidRestart()
{
	// grayman #3807 - set spyglass overlay per aspect ratio
	int ar = DetermineAspectRatio();
	m_spyglassOverlay = ar;

	// grayman #4882 - set peeking overlay per aspect ratio
	m_peekOverlay = ar;

	idPlayer* player = GetLocalPlayer();

	if (player != NULL)
	{
		player->GetPlayerView().OnVidRestart();
	}
}

// grayman #3556 - Engine asks whether player is underwater

bool idGameLocal::PlayerUnderwater()
{
	bool result = false;

	idPlayer* player = GetLocalPlayer();

	if ( player )
	{
		idPhysics* phys = player->GetPhysics();

		if ( phys && phys->IsType(idPhysics_Actor::Type) )
		{
			waterLevel_t waterLevel = static_cast<idPhysics_Actor*>(phys)->GetWaterLevel();
			result = (waterLevel == WATERLEVEL_HEAD);
		}
	}

	return result;
}

// grayman #3317 - Clear the time a stim last fired from an entity, so that
// it can be fired immediately instead of waiting for the next time it was
// scheduled to fire.

void idGameLocal::AllowImmediateStim( idEntity* e, int stimType )
{
	// Find the entity on the list of stimming entities
	for ( int i = 0 ; i < m_StimEntity.Num() ; i++ )
	{
		idEntity* entity = m_StimEntity[i].GetEntity();
		if ( entity == NULL )
		{
			continue;
		}
		if ( entity == e )
		{
			CStimResponseCollection* srColl = entity->GetStimResponseCollection();
			assert( srColl != NULL );
			int numStims = srColl->GetNumStims();

			// Traverse through the stims on this entity, searching
			// for a match with the type we want
			for ( int stimIdx = 0 ; stimIdx < numStims ; ++stimIdx )
			{
				const CStimPtr& stim = srColl->GetStim(stimIdx);
				if ( stim->m_StimTypeId == (StimType) stimType )
				{
					stim->m_TimeInterleaveStamp = 0; // allow immediate firing of this stim
					break;
				}
			}
			break;
		}
	}
}

// grayman #3355

int idGameLocal::GetNextMessageTag()
{
	m_uniqueMessageTag++;
	return ( m_uniqueMessageTag );
}

// grayman #3807

int idGameLocal::GetSpyglassOverlay()
{
	return m_spyglassOverlay;
}

// grayman #4882

int idGameLocal::GetPeekOverlay()
{
	return m_peekOverlay;
}


// grayman #3857
SuspiciousEvent* idGameLocal::FindSuspiciousEvent( int eventID ) // grayman #3857
{
	if ( eventID < 0 )
	{
		return NULL;
	}

	return &m_suspiciousEvents[eventID];
}

// grayman #3424

int idGameLocal::FindSuspiciousEvent( EventType type, idVec3 location, idEntity* entity, int time ) // grayman #3857
{
	for ( int i = 0 ; i < gameLocal.m_suspiciousEvents.Num() ; i++ )
	{
		SuspiciousEvent se = gameLocal.m_suspiciousEvents[i];

		if ( se.type == type ) // type of event
		{
			// grayman #3848 - must use separate booleans
			bool locationMatch = true;
			bool entityMatch   = true;
			bool timeMatch	   = true; // grayman #3857

			// check location

			if ( !location.Compare(idVec3(0,0,0)) )
			{
				if (se.type == E_EventTypeNoisemaker)
				{
					// grayman #3857 - noisemakers must be considered
					// separate events, so the locations must match

					locationMatch = location.Compare(se.location);
				}
				else
				{
					// Allow for some variance in location. Two events of
					// the same type that are near each other should be
					// considered the same event.

					float distSqr = (se.location - location).LengthSqr();
					locationMatch = (distSqr <= 90000); // 300*300
				}
			}

			// check entity

			if ( entity != NULL )
			{
				if ( se.entity.GetEntity() != entity )
				{
					entityMatch = false;
				}
			}

			// grayman #3857 - check timestamp

			if ( time > 0 )
			{
				// time must match exactly

				timeMatch = ( time == se.time);
			}

			if ( locationMatch && entityMatch && timeMatch ) // grayman #3857
			{
				return i;
			}
		}
	}
	return -1;
}

// duzenko #4409 - last frame time in msec, used for head bob cycling, physics
int idGameLocal::getMsec() {
	return time - previousTime;
}

int idGameLocal::LogSuspiciousEvent( SuspiciousEvent se, bool forceLog ) // grayman #3857   
{
	int index = -1;

	// See if this event has already been logged

	if (!forceLog)
	{
		if ( se.type == E_EventTypeEnemy )
		{
			index = FindSuspiciousEvent( E_EventTypeEnemy, se.location, NULL, 0 );
		}
		else if ( se.type == E_EventTypeDeadPerson )
		{
			index = FindSuspiciousEvent( E_EventTypeDeadPerson, idVec3(0,0,0), se.entity.GetEntity(), 0 );
		}
		else if ( se.type == E_EventTypeUnconsciousPerson ) // grayman #3857
		{
			index = FindSuspiciousEvent( E_EventTypeUnconsciousPerson, idVec3(0,0,0), se.entity.GetEntity(), 0 );
		}
		else if ( se.type == E_EventTypeMissingItem )
		{
			index = FindSuspiciousEvent( E_EventTypeMissingItem, se.location, se.entity.GetEntity(), 0 );
		}
		else if ( se.type == E_EventTypeMisc ) // grayman #3857
		{
			index = FindSuspiciousEvent( E_EventTypeMisc, se.location, se.entity.GetEntity(), se.time );
		}
		else if ( se.type == E_EventTypeNoisemaker ) // grayman #3857
		{
			index = FindSuspiciousEvent( E_EventTypeNoisemaker, se.location, NULL, 0 );
		}
		else if ( se.type == E_EventTypeSound ) // grayman #3857
		{
			index = FindSuspiciousEvent( E_EventTypeSound, idVec3(0,0,0), NULL, se.time );
		}
	}

	if ( index < 0 )
	{
		gameLocal.m_suspiciousEvents.Append(se); // log this new event
		index = gameLocal.m_suspiciousEvents.Num() - 1;
	}

	return index;
}




