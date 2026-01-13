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

#ifndef __GAME_LOCAL_H__
#define	__GAME_LOCAL_H__

#include "GameBase.h"

#include "idlib/containers/StrList.h"
#include "idlib/containers/LinkList.h"
#include "idlib/BitMsg.h"
#include "framework/Game.h"

#include "gamesys/SaveGame.h"
#include "physics/Clip.h"
#include "physics/Push.h"
#include "script/Script_Program.h"
#include "ai/AAS.h"
#include "anim/Anim.h"
#include "Pvs.h"
#include "MultiplayerGame.h"

#ifdef ID_DEBUG_UNINITIALIZED_MEMORY
// This is real evil but allows the code to inspect arbitrary class variables.
#define private		public
#define protected	public
#endif

const int MAX_SORT_ITERATIONS	= 7500; //COOP: added by stradex. Iterations per player by the server
const int MAX_SERVER_EVENTS_PER_FRAME = 15; //COOP: May the limit could be higher but shouldn't be necessary, I prefer a bit of desync over events overflow.
const int SERVER_EVENTS_QUEUE_SIZE = 512; //Added to avoid events overflow by server
const int SERVER_EVENT_NONE = -999; //Added to avoid events overflow by server
const int SERVER_EVENT_OVERFLOW_WAIT = 8; //How many frames to wait in case of server event overflow

/*
===============================================================================

	Local implementation of the public game interface.

===============================================================================
*/
class idDeclEntityDef;
class idRenderWorld;
class idSoundWorld;
class idUserInterface;

extern idRenderWorld *				gameRenderWorld;
extern idSoundWorld *				gameSoundWorld;

// classes used by idGameLocal
class idEntity;
class idActor;
class idPlayer;
class idCamera;
class idWorldspawn;
class idTestModel;
class idSmokeParticles;
class idEntityFx;
class idTypeInfo;
class idThread;
class idEditEntities;
class idLocationEntity;

//============================================================================
extern const int NUM_RENDER_PORTAL_BITS;

void gameError( const char *fmt, ... );

extern idRenderWorld *				gameRenderWorld;
extern idSoundWorld *				gameSoundWorld;

extern const int NUM_RENDER_PORTAL_BITS;
/*
===============================================================================

	Local implementation of the public game interface.

===============================================================================
*/

typedef struct entityState_s {
	int						entityNumber;
	int						entityCoopNumber; //added for coop by Stradex
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
	int						coopId; //added for coop by stradex
	int						event;
	int						time;
	int						paramsSize;
	byte					paramsBuf[MAX_EVENT_PARAM_SIZE];
	struct entityNetEvent_s	*next;
	struct entityNetEvent_s *prev;
} entityNetEvent_t;

typedef struct serverEvent_s { //added for coop to avoid events overflow 
	int							eventId;
	int							paramsSize;
	byte						paramsBuf[MAX_EVENT_PARAM_SIZE];
	bool						saveEvent;
	int							excludeClient;
	int							eventTime;
	idEntity*					eventEnt;
	bool						isEventType;
	bool						saveLastOnly; //added by stradex for coop
	struct entityNetEvent_s		*event;
}serverEvent_t;

typedef struct snapshotsort_context_s {
	int clientNum;
} snapshotsort_context_s;

