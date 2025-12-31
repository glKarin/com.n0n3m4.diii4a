/*
** v_font.cpp
** Font management
**
**---------------------------------------------------------------------------
** Copyright 1998-2008 Randy Heit
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

/* Special file formats handled here:

FON1 "console" fonts have the following header:
	char  Magic[4];			-- The characters "FON1"
	uword CharWidth;		-- Character cell width
	uword CharHeight;		-- Character cell height

The FON1 header is followed by RLE character data for all 256
8-bit ASCII characters.


FON2 "standard" fonts have the following header:
	char  Magic[4];			-- The characters "FON2"
	uword FontHeight;		-- Every character in a font has the same height
	ubyte FirstChar;		-- First character defined by this font.
	ubyte LastChar;			-- Last character definde by this font.
	ubyte bConstantWidth;
	ubyte ShadingType;
	ubyte PaletteSize;		-- size of palette in entries (not bytes!)
	ubyte Flags;

There is presently only one flag for FON2:
	FOF_WHOLEFONTKERNING 1	-- The Kerning field is present in the file

The FON2 header is followed by variable length data:
	word  Kerning;
		-- only present if FOF_WHOLEFONTKERNING is set

	ubyte Palette[PaletteSize+1][3];
		-- The last entry is the delimiter color. The delimiter is not used
		-- by the font but is used by imagetool when converting the font
		-- back to an image. Color 0 is the transparent color and is also
		-- used only for converting the font back to an image. The other
		-- entries are all presorted in increasing order of brightness.

	ubyte CharacterData[...];
		-- RLE character data, in order
*/

// HEADER FILES ------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <limits.h>

#include "templates.h"
#include "wl_def.h"
#include "m_swap.h"
#include "v_font.h"
#include "v_video.h"
#include "w_wad.h"
//#include "i_system.h"
//#include "cmdlib.h"
#include "scanner.h"
//#include "hu_stuff.h"
#include "farchive.h"
#include "textures/textures.h"
#include "r_data/r_translate.h"
#include "colormatcher.h"
#include "v_palette.h"
#include "zdoomsupport.h"
#include "doomerrors.h"

//
// Globally visible constants.
//
#define HU_FONTSTART	BYTE('!')		// the first font characters
#define HU_FONTEND		BYTE('\377')	// the last font characters

// Calculate # of glyphs in font.
#define HU_FONTSIZE		(HU_FONTEND - HU_FONTSTART + 1)

FFont *SmallFont, *SmallFont2, *BigFont, *ConFont, *IntermissionFont, *Tile8Font;

// MACROS ------------------------------------------------------------------

#define DEFAULT_LOG_COLOR	PalEntry(223,223,223)

// TYPES -------------------------------------------------------------------
void RecordTextureColors (FTexture *pic, BYTE *colorsused);

// This structure is used by BuildTranslations() to hold color information.
struct TranslationParm
{
	short RangeStart;	// First level for this range
	short RangeEnd;		// Last level for this range
	BYTE Start[3];		// Start color for this range
	BYTE End[3];		// End color for this range
};

struct TranslationMap
{
	FName Name;
	int Number;
};

class FSingleLumpFont : public FFont
{
public:
	FSingleLumpFont (const char *fontname, int lump);

protected:
	void CheckFON1Chars (double *luminosity);
	void BuildTranslations2 ();
	void FixupPalette (BYTE *identity, double *luminosity, const BYTE *palette,
		bool rescale, PalEntry *out_palette);
	void LoadTranslations ();
	void LoadFON1 (int lump, const BYTE *data);
	void LoadFON2 (int lump, const BYTE *data);
	void LoadBMF (int lump, const BYTE *data);
	void LoadTile8 (int lump, const BYTE *data);
	bool LoadWolfFont (int lump, const BYTE *data, size_t length);
	void CreateFontFromPic (FTextureID picnum);

	static int STACK_ARGS BMFCompare(const void *a, const void *b);

	enum
	{
		FONT1,
		FONT2,
		BMFFONT,
		WOLFFONT,
		ROTTCFONT,
		TILE8FONT
	} FontType;
	BYTE PaletteData[768];
	bool RescalePalette;
};

class FSinglePicFont : public FFont
{
public:
	FSinglePicFont(const char *picname);

	// FFont interface
	FTexture *GetChar (int code, int *const width) const;
	int GetCharWidth (int code) const;

protected:
	FTextureID PicNum;
};

// Essentially a normal multilump font but with an explicit list of character patches
class FSpecialFont : public FFont
{
public:
	FSpecialFont (const char *name, int first, int count, FTextureID *lumplist, const bool *notranslate, int lump);

	void LoadTranslations();

protected:
	bool notranslate[256];
};


// This is a font character that loads a texture and recolors it.
class FFontChar1 : public FFontTexture
{
public:
   FFontChar1 (FTexture *sourcelump);
   const BYTE *GetColumn (unsigned int column, const Span **spans_out);
   const BYTE *GetPixels ();
   void SetSourceRemap(const BYTE *sourceremap);
   void Unload ();
   ~FFontChar1 ();

protected:
   void MakeTexture ();

   FTexture *BaseTexture;
   BYTE *Pixels;
   const BYTE *SourceRemap;
};

// This is a font character that reads RLE compressed data.
class FFontChar2 : public FFontTexture
{
public:
	FFontChar2 (int sourcelump, int sourcepos, int width, int height, int leftofs=0, int topofs=0);
	~FFontChar2 ();

	const BYTE *GetColumn (unsigned int column, const Span **spans_out);
	const BYTE *GetPixels ();
	void SetSourceRemap(const BYTE *sourceremap);
	void Unload ();

protected:
	int SourceLump;
	int SourcePos;
	BYTE *Pixels;
	Span **Spans;
	const BYTE *SourceRemap;

	virtual void MakeTexture ();
};

class FTile8Char : public FFontChar2
{
public:
	FTile8Char (int sourcelump, int sourcepos, int width, int height, int maskcolor=-1) :
		FFontChar2(sourcelump, sourcepos, width, height), MaskColor(maskcolor)
	{
	}

protected:
	void MakeTexture ();

	int MaskColor;
};

class FROTTChar : public FFontChar2
{
public:
	FROTTChar (int sourcelump, int sourcepos, int width, int height) :
		FFontChar2(sourcelump, sourcepos, width, height)
	{
	}

protected:
	void MakeTexture ();
};

struct TempParmInfo
{
	unsigned int StartParm[3];
	unsigned int ParmLen[3];
	int Index;
};
struct TempColorInfo
{
	FName Name;
	unsigned int ParmInfo;
	PalEntry LogColor;
};

// EXTERNAL FUNCTION PROTOTYPES --------------------------------------------

// PUBLIC FUNCTION PROTOTYPES ----------------------------------------------

// PRIVATE FUNCTION PROTOTYPES ---------------------------------------------

static int STACK_ARGS TranslationMapCompare (const void *a, const void *b);

// EXTERNAL DATA DECLARATIONS ----------------------------------------------

extern int PrintColors[];

// PUBLIC DATA DEFINITIONS -------------------------------------------------

FFont *FFont::FirstFont = NULL;
int NumTextColors;

// PRIVATE DATA DEFINITIONS ------------------------------------------------

static TArray<TranslationParm> TranslationParms[3];
static TArray<TranslationMap> TranslationLookup;
static TArray<PalEntry> TranslationColors;

// CODE --------------------------------------------------------------------

static bool myislower(int code)
{
	if (code >= 'a' && code <= 'z')
		return true;
	if (code != 0xF7 && code != 0xFF && (code & 0xE0) == 0xE0)
		return true;
	return false;
}

// Returns a character without an accent mark.
// FIXME: Only valid for CP-1252; we should go Unicode at some point.

static int stripaccent(int code)
{
	if (code < 0x8a)
		return code;
	if (code == 0x8a)	// Latin capital letter S with caron
		return 'S';
	if (code == 0x8e)	// Latin capital letter Z with caron
		return 'Z';
	if (code == 0x9a)	// Latin small letter S with caron
		return 's';
	if (code == 0x9e)	// Latin small letter Z with caron
		return 'z';
	if (code == 0x9f)	// Latin capital letter Y with diaeresis
		return 'Y';
	if (code == 0xff)	// Latin small letter Y with diaeresis
		return 'y';
	// Every other accented character has the high two bits set.
	if ((code & 0xC0) == 0)
		return code;
	// Make lowercase characters uppercase so there are half as many tests.
	int acode = code & 0xDF;
	if (acode >= 0xC0 && acode <= 0xC5)		// A with accents
		return 'A' + (code & 0x20);
	if (acode == 0xC7)						// Cedilla
		return 'C' + (acode & 0x20);
	if (acode >= 0xC8 && acode <= 0xCB)		// E with accents
		return 'E' + (code & 0x20);
	if (acode >= 0xCC && acode <= 0xCF)		// I with accents
		return 'I' + (code & 0x20);
	if (acode == 0xD0)						// Eth
		return 'D' + (code & 0x20);
	if (acode == 0xD1)						// N with tilde
		return 'N' + (code & 0x20);
	if ((acode >= 0xD2 && acode <= 0xD6) ||	// O with accents
		acode == 0xD8)						// O with stroke
		return 'O' + (code & 0x20);
	if (acode >= 0xD9 && acode <= 0xDC)		// U with accents
		return 'U' + (code & 0x20);
	if (acode == 0xDD)						// Y with accute
		return 'Y' + (code & 0x20);
	if (acode == 0xDE)						// Thorn
		return 'P' + (code & 0x20);			// well, it sort of looks like a 'P'
	return code;
}

FFont *V_GetFont(const char *name)
{
	FFont *font = FFont::FindFont (name);
	if (font == NULL)
	{
		int lump = -1;

		lump = Wads.CheckNumForFullName(name, true);
		
		if (lump != -1)
		{
			try
			{
				font = new FSingleLumpFont (name, lump);
			}
			catch(class CDoomError &)
			{
			}
		}
		if (font == NULL)
		{
			FTextureID picnum = TexMan.CheckForTexture (name, FTexture::TEX_Any);
			if (picnum.isValid())
			{
				font = new FSinglePicFont (name);
			}
		}
	}
	return font;
}
//==========================================================================
//
// SerializeFFontPtr
//
//==========================================================================

