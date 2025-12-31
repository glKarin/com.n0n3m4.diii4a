// WL_PLAY.C

#include "c_cvars.h"
#include "wl_def.h"
#include "wl_menu.h"
#include "id_ca.h"
#include "id_sd.h"
#include "id_vl.h"
#include "id_vh.h"
#include "id_us.h"

#include "wl_cloudsky.h"
#include "wl_shade.h"
#include "language.h"
#include "lumpremap.h"
#include "thinker.h"
#include "actor.h"
#include "textures/textures.h"
#include "v_video.h"
#include "wl_agent.h"
#include "wl_debug.h"
#include "wl_draw.h"
#include "wl_game.h"
#include "wl_inter.h"
#include "wl_net.h"
#include "wl_play.h"
#include "g_mapinfo.h"
#include "a_inventory.h"
#include "am_map.h"

/*
=============================================================================

												LOCAL CONSTANTS

=============================================================================
*/

#define sc_Question     0x35

/*
=============================================================================

												GLOBAL VARIABLES

=============================================================================
*/

bool madenoise;              // true when shooting or screaming

exit_t playstate;

#ifdef __ANDROID__
extern bool ShadowingEnabled;
#endif

bool noclip, ammocheat, mouselook = false;
int godmode, singlestep;
bool notargetmode = false;
unsigned int extravbls = 0; // to remove flicker (gray stuff at the bottom)
unsigned short Paused;

//
// replacing refresh manager
//
bool noadaptive = false;
unsigned tics;

#ifdef _DIII4A //karin: main loop control
extern volatile bool q3e_running;
extern bool GLimp_CheckGLInitialized(void);
#endif

//
// control info
//
#define JoyAx(x) (32+(x<<1))
#define CS_AxisDigital -1
ControlScheme controlScheme[] =
{
	{ bt_moveforward,		"Forward",		JoyAx(1),	sc_UpArrow,		-1, offsetof(TicCmd_t, controly), 1 },
	{ bt_movebackward,		"Backward",		JoyAx(1)+1,	sc_DownArrow,	-1, offsetof(TicCmd_t, controly), 0 },
	{ bt_strafeleft,		"Strafe Left",	JoyAx(0),	sc_Comma,		-1, offsetof(TicCmd_t, controlstrafe), 1 },
	{ bt_straferight,		"Strafe Right",	JoyAx(0)+1,	sc_Peroid,		-1, offsetof(TicCmd_t, controlstrafe), 0 },
	{ bt_turnleft,			"Turn Left",	JoyAx(3),	sc_LeftArrow,	-1, offsetof(TicCmd_t, controlx), 1 },
	{ bt_turnright,			"Turn Right",	JoyAx(3)+1,	sc_RightArrow,	-1, offsetof(TicCmd_t, controlx), 0 },
	{ bt_attack,			"Attack",		0,			sc_Control,		0,  CS_AxisDigital, 0},
	{ bt_strafe,			"Strafe",		3,			sc_Alt,			-1, CS_AxisDigital, 0 },
	{ bt_run,				"Run",			2,			sc_LShift,		-1, CS_AxisDigital, 0 },
	{ bt_use,				"Use",			1,			sc_Space,		-1, CS_AxisDigital, 0 },
	{ bt_slot1,				"Slot 1",		-1,			sc_1,			-1, CS_AxisDigital, 0 },
	{ bt_slot2,				"Slot 2", 		-1,			sc_2,			-1, CS_AxisDigital, 0 },
	{ bt_slot3,				"Slot 3",		-1,			sc_3,			-1, CS_AxisDigital, 0 },
	{ bt_slot4,				"Slot 4",		-1,			sc_4,			-1, CS_AxisDigital, 0 },
	{ bt_slot5,				"Slot 5",		-1,			sc_5,			-1, CS_AxisDigital, 0 },
	{ bt_slot6,				"Slot 6",		-1,			sc_6,			-1, CS_AxisDigital, 0 },
	{ bt_slot7,				"Slot 7",		-1,			sc_7,			-1, CS_AxisDigital, 0 },
	{ bt_slot8,				"Slot 8",		-1,			sc_8,			-1, CS_AxisDigital, 0 },
	{ bt_slot9,				"Slot 9",		-1,			sc_9,			-1, CS_AxisDigital, 0 },
	{ bt_slot0,				"Slot 0",		-1,			sc_0,			-1, CS_AxisDigital, 0 },
	{ bt_nextweapon,		"Next Weapon",	4,			-1,				-1, CS_AxisDigital, 0 },
	{ bt_prevweapon,		"Prev Weapon",	5, 			-1,				-1, CS_AxisDigital, 0 },
	{ bt_altattack,			"Alt Attack",	-1,			-1,				-1, CS_AxisDigital, 0 },
	{ bt_reload,			"Reload",		-1,			-1,				-1, CS_AxisDigital, 0 },
	{ bt_zoom,				"Zoom",			-1,			-1,				-1, CS_AxisDigital, 0 },
	{ bt_automap,			"Automap",		-1,			-1,				-1, CS_AxisDigital, 0 },
	{ bt_showstatusbar,		"Show Status",	-1,			sc_Tab,			-1,	CS_AxisDigital, 0 },
	{ bt_pause,				"Pause",		-1,			sc_Pause,		-1, CS_AxisDigital, 0 },

	// End of List
	{ bt_nobutton,			NULL, -1, -1, -1, CS_AxisDigital, 0 }
};
ControlScheme &schemeAutomapKey = controlScheme[25]; // When the input system is redone, hopefully we don't need this kind of thing

