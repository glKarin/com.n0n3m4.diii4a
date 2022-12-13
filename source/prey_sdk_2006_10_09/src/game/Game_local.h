// Copyright (C) 2004 Id Software, Inc.
//

#ifndef __GAME_LOCAL_H__
#define	__GAME_LOCAL_H__

/*
===============================================================================

	Local implementation of the public game interface.

===============================================================================
*/

#define LAGO_IMG_WIDTH 64
#define LAGO_IMG_HEIGHT 64
#define LAGO_WIDTH	64
#define LAGO_HEIGHT	44
#define LAGO_MATERIAL	"textures/sfx/lagometer"
#define LAGO_IMAGE		"textures/sfx/lagometer.tga"

// if set to 1 the server sends the client PVS with snapshots and the client compares against what it sees
#ifndef ASYNC_WRITE_PVS
	#define ASYNC_WRITE_PVS 0
#endif

#ifdef ID_DEBUG_UNINITIALIZED_MEMORY
// This is real evil but allows the code to inspect arbitrary class variables.
#define private		public
#define protected	public
#endif

extern idRenderWorld *				gameRenderWorld;
extern idSoundWorld *				gameSoundWorld;

// HUMANHEAD pdm
#if INGAME_PROFILER_ENABLED
extern hhProfiler *					profiler;
#endif
// HUMANHEAD END

// the "gameversion" client command will print this plus compile date
#define	GAME_VERSION		"basePrey-1"

// classes used by idGameLocal
class idEntity;
class idActor;
class idPlayer;
class idCamera;
class idWorldspawn;
class idTestModel;
class idAAS;
class idAI;
class idSmokeParticles;
class idEntityFx;
class idTypeInfo;
class idProgram;
class idThread;
class idEditEntities;
class idLocationEntity;

#define	MAX_CLIENTS				32
#define	GENTITYNUM_BITS			12
#define	MAX_GENTITIES			(1<<GENTITYNUM_BITS)
//HUMANHEAD rww - client entities!
#define GENTITYNUM_BITS_PLUSCENT	13 //8192 allowable
#define	MAX_CENTITIES				MAX_GENTITIES //it doesn't have to be this, but whatever.
//END HUMANHEAD
#define	ENTITYNUM_NONE			(MAX_GENTITIES-1)
#define	ENTITYNUM_WORLD			(MAX_GENTITIES-2)
#define	ENTITYNUM_MAX_NORMAL	(MAX_GENTITIES-2)

//HUMANHEAD: aob - put here it can be used anywhere.  hhSafeEntity wouldn't compile otherwise.
#if GOLD
// mdl:  Disable if we're gold
#define HH_ASSERT( boolArg )
#else

#ifdef _DEBUG
#define HH_ASSERT( boolArg ) assert( (boolArg) )
#else
#define HH_ASSERT( boolArg ) if( !(boolArg) ) { gameLocal.Error("Assertion Failed: %s, File: %s, Line: %d\n", #boolArg, __FILE__, __LINE__); }
#endif

#endif // GOLD
//HUMANHEAD END

//============================================================================

#include "gamesys/Event.h"
#include "gamesys/Class.h"
#include "gamesys/SysCvar.h"
#include "gamesys/SysCmds.h"
#include "gamesys/SaveGame.h"
#include "gamesys/DebugGraph.h"

#include "script/Script_Program.h"

#include "anim/Anim.h"
// HUMANHEAD nla
#include "../prey/game_anim.h"
#include "../prey/game_animBlend.h"
#include "../prey/prey_animator.h"
// HUMANHEAD END

#include "ai/AAS.h"

#include "physics/Clip.h"
#include "physics/Push.h"

#include "Pvs.h"
#include "MultiplayerGame.h"

// HUMANHEAD
#include "../prey/prey_camerainterpolator.h"	// HUMANHEAD
#include "../prey/anim_baseanim.h"				// HUMANHEAD nla - For playing partial animations
#define NUM_AAS 3
// HUMANHEAD END

//============================================================================

const int MAX_GAME_MESSAGE_SIZE		= 8192;
const int MAX_ENTITY_STATE_SIZE		= 512;
const int ENTITY_PVS_SIZE			= ((MAX_GENTITIES+31)>>5);
const int NUM_RENDER_PORTAL_BITS	= idMath::BitsForInteger( PS_BLOCK_ALL );

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

//HUMANHEAD rww - for assistance in cleaning up garbage events for ents that no longer exist on client
//#define _HH_NET_EVENT_TYPE_VALIDATION
//HUMANHEAD END

typedef struct entityNetEvent_s {
	int						spawnId;
	int						event;
	int						time;
	int						paramsSize;
	byte					paramsBuf[MAX_EVENT_PARAM_SIZE];
#ifdef _HH_NET_EVENT_TYPE_VALIDATION //HUMANHEAD rww
	int						entTypeId;
#endif //HUMANHEAD END
	struct entityNetEvent_s	*next;
	struct entityNetEvent_s *prev;
} entityNetEvent_t;

