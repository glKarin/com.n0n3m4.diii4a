/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

/*
==========================
This file contains "stubs" of class member implementations so that we can predict certain
 weapons client side.  From time to time you might find that you need to implement part of the
 these functions.  If so, cut it from here, paste it in hl_weapons.cpp or somewhere else and
 add in the functionality you need.
==========================
*/
#include    "port.h"
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"player.h"
#include	"weapons.h"
#include	"nodes.h"
#include	"soundent.h"
#include	"skill.h"

// Globals used by game logic
const Vector g_vecZero = Vector( 0, 0, 0 );
int gmsgWeapPickup = 0;
enginefuncs_t g_engfuncs;
globalvars_t  *gpGlobals;
ItemInfo CBasePlayerItem::ItemInfoArray[MAX_WEAPONS];

