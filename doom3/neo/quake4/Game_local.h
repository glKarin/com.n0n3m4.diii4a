#ifndef __GAME_LOCAL_H__
#define	__GAME_LOCAL_H__

// RAVEN BEGIN
// jsinger: attempt to eliminate cross-DLL allocation issues
#ifdef RV_UNIFIED_ALLOCATOR
inline void *operator new( size_t s ) { return Memory::Allocate(s); }
inline void operator delete( void *p ) { Memory::Free(p); }
inline void *operator new[]( size_t s ) { return Memory::Allocate(s); }
inline void operator delete[]( void *p ) { Memory::Free(p); }
#endif
// RAVEN END

/*
===============================================================================

	Local implementation of the public game interface.

===============================================================================
*/

#define LAGO_IMG_WIDTH 64
#define LAGO_IMG_HEIGHT 64
#define LAGO_WIDTH	64
#define LAGO_HEIGHT	44
#define LAGO_MATERIAL	"textures/mptextures/lagometer"
#define LAGO_IMAGE		"textures/mptextures/lagometer.tga"

// if set to 1 the server sends the client PVS with snapshots and the client compares against what it sees
#ifndef ASYNC_WRITE_PVS
	#define ASYNC_WRITE_PVS 0
#endif

extern idRenderWorld *				gameRenderWorld;

//k
#include "../raven/sys/AutoVersion.h"
// the "gameversion" client command will print this plus compile date
#define	GAME_VERSION		"baseQUAKE4-1"

// classes used by idGameLocal
class idEntity;
class idActor;
class idPlayer;
class idCamera;
class idWorldspawn;
class idTestModel;
class idAAS;
class idAI;
// RAVEN BEGIN
// bdube: not using id effects
//class idSmokeParticles;
//class idEntityFx;
// bdube: client side entities
class rvInstance;
class rvClientEntity;
class rvClientModel;
class rvCTFAssaultPlayerStart;
class idPlayerStart;
// RAVEN END
class idTypeInfo;
class idProgram;
class idThread;
class idEditEntities;
class idLocationEntity;

#ifdef _QUAKE4
// jmarshall
class rvmBot;
// jmarshall end
#endif

// RAVEN BEGIN
// dluetscher: reduced max clients for memory usage
#ifdef _XENON
#define	MAX_CLIENTS				16
#else
// RAVEN END
#define	MAX_CLIENTS				32
#endif

#define	GENTITYNUM_BITS			12
#define	MAX_GENTITIES			(1<<GENTITYNUM_BITS)
#define	ENTITYNUM_NONE			(MAX_GENTITIES-1)
#define	ENTITYNUM_WORLD			(MAX_GENTITIES-2)
// RAVEN BEGIN
// bdube: dummy entity for client side physics
#define ENTITYNUM_CLIENT		(MAX_GENTITIES-3)
#define	ENTITYNUM_MAX_NORMAL	(MAX_GENTITIES-3)
// RAVEN END

// RAVEN BEGIN
// bdube: client entities
#define	CENTITYNUM_BITS			12
#define	MAX_CENTITIES			(1<<CENTITYNUM_BITS)
// shouchard:  for ban lists and because I hate magic numbers
#define CLIENT_GUID_LENGTH		12
// RAVEN END

// RAVEN BEGIN
// abahr: helper macros
#define SAFE_DELETE_PTR(p) if(p) { delete (p); (p) = NULL; }
#define SAFE_REMOVE(p) if(p) { (p)->PostEventMS(&EV_Remove, 0); (p) = NULL; }
// RAVEN END

//============================================================================

void gameError( const char *fmt, ... );

#include "gamesys/Event.h"
// RAVEN BEGIN
// bdube: added
#include "gamesys/State.h"
// RAVEN END
#include "gamesys/Class.h"
#include "gamesys/SysCvar.h"
#include "gamesys/SysCmds.h"
#include "gamesys/SaveGame.h"
#include "gamesys/DebugGraph.h"

#include "script/Script_Program.h"

#include "anim/Anim.h"

#include "ai/AAS.h"

#include "physics/Clip.h"
#include "physics/Push.h"

#include "Pvs.h"

#include "FreeView.h"

//============================================================================

const int MAX_GAME_MESSAGE_SIZE		= 8192;
const int MAX_ENTITY_STATE_SIZE		= 512;
const int ENTITY_PVS_SIZE			= ((MAX_GENTITIES+31)>>5);
// RAVEN BEGIN
// abahr: changed to NUM_PORTAL_ATTRIBUTES to take into account gravity
const int NUM_RENDER_PORTAL_BITS	= NUM_PORTAL_ATTRIBUTES;
// RAVEN END

typedef struct entityState_s {
	int						entityNumber;
	idBitMsg				state;
	byte					stateBuf[MAX_ENTITY_STATE_SIZE];
	struct entityState_s *	next;
} entityState_t;

typedef struct snapshot_s {
	int						sequence;
	entityState_t *			firstEntityState;
	int						pvs[ENTITY_PVS_SIZE];
	struct snapshot_s *		next;
} snapshot_t;

const int MAX_EVENT_PARAM_SIZE		= 128;

typedef struct entityNetEvent_s {
	int						spawnId;
	int						event;
	int						time;
	int						paramsSize;
	byte					paramsBuf[MAX_EVENT_PARAM_SIZE];
	struct entityNetEvent_s	*next;
	struct entityNetEvent_s *prev;
} entityNetEvent_t;

enum {
	GAME_RELIABLE_MESSAGE_SPAWN_PLAYER,
	GAME_RELIABLE_MESSAGE_DELETE_ENT,
	GAME_RELIABLE_MESSAGE_CHAT,
	GAME_RELIABLE_MESSAGE_TCHAT,
	GAME_RELIABLE_MESSAGE_DB,
	GAME_RELIABLE_MESSAGE_KILL,
	GAME_RELIABLE_MESSAGE_DROPWEAPON,
	GAME_RELIABLE_MESSAGE_RESTART,
	GAME_RELIABLE_MESSAGE_SERVERINFO,
	GAME_RELIABLE_MESSAGE_CALLVOTE,
	GAME_RELIABLE_MESSAGE_CASTVOTE,
	GAME_RELIABLE_MESSAGE_STARTVOTE,
	GAME_RELIABLE_MESSAGE_UPDATEVOTE,
	GAME_RELIABLE_MESSAGE_PORTALSTATES,
	GAME_RELIABLE_MESSAGE_PORTAL,
	GAME_RELIABLE_MESSAGE_VCHAT,
	GAME_RELIABLE_MESSAGE_STARTSTATE,
	GAME_RELIABLE_MESSAGE_MENU,
	GAME_RELIABLE_MESSAGE_EVENT,
// RAVEN BEGIN
// bdube: effect
	GAME_RELIABLE_MESSAGE_ITEMACQUIRESOUND,
// ddynerman: death messages
	GAME_RELIABLE_MESSAGE_DEATH,
// ddynerman: game state
	GAME_RELIABLE_MESSAGE_GAMESTATE,
// ddynerman: game stat
	GAME_RELIABLE_MESSAGE_STAT,
// asalmon: game stats for xenon
	GAME_RELIABLE_MESSAGE_ALL_STATS,
// ddynerman: ingame awards
	GAME_RELIABLE_MESSAGE_INGAMEAWARD,
// ddynerman: instances
	GAME_RELIABLE_MESSAGE_SET_INSTANCE,
// shouchard:  for voicechat
	GAME_RELIABLE_MESSAGE_VOICECHAT_MUTING,
// shouchard:  for server admin
	GAME_RELIABLE_MESSAGE_SERVER_ADMIN,
// shouchard:  for voting
	GAME_RELIABLE_MESSAGE_CALLPACKEDVOTE,
	GAME_RELIABLE_MESSAGE_STARTPACKEDVOTE,
// mekberg: get ban list for server
	GAME_RELIABLE_MESSAGE_GETADMINBANLIST,
	GAME_RELIABLE_MESSAGE_PRINT
// jscott: for voice comms
// TTimo: implemented or not by the OS, the network protocol should not be affected
	,
	GAME_RELIABLE_MESSAGE_VOICEDATA_CLIENT,
	GAME_RELIABLE_MESSAGE_VOICEDATA_CLIENT_ECHO,
	GAME_RELIABLE_MESSAGE_VOICEDATA_CLIENT_TEST,
	GAME_RELIABLE_MESSAGE_VOICEDATA_CLIENT_ECHO_TEST
// RAVEN END	
};

