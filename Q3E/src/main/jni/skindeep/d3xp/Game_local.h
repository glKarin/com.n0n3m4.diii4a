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

#include "bc_vomanager.h" //BC





#ifdef ID_DEBUG_UNINITIALIZED_MEMORY
// This is real evil but allows the code to inspect arbitrary class variables.
#define private		public
#define protected	public
#endif

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
class idFeedAlertWindow;

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


const int MAX_SUBTITLES = 3; //max subtitles onscreen at a given time.
struct idSubtitleItem {
	idStr	speakerName;		// name of the speaker
	idStr	displayText;		// what text to show.
	int		startTime;			// time of subtitle creation (for transition/fade in)
	int		expirationTime;		// what time to make the subtitle disappear.	
	bool	transitioned;		// item has finished transitioning
	float   rectHeight;			// pre calculated rect size
};


//for the event log.
struct eventlog_t
{
	idStr	name;			//the displaystring for the event.
	int		timestamp;		//when the event happened.
	idVec3	position;		//where the event happened.
	int		eventType;		//what type of event is this.
};


#define SOUNDCLASS_ALL			0
#define SOUNDCLASS_BAFFLERS		1
#define SOUNDCLASS_FOOTSTEPS	2
#define SOUNDCLASS_SPACE		3


#define CARRYFROB_INDEX			-777	//special frobindex when player frobs the thing they're holding.
#define PEEKFROB_INDEX			-760	//special frobindex when frobbing while peeking (for exiting cryopod, vents)
#define JOCKEYFROB_INDEX		-750	//special frobindex when jockeying frobs something.


#define VOICEPRINT_A		0
#define VOICEPRINT_B		1
#define VOICEPRINT_BOSS		2



#define ROOMLABELCOUNT 8


const idVec3 ENEMYCOLOR = idVec3(1, 0, 0);
const idVec3 FRIENDLYCOLOR = idVec3(0, .75f, 1);


const int LASTHEALTH_MAXTIME = 500; //when taking damage, how long to display on UI the amount of damage last received.


//bc AI state enums. Make sure this retains parity with the enum list in the monster gunner script file.
enum
{
	AISTATE_IDLE,			//0
	AISTATE_SUSPICIOUS,		//1
	AISTATE_COMBAT,			//2
	AISTATE_COMBATOBSERVE,	//3
	AISTATE_COMBATSTALK,	//4
	AISTATE_SEARCHING,		//5
	AISTATE_OVERWATCH,		//6
	AISTATE_VICTORY,		//7
	AISTATE_STUNNED,		//8
	AISTATE_JOCKEYED,		//9

	AISTATE_SPACEFLAIL,
};



enum
{
	BAFFLE_NONE,
	BAFFLE_CAMOUFLAGED,
	BAFFLE_MUTE
};

enum
{
	TEAM_FRIENDLY,	//0
	TEAM_ENEMY,		//1
	TEAM_NEUTRAL	//2
};

enum
{
	SPEWTYPE_PEPPER, //0
	SPEWTYPE_DEODORANT //1
};

enum
{
	JOCKATKTYPE_NONE,
	JOCKATKTYPE_KILLENTITY,
	JOCKATKTYPE_WALLSMASH,
};

enum
{
	BOT_REPAIRBOT,
	BOT_SECURITYBOT
};

enum { CONF_VENT, CONF_HIDETRIGGER };

enum
{
	ACROTYPE_NONE,				//0
	ACROTYPE_SPLITS,			//1
	//ACROTYPE_SPLITS_DOWN,		//2 // SW 5th May 2025: We haven't been officially able to flip upside down in splits for a long time and it's high time we removed it for good. I'm keeping the enum values the same just in case, though.
	ACROTYPE_CEILINGHIDE = 3,	//3
	ACROTYPE_CARGOHIDE
};

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
#ifdef CTF
	int			team;
#endif
} spawnSpot_t;

typedef struct {	
	int			distance;
	idVec3		position;
} vecSpot_t;


