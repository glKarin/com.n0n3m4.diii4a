////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
// Title: JA Config
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __JA_CONFIG_H__
#define __JA_CONFIG_H__

#include "Omni-Bot_Events.h"

typedef enum eJA_Version
{
	JA_VERSION_0_7 = 1,
	JA_VERSION_0_71,
	JA_VERSION_0_8,
	JA_VERSION_LAST,
	JA_VERSION_LATEST = JA_VERSION_LAST - 1
} JA_Version;

typedef enum eJA_Events
{
	JA_MESSAGE_BEGIN = EVENT_NUM_EVENTS,
	JA_PERCEPT_FEEL_FORCE,
	JA_MESSAGE_END
} JA_Event;

typedef enum eJA_Msgs
{
	JA_MSG_START = GEN_MSG_END,
	
	// misc query stuff
	//JA_MSG_REINFORCETIME,
	
	// goal query stuff
	JA_MSG_GHASFLAG,
	JA_MSG_FORCEDATA,
	JA_MSG_GCANBEGRABBED,
	JA_MSG_GNUMTEAMMINES,		// check the number of team mines
	JA_MSG_GNUMTEAMDETPACKS,	// check the number of team detpacks
	JA_MSG_MINDTRICKED,

	JA_MSG_END
} JA_Msg;

typedef enum eJA_PlayerClass
{
	JA_CLASS_UNKNOWN = 0,
	JA_CLASS_NULL = 0,
	JA_CLASS_PLAYER,
	JA_CLASS_ASSAULT,
	JA_CLASS_SCOUT,
	JA_CLASS_TECH,
	JA_CLASS_JEDI,
	JA_CLASS_DEMO,
	JA_CLASS_HW,
	JA_CLASS_MAX,
	JA_CLASS_ANY = JA_CLASS_MAX,

	// Other values to identify the "class"
	JA_CLASSEX_ROCKET,
	JA_CLASSEX_MINE,
	JA_CLASSEX_DETPACK,
	JA_CLASSEX_THERMAL,
	JA_CLASSEX_CONCUSSION,
	JA_CLASSEX_BOWCASTER,
	JA_CLASSEX_BOWCASTER_ALT,
	JA_CLASSEX_REPEATER,
	JA_CLASSEX_REPEATER_BLOB,
	JA_CLASSEX_FLECHETTE,
	JA_CLASSEX_FLECHETTE_ALT,
	JA_CLASSEX_BLASTER,
	JA_CLASSEX_PISTOL,
	JA_CLASSEX_DEMP2,
	JA_CLASSEX_DEMP2_ALT,
	JA_CLASSEX_TURRET_MISSILE,
	JA_CLASSEX_VEHMISSILE,
	JA_CLASSEX_LIGHTSABER,
	JA_CLASSEX_NPC,
	JA_CLASSEX_VEHICLE,
	JA_CLASSEX_AUTOTURRET,
	JA_CLASSEX_BREAKABLE,
	JA_CLASSEX_FORCEFIELD,
	JA_CLASSEX_SIEGEITEM,
	JA_CLASSEX_CORPSE,
	JA_CLASSEX_WEAPON,
	JA_CLASSEX_HOLDABLE,
	JA_CLASSEX_FLAGITEM,
	JA_CLASSEX_POWERUP,
	JA_CLASSEX_HOLOCRON,

	JA_NUM_CLASSES
} JA_PlayerClass;

