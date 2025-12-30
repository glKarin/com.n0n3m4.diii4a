#include "colormatcher.h"
#include "v_video.h"
#include "thingdef/thingdef.h"
#include "wl_main.h"

#define dimamount (-1.f)
#define dimcolor 0xffd700

extern "C" {
DWORD Col2RGB8[65][256];
DWORD *Col2RGB8_LessPrecision[65];
DWORD Col2RGB8_Inverse[65][256];
}

IMPLEMENT_ABSTRACT_CLASS (DCanvas)
IMPLEMENT_ABSTRACT_CLASS (DFrameBuffer)

#if defined(_DEBUG) && defined(_M_IX86)
#define DBGBREAK	{ __asm int 3 }
#else
#define DBGBREAK
#endif

class DDummyFrameBuffer : public DFrameBuffer
{
	DECLARE_CLASS (DDummyFrameBuffer, DFrameBuffer);
public:
	DDummyFrameBuffer (int width, int height)
		: DFrameBuffer (0, 0)
	{
		Width = width;
		Height = height;
	}
	bool Lock(bool buffered) { DBGBREAK; return false; }
	void Update() { DBGBREAK; }
	PalEntry *GetPalette() { DBGBREAK; return NULL; }
	void GetFlashedPalette(PalEntry palette[256]) { DBGBREAK; }
	void UpdatePalette() { DBGBREAK; }
	bool SetGamma(float gamma) { Gamma = gamma; return true; }
	bool SetFlash(PalEntry rgb, int amount) { DBGBREAK; return false; }
	void GetFlash(PalEntry &rgb, int &amount) { DBGBREAK; }
	int GetPageCount() { DBGBREAK; return 0; }
	bool IsFullscreen() { DBGBREAK; return 0; }
	void PaletteChanged() {}
	int QueryNewPalette() { return 0; }
	bool Is8BitMode() { return false; }

	float Gamma;
};
IMPLEMENT_INTERNAL_CLASS (DDummyFrameBuffer)

IMPLEMENT_INTERNAL_CLASS (DSimpleCanvas)

DCanvas *DCanvas::CanvasChain = NULL;

//==========================================================================
//
// DCanvas Constructor
//
//==========================================================================

DCanvas::DCanvas (int _width, int _height)
{
	// Init member vars
	Buffer = NULL;
	LockCount = 0;
	Width = _width;
	Height = _height;

	// Add to list of active canvases
	Next = CanvasChain;
	CanvasChain = this;
}

//==========================================================================
//
// DCanvas Destructor
//
//==========================================================================

DCanvas::~DCanvas ()
{
	// Remove from list of active canvases
	DCanvas *probe = CanvasChain, **prev;

	prev = &CanvasChain;
	probe = CanvasChain;

	while (probe != NULL)
	{
		if (probe == this)
		{
			*prev = probe->Next;
			break;
		}
		prev = &probe->Next;
		probe = probe->Next;
	}
}

//==========================================================================
//
// DCanvas :: IsValid
//
//==========================================================================

bool DCanvas::IsValid ()
{
	// A nun-subclassed DCanvas is never valid
	return false;
}

//==========================================================================
//
// DCanvas :: FlatFill
//
// Fill an area with a texture. If local_origin is false, then the origin
// used for the wrapping is (0,0). Otherwise, (left,right) is used.
//
//==========================================================================

void DCanvas::FlatFill (int left, int top, int right, int bottom, FTexture *src, bool local_origin)
{
	int w = src->GetWidth();
	int h = src->GetHeight();

	// Repeatedly draw the texture, left-to-right, top-to-bottom.
	for (int y = local_origin ? top : (top / h * h); y < bottom; y += h)
	{
		for (int x = local_origin ? left : (left / w * w); x < right; x += w)
		{
			DrawTexture (src, x, y,
				DTA_ClipLeft, left,
				DTA_ClipRight, right,
				DTA_ClipTop, top,
				DTA_ClipBottom, bottom,
				DTA_TopOffset, 0,
				DTA_LeftOffset, 0,
				TAG_DONE);
		}
	}
}