ControlScheme amControlScheme[] =
{
	{ bt_zoomin,			"Zoom In",		JoyAx(2),	sc_Equals,		-1, -1, 0 },
	{ bt_zoomout,			"Zoom Out",		JoyAx(2)+1,	sc_Minus,		-1, -1, 0 },
	{ bt_panup,				"Pan Up",		JoyAx(1),	sc_UpArrow,		-1, offsetof(TicCmd_t, controlpany), 0 },
	{ bt_pandown,			"Pan Down",		JoyAx(1)+1,	sc_DownArrow,	-1, offsetof(TicCmd_t, controlpany), 1 },
	{ bt_panleft,			"Pan Left",		JoyAx(0),	sc_LeftArrow,	-1, offsetof(TicCmd_t, controlpanx), 0 },
	{ bt_panright,			"Pan Right",	JoyAx(0)+1,	sc_RightArrow,	-1, offsetof(TicCmd_t, controlpanx), 1 },

	{ bt_nobutton,			NULL, -1, -1, -1, -1, 0 }
};

void ControlScheme::setKeyboard(ControlScheme* scheme, Button button, int value)
{
	for(int i = 0;scheme[i].button != bt_nobutton;i++)
	{
		if(scheme[i].keyboard == value)
			scheme[i].keyboard = -1;
		if(scheme[i].button == button)
			scheme[i].keyboard = value;
	}
}

void ControlScheme::setJoystick(ControlScheme* scheme, Button button, int value)
{
	for(int i = 0;scheme[i].button != bt_nobutton;i++)
	{
		if(scheme[i].joystick == value)
			scheme[i].joystick = -1;
		if(scheme[i].button == button)
			scheme[i].joystick = value;
	}
}

void ControlScheme::setMouse(ControlScheme* scheme, Button button, int value)
{
	for(int i = 0;scheme[i].button != bt_nobutton;i++)
	{
		if(scheme[i].mouse == value)
			scheme[i].mouse = -1;
		if(scheme[i].button == button)
			scheme[i].mouse = value;
	}
}

int viewsize;

bool demorecord, demoplayback;
int8_t *demoptr, *lastdemoptr;
memptr demobuffer;

//
// current user input
//
unsigned int ConsolePlayer = 0;
TicCmd_t control[MAXPLAYERS];

//===========================================================================


void CenterWindow (word w, word h);
int StopMusic (void);
void StartMusic (void);
void ContinueMusic (int offs);
void PlayLoop (void);