//BC
enum
{
	NOISE_LOWPRIORITY = 0,
	NOISE_HIGHPRIORITY,
	NOISE_COMBATPRIORITY //immediately go to combat mode.
};

enum
{
	INTERESTROLE_INVESTIGATE,
	INTERESTROLE_OVERWATCH
};

enum
{
	INTEREST_LOWPRIORITY = 1,
	INTEREST_HIGHPRIORITY = 5,
	INTEREST_COMBATPRIORITY = 10
};

enum
{
	SYS_NONE,			//0
	SYS_TRASHCHUTES,	//1
	SYS_AIRLOCKS,		//2
	SYS_VENTS,			//3
	SYS_WINDOWS			//4
};

enum
{
	EL_NONE,
	EL_DEATH,
	EL_DESTROYED,
	EL_DAMAGE,
	EL_HEAL,
	EL_INTERESTPOINT,
	EL_ALERT
};

const int ENERGYSHIELD_AMOUNTPERPIP = 100;
const int EXPENSIVEOBSERVATION_TIMEDELAY = 1000;

const int MAXMILESTONES = 3;


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
	void					Save( idSaveGame *savefile ) const;	// blendo eric: savegame pass 1
	void					Restore( idRestoreGame *savefile );					// unarchives object from save game file

	idEntityPtr<type> &		operator=( type *ent );
	bool					operator==(idEntityPtr<type>& other) const;
#if !defined(_MSC_VER)
	bool					operator==(const idEntityPtr<type>& other) const;
#endif

	// synchronize entity pointers over the network
	int						GetSpawnId( void ) const { return spawnId; }
	bool					SetSpawnId( int id );
	bool					UpdateSpawnId( void );

	bool					IsValid( void ) const;
	type *					GetEntity( void ) const;
	int						GetEntityNum( void ) const;

private:
	int						spawnId;
};

#ifdef _D3XP
struct timeState_t {
	int					time;
	int					previousTime;
	int					msec;
	int					framenum;
	int					realClientTime;

	void				Set( int t, int pt, int ms, int f, int rct )		{ time = t; previousTime = pt; msec = ms; framenum = f; realClientTime = rct; };
	void				Get( int& t, int& pt, int& ms, int& f, int& rct )	{ t = time; pt = previousTime; ms = msec; f = framenum; rct = realClientTime; };
	void				Save( idSaveGame *savefile ) const	{ savefile->WriteInt( time ); savefile->WriteInt( previousTime ); savefile->WriteInt( msec ); savefile->WriteInt( framenum ); savefile->WriteInt( realClientTime ); }
	void				Restore( idRestoreGame *savefile )	{ savefile->ReadInt( time ); savefile->ReadInt( previousTime ); savefile->ReadInt( msec ); savefile->ReadInt( framenum ); savefile->ReadInt( realClientTime ); }
	void				Increment()											{ framenum++; previousTime = time; time += msec; realClientTime = time; };
};

enum slowmoState_t {
	SLOWMO_STATE_OFF,
	SLOWMO_STATE_RAMPUP,
	SLOWMO_STATE_ON,
	SLOWMO_STATE_RAMPDOWN
};
#endif

//============================================================================

class idGameLocal : public idGame {
public:
	idDict					serverInfo;				// all the tunable parameters, like numclients, etc
	int						numClients;				// pulled from serverInfo and verified
	idDict					userInfo[MAX_CLIENTS];	// client specific settings
	usercmd_t				usercmds[MAX_CLIENTS];	// client input commands
	idDict					persistentPlayerInfo[MAX_CLIENTS];
	idEntity *				entities[MAX_GENTITIES];// index to entities
	int						spawnIds[MAX_GENTITIES];// for use in idEntityPtr
	int						firstFreeIndex;			// first free index in the entities array
	int						num_entities;			// current number <= MAX_GENTITIES
	idHashIndex				entityHash;				// hash table to quickly find entities by name
	idWorldspawn *			world;					// world entity
	idLinkList<idEntity>	spawnedEntities;		// all spawned entities
	idLinkList<idEntity>	activeEntities;			// all thinking entities (idEntity::thinkFlags != 0)
	
