/*****************************************************************************
The Dark Mod GPL Source Code

This file is part of the The Dark Mod Source Code, originally based
on the Doom 3 GPL Source Code as published in 2011.

The Dark Mod Source Code is free software: you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation, either version 3 of the License,
or (at your option) any later version. For details, see LICENSE.TXT.

Project: The Dark Mod (http://www.thedarkmod.com/)

******************************************************************************/

#include "precompiled.h"
#pragma hdrstop


#include "GamepadInput.h"
#include "Session_local.h"
#include "../sys/sys_padinput.h"

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
			my == rhs.my &&
			jx == rhs.jx &&
			jy == rhs.jy );
}


const int KEY_MOVESPEED	= 127;

/*typedef enum {
	UB_NONE,

	UB_UP,
	UB_DOWN,
	UB_LEFT,
	UB_RIGHT,
	UB_FORWARD,
	UB_BACK,
	UB_LOOKUP,
	UB_LOOKDOWN,
	UB_STRAFE,
	UB_MOVELEFT,
	UB_MOVERIGHT,

	UB_BUTTON0,
	UB_BUTTON1,
	UB_BUTTON2,
	UB_BUTTON3,
	UB_BUTTON4,
	UB_BUTTON5,
	UB_BUTTON6,
	UB_BUTTON7,

	UB_ATTACK,
	UB_SPEED,
	UB_ZOOM,
	UB_SHOWSCORES,
	UB_MLOOK,

	UB_IMPULSE0,
	UB_IMPULSE1,
	UB_IMPULSE2,
	UB_IMPULSE3,
	UB_IMPULSE4,
	UB_IMPULSE5,
	UB_IMPULSE6,
	UB_IMPULSE7,
	UB_IMPULSE8,
	UB_IMPULSE9,
	UB_IMPULSE10,
	UB_IMPULSE11,
	UB_IMPULSE12,
	UB_IMPULSE13,
	UB_IMPULSE14,
	UB_IMPULSE15,
	UB_IMPULSE16,
	UB_IMPULSE17,
	UB_IMPULSE18,
	UB_IMPULSE19,
	UB_IMPULSE20,
	UB_IMPULSE21,
	UB_IMPULSE22,
	UB_IMPULSE23,
	UB_IMPULSE24,
	UB_IMPULSE25,
	UB_IMPULSE26,
	UB_IMPULSE27,
	UB_IMPULSE28,
	UB_IMPULSE29,
	UB_IMPULSE30,
	UB_IMPULSE31,
	UB_IMPULSE32,
	UB_IMPULSE33,
	UB_IMPULSE34,
	UB_IMPULSE35,
	UB_IMPULSE36,
	UB_IMPULSE37,
	UB_IMPULSE38,
	UB_IMPULSE39,
	UB_IMPULSE40,
	UB_IMPULSE41,
	UB_IMPULSE42,
	UB_IMPULSE43,
	UB_IMPULSE44,
	UB_IMPULSE45,
	UB_IMPULSE46,
	UB_IMPULSE47,
	UB_IMPULSE48,
	UB_IMPULSE49,
	UB_IMPULSE50,
	UB_IMPULSE51,
	UB_IMPULSE52,
	UB_IMPULSE53,
	UB_IMPULSE54,
	UB_IMPULSE55,
	UB_IMPULSE56,
	UB_IMPULSE57,
	UB_IMPULSE58,
	UB_IMPULSE59,
	UB_IMPULSE60,
	UB_IMPULSE61,
	UB_IMPULSE62,
	UB_IMPULSE63,

	UB_MAX_BUTTONS
} usercmdButton_t;*/

typedef struct {
	const char *string;
	UserCmdButton	button;
} userCmdString_t;