FArchive &SerializeFFontPtr (FArchive &arc, FFont* &font)
{
	if (arc.IsStoring ())
	{
		arc << font->Name;
	}
	else
	{
		char *name = NULL;

		arc << name;
		font = V_GetFont(name);
		if (font == NULL)
		{
			Printf ("Could not load font %s\n", name);
			font = SmallFont;
		}
		delete[] name;
	}
	return arc;
}

//==========================================================================
//
// FFont :: FFont
//
// Loads a multi-texture font.
//
//==========================================================================

FFont::FFont (const char *name, const char *nametemplate, int first, int count, int start, int fdlump, int spacewidth)
{
	int i;
	FTextureID lump;
	char buffer[12];
	FTexture **charlumps;
	int maxyoffs;
	bool stcfn121 = false;

	Lump = fdlump;
	Chars = new CharData[count];
	charlumps = new FTexture *[count];
	PatchRemap = new BYTE[256];
	FirstChar = first;
	LastChar = first + count - 1;
	FontHeight = 0;
	GlobalKerning = false;
	Name = copystring (name);
	Next = FirstFont;
	FirstFont = this;
	Cursor = '_';

	maxyoffs = 0;

	for (i = 0; i < count; i++)
	{
		charlumps[i] = NULL;
		mysnprintf (buffer, countof(buffer), nametemplate, i + start);

		lump = TexMan.CheckForTexture(buffer, FTexture::TEX_MiscPatch);

		if (lump.isValid())
		{
			FTexture *pic = TexMan[lump];
			if (pic != NULL)
			{
				// set the lump here only if it represents a valid texture
				if (i != 124-start || !stcfn121)
					charlumps[i] = pic;

				int height = pic->GetScaledHeight();
				int yoffs = pic->GetScaledTopOffset();

				if (yoffs > maxyoffs)
				{
					maxyoffs = yoffs;
				}
				height += abs (yoffs);
				if (height > FontHeight)
				{
					FontHeight = height;
				}
			}
		}

		if (charlumps[i] != NULL)
		{
			Chars[i].Pic = new FFontChar1 (charlumps[i]);
			Chars[i].ID = lump;
			Chars[i].XMove = Chars[i].Pic->GetScaledWidth();
		}
		else
		{
			Chars[i].Pic = NULL;
			Chars[i].ID.SetInvalid();
			Chars[i].XMove = INT_MIN;
		}
	}

	if (spacewidth != -1)
	{
		SpaceWidth = spacewidth;
	}
	else if ('N'-first >= 0 && 'N'-first < count && Chars['N' - first].Pic != NULL)
	{
		SpaceWidth = (Chars['N' - first].XMove + 1) / 2;
	}
	else
	{
		SpaceWidth = 4;
	}

	FixXMoves();

	LoadTranslations();

	delete[] charlumps;
}

//==========================================================================
//
// FFont :: ~FFont
//
//==========================================================================

FFont::~FFont ()
{
	if (Chars)
	{
		int count = LastChar - FirstChar + 1;

		// Delete any textures we haven't directly added to our texture manager
		for (int i = 0; i < count; ++i)
		{
			if (Chars[i].Pic != NULL && Chars[i].Pic->Name[0] == 0 && TexMan[Chars[i].ID] != Chars[i].Pic)
			{
				delete Chars[i].Pic;
			}
		}
		delete[] Chars;
		Chars = NULL;
	}
	if (PatchRemap)
	{
		delete[] PatchRemap;
		PatchRemap = NULL;
	}
	if (Name)
	{
		delete[] Name;
		Name = NULL;
	}

	FFont **prev = &FirstFont;
	FFont *font = *prev;

	while (font != NULL && font != this)
	{
		prev = &font->Next;
		font = *prev;
	}

	if (font != NULL)
	{
		*prev = font->Next;
	}
}

//==========================================================================
//
// FFont :: FindFont
//
// Searches for the named font in the list of loaded fonts, returning the
// font if it was found. The disk is not checked if it cannot be found.
//
//==========================================================================

FFont *FFont::FindFont (const char *name)
{
	if (name == NULL)
	{
		return NULL;
	}
	FFont *font = FirstFont;

	while (font != NULL)
	{
		if (stricmp (font->Name, name) == 0)
			break;
		font = font->Next;
	}
	return font;
}

//==========================================================================
//
// RecordTextureColors
//
// Given a 256 entry buffer, sets every entry that corresponds to a color
// used by the texture to 1.
//
//==========================================================================

void RecordTextureColors (FTexture *pic, BYTE *usedcolors)
{
	int x;

	for (x = pic->GetWidth() - 1; x >= 0; x--)
	{
		const FTexture::Span *spans;
		const BYTE *column = pic->GetColumn (x, &spans);

		while (spans->Length != 0)
		{
			const BYTE *source = column + spans->TopOffset;
			int count = spans->Length;

			do
			{
				usedcolors[*source++] = 1;
			} while (--count);

			spans++;
		}
	}
}

//==========================================================================
//
// compare
//
// Used for sorting colors by brightness.
//
//==========================================================================

static int STACK_ARGS compare (const void *arg1, const void *arg2)
{
	if (RPART(GPalette.BaseColors[*((BYTE *)arg1)]) * 299 +
		GPART(GPalette.BaseColors[*((BYTE *)arg1)]) * 587 +
		BPART(GPalette.BaseColors[*((BYTE *)arg1)]) * 114  <
		RPART(GPalette.BaseColors[*((BYTE *)arg2)]) * 299 +
		GPART(GPalette.BaseColors[*((BYTE *)arg2)]) * 587 +
		BPART(GPalette.BaseColors[*((BYTE *)arg2)]) * 114)
		return -1;
	else
		return 1;
}

//==========================================================================
//
// FFont :: SimpleTranslation
//
// Colorsused, translation, and reverse must all be 256 entry buffers.
// Colorsused must already be filled out.
// Translation be set to remap the source colors to a new range of
// consecutive colors based at 1 (0 is transparent).
// Reverse will be just the opposite of translation: It maps the new color
// range to the original colors.
// *Luminosity will be an array just large enough to hold the brightness
// levels of all the used colors, in consecutive order. It is sorted from
// darkest to lightest and scaled such that the darkest color is 0.0 and
// the brightest color is 1.0.
// The return value is the number of used colors and thus the number of
// entries in *luminosity.
//
//==========================================================================

int FFont::SimpleTranslation (BYTE *colorsused, BYTE *translation, BYTE *reverse, double **luminosity)
{
	double min, max, diver;
	int i, j;

	memset (translation, 0, 256);

	reverse[0] = 0;
	for (i = 1, j = 1; i < 256; i++)
	{
		if (colorsused[i])
		{
			reverse[j++] = i;
		}
	}

	qsort (reverse+1, j-1, 1, compare);

	*luminosity = new double[j];
	(*luminosity)[0] = 0.0; // [BL] Prevent uninitalized memory
	max = 0.0;
	min = 100000000.0;
	for (i = 1; i < j; i++)
	{
		translation[reverse[i]] = i;

		(*luminosity)[i] = RPART(GPalette.BaseColors[reverse[i]]) * 0.299 +
						   GPART(GPalette.BaseColors[reverse[i]]) * 0.587 +
						   BPART(GPalette.BaseColors[reverse[i]]) * 0.114;
		if ((*luminosity)[i] > max)
			max = (*luminosity)[i];
		if ((*luminosity)[i] < min)
			min = (*luminosity)[i];
	}
	diver = 1.0 / (max - min);
	for (i = 1; i < j; i++)
	{
		(*luminosity)[i] = ((*luminosity)[i] - min) * diver;
	}

	return j;
}

//==========================================================================
//
// FFont :: BuildTranslations
//
// Build color translations for this font. Luminosity is an array of
// brightness levels. The ActiveColors member must be set to indicate how
// large this array is. Identity is an array that remaps the colors to
// their original values; it is only used for CR_UNTRANSLATED. Ranges
// is an array of TranslationParm structs defining the ranges for every
// possible color, in order. Palette is the colors to use for the
// untranslated version of the font.
//
//==========================================================================

void FFont::BuildTranslations (const double *luminosity, const BYTE *identity,
							   const void *ranges, int total_colors, const PalEntry *palette)
{
	int i, j;
	const TranslationParm *parmstart = (const TranslationParm *)ranges;

	FRemapTable remap(total_colors);

	// Create different translations for different color ranges
	Ranges.Clear();
	for (i = 0; i < NumTextColors; i++)
	{
		if (i == CR_UNTRANSLATED)
		{
			if (identity != NULL)
			{
				memcpy (remap.Remap, identity, ActiveColors);
				if (palette != NULL)
				{
					memcpy (remap.Palette, palette, ActiveColors*sizeof(PalEntry));
				}
				else
				{
					remap.Palette[0] = GPalette.BaseColors[identity[0]] & MAKEARGB(0,255,255,255);
					for (j = 1; j < ActiveColors; ++j)
					{
						remap.Palette[j] = GPalette.BaseColors[identity[j]] | MAKEARGB(255,0,0,0);
					}
				}
			}
			else
			{
				remap = Ranges[0];
			}
			Ranges.Push(remap);
			continue;
		}

		assert(parmstart->RangeStart >= 0);

		remap.Remap[0] = 0;
		remap.Palette[0] = 0;

		for (j = 1; j < ActiveColors; j++)
		{
			int v = int(luminosity[j] * 256.0);

			// Find the color range that this luminosity value lies within.
			const TranslationParm *parms = parmstart - 1;
			do
			{
				parms++;
				if (parms->RangeStart <= v && parms->RangeEnd >= v)
					break;
			}
			while (parms[1].RangeStart > parms[0].RangeEnd);

			// Linearly interpolate to find out which color this luminosity level gets.
			int rangev = ((v - parms->RangeStart) << 8) / (parms->RangeEnd - parms->RangeStart);
			int r = ((parms->Start[0] << 8) + rangev * (parms->End[0] - parms->Start[0])) >> 8; // red
			int g = ((parms->Start[1] << 8) + rangev * (parms->End[1] - parms->Start[1])) >> 8; // green
			int b = ((parms->Start[2] << 8) + rangev * (parms->End[2] - parms->Start[2])) >> 8; // blue
			r = clamp(r, 0, 255);
			g = clamp(g, 0, 255);
			b = clamp(b, 0, 255);
			remap.Remap[j] = ColorMatcher.Pick(r, g, b);
			remap.Palette[j] = PalEntry(255,r,g,b);
		}
		Ranges.Push(remap);

		// Advance to the next color range.
		while (parmstart[1].RangeStart > parmstart[0].RangeEnd)
		{
			parmstart++;
		}
		parmstart++;
	}
}

