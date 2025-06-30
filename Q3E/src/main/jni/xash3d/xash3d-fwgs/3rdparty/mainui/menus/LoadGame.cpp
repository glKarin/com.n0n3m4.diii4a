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
#include "keydefs.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "Table.h"
#include "Action.h"
#include "YesNoMessageBox.h"

#define ART_BANNER_LOAD "gfx/shell/head_load"
#define ART_BANNER_SAVE "gfx/shell/head_save"

#define LEVELSHOT_X		72
#define LEVELSHOT_Y		400
#define LEVELSHOT_W		260
#define LEVELSHOT_H		160

#define MAX_CELLSTRING CS_SIZE

class CMenuLoadGame;

struct save_t
{
	char name[CS_SIZE];

private:
	friend class CMenuSavesListModel;
	char date[CS_SIZE];
	char comment[256];
	char elapsed_time[CS_SIZE];
};

class CMenuSavePreview : public CMenuBaseItem
{
public:
	CMenuSavePreview() : CMenuBaseItem(), fallback( "{GRAF001" )
	{
		iFlags = QMF_INACTIVE;
	}

	void Draw() override;

	void SetSaveName( const char *name )
	{
		if( name == nullptr )
			saveshot.ForceUnload();
		else
		{
			char path[128];
			snprintf( path, sizeof( path ), "save/%s.bmp", name );
			saveshot.Load( path );

			if( saveshot.IsValid( ))
			{
				double w = EngFuncs::PIC_Width( saveshot.Handle( ));
				double h = EngFuncs::PIC_Height( saveshot.Handle( ));

				size.h = round( size.w / ( w / h ));
				CalcSizes();
			}
		}
	}

	CImage fallback;
	CImage saveshot;
};

class CMenuSavesListModel : public CMenuBaseModel, public CUtlVector<save_t>
{
public:
	CMenuSavesListModel( CMenuLoadGame *parent ) : parent( parent ) { }

	void Update() override;
	int GetColumns() const override
	{
		// time, name, gametime
		return 3;
	}
	int GetRows() const override
	{
		return Count();
	}
	const char *GetCellText( int line, int column ) override
	{
		switch( column )
		{
		case 0: return Element( line ).date;
		case 1: return Element( line ).comment;
		case 2: return Element( line ).elapsed_time;
		}
		ASSERT( 0 );
		return NULL;
	}
	unsigned int GetAlignmentForColumn(int column) const override
	{
		return column == 2 ? QM_RIGHT : QM_LEFT;
	}

	void OnDeleteEntry( int line ) override;
private:
	CMenuLoadGame *parent;
};

class CMenuLoadGame : public CMenuFramework
{
public:
	CMenuLoadGame() : CMenuFramework( "CMenuLoadGame" ), savesListModel( this ) { }

	// true to turn this menu into save mode, false to turn into load mode
	void SetSaveMode( bool saveMode );
	bool IsSaveMode() { return m_fSaveMode; }
	void UpdateList() { savesListModel.Update(); }

private:
	void _Init( void );

	void LoadGame();
	void SaveGame();
	void UpdateGame();
	void DeleteGame();

	CMenuPicButton	load;
	CMenuPicButton  save;
	CMenuPicButton	remove;
	CMenuPicButton	cancel;

	CMenuTable	savesList;

	CMenuSavePreview	levelShot;
	bool m_fSaveMode;
	char		hintText[MAX_HINT_TEXT];

	// prompt dialog
	CMenuYesNoMessageBox msgBox;
	CMenuSavesListModel savesListModel;

	friend class CMenuSavesListModel;
};

void CMenuSavePreview::Draw()
{
	if( saveshot.IsValid( ))
		UI_DrawPic( m_scPos, m_scSize, uiColorWhite, saveshot );
	else
		UI_DrawPic( m_scPos, m_scSize, uiColorWhite, fallback, QM_DRAWADDITIVE );

	// draw the rectangle
	UI_DrawRectangle( m_scPos, m_scSize, uiInputFgColor );
}

