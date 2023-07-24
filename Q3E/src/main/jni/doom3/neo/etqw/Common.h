// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __GAME_COMMON_H__
#define __GAME_COMMON_H__

class idPlayerStart;
class sdDeclInvItem;
class idClipModel;

const int	MAX_LOGGED_TRACES				= 16;
const int	MAX_LOGGED_DECALS				= 32;

const int	MAX_CLIENTS						= 32; // FIXME: Gordon: these should just be shared between engine/game
const int	ASYNC_DEMO_CLIENT_INDEX			= MAX_CLIENTS;
const int	MAX_MISSION_TEAM_PLAYERS		= 8;

const int	GENTITYNUM_BITS					= 11;
const int	MAX_GENTITIES					= ( 1 << GENTITYNUM_BITS );
const int	ENTITYNUM_NONE					= ( MAX_GENTITIES - 1 );
const int	ENTITYNUM_WORLD					= ( MAX_GENTITIES - 2 );
const int	ENTITYNUM_MAX_NORMAL			= ( MAX_GENTITIES - 2 );
const int	MAX_GAME_MESSAGE_SIZE			= 1024;
const int	ENTITY_PVS_SIZE					= ( ( MAX_GENTITIES + 31 ) >> 5 );
const float THIRD_PERSON_FOCUS_DISTANCE		= 512.0f;
const int	LAND_DEFLECT_TIME				= 150;
const int	LAND_RETURN_TIME				= 300;
const int	FOCUS_GUI_TIME					= 50;
const int	ENTITYNUM_CLIENT				= ( MAX_GENTITIES - 3 );
const int	CENTITYNUM_BITS					= 12;
const int 	MAX_CENTITIES					= ( 1 << CENTITYNUM_BITS );
const char* const DEFAULT_GRAVITY_STRING	= "1066";
const float DEFAULT_GRAVITY					= 1066.0f;
const int	NETWORKEVENT_RULES_ID			= -2;
const int	NETWORKEVENT_OBJECTIVE_ID		= -3;

const int	net_clientBits					= idMath::BitsForInteger( MAX_CLIENTS );

const int	MAX_BOUNDS_AREAS				= 16;

const idVec3 DEFAULT_GRAVITY_VEC3( 0, 0, -DEFAULT_GRAVITY );

template< class type >
class idEntityPtr {
public:
							idEntityPtr( void );
							idEntityPtr( const type* other );

	idEntityPtr< type >&	operator=( const type *ent );

	// synchronize entity pointers over the network
	int						GetSpawnId( void ) const { return spawnId; }
	bool					SetSpawnId( int id );

	// Always sets, regardless of whether the entity is valid or not, required for one off sets, where the entity in question may not exist on the client yet
	void					ForceSpawnId( int id );

	bool					IsValid( void ) const;
	type*					GetEntity( void ) const;
	int						GetEntityNum( void ) const;
	int						GetId( void ) const;
	// RAVEN BEGIN
	type*					operator->( void ) const;
	operator				type*( void ) const;
	// RAVEN END

private:
	int						spawnId;
};

typedef enum {
	AF_JOINTMOD_AXIS,
	AF_JOINTMOD_ORIGIN,
	AF_JOINTMOD_BOTH
} AFJointModType_t;

