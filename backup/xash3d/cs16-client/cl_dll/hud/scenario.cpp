// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
/*
scenario.cpp -- CSCZ Scenario HUD implementation
Copyright (C) 2017 a1batross
This program is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by the
Free Software Foundation; either version 2 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

In addition, as a special exception, the author gives permission to
link the code of this program with the Half-Life Game Engine ("HL
Engine") and Modified Game Libraries ("MODs") developed by Valve,
L.L.C ("Valve").  You must obey the GNU General Public License in all
respects for all of the code used other than the HL Engine and MODs
from Valve.  If you modify this file, you may extend this exception
to your version of the file, but you are not obligated to do so.  If
you do not wish to do so, delete this exception statement from your
version.
*/

#include "hud.h"
#include "parsemsg.h"
#include "cl_util.h"
#include "r_efx.h"
#include "event_api.h"
#include "com_model.h"
#include "draw_util.h"
#include <string.h>

int CHudScenario::Init( )
{
	HOOK_MESSAGE( gHUD.m_Scenario, Scenario );
	m_iFlags = 0;
	gHUD.AddHudElem( this );

	return 0;
}

int CHudScenario::VidInit( )
{
	return 0;
}

void CHudScenario::Reset()
{
	m_sprite.spr = 0;
}

int CHudScenario::Draw(float flTime)
{
	if( gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH || g_iUser1 || gEngfuncs.IsSpectateOnly() || !m_sprite.spr )
		return 1;

	if (!(gHUD.m_iWeaponBits & (1<<(WEAPON_SUIT)) ))
		return 1;

	int r, g, b;
	int x, y;
	DrawUtils::UnpackRGB( r, g, b, gHUD.m_iDefaultHUDColor );

	if( m_fFlashRate != 0.0f && m_fNextFlash < flTime )
	{
		m_iAlpha = m_iFlashAlpha;
		m_fNextFlash = flTime + m_fFlashRate;
	}

	if ( m_iAlpha > MIN_ALPHA ) m_iAlpha -= 20;
	else m_iAlpha = MIN_ALPHA;

	DrawUtils::ScaleColors( r, g, b, m_iAlpha );

	x = gHUD.m_Timer.m_right + gHUD.m_iFontWidth * 1.5;
	y = ScreenHeight - 1.5 * gHUD.m_iFontHeight - ( m_sprite.rect.Height() - gHUD.m_iFontHeight ) / 2 ;

	SPR_Set( m_sprite.spr, r, g, b );
	SPR_DrawAdditive( 0, x, y, &m_sprite.rect );
	return 1;
}

int CHudScenario::MsgFunc_Scenario(const char *pszName, int iSize, void *buf)
{
	BufferReader reader( pszName, buf, iSize );

	bool active = reader.ReadByte();
	if( !active )
	{
		m_iFlags = 0;
		return 1;
	}

	m_iFlags = HUD_DRAW;

	const char *spriteName = reader.ReadString();
	m_sprite.SetSpriteByName( spriteName );

	m_iFlashAlpha = reader.ReadByte();
	m_fFlashRate = reader.ReadShort() * 0.01;
	m_fNextFlash = reader.ReadShort() * 0.01 + gHUD.m_flTime;

	return 1;
}
