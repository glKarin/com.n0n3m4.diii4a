/*
PicButton.h - animated button with picture
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
#include "PicButton.h"
#include "Utils.h"
#include "Scissor.h"
#include "BtnsBMPTable.h"
#include <stdlib.h>
#include "Framework.h"

static int g_hotkeys[PC_BUTTONCOUNT] =
{
	'n', // PC_NEW_GAME = 0,
	'r', // PC_RESUME_GAME,
	'h', // PC_HAZARD_COURSE,
	'c', // PC_CONFIG,
	'l', // PC_LOAD_GAME,
	's', // PC_SAVE_LOAD_GAME,
	'v', // PC_VIEW_README,
	'q', // PC_QUIT,
	'm', // PC_MULTIPLAYER,
	'e', // PC_EASY,
	'm', // PC_MEDIUM,
	'd', // PC_DIFFICULT,
	's', // PC_SAVE_GAME,
	'l', // PC_LOAD_GAME2,
	'c', // PC_CANCEL,
	'g', // PC_GAME_OPTIONS,
	'v', // PC_VIDEO,
	'a', // PC_AUDIO,
	'c', // PC_CONTROLS,
	'd', // PC_DONE,
	'q', // PC_QUICKSTART,
	'u', // PC_USE_DEFAULTS,
	'o', // PC_OK,
	'v', // PC_VID_OPT,
	'm', // PC_VID_MODES,
	'a', // PC_ADV_CONTROLS,
	'o', // PC_ORDER_HL,
	'd', // PC_DELETE,
	'i', // PC_INET_GAME,
	'h', // PC_CHAT_ROOMS,
	'l', // PC_LAN_GAME,
	'u', // PC_CUSTOMIZE,
	's', // PC_SKIP,
	'e', // PC_EXIT,
	'c', // PC_CONNECT,
	'r', // PC_REFRESH,
	'f', // PC_FILTER,
	'f', // PC_FILTER2,
	'c', // PC_CREATE,
	't', // PC_CREATE_GAME,
	'h', // PC_CHAT_ROOMS2,
	'l', // PC_LIST_ROOMS,
	's', // PC_SEARCH,
	's', // PC_SERVERS,
	'j', // PC_JOIN,
	'f', // PC_FIND,
	'r', // PC_CREATE_ROOM,
	'j', // PC_JOIN_GAME,
	's', // PC_SEARCH_GAMES,
	'f', // PC_FIND_GAME,
	't', // PC_START_GAME,
	'v', // PC_VIEW_GAME_INFO,
	'u', // PC_UPDATE,
	'a', // PC_ADD_SERVER,
	'd', // PC_DISCONNECT,
	'o', // PC_CONSOLE,	// a1ba: set to O
	'o', // PC_CONTENT_CONTROL,
	'u', // PC_UPDATE2,
	'w', // PC_VISIT_WON,
	'p', // PC_PREVIEWS,
	'a', // PC_ADV_OPT,
	0, // PC_3DINFO_SITE,
	'u', // PC_CUSTOM_GAME,
	'a', // PC_ACTIVATE,
	'i', // PC_INSTALL,
	'v', // PC_VISIT_WEB_SITE,
	'r', // PC_REFRESH_LIST,
	'e', // PC_DEACTIVATE,
	'a', // PC_ADV_OPT2,
	's', // PC_SPECTATE_GAME,
	'p', // PC_SPECTATE_GAMES,
};

CMenuPicButton::CMenuPicButton() : BaseClass()
{
	bEnableTransitions = true;
	eFocusAnimation = QM_HIGHLIGHTIFFOCUS;
	iFlags = QMF_DROPSHADOW;

	iFocusStartTime = 0;

	eTextAlignment = QM_LEFT;

	hPic = 0;
	hotkey = 0;
	button_id = -1;
	iOldState = BUTTON_NOFOCUS;
	m_iLastFocusTime = -512;
	bPulse = false;

	SetSize( UI_BUTTONS_WIDTH, UI_BUTTONS_HEIGHT );

	SetCharSize( QM_DEFAULTFONT );
}

/*
=================
CMenuPicButton::Key
=================
*/
bool CMenuPicButton::KeyUp( int key )
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

void CMenuPicButton::_Event( int ev )
{
	BaseClass::_Event( ev );
	if( ev == QM_RELEASED )
	{
		PlayLocalSound( uiStatic.sounds[SND_LAUNCH] );
		CheckWindowChanged( );
	}
}

