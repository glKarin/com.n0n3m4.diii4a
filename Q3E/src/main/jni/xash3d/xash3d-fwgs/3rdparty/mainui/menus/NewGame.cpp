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

#include "CheckBox.h"
#include "EventSystem.h"
#include "Framework.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "YesNoMessageBox.h"
#include "keydefs.h"
#include "MenuStrings.h"

#define ART_BANNER		"gfx/shell/head_newgame"

class CMenuNewGame : public CMenuFramework
{
public:
	CMenuNewGame() : CMenuFramework( "CMenuNewGame" ) { }
	void StartGame( uintptr_t skill );
	void Show() override
	{
		if( gMenu.m_gameinfo.flags & GFL_NOSKILLS )
		{
			StartGame( 1.0f );
			return;
		}

		CMenuFramework::Show();
	}

private:
	void _Init() override;

	static void StartGameCb( CMenuBaseItem *pSelf, void *pExtra );
	static void ShowDialogCb( CMenuBaseItem *pSelf, void *pExtra  );

	CMenuYesNoMessageBox  msgBox;

	CEventCallback easyCallback;
	CEventCallback normCallback;
	CEventCallback hardCallback;

	CMenuCheckBox startDemoChapter;
};

/*
=================
CMenuNewGame::StartGame
=================
*/
void CMenuNewGame::StartGame( uintptr_t skill )
{
	if( EngFuncs::GetCvarFloat( "host_serverstate" ) && EngFuncs::GetCvarFloat( "maxplayers" ) > 1 )
		EngFuncs::HostEndGame( "end of the game" );

	EngFuncs::CvarSetValue( "skill", skill );
	EngFuncs::CvarSetValue( "deathmatch", 0.0f );
	EngFuncs::CvarSetValue( "teamplay", 0.0f );
	EngFuncs::CvarSetValue( "pausable", 1.0f ); // singleplayer is always allowing pause
	EngFuncs::CvarSetValue( "maxplayers", 1.0f );
	EngFuncs::CvarSetValue( "coop", 0.0f );

	EngFuncs::PlayBackgroundTrack( NULL, NULL );

	if( startDemoChapter.bChecked )
		EngFuncs::ClientCmdF( false, "newgame \"%s\"\n", gMenu.m_gameinfo.demomap );
	else EngFuncs::ClientCmd( FALSE, "newgame\n" );
}

void CMenuNewGame::StartGameCb( CMenuBaseItem *pSelf, void *pExtra )
{
	CMenuNewGame *ui = (CMenuNewGame*)pSelf->Parent();

	ui->StartGame( (uintptr_t)pExtra );
}

void CMenuNewGame::ShowDialogCb( CMenuBaseItem *pSelf, void *pExtra )
{
	CMenuNewGame *ui = (CMenuNewGame*)pSelf->Parent();

	ui->msgBox.onPositive = *(CEventCallback*)pExtra;
	ui->msgBox.Show();
}

/*
=================
CMenuNewGame::Init
=================
*/
void CMenuNewGame::_Init( void )
{
	AddItem( banner );

	banner.SetPicture( ART_BANNER );

	easyCallback = StartGameCb;
	easyCallback.pExtra = (void *)1;

	normCallback = StartGameCb;
	normCallback.pExtra = (void *)2;

	hardCallback = StartGameCb;
	hardCallback.pExtra = (void *)3;

	CMenuPicButton *easy = AddButton( L( "GameUI_Easy" ), L( "StringsList_200" ), PC_EASY, easyCallback, QMF_NOTIFY );
	CMenuPicButton *norm = AddButton( L( "GameUI_Medium" ), L( "StringsList_201" ), PC_MEDIUM, normCallback, QMF_NOTIFY );
	CMenuPicButton *hard = AddButton( L( "GameUI_Hard" ), L( "StringsList_202" ), PC_DIFFICULT, hardCallback, QMF_NOTIFY );

	easy->onReleasedClActive =
		norm->onReleasedClActive =
		hard->onReleasedClActive = ShowDialogCb;
	easy->onReleasedClActive.pExtra = &easyCallback;
	norm->onReleasedClActive.pExtra = &normCallback;
	hard->onReleasedClActive.pExtra = &hardCallback;

	AddButton( L( "GameUI_Cancel" ), L( "Go back to the Main menu" ), PC_CANCEL, VoidCb( &CMenuNewGame::Hide ), QMF_NOTIFY );

	startDemoChapter.SetCoord( 72, 230 + m_iBtnsNum * 50 );
	startDemoChapter.SetNameAndStatus( L( "GameUI_PlayGame_Alt" ), L( "Play the demo chapter on selected difficulty" ));

	if( EngFuncs::IsMapValid( gMenu.m_gameinfo.demomap ))
		AddItem( startDemoChapter );

	msgBox.SetMessage( L( "StringsList_240" ) );
	msgBox.HighlightChoice( CMenuYesNoMessageBox::HIGHLIGHT_NO );
	msgBox.Link( this );

}

ADD_MENU( menu_newgame, CMenuNewGame, UI_NewGame_Menu );
