////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
// about: D3 game definitions
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __D3_EVENTS_H__
#define __D3_EVENTS_H__

#include "Omni-Bot_Types.h"
#include "Omni-Bot_Events.h"

typedef enum eD3_Version
{
	D3_VERSION_0_53 = 1,
	D3_VERSION_0_6,
	D3_VERSION_0_65,
	D3_VERSION_0_7_alpha1,
	D3_VERSION_0_7_alpha2,
	D3_VERSION_0_7,
	D3_VERSION_0_8,
	D3_VERSION_LATEST
} D3_Version;

// typedef: D3_Events
//		Defines the events specific to the game, numbered starting at the end of
//		the global events.
typedef enum eD3_Events
{
	D3_EVENT_BEGIN = EVENT_NUM_EVENTS,	
	D3_EVENT_END
} D3_Events;

// typedef: D3_GameEvents
//		Events that allow the bot to query for information from the game.
typedef enum eD3_GameMessage
{
	D3_MSG_START = GEN_MSG_END,
	D3_MSG_GETLOCATION,
	D3_MSG_END
} D3_GameMessage;

// enumerations: D3_Weapon
//		D3_WP_FLASHLIGHT - Flashlight
//		D3_WP_FISTS - Punch.
//		D3_WP_CHAINSAW - Chainsaw
//		D3_WP_PISTOL - Pistol
//		D3_WP_SHOTGUN - Shotgun
//		D3_WP_SHOTGUN_DBL - Dbl Barrel Shotgun
//		D3_WP_MACHINEGUN - MachineGun
//		D3_WP_CHAINGUN - Chaingun
//		D3_WP_HANDGRENADE -Hand Grenade
//		D3_WP_PLASMAGUN - Plasma gun
//		D3_WP_ROCKETLAUNCHER - Rocket Launcher
//		D3_WP_BFG - BFG
//		D3_WP_SOULCUBE - Soulcube
//		D3_WP_GRABBER - Grabber
//		D3_WP_ARTIFACT - Artifact
typedef enum eD3_Weapon
{
	D3_WP_NONE = INVALID_WEAPON,
	D3_WP_FLASHLIGHT,
	D3_WP_FISTS,
	D3_WP_CHAINSAW,
	D3_WP_PISTOL,
	D3_WP_SHOTGUN,
	D3_WP_SHOTGUN_DBL,
	D3_WP_MACHINEGUN,
	D3_WP_CHAINGUN,
	D3_WP_HANDGRENADE,
	D3_WP_PLASMAGUN,
	D3_WP_ROCKETLAUNCHER,	
	D3_WP_BFG,
	D3_WP_SOULCUBE,
	D3_WP_GRABBER,
	//D3_WP_ARTIFACT, todo:
	D3_WP_MAX,
} D3_Weapon;

// enumerations: D3_PlayerClass_enum
//		D3_CLASS_DEFAULT - Default playerclass
typedef enum eD3_PlayerClass_enum
{
	D3_CLASS_NULL = 0,
	D3_CLASS_PLAYER,
	D3_CLASS_MAX,
	D3_CLASS_ANY = D3_CLASS_MAX,

	// weapon pickups
	D3_CLASSEX_WEAPON,
	D3_CLASSEX_WEAPON_LAST = D3_CLASSEX_WEAPON+D3_WP_MAX,

	// projectiles
	D3_CLASSEX_GRENADE,
	D3_CLASSEX_ROCKET,
	D3_CLASSEX_PLASMA,
	D3_CLASSEX_BFG,
	D3_CLASSEX_SOULBLAST,

	// ammo pickups
	D3_CLASSEX_AMMO,
	D3_CLASSEX_AMMO_SHELLS,
	D3_CLASSEX_AMMO_ROCKET,
	D3_CLASSEX_AMMO_CLIP,
	D3_CLASSEX_AMMO_BULLETS,
	D3_CLASSEX_AMMO_BELT,
	D3_CLASSEX_AMMO_CELLS,
	D3_CLASSEX_AMMO_GRENADE,
	D3_CLASSEX_AMMO_BFG,

	// powerups
	D3_CLASSEX_POWERUP_QUADDAMAGE, 
	D3_CLASSEX_POWERUP_HASTE,
	D3_CLASSEX_POWERUP_REGENERATION,
	D3_CLASSEX_POWERUP_INVISIBILITY,
	D3_CLASSEX_POWERUP_CTF_MARINEFLAG,
	D3_CLASSEX_POWERUP_CTF_STROGGFLAG,
	D3_CLASSEX_POWERUP_CTF_ONEFLAG,
	D3_CLASSEX_POWERUP_AMMOREGEN,
	D3_CLASSEX_POWERUP_GUARD,
	D3_CLASSEX_POWERUP_DOUBLER,
	D3_CLASSEX_POWERUP_SCOUT,

	D3_NUM_CLASSES
} D3_PlayerClass_enum;