	//DOOM BFG
	idLinkList<idEntity>	aimAssistEntities;		// all aim Assist entities
	
	int						numEntitiesToDeactivate;// number of entities that became inactive in current frame
	bool					sortPushers;			// true if active lists needs to be reordered to place pushers at the front
	bool					sortTeamMasters;		// true if active lists needs to be reordered to place physics team masters before their slaves
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
	int						msec;					// time since last update in milliseconds

	int						hudTime;				// SW: We want the HUD to be able to update even if time is frozen, so it has a separate time tracker

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

#ifdef _D3XP
	idEntityPtr<idEntity>	portalSkyEnt;
	bool					portalSkyActive;

	void					SetPortalSkyEnt( idEntity *ent );
	bool					IsPortalSkyAcive();

	timeState_t				fast;
	timeState_t				slow;

	slowmoState_t			slowmoState;
	float					slowmoMsec;
	int						soundSlowmoHandle;
	bool					soundSlowmoActive;

	bool					quickSlowmoReset;

	virtual void			SelectTimeGroup( int timeGroup );
	virtual int				GetTimeGroupTime( int timeGroup );

	virtual void			GetBestGameType( const char* map, const char* gametype, char buf[ MAX_STRING_CHARS ] );

	void					ComputeSlowMsec();
	void					RunTimeGroup2();

	void					ResetSlowTimeVars();
	void					QuickSlowmoReset();

	bool					NeedRestart();
#endif

	void					Tokenize( idStrList &out, const char *in );

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

	virtual void				GetMapLoadingGUI( char gui[ MAX_STRING_CHARS ] );
	virtual idStr				GetDLLVersionString();

	// ---------------------- Public idGameLocal Interface -------------------

	void					Printf( const char *fmt, ... ) const id_attribute((format(printf,2,3)));
	void					DPrintf( const char *fmt, ... ) const id_attribute((format(printf,2,3)));
	void					Warning( const char *fmt, ... ) const id_attribute((format(printf,2,3)));
	void					DWarning( const char *fmt, ... ) const id_attribute((format(printf,2,3)));
	void					Error( const char *fmt, ... ) const id_attribute((format(printf,2,3)));

							// Initializes all map variables common to both save games and spawned games
	void					LoadMap( const char *mapName, int randseed );

	void					HotReloadMap();
	void					ReloadLights();

	void					LocalMapRestart( void );
	void					MapRestart( void );
	static void				MapRestart_f( const idCmdArgs &args );
	bool					NextMap( void );	// returns wether serverinfo settings have been modified
	static void				NextMap_f( const idCmdArgs &args );

	idMapFile *				GetLevelMap( void );
	const char *			GetMapName( void ) const;
	idStr			        GetMapNameStripped(void) const;

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
	bool					SpawnEntityDef( const idDict &args, idEntity **ent = NULL, bool setDefaults = true );
	int						GetSpawnId( const idEntity *ent ) const;

	const idDeclEntityDef *	FindEntityDef( const char *name, bool makeDefault = true ) const;
	const idDict *			FindEntityDefDict( const char *name, bool makeDefault = true ) const;

	void					RegisterEntity( idEntity *ent );
	void					UnregisterEntity( idEntity *ent );

	bool					RequirementMet( idEntity *activator, const idStr &_requires, int removeItem );
	bool					RequirementMet_Inventory(idEntity* activator, const idStr& _requires, int removeItem); //BC

	//void					AlertAI( idEntity *ent ); //BC replace this with SuspiciousNoise system.
	//idActor *				GetAlertEntity( void );

	bool					InPlayerPVS( idEntity *ent ) const;
	bool					InPlayerConnectedArea( idEntity *ent ) const;
	bool					InPlayerAreasRecalculated(idEntity* ent); // blendo eric: recalcs area if outside of game think
#ifdef _D3XP
	pvsHandle_t				GetPlayerPVS()			{ return playerPVS; };
#endif

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

	