/*
=============================================================================

							TIMING

=============================================================================
*/

static int32_t lasttimecount;

int32_t GetTimeCount()
{
	return MS2TICS(SDL_GetTicks());
}

/*
=====================
=
= CalcTics
=
=====================
*/

void CalcTics()
{
//
// calculate tics since last refresh for adaptive timing
//

	// Have we arrived too soon?
	while(lasttimecount == GetTimeCount()+1)
		SDL_Delay(1);

	// Detect rollover, particularly if the game were paused for a LONG time
	if(lasttimecount > GetTimeCount())
		ResetTimeCount();

	uint32_t curtime = SDL_GetTicks();
	tics = MS2TICS(curtime) - lasttimecount;
	if(!tics)
	{
		// wait until end of current tic
		SDL_Delay(TICS2MS(lasttimecount + 1) - curtime);
		tics = 1;
	}
	else if(noadaptive || Net::IsBlocked())
		tics = 1;

	lasttimecount += tics;

	if (tics>MAXTICS)
		tics = MAXTICS;
}

void ResetTimeCount()
{
	lasttimecount = GetTimeCount();
}

void Delay(int wolfticks)
{
	if(wolfticks>0)
		SDL_Delay(TICS2MS(wolfticks));
}

/*
=============================================================================

							USER CONTROL

=============================================================================
*/

/*
===================
=
= PollKeyboardButtons
=
===================
*/

void PollKeyboardButtons (void)
{
	if(automap == AMA_Normal)
	{
		// HACK
		bool jam[512] = {false};
		bool jamall = !!(Paused & 2); // Paused for automap

		for(int i = 0;jamall ? amControlScheme[i].button != bt_nobutton : amControlScheme[i].button <= bt_zoomout;i++)
		{
			if(amControlScheme[i].keyboard != -1 && Keyboard[amControlScheme[i].keyboard])
			{
				control[ConsolePlayer].ambuttonstate[amControlScheme[i].button] = true;
				jam[amControlScheme[i].keyboard] = true;
			}
		}
		for(int i = 0;controlScheme[i].button != bt_nobutton;i++)
		{
			if(controlScheme[i].keyboard != -1 && Keyboard[controlScheme[i].keyboard] && !jam[controlScheme[i].keyboard])
				control[ConsolePlayer].buttonstate[controlScheme[i].button] = true;
		}
	}
	else
	{
		for(int i = 0;controlScheme[i].button != bt_nobutton;i++)
		{
			if(controlScheme[i].keyboard != -1 && Keyboard[controlScheme[i].keyboard])
				control[ConsolePlayer].buttonstate[controlScheme[i].button] = true;
		}
	}
}


/*
===================
=
= PollMouseButtons
=
===================
*/

void PollMouseButtons (void)
{
	int buttons = IN_MouseButtons();
	for (int i = 0; controlScheme[i].button != bt_nobutton; i++)
	{
		if (controlScheme[i].mouse == -1)
			continue;

		BYTE &state = control[ConsolePlayer].buttonstate[controlScheme[i].button];
		switch(controlScheme[i].mouse)
		{
		case ControlScheme::MWheel_Left:
			if (MouseWheel[di_west])
				state = true;
			break;
		case ControlScheme::MWheel_Right:
			if (MouseWheel[di_east])
				state = true;
			break;
		case ControlScheme::MWheel_Down:
			if (MouseWheel[di_south])
				state = true;
			break;
		case ControlScheme::MWheel_Up:
			if (MouseWheel[di_north])
				state = true;
			break;
		default:
			if ((buttons & (1 << controlScheme[i].mouse)))
				state = true;
			break;
		}
	}

	IN_ClearWheel();
}



/*
===================
=
= PollJoystickButtons
=
===================
*/

