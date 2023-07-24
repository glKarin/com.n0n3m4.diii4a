// Copyright (C) 2007 Id Software, Inc.
//

#ifndef __BOT_COMMON_H__
#define __BOT_COMMON_H__

enum playerWeaponTypes_t {	//mal: keep adding more as we support them!
    NULL_WEAP = -1,
	KNIFE,					 // knife and spikes
	PISTOL,					 // blaster/pistol
	SMG,					 // assault rifle and lacerator
	GRENADE,				 // shrap grenade of either team
	EMP,					 // emp grenade of either team
	ROCKET,					 // rocket launcher
	HEAVY_MG,				 //	heavy mg/ hyper blaster
	HEALTH,					 // health packs
	NEEDLE,					 // defib/stroy tool
	BINOCS,					 // normal binoculars - NOT deploy tools!
	AIRCAN,					 // violator/airstrike marker
	SHOTGUN,				 // shotgun, nailgun
	PLIERS,					 // tool for fixing stuff
	SCOPED_SMG,				 // covert's SMG
	NADE_SMG,				 // eng's SMG
	SNIPERRIFLE,			 // sniper rifle/railgun
	HE_CHARGE,				 // he and plasma charge
	LANDMINE,				 // landmine
	HACK_TOOL,				 // hack tool for covertops
	AMMO_PACK,				 // just what it says.
	SMOKE_NADE,				 // GDF smoke grenades
	DEPLOY_TOOL,			 // Eng/FOps/Covert deploy tool
	SHIELD_GUN,				 // Strogg Force Shield weapon
	TELEPORTER,				 // Strogg Covert teleport weapon
	FLYER_HIVE,				 // Strogg Covert Flyer Hive
	SUPPLY_MARKER,			 // GDF Supply Marker
	THIRD_EYE,				 // GDF Covert 3rd Eye camera.
	PARACHUTE				 // GDF and STROGG have this when spawning in.
};

enum playerVehicleTypes_t {	//mal: keep adding more as we support them!
    NULL_VEHICLE = -1,
	HUSKY,
	BADGER,
	TITAN,
	HOG,
	GOLIATH,
	DESECRATOR,
	MCP,
//mal: the 2 below are the only water vehicles
	PLATYPUS,
	TROJAN,
//mal: from here on out, is nothing but flying vehicles. We check in the code if the vehicle type is >= to icarus to find out if vehicle flys.
	ICARUS,
	ANANSI,
	HORNET,
	BUFFALO
};

#define TURRET_ANGLE_RANGE	135

#define NULL_DEPLOYABLE					0
#define ARTILLERY						1
#define ROCKET_ARTILLERY				2
#define NUKE							4 //mal: the BIG guns like the Hammer and DMC
#define APT								8
#define AVT								16
#define RADAR							32
#define AIT								64

//mal: vehicles can have multiple flags ( ex: icarus can be a personal vehicle, as well as an air one, trojan can be a big ground, and a water ).
#define NO_VEHICLE						-1
#define NULL_VEHICLE_FLAGS				0
#define PERSONAL						1
#define GROUND							2
#define ARMOR							4
#define AIR								8
#define WATER							16
#define AIR_TRANSPORT					32 //buffalo only atm.
#define GROUND_TRANSPORT				64 //trojan only atm.
#define ALL_VEHICLES_BUT_ICARUS			126

#define MAX_VEHICLE_RANGE		2500.0f
#define HUNT_MAX_VEHICLE_RANGE	3000.0f

#define BOT_SKILL_EASY		0
#define BOT_SKILL_NORMAL	1
#define BOT_SKILL_EXPERT	2
#define BOT_SKILL_DEMO		3


#define LOW_SKILL_BOT_HEALTH	60


#define MISSION_NULL		-1
#define MISSION_OBJ			-2

enum playerTeamTypes_t {
    NOTEAM = -1,			 // spectator
    GDF,
	STROGG
};

enum moveDirections_t {
	NULL_DIR = -1,
	FORWARD,
	BACK,
	RIGHT,
	LEFT
};

enum playerClassTypes_t { //mal: we dont care about the strogg, since the classes are pretty much a 1 to 1 match
    NOCLASS = -1,
	MEDIC,
	SOLDIER,
	ENGINEER, 
	FIELDOPS,
	COVERTOPS,
	MAX_BOT_TEAMS
};