	void					KillBox( idEntity *ent, bool catch_teleport = false );
	void					RadiusDamage( const idVec3 &origin, idEntity *inflictor, idEntity *attacker, idEntity *ignoreDamage, idEntity *ignorePush, const char *damageDefName, float dmgPower = 1.0f );
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

	void					SpreadLocations();
	idLocationEntity *		LocationForPoint( const idVec3 &point );	// May return NULL
	
	// SM: These functions are recommended for figuring out which location/area an entity is in
	idLocationEntity *		LocationForEntity( const idEntity *ent );	// May return NULL
	int						AreaNumForEntity( const idEntity *ent );	// Returns -1 if none

	idLocationEntity **		GetLocationEntities() { return locationEntities; }

	idEntity *				SelectInitialSpawnPoint( idPlayer *player );

	void					SetPortalState( qhandle_t portal, int blockingBits );
	void					SaveEntityNetworkEvent( const idEntity *ent, int event, const idBitMsg *msg );
	void					ServerSendChatMessage( int to, const char *name, const char *text );
	int						ServerRemapDecl( int clientNum, declType_t type, int index );
	int						ClientRemapDecl( declType_t type, int index );

	void					SetGlobalMaterial( const idMaterial *mat );
	const idMaterial *		GetGlobalMaterial();

	void					SetGibTime( int _time ) { nextGibTime = _time; };
	int						GetGibTime() { return nextGibTime; };


	//moving this to public.
	static int				sortSpawnPoints_Farthest(const void *ptr1, const void *ptr2);
	static int				sortSpawnPoints_Nearest(const void *ptr1, const void *ptr2);


	static int				sortVecPoints_Farthest(const void *ptr1, const void *ptr2);



	//BC PUBLIC
	int						EntitiesWithinAbsBoundingbox(const idBounds bounds, idEntity **entityList, int maxCount) const;
	int						EntitiesWithinBoundingbox(const idBounds bounds, idVec3 boxPosition, idEntity **entityList, int maxCount) const;
	
	
	void					ThrowShrapnel(const idVec3 &origin, const char *defName, const idEntity *passEntity);
	bool					PrecacheDef(idEntity *ent, const char *defName);
	bool					PrecacheModel(idEntity *ent, const char *modelDef);
	idVec3 *				GetPointsOnSphere(int num);	

	void					SetSuspiciousNoise(idEntity *ent, idVec3 position, int radius, int priority);
	idVec3					suspiciousNoisePos;
	int						suspiciousNoiseRadius;
	int						lastSuspiciousNoiseTime;
	int						lastSuspiciousNoisePriority;

	int						IsBaffled(idVec3 soundPosition);


	void					SetSlowmo(bool value, bool quickReset = false);

	idStr					GetKeyFromBinding(const char *bindname, bool joystick = false);

	

	const char *			ParseTime(float value);
	const char *			ParseTimeMS(int value);
	const char *			ParseTimeDetailedMS(int value);
	const char *			ParseTimeMS_SecAndDecisec(int value);
	idStr					ParseTimeVerbose(int value);

	void					SetDebrisBurst(const char *defName, idVec3 position, int count, float radius, float speed, idVec3 direction);

	idEntityPtr<idEntity>	metaEnt;

	idEntity *				DoParticle(const char *particleName, idVec3 position, idVec3 angle = vec3_zero, bool autoRemove = true, bool airlessGravity = false, idVec3 color = idVec3(1, 1, 1));
	idEntity*				DoParticleAng(const char* particleName, idVec3 position, idAngles angle = idAngles(0,0,0), bool autoRemove = true, bool airlessGravity = false, idVec3 color = idVec3(1, 1, 1));

	void					DoGravityCheck();

	

	bool					GetAirlessAtPoint(idVec3 point);

	int						lastExpensiveObservationCallTime;

	idEntity *				SpawnInterestPoint(idEntity *ownerEnt, idVec3 position, const char *interestDef);
	void					ClearInterestPoints(void);

	bool					IsInEndGame();

