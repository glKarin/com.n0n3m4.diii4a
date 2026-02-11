/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "sys/platform.h"
#include "idlib/math/Vector.h"
#include "idlib/Lib.h"
#include "framework/CVarSystem.h"
#include "framework/KeyInput.h"
#include "framework/async/AsyncNetwork.h"

#include "framework/UsercmdGen.h"
#include "framework/EventLoop.h"
#include "framework/Console.h"

/*
================
usercmd_t::ByteSwap
================
*/
void usercmd_t::ByteSwap( void ) {
	angles[0] = LittleShort( angles[0] );
	angles[1] = LittleShort( angles[1] );
	angles[2] = LittleShort( angles[2] );
	sequence = LittleInt( sequence );
}

/*
================
usercmd_t::operator==
================
*/
bool usercmd_t::operator==( const usercmd_t &rhs ) const {
	return ( buttons == rhs.buttons &&
			forwardmove == rhs.forwardmove &&
			rightmove == rhs.rightmove &&
			upmove == rhs.upmove &&
			angles[0] == rhs.angles[0] &&
			angles[1] == rhs.angles[1] &&
			angles[2] == rhs.angles[2] &&
			impulse == rhs.impulse &&
			flags == rhs.flags &&
			mx == rhs.mx &&
			my == rhs.my );
}

typedef struct {
	const char *string;
	usercmdButton_t	button;
} userCmdString_t;

userCmdString_t	userCmdStrings[] = {
	{ "_moveUp",		UB_UP },
	{ "_moveDown",		UB_DOWN },
	{ "_left",			UB_LEFT },
	{ "_right",			UB_RIGHT },
	{ "_forward",		UB_FORWARD },
	{ "_back",			UB_BACK },
	{ "_lookUp",		UB_LOOKUP },
	{ "_lookDown",		UB_LOOKDOWN },
	{ "_strafe",		UB_STRAFE },
	{ "_moveLeft",		UB_MOVELEFT },
	{ "_moveRight",		UB_MOVERIGHT },

	{ "_attack",		UB_ATTACK },
	{ "_speed",			UB_SPEED },
	{ "_zoom",			UB_ZOOM },
	{ "_contextmenu",	UB_CONTEXTMENU }, //BC make this lines up with usercmdButton_t enum above!
	{ "_frob",			UB_FROB },
	{ "_lean",			UB_LEAN },
	{ "_reload",		UB_RELOAD },
	{ "_rackslide",		UB_RACKSLIDE },
	{ "_quickthrow",	UB_QUICKTHROW },
	{ "_bash",			UB_BASH },

	{ "_button0",		UB_BUTTON0 },
	{ "_button1",		UB_BUTTON1 },
	{ "_button2",		UB_BUTTON2 },
	{ "_button3",		UB_BUTTON3 },
	{ "_button4",		UB_BUTTON4 },
	{ "_button5",		UB_BUTTON5 },
	{ "_button6",		UB_BUTTON6 },
	{ "_button7",		UB_BUTTON7 },

	{ "_impulse0",		UB_IMPULSE0 },
	{ "_impulse1",		UB_IMPULSE1 },
	{ "_impulse2",		UB_IMPULSE2 },
	{ "_impulse3",		UB_IMPULSE3 },
	{ "_impulse4",		UB_IMPULSE4 },
	{ "_impulse5",		UB_IMPULSE5 },
	{ "_impulse6",		UB_IMPULSE6 },
	{ "_impulse7",		UB_IMPULSE7 },
	{ "_impulse8",		UB_IMPULSE8 },
	{ "_impulse9",		UB_IMPULSE9 },
	{ "_impulse10",		UB_IMPULSE10 },
	{ "_impulse11",		UB_IMPULSE11 },
	{ "_impulse12",		UB_IMPULSE12 },
	{ "_impulse13",		UB_IMPULSE13 },
	{ "_impulse14",		UB_IMPULSE14 },
	{ "_impulse15",		UB_IMPULSE15 },
	{ "_impulse16",		UB_IMPULSE16 },
	{ "_impulse17",		UB_IMPULSE17 },
	{ "_impulse18",		UB_IMPULSE18 },
	{ "_impulse19",		UB_IMPULSE19 },
	{ "_impulse20",		UB_IMPULSE20 },
	{ "_impulse21",		UB_IMPULSE21 },
	{ "_impulse22",		UB_IMPULSE22 },
	{ "_impulse23",		UB_IMPULSE23 },
	{ "_impulse24",		UB_IMPULSE24 },
	{ "_impulse25",		UB_IMPULSE25 },
	{ "_impulse26",		UB_IMPULSE26 },
	{ "_impulse27",		UB_IMPULSE27 },
	{ "_impulse28",		UB_IMPULSE28 },
	{ "_impulse29",		UB_IMPULSE29 },
	{ "_impulse30",		UB_IMPULSE30 },
	{ "_impulse31",		UB_IMPULSE31 },
	{ "_impulse32",		UB_IMPULSE32 },
	{ "_impulse33",		UB_IMPULSE33 },
	{ "_impulse34",		UB_IMPULSE34 },
	{ "_impulse35",		UB_IMPULSE35 },
	{ "_impulse36",		UB_IMPULSE36 },
	{ "_impulse37",		UB_IMPULSE37 },
	{ "_impulse38",		UB_IMPULSE38 },
	{ "_impulse39",		UB_IMPULSE39 },
	{ "_impulse40",		UB_IMPULSE40 },
	{ "_impulse41",		UB_IMPULSE41 },
	{ "_impulse42",		UB_IMPULSE42 },
	{ "_impulse43",		UB_IMPULSE43 },
	{ "_impulse44",		UB_IMPULSE44 },
	{ "_impulse45",		UB_IMPULSE45 },
	{ "_impulse46",		UB_IMPULSE46 },
	{ "_impulse47",		UB_IMPULSE47 },
	{ "_impulse48",		UB_IMPULSE48 },
	{ "_impulse49",		UB_IMPULSE49 },
	{ "_impulse50",		UB_IMPULSE50 },
	{ "_impulse51",		UB_IMPULSE51 },
	{ "_impulse52",		UB_IMPULSE52 },
	{ "_impulse53",		UB_IMPULSE53 },
	{ "_impulse54",		UB_IMPULSE54 },
	{ "_impulse55",		UB_IMPULSE55 },
	{ "_impulse56",		UB_IMPULSE56 },
	{ "_impulse57",		UB_IMPULSE57 },
	{ "_impulse58",		UB_IMPULSE58 },
	{ "_impulse59",		UB_IMPULSE59 },
	{ "_impulse60",		UB_IMPULSE60 },
	{ "_impulse61",		UB_IMPULSE61 },
	{ "_impulse62",		UB_IMPULSE62 },
	{ "_impulse63",		UB_IMPULSE63 },

	{ NULL,				UB_NONE },
};

 class buttonState_t {
 public:
	int		on;
	bool	held;

			buttonState_t() { Clear(); };
	void	Clear( void );
	void	SetKeyState( int keystate, bool toggle );
};

