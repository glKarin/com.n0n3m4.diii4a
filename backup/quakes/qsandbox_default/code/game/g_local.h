/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copyPl of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
//
// g_local.h -- local definitions for game module

#include "../qcommon/q_shared.h"
#include "bg_public.h"
#include "g_public.h"

// the "gameversion" client command will print this plus compile date
#define	GAMEVERSION	BASEGAME

#define BODY_QUEUE_SIZE		8

#define INFINITE			1000000

#define	FRAMETIME			100					// msec
#define	CARNAGE_REWARD_TIME	3000
#define REWARD_SPRITE_TIME	2000

#define	INTERMISSION_DELAY_TIME	1000
#define	SP_INTERMISSION_DELAY_TIME	5000

//Domination how many seconds between awarded a point (multiplied by two if more than 3 points)
#define DOM_SECSPERPOINT	2000

//limit of the votemaps.cfg file and other custom map files
#define	MAX_MAPS_TEXT		8192

// gentity->flags
#define	FL_GODMODE				0x00000010
#define	FL_NOTARGET				0x00000020
#define	FL_TEAMSLAVE			0x00000400	// not the first on the team
#define FL_NO_KNOCKBACK			0x00000800
#define FL_DROPPED_ITEM			0x00001000
#define FL_NO_BOTS				0x00002000	// spawn point not for bot use
#define FL_NO_HUMANS			0x00004000	// spawn point just for bots
#define FL_FORCE_GESTURE		0x00008000	// force gesture on client
#define FL_NO_SPAWN				0x00010000  // spawn point not in use

// target_debrisemitter and func_breakable debris spawnflags
#define SF_DEBRIS_LIGHT					1
#define SF_DEBRIS_DARK					2
#define SF_DEBRIS_LIGHT_LARGE			4
#define SF_DEBRIS_DARK_LARGE			8
#define SF_DEBRIS_WOOD					16
#define SF_DEBRIS_FLESH					32
#define SF_DEBRIS_GLASS					64
#define SF_DEBRIS_STONE					128

// target_effect spawnflags
#define SF_EFFECT_EXPLOSION				1
#define SF_EFFECT_PARTICLES_GRAVITY		2
#define SF_EFFECT_PARTICLES_LINEAR		4
#define SF_EFFECT_PARTICLES_LINEAR_UP	8
#define SF_EFFECT_PARTICLES_LINEAR_DOWN	16
#define SF_EFFECT_OVERLAY				32
#define SF_EFFECT_FADE					64
#define SF_EFFECT_SMOKEPUFF				128
#define SF_EFFECT_ACTIVATOR				256

// physics engine
#define		PHYS_ROTATING 0.020
#define		PHYS_PROP_IMPACT g_physimpact.value
#define		PHYS_SENS g_physimpulse.integer
#define		PHYS_DAMAGE g_physdamage.value
#define		PHYS_DAMAGESENS 30

#define		VEHICLE_PROP_IMPACT g_physimpact.value
#define		VEHICLE_SENS 30
#define		VEHICLE_DAMAGE 0.05
#define		VEHICLE_DAMAGESENS 30

// movers are things like doors, plats, buttons, etc
typedef enum {
	MOVER_POS1,
	MOVER_POS2,
	MOVER_1TO2,
	MOVER_2TO1,
	
	ROTATOR_POS1,
	ROTATOR_POS2,
	ROTATOR_1TO2,
	ROTATOR_2TO1
} moverState_t;

#define SP_PODIUM_MODEL		"models/mapobjects/podium/podium4.md3"

#define MAX_LOGIC_ENTITIES		256 //maximum number of entities that can target a target_logic
#define MAX_NETNAME			36

typedef struct gentity_s gentity_t;
typedef struct gclient_s gclient_t;

struct gentity_s {
	entityState_t	s;				// communicated by server to clients
	entityShared_t	r;				// shared by both the server system and game

	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!
	//================================

	struct gclient_s	*client;			// NULL if not a client

	qboolean	inuse;

	char		*classname;			// set in QuakeEd
	int			spawnflags;			// set in QuakeEd

	qboolean	neverFree;			// if true, FreeEntity will only unlink
									// bodyque uses this

	int			flags;				// FL_* variables

	char		*model;
	char		*model2;
	int			freetime;			// level.time when the object was freed
	
	int			eventTime;			// events will be cleared EVENT_VALID_MSEC after set
	qboolean	freeAfterEvent;
	qboolean	unlinkAfterEvent;

	qboolean	physicsObject;		// if true, it can be pushed by movers and fall off edges
									// all game items are physicsObjects, 
	float		physicsBounce;		// 1.0 = continuous bounce, 0.0 = no bounce
	int			clipmask;			// brushes with this content value will be collided against
									// when moving.  items and corpses do not collide against
									// players, for instance

	// movers
	moverState_t moverState;
	int			soundPos1;
	int			sound1to2;
	int			sound2to1;
	int			soundPos2;
	int			soundLoop;
	gentity_t	*parent;
	gentity_t	*nextTrain;
	gentity_t	*prevTrain;
	vec3_t		pos1, pos2;

	char		*message;

	int			timestamp;		// body queue sinking, etc

	float		angle;			// set in editor, -1 = up, -2 = down
	char		*target;
	char		*targetname;
	char		*team;
	char		*targetShaderName;
	char		*targetShaderNewName;
	gentity_t	*target_ent;

	float		speed;
	vec3_t		movedir;

	int			nextthink;
	void		(*think)(gentity_t *self);
	void		(*reached)(gentity_t *self);	// movers call this when hitting endpoint
	void		(*blocked)(gentity_t *self, gentity_t *other);
	void		(*touch)(gentity_t *self, gentity_t *other, trace_t *trace);
	void		(*use)(gentity_t *self, gentity_t *other, gentity_t *activator);
	void		(*pain)(gentity_t *self, gentity_t *attacker, int damage);
	void		(*die)(gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod);

	int			pain_debounce_time;
	int			fly_sound_debounce_time;	// wind tunnel
	int			last_move_time;

	int			health;

	qboolean	takedamage;

	int			damage;
	int			splashDamage;	// quad will increase this without increasing radius
	int			splashRadius;
	int			methodOfDeath;
	int			splashMethodOfDeath;

	int			count;

	gentity_t	*chain;
	gentity_t	*enemy;
	gentity_t	*activator;
	gentity_t	*teamchain;		// next entity in team
	gentity_t	*teammaster;	// master of the team

	int			kamikazeTime;
	int			kamikazeShockTime;

	int			watertype;
	int			waterlevel;

	int			noise_index;

	// timing variables
	float		wait;
	float		random;

	gitem_t		*item;			// for bonus items

	//NEW VARIABLES

	int			playerangle;
	int			flashon;
	int			crosson;
	int			price;
	int			owner;	// clientNum player owner
	int			locked;	// clientNum player owner
	char		*ownername;	// clientNum player owner
	int			sandboxObject;
	qboolean	takedamage2;
	char		*botname;

	int			lastThinkTime;
	
	int			sb_coltype;
	int			sb_gravity;
	float		sb_colscale0;
	float		sb_colscale1;
	float		sb_colscale2;
	float		sb_rotate0;
	float		sb_rotate1;
	float		sb_rotate2;
	char		*sb_class;
	char		*sb_model;
	char		*sb_sound;
	int			sb_material;
	int			sb_phys;
	int			sb_coll;
	int			sb_ettype;
	int			sb_red;
	int			sb_green;
	int			sb_blue;
	int			sb_radius;
	int			sb_takedamage;
	int			sb_takedamage2;

	float		lip;
	float		height;
	float		phase;

	int			wait_to_pickup;
	int			singlebot;
	int			tool_id;
	char		text;
	int			botskill;
	
	float		distance;
	int			type;
	int			vehicle;
	int			objectType;

	//entityplus variables
	gentity_t	*botspawn;
	char		*clientname;			// name of the bot to spawn for target_botspawn
	char		*mapname;				// name of the map to switch to for target_mapchange
	char		*teleporterTarget;		// forces a client to be teleported to the entity with this targetname when using holdable_teleporter. Also used as key for holdable_teleporter itself.
	int			logicEntities[MAX_LOGIC_ENTITIES];	//keeping track of entities targeting a target_logic
	char		*target2;	//second target
	char		*damagetarget;	//second target
	char		*targetname2; //second targetname
	char		*deathTarget;	// target to trigger when bot from target_botspawn dies
	char		*lootTarget;	//item to drop when bot from target_botspawn dies
	float		skill; // skill level set by target_skill
	char		*overlay; // reference to overlay texture for target_effect
	char		*key;	// key for target_modify to change
	char		*value; // value for target_modify to change to
	int			armor; // armor for the target_playerstats entity
	vec3_t		orgOrigin; // origin of entity (player) when cutscene starts
	char		*music; //path to music file(s) for target_music
	vec4_t		rgba1; //start color for target_effect fade
	vec4_t		rgba2; //end color for target_effect fade
	int			mtype;
	int			mtimeout;
	int			mhoming;
	int			mspeed;
	int			mbounce;
	int			mdamage;
	int			msdamage;
	int			msradius;
	int			mgravity;
	int			mnoclip;
	int			allowuse;
	
	int			swep_list[WEAPONS_NUM];
	int			swep_ammo[WEAPONS_NUM];
	int			swep_id;
	
	gentity_t 	*grabbedEntity;		//physgun object for player
	qboolean	isGrabbed;			//object is grabbed by player for prop
	float		grabDist;			//physgun distance for player
	vec3_t		grabOffset;			//physgun offset for player
	vec3_t		grabOldOrigin;		//physgun old origin for prop
	int			grabNewPhys;		//for freeze prop for prop
	gentity_t 	*lastPlayer;		//for damage and killfeed
	int			backpackContentsList[WEAPONS_NUM];
	int			backpackContentsAmmo[WEAPONS_NUM];
};


typedef enum {
	CON_DISCONNECTED,
	CON_CONNECTING,
	CON_CONNECTED
} clientConnected_t;

typedef enum {
	SPECTATOR_NOT,
	SPECTATOR_FREE,
	SPECTATOR_FOLLOW,
	SPECTATOR_SCOREBOARD
} spectatorState_t;

typedef enum {
	TEAM_BEGIN,		// Beginning a team game, spawn at base
	TEAM_ACTIVE		// Now actively playing
} playerTeamStateState_t;

