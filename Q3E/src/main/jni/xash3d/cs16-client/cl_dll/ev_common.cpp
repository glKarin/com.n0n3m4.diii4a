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
// shared event functions
#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"

#include "r_efx.h"

#include "eventscripts.h"
#include "event_api.h"
#include "pm_shared.h"
#include "events.h"

int g_iRShell, g_iPShell, g_iShotgunShell;

/*
======================
Game_HookEvents

Associate script file name with callback functions.  Callback's must be extern "C" so
 the engine doesn't get confused about name mangling stuff.  Note that the format is
 always the same.  Of course, a clever mod team could actually embed parameters, behavior
 into the actual .sc files and create a .sc file parser and hook their functionality through
 that.. i.e., a scripting system.

That was what we were going to do, but we ran out of time...oh well.
======================
*/
void Game_HookEvents( void )
{
	HOOK_EVENT( ak47, FireAK47 );
	HOOK_EVENT( aug, FireAUG );
	HOOK_EVENT( awp, FireAWP );
	HOOK_EVENT( createexplo, CreateExplo );
	HOOK_EVENT( createsmoke, CreateSmoke );
	HOOK_EVENT( deagle, FireDEAGLE );
	HOOK_EVENT( decal_reset, DecalReset );
	HOOK_EVENT( elite_left, FireEliteLeft );
	HOOK_EVENT( elite_right, FireEliteRight );
	HOOK_EVENT( famas, FireFAMAS );
	HOOK_EVENT( fiveseven, Fire57 );
	HOOK_EVENT( g3sg1, FireG3SG1 );
	HOOK_EVENT( galil, FireGALIL );
	HOOK_EVENT( glock18, Fireglock18 );
	HOOK_EVENT( knife, Knife );
	HOOK_EVENT( m249, FireM249 );
	HOOK_EVENT( m3, FireM3 );
	HOOK_EVENT( m4a1, FireM4A1 );
	HOOK_EVENT( mac10, FireMAC10 );
	HOOK_EVENT( mp5n, FireMP5 );
	HOOK_EVENT( p228, FireP228 );
	HOOK_EVENT( p90, FireP90 );
	HOOK_EVENT( scout, FireScout );
	HOOK_EVENT( sg550, FireSG550 );
	HOOK_EVENT( sg552, FireSG552 );
	HOOK_EVENT( tmp, FireTMP );
	HOOK_EVENT( ump45, FireUMP45 );
	HOOK_EVENT( usp, FireUSP );
	HOOK_EVENT( vehicle, Vehicle );
	HOOK_EVENT( xm1014, FireXM1014 );

	if( !stricmp( gEngfuncs.pfnGetGameDirectory(), "czeror" ) )
	{
		HOOK_EVENT( m60, FireM60 );
		HOOK_EVENT( camera, FireCamera );
		HOOK_EVENT( fiberopticcamera, FireFiberOpticCamera );
		HOOK_EVENT( shieldgun, FireShieldGun );
		HOOK_EVENT( blowtorchholster, HolsterBlowtorch );
		HOOK_EVENT( blowtorchidle, IdleBlowtorch );
		HOOK_EVENT( blowtorch, FireBlowtorch );
		HOOK_EVENT( laws, FireLaws );
		HOOK_EVENT( briefcase, FireBriefcase );
		HOOK_EVENT( medkit, FireMedkit );
		HOOK_EVENT( syringe, FireSyringe );
		HOOK_EVENT( radio, FireRadio );
		HOOK_EVENT( zipline, FireZipline );
		HOOK_EVENT( create_glass, CreateGlass );
		HOOK_EVENT( explosion, GrenadeExplosion );
	}
}

/*
=================
EV_GetGunPosition

Figure out the height of the gun
=================
*/
void EV_GetGunPosition( event_args_t *args, Vector &pos, const Vector &origin )
{
	int idx;
	Vector view_ofs(0, 0, 0);

	idx = args->entindex;

	if ( EV_IsPlayer( idx ) )
	{
		// in spec mode use entity viewheigh, not own
		if ( EV_IsLocal( idx ) && !IS_FIRSTPERSON_SPEC )
		{
			// Grab predicted result for local player
			gEngfuncs.pEventAPI->EV_LocalPlayerViewheight( view_ofs );
		}
		else if ( args->ducking == 1 )
		{
			view_ofs[2] = VEC_DUCK_VIEW;
		}
	}
	else
	{
		view_ofs[2] = DEFAULT_VIEWHEIGHT;
	}

	pos = origin + view_ofs;
}

/*
=================
EV_GetDefaultShellInfo

Determine where to eject shells from
=================
*/
void EV_GetDefaultShellInfo( event_args_t *args, float *origin, float *velocity, float *ShellVelocity, float *ShellOrigin, float *forward, float *right, float *up, float forwardScale, float upScale, float rightScale, bool bReverseDirection )
{
	int idx = args->entindex;

	vec3_t view_ofs = { 0, 0, DEFAULT_VIEWHEIGHT };
	if ( EV_IsPlayer( idx ) )
	{
		if ( EV_IsLocal( idx ) )
		{
			gEngfuncs.pEventAPI->EV_LocalPlayerViewheight( view_ofs );
		}
		else if ( args->ducking == 1 )
		{
			view_ofs[2] = VEC_DUCK_VIEW;
		}
	}

	float fR = gEngfuncs.pfnRandomFloat( 50, 70 );
	float fU = gEngfuncs.pfnRandomFloat( 75, 175 );
	float fF = gEngfuncs.pfnRandomFloat( 25, 250 );
	float fDirection = rightScale > 0.0f ? -1.0f : 1.0f;

	for ( int i = 0; i < 3; i++ )
	{
		if( bReverseDirection )
		{
			ShellVelocity[i] = velocity[i] * 0.5f - right[i] * fR * fDirection + up[i] * fU + forward[i] * fF;
		}
		else
		{
			ShellVelocity[i] = velocity[i] * 0.5f + right[i] * fR * fDirection + up[i] * fU + forward[i] * fF;
		}
		ShellOrigin[i]   = velocity[i] * 0.1f + origin[i] + view_ofs[i] +
				upScale * up[i] + forwardScale * forward[i] + rightScale * right[i];
	}
}
