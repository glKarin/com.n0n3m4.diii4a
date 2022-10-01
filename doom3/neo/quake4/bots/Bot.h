// Bot.h
//

class rvmBotAIBotActionBase;

// These need to match items.def
#define INVENTORY_ARMOR				1
#define INVENTORY_GAUNTLET			4
#define INVENTORY_SHOTGUN			5
#define INVENTORY_MACHINEGUN		6
#define INVENTORY_GRENADELAUNCHER	7
#define INVENTORY_ROCKETLAUNCHER	8
#define INVENTORY_LIGHTNING			9
#define INVENTORY_RAILGUN			10
#define INVENTORY_PLASMAGUN			11
#define INVENTORY_BFG10K			13
#define INVENTORY_GRAPPLINGHOOK		14
#define INVENTORY_NAILGUN			15
#define INVENTORY_PROXLAUNCHER		16
#define INVENTORY_CHAINGUN			17
#define INVENTORY_SHELLS			18
#define INVENTORY_BULLETS			19
#define INVENTORY_GRENADES			20
#define INVENTORY_CELLS				21
#define INVENTORY_LIGHTNINGAMMO		22
#define INVENTORY_ROCKETS			23
#define INVENTORY_SLUGS				24
#define INVENTORY_BFGAMMO			25
#define INVENTORY_NAILS				26
#define INVENTORY_MINES				27
#define INVENTORY_BELT			28
#define INVENTORY_HEALTH			29
#define INVENTORY_TELEPORTER		30
#define INVENTORY_MEDKIT			31
#define INVENTORY_KAMIKAZE			32
#define INVENTORY_PORTAL			33
#define INVENTORY_INVULNERABILITY	34
#define INVENTORY_QUAD				35
#define INVENTORY_ENVIRONMENTSUIT	36
#define INVENTORY_HASTE				37
#define INVENTORY_INVISIBILITY		38
#define INVENTORY_REGEN				39
#define INVENTORY_FLIGHT			40
#define INVENTORY_SCOUT				41
#define INVENTORY_GUARD				42
#define INVENTORY_DOUBLER			43
#define INVENTORY_AMMOREGEN			44
#define INVENTORY_REDFLAG			45
#define INVENTORY_BLUEFLAG			46
#define INVENTORY_NEUTRALFLAG		47
#define INVENTORY_REDCUBE			48
#define INVENTORY_BLUECUBE			49
#define INVENTORY_DOUBLESHOTGUN		50