/*
================
buttonState_t::Clear
================
*/
void buttonState_t::Clear( void ) {
	held = false;
	on = 0;
}

/*
================
buttonState_t::SetKeyState
================
*/
void buttonState_t::SetKeyState( int keystate, bool toggle ) {
	if ( !toggle ) {
		held = false;
		on = keystate;
	} else if ( !keystate ) {
		held = false;
	} else if ( !held ) {
		held = true;
		on ^= 1;
	}
}


const int NUM_USER_COMMANDS = sizeof(userCmdStrings) / sizeof(userCmdString_t);

const int MAX_CHAT_BUFFER = 127;

class idUsercmdGenLocal : public idUsercmdGen {
public:
					idUsercmdGenLocal( void );

	void			Init( void );

	void			InitForNewMap( void );

	void			Shutdown( void );

	void			Clear( void );

	void			ClearAngles( void );

	usercmd_t		TicCmd( int ticNumber );

	void			InhibitUsercmd( inhibit_t subsystem, bool inhibit );

	void			UsercmdInterrupt( void );

	int				CommandStringUsercmdData( const char *cmdString );

	int				GetNumUserCommands( void );

	const char *	GetUserCommandName( int index );

	void			MouseState( int *x, int *y, int *button, bool *down );

	int				ButtonState( int key );
	int				KeyState( int key );

	usercmd_t		GetDirectUsercmd( void );
	virtual bool	IsUsingJoystick() const { return isUsingJoystick; }
	float			JoystickAxisState(int axis) const { return joystickAxis[axis]; }

	int*			ButtonStates() { return buttonState; }
	int*			PrevButtonStates() { return prevButtonState; }

private:
	void			MakeCurrent( void );
	void			InitCurrent( void );

	bool			Inhibited( void );
	void			AdjustAngles( void );
	void			KeyMove( void );
	void			JoystickMove( void );
	void			JoystickMouseUI();

	//QC
	void			CircleToSquare(float & axis_x, float & axis_y) const;
	void			HandleJoystickAxis(int keyNum, float unclampedValue, float threshold, bool positive);

	void			MouseMove( void );
	void			CmdButtons( void );

	void			Mouse( void );
	void			Keyboard( void );
	void			Joystick( void );

	void			Key( int keyNum, bool down );

	idVec3			viewangles;
	int				flags;
	int				impulse;

	buttonState_t	toggled_run;
	buttonState_t	toggled_zoom;

	int				buttonState[UB_MAX_BUTTONS];
	int				prevButtonState[UB_MAX_BUTTONS]; // SM
	bool			keyState[K_LAST_KEY];

	int				inhibitCommands;	// true when in console or menu locally
	int				lastCommandTime;

	bool			initialized;

	usercmd_t		cmd;		// the current cmd being built
	usercmd_t		buffered[MAX_BUFFERED_USERCMD];

	int				continuousMouseX, continuousMouseY;	// for gui event generatioin, never zerod
	int				mouseButton;						// for gui event generatioin
	bool			mouseDown;

	int				mouseDx, mouseDy;	// added to by mouse events
	
	//QC
	float			joystickAxis[MAX_JOYSTICK_AXIS];	// set by joystick events
	int				pollTime;
	int				lastPollTime;
	float			lastLookValuePitch;
	float			lastLookValueYaw;
	bool			isUsingJoystick;

};

idCVar in_yawSpeed( "in_yawspeed", "140", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_FLOAT, "yaw change speed when holding down _left or _right button" );
idCVar in_pitchSpeed( "in_pitchspeed", "140", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_FLOAT, "pitch change speed when holding down look _lookUp or _lookDown button" );
idCVar in_angleSpeedKey( "in_anglespeedkey", "1.5", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_FLOAT, "angle change scale when holding down _speed button" );
idCVar in_freeLook( "in_freeLook", "1", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_BOOL, "look around with mouse" );
idCVar in_alwaysRun( "in_alwaysRun", "0", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_BOOL, "always run (reverse _speed button) - only in MP" );
idCVar in_toggleRun( "in_toggleRun", "0", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_BOOL, "pressing _speed button toggles run on/off - only in MP" );
idCVar in_toggleZoom( "in_toggleZoom", "0", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_BOOL, "pressing _zoom button toggles zoom on/off" );
idCVar m_sensitivity( "sensitivity", "3", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_FLOAT, "mouse view sensitivity" );
idCVar m_pitch( "m_pitch", "0.022", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_FLOAT, "mouse pitch scale" );
idCVar m_yaw( "m_yaw", "0.022", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_FLOAT, "mouse yaw scale" );
idCVar m_strafeScale( "m_strafeScale", "6.25", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_FLOAT, "mouse strafe movement scale" );
idCVar m_smooth( "m_smooth", "1", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_INTEGER, "number of samples blended for mouse viewing", 1, 8, idCmdSystem::ArgCompletion_Integer<1,8> );
idCVar m_strafeSmooth( "m_strafeSmooth", "1", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_INTEGER, "number of samples blended for mouse moving", 1, 8, idCmdSystem::ArgCompletion_Integer<1,8> );
idCVar m_showMouseRate( "m_showMouseRate", "0", CVAR_SYSTEM | CVAR_BOOL, "shows mouse movement" );

