#include "wl_def.h"
#include "wl_play.h"
#include "am_map.h"
#include "id_in.h"
#include "id_vl.h"
#include "id_vh.h"
#include "w_wad.h"
#include "v_font.h"
#include "v_palette.h"
#include "v_video.h"
#include "r_data/r_translate.h"
#include "textures/textures.h"
#include "templates.h"

int	    pa=MENU_CENTER,px,py;

//==========================================================================

void VWB_DrawPropString(FFont *font, const char* string, EColorRange translation, bool stencil, BYTE stencilcolor)
{
	int		    width, height;
	FTexture	*source;
	byte	    ch;
	int cx = px, cy = py;

	height = font->GetHeight();
	FRemapTable *remap = font->GetColorTranslation(translation);

	while ((ch = (byte)*string++)!=0)
	{
		if(ch == '\n')
		{
			cy += height;
			cx = px;
			continue;
		}

		source = font->GetChar(ch, &width);
		if(source)
			VWB_DrawGraphic(source, cx, cy, (MenuOffset)pa, remap, stencil, stencilcolor);
		cx += width;
	}
}

// Prints a string with word wrapping
void VWB_DrawPropStringWrap(unsigned int wrapWidth, unsigned int wrapHeight, FFont *font, const char* string, EColorRange translation, bool stencil, BYTE stencilcolor)
{
	const char* lineStart = string;
	const char* lastBreak = string;
	unsigned int lastBreakX = 0;
	unsigned int cx = 0;
	char ch;

	while ((ch = (byte)*string++)!=0)
	{
		if(ch == '\n')
		{
			cx = 0;
			continue;
		}
		else if(ch == ' ' || ch == '\t')
		{
			lastBreak = string;
			lastBreakX = cx;
		}

		cx += font->GetCharWidth(ch);
		if(cx > wrapWidth)
		{
			FString part(lineStart, static_cast<int>(lastBreak-lineStart));
			VWB_DrawPropString(font, part, translation, stencil, stencilcolor);

			lineStart = lastBreak;
			cx -= lastBreakX;
			lastBreakX = 0;
			py += font->GetHeight();

			if((unsigned)py >= wrapHeight)
				break;
		}
	}

	// Flush the rest of the string.
	VWB_DrawPropString(font, lineStart, translation, stencil, stencilcolor);
	py += font->GetHeight();
}

void VW_MeasurePropString (FFont *font, const char *string, word &width, word &height, word *finalWidth)
{
	int w = 0;

	height = font->GetHeight();
	for(width = 0;*string;++string)
	{
		if(*string == '\n')
		{
			w = 0;
			height += font->GetHeight();
			continue;
		}

		w += font->GetCharWidth(*((byte *)string));
		if(w > width)
			width = w;
	}

	if(finalWidth)
		*finalWidth = w;
}

/*
=============================================================================

				Double buffer management routines

=============================================================================
*/
#ifndef LIBRETRO
#if SDL_VERSION_ATLEAST(2,0,0)
void Blit8BitSurfaceToTexture(SDL_Texture *tex, SDL_Surface *surf)
{
	void* pixels;
	int pitch;
	if(!SDL_LockTexture(tex, NULL, &pixels, &pitch))
	{
		if(!SDL_LockSurface(surf))
		{
			const SDL_Color* colors = surf->format->palette->colors;
			DWORD* dest = reinterpret_cast<DWORD*>(pixels);
			BYTE* src = reinterpret_cast<BYTE*>(surf->pixels);
			for(unsigned int y = 0;y < screenHeight;++y)
			{
				for(unsigned int x = 0;x < screenWidth;++x, ++src)
					*dest++ = (colors[*src].r<<16)|(colors[*src].g<<8)|(colors[*src].b);
				src += (screenWidth-surf->pitch);
				dest += (screenWidth-pitch/4);
			}
			SDL_UnlockSurface(surf);
		}
		else
			Printf("Can't lock surface!\n");
		SDL_UnlockTexture(tex);
	}
	else
		Printf("Can't lock texture!\n");
}
#endif
#endif

void VH_UpdateScreen()
{
	screen->Update();
	screen->Lock(false);
}

/*
=============================================================================

						WOLFENSTEIN STUFF

=============================================================================
*/

//==========================================================================

/*
===================
=
= FizzleFade
=
= returns true if aborted
=
= It uses maximum-length Linear Feedback Shift Registers (LFSR) counters.
= You can find a list of them with lengths from 3 to 168 at:
= http://www.xilinx.com/support/documentation/application_notes/xapp052.pdf
= Many thanks to Xilinx for this list!!!
=
===================
*/

// XOR masks for the pseudo-random number sequence starting with n=17 bits
static const uint32_t rndmasks[] = {
					// n    XNOR from (starting at 1, not 0 as usual)
	0x00012000,     // 17   17,14
	0x00020400,     // 18   18,11
	0x00040023,     // 19   19,6,2,1
	0x00090000,     // 20   20,17
	0x00140000,     // 21   21,19
	0x00300000,     // 22   22,21
	0x00420000,     // 23   23,18
	0x00e10000,     // 24   24,23,22,17
	0x01200000,     // 25   25,22      (this is enough for 8191x4095)
};

unsigned int rndbits_y;
unsigned int rndmask;

// Returns the number of bits needed to represent the given value
static int log2_ceil(uint32_t x)
{
	int n = 0;
	uint32_t v = 1;
	while(v < x)
	{
		n++;
		v <<= 1;
	}
	return n;
}

