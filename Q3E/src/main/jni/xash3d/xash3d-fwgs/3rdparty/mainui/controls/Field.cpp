/*
Field.h - edit field
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
#include "Field.h"
#include "Utils.h"


CMenuField::CMenuField() : BaseClass()
{
	bAllowColorstrings = true;
	bHideInput = false;
	bNumbersOnly = false;

	iFlags |= QMF_DROPSHADOW;
	eTextAlignment = QM_CENTER;

	SetSize( 200, 32 );

	iMaxLength = 0;
	iCursor = 0;
	iScroll = 0;
	iRealWidth = 0;
	szBackground = 0;
	szBuffer[0] = 0;
}

void CMenuField::Init()
{
	//Clear();
	iMaxLength++;
	if( iMaxLength <= 1 || iMaxLength >= UI_MAX_FIELD_LINE )
		iMaxLength = UI_MAX_FIELD_LINE - 1;
}

/*
=================
CMenuField::Init
=================
*/
void CMenuField::VidInit( void )
{
	BaseClass::VidInit();

	iCursor = strlen( szBuffer );
	iScroll = g_FontMgr->CutText( font, szBuffer, m_scChSize, iRealWidth, true );

	iRealWidth = m_scSize.w - UI_OUTLINE_WIDTH * 2;
}

/*
================
CMenuField::_Event
================
*/
void CMenuField::_Event( int ev )
{
	switch( ev )
	{
	case QM_LOSTFOCUS:
		UI_EnableTextInput( false );
		VidInit();
		break;
	case QM_GOTFOCUS:
		UI_EnableTextInput( true );
		break;
	case QM_IMRESIZED:
	{
		int originalY = 0;

		if( iFlags & QMF_DISABLESCAILING )
			originalY = pos.y;
		else
			originalY = pos.Scale().y;

		if( m_pParent && !IsAbsolutePositioned() )
			originalY += m_pParent->GetRenderPosition().y;

		if( originalY > gpGlobals->scrHeight - 100 * uiStatic.scaleY )
			m_scPos.y = gpGlobals->scrHeight - 100 * uiStatic.scaleY;
		else
			VidInit();
	}
		break;
	}

	CMenuBaseItem::_Event( ev );
}

/*
================
CMenuField::Paste
================
*/
void CMenuField::Paste( void )
{
	char	*str;
	int	pasteLen, i;

	str = EngFuncs::GetClipboardData ();
	if( !str ) return;

	// send as if typed, so insert / overstrike works properly
	pasteLen = strlen( str );
	for( i = 0; i < pasteLen; i++ )
		Char( str[i] );
}

/*
================
CMenuField::Clear
================
*/
void CMenuField::Clear( void )
{
	memset( szBuffer, 0, UI_MAX_FIELD_LINE );
	iCursor = 0;
	iScroll = 0;
}

