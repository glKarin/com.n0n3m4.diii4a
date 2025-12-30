/*
** file_rtl.cpp
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

#include "doomerrors.h"
#include "wl_def.h"
#include "resourcefile.h"
#include "w_wad.h"
#include "m_swap.h"
#include "zstring.h"
#include "wolfmapcommon.h"

#pragma pack(1)
struct RtlMapHeader
{
public:
	DWORD usedFlag;
	DWORD crc; // Useless for us, this is just a random value to check maps in multiplayer
	DWORD rlewTag;
	DWORD mapSpecials;
	DWORD planeOffset[PLANES];
	DWORD planeLength[PLANES];
	char name[24];
};
#pragma pack()

class FRtlFile : public FResourceFile
{
	public:
		FRtlFile(const char* filename, FileReader *file);
		~FRtlFile();

		FResourceLump *GetLump(int lump);
		bool Open(bool quiet);

	private:
		FMapLump*	Lumps;
};

FRtlFile::FRtlFile(const char* filename, FileReader *file) : FResourceFile(filename, file), Lumps(NULL)
{
}

FRtlFile::~FRtlFile()
{
	if(Lumps != NULL)
		delete[] Lumps;
}

FResourceLump *FRtlFile::GetLump(int lump)
{
	return &Lumps[lump];
}

bool FRtlFile::Open(bool quiet)
{
	RtlMapHeader header[100];

	Reader->Seek(8, SEEK_SET);
	Reader->Read(&header, sizeof(RtlMapHeader)*100);

	// We allocate 2 lumps per map so...
	static const unsigned int NUM_MAP_LUMPS = 2;
	NumLumps = 0;
	for(unsigned int i = 0;i < 100;++i)
	{
		if(header[i].usedFlag)
			NumLumps += NUM_MAP_LUMPS;
	}

	Lumps = new FMapLump[NumLumps];
	// Preserve map position in the MAPxy notation
	for(unsigned int i = 0;i < 100;++i)
	{
		if(!header[i].usedFlag)
			continue;

		// Map marker
		FMapLump &markerLump = Lumps[i*NUM_MAP_LUMPS];
		char lumpname[6];
		sprintf(lumpname, "MAP%02d", i != 99 ? i+1 : 0);
		markerLump.Owner = this;
		markerLump.LumpNameSetup(lumpname);
		markerLump.Namespace = ns_global;
		markerLump.LumpSize = 0;

		// Make the data lump
		FMapLump &dataLump = Lumps[i*NUM_MAP_LUMPS+1];
		dataLump.Owner = this;
		dataLump.LumpNameSetup("PLANES");
		dataLump.Namespace = ns_global;
		for(unsigned int j = 0;j < PLANES;j++)
		{
			dataLump.Header.PlaneOffset[j] = LittleLong(header[i].planeOffset[j]);
			dataLump.Header.PlaneLength[j] = LittleLong(header[i].planeLength[j]);
		}
		dataLump.rlewTag = LittleLong(header[i].rlewTag);
		dataLump.carmackCompressed = false;
		dataLump.rtlMap = true;
		dataLump.Header.Width = 128;
		dataLump.Header.Height = 128;
		memcpy(dataLump.Header.Name, header[i].name, 24);
		dataLump.LumpSize += dataLump.Header.Width*dataLump.Header.Height*RTLCONVERTEDPLANES*2;
	}
	if(!quiet) Printf(", %d lumps\n", NumLumps);
	return true;
}

FResourceFile *CheckRtl(const char *filename, FileReader *file, bool quiet)
{
	char head[4];
	DWORD version;

	if(file->GetLength() >= static_cast<signed>(8+sizeof(RtlMapHeader)*100))
	{
		file->Seek(0, SEEK_SET);
		file->Read(&head, 4);
		file->Read(&version, 4);
		file->Seek(0, SEEK_SET);
		if((!memcmp(head, "RTL", 4) || !memcmp(head, "RTC", 4)) && LittleLong(version) == 0x0101)
		{
			FResourceFile *rf = new FRtlFile(filename, file);
			if(rf->Open(quiet)) return rf;
			rf->Reader = NULL; // to avoid destruction of reader
			delete rf;
		}
	}
	return NULL;
}
