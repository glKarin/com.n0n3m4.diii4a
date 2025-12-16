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

#include "com_model.h"

#define SMOKE_CLOUDS 20

void EV_CreateSmoke(event_args_s *args)
{
	TEMPENTITY *pTemp;

	if( !args->bparam2 ) //first explosion
	{
		const model_t *pGasModel = gEngfuncs.GetSpritePointer(gHUD.m_hGasPuff);

		for( int i = 0; i < SMOKE_CLOUDS; i++ )
		{
			// randomize smoke cloud position
			Vector org = args->origin;
			if( i != 0 )
			{
				org.x += Com_RandomFloat(-100.0f, 100.0f);
				org.y += Com_RandomFloat(-100.0f, 100.0f);
			}
			org.z += 30; 

			pTemp = gEngfuncs.pEfxAPI->CL_TempEntAllocNoModel( org );
			if( pTemp )
			{
				// don't die when animation is ended
				pTemp->flags |= (FTENT_SPRANIMATELOOP | FTENT_COLLIDEWORLD | FTENT_CLIENTCUSTOM);
				pTemp->flags &= ~(FTENT_NOMODEL);
				pTemp->die = gEngfuncs.GetClientTime() + 30.0f;
				pTemp->callback = [](struct tempent_s *te, float frametime, float currenttime) -> void
				{
					if( te->entity.curstate.renderamt > 0 && currenttime >= te->entity.curstate.fuser3 )
					{
						te->entity.curstate.renderamt = 255.0f - (currenttime - te->entity.curstate.fuser3) * te->entity.baseline.renderamt ;
						if( te->entity.curstate.renderamt < 0 ) te->entity.curstate.renderamt = 0;
					}
					EV_CS16Client_KillEveryRound( te, frametime, currenttime );
				};

				// !!! Setup model !!!
				pTemp->entity.model = (struct model_s*)pGasModel;
				pTemp->frameMax = max( 0, pGasModel->numframes - 1 );

				pTemp->entity.curstate.fuser3 = gEngfuncs.GetClientTime() + 15.0f; // start fading after 15 sec
				pTemp->entity.curstate.fuser4 = gEngfuncs.GetClientTime(); // entity creation time

				pTemp->entity.curstate.renderamt = 255;
				pTemp->entity.curstate.rendermode = kRenderTransTexture;
				pTemp->entity.curstate.rendercolor.r = Com_RandomLong(210, 230);
				pTemp->entity.curstate.rendercolor.g = Com_RandomLong(210, 230);
				pTemp->entity.curstate.rendercolor.b = Com_RandomLong(210, 230);
				pTemp->entity.curstate.scale = 5.0f;

				// make it move slowly
				pTemp->entity.baseline.origin.x = Com_RandomLong(-5, 5);
				pTemp->entity.baseline.origin.y = Com_RandomLong(-5, 5);
				pTemp->entity.baseline.renderamt = 18;

				if( i == 0 )
				{
					gHUD.m_Spectator.AddOverviewEntityToList( gHUD.m_hGasPuff, &pTemp->entity, 14.0f );
				}
			}
		}
	}
	else // second and other
	{
		int g = gEngfuncs.pfnRandomLong(155, 175);

		Vector dir( args->fparam1, args->fparam2, 0.0f );
		Vector vel( 0.0f, 0.0f, 0.0f );

		EV_CS16Client_CreateSmoke( SMOKE_BLACK, args->origin, dir, args->iparam1 / 100, 1.0f, g, g, g, true, vel, 25 );
	}
}
