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
along with Doom 3 Source Code.	If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifdef USE_SDL

#include "../idlib/precompiled.h"

#include "../idlib/containers/List.h"
#include "../idlib/Heap.h"
#include "../framework/Common.h"
#include "../framework/KeyInput.h"
#include "../renderer/RenderSystem.h"
#include "../renderer/tr_local.h"

#include "../sys/sys_public.h"

#ifdef __WINDOWS__
#include "win32/win_local.h"

#include <SDL_syswm.h>
#endif

#ifdef __linux__
extern char *Posix_ConsoleInput( void );
#define Sys_ConsoleInput Posix_ConsoleInput
#endif

extern bool g_inputGrabbed;

const char *kbdNames[] = {
	"english", "french", "german", "italian", "spanish", "turkish", "norwegian", NULL
};

idCVar in_kbd("in_kbd", "english", CVAR_SYSTEM | CVAR_ARCHIVE | CVAR_NOCHEAT, "keyboard layout", kbdNames, idCmdSystem::ArgCompletion_String<kbdNames> );

struct kbd_poll_t {
	int key;
	bool state;

	kbd_poll_t() {
	}

	kbd_poll_t(int k, bool s) {
		key = k;
		state = s;
	}
};

struct mouse_poll_t {
	int action;
	int value;

	mouse_poll_t() {
	}

	mouse_poll_t(int a, int v) {
		action = a;
		value = v;
	}
};

struct joystick_poll_t {
	int action;
	int value;

	joystick_poll_t() {
	}

	joystick_poll_t(int a, int v) {
		action = a;
		value = v;
	}
};

static idList<kbd_poll_t> kbd_polls;
static idList<mouse_poll_t> mouse_polls;
static idList<joystick_poll_t> joystick_polls;

static bool buttonStates[K_LAST_KEY];
static int	joyAxis[MAX_JOYSTICK_AXIS];

static idList<sysEvent_t> event_overflow;

