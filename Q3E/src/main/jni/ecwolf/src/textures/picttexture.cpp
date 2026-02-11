/*
** picttexture.cpp
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
** Implements the PICT format for Mac mods. This is a very complex format
** so we'll be implementing only what we care to read.
**
*/

#include "m_swap.h"
#include "textures.h"
#include "tmemory.h"
#include "v_palette.h"
#include "w_wad.h"

#pragma pack(1)
struct PictRect
{
	WORD y1, x1, y2, x2;
};

struct ColorTabEntry
{
	WORD index;
	// 16 bpc color, but we're only considered about the high byte.
	BYTE r, rLo, g, gLo, b, bLo;
};

struct ColorTab
{
	DWORD id;
	WORD flags;
	WORD numColors;
};

struct PixMap
{
	WORD pitch;
	PictRect bounds;
	WORD version; // Should be 0
	WORD packing;
	DWORD packedSize;
	fixed hRes, vRes;
	WORD type;
	WORD bpp;
	WORD compCount;
	WORD compSize;
	DWORD planePitch;
	DWORD colorTablePtr;
	DWORD reserved;

	// Not technically part of a pixmap, but might work for us
	ColorTab colorTable;
};
#pragma pack()

enum EOpCodes
{
	OP_NOP,
	OP_Clip,
	OP_PackBitsRect = 0x98,
	OP_EndPic = 0xFF
};

class FPictTexture : public FTexture
{
public:
	FPictTexture (int lumpnum, PictRect bounds, BYTE version);

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
	BYTE version;

	virtual void MakeTexture ();
};

FPictTexture::FPictTexture(int lumpnum, PictRect bounds, BYTE version)
: FTexture(NULL, lumpnum), Spans(NULL), version(version)
{
	bounds.y1 = BigShort(bounds.y1);
	bounds.x1 = BigShort(bounds.x1);
	bounds.y2 = BigShort(bounds.y2);
	bounds.x2 = BigShort(bounds.x2);

	Width = bounds.x2-bounds.x1;
	Height = bounds.y2-bounds.y1;
	LeftOffset = bounds.x1;
	TopOffset = bounds.y1;
	CalcBitSize();
}

void FPictTexture::MakeTexture()
{
	BYTE remap[256] = {};

	FMemLump lump = Wads.ReadLump(SourceLump);
	const BYTE* data = (const BYTE*)lump.GetMem();
	const BYTE* end = data+lump.GetSize();
	switch(version) // skip header
	{
		default:
			Printf("Invalid version\n");
			return;
		case 1:
			data += 0xC;
			break;
		case 2:
			data += 0x28;
			break;
		case 3:
			data += 0x26;
			break;
	}

	// We'll read row major to make things easy and then flip to column major
	// while remapping to our palette.
	TUniquePtr<BYTE[]> rowMajor(new BYTE[Width*Height]);
	Pixels.Reset(new BYTE[Width*Height]);

	while(data < end)
	{
		WORD opcode;
		if(version > 1)
		{
			opcode = ReadBigShort(data);
			data += 2;
		}
		else
			opcode = *data++;

		switch(opcode)
		{
		default:
			Printf("Unknown opcode %04X in %s\n", opcode, Name.GetChars());
		case OP_EndPic:
			goto FinishTexture;

		case OP_NOP:
			break;
		case OP_Clip: // Do we care about the clipping rectangle?
		{
			WORD regionSize = ReadBigShort(data);
			if(regionSize != 0xA)
			{
				// Region is in an undocumented format if it's not rectangular.
				Printf("Non-rectangular clipping region in %s.\n", Name.GetChars());
				return;
			}
			data += regionSize;
			break;
		}
		case OP_PackBitsRect:
		{
			PixMap pm = *(const PixMap *)data;
			const ColorTabEntry *colors = (const ColorTabEntry *)(data+sizeof(PixMap));
			pm.pitch = BigShort(pm.pitch)&0x7FFF;
			pm.colorTable.numColors = BigShort(pm.colorTable.numColors);

			if(BigShort(pm.compCount) != 1 || BigShort(pm.compSize) != 8)
			{
				Printf("Only 8bpp images are supported.\n");
				return;
			}

			// Convert 48-bit palette to 24-bit for our remapping code
			DWORD rgb[256] = {};
			for(unsigned int i = 0;i < pm.colorTable.numColors;++i)
			{
				WORD index = BigShort(colors[i].index);
				if(index > 255)
					Printf("Color index %d for entry %d out of range in palette.\n", index, i);
				else
					rgb[index] = MAKERGB(colors[i].r, colors[i].g, colors[i].b);
			}
			GPalette.MakeRemap(rgb, remap, NULL, pm.colorTable.numColors+1);

			data += sizeof(PixMap)+(pm.colorTable.numColors+1)*sizeof(ColorTabEntry)
			// Skip over src and dest rectangles and mode unless we know we
			// need to do something  with them.
				+ 2*sizeof(PictRect) + 2;

			BYTE *dest = rowMajor.Get();
			BYTE *start = dest;
			for(unsigned int i = 0;i < Height;++i, dest = (start += Width))
			{
				// Get size of compressed row
				int rowSize;
				if(pm.pitch <= 250)
					rowSize = *data++;
				else
				{
					rowSize = ReadBigShort(data);
					data += 2;
				}

				while(rowSize > 0)
				{
					BYTE code = *data++;
					if(code & 0x80) // Run of single color
					{
						BYTE color = *data++;
						memset(dest, color, 257-code);
						dest += 257-code;
						rowSize -= 2;
					}
					else // Copy from source
					{
						memcpy(dest, data, code+1);
						data += code+1;
						dest += code+1;
						rowSize -= code+2;
					}
				}
			}
			break;
		}

		}
	}

FinishTexture:
	FlipNonSquareBlockRemap(Pixels.Get(), rowMajor.Get(), Width, Height, Width, remap);
}

FTexture *PictTexture_TryCreate(FileReader & file, int lumpnum)
{
	BYTE header[0x28];
	file.Seek(0, SEEK_SET);
	long bytesRead = file.Read(header, 0x28);
	if(bytesRead < 0xD) // Version 1 files with single end of file instruction
		return NULL;

	// At this point we can determine if we have a version 2 or version 2
	// extended format.
	BYTE version = 2;
	// Technically version 2 (extended) files don't need to have the size here
	// but it looks like they put the lower bytes of the size anyway.
	if(ReadBigShort(header) != (Wads.LumpLength(lumpnum)&0xFFFF))
		return NULL;
	if(ReadBigShort(&header[0xA]) == 0x1101)
	{
		version = 1;
	}
	// Basic check for version 2. 0x28 is the size of V2 header, V2 extended is
	// 0x26 but the file must have an end of file instruction any way.
	else if(bytesRead < 0x28 && ReadBigShort(&header[0xA]) != 0x0011 && ReadBigLong(&header[0xC]) != 0x02FF0C00)
		return NULL;
	// Check version 2 or 2 extended
	else if(ReadBigLong(&header[0x10]) != 0xFFFFFFFF)
	{
		// Final version 2 extended check
		if(ReadBigLong(&header[0x10]) != 0xFFFE0000 && ReadBigShort(&header[0x24]) != 0)
			return NULL;
		version = 3;
	}
	// Version 2 final check
	else if(ReadBigLong(&header[0x24]) != 0)
		return NULL;

	return new FPictTexture(lumpnum, *(PictRect *)&header[2], version);
}