typedef struct {
	playerTeamStateState_t	state;

	int			location;

	int			captures;
	int			basedefense;
	int			carrierdefense;
	int			flagrecovery;
	int			fragcarrier;
	int			assists;

	float		lasthurtcarrier;
	float		lastreturnedflag;
	float		flagsince;
	float		lastfraggedcarrier;
} playerTeamState_t;

// the auto following clients don't follow a specific client
// number, but instead follow the first two active players
#define	FOLLOW_ACTIVE1	-1
#define	FOLLOW_ACTIVE2	-2

// client data that stays across multiple levels or tournament restarts
// this is achieved by writing all the data to cvar strings at game shutdown
// time and reading them back at connection time.  Anything added here
// MUST be dealt with in G_InitSessionData() / G_ReadSessionData() / G_WriteSessionData()
typedef struct {
	team_t		sessionTeam;
	int			spectatorNum;		// for determining next-in-line to play
	spectatorState_t	spectatorState;
	int			spectatorClient;	// for chasecam and follow mode
	int			wins, losses;		// tournament stats
	qboolean	teamLeader;			// true when this client is a team leader
} clientSession_t;

//
#define	MAX_VOTE_COUNT		"3"

//unlagged - true ping
#define NUM_PING_SAMPLES 64
//unlagged - true ping

// client data that stays across multiple respawns, but is cleared
// on each level change or team change at ClientBegin()
typedef struct {
	clientConnected_t	connected;
	usercmd_t	cmd;				// we would lose angles if not persistant
	qboolean	localClient;		// true if "ip" info key is "localhost"
	qboolean	initialSpawn;		// the first spawn should be at a cool location
	qboolean	predictItemPickup;	// based on cg_predictItems userinfo
	qboolean	pmoveFixed;			//
	char		netname[MAX_NETNAME];
	int			maxHealth;			// for handicapping
	int			enterTime;			// level.time the client entered the game
	playerTeamState_t teamState;	// status in teamplay games
	int			voteCount;			// to prevent people from constantly calling votes
	int			teamVoteCount;		// to prevent people from constantly calling votes
	qboolean	teamInfo;			// send team overlay updates?

	//elimination:
	int		roundReached;			//Only spawn if we are new to this round
	int		livesLeft;			//lives in LMS
	int			delag;
	int			cmdTimeNudge;
	int			realPing;
	int			pingsamples[NUM_PING_SAMPLES];
	int			samplehead;
	
    int         oldmoney;
} clientPersistant_t;

//unlagged - backward reconciliation #1
// the size of history we'll keep
#define NUM_CLIENT_HISTORY 17

// everything we need to know to backward reconcile
typedef struct {
	vec3_t		mins, maxs;
	vec3_t		currentOrigin;
	int			leveltime;
} clientHistory_t;
//unlagged - backward reconciliation #1

// this structure is cleared on each ClientSpawn(),
// except for 'client->pers' and 'client->sess'
struct gclient_s {
	// ps MUST be the first element, because the server expects it
	playerState_t	ps;				// communicated by server to clients

	// the rest of the structure is private to game
	clientPersistant_t	pers;
	clientSession_t		sess;

	qboolean	readyToExit;		// wishes to leave the intermission
	qboolean	noclip;

	int			lastCmdTime;		// level.time of last usercmd_t, for EF_CONNECTION
									// we can't just use pers.lastCommand.time, because
									// of the g_sycronousclients case
	int			buttons;
	int			oldbuttons;
	int			latched_buttons;

	vec3_t		oldOrigin;

	// sum up damage over an entire frame, so
	// shotgun blasts give a single big kick
	int			damage_armor;		// damage absorbed by armor
	int			damage_blood;		// damage taken out of health
	int			damage_knockback;	// impact damage
	vec3_t		damage_from;		// origin for vector calculation
	qboolean	damage_fromWorld;	// if true, don't use the damage_from vector

	int			accurateCount;		// for "impressive" reward sound

	int			accuracy_shots;		// total number of shots
	int			accuracy_hits;		// total number of hits

	//
	int			lastkilled_client;	// last client that this client killed
	int			lasthurt_client;	// last client that damaged this client
	int			lasthurt_mod;		// type of damage the client did

	// timers
	int			respawnTime;		// can respawn when time > this, force after g_forcerespwan
	int			inactivityTime;		// kick players when time > this
	qboolean	inactivityWarning;	// qtrue if the five second warning has been given
	int			rewardTime;			// clear the EF_AWARD_IMPRESSIVE, etc when time > this

	int			airOutTime;

	int			lastKillTime;		// for multiple kill rewards

	qboolean	fireHeld;			// used for hook
	gentity_t	*hook;				// grapple hook if out
	gentity_t	*lasersight;		// flashlight

	int			switchTeamTime;		// time the player switched teams

	// timeResidual is used to handle events that happen every second
	// like health / armor countdowns and regeneration
	int			timeResidual;

	gentity_t	*persistantPowerup;
	int			portalID;
	int			ammoTimes[MAX_WEAPONS];
	int			invulnerabilityTime;

	char		*areabits;

	// NEW VARIABLES
	qboolean	isEliminated;			// Has been killed in this round
	
	// New vote system. The votes are saved in the client info, so we know who voted on what and can cancel votes on leave.
	// 0=not voted, 1=voted yes, -1=voted no
	int vote;
	
	int lastSentFlying;                             // The last client that sent the player flying
	int lastSentFlyingTime;                         // So we can time out

	// unlagged - backward reconciliation #1
	// the serverTime the button was pressed
	// (stored before pmove_fixed changes serverTime)
	int			attackTime;
	// the head of the history queue
	int			historyHead;
	// the history queue
	clientHistory_t	history[NUM_CLIENT_HISTORY];
	// the client's saved position
	clientHistory_t	saved;			// used to restore after time shift
	// an approximation of the actual server time we received this
	// command (not in 50ms increments)
	int			frameOffset;

	int			vehiclenum;

	// unlagged - smooth clients #1
	// the last frame number we got an update from this client
	int			lastUpdateFrame;
	qboolean        spawnprotected;

	int			accuracy[MAX_WEAPONS][2];
};

//
// this structure is cleared as each map is entered
//
#define	MAX_SPAWN_VARS			64
#define	MAX_SPAWN_VARS_CHARS	4096

typedef struct {
	struct gclient_s	*clients;		// [maxclients]

	struct gentity_s	*gentities;
	int			gentitySize;
	int			num_entities;		// current number, <= MAX_GENTITIES

	int			warmupTime;			// restart match at this time

	fileHandle_t	logFile;

	// store latched cvars here that we want to get at often
	int			maxclients;

	int			framenum;
	int			time;					// in msec
	int			previousTime;			// so movers can back up when blocked

	int			startTime;				// level.time the map was started

	int			teamScores[TEAM_NUM_TEAMS];
	int			lastTeamLocationTime;		// last time of client team location update

	qboolean	newSession;				// don't use any old session data, because
										// we changed gametype

	qboolean	restarted;				// waiting for a map_restart to fire

	int			numConnectedClients;
	int			numNonSpectatorClients;	// includes connecting clients
	int			numPlayingClients;		// connected, non-spectators
	int			sortedClients[MAX_CLIENTS];		// sorted by score
	int			follow1, follow2;		// clientNums for auto-follow spectators

	int			snd_fry;				// sound index for standing in lava

	int			warmupModificationCount;	// for detecting if g_warmup is changed

	// voting state
	char		voteString[MAX_STRING_CHARS];
	char		voteDisplayString[MAX_STRING_CHARS];
	int			voteTime;				// level.time vote was called
	int			voteExecuteTime;		// time the vote is executed
	int			voteYes;
	int			voteNo;
	int			numVotingClients;		// set by CountVotes
        int             voteKickClient;                         // if non-negative the current vote is about this client.
        int             voteKickType;                           // if 1 = ban (execute ban)

	// team voting state
	char		teamVoteString[2][MAX_STRING_CHARS];
	int			teamVoteTime[2];		// level.time vote was called
	int			teamVoteYes[2];
	int			teamVoteNo[2];
	int			numteamVotingClients[TEAM_NUM_TEAMS];// set by CalculateRanks

	// spawn variables
	qboolean	spawning;				// the G_Spawn*() functions are valid
	int			numSpawnVars;
	char		*spawnVars[MAX_SPAWN_VARS][2];	// key / value pairs
	int			numSpawnVarChars;
	char		spawnVarChars[MAX_SPAWN_VARS_CHARS];

	// intermission state
	int			intermissionQueued;		// intermission was qualified, but
										// wait INTERMISSION_DELAY_TIME before
										// actually going there so the last
										// frag can be watched.  Disable future
										// kills during this delay
	int			intermissiontime;		// time the intermission was started
	char		*changemap;
	qboolean	readyToExit;			// at least one client wants to exit
	int			exitTime;
	vec3_t		intermission_origin;	// also used for spectator spawns
	vec3_t		intermission_angle;

	qboolean	locationLinked;			// target_locations get linked
	gentity_t	*locationHead;			// head of the location list
	int			bodyQueIndex;			// dead bodies
	gentity_t	*bodyQue[BODY_QUEUE_SIZE];
	int			portalSequence;
	//Added for elimination:
	int roundStartTime;		//time the current round was started
	int roundNumber;			//The round number we have reached
	int roundNumberStarted;			//1 less than roundNumber if we are allowed to spawn
	int roundRedPlayers;			//How many players was there at start of round
	int roundBluePlayers;			//used to find winners in a draw.
	qboolean roundRespawned;		//We have respawned for this round!
	int eliminationSides;			//Random, change red/blue bases

	//Added for Double Domination
	//Points get status: TEAM_FREE for not taking, TEAM_RED/TEAM_BLUE for taken and TEAM_NONE for not spawned yet
	int pointStatusA;			//Status of the RED (A) domination point
	int pointStatusB;			//Status of the BLUE (B) doimination point
	int timeTaken;				//Time team started having both points
	//use roundStartTime for telling, then the points respawn

	//Added for standard domination
	int pointStatusDom[MAX_DOMINATION_POINTS]; //Holds the owner of all the points
	int dom_scoreGiven;				//Number of times we have provided scores
	int domination_points_count;
	char domination_points_names[MAX_DOMINATION_POINTS][MAX_DOMINATION_POINTS_NAMES];

	//unlagged - backward reconciliation #4
	// actual time this server frame started
	int			frameStartTime;
	//unlagged - backward reconciliation #4

    //Obelisk tell
    int healthRedObelisk; //health in percent
    int healthBlueObelisk; //helth in percent
    qboolean MustSendObeliskHealth; //Health has changed
	gentity_t	*player;				// refers to the player in SP mode. Provides quick access to the player entity
	char		scoreLevelName[64];
	int			secretCount;			// number of target_secret entities in map
} level_locals_t;