//==========================================================================
//
// FFont :: GetColorTranslation
//
//==========================================================================

FRemapTable *FFont::GetColorTranslation (EColorRange range) const
{
	if (ActiveColors == 0)
		return NULL;
	else if (range >= NumTextColors)
		range = CR_UNTRANSLATED;
	return &Ranges[range];
}

//==========================================================================
//
// FFont :: GetCharCode
//
// If the character code is in the font, returns it. If it is not, but it
// is lowercase and has an uppercase variant present, return that. Otherwise
// return -1.
//
//==========================================================================

int FFont::GetCharCode(int code, bool needpic) const
{
	if (code < 0 && code >= -128)
	{
		// regular chars turn negative when the 8th bit is set.
		code &= 255;
	}
	if (code >= FirstChar && code <= LastChar && (!needpic || Chars[code - FirstChar].Pic != NULL))
	{
		return code;
	}
	// Try converting lowercase characters to uppercase.
	if (myislower(code))
	{
		code -= 32;
		if (code >= FirstChar && code <= LastChar && (!needpic || Chars[code - FirstChar].Pic != NULL))
		{
			return code;
		}
	}
	// Try stripping accents from accented characters.
	int newcode = stripaccent(code);
	if (newcode != code)
	{
		code = newcode;
		if (code >= FirstChar && code <= LastChar && (!needpic || Chars[code - FirstChar].Pic != NULL))
		{
			return code;
		}
	}
	return -1;
}

//==========================================================================
//
// FFont :: GetChar
//
//==========================================================================

FTexture *FFont::GetChar (int code, int *const width) const
{
	code = GetCharCode(code, false);
	//Printf("Got code %X %c\n", code, code);
	int xmove = SpaceWidth;

	if (code >= 0)
	{
		code -= FirstChar;
		xmove = Chars[code].XMove;
		if (Chars[code].Pic == NULL)
		{
			code = GetCharCode(code + FirstChar, true);
			if (code >= 0)
			{
				code -= FirstChar;
				xmove = Chars[code].XMove;
			}
		}
	}
	if (width != NULL)
	{
		*width = xmove;
	}
	return (code < 0) ? NULL : Chars[code].Pic;
}

FTextureID FFont::GetCharID (int code) const
{
	code = GetCharCode(code, false);
	if (code >= 0)
	{
		code -= FirstChar;
		if (Chars[code].Pic == NULL)
		{
			code = GetCharCode(code + FirstChar, true);
			if (code >= 0)
				code -= FirstChar;
		}
	}

	if (code < 0)
	{
		FTextureID id;
		id.SetInvalid();
		return id;
	}
	return Chars[code].ID;
}

//==========================================================================
//
// FFont :: GetCharWidth
//
//==========================================================================

int FFont::GetCharWidth (int code) const
{
	code = GetCharCode(code, false);
	return (code < 0) ? SpaceWidth : Chars[code - FirstChar].XMove;
}

//==========================================================================
//
// FFont :: LoadTranslations
//
//==========================================================================

void FFont::LoadTranslations()
{
	unsigned int count = LastChar - FirstChar + 1;
	BYTE usedcolors[256], identity[256];
	double *luminosity;

	memset (usedcolors, 0, 256);
	for (unsigned int i = 0; i < count; i++)
	{
		FFontChar1 *pic = static_cast<FFontChar1 *>(Chars[i].Pic);
		if(pic)
		{
			pic->SetSourceRemap(NULL); // Force the FFontChar1 to return the same pixels as the base texture
			RecordTextureColors (pic, usedcolors);
		}
	}

	ActiveColors = SimpleTranslation (usedcolors, PatchRemap, identity, &luminosity);

	for (unsigned int i = 0; i < count; i++)
	{
		if(Chars[i].Pic)
			static_cast<FFontChar1 *>(Chars[i].Pic)->SetSourceRemap(PatchRemap);
	}

	BuildTranslations (luminosity, identity, &TranslationParms[0][0], ActiveColors, NULL);

	delete[] luminosity;
}

//==========================================================================
//
// FFont :: Preload
//
// Loads most of the 7-bit ASCII characters. In the case of D3DFB, this
// means all the characters of a font have a better chance of being packed
// into the same hardware texture.
//
//==========================================================================

void FFont::Preload() const
{
#if 0
	// First and last char are the same? Wait until it's actually needed
	// since nothing is gained by preloading now.
	if (FirstChar == LastChar)
	{
		return;
	}
	for (int i = MAX(FirstChar, 0x21); i < MIN(LastChar, 0x7e); ++i)
	{
		int foo;
		FTexture *pic = GetChar(i, &foo);
		if (pic != NULL)
		{
			pic->GetNative(false);
		}
	}
#endif
}

//==========================================================================
//
// FFont :: StaticPreloadFonts
//
// Preloads all the defined fonts.
//
//==========================================================================

void FFont::StaticPreloadFonts()
{
	for (FFont *font = FirstFont; font != NULL; font = font->Next)
	{
		font->Preload();
	}
}

//==========================================================================
//
// FFont :: FFont - default constructor
//
//==========================================================================

FFont::FFont (int lump)
{
	Lump = lump;
	Chars = NULL;
	PatchRemap = NULL;
	Name = NULL;
	Cursor = '_';
}

//==========================================================================
//
// FSingleLumpFont :: FSingleLumpFont
//
// Loads a FON1 or FON2 font resource.
//
//==========================================================================

FSingleLumpFont::FSingleLumpFont (const char *name, int lump) : FFont(lump)
{
	assert(lump >= 0);

	Name = copystring (name);

	FMemLump data1 = Wads.ReadLump (lump);
	const BYTE *data = (const BYTE *)data1.GetMem();

	if (data[0] == 0xE1 && data[1] == 0xE6 && data[2] == 0xD5 && data[3] == 0x1A)
	{
		LoadBMF(lump, data);
	}
	else if (data[0] != 'F' || data[1] != 'O' || data[2] != 'N' ||
		(data[3] != '1' && data[3] != '2'))
	{
		if(!LoadWolfFont(lump, data, data1.GetSize()))
		{
			// FMemLump adds one to GetSize so account for it.
			if((data1.GetSize() - 1)%64 == 0)
			{
				LoadTile8(lump, data);
			}
			else
				I_FatalError ("%s is not a recognizable font", name);
		}
	}
	else
	{
		switch (data[3])
		{
		case '1':
			LoadFON1 (lump, data);
			break;

		case '2':
			LoadFON2 (lump, data);
			break;
		}
	}

	Next = FirstFont;
	FirstFont = this;

	// To allow access to characters outside of the font system, we need to add
	// our textures to the texture manager.
	for (unsigned int i = LastChar - FirstChar + 1;i-- > 0;)
	{
		if(Chars[i].Pic)
		{
			static_cast<FFontTexture*>(Chars[i].Pic)->SourceFont = this;
			Chars[i].Pic->Name.Format(FONT_CHAR_NAME "%X", i + FirstChar);
			Chars[i].ID = TexMan.AddTexture(Chars[i].Pic);
		}
	}
}

//==========================================================================
//
// FSingleLumpFont :: CreateFontFromPic
//
//==========================================================================

void FSingleLumpFont::CreateFontFromPic (FTextureID picnum)
{
	FTexture *pic = TexMan[picnum];

	FontHeight = pic->GetHeight ();
	SpaceWidth = pic->GetWidth ();
	GlobalKerning = 0;

	FirstChar = LastChar = 'A';
	Chars = new CharData[1];
	Chars->Pic = pic;

	// Only one color range. Don't bother with the others.
	ActiveColors = 0;
}

//==========================================================================
//
// FSingleLumpFont :: LoadTranslations
//
//==========================================================================

void FSingleLumpFont::LoadTranslations()
{
	double luminosity[256];
	BYTE identity[256];
	PalEntry local_palette[256];
	bool useidentity = true;
	bool usepalette = false;
	const void* ranges = NULL;
	unsigned int count = LastChar - FirstChar + 1;

	switch(FontType)
	{
		case FONT1:
			useidentity = false;
			ranges = &TranslationParms[1][0];
			CheckFON1Chars (luminosity);
			break;

		case BMFFONT:
		case FONT2:
			usepalette = true;
			FixupPalette (identity, luminosity, PaletteData, RescalePalette, local_palette);

			ranges = &TranslationParms[0][0];
			break;

		case TILE8FONT:
		{
			BYTE colorsused[256] = {};
			BYTE translation[256];
			BYTE tempIdentity[256];
			double* tempLuminosity;

			ranges = &TranslationParms[0][0];

			for(unsigned int i = 0;i < count;++i)
			{
				if(Chars[i].Pic)
					RecordTextureColors(Chars[i].Pic, colorsused);
			}

			for(unsigned int i = 0;i < 256;++i)
			{
				identity[i] = i;
				luminosity[i] = 0;
			}

			unsigned int tempColors = SimpleTranslation(colorsused, translation, tempIdentity, &tempLuminosity);
			while(tempColors-- > 0)
				luminosity[tempIdentity[tempColors]] = tempLuminosity[tempColors];
			delete[] tempLuminosity;
			break;
		}

		case WOLFFONT:
			ranges = &TranslationParms[2][0];
			for(unsigned int i = 0;i < 256;++i)
				luminosity[i] = i != 0 ? 0.5 : 0.0;
			memset(identity, GPalette.BlackIndex, 256);
			break;

		case ROTTCFONT:
		{
			BYTE colorsused[256] = {};

			useidentity = false;
			ranges = &TranslationParms[0][0];
			for(unsigned int i = 0;i < count;++i)
			{
				if(Chars[i].Pic)
					RecordTextureColors(Chars[i].Pic, colorsused);
			}

			colorsused[0xFE] = 0; // Don't count the mask color
			ActiveColors = 0;
			unsigned int lastcolor = 0;
			for(unsigned int i = 0;i < 256;++i)
			{
				if(!colorsused[i])
					continue;

				++ActiveColors;
				lastcolor = i;
			}

			// We'll be loading the texture with +1 on all the colors, so
			// calculate luminosity shifted.
			for(unsigned int i = 0, j = 0;i < 256;++i)
				luminosity[i] = ((double)(i-1))/lastcolor;
			break;
		}

		default:
			// Should be unreachable.
			I_Error("Unknown font type in FSingleLumpFont::LoadTranslation.");
			break;
	}

	for(unsigned int i = 0;i < count;++i)
	{
		if(Chars[i].Pic)
			static_cast<FFontChar2*>(Chars[i].Pic)->SetSourceRemap(PatchRemap);
	}

	BuildTranslations (luminosity, useidentity ? identity : NULL, ranges, ActiveColors, usepalette ? local_palette : NULL);
}