//QC
idCVar joy_triggerThreshold("joy_triggerThreshold", "0.05", CVAR_FLOAT | CVAR_ARCHIVE, "how far the joystick triggers have to be pressed before they register as down");
idCVar joy_deadZone("joy_deadZone", "0.25", CVAR_FLOAT | CVAR_ARCHIVE, "specifies how large the dead-zone is on the joystick");
idCVar joy_gammaLook("joy_gammaLook", "1", CVAR_INTEGER | CVAR_ARCHIVE, "Look: use log curve instead of a power curve for looking.");
idCVar joy_powerScale("joy_powerScale", "2", CVAR_FLOAT | CVAR_ARCHIVE, "Look: raise joystick values to this power.");
idCVar joy_pitchSpeed("joy_pitchSpeed", "30", CVAR_ARCHIVE | CVAR_FLOAT, "pitch speed when pressing up or down on the joystick", 1, 600);
idCVar joy_yawSpeed("joy_yawSpeed", "40", CVAR_ARCHIVE | CVAR_FLOAT, "pitch speed when pressing left or right on the joystick", 1, 600);
idCVar joy_dampenLook("joy_dampenLook", "1", CVAR_BOOL | CVAR_ARCHIVE, "Look: dampen full acceleration on look.");
idCVar joy_deltaPerMSLook("joy_deltaPerMSLook", "0.0005", CVAR_FLOAT | CVAR_ARCHIVE, "Look: acceleration to be added on look.");
idCVar joy_invertPitch("joy_invertPitch", "0", CVAR_BOOL | CVAR_ARCHIVE, "If true, invert joystick pitch");
idCVar in_useJoystick("in_useJoystick", "1", CVAR_ARCHIVE | CVAR_BOOL, "enables/disables the gamepad for PC use");
idCVar in_forceButtonPrompts("in_forceButtonPrompts", "-1", CVAR_ARCHIVE | CVAR_INTEGER, "force button prompts to a specific type (or -1 for automatic)", -1, CT_NUM + 1);
idCVar joy_mouseSpeed("joy_mouseSpeed", "12", CVAR_ARCHIVE | CVAR_FLOAT, "How fast the joystick moves the mouse cursor in full-screen UI mode (5-100)", 5, 100);
idCVar joy_mouseFriction("joy_mouseFriction", "0.3", CVAR_ARCHIVE | CVAR_FLOAT, "Multiplier to apply to gamepad mouse movement speed when focusing on an interactive element (0.1-1)", 0.1, 1.0);


static idUsercmdGenLocal localUsercmdGen;
idUsercmdGen	*usercmdGen = &localUsercmdGen;

/*
================
idUsercmdGenLocal::idUsercmdGenLocal
================
*/
idUsercmdGenLocal::idUsercmdGenLocal( void ) {
	lastCommandTime = 0;
	initialized = false;

	flags = 0;
	impulse = 0;

	toggled_run.Clear();
	toggled_zoom.Clear();
	toggled_run.on = in_alwaysRun.GetBool();

	ClearAngles();
	Clear();
}

/*
================
idUsercmdGenLocal::InhibitUsercmd
================
*/
void idUsercmdGenLocal::InhibitUsercmd( inhibit_t subsystem, bool inhibit ) {
	if ( inhibit ) {
		inhibitCommands |= 1 << subsystem;
	} else {
		inhibitCommands &= ( 0xffffffff ^ ( 1 << subsystem ) );
	}
}

/*
===============
idUsercmdGenLocal::ButtonState

Returns (the fraction of the frame) that the key was down
===============
*/
int	idUsercmdGenLocal::ButtonState( int key ) {
	if ( key<0 || key>=UB_MAX_BUTTONS ) {
		return -1;
	}
	return ( buttonState[key] > 0 ) ? 1 : 0;
}

/*
===============
idUsercmdGenLocal::KeyState

Returns (the fraction of the frame) that the key was down
bk20060111
===============
*/
int	idUsercmdGenLocal::KeyState( int key ) {
	if ( key<0 || key>=K_LAST_KEY ) {
		return -1;
	}
	return ( keyState[key] ) ? 1 : 0;
}


//=====================================================================


/*
================
idUsercmdGenLocal::GetNumUserCommands
================
*/
int idUsercmdGenLocal::GetNumUserCommands( void ) {
	return NUM_USER_COMMANDS;
}

/*
================
idUsercmdGenLocal::GetNumUserCommands
================
*/
const char *idUsercmdGenLocal::GetUserCommandName( int index ) {
	if (index >= 0 && index < NUM_USER_COMMANDS) {
		return userCmdStrings[index].string;
	}
	return "";
}

/*
================
idUsercmdGenLocal::Inhibited

is user cmd generation inhibited
================
*/
bool idUsercmdGenLocal::Inhibited( void ) {
	return ( inhibitCommands != 0);
}

/*
================
idUsercmdGenLocal::AdjustAngles

Moves the local angle positions
================
*/
void idUsercmdGenLocal::AdjustAngles( void ) {
	float	speed;

	if ( toggled_run.on ^ ( in_alwaysRun.GetBool() && idAsyncNetwork::IsActive() ) ) {
		speed = idMath::M_MS2SEC * USERCMD_MSEC * in_angleSpeedKey.GetFloat();
	} else {
		speed = idMath::M_MS2SEC * USERCMD_MSEC;
	}

	if ( !ButtonState( UB_STRAFE ) ) {
		viewangles[YAW] -= speed * in_yawSpeed.GetFloat() * ButtonState( UB_RIGHT );
		viewangles[YAW] += speed * in_yawSpeed.GetFloat() * ButtonState( UB_LEFT );
	}

	viewangles[PITCH] -= speed * in_pitchSpeed.GetFloat() * ButtonState( UB_LOOKUP );
	viewangles[PITCH] += speed * in_pitchSpeed.GetFloat() * ButtonState( UB_LOOKDOWN );
}

