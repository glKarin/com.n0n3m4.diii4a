#include "quakedef.h"
#include "glquake.h"
#include "vr.h"
#include "shader.h"
vfsfile_t *FSWEB_OpenTempHandle(int f);

extern cvar_t gl_lateswap;
extern qboolean gammaworks;

extern qboolean vid_isfullscreen;

qboolean mouseactive;
extern qboolean mouseusedforgui;

static struct
{
	int id;
	unsigned axistobuttonp;	//bitmask of whether we're currently reporting each axis as pressed. without saving values.
	unsigned axistobuttonn;
	int repeatkey;
    float repeattime;
} gamepaddevices[] = {{DEVID_UNSET},{DEVID_UNSET},{DEVID_UNSET},{DEVID_UNSET},{DEVID_UNSET},{DEVID_UNSET},{DEVID_UNSET},{DEVID_UNSET}};
static int keyboardid[] = {0};
static int mouseid[] = {0,1,2,3,4,5,6,7};

static cvar_t *xr_enable;	//refrains from starting up when 0 and closes it too, and forces it off too
static cvar_t *xr_metresize;
static cvar_t *xr_skipregularview;

static void WebXR_Toggle_f(void)
{
	emscriptenfte_xr_setup(-3);	//toggle it.
}
static void WebXR_Start_f(void)
{
	emscriptenfte_xr_setup(-2);	//start it (any mode)
}
static void WebXR_End_f(void)
{
	emscriptenfte_xr_setup(-1);	//end it (if running)
}
static void WebXR_Start_Inline_f(void)
{
	emscriptenfte_xr_setup(-1);	//end it (if running)
	emscriptenfte_xr_setup(0);	//start it (inline mode)
}
static void WebXR_Start_VR_f(void)
{
	emscriptenfte_xr_setup(-1);	//end it (if running)
	emscriptenfte_xr_setup(1);	//start it (inline mode)
}
static void WebXR_Start_AR_f(void)
{
	emscriptenfte_xr_setup(-1);	//end it (if running)
	emscriptenfte_xr_setup(2);	//start it (inline mode)
}
static void WebXR_Info_f(void)
{
	int modes = emscriptenfte_xr_issupported();
	if (modes == 0)
	{
		Con_Printf(S_COLOR_RED"WebXR is unavailable\n");
		return;
	}
	if (modes < 0)
		Con_Printf("WebXR availability is unknown\n");
	else
	{
		Con_Printf("WebXR-inline is %savailable\n",			(modes & (1<<0))?S_COLOR_GREEN:S_COLOR_RED"un");
		Con_Printf("WebXR-immersive-vr is %savailable\n",	(modes & (1<<1))?S_COLOR_GREEN:S_COLOR_RED"un");
		Con_Printf("WebXR-immersive-ar is %savailable\n",	(modes & (1<<2))?S_COLOR_GREEN:S_COLOR_RED"un");

		if (emscriptenfte_xr_isactive())
			Con_Printf("WebXR is active\n");
		else
			Con_Printf("WebXR is inactive\n");
	}
}