/*
=================
CMenuField::Key
=================
*/
bool CMenuField::KeyDown( int key )
{
	bool handled = false;

	// clipboard paste
	if( UI::Key::IsInsert( key ) && EngFuncs::KEY_IsDown( K_SHIFT ))
	{
		Paste();
		handled = true;
	}
	else
	{
		int len = strlen( szBuffer );
		handled = true; // predict state
		if( UI::Key::IsInsert( key ))
		{
			EngFuncs::KEY_SetOverstrike( !EngFuncs::KEY_GetOverstrike( ));
		}
		else if( UI::Key::IsLeftArrow( key ))
		{
			if( iCursor > 0 ) iCursor = EngFuncs::UtfMoveLeft( szBuffer, iCursor );
			if( iCursor < iScroll ) iScroll = EngFuncs::UtfMoveLeft( szBuffer, iScroll );
		}
		else if( UI::Key::IsRightArrow( key ))
		{
			bool remaining;

			int maxIdx = g_FontMgr->CutText( font, szBuffer + iScroll, m_scChSize, iRealWidth, false, false, NULL, &remaining );

			if( iCursor < len ) iCursor = EngFuncs::UtfMoveRight( szBuffer, iCursor, len );
			if( remaining && iCursor > maxIdx ) iScroll = EngFuncs::UtfMoveRight( szBuffer, iScroll, len );
		}
		else if( UI::Key::IsHome( key ))
		{
			iCursor = iScroll = 0;
		}
		else if( UI::Key::IsEnd( key ))
		{
			iCursor = len;
			iScroll = g_FontMgr->CutText( font, szBuffer, m_scChSize, iRealWidth, true );
		}
		else if( UI::Key::IsBackspace( key ))
		{
			if( iCursor > 0 )
			{
				int pos = EngFuncs::UtfMoveLeft( szBuffer, iCursor );
				memmove( szBuffer + pos, szBuffer + iCursor, len - iCursor + 1 );
				iCursor = pos;
				if( iScroll )
					iScroll = EngFuncs::UtfMoveLeft( szBuffer, iScroll );
			}
		}
		else if( UI::Key::IsDelete( key, true ))
		{
			if( iCursor < len )
			{
				int pos = EngFuncs::UtfMoveRight( szBuffer, iCursor, len );
				memmove( szBuffer + iCursor, szBuffer + pos, len - pos + 1 );

				iScroll = g_FontMgr->CutText( font, szBuffer, m_scChSize, iRealWidth, true );
			}
		}
		else if( UI::Key::IsLeftMouse( key ))
		{
			float y = m_scPos.y;

			if( y > ScreenHeight - size.h - 40 )
				y = ScreenHeight - size.h - 15;

			if( UI_CursorInRect( m_scPos.x, y, m_scSize.w, m_scSize.h ) )
			{
				int x, charpos;
				int w = 0;
				bool remaining;
				int newScroll = iScroll;

				int iWidthInChars = g_FontMgr->CutText( font, szBuffer + iScroll, m_scChSize, iRealWidth, false, false, &w, &remaining );

				if( eTextAlignment & QM_LEFT )
				{
					x = m_scPos.x;
				}
				else if( eTextAlignment & QM_RIGHT )
				{
					x = m_scPos.x + (m_scSize.w - w);
					if( remaining )
					{
						// add extra space for left char
						if( newScroll > 0 )
							newScroll--;
						if( iWidthInChars > 0 )
							iWidthInChars--;
					}
				}
				else
				{
					x = m_scPos.x + (m_scSize.w - w) / 2;
				}
				charpos = g_FontMgr->CutText(font, szBuffer + newScroll, m_scChSize, uiStatic.cursorX - x, false, false, &w, &remaining );

				iCursor = charpos + iScroll;
				if( iCursor > 0 )
				{
					iCursor = EngFuncs::UtfMoveLeft( szBuffer, iCursor );
					iCursor = EngFuncs::UtfMoveRight( szBuffer, iCursor, len );
				}
				if( charpos == 0 && iScroll )
					iScroll = EngFuncs::UtfMoveLeft( szBuffer, iScroll );
				if( charpos >= iWidthInChars && remaining )
					iScroll = EngFuncs::UtfMoveRight( szBuffer, iScroll, len );
				if( iScroll > len )
					iScroll = len;
				if( iCursor > len )
					iCursor = len;
			}
		}
		else handled = false;
	}

	if( handled )
	{
		SetCvarString( szBuffer );
		_Event( QM_CHANGED );
	}
	return handled; // handled
}