/*
================
idUsercmdGenLocal::KeyMove

Sets the usercmd_t based on key states
================
*/
void idUsercmdGenLocal::KeyMove( void ) {
	int		forward, side, up;

	forward = 0;
	side = 0;
	up = 0;
	if ( ButtonState( UB_STRAFE ) ) {
		side += KEY_MOVESPEED * ButtonState( UB_RIGHT );
		side -= KEY_MOVESPEED * ButtonState( UB_LEFT );
	}

	side += KEY_MOVESPEED * ButtonState( UB_MOVERIGHT );
	side -= KEY_MOVESPEED * ButtonState( UB_MOVELEFT );

	up -= KEY_MOVESPEED * ButtonState( UB_DOWN );
	up += KEY_MOVESPEED * ButtonState( UB_UP );

	forward += KEY_MOVESPEED * ButtonState( UB_FORWARD );
	forward -= KEY_MOVESPEED * ButtonState( UB_BACK );

#ifdef _DIII4A //karin: joystick smooth movement on Android
    Sys_Analog(side, forward, KEY_MOVESPEED);
#endif

	cmd.forwardmove = idMath::ClampChar( forward );
	cmd.rightmove = idMath::ClampChar( side );
	cmd.upmove = idMath::ClampChar( up );
}

/*
=================
idUsercmdGenLocal::MouseMove
=================
*/
void idUsercmdGenLocal::MouseMove( void ) {
	float		mx, my, strafeMx, strafeMy;
	static int	history[8][2];
	static int	historyCounter;
	int			i;


	// blendo eric: modify exceeding large mouse movement input check to use unsmoothed mouse accel instead of delta
	// with frame time, sensitivity, and stutter taken into account
	float pollTimeDelta = pollTime - lastPollTime;

	static float lastUnsmoothX = 0;
	static float lastUnsmoothY = 0;
	float unsmoothMx = mouseDx * m_sensitivity.GetFloat();
	float unsmoothMy = mouseDy * m_sensitivity.GetFloat();

	float accelAbsX = idMath::Fabs(unsmoothMx - lastUnsmoothX) / pollTimeDelta; // accel dots per second
	float accelAbsY = idMath::Fabs(unsmoothMy - lastUnsmoothY) / pollTimeDelta;

	//Sys_DebugPrintf("accel %.2f, %.2f \n", accelAbsX, accelAbsY);

	lastUnsmoothX = unsmoothMx;
	lastUnsmoothY = unsmoothMy;

	const float MOUSE_ACCEL_PER_MS_MAX = 1000.0f / (1000 / 30); // prev max input speed divided by 30fps frametime (1000px / (1000ms/30ms)) = ~30
	bool deltaOutOfRange = pollTimeDelta > 500; // there was probably a hiccup / stutter if the frame time was large

	if (deltaOutOfRange || accelAbsX > MOUSE_ACCEL_PER_MS_MAX || accelAbsY > MOUSE_ACCEL_PER_MS_MAX) {
		Sys_DebugPrintf("idUsercmdGenLocal::MouseMove: Ignoring ridiculous mouse accel delta %.2f,%.2f\n", accelAbsX, accelAbsY);
		mouseDx = mouseDy = 0;
	}

	history[historyCounter&7][0] = mouseDx;
	history[historyCounter&7][1] = mouseDy;

	// allow mouse movement to be smoothed together
	int smooth = m_smooth.GetInteger();
	if ( smooth < 1 ) {
		smooth = 1;
	}
	if ( smooth > 8 ) {
		smooth = 8;
	}
	mx = 0;
	my = 0;
	for ( i = 0 ; i < smooth ; i++ ) {
		mx += history[ ( historyCounter - i + 8 ) & 7 ][0];
		my += history[ ( historyCounter - i + 8 ) & 7 ][1];
	}
	mx /= smooth;
	my /= smooth;

	// use a larger smoothing for strafing
	smooth = m_strafeSmooth.GetInteger();
	if ( smooth < 1 ) {
		smooth = 1;
	}
	if ( smooth > 8 ) {
		smooth = 8;
	}
	strafeMx = 0;
	strafeMy = 0;
	for ( i = 0 ; i < smooth ; i++ ) {
		strafeMx += history[ ( historyCounter - i + 8 ) & 7 ][0];
		strafeMy += history[ ( historyCounter - i + 8 ) & 7 ][1];
	}
	strafeMx /= smooth;
	strafeMy /= smooth;

	historyCounter++;

	mx *= m_sensitivity.GetFloat();
	my *= m_sensitivity.GetFloat();

	if ( m_showMouseRate.GetBool() ) {
		Sys_DebugPrintf( "[%3i %3i  = %5.1f %5.1f = %5.1f %5.1f] ", mouseDx, mouseDy, mx, my, strafeMx, strafeMy );
	}

	mouseDx = 0;
	mouseDy = 0;

	if ( !strafeMx && !strafeMy ) {
		return;
	}

	if ( ButtonState( UB_STRAFE )  ) {
		// add mouse X/Y movement to cmd
		strafeMx *= m_strafeScale.GetFloat();
		strafeMy *= m_strafeScale.GetFloat();
		// clamp as a vector, instead of separate floats
		float len = sqrt( strafeMx * strafeMx + strafeMy * strafeMy );
		if ( len > 127 ) {
			strafeMx = strafeMx * 127 / len;
			strafeMy = strafeMy * 127 / len;
		}
	}

	if ( !ButtonState( UB_STRAFE ) ) {
		viewangles[YAW] -= m_yaw.GetFloat() * mx;
	} else {
		cmd.rightmove = idMath::ClampChar( (int)(cmd.rightmove + strafeMx) );
	}

	viewangles[PITCH] += m_pitch.GetFloat() * my;
}