//==========================================================================
//
// FSingleLumpFont :: LoadFON1
//
// FON1 is used for the console font.
//
//==========================================================================

void FSingleLumpFont::LoadFON1 (int lump, const BYTE *data)
{
	int w, h;

	Chars = new CharData[256];

	w = data[4] + data[5]*256;
	h = data[6] + data[7]*256;

	FontType = FONT1;
	FontHeight = h;
	SpaceWidth = w;
	FirstChar = 0;
	LastChar = 255;
	GlobalKerning = 0;
	PatchRemap = new BYTE[256];

	for(unsigned int i = 0;i < 256;++i)
		Chars[i].Pic = NULL;

	LoadTranslations();
}

//==========================================================================
//
// FSingleLumpFont :: LoadFON2
//
// FON2 is used for everything but the console font. The console font should
// probably use FON2 as well, but oh well.
//
//==========================================================================

void FSingleLumpFont::LoadFON2 (int lump, const BYTE *data)
{
	int count, i, totalwidth;
	int *widths2;
	WORD *widths;
	const BYTE *palette;
	const BYTE *data_p;

	FontType = FONT2;
	FontHeight = data[4] + data[5]*256;
	FirstChar = data[6];
	LastChar = data[7];
	ActiveColors = data[10]+1;
	PatchRemap = NULL;
	RescalePalette = data[9] == 0;
	
	count = LastChar - FirstChar + 1;
	Chars = new CharData[count];
	widths2 = new int[count];
	if (data[11] & 1)
	{ // Font specifies a kerning value.
		GlobalKerning = LittleShort(*(SWORD *)&data[12]);
		widths = (WORD *)(data + 14);
	}
	else
	{ // Font does not specify a kerning value.
		GlobalKerning = 0;
		widths = (WORD *)(data + 12);
	}
	totalwidth = 0;

	if (data[8])
	{ // Font is mono-spaced.
		totalwidth = LittleShort(widths[0]);
		for (i = 0; i < count; ++i)
		{
			widths2[i] = totalwidth;
		}
		totalwidth *= count;
		palette = (BYTE *)&widths[1];
	}
	else
	{ // Font has varying character widths.
		for (i = 0; i < count; ++i)
		{
			widths2[i] = LittleShort(widths[i]);
			totalwidth += widths2[i];
		}
		palette = (BYTE *)(widths + i);
	}

	if (FirstChar <= ' ' && LastChar >= ' ')
	{
		SpaceWidth = widths2[' '-FirstChar];
	}
	else if (FirstChar <= 'N' && LastChar >= 'N')
	{
		SpaceWidth = (widths2['N' - FirstChar] + 1) / 2;
	}
	else
	{
		SpaceWidth = totalwidth * 2 / (3 * count);
	}

	memcpy(PaletteData, palette, ActiveColors*3);

	data_p = palette + ActiveColors*3;

	for (i = 0; i < count; ++i)
	{
		int destSize = widths2[i] * FontHeight;
		Chars[i].XMove = widths2[i];
		if (destSize <= 0)
		{
			Chars[i].Pic = NULL;
		}
		else
		{
			Chars[i].Pic = new FFontChar2 (lump, int(data_p - data), widths2[i], FontHeight);
			do
			{
				SBYTE code = *data_p++;
				if (code >= 0)
				{
					data_p += code+1;
					destSize -= code+1;
				}
				else if (code != -128)
				{
					data_p++;
					destSize -= (-code)+1;
				}
			} while (destSize > 0);
		}
		if (destSize < 0)
		{
			i += FirstChar;
			I_FatalError ("Overflow decompressing char %d (%c) of %s", i, i, Name);
		}
	}

	LoadTranslations();
	delete[] widths2;
}

//==========================================================================
//
// FSingleLumpFont :: LoadTile8
//
//==========================================================================

void FSingleLumpFont::LoadTile8(int lump, const BYTE *data)
{
	FontType = TILE8FONT;
	FontHeight = 8;
	SpaceWidth = 8;
	FirstChar = ' '-8;
	LastChar = 0x97;
	GlobalKerning = 0;
	ActiveColors = 256;
	PatchRemap = NULL;

	unsigned int count = LastChar - FirstChar + 1;
	Chars = new CharData[count];

	const int tile8size = Wads.LumpLength(lump);

	for(unsigned int i = 0;i < count;++i)
	{
		int pos;
		int mask = -1;

		if(i <= 7)
			pos = i*64;
		else if(i >= (unsigned int)('0'-FirstChar) && i <= (unsigned int)('9'-FirstChar))
			pos = (i-('0'-FirstChar)+9)*64;
		// Mask off background color in S3DNA automap things (0x71-0x78 = Player, 0x7A+ = Things)
		else if((i >= 0x71u-FirstChar && i <= 0x78u-FirstChar) || (i >= 0x7Au-FirstChar))
		{
			mask = 215;
			pos = (i-('A'-FirstChar)+19)*64;
		}
		else if(i >= (unsigned int)('A'-FirstChar))
			pos = (i-('A'-FirstChar)+19)*64;
		else
			pos = 8*64;

		Chars[i].Pic = pos < tile8size ? new FTile8Char (lump, pos, SpaceWidth, FontHeight, mask) : NULL;
		Chars[i].XMove = 8;
	}

	LoadTranslations();
}

//==========================================================================
//
// FSingleLumpFont :: LoadWolfFont
//
//==========================================================================

bool FSingleLumpFont::LoadWolfFont(int lump, const BYTE *data, size_t length)
{
	if(length < 770)
		return false;

	uint16_t location[256];
	uint8_t width[256];

	FirstChar = 0;
	LastChar = 0;

	// Copy header information and then do a sanity check
	FontHeight = LittleShort(*(const int16_t *)data);
	if(FontHeight == 0 && length >= 1540 )
	{
		// ROTT defines the first byte as "color" but doesn't seem to read it.
		// We'll probably need to change this check later since you just know
		// somebody didn't put zero here, but I haven't really heard of ROTT
		// mods either. Byte 2 seems to be padding.
		FontType = ROTTCFONT;
		FontHeight = LittleShort(*(const int16_t *)(data+2));
		memcpy(width, data+229, 31);
		memcpy(width+31, data+4, 225);
		memcpy(location, data+710, 62);
		memcpy(location+31, data+260, 450);
		// data+772 through data+1540 should be a palette but ROTT seems to not use it.
	}
	else
	{
		FontType = WOLFFONT;
		memcpy(location, data+2, 512);
		memcpy(width, data+514, 256);
	}

	for(int i = 0;i < 256;++i)
	{
		// Find first and last defined characters
		if(width[i] == 0 && i == FirstChar) FirstChar = i+1;
		if(width[i] != 0) LastChar = i;

		location[i] = LittleShort(location[i]);
		if(location[i] + (unsigned)FontHeight*width[i] >= length)
			return false;
	}

	// Find any characters?
	if(LastChar < FirstChar)
		return false;

	PatchRemap = NULL;
	RescalePalette = false;
	Chars = new CharData[LastChar-FirstChar+1];
	SpaceWidth = width[(int)' '];
	GlobalKerning = 0;
	ActiveColors = 256;
	Cursor = (char)0x80;
	for(int i = FirstChar;i <= LastChar;++i)
	{
		CharData &c = Chars[i-FirstChar];
		c.Pic = FontType == ROTTCFONT
			? new FROTTChar (lump, location[i], width[i], FontHeight)
			: new FFontChar2 (lump, location[i], width[i], FontHeight);
		c.XMove = width[i];
	}

	LoadTranslations();

	return true;
}

//==========================================================================
//
// FSingleLumpFont :: LoadBMF
//
// Loads a BMF font. The file format is described at
// <http://bmf.wz.cz/bmf-format.htm>
//
//==========================================================================

