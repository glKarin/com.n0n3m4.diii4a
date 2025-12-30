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
#include "wl_iwad.h"
#include "state_machine.h"

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

//
// replacing refresh manager
//
bool noadaptive = false;
unsigned tics;

//
// control info
//
#define JoyAx(x) (32+(x<<1))
#define CS_AxisDigital -1

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

/*
=============================================================================

							TIMING

=============================================================================
*/


/*
=====================
=
= CalcTics
=
=====================
*/

void CalcTics()
{
}

void ResetTimeCount()
{
}

void Delay(int wolfticks)
{
#ifdef TODO
	if(wolfticks>0)
		SDL_Delay(wolfticks * 100 / 7);
#endif
}

/*
=============================================================================

							USER CONTROL

=============================================================================
*/

/*
=====================
=
= CheckKeys
=
=====================
*/

bool changeSize = true;


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

SDWORD damagecount, bonuscount;
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
= PlayLoop
=
===================
*/
int32_t funnyticount;


void PlayInit (void)
{
#if defined(USE_FEATUREFLAGS) && defined(USE_CLOUDSKY)
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
		IN_StartAck ();

	StatusBar->NewGame();
}

void ClearActions(void)
{
	TicCmd_t &cmd = control[ConsolePlayer];
	cmd.buttonstate[bt_use] = 0;
	cmd.buttonstate[bt_prevweapon] = 0;
	cmd.buttonstate[bt_nextweapon] = 0;
}

void PlayLoopA (void)
{
//
// actor thinking
//
	madenoise = false;

	// Run tics
	if(!(Paused & 2))
	{
		for (unsigned int i = 0;i < tics;++i)
		{
			++gamestate.TimeCount;
			thinkerList.Tick();
			AActor::FinishSpawningActors();
			ClearActions();
		}
	}

	UpdatePaletteShifts ();

	ThreeDRefresh ();

	if(automap && !gamestate.victoryflag)
		BasicOverhead();

	//
	// MAKE FUNNY FACE IF BJ DOESN'T MOVE FOR AWHILE
	//
	funnyticount += tics;

	TexMan.UpdateAnimations(GetTimeCount()*14);
	GC::CheckGC();

	UpdateSoundLoc ();      // JAB
}

bool PlayLoopB (wl_state_t *state)
{
	if (!loadedgame)
	{
		StatusBar->Tick();
		if ((gamestate.TimeCount & 1) || !(tics & 1))
			StatusBar->DrawStatusBar();
	}

	VH_UpdateScreen();
//
// debug aids
//
#ifdef TODO
	if (singlestep)
	{
		VW_WaitVBL (singlestep);
		ResetTimeCount();
	}
	if (extravbls)
		VW_WaitVBL (extravbls);
#endif
	if (demoplayback)
	{
		if (IN_CheckAck ())
		{
			IN_ClearKeysDown ();
			playstate = ex_abort;
		}
	}
	
	if (!playstate && !startgame)
	{
		state->stage = PLAY_STEP_A;
		return true;
	}

	if (playstate != ex_died)
		FinishPaletteShifts ();

	if(playstate == ex_victorious)
		state->stage = VICTORY_ZOOMER_START;
	else
		state->stage = GAME_END_MAP;
	return false;
}
