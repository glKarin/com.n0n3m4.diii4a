// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __BOTAI_H__
#define __BOTAI_H__

#include "aas/AAS.h"
#include "aas/ObstacleAvoidance.h"
#include "BotAI_Routes.h"
#include "BotAI_VNodes.h"
#include "../ContentMask.h"

#define AAS_PLAYER				0
#define AAS_VEHICLE				1

#define MEDIC_RANGE				1500.0f		//mal: how far a medic's "awareness" of hurt/dead mates extends by default
#define MEDIC_RANGE_BUSY		800.0f		//mal: how far a medic's "awareness" of hurt/dead mates extends when busy (escorting, already healing, carrying obj, etc).
#define MEDIC_RANGE_CARRIER		500.0f		//mal: how far a medic's "awareness" of hurt/dead mates extends when carrier
#define MEDIC_RANGE_REQUEST		3000.0f

#define FOLLOW_RANGE			2000.0f		//mal: how far we'll follow a teammate

#define FDA_AUTO_GRAB_DIST		2500.0f

#define MAX_PATROL_DELIVER_CLIENTS	1
#define PATROL_DELIVER_TEST_DIST	350.0f

#define	IGNORE_ITEM_TIME		15000

#define	ITEM_PACK_OFFSET		24.0f

#define FLYER_WORRY_ABOUT_ROCKETS_MAX_DIST	2500.0f

#define MEDIC_ACK_MIN_DIST		150.0f

#define TOO_CLOSE_TO_GOAL_TO_FOLLOW_DIST	1000.0f

#define TOO_CLOSE_TO_DELIVER_TO_FOLLOW_DIST 1500.0f

#define FLYER_AVOID_DANGER_TIME	2500

#define IGNORE_PLANTED_CHARGE_TIME	10000

#define IGNORE_PATH_NODE_TIME	5000

#define BOT_WILL_ATTACK_TURRETS_IN_COMBAT_DIST 1500.0f

#define	MAX_FRAMES_BLOCKED_BY_OBSTACLE_ON_FOOT				50
#define MAX_FRAMES_BLOCKED_BY_OBSTACLE_IN_VEHICLE			50

#define EASY_MODE_CHANCE_WILL_BUILD_DEPLOYABLE_OR_USE_VEHICLE 40

#define RANDOMLY_LOOK_AROUND_WHILE_VEHICLE_GUNNER_CHANCE	95

#define	MAX_MOVE_ATTEMPTS	10

#define GUNNER_ATTACK_DEPLOYABLE_DIST 8192.0f

#define	IGNORE_CLASS_GOALS_TIME	7000

#define IGNORE_IF_JUST_LEFT_VEHICLE_TIME	5000

#define REPAIR_DRONE_RANGE		625.0f

#define MIN_NUM_INVESTIGATE_CLIENTS	3

#define MAX_NUM_DEFEND_CHARGE_CLIENTS	3

#define MAX_REPAIR_DIST			450.0f

#define	SUPPLY_CRATE_DELAY_TIME 20000

#define MAX_CONSIDER_HUMAN_FOR_RIDE_DIST 900.0f

#define MIN_CLASS_CHANGE_DELAY	15000
#define MIN_WEAPON_CHANGE_DELAY 180000

#define TAKE_SPAWN_IN_DEMO_MODE_CHANCE 25

#define CLOSE_ENOUGH_TO_INVESTIGATE_GOAL_RANGE 75.0f

#define CRITICAL_ENEMY_CLOSE_TO_GOAL_RANGE	350.0f

#define MAX_DEPLOYABLE_FLANK_POINTS 6

#define TRAINING_MODE_RANGE_ADDITION	2000.0f

#define KILLING_SPREE			4

#define MAX_ATTACK_GRIEFER_RANGE 8192.0f

#define IGNORE_NEW_ENEMIES_TIME 5000 //mal: ignore new enemies for 5 seconds when find an important one!

#define MAX_AI_NODE_SWITCH		25

#define MAX_MOVE_ERRORS			5

#define SHIELD_CONSIDER_RANGE	900.0f

#define SHIELD_FIRING_RANGE		250.0f

#define SHEILD_FIRE_CONSIDER_RANGE 1500.0f

#define SMOKE_CONSIDER_RANGE	1500.0f

#define MAX_OWN_VEHICLE_TIME	20000	//mal: time enough for the human to jump out and repair, or look around for a bit, before jumping back in.

#define BACKSTAB_DIST			145.0f

#define DEPLOYABLE_PAUSE_TIME	1500

#define THIRD_EYE_SAFE_RANGE	700.0f

#define TANK_MINIGUN_RANGE		3000.0f

#define MINIMUM_VEHICLE_IGNORE_TIME	3000.0f

#define COS_TURRET_ANGLE_ARC	0.38f

#define DRONE_REPAIR_TIME		60000

#define MIN_TELEPORTER_RANGE	2500.0f
#define MAX_TELEPORTER_RANGE	5000.0f

#define MAX_NUM_OF_FRIENDS_OF_VICTIM 1

#define SPAWNHOST_DESTROY_ORDER_RANGE 5000.0f

#define AIRCRAFT_ATTACK_DIST	10000.0f

#define SPAWN_CONSIDER_TIME		7000

#define MEDIC_IGNORE_TIME		15000		//mal: medics ignore unreachable clients 
#define ENEMY_IGNORE_TIME		7000		//mal: enemies are ignored for 7 seconds
#define BODY_IGNORE_TIME		15000		//mal: bodies are ignored for 15 seconds
#define CLIENT_IGNORE_TIME		15000		//mal: regular clients are ignored for 15 seconds
#define ACTION_IGNORE_TIME		7000		//mal: actions are ignored for 7 seconds.
#define DEPLOYABLE_IGNORE_TIME	30000

#define ENEMY_SIGHT_DIST			3000.0f		//2500.0f		//mal: how far a bot will by default see an enemy, can be overriden by script	   //mal_TODO: good value?
#define ENEMY_SIGHT_BUSY_DIST		2500.0f		//mal: how far a bot will see when busy with another enemy		//mal_TODO: good value?
#define ENEMY_VEHICLE_SIGHT_DIST	8000.0f

#define COVERT_SIGHT_DIST		512.0f

#define MAX_PATH_FAILED_COUNT	12

#define GOAL_ORIGIN_DIST		150.0f

const int BOT_VISIBILITY_TRACE_MASK = MASK_SHOT_BOUNDINGBOX | MASK_VEHICLESOLID | CONTENTS_FORCEFIELD | CONTENTS_MONSTER | MASK_SHOT_RENDERMODEL;

#define SNIPER_HEARING_DIST		4096.0f
#define VEHICLE_HEARING_DIST	6000.0f
#define PLAYER_HEARING_DIST		1200.0f

#define DEFENSE_CAMP_MATE_RANGE	150.0f

#define WARMUP_ACTION_TIME		2500

#define PLASMA_CANNON_RANGE		8000.0f

#define FLYER_HIVE_SIGHT_DIST		1500.0f
#define FLYER_HIVE_ATTACK_DIST		125.0f
#define FLYER_HIVE_HEIGHT_OFFSET	82.0f

#define MEDIC_REQUEST_CONSIDER_TIME	10000

#define SLIPGATE_ORIGIN			idVec3( -9627.0f, 9098.0f, 1627.0f )

#define	NULL_AREANUM				0

#define DEPLOYABLE_ORIGIN_OFFSET	32.0f
#define DEPLOYABLE_PATH_ORIGIN_OFFSET	16.0f

#define VEHICLE_PATH_ORIGIN_OFFSET	16.0f

#define TK_REVIVE_CHAT_TIME		10000

#define HUMAN_OWN_VEHICLE_DIST	350.0f

#define CLOSE_TO_GOAL_RANGE		3000.0f

#define MAX_RIDE_DIST			6000.0f

#define MAX_MINE_CHECK_DIST		2048.0f

#define VEHICLE_NODE_TIMEOUT	10000

#define MAX_MOVE_FAILED_TIME	5000
#define MAX_MOVE_FAILED_COUNT	150

#define DEFAULT_DEPLOYABLE_COVERAGE_RANGE 6192.0f

#define AVT_NEAR_MCP_CONSIDER_RANGE 1900.0f

#define LIGHTNING_GUN_DIST		1900.0f	//mal: a little shorter then the actual range - so the bots always hit the target.

#define ACTION_CHECK_TEAMMATE_DIST	512.0f

#define TRAVEL_TIME_MULTIPLY	4

#define SAFE_PLAYER_BODY_WIDTH  65.0f

#define MAX_SHOT_BLOCKED_COUNT	30

#define BOT_ATTACK_DEPLOYABLE_RANGE 1700.0f

#define ARTY_LOCKON_TIME		900 //1200

#define THIRD_EYE_RANGE			2048.0f

#define MAX_LANDMINE_DIST		256.0f

#define GRENADE_IGNORE_TIME		7000

#define ESCORT_RANGE			3000.0f

#define REQUEST_CONSIDER_TIME	5000

#define SPAWNHOST_RELEVANT_DIST 6000.0f

#define GRENADE_THROW_MAXDIST	1200.0f
#define GRENADE_THROW_MINDIST	1000.0f
#define GRENADE_ATTACK_DIST		1700.0f

#define SHOTGUN_DIST			1500.0f

#define BOT_AWARENESS_DIST		1500.0f

#define OBJ_AWARENESS_DIST		3500.0f

#define SPAWNHOST_RANGE			1500.0f

#define	TACTICAL_PAUSE_ACTION	9999999

#define	FIX_DEPLOYABLE_DIST		1900.0f	

#define BASE_ANTI_MISSILE_RANGE	3000.0f

#define ANTI_MISSILE_RANGE		7000.0f

#define GAME_PLAY_RANGE			8192.0f

#define CLOSE_MINE_DIST			700.0f

#define INFANTRY_ATTACK_HEAVY_DIST	1200.0f

#define CLIENT_HAS_NO_VEHICLE	-1

#define DEPLOYABLE_GOAL_DIST	8000.0f

#define MG_REPAIR_MAX_DIST		5000.0f

#define DEPLOYABLE_DISABLED_PERCENT			1.5f