extern int				SourceTechEntityList[MAX_GENTITIES];

//
// g_spawn.c
//
qboolean	G_SpawnString( const char *key, const char *defaultString, char **out );
// spawn string returns a temporary reference, you must CopyString() if you want to keep it
qboolean	G_SpawnFloat( const char *key, const char *defaultString, float *out );
qboolean	G_SpawnInt( const char *key, const char *defaultString, int *out );
qboolean	G_SpawnVector( const char *key, const char *defaultString, float *out );
qboolean	G_SpawnVector4( const char *key, const char *defaultString, float *out );
void		G_SpawnEntitiesFromString( void );
char *G_NewString( const char *string );

//
// g_cmds.c
//
void Cmd_Score_f (gentity_t *ent);
void StopFollowing( gentity_t *ent );
void BroadcastTeamChange( gclient_t *client, int oldTeam );
void SetTeam( gentity_t *ent, char *s );
void Cmd_FollowCycle_f( gentity_t *ent );  //KK-OAX Changed to match definition
char *ConcatArgs( int start );  //KK-OAX This declaration moved from g_svccmds.c
//KK-OAX Added this to make accessible from g_svcmds_ext.c
void G_Say( gentity_t *ent, gentity_t *target, int mode, const char *chatText );

// KK-OAX Added this for common file stuff between Admin and Sprees.
// g_fileops.c
//
void readFile_int( char **cnf, int *v );
void readFile_string( char **cnf, char *s, int size );
void writeFile_int( int v, fileHandle_t f );
void writeFile_string( char *s, fileHandle_t f );

//
// g_items.c
//
void G_CheckTeamItems( void );
void G_RunItem( gentity_t *ent );
void RespawnItem( gentity_t *ent );
void RespawnItemCtf( gentity_t *ent );

void UseHoldableItem( gentity_t *ent );
void PrecacheItem (gitem_t *it);
gentity_t *Drop_Item( gentity_t *ent, gitem_t *item, float angle );
gentity_t *LaunchItem( gitem_t *item, vec3_t origin, vec3_t velocity );
gentity_t *LaunchBackpack( gitem_t *item, gentity_t *self, vec3_t velocity );
void SetRespawn (gentity_t *ent, float delay);
void G_SpawnItem (gentity_t *ent, gitem_t *item);
void FinishSpawningItem( gentity_t *ent );
void Think_Weapon (gentity_t *ent);
int ArmorIndex (gentity_t *ent);
void	Add_Ammo (gentity_t *ent, int weapon, int count);
void	Set_Ammo (gentity_t *ent, int weapon, int count);
void	Set_Weapon (gentity_t *ent, int weapon, int count);
void Touch_Item (gentity_t *ent, gentity_t *other, trace_t *trace );
void Touch_Item2(gentity_t *ent, gentity_t *other, trace_t *trace, qboolean allowBot);

void ClearRegisteredItems( void );
void RegisterItem( gitem_t *item );
void SaveRegisteredItems( void );

// oatmeal begin
gentity_t *Throw_Item( gentity_t *ent, gitem_t *item, float angle );
// oatmeal end

//
// g_utils.c
//
int G_ModelIndex( char *name );
int		G_SoundIndex( char *name );
void	G_TeamCommand( team_t team, char *cmd );
void	G_KillBox (gentity_t *ent);
gentity_t *G_Find (gentity_t *from, int fieldofs, const char *match);
gentity_t *G_PickTarget (char *targetname);
void	G_UseTargets (gentity_t *ent, gentity_t *activator);
void	G_UseDeathTargets (gentity_t *ent, gentity_t *activator);
void	G_SetMovedir ( vec3_t angles, vec3_t movedir);
char	*G_GetScoringMapName();
void	G_Fade( float duration, vec4_t startColor, vec4_t endColor, int clientn );
void	G_FadeOut( float duration, int clientn );
void	G_FadeIn( float duration, int clientn );
playerscore_t G_CalculatePlayerScore( gentity_t *ent );
void botsandbox_check (gentity_t *self);
void VehiclePhys( gentity_t *self );
gentity_t *FindEntityForPhysgun( gentity_t *ent, int range );
gentity_t *FindEntityForGravitygun( gentity_t *ent, int range );
void CrosshairPointPhys(gentity_t *ent, int range, vec3_t outPoint);
void CrosshairPointGravity(gentity_t *ent, int range, vec3_t outPoint);

void	G_InitGentity( gentity_t *e );
gentity_t *findradius (gentity_t *ent, vec3_t org, float rad);
gentity_t	*G_Spawn (void);
gentity_t *G_TempEntity( vec3_t origin, int event );
void	G_Sound( gentity_t *ent, int channel, int soundIndex );

//KK-OAX For Playing Sounds Globally
void    G_GlobalSound( int soundIndex );

void	    G_FreeEntity( gentity_t *e );
qboolean	G_EntitiesFree( void );

void	G_TouchTriggers (gentity_t *ent);
void	G_TouchSolids (gentity_t *ent);

float	*tv (float x, float y, float z);
char	*vtos( const vec3_t v );

void G_AddPredictableEvent( gentity_t *ent, int event, int eventParm );
void G_AddEvent( gentity_t *ent, int event, int eventParm );
void G_SetOrigin( gentity_t *ent, vec3_t origin );
void G_SetTarget( gentity_t *ent, char *targ );
void G_SetTargetname( gentity_t *ent, char *targname );
void AddRemap(const char *oldShader, const char *newShader, float timeOffset);
const char *BuildShaderStateConfig( void );


void target_finish_think(gentity_t* self);
void target_finish_use (gentity_t *self, gentity_t *other, gentity_t *activator);

//
// g_combat.c
//
qboolean CanDamage (gentity_t *targ, vec3_t origin);
void G_Damage (gentity_t *targ, gentity_t *inflictor, gentity_t *attacker, vec3_t dir, vec3_t point, int damage, int dflags, int mod);
void G_PropDamage (gentity_t *targ, gentity_t *attacker, int damage);
void G_CarDamage (gentity_t *targ, gentity_t *attacker, int damage);
void G_ExitVehicle (int num);
qboolean G_RadiusDamage (vec3_t origin, gentity_t *attacker, float damage, float radius, gentity_t *ignore, int mod);
int G_InvulnerabilityEffect( gentity_t *targ, vec3_t dir, vec3_t point, vec3_t impactpoint, vec3_t bouncedir );
void body_die( gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int meansOfDeath );
void TossClientItems( gentity_t *self );
void TossClientPersistantPowerups( gentity_t *self );
void TossClientCubes( gentity_t *self );

// damage flags
#define DAMAGE_RADIUS				0x00000001	// damage was indirect
#define DAMAGE_NO_ARMOR				0x00000002	// armour does not protect from this damage
#define DAMAGE_NO_KNOCKBACK			0x00000004	// do not affect velocity, just view angles
#define DAMAGE_NO_PROTECTION		0x00000008  // armor, shields, invulnerability, and godmode have no effect
#define DAMAGE_NO_TEAM_PROTECTION	0x00000010  // armor, shields, invulnerability, and godmode have no effect

//
// g_missile.c
//
void G_RunMissile( gentity_t *ent );
void ProximityMine_RemoveAll( void );

gentity_t *fire_blaster (gentity_t *self, vec3_t start, vec3_t aimdir);
gentity_t *fire_custom (gentity_t *self, vec3_t start, vec3_t dir);
gentity_t *fire_plasma (gentity_t *self, vec3_t start, vec3_t aimdir);
gentity_t *fire_grenade (gentity_t *self, vec3_t start, vec3_t aimdir);
gentity_t *fire_rocket (gentity_t *self, vec3_t start, vec3_t dir);
gentity_t *fire_bfg (gentity_t *self, vec3_t start, vec3_t dir);
gentity_t *fire_grapple (gentity_t *self, vec3_t start, vec3_t dir);
gentity_t *fire_nail( gentity_t *self, vec3_t start, vec3_t forward, vec3_t right, vec3_t up );
gentity_t *fire_prox( gentity_t *self, vec3_t start, vec3_t aimdir );
gentity_t *fire_flame (gentity_t *self, vec3_t start, vec3_t aimdir);
gentity_t *fire_antimatter (gentity_t *self, vec3_t start, vec3_t aimdir);
gentity_t *fire_thrower( gentity_t *self, vec3_t start, vec3_t forward, vec3_t right, vec3_t up );
gentity_t *fire_bouncer( gentity_t *self, vec3_t start, vec3_t forward, vec3_t right, vec3_t up );
gentity_t *fire_exploder (gentity_t *self, vec3_t start, vec3_t dir);
gentity_t *fire_knocker( gentity_t *self, vec3_t start, vec3_t forward, vec3_t right, vec3_t up );
gentity_t *fire_regenerator (gentity_t *self, vec3_t start, vec3_t dir);
gentity_t *fire_propgun( gentity_t *self, vec3_t start, vec3_t forward, vec3_t right, vec3_t up );
gentity_t *fire_nuke( gentity_t *self, vec3_t start, vec3_t forward, vec3_t right, vec3_t up );

//
// g_mover.c
//
void G_RunMover( gentity_t *ent );
void Touch_DoorTrigger( gentity_t *ent, gentity_t *other, trace_t *trace );
void Break_Breakable(gentity_t *ent, gentity_t *other);

//
// g_trigger.c
//
void trigger_teleporter_touch (gentity_t *self, gentity_t *other, trace_t *trace );
void lock_touch( gentity_t *self, gentity_t *other, trace_t *trace );


//
// g_misc.c
//
void TeleportPlayer( gentity_t *player, vec3_t origin, vec3_t angles );
void DropPortalSource( gentity_t *ent );
void DropPortalDestination( gentity_t *ent );
void G_ModProp( gentity_t *targ, gentity_t *attacker, char *arg01, char *arg02, char *arg03, char *arg04, char *arg05, char *arg06, char *arg07, char *arg08, char *arg09, char *arg10, char *arg11, char *arg12, char *arg13, char *arg14, char *arg15, char *arg16, char *arg17, char *arg18, char *arg19 );
void G_RunProp( gentity_t *ent );
void G_BounceProp( gentity_t *ent, trace_t *trace );
void G_HideObjects();
void G_ShowObjects();
gentity_t *G_FindEntityForEntityNum(int entityn);
gentity_t *G_FindEntityForClientNum(int entityn);
void BlockDie (gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod);