enum {
	GAME_RELIABLE_MESSAGE_INIT_DECL_REMAP,
	GAME_RELIABLE_MESSAGE_REMAP_DECL,
	GAME_RELIABLE_MESSAGE_SPAWN_PLAYER,
	GAME_RELIABLE_MESSAGE_DELETE_ENT,
	GAME_RELIABLE_MESSAGE_CHAT,
	//HUMANHEAD PCF rww 05/10/06 - "fix" for server-localized join messages (this is dumb).
	GAME_RELIABLE_MESSAGE_SPECIAL,
	//HUMANHEAD END
	GAME_RELIABLE_MESSAGE_TCHAT,
	GAME_RELIABLE_MESSAGE_SOUND_EVENT,
	GAME_RELIABLE_MESSAGE_SOUND_INDEX,
	GAME_RELIABLE_MESSAGE_DB,
	GAME_RELIABLE_MESSAGE_DB_DEATH, //HUMANHEAD rww
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
	GAME_RELIABLE_MESSAGE_EVENT
};

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

#ifndef HUMANHEAD //HUMANHEAD: aob - changed prototype.  new version in HH section
	idEntityPtr<type> &		operator=( type *ent );
#endif

	// synchronize entity pointers over the network
	int						GetSpawnId( void ) const { return spawnId; }
	bool					SetSpawnId( int id );
	bool					UpdateSpawnId( void );

	bool					IsValid( void ) const;
	type *					GetEntity( void ) const;
	int						GetEntityNum( void ) const;

#ifdef HUMANHEAD //HUMANHEAD: aob
							idEntityPtr( const type* ent ) { Assign( ent ); }

	void					Clear();
	idEntityPtr<type> &		Assign( const idEntity *ent );
	idEntityPtr<type> &		Assign( const idEntityPtr<type> &ent );
	idEntityPtr<type> &		operator=( const idEntity *ent );
	idEntityPtr<type> &		operator=( const idEntityPtr<type> &ent );
	type *					operator->() const { return GetEntity(); }
	bool					IsEqualTo( const idEntity *ent ) const;
	bool					IsEqualTo( const idEntityPtr<type> &ent ) const;
	bool					operator==( const idEntity *ent ) const;
	bool					operator==( const idEntityPtr<type> &ent ) const;
	bool					operator!=( const idEntity *ent ) const;
	bool					operator!=( const idEntityPtr<type> &ent ) const;
#endif

private:
	int						spawnId;
};

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

//HUMANHEAD: aob - moved operator= code to this helper function
#ifdef HUMANHEAD
template< class type >
ID_INLINE void idEntityPtr<type>::Clear() {
	spawnId = 0;
}

template< class type >
ID_INLINE bool idEntityPtr<type>::IsEqualTo( const idEntity *ent ) const {
	return GetEntity() == ent;
}

template< class type >
ID_INLINE bool idEntityPtr<type>::IsEqualTo( const idEntityPtr<type> &ent ) const {
	return IsEqualTo( ent.GetEntity() );
}

template< class type >
ID_INLINE bool idEntityPtr<type>::operator==( const idEntity *ent ) const {
	return IsEqualTo( ent );
}

template< class type >
ID_INLINE bool idEntityPtr<type>::operator==( const idEntityPtr<type> &ent ) const {
	return IsEqualTo( ent );
}

template< class type >
ID_INLINE bool idEntityPtr<type>::operator!=( const idEntity *ent ) const {
	return !IsEqualTo( ent );
}

template< class type >
ID_INLINE bool idEntityPtr<type>::operator!=( const idEntityPtr<type> &ent ) const {
	return !IsEqualTo( ent );
}

template< class type >
ID_INLINE idEntityPtr<type> &idEntityPtr<type>::Assign( const idEntityPtr<type> &ent ) {
	return Assign( ent.GetEntity() );
}

template< class type >
ID_INLINE idEntityPtr<type> &idEntityPtr<type>::Assign( const idEntity *ent ) {
	if ( ent == NULL ) {
		spawnId = 0;
	} else {
		//HUMANHEAD rww - take cent bits into account
		spawnId = ( gameLocal.spawnIds[ent->entityNumber] << GENTITYNUM_BITS_PLUSCENT ) | ent->entityNumber;
	}
	return *this;
}

template< class type >
ID_INLINE idEntityPtr<type> &idEntityPtr<type>::operator=( const idEntityPtr<type> &ent ) {
	return Assign( ent );
}

