//Generic input code.
//mostly mouse support, but can also handle a few keyboard events.

//Issues:

//VirtualBox mouse integration is bugged. X11 code can handle tablets, but VirtualBox sends mouse clicks on the ps/2 device instead.
//  you should be able to fix this with 'in_deviceids * 0 *', remapping both tablet+ps/2 to the same device id. 

//Android touchscreen inputs suck. should have some onscreen buttons, but they're still a bit poo or something. blame mods for not using csqc to do things themselves.

#include "quakedef.h"

extern qboolean mouse_active;

static cvar_t m_filter = CVARF("m_filter", "0", CVAR_ARCHIVE);
static cvar_t m_forcewheel = CVARD("m_forcewheel", "1", "0: ignore mousewheels in apis where it is abiguous.\n1: Use mousewheel when it is treated as a third axis. Motion above a threshold is ignored, to avoid issues with an unknown threshold.\n2: Like 1, but excess motion is retained. The threshold specifies exact z-axis distance per notice.");
static cvar_t m_forcewheel_threshold = CVARD("m_forcewheel_threshold", "32", "Mousewheel graduations smaller than this will not trigger mousewheel deltas.");
static cvar_t m_touchstrafe = CVARAFD("m_touchstrafe", "0", "m_strafeonright", CVAR_ARCHIVE, "0: entire screen changes angles only.\n1: right hand side controls strafing.\n2:left hand side strafes.");
//static cvar_t m_fatpressthreshold = CVARFD("m_fatpressthreshold", "0.2", CVAR_ARCHIVE, "How fat your thumb has to be to register a fat press (touchscreens).");
static cvar_t m_longpressthreshold = CVARFD("m_longpressthreshold", "1", CVAR_ARCHIVE, "How long to press for it to register as a long press (touchscreens).");
static cvar_t m_touchmajoraxis = CVARFD("m_touchmajoraxis", "1", CVAR_ARCHIVE, "When using a touchscreen, use only the major axis for strafing.");
static cvar_t m_slidethreshold = CVARFD("m_slidethreshold", "10", CVAR_ARCHIVE, "How far your finger needs to move to be considered a slide event (touchscreens).");

static cvar_t m_accel			= CVARAFD("m_accel",		"0",	"cl_mouseAccel", CVAR_ARCHIVE, "Values >0 will amplify mouse movement proportional to velocity. Small values have great effect. A lot of good Quake Live players use around the 0.1-0.2 mark, but this depends on your mouse CPI and polling rate.");
static cvar_t m_accel_style		= CVARAD("m_accel_style",	"1",	"cl_mouseAccelStyle",	"1 = Quake Live mouse acceleration, 0 = Old style accelertion.");
static cvar_t m_accel_power		= CVARAD("m_accel_power",	"2",	"cl_mouseAccelPower",	"Used when m_accel_style is 1.\nValues 1 or below are dumb. 2 is linear and the default. 99% of accel users use this. Above 2 begins to amplify exponentially and you will get more acceleration at higher velocities. Great if you want low accel for slow movements, and high accel for fast movements. Good in combination with a sensitivity cap.");
static cvar_t m_accel_offset	= CVARAD("m_accel_offset",	"0",	"cl_mouseAccelOffset",	"Used when m_accel_style is 1.\nAcceleration will not be active until the mouse movement exceeds this speed (counts per millisecond). Negative values are supported, which has the effect of causing higher rates of acceleration to happen at lower velocities.");
static cvar_t m_accel_senscap	= CVARAD("m_accel_senscap",	"0",	"cl_mouseSensCap",		"Used when m_accel_style is 1.\nSets an upper limit on the amplified mouse movement. Great for tuning acceleration around lower velocities while still remaining in control of fast motion such as flicking.");

void QDECL joyaxiscallback(cvar_t *var, char *oldvalue)
{
	int sign;
	char *end;
	sign = strtol(var->string, &end, 0);
	if (!*end)
	{
		//okay, its missing or an actual number.
		if (sign >= -6  &&  sign <= 6) {
			var->ival = sign;
		}
		return;
	}

	end = var->string;
	if (*end == '-')
	{
		end++;
		sign = -1;
	}
	else if (*end == '+')
	{
		end++;
		sign = 1;
	}
	else
		sign = 1;
	if (!Q_strcasecmp(end, "forward") || !Q_strcasecmp(end, "moveforward"))
		var->ival = 1*sign;
	else if (!Q_strcasecmp(end, "back") || !Q_strcasecmp(end, "moveback"))
		var->ival = 1*sign*-1;
	else if (!Q_strcasecmp(end, "lookup") || !Q_strcasecmp(end, "pitchup"))
		var->ival = 2*sign;
	else if (!Q_strcasecmp(end, "lookdown") || !Q_strcasecmp(end, "pitchdown"))
		var->ival = 2*sign*-1;
	else if (!Q_strcasecmp(end, "moveright"))
		var->ival = 3*sign;
	else if (!Q_strcasecmp(end, "moveleft"))
		var->ival = 3*sign*-1;
	else if (!Q_strcasecmp(end, "right") || !Q_strcasecmp(end, "turnright"))
		var->ival = 4*sign;
	else if (!Q_strcasecmp(end, "left") || !Q_strcasecmp(end, "turnleft"))
		var->ival = 4*sign*-1;
	else if (!Q_strcasecmp(end, "up") || !Q_strcasecmp(end, "moveup"))
		var->ival = 5*sign;
	else if (!Q_strcasecmp(end, "down") || !Q_strcasecmp(end, "movedown"))
		var->ival = 5*sign*-1;
	else if (!Q_strcasecmp(end, "rollright"))
		var->ival = 6*sign;
	else if (!Q_strcasecmp(end, "rollleft"))
		var->ival = 6*sign*-1;
}

