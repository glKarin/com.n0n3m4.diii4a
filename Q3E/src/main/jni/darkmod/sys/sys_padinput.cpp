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
#include "sys_padinput.h"
#include "../renderer/tr_local.h"
#include "../framework/GamepadInput.h"
#if !defined(__ANDROID__)
#include <GLFW/glfw3.h>
#endif

idCVar in_padMouseSpeed( "in_padMouseSpeed", "2.5", CVAR_GAME|CVAR_FLOAT|CVAR_ARCHIVE, "The speed at which the mouse cursor moves from gamepad input" );
idCVar in_padDeadZone( "in_padDeadZone", "0.15", CVAR_GAME|CVAR_FLOAT|CVAR_ARCHIVE, "Defines the deadzone for the gamepad sticks - any deflection below this value is considered 0" );
idCVar in_padInverseRX( "in_padInverseRX", "0", CVAR_GAME|CVAR_BOOL|CVAR_ARCHIVE, "If enabled, movement of the right stick X axis is inversed" );
idCVar in_padInverseRY( "in_padInverseRY", "0", CVAR_GAME|CVAR_BOOL|CVAR_ARCHIVE, "If enabled, movement of the right stick Y axis is inversed" );

#if !defined(__ANDROID__)
namespace {
	GLFWgamepadstate currentState;
	idList<sysEvent_t> collectedEvents;
	std::map<int, int> buttonMap;
}
#endif

void Sys_InitPadInput() {
#if !defined(__ANDROID__)
	glfwInit();
	memset( &currentState, 0, sizeof(GLFWgamepadstate) );
	buttonMap[GLFW_GAMEPAD_BUTTON_A] = PAD_A;
	buttonMap[GLFW_GAMEPAD_BUTTON_B] = PAD_B;
	buttonMap[GLFW_GAMEPAD_BUTTON_X] = PAD_X;
	buttonMap[GLFW_GAMEPAD_BUTTON_Y] = PAD_Y;
	buttonMap[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER] = PAD_L1;
	buttonMap[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER] = PAD_R1;
	buttonMap[GLFW_GAMEPAD_BUTTON_DPAD_UP] = PAD_UP;
	buttonMap[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT] = PAD_RIGHT;
	buttonMap[GLFW_GAMEPAD_BUTTON_DPAD_DOWN] = PAD_DOWN;
	buttonMap[GLFW_GAMEPAD_BUTTON_DPAD_LEFT] = PAD_LEFT;
	buttonMap[GLFW_GAMEPAD_BUTTON_LEFT_THUMB] = PAD_L3;
	buttonMap[GLFW_GAMEPAD_BUTTON_RIGHT_THUMB] = PAD_R3;
	buttonMap[GLFW_GAMEPAD_BUTTON_BACK] = PAD_BACK;
	buttonMap[GLFW_GAMEPAD_BUTTON_START] = PAD_START;
	buttonMap[GLFW_GAMEPAD_BUTTON_GUIDE] = PAD_HOME;
#endif
}