	idLinkList<idEntity>	bafflerEntities;
	idLinkList<idEntity>	interestEntities;
	idLinkList<idEntity>	searchnodeEntities;
	idLinkList<idEntity>	confinedEntities;
	idLinkList<idEntity>	ventdoorEntities;
	idLinkList<idEntity>	repairEntities;		//Things that can be repaired by repair bots.
	idLinkList<idEntity>	hatchEntities;		//repair hatches.
	idLinkList<idEntity>	idletaskEntities;
	idLinkList<idEntity>	turretEntities;
	idLinkList<idEntity>	petEntities;
	idLinkList<idEntity>	securitycameraEntities;
	idLinkList<idEntity>	airlockEntities;
	idLinkList<idEntity>	catfriendsEntities;
	idLinkList<idEntity>	landmineEntities;
	idLinkList<idEntity>	skullsaverEntities;
	idLinkList<idEntity>	catcageEntities;
	idLinkList<idEntity>	spacenudgeEntities;
	idLinkList<idEntity>	memorypalaceEntities;
	idLinkList<idEntity>	dynatipEntities;
	idLinkList<idEntity>	windowshutterEntities;
	idLinkList<idEntity>	electricalboxEntities;
	idLinkList<idEntity>	spectatenodeEntities;
	


	//Static lists. These lists don't change. Does not grow, does not shrink.
	idStaticList<idEntity *, MAX_GENTITIES> repairpatrolEntities;
	idStaticList<idEntity *, MAX_GENTITIES> upgradecargoEntities;
	idStaticList<idEntity *, MAX_GENTITIES> healthstationEntities;
	idStaticList<idEntity *, MAX_GENTITIES> trashexitEntities;


	bool					menuPause;
	bool					spectatePause;
	bool					requestPauseMenu;

	int						GetAmountEnemiesSeePlayer(bool onlyAlerted);


	virtual void			OnMapChange(); //call this after things have finished spawning and all systems are initialized.

	int						nextGrenadeTime;
	int						nextPetTime;

	idListGUI *				eventlogGuiList;
	idFeedAlertWindow*		eventLogAlerts = nullptr;
	void					InitEventLog(void);
	void					ShutdownEventLog();
	void					InitEventLogFile(bool startOfSession);
	void					CloseEventLogFile(void);
	void					AddEventLog(const char *text, idVec3 _position, bool showInfoFeed = true, int eventType = 0, bool showDupes = true); // SW 6th May 2025: adding showDupes arg, defaults to true
	void					AddEventlogDamage(idEntity *target, int damage, idEntity *inflictor, idEntity *attackerEnt, const char *damageDefname);
	void					AddEventlogDeath(idEntity *target, int damage, idEntity *inflictor, idEntity *attackerEnt, const char *damageDefname, int eventType);
	void					DisplayEventLogAlert(const char *text, const char * icon = nullptr, idVec4 * textColor = nullptr, idVec4 * bgColor = nullptr, idVec4 * iconColor = nullptr, float durationSeconds = 0.0f, bool allowDupes = true);
	static void				TestEventLogFeed();
	idList<eventlog_t>		eventLogList;


	bool					GetPositionIsInsideConfinedspace(idVec3 _targetPosition);
	idEntity*				GetConfinedTriggerAtPos( const idVec3& targetPosition );

	idEntity *				LaunchProjectile(idEntity *owner, const char *defName, idVec3 projectileSpawnPos, idVec3 projectileTrajectory);

	idVOManager				voManager;

	bool					DoOriginContainmentCheck(idEntity *ent);

	void					DoVacuumSuctionActors(idVec3 suctionInteriorPosition, idVec3 suctionExteriorPosition, const idAngles& angles, bool pullPlayer);
	void					DoVacuumSuctionItems(idVec3 suctionInteriorPosition, idVec3 suctionExteriorPosition);

    idVec3                  GetPhasedDestination(trace_t collisionInfo, idVec3 startPosition);

	bool					InPVS_Entity(idEntity *ent, idVec3 targetPos) const;
	bool					InPVS_Pos(idVec3 observerPos, idVec3 targetPos) const;