#define MODELINDEX_DEFAULT			0
#define MODELINDEX_ARMORSHARD		1
#define MODELINDEX_ARMORCOMBAT		2
#define MODELINDEX_ARMORBODY		3
#define MODELINDEX_HEALTHSMALL		4
#define MODELINDEX_HEALTH			5
#define MODELINDEX_HEALTHLARGE		6
#define MODELINDEX_HEALTHMEGA		7
#define MODELINDEX_GAUNTLET			8
#define MODELINDEX_SHOTGUN			9
#define MODELINDEX_MACHINEGUN		10
#define MODELINDEX_GRENADELAUNCHER	11
#define MODELINDEX_ROCKETLAUNCHER	12
#define MODELINDEX_LIGHTNING		13
#define MODELINDEX_RAILGUN			14
#define MODELINDEX_PLASMAGUN		15
#define MODELINDEX_BFG10K			16
#define MODELINDEX_GRAPPLINGHOOK	17
#define MODELINDEX_SHELLS			18
#define MODELINDEX_BULLETS			19
#define MODELINDEX_GRENADES			20
#define MODELINDEX_CELLS			21
#define MODELINDEX_LIGHTNINGAMMO	22
#define MODELINDEX_ROCKETS			23
#define MODELINDEX_SLUGS			24
#define MODELINDEX_BFGAMMO			25
#define MODELINDEX_TELEPORTER		26
#define MODELINDEX_MEDKIT			27
#define MODELINDEX_QUAD				28
#define MODELINDEX_ENVIRONMENTSUIT	29
#define MODELINDEX_HASTE			30
#define MODELINDEX_INVISIBILITY		31
#define MODELINDEX_REGEN			32
#define MODELINDEX_FLIGHT			33
#define MODELINDEX_REDFLAG			34
#define MODELINDEX_BLUEFLAG			35
#define MODELINDEX_KAMIKAZE			36
#define MODELINDEX_PORTAL			37
#define MODELINDEX_INVULNERABILITY	38
#define MODELINDEX_NAILS			39
#define MODELINDEX_MINES			40
#define MODELINDEX_BELT				41
#define MODELINDEX_SCOUT			42
#define MODELINDEX_GUARD			43
#define MODELINDEX_DOUBLER			44
#define MODELINDEX_AMMOREGEN		45
#define MODELINDEX_NEUTRALFLAG		46
#define MODELINDEX_REDCUBE			47
#define MODELINDEX_BLUECUBE			48
#define MODELINDEX_NAILGUN			49
#define MODELINDEX_PROXLAUNCHER		50
#define MODELINDEX_CHAINGUN			51
#define MODELINDEX_POINTABLUE		52
#define MODELINDEX_POINTBBLUE		53
#define MODELINDEX_POINTARED		54
#define MODELINDEX_POINTBRED		55
#define MODELINDEX_POINTAWHITE		56
#define MODELINDEX_POINTBWHITE		57
#define MODELINDEX_POINTWHITE		58
#define MODELINDEX_POINTRED			59
#define MODELINDEX_POINTBLUE		60
#define WEAPONINDEX_GAUNTLET			1
#define WEAPONINDEX_MACHINEGUN			2
#define WEAPONINDEX_SHOTGUN				3
#define WEAPONINDEX_GRENADE_LAUNCHER	4
#define WEAPONINDEX_ROCKET_LAUNCHER		5
#define WEAPONINDEX_LIGHTNING			6
#define WEAPONINDEX_RAILGUN				7
#define WEAPONINDEX_PLASMAGUN			8
#define WEAPONINDEX_BFG					9
#define WEAPONINDEX_GRAPPLING_HOOK		10
#define WEAPONINDEX_NAILGUN				11
#define WEAPONINDEX_PROXLAUNCHER		12
#define WEAPONINDEX_CHAINGUN			13

//enemy stuff
#define ENEMY_HORIZONTAL_DIST		200
#define ENEMY_HEIGHT				201

#define MAX_AVOIDGOALS			256
#define MAX_GOALSTACK			8

#define GFL_NONE				0
#define GFL_ITEM				1
#define GFL_ROAM				2
#define GFL_DROPPED				4

#define BLERR_NOERROR					0	//no error
#define BLERR_LIBRARYNOTSETUP			1	//library not setup
#define BLERR_INVALIDENTITYNUMBER		2	//invalid entity number
#define BLERR_NOAASFILE					3	//no AAS file available
#define BLERR_CANNOTOPENAASFILE			4	//cannot open AAS file
#define BLERR_WRONGAASFILEID			5	//incorrect AAS file id
#define BLERR_WRONGAASFILEVERSION		6	//incorrect AAS file version
#define BLERR_CANNOTREADAASLUMP			7	//cannot read AAS file lump
#define BLERR_CANNOTLOADICHAT			8	//cannot load initial chats
#define BLERR_CANNOTLOADITEMWEIGHTS		9	//cannot load item weights
#define BLERR_CANNOTLOADITEMCONFIG		10	//cannot load item config
#define BLERR_CANNOTLOADWEAPONWEIGHTS	11	//cannot load weapon weights
#define BLERR_CANNOTLOADWEAPONCONFIG	12	//cannot load weapon config

#define WT_BALANCE			1
#define MAX_WEIGHTS			128

//fuzzy seperator
struct fuzzyseperator_t
{
	fuzzyseperator_t()
	{
		Reset();
	}

	void Reset()
	{
		inUse = false;
		index = 0;
		value = 0;
		type = 0;
		weight = 0;
		minweight = 0.0f;
		maxweight = 0.0f;
		child = 0;
		next = 0;
	}

	bool inUse;
	int index;
	int value;
	int type;
	float weight;
	float minweight;
	float maxweight;
	fuzzyseperator_t* child;
	fuzzyseperator_t* next;
};

//fuzzy weight
struct weight_t
{
	weight_t()
	{
		Reset();
	}

	void Reset()
	{
		name.Clear();
		firstseperator = 0;
	}

	idStr name;
	fuzzyseperator_t* firstseperator;
};

//weight configuration
struct weightconfig_t
{
	weightconfig_t()
	{
		Reset();
	}