//
// g_weapon.c
//
qboolean LogAccuracyHit( gentity_t *target, gentity_t *attacker );
void CalcMuzzlePoint ( gentity_t *ent, vec3_t forward, vec3_t right, vec3_t up, vec3_t muzzlePoint );
//unlagged - attack prediction #3
// we're making this available to both games
void SnapVectorTowards( vec3_t v, vec3_t to );
//unlagged - attack prediction #3
qboolean CheckGauntletAttack( gentity_t *ent );
void Weapon_HookFree (gentity_t *ent);
void Weapon_HookThink (gentity_t *ent);
	void Laser_Gen (gentity_t *ent);
	void Laser_Think( gentity_t *self );

//unlagged - g_unlagged.c
void G_ResetHistory( gentity_t *ent );
void G_StoreHistory( gentity_t *ent );
void G_TimeShiftAllClients( int time, gentity_t *skip );
void G_UnTimeShiftAllClients( gentity_t *skip );
void G_DoTimeShiftFor( gentity_t *ent );
void G_UndoTimeShiftFor( gentity_t *ent );
void G_UnTimeShiftClient( gentity_t *client );
void G_PredictPlayerMove( gentity_t *ent, float frametime );
//unlagged - g_unlagged.c

//
// g_client.c
//
team_t TeamCount( int ignoreClientNum, int team );
team_t TeamLivingCount( int ignoreClientNum, int team ); //Elimination
team_t TeamHealthCount( int ignoreClientNum, int team ); //Elimination
void RespawnAll(void); //For round elimination
void RespawnDead(void);
void EnableWeapons(void);
void DisableWeapons(void);
void EndEliminationRound(void);
void LMSpoint(void);
//void wins2score(void);
int TeamLeader( int team );
team_t PickTeam( int ignoreClientNum );
void SetClientViewAngle( gentity_t *ent, vec3_t angle );
gentity_t *SelectSpawnPoint ( vec3_t avoidPoint, vec3_t origin, vec3_t angles );
void CopyToBodyQue( gentity_t *ent );
void ClientRespawn(gentity_t *ent);
void BeginIntermission (void);
void InitClientPersistant (gclient_t *client);
void InitClientResp (gclient_t *client);
void InitBodyQue (void);
void ClientSpawn( gentity_t *ent );
void player_die (gentity_t *self, gentity_t *inflictor, gentity_t *attacker, int damage, int mod);
void AddScore( gentity_t *ent, vec3_t origin, int score );
void CalculateRanks( void );
qboolean SpotWouldTelefrag( gentity_t *spot );
qboolean SpawnPointIsActive( gentity_t *spot );

//
// g_svcmds.c
//
qboolean	ConsoleCommand( void );
void G_ProcessIPBans(void);
qboolean G_FilterPacket (char *from);

//KK-OAX Added this to make accessible from g_svcmds_ext.c
gclient_t	*ClientForString( const char *s );

//
// g_weapon.c
//
void FireWeapon( gentity_t *ent );
void KamikazeDamage( gentity_t *self );
void CarExplodeDamage( gentity_t *self );
void KamikazeRadiusDamage( vec3_t origin, gentity_t *attacker, float damage, float radius, int mod );
void KamikazeShockWave( vec3_t origin, gentity_t *attacker, float damage, float push, float radius, int mod );
void G_StartKamikaze( gentity_t *ent );
void G_StartCarExplode( gentity_t *ent );
void G_StartNukeExplode( gentity_t *ent );

//
// p_hud.c
//
void MoveClientToIntermission (gentity_t *client);
void G_SetStats (gentity_t *ent);
void DeathmatchScoreboardMessage (gentity_t *client);

//
// g_cmds.c
// Also another place /Sago

void DoubleDominationScoreTimeMessage( gentity_t *ent );
void AttackingTeamMessage( gentity_t *ent );
void ObeliskHealthMessage( void );
void DeathmatchScoreboardMessage (gentity_t *client);
void EliminationMessage (gentity_t *client);
void RespawnTimeMessage(gentity_t *ent, int time);
void DominationPointNamesMessage (gentity_t *client);
void DominationPointStatusMessage( gentity_t *ent );

//
// g_pweapon.c
//


//
// g_main.c
//
void FindIntermissionPoint( void );
void SetLeader(int team, int client);
void CheckTeamLeader( int team );
void G_RunThink (gentity_t *ent);
void AddTournamentQueue(gclient_t *client);
void ExitLevel( void );
void QDECL G_LogPrintf( const char *fmt, ... );
void SendScoreboardMessageToAllClients( void );
void SendEliminationMessageToAllClients( void );
void SendDDtimetakenMessageToAllClients( void );
void SendDominationPointsStatusMessageToAllClients( void );
void QDECL G_Printf( const char *fmt, ... );
void QDECL G_Error( const char *fmt, ... ) __attribute__((noreturn));
//KK-OAX Made Accessible for g_admin.c
void LogExit( const char *string );
void CheckTeamVote( int team );
void G_LevelLoadComplete(void);
qboolean G_NpcFactionProp(int prop, gentity_t* ent);

//
// g_client.c
//
char *ClientConnect( int clientNum, qboolean firstTime, qboolean isBot );
void ClientUserinfoChanged( int clientNum );
void ClientDisconnect( int clientNum );
void ClientBegin( int clientNum );
void ClientCommand( int clientNum );
void DropClientSilently( int clientNum );
void LinkBotSpawnEntity( gentity_t *bot, char parentid[] );
void SetupCustomBot( gentity_t *bot );
void SetUnlimitedWeapons( gentity_t *ent );
void SetSandboxWeapons( gentity_t *ent );
void SetCustomWeapons( gentity_t *ent );
void PrecacheBotAssets();

//
// g_active.c
//
void ClientThink( int clientNum );
void ClientEndFrame( gentity_t *ent );
void G_RunClient( gentity_t *ent );
qboolean G_CheckSwep( int clientNum, int wp, int finish );
int G_CheckSwepAmmo( int clientNum, int wp );
void G_DefaultSwep( int clientNum, int wp );

//
// g_team.c
//
qboolean OnSameTeam( gentity_t *ent1, gentity_t *ent2 );
void Team_CheckDroppedItem( gentity_t *dropped );
qboolean CheckObeliskAttack( gentity_t *obelisk, gentity_t *attacker );
void ShuffleTeams(void);
//KK-OAX Added for Command Handling Changes (r24)
team_t G_TeamFromString( char *str );

//KK-OAX Removed these in Code in favor of bg_alloc.c from Tremulous
// g_mem.c
//
//void *G_Alloc( int size );
//void G_InitMemory( void );

//KK-OAX This was moved
// bg_alloc.c
//
void Svcmd_GameMem_f( void );

//
// g_session.c
//
void G_ReadSessionData( gclient_t *client );
void G_InitSessionData( gclient_t *client, char *userinfo );

void G_InitWorldSession( void );
void G_WriteSessionData( void );

//
// g_arenas.c
//
void UpdateTournamentInfo( void );
void SpawnModelsOnVictoryPads( void );

//
// g_bot.c
//
void G_InitBots( qboolean restart );
char *G_GetBotInfoByNumber( int num );
char *G_GetBotInfoByName( const char *name );
void G_CheckBotSpawn( void );
void G_RemoveQueuedBotBegin( int clientNum );
qboolean G_BotConnect( int clientNum, qboolean restart );
void Svcmd_AddBot_f( void );
void Svcmd_BotList_f( void );
void BotInterbreedEndMatch( void );

//
// g_playerstore.c
//

void PlayerStoreInit( void );
void PlayerStore_store(char* guid, playerState_t ps);
void PlayerStore_restore(char* guid, playerState_t *ps);

//
// g_vote.c
//
int allowedVote(char *commandStr);
void CheckVote( void );
void CountVotes( void );
void ClientLeaving(int clientNumber);

#define MAX_MAPNAME 32
#define MAPS_PER_PAGE 10
#define MAX_MAPNAME_BUFFER MAX_MAPNAME*600
#define MAX_MAPNAME_LENGTH 34
#define MAX_CUSTOMNAME  MAX_MAPNAME
#define MAX_CUSTOMCOMMAND 100
#define MAX_CUSTOMDISPLAYNAME 50

typedef struct {
	int pagenumber;
	char mapname[MAPS_PER_PAGE][MAX_MAPNAME];
} t_mappage;

extern t_mappage getMappage(int page);
extern int allowedMap(char *mapname);
extern int allowedGametype(char *gametypeStr);
extern int allowedTimelimit(int limit);
extern int allowedFraglimit(int limit);

//
// g_mapcycle.c
//
void G_GotoNextMapCycle ( void );
char *G_GetNextMapCycle ( char *map );
char *G_GetNextMap ( char *map );
void G_GetMapfile ( char *map );
qboolean G_mapIsVoteable ( char* map );
void G_drawAllowedMaps ( gentity_t *ent );
void G_drawMapcycle ( gentity_t *ent );
void G_sendMapcycle( void );
void G_LoadMapcycle ( void );
qboolean SkippedChar ( char in );
int G_GetMapLockArena ( char *map );

//
// g_mapfiles.c
//
qboolean G_ClassnameAllowed( char *input );
qboolean G_ClassnameAllowedAll( char *input );
void G_WriteMapfile_f( void );
void G_WriteMapfileAll_f( void );
void G_LoadMapfile( char *filename );
void G_LoadMapfileAll( char *filename );
void G_LoadMapfile_f( void );
void G_LoadMapfileAll_f( void );

// ai_main.c
#define MAX_FILEPATH			144

//bot settings
typedef struct bot_settings_s
{
	char characterfile[MAX_FILEPATH];
	float skill;
	char team[MAX_FILEPATH];
	char waypoint[MAX_TOKEN_CHARS];
} bot_settings_t;

int BotAISetup( int restart );
int BotAIShutdown( int restart );
int BotAILoadMap( int restart );
int BotAISetupClient(int client, struct bot_settings_s *settings, qboolean restart);
int BotAIShutdownClient( int client, qboolean restart );
int BotAIStartFrame( int time );
void BotTestAAS(vec3_t origin);

