////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __TF2_EVENTS_H__
#define __TF2_EVENTS_H__

#include "Omni-Bot_Types.h"
#include "Omni-Bot_Events.h"
#include "TF_Config.h"

typedef enum eTF2_Version
{
	TF2_VERSION_0_1 = 1,
	TF2_VERSION_0_8,
	TF2_VERSION_LAST,
	TF2_VERSION_LATEST = TF2_VERSION_LAST - 1
} TF2_Version;

typedef enum eTF2_Weapon
{
	TF2_WP_NONE = INVALID_WEAPON,
	
	TF2_WP_MELEE, // overloaded for all melee weapons

	// scout
	TF2_WP_SCATTERGUN,
	TF2_WP_PISTOL,
	//TF2_WP_BASEBALLBAT,

	// sniper
	TF2_WP_SNIPER_RIFLE,
	TF2_WP_SMG,
	//TF2_MACHETE,

	// soldier
	TF2_WP_ROCKET_LAUNCHER,
	TF2_WP_SHOTGUN,
	//TF2_SHOVEL,

	// demoman
	TF2_WP_GRENADE_LAUNCHER,
	TF2_WP_PIPE_LAUNCHER,
	//TF2_WP_BOTTLE,
	
	// hwguy
	TF2_WP_MINIGUN,
	//TF2_WP_SHOTGUN,
	//TF2_WP_FISTS,

	// medic
	TF2_WP_SYRINGE_GUN,
	TF2_WP_MEDIGUN,
	//TF2_WP_BONE_SAW,

	// pyro
	TF2_WP_FLAMETHROWER,
	//TF2_WP_SHOTGUN_PYRO,
	//TF2_WP_FIRE_AXE,

	// spy
	TF2_WP_REVOLVER,
	TF2_WP_ELECTRO_SAPPER,
	//TF2_WP_KNIFE,

	// engineer
	//TF2_WP_SHOTGUN_ENGINEER,
	//TF2_WP_PISTOL_ENGINEER,
	//TF2_WP_WRENCH,
	TF2_WP_ENGINEER_BUILD,
	TF2_WP_ENGINEER_DESTROY,
	TF2_WP_ENGINEER_BUILDER,

	// THIS MUST STAY LAST
	TF2_WP_MAX
} TF2_Weapon;

#endif
