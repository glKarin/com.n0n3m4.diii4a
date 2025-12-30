//
//	ID Engine
//	ID_IN.c - Input Manager
//	v1.0d1
//	By Jason Blochowiak
//

//
//	This module handles dealing with the various input devices
//
//	Depends on: Memory Mgr (for demo recording), Sound Mgr (for timing stuff),
//				User Mgr (for command line parms)
//
//	Globals:
//		LastScan - The keyboard scan code of the last key pressed
//		LastASCII - The ASCII value of the last key pressed
//	DEBUG - there are more globals
//


#include "wl_def.h"
#include "c_cvars.h"
#include "id_sd.h"
#include "id_in.h"
#include "id_vl.h"
#include "id_vh.h"
#include "config.h"
#include "wl_play.h"


#if !SDL_VERSION_ATLEAST(1,3,0)
#define SDLK_KP_0 SDLK_KP0
#define SDLK_KP_1 SDLK_KP1
#define SDLK_KP_2 SDLK_KP2
#define SDLK_KP_3 SDLK_KP3
#define SDLK_KP_4 SDLK_KP4
#define SDLK_KP_5 SDLK_KP5
#define SDLK_KP_6 SDLK_KP6
#define SDLK_KP_7 SDLK_KP7
#define SDLK_KP_8 SDLK_KP8
#define SDLK_KP_9 SDLK_KP9
#define SDLK_SCROLLLOCK SDLK_SCROLLOCK
#define SDL_NUM_SCANCODES SDLK_LAST
typedef SDLMod SDL_Keymod;

inline void SDL_SetRelativeMouseMode(bool enabled)
{
	SDL_WM_GrabInput(enabled ? SDL_GRAB_ON : SDL_GRAB_OFF);
}
inline void SDL_WarpMouseInWindow(struct SDL_Window* window, int x, int y)
{
	SDL_WarpMouse(x, y);
}
#endif

#ifdef _WIN32
// SDL doesn't track the real state of the numlock or capslock key, so we need
// to query the system. It's probably like this since Linux and OS X don't
// appear to have nice APIs for this.
void I_CheckKeyMods();
#endif

/*
=============================================================================

					GLOBAL VARIABLES

=============================================================================
*/

#ifdef _DIII4A //karin: pull events
extern void Android_PollInput(void);
#endif

//
// configuration variables
//
bool MousePresent;
bool MouseWheel[4];

// 	Global variables
bool Keyboard[SDL_NUM_SCANCODES];
unsigned short Paused;
char LastASCII;
ScanCode LastScan;

static KeyboardDef KbdDefs = {
	sc_Control,             // button0
	sc_Alt,                 // button1
	sc_Home,                // upleft
	sc_UpArrow,             // up
	sc_PgUp,                // upright
	sc_LeftArrow,           // left
	sc_RightArrow,          // right
	sc_End,                 // downleft
	sc_DownArrow,           // down
	sc_PgDn                 // downright
};

static SDL_Joystick *Joystick;
#if SDL_VERSION_ATLEAST(2,0,0)
static SDL_GameController *GameController;
// Flip the right stick axes to match usual mapping of Joystick API.
static SDL_GameControllerAxis GameControllerAxisMap[SDL_CONTROLLER_AXIS_MAX] = {
	SDL_CONTROLLER_AXIS_LEFTX, SDL_CONTROLLER_AXIS_LEFTY, // X, Y
	SDL_CONTROLLER_AXIS_RIGHTY, SDL_CONTROLLER_AXIS_RIGHTX, // Z, R
	SDL_CONTROLLER_AXIS_TRIGGERLEFT, SDL_CONTROLLER_AXIS_TRIGGERRIGHT
};
#endif
JoystickSens *JoySensitivity;
int JoyNumButtons;
int JoyNumAxes;
static int JoyNumHats;

static bool GrabInput = false;

/*
=============================================================================

					LOCAL VARIABLES

=============================================================================
*/
static	bool		IN_Started;

static	Direction	DirTable[] =		// Quick lookup for total direction
{
	dir_NorthWest,	dir_North,	dir_NorthEast,
	dir_West,		dir_None,	dir_East,
	dir_SouthWest,	dir_South,	dir_SouthEast
};