enum {
	GAME_RELIABLE_MESSAGE_INIT_DECL_REMAP,
	GAME_RELIABLE_MESSAGE_REMAP_DECL,
	GAME_RELIABLE_MESSAGE_SPAWN_PLAYER,
	GAME_RELIABLE_MESSAGE_DELETE_ENT,
	GAME_RELIABLE_MESSAGE_CHAT,
	GAME_RELIABLE_MESSAGE_TCHAT,
	GAME_RELIABLE_MESSAGE_SOUND_EVENT,
	GAME_RELIABLE_MESSAGE_SOUND_INDEX,
	GAME_RELIABLE_MESSAGE_DB,
	GAME_RELIABLE_MESSAGE_KILL,
	GAME_RELIABLE_MESSAGE_DROPWEAPON,
	GAME_RELIABLE_MESSAGE_RESTART,
	GAME_RELIABLE_MESSAGE_SERVERINFO,
	GAME_RELIABLE_MESSAGE_TOURNEYLINE,
	GAME_RELIABLE_MESSAGE_CALLVOTE,
	GAME_RELIABLE_MESSAGE_CASTVOTE,
	GAME_RELIABLE_MESSAGE_STARTVOTE,
	GAME_RELIABLE_MESSAGE_UPDATEVOTE,
	GAME_RELIABLE_MESSAGE_PORTALSTATES,
	GAME_RELIABLE_MESSAGE_PORTAL,
	GAME_RELIABLE_MESSAGE_VCHAT,
	GAME_RELIABLE_MESSAGE_STARTSTATE,
	GAME_RELIABLE_MESSAGE_MENU,
	GAME_RELIABLE_MESSAGE_WARMUPTIME,
	GAME_RELIABLE_MESSAGE_EVENT,
	//New messages added for coop
	GAME_RELIABLE_MESSAGE_ADDCHECKPOINT,
	GAME_RELIABLE_MESSAGE_GOTOCHECKPOINT,
	GAME_RELIABLE_MESSAGE_GLOBALCHECKPOINT,
	GAME_RELIABLE_MESSAGE_NOCLIP,
	GAME_RELIABLE_MESSAGE_FADE, //For fadeTo, fadeIn, fadeOut FX in coop
	GAME_RELIABLE_MESSAGE_REMOVED_ENTITIES //To sync the entities that were deleted from the map for clients that join
};

const int GAME_RELIABLE_MESSAGE_SDK_CHECK = 126;

typedef enum {
	GAMESTATE_UNINITIALIZED,		// prior to Init being called
	GAMESTATE_NOMAP,				// no map loaded
	GAMESTATE_STARTUP,				// inside InitFromNewMap().  spawning map entities.
	GAMESTATE_ACTIVE,				// normal gameplay
	GAMESTATE_SHUTDOWN				// inside MapShutdown().  clearing memory.
} gameState_t;

typedef struct {
	idEntity	*ent;
	int			dist;
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
	idBlockAlloc<entityNetEvent_t,32>	eventAllocator;
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

	//added for coop by Stradex
	int						GetCoopId( void ) const { return coopId; }
	bool					SetCoopId( int id );
	bool					UpdateCoopId( void );
	type *					GetCoopEntity( void ) const;
	int						GetCoopEntityNum( void ) const;
	bool					forceCoopEntity; //EVIL HACK

private:
	int						spawnId;
	int						coopId; 	//added for coop by Stradex
};

//============================================================================

class idGameLocal : public idGame {
public:
	idDict					serverInfo;				// all the tunable parameters, like numclients, etc
	int						numClients;				// pulled from serverInfo and verified
	idDict					userInfo[MAX_CLIENTS];	// client specific settings
	usercmd_t				usercmds[MAX_CLIENTS];	// client input commands
	idDict					persistentPlayerInfo[MAX_CLIENTS];
	idDict					persistentPlayerInfoClientside; //added for Coop (pdas data mostly)
	idList<idDict*>			serverLevelInventory;	// Stores pdas, keys and power cells for clients that connect in middle of the game
	idEntity *				entities[MAX_GENTITIES];// index to entities
	int						spawnIds[MAX_GENTITIES];// for use in idEntityPtr
	int						firstFreeIndex;			// first free index in the entities array
	int						num_entities;			// current number <= MAX_GENTITIES
	idHashIndex				entityHash;				// hash table to quickly find entities by name
	idWorldspawn *			world;					// world entity
	idLinkList<idEntity>	spawnedEntities;		// all spawned entities
	idLinkList<idEntity>	activeEntities;			// all thinking entities (idEntity::thinkFlags != 0)
	int						numEntitiesToDeactivate;// number of entities that became inactive in current frame
	bool					sortPushers;			// true if active lists needs to be reordered to place pushers at the front
	bool					sortTeamMasters;		// true if active lists needs to be reordered to place physics team masters before their slaves
	idDict					persistentLevelInfo;	// contains args that are kept around between levels

	//start: stradex for coop netcode
	int						firstFreeCoopIndex;			// first free index in the entities array for coop
	idEntity*				coopentities[MAX_GENTITIES];	//For coop netcode only by Stradex
	idLinkList<idEntity>	coopSyncEntities;				// all net-synced (used by Coop only)
	int						coopIds[MAX_GENTITIES];			// for use in idEntityPtr in coop
	int						num_coopentities;				//for coop netcode only by stradex 