enum botGoalTypes_t {
	NULL_GOAL_TYPE = -1,
	REVIVE_SOMEONE,						//0
	HEAL_SOMEONE,						//1
	PLANT_MINES,						//2
	GRAB_FORWARD_SPAWN,					//3
	DO_OBJECTIVE,						//4
	REPAIR_VEHICLE,						//5
	REPAIR_DEPLOYABLE,					//6
	DESTROY_VEHICLE,					//7
	GO_DESTROY_DEPLOYABLE,				//8
	GO_DEPLOY_DEPLOYABLE,				//9
	KILL_PLAYER,						//10
	DEFEND_AREA,						//11
	GO_HACK_DEPLOYABLE,					//12
	CONTRUCT_TOWER,						//13
	SUPPLY_AMMO,						//14
	SEND_FIRESUPPORT,					//15
	REPAIR_MCP_GOAL,					//16
	CAPTURE_FORWARD_SPAWN,				//17
	LIBERATE_FORWARD_SPAWN,				//18
	LIBERATE_FORWARD_SPAWN_COVERTOPS,	//19
	RETURN_CARRYABLE,					//20
	GRAB_CARRYABLE,						//21
	DELIVER_CARRYABLE,					//22
	SPAWNHOST_BODY,						//23
	DESTROY_STROGG_SPAWNHOST,			//24
	CONSTRUCT_MG_NEST,					//25
	GO_DEPLOY_RADAR,					//26
	GO_HACK_SHIELD,						//27
	GO_DEPLOY_FIRESUPPORT,				//28
	ATTACK_MCP							//29
};

enum botMoveFlags_t {
    NULLMOVEFLAG = -1,
	PRONE,
	CROUCH,
	WALK,
	RANDOM_STANCE, //mal: with a random stance, could decide to randomly stand or crouch.
	RUN,
	REVERSE,		//mal: move backwards to this location - used for vehicles.
	SPRINT,
	SPRINT_ATTACK,	//mal: this is a vehicle based sprint - where we use it to attack, instead of just to get somewhere fast ( less safety precautions ).
	STRAFEJUMP,
	JUMP_MOVE		//mal: only used by the flyer hives for now
};

enum dangerTypes_t { //mal: we also use this to track entity types in the botThread
	NULL_DANGER,
	HAND_GRENADE,
	PLANTED_CHARGE,
	THROWN_AIRSTRIKE,
	PLANTED_LANDMINE,
	INGAME_PLAYER,
	INGAME_VEHICLE,
	THIRD_EYE_CAM,
	STROGG_HIVE,
	ANTI_PERSONAL,
	ANTI_VEHICLE,
	ARTY_DANGER,
	STROY_BOMB_DANGER
};

enum botPostureFlags_t {
	IS_PRONE,
	IS_CROUCHED,
	IS_STANDING
};

enum botChatTypes_t { 
//NOTE: for many of these, there is more then 1 chat that works, so we'll randomly pick one for variety.	
	NULL_CHAT,
	HEAL_ME,
	REVIVE_ME,
	COVER_ME,
	FOLLOW_ME,
	ENEMY_DISGUISED,
	NEED_BACKUP,
	YES,
	NO,
	HOLDFIRE,
	ONOFFENSE,
	ONDEFENSE,
	THANKS,
	MY_CLASS,
	YOURWELCOME,
	IMAMEDIC,
	MOVE,
	REARM_ME,
	KILLED_TAUNT,
	WARMUP_TAUNT,
	GENERAL_TAUNT,
	SUPPLIES_DROPPED,
	MEDPACK_DROPPED,
	AMMO_ACK,
	IM_DISGUISED,
	ENEMY_DISGUISED_AS_ME,
	REPLY_TAUNT,
	ACKNOWLEDGE_YES,
	ACKNOWLEDGE_NO,
	MEDIC_ACK,
	LETS_GO,
	GENERIC_PLANT,
	GENERIC_BUILD,
	GENERIC_HACK,
	INCOMING_INFANTRY,
	INCOMING_VEHICLE,
	INCOMING_AIRCRAFT,
	MINES_SPOTTED,
	HOLD_VEHICLE,
	NEED_LIFT,
	HELLO,
	ENDGAME_WIN,
	ENDGAME_LOSE,
	SORRY,
	NEED_ESCORT,
	CMD_DECLINED,
	NEED_REPAIR,
	TAKE_SPAWN,
	CONTEXT_NEED_RIDE,
	CONTEXT_KILL_TARGET,
	CONTEXT_REPAIR_TARGET,
	CONTEXT_DO_OBJECTIVE,
	TK_REVIVE_CHAT,
	WILL_FIX_RIDE,
	STOP_WILL_FIX_RIDE,
	MOVE_OUT,
	GOT_YOUR_BACK,
	GLOBAL_SORRY
};

#endif /* !__BOT_COMMON_H__ */
