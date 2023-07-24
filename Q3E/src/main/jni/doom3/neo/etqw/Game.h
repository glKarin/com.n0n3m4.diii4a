// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_H__
#define __GAME_H__

/*
===============================================================================

	Public game interface with methods to run the game.

===============================================================================
*/

#include "../sound/SoundShader.h"
#include "../renderer/Material.h"
#include "../framework/UsercmdGen.h"
#include "../framework/Session.h"
#include "../framework/async/NetworkSystem.h"
#include "../framework/async/AllowFailureReason.h"
#include "../framework/async/AsyncUpdates.h"
#include "../framework/CmdSystem.h"

#include "../game/botai/BotAI_Debug.h"

class idEntity;
class idRenderWorld;
class idSoundWorld;
struct usercmd_s;
class sdUserInterfaceLocal;
struct trace_t;
class idSoundEmitter;
class rvClientMoveable;
class sdKeyCommand;
class idKey;
class sdLogitechLCDSystem;

class usercmd_t;

// RAVEN BEGIN
// bdube: forward reference
class rvClientEffect;
class rvBSEManager;
// RAVEN END

struct clientNetworkAddress_t;
class sdUserInterface;

enum allowReply_t {
	ALLOW_YES = 0,
	ALLOW_BADPASS,	// core will prompt for password and connect again
	ALLOW_NOTYET,	// core will wait with transmitted message
	ALLOW_NO		// core will abort with transmitted message
};

enum userMapChangeResult_e {
	UMCR_ERROR,			// failed to load the desired campaign/map
	UMCR_CONTINUE,		// continue the map loading process
	UMCR_STOP,			// do nothing (useful for pure restarts)
};

class idGame {
public:
	virtual						~idGame() {}

	// Initialize the game for the first time.
	virtual void				Init( void ) = 0;

	// Shut down the entire game.
	virtual void				Shutdown( void ) = 0;

	// Sets the user info for a client.
	// The game can modify the user info in the returned dictionary pointer, server will forward back.
	virtual bool				ValidateUserInfo( int clientNum, idDict& userInfo ) = 0;
	virtual void				UserInfoChanged( int clientNum ) = 0;

	// Sets the serverinfo at map loads and when it changes.
	virtual void				SetServerInfo( const idDict &serverInfo ) = 0;

	// Loads a map and spawns all the entities.
	virtual void				InitFromNewMap( const char *mapName, idRenderWorld *renderWorld, idSoundWorld *soundWorld, bool isServer, bool isClient, int randSeed, int startTime, bool isUserChange ) = 0;

	// Shut down the current map.
	virtual void				MapShutdown( void ) = 0;

	// Caches media referenced from in key/value pairs in the given dictionary.
	virtual void				CacheDictionaryMedia( const idDict& dict ) = 0;

	//
	virtual void				FinishBuild( void ) = 0;

	// Spawns the player entity to be used by the client.
	virtual void				SpawnPlayer( int clientNum, bool isBot ) = 0;

	// Runs a game frame, may return a session command for level changing, etc
	virtual void				RunFrame( const usercmd_t* clientCmds, int elapsedTime ) = 0;

	// Makes rendering and sound system calls to display for a given clientNum.
	virtual bool				Draw() = 0;

	// Makes 2D rendering system calls to display for a given clientNum.
	virtual bool				Draw2D() = 0;

	virtual bool				HandleGuiEvent( const sdSysEvent* event ) = 0;

	// Server is shutting down
	virtual void				OnServerShutdown( void ) = 0;

	// Early check to deny connect.
	virtual allowReply_t		ServerAllowClient( int numClients, int numBots, const clientNetworkAddress_t& address, const sdNetClientId& netClientId, const char *guid, const char *password, allowFailureReason_t& reason ) = 0;
#ifdef SD_SUPPORT_REPEATER
	virtual allowReply_t		RepeaterAllowClient( int numViewers, int maxViewers, const clientNetworkAddress_t& address, const sdNetClientId& netClientId, const char *guid, const char *password, allowFailureReason_t& reason, bool isRepeater ) = 0;
#endif // SD_SUPPORT_REPEATER