template< class type >
ID_INLINE idEntityPtr<type> &idEntityPtr<type>::operator=( const idEntity *ent ) {
	return Assign( ent );
}
#else
template< class type >
ID_INLINE idEntityPtr<type> &idEntityPtr<type>::operator=( type *ent ) {
	if ( ent == NULL ) {
		spawnId = 0;
	} else {
		spawnId = ( gameLocal.spawnIds[ent->entityNumber] << GENTITYNUM_BITS ) | ent->entityNumber;
	}
	return *this;
}
#endif
// HUMANHEAD END

template< class type >
ID_INLINE bool idEntityPtr<type>::SetSpawnId( int id ) {
	if ( id == spawnId ) {
		return false;
	}
	//HUMANHEAD rww - take cent bits into account
	if ( ( id >> GENTITYNUM_BITS_PLUSCENT ) == gameLocal.spawnIds[ id & ( ( 1 << GENTITYNUM_BITS_PLUSCENT ) - 1 ) ] ) {
		spawnId = id;
		return true;
	}
	return false;
}

template< class type >
ID_INLINE bool idEntityPtr<type>::IsValid( void ) const {
	//HUMANHEAD rww - take cent bits into account
	return ( gameLocal.spawnIds[ spawnId & ( ( 1 << GENTITYNUM_BITS_PLUSCENT ) - 1 ) ] == ( spawnId >> GENTITYNUM_BITS_PLUSCENT ) );
}

template< class type >
ID_INLINE type *idEntityPtr<type>::GetEntity( void ) const {
	//HUMANHEAD rww - take cent bits into account
	int entityNum = spawnId & ( ( 1 << GENTITYNUM_BITS_PLUSCENT ) - 1 );
	if ( ( gameLocal.spawnIds[ entityNum ] == ( spawnId >> GENTITYNUM_BITS_PLUSCENT ) ) ) {
		return static_cast<type *>( gameLocal.entities[ entityNum ] );
	}
	return NULL;
}

template< class type >
ID_INLINE int idEntityPtr<type>::GetEntityNum( void ) const {
	//HUMANHEAD rww - take cent bits into account
	return ( spawnId & ( ( 1 << GENTITYNUM_BITS_PLUSCENT ) - 1 ) );
}

//============================================================================

class idGameLocal : public idGame {
public:
	idDict					serverInfo;				// all the tunable parameters, like numclients, etc
	int						numClients;				// pulled from serverInfo and verified
	idDict					userInfo[MAX_CLIENTS];	// client specific settings
	usercmd_t				usercmds[MAX_CLIENTS];	// client input commands
	idDict					persistentPlayerInfo[MAX_CLIENTS];
	//HUMANHEAD rww - introducing client entities.
	/*
	idEntity *				entities[MAX_GENTITIES];// index to entities
	int						spawnIds[MAX_GENTITIES];// for use in idEntityPtr
	*/
	idEntity *				entities[MAX_GENTITIES+MAX_CENTITIES];// index to entities
	int						spawnIds[MAX_GENTITIES+MAX_CENTITIES];// for use in idEntityPtr
	//END HUMANHEAD
	int						firstFreeIndex;			// first free index in the entities array
	int						num_entities;			// current number <= MAX_GENTITIES
	idHashIndex				entityHash;				// hash table to quickly find entities by name
	idWorldspawn *			world;					// world entity
	idLinkList<idEntity>	spawnedEntities;		// all spawned entities
	idLinkList<idEntity>	activeEntities;			// all thinking entities (idEntity::thinkFlags != 0)
	int						numEntitiesToDeactivate;// number of entities that became inactive in current frame
	bool					sortPushers;			// true if active lists needs to be reordered to place pushers at the front
	bool					sortTeamMasters;		// true if active lists needs to be reordered to place physics team masters before their slaves
	//HUMANHEAD rww
	bool					sortSnapshotPushers;	// true if snapshot lists needs to be reordered to place pushers at the front
	bool					sortSnapshotTeamMasters;// true if snapshot lists needs to be reordered to place physics team masters before their slaves
	//HUMANHEAD END
	idDict					persistentLevelInfo;	// contains args that are kept around between levels

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
	int						realClientTime;			// real client time
	bool					isNewFrame;				// true if this is a new game frame, not a rerun due to prediction
	float					clientSmoothing;		// smoothing of other clients in the view
	int						entityDefBits;			// bits required to store an entity def number

	static const char *		sufaceTypeNames[ MAX_SURFACE_TYPES ];	// text names for surface types

	idEntityPtr<idEntity>	lastGUIEnt;				// last entity with a GUI, used by Cmd_NextGUI_f
	int						lastGUI;				// last GUI on the lastGUIEnt