#define ATTACK_APT_DIST			1700.0f
#define HIVE_ATTACK_APT_DIST	3000.0f

#define MINE_TOO_FAR_DIST		512.0f
#define MINE_TOO_CLOSE_DIST		300.0f

#define LOOK_AT_GOAL_DIST		350.0f

#define ATTACK_MCP_DIST			1200.0f

#define VEHICLE_PAUSE_TIME		50

#define ENEMY_CHASE_DIST		1500.0f		//mal: max dist bot will chase you					//mal_TODO: good value?

#define FOOTSTEP_DIST			500.0f		//mal: how far your footsteps sound					//mal_TODO: is this accurate?

#define FOLLOW_MATE_TIMELIMIT	120000		//mal: default is follow this teammate for a 2 minutes.
#define DEFAULT_LTG_TIME		120000		//mal: default timelimit for doing an ltg - 2 minutes.

#define TACTICAL_ACTION_TIME	5000		//mal: how long to execute a tactical action
#define TACTICAL_HIVE_TIME		30000		//mal: we do the flyer hive for longer.

#define BOT_INFINITY			99999999	//mal: something to do for a long time.

#define MAX_IGNORE_ENTITIES		8			//mal: how many enemies/clients/bodies a bot can ignore at a time.

#define TIME_BEFORE_CHARGE_BLOWS_AWARENESS_TIME		4

#define IGNORE_ICARUS_TIME		15000

#define PLIERS_RANGE			125.0f

#define ITEM_RANGE				900.0f

#define HORNET_SLOWDOWN_DIST	1700.0f //mal: ugly hacks.
#define HORNET_TOOFAST_SPEED	1500.0f

#define	BOT_RECOVER_OBJ_RANGE	4500.0f
#define BOT_INVESTIGATE_RANGE	3500.0f

#define MAX_HIVE_RANGE			8192.0f

#define MAX_DELAYED_CHATS		4
#define MAX_DANGERS				8

#define	SLIPGATE_DIVIDING_PLANE_X_VALUE -3120.0f //mal: HACK

#define BODY_IGNORE_RANGE		2500.0f

#define MAX_MCP_ATTRACT_DIST	2500.0f

//mal: rough approximations
#define WALKING_SPEED			78.0f
#define RUNNING_SPEED			250.0f
#define SPRINTING_SPEED			350.0f

#define BASE_VEHICLE_SPEED		512.0f

#define TK_REVIVE_HEALTH		20

#define WEAPON_LOCK_DIST		8192.0f

#define MAX_SNIPER_VIEW_DIST	12000.0f	//mal: test to see if this works
#define MAX_SNIPER_CROSSHAIR_DANGER_DIST 700.0f

#define MCP_PARKED_DIST			150.0f

//mal: what kind of charge should we look for?
#define ARM						true 
#define DISARM					false

#define PACKS_HAVE_NO_NADES		//mal: put in for a last minute change to the gameplay behavior

#ifdef _XENON
	#define STROGG_INSTANT_REVIVE	//mal: on the console, strogg have instant revives.
#endif

enum bot_LTG_Types_t {
	NO_LTG = -1,
	FOLLOW_TEAMMATE,
	ROAM_GOAL,
	CAMP_GOAL,
	BUILD_GOAL,
	HACK_GOAL,
	HUNT_GOAL,					// the AI has a client it wants to kill for some reason.
	PLANT_GOAL,
	DEFUSE_GOAL,
	FIRESUPPORT_CAMP_GOAL,
	MINE_GOAL,
	SNIPE_GOAL,
	INVESTIGATE_ACTION,
	ENTER_VEHICLE,
	FIX_MCP,
	DRIVE_MCP,
	MINOR_BUILD_GOAL,
	FDA_GOAL,
	STEAL_SPAWN_GOAL,
	STEAL_GOAL,
	DELIVER_GOAL,
	RECOVER_GOAL,
	DROP_DEPLOYABLE_GOAL,
	DESTROY_DEPLOYABLE_GOAL,
	HACK_DEPLOYABLE_GOAL,
	GRAB_VEHICLE_GOAL,
	DEFENSE_CAMP_GOAL,
	ENTER_HEAVY_VEHICLE,
	MINOR_BUILD_MG,
	MG_CAMP_GOAL,
	FLYER_HIVE_GOAL,
	MG_REPAIR_GOAL,
	DROP_PRIORITY_DEPLOYABLE_GOAL,
	PRIORITY_MINE_GOAL,
	FOLLOW_TEAMMATE_BY_REQUEST,
	ENTER_VEHICLE_GOAL,
	ACTOR_GOAL,
	LAST_ACTOR_GOAL,
	THIRD_EYE_GOAL,
	PROTECT_CHARGE,
	PATROL_DELIVER_GOAL
	
//mal_TODO: add more here as you add them in the AI network

};


enum bot_Vehicle_LTG_Types_t {
	NO_VEHICLE_LTG = -1,
	V_ROAM_GOAL,		//mal: this is vehicle specific
	V_CAMP_GOAL,
	V_GOTO_REVIVE_MATE,
	V_TRAVEL_GOAL,
	V_FOLLOW_TEAMMATE,
	V_STOP_VEHICLE,
	V_DRIVE_MCP,
	V_DESTROY_DEPLOYABLE,
	V_ORIGIN_GOAL,
	V_DRIVE_MCP_ROUTE,
	V_HUNT_GOAL
};

enum bot_NBG_Types_t {
	NO_NBG = -1,
	REVIVE_TEAMMATE,
	TK_REVIVE_TEAMMATE,	
	SUPPLY_TEAMMATE,
	CREATE_SPAWNHOST,
	HAZE_ENEMY,
	CAMP,
	BUILD,
	PLANT_MINE,
	PLANT_BOMB,
	HACK,
	STEAL_UNIFORM,
	STALK_VICTIM,
	DEFUSE_BOMB,
	GRAB_SUPPLIES,
	BUG_FOR_SUPPLIES,
	AVOID_DANGER,
	SNIPE,
	DESTROY_DANGER,
	FIX_VEHICLE,
	DESTROY_MCP,
	DESTORY_SPAWNHOST,
	GRAB_SPAWN,
	FIX_DEPLOYABLE,
	DROP_DEPLOYABLE,
	DESTROY_DEPLOYABLE,
	GRAB_SPAWNHOST,
	HACK_DEPLOYABLE,
	DEFENSE_CAMP,
	MG_CAMP,
	FLYER_HIVE_ATTACK,
	FIX_GUN,
	PAUSE,
	ENG_ATTACK_AVT,
	FIXING_MCP,
	DROPPING_SHIELD,
	INVESTIGATE_CAMP,
	DROPPING_SMOKE_FOR_MATE

//mal_TODO: add more here as you add them in the AI network

};

enum bombStates_t {
	BOMB_NULL,		// the bot has no bomb out there, or its not armed yet.
	BOMB_ARMED		// the bot's bomb is hot!
};

enum entityTypes_t {
	ENTITY_NULL,
	ENTITY_PLAYER,
	ENTITY_DEPLOYABLE,
	ENTITY_VEHICLE
};

struct dangerList_t {
	int						num;
	int						time;
	int						ownerNum;
	dangerTypes_t			type;
	idVec3					origin;
	idMat3					dir;		//mal: only airstrikes need this.
};

struct genericNumTimeList_t {
	int						num;
	int						time;
};

struct plantedChargeInfo_t {	//mal: tracks the player's HE/Plasma charges
	bool					checkedAreaNum;
	bool					isOnObjective;
	int						entNum;
	int						areaNum;
	int						spawnID;
	int						explodeTime;				//mal: when this charge will explode
	int						ownerSpawnID;
	int						ownerEntNum;
	bombStates_t			state;						//mal: 0 = not deployed, 1 = deployed, 2 = armed.
	playerTeamTypes_t		team;
	idVec3					origin;
	idBounds				bbox;
};

struct plantedMineInfo_t {	//mal: tracks the player's landmines
	bool					spotted;
	bool					selfArming;
	int						entNum;
	int						spawnID;
	float					xySpeed;
	bombStates_t			state;						//mal: 0 = not deployed, 1 = deployed, 2 = armed.
	idBounds				bbox;
	idVec3					origin;
};

struct grenadeInfo_t {			//mal: tracks the player's EMP/Shrap grenades
	int						entNum;
	int						spawnID;
	float					xySpeed;
	idVec3					origin;
};

struct airstrikeInfo_t {			//mal: tracks the player's airstrikes
	int						entNum;
	int						spawnID;
	int						timeTilStrike;
	int						oldEntNum;
	float					xySpeed;
	idVec3					origin;
	idMat3					dir;
};

struct botChats_t {
	bool					forceChat;
	int						delayTime;
	botChatTypes_t			chat;
};

struct deployableFlankPoints_t {
	bool					checked;
	idVec3					flankPoint;
};

struct deployableInfo_t {
	bool					inPlace;					//mal: will be false if its being dropped in still
	bool					disabled;					//mal: will be true if health < 50%, or is emped.
	int						ownerClientNum;
	int						health;
	int						type;
	int						enemyEntNum;				//mal: who our current enemy is. Could be a player or a vehicle.
	int						areaNum;
	int						areaNumVehicle;
	int						entNum;
	int						spawnID;
	int						maxHealth;
	float					maxAttackRange;				//mal: how far this deployable can reach.
	float					minAttackRange;				//mal: if your closer then this, it wont attack you.
	playerTeamTypes_t		team;
	deployableFlankPoints_t flankPoints[ MAX_DEPLOYABLE_FLANK_POINTS ];
	idVec3					origin;
	idMat3					axis;
	idBounds				bbox;
	idBox					dangerAreaBox;
};