	// Connects a client.
	virtual void				ServerClientConnect( int clientNum ) = 0;

	// Spawns the player entity to be used by the client.
	virtual void				ServerClientBegin( int clientNum, bool isBot ) = 0;
#ifdef SD_SUPPORT_REPEATER
	virtual void				RepeaterClientBegin( int clientNum ) = 0;
#endif // SD_SUPPORT_REPEATER

	//
	virtual void				SetClientNum( int clientNum, bool server ) = 0;

	// Disconnects a client and removes the player entity from the game.
	virtual void				ServerClientDisconnect( int clientNum ) = 0;
#ifdef SD_SUPPORT_REPEATER
	virtual void				RepeaterClientDisconnect( int clientNum ) = 0;
#endif // SD_SUPPORT_REPEATER

	// Writes initial reliable messages a client needs to receive when first joining the game.
	virtual void				ServerWriteInitialReliableMessages( int clientNum ) = 0;
#ifdef SD_SUPPORT_REPEATER
	virtual void				RepeaterWriteInitialReliableMessages( int clientNum ) = 0;
#endif // SD_SUPPORT_REPEATER

	// Writes a snapshot of the server game state for the given client.
	virtual void				ServerWriteSnapshot( int clientNum, int sequence, idBitMsg &msg, idBitMsg &ucmdmsg ) = 0;
#ifdef SD_SUPPORT_REPEATER
	virtual void				RepeaterWriteSnapshot( int clientNum, int sequence, idBitMsg &msg, idBitMsg &ucmdmsg, const repeaterUserOrigin_t& userOrigin, bool clientIsRepeater ) = 0;
#endif // SD_SUPPORT_REPEATER

	// Patches the network entity states at the server with a snapshot for the given client.
	virtual bool				ServerApplySnapshot( int clientNum, int sequence ) = 0;
#ifdef SD_SUPPORT_REPEATER
	virtual bool				RepeaterApplySnapshot( int clientNum, int sequence ) = 0;
#endif // SD_SUPPORT_REPEATER

	// Processes a reliable message from a client.
	virtual void				ServerProcessReliableMessage( int clientNum, const idBitMsg &msg ) = 0;
#ifdef SD_SUPPORT_REPEATER
	virtual void				RepeaterProcessReliableMessage( int clientNum, const idBitMsg &msg ) = 0;
#endif // SD_SUPPORT_REPEATER

	// Reads a snapshot and updates the client game state.
	virtual bool				ClientReadSnapshot( int sequence, const int gameFrame, const int gameTime, const int numDuplicatedUsercmds, const int aheadOfServer, const idBitMsg &msg, const idBitMsg &ucmdmsg ) = 0;

	// Patches the network entity states at the client with a snapshot.
	virtual bool				ClientApplySnapshot( int sequence ) = 0;

	// Processes a reliable message from the server.
	virtual void				ClientProcessReliableMessage( const idBitMsg& msg ) = 0;

	// Runs prediction on entities at the client.
	virtual void				ClientPrediction( const usercmd_t *clientCmds, const usercmd_t* demoCmd ) = 0;

	virtual void				OnSnapshotHitch( int snapshotTime ) = 0;

	virtual void				OnClientDisconnected( void ) = 0;

	// Update the view to prepare for an extra draw (com_unlockFPS)
	virtual void				ClientUpdateView( const usercmd_t &cmd, int timeLeft ) = 0;

	// Writes current network info to a file (used as initial state for demo playback).
	virtual void				WriteClientNetworkInfo( idFile* file ) = 0;

	// Reads current network info from a file (used as initial state for demo playback).
	virtual void				ReadClientNetworkInfo( idFile* file ) = 0;

	virtual void				WriteUserInfo( idBitMsg& msg, const idDict& info ) = 0;
	virtual void				ReadUserInfo( const idBitMsg& msg, idDict& info ) = 0;

	virtual void				PacifierUpdate() = 0;

	virtual void				UpdateLevelLoadScreen( const wchar_t* status ) = 0;