	int						timeRandom;				//HUMANHEAD rww - for time seeding

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
	virtual allowReply_t	ServerAllowClient( int numClients, const char *IP, const char *guid, const char *password, char reason[MAX_STRING_CHARS] );
	virtual void			ServerClientConnect( int clientNum );
	virtual void			ServerClientBegin( int clientNum );
	virtual void			ServerClientDisconnect( int clientNum );
	virtual void			ServerWriteInitialReliableMessages( int clientNum );
	virtual void			ServerWriteSnapshot( int clientNum, int sequence, idBitMsg &msg, byte *clientInPVS, int numPVSClients );
	virtual bool			ServerApplySnapshot( int clientNum, int sequence );
	virtual void			ServerProcessReliableMessage( int clientNum, const idBitMsg &msg );
	virtual void			ClientReadSnapshot( int clientNum, int sequence, const int gameFrame, const int gameTime, const int dupeUsercmds, const int aheadOfServer, const idBitMsg &msg );
	virtual bool			ClientApplySnapshot( int clientNum, int sequence );
	virtual void			ClientProcessReliableMessage( int clientNum, const idBitMsg &msg );
	virtual gameReturn_t	ClientPrediction( int clientNum, const usercmd_t *clientCmds );

	//HUMANHEAD rww
	virtual void			LogitechLCDUpdate(void) {}

	virtual void			ServerAddUnreliableSnapMessage(int clientNum, const idBitMsg &msg);
	virtual void			ClientReadUnreliableSnapMessages(int clientNum, const idBitMsg &msg);
	//HUMANHEAD END

	virtual void			GetClientStats( int clientNum, char *data, const int len );
	virtual void			SwitchTeam( int clientNum, int team );

	virtual bool			DownloadRequest( const char *IP, const char *guid, const char *paks, char urls[ MAX_STRING_CHARS ] );

	// ---------------------- Public idGameLocal Interface -------------------

	void					Printf( const char *fmt, ... ) const id_attribute((format(printf,2,3)));
	void					DPrintf( const char *fmt, ... ) const id_attribute((format(printf,2,3)));
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
//	void					SetSkill( int value );	// HUMANHEAD pdm: not used
	gameState_t				GameState( void ) const;
	idEntity *				SpawnEntityType( const idTypeInfo &classdef, const idDict *args = NULL, bool bIsClientReadSnapshot = false );
	//HUMANHEAD rww
	idEntity *				SpawnEntityTypeClient( const idTypeInfo &classdef, const idDict *args );
	//HUMANHEAD END

	//HUMANHEAD rww - added clientEntity, bIsClientReadSnapshot
	bool					SpawnEntityDef( const idDict &args, idEntity **ent = NULL, bool setDefaults = true, bool clientEntity = false, bool bIsClientReadSnapshot = false );
	//HUMANHEAD END

	int						GetSpawnId( const idEntity *ent ) const;

	const idDeclEntityDef *	FindEntityDef( const char *name, bool makeDefault = false ) const;
	const idDict *			FindEntityDefDict( const char *name, bool makeDefault = false ) const;

	void					RegisterEntity( idEntity *ent );
	void					UnregisterEntity( idEntity *ent );

	bool					RequirementMet( idEntity *activator, const idStr &requires, int removeItem );

	virtual					//HUMANHEAD jsh made virtual
	void					AlertAI( idEntity *ent );
	idActor *				GetAlertEntity( void );

	bool					InPlayerPVS( idEntity *ent ) const;
	bool					InPlayerConnectedArea( idEntity *ent ) const;

	void					SetCamera( idCamera *cam );
	idCamera *				GetCamera( void ) const;
	bool					SkipCinematic( void );
	void					CalcFov( float base_fov, float &fov_x, float &fov_y ) const;

	void					AddEntityToHash( const char *name, idEntity *ent );
	bool					RemoveEntityFromHash( const char *name, idEntity *ent );
	int						GetTargets( const idDict &args, idList< idEntityPtr<idEntity> > &list, const char *ref ) const;

							// returns the master entity of a trace.  for example, if the trace entity is the player's head, it will return the player.
	idEntity *				GetTraceEntity( const trace_t &trace ) const;

	static void				ArgCompletion_EntityName( const idCmdArgs &args, void(*callback)( const char *s ) );
	idEntity *				FindTraceEntity( idVec3 start, idVec3 end, const idTypeInfo &c, const idEntity *skip ) const;
	idEntity *				FindEntity( const char *name ) const;
	idEntity *				FindEntityUsingDef( idEntity *from, const char *match ) const;
	int						EntitiesWithinRadius( const idVec3 org, float radius, idEntity **entityList, int maxCount ) const;

	// HUMANHEAD pdm
	static void				ArgCompletion_ClassName( const idCmdArgs &args, void(*callback)( const char *s ) );
	static void				ArgCompletion_EntityOrClassName( const idCmdArgs &args, void(*callback)( const char *s ) );
	virtual bool			PlayerIsDeathwalking( void );
	virtual unsigned int	GetTimePlayed( void );
	virtual void			ClearTimePlayed( void );

