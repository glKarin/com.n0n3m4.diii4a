/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
#include "MovieBanner.h"

void CMenuMovieBanner::VidInit()
{
	float scaleX = ScreenWidth / 640.0f;
	float scaleY = ScreenHeight / 480.0f;

	m_scPos.x = 0;
	m_scPos.y = 70 * scaleY * uiStatic.scaleY; // 70 is empirically determined value (magic number)

	// a1ba: multiply by height scale to look better on wide screens
	m_scSize.w = EngFuncs::GetLogoWidth() * scaleX;
	m_scSize.h = EngFuncs::GetLogoHeight() * scaleY * uiStatic.scaleY;
}

void CMenuMovieBanner::Draw()
{
	if( EngFuncs::ClientInGame() && EngFuncs::GetCvarFloat( "ui_renderworld" ))
		return;

	if( EngFuncs::GetLogoLength() <= 0.05f || EngFuncs::GetLogoWidth() <= 32 )
		return;

	EngFuncs::DrawLogo( "logo.avi", m_scPos.x, m_scPos.y, m_scSize.w, m_scSize.h );
}