void PollJoystickButtons (void)
{
	if(automap == AMA_Normal)
	{
		// HACK
		bool jam[64] = {false};
		bool jamall = !!(Paused & 2); // Paused for automap

		int buttons = IN_JoyButtons();
		int axes = IN_JoyAxes();
		for(int i = 0;jamall ? amControlScheme[i].button != bt_nobutton : amControlScheme[i].button <= bt_zoomout;i++)
		{
			if(amControlScheme[i].joystick != -1)
			{
				if(amControlScheme[i].joystick < 32 && (buttons & (1<<amControlScheme[i].joystick)))
				{
					control[ConsolePlayer].ambuttonstate[amControlScheme[i].button] = true;
					jam[amControlScheme[i].joystick] = true;
				}
				else if(amControlScheme[i].axis == -1 && amControlScheme[i].joystick >= 32 && (axes & (1<<(amControlScheme[i].joystick-32))))
				{
					control[ConsolePlayer].ambuttonstate[amControlScheme[i].button] = true;
					jam[amControlScheme[i].joystick] = true;
				}
			}
		}
		for(int i = 0;controlScheme[i].button != bt_nobutton;i++)
		{
			if(controlScheme[i].joystick != -1 && !jam[controlScheme[i].joystick])
			{
				if(controlScheme[i].joystick < 32 && (buttons & (1<<controlScheme[i].joystick)))
					control[ConsolePlayer].buttonstate[controlScheme[i].button] = true;
				else if(controlScheme[i].axis == -1 && controlScheme[i].joystick >= 32 && (axes & (1<<(controlScheme[i].joystick-32))))
					control[ConsolePlayer].buttonstate[controlScheme[i].button] = true;
			}
		}
	}
	else
	{
		int buttons = IN_JoyButtons();
		int axes = IN_JoyAxes();
		for(int i = 0;controlScheme[i].button != bt_nobutton;i++)
		{
			if(controlScheme[i].joystick != -1)
			{
				if(controlScheme[i].joystick < 32 && (buttons & (1<<controlScheme[i].joystick)))
					control[ConsolePlayer].buttonstate[controlScheme[i].button] = true;
				else if(controlScheme[i].axis == -1 && controlScheme[i].joystick >= 32 && (axes & (1<<(controlScheme[i].joystick-32))))
					control[ConsolePlayer].buttonstate[controlScheme[i].button] = true;
			}
		}
	}
}


/*
===================
=
= PollKeyboardMove
=
===================
*/

void PollKeyboardMove (void)
{
	TicCmd_t &cmd = control[ConsolePlayer];

	int delta = (!alwaysrun && cmd.buttonstate[bt_run]) || (alwaysrun && !cmd.buttonstate[bt_run]) ? RUNMOVE : BASEMOVE;

	if(cmd.buttonstate[bt_moveforward])
		cmd.controly -= delta;
	if(cmd.buttonstate[bt_movebackward])
		cmd.controly += delta;
	if(cmd.buttonstate[bt_turnleft])
		cmd.controlx -= delta;
	if(cmd.buttonstate[bt_turnright])
		cmd.controlx += delta;
	if(cmd.buttonstate[bt_strafeleft])
		cmd.controlstrafe -= delta;
	if(cmd.buttonstate[bt_straferight])
		cmd.controlstrafe += delta;
}


/*
===================
=
= PollMouseMove
=
===================
*/

void PollMouseMove (void)
{
	SDL_GetRelativeMouseState(&control[ConsolePlayer].controlpanx, &control[ConsolePlayer].controlpany);

	control[ConsolePlayer].controlx += control[ConsolePlayer].controlpanx * 20 / (21 - mousexadjustment);
	if(mouselook)
	{
		int mousey = control[ConsolePlayer].controlpany;

		if(players[ConsolePlayer].ReadyWeapon && players[ConsolePlayer].ReadyWeapon->fovscale > 0)
			mousey = xs_ToInt(control[ConsolePlayer].controlpany*fabs(players[ConsolePlayer].ReadyWeapon->fovscale));

		players[ConsolePlayer].mo->pitch += mousey * (ANGLE_1 / (21 - mouseyadjustment));
		if(players[ConsolePlayer].mo->pitch+ANGLE_180 > ANGLE_180+56*ANGLE_1)
			players[ConsolePlayer].mo->pitch = 56*ANGLE_1;
		else if(players[ConsolePlayer].mo->pitch+ANGLE_180 < ANGLE_180-56*ANGLE_1)
			players[ConsolePlayer].mo->pitch = ANGLE_NEG(56*ANGLE_1);
	}
	else if(!mouseyaxisdisabled)
		control[ConsolePlayer].controly += control[ConsolePlayer].controlpany * 40 / (21 - mouseyadjustment);
}


