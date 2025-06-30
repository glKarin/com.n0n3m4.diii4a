/*
BaseItem.cpp -- base menu item
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
#include "BaseItem.h"

/*
==================
CMenuBaseItem::CMenuBaseItem
==================
*/

CMenuBaseItem::CMenuBaseItem()
{
	SetNameAndStatus( "", NULL );
	SetCharSize( QM_DEFAULTFONT );
	SetCoord( 0, 0 );
	SetSize( 0, 0 );

	iFlags = 0;

	eTextAlignment = QM_TOPLEFT;
	eFocusAnimation = QM_NOFOCUSANIMATION;
	eLetterCase = QM_NOLETTERCASE;

	bDrawStroke = false;
	iStrokeWidth = 0;

	m_iLastFocusTime = 0;
	m_bPressed = false;

	m_pParent = NULL;

	m_bAllocName = false;
}

CMenuBaseItem::~CMenuBaseItem()
{
	if( m_bAllocName )
	{
		delete[] (char*)szName;
	}
}

void CMenuBaseItem::Init()
{
	;
}

void CMenuBaseItem::VidInit()
{
	CalcPosition();
	CalcSizes();

	colorBase.SetDefault( uiPromptTextColor );
	colorFocus.SetDefault( uiPromptFocusColor );
	colorStroke.SetDefault( uiInputFgColor );
}

void CMenuBaseItem::Reload()
{
	;
}

void CMenuBaseItem::Draw()
{
	;
}

void CMenuBaseItem::Think()
{
	;
}

void CMenuBaseItem::Char( int key )
{
	;
}

bool CMenuBaseItem::KeyUp( int key )
{
	return false;
}

bool CMenuBaseItem::KeyDown( int key )
{
	return false;
}

void CMenuBaseItem::SetCharSize( EFontSizes fs )
{
	font = fs + 1; // It's guaranteed that handles will match font sizes

	switch( fs )
	{
	case QM_DEFAULTFONT:
	case QM_BOLDFONT:
		charSize = UI_MED_CHAR_HEIGHT;
		break;
	case QM_SMALLFONT:
		charSize = UI_SMALL_CHAR_HEIGHT;
		break;
	case QM_BIGFONT:
		charSize = UI_BIG_CHAR_HEIGHT;
		break;
	}
}

void CMenuBaseItem::_Event( int ev )
{
	CEventCallback callback;

	switch( ev )
	{
	case QM_CHANGED:   callback = onChanged; break;
	case QM_PRESSED:
		callback = onPressed;
		m_bPressed = true;
		break;
	case QM_GOTFOCUS:  callback = onGotFocus; break;
	case QM_LOSTFOCUS: callback = onLostFocus; break;
	case QM_RELEASED:
		if( (bool)onReleasedClActive && CL_IsActive( ))
			callback = onReleasedClActive;
		else callback = onReleased;
		m_bPressed = false;
		break;
	}

	if( callback ) callback( this );
}

bool CMenuBaseItem::IsCurrentSelected() const
{
	if( m_pParent )
		return this == m_pParent->ItemAtCursor();
	return false;
}

void CMenuBaseItem::CalcPosition()
{
	if( iFlags & QMF_DISABLESCAILING )
		m_scPos = pos;
	else
		m_scPos = pos.Scale();

	if( m_scPos.x < 0 )
	{
		int pos;
		if( m_pParent && !IsAbsolutePositioned() )
			pos = m_pParent->GetRenderSize().w;
		else pos = ScreenWidth;

		m_scPos.x = pos + m_scPos.x;
	}

	if( m_scPos.y < 0 )
	{
		int pos;
		if( m_pParent && !IsAbsolutePositioned() )
			pos = m_pParent->GetRenderSize().h;
		else pos = ScreenHeight;

		m_scPos.y = pos + m_scPos.y;
	}

	if( !IsAbsolutePositioned() && m_pParent )
	{
		Point offset = m_pParent->GetPositionOffset();
		m_scPos += offset;
	}
}

void CMenuBaseItem::CalcSizes()
{
	m_scChSize = charSize;
	if( iFlags & QMF_DISABLESCAILING )
	{
		m_scSize = size;
	}
	else
	{
		m_scSize = size.Scale();
		m_scChSize *= uiStatic.scaleY;
	}

	if( m_scSize.w < 0 )
	{
		int size;
		if( m_pParent && !IsAbsolutePositioned() )
			size = m_pParent->GetRenderSize().w;
		else size = ScreenWidth;

		m_scSize.w = size + m_scSize.w - m_scPos.x;
	}

	if( m_scSize.h < 0 )
	{
		int size;
		if( m_pParent && !IsAbsolutePositioned() )
			size = m_pParent->GetRenderSize().h;
		else size = ScreenHeight;

		m_scSize.h = size + m_scSize.h - m_scPos.y;
	}
}

// we need to remap position, because resource files are keep screen at 640x480, but we in 1024x768
#define REMAP_RATIO ( 1.6f )

bool CMenuBaseItem::KeyValueData(const char *key, const char *data)
{
	if( !strcmp( key, "xpos" ))
	{
		int coord;
		if( data[0] == 'c' ) // center
		{
			data++;

			coord = 320 + atoi( data );
		}
		else
		{
			coord = atoi( data );
			if( coord  < 0 )
				coord += 640;
		}
		pos.x = coord * REMAP_RATIO;
	}
	else if( !strcmp( key, "ypos" ) )
	{
		int coord;
		if( data[0] == 'c' ) // center
		{
			data++;

			coord = 240 + atoi( data );
		}
		else
		{
			coord = atoi( data );
			if( coord  < 0 )
				coord += 480;
		}
		pos.y = coord * REMAP_RATIO;
	}
	else if( !strcmp( key, "wide" ) )
	{
		size.w = atoi( data ) * REMAP_RATIO;
	}
	else if( !strcmp( key, "tall" ) )
	{
		size.h = atoi( data ) * REMAP_RATIO;
	}
	else if( !strcmp( key, "visible" ) )
	{
		SetVisibility( (bool) atoi( data ) );
	}
	else if( !strcmp( key, "enabled" ) )
	{
		bool enabled = (bool) atoi( data );

		SetInactive( !enabled );
		SetGrayed( !enabled );
	}
	else if( !strcmp( key, "labelText" ) )
	{
		/*if( *data == '#')
		{
			szName = Localize( data + 1 );
			if( szName == data + 1 ) // not localized
			{
				m_bAllocName = true;
			}
		}
		else*/ m_bAllocName = true;

		if( m_bAllocName )
		{
			szName = StringCopy( data );
		}
	}
	else if( !strcmp( key, "textAlignment" ) )
	{
		if( !strcmp( data, "west" ) )
		{
			eTextAlignment = QM_LEFT;
		}
		else if( !strcmp( data, "east" ) )
		{
			eTextAlignment = QM_RIGHT;
		}
		else
		{
			Con_DPrintf( "KeyValueData: unknown textAlignment %s\n", data );
		}
	}
	/*else if( !strcmp( key, "command" ) )
	{
		CEventCallback ev;

		if( m_pParent )
		{
			onReleased = m_pParent->FindEventByName( data );
		}
		else if( !strcmp( data, "engine " ) )
		{
			onReleased.SetCommand( FALSE, data + sizeof( "engine " ) );
		}
		else
		{
			// should not happen, as parent parses the resource file and sends KeyValueData to every item inside
			// if this happens, there is a bug
			Con_DPrintf( "KeyValueData: cannot set command '%s' on '%s'\n", data, szName );
		}
	}*/
	// TODO: nomulti, nosingle, nosteam

	return true;
}