userCmdString_t	userCmdStrings[] = {
	// self-explanatory names
	{ "_jump",			UB_UP },
	{ "_parry",			UB_PARRY_MANIPULATE },

	{ "_weapon0",		UB_WEAPON0 },
	{ "_weapon1",		UB_WEAPON1 },
	{ "_weapon2",		UB_WEAPON2 },
	{ "_weapon3",		UB_WEAPON3 },
	{ "_weapon4",		UB_WEAPON4 },
	{ "_weapon5",		UB_WEAPON5 },
	{ "_weapon6",		UB_WEAPON6 },
	{ "_weapon7",		UB_WEAPON7 },
	{ "_weapon8",		UB_WEAPON8 },
	{ "_weapon9",		UB_WEAPON9 },
	{ "_weapon10",		UB_WEAPON10 },
	{ "_weapon11",		UB_WEAPON11 },
	{ "_weapon12",		UB_WEAPON12 },
	{ "_reload",			UB_RELOAD },
	{ "_weapon_next",	UB_WEAPON_NEXT },
	{ "_weapon_prev",	UB_WEAPON_PREV },
	{ "_ready",			UB_READY },
	{ "_center_view",	UB_CENTER_VIEW },
	{ "_objectives",		UB_OBJECTIVES },
	{ "_crouch",			UB_CROUCH },
	{ "_mantle",			UB_MANTLE },
	{ "_inventory_grid",	UB_INVENTORY_GRID },
	{ "_frob",			UB_FROB },
	{ "_lean_forward",	UB_LEAN_FORWARD },
	{ "_lean_left",		UB_LEAN_LEFT },
	{ "_lean_right",		UB_LEAN_RIGHT },
	{ "_inventory_prev",	UB_INVENTORY_PREV },
	{ "_inventory_next",	UB_INVENTORY_NEXT },
	{ "_inventory_group_prev", UB_INVENTORY_GROUP_PREV },
	{ "_inventory_group_next", UB_INVENTORY_GROUP_NEXT },
	{ "_inventory_use",	UB_INVENTORY_USE },
	{ "_inventory_drop",	UB_INVENTORY_DROP },
	{ "_creep",	UB_CREEP },

	// legacy names
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
	{ "_zoom",			UB_PARRY_MANIPULATE },
	{ "_showScores",	UB_SHOWSCORES },
	{ "_mlook",			UB_MLOOK },

	{ "_button0",		UB_BUTTON0 },
	{ "_button1",		UB_BUTTON1 },
	{ "_button2",		UB_BUTTON2 },
	{ "_button3",		UB_BUTTON3 },
	{ "_button4",		UB_BUTTON4 },
	{ "_button5",		UB_BUTTON5 },
	{ "_button6",		UB_BUTTON6 },
	{ "_button7",		UB_BUTTON7 },

	{ "_impulse0",		UB_WEAPON0 },
	{ "_impulse1",		UB_WEAPON1 },
	{ "_impulse2",		UB_WEAPON2 },
	{ "_impulse3",		UB_WEAPON3 },
	{ "_impulse4",		UB_WEAPON4 },
	{ "_impulse5",		UB_WEAPON5 },
	{ "_impulse6",		UB_WEAPON6 },
	{ "_impulse7",		UB_WEAPON7 },
	{ "_impulse8",		UB_WEAPON8 },
	{ "_impulse9",		UB_WEAPON9 },
	{ "_impulse10",		UB_WEAPON10 },
	{ "_impulse11",		UB_WEAPON11 },
	{ "_impulse12",		UB_WEAPON12 },
	{ "_impulse13",		UB_RELOAD },
	{ "_impulse14",		UB_WEAPON_NEXT },
	{ "_impulse15",		UB_WEAPON_PREV },
	{ "_impulse16",		UB_IMPULSE16 },
	{ "_impulse17",		UB_READY },
	{ "_impulse18",		UB_CENTER_VIEW },
	{ "_impulse19",		UB_OBJECTIVES },
	{ "_impulse20",		UB_IMPULSE20 },
	{ "_impulse21",		UB_IMPULSE21 },
	{ "_impulse22",		UB_IMPULSE22 },
	{ "_impulse23",		UB_CROUCH },
	{ "_impulse24",		UB_MANTLE },
	{ "_impulse25",		UB_IMPULSE25 },
	{ "_impulse26",		UB_IMPULSE26 },
	{ "_impulse27",		UB_IMPULSE27 },
	{ "_impulse28",		UB_IMPULSE28 },
	{ "_impulse29",		UB_IMPULSE29 },
	{ "_impulse30",		UB_INVENTORY_GRID },
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
	{ "_impulse41",		UB_FROB },
	{ "_impulse42",		UB_IMPULSE42 },
	{ "_impulse43",		UB_IMPULSE43 },
	{ "_impulse44",		UB_LEAN_FORWARD },
	{ "_impulse45",		UB_LEAN_LEFT },
	{ "_impulse46",		UB_LEAN_RIGHT },
	{ "_impulse47",		UB_INVENTORY_PREV },
	{ "_impulse48",		UB_INVENTORY_NEXT },
	{ "_impulse49",		UB_INVENTORY_GROUP_PREV },
	{ "_impulse50",		UB_INVENTORY_GROUP_NEXT },
	{ "_impulse51",		UB_INVENTORY_USE },
	{ "_impulse52",		UB_INVENTORY_DROP },
	{ "_impulse53",		UB_CREEP },
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
	
	virtual void	Init( void ) override;

	virtual void	InitForNewMap( void ) override;

	virtual void	Shutdown( void ) override;

	virtual void	Clear( void ) override;

	virtual void	ClearAngles( void ) override;

	virtual usercmd_t	TicCmd( int ticNumber ) override;

	virtual void	InhibitUsercmd( inhibit_t subsystem, bool inhibit ) override;

	virtual void	UsercmdInterrupt( void ) override;

	virtual int		CommandStringUsercmdData( const char *cmdString ) override;

	virtual int		GetNumUserCommands( void ) override;

	virtual const char *GetUserCommandName( int index ) override;

	virtual void	MouseState( int *x, int *y, int *button, bool *down ) override;

	virtual int		ButtonState( int key ) override;
	virtual int		KeyState( int key ) override;

	virtual int 	GetToggledRunState( void ) override;
	virtual void	SetToggledRunState( int on ) override;

	virtual usercmd_t	GetDirectUsercmd( void ) override;
	virtual int&	hack_Flags() override;

private:
	void			MakeCurrent( void );
	void			InitCurrent( void );

	bool			Inhibited( void );
	void			AdjustAngles( void );
	void			KeyMove( void );
	void			JoystickMove( void );
	void			MouseMove( void );
	void			CmdButtons( void );

	void			Mouse( void );
	void			Keyboard( void );
	void			Joystick( void );

	void			Key( int keyNum, bool down );

	idVec3			viewangles;
	int				flags;
	int				impulse;

	buttonState_t	toggled_crouch;
	buttonState_t	toggled_run;
	buttonState_t	toggled_zoom;

	int				buttonState[UB_MAX_BUTTONS];
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
	int				joystickAxis[MAX_JOYSTICK_AXIS];	// set by joystick events

	static idCVar	in_yawSpeed;
	static idCVar	in_pitchSpeed;
	static idCVar	in_angleSpeedKey;
	static idCVar	in_freeLook;
	static idCVar	in_alwaysRun;
	static idCVar	in_toggleRun;
	static idCVar	in_toggleCrouch;
	static idCVar	in_toggleZoom;
	static idCVar	sensitivity;
	static idCVar	sensitivityMenu;
	static idCVar	sensitivityMenuOverride;
	static idCVar	m_pitch;
	static idCVar	m_yaw;
	static idCVar	m_strafeScale;
	static idCVar	m_smooth;
	static idCVar	m_strafeSmooth;
	static idCVar	m_showMouseRate;
};

