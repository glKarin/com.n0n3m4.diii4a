// Standard headers.
#include <stdlib.h> // rand().
#include <time.h> // time()

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    #define getcwd _getcwd
#else
    #include <unistd.h> // readlink()
#endif

// Good headers.
#include <good/file.h>
#include <good/string_buffer.h>

// Plugin headers.
#include "bot.h"
#include "clients.h"
#include "config.h"
#include "console_commands.h"
#include "item.h"
#include "event.h"
#include "icvar.h"
#include "server_plugin.h"
#include "source_engine.h"
#include "mod.h"
#include "waypoint.h"

// Source headers.
// Ugly fix for Source Engine.
/*#undef MINMAX_H
#undef min
#undef max*/
#include "cbase.h"
#include "filesystem.h"
#include "interface.h"
#include "eiface.h"
#include "iplayerinfo.h"
#include "convar.h"
#include "IEngineTrace.h"
#include "IEffects.h"
#include "ndebugoverlay.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define BUFFER_LOG_SIZE (8*1024)

char szMainBufferArray[BUFFER_LOG_SIZE];
char szLogBufferArray[BUFFER_LOG_SIZE];  // Static buffers for different string purposes.

int iMainBufferSize = BUFFER_LOG_SIZE;
char* szMainBuffer = szMainBufferArray;

// For logging purpuses.
int iLogBufferSize = BUFFER_LOG_SIZE;
char* szLogBuffer = szLogBufferArray;

//----------------------------------------------------------------------------------------------------------------
// The plugin is a static singleton that is exported as an interface.
//----------------------------------------------------------------------------------------------------------------
static CBotrixPlugin g_cBotrixPlugin;
CBotrixPlugin* CBotrixPlugin::instance = &g_cBotrixPlugin;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR(CBotrixPlugin, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS, g_cBotrixPlugin);

//----------------------------------------------------------------------------------------------------------------
// CBotrixPlugin static members.
//----------------------------------------------------------------------------------------------------------------
int CBotrixPlugin::iFPS = 60;
float CBotrixPlugin::fTime = 0.0f;
float CBotrixPlugin::fEngineTime = 0.0f;
float CBotrixPlugin::m_fFpsEnd = 0.0f;
int CBotrixPlugin::m_iFramesCount = 60;


//----------------------------------------------------------------------------------------------------------------
good::string sBotrix("botrix");
good::string sConfigIni("config.ini");

//----------------------------------------------------------------------------------------------------------------
bool find_config_ini( good::string_buffer& sbBuffer )
{
    bool bResult = false;

    sbBuffer << PATH_SEPARATOR << sBotrix << PATH_SEPARATOR << sConfigIni;

    BLOG_T( "Looking for configuration file %s", sbBuffer.c_str() );
    if ( good::file::exists(sbBuffer.c_str()) )
    {
        BLOG_T( "  found.");
        bResult = true;
    }
    else
    {
        // Go one level up.
        int iPos = sbBuffer.rfind(PATH_SEPARATOR, sbBuffer.size() - sConfigIni.size() - sBotrix.size() - 3);
        if ( iPos != good::string::npos )
        {
            sbBuffer.erase(iPos);
            sbBuffer << PATH_SEPARATOR << sBotrix << PATH_SEPARATOR << sConfigIni;

            BLOG_T( "Looking for configuration file %s", sbBuffer.c_str() );
            if ( good::file::exists(sbBuffer.c_str()) )
            {
                BLOG_T( "  found.");
                bResult = true;
            }
        }
    }
    return bResult;
}

//----------------------------------------------------------------------------------------------------------------
// Interfaces from the CBotrixPlugin::pEngineServer.
//----------------------------------------------------------------------------------------------------------------
IVEngineServer* CBotrixPlugin::pEngineServer = NULL;

#ifdef USE_OLD_GAME_EVENT_MANAGER
IGameEventManager* CBotrixPlugin::pGameEventManager = NULL;
#else
IGameEventManager2* CBotrixPlugin::pGameEventManager = NULL;
#endif
IPlayerInfoManager* CBotrixPlugin::pPlayerInfoManager = NULL;
IServerPluginHelpers* CBotrixPlugin::pServerPluginHelpers = NULL;
IServerGameClients* CBotrixPlugin::pServerGameClients = NULL;
//IServerGameEnts* CBotrixPlugin::pServerGameEnts;
IEngineTrace* CBotrixPlugin::pEngineTrace = NULL;
IEffects* pEffects = NULL;
IBotManager* CBotrixPlugin::pBotManager = NULL;
//CGlobalVars* CBotrixPlugin::pGlobalVars = NULL;
ICvar* CBotrixPlugin::pCVar = NULL;