typedef enum gameReliableServerMessage_e {
	GAME_RELIABLE_SMESSAGE_DELETE_ENT,
	GAME_RELIABLE_SMESSAGE_CREATE_ENT,
	GAME_RELIABLE_SMESSAGE_MULTI_CREATE_ENT,
	GAME_RELIABLE_SMESSAGE_CHAT,
	GAME_RELIABLE_SMESSAGE_TEAM_CHAT,
	GAME_RELIABLE_SMESSAGE_FIRETEAM_CHAT,
	GAME_RELIABLE_SMESSAGE_QUICK_CHAT,
	GAME_RELIABLE_SMESSAGE_ENTITY_EVENT,
	GAME_RELIABLE_SMESSAGE_MULTI_ENTITY_EVENT,
	GAME_RELIABLE_SMESSAGE_TOOLTIP,
	GAME_RELIABLE_SMESSAGE_TASK,
	GAME_RELIABLE_SMESSAGE_FIRETEAM,
	GAME_RELIABLE_SMESSAGE_CREATEPLAYERTARGETTIMER,
	GAME_RELIABLE_SMESSAGE_NETWORKEVENT,
	GAME_RELIABLE_SMESSAGE_CREATEDEPLOYREQUEST,
	GAME_RELIABLE_SMESSAGE_DELETEDEPLOYREQUEST,
	GAME_RELIABLE_SMESSAGE_USERGROUPS,
	GAME_RELIABLE_SMESSAGE_OBITUARY,
	GAME_RELIABLE_SMESSAGE_VOTE,
	GAME_RELIABLE_SMESSAGE_SETNEXTOBJECTIVE,
	GAME_RELIABLE_SMESSAGE_RECORD_DEMO,
	GAME_RELIABLE_SMESSAGE_RULES_DATA,
	GAME_RELIABLE_SMESSAGE_SETSPAWNPOINT,
	GAME_RELIABLE_SMESSAGE_FIRETEAM_DISBAND,
	GAME_RELIABLE_SMESSAGE_SESSION_ID,
	GAME_RELIABLE_SMESSAGE_LOCALISEDMESSAGE,
	GAME_RELIABLE_SMESSAGE_STATSMESSAGE,
	GAME_RELIABLE_SMESSAGE_STATSFINIHED,
	GAME_RELIABLE_SMESSAGE_ENDGAMESTATS,
	GAME_RELIABLE_SMESSAGE_ADMINFEEDBACK,
	GAME_RELIABLE_SMESSAGE_SETCRITICALCLASS,
	GAME_RELIABLE_SMESSAGE_MAP_RESTART,
	GAME_RELIABLE_SMESSAGE_PAUSEINFO,
#ifdef SD_SUPPORT_REPEATER
	GAME_RELIABLE_SMESSAGE_REPEATER_CHAT,
#endif // SD_SUPPORT_REPEATER
	GAME_RELIABLE_SMESSAGE_BANLISTMESSAGE,
	GAME_RELIABLE_SMESSAGE_BANLISTFINISHED,
	GAME_RELIABLE_SMESSAGE_REPEATERSTATUS,
	GAME_RELIABLE_SMESSAGE_NUM_MESSAGES
} gameReliableServerMessage_t;

typedef enum gameReliableClientMessage_e {
	GAME_RELIABLE_CMESSAGE_CHAT,
	GAME_RELIABLE_CMESSAGE_TEAM_CHAT,
	GAME_RELIABLE_CMESSAGE_FIRETEAM_CHAT,
	GAME_RELIABLE_CMESSAGE_QUICK_CHAT,
	GAME_RELIABLE_CMESSAGE_KILL,
	GAME_RELIABLE_CMESSAGE_GOD,
	GAME_RELIABLE_CMESSAGE_NOCLIP,
	GAME_RELIABLE_CMESSAGE_IMPULSE,
	GAME_RELIABLE_CMESSAGE_GUISCRIPT,
	GAME_RELIABLE_CMESSAGE_NETWORKCOMMAND,
	GAME_RELIABLE_CMESSAGE_NETWORKSPAWN,
	GAME_RELIABLE_CMESSAGE_TEAMSWITCH,
	GAME_RELIABLE_CMESSAGE_CLASSSWITCH,
	GAME_RELIABLE_CMESSAGE_CHANGEWEAPON,
	GAME_RELIABLE_CMESSAGE_DEFAULTSPAWN,
	GAME_RELIABLE_CMESSAGE_GIVECLASS,
	GAME_RELIABLE_CMESSAGE_RESPAWN,
	GAME_RELIABLE_CMESSAGE_ADMIN,
	GAME_RELIABLE_CMESSAGE_SELECTWEAPON,
	GAME_RELIABLE_CMESSAGE_CALLVOTE,
	GAME_RELIABLE_CMESSAGE_VOTE,
	GAME_RELIABLE_CMESSAGE_FIRETEAM,
	GAME_RELIABLE_CMESSAGE_TASK,
	GAME_RELIABLE_CMESSAGE_RESERVECLIENTSLOT,
	GAME_RELIABLE_CMESSAGE_REQUESTSTATS,
	GAME_RELIABLE_CMESSAGE_ACKNOWLEDGESTATS,
	GAME_RELIABLE_CMESSAGE_SETMUTESTATUS,
	GAME_RELIABLE_CMESSAGE_SETSPAWNPOINT,
	GAME_RELIABLE_CMESSAGE_SETSPECTATECLIENT,
	GAME_RELIABLE_CMESSAGE_ANTILAGDEBUG,
	GAME_RELIABLE_CMESSAGE_ACKNOWLEDGEBANLIST,
	GAME_RELIABLE_CMESSAGE_REPEATERSTATUS,
	GAME_RELIABLE_CMESSAGE_SPECTATECOMMAND,
	GAME_RELIABLE_CMESSAGE_NUM_MESSAGES
} gameReliableClientMessage_t;

