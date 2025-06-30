/*
WindowSystem.cpp -- window system
Copyright (C) 2019 a1batross

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "Primitive.h"
#include "WindowSystem.h"
#include "BaseWindow.h"
#include "con_nprint.h"

void CWindowStack::VidInit( bool calledOnce )
{
	// now recalc all the menus in stack
	FOR_EACH_LL( stack, i )
	{
		CMenuBaseWindow *item = stack[i];

		if( item )
		{
			int cursor, cursorPrev;
			bool valid = false;

			// HACKHACK: Save cursor values when VidInit is called once
			// this don't let menu "forget" actual cursor values after, for example, window resizing
			if( calledOnce
				&& item->GetCursor() > 0 // ignore 0, because useless
				&& item->GetCursor() < item->ItemCount()
				&& item->GetCursorPrev() > 0
				&& item->GetCursorPrev() < item->ItemCount() )
			{
				valid = true;
				cursor = item->GetCursor();
				cursorPrev = item->GetCursorPrev();
			}

			// do vid restart for all pushed elements
			item->VidInit();

			item->Reload();

			if( valid )
			{
				// don't notify menu widget about cursor changes
				item->SetCursor( cursorPrev, false );
				item->SetCursor( cursor, false );
			}
		}
	}
}

bool CWindowStack::IsVisible( const CMenuBaseWindow *menu ) const
{
	if( menu == Current() )
		return true;

	if( FBitSet( menu->iFlags, QMF_HIDDEN ))
		return false;

	// for some reason const_cast need here
	if( stack.Find( const_cast<CMenuBaseWindow*>( menu )) == stack.InvalidIndex())
		return false;

	return true;
}

void CWindowStack::Update( )
{
	if( !IsActive() )
		return;

	CUtlVector<CMenuBaseWindow *> drawList( 16 );
	CUtlVector<int> removeList( 16 );

	bool stop = Current()->IsRoot();

	// always add current window
	drawList.AddToTail( Current() );

	FOR_EACH_LL_BACK( stack, i )
	{
		stack[i]->Think(); // any window must think

		if( i == active )
			continue; // will be added last

		if( stack[i]->eTransitionType )
		{
			drawList.AddToTail( stack[i] );
			continue;
		}

		if( FBitSet( stack[i]->iFlags, QMF_CLOSING ))
		{
			// it's better to not change stack while we traverse it
			ClearBits( stack[i]->iFlags, QMF_CLOSING );
			removeList.AddToTail( i );
			continue;
		}

		// minimized windows
		if( stack[i]->iFlags & QMF_HIDDEN )
			continue;

		if( stop ) // we don't need to search windows to draw
			continue;

		// maximized, no need to go further anymore,
		// but we need to check for closing windows, so don't break
		if( stack[i]->IsRoot() )
		{
			stop = true;
		}
		else
		{
			// start region test
			bool inside = false;

			Rect window( stack[i]->GetRenderPosition(), stack[i]->GetRenderSize() );

			if( !inside )
			{
				FOR_EACH_VEC( drawList, j )
				{
					if( drawList[j]->eTransitionType )
						continue; // skip animating window

					Rect drawnWindow( drawList[j]->GetRenderPosition(), drawList[j]->GetRenderSize() );

					if( drawnWindow.IsInside( window ))
						inside = true;
				}
			}

			// region test complete
			if( inside )
				continue;
		}

		// add to draw list
		drawList.AddToTail( stack[i] );
	}

	FOR_EACH_VEC( removeList, j )
	{
		stack.Remove( removeList[j] );
	}

	FOR_EACH_VEC_BACK( drawList, k )
	{
		CMenuBaseWindow *window = drawList[k];

		if( window->eTransitionType > CMenuBaseWindow::ANIM_CLOSING )
		{
			if( window->DrawAnimation( ) )
				window->DisableTransition();
		}

		if( !window->eTransitionType )
			drawList[k]->Draw();

		if( k != drawList.Count() - 1 )
		{
			window = drawList[k+1];

			if( window->eTransitionType == CMenuBaseWindow::ANIM_CLOSING )
			{
				if( window->DrawAnimation( ) )
					window->DisableTransition();
			}
		}
	}

	if( ui_show_window_stack && ui_show_window_stack->value )
	{
		con_nprint_t con;
		con.index = 0;
		con.time_to_live = 0.01f;
		con.color[0] = con.color[1] = con.color[2] = 1.0f;

		Con_NXPrintf( &con, "Stack:\n" );

		FOR_EACH_LL( stack, l )
		{
			con.index++;
			if( active == l )
			{
				con.color[0] = 0.0f;
				con.color[1] = 1.0f;
				con.color[2] = 0.0f;
			}
			else if( FBitSet( stack[l]->iFlags, QMF_CLOSING ))
			{
				// bloody :)
				con.color[0] = 1.0f;
				con.color[1] = 0.0f;
				con.color[2] = 0.0f;
			}
			else
			{
				con.color[0] = con.color[1] = con.color[2] = 1.0f;
			}

			char visible = '-';
			if( drawList.Find( stack[l] ) != drawList.InvalidIndex() )
				visible = '+';

			if( stack[l]->IsRoot() )
			{
				Con_NXPrintf( &con, "%c %p - %s\n", visible, stack[l], stack[l]->szName );
			}
			else
			{
				Con_NXPrintf( &con, "%c     %p - %s\n", visible, stack[l], stack[l]->szName );
			}
		}
	}
}

void CWindowStack::Add( CMenuBaseWindow *menu )
{
	if( menu == Current() )
		return; // do nothing

	if( stack.IsValidIndex( active ))
	{
		if( stack[active]->IsRoot() && menu->IsRoot() )
		{
			stack[active]->EnableTransition( CMenuBaseWindow::ANIM_CLOSING );
		}
	}

	// check if menu already in stack and set it active
	int i;
	if( ( i = stack.Find( menu )) != stack.InvalidIndex() )
	{
		active = i;
		ClearBits( menu->iFlags, QMF_CLOSING );
		return;
	}

	active = stack.AddToTail( menu );
	ClearBits( menu->iFlags, QMF_CLOSING );

	if( this == &uiStatic.menu ) // hack!
	{
		uiStatic.firstDraw = true;

		EngFuncs::KEY_SetDest( KEY_MENU );
	}
}

void CWindowStack::Remove( CMenuBaseWindow *menu )
{
	int min;
	int idx = stack.Find( menu );

	if( idx == stack.InvalidIndex() )
	{
		Con_DPrintf( "CWindowStack::Remove: can't remove not opened window" );
	}

	if( this == &uiStatic.menu ) // hack!
		min = 1; // main menu can't close last menu
	else
		min = 0;


	if( stack.Count() > min )
	{
		SetBits( menu->iFlags, QMF_CLOSING );
		FOR_EACH_LL_BACK( stack, i )
		{
			if( !FBitSet( stack[i]->iFlags, QMF_CLOSING ))
			{
				active = i; // maybe isn't best solution

				if( stack[active]->IsRoot() && menu->IsRoot() )
					stack[active]->EnableTransition( CMenuBaseWindow::ANIM_OPENING );

				// notify new active window about changed mouse position
				stack[active]->MouseMove( uiStatic.cursorX, uiStatic.cursorY );

				break;
			}
		}
	}
	else
	{
		if( CL_IsActive() )
		{
			UI_CloseMenu();
		}
		else
		{
			EngFuncs::KEY_SetDest( KEY_CONSOLE );
		}
	}

	// hacks for demos and some environments where we can't play them on background
	if( this == &uiStatic.menu && uiStatic.m_fDemosPlayed && uiStatic.m_iOldMenuDepth == stack.Count() - 1 )
	{
		EngFuncs::ClientCmd( FALSE, "demos\n" );
		uiStatic.m_fDemosPlayed = false;
		uiStatic.m_iOldMenuDepth = 0;
	}
}

void CWindowStack::InputMethodResized( void )
{
	if( Current() && Current()->ItemAtCursor() )
	{
		Current()->ItemAtCursor()->_Event( QM_IMRESIZED );
	}
}

void CWindowStack::KeyUpEvent( int key )
{
	if( Current() ) Current()->KeyUp( key );
}

void CWindowStack::KeyDownEvent( int key )
{
	if( Current() ) Current()->KeyDown( key );
}

void CWindowStack::CharEvent( int key )
{
	if( Current() ) Current()->Char( key );
}

void CWindowStack::MouseEvent( int x, int y )
{
	if( Current() ) Current()->MouseMove( x, y );
}

