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
#ifndef MENU_PICBUTTON_H
#define MENU_PICBUTTON_H

#include "BtnsBMPTable.h"
#include "BaseWindow.h"

// Use hover bitmap from btns_main.bmp instead of head_%s.bmp
// #define TA_ALT_MODE 1

// Use banner for animation
#define TA_ALT_MODE2 1

// Title Transition Time period
#define TTT_PERIOD		200.0f

class CMenuPicButton : public CMenuBaseItem
{
public:
	typedef CMenuBaseItem BaseClass;

	CMenuPicButton();
	bool KeyUp( int key ) override;
	bool KeyDown( int key ) override;
	void Draw( void ) override;
	bool HotKey( int key ) override;

	void SetPicture( EDefaultBtns ID );
	void SetPicture( const char *filename, int hotkey = 0 );

	bool bEnableTransitions;
	bool bPulse;
private:
	bool bRollOver;

	void CheckWindowChanged( void );
	void _Event( int ev ) override;

	void DrawButton( int r, int g, int b, int a, wrect_t *rects, int state );

	HIMAGE hPic;
	int hotkey;
	int button_id;
	int iFocusStartTime;
	int iOldState;
};

#endif
