#include "quakedef.h"

#include <SDL.h>

#if SDL_VERSION_ATLEAST(2,0,6) && defined(VKQUAKE)
#include <SDL_vulkan.h>
#include "../vk/vkrenderer.h"
#endif

#if SDL_MAJOR_VERSION >=2
extern SDL_Window *sdlwindow;
#else
extern SDL_Surface *sdlsurf;
#endif

qboolean mouseactive;
extern qboolean mouseusedforgui;
extern qboolean vid_isfullscreen;

#if SDL_MAJOR_VERSION > 1 || (SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION >= 3)
#define HAVE_SDL_TEXTINPUT
cvar_t sys_osk = CVARD("sys_osk", "0", "Enables support for text input. This will be ignored when the console has focus, but gamecode may end up with composition boxes appearing.");
#endif

void IN_ActivateMouse(void)
{
	if (mouseactive)
		return;

	mouseactive = true;
	SDL_ShowCursor(0);

#if SDL_MAJOR_VERSION >= 2
	SDL_SetRelativeMouseMode(SDL_TRUE);
	SDL_SetWindowGrab(sdlwindow, SDL_TRUE);
#else
	SDL_WM_GrabInput(SDL_GRAB_ON);
#endif
}

void IN_DeactivateMouse(void)
{
	if (!mouseactive)
		return;

	mouseactive = false;
	SDL_ShowCursor(1);
#if SDL_MAJOR_VERSION >= 2
	SDL_SetRelativeMouseMode(SDL_FALSE);
	SDL_SetWindowGrab(sdlwindow, SDL_FALSE);
#else
	SDL_WM_GrabInput(SDL_GRAB_OFF);
#endif
}

#if SDL_MAJOR_VERSION >= 2
#define MAX_FINGERS 16 //zomg! mutant!
static struct sdlfinger_s
{
	qboolean active;
	SDL_JoystickID jid;
	SDL_TouchID tid;
	SDL_FingerID fid;
} sdlfinger[MAX_FINGERS];
//sanitizes sdl fingers into touch events that our engine can eat.
//we don't really deal with different devices, we just munge the lot into a single thing (allowing fingers to be tracked, at least if splitscreen isn't active).
static uint32_t SDL_GiveFinger(SDL_JoystickID jid, SDL_TouchID tid, SDL_FingerID fid, qboolean fingerraised)
{
	uint32_t f;
	for (f = 0; f < countof(sdlfinger); f++)
	{
		if (sdlfinger[f].active)
		{
			if (sdlfinger[f].jid == jid && sdlfinger[f].tid == tid && sdlfinger[f].fid == fid)
			{
				sdlfinger[f].active = !fingerraised;
				return f;
			}
		}
	}
	for (f = 0; f < countof(sdlfinger); f++)
	{
		if (!sdlfinger[f].active)
		{
			sdlfinger[f].active = !fingerraised;
			sdlfinger[f].jid = jid;
			sdlfinger[f].tid = tid;
			sdlfinger[f].fid = fid;
			return f;
		}
	}
	return f;
}
#endif

#if SDL_MAJOR_VERSION >= 2
#define MAX_JOYSTICKS 16
#ifndef MAXJOYAXIS
#define MAXJOYAXIS 6
#endif

static struct
{
	int qaxis;
	keynum_t pos, neg;
} gpaxismap[] =
{
	{/*SDL_CONTROLLER_AXIS_LEFTX*/			GPAXIS_LT_RIGHT,	K_GP_LEFT_THUMB_RIGHT,	K_GP_LEFT_THUMB_LEFT},
	{/*SDL_CONTROLLER_AXIS_LEFTY*/			GPAXIS_LT_DOWN,		K_GP_LEFT_THUMB_DOWN,	K_GP_LEFT_THUMB_UP},
	{/*SDL_CONTROLLER_AXIS_RIGHTX*/			GPAXIS_RT_RIGHT,	K_GP_RIGHT_THUMB_RIGHT,	K_GP_RIGHT_THUMB_LEFT},
	{/*SDL_CONTROLLER_AXIS_RIGHTY*/			GPAXIS_RT_DOWN,		K_GP_RIGHT_THUMB_DOWN,	K_GP_RIGHT_THUMB_UP},
	{/*SDL_CONTROLLER_AXIS_TRIGGERLEFT*/	GPAXIS_LT_TRIGGER,	K_GP_LEFT_TRIGGER,		0},
	{/*SDL_CONTROLLER_AXIS_TRIGGERRIGHT*/	GPAXIS_RT_TRIGGER,	K_GP_RIGHT_TRIGGER,		0},
};
static const int gpbuttonmap[] =
{
	K_GP_A,
	K_GP_B,
	K_GP_X,
	K_GP_Y,
	K_GP_BACK,
	K_GP_GUIDE,
	K_GP_START,
	K_GP_LEFT_STICK,
	K_GP_RIGHT_STICK,
	K_GP_LEFT_SHOULDER,
	K_GP_RIGHT_SHOULDER,
	K_GP_DPAD_UP,
	K_GP_DPAD_DOWN,
	K_GP_DPAD_LEFT,
	K_GP_DPAD_RIGHT,
	K_GP_MISC1,
	K_GP_PADDLE1,
	K_GP_PADDLE2,
	K_GP_PADDLE3,
	K_GP_PADDLE4,
	K_GP_TOUCHPAD
};

static struct sdljoy_s
{
	//fte doesn't distinguish between joysticks and controllers.
	//in sdl, controllers are some glorified version of joysticks apparently.
	char *devname;
	SDL_Joystick *joystick;
	SDL_GameController *controller;
	SDL_JoystickID id;
	unsigned int qdevid;

	//axis junk
	unsigned int axistobuttonp;
	unsigned int axistobuttonn;
	float repeattime[countof(gpaxismap)];

	//button junk
	unsigned int buttonheld;
	float buttonrepeat[countof(gpbuttonmap)];
} sdljoy[MAX_JOYSTICKS]; 

static cvar_t joy_only = CVARD("joyonly", "0", "If true, treats \"game controllers\" as regular joysticks.\nMust be set at startup.");