struct proxyInfo_t { //mal_TODO: keep adding more info for vehicles, as its needed
	bool					isEmpty;
	bool					isDeployed;
	bool					isEMPed;
	bool					isOwned;
	bool					neverDriven;
	bool					isFlipped;
	bool					isAmphibious;
	bool					isAirborneVehicle;
	bool					isAirborneAttackVehicle;
	bool					isImmobilized;
	bool					inWater;
	bool					inPlayZone;
	bool					inSiegeMode;
	bool					hasGroundContact;
	bool					hasFreeSeat;	//mal: does this vehicle have a seat open?
	bool					isCareening;	//mal: this vehicle is out of control!
	bool					canRotateInPlace;
	bool					isBoobyTrapped;
	int						damagedPartsCount;		//mal: this will only be true if the vehicle has missing wheels/legs/whatever that affect its ability to drive properly, NOT health.
	int						entNum;
	int						spawnID;
	int						ownerNum;
	int						health;
	int						maxHealth;
	int						areaNum;
	int						areaNumVehicle;
	int						driverEntNum;
	int						flags;
	int						actionRouteNumber;
	float					xyspeed;
	float					forwardSpeed;
	float					wheelsAreOnGround;
	playerTeamTypes_t		team;
	playerVehicleTypes_t	type;
	idBounds				bbox;
	idVec3					origin;
	idVec3					aasOrigin;
	idVec3					aasVehicleOrigin;
	idMat3					axis;
};

struct supplyPackInfo_t {
	bool					checkedAreaNum;
	bool					inPlayZone;
	bool					available;
	bool					inWater;
	int						entNum;
	int						spawnID;
	int						areaNum;
	float					xySpeed; //mal: want to make sure the pack isn't moving!
	playerTeamTypes_t		team; //mal: team that my owner was in when I was born ( in case he changes ).
	idVec3					origin;	//mal: my current origin - this will be actively tracked while I'm alive ( in case I move for some reason ).
	idBounds				bbox;	//mal: only supply crates really need this.
};

struct killedPlayersInfo_t {
	int						clientNum; //mal: client we killed last.
	idVec3					ourOrigin; //mal: where we were when client died.
};


enum bot_COMBAT_Types_t {
	NO_COMBAT_TYPE = -1,
	COMBAT_REVIVE_MATE,
	COMBAT_GRAB_VEHICLE,
	COMBAT_ATTACK_TURRET

//mal_TODO: add more here as you add them in the AI network





};

enum weaponBankTypes_t {
	RESET_WEAPON = -2,	//mal: if the bot needs to reset the weapon for some purpose.
	NO_WEAPON = -1, // when the bot doesn't want to switch weapons
	MELEE, // 0: KNIFE, blade, fists, whatever.
	SIDEARM, // 1:
	GUN, // 2:
	NADE, // 3:
	SPECIAL1, // 4: defrib, pliers, tools, deploy tools
	SPECIAL2, // 5: health pack, HE charge, landmine, aircan
	GUN_ALT, // 6: the alt fire mode of the main gun (nade launcher, homing missle, etc).
	SPECIAL1_ALT // 7: deploy tools
};

enum bot_AI_States_t {
	LTG,
	NBG,
	COMBAT,
	VLTG,
	VCOMBAT
};

enum targetTypes_t {
	NOTYPE = -1,
	CLIENT,
	BODY,
	LOCATION,
	HEAL,
	REARM,
	HEAL_REQUESTED,
	REARM_REQUESTED,
	LANDMINE_DANGER,
	DEPLOYABLE_DANGER,
	STROGG_HIVE_DANGER,
	GDF_CAM_DANGER,
	VEHICLE,
	DEPLOYABLE,
	SUPPLY_CRATE,
	MCP_DANGER,
	MCP_GOAL,
	REARM_REVIVE
};

enum botTurnTypes_t {
    INSTANT_TURN,
	SMOOTH_TURN,
	AIM_TURN,
	FAST_TURN		// a bit inbetween INSTANT and SMOOTH
};

enum botViewTypes_t {
	VIEW_ANGLES,	// look at a specific angle
	VIEW_ORIGIN,	// look at a specific point
	VIEW_ENTITY,	// look at a specific entity
	VIEW_MOVEMENT,	// look in the direction we're moving
	VIEW_RANDOM,	// look at a random point - gets us off a ladder we may be stuck on.
	VIEW_REVERSE
};

enum botMoveTypes_t {
    NULLMOVETYPE = -1,
    RANDOM_JUMP, // straight jump, doesn't veer off move direction
	QUICK_JUMP,	 // straight jump, MUCH faster then a random jump
	LOCATION_JUMP, //mal: theres a certain spot we need to jump at, so bot will need to know to look for the goal spot we're jumping to.
	LOCATION_BARRIERJUMP,
	LOCATION_WALKOFFLEDGE,
	STRAFE_RIGHT,
	STRAFE_LEFT,
	RANDOM_JUMP_RIGHT,
	RANDOM_JUMP_LEFT,
	HAND_BRAKE,
	AIR_BRAKE, //mal: for planes
	FULL_STOP, //mal: stop NOW!
	LAND,	   //mal: for planes - land this craft!
	LEAN_LEFT,
	LEAN_RIGHT,
	TAUNT_MOVE,
	AIR_COAST,
	SLOWMOVE,
	REVERSEMOVE,
	RANDOM_DIR_JUMP,
	ICARUS_BOOST,
	IGNORE_ALTITUDE,
	BACKSTEP,
	SKIP_MOVE
};

enum botAttackMoveTypes_t {
	COMBAT_MOVE_NULL = -1,
	KNIFE_ATTACK,		//movements we do with a knife
    RUN_N_GUN_ATTACK, // bascially run towards the enemy, blasting them as we go.
	CROUCH_ATTACK, // will crouch down and fire.
	PRONE_ATTACK, // usually snipers/campers use this
	HAL_STRAFE_ATTACK, // various strafes....
	SIDE_STRAFE_ATTACK,
	CIRCLE_STRAFE_ATTACK,
	CRAZY_JUMP_ATTACK, // jump around wildly, to make yourself a harder to hit target. Usually used by guys with knifes, but also in close quarters when reloading
	STAND_GROUND_ATTACK, // stands there and fights
	GRENADE_ATTACK,
	AVOID_DANGER_ATTACK,
	NULL_MOVE_ATTACK,
	MOVETO_SHIELD_ATTACK,
	REVIVE_MATE_ATTACK,

//mal: vehicle based combat movements
	VEHICLE_STAND_GROUND,
	VEHICLE_NULL_MOVEMENT, // used mostly by gunners in a vehicle.
	VEHICLE_RANDOM_MOVEMENT,
	VEHICLE_RAM_ATTACK_MOVEMENT,
	VEHICLE_CHASE_MOVEMENT,
	VEHICLE_AIR_MOVEMENT
};

struct clientInfo_t;
struct botAIOutput_t;
struct sharedWorldState_t;

class idBotAI {
public:
							idBotAI();
							~idBotAI();

	void					Think();

	void					VThink(); //mal: vehicle specific think function.

	void					Spawn( int playerClass, int playerTeam );

	void					SetBotClientNum( int clientNum ) { botNum = clientNum; }

	void					UpdateAAS();

	const bot_AI_States_t	GetAIState() { return aiState; }
	const bot_LTG_Types_t	GetLTGType() { return ltgType; }
	const bool						GetLTGUseVehicle() { return ltgUseVehicle; }
	const bot_NBG_Types_t	GetNBGType() { return nbgType; }
	const int				GetLTGTarget() { return ltgTarget; }
	const int				GetNBGTarget() { return nbgTarget; }
	const bot_Vehicle_LTG_Types_t	GetVehicleLTGType() { return vLTGType; }
	const int						GetVehicleLTGTarget() { return vLTGTarget; }
	const int				GetEnemyNum() { return enemy; }
	const int						GetActionNum() { return actionNum; }
	const int						GetVehicleSurrenderTime() { return vehicleSurrenderTime; }
	const int						GetVehicleSurrenderClient() { return vehicleSurrenderClient; }

	void					ResetLastActionNum() { lastActionNum = ACTION_NULL; }