	void					AddSubtitle(idStr speakerName, idStr text, int durationMS);
	void					DrawSubtitles( class idUserInterface* hud );

	bool					RunMapScript(idStr functionName);
	bool					RunMapScript(idStr functionName, const idCmdArgs& args);
	bool					RunMapScriptArgs(idStr functionName, idEntity *activator, idEntity *callee);
	void					SetGravityFromCVar();
	LoadingContext			GetLoadingContext();

	void					CreateSparkObject(idVec3 position, idVec3 normal);
	bool					IsCollisionSparkable(const trace_t &collision);

	bool					HasJockeyLOSToEnt(idEntity *ent);

	int						lastDebugNoteIndex;

	idStr					GetShipName();
	idStr					GetShipDesc();

	idStr					GetMilestonenameViaLevelIndex(int levelIndex, int milestoneIndex);

	idDict					mouseButtonDict;
	idDict					controllerButtonDicts[CT_NUM];

	void					DoSpewBurst(idEntity* ent, int spewType);

	idStr					GetMapdisplaynameViaProgressIndex(int index);
	idStr					GetMapdisplaynameViaMapfile(idStr mapfilename);

	void					BindStickyItemViaTrace(idEntity* stickyItem, trace_t tr);

	idStr					GetCatViaModel(idStr modelname, int* voiceprint);

	idStr					GetBindingnameViaBind(idStr bindname);

	static void				TestTimedFunc_f(const idCmdArgs& args);
	void					TestTimedFunc(const idCmdArgs& args);
	//BC PUBLIC end


private:
	const static int		INITIAL_SPAWN_COUNT = 1;

	idStr					mapFileName;			// name of the map, empty string if no map loaded
	idMapFile *				mapFile;				// will be NULL during the game unless in-game editing is used
	bool					mapCycleLoaded;

	int						spawnCount;
	int						mapSpawnCount;			// it's handy to know which entities are part of the map

	idLocationEntity **		locationEntities;		// for location names, etc

	idCamera *				camera;
	const idMaterial *		globalMaterial;		// for overriding everything

	idList<idAAS *>			aasList;				// area system
	idStrList				aasNames;

	//idEntityPtr<idActor>	lastAIAlertEntity;
	//int						lastAIAlertTime;

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

	idStaticList<spawnSpot_t, MAX_GENTITIES> spawnSpots;
	idStaticList<idEntity *, MAX_GENTITIES> initialSpots;
	int						currentInitialSpot;

#ifdef CTF
	idStaticList<spawnSpot_t, MAX_GENTITIES> teamSpawnSpots[2];
	idStaticList<idEntity *, MAX_GENTITIES> teamInitialSpots[2];
	int						teamCurrentInitialSpot[2];
#endif

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
	

	void					DumpOggSounds( void );
	void					GetShakeSounds( const idDict *dict );

	void					UpdateLagometer( int aheadOfServer, int dupeUsercmds );

	//BC PRIVATE
	bool					menuSlowmo;
	int						lastEditstate;
	void					DrawEntityDebug(idEntity *ent, idBounds viewTextBounds, idMat3 axis, idVec3 up, float distAlpha = 1.0f, bool fullInfo = false);

	idFile*					eventLogFile;
	idStr					TimeStampForEventLog(tm* time);


	//Subtitles.
	int						slotCircularIterator; // blendo eric: internal index to the newest item in queue
	idSubtitleItem			subtitleItems[MAX_SUBTITLES];
	
	idSubtitleItem&			GetSubtitleSlot(int slot); // slot 0 starting from most recent subtitle to oldest
	idSubtitleItem&			QueueSubtitleSlot(); // adds new item to queue
	int						GetSubtitleCount(); // active subtitles in queue
	void					ResetSubtitleSlots(); // reset queue and items

	void					UpdateSubtitles();

	int						loadCount;
	void					AddLoadingContext();
	void					RemoveLoadingContext();
	bool					InLoadingContext() { return loadCount > 0; }