	int						firstFreeCsIndex;		// first free index in the entities array for clientsideEntities

	int						firstFreeTargetIndex;	// first free index in the targetentities array
	idEntity *				targetentities[MAX_GENTITIES];	//For coop netcode only by Stradex
	idEntity *				sortsnapshotentities[MAX_GENTITIES]; //for coop only to sort the priority of snapshot

	idEntity*				removeSyncEntities[MAX_GENTITIES];	//For coop netcode only by Stradex
	int						num_removeSyncEntities;


	idLinkList<idEntity>	serverPriorityEntities;			// coopSyncEnities but sort by snapshotPriority (used by Coop only)
	int						serverEventsCount;				//just to debug delete later
	int						clientEventsCount;				//just to debug, delete later
	serverEvent_t			serverOverflowEvents[SERVER_EVENTS_QUEUE_SIZE]; //To avoid server reliabe messages overflow
	void					addToServerEventOverFlowList(int eventId, const idBitMsg *msg, bool saveEvent, int excludeClient, int eventTime, idEntity* ent, bool saveLastOnly=false); //To avoid server reliabe messages overflow
	void					addToServerEventOverFlowList(entityNetEvent_t* event, int clientNum); //To avoid server reliabe messages overflow
	void					sendServerOverflowEvents( void ); //to send the overflow events that are in queue to avoid event overflow
	int						overflowEventCountdown; //FIXME: Not pretty way I I think

	bool					isRestartingMap;				//added for coop to fix a script bug after serverMapRestart
	//end: stradex for coop netcode

	// can be used to automatically effect every material in the world that references globalParms
	float					globalShaderParms[ MAX_GLOBAL_SHADER_PARMS ];

	idRandom				random;					// random number generator used throughout the game

	idProgram				program;				// currently loaded script and data space
	idThread *				frameCommandThread;

	idClip					clip;					// collision detection
	idPush					push;					// geometric pushing
	idPVS					pvs;					// potential visible set

	idTestModel *			testmodel;				// for development testing of models
	idEntityFx *			testFx;					// for development testing of fx

	idStr					sessionCommand;			// a target_sessionCommand can set this to return something to the session

	idMultiplayerGame		mpGame;					// handles rules for standard dm

	idSmokeParticles *		smokeParticles;			// global smoke trails
	idEditEntities *		editEntities;			// in game editing

	int						cinematicSkipTime;		// don't allow skipping cinemetics until this time has passed so player doesn't skip out accidently from a firefight
	int						cinematicStopTime;		// cinematics have several camera changes, so keep track of when we stop them so that we don't reset cinematicSkipTime unnecessarily
	int						cinematicMaxSkipTime;	// time to end cinematic when skipping.  there's a possibility of an infinite loop if the map isn't set up right.
	bool					inCinematic;			// game is playing cinematic (player controls frozen)
	bool					skipCinematic;

													// are kept up to date with changes to serverInfo
	int						framenum;
	int						previousTime;			// time in msec of last frame
	int						previousClientsideTime;	// time in msec of last frame (for clientsidecoop)
	int						time;					// in msec
	static const int		msec = USERCMD_MSEC;	// time since last update in milliseconds

	int						vacuumAreaNum;			// -1 if level doesn't have any outside areas

	gameType_t				gameType;
	bool					isMultiplayer;			// set if the game is run in multiplayer mode
	bool					isServer;				// set if the game is run for a dedicated or listen server
	bool					isClient;				// set if the game is run for a client
													// discriminates between the RunFrame path and the ClientPrediction path
													// NOTE: on a listen server, isClient is false
	int						localClientNum;			// number of the local client. MP: -1 on a dedicated
	idLinkList<idEntity>	snapshotEntities;		// entities from the last snapshot
	idLinkList<idEntity>	clientsideEntities;		// entities only present in the client machine that requires to think (added by Stradex)
	int						realClientTime;			// real client time
	int						clientsideTime;			// only for clients added by Stradex
	bool					isNewFrame;				// true if this is a new game frame, not a rerun due to prediction
	float					clientSmoothing;		// smoothing of other clients in the view
	int						entityDefBits;			// bits required to store an entity def number

	static const char *		sufaceTypeNames[ MAX_SURFACE_TYPES ];	// text names for surface types