/*
=================
idUsercmdGenLocal::JoystickMove
=================
*/
void idUsercmdGenLocal::JoystickMove( void )
{
	/*
	float	anglespeed;

	if ( toggled_run.on ^ ( in_alwaysRun.GetBool() && idAsyncNetwork::IsActive() ) ) {
		anglespeed = idMath::M_MS2SEC * USERCMD_MSEC * in_angleSpeedKey.GetFloat();
	} else {
		anglespeed = idMath::M_MS2SEC * USERCMD_MSEC;
	}

	if ( !ButtonState( UB_STRAFE ) ) {
		viewangles[YAW] += anglespeed * in_yawSpeed.GetFloat() * joystickAxis[AXIS_SIDE];
		viewangles[PITCH] += anglespeed * in_pitchSpeed.GetFloat() * joystickAxis[AXIS_FORWARD];
	} else {
		cmd.rightmove = idMath::ClampChar( cmd.rightmove + joystickAxis[AXIS_SIDE] );
		cmd.forwardmove = idMath::ClampChar( cmd.forwardmove + joystickAxis[AXIS_FORWARD] );
	}

	cmd.upmove = idMath::ClampChar( cmd.upmove + joystickAxis[AXIS_UP] );
	*/

	float threshold = joy_deadZone.GetFloat();
	float triggerThreshold = joy_triggerThreshold.GetFloat();

	float axis_y = joystickAxis[AXIS_LEFT_Y];
	float axis_x = joystickAxis[AXIS_LEFT_X];
	CircleToSquare(axis_x, axis_y);

	HandleJoystickAxis(K_JOY_STICK1_UP, axis_y, threshold, false);
	HandleJoystickAxis(K_JOY_STICK1_DOWN, axis_y, threshold, true);
	HandleJoystickAxis(K_JOY_STICK1_LEFT, axis_x, threshold, false);
	HandleJoystickAxis(K_JOY_STICK1_RIGHT, axis_x, threshold, true);

	axis_y = joystickAxis[AXIS_RIGHT_Y];
	axis_x = joystickAxis[AXIS_RIGHT_X];
	CircleToSquare(axis_x, axis_y);

	HandleJoystickAxis(K_JOY_STICK2_UP, axis_y, threshold, false);
	HandleJoystickAxis(K_JOY_STICK2_DOWN, axis_y, threshold, true);
	HandleJoystickAxis(K_JOY_STICK2_LEFT, axis_x, threshold, false);
	HandleJoystickAxis(K_JOY_STICK2_RIGHT, axis_x, threshold, true);

	HandleJoystickAxis(K_JOY_TRIGGER1, joystickAxis[AXIS_LEFT_TRIG], triggerThreshold, true);
	HandleJoystickAxis(K_JOY_TRIGGER2, joystickAxis[AXIS_RIGHT_TRIG], triggerThreshold, true);
}

extern idCVar joy_swapSticks;

void idUsercmdGenLocal::JoystickMouseUI()
{
	bool swapSticks = joy_swapSticks.GetBool();
	float axis_x = swapSticks ? joystickAxis[AXIS_RIGHT_X] : joystickAxis[AXIS_LEFT_X];
	float axis_y = swapSticks ? joystickAxis[AXIS_RIGHT_Y] : joystickAxis[AXIS_LEFT_Y];
	CircleToSquare(axis_x, axis_y);
	
	float threshold = joy_deadZone.GetFloat();
	idVec2 joyMove(axis_x, axis_y);

	if (joyMove.Length() > threshold) {
		float mouseSpeed = joy_mouseSpeed.GetFloat();
		isUsingJoystick = true;

		sysEvent_t ev;
		ev.evType = SE_MOUSEGAMEPAD;
		ev.evValue = axis_x * mouseSpeed;
		ev.evValue2 = axis_y * mouseSpeed;
		ev.evPtrLength = 0;
		ev.evPtr = nullptr;
		eventLoop->PushEvent(&ev);
	}
}

/*
========================
idUsercmdGenLocal::CircleToSquare
========================
*/
void idUsercmdGenLocal::CircleToSquare(float & axis_x, float & axis_y) const {
	// bring everything in the first quadrant
	bool flip_x = false;
	if (axis_x < 0.0f) {
		flip_x = true;
		axis_x *= -1.0f;
	}
	bool flip_y = false;
	if (axis_y < 0.0f) {
		flip_y = true;
		axis_y *= -1.0f;
	}

	// swap the two axes so we project against the vertical line X = 1
	bool swap = false;
	if (axis_y > axis_x) {
		float tmp = axis_x;
		axis_x = axis_y;
		axis_y = tmp;
		swap = true;
	}

	if (axis_x < 0.001f) {
		// on one of the axes where no correction is needed
		return;
	}

	// length (max 1.0f at the unit circle)
	float len = idMath::Sqrt(axis_x * axis_x + axis_y * axis_y);
	if (len > 1.0f) {
		len = 1.0f;
	}
	// thales
	float axis_y_us = axis_y / axis_x;

	// use a power curve to shift the correction to happen closer to the unit circle
	float correctionRatio = Square(len);
	axis_x += correctionRatio * (len - axis_x);
	axis_y += correctionRatio * (axis_y_us - axis_y);

	// go back through the symmetries
	if (swap) {
		float tmp = axis_x;
		axis_x = axis_y;
		axis_y = tmp;
	}
	if (flip_x) {
		axis_x *= -1.0f;
	}
	if (flip_y) {
		axis_y *= -1.0f;
	}
}