#include "g_team.h" // teamplay specific stuff


extern	level_locals_t	level;
extern	gentity_t		g_entities[MAX_GENTITIES];

#define	FOFS(x) ((size_t)&(((gentity_t *)0)->x))
#define	FOFSCP(x) ((size_t)&(((clientPersistant_t *)0)->x))

//CVARS
void G_SendWeaponProperties( gentity_t *ent );
void G_SendSwepWeapons( gentity_t *ent );
void G_SendSpawnSwepWeapons( gentity_t *ent );
void plasma_think( gentity_t *ent );
void rocket_think( gentity_t *ent );
void grenade_think( gentity_t *ent );
void bfg_think( gentity_t *ent );
void nailgun_think( gentity_t *ent );
void 	Newmodcommands( void );
extern	int 		CustomModRun;
extern	char 		cmapname[64];
extern	int			mod_ammolimit;
extern 	int 		mod_jumpheight;
extern 	int			mod_gdelay;
extern 	int			mod_mgdelay;
extern	int			mod_mgspread;
extern 	int			mod_sgdelay;
extern	int			mod_sgspread;
extern	int			mod_sgcount;
extern 	int			mod_gldelay;
extern 	int			mod_rldelay;
extern 	int			mod_lgdelay;
extern	int			mod_lgrange;
extern 	int			mod_pgdelay;
extern 	int			mod_rgdelay;
extern 	int			mod_bfgdelay;
extern 	int			mod_ngdelay;
extern 	int			mod_pldelay;
extern 	int			mod_cgdelay;
extern	int			mod_cgspread;
extern 	int			mod_ftdelay;
extern 	int			mod_amdelay;
extern	int			mod_vampire_max_health;
extern	float 		mod_hastefirespeed;
extern	float 		mod_ammoregenfirespeed;
extern	float 		mod_scoutfirespeed;
extern	int			mod_poweruptime;
extern	float			mod_guardfirespeed;
extern	float			mod_doublerfirespeed;
extern	int			mod_quadtime;
extern	int			mod_bsuittime;
extern	int			mod_hastetime;
extern	int			mod_invistime;
extern	int			mod_regentime;
extern	int			mod_flighttime;
extern	int			mod_noplayerclip;
extern	int			mod_ammolimit;
extern	int			mod_invulmove;
extern	float			mod_teamred_firespeed;
extern	float			mod_teamblue_firespeed;
extern	int			mod_medkitlimit;
extern	int			mod_medkitinf;
extern	int			mod_teleporterinf;
extern	int			mod_portalinf;
extern	int			mod_kamikazeinf;
extern	int			mod_invulinf;
extern	int 		mod_teamblue_damage;
extern	int 		mod_teamred_damage;
extern	int			mod_accelerate;
extern	int			mod_slickmove;
extern	int			mod_overlay;
extern	int			mod_gravity;

extern	vmCvar_t 	cl_propsmallsizescale;
extern	vmCvar_t 	cl_propheight;
extern	vmCvar_t 	cl_propspacewidth;
extern	vmCvar_t 	cl_propgapwidth;
extern	vmCvar_t 	cl_smallcharwidth;
extern	vmCvar_t 	cl_smallcharheight;
extern	vmCvar_t 	cl_bigcharwidth;
extern	vmCvar_t 	cl_bigcharheight;
extern	vmCvar_t 	cl_giantcharwidth;
extern	vmCvar_t 	cl_giantcharheight;

extern	vmCvar_t	g_physimpact;
extern	vmCvar_t	g_physimpulse;
extern	vmCvar_t	g_physdamage;

//QS settings
extern	vmCvar_t	g_minigame;
//gh set
extern	vmCvar_t	g_ghspeed;
extern	vmCvar_t	g_ghtimeout;
extern	int 		mod_ghtimeout;
//g set
extern	vmCvar_t	g_gdelay;
extern	vmCvar_t	g_gdamage;
extern	vmCvar_t	g_grange;
extern	vmCvar_t	g_gknockback;
//mg set
extern	vmCvar_t	g_mgdelay;
extern	vmCvar_t	g_mgdamage;
extern	vmCvar_t	g_mgspread;
extern	vmCvar_t	g_mgexplode;
extern	vmCvar_t	g_mgsdamage;
extern	vmCvar_t	g_mgsradius;
extern	vmCvar_t	g_mgvampire;
extern	vmCvar_t	g_mginf;
extern	vmCvar_t	g_mgknockback;
//sg set
extern	vmCvar_t	g_sgdelay;
extern	vmCvar_t	g_sgdamage;
extern	vmCvar_t	g_sgspread;
extern	vmCvar_t	g_sgexplode;
extern	vmCvar_t	g_sgsdamage;
extern	vmCvar_t	g_sgsradius;
extern	vmCvar_t	g_sgcount;
extern	vmCvar_t	g_sgvampire;
extern	vmCvar_t	g_sginf;
extern	vmCvar_t	g_sgknockback;
//gl set
extern	vmCvar_t	g_gldelay;
extern	vmCvar_t	g_glspeed;
extern	vmCvar_t	g_gltimeout;
extern	vmCvar_t	g_glsradius;
extern	vmCvar_t	g_glsdamage;
extern	vmCvar_t	g_gldamage;
extern	vmCvar_t	g_glbounce;
extern	vmCvar_t	g_glgravity;
extern	vmCvar_t	g_glvampire;
extern	vmCvar_t	g_glinf;
extern	vmCvar_t	g_glbouncemodifier;
extern	vmCvar_t	g_glknockback;
//rl set
extern	vmCvar_t	g_rldelay;
extern	vmCvar_t	g_rlspeed;
extern	vmCvar_t	g_rltimeout;
extern	vmCvar_t	g_rlsradius;
extern	vmCvar_t	g_rlsdamage;
extern	vmCvar_t	g_rldamage;
extern	vmCvar_t	g_rlbounce;
extern	vmCvar_t	g_rlgravity;
extern	vmCvar_t	g_rlvampire;
extern	vmCvar_t	g_rlinf;
extern	vmCvar_t	g_rlbouncemodifier;
extern	vmCvar_t	g_rlknockback;
//lg set
extern	vmCvar_t	g_lgdamage;
extern	vmCvar_t	g_lgdelay;
extern	vmCvar_t	g_lgrange;
extern	vmCvar_t	g_lgexplode;
extern	vmCvar_t	g_lgsdamage;
extern	vmCvar_t	g_lgsradius;
extern	vmCvar_t	g_lgvampire;
extern	vmCvar_t	g_lginf;
extern	vmCvar_t	g_lgknockback;
//rg set
extern	vmCvar_t	g_rgdelay;
extern	vmCvar_t	g_rgdamage;
extern	vmCvar_t	g_rgvampire;
extern	vmCvar_t	g_rginf;
extern	vmCvar_t	g_rgknockback;
//pg set
extern	vmCvar_t	g_pgdelay;
extern	vmCvar_t	g_pgsradius;
extern	vmCvar_t	g_pgspeed;
extern	vmCvar_t	g_pgsdamage;
extern	vmCvar_t	g_pgdamage;
extern	vmCvar_t	g_pgtimeout;
extern	vmCvar_t	g_pgbounce;
extern	vmCvar_t	g_pggravity;
extern	vmCvar_t	g_pgvampire;
extern	vmCvar_t	g_pginf;
extern	vmCvar_t	g_pgbouncemodifier;
extern	vmCvar_t	g_pgknockback;
//bfg set
extern	vmCvar_t	g_bfgdelay;
extern	vmCvar_t	g_bfgspeed;
extern	vmCvar_t	g_bfgtimeout;
extern	vmCvar_t	g_bfgsradius;
extern	vmCvar_t	g_bfgsdamage;
extern	vmCvar_t	g_bfgdamage;
extern	vmCvar_t	g_bfgbounce;
extern	vmCvar_t	g_bfggravity;
extern	vmCvar_t	g_bfgvampire;
extern	vmCvar_t	g_bfginf;
extern	vmCvar_t	g_bfgbouncemodifier;
extern	vmCvar_t	g_bfgknockback;
//ng set
extern	vmCvar_t	g_ngdelay;
extern	vmCvar_t	g_ngspeed;
extern	vmCvar_t	g_ngspread;
extern	vmCvar_t	g_ngdamage;
extern	vmCvar_t	g_ngtimeout;
extern	vmCvar_t	g_ngcount;
extern	vmCvar_t	g_ngbounce;
extern	vmCvar_t	g_nggravity;
extern	vmCvar_t	g_ngrandom;
extern	vmCvar_t	g_ngvampire;
extern	vmCvar_t	g_nginf;
extern	vmCvar_t	g_ngbouncemodifier;
extern	vmCvar_t	g_ngknockback;
//pl set
extern	vmCvar_t	g_pldelay;
extern	vmCvar_t	g_plspeed;
extern	vmCvar_t	g_pltimeout;
extern	vmCvar_t	g_plsradius;
extern	vmCvar_t	g_plsdamage;
extern	vmCvar_t	g_pldamage;
extern	vmCvar_t	g_plgravity;
extern	vmCvar_t	g_plvampire;
extern	vmCvar_t	g_plinf;
extern	vmCvar_t	g_plknockback;
//cg set
extern	vmCvar_t	g_cgdelay;
extern	vmCvar_t	g_cgdamage;
extern	vmCvar_t	g_cgspread;
extern	vmCvar_t	g_cgvampire;
extern	vmCvar_t	g_cginf;
extern	vmCvar_t	g_cgknockback;
//ft set
extern	vmCvar_t	g_ftdelay;
extern	vmCvar_t	g_ftsradius;
extern	vmCvar_t	g_ftspeed;
extern	vmCvar_t	g_ftsdamage;
extern	vmCvar_t	g_ftdamage;
extern	vmCvar_t	g_fttimeout;
extern	vmCvar_t	g_ftbounce;
extern	vmCvar_t	g_ftgravity;
extern	vmCvar_t	g_ftvampire;
extern	vmCvar_t	g_ftinf;
extern	vmCvar_t	g_ftbouncemodifier;
extern	vmCvar_t	g_ftknockback;
//am set
extern	vmCvar_t	g_amdelay;
extern	vmCvar_t	g_amsradius;
extern	vmCvar_t	g_amspeed;
extern	vmCvar_t	g_amsdamage;
extern	vmCvar_t	g_amdamage;
extern	vmCvar_t	g_amtimeout;
extern	vmCvar_t	g_ambounce;
extern	vmCvar_t	g_amgravity;
extern	vmCvar_t	g_amvampire;
extern	vmCvar_t	g_aminf;
extern	vmCvar_t	g_ambouncemodifier;
extern	vmCvar_t	g_amknockback;
//guided and homing
extern	vmCvar_t	g_glhoming;
extern	vmCvar_t	g_glguided;
extern	vmCvar_t	g_rlhoming;
extern	vmCvar_t	g_rlguided;
extern	vmCvar_t	g_pghoming;
extern	vmCvar_t	g_pgguided;
extern	vmCvar_t	g_bfghoming;
extern	vmCvar_t	g_bfgguided;
extern	vmCvar_t	g_nghoming;
extern	vmCvar_t	g_ngguided;
extern	vmCvar_t	g_fthoming;
extern	vmCvar_t	g_ftguided;
extern	vmCvar_t	g_amhoming;
extern	vmCvar_t	g_amguided;
//rune s set
extern	vmCvar_t	g_scoutspeedfactor;
extern	vmCvar_t	g_scoutfirespeed;
extern	vmCvar_t	g_scoutdamagefactor;
extern	vmCvar_t	g_scoutgravitymodifier;
extern	vmCvar_t	g_scout_infammo;
extern	vmCvar_t	g_scouthealthmodifier;
//rune d set
extern	vmCvar_t	g_doublerfirespeed;
extern	vmCvar_t	g_doublerdamagefactor;
extern	vmCvar_t	g_doublerspeedfactor;
extern	vmCvar_t	g_doublergravitymodifier;
extern	vmCvar_t	g_doubler_infammo;
extern	vmCvar_t	g_doublerhealthmodifier;
//rune g set
extern	vmCvar_t	g_guardhealthmodifier;
extern	vmCvar_t	g_guardfirespeed;
extern	vmCvar_t	g_guarddamagefactor;
extern	vmCvar_t	g_guardspeedfactor;
extern	vmCvar_t	g_guardgravitymodifier;
extern	vmCvar_t	g_guard_infammo;
//rune a set
extern	vmCvar_t	g_ammoregenfirespeed;
extern	vmCvar_t	g_ammoregen_infammo;
extern	vmCvar_t	g_ammoregendamagefactor;
extern	vmCvar_t	g_ammoregenspeedfactor;
extern	vmCvar_t	g_ammoregengravitymodifier;
extern	vmCvar_t	g_ammoregenhealthmodifier;
//ammocount,s
extern	vmCvar_t	g_mgammocount;
extern	vmCvar_t	g_sgammocount;
extern	vmCvar_t	g_glammocount;
extern	vmCvar_t	g_rlammocount;
extern	vmCvar_t	g_lgammocount;
extern	vmCvar_t	g_rgammocount;
extern	vmCvar_t	g_pgammocount;
extern	vmCvar_t	g_bfgammocount;
extern	vmCvar_t	g_ngammocount;
extern	vmCvar_t	g_plammocount;
extern	vmCvar_t	g_cgammocount;
extern	vmCvar_t	g_ftammocount;
//weaponcount,s
extern	vmCvar_t	g_mgweaponcount;
extern	vmCvar_t	g_sgweaponcount;
extern	vmCvar_t	g_glweaponcount;
extern	vmCvar_t	g_rlweaponcount;
extern	vmCvar_t	g_lgweaponcount;
extern	vmCvar_t	g_rgweaponcount;
extern	vmCvar_t	g_pgweaponcount;
extern	vmCvar_t	g_bfgweaponcount;
extern	vmCvar_t	g_ngweaponcount;
extern	vmCvar_t	g_plweaponcount;
extern	vmCvar_t	g_cgweaponcount;
extern	vmCvar_t	g_ftweaponcount;
extern	vmCvar_t	g_amweaponcount;
//blueteam set
extern	vmCvar_t	g_teamblue_speed;
extern	vmCvar_t	g_teamblue_gravityModifier;
extern	vmCvar_t	g_teamblue_firespeed;
extern	vmCvar_t	g_teamblue_damage;
extern	vmCvar_t	g_teamblue_infammo;
extern	vmCvar_t	g_teamblue_respawnwait;
extern	vmCvar_t	g_teamblue_pickupitems;
//redteam set
extern	vmCvar_t	g_teamred_speed;
extern	vmCvar_t	g_teamred_gravityModifier;
extern	vmCvar_t	g_teamred_firespeed;
extern	vmCvar_t	g_teamred_damage;
extern	vmCvar_t	g_teamred_infammo;
extern	vmCvar_t	g_teamred_respawnwait;
extern	vmCvar_t	g_teamred_pickupitems;
//cvars