	void					ClearBotUcmd( int clientNum );

//mal: major AI nodes
	bool 					Run_LTG_Node();
	bool					Run_NBG_Node();
	bool					Run_Combat_Node();
	bool					Enter_Run_Dead_Node();
	bool					Run_Dead_Node();
	bool					Run_Intermission_Node();
	bool					Run_Warmup_Node();

//mal: major vehicle AI nodes
	bool					Run_VLTG_Node(); //mal: vehicle specific LTG node.
	bool					Run_VCombat_Node(); //mal: vehicle specific Combat node.

//mal: LTG nodes....
	bool					Enter_LTG_FollowMate();
	bool					LTG_FollowMate();
	bool					Enter_LTG_RoamGoal();
	bool					LTG_RoamGoal();
	bool					Enter_LTG_CampGoal();
	bool					LTG_CampGoal();
	bool					Enter_LTG_BuildGoal();
	bool					LTG_BuildGoal();
	bool					LTG_ErrorThink();				//mal: a special node to think when the bot has nothing to think about ( i.e. no goals on map ).
	bool					Enter_LTG_PlantGoal();
	bool					LTG_PlantGoal();
	bool					Enter_LTG_HackGoal();
	bool					LTG_HackGoal();
	bool					Enter_LTG_DefuseGoal();
	bool					LTG_DefuseGoal();
	bool					Enter_LTG_HuntGoal();
	bool					LTG_HuntGoal();
	bool					Enter_LTG_MineGoal();
	bool					LTG_MineGoal();
	bool					Enter_LTG_SnipeGoal();
	bool					LTG_SnipeGoal();
	bool					Enter_LTG_InvestigateGoal();
	bool					LTG_InvestigateGoal();
	bool					Enter_LTG_UseVehicle();
	bool					LTG_UseVehicle();
	bool					Enter_LTG_FixMCP();
	bool					LTG_FixMCP();
	bool					Enter_LTG_SpawnPointGoal();
	bool					LTG_SpawnPointGoal();
	bool					Enter_LTG_DeliverGoal();
	bool					LTG_DeliverGoal();
	bool					Enter_LTG_StealGoal();
	bool					LTG_StealGoal();
	bool					Enter_LTG_RecoverDroppedGoal();
	bool					LTG_RecoverDroppedGoal();
	bool					Enter_LTG_DeployableGoal();
	bool					LTG_DeployableGoal();
	bool					Enter_LTG_DestroyDeployable();
	bool					LTG_DestroyDeployable();
	bool					Enter_LTG_FireSupportGoal();
	bool					LTG_FireSupportGoal();
	bool					Enter_LTG_HackDeployableGoal();
	bool					LTG_HackDeployableGoal();
	bool					Enter_LTG_MG_CampGoal();
	bool					LTG_MG_CampGoal();
	bool					Enter_LTG_FlyerHiveGoal();
	bool					LTG_FlyerHiveGoal();
	bool					Enter_LTG_RepairGunGoal();
	bool					LTG_RepairGunGoal();
	bool					Enter_LTG_EnterVehicleGoal();
	bool					LTG_EnterVehicleGoal();
	bool					Enter_LTG_ActorEscortPlayerToGoal();
	bool					LTG_ActorEscortPlayerToGoal();
	bool					Enter_LTG_ActorGiveFinalBriefingToPlayer();
	bool					LTG_ActorGiveFinalBriefingToPlayer();
	bool					Enter_LTG_ThirdEyeCameraGoal();
	bool					LTG_ThirdEyeCameraGoal();
	bool					Enter_LTG_ProtectCharge();
	bool					LTG_ProtectCharge();
	bool					Enter_LTG_PatrolDeliverGoal();
	bool					LTG_PatrolDeliverGoal();

//mal: vehicle LTG nodes...
    bool					Enter_VLTG_RoamGoal();
	bool					VLTG_RoamGoal();
	bool					Enter_VLTG_GotoReviveMate();
	bool					VLTG_GotoReviveMate();
	bool					Enter_VLTG_TravelGoal();
	bool					VLTG_TravelGoal();
	bool					Enter_VLTG_RideWithMate();
	bool					VLTG_RideWithMate();
	bool					Enter_VLTG_StopVehicle();
	bool					VLTG_StopVehicle();
	bool					Enter_VLTG_CampGoal();
	bool					VLTG_CampGoal();
	bool					Enter_VLTG_DriveMCPToGoal();
	bool					VLTG_DriveMCPToGoal();
	bool					Enter_VLTG_GroundVehicleDestroyDeployable();
	bool					VLTG_GroundVehicleDestroyDeployable();
	bool					Enter_VLTG_AircraftDestroyDeployable();
	bool					VLTG_AircraftDestroyDeployable();
	bool					Enter_VLTG_TravelToGoalOrigin();
	bool					VLTG_TravelToGoalOrigin();
	bool					Enter_VLTG_DriveMCPToRouteGoal();
	bool					VLTG_DriveMCPToRouteGoal();
	bool					Enter_VLTG_HuntGoal();
	bool					VLTG_HuntGoal();

//mal: some NBG nodes...
	bool					Enter_NBG_SupplyTeammate();
	bool					NBG_SupplyTeammate();
	bool					Enter_NBG_ReviveTeammate();
	bool					NBG_ReviveTeammate();
	bool					Enter_NBG_TKReviveTeammate();
	bool					NBG_TKReviveTeammate();
	bool					Enter_NBG_CreateSpawnHost();
	bool					NBG_CreateSpawnHost();
	bool					Enter_NBG_HumiliateEnemy();
	bool					NBG_HumiliateEnemy();
	bool					Enter_NBG_Camp();
	bool					NBG_Camp();
	bool					Enter_NBG_Build();
	bool					NBG_Build();
	bool					Enter_NBG_PlantBomb();
	bool					NBG_PlantBomb();
	bool					Enter_NBG_Hack();
	bool					NBG_Hack();
	bool					Enter_NBG_StealUniform();
	bool					NBG_StealUniform();
	bool					Enter_NBG_HuntVictim();
	bool					NBG_HuntVictim();
	bool					Enter_NBG_DefuseBomb();
	bool					NBG_DefuseBomb();
	bool					Enter_NBG_GetSupplies();
	bool					NBG_GetSupplies();
	bool					Enter_NBG_BugForSupplies();
	bool					NBG_BugForSupplies();
	bool					Enter_NBG_AvoidDanger();
	bool					NBG_AvoidDanger();
	bool					Enter_NBG_PlantMine();
	bool					NBG_PlantMine();
	bool					Enter_NBG_Snipe();
	bool					NBG_Snipe();
	bool					Enter_NBG_DestroyDanger();
	bool					NBG_DestroyDanger();
	bool					Enter_NBG_FixProxyEntity();
	bool					NBG_FixProxyEntity();
	bool					Enter_NBG_DestroyMCP();
	bool					NBG_DestroyMCP();
	bool					Enter_NBG_DestroySpawnHost();
	bool					NBG_DestroySpawnHost();
	bool					Enter_NBG_GrabSpawnPoint();
	bool					NBG_GrabSpawnPoint();
	bool					Enter_NBG_FixDeployable();
	bool					NBG_FixDeployable();
	bool					Enter_NBG_PlaceDeployable();
	bool					NBG_PlaceDeployable();
	bool					Enter_NBG_DestroyAPTDanger();
	bool					NBG_DestroyAPTDanger();
	bool					Enter_NBG_GrabSpawnHost();
	bool					NBG_GrabSpawnHost();
	bool					Enter_NBG_HackDeployable();
	bool					NBG_HackDeployable();
	bool					Enter_NBG_MG_Camp();
	bool					NBG_MG_Camp();
	bool					Enter_NBG_FlyerHive();
	bool					NBG_FlyerHive();
	bool					Enter_NBG_FixGun();
	bool					NBG_FixGun();
	bool					Enter_NBG_Pause();
	bool					NBG_Pause();
	bool					Enter_NBG_EngAttackAVTNearMCP();
	bool					NBG_EngAttackAVTNearMCP();
	bool					Enter_NBG_ShieldTeammate();
	bool					NBG_ShieldTeammate();
	bool					Enter_NBG_SmokeTeammate();
	bool					NBG_SmokeTeammate();


//mal: some COMBAT nodes
	bool					Enter_COMBAT_Foot_AttackEnemy();
	bool					COMBAT_Foot_AttackEnemy();
	bool					Enter_COMBAT_Foot_ChaseEnemy();
	bool					COMBAT_Foot_ChaseEnemy();
	bool					Enter_COMBAT_Foot_RetreatFromEnemy();
	bool					COMBAT_Foot_RetreatFromEnemy();
	bool					Enter_COMBAT_Foot_EvadeEnemy();
	bool					COMBAT_Foot_EvadeEnemy();
	bool					Enter_COMBAT_Foot_LostEnemyInSmoke();
	bool					COMBAT_Foot_LostEnemyInSmoke();
	bool					Enter_COMBAT_Foot_ReviveTeammate();
	bool					COMBAT_Foot_ReviveTeammate();
	bool					Enter_COMBAT_Foot_ChaseEnemy_Grenade();
	bool					COMBAT_Foot_ChaseEnemy_Grenade();
	bool					Enter_COMBAT_Foot_ChaseEnemy_Aircan();
	bool					COMBAT_Foot_ChaseEnemy_Aircan();
	bool					Enter_COMBAT_Foot_Hide();
	bool					COMBAT_Foot_Hide();
	bool					Enter_COMBAT_Foot_ChaseUnseenEnemy();
	bool					COMBAT_Foot_ChaseUnseenEnemy();
	bool					Enter_COMBAT_Foot_GrabVehicle();
	bool					COMBAT_Foot_GrabVehicle();
	bool					Enter_COMBAT_Foot_AttackTurret();
	bool					COMBAT_Foot_AttackTurret();

	// vehicle combat nodes

	bool					Enter_COMBAT_Vehicle_EvadeEnemy();
	bool					COMBAT_Vehicle_EvadeEnemy();
	bool					Enter_COMBAT_Vehicle_AttackEnemy();
	bool					COMBAT_Vehicle_AttackEnemy();
	bool					Enter_COMBAT_Vehicle_ChaseEnemy();
	bool					COMBAT_Vehicle_ChaseEnemy();


//mal: some COMBAT movement nodes
	bool					Enter_Run_And_Gun_Movement();
	bool					Run_And_Gun_Movement();
	bool					Enter_Knife_Attack_Movement();
	bool					Knife_Attack_Movement();
	bool					Enter_Stand_Ground_Attack_Movement();
	bool					Stand_Ground_Attack_Movement();
	bool					Enter_Hal_Strafe_Attack_Movement();
	bool					Hal_Strafe_Attak_Movement();
	bool					Enter_Crazy_Jump_Attack_Movement();
	bool					Crazy_Jump_Attack_Movement();
	bool					Enter_Side_Strafe_Attack_Movement();
	bool					Side_Strafe_Attack_Movement();
	bool					Enter_Crouch_Attack_Movement();
	bool					Enter_Prone_Attack_Movement();
	bool					Prone_Attack_Movement();
	bool					Crouch_Attack_Movement();
	bool					Enter_Circle_Strafe_Attack_Movement();
	bool					Circle_Strafe_Attack_Movement();
	bool					Enter_Grenade_Attack_Movement();
	bool					Grenade_Attack_Movement();
	bool					Enter_Avoid_Danger_Movement();
	bool					Avoid_Danger_Movement();
	bool					Enter_Null_Move_Attack();
	bool					Null_Move_Attack();
	bool					Enter_MoveTo_Shield_Attack_Movement();
	bool					MoveTo_Shield_Attack_Movement();

//mal: some vehicle COMBAT movement nodes

	bool					Enter_Vehicle_Stand_Ground_Movement();
	bool					Vehicle_Stand_Ground_Movement();
	bool					Enter_Vehicle_NULL_Movement();					//mal: only used by bots who are gunners/passengers in someone else's ride.
	bool					Vehicle_NULL_Movement();
	bool					Enter_Vehicle_Ram_Attack_Movement();
	bool					Vehicle_Ram_Attack_Movement();
	bool					Enter_Vehicle_Chase_Movement();
	bool					Vehicle_Chase_Movement();
	bool					Enter_Vehicle_Random_Movement();
	bool					Vehicle_Random_Movement();
	bool					Enter_Vehicle_Air_Movement();
	bool					Vehicle_Air_To_Air_Movement();
	bool					Vehicle_Air_To_Ground_Movement();
	bool					Buffalo_Air_To_Air_Movement();
	bool					Buffalo_Air_To_Ground_Movement();