//==========================================================================
//
// DCanvas :: Dim
//
// Applies a colored overlay to the entire screen, with the opacity
// determined by the dimamount cvar.
//
//==========================================================================

void DCanvas::Dim (PalEntry color)
{
	PalEntry dimmer;
	float amount;

	//if (dimamount >= 0)
	//{
		dimmer = PalEntry(dimcolor);
		amount = dimamount;
	//}
	//else
	//{
		//dimmer = gameinfo.dimcolor;
		//amount = gameinfo.dimamount;
	//}

	// Add the cvar's dimming on top of the color passed to the function
	if (color.a != 0)
	{
		float dim[4] = { color.r/255.f, color.g/255.f, color.b/255.f, color.a/255.f };
		//V_AddBlend (dimmer.r/255.f, dimmer.g/255.f, dimmer.b/255.f, amount, dim);
		dimmer = PalEntry (BYTE(dim[0]*255), BYTE(dim[1]*255), BYTE(dim[2]*255));
		amount = dim[3];
	}
	Dim (dimmer, amount, 0, 0, Width, Height);
}

//==========================================================================
//
// DCanvas :: Dim
//
// Applies a colored overlay to an area of the screen.
//
//==========================================================================

void DCanvas::Dim (PalEntry color, float damount, int x1, int y1, int w, int h)
{
	if (damount == 0.f)
		return;

	DWORD *bg2rgb;
	DWORD fg;
	int gap;
	BYTE *spot;
	int x, y;

	if (x1 >= Width || y1 >= Height)
	{
		return;
	}
	if (x1 + w > Width)
	{
		w = Width - x1;
	}
	if (y1 + h > Height)
	{
		h = Height - y1;
	}
	if (w <= 0 || h <= 0)
	{
		return;
	}

	{
		int amount;

		amount = (int)(damount * 64);
		bg2rgb = Col2RGB8[64-amount];

		fg = (((color.r * amount) >> 4) << 20) |
			  ((color.g * amount) >> 4) |
			 (((color.b * amount) >> 4) << 10);
	}

	spot = Buffer + x1 + y1*Pitch;
	gap = Pitch - w;
	for (y = h; y != 0; y--)
	{
		for (x = w; x != 0; x--)
		{
			DWORD bg;

			bg = bg2rgb[(*spot)&0xff];
			bg = (fg+bg) | 0x1f07c1f;
			*spot = RGB32k[0][0][bg&(bg>>15)];
			spot++;
		}
		spot += gap;
	}
}

//==========================================================================
//
// DCanvas :: GetScreenshotBuffer
//
// Returns a buffer containing the most recently displayed frame. The
// width and height of this buffer are the same as the canvas.
//
//==========================================================================

void DCanvas::GetScreenshotBuffer(const BYTE *&buffer, int &pitch, ESSType &color_type)
{
	Lock(true);
	buffer = GetBuffer();
	pitch = GetPitch();
	color_type = SS_PAL;
}

//==========================================================================
//
// DCanvas :: ReleaseScreenshotBuffer
//
// Releases the buffer obtained through GetScreenshotBuffer. These calls
// must not be nested.
//
//==========================================================================

void DCanvas::ReleaseScreenshotBuffer()
{
	Unlock();
}

//==========================================================================
//
// DCanvas :: CalcGamma
//
//==========================================================================

void DCanvas::CalcGamma (float gamma, BYTE gammalookup[256])
{
	// I found this formula on the web at
	// <http://panda.mostang.com/sane/sane-gamma.html>,
	// but that page no longer exits.

	double invgamma = 1.f / gamma;
	int i;

	for (i = 0; i < 256; i++)
	{
		gammalookup[i] = (BYTE)(255.0 * pow (i / 255.0, invgamma));
	}
}

