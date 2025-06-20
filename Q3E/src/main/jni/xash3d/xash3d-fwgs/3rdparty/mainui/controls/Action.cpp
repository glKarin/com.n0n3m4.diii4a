/*
Action.cpp - simple label with background item
Copyright (C) 2010 Uncle Mike
Copyright (C) 2017 a1batross

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
#include "Action.h"
#include "Utils.h"

CMenuAction::CMenuAction() : BaseClass()
{
	m_szBackground = 0;
	m_bfillBackground = false;
	forceCalcW = forceCalcY = false;
	bIgnoreColorstring = false;
}

/*
=================
CMenuAction::Init
=================
*/
void CMenuAction::VidInit( )
{
	m_iBackcolor.SetDefault( 0 );

	if( !forceCalcW )
		forceCalcW = size.w < 1;

	if( !forceCalcY )
		forceCalcY = size.h < 1;

	if( forceCalcW || forceCalcY )
	{
		if( m_szBackground )
		{
			size.w = EngFuncs::PIC_Width( m_szBackground );
			size.h = EngFuncs::PIC_Height( m_szBackground );
		}
		else
		{
			if( forceCalcW )
				size.w = g_FontMgr->GetTextWideScaled( font, szName, charSize ) / uiStatic.scaleX;

			if( forceCalcY )
				size.h = g_FontMgr->GetTextHeightExt( font, szName, charSize, size.w ) / uiStatic.scaleX;
		}

		m_bLimitBySize = false;
	}
	else
	{
		m_bLimitBySize = true;
	}

	BaseClass::VidInit();
}

/*
=================
CMenuAction::Key
=================
*/
bool CMenuAction::KeyUp( int key )
{
	const char *sound = 0;

	if( UI::Key::IsEnter( key ) && !(iFlags & QMF_MOUSEONLY) )
		sound = uiStatic.sounds[SND_LAUNCH];
	else if( UI::Key::IsLeftMouse( key ) && ( iFlags & QMF_HASMOUSEFOCUS ) )
		sound = uiStatic.sounds[SND_LAUNCH];

	if( sound )
	{
		_Event( QM_PRESSED );
		PlayLocalSound( sound );
	}

	return sound != NULL;
}

bool CMenuAction::KeyDown( int key )
{
	bool handled = false;

	if( UI::Key::IsEnter( key ) && !(iFlags & QMF_MOUSEONLY) )
		handled = true;
	else if( UI::Key::IsLeftMouse( key ) && ( iFlags & QMF_HASMOUSEFOCUS ) )
		handled = true;

	if( handled )
		_Event( QM_PRESSED );

	return handled;
}

/*
=================
CMenuAction::Draw
=================
*/
void CMenuAction::Draw( )
{
	uint textflags = 0;

	if( FBitSet( iFlags, QMF_DROPSHADOW ))
		SetBits( textflags, ETF_SHADOW );

	if( !m_bLimitBySize )
		SetBits( textflags, ETF_NOSIZELIMIT );

	if( bIgnoreColorstring )
		SetBits( textflags, ETF_FORCECOL );

	if( bDrawStroke )
		UI_DrawRectangleExt( m_scPos, m_scSize, colorStroke, iStrokeWidth );

	if( m_szBackground )
	{
		UI_DrawPic( m_scPos, m_scSize, m_iBackcolor, m_szBackground );
	}
	else if( m_bfillBackground )
	{
		if( this != m_pParent->ItemAtCursor() || iFlags & QMF_GRAYED )
		{
			UI_FillRect( m_scPos, m_scSize, m_iBackcolor );
		}
		else
		{
			UI_FillRect( m_scPos, m_scSize, m_iBackColorFocused );
		}
	}

	if( szStatusText && iFlags & QMF_NOTIFY )
	{
		Point coord;

		coord.x = m_scPos.x + 16 * uiStatic.scaleX;
		coord.y = m_scPos.y + m_scSize.h / 2 - EngFuncs::ConsoleCharacterHeight() / 2;

		int	r, g, b;

		UnpackRGB( r, g, b, uiColorHelp );
		EngFuncs::DrawSetTextColor( r, g, b );
		EngFuncs::DrawConsoleString( coord, szStatusText );
	}

	if( iFlags & QMF_GRAYED )
	{
		UI_DrawString( font, m_scPos, m_scSize, szName, uiColorDkGrey, m_scChSize, eTextAlignment, textflags | ETF_FORCECOL );
		return; // grayed
	}

	if( this != m_pParent->ItemAtCursor() || eFocusAnimation == QM_NOFOCUSANIMATION )
	{
		UI_DrawString( font, m_scPos, m_scSize, szName, colorBase, m_scChSize, eTextAlignment, textflags );
		return; // no focus
	}

	if( eFocusAnimation == QM_HIGHLIGHTIFFOCUS )
	{
		UI_DrawString( font, m_scPos, m_scSize, szName, colorFocus, m_scChSize, eTextAlignment, textflags );
	}
	else if( eFocusAnimation == QM_PULSEIFFOCUS )
	{
		int	color;

		color = PackAlpha( colorBase, 255 * (0.5f + 0.5f * sin( (float)uiStatic.realTime / UI_PULSE_DIVISOR )));

		UI_DrawString( font, m_scPos, m_scSize, szName, color, m_scChSize, eTextAlignment, textflags );
	}
}

void CMenuAction::SetBackground(const char *path, unsigned int color)
{
	m_szBackground = path;
	m_iBackcolor = color;
	m_bfillBackground = false;
}

void CMenuAction::SetBackground(unsigned int color, unsigned int focused )
{
	m_bfillBackground = true;
	m_szBackground = 0;
	m_iBackcolor = color;
	m_iBackColorFocused = focused;
}