static qboolean	WebXR_Prepare	(vrsetup_t *setupinfo)
{	//called before graphics context init. basically just checks if we can do vr.
	xr_enable			= Cvar_Get2("xr_enable",			"1",			CVAR_SEMICHEAT,	"Controls whether to use webxr rendering or not.",			"WebXR configuration");
	xr_metresize		= Cvar_Get2("xr_metresize",			"26.24671916",	CVAR_ARCHIVE,	"Size of a metre in game units",							"WebXR configuration");
	xr_skipregularview	= Cvar_Get2("xr_skipregularview",	"1",			CVAR_ARCHIVE,	"Skip rendering the regular view when OpenXR is active",	"WebXR configuration");

	return xr_enable->ival;//emscriptenfte_xr_issupported() != 0;
}
static qboolean	WebXR_Init		(vrsetup_t *setupinfo, rendererstate_t *info)
{	//called after graphics context init.
	Cmd_AddCommand("xr_toggle", WebXR_Toggle_f);
	Cmd_AddCommand("xr_start",	WebXR_Start_f);
	Cmd_AddCommand("xr_end",	WebXR_End_f);
	Cmd_AddCommand("xr_start_inline", WebXR_Start_Inline_f);
	Cmd_AddCommand("xr_start_vr", WebXR_Start_VR_f);
	Cmd_AddCommand("xr_start_ar", WebXR_Start_AR_f);
	Cmd_AddCommand("xr_info", WebXR_Info_f);
	return true;	//we don't have much control here, so we have no failure paths here. probably the session isn't even created.
}
static unsigned int	WebXR_SyncFrame	(double *frametime)
{	//called in the client's main loop, to block/tweak frame times. True means the game should render as fast as possible.
	return emscriptenfte_xr_isactive();	//we're using openxr's session's sync stuff when this is active.
}
static void WebXR_MatrixToQuake(const float in[16], float out[12])
{
	float tempmat[16], tempmat2[16];
	const float fixupmat[16]	= { 0, 0, -1, 0,   -1,  0,  0,  0,    0,  1,  0,  0,    0,  0,  0,  1};
	const float reorient[16]	= { 0, -1, 0, 0,    0,  0,  1,  0,   -1,  0,  0,  0,    0,  0,  0,  1};

	Matrix4_Multiply(in, fixupmat, tempmat2);//we want z-up...
	Matrix4_Multiply(reorient, tempmat2, tempmat);//rotate it.

	//transpose it cos smaller? urgh.
	out[ 0] = tempmat[0];
	out[ 1] = tempmat[4];
	out[ 2] = tempmat[8];
	out[ 3] = tempmat[12];
	out[ 4] = tempmat[1];
	out[ 5] = tempmat[5];
	out[ 6] = tempmat[9];
	out[ 7] = tempmat[13];
	out[ 8] = tempmat[2];
	out[ 9] = tempmat[6];
	out[10] = tempmat[10];
	out[11] = tempmat[14];

	//fix up offset scaling parts
	out[3] *= xr_metresize->value;
	out[7] *= xr_metresize->value;
	out[11]*= xr_metresize->value;
}
static qboolean	WebXR_Render	(void(*rendereye)(texid_t tex, const pxrect_t *viewport, const vec4_t fovoverride, const float projmatrix[16], const float eyematrix[12]))
{	//calls rendereye for each view we're meant to be drawing.
	//webxr uses separate viewpoints on the same fbo
	struct webxrinfo_s eye[16];
	fbostate_t fbo;
	int oldfbo = 0;
	float eyematrix[12];
	pxrect_t vp;

	int e, eyes = emscriptenfte_xr_geteyeinfo(countof(eye), eye);
	if (!eyes)
		return false;	//erk? lets just do normal drawing.

	for (e = 0; e < eyes; e++)
	{
		fbo.fbo = eye[e].fbo;
		if (!e)
			oldfbo = GLBE_FBO_Push(&fbo);
		else
			GLBE_FBO_Push(&fbo);

		if (!eye[e].viewport[2] || !eye[e].viewport[3])
			continue;	//no pixels getting drawn... don't waste time (emulators that feel an urge to give a dodgy eye to report two eyes instead of one)

		vp.x		= eye[e].viewport[0];
		vp.width	= eye[e].viewport[2];
		vp.height	= eye[e].viewport[3];
		vp.maxheight = vp.y+vp.height;	//negatives suck.
		vp.y = vp.maxheight-eye[e].viewport[1]-vp.height;	//opengl sucks

		WebXR_MatrixToQuake(eye[e].transform, eyematrix);

		//really just a pointer to R_RenderEyeScene...
		rendereye(NULL, &vp, NULL, eye[e].projmatrix, eyematrix);
	}
	GLBE_FBO_Pop(oldfbo);

	if (!xr_enable->ival)
		emscriptenfte_xr_shutdown();

	if (eye[e].fbo==0)
		return true;	//always skip the non-vr screen when we're fighting over the same FB.
	return xr_skipregularview->ival;	//skip non-vr rendering.
}
static void		WebXR_Shutdown	(void)
{
	Cmd_RemoveCommand("xr_toggle");
	Cmd_RemoveCommand("xr_start");
	Cmd_RemoveCommand("xr_end");
	Cmd_RemoveCommand("xr_start_inline");
	Cmd_RemoveCommand("xr_start_vr");
	Cmd_RemoveCommand("xr_start_ar");
	Cmd_RemoveCommand("xr_info");
	emscriptenfte_xr_shutdown();
}
static struct plugvrfuncs_s webxrfuncs = {
	"WebXR",
	WebXR_Prepare,
	WebXR_Init,
	WebXR_SyncFrame,
	WebXR_Render,
	WebXR_Shutdown,
};

