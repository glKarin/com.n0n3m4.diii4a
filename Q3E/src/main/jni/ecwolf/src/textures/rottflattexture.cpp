/*
** rottflattexture.cpp
** Rise of the Triad flats
**
**---------------------------------------------------------------------------
** Copyright 2013 Braden Obrzut
** Copyright 2004-2006 Randy Heit
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

#include "wl_def.h"
#include "files.h"
#include "w_wad.h"
#include "m_swap.h"
#include "templates.h"
#include "v_palette.h"
#include "textures.h"

//==========================================================================
//
// A texture that is a Rise of the Triad Flat
//
//==========================================================================

class FRottFlatTexture : public FTexture
{
public:
	FRottFlatTexture (int lumpnum, FileReader &file);
	~FRottFlatTexture ();

	const BYTE *GetColumn (unsigned int column, const Span **spans_out);
	const BYTE *GetPixels ();
	void Unload ();

protected:
	BYTE *Pixels;
	Span **Spans;

	virtual void MakeTexture ();
};

//==========================================================================
//
// Checks if the lump is a Rise of the Triad Flat
//
//==========================================================================

static bool CheckIfRottFlat(FileReader &file)
{
	if(file.GetLength() < 9) return false;
	
	WORD header[2];
	file.Seek(0, SEEK_SET);
	file.Read(header, 4);

	WORD Width = LittleShort(header[0]);
	WORD Height = LittleShort(header[1]);
	if(file.GetLength() == Width*Height+8)
		return true;
	return false;
}

//==========================================================================
//
//
//
//==========================================================================

FTexture *RottFlatTexture_TryCreate(FileReader &file, int lumpnum)
{
	if(!CheckIfRottFlat(file))
		return NULL;
	return new FRottFlatTexture(lumpnum, file);
}

//==========================================================================
//
//
//
//==========================================================================

FRottFlatTexture::FRottFlatTexture(int lumpnum, FileReader &file)
: FTexture(NULL, lumpnum), Pixels(0), Spans(0)
{
	WORD header[4];
	file.Seek(0, SEEK_SET);
	file.Read(header, 8);
	Width = LittleShort(header[0]);
	Height = LittleShort(header[1]);
	LeftOffset = LittleShort(header[2]);
	TopOffset = LittleShort(header[3]);
	CalcBitSize ();
}

//==========================================================================
//
//
//
//==========================================================================

FRottFlatTexture::~FRottFlatTexture ()
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
//
//
//==========================================================================

void FRottFlatTexture::Unload ()
{
	if(Pixels != NULL)
	{
		delete[] Pixels;
		Pixels = NULL;
	}
}

//==========================================================================
//
//
//
//==========================================================================

const BYTE *FRottFlatTexture::GetPixels ()
{
	if (Pixels == NULL)
	{
		MakeTexture ();
	}
	return Pixels;
}

//==========================================================================
//
//
//
//==========================================================================

const BYTE *FRottFlatTexture::GetColumn (unsigned int column, const Span **spans_out)
{
	if (Pixels == NULL)
	{
		MakeTexture ();
	}
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
	return Pixels + column*Height;
}


//==========================================================================
//
//
//
//==========================================================================

void FRottFlatTexture::MakeTexture ()
{
	FMemLump lump = Wads.ReadLump (SourceLump);
	const BYTE* data = ((const BYTE*)lump.GetMem())+8;

	Pixels = new BYTE[Width*Height];
	memset(Pixels, 0, Width*Height);
	memcpy(Pixels, data, Width*Height);
}