//==========================================================================
//
// DSimpleCanvas Constructor
//
// A simple canvas just holds a buffer in main memory.
//
//==========================================================================

DSimpleCanvas::DSimpleCanvas (int width, int height)
	: DCanvas (width, height)
{
	// Making the pitch a power of 2 is very bad for performance
	// Try to maximize the number of cache lines that can be filled
	// for each column drawing operation by making the pitch slightly
	// longer than the width. The values used here are all based on
	// empirical evidence.

	if (width <= 640)
	{
		// For low resolutions, just keep the pitch the same as the width.
		// Some speedup can be seen using the technique below, but the speedup
		// is so marginal that I don't consider it worthwhile.
		Pitch = width;
	}
	else
	{
#if 0
		// If we couldn't figure out the CPU's L1 cache line size, assume
		// it's 32 bytes wide.
		if (CPU.DataL1LineSize == 0)
		{
			CPU.DataL1LineSize = 32;
		}
		// The Athlon and P3 have very different caches, apparently.
		// I am going to generalize the Athlon's performance to all AMD
		// processors and the P3's to all non-AMD processors. I don't know
		// how smart that is, but I don't have a vast plethora of
		// processors to test with.
		if (CPU.bIsAMD)
		{
			Pitch = width + CPU.DataL1LineSize;
		}
		else
		{
			Pitch = width + MAX(0, CPU.DataL1LineSize - 8);
		}
#endif
		Pitch = width + 32 - 8;
	}
	MemBuffer = new BYTE[Pitch * height];
	memset (MemBuffer, 0, Pitch * height);
}

//==========================================================================
//
// DSimpleCanvas Destructor
//
//==========================================================================

DSimpleCanvas::~DSimpleCanvas ()
{
	if (MemBuffer != NULL)
	{
		delete[] MemBuffer;
		MemBuffer = NULL;
	}
}

//==========================================================================
//
// DSimpleCanvas :: IsValid
//
//==========================================================================

bool DSimpleCanvas::IsValid ()
{
	return (MemBuffer != NULL);
}

//==========================================================================
//
// DSimpleCanvas :: Lock
//
//==========================================================================

bool DSimpleCanvas::Lock (bool)
{
	if (LockCount == 0)
	{
		Buffer = MemBuffer;
	}
	LockCount++;
	return false;		// System surfaces are never lost
}

//==========================================================================
//
// DSimpleCanvas :: Unlock
//
//==========================================================================

void DSimpleCanvas::Unlock ()
{
	if (--LockCount <= 0)
	{
		LockCount = 0;
		Buffer = NULL;	// Enforce buffer access only between Lock/Unlock
	}
}

//==========================================================================
//
// DFrameBuffer Constructor
//
// A frame buffer canvas is the most common and represents the image that
// gets drawn to the screen.
//
//==========================================================================

DFrameBuffer::DFrameBuffer (int width, int height)
	: DSimpleCanvas (width, height)
{
	LastMS = LastSec = FrameCount = LastCount = LastTic = 0;
	Accel2D = false;
}

//==========================================================================
//
// DFrameBuffer :: DrawRateStuff
//
// Draws the fps counter, dot ticker, and palette debug.
//
//==========================================================================