enum {
	GAME_UNRELIABLE_MESSAGE_EVENT,
	GAME_UNRELIABLE_MESSAGE_EFFECT,
	GAME_UNRELIABLE_MESSAGE_HITSCAN,
	GAME_UNRELIABLE_MESSAGE_VOICEDATA_SERVER
};

enum {
	GAME_UNRELIABLE_RECORD_CLIENTNUM,
	GAME_UNRELIABLE_RECORD_AREAS,

	GAME_UNRELIABLE_RECORD_COUNT
};

typedef enum {
	GAMESTATE_UNINITIALIZED,		// prior to Init being called
	GAMESTATE_NOMAP,				// no map loaded
	GAMESTATE_STARTUP,				// inside InitFromNewMap().  spawning map entities.
	GAMESTATE_RESTART,				// spawning map entities from an instance restart, but not fully restarting
	GAMESTATE_ACTIVE,				// normal gameplay
	GAMESTATE_SHUTDOWN				// inside MapShutdown().  clearing memory.
} gameState_t;

typedef struct {
	idPlayerStart	*ent;
	int				dist;
} spawnSpot_t;

//============================================================================
class idEventQueue {
public:
	typedef enum {
		OUTOFORDER_IGNORE,
		OUTOFORDER_DROP,
		OUTOFORDER_SORT
	} outOfOrderBehaviour_t;

							idEventQueue() : start( NULL ), end( NULL ) {}

	entityNetEvent_t *		Alloc();
	void					Free( entityNetEvent_t *event );
	void					Shutdown();

	void					Init();
	void					Enqueue( entityNetEvent_t* event, outOfOrderBehaviour_t oooBehaviour );
	entityNetEvent_t *		Dequeue( void );
	entityNetEvent_t *		RemoveLast( void );

	entityNetEvent_t *		Start( void ) { return start; }

private:
	entityNetEvent_t *					start;
	entityNetEvent_t *					end;
// RAVEN BEGIN
// jnewquist: Mark memory tags for idBlockAlloc
	idBlockAlloc<entityNetEvent_t,32/*,MA_EVENT*/>	eventAllocator; //k
// RAVEN END
};

//============================================================================

template< class type >
class idEntityPtr {
public:
							idEntityPtr();

	// save games
	void					Save( idSaveGame *savefile ) const;					// archives object for save game file
	void					Restore( idRestoreGame *savefile );					// unarchives object from save game file

	idEntityPtr<type> &		operator=( type *ent );

	// synchronize entity pointers over the network
	int						GetSpawnId( void ) const { return spawnId; }
	bool					SetSpawnId( int id );
	bool					UpdateSpawnId( void );

	bool					IsValid( void ) const;
	type *					GetEntity( void ) const;
	int						GetEntityNum( void ) const;

// RAVEN BEGIN
// bdube: overloaded operators
							idEntityPtr( type* ent ) { *this = ent; }
	idEntityPtr<type>&		operator=( idEntityPtr<type>& ent ) { *this = ent.GetEntity(); return *this; }
	type *					operator->( void ) const;				
	operator				type *( void ) const;
// RAVEN END

private:
	int						spawnId;
};

// RAVEN BEGIN
// abahr: forward declaration
class rvGravityArea;
// RAVEN END

//============================================================================
// ddynerman: moved MultiplayerGame.h down here, so it can use more stuff in Game_local (idEntityPtr)
#include "MultiplayerGame.h"

//============================================================================

class idGameLocal : public idGame {
public:
	idDict					serverInfo;				// all the tunable parameters, like numclients, etc
	int						numClients;				// pulled from serverInfo and verified
	idDict					userInfo[MAX_CLIENTS];	// client specific settings
	const usercmd_t			*usercmds;				// client input commands
	idDict					persistentPlayerInfo[MAX_CLIENTS];
	idEntity *				entities[MAX_GENTITIES];// index to entities
	int						spawnIds[MAX_GENTITIES];// for use in idEntityPtr
	int						firstFreeIndex;			// first free index in the entities array
	int						minSpawnIndex;			// when spawning multiple instances, so nothing pollutes in between the instances
	int						num_entities;			// current number <= MAX_GENTITIES
	idHashIndex				entityHash;				// hash table to quickly find entities by name
	idWorldspawn *			world;					// world entity
	idLinkList<idEntity>	spawnedEntities;		// all spawned entities
	idLinkList<idEntity>	activeEntities;			// all thinking entities (idEntity::thinkFlags != 0)
	int						numEntitiesToDeactivate;// number of entities that became inactive in current frame
	bool					sortPushers;			// true if active lists needs to be reordered to place pushers at the front
	bool					sortTeamMasters;		// true if active lists needs to be reordered to place physics team masters before their slaves
	idDict					persistentLevelInfo;	// contains args that are kept around between levels

// RAVEN BEGIN
// bdube: client entities
	rvClientEntity *			clientEntities[MAX_CENTITIES];	// index to client entities
	int							clientSpawnIds[MAX_CENTITIES];	// for use in idClientEntityPtr
	idLinkList<rvClientEntity>	clientSpawnedEntities;			// all client side entities
	int							num_clientEntities;				// current number of client entities
	int							firstFreeClientIndex;			// first free index in the client entities array
	
	int							entityRegisterTime;
// RAVEN END

	// can be used to automatically effect every material in the world that references globalParms
	float					globalShaderParms[ MAX_GLOBAL_SHADER_PARMS ];	

	idRandom				random;					// random number generator used throughout the game

	idProgram				program;				// currently loaded script and data space
	idThread *				frameCommandThread;

	idPush					push;					// geometric pushing
	idPVS					pvs;					// potential visible set
	pvsHandle_t				clientsPVS[MAX_CLIENTS];// PVS of multiplayer clients updated every frame

	idTestModel *			testmodel;				// for development testing of models
// RAVEN BEGIN
// bdube: not using id effects
//	idEntityFx *			testFx;					// for development testing of fx
// RAVEN END

	// only set when an end level is activated, which will take over camera positioning
	// and draw end-level guis, then 

	idStr					sessionCommand;			// a target_sessionCommand can set this to return something to the session 

	idMultiplayerGame		mpGame;					// handles rules for standard dm

	idEditEntities *		editEntities;			// in game editing

	int						cinematicSkipTime;		// don't allow skipping cinemetics until this time has passed so player doesn't skip out accidently from a firefight
	int						cinematicStopTime;		// cinematics have several camera changes, so keep track of when we stop them so that we don't reset cinematicSkipTime unnecessarily
	int						cinematicMaxSkipTime;	// time to end cinematic when skipping.  there's a possibility of an infinite loop if the map isn't set up right.
	bool					inCinematic;			// game is playing cinematic (player controls frozen)
	bool					skipCinematic;

													// are kept up to date with changes to serverInfo
	int						framenum;
	int						previousTime;			// time in msec of last frame
	int						time;					// in msec
	int						msec;					// time since last update in milliseconds
	int						mHz;					// hertz

	int						vacuumAreaNum;			// -1 if level doesn't have any outside areas

// RAVEN BEGIN
// abahr:
	idList<rvGravityArea*>	gravityInfo;			// area num for each gravity zone
	idList< idEntityPtr<idEntity> > scriptObjectProxies;
// RAVEN END

	gameType_t				gameType;
	bool					isMultiplayer;			// set if the game is run in multiplayer mode
	bool					isServer;				// set if the game is run for a dedicated or listen server
	bool					isClient;				// set if the game is run for a client
													// discriminates between the RunFrame path and the ClientPrediction path
													// NOTE: on a listen server, isClient is false
// RAVEN BEGIN
// ddynerman: set if we're a server and not dedicated
	bool					isListenServer;			
// RAVEN END
	int						localClientNum;			// number of the local client. MP: -1 on a dedicated, MAX_CLIENTS when playing a server demo
	idLinkList<idEntity>	snapshotEntities;		// entities from the last snapshot
	int						realClientTime;			// real client time
	bool					isNewFrame;				// true if this is a new game frame, not a rerun due to prediction
	int						entityDefBits;			// bits required to store an entity def number

	static const char *		sufaceTypeNames[ MAX_SURFACE_TYPES ];	// text names for surface types

