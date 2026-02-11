/*
** r_translate.cpp
** Translatioo table handling
**
**---------------------------------------------------------------------------
** Copyright 1998-2006 Randy Heit
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

#include <stddef.h>

#include "templates.h"
//#include "r_draw.h"
//#include "r_main.h"
#include "r_translate.h"
//#include "v_video.h"
//#include "g_game.h"
#include "colormatcher.h"
//#include "d_netinf.h"
#include "v_palette.h"
#include "scanner.h"
//#include "sc_man.h"
#include "doomerrors.h"
//#include "i_system.h"
#include "w_wad.h"
#include "farchive.h"

//#include "gi.h"
//#include "stats.h"
#include "zdoomsupport.h"

TAutoGrowArray<FRemapTablePtr, FRemapTable *> translationtables[NUM_TRANSLATION_TABLES];

BYTE identitymap[256];

const BYTE IcePalette[16][3] =
{
	{  10,  8, 18 },
	{  15, 15, 26 },
	{  20, 16, 36 },
	{  30, 26, 46 },
	{  40, 36, 57 },
	{  50, 46, 67 },
	{  59, 57, 78 },
	{  69, 67, 88 },
	{  79, 77, 99 },
	{  89, 87,109 },
	{  99, 97,120 },
	{ 109,107,130 },
	{ 118,118,141 },
	{ 128,128,151 },
	{ 138,138,162 },
	{ 148,148,172 }
};

/****************************************************/
/****************************************************/