	idEntityPtr<idEntity>	lastGUIEnt;				// last entity with a GUI, used by Cmd_NextGUI_f
	int						lastGUI;				// last GUI on the lastGUIEnt

	// ---------------------- Public idGame Interface -------------------

							idGameLocal();

	virtual void			Init( void );
	virtual void			Shutdown( void );
	virtual void			SetLocalClient( int clientNum );
	virtual void			ThrottleUserInfo( void );
	virtual const idDict *	SetUserInfo( int clientNum, const idDict &userInfo, bool isClient, bool canModify );
	virtual const idDict *	GetUserInfo( int clientNum );
	virtual void			SetServerInfo( const idDict &serverInfo );

	virtual const idDict &	GetPersistentPlayerInfo( int clientNum );
	virtual void			SetPersistentPlayerInfo( int clientNum, const idDict &playerInfo );
	virtual void			InitFromNewMap( const char *mapName, idRenderWorld *renderWorld, idSoundWorld *soundWorld, bool isServer, bool isClient, int randSeed );
	virtual bool			InitFromSaveGame( const char *mapName, idRenderWorld *renderWorld, idSoundWorld *soundWorld, idFile *saveGameFile );
	virtual void			SaveGame( idFile *saveGameFile );
	virtual void			MapShutdown( void );
	virtual void			CacheDictionaryMedia( const idDict *dict );
	virtual void			SpawnPlayer( int clientNum );
	virtual gameReturn_t	RunFrame( const usercmd_t *clientCmds );
	virtual bool			Draw( int clientNum );
	virtual escReply_t		HandleESC( idUserInterface **gui );
	virtual idUserInterface	*StartMenu( void );
	virtual const char *	HandleGuiCommands( const char *menuCommand );
	virtual void			HandleMainMenuCommands( const char *menuCommand, idUserInterface *gui );
	virtual allowReply_t	ServerAllowClient( int numClients, const char *IP, const char *guid, const char *password, char reason[MAX_STRING_CHARS] );
	virtual void			ServerClientConnect( int clientNum, const char *guid );
	virtual void			ServerClientBegin( int clientNum );
	virtual void			ServerClientDisconnect( int clientNum );
	virtual void			ServerWriteInitialReliableMessages( int clientNum );
	virtual void			ServerWriteSnapshot( int clientNum, int sequence, idBitMsg &msg, byte *clientInPVS, int numPVSClients );
	virtual bool			ServerApplySnapshot( int clientNum, int sequence );
	virtual void			ServerProcessReliableMessage( int clientNum, const idBitMsg &msg );
	virtual void			ClientReadSnapshot( int clientNum, int sequence, const int gameFrame, const int gameTime, const int dupeUsercmds, const int aheadOfServer, const idBitMsg &msg );
	virtual bool			ClientApplySnapshot( int clientNum, int sequence );
	virtual void			ClientProcessReliableMessage( int clientNum, const idBitMsg &msg );
	virtual gameReturn_t	ClientPrediction( int clientNum, const usercmd_t *clientCmds, bool lastPredictFrame );

	virtual void			GetClientStats( int clientNum, char *data, const int len );
	virtual void			SwitchTeam( int clientNum, int team );

	virtual bool			DownloadRequest( const char *IP, const char *guid, const char *paks, char urls[ MAX_STRING_CHARS ] );

	// ---------------------- Public idGameLocal Interface -------------------

	void					Printf( const char *fmt, ... ) const id_attribute((format(printf,2,3)));
	void					DPrintf( const char *fmt, ... ) const id_attribute((format(printf,2,3)));
	void					DebugPrintf( const char *fmt, ... ) const id_attribute((format(printf,2,3))); //to show in debug only
	void					Warning( const char *fmt, ... ) const id_attribute((format(printf,2,3)));
	void					DWarning( const char *fmt, ... ) const id_attribute((format(printf,2,3)));
	void					Error( const char *fmt, ... ) const id_attribute((format(printf,2,3)));

							// Initializes all map variables common to both save games and spawned games
	void					LoadMap( const char *mapName, int randseed );

	void					LocalMapRestart( void );
	void					MapRestart( void );
	static void				MapRestart_f( const idCmdArgs &args );
	bool					NextMap( void );	// returns wether serverinfo settings have been modified
	static void				NextMap_f( const idCmdArgs &args );

	idMapFile *				GetLevelMap( void );
	const char *			GetMapName( void ) const;