	// ram, stand and fight, chase, random move, etc.





//mal: combat utility functions
	bool					Bot_FindEnemy ( int ignoreClientNum );
	bool					Bot_FindBetterEnemy ();
	bool					ClientIsVisibleToBot ( int clientNum, bool useFOV, bool saveEnt );
	bool					ClientIsAudibleToBot ( int clientNum );
	bool					ClientObscuredBySmokeToBot ( int clientNum );
	void					Bot_SetAttackTimeDelay ( bool inFront ); //mal: this sets a delay on how long the bot should take to see enemy, based on bot's state.
	void					Bot_FindBestCombatMovement();
	bool					BotLeftEnemysSight();		//mal: did the bot move out of the enemies sight, or vice-versa?
	void					Bot_ResetEnemy();
	void					Bot_PickBestWeapon( bool useNades );
	bool					Bot_PickPostCombatGoal();
	bool					ClassWeaponCharged( const playerWeaponTypes_t weaponNum ); //mal: does weaponNum have enough charge to be fired this frame?
	void					UpdateEnemyInfo(); //mal: updates the enemy visibility info.
	void					UpdateNonVisEnemyInfo(); //mal: updates the enemy visibility info.
	bool					Bot_ThrowGrenade( const idVec3 &origin, bool fastNade );
	void					Bot_CheckCurrentStateForCombat();
	bool					Bot_ShouldChaseHiddenEnemy( bool chase );
	bool					Bot_CheckShouldUseGrenade( bool targetVisible );
	bool					Bot_CheckShouldUseAircan( bool targetVisible );
	void					Bot_UseCannister( const playerWeaponTypes_t weapType, const idVec3 &origin );
	bool					EnemyValid();
	void					Bot_CheckAttack(); //mal: is it OK to fire our weapon now?
	int						CheckClientAttacker( int clientNum, int checkTimeInSeconds ); // returns which client is attacking the client in question.
	bool					Bot_CanBackStabClient( int clientNum, float backStabDist = BACKSTAB_DIST );
	bool					ClientHasAttackedTeammate( int clientNum, bool criticalOnly, int time );
	bool					DisguisedKillerInArea();
	bool					EnemyIsIgnored( int clientNum );
	void					Bot_IgnoreEnemy( int clientNum, int time );
	bool					VehicleIsIgnored( int vehicleNum );
	void					Bot_IgnoreVehicle( int vehicleNum, int time );
	bool					ClientIsDefusingOurTeamCharge( int clientNum );
	void					Bot_PickChaseType();
	int						Bot_ShouldInvestigateNoise( int clientNum );
	void					Bot_SetTimeOnTarget( bool inVehicle );
	void					Bot_CheckForNearbyVehicleToGrab();
	bool					Bot_CheckEnemyHasLockOn( int clientNum, bool ignoreTargetIfRangeIsGreat = false );
	bool					Client_HasMultipleAttackers( int clientNum );
	bool					Bot_IsNearTeamGoalUnderAttack();
	bool					Bot_IsNearForwardSpawnToGrab();
	bool					DisguisedClientIsActingOdd( int clientNum );
	bool					ClientHasNadeInWorld( int clientNum );
	bool					ClientHasFireSupportInWorld( int clientNum );
	bool					ClientIsDangerous( int clientNum );
	bool					Bot_EnemyAITInArea( const idVec3& org );
	bool					ClientIsMarkedForDeath( int clientNum, bool clearRequest );
	bool					DeployableIsMarkedForDeath( int entNum, bool clearRequest );
	bool					Bot_CheckCovertToolState();
	int						Flyer_FindEnemy( float range );
	bool					Bot_IsUnderAttackFromUnknownEnemy();
	bool					Bot_CanAttackVehicles();
	bool					Bot_HasEnemySniperInArea( float range );
	bool					Bot_IsUnderAttackByAPT( idVec3& turretLocation );
	bool					Bot_TeammateHasClientAsEnemy( int clientNum, int& attackingClient );
	bool					ClientHasAttackedActorsMate( int clientNum, int time );
	bool					Bot_ShouldIgnoreEnemies();
	bool					Bot_CheckIfEnemyHasUsInTheirSightsWhenInAirVehicle();

//mal: vehicle combat/misc utility functions
	int						FindClosestVehicle( float range, const idVec3& org, const playerVehicleTypes_t vehicleType, int vehicleFlags, int vehicleIgnoreFlags, bool emptyOnly );
	void					GetVehicleInfo( int entNum, proxyInfo_t& vehicleInfo ) const;
	const proxyInfo_t *		GetBotVehicleInfo( int entNum ) const;
	void					Bot_FindDeadWhileInVehicle();
	bool					Bot_ShouldUseVehicleForAction( int actionNumber, bool ignoreArmor = false );
	bool					Bot_MeetsVehicleRequirementsForAction( int actionNumber );
	bool					Bot_GetIntoVehicle( int vehicleNum );
	void					Bot_ExitVehicle( bool ignoreMCP = true );
	bool					Bot_VehicleFindEnemy();
	bool					Bot_FindParkingSpotAoundLocaction( idVec3 &loc ); //mal: finds a parking spot near location - something the bot can move toward, without running over its goal.
	bool					VehicleIsValid( int entNum, bool skipSpeedCheck = false, bool addDriverCheck = false );
	bool					VehicleHasGunnerSeatOpen( int entNum );
	bool					InFrontOfVehicle( int vehicleNum, const idVec3 &org, bool precise = false, float preciseValue = 0.60f );
	void					Bot_CheckCurrentStateForVehicleCombat();
	bool					Bot_VehicleCanAttackEnemy( int clientNum );
	void					Bot_PickBestVehicleWeapon();
	void					Bot_PickBestVehiclePosition();
	void					Bot_CheckVehicleAttack();
	bool					Bot_VehicleFindBetterEnemy();
	bool					Bot_ShouldVehicleChaseHiddenEnemy();
	bool					Bot_FindBestVehicleCombatMovement();
	bool					Bot_VehicleCanRamClient( int clientNum );
	void					Bot_PickVehicleChaseType();
	void					Bot_ExitVehicleAINode( bool resetStack );
	bool					Bot_VehicleCanMove( const moveDirections_t direction, float gUnits, bool copyEndPos );
	bool					InAirVehicleGunSights( int vehicleNum, const idVec3 &org );
	bool					Bot_CheckMoveIsClear( idVec3& goalOrigin );
	bool					Bot_CheckVehicleNodePath( const idVec3& goalOrigin, idVec3& pathPoint, bool& resetGoal );
	bool					FindEntityByEntNum( int entNum, idVec3& origin, idBox& bbox );
	bool					Bot_IsInHeavyAttackVehicle( bool ignoreGroundVehicles = false );
	int						Bot_VehicleIsUnderAVTAttack();
	bool					Bot_CheckHumanRequestingTransport();
	bool					Bot_CheckIfClientHasRideWaiting( int clientNum );
	void					IcarusCombatMove();
	bool					ClientIsAudibleToVehicle( int clientNum );
	bool					HumanVehicleOwnerNearby( const playerTeamTypes_t playerTeam, const idVec3 &loc, float range, int vehicleSpawnID );
	bool					Bot_CheckFriendlyEngineerIsNearbyOurVehicle();
	bool					Bot_CheckForHumanNearByWhoMayWantRide();
	int						Bot_GetVehicleGunnerClientNum( int vehicleEntNum );
	bool					VehicleGoalsExistForVehicle( const proxyInfo_t& vehicleInfo );
	bool					Bot_WithObjShouldLeaveVehicle();
	int						Bot_CheckForDeployableTargetsWhileVehicleGunner();
	void					Bot_AttackDeployableTargetsWhileVehicleGunner( int deployableTargetToKill );
	bool					Bot_WhoIsCriticalForCurrentObjShouldLeaveVehicle();


//mal: misc utility functions
	void					UpdateBotsInformation( const playerClassTypes_t playerClass, const playerTeamTypes_t playerTeam, bool botInit );
	void					BotAI_ResetUcmd();
	bool					BotAI_CheckThinkState();
	void					ClearBotState();
	void					Bot_ResetState( bool resetEnemy, bool resetStack );
	void					InitAAS(); //mal: setup the aas for this bot
	void					Bot_IgnoreClient( int clientNum, int time );
	void					Bot_IgnoreAction( int actionNumber, int time );
	bool					Bot_RandomLook( idVec3 &position, bool ignoreDelay = false ); //mal: picks a random position for the bot to look towards
	void					Bot_ExitAINode();
	bool					Bot_NBGIsAvailable( int clientNum, int actionNumber, const bot_NBG_Types_t goalType, int &busyClient );
	bool					Bot_LTGIsAvailable( int clientNum, int actionNumber, const bot_LTG_Types_t goalType, int minNumClients );
	int						Bot_NumClientsDoingLTGGoal( int clientNum, int actionNumber, const bot_LTG_Types_t goalType, idList< int >& busyClients );
	void					Bot_Input(); //mal: sends the bots current user cmds to the game thread.
	void					Bot_LookAtLocation( const idVec3 &spot, const botTurnTypes_t turnType, bool useOriginOnly = false ); // where the bot wants to look
	void					Bot_LookAtEntity( int entityNum, const botTurnTypes_t turnType ); // what the bot wants to look at
	void					Bot_LookAtNothing( const botTurnTypes_t turnType); // just look straight ahead
	int						ClientsInArea( int ignoreClientNum, const idVec3 &org, float range, int team, const playerClassTypes_t clientClass, bool inFront, bool vis2Sky, bool ignoreInvulnerable, bool ignoreDisguised, bool ignoreInVehicle, bool humanOnly = false ); //looks for clients of a specific team, possibly infront of client[ clientNum ].
	int						VehiclesInArea( int ignoreClientNum, const idVec3 &org, float range, int team, bool vis2Sky, int ignoreVehicleNum, bool humanOnly = false ); //looks for vehicles in an area
	bool					InFrontOfClient( int clientNum, const idVec3 &origin, bool precise = false, float preciseValue = 0.80f ); //mal: is this location infront of the bot?
	void					Bot_ClearAngleMods();
	void					CheckBotStuckState();
	void					ResetRandomLook() { randomLookRange = 900.0f; randomLookOrg = vec3_zero; randomLookDelay = 0; }
	void					Bot_CheckWeapon();
	void					BotAI_UpdateStateInfo();
	bool					Bot_CheckHasVisibleMedicNearby();
	bool					LocationVis2Sky( const idVec3 &loc );
	void					ResetBotsAI( bool critical );
	bool					ClientIsValid( int clientNum, int spawnID );
	bool					ClientIsIgnored( int clientNum );
	bool					BodyIsIgnored( int bodyNum );
	bool					SpawnHostIsIgnored( int bodyNum );
	void					Bot_IgnoreBody( int bodyNum, int time );
	void					Bot_IgnoreSpawnHost( int bodyNum, int time );
	bool					ActionIsIgnored( int actionNumber );
	bool					ItemIsIgnored( int itemNumber );
	void					Bot_IgnoreItem( int itemNumber, int time );
	bool					ClientHasChargeInWorld( int clientNum, bool armedOnly, int actionNumber, bool unArmedOnly = false );
	bool					FindChargeInWorld( int actionNumber, plantedChargeInfo_t &bombInfo, bool chargeState, bool ignoreZCheck = false );
	bool					CheckItemPackIsValid( int entNum, idVec3 &packOrg, idBounds &entBounds, int& spawnID );
	void					Bot_AddDelayedChat( int botClientNum, const botChatTypes_t chatType, int delayTimeInSecs, bool forceChat = false );
	void					Bot_CheckDelayedChats();
	bool					Client_IsCriticalForCurrentObj( int clientNum, float range );
	int						FindBodyOfClient( int clientNum, bool stealUniform, const idVec3 &org );
	float					LocationDistFromCurrentObj( const playerTeamTypes_t team, const idVec3 &org );
	bool					Bot_IsAwareOfDanger( int entNum, const idVec3 &org, const idMat3& dir );
	int						AddDangerToAwareList( int entNum, const idVec3 &org, const dangerTypes_t dangerType, int time, int ownerClientNum, const idMat3& dir = mat3_identity );
	void					Bot_CheckForDangers( bool inCombat );
	void					FindMineInWorld( plantedMineInfo_t &mineInfo );
	int						NumPlayerMines();
	bool					DangerStillExists( int entNum, int ownerClientNum );
	bool					Bot_HasExplosives( bool inCombat, bool makeSureSoldierWeaponReady = false );
	void					Bot_RunTacticalAction();
	bool					TeamHasHuman( const playerTeamTypes_t playerTeam );
	bool					NeedsReload();
	int						FindClosestDangerIndex();
	void					ClearDangerFromDangerList( int dangerIndex, bool isEntNum );
	bool					TeamHumanNearLocation( const playerTeamTypes_t playerTeam, const idVec3 &loc, float range, bool ignorePlayersInVehicle = true, const playerClassTypes_t playerClass = NOCLASS, bool ignoreDeadMates = true, bool ignoreIfJustLeftAVehicle = false );
	bool					ClientHasVehicleInWorld( int clientNum, float range );
	bool					BodyIsObstructed( int entNum, bool isCorpse, bool isSpawnHost = false );
	int						Bot_ApproxTravelTimeToLocation( const idVec3 & from, const idVec3 &to, bool inVehicle ) const;
	bool					ClientIsDead( int clientNum );
	bool					ClientCanBeTKRevived( int clientNum );
	bool					DeployableIsIgnored( int deployableNum );
	void					Bot_IgnoreDeployable( int deployableNum, int time );
	bool					GetDeployableInfo( bool selfDeployable, int entNum, deployableInfo_t& deployableInfo );
	bool					DeployableAtAction( int actionNumber, bool checkOwnDeployable );
	bool					Bot_HasWorkingDeployable( bool allowDisabledDeployables = false, int deployableType = NULL_DEPLOYABLE );
	bool					FindMCPStartAction( idVec3& origin );
	int						Bot_GetDeployableTypeForAction( int actionNumber );
	bool					Bot_CheckTeamHasDeployableTypeNearAction( const playerTeamTypes_t playerTeam, int deployableType, int actionNumber, float dist );
	bool					Bot_CheckTeamHasDeployableTypeNearLocation( const playerTeamTypes_t playerTeam, int deployableType, const idVec3& location, float dist );
	bool					PointIsClearOfTeammates( const idVec3& point );
	bool					EntityIsClient( int entNum, bool enemyOnly );
	bool					EntityIsVehicle( int entNum, bool enemyOnly, bool occupiedOnly );
	bool					EntityIsDeployable( int entNum, bool enemyOnly );
	bool					SpawnHostIsUsed( int entNum );
	bool					TeamMineInArea( const idVec3& org, float range );
	bool					Bot_CheckActionIsValid( int actionNumber );	
	const entityTypes_t		FindEntityType( int entNum, int spawnID );
	bool					VehicleIsMarkedForRepair( int entNum, bool clearRequest );
	bool					DeployableIsMarkedForRepair( int entNum, bool clearRequest );
	bool					Bot_CheckAdrenaline( const idVec3& goalOrigin );
	void					Flyer_LookAtLocation( const idVec3 &spot );
	bool					CurrentActionIsLinkedToAction( int curActionNum, int testActionNum );
	void					FindLinkedActionsForAction( int testActionNum, idList< int >& testActionList, idList< int >& linkedActionList );
	bool					Bot_CheckLocationIsOnSameSideOfSlipGate( const idVec3& origin );
	bool					Bot_CheckLocationIsVisible( const idVec3& location, int scanEntityNum = -1, int ignoreEntNum = -1, bool scanFromCrouch = false );
	bool					Bot_CheckHasUnArmedMineNearby();
	void					Bot_CheckMountedGPMGState();
	bool					Bot_CheckIfClientHasChargeAsGoal( int chargeEntNum );
	bool					Bot_IsClearOfObstacle( const idBox& box, const float bboxHeight, const idVec3& botOrigin );
	bool					OtherBotsWantVehicle( const proxyInfo_t& vehicle );
	idVec3					GetPlayerViewPosition( int clientNum );
	float					Bot_GetDeployableOffSet( int deployableType );
	int						Bot_FindNearbySafeActionToMoveToward( const idVec3& origin, float minAvoidDist, bool pickRandom = false );
	bool					ClientHasShieldInWorld( int clientNum, float considerRange );
	float					Bot_DistSqrToClosestForceShield( idVec3& shieldOrg );
	bool					Bot_UseTeleporter();
	bool					Bot_UseRepairDrone( int entNum, const idVec3& entOrg );
	int						Bot_FindClosestAVTDanger( const idVec3& location, float range, bool useAngleCheck = false );
	bool					SpawnHostIsMarkedForDeath( int spawnHostSpawnID );
	int						Bot_GetRequestedEscortClient();
	bool					Bot_CheckForHumanInteractingWithEntity( int entNum );
	int						Bot_HasTeammateWhoCouldUseShieldCoverNearby();
	bool					ClientHasCloseShieldNearby( int clientNum, float considerRange );
	const playerClassTypes_t	TeamCriticalClass( const playerTeamTypes_t playerTeam );
	int						FindHumanOnTeam( const playerTeamTypes_t playerTeam );
	void					Bot_CheckHealthCrateState();
	bool					Bot_CheckIfHealthCrateInArea( float range );
	bool					FindChargeBySpawnID( int spawnID, plantedChargeInfo_t& bombInfo );
	int						Bot_NumShieldsInWorld();
	bool					Bot_CheckChargeExistsOnObjInWorld();
	bool					GetDeployableAtAction( int actionNum, deployableInfo_t& deployable );

//mal: movement utilities
	void					Bot_MoveToGoal( const idVec3 &spot1, const idVec3 &spot2, const botMoveFlags_t moveFlag, const botMoveTypes_t moveType );
	bool					Run_OnFoot_Movement();
	bool					Bot_CanMove( const moveDirections_t direction, float gUnits, bool copyEndPos, bool endPosMustBeOutside = false ); //mal: checks "direction" out the number of "units" to see if a move is possible
	void					Bot_SetupMove( const idVec3 &org, int clientNum, int actionNumber, int areaNum = 0 );
	void					Bot_SetupVehicleMove( const idVec3 &org, int clientNum, int actionNumber, bool ignoreNodes = false );
	bool					Bot_CanProne( int clientNum );	//mal: checks to see if bot can prone in its current location, and still be able to see clientNum.
	bool					Bot_CanCrouch( int clientNum ); //mal: checks to see if bot can crouch in currect loc, and still see clientNum
	const botMoveFlags_t	Bot_ShouldStrafeJump( const idVec3 &targetOrigin );
	bool					MoveIsInvalid();
	int						Bot_CheckBlockingOtherClients( int ignoreClient, float avoidDist = 50.0f );
	void					Bot_MoveAwayFromClient( int clientNum, bool randomStrafe = false );
	bool					AddGroundObstacles( bool largePlayerBBox, bool inVehicle );
	bool					AddGroundAndAirObstacles( bool largePlayerBBox, bool inVehicle, bool inAirVehicle );
	bool					BuildObstacleList( bool largePlayerBBox, bool inVehicle );
	void					Bot_SetupQuickMove( const idVec3 &org, bool largePlayerBBox );
	void					Bot_SetupVehicleQuickMove( const idVec3 &org, bool largePlayerBBox );
	const botMoveFlags_t	Bot_FindStanceForLocation( const idVec3 &org, bool allowProne );
	void					Bot_FindRouteToCurrentGoal();
	void					Bot_FindNextRouteToGoal();
	bool					Bot_ReachedCurrentRouteNode();
	void					Bot_MoveAlongPath( botMoveFlags_t defaultMoveFlags );	// helper function used by all the LTG nodes
	const moveDirections_t	Bot_DirectionToLocation( const idVec3& location, bool precise );
	bool					LocationHasHeadRoom( const idVec3 &loc );
	bool					Bot_LocationIsReachable( bool inVehicle, const idVec3& loc, int& travelTime );
	void					Bot_SetupIcarusMove( const idVec3 &org, int clientNum, int actionNumber );
	void					Bot_SetupFlyerMove( idVec3& goalOrigin, int areaNum );
	void					FlyerHive_BuildPlayerObstacleList();
	int						TravelFlagForTeam() const;
	int						TravelFlagWalkForTeam() const;
	int						TravelFlagInvalidForTeam() const;
	bool					LocationIsReachable( bool inVehicle, const idVec3& loc1, const idVec3& loc2, int& travelTime );
	bool					Bot_CheckIfObstacleInArea( float minAvoidDist );


//mal: goal related utilities
	int						Bot_MedicCheckForWoundedMate( int healClientNum, int escortClientNum, float range, int mateHealth );
	int						Bot_MedicCheckForDeadMate( int reviveClientNum, int escortClientNum, float range );
	int						Bot_StroggCheckForGDFBodies( float range );
	bool					Bot_CheckCombatExceptions();
	int						Bot_MedicCheckForDeadMateDuringCombat( const idVec3 &dangerOrg, float range );
	bool					Bot_MedicCheckNBGState_InCombat();
	bool					Bot_MedicCheckNBGState_InLTG();
	bool					Bot_MedicCheckNBGState_InNBG();
	bool					Bot_CovertOpsCheckNBGState();
	bool					Bot_Check_NBG_Goals();
	void					Bot_FindLongTermGoal(); //mal: finds the bot a LTG in life - called on every spawn/revive/completion of previous goal.
	bool					Bot_CheckSelfState();
	void					Bot_CheckClassState();
	bool					Bot_CheckSelfSupply();
	bool					Bot_IsBusy(); //mal: lets me quickly know if bot is running important LTG/NBG and cant be buggered!
	bool					PopAINodeOffStack();
	void					Bot_ClearVehicleOffAIStack();
	void					PushAINodeOntoStack( int clientNum, int entNum, int actionNumber, int timeLimit, bool isPriority, bool useVehicle, bool useRoute = false );
	void					Bot_ClearAIStack(); //mal: clears the AI stack
	bool					Bot_CheckForTeamGoals();
	bool					Bot_CheckMapScripts();
	bool					Bot_CheckForFallBackGoals();
	bool					Bot_CheckForVehicleFallBackGoals();
	bool					ClientHasObj( int clientNum );
	int						Bot_CovertCheckForUniforms( float range );
	int						Bot_CovertCheckForVictims( float range );
	bool					Bot_FindCloseSupplyPack( bool healthOnly, bool grenadesOnly );
	bool					Bot_FindCloseSupplyTeammate( bool buggerForHealth );
	bool					Bot_FopsCheckNBGState();
	int						Bot_CheckForNeedyTeammates( float range );
	bool					Bot_CheckForSoldierGoals();
	bool					Bot_CheckForEngineerGoals();
	bool					Bot_CheckForCovertGoals();
	bool					Bot_CheckForEscortGoals();
	bool					Bot_CheckForFieldOpsGoals();
	int						Bot_CheckForTacticalAction();
	int						Bot_CheckForNeedyVehicles( float range, bool& chatRequest );
	bool					Bot_EngCheckNBGState();
	int						Bot_CheckForSpawnHostsToDestroy( float range, bool& useChat );
	bool					ObjIsOnGround();
	bool					Bot_CheckForDroppedObjGoals();
	int						Bot_CheckForNeedyDeployables( float range );
	bool					Bot_CheckForSpawnHostToGrab();
	bool					Bot_HasTeamWoundedInArea( bool deadOnly, float medicRange = MEDIC_RANGE );
	bool					Bot_CheckForHumanWantingEscort();
	int						Bot_HasDeployableTargetGoals( bool hackDeployable );
	bool					ClientsNearObj( const playerTeamTypes_t& playerTeam );
	bool					CriticalEnemyClientNearUs( const playerClassTypes_t& playerClass );
	bool					Bot_CheckNeedClientsOnDefense();
	bool					Bot_IsInvestigatingTeamObj();
	bool					Bot_VehicleLTGIsAvailable( int clientNum, int actionNumber, const bot_Vehicle_LTG_Types_t goalType, int minNumClients );
	bool					Bot_IsAttackingDeployables();
	bool					Bot_CheckMCPGoals();
	bool					Bot_IsNearDroppedObj();
	bool					Bot_WantsVehicle();
	bool					ClientIsCloseToDeliverObj( int clientNum, float desiredRange = -1.0f );
	int						Bot_CheckForGrieferTargetGoals();
	bool					ActionIsActiveForTrainingMode( int actionNumber, int numSecondsToDelay );
	bool					TeamHumanMissionIsObjective();
	int						Bot_CheckForNearbyTeammateWhoCouldUseSmoke();
	bool					Bot_CheckThereIsHeavyVehicleInUseAlready();
	bool					Bot_HasShieldInWorldNearLocation( const idVec3& checkOrg, float checkDist );
	bool					CarrierInWorld();
	bool					Bot_IsDefendingTeamCharge();

//mal: debug utilities
	void					SetupDebugHud();
	void					RunDebugChecks();

