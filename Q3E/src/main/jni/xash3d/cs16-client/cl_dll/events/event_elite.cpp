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
#include "wpn_shared.h"


static const char *SOUNDS_NAME = "weapons/elite_fire.wav";

// false is left
// true is right
void EV_FireElite( event_args_s *args, bool isRight )
{
	Vector ShellVelocity;
	Vector ShellOrigin;
	Vector vecSrc, vecAiming;

	int    idx = args->entindex;
	Vector origin( args->origin );
	Vector angles( args->angles );
	Vector velocity( args->velocity );
	Vector forward, right, up;
	int		sequence;
	float flTimeDiff = args->fparam1;
	int iBulletsLeft = args->iparam2;
	AngleVectors( angles, forward, right, up );

	if ( EV_IsLocal( idx ) )
	{
		++g_iShotsFired;
		EV_MuzzleFlash();

		if( !iBulletsLeft ) sequence = ELITE_SHOOTLEFTLAST;
		else if( flTimeDiff >= 0.5 ) sequence = ELITE_SHOOTLEFT5;
		else if( flTimeDiff >= 0.4 ) sequence = ELITE_SHOOTLEFT4;
		else if( flTimeDiff >= 0.3 ) sequence = ELITE_SHOOTLEFT3;
		else if( flTimeDiff >= 0.2 ) sequence = ELITE_SHOOTLEFT2;
		else sequence = ELITE_SHOOTLEFT1;

		if( isRight )
		{
			sequence += (ELITE_SHOOTRIGHT1 - ELITE_SHOOTLEFT1);

			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 35.0, -11.0, -16.0, 0);
		}
		else
		{
			EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 35.0, -11.0, 16.0, 0);
		}

		gEngfuncs.pEventAPI->EV_WeaponAnimation(sequence, 2);
	}
	else
	{
		EV_GetDefaultShellInfo( args, origin, velocity, ShellVelocity, ShellOrigin, forward, right, up, 20.0, -12.0, 4.0, 0);
	}
	EV_EjectBrass(ShellOrigin, ShellVelocity, angles[ YAW ], g_iPShell, TE_BOUNCE_SHELL);

	PLAY_EVENT_SOUND( SOUNDS_NAME );

	EV_GetGunPosition( args, vecSrc, origin );
	VectorCopy( forward, vecAiming );
	Vector vSpread;
	
	vSpread.x = args->fparam2;
	vSpread.y = args->iparam1 / 100.0f;

	if( isRight )
	{
		vecSrc = vecSrc + right * 5;
	}
	else
	{
		vecSrc = vecSrc - right * 5;
	}

	EV_HLDM_FireBullets( idx,
		forward, right,	up,
		1, vecSrc, vecAiming,
		vSpread, 8192.0, BULLET_PLAYER_9MM,
		2 );
}

void EV_FireEliteLeft(event_args_s *args)
{
	EV_FireElite( args, false );
}

void EV_FireEliteRight( event_args_s *args )
{
	EV_FireElite( args, true );
}
