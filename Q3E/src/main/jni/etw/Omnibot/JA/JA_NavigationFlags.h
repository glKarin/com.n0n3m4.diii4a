////////////////////////////////////////////////////////////////////////////////
// 
// $LastChangedBy$
// $LastChangedDate$
// $LastChangedRevision$
//
// File: JA Navigation Flags
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __JA_NAVIGATIONFLAGS_H__
#define __JA_NAVIGATIONFLAGS_H__

#include "NavigationFlags.h"

static const NavFlags F_JA_NAV_HIDE			= F_NAV_NEXT;
static const NavFlags F_JA_NAV_CAMP			= (F_NAV_NEXT<<1);
static const NavFlags F_JA_NAV_CAPPOINT		= (F_NAV_NEXT<<2);
static const NavFlags F_JA_NAV_WALL			= (F_NAV_NEXT<<3);
static const NavFlags F_JA_NAV_BRIDGE		= (F_NAV_NEXT<<4);
static const NavFlags F_JA_NAV_BUTTON1		= (F_NAV_NEXT<<5);
static const NavFlags F_JA_NAV_FORCEJUMP		= (F_NAV_NEXT<<6);

#endif