	idBotAI *				nextRemoved;

	idObstacleAvoidance		obstacles;

private:

	const clientInfo_t *		botInfo;
	const sharedWorldState_t*	botWorld;
	const proxyInfo_t *			botVehicleInfo;
	botAIOutput_t *				botUcmd;

	bool					CallFuncPtr( bool ( idBotAI::*funcPtr )() )	{ assert( funcPtr != NULL ); return ( this->*funcPtr )(); }

	//mal: AI node pointers
	bool					( idBotAI::*LTG_CHECK_FOR_CLASS_NBG )();	//mal: this is class/team specific sub goal check, while in process of long term goal
	bool					( idBotAI::*NBG_CHECK_FOR_CLASS_NBG )();	//mal: is sub goal check while in process of short term goals
	bool					( idBotAI::*COMBAT_MOVEMENT_STATE )();	//mal: the current move state of the bot (on foot, in vehicle (and type), etc.
	bool					( idBotAI::*ROOT_AI_NODE )();
	bool					( idBotAI::*V_ROOT_AI_NODE )();
	bool					( idBotAI::*NBG_AI_SUB_NODE )();
	bool					( idBotAI::*LTG_AI_SUB_NODE )();
	bool					( idBotAI::*V_LTG_AI_SUB_NODE )();
	bool					( idBotAI::*COMBAT_AI_SUB_NODE )();
	bool					( idBotAI::*VEHICLE_COMBAT_AI_SUB_NODE )();
	bool					( idBotAI::*VEHICLE_COMBAT_MOVEMENT_STATE )();	//mal: the current move state of the bot (on foot, in vehicle (and type), etc.