static void *GLVID_getwebglfunction(char *functionname)
{
	return NULL;
}

//the enumid is the value for the open function rather than the working id.
static int J_AllocateDevID(void)
{
    extern cvar_t in_skipplayerone;
    unsigned int id = (in_skipplayerone.ival?1:0), j;
    for (j = 0; j < countof(gamepaddevices);)
    {
        if (gamepaddevices[j++].id == id)
        {
            j = 0;
            id++;
        }
    }

	return id;
}

static void IN_GamePadButtonEvent(int joydevid, int button, int ispressed, int isstandardmapping)
{
	//note that the gamepad API handles 'buttons' as float values, so triggers are here instead of as 'axis' values (unlike other APIs). on the plus side, we're no longer responsible for figuring out the required threshold value to denote a 'press', but we're not tracking half-presses and that's our fault and we don't care.
	static const int standardmapping[] =
	{	//the order of these keys is different from that of xinput
		//however, the quake button codes should be the same. I really ought to define some K_ aliases for them.
		K_GP_A,
		K_GP_B,
		K_GP_X,
		K_GP_Y,
		K_GP_LEFT_SHOULDER,
		K_GP_RIGHT_SHOULDER,
		K_GP_LEFT_TRIGGER,
		K_GP_RIGHT_TRIGGER,
		K_GP_BACK,
		K_GP_START,
		K_GP_LEFT_STICK,
		K_GP_RIGHT_STICK,
		K_GP_DPAD_UP,
		K_GP_DPAD_DOWN,
		K_GP_DPAD_LEFT,
		K_GP_DPAD_RIGHT,
		K_GP_GUIDE,
		//K_GP_UNKNOWN
	};

	if (joydevid < 0)
	{
		static const int standardxrmapping[] =
		{
			//Right					Left
			K_GP_RIGHT_TRIGGER,		K_GP_LEFT_TRIGGER,	//Primary trigger/button
			K_GP_RIGHT_SHOULDER,	K_GP_LEFT_SHOULDER,	//Primary squeeze
			K_GP_TOUCHPAD,			K_GP_MISC1,			//Primary touchpad
			K_GP_RIGHT_STICK,		K_GP_LEFT_STICK,	//Primary thumbstick

			//'Additional inputs may be exposed after', which should be in some pseudo-prioritised order with the awkward ones last.
			K_GP_A,					K_GP_DPAD_DOWN,
			K_GP_B,					K_GP_DPAD_RIGHT,
			K_GP_X,					K_GP_DPAD_LEFT,
			K_GP_Y,					K_GP_DPAD_UP,
		};
		button = button*2 + (joydevid != -1);	//munge them into a single array cos I cba with all the extra conditionals
		if (button < countof(standardxrmapping))
			button = standardxrmapping[button];
		else
			return; //err...

		joydevid = countof(gamepaddevices)-1;
	}
	else if (isstandardmapping && button < countof(standardmapping))
		button = standardmapping[button];
	else if (button < 32+4)
		button = K_JOY1+button;
	else
		return;	//err...

	if (joydevid < countof(gamepaddevices))
	{
		if (DEVID_UNSET == gamepaddevices[joydevid].id)
		{
			if (!ispressed)
				return;	//don't send axis events until its enabled.
			gamepaddevices[joydevid].id = J_AllocateDevID();
		}
		if (ispressed)
		{
			gamepaddevices[joydevid].repeatkey = button;
			gamepaddevices[joydevid].repeattime = 1.0;
		}
		else if (gamepaddevices[joydevid].repeatkey == button)
			gamepaddevices[joydevid].repeatkey = 0;
		joydevid = gamepaddevices[joydevid].id;
	}

	IN_KeyEvent(joydevid, ispressed, button, 0);
}
static void IN_GamePadButtonRepeats(void)
{
	int j;
	for (j = 0; j < countof(gamepaddevices); j++)
	{
		if (gamepaddevices[j].id == DEVID_UNSET)
			continue;
		if (!gamepaddevices[j].repeatkey)
			continue;
		gamepaddevices[j].repeattime -= host_frametime;
		if (gamepaddevices[j].repeattime < 0)
		{	//it is time!
			gamepaddevices[j].repeattime = 0.25; //faster re-repeat than the initial delay.
			IN_KeyEvent(gamepaddevices[j].id, true, gamepaddevices[j].repeatkey, 0);	//an extra down. no ups.
		}
	}
}