	void					KillBox( idEntity *ent, bool catch_teleport = false );
	//HUMANHEAD PCF rww 05/15/06 - use a custom clipmask killbox (seperated due to pcf paranoia)
	void					KillBoxMasked( idEntity *ent, int clipMask, bool catch_teleport = false );
	//HUMANHEAD END
	virtual					// HUMANHEAD JRM - made virtual
	void					RadiusDamage( const idVec3 &origin, idEntity *inflictor, idEntity *attacker, idEntity *ignoreDamage, idEntity *ignorePush, const char *damageDefName, float dmgPower = 1.0f );
	virtual					// HUMANHEAD JRM - made virtual
	void					RadiusPush( const idVec3 &origin, const float radius, const float push, const idEntity *inflictor, const idEntity *ignore, float inflictorScale, const bool quake );
	void					RadiusPushClipModel( const idVec3 &origin, const float push, const idClipModel *clipModel );

	void					ProjectDecal( const idVec3 &origin, const idVec3 &dir, float depth, bool parallel, float size, const char *material, float angle = 0.0f );
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
	// HUMANHEAD nla
	bool 					AmLocalClient( idPlayer *player );
	bool					isCoop;
	bool					IsCooperative() const	{	return isCoop;	}
	bool					IsCompetitive() const	{	return isMultiplayer && !IsCooperative();	}

	// For saving objects that we don't want to have to spawn -mdl
	void					RegisterUniqueObject( idClass *object ); // Meant to be called once by constructor of object being registered
	void					UnregisterUniqueObject( idClass *object ); // Meant to be called by once destructor of object being registered

	virtual void			FocusGUICleanup(idUserInterface *gui); //HUMANHEAD rww

	// HUMANHEAD pdm: print game side memory statistics
	virtual void			PrintMemInfo( MemInfo_t *mi );

// HUMANHEAD pdm: Support for level appending
	bool					DeathwalkMapLoaded() const	{	return bShouldAppend;	}
#if DEATHWALK_AUTOLOAD
	bool					bShouldAppend;			// Taken from serverinfo
protected:
	idMapFile *				additionalMapFile;
	virtual void			SpawnAppendedMapEntities() {}
#endif
// HUMANHEAD END

// HUMANHEAD mdl
protected:
	// Special case, these must be virtual.  Allows hhGameLocal to save and restore it's data -mdl
	virtual void			Save( idSaveGame *savefile ) const { }
	virtual void			Restore( idRestoreGame *savefile ) { }

	// For saving objects that we don't want to have to spawn -mdl
	idList<idClass *>		uniqueObjects; // The object list
	idList<int>				uniqueObjRefs; // The reference count list

public:
// HUMANHEAD END

	void					SpreadLocations();
	idLocationEntity *		LocationForPoint( const idVec3 &point );	// May return NULL
	idEntity *				SelectInitialSpawnPoint( idPlayer *player );

	void					SetPortalState( qhandle_t portal, int blockingBits );
	void					SaveEntityNetworkEvent( const idEntity *ent, int event, const idBitMsg *msg );
	void					ServerSendChatMessage( int to, const char *name, const char *text );
	//HUMANHEAD PCF rww 05/10/06 - "fix" for server-localized join messages (this is dumb).
	typedef enum {
		SPECIALMSG_UNKNOWN = 0,
		SPECIALMSG_JOINED,
		//HUMANHEAD PCF rww 05/17/06 - localized vote checks (nested pcf's, hot!)
		SPECIALMSG_ALREADYRUNNINGMAP,
		SPECIALMSG_JUSTONE,
		//HUMANHEAD END
		SPECIALMSG_NUM
	} serverSpecialMsg_e;
	void					ServerSendSpecialMessage( serverSpecialMsg_e msgType, int to, const char *fromNonLoc, int numTextPtrs, const char **text );
	//HUMANHEAD END
	int						ServerRemapDecl( int clientNum, declType_t type, int index );
	int						ClientRemapDecl( declType_t type, int index );

	void					SetGlobalMaterial( const idMaterial *mat );
	const idMaterial *		GetGlobalMaterial();

	void					SetGibTime( int _time ) { nextGibTime = _time; };
	int						GetGibTime() { return nextGibTime; };

	bool					NeedRestart();

// HUMANHEAD
	void					SetSteamTime( int _time ) { nextSteamTime = _time; };
	int						GetSteamTime() { return nextSteamTime; };
	void					SpiritWalkSoundMode(bool active);
	void					DialogSoundMode(bool active);
// HUMANHEAD END

	//HUMANHEAD rww
	bool					EntInClientSnapshot(int entNum) { return (!isClient || (clientPVS[localClientNum][entNum >> 5] & ( 1 << ( entNum & 31 ) ))); }
	int						GetMapSpawnCount(void) { return mapSpawnCount; }

	void					GetAPUserInfo(idDict &dict, int clientNum);
	void					SpawnArtificialPlayer(void);
	//HUMANHEAD END

