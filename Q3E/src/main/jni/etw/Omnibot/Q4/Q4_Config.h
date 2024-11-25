////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
// about: Q4 game definitions
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __Q4_EVENTS_H__
#define __Q4_EVENTS_H__

#include "Omni-Bot_Types.h"
#include "Omni-Bot_Events.h"

typedef enum eQ4_Version
{
	Q4_VERSION_0_53 = 1,
	Q4_VERSION_0_6,
	Q4_VERSION_0_65,
	Q4_VERSION_0_7_alpha1,
	Q4_VERSION_0_7_alpha2,
	Q4_VERSION_0_7,
	Q4_VERSION_0_8,

	Q4_VERSION_LATEST
} Q4_Version;

// typedef: Q4_Events
//		Defines the events specific to the game, numbered starting at the end of
//		the global events.
typedef enum eQ4_Events
{
	Q4_EVENT_BEGIN = EVENT_NUM_EVENTS,	
	Q4_EVENT_END
} Q4_Events;

// typedef: Q4_GameEvents
//		Events that allow the bot to query for information from the game.
typedef enum eQ4_GameMessage
{
	Q4_MSG_START = GEN_MSG_END,
	Q4_MSG_GETLOCATION,
	Q4_MSG_GETPLAYERCASH,
	Q4_MSG_ISBUYINGALLOWED,
	Q4_MSG_BUYSOMETHING,
	Q4_MSG_END
} Q4_GameMessage;

// enumerations: Q4_Weapon
//		Q4_WP_BLASTER - Blaster.
//		Q4_WP_MACHINEGUN - Machine gun.
//		Q4_WP_SHOTGUN - Shotgun
//		Q4_WP_NAILGUN - Nailgun
//		Q4_WP_HYPERBLASTER - Hyperblaster
//		Q4_WP_ROCKETLAUNCHER - Rocket Launcher
//		Q4_WP_LIGHTNINGGUN - Lightning Gun
//		Q4_WP_GRENADELAUNCHER - Grenade Launcher
//		Q4_WP_GAUNTLET - Gauntlet
//		Q4_WP_DMG - Dark Matter Gun
//		Q4_WP_RAILGUN - Railgun
typedef enum eQ4_Weapon
{
	Q4_WP_NONE = INVALID_WEAPON,
	Q4_WP_BLASTER,
	Q4_WP_MACHINEGUN,
	Q4_WP_SHOTGUN,
	Q4_WP_NAILGUN,
	Q4_WP_HYPERBLASTER,
	Q4_WP_ROCKETLAUNCHER,
	Q4_WP_LIGHTNINGGUN,
	Q4_WP_GRENADELAUNCHER,
	Q4_WP_GAUNTLET,	
	Q4_WP_DMG,
	Q4_WP_RAILGUN,
	Q4_WP_NAPALMGUN,
	Q4_WP_MAX,
} Q4_Weapon;

// enumerations: Q4_PlayerClass_enum
//		Q4_CLASS_DEFAULT - Default playerclass
typedef enum eQ4_PlayerClass_enum
{
	Q4_CLASS_NULL = 0,
	Q4_CLASS_PLAYER,
	Q4_CLASS_MAX,
	Q4_CLASS_ANY = Q4_CLASS_MAX,

	// projectiles
	Q4_CLASSEX_GRENADE,
	Q4_CLASSEX_ROCKET,
	Q4_CLASSEX_NAIL,
	Q4_CLASSEX_HYPERBLASTERSHOT,
	Q4_CLASSEX_NAPALMSHOT,
	Q4_CLASSEX_DARKMATTERSHOT,

	// weapon pickups
	Q4_CLASSEX_WEAPON,
	Q4_CLASSEX_WEAPON_LAST = Q4_CLASSEX_WEAPON+Q4_WP_MAX,

	// ammo pickups
	Q4_CLASSEX_AMMO,
	Q4_CLASSEX_AMMOEND = Q4_CLASSEX_AMMO+Q4_WP_MAX,

	// powerups
	Q4_CLASSEX_POWERUP_MEGAHEALTH, 
	Q4_CLASSEX_POWERUP_QUADDAMAGE, 
	Q4_CLASSEX_POWERUP_HASTE,
	Q4_CLASSEX_POWERUP_REGENERATION,
	Q4_CLASSEX_POWERUP_INVISIBILITY,
	Q4_CLASSEX_POWERUP_CTF_MARINEFLAG,
	Q4_CLASSEX_POWERUP_CTF_STROGGFLAG,
	Q4_CLASSEX_POWERUP_CTF_ONEFLAG,
	Q4_CLASSEX_POWERUP_AMMOREGEN,
	Q4_CLASSEX_POWERUP_GUARD,
	Q4_CLASSEX_POWERUP_DOUBLER,
	Q4_CLASSEX_POWERUP_SCOUT,

	Q4_CLASSEX_DEADZONEPOWERUP,

	Q4_NUM_CLASSES
} Q4_PlayerClass_enum;