IVDebugOverlay* pVDebugOverlay = NULL;

//----------------------------------------------------------------------------------------------------------------
// Plugin functions.
//----------------------------------------------------------------------------------------------------------------
#define LOAD_INTERFACE(var,type,version) \
    if ((var =(type*)pInterfaceFactory(version, NULL)) == NULL ) {\
        BLOG_W("Cannot open interface " version);\
        return false;\
    }

#define LOAD_INTERFACE_IGNORE_ERROR(var,type,version) \
    if ((var =(type*)pInterfaceFactory(version, NULL)) == NULL ) {\
        BLOG_W("Cannot open interface " version);\
    }

#define LOAD_GAME_SERVER_INTERFACE(var, type, version) \
    if ((var =(type*)pGameServerFactory(version, NULL)) == NULL ) {\
        BLOG_W("Cannot open game server interface " version);\
        return false;\
    }

//----------------------------------------------------------------------------------------------------------------
CBotrixPlugin::CBotrixPlugin(): m_bEnabled(true) {}

//----------------------------------------------------------------------------------------------------------------
CBotrixPlugin::~CBotrixPlugin() {}

//----------------------------------------------------------------------------------------------------------------
// Called when the plugin is loaded. Load all needed interfaces using engine.
//----------------------------------------------------------------------------------------------------------------
bool CBotrixPlugin::Load( CreateInterfaceFn pInterfaceFactory, CreateInterfaceFn pGameServerFactory )
{
    good::log::bLogToStdOut = false; // Disable log to stdout, Msg() will print there.
    good::log::bLogToStdErr = false; // Disable log to stderr, Warning() will print there.
    good::log::iStdErrLevel = good::ELogLevelWarning; // Log warnings and errors to stderr.
    CUtil::iLogLevel = good::log::iLogLevel = good::ELogLevelTrace; // Trace before loading config.ini
    good::log::set_prefix("[Botrix] ");

#ifndef DONT_USE_VALVE_FUNCTIONS
    LOAD_GAME_SERVER_INTERFACE(pPlayerInfoManager, IPlayerInfoManager, INTERFACEVERSION_PLAYERINFOMANAGER);

    //pGlobalVars = pPlayerInfoManager->GetGlobalVars();

#ifdef OSX
    LOAD_INTERFACE(pEngineServer, IVEngineServer, INTERFACEVERSION_VENGINESERVER_VERSION_21);
#else
    LOAD_INTERFACE(pEngineServer, IVEngineServer, INTERFACEVERSION_VENGINESERVER);
#endif
    LOAD_INTERFACE(pServerPluginHelpers, IServerPluginHelpers, INTERFACEVERSION_ISERVERPLUGINHELPERS);
    LOAD_INTERFACE(pEngineTrace, IEngineTrace, INTERFACEVERSION_ENGINETRACE_SERVER);
    LOAD_GAME_SERVER_INTERFACE(pEffects, IEffects, IEFFECTS_INTERFACE_VERSION);
    LOAD_GAME_SERVER_INTERFACE(pBotManager, IBotManager, INTERFACEVERSION_PLAYERBOTMANAGER);

    LOAD_INTERFACE_IGNORE_ERROR(pVDebugOverlay, IVDebugOverlay, VDEBUG_OVERLAY_INTERFACE_VERSION);
#ifdef USE_OLD_GAME_EVENT_MANAGER
    LOAD_INTERFACE(pGameEventManager, IGameEventManager,  INTERFACEVERSION_GAMEEVENTSMANAGER);
#else
    LOAD_INTERFACE(pGameEventManager, IGameEventManager2, INTERFACEVERSION_GAMEEVENTSMANAGER2);
#endif

    LOAD_GAME_SERVER_INTERFACE(pServerGameClients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
    //LOAD_GAME_SERVER_INTERFACE(pServerGameEnts, IServerGameEnts, INTERFACEVERSION_SERVERGAMECLIENTS);

#ifdef BOTRIX_SOURCE_ENGINE_2006
    LOAD_INTERFACE(pCVar, ICvar, VENGINE_CVAR_INTERFACE_VERSION);
#else
    LOAD_INTERFACE(pCVar, ICvar, CVAR_INTERFACE_VERSION);
#endif

#endif // DONT_USE_VALVE_FUNCTIONS


    // Get game/mod and botrix base directories.
    good::string sIniPath;

#ifdef DONT_USE_VALVE_FUNCTIONS
    #define BOTRIX_XSTRINGIFY(a) BOTRIX_STRINGIFY(a)
    #define BOTRIX_STRINGIFY(a) #a
    strcpy(szMainBuffer, BOTRIX_XSTRINGIFY(DONT_USE_VALVE_FUNCTIONS)); // Mod directory.
#else
    pEngineServer->GetGameDir(szMainBuffer, iMainBufferSize); // Mod directory.
#endif

    // Get full path of game directory in sbBuffer.
    good::string_buffer sbBuffer(szMainBuffer, iMainBufferSize, false, false); // Don't deallocate nor clear.
    if ( good::ends_with(sbBuffer, PATH_SEPARATOR) )
        sbBuffer.erase( sbBuffer.size()-1, 1 );

    // Get mod folder (-game argument of hl2/srcds executable).
    int iPos = sbBuffer.rfind(PATH_SEPARATOR);
    if ( iPos == good::string::npos )
        iPos = 0;
    else
        iPos++;
    sModFolder.assign(&szMainBuffer[iPos], sbBuffer.size()-iPos, true); // Allocate new string.
    good::lower_case(sModFolder);

    // Check if configuration file exists in "mod directory/botrix" and "mod directory/addons/botrix".
    sbBuffer << PATH_SEPARATOR << "addons";
    if ( find_config_ini(sbBuffer) )
    {
        sIniPath.assign(sbBuffer, true);
        sBotrixPath.assign( sbBuffer.c_str(), sbBuffer.size() - sConfigIni.size() - 1, true);
    }

    // Find hl2/srcds executable directory (game directory).
#ifdef _WIN32
    iPos = GetModuleFileName(NULL, szLogBuffer, iLogBufferSize);
#else
    iPos = readlink("/proc/self/exe", szLogBuffer, iLogBufferSize);
    if ( iPos < 0 )
        iPos = 0;
#endif

    szLogBuffer[iPos] = 0;
    GoodAssert( good::file::absolute(szLogBuffer) );
    sbBuffer = szLogBuffer;

    // Remove exe name.
    good::file::dir(sbBuffer);

    // Get game folder (where hl2/srcds executable is located).
    iPos = sbBuffer.rfind(PATH_SEPARATOR);
    if ( iPos == good::string::npos )
        iPos = 0;
    else
        iPos++;
    sGameFolder.assign(&szMainBuffer[iPos], sbBuffer.size()-iPos, true); // Allocate new string.
    good::lower_case(sGameFolder);

    // Check if configuration file exists in mod directory/botrix and one directory up.
    if ( sIniPath.size() == 0 )
    {
        if ( find_config_ini(sbBuffer) )
            sIniPath.assign(sbBuffer, true);

        // Set botrix path to game/../ directory.
        sbBuffer.erase(sbBuffer.size() - sConfigIni.size() - 1, sConfigIni.size() + 1);
        sBotrixPath.assign( sbBuffer.c_str(), sbBuffer.size(), true);
        sbBuffer << PATH_SEPARATOR;
        good::file::make_folders( sbBuffer.c_str() );
    }

    // Load configuration file.
    TModId iModId = EModId_Invalid;
    if ( sIniPath.size() > 0 )
    {
        CConfiguration::SetFileName(sIniPath);
        iModId = CConfiguration::Load(sGameFolder, sModFolder);
        GoodAssert(iModId != EModId_Invalid);
    }
	
    if ( iModId == EModId_Invalid )
    {
        BLOG_E("Configuration file not founded.");
        BLOG_W("Using default mod HalfLife2Deathmatch without items/weapons.");
        CMod::sModName = "HalfLife2Deathmatch";
        iModId = EModId_HL2DM;
        CUtil::iLogLevel = good::ELogLevelInfo;
        good::log::iFileLogLevel = good::ELogLevelDebug;

        sbBuffer = sBotrixPath;
        sbBuffer << PATH_SEPARATOR << "botrix.log";
        if ( good::log::start_log_to_file( sbBuffer.c_str(), false ) )
            BLOG_I("Log to file: %s.", sbBuffer.c_str());
        else
            BLOG_E( "Can't open log file %s.", sbBuffer.c_str() );

        sbBuffer = sBotrixPath;
        sbBuffer << PATH_SEPARATOR << "config.ini";
        CConfiguration::SetFileName(sbBuffer);
        CConfiguration::SetClientAccessLevel("STEAM_ID_LAN", FCommandAccessAll);

        BLOG_W( "Using default teams (unassigned, spectator, unknown1, unknown2)." );
        CMod::aTeamsNames.push_back("unassigned");
        CMod::aTeamsNames.push_back("spectator");
        CMod::aTeamsNames.push_back("unknown1");
        CMod::aTeamsNames.push_back("unknown2");

        BLOG_W( "Using default bot name (Botrix)." );
        CMod::aBotNames.push_back("Botrix");
    }

    // Create console command instance.
    CBotrixCommand::instance = new CBotrixCommand();
	
    // Load mod configuration.
    CMod::LoadDefaults(iModId);

    

    // Load mod dependent config. For example, HL2DM will load models.
    if ( CMod::pCurrentMod )
        CMod::pCurrentMod->ProcessConfig( CConfiguration::m_iniFile );

    //BLOG_W("  No weapons available.");
    //BLOG_W("  No models of type 'object' available.");
    //else if ( (iType != EItemTypeAmmo) && (iType != EItemTypeWeapon) ) // Will load ammo&weapons later.
    //    BLOG_W("  No entities of type '%s' available.", CTypeToString::EntityTypeToString(iType).c_str());


#ifdef BOTRIX_SOURCE_ENGINE_2006
    MathLib_Init(1.0, 1.0, 1.0, 1.0, false, true, false, true);
#else
    MathLib_Init();
#endif
    srand( time(NULL) );

	CItems::PrintClasses();

    const char* szMod = CMod::sModName.c_str();
    if ( CMod::sModName.size() == 0 )
        szMod = "unknown";
	
    BLOG_I("Botrix loaded. Current mod: %s.", szMod);

    return true;
}

//----------------------------------------------------------------------------------------------------------------
// Called when the plugin is unloaded
//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::Unload( void )
{
    CConfiguration::Unload();
    CBotrixCommand::instance.reset();
    CMod::UnLoad();

    good::log::stop_log_to_file();
}

//----------------------------------------------------------------------------------------------------------------
// Called when the plugin is paused(i.e should stop running but isn't unloaded)
//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::Pause( void )
{
    CBotrixCommand::instance.reset();
    LevelShutdown();
    BLOG_W( "Plugin paused." );
}

//----------------------------------------------------------------------------------------------------------------
// Called when the plugin is unpaused(i.e should start executing again)
//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::UnPause( void )
{
    BLOG_W( "Plugin unpaused. It will not work until level change." );
    CBotrixCommand::instance = new CBotrixCommand();
}

//----------------------------------------------------------------------------------------------------------------
// the name of this plugin, returned in "plugin_print" command
//----------------------------------------------------------------------------------------------------------------
const char* CBotrixPlugin::GetPluginDescription( void )
{
    return "Botrix plugin " PLUGIN_VERSION " (c) 2012 by Borzh. BUILD " __DATE__;
}

//----------------------------------------------------------------------------------------------------------------
// Called on level start
//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::LevelInit( const char* szMapName )
{
    PrepareLevel(szMapName);
}

//----------------------------------------------------------------------------------------------------------------
// Called on level start, when the server is ready to accept client connections
// edictCount is the number of entities in the level, clientMax is the max client count
//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::ServerActivate( edict_t* /*pEdictList*/, int /*edictCount*/, int clientMax )
{
    ActivateLevel(clientMax);
}

//----------------------------------------------------------------------------------------------------------------
// Called once per server frame, do recurring work here(like checking for timeouts)
//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::GameFrame( bool /*simulating*/ )
{
    if ( bMapRunning && m_bEnabled )
    {
        // try
        // {
            if ( CWaypoints::IsAnalyzing() )
                CWaypoints::AnalyzeStep();
        // }
        // catch ( ... )
        // {
        //     BLOG_E( "FATAL EXCEPTION in CWaypoints::AnalyzeStep'." );
        //     BLOG_E( "Please report to botrix.plugin@gmail.com, attaching the log file." );
        //     GoodAssert( false );
        // }
        

        // try
        // {
            CUtil::PrintMessagesInQueue();
        // }
        // catch ( ... )
        // {
        //     BLOG_E( "FATAL EXCEPTION in CUtil::PrintMessagesInQueue." );
        //     BLOG_E( "Please report to botrix.plugin@gmail.com, attaching the log file." );
        //     GoodAssert( false );
        // }

        float fPrevEngineTime = fEngineTime;
        fEngineTime = pEngineServer->Time();

        float fDiff = fEngineTime - fPrevEngineTime;
#if defined(_DEBUG) || defined(DEBUG)
        if ( fDiff > 1.0f ) // Too low fps, possibly debugging.
            fTime += 0.1f;
        else
#endif
            fTime += fDiff;

		// FPS counting. Used in draw waypoints.
#ifdef SHOW_FPS
		m_iFramesCount++;
		if (fEngineTime >= m_fFpsEnd)
		{
			iFPS = m_iFramesCount;
			m_iFramesCount = 0;
			m_fFpsEnd = fEngineTime + 1.0f;
			//BLOG_T(NULL, "FPS: %d", iFPS);
		}
#endif

        // try
        // {
            CMod::Think();
        // }
        // catch ( ... )
        // {
        //     BLOG_E( "FATAL EXCEPTION in CMod::Think()." );
        //     BLOG_E( "Please report to botrix.plugin@gmail.com, attaching the log file." );
        //     GoodAssert( false );
        // }
        

#ifdef SHOW_FPS
        m_iFramesCount++;
        if (fTime >= m_fFpsEnd)
        {
        	iFPS = m_iFramesCount;
        	m_iFramesCount = 0;
        	m_fFpsEnd = fTime + 1.0f;
        	BLOG_T("FPS: %d", iFPS);
        }
#endif

        // try
        // {
            CItems::Update();
        // }
        // catch ( ... )
        // {
        //     BLOG_E( "FATAL EXCEPTION in CItems::Update()." );
        //     BLOG_E( "Please report to botrix.plugin@gmail.com, attaching the log file." );
        //     GoodAssert( false );
        // }
        
        
        // try
        // {
            CPlayers::PreThink();
        // }
        // catch ( ... )
        // {
        //     BLOG_E( "FATAL EXCEPTION in CPlayers::PreThink()." );
        //     BLOG_E( "Please report to botrix.plugin@gmail.com, attaching the log file." );
        //     GoodAssert( false );
        // }

//#define BOTRIX_SHOW_PERFORMANCE
#ifdef BOTRIX_SHOW_PERFORMANCE
        static const float fInterval = 10.0f; // Print & reset every 10 seconds.
        static float fStart = 0.0f, fSum = 0.0f;
        static int iCount = 0;

        if ( fStart == 0.0f )
            fStart = fEngineTime;

        fSum += pEngineServer->Time() - fEngineTime;
        iCount++;

        if ( (fStart + fInterval) <= pEngineServer->Time() )
        {
            BLOG_T("Botrix think time in %d frames (%.0f seconds): %.5f msecs",
                   iCount, fInterval, fSum / (float)iCount * 1000.0f);
            fStart = fSum = 0.0f;
            iCount = 0;
        }
#endif
    }
}

//----------------------------------------------------------------------------------------------------------------
// Called on level end (as the server is shutting down or going to a new map)
//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::LevelShutdown( void )
{
    // This function gets called multiple times per map change.
    bMapRunning = false;
    sMapName = "";

    if ( CWaypoints::IsAnalyzing() )
        CWaypoints::StopAnalyzing(); // Don't save if currently analyzing.
    else if ( CWaypoint::bSaveOnMapChange )
        CWaypoints::Save();
        
    CWaypoints::ClearUnreachablePaths();
    CWaypoints::Clear();
    for ( int i = 0; i < CWaypoints::EAnalyzeWaypointsTotal; ++i )
        CWaypoints::AnalyzeClear( i );

    CPlayers::Clear();
    CItems::MapUnloaded( true );

#ifdef USE_OLD_GAME_EVENT_MANAGER
    pGameEventManager->RemoveListener(this);
#endif
}

//----------------------------------------------------------------------------------------------------------------
// Called when a client spawns into a server (i.e as they begin to play)
//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::ClientActive( edict_t* pEntity )
{
    if ( bMapRunning )
        CPlayers::PlayerConnected(pEntity);
}

//----------------------------------------------------------------------------------------------------------------
// Called when a client leaves a server(or is timed out)
//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::ClientDisconnect( edict_t* pEntity )
{
    if ( bMapRunning )
        CPlayers::PlayerDisconnected(pEntity);
}

//----------------------------------------------------------------------------------------------------------------
// Called on client being added to this server
//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::ClientPutInServer( edict_t* /*pEntity*/, const char* /*playername*/ )
{
}

//----------------------------------------------------------------------------------------------------------------
// Sets the client index for the client who typed the command into their console
//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::SetCommandClient( int /*index*/ )
{
}

//----------------------------------------------------------------------------------------------------------------
// A player changed one/several replicated cvars (name etc)
//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::ClientSettingsChanged( edict_t* /*pEdict*/ )
{
    //const char* name = pEngineServer->GetClientConVarValue( pEngineServer->IndexOfEdict(pEdict), "name" );
}

//----------------------------------------------------------------------------------------------------------------
// Called when a client joins a server.
//----------------------------------------------------------------------------------------------------------------
PLUGIN_RESULT CBotrixPlugin::ClientConnect( bool* /*bAllowConnect*/, edict_t* /*pEntity*/, const char* /*pszName*/,
                                            const char* /*pszAddress*/, char* /*reject*/, int /*maxrejectlen*/ )
{
    return PLUGIN_CONTINUE;
}

//----------------------------------------------------------------------------------------------------------------
// Called when a client types in a command (only a subset of commands however, not CON_COMMAND's)
//----------------------------------------------------------------------------------------------------------------
#ifdef BOTRIX_SOURCE_ENGINE_2006
PLUGIN_RESULT CBotrixPlugin::ClientCommand( edict_t* pEntity )
#else
PLUGIN_RESULT CBotrixPlugin::ClientCommand( edict_t* pEntity, const CCommand &args )
#endif
{
    GoodAssert( pEntity && !pEntity->IsFree() ); // Valve check.

#ifdef BOTRIX_SOURCE_ENGINE_2006
    int argc = MIN2(CBotrixPlugin::pEngineServer->Cmd_Argc(), 16);

    if ( (argc == 0) || !CBotrixCommand::instance->IsCommand( CBotrixPlugin::pEngineServer->Cmd_Argv(0) ) )
        return PLUGIN_CONTINUE;

    static const char* argv[16];
    for (int i = 0; i < argc; ++i)
        argv[i] = CBotrixPlugin::pEngineServer->Cmd_Argv(i);
#else
    int argc = args.ArgC();
    const char** argv = args.ArgV();

    if ( (argc == 0) || !CBotrixCommand::instance->IsCommand( argv[0] ) )
        return PLUGIN_CONTINUE; // Not a "botrix" command.
#endif

    CPlayer* pPlayer = CPlayers::Get(pEntity);
    GoodAssert( pPlayer && !pPlayer->IsBot() ); // Valve check.

    CClient* pClient = (CClient*)pPlayer;

    TCommandResult iResult = CBotrixCommand::instance->Execute( pClient, argc-1, &argv[1] );

	if ( iResult != ECommandPerformed )
		BULOG_E( pClient ? pClient->GetEdict() : NULL, "%s", CTypeToString::ConsoleCommandResultToString( iResult ).c_str() );
    return PLUGIN_STOP; // We handled this command.
}

//----------------------------------------------------------------------------------------------------------------
#ifndef BOTRIX_SOURCE_ENGINE_2006
void CBotrixPlugin::OnEdictAllocated( edict_t *edict )
{
    if ( bMapRunning )
        CItems::Allocated(edict);
}

//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::OnEdictFreed( const edict_t *edict  )
{
    if ( bMapRunning )
        CItems::Freed(edict);
}
#endif

//----------------------------------------------------------------------------------------------------------------
// Called when a client is authenticated
//----------------------------------------------------------------------------------------------------------------
PLUGIN_RESULT CBotrixPlugin::NetworkIDValidated( const char *pszUserName, const char *pszNetworkID )
{
    if ( !bMapRunning )
        return PLUGIN_CONTINUE;

    TCommandAccessFlags iAccess = CConfiguration::ClientAccessLevel(pszNetworkID);
    if ( iAccess ) // Founded.
    {
        for ( int i = 0; i < CPlayers::GetPlayersCount(); ++i )
        {
            CPlayer* pPlayer = CPlayers::Get(i);
            if ( pPlayer && !pPlayer->IsBot() )
            {
                CClient* pClient = (CClient*)pPlayer;
                if ( good::string(pszNetworkID) == pClient->GetSteamID() )
                {
                    pClient->iCommandAccessFlags = iAccess;
                    break;
                }
            }
        }
    }

    BLOG_I( "User id validated %s (steam id %s), access: %s.", pszUserName, pszNetworkID,
            CTypeToString::AccessFlagsToString(iAccess).c_str() );

    return PLUGIN_CONTINUE;
}


//----------------------------------------------------------------------------------------------------------------
bool CBotrixPlugin::GenerateSayEvent( edict_t* pEntity, const char* szText, bool bTeamOnly )
{
    int iUserId = pEngineServer->GetPlayerUserId(pEntity);
    BASSERT(iUserId != -1, return false);

    IGameEvent* pEvent = pGameEventManager->CreateEvent("player_say");
    if ( pEvent )
    {
        pEvent->SetInt("userid", iUserId);
        pEvent->SetString("text", szText);
        pEvent->SetBool("teamonly", bTeamOnly);
        //pEvent->SetInt("priority", 1 );	// HLTV event priority, not transmitted
        pGameEventManager->FireEvent(pEvent);
        //pGameEventManager2->FreeEvent(pEvent); // Don't free it !!!
        return true;
    }
    return false;
}


//----------------------------------------------------------------------------------------------------------------
//void CBotrixPlugin::HudTextMessage( edict_t* pEntity, char* /*szTitle*/, char* szMessage, Color colour, int level, int time )
//{
//    KeyValues *kv = new KeyValues( "menu" );
//    kv->SetString( "title", szMessage );
//    //kv->SetString( "msg", szMessage );
//    kv->SetColor( "color", colour);
//    kv->SetInt( "level", level);
//    kv->SetInt( "time", time);
//    //DIALOG_TEXT
//    pServerPluginHelpers->CreateMessage( pEntity, DIALOG_MSG, kv, instance );

//    kv->deleteThis();
//}

//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::Enable( bool bEnable )
{
    GoodAssert( bEnable != m_bEnabled );
    m_bEnabled = bEnable;
    CBotrixCommand::instance.reset();
    CBotrixCommand::instance = new CBotrixCommand();
}


//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::PrepareLevel( const char* szMapName )
{
    // Get map name.
//    const ConVar* pVarMapName = pCVar->FindVar("host_map");
//    const char* szMapName = pVarMapName->GetString();

//    const char* szPoint = strchr(szMapName, '.');
//    good::string::size_type iSize = szPoint ? (szPoint - szMapName) : good::string::npos;

    sMapName.assign(szMapName, good::string::npos, true);
    good::lower_case(sMapName);

#ifndef DONT_USE_VALVE_FUNCTIONS
    ConVar *pTeamplay = pCVar->FindVar("mp_teamplay");
    bTeamPlay = pTeamplay ? pTeamplay->GetBool() : false;

    CWaypoint::iWaypointTexture = CBotrixPlugin::pEngineServer->PrecacheModel( "sprites/lgtning.vmt" );
#endif
}


//----------------------------------------------------------------------------------------------------------------
void CBotrixPlugin::ActivateLevel( int iMaxPlayers )
{
    bMapRunning = true;

//    const ConVar* pVarMaxPlayers = pCVar->FindVar("maxplayers");
//    int iMaxPlayers = pVarMaxPlayers ? pVarMaxPlayers->GetInt() : 64;
//    if ( iMaxPlayers <= 0 )
//        iMaxPlayers = 256;

    CPlayers::Init(iMaxPlayers);
 
    // Waypoints should be loaded after CPlayers::Size() is known.
    if ( CWaypoints::Load() )
        BLOG_I( "%d waypoints loaded for map %s.", CWaypoints::Size(), CBotrixPlugin::instance->sMapName.c_str() );

    // Waypoints can save object, so waypoints should be loaded first.
    CItems::MapLoaded();

    // May depend on items / waypoints.
    CMod::MapLoaded();

    BLOG_I("Level \"%s\" has been loaded.", sMapName.c_str());

    if ( CWaypoints::Size() <= CWaypoint::iWaypointsMaxCountToAnalyzeMap )
        CWaypoints::Analyze( NULL, false );
}
