////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
// Title: OF Config
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __OF_CONFIG_H__
#define __OF_CONFIG_H__

#include "TF_Config.h"

#include "Omni-Bot_Types.h"
#include "Omni-Bot_Events.h"

typedef enum eOF_Version
{
	OF_VERSION_0_1 = 17,
	OF_VERSION_LAST,
	OF_VERSION_LATEST = OF_VERSION_LAST - 1
} OF_Version;

// Override this to new value
#ifdef MAX_DEMO_TEAM_PIPES
#undef MAX_DEMO_TEAM_PIPES
#endif
#define MAX_DEMO_TEAM_PIPES 10

typedef enum eOF_Weapon
{
	OF_WP_GRENADE_FLASH = TF_WP_GRENADE_CALTROPS // remapping this
} OF_Weapon;

typedef enum eOF_EntityClass
{
	OF_CLASSEX_FLASH_GRENADE = TF_CLASSEX_CALTROP // remapping this
} OF_EntityClass;

typedef enum eOF_EntityFlags
{
	OF_ENT_FLAG_BLIND = TF_ENT_FLAG_CALTROP, // remapping this
	OF_ENT_FLAG_CONCED = TF_ENT_FLAG_LEVEL3 + 1,
} OF_EntityFlags;

typedef enum eOF_Powerups
{
	OF_PWR_QUAD = TF_PWR_CLOAKED + 1,
	OF_PWR_SUIT,
	OF_PWR_HASTE,
	OF_PWR_INVIS,
	OF_PWR_REGEN,
	OF_PWR_FLIGHT,
	OF_PWR_INVULN,
	OF_PWR_AQUALUNG,
} OF_Powerups;

#endif
