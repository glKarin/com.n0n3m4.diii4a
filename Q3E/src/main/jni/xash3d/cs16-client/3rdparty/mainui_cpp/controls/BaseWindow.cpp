/*
BaseWindow.cpp -- base menu window
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
#include "Utils.h"
#include "PicButton.h"
#include "ItemsHolder.h"
#include "BaseWindow.h"

CMenuBaseWindow::CMenuBaseWindow( const char *name, CWindowStack *pStack ) : BaseClass()
{
	bAllowDrag = false; // UNDONE
	m_bHolding = false;
	szName = name;
	m_pStack = pStack;
	DisableTransition();
}

void CMenuBaseWindow::Show()
{
	Init();
	VidInit();
	Reload(); // take a chance to reload info for items
	m_pStack->Add( this );
	m_iCursor = 0;

	// Probably not a best way
	// but we need to inform new window about cursor position,
	// otherwise we will have an invalid cursor until first mouse move event
#if 1
	m_iCursorPrev = -1;
	MouseMove( uiStatic.cursorX, uiStatic.cursorY );
#else
	m_iCursor = 0;
	m_iCursorPrev = 0;
	// force first available item to have focus
	FOR_EACH_VEC( m_pItems, i )
	{
		item = m_pItems[i];

		if( !item->IsVisible() || item->iFlags & (QMF_GRAYED|QMF_INACTIVE|QMF_MOUSEONLY))
			continue;

		m_iCursorPrev = -1;
		SetCursor( i );
		break;
	}
#endif
	EnableTransition( ANIM_OPENING );
}

void CMenuBaseWindow::Hide()
{
	if( m_pStack == &uiStatic.menu ) // hack!
	{
		EngFuncs::PlayLocalSound( uiStatic.sounds[SND_OUT] );
	}

	m_pStack->Remove( this );
	EnableTransition( ANIM_CLOSING );
}

bool CMenuBaseWindow::IsVisible() const
{
	return m_pStack->IsVisible( this );
}

void CMenuBaseWindow::SaveAndPopMenu()
{
	EngFuncs::ClientCmd( FALSE, "host_writeconfig\n" );
	Hide();
}

void CMenuBaseWindow::DragDrop( int down )
{
	m_bHolding = down;
	m_bHoldOffset.x = uiStatic.cursorX;
	m_bHoldOffset.y = uiStatic.cursorY;
}

bool CMenuBaseWindow::KeyDown( int key )
{
	if( UI::Key::IsLeftMouse( key ) && bAllowDrag )
		DragDrop( true );

	if( UI::Key::IsEscape( key ) )
	{
		Hide( );
		return true;
	}

	return BaseClass::KeyDown( key );
}

bool CMenuBaseWindow::KeyUp( int key )
{
	if( UI::Key::IsLeftMouse( key ) && bAllowDrag )
		DragDrop( false );

	return BaseClass::KeyUp( key );
}

void CMenuBaseWindow::Draw()
{
	if( !IsRoot() && m_bHolding && bAllowDrag )
	{
		m_scPos.x += uiStatic.cursorX - m_bHoldOffset.x;
		m_scPos.y += uiStatic.cursorY - m_bHoldOffset.y;

		m_bHoldOffset.x = uiStatic.cursorX;
		m_bHoldOffset.y = uiStatic.cursorY;
		// CalcPosition();
		CalcItemsPositions();
	}
	CMenuItemsHolder::Draw();
}


bool CMenuBaseWindow::DrawAnimation()
{
	float alpha;

	if( eTransitionType == ANIM_OPENING )
	{
		alpha = ( uiStatic.realTime - m_iTransitionStartTime ) / TTT_PERIOD;
	}
	else if( eTransitionType == ANIM_CLOSING )
	{
		alpha = 1.0f - ( uiStatic.realTime - m_iTransitionStartTime ) / TTT_PERIOD;
	}

	if(        ( eTransitionType == ANIM_OPENING  && alpha < 1.0f )
		|| ( eTransitionType == ANIM_CLOSING && alpha > 0.0f ) )
	{
		UI_EnableAlphaFactor( alpha );

		Draw();

		UI_DisableAlphaFactor();

		return false;
	}

	return true;
}

bool CMenuBaseWindow::KeyValueData(const char *key, const char *data)
{
	if( !strcmp( key, "enabled" ) || !strcmp( key, "visible" ) )
		return true;

	return CMenuBaseItem::KeyValueData(key, data);
}

void CMenuBaseWindow::EnableTransition( EAnimation type )
{
	eTransitionType = type;
	m_iTransitionStartTime = uiStatic.realTime;
}
