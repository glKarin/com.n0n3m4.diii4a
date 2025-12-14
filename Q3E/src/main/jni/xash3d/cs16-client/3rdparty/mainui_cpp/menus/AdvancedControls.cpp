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
#include "kbutton.h"
#include "MenuStrings.h"
#include "Bitmap.h"
#include "PicButton.h"
#include "CheckBox.h"
#include "Slider.h"

#define ART_BANNER			"gfx/shell/head_advanced"

class CAdvancedControls : public CMenuFramework
{
public:
	typedef CMenuFramework BaseClass;
	CAdvancedControls() : CMenuFramework("CAdvancedControls") { }

	void ToggleLookCheckboxes( bool write );

private:
	void _Init( void ) override;
	void _VidInit( void ) override;
	void SaveAndPopMenu() override;

	void GetConfig( void );
	void PitchInvert( void );

	CMenuPicButton done, inputDev;

	CMenuCheckBox	crosshair;
	CMenuCheckBox	invertMouse;
	CMenuCheckBox	mouseLook;
	CMenuCheckBox	lookSpring;
	CMenuCheckBox	lookStrafe;
	CMenuCheckBox	lookFilter;
	CMenuCheckBox	autoaim;
	CMenuCheckBox	rawinput;
	CMenuSlider	sensitivity;
};

/*
=================
UI_AdvControls_GetConfig
=================
*/
void CAdvancedControls::GetConfig( )
{
	kbutton_t	*mlook;

	if( EngFuncs::GetCvarFloat( "m_pitch" ) < 0 )
		invertMouse.bChecked = true;

	mlook = (kbutton_t *)EngFuncs::KEY_GetState( "in_mlook" );
	if( mlook )
	{
		if( mlook && mlook->state & 1 )
			mouseLook.bChecked = true;
		else
			mouseLook.bChecked = false;
	}
	else
	{
		mouseLook.SetGrayed( true );
		mouseLook.bChecked = true;
	}

	crosshair.LinkCvar( "crosshair" );
	lookSpring.LinkCvar( "lookspring" );
	lookStrafe.LinkCvar( "lookstrafe" );
	lookFilter.LinkCvar( "look_filter" );

	autoaim.LinkCvar( "sv_aim" );
	rawinput.LinkCvar( "m_rawinput" );
	sensitivity.LinkCvar( "sensitivity" );

	ToggleLookCheckboxes( false );
}

void CAdvancedControls::PitchInvert()
{
	bool invert = invertMouse.bChecked;
	float m_pitch = EngFuncs::GetCvarFloat( "m_pitch" );
	if( ( invert && (m_pitch > 0) ) ||
		( !invert && (m_pitch < 0) ) )
	{
		EngFuncs::CvarSetValue( "m_pitch", -m_pitch );
	}
}

void CAdvancedControls::ToggleLookCheckboxes( bool write )
{
	lookSpring.SetGrayed( mouseLook.bChecked );
	lookStrafe.SetGrayed( mouseLook.bChecked );

	if( write )
	{
		if( mouseLook.bChecked )
			EngFuncs::ClientCmd( false, "+mlook\nbind _force_write\n" );
		else
			EngFuncs::ClientCmd( false, "-mlook\nbind _force_write\n" );
	}
}

void CAdvancedControls::SaveAndPopMenu()
{
	crosshair.WriteCvar();
	lookSpring.WriteCvar();
	lookStrafe.WriteCvar();
	lookFilter.WriteCvar();
	if( EngFuncs::GetCvarString("m_filter")[0] )
		EngFuncs::CvarSetValue( "m_filter", lookFilter.bChecked );
	autoaim.WriteCvar();
	rawinput.WriteCvar();
	sensitivity.WriteCvar();

	ToggleLookCheckboxes( true );

	CMenuFramework::SaveAndPopMenu();
}