	//HUMANHEAD rww - keep track of layered spawning
	//if an object spawns another object in its spawn on mapload, this will completely destroy everything because
	//we need map-load-time entities to be in sync on client and server, and obviously the client cannot spawn them.
	//if this is really required and a postevent spawn cannot be done, then the spawnCount must be incremented to fake
	//the spawn on the client and the server should force the artificial entity to spawn above the map entity count.
	//(this does work as i have tested it, but it's an ugly hack that i would like to avoid)
#if !GOLD
	int						layeredSpawn;
	bool					spawnPopulating;
#endif
	//HUMANHEAD END

	//HUMANHEAD rww
	bool					logitechLCDEnabled;
	bool					logitechLCDDisplayAlt;
	DWORD					logitechLCDButtonsLast;
	int						logitechLCDUpdateTime;
	//HUMANHEAD END

protected:	// HUMANHEAD
	const static int		INITIAL_SPAWN_COUNT = 1;

	idStr					mapFileName;			// name of the map, empty string if no map loaded
	idMapFile *				mapFile;				// will be NULL during the game unless in-game editing is used
	bool					mapCycleLoaded;

	//HUMANHEAD rww
	class hhDDAManager*		ddaManager;
	//HUMANHEAD END

	//HUMANHEAD jsh
	class hhReactionHandler*	reactionHandler;
	//HUMANHEAD END

	int						spawnCount;
	int						mapSpawnCount;			// it's handy to know which entities are part of the map

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
	int						nextSteamTime;			// HUMANHEAD mdl:  Don't spawn too many steam entities at once

	idList<int>				clientDeclRemap[MAX_CLIENTS][DECL_MAX_TYPES];

	entityState_t *			clientEntityStates[MAX_CLIENTS][MAX_GENTITIES];
	int						clientPVS[MAX_CLIENTS][ENTITY_PVS_SIZE];
	snapshot_t *			clientSnapshots[MAX_CLIENTS];
	idBlockAlloc<entityState_t,256>entityStateAllocator;
	idBlockAlloc<snapshot_t,64>snapshotAllocator;

	idMsgQueue				unreliableSnapMsg[MAX_CLIENTS]; //HUMANHEAD rww - unreliable messages get appended to the snapshot message (since snapshots are unreliable)

	idEventQueue			eventQueue;
	idEventQueue			savedEventQueue;

	idStaticList<spawnSpot_t, MAX_GENTITIES> spawnSpots;
	idStaticList<idEntity *, MAX_GENTITIES> initialSpots;
	int						currentInitialSpot;

	idDict					newInfo;

	idStrList				shakeSounds;

	byte					lagometer[ LAGO_IMG_HEIGHT ][ LAGO_IMG_WIDTH ][ 4 ];

	// HUMANHEAD mdl:  Play time
	unsigned int			playTime; // Total play time, not including current session
	unsigned int			playTimeStart; // -1 if not playing, otherwise time when current sesion started
	// HUMANHEAD END

	void					Clear( void );
							// returns true if the entity shouldn't be spawned at all in this game type or difficulty level
	virtual					// HUMANHEAD mdl: Made virtual
	bool					InhibitEntitySpawn( idDict &spawnArgs );
							// spawn entities from the map file
	virtual	// HUMANHEAD JRM - made virtual
	void					SpawnMapEntities( void );
							// commons used by init, shutdown, and restart
	void					MapPopulate( void );
	void					MapClear( bool clearClients );

	pvsHandle_t				GetClientPVS( idPlayer *player, pvsType_t type );
	void					SetupPlayerPVS( void );
	void					FreePlayerPVS( void );
	void					UpdateGravity( void );
	void					SortActiveEntityList( void );
	//HUMANHEAD
	void					SortSnapshotEntityList( void );
	void					RegisterLocationsWithSoundWorld();
	//HUMANHEAD END
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

	void					SelectTimeGroup( int timeGroup );
	int						GetTimeGroupTime( int timeGroup );
	idStr					GetBestGameType( const char* map, const char* gametype );

	void					Tokenize( idStrList &out, const char *in );

	void					UpdateLagometer( int aheadOfServer, int dupeUsercmds );
};

//============================================================================

// HUMANHEAD pdm
// HUMANHEAD mdl:  Commented out the old gameLocal because it was messing up tagging
//#ifdef HUMANHEAD
	#include "../prey/prey_game.h"
	extern hhGameLocal			gameLocal;
//#else	// HUMANHEAD
//extern idGameLocal			gameLocal;
//#endif	// HUMANHEAD

extern idAnimManager		animationLib;

//============================================================================

class idGameError : public idException {
public:
	idGameError( const char *text ) : idException( text ) {}
};

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
	SND_CHANNEL_LANDING,	// HUMANHEAD pdm: needed seperate channel for landing
	SND_CHANNEL_RADIO,

