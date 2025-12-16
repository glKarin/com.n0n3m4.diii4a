// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/*
*
*    This program is free software; you can redistribute it and/or modify it
*    under the terms of the GNU General Public License as published by the
*    Free Software Foundation; either version 2 of the License, or (at
*    your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*    In addition, as a special exception, the author gives permission to
*    link the code of this program with the Half-Life Game Engine ("HL
*    Engine") and Modified Game Libraries ("MODs") developed by Valve,
*    L.L.C ("Valve").  You must obey the GNU General Public License in all
*    respects for all of the code used other than the HL Engine and MODs
*    from Valve.  If you modify this file, you may extend this exception
*    to your version of the file, but you are not obligated to do so.  If
*    you do not wish to do so, delete this exception statement from your
*    version.
*
*/
#include "events.h"

void EV_FireM60( event_args_t *args )
{
	Vector ShellVelocity;
	Vector ShellOrigin;
	Vector vecSrc, vecAiming;
	int    idx = args->entindex;
	Vector origin( args->origin );
	Vector angles(
		args->iparam1 / 100.0f + args->angles[0],
		args->iparam2 / 100.0f + args->angles[1],
		args->angles[2]
		);
	Vector velocity( args->velocity );
	Vector forward, right, up;

	AngleVectors( angles, forward, right, up );

	if ( EV_IsLocal( idx ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();
		gEngfuncs.pEventAPI->EV_WeaponAnimation(Com_RandomLong(1,2), 2);
		if( !gHUD.cl_righthand->value )
		{
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20.0, -10.0, -13.0, true);
		}
		else
		{
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20.0, -10.0, 13.0, 0);
		}
	}
	else
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20.0, -12.0, 4.0, 0);
	}


	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[ YAW ], g_iRShell, TE_BOUNCE_SHELL);

	PLAY_EVENT_SOUND( "weapons/m60-1.wav" );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );
	Vector vSpread;

	vSpread.x = args->fparam1;
	vSpread.y = args->fparam2;
	EV_HLDM_FireBullets( idx,
		forward, right,	up,
		1, vecSrc, vecAiming,
		vSpread, 8192.0, BULLET_PLAYER_762MM,
		2 );
}

void EV_FireCamera( event_args_t *args )
{
	int idx = args->entindex;
	Vector origin = args->origin;

	if( EV_IsLocal( idx ) )
		gEngfuncs.pEventAPI->EV_WeaponAnimation( 1, 2 );

	PLAY_EVENT_SOUND( "weapons/camera-1.wav" );
}
void EV_FireFiberOpticCamera( event_args_t *args )
{
	ConsolePrint( "FIRE FIBER OPTIC CAMERA !!! !!! !!!\n");
}
void EV_FireShieldGun( event_args_t *args )
{

}

int g_iBlowTorchFiring = 0;
void EV_HolsterBlowtorch( event_args_t *args )
{
	g_iBlowTorchFiring = 0;
}
void EV_IdleBlowtorch( event_args_t *args )
{
	g_iBlowTorchFiring = 1;
}

void EV_FireBlowtorch( event_args_t *args )
{
	int idx = args->entindex;
	Vector origin = args->origin;
	Vector angles(
		args->iparam1 / 10000000.0f + args->angles[0],
		args->iparam2 / 10000000.0f + args->angles[1],
		args->angles[2] );

	Vector forward, right, up;
	Vector vecSrc;

	AngleVectors( angles, forward, right, up );

	EV_GetGunPosition( args, vecSrc, origin );

	if ( EV_IsLocal(idx) )
	  gEngfuncs.pEventAPI->EV_WeaponAnimation(1, 2);

	g_iBlowTorchFiring = 2;

	Vector vSpread( args->fparam1, args->fparam2, 0.0f );

	EV_HLDM_FireBullets( idx,
		forward, right,	up,
		1, vecSrc, forward,
		vSpread, 36.0, BULLET_PLAYER_BLOWTORCH,
		2 );

}
void EV_FireLaws( event_args_t *args )
{

}
void EV_FireBriefcase( event_args_t *args )
{
	int idx = args->entindex;
	Vector origin = args->origin;

	if( EV_IsLocal( idx ) )
		gEngfuncs.pEventAPI->EV_WeaponAnimation( 3, 2 );

	PLAY_EVENT_SOUND( "weapons/briefcase_use.wav" );
}
void EV_FireMedkit( event_args_t *args )
{
	int idx = args->entindex;
	Vector origin = args->origin;

	if( EV_IsLocal( idx ) )
		gEngfuncs.pEventAPI->EV_WeaponAnimation( 2, 2 );

	PLAY_EVENT_SOUND( "weapons/blowtorch-1.wav" );

}
void EV_FireSyringe( event_args_t *args )
{
	int idx = args->entindex;
	Vector origin = args->origin;

	if( EV_IsLocal( idx ) )
		gEngfuncs.pEventAPI->EV_WeaponAnimation( 2, 2 );

	PLAY_EVENT_SOUND( "weapons/syringe_use.wav" );

}
void EV_FireRadio( event_args_t *args )
{
	int idx = args->entindex;
	Vector origin = args->origin;

	if( EV_IsLocal( idx ) )
		gEngfuncs.pEventAPI->EV_WeaponAnimation( 1, 2 );

	PLAY_EVENT_SOUND( "weapons/radio_activate.wav" );
}
void EV_FireZipline( event_args_t *args )
{
	if ( EV_IsLocal(args->entindex) )
	  gEngfuncs.pEventAPI->EV_WeaponAnimation(3, 2);
}
void EV_CreateGlass( event_args_t *args )
{

}
void EV_GrenadeExplosion( event_args_t *args )
{

}
