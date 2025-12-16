/*
Bitmap.cpp - bitmap menu item
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
#include "Bitmap.h"
#include "PicButton.h" // GetTitleTransFraction
#include "Utils.h"
#include "BaseWindow.h"

CMenuBitmap::CMenuBitmap() : BaseClass()
{
	SetRenderMode( QM_DRAWNORMAL );
}

/*
=================
CMenuBitmap::Init
=================
*/
void CMenuBitmap::VidInit( )
{
	colorBase.SetDefault( uiColorWhite );
	colorFocus.SetDefault( uiColorWhite );

	BaseClass::VidInit();
	if( !szFocusPic )
		szFocusPic = szPic;
}

bool CMenuBitmap::KeyUp( int key )
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

bool CMenuBitmap::KeyDown( int key )
{
	bool handled = false;

	if( UI::Key::IsEnter( key ) && !(iFlags & QMF_MOUSEONLY) )
		handled = true;
	else if( UI::Key::IsLeftMouse( key ) && ( iFlags & QMF_HASMOUSEFOCUS ) )
		handled = true;

	if( handled )
		_Event( QM_RELEASED );

	return handled;
}

/*
=================
CMenuBitmap::Draw
=================
*/
void CMenuBitmap::Draw( void )
{
	if( !szPic )
	{
		UI_FillRect( m_scPos, m_scSize, colorBase );
		return;
	}

	if( iFlags & QMF_GRAYED )
	{
		UI_DrawPic( m_scPos, m_scSize, uiColorDkGrey, szPic, eRenderMode );
		return; // grayed
	}

	if(( iFlags & QMF_MOUSEONLY ) && !( iFlags & QMF_HASMOUSEFOCUS ))
	{
		UI_DrawPic( m_scPos, m_scSize, colorBase, szPic, eRenderMode );
		return; // no focus
	}

	if( this != m_pParent->ItemAtCursor() )
	{
		UI_DrawPic( m_scPos, m_scSize, colorBase, szPic, eRenderMode );
		return; // no focus
	}

	if( this->m_bPressed )
	{
		UI_DrawPic( m_scPos, m_scSize, colorBase, szPressPic, ePressRenderMode );
	}

	switch( eFocusAnimation )
	{
	case QM_HIGHLIGHTIFFOCUS:
		UI_DrawPic( m_scPos, m_scSize, colorBase, szFocusPic, eFocusRenderMode );
		break;
	case QM_PULSEIFFOCUS:
	{
		int	color;

		color = PackAlpha( colorBase, 255 * (0.5f + 0.5f * sin( (float)uiStatic.realTime / UI_PULSE_DIVISOR )));
		UI_DrawPic( m_scPos, m_scSize, color, szFocusPic, eFocusRenderMode );
		break;
	}
	default:
		UI_DrawPic( m_scPos, m_scSize, colorBase, szPic, eRenderMode ); // ignore focus
		break;
	}
}