// enumerations: JA_AmmoType
//		JA_WP_STUN_BATON - Stun Baton.
//		JA_WP_MELEE - Melee.
//		JA_WP_SABER - Lightsaber.
//		JA_WP_BRYAR_PISTOL - DL-44 Heavy Blaster Pistol.
//		JA_WP_BLASTER - E11-Blaster Rifle.
//		JA_WP_DISRUPTOR - Tenloss DXR-6 Disruptor rifle.
//		JA_WP_BOWCASTER - Wookie Bowcaster.
//		JA_WP_REPEATER - Imperial Heavy Repeater.
//		JA_WP_DEMP2 - Destructive Electro-Magnetic Pulse 2 Gun.
//		JA_WP_FLECHETTE - Golan Arms FC1 Flechette Weapon.
//		JA_WP_ROCKET_LAUNCHER - Merr-Sonn PLX-2M Portable Missile System.
//		JA_WP_THERMAL - Thermal Detonators.
//		JA_WP_TRIP_MINE - Trip Mines.
//		JA_WP_DET_PACK - Detonation Packs.
//		JA_WP_CONCUSSION - Stouker Concussion Rifle.
//		JA_WP_BRYAR_OLD - Bryar Blaster Pistol.
typedef enum eJA_Weapons
{
	JA_WP_UNKNOWN = INVALID_WEAPON,
	JA_WP_NONE = INVALID_WEAPON,
	JA_WP_STUN_BATON,
	JA_WP_MELEE,
	JA_WP_SABER,
	JA_WP_BRYAR_PISTOL,
	JA_WP_BLASTER,
	JA_WP_DISRUPTOR,
	JA_WP_BOWCASTER,
	JA_WP_REPEATER,
	JA_WP_DEMP2,
	JA_WP_FLECHETTE,
	JA_WP_ROCKET_LAUNCHER,
	JA_WP_THERMAL,
	JA_WP_TRIP_MINE,
	JA_WP_DET_PACK,
	JA_WP_CONCUSSION,
	JA_WP_BRYAR_OLD,
	JA_WP_EMPLACED_GUN,
	JA_WP_TURRET,
	JA_WP_MAX
} JA_Weapon;

// enumerations: JA_ForcePower
//		JA_FP_HEAL - Self Heal Force Power.
//		JA_FP_LEVITATION - Force Jump.
//		JA_FP_SPEED - Force Speed.
//		JA_FP_PUSH - Force Push.
//		JA_FP_PULL - Force Pull.
//		JA_FP_TELEPATHY - Mind Trick Force Power.
//		JA_FP_GRIP - Force Grip.
//		JA_FP_LIGHTNING - Force Lightning.
//		JA_FP_RAGE - Force Rage.
//		JA_FP_PROTECT - Force Protect.
//		JA_FP_ABSORB - Force Absorb.
//		JA_FP_TEAM_HEAL - Team Heal Force Power.
//		JA_FP_TEAM_ENGERGIZE - Team Energize Force Power.
//		JA_FP_DRAIN - Force Drain.
//		JA_FP_SEE - Force Sense.
//		JA_FP_SABER_OFFENSE - Saber Offense Knowledge Level.
//		JA_FP_SABER_DEFENSE - Saber Defense Knowledge Level.
//		JA_FP_SABERTHROW - Saber Throw Knowledge Level..
typedef enum eJA_ForcePowers
{
	JA_FP_HEAL = 0,
	JA_FP_LEVITATION,
	JA_FP_SPEED,
	JA_FP_PUSH,
	JA_FP_PULL,
	JA_FP_TELEPATHY,
	JA_FP_GRIP,
	JA_FP_LIGHTNING,
	JA_FP_RAGE,
	JA_FP_PROTECT,
	JA_FP_ABSORB,
	JA_FP_TEAM_HEAL,
	JA_FP_TEAM_FORCE,
	JA_FP_DRAIN,
	JA_FP_SEE,
	JA_FP_SABER_OFFENSE,
	JA_FP_SABER_DEFENSE,
	JA_FP_SABERTHROW,
	JA_MAX_FORCE_POWERS
} JA_ForcePower;

// enumerations: JA_Team
//		JA_TEAM_RED - Red Team.
//		JA_TEAM_BLUE - Blue Team.
//		JA_TEAM_FREE - Free For All.
typedef enum eJA_Team
{
	JA_TEAM_NONE = OB_TEAM_NONE,
	JA_TEAM_RED,
	JA_TEAM_BLUE,
	JA_TEAM_FREE,
	JA_TEAM_MAX
} JA_Team;