	idEntityPtr<idEntity>	lastGUIEnt;				// last entity with a GUI, used by Cmd_NextGUI_f
	int						lastGUI;				// last GUI on the lastGUIEnt

// RAVEN BEGIN
// bdube: added
	int						editors;				// Mirrored editors flags from common determine which editors are running
	bool					isLastPredictFrame;		// on an MP server or in SP game this means 'last catchup frame' rather than predict
// RAVEN END

// RAVEN BEGIN
// rjohnson: entity usage stats
	idStr					mapFileNameStripped;		// name of the map, empty string if no map loaded, with path and extension removed.  If entity filter, that is appended
	idList<idDict>			entityUsageList;
// ddynerman: the entity currently thinking, used to play effects/etc only in the appropriate instance
	idEntity*				currentThinkingEntity;

	const static int		INITIAL_SPAWN_COUNT = 1;

	idFreeView				freeView;

// RAVEN END

	int filterMod;
	idList<idStr> modList;

	// ---------------------- Public idGame Interface -------------------

							idGameLocal();

// RAVEN BEGIN
// jsinger: attempt to eliminate cross-DLL allocation issues
#ifdef RV_UNIFIED_ALLOCATOR
	virtual void			Init( void *(*allocator)( size_t size ), void (*deallocator)( void *ptr ), size_t (*msize)( void *ptr ) );
#else
	virtual void			Init( void );
#endif
// RAVEN END
	virtual void			Shutdown( void );
	virtual void			SetLocalClient( int clientNum );
	virtual void			ThrottleUserInfo( void );
	virtual const idDict *	SetUserInfo( int clientNum, const idDict &userInfo, bool isClient );
	virtual const idDict *	GetUserInfo( int clientNum );
	virtual bool			IsClientActive( int clientNum );
	virtual void			SetServerInfo( const idDict &serverInfo );

	virtual const idDict *	RepeaterSetUserInfo( int clientNum, const idDict &userInfo ) { assert(false); return NULL; }

	virtual const idDict &	GetPersistentPlayerInfo( int clientNum );
	virtual void			SetPersistentPlayerInfo( int clientNum, const idDict &playerInfo );
	virtual void			InitFromNewMap( const char *mapName, idRenderWorld *renderWorld, bool isServer, bool isClient, int randSeed );
	virtual bool			InitFromSaveGame( const char *mapName, idRenderWorld *renderWorld, idFile *saveGameFile );
// RAVEN BEGIN
// mekberg: added saveTypes
	virtual void			SaveGame( idFile *saveGameFile, saveType_t saveType = ST_REGULAR );
// RAVEN END
	virtual void			MapShutdown( void );
	virtual void			CacheDictionaryMedia( const idDict *dict );
	virtual void			SpawnPlayer( int clientNum );
// RAVEN BEGIN
	virtual gameReturn_t	RunFrame( const usercmd_t *clientCmds, int activeEditors, bool lastCatchupFrame, int serverGameFrame );
	virtual	void			MenuFrame( void );
// RAVEN END
	virtual void			RepeaterFrame( const userOrigin_t *clientOrigins, bool lastCatchupFrame, int spoolTime = 0 ) {};
	virtual bool			Draw( int clientNum );
	virtual escReply_t		HandleESC( idUserInterface **gui );
	virtual idUserInterface	*StartMenu( void );
	virtual const char *	HandleGuiCommands( const char *menuCommand );
	virtual void			HandleMainMenuCommands( const char *menuCommand, idUserInterface *gui );
	virtual allowReply_t	ServerAllowClient( int clientId, int numClients, const char *IP, const char *guid, const char *password, const char *privatePassword, char reason[MAX_STRING_CHARS] );
	virtual void			ServerClientConnect( int clientNum, const char *guid );
	virtual void			ServerClientBegin( int clientNum );
	virtual void			ServerClientDisconnect( int clientNum );
	virtual void			ServerWriteInitialReliableMessages( int clientNum );
	virtual allowReply_t	RepeaterAllowClient( int clientId, int numClients, const char *IP, const char *guid, bool repeater, const char *password, const char *privatePassword, char reason[MAX_STRING_CHARS] ) { idStr::Copynz( reason, "#str_107239" /* zinx - FIXME - not banned... */, sizeof(reason) ); return ALLOW_NO; };
	virtual void			RepeaterClientConnect( int clientNum ) {assert(false);};
	virtual void			RepeaterClientBegin( int clientNum ) {assert(false);};
	virtual void			RepeaterClientDisconnect( int clientNum ) {assert(false);};
	virtual void			RepeaterWriteInitialReliableMessages( int clientNum ) {assert(false);};
// RAVEN BEGIN
// jnewquist: Use dword array to match pvs array so we don't have endianness problems.
	virtual void			ServerWriteSnapshot( int clientNum, int sequence, idBitMsg &msg, dword *clientInPVS, int numPVSClients, int lastSnapshotFrame );
// RAVEN END
	virtual bool			ServerApplySnapshot( int clientNum, int sequence );
	virtual void			ServerProcessReliableMessage( int clientNum, const idBitMsg &msg );
	virtual bool			RepeaterApplySnapshot( int clientNum, int sequence ) { assert(false); return false; }
	virtual void			RepeaterProcessReliableMessage( int clientNum, const idBitMsg &msg ) { assert(false); }
	virtual void			ClientReadSnapshot( int clientNum, int snapshotSequence, const int gameFrame, const int gameTime, const int dupeUsercmds, const int aheadOfServer, const idBitMsg &msg );
	virtual bool			ClientApplySnapshot( int clientNum, int sequence );
	virtual void			ClientProcessReliableMessage( int clientNum, const idBitMsg &msg );
	virtual gameReturn_t	ClientPrediction( int clientNum, const usercmd_t *clientCmds, bool lastPredictFrame = true, ClientStats_t *cs = NULL );
// RAVEN BEGIN
// ddynerman: client game frame
	virtual void			ClientRun( void );
	virtual void			ClientEndFrame( void );

// jshepard: rcon password check
	virtual void			ProcessRconReturn( bool success );
	virtual void			ResetRconGuiStatus( void );
// RAVEN END
	
	virtual void			GetClientStats( int clientNum, char *data, const int len );
	virtual void			SwitchTeam( int clientNum, int team );

	virtual bool			DownloadRequest( const char *IP, const char *guid, const char *paks, char urls[ MAX_STRING_CHARS ] );

	virtual bool			HTTPRequest( const char *IP, const char *file, bool isGamePak );

// RAVEN BEGIN
// bdube: client hitscan
	virtual void			ClientHitScan( const idBitMsg &msg );
// jscott: for effects system
	virtual void			StartViewEffect( int type, float time, float scale );
	virtual void			GetPlayerView( idVec3 &origin, idMat3 &axis );
	virtual void			Translation( trace_t &trace, idVec3 &source, idVec3 &dest, idTraceModel *trm, int clipMask );
	virtual void			SpawnClientMoveable ( const char* name, int lifetime, const idVec3& origin, const idMat3& axis, const idVec3& velocity, const idVec3& angular_velocity );
// bdube: added debug methods
	virtual void			DebugSetString ( const char* name, const char* value );
	virtual void			DebugSetFloat ( const char* name, float value );
	virtual void			DebugSetInt ( const char* name, int value );
	virtual const char*		DebugGetStatString ( const char* name );
	virtual int				DebugGetStatInt ( const char* name );
	virtual float			DebugGetStatFloat ( const char* name );
	virtual bool			IsDebugHudActive ( void ) const;
// rjohnson: for new note taking mechanism
	virtual bool			GetPlayerInfo( idVec3 &origin, idMat3 &axis, int PlayerNum = -1, idAngles *deltaViewAngles = NULL, int reqClientNum = -1 );
	virtual void			SetPlayerInfo( idVec3 &origin, idMat3 &axis, int PlayerNum = -1 );
	virtual	bool			PlayerChatDisabled( int clientNum );
	virtual void			SetViewComments( const char *text = 0 );
// ddynerman: utility functions
	virtual void			GetPlayerName( int clientNum, char* name );
	virtual void			GetPlayerClan( int clientNum, char* clan );
	virtual void			SetFriend( int clientNum, bool isFriend );
	static  void			Cmd_PrintMapEntityNumbers_f( const idCmdArgs& args );
	static  void			Cmd_PrintSpawnIds_f( const idCmdArgs& args );
// abahr:
	virtual int				GetNumGravityAreas() const;
	virtual const rvGravityArea* GetGravityInfo( int index ) const;
	virtual void			SetGravityInfo( int index, rvGravityArea* info );
	virtual void			AddUniqueGravityInfo( rvGravityArea* info );

