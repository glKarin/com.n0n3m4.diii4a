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
#include "SpinControl.h"
#include "Action.h"
#include "CheckBox.h"

#define ART_BANNER			"gfx/shell/head_advoptions"

class CMenuGameOptions : public CMenuFramework
{
public:
	CMenuGameOptions() : CMenuFramework("CMenuGameOptions") { }

	bool KeyDown( int key ) override;
	void SetNetworkMode( int maxpacket, int maxpayload, int cmdrate, int updaterate, int rate );
private:
	void _Init() override;
	void SaveCb( );
	void RestoreCb( );
	void Restore();
	void GetConfig();

	CMenuSpinControl	maxFPS;
	//CMenuCheckBox	hand;
	CMenuCheckBox	allowDownload;
	CMenuPicButton	allowConsole;

	CMenuSpinControl	cmdrate, updaterate, rate;
	CMenuAction networkMode;
	CMenuCheckBox normal, dsl, slowest;
};

/*
=================
UI_GameOptions_KeyFunc
=================
*/
bool CMenuGameOptions::KeyDown( int key )
{
	if( UI::Key::IsEscape( key ) )
		Restore();
	return CMenuFramework::KeyDown( key );
}

void CMenuGameOptions::SetNetworkMode( int maxpacket1, int maxpayload1, int cmdrate1, int updaterate1, int rate1 )
{
	normal.bChecked = dsl.bChecked = slowest.bChecked = false;
	cmdrate.SetCurrentValue( cmdrate1 );
	updaterate.SetCurrentValue( updaterate1 );
	rate.SetCurrentValue( rate1 );
}

void CMenuGameOptions::SaveCb()
{
	maxFPS.WriteCvar();
	//hand.WriteCvar();
	allowDownload.WriteCvar();
	cmdrate.WriteCvar();
	updaterate.WriteCvar();
	rate.WriteCvar();

	SaveAndPopMenu();
}

void CMenuGameOptions::Restore()
{
	maxFPS.DiscardChanges();
	//hand.DiscardChanges();
	allowDownload.DiscardChanges();
	cmdrate.DiscardChanges();
	updaterate.DiscardChanges();
	rate.DiscardChanges();
}

void CMenuGameOptions::RestoreCb()
{
	Restore();
	Hide();
}

/*
=================
UI_GameOptions_Init
=================
*/
void CMenuGameOptions::_Init( void )
{
	banner.SetPicture( ART_BANNER );
	maxFPS.SetNameAndStatus( L( "FPS limit" ), L( "Cap your game frame rate" ));
	maxFPS.Setup( 20, 500, 20 );
	maxFPS.LinkCvar( "fps_max", CMenuEditable::CVAR_VALUE );
	maxFPS.SetRect( 360, 260, 220, 32 );

	//hand.SetNameAndStatus( "Use left hand", "Draw gun at left side" );
	//hand.LinkCvar( "cl_righthand" );
	//hand.SetCoord( 240, 330 );

	allowDownload.SetNameAndStatus( L( "Allow download" ), L( "Allow download of files from servers" ) );
	allowDownload.LinkCvar( "cl_allowdownload" );
	allowDownload.SetCoord( 360, 315 );

	allowConsole.SetNameAndStatus( L( "Enable developer console" ), L( "Turns on console when engine was run without -console or -dev parameter" ));
	allowConsole.SetCoord( 360, 365 );
	allowConsole.onReleased.SetCommand( false, "ui_allowconsole\n" );

	cmdrate.SetRect( 600, 470, 200, 32 );
	cmdrate.Setup( 20, 60, 5 );
	cmdrate.LinkCvar( "cl_cmdrate", CMenuEditable::CVAR_VALUE );
	cmdrate.SetNameAndStatus( L( "Command rate (cl_cmdrate)" ), L( "How many commands sent to server per second" ) );

	updaterate.SetRect( 600, 570, 200, 32 );
	updaterate.Setup( 20, 100, 5 );
	updaterate.LinkCvar( "cl_updaterate", CMenuEditable::CVAR_VALUE );
	updaterate.SetNameAndStatus( L( "Update rate (cl_updaterate)" ), L( "How many updates sent from server per second" ) );

	rate.SetRect( 600, 670, 200, 32 );
	rate.Setup( 2500, 90000, 500 );
	rate.LinkCvar( "rate", CMenuEditable::CVAR_VALUE );
	rate.SetNameAndStatus( L( "Network speed (rate)" ), L( "Limit traffic (bytes per second)" ) );

	networkMode.iFlags = QMF_INACTIVE|QMF_DROPSHADOW;
	networkMode.szName = L( "Select network mode:" );
	networkMode.colorBase = uiColorHelp;
	networkMode.SetCharSize( QM_BIGFONT );
	networkMode.SetRect( 77, 430, 400, 32 );

	normal.SetRect( 77, 480, 32, 32 );
	normal.szName = L( "Normal internet connection" ); 	// Такая строка где-то уже была, поэтому в отдельный файл НЕ ВЫНОШУ !
	SET_EVENT_MULTI( normal.onChanged,
	{
		pSelf->GetParent(CMenuGameOptions)->SetNetworkMode( 1400, 0, 30, 60, 25000 );
		((CMenuCheckBox*)pSelf)->bChecked = true;
	});

	dsl.SetRect( 77, 530, 32, 32 );
	dsl.szName = L( "DSL or PPTP with limited packet size" );	// И такое тоже уже было !
	SET_EVENT_MULTI( dsl.onChanged,
	{
		pSelf->GetParent(CMenuGameOptions)->SetNetworkMode( 1200, 1000, 30, 60, 25000 );
		((CMenuCheckBox*)pSelf)->bChecked = true;
	});


	slowest.SetRect( 77, 580, 32, 32 );
	slowest.szName = L( "Slow connection mode (64kbps)" );	// Было, повтор !
	SET_EVENT_MULTI( slowest.onChanged,
	{
		pSelf->GetParent(CMenuGameOptions)->SetNetworkMode( 900, 700, 25, 30, 7500 );
		((CMenuCheckBox*)pSelf)->bChecked = true;
	});

	AddItem( banner );
	AddButton( L( "Done" ), nullptr, PC_DONE, VoidCb( &CMenuGameOptions::SaveCb ) );
	AddButton( L( "GameUI_Cancel" ), nullptr, PC_CANCEL, VoidCb( &CMenuGameOptions::RestoreCb ) );

	AddItem( maxFPS );
	//AddItem( hand );

	AddItem( allowDownload );
	AddItem( allowConsole );
	AddItem( cmdrate );
	AddItem( updaterate );
	AddItem( rate );
	AddItem( networkMode );
	AddItem( normal );
	AddItem( dsl );
	AddItem( slowest );

	// only for game/engine developers
	if( EngFuncs::GetCvarFloat( "developer" ) < 1 )
	{
		rate.Hide();
	}

	if( EngFuncs::GetCvarFloat( "developer" ) < 2 )
	{
		cmdrate.Hide();
		updaterate.Hide();
		rate.SetCoord( 650, 370 );
	}
}

ADD_MENU( menu_gameoptions, CMenuGameOptions, UI_GameOptions_Menu );
