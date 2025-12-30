/*
** file_gamemaps.cpp
**
**---------------------------------------------------------------------------
** Copyright 2011 Braden Obrzut
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

#include "filesys.h"
#include "doomerrors.h"
#include "wl_def.h"
#include "resourcefile.h"
#include "tmemory.h"
#include "w_wad.h"
#include "m_swap.h"
#include "zstring.h"
#include "wolfmapcommon.h"

struct FMapLump;

class FGamemaps : public FResourceFile
{
	public:
		FGamemaps(const char* filename, FileReader *file);
		~FGamemaps();

		FResourceLump *GetLump(int lump);
		bool Open(bool quiet);

	private:
		FMapLump* Lumps;

		TUniquePtr<FileReader> mapheadReader;
		// Gamemaps = Carmack+RLEW, Maptemp = RLEW
		bool carmacked;
};

FGamemaps::FGamemaps(const char* filename, FileReader *file) : FResourceFile(filename, file), Lumps(NULL), mapheadReader(NULL)
{
	FString path(filename);
	int lastSlash = path.LastIndexOfAny("/\\:");
	int lastDot = path.LastIndexOf('.');
	FString extension = path.Mid(lastDot+1);

	carmacked = path.Mid(lastSlash+1, 7).CompareNoCase("maptemp") != 0;

	path = path.Left(lastSlash+1);

	FString mapheadFile = FString("maphead.") + extension;
	if(Wads.CheckIfWadLoaded(path.Left(lastSlash)) == -1)
	{
		File directory(path.Len() > 0 ? path : ".");
		mapheadFile = path + directory.getInsensitiveFile(mapheadFile, true);

		mapheadReader = new FileReader();
		if(!mapheadReader->Open(mapheadFile))
			mapheadReader.Reset();
	}
	else // Embedded vanilla data?
	{
		FLumpReader *lreader = reinterpret_cast<FLumpReader *>(file);

		for(DWORD i = 0; i < lreader->LumpOwner()->LumpCount(); ++i)
		{
			FResourceLump *lump = lreader->LumpOwner()->GetLump(i);
			if(lump->FullName.CompareNoCase(mapheadFile) == 0)
			{
				mapheadReader = lump->NewReader();
				break;
			}
		}
	}

	if(!mapheadReader)
	{
		FString error;
		error.Format("Could not open gamemaps since %s is missing.", mapheadFile.GetChars());
		throw CRecoverableError(error);
	}
}

FGamemaps::~FGamemaps()
{
	if(Lumps != NULL)
		delete[] Lumps;
}

FResourceLump *FGamemaps::GetLump(int lump)
{
	return &Lumps[lump];
}

bool FGamemaps::Open(bool quiet)
{
	WORD rlewTag;

	// Read the map head.
	// First two bytes is the tag for the run length encoding
	// Followed by offsets in the gamemaps file, we'll count until we
	// hit a 0 offset.
	unsigned int NumPossibleMaps = (mapheadReader->GetLength()-2)/4;
	mapheadReader->Seek(0, SEEK_SET);
	DWORD* offsets = new DWORD[NumPossibleMaps];
	mapheadReader->Read(&rlewTag, 2);
	rlewTag = LittleShort(rlewTag);
	mapheadReader->Read(offsets, NumPossibleMaps*4);
	for(NumLumps = 0;NumLumps < NumPossibleMaps && offsets[NumLumps] != 0;++NumLumps)
		offsets[NumLumps] = LittleLong(offsets[NumLumps]);

	// We allocate 2 lumps per map so...
	static const unsigned int NUM_MAP_LUMPS = 2;
	NumLumps *= NUM_MAP_LUMPS;

	Lumps = new FMapLump[NumLumps];
	for(unsigned int i = 0;i < NumLumps/NUM_MAP_LUMPS;++i)
	{
		// Map marker
		FMapLump &markerLump = Lumps[i*NUM_MAP_LUMPS];
		// Hey we don't need to use a temporary name here!
		// First map is MAP01 and so forth.
		char lumpname[14];
		sprintf(lumpname, "MAP%02d", i+1);
		markerLump.Owner = this;
		markerLump.LumpNameSetup(lumpname);
		markerLump.Namespace = ns_global;
		markerLump.LumpSize = 0;

		// Make the data lump
		FMapLump &dataLump = Lumps[i*NUM_MAP_LUMPS+1];
		dataLump.rlewTag = rlewTag;
		dataLump.carmackCompressed = carmacked;
		BYTE header[PLANES*6+20];
		Reader->Seek(offsets[i], SEEK_SET);
		Reader->Read(&header, PLANES*6+20);

		dataLump.Owner = this;
		dataLump.LumpNameSetup("PLANES");
		dataLump.Namespace = ns_global;
		for(unsigned int j = 0;j < PLANES;j++)
		{
			dataLump.Header.PlaneOffset[j] = ReadLittleLong(&header[4*j]);
			dataLump.Header.PlaneLength[j] = ReadLittleShort(&header[PLANES*4+2*j]);
		}
		dataLump.Header.Width = ReadLittleShort(&header[PLANES*6]);
		dataLump.Header.Height = ReadLittleShort(&header[PLANES*6+2]);
		memcpy(dataLump.Header.Name, &header[PLANES*6+4], 16);
		dataLump.LumpSize += dataLump.Header.Width*dataLump.Header.Height*PLANES*2;
	}
	delete[] offsets;
	if(!quiet) Printf(", %d lumps\n", NumLumps);
	return true;
}

FResourceFile *CheckGamemaps(const char *filename, FileReader *file, bool quiet)
{
	FString fname(filename);
	int lastSlash = fname.LastIndexOfAny("/\\:");
	if(lastSlash != -1)
		fname = fname.Mid(lastSlash+1, 8);
	else
		fname = fname.Left(8);

	// File must be gamemaps.something or maptemp.something
	if(fname.Len() == 8 && (fname.CompareNoCase("gamemaps") == 0 || fname.Left(7).CompareNoCase("maptemp") == 0))
	{
		FResourceFile *rf = new FGamemaps(filename, file);
		if(rf->Open(quiet)) return rf;
		rf->Reader = NULL; // to avoid destruction of reader
		delete rf;
	}
	return NULL;
}