//the enumid is the value for the open function rather than the working id.
static void J_AllocateDevID(struct sdljoy_s *joy)
{
	extern cvar_t in_skipplayerone;
	unsigned int id = (in_skipplayerone.ival?1:0), j;
	for (j = 0; j < MAX_JOYSTICKS;)
	{
		if (sdljoy[j++].qdevid == id)
		{
			j = 0;
			id++;
		}
	}

	joy->qdevid = id;

#if SDL_VERSION_ATLEAST(2,0,14)
	if (joy->controller)
	{
		//enable some sensors if they're there. because we can.
		if (SDL_GameControllerHasSensor(joy->controller, SDL_SENSOR_ACCEL))
			SDL_GameControllerSetSensorEnabled(joy->controller, SDL_SENSOR_ACCEL, SDL_TRUE);
		if (SDL_GameControllerHasSensor(joy->controller, SDL_SENSOR_GYRO))
			SDL_GameControllerSetSensorEnabled(joy->controller, SDL_SENSOR_GYRO, SDL_TRUE);
	}
#endif
}
static void J_ControllerAdded(int enumid)
{
	const char *cname;
	int i;

	if (joy_only.ival)
		return;

	for (i = 0; i < MAX_JOYSTICKS; i++)
		if (sdljoy[i].controller == NULL && sdljoy[i].joystick == NULL)
			break;
	if (i == MAX_JOYSTICKS)
		return;

	sdljoy[i].controller = SDL_GameControllerOpen(enumid);
	if (!sdljoy[i].controller)
		return;
	sdljoy[i].joystick = SDL_GameControllerGetJoystick(sdljoy[i].controller);
	sdljoy[i].id = SDL_JoystickInstanceID(sdljoy[i].joystick);

	cname = SDL_GameControllerName(sdljoy[i].controller);
	if (!cname)
		cname = "Unknown Controller";
	Con_Printf("Found new controller (%i): %s\n", i, cname);
	sdljoy[i].devname = Z_StrDup(cname);
	sdljoy[i].qdevid = DEVID_UNSET;
}
static void J_JoystickAdded(int enumid)
{
	const char *cname;
	int i;
	for (i = 0; i < MAX_JOYSTICKS; i++)
		if (sdljoy[i].joystick == NULL && sdljoy[i].controller == NULL)
			break;
	if (i == MAX_JOYSTICKS)
		return;

	if (!joy_only.ival  &&  SDL_IsGameController(enumid))	//if its reported via the gamecontroller api then use that instead. don't open it twice.
		return;

	sdljoy[i].joystick = SDL_JoystickOpen(enumid);
	if (!sdljoy[i].joystick)
		return;
	sdljoy[i].id = SDL_JoystickInstanceID(sdljoy[i].joystick);

	cname = SDL_JoystickName(sdljoy[i].joystick);
	if (!cname)
		cname = "Unknown Joystick";
	Con_Printf("Found new joystick (%i): %s\n", i, cname);
	sdljoy[i].qdevid = DEVID_UNSET;
}
static struct sdljoy_s *J_DevId(SDL_JoystickID jid)
{
	int i;
	for (i = 0; i < MAX_JOYSTICKS; i++)
		if (sdljoy[i].joystick && sdljoy[i].id == jid)
			return &sdljoy[i];
	return NULL;
}
static void J_ControllerAxis(SDL_JoystickID jid, int axis, int value)
{
	struct sdljoy_s *joy = J_DevId(jid);

	if (joy->qdevid == DEVID_UNSET)
	{
		if (abs(value) < 0x4000)
			return;	//has to be big enough to handle any generous dead zone.
		J_AllocateDevID(joy);
	}

	if (joy && axis < countof(gpaxismap) && joy->qdevid != DEVID_UNSET)
	{
		if (value > 0x4000 && gpaxismap[axis].pos)
		{
			if (!(joy->axistobuttonp & (1u<<axis)))
			{
				IN_KeyEvent(joy->qdevid, true, gpaxismap[axis].pos, 0);
				joy->repeattime[axis] = 1.0;
			}
			joy->axistobuttonp |= 1u<<axis;
		}
		else if (joy->axistobuttonp & (1u<<axis))
		{
			IN_KeyEvent(joy->qdevid, false, gpaxismap[axis].pos, 0);
			joy->axistobuttonp &= ~(1u<<axis);
		}

		if (value < -0x4000 && gpaxismap[axis].neg)
		{
			if (!(joy->axistobuttonn & (1u<<axis)))
			{
				IN_KeyEvent(joy->qdevid, true, gpaxismap[axis].neg, 0);
				joy->repeattime[axis] = 1.0;
			}
			joy->axistobuttonn |= 1u<<axis;
		}
		else if (joy->axistobuttonn & (1u<<axis))
		{
			IN_KeyEvent(joy->qdevid, false, gpaxismap[axis].neg, 0);
			joy->axistobuttonn &= ~(1u<<axis);
		}

		IN_JoystickAxisEvent(joy->qdevid, gpaxismap[axis].qaxis, value / 32767.0);
	}
}
static void J_JoystickAxis(SDL_JoystickID jid, int axis, int value)
{
	struct sdljoy_s *joy = J_DevId(jid);

	if (joy->qdevid == DEVID_UNSET)
	{
		if (abs(value) < 0x1000)
			return;
		J_AllocateDevID(joy);
	}

	if (joy && !joy->controller && axis < MAXJOYAXIS && joy->qdevid != DEVID_UNSET)
		IN_JoystickAxisEvent(joy->qdevid, axis, value / 32767.0);
}
//we don't do hats and balls and stuff.
static void J_ControllerButton(SDL_JoystickID jid, int button, qboolean pressed)
{
	//controllers have reliable button maps.
	//but that doesn't meant that fte has specific k_ names for those buttons, but the mapping should be reliable, at least until they get mapped to proper k_ values.
	struct sdljoy_s *joy = J_DevId(jid);
	if (joy && button < countof(gpbuttonmap))
	{
		if (joy->qdevid == DEVID_UNSET)
		{
			if (!pressed)
				return;
			J_AllocateDevID(joy);
		}
		if (pressed)
			joy->buttonheld |= (1u<<button);
		else
			joy->buttonheld &= ~(1u<<button);
		IN_KeyEvent(joy->qdevid, pressed, gpbuttonmap[button], 0);
		joy->buttonrepeat[button] = 1.0;
	}
}
static void J_JoystickButton(SDL_JoystickID jid, int button, qboolean pressed)
{
	//generic joysticks have no specific mappings. they're really random like that.
	static const int buttonmap[] = {
		K_JOY1,
		K_JOY2,
		K_JOY3,
		K_JOY4,
		K_JOY5,
		K_JOY6,
		K_JOY7,
		K_JOY8,
		K_JOY9,
		K_JOY10,
		K_JOY11,
		K_JOY12,
		K_JOY13,
		K_JOY14,
		K_JOY15,
		K_JOY16,
		K_JOY17,
		K_JOY18,
		K_JOY19,
		K_JOY20,
		K_JOY21,
		K_JOY22,
		K_JOY23,
		K_JOY24,
		K_JOY25,
		K_JOY26,
		K_JOY27,
		K_JOY28,
		K_JOY29,
		K_JOY30,
		K_JOY31,
		K_JOY32,
		K_AUX1,
		K_AUX2,
		K_AUX3,
		K_AUX4,
		K_AUX5,
		K_AUX6,
		K_AUX7,
		K_AUX8,
		K_AUX9,
		K_AUX10,
		K_AUX11,
		K_AUX12,
		K_AUX13,
		K_AUX14,
		K_AUX15,
		K_AUX16
	};

	struct sdljoy_s *joy = J_DevId(jid);
	if (joy && !joy->controller && button < sizeof(buttonmap)/sizeof(buttonmap[0]))
	{
		if (joy->qdevid == DEVID_UNSET)
		{
			if (!pressed)
				return;
			J_AllocateDevID(joy);
		}
		IN_KeyEvent(joy->qdevid, pressed, buttonmap[button], 0);
	}
}

enum controllertype_e INS_GetControllerType(int id)
{
#if SDL_VERSION_ATLEAST(2,0,12)
	int i;
	for (i = 0; i < MAX_JOYSTICKS; i++)
	{
		if (sdljoy[i].qdevid == id)
		{
			switch(SDL_GameControllerTypeForIndex(sdljoy[i].id))
			{
			default:	//for the future...
			case SDL_CONTROLLER_TYPE_UNKNOWN:
				return CONTROLLER_UNKNOWN;
#if SDL_VERSION_ATLEAST(2,0,14)
			case SDL_CONTROLLER_TYPE_VIRTUAL:	//don't really know... assume steaminput and thus steamdeck and thus xbox-like.
				return CONTROLLER_VIRTUAL;
#endif
			case SDL_CONTROLLER_TYPE_XBOX360:
			case SDL_CONTROLLER_TYPE_XBOXONE:
#if SDL_VERSION_ATLEAST(2,0,16)
			case SDL_CONTROLLER_TYPE_GOOGLE_STADIA:	//close enough
			case SDL_CONTROLLER_TYPE_AMAZON_LUNA:	//it'll do. I guess we're starting to see a standard here.
#endif
#if SDL_VERSION_ATLEAST(2,0,24)
			case SDL_CONTROLLER_TYPE_NVIDIA_SHIELD:
#endif
				return CONTROLLER_XBOX;	//a on bottom, b('cancel') to right
			case SDL_CONTROLLER_TYPE_PS3:
			case SDL_CONTROLLER_TYPE_PS4:
#if SDL_VERSION_ATLEAST(2,0,14)
			case SDL_CONTROLLER_TYPE_PS5:
#endif
				return CONTROLLER_PLAYSTATION;	//weird indecipherable shapes.
			case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO:
#if SDL_VERSION_ATLEAST(2,0,24)
			case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_LEFT:
			case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT:
			case SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
#endif
				return CONTROLLER_NINTENDO;	//b on bottom, a('cancel') to right
			}
		}
	}
#endif
	return 0;
}
void INS_Rumble(int id, quint16_t amp_low, quint16_t amp_high, quint32_t duration)
{
#if SDL_VERSION_ATLEAST(2,0,9)
	int i;
	if (duration > 10000)
		duration = 10000;

	for (i = 0; i < MAX_JOYSTICKS; i++)
	{
		if (sdljoy[i].qdevid == id)
		{
			SDL_GameControllerRumble(SDL_GameControllerFromInstanceID(sdljoy[i].id), amp_low, amp_high, duration);
			return;
		}
	}
#else
	Con_DPrintf(CON_WARNING "Rumble is requires at least SDL 2.0.9\n");
#endif
}