	int						NumAAS( void ) const;
	idAAS *					GetAAS( int num ) const;
	idAAS *					GetAAS( const char *name ) const;
	void					SetAASAreaState( const idBounds &bounds, const int areaContents, bool closed );
	aasHandle_t				AddAASObstacle( const idBounds &bounds );
	void					RemoveAASObstacle( const aasHandle_t handle );
	void					RemoveAllAASObstacles( void );

	bool					CheatsOk( bool requirePlayer = true );
	void					SetSkill( int value );
	gameState_t				GameState( void ) const;
	idEntity *				SpawnEntityType( const idTypeInfo &classdef, const idDict *args = NULL, bool bIsClientReadSnapshot = false );
	bool					SpawnEntityDef( const idDict &args, idEntity **ent = NULL, bool setDefaults = true , bool bIsClientReadSnapshot = false ); //bIsClientReadSnapshot added by Stradex for DEBUG
	int						GetSpawnId( const idEntity *ent ) const;
	int						GetCoopId( const idEntity *ent ) const; //added by Stradex for coop

	const idDeclEntityDef *	FindEntityDef( const char *name, bool makeDefault = true ) const;
	const idDict *			FindEntityDefDict( const char *name, bool makeDefault = true ) const;

	void					RegisterEntity( idEntity *ent );
	void					RegisterCoopEntity( idEntity *ent ); //added by Stradex for coop
	void					RegisterTargetEntity( idEntity *ent ); //added by Stradex for coop
	void					RegisterRemoveSyncEntity(idEntity* ent); //added by Stradex for coop
	void					UnregisterRemoveSyncEntity(idEntity* ent); //added by Stradex for coop
	int						CountMapSyncEntitiesRemoved() const;
	void					UnregisterEntity( idEntity *ent );
	void					UnregisterCoopEntity(idEntity* ent); //added by Stradex for coop
	void					UnregisterTargetEntity(idEntity* ent); //added by Stradex for coop
	void					UnregisterClientsideEntity(idEntity* ent); //added by Stradex for coop

	bool					SecureCheckIfEntityExists(idEntity* ent); //proper secure check about if entity exists, to avoid VERY RARE bug in coop (monorail respawn after dying).

	void					WriteRemovedEntitiesToEvent(idBitMsg& msg); // Coop
	void					ReadRemovedEntitiesFromEvent(const idBitMsg& msg); // Coop

	bool					RequirementMet( idEntity *activator, const idStr &requires, int removeItem );

	void					AlertAI( idEntity *ent );
	idActor *				GetAlertEntity( void );

	bool					InCoopPlayersPVS( idEntity *ent ) const; //added by Stradex for coop
	bool					InPlayerPVS( idEntity *ent ) const;
	bool					InPlayerConnectedArea( idEntity *ent ) const;

	void					SetClientCamera(idCamera* cam); // new cinematics netcode
	void					SetCameraCoop( idCamera *cam ); //Cinematics in coop testing
	void					SetCamera( idCamera *cam );
	idCamera *				GetCamera( void ) const;
	bool					SkipCinematic( void );
	void					CalcFov( float base_fov, float &fov_x, float &fov_y ) const;

	void					AddEntityToHash( const char *name, idEntity *ent );
	bool					RemoveEntityFromHash( const char *name, idEntity *ent );
	bool					EntityFromHashExists( const char *name );
	int						GetTargets( const idDict &args, idList< idEntityPtr<idEntity> > &list, const char *ref ) const;

							// returns the master entity of a trace.  for example, if the trace entity is the player's head, it will return the player.
	idEntity *				GetTraceEntity( const trace_t &trace ) const;

	static void				ArgCompletion_EntityName( const idCmdArgs &args, void(*callback)( const char *s ) );
	idEntity *				FindTraceEntity( idVec3 start, idVec3 end, const idTypeInfo &c, const idEntity *skip ) const;
	idEntity *				FindEntity( const char *name ) const;
	idEntity *				FindEntityUsingDef( idEntity *from, const char *match ) const;
	int						EntitiesWithinRadius( const idVec3 org, float radius, idEntity **entityList, int maxCount ) const;

