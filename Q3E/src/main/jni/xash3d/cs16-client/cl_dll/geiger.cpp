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
// Geiger.cpp
//
// implementation of CHudAmmo class
//

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <time.h>
#include <stdio.h>

#include "parsemsg.h"

int CHudGeiger::Init(void)
{
	HOOK_MESSAGE( gHUD.m_Geiger, Geiger );

	m_iGeigerRange = 0;
	m_iFlags = 0;

	gHUD.AddHudElem(this);

	srand( (unsigned)time( NULL ) );

	return 1;
}

int CHudGeiger::VidInit(void)
{
	return 1;
}

int CHudGeiger::MsgFunc_Geiger(const char *pszName,  int iSize, void *pbuf)
{

	BufferReader reader( pszName, pbuf, iSize );

	// update geiger data
	m_iGeigerRange = reader.ReadByte() << 2;

	if( m_iGeigerRange < 0 || m_iGeigerRange > 1000 )
		m_iFlags &= ~HUD_DRAW;
	else
		m_iFlags |= HUD_DRAW;

	return 1;
}

int CHudGeiger::Draw (float flTime)
{
	int pct, i;
	float flvol;
	
	if (m_iGeigerRange < 1000 && m_iGeigerRange > 0)
	{
		// peicewise linear is better than continuous formula for this
		if (m_iGeigerRange > 800)
		{
			pct = 0;			//Con_Printf ( "range > 800\n");
			i = 0;
		}
		else if (m_iGeigerRange > 600)
		{
			pct = 2;
			//flvol = 0.4;		//Con_Printf ( "range > 600\n");
			i = 2;
		}
		else if (m_iGeigerRange > 500)
		{
			pct = 4;
			//flvol = 0.5;		//Con_Printf ( "range > 500\n");
			i = 2;
		}
		else if (m_iGeigerRange > 400)
		{
			pct = 8;
			//flvol = 0.6;		//Con_Printf ( "range > 400\n");
			i = 3;
		}
		else if (m_iGeigerRange > 300)
		{
			pct = 8;
			//flvol = 0.7;		//Con_Printf ( "range > 300\n");
			i = 3;
		}
		else if (m_iGeigerRange > 200)
		{
			pct = 28;
			//flvol = 0.78;		//Con_Printf ( "range > 200\n");
			i = 3;
		}
		else if (m_iGeigerRange > 150)
		{
			pct = 40;
			//flvol = 0.80;		//Con_Printf ( "range > 150\n");
			i = 3;
		}
		else if (m_iGeigerRange > 100)
		{
			pct = 60;
			//flvol = 0.85;		//Con_Printf ( "range > 100\n");
			i = 3;
		}
		else if (m_iGeigerRange > 75)
		{
			pct = 80;
			//flvol = 0.9;		//Con_Printf ( "range > 75\n");
			//gflGeigerDelay = cl.time + GEIGERDELAY * 0.75;
			i = 3;
		}
		else if (m_iGeigerRange > 50)
		{
			pct = 90;
			//flvol = 0.95;		//Con_Printf ( "range > 50\n");
			i = 2;
		}
		else
		{
			pct = 95;
			//flvol = 1.0;		//Con_Printf ( "range < 50\n");
			i = 2;
		}

		flvol = Com_RandomFloat(0.25, 25);

		if( pct && (rand() & 127) < pct )
		{
			//S_StartDynamicSound (-1, 0, rgsfx[rand() % i], r_origin, flvol, 1.0, 0, 100);	
			char sz[256];
			
			int j = rand() & 1;
			if( i > 2 )
				j += rand() & 1;

			sprintf( sz, "player/geiger%d.wav", j + 1 );
			PlaySound( sz, flvol );
			
		}
	}

	return 1;
}