void FSingleLumpFont::LoadBMF(int lump, const BYTE *data)
{
	const BYTE *chardata;
	int numchars, count, totalwidth, nwidth;
	int infolen;
	int i, chari;
	BYTE raw_palette[256*3];
	PalEntry sort_palette[256];

	FontType = BMFFONT;
	FontHeight = data[5];
	GlobalKerning = (SBYTE)data[8];
	ActiveColors = data[16];
	SpaceWidth = -1;
	nwidth = -1;
	RescalePalette = true;

	infolen = data[17 + ActiveColors*3];
	chardata = data + 18 + ActiveColors*3 + infolen;
	numchars = chardata[0] + 256*chardata[1];
	chardata += 2;

	// Scan for lowest and highest characters defined and total font width.
	FirstChar = 256;
	LastChar = 0;
	totalwidth = 0;
	for (i = chari = 0; i < numchars; ++i, chari += 6 + chardata[chari+1] * chardata[chari+2])
	{
		if ((chardata[chari+1] == 0 || chardata[chari+2] == 0) && chardata[chari+5] == 0)
		{ // Don't count empty characters.
			continue;
		}
		if (chardata[chari] < FirstChar)
		{
			FirstChar = chardata[chari];
		}
		if (chardata[chari] > LastChar)
		{
			LastChar = chardata[chari];
		}
		totalwidth += chardata[chari+1];
	}
	if (LastChar < FirstChar)
	{
		I_FatalError("BMF font defines no characters");
	}
	count = LastChar - FirstChar + 1;
	Chars = new CharData[count];
	for (i = 0; i < count; ++i)
	{
		Chars[i].Pic = NULL;
		Chars[i].XMove = INT_MIN;
	}

	// BMF palettes are only six bits per component. Fix that.
	for (i = 0; i < ActiveColors*3; ++i)
	{
		raw_palette[i+3] = (data[17 + i] << 2) | (data[17 + i] >> 4);
	}
	ActiveColors++;

	// Sort the palette by increasing brightness
	for (i = 0; i < ActiveColors; ++i)
	{
		PalEntry *pal = &sort_palette[i];
		pal->a = i;		// Use alpha part to point back to original entry
		pal->r = raw_palette[i*3 + 0];
		pal->g = raw_palette[i*3 + 1];
		pal->b = raw_palette[i*3 + 2];
	}
	qsort(sort_palette + 1, ActiveColors - 1, sizeof(PalEntry), BMFCompare);

	// Create the PatchRemap table from the sorted "alpha" values.
	PatchRemap = new BYTE[ActiveColors];
	PatchRemap[0] = 0;
	for (i = 1; i < ActiveColors; ++i)
	{
		PatchRemap[sort_palette[i].a] = i;
	}

	memcpy(PaletteData, raw_palette, 768);

	// Now scan through the characters again, creating glyphs for each one.
	for (i = chari = 0; i < numchars; ++i, chari += 6 + chardata[chari+1] * chardata[chari+2])
	{
		assert(chardata[chari] - FirstChar >= 0);
		assert(chardata[chari] - FirstChar < count);
		if (chardata[chari] == ' ')
		{
			SpaceWidth = chardata[chari+5];
		}
		else if (chardata[chari] == 'N')
		{
			nwidth = chardata[chari+5];
		}
		Chars[chardata[chari] - FirstChar].XMove = chardata[chari+5];
		if (chardata[chari+1] == 0 || chardata[chari+2] == 0)
		{ // Empty character: skip it.
			continue;
		}
		Chars[chardata[chari] - FirstChar].Pic = new FFontChar2(lump, int(chardata + chari + 6 - data),
			chardata[chari+1],	// width
			chardata[chari+2],	// height
			-(SBYTE)chardata[chari+3],	// x offset
			-(SBYTE)chardata[chari+4]	// y offset
		);
	}

	// If the font did not define a space character, determine a suitable space width now.
	if (SpaceWidth < 0)
	{
		if (nwidth >= 0)
		{
			SpaceWidth = nwidth;
		}
		else
		{
			SpaceWidth = totalwidth * 2 / (3 * count);
		}
	}

	FixXMoves();
	LoadTranslations();
}

//==========================================================================
//
// FSingleLumpFont :: BMFCompare									STATIC
//
// Helper to sort BMF palettes.
//
//==========================================================================

int STACK_ARGS FSingleLumpFont::BMFCompare(const void *a, const void *b)
{
	const PalEntry *pa = (const PalEntry *)a;
	const PalEntry *pb = (const PalEntry *)b;

	return (pa->r * 299 + pa->g * 587 + pa->b * 114) -
		   (pb->r * 299 + pb->g * 587 + pb->b * 114);
}

//==========================================================================
//
// FSingleLumpFont :: CheckFON1Chars
//
// Scans a FON1 resource for all the color values it uses and sets up
// some tables like SimpleTranslation. Data points to the RLE data for
// the characters. Also sets up the character textures.
//
//==========================================================================

void FSingleLumpFont::CheckFON1Chars (double *luminosity)
{
	FMemLump memLump = Wads.ReadLump(Lump);
	const BYTE* data = (const BYTE*) memLump.GetMem();

	BYTE used[256], reverse[256];
	const BYTE *data_p;
	int i, j;

	memset (used, 0, 256);
	data_p = data + 8;

	for (i = 0; i < 256; ++i)
	{
		int destSize = SpaceWidth * FontHeight;

		if(!Chars[i].Pic)
		{
			Chars[i].Pic = new FFontChar2 (Lump, int(data_p - data), SpaceWidth, FontHeight);
			Chars[i].XMove = SpaceWidth;
		}

		// Advance to next char's data and count the used colors.
		do
		{
			SBYTE code = *data_p++;
			if (code >= 0)
			{
				destSize -= code+1;
				while (code-- >= 0)
				{
					used[*data_p++] = 1;
				}
			}
			else if (code != -128)
			{
				used[*data_p++] = 1;
				destSize -= 1 - code;
			}
		} while (destSize > 0);
	}

	memset (PatchRemap, 0, 256);
	reverse[0] = 0;
	for (i = 1, j = 1; i < 256; ++i)
	{
		if (used[i])
		{
			reverse[j++] = i;
		}
	}
	for (i = 1; i < j; ++i)
	{
		PatchRemap[reverse[i]] = i;
		luminosity[i] = (reverse[i] - 1) / 254.0;
	}
	ActiveColors = j;
}

//==========================================================================
//
// FSingleLumpFont :: FixupPalette
//
// Finds the best matches for the colors used by a FON2 font and sets up
// some tables like SimpleTranslation.
//
//==========================================================================

void FSingleLumpFont::FixupPalette (BYTE *identity, double *luminosity, const BYTE *palette, bool rescale, PalEntry *out_palette)
{
	int i;
	double maxlum = 0.0;
	double minlum = 100000000.0;
	double diver;

	identity[0] = 0;
	palette += 3;	// Skip the transparent color

	for (i = 1; i < ActiveColors; ++i, palette += 3)
	{
		int r = palette[0];
		int g = palette[1];
		int b = palette[2];
		double lum = r*0.299 + g*0.587 + b*0.114;
		identity[i] = ColorMatcher.Pick (r, g, b);
		luminosity[i] = lum;
		out_palette[i].r = r;
		out_palette[i].g = g;
		out_palette[i].b = b;
		out_palette[i].a = 255;
		if (lum > maxlum)
			maxlum = lum;
		if (lum < minlum)
			minlum = lum;
	}
	out_palette[0] = 0;

	if (rescale)
	{
		diver = 1.0 / (maxlum - minlum);
	}
	else
	{
		diver = 1.0 / 255.0;
	}
	for (i = 1; i < ActiveColors; ++i)
	{
		luminosity[i] = (luminosity[i] - minlum) * diver;
	}
}

//==========================================================================
//
// FSinglePicFont :: FSinglePicFont
//
// Creates a font to wrap a texture so that you can use hudmessage as if it
// were a hudpic command. It does not support translation, but animation
// is supported, unlike all the real fonts.
//
//==========================================================================

FSinglePicFont::FSinglePicFont(const char *picname) :
	FFont(-1) // Since lump is only needed for priority information we don't need to worry about this here.
{
	FTextureID picnum = TexMan.CheckForTexture (picname, FTexture::TEX_Any);

	if (!picnum.isValid())
	{
		I_FatalError ("%s is not a font or texture", picname);
	}

	FTexture *pic = TexMan[picnum];

	Name = copystring(picname);
	FontHeight = pic->GetScaledHeight();
	SpaceWidth = pic->GetScaledWidth();
	GlobalKerning = 0;
	FirstChar = LastChar = 'A';
	ActiveColors = 0;
	PicNum = picnum;

	Next = FirstFont;
	FirstFont = this;
}

//==========================================================================
//
// FSinglePicFont :: GetChar
//
// Returns the texture if code is 'a' or 'A', otherwise NULL.
//
//==========================================================================

FTexture *FSinglePicFont::GetChar (int code, int *const width) const
{
	*width = SpaceWidth;
	if (code == 'a' || code == 'A')
	{
		return TexMan(PicNum);
	}
	else
	{
		return NULL;
	}
}

//==========================================================================
//
// FSinglePicFont :: GetCharWidth
//
// Don't expect the text functions to work properly if I actually allowed
// the character width to vary depending on the animation frame.
//
//==========================================================================

int FSinglePicFont::GetCharWidth (int code) const
{
	return SpaceWidth;
}

//==========================================================================
//
// FFontChar1 :: FFontChar1
//
// Used by fonts made from textures.
//
//==========================================================================

FFontChar1::FFontChar1 (FTexture *sourcelump)
: SourceRemap (NULL)
{
	UseType = FTexture::TEX_FontChar;
	BaseTexture = sourcelump;

	// now copy all the properties from the base texture
	assert(BaseTexture != NULL);
	CopySize(BaseTexture);
	Pixels = NULL;
}

//==========================================================================
//
// FFontChar1 :: GetPixels
//
//==========================================================================

const BYTE *FFontChar1::GetPixels ()
{
	if (Pixels == NULL)
	{
		MakeTexture ();
	}
	return Pixels;
}

//==========================================================================
//
// FFontChar1 :: MakeTexture
//
//==========================================================================

void FFontChar1::MakeTexture ()
{
	// Make the texture as normal, then remap it so that all the colors
	// are at the low end of the palette
	Pixels = new BYTE[Width*Height];
	const BYTE *pix = BaseTexture->GetPixels();

	if (!SourceRemap)
	{
		memcpy(Pixels, pix, Width*Height);
	}
	else
	{
		for (int x = 0; x < Width*Height; ++x)
		{
			Pixels[x] = SourceRemap[pix[x]];
		}
	}
}

//==========================================================================
//
// FFontChar1 :: GetColumn
//
//==========================================================================

const BYTE *FFontChar1::GetColumn (unsigned int column, const Span **spans_out)
{
	if (Pixels == NULL)
	{
		MakeTexture ();
	}

	BaseTexture->GetColumn(column, spans_out);
	return Pixels + column*Height;
}

//==========================================================================
//
// FFontChar1 :: SetSourceRemap
//
//==========================================================================

void FFontChar1::SetSourceRemap(const BYTE *sourceremap)
{
	Unload();
	SourceRemap = sourceremap;
}

//==========================================================================
//
// FFontChar1 :: Unload
//
//==========================================================================

void FFontChar1::Unload ()
{
	if (Pixels != NULL)
	{
		delete[] Pixels;
		Pixels = NULL;
	}
}