	void					KillBox( idEntity *ent, bool catch_teleport = false );
	void					RadiusDamage( const idVec3 &origin, idEntity *inflictor, idEntity *attacker, idEntity *ignoreDamage, idEntity *ignorePush, const char *damageDefName, float dmgPower = 1.0f, bool clientsideDamage = false ); 
	void					RadiusPush( const idVec3 &origin, const float radius, const float push, const idEntity *inflictor, const idEntity *ignore, float inflictorScale, const bool quake );
	void					RadiusPushClipModel( const idVec3 &origin, const float push, const idClipModel *clipModel );

	void					ProjectDecal( const idVec3 &origin, const idVec3 &dir, float depth, bool parallel, float size, const char *material, float angle = 0 );
	void					BloodSplat( const idVec3 &origin, const idVec3 &dir, float size, const char *material );

	void					CallFrameCommand( idEntity *ent, const function_t *frameCommand );
	void					CallObjectFrameCommand( idEntity *ent, const char *frameCommand );

	const idVec3 &			GetGravity( void ) const;

	// added the following to assist licensees with merge issues
	int						GetFrameNum() const { return framenum; };
	int						GetTime() const { return time; };
	int						GetMSec() const { return msec; };

	int						GetNextClientNum( int current ) const;
	idPlayer *				GetClientByNum( int current ) const;
	idPlayer *				GetClientByName( const char *name ) const;
	idPlayer *				GetClientByCmdArgs( const idCmdArgs &args ) const;

	idPlayer *				GetLocalPlayer() const;
	idPlayer *				GetCoopPlayer() const; //added for Coop
	idEntity *				GetCoopPlayerScriptHack() const; //added for Coop

	void					SpreadLocations();
	idLocationEntity *		LocationForPoint( const idVec3 &point );	// May return NULL
	idEntity *				SelectInitialSpawnPoint( idPlayer *player );

	void					SetPortalState( qhandle_t portal, int blockingBits );
	void					SaveEntityNetworkEvent( const idEntity *ent, int event, const idBitMsg *msg , bool saveLastOnly=false); //COOP: added saveLastOnly
	void					ServerSendChatMessage( int to, const char *name, const char *text );
	int						ServerRemapDecl( int clientNum, declType_t type, int index );
	int						ClientRemapDecl( declType_t type, int index );

	void					SetGlobalMaterial( const idMaterial *mat );
	const idMaterial *		GetGlobalMaterial();

	void					SetGibTime( int _time ) { nextGibTime = _time; };
	int						GetGibTime() { return nextGibTime; };

	bool					NeedRestart();

	//specific coop stuff
	bool					isNPC(idEntity *ent ) const; //added for COOP hack
	const idDict &			CS_SavePersistentPlayerInfo( void );
	void					SaveGlobalInventory( idDict* item );
	void					LoadGlobalInventory( int clientNum );

	bool					firstClientToSpawn; //used in coop for dedicated server not starting scripts until a player joins
	bool					coopMapScriptLoad; //used in coop for dedicated server not starting scripts until a player joins
	spawnSpot_t				spPlayerStartSpot; //added for COOP
	idStaticList<spawnSpot_t, MAX_GENTITIES> spawnSpots; //public for coop
	idStaticList<idEntity *, MAX_GENTITIES> initialSpots; //public for coop
private:
	const static int		INITIAL_SPAWN_COUNT = 1;

	idStr					mapFileName;			// name of the map, empty string if no map loaded
	idMapFile *				mapFile;				// will be NULL during the game unless in-game editing is used
	bool					mapCycleLoaded;

	int						spawnCount;
	int						mapSpawnCount;			// it's handy to know which entities are part of the map

	int						coopCount;				//added by Stradex for coop entities
	int						mapCoopCount;			//added by Stradex for coop entities

	idLocationEntity **		locationEntities;		// for location names, etc

	idCamera *				camera;
	const idMaterial *		globalMaterial;			// for overriding everything

	idList<idAAS *>			aasList;				// area system
	idStrList				aasNames;

	idEntityPtr<idActor>	lastAIAlertEntity;
	int						lastAIAlertTime;

	idDict					spawnArgs;				// spawn args used during entity spawning  FIXME: shouldn't be necessary anymore

	pvsHandle_t				playerPVS;				// merged pvs of all players
	pvsHandle_t				playerConnectedAreas;	// all areas connected to any player area

