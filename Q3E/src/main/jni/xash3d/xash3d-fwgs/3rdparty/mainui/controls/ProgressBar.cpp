/*
ProgressBar.cpp -- progress bar
Copyright (C) 2017 mittorn

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "extdll_menu.h"
#include "BaseMenu.h"
#include "ProgressBar.h"

CMenuProgressBar::CMenuProgressBar() : BaseClass()
{
	m_flMin = 0.0f;
	m_flMax = 100.0f;
	m_flValue = 0.0f;
	m_szCvarName = NULL;
}

void CMenuProgressBar::LinkCvar( const char *cvName, float flMin, float flMax )
{
	m_szCvarName = cvName;

	m_flMax = flMax;
	m_flMin = flMin;
}

void CMenuProgressBar::SetValue( float flValue )
{
	if( flValue > 1.0f ) flValue = 1;
	if( flValue < 0.0f ) flValue = 0;
	m_flValue = flValue;
	m_szCvarName = NULL;
}

void CMenuProgressBar::Draw( void )
{
	float flProgress;

	if( m_szCvarName )
	{
		flProgress = bound( m_flMin, EngFuncs::GetCvarFloat( m_szCvarName ), m_flMax );
		flProgress = ( flProgress - m_flMin ) / ( m_flMax - m_flMin );
	}
	else
	{
		flProgress = m_flValue;
	}

	// draw the background
	UI_FillRect( m_scPos, m_scSize, uiInputBgColor );

	UI_FillRect( m_scPos.x, m_scPos.y, m_scSize.w * flProgress, m_scSize.h, colorBase );

	// draw the rectangle
	UI_DrawRectangle( m_scPos, m_scSize, uiInputFgColor );
}