static void IN_GamePadAxisEvent(int joydevid, int axis, float value, int isstandardmapping)
{
	static const struct
	{
		int axis;
		int poskey;	//mostly for navigating menus, but oh well.
		int negkey;
	} standardmapping[] =
	{
		{GPAXIS_LT_RIGHT,	K_GP_LEFT_THUMB_RIGHT,	K_GP_LEFT_THUMB_LEFT},
		{GPAXIS_LT_DOWN,	K_GP_LEFT_THUMB_DOWN,	K_GP_LEFT_THUMB_UP},
		{GPAXIS_RT_RIGHT,	K_GP_RIGHT_THUMB_RIGHT,	K_GP_RIGHT_THUMB_LEFT},
		{GPAXIS_RT_DOWN,	K_GP_RIGHT_THUMB_DOWN,	K_GP_RIGHT_THUMB_UP},

		//this seems fucked. only 4 axis are defined as part of the standard mapping. triggers are implemented as buttons with a .value (instead of .pressed) but they don't seem to work at all.
		//emulating here should be giving dupes, but I don't know how else to get this shite to work properly.
		{GPAXIS_LT_AUX,		K_GP_LEFT_TRIGGER,0},
		{GPAXIS_RT_AUX,		K_GP_RIGHT_TRIGGER,0},
	};
	int qdevid;
	int pos=0, neg=0;
	int qaxis;
	if (joydevid < 0)
	{
		static const int standardxrmapping[] =
		{
			//Right					Left
			-1,						-1,					//Primary touchpad X
			-1,						-1,					//Primary touchpad Y
			GPAXIS_RT_RIGHT,		GPAXIS_LT_RIGHT,	//Primary thumbstick X
			GPAXIS_RT_DOWN,			GPAXIS_LT_DOWN,		//Primary thumbstick Y

			//'Additional inputs may be exposed after', which should be in some pseudo-prioritised order with the awkward ones last.
		};
		qaxis = axis*2 + (joydevid != -1);	//munge them into a single array cos I cba with all the extra conditionals
		if (qaxis < countof(standardxrmapping))
			qaxis = standardxrmapping[qaxis];
		else
			return; //err...

		joydevid = countof(gamepaddevices)-1;
	}
	else if (isstandardmapping)
	{
		if (axis >= 0 && axis < countof(standardmapping))
		{
			pos = standardmapping[axis].poskey;
			neg = standardmapping[axis].negkey;
			qaxis = standardmapping[axis].axis;
		}
		else
			qaxis = axis;
	}
	else
		return;	//random mappings? erk?

	if (joydevid < countof(gamepaddevices))
	{
		qdevid = gamepaddevices[joydevid].id;
		if (qdevid == DEVID_UNSET)
		{
			if (value < -0.9 || value > 0.9)
				gamepaddevices[joydevid].id = J_AllocateDevID();
			return;	//don't send axis events until its enabled.
		}

		if (value > 0.5 && pos)
		{
			if (!(gamepaddevices[joydevid].axistobuttonp & (1u<<axis)))
			{
				IN_KeyEvent(qdevid, true, pos, 0);
				gamepaddevices[joydevid].repeatkey = pos;
				gamepaddevices[joydevid].repeattime = 1.0;
			}
			gamepaddevices[joydevid].axistobuttonp |= 1u<<axis;
		}
		else if (gamepaddevices[joydevid].axistobuttonp & (1u<<axis))
		{
			IN_KeyEvent(qdevid, false, pos, 0);
			gamepaddevices[joydevid].axistobuttonp &= ~(1u<<axis);
			if (gamepaddevices[joydevid].repeatkey == pos)
				gamepaddevices[joydevid].repeatkey = 0;
		}

		if (value < -0.5 && neg)
		{
			if (!(gamepaddevices[joydevid].axistobuttonn & (1u<<axis)))
			{
				IN_KeyEvent(qdevid, true, neg, 0);
				gamepaddevices[joydevid].repeatkey = neg;
				gamepaddevices[joydevid].repeattime = 1.0;
			}
			gamepaddevices[joydevid].axistobuttonn |= 1u<<axis;
		}
		else if (gamepaddevices[joydevid].axistobuttonn & (1u<<axis))
		{
			IN_KeyEvent(qdevid, false, neg, 0);
			gamepaddevices[joydevid].axistobuttonn &= ~(1u<<axis);
			if (gamepaddevices[joydevid].repeatkey == neg)
				gamepaddevices[joydevid].repeatkey = 0;
		}
	}
	else
		qdevid = joydevid;

	IN_JoystickAxisEvent(qdevid, qaxis, value);
}