static cvar_t	joy_advaxis[6] =
{
#define ADVAXISDESC (const char *)"Provides a way to remap each joystick/controller axis.\nShould be set to one of: moveforward, moveback, lookup, lookdown, turnleft, turnright, moveleft, moveright, moveup, movedown, rollleft, rollright"
	CVARCD("joyadvaxisx", "moveright", joyaxiscallback, ADVAXISDESC),	//left rightwards axis
	CVARCD("joyadvaxisy", "moveback", joyaxiscallback, ADVAXISDESC),	//left downwards axis
	CVARCD("joyadvaxisz", "", joyaxiscallback, ADVAXISDESC),			//typically left trigger (use it as a button)
	CVARCD("joyadvaxisr", "turnright", joyaxiscallback, ADVAXISDESC),	//right rightwards axis
	CVARCD("joyadvaxisu", "lookup", joyaxiscallback, ADVAXISDESC),		//right downwards axis
	CVARCD("joyadvaxisv", "", joyaxiscallback, ADVAXISDESC)				//typically right trigger
};
static cvar_t	joy_advaxisscale[6] =
{
#define ADVAXISSCALEDESC "Because joyadvaxisx etc can be added together, this provides a way to rescale or invert an individual axis without affecting another with the same action."
	CVARD("joyadvaxisx_scale", "1.0", ADVAXISSCALEDESC),
	CVARD("joyadvaxisy_scale", "1.0", ADVAXISSCALEDESC),
	CVARD("joyadvaxisz_scale", "1.0", ADVAXISSCALEDESC),
	CVARD("joyadvaxisr_scale", "1.0", ADVAXISSCALEDESC),
	CVARD("joyadvaxisu_scale", "1.0", ADVAXISSCALEDESC),
	CVARD("joyadvaxisv_scale", "1.0", ADVAXISSCALEDESC)
};
static cvar_t	joy_anglesens[3] =
{
#define ANGLESENSDESC "Scaler value for the controller when it is at its most extreme value"
	CVARD("joypitchsensitivity", "0.5", ANGLESENSDESC),
	CVARD("joyyawsensitivity", "1.0", ANGLESENSDESC),
	CVARD("joyrollsensitivity", "1.0", ANGLESENSDESC)
};
static cvar_t	joy_movesens[3] =
{
	CVAR("joyforwardsensitivity", "1.0"),
	CVAR("joysidesensitivity", "1.0"),
	CVAR("joyupsensitivity", "1.0")
};
//comments on threshholds comes from microsoft's xinput docs.
static cvar_t	joy_anglethreshold[3] =
{
#define ANGLETHRESHOLDDESC "Values reported near the center of the analog joystick/controller are often erroneous and undesired.\nThe joystick threshholds are how much of the total values to ignore."
	CVARD("joypitchthreshold", "0.19", ANGLETHRESHOLDDESC),	//8689/32767 (right thumb)
	CVARD("joyyawthreshold", "0.19", ANGLETHRESHOLDDESC),	//8689/32767 (right thumb)
	CVARD("joyrollthreshold", "0.118", ANGLETHRESHOLDDESC),	//30/255	 (trigger)
};
static cvar_t	joy_movethreshold[3] =
{
	CVAR("joyforwardthreshold", "0.17"),//7849/32767 (left thumb)
	CVAR("joysidethreshold", "0.17"),	//7849/32767 (left thumb)
	CVAR("joyupthreshold", "0.118"),	//30/255	 (trigger)
};

static cvar_t joy_exponent = CVARD("joyexponent", "1", "Scales joystick/controller sensitivity non-linearly to increase precision in the center.\nA value of 1 is linear.");

#if defined(__linux__) && defined(FTE_SDL)
#ifdef FTE_SDL3
#include <SDL3/SDL.h>
#else
#include <SDL.h>
#endif
void joy_radialdeadzone_cb(cvar_t *var, char *oldvalue)
{
	if (!*var->string)
	{
#if SDL_VERSION_ATLEAST(3,0,0)
		if (SDL_GetHintBoolean(SDL_HINT_JOYSTICK_LINUX_DEADZONES, true))
#else
		if (SDL_GetHintBoolean(SDL_HINT_LINUX_JOYSTICK_DEADZONES, true))
#endif
			var->ival = 2;	//sdl2 provides its own deadzones on linux, by default.
		else
			var->ival = 1;
	}
}
#else
void joy_radialdeadzone_cb(cvar_t *var, char *oldvalue)
{
	if (!*var->string)
		var->ival = 1;
}
#endif
static cvar_t joy_radialdeadzone = CVARCD("joyradialdeadzone", "", joy_radialdeadzone_cb, "Treat controller dead zones as a pair, rather than per-axis.\n0: treat joystick axis independantly (square).\n1: treat the axis together, radially.\n2: do not handle deadzones (prefiltered).");

cvar_t in_skipplayerone = CVARD("in_skipplayerone", "1", "Do not auto-assign joysticks/game-controllers to the first player. Requires in_restart to take effect.");	//FIXME: this needs to be able to change deviceids when changed. until then menus will need to in_restart.


#define EVENTQUEUELENGTH 1024
static struct eventlist_s
{
	enum
	{
		IEV_KEYDOWN,
		IEV_KEYRELEASE,
		IEV_MOUSEABS,
		IEV_MOUSEDELTA,
		IEV_JOYAXIS,
		IEV_ACCELEROMETER,
		IEV_GYROSCOPE,
	} type;
	unsigned int devid;

	union
	{
		struct
		{
			float x, y, z;
			float tsize;	//the size of the touch
		} mouse;
		struct
		{
			int scancode, unicode;
		} keyboard;
		struct
		{
			int axis;
			float value;
		} joy;
		struct
		{	//metres per second, ish.
			float x, y, z;
		} accel;
		struct
		{	//these are in radians, not degrees.
			float pitch, yaw, roll;
		} gyro;
	};
} eventlist[EVENTQUEUELENGTH];
static volatile int events_avail; /*volatile to make sure the cc doesn't try leaving these cached in a register*/
static volatile int events_used;

static struct eventlist_s *in_newevent(void)
{
	if (events_avail >= events_used + EVENTQUEUELENGTH)
		return NULL;
	return &eventlist[events_avail & (EVENTQUEUELENGTH-1)];
}

static void in_finishevent(void)
{
	events_avail++;
}