// enumerations: JA_Powerups
//		JA_ENT_FLAG_NPC - This entity is a non-vehicle NPC.
//		JA_ENT_FLAG_VEHICLE - This entity is a vehicle NPC that is not piloted.
//		JA_ENT_FLAG_VEHICLE_PILOTED - This entity is a vehicle NPC with a pilot.
//		JA_ENT_FLAG_JETPACK - This entity has the jetpack item activated.
//		JA_ENT_FLAG_CLOAKED - This entity is currently cloaked with the cloak item.
//		JA_ENT_FLAG_CARRYINGFLAG - This entity is currently carrying a CTF flag or SIEGE item.
//		JA_ENT_FLAG_SIEGEDEAD - This entity is "dead" in siege.
//		JA_ENT_FLAG_YSALAMIRI - This entity has a ysalamiri (not able to use force or have force acted upon).
//		JA_ENT_FLAG_MUTANT - This entity is the mutant.
//		JA_ENT_FLAG_BOTTOMFEEDER - This entity is the bottom feeder.
//		JA_ENT_FLAG_SHIELDED - This entity is not affected by small weapons fire and the projectiles bounce off.
typedef enum eJA_EntityFlags
{
	JA_ENT_FLAG_NPC = ENT_FLAG_FIRST_USER,
	JA_ENT_FLAG_VEHICLE,
	JA_ENT_FLAG_VEHICLE_PILOTED,
	JA_ENT_FLAG_JETPACK,
	JA_ENT_FLAG_CLOAKED,
	JA_ENT_FLAG_CARRYINGFLAG,
	JA_ENT_FLAG_SIEGEDEAD,
	JA_ENT_FLAG_YSALAMIRI,
	JA_ENT_FLAG_MUTANT,
	JA_ENT_FLAG_BOTTOMFEEDER,
	JA_ENT_FLAG_SHIELDED
} JA_EntityFlags;

// enumerations: JA_Powerups
//		JA_PWR_REDFLAG - This entity is carrying the red flag.
//		JA_PWR_BLUEFLAG - This entity is carrying the blue flag.
//		JA_PWR_ONEFLAG - This entity is carrying the one flag. (unused for now)
//		JA_PWR_FORCE_ENLIGHTENED_LIGHT - This entity has force enlightenment
//		JA_PWR_FORCE_ENLIGHTENED_DARK - This entity has force endarkenment.
//		JA_PWR_FORCE_BOON - This entity has force boon.
//		JA_PWR_YSALAMIRI - This entity has the Ysalamiri.
//						A small lizard carried on the player, which prevents the possessor from using any Force power.
//						However, (s)he is unaffected by any Force power.
//		JA_PWR_SIEGEITEM - This entity has a siege item held
typedef enum eJA_Powerups
{
	JA_PWR_REDFLAG = PWR_FIRST_USER,
	JA_PWR_BLUEFLAG,
	JA_PWR_ONEFLAG,
	JA_PWR_FORCE_ENLIGHTENED_LIGHT,
	JA_PWR_FORCE_ENLIGHTENED_DARK,
	JA_PWR_FORCE_BOON,
	JA_PWR_YSALAMIRI,
	JA_PWR_SIEGEITEM
} JA_Powerups;

// enumerations: JA_Contents
//		CONT_LIGHTSABER - Lightsaber blade entity.
typedef enum eJA_Contents
{	
	CONT_LIGHTSABER	= CONT_START_USER
} JA_Contents;

// enumerations: JA_SurfaceFlags
//		SURFACE_FORCEFIELD - A forcefield is here.
typedef enum eJA_SurfaceFlags
{
	SURFACE_FORCEFIELD = SURFACE_START_USER
} JA_SurfaceFlags;

// enumerations: JA_ButtonFlags
typedef enum eJA_ButtonFlags
{	
	BOT_BUTTON_FORCEPOWER = BOT_BUTTON_FIRSTUSER,
	BOT_BUTTON_FORCEGRIP,
	BOT_BUTTON_FORCELIGHTNING,
	BOT_BUTTON_FORCEDRAIN
} JA_ButtonFlags;

#endif