static byte mapkey(SDL_Keycode key) {
	switch (key) {
	case SDLK_BACKSPACE:
		return K_BACKSPACE;
	case SDLK_PAUSE:
		return K_PAUSE;
	}

	if (key <= SDLK_z)
		return key & 0xff;

	switch (key) {
	case SDLK_APPLICATION:
		return K_COMMAND;
	case SDLK_CAPSLOCK:
		return K_CAPSLOCK;
	case SDLK_SCROLLLOCK:
		return K_SCROLL;
	case SDLK_POWER:
		return K_POWER;

	case SDLK_UP:
		return K_UPARROW;
	case SDLK_DOWN:
		return K_DOWNARROW;
	case SDLK_LEFT:
		return K_LEFTARROW;
	case SDLK_RIGHT:
		return K_RIGHTARROW;

	case SDLK_LGUI:
		return K_LWIN;
	case SDLK_RGUI:
		return K_RWIN;
	case SDLK_MENU:
		return K_MENU;

	case SDLK_LALT:
	case SDLK_RALT:
		return K_ALT;
	case SDLK_RCTRL:
	case SDLK_LCTRL:
		return K_CTRL;
	case SDLK_RSHIFT:
	case SDLK_LSHIFT:
		return K_SHIFT;
	case SDLK_INSERT:
		return K_INS;
	case SDLK_DELETE:
		return K_DEL;
	case SDLK_PAGEDOWN:
		return K_PGDN;
	case SDLK_PAGEUP:
		return K_PGUP;
	case SDLK_HOME:
		return K_HOME;
	case SDLK_END:
		return K_END;

	case SDLK_F1:
		return K_F1;
	case SDLK_F2:
		return K_F2;
	case SDLK_F3:
		return K_F3;
	case SDLK_F4:
		return K_F4;
	case SDLK_F5:
		return K_F5;
	case SDLK_F6:
		return K_F6;
	case SDLK_F7:
		return K_F7;
	case SDLK_F8:
		return K_F8;
	case SDLK_F9:
		return K_F9;
	case SDLK_F10:
		return K_F10;
	case SDLK_F11:
		return K_F11;
	case SDLK_F12:
		return K_F12;
	// K_INVERTED_EXCLAMATION;
	case SDLK_F13:
		return K_F13;
	case SDLK_F14:
		return K_F14;
	case SDLK_F15:
		return K_F15;

	case SDLK_KP_7:
		return K_KP_HOME;
	case SDLK_KP_8:
		return K_KP_UPARROW;
	case SDLK_KP_9:
		return K_KP_PGUP;
	case SDLK_KP_4:
		return K_KP_LEFTARROW;
	case SDLK_KP_5:
		return K_KP_5;
	case SDLK_KP_6:
		return K_KP_RIGHTARROW;
	case SDLK_KP_1:
		return K_KP_END;
	case SDLK_KP_2:
		return K_KP_DOWNARROW;
	case SDLK_KP_3:
		return K_KP_PGDN;
	case SDLK_KP_ENTER:
		return K_KP_ENTER;
	case SDLK_KP_0:
		return K_KP_INS;
	case SDLK_KP_PERIOD:
		return K_KP_DEL;
	case SDLK_KP_DIVIDE:
		return K_KP_SLASH;
	// K_SUPERSCRIPT_TWO;
	case SDLK_KP_MINUS:
		return K_KP_MINUS;
	// K_ACUTE_ACCENT;
	case SDLK_KP_PLUS:
		return K_KP_PLUS;
	case SDLK_NUMLOCKCLEAR:
		return K_KP_NUMLOCK;
	case SDLK_KP_MULTIPLY:
		return K_KP_STAR;
	case SDLK_KP_EQUALS:
		return K_KP_EQUALS;

	// K_MASCULINE_ORDINATOR;
	// K_GRAVE_A;
	// K_AUX1;
	// K_CEDILLA_C;
	// K_GRAVE_E;
	// K_AUX2;
	// K_AUX3;
	// K_AUX4;
	// K_GRAVE_I;
	// K_AUX5;
	// K_AUX6;
	// K_AUX7;
	// K_AUX8;
	// K_TILDE_N;
	// K_GRAVE_O;
	// K_AUX9;
	// K_AUX10;
	// K_AUX11;
	// K_AUX12;
	// K_AUX13;
	// K_AUX14;
	// K_GRAVE_U;
	// K_AUX15;
	// K_AUX16;

	case SDLK_PRINTSCREEN:
		return K_PRINT_SCR;
	case SDLK_MODE:
		return K_RIGHT_ALT;
	}

	return 0;
}

static sys_jEvents mapjoybutton(SDL_GameControllerButton button) {

	switch (button)
	{
	case SDL_CONTROLLER_BUTTON_A:
		return J_ACTION1;
	case SDL_CONTROLLER_BUTTON_B:
		return J_ACTION2;
	case SDL_CONTROLLER_BUTTON_X:
		return J_ACTION3;
	case SDL_CONTROLLER_BUTTON_Y:
		return J_ACTION4;
	case SDL_CONTROLLER_BUTTON_BACK:
		return J_ACTION10;
	case SDL_CONTROLLER_BUTTON_GUIDE:
		// TODO:
		break;
	case SDL_CONTROLLER_BUTTON_START:
		return J_ACTION9;
	case SDL_CONTROLLER_BUTTON_LEFTSTICK:
		return J_ACTION7;
	case SDL_CONTROLLER_BUTTON_RIGHTSTICK:
		return J_ACTION8;
	case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
		return J_ACTION5;
	case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
		return J_ACTION6;
	case SDL_CONTROLLER_BUTTON_DPAD_UP:
		return J_DPAD_UP;
	case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		return J_DPAD_DOWN;
	case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
		return J_DPAD_LEFT;
	case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
		return J_DPAD_RIGHT;
	default:
		common->Warning("unknown game controller button %u", button);
		break;
	}

	return MAX_JOY_EVENT;
}

