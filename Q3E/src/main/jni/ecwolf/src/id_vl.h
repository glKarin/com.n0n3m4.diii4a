// ID_VL.H

#ifndef __ID_VL_H__
#define __ID_VL_H__

//===========================================================================

extern  bool	fullscreen;
extern  unsigned screenWidth, screenHeight, screenBits, curPitch;
extern  unsigned fullScreenWidth, fullScreenHeight;
extern  unsigned windowedScreenWidth, windowedScreenHeight;
extern  unsigned scaleFactorX, scaleFactorY;
extern	float	screenGamma;

extern	bool  screenfaded;

//===========================================================================

//
// VGA hardware routines
//

#define VL_WaitVBL(a) SDL_Delay((SDL_GetTicks() - TICS2MS(GetTimeCount())) + TICS2MS((a)-1))

void VL_ToggleFullscreen();
void VL_SetFullscreen(bool isFull);

void VL_ReadPalette(const char* lump);

void VL_SetVGAPlaneMode (bool forSignon=false);
void VL_SetTextMode (void);

class FFader
{
public:
	virtual ~FFader() {}

	// Performs a fade step and returns true if fade is complete
	virtual bool Update()=0;
};

class FBlendFader : public FFader
{
	fixed start, end;
	int red, green, blue;
	int32_t fadems;
	int32_t startms;
	fixed aStep;

public:
	FBlendFader(int start, int end, int red, int green, int blue, int steps);

	bool Update();

	int R() const { return red; }
	int G() const { return green; }
	int B() const { return blue; }
};

void VL_FadeOut     (int start, int end, int red, int green, int blue, int steps);
void VL_FadeIn      (int start, int end, int steps);
void VL_FadeClear   ();

byte *VL_LockSurface();
void VL_UnlockSurface();

#define VL_ClearScreen(color) VWB_Clear(color, 0, 0, SCREENWIDTH, SCREENHEIGHT)

#endif
