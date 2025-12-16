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
#pragma once
#ifndef EVENTS_H
#define EVENTS_H

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"

#include "r_efx.h"

#include "eventscripts.h"
#include "event_api.h"
#include "pm_defs.h"
#include "ev_hldm.h"
#include "com_weapons.h"

#ifndef PITCH
// MOVEMENT INFO
// up / down
#define	PITCH	0
// left / right
#define	YAW		1
// fall over
#define	ROLL	2
#endif

#define DECLARE_EVENT( x ) void EV_##x( struct event_args_s *args )
#define HOOK_EVENT( x, y ) gEngfuncs.pfnHookEvent( "events/" #x ".sc", EV_##y )

#define PLAY_EVENT_SOUND( x ) gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_WEAPON, (x), VOL_NORM, ATTN_NORM, 0, 94 + gEngfuncs.pfnRandomLong( 0, 15 ) )


// enable it until fix isn't applied in Xash3D
//#define _CS16CLIENT_FIX_EVENT_ORIGIN

extern int g_iPShell;
extern int g_iRShell;
extern int g_iShotgunShell;

extern "C"
{
DECLARE_EVENT(FireAK47);
DECLARE_EVENT(FireAUG);
DECLARE_EVENT(FireAWP);
DECLARE_EVENT(CreateExplo);
DECLARE_EVENT(CreateSmoke);
DECLARE_EVENT(FireDEAGLE);
DECLARE_EVENT(DecalReset);
DECLARE_EVENT(FireEliteLeft);
DECLARE_EVENT(FireEliteRight);
DECLARE_EVENT(FireFAMAS);
DECLARE_EVENT(Fire57);
DECLARE_EVENT(FireG3SG1);
DECLARE_EVENT(FireGALIL);
DECLARE_EVENT(Fireglock18);
DECLARE_EVENT(Knife);
DECLARE_EVENT(FireM249);
DECLARE_EVENT(FireM3);
DECLARE_EVENT(FireM4A1);
DECLARE_EVENT(FireMAC10);
DECLARE_EVENT(FireMP5);
DECLARE_EVENT(FireP228);
DECLARE_EVENT(FireP90);
DECLARE_EVENT(FireScout);
DECLARE_EVENT(FireSG550);
DECLARE_EVENT(FireSG552);
DECLARE_EVENT(FireTMP);
DECLARE_EVENT(FireUMP45);
DECLARE_EVENT(FireUSP);
DECLARE_EVENT(Vehicle);
DECLARE_EVENT(FireXM1014);
DECLARE_EVENT(TrainPitchAdjust);

// CZERODS START
DECLARE_EVENT( FireM60 );
DECLARE_EVENT( FireCamera );
DECLARE_EVENT( FireFiberOpticCamera );
DECLARE_EVENT( FireShieldGun );
DECLARE_EVENT( HolsterBlowtorch );
DECLARE_EVENT( IdleBlowtorch );
DECLARE_EVENT( FireBlowtorch );
DECLARE_EVENT( FireLaws );
DECLARE_EVENT( FireBriefcase );
DECLARE_EVENT( FireMedkit );
DECLARE_EVENT( FireSyringe );
DECLARE_EVENT( FireRadio );
DECLARE_EVENT( FireZipline );
DECLARE_EVENT( CreateGlass );
DECLARE_EVENT( GrenadeExplosion );
// CZERODS END
}

void Game_HookEvents( void );

#endif
