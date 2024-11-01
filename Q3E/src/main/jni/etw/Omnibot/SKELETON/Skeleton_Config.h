////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
// about: Skeleton game definitions
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __SKELETON_EVENTS_H__
#define __SKELETON_EVENTS_H__

#include "Omni-Bot_Types.h"
#include "Omni-Bot_Events.h"

typedef enum eSkeleton_Version
{
	SKELETON_VERSION_1 = 1,
	SKELETON_VERSION_LATEST
} Skeleton_Version;

// typedef: Skeleton_Events
//		Defines the events specific to the game, numbered starting at the end of
//		the global events.
typedef enum
{
	SKELETON_MESSAGE_BEGIN = EVENT_NUM_EVENTS,
	
	SKELETON_MESSAGE_END
} Skeleton_Events;

// typedef: TF_GameEvents
//		Events that allow the bot to query for information from the game.
typedef enum
{
	SKELETON_MSG_START = GEN_MSG_END,

	SKELETON_MSG_END
} Skeleton_GameMessage;

// typedef: Skeleton_PlayerClass_enum
//		The available classes for this gametype
typedef enum 
{
	SKELETON_CLASS_NULL = 0,
	SKELETON_CLASS_PLAYER,
	SKELETON_CLASS_MAX,
	SKELETON_CLASS_ANY = SKELETON_CLASS_MAX,

	// Other values to identify the "class"
	SKELETON_CLASSEX_GRENADE,

	SKELETON_NUM_CLASSES
} Skeleton_PlayerClass_enum;

// typedef: Skeleton_Weapon
//		The available weapons for this gametype
typedef enum
{
	SKELETON_WP_NONE = INVALID_WEAPON,
	SKELETON_WP_SMG,	
	SKELETON_WP_RPG,
	SKELETON_WP_MAX
} Skeleton_Weapon;

// typedef: Skeleton_Team
//		The available teams for this gametype
typedef enum
{
	SKELETON_TEAM_NONE = OB_TEAM_NONE,
	SKELETON_TEAM_1 = 0,
	SKELETON_TEAM_2,
	SKELETON_TEAM_MAX
} Skeleton_Team;

#endif
