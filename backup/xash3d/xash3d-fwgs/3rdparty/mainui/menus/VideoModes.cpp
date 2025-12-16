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
#include "MenuStrings.h"
#include "Table.h"
#include "CheckBox.h"
#include "YesNoMessageBox.h"
#include "SpinControl.h"
#include "utlvector.h"
#include "StringArrayModel.h"

#define ART_BANNER		"gfx/shell/head_vidmodes"

// TODO:
// - Get rid of vid mode strings, always compare the Width X Height of the window
//   Because if user has resized the window, or set it to cmdline, it will go
//   out of sync with the vid mode strings list
//   Doing this, however, will require API extension

class CMenuVidModesModel : public CMenuBaseArrayModel
{
public:
	void Update() override;
	int GetRows() const override
	{
		return m_szModes.Count();
	}

	const char *GetText( int line ) override
	{
		return m_szModes[line];
	}

private:
	CUtlVector<const char *> m_szModes;
};

class CMenuRenderersModel : public CMenuBaseArrayModel
{
public:
	void Update() override;
	int GetRows() const override
	{
		return m_refs.Count();
	}

	const char *GetText( int i ) override
	{
		return m_refs[i].readable;
	}

	const char *GetShortName( int i )
	{
		return m_refs[i].shortName;
	}

	void Add( const char *str )
	{
		refdll temp;

		Q_strncpy( temp.shortName, str, sizeof( temp.shortName ));
		Q_strncpy( temp.readable, str, sizeof( temp.readable ));

		m_refs.AddToTail( temp );
	}
private:
	struct refdll
	{
		char shortName[64];
		char readable[64];
	};

	CUtlVector<refdll> m_refs;
};

class CMenuVidModes : public CMenuFramework
{
private:
	void _Init() override;
	void Reload() override;
	void Draw() override; // put test mode timer here

public:
	CMenuVidModes() : CMenuFramework( "CMenuVidModes" ) { testModeTimer = 0; }

	void SetMode( int mode );
	void SetMode( int w, int h );
	void SetConfig();
	void RevertChanges();
	void ApplyChanges();
	void FinalizeChanges();

	void GetConfig();

	void GetRendererConfig();
	void WriteRendererConfig();

	CMenuCheckBox	vsync;

	CMenuTable	vidList;
	CMenuVidModesModel vidListModel;

	CMenuYesNoMessageBox testModeMsgBox;

	CMenuRenderersModel renderersModel;
	CMenuSpinControl renderers;
	CMenuSpinControl windowMode;

	int prevMode;
	int prevModeX;
	int prevModeY;
	int prevFullscreen;
	float testModeTimer;
	char testModeMsg[256];
};

void CMenuVidModesModel::Update()
{
	m_szModes.Purge();

	for( int i = 0; ; i++ )
	{
		const char *mode = EngFuncs::GetModeString( i );

		if( !mode )
			break;

		m_szModes.AddToTail( mode );
	}
}

void CMenuRenderersModel::Update()
{
	const char *r_refdll_loaded = EngFuncs::GetCvarString( "r_refdll_loaded" );
	m_refs.Purge();

	for( int i = 0; ; i++ )
	{
		refdll temp;

		if( !EngFuncs::GetRenderers( i, temp.shortName, sizeof( temp.shortName ), temp.readable, sizeof( temp.readable )))
			break;

		// append asterisk to currently loaded renderer
		if( !stricmp( r_refdll_loaded, temp.shortName ))
			strncat( temp.readable, "*", sizeof( temp.readable ));

		m_refs.AddToTail( temp );
	}
}

void CMenuVidModes::GetRendererConfig()
{
	// get current _configured_ renderer
	const char *refdll = EngFuncs::GetCvarString( "r_refdll" );

	if( !refdll[0] )
	{
		renderers.SetCurrentValue( 0.0f );
		return;
	}

	int i;
	for( i = 0; i < renderersModel.GetRows(); i++ )
	{
		if( !stricmp( renderersModel.GetShortName( i ), refdll ))
		{
			renderers.SetCurrentValue( i );
			break;
		}
	}

	if( i == renderersModel.GetRows( ))
	{
		renderersModel.Add( refdll ); // add unknown user selection
		renderers.SetCurrentValue( i );
	}
}