void CMenuPicButton::CheckWindowChanged()
{
	// parent is not a window, ignore
	if( !m_pParent->IsWindow())
		return;

	CMenuBaseWindow *parentWindow = (CMenuBaseWindow*)m_pParent;
	CMenuBaseWindow *newWindow = parentWindow->WindowStack()->Current();

	// menu is closed, ignore
	if( !newWindow )
		return;

	// no change, ignore
	if( parentWindow == newWindow )
		return;

	// parent and new are not a root windows, ignore
	if( !parentWindow->IsRoot() || !newWindow->IsRoot() )
		return;

	// decide transition direction
	if( FBitSet( parentWindow->iFlags, QMF_CLOSING ))
	{
		// our parent window is closing right now
		// play backward animation
		// Con_NPrintf( 10, "%s banner down", parentWindow->szName );

		CMenuFramework *f = (CMenuFramework*)parentWindow;
		f->PrepareBannerAnimation( CMenuFramework::ANIM_CLOSING, nullptr );
	}
	else
	{
		// new window overlaps parent window
		// play forward animation
		// Con_NPrintf( 10, "%s banner up", newWindow->szName );

		CMenuFramework *f = (CMenuFramework*)newWindow;
		f->PrepareBannerAnimation( CMenuFramework::ANIM_OPENING, this );
	}
}

bool CMenuPicButton::KeyDown( int key )
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


// #define ALT_PICBUTTON_FOCUS_ANIM

/*
=================
CMenuPicButton::DrawButton
=================
*/
void CMenuPicButton::DrawButton( int r, int g, int b, int a, wrect_t *rects, int state )
{
	EngFuncs::PIC_Set( hPic, r, g, b, a );
#ifdef ALT_PICBUTTON_FOCUS_ANIM
	UI::PushScissor( m_scPos.x, m_scPos.y, uiStatic.buttons_draw_width * flFill, uiStatic.buttons_draw_height );
#endif
	EngFuncs::PIC_DrawAdditive( m_scPos, uiStatic.buttons_draw_size, &rects[state] );

#ifdef ALT_PICBUTTON_FOCUS_ANIM
	UI::PopScissor();
#endif
}

