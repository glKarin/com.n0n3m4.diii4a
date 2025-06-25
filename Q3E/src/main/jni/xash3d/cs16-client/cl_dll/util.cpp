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
//
// util.cpp
//
// implementation of class-less helper functions
//

#include "stdio.h"
#include "stdlib.h"
#include "math.h"

#include "hud.h"
#include "cl_util.h"
#include <string.h>

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

//vec3_t vec3_origin( 0, 0, 0 );

//double sqrt(double x);

float rsqrt( float number )
{
	int	i;
	float	x, y;

	if( number == 0.0f )
		return 0.0f;

	x = number * 0.5f;
	i = *(int *)&number;	// evil floating point bit level hacking
	i = 0x5f3759df - (i >> 1);	// what the fuck?
	y = *(float *)&i;
	y = y * (1.5f - (x * y * y));	// first iteration

	return y;
}

int HUD_GetSpriteIndexByName( const char *sz )
{
	return gHUD.GetSpriteIndex(sz);
}

HSPRITE HUD_GetSprite( int index )
{
	return gHUD.GetSprite(index);
}

wrect_t HUD_GetSpriteRect( int index )
{
	return gHUD.GetSpriteRect( index );
}

vec3_t g_ColorBlue	= { 0.6, 0.8, 1.0 };
vec3_t g_ColorRed	= { 1.0, 0.25, 0.25 };
vec3_t g_ColorGreen	= { 0.0, 1.0, 0.0 };
vec3_t g_ColorYellow= { 1.0, 0.7, 0.0 };
vec3_t g_ColorGrey	= { 0.8, 0.8, 0.8 };

float *GetClientColor( int clientIndex )
{
	switch ( g_PlayerExtraInfo[clientIndex].teamnumber )
	{
	case TEAM_CT:         return g_ColorBlue;
	case TEAM_TERRORIST:  return g_ColorRed;
	case TEAM_UNASSIGNED:
	case TEAM_SPECTATOR:  return g_ColorGrey;
	case 4:		          return g_ColorGreen;
	default:              return g_ColorGrey;
	}
}