	idVec3					gravity;				// global gravity vector
	gameState_t				gamestate;				// keeps track of whether we're spawning, shutting down, or normal gameplay
	bool					influenceActive;		// true when a phantasm is happening
	int						nextGibTime;

	idList<int>				clientDeclRemap[MAX_CLIENTS][DECL_MAX_TYPES];

	entityState_t *			clientEntityStates[MAX_CLIENTS][MAX_GENTITIES];
	int						clientPVS[MAX_CLIENTS][ENTITY_PVS_SIZE];
	snapshot_t *			clientSnapshots[MAX_CLIENTS];
	idBlockAlloc<entityState_t,256>entityStateAllocator;
	idBlockAlloc<snapshot_t,64>snapshotAllocator;

	idEventQueue			eventQueue;
	idEventQueue			savedEventQueue;

	int						currentInitialSpot;

	idDict					newInfo;

	idStrList				shakeSounds;

	byte					lagometer[ LAGO_IMG_HEIGHT ][ LAGO_IMG_WIDTH ][ 4 ];

	void					Clear( void );
							// returns true if the entity shouldn't be spawned at all in this game type or difficulty level
	bool					InhibitEntitySpawn( idDict &spawnArgs );
							// spawn entities from the map file
	void					SpawnMapEntities( void );
							// commons used by init, shutdown, and restart
	void					MapPopulate( void );
	void					MapClear( bool clearClients );

	pvsHandle_t				GetClientPVS( idPlayer *player, pvsType_t type );
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
	void					InitClientDeclRemap( int clientNum );
	void					ServerSendDeclRemapToClient( int clientNum, declType_t type, int index );
	void					FreeSnapshotsOlderThanSequence( int clientNum, int sequence );
	bool					ApplySnapshot( int clientNum, int sequence );
	void					WriteGameStateToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadGameStateFromSnapshot( const idBitMsgDelta &msg );
	void					NetworkEventWarning( const entityNetEvent_t *event, const char *fmt, ... ) id_attribute((format(printf,3,4)));
	void					ServerProcessEntityNetworkEventQueue( void );
	void					ClientProcessEntityNetworkEventQueue( void );
	void					ClientShowSnapshot( int clientNum ) const;
							// call after any change to serverInfo. Will update various quick-access flags
	void					UpdateServerInfoFlags( void );
	void					RandomizeInitialSpawns( void );
	static int				sortSpawnPoints( const void *ptr1, const void *ptr2 );

	void					DumpOggSounds( void );
	void					GetShakeSounds( const idDict *dict );

	virtual void			SelectTimeGroup( int timeGroup );
	virtual int				GetTimeGroupTime( int timeGroup );
	virtual void			GetBestGameType( const char* map, const char* gametype, char buf[ MAX_STRING_CHARS ] );

	void					Tokenize( idStrList &out, const char *in );

	void					UpdateLagometer( int aheadOfServer, int dupeUsercmds );

	virtual void			GetMapLoadingGUI(char gui[MAX_STRING_CHARS]);

	//STRADEX: COOP
	//Added by Stradex for Coop
	void					RunClientSideFrame(idPlayer* clientPlayer, const usercmd_t* clientCmds);
	void					ServerWriteSnapshotCoop(int clientNum, int sequence, idBitMsg& msg, byte* clientInPVS, int numPVSClients);
	void					ClientReadSnapshotCoop(int clientNum, int sequence, const int gameFrame, const int gameTime, const int dupeUsercmds, const int aheadOfServer, const idBitMsg& msg);
	bool					isSnapshotEntity(idEntity* ent);
	idEntity*				getEntityBySpawnId(int spawnId);
	//Quick sort snapshot entities start (thanks fluffy)
	void					snapshotsort_swap(idEntity* entities[], int lhs, int rhs);
	bool					snapshotsort_notInOrder(const snapshotsort_context_s& context, idEntity* lhs, idEntity* rhs);
	int						snapshotsort_partition(const snapshotsort_context_s& context, idEntity* entities[], int low, int high);
	void					snapshotsort(const snapshotsort_context_s& context, idEntity* entities[], int low, int high);
	//Quick sort snapshot entities ends
	bool					ProjectileCanDoRadiusDamage(idEntity* inflictor, idEntity* attacker, idEntity* victim);
	void					FixScriptsInMapRestart( void ); //hack
	void					FixNoDynamicInteractions( bool isLocalMapRestart); //hack
	idEntity*				CheckAndGetValidSpawnSpot(idEntity* spotEnt, int spotEntIndex); //hack
	void					SetEntityNetEventData(entityNetEvent_t* event, int eventId, const idBitMsg* msg, int eventTime);
	void					ProcessEntityReceiveEvent(idEntity* ent, idBitMsg &eventMsg, entityNetEvent_t* event, bool isClientEvent);
};

