// ID_VL.C

#include <string.h>
#include "c_cvars.h"
#include "colormatcher.h"
#include "wl_def.h"
#include "id_in.h"
#include "id_vl.h"
#include "id_vh.h"
#include "w_wad.h"
#include "r_2d/r_main.h"
#include "r_data/colormaps.h"
#include "v_font.h"
#include "v_video.h"
#include "v_palette.h"
#include "wl_draw.h"
#include "wl_game.h"
#include "wl_main.h"
#include "wl_play.h"


// Uncomment the following line, if you get destination out of bounds
// assertion errors and want to ignore them during debugging
//#define IGNORE_BAD_DEST

#ifdef IGNORE_BAD_DEST
#undef assert
#define assert(x) if(!(x)) return
#define assert_ret(x) if(!(x)) return 0
#else
#define assert_ret(x) assert(x)
#endif

bool fullscreen = true;
unsigned screenWidth = 640;
unsigned screenHeight = 480;
unsigned fullScreenWidth = 640;
unsigned fullScreenHeight = 480;
unsigned windowedScreenWidth = 640;
unsigned windowedScreenHeight = 480;
unsigned screenBits = static_cast<unsigned> (-1);      // use "best" color depth according to libSDL
float screenGamma = 1.0f;

unsigned curPitch;

unsigned scaleFactorX, scaleFactorY;

bool	 screenfaded;

//===========================================================================

void VL_ToggleFullscreen()
{
	VL_SetFullscreen(!fullscreen);
}

void VL_SetFullscreen(bool isFull)
{
	vid_fullscreen = fullscreen = isFull;

	if (fullscreen)
	{
		screenWidth = fullScreenWidth;
		screenHeight = fullScreenHeight;
	}
	else
	{
		screenWidth = windowedScreenWidth;
		screenHeight = windowedScreenHeight;
	}

	// Recalculate the aspect ratio, because this can change from fullscreen to windowed now
	r_ratio = static_cast<Aspect>(CheckRatio(screenWidth, screenHeight));
	VL_SetVGAPlaneMode();
	if(playstate)
	{
		DrawPlayScreen();
	}
	IN_AdjustMouse();
}

//===========================================================================

void VL_ReadPalette(const char* lump)
{
	InitPalette(lump);
	R_InitColormaps();
	TexMan.InvalidatePalette();
	V_RetranslateFonts();
}

/*
=======================
=
= VL_SetVGAPlaneMode
=
=======================
*/

void I_InitGraphics ();
void	VL_SetVGAPlaneMode (bool forSignon)
{
	if(!forSignon)
		screen->Unlock();

	I_InitGraphics();
	Video->SetResolution(screenWidth, screenHeight, 8);
	screen->Lock(true);
	R_SetupBuffer ();
	screen->Unlock();

	scaleFactorX = CleanXfac;
	scaleFactorY = CleanYfac;

	pixelangle = new short[SCREENWIDTH];
	wallheight = new int[SCREENWIDTH];

	NewViewSize(viewsize);

	screen->Lock(false);
}

/*
=============================================================================

						PALETTE OPS

		To avoid snow, do a WaitVBL BEFORE calling these

=============================================================================
*/

FBlendFader::FBlendFader(int start, int end, int red, int green, int blue, int steps)
: start(start<<FRACBITS), end(end<<FRACBITS), red(red), green(green),
  blue(blue), fadems(TICS2MS(steps)), startms(SDL_GetTicks()),
  aStep((this->end-this->start)/fadems)
{
}

bool FBlendFader::Update()
{
	int32_t curtime;
	if((curtime = SDL_GetTicks() - startms) < fadems)
	{
		V_SetBlend(red, green, blue, (start+curtime*aStep)>>FRACBITS);
		return false;
	}
	else
	{
		V_SetBlend(red, green, blue, end>>FRACBITS);
		return true;
	}
}

/*
=================
=
= VL_FadeOut
=
= Fades the current palette to the given color in the given number of steps
=
=================
*/

static FBlendFader fade(0, 0, 0, 0, 0, 1);
void VL_Fade (int start, int end, int red, int green, int blue, int steps)
{
	fade = FBlendFader(start, end, red, green, blue, steps);

	while(!fade.Update())
		VH_UpdateScreen();
	VH_UpdateScreen();

	screenfaded = end != 0;

	// Clear out any input at this point that may be stored up. This solves
	// issues such as starting facing the wrong angle in super 3d noah's ark.
	IN_ProcessEvents();
}

void VL_FadeOut (int start, int end, int red, int green, int blue, int steps)
{
	VL_Fade(start, end, red, green, blue, steps);
}


/*
=================
=
= VL_FadeIn
=
=================
*/

void VL_FadeIn (int start, int end, int steps)
{
	if(screenfaded)
		VL_Fade(end, start, fade.R(), fade.G(), fade.B(), steps);
}

/*
=================
=
= VL_FadeIn
= Match fade color and remove palette blend
=
=================
*/

void VL_FadeClear ()
{
	VWB_Clear(ColorMatcher.Pick(fade.R(), fade.G(), fade.B()), 0, 0, screenWidth, screenHeight);
	V_SetBlend(0, 0, 0, 0);
	VH_UpdateScreen();
}

/*
=============================================================================

							PIXEL OPS

=============================================================================
*/

byte *VL_LockSurface()
{
	screen->Lock(false);
	return (byte *) screen->GetBuffer();
}

void VL_UnlockSurface()
{
	screen->Unlock();
}