void INS_RumbleTriggers(int id, quint16_t left, quint16_t right, quint32_t duration)
{
#if SDL_VERSION_ATLEAST(2,0,14)
	int i;
	if (duration > 10000)
		duration = 10000;

	for (i = 0; i < MAX_JOYSTICKS; i++)
	{
		if (sdljoy[i].qdevid == id)
		{
			SDL_GameControllerRumbleTriggers(SDL_GameControllerFromInstanceID(sdljoy[i].id), left, right, duration);
			return;
		}
	}
#else
	Con_DPrintf(CON_WARNING "Trigger rumble is requires at least SDL 2.0.14\n");
#endif
}

void INS_SetLEDColor(int id, vec3_t color)
{
#if SDL_VERSION_ATLEAST(2,0,14)
	int i;
	/* maybe we'll eventually get sRGB LEDs */
	color[0] *= 255.0f;
	color[1] *= 255.0f;
	color[2] *= 255.0f;

	for (i = 0; i < MAX_JOYSTICKS; i++)
	{
		if (sdljoy[i].qdevid == id)
		{
			SDL_GameControllerSetLED(SDL_GameControllerFromInstanceID(sdljoy[i].id), (uint8_t)color[0], (uint8_t)color[1], (uint8_t)color[2]);
			return;
		}
	}
#else
	Con_DPrintf(CON_WARNING "Trigger rumble is requires at least SDL 2.0.14\n");
#endif
}

void INS_SetTriggerFX(int id, const void *data, size_t size)
{
#if SDL_VERSION_ATLEAST(2,0,15)
	int i;
	for (i = 0; i < MAX_JOYSTICKS; i++)
	{
		if (sdljoy[i].qdevid == id)
		{
			SDL_GameControllerSendEffect(SDL_GameControllerFromInstanceID(sdljoy[i].id), &data, size);
			return;
		}
	}
#else
	Con_DPrintf(CON_WARNING "Trigger FX is requires at least SDL 2.0.15\n");
#endif
}

#if SDL_VERSION_ATLEAST(2,0,14)
static void J_ControllerTouchPad(SDL_JoystickID jid, int pad, int finger, int fingerstate, float x, float y, float pressure)
{
#if 1
	//FIXME: forgets which seat its meant to be controlling.
	uint32_t thefinger = SDL_GiveFinger(jid, pad, finger, fingerstate<0);
#else
	//FIXME: conflicts with regular mice (very problematic when using absolute coords)
	uint32_t thefinger;
	struct sdljoy_s *joy = J_DevId(jid);
	if (!joy)
		return;
	if (joy->qdevid == DEVID_UNSET)
	{
		if (fingerstate!=1)
			return;
		J_AllocateDevID(joy);
	}
	thefinger = joy->qdevid;
#endif
	x *= vid.pixelwidth;
	y *= vid.pixelheight;
	IN_MouseMove(thefinger, true, x, y, 0, pressure);
	if (finger)
		IN_KeyEvent(thefinger, finger>0, K_MOUSE1, 0);
}
static void J_ControllerSensor(SDL_JoystickID jid, SDL_SensorType sensor, float *data)
{
	struct sdljoy_s *joy = J_DevId(jid);
	if (!joy)
		return;
	//don't assign an id here. wait for a button.
	if (joy->qdevid == DEVID_UNSET)
		return;

	switch(sensor)
	{
	case SDL_SENSOR_ACCEL:
		IN_Accelerometer(joy->qdevid, data[0], data[1], data[2]);
		break;
	case SDL_SENSOR_GYRO:
		IN_Gyroscope(joy->qdevid, data[0], data[1], data[2]);
		break;
/*#if SDL_VERSION_ATLEAST(2,25,0)
	case SDL_SENSOR_ACCEL_L:
	case SDL_SENSOR_ACCEL_R:
	case SDL_SENSOR_GYRO_L:
	case SDL_SENSOR_GYRO_R:
#endif*/
	case SDL_SENSOR_INVALID:
	case SDL_SENSOR_UNKNOWN:
	default:
		break;
	}
}
#endif

static void J_Kill(SDL_JoystickID jid, qboolean verbose)
{
	int i;
	struct sdljoy_s *joy = J_DevId(jid);

	if (!joy)
		return;

	//make sure all the axis are nulled out, to avoid surprises.
	if (joy->qdevid != DEVID_UNSET)
	{
		for (i = 0; i < 6; i++)
			IN_JoystickAxisEvent(joy->qdevid, i, 0);
		if (joy->controller)
		{
			for (i = 0; i < 32; i++)
				J_ControllerButton(jid, i, false);
		}
		else
		{
			for (i = 0; i < 32; i++)
				J_JoystickButton(jid, i, false);
		}
	}

	if (joy->controller)
	{
		Con_Printf("Controller unplugged(%i): %s\n", (int)(joy - sdljoy), joy->devname);
		SDL_GameControllerClose(joy->controller);
	}
	else
	{
		Con_Printf("Joystick unplugged(%i): %s\n", (int)(joy - sdljoy), joy->devname);
		SDL_JoystickClose(joy->joystick);
	}
	joy->controller = NULL;
	joy->joystick = NULL;
	Z_Free(joy->devname);
	joy->devname = NULL;
	joy->qdevid = DEVID_UNSET;
}
static void J_KillAll(void)
{
	int i;
	for (i = 0; i < MAX_JOYSTICKS; i++)
		J_Kill(sdljoy[i].id, false);
}
#endif