//============================================================================

extern idGameLocal			gameLocal;
extern idAnimManager		animationLib;

//============================================================================

class idGameError : public idException {
public:
	idGameError( const char *text ) : idException( text ) {}
};

//============================================================================

template< class type >
ID_INLINE idEntityPtr<type>::idEntityPtr() {
	spawnId = 0;
	coopId = 0;
	forceCoopEntity = false;
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
		coopId = 0;
	} else {
		spawnId = ( gameLocal.spawnIds[ent->entityNumber] << GENTITYNUM_BITS ) | ent->entityNumber;
		coopId = ( gameLocal.coopIds[ent->entityCoopNumber] << GENTITYNUM_BITS ) | ent->entityCoopNumber;
	}
	return *this;
}

template< class type >
ID_INLINE bool idEntityPtr<type>::SetSpawnId( int id ) {
	// the reason for this first check is unclear:
	// the function returning false may mean the spawnId is already set right, or the entity is missing
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

	if (forceCoopEntity) {
		return GetCoopEntity(); //evil stuff, forgive me god
	}

	int entityNum = spawnId & ( ( 1 << GENTITYNUM_BITS ) - 1 );
	if ( gameLocal.spawnIds[ entityNum ] == ( spawnId >> GENTITYNUM_BITS ) ) {
		return static_cast<type *>( gameLocal.entities[ entityNum ] );
	}

	return NULL;
}

template< class type >
ID_INLINE int idEntityPtr<type>::GetEntityNum( void ) const {
	return ( spawnId & ( ( 1 << GENTITYNUM_BITS ) - 1 ) );
}

/***************
Added by Stradex for coop
***************/

template< class type >
ID_INLINE bool idEntityPtr<type>::SetCoopId( int id ) {
	// the reason for this first check is unclear:
	// the function returning false may mean the spawnId is already set right, or the entity is missing
	if ( id == coopId ) {
		return false;
	}
	if ( ( id >> GENTITYNUM_BITS ) == gameLocal.coopIds[ id & ( ( 1 << GENTITYNUM_BITS ) - 1 ) ] ) {
		coopId = id;
		return true;
	}
	return false;
}

template< class type >
ID_INLINE type *idEntityPtr<type>::GetCoopEntity( void ) const {
	int entityNum = coopId & ( ( 1 << GENTITYNUM_BITS ) - 1 );
	if ( gameLocal.coopIds[ entityNum ] == ( coopId >> GENTITYNUM_BITS ) ) {
		return static_cast<type *>( gameLocal.coopentities[ entityNum ] );
	}
	return NULL;
}

template< class type >
ID_INLINE int idEntityPtr<type>::GetCoopEntityNum( void ) const {
	return ( coopId & ( ( 1 << GENTITYNUM_BITS ) - 1 ) );
}

//============================================================================

//
// these defines work for all startsounds from all entity types
// make sure to change script/doom_defs.script if you add any channels, or change their order
//
typedef enum {
	SND_CHANNEL_ANY = SCHANNEL_ANY,
	SND_CHANNEL_VOICE = SCHANNEL_ONE,
	SND_CHANNEL_VOICE2,
	SND_CHANNEL_BODY,
	SND_CHANNEL_BODY2,
	SND_CHANNEL_BODY3,
	SND_CHANNEL_WEAPON,
	SND_CHANNEL_ITEM,
	SND_CHANNEL_HEART,
	SND_CHANNEL_PDA,
	SND_CHANNEL_DEMONIC,
	SND_CHANNEL_RADIO,

	// internal use only.  not exposed to script or framecommands.
	SND_CHANNEL_AMBIENT,
	SND_CHANNEL_DAMAGE
} gameSoundChannel_t;

extern const float	DEFAULT_GRAVITY;
extern const idVec3	DEFAULT_GRAVITY_VEC3;
extern const int	CINEMATIC_SKIP_DELAY;

#endif	/* !__GAME_LOCAL_H__ */