	void Reset()
	{
		inUse = false;
		numweights = 0;
		filename.Clear();

		for( int i = 0; i < MAX_WEIGHTS; i++ )
		{
			weights[i].Reset();
		}
	}

	bool inUse;
	int numweights;
	weight_t weights[MAX_WEIGHTS];
	idStr filename;
};

// ------------------------------------------------------------------------------------

#define BOTFILESBASEFOLDER		"botfiles"
//debug line colors
#define LINECOLOR_NONE			-1
#define LINECOLOR_RED			1//0xf2f2f0f0L
#define LINECOLOR_GREEN			2//0xd0d1d2d3L
#define LINECOLOR_BLUE			3//0xf3f3f1f1L
#define LINECOLOR_YELLOW		4//0xdcdddedfL
#define LINECOLOR_ORANGE		5//0xe0e1e2e3L

//Print types
#define PRT_MESSAGE				1
#define PRT_WARNING				2
#define PRT_ERROR				3
#define PRT_FATAL				4
#define PRT_EXIT				5

//console message types
#define CMS_NORMAL				0
#define CMS_CHAT				1

//action flags
#define ACTION_ATTACK			0x0000001
#define ACTION_USE				0x0000002
#define ACTION_RESPAWN			0x0000008
#define ACTION_JUMP				0x0000010
#define ACTION_MOVEUP			0x0000020
#define ACTION_CROUCH			0x0000080
#define ACTION_MOVEDOWN			0x0000100
#define ACTION_MOVEFORWARD		0x0000200
#define ACTION_MOVEBACK			0x0000800
#define ACTION_MOVELEFT			0x0001000
#define ACTION_MOVERIGHT		0x0002000
#define ACTION_DELAYEDJUMP		0x0008000
#define ACTION_TALK				0x0010000
#define ACTION_GESTURE			0x0020000
#define ACTION_WALK				0x0080000
#define ACTION_AFFIRMATIVE		0x0100000
#define ACTION_NEGATIVE			0x0200000
#define ACTION_GETFLAG			0x0800000
#define ACTION_GUARDBASE		0x1000000
#define ACTION_PATROL			0x2000000
#define ACTION_FOLLOWME			0x8000000


//#define DEBUG
#define CTF

#define MAX_ITEMS					256
//bot flags
#define BFL_STRAFERIGHT				1	//strafe to the right
#define BFL_ATTACKED				2	//bot has attacked last ai frame
#define BFL_ATTACKJUMPED			4	//bot jumped during attack last frame
#define BFL_AIMATENEMY				8	//bot aimed at the enemy this frame
#define BFL_AVOIDRIGHT				16	//avoid obstacles by going to the right
#define BFL_IDEALVIEWSET			32	//bot has ideal view angles set
#define BFL_FIGHTSUICIDAL			64	//bot is in a suicidal fight
//long term goal types
#define LTG_TEAMHELP				1	//help a team mate
#define LTG_TEAMACCOMPANY			2	//accompany a team mate
#define LTG_DEFENDKEYAREA			3	//defend a key area
#define LTG_GETFLAG					4	//get the enemy flag
#define LTG_RUSHBASE				5	//rush to the base
#define LTG_RETURNFLAG				6	//return the flag
#define LTG_CAMP					7	//camp somewhere
#define LTG_CAMPORDER				8	//ordered to camp somewhere
#define LTG_PATROL					9	//patrol
#define LTG_GETITEM					10	//get an item
#define LTG_KILL					11	//kill someone
#define LTG_HARVEST					12	//harvest skulls
#define LTG_ATTACKENEMYBASE			13	//attack the enemy base
#define LTG_MAKELOVE_UNDER			14
#define LTG_MAKELOVE_ONTOP			15
//some goal dedication times
#define TEAM_HELP_TIME				60	//1 minute teamplay help time
#define TEAM_ACCOMPANY_TIME			600	//10 minutes teamplay accompany time
#define TEAM_DEFENDKEYAREA_TIME		600	//10 minutes ctf defend base time
#define TEAM_CAMP_TIME				600	//10 minutes camping time
#define TEAM_PATROL_TIME			600	//10 minutes patrolling time
#define TEAM_LEAD_TIME				600	//10 minutes taking the lead
#define TEAM_GETITEM_TIME			60	//1 minute
#define	TEAM_KILL_SOMEONE			180	//3 minute to kill someone
#define TEAM_ATTACKENEMYBASE_TIME	600	//10 minutes
#define TEAM_HARVEST_TIME			120	//2 minutes
#define CTF_GETFLAG_TIME			600	//10 minutes ctf get flag time
#define CTF_RUSHBASE_TIME			120	//2 minutes ctf rush base time
#define CTF_RETURNFLAG_TIME			180	//3 minutes to return the flag
#define CTF_ROAM_TIME				60	//1 minute ctf roam time
//patrol flags
#define PATROL_LOOP					1
#define PATROL_REVERSE				2
#define PATROL_BACK					4
//teamplay task preference
#define TEAMTP_DEFENDER				1
#define TEAMTP_ATTACKER				2
//CTF strategy
#define CTFS_AGRESSIVE				1
//copied from the aas file header
#define PRESENCE_NONE				1
#define PRESENCE_NORMAL				2
#define PRESENCE_CROUCH				4
//
#define MAX_PROXMINES				64


