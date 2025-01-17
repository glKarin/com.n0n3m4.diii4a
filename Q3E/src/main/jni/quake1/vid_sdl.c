/*
Copyright (C) 2003  T. Joseph Carter

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
#undef WIN32_LEAN_AND_MEAN  //hush a warning, SDL.h redefines this
#include <SDL.h>
#include <stdio.h>

#include "quakedef.h"
#include "image.h"
#include "utf8lib.h"

#ifndef __IPHONEOS__
#ifdef MACOSX
#include <Carbon/Carbon.h>
#include <IOKit/hidsystem/IOHIDLib.h>
#include <IOKit/hidsystem/IOHIDParameter.h>
#include <IOKit/hidsystem/event_status_driver.h>
#if (MAC_OS_X_VERSION_MIN_REQUIRED < 120000)
	#define IOMainPort IOMasterPort
#endif
static cvar_t apple_mouse_noaccel = {CF_CLIENT | CF_ARCHIVE, "apple_mouse_noaccel", "1", "disables mouse acceleration while DarkPlaces is active"};
static qbool vid_usingnoaccel;
static double originalMouseSpeed = -1.0;
static io_connect_t IN_GetIOHandle(void)
{
	io_connect_t iohandle = MACH_PORT_NULL;
	kern_return_t status;
	io_service_t iohidsystem = MACH_PORT_NULL;
	mach_port_t masterport;

	status = IOMainPort(MACH_PORT_NULL, &masterport);
	if(status != KERN_SUCCESS)
		return 0;

	iohidsystem = IORegistryEntryFromPath(masterport, kIOServicePlane ":/IOResources/IOHIDSystem");
	if(!iohidsystem)
		return 0;

	status = IOServiceOpen(iohidsystem, mach_task_self(), kIOHIDParamConnectType, &iohandle);
	IOObjectRelease(iohidsystem);

	return iohandle;
}
#endif
#endif


// Tell startup code that we have a client
int cl_available = true;

qbool vid_supportrefreshrate = false;

static qbool vid_usingmouse = false;
static qbool vid_usingmouse_relativeworks = false; // SDL2 workaround for unimplemented RelativeMouse mode
static qbool vid_usinghidecursor = false;
static qbool vid_hasfocus = false;
static qbool vid_wmborder_waiting, vid_wmborderless;
static SDL_Joystick *vid_sdljoystick = NULL;
static SDL_GameController *vid_sdlgamecontroller = NULL;
static cvar_t joy_sdl2_trigger_deadzone = {CF_ARCHIVE | CF_CLIENT, "joy_sdl2_trigger_deadzone", "0.5", "deadzone for triggers to be registered as key presses"};
// GAME_STEELSTORM specific
static cvar_t *steelstorm_showing_map = NULL; // detect but do not create the cvar
static cvar_t *steelstorm_showing_mousecursor = NULL; // detect but do not create the cvar

static SDL_GLContext context;
static SDL_Window *window;

// Input handling

#ifndef SDLK_PERCENT
#define SDLK_PERCENT '%'
#endif

static int MapKey( unsigned int sdlkey )
{
	switch(sdlkey)
	{
	// sdlkey can be Unicode codepoint for non-ascii keys, which are valid
	default:                      return sdlkey & SDLK_SCANCODE_MASK ? 0 : sdlkey;
//	case SDLK_UNKNOWN:            return K_UNKNOWN;
	case SDLK_RETURN:             return K_ENTER;
	case SDLK_ESCAPE:             return K_ESCAPE;
	case SDLK_BACKSPACE:          return K_BACKSPACE;
	case SDLK_TAB:                return K_TAB;
	case SDLK_SPACE:              return K_SPACE;
	case SDLK_EXCLAIM:            return '!';
	case SDLK_QUOTEDBL:           return '"';
	case SDLK_HASH:               return '#';
	case SDLK_PERCENT:            return '%';
	case SDLK_DOLLAR:             return '$';
	case SDLK_AMPERSAND:          return '&';
	case SDLK_QUOTE:              return '\'';
	case SDLK_LEFTPAREN:          return '(';
	case SDLK_RIGHTPAREN:         return ')';
	case SDLK_ASTERISK:           return '*';
	case SDLK_PLUS:               return '+';
	case SDLK_COMMA:              return ',';
	case SDLK_MINUS:              return '-';
	case SDLK_PERIOD:             return '.';
	case SDLK_SLASH:              return '/';
	case SDLK_0:                  return '0';
	case SDLK_1:                  return '1';
	case SDLK_2:                  return '2';
	case SDLK_3:                  return '3';
	case SDLK_4:                  return '4';
	case SDLK_5:                  return '5';
	case SDLK_6:                  return '6';
	case SDLK_7:                  return '7';
	case SDLK_8:                  return '8';
	case SDLK_9:                  return '9';
	case SDLK_COLON:              return ':';
	case SDLK_SEMICOLON:          return ';';
	case SDLK_LESS:               return '<';
	case SDLK_EQUALS:             return '=';
	case SDLK_GREATER:            return '>';
	case SDLK_QUESTION:           return '?';
	case SDLK_AT:                 return '@';
	case SDLK_LEFTBRACKET:        return '[';
	case SDLK_BACKSLASH:          return '\\';
	case SDLK_RIGHTBRACKET:       return ']';
	case SDLK_CARET:              return '^';
	case SDLK_UNDERSCORE:         return '_';
	case SDLK_BACKQUOTE:          return '`';
	case SDLK_a:                  return 'a';
	case SDLK_b:                  return 'b';
	case SDLK_c:                  return 'c';
	case SDLK_d:                  return 'd';
	case SDLK_e:                  return 'e';
	case SDLK_f:                  return 'f';
	case SDLK_g:                  return 'g';
	case SDLK_h:                  return 'h';
	case SDLK_i:                  return 'i';
	case SDLK_j:                  return 'j';
	case SDLK_k:                  return 'k';
	case SDLK_l:                  return 'l';
	case SDLK_m:                  return 'm';
	case SDLK_n:                  return 'n';
	case SDLK_o:                  return 'o';
	case SDLK_p:                  return 'p';
	case SDLK_q:                  return 'q';
	case SDLK_r:                  return 'r';
	case SDLK_s:                  return 's';
	case SDLK_t:                  return 't';
	case SDLK_u:                  return 'u';
	case SDLK_v:                  return 'v';
	case SDLK_w:                  return 'w';
	case SDLK_x:                  return 'x';
	case SDLK_y:                  return 'y';
	case SDLK_z:                  return 'z';
	case SDLK_CAPSLOCK:           return K_CAPSLOCK;
	case SDLK_F1:                 return K_F1;
	case SDLK_F2:                 return K_F2;
	case SDLK_F3:                 return K_F3;
	case SDLK_F4:                 return K_F4;
	case SDLK_F5:                 return K_F5;
	case SDLK_F6:                 return K_F6;
	case SDLK_F7:                 return K_F7;
	case SDLK_F8:                 return K_F8;
	case SDLK_F9:                 return K_F9;
	case SDLK_F10:                return K_F10;
	case SDLK_F11:                return K_F11;
	case SDLK_F12:                return K_F12;
	case SDLK_PRINTSCREEN:        return K_PRINTSCREEN;
	case SDLK_SCROLLLOCK:         return K_SCROLLOCK;
	case SDLK_PAUSE:              return K_PAUSE;
	case SDLK_INSERT:             return K_INS;
	case SDLK_HOME:               return K_HOME;
	case SDLK_PAGEUP:             return K_PGUP;
#ifdef __IPHONEOS__
	case SDLK_DELETE:             return K_BACKSPACE;
#else
	case SDLK_DELETE:             return K_DEL;
#endif
	case SDLK_END:                return K_END;
	case SDLK_PAGEDOWN:           return K_PGDN;
	case SDLK_RIGHT:              return K_RIGHTARROW;
	case SDLK_LEFT:               return K_LEFTARROW;
	case SDLK_DOWN:               return K_DOWNARROW;
	case SDLK_UP:                 return K_UPARROW;
	case SDLK_NUMLOCKCLEAR:       return K_NUMLOCK;
	case SDLK_KP_DIVIDE:          return K_KP_DIVIDE;
	case SDLK_KP_MULTIPLY:        return K_KP_MULTIPLY;
	case SDLK_KP_MINUS:           return K_KP_MINUS;
	case SDLK_KP_PLUS:            return K_KP_PLUS;
	case SDLK_KP_ENTER:           return K_KP_ENTER;
	case SDLK_KP_1:               return ((SDL_GetModState() & KMOD_NUM) ? K_KP_1 : K_END);
	case SDLK_KP_2:               return ((SDL_GetModState() & KMOD_NUM) ? K_KP_2 : K_DOWNARROW);
	case SDLK_KP_3:               return ((SDL_GetModState() & KMOD_NUM) ? K_KP_3 : K_PGDN);
	case SDLK_KP_4:               return ((SDL_GetModState() & KMOD_NUM) ? K_KP_4 : K_LEFTARROW);
	case SDLK_KP_5:               return K_KP_5;
	case SDLK_KP_6:               return ((SDL_GetModState() & KMOD_NUM) ? K_KP_6 : K_RIGHTARROW);
	case SDLK_KP_7:               return ((SDL_GetModState() & KMOD_NUM) ? K_KP_7 : K_HOME);
	case SDLK_KP_8:               return ((SDL_GetModState() & KMOD_NUM) ? K_KP_8 : K_UPARROW);
	case SDLK_KP_9:               return ((SDL_GetModState() & KMOD_NUM) ? K_KP_9 : K_PGUP);
	case SDLK_KP_0:               return ((SDL_GetModState() & KMOD_NUM) ? K_KP_0 : K_INS);
	case SDLK_KP_PERIOD:          return ((SDL_GetModState() & KMOD_NUM) ? K_KP_PERIOD : K_DEL);
//	case SDLK_APPLICATION:        return K_APPLICATION;
//	case SDLK_POWER:              return K_POWER;
	case SDLK_KP_EQUALS:          return K_KP_EQUALS;
//	case SDLK_F13:                return K_F13;
//	case SDLK_F14:                return K_F14;
//	case SDLK_F15:                return K_F15;
//	case SDLK_F16:                return K_F16;
//	case SDLK_F17:                return K_F17;
//	case SDLK_F18:                return K_F18;
//	case SDLK_F19:                return K_F19;
//	case SDLK_F20:                return K_F20;
//	case SDLK_F21:                return K_F21;
//	case SDLK_F22:                return K_F22;
//	case SDLK_F23:                return K_F23;
//	case SDLK_F24:                return K_F24;
//	case SDLK_EXECUTE:            return K_EXECUTE;
//	case SDLK_HELP:               return K_HELP;
//	case SDLK_MENU:               return K_MENU;
//	case SDLK_SELECT:             return K_SELECT;
//	case SDLK_STOP:               return K_STOP;
//	case SDLK_AGAIN:              return K_AGAIN;
//	case SDLK_UNDO:               return K_UNDO;
//	case SDLK_CUT:                return K_CUT;
//	case SDLK_COPY:               return K_COPY;
//	case SDLK_PASTE:              return K_PASTE;
//	case SDLK_FIND:               return K_FIND;
//	case SDLK_MUTE:               return K_MUTE;
//	case SDLK_VOLUMEUP:           return K_VOLUMEUP;
//	case SDLK_VOLUMEDOWN:         return K_VOLUMEDOWN;
//	case SDLK_KP_COMMA:           return K_KP_COMMA;
//	case SDLK_KP_EQUALSAS400:     return K_KP_EQUALSAS400;
//	case SDLK_ALTERASE:           return K_ALTERASE;
//	case SDLK_SYSREQ:             return K_SYSREQ;
//	case SDLK_CANCEL:             return K_CANCEL;
//	case SDLK_CLEAR:              return K_CLEAR;
//	case SDLK_PRIOR:              return K_PRIOR;
//	case SDLK_RETURN2:            return K_RETURN2;
//	case SDLK_SEPARATOR:          return K_SEPARATOR;
//	case SDLK_OUT:                return K_OUT;
//	case SDLK_OPER:               return K_OPER;
//	case SDLK_CLEARAGAIN:         return K_CLEARAGAIN;
//	case SDLK_CRSEL:              return K_CRSEL;
//	case SDLK_EXSEL:              return K_EXSEL;
//	case SDLK_KP_00:              return K_KP_00;
//	case SDLK_KP_000:             return K_KP_000;
//	case SDLK_THOUSANDSSEPARATOR: return K_THOUSANDSSEPARATOR;
//	case SDLK_DECIMALSEPARATOR:   return K_DECIMALSEPARATOR;
//	case SDLK_CURRENCYUNIT:       return K_CURRENCYUNIT;
//	case SDLK_CURRENCYSUBUNIT:    return K_CURRENCYSUBUNIT;
//	case SDLK_KP_LEFTPAREN:       return K_KP_LEFTPAREN;
//	case SDLK_KP_RIGHTPAREN:      return K_KP_RIGHTPAREN;
//	case SDLK_KP_LEFTBRACE:       return K_KP_LEFTBRACE;
//	case SDLK_KP_RIGHTBRACE:      return K_KP_RIGHTBRACE;
//	case SDLK_KP_TAB:             return K_KP_TAB;
//	case SDLK_KP_BACKSPACE:       return K_KP_BACKSPACE;
//	case SDLK_KP_A:               return K_KP_A;
//	case SDLK_KP_B:               return K_KP_B;
//	case SDLK_KP_C:               return K_KP_C;
//	case SDLK_KP_D:               return K_KP_D;
//	case SDLK_KP_E:               return K_KP_E;
//	case SDLK_KP_F:               return K_KP_F;
//	case SDLK_KP_XOR:             return K_KP_XOR;
//	case SDLK_KP_POWER:           return K_KP_POWER;
//	case SDLK_KP_PERCENT:         return K_KP_PERCENT;
//	case SDLK_KP_LESS:            return K_KP_LESS;
//	case SDLK_KP_GREATER:         return K_KP_GREATER;
//	case SDLK_KP_AMPERSAND:       return K_KP_AMPERSAND;
//	case SDLK_KP_DBLAMPERSAND:    return K_KP_DBLAMPERSAND;
//	case SDLK_KP_VERTICALBAR:     return K_KP_VERTICALBAR;
//	case SDLK_KP_DBLVERTICALBAR:  return K_KP_DBLVERTICALBAR;
//	case SDLK_KP_COLON:           return K_KP_COLON;
//	case SDLK_KP_HASH:            return K_KP_HASH;
//	case SDLK_KP_SPACE:           return K_KP_SPACE;
//	case SDLK_KP_AT:              return K_KP_AT;
//	case SDLK_KP_EXCLAM:          return K_KP_EXCLAM;
//	case SDLK_KP_MEMSTORE:        return K_KP_MEMSTORE;
//	case SDLK_KP_MEMRECALL:       return K_KP_MEMRECALL;
//	case SDLK_KP_MEMCLEAR:        return K_KP_MEMCLEAR;
//	case SDLK_KP_MEMADD:          return K_KP_MEMADD;
//	case SDLK_KP_MEMSUBTRACT:     return K_KP_MEMSUBTRACT;
//	case SDLK_KP_MEMMULTIPLY:     return K_KP_MEMMULTIPLY;
//	case SDLK_KP_MEMDIVIDE:       return K_KP_MEMDIVIDE;
//	case SDLK_KP_PLUSMINUS:       return K_KP_PLUSMINUS;
//	case SDLK_KP_CLEAR:           return K_KP_CLEAR;
//	case SDLK_KP_CLEARENTRY:      return K_KP_CLEARENTRY;
//	case SDLK_KP_BINARY:          return K_KP_BINARY;
//	case SDLK_KP_OCTAL:           return K_KP_OCTAL;
//	case SDLK_KP_DECIMAL:         return K_KP_DECIMAL;
//	case SDLK_KP_HEXADECIMAL:     return K_KP_HEXADECIMAL;
	case SDLK_LCTRL:              return K_CTRL;
	case SDLK_LSHIFT:             return K_SHIFT;
	case SDLK_LALT:               return K_ALT;
//	case SDLK_LGUI:               return K_LGUI;
	case SDLK_RCTRL:              return K_CTRL;
	case SDLK_RSHIFT:             return K_SHIFT;
	case SDLK_RALT:               return K_ALT;
//	case SDLK_RGUI:               return K_RGUI;
//	case SDLK_MODE:               return K_MODE;
//	case SDLK_AUDIONEXT:          return K_AUDIONEXT;
//	case SDLK_AUDIOPREV:          return K_AUDIOPREV;
//	case SDLK_AUDIOSTOP:          return K_AUDIOSTOP;
//	case SDLK_AUDIOPLAY:          return K_AUDIOPLAY;
//	case SDLK_AUDIOMUTE:          return K_AUDIOMUTE;
//	case SDLK_MEDIASELECT:        return K_MEDIASELECT;
//	case SDLK_WWW:                return K_WWW;
//	case SDLK_MAIL:               return K_MAIL;
//	case SDLK_CALCULATOR:         return K_CALCULATOR;
//	case SDLK_COMPUTER:           return K_COMPUTER;
//	case SDLK_AC_SEARCH:          return K_AC_SEARCH; // Android button
//	case SDLK_AC_HOME:            return K_AC_HOME; // Android button
	case SDLK_AC_BACK:            return K_ESCAPE; // Android button
//	case SDLK_AC_FORWARD:         return K_AC_FORWARD; // Android button
//	case SDLK_AC_STOP:            return K_AC_STOP; // Android button
//	case SDLK_AC_REFRESH:         return K_AC_REFRESH; // Android button
//	case SDLK_AC_BOOKMARKS:       return K_AC_BOOKMARKS; // Android button
//	case SDLK_BRIGHTNESSDOWN:     return K_BRIGHTNESSDOWN;
//	case SDLK_BRIGHTNESSUP:       return K_BRIGHTNESSUP;
//	case SDLK_DISPLAYSWITCH:      return K_DISPLAYSWITCH;
//	case SDLK_KBDILLUMTOGGLE:     return K_KBDILLUMTOGGLE;
//	case SDLK_KBDILLUMDOWN:       return K_KBDILLUMDOWN;
//	case SDLK_KBDILLUMUP:         return K_KBDILLUMUP;
//	case SDLK_EJECT:              return K_EJECT;
//	case SDLK_SLEEP:              return K_SLEEP;
	}
}

qbool VID_HasScreenKeyboardSupport(void)
{
	return SDL_HasScreenKeyboardSupport() != SDL_FALSE;
}

void VID_ShowKeyboard(qbool show)
{
	if (!SDL_HasScreenKeyboardSupport())
		return;

	if (show)
	{
		if (!SDL_IsTextInputActive())
			SDL_StartTextInput();
	}
	else
	{
		if (SDL_IsTextInputActive())
			SDL_StopTextInput();
	}
}

qbool VID_ShowingKeyboard(void)
{
	return SDL_IsTextInputActive() != 0;
}

static void VID_SetMouse(qbool relative, qbool hidecursor)
{
#ifndef DP_MOBILETOUCH
#ifdef MACOSX
	if(relative)
		if(vid_usingmouse && (vid_usingnoaccel != !!apple_mouse_noaccel.integer))
			VID_SetMouse(false, false); // ungrab first!
#endif
	if (vid_usingmouse != relative)
	{
		vid_usingmouse = relative;
		cl_ignoremousemoves = 2;
		vid_usingmouse_relativeworks = SDL_SetRelativeMouseMode(relative ? SDL_TRUE : SDL_FALSE) == 0;
//		Con_Printf("VID_SetMouse(%i, %i) relativeworks = %i\n", (int)relative, (int)hidecursor, (int)vid_usingmouse_relativeworks);
#ifdef MACOSX
		if(relative)
		{
			// Save the status of mouse acceleration
			originalMouseSpeed = -1.0; // in case of error
			if(apple_mouse_noaccel.integer)
			{
				io_connect_t mouseDev = IN_GetIOHandle();
				if(mouseDev != 0)
				{
					if(IOHIDGetAccelerationWithKey(mouseDev, CFSTR(kIOHIDMouseAccelerationType), &originalMouseSpeed) == kIOReturnSuccess)
					{
						Con_DPrintf("previous mouse acceleration: %f\n", originalMouseSpeed);
						if(IOHIDSetAccelerationWithKey(mouseDev, CFSTR(kIOHIDMouseAccelerationType), -1.0) != kIOReturnSuccess)
						{
							Con_Print("Could not disable mouse acceleration (failed at IOHIDSetAccelerationWithKey).\n");
							Cvar_SetValueQuick(&apple_mouse_noaccel, 0);
						}
					}
					else
					{
						Con_Print("Could not disable mouse acceleration (failed at IOHIDGetAccelerationWithKey).\n");
						Cvar_SetValueQuick(&apple_mouse_noaccel, 0);
					}
					IOServiceClose(mouseDev);
				}
				else
				{
					Con_Print("Could not disable mouse acceleration (failed at IO_GetIOHandle).\n");
					Cvar_SetValueQuick(&apple_mouse_noaccel, 0);
				}
			}

			vid_usingnoaccel = !!apple_mouse_noaccel.integer;
		}
		else
		{
			if(originalMouseSpeed != -1.0)
			{
				io_connect_t mouseDev = IN_GetIOHandle();
				if(mouseDev != 0)
				{
					Con_DPrintf("restoring mouse acceleration to: %f\n", originalMouseSpeed);
					if(IOHIDSetAccelerationWithKey(mouseDev, CFSTR(kIOHIDMouseAccelerationType), originalMouseSpeed) != kIOReturnSuccess)
						Con_Print("Could not re-enable mouse acceleration (failed at IOHIDSetAccelerationWithKey).\n");
					IOServiceClose(mouseDev);
				}
				else
					Con_Print("Could not re-enable mouse acceleration (failed at IO_GetIOHandle).\n");
			}
		}
#endif
	}
	if (vid_usinghidecursor != hidecursor)
	{
		vid_usinghidecursor = hidecursor;
		SDL_ShowCursor( hidecursor ? SDL_DISABLE : SDL_ENABLE);
	}
#endif
}

// multitouch[10][] represents the mouse pointer
// multitouch[][0]: finger active
// multitouch[][1]: Y
// multitouch[][2]: Y
// X and Y coordinates are 0-1.
#define MAXFINGERS 11
float multitouch[MAXFINGERS][3];

// this one stores how many areas this finger has touched
int multitouchs[MAXFINGERS];

// modified heavily by ELUAN
static qbool VID_TouchscreenArea(int corner, float px, float py, float pwidth, float pheight, const char *icon, float textheight, const char *text, float *resultmove, qbool *resultbutton, keynum_t key, const char *typedtext, float deadzone, float oversizepixels_x, float oversizepixels_y, qbool iamexclusive)
{
	int finger;
	float fx, fy, fwidth, fheight;
	float overfx, overfy, overfwidth, overfheight;
	float rel[3];
	float sqsum;
	qbool button = false;
	VectorClear(rel);
	if (pwidth > 0 && pheight > 0)
	{
		if (corner & 1) px += vid_conwidth.value;
		if (corner & 2) py += vid_conheight.value;
		if (corner & 4) px += vid_conwidth.value * 0.5f;
		if (corner & 8) py += vid_conheight.value * 0.5f;
		if (corner & 16) {px *= vid_conwidth.value * (1.0f / 640.0f);py *= vid_conheight.value * (1.0f / 480.0f);pwidth *= vid_conwidth.value * (1.0f / 640.0f);pheight *= vid_conheight.value * (1.0f / 480.0f);}
		fx = px / vid_conwidth.value;
		fy = py / vid_conheight.value;
		fwidth = pwidth / vid_conwidth.value;
		fheight = pheight / vid_conheight.value;

		// try to prevent oversizepixels_* from interfering with the iamexclusive cvar by not letting we start controlling from too far of the actual touch area (areas without resultbuttons should NEVER have the oversizepixels_* parameters set to anything other than 0)
		if (resultbutton)
			if (!(*resultbutton))
			{
				oversizepixels_x *= 0.2;
				oversizepixels_y *= 0.2;
			}

		oversizepixels_x /= vid_conwidth.value;
		oversizepixels_y /= vid_conheight.value;

		overfx = fx - oversizepixels_x;
		overfy = fy - oversizepixels_y;
		overfwidth = fwidth + 2*oversizepixels_x;
		overfheight = fheight + 2*oversizepixels_y;

		for (finger = 0;finger < MAXFINGERS;finger++)
		{
			if (multitouchs[finger] && iamexclusive) // for this to work correctly, you must call touch areas in order of highest to lowest priority
				continue;

			if (multitouch[finger][0] && multitouch[finger][1] >= overfx && multitouch[finger][2] >= overfy && multitouch[finger][1] < overfx + overfwidth && multitouch[finger][2] < overfy + overfheight)
			{
				multitouchs[finger]++;

				rel[0] = bound(-1, (multitouch[finger][1] - (fx + 0.5f * fwidth)) * (2.0f / fwidth), 1);
				rel[1] = bound(-1, (multitouch[finger][2] - (fy + 0.5f * fheight)) * (2.0f / fheight), 1);
				rel[2] = 0;

				sqsum = rel[0]*rel[0] + rel[1]*rel[1];
				// 2d deadzone
				if (sqsum < deadzone*deadzone)
				{
					rel[0] = 0;
					rel[1] = 0;
				}
				else if (sqsum > 1)
				{
					// ignore the third component
					Vector2Normalize2(rel, rel);
				}
				button = true;
				break;
			}
		}
		if (scr_numtouchscreenareas < 128)
		{
			scr_touchscreenareas[scr_numtouchscreenareas].pic = icon;
			scr_touchscreenareas[scr_numtouchscreenareas].text = text;
			scr_touchscreenareas[scr_numtouchscreenareas].textheight = textheight;
			scr_touchscreenareas[scr_numtouchscreenareas].rect[0] = px;
			scr_touchscreenareas[scr_numtouchscreenareas].rect[1] = py;
			scr_touchscreenareas[scr_numtouchscreenareas].rect[2] = pwidth;
			scr_touchscreenareas[scr_numtouchscreenareas].rect[3] = pheight;
			scr_touchscreenareas[scr_numtouchscreenareas].active = button;
			// the pics may have alpha too.
			scr_touchscreenareas[scr_numtouchscreenareas].activealpha = 1.f;
			scr_touchscreenareas[scr_numtouchscreenareas].inactivealpha = 0.95f;
			scr_numtouchscreenareas++;
		}
	}
	if (resultmove)
	{
		if (button)
			VectorCopy(rel, resultmove);
		else
			VectorClear(resultmove);
	}
	if (resultbutton)
	{
		if (*resultbutton != button)
		{
			if ((int)key > 0)
				Key_Event(key, 0, button);
			if (typedtext && typedtext[0] && !*resultbutton)
			{
				// FIXME: implement UTF8 support - nothing actually specifies a UTF8 string here yet, but should support it...
				int i;
				for (i = 0;typedtext[i];i++)
				{
					Key_Event(K_TEXT, typedtext[i], true);
					Key_Event(K_TEXT, typedtext[i], false);
				}
			}
		}
		*resultbutton = button;
	}
	return button;
}

// ELUAN:
// not reentrant, but we only need one mouse cursor anyway...
static void VID_TouchscreenCursor(float px, float py, float pwidth, float pheight, qbool *resultbutton, keynum_t key)
{
	int finger;
	float fx, fy, fwidth, fheight;
	qbool button = false;
	static int cursorfinger = -1;
	static int cursorfreemovement = false;
	static int canclick = false;
	static int clickxy[2];
	static int relclickxy[2];
	static double clickrealtime = 0;

	if (steelstorm_showing_mousecursor && steelstorm_showing_mousecursor->integer)
	if (pwidth > 0 && pheight > 0)
	{
		fx = px / vid_conwidth.value;
		fy = py / vid_conheight.value;
		fwidth = pwidth / vid_conwidth.value;
		fheight = pheight / vid_conheight.value;
		for (finger = 0;finger < MAXFINGERS;finger++)
		{
			if (multitouch[finger][0] && multitouch[finger][1] >= fx && multitouch[finger][2] >= fy && multitouch[finger][1] < fx + fwidth && multitouch[finger][2] < fy + fheight)
			{
				if (cursorfinger == -1)
				{
					clickxy[0] =  multitouch[finger][1] * vid_width.value - 0.5f * pwidth;
					clickxy[1] =  multitouch[finger][2] * vid_height.value - 0.5f * pheight;
					relclickxy[0] =  (multitouch[finger][1] - fx) * vid_width.value - 0.5f * pwidth;
					relclickxy[1] =  (multitouch[finger][2] - fy) * vid_height.value - 0.5f * pheight;
				}
				cursorfinger = finger;
				button = true;
				canclick = true;
				cursorfreemovement = false;
				break;
			}
		}
		if (scr_numtouchscreenareas < 128)
		{
			if (clickrealtime + 1 > host.realtime)
			{
				scr_touchscreenareas[scr_numtouchscreenareas].pic = "gfx/gui/touch_puck_cur_click.tga";
			}
			else if (button)
			{
				scr_touchscreenareas[scr_numtouchscreenareas].pic = "gfx/gui/touch_puck_cur_touch.tga";
			}
			else
			{
				switch ((int)host.realtime * 10 % 20)
				{
				case 0:
					scr_touchscreenareas[scr_numtouchscreenareas].pic = "gfx/gui/touch_puck_cur_touch.tga";
					break;
				default:
					scr_touchscreenareas[scr_numtouchscreenareas].pic = "gfx/gui/touch_puck_cur_idle.tga";
				}
			}
			scr_touchscreenareas[scr_numtouchscreenareas].text = "";
			scr_touchscreenareas[scr_numtouchscreenareas].textheight = 0;
			scr_touchscreenareas[scr_numtouchscreenareas].rect[0] = px;
			scr_touchscreenareas[scr_numtouchscreenareas].rect[1] = py;
			scr_touchscreenareas[scr_numtouchscreenareas].rect[2] = pwidth;
			scr_touchscreenareas[scr_numtouchscreenareas].rect[3] = pheight;
			scr_touchscreenareas[scr_numtouchscreenareas].active = button;
			scr_touchscreenareas[scr_numtouchscreenareas].activealpha = 1.0f;
			scr_touchscreenareas[scr_numtouchscreenareas].inactivealpha = 1.0f;
			scr_numtouchscreenareas++;
		}
	}

	if (cursorfinger != -1)
	{
		if (multitouch[cursorfinger][0])
		{
			if (multitouch[cursorfinger][1] * vid_width.value - 0.5f * pwidth < clickxy[0] - 1 ||
				multitouch[cursorfinger][1] * vid_width.value - 0.5f * pwidth > clickxy[0] + 1 ||
				multitouch[cursorfinger][2] * vid_height.value - 0.5f * pheight< clickxy[1] - 1 ||
				multitouch[cursorfinger][2] * vid_height.value - 0.5f * pheight> clickxy[1] + 1) // finger drifted more than the allowed amount
			{
				cursorfreemovement = true;
			}
			if (cursorfreemovement)
			{
				// in_windowmouse_x* is in screen resolution coordinates, not console resolution
				in_windowmouse_x = multitouch[cursorfinger][1] * vid_width.value - 0.5f * pwidth - relclickxy[0];
				in_windowmouse_y = multitouch[cursorfinger][2] * vid_height.value - 0.5f * pheight - relclickxy[1];
			}
		}
		else
		{
			cursorfinger = -1;
		}
	}

	if (resultbutton)
	{
		if (/**resultbutton != button && */(int)key > 0)
		{
			if (!button && !cursorfreemovement && canclick)
			{
				Key_Event(key, 0, true);
				canclick = false;
				clickrealtime = host.realtime;
			}

			// SS:BR can't qc can't cope with presses and releases on the same frame
			if (clickrealtime && clickrealtime + 0.1 < host.realtime)
			{
				Key_Event(key, 0, false);
				clickrealtime = 0;
			}
		}

		*resultbutton = button;
	}
}

