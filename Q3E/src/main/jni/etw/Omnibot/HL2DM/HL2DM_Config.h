////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
// about: HL2DM definitions
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __HL2DM_EVENTS_H__
#define __HL2DM_EVENTS_H__

#include "Omni-Bot_Types.h"
#include "Omni-Bot_Events.h"

typedef enum eHL2DM_Version
{
	HL2DM_VERSION_CURRENT = 1
} HL2DM_Version;

// typedef: HL2DM_Events
//		Defines the events specific to the ETF game, numbered starting at the end of
//		the global events.
typedef enum
{
	HL2DM_MESSAGE_BEGIN = EVENT_NUM_EVENTS,
	
	HL2DM_MESSAGE_END
} HL2DM_Events;

// typedef: TF_GameEvents
//		Events that allow the bot to query for information from the game.
typedef enum
{
	HL2DM_MSG_START = GEN_MSG_END,
	HL2DM_MSG_END
} HL2DM_GameMessage;

// typedef: HL2DM_PlayerClass_enum
//		The available classes for this gametype
typedef enum 
{
	HL2DM_CLASS_NULL = 0,
	HL2DM_CLASS_PLAYER,
	HL2DM_CLASS_MAX,
	HL2DM_CLASS_ANY = HL2DM_CLASS_MAX,

	// Other values to identify the "class"
	HL2DM_CLASSEX_GRENADE,

	HL2DM_NUM_CLASSES
} HL2DM_PlayerClass_enum;

// typedef: HL2DM_Weapon
//		The available weapons for this gametype
typedef enum
{
	HL2DM_WP_NONE = INVALID_WEAPON,
	HL2DM_WP_GRAVGUN,
	HL2DM_WP_STUNSTICK,
	HL2DM_WP_PISTOL,
	HL2DM_WP_REVOLVER, 
	HL2DM_WP_SMG,
	HL2DM_WP_PULSERIFLE, 
	HL2DM_WP_SHOTGUN,
	HL2DM_WP_CROSSBOW,
	HL2DM_WP_GRENADE,
	HL2DM_WP_RPG,
	HL2DM_WP_SLAM,
	HL2DM_WP_MAX
} HL2DM_Weapon;


// typedef: HL2DM_Team
//		The available teams for this gametype
typedef enum
{
	HL2DM_TEAM_NONE = OB_TEAM_NONE,
	HL2DM_TEAM_1 = 0,
	HL2DM_TEAM_2,
	HL2DM_TEAM_MAX
} HL2DM_Team;

#endif
