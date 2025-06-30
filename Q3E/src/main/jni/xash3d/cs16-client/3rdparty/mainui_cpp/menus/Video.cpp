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

#define ART_BANNER		"gfx/shell/head_video"

class CMenuVideo : public CMenuFramework
{
public:
	CMenuVideo() : CMenuFramework( "CMenuVideo" ) { }
};

ADD_MENU3( menu_video, CMenuVideo, UI_Video_Menu );

/*
=================
UI_Video_Menu
=================
*/
void UI_Video_Menu( void )
{
	if( !menu_video->WasInit() )
	{
		menu_video->banner.SetPicture( ART_BANNER );

		menu_video->AddItem( menu_video->banner );
		menu_video->AddButton( L( "Video options" ), L( "Set video options such as screen size, gamma and image quality." ), PC_VID_OPT, UI_VidOptions_Menu, QMF_NOTIFY );
		menu_video->AddButton( L( "Video modes" ), L( "Set video modes and configure 3D accelerators." ), PC_VID_MODES, UI_VidModes_Menu, QMF_NOTIFY );
		menu_video->AddButton( L( "Done" ), L( "Go back to the previous menu." ), PC_DONE, VoidCb( &CMenuFramework::Hide ), QMF_NOTIFY );
	}

	menu_video->Show();
}