/*
=================
CMenuSavesListModel::Update
=================
*/
void CMenuSavesListModel::Update( void )
{
	char **filenames;
	int numFiles;

	RemoveAll();
	filenames = EngFuncs::GetFilesList( "save/*.sav", &numFiles, TRUE );

	// sort the saves in reverse order (oldest past at the end)
	qsort( filenames, numFiles, sizeof( *filenames ), (cmpfunc)COM_CompareSaves );

	if( parent->IsSaveMode( ) && CL_IsActive( ))
	{
		// create new entry for current save game
		save_t save;
		Q_strncpy( save.name, "new", sizeof( save.name )); // special name, handled in SV_Save_f
		Q_strncpy( save.date, L( "GameUI_SaveGame_Current" ), sizeof( save.date ));
		Q_strncpy( save.comment, L( "GameUI_SaveGame_NewSavedGame" ), sizeof( save.comment ));
		Q_strncpy( save.elapsed_time, L( "GameUI_SaveGame_New" ), sizeof( save.elapsed_time ));

		AddToTail( save );
	}

	for( int i = 0; i < numFiles; i++ )
	{
		save_t save;
		char comment[256];
		char time[CS_TIME];
		char date[CS_TIME];

		// strip path, leave only filename (empty slots doesn't have savename)
		COM_FileBase( filenames[i], save.name, sizeof( save.name ));

		if( !EngFuncs::GetSaveComment( filenames[i], comment ))
		{
			if( comment[0] )
			{
				// get name string even if not found - SV_GetComment can be mark saves
				// as <CORRUPTED> <OLD VERSION> etc
				Q_strncpy( save.date, comment, sizeof( save.date ));
				save.elapsed_time[0] = 0;
				save.comment[0] = 0;

				AddToTail( save );
			}
			continue;
		}

		// they are defined by comment string format
		// time and date
		Q_strncpy( time, comment + CS_SIZE, CS_TIME );
		Q_strncpy( date, comment + CS_SIZE + CS_TIME, CS_TIME );

		snprintf( save.date, sizeof( save.date ), "%s %s", time, date );

		// ingame time
		Q_strncpy( save.elapsed_time, comment + CS_SIZE + CS_TIME * 2, sizeof( save.elapsed_time ));

		char *title, *type, *p;
		type = p = nullptr;

		// we need real title
		// so search for square brackets
		if( comment[0] == '[' && ( p = strchr( comment, ']' )))
		{
			type = comment + 1; // this might be "autosave", "quick", etc...
			title = p + 1; // this is a title
		}
		else title = comment;

		if( title[0] == '#' )
		{
			char s[CS_SIZE];

			// remove the second ], we don't need it to concatenate
			if( p )
				*p = 0;

			// strip everything after first space, assume translatable save titles have no space
			p = strchr( title, ' ' );
			if( p )
				*p = 0;

			Q_strncpy( s, title, sizeof( s ));

			if( type )
				snprintf( save.comment, sizeof( save.comment ), "[%.16s]%s", type, L( s ));
			else Q_strncpy( save.comment, L( s ), sizeof( save.comment ));
		}
		else
		{
			// strip whitespace from the end of string
			for( size_t len = strlen( title ) - 1; len >= 0; len-- )
			{
				if( !isspace( title[len] ))
					break;

				title[len] = '\0';
			}

			Q_strncpy( save.comment, comment, sizeof( save.comment ));
		}

		AddToTail( save );
	}

	if( !parent->IsSaveMode( ))
	{
		parent->levelShot.SetSaveName( IsValidIndex( 0 ) ? Element( 0 ).name : nullptr );
		parent->load.SetGrayed( !IsValidIndex( 0 ) );
	}
	else
	{
		parent->levelShot.SetSaveName( nullptr );
		parent->save.SetGrayed( !IsValidIndex( 0 ) || !CL_IsActive( ));
	}

	parent->remove.SetGrayed( !IsValidIndex( 0 ));
}

void CMenuSavesListModel::OnDeleteEntry( int line )
{
	parent->msgBox.Show();
}

/*
=================
UI_LoadGame_Init
=================
*/
void CMenuLoadGame::_Init( void )
{
	save.szName = L( "GameUI_Save" );
	save.SetPicture( PC_SAVE_GAME );
	save.onReleased = VoidCb( &CMenuLoadGame::SaveGame );
	save.SetCoord( 72, 230 );

	load.szName = L( "GameUI_Load" );
	load.SetPicture( PC_LOAD_GAME );
	load.onReleased = VoidCb( &CMenuLoadGame::LoadGame );
	load.SetCoord( 72, 230 );

	remove.szName = L( "Delete" );
	remove.SetPicture( PC_DELETE );
	remove.onReleased = msgBox.MakeOpenEvent();
	remove.SetCoord( 72, 280 );

	cancel.szName = L( "GameUI_Cancel" );
	cancel.SetPicture( PC_CANCEL );
	cancel.onReleased = VoidCb( &CMenuLoadGame::Hide );
	cancel.SetCoord( 72, 330 );

	savesList.szName = hintText;
	savesList.onChanged = VoidCb( &CMenuLoadGame::UpdateGame );
	savesList.SetupColumn( 0, L( "GameUI_Time" ), 0.30f );
	savesList.SetupColumn( 1, L( "GameUI_Game" ), 0.55f );
	savesList.SetupColumn( 2, L( "GameUI_ElapsedTime" ), 0.15f );

	savesList.SetModel( &savesListModel );
	savesList.SetCharSize( QM_SMALLFONT );
	savesList.SetRect( 360, 230, -20, 465 );

	msgBox.SetMessage( L( "Delete this saved game?" ) );
	msgBox.onPositive = VoidCb( &CMenuLoadGame::DeleteGame );
	msgBox.Link( this );

	levelShot.SetRect( LEVELSHOT_X, LEVELSHOT_Y, LEVELSHOT_W, LEVELSHOT_H );

	AddItem( banner );
	AddItem( load );
	AddItem( save );
	AddItem( remove );
	AddItem( cancel );
	AddItem( levelShot );
	AddItem( savesList );
}