#define MAXPOINTERS 8
static struct mouse_s
{
	enum
	{
		M_INVALID,
		M_MOUSE,	//using deltas
		M_TOUCH		//using absolutes
		//the only functional difference between touch and tablets is that tablets should draw the cursor when hovering too.
	} type;
	unsigned int qdeviceid;	//so we can just use pointers.
	vec2_t oldpos;		//last-known-cursor-position
	vec2_t heldpos;		//position the cursor was at when the button was held (touch-start pos)
	float moveddist;	//how far it has moved while held. this provides us with our emulated mouse1 when they release the press
	vec2_t delta;		//how far its moved recently
	vec2_t old_delta;	//how far its moved previously, for mouse smoothing
	float wheeldelta;
	double touchtime;	//0 when not touching, otherwise start time of touch.
	unsigned int touchkey;	//which other key we generated (so it gets released again.
	unsigned int updates;	//tracks updates per second
	qboolean updated;
} ptr[MAXPOINTERS];
static int touchcursor = -1;	//the cursor follows whichever finger was most recently pressed in preference to any mouse also on the same system

#define MAXJOYAXIS 6
#define MAXJOYSTICKS 8
static struct joy_s
{
	unsigned int qdeviceid;
	float axis[MAXJOYAXIS];
} joy[MAXJOYSTICKS];

void IN_Shutdown(void)
{
	INS_Shutdown();
}

void IN_ReInit(void)
{
	int i;

	for (i = 0; i < MAXPOINTERS; i++)
	{
		memset(&ptr[i], 0, sizeof(ptr[i]));
		ptr[i].type = M_INVALID;
		ptr[i].qdeviceid = i;
	}

	for (i = 0; i < MAXJOYSTICKS; i++)
	{
		memset(&joy[i], 0, sizeof(joy[i]));
		joy[i].qdeviceid = i;
	}

	INS_ReInit();
}

struct remapctx
{
	char *type;
	char *devicename;
	unsigned int newdevid;
	unsigned int found;
	unsigned int failed;
};
static void IN_DeviceIDs_DoRemap(void *vctx, const char *type, const char *devicename, unsigned int *qdevid)
{
	struct remapctx *ctx = vctx;

	if (!strcmp(ctx->type, type) || !strcmp(ctx->type, "*"))
		if (!strcmp(ctx->devicename, devicename) || !strcmp(ctx->devicename, "*"))
		{
			if (qdevid)
				*qdevid = ctx->newdevid;
			else
				ctx->failed = true;
			ctx->found++;
		}
}
void IN_DeviceIDs_Enumerate(void *ctx, const char *type, const char *devicename, unsigned int *qdevid)
{
	char buf[8192];
	devicename = COM_QuotedString(devicename, buf, sizeof(buf), false);
	if (!qdevid)
		Con_Printf("%s\t%s\t%s\n", type, "N/A", devicename);
	else if (*qdevid == DEVID_UNSET)
		Con_Printf("%s\t%s\t%s\n", type, "Unset", devicename);
	else
		Con_Printf("%s\t%u\t%s\n", type, *qdevid, devicename);
}

void IN_DeviceIDs_f(void)
{
	struct remapctx ctx;

	if (Cmd_Argc() > 3)
	{
		ctx.failed = false;
		ctx.found = 0;
		ctx.type = Cmd_Argv(1);
		ctx.newdevid = strtoul(Cmd_Argv(2), NULL, 0);
		ctx.devicename = Cmd_Argv(3);
		INS_EnumerateDevices(&ctx, IN_DeviceIDs_DoRemap);

		if (ctx.failed)
			Con_Printf("device cannot be remapped\n");
		else if (!ctx.found)
			Con_Printf("%s \"%s\" not known\n", ctx.type, ctx.devicename);
		else if (!cl_warncmd.ival)
			Con_Printf("device remapped\n");
	}
	else if (Cmd_Argc() > 1)
	{
		Con_Printf("%s TYPE NEWID DEVICENAME\n", Cmd_Argv(0));
	}
	else
	{
		Con_Printf("Type\tMapping\tName\n");
		INS_EnumerateDevices(NULL, IN_DeviceIDs_Enumerate);
	}
}

float IN_DetermineMouseRate(void)
{
	double time = Sys_DoubleTime();
	static double timer;
	static float last;
	float interval = time - timer;
	if (fabs(interval) >= 1)
	{
		timer = time;
		last = ptr[0].updates/interval;
		ptr[0].updates = 0;
	}
	return last;
}

void IN_Init(void)
{
	int i;
	events_avail = 0;
	events_used = 0;

	Cvar_Register (&m_filter, "input controls");
	Cvar_Register (&m_forcewheel, "Input Controls");
	Cvar_Register (&m_forcewheel_threshold, "Input Controls");
	Cvar_Register (&m_touchstrafe, "input controls");
	Cvar_Register (&m_longpressthreshold, "input controls");
	Cvar_Register (&m_slidethreshold, "input controls");
	Cvar_Register (&m_touchmajoraxis, "input controls");
	Cvar_Register (&m_accel, "input controls");
	Cvar_Register (&m_accel_style, "input controls");
	Cvar_Register (&m_accel_power, "input controls");
	Cvar_Register (&m_accel_offset, "input controls");
	Cvar_Register (&m_accel_senscap, "input controls");

	for (i = 0; i < 6; i++)
	{
		Cvar_Register (&joy_advaxis[i], "input controls");
		Cvar_Register (&joy_advaxisscale[i], "input controls");

		Cvar_ForceCallback(&joy_advaxis[i]);
	}
	for (i = 0; i < 3; i++)
	{
		Cvar_Register (&joy_anglesens[i], "input controls");
		Cvar_Register (&joy_movesens[i], "input controls");
		Cvar_Register (&joy_anglethreshold[i], "input controls");
		Cvar_Register (&joy_movethreshold[i], "input controls");
	}
	Cvar_Register (&joy_exponent, "input controls");
	Cvar_Register (&joy_radialdeadzone, "input controls");
	Cvar_Register (&in_skipplayerone, "input controls");

	Cmd_AddCommand ("in_deviceids", IN_DeviceIDs_f);

	INS_Init();
}