static sys_jEvents mapjoyaxis(SDL_GameControllerAxis axis) {

	switch (axis)
	{
	case SDL_CONTROLLER_AXIS_LEFTX:
		return J_AXIS_LEFT_X;
    case SDL_CONTROLLER_AXIS_LEFTY:
    	return J_AXIS_LEFT_Y;
    case SDL_CONTROLLER_AXIS_RIGHTX:
    	return J_AXIS_RIGHT_X;
    case SDL_CONTROLLER_AXIS_RIGHTY:
    	return J_AXIS_RIGHT_Y;
    case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
    	return J_AXIS_LEFT_TRIG;
    case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
    	return J_AXIS_RIGHT_TRIG;
	default:
		common->Warning("unknown game controller axis %u", axis);
		break;
	}

	return J_AXIS_MAX;
}

static void PushConsoleEvent(const char *s) {
	char *b;
	size_t len;

	len = strlen(s) + 1;
	b = (char *)Mem_Alloc(len);
	strcpy(b, s);

	SDL_Event event;

	event.type = SDL_USEREVENT;
	event.user.code = SE_CONSOLE;
	event.user.data1 = (void *)len;
	event.user.data2 = b;

	SDL_PushEvent(&event);
}

/*
=================
Sys_InitInput
=================
*/
void Sys_InitInput() {
	kbd_polls.SetGranularity(64);
	mouse_polls.SetGranularity(64);
	joystick_polls.SetGranularity(64);

	event_overflow.SetGranularity(64);

	memset( buttonStates, 0, sizeof( buttonStates ) );
	memset( joyAxis, 0, sizeof( joyAxis ) );

	in_kbd.SetModified();

	const int NumJoysticks = SDL_NumJoysticks();
	for( int i = 0; i < NumJoysticks; ++i )
	{
		SDL_GameControllerOpen( i );
	}
}

/*
=================
Sys_ShutdownInput
=================
*/
void Sys_ShutdownInput() {
	kbd_polls.Clear();
	mouse_polls.Clear();
	joystick_polls.Clear();
	event_overflow.Clear();
}

/*
===========
Sys_InitScanTable
===========
*/
// Windows has its own version due to the tools
#ifndef _WIN32
void Sys_InitScanTable() {
}
#endif

/*
===============
Sys_GetConsoleKey
===============
*/
unsigned char Sys_GetConsoleKey(bool shifted) {
	static unsigned char keys[2] = { '`', '~' };

	if (in_kbd.IsModified()) {
		idStr lang = in_kbd.GetString();

		if (lang.Length()) {
			if (!lang.Icmp("french")) {
				keys[0] = '<';
				keys[1] = '>';
			} else if (!lang.Icmp("german")) {
				keys[0] = '^';
				keys[1] = 176; // °
			} else if (!lang.Icmp("italian")) {
				keys[0] = '\\';
				keys[1] = '|';
			} else if (!lang.Icmp("spanish")) {
				keys[0] = 186; // º
				keys[1] = 170; // ª
			} else if (!lang.Icmp("turkish")) {
				keys[0] = '"';
				keys[1] = 233; // é
			} else if (!lang.Icmp("norwegian")) {
				keys[0] = 124; // |
				keys[1] = 167; // §
			}
		}

		in_kbd.ClearModified();
	}

	return shifted ? keys[1] : keys[0];
}

/*
===============
Sys_MapCharForKey
===============
*/
unsigned char Sys_MapCharForKey(int key) {
	return key & 0xff;
}

/*
===============
Sys_GrabMouseCursor
===============
*/
void Sys_GrabMouseCursor(bool grabIt) {
	int flags;

	if (grabIt)
		flags = GRAB_ENABLE | GRAB_HIDECURSOR;
	else
		flags = 0;

	GLimp_GrabInput(flags);
}


void PushButton( int key, bool value ) {
	// So we don't keep sending the same SE_KEY message over and over again
	if ( buttonStates[key] != value ) {
		buttonStates[key] = value;
		sysEvent_t res = { SE_KEY, key, value ? 1 : 0, 0, NULL };
		event_overflow.Append(res);
	}
}