/*
=================
CMenuField::Char
=================
*/
void CMenuField::Char( int key )
{
	int	len;
	bool changed = false;

	if( key == 'v' - 'a' + 1 )
	{
		// ctrl-v is paste
		Paste();
		changed = true;
	}
	else if( key == 'c' - 'a' + 1 )
	{
		// ctrl-c clears the field
		Clear( );
		changed = true;
	}

	len = strlen( szBuffer );

	if( key == 'a' - 'a' + 1 )
	{
		// ctrl-a is home
		iCursor = 0;
		iScroll = 0;
	}
	else if( key == 'e' - 'a' + 1 )
	{
		// ctrl-e is end
		iCursor = len;
		iScroll = g_FontMgr->CutText( font, szBuffer, m_scChSize, iRealWidth, true );
	}
	else if( key == '^' && !( bAllowColorstrings ))
	{
		// ignore color key-symbol
		return;
	}
	else if( bNumbersOnly )
	{
		if( key < '0' || key > '9' )
			return;
	}
	else if( key < 32 )	// non-printable
	{
		return;
	}

	if( eLetterCase == QM_LOWERCASE )
		key = tolower( key );
	else if( eLetterCase == QM_UPPERCASE )
		key = toupper( key );

	if( EngFuncs::KEY_GetOverstrike( ) && !m_bOverrideOverstrike )
	{
		if( iCursor == iMaxLength - 1 ) return;

		// in case a character with X bytes replaced by character with Y bytes
		// where Y < X, e.g. russian replaced by latin
		int pos = EngFuncs::UtfMoveRight( szBuffer, iCursor, len );
		if( pos != iCursor + 1 )
			memmove( szBuffer + iCursor + 1, szBuffer + pos, len - pos + 1 );

		// in case a character with X bytes replaced by character with Y bytes
		// where Y > X, e.g. latin replaced by russian
		// m_bOverrideOverstrike = EngFuncs::UtfProcessChar( key ) == false;
		// TODO ???

		szBuffer[iCursor] = key;
		iCursor++;
		changed = true;
	}
	else
	{
		// insert mode
		if( len == iMaxLength - 1 ) return; // all full
		memmove( szBuffer + iCursor + 1, szBuffer + iCursor, len + 1 - iCursor );
		szBuffer[iCursor] = key;
		iCursor++;
		changed = true;
	}

	if( iCursor > len )
	{
		szBuffer[iCursor] = 0;
		iScroll = g_FontMgr->CutText( font, szBuffer, m_scChSize, iRealWidth, true );
		changed = true;
	}

	SetCvarString( szBuffer );
	_Event( QM_CHANGED );
}