static void IN_GamePadOrientationEvent(int joydevid, float px,float py,float pz, float qx,float qy,float qz,float qw)
{	//(some) vr controllers only
	vec3_t org, ang;
	const char *dev;

	const float sqw = qw * qw;
	const float sqx = qx * qx;
	const float sqy = qy * qy;
	const float sqz = qz * qz;

	ang[PITCH] = -asin(-2 * (qy * qz - qw * qx)) * (180/M_PI);
	ang[YAW] = atan2(2 * (qx * qz + qw * qy), sqw - sqx - sqy + sqz) * (180/M_PI);
	ang[ROLL] = -atan2(2 * (qx * qy + qw * qz), sqw - sqx + sqy - sqz) * (180/M_PI);

	org[0] = -pz * xr_metresize->value;
	org[1] = -px * xr_metresize->value;
	org[2] = py * xr_metresize->value;

	if (joydevid == -1)
		dev = "right";
	else if (joydevid == -2)
		dev = "left";
	else if (joydevid == -3)
		dev = "head";
	else if (joydevid == -4)
		dev = "gaze";
	else
		return;

	IN_SetHandPosition(dev, org, ang, NULL, NULL);
}

static void VID_Resized(int width, int height, float scale)
{
	extern cvar_t vid_conautoscale, vid_conwidth;
	extern cvar_t vid_dpi_x, vid_dpi_y;
	vid.pixelwidth = width;
	vid.pixelheight = height;
//Con_Printf("Resized: %i %i\n", vid.pixelwidth, vid.pixelheight);

	//if you're zooming in, it should stay looking like its zoomed.
	vid.dpi_x = 96*scale;
	vid.dpi_y = 96*scale;
	Cvar_ForceSetValue(&vid_dpi_x, vid.dpi_x);
	Cvar_ForceSetValue(&vid_dpi_y, vid.dpi_y);

	Cvar_ForceCallback(&vid_conautoscale);
	Cvar_ForceCallback(&vid_conwidth);
}
static unsigned int domkeytoquake(unsigned int code)
{
	static const unsigned short tab[256] =
	{
		/*  0*/ 0,0,0,0,0,0,0,0,                K_BACKSPACE,K_TAB,0,0,0,K_ENTER,0,0,
		/* 16*/ K_SHIFT,K_CTRL,K_ALT,K_PAUSE,K_CAPSLOCK,0,0,0,0,0,0,K_ESCAPE,0,0,0,0,
		/* 32*/ ' ',K_PGUP,K_PGDN,K_END,K_HOME,K_LEFTARROW,K_UPARROW,K_RIGHTARROW,              K_DOWNARROW,0,0,0,K_PRINTSCREEN,K_INS,K_DEL,0,
		/* 48*/ '0','1','2','3','4','5','6','7',                '8','9',0,';',0,'=',0,0,

		/* 64*/ 0,'a','b','c','d','e','f','g',          'h','i','j','k','l','m','n','o',
		/* 80*/ 'p','q','r','s','t','u','v','w',                'x','y','z',K_LWIN,K_RWIN,K_APP,0,0,
		/* 96*/ K_KP_INS,K_KP_END,K_KP_DOWNARROW,K_KP_PGDN,K_KP_LEFTARROW,K_KP_5,K_KP_RIGHTARROW,K_KP_HOME,             K_KP_UPARROW,K_KP_PGDN,K_KP_STAR,K_KP_PLUS,0,K_KP_MINUS,K_KP_DEL,K_KP_SLASH,
		/*112*/ K_F1,K_F2,K_F3,K_F4,K_F5,K_F6,K_F7,K_F8,K_F9,K_F10,K_F11,K_F12,0,0,0,0,
		/*128*/ 0,0,0,0,0,0,0,0,                0,0,0,0,0,0,0,0,
		/*144*/ K_KP_NUMLOCK,K_SCRLCK,0,0,0,0,0,0,              0,0,0,0,0,0,0,0,
		/*160*/ 0,0,0,'#',0,0,0,0,                0,0,0,0,0,'-',0,0,
		/*176*/ 0,0,0,0,0,0,0,0,                0,0,';','=',',','-','.','/',
		/*192*/ '`',0,0,0,0,0,0,0,             0,0,0,0,0,0,0,0,
		/*208*/ 0,0,0,0,0,0,0,0,                0,0,0,'[','\\',']','\'','`',
		/*224*/ 0,0,0,0,0,0,0,0,                0,0,0,0,0,0,0,0,
		/*240*/ 0,0,0,0,0,0,0,0,                0,0,0,0,0,0,0,0,
	};
	if (!code)
		return 0;
	if (code >= sizeof(tab)/sizeof(tab[0]))
	{
		Con_DPrintf("You just pressed key %u, but I don't know what its meant to be\n", code);
		return 0;
	}
	if (!tab[code])
		Con_DPrintf("You just pressed key %u, but I don't know what its meant to be\n", code);

//	Con_DPrintf("You just pressed dom key %u, which is quake key %u\n", code, tab[code]);
	return tab[code];
}
static int DOM_KeyEvent(unsigned int devid, int down, int scan, int uni)
{
	extern int		shift_down;
//	Con_Printf("Key %s %i %i:%c\n", down?"down":"up", scan, uni, uni?(char)uni:' ');
	if (shift_down)
	{
		scan = domkeytoquake(scan);
	}
	else
	{
		scan = domkeytoquake(scan);
	}
	IN_KeyEvent(keyboardid[devid], down, scan, uni);
	//Chars which don't map to some printable ascii value get preventDefaulted.
	//This is to stop fucking annoying fucking things like backspace randomly destroying the page and thus game.
	//And it has to be conditional, or we don't get any unicode chars at all.
	//The behaviour browsers seem to give is retardedly unhelpful, and just results in hacks to detect keys that appear to map to ascii...
	//Preventing the browser from leaving the page etc should NOT mean I can no longer get ascii/unicode values, only that the browser stops trying to do something random due to the event.
	//If you are the person that decreed that this is the holy way, then please castrate yourself now.
//	if (scan == K_BACKSPACE || scan == K_LCTRL || scan == K_LALT || scan == K_LSHIFT || scan == K_RCTRL || scan == K_RALT || scan == K_RSHIFT)
		return true;
//	return false;
}
static int RemapTouchId(int id, qboolean final)
{
	static int touchids[countof(mouseid)];
	int i;
	if (!id)
		return id;
	for (i = 1; i < countof(touchids); i++)
		if (touchids[i] == id)
		{
			if (final)	
				touchids[i] = 0;
			return mouseid[i];
		}
	for (i = 1; i < countof(touchids); i++)
		if (touchids[i] == 0)
		{
			if (!final)
				touchids[i] = id;
			if (mouseid[i] == DEVID_UNSET)
				mouseid[i] = i;
			return mouseid[i];
		}
	return id;
}
static void DOM_ButtonEvent(unsigned int devid, int down, int button)
{
	devid = RemapTouchId(devid, !down);
	if (down == 2)
	{
		//fixme: the event is a float. we ignore that.
		while(button < 0)
		{
			IN_KeyEvent(devid, true, K_MWHEELUP, 0);
			IN_KeyEvent(devid, false, K_MWHEELUP, 0);
			button += 1;
		}
		while(button > 0)
		{
			IN_KeyEvent(devid, true, K_MWHEELDOWN, 0);
			IN_KeyEvent(devid, false, K_MWHEELUP, 0);
			button -= 1;
		}
	}
	else
	{
		//swap buttons 2 and 3, so rmb is still +forward by default and not +mlook.
		if (button == 2)
			button = 1;
		else if (button == 1)
			button = 2;

		if (button < 0)
			button = K_TOUCH;
		else
			button += K_MOUSE1;
		IN_KeyEvent(devid, down, button, 0);
	}
}
static void DOM_MouseMove(unsigned int devid, int abs, float x, float y, float z, float size)
{
	devid = RemapTouchId(devid, false);
	IN_MouseMove(devid, abs, x, y, z, size);
}