/*
================
Sys_GetEvent
================
*/
sysEvent_t Sys_GetEvent() {
	SDL_Event ev;
	sysEvent_t res = { };
	byte key;

	static const sysEvent_t res_none = { SE_NONE, 0, 0, 0, NULL };

	// process any overflow.
	if (event_overflow.Num() > 0)
	{
		res = event_overflow[0];
		event_overflow.RemoveIndex(0);
		return res;
	}

	// overflow text input.
	static char *s = NULL;
	static size_t s_pos = 0;

	if (s) {
		res.evType = SE_CHAR;
		res.evValue = s[s_pos];

		s_pos++;
		if (!s[s_pos]) {
			free(s);
			s = NULL;
			s_pos = 0;
		}

		return res;
	}

	static byte c = 0;

	if (c) {
		res.evType = SE_CHAR;
		res.evValue = c;

		c = 0;

		return res;
	}

    bool getNext = true;
	while (SDL_PollEvent(&ev) && getNext) {
        getNext = false;
		switch (ev.type) {
#ifdef __WINDOWS__ // on windows we need to grab the hwnd.
		case SDL_SYSWMEVENT:
			if (win32.hWnd == NULL)
			{
				win32.hWnd = ev.syswm.msg->msg.win.hwnd;
			}
			getNext = true; // try to get a decent event.
			break;
#endif
		case SDL_WINDOWEVENT:
			switch (ev.window.event) {
				case SDL_WINDOWEVENT_FOCUS_GAINED: {
						// unset modifier, in case alt-tab was used to leave window and ALT is still set
						// as that can cause fullscreen-toggling when pressing enter...
						SDL_Keymod currentmod = SDL_GetModState();
					
						int newmod = KMOD_NONE;
						if (currentmod & KMOD_CAPS) // preserve capslock
							newmod |= KMOD_CAPS;

						SDL_SetModState((SDL_Keymod)newmod);
					} // new context because visual studio complains about newmod and currentmod not initialized because of the case SDL_WINDOWEVENT_FOCUS_LOST

					GLimp_GrabInput(GRAB_ENABLE | GRAB_REENABLE | GRAB_HIDECURSOR);
					break;
				case SDL_WINDOWEVENT_FOCUS_LOST:
					GLimp_GrabInput(0);
					break;
			}

			return res_none;

		case SDL_KEYDOWN:
			if (ev.key.keysym.sym == SDLK_RETURN && (ev.key.keysym.mod & KMOD_ALT) > 0) {
				cvarSystem->SetCVarBool("r_fullscreen", !renderSystem->IsFullScreen());
				PushConsoleEvent("vid_restart");
				return res_none;
			}

			// fall through
		case SDL_KEYUP:
			key = mapkey(ev.key.keysym.sym);
			if(!key) {
				if (ev.key.keysym.scancode == SDL_SCANCODE_GRAVE) {
					key = Sys_GetConsoleKey(true);
				} else {
					if (ev.type == SDL_KEYDOWN) {
						common->Warning("unmapped SDL key %d", ev.key.keysym.sym);
            			getNext = true; // try to get a decent event.
						break;
					}

				}
			}

			res.evType = SE_KEY;
			res.evValue = key;
			res.evValue2 = ev.key.state == SDL_PRESSED ? 1 : 0;

			kbd_polls.Append(kbd_poll_t(key, ev.key.state == SDL_PRESSED));

			if ( (key == K_BACKSPACE && ev.key.state == SDL_PRESSED)
				|| SDL_GetEventState(SDL_TEXTINPUT) == SDL_DISABLE)
				c = key;

			return res;

		case SDL_TEXTINPUT:
			if (ev.text.text && *ev.text.text) {

				res.evType = SE_CHAR;
				res.evValue = *ev.text.text;
				
				// if there are more characters hold onto them for later events.
				if (ev.text.text[1] != 0)
					s = strdup(ev.text.text+1);

				return res;
			}

			getNext = true; // try to get a decent event.
			break;

		case SDL_MOUSEMOTION:
			if (g_inputGrabbed)
			{
				res.evType = SE_MOUSE;

				res.evValue = ev.motion.xrel;
				res.evValue2 = ev.motion.yrel;

				mouse_polls.Append(mouse_poll_t(M_DELTAX, ev.motion.xrel));
				mouse_polls.Append(mouse_poll_t(M_DELTAY, ev.motion.yrel));
			
				return res;
			}

			getNext = true;
			break;

		case SDL_MOUSEWHEEL:
			if (g_inputGrabbed)
			{
				res.evType = SE_KEY;

				if (ev.wheel.y > 0) {
					res.evValue = K_MWHEELUP;
					mouse_polls.Append(mouse_poll_t(M_DELTAZ, 1));
				} else {
					res.evValue = K_MWHEELDOWN;
					mouse_polls.Append(mouse_poll_t(M_DELTAZ, -1));
				}

				res.evValue2 = 1;

				return res;
			}

			getNext = true;
			break;

		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			if (g_inputGrabbed)
			{
				res.evType = SE_KEY;

				switch (ev.button.button) {
				case SDL_BUTTON_LEFT:
					res.evValue = K_MOUSE1;
					mouse_polls.Append(mouse_poll_t(M_ACTION1, ev.button.state == SDL_PRESSED ? 1 : 0));
					break;
				case SDL_BUTTON_MIDDLE:
					res.evValue = K_MOUSE3;
					mouse_polls.Append(mouse_poll_t(M_ACTION3, ev.button.state == SDL_PRESSED ? 1 : 0));
					break;
				case SDL_BUTTON_RIGHT:
					res.evValue = K_MOUSE2;
					mouse_polls.Append(mouse_poll_t(M_ACTION2, ev.button.state == SDL_PRESSED ? 1 : 0));
					break;
				}

				res.evValue2 = ev.button.state == SDL_PRESSED ? 1 : 0;

				return res;
			}

			getNext = true;
			break;

		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP:
			{
				sys_jEvents jEvent =  mapjoybutton( (SDL_GameControllerButton)ev.cbutton.button);
				joystick_polls.Append(joystick_poll_t(	jEvent,
														ev.cbutton.state == SDL_PRESSED ? 1 : 0) );

				res.evType = SE_KEY;
				res.evValue2 = ev.cbutton.state == SDL_PRESSED ? 1 : 0;
				if ( ( jEvent >= J_ACTION1 ) && ( jEvent <= J_ACTION_MAX ) ) {
					res.evValue = K_JOY1 + ( jEvent - J_ACTION1 );
					return res;
				} else if ( ( jEvent >= J_DPAD_UP ) && ( jEvent <= J_DPAD_RIGHT ) ) {
					res.evValue = K_JOY_DPAD_UP + ( jEvent - J_DPAD_UP );
					return res;
				}

				getNext = true; // try to get a decent event.
			}
			break;

		case SDL_CONTROLLERAXISMOTION:
			{
				const int range = 16384;

				sys_jEvents jEvent = mapjoyaxis( (SDL_GameControllerAxis)ev.caxis.axis);
				joystick_polls.Append(joystick_poll_t(	jEvent, ev.caxis.value) );

				if ( jEvent == J_AXIS_LEFT_X ) {
					PushButton( K_JOY_STICK1_LEFT, ( ev.caxis.value < -range ) );
					PushButton( K_JOY_STICK1_RIGHT, ( ev.caxis.value > range ) );
				} else if ( jEvent == J_AXIS_LEFT_Y ) {
					PushButton( K_JOY_STICK1_UP, ( ev.caxis.value < -range ) );
					PushButton( K_JOY_STICK1_DOWN, ( ev.caxis.value > range ) );
				} else if ( jEvent == J_AXIS_RIGHT_X ) {
					PushButton( K_JOY_STICK2_LEFT, ( ev.caxis.value < -range ) );
					PushButton( K_JOY_STICK2_RIGHT, ( ev.caxis.value > range ) );
				} else if ( jEvent == J_AXIS_RIGHT_Y ) {
					PushButton( K_JOY_STICK2_UP, ( ev.caxis.value < -range ) );
					PushButton( K_JOY_STICK2_DOWN, ( ev.caxis.value > range ) );
				} else if ( jEvent == J_AXIS_LEFT_TRIG ) {
					PushButton( K_JOY_TRIGGER1, ( ev.caxis.value > range ) );
				} else if ( jEvent == J_AXIS_RIGHT_TRIG ) {
					PushButton( K_JOY_TRIGGER2, ( ev.caxis.value > range ) );
				}
				if ( jEvent >= J_AXIS_MIN && jEvent <= J_AXIS_MAX ) {
					int axis = jEvent - J_AXIS_MIN;
					int percent = ( ev.caxis.value * 16 ) / range;
					if ( joyAxis[axis] != percent ) {
						joyAxis[axis] = percent;
						res.evType = SE_JOYSTICK;
						res.evValue = axis;
						res.evValue2 = percent;
						return res;
					}
				}

				getNext = true; // try to get a decent event.
			}
			break;

		case SDL_JOYDEVICEADDED:
			SDL_GameControllerOpen( ev.jdevice.which );
			// TODO: hot swapping maybe.
			//lbOnControllerPlugIn(event.jdevice.which);
			break;

		case SDL_JOYDEVICEREMOVED:
			// TODO: hot swapping maybe.
			//lbOnControllerUnPlug(event.jdevice.which);
			break;

		case SDL_QUIT:
			PushConsoleEvent("quit");
			return res_none;

		case SDL_USEREVENT:
			switch (ev.user.code) {
			case SE_CONSOLE:
				res.evType = SE_CONSOLE;
				res.evPtrLength = (intptr_t)ev.user.data1;
				res.evPtr = ev.user.data2;
				return res;
			default:
				common->Warning("unknown user event %u", ev.user.code);
            	getNext = true; // try to get a decent event.
            	break;
			}
		default:
            getNext = true; // try to get a decent event.
            break;
		}
	}

#ifdef _DIII4A //karin: pull Android events
    Android_PollInput();
#endif

	return res_none;
}

