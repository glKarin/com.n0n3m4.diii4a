/**
* @mainpage Botrix plugin.
* @author Botrix ( botrix.plugin\@gmail.com )
*
* @version 1.0.1
*
* <h2>Bots plugin for Valve Games made with Source SDK.</h2>
*
* Originated from botman's bot template (aka HPB_bot).
*/

// TODO: Smart bots will keep distance according to the weapon they have.

#ifndef __BOTRIX_SERVER_PLUGIN_H__
#define __BOTRIX_SERVER_PLUGIN_H__


#include <good/string.h>

#include "iserverplugin.h"
#include "igameevents.h"


#define PLUGIN_VERSION "1.0.1-AnniversaryUpdate"

class IVEngineServer;
class IFileSystem;
class IGameEventManager2;
class IPlayerInfoManager;
class IVEngineServer;
class IServerPluginHelpers;
class IServerGameClients;
class IEngineTrace;
class IEffects;
class IBotManager;
class ICvar;


//****************************************************************************************************************
/// Plugin class.
//****************************************************************************************************************
class CBotrixPlugin : public IServerPluginCallbacks
{

public: // Static members.
    static CBotrixPlugin* instance;                    ///< Plugin singleton.

    static IVEngineServer* pEngineServer;              ///< Interface to util engine functions.
    static IGameEventManager2* pGameEventManager;      ///< Game events interface.
    static IPlayerInfoManager* pPlayerInfoManager;     ///< Interface to interact with players.
    static IServerPluginHelpers* pServerPluginHelpers; ///< To create menues on client side.
    static IServerGameClients* pServerGameClients;
    static IEngineTrace* pEngineTrace;
    static IBotManager* pBotManager;
    static ICvar* pCVar;


public: // Methods.
    //------------------------------------------------------------------------------------------------------------
    /// Constructor.
    //------------------------------------------------------------------------------------------------------------
    CBotrixPlugin();

    /// Destructor.
    ~CBotrixPlugin();

    //------------------------------------------------------------------------------------------------------------
    // IServerPluginCallbacks implementation.
    //------------------------------------------------------------------------------------------------------------
    /// Initialize the plugin to run
    /// Return false if there is an error during startup.
    virtual bool Load(	CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory  );

    /// Called when the plugin should be shutdown
    virtual void Unload( void );

    /// called when a plugins execution is stopped but the plugin is not unloaded
    virtual void Pause( void );

    /// called when a plugin should start executing again (sometime after a Pause() call)
    virtual void UnPause( void );

    /// Returns string describing current plugin.  e.g., Admin-Mod.
    virtual const char     *GetPluginDescription( void );

    /// Called any time a new level is started (after GameInit() also on level transitions within a game)
    virtual void LevelInit( char const *pMapName );

    /// The server is about to activate
    virtual void ServerActivate( edict_t *pEdictList, int edictCount, int clientMax );

    /// The server should run physics/think on all edicts
    virtual void GameFrame( bool simulating );

    /// Called when a level is shutdown (including changing levels)
    virtual void LevelShutdown( void );

    /// Client is going active
    virtual void ClientActive( edict_t *pEntity );

    /// Client is disconnecting from server
    virtual void ClientDisconnect( edict_t *pEntity );

    /// Client is connected and should be put in the game
    virtual void ClientPutInServer( edict_t *pEntity, char const *playername );

    /// Sets the client index for the client who typed the command into their console
    virtual void SetCommandClient( int index );

    /// A player changed one/several replicated cvars (name etc)
    virtual void ClientSettingsChanged( edict_t *pEdict );

    /// Client is connecting to server ( set retVal to false to reject the connection )
    ///	You can specify a rejection message by writing it into reject
    virtual PLUGIN_RESULT ClientConnect( bool *bAllowConnect, edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );

    /// A user has had their network id setup and validated
    virtual PLUGIN_RESULT NetworkIDValidated( const char *pszUserName, const char *pszNetworkID );

#ifdef BOTRIX_SOURCE_ENGINE_2006
    // The client has typed a command at the console
    virtual PLUGIN_RESULT ClientCommand( edict_t *pEntity );
#else
    /// The client has typed a command at the console
    virtual PLUGIN_RESULT ClientCommand( edict_t *pEntity, const CCommand &args );

    /// This is called when a query from IServerPluginHelpers::StartQueryCvarValue is finished.
    /// iCookie is the value returned by IServerPluginHelpers::StartQueryCvarValue.
    /// Added with version 2 of the interface.
    virtual void OnQueryCvarValueFinished( QueryCvarCookie_t, edict_t*, EQueryCvarValueStatus, const char*, const char* ) {}

    /// added with version 3 of the interface.
    virtual void OnEdictAllocated( edict_t *edict );
    virtual void OnEdictFreed( const edict_t *edict  );
#endif

    // Generate say event.
    bool GenerateSayEvent( edict_t* pEntity, const char* szText, bool bTeamOnly );

public: // Static methods.
//    static void HudTextMessage( edict_t* pEntity, char *szTitle, char *szMessage,
//                                Color colour, int level, int time );

    /// Return true if plugin is enabled.
    bool IsEnabled() const { return m_bEnabled; }

    /// Enable/disable plugin.
    void Enable( bool bEnable );

public: // Members.
    good::string sGameFolder;             ///< Game folder, i.e. "counter-strike source"
    good::string sModFolder;              ///< Mod folder, i.e. "cstrike"
    good::string sBotrixPath;             ///< Full path to Botrix folder, where config.ini and waypoints are.

    bool bTeamPlay;                       ///< True if game is team based (like Counter-Strike), if false then it is deathmatch.

    bool bMapRunning;                     ///< True if map is currently running (LevelInit() was called by Source engine).
    good::string sMapName;                ///< Current map name (set at LevelInit()).

    static int iFPS;                      ///< Frames per second. Updated each second.
    static float fTime;                   ///< Current time.
    static float fEngineTime;             ///< Current engine time.

protected:

    void PrepareLevel( const char* szMapName );
    void ActivateLevel( int iMaxPlayers );

    bool m_bEnabled;                      ///< True if this plugin is enabled.
    bool m_bPaused;                       ///< True if this plugin is paused.
    static float m_fFpsEnd;               ///< Time of ending counting frames to calculate fps.
    static int m_iFramesCount;            ///< Count of frames since m_fFpsStart.

};

#endif // __BOTRIX_SERVER_PLUGIN_H__