	idStr					GetLogAttackerName(idEntity *inflictor, idEntity *attackerEnt, const char *damageDefname);

	idStr					currentSessionSaveGamePath;

	//BC PRIVATE END
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
}

template< class type >
ID_INLINE void idEntityPtr<type>::Save( idSaveGame *savefile ) const {
	savefile->WriteInt( spawnId ); // int spawnId
}

template< class type >
ID_INLINE void idEntityPtr<type>::Restore( idRestoreGame *savefile ) {
	savefile->ReadInt( spawnId ); // int spawnId
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
ID_INLINE bool idEntityPtr<type>::operator==(idEntityPtr<type>& other) const {
	return spawnId == other.spawnId;
}

#if !defined(_MSC_VER)
template< class type >
ID_INLINE bool idEntityPtr<type>::operator==(const idEntityPtr<type>& other) const {
	return spawnId == other.spawnId;
}
#endif

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
	return ( gameLocal.spawnIds[ spawnId & ( ( 1 << GENTITYNUM_BITS ) - 1 ) ] == ( spawnId >> GENTITYNUM_BITS ) )
		&& GetEntity() != nullptr; // SM: Added this extra check for some weird edge cases
}

template< class type >
ID_INLINE type *idEntityPtr<type>::GetEntity( void ) const {
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


//  ===========================================================================
// blendo eric: gamepad / controller vibration / rumble system
class GamePadFXSystem
{
public:
	static GamePadFXSystem&	GetSys(){ return gamePadFXSys; } // singleton for command calls

	void					AddRumble( float intensityPct = 0.5f /*[0,1]*/, int timeInMs = 1000 /*[0,10000]*/); // additive
	void					AddLight( idVec3 lightRGB /*[0,1]*/, int timeInMs = idMath::INFINITY /*[0,inf]*/);  // additive

	static void				Rumble_f( const idCmdArgs &args); // additive
	static void				Light_f( const idCmdArgs &args); // additive

	void					Reset(); // deactivate rumbles etc

	void					Update(int frameTimeMs); // call once per frame to update gamepad fx

	// detailed event
	enum GamePadFXIDTypes { ID_REPLACE_ALL = -1, ID_REPLACE_NONE = 0 };
	enum GamePadFXType { TYPE_RUMBLE = BIT(0), TYPE_LIGHT = BIT(1) };
	struct gamePadFXEvent_t
	{
		uint fxTypeFlag = TYPE_RUMBLE | TYPE_LIGHT; // the type of event
		int uniqueID = 0;				// id value for unique fx channels, where the new fx replaces old ones of same id, 0 replaces none, -1 replaces all
		float rumblePctFreqLow = 0;		// intensity big thumpy motor [0.0f,1.0f]
		float rumblePctFreqHigh = 0;	// intensity small buzzy motor [0.0f,1.0f]
		float rumblePctTriggerLeft = 0; // intensity trigger left[0.0f,1.0f]
		float rumblePctTriggerRight = 0;// intensity trigger right [0.0f,1.0f]

		idVec3 ledLight = idVec3(0.0f,0.0f,0.0f);

		float timePeriodMs = 0;			// in ms, sinewave intensity, reaches zero after period time

		// initialized by sys
		int timeLeftMs = 0;				// lifetime of event
		int timeAliveMs = 0;			// age of event
	};
	void AddFX( gamePadFXEvent_t & fx );

protected:
	idList<gamePadFXEvent_t> fxEventList = idList<gamePadFXEvent_t>();
	static GamePadFXSystem gamePadFXSys;
};

//  ===========================================================================

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
	SND_CHANNEL_MUSIC = SCHANNEL_MUSIC,

	// internal use only.  not exposed to script or framecommands.
	SND_CHANNEL_AMBIENT,
	SND_CHANNEL_DAMAGE
} gameSoundChannel_t;

extern const float	DEFAULT_GRAVITY;
extern const idVec3	DEFAULT_GRAVITY_VEC3;
extern const int	CINEMATIC_SKIP_DELAY;

#endif	/* !__GAME_LOCAL_H__ */