void VID_BuildJoyState(vid_joystate_t *joystate)
{
	VID_Shared_BuildJoyState_Begin(joystate);

	if (vid_sdljoystick)
	{
		SDL_Joystick *joy = vid_sdljoystick;
		int j;

		if (vid_sdlgamecontroller)
		{
			for (j = 0; j <= SDL_CONTROLLER_AXIS_MAX; ++j)
			{
				joystate->axis[j] = SDL_GameControllerGetAxis(vid_sdlgamecontroller, (SDL_GameControllerAxis)j) * (1.0f / 32767.0f);
			}
			for (j = 0; j < SDL_CONTROLLER_BUTTON_MAX; ++j)
				joystate->button[j] = SDL_GameControllerGetButton(vid_sdlgamecontroller, (SDL_GameControllerButton)j);
			// emulate joy buttons for trigger "axes"
			joystate->button[SDL_CONTROLLER_BUTTON_MAX] = VID_JoyState_GetAxis(joystate, SDL_CONTROLLER_AXIS_TRIGGERLEFT, 1, joy_sdl2_trigger_deadzone.value) > 0.0f;
			joystate->button[SDL_CONTROLLER_BUTTON_MAX+1] = VID_JoyState_GetAxis(joystate, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 1, joy_sdl2_trigger_deadzone.value) > 0.0f;
		}
		else

		{
			int numaxes;
			int numbuttons;
			numaxes = SDL_JoystickNumAxes(joy);
			for (j = 0;j < numaxes;j++)
				joystate->axis[j] = SDL_JoystickGetAxis(joy, j) * (1.0f / 32767.0f);
			numbuttons = SDL_JoystickNumButtons(joy);
			for (j = 0;j < numbuttons;j++)
				joystate->button[j] = SDL_JoystickGetButton(joy, j);
		}
	}

	VID_Shared_BuildJoyState_Finish(joystate);
}