void CMenuVidModes::WriteRendererConfig()
{
	int i = renderers.GetCurrentValue();
	EngFuncs::CvarSetString( "r_refdll", renderersModel.GetShortName( i ));
}

void CMenuVidModes::GetConfig()
{
	float fullscreen = EngFuncs::GetCvarFloat( "fullscreen" );
	fullscreen = bound( 0, fullscreen, 2 ); // windowed, fullscreen, borderless

	float vid_mode = EngFuncs::GetCvarFloat( "vid_mode" );
	vid_mode = bound( 0, vid_mode, vidListModel.GetRows( ));

	windowMode.SetCurrentValue( fullscreen );
	vidList.SetCurrentIndex( vid_mode );

	renderers.UpdateCvar( true );
	vsync.UpdateCvar( true );

	ApplyChanges();
}

void CMenuVidModes::SetMode( int w, int h )
{
	// only possible on Xash3D FWGS!
	EngFuncs::ClientCmdF( true, "vid_setmode %i %i\n", w, h );
}

void CMenuVidModes::SetMode( int mode )
{
	// vid_setmode is a new command, which does not depends on
	// static resolution list but instead uses dynamic resolution
	// list provided by video backend
	EngFuncs::ClientCmdF( true, "vid_setmode %i\n", mode );
}

/*
=================
UI_VidModes_SetConfig
=================
*/
void CMenuVidModes::SetConfig( )
{
	bool testMode = false;
	int  currentWindowModeIndex = windowMode.GetCurrentValue();
	int  currentModeIndex = vidList.GetCurrentIndex();
	bool isVidModeChanged = prevMode != currentModeIndex;
	bool isWindowedModeChanged = prevFullscreen != currentWindowModeIndex;

	/*
	checking windowed mode first because it'll be checked next in
	screen resolution changing code, otherwise when user try to
	change screen resolution and windowed flag at same time,
	only resolution will be changed.
	*/

	if( isWindowedModeChanged ) // moved to fullscreen, enable test mode
		testMode |= currentWindowModeIndex > 0;

	if( isVidModeChanged ) // have changed resolution, but enable test mode only in fullscreen
		testMode |= currentWindowModeIndex > 0;

	if( testMode ) // show this dialog before changing any settings
	{
		testModeMsgBox.Show();
		testModeTimer = gpGlobals->time + 10.0f; // ten seconds should be enough
	}

	vsync.WriteCvar();

	if( isWindowedModeChanged )
		EngFuncs::CvarSetValue( "fullscreen", currentWindowModeIndex );

	if( isVidModeChanged )
	{
		SetMode( currentModeIndex );
		EngFuncs::CvarSetValue( "vid_mode", currentModeIndex );
		vidList.SetCurrentIndex( currentModeIndex );
	}

	if( !testMode )
		Hide();
}

void CMenuVidModes::FinalizeChanges()
{
	prevMode = EngFuncs::GetCvarFloat( "vid_mode" );
	prevFullscreen = EngFuncs::GetCvarFloat( "fullscreen" );
	prevModeX = EngFuncs::GetCvarFloat( "width" );
	prevModeY = EngFuncs::GetCvarFloat( "height" );
}

void CMenuVidModes::ApplyChanges()
{
	if( testModeMsgBox.IsVisible( ))
		return;

	FinalizeChanges();
}

