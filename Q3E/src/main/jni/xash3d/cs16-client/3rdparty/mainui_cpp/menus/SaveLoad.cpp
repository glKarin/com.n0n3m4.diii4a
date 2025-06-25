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
#include "Action.h"

#define ART_BANNER		"gfx/shell/head_saveload"

class CMenuSaveLoad : public CMenuFramework
{
public:
	CMenuSaveLoad() : CMenuFramework( "CMenuSaveLoad" ) { }
private:
	void _Init();

	CMenuAction	hintMessage;
	char		hintText[MAX_HINT_TEXT];
};

/*
=================
UI_SaveLoad_Init
=================
*/
void CMenuSaveLoad::_Init( void )
{
	snprintf( hintText, sizeof( hintText ),
		L( "During play, you can quickly save your game by pressing %s.\nLoad this game again by pressing %s." ),
		EngFuncs::KeynumToString( KEY_GetKey( "save quick" ) ),
		EngFuncs::KeynumToString( KEY_GetKey( "load quick" ) ) );

	banner.SetPicture( ART_BANNER );

	hintMessage.iFlags = QMF_INACTIVE;
	hintMessage.colorBase = uiColorHelp;
	hintMessage.SetCharSize( QM_SMALLFONT );
	hintMessage.szName = hintText;
	hintMessage.SetCoord( 360, 480 );

	AddItem( banner );
	AddButton( L( "GameUI_LoadGame" ), L( "GameUI_LoadGameHelp" ), PC_LOAD_GAME, UI_LoadGame_Menu, QMF_NOTIFY );
	AddButton( L( "GameUI_SaveGame" ), L( "GameUI_SaveGameHelp" ), PC_SAVE_GAME, UI_SaveGame_Menu, QMF_NOTIFY );
	AddButton( L( "Done" ), L( "Go back to the Main menu" ), PC_DONE, VoidCb( &CMenuSaveLoad::Hide ), QMF_NOTIFY );
	AddItem( hintMessage );
}

ADD_MENU( menu_saveload, CMenuSaveLoad, UI_SaveLoad_Menu );