// enumerations: D3_Team
//		D3_TEAM_MARINE - Marine Team.
//		D3_TEAM_STROGG - Strogg Team.
typedef enum eD3_Team
{
	D3_TEAM_NONE = OB_TEAM_NONE,
	D3_TEAM_RED,
	D3_TEAM_BLUE,
	D3_TEAM_MAX
} D3_Team;

// enumerations: D3_EntityFlags
//		D3_ENT_IN_VEHICLE - The entity is in a vehicle.
//		D3_ENT_FLASHLIGHT_ON - The entity has their flashlight on.
typedef enum eD3_EntityFlags
{
	D3_ENT_IN_VEHICLE = ENT_FLAG_FIRST_USER,
	D3_ENT_FLASHLIGHT_ON,
} D3_EntityFlags;

// enumerations: D3_Powerups
//		D3_ENT_GOTMARINEFLAG - This entity is carrying the marines flag
//		D3_PWR_GOTSTROGGFLAG - This entity is carrying the strogg flag
//		D3_PWR_GOTONEFLAG - This entity is carrying the one flag
//		D3_PWR_GOTQUAD - This entity has quad damage
//		D3_PWR_HASTE - This entity has haste
//		D3_PWR_REGEN - Accelerates all weapons, projectiles and even your movement rate by two. 
//					Player has smoke trails when the powerup is active. 
//		D3_PWR_INVIS - Looks like the effect in Predator with shimmering distortion of textures 
//					around the player using the powerup. You can still hear the player. 
//		D3_PWR_AMMOREGEN - Regenerates ammunition at a steady rate and appears to increases 
//					firing and reload rate by one third. This powerup stays with you until you are fragged
//		D3_PWR_GUARD - Similar to Autodoc in Quake2 and Guard in Quake3 Team Arena. 
//					This powerup immediately raises your health and armour to 200 on pickup. 
//					Regenerates your health to 200 after taking damage but does not regenerate armour, 
//					it only allows armour to reach a maximum of 200 without it counting down to 100. 
//					This powerup stays with you until you are fragged
//		D3_PWR_DOUBLER - Similar to PowerAmplifier in Quake2 and the Doubler in Quake3 Team arena. 
//					This powerup allows any weapon to do double damage. 
//					This powerup stays with you until you are fragged
//		D3_PWR_SCOUT - Similar to Haste in Quake3 and Scout in Quake3 Team Arena. 
//					This powerup allows movement and firing at double the normal speed but removes all armour. 
//					This powerup stays with you until you are fragged
//		D3_PWR_DEADZONE - Powerup for deadzone gametype.
//		D3_PWR_TEAM_AMMO_REGEN - Ammo regen for the entire team.
//		D3_PWR_TEAM_HEALTH_REGEN - Health regen for the entire team.
//		D3_PWR_TEAM_DAMAGE_MOD - Damage multiplier for the entire team.
typedef enum eD3_Powerups
{
	D3_PWR_BERSERK = PWR_FIRST_USER,
	D3_PWR_INVISIBILITY,
	D3_PWR_MEGAHEALTH,
	D3_PWR_ADRENALINE,
	D3_PWR_REDFLAG,
	D3_PWR_BLUEFLAG,
} D3_Powerups;

#endif
