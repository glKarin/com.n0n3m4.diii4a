////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __DOD_EVENTS_H__
#define __DOD_EVENTS_H__

#include "Omni-Bot_Types.h"
#include "Omni-Bot_Events.h"

// enumerations: eDOD_ButtonFlags
//		DOD_BOT_BUTTON_DROPITEM - Drop carried item(flag).
//		DOD_BOT_BUTTON_DROPAMMO - Drop ammo(discard).
typedef enum eDOD_ButtonFlags
{	
	DOD_BOT_BUTTON_DROPITEM = BOT_BUTTON_FIRSTUSER,
	DOD_BOT_BUTTON_DROPAMMO,

	// THIS MUST BE LAST
	DOD_BOT_BUTTON_FIRSTUSER
} DOD_ButtonFlags;

// enumerations: DOD_EntityClass
//		DOD_CLASS_RIFLEMAN - Rifleman player class.
//		DOD_CLASS_ASSAULT - Assault player class.
//		DOD_CLASS_SUPPORT - Support player class.
//		DOD_CLASS_SNIPER - Sniper player class.
//		DOD_CLASS_MACHINEGUNNER - Machinegunner player class.
//		DOD_CLASS_ROCKET - Rocket player class.
typedef enum eDOD_EntityClass
{
	DOD_CLASS_UNKNOWN = 0,
	DOD_CLASS_NONE = 0,
	DOD_CLASS_RIFLEMAN,
	DOD_CLASS_ASSAULT,
	DOD_CLASS_SUPPORT,
	DOD_CLASS_SNIPER,
	DOD_CLASS_MACHINEGUNNER,
	DOD_CLASS_ROCKET,
	DOD_CLASS_MAX,
	DOD_CLASS_ANY = DOD_CLASS_MAX,

	DOD_CLASSEX_RIFLEGREN_GER_GRENADE,
	DOD_CLASSEX_RIFLEGREN_US_GRENADE,
	DOD_CLASSEX_PSCHRECK_ROCKET,
	DOD_CLASSEX_BAZOOKA_ROCKET,

	// THIS MUST STAY LAST
	DOD_NUM_CLASSES
} DOD_EntityClass;

// enumerations: eDOD_Weapon
typedef enum eDOD_Weapon
{
	DOD_WP_NONE = INVALID_WEAPON,
	
	// all but assault
	DOD_WP_SPADE,
	DOD_WP_AMERKNIFE,

	// assault and support only
	DOD_WP_FRAG_GER,
	DOD_WP_FRAG_US,

	// rifleman
	DOD_WP_K98,
	DOD_WP_RIFLEGREN_GER,
	DOD_WP_GARAND,
	DOD_WP_RIFLEGREN_US,

	// assault
	DOD_WP_MP40,
	DOD_WP_P38,
	DOD_WP_SMOKE_GER,
	DOD_WP_THOMPSON,
	DOD_WP_COLT,
	DOD_WP_SMOKE_US,

	// support
	DOD_WP_MP44,
	DOD_WP_BAR,

	// sniper
	DOD_WP_K98S,
	DOD_WP_SPRING,
	
	// machine gunner
	DOD_WP_MG42,
	DOD_WP_30CAL,

	// rocket
	DOD_WP_PSCHRECK,
	DOD_WP_C96,
	DOD_WP_BAZOOKA,
	DOD_WP_M1CARBINE,

	// THIS MUST STAY LAST
	DOD_WP_MAX
} DOD_Weapon;

// constants: DOD_ItemTypes
//		DOD_GREN_FRAG - Frag grenade.
//		DOD_GREN_SMOKE - Smoke grenade.
typedef enum eDOD_ItemTypes
{
	DOD_GREN_FRAG = 1,
	DOD_GREN_SMOKE,
	
	// THIS MUST STAY LAST
	DOD_NUM_GRENADES
} DOD_ItemTypes;

// enumerations: DOD_Team
//		DOD_TEAM_AXIS - Axis team.
//		DOD_TEAM_ALLIES - Allies team.
typedef enum eDOD_Team
{
	DOD_TEAM_NONE = OB_TEAM_NONE,
	DOD_TEAM_AXIS,
	DOD_TEAM_ALLIES,

	// THIS MUST STAY LAST
	DOD_TEAM_MAX
} DOD_Team;

// typedef: DOD_Events
//		Defines the events specific to the DOD game, numbered starting at the end of
//		the global events.
typedef enum eDOD_Events
{
	DOD_MSG_BEGIN = EVENT_NUM_EVENTS,

	// General Events
	DOD_MSG_BUILD_MUSTBEONGROUND,
	
	// Rifleman
	DOD_MSG_RIFLEMAN_START,
	// Game Events
	// Internal Events.
	DOD_MSG_RIFLEMAN_END,

	// Assault
	DOD_MSG_ASSAULT_START,
	// Game Events
	// Internal Events
	DOD_MSG_ASSAULT_END,

	// Support
	DOD_MSG_SUPPORT_START,
	// Game Events
	// Internal Events
	DOD_MSG_SUPPORT_END,

	// Sniper
	DOD_MSG_SNIPER_START,
	// Game Events
	// Internal Events
	DOD_MSG_SNIPER_END,

	// Machinegunner
	DOD_MSG_MACHINEGUNNER_START,
	// Game Events
	// Internal Events
	DOD_MSG_MACHINEGUNNER_END,

	// Rocket
	DOD_MSG_ROCKET_START,
	// Game Events	
	// Internal Events
	DOD_MSG_ROCKET_END,

	// THIS MUST STAY LAST
	DOD_MSG_END_EVENTS
} DOD_Events;

// typedef: DOD_GameMessage
//		Events that allow the bot to query for information from the game.
typedef enum eDOD_GameMessage
{
	DOD_MSG_START = GEN_MSG_END,

	DOD_MSG_HUDHINT,
	DOD_MSG_HUDMENU,
	DOD_MSG_HUDTEXT,

	// THIS MUST STAY LAST
	DOD_MSG_END
} DOD_GameMessage;

typedef enum eDOD_Version
{
	DOD_VERSION_0_1 = 1,
	DOD_VERSION_0_8,
	DOD_VERSION_LAST,
	DOD_VERSION_LATEST = DOD_VERSION_LAST - 1
} DOD_Version;

#endif
