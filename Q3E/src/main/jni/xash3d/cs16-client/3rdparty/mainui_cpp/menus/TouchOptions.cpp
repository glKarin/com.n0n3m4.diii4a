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
#include "Slider.h"
#include "PicButton.h"
#include "CheckBox.h"
#include "Table.h"
#include "SpinControl.h"
#include "Field.h"
#include "YesNoMessageBox.h"
#include "StringVectorModel.h"

#define ART_BANNER	  	"gfx/shell/head_touch_options"

class CMenuTouchOptions : public CMenuFramework
{
private:
	void _Init();
	void _VidInit();

public:
	CMenuTouchOptions() : CMenuFramework( "CMenuTouchOptions" ) { }

	void DeleteProfileCb( );
	void ResetButtonsCb();

	void SaveAndPopMenu();
	void GetConfig();

	void ResetMsgBox();
	void DeleteMsgBox();

	void Apply();
	void Save();

	class CProfiliesListModel : public CStringVectorModel
	{
	public:
		CProfiliesListModel() : CStringVectorModel() {}
		void Update();
		int	 iHighlight;
		int	 firstProfile;
	} model;

	CMenuPicButton	done;

	CMenuSlider	lookX;
	CMenuSlider	lookY;
	CMenuSlider	moveX;
	CMenuSlider	moveY;
	CMenuCheckBox	enable;
	CMenuCheckBox	grid;
	CMenuCheckBox	nomouse;
	CMenuPicButton	reset;
	CMenuPicButton	save;
	CMenuPicButton	remove;
	CMenuPicButton	apply;
	CMenuField	profilename;
	CMenuTable profiles;
	CMenuSpinControl gridsize;
	CMenuCheckBox acceleration;
	CMenuSlider power;
	CMenuSlider multiplier;
	CMenuSlider exponent;

	// prompt dialog
	CMenuYesNoMessageBox msgBox;
	void UpdateProfilies();
};

void CMenuTouchOptions::CProfiliesListModel::Update( void )
{
	char **filenames;
	int numFiles;
	const char *curprofile;

	filenames = EngFuncs::GetFilesList( "touch_presets/*.cfg", &numFiles, true );

	if( filenames && numFiles > 0 )
	{
		AddToTail( L( "Presets:" ));

		for( int j = 0; j < numFiles; j++ )
		{
			char profileDesc[128];
			COM_FileBase( filenames[j], profileDesc, sizeof( profileDesc ));
			AddToTail( profileDesc );
		}
	}

	filenames = EngFuncs::GetFilesList( "touch_profiles/*.cfg", &numFiles, true );
	curprofile = EngFuncs::GetCvarString("touch_config_file");

	AddToTail( L( "Profiles:" ));

	firstProfile = Count();
	AddToTail( "default" );

	for( int j = 0; j < numFiles; j++ )
	{
		char profileDesc[128];
		COM_FileBase( filenames[j], profileDesc, sizeof( profileDesc ));
		AddToTail( profileDesc );
		if( !strcmp( filenames[j], curprofile ))
			iHighlight = Count() - 1;
	}
}

/*
=================
UI_TouchOptions_SetConfig
=================
*/
void CMenuTouchOptions::SaveAndPopMenu( void )
{
	grid.WriteCvar();
	gridsize.WriteCvar();
	lookX.WriteCvar();
	lookY.WriteCvar();
	moveX.WriteCvar();
	moveY.WriteCvar();
	enable.WriteCvar();
	nomouse.WriteCvar();
	acceleration.WriteCvar();
	power.WriteCvar();
	multiplier.WriteCvar();
	exponent.WriteCvar();

	CMenuFramework::SaveAndPopMenu();
}

void CMenuTouchOptions::GetConfig( void )
{
	grid.UpdateEditable();
	gridsize.UpdateEditable();
	lookX.UpdateEditable();
	lookY.UpdateEditable();
	moveX.UpdateEditable();
	moveY.UpdateEditable();
	enable.UpdateEditable();
	nomouse.UpdateEditable();
	acceleration.UpdateEditable();
	power.UpdateEditable();
	multiplier.UpdateEditable();
	exponent.UpdateEditable();
}

void CMenuTouchOptions::ResetMsgBox()
{
	msgBox.SetMessage( L( "Reset sensitivity options?" ) );
	msgBox.onPositive = VoidCb( &CMenuTouchOptions::ResetButtonsCb );
	msgBox.Show();
}

void CMenuTouchOptions::DeleteMsgBox()
{
	msgBox.SetMessage( L( "Delete selected profile?" ) );
	msgBox.onPositive = VoidCb( &CMenuTouchOptions::DeleteProfileCb );
	msgBox.Show();
}