void CMenuLoadGame::LoadGame()
{
	if( !savesListModel.IsValidIndex( savesList.GetCurrentIndex( )))
		return;

	char cmd[128];
	const char *name = savesListModel[savesList.GetCurrentIndex( )].name;

	snprintf( cmd, sizeof( cmd ), "load \"%s\"\n", name );
	EngFuncs::StopBackgroundTrack( );
	EngFuncs::ClientCmd( FALSE, cmd );
	UI_CloseMenu();
}

void CMenuLoadGame::SaveGame()
{
	if( !savesListModel.IsValidIndex( savesList.GetCurrentIndex( )))
		return;

	char cmd[128];
	const char *name = savesListModel[savesList.GetCurrentIndex( )].name;

	snprintf( cmd, sizeof( cmd ), "save/%s.bmp", name );
	EngFuncs::PIC_Free( cmd );

	snprintf( cmd, sizeof( cmd ), "save \"%s\"\n", name );
	EngFuncs::ClientCmd( FALSE, cmd );
	UI_CloseMenu();
}

void CMenuLoadGame::UpdateGame()
{
	// first item is for creating new saves
	if(( IsSaveMode() && savesList.GetCurrentIndex() == 0 ) || !savesListModel.IsValidIndex( savesList.GetCurrentIndex( )))
	{
		remove.SetGrayed( true );
		levelShot.SetSaveName( nullptr );
	}
	else
	{
		remove.SetGrayed( false );
		levelShot.SetSaveName( savesListModel[savesList.GetCurrentIndex( )].name );
	}
}

void CMenuLoadGame::DeleteGame()
{
	if( !savesListModel.IsValidIndex( savesList.GetCurrentIndex( )))
		return;

	char cmd[128];
	const char *name = savesListModel[savesList.GetCurrentIndex( )].name;

	snprintf( cmd, sizeof( cmd ), "killsave \"%s\"\n", name );
	EngFuncs::ClientCmd( TRUE, cmd );

	snprintf( cmd, sizeof( cmd ), "save/%s.bmp", name );
	EngFuncs::PIC_Free( cmd );

	savesListModel.Update();
}

void CMenuLoadGame::SetSaveMode( bool saveMode )
{
	m_fSaveMode = saveMode;
	if( saveMode )
	{
		banner.SetPicture( ART_BANNER_SAVE );
		save.SetVisibility( true );
		load.SetVisibility( false );
		szName = "CMenuSaveGame";
	}
	else
	{
		banner.SetPicture( ART_BANNER_LOAD );
		save.SetVisibility( false );
		load.SetVisibility( true );
		szName = "CMenuLoadGame";
	}
}

static CMenuLoadGame *menu_loadgame = NULL;

/*
=================
UI_LoadGame_Precache
=================
*/
void UI_LoadSaveGame_Precache( void )
{
	menu_loadgame = new CMenuLoadGame();
	EngFuncs::PIC_Load( ART_BANNER_SAVE );
	EngFuncs::PIC_Load( ART_BANNER_LOAD );
}

void UI_LoadSaveGame_Menu( bool saveMode )
{
	if( gMenu.m_gameinfo.gamemode == GAME_MULTIPLAYER_ONLY )
	{
		// completely ignore save\load menus for multiplayer_only
		return;
	}

	if( !EngFuncs::CheckGameDll( )) return;

	menu_loadgame->Show();
	menu_loadgame->SetSaveMode( saveMode );
	menu_loadgame->UpdateList();
}

void UI_LoadSaveGame_Shutdown( void )
{
	delete menu_loadgame;
}

/*
=================
UI_LoadGame_Menu
=================
*/
void UI_LoadGame_Menu( void )
{
	UI_LoadSaveGame_Menu( false );
}

void UI_SaveGame_Menu( void )
{
	UI_LoadSaveGame_Menu( true );
}
ADD_MENU4( menu_loadgame, UI_LoadSaveGame_Precache, UI_LoadGame_Menu, UI_LoadSaveGame_Shutdown );
ADD_MENU4( menu_savegame, NULL, UI_SaveGame_Menu, NULL );