idCVar idUsercmdGenLocal::in_yawSpeed( "in_yawspeed", "140", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_FLOAT, "yaw change speed when holding down _left or _right button" );
idCVar idUsercmdGenLocal::in_pitchSpeed( "in_pitchspeed", "140", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_FLOAT, "pitch change speed when holding down look _lookUp or _lookDown button" );
idCVar idUsercmdGenLocal::in_angleSpeedKey( "in_anglespeedkey", "1.5", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_FLOAT, "angle change scale when holding down _speed button" );
idCVar idUsercmdGenLocal::in_freeLook( "in_freeLook", "1", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_BOOL, "look around with mouse (reverse _mlook button)" );
idCVar idUsercmdGenLocal::in_alwaysRun( "in_alwaysRun", "0", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_BOOL, "always run (reverse _speed button) - only in MP" );
idCVar idUsercmdGenLocal::in_toggleRun( "in_toggleRun", "0", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_BOOL, "pressing _speed button toggles run on/off - only in MP" );
idCVar idUsercmdGenLocal::in_toggleCrouch( "in_toggleCrouch", "0", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_BOOL, "pressing _movedown button toggles player crouching/standing" );
idCVar idUsercmdGenLocal::in_toggleZoom( "in_toggleZoom", "0", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_BOOL, "pressing _zoom button toggles zoom on/off" );
idCVar idUsercmdGenLocal::sensitivity( "sensitivity", "5", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_FLOAT, "Mouse view sensitivity (during gameplay)" );
idCVar idUsercmdGenLocal::sensitivityMenu("sensitivityMenu", "1.0", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_FLOAT, "In menu GUIs: defines mouse sensitivity. ");
idCVar idUsercmdGenLocal::sensitivityMenuOverride("sensitivityMenuOverride", "0", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_BOOL, "In meni GUIs: ignore OS-specific mouse movement adjustments. ");
idCVar idUsercmdGenLocal::m_pitch( "m_pitch", "0.022", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_FLOAT, "mouse pitch scale" );
idCVar idUsercmdGenLocal::m_yaw( "m_yaw", "0.022", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_FLOAT, "mouse yaw scale" );
idCVar idUsercmdGenLocal::m_strafeScale( "m_strafeScale", "6.25", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_FLOAT, "mouse strafe movement scale" );
idCVar idUsercmdGenLocal::m_smooth( "m_smooth", "1", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_INTEGER, "number of samples blended for mouse viewing", 1, 8, idCmdSystem::ArgCompletion_Integer<1,8> );
idCVar idUsercmdGenLocal::m_strafeSmooth( "m_strafeSmooth", "4", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_INTEGER, "number of samples blended for mouse moving", 1, 8, idCmdSystem::ArgCompletion_Integer<1,8> );
idCVar idUsercmdGenLocal::m_showMouseRate( "m_showMouseRate", "0", CVAR_SYSTEM | CVAR_BOOL, "shows mouse movement" );

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

	toggled_crouch.Clear();
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

