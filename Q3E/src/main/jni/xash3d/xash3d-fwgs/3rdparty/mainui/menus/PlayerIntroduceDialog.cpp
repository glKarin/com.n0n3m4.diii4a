/*
Copyright (C) 2017 a1batross.
PlayerIntroduceDialog.cpp -- dialog intended to let player introduce themselves: enter nickname

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

#include "BaseWindow.h"
#include "PicButton.h"
#include "YesNoMessageBox.h"
#include "MessageBox.h"
#include "Field.h"
#include "PlayerIntroduceDialog.h"

class CMenuPlayerIntroduceDialog : public CMenuYesNoMessageBox
{
public:
	CMenuPlayerIntroduceDialog() : CMenuYesNoMessageBox( false ), msgBox( true )
	{
	}

	void WriteOrDiscard();
	void _Init() override;
	bool KeyDown( int key ) override;

	CMenuBaseWindow *pCaller;

private:
	CMenuField name;
	CMenuYesNoMessageBox msgBox;
};

void CMenuPlayerIntroduceDialog::WriteOrDiscard()
{
	if( !UI::Names::CheckIsNameValid( name.GetBuffer() ) )
	{
		msgBox.Show();
	}
	else
	{
		name.WriteCvar();
		SaveAndPopMenu();
	}
}

bool CMenuPlayerIntroduceDialog::KeyDown( int key )
{
	if( UI::Key::IsEscape( key ) )
	{
		return true; // handled
	}

	if( UI::Key::IsEnter( key ) && ItemAtCursor() == &name )
	{
		WriteOrDiscard();
	}

	return CMenuYesNoMessageBox::KeyDown( key );
}

void CMenuPlayerIntroduceDialog::_Init()
{
	onPositive = VoidCb( &CMenuPlayerIntroduceDialog::WriteOrDiscard );
	SET_EVENT_MULTI( onNegative,
	{
		CMenuPlayerIntroduceDialog *self = (CMenuPlayerIntroduceDialog*)pSelf;
		self->Hide(); // hide ourselves first
		self->pCaller->Hide(); // hide our parent
	});

	SetMessage( L( "GameUI_PlayerName" ) );

	name.bAllowColorstrings = true;
	name.SetRect( 188, 140, 270, 32 );
	name.LinkCvar( "name" );
	name.iMaxLength = MAX_SCOREBOARDNAME;
	name.SetBuffer( EngFuncs::GetCvarString( "ui_username" ));

	msgBox.SetMessage( L( "Please, choose another player name" ) );
	msgBox.Link( this );

	// don't close automatically
	bAutoHide = false;
	Link( this ); // i am my own son

	CMenuYesNoMessageBox::_Init();

	AddItem( name );
}

ADD_MENU3( menu_playerintroducedialog, CMenuPlayerIntroduceDialog, UI_PlayerIntroduceDialog_Show );

void UI_PlayerIntroduceDialog_Show() { }

void UI_PlayerIntroduceDialog_Show( CMenuBaseWindow *pCaller )
{
	menu_playerintroducedialog->pCaller = pCaller;
	menu_playerintroducedialog->Show();
}
