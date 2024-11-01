////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
// Title: ET Navigation Flags
// 
////////////////////////////////////////////////////////////////////////////////

#ifndef __ETQW_NavFlags_H__
#define __ETQW_NavFlags_H__

#include "NavigationFlags.h"

// bits: Enemy Territory Navigation Flags
//		F_ETQW_NAV_MG42SPOT - Area for soldier mg42.
//		F_ETQW_NAV_PANZERFSPOT - Area for soldier panzerfaust.
//		F_ETQW_NAV_MORTAR - Area for soldier mortar firing.
//		F_ETQW_NAV_MINEAREA - Area for engineer mine laying.
//		F_ETQW_NAV_AIRSTRAREA - Area for field-ops air strike.
//		F_ETQW_NAV_WALL - Blockable flag for walls that can be destroyed or opened.
//		F_ETQW_NAV_BRIDGE - Blockable flag bridges that can be built to open a pathway.
//		F_ETQW_NAV_SPRINT - Tells the bot to sprite to a waypoint.
//		F_ETQW_NAV_WATERBLOCKABLE - Blockable flag for pathways that can be flooded with water.
//		F_ETQW_NAV_MORTARTARGETQW_S - A static target area for a soldier mortar fire.
//		F_ETQW_NAV_MORTARTARGETQW_D - A dynamic target area for a soldier mortar fire.
//		F_ETQW_NAV_CAPPOINT - A capture point for flags or carryable flag-like goals.
//		F_ETQW_NAV_SATCHELSPOT - Area for covert-ops satchel charge.
//		F_ETQW_NAV_ARTSPOT - Area for field-ops artillery.
//		F_ETQW_NAV_ARTYTARGETQW_S - A static target area for a field-ops artillery fire.
//		F_ETQW_NAV_ARTYTARGETQW_D - A dynamic target area for a field-ops artillery fire.
static const NavFlags F_ETQW_NAV_MG42SPOT		= F_NAV_NEXT;
static const NavFlags F_ETQW_NAV_PANZERFSPOT	= (F_NAV_NEXT<<1);
static const NavFlags F_ETQW_NAV_MORTAR			= (F_NAV_NEXT<<2);
static const NavFlags F_ETQW_NAV_MINEAREA		= (F_NAV_NEXT<<3);
static const NavFlags F_ETQW_NAV_AIRSTRAREA		= (F_NAV_NEXT<<4);
static const NavFlags F_ETQW_NAV_WALL			= (F_NAV_NEXT<<5);
static const NavFlags F_ETQW_NAV_BRIDGE			= (F_NAV_NEXT<<6);
static const NavFlags F_ETQW_NAV_SPRINT			= (F_NAV_NEXT<<7);
static const NavFlags F_ETQW_NAV_WATERBLOCKABLE	= (F_NAV_NEXT<<8);
static const NavFlags F_ETQW_NAV_MORTARTARGETQW_S= (F_NAV_NEXT<<9);
static const NavFlags F_ETQW_NAV_MORTARTARGETQW_D= (F_NAV_NEXT<<10);
static const NavFlags F_ETQW_NAV_CAPPOINT		= (F_NAV_NEXT<<11);
static const NavFlags F_ETQW_NAV_SATCHELSPOT	= (F_NAV_NEXT<<12);
static const NavFlags F_ETQW_NAV_ARTSPOT		= (F_NAV_NEXT<<13);
static const NavFlags F_ETQW_NAV_ARTYTARGETQW_S	= (F_NAV_NEXT<<14);
static const NavFlags F_ETQW_NAV_ARTYTARGETQW_D	= (F_NAV_NEXT<<15);
static const NavFlags F_ETQW_NAV_DISGUISE		= (F_NAV_NEXT<<16);

#endif
