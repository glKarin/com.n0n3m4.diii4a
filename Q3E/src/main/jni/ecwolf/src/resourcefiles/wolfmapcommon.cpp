/*
** wolfmapcommon.cpp
**
**---------------------------------------------------------------------------
** Copyright 2013 Braden Obrzut
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

#include "wolfmapcommon.h"

// Only important thing to remember is that both
// Compression methods work on WORDs rather than bytes.

void FMapLump::ExpandCarmack(const unsigned char* in, unsigned char* out)
{
	const unsigned char* const end = out + ReadLittleShort((const BYTE*)in);
	const unsigned char* const start = out;
	in += 2;

	const unsigned char* copy;
	BYTE length;
	while(out < end)
	{
		length = *in++;
		if(length == 0 && (*in == CARMACK_NEARTAG || *in == CARMACK_FARTAG))
		{
			*out++ = in[1];
			*out++ = in[0];
			in += 2;
			continue;
		}
		else if(*in == CARMACK_NEARTAG)
		{
			copy = out-(in[1]*2);
			in += 2;
		}
		else if(*in == CARMACK_FARTAG)
		{
			copy = start+(ReadLittleShort((const BYTE*)(in+1))*2);
			in += 3;
		}
		else
		{
			*out++ = length;
			*out++ = *in++;
			continue;
		}
		if(out+(length*2) > end)
			break;
		while(length-- > 0)
		{
			*out++ = *copy++;
			*out++ = *copy++;
		}
	}
}

void FMapLump::ExpandRLEW(const unsigned char* in, unsigned char* out, const DWORD length, const WORD rlewTag)
{
	const unsigned char* const end = out+length;

	while(out < end)
	{
		if(ReadLittleShort((const BYTE*)in) != rlewTag)
		{
			*out++ = *in++;
			*out++ = *in++;
		}
		else
		{
			WORD count = ReadLittleShort((const BYTE*)(in+2));
			WORD input = ReadLittleShort((const BYTE*)(in+4));
			in += 6;
			while(count-- > 0)
			{
				WriteLittleShort((BYTE*)out, input);
				out += 2;
			}
		}
	}
}

int FMapLump::FillCache()
{
	if(LumpSize == 0)
		return 1;

	unsigned int PlaneSize = Header.Width*Header.Height*2;

	Cache = new char[LumpSize];
	strcpy(Cache, "WDC3.1");
	WriteLittleShort((BYTE*)&Cache[10], rtlMap ? 4 : 3);
	WriteLittleShort((BYTE*)&Cache[12], 16);
	WriteLittleShort((BYTE*)&Cache[HEADERSIZE-4], Header.Width);
	WriteLittleShort((BYTE*)&Cache[HEADERSIZE-2], Header.Height);
	memcpy(&Cache[14], Header.Name, 16);

	// Read map data and expand it
	unsigned char* output = reinterpret_cast<unsigned char*>(Cache+HEADERSIZE);
	for(unsigned int i = 0;i < PLANES;++i)
	{
		// ChaosEdit HACK: Likely in order to save a few bytes ChaosEdit sets
		// the second and third map plane offsets to be the same (since vanilla
		// doesn't use the data). If we see this we need to zero fill the plane.
		if(i == 2 && Header.PlaneOffset[1] == Header.PlaneOffset[2] && !rtlMap)
		{
			memset(output, 0, PlaneSize);
			output += PlaneSize;
			continue;
		}

		if(Header.PlaneLength[i])
		{
			unsigned char* input = new unsigned char[Header.PlaneLength[i]];
			Owner->Reader->Seek(Header.PlaneOffset[i], SEEK_SET);
			Owner->Reader->Read(input, Header.PlaneLength[i]);

			if(carmackCompressed)
			{
				unsigned char* tempOut = new unsigned char[ReadLittleShort((BYTE*)input)];
				ExpandCarmack(input, tempOut);
				ExpandRLEW(tempOut+2, output, ReadLittleShort((const BYTE*)tempOut), rlewTag);
				delete[] tempOut;
			}
			else
			{
				if(rtlMap)
					ExpandRLEW(input, output, PlaneSize, rlewTag);
				else
					ExpandRLEW(input+2, output, ReadLittleShort((const BYTE*)input), rlewTag);
			}

			delete[] input;
		}
		else
			memset(output, 0, PlaneSize);
		output += PlaneSize;

		// RTL maps don't have a floor/ceiling texture plane so insert one
		// We do this after the things plane has been read
		if(rtlMap && i == 1)
		{
			const WORD floorTex = ReadLittleShort((const BYTE*)(Cache+HEADERSIZE))-0xB4;
			const WORD ceilingTex = ReadLittleShort((const BYTE*)(Cache+HEADERSIZE+2))-0xC6;
			const WORD fill = (floorTex&0xFF)|((ceilingTex&0xFF)<<8);
			WORD *out = reinterpret_cast<WORD*>(output);
			for(unsigned int j = 0;j < PlaneSize/2;++j)
				*out++ = fill;
			output += PlaneSize;
		}
	}
	return 1;
}