/*
================
Sys_ClearEvents
================
*/
void Sys_ClearEvents() {
	SDL_Event ev;

	while (SDL_PollEvent(&ev))
		;

	kbd_polls.SetNum(0, false);
	mouse_polls.SetNum(0, false);
	joystick_polls.SetNum(0, false);

	memset( buttonStates, 0, sizeof( buttonStates ) );
	memset( joyAxis, 0, sizeof( joyAxis ) );

	event_overflow.SetNum(0, false);
}

/*
================
Sys_GenerateEvents
================
*/
void Sys_GenerateEvents() {
	char *s = Sys_ConsoleInput();

	if (s)
		PushConsoleEvent(s);

	SDL_PumpEvents();
}

/*
================
Sys_PollKeyboardInputEvents
================
*/
int Sys_PollKeyboardInputEvents() {
	return kbd_polls.Num();
}

/*
================
Sys_ReturnKeyboardInputEvent
================
*/
int Sys_ReturnKeyboardInputEvent(const int n, int &key, bool &state) {
	if (n >= kbd_polls.Num())
		return 0;

	key = kbd_polls[n].key;
	state = kbd_polls[n].state;
	return 1;
}

/*
================
Sys_EndKeyboardInputEvents
================
*/
void Sys_EndKeyboardInputEvents() {
	kbd_polls.SetNum(0, false);
}