/*
=================
CMenuField::Draw
=================
*/
void CMenuField::Draw( void )
{
	char	text[UI_MAX_FIELD_LINE];
	int	len, drawLen, prestep;
	int	cursor, x, textHeight;
	char	cursor_char[3];
	float y = m_scPos.y;
	uint textflags = ( iFlags & QMF_DROPSHADOW ) ? ETF_SHADOW : 0;
	Point newPos = m_scPos;

	textflags |= ETF_NOSIZELIMIT;

	if( szStatusText && iFlags & QMF_NOTIFY )
	{
		int	x;

		x = m_scPos.x + m_scSize.w + 16 * uiStatic.scaleX;

		int	r, g, b;

		UnpackRGB( r, g, b, uiColorHelp );
		EngFuncs::DrawSetTextColor( r, g, b );
		EngFuncs::DrawConsoleString( x, m_scPos.y, szStatusText );
	}

	if( newPos.y > ScreenHeight - m_scSize.h - 40 )
	{
		if(iFlags & QMF_HASKEYBOARDFOCUS)
			newPos.y = ScreenHeight - m_scSize.h - 15;
		else
			return;
	}

	cursor_char[1] = '\0';
	if( EngFuncs::KEY_GetOverstrike( ))
		cursor_char[0] = 11;
	else cursor_char[0] = '_';

	drawLen = g_FontMgr->CutText( font, szBuffer + iScroll, m_scChSize, m_scSize.w, false );
	len = strlen( szBuffer ) + 1;

	// guarantee that cursor will be visible
	if( len <= drawLen )
	{
		prestep = 0;
	}
	else
	{
		if( iScroll + drawLen > len )
		{
			iScroll = len - drawLen;
			if( iScroll < 0 ) iScroll = 0;
		}
		prestep = iScroll;
	}

	if( prestep + drawLen > len )
		drawLen = len - prestep;

	// extract <drawLen> characters from the field at <prestep>
	if( drawLen >= UI_MAX_FIELD_LINE )
		Host_Error( "CMenuField::Draw: drawLen >= UI_MAX_FIELD_LINE\n" );

	if( bHideInput )
	{
		EngFuncs::UtfProcessChar( 0 );

		const char *sz = szBuffer + prestep;
		int i, j;
		for( i = 0, j = 0; i < drawLen; i++ )
		{
			int uch = EngFuncs::UtfProcessChar( (unsigned char)sz[i] );
			if( uch )
				text[j++] = '*';
		}
		text[j] = 0;

		EngFuncs::UtfProcessChar( 0 );
	}
	else
	{
		memcpy( text, szBuffer + prestep, drawLen );
		text[drawLen] = 0;
	}

	// find cursor position
	x = drawLen - (ColorStrlen( text ) + 1 );
	if( x < 0 ) x = 0;
	cursor = ( iCursor - prestep );
	if( cursor < 0 ) cursor = 0;

	if( szBackground )
	{
		UI_DrawPic( newPos, m_scSize, uiColorWhite, szBackground );
	}
	else
	{
		// draw the background
		UI_FillRect( newPos, m_scSize, uiInputBgColor );

		// draw the rectangle
		UI_DrawRectangle( newPos, m_scSize, uiInputFgColor );
	}

	textHeight = y - (m_scChSize * 1.5f);
	UI_DrawString( font, m_scPos.x, textHeight, m_scSize.w, m_scChSize, szName, uiColorHelp, m_scChSize, QM_LEFT, textflags | ETF_FORCECOL );

	if( iFlags & QMF_GRAYED )
	{
		UI_DrawString( font, newPos, m_scSize, text, uiColorDkGrey, m_scChSize, eTextAlignment, textflags | ETF_FORCECOL );
		return; // grayed
	}

	if(this != m_pParent->ItemAtCursor())
	{
		UI_DrawString( font, newPos, m_scSize, text, colorBase, m_scChSize, eTextAlignment, textflags );
		return; // no focus
	}

	if( eTextAlignment & QM_LEFT )
	{
		x = newPos.x;
	}
	else if( eTextAlignment & QM_RIGHT )
	{
		x = newPos.x + (m_scSize.w - g_FontMgr->GetTextWideScaled( font, text, m_scChSize ) );
	}
	else
	{
		x = newPos.x + (m_scSize.w - g_FontMgr->GetTextWideScaled( font, text, m_scChSize )) / 2;
	}

	UI_DrawString( font, newPos, m_scSize, text, colorBase, m_scChSize, eTextAlignment, textflags );

	int cursorOffset = cursor? g_FontMgr->GetTextWideScaled( font, text, m_scChSize, cursor ):0;

	// int cursorOffset = 0;

	int cursor_char_width = g_FontMgr->GetTextWideScaled( font, cursor_char, m_scChSize );

	if(( uiStatic.realTime & 499 ) < 250 )
		UI_DrawString( font, x + cursorOffset, y, cursor_char_width, m_scSize.h, cursor_char, colorBase, m_scChSize, QM_LEFT, textflags | ETF_FORCECOL );


	switch( eFocusAnimation )
	{
	case QM_HIGHLIGHTIFFOCUS:
		UI_DrawString( font, newPos, m_scSize, text, colorFocus, m_scChSize, eTextAlignment, textflags );

		if(( uiStatic.realTime & 499 ) < 250 )
			UI_DrawString( font, x + cursorOffset, y, cursor_char_width, m_scSize.h, cursor_char, colorFocus, m_scChSize, QM_LEFT, textflags | ETF_FORCECOL  );
		break;
	case QM_PULSEIFFOCUS:
	{
		uint	color;

		color = PackAlpha( colorBase, 255 * (0.5f + 0.5f * sin( (float)uiStatic.realTime / UI_PULSE_DIVISOR )));
		UI_DrawString( font, newPos, m_scSize, text, color, m_scChSize, eTextAlignment, textflags );

		if(( uiStatic.realTime & 499 ) < 250 )
			UI_DrawString( font, x + cursorOffset, y, cursor_char_width, m_scSize.h, cursor_char, color, m_scChSize, QM_LEFT, textflags | ETF_FORCECOL );

		break;
	}
	default: break;
	}
}

void CMenuField::UpdateEditable()
{
	const char *szValue = EngFuncs::GetCvarString( m_szCvarName );

	if( szValue )
	{
		Q_strncpy( szBuffer, szValue, iMaxLength + 1 );
	}
}

bool CMenuField::KeyValueData(const char *key, const char *data)
{
	if( !strcmp( key, "maxchars" ) )
	{
		iMaxLength = atoi( data );
	}
	else if( !strcmp( key, "NumericInputOnly" ) )
	{
		bNumbersOnly = (bool) atoi( data );
	}
	else if( !strcmp( key, "textHidden" ) )
	{
		bHideInput = (bool) atoi( data );
	}
	else
	{
		return CMenuBaseItem::KeyValueData(key, data);
	}

	return true;
}
