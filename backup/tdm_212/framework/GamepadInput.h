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
#pragma once

typedef enum {
	PAD_A,
	PAD_B,
	PAD_X,
	PAD_Y,
	PAD_L1,
	PAD_R1,
	PAD_L2,
	PAD_R2,
	PAD_L3,
	PAD_R3,
	PAD_BACK,
	PAD_START,
	PAD_LEFT,
	PAD_RIGHT,
	PAD_UP,
	PAD_DOWN,
	PAD_HOME,

	PAD_AXIS_LX,
	PAD_AXIS_LY,
	PAD_AXIS_RX,
	PAD_AXIS_RY,

	PAD_TOTAL_NUM,
	PAD_NUM_BUTTONS = PAD_HOME + 1,
	PAD_MIN_AXIS = PAD_AXIS_LX,
	PAD_MAX_AXIS = PAD_AXIS_RY,
	PAD_NUM_AXES = PAD_MAX_AXIS - PAD_MIN_AXIS + 1,
} padNum_t;

struct padActionChange_t {
	int action;
	idStr binding;
	bool active;
};

class idGamepadInput {
public:
	static void Init();
	static void Shutdown();

	static int StringToButton( const idStr &str );
	static int StringToType( const idStr &str );
	static idStr ButtonToString( int button );
	static idStr TypeToString( int type );

	static void UnbindAll();
	static void SetBinding( int button, int type, const idStr &binding );

	static void SetButtonState( int button, bool pressed );
	static void SetAxisState( int axis, int value );
	static void UpdateAxisState( int gameAxis[MAX_JOYSTICK_AXIS] );

	static idList<padActionChange_t> GetActionStateChange();

	static void WriteBindings( idFile *f );
};