/*
================
Sys_PollMouseInputEvents
================
*/
int Sys_PollMouseInputEvents() {
	return mouse_polls.Num();
}

/*
================
Sys_ReturnMouseInputEvent
================
*/
int	Sys_ReturnMouseInputEvent(const int n, int &action, int &value) {
	if (n >= mouse_polls.Num())
		return 0;

	action = mouse_polls[n].action;
	value = mouse_polls[n].value;
	return 1;
}

/*
================
Sys_EndMouseInputEvents
================
*/
void Sys_EndMouseInputEvents() {
	mouse_polls.SetNum(0, false);
}

/*
================
Joystick Input Methods
================
*/
void Sys_SetRumble( int device, int low, int hi ) {
	// TODO: support multiple controllers.
	assert(device == 0);
	// TODO: support rumble maybe.
	assert(0);
}

int	Sys_PollJoystickInputEvents( int deviceNum ) {
	// TODO: support multiple controllers.
	assert(deviceNum == 0);
	return joystick_polls.Num();
}

int	Sys_ReturnJoystickInputEvent( const int n, int &action, int &value ) {
	if (n >= joystick_polls.Num())
		return 0;

	action = joystick_polls[n].action;
	value = joystick_polls[n].value;
	return 1;
}

void Sys_EndJoystickInputEvents() {
	joystick_polls.SetNum(0, false);
}

#endif