	virtual bool				KeyMove( char forward, char right, char up, usercmd_t& cmd ) = 0;
	virtual void				ControllerMove( bool doGameCallback, const int numControllers, const int* controllerNumbers,
													const float** controllerAxis, idVec3& viewAngles, usercmd_t& cmd ) = 0;
	virtual void				MouseMove( const idVec3& angleBase, idVec3& angleDelta ) = 0;

	virtual bool				GetSensitivity( float& scaleX, float& scaleY ) = 0;

	virtual void				UsercommandCallback( usercmd_t& cmd ) = 0;

	virtual void				ShowLevelLoadScreen( const char* mapName ) = 0;
	virtual void				HideLevelLoadScreen( void ) = 0;

	virtual	void				BeginLevelLoad() = 0;
	virtual	void				EndLevelLoad() = 0;

	virtual void				ShowMainMenu() = 0;
	virtual void				HideMainMenu() = 0;
    virtual bool				IsMainMenuActive() = 0;
	virtual void				DrawMainMenu() = 0;
	virtual void				DrawLoadScreen() = 0;
	virtual void				DrawPureWaitScreen() = 0;
	virtual void				DrawSystemUI() = 0;

	virtual void				GuiFrameEvents( bool outOfSequence ) = 0;

	virtual class sdUserInterfaceManager* GetUIManager() = 0;

	virtual void					MessageBox( msgBoxType_t type, const wchar_t* message, const sdDeclLocStr* title ) = 0;
	virtual void					CloseMessageBox() = 0;
	virtual void					SetUpdateAvailability( updateAvailability_t type ) = 0;
	virtual guiUpdateResponse_t		GetUpdateResponse() = 0;
	virtual void					SetUpdateState( updateState_t state ) = 0;
	virtual void					SetUpdateProgress( float percent ) = 0;
	virtual void					SetUpdateFromServer( bool fromServer ) = 0;
	virtual void					AddSystemNotification( const wchar_t* text ) = 0;
	virtual void					SetUpdateMessage( const wchar_t* text ) = 0;

	virtual bool				ClientsOnSameTeam( int clientNum1, int clientNum2, voiceMode_t mode ) = 0;
	virtual bool				AllowClientAudio( int clientNum1, voiceMode_t mode ) = 0;

	// RAVEN BEGIN
	// jscott: for the effects system
	virtual void				StartViewEffect( int type, float time, float scale ) = 0;
	virtual rvClientEffect*		PlayEffect( const int effectHandle, const idVec3& color, const idVec3& origin, const idMat3& axis, bool loop = false, const idVec3& endOrigin = vec3_origin, float distanceOffset = 0.0f ) = 0;
	virtual void				GetPlayerView( idVec3& origin, idMat3& axis, float& fovx ) = 0;
	virtual void				Translation( trace_t& trace, const idVec3 &source, const idVec3 &dest, const idTraceModel &trm, int clipMask ) = 0;
	virtual void				TracePoint( trace_t& trace, const idVec3 &source, const idVec3 &dest, int clipMask ) = 0;
	virtual rvClientMoveable*	SpawnClientMoveable( const char* name, int lifetime, const idVec3& origin, const idMat3& axis, const idVec3& velocity, const idVec3& angular_velocity, int effectSet = 0 ) = 0;
	// RAVEN END

	virtual userMapChangeResult_e OnUserStartMap( const char* text, idStr& reason, idStr& mapName ) = 0;
	virtual void				ArgCompletion_StartGame( const idCmdArgs& args, argCompletionCallback_t callback ) = 0;

	virtual void				RunFrame() = 0;

	virtual void				CreateStatusResponseDict( const idDict& serverInfo, idDict& statusResponseDict ) = 0;
	virtual int					GetProbeTime() const = 0;
	virtual byte				GetProbeState() const = 0;
	virtual void				WriteExtendedProbeData( idBitMsg& msg ) = 0;

#ifdef ID_DEBUG_MEMORY
	virtual void				MemDump( const char* fileName ) = 0;
	virtual void				MemDumpCompressed( const char *fileName, memoryGroupType_t memGroup, memorySortType_t memSort, int sortCallStack, int numFrames, bool xlFriendly ) = 0;
	virtual void				MemDumpPerClass( const char* fileName ) = 0;
#endif