void DFrameBuffer::DrawRateStuff ()
{
#if 0
	// Draws frame time and cumulative fps
	if (vid_fps)
	{
		DWORD ms = I_FPSTime();
		DWORD howlong = ms - LastMS;
		if ((signed)howlong >= 0)
		{
			char fpsbuff[40];
			int chars;
			int rate_x;

			chars = mysnprintf (fpsbuff, countof(fpsbuff), "%2u ms (%3u fps)", howlong, LastCount);
			rate_x = Width - chars * 8;
			Clear (rate_x, 0, Width, 8, GPalette.BlackIndex, 0);
			DrawText (ConFont, CR_WHITE, rate_x, 0, (char *)&fpsbuff[0], TAG_DONE);

			DWORD thisSec = ms/1000;
			if (LastSec < thisSec)
			{
				LastCount = FrameCount / (thisSec - LastSec);
				LastSec = thisSec;
				FrameCount = 0;
			}
			FrameCount++;
		}
		LastMS = ms;
	}

	// draws little dots on the bottom of the screen
	if (ticker)
	{
		int i = I_GetTime(false);
		int tics = i - LastTic;
		BYTE *buffer = GetBuffer();

		LastTic = i;
		if (tics > 20) tics = 20;

		// Buffer can be NULL if we're doing hardware accelerated 2D
		if (buffer != NULL)
		{
			buffer += (GetHeight()-1) * GetPitch();
			
			for (i = 0; i < tics*2; i += 2)		buffer[i] = 0xff;
			for ( ; i < 20*2; i += 2)			buffer[i] = 0x00;
		}
		else
		{
			for (i = 0; i < tics*2; i += 2)		Clear(i, Height-1, i+1, Height, 255, 0);
			for ( ; i < 20*2; i += 2)			Clear(i, Height-1, i+1, Height, 0, 0);
		}
	}

	// draws the palette for debugging
	if (vid_showpalette)
	{
		// This used to just write the palette to the display buffer.
		// With hardware-accelerated 2D, that doesn't work anymore.
		// Drawing it as a texture does and continues to show how
		// well the PalTex shader is working.
		static FPaletteTester palette;

		palette.SetTranslation(vid_showpalette);
		DrawTexture(&palette, 0, 0,
			DTA_DestWidth, 16*7,
			DTA_DestHeight, 16*7,
			DTA_Masked, false,
			TAG_DONE);
	}
#endif
}

//==========================================================================
//
// DFrameBuffer :: CopyFromBuff
//
// Copies pixels from main memory to video memory. This is only used by
// DDrawFB.
//
//==========================================================================

void DFrameBuffer::CopyFromBuff (BYTE *src, int srcPitch, int width, int height, BYTE *dest)
{
	if (Pitch == width && Pitch == Width && srcPitch == width)
	{
		memcpy (dest, src, Width * Height);
	}
	else
	{
		for (int y = 0; y < height; y++)
		{
			memcpy (dest, src, width);
			dest += Pitch;
			src += srcPitch;
		}
	}
}

//==========================================================================
//
// DFrameBuffer :: SetVSync
//
// Turns vertical sync on and off, if supported.
//
//==========================================================================

void DFrameBuffer::SetVSync (bool vsync)
{
}

//==========================================================================
//
// DFrameBuffer :: NewRefreshRate
//
// Sets the fullscreen display to the new refresh rate in vid_refreshrate,
// if possible.
//
//==========================================================================

void DFrameBuffer::NewRefreshRate ()
{
}

//==========================================================================
//
// DFrameBuffer :: SetBlendingRect
//
// Defines the area of the screen containing the 3D view.
//
//==========================================================================

void DFrameBuffer::SetBlendingRect (int x1, int y1, int x2, int y2)
{
}

//==========================================================================
//
// DFrameBuffer :: Begin2D
//
// Signal that 3D rendering is complete, and the rest of the operations on
// the canvas until Unlock() will be 2D ones.
//
//==========================================================================

bool DFrameBuffer::Begin2D (bool copy3d)
{
	return false;
}

//==========================================================================
//
// DFrameBuffer :: DrawBlendingRect
//
// In hardware 2D modes, the blending rect needs to be drawn separately
// from transferring the 3D scene to video memory, because the weapon
// sprite is drawn on top of that.
//
//==========================================================================

void DFrameBuffer::DrawBlendingRect()
{
}

//==========================================================================
//
// DFrameBuffer :: CreateTexture
//
// Creates a native texture for a game texture, if supported.
//
//==========================================================================

FNativeTexture *DFrameBuffer::CreateTexture(FTexture *gametex, bool wrapping)
{
	return NULL;
}

