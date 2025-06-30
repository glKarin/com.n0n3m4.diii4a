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
// flashlight.cpp
//
// implementation of CHudFlashlight class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

#include "draw_util.h"

#define BAT_NAME "sprites/%d_Flashlight.spr"

int CHudFlashlight::Init(void)
{
	m_fFade = 0;
	m_fOn = 0;

	HOOK_MESSAGE(gHUD.m_Flash, Flashlight);
	HOOK_MESSAGE(gHUD.m_Flash, FlashBat);

	m_iFlags |= HUD_DRAW;

	gHUD.AddHudElem(this);

	return 1;
}

void CHudFlashlight::Reset(void)
{
	m_fFade = 0;
	m_fOn = 0;
}

int CHudFlashlight::VidInit(void)
{
	m_hSprite1.SetSpriteByName("flash_empty");
	m_hSprite2.SetSpriteByName("flash_full");
	m_hBeam.SetSpriteByName("flash_beam");
	m_iWidth = m_hSprite1.rect.Width();

	return 1;
}

int CHudFlashlight:: MsgFunc_FlashBat(const char *pszName,  int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	int x = reader.ReadByte();
	m_iBat = x;
	m_flBat = ((float)x)/100.0;

	return 1;
}

int CHudFlashlight:: MsgFunc_Flashlight(const char *pszName,  int iSize, void *pbuf )
{
	BufferReader reader( pszName, pbuf, iSize );
	m_fOn = reader.ReadByte();
	int x = reader.ReadByte();
	m_iBat = x;
	m_flBat = ((float)x)/100.0;

	return 1;
}

int CHudFlashlight::Draw(float flTime)
{
	if ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_FLASHLIGHT | HIDEHUD_ALL ) )
		return 1;

	int r, g, b, x, y, a;
	wrect_t rc;

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return 1;

	if (m_fOn)
		a = 225;
	else
		a = MIN_ALPHA;

	if (m_flBat < 0.20)
		DrawUtils::UnpackRGB(r,g,b, RGB_REDISH);
	else
		DrawUtils::UnpackRGB( r, g, b, gHUD.m_iDefaultHUDColor );

	DrawUtils::ScaleColors(r, g, b, a);

	y = (m_hSprite1.rect.Height())/2;
	x = ScreenWidth - m_iWidth - m_iWidth/2 ;

	// Draw the flashlight casing
	SPR_Set(m_hSprite1.spr, r, g, b );
	SPR_DrawAdditive( 0,  x, y, &m_hSprite1.rect);

	if ( m_fOn )
	{  // draw the flashlight beam
		x = ScreenWidth - m_iWidth/2;

		SPR_Set( m_hBeam.spr, r, g, b );
		SPR_DrawAdditive( 0, x, y, &m_hBeam.rect );
	}

	// draw the flashlight energy level
	x = ScreenWidth - m_iWidth - m_iWidth/2 ;
	int iOffset = m_iWidth * (1.0 - m_flBat);
	if (iOffset < m_iWidth)
	{
		rc = m_hSprite2.rect;
		rc.left += iOffset;

		SPR_Set(m_hSprite2.spr, r, g, b );
		SPR_DrawAdditive( 0, x + iOffset, y, &rc);
	}


	return 1;
}