//HUMANHEAD: aob
	SND_CHANNEL_WALLWALK,
	SND_CHANNEL_WALLWALK2,
	SND_CHANNEL_SPIRITWALK,
	SND_CHANNEL_MISC1,
	SND_CHANNEL_MISC2,
	SND_CHANNEL_MISC3,
	SND_CHANNEL_MISC4,
	SND_CHANNEL_MISC5,
	SND_CHANNEL_IDLE,
	SND_CHANNEL_DYING,
	SND_CHANNEL_THRUSTERS,
	SND_CHANNEL_DOCKED,
	SND_CHANNEL_TRACTORBEAM,
	SND_CHANNEL_RECHARGE,
//HUMANHEAD END

	// internal use only.  not exposed to script or framecommands.
	SND_CHANNEL_AMBIENT,
	SND_CHANNEL_DAMAGE
} gameSoundChannel_t;

// content masks
#ifdef HUMANHEAD
#define	MASK_ALL					(-1)
#define	MASK_SOLID					(CONTENTS_SOLID|CONTENTS_FORCEFIELD)
#define	MASK_MONSTERSOLID			(CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_BODY|CONTENTS_FORCEFIELD|CONTENTS_HUNTERCLIP)
#define	MASK_PLAYERSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY|CONTENTS_FORCEFIELD)
#define	MASK_DEADSOLID				(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_FORCEFIELD|CONTENTS_CORPSE)
#define MASK_SPECTATOR				(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_CORPSE)
#define	MASK_WATER					(CONTENTS_WATER)
#define	MASK_OPAQUE					(CONTENTS_OPAQUE) // HUMANHEAD:  Used to be MASK_SOLID for us
#define	MASK_SPIRITPLAYER			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY|CONTENTS_SPIRITBRIDGE)						// HUMANHEAD:  Identical to playersolid, except for forcefield and spiritbridge
#define MASK_VISIBILITY				(CONTENTS_OPAQUE|CONTENTS_RENDERMODEL|CONTENTS_BODY)
#define MASK_TRACTORBEAM			(CONTENTS_SOLID|CONTENTS_RENDERMODEL|CONTENTS_BODY|CONTENTS_FORCEFIELD|CONTENTS_SHOOTABLE|CONTENTS_CORPSE)
#define	MASK_SHOT_RENDERMODEL		(CONTENTS_SOLID|CONTENTS_RENDERMODEL|CONTENTS_SHOOTABLE|CONTENTS_CORPSE|CONTENTS_WATER)
#define	MASK_SHOT_BOUNDINGBOX		(CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_SHOOTABLE|CONTENTS_CORPSE|CONTENTS_WATER)
#define	MASK_SHOTANDPROJECTILES		(MASK_SHOT_RENDERMODEL|CONTENTS_PROJECTILE)														// for shooting projectiles with projectiles
#define MASK_SPIRITARROW			(CONTENTS_SOLID|CONTENTS_RENDERMODEL|CONTENTS_CORPSE|CONTENTS_WATER|CONTENTS_SHOOTABLEBYARROW)	// No forcefield (forcefields/spiritsecrets)
#else	// HUMANHEAD END
#define	MASK_ALL					(-1)
#define	MASK_SOLID					(CONTENTS_SOLID)
#define	MASK_MONSTERSOLID			(CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_BODY)
#define	MASK_PLAYERSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY)
#define	MASK_DEADSOLID				(CONTENTS_SOLID|CONTENTS_PLAYERCLIP)
#define	MASK_WATER					(CONTENTS_WATER)
#define	MASK_OPAQUE					(CONTENTS_OPAQUE)
#define	MASK_SHOT_RENDERMODEL		(CONTENTS_SOLID|CONTENTS_RENDERMODEL)
#define	MASK_SHOT_BOUNDINGBOX		(CONTENTS_SOLID|CONTENTS_BODY)
#endif	// HUMANHEAD

const float DEFAULT_GRAVITY			= 1066.0f;
#define DEFAULT_GRAVITY_STRING		"1066"
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

//HUMANHEAD
#include "physics/Physics_PreyPlayer.h"			// HUMANHEAD
#include "../Prey/Physics_PreyParametric.h"		// HUMANHEAD nla
#include "../Prey/Physics_PreyAI.h"				// HUMANHEAD jrm
#include "../Prey/physics_simple.h"				// HUMANHEAD aob
#include "../prey/game_woundmanager.h"			// HUMANHEAD: aob - must be before Entity.h
//HUMANHEAD END

#include "SmokeParticles.h"

#include "Entity.h"

//HUMANHEAD
//#include "../prey/game_renderentity.h"			// should be right after idRenderEntity.h
#include "../prey/game_animatedentity.h"		// should be right after entity.h
//HUMANHEAD END

#include "GameEdit.h"
#include "AF.h"
#include "IK.h"
#include "AFEntity.h"
#include "Misc.h"

// HUMANHEAD
#include "ai/aas_Local.h"						// HUMANHEAD nla
// HUMANHEAD END