#define MAX_CHARACTERISTICS		80

#define CT_INTEGER				1
#define CT_FLOAT				2
#define CT_STRING				3

#define DEFAULT_CHARACTER		"bots/default_c.c"

#define MAX_AVOIDGOALS			256
#define MAX_GOALSTACK			8

#define GFL_NONE				0
#define GFL_ITEM				1
#define GFL_ROAM				2
#define GFL_DROPPED				4
#define MAX_EPAIRKEY		128

//characteristic value
struct cvalue
{
	cvalue()
	{
		integer = 0;
		_float = 0.0f;
		string = "";
	}

	int integer;
	float _float;
	idStr string;
};

//a characteristic
struct bot_characteristic_t
{
	bot_characteristic_t()
	{
		type = 0;
	}

	char type;						//characteristic type
	cvalue value;				//characteristic value
};

//a bot character
struct bot_character_t
{
	bot_character_t()
	{
		filename = "";
		inUse = false;
		skill = 0.0f;
	}

	idStr filename;
	bool inUse;
	float skill;
	bot_characteristic_t c[MAX_CHARACTERISTICS];
};

//the bot input, will be converted to an usercmd_t
//the bot input, will be converted to an usercmd_t
struct bot_input_t
{
	bot_input_t()
	{
		Reset();
	}

	void Reset()
	{
		thinktime = 0;
		dir.Zero();
		speed = 0;
		viewangles.Zero();
		actionflags = 0;
		weapon = 0;
		lastWeaponNum = 0;
		respawn = false;
	}

	float thinktime;		//time since last output (in seconds)
	idVec3 dir;				//movement direction
	float speed;			//speed in the range [0, 400]
	idAngles viewangles;		//the view angles
	int actionflags;		//one of the ACTION_? flags
	int weapon;				//weapon to use
	int lastWeaponNum;
	bool respawn;
};

#if 0
//entity state
typedef struct bot_entitystate_s
{
	int		type;			// entity type
	int		flags;			// entity flags
	vec3_t	origin;			// origin of the entity
	vec3_t	angles;			// angles of the model
	vec3_t	old_origin;		// for lerping
	vec3_t	mins;			// bounding box minimums
	vec3_t	maxs;			// bounding box maximums
	int		groundent;		// ground entity
	int		solid;			// solid type
	int		modelindex;		// model used
	int		modelindex2;	// weapons, CTF flags, etc
	int		frame;			// model frame number
	int		event;			// impulse events -- muzzle flashes, footsteps, etc
	int		eventParm;		// even parameter
	int		powerups;		// bit flags
	int		weapon;			// determines weapon and flash model, etc
	int		legsAnim;		// mask off ANIM_TOGGLEBIT
	int		torsoAnim;		// mask off ANIM_TOGGLEBIT
} bot_entitystate_t;

//check points
typedef struct bot_waypoint_s
{
	int			inuse;
	char		name[32];
	bot_goal_t	goal;
	struct		bot_waypoint_s* next, * prev;
} bot_waypoint_t;

//bot settings
typedef struct bot_settings_s
{
	char characterfile[MAX_QPATH];
	float skill;
	char team[MAX_QPATH];
} bot_settings_t;

#define MAX_ACTIVATESTACK		8
#define MAX_ACTIVATEAREAS		32