    struct botAAS_t {
		bool				hasPath;
		bool				hasClearPath;
		bool				hasReachedVehicleNodeGoal;
		bool				triedToMoveThisFrame;
		int					obstacleNum;
		int					aasType;
		int					blockedByObstacleCounterOnFoot;
		int					blockedByObstacleCounterInVehicle;
		idAASPath			path;
		idAAS *				aas;						// pointer to the AAS used by this AI
	};

	botAAS_t				botAAS;

	idList< idBotNode::botLink_t >	botVehiclePathList;

	botChats_t				delayedChats[ MAX_DELAYED_CHATS ];

	int						botNum; //mal: lets me track which client slot this bot has.

	int						botExitTime;

	genericNumTimeList_t	ignoreClients[ MAX_IGNORE_ENTITIES ];
	genericNumTimeList_t	ignoreVehicles[ MAX_IGNORE_ENTITIES ];
	genericNumTimeList_t	ignoreDeployables[ MAX_IGNORE_ENTITIES ];
	genericNumTimeList_t	ignoreEnemies[ MAX_IGNORE_ENTITIES ];
	genericNumTimeList_t	ignoreBodies[ MAX_IGNORE_ENTITIES ];
	genericNumTimeList_t	ignoreSpawnHosts[ MAX_IGNORE_ENTITIES ];
	genericNumTimeList_t	ignoreActions[ MAX_IGNORE_ENTITIES ];
	genericNumTimeList_t	ignoreItems[ MAX_IGNORE_ENTITIES ];

	dangerList_t			currentDangers[ MAX_DANGERS ];
	dangerList_t			currentArtyDanger;	//mal: only track one at a time.
	int						actionNumInsideDanger;
	int						ignoreDangersTime;

	bot_AI_States_t			aiState; //mal: a var to track the bots current overall AI State

	idVec3					botCanMoveGoal; //mal: a test location, set by Bot_CanMove - use if desired.

	idVec3					botBackStabMoveGoal; //mal: a test location, set by Bot_CanMove - use if desired.