// clear every touch screen area, except the one with button[skip]
#define Vid_ClearAllTouchscreenAreas(skip) \
	if (skip != 0) \
		VID_TouchscreenCursor(0, 0, 0, 0, &buttons[0], K_MOUSE1); \
	if (skip != 1) \
		VID_TouchscreenArea( 0,   0,   0,   0,   0, NULL                         , 0.0f, NULL, move, &buttons[1], K_MOUSE4, NULL, 0, 0, 0, false); \
	if (skip != 2) \
		VID_TouchscreenArea( 0,   0,   0,   0,   0, NULL                         , 0.0f, NULL, aim,  &buttons[2], K_MOUSE5, NULL, 0, 0, 0, false); \
	if (skip != 3) \
		VID_TouchscreenArea( 0,   0,   0,   0,   0, NULL                         , 0.0f, NULL, NULL, &buttons[3], K_SHIFT, NULL, 0, 0, 0, false); \
	if (skip != 4) \
		VID_TouchscreenArea( 0,   0,   0,   0,   0, NULL                         , 0.0f, NULL, NULL, &buttons[4], K_MOUSE2, NULL, 0, 0, 0, false); \
	if (skip != 9) \
		VID_TouchscreenArea( 0,   0,   0,   0,   0, NULL                         , 0.0f, NULL, NULL, &buttons[9], K_MOUSE3, NULL, 0, 0, 0, false); \
	if (skip != 10) \
		VID_TouchscreenArea( 0,   0,   0,   0,   0, NULL                         , 0.0f, NULL, NULL, &buttons[10], (keynum_t)'m', NULL, 0, 0, 0, false); \
	if (skip != 11) \
		VID_TouchscreenArea( 0,   0,   0,   0,   0, NULL                         , 0.0f, NULL, NULL, &buttons[11], (keynum_t)'b', NULL, 0, 0, 0, false); \
	if (skip != 12) \
		VID_TouchscreenArea( 0,   0,   0,   0,   0, NULL                         , 0.0f, NULL, NULL, &buttons[12], (keynum_t)'q', NULL, 0, 0, 0, false); \
	if (skip != 13) \
		VID_TouchscreenArea( 0,   0,   0,   0,   0, NULL                         , 0.0f, NULL, NULL, &buttons[13], (keynum_t)'`', NULL, 0, 0, 0, false); \
	if (skip != 14) \
		VID_TouchscreenArea( 0,   0,   0,   0,   0, NULL                         , 0.0f, NULL, NULL, &buttons[14], K_ESCAPE, NULL, 0, 0, 0, false); \
	if (skip != 15) \
		VID_TouchscreenArea( 0,  0,  0,  0,  0, NULL                         , 0.0f, NULL, NULL, &buttons[15], K_SPACE, NULL, 0, 0, 0, false); \