//==========================================================================
//
// FFontChar1 :: ~FFontChar1
//
//==========================================================================

FFontChar1::~FFontChar1 ()
{
	Unload ();
}

//==========================================================================
//
// FFontChar2 :: FFontChar2
//
// Used by FON1 and FON2 fonts.
//
//==========================================================================

FFontChar2::FFontChar2 (int sourcelump, int sourcepos, int width, int height, int leftofs, int topofs)
: SourceLump (sourcelump), SourcePos (sourcepos), Pixels (0), Spans (0), SourceRemap(NULL)
{
	UseType = TEX_FontChar;
	Width = width;
	Height = height;
	LeftOffset = leftofs;
	TopOffset = topofs;
	CalcBitSize ();
}

//==========================================================================
//
// FFontChar2 :: ~FFontChar2
//
//==========================================================================

FFontChar2::~FFontChar2 ()
{
	Unload ();
	if (Spans != NULL)
	{
		FreeSpans (Spans);
		Spans = NULL;
	}
}

//==========================================================================
//
// FFontChar2 :: Unload
//
//==========================================================================

void FFontChar2::Unload ()
{
	if (Pixels != NULL)
	{
		delete[] Pixels;
		Pixels = NULL;
	}
}

//==========================================================================
//
// FFontChar2 :: GetPixels
//
//==========================================================================

const BYTE *FFontChar2::GetPixels ()
{
	if (Pixels == NULL)
	{
		MakeTexture ();
	}
	return Pixels;
}

//==========================================================================
//
// FFontChar2 :: GetColumn
//
//==========================================================================

const BYTE *FFontChar2::GetColumn (unsigned int column, const Span **spans_out)
{
	if (Pixels == NULL)
	{
		MakeTexture ();
	}
	if (column >= Width)
	{
		column = WidthMask;
	}
	if (spans_out != NULL)
	{
		if (Spans == NULL)
		{
			Spans = CreateSpans (Pixels);
		}
		*spans_out = Spans[column];
	}
	return Pixels + column*Height;
}

//==========================================================================
//
// FFontChar2 :: SetSourceRemap
//
//==========================================================================

void FFontChar2::SetSourceRemap(const BYTE *sourceremap)
{
	Unload();
	if(Spans) // ECWolf: Free spans since ROTT fonts have an unusual transparent color
	{
		FreeSpans(Spans);
		Spans = NULL;
	}
	SourceRemap = sourceremap;
}

//==========================================================================
//
// FFontChar2 :: MakeTexture
//
//==========================================================================

void FFontChar2::MakeTexture ()
{
	FWadLump lump = Wads.OpenLumpNum (SourceLump);
	int destSize = Width * Height;
	BYTE max = 255;
	bool rle = true;

	// This is to "fix" bad fonts
	{
		BYTE buff[16];
		lump.Read (buff, 4);
		if (buff[3] == '2')
		{
			lump.Read (buff, 7);
			max = buff[6];
			lump.Seek (SourcePos - 11, SEEK_CUR);
		}
		else if (buff[3] == 0x1A)
		{
			lump.Read(buff, 13);
			max = buff[12] - 1;
			lump.Seek (SourcePos - 17, SEEK_CUR);
			rle = false;
		}
		else if (buff[3] == '1')
		{
			lump.Seek (SourcePos - 4, SEEK_CUR);
		}
		else
		{
			rle = false;
			lump.Seek (SourcePos - 4, SEEK_CUR);
		}
	}

	Pixels = new BYTE[destSize];

	int runlen = 0, setlen = 0;
	BYTE setval = 0;  // Shut up, GCC!
	BYTE *dest_p = Pixels;
	int dest_adv = Height;
	int dest_rew = destSize - 1;

	if (rle)
	{
		for (int y = Height; y != 0; --y)
		{
			for (int x = Width; x != 0; )
			{
				if (runlen != 0)
				{
					BYTE color;

					lump >> color;
					color = MIN (color, max);
					if (SourceRemap != NULL)
					{
						color = SourceRemap[color];
					}
					*dest_p = color;
					dest_p += dest_adv;
					x--;
					runlen--;
				}
				else if (setlen != 0)
				{
					*dest_p = setval;
					dest_p += dest_adv;
					x--;
					setlen--;
				}
				else
				{
					SBYTE code;

					lump >> code;
					if (code >= 0)
					{
						runlen = code + 1;
					}
					else if (code != -128)
					{
						BYTE color;

						lump >> color;
						setlen = (-code) + 1;
						setval = MIN (color, max);
						if (SourceRemap != NULL)
						{
							setval = SourceRemap[setval];
						}
					}
				}
			}
			dest_p -= dest_rew;
		}
	}
	else
	{
		for (int y = Height; y != 0; --y)
		{
			for (int x = Width; x != 0; --x)
			{
				BYTE color;
				lump >> color;
				if (color > max)
				{
					color = max;
				}
				if (SourceRemap != NULL)
				{
					color = SourceRemap[color];
				}
				*dest_p = color;
				dest_p += dest_adv;
			}
			dest_p -= dest_rew;
		}
	}

	if (destSize < 0)
	{
		char name[9];
		Wads.GetLumpName (name, SourceLump);
		name[8] = 0;
		I_FatalError ("The font %s is corrupt", name);
 	}
}

void FTile8Char::MakeTexture()
{
	FWadLump lump = Wads.OpenLumpNum (SourceLump);
	int destSize = Width * Height;

	lump.Seek (SourcePos, SEEK_SET);

	Pixels = new BYTE[destSize];

	BYTE *dest_p = Pixels;
	int dest_adv = Height*4;
	int dest_rew = destSize - 1;

	for (int p = 16;p != 0;--p)
	{
		for (int y = Height/4; y != 0; --y)
		{
			for (int x = Width/4; x != 0; --x)
			{
				BYTE color;
				lump >> color;
				if (SourceRemap != NULL)
				{
					color = SourceRemap[color];
				}
				*dest_p = color != MaskColor ? color : 0;
				dest_p += dest_adv;
			}
			dest_p -= dest_rew;
		}
	}
}

void FROTTChar::MakeTexture()
{
	FWadLump lump = Wads.OpenLumpNum (SourceLump);
	const int destSize = Width * Height;

	lump.Seek (SourcePos, SEEK_SET);

	Pixels = new BYTE[destSize];
	lump.Read(Pixels, destSize);

	// A ROTT font character is row major with 0xFE as the mask color. Coloring
	// is number of palette indexes off from a base color.
	// Due to how we assume mask is 0, we'll load with a +1 offset and we can't
	// do much about index 0xFF.
	int i = destSize;
	while (i-- != 0)
	{
		BYTE color = Pixels[i] + 1;
		if(color == 0xFF)
			color = 0;
		if(SourceRemap)
			color = SourceRemap[color];
		Pixels[i] = color;
	}
}

//==========================================================================
//
// FSpecialFont :: FSpecialFont
//
//==========================================================================

FSpecialFont::FSpecialFont (const char *name, int first, int count, FTextureID *lumplist, const bool *notranslate, int lump) : FFont(lump)
{
	int i;
	FTexture **charlumps;
	int maxyoffs;
	FTexture *pic;

	memcpy(this->notranslate, notranslate, 256*sizeof(bool));

	Name = copystring(name);
	Chars = new CharData[count];
	charlumps = new FTexture*[count];
	PatchRemap = new BYTE[256];
	FirstChar = first;
	LastChar = first + count - 1;
	FontHeight = 0;
	GlobalKerning = false;
	Next = FirstFont;
	FirstFont = this;

	maxyoffs = 0;

	for (i = 0; i < count; i++)
	{
		if(lumplist[i].Exists())
			pic = charlumps[i] = TexMan[lumplist[i]];
		else
			pic = charlumps[i] = NULL;

		if (pic != NULL)
		{
			int height = pic->GetScaledHeight();
			int yoffs = pic->GetScaledTopOffset();

			if (yoffs > maxyoffs)
			{
				maxyoffs = yoffs;
			}
			height += abs (yoffs);
			if (height > FontHeight)
			{
				FontHeight = height;
			}
		}

		if (charlumps[i] != NULL)
		{
			Chars[i].Pic = new FFontChar1 (charlumps[i]);
			Chars[i].ID = lumplist[i];
			Chars[i].XMove = Chars[i].Pic->GetScaledWidth();
		}
		else
		{
			Chars[i].Pic = NULL;
			Chars[i].ID.SetInvalid();
			Chars[i].XMove = INT_MIN;
		}
	}

	// Special fonts normally don't have all characters so be careful here!
	if ('N'-first >= 0 && 'N'-first < count && Chars['N' - first].Pic != NULL)
	{
		SpaceWidth = (Chars['N' - first].XMove + 1) / 2;
	}
	else
	{
		SpaceWidth = 4;
	}

	FixXMoves();

	LoadTranslations();

	delete[] charlumps;
}

//==========================================================================
//
// FSpecialFont :: LoadTranslations
//
//==========================================================================

void FSpecialFont::LoadTranslations()
{
	int count = LastChar - FirstChar + 1;
	BYTE usedcolors[256], identity[256];
	double *luminosity;
	int TotalColors;
	int i, j;

	memset (usedcolors, 0, 256);
	for (i = 0; i < count; i++)
	{
		FFontChar1 *pic = static_cast<FFontChar1 *>(Chars[i].Pic);
		if(pic)
		{
			pic->SetSourceRemap(NULL); // Force the FFontChar1 to return the same pixels as the base texture
			RecordTextureColors (pic, usedcolors);
		}
	}

	// exclude the non-translated colors from the translation calculation
	for (i = 0; i < 256; i++)
		if (notranslate[i])
			usedcolors[i] = false;

	TotalColors = ActiveColors = SimpleTranslation (usedcolors, PatchRemap, identity, &luminosity);

	// Map all untranslated colors into the table of used colors
	for (i = 0; i < 256; i++) 
	{
		if (notranslate[i]) 
		{
			PatchRemap[i] = TotalColors;
			identity[TotalColors] = i;
			TotalColors++;
		}
	}

	for (i = 0; i < count; i++)
	{
		if(Chars[i].Pic)
			static_cast<FFontChar1 *>(Chars[i].Pic)->SetSourceRemap(PatchRemap);
	}

	BuildTranslations (luminosity, identity, &TranslationParms[0][0], TotalColors, NULL);

	// add the untranslated colors to the Ranges tables
	if (ActiveColors < TotalColors)
	{
		for (i = 0; i < NumTextColors; i++)
		{
			FRemapTable *remap = &Ranges[i];
			for (j = ActiveColors; j < TotalColors; ++j)
			{
				remap->Remap[j] = identity[j];
				remap->Palette[j] = GPalette.BaseColors[identity[j]];
				remap->Palette[j].a = 0xff;
			}
		}
	}
	ActiveColors = TotalColors;

	delete[] luminosity;
}