	virtual int				GetCurrentGravityInfoIndex( const idVec3& origin ) const;
	virtual bool			InGravityArea( idEntity* entity ) const;
	virtual int				GetCurrentGravityInfoIndex( idEntity* entity ) const;
	virtual const idVec3	GetCurrentGravity( idEntity* entity ) const;

	virtual const idVec3	GetCurrentGravity( const idVec3& origin, const idMat3& axis ) const;

	virtual bool			InGravityArea( rvClientEntity* entity ) const;
	virtual int				GetCurrentGravityInfoIndex( rvClientEntity* entity ) const;
	virtual const idVec3	GetCurrentGravity( rvClientEntity* entity ) const;
	virtual idEntity*		ReferenceScriptObjectProxy( const char* scriptObjectName );
	virtual void			ReleaseScriptObjectProxy( const char* proxyName );

// rjohnson: entity usage stats
	virtual void			ListEntityStats( const idCmdArgs &args );
// RAVEN END

	virtual void			SetDemoState( demoState_t state, bool serverDemo, bool timeDemo );
	virtual void			SetRepeaterState( bool isRepeater, bool serverIsRepeater ) {if (isRepeater || serverIsRepeater) Warning("Repeater does not work for single player.");};
	virtual void			WriteNetworkInfo( idFile* file, int clientNum );
	virtual void			ReadNetworkInfo( int gameTime, idFile* file, int clientNum );
	virtual bool			ValidateDemoProtocol( int minor_ref, int minor );

	virtual void			ServerWriteServerDemoSnapshot( int sequence, idBitMsg &msg, int lastSnapshotFrame );
	virtual void			ClientReadServerDemoSnapshot( int sequence, const int gameFrame, const int gameTime, const idBitMsg &msg );

	virtual void			RepeaterWriteSnapshot( int clientNum, int sequence, idBitMsg &msg, dword *clientInPVS, int numPVSClients, const userOrigin_t &pvs_origin, int lastSnapshotFrame ) {assert(false);};
	virtual void			RepeaterEndSnapshots( void ) {};
	virtual void			ClientReadRepeaterSnapshot( int sequence, const int gameFrame, const int gameTime, const int aheadOfServer, const idBitMsg &msg ) {assert(false);};

	virtual int				GetDemoFollowClient( void ) { return serverDemo ? followPlayer : -1; }

	virtual void			GetBotInput( int clientNum, usercmd_t &userCmd ) { Error( "Bot input requested\n" ); };

	virtual const char *	GetLoadingGui( const char *mapDeclName ) { return NULL; }
	virtual void			SetupLoadingGui( idUserInterface *gui ) {}

	// ---------------------- Public idGameLocal Interface -------------------

	void					Printf( const char *fmt, ... ) const;
	void					DPrintf( const char *fmt, ... ) const;
	void					Warning( const char *fmt, ... ) const;
	void					DWarning( const char *fmt, ... ) const;
	void					Error( const char *fmt, ... ) const;

							// Initializes all map variables common to both save games and spawned games
	void					LoadMap( const char *mapName, int randseed );

	void					LocalMapRestart( int instance = -1 );
	void					MapRestart( int instance = -1 );
	static void				VerifyServerSettings_f( const idCmdArgs &args );
	static void				MapRestart_f( const idCmdArgs &args );
	bool					NextMap( void );	// returns wether serverinfo settings have been modified
	static void				NextMap_f( const idCmdArgs &args );

	idMapFile *				GetLevelMap( void );
	const char *			GetMapName( void ) const;

	int						NumAAS( void ) const;
	idAAS *					GetAAS( int num ) const;
	idAAS *					GetAAS( const char *name ) const;
// RAVEN BEGIN
// jscott: added accessor for memory tracking
	int						GetNumAAS( void ) const { return( aasList.Num() ); }
// RAVEN END
	void					SetAASAreaState( const idBounds &bounds, const int areaContents, bool closed );
	aasHandle_t				AddAASObstacle( const idBounds &bounds );
	void					RemoveAASObstacle( const aasHandle_t handle );
	void					RemoveAllAASObstacles( void );
// RAVEN BEGIN
// mwhitlock: added entity memory usage stuff.
	size_t					GetEntityMemoryUsage ( void ) const;
// RAVEN END
	bool					CheatsOk( bool requirePlayer = true );
	void					SetSkill( int value );
	gameState_t				GameState( void ) const;
	void					SetGameState( gameState_t newState ) { gamestate = newState; }
	idEntity *				SpawnEntityType( const idTypeInfo &classdef, const idDict *args = NULL, bool bIsClientReadSnapshot = false );
	bool					SpawnEntityDef( const idDict &args, idEntity **ent = NULL, bool setDefaults = true );
	bool					SpawnClientEntityDef( const idDict &args, rvClientEntity **ent = NULL, bool setDefaults = true, const char* spawn = NULL );
// abahr:
	idEntity*				SpawnEntityDef( const char* entityDefName, const idDict* additionalArgs = NULL );
	template< class type >
	type*					SpawnSafeEntityDef( const char* entityDefName, const idDict* additionalArgs = NULL );
	int						GetPreviousTime() const { return previousTime; }
// RAVEN END
	int						GetSpawnId( const idEntity *ent ) const;

	const idDeclEntityDef *	FindEntityDef( const char *name, bool makeDefault = true ) const;
	const idDict *			FindEntityDefDict( const char *name, bool makeDefault = true ) const;

	void					RegisterEntity( idEntity *ent );
	void					UnregisterEntity( idEntity *ent );
	// used to skip one when registering entities, leaving an empty entity in the array
	void					SkipEntityIndex( void );

	bool					RequirementMet( idEntity *activator, const idStr &requires, int removeItem );

// RITUAL BEGIN
// squirrel: accessor for si_weaponStay checks
	bool					IsWeaponsStayOn( void );
// RITUAL END

// RAVEN BEGIN
// bdube: client entities
	void					RegisterClientEntity( rvClientEntity *cent );
	void					UnregisterClientEntity( rvClientEntity *cent );
// RAVEN END

	void					AlertAI( idEntity *ent );
// RAVEN BEGIN
// bdube: added get alert actor
	idActor *				GetAlertActor( void );
	idEntity *				GetAlertEntity( void );
// RAVEN END


	bool					InPlayerPVS( idEntity *ent ) const;
	bool					InPlayerConnectedArea( idEntity *ent ) const;

	void					SetCamera( idCamera *cam );
	idCamera *				GetCamera( void ) const;
	bool					SkipCinematic( void );

// RAVEN BEGIN
// jscott: for portal skies
	idCamera				*GetPortalSky( void ) const;
	void					SetPortalSky( idCamera *cam );
// RAVEN END

	void					CalcFov( float base_fov, float &fov_x, float &fov_y ) const;

	void					AddEntityToHash( const char *name, idEntity *ent );
	bool					RemoveEntityFromHash( const char *name, idEntity *ent );
	int						GetTargets( const idDict &args, idList< idEntityPtr<idEntity> > &list, const char *ref ) const;

							// returns the master entity of a trace.  for example, if the trace entity is the player's head, it will return the player.
	idEntity *				GetTraceEntity( const trace_t &trace ) const;

	static void				ArgCompletion_EntityName( const idCmdArgs &args, void(*callback)( const char *s ) );
	idEntity *				FindTraceEntity( idVec3 start, idVec3 end, const idTypeInfo &c, const idEntity *skip ) const;
	static void				ArgCompletion_AIName( const idCmdArgs &args, void(*callback)( const char *s ) );

//RAVEN BEGIN
// bgeisler: added, I don't want to have to do this work myself every single time I have an entityNumber
	idEntity *				FindEntity( int entityNumber )	{ return ((entityNumber >= 0 && entityNumber < MAX_GENTITIES) ? entities[entityNumber] : NULL); }
//RAVEN BEGIN

	idEntity *				FindEntity( const char *name ) const;
	idEntity *				FindEntityUsingDef( idEntity *from, const char *match ) const;
	int						EntitiesWithinRadius( const idVec3 org, float radius, idEntity **entityList, int maxCount ) const;

	void					KillBox( idEntity *ent, bool catch_teleport = false );
	void					RadiusPush( const idVec3 &origin, const float radius, const float push, const idEntity *inflictor, const idEntity *ignore, float inflictorScale, const bool quake );
// RAVEN BEGIN
// ddynerman: return number of people damaged
	void					RadiusDamage( const idVec3 &origin, idEntity *inflictor, idEntity *attacker, idEntity *ignoreDamage, idEntity *ignorePush, const char *damageDefName, float dmgPower = 1.0f, int* hitCount = NULL );
// bdube: inflictor
	void					RadiusPushClipModel( idEntity* inflictor, const idVec3 &origin, const float push, const idClipModel *clipModel );
// RAVEN END

