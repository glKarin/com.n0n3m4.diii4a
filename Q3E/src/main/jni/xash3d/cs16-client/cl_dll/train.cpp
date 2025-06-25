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
// Train.cpp
//
// implementation of CHudAmmo class
//

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"
#include "draw_util.h"

int CHudTrain::Init(void)
{
	HOOK_MESSAGE( gHUD.m_Train, Train );

	m_iPos = 0;
	m_iFlags = 0;
	gHUD.AddHudElem(this);

	return 1;
}

int CHudTrain::VidInit(void)
{
	m_hSprite = 0;

	return 1;
}

int CHudTrain::Draw(float fTime)
{
	if ( !m_hSprite )
		m_hSprite = LoadSprite("sprites/%d_train.spr");

	if (m_iPos)
	{
		int r, g, b, x, y;

		DrawUtils::UnpackRGB( r, g, b, gHUD.m_iDefaultHUDColor );
		SPR_Set(m_hSprite, r, g, b );

		// This should show up to the right and part way up the armor number
		y = ScreenHeight - SPR_Height(m_hSprite,0) - gHUD.m_iFontHeight;
		x = ScreenWidth/3 + SPR_Width(m_hSprite,0)/4;

		SPR_DrawAdditive( m_iPos - 1,  x, y, NULL);

	}

	return 1;
}


int CHudTrain::MsgFunc_Train(const char *pszName,  int iSize, void *pbuf)
{
	BufferReader reader( pszName, pbuf, iSize );

	// update Train data
	m_iPos = reader.ReadByte();

	if (m_iPos)
		m_iFlags |= HUD_DRAW;
	else
		m_iFlags &= ~HUD_DRAW;

	return 1;
}