//there was no ui to click on at least...
//translates touch press events into ones that are actually bound, according to touchstrafe and position.
int IN_Touch_Fallback(unsigned int devid)
{
	int ret;
	if (devid >= countof(ptr))
		ret = 0;
	else switch(m_touchstrafe.ival)	//translate touch to mouse2 if its on the strafing side of the screen.
	{
	case 2:
		ret = ptr[devid].heldpos[0] < vid.pixelwidth/2;
		break;
	default:
		ret = ptr[devid].heldpos[0] > vid.pixelwidth/2;
		break;
	case 0:
		ret = false;
		break;
	}
	ret = ret?K_MOUSE2:K_MOUSE1;

	return ret;
}
void IN_Touch_BlockGestures(unsigned int devid)
{	//called via K_TOUCH, blocks K_TOUCHTAP etc gestures
	if (devid < countof(ptr))
		ptr[devid].touchkey = 0;	//block it all.
}
qboolean IN_Touch_MouseIsAbs(unsigned int devid)
{	//lets the caller know if a mouse1 down event was abs
	if (devid < countof(ptr))
		return ptr[devid].type == M_TOUCH;
	return false;
}

/*a 'pointer' is either a multitouch pointer, or a separate device
note that mice use the keyboard button api, but separate devices*/
void IN_Commands(void)
{
	struct eventlist_s *ev;

	INS_Commands();

	while (events_used != events_avail)
	{
		ev = &eventlist[events_used & (EVENTQUEUELENGTH-1)];

		switch(ev->type)
		{
		case IEV_KEYRELEASE:
//Con_Printf("IEV_KEYDOWN %i: %i '%c'\n", ev->devid, ev->keyboard.scancode, ev->keyboard.unicode?ev->keyboard.unicode:' ');
			if (ev->keyboard.scancode == -1)
			{
				int i;
				for (i = 0; i < K_MAX; i++)
					Key_Event(ev->devid, i, 0, false);
				break;
			}
			if ((ev->keyboard.scancode == K_MOUSE1||ev->keyboard.scancode==K_TOUCH) && ev->devid < MAXPOINTERS && (ptr[ev->devid].type == M_TOUCH))
			{	//touch (or abs clicks)
				struct mouse_s *m = &ptr[ev->devid];
				if (touchcursor == ev->devid)
					touchcursor = -1;	//revert it to the mouse, or whatever device was 0.

				if (ev->keyboard.scancode==K_TOUCH && m->touchtime && m->touchkey==K_TOUCH)
				{	//convert to tap/longpress if it wasn't already a gesture. don't ever convert mouse.
					if (Sys_DoubleTime()-m->touchtime > m_longpressthreshold.value)
						m->touchkey = K_TOUCHLONG;
					else
						m->touchkey = K_TOUCHTAP;
					Key_Event(ev->devid, m->touchkey, 0, true);
				}
				if (m->touchkey && m->touchkey!=ev->keyboard.scancode)
					Key_Event(ev->devid, m->touchkey, 0, false);	//and release...

				//reset it.
				m->touchkey = 0;
				m->touchtime = 0;
				m->moveddist = 0;
			}
			Key_Event(ev->devid, ev->keyboard.scancode, ev->keyboard.unicode, false);
			break;
		case IEV_KEYDOWN:
//Con_Printf("IEV_KEYDOWN %i: %i '%c'\n", ev->devid, ev->keyboard.scancode, ev->keyboard.unicode?ev->keyboard.unicode:' ');
			if ((ev->keyboard.scancode == K_MOUSE1||ev->keyboard.scancode==K_TOUCH) && ev->devid < MAXPOINTERS && (ptr[ev->devid].type == M_TOUCH))
			{	//touch (or abs clicks)
				struct mouse_s *m = &ptr[ev->devid];
				float fl;
				touchcursor = ev->devid;
				fl = m->oldpos[0] * vid.width / vid.pixelwidth;		mousecursor_x = bound(0, fl, vid.width-1);
				fl = m->oldpos[1] * vid.height / vid.pixelheight;	mousecursor_y = bound(0, fl, vid.height-1);

				m->touchtime = Sys_DoubleTime();
				m->moveddist = 0;
				if (ev->keyboard.scancode==K_TOUCH)
					m->touchkey = K_TOUCH;
				else
					m->touchkey = 0;
			}
			Key_Event(ev->devid, ev->keyboard.scancode, ev->keyboard.unicode, true);
			break;
		case IEV_JOYAXIS:
			if (ev->devid < MAXJOYSTICKS && ev->joy.axis>=0 && ev->joy.axis<MAXJOYAXIS)
			{
				if (topmenu && topmenu->joyaxis && topmenu->joyaxis(topmenu, ev->devid, ev->joy.axis, ev->joy.value))
					joy[ev->devid].axis[ev->joy.axis] = 0;
#ifdef CSQC_DAT
				else if (CSQC_JoystickAxis(ev->joy.axis, ev->joy.value, ev->devid))
					joy[ev->devid].axis[ev->joy.axis] = 0;
#endif
				else
					joy[ev->devid].axis[ev->joy.axis] = ev->joy.value;
			}
			break;
		case IEV_MOUSEDELTA:
//Con_Printf("IEV_MOUSEDELTA %i: %f %f\n", ev->devid, ev->mouse.x, ev->mouse.y);
			if (ev->devid < MAXPOINTERS)
			{
				if (ev->mouse.x || ev->mouse.y)
					ptr[ev->devid].updated = true;
				if (ptr[ev->devid].type != M_MOUSE)
				{
					ptr[ev->devid].type = M_MOUSE;
				}
				ptr[ev->devid].delta[0] += ev->mouse.x;
				ptr[ev->devid].delta[1] += ev->mouse.y;

				//if we're emulating a cursor, make sure that's updated too.
				if (touchcursor < 0 && !vrui.enabled && Key_MouseShouldBeFree())
				{
					mousecursor_x += ev->mouse.x;
					mousecursor_y += ev->mouse.y;
					mousecursor_x = bound(0, mousecursor_x, vid.pixelwidth-1);
					mousecursor_y = bound(0, mousecursor_y, vid.pixelheight-1);
				}
				ptr[ev->devid].oldpos[0] = mousecursor_x;
				ptr[ev->devid].oldpos[1] = mousecursor_y;

				if (m_forcewheel.value >= 2)
					ptr[ev->devid].wheeldelta -= ev->mouse.z;
				else if (m_forcewheel.value)
				{
					int mfwt = (int)m_forcewheel_threshold.value;

					if (ev->mouse.z > mfwt)
						ptr[ev->devid].wheeldelta -= mfwt;
					else if (ev->mouse.z < -mfwt)
						ptr[ev->devid].wheeldelta += mfwt;
				}

				if (ev->mouse.x || ev->mouse.y)
					ptr[ev->devid].updates++;
			}
			break;
		case IEV_MOUSEABS:
//Con_Printf("IEV_MOUSEABS %i: %f %f\n", ev->devid, ev->mouse.x, ev->mouse.y);
			/*mouse cursors only really work with one pointer*/
			if (!vrui.enabled)
			if (ev->devid == touchcursor || (touchcursor < 0 && ev->devid < MAXPOINTERS && (ptr[ev->devid].oldpos[0] != ev->mouse.x || ptr[ev->devid].oldpos[1] != ev->mouse.y)))
			{
				float fl;
				fl = ev->mouse.x * vid.width / vid.pixelwidth;
				mousecursor_x = bound(0, fl, vid.width-1);
				fl = ev->mouse.y * vid.height / vid.pixelheight;
				mousecursor_y = bound(0, fl, vid.height-1);
			}

			if (ev->devid < MAXPOINTERS)
			{
				struct mouse_s *m = &ptr[ev->devid];
				if (m->type != M_TOUCH)
				{
					//if its now become an absolute device, clear stuff so we don't get confused.
					m->type = M_TOUCH;
					m->touchtime = 0;
					m->touchkey = 0;
					m->moveddist = 0;
					m->oldpos[0] = ev->mouse.x;
					m->oldpos[1] = ev->mouse.y;
				}

				if (m->touchtime)
				{	//only do this when its actually held in some form...
					m->delta[0] += ev->mouse.x - m->oldpos[0];
					m->delta[1] += ev->mouse.y - m->oldpos[1];
		
					m->moveddist += fabs(ev->mouse.x - m->oldpos[0]) + fabs(ev->mouse.y - m->oldpos[1]);
				}

				if (ev->mouse.x != m->oldpos[0] ||
					ev->mouse.y != m->oldpos[1])
				{
					m->updates++;
					m->updated = true;
				}

				m->oldpos[0] = ev->mouse.x;
				m->oldpos[1] = ev->mouse.y;

				if (m->touchtime && m->touchkey == K_TOUCH)
				{
					if (Sys_DoubleTime()-m->touchtime > 1)
						m->touchkey = K_TOUCHLONG;	//held for long enough...
					else if (m->moveddist >= m_slidethreshold.value)
						m->touchkey = K_TOUCHSLIDE;	//moved far enough to consitute a slide
					else
						break;
					Key_Event(ev->devid, m->touchkey, 0, true);
				}
			}
			break;

		case IEV_ACCELEROMETER:
			//down: x= +9.8
			//left: y= -9.8
			//up:   z= +9.8
#ifdef CSQC_DAT
			CSQC_Accelerometer(ev->accel.x, ev->accel.y, ev->accel.z);
#endif
			break;
		case IEV_GYROSCOPE:
#ifdef CSQC_DAT
			CSQC_Gyroscope(ev->gyro.pitch * 180.0/M_PI, ev->gyro.yaw * 180.0/M_PI, ev->gyro.roll * 180.0/M_PI);
#endif
			break;
		}
		events_used++;
	}
}