int Sys_PollGamepadInputEvents() {
#if !defined(__ANDROID__)
	if ( !glfwJoystickIsGamepad( GLFW_JOYSTICK_1 ) ) {
    return 0;
}

	const float deadZone = idMath::Fabs(in_padDeadZone.GetFloat());

	GLFWgamepadstate state;
	if( glfwGetGamepadState( GLFW_JOYSTICK_1, &state ) ) {
		for ( int i = 0; i <= GLFW_GAMEPAD_BUTTON_LAST; ++i ) {
			if ( state.buttons[i] != currentState.buttons[i] ) {
				collectedEvents.AddGrow( sysEvent_t{ SE_PAD_BUTTON, buttonMap[i], state.buttons[i] == GLFW_PRESS, 0, nullptr } );
			}
		}

		// apply deadzone
		for ( int i = 0; i <= GLFW_GAMEPAD_AXIS_LAST; ++i ) {
			if ( state.axes[i] < deadZone && state.axes[i] > -deadZone ) {
				state.axes[i] = 0;
			}
		}
		
		// L2 and R2 are reported as axes, but we want to treat them as buttons
		state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] = state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] < .25f ? 0.f : 1.f;
		state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] < .25f ? 0.f : 1.f;
		if ( state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] != currentState.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] ) {
			collectedEvents.AddGrow( sysEvent_t{ SE_PAD_BUTTON, PAD_L2, state.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER] == 1.f, 0, nullptr } );
		}
		if ( state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] != currentState.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] ) {
			collectedEvents.AddGrow( sysEvent_t{ SE_PAD_BUTTON, PAD_R2, state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] == 1.f, 0, nullptr } );
		}

		int invRX = in_padInverseRX.GetBool() ? -1 : 1;
		int invRY = in_padInverseRY.GetBool() ? -1 : 1;
		collectedEvents.AddGrow( sysEvent_t{ SE_PAD_AXIS, PAD_AXIS_LX, (int)(127.f * state.axes[GLFW_GAMEPAD_AXIS_LEFT_X]), 0, nullptr } );
		collectedEvents.AddGrow( sysEvent_t{ SE_PAD_AXIS, PAD_AXIS_LY, -(int)(127.f * state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y]), 0, nullptr } );
		collectedEvents.AddGrow( sysEvent_t{ SE_PAD_AXIS, PAD_AXIS_RX, invRX * (int)(127.f * state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X]), 0, nullptr } );
		collectedEvents.AddGrow( sysEvent_t{ SE_PAD_AXIS, PAD_AXIS_RY, -invRY * (int)(127.f * state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]), 0, nullptr } );
	}

	currentState = state;

	bool doMouseEvents = !uiManager->IsBindHandlerActive();
	
	for ( sysEvent_t &ev : collectedEvents ) {
		Sys_QueEvent( 0, ev.evType, ev.evValue, ev.evValue2, ev.evPtrLength, ev.evPtr );
		// hack: send specific keys also as mouse input to make GUI usable
		if ( doMouseEvents ) {
			if ( ev.evType == SE_PAD_BUTTON && ( ev.evValue == PAD_A || ev.evValue == PAD_R1 || ev.evValue == PAD_R2 ) ) {
				Sys_QueEvent( 0, SE_KEY, K_MOUSE1, ev.evValue2, 0, nullptr );
			}
			if ( ev.evType == SE_PAD_BUTTON && ( ev.evValue == PAD_B || ev.evValue == PAD_L1 || ev.evValue == PAD_L2 ) ) {
				Sys_QueEvent( 0, SE_KEY, K_MOUSE2, ev.evValue2, 0, nullptr );
			}
		}
	}

	if ( doMouseEvents ) {
		// hack: send stick movement as mouse delta to move the cursor in GUIs with the gamepad
		int speed = glConfig.vidWidth * in_padMouseSpeed.GetFloat() / 640.f;
		int xDelta = speed * ( state.axes[GLFW_GAMEPAD_AXIS_LEFT_X] + state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X] );
		int yDelta = speed * ( state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] + state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y] );
		if ( xDelta != 0 || yDelta != 0 ) {
			Sys_QueEvent( 0, SE_MOUSE, xDelta, yDelta, 0, nullptr );
		}

		// send regular mouse wheel messages from up/down gamepad events
		static uint64_t lastWheelUpdate = 0;
		if ( Sys_GetTimeMicroseconds() - lastWheelUpdate >= 100000 ) {
			lastWheelUpdate = Sys_GetTimeMicroseconds();
			if ( state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP] == GLFW_PRESS ) {
				Sys_QueEvent( 0, SE_KEY, K_MWHEELUP, 1, 0, nullptr );
				Sys_QueEvent( 0, SE_KEY, K_MWHEELUP, 0, 0, nullptr );
			}
			if ( state.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN] == GLFW_PRESS ) {
				Sys_QueEvent( 0, SE_KEY, K_MWHEELDOWN, 1, 0, nullptr );
				Sys_QueEvent( 0, SE_KEY, K_MWHEELDOWN, 0, 0, nullptr );
			}
		}
	}

	return collectedEvents.Num();
#else
	return 0;
#endif
}

int Sys_ReturnGamepadInputEvent( int n, int &type, int &id, int &value ) {
#if !defined(__ANDROID__)
	if ( n < 0 || n >= collectedEvents.Num() ) {
    return 0;
}

	sysEvent_t &ev = collectedEvents[n];
	type = ev.evType;
	id = ev.evValue;
	value = ev.evValue2;
	return 1;
#else
	return 0;
#endif
}

void Sys_GetCombinedAxisDeflection( int &x, int &y ) {
#if !defined(__ANDROID__)
	x = 127 * (currentState.axes[GLFW_GAMEPAD_AXIS_LEFT_X] + currentState.axes[GLFW_GAMEPAD_AXIS_RIGHT_X]);
	y = -127 * (currentState.axes[GLFW_GAMEPAD_AXIS_LEFT_Y] + currentState.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y]);
#else
	x = 0;
    y = 0;
#endif
}

void Sys_EndGamepadInputEvents() {
#if !defined(__ANDROID__)
	collectedEvents.SetNum( 0 );
#endif
}