/*
===================
=
= PollJoystickMove
=
===================
*/

void PollJoystickMove (void)
{
	const bool useam = automap == AMA_Normal && Paused;
	const ControlScheme *scheme = useam ? amControlScheme+2 : controlScheme;
	do
	{
		if(scheme->joystick >= 32)
		{
			int axisnum = (scheme->joystick-32)>>1;
			bool positive = (scheme->joystick&1) != 0;
			// Scale to -100 - 100
			const int rawaxis = clamp(IN_GetJoyAxis(axisnum), -0x7FFF, 0x7FFF);
			const int dzfactor = clamp(JoySensitivity[axisnum].deadzone*0x8000/20, 0, 0x7FFF);
			int axis = clamp(abs(rawaxis)+1-dzfactor, 0, 0x8000)*5*JoySensitivity[axisnum].sensitivity/(0x8000-dzfactor);
			if(useam)
				axis >>= 2;
			else if(control[ConsolePlayer].buttonstate[bt_run])
				axis <<= 1;
			if(positive ^ (rawaxis < 0))
				*(int*)((char*)&control[ConsolePlayer] + scheme->axis) += scheme->negative ? -axis : axis;
		}
	}
	while((++scheme)->axis != CS_AxisDigital);
}

/*
===================
=
= PollControls
=
= Gets user or demo input
= Enable absolute positioning once per frame. This prevents absolute devices
= from being carried over to adaptive tics.
=
= controlx              set between -100 and 100 per tic
= controly
= buttonheld[]  the state of the buttons LAST frame
= buttonstate[] the state of the buttons THIS frame
=
===================
*/

void PollControls (bool absolutes)
{
	int i;
	byte buttonbits;

	TicCmd_t &cmd = control[ConsolePlayer];

	cmd.controlx = 0;
	cmd.controly = 0;
	cmd.controlpanx = 0;
	cmd.controlpany = 0;
	cmd.controlstrafe = 0;
	memcpy (cmd.buttonheld, cmd.buttonstate, sizeof (cmd.buttonstate));
	memset (cmd.buttonstate, 0, sizeof (cmd.buttonstate));
	if (automap)
	{
		memcpy (cmd.ambuttonheld, cmd.ambuttonstate, sizeof (cmd.ambuttonstate));
		memset (cmd.ambuttonstate, 0, sizeof (cmd.ambuttonstate));
	}

	if (demoplayback)
	{
		//
		// read commands from demo buffer
		//
		buttonbits = *demoptr++;
		for (i = 0; i < NUMBUTTONS; i++)
		{
			cmd.buttonstate[i] = buttonbits & 1;
			buttonbits >>= 1;
		}

		cmd.controlx = *demoptr++;
		cmd.controly = *demoptr++;

		if (demoptr == lastdemoptr)
			playstate = ex_completed;   // demo is done

		return;
	}


//
// get button states
//
	PollKeyboardButtons ();

	if (mouseenabled && IN_IsInputGrabbed())
		PollMouseButtons ();

	if (joystickenabled && IN_JoyPresent())
		PollJoystickButtons ();

//
// get movements
//
	PollKeyboardMove ();

	if (absolutes && mouseenabled && IN_IsInputGrabbed())
		PollMouseMove ();

	if (joystickenabled && IN_JoyPresent())
		PollJoystickMove ();

#ifdef __ANDROID__
	extern void pollAndroidControls();
	pollAndroidControls();
#endif

	if (demorecord)
	{
		//
		// save info out to demo buffer
		//
		buttonbits = 0;

		// TODO: Support 32-bit buttonbits
		for (i = NUMBUTTONS - 1; i >= 0; i--)
		{
			buttonbits <<= 1;
			if (cmd.buttonstate[i])
				buttonbits |= 1;
		}

		*demoptr++ = buttonbits;
		*demoptr++ = cmd.controlx;
		*demoptr++ = cmd.controly;

		if (demoptr >= lastdemoptr - 8)
			playstate = ex_completed;
	}
	else if(Net::InitVars.mode != Net::MODE_SinglePlayer)
		Net::PollControls();

	// Check automap toggle before we set any buttons as held
	if (cmd.buttonstate[bt_automap] && !cmd.buttonheld[bt_automap])
	{
		AM_Toggle();
	}
	if (automap)
	{
		AM_CheckKeys();
	}

	for(unsigned int i = 0;i < Net::InitVars.numPlayers;++i)
	{
		if(control[i].buttonstate[bt_pause] && !control[i].buttonheld[bt_pause])
		{
			Paused ^= 1;

			static int lastoffs;
			if(Paused & 1)
			{
				lastoffs = StopMusic();
				IN_ReleaseMouse();
			}
			else
			{
				IN_GrabMouse();
				ContinueMusic(lastoffs);
				if (MousePresent && IN_IsInputGrabbed())
					IN_CenterMouse();     // Clear accumulated mouse movement
				ResetTimeCount();
			}
		}
	}
}