static void DOM_LoadFile(char *loc, char *mime, int handle)
{
	vfsfile_t *file = NULL;
	if (handle != -1)
		file = FSWEB_OpenTempHandle(handle);
	else
	{
		char str[1024];
		if (!strcmp(mime, "joinurl") || !strcmp(mime, "observeurl")  || !strcmp(mime, "connecturl"))
		{
			extern cvar_t spectator;
			if (!strcmp(mime, "joinurl"))
				Cvar_Set(&spectator, "0");
			if (!strcmp(mime, "observeurl"))
				Cvar_Set(&spectator, "1");
			Cbuf_AddText(va("connect %s\n", COM_QuotedString(loc, str, sizeof(str), false)), RESTRICT_INSECURE);
			return;
		}
		if (!strcmp(mime, "demourl"))
		{
			Cbuf_AddText(va("qtvplay %s\n", COM_QuotedString(loc, str, sizeof(str), false)), RESTRICT_INSECURE);
			return;
		}
	}
	//try and open it. generally downloading it from the server.
	if (!Host_RunFile(loc, strlen(loc), file))
	{
		if (file)
			VFS_CLOSE(file);
	}
}
static void DOM_CbufAddText(const char *text)
{
	Cbuf_AddText(text, RESTRICT_LOCAL);
}
static int VID_ShouldSwitchToFullscreen(void)
{	//if false, mouse grabs won't work and we'll be forced to touchscreen mode.
	//we can only go fullscreen when the user clicks something.
	//this means that the user will get pissed off at the fullscreen state changing when they first click on the menus after it loading up.
	//this is confounded by escape bringing up the menu. <ESC>GRR IT CHANGED MODE!<options>WTF IT CHANGED AGAIN FUCKING PIECE OF SHIT!.
	//annoying, but that's web browsers for you. the best thing we can do is to not regrab until they next click while actually back in the game.
	extern cvar_t vid_fullscreen;
	return !!vid_fullscreen.value && !Key_MouseShouldBeFree();
}
qboolean GLVID_Init (rendererstate_t *info, unsigned char *palette)
{
	vid_isfullscreen = true;

	if (!emscriptenfte_setupcanvas(
		info->width,
		info->height,
		VID_Resized,
		DOM_MouseMove,
		DOM_ButtonEvent,
		DOM_KeyEvent,
		DOM_LoadFile,
		DOM_CbufAddText,
		IN_GamePadButtonEvent,
		IN_GamePadAxisEvent,
		IN_GamePadOrientationEvent,
		VID_ShouldSwitchToFullscreen
		))
	{
		Con_Printf("Couldn't set up canvas\n");
		return false;
	}

	vid.activeapp = true;

	if (info->vr && !info->vr->Prepare(NULL))
		info->vr = NULL;	//not available.

	if (!GL_Init(info, GLVID_getwebglfunction))
		return false;		
	if (info->vr && !info->vr->Init(NULL, info))
		return false;
	vid.vr = info->vr;

	qglViewport (0, 0, vid.pixelwidth, vid.pixelheight);

	VID_Resized(vid.pixelwidth, vid.pixelheight, 1);

	mouseactive = false;

	return true;
}