void IN_MoveMouse(struct mouse_s *mouse, float *movements, int pnum, float frametime)
{
	float mx, my;
	double mouse_x, mouse_y, mouse_deltadist;
	int mfwt;
	qboolean strafe_x, strafe_y;
	int wpnum;
#ifdef CSQC_DAT
#ifdef MULTITHREAD
	extern qboolean runningindepphys;
#else
	const qboolean runningindepphys = false;
#endif
#endif

	//small performance boost
	if (mouse->type == M_INVALID)
		return;

	/*each device will be processed when its player comes to be processed*/
	wpnum = cl.splitclients;
	if (wpnum < 1)
		wpnum = 1;
	if (cl_forceseat.ival)
		wpnum = (cl_forceseat.ival-1) % wpnum;
	else
		wpnum = mouse->qdeviceid % wpnum;
	if (wpnum != pnum)
		return;

	if (m_forcewheel.value)
	{
		mfwt = m_forcewheel_threshold.ival;
		if (mfwt)
		{
			while(mouse->wheeldelta <= -mfwt)
			{
				Key_Event (mouse->qdeviceid, K_MWHEELUP, 0, true);
				Key_Event (mouse->qdeviceid, K_MWHEELUP, 0, false);
				mouse->wheeldelta += mfwt;
			}

			while(mouse->wheeldelta >= mfwt)
			{
				Key_Event (mouse->qdeviceid, K_MWHEELDOWN, 0, true);
				Key_Event (mouse->qdeviceid, K_MWHEELDOWN, 0, false);
				mouse->wheeldelta -= mfwt;
			}
		}

		if (m_forcewheel.value < 2)
			mouse->wheeldelta = 0;
	}

	mx = mouse->delta[0];
	mouse->delta[0]=0;
	my = mouse->delta[1];
	mouse->delta[1]=0;


	if(in_xflip.value) mx *= -1;

	mousemove_x += mx;
	mousemove_y += my;

	if (!vrui.enabled && Key_MouseShouldBeFree())
		mx=my=0;

	if (mouse->type == M_TOUCH)
	{
		qboolean strafing = false;

		if (mouse->touchtime && mouse->touchkey==K_TOUCH)
		{	//convert to tap/longpress if it wasn't already a gesture. don't ever convert mouse.
			if (Sys_DoubleTime()-mouse->touchtime > m_longpressthreshold.value)
			{	//might as well trigger this here...
				mouse->touchkey = K_TOUCHLONG;
				Key_Event(mouse-ptr, mouse->touchkey, 0, true);
			}
		}


		switch(m_touchstrafe.ival)
		{
		case 2:
			strafing = mouse->heldpos[0] < vid.pixelwidth/2;
			break;
		default:
			strafing = mouse->heldpos[0] > vid.pixelwidth/2;
			break;
		case 0:
			strafing = false;
			break;
		}
		if (strafing && movements != NULL && !Key_Dest_Has(~kdm_game))
		{
			//if they're strafing, calculate the speed to move at based upon their displacement
			if (mouse->touchtime)
			{
				if (m_touchstrafe.ival == 2)	//left side
					mx = mouse->oldpos[0] - (vid.pixelwidth*1)/4.0;
				else	//right side
					mx = mouse->oldpos[0] - (vid.pixelwidth*3)/4.0;
				my = mouse->oldpos[1] - (vid.pixelheight*3)/4.0;

				//mx = (mouse->oldpos[0] - mouse->heldpos[0])*0.1;
				//my = (mouse->oldpos[1] - mouse->heldpos[1])*0.1;
			}
			else
			{
				mx = 0;
				my = 0;
			}

			if (m_touchmajoraxis.ival)
			{
				//major axis only
				if (fabs(mx) > fabs(my))
					my = 0;
				else
					mx = 0;
			}

			strafe_x = true;
			strafe_y = true;
		}
		else
		{
			strafe_x = false;
			strafe_y = false;

			//boost sensitivity so that the default works okay.
			mx *= 1.75;
			my *= 1.75;

#ifdef QUAKESTATS
			if (IN_WeaponWheelAccumulate(pnum, mx, my, 0))
				mx = my = 0;
#endif
		}
	}
	else
	{
		strafe_x = (in_strafe.state[pnum] & 1) || (lookstrafe.value && (in_mlook.state[pnum] & 1) );
		strafe_y = !((in_mlook.state[pnum] & 1) && !(in_strafe.state[pnum] & 1));
	}

	if (mouse->type == M_TOUCH || Key_MouseShouldBeFree())
	{
		if (mouse->updated)
		{	//many mods assume only a single mouse device.
			//when we have multiple active abs devices, we need to avoid sending all of them, because that just confuses everyone. such mods will see only devices that are actually moving, so uni-cursor mods will see only the one that moved most recently.
			mouse->updated = false;
			if (!runningindepphys)
			{
				if ((promptmenu && promptmenu->mousemove && promptmenu->mousemove(topmenu, true, mouse->qdeviceid, mouse->oldpos[0], mouse->oldpos[1])) ||
					(topmenu && topmenu->mousemove && topmenu->mousemove(topmenu, true, mouse->qdeviceid, mouse->oldpos[0], mouse->oldpos[1])))
				{
					mx = 0;
					my = 0;
				}
#ifdef CSQC_DAT
				if (!runningindepphys && CSQC_MousePosition(mouse->oldpos[0], mouse->oldpos[1], mouse->qdeviceid))
				{
					mx = 0;
					my = 0;
				}
#endif
			}
		}
	}
	else
	{
		if (Key_Dest_Has(kdm_menu))
		if (mx || my)
		if (!runningindepphys && topmenu && topmenu->mousemove && topmenu->mousemove(topmenu, false, mouse->qdeviceid, mx, my))
		{
			mx = 0;
			my = 0;
		}

#ifdef CSQC_DAT
		if (mx || my)
		if (!runningindepphys && CSQC_MouseMove(mx, my, mouse->qdeviceid))
		{
			mx = 0;
			my = 0;
		}
#endif

		//if game is not focused, kill any mouse look
		if (
#ifdef QUAKESTATS
			IN_WeaponWheelAccumulate(pnum, mx, my, 0) ||
#endif
			Key_Dest_Has(~kdm_game))
		{
			mx = 0;
			my = 0;
		}
	}

	if (m_filter.value)
	{
		double fraction = bound(0, m_filter.value, 2) * 0.5;
		mouse_x = (mx*(1-fraction) + mouse->old_delta[0]*fraction);
		mouse_y = (my*(1-fraction) + mouse->old_delta[1]*fraction);
	}
	else
	{
		mouse_x = mx;
		mouse_y = my;
	}

	mouse->old_delta[0] = mx;
	mouse->old_delta[1] = my;

	if (m_accel.value)
	{
		if (m_accel_style.ival && frametime)
		{
			float accelsens = sensitivity.value*in_sensitivityscale;
			float mousespeed = (sqrt (mx * mx + my * my)) / (1000.0f * (float) frametime);
			mousespeed -= m_accel_offset.value;
			if (mousespeed > 0)
			{
				mousespeed *= m_accel.value;
				if (m_accel_power.value > 1)
					accelsens += exp((m_accel_power.value - 1) * log(mousespeed));
				else
					accelsens = 1;
			}
			if (m_accel_senscap.value > 0 && accelsens > m_accel_senscap.value)
				accelsens = m_accel_senscap.value;
			mouse_x *= accelsens;
			mouse_y *= accelsens;
		}
		else
		{
			mouse_deltadist = sqrt(mx*mx + my*my);
			mouse_x *= (mouse_deltadist*m_accel.value + sensitivity.value*in_sensitivityscale);
			mouse_y *= (mouse_deltadist*m_accel.value + sensitivity.value*in_sensitivityscale);
		}
	}
	else
	{
		mouse_x *= sensitivity.value*in_sensitivityscale;
		mouse_y *= sensitivity.value*in_sensitivityscale;
	}

/*
#ifdef QUAKESTATS
	if (cl.playerview[pnum].statsf[STAT_VIEWZOOM])
	{
		mouse_x *= cl.playerview[pnum].statsf[STAT_VIEWZOOM]/STAT_VIEWZOOM_SCALE;
		mouse_y *= cl.playerview[pnum].statsf[STAT_VIEWZOOM]/STAT_VIEWZOOM_SCALE;
	}
#endif
*/

	if (!movements || cl.disablemouse)
	{
		return;
	}

	if (r_xflip.ival)
		mouse_x *= -1;

// add mouse X/Y movement to cmd
	if (strafe_x)
		movements[1] += m_side.value * mouse_x;
	else
	{
//		if ((int)((cl.viewangles[pnum][PITCH]+89.99)/180) & 1)
//			mouse_x *= -1;
		cl.playerview[pnum].viewanglechange[YAW] -= m_yaw.value * mouse_x;
	}

	if (in_mlook.state[pnum] & 1)
		V_StopPitchDrift (&cl.playerview[pnum]);

	if (!strafe_y)
	{
		cl.playerview[pnum].viewanglechange[PITCH] += m_pitch.value * mouse_y;
	}
	else
	{
		if ((in_strafe.state[pnum] & 1) && noclip_anglehack)
			movements[2] -= m_forward.value * mouse_y;
		else
			movements[0] -= m_forward.value * mouse_y;
	}
}