typedef struct bot_activategoal_s
{
	int inuse;
	bot_goal_t goal;						//goal to activate (buttons etc.)
	float time;								//time to activate something
	float start_time;						//time starting to activate something
	float justused_time;					//time the goal was used
	int shoot;								//true if bot has to shoot to activate
	int weapon;								//weapon to be used for activation
	vec3_t target;							//target to shoot at to activate something
	vec3_t origin;							//origin of the blocking entity to activate
	int areas[MAX_ACTIVATEAREAS];			//routing areas disabled by blocking entity
	int numareas;							//number of disabled routing areas
	int areasdisabled;						//true if the areas are disabled for the routing
	struct bot_activategoal_s* next;		//next activate goal on stack
} bot_activategoal_t;

//bot state
typedef struct bot_state_s
{
	int inuse;										//true if this state is used by a bot client
	int botthink_residual;							//residual for the bot thinks
	int client;										//client number of the bot
	int entitynum;									//entity number of the bot
	playerState_t cur_ps;							//current player state
	int last_eFlags;								//last ps flags
	usercmd_t lastucmd;								//usercmd from last frame
	int entityeventTime[1024];						//last entity event time
	//
	bot_settings_t settings;						//several bot settings
	int ( *ainode )( struct bot_state_s* bs );			//current AI node
	float thinktime;								//time the bot thinks this frame
	vec3_t origin;									//origin of the bot
	vec3_t velocity;								//velocity of the bot
	int presencetype;								//presence type of the bot
	vec3_t eye;										//eye coordinates of the bot
	int areanum;									//the number of the area the bot is in
	int inventory[MAX_ITEMS];						//string with items amounts the bot has
	int tfl;										//the travel flags the bot uses
	int flags;										//several flags
	int respawn_wait;								//wait until respawned
	int lasthealth;									//health value previous frame
	int lastkilledplayer;							//last killed player
	int lastkilledby;								//player that last killed this bot
	int botdeathtype;								//the death type of the bot
	int enemydeathtype;								//the death type of the enemy
	int botsuicide;									//true when the bot suicides
	int enemysuicide;								//true when the enemy of the bot suicides
	int setupcount;									//true when the bot has just been setup
	int map_restart;									//true when the map is being restarted
	int entergamechat;								//true when the bot used an enter game chat
	int num_deaths;									//number of time this bot died
	int num_kills;									//number of kills of this bot
	int revenge_enemy;								//the revenge enemy
	int revenge_kills;								//number of kills the enemy made
	int lastframe_health;							//health value the last frame
	int lasthitcount;								//number of hits last frame
	int chatto;										//chat to all or team
	float walker;									//walker charactertic
	float ltime;									//local bot time
	float entergame_time;							//time the bot entered the game
	float ltg_time;									//long term goal time
	float nbg_time;									//nearby goal time
	float respawn_time;								//time the bot takes to respawn
	float respawnchat_time;							//time the bot started a chat during respawn
	float chase_time;								//time the bot will chase the enemy
	float enemyvisible_time;						//time the enemy was last visible
	float check_time;								//time to check for nearby items
	float stand_time;								//time the bot is standing still
	float lastchat_time;							//time the bot last selected a chat
	float kamikaze_time;							//time to check for kamikaze usage
	float invulnerability_time;						//time to check for invulnerability usage
	float standfindenemy_time;						//time to find enemy while standing
	float attackstrafe_time;						//time the bot is strafing in one dir
	float attackcrouch_time;						//time the bot will stop crouching
	float attackchase_time;							//time the bot chases during actual attack
	float attackjump_time;							//time the bot jumped during attack
	float enemysight_time;							//time before reacting to enemy
	float enemydeath_time;							//time the enemy died
	float enemyposition_time;						//time the position and velocity of the enemy were stored
	float defendaway_time;							//time away while defending
	float defendaway_range;							//max travel time away from defend area
	float rushbaseaway_time;						//time away from rushing to the base
	float attackaway_time;							//time away from attacking the enemy base
	float harvestaway_time;							//time away from harvesting
	float ctfroam_time;								//time the bot is roaming in ctf
	float killedenemy_time;							//time the bot killed the enemy
	float arrive_time;								//time arrived (at companion)
	float lastair_time;								//last time the bot had air
	float teleport_time;							//last time the bot teleported
	float camp_time;								//last time camped
	float camp_range;								//camp range
	float weaponchange_time;						//time the bot started changing weapons
	float firethrottlewait_time;					//amount of time to wait
	float firethrottleshoot_time;					//amount of time to shoot
	float notblocked_time;							//last time the bot was not blocked
	float blockedbyavoidspot_time;					//time blocked by an avoid spot
	float predictobstacles_time;					//last time the bot predicted obstacles
	int predictobstacles_goalareanum;				//last goal areanum the bot predicted obstacles for
	vec3_t aimtarget;
	vec3_t enemyvelocity;							//enemy velocity 0.5 secs ago during battle
	vec3_t enemyorigin;								//enemy origin 0.5 secs ago during battle
	//
	int kamikazebody;								//kamikaze body
	int proxmines[MAX_PROXMINES];
	int numproxmines;
	//
	bot_character_t* character;						//the bot character
	int ms;											//move state of the bot
	int gs;											//goal state of the bot
	int cs;											//chat state of the bot
	int ws;											//weapon state of the bot
	//
	int enemy;										//enemy entity number
	int lastenemyareanum;							//last reachability area the enemy was in
	vec3_t lastenemyorigin;							//last origin of the enemy in the reachability area
	int weaponnum;									//current weapon number
	vec3_t viewangles;								//current view angles
	vec3_t ideal_viewangles;						//ideal view angles
	vec3_t viewanglespeed;
	//
	int ltgtype;									//long term goal type
	// team goals
	int teammate;									//team mate involved in this team goal
	int decisionmaker;								//player who decided to go for this goal
	int ordered;									//true if ordered to do something
	float order_time;								//time ordered to do something
	int owndecision_time;							//time the bot made it's own decision
	bot_goal_t teamgoal;							//the team goal
	bot_goal_t altroutegoal;						//alternative route goal
	float reachedaltroutegoal_time;					//time the bot reached the alt route goal
	float teammessage_time;							//time to message team mates what the bot is doing
	float teamgoal_time;							//time to stop helping team mate
	float teammatevisible_time;						//last time the team mate was NOT visible
	int teamtaskpreference;							//team task preference
	// last ordered team goal
	int lastgoal_decisionmaker;
	int lastgoal_ltgtype;
	int lastgoal_teammate;
	bot_goal_t lastgoal_teamgoal;
	// for leading team mates
	int lead_teammate;								//team mate the bot is leading
	bot_goal_t lead_teamgoal;						//team goal while leading
	float lead_time;								//time leading someone
	float leadvisible_time;							//last time the team mate was visible
	float leadmessage_time;							//last time a messaged was sent to the team mate
	float leadbackup_time;							//time backing up towards team mate
	//
	char teamleader[32];							//netname of the team leader
	float askteamleader_time;						//time asked for team leader
	float becometeamleader_time;					//time the bot will become the team leader
	float teamgiveorders_time;						//time to give team orders
	float lastflagcapture_time;						//last time a flag was captured
	int numteammates;								//number of team mates
	int redflagstatus;								//0 = at base, 1 = not at base
	int blueflagstatus;								//0 = at base, 1 = not at base
	int neutralflagstatus;							//0 = at base, 1 = our team has flag, 2 = enemy team has flag, 3 = enemy team dropped the flag
	int flagstatuschanged;							//flag status changed
	int forceorders;								//true if forced to give orders
	int flagcarrier;								//team mate carrying the enemy flag
	int ctfstrategy;								//ctf strategy
	char subteam[32];								//sub team name
	float formation_dist;							//formation team mate intervening space
	char formation_teammate[16];					//netname of the team mate the bot uses for relative positioning
	float formation_angle;							//angle relative to the formation team mate
	vec3_t formation_dir;							//the direction the formation is moving in
	vec3_t formation_origin;						//origin the bot uses for relative positioning
	bot_goal_t formation_goal;						//formation goal

	bot_activategoal_t* activatestack;				//first activate goal on the stack
	bot_activategoal_t activategoalheap[MAX_ACTIVATESTACK];	//activate goal heap

	bot_waypoint_t* checkpoints;					//check points
	bot_waypoint_t* patrolpoints;					//patrol points
	bot_waypoint_t* curpatrolpoint;					//current patrol point the bot is going for
	int patrolflags;								//patrol flags

// jmarshall
	bot_goal_t  currentgoal;

	vec3_t		currentMoveGoal;
	vec3_t		movement_waypoints[NAV_MAX_PATHSTEPS];
	int			numMovementWaypoints;
	int			currentWaypoint;

	vec3_t		last_origin;
	vec3_t		very_short_term_origin;

	int			stuck_time;

	vec3_t		last_enemy_visible_position;
	vec3_t		random_move_position;
	// jmarshall end

	bot_input_t input;
} bot_state_t;