void VH_Startup()
{
	int rndbits_x = log2_ceil(screenWidth);
	rndbits_y = log2_ceil(screenHeight);

	int rndbits = rndbits_x + rndbits_y;
	if(rndbits < 17)
		rndbits = 17;       // no problem, just a bit slower
	else if(rndbits > 25)
		rndbits = 25;       // fizzle fade will not fill whole screen

	rndmask = rndmasks[rndbits - 17];

	AM_ChangeResolution();
}

byte *fizzleSurface = NULL;
void FizzleFadeStart()
{
	screen->Lock(false);
	fizzleSurface = new byte[SCREENHEIGHT*SCREENPITCH];
	memcpy(fizzleSurface, screen->GetBuffer(), SCREENHEIGHT*SCREENPITCH);
	screen->Unlock();
}
bool FizzleFade (int x1, int y1,
	unsigned width, unsigned height, unsigned frames, bool abortable)
{
	unsigned x, y, frame, pixperframe;
	int32_t  rndval=0;

	assert(fizzleSurface != NULL);

	pixperframe = width * height / frames;

	IN_StartAck ();

	frame = GetTimeCount();
	screen->Lock(false);
	byte * const srcptr = new byte[SCREENHEIGHT*SCREENPITCH];
	memcpy(srcptr, screen->GetBuffer(), SCREENHEIGHT*SCREENPITCH);
	screen->Unlock();

	do
	{
		IN_ProcessEvents();

		if(abortable && IN_CheckAck ())
		{
			VH_UpdateScreen();
			delete[] fizzleSurface;
			delete[] srcptr;
			fizzleSurface = NULL;
			return true;
		}

		byte *destptr = fizzleSurface;

		if(destptr != NULL)
		{
			for(unsigned p = 0; p < pixperframe; p++)
			{
				//
				// seperate random value into x/y pair
				//

				x = rndval >> rndbits_y;
				y = rndval & ((1 << rndbits_y) - 1);

				//
				// advance to next random element
				//

				rndval = (rndval >> 1) ^ (rndval & 1 ? 0 : rndmask);

				if(x >= width || y >= height)
				{
					if(rndval == 0)     // entire sequence has been completed
						goto finished;
					p--;
					continue;
				}

				//
				// copy one pixel
				//
				*(destptr + (y1 + y) * SCREENPITCH + x1 + x)
					= *(srcptr + (y1 + y) * SCREENPITCH + x1 + x);

				if(rndval == 0)		// entire sequence has been completed
					goto finished;
			}

			memcpy(screen->GetBuffer(), destptr, SCREENHEIGHT*SCREENPITCH);
			VH_UpdateScreen();
		}
		else
		{
			// No surface, so only enhance rndval
			for(unsigned p = 0; p < pixperframe; p++)
			{
				rndval = (rndval >> 1) ^ (rndval & 1 ? 0 : rndmask);
				if(rndval == 0)
					goto finished;
			}
		}

		frame++;
		Delay(frame - GetTimeCount());        // don't go too fast
	} while (1);

finished:
	for (y = y1; y < (y1 + height); ++y)
	{
		memcpy(fizzleSurface + (y * SCREENPITCH) + x1,
					srcptr + (y * SCREENPITCH) + x1,
					width); 
	}
	memcpy(screen->GetBuffer(), fizzleSurface, SCREENHEIGHT * SCREENPITCH);
	VH_UpdateScreen();
	delete[] fizzleSurface;
	delete[] srcptr;
	fizzleSurface = NULL;
	return false;
}

//==========================================================================

void VWB_Clear(int color, int x1, int y1, int x2, int y2)
{
	screen->Clear(x1, y1, x2, y2, color, GPalette.BaseColors[color]);
}

void VWB_DrawFill(FTexture *tex, int ix, int iy, int ix2, int iy2, bool local)
{
	screen->FlatFill(ix, iy, ix2, iy2, tex, local);
}

void VWB_DrawGraphic(FTexture *tex, int ix, int iy, double wd, double hd, MenuOffset menu, FRemapTable *remap, bool stencil, BYTE stencilcolor)
{
	double x = ix, y = iy;

	screen->Lock(false);
	if(menu)
		MenuToRealCoords(x, y, wd, hd, menu);
	else
		screen->VirtualToRealCoords(x, y, wd, hd, 320, 200, true, true);

	if(stencil)
	{
		screen->DrawTexture(tex, x, y,
			DTA_DestWidthF, wd,
			DTA_DestHeightF, hd,
			DTA_Translation, remap,
			DTA_FillColor, GPalette.BaseColors[stencilcolor].d,
			TAG_DONE);
	}
	else
	{
		screen->DrawTexture(tex, x, y,
			DTA_DestWidthF, wd,
			DTA_DestHeightF, hd,
			DTA_Translation, remap,
			TAG_DONE);
	}
	screen->Unlock();
}

void VWB_DrawGraphic(FTexture *tex, int ix, int iy, MenuOffset menu, FRemapTable *remap, bool stencil, BYTE stencilcolor)
{
	if(!tex)
		return;

	VWB_DrawGraphic(tex, ix, iy, tex->GetScaledWidthDouble(), tex->GetScaledHeightDouble(),
		menu, remap, stencil, stencilcolor);
}

void CA_CacheScreen(FTexture* tex, bool noaspect)
{
	screen->Lock(false);
	screen->Clear(0, 0, SCREENWIDTH, SCREENHEIGHT, GPalette.BlackIndex, 0);
	if(noaspect)
	{
		screen->DrawTexture(tex, 0, 0,
			DTA_DestWidth, SCREENWIDTH,
			DTA_DestHeight, SCREENHEIGHT,
			TAG_DONE);
	}
	else
	{
		screen->DrawTexture(tex, 0, 0,
			DTA_Fullscreen, true,
			TAG_DONE);
	}
	screen->Unlock();
}