	int						actionNum; //mal: the action that is our current goal
	int						lastActionNum; //mal: the last action we were doing
	int						lastCheckActionTime; //mal: last time we checked an action for some event.
	int						tacticalActionNum;
	int						tacticalActionTime;
	int						tacticalActionTimer;
	int						tacticalActionTimer2;
	idVec3					tacticalActionOrigin;
	int						tacticalDeviceCharge;
	int						tacticalActionIgnoreTime;
	int						tacticalActionPauseTime;

// Long Term Goal Stuff
	bot_LTG_Types_t			ltgType; //mal: what kind of LTG....
	int						ltgTarget; //mal: the entity that is our LTG goal, if exists!
	int						ltgTargetSpawnID;
	int						ltgTime; //mal: how long we should persue this goal.
	bool					ltgReached; //mal: tracks goal progress
	bool					ltgReachedTarget;
	bool					ltgUseVehicle;
	bool					ltgChat;
	bool					ltgExit;
	int						ltgTimer;
	targetTypes_t			ltgTargetType; //mal: the type of target, for special cases.
	float					ltgDist;
	int						ltgPauseTime;
	botMoveFlags_t			ltgPosture;	//mal: some actions can have a random amount of postures, track which the bot picked here.
	idVec3					ltgOrigin;
	moveDirections_t		ltgMoveDir;
	int						ltgCounter;
	int						ltgMoveTime;
	int						ltgTryMoveCounter;
	bool					ltgUseShield;
	bool					ltgUseSmoke;
	bool					ltgUseMine;

// Vehicle Long Term Goal Stuff
	bot_Vehicle_LTG_Types_t	vLTGType; //mal: what kind of vehicle LTG....
	int						vLTGTarget; //mal: the entity that is our vehicle LTG goal, if exists!
	int						vLTGTargetSpawnID;
	int						vLTGTime; //mal: how long we should persue this goal.
	bool					vLTGReached; //mal: tracks goal progress
	bool					vLTGExit;
	int						vLTGTimer;
	int						vLTGMoveTimer;
	int						vLTGVisTimer;
	int						vLTGTimer2;
	targetTypes_t			vLTGTargetType; //mal: the type of target, for special cases.
	float					vLTGDist;
	float					vLTGMoveTooCloseRange;
	idVec3					vLTGOrigin;
	moveDirections_t		vLTGMoveDir;
	int						vLTGMoveTime;
	int						vLTGSightTime;
	int						vLTGDriveTime;
	int						vLTGPauseTime;
	int						vLTGMoveActionGoal;
	bool					vLTGChat;
	float					vLTGDistFoundDeployable;
	bool					vLTGUseAltAttackPointOnDeployable;
	int						vLTGAttackDeployableCounter;
	int						vLTGKeepMovingTime;
	int						vLTGDeployableTarget;
	int						vLTGDeployableTargetAttackTime;

// Short Term Goal Stuff
	bot_NBG_Types_t			nbgType; //mal: ... and NBG we have currently
	int						nbgTarget; //mal: the entity that is our NBG goal, if exists!
	int						nbgTargetSpawnID;
	int						nbgTime; //mal: how long we should do this goal.
	bool					nbgReached; //mal: tracks goal progress
	bool					nbgReachedTarget;
	bool					nbgExit;
	bool					nbgSwitch;
	int						nbgClientNum;
	int						nbgTimer; //mal: can be used to time certain things
	int						nbgTimer2;
	int						nbgMoveTimer;
	int						nbgTryMoveCounter;
	int						nbgMoveTime;
	targetTypes_t			nbgTargetType; //mal: what type of target this is, for special cases.
	float					nbgDist;
	idVec3					nbgOrigin;
	botMoveFlags_t			nbgPosture;	//mal: some actions can have a random amount of postures, track which the bot picked here.
	botMoveTypes_t			nbgMoveType;
	moveDirections_t		nbgMoveDir;
	bool					nbgChat;



// Combat Goal Stuff
	int						combatNBGTarget; //mal: the entity that is our NBG goal, if exists!
	int						combatNBGTime; //mal: how long we should do this goal.
	int						combatTryMoveCounter;
	int						combatTryMoveTime;
	bool					combatNBGReached; //mal: tracks goal progress
	bool					combatDangerExists;
	bot_COMBAT_Types_t		combatNBGType;
	botMoveFlags_t			combatMoveFlag;

	int						botIsBlocked;
	int						vehicleWheelsAreOffGroundCounter;
	int						botIsBlockedTime;

//mal: enemy stuff
	idVec3						bot_FS_Enemy_Pos;	// FS = First Seen - where the bot was when he first saw the enemy
	idVec3						bot_LS_Enemy_Pos;	// LS = Last Seen - where the bot was when he last saw the enemy
	int							numVisEnemies;		//mal: how many visible enemies FindEnemy found this frame - useful for tactical thinking
	int							enemy;				//mal: the entity that is the bot's current enemy.
	int							enemySpawnID;
	int							enemyAcquireTime;
	int							heardClient;		//mal: a client we're aware of because we heard them, but we haven't seen yet.

	moveDirections_t			combatMoveDir;
	botAttackMoveTypes_t		combatMoveType;
	int							combatMoveTime;			//mal: how long to move this direction
	int							combatMoveFailedCount;	//mal: how many times the bot has tried a move that failed.
	int							combatMoveActionGoal;
	float						combatMoveTooCloseRange;
	int							combatKeepMovingTime;


	int							gunTargetEntNum;			// the ent, if exists, thats in front of us ATM.

	int							shotIsBlockedCounter;

	bool						hammerVehicle;
	bool						hammerTime;
	idVec3						hammerLocation;
	int							hammerClient;
	bool						resetEnemy;

	bool						turretDangerExists;
	int							turretDangerEntNum;

	int							classAbilityDelay;	// how long we should wait before use class ability
	
	struct enemyInfo_t {
		bool					enemyVisible;
		bool					enemyFacingBot;
		bool					enemyInfont;
		int						enemyLastVisTime;	// when this enemy was last visible.
		int						enemyHeight;		// how far above/below the bot this enemy is.
		float					enemyDist;
		idVec3					enemy_FS_Pos;		// FS = First Seen - where the enemy was when the bot first saw him
		idVec3					enemy_LS_Pos;		// LS = Last Seen - where the enemy was when the bot last saw him
		idVec3					enemy_NS_Pos;		// NS = Not Seen - one location bot could use to chase enemies in complicated map areas
	};

	enemyInfo_t				enemyInfo;

	bool					ignoreWeapChange;
	bool					lefty;
	int						botTeleporterAttempts;

	idBotRoutes*			routeNode;
	idBotNode*				vehicleNode;

	playerWeaponTypes_t		botIdealWeapNum;		// the specific weapon the bot wants to use.
	weaponBankTypes_t		botIdealWeapSlot;		//mal: tracks which weapon bank the bot wants to switch to.

	int						lastGrenadeTime;
	bool					safeGrenade;
	bool					chaseEnemy;			// should the bot really chase this enemy around - otherwise will give up once reach place enemy last seen.
	int						timeTilAttackEnemy; //mal: allows me to set a delay in how long from when a bot sees and enemy, til he attacks.
	bool					useNadeOnEnemy;	// does this bot want to use a nade on its enemy?
	bool					useAircanOnEnemy;
	int						vehicleGunnerTime;
	int						vehicleDriverTime;
	int						vehicleUpdateTime;
	int						rocketTime;

	int						crateGoodTime;

	int						ignoreNewEnemiesTime;
	int						ignoreNewEnemiesWhileInVehicleTime;

	int						timeOnTarget;	// the time the bot should wait untill it attacks the target.

	int						weapSwitchTime;

	int						reachedPatrolPointTime;

	int						rocketLauncherLockFailedTime;

	int						lastVehicleSpawnID;
	int						lastVehicleTime;
	int						lastNearbySafeActionToMoveTowardActionNum;

	struct bot_AI_Stack_t {
		bool						( idBotAI::*STACK_AI_NODE )(); //mal: node to run when get back to stack
		bool						( idBotAI::*VEHICLE_STACK_AI_NODE )(); //mal: vehicle node to run when get back to stack
		bool						isPriority;					 //mal: if true - we should do a combat evade, if its combat that makes us stack our goal.
		bool						useVehicle;					 //mal: if true - bot will find a vehicle to reach this goal.
		int							stackClientNum;				 //mal: client to target, if exists
		int							stackClientSpawnID;			 //mal: make sure we grab the right client, incase he disconnected, etc.
		int							stackEntNum;				 //mal: entity to target, if exists
		int							stackTimeLimit;				 //mal: time to do target, if exists
		int							stackActionNum;				 //mal: the action the bot was doing, if exists.
		idBotRoutes*				routeNode;
		bot_Vehicle_LTG_Types_t		vehicleStackLTGType;
		bot_LTG_Types_t				stackLTGType;
	};

	int						framesStuck;
	int						noAASCounter;
	int						overallFramesStuck;
	int						vehicleFramesStuck;
	bool					enemyIsHuntGoal;

	struct vehicleAINodeSwitch_t {
		int						nodeSwitchCount;
		int						nodeSwitchTime;
	};

	vehicleAINodeSwitch_t	vehicleAINodeSwitch;

	struct moveErrorCounter_t {
		int						moveErrorCount;
		int						moveErrorTime;
	};

	moveErrorCounter_t		moveErrorCounter;

	int						ignoreSpyTime;

	int						framesVehicleStuck;

	int						blockingTeammateCounter;

	int						chaseEnemyTime;

	int						fdaUpdateTime;

	int						framesFlipped;

	int						bugForSuppliesDelay;

	int						ignorePlantedChargeTime;

	bool					vehicleEnemyWasInheritedFromFootCombat;

	bool					ignoreMCPRouteAction;

	bool					stayInPosition;

	bool					weaponLocked;

	bool					chasingEnemy;

	bool					hasGreetedServer;

	float					randomLookRange;

	int						botWantsVehicleBackTime;

	int						checkRoadKillTime;

	int						vehicleObstacleSpawnID;
	int						vehicleObstacleTime;

	bool					isStrafeJumping;
	bool					isBoosting;

	int						vehiclePauseTime;
	int						vehicleReverseTime;
	int						nextObjChargeCheckTime;

	int						vehicleSurrenderTime;
	bool					vehicleSurrenderChatSent;
	int						vehicleSurrenderClient;

	int						vehicleCheckForPossibleHumanPassengerChatTime;

	int						nextMissionUpdateTime;

	int						botPathFailedCounter;

	bool					fastAwareness;
	int						skipStrafeJumpTime; //mal: how long we should ignore the strafe jump check

	int						badMoveTime;

	bool					testFireShot;

	int						ignoreNadeTime;	//mal: sometimes the bot won't use his grenade in combat, just to mix it up a bit.

	int						supplySelfTime;		//mal: prevents the bots from trying to supply self, and stop doing it because it has no charge.

	int						randomLookDelay;	//mal: a minimum time to ignore randomLook calls, so that the bots dont swivel around like maniacs.

	int						pistolTime;	//mal: how long the bot should have the pistol out.

	idVec3					randomLookOrg; //mal: keep track of where we've been looking, in case we need to look somewhere else...

	idVec3					hopMoveGoal;
	int						stuckRandomMoveTime;

	bool					wantsVehicle;

	int						ignoreIcarusTime;

	int						selfShockTime;

	int						nodeTimeOut;
	int						newPathTime;

	int						strafeToAvoidDangerTime;
	moveDirections_t		strafeToAvoidDangerDir;

	int						warmupActionMoveGoal;
	int						warmupUseActionMoveGoalTime;

	struct ignoreNodeTimer_t {
		idBotNode*				ignorePathNode;
		int						ignorePathNodeTime;
		int						ignorePathNodeUpdateTime;
	};

	ignoreNodeTimer_t		pathNodeTimer;

	bot_AI_Stack_t			AIStack;	//mal: only important AI nodes will be put on the stack, so that the bots can resume them after doing a NBG/COMBAT
    
	char					*lastAINode;
	char					*lastMoveNode;

#ifdef _DEBUG
	int						debugVar1;
	int						debugVar2;
#endif
};

idEntity *GetGameEntity( int entityNumber );

#endif /* !__BOTAI_H__ */