/////////////////////
// Movement handling
////

static void IN_Move_TouchScreen_SteelStorm(void)
{
	// ELUAN
	int i, numfingers;
	float xscale, yscale;
	float move[3], aim[3];
	static qbool oldbuttons[128];
	static qbool buttons[128];
	keydest_t keydest = (key_consoleactive & KEY_CONSOLEACTIVE_USER) ? key_console : key_dest;
	memcpy(oldbuttons, buttons, sizeof(oldbuttons));
	memset(multitouchs, 0, sizeof(multitouchs));

	for (i = 0, numfingers = 0; i < MAXFINGERS - 1; i++)
		if (multitouch[i][0])
			numfingers++;

	/*
	Enable this to use a mouse as a touch device (it may conflict with the iamexclusive parameter if a finger is also reported as a mouse at the same location
	if (numfingers == 1)
	{
		multitouch[MAXFINGERS-1][0] = SDL_GetMouseState(&x, &y) ? 11 : 0;
		multitouch[MAXFINGERS-1][1] = (float)x / vid.width;
		multitouch[MAXFINGERS-1][2] = (float)y / vid.height;
	}
	else
	{
		// disable it so it doesn't get stuck, because SDL seems to stop updating it if there are more than 1 finger on screen
		multitouch[MAXFINGERS-1][0] = 0;
	}*/

	// TODO: make touchscreen areas controlled by a config file or the VMs. THIS IS A MESS!
	// TODO: can't just clear buttons[] when entering a new keydest, some keys would remain pressed
	// SS:BR menuqc has many peculiarities, including that it can't accept more than one command per frame and pressing and releasing on the same frame

	// Tuned for the SGS3, use it's value as a base. CLEAN THIS.
	xscale = vid_touchscreen_density.value / 2.0f;
	yscale = vid_touchscreen_density.value / 2.0f;
	switch(keydest)
	{
	case key_console:
		Vid_ClearAllTouchscreenAreas(14);
		VID_TouchscreenArea( 0,   0, 160,  64,  64, "gfx/gui/touch_menu_button.tga"         , 0.0f, NULL, NULL, &buttons[14], K_ESCAPE, NULL, 0, 0, 0, false);
		break;
	case key_game:
		if (steelstorm_showing_map && steelstorm_showing_map->integer) // FIXME: another hack to be removed when touchscreen areas go to QC
		{
			VID_TouchscreenArea( 0,   0,   0, vid_conwidth.value, vid_conheight.value, NULL                         , 0.0f, NULL, NULL, &buttons[10], (keynum_t)'m', NULL, 0, 0, 0, false);
			Vid_ClearAllTouchscreenAreas(10);
		}
		else if (steelstorm_showing_mousecursor && steelstorm_showing_mousecursor->integer)
		{
			// in_windowmouse_x* is in screen resolution coordinates, not console resolution
			VID_TouchscreenCursor((float)in_windowmouse_x/vid_width.value*vid_conwidth.value, (float)in_windowmouse_y/vid_height.value*vid_conheight.value, 192*xscale, 192*yscale, &buttons[0], K_MOUSE1);
			Vid_ClearAllTouchscreenAreas(0);
		}
		else
		{
			VID_TouchscreenCursor(0, 0, 0, 0, &buttons[0], K_MOUSE1);

			VID_TouchscreenArea( 2,16*xscale,-240*yscale, 224*xscale, 224*yscale, "gfx/gui/touch_l_thumb_dpad.tga", 0.0f, NULL, move, &buttons[1], (keynum_t)0, NULL, 0.15, 112*xscale, 112*yscale, false);

			VID_TouchscreenArea( 3,-240*xscale,-160*yscale, 224*xscale, 128*yscale, "gfx/gui/touch_r_thumb_turn_n_shoot.tga"    , 0.0f, NULL, NULL,  0, (keynum_t)0, NULL, 0, 56*xscale, 0, false);
			VID_TouchscreenArea( 3,-240*xscale,-256*yscale, 224*xscale, 224*yscale, NULL    , 0.0f, NULL, aim,  &buttons[2], (keynum_t)0, NULL, 0.2, 56*xscale, 0, false);

			VID_TouchscreenArea( 2, (vid_conwidth.value / 2) - 128,-80,  256,  80, NULL, 0.0f, NULL, NULL, &buttons[3], K_SHIFT, NULL, 0, 0, 0, true);

			VID_TouchscreenArea( 3,-240*xscale,-256*yscale, 224*xscale,  64*yscale, "gfx/gui/touch_secondary_slide.tga", 0.0f, NULL, NULL, &buttons[4], K_MOUSE2, NULL, 0, 56*xscale, 0, false);
			VID_TouchscreenArea( 3,-240*xscale,-256*yscale, 224*xscale,  160*yscale, NULL , 0.0f, NULL, NULL, &buttons[9], K_MOUSE3, NULL, 0.2, 56*xscale, 0, false);

			VID_TouchscreenArea( 1,-100,   0, 100, 100, NULL                         , 0.0f, NULL, NULL, &buttons[10], (keynum_t)'m', NULL, 0, 0, 0, true);
			VID_TouchscreenArea( 1,-100, 120, 100, 100, NULL                         , 0.0f, NULL, NULL, &buttons[11], (keynum_t)'b', NULL, 0, 0, 0, true);
			VID_TouchscreenArea( 0,   0,   0,  64,  64, NULL                         , 0.0f, NULL, NULL, &buttons[12], (keynum_t)'q', NULL, 0, 0, 0, true);
			if (developer.integer)
				VID_TouchscreenArea( 0,   0,  96,  64,  64, NULL                         , 0.0f, NULL, NULL, &buttons[13], (keynum_t)'`', NULL, 0, 0, 0, true);
			else
				VID_TouchscreenArea( 0,   0,   0,   0,   0, NULL                         , 0.0f, NULL, NULL, &buttons[13], (keynum_t)'`', NULL, 0, 0, 0, false);
			VID_TouchscreenArea( 0,   0, 160,  64,  64, "gfx/gui/touch_menu_button.tga"         , 0.0f, NULL, NULL, &buttons[14], K_ESCAPE, NULL, 0, 0, 0, true);
			switch(cl.activeweapon)
			{
			case 14:
				VID_TouchscreenArea( 2,  16*xscale,-320*yscale, 224*xscale, 64*yscale, "gfx/gui/touch_booster.tga" , 0.0f, NULL, NULL, &buttons[15], K_SPACE, NULL, 0, 0, 0, true);
				break;
			case 12:
				VID_TouchscreenArea( 2,  16*xscale,-320*yscale, 224*xscale, 64*yscale, "gfx/gui/touch_shockwave.tga" , 0.0f, NULL, NULL, &buttons[15], K_SPACE, NULL, 0, 0, 0, true);
				break;
			default:
				VID_TouchscreenArea( 0,  0,  0,  0,  0, NULL , 0.0f, NULL, NULL, &buttons[15], K_SPACE, NULL, 0, 0, 0, false);
			}
		}
		break;
	default:
		if (!steelstorm_showing_mousecursor || !steelstorm_showing_mousecursor->integer)
		{
			Vid_ClearAllTouchscreenAreas(14);
			// this way we can skip cutscenes
			VID_TouchscreenArea( 0,   0,   0, vid_conwidth.value, vid_conheight.value, NULL                         , 0.0f, NULL, NULL, &buttons[14], K_ESCAPE, NULL, 0, 0, 0, false);
		}
		else
		{
			// in_windowmouse_x* is in screen resolution coordinates, not console resolution
			VID_TouchscreenCursor((float)in_windowmouse_x/vid_width.value*vid_conwidth.value, (float)in_windowmouse_y/vid_height.value*vid_conheight.value, 192*xscale, 192*yscale, &buttons[0], K_MOUSE1);
			Vid_ClearAllTouchscreenAreas(0);
		}
		break;
	}

	if (VID_ShowingKeyboard() && (float)in_windowmouse_y > vid_height.value / 2 - 10)
		in_windowmouse_y = 128;

	cl.cmd.forwardmove -= move[1] * cl_forwardspeed.value;
	cl.cmd.sidemove += move[0] * cl_sidespeed.value;
	cl.viewangles[0] += aim[1] * cl_pitchspeed.value * cl.realframetime;
	cl.viewangles[1] -= aim[0] * cl_yawspeed.value * cl.realframetime;
}