void BotUpdateInput( bot_state_t* bs, int time, int elapsed_time );
qboolean BotIsDead( bot_state_t* bs );

void AIEnter_Respawn( bot_state_t* bs, char* s );

extern float floattime;
#define FloatTime() floattime


float Characteristic_BFloat( bot_character_t* ch, int index, float min, float max );
void Characteristic_String( bot_character_t* ch, int index, char* buf, int size );

bot_character_t* BotLoadCharacterFromFile( char* charfile, int skill );

void BotInitLevelItems( void );

void BotChooseWeapon( bot_state_t* bs );

inline float AAS_Time()
{
	return floattime;
}

unsigned short int BotTravelTime( vec3_t start, vec3_t end );
#endif

#define MAX_BOT_INVENTORY 256

typedef enum
{
	NULLMOVEFLAG = -1,
	MOVE_PRONE,
	MOVE_CROUCH,
	MOVE_WALK,
	MOVE_RUN2,
	MOVE_SPRINT,
	MOVE_JUMP,
} botMoveFlags_t;

//
// rvmBotUtil
//
class rvmBotUtil
{
public:
	static float random()
	{
		return ( ( rand() & 0x7fff ) / ( ( float )0x7fff ) );
	}

	static float crandom()
	{
		return ( 2.0 * ( random() - 0.5 ) );
	}
};

