////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
// about: WOLF definitions
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __WOLF_EVENTS_H__
#define __WOLF_EVENTS_H__

#include "Omni-Bot_Types.h"
#include "Omni-Bot_Events.h"

typedef enum eWOLF_Version
{
	WOLF_VERSION_CURRENT = 1
} WOLF_Version;

// typedef: WOLF_Events
//		Defines the events specific to the ETF game, numbered starting at the end of
//		the global events.
typedef enum
{
	WOLF_MESSAGE_BEGIN = EVENT_NUM_EVENTS,
	
	WOLF_MESSAGE_END
} WOLF_Events;

// typedef: TF_GameEvents
//		Events that allow the bot to query for information from the game.
typedef enum
{
	WOLF_MSG_START = GEN_MSG_END,
	WOLF_MSG_END
} WOLF_GameMessage;

// typedef: WOLF_PlayerClass_enum
//		The available classes for this gametype
typedef enum 
{
	WOLF_CLASS_NULL = 0,
	WOLF_CLASS_ENGINEER,
	WOLF_CLASS_MEDIC,
	WOLF_CLASS_SOLDIER,
	WOLF_CLASS_MAX,
	WOLF_CLASS_ANY = WOLF_CLASS_MAX,

	// Other values to identify the "class"
	WOLF_CLASSEX_GRENADE,

	WOLF_NUM_CLASSES
} WOLF_PlayerClass_enum;

// typedef: WOLF_Weapon
//		The available weapons for this gametype
typedef enum
{
	WOLF_WP_NONE = INVALID_WEAPON,
	WOLF_WP_FLAMETHROWER,
	WOLF_WP_PANZERSCHRECK,
	WOLF_WP_PARTICLECANNON,
	WOLF_WP_TESLACANNON,
	WOLF_WP_MP40,
	WOLF_WP_KAR98,
	WOLF_WP_MP43,
	WOLF_WP_MAX
} WOLF_Weapon;


// typedef: WOLF_Team
//		The available teams for this gametype
typedef enum
{
	WOLF_TEAM_NONE = OB_TEAM_NONE,
	WOLF_ALLIES,
	WOLF_AXIS,
	WOLF_TEAM_MAX
} WOLF_Team;

#endif