	void					ProjectDecal( const idVec3 &origin, const idVec3 &dir, float depth, bool parallel, float size, const char *material, float angle = 0 );
// RAVEN BEGIN
// ddynerman: multiple collision worlds
	void					BloodSplat( const idEntity* ent, const idVec3 &origin, const idVec3 &dir, float size, const char *material );
// RAVEN END

	void					CallFrameCommand( idEntity *ent, const function_t *frameCommand );
// RAVEN BEGIN
// bdube: added script object frame commands
	void					CallFrameCommand( idScriptObject* obj, const function_t* frameCommand );
	void					CallFrameCommand( idEntity* ent, const char* frameCommand );
// RAVEN END	

	void					CallObjectFrameCommand( idEntity *ent, const char *frameCommand );

	const idVec3 &			GetGravity( void ) const;

	// added the following to assist licensees with merge issues
	int						GetFrameNum() const { return framenum; }
	int						GetTime() const { return time; }
	int						GetMSec() const { return msec; }
	int						GetMHz() const { return mHz; }

	int						GetNextClientNum( int current ) const;
	idPlayer *				GetClientByNum( int current ) const;
	idPlayer *				GetClientByName( const char *name ) const;
	idPlayer *				GetClientByCmdArgs( const idCmdArgs &args ) const;
	int						GetClientNumByName( const char *name ) const;

	idPlayer *				GetLocalPlayer() const;
	
// RAVEN BEGIN
// jshepard: update player data after main menu close
	void					UpdatePlayerPostMainMenu();

// bdube: added
	int						GetSpawnCount ( void ) const;
	void					SetSpawnCount ( int newSpawnCount ) { spawnCount = newSpawnCount; }
// ddynerman: team type
	bool					IsTeamGame ( void ) const;
// RAVEN END	
	
	void					SpreadLocations();
	idLocationEntity *		LocationForPoint( const idVec3 &point );	// May return NULL
// RAVEN BEGIN
// bdube: added
	idLocationEntity*		AddLocation				( const idVec3& point, const char* name );
// ddynerman: new gametype specific spawn code
	bool					SpotWouldTelefrag( idPlayer* player, idPlayerStart* spawn );
	idEntity*				SelectSpawnPoint( idPlayer* player );
	void					UpdateForwardSpawns( rvCTFAssaultPlayerStart* point, int team );
	void					ClearForwardSpawns( void );
// RAVEN END

	void					SetPortalState( qhandle_t portal, int blockingBits );
	void					ServerSendChatMessage( int to, const char *name, const char *text, const char *parm = "" );

	void					SetGlobalMaterial( const idMaterial *mat );
	const idMaterial *		GetGlobalMaterial();

	void					SetGibTime( int _time ) { nextGibTime = _time; }
	int						GetGibTime() { return nextGibTime; }
// RITUAL BEGIN
// squirrel: added DeadZone multiplayer mode
	void					SetUnFreezeTime( int _time ) { unFreezeTime = _time; };
	int						GetUnFreezeTime() { return unFreezeTime; };
	void					SetIsFrozen( bool _isFrozen ) { isFrozen = _isFrozen; };
	bool					GetIsFrozen() { return isFrozen; };
// RITUAL END
	bool					NeedRestart();

// RAVEN BEGIN
// jshepard: update end of level on player hud
	void					UpdateEndLevel();
// MCG: added whizz-by sound
	void					CheckPlayerWhizzBy	( idVec3 start, idVec3 end, idEntity* hitEnt, idEntity *attacker );
// bdube: added hitscan
// twhitaker: added additionalIgnore parameter
	idEntity*				HitScan				( const idDict& hitscanDef, const idVec3& origin, const idVec3& dir, const idVec3& fxOrigin, idEntity* owner = NULL, bool noFX = false, float damageScale = 1.0f, idEntity * additionalIgnore = NULL, int *areas = NULL );
// bdube: added effect calls
	virtual rvClientEffect*	PlayEffect			( const idDecl *effect, const idVec3& origin, const idMat3& axis, bool loop = false, const idVec3& endOrigin = vec3_origin, bool broadcast = false, bool predictBit = false, effectCategory_t category = EC_IGNORE, const idVec4& effectTint = vec4_one );
	rvClientEffect*			PlayEffect			( const idDict& args, const char* effectName, const idVec3& origin, const idMat3& axis, bool loop = false, const idVec3& endOrigin = vec3_origin, bool broadcast = false, effectCategory_t category = EC_IGNORE, const idVec4& effectTint = vec4_one );
	const idDecl			*GetEffect			( const idDict& args, const char* effectName, const rvDeclMatType* materialType = NULL );

	idList<idEntity*>		ambientLights; // lights that cast ambient

// ddynerman:	multiple collision world - game collision wrapper functions to
//				use the correct idClip
//				---------------------------------------------------------------
//				These are wrapper functions around idClip collision detection 
//				functions.  They expose the collision detection engine to the 
//				game code, but do collision world determination in one spot.
//              'ent' refers to the entity we want collision information about
	bool					Translation	( const idEntity* ent, trace_t &results, const idVec3 &start, const idVec3 &end, const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, const idEntity *passEntity2 = 0 );
	bool					Rotation	( const idEntity* ent, trace_t &results, const idVec3 &start, const idRotation &rotation, const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity );
	bool					Motion		( const idEntity* ent, trace_t &results, const idVec3 &start, const idVec3 &end, const idRotation &rotation, const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity );
	int						Contacts	( const idEntity* ent, contactInfo_t *contacts, const int maxContacts, const idVec3 &start, const idVec6 &dir, const float depth, const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity );
	int						Contents	( const idEntity* ent, const idVec3 &start, const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, idEntity **touchedEntity = NULL );
	// special case translations versus the rest of the world
	bool					TracePoint	( const idEntity* ent, trace_t &results, const idVec3 &start, const idVec3 &end, int contentMask, const idEntity *passEntity );
	bool					TraceBounds	( const idEntity* ent, trace_t &results, const idVec3 &start, const idVec3 &end, const idBounds &bounds, int contentMask, const idEntity *passEntity );
	// clip versus a specific model
	void					TranslationModel( const idEntity* ent, trace_t &results, const idVec3 &start, const idVec3 &end, const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, cmHandle_t model, const idVec3 &modelOrigin, const idMat3 &modelAxis );
	void					RotationModel	( const idEntity* ent, trace_t &results, const idVec3 &start, const idRotation &rotation, const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, cmHandle_t model, const idVec3 &modelOrigin, const idMat3 &modelAxis );
	int						ContactsModel	( const idEntity* ent, contactInfo_t *contacts, const int maxContacts, const idVec3 &start, const idVec6 &dir, const float depth, const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, cmHandle_t model, const idVec3 &modelOrigin, const idMat3 &modelAxis );
	int						ContentsModel	( const idEntity* ent, const idVec3 &start, const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, cmHandle_t model, const idVec3 &modelOrigin, const idMat3 &modelAxis );
	// clip versus all entities but not the world
	void					TranslationEntities( const idEntity* ent, trace_t &results, const idVec3 &start, const idVec3 &end, const idClipModel *mdl, const idMat3 &trmAxis, int contentMask, const idEntity *passEntity, const idEntity *passEntity2 = 0 );
	// get a contact feature
	bool					GetModelContactFeature( const idEntity* ent, const contactInfo_t &contact, const idClipModel *clipModel, idFixedWinding &winding ) const;
	// get entities/clip models within or touching the given bounds
	int						EntitiesTouchingBounds	( const idEntity* ent, const idBounds &bounds, int contentMask, idEntity **entityList, int maxCount ) const;
	int						ClipModelsTouchingBounds( const idEntity* ent, const idBounds &bounds, int contentMask, idClipModel **clipModelList, int maxCount ) const;
	int						PlayersTouchingBounds	( const idEntity* ent, const idBounds &bounds, int contentMask, idPlayer **entityList, int maxCount ) const;
	const idBounds &		GetWorldBounds( const idEntity* ent ) const;

	void					Link( idClipModel* clip, idEntity *ent, int newId, const idVec3 &newOrigin, const idMat3 &newAxis, int renderModelHandle = -1 );

	idClip*					GetEntityClipWorld( const idEntity* ent );
	const idClip*			GetEntityClipWorld( const idEntity* ent ) const;
	int						GetNumMapEntities( void ) const;

