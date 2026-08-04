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
#include "../../idlib/precompiled.h"
#include "../posix/posix_public.h"
#include "local.h"
#include "framework/KeyInput.h"

idCVar in_rawmouse( "in_rawmouse", "1", CVAR_SYSTEM | CVAR_ARCHIVE, "Use raw mouse input if available" );
idCVar in_grabmouse( "in_grabmouse", "1", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_NOCHEAT, "When set, the mouse is grabbed, so input goes exclusively to this game." );

#if 0
static byte MapKeySym(int key) {
	switch (key) {
		case GLFW_KEY_TAB:
			return K_TAB;
		case GLFW_KEY_ENTER:
			return K_ENTER;
		case GLFW_KEY_ESCAPE:
			return K_ESCAPE;
		case GLFW_KEY_SPACE:
			return K_SPACE;
		case GLFW_KEY_BACKSPACE:
			return K_BACKSPACE;
		case GLFW_KEY_CAPS_LOCK:
			return K_CAPSLOCK;
		case GLFW_KEY_SCROLL_LOCK:
			return K_SCROLL;
		case GLFW_KEY_PAUSE:
			return K_PAUSE;
		case GLFW_KEY_UP:
			return K_UPARROW;
		case GLFW_KEY_DOWN:
			return K_DOWNARROW;
		case GLFW_KEY_LEFT:
			return K_LEFTARROW;
		case GLFW_KEY_RIGHT:
			return K_RIGHTARROW;
		case GLFW_KEY_LEFT_SUPER:
			return K_LWIN;
		case GLFW_KEY_RIGHT_SUPER:
			return K_RWIN;
		case GLFW_KEY_MENU:
			return K_MENU;
		case GLFW_KEY_LEFT_ALT:
			return K_ALT;
		case GLFW_KEY_LEFT_CONTROL: case GLFW_KEY_RIGHT_CONTROL:
			return K_CTRL;
		case GLFW_KEY_LEFT_SHIFT: case GLFW_KEY_RIGHT_SHIFT:
			return K_SHIFT;
		case GLFW_KEY_INSERT:
			return K_INS;
		case GLFW_KEY_DELETE:
			return K_DEL;
		case GLFW_KEY_PAGE_DOWN:
			return K_PGDN;
		case GLFW_KEY_PAGE_UP:
			return K_PGUP;
		case GLFW_KEY_HOME:
			return K_HOME;
		case GLFW_KEY_END:
			return K_END;
		case GLFW_KEY_F1:
			return K_F1;
		case GLFW_KEY_F2:
			return K_F2;
		case GLFW_KEY_F3:
			return K_F3;
		case GLFW_KEY_F4:
			return K_F4;
		case GLFW_KEY_F5:
			return K_F5;
		case GLFW_KEY_F6:
			return K_F6;
		case GLFW_KEY_F7:
			return K_F7;
		case GLFW_KEY_F8:
			return K_F8;
		case GLFW_KEY_F9:
			return K_F9;
		case GLFW_KEY_F10:
			return K_F10;
		case GLFW_KEY_F11:
			return K_F11;
		case GLFW_KEY_F12:
			return K_F12;
		case GLFW_KEY_KP_7:
			return K_KP_HOME;
		case GLFW_KEY_KP_8:
			return K_KP_UPARROW;
		case GLFW_KEY_KP_9:
			return K_KP_PGUP;
		case GLFW_KEY_KP_4:
			return K_KP_LEFTARROW;
		case GLFW_KEY_KP_5:
			return K_KP_5;
		case GLFW_KEY_KP_6:
			return K_KP_RIGHTARROW;
		case GLFW_KEY_KP_1:
			return K_KP_END;
		case GLFW_KEY_KP_2:
			return K_KP_DOWNARROW;
		case GLFW_KEY_KP_3:
			return K_KP_PGDN;
		case GLFW_KEY_KP_ENTER:
			return K_KP_ENTER;
		case GLFW_KEY_KP_0:
			return K_KP_INS;
		case GLFW_KEY_KP_DECIMAL:
			return K_KP_DEL;
		case GLFW_KEY_KP_DIVIDE:
			return K_KP_SLASH;
		case GLFW_KEY_KP_SUBTRACT:
			return K_KP_MINUS;
		case GLFW_KEY_KP_ADD:
			return K_KP_PLUS;
		case GLFW_KEY_KP_MULTIPLY:
			return K_KP_STAR;
		case GLFW_KEY_KP_EQUAL:
			return K_KP_EQUALS;
		case GLFW_KEY_PRINT_SCREEN:
			return K_PRINT_SCR;
		case GLFW_KEY_RIGHT_ALT:
			return K_RIGHT_ALT;
		default:
			// we should be left primarily with printable keys, which can be passed directly as ASCII code to TDM.
			// But beware for letter keys: GLFW uses the uppercase letter ASCII code to represent them, whereas
			// TDH expects the lowercase code!
			if ( key >= 'A' && key <= 'Z' )
				return key - 'A' + 'a';
			if ( key < 256 )
				return key;
			return 0;
	}
}
#endif

/*
=================
IN_Clear_f
=================
*/
void IN_Clear_f( const idCmdArgs &args ) {
	idKeyInput::ClearStates();
}

/*
=================
Sys_InitInput
=================
*/
void Sys_InitInput(void) {
	common->Printf( "\n------- Input Initialization -------\n" );
	common->Printf( "------------------------------------\n" );
}

extern void Android_GrabMouseCursor(bool grab);
void Sys_GrabMouseCursor( bool grabIt ) {
    Android_GrabMouseCursor(grabIt);
}

void Sys_AdjustMouseMovement(float &dx, float &dy) {
	//TODO: apply desktop acceleration settings
	//supposedly, base them on:
	//  mouse_accel_numerator;
	//  mouse_accel_denominator;
	//  mouse_threshold;
	//just make sure they are available...
}

/*
==========================
Posix_PollInput
==========================
*/
extern void Android_PollInput(void);
void Posix_PollInput() {
    Android_PollInput();
}

/*
=================
Sys_ShutdownInput
=================
*/
void Sys_ShutdownInput( void ) { }

/*
===============
Sys_MapCharForKey
===============
*/
unsigned char Sys_MapCharForKey( int _key ) {
	return (unsigned char)_key;
}