#include "Actor.h"
#include "Projectile.h"

//HUMANHEAD:
#include "../prey/game_fxinfo.h"				// HUMANHEAD
#include "../prey/particles_particles.h"
#include "../prey/game_spherepart.h"			//HUMANHEAD: aob
#include "../prey/physics_delta.h"				//HUMANHEAD: aob
#include "../prey/game_animDriven.h"			//HUMANHEAD: aob
#include "../prey/prey_projectile.h"			//HUMANHEAD: aob
#include "../prey/prey_projectileautocannon.h"	//HUMANHEAD: aob
#include "../prey/prey_projectilerifle.h"		//HUMANHEAD: aob
#include "../prey/prey_projectilesoulcannon.h"		//HUMANHEAD: aob
#include "../prey/prey_projectiletracking.h"		//HUMANHEAD: aob
#include "../prey/prey_projectilerocketlauncher.h"	//HUMANHEAD: aob
#include "../prey/prey_projectilehiderweapon.h"		//HUMANHEAD: aob
#include "../prey/prey_projectilespiritarrow.h"		//HUMANHEAD: aob
#include "../prey/prey_projectilecrawlergrenade.h"	//HUMANHEAD: aob
#include "../prey/prey_projectileshuttle.h"			//HUMANHEAD: aob
#include "../prey/prey_projectilemine.h"		//HUMANHEAD: aob
#include "../prey/prey_projectilebugtrigger.h"			//HUMANHEAD: jsh
#include "../prey/prey_projectilebug.h"			//HUMANHEAD: jsh
#include "../prey/prey_projectilecocoon.h"			//HUMANHEAD: jsh
#include "../prey/prey_projectilegasbagpod.h"		//HUMANHEAD: mdl
#include "../prey/prey_projectilebounce.h"	//HUMANHEAD: aob
#include "../prey/prey_projectiletrigger.h"			//HUMANHEAD: jsh
#include "../prey/prey_projectilefreezer.h"			//HUMANHEAD: bjk
#include "../prey/prey_projectilewrench.h"			//HUMANHEAD: bjk
#include "item.h"								//HUMANHEAD: aob - must be above weapon.h
#include "../prey/game_dda.h"
#include "../prey/game_utils.h"					//HUMANHEAD: aob
#include "../prey/prey_soundleadincontroller.h" //HUMANHEAD: aob
//HUMANHEAD END

#include "Weapon.h"
#include "Light.h"
#include "WorldSpawn.h"
// HUMANHEAD AOB: Moved up above weapon
#include "PlayerView.h"
#include "PlayerIcon.h"

// HUMANHEAD
#include "../prey/game_light.h"					// HUMANHEAD: aob
#include "../prey/game_playerview.h"
#include "../prey/prey_firecontroller.h"		// HUMANHEAD: aob
#include "../prey/prey_weaponfirecontroller.h"	// HUMANHEAD: aob
#include "../prey/prey_vehiclefirecontroller.h"	// HUMANHEAD: aob
#include "../prey/prey_baseweapons.h"
#include "../prey/prey_items.h"					// HUMANHEAD: aob
#include "../prey/game_hand.h"					// HUMANHEAD nla - must be before guihand
#include "../prey/game_guihand.h"				// HUMANHEAD nla - must be before Player
#include "../prey/game_handcontrol.h"			// HUMANHEAD pdm - must be before Player
#include "../prey/game_weaponhandstate.h"
// HUMANHEAD END

#include "Player.h"
#include "../prey/physics_vehicle.h"			// HUMANHEAD pdm - must be before game_vehicle.h
#include "../prey/force_converge.h"				// HUMANHEAD pdm - must be before game_vehicle.h
#include "../prey/game_vehicle.h"				// HUMANHEAD pdm - must be before game_player.h
#include "../prey/game_dock.h"					// HUMANHEAD
#include "../prey/game_shuttle.h"				// HUMANHEAD pdm - must be after game_vehicle.h
#include "../prey/game_player.h"				// HUMANHEAD nla 
#include "Mover.h"
#include "Camera.h"
#include "Moveable.h"
#include "Target.h"
#include "Trigger.h"
#include "Sound.h"
#include "../prey/prey_sound.h"//HUMANHEAD: aob
#include "../prey/prey_spiritproxy.h"			// HUMANHEAD
#include "Fx.h"
#include "../prey/game_entityfx.h"				// HUMANHEAD
#include "SecurityCamera.h"
#include "BrittleFracture.h"

#include "ai/AI.h"
#include "anim/Anim_Testmodel.h"

#include "script/Script_Compiler.h"
#include "script/Script_Interpreter.h"
#include "script/Script_Thread.h"

//HUMANHEAD: aob - must be after Script_Thread.h
#include "../prey/prey_script_thread.h"
//HUMANHEAD END

#endif	/* !__GAME_LOCAL_H__ */
