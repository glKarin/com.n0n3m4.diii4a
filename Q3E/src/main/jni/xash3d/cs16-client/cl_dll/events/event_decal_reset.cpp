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
#include <math.h>
#include "events.h"

// HACKHACK: This is very unreliable way to get round time
float g_flRoundTime = 0.0f;

extern TEMPENTITY *g_DeadPlayerModels[64];

void EV_DecalReset(event_args_s *args)
{
	int decalnum = (int)(gEngfuncs.pfnGetCvarFloat("r_decals"));

	for( int i = 0; i < decalnum; i++ )
		gEngfuncs.pEfxAPI->R_DecalRemoveAll( i );
	
	g_flRoundTime = gEngfuncs.GetClientTime();
	
	if ( g_DeadPlayerModels )
	{
		for ( int i = 0; i < 64; i++ )
		{
			if ( g_DeadPlayerModels[i] )
			{
				g_DeadPlayerModels[i]->die = 0.0f;
				g_DeadPlayerModels[i] = NULL;
			}
		}
	}
}
