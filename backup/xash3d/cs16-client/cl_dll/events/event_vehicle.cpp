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

#include <string.h>

static const char *SOUNDS_NAME[] =
{
	"plats/vehicle1.wav",
	"plats/vehicle2.wav",
	"plats/vehicle3.wav",
	"plats/vehicle4.wav",
	"plats/vehicle6.wav",
	"plats/vehicle7.wav"
};

static const char *SOUNDS_NAME_TRAIN[] =
{
	"plats/ttrain1.wav",
	"plats/ttrain2.wav",
	"plats/ttrain3.wav",
	"plats/ttrain4.wav",
	"plats/ttrain6.wav",
	"plats/ttrain7.wav"
};


void EV_Vehicle(event_args_s *args)
{
	Vector origin(args->origin);
	int idx = args->entindex;
	unsigned short us_params = (unsigned short)args->iparam1;
	int stop	  = args->bparam1;
	float m_flVolume	= (float)(us_params & 0x003f)/40.0;
	int noise		= (int)(((us_params) >> 12 ) & 0x0007);
	int pitch		= (int)( 10.0 * (float)( ( us_params >> 6 ) & 0x003f ) );


	if( noise < 0 || noise > 5 )
		return;

	if ( stop )
	{
		gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, SOUNDS_NAME[noise] );
	}
	else
	{
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_STATIC, SOUNDS_NAME[noise], m_flVolume, ATTN_NORM, 0, pitch );
	}
}


void EV_TrainPitchAdjust( event_args_t *args )
{
	Vector origin(args->origin);
	int idx = args->entindex;
	unsigned short us_params = (unsigned short)args->iparam1;
	int stop	  = args->bparam1;
	float m_flVolume	= (float)(us_params & 0x003f)/40.0;
	int noise		= (int)(((us_params) >> 12 ) & 0x0007);
	int pitch		= (int)( 10.0 * (float)( ( us_params >> 6 ) & 0x003f ) );

	if( noise < 0 || noise > 5 )
		return;

	if ( stop )
	{
		gEngfuncs.pEventAPI->EV_StopSound( idx, CHAN_STATIC, SOUNDS_NAME_TRAIN[noise] );
	}
	else
	{
		gEngfuncs.pEventAPI->EV_PlaySound( idx, origin, CHAN_STATIC, SOUNDS_NAME_TRAIN[noise], m_flVolume, ATTN_NORM, 0, pitch );
	}
}
