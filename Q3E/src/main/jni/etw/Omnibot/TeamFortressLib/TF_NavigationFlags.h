////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
// Title: TF Navigation Flags
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __TF_NAVIGATIONFLAGS_H__
#define __TF_NAVIGATIONFLAGS_H__

#include "NavigationFlags.h"

// bits: Team Fortress Navigation Flags
//		F_TF_NAV_SENTRY - Build a sentry gun here.
//		F_TF_NAV_DISPENSER - Build a dispenser here.
//		F_TF_NAV_PIPETRAP - Source of pipe trap.
//		F_TF_NAV_DETPACK - Mark a detpack blockable.
//		F_TF_NAV_CAPPOINT - Capture flags here.
//		F_TF_NAV_GRENADES - Grenade pack.
//		F_TF_NAV_ROCKETJUMP - Rocket Jump to here.
//		F_TF_NAV_CONCJUMP - Concussion Jump to here.
static const NavFlags F_TF_NAV_SENTRY			= F_NAV_NEXT;
static const NavFlags F_TF_NAV_DISPENSER			= (F_NAV_NEXT<<1);
static const NavFlags F_TF_NAV_PIPETRAP			= (F_NAV_NEXT<<2);
//static const NavFlags F_TF_NAV_PIPETRAP2			= (F_NAV_NEXT<<3);
static const NavFlags F_TF_NAV_DETPACK			= (F_NAV_NEXT<<4);
static const NavFlags F_TF_NAV_CAPPOINT			= (F_NAV_NEXT<<5);
static const NavFlags F_TF_NAV_GRENADES			= (F_NAV_NEXT<<6);
static const NavFlags F_TF_NAV_ROCKETJUMP		= (F_NAV_NEXT<<7);
static const NavFlags F_TF_NAV_CONCJUMP			= (F_NAV_NEXT<<8);
static const NavFlags F_TF_NAV_WALL				= (F_NAV_NEXT<<9);
static const NavFlags F_TF_NAV_TELE_ENTER		= (F_NAV_NEXT<<10);
static const NavFlags F_TF_NAV_TELE_EXIT		= (F_NAV_NEXT<<11);
static const NavFlags F_TF_NAV_DOUBLEJUMP		= (F_NAV_NEXT<<12);

#endif