#include "Bot_char.h"
#include "Bot_weights.h"
#include "Bot_weapons.h"
#include "Bot_goal.h"

struct bot_state_t
{
	bot_state_t()
	{
		character = NULL;
		gs = 0;
		ws = 0;
		Reset();
	}
	void Reset()
	{		
		attackerEntity = NULL;
		client = 0;
		entitynum = 0;
		setupcount = 0;
		entergame_time = 0;
		weaponnum = 0;
		lasthealth = 0;
		ltg_time = 0;
		weaponchange_time = 0;
		enemy = 0;
		enemyvisible_time = 0;
		enemysuicide = 0;
		enemysight_time = 0;
		check_time = 0;
		nbg_time = 0;
		enemydeath_time = 0;
		teleport_time = 0;
		flags = 0;
		firethrottlewait_time = 0;
		attackchase_time = 0;
		attackcrouch_time = 0;
		attackstrafe_time = 0;
		attackjump_time = 0;
		firethrottleshoot_time = 0;
		chase_time = 0;
		thinktime = 0;
		useRandomPosition = false;
		aimtarget.Zero();
		lastenemyorigin.Zero();
		origin.Zero();
		enemyorigin.Zero();
		random_move_position.Zero();
		last_enemy_visible_position.Zero();
		viewangles.Zero();
		eye.Zero();
		memset( &inventory[0], 0, sizeof( inventory ) );
	}

	bot_character_t* character;
	int gs;
	int ws;
	int enemy;
	int client;
	idEntity* attackerEntity;
	int lasthealth;
	int entitynum;
	int setupcount;
	int ltg_time;
	int flags;
	int weaponnum;
	bool useRandomPosition;
	float thinktime;
	float chase_time;
	float attackjump_time;
	float attackcrouch_time;
	float attackstrafe_time;
	float attackchase_time;
	float firethrottlewait_time;
	float firethrottleshoot_time;
	float nbg_time;									//nearby goal time
	float entergame_time;
	float weaponchange_time;
	float check_time;
	float teleport_time;
	float enemyvisible_time;						//time the enemy was last visible
	int enemysuicide;								//true when the enemy of the bot suicides
	float enemysight_time;							//time before reacting to enemy
	float enemydeath_time;							//time the enemy died
	float aggressiveAttackTime;
	idVec3 origin;
	idVec3 aimtarget;
	idVec3 random_move_position;
	idVec3 last_enemy_visible_position;
	idAngles viewangles;
	idVec3 enemyorigin;
	idVec3 eye;
	idVec3 lastenemyorigin;
	int inventory[MAX_BOT_INVENTORY];
	bot_goal_t	currentGoal;
	bot_input_t	botinput;
};