extern	vmCvar_t	g_fogModel;
extern	vmCvar_t	g_fogShader;
extern	vmCvar_t	g_fogDistance;
extern	vmCvar_t	g_fogInterval;
extern	vmCvar_t	g_fogColorR;
extern	vmCvar_t	g_fogColorG;
extern	vmCvar_t	g_fogColorB;
extern	vmCvar_t	g_fogColorA;
extern	vmCvar_t	g_skyShader;
extern	vmCvar_t	g_skyColorR;
extern	vmCvar_t	g_skyColorG;
extern	vmCvar_t	g_skyColorB;
extern	vmCvar_t	g_skyColorA;

extern	vmCvar_t	g_allowprops;
extern	vmCvar_t	g_allownpc;
extern	vmCvar_t	g_allowitems;
extern	vmCvar_t	g_allownoclip;
extern	vmCvar_t	g_allowtoolgun;
extern	vmCvar_t	g_allowphysgun;
extern	vmCvar_t	g_allowgravitygun;
extern	vmCvar_t	g_safe;
extern	vmCvar_t	g_npcdrop;
extern	vmCvar_t	g_maxEntities;
extern	vmCvar_t	cl_selectedmod;
extern  vmCvar_t	cl_language;
extern	vmCvar_t	g_tests;
extern	vmCvar_t	g_currentmap;
extern	vmCvar_t	g_connectmsg;
extern	vmCvar_t	g_scoreboardlock;
extern	vmCvar_t	tex_name;
extern	vmCvar_t	tex_newname;
extern	vmCvar_t	g_regenarmor;
extern	vmCvar_t	g_spectatorspeed;
extern	vmCvar_t	eliminationredrespawn;
extern	vmCvar_t	eliminationrespawn;
extern	vmCvar_t	g_lavatowater;
extern	vmCvar_t	g_overlay;
extern	vmCvar_t	g_slickmove;
extern	vmCvar_t	g_accelerate;
extern	vmCvar_t	g_randomItems;
extern 	vmCvar_t	g_mapcycle;
extern 	vmCvar_t	g_useMapcycle;
extern	vmCvar_t	g_mapcycleposition;
extern	vmCvar_t	g_kill;
extern	vmCvar_t	g_kamikazeinfinf;
extern	vmCvar_t	g_invulinf;
extern	vmCvar_t	g_medkitinf;
extern	vmCvar_t	g_teleporterinf;
extern	vmCvar_t	g_portalinf;
extern	vmCvar_t	g_kamikazeinf;
extern	vmCvar_t	g_invulinf;
extern	vmCvar_t	g_medkitlimit;
extern	vmCvar_t	g_waterdamage;
extern	vmCvar_t	g_lavadamage;
extern	vmCvar_t	g_slimedamage;
extern	vmCvar_t	g_maxweaponpickup;
extern	vmCvar_t	g_randomteleport;
extern	vmCvar_t	g_falldamagesmall;
extern	vmCvar_t	g_falldamagebig;
extern	vmCvar_t	g_noplayerclip;
extern	vmCvar_t	g_flagrespawn;
extern	vmCvar_t	g_portaltimeout;
extern	vmCvar_t	g_portalhealth;
extern	vmCvar_t	g_invulmove;
extern	vmCvar_t	g_invultime;
extern	vmCvar_t	g_fasthealthregen;
extern	vmCvar_t	g_slowhealthregen;
extern	vmCvar_t	g_droppeditemtime;
extern	vmCvar_t	g_autoflagreturn;
extern	vmCvar_t	g_hastefirespeed;
extern	vmCvar_t	g_medkitmodifier;
extern	vmCvar_t	g_armorprotect;
extern	vmCvar_t	g_respawnwait;
extern	vmCvar_t	g_jumpheight;
extern	vmCvar_t	g_speedfactor;
extern  vmCvar_t	g_drowndamage;
extern  vmCvar_t	g_ammolimit;
extern  vmCvar_t	g_armorrespawn;
extern	vmCvar_t	g_healthrespawn;
extern	vmCvar_t	g_ammorespawn;
extern	vmCvar_t	g_holdablerespawn;
extern	vmCvar_t	g_megahealthrespawn;
extern	vmCvar_t	g_poweruprespawn;
extern	vmCvar_t	g_gametype;
extern	vmCvar_t	g_dedicated;
extern	vmCvar_t	g_cheats;
extern	vmCvar_t	g_maxclients;			// allow this many total, including spectators
extern	vmCvar_t	g_maxGameClients;		// allow this many active
extern	vmCvar_t	g_restarted;