// This should be called once per frame
void ProcessEvents()
{
	IN_ProcessEvents();

//
// get timing info for last frame
//
	if (demoplayback || demorecord)   // demo recording and playback needs to be constant
	{
		// wait up to DEMOTICS Wolf tics
		uint32_t curtime = SDL_GetTicks();
		lasttimecount += DEMOTICS;
		int32_t timediff = TICS2MS(lasttimecount) - curtime;
		if(timediff > 0)
			SDL_Delay(timediff);

		if(timediff < -2 * DEMOTICS)       // more than 2-times DEMOTICS behind?
			lasttimecount = MS2TICS(curtime);    // yes, set to current timecount

		tics = DEMOTICS;
	}
	else
		CalcTics ();
}

//===========================================================================


void BumpGamma()
{
	screenGamma += 0.1f;
	if(screenGamma > 3.0f)
		screenGamma = 1.0f;
	screen->SetGamma(screenGamma);
	US_CenterWindow (10,2);
	FString msg;
	msg.Format("Gamma: %g", screenGamma);
	US_PrintCentered (msg);
	VW_UpdateScreen();
	IN_Ack(ACK_Block);
}

/*
=====================
=
= CheckKeys
=
= This should only cover control panel keys, debug mode key checks have been
= moved to CheckDebugKeys.
=
=====================
*/