//==========================================================================
//
// DFrameBuffer :: CreatePalette
//
// Creates a native palette from a remap table, if supported.
//
//==========================================================================

FNativePalette *DFrameBuffer::CreatePalette(FRemapTable *remap)
{
	return NULL;
}

//==========================================================================
//
// DFrameBuffer :: WipeStartScreen
//
// Grabs a copy of the screen currently displayed to serve as the initial
// frame of a screen wipe. Also determines which screenwipe will be
// performed.
//
//==========================================================================

bool DFrameBuffer::WipeStartScreen(int type)
{
	return false;
//	return wipe_StartScreen(type);
}

//==========================================================================
//
// DFrameBuffer :: WipeEndScreen
//
// Grabs a copy of the most-recently drawn, but not yet displayed, screen
// to serve as the final frame of a screen wipe.
//
//==========================================================================

void DFrameBuffer::WipeEndScreen()
{
#if 0
	wipe_EndScreen();
	Unlock();
#endif
}

//==========================================================================
//
// DFrameBuffer :: WipeDo
//
// Draws one frame of a screenwipe. Should be called no more than 35
// times per second. If called less than that, ticks indicates how many
// ticks have passed since the last call.
//
//==========================================================================

bool DFrameBuffer::WipeDo(int ticks)
{
#if 0
	Lock(true);
	return wipe_ScreenWipe(ticks);
#endif
	return false;
}

//==========================================================================
//
// DFrameBuffer :: WipeCleanup
//
//==========================================================================

void DFrameBuffer::WipeCleanup()
{
#if 0
	wipe_Cleanup();
#endif
}

//===========================================================================
//
// Create texture hitlist
//
//===========================================================================

void DFrameBuffer::GetHitlist(BYTE *hitlist)
{
#if 0
	BYTE *spritelist;
	int i;

	spritelist = new BYTE[sprites.Size()];
	
	// Precache textures (and sprites).
	memset (spritelist, 0, sprites.Size());

	{
		AActor *actor;
		TThinkerIterator<AActor> iterator;

		while ( (actor = iterator.Next ()) )
			spritelist[actor->sprite] = 1;
	}

	for (i = (int)(sprites.Size () - 1); i >= 0; i--)
	{
		if (spritelist[i])
		{
			int j, k;
			for (j = 0; j < sprites[i].numframes; j++)
			{
				const spriteframe_t *frame = &SpriteFrames[sprites[i].spriteframes + j];

				for (k = 0; k < 16; k++)
				{
					FTextureID pic = frame->Texture[k];
					if (pic.isValid())
					{
						hitlist[pic.GetIndex()] = 1;
					}
				}
			}
		}
	}

	delete[] spritelist;

	for (i = numsectors - 1; i >= 0; i--)
	{
		hitlist[sectors[i].GetTexture(sector_t::floor).GetIndex()] = 
			hitlist[sectors[i].GetTexture(sector_t::ceiling).GetIndex()] |= 2;
	}

	for (i = numsides - 1; i >= 0; i--)
	{
		hitlist[sides[i].GetTexture(side_t::top).GetIndex()] =
		hitlist[sides[i].GetTexture(side_t::mid).GetIndex()] =
		hitlist[sides[i].GetTexture(side_t::bottom).GetIndex()] |= 1;
	}

	// Sky texture is always present.
	// Note that F_SKY1 is the name used to
	//	indicate a sky floor/ceiling as a flat,
	//	while the sky texture is stored like
	//	a wall texture, with an episode dependant
	//	name.

	if (sky1texture.isValid())
	{
		hitlist[sky1texture.GetIndex()] |= 1;
	}
	if (sky2texture.isValid())
	{
		hitlist[sky2texture.GetIndex()] |= 1;
	}
#endif
}

//==========================================================================
//
// DFrameBuffer :: GameRestart
//
//==========================================================================

void DFrameBuffer::GameRestart()
{
}

//===========================================================================
//
// 
//
//===========================================================================