/*
=================
CMenuPicButton::Draw
=================
*/
void CMenuPicButton::Draw( )
{
	int state = BUTTON_NOFOCUS;

#ifdef CS16CLIENT
	if( UI_CursorInRect( m_scPos, m_scSize ) &&
		m_pParent && m_pParent->IsVisible() )
	{
		if( !bRollOver )
		{
			PlayLocalSound( uiSoundRollOver );
			bRollOver = true;
		}
	}
	else
	{
		if( bRollOver )
			bRollOver = false;
	}
#endif // CS16CLIENT

	if( iFlags & (QMF_HASMOUSEFOCUS|QMF_HASKEYBOARDFOCUS))
	{
		state = BUTTON_FOCUS;
	}

	// make sure what cursor in rect
	if( m_bPressed )
		state = BUTTON_PRESSED;

	if( iOldState == BUTTON_NOFOCUS && state != BUTTON_NOFOCUS )
		iFocusStartTime = uiStatic.realTime;

#ifndef CS16CLIENT
	if( szStatusText && iFlags & QMF_NOTIFY )
	{
		Point coord;

		coord.x = m_scPos.x + 290 * uiStatic.scaleX;
		coord.y = m_scPos.y + m_scSize.h / 2 - EngFuncs::ConsoleCharacterHeight() / 2;

		int	r, g, b;

		UnpackRGB( r, g, b, uiColorHelp );
		EngFuncs::DrawSetTextColor( r, g, b );
		EngFuncs::DrawConsoleString( coord, szStatusText );
	}
#endif

	int a = (512 - (uiStatic.realTime - m_iLastFocusTime)) >> 1;

	if( hPic && !uiStatic.renderPicbuttonText )
	{
		int r, g, b;

		UnpackRGB( r, g, b, iFlags & QMF_GRAYED ? uiColorDkGrey : uiColorWhite );

		wrect_t rects[3];
		for( int i = 0; i < 3; i++ )
		{
			if( button_id > 0 )
			{
				rects[i].top = uiStatic.buttons_points[i];
				rects[i].bottom = uiStatic.buttons_points[i] + uiStatic.buttons_height;
			}
			else
			{
				rects[i].top = 26 * i;
				rects[i].bottom = 26 * ( i + 1 );
			}
			rects[i].left = 0;
			rects[i].right = uiStatic.buttons_width;
		}

		// decay
		if( state == BUTTON_NOFOCUS && a > 0 )
		{
			DrawButton( r, g, b, a, rects, BUTTON_FOCUS );
		}

		// pulse code.
		if( ( state == BUTTON_NOFOCUS && bPulse ) ||
			( state == BUTTON_FOCUS   && eFocusAnimation == QM_PULSEIFFOCUS ) )
		{
			a = 255 * (0.5f + 0.5f * sin( (float)uiStatic.realTime / ( UI_PULSE_DIVISOR * 2 )));

			DrawButton( r, g, b, a, rects, BUTTON_FOCUS );
			DrawButton( r, g, b, 255, rects, BUTTON_NOFOCUS );
		}
		// special handling for focused
		else if( state == BUTTON_FOCUS )
		{
			DrawButton( r, g, b, 255, rects, BUTTON_FOCUS );
			DrawButton( r, g, b, 255, rects, BUTTON_NOFOCUS );
		}
		else
		{
			// just draw
			DrawButton( r, g, b, 255, rects, state );
		}
	}
	else if( !uiStatic.lowmemory )
	{
		const uint heavy_blur_flags = ETF_NOSIZELIMIT | ETF_FORCECOL;
		const uint light_blur_flags = ETF_NOSIZELIMIT | ETF_FORCECOL | ETF_ADDITIVE;
		CColor light_blur_color, heavy_blur_color = colorBase;
		Point pos = m_scPos;

		if( iFlags & QMF_GRAYED )
			light_blur_color = InterpColor( uiColorBlack, colorBase, 0.333f ); // because additive, tone down all channels
		else
			light_blur_color = colorBase;

		pos.x += 7 * uiStatic.scaleX;
		pos.y -= uiStatic.scaleY;

		if( this != m_pParent->ItemAtCursor() )
		{
			if( a > 0 )
			{
				UI_DrawString( uiStatic.hHeavyBlur, pos, m_scSize, szName,
					PackAlpha( heavy_blur_color, a ), m_scChSize, eTextAlignment, heavy_blur_flags );
			}
			UI_DrawString( uiStatic.hLightBlur, pos, m_scSize, szName, light_blur_color, m_scChSize, eTextAlignment, light_blur_flags );
		}
		else if( m_bPressed )
		{
			UI_DrawString( uiStatic.hHeavyBlur, pos, m_scSize, szName, heavy_blur_color, m_scChSize, eTextAlignment, heavy_blur_flags );
			UI_DrawString( uiStatic.hLightBlur, pos, m_scSize, szName, 0xFF000000, m_scChSize, eTextAlignment, light_blur_flags & ( ~ETF_ADDITIVE ));
		}
		else if( eFocusAnimation == QM_HIGHLIGHTIFFOCUS )
		{
			UI_DrawString( uiStatic.hHeavyBlur, pos, m_scSize, szName, heavy_blur_color, m_scChSize, eTextAlignment, heavy_blur_flags );
			UI_DrawString( uiStatic.hLightBlur, pos, m_scSize, szName, light_blur_color, m_scChSize, eTextAlignment, light_blur_flags );
		}
		else if( eFocusAnimation == QM_PULSEIFFOCUS )
		{
			float pulsar = 0.5f + 0.5f * sin( (float)uiStatic.realTime / UI_PULSE_DIVISOR );

			UI_DrawString( uiStatic.hHeavyBlur, pos, m_scSize, szName,
				InterpColor( uiColorBlack, heavy_blur_color, pulsar ), m_scChSize, eTextAlignment, heavy_blur_flags );
			UI_DrawString( uiStatic.hLightBlur, pos, m_scSize, szName, light_blur_color, m_scChSize, eTextAlignment, light_blur_flags );
		}
	}
	else
	{
		uint textflags = ETF_NOSIZELIMIT | ETF_FORCECOL;

		SetBits( textflags, (iFlags & QMF_DROPSHADOW) ? ETF_SHADOW : 0 );

		if( iFlags & QMF_GRAYED )
		{
			UI_DrawString( font, m_scPos, m_scSize, szName, uiColorDkGrey, m_scChSize, eTextAlignment, textflags );
		}
		else if( this != m_pParent->ItemAtCursor() )
		{
			UI_DrawString( font, m_scPos, m_scSize, szName, colorBase, m_scChSize, eTextAlignment, textflags );
		}
		else if( eFocusAnimation == QM_HIGHLIGHTIFFOCUS )
		{
			UI_DrawString( font, m_scPos, m_scSize, szName, colorFocus, m_scChSize, eTextAlignment, textflags );
		}
		else if( eFocusAnimation == QM_PULSEIFFOCUS )
		{
			float pulsar = 0.5f + 0.5f * sin( (float)uiStatic.realTime / UI_PULSE_DIVISOR );

			UI_DrawString( font, m_scPos, m_scSize, szName,
				InterpColor( colorBase, colorFocus, pulsar ), m_scChSize, eTextAlignment, textflags );
		}
	}

	iOldState = state;
}

void CMenuPicButton::SetPicture( EDefaultBtns ID )
{
	if( ID < 0 || ID > PC_BUTTONCOUNT )
		return; // bad id

	hPic = uiStatic.buttonsPics[ID];
	button_id = ID;
	hotkey = g_hotkeys[ID];
}

void CMenuPicButton::SetPicture( const char *filename, int hk )
{
	hPic = EngFuncs::PIC_Load( filename );
	hotkey = hk;
}

bool CMenuPicButton::HotKey( int key )
{
	return hotkey == key;
}