void CMenuTouchOptions::Apply()
{
	int i = profiles.GetCurrentIndex();

	// preset selected
	if( i > 0 && i < model.firstProfile - 1 )
	{
		char command[256];
		const char *curconfig = EngFuncs::GetCvarString( "touch_config_file" );
		snprintf( command, sizeof( command ), "exec \"touch_presets/%s\"\n", model[i].Get());
		EngFuncs::ClientCmd( 1, command );

		while( EngFuncs::FileExists( curconfig, true ) )
		{
			char copystring[256];
			char filebase[256];

			COM_FileBase( curconfig, filebase, sizeof( filebase ));

			if( snprintf( copystring, sizeof( copystring ), "touch_profiles/%s (new).cfg", filebase ) > sizeof( copystring ) - 1 )
				break;

			EngFuncs::CvarSetString( "touch_config_file", copystring );
			curconfig = EngFuncs::GetCvarString( "touch_config_file" );
		}
	}
	else if( i == model.firstProfile )
		EngFuncs::ClientCmd( 1, "exec touch.cfg\n" );
	else if( i > model.firstProfile )
	{
		char command[256];
		snprintf( command, sizeof( command ), "exec \"touch_profiles/%s\"\n", model[i].Get());
		EngFuncs::ClientCmd( 1,  command );
	}

	// try save config
	EngFuncs::ClientCmd( 1, "touch_writeconfig\n" );

	// check if it failed ant reset profile to default if it is
	if( !EngFuncs::FileExists( EngFuncs::GetCvarString( "touch_config_file" ), true ) )
	{
		EngFuncs::CvarSetString( "touch_config_file", "touch.cfg" );
		profiles.SetCurrentIndex( model.firstProfile );
	}
	model.Update();
	GetConfig();
}

void CMenuTouchOptions::Save()
{
	if( profilename.GetBuffer()[0] )
	{
		char name[512];

		EngFuncs::CvarSetStringF( "touch_config_file", "touch_profiles/%s.cfg", profilename.GetBuffer() );

		Com_EscapeCommand( name, profilename.GetBuffer(), sizeof( name ));
		EngFuncs::ClientCmdF( 1, "touch_exportconfig \"touch_profiles/%s.cfg\"\n", name );
	}

	model.Update();
	profilename.Clear();
}

void CMenuTouchOptions::UpdateProfilies()
{
	char curprofile[256];
	bool isCurrent = false;
	int idx = profiles.GetCurrentIndex();

	COM_FileBase( EngFuncs::GetCvarString( "touch_config_file" ), curprofile, sizeof( curprofile ));

	if( model.IsValidIndex( idx ))
		isCurrent = !strcmp( curprofile, model[idx] );

	// Scrolllist changed, update available options
	remove.SetGrayed( true );
	if( ( idx > model.firstProfile ) && !isCurrent )
		remove.SetGrayed( false );

	apply.SetGrayed( false );
	if( idx == 0 || idx == model.firstProfile - 1 )
		profiles.SetCurrentIndex( idx + 1 );
	if( isCurrent )
		apply.SetGrayed( true );
}

void CMenuTouchOptions::DeleteProfileCb()
{
	char command[256];

	if( profiles.GetCurrentIndex() <= model.firstProfile )
		return;

	snprintf(command, 256, "touch_deleteprofile \"%s\"\n", model[profiles.GetCurrentIndex()].Get() );
	EngFuncs::ClientCmd( true, command );

	model.Update();
}

void CMenuTouchOptions::ResetButtonsCb()
{
	EngFuncs::ClientCmd( false, "touch_pitch 90\n" );
	EngFuncs::ClientCmd( false, "touch_yaw 120\n" );
	EngFuncs::ClientCmd( false, "touch_forwardzone 0.06\n" );
	EngFuncs::ClientCmd( false, "touch_sidezone 0.06\n" );
	EngFuncs::ClientCmd( false, "touch_grid 1\n" );
	EngFuncs::ClientCmd( false, "touch_grid_count 50\n" );
	EngFuncs::ClientCmd( false, "touch_nonlinear_look 0\n" );
	EngFuncs::ClientCmd( false, "touch_pow_factor 1.3\n" );
	EngFuncs::ClientCmd( false, "touch_pow_mult 400\n" );
	EngFuncs::ClientCmd( true,  "touch_exp_mult 0\n" );

	GetConfig();
}