FNativePalette::~FNativePalette()
{
}

FNativeTexture::~FNativeTexture()
{
}

bool FNativeTexture::CheckWrapping(bool wrapping)
{
	return true;
}

// -----------------------------------------------------------------------------

int DisplayWidth, DisplayHeight, DisplayBits;
BYTE RGB32k[32][32][32];

// [RH] The framebuffer is no longer a mere byte array.
// There's also only one, not four.
DFrameBuffer *screen;

void GenerateLookupTables()
{
	static DWORD Col2RGB8_2[63][256];

	// create the RGB555 lookup table
	for(int r = 0;r < 32;r++)
		for(int g = 0;g < 32;g++)
			for(int b = 0;b < 32;b++)
				RGB32k[r][g][b] = ColorMatcher.Pick((r<<3)|(r>>2), (g<<3)|(g>>2), (b<<3)|(b>>2));

	// create the swizzled palette
	for (int x = 0; x < 65; x++)
		for (int y = 0; y < 256; y++)
			Col2RGB8[x][y] = (((GPalette.BaseColors[y].r*x)>>4)<<20) |
							  ((GPalette.BaseColors[y].g*x)>>4) |
							 (((GPalette.BaseColors[y].b*x)>>4)<<10);

	// create the swizzled palette with the lsb of red and blue forced to 0
	// (for green, a 1 is okay since it never gets added into)
	for (int x = 1; x < 64; x++)
	{
		Col2RGB8_LessPrecision[x] = Col2RGB8_2[x-1];
		for (int y = 0; y < 256; y++)
		{
			Col2RGB8_2[x-1][y] = Col2RGB8[x][y] & 0x3feffbff;
		}
	}
	Col2RGB8_LessPrecision[0] = Col2RGB8[0];
	Col2RGB8_LessPrecision[64] = Col2RGB8[64];

	// create the inverse swizzled palette
	for (int x = 0; x < 65; x++)
		for (int y = 0; y < 256; y++)
		{
			Col2RGB8_Inverse[x][y] = (((((255-GPalette.BaseColors[y].r)*x)>>4)<<20) |
									  (((255-GPalette.BaseColors[y].g)*x)>>4) |
									  ((((255-GPalette.BaseColors[y].b)*x)>>4)<<10)) & 0x3feffbff;
		}
}

int ParseHex(const char* hex)
{
	int num = 0;
	int i = 1;
	do
	{
		int digit;
		char cdigit = *hex++;
		// Switch to uppercase for valid range
		if(cdigit >= 'a' && cdigit <= 'f')
			cdigit += 'A'-'a';
		// Check range and convert to integer
		if(cdigit >= '0' && cdigit <= '9')
			digit = cdigit - '0';
		else if(cdigit >= 'A' && cdigit <= 'F')
			digit = cdigit - 'A' + 0xA;
		else
			return 0;

		num |= digit<<(4*i);
		--i;
	}
	while(i >= 0 && *hex != '\0');

	return num;
}

//==========================================================================
//
// V_GetColorFromString
//
// Passed a string of the form "#RGB", "#RRGGBB", "R G B", or "RR GG BB",
// returns a number representing that color. If palette is non-NULL, the
// index of the best match in the palette is returned, otherwise the
// RRGGBB value is returned directly.
//
//==========================================================================