	int						AddClipWorld( int id );
	void					RemoveClipWorld( int id );
	int						AddInstance( int id = -1, bool deferPopulate = false );
	void					RemoveInstance( int id );
	rvInstance*				GetInstance( int id );
	int						GetNumInstances( void );
// ddynerman: multiple game instances
	void					SpawnMapEntities( int instance = 0, unsigned short* entityNumIn = NULL, unsigned short* entityNumOut = NULL, int* startSpawnCount = NULL );
	void					InstanceClear( void );
// ddynerman: utility function
	virtual const char*		GetLongGametypeName( const char* gametype );
	virtual void			ReceiveRemoteConsoleOutput( const char* output );

	bool					IsFlagGameType( void ) { return ( gameType == GAME_CTF || gameType == GAME_1F_CTF || gameType == GAME_ARENA_CTF || gameType == GAME_ARENA_1F_CTF ); }
	bool					IsTeamGameType( void ) { return ( gameType == GAME_TDM || gameType == GAME_CTF || gameType == GAME_ARENA_CTF || gameType == GAME_DEADZONE ); }
	bool					IsTeamPowerups( void );

	// twhitaker: needed this for difficulty settings
	float					GetDifficultyModifier( void ) { const static float difficulty[] = { -0.3f, 0.0f, 0.4f, 0.8f }; return difficulty[ idMath::ClampInt( 0, 3, g_skill.GetInteger() ) ]; }

	bool					IsMultiplayer( void ) { return isMultiplayer; }

// mekberg: added
	bool					InCinematic( void ) { return inCinematic; }

	// mekberg: so ban list can be populated outside of multiplayer game
	void					PopulateBanList( idUserInterface* hud );
// RAVEN END

// RAVEN BEGIN
// mwhitlock: Dynamic memory consolidation
#if defined(_RV_MEM_SYS_SUPPORT)
	virtual void			FlushBeforelevelLoad( void );
#endif
// RAVEN END

	void					ServerSendInstanceReliableMessageExcluding( const idEntity* owner, int excludeClient, const idBitMsg& msg );
	void					ServerSendInstanceReliableMessage( const idEntity* owner, int clientNum, const idBitMsg& msg );	

	void					SendUnreliableMessage( const idBitMsg &msg, const int clientNum );
	// note: local client on dedicated server is always excluded
	void					SendUnreliableMessagePVS( const idBitMsg &msg, const idEntity *instanceEnt, int area1 = -1, int area2 = -1 );

	demoState_t				GetDemoState( void ) const { return demoState; }
	bool					IsServerDemo( void ) const { return serverDemo; }
	bool					IsTimeDemo( void ) const { return timeDemo; }
	int						GetDemoFollowClient( void ) const { return serverDemo ? followPlayer : -1; }
	idUserInterface			*GetDemoHud( void );
	idUserInterface			*GetDemoMphud( void );
	idUserInterface			*GetDemoCursor( void );

	/*
	do not synchronize implicit decls over the network
	implicit decls are created when loading sounds and materials that don't have an explicit entry in the def files
	clients and server may load those in different orders in a same map, or even join in with different orders because of different map histories before this game
	in D3 we maintain remap tables, but it's much better to have tighter multiplayer assets so we have no need at all
	still, you want to catch when bad things happen, so indexes should ALL be read and written through these functions
	*/
	static void				WriteDecl( idBitMsg &msg, const idDecl *decl );
	static const idDecl*	ReadDecl( const idBitMsg &msg, declType_t type );
	static void				WriteDecl( idBitMsgDelta &msg, const idDecl *decl );
	static const idDecl*	ReadDecl( const idBitMsgDelta &msg, declType_t type );

	idPlayerStart			*RandomSpawn( void );

	int						GetStartingIndexForInstance( int instanceID );
	void					ClientSetStartingIndex( int i ) { clientInstanceFirstFreeIndex = i; }
	void					ServerSetMinSpawnIndex( void );
	void					ServerSetEntityIndexWatermark( int instanceID );

#ifndef _QUAKE4
private:
#endif
// RAVEN BEGIN
// ddynerman: multiple instance for MP
	idList<idClip*>			clip;					// collision detection
	idList<rvInstance*>		instances;
// RAVEN END
#ifdef _RAVEN
	virtual void				GetBestGameType(const char *map, const char *gametype, char buf[ MAX_STRING_CHARS ]);
private:
	idStr GetBestMPGametype(const char *map, const char *gametype) const;
		int						GetTimeGroupTime(int timeGroup);
#endif

	// keep watermarks on the high entity index
	// server transmits this to clients so they use the right entity layout
	idList<int>				instancesEntityIndexWatermarks;
	int						clientInstanceFirstFreeIndex;

	idStr					mapFileName;			// name of the map, empty string if no map loaded
	idMapFile *				mapFile;				// will be NULL during the game unless in-game editing is used
	bool					mapCycleLoaded;

	int						spawnCount;
	bool					isMapEntity[ MAX_GENTITIES ]; // it's handy to know which entities are part of the map
// RAVEN BEGIN
// bdube: client entities	
	int						clientSpawnCount;
// RAVEN END

	idLocationEntity **		locationEntities;		// for location names, etc

	idCamera *				camera;
	const idMaterial *		globalMaterial;			// for overriding everything

// RAVEN BEGIN
// jscott: for portal skies
	idCamera				*portalSky;
	bool					portalSkyVisible;
// RAVEN END

	idList<idAAS *>			aasList;				// area system
	idStrList				aasNames;

// RAVEN BEGIN
// bdube: GetAlertActor
	idEntityPtr<idActor>	lastAIAlertActor;
	int						lastAIAlertActorTime;
	idEntityPtr<idEntity>	lastAIAlertEntity;
	int						lastAIAlertEntityTime;
// RAVEN END

	idDict					spawnArgs;				// spawn args used during entity spawning  FIXME: shouldn't be necessary anymore
// RAVEN BEGIN
// nmckenzie:
	const idDeclEntityDef *	spawnOverrides;
// RAVEN END

	pvsHandle_t				playerPVS;				// merged pvs of all players
	bool					freePlayerPVS;			// tracks if playerPVS needs to be released
	pvsHandle_t				playerConnectedAreas;	// all areas connected to any player area

	idVec3					gravity;				// global gravity vector
	gameState_t				gamestate;				// keeps track of whether we're spawning, shutting down, or normal gameplay
	bool					influenceActive;		// true when a phantasm is happening
	int						nextGibTime;
// RITUAL BEGIN
// squirrel: added DeadZone multiplayer mode
	int						unFreezeTime;			// time at which players unfreeze and the match begins
	bool					isFrozen;				// true if the match is frozen (for buying, etc.)
// RITUAL END

	entityState_t *			clientEntityStates[MAX_CLIENTS+1][MAX_GENTITIES];	// MAX_CLIENTS slot is for server demo recordings
	int						clientPVS[MAX_CLIENTS+1][ENTITY_PVS_SIZE];
	snapshot_t *			clientSnapshots[MAX_CLIENTS+1];
// RAVEN BEGIN
// jnewquist: Mark memory tags for idBlockAlloc
	idBlockAlloc<entityState_t,256/*,MA_ENTITY*/> entityStateAllocator; //k
	idBlockAlloc<snapshot_t,64/*,MA_ENTITY*/> snapshotAllocator; //k
// RAVEN END

	idEventQueue			eventQueue;

	idList<idPlayerStart*>	spawnSpots;
// RAVEN BEGIN
// ddynerman: two lists to hold team spawn points for team based games
	idList<idPlayerStart*>	teamSpawnSpots[TEAM_MAX];	
	idList<idPlayerStart*>	teamForwardSpawnSpots[TEAM_MAX]; // forward spawn positions, used in CTF
// RAVEN END

	idDict					newInfo;

	idStrList				shakeSounds;

	byte					lagometer[ LAGO_IMG_HEIGHT ][ LAGO_IMG_WIDTH ][ 4 ];

	idMsgQueue				unreliableMessages[ MAX_CLIENTS+1 ];	// MAX_CLIENTS slot for server demo recording

	demoState_t				demoState;
	bool					serverDemo;
	bool					timeDemo;

	// demo interaction usercmds
	usercmd_t				usercmd, oldUsercmd;
	int						followPlayer;	// free fly or spectate follow through local interaction during server demo replays
	idUserInterface			*demo_hud;
	idUserInterface			*demo_mphud;
	idUserInterface			*demo_cursor;