typedef enum {
	GAMESTATE_UNINITIALIZED,		// prior to Init being called
	GAMESTATE_NOMAP,				// no map loaded
	GAMESTATE_STARTUP,				// inside InitFromNewMap().  spawning map entities.
	GAMESTATE_ACTIVE,				// normal gameplay
	GAMESTATE_SHUTDOWN				// inside MapShutdown().  clearing memory.
} gameState_t;

typedef enum lightType_e {
	LIGHT_NONE				= BITT< 0 >::VALUE,
	LIGHT_STANDARD			= BITT< 1 >::VALUE,
	LIGHT_BRAKELIGHT		= BITT< 2 >::VALUE,
} lightType_t;

typedef enum rotorType_e {
	RT_MAIN,
	RT_TAIL,
} rotorType_t;

typedef enum aorFlags_e {
	AOR_INHIBIT_USERCMDS		= BITT< 0 >::VALUE,
	AOR_INHIBIT_ANIMATION		= BITT< 1 >::VALUE,
	AOR_INHIBIT_IK				= BITT< 2 >::VALUE,
	AOR_INHIBIT_PHYSICS			= BITT< 3 >::VALUE,
	AOR_BOX_DECAY_CLIP			= BITT< 4 >::VALUE,
	AOR_POINT_DECAY_CLIP		= BITT< 5 >::VALUE,
	AOR_HEIGHTMAP_DECAY_CLIP	= BITT< 6 >::VALUE,
	AOR_PHYSICS_LOD_1			= BITT< 7 >::VALUE,
	AOR_PHYSICS_LOD_2			= BITT< 8 >::VALUE,
	AOR_PHYSICS_LOD_3			= BITT< 9 >::VALUE,
} aorFlags_t;

typedef struct angleClamp_s {
	typedef struct flags_s {
		bool	enabled		: 1;
		bool	limitRate	: 1;
	} flags_t;

	flags_t					flags;

	idVec2					extents;
	idVec2					rate;
	float					filter;
	const idSoundShader*	sound;
} angleClamp_t;

typedef struct partDamageInfo_s {
	float damageScale;
	float collisionScale;
	float collisionMinSpeed;
	float collisionMaxSpeed;
} partDamageInfo_t;

typedef enum deployResult_e {
	DR_CLEAR,
	DR_WARNING,
	DR_FAILED,
	DR_CONDITION_FAILED,
	DR_OUT_OF_RANGE,
} deployResult_t;

struct deployMaskExtents_t {
	int							minx;
	int							miny;
	int							maxx;
	int							maxy;
};

enum traceMode_t {
	TM_DEFAULT = 0,
	TM_CROSSHAIR_INFO,
	TM_THIRDPERSON_OFFSET,
	TM_NUM_TRACE_MODES,
};