/*
========================
idUsercmdGenLocal::HandleJoystickAxis
========================
*/
void idUsercmdGenLocal::HandleJoystickAxis(int keyNum, float unclampedValue, float threshold, bool positive) {
	if ((unclampedValue > 0.0f) && !positive) {
		return;
	}
	if ((unclampedValue < 0.0f) && positive) {
		return;
	}
	float value = 0.0f;
	bool pressed = false;
	if (unclampedValue > threshold) {
		value = idMath::Fabs((unclampedValue - threshold) / (1.0f - threshold));
		pressed = true;
	}
	else if (unclampedValue < -threshold) {
		value = idMath::Fabs((-unclampedValue - threshold) / (1.0f - threshold));
		pressed = true;
	}

	int action = idKeyInput::GetUsercmdAction(keyNum);
	if (action >= UB_ATTACK) {
		Key(keyNum, pressed);
		return;
	}
	if (!pressed) {
		return;
	}

	isUsingJoystick = true;

	float lookValue = 0.0f;
	if (joy_gammaLook.GetBool()) {
		lookValue = idMath::Pow(1.04712854805f, value * 100.0f) * 0.01f;
	}
	else {
		lookValue = idMath::Pow(value, joy_powerScale.GetFloat());
	}

#if 0 // TODO: aim assist maybe.
	idGame * game = common->Game();
	if (game != NULL) {
		lookValue *= game->GetAimAssistSensitivity();
	}
#endif

	switch (action) {
	case UB_FORWARD: {
		float move = (float)cmd.forwardmove + (KEY_MOVESPEED * value);
		cmd.forwardmove = idMath::ClampChar(idMath::Ftoi(move));
		break;
	}
	case UB_BACK: {
		float move = (float)cmd.forwardmove - (KEY_MOVESPEED * value);
		cmd.forwardmove = idMath::ClampChar(idMath::Ftoi(move));
		break;
	}
	case UB_MOVELEFT: {
		float move = (float)cmd.rightmove - (KEY_MOVESPEED * value);
		cmd.rightmove = idMath::ClampChar(idMath::Ftoi(move));
		break;
	}
	case UB_MOVERIGHT: {
		float move = (float)cmd.rightmove + (KEY_MOVESPEED * value);
		cmd.rightmove = idMath::ClampChar(idMath::Ftoi(move));
		break;
	}
	case UB_LOOKUP: {
		if (joy_dampenLook.GetBool()) {
			lookValue = Min(lookValue, (pollTime - lastPollTime) * joy_deltaPerMSLook.GetFloat() + lastLookValuePitch);
			lastLookValuePitch = lookValue;
		}

		//BC 2-25-2025: fixed gamepad invert.
		float invertPitch = joy_invertPitch.GetBool() ? -1.0f : 1.0f;
		viewangles[PITCH] -= MS2SEC(pollTime - lastPollTime) * lookValue * joy_pitchSpeed.GetFloat() * invertPitch;
		break;
	}
	case UB_LOOKDOWN: {
		if (joy_dampenLook.GetBool()) {
			lookValue = Min(lookValue, (pollTime - lastPollTime) * joy_deltaPerMSLook.GetFloat() + lastLookValuePitch);
			lastLookValuePitch = lookValue;
		}

		//BC 2-25-2025: fixed gamepad invert.
		float invertPitch = joy_invertPitch.GetBool() ? -1.0f : 1.0f;
		viewangles[PITCH] += MS2SEC(pollTime - lastPollTime) * lookValue * joy_pitchSpeed.GetFloat() * invertPitch;
		break;
	}
	case UB_LEFT: {
		if (joy_dampenLook.GetBool()) {
			lookValue = Min(lookValue, (pollTime - lastPollTime) * joy_deltaPerMSLook.GetFloat() + lastLookValueYaw);
			lastLookValueYaw = lookValue;
		}
		viewangles[YAW] += MS2SEC(pollTime - lastPollTime) * lookValue * joy_yawSpeed.GetFloat();
		break;
	}
	case UB_RIGHT: {
		if (joy_dampenLook.GetBool()) {
			lookValue = Min(lookValue, (pollTime - lastPollTime) * joy_deltaPerMSLook.GetFloat() + lastLookValueYaw);
			lastLookValueYaw = lookValue;
		}
		viewangles[YAW] -= MS2SEC(pollTime - lastPollTime) * lookValue * joy_yawSpeed.GetFloat();
		break;
	}
	}
}

/*
==============
idUsercmdGenLocal::CmdButtons
==============
*/
void idUsercmdGenLocal::CmdButtons( void ) {
	int		i;

	cmd.buttons = 0;

	// figure button bits
	for (i = 0 ; i <= 7 ; i++) {
		if ( ButtonState( (usercmdButton_t)( UB_BUTTON0 + i ) ) ) {
			cmd.buttons |= 1 << i;
		}
	}

	// check the attack button
	if ( ButtonState( UB_ATTACK ) ) {
		cmd.buttons |= BUTTON_ATTACK;
	}

	// check the run button
	if ( toggled_run.on ^ ( in_alwaysRun.GetBool() && idAsyncNetwork::IsActive() ) ) {
		cmd.buttons |= BUTTON_RUN;
	}

	// check the zoom button
	if ( toggled_zoom.on ) {
		cmd.buttons |= BUTTON_ZOOM;
	}

	

	//BC
	if (ButtonState(UB_FROB))
	{
		cmd.buttons |= BUTTON_FROB;
	}

	//BC
	if (ButtonState(UB_CONTEXTMENU))
	{
		cmd.buttons |= BUTTON_CONTEXTMENU;
	}

	//BC
	if (ButtonState(UB_LEAN))
	{
		cmd.buttons |= BUTTON_LEAN;
	}

	//bc
	if (ButtonState(UB_RELOAD))
	{
		cmd.buttons |= BUTTON_RELOAD;
	}

	if (ButtonState(UB_RACKSLIDE))
	{
		cmd.buttons |= BUTTON_RACKSLIDE;
	}

	if (ButtonState(UB_QUICKTHROW))
	{
		cmd.buttons |= BUTTON_QUICKTHROW;
	}

	if (ButtonState(UB_BASH))
	{
		cmd.buttons |= BUTTON_BASH;
	}

}