#if SDL_MAJOR_VERSION >= 2
//FIXME: switch to scancodes rather than keysyms
//use SDL_GetKeyName(SDL_GetKeyFromScancode(quaketosdl[qkey])) for keybinds menu
unsigned int MySDL_MapKey(SDL_Keycode sdlkey)
{
	switch(sdlkey)
	{
	default:				return 0;
	//any ascii chars can be mapped directly to keys, even if they're only ever accessed with shift etc... oh well.
	case SDLK_RETURN:		return K_ENTER;
	case SDLK_ESCAPE:		return K_ESCAPE;
	case SDLK_BACKSPACE:	return K_BACKSPACE;
	case SDLK_TAB:			return K_TAB;
	case SDLK_SPACE:		return K_SPACE;
	case SDLK_EXCLAIM:
	case SDLK_QUOTEDBL:
	case SDLK_HASH:
	case SDLK_PERCENT:
	case SDLK_DOLLAR:
	case SDLK_AMPERSAND:
	case SDLK_QUOTE:
	case SDLK_LEFTPAREN:
	case SDLK_RIGHTPAREN:
	case SDLK_ASTERISK:
	case SDLK_PLUS:
	case SDLK_COMMA:
	case SDLK_MINUS:
	case SDLK_PERIOD:
	case SDLK_SLASH:
	case SDLK_0:
	case SDLK_1:
	case SDLK_2:
	case SDLK_3:
	case SDLK_4:
	case SDLK_5:
	case SDLK_6:
	case SDLK_7:
	case SDLK_8:
	case SDLK_9:
	case SDLK_COLON:
	case SDLK_SEMICOLON:
	case SDLK_LESS:
	case SDLK_EQUALS:
	case SDLK_GREATER:
	case SDLK_QUESTION:
	case SDLK_AT:
	case SDLK_LEFTBRACKET:
	case SDLK_BACKSLASH:
	case SDLK_RIGHTBRACKET:
	case SDLK_CARET:
	case SDLK_UNDERSCORE:
	case SDLK_BACKQUOTE:
	case SDLK_a:
	case SDLK_b:
	case SDLK_c:
	case SDLK_d:
	case SDLK_e:
	case SDLK_f:
	case SDLK_g:
	case SDLK_h:
	case SDLK_i:
	case SDLK_j:
	case SDLK_k:
	case SDLK_l:
	case SDLK_m:
	case SDLK_n:
	case SDLK_o:
	case SDLK_p:
	case SDLK_q:
	case SDLK_r:
	case SDLK_s:
	case SDLK_t:
	case SDLK_u:
	case SDLK_v:
	case SDLK_w:
	case SDLK_x:
	case SDLK_y:
	case SDLK_z:
		return sdlkey;
	case SDLK_CAPSLOCK:		return K_CAPSLOCK;
	case SDLK_F1:			return K_F1;
	case SDLK_F2:			return K_F2;
	case SDLK_F3:			return K_F3;
	case SDLK_F4:			return K_F4;
	case SDLK_F5:			return K_F5;
	case SDLK_F6:			return K_F6;
	case SDLK_F7:			return K_F7;
	case SDLK_F8:			return K_F8;
	case SDLK_F9: 			return K_F9;
	case SDLK_F10:			return K_F10;
	case SDLK_F11:			return K_F11;
	case SDLK_F12:			return K_F12;
	case SDLK_PRINTSCREEN:	return K_PRINTSCREEN;
	case SDLK_SCROLLLOCK:	return K_SCRLCK;
	case SDLK_PAUSE:		return K_PAUSE;
	case SDLK_INSERT:		return K_INS;
	case SDLK_HOME:			return K_HOME;
	case SDLK_PAGEUP:		return K_PGUP;
	case SDLK_DELETE:		return K_DEL;
	case SDLK_END:			return K_END;
	case SDLK_PAGEDOWN:		return K_PGDN;
	case SDLK_RIGHT:		return K_RIGHTARROW;
	case SDLK_LEFT:			return K_LEFTARROW;
	case SDLK_DOWN:			return K_DOWNARROW;
	case SDLK_UP:			return K_UPARROW;
	case SDLK_NUMLOCKCLEAR:	return K_KP_NUMLOCK;
	case SDLK_KP_DIVIDE:	return K_KP_SLASH;
	case SDLK_KP_MULTIPLY:	return K_KP_STAR;
	case SDLK_KP_MINUS:		return K_KP_MINUS;
	case SDLK_KP_PLUS:		return K_KP_PLUS;
	case SDLK_KP_ENTER:		return K_KP_ENTER;
	case SDLK_KP_1:			return K_KP_END;
	case SDLK_KP_2:			return K_KP_DOWNARROW;
	case SDLK_KP_3:			return K_KP_PGDN;
	case SDLK_KP_4:			return K_KP_LEFTARROW;
	case SDLK_KP_5:			return K_KP_5;
	case SDLK_KP_6:			return K_KP_RIGHTARROW;
	case SDLK_KP_7:			return K_KP_HOME;
	case SDLK_KP_8:			return K_KP_UPARROW;
	case SDLK_KP_9:			return K_KP_PGDN;
	case SDLK_KP_0:			return K_KP_INS;
	case SDLK_KP_PERIOD:	return K_KP_DEL;
	case SDLK_APPLICATION:	return K_APP;
	case SDLK_POWER:		return K_POWER;
	case SDLK_KP_EQUALS:	return K_KP_EQUALS;
	case SDLK_F13:			return K_F13;
	case SDLK_F14:			return K_F14;
	case SDLK_F15:			return K_F15;
/*
	case SDLK_F16:			return K_;
	case SDLK_F17:			return K_;
	case SDLK_F18:			return K_;
	case SDLK_F19:			return K_;
	case SDLK_F20:			return K_;
	case SDLK_F21:			return K_;
	case SDLK_F22:			return K_;
	case SDLK_F23:			return K_;
	case SDLK_F24:			return K_;
	case SDLK_EXECUTE:		return K_;
	case SDLK_HELP:			return K_;
	case SDLK_MENU:			return K_;
	case SDLK_SELECT:		return K_;
	case SDLK_STOP:			return K_;
	case SDLK_AGAIN:		return K_;
	case SDLK_UNDO:			return K_;
	case SDLK_CUT:			return K_;
	case SDLK_COPY:			return K_;
	case SDLK_PASTE:		return K_;
	case SDLK_FIND:			return K_;
	case SDLK_MUTE:			return K_;
*/
	case SDLK_VOLUMEUP:		return K_VOLUP;
	case SDLK_VOLUMEDOWN:	return K_VOLDOWN;
/*
	case SDLK_KP_COMMA:		return K_;
	case SDLK_KP_EQUALSAS400:	return K_;
	case SDLK_ALTERASE:		return K_;
	case SDLK_SYSREQ:		return K_;
	case SDLK_CANCEL:		return K_;
	case SDLK_CLEAR:		return K_;
	case SDLK_PRIOR:		return K_;
	case SDLK_RETURN2:		return K_;
	case SDLK_SEPARATOR:	return K_;
	case SDLK_OUT:			return K_;
	case SDLK_OPER:			return K_;
	case SDLK_CLEARAGAIN:	return K_;
	case SDLK_CRSEL:		return K_;
	case SDLK_EXSEL:		return K_;
	case SDLK_KP_00:		return K_;
	case SDLK_KP_000:		return K_;
	case SDLK_THOUSANDSSEPARATOR:	return K_;
	case SDLK_DECIMALSEPARATOR:		return K_;
	case SDLK_CURRENCYUNIT:			return K_;
	case SDLK_CURRENCYSUBUNIT:		return K_;
	case SDLK_KP_LEFTPAREN:			return K_;
	case SDLK_KP_RIGHTPAREN:		return K_;
	case SDLK_KP_LEFTBRACE:			return K_;
	case SDLK_KP_RIGHTBRACE:		return K_;
	case SDLK_KP_TAB:		return K_;
	case SDLK_KP_BACKSPACE:	return K_;
	case SDLK_KP_A:			return K_;
	case SDLK_KP_B:			return K_;
	case SDLK_KP_C:			return K_;
	case SDLK_KP_D:			return K_;
	case SDLK_KP_E:			return K_;
	case SDLK_KP_F:			return K_;
	case SDLK_KP_XOR:		return K_;
	case SDLK_KP_POWER:		return K_;
	case SDLK_KP_PERCENT:	return K_;
	case SDLK_KP_LESS:		return K_;
	case SDLK_KP_GREATER:	return K_;
	case SDLK_KP_AMPERSAND: return K_;
	case SDLK_KP_DBLAMPERSAND:		return K_;
	case SDLK_KP_VERTICALBAR:		return K_;
	case SDLK_KP_DBLVERTICALBAR:	return K_;
	case SDLK_KP_COLON:		return K_;
	case SDLK_KP_HASH:		return K_;
	case SDLK_KP_SPACE:		return K_;
	case SDLK_KP_AT:		return K_;
	case SDLK_KP_EXCLAM:	return K_;
	case SDLK_KP_MEMSTORE:	return K_;
	case SDLK_KP_MEMRECALL:	return K_;
	case SDLK_KP_MEMCLEAR:	return K_;
	case SDLK_KP_MEMADD:	return K_;
	case SDLK_KP_MEMSUBTRACT:	return K_;
	case SDLK_KP_MEMMULTIPLY:	return K_;
	case SDLK_KP_MEMDIVIDE:		return K_;
	case SDLK_KP_PLUSMINUS:		return K_;
	case SDLK_KP_CLEAR:			return K_;
	case SDLK_KP_CLEARENTRY:	return K_;
	case SDLK_KP_BINARY:		return K_;
	case SDLK_KP_OCTAL:			return K_;
	case SDLK_KP_DECIMAL:		return K_;
	case SDLK_KP_HEXADECIMAL:	return K_;
*/
	case SDLK_LCTRL:		return K_LCTRL;
	case SDLK_LSHIFT:		return K_LSHIFT;
	case SDLK_LALT:			return K_LALT;
	case SDLK_LGUI:			return K_APP;
	case SDLK_RCTRL:		return K_RCTRL;
	case SDLK_RSHIFT:		return K_RSHIFT;
	case SDLK_RALT:			return K_RALT;
/*
	case SDLK_RGUI:			return K_;
	case SDLK_MODE:			return K_;
	case SDLK_AUDIONEXT:	return K_;
	case SDLK_AUDIOPREV:	return K_;
	case SDLK_AUDIOSTOP:	return K_;
	case SDLK_AUDIOPLAY:	return K_;
	case SDLK_AUDIOMUTE:	return K_;
	case SDLK_MEDIASELECT:	return K_;
	case SDLK_WWW:			return K_;
	case SDLK_MAIL:			return K_;
	case SDLK_CALCULATOR:	return K_;
	case SDLK_COMPUTER:		return K_;
	case SDLK_AC_SEARCH:	return K_;
	case SDLK_AC_HOME:		return K_;
	case SDLK_AC_BACK:		return K_;
	case SDLK_AC_FORWARD:	return K_;
	case SDLK_AC_STOP:		return K_;
	case SDLK_AC_REFRESH:	return K_;
	case SDLK_AC_BOOKMARKS:	return K_;
	case SDLK_BRIGHTNESSDOWN:	return K_;
	case SDLK_BRIGHTNESSUP:		return K_;
	case SDLK_DISPLAYSWITCH:	return K_;
	case SDLK_KBDILLUMTOGGLE:	return K_;
	case SDLK_KBDILLUMDOWN:		return K_;
	case SDLK_KBDILLUMUP:		return K_;
	case SDLK_EJECT:			return K_;
	case SDLK_SLEEP:			return K_;
*/
	}
}
#else
#define tenoh	0,0,0,0,0, 0,0,0,0,0
#define fiftyoh tenoh, tenoh, tenoh, tenoh, tenoh
#define hundredoh fiftyoh, fiftyoh
static unsigned int tbl_sdltoquake[] =
{
	0,0,0,0,		//SDLK_UNKNOWN		= 0,
	0,0,0,0,		//SDLK_FIRST		= 0,
	K_BACKSPACE,	//SDLK_BACKSPACE	= 8,
	K_TAB,			//SDLK_TAB			= 9,
	0,0,
	0,				//SDLK_CLEAR		= 12,
	K_ENTER,		//SDLK_RETURN		= 13,
	0,0,0,0,0,
	K_PAUSE,		//SDLK_PAUSE		= 19,
	0,0,0,0,0,0,0,
	K_ESCAPE,		//SDLK_ESCAPE		= 27,
	0,0,0,0,
	K_SPACE,		//SDLK_SPACE		= 32,
	'!',			//SDLK_EXCLAIM		= 33,
	'"',			//SDLK_QUOTEDBL		= 34,
	'#',			//SDLK_HASH			= 35,
	'$',			//SDLK_DOLLAR		= 36,
	0,
	'&',			//SDLK_AMPERSAND	= 38,
	'\'',			//SDLK_QUOTE		= 39,
	'(',			//SDLK_LEFTPAREN	= 40,
	')',			//SDLK_RIGHTPAREN	= 41,
	'*',			//SDLK_ASTERISK		= 42,
	'+',			//SDLK_PLUS			= 43,
	',',			//SDLK_COMMA		= 44,
	'-',			//SDLK_MINUS		= 45,
	'.',			//SDLK_PERIOD		= 46,
	'/',			//SDLK_SLASH		= 47,
	'0',			//SDLK_0			= 48,
	'1',			//SDLK_1			= 49,
	'2',			//SDLK_2			= 50,
	'3',			//SDLK_3			= 51,
	'4',			//SDLK_4			= 52,
	'5',			//SDLK_5			= 53,
	'6',			//SDLK_6			= 54,
	'7',			//SDLK_7			= 55,
	'8',			//SDLK_8			= 56,
	'9',			//SDLK_9			= 57,
	':',			//SDLK_COLON		= 58,
	';',			//SDLK_SEMICOLON	= 59,
	'<',			//SDLK_LESS			= 60,
	'=',			//SDLK_EQUALS		= 61,
	'>',			//SDLK_GREATER		= 62,
	'?',			//SDLK_QUESTION		= 63,
	'@',			//SDLK_AT			= 64,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	'[',		//SDLK_LEFTBRACKET	= 91,
	'\\',		//SDLK_BACKSLASH	= 92,
	']',		//SDLK_RIGHTBRACKET	= 93,
	'^',		//SDLK_CARET		= 94,
	'_',		//SDLK_UNDERSCORE	= 95,
	'`',		//SDLK_BACKQUOTE	= 96,
	'a',		//SDLK_a			= 97,
	'b',		//SDLK_b			= 98,
	'c',		//SDLK_c			= 99,
	'd',		//SDLK_d			= 100,
	'e',		//SDLK_e			= 101,
	'f',		//SDLK_f			= 102,
	'g',		//SDLK_g			= 103,
	'h',		//SDLK_h			= 104,
	'i',		//SDLK_i			= 105,
	'j',		//SDLK_j			= 106,
	'k',		//SDLK_k			= 107,
	'l',		//SDLK_l			= 108,
	'm',		//SDLK_m			= 109,
	'n',		//SDLK_n			= 110,
	'o',		//SDLK_o			= 111,
	'p',		//SDLK_p			= 112,
	'q',		//SDLK_q			= 113,
	'r',		//SDLK_r			= 114,
	's',		//SDLK_s			= 115,
	't',		//SDLK_t			= 116,
	'u',		//SDLK_u			= 117,
	'v',		//SDLK_v			= 118,
	'w',		//SDLK_w			= 119,
	'x',		//SDLK_x			= 120,
	'y',		//SDLK_y			= 121,
	'z',		//SDLK_z			= 122,
	0,0,0,0,
	K_DEL, 		//SDLK_DELETE		= 127,
	hundredoh /*227*/, tenoh, tenoh, 0,0,0,0,0,0,0,0,
	K_KP_INS,		//SDLK_KP0		= 256,
	K_KP_END,		//SDLK_KP1		= 257,
	K_KP_DOWNARROW,		//SDLK_KP2		= 258,
	K_KP_PGDN,		//SDLK_KP3		= 259,
	K_KP_LEFTARROW,		//SDLK_KP4		= 260,
	K_KP_5,		//SDLK_KP5		= 261,
	K_KP_RIGHTARROW,		//SDLK_KP6		= 262,
	K_KP_HOME,		//SDLK_KP7		= 263,
	K_KP_UPARROW,		//SDLK_KP8		= 264,
	K_KP_PGUP,		//SDLK_KP9		= 265,
	K_KP_DEL,//SDLK_KP_PERIOD	= 266,
	K_KP_SLASH,//SDLK_KP_DIVIDE	= 267,
	K_KP_STAR,//SDLK_KP_MULTIPLY= 268,
	K_KP_MINUS,	//SDLK_KP_MINUS		= 269,
	K_KP_PLUS,	//SDLK_KP_PLUS		= 270,
	K_KP_ENTER,	//SDLK_KP_ENTER		= 271,
	K_KP_EQUALS,//SDLK_KP_EQUALS	= 272,
	K_UPARROW,	//SDLK_UP		= 273,
	K_DOWNARROW,//SDLK_DOWN		= 274,
	K_RIGHTARROW,//SDLK_RIGHT	= 275,
	K_LEFTARROW,//SDLK_LEFT		= 276,
	K_INS,		//SDLK_INSERT	= 277,
	K_HOME,		//SDLK_HOME		= 278,
	K_END,		//SDLK_END		= 279,
	K_PGUP, 	//SDLK_PAGEUP	= 280,
	K_PGDN,		//SDLK_PAGEDOWN	= 281,
	K_F1,		//SDLK_F1		= 282,
	K_F2,		//SDLK_F2		= 283,
	K_F3,		//SDLK_F3		= 284,
	K_F4,		//SDLK_F4		= 285,
	K_F5,		//SDLK_F5		= 286,
	K_F6,		//SDLK_F6		= 287,
	K_F7,		//SDLK_F7		= 288,
	K_F8,		//SDLK_F8		= 289,
	K_F9,		//SDLK_F9		= 290,
	K_F10,		//SDLK_F10		= 291,
	K_F11,		//SDLK_F11		= 292,
	K_F12,		//SDLK_F12		= 293,
	0,			//SDLK_F13		= 294,
	0,			//SDLK_F14		= 295,
	0,			//SDLK_F15		= 296,
	0,0,0,
	0,//K_NUMLOCK,	//SDLK_NUMLOCK	= 300,
	K_CAPSLOCK,	//SDLK_CAPSLOCK	= 301,
	0,//K_SCROLLOCK,//SDLK_SCROLLOCK= 302,
	K_RSHIFT,	//SDLK_RSHIFT	= 303,
	K_LSHIFT,	//SDLK_LSHIFT	= 304,
	K_RCTRL,	//SDLK_RCTRL	= 305,
	K_LCTRL,	//SDLK_LCTRL	= 306,
	K_RALT,		//SDLK_RALT		= 307,
	K_LALT,		//SDLK_LALT		= 308,
	0,			//SDLK_RMETA	= 309,
	0,			//SDLK_LMETA	= 310,
	0,			//SDLK_LSUPER	= 311,		/* Left "Windows" key */
	0,			//SDLK_RSUPER	= 312,		/* Right "Windows" key */
	0,			//SDLK_MODE		= 313,		/* "Alt Gr" key */
	0,			//SDLK_COMPOSE	= 314,		/* Multi-key compose key */
	0,			//SDLK_HELP		= 315,
	0,			//SDLK_PRINT	= 316,
	0,			//SDLK_SYSREQ	= 317,
	K_PAUSE,	//SDLK_BREAK	= 318,
	0,			//SDLK_MENU		= 319,
	0,			//SDLK_POWER	= 320,		/* Power Macintosh power key */
	'e',		//SDLK_EURO		= 321,		/* Some european keyboards */
	0			//SDLK_UNDO		= 322,		/* Atari keyboard has Undo */
};
#endif