enum usercmdImpulse_t {
	UCI_WEAP0 = 0,
	UCI_WEAP1 = 1,
	UCI_WEAP2 = 2,
	UCI_WEAP3 = 3,
	UCI_WEAP4 = 4,
	UCI_WEAP5 = 5,
	UCI_WEAP6 = 6,
	UCI_WEAP7 = 7,
	UCI_WEAP8 = 8,
	UCI_WEAP9 = 9,
	UCI_WEAP10 = 10,
	UCI_WEAP11 = 11,
	UCI_WEAP12 = 12,
	UCI_RELOAD = 13,
	UCI_WEAPNEXT = 14,
	UCI_WEAPPREV = 15,
	UCI_STROYUP = 16,
	UCI_STROYDOWN = 17,
	UCI_USE_VEHICLE = 18,
	UCI_VEHICLE_CAMERA_MODE = 19,
	UCI_PRONE = 20,
	UCI_READY = 21,
	UCI_NUM_IMPULSES = 22,
};

enum usercmdLocalImpulse_t {
	ULI_ADMIN_MENU = 0,
	ULI_QUICKCHAT = 1,
	ULI_LIMBO_MENU = 2,
	ULI_COMMAND_MAP = 3,
	ULI_TASK_MENU = 4,
	ULI_FIRETEAM = 5,
	ULI_MENU_EVENT_CLICK = 6,
	ULI_MENU_EVENT_GENERAL1 = 7,
	ULI_MENU_EVENT_GENERAL2 = 8,
	ULI_MENU_EVENT_GENERAL3 = 9,
	ULI_MENU_EVENT_GENERAL4 = 10,
	ULI_MENU_EVENT_GENERAL5 = 11,
	ULI_MENU_EVENT_GENERAL6 = 12,
	ULI_MENU_EVENT_NAV_FORWARD = 13,
	ULI_MENU_EVENT_NAV_BACKWARD = 14,
	ULI_MENU_EVENT_ACCEPT = 15,
	ULI_MENU_EVENT_CANCEL = 16,
	ULI_SHOWLOCATIONS = 17,
	ULI_CONTEXT = 18,
	ULI_VOTE_MENU = 19,
	ULI_SHOW_WAYPOINTS = 20,
	ULI_MENU_NEWLINE = 21,
	ULI_MENU_EVENT_CONTEXT = 22,
	ULI_SHOW_FIRETEAM = 23,
	ULI_NUM_IMPULSES = 24,
};

typedef idList< const sdDeclInvItem* > sdDeclItemPackageItems;
typedef int ammoType_t;

#define CALL_MEMBER_FN( object, ptrToMember )  ( ( object ).*( ptrToMember ) )
#define CALL_MEMBER_FN_PTR( object, ptrToMember )  ( ( object )->*( ptrToMember ) )

#ifndef SD_PUBLIC_BUILD
	#define ASYNC_SECURITY
#endif // SD_PUBLIC_BUILD

#ifdef ASYNC_SECURITY

#define ASYNC_SECURITY_READ( STREAM )													\
	if ( STREAM.ReadLong() != ASYNC_MAGIC_NUMBER ) {									\
		gameLocal.Error( "Network State Corrupted: %s(%d)", __FILE__, __LINE__ );		\
	}

#define ASYNC_SECURITY_WRITE( STREAM )													\
	STREAM.WriteLong( ASYNC_MAGIC_NUMBER );

#else
#define ASYNC_SECURITY_READ( STREAM )
#define ASYNC_SECURITY_WRITE( STREAM )
#endif // ASYNC_SECURITY

#define ASYNC_SECURITY_READ_FILE( STREAM )												\
	{																					\
		int dummy;																		\
		STREAM->ReadInt( dummy );														\
		if ( dummy != ASYNC_MAGIC_NUMBER ) {											\
			gameLocal.Error( "Network State Corrupted: %s(%d)", __FILE__, __LINE__ );	\
		}																				\
	}

#define ASYNC_SECURITY_WRITE_FILE( STREAM )												\
	STREAM->WriteInt( ASYNC_MAGIC_NUMBER );

#endif // __GAME_COMMON_H__