/*
================
idUsercmdGenLocal::InitCurrent

inits the current command for this frame
================
*/
void idUsercmdGenLocal::InitCurrent( void ) {
	memset( &cmd, 0, sizeof( cmd ) );
	cmd.flags = flags;
	cmd.impulse = impulse;
	cmd.buttons |= ( in_alwaysRun.GetBool() && idAsyncNetwork::IsActive() ) ? BUTTON_RUN : 0;

	// SM: Setup button states
	memcpy( prevButtonState, buttonState, sizeof( buttonState ) );

	// eric blendo: altered for idlist, set buttonstate list in makecurrent
	for (int idx = 0; idx < UB_MAX_BUTTONS; ++idx) {
		cmd.prevButtonState[idx] = prevButtonState[idx];
	}
}

/*
================
idUsercmdGenLocal::MakeCurrent

creates the current command for this frame
================
*/
void idUsercmdGenLocal::MakeCurrent( void ) {
	idVec3		oldAngles;
	int		i;

	oldAngles = viewangles;

	if ( !Inhibited() ) {
		// update toggled key states
		toggled_run.SetKeyState( ButtonState( UB_SPEED ), in_toggleRun.GetBool() && idAsyncNetwork::IsActive() );
		toggled_zoom.SetKeyState( ButtonState( UB_ZOOM ), in_toggleZoom.GetBool() );

		// keyboard angle adjustment
		AdjustAngles();

		// set button bits
		CmdButtons();

		// get basic movement from keyboard
		KeyMove();

		// get basic movement from mouse
		MouseMove();

		// get basic movement from joystick
		JoystickMove();

		// check to make sure the angles haven't wrapped
		if ( viewangles[PITCH] - oldAngles[PITCH] > 90 ) {
			viewangles[PITCH] = oldAngles[PITCH] + 90;
		} else if ( oldAngles[PITCH] - viewangles[PITCH] > 90 ) {
			viewangles[PITCH] = oldAngles[PITCH] - 90;
		}
	} else {
		mouseDx = 0;
		mouseDy = 0;

		JoystickMouseUI();
		// SM: It's probably okay to do this as it allows certain special menus
		// (like the context one) to still process on actions player
		if (!console->Active())
		{
			CmdButtons();
		}
	}

	for ( i = 0; i < 3; i++ ) {
		cmd.angles[i] = ANGLE2SHORT( viewangles[i] );
	}

	cmd.mx = continuousMouseX;
	cmd.my = continuousMouseY;

	flags = cmd.flags;
	impulse = cmd.impulse;

	for (int idx = 0; idx < UB_MAX_BUTTONS; ++idx) {
		cmd.buttonState[idx] = buttonState[idx];
	}
}

//=====================================================================


/*
================
idUsercmdGenLocal::CommandStringUsercmdData

Returns the button if the command string is used by the async usercmd generator.
================
*/
int	idUsercmdGenLocal::CommandStringUsercmdData( const char *cmdString ) {
	for ( userCmdString_t *ucs = userCmdStrings ; ucs->string ; ucs++ ) {
		if ( idStr::Icmp( cmdString, ucs->string ) == 0 ) {
			return ucs->button;
		}
	}
	return UB_NONE;
}

/*
================
idUsercmdGenLocal::Init
================
*/
void idUsercmdGenLocal::Init( void ) {
	initialized = true;
}

/*
================
idUsercmdGenLocal::InitForNewMap
================
*/
void idUsercmdGenLocal::InitForNewMap( void ) {
	flags = 0;
	impulse = 0;

	toggled_run.Clear();
	toggled_zoom.Clear();
	toggled_run.on = in_alwaysRun.GetBool();

	Clear();
	ClearAngles();
}

/*
================
idUsercmdGenLocal::Shutdown
================
*/
void idUsercmdGenLocal::Shutdown( void ) {
	initialized = false;
}

/*
================
idUsercmdGenLocal::Clear
================
*/
void idUsercmdGenLocal::Clear( void ) {
	// clears all key states
	memset( buttonState, 0, sizeof( buttonState ) );
	memset( prevButtonState, 0, sizeof( prevButtonState ) ); // SM
	memset( keyState, false, sizeof( keyState ) );

	//QC
	memset(joystickAxis, 0, sizeof(joystickAxis));

	inhibitCommands = false;

	mouseDx = mouseDy = 0;
	mouseButton = 0;
	mouseDown = false;
	isUsingJoystick = false;

	// SM: Default to "true" on steam deck/big picture mode
	if (common->g_SteamUtilities && common->g_SteamUtilities->ShouldDefaultToController())
	{
		isUsingJoystick = true;
	}
}

/*
================
idUsercmdGenLocal::ClearAngles
================
*/
void idUsercmdGenLocal::ClearAngles( void ) {
	viewangles.Zero();
}

/*
================
idUsercmdGenLocal::TicCmd

Returns a buffered usercmd
================
*/
usercmd_t idUsercmdGenLocal::TicCmd( int ticNumber ) {

	// the packetClient code can legally ask for com_ticNumber+1, because
	// it is in the async code and com_ticNumber hasn't been updated yet,
	// but all other code should never ask for anything > com_ticNumber
	if ( ticNumber > com_ticNumber+1 ) {
		common->Error( "idUsercmdGenLocal::TicCmd ticNumber > com_ticNumber" );
	}

	if ( ticNumber <= com_ticNumber - MAX_BUFFERED_USERCMD ) {
		// this can happen when something in the game code hitches badly, allowing the
		// async code to overflow the buffers
		//common->Printf( "warning: idUsercmdGenLocal::TicCmd ticNumber <= com_ticNumber - MAX_BUFFERED_USERCMD\n" );
	}

	return buffered[ ticNumber & (MAX_BUFFERED_USERCMD-1) ];
}

//======================================================================