void CheckKeys (void)
{
	static bool changeSize = true;
	ScanCode scan;


	if (screenfaded || demoplayback)    // don't do anything with a faded screen
		return;

	scan = LastScan;

	// [BL] Allow changing the screen size with the -/= keys a la Doom.
	if(automap != AMA_Normal && changeSize)
	{
		if(Keyboard[sc_Equals] && !Keyboard[sc_Minus])
			NewViewSize(viewsize+1);
		else if(!Keyboard[sc_Equals] && Keyboard[sc_Minus])
			NewViewSize(viewsize-1);
		if(Keyboard[sc_Equals] || Keyboard[sc_Minus])
		{
			SD_PlaySound("world/hitwall");
			if (viewsize < 21)
				DrawPlayScreen();
			changeSize = false;
		}
	}
	else if(!Keyboard[sc_Equals] && !Keyboard[sc_Minus])
		changeSize = true;

	if(Keyboard[sc_Alt] && Keyboard[sc_Enter])
		VL_ToggleFullscreen();

//
// F1-F7/ESC to enter control panel
//
	if (scan == sc_F10 ||
		scan == sc_F9 || scan == sc_F7 || scan == sc_F8)     // pop up quit dialog
	{
		ClearSplitVWB ();
		US_ControlPanel (scan);

		DrawPlayBorderSides ();

		IN_ClearKeysDown ();

		if(screenfaded && Net::IsBlocked())
			PlayFrame();
		return;
	}

	if ((scan >= sc_F1 && scan <= sc_F9) || scan == sc_Escape || control[ConsolePlayer].buttonstate[bt_esc])
	{
		int lastoffs = StopMusic ();
		SD_StopDigitized();

		US_ControlPanel (control[ConsolePlayer].buttonstate[bt_esc] ? sc_Escape : scan);

		IN_ClearKeysDown ();

		if(screenfaded)
		{
			if (!startgame && !loadedgame)
			{
				VW_FadeOut();
				ContinueMusic (lastoffs);
				if(viewsize != 21)
					DrawPlayScreen ();
			}
			if (loadedgame)
				playstate = ex_abort;
			if (MousePresent && IN_IsInputGrabbed())
				IN_CenterMouse();     // Clear accumulated mouse movement

			// If another player is blocking the play sim we may need to refresh
			// the frame now before we wait for input.
			if (Net::IsBlocked())
				PlayFrame();
		}
		else
		{
			ContinueMusic (lastoffs);
		}
		return;
	}

	if(scan == sc_F11)
	{
		BumpGamma();
		return;
	}
}


//===========================================================================

/*
=============================================================================

												MUSIC STUFF

=============================================================================
*/


/*
=================
=
= StopMusic
=
=================
*/
int StopMusic (void)
{
	return SD_MusicOff();
}

//==========================================================================


/*
=================
=
= StartMusic
=
=================
*/

void StartMusic ()
{
	SD_MusicOff ();
	SD_StartMusic(levelInfo->GetMusic(map));
}

void ContinueMusic (int offs)
{
	SD_MusicOff ();
	if(!(Paused & 1))
		SD_ContinueMusic(levelInfo->GetMusic(map), offs);
}

/*
=============================================================================

										PALETTE SHIFTING STUFF

=============================================================================
*/

#define NUMREDSHIFTS    6
#define REDSTEPS        8

#define NUMWHITESHIFTS  3
#define WHITESTEPS      20
#define WHITETICS       6

int damagecount, bonuscount;
bool palshifted;

/*
=====================
=
= ClearPaletteShifts
=
=====================
*/

void ClearPaletteShifts (void)
{
	bonuscount = damagecount = 0;
	palshifted = false;
}


/*
=====================
=
= StartBonusFlash
=
=====================
*/

void StartBonusFlash (void)
{
	bonuscount = NUMWHITESHIFTS * WHITETICS;    // white shift palette
}


/*
=====================
=
= StartDamageFlash
=
=====================
*/

void StartDamageFlash (int damage)
{
	damagecount += damage;
}


/*
=====================
=
= UpdatePaletteShifts
=
=====================
*/

void UpdatePaletteShifts (void)
{
	int red, white;

	if (bonuscount)
	{
		white = bonuscount / WHITETICS + 1;
		if (white > NUMWHITESHIFTS)
			white = NUMWHITESHIFTS;
		bonuscount -= tics;
		if (bonuscount < 0)
			bonuscount = 0;
	}
	else
		white = 0;


	if (damagecount)
	{
		red = damagecount / 10 + 1;
		if (red > NUMREDSHIFTS)
			red = NUMREDSHIFTS;

		damagecount -= tics;
		if (damagecount < 0)
			damagecount = 0;
	}
	else
		red = 0;

	if (red)
	{
		V_SetBlend(RPART(players[ConsolePlayer].mo->damagecolor),
                             GPART(players[ConsolePlayer].mo->damagecolor),
                             BPART(players[ConsolePlayer].mo->damagecolor), red*(174/NUMREDSHIFTS));
		palshifted = true;
	}
	else if (white)
	{
		// [BL] More of a yellow if you ask me.
		V_SetBlend(0xFF, 0xF8, 0x00, white*(38/NUMWHITESHIFTS));
		palshifted = true;
	}
	else if (palshifted)
	{
		V_SetBlend(0, 0, 0, 0);
		palshifted = false;
	}
}


