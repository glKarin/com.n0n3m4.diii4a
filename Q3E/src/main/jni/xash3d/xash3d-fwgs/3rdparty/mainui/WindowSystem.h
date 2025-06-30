/*
WindowSystem.h -- window system
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

#pragma once
#ifndef WINDOWSYSTEM_H
#define WINDOWSYSTEM_H

#include "utllinkedlist.h"

class CMenuBaseWindow;

class CWindowStack
{
public:
	CWindowStack() :
		active( stack.InvalidIndex() )
	{

	}

	CMenuBaseWindow *Current() const { return stack.IsValidIndex( active ) ? stack[active] : NULL; }

	bool IsActive( void ) { return stack.Count() > 0; }
	int  Count( void ) { return stack.Count(); }
	void Clean( void )
	{
		stack.RemoveAll();
		active = stack.InvalidIndex();
	}
	void Add( CMenuBaseWindow *menu );
	void Remove( CMenuBaseWindow *menu );

	bool IsVisible( const CMenuBaseWindow *menu ) const;

	void VidInit( bool firstTime );
	void Update( void );
	void KeyUpEvent( int key );
	void KeyDownEvent( int key );
	void CharEvent( int ch );
	void MouseEvent( int x, int y );
	void InputMethodResized( void );
private:
	CUtlLinkedList<CMenuBaseWindow *> stack;

	int active; // current active window
};

#endif // WINDOWSYSTEM_H
