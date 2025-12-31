// ID_VH.H

#ifndef __ID_VH_H__
#define __ID_VH_H__

#include "id_vl.h"
#include "tmemory.h"
#include "wl_main.h"
#include "v_font.h"

extern	int             pa,px,py;

//
// mode independant routines
// coordinates in pixels, rounded to best screen res
// regions marked in double buffer
//

class FTexture;
enum MenuOffset
{
	MENU_NONE = 0,
	MENU_TOP = 2,
	MENU_CENTER = 1,
	MENU_BOTTOM = 3
};
void VWB_Clear(int color, int x1, int y1, int x2, int y2);
static inline void VWB_Clear(int color, double x1, double y1, double x2, double y2)
{
	VWB_Clear(color, static_cast<int>(x1), static_cast<int>(y1), static_cast<int>(x2), static_cast<int>(y2));
}
void VWB_DrawFill(FTexture *tex, int ix, int iy, int ix2, int iy2, bool local=false);
static inline void VWB_DrawFill(FTexture *tex, double x, double y, double x2, double y2, bool local=false)
{
	VWB_DrawFill(tex, static_cast<int>(x), static_cast<int>(y), static_cast<int>(x2), static_cast<int>(y2), local);
}
void VWB_DrawGraphic(FTexture *tex, int ix, int iy, double wd, double hd, MenuOffset menu=MENU_NONE, struct FRemapTable *remap=NULL, bool stencil=false, BYTE stencilcolor=0);
void VWB_DrawGraphic(FTexture *tex, int ix, int iy, MenuOffset menu=MENU_NONE, struct FRemapTable *remap=NULL, bool stencil=false, BYTE stencilcolor=0);
template<class T> void MenuToRealCoords(T &x, T &y, T &w, T &h, MenuOffset offset)
{
	x = screenWidth/2 + (x-160)*scaleFactorX;
	switch(offset)
	{
		default:
			y = screenHeight/2 + (y-100)*scaleFactorY;
			break;
		case MENU_TOP:
			y *= scaleFactorY;
			break;
		case MENU_BOTTOM:
			y = screenHeight + (y-200)*scaleFactorY;
			break;
	}
	w *= scaleFactorX;
	h *= scaleFactorY;
}

void VWB_DrawPropString(FFont *font, const char *string, EColorRange translation=CR_UNTRANSLATED, bool stencil=false, BYTE stencilcolor=0);
void VWB_DrawPropStringWrap(unsigned int wrapWidth, unsigned int wrapHeight, FFont *font, const char *string, EColorRange translation=CR_UNTRANSLATED, bool stencil=false, BYTE stencilcolor=0);

void VH_UpdateScreen();
#define VW_UpdateScreen VH_UpdateScreen

//
// wolfenstein EGA compatability stuff
//

#define VW_WaitVBL		    VL_WaitVBL
#define VW_FadeIn()		    VL_FadeIn(0,255,30);
#define VW_FadeOut()	    VL_FadeOut(0,255,0,0,0,30);
void	VW_MeasurePropString (FFont *font, const char *string, word &width, word &height, word *finalWidth=NULL);

void    VH_Startup();

class FFizzleFader : public FFader
{
	TUniquePtr<byte[]> destptr;
	TUniquePtr<byte[]> srcptr;
	int x1, y1;
	unsigned width, height;
	int32_t fadems;
	int32_t startms;

	unsigned pixcovered;
	int32_t rndval;
	bool staticframes;

public:
	FFizzleFader(int x1, int y1, unsigned width, unsigned height, unsigned frames, bool staticframes);

	void CaptureFrame();
	void FadeToColor(int red, int green, int blue);
	bool Update();
};

void FizzleFade (FFader &fader);

#endif