	virtual void				OnInputInit( void ) = 0;
	virtual void				OnInputShutdown( void ) = 0;

	virtual void				OnLanguageInit( void ) = 0;
	virtual void				OnLanguageShutdown( void ) = 0;

	virtual sdKeyCommand*		Translate( const idKey& key ) = 0;
	virtual usercmdbuttonType_t	SetupBinding( const char* binding, int& action ) = 0;
	virtual void				HandleLocalImpulse( int action, bool down ) = 0;

	virtual int					GetBotFPS( void ) const = 0;
	virtual botDebugInfo_t		GetBotDebugInfo( int clientNum ) = 0;
	virtual bool				GetRandomBotName( int clientNum, idStr& botName ) = 0;

	virtual void				DrawLCD( sdLogitechLCDSystem* lcd ) = 0;

	virtual void				AddChatLine( const wchar_t* text ) = 0;

	virtual bool				DownloadRequest( const char *IP, const char *guid, const char *paks, char urls[ MAX_STRING_CHARS ] ) = 0;

	// return true to allow download from the built-in http server
	virtual bool				HTTPRequest( const char *IP, const char *file, bool isGamePak ) = 0;

#ifdef SD_SUPPORT_REPEATER
	// Set the repeater state; engine will call this with true for isRepeater if this is a repeater
	virtual void				SetRepeaterState( bool isRepeater ) = 0;
#endif // SD_SUPPORT_REPEATER
};

extern idGame *					game;


/*
===============================================================================

	Public game interface with methods for in-game editing.

===============================================================================
*/

class idSoundShader;

typedef struct {
	idSoundEmitter *			referenceSound;	// this is the interface to the sound system, created
												// with idSoundWorld::AllocSoundEmitter() when needed
	idVec3						origin;
	int							listenerId;		// SSF_PRIVATE_SOUND only plays if == listenerId from PlaceListener
												// no spatialization will be performed if == listenerID
	const idSoundShader *		shader;			// this really shouldn't be here, it is a holdover from single channel behavior
	float						diversity;		// 0.0 to 1.0 value used to select which
												// samples in a multi-sample list from the shader are used
	bool						waitfortrigger;	// don't start it at spawn time
	soundShaderParms_t			parms;			// override volume, flags, etc
} refSound_t;

enum {
	TEST_PARTICLE_MODEL = 0,
	TEST_PARTICLE_IMPACT,
	TEST_PARTICLE_MUZZLE,
	TEST_PARTICLE_FLIGHT,
	TEST_PARTICLE_SELECTED
};

class idEntity;
class idMD5Anim;
// RAVEN BEGIN
// bdube: more forward declarations
class idProgram;
class idInterpreter;

struct renderEntity_t;
struct renderLight_t;

class idRenderModel;

class sdUserInterfaceScope;
struct guiScope_t {
	guiScope_t( const char* name_ = "", sdUserInterfaceScope* scope_ = NULL ) : name( name_ ), scope( scope_ ) {}
	const char*				name;
	sdUserInterfaceScope*	scope;
};	


// RAVEN END

// FIXME: this interface needs to be reworked but it properly separates code for the time being
class idGameEdit {
public:
	virtual						~idGameEdit( void ) {}

	// These are the canonical idDict to parameter parsing routines used by both the game and tools.
	virtual void				ParseSpawnArgsToRenderLight( const idDict& args, renderLight_t& renderLight );
	virtual void				ParseSpawnArgsToRenderEntity( const idDict& args, renderEntity_t& renderEntity );
	virtual void				ParseSpawnArgsToRefSound( const idDict& args, refSound_t& refSound );

	// This refreshes all the flags and GUIs and stuff
	virtual void				RefreshRenderEntity( const idDict& args, renderEntity_t& renderEntity );

	// This cleans up a render entity
	virtual void				DestroyRenderEntity( renderEntity_t& renderEntity );