int V_GetColorFromString (const DWORD *palette, const char *cstr)
{
	int c[3], i, p;
	char val[3];

	val[2] = '\0';

	// Check for HTML-style #RRGGBB or #RGB color string
	if (cstr[0] == '#')
	{
		size_t len = strlen (cstr);

		if (len == 7)
		{
			// Extract each eight-bit component into c[].
			for (i = 0; i < 3; ++i)
			{
				val[0] = cstr[1 + i*2];
				val[1] = cstr[2 + i*2];
				c[i] = ParseHex (val);
			}
		}
		else if (len == 4)
		{
			// Extract each four-bit component into c[], expanding to eight bits.
			for (i = 0; i < 3; ++i)
			{
				val[1] = val[0] = cstr[1 + i];
				c[i] = ParseHex (val);
			}
		}
		else
		{
			// Bad HTML-style; pretend it's black.
			c[2] = c[1] = c[0] = 0;
		}
	}
	else
	{
		if (strlen(cstr) == 6)
		{
			char *p;
			int color = strtol(cstr, &p, 16);
			if (*p == 0)
			{
				// RRGGBB string
				c[0] = (color & 0xff0000) >> 16;
				c[1] = (color & 0xff00) >> 8;
				c[2] = (color & 0xff);
			}
			else goto normal;
		}
		else
		{
normal:
			// Treat it as a space-delemited hexadecimal string
			for (i = 0; i < 3; ++i)
			{
				// Skip leading whitespace
				while (*cstr <= ' ' && *cstr != '\0')
				{
					cstr++;
				}
				// Extract a component and convert it to eight-bit
				for (p = 0; *cstr > ' '; ++p, ++cstr)
				{
					if (p < 2)
					{
						val[p] = *cstr;
					}
				}
				if (p == 0)
				{
					c[i] = 0;
				}
				else
				{
					if (p == 1)
					{
						val[1] = val[0];
					}
					c[i] = ParseHex (val);
				}
			}
		}
	}
	if (palette)
		return ColorMatcher.Pick (c[0], c[1], c[2]);
	else
		return MAKERGB(c[0], c[1], c[2]);
}

//==========================================================================
//
// V_GetColor
//
// Works like V_GetColorFromString(), but also understands X11 color names.
//
//==========================================================================

int V_GetColor (const DWORD *palette, const char *str)
{
	return V_GetColorFromString(palette, str);
#if 0
	FString string = V_GetColorStringByName (str);
	int res;

	if (!string.IsEmpty())
	{
		res = V_GetColorFromString (palette, string);
	}
	else
	{
		res = V_GetColorFromString (palette, str);
	}
	return res;
#endif
}

void V_CalcCleanFacs (int designwidth, int designheight, int realwidth, int realheight, int *cleanx, int *cleany, int *_cx1, int *_cx2)
{
	int ratio;
	int cwidth;
	int cheight;
	int cx1, cy1, cx2, cy2;

	ratio = CheckRatio(realwidth, realheight);
	if (ratio & 4)
	{
		cwidth = realwidth;
		cheight = realheight * AspectCorrection[ratio].multiplier / 48;
	}
	else
	{
		cwidth = realwidth * AspectCorrection[ratio].multiplier / 48;
		cheight = realheight;
	}
	// Use whichever pair of cwidth/cheight or width/height that produces less difference
	// between CleanXfac and CleanYfac.
	cx1 = MAX(cwidth / designwidth, 1);
	cy1 = MAX(cheight / designheight, 1);
	cx2 = MAX(realwidth / designwidth, 1);
	cy2 = MAX(realheight / designheight, 1);
	if (abs(cx1 - cy1) <= abs(cx2 - cy2))
	{ // e.g. 640x360 looks better with this.
		*cleanx = cx1;
		*cleany = cy1;
	}
	else
	{ // e.g. 720x480 looks better with this.
		*cleanx = cx2;
		*cleany = cy2;
	}

	// 1600x1200 can do clean aspect correction so why not?
	if (*cleany % 6 == 0)
	{
		int newXfactor = *cleany - *cleany/6;
		if(newXfactor <= *cleanx)
			*cleanx = newXfactor;
		else
			*cleany = *cleanx;
	}
	else if (*cleanx > 1 && *cleany > 1 && *cleanx != *cleany)
	{
		if (*cleanx < *cleany)
			*cleany = *cleanx;
		else
			*cleanx = *cleany;
	}
	if (_cx1 != NULL)	*_cx1 = cx1;
	if (_cx2 != NULL)	*_cx2 = cx2;
}
