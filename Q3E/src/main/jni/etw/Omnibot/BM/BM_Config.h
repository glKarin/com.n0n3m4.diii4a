////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
// about: WOLF definitions
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __BM_EVENTS_H__
#define __BM_EVENTS_H__

#include "Omni-Bot_Types.h"
#include "Omni-Bot_Events.h"

typedef enum eBM_Version
{
	BM_VERSION_CURRENT = 1,
	BM_VERSION_LATEST,
} BM_Version;

// enumerations: TF_ButtonFlags
typedef enum eBM_ButtonFlags
{
	BM_BOT_BUTTON_USE_POWERUP = BOT_BUTTON_FIRSTUSER,

	// must be last active module
	BM_BOT_BUTTON_MODULE_LAST,
} BM_ButtonFlags;

// enumerations: BM_EntityCategory
typedef enum eTF_EntityCategory
{
	BM_ENT_CAT_SOMETHING = ENT_CAT_MAX,

	// THIS MUST BE LAST
	BM_ENT_CAT_MAX,
} BM_EntityCategory;

// typedef: BM_Events
//		Defines the events specific to the ETF game, numbered starting at the end of
//		the global events.
typedef enum eBM_Events
{
	BM_EVENT_BEGIN = EVENT_NUM_EVENTS,
	BM_EVENT_PLAYER_SPREE,
	BM_EVENT_PLAYER_SPREE_END,
	BM_EVENT_SPREEWAR_START,
	BM_EVENT_SPREEWAR_END,
	BM_EVENT_LEVEL_UP,
	BM_EVENT_END
} BM_Events;

// typedef: BM_GameEvents
//		Events that allow the bot to query for information from the game.
typedef enum
{
	BM_MSG_START = GEN_MSG_END,
	BM_MSG_END
} BM_GameMessage;

// typedef: BM_Weapon
//		The available weapons for this gametype
typedef enum
{
	BM_WP_NONE = INVALID_WEAPON,
	
	BM_WP_SMG,
	BM_WP_MINIGUN,
	BM_WP_PLASMA,
	BM_WP_RPG,
	BM_WP_GL,
	BM_WP_PHOENIX,
	BM_WP_ION_CANNON,

	BM_WP_MAX
} BM_Weapon;

// typedef: BM_PlayerClass
//		The available classes for this gametype
typedef enum 
{
	BM_CLASS_NULL = 0,
	BM_CLASS_PLAYER,
	BM_CLASS_MAX,
	BM_CLASS_ANY = BM_CLASS_MAX,

	BM_CLASSEX_WEAPON,
	BM_CLASSEX_WEAPON_LAST = BM_CLASSEX_WEAPON+BM_WP_MAX,

	BM_CLASSEX_SMALL_HEALTH,
	BM_CLASSEX_MEDIUM_HEALTH,
	BM_CLASSEX_LARGE_HEALTH,
	BM_CLASSEX_LIGHT_ARMOR,
	BM_CLASSEX_HEAVY_ARMOR,

	BM_CLASSEX_BULLET_AMMO,
	BM_CLASSEX_PLASMA_AMMO,
	BM_CLASSEX_ROCKET_AMMO,
	BM_CLASSEX_GRENADE_AMMO,
	BM_CLASSEX_PHOENIX_AMMO,

	BM_NUM_CLASSES
} BM_PlayerClass;

// typedef: BM_Team
//		The available teams for this gametype
typedef enum
{
	BM_TEAM_NONE = OB_TEAM_NONE,
	BM_TEAM_RED,
	BM_TEAM_BLUE,
	BM_TEAM_MAX
} BM_Team;

typedef enum eBM_Powerups
{
	BM_PWR_KILLER, // doubles firepower
	BM_PWR_GHOST, // partially invisible
	BM_PWR_SHIELD, // take 1/3 damage
	BM_PWR_AVENGER, // mirrors damage to source
	BM_PWR_SPRINTER, // run faster
	BM_PWR_NUKE, // kill everyone around you when triggered
} BM_Powerups;

#endif