static void IN_Move_TouchScreen_Quake(void)
{
	int x, y;
	float move[3], aim[3], click[3];
	static qbool oldbuttons[128];
	static qbool buttons[128];
	keydest_t keydest = (key_consoleactive & KEY_CONSOLEACTIVE_USER) ? key_console : key_dest;
	memcpy(oldbuttons, buttons, sizeof(oldbuttons));
	memset(multitouchs, 0, sizeof(multitouchs));

	// simple quake controls
	multitouch[MAXFINGERS-1][0] = SDL_GetMouseState(&x, &y);
	multitouch[MAXFINGERS-1][1] = x * 32768 / vid.mode.width;
	multitouch[MAXFINGERS-1][2] = y * 32768 / vid.mode.height;

	// top of screen is toggleconsole and K_ESCAPE
	switch(keydest)
	{
	case key_console:
		VID_TouchscreenArea( 0,   0,   0,  64,  64, NULL                         , 0.0f, NULL, NULL, &buttons[13], (keynum_t)'`', NULL, 0, 0, 0, true);
		VID_TouchscreenArea( 0,  64,   0,  64,  64, "gfx/touch_menu.tga"         , 0.0f, NULL, NULL, &buttons[14], K_ESCAPE, NULL, 0, 0, 0, true);
		if (!VID_ShowingKeyboard())
		{
			// user entered a command, close the console now
			Con_ToggleConsole_f(cmd_local);
		}
		VID_TouchscreenArea( 0,   0,   0,   0,   0, NULL                         , 0.0f, NULL, NULL, &buttons[15], (keynum_t)0, NULL, 0, 0, 0, true);
		VID_TouchscreenArea( 0,   0,   0,   0,   0, NULL                         , 0.0f, NULL, move, &buttons[0], K_MOUSE4, NULL, 0, 0, 0, true);
		VID_TouchscreenArea( 0,   0,   0,   0,   0, NULL                         , 0.0f, NULL, aim,  &buttons[1], K_MOUSE5, NULL, 0, 0, 0, true);
		VID_TouchscreenArea( 0,   0,   0,   0,   0, NULL                         , 0.0f, NULL, click,&buttons[2], K_MOUSE1, NULL, 0, 0, 0, true);
		VID_TouchscreenArea( 0,   0,   0,   0,   0, NULL                         , 0.0f, NULL, NULL, &buttons[3], K_SPACE, NULL, 0, 0, 0, true);
		VID_TouchscreenArea( 0,   0,   0,   0,   0, NULL                         , 0.0f, NULL, NULL, &buttons[4], K_MOUSE2, NULL, 0, 0, 0, true);
		break;
	case key_game:
		VID_TouchscreenArea( 0,   0,   0,  64,  64, NULL                         , 0.0f, NULL, NULL, &buttons[13], (keynum_t)'`', NULL, 0, 0, 0, true);
		VID_TouchscreenArea( 0,  64,   0,  64,  64, "gfx/touch_menu.tga"         , 0.0f, NULL, NULL, &buttons[14], K_ESCAPE, NULL, 0, 0, 0, true);
		VID_TouchscreenArea( 2,   0,-128, 128, 128, "gfx/touch_movebutton.tga"   , 0.0f, NULL, move, &buttons[0], K_MOUSE4, NULL, 0, 0, 0, true);
		VID_TouchscreenArea( 3,-128,-128, 128, 128, "gfx/touch_aimbutton.tga"    , 0.0f, NULL, aim,  &buttons[1], K_MOUSE5, NULL, 0, 0, 0, true);
		VID_TouchscreenArea( 2,   0,-160,  64,  32, "gfx/touch_jumpbutton.tga"   , 0.0f, NULL, NULL, &buttons[3], K_SPACE, NULL, 0, 0, 0, true);
		VID_TouchscreenArea( 3,-128,-160,  64,  32, "gfx/touch_attackbutton.tga" , 0.0f, NULL, NULL, &buttons[2], K_MOUSE1, NULL, 0, 0, 0, true);
		VID_TouchscreenArea( 3, -64,-160,  64,  32, "gfx/touch_attack2button.tga", 0.0f, NULL, NULL, &buttons[4], K_MOUSE2, NULL, 0, 0, 0, true);
		buttons[15] = false;
		break;
	default:
		VID_TouchscreenArea( 0,   0,   0,  64,  64, NULL                         , 0.0f, NULL, NULL, &buttons[13], (keynum_t)'`', NULL, 0, 0, 0, true);
		VID_TouchscreenArea( 0,  64,   0,  64,  64, "gfx/touch_menu.tga"         , 0.0f, NULL, NULL, &buttons[14], K_ESCAPE, NULL, 0, 0, 0, true);
		// in menus, an icon in the corner activates keyboard
		VID_TouchscreenArea( 2,   0, -32,  32,  32, "gfx/touch_keyboard.tga"     , 0.0f, NULL, NULL, &buttons[15], (keynum_t)0, NULL, 0, 0, 0, true);
		if (buttons[15])
			VID_ShowKeyboard(true);
		VID_TouchscreenArea( 0,   0,   0,   0,   0, NULL                         , 0.0f, NULL, move, &buttons[0], K_MOUSE4, NULL, 0, 0, 0, true);
		VID_TouchscreenArea( 0,   0,   0,   0,   0, NULL                         , 0.0f, NULL, aim,  &buttons[1], K_MOUSE5, NULL, 0, 0, 0, true);
		VID_TouchscreenArea(16, -320,-480,640, 960, NULL                         , 0.0f, NULL, click,&buttons[2], K_MOUSE1, NULL, 0, 0, 0, true);
		VID_TouchscreenArea( 0,   0,   0,   0,   0, NULL                         , 0.0f, NULL, NULL, &buttons[3], K_SPACE, NULL, 0, 0, 0, true);
		VID_TouchscreenArea( 0,   0,   0,   0,   0, NULL                         , 0.0f, NULL, NULL, &buttons[4], K_MOUSE2, NULL, 0, 0, 0, true);
		if (buttons[2])
		{
			in_windowmouse_x = x;
			in_windowmouse_y = y;
		}
		break;
	}

	cl.cmd.forwardmove -= move[1] * cl_forwardspeed.value;
	cl.cmd.sidemove += move[0] * cl_sidespeed.value;
	cl.viewangles[0] += aim[1] * cl_pitchspeed.value * cl.realframetime;
	cl.viewangles[1] -= aim[0] * cl_yawspeed.value * cl.realframetime;
}

void IN_Move( void )
{
	static int old_x = 0, old_y = 0;
	static int stuck = 0;
	static keydest_t oldkeydest;
	static qbool oldshowkeyboard;
	int x, y;
	vid_joystate_t joystate;
	keydest_t keydest = (key_consoleactive & KEY_CONSOLEACTIVE_USER) ? key_console : key_dest;

	scr_numtouchscreenareas = 0;

	// Only apply the new keyboard state if the input changes.
	if (keydest != oldkeydest || !!vid_touchscreen_showkeyboard.integer != oldshowkeyboard)
	{
		switch(keydest)
		{
			case key_console: VID_ShowKeyboard(true);break;
			case key_message: VID_ShowKeyboard(true);break;
			default: VID_ShowKeyboard(!!vid_touchscreen_showkeyboard.integer); break;
		}
	}
	oldkeydest = keydest;
	oldshowkeyboard = !!vid_touchscreen_showkeyboard.integer;

	if (vid_touchscreen.integer)
	{
		switch(gamemode)
		{
		case GAME_STEELSTORM:
			IN_Move_TouchScreen_SteelStorm();
			break;
		default:
			IN_Move_TouchScreen_Quake();
			break;
		}
	}
	else
	{
		if (vid_usingmouse)
		{
			if (vid_stick_mouse.integer || !vid_usingmouse_relativeworks)
			{
				// have the mouse stuck in the middle, example use: prevent expose effect of beryl during the game when not using
				// window grabbing. --blub
				int win_half_width = vid.mode.width>>1;
				int win_half_height = vid.mode.height>>1;
	
				// we need 2 frames to initialize the center position
				if(!stuck)
				{
					SDL_WarpMouseInWindow(window, win_half_width, win_half_height);
					SDL_GetMouseState(&x, &y);
					SDL_GetRelativeMouseState(&x, &y);
					++stuck;
				} else {
					SDL_GetRelativeMouseState(&x, &y);
					in_mouse_x = x + old_x;
					in_mouse_y = y + old_y;
					SDL_GetMouseState(&x, &y);
					old_x = x - win_half_width;
					old_y = y - win_half_height;
					SDL_WarpMouseInWindow(window, win_half_width, win_half_height);
				}
			} else {
				SDL_GetRelativeMouseState( &x, &y );
				in_mouse_x = x;
				in_mouse_y = y;
			}
		}

		SDL_GetMouseState(&x, &y);
		in_windowmouse_x = x;
		in_windowmouse_y = y;
	}

	//Con_Printf("Mouse position: in_mouse %f %f in_windowmouse %f %f\n", in_mouse_x, in_mouse_y, in_windowmouse_x, in_windowmouse_y);

	VID_BuildJoyState(&joystate);
	VID_ApplyJoyState(&joystate);
}

/////////////////////
// Message Handling
////

static keynum_t buttonremap[] =
{
	K_MOUSE1,
	K_MOUSE3,
	K_MOUSE2,
	K_MOUSE4,
	K_MOUSE5,
	K_MOUSE6,
	K_MOUSE7,
	K_MOUSE8,
	K_MOUSE9,
	K_MOUSE10,
	K_MOUSE11,
	K_MOUSE12,
	K_MOUSE13,
	K_MOUSE14,
	K_MOUSE15,
	K_MOUSE16,
};