void CMenuVidModes::RevertChanges()
{
	bool fullscreenChanged = false;

	// if we switched FROM fullscreen TO windowed, then we must get rid of fullscreen mode
	// so we can easily change the window size, without jerking display mode
	if( prevFullscreen == false && EngFuncs::GetCvarFloat( "fullscreen" ) != 0.0f )
	{
		EngFuncs::CvarSetValue( "fullscreen", prevFullscreen );
		fullscreenChanged = true;
	}

	SetMode( prevModeX, prevModeY );

	// otherwise, we better to set window size at first, then switch TO fullscreen
	if( !fullscreenChanged )
		EngFuncs::CvarSetValue( "fullscreen", prevFullscreen );
}

void CMenuVidModes::Draw()
{
	if( testModeMsgBox.IsVisible( ) && !FBitSet( testModeMsgBox.iFlags, QMF_CLOSING ))
	{
		if( testModeTimer - gpGlobals->time > 0 )
		{
			snprintf( testModeMsg, sizeof( testModeMsg ), L( "Keep this resolution? %i seconds remaining" ), (int)(testModeTimer - gpGlobals->time) );
		}
		else
		{
			testModeMsgBox.Hide();
			RevertChanges();
		}
	}
	CMenuFramework::Draw();
}

/*
=================
UI_VidModes_Init
=================
*/
void CMenuVidModes::_Init( void )
{
	static const char *windowModeStr[] =
	{
		L( "Windowed" ),
		L( "Fullscreen" ),
		L( "Borderless" ),
	};
	static CStringArrayModel windowModeModel( windowModeStr, V_ARRAYSIZE( windowModeStr ));

	banner.SetPicture( ART_BANNER );

	vidList.SetRect( 360, 230, -20, 365 );
	vidList.SetupColumn( 0, L( "GameUI_Resolution" ), 1.0f );
	vidList.SetModel( &vidListModel );

	vsync.szName = L( "GameUI_VSync" );
	vsync.SetCoord( 360, 670 );
	vsync.LinkCvar( "gl_vsync" );

	testModeMsgBox.SetMessage( testModeMsg );
	testModeMsgBox.onPositive = VoidCb( &CMenuVidModes::FinalizeChanges );
	testModeMsgBox.onNegative = VoidCb( &CMenuVidModes::RevertChanges );
	testModeMsgBox.Link( this );

	renderersModel.Update();
	renderers.szName = L( "GameUI_Renderer" );
	renderers.Setup( &renderersModel );
	renderers.SetRect( 80, 480, 250, 32 );
	renderers.SetCharSize( QM_SMALLFONT );
	renderers.onCvarGet = VoidCb( &CMenuVidModes::GetRendererConfig );
	renderers.onCvarWrite = VoidCb( &CMenuVidModes::WriteRendererConfig );
	renderers.bUpdateImmediately = true;

	windowMode.szName = L( "Window mode" );
	windowMode.Setup( &windowModeModel );
	windowMode.SetRect( 80, 550, 250, 32 );
	windowMode.SetCharSize( QM_SMALLFONT );
	SET_EVENT_MULTI( windowMode.onChanged,
	{
		// disable vid modes list when borderless is used
		CMenuVidModes *parent = pSelf->GetParent( CMenuVidModes );
		parent->vidList.SetGrayed( parent->windowMode.GetCurrentValue( ) == 2 );
		parent->vidList.SetInactive( parent->windowMode.GetCurrentValue( ) == 2 );
	});

	AddItem( banner );
	AddButton( L( "GameUI_Apply" ), nullptr, PC_OK, VoidCb( &CMenuVidModes::SetConfig ) );
	AddButton( L( "GameUI_Cancel" ), nullptr, PC_CANCEL, VoidCb( &CMenuVidModes::Hide ) );
	AddItem( renderers );
	AddItem( windowMode );
	AddItem( vsync );
	AddItem( vidList );

	renderers.LinkCvar( "r_refdll", CMenuEditable::CVAR_STRING );
}

void CMenuVidModes::Reload()
{
	GetConfig();
	CMenuFramework::Reload();
}

ADD_MENU( menu_vidmodes, CMenuVidModes, UI_VidModes_Menu )