/* stubbed */
qboolean INS_KeyToLocalName(int qkey, char *buf, size_t bufsize)
{
	const char *keyname;

	//too lazy to make+maintain a reverse MySDL_MapKey, so lets just make a reverse table with it instead.
	static qboolean stupidtabsetup;
	static SDL_Keycode stupidtable[K_MAX];
	if (!stupidtabsetup)
	{
		int i, k;
		for (i = 0; i < SDL_NUM_SCANCODES; i++)
		{
			k = MySDL_MapKey(i);
			if (k)
				stupidtable[k] = i;

			//grr, oh well, at least we're not scanning 2 billion codes here
			k = MySDL_MapKey(SDL_SCANCODE_TO_KEYCODE(i));
			if (k)
				stupidtable[k] = SDL_SCANCODE_TO_KEYCODE(i);
		}
		stupidtabsetup = true;
	}

	if (stupidtable[qkey])
		keyname = SDL_GetKeyName(stupidtable[qkey]);
	else if (qkey >= K_GP_DIAMOND_DOWN && qkey < K_GP_TOUCHPAD)
	{
		SDL_GameControllerButton b;
		switch(qkey)
		{
		case K_GP_DIAMOND_DOWN:		b = SDL_CONTROLLER_BUTTON_A;				break;
		case K_GP_DIAMOND_RIGHT:	b = SDL_CONTROLLER_BUTTON_B;				break;
		case K_GP_DIAMOND_LEFT:		b = SDL_CONTROLLER_BUTTON_X;				break;
		case K_GP_DIAMOND_UP:		b = SDL_CONTROLLER_BUTTON_Y;				break;
		case K_GP_BACK:				b = SDL_CONTROLLER_BUTTON_BACK;				break;
		case K_GP_GUIDE:			b = SDL_CONTROLLER_BUTTON_GUIDE;			break;
		case K_GP_START:			b = SDL_CONTROLLER_BUTTON_START;			break;
		case K_GP_LEFT_STICK:		b = SDL_CONTROLLER_BUTTON_LEFTSTICK;		break;
		case K_GP_RIGHT_STICK:		b = SDL_CONTROLLER_BUTTON_RIGHTSTICK;		break;
		case K_GP_LEFT_SHOULDER:	b = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;		break;
		case K_GP_RIGHT_SHOULDER:	b = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;	break;
		case K_GP_DPAD_UP:			b = SDL_CONTROLLER_BUTTON_DPAD_UP;			break;
		case K_GP_DPAD_DOWN:		b = SDL_CONTROLLER_BUTTON_DPAD_DOWN;		break;
		case K_GP_DPAD_LEFT:		b = SDL_CONTROLLER_BUTTON_DPAD_LEFT;		break;
		case K_GP_DPAD_RIGHT:		b = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;		break;
#if SDL_VERSION_ATLEAST(2, 0, 14)
		case K_GP_MISC1:			b = SDL_CONTROLLER_BUTTON_MISC1;			break;
		case K_GP_PADDLE1:			b = SDL_CONTROLLER_BUTTON_PADDLE1;			break;
		case K_GP_PADDLE2:			b = SDL_CONTROLLER_BUTTON_PADDLE2;			break;
		case K_GP_PADDLE3:			b = SDL_CONTROLLER_BUTTON_PADDLE3;			break;
		case K_GP_PADDLE4:			b = SDL_CONTROLLER_BUTTON_PADDLE4;			break;
		case K_GP_TOUCHPAD:			b = SDL_CONTROLLER_BUTTON_TOUCHPAD;			break;
#endif
		default:
			return false;
		}
		keyname = SDL_GameControllerGetStringForButton(b);
	}
	else
		return false;

	if (keyname)
	{
		Q_strncpyz(buf, keyname, bufsize);
		return true;
	}
	return false;
}