//==========================================================================
//
// FFont :: FixXMoves
//
// If a font has gaps in its characters, set the missing characters'
// XMoves to either SpaceWidth or the unaccented or uppercase variant's
// XMove. Missing XMoves must be initialized with INT_MIN beforehand.
//
//==========================================================================

void FFont::FixXMoves()
{
	for (int i = 0; i <= LastChar - FirstChar; ++i)
	{
		if (Chars[i].XMove == INT_MIN)
		{
			// Try an uppercase character.
			if (myislower(i + FirstChar))
			{
				int upper = i - 32;
				if (upper >= 0)
				{
					Chars[i].XMove = Chars[upper].XMove;
					continue;
				}
			}
			// Try an unnaccented character.
			int noaccent = stripaccent(i + FirstChar);
			if (noaccent != i + FirstChar)
			{
				noaccent -= FirstChar;
				if (noaccent >= 0)
				{
					Chars[i].XMove = Chars[noaccent].XMove;
					continue;
				}
			}
			Chars[i].XMove = SpaceWidth;
		}
	}
}


//==========================================================================
//
// V_InitCustomFonts
//
// Initialize a list of custom multipatch fonts
//
//==========================================================================

#define WRONG sc.ScriptMessage (Scanner::ERROR, "Invalid combination of properties in font '%s'", namebuffer.GetChars())
void V_InitCustomFonts()
{
	FTextureID lumplist[256];
	bool notranslate[256];
	FString namebuffer, templatebuf;
	int i;
	int llump,lastlump=0;
	int format;
	int start;
	int first;
	int count;
	int spacewidth;
	char cursor = '_';

	while ((llump = Wads.FindLump ("FONTDEFS", &lastlump)) != -1)
	{
		Scanner sc(llump);
		while (sc.GetNextString())
		{
			memset (lumplist, 0, sizeof(lumplist));
			memset (notranslate, 0, sizeof(notranslate));
			namebuffer = sc->str;
			format = 0;
			start = 33;
			first = 33;
			count = 223;
			spacewidth = -1;

			sc.MustGetToken ('{');
			while (!sc.CheckToken ('}'))
			{
				if(!sc.GetNextString()) sc.ScriptMessage(Scanner::ERROR, "Expected string.");;
				if (0 == sc->str.CompareNoCase ("TEMPLATE"))
				{
					if (format == 2) WRONG;
					if(!sc.GetNextString()) sc.ScriptMessage(Scanner::ERROR, "Expected string.");;
					templatebuf = sc->str;
					format = 1;
				}
				else if (0 == sc->str.CompareNoCase ("BASE"))
				{
					if (format == 2) WRONG;
					sc.MustGetToken(TK_IntConst);
					start = sc->number;
					format = 1;
				}
				else if (0 == sc->str.CompareNoCase ("FIRST"))
				{
					if (format == 2) WRONG;
					sc.MustGetToken(TK_IntConst);
					first = sc->number;
					format = 1;
				}
				else if (0 == sc->str.CompareNoCase ("COUNT"))
				{
					if (format == 2) WRONG;
					sc.MustGetToken(TK_IntConst);
					count = sc->number;
					format = 1;
				}
				else if (0 == sc->str.CompareNoCase ("CURSOR"))
				{
					if(!sc.GetNextString()) sc.ScriptMessage(Scanner::ERROR, "Expected string.");;
					cursor = sc->str[0];
				}
				else if (0 == sc->str.CompareNoCase ("SPACEWIDTH"))
				{
					if (format == 2) WRONG;
					sc.MustGetToken(TK_IntConst);
					spacewidth = sc->number;
					format = 1;
				}
				else if (0 == sc->str.CompareNoCase ("NOTRANSLATION"))
				{
					if (format == 1) WRONG;
					while (sc.CheckToken(TK_IntConst))// && !sc.Crossed)
					{
						if (sc->number < 256)
							notranslate[sc->number] = true;
					}
					format = 2;
				}
				else
				{
					if (format == 1) WRONG;
					FTextureID &p = lumplist[*(unsigned char*)sc->str.GetChars()];
					if(!sc.GetNextString()) sc.ScriptMessage(Scanner::ERROR, "Expected string.");
					FTextureID texid = TexMan.CheckForTexture(sc->str, FTexture::TEX_MiscPatch);
					if (texid.Exists())
					{
						p = texid;
					}
					if (!texid.Exists() && Wads.GetLumpFile(llump) >= Wads.IWAD_FILENUM)
					{
						// Print a message only if this isn't in zdoom.pk3
						sc.ScriptMessage(Scanner::WARNING, "%s: Unable to find texture in font definition for %s", sc->str.GetChars(), namebuffer.GetChars());
					}
					format = 2;
				}
			}
			if (format == 1)
			{
				FFont *fnt = new FFont (namebuffer, templatebuf, first, count, start, llump, spacewidth);
				fnt->SetCursor(cursor);
			}
			else if (format == 2)
			{
				for (i = 0; i < 256; i++)
				{
					if (lumplist[i].isValid())
					{
						first = i;
						break;
					}
				}
				for (i = 255; i >= 0; i--)
				{
					if (lumplist[i].isValid())
					{
						count = i - first + 1;
						break;
					}
				}
				if (count > 0)
				{
					FFont *fnt = new FSpecialFont (namebuffer, first, count, &lumplist[first], notranslate, llump);
					fnt->SetCursor(cursor);
				}
			}
			else WRONG;
		}
	}
}

//==========================================================================
//
// V_InitFontColors
//
// Reads the list of color translation definitions into memory.
//
//==========================================================================