/*
=================
UI_TouchOptions_Init
=================
*/
void CMenuTouchOptions::_Init( void )
{
	banner.SetPicture(ART_BANNER);

	done.szName = L( "Done" );
	done.SetPicture( PC_DONE );
	done.onReleased = VoidCb( &CMenuTouchOptions::SaveAndPopMenu );

	lookX.SetNameAndStatus( L( "Look X" ), L( "Horizontal look sensitivity" ) );
	lookX.Setup( 50, 500, 5 );
	lookX.LinkCvar( "touch_yaw" );

	lookY.SetNameAndStatus( L( "Look Y" ), L( "Vertical look sensitivity" ) );
	lookY.Setup( 50, 500, 5 );
	lookY.LinkCvar( "touch_pitch" );

	moveX.SetNameAndStatus( L( "Side" ), L( "Side movement sensitity" ) );
	moveX.Setup( 0.02, 1.0, 0.05 );
	moveX.LinkCvar( "touch_sidezone" );

	moveY.SetNameAndStatus( L( "Forward" ), L( "Forward movement sensitivity" ) );
	moveY.Setup( 0.02, 1.0, 0.05 );
	moveY.LinkCvar( "touch_forwardzone" );

	gridsize.szStatusText = L( "Set grid size" );
	gridsize.Setup( 25, 100, 5 );
	gridsize.LinkCvar( "touch_grid_count", CMenuEditable::CVAR_VALUE );

	grid.SetNameAndStatus( L( "Grid" ), L( "Enable or disable grid" ) );
	grid.LinkCvar( "touch_grid_enable" );

	enable.SetNameAndStatus( L( "Enable touch" ), L( "Enable or disable touch controls" ) );
	enable.LinkCvar( "touch_enable" );

	nomouse.SetNameAndStatus( L( "Ignore mouse" ), L( "Ignore mouse input" ) );
	nomouse.LinkCvar( "m_ignore" );

	acceleration.SetNameAndStatus( L( "Enable acceleration" ), L( "Nonlinear looking (touch_nonlinear_look)" ) );
	acceleration.LinkCvar( "touch_nonlinear_look" );

	power.SetNameAndStatus( L( "Power factor" ), L( "Power acceleration factor (touch_pow_factor)" ) );
	power.Setup( 1, 1.7, 0.05 );
	power.LinkCvar( "touch_pow_factor" );

	multiplier.SetNameAndStatus( L( "Power multiplier" ), L( "Pre-multiplier for pow (touch_pow_mult)" ) );
	multiplier.Setup( 100, 1000, 1 );
	multiplier.LinkCvar( "touch_pow_mult" );

	exponent.SetNameAndStatus( L( "Exponent" ), L( "Exponent factor, more agressive (touch_exp_mult)" ) );
	exponent.Setup( 0, 100, 1 );
	exponent.LinkCvar( "touch_exp_mult" );

	profiles.SetModel( &model );
	UpdateProfilies();
	profiles.onChanged = VoidCb( &CMenuTouchOptions::UpdateProfilies );

	profilename.szName = L( "New Profile:" );
	profilename.iMaxLength = 16;

	reset.szName = L( "Reset" );
	reset.SetPicture("gfx/shell/btn_touch_reset");
	reset.onReleased = VoidCb( &CMenuTouchOptions::ResetMsgBox );

	remove.SetNameAndStatus( L( "Delete" ), L( "Delete saved game" ) );
	remove.SetPicture( PC_DELETE );
	remove.onReleased = VoidCb( &CMenuTouchOptions::DeleteMsgBox );

	apply.SetNameAndStatus( L( "Activate" ), L( "Apply selected profile" ) );
	apply.SetPicture( PC_ACTIVATE );
	apply.onReleased = VoidCb( &CMenuTouchOptions::Apply );

	save.SetNameAndStatus( L( "GameUI_Save" ), L( "Save new profile" ) );
	save.SetPicture("gfx/shell/btn_touch_save");
	save.onReleased = VoidCb( &CMenuTouchOptions::Save );

	msgBox.SetPositiveButton( L( "GameUI_OK" ), PC_OK );
	msgBox.Link( this );
	
	AddItem( banner );
	AddItem( done );
	AddItem( lookX );
	AddItem( lookY );
	AddItem( moveX );
	AddItem( moveY );
	AddItem( reset );
	AddItem( profiles );
	AddItem( save );
	AddItem( profilename );
	AddItem( remove );
	AddItem( apply );
	AddItem( grid );
	AddItem( gridsize );
	AddItem( enable );
	AddItem( nomouse );
	AddItem( acceleration );
	AddItem( power );
	AddItem( multiplier );
	AddItem( exponent );
}

void CMenuTouchOptions::_VidInit( void )
{
	int sliders_x = 72;
	int sliders_w = 210;
	int profile_x = 330;
	int profile_w = 320 + uiStatic.width - 1024;
	int other_x = 680 + uiStatic.width - 1024;

	lookX.SetCoord( sliders_x, 280 );
	lookY.SetCoord( sliders_x, 340 );
	moveX.SetCoord( sliders_x, 400 );
	moveY.SetCoord( sliders_x, 460 );
	gridsize.SetRect( sliders_x, 580, sliders_w, 30 );
	grid.SetCoord( sliders_x, 520 );
	done.SetCoord ( sliders_x, 680 );
	reset.SetCoord( sliders_x, 630 );

	profiles.SetRect( profile_x, 230, profile_w, 270 );
	profilename.SetRect( profile_x, 610, 205, 32 );
	save.SetCoord( profile_x + 220, 610 );
	remove.SetCoord( profile_x + profile_w - 120, 510 );
	remove.size.w = 100;
	apply.SetCoord( profile_x + 30, 510 );
	apply.size.w = 120;

	enable.SetCoord( other_x, 255 );
	nomouse.SetCoord( other_x, 305 );
	acceleration.SetCoord( other_x, 355 );
	power.SetCoord( other_x, 455 );
	multiplier.SetCoord( other_x, 555 );
	exponent.SetCoord( other_x, 655 );

}

ADD_MENU( menu_touchoptions, CMenuTouchOptions, UI_TouchOptions_Menu );