extern	vmCvar_t	g_dmflags;
extern	vmCvar_t	g_videoflags;
extern	vmCvar_t	g_elimflags;
extern	vmCvar_t	g_voteflags;
extern	vmCvar_t	g_fraglimit;
extern	vmCvar_t	g_timelimit;
extern	vmCvar_t	g_capturelimit;
extern	vmCvar_t	g_friendlyFire;
extern	vmCvar_t	g_password;
extern	vmCvar_t	g_needpass;
extern	vmCvar_t	g_gravity;
extern	vmCvar_t	g_gravityModifier;
extern  vmCvar_t        g_damageModifier;
extern	vmCvar_t	g_speed;
extern	vmCvar_t	g_knockback;
extern	vmCvar_t	g_quadfactor;
extern	vmCvar_t	g_forcerespawn;
extern	vmCvar_t	g_respawntime;
extern	vmCvar_t	g_inactivity;
extern	vmCvar_t	g_disableCutscenes;
extern	vmCvar_t	g_debugMove;
extern	vmCvar_t	g_debugAlloc;
extern	vmCvar_t	g_debugDamage;
extern	vmCvar_t	g_debugCameras;
extern	vmCvar_t	g_debugScore;
extern	vmCvar_t	g_debugVariables;
extern	vmCvar_t	g_debugBotspawns;
extern	vmCvar_t	g_allowSyncCutscene;
extern	vmCvar_t	g_weaponRespawn;
extern	vmCvar_t	g_weaponTeamRespawn;
extern	vmCvar_t	g_synchronousClients;
extern	vmCvar_t	g_motd;
extern	vmCvar_t	g_motdfile;
extern  vmCvar_t        g_votemaps;
extern  vmCvar_t        g_votecustom;
extern	vmCvar_t	g_warmup;
extern	vmCvar_t	g_doWarmup;
extern	vmCvar_t	g_blood;
extern	vmCvar_t	g_allowVote;
extern	vmCvar_t	g_teamAutoJoin;
extern	vmCvar_t	g_teamForceBalance;
extern	vmCvar_t	g_banIPs;
extern	vmCvar_t	g_filterBan;
extern	vmCvar_t	g_obeliskHealth;
extern	vmCvar_t	g_obeliskRegenPeriod;
extern	vmCvar_t	g_obeliskRegenAmount;
extern	vmCvar_t	g_obeliskRespawnDelay;
extern	vmCvar_t	g_cubeTimeout;
extern	vmCvar_t	g_smoothClients;
extern	vmCvar_t	pmove_fixed;
extern	vmCvar_t	pmove_msec;
extern	vmCvar_t	pmove_float;
extern	vmCvar_t	g_rankings;
extern	vmCvar_t	g_enableDust;
extern	vmCvar_t	g_enableBreath;
extern	vmCvar_t	g_proxMineTimeout;
extern	vmCvar_t	g_music;
extern  vmCvar_t        g_spawnprotect;

//elimination:
extern	vmCvar_t	g_elimination_selfdamage;
extern	vmCvar_t	g_elimination_startHealth;
extern	vmCvar_t	g_elimination_startArmor;
extern	vmCvar_t	g_elimination_bfg;
extern	vmCvar_t	g_elimination_grapple;
extern	vmCvar_t	g_elimination_roundtime;
extern	vmCvar_t	g_elimination_warmup;
extern	vmCvar_t	g_elimination_activewarmup;
extern  vmCvar_t    g_elimination_allgametypes;
extern	vmCvar_t	g_elimination_gauntlet;
extern	vmCvar_t	g_elimination_machinegun;
extern	vmCvar_t	g_elimination_shotgun;
extern	vmCvar_t	g_elimination_grenade;
extern	vmCvar_t	g_elimination_rocket;
extern	vmCvar_t	g_elimination_railgun;
extern	vmCvar_t	g_elimination_lightning;
extern	vmCvar_t	g_elimination_plasmagun;
extern	vmCvar_t	g_elimination_chain;
extern	vmCvar_t	g_elimination_mine;
extern	vmCvar_t	g_elimination_nail;
extern	vmCvar_t	g_elimination_flame;
extern	vmCvar_t	g_elimination_antimatter;
extern	vmCvar_t	g_elimination_quad;
extern	vmCvar_t	g_elimination_haste;
extern	vmCvar_t	g_elimination_bsuit;
extern	vmCvar_t	g_elimination_invis;
extern	vmCvar_t	g_elimination_regen;
extern	vmCvar_t	g_elimination_flight;
extern	vmCvar_t	g_elimination_items;
extern	vmCvar_t	g_elimination_holdable;

//elimination:
extern	vmCvar_t	g_eliminationred_startHealth;
extern	vmCvar_t	g_eliminationred_startArmor;
extern	vmCvar_t	g_eliminationred_bfg;
extern	vmCvar_t	g_eliminationred_grapple;
extern	vmCvar_t	g_eliminationred_gauntlet;
extern	vmCvar_t	g_eliminationred_machinegun;
extern	vmCvar_t	g_eliminationred_shotgun;
extern	vmCvar_t	g_eliminationred_grenade;
extern	vmCvar_t	g_eliminationred_rocket;
extern	vmCvar_t	g_eliminationred_railgun;
extern	vmCvar_t	g_eliminationred_lightning;
extern	vmCvar_t	g_eliminationred_plasmagun;
extern	vmCvar_t	g_eliminationred_chain;
extern	vmCvar_t	g_eliminationred_mine;
extern	vmCvar_t	g_eliminationred_nail;
extern	vmCvar_t	g_eliminationred_flame;
extern	vmCvar_t	g_eliminationred_antimatter;
extern	vmCvar_t	g_eliminationred_quad;
extern	vmCvar_t	g_eliminationred_haste;
extern	vmCvar_t	g_eliminationred_bsuit;
extern	vmCvar_t	g_eliminationred_invis;
extern	vmCvar_t	g_eliminationred_regen;
extern	vmCvar_t	g_eliminationred_flight;
extern	vmCvar_t	g_eliminationred_holdable;

//If lockspectator: 0=no limit, 1 = cannot follow enemy, 2 = must follow friend
extern  vmCvar_t        g_elimination_lockspectator;

extern vmCvar_t		g_vampire;
extern vmCvar_t		g_vampireMaxHealth;
//new in elimination Beta3
extern vmCvar_t		g_regen;
//Free for all gametype
extern int		g_ffa_gt; //0 = TEAM GAME, 1 = FFA, 2 = TEAM GAME without bases

extern vmCvar_t		g_lms_lives;

extern vmCvar_t		g_lms_mode; //How do we score: 0 = One Survivor get a point, 1 = same but without overtime, 2 = one point for each player killed (+overtime), 3 = same without overtime

extern vmCvar_t		g_elimination_ctf_oneway;	//Only attack in one direction (level.eliminationSides+level.roundNumber)%2 == 0 red attacks

extern vmCvar_t         g_awardpushing; //The server can decide if players are awarded for pushing people in lave etc.

extern vmCvar_t        g_catchup; //Favors the week players

extern vmCvar_t         g_autonextmap; //Autochange map
extern vmCvar_t         g_mappools; //mappools to be used for autochange

extern vmCvar_t        g_voteNames;
extern vmCvar_t        g_voteBan;
extern vmCvar_t        g_voteGametypes;
extern vmCvar_t        g_voteMinTimelimit;
extern vmCvar_t        g_voteMaxTimelimit;
extern vmCvar_t        g_voteMinFraglimit;
extern vmCvar_t        g_voteMaxFraglimit;
extern vmCvar_t        g_maxvotes;

//unlagged - server options
// some new server-side variables
extern	vmCvar_t	g_delagHitscan;
extern	vmCvar_t	g_truePing;
// this is for convenience - using "sv_fps.integer" is nice :)
extern	vmCvar_t	sv_fps;
extern  vmCvar_t        g_lagLightning;
//unlagged - server options