typedef enum eQ4_BuyOptions
{
	Q4_BUY_NONE = 0,
	Q4_BUY_MACHINEGUN,
	Q4_BUY_SHOTGUN,
	Q4_BUY_NAILGUN,
	Q4_BUY_HYPERBLASTER,
	Q4_BUY_ROCKETLAUNCHER,
	Q4_BUY_LIGHTNINGGUN,
	Q4_BUY_GRENADELAUNCHER,
	Q4_BUY_RAILGUN,
	Q4_BUY_NAPALMGUN,

	Q4_BUY_ARMOR_SM,
	Q4_BUY_ARMOR_LG,
	Q4_BUY_AMMO_REFILL,

	Q4_BUY_AMMO_REGEN,
	Q4_BUY_HEALTH_REGEN,
	Q4_BUY_DAMAGE_BOOST,
} Q4_BuyOptions;

// enumerations: Q4_Team
//		Q4_TEAM_MARINE - Marine Team.
//		Q4_TEAM_STROGG - Strogg Team.
typedef enum eQ4_Team
{
	Q4_TEAM_NONE = OB_TEAM_NONE,
	Q4_TEAM_MARINE,
	Q4_TEAM_STROGG,
	Q4_TEAM_MAX
} Q4_Team;

// enumerations: Q4_EntityFlags
//		Q4_ENT_IN_VEHICLE - The entity is in a vehicle.
//		Q4_ENT_FLASHLIGHT_ON - The entity has their flashlight on.
typedef enum eQ4_EntityFlags
{
	Q4_ENT_IN_VEHICLE = ENT_FLAG_FIRST_USER,
	Q4_ENT_FLASHLIGHT_ON,
	Q4_ENT_IN_BUY_ZONE,
} Q4_EntityFlags;

// enumerations: Q4_Powerups
//		Q4_PWR_GOTMARINEFLAG - This entity is carrying the marines flag
//		Q4_PWR_GOTSTROGGFLAG - This entity is carrying the strogg flag
//		Q4_PWR_GOTONEFLAG - This entity is carrying the one flag
//		Q4_PWR_GOTQUAD - This entity has quad damage
//		Q4_PWR_HASTE - This entity has haste
//		Q4_PWR_REGEN - Accelerates all weapons, projectiles and even your movement rate by two. 
//					Player has smoke trails when the powerup is active. 
//		Q4_PWR_INVIS - Looks like the effect in Predator with shimmering distortion of textures 
//					around the player using the powerup. You can still hear the player. 
//		Q4_PWR_AMMOREGEN - Regenerates ammunition at a steady rate and appears to increases 
//					firing and reload rate by one third. This powerup stays with you until you are fragged
//		Q4_PWR_GUARD - Similar to Autodoc in Quake2 and Guard in Quake3 Team Arena. 
//					This powerup immediately raises your health and armour to 200 on pickup. 
//					Regenerates your health to 200 after taking damage but does not regenerate armour, 
//					it only allows armour to reach a maximum of 200 without it counting down to 100. 
//					This powerup stays with you until you are fragged
//		Q4_PWR_DOUBLER - Similar to PowerAmplifier in Quake2 and the Doubler in Quake3 Team arena. 
//					This powerup allows any weapon to do double damage. 
//					This powerup stays with you until you are fragged
//		Q4_PWR_SCOUT - Similar to Haste in Quake3 and Scout in Quake3 Team Arena. 
//					This powerup allows movement and firing at double the normal speed but removes all armour. 
//					This powerup stays with you until you are fragged
//		Q4_PWR_DEADZONE - Powerup for deadzone gametype.
//		Q4_PWR_TEAM_AMMO_REGEN - Ammo regen for the entire team.
//		Q4_PWR_TEAM_HEALTH_REGEN - Health regen for the entire team.
//		Q4_PWR_TEAM_DAMAGE_MOD - Damage multiplier for the entire team.
typedef enum eQ4_Powerups
{
	Q4_PWR_MARINEFLAG = PWR_FIRST_USER,
	Q4_PWR_STROGGFLAG,
	Q4_PWR_ONEFLAG,
	Q4_PWR_QUAD,
	Q4_PWR_HASTE,
	Q4_PWR_REGEN,
	Q4_PWR_INVIS,
	Q4_PWR_AMMOREGEN,
	Q4_PWR_GUARD,
	Q4_PWR_DOUBLER,
	Q4_PWR_SCOUT,
	Q4_PWR_DEADZONE,

	// Team Powerups
	Q4_PWR_TEAM_AMMO_REGEN,
	Q4_PWR_TEAM_HEALTH_REGEN,
	Q4_PWR_TEAM_DAMAGE_MOD,
} Q4_Powerups;

#endif