/*
=====================
=
= FinishPaletteShifts
=
= Resets palette to normal if needed
=
=====================
*/

void FinishPaletteShifts (void)
{
	damagecount = 0;
	bonuscount = 0;

	if (palshifted)
	{
		V_SetBlend(0, 0, 0, 0);
		VH_UpdateScreen();
		palshifted = false;
	}
}


/*
=============================================================================

												CORE PLAYLOOP

=============================================================================
*/

/*
===================
=
= PlayFrame
=
===================
*/

void PlayFrame()
{
	UpdatePaletteShifts ();

	ThreeDRefresh ();

	if(automap && !gamestate.victoryflag)
		BasicOverhead();
	if(Paused & 1)
		VWB_DrawGraphic(TexMan("PAUSED"), (20 - 4)*8, 80 - 2*8);

	if(Net::IsBlocked())
	{
		ClearSplitVWB();
		Message("Waiting for players to return");
	}

	if (!loadedgame)
	{
		StatusBar->Tick();
		if ((gamestate.TimeCount & 1) || !(tics & 1))
			StatusBar->DrawStatusBar();
	}

	if (screenfaded)
	{
		VW_FadeIn ();
		ResetTimeCount();
	}

	VH_UpdateScreen();
}

/*
===================
=
= PlayLoop
=
===================
*/
int32_t funnyticount;


void PlayLoop (void)
{
#if 0 // USE_CLOUDSKY
	if(GetFeatureFlags() & FF_CLOUDSKY)
		InitSky();
#endif

	playstate = ex_stillplaying;
	ResetTimeCount();
	frameon = 0;
	funnyticount = 0;
	memset (control[ConsolePlayer].buttonstate, 0, sizeof (control[ConsolePlayer].buttonstate));
	ClearPaletteShifts ();

	if(automap != AMA_Off)
	{
			// Force the automap to off if it were previously on, unpause the game if am_pause
		automap = AMA_Off;

		if(am_pause) Paused &= ~2;
	}


	if (MousePresent && IN_IsInputGrabbed())
		IN_CenterMouse();         // Clear accumulated mouse movement

	if (demoplayback)
		IN_StartAck (ACK_Local);

	StatusBar->NewGame();

	do
	{
#ifdef _DIII4A //karin: loop control
        if(!q3e_running)
            break;
		if(!GLimp_CheckGLInitialized())
			continue;
#endif
		ProcessEvents();

//
// actor thinking
//
		madenoise = false;

		// Run tics
		for (unsigned int i = 0;i < tics;++i)
		{
			PollControls(!i);

			// Net code may require this loop to abort early
			if(playstate != ex_stillplaying)
				break;

			if(!Paused)
		{
				++gamestate.TimeCount;

				CheckSpawnPlayer();

				// In single player if the player dies only tick the pawn
				if(Net::InitVars.mode != Net::MODE_SinglePlayer || players[0].state != player_t::PST_DEAD)
				thinkerList.Tick();
				else
					thinkerList.Tick(ThinkerList::PLAYER);

				AActor::FinishSpawningActors();
			}
		}

		PlayFrame();

		//
		// MAKE FUNNY FACE IF BJ DOESN'T MOVE FOR AWHILE
		//
		funnyticount += tics;

		TexMan.UpdateAnimations(lasttimecount*14);
		GC::CheckGC();

		UpdateSoundLoc ();      // JAB

		CheckKeys ();
		CheckDebugKeys ();

//
// debug aids
//
		if (singlestep)
		{
			VW_WaitVBL (singlestep);
			ResetTimeCount();
		}
		if (extravbls)
			VW_WaitVBL (extravbls);

		if (demoplayback)
		{
			if (IN_CheckAck ())
			{
				IN_ClearKeysDown ();
				playstate = ex_abort;
			}
		}
	}
	while (!playstate && !startgame);

	if (playstate != ex_died)
		FinishPaletteShifts ();
}