static unsigned int tbl_sdltoquakemouse[] =
{
	K_MOUSE1,
	K_MOUSE3,
	K_MOUSE2,
#if SDL_MAJOR_VERSION < 2
	K_MWHEELUP,
	K_MWHEELDOWN,
#endif
	K_MOUSE4,
	K_MOUSE5,
	K_MOUSE6,
	K_MOUSE7,
	K_MOUSE8,
	K_MOUSE9,
	K_MOUSE10
};

#ifdef HAVE_SDL_TEXTINPUT
#ifdef __linux__
#include <SDL_misc.h>
static qboolean usesteamosk;
#endif

void INS_SetOSK(int osk)
{
	static SDL_bool active = false;
	if (osk && sdlwindow)
	{
		SDL_Rect rect;
		rect.x = 0;
		rect.y = vid.rotpixelheight/2;
		rect.w = vid.rotpixelwidth;
		rect.h = vid.rotpixelheight - rect.y;
		SDL_SetTextInputRect(&rect);

		if (!active)
		{
#ifdef __linux__
			if (usesteamosk)
				SDL_OpenURL("steam://open/keyboard?Mode=1");
			else
#endif
				SDL_StartTextInput();
			active = true;
//			Con_Printf("OSK shown...\n");
		}
	}
	else
	{
		if (active)
		{
#ifdef __linux__
			if (usesteamosk)
				SDL_OpenURL("steam://close/keyboard?Mode=1");
			else
#endif
				SDL_StopTextInput();
			active = false;
//			Con_Printf("OSK shown... killed\n");
		}
	}
}
#else
void INS_SetOSK(int osk)
{
}
#endif
void Sys_SendKeyEvents(void)
{
	SDL_Event event;
	int axis, j;

#ifdef HAVE_SERVER
	if (isDedicated)
	{
		SV_GetConsoleCommands ();
		return;
	}
#endif

#ifdef HAVE_SDL_TEXTINPUT
	{
		SDL_bool osk = Key_Dest_Has(kdm_console|kdm_cwindows|kdm_message);
		if (Key_Dest_Has(kdm_prompt|kdm_menu))
		{
			j = Menu_WantOSK();
			if (j < 0)
				osk |= sys_osk.ival;
			else
				osk |= j;
		}
		else if (Key_Dest_Has(kdm_game))
			osk |= sys_osk.ival;
		INS_SetOSK(osk);
	}
#endif

	while(SDL_PollEvent(&event))
	{
		switch(event.type)
		{
#if SDL_MAJOR_VERSION >= 2
		case SDL_WINDOWEVENT:
			switch(event.window.event)
			{
			default:
				break;
			case SDL_WINDOWEVENT_SIZE_CHANGED:
#if SDL_VERSION_ATLEAST(2,0,6) && defined(VKQUAKE)
				if (qrenderer == QR_VULKAN)
				{
					unsigned window_width, window_height;
					SDL_Vulkan_GetDrawableSize(sdlwindow, &window_width, &window_height);	//get the proper physical size.
					if (vid.pixelwidth != window_width || vid.pixelheight != window_height)
						vk.neednewswapchain = true;
					break;
				}
#endif
				if (qrenderer == QR_OPENGL)
				{
					#if SDL_VERSION_ATLEAST(2,0,1)
						SDL_GL_GetDrawableSize(sdlwindow, &vid.pixelwidth, &vid.pixelheight);	//get the proper physical size.
					#else
						SDL_GetWindowSize(sdlwindow, &vid.pixelwidth, &vid.pixelheight);
					#endif
					{
						extern cvar_t vid_conautoscale, vid_conwidth;	//make sure the screen is updated properly.
						Cvar_ForceCallback(&vid_conautoscale);
						Cvar_ForceCallback(&vid_conwidth);
					}
				}
				break;
			case SDL_WINDOWEVENT_FOCUS_GAINED:
				vid.activeapp = true;
				break;
			case SDL_WINDOWEVENT_FOCUS_LOST:
				vid.activeapp = false;
				break;
			case SDL_WINDOWEVENT_CLOSE:
				Cbuf_AddText("quit prompt\n", RESTRICT_LOCAL);
				break;
			}
			break;
#else

		case SDL_ACTIVEEVENT:
			if (event.active.state & SDL_APPINPUTFOCUS)
			{	//follow keyboard status
				vid.activeapp = !!event.active.gain;
				break;
			}
			break;

		case SDL_VIDEORESIZE:
#ifndef SERVERONLY
			vid.pixelwidth = event.resize.w;
			vid.pixelheight = event.resize.h;
			{
			extern cvar_t vid_conautoscale, vid_conwidth;	//make sure the screen is updated properly.
			Cvar_ForceCallback(&vid_conautoscale);
			Cvar_ForceCallback(&vid_conwidth);
			}
#endif
			break;
#endif

		case SDL_KEYUP:
		case SDL_KEYDOWN:
			{
				int s = event.key.keysym.sym;
				int qs;
#if SDL_MAJOR_VERSION >= 2
				qs = MySDL_MapKey(s);
#else
				if (s < sizeof(tbl_sdltoquake) / sizeof(tbl_sdltoquake[0]))
					qs = tbl_sdltoquake[s];
				else 
					qs = 0;
#endif

#ifdef FTE_TARGET_WEB
				if (s == 1249)
					qs = K_SHIFT;
#endif
#ifdef HAVE_SDL_TEXTINPUT
				IN_KeyEvent(0, event.key.state, qs, 0);
#else
				IN_KeyEvent(0, event.key.state, qs, event.key.keysym.unicode);
#endif
			}
			break;
#ifdef HAVE_SDL_TEXTINPUT
		/*case SDL_TEXTEDITING:
			{
				event.edit;
			}
			break;*/
		case SDL_TEXTINPUT:
			{
				unsigned int uc;
				int err;
				const char *text = event.text.text;
				while(*text)
				{
					uc = utf8_decode(&err, text, &text);
					if (uc && !err)
					{
						IN_KeyEvent(0, true, 0, uc);
						IN_KeyEvent(0, false, 0, uc);
					}
				}
			}
			break;
#endif

#if SDL_MAJOR_VERSION >= 2
		case SDL_FINGERDOWN:
		case SDL_FINGERUP:
			{
				uint32_t thefinger = SDL_GiveFinger(-1, event.tfinger.touchId, event.tfinger.fingerId, event.type==SDL_FINGERUP);
				IN_MouseMove(thefinger, true, event.tfinger.x * vid.pixelwidth, event.tfinger.y * vid.pixelheight, 0, event.tfinger.pressure);
				IN_KeyEvent(thefinger, event.type==SDL_FINGERDOWN, K_TOUCH, 0);
			}
			break;
		case SDL_FINGERMOTION:
			{
				uint32_t thefinger = SDL_GiveFinger(-1, event.tfinger.touchId, event.tfinger.fingerId, false);
				IN_MouseMove(thefinger, true, event.tfinger.x * vid.pixelwidth, event.tfinger.y * vid.pixelheight, 0, event.tfinger.pressure);
			}
			break;

		case SDL_DROPFILE:
			Host_RunFile(event.drop.file, strlen(event.drop.file), NULL);
			SDL_free(event.drop.file);
			break;
#endif

		case SDL_MOUSEMOTION:
#if SDL_MAJOR_VERSION >= 2
			if (event.motion.which == SDL_TOUCH_MOUSEID)
				break;	//ignore legacy touch events.
#endif
			if (!mouseactive)
				IN_MouseMove(event.motion.which, true, event.motion.x, event.motion.y, 0, 0);
			else
				IN_MouseMove(event.motion.which, false, event.motion.xrel, event.motion.yrel, 0, 0);
			break;

#if SDL_MAJOR_VERSION >= 2
		case SDL_MOUSEWHEEL:
			if (event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED)
				event.wheel.y *= -1;
			for (; event.wheel.y > 0; event.wheel.y--)
			{
				IN_KeyEvent(event.button.which, true, K_MWHEELUP, 0);
				IN_KeyEvent(event.button.which, false, K_MWHEELUP, 0);
			}
			for (; event.wheel.y < 0; event.wheel.y++)
			{
				IN_KeyEvent(event.button.which, true, K_MWHEELDOWN, 0);
				IN_KeyEvent(event.button.which, false, K_MWHEELDOWN, 0);
			}
/*			for (; event.wheel.x > 0; event.wheel.x--)
			{
				IN_KeyEvent(event.button.which, true, K_MWHEELRIGHT, 0);
				IN_KeyEvent(event.button.which, false, K_MWHEELRIGHT, 0);
			}
			for (; event.wheel.x < 0; event.wheel.x++)
			{
				IN_KeyEvent(event.button.which, true, K_MWHEELLEFT, 0);
				IN_KeyEvent(event.button.which, false, K_MWHEELLEFT, 0);
			}*/
			break;
#endif

		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
#if SDL_MAJOR_VERSION >= 2
			if (event.button.which == SDL_TOUCH_MOUSEID)
				break;	//ignore legacy touch events. SDL_FINGER* events above will handle it (for multitouch)
#endif
			//Hmm. SDL allows for 255 buttons, but only defines 5...
			if (event.button.button > sizeof(tbl_sdltoquakemouse)/sizeof(tbl_sdltoquakemouse[0]))
				event.button.button = sizeof(tbl_sdltoquakemouse)/sizeof(tbl_sdltoquakemouse[0]);
			IN_KeyEvent(event.button.which, event.button.state, tbl_sdltoquakemouse[event.button.button-1], 0);
			break;

#if SDL_MAJOR_VERSION >= 2
		case SDL_APP_TERMINATING:
			Cbuf_AddText("quit force\n", RESTRICT_LOCAL);
			break;
#endif
		case SDL_QUIT:
			Cbuf_AddText("quit\n", RESTRICT_LOCAL);
			break;

#if SDL_MAJOR_VERSION >= 2
		//actually, joysticks *should* work with sdl1 as well, but there are some differences (like no hot plugging, I think).
		case SDL_JOYAXISMOTION:
			J_JoystickAxis(event.jaxis.which, event.jaxis.axis, event.jaxis.value);
			break;
//		case SDL_JOYBALLMOTION:
//		case SDL_JOYHATMOTION:
//			break;
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
			J_JoystickButton(event.jbutton.which, event.jbutton.button, event.type==SDL_JOYBUTTONDOWN);
			break;
		case SDL_JOYDEVICEADDED:
			J_JoystickAdded(event.jdevice.which);
			break;
		case SDL_JOYDEVICEREMOVED:
			J_Kill(event.jdevice.which, true);
			break;

		case SDL_CONTROLLERAXISMOTION:
			J_ControllerAxis(event.caxis.which, event.caxis.axis, event.caxis.value);
			break;
		case SDL_CONTROLLERBUTTONDOWN:
		case SDL_CONTROLLERBUTTONUP:
			J_ControllerButton(event.cbutton.which, event.cbutton.button, event.type==SDL_CONTROLLERBUTTONDOWN);
			break;
		case SDL_CONTROLLERDEVICEADDED:
			J_ControllerAdded(event.cdevice.which);
			break;
		case SDL_CONTROLLERDEVICEREMOVED:
			J_Kill(event.cdevice.which, true);
			break;
//		case SDL_CONTROLLERDEVICEREMAPPED:
//			break;
#if SDL_VERSION_ATLEAST(2,0,14)
		case SDL_CONTROLLERTOUCHPADDOWN:
			J_ControllerTouchPad(event.cdevice.which, event.ctouchpad.touchpad, event.ctouchpad.finger, 1, event.ctouchpad.x, event.ctouchpad.y, event.ctouchpad.pressure);
			break;
		case SDL_CONTROLLERTOUCHPADMOTION:
			J_ControllerTouchPad(event.cdevice.which, event.ctouchpad.touchpad, event.ctouchpad.finger, 0, event.ctouchpad.x, event.ctouchpad.y, event.ctouchpad.pressure);
			break;
		case SDL_CONTROLLERTOUCHPADUP:
			J_ControllerTouchPad(event.cdevice.which, event.ctouchpad.touchpad, event.ctouchpad.finger, -1, event.ctouchpad.x, event.ctouchpad.y, event.ctouchpad.pressure);
			break;
		case SDL_CONTROLLERSENSORUPDATE:
			J_ControllerSensor(event.csensor.which, event.csensor.sensor, event.csensor.data);
			break;
#endif
#endif
		}
	}

	for (j = 0; j < MAX_JOYSTICKS; j++)
	{
		struct sdljoy_s *joy = &sdljoy[j];
		if (joy->qdevid == DEVID_UNSET || !joy->controller)
			continue;

		if (joy->buttonheld)
		{
			for (axis = 0; axis < countof(gpbuttonmap); axis++)
			{
				if (joy->buttonheld & (1u<<axis))
				{
					joy->buttonrepeat[axis] -= host_frametime;
					if (joy->buttonrepeat[axis] < 0)
					{
						IN_KeyEvent(joy->qdevid, true, gpbuttonmap[axis], 0);
						joy->buttonrepeat[axis] = 0.25;
					}
				}
			}
		}

		for (axis = 0; axis < countof(gpaxismap); axis++)
		{
			if ((joy->axistobuttonp | joy->axistobuttonn) & (1u<<axis))
			{
				joy->repeattime[axis] -= host_frametime;
				if (joy->repeattime[axis] < 0)
				{
					if (joy->axistobuttonp & (1u<<axis))
					{
						IN_KeyEvent(joy->qdevid, true, gpaxismap[axis].pos, 0);
						joy->repeattime[axis] = 0.25;
					}
					if (joy->axistobuttonn & (1u<<axis))
					{
						IN_KeyEvent(joy->qdevid, true, gpaxismap[axis].neg, 0);
						joy->repeattime[axis] = 0.25;
					}
				}
			}
		}
	}
}