void	trap_Printf( const char *fmt );
void	trap_Error( const char *fmt ) __attribute__((noreturn));
int		trap_Milliseconds( void );
int     trap_RealTime( qtime_t *qtime );
int		trap_Argc( void );
void	trap_Argv( int n, char *buffer, int bufferLength );
void	trap_Args( char *buffer, int bufferLength );
int		trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void	trap_FS_Read( void *buffer, int len, fileHandle_t f );
void	trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void	trap_FS_FCloseFile( fileHandle_t f );
int		trap_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize );
int		trap_FS_Seek( fileHandle_t f, long offset, int origin ); // fsOrigin_t
void 	trap_System( const char *command );
void	trap_SendConsoleCommand( int exec_when, const char *text );
void	trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags );
void	trap_Cvar_Update( vmCvar_t *cvar );
void	trap_Cvar_Set( const char *var_name, const char *value );
int		trap_Cvar_VariableIntegerValue( const char *var_name );
float	trap_Cvar_VariableValue( const char *var_name );
void	trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );
void	trap_LocateGameData( gentity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *gameClients, int sizeofGameClient );
void	trap_DropClient( int clientNum, const char *reason );
void	trap_SendServerCommand( int clientNum, const char *text );
void	trap_SetConfigstring( int num, const char *string );
void	trap_GetConfigstring( int num, char *buffer, int bufferSize );
void	trap_GetUserinfo( int num, char *buffer, int bufferSize );
void	trap_SetUserinfo( int num, const char *buffer );
void	trap_GetServerinfo( char *buffer, int bufferSize );
void	trap_SetBrushModel( gentity_t *ent, const char *name );
void	trap_Trace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
int		trap_PointContents( const vec3_t point, int passEntityNum );
qboolean trap_InPVS( const vec3_t p1, const vec3_t p2 );
qboolean trap_InPVSIgnorePortals( const vec3_t p1, const vec3_t p2 );
void	trap_AdjustAreaPortalState( gentity_t *ent, qboolean open );
qboolean trap_AreasConnected( int area1, int area2 );
void	trap_LinkEntity( gentity_t *ent );
void	trap_UnlinkEntity( gentity_t *ent );
int		trap_EntitiesInBox( const vec3_t mins, const vec3_t maxs, int *entityList, int maxcount );
qboolean trap_EntityContact( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
int		trap_BotAllocateClient( void );
void	trap_BotFreeClient( int clientNum );
void	trap_GetUsercmd( int clientNum, usercmd_t *cmd );
qboolean	trap_GetEntityToken( char *buffer, int bufferSize );

int		trap_DebugPolygonCreate(int color, int numPoints, vec3_t *points);
void	trap_DebugPolygonDelete(int id);

int		trap_BotLibSetup( void );
int		trap_BotLibShutdown( void );
int		trap_BotLibVarSet(char *var_name, char *value);
int		trap_BotLibVarGet(char *var_name, char *value, int size);
int		trap_BotLibDefine(char *string);
int		trap_BotLibStartFrame(float time);
int		trap_BotLibLoadMap(const char *mapname);
int		trap_BotLibUpdateEntity(int ent, void /* struct bot_updateentity_s */ *bue);
int		trap_BotLibTest(int parm0, char *parm1, vec3_t parm2, vec3_t parm3);

int		trap_BotGetSnapshotEntity( int clientNum, int sequence );
int		trap_BotGetServerCommand(int clientNum, char *message, int size);
void	trap_BotUserCommand(int client, usercmd_t *ucmd);

int		trap_AAS_BBoxAreas(vec3_t absmins, vec3_t absmaxs, int *areas, int maxareas);
int		trap_AAS_AreaInfo( int areanum, void /* struct aas_areainfo_s */ *info );
void	trap_AAS_EntityInfo(int entnum, void /* struct aas_entityinfo_s */ *info);

int		trap_AAS_Initialized(void);
void	trap_AAS_PresenceTypeBoundingBox(int presencetype, vec3_t mins, vec3_t maxs);
float	trap_AAS_Time(void);

int		trap_AAS_PointAreaNum(vec3_t point);
int		trap_AAS_PointReachabilityAreaIndex(vec3_t point);
int		trap_AAS_TraceAreas(vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas);

int		trap_AAS_PointContents(vec3_t point);
int		trap_AAS_NextBSPEntity(int ent);
int		trap_AAS_ValueForBSPEpairKey(int ent, char *key, char *value, int size);
int		trap_AAS_VectorForBSPEpairKey(int ent, char *key, vec3_t v);
int		trap_AAS_FloatForBSPEpairKey(int ent, char *key, float *value);
int		trap_AAS_IntForBSPEpairKey(int ent, char *key, int *value);

int		trap_AAS_AreaReachability(int areanum);

int		trap_AAS_AreaTravelTimeToGoalArea(int areanum, vec3_t origin, int goalareanum, int travelflags);
int		trap_AAS_EnableRoutingArea( int areanum, int enable );
int		trap_AAS_PredictRoute(void /*struct aas_predictroute_s*/ *route, int areanum, vec3_t origin,
							int goalareanum, int travelflags, int maxareas, int maxtime,
							int stopevent, int stopcontents, int stoptfl, int stopareanum);

int		trap_AAS_AlternativeRouteGoals(vec3_t start, int startareanum, vec3_t goal, int goalareanum, int travelflags,
										void /*struct aas_altroutegoal_s*/ *altroutegoals, int maxaltroutegoals,
										int type);
int		trap_AAS_Swimming(vec3_t origin);
int		trap_AAS_PredictClientMovement(void /* aas_clientmove_s */ *move, int entnum, vec3_t origin, int presencetype, int onground, vec3_t velocity, vec3_t cmdmove, int cmdframes, int maxframes, float frametime, int stopevent, int stopareanum, int visualize);


void	trap_EA_Say(int client, char *str);
void	trap_EA_SayTeam(int client, char *str);
void	trap_EA_Command(int client, char *command);

void	trap_EA_Action(int client, int action);
void	trap_EA_Gesture(int client);
void	trap_EA_Talk(int client);
void	trap_EA_Attack(int client);
void	trap_EA_Use(int client);
void	trap_EA_Respawn(int client);
void	trap_EA_Crouch(int client);
void	trap_EA_MoveUp(int client);
void	trap_EA_MoveDown(int client);
void	trap_EA_MoveForward(int client);
void	trap_EA_MoveBack(int client);
void	trap_EA_MoveLeft(int client);
void	trap_EA_MoveRight(int client);
void	trap_EA_SelectWeapon(int client, int weapon);
void	trap_EA_Jump(int client);
void	trap_EA_DelayedJump(int client);
void	trap_EA_Move(int client, vec3_t dir, float speed);
void	trap_EA_View(int client, vec3_t viewangles);

void	trap_EA_EndRegular(int client, float thinktime);
void	trap_EA_GetInput(int client, float thinktime, void /* struct bot_input_s */ *input);
void	trap_EA_ResetInput(int client);


int		trap_BotLoadCharacter(char *charfile, float skill);
void	trap_BotFreeCharacter(int character);
float	trap_Characteristic_Float(int character, int index);
float	trap_Characteristic_BFloat(int character, int index, float min, float max);
int		trap_Characteristic_Integer(int character, int index);
int		trap_Characteristic_BInteger(int character, int index, int min, int max);
void	trap_Characteristic_String(int character, int index, char *buf, int size);

int		trap_BotAllocChatState(void);
void	trap_BotFreeChatState(int handle);
void	trap_BotQueueConsoleMessage(int chatstate, int type, char *message);
void	trap_BotRemoveConsoleMessage(int chatstate, int handle);
int		trap_BotNextConsoleMessage(int chatstate, void /* struct bot_consolemessage_s */ *cm);
int		trap_BotNumConsoleMessages(int chatstate);
void	trap_BotInitialChat(int chatstate, char *type, int mcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 );
int		trap_BotNumInitialChats(int chatstate, char *type);
int		trap_BotReplyChat(int chatstate, char *message, int mcontext, int vcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 );
int		trap_BotChatLength(int chatstate);
void	trap_BotEnterChat(int chatstate, int client, int sendto);
void	trap_BotGetChatMessage(int chatstate, char *buf, int size);
int		trap_StringContains(char *str1, char *str2, int casesensitive);
int		trap_BotFindMatch(char *str, void /* struct bot_match_s */ *match, unsigned long int context);
void	trap_BotMatchVariable(void /* struct bot_match_s */ *match, int variable, char *buf, int size);
void	trap_UnifyWhiteSpaces(char *string);
void	trap_BotReplaceSynonyms(char *string, unsigned long int context);
int		trap_BotLoadChatFile(int chatstate, char *chatfile, char *chatname);
void	trap_BotSetChatGender(int chatstate, int gender);
void	trap_BotSetChatName(int chatstate, char *name, int client);
void	trap_BotResetGoalState(int goalstate);
void	trap_BotRemoveFromAvoidGoals(int goalstate, int number);
void	trap_BotResetAvoidGoals(int goalstate);
void	trap_BotPushGoal(int goalstate, void /* struct bot_goal_s */ *goal);
void	trap_BotPopGoal(int goalstate);
void	trap_BotEmptyGoalStack(int goalstate);
void	trap_BotDumpAvoidGoals(int goalstate);
void	trap_BotDumpGoalStack(int goalstate);
void	trap_BotGoalName(int number, char *name, int size);
int		trap_BotGetTopGoal(int goalstate, void /* struct bot_goal_s */ *goal);
int		trap_BotGetSecondGoal(int goalstate, void /* struct bot_goal_s */ *goal);
int		trap_BotChooseLTGItem(int goalstate, vec3_t origin, int *inventory, int travelflags);
int		trap_BotChooseNBGItem(int goalstate, vec3_t origin, int *inventory, int travelflags, void /* struct bot_goal_s */ *ltg, float maxtime);
int		trap_BotTouchingGoal(vec3_t origin, void /* struct bot_goal_s */ *goal);
int		trap_BotItemGoalInVisButNotVisible(int viewer, vec3_t eye, vec3_t viewangles, void /* struct bot_goal_s */ *goal);
int		trap_BotGetNextCampSpotGoal(int num, void /* struct bot_goal_s */ *goal);
int		trap_BotGetMapLocationGoal(char *name, void /* struct bot_goal_s */ *goal);
int		trap_BotGetLevelItemGoal(int index, char *classname, void /* struct bot_goal_s */ *goal);
float	trap_BotAvoidGoalTime(int goalstate, int number);
void	trap_BotSetAvoidGoalTime(int goalstate, int number, float avoidtime);
void	trap_BotInitLevelItems(void);
void	trap_BotUpdateEntityItems(void);
int		trap_BotLoadItemWeights(int goalstate, char *filename);
void	trap_BotFreeItemWeights(int goalstate);
void	trap_BotInterbreedGoalFuzzyLogic(int parent1, int parent2, int child);
void	trap_BotSaveGoalFuzzyLogic(int goalstate, char *filename);
void	trap_BotMutateGoalFuzzyLogic(int goalstate, float range);
int		trap_BotAllocGoalState(int state);
void	trap_BotFreeGoalState(int handle);

void	trap_BotResetMoveState(int movestate);
void	trap_BotMoveToGoal(void /* struct bot_moveresult_s */ *result, int movestate, void /* struct bot_goal_s */ *goal, int travelflags);
int		trap_BotMoveInDirection(int movestate, vec3_t dir, float speed, int type);
void	trap_BotResetAvoidReach(int movestate);
void	trap_BotResetLastAvoidReach(int movestate);
int		trap_BotReachabilityArea(vec3_t origin, int testground);
int		trap_BotMovementViewTarget(int movestate, void /* struct bot_goal_s */ *goal, int travelflags, float lookahead, vec3_t target);
int		trap_BotPredictVisiblePosition(vec3_t origin, int areanum, void /* struct bot_goal_s */ *goal, int travelflags, vec3_t target);
int		trap_BotAllocMoveState(void);
void	trap_BotFreeMoveState(int handle);
void	trap_BotInitMoveState(int handle, void /* struct bot_initmove_s */ *initmove);
void	trap_BotAddAvoidSpot(int movestate, vec3_t origin, float radius, int type);

int		trap_BotChooseBestFightWeapon(int weaponstate, int *inventory);
void	trap_BotGetWeaponInfo(int weaponstate, int weapon, void /* struct weaponinfo_s */ *weaponinfo);
int		trap_BotLoadWeaponWeights(int weaponstate, char *filename);
int		trap_BotAllocWeaponState(void);
void	trap_BotFreeWeaponState(int weaponstate);
void	trap_BotResetWeaponState(int weaponstate);

int		trap_GeneticParentsAndChildSelection(int numranks, float *ranks, int *parent1, int *parent2, int *child);

void	trap_SnapVector( float *v );

#define CMD_CHEAT           0x0001
#define CMD_CHEAT_TEAM      0x0002 // is a cheat when used on a team
#define CMD_MESSAGE         0x0004 // sends message to others (skip when muted)
#define CMD_TEAM            0x0008 // must be on a team
#define CMD_NOTEAM          0x0010 // must not be on a team
#define CMD_LIVING          0x0020
#define CMD_INTERMISSION    0x0040 // valid during intermission


typedef struct
{
    char    *cmdName;
    int     cmdFlags;
    void    ( *cmdHandler )( gentity_t *ent );
} commands_t;

//
// g_svcmds_ext.c
// These were added to a seperate file to keep g_svcmds.c navigable.
void Svcmd_Status_f( void );
void Svcmd_TeamMessage_f( void );
void Svcmd_CenterPrint_f( void );
void Svcmd_ReplaceTexture_f( void );
void Svcmd_BannerPrint_f( void );
void Svcmd_EjectClient_f( void );
void Svcmd_DumpUser_f( void );
void Svcmd_Chat_f( void );
void Svcmd_ListIP_f( void );
void Svcmd_MessageWrapper( void );
void Svcmd_PropNpc_AS_f( void );

//Noire.Script
void Svcmd_NS_OpenScript_f( void );
void Svcmd_NS_Interpret_f( void );
void Svcmd_NS_VariableList_f( void );
void Svcmd_NS_ThreadList_f( void );
void Svcmd_NS_SendVariable_f( void );