//#define DEBUGSDLEVENTS
void Sys_SDL_HandleEvents(void)
{
	int keycode;
	int i;
	const char *chp;
	qbool isdown;
	Uchar unicode;
	SDL_Event event;

	VID_EnableJoystick(true);

	while( SDL_PollEvent( &event ) )
		loop_start:
		switch( event.type ) {
			case SDL_QUIT:
#ifdef DEBUGSDLEVENTS
				Con_DPrintf("SDL_Event: SDL_QUIT\n");
#endif
				host.state = host_shutdown;
				break;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
#ifdef DEBUGSDLEVENTS
				if (event.type == SDL_KEYDOWN)
					Con_DPrintf("SDL_Event: SDL_KEYDOWN %i\n", event.key.keysym.sym);
				else
					Con_DPrintf("SDL_Event: SDL_KEYUP %i\n", event.key.keysym.sym);
#endif
				keycode = MapKey(event.key.keysym.sym);
				isdown = (event.key.state == SDL_PRESSED);
				unicode = 0;
				if(isdown)
				{
					if(SDL_PollEvent(&event))
					{
						if(event.type == SDL_TEXTINPUT)
						{
							// combine key code from SDL_KEYDOWN event and character
							// from SDL_TEXTINPUT event in a single Key_Event call
#ifdef DEBUGSDLEVENTS
							Con_DPrintf("SDL_Event: SDL_TEXTINPUT - text: %s\n", event.text.text);
#endif
							unicode = u8_getchar_utf8_enabled(event.text.text + (int)u8_bytelen(event.text.text, 0), NULL);
						}
						else
						{
							if (!VID_JoyBlockEmulatedKeys(keycode))
								Key_Event(keycode, 0, isdown);
							goto loop_start;
						}
					}
				}
				if (!VID_JoyBlockEmulatedKeys(keycode))
					Key_Event(keycode, unicode, isdown);
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
#ifdef DEBUGSDLEVENTS
				if (event.type == SDL_MOUSEBUTTONDOWN)
					Con_DPrintf("SDL_Event: SDL_MOUSEBUTTONDOWN\n");
				else
					Con_DPrintf("SDL_Event: SDL_MOUSEBUTTONUP\n");
#endif
				if (!vid_touchscreen.integer)
				if (event.button.button > 0 && event.button.button <= ARRAY_SIZE(buttonremap))
					Key_Event( buttonremap[event.button.button - 1], 0, event.button.state == SDL_PRESSED );
				break;
			case SDL_MOUSEWHEEL:
				// TODO support wheel x direction.
				i = event.wheel.y;
				while (i > 0) {
					--i;
					Key_Event( K_MWHEELUP, 0, true );
					Key_Event( K_MWHEELUP, 0, false );
				}
				while (i < 0) {
					++i;
					Key_Event( K_MWHEELDOWN, 0, true );
					Key_Event( K_MWHEELDOWN, 0, false );
				}
				break;
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
			case SDL_JOYAXISMOTION:
			case SDL_JOYBALLMOTION:
			case SDL_JOYHATMOTION:
#ifdef DEBUGSDLEVENTS
				Con_DPrintf("SDL_Event: SDL_JOY*\n");
#endif
				break;
			case SDL_WINDOWEVENT:
#ifdef DEBUGSDLEVENTS
				Con_DPrintf("SDL_Event: SDL_WINDOWEVENT %i\n", (int)event.window.event);
#endif
				//if (event.window.windowID == window) // how to compare?
				{
					switch(event.window.event)
					{
					case SDL_WINDOWEVENT_SHOWN:
						vid_hidden = false;
						break;
					case  SDL_WINDOWEVENT_HIDDEN:
						vid_hidden = true;
						break;
					case SDL_WINDOWEVENT_EXPOSED:
#ifdef DEBUGSDLEVENTS
						Con_DPrintf("SDL_Event: SDL_WINDOWEVENT_EXPOSED\n");
#endif
						break;
					case SDL_WINDOWEVENT_MOVED:
						vid.xPos = event.window.data1;
						vid.yPos = event.window.data2;
						// Update vid.displayindex (current monitor) as it may have changed
						// SDL_GetWindowDisplayIndex() doesn't work if the window manager moves the fullscreen window, but this works:
						for (i = 0; i < vid_info_displaycount.integer; ++i)
						{
							SDL_Rect displaybounds;
							if (SDL_GetDisplayBounds(i, &displaybounds) < 0)
							{
								Con_Printf(CON_ERROR "Error getting bounds of display %i: \"%s\"\n", i, SDL_GetError());
								return;
							}
							if (vid.xPos >= displaybounds.x && vid.xPos < displaybounds.x + displaybounds.w)
							if (vid.yPos >= displaybounds.y && vid.yPos < displaybounds.y + displaybounds.h)
							{
								vid.mode.display = i;
								break;
							}
						}
						// when the window manager adds/removes the border it's likely to move the SDL window
						// we'll need to correct that to (re)align the xhair with the monitor
						if (vid_wmborder_waiting)
						{
							SDL_GetWindowBordersSize(window, &i, NULL, NULL, NULL);
							if (!i != vid_wmborderless) // border state changed
							{
								SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED_DISPLAY(vid.mode.display), SDL_WINDOWPOS_CENTERED_DISPLAY(vid.mode.display));
								SDL_GetWindowPosition(window, &vid.xPos, &vid.yPos);
								vid_wmborder_waiting = false;
							}
						}
						break;
					case SDL_WINDOWEVENT_RESIZED: // external events only
						if(vid_resizable.integer < 2)
						{
							//vid.width = event.window.data1;
							//vid.height = event.window.data2;
							// get the real framebuffer size in case the platform's screen coordinates are DPI scaled
							SDL_GL_GetDrawableSize(window, &vid.mode.width, &vid.mode.height);
						}
						break;
					case SDL_WINDOWEVENT_SIZE_CHANGED: // internal and external events
						break;
					case SDL_WINDOWEVENT_MINIMIZED:
						break;
					case SDL_WINDOWEVENT_MAXIMIZED:
						break;
					case SDL_WINDOWEVENT_RESTORED:
						break;
					case SDL_WINDOWEVENT_ENTER:
						break;
					case SDL_WINDOWEVENT_LEAVE:
						break;
					case SDL_WINDOWEVENT_FOCUS_GAINED:
						vid_hasfocus = true;
						break;
					case SDL_WINDOWEVENT_FOCUS_LOST:
						vid_hasfocus = false;
						break;
					case SDL_WINDOWEVENT_CLOSE:
						host.state = host_shutdown;
						break;
					case SDL_WINDOWEVENT_TAKE_FOCUS:
						break;
					case SDL_WINDOWEVENT_HIT_TEST:
						break;
					case SDL_WINDOWEVENT_ICCPROF_CHANGED:
						break;
					case SDL_WINDOWEVENT_DISPLAY_CHANGED:
						// this event can't be relied on in fullscreen, see SDL_WINDOWEVENT_MOVED above
						vid.mode.display = event.window.data1;
						break;
					}
				}
				break;
			case SDL_DISPLAYEVENT: // Display hotplugging
				switch (event.display.event)
				{
					case SDL_DISPLAYEVENT_CONNECTED:
						Con_Printf(CON_WARN "Display %i connected: %s\n", event.display.display, SDL_GetDisplayName(event.display.display));
#ifdef __linux__
						Con_Print(CON_WARN "A vid_restart may be necessary!\n");
#endif
						Cvar_SetValueQuick(&vid_info_displaycount, SDL_GetNumVideoDisplays());
						// Ideally we'd call VID_ApplyDisplayMode() to try to switch to the preferred display here,
						// but we may need a vid_restart first, see comments in VID_ApplyDisplayMode().
						break;
					case SDL_DISPLAYEVENT_DISCONNECTED:
						Con_Printf(CON_WARN "Display %i disconnected.\n", event.display.display);
#ifdef __linux__
						Con_Print(CON_WARN "A vid_restart may be necessary!\n");
#endif
						Cvar_SetValueQuick(&vid_info_displaycount, SDL_GetNumVideoDisplays());
						break;
					case SDL_DISPLAYEVENT_ORIENTATION:
						break;
				}
				break;
			case SDL_TEXTEDITING:
#ifdef DEBUGSDLEVENTS
				Con_DPrintf("SDL_Event: SDL_TEXTEDITING - composition = %s, cursor = %d, selection lenght = %d\n", event.edit.text, event.edit.start, event.edit.length);
#endif
				// FIXME!  this is where composition gets supported
				break;
			case SDL_TEXTINPUT:
#ifdef DEBUGSDLEVENTS
				Con_DPrintf("SDL_Event: SDL_TEXTINPUT - text: %s\n", event.text.text);
#endif
				// convert utf8 string to char
				// NOTE: this code is supposed to run even if utf8enable is 0
				chp = event.text.text;
				while (*chp != 0)
				{
					// input the chars one by one (there can be multiple chars when e.g. using an "input method")
					unicode = u8_getchar_utf8_enabled(chp, &chp);
					Key_Event(K_TEXT, unicode, true);
					Key_Event(K_TEXT, unicode, false);
				}
				break;
			case SDL_MOUSEMOTION:
				break;
			case SDL_FINGERDOWN:
#ifdef DEBUGSDLEVENTS
				Con_DPrintf("SDL_FINGERDOWN for finger %i\n", (int)event.tfinger.fingerId);
#endif
				for (i = 0;i < MAXFINGERS-1;i++)
				{
					if (!multitouch[i][0])
					{
						multitouch[i][0] = event.tfinger.fingerId + 1;
						multitouch[i][1] = event.tfinger.x;
						multitouch[i][2] = event.tfinger.y;
						// TODO: use event.tfinger.pressure?
						break;
					}
				}
				if (i == MAXFINGERS-1)
					Con_DPrintf("Too many fingers at once!\n");
				break;
			case SDL_FINGERUP:
#ifdef DEBUGSDLEVENTS
				Con_DPrintf("SDL_FINGERUP for finger %i\n", (int)event.tfinger.fingerId);
#endif
				for (i = 0;i < MAXFINGERS-1;i++)
				{
					if (multitouch[i][0] == event.tfinger.fingerId + 1)
					{
						multitouch[i][0] = 0;
						break;
					}
				}
				if (i == MAXFINGERS-1)
					Con_DPrintf("No SDL_FINGERDOWN event matches this SDL_FINGERMOTION event\n");
				break;
			case SDL_FINGERMOTION:
#ifdef DEBUGSDLEVENTS
				Con_DPrintf("SDL_FINGERMOTION for finger %i\n", (int)event.tfinger.fingerId);
#endif
				for (i = 0;i < MAXFINGERS-1;i++)
				{
					if (multitouch[i][0] == event.tfinger.fingerId + 1)
					{
						multitouch[i][1] = event.tfinger.x;
						multitouch[i][2] = event.tfinger.y;
						break;
					}
				}
				if (i == MAXFINGERS-1)
					Con_DPrintf("No SDL_FINGERDOWN event matches this SDL_FINGERMOTION event\n");
				break;
			default:
#ifdef DEBUGSDLEVENTS
				Con_DPrintf("Received unrecognized SDL_Event type 0x%x\n", event.type);
#endif
				break;
		}

	vid_activewindow = !vid_hidden && vid_hasfocus;

	if (!vid_activewindow || key_consoleactive || scr_loading)
		VID_SetMouse(false, false);
	else if (key_dest == key_menu || key_dest == key_menu_grabbed)
		VID_SetMouse(vid_mouse.integer && !in_client_mouse && !vid_touchscreen.integer, !vid_touchscreen.integer);
	else
		VID_SetMouse(vid_mouse.integer && !cl.csqc_wantsmousemove && cl_prydoncursor.integer <= 0 && (!cls.demoplayback || cl_demo_mousegrab.integer) && !vid_touchscreen.integer, !vid_touchscreen.integer);
}

/////////////////
// Video system
////

void *GL_GetProcAddress(const char *name)
{
	void *p = NULL;
	p = SDL_GL_GetProcAddress(name);
	return p;
}

qbool GL_ExtensionSupported(const char *name)
{
	return SDL_GL_ExtensionSupported(name);
}