void INS_Shutdown (void)
{
	IN_DeactivateMouse();

#if SDL_MAJOR_VERSION >= 2
	J_KillAll();
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK|SDL_INIT_GAMECONTROLLER);
#endif
}

void INS_ReInit (void)
{
#if SDL_MAJOR_VERSION >= 2
	unsigned int i;
	memset(sdljoy, 0, sizeof(sdljoy));
	for (i = 0; i < MAX_JOYSTICKS; i++)
		sdljoy[i].qdevid = DEVID_UNSET;
	SDL_InitSubSystem(SDL_INIT_JOYSTICK|SDL_INIT_GAMECONTROLLER);
#endif

	IN_ActivateMouse();

#ifndef HAVE_SDL_TEXTINPUT
	SDL_EnableUNICODE(SDL_ENABLE);
#endif
#if SDL_MAJOR_VERSION >= 2
	SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
#endif
}

//stubs, all the work is done in Sys_SendKeyEvents
void INS_Move(void)
{
}
void INS_Init (void)
{
#ifdef HAVE_SDL_TEXTINPUT
	Cvar_Register(&sys_osk, "input controls");
#endif
	Cvar_Register (&joy_only, "input controls");

#ifdef HAVE_SDL_TEXTINPUT
#ifdef __linux__
	usesteamosk = SDL_GetHintBoolean("SteamDeck", false);	//looks like there's a 'SteamDeck=1' environment setting on the deck (when started via steam itself, at least), so we don't get valve's buggy-as-poop osk on windows etc.
#endif
#endif
}
void INS_Accumulate(void)	//input polling
{
}
void INS_Commands (void)	//used to Cbuf_AddText joystick button events in windows.
{
}
void INS_EnumerateDevices(void *ctx, void(*callback)(void *ctx, const char *type, const char *devicename, unsigned int *qdevid))
{
#if SDL_MAJOR_VERSION >= 2
	unsigned int i;
	for (i = 0; i < MAX_JOYSTICKS; i++)
		if (sdljoy[i].controller || sdljoy[i].joystick)
			callback(ctx, "joy", sdljoy[i].devname, &sdljoy[i].qdevid);
#endif
}


