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

#include <stdio.h>
#include <stdlib.h>
#include <cmath>

#include "hud.h"
#include "cl_util.h"
#include <string.h>

#if !defined(M_PI)
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

#if !defined(M_PI_F)
#define M_PI_F		(float)M_PI
#endif
// extern vec3_t vec3_origin;

// if C++ mangling differs from C symbol name
#if _MSC_VER || __WATCOMC__
float vec3_origin[3];
#endif

float Length( const float *v )
{
	int	i;
	float	length;

	length = 0.0f;
	for( i = 0; i < 3; i++ )
		length += v[i] * v[i];
	length = sqrt( length );		// FIXME

	return length;
}

void VectorAngles( const float *forward, float *angles )
{
	float tmp, yaw, pitch;

	if( forward[1] == 0.0f && forward[0] == 0.0f )
	{
		yaw = 0.0f;
		if( forward[2] > 0.0f )
			pitch = 90.0f;
		else
			pitch = 270.0f;
	}
	else
	{
		yaw = ( atan2( forward[1], forward[0]) * 180.0f / M_PI_F );
		if( yaw < 0.0f )
			yaw += 360.0f;

		tmp = sqrt( forward[0] * forward[0] + forward[1] * forward[1] );
		pitch = ( atan2( forward[2], tmp ) * 180.0f / M_PI_F );
		if( pitch < 0.0f )
			pitch += 360.0f;
	}

	angles[0] = pitch;
	angles[1] = yaw;
	angles[2] = 0.0f;
}

float VectorNormalize( float *v )
{
	float length, ilength;

	length = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
	length = sqrt( length );		// FIXME

	if( length )
	{
		ilength = 1.0f / length;
		v[0] *= ilength;
		v[1] *= ilength;
		v[2] *= ilength;
	}

	return length;
}

void VectorInverse( float *v )
{
	v[0] = -v[0];
	v[1] = -v[1];
	v[2] = -v[2];
}

void VectorScale( const float *in, float scale, float *out )
{
	out[0] = in[0] * scale;
	out[1] = in[1] * scale;
	out[2] = in[2] * scale;
}

void VectorMA( const float *veca, float scale, const float *vecb, float *vecc )
{
	vecc[0] = veca[0] + scale * vecb[0];
	vecc[1] = veca[1] + scale * vecb[1];
	vecc[2] = veca[2] + scale * vecb[2];
}

HSPRITE LoadSprite( const char *pszName )
{
	int i = GetSpriteRes( ScreenWidth, ScreenHeight );
	char sz[256];

	sprintf( sz, pszName, i );

	return SPR_Load( sz );
}