	int						demo_protocol;	// keep track of the protocol of the demo we're replaying

private:

	void					Clear( void );
							// returns true if the entity shouldn't be spawned at all in this game type or difficulty level
	bool					InhibitEntitySpawn( idDict &spawnArgs );
							// spawn entities from the map file
							// commons used by init, shutdown, and restart
	void					MapPopulate( int instance = -1 );
	void					MapClear( bool clearClients, int instance = -1 );

	bool					SetupPortalSkyPVS( idPlayer *player );
// RAVEN END
	void					SetupPlayerPVS( void );
	void					FreePlayerPVS( void );
	void					UpdateGravity( void );
	void					SortActiveEntityList( void );
	void					ShowTargets( void );
	void					RunDebugInfo( void );

	void					InitScriptForMap( void );
	void					InitConsoleCommands( void );
	void					ShutdownConsoleCommands( void );

	void					InitAsyncNetwork( void );
	void					ShutdownAsyncNetwork( void );
	void					InitLocalClient( int clientNum );
	void					FreeSnapshotsOlderThanSequence( int clientNum, int sequence );
	bool					ApplySnapshot( int clientNum, int sequence );
	void					WriteGameStateToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadGameStateFromSnapshot( const idBitMsgDelta &msg );
	void					NetworkEventWarning( const entityNetEvent_t *event, const char *fmt, ... ) id_attribute((format(printf,3,4)));
	void					ServerProcessEntityNetworkEventQueue( void );
	void					ClientProcessEntityNetworkEventQueue( void );
	void					ClientShowSnapshot( int clientNum ) const;
	void					SetGameType( void );
// RAVEN BEGIN
// ddynerman: gametype specific spawn code
	void					InitializeSpawns( void );
	idList<spawnSpot_t>		WeightSpawnSpots( idPlayer* player );
// RAVEN END
	static int				sortSpawnPoints( const void *ptr1, const void *ptr2 );

	void					DumpOggSounds( void );
	void					GetShakeSounds( const idDict *dict );
	bool					ValidateServerSettings( const char *map, const char *gametype );

	void					Tokenize( idStrList &out, const char *in );

// RAVEN BEGIN
// ddynerman: multiple clip worlds
	void					ShutdownInstances( void );
// shouchard:  ban list support (lives in game_network.cpp)

	void					UpdateLagometer( int aheadOfServer, int dupeUsercmds );

	void					ClientReadUnreliableMessages( const idBitMsg &msg );
	void					ProcessUnreliableMessage( const idBitMsg &msg );

	void					UpdateClientsPVS( void );

	bool					IsDemoReplayInAreas( int area1, int area2 );

	void					BuildModList( void );

public:
	void					LoadBanList();
	void					SaveBanList();
	void					FlushBanList();
	bool					IsPlayerBanned( const char *name );
	bool					IsGuidBanned( const char *guid );
	void					AddGuidToBanList( const char *guid );
	void					RemoveGuidFromBanList( const char *guid );
	virtual void			RegisterClientGuid( int clientNum, const char *guid );

	int						GetBanListCount();
	const mpBanInfo_t*		GetBanListEntry( int entry );	// returns client name
	const char*				GetGuidByClientNum( int clientNum );	// returns GUID
	int						GetClientNumByGuid( const char* );		// returns clientNum

// mekberg: get and send ban list
	void					ServerSendBanList( int clientNum );

// jscott: made public
	pvsHandle_t				GetClientPVS( idPlayer *player, pvsType_t type );

	int						GetCurrentDemoProtocol( void ) { return demo_protocol; }
#ifdef _QUAKE4
// jmarshall
	virtual void			ServerClientBegin( int clientNum, bool isBot, const char* botName);
	virtual void			SpawnPlayer( int clientNum, bool isBot, const char* botName);
	virtual int				GetBotItemEntry(const char* name);
	virtual int				TravelTimeToGoal(const idVec3& origin, const idVec3& goal);
	void	Trace(trace_t& results, const idVec3& start, const idVec3& end, int contentMask, int passEntity);
	idAAS* GetBotAAS(void)
	{
		if (NumAAS() == 0)
			return 0;

		return GetAAS(0);
	}
	void					RegisterBot(rvmBot* bot)
	{
		registeredBots.AddUnique(bot);
	}
	void					UnRegisterBot(rvmBot* bot)
	{
		registeredBots.Remove(bot);
	}
	float					SysScriptTime(void) const
	{
		return MS2SEC(realClientTime);
	}
	void					AddBot(const char* botName);
	void					AlertBots(idPlayer* player, idVec3 alert_position);
// jmarshall end

private:
	idList<rvmBot*> registeredBots;
	const idDeclEntityDef* botItemTable;
#endif

private:
	char					clientGuids[ MAX_CLIENTS ][ CLIENT_GUID_LENGTH ];
	idList<mpBanInfo_t>		banList;
	bool					banListLoaded;
	bool					banListChanged;
// RAVEN END
};

//============================================================================

extern idGameLocal			gameLocal;
// RAVEN BEGIN
// jsinger: animationLib changed to a pointer to prevent it from allocating memory
//          before the unified allocator is initialized
extern idAnimManager		*animationLib;
// RAVEN END

//============================================================================

ID_INLINE void idGameLocal::WriteDecl( idBitMsg &msg, const idDecl *decl ) {
	assert( decl );
	if ( decl->IsImplicit() ) {
		gameLocal.Error( "WriteDecl: %s decl %s ( index %d ) is implicit", declManager->GetDeclNameFromType( decl->GetType() ), decl->GetName(), decl->Index() );
	}
	msg.WriteLong( decl->Index() );
}

ID_INLINE const idDecl* idGameLocal::ReadDecl( const idBitMsg &msg, declType_t type ) {
	int index = msg.ReadLong();
	const idDecl *decl = declManager->DeclByIndex( type, index );
	if ( !decl ) {
		gameLocal.Error( "ReadDecl: NULL %s decl at index %d", declManager->GetDeclNameFromType( type ), index );
	}
	if ( decl->IsImplicit() ) {
		gameLocal.Error( "ReadDecl: %s decl %s ( index %d ) is implicit", declManager->GetDeclNameFromType( type ), decl->GetName(), decl->Index() );
	}
	return decl;
}

ID_INLINE void idGameLocal::WriteDecl( idBitMsgDelta &msg, const idDecl *decl ) {
	assert( decl );
	if ( decl->IsImplicit() ) {
		gameLocal.Error( "WriteDecl: %s decl %s ( index %d ) is implicit", declManager->GetDeclNameFromType( decl->GetType() ), decl->GetName(), decl->Index() );
	}
	msg.WriteLong( decl->Index() );
}

ID_INLINE const idDecl* idGameLocal::ReadDecl( const idBitMsgDelta &msg, declType_t type ) {
	int index = msg.ReadLong();
	const idDecl *decl = declManager->DeclByIndex( type, index );
	if ( !decl ) {
		gameLocal.Error( "ReadDecl: NULL %s decl at index %d", declManager->GetDeclNameFromType( type ), index );
	}
	if ( decl->IsImplicit() ) {
		gameLocal.Error( "ReadDecl: %s decl %s ( index %d ) is implicit", declManager->GetDeclNameFromType( type ), decl->GetName(), decl->Index() );
	}
	return decl;
}

//============================================================================

class idGameError : public idException {
public:
	idGameError( const char *text ) : idException( text ) {}
};

//============================================================================


// content masks
#define	MASK_ALL					(-1)
#define	MASK_SOLID					(CONTENTS_SOLID)
#define	MASK_MONSTERSOLID			(CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_BODY)
#define	MASK_PLAYERSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY)
#define	MASK_DEADSOLID				(CONTENTS_SOLID|CONTENTS_PLAYERCLIP)
#define	MASK_WATER					(CONTENTS_WATER)
#define	MASK_OPAQUE					(CONTENTS_OPAQUE|CONTENTS_SIGHTCLIP)
#define	MASK_SHOT_RENDERMODEL		(CONTENTS_SOLID|CONTENTS_RENDERMODEL)
#define	MASK_SHOT_BOUNDINGBOX		(CONTENTS_SOLID|CONTENTS_BODY)
#define MASK_LARGESHOT_RENDERMODEL	(CONTENTS_SOLID|CONTENTS_RENDERMODEL|CONTENTS_LARGESHOTCLIP)
#define MASK_LARGESHOT_BOUNDINGBOX	(CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_LARGESHOTCLIP)
#define MASK_DMGSOLID				(CONTENTS_SOLID|CONTENTS_LARGESHOTCLIP)

