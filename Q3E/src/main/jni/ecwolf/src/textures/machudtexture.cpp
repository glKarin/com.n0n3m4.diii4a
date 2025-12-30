/*
** machudtexture.cpp
**
**---------------------------------------------------------------------------
** Copyright 2015 Braden Obrzut
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
**
*/

#include "textures.h"
#include "tmemory.h"
#include "v_palette.h"
#include "w_wad.h"

class FMacHudTexture : public FTexture
{
public:
	FMacHudTexture (const char *name, int lumpnum, int offset, FileReader &file, bool masked);

	const BYTE *GetColumn (unsigned int column, const Span **spans_out)
	{
		if (!Pixels)
			MakeTexture ();

		if ((unsigned)column >= (unsigned)Width)
		{
			if (WidthMask + 1 == Width)
			{
				column &= WidthMask;
			}
			else
			{
				column %= Width;
			}
		}
		if (spans_out != NULL)
		{
			if (Spans == NULL)
			{
				Spans = CreateSpans(Pixels);
			}
			*spans_out = Spans[column];
		}
		return Pixels.Get() + column*Height;
	}
	const BYTE *GetPixels ()
	{
		if(!Pixels)
			MakeTexture();
		return Pixels;
	}
	void Unload () { Pixels.Reset(); }

protected:
	TUniquePtr<BYTE[]> Pixels;
	Span **Spans;
	int Offset;
	bool Masked;

	virtual void MakeTexture ();
};

FMacHudTexture::FMacHudTexture(const char* name, int lumpnum, int offset, FileReader &file, bool masked)
: FTexture(name, lumpnum), Spans(NULL), Offset(offset), Masked(masked)
{
	file.Seek(offset, SEEK_SET);

	WORD width, height;
	file >> width >> height;
	Width = BigShort(width);
	Height = BigShort(height);
	yScale = xScale = 2*FRACUNIT;
	Offset += 4;

	if(Masked)
	{
		// In this case the previous was the x/y offset.
		LeftOffset = -256-Width;
		TopOffset = -172-Height;

		file >> width >> height;
		Width = BigShort(width);
		Height = BigShort(height);
		Offset += 4;

		UseType = TEX_Sprite;
	}
	else
		UseType = TEX_MiscPatch;

	CalcBitSize();
}

void FMacHudTexture::MakeTexture()
{
	unsigned int Size = Width*Height;
	if(Masked) Size <<= 1;

	Pixels = new BYTE[Width*Height];
	TUniquePtr<BYTE[]> newpix(new BYTE[Size]);

	FWadLump lump = Wads.OpenLumpNum(SourceLump);
	lump.Seek(Offset, SEEK_SET);
	lump.Read(newpix.Get(), Size);

	FlipNonSquareBlockRemap (Pixels, newpix, Width, Height, Width, GPalette.Remap);
	if(Masked)
	{
		Size >>= 1;
		for(unsigned int y = Height;y-- > 0;)
		{
			for(unsigned int x = Width;x-- > 0;)
			{
				if(newpix[Size+y*Width+x])
					Pixels[x*Height+y] = 0;
			}
		}
	}
}

static bool CheckHudGraphicsLump(FWadLump &lump, unsigned int numgraphics, DWORD* offsets)
{
	if(lump.GetLength() < (long)(numgraphics*4)) return false;

	lump.Read(offsets, numgraphics*4);
	// Check potentially valid data.
	for(unsigned int i = 0;i < numgraphics;++i)
	{
		offsets[i] = BigLong(offsets[i]);
		if((unsigned)lump.GetLength() < offsets[i]) return false;
	}

	return true;
}

void FTextureManager::InitMacHud()
{
	enum { NUM_HUDGRAPHICS = 47, NUM_INTERGRAPHICS = 3 };
	static const char* const MacHudGfxNames[NUM_HUDGRAPHICS] = {
		"STFST00", "STFST01",
		"STFTR00", "STFTL00",
		"STFEVL0",
		"STFST10", "STFST11",
		"STFTR10", "STRTL10",
		"STFDEAD0",
		"STKEYS1", "STKEYS2",
		"KNIFA0", "KNIFB0", "KNIFC0", "KNIFD0",
		"PISGA0", "PISGB0", "PISGC0", "PISGD0",
		"MCHGA0", "MCHGB0", "MCHGC0", "MCHGD0",
		"CHGGA0", "CHGGB0", "CHGGC0", "CHGGD0",
		"FLMGA0", "FLMGB0", "FLMGC0", "FLMGD0",
		"MISGA0", "MISGB0", "MISGC0", "MISGD0",
		"FONTN048", "FONTN049", "FONTN050", "FONTN051", "FONTN052",
		"FONTN053", "FONTN054", "FONTN055", "FONTN056", "FONTN057",
		"STBACK",
	};

	static const char* const InterBJGfxNames[NUM_INTERGRAPHICS] = {
		"L_GUY1", "L_GUY2", "L_BJWINS"
	};

	int lumpnum = Wads.CheckNumForName("FACE640", ns_graphics);
	if(lumpnum != -1)
	{
		FWadLump lump = Wads.OpenLumpNum(lumpnum);
		DWORD offsets[NUM_HUDGRAPHICS];
		if(CheckHudGraphicsLump(lump, NUM_HUDGRAPHICS, offsets))
		{
			for(unsigned int i = 0;i < NUM_HUDGRAPHICS;++i)
				AddTexture(new FMacHudTexture(MacHudGfxNames[i], lumpnum, offsets[i], lump, i >= 12 && i <= 35));
		}
	}

	lumpnum = Wads.CheckNumForName("INTERBJ", ns_graphics);
	if(lumpnum != -1)
	{
		FWadLump lump = Wads.OpenLumpNum(lumpnum);
		DWORD offsets[NUM_INTERGRAPHICS];
		if(CheckHudGraphicsLump(lump, NUM_INTERGRAPHICS, offsets))
		{
			for(unsigned int i = 0;i < NUM_INTERGRAPHICS;++i)
				AddTexture(new FMacHudTexture(InterBJGfxNames[i], lumpnum, offsets[i], lump, false));
		}
	}
}