void V_InitFontColors ()
{
	TArray<FName> names;
	int lump, lastlump = 0;
	TranslationParm tparm = { 0, 0, {0}, {0} };	// Silence GCC (for real with -Wextra )
	TArray<TranslationParm> parms;
	TArray<TempParmInfo> parminfo;
	TArray<TempColorInfo> colorinfo;
	int c, parmchoice;
	TempParmInfo info;
	TempColorInfo cinfo;
	PalEntry logcolor;
	unsigned int i, j;
	int k, index;

	info.Index = -1;

	TranslationParms[0].Clear();
	TranslationParms[1].Clear();
	TranslationParms[2].Clear();
	TranslationLookup.Clear();
	TranslationColors.Clear();

	while ((lump = Wads.FindLump ("TEXTCOLO", &lastlump)) != -1)
	{
#if 0
		if (gameinfo.flags & GI_NOTEXTCOLOR)
		{
			// Chex3 contains a bad TEXTCOLO lump, probably to force all text to be green.
			// This renders the Gray, Gold, Red and Yellow color range inoperable, some of
			// which are used by the menu. So we have no choice but to skip this lump so that
			// all colors work properly.
			// The text colors should be the end user's choice anyway.
			if (Wads.GetLumpFile(lump) == 1) continue;
		}
#endif
		Scanner sc(lump);

		while (sc.GetNextString())
		{
			names.Clear();

			logcolor = DEFAULT_LOG_COLOR;

			// Everything until the '{' is considered a valid name for the
			// color range.
			names.Push (sc->str);
			if(!sc.GetNextString()) sc.ScriptMessage(Scanner::ERROR, "Expected string.");
			while (0 != sc->str.CompareNoCase ("{"))
			{
				if (names[0] == NAME_Untranslated)
				{
					sc.ScriptMessage (Scanner::ERROR, "The \"untranslated\" color may not have any other names");
				}
				names.Push (sc->str);
				if(!sc.GetNextString()) sc.ScriptMessage(Scanner::ERROR, "Expected string.");
			}

			parmchoice = 0;
			info.StartParm[0] = parms.Size();
			info.StartParm[2] = info.StartParm[1] = 0;
			info.ParmLen[2] = info.ParmLen[1] = info.ParmLen[0] = 0;
			tparm.RangeEnd = tparm.RangeStart = -1;

			if(!sc.GetNextString()) sc.ScriptMessage(Scanner::ERROR, "Expected string.");
			while (0 != sc->str.CompareNoCase ("}"))
			{
				if (0 == sc->str.CompareNoCase ("Console:"))
				{
					if (info.ParmLen[1] != 0)
					{
						sc.ScriptMessage (Scanner::ERROR, "Each color may only have one set of console ranges");
					}
					info.StartParm[1] = parms.Size();
					info.ParmLen[parmchoice] = info.StartParm[1] - info.StartParm[parmchoice];
					parmchoice = 1;
					tparm.RangeEnd = tparm.RangeStart = -1;
				}
				else if (0 == sc->str.CompareNoCase ("Flat:"))
				{
					if (info.ParmLen[2] != 0)
					{
						sc.ScriptMessage (Scanner::ERROR, "Each color may only have one set of console ranges");
					}
					if(!sc.GetNextString()) sc.ScriptMessage(Scanner::ERROR, "Expected string.");;
					logcolor = V_GetColor (NULL, sc->str);

					// [BL] We want to be able to use Flat coloring in ECWolf
					info.StartParm[2] = parms.Size();
					info.ParmLen[parmchoice] = info.StartParm[2] - info.StartParm[parmchoice];
					parmchoice = 2;
					tparm.RangeEnd = 256;
					tparm.RangeStart = 0;
					tparm.Start[0] = tparm.End[0] = RPART(logcolor);
					tparm.Start[1] = tparm.End[1] = GPART(logcolor);
					tparm.Start[2] = tparm.End[2] = BPART(logcolor);
					parms.Push (tparm);
				}
				else
				{
					// Get first color
					c = V_GetColor (NULL, sc->str);
					tparm.Start[0] = RPART(c);
					tparm.Start[1] = GPART(c);
					tparm.Start[2] = BPART(c);

					// Get second color
					if(!sc.GetNextString()) sc.ScriptMessage(Scanner::ERROR, "Expected string.");;
					c = V_GetColor (NULL, sc->str);
					tparm.End[0] = RPART(c);
					tparm.End[1] = GPART(c);
					tparm.End[2] = BPART(c);

					// Check for range specifier
					if (sc.CheckToken(TK_IntConst))
					{
						if (tparm.RangeStart == -1 && sc->number != 0)
						{
							sc.ScriptMessage (Scanner::ERROR, "The first color range must start at position 0");
						}
						if (sc->number > 256)
						{
							sc.ScriptMessage (Scanner::ERROR, "The color range must be within positions [0,256]");
						}
						if ((signed)sc->number <= tparm.RangeEnd)
						{
							sc.ScriptMessage (Scanner::ERROR, "The color range must not start before the previous one ends");
						}
						tparm.RangeStart = sc->number;

						sc.MustGetToken(TK_IntConst);
						if (sc->number > 256)
						{
							sc.ScriptMessage (Scanner::ERROR, "The color range must be within positions [0,256]");
						}
						if ((signed)sc->number <= tparm.RangeStart)
						{
							sc.ScriptMessage (Scanner::ERROR, "The color range end position must be larger than the start position");
						}
						tparm.RangeEnd = sc->number;
					}
					else
					{
						tparm.RangeStart = tparm.RangeEnd + 1;
						tparm.RangeEnd = 256;
						if (tparm.RangeStart >= tparm.RangeEnd)
						{
							sc.ScriptMessage (Scanner::ERROR, "The color has too many ranges");
						}
					}
					parms.Push (tparm);
				}
				if(!sc.GetNextString()) sc.ScriptMessage(Scanner::ERROR, "Expected string.");
			}
			info.ParmLen[parmchoice] = parms.Size() - info.StartParm[parmchoice];
			if (info.ParmLen[0] == 0)
			{
				if (names[0] != NAME_Untranslated)
				{
					sc.ScriptMessage (Scanner::ERROR, "There must be at least one normal range for a color");
				}
			}
			else
			{
				if (names[0] == NAME_Untranslated)
				{
					sc.ScriptMessage (Scanner::ERROR, "The \"untranslated\" color must be left undefined");
				}
			}
			if (info.ParmLen[1] == 0 && names[0] != NAME_Untranslated)
			{ // If a console translation is unspecified, make it white, since the console
			  // font has no color information stored with it.
				tparm.RangeStart = 0;
				tparm.RangeEnd = 256;
				tparm.Start[2] = tparm.Start[1] = tparm.Start[0] = 0;
				tparm.End[2] = tparm.End[1] = tparm.End[0] = 255;
				info.StartParm[1] = parms.Push (tparm);
				info.ParmLen[1] = 1;
			}
			if (info.ParmLen[2] == 0 && names[0] != NAME_Untranslated)
			{ // [BL] Likewise do the same for the lack of a flat color
				tparm.RangeStart = 0;
				tparm.RangeEnd = 256;
				tparm.Start[2] = tparm.Start[1] = tparm.Start[0] = 255;
				tparm.End[2] = tparm.End[1] = tparm.End[0] = 255;
				info.StartParm[2] = parms.Push (tparm);
				info.ParmLen[2] = 1;
			}
			cinfo.ParmInfo = parminfo.Push (info);
			// Record this color information for each name it goes by
			for (i = 0; i < names.Size(); ++i)
			{
				// Redefine duplicates in-place
				for (j = 0; j < colorinfo.Size(); ++j)
				{
					if (colorinfo[j].Name == names[i])
					{
						colorinfo[j].ParmInfo = cinfo.ParmInfo;
						colorinfo[j].LogColor = logcolor;
						break;
					}
				}
				if (j == colorinfo.Size())
				{
					cinfo.Name = names[i];
					cinfo.LogColor = logcolor;
					colorinfo.Push (cinfo);
				}
			}
		}
	}
	// Make permananent copies of all the color information we found.
	for (i = 0, index = 0; i < colorinfo.Size(); ++i)
	{
		TranslationMap tmap;
		TempParmInfo *pinfo;

		tmap.Name = colorinfo[i].Name;
		pinfo = &parminfo[colorinfo[i].ParmInfo];
		if (pinfo->Index < 0)
		{
			// Write out the set of remappings for this color.
			for (k = 0; k < 3; ++k)
			{
				for (j = 0; j < pinfo->ParmLen[k]; ++j)
				{
					TranslationParms[k].Push (parms[pinfo->StartParm[k] + j]);
				}
			}
			TranslationColors.Push (colorinfo[i].LogColor);
			pinfo->Index = index++;
		}
		tmap.Number = pinfo->Index;
		TranslationLookup.Push (tmap);
	}
	// Leave a terminating marker at the ends of the lists.
	tparm.RangeStart = -1;
	TranslationParms[0].Push (tparm);
	TranslationParms[1].Push (tparm);
	TranslationParms[2].Push (tparm);
	// Sort the translation lookups for fast binary searching.
	qsort (&TranslationLookup[0], TranslationLookup.Size(), sizeof(TranslationLookup[0]), TranslationMapCompare);

	NumTextColors = index;
	assert (NumTextColors >= NUM_TEXT_COLORS);
}

//==========================================================================
//
// TranslationMapCompare
//
//==========================================================================

static int STACK_ARGS TranslationMapCompare (const void *a, const void *b)
{
	return int(((const TranslationMap *)a)->Name) - int(((const TranslationMap *)b)->Name);
}

//==========================================================================
//
// V_FindFontColor
//
// Returns the color number for a particular named color range.
//
//==========================================================================

EColorRange V_FindFontColor (FName name)
{
	int min = 0, max = TranslationLookup.Size() - 1;

	while (min <= max)
	{
		unsigned int mid = (min + max) / 2;
		const TranslationMap *probe = &TranslationLookup[mid];
		if (probe->Name == name)
		{
			return EColorRange(probe->Number);
		}
		else if (probe->Name < name)
		{
			min = mid + 1;
		}
		else
		{
			max = mid - 1;
		}
	}
	return CR_UNTRANSLATED;
}

//==========================================================================
//
// V_LogColorFromColorRange
//
// Returns the color to use for text in the startup/error log window.
//
//==========================================================================

PalEntry V_LogColorFromColorRange (EColorRange range)
{
	if ((unsigned int)range >= TranslationColors.Size())
	{ // Return default color
		return DEFAULT_LOG_COLOR;
	}
	return TranslationColors[range];
}

//==========================================================================
//
// V_ParseFontColor
//
// Given a pointer to a color identifier (presumably just after a color
// escape character), return the color it identifies and advances
// color_value to just past it.
//
//==========================================================================

EColorRange V_ParseFontColor (const BYTE *&color_value, int normalcolor, int boldcolor)
{
	const BYTE *ch = color_value;
	int newcolor = *ch++;

	if (newcolor == '-')			// Normal
	{
		newcolor = normalcolor;
	}
	else if (newcolor == '+')		// Bold
	{
		newcolor = boldcolor;
	}
	/*else if (newcolor == '!')		// Team chat
	{
		newcolor = PrintColors[PRINT_TEAMCHAT];
	}
	else if (newcolor == '*')		// Chat
	{
		newcolor = PrintColors[PRINT_CHAT];
	}*/
	else if (newcolor == '[')		// Named
	{
		const BYTE *namestart = ch;
		while (*ch != ']' && *ch != '\0')
		{
			ch++;
		}
		FName rangename((const char *)namestart, int(ch - namestart), true);
		if (*ch != '\0')
		{
			ch++;
		}
		newcolor = V_FindFontColor (rangename);
	}
	else if (newcolor >= 'A' && newcolor < NUM_TEXT_COLORS + 'A')	// Standard, uppercase
	{
		newcolor -= 'A';
	}
	else if (newcolor >= 'a' && newcolor < NUM_TEXT_COLORS + 'a')	// Standard, lowercase
	{
		newcolor -= 'a';
	}
	else							// Incomplete!
	{
		color_value = ch - (*ch == '\0');
		return CR_UNDEFINED;
	}
	color_value = ch;
	return EColorRange(newcolor);
}

//==========================================================================
//
// V_InitFonts
//
//==========================================================================

static FFont *FindNewestFont(const char* fullname, const char* lumpname)
{
	// Load either the FONTDEFS font or the single lump font based on whichever
	// has a higher lump number.

	FFont *font = FFont::FindFont(fullname);
	int slLumpNum = Wads.CheckNumForName(lumpname);
	if(slLumpNum >= 0 && (!font || slLumpNum > font->GetLump()))
		font = new FSingleLumpFont(fullname, slLumpNum);
	return font;
}

void V_InitFonts()
{
	V_InitCustomFonts ();

	// load the heads-up font
	if(!(SmallFont = FindNewestFont("SmallFont", "SMALLFNT")))
		SmallFont = FindNewestFont("SmallFont", "SIFONT");

	if(!(SmallFont2 = FindNewestFont("SmallFont2", "SMALLFN2")))
		SmallFont2 = SmallFont;

	if(!(BigFont = FindNewestFont("BigFont", "BIGFONT")))
		BigFont = FindNewestFont("BigFont", "LIFONT");

	ConFont = FindNewestFont("ConsoleFont", "CONFONT");

	if(!(IntermissionFont = FindNewestFont("IntermissionFont", "INTERFNT")))
		IntermissionFont = BigFont;

	if(!(Tile8Font = FindNewestFont("Tile8", "TILE8")))
		Tile8Font = FindNewestFont("Tile8", "IFNT");

	assert(SmallFont && SmallFont2 && BigFont && ConFont && IntermissionFont);
}

void V_ClearFonts()
{
	while (FFont::FirstFont != NULL)
	{
		delete FFont::FirstFont;
	}
	FFont::FirstFont = NULL;
	SmallFont = SmallFont2 = BigFont = ConFont = IntermissionFont = NULL;
}

void V_RetranslateFonts()
{
	FFont *font = FFont::FirstFont;
	while(font)
	{
		font->LoadTranslations();
		font = font->Next;
	}
}