//rescales threshold-1 down 0-1
static float joydeadzone(float mag, float deadzone)
{
	if (joy_radialdeadzone.ival == 2)	//hacky overload to disable dead zones where the system provides it instead.
		deadzone = 0;

	if (mag > 1)	//erg?
		mag = 1;
	if (mag > deadzone)
	{
		mag -= deadzone;
		mag = mag / (1.f-deadzone);

		mag = pow(mag, joy_exponent.value);
	}
	else
		mag = 0;
	return mag;
}

void IN_MoveJoystick(struct joy_s *joy, float *movements, int pnum, float frametime)
{
	float mag;
	vec3_t jlook, jstrafe;

	int wpnum, i;
	for (i = 0; i < MAXJOYAXIS; i++)
		if (joy->axis[i])
			break;
	if (i == MAXJOYAXIS)
		return;

	/*each device will be processed when its player comes to be processed*/
	wpnum = cl.splitclients;
	if (wpnum < 1)
		wpnum = 1;
	if (cl_forceseat.ival)
		wpnum = (cl_forceseat.ival-1) % wpnum;
	else
		wpnum = joy->qdeviceid % wpnum;
	if (wpnum != pnum)
		return;

	memset(jstrafe, 0, sizeof(jstrafe));
	memset(jlook, 0, sizeof(jlook));

	for (i = 0; i < 6; i++)
	{
		int ax = joy_advaxis[i].ival;
		switch(ax)
		{
		default:
		case 0:	//dead axis
			break;
		case 1:
		case 3:
		case 5:
			jstrafe[(ax-1)/2] += joy->axis[i] * joy_advaxisscale[i].value;
			break;
		case -1:
		case -3:
		case -5:
			jstrafe[(-ax-1)/2] -= joy->axis[i] * joy_advaxisscale[i].value;
			break;

		case 2:
		case 4:
		case 6:
			jlook[(ax-2)/2] += joy->axis[i] * joy_advaxisscale[i].value;
			break;
		case -2:
		case -4:
		case -6:
			jlook[(-ax-2)/2] -= joy->axis[i] * joy_advaxisscale[i].value;
			break;
		}
	}

	if (joy_radialdeadzone.ival)
	{
		//uses a radial deadzone for x+y axis, and separate out the z axis, just because most controllers are 2d affairs with any 3rd axis being a separate knob.
		//deadzone values are stolen from microsoft's xinput documentation. they seem quite large to me - I guess that means that xbox controllers are just dodgy imprecise crap with excessive amounts of friction and finger grease.
		float basemag = sqrt(jlook[0]*jlook[0] + jlook[1]*jlook[1]);
		if (basemag)
		{
			mag = joydeadzone(basemag, sqrt(joy_anglethreshold[0].value*joy_anglethreshold[0].value + joy_anglethreshold[1].value*joy_anglethreshold[1].value));
			jlook[0] = (jlook[0]/basemag) * mag;
			jlook[1] = (jlook[1]/basemag) * mag;
		}
		else
			jlook[0] = jlook[1] = 0;
		mag = joydeadzone(fabs(jlook[2]), joy_anglethreshold[2].value);
		jlook[2] = mag;

		basemag = sqrt(jstrafe[0]*jstrafe[0] + jstrafe[1]*jstrafe[1]);
		if (basemag)
		{
			mag = joydeadzone(basemag, sqrt(joy_movethreshold[0].value*joy_movethreshold[0].value + joy_movethreshold[1].value*joy_movethreshold[1].value));
			jstrafe[0] = (jstrafe[0]/basemag) * mag;
			jstrafe[1] = (jstrafe[1]/basemag) * mag;
		}
		else
			jstrafe[0] = jstrafe[1] = 0;
		mag = joydeadzone(fabs(jstrafe[2]), joy_movethreshold[2].value);
		jstrafe[2] = mag;
	}
	else
	{
		for (i = 0; i < 3; i++)
		{
			mag = joydeadzone(fabs(jlook[i]), joy_anglethreshold[i].value);
			jlook[i] = ((jlook[i]<0)?-1:1)*mag;

			mag = joydeadzone(fabs(jstrafe[i]), joy_movethreshold[i].value);
			jstrafe[i] = ((jstrafe[i]<0)?-1:1)*mag;
		}
	}

#ifdef QUAKESTATS
	if (IN_WeaponWheelAccumulate(joy->qdeviceid, jstrafe[1]*50, -jstrafe[0]*50, 20))
		jstrafe[0] = jstrafe[1] = 0;
	if (IN_WeaponWheelAccumulate(joy->qdeviceid, jlook[1]*50, jlook[0]*50, 20))
		jlook[0] = jlook[1] = 0;
#endif

	if (Key_Dest_Has(~kdm_game))
	{
		VectorClear(jlook);
		VectorClear(jstrafe);
	}
	if (r_xflip.ival)
		jlook[0] *= -1, jstrafe[0] *= -1;

	if (in_speed.state[pnum] & 1)
	{
		VectorScale(jlook, 360*cl_movespeedkey.value, jlook);
		VectorScale(jstrafe, 360*cl_movespeedkey.value, jstrafe);
	}
	VectorScale(jlook, 360*frametime*in_sensitivityscale, jlook);

	if (!movements)	//if this is null, gamecode should still get inputs, just no camera looking or anything.
		return;

	//angle changes
	cl.playerview[pnum].viewanglechange[PITCH] += joy_anglesens[0].value * jlook[0] * in_sensitivityscale;
	cl.playerview[pnum].viewanglechange[YAW] -= joy_anglesens[1].value * jlook[1] * in_sensitivityscale;
	cl.playerview[pnum].viewanglechange[ROLL] += joy_anglesens[2].value * jlook[2] * in_sensitivityscale;

	if (in_mlook.state[pnum] & 1)
		V_StopPitchDrift (&cl.playerview[pnum]);

	//movement
	mag = 1;
	if ((in_speed.state[pnum] & 1) ^ cl_run.ival)
		mag *= cl_movespeedkey.value;
	movements[0] += joy_movesens[0].value * mag*cl_forwardspeed.value * jstrafe[0];
	movements[1] += joy_movesens[1].value * mag*cl_sidespeed.value * jstrafe[1];
	movements[2] += joy_movesens[2].value * mag*cl_upspeed.value * jstrafe[2];
}

