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
#include "YesNoMessageBox.h"
#include "keydefs.h"
#include "PlayerIntroduceDialog.h"

#define ART_BANNER			"gfx/shell/head_multi"

class CMenuMultiplayer : public CMenuFramework
{
public:
	CMenuMultiplayer() : CMenuFramework( "CMenuMultiplayer" ) { }

	void Show() override
	{
		CMenuFramework::Show();

		if( EngFuncs::GetCvarFloat( "menu_mp_firsttime2" ) && EngFuncs::GetCvarFloat( "cl_nopred" ) )
		{
			EngFuncs::CvarSetValue( "cl_nopred", 0.0f );
		}

		if( !UI::Names::CheckIsNameValid( EngFuncs::GetCvarString( "name" ) ) )
		{
			UI_PlayerIntroduceDialog_Show( this );
		}
	}

private:
	void _Init() override;
};

/*
=================
CMenuMultiplayer::Init
=================
*/
void CMenuMultiplayer::_Init( void )
{
	banner.SetPicture( ART_BANNER );
	AddItem( banner );

	AddButton( L( "Internet game" ), L( "View a list of Counter-Strike game servers and join the one of your choice." ), PC_INET_GAME, UI_InternetGames_Menu, QMF_NOTIFY );
	// AddButton( L( "Spectate game" ), L( "Spectate internet games" ), PC_SPECTATE_GAMES, NoopCb, QMF_GRAYED | QMF_NOTIFY );
	AddButton( L( "LAN game" ), L( "Set up a Counter-Strike game on a local area network." ), PC_LAN_GAME, UI_LanGame_Menu, QMF_NOTIFY );
	AddButton( L( "GameUI_GameMenu_Customize" ), L( "Choose your player name, and select visual options for your character" ), PC_CUSTOMIZE, UI_PlayerSetup_Menu, QMF_NOTIFY );
	AddButton( L( "Controls" ), L( "Change keyboard, mouse, touch and joystick settings." ), PC_CONTROLS, UI_Controls_Menu, QMF_NOTIFY );
	AddButton( L( "Done" ), L( "Go back to the Main Menu." ), PC_DONE, VoidCb( &CMenuMultiplayer::Hide ), QMF_NOTIFY );
}

ADD_MENU( menu_multiplayer, CMenuMultiplayer, UI_MultiPlayer_Menu );