/// Applies display settings immediately (no vid_restart required).
static void VID_ApplyDisplayMode(const viddef_mode_t *mode)
{
	uint32_t fullscreenwanted;
	int displaywanted = bound(0, mode->display, vid_info_displaycount.integer - 1);
	SDL_DisplayMode modefinal;

	if (mode->fullscreen)
		fullscreenwanted = mode->desktopfullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : SDL_WINDOW_FULLSCREEN;
	else
		fullscreenwanted = 0;

	// moving to another display or switching to windowed
	if (vid.mode.display != displaywanted // SDL seems unable to move any fullscreen window to another display
	|| !fullscreenwanted)
	{
		if (SDL_SetWindowFullscreen(window, 0) < 0)
		{
			Con_Printf(CON_ERROR "ERROR: can't deactivate fullscreen on display %i because %s\n", vid.mode.display, SDL_GetError());
			return;
		}
		vid.mode.desktopfullscreen = vid.mode.fullscreen = false;
		Con_DPrintf("Fullscreen deactivated on display %i\n", vid.mode.display);
	}

	// switching to windowed
	if (!fullscreenwanted)
	{
		int toppx;

		SDL_SetWindowSize(window, vid.mode.width = mode->width, vid.mode.height = mode->height);
		// resizable and borderless set here cos a separate callback would fail if the cvar is changed when the window is fullscreen
		SDL_SetWindowResizable(window, vid_resizable.integer ? SDL_TRUE : SDL_FALSE);
		SDL_SetWindowBordered(window, (SDL_bool)!vid_borderless.integer);
		SDL_GetWindowBordersSize(window, &toppx, NULL, NULL, NULL);
		vid_wmborderless = !toppx;
		if (vid_borderless.integer != vid_wmborderless) // this is not the state we're looking for
			vid_wmborder_waiting = true;
	}

	// moving to another display or switching to windowed
	if (vid.mode.display != displaywanted || !fullscreenwanted)
	{
//		SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED_DISPLAY(displaywanted), SDL_WINDOWPOS_CENTERED_DISPLAY(displaywanted));
//		SDL_GetWindowPosition(window, &vid.xPos, &vid.yPos);

		/* bones_was_here BUG: after SDL_DISPLAYEVENT hotplug events, on Xorg + NVIDIA,
		 * SDL_WINDOWPOS_CENTERED_DISPLAY(displaywanted) may place the window somewhere completely invisible.
		 * WORKAROUND: manual positioning seems safer: although SDL_GetDisplayBounds() may return outdated values,
		 * SDL_SetWindowPosition() always placed the window somewhere fully visible, even if it wasn't correct,
		 * when tested with SDL 2.26.5.
		 */
		SDL_Rect displaybounds;
		if (SDL_GetDisplayBounds(displaywanted, &displaybounds) < 0)
		{
			Con_Printf(CON_ERROR "Error getting bounds of display %i: \"%s\"\n", displaywanted, SDL_GetError());
			return;
		}
		vid.xPos = displaybounds.x + 0.5 * (displaybounds.w - vid.mode.width);
		vid.yPos = displaybounds.y + 0.5 * (displaybounds.h - vid.mode.height);
		SDL_SetWindowPosition(window, vid.xPos, vid.yPos);

		vid.mode.display = displaywanted;
	}

	// switching to a fullscreen mode
	if (fullscreenwanted)
	{
		if (fullscreenwanted == SDL_WINDOW_FULLSCREEN)
		{
			// determine if a modeset is needed and if the requested resolution is supported
			SDL_DisplayMode modewanted, modecurrent;

			modewanted.w = mode->width;
			modewanted.h = mode->height;
			modewanted.format = mode->bitsperpixel == 16 ? SDL_PIXELFORMAT_RGB565 : SDL_PIXELFORMAT_RGB888;
			modewanted.refresh_rate = mode->refreshrate;
			if (!SDL_GetClosestDisplayMode(displaywanted, &modewanted, &modefinal))
			{
				// SDL_GetError() returns a random unrelated error if this fails (in 2.26.5)
				Con_Printf(CON_ERROR "Error getting closest mode to %ix%i@%ihz for display %i\n", modewanted.w, modewanted.h, modewanted.refresh_rate, vid.mode.display);
				return;
			}
			if (SDL_GetCurrentDisplayMode(displaywanted, &modecurrent) < 0)
			{
				Con_Printf(CON_ERROR "Error getting current mode of display %i: \"%s\"\n", vid.mode.display, SDL_GetError());
				return;
			}
			if (memcmp(&modecurrent, &modefinal, sizeof(modecurrent)) != 0)
			{
				if (mode->width != modefinal.w || mode->height != modefinal.h)
				{
					Con_Printf(CON_WARN "Display %i doesn't support resolution %ix%i\n", vid.mode.display, modewanted.w, modewanted.h);
					return;
				}
				if (SDL_SetWindowDisplayMode(window, &modefinal) < 0)
				{
					Con_Printf(CON_ERROR "Error setting mode %ix%i@%ihz for display %i: \"%s\"\n", modefinal.w, modefinal.h, modefinal.refresh_rate, vid.mode.display, SDL_GetError());
					return;
				}
				// HACK to work around SDL BUG when switching from a lower to a higher res:
				// the display res gets increased but the window size isn't increased
				// (unless we do this first; switching to windowed mode first also works).
				SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
			}
		}

		if (SDL_SetWindowFullscreen(window, fullscreenwanted) < 0)
		{
			Con_Printf(CON_ERROR "ERROR: can't activate fullscreen on display %i because %s\n", vid.mode.display, SDL_GetError());
			return;
		}
		// get the real framebuffer size in case the platform's screen coordinates are DPI scaled
		SDL_GL_GetDrawableSize(window, &vid.mode.width, &vid.mode.height);
		vid.mode.fullscreen = true;
		vid.mode.desktopfullscreen = fullscreenwanted == SDL_WINDOW_FULLSCREEN_DESKTOP;
		Con_DPrintf("Fullscreen activated on display %i\n", vid.mode.display);
	}

	if (!fullscreenwanted || fullscreenwanted == SDL_WINDOW_FULLSCREEN_DESKTOP)
		SDL_GetDesktopDisplayMode(displaywanted, &modefinal);
	else { /* modefinal was set by SDL_GetClosestDisplayMode */ }
	vid.mode.bitsperpixel = SDL_BITSPERPIXEL(modefinal.format);
	vid.mode.refreshrate  = mode->refreshrate && mode->fullscreen && !mode->desktopfullscreen ? modefinal.refresh_rate : 0;
	vid.stencil           = mode->bitsperpixel > 16;
}

static void VID_ApplyDisplayMode_c(cvar_t *var)
{
	viddef_mode_t mode;

	if (!window)
		return;

	// Menu designs aren't suitable for instant hardware modesetting
	// they make players scroll through a list, setting the cvars at each step.
	if (key_dest == key_menu && !key_consoleactive // in menu, console closed
	&& vid_fullscreen.integer && !vid_desktopfullscreen.integer) // modesetting enabled
		return;

	Con_DPrintf("%s: applying %s \"%s\"\n", __func__, var->name, var->string);

	mode.display           = vid_display.integer;
	mode.fullscreen        = vid_fullscreen.integer;
	mode.desktopfullscreen = vid_desktopfullscreen.integer;
	mode.width             = vid_width.integer;
	mode.height            = vid_height.integer;
	mode.bitsperpixel      = vid_bitsperpixel.integer;
	mode.refreshrate       = max(0, vid_refreshrate.integer);
	VID_ApplyDisplayMode(&mode);
}

static void VID_SetVsync_c(cvar_t *var)
{
	int vsyncwanted = cls.timedemo ? 0 : vid_vsync.integer;

	if (!context)
		return;
/*
Can't check first: on Wayland SDL_GL_GetSwapInterval() may initially return 0 when vsync is on.
On Xorg it returns the correct value.
	if (SDL_GL_GetSwapInterval() == vsyncwanted)
		return;
*/

	// __EMSCRIPTEN__ SDL_GL_SetSwapInterval() calls emscripten_set_main_loop_timing()
	if (SDL_GL_SetSwapInterval(vsyncwanted) >= 0)
		Con_DPrintf("Vsync %s\n", vsyncwanted ? "activated" : "deactivated");
	else
		Con_Printf(CON_ERROR "ERROR: can't %s vsync because %s\n", vsyncwanted ? "activate" : "deactivate", SDL_GetError());
}

static void VID_SetHints_c(cvar_t *var)
{
	SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH,     vid_mouse_clickthrough.integer     ? "1" : "0");
	SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, vid_minimize_on_focus_loss.integer ? "1" : "0");
}

void VID_Init (void)
{
	SDL_version version;

#ifndef __IPHONEOS__
#ifdef MACOSX
	Cvar_RegisterVariable(&apple_mouse_noaccel);
#endif
#endif
#ifdef DP_MOBILETOUCH
	Cvar_SetValueQuick(&vid_touchscreen, 1);
#endif
	Cvar_RegisterVariable(&joy_sdl2_trigger_deadzone);

	Cvar_RegisterCallback(&vid_display,                VID_ApplyDisplayMode_c);
	Cvar_RegisterCallback(&vid_fullscreen,             VID_ApplyDisplayMode_c);
	Cvar_RegisterCallback(&vid_desktopfullscreen,      VID_ApplyDisplayMode_c);
	Cvar_RegisterCallback(&vid_width,                  VID_ApplyDisplayMode_c);
	Cvar_RegisterCallback(&vid_height,                 VID_ApplyDisplayMode_c);
	Cvar_RegisterCallback(&vid_refreshrate,            VID_ApplyDisplayMode_c);
	Cvar_RegisterCallback(&vid_resizable,              VID_ApplyDisplayMode_c);
	Cvar_RegisterCallback(&vid_borderless,             VID_ApplyDisplayMode_c);
	Cvar_RegisterCallback(&vid_vsync,                  VID_SetVsync_c);
	Cvar_RegisterCallback(&vid_mouse_clickthrough,     VID_SetHints_c);
	Cvar_RegisterCallback(&vid_minimize_on_focus_loss, VID_SetHints_c);

	// DPI scaling prevents use of the native resolution, causing blurry rendering
	// and/or mouse cursor problems and/or incorrect render area, so we need to opt-out.
	// Must be set before first SDL_INIT_VIDEO. Documented in SDL_hints.h.
#ifdef WIN32
	// make SDL coordinates == hardware pixels
	SDL_SetHint(SDL_HINT_WINDOWS_DPI_SCALING, "0");
	// use best available awareness mode
	SDL_SetHint(SDL_HINT_WINDOWS_DPI_AWARENESS, "permonitorv2");
#endif

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		Sys_Error ("Failed to init SDL video subsystem: %s", SDL_GetError());
	if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0)
		Con_Printf(CON_ERROR "Failed to init SDL joystick subsystem: %s\n", SDL_GetError());

	SDL_GetVersion(&version);
	Con_Printf("Linked against SDL version %d.%d.%d\n"
	           "Using SDL library version %d.%d.%d\n",
	           SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL,
	           version.major, version.minor, version.patch);
}

static int vid_sdljoystickindex = -1;
void VID_EnableJoystick(qbool enable)
{
	int index = joy_enable.integer > 0 ? joy_index.integer : -1;
	int numsdljoysticks;
	qbool success = false;
	int sharedcount = 0;
	int sdlindex = -1;
	sharedcount = VID_Shared_SetJoystick(index);
	if (index >= 0 && index < sharedcount)
		success = true;
	sdlindex = index - sharedcount;

	numsdljoysticks = SDL_NumJoysticks();
	if (sdlindex < 0 || sdlindex >= numsdljoysticks)
		sdlindex = -1;

	// update cvar containing count of XInput joysticks + SDL joysticks
	if (joy_detected.integer != sharedcount + numsdljoysticks)
		Cvar_SetValueQuick(&joy_detected, sharedcount + numsdljoysticks);

	if (vid_sdljoystickindex != sdlindex)
	{
		vid_sdljoystickindex = sdlindex;
		// close SDL joystick if active
		if (vid_sdljoystick)
		{
			SDL_JoystickClose(vid_sdljoystick);
			vid_sdljoystick = NULL;
		}
		if (vid_sdlgamecontroller)
		{
			SDL_GameControllerClose(vid_sdlgamecontroller);
			vid_sdlgamecontroller = NULL;
		}
		if (sdlindex >= 0)
		{
			vid_sdljoystick = SDL_JoystickOpen(sdlindex);
			if (vid_sdljoystick)
			{
				const char *joystickname = SDL_JoystickName(vid_sdljoystick);
				if (SDL_IsGameController(vid_sdljoystickindex))
				{
					vid_sdlgamecontroller = SDL_GameControllerOpen(vid_sdljoystickindex);
					Con_DPrintf("Using SDL GameController mappings for Joystick %i\n", index);
				}
				Con_Printf("Joystick %i opened (SDL_Joystick %i is \"%s\" with %i axes, %i buttons, %i balls)\n", index, sdlindex, joystickname, (int)SDL_JoystickNumAxes(vid_sdljoystick), (int)SDL_JoystickNumButtons(vid_sdljoystick), (int)SDL_JoystickNumBalls(vid_sdljoystick));
			}
			else
			{
				Con_Printf(CON_ERROR "Joystick %i failed (SDL_JoystickOpen(%i) returned: %s)\n", index, sdlindex, SDL_GetError());
				sdlindex = -1;
			}
		}
	}

	if (sdlindex >= 0)
		success = true;

	if (joy_active.integer != (success ? 1 : 0))
		Cvar_SetValueQuick(&joy_active, success ? 1 : 0);
}