void IN_Move (float *nudgemovements, float *absmovements, int pnum, float frametime)
{
	int i;
	for (i = 0; i < MAXPOINTERS; i++)
		IN_MoveMouse(&ptr[i], nudgemovements, pnum, frametime);

	for (i = 0; i < MAXJOYSTICKS; i++)
		IN_MoveJoystick(&joy[i], absmovements, pnum, frametime);
}

void IN_JoystickAxisEvent(unsigned int devid, int axis, float value)
{
	struct eventlist_s *ev = in_newevent();
	if (!ev)	
		return;
	ev->type = IEV_JOYAXIS;
	ev->devid = devid;
	ev->joy.axis = axis;
	ev->joy.value = value;
	in_finishevent();
}

void IN_KeyEvent(unsigned int devid, int down, int keycode, int unicode)
{
	struct eventlist_s *ev = in_newevent();
	if (!ev)
		return;
	ev->type = down?IEV_KEYDOWN:IEV_KEYRELEASE;
	ev->devid = devid;
	ev->keyboard.scancode = keycode;
	ev->keyboard.unicode = unicode;
	in_finishevent();
}

/*
devid is the mouse device id. generally idependant from keyboards.
for multitouch, devid might be the touch identifier, which will persist until released.
x is horizontal, y is vertical.
z is height... generally its used as a mousewheel instead, but there are some '3d' mice out there, so its provided in this api.
*/
void IN_MouseMove(unsigned int devid, int abs, float x, float y, float z, float size)
{
	struct eventlist_s *ev;
	if (!abs && !x && !y && !z)
		return;	//ignore non-movements
	ev = in_newevent();
	if (!ev)
		return;
	ev->devid = devid;
	ev->type = abs?IEV_MOUSEABS:IEV_MOUSEDELTA;
	ev->mouse.x = x;
	ev->mouse.y = y;
	ev->mouse.z = z;
	ev->mouse.tsize = size;
	in_finishevent();
}