// RAVEN BEGIN
// creed: added monster clip
#define	MASK_MONSTERCLIP			(CONTENTS_SOLID|CONTENTS_MONSTERCLIP)
// RAVEN END

const float DEFAULT_GRAVITY			= 1066.0f;
const float DEFAULT_GRAVITY_MP		= 800.0f;

#define DEFAULT_GRAVITY_STRING		"1066"
#define DEFAULT_MP_GRAVITY_STRING	"800"
const idVec3 DEFAULT_GRAVITY_VEC3( 0, 0, -DEFAULT_GRAVITY );

const int	CINEMATIC_SKIP_DELAY	= SEC2MS( 2.0f );

//============================================================================

#include "physics/Force.h"
#include "physics/Force_Constant.h"
#include "physics/Force_Drag.h"
#include "physics/Force_Field.h"
#include "physics/Force_Spring.h"
#include "physics/Physics.h"
#include "physics/Physics_Static.h"
#include "physics/Physics_StaticMulti.h"
#include "physics/Physics_Base.h"
#include "physics/Physics_Actor.h"
#include "physics/Physics_Monster.h"
#include "physics/Physics_Player.h"
#include "physics/Physics_Parametric.h"
#include "physics/Physics_RigidBody.h"
#include "physics/Physics_AF.h"
#include "physics/Physics_Particle.h"
#include "physics/Physics_VehicleMonster.h"

#include "vehicle/VehicleController.h"

#include "Entity.h"
#include "Game_Debug.h"
#include "IconManager.h"
#include "GameEdit.h"
#include "AF.h"
#include "IK.h"
#include "AFEntity.h"
#include "Misc.h"
#include "Actor.h"

// client entities
#include "client/ClientEntity.h"
#include "client/ClientEffect.h"
#include "client/ClientMoveable.h"
#include "client/ClientModel.h"
#include "client/ClientAFEntity.h"

#include "Weapon.h"

#include "script/ScriptFuncUtility.h"

#include "Light.h"
#include "WorldSpawn.h"
#include "Item.h"
#include "PlayerView.h"
// TTimo: moved AI.h up, can't do template instanciation on forward declared-classes
#include "ai/AI.h"
#include "Player.h"
#include "Mover.h"
// RAVEN BEGIN
// abahr:
#include "vehicle/VehicleParts.h"
#include "vehicle/Vehicle.h"
#include "SplineMover.h"
#include "TramGate.h"
#include "vehicle/VehicleDriver.h"
// RAVEN END
#include "Camera.h"
#include "Moveable.h"
#include "Target.h"
#include "Trigger.h"
#include "Sound.h"
#include "SecurityCamera.h"
#include "BrittleFracture.h"

// RAVEN BEGIN
// nmckenzie: Reduce dependencies.
#include "mp/CTF.h"
#include "mp/stats/StatManager.h"
#include "mp/Tourney.h"
#include "Instance.h"
// RAVEN END
#include "anim/Anim_Testmodel.h"

// RAVEN BEGIN
// jscott: for lip syncing
#include "LipSync.h"
// RAVEN END

#include "script/Script_Compiler.h"
#include "script/Script_Interpreter.h"
#include "script/Script_Thread.h"

#ifdef _XENON
#define PACIFIER_UPDATE session->PacifierUpdate()
#else
#define PACIFIER_UPDATE
#endif

// RAVEN BEGIN
// bdube: inlines
ID_INLINE rvClientEffect* idGameLocal::PlayEffect( const idDict& args, const char* effectName, const idVec3& origin, const idMat3& axis, bool loop, const idVec3& endOrigin, bool broadcast, effectCategory_t category, const idVec4& effectTint ) {
	return PlayEffect ( GetEffect ( args, effectName ), origin, axis, loop, endOrigin, broadcast, false, category, effectTint );
}

ID_INLINE bool idGameLocal::IsTeamGame( void ) const {
	return ( isMultiplayer && ( gameType == GAME_CTF || gameType == GAME_TDM || gameType == GAME_1F_CTF || gameType == GAME_ARENA_CTF || gameType == GAME_DEADZONE ) );
}

ID_INLINE int idGameLocal::GetNumMapEntities( void ) const {
	if ( mapFile == NULL ) {
		return -1;
	} else {
		return mapFile->GetNumEntities();
	}
}

ID_INLINE rvInstance* idGameLocal::GetInstance( int id ) {
	return instances[ id ];
}

ID_INLINE int idGameLocal::GetNumInstances( void ) {
	return instances.Num();
}

ID_INLINE void idGameLocal::ReceiveRemoteConsoleOutput( const char* output ) {
	if( isMultiplayer ) {
		mpGame.ReceiveRemoteConsoleOutput( output );
	}
}

// abahr:
template< class type >
type* idGameLocal::SpawnSafeEntityDef( const char* entityDefName, const idDict* additionalArgs ) {
	idEntity* entity = SpawnEntityDef( entityDefName, additionalArgs );
	if( !entity ) {
		return NULL;
	}
	
	if( !entity->IsType(type::GetClassType()) ) {
		entity->PostEventMS( &EV_Remove, 0 );
		return NULL;
	}
	
	return static_cast<type*>( entity );
}
// RAVEN END

template< class type >
ID_INLINE idEntityPtr<type>::idEntityPtr() {
	spawnId = 0;
}

template< class type >
ID_INLINE void idEntityPtr<type>::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( spawnId );
}

template< class type >
ID_INLINE void idEntityPtr<type>::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( spawnId );
}

template< class type >
ID_INLINE idEntityPtr<type> &idEntityPtr<type>::operator=( type *ent ) {
	if ( ent == NULL ) {
		spawnId = 0;
	} else {
		spawnId = ( gameLocal.spawnIds[ent->entityNumber] << GENTITYNUM_BITS ) | ent->entityNumber;
	}
	return *this;
}

template< class type >
ID_INLINE bool idEntityPtr<type>::SetSpawnId( int id ) {
	if ( id == spawnId ) {
		return false;
	}
	if ( ( id >> GENTITYNUM_BITS ) == gameLocal.spawnIds[ id & ( ( 1 << GENTITYNUM_BITS ) - 1 ) ] ) {
		spawnId = id;
		return true;
	}
	return false;
}

template< class type >
ID_INLINE bool idEntityPtr<type>::IsValid( void ) const {
	return ( gameLocal.spawnIds[ spawnId & ( ( 1 << GENTITYNUM_BITS ) - 1 ) ] == ( spawnId >> GENTITYNUM_BITS ) );
}

template< class type >
ID_INLINE type *idEntityPtr<type>::GetEntity( void ) const {
	int entityNum = spawnId & ( ( 1 << GENTITYNUM_BITS ) - 1 );
	if ( ( gameLocal.spawnIds[ entityNum ] == ( spawnId >> GENTITYNUM_BITS ) ) ) {
		return static_cast<type *>( gameLocal.entities[ entityNum ] );
	}
	return NULL;
}

template< class type >
ID_INLINE int idEntityPtr<type>::GetEntityNum( void ) const {
	return ( spawnId & ( ( 1 << GENTITYNUM_BITS ) - 1 ) );
}

// RAVEN BEGIN
// bdube: overloaded operator
template< class type >
ID_INLINE type * idEntityPtr<type>::operator->( void ) const {
	return GetEntity ( );
}

template< class type >
ID_INLINE idEntityPtr<type>::operator type * ( void ) const { 
	return GetEntity(); 
}
// RAVEN END

//k
#include "../raven/idlib/containers/ListGame.h"
#include "bots/Bot.h"

#ifdef _QUAKE4
extern idCVar harm_g_alwaysRun;
#if 1
extern idCVar harm_in_alwaysRun;
#define in_alwaysRun_GetBool() harm_in_alwaysRun.GetBool()
#else
#define in_alwaysRun_GetBool() cvarSystem->GetCVarBool("in_alwaysRun")
#endif
#define usercmd_buttons_and_BUTTON_RUN (harm_g_alwaysRun.GetBool() ? ((gameLocal.isMultiplayer && in_alwaysRun_GetBool()) || !(usercmd.buttons & BUTTON_RUN)) : (usercmd.buttons & BUTTON_RUN))
#define not_usercmd_buttons_and_BUTTON_RUN (!(usercmd_buttons_and_BUTTON_RUN))
#endif

#endif	/* !__GAME_LOCAL_H__ */