/*
================
idUsercmdGenLocal::GetToggledRunState
================
*/
int idUsercmdGenLocal::GetToggledRunState( void ) {
	return toggled_run.on;
}

/*
================
idUsercmdGenLocal::SetToggledRunState
================
*/
void idUsercmdGenLocal::SetToggledRunState( int on ) {
	toggled_run.on = idMath::ClampInt(0, 1, on);
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
idUsercmdGenLocal::GetUserCommandName
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
	
	if ( toggled_run.on ) {
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

	up -= KEY_MOVESPEED * toggled_crouch.on;
	up += KEY_MOVESPEED * ButtonState( UB_UP );

	forward += KEY_MOVESPEED * ButtonState( UB_FORWARD );
	forward -= KEY_MOVESPEED * ButtonState( UB_BACK );

#ifdef __ANDROID__ //karin: joystick smooth movement on Android
    //ANALOG
    Sys_Analog(side, forward, KEY_MOVESPEED);
    //END
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

	if ( idMath::Fabs( mx ) > 1000 || idMath::Fabs( my ) > 1000 ) {
		Sys_DebugPrintf( "idUsercmdGenLocal::MouseMove: Ignoring ridiculous mouse delta.\n" );
		mx = my = 0;
	}

	mx *= sensitivity.GetFloat();
	my *= sensitivity.GetFloat();

	if ( m_showMouseRate.GetBool() ) {
		Sys_DebugPrintf( "[%3i %3i  = %5.1f %5.1f = %5.1f %5.1f] ", mouseDx, mouseDy, mx, my, strafeMx, strafeMy );
	}

	mouseDx = 0;
	mouseDy = 0;

	if ( !strafeMx && !strafeMy ) {
		return;
	}

	if ( ButtonState( UB_STRAFE ) || !( cmd.buttons & BUTTON_MLOOK ) ) {
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

	if ( !ButtonState( UB_STRAFE ) && ( cmd.buttons & BUTTON_MLOOK ) ) {
		viewangles[PITCH] += m_pitch.GetFloat() * my;
	} else {
		cmd.forwardmove = idMath::ClampChar( (int)(cmd.forwardmove - strafeMy) );
	}
}

/*
=================
idUsercmdGenLocal::JoystickMove
=================
*/
idCVar in_padInvertYawAxis( "in_padInvertYawAxis", "0", CVAR_GAME|CVAR_BOOL|CVAR_ARCHIVE, "Invert gamepad yaw axis" );
idCVar in_padInvertPitchAxis( "in_padInvertPitchAxis", "0", CVAR_GAME|CVAR_BOOL|CVAR_ARCHIVE, "Invert gamepad pitch axis" );
void idUsercmdGenLocal::JoystickMove( void ) {
	float	anglespeed;

	if ( toggled_run.on ) {
		anglespeed = idMath::M_MS2SEC * USERCMD_MSEC * in_angleSpeedKey.GetFloat();
	} else {
		anglespeed = idMath::M_MS2SEC * USERCMD_MSEC;
	}

	float yawAngleSpeed = anglespeed * ( in_padInvertYawAxis.GetBool() ? 1 : -1 );
	float pitchAngleSpeed = anglespeed * ( in_padInvertPitchAxis.GetBool() ? 1 : -1 );
	viewangles[YAW] += yawAngleSpeed * in_yawSpeed.GetFloat() * joystickAxis[AXIS_YAW] / 127.f;
	viewangles[PITCH] += pitchAngleSpeed * in_pitchSpeed.GetFloat() * joystickAxis[AXIS_PITCH] / 127.f;
	cmd.rightmove = idMath::ClampChar( cmd.rightmove + joystickAxis[AXIS_SIDE] );
	cmd.forwardmove = idMath::ClampChar( cmd.forwardmove + joystickAxis[AXIS_FORWARD] );
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
		if ( ButtonState( (UserCmdButton)( UB_BUTTON0 + i ) ) ) {
			cmd.buttons |= 1 << i;
		}
	}

	// check the attack button
	if ( ButtonState( UB_ATTACK ) ) {
		cmd.buttons |= BUTTON_ATTACK;
	}

	// check the run button
	if ( toggled_run.on ) {
		cmd.buttons |= BUTTON_RUN;
	}

	// check the zoom button
	if ( toggled_zoom.on ) {
		cmd.buttons |= BUTTON_ZOOM;
	}

	// check the scoreboard button
	if ( ButtonState( UB_SHOWSCORES ) || ButtonState( UB_OBJECTIVES ) ) {
		// the button is toggled in SP mode as well but without effect
		cmd.buttons |= BUTTON_SCORES;
	}

	// check the mouse look button
	if ( ButtonState( UB_MLOOK ) ^ in_freeLook.GetInteger() ) {
		cmd.buttons |= BUTTON_MLOOK;
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
	cmd.buttons |= in_alwaysRun.GetBool() ? BUTTON_RUN : 0;
	cmd.buttons |= in_freeLook.GetBool() ? BUTTON_MLOOK : 0;
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
		toggled_crouch.SetKeyState( ButtonState( UB_DOWN ), in_toggleCrouch.GetBool() );
		toggled_run.SetKeyState( ButtonState( UB_SPEED ), in_toggleRun.GetBool() );
		toggled_zoom.SetKeyState( ButtonState( UB_PARRY_MANIPULATE ), in_toggleZoom.GetBool() );

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
	}

	for ( i = 0; i < 3; i++ ) {
		cmd.angles[i] = ANGLE2SHORT( viewangles[i] );
	}

	cmd.mx = continuousMouseX;
	cmd.my = continuousMouseY;

	cmd.jx = joystickAxis[AXIS_SIDE] + joystickAxis[AXIS_YAW];
	cmd.jy = joystickAxis[AXIS_FORWARD] + joystickAxis[AXIS_PITCH];

	flags = cmd.flags;
	impulse = cmd.impulse;

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

	toggled_crouch.Clear();
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
	memset( keyState, false, sizeof( keyState ) );

	inhibitCommands = false;

	mouseDx = mouseDy = 0;
	mouseButton = 0;
	mouseDown = false;
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
			if ( action >= UB_WEAPON0 && action <= UB_IMPULSE61 ) {
				cmd.impulse = action - UB_WEAPON0;
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
	memset( joystickAxis, 0, sizeof( joystickAxis ) );

	int numEvents = Sys_PollGamepadInputEvents();
	if ( numEvents ) {
		int type;
		int id;
		int value;

		for ( int i = 0; i < numEvents; ++i ) {
			if ( Sys_ReturnGamepadInputEvent( i, type, id, value ) ) {
				if ( type == SE_PAD_BUTTON ) {
					idGamepadInput::SetButtonState( id, value );
				} else if ( type == SE_PAD_AXIS ) {
					idGamepadInput::SetAxisState( id, value );					
				}
			}
		}
	}

	idGamepadInput::UpdateAxisState( joystickAxis );
	idList<padActionChange_t> stateChanges = idGamepadInput::GetActionStateChange();
	for ( auto change : stateChanges ) {
		if ( change.active ) {

			buttonState[ change.action ]++;

			if ( !Inhibited()  ) {
				if ( change.action >= UB_WEAPON0 && change.action <= UB_IMPULSE61 ) {
					cmd.impulse = change.action - UB_WEAPON0;
					cmd.flags ^= UCF_IMPULSE_SEQUENCE;
				}
			}

			if ( change.action == UB_NONE && change.binding.Length() > 0 ) {
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, change.binding.c_str() );
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "\n" );
			}
		} else {
			buttonState[ change.action ]--;
			// we might have one held down across an app active transition
			if ( buttonState[ change.action ] < 0 ) {
				buttonState[ change.action ] = 0;
			}
		}
	}
	
	Sys_EndGamepadInputEvents();
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

	// initialize current usercmd
	InitCurrent();

	// process the system mouse events
	Mouse();

	// process the system keyboard events
	Keyboard();

	// process the system joystick events
	Joystick();

	// create the usercmd
	MakeCurrent();

	cmd.duplicateCount = 0;

	return cmd;
}

int& idUsercmdGenLocal::hack_Flags() {
	return flags;
}