void IN_Accelerometer(unsigned int devid, float x, float y, float z)
{
	struct eventlist_s *ev = in_newevent();
	if (!ev)
		return;
	ev->devid = devid;
	ev->type = IEV_ACCELEROMETER;
	ev->accel.x = x;
	ev->accel.y = y;
	ev->accel.z = z;
	in_finishevent();
}
void IN_Gyroscope(unsigned int devid, float pitch, float yaw, float roll)
{
	struct eventlist_s *ev = in_newevent();
	if (!ev)
		return;
	ev->devid = devid;
	ev->type = IEV_GYROSCOPE;
	ev->gyro.pitch = pitch;
	ev->gyro.yaw = yaw;
	ev->gyro.roll = roll;
	in_finishevent();
}


extern usercmd_t cl_pendingcmd[MAX_SPLITS];
qboolean IN_SetHandPosition(const char *devname, vec3_t org, vec3_t ang, vec3_t vel, vec3_t avel)
{
	int dtype;
	int seat;
	struct vrdevinfo_s *dev;
	if (!strncasecmp(devname, "left", 4))
	{
		seat = atoi(devname+4);
		dtype = VRDEV_LEFT;
	}
	else if (!strncasecmp(devname, "right", 5))
	{
		seat = atoi(devname+5);
		dtype = VRDEV_RIGHT;
	}
	else if (!strncasecmp(devname, "head", 4))
	{
		seat = atoi(devname+4);
		dtype = VRDEV_HEAD;
	}
	else
		return false;	//no idea what you're talking about.
	if (seat < 0 || seat >= MAX_SPLITS)
		return false;	//duuuude!
	dev = &cl.playerview[seat].vrdev[dtype];

	if (org)
		VectorCopy(org, dev->origin);
	else
		VectorClear(dev->origin);
	if (ang)
	{
		dev->angles[0] = ANGLE2SHORT(ang[0]),
		dev->angles[1] = ANGLE2SHORT(ang[1]),
		dev->angles[2] = ANGLE2SHORT(ang[2]);
	}
	else
		VectorClear(dev->angles);
	if (vel)
		VectorCopy(vel, dev->velocity);
	else
		VectorClear(dev->velocity);
	if (avel)
		dev->avelocity[0] = ANGLE2SHORT(avel[0]),
		dev->avelocity[1] = ANGLE2SHORT(avel[1]),
		dev->avelocity[2] = ANGLE2SHORT(avel[2]);
	else
		VectorClear(dev->avelocity);

	dev->status =
			(org ?VRSTATUS_ORG:0)|
			(ang ?VRSTATUS_ANG:0)|
			(vel ?VRSTATUS_VEL:0)|
			(avel?VRSTATUS_AVEL:0);
	return true;
}