#ifdef WIN32
static void AdjustWindowBounds(viddef_mode_t *mode, RECT *rect)
{
	int workWidth;
	int workHeight;
	int titleBarPixels = 2;
	int screenHeight;
	RECT workArea;
	LONG width = mode->width; // vid_width
	LONG height = mode->height; // vid_height

	// adjust width and height for the space occupied by window decorators (title bar, borders)
	rect->top = 0;
	rect->left = 0;
	rect->right = width;
	rect->bottom = height;
	AdjustWindowRectEx(rect, WS_CAPTION|WS_THICKFRAME, false, 0);

	SystemParametersInfo(SPI_GETWORKAREA, 0, &workArea, 0);
	workWidth = workArea.right - workArea.left;
	workHeight = workArea.bottom - workArea.top;

	// SDL forces the window height to be <= screen height - 27px (on Win8.1 - probably intended for the title bar) 
	// If the task bar is docked to the the left screen border and we move the window to negative y,
	// there would be some part of the regular desktop visible on the bottom of the screen.
	screenHeight = GetSystemMetrics(SM_CYSCREEN);
	if (screenHeight == workHeight)
		titleBarPixels = -rect->top;

	//Con_Printf("window mode: %dx%d, workArea: %d/%d-%d/%d (%dx%d), title: %d\n", width, height, workArea.left, workArea.top, workArea.right, workArea.bottom, workArea.right - workArea.left, workArea.bottom - workArea.top, titleBarPixels);

	// if height and width matches the physical or previously adjusted screen height and width, adjust it to available desktop area
	if ((width == GetSystemMetrics(SM_CXSCREEN) || width == workWidth) && (height == screenHeight || height == workHeight - titleBarPixels))
	{
		rect->left = workArea.left;
		mode->width = workWidth;
		rect->top = workArea.top + titleBarPixels;
		mode->height = workHeight - titleBarPixels;
	}
	else 
	{
		rect->left = workArea.left + max(0, (workWidth - width) / 2);
		rect->top = workArea.top + max(0, (workHeight - height) / 2);
	}
}
#endif

static qbool VID_InitModeGL(const viddef_mode_t *mode)
{
	int windowflags = SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL;
	int i;
	// SDL usually knows best
	const char *drivername = NULL;

	// video display selection (multi-monitor)
	Cvar_SetValueQuick(&vid_info_displaycount, SDL_GetNumVideoDisplays());
	vid.mode.display = bound(0, mode->display, vid_info_displaycount.integer - 1);
	vid.xPos = SDL_WINDOWPOS_CENTERED_DISPLAY(vid.mode.display);
	vid.yPos = SDL_WINDOWPOS_CENTERED_DISPLAY(vid.mode.display);
	vid_wmborder_waiting = vid_wmborderless = false;

	if(vid_resizable.integer)
		windowflags |= SDL_WINDOW_RESIZABLE;

#ifndef USE_GLES2
// COMMANDLINEOPTION: SDL GL: -gl_driver <drivername> selects a GL driver library, default is whatever SDL recommends, useful only for 3dfxogl.dll/3dfxvgl.dll or fxmesa or similar, if you don't know what this is for, you don't need it
	i = Sys_CheckParm("-gl_driver");
	if (i && i < sys.argc - 1)
		drivername = sys.argv[i + 1];
	if (SDL_GL_LoadLibrary(drivername) < 0)
	{
		Con_Printf(CON_ERROR "Unable to load GL driver \"%s\": %s\n", drivername, SDL_GetError());
		return false;
	}
#endif

#ifdef DP_MOBILETOUCH
	// mobile platforms are always fullscreen, we'll get the resolution after opening the window
	mode->fullscreen = true;
	// hide the menu with SDL_WINDOW_BORDERLESS
	windowflags |= SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS;
#endif

	// SDL_CreateWindow() supports only width and height modesetting,
	// so initially we use desktopfullscreen and perform a modeset later if necessary,
	// this way we do only one modeset to apply the full config.
	if (mode->fullscreen)
	{
		windowflags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		vid.mode.fullscreen = vid.mode.desktopfullscreen = true;
	}
	else
	{
		if (vid_borderless.integer)
			windowflags |= SDL_WINDOW_BORDERLESS;
		else
			vid_wmborder_waiting = true; // waiting for border to be added
#ifdef WIN32
		if (!vid_ignore_taskbar.integer)
		{
			RECT rect;
			AdjustWindowBounds((viddef_mode_t *)mode, &rect);
			vid.xPos = rect.left;
			vid.xPos = rect.top;
			vid_wmborder_waiting = false;
		}
#endif
		vid.mode.fullscreen = vid.mode.desktopfullscreen = false;
	}

	VID_SetHints_c(NULL);

	SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute (SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute (SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute (SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute (SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute (SDL_GL_STENCIL_SIZE, 8);
	if (mode->stereobuffer)
	{
		SDL_GL_SetAttribute (SDL_GL_STEREO, 1);
		vid.mode.stereobuffer = true;
	}
	if (mode->samples > 1)
	{
		SDL_GL_SetAttribute (SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute (SDL_GL_MULTISAMPLESAMPLES, mode->samples);
	}

#ifdef USE_GLES2
	SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute (SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute (SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
	SDL_GL_SetAttribute (SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	/* Requesting a Core profile and 3.2 minimum is mandatory on macOS and older Mesa drivers.
	 * It works fine on other drivers too except NVIDIA, see HACK below.
	 */
#endif

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, (gl_debug.integer > 0 ? SDL_GL_CONTEXT_DEBUG_FLAG : 0));

	window = SDL_CreateWindow(gamename, vid.xPos, vid.yPos, mode->width, mode->height, windowflags);
	if (window == NULL)
	{
		Con_Printf(CON_ERROR "Failed to set video mode to %ix%i: %s\n", mode->width, mode->height, SDL_GetError());
		VID_Shutdown();
		return false;
	}

	context = SDL_GL_CreateContext(window);
	if (context == NULL)
		Sys_Error("Failed to initialize OpenGL context: %s\n", SDL_GetError());

	GL_InitFunctions();

#if !defined(USE_GLES2) && !defined(MACOSX)
	// NVIDIA hates the Core profile and limits the version to the minimum we specified.
	// HACK: to detect NVIDIA we first need a context, fortunately replacing it takes a few milliseconds
	gl_vendor = (const char *)qglGetString(GL_VENDOR);
	if (strncmp(gl_vendor, "NVIDIA", 6) == 0)
	{
		Con_DPrint("The Way It's Meant To Be Played: replacing OpenGL Core profile with Compatibility profile...\n");
		SDL_GL_DeleteContext(context);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
		context = SDL_GL_CreateContext(window);
		if (context == NULL)
			Sys_Error("Failed to initialize OpenGL context: %s\n", SDL_GetError());
	}
#endif

	// apply vid_vsync
	Cvar_Callback(&vid_vsync);

	vid_hidden = false;
	vid_activewindow = true;
	vid_hasfocus = true;
	vid_usingmouse = false;
	vid_usinghidecursor = false;

	// clear to black (loading plaque will be seen over this)
	GL_Clear(GL_COLOR_BUFFER_BIT, NULL, 1.0f, 0);
	VID_Finish(); // checks vid_hidden

	GL_Setup();

	// VorteX: set other info
	Cvar_SetQuick(&gl_info_vendor, gl_vendor);
	Cvar_SetQuick(&gl_info_renderer, gl_renderer);
	Cvar_SetQuick(&gl_info_version, gl_version);
	Cvar_SetQuick(&gl_info_driver, drivername ? drivername : "");

	for (i = 0; i < vid_info_displaycount.integer; ++i)
		Con_Printf("Display %i: %s\n", i, SDL_GetDisplayName(i));

	// Perform any hardware modesetting and update vid.mode
	// if modesetting fails desktopfullscreen continues to be used (see above).
	VID_ApplyDisplayMode(mode);

	return true;
}

qbool VID_InitMode(const viddef_mode_t *mode)
{
	// GAME_STEELSTORM specific
	steelstorm_showing_map = Cvar_FindVar(&cvars_all, "steelstorm_showing_map", ~0);
	steelstorm_showing_mousecursor = Cvar_FindVar(&cvars_all, "steelstorm_showing_mousecursor", ~0);

	if (!SDL_WasInit(SDL_INIT_VIDEO) && SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
		Sys_Error ("Failed to init SDL video subsystem: %s", SDL_GetError());

	Cvar_SetValueQuick(&vid_touchscreen_supportshowkeyboard, SDL_HasScreenKeyboardSupport() ? 1 : 0);
	return VID_InitModeGL(mode);
}

void VID_Shutdown (void)
{
	VID_EnableJoystick(false);
	VID_SetMouse(false, false);

	SDL_GL_DeleteContext(context);
	context = NULL;
	SDL_DestroyWindow(window);
	window = NULL;

	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void VID_Finish (void)
{
	VID_UpdateGamma();

	if (!vid_hidden)
	{
		switch(vid.renderpath)
		{
		case RENDERPATH_GL32:
		case RENDERPATH_GLES2:
			CHECKGLERROR
			if (r_speeds.integer == 2 || gl_finish.integer)
				GL_Finish();
			SDL_GL_SwapWindow(window);
			break;
		}
	}
}

vid_mode_t VID_GetDesktopMode(void)
{
	SDL_DisplayMode mode;
	int bpp;
	Uint32 rmask, gmask, bmask, amask;
	vid_mode_t desktop_mode;

	SDL_GetDesktopDisplayMode(vid.mode.display, &mode);
	SDL_PixelFormatEnumToMasks(mode.format, &bpp, &rmask, &gmask, &bmask, &amask);
	desktop_mode.width = mode.w;
	desktop_mode.height = mode.h;
	desktop_mode.bpp = bpp;
	desktop_mode.refreshrate = mode.refresh_rate;
	desktop_mode.pixelheight_num = 1;
	desktop_mode.pixelheight_denom = 1; // SDL does not provide this
	return desktop_mode;
}

size_t VID_ListModes(vid_mode_t *modes, size_t maxcount)
{
	size_t k = 0;
	int modenum;
	int nummodes = SDL_GetNumDisplayModes(vid.mode.display);
	SDL_DisplayMode mode;
	for (modenum = 0;modenum < nummodes;modenum++)
	{
		if (k >= maxcount)
			break;
		if (SDL_GetDisplayMode(vid.mode.display, modenum, &mode))
			continue;
		modes[k].width = mode.w;
		modes[k].height = mode.h;
		modes[k].bpp = SDL_BITSPERPIXEL(mode.format);
		modes[k].refreshrate = mode.refresh_rate;
		modes[k].pixelheight_num = 1;
		modes[k].pixelheight_denom = 1; // SDL does not provide this
		Con_DPrintf("Display %i mode %i: %ix%i %ibpp %ihz\n", vid.mode.display, modenum, modes[k].width, modes[k].height, modes[k].bpp, modes[k].refreshrate);
		k++;
	}
	return k;
}