void GLVID_DeInit (void)
{
	vid.activeapp = false;

	emscriptenfte_setupcanvas(-1, -1, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

	GL_ForgetPointers();
}


void GLVID_SwapBuffers (void)
{
	//webgl doesn't support swapbuffers.
	//you can't use it for loading screens.
	//such things must result in waiting until the following frame.
	//although there IS a swapped-buffers event, which we should probably use in preference to requestanimationframe or whatever the call is.

/*
	if (!vid_isfullscreen)
	{
		if (!in_windowed_mouse.value)
		{
			if (mouseactive)
			{
				IN_DeactivateMouse ();
			}
		}
		else
		{
			if ((key_dest == key_game||mouseusedforgui) && vid.activeapp)
				IN_ActivateMouse ();
			else if (!(key_dest == key_game || mouseusedforgui) || !vid.activeapp)
				IN_DeactivateMouse ();
		}
	}
*/
}

qboolean GLVID_ApplyGammaRamps (unsigned int gammarampsize, unsigned short *ramps)
{
	gammaworks = false;
	return gammaworks;
}

void GLVID_SetCaption(const char *text)
{
	emscriptenfte_settitle(text);
}

void Sys_SendKeyEvents(void)
{
	/*most callbacks happen outside our code, we don't need to poll for events - except for joysticks*/
	qboolean shouldbefree = Key_MouseShouldBeFree();
	emscriptenfte_updatepointerlock(in_windowed_mouse.ival && !shouldbefree, shouldbefree);
	emscriptenfte_polljoyevents();

	IN_GamePadButtonRepeats();
}
/*various stuff for joysticks, which we don't support in this port*/
void INS_Shutdown (void)
{
}
void INS_ReInit (void)
{
}
void INS_Move(void)
{
}
void INS_Init (void)
{
	//mneh, handy enough
	R_RegisterVRDriver(NULL, &webxrfuncs);
}
void INS_Accumulate(void)
{
}
void INS_Commands (void)
{
}
void INS_EnumerateDevices(void *ctx, void(*callback)(void *ctx, const char *type, const char *devicename, unsigned int *qdevid))
{
	size_t i;
	char foobar[64];
	for (i = 0; i < countof(gamepaddevices); i++)
	{
		Q_snprintfz(foobar, sizeof(foobar), "gp%i", (int)i);
		callback(ctx, "gamepad", foobar, &gamepaddevices[i].id);
	}
	for (i = 0; i < countof(mouseid); i++)
	{
		Q_snprintfz(foobar, sizeof(foobar), "m%i", (int)i);
		callback(ctx, "mouse", foobar, &mouseid[i]);
	}
	for (i = 0; i < countof(keyboardid); i++)
	{
		Q_snprintfz(foobar, sizeof(foobar), "kb%i", (int)i);
		callback(ctx, "keyboard", foobar, &keyboardid[i]);
	}
}

enum controllertype_e INS_GetControllerType(int id)
{
	size_t i;
	for (i = 0; i < countof(gamepaddevices); i++)
	{
		if (id == gamepaddevices[i].id)
			return CONTROLLER_UNKNOWN;	//browsers don't really like providing more info, to thwart fingerprinting. shame. you should just use generic glyphs.
	}
	return CONTROLLER_NONE;	//nuffin here. yay fingerprinting?
}
void INS_Rumble(int joy, quint16_t amp_low, quint16_t amp_high, quint32_t duration)
{
}
void INS_RumbleTriggers(int joy, quint16_t left, quint16_t right, quint32_t duration)
{
}
void INS_SetLEDColor(int id, vec3_t color)
{
}
void INS_SetTriggerFX(int id, const void *data, size_t size)
{
}

