#ifndef __RTCW_NAVIGATIONFLAGS_H__
#define __RTCW_NAVIGATIONFLAGS_H__

#include "NavigationFlags.h"

static const NavFlags F_RTCW_NAV_MG42SPOT		= F_NAV_NEXT;
static const NavFlags F_RTCW_NAV_PANZERFSPOT	= (F_NAV_NEXT<<1);
static const NavFlags F_RTCW_NAV_MORTAR			= (F_NAV_NEXT<<2);
static const NavFlags F_RTCW_NAV_MINEAREA		= (F_NAV_NEXT<<3);
static const NavFlags F_RTCW_NAV_AIRSTRAREA		= (F_NAV_NEXT<<4);
static const NavFlags F_RTCW_NAV_WALL			= (F_NAV_NEXT<<5);
static const NavFlags F_RTCW_NAV_BRIDGE			= (F_NAV_NEXT<<6);
static const NavFlags F_RTCW_NAV_SPRINT			= (F_NAV_NEXT<<7);
static const NavFlags F_RTCW_NAV_WATERBLOCKABLE	= (F_NAV_NEXT<<8);
//static const NavFlags F_RTCW_NAV_MORTARTARGET_S	= (F_NAV_NEXT<<9);
//static const NavFlags F_RTCW_NAV_MORTARTARGET_D	= (F_NAV_NEXT<<10);
static const NavFlags F_RTCW_NAV_CAPPOINT		= (F_NAV_NEXT<<11);
static const NavFlags F_RTCW_NAV_SATCHELSPOT	= (F_NAV_NEXT<<12);
static const NavFlags F_RTCW_NAV_ARTSPOT		= (F_NAV_NEXT<<13);
static const NavFlags F_RTCW_NAV_ARTYTARGET_S	= (F_NAV_NEXT<<14);
static const NavFlags F_RTCW_NAV_ARTYTARGET_D	= (F_NAV_NEXT<<15);
//static const NavFlags F_RTCW_NAV_DISGUISE		= (F_NAV_NEXT<<16); 
static const NavFlags F_RTCW_NAV_FLAMETHROWER	= (F_NAV_NEXT<<17);
static const NavFlags F_RTCW_NAV_PANZER			= (F_NAV_NEXT<<18);
static const NavFlags F_RTCW_NAV_STRAFE_L		= (F_NAV_NEXT<<19);
static const NavFlags F_RTCW_NAV_STRAFE_R		= (F_NAV_NEXT<<20);
static const NavFlags F_RTCW_NAV_USERGOAL		= (F_NAV_NEXT<<21);
static const NavFlags F_RTCW_NAV_USEPATH		= (F_NAV_NEXT<<22);
static const NavFlags F_RTCW_NAV_VENOM			= (F_NAV_NEXT<<23);
static const NavFlags F_RTCW_NAV_STRAFE_JUMP_L		= (F_NAV_NEXT<<24);
static const NavFlags F_RTCW_NAV_STRAFE_JUMP_R		= (F_NAV_NEXT<<25);

#endif