FRemapTable::FRemapTable(int count)
{
	assert(count <= 256);
	Inactive = false;
	Alloc(count);

	// Note that the tables are left uninitialized. It is assumed that
	// the caller will do that next, if only by calling MakeIdentity().
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

FRemapTable::~FRemapTable()
{
	Free();
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

void FRemapTable::Alloc(int count)
{
	Remap = (BYTE *)M_Malloc(count*sizeof(*Remap) + count*sizeof(*Palette));
	assert (Remap != NULL);
	Palette = (PalEntry *)(Remap + count*(sizeof(*Remap)));
	NumEntries = count;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

void FRemapTable::Free()
{
	if (Remap != NULL)
	{
		M_Free(Remap);
		Remap = NULL;
		Palette = NULL;
		NumEntries = 0;
	}
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

FRemapTable::FRemapTable(const FRemapTable &o)
{
	Remap = NULL;
	NumEntries = 0;
	operator= (o);
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

FRemapTable &FRemapTable::operator=(const FRemapTable &o)
{
	if (&o == this)
	{
		return *this;
	}
	if (o.NumEntries != NumEntries)
	{
		Free();
	}
	if (Remap == NULL)
	{
		Alloc(o.NumEntries);
	}
	Inactive = o.Inactive;
	memcpy(Remap, o.Remap, NumEntries*sizeof(*Remap) + NumEntries*sizeof(*Palette));
	return *this;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

bool FRemapTable::operator==(const FRemapTable &o)
{
	// Two translations are identical when they have the same amount of colors
	// and the palette values for both are identical.
	if (&o == this) return true;
	if (o.NumEntries != NumEntries) return false;
	return !memcmp(o.Palette, Palette, NumEntries * sizeof(*Palette));
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

void FRemapTable::Serialize(FArchive &arc)
{
	int n = NumEntries;

	arc << NumEntries;
	if (arc.IsStoring())
	{
		arc.Write (Remap, NumEntries);
	}
	else
	{
		if (n != NumEntries)
		{
			Free();
			Alloc(NumEntries);
		}
		arc.Read (Remap, NumEntries);
	}
	for (int j = 0; j < NumEntries; ++j)
	{
		arc << Palette[j];
	}
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

void FRemapTable::MakeIdentity()
{
	int i;

	for (i = 0; i < NumEntries; ++i)
	{
		Remap[i] = i;
	}
	for (i = 0; i < NumEntries; ++i)
	{
		Palette[i] = GPalette.BaseColors[i];
	}
	for (i = 1; i < NumEntries; ++i)
	{
		Palette[i].a = 255;
	}
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

bool FRemapTable::IsIdentity() const
{
	for (int j = 0; j < 256; ++j)
	{
		if (Remap[j] != j)
		{
			return false;
		}
	}
	return true;
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

void FRemapTable::AddIndexRange(int start, int end, int pal1, int pal2)
{
	fixed_t palcol, palstep;

	if (start > end)
	{
		swapvalues (start, end);
		swapvalues (pal1, pal2);
	}
	else if (start == end)
	{
		start = GPalette.Remap[start];
		pal1 = GPalette.Remap[pal1];
		Remap[start] = pal1;
		Palette[start] = GPalette.BaseColors[pal1];
		Palette[start].a = start == 0 ? 0 : 255;
		return;
	}
	palcol = pal1 << FRACBITS;
	palstep = ((pal2 << FRACBITS) - palcol) / (end - start);
	for (int i = start; i <= end; palcol += palstep, ++i)
	{
		int j = GPalette.Remap[i], k = GPalette.Remap[palcol >> FRACBITS];
		Remap[j] = k;
		Palette[j] = GPalette.BaseColors[k];
		Palette[j].a = j == 0 ? 0 : 255;
	}
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

void FRemapTable::AddColorRange(int start, int end, int _r1,int _g1, int _b1, int _r2, int _g2, int _b2)
{
	fixed_t r1 = _r1 << FRACBITS;
	fixed_t g1 = _g1 << FRACBITS;
	fixed_t b1 = _b1 << FRACBITS;
	fixed_t r2 = _r2 << FRACBITS;
	fixed_t g2 = _g2 << FRACBITS;
	fixed_t b2 = _b2 << FRACBITS;
	fixed_t r, g, b;
	fixed_t rs, gs, bs;

	if (start > end)
	{
		swapvalues (start, end);
		r = r2;
		g = g2;
		b = b2;
		rs = r1 - r2;
		gs = g1 - g2;
		bs = b1 - b2;
	}
	else
	{
		r = r1;
		g = g1;
		b = b1;
		rs = r2 - r1;
		gs = g2 - g1;
		bs = b2 - b1;
	}
	if (start == end)
	{
		start = GPalette.Remap[start];
		Remap[start] = ColorMatcher.Pick(r >> FRACBITS, g >> FRACBITS, b >> FRACBITS);
		Palette[start] = PalEntry(r >> FRACBITS, g >> FRACBITS, b >> FRACBITS);
		Palette[start].a = start == 0 ? 0 : 255;
	}
	else
	{
		rs /= (end - start);
		gs /= (end - start);
		bs /= (end - start);
		for (int i = start; i <= end; ++i)
		{
			int j = GPalette.Remap[i];
			Remap[j] = ColorMatcher.Pick(r >> FRACBITS, g >> FRACBITS, b >> FRACBITS);
			Palette[j] = PalEntry(j == 0 ? 0 : 255, r >> FRACBITS, g >> FRACBITS, b >> FRACBITS);
			r += rs;
			g += gs;
			b += bs;
		}
	}
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

void FRemapTable::AddDesaturation(int start, int end, double r1, double g1, double b1, double r2, double g2, double b2)
{
	r1 = clamp(r1, 0.0, 2.0);
	g1 = clamp(g1, 0.0, 2.0);
	b1 = clamp(b1, 0.0, 2.0);
	r2 = clamp(r2, 0.0, 2.0);
	g2 = clamp(g2, 0.0, 2.0);
	b2 = clamp(b2, 0.0, 2.0);

	if (start > end)
	{
		swapvalues(start, end);
		swapvalues(r1, r2);
		swapvalues(g1, g2);
		swapvalues(b1, b2);
	}

	r2 -= r1;
	g2 -= g1;
	b2 -= b1;
	r1 *= 255;
	g1 *= 255;
	b1 *= 255;

	for(int c = start; c <= end; c++)
	{
		double intensity = (GPalette.BaseColors[c].r * 77 +
							GPalette.BaseColors[c].g * 143 +
							GPalette.BaseColors[c].b * 37) / 256.0;

		PalEntry pe = PalEntry(	MIN(255, int(r1 + intensity*r2)), 
								MIN(255, int(g1 + intensity*g2)), 
								MIN(255, int(b1 + intensity*b2)));

		int cc = GPalette.Remap[c];

		Remap[cc] = ColorMatcher.Pick(pe);
		Palette[cc] = pe;
		Palette[cc].a = cc == 0 ? 0:255;
	}
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

void FRemapTable::AddToTranslation(const char * range)
{
	int start,end;
	bool desaturated = false;
	Scanner sc(range, int(strlen(range)));
	sc.SetScriptIdentifier("Translation");

	try
	{
		sc.MustGetToken(TK_IntConst);
		start = sc->number;
		sc.MustGetToken(':');
		sc.MustGetToken(TK_IntConst);
		end = sc->number;
		sc.MustGetToken('=');
		if (start < 0 || start > 255 || end < 0 || end > 255)
		{
			sc.ScriptMessage(Scanner::ERROR, "Palette index out of range");
			return;
		}

		sc.GetNextToken();

		if (sc->token != '[' && sc->token != '%')
		{
			int pal1,pal2;

			if(sc->token != TK_IntConst)
				sc.ScriptMessage(Scanner::ERROR, "Expected integer constant.");
			pal1 = sc->number;
			sc.MustGetToken(':');
			sc.MustGetToken(TK_IntConst);
			pal2 = sc->number;
			AddIndexRange(start, end, pal1, pal2);
		}
		else if (sc->token == '[')
		{ 
			// translation using RGB values
			int r1,g1,b1,r2,g2,b2;

			sc.MustGetToken(TK_IntConst);
			r1 = sc->number;
			sc.MustGetToken(',');

			sc.MustGetToken(TK_IntConst);
			g1 = sc->number;
			sc.MustGetToken(',');

			sc.MustGetToken(TK_IntConst);
			b1 = sc->number;
			sc.MustGetToken(']');
			sc.MustGetToken(':');
			sc.MustGetToken('[');

			sc.MustGetToken(TK_IntConst);
			r2 = sc->number;
			sc.MustGetToken(',');

			sc.MustGetToken(TK_IntConst);
			g2 = sc->number;
			sc.MustGetToken(',');

			sc.MustGetToken(TK_IntConst);
			b2 = sc->number;
			sc.MustGetToken(']');

			AddColorRange(start, end, r1, g1, b1, r2, g2, b2);
		}
		else if (sc->token == '%')
		{
			// translation using RGB values
			double r1,g1,b1,r2,g2,b2;

			sc.MustGetToken('[');
			sc.GetNextToken();
			if (sc->token != TK_IntConst && sc->token != TK_FloatConst) sc.ScriptMessage(Scanner::ERROR, "Expected floating point constant.");
			r1 = sc->decimal;
			sc.MustGetToken(',');

			sc.GetNextToken();
			if (sc->token != TK_IntConst && sc->token != TK_FloatConst) sc.ScriptMessage(Scanner::ERROR, "Expected floating point constant.");
			g1 = sc->decimal;
			sc.MustGetToken(',');

			sc.GetNextToken();
			if (sc->token != TK_IntConst && sc->token != TK_FloatConst) sc.ScriptMessage(Scanner::ERROR, "Expected floating point constant.");
			b1 = sc->decimal;
			sc.MustGetToken(']');
			sc.MustGetToken(':');
			sc.MustGetToken('[');

			sc.GetNextToken();
			if (sc->token != TK_IntConst && sc->token != TK_FloatConst) sc.ScriptMessage(Scanner::ERROR, "Expected floating point constant.");
			r2 = sc->decimal;
			sc.MustGetToken(',');

			sc.GetNextToken();
			if (sc->token != TK_IntConst && sc->token != TK_FloatConst) sc.ScriptMessage(Scanner::ERROR, "Expected floating point constant.");
			g2 = sc->decimal;
			sc.MustGetToken(',');

			sc.GetNextToken();
			if (sc->token != TK_IntConst && sc->token != TK_FloatConst) sc.ScriptMessage(Scanner::ERROR, "Expected floating point constant.");
			b2 = sc->decimal;
			sc.MustGetToken(']');

			AddDesaturation(start, end, r1, g1, b1, r2, g2, b2);
		}
	}
	catch (CRecoverableError &err)
	{
		Printf("Error in translation '%s':\n%s\n", range, err.GetMessage());
	}
}

//----------------------------------------------------------------------------
//
// Stores a copy of this translation in the DECORATE translation table
//
//----------------------------------------------------------------------------

int FRemapTable::StoreTranslation()
{
	unsigned int i;

	for (i = 0; i < translationtables[TRANSLATION_Decorate].Size(); i++)
	{
		if (*this == *translationtables[TRANSLATION_Decorate][i])
		{
			// A duplicate of this translation already exists
			return TRANSLATION(TRANSLATION_Decorate, i);
		}
	}
	if (translationtables[TRANSLATION_Decorate].Size() >= MAX_DECORATE_TRANSLATIONS)
	{
		I_Error("Too many DECORATE translations");
	}
	FRemapTable *newtrans = new FRemapTable;
	*newtrans = *this;
	i = translationtables[TRANSLATION_Decorate].Push(newtrans);
	return TRANSLATION(TRANSLATION_Decorate, i);
}


//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

TArray<PalEntry> BloodTranslationColors;

int CreateBloodTranslation(PalEntry color)
{
	unsigned int i;

	if (BloodTranslationColors.Size() == 0)
	{
		// Don't use the first slot.
		translationtables[TRANSLATION_Blood].Push(NULL);
		BloodTranslationColors.Push(0);
	}

	for (i = 1; i < BloodTranslationColors.Size(); i++)
	{
		if (color.r == BloodTranslationColors[i].r &&
			color.g == BloodTranslationColors[i].g &&
			color.b == BloodTranslationColors[i].b)
		{
			// A duplicate of this translation already exists
			return i;
		}
	}
	if (BloodTranslationColors.Size() >= MAX_DECORATE_TRANSLATIONS)
	{
		I_Error("Too many blood colors");
	}
	FRemapTable *trans = new FRemapTable;
	for (i = 0; i < 256; i++)
	{
		int bright = MAX(MAX(GPalette.BaseColors[i].r, GPalette.BaseColors[i].g), GPalette.BaseColors[i].b);
		PalEntry pe = PalEntry(color.r*bright/255, color.g*bright/255, color.b*bright/255);
		int entry = ColorMatcher.Pick(pe.r, pe.g, pe.b);

		trans->Palette[i] = pe;
		trans->Remap[i] = entry;
	}
	translationtables[TRANSLATION_Blood].Push(trans);
	return BloodTranslationColors.Push(color);
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

FRemapTable *TranslationToTable(int translation)
{
	unsigned int type = GetTranslationType(translation);
	unsigned int index = GetTranslationIndex(translation);
	TAutoGrowArray<FRemapTablePtr, FRemapTable *> *slots;

	if (type <= 0 || type >= NUM_TRANSLATION_TABLES)
	{
		return NULL;
	}
	slots = &translationtables[type];
	if (index >= slots->Size())
	{
		return NULL;
	}
	return slots->operator[](index);
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

static void PushIdentityTable(int slot)
{
	FRemapTable *table = new FRemapTable;
	table->MakeIdentity();
	translationtables[slot].Push(table);
}

//----------------------------------------------------------------------------
//
// R_InitTranslationTables
// Creates the translation tables to map the green color ramp to gray,
// brown, red. Assumes a given structure of the PLAYPAL.
//
//----------------------------------------------------------------------------

void R_InitTranslationTables ()
{
	int i;//, j;

	// Each player gets two translations. Doom and Strife don't use the
	// extra ones, but Heretic and Hexen do. These are set up during
	// netgame arbitration and as-needed, so they just get to be identity
	// maps until then so they won't be invalid.
	for (i = 0; i < MAXPLAYERS; ++i)
	{
		PushIdentityTable(TRANSLATION_Players);
		PushIdentityTable(TRANSLATION_PlayersExtra);
	}
	// The menu player also gets a separate translation table
	PushIdentityTable(TRANSLATION_Players);

	// The three standard translations from Doom or Heretic (seven for Strife),
	// plus the generic ice translation.
	for (i = 0; i < 8; ++i)
	{
		PushIdentityTable(TRANSLATION_Standard);
	}

	// Each player corpse has its own translation so they won't change
	// color if the player who created them changes theirs.
	for (i = 0; i < BODYQUESIZE; ++i)
	{
		PushIdentityTable(TRANSLATION_PlayerCorpses);
	}

	// Create the standard translation tables
	for (i = 0x70; i < 0x80; i++)
	{ // map green ramp to gray, brown, red
		translationtables[TRANSLATION_Standard][0]->Remap[i] = 0x60 + (i&0xf);
		translationtables[TRANSLATION_Standard][1]->Remap[i] = 0x40 + (i&0xf);
		translationtables[TRANSLATION_Standard][2]->Remap[i] = 0x20 + (i&0xf);

		translationtables[TRANSLATION_Standard][0]->Palette[i] = GPalette.BaseColors[0x60 + (i&0xf)] | MAKEARGB(255,0,0,0);
		translationtables[TRANSLATION_Standard][1]->Palette[i] = GPalette.BaseColors[0x40 + (i&0xf)] | MAKEARGB(255,0,0,0);
		translationtables[TRANSLATION_Standard][2]->Palette[i] = GPalette.BaseColors[0x20 + (i&0xf)] | MAKEARGB(255,0,0,0);
	}

	// Create the ice translation table, based on Hexen's. Alas, the standard
	// Doom palette has no good substitutes for these bluish-tinted grays, so
	// they will just look gray unless you use a different PLAYPAL with Doom.

	BYTE IcePaletteRemap[16];
	for (i = 0; i < 16; ++i)
	{
		IcePaletteRemap[i] = ColorMatcher.Pick (IcePalette[i][0], IcePalette[i][1], IcePalette[i][2]);
	}
	FRemapTable *remap = translationtables[TRANSLATION_Standard][7];
	for (i = 0; i < 256; ++i)
	{
		int r = GPalette.BaseColors[i].r;
		int g = GPalette.BaseColors[i].g;
		int b = GPalette.BaseColors[i].b;
		int v = (r*77 + g*143 + b*37) >> 12;
		remap->Remap[i] = IcePaletteRemap[v];
		remap->Palette[i] = PalEntry(255, IcePalette[v][0], IcePalette[v][1], IcePalette[v][2]);
	}

	// set up shading tables for shaded columns
	// 16 colormap sets, progressing from full alpha to minimum visible alpha

	/*BYTE *table = shadetables;

	// Full alpha
	for (i = 0; i < 16; ++i)
	{
		ShadeFakeColormap[i].Color = ~0u;
		ShadeFakeColormap[i].Desaturate = ~0u;
		ShadeFakeColormap[i].Next = NULL;
		ShadeFakeColormap[i].Maps = table;

		for (j = 0; j < NUMCOLORMAPS; ++j)
		{
			int a = (NUMCOLORMAPS - j) * 256 / NUMCOLORMAPS * (16-i);
			for (int k = 0; k < 256; ++k)
			{
				BYTE v = (((k+2) * a) + 256) >> 14;
				table[k] = MIN<BYTE> (v, 64);
			}
			table += 256;
		}
	}
	for (i = 0; i < NUMCOLORMAPS*16*256; ++i)
	{
		assert(shadetables[i] <= 64);
	}*/

	// Set up a guaranteed identity map
	for (i = 0; i < 256; ++i)
	{
		identitymap[i] = i;
	}
}

//----------------------------------------------------------------------------
//
//
//
//----------------------------------------------------------------------------

void R_DeinitTranslationTables()
{
	for (int i = 0; i < NUM_TRANSLATION_TABLES; ++i)
	{
		for (unsigned int j = 0; j < translationtables[i].Size(); ++j)
		{
			if (translationtables[i][j] != NULL)
			{
				delete translationtables[i][j];
				translationtables[i][j] = NULL;
			}
		}
		translationtables[i].Clear();
	}
	BloodTranslationColors.Clear();
}

//----------------------------------------------------------------------------
//
// [RH] Create a player's translation table based on a given mid-range color.
// [GRB] Split to 2 functions (because of player setup menu)
//
//----------------------------------------------------------------------------

static void SetRemap(FRemapTable *table, int i, float r, float g, float b)
{
	int ir = clamp (int(r * 255.f), 0, 255);
	int ig = clamp (int(g * 255.f), 0, 255);
	int ib = clamp (int(b * 255.f), 0, 255);
	table->Remap[i] = ColorMatcher.Pick (ir, ig, ib);
	table->Palette[i] = PalEntry(255, ir, ig, ib);
}
