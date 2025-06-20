/*
Copyright (C) 1997-2001 Id Software, Inc.

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

#include "Framework.h"
#include "Bitmap.h"
#include "PicButton.h"

#define ART_BANNER		"gfx/shell/head_touch"

class CMenuTouch : public CMenuFramework
{
public:
	CMenuTouch() : CMenuFramework ( "CMenuTouch" ) { }
};

ADD_MENU3( menu_touch, CMenuTouch, UI_Touch_Menu );

/*
=================
UI_Touch_Menu
=================
*/
void UI_Touch_Menu( void )
{
	if( !menu_touch->WasInit() )
	{
		menu_touch->banner.SetPicture( ART_BANNER );
		menu_touch->AddItem( menu_touch->banner );

		menu_touch->AddButton( L( "Touch options" ), L( "Touch sensitivity and profile options" ), PC_TOUCH_OPTIONS,
			UI_TouchOptions_Menu, QMF_NOTIFY, 'o' );

		menu_touch->AddButton( L( "Touch buttons" ), L( "Add, remove, edit touch buttons" ), PC_TOUCH_BUTTONS,
			UI_TouchButtons_Menu, QMF_NOTIFY, 'b' );

		menu_touch->AddButton( L( "Done" ),  L( "Go back to the previous menu" ), PC_DONE, VoidCb( &CMenuFramework::Hide ), QMF_NOTIFY );
	}

	menu_touch->Show();
}