/*
===================
idUsercmdGenLocal::Key

Handles async mouse/keyboard button actions
===================
*/
void idUsercmdGenLocal::Key( int keyNum, bool down ) {

	// Sanity check, sometimes we get double message :(
	if ( keyState[ keyNum ] == down ) {
		return;
	}
	keyState[ keyNum ] = down;

	int action = idKeyInput::GetUsercmdAction( keyNum );

	if ( down ) {

		buttonState[ action ]++;

		if ( !Inhibited()  ) {
			if ( action >= UB_IMPULSE0 && action <= UB_IMPULSE61 ) {
				cmd.impulse = action - UB_IMPULSE0;
				cmd.flags ^= UCF_IMPULSE_SEQUENCE;
			}
		}
	} else {
		buttonState[ action ]--;
		// we might have one held down across an app active transition
		if ( buttonState[ action ] < 0 ) {
			buttonState[ action ] = 0;
		}
	}
}

/*
===================
idUsercmdGenLocal::Mouse
===================
*/
void idUsercmdGenLocal::Mouse( void ) {
	int i, numEvents;

	numEvents = Sys_PollMouseInputEvents();

	if ( numEvents ) {
		//
		// Study each of the buffer elements and process them.
		//
		for( i = 0; i < numEvents; i++ ) {
			int action, value;
			if ( Sys_ReturnMouseInputEvent( i, action, value ) ) {

				isUsingJoystick = value != 0 ? false : isUsingJoystick;

				if ( action >= M_ACTION1 && action <= M_ACTION8 ) {
					mouseButton = K_MOUSE1 + ( action - M_ACTION1 );
					mouseDown = ( value != 0 );
					Key( mouseButton, mouseDown );
				} else {
					switch ( action ) {
						case M_DELTAX:
							mouseDx += value;
							continuousMouseX += value;
							break;
						case M_DELTAY:
							mouseDy += value;
							continuousMouseY += value;
							break;
						case M_DELTAZ:
							int key = value < 0 ? K_MWHEELDOWN : K_MWHEELUP;
							value = abs( value );
							while( value-- > 0 ) {
								Key( key, true );
								Key( key, false );
								mouseButton = key;
								mouseDown = true;
							}
							break;
					}
				}
			}
		}
	}

	Sys_EndMouseInputEvents();
}

/*
===============
idUsercmdGenLocal::Keyboard
===============
*/
void idUsercmdGenLocal::Keyboard( void ) {

	int numEvents = Sys_PollKeyboardInputEvents();

	if ( numEvents ) {
		isUsingJoystick = false;
		//
		// Study each of the buffer elements and process them.
		//
		int key;
		bool state;
		for( int i = 0; i < numEvents; i++ ) {
			if (Sys_ReturnKeyboardInputEvent( i, key, state )) {
				Key ( key, state );
			}
		}
	}

	Sys_EndKeyboardInputEvents();
}

/*
===============
idUsercmdGenLocal::Joystick
===============
*/
void idUsercmdGenLocal::Joystick( void ) {
	//memset( joystickAxis, 0, sizeof( joystickAxis ) );

	int numEvents = Sys_PollJoystickInputEvents(0);

	if (numEvents) {
		//QC Study each of the buffer elements and process them.
		for (int i = 0; i < numEvents; i++) {
			int action;
			int value;
			if (Sys_ReturnJoystickInputEvent(i, action, value)) {
				if (action >= J_ACTION1 && action <= J_ACTION_MAX) {
					int joyButton = K_JOY1 + (action - J_ACTION1);
					Key(joyButton, (value != 0));
					isUsingJoystick = true;
				}
				else if ((action >= J_AXIS_MIN) && (action <= J_AXIS_MAX)) {
					joystickAxis[action - J_AXIS_MIN] = static_cast<float>(value) / 32767.0f;
				}
				else if (action >= J_DPAD_UP && action <= J_DPAD_RIGHT) {
					int joyButton = K_JOY_DPAD_UP + (action - J_DPAD_UP);
					Key(joyButton, (value != 0));
					isUsingJoystick = true;
				}
				else {
					//assert( !"Unknown joystick event" );
				}
			}
		}
	}

	Sys_EndJoystickInputEvents();
}

/*
================
idUsercmdGenLocal::UsercmdInterrupt

Called asyncronously
================
*/
void idUsercmdGenLocal::UsercmdInterrupt( void ) {
	// dedicated servers won't create usercmds
	if ( !initialized ) {
		return;
	}

	// init the usercmd for com_ticNumber+1
	InitCurrent();

	// process the system mouse events
	Mouse();

	// process the system keyboard events
	Keyboard();

	// process the system joystick events
	if (in_useJoystick.GetBool()) //QC
		Joystick();

	// create the usercmd for com_ticNumber+1
	MakeCurrent();

	// save a number for debugging cmdDemos and networking
	cmd.sequence = com_ticNumber+1;

	buffered[(com_ticNumber+1) & (MAX_BUFFERED_USERCMD-1)] = cmd;
}

/*
================
idUsercmdGenLocal::MouseState
================
*/
void idUsercmdGenLocal::MouseState( int *x, int *y, int *button, bool *down ) {
	*x = continuousMouseX;
	*y = continuousMouseY;
	*button = mouseButton;
	*down = mouseDown;
}

/*
================
idUsercmdGenLocal::GetDirectUsercmd
================
*/
usercmd_t idUsercmdGenLocal::GetDirectUsercmd( void ) {
	// QC
	pollTime = Sys_Milliseconds();
	if (pollTime - lastPollTime > 100) {
		lastPollTime = pollTime - 100;
	}
	
	// initialize current uspollTimeercmd
	InitCurrent();

	// process the system mouse events
	Mouse();

	// process the system keyboard events
	Keyboard();

	// process the system joystick events
	if (in_useJoystick.GetBool()) //QC
		Joystick();

	// create the usercmd
	MakeCurrent();

	cmd.duplicateCount = 0;

	return cmd;
}