	// Animation system calls for non-game based skeletal rendering.
	virtual idRenderModel *		ANIM_GetModelFromEntityDef( const char *classname );
	virtual const idVec3 		&ANIM_GetModelOffsetFromEntityDef( const idDict *args );
	virtual idRenderModel *		ANIM_GetModelFromEntityDef( const idDict *args );
	virtual idRenderModel *		ANIM_GetModelFromName( const char *modelName );
	virtual const idMD5Anim *	ANIM_GetAnimFromEntityDef( const idDict *args );
	virtual int					ANIM_GetNumAnimsFromEntityDef( const idDict *args );
	virtual const char *		ANIM_GetAnimNameFromEntityDef( const idDict *args, int animNum );
	virtual const idMD5Anim *	ANIM_GetAnim( const char *fileName );
	virtual int					ANIM_GetLength( const idMD5Anim *anim );
	virtual int					ANIM_GetNumFrames( const idMD5Anim *anim );
	virtual void				ANIM_CreateAnimFrame( const idRenderModel *model, const idMD5Anim *anim, int numJoints, idJointMat *frame, int time, const idVec3 &offset, bool remove_origin_offset );
	virtual idRenderModel *		ANIM_CreateMeshForAnim( idRenderModel *model, const idDict& args, bool remove_origin_offset );
	virtual void				ANIM_GetFrameBounds( const idMD5Anim *anim, idBounds& bounds, int frame, int cyclecount );

	// Articulated Figure calls for AF editor and Radiant.
	virtual bool				AF_SpawnEntity( const char *fileName );
	virtual void				AF_UpdateEntities( const char *fileName );
	virtual void				AF_UndoChanges( void );
	virtual idRenderModel *		AF_CreateMesh( const idDict &args, idVec3 &meshOrigin, idMat3 &meshAxis, bool &poseIsSet );

	// Entity selection.
	virtual void				ClearEntitySelection( void );
	virtual int					GetSelectedEntities( idEntity *list[], int max );
	virtual int					GetSelectedEntitiesByName( idStr *list[], int max );
	virtual void				AddSelectedEntity( idEntity *ent );
	virtual void				RemoveSelectedEntity( idEntity *ent );

	// Selection methods
	virtual void				TriggerSelected();

	// Entity defs and spawning.
	virtual const idDict *		FindEntityDefDict( const char *name, bool makeDefault = true ) const;
	virtual void				SpawnEntityDef( const idDict &args, idEntity **ent );
	virtual idEntity *			FindEntity( const char *name ) const;
	virtual const char *		GetUniqueEntityName( const char *classname ) const;

	// Entity methods.
	virtual void				EntityGetOrigin( idEntity *ent, idVec3 &org ) const;
	virtual void				EntityGetAxis( idEntity *ent, idMat3 &axis ) const;
	virtual void				EntitySetOrigin( idEntity *ent, const idVec3 &org );
	virtual void				EntitySetAxis( idEntity *ent, const idMat3 &axis );
	virtual void				EntityTranslate( idEntity *ent, const idVec3 &org );
	virtual const idDict *		EntityGetSpawnArgs( idEntity *ent ) const;
	virtual void				EntityUpdateChangeableSpawnArgs( idEntity *ent, const idDict *dict );
	virtual void				EntityChangeSpawnArgs( idEntity *ent, const idDict *newArgs );
	virtual void				EntityUpdateVisuals( idEntity *ent );
	virtual void				EntitySetModel( idEntity *ent, const char *val );
	virtual void				EntityStopSound( idEntity *ent );
	virtual void				EntityDelete( idEntity *ent );
	virtual void				EntitySetColor( idEntity *ent, const idVec3 color );

	virtual int					EntityToSafeId( idEntity* ent ) const;
	virtual idEntity *			EntityFromSafeId( int safeID ) const;
	virtual idEntity *			EntityFromIndex( int index ) const;

	// Player methods.
	virtual bool				PlayerIsValid() const;
	virtual void				PlayerGetOrigin( idVec3 &org ) const;
	virtual void				PlayerGetAxis( idMat3 &axis ) const;
	virtual void				PlayerGetViewAngles( idAngles &angles ) const;
	virtual void				PlayerGetEyePosition( idVec3 &org ) const;

// RAVEN BEGIN
// bdube: new game edit stuff
	virtual bool				PlayerTraceFromEye ( trace_t &results, float length, int contentMask );