#include "Bot_chat.h"

#define Bot_Time() ((float)gameLocal.time / 1000.0f)

//
// rvmBot
//
class rvmBot : public idPlayer
{
public:
	friend class rvmBotAI;

	CLASS_PROTOTYPE( rvmBot );

	rvmBot();
	~rvmBot();

	void			Spawn( void );
	virtual void	Think( void ) /*override //k*/;
	virtual void	SpawnToPoint( const idVec3& spawn_origin, const idAngles& spawn_angles ) /*override /k*/;
	virtual	void	Damage( idEntity* inflictor, idEntity* attacker, const idVec3& dir, const char* damageDefName, const float damageScale, const int location ) /*override //k*/;
	virtual void	InflictedDamageEvent(idEntity* target) /*override //k*/;
	virtual void	StateThreadChanged(void) /*override //k*/;
	virtual bool	IsBot(void) /*override //k*/ { return true; }

	void			SetEnemy( idPlayer* player, idVec3 origin );

	void			BotInputFrame( void );
	void			Bot_ResetUcmd( usercmd_t& ucmd );
	

	static void		PresenceTypeBoundingBox( int presencetype, idVec3& mins, idVec3& maxs );
private:
	void			BotSendChatMessage(botChat_t chat, const char *targetName);

	void			BotInputToUserCommand( bot_input_t* bi, usercmd_t* ucmd, int time );

	void			BotMoveToGoalOrigin( idVec3 goalOrigin );

	void			ServerThink( void );
	void			BotUpdateInventory( void );

	bool			HasWeapon( int index )
	{
		return inventory.weapons & ( 1 << index );
	}
private:
	bot_state_t		bs;
	bool			hasSpawned;

private:
	int				weapon_machinegun;
	int				weapon_shotgun;
	int				weapon_plasmagun;
	int				weapon_rocketlauncher;
protected:
	bool			BotIsDead(bot_state_t* bs);
	bool			BotReachedGoal(bot_state_t* bs, bot_goal_t* goal);
	int				BotGetItemLongTermGoal(bot_state_t* bs, int tfl, bot_goal_t* goal);
	void			BotChooseWeapon(bot_state_t* bs);
	int				BotFindEnemy(bot_state_t* bs, int curenemy);
	bool			EntityIsDead(idEntity* entity);
	float			BotEntityVisible(int viewer, idVec3 eye, idAngles viewangles, float fov, int ent);
	float			BotEntityVisibleTest(int viewer, idVec3 eye, idAngles viewangles, float fov, int ent, bool allowHeightTest);
	void			BotUpdateBattleInventory(bot_state_t* bs, int enemy);
	float			BotAggression(bot_state_t* bs);
	int				BotWantsToRetreat(bot_state_t* bs);
	void			BotBattleUseItems(bot_state_t* bs);
	void			BotAimAtEnemy(bot_state_t* bs);
	void			BotCheckAttack(bot_state_t* bs);
	bool			BotWantsToChase(bot_state_t* bs);
	int				BotNearbyGoal(bot_state_t* bs, int tfl, bot_goal_t* ltg, float range);
	void			BotGetRandomPointNearPosition(idVec3 point, idVec3& randomPoint, float radius);
	int				BotMoveInRandomDirection(bot_state_t* bs);
	void			BotMoveToGoal(bot_state_t* bs, bot_goal_t* goal);

	void			MoveToCoverPoint(void);

	static int	WP_MACHINEGUN;
	static int	WP_SHOTGUN;
	static int	WP_PLASMAGUN;
	static int	WP_ROCKET_LAUNCHER;
public:
	stateResult_t	state_Chase(const stateParms_t& parms);
	stateResult_t	state_BattleFight(const stateParms_t& parms);
	stateResult_t	state_BattleNBG(const stateParms_t& parms);
	stateResult_t	state_Retreat(const stateParms_t& parms);
	stateResult_t	state_Respawn(const stateParms_t& parms);
	stateResult_t	state_SeekNBG(const stateParms_t& parms);
	stateResult_t	state_SeekLTG(const stateParms_t& parms);
	stateResult_t	state_Attacked(const stateParms_t& parms);

	CLASS_STATES_PROTOTYPE(rvmBot);
private:
	idAAS*			aas;
};


extern idCVar bot_skill;