/*
=================
UI_AdvControls_Init
=================
*/
void CAdvancedControls::_Init( void )
{
	banner.SetPicture( ART_BANNER );

	done.szName = L( "Done" );
	done.SetPicture( PC_DONE );
	done.onReleased = VoidCb( &CAdvancedControls::SaveAndPopMenu );
	done.SetCoord( 72, 710 );

	crosshair.SetNameAndStatus( L( "Crosshair" ), L( "Enable the weapon aiming crosshair" ) );
	crosshair.iFlags |= QMF_NOTIFY;
	crosshair.SetCoord( 72, 260 );

	invertMouse.SetNameAndStatus( L( "GameUI_ReverseMouse" ), L( "GameUI_ReverseMouseLabel" ) );
	invertMouse.iFlags |= QMF_NOTIFY;
	invertMouse.onChanged = VoidCb( &CAdvancedControls::PitchInvert );
	invertMouse.SetCoord( 72, 310 );

	mouseLook.SetNameAndStatus( L( "GameUI_MouseLook" ), L( "GameUI_MouseLookLabel" ) );
	mouseLook.iFlags |= QMF_NOTIFY;
	SET_EVENT( mouseLook.onChanged,
		((CAdvancedControls*)pSelf->Parent())->ToggleLookCheckboxes( true ) );
	mouseLook.SetCoord( 72, 360 );

	lookSpring.SetNameAndStatus( L( "Look spring" ), L( "Causes the screen to 'spring' back to looking straight ahead when you move forward" ) );
	lookSpring.iFlags |= QMF_NOTIFY;
	lookSpring.SetCoord( 72, 410 );

	lookStrafe.SetNameAndStatus( L( "Look strafe" ), L( "In combination with your mouse look modifier, causes left-right movements to strafe instead of turn" ) );
	lookStrafe.iFlags |= QMF_NOTIFY;
	lookStrafe.SetCoord( 72, 460 );

	lookFilter.SetNameAndStatus( L( "GameUI_MouseFilter" ), L( "GameUI_MouseFilterLabel" ) );
	lookFilter.iFlags |= QMF_NOTIFY;
	lookFilter.SetCoord( 72, 510 );

	autoaim.SetNameAndStatus( L( "GameUI_AutoAim" ), L( "GameUI_AutoaimLabel" ) );
	autoaim.iFlags |= QMF_NOTIFY;
	autoaim.SetCoord( 72, 560 );
	
	rawinput.SetNameAndStatus( L( "GameUI_RawInput" ), L( "GameUI_RawInputLabel" ) );
	rawinput.iFlags |= QMF_NOTIFY;
	rawinput.SetCoord( 72, 610 );

	sensitivity.szName = L( "GameUI_MouseSensitivity" );
	sensitivity.Setup( 0.0, 20.0f, 0.1 );
	sensitivity.SetCoord( 72, 690 );

	inputDev.SetNameAndStatus( L( "Input devices" ), L( "Toggle mouse, touch controls" ) );
	inputDev.onReleased = UI_InputDevices_Menu;
	inputDev.iFlags |= QMF_NOTIFY;
	if( CL_IsActive() && !EngFuncs::GetCvarFloat( "host_serverstate" ))
		inputDev.SetGrayed( true );
	//inputDev.SetRect( 72, 230, UI_BUTTONS_WIDTH, UI_BUTTONS_HEIGHT );
	inputDev.SetCoord( 72, 210 );

	AddItem( banner );
	AddItem( done );
	AddItem( inputDev );
	AddItem( crosshair );
	AddItem( invertMouse );
	AddItem( mouseLook );
	AddItem( lookSpring );
	AddItem( lookStrafe );
	AddItem( lookFilter );
	AddItem( autoaim );
	AddItem( rawinput );
	AddItem( sensitivity );
}


void CAdvancedControls::_VidInit()
{
	GetConfig();
}

ADD_MENU( menu_advcontrols, CAdvancedControls, UI_AdvControls_Menu );