	// Effect methods
	virtual void				EffectRefreshTemplate ( int effectTemplate ) const;

	// Flare methods
	virtual void				FlareUpdate( const idDict& args, renderEntity_t& renderEntity, const struct renderView_s* renderView );

	// Common editing functions
	virtual int					GetGameTime ( int *previous = NULL ) const;
	virtual void				SetGameTime	( int time ) const;
	virtual bool				TracePoint ( trace_t& results, const idVec3 &start, const idVec3 &end, int contentMask ) const;

	// In game map editing support.
	virtual const idDict *		MapGetEntityDict( const char *name ) const;
	virtual void				MapSave( const char *path = NULL ) const;
	virtual void				MapSaveClass( const char *path, const char* classname ) const;
	virtual void				MapSetEntityKeyVal( const char *name, const char *key, const char *val ) const ;
	virtual void				MapCopyDictToEntity( const char *name, const idDict *dict ) const;
	virtual int					MapGetUniqueMatchingKeyVals( const char *key, const char *list[], const int max ) const;
	virtual void				MapAddEntity( const idDict *dict ) const;
	virtual int					MapGetEntitiesMatchingClassWithString( const char *classname, const char *list[], const int max, const char* matchKey = NULL, const char *matchValue = NULL ) const;
	virtual void				MapRemoveEntity( const char *name ) const;
	virtual void				MapEntityTranslate( const char *name, const idVec3 &v ) const;

	virtual idRenderWorld*		GetRenderWorld() const;
	virtual const char*			MapGetName() const;

	virtual void				KillClass( const char* classname );

	// UI info
	virtual int					NumUserInterfaceScopes() const;
	virtual const guiScope_t&	GetScope( int index );
};

extern idGameEdit *				gameEdit;


/*
===============================================================================

	Game API.

===============================================================================
*/

#ifdef SD_PUBLIC_TOOLS
	const int GAME_API_VERSION		= 1008;
#else
	const int GAME_API_VERSION		= 8;
#endif


class idNetworkSystem;
class idRenderSystem;
class sdDeviceContext;
class idSoundSystem;
class idRenderModelManager;
class idDeclManager;
//class idAASFileManager;
class idCollisionModelManager;
class idCmdSystem;
class sdNetService;
class sdAdManager;
class sdKeyInputManager;
class idAASFileManager;
class sdNotificationSystem;
class sdGraphManager;

#ifdef _XENON
class LiveService;
#endif

struct gameImport_t {
	int							version;				// API version
	idSys*						sys;					// non-portable system services
	idCommon*					common;					// common
	idCmdSystem*				cmdSystem;				// console command system
	idCVarSystem*				cvarSystem;				// console variable system
	idFileSystem*				fileSystem;				// file system
	idNetworkSystem*			networkSystem;			// network system
	idRenderSystem*				renderSystem;			// render system
	sdDeviceContext*			deviceContext;			// render system
	idSoundSystem*				soundSystem;			// sound system
	idRenderModelManager*		renderModelManager;		// render model manager
	idDeclManager*				declManager;			// declaration manager
	idCollisionModelManager*	collisionModelManager;	// collision model manager
	idAASFileManager*			AASFileManager;
	rvBSEManager*				bse;					// effects system
#ifndef _XENON
	sdNetService*				networkService;			// online services
#endif
	sdAdManager*				adManager;
	sdKeyInputManager*			keyInputManager;
	sdNotificationSystem*		notificationSystem;
	idStrPool*					globalKeys;
	idStrPool*					globalValues;
	stringDataAllocator_t*		stringAllocator;
	wideStringDataAllocator_t*	wideStringAllocator;
	sdGraphManager*				graphManager;

};

struct gameExport_t{
	int							version;				// API version
	idGame *					game;					// interface to run the game
	idGameEdit *				gameEdit;				// interface for in-game editing
};

extern "C" {
typedef gameExport_t * (*GetGameAPI_t)( gameImport_t *import );
}

#endif /* !__GAME_H__ */