///////////////////////////////////////////////////////////////////////////
//
//	INL_GetMouseButtons() - Gets the status of the mouse buttons from the
//		mouse driver
//
///////////////////////////////////////////////////////////////////////////
static int
INL_GetMouseButtons(void)
{
	int buttons = SDL_GetMouseState(NULL, NULL);
	int middlePressed = buttons & SDL_BUTTON(SDL_BUTTON_MIDDLE);
	int rightPressed = buttons & SDL_BUTTON(SDL_BUTTON_RIGHT);
	buttons &= ~(SDL_BUTTON(SDL_BUTTON_MIDDLE) | SDL_BUTTON(SDL_BUTTON_RIGHT));
	if(middlePressed) buttons |= 1 << 2;
	if(rightPressed) buttons |= 1 << 1;

	return buttons;
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_GetJoyDelta() - Returns the relative movement of the specified
//		joystick (from +/-127)
//
///////////////////////////////////////////////////////////////////////////
void IN_GetJoyDelta(int *dx,int *dy)
{
	if(!IN_JoyPresent())
	{
		*dx = *dy = 0;
		return;
	}

	int x, y;
#if SDL_VERSION_ATLEAST(2,0,0)
	if(GameController)
	{
		SDL_GameControllerUpdate();
		x = SDL_GameControllerGetAxis(GameController, SDL_CONTROLLER_AXIS_LEFTX) >> 8;
		y = SDL_GameControllerGetAxis(GameController, SDL_CONTROLLER_AXIS_LEFTY) >> 8;

		if(SDL_GameControllerGetButton(GameController, SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
			x += 127;
		else if(SDL_GameControllerGetButton(GameController, SDL_CONTROLLER_BUTTON_DPAD_LEFT))
			x -= 127;
		if(SDL_GameControllerGetButton(GameController, SDL_CONTROLLER_BUTTON_DPAD_DOWN))
			y += 127;
		else if(SDL_GameControllerGetButton(GameController, SDL_CONTROLLER_BUTTON_DPAD_UP))
			y -= 127;
	}
	else
#endif
	{
		SDL_JoystickUpdate();
		x = SDL_JoystickGetAxis(Joystick, 0) >> 8;
		y = SDL_JoystickGetAxis(Joystick, 1) >> 8;

		if(param_joystickhat != -1)
		{
			uint8_t hatState = SDL_JoystickGetHat(Joystick, param_joystickhat);
			if(hatState & SDL_HAT_RIGHT)
				x += 127;
			else if(hatState & SDL_HAT_LEFT)
				x -= 127;
			if(hatState & SDL_HAT_DOWN)
				y += 127;
			else if(hatState & SDL_HAT_UP)
				y -= 127;
		}
	}

	if(x < -128) x = -128;
	else if(x > 127) x = 127;

	if(y < -128) y = -128;
	else if(y > 127) y = 127;

	*dx = x;
	*dy = y;
}

int IN_GetJoyAxis(int axis)
{
#if SDL_VERSION_ATLEAST(2,0,0)
	if(GameController)
		return SDL_GameControllerGetAxis(GameController, GameControllerAxisMap[axis]);
#endif
	return SDL_JoystickGetAxis(Joystick, axis);
}

/*
===================
=
= IN_JoyButtons
=
===================
*/

int IN_JoyButtons()
{
#if SDL_VERSION_ATLEAST(2,0,0)
	if(GameController)
	{
		SDL_GameControllerUpdate();

		int res = 0;
		for(int i = 0; i < JoyNumButtons; ++i)
		{
			if(SDL_GameControllerGetButton(GameController, (SDL_GameControllerButton)i))
			{
				// Attempt to allow controllers using the game controller API
				// to enter the menu.
				if(i == SDL_CONTROLLER_BUTTON_START)
					control[ConsolePlayer].buttonstate[bt_esc] = true;
				else
					res |= 1<<i;
			}
		}
		return res;
	}
#endif

	if(!Joystick) return 0;

	SDL_JoystickUpdate();

	int res = 0;
	int i;
	for(i = 0; i < JoyNumButtons && i < 32; i++)
		res |= SDL_JoystickGetButton(Joystick, i) << i;

	// Need four buttons for hat
	if(i < 28 && param_joystickhat != -1)
	{
		uint8_t hatState = SDL_JoystickGetHat(Joystick, param_joystickhat);
		if(hatState & SDL_HAT_UP)
			res |= 0x1 << i;
		else if(hatState & SDL_HAT_DOWN)
			res |= 0x4 << i;
		if(hatState & SDL_HAT_RIGHT)
			res |= 0x2 << i;
		else if(hatState & SDL_HAT_LEFT)
			res |= 0x8 << i;
	}
	return res;
}

int IN_JoyAxes()
{
#if SDL_VERSION_ATLEAST(2,0,0)
	if(GameController)
	{
		SDL_GameControllerUpdate();
		int res = 0;
		for(int i = 0; i < JoyNumAxes; ++i)
		{
			SWORD pos = SDL_GameControllerGetAxis(GameController, (SDL_GameControllerAxis)GameControllerAxisMap[i]);
			if(pos <= -0x1000)
				res |= 1 << (i*2);
			else if(pos >= 0x1000)
				res |= 1 << (i*2+1);
		}
		return res;
	}
#endif
	if(!Joystick) return 0;

	SDL_JoystickUpdate();

	int res = 0;
	for(int i = 0; i < JoyNumAxes && i < 16; ++i)
	{
		SWORD pos = SDL_JoystickGetAxis(Joystick, i);
		if(pos <= -0x1000)
			res |= 1 << (i*2);
		else if(pos >= 0x1000)
			res |= 1 << (i*2+1);
	}
	return res;
}

bool IN_JoyPresent()
{
	return Joystick != NULL
#if SDL_VERSION_ATLEAST(2,0,0)
		|| GameController != NULL
#endif
	;
}

#ifdef __ANDROID__
static bool ShadowKey = false;
bool ShadowingEnabled = false;
extern "C" int hasHardwareKeyboard();
#endif

static void processEvent(SDL_Event *event)
{
	switch (event->type)
	{
		// exit if the window is closed
		case SDL_QUIT:
			Quit();

		// ASCII (Unicode) text entry for saves and stuff like that.
#if SDL_VERSION_ATLEAST(1,3,0)
		case SDL_TEXTINPUT:
		{
			LastASCII = event->text.text[0];
			break;
		}
#endif

		// check for keypresses
		case SDL_KEYDOWN:
		{
			if(event->key.keysym.sym==SDLK_SCROLLLOCK)
			{
				GrabInput = !GrabInput;
				SDL_SetRelativeMouseMode(GrabInput ? SDL_TRUE : SDL_FALSE);
				return;
			}

#if SDL_VERSION_ATLEAST(1,3,0)
			LastScan = event->key.keysym.scancode;

			// Android back button should be treated as escape for now
			if(LastScan == SDL_SCANCODE_AC_BACK)
				LastScan = SDL_SCANCODE_ESCAPE;
#else
			LastScan = event->key.keysym.sym;
#endif
			SDL_Keymod mod = SDL_GetModState();
			if(Keyboard[sc_Alt])
			{
				if(LastScan==SDLx_SCANCODE(F4))
					Quit();
			}

			if(LastScan == SDLx_SCANCODE(KP_ENTER)) LastScan = SDLx_SCANCODE(RETURN);
			else if(LastScan == SDLx_SCANCODE(RSHIFT)) LastScan = SDLx_SCANCODE(LSHIFT);
			else if(LastScan == SDLx_SCANCODE(RALT)) LastScan = SDLx_SCANCODE(LALT);
			else if(LastScan == SDLx_SCANCODE(RCTRL)) LastScan = SDLx_SCANCODE(LCTRL);
			else
			{
				if((mod & KMOD_NUM) == 0)
				{
					switch(LastScan)
					{
						case SDLx_SCANCODE(KP_2): LastScan = SDLx_SCANCODE(DOWN); break;
						case SDLx_SCANCODE(KP_4): LastScan = SDLx_SCANCODE(LEFT); break;
						case SDLx_SCANCODE(KP_6): LastScan = SDLx_SCANCODE(RIGHT); break;
						case SDLx_SCANCODE(KP_8): LastScan = SDLx_SCANCODE(UP); break;
					}
				}
			}

#if !SDL_VERSION_ATLEAST(1,3,0)
			static const byte ASCIINames[] = // Unshifted ASCII for scan codes       // TODO: keypad
			{
			//	 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
				0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,8  ,9  ,0  ,0  ,0  ,13 ,0  ,0  ,	// 0
				0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,27 ,0  ,0  ,0  ,	// 1
				' ',0  ,0  ,0  ,0  ,0  ,0  ,39 ,0  ,0  ,'*','+',',','-','.','/',	// 2
				'0','1','2','3','4','5','6','7','8','9',0  ,';',0  ,'=',0  ,0  ,	// 3
				'`','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',	// 4
				'p','q','r','s','t','u','v','w','x','y','z','[',92 ,']',0  ,0  ,	// 5
				0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,	// 6
				0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0		// 7
			};
			static const byte ShiftNames[] = // Shifted ASCII for scan codes
			{
			//	 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
				0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,8  ,9  ,0  ,0  ,0  ,13 ,0  ,0  ,	// 0
				0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,27 ,0  ,0  ,0  ,	// 1
				' ',0  ,0  ,0  ,0  ,0  ,0  ,34 ,0  ,0  ,'*','+','<','_','>','?',	// 2
				')','!','@','#','$','%','^','&','*','(',0  ,':',0  ,'+',0  ,0  ,	// 3
				'~','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O',	// 4
				'P','Q','R','S','T','U','V','W','X','Y','Z','{','|','}',0  ,0  ,	// 5
				0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,	// 6
				0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0		// 7
			};

			int sym = event->key.keysym.sym;
			if(sym >= 'a' && sym <= 'z')
				sym -= 32;  // convert to uppercase

			if(mod & KMOD_SHIFT)
			{
				if((unsigned)sym < lengthof(ShiftNames) && ShiftNames[sym])
					LastASCII = ShiftNames[sym];
			}
			else
			{
				if((unsigned)sym < lengthof(ASCIINames) && ASCIINames[sym])
					LastASCII = ASCIINames[sym];
			}
			if(LastASCII && sym >= 'A' && sym <= 'Z' && (mod & KMOD_CAPS))
				LastASCII ^= 0x20;
#endif

			if(LastScan<SDL_NUM_SCANCODES)
				Keyboard[LastScan] = 1;
			if(LastScan == SDLx_SCANCODE(PAUSE))
				Paused |= 1;
			break;
		}

		case SDL_KEYUP:
		{
#if SDL_VERSION_ATLEAST(1,3,0)
			int key = event->key.keysym.scancode;
#else
			int key = event->key.keysym.sym;
#endif
			if(key == SDLx_SCANCODE(KP_ENTER)) key = SDLx_SCANCODE(RETURN);
			else if(key == SDLx_SCANCODE(RSHIFT)) key = SDLx_SCANCODE(LSHIFT);
			else if(key == SDLx_SCANCODE(RALT)) key = SDLx_SCANCODE(LALT);
			else if(key == SDLx_SCANCODE(RCTRL)) key = SDLx_SCANCODE(LCTRL);
			else
			{
				if((SDL_GetModState() & KMOD_NUM) == 0)
				{
					switch(key)
					{
						case SDLx_SCANCODE(KP_2): key = SDLx_SCANCODE(DOWN); break;
						case SDLx_SCANCODE(KP_4): key = SDLx_SCANCODE(LEFT); break;
						case SDLx_SCANCODE(KP_6): key = SDLx_SCANCODE(RIGHT); break;
						case SDLx_SCANCODE(KP_8): key = SDLx_SCANCODE(UP); break;
					}
				}
			}

#if !defined(_DIII4A) //karin: raw input
#ifdef __ANDROID__
			if(ShadowKey && LastScan == event->key.keysym.scancode)
			{
				ShadowKey = false;
				break;
			}
#endif
#endif

			if(key<SDL_NUM_SCANCODES)
				Keyboard[key] = 0;
			break;
		}

#if SDL_VERSION_ATLEAST(2,0,0)
		case SDL_MOUSEWHEEL:
		{
			if(event->wheel.x > 0)
				MouseWheel[di_west] = true;
			else if(event->wheel.x < 0)
				MouseWheel[di_east] = true;
			if(event->wheel.y > 0)
				MouseWheel[di_north] = true;
			else if(event->wheel.y < 0)
				MouseWheel[di_south] = true;
			break;
		}
#else
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
		{
			if(event->button.button == SDL_BUTTON_WHEELUP)
				MouseWheel[di_north] = true;
			if(event->button.button == SDL_BUTTON_WHEELDOWN)
				MouseWheel[di_south] = true;
			break;
		}
#endif

#if !SDL_VERSION_ATLEAST(1,3,0)
		case SDL_ACTIVEEVENT:
		{
			if (!fullscreen && forcegrabmouse && (event->active.state & SDL_APPINPUTFOCUS || event->active.state & SDL_APPACTIVE))
			{
					// Release the mouse if we lose input focus, grab it again
					// when we gain input focus.
				if (event->active.gain == 1)
				{
					IN_GrabMouse();
				}
				else
				{
					IN_ReleaseMouse();
				}
			}
			break;
		}
#else
#ifdef _WIN32
		case SDL_WINDOWEVENT:
			if (event->window.event == SDL_WINDOWEVENT_FOCUS_GAINED)
				I_CheckKeyMods();
			break;
#endif
#endif
	}
}

void IN_WaitAndProcessEvents()
{
	SDL_Event event;
	if(!SDL_WaitEvent(&event)) return;
	do
	{
		processEvent(&event);
	}
	while(SDL_PollEvent(&event));
}

void IN_ProcessEvents()
{
	SDL_Event event;

#if !defined(_DIII4A) //karin: raw input
#ifdef __ANDROID__
	if(ShadowingEnabled)
	{
		if(!ShadowKey)
			Keyboard[LastScan] = 0;
		ShadowKey = true;
	}
#endif
#else
	Android_PollInput();
#endif

	while (SDL_PollEvent(&event))
	{
		processEvent(&event);
	}
}


///////////////////////////////////////////////////////////////////////////
//
//	IN_Startup() - Starts up the Input Mgr
//
///////////////////////////////////////////////////////////////////////////
void
IN_Startup(void)
{
	if (IN_Started)
		return;

#ifdef __ANDROID__
	ShadowingEnabled = !hasHardwareKeyboard();
#endif

#ifdef _WIN32
	I_CheckKeyMods();
#else
	// Turn numlock on by default since that's probably what most people want.
	SDL_SetModState(SDL_Keymod(SDL_GetModState()|KMOD_NUM));
#endif

	IN_ClearKeysDown();

	if(SDL_InitSubSystem(SDL_INIT_JOYSTICK) == 0 &&
		param_joystickindex >= 0 && param_joystickindex < SDL_NumJoysticks())
	{
#if SDL_VERSION_ATLEAST(2,0,0)
		if(SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == 0 && SDL_IsGameController(param_joystickindex))
		{
			GameController = SDL_GameControllerOpen(param_joystickindex);
			if(GameController)
			{
				Printf("Using game controller: %s\n", SDL_GameControllerName(GameController));
				SDL_GameControllerEventState(SDL_IGNORE);
				JoyNumButtons = SDL_CONTROLLER_BUTTON_MAX;
				JoyNumAxes = SDL_CONTROLLER_AXIS_MAX;
				JoyNumHats = 0;

				JoySensitivity = new JoystickSens[JoyNumAxes];
			}
		}
		else
#endif
		{
			Joystick = SDL_JoystickOpen(param_joystickindex);
			if(Joystick)
			{
				JoyNumButtons = SDL_JoystickNumButtons(Joystick);
				if(JoyNumButtons > 32) JoyNumButtons = 32;      // only up to 32 buttons are supported
				JoyNumAxes = SDL_JoystickNumAxes(Joystick);
				JoyNumHats = SDL_JoystickNumHats(Joystick);
				if(param_joystickhat >= JoyNumHats)
					I_FatalError("The joystickhat param must be between 0 and %i!", JoyNumHats - 1);
				else if(param_joystickhat < 0 && JoyNumHats > 0) // Default to hat 0
					param_joystickhat = 0;

				JoySensitivity = new JoystickSens[JoyNumAxes];
			}
		}

		if(JoySensitivity)
		{
			for(int i = 0;i < JoyNumAxes;++i)
			{
				FString settingName;
				settingName.Format("JoyAxis%dSensitivity", i);
				config.CreateSetting(settingName, 10);
				JoySensitivity[i].sensitivity = config.GetSetting(settingName)->GetInteger();
				settingName.Format("JoyAxis%dDeadzone", i);
				config.CreateSetting(settingName, 2);
				JoySensitivity[i].deadzone = config.GetSetting(settingName)->GetInteger();
			}
		}
	}

	SDL_EventState(SDL_MOUSEMOTION, SDL_IGNORE);

	IN_GrabMouse();

	// I didn't find a way to ask libSDL whether a mouse is present, yet...
	MousePresent = true;

#ifdef _DIII4A //karin: use mouse look default
	mouselook = true;
#endif

	IN_Started = true;
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_Shutdown() - Shuts down the Input Mgr
//
///////////////////////////////////////////////////////////////////////////
void
IN_Shutdown(void)
{
	if (!IN_Started)
		return;

	if(JoySensitivity)
	{
		for(int i = 0;i < JoyNumAxes;++i)
		{
			FString settingName;
			settingName.Format("JoyAxis%dSensitivity", i);
			config.GetSetting(settingName)->SetValue(JoySensitivity[i].sensitivity);
			settingName.Format("JoyAxis%dDeadzone", i);
			config.GetSetting(settingName)->SetValue(JoySensitivity[i].deadzone);
		}
		delete[] JoySensitivity;
	}

	if(Joystick)
		SDL_JoystickClose(Joystick);
#if SDL_VERSION_ATLEAST(2,0,0)
	if(GameController)
		SDL_GameControllerClose(GameController);
#endif

	IN_Started = false;
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_ClearKeysDown() - Clears the keyboard array
//
///////////////////////////////////////////////////////////////////////////
void
IN_ClearKeysDown(void)
{
	LastScan = sc_None;
	LastASCII = key_None;
	memset ((void *) Keyboard,0,sizeof(Keyboard));

	// Clear the wheel too since although we want to be able to acknowledge it
	// separately since there's no key up state, we still generally think of it
	// as a key.
	IN_ClearWheel();
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_ClearKeysDown() - Clears only mouse wheel state
//
///////////////////////////////////////////////////////////////////////////
void
IN_ClearWheel()
{
	memset ((void *) MouseWheel,0,sizeof(MouseWheel));
}


///////////////////////////////////////////////////////////////////////////
//
//	IN_ReadControl() - Reads the device associated with the specified
//		player and fills in the control info struct
//
///////////////////////////////////////////////////////////////////////////
void
IN_ReadControl(int player,ControlInfo *info)
{
	word		buttons;
	int			dx,dy;
	Motion		mx,my;

	dx = dy = 0;
	mx = my = motion_None;
	buttons = 0;

	IN_ProcessEvents();

	if (Keyboard[KbdDefs.upleft])
		mx = motion_Left,my = motion_Up;
	else if (Keyboard[KbdDefs.upright])
		mx = motion_Right,my = motion_Up;
	else if (Keyboard[KbdDefs.downleft])
		mx = motion_Left,my = motion_Down;
	else if (Keyboard[KbdDefs.downright])
		mx = motion_Right,my = motion_Down;

	if (Keyboard[KbdDefs.up])
		my = motion_Up;
	else if (Keyboard[KbdDefs.down])
		my = motion_Down;

	if (Keyboard[KbdDefs.left])
		mx = motion_Left;
	else if (Keyboard[KbdDefs.right])
		mx = motion_Right;

	if (Keyboard[KbdDefs.button0])
		buttons += 1 << 0;
	if (Keyboard[KbdDefs.button1])
		buttons += 1 << 1;

	dx = mx * 127;
	dy = my * 127;

	info->x = dx;
	info->xaxis = mx;
	info->y = dy;
	info->yaxis = my;
	info->button0 = (buttons & (1 << 0)) != 0;
	info->button1 = (buttons & (1 << 1)) != 0;
	info->button2 = (buttons & (1 << 2)) != 0;
	info->button3 = (buttons & (1 << 3)) != 0;
	info->dir = DirTable[((my + 1) * 3) + (mx + 1)];
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_WaitForKey() - Waits for a scan code, then clears LastScan and
//		returns the scan code
//
///////////////////////////////////////////////////////////////////////////
ScanCode
IN_WaitForKey(void)
{
	ScanCode	result;

	while ((result = LastScan)==0)
		IN_WaitAndProcessEvents();
	LastScan = 0;
	return(result);
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_WaitForASCII() - Waits for an ASCII char, then clears LastASCII and
//		returns the ASCII value
//
///////////////////////////////////////////////////////////////////////////
char
IN_WaitForASCII(void)
{
	char		result;

	while ((result = LastASCII)==0)
		IN_WaitAndProcessEvents();
	LastASCII = '\0';
	return(result);
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_Ack() - waits for a button or key press.  If a button is down, upon
// calling, it must be released for it to be recognized
//
///////////////////////////////////////////////////////////////////////////

bool	btnstate[NUMBUTTONS];

void IN_StartAck(void)
{
	IN_ProcessEvents();
//
// get initial state of everything
//
	IN_ClearKeysDown();
	memset(btnstate, 0, sizeof(btnstate));

	int buttons = IN_JoyButtons() << 4;

	if(MousePresent)
		buttons |= IN_MouseButtons();

	for(int i = 0; i < NUMBUTTONS; i++, buttons >>= 1)
		if(buttons & 1)
			btnstate[i] = true;
}


bool IN_CheckAck (void)
{
	IN_ProcessEvents();
//
// see if something has been pressed
//
	if(LastScan)
		return true;

	int buttons = IN_JoyButtons() << 4;

	if(MousePresent)
		buttons |= IN_MouseButtons();

	for(int i = 0; i < NUMBUTTONS; i++, buttons >>= 1)
	{
		if(buttons & 1)
		{
			if(!btnstate[i])
			{
				// Wait until button has been released
				do
				{
					IN_WaitAndProcessEvents();
					buttons = IN_JoyButtons() << 4;

					if(MousePresent)
						buttons |= IN_MouseButtons();
				}
				while(buttons & (1 << i));

				return true;
			}
		}
		else
			btnstate[i] = false;
	}

	return false;
}


void IN_Ack (void)
{
	IN_StartAck ();

	do
	{
		IN_WaitAndProcessEvents();
	}
	while(!IN_CheckAck ());
}


///////////////////////////////////////////////////////////////////////////
//
//	IN_UserInput() - Waits for the specified delay time (in ticks) or the
//		user pressing a key or a mouse button. If the clear flag is set, it
//		then either clears the key or waits for the user to let the mouse
//		button up.
//
///////////////////////////////////////////////////////////////////////////
bool IN_UserInput(longword delay)
{
	longword	lasttime;

	lasttime = GetTimeCount();
	IN_StartAck ();
	do
	{
		IN_ProcessEvents();
		if (IN_CheckAck())
			return true;
		SDL_Delay(5);
	} while (GetTimeCount() - lasttime < delay);
	return(false);
}

//===========================================================================

/*
===================
=
= IN_MouseButtons
=
===================
*/
int IN_MouseButtons (void)
{
	if (MousePresent)
		return INL_GetMouseButtons();
	else
		return 0;
}

void IN_ReleaseMouse()
{
	GrabInput = false;
	SDL_SetRelativeMouseMode(SDL_FALSE);
}

void IN_GrabMouse()
{
#ifdef _DIII4A //karin: always fullscreen
	if(/*fullscreen*/true || forcegrabmouse)
#else
	if(fullscreen || forcegrabmouse)
#endif
	{
		GrabInput = true;
		SDL_SetRelativeMouseMode(SDL_TRUE);
	}
}

void IN_AdjustMouse()
{
#ifdef _DIII4A //karin: always fullscreen
	if (mouseenabled && (forcegrabmouse || true/*fullscreen*/))
		IN_GrabMouse();
#else
	if (mouseenabled && (forcegrabmouse || fullscreen))
		IN_GrabMouse();
	else if (!fullscreen)
		IN_ReleaseMouse();
#endif
}

bool IN_IsInputGrabbed()
{
	return GrabInput;
}

void IN_CenterMouse()
{
	// Clear out relative mouse movement
	int x, y;
	SDL_GetRelativeMouseState(&x, &y);
}
