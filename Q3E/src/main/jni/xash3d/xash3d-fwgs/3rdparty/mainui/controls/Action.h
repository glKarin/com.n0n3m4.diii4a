/*
Action.h - simple label with background item
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
#ifndef MENU_ACTION_H
#define MENU_ACTION_H

#include "BaseItem.h"

class CMenuAction : public CMenuBaseItem
{
public:
	typedef CMenuBaseItem BaseClass;

	CMenuAction();

	void VidInit( void ) override;
	bool KeyUp( int key ) override;
	bool KeyDown( int key ) override;
	void Draw( void ) override;

	void SetBackground( const char *path, unsigned int color = uiColorWhite );
	void SetBackground( unsigned int color, unsigned int focused = 0 );

	bool m_bLimitBySize;
	bool bIgnoreColorstring;

private:
	CColor m_iBackcolor;
	CColor m_iBackColorFocused;
	CImage m_szBackground;
	bool m_bfillBackground;
	bool forceCalcW, forceCalcY;
};

#endif // MENU_ACTION_H
