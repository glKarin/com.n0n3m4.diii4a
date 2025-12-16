/*
DropDown.cpp - simple drop down menus
Copyright (C) 2023 numas13

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

#include "BaseMenu.h"
#include "DropDown.h"

CMenuDropDown::CMenuDropDown() : BaseClass()
{
	SetSize( 120, 35 );
	SetCharSize( QM_BOLDFONT );

	eTextAlignment = QM_LEFT;
	iFlags |= QMF_DROPSHADOW;

	bDropUp = false;

	m_iState = 0;
	m_szNames.Purge();
}

void CMenuDropDown::VidInit()
{
	iBackgroundColor.SetDefault( uiColorBlack );
	iFgTextColor.SetDefault( uiInputFgColor );
	iBgTextColor.SetDefault( uiPromptTextColor );

	BaseClass::VidInit();

	m_scItemSize = m_scSize;

	if( bDropUp )
	{
		m_ArrowClosed.Load(UI_UPARROW);
		m_ArrowOpened.Load(UI_UPARROWFOCUS);
	}
	else
	{
		m_ArrowClosed.Load(UI_DOWNARROW);
		m_ArrowOpened.Load(UI_DOWNARROWFOCUS);
	}
	m_ArrowSize = EngFuncs::PIC_Size( m_ArrowClosed );
}

int CMenuDropDown::IsNewStateByMouseClick()
{
	int state = m_iState;
	Point pt(m_scPos);

	if( !bDropUp )
		pt.y += m_scItemSize.h;

	for( int i = 0; i < m_szNames.Count(); i++ )
	{
		if( UI_CursorInRect( pt, m_scItemSize ) && !(iFlags & (QMF_GRAYED|QMF_INACTIVE)))
		{
			state = i;
			break;
		}
		pt.y += m_scItemSize.h;
	}

	return state;
}

bool CMenuDropDown::KeyDown( int key )
{
	return false;
}

bool CMenuDropDown::KeyUp( int key )
{
	bool ret = false;
	int state = m_iState;

	if( UI::Key::IsLeftMouse( key ) && FBitSet( iFlags, QMF_HASMOUSEFOCUS ) )
	{
		if( m_isOpen )
			state = IsNewStateByMouseClick( );

		MenuToggle( );
		ret = true;
	}

	// if( m_isOpen && UI::Key::IsLeftMouse( key ) && FBitSet( iFlags, QMF_HASMOUSEFOCUS ) )
	// 	state = IsNewStateByMouseClick( );

	if( !FBitSet( iFlags, QMF_MOUSEONLY ) )
	{
		if( UI::Key::IsDownArrow( key ) )
			state += 1;
		else if( UI::Key::IsUpArrow( key ) )
			state -= 1;
		else if( UI::Key::IsEnter( key ) )
		{
			MenuToggle( );
			return true;
		}
	}

	if( state != m_iState )
	{
		m_iState = state;

		if( m_iState < 0 )
			m_iState = m_szNames.Count() - 1;
		else if( m_iState >= m_szNames.Count() )
			m_iState = 0;

		SetCvar();
		_Event( QM_CHANGED );

		ret = true;
	}

	return ret;
}

void CMenuDropDown::SetMenuOpen( bool isOpen )
{
	if( m_isOpen && bDropUp )
		m_scPos.y += m_scSize.h - m_scItemSize.h;

	m_isOpen = isOpen;

	if( m_isOpen )
	{
		m_scSize.h = m_scItemSize.h * m_szNames.Count();

		if( bDropUp )
			m_scPos.y -= m_scSize.h;

		m_scSize.h += m_scItemSize.h;
	}
	else
		m_scSize.h = m_scItemSize.h;
}

void CMenuDropDown::Draw()
{
	uint textflags = ETF_NO_WRAP;
	uint borderColor = uiInputFgColor;

	if( iFlags & QMF_DROPSHADOW )
		textflags |= ETF_SHADOW;

	if( m_isOpen )
	{
		Point pt( m_scPos.x, m_scPos.y );

		if( !bDropUp )
			pt.y += m_scItemSize.h;

		// all other items
		UI_FillRect( pt.x, pt.y, m_scItemSize.w, m_scSize.h - m_scItemSize.h, 0xff000000 );
		for( int i = 0; i < m_szNames.Count(); i++ )
		{
			uint textColor = iBgTextColor;
			uint tempflags = textflags;

			if( UI_CursorInRect( pt, m_scItemSize ) && !(iFlags & (QMF_GRAYED|QMF_INACTIVE)))
			{
				UI_FillRect( pt, m_scItemSize, colorFocus );
				tempflags |= ETF_FORCECOL;
			}

			UI_DrawString( font, pt, m_scItemSize, m_szNames[i], textColor, m_scChSize, eTextAlignment, tempflags );
			pt.y += m_scItemSize.h;
		}
	}

	// selected item

	Point selectedPos = m_scPos;
	Size selectedSize = Size(m_scItemSize.w - m_ArrowSize.w + uiStatic.outlineWidth, m_scItemSize.h);
	uint selectedTextColor = iBgTextColor;
	uint selectedBgColor = iBackgroundColor;
	if( m_isOpen )
	{
		selectedTextColor = iFgTextColor;
		selectedBgColor = borderColor;
	}
	if( bDropUp )
	{
		selectedPos.y += m_scSize.h - m_scItemSize.h;
	}
	UI_FillRect( selectedPos, selectedSize, selectedBgColor );
	UI_DrawString( font, selectedPos, selectedSize, m_szNames[m_iState], selectedTextColor, m_scChSize, eTextAlignment, textflags );

	// border
	UI_DrawRectangle( m_scPos, m_scSize, borderColor );

	// arrow background
	uint arrowX = selectedPos.x + selectedSize.w;
	UI_FillRect( arrowX, selectedPos.y, m_ArrowSize.w, selectedSize.h, uiInputFgColor );
	// arrow
	CImage &arrow = m_isOpen ? m_ArrowOpened : m_ArrowClosed;
	Point arrowPoint( arrowX, selectedPos.y + (selectedSize.h - m_ArrowSize.h)/2 );
	UI_DrawPic( arrowPoint, m_ArrowSize, -1, arrow );
}

void CMenuDropDownStr::UpdateEditable()
{
	const char *val = EngFuncs::GetCvarString( m_szCvarName );
	int state = 0;

	for( int i = 0; i < m_Values.Count(); i++ )
	{
		if( strcmp( m_Values[i], val ) == 0 )
		{
			state = i;
			break;
		}
	}

	if( m_iState != state )
		m_iState = state;
}

void CMenuDropDownInt::UpdateEditable()
{
	int val = EngFuncs::GetCvarFloat( m_szCvarName );
	int state = 0;

	for( int i = 0; i < m_Values.Count(); i++ )
	{
		if( m_Values[i] == val )
		{
			state = i;
			break;
		}
	}

	if( m_iState != state )
		m_iState = state;
}

void CMenuDropDownFloat::UpdateEditable()
{
	float val = EngFuncs::GetCvarFloat( m_szCvarName );
	int state = 0;

	for( int i = 0; i < m_Values.Count(); i++ )
	{
		if( m_Values[i] == val )
		{
			state = i;
			break;
		}
	}

	if( m_iState != state )
		m_iState = state;
}
