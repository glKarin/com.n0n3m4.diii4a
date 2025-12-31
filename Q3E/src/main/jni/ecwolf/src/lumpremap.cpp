/*
** lumpremap.cpp
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

#include "lumpremap.h"
#include "w_wad.h"
#include "zstring.h"
#include "scanner.h"

#define TIMER_VALUE_DEFAULT 114

struct PSprite
{
public:
	PSprite(const FString &name, LumpRemapper::PSpriteType type) : name(name), type(type) {}

	FString name;
	LumpRemapper::PSpriteType type;
};

static TMap<int, unsigned int> sampleRateMap;
static TMap<FName, LumpRemapper> remaps;
static TArray<PSprite> psprites;

LumpRemapper::LumpRemapper(const char* extension) : digiTimerValue(TIMER_VALUE_DEFAULT),
	loaded(false), mapLumpName(extension)
{
	mapLumpName.ToUpper();
	mapLumpName += "MAP";
}

void LumpRemapper::AddFile(const char* extension, FResourceFile *file, Type type)
{
	LumpRemapper *iter = remaps.CheckKey(extension);
	if(iter == NULL)
	{
		LumpRemapper remaper(extension);
		remaper.AddFile(file, type);
		remaps.Insert(extension, remaper);
		return;
	}
	iter->AddFile(file, type);
}

void LumpRemapper::AddFile(FResourceFile *file, Type type)
{
	RemapFile rFile;
	rFile.file = file;
	rFile.type = type;
	files.Push(rFile);
}

void LumpRemapper::ClearRemaps()
{
	sampleRateMap.Clear();
	remaps.Clear();
	psprites.Clear();
}

void LumpRemapper::DoRemap()
{
	if(!LoadMap())
		return;

	for(unsigned int i = 0;i < files.Size();i++)
	{
		RemapFile &file = files[i];

		// Register sample rate
		sampleRateMap[Wads.GetLumpFile(files[i].file->GetFirstLump())] = digiTimerValue;

		int temp = 0; // Use to count something
		int temp2 = 0;
		for(unsigned int i = 0;i < file.file->LumpCount();i++)
		{
			FResourceLump *lump = file.file->GetLump(i);
			int oldNamespace = lump->Namespace;
			switch(file.type)
			{
				case AUDIOT:
					if(lump->Namespace == ns_sounds)
					{
						if(i < sounds.Size() && lump->LumpSize > 0)
							lump->LumpNameSetup(sounds[i]);
						temp++;
					}
					else if(lump->Namespace == ns_music && i-temp < music.Size() && lump->LumpSize > 0)
					{
						lump->LumpNameSetup(music[i-temp]);
					}
					break;
				case VGAGRAPH:
					if(i < graphics.Size())
					{
						lump->LumpNameSetup(graphics[i]);
					}
					break;
				case VSWAP:
					if(lump->Namespace == ns_flats)
					{
						if(i < textures.Size() && lump->LumpSize > 0)
							lump->LumpNameSetup(textures[i]);
						temp++;
						temp2++;
					}
					else if(lump->Namespace == ns_sprites)
					{
						if(i-temp < sprites.Size() && lump->LumpSize > 0)
							lump->LumpNameSetup(sprites[i-temp]);
						temp2++;
					}
					else if(lump->Namespace == ns_sounds && i-temp2 < digitalsounds.Size())
					{
						if(lump->LumpSize > 0)
							lump->LumpNameSetup(digitalsounds[i-temp2]);
					}
					break;
				default:
					break;
			}
			lump->Namespace = oldNamespace;
			lump->DoFinishRemap();
		}
	}
	Wads.InitHashChains();
}

LumpRemapper::PSpriteType LumpRemapper::IsPSprite(int lumpnum)
{
	for(unsigned int i = 0;i < psprites.Size();++i)
	{
		if(Wads.CheckLumpName(lumpnum, psprites[i].name))
			return psprites[i].type;
	}
	return PSPR_NONE;
}

unsigned int LumpRemapper::LumpSampleRate(FResourceFile *Owner)
{
	const int file = Wads.GetLumpFile(Owner->GetFirstLump());
	const unsigned int* rate = sampleRateMap.CheckKey(file);
	if(rate)
		return 1000000/(256 - *rate);
	return 1000000/(256 - TIMER_VALUE_DEFAULT);
}

bool LumpRemapper::LoadMap()
{
	if(loaded)
		return true;
	if(Wads.GetNumLumps() == 0)
		return false;

	int lump = Wads.GetNumForName(mapLumpName);
	if(lump == -1)
	{
		printf("\n");
		return false;
	}
	FWadLump mapLump = Wads.OpenLumpNum(lump);

	char* mapData = new char[Wads.LumpLength(lump)];
	mapLump.Read(mapData, Wads.LumpLength(lump));
	Scanner sc(mapData, Wads.LumpLength(lump));
	sc.SetScriptIdentifier(Wads.GetLumpFullName(lump));
	delete[] mapData;

	ParseMap(sc);

	loaded = true;
	return true;
}

void LumpRemapper::LoadMap(const char* name, const char* data, unsigned int length)
{
	if(loaded)
		return;

	Scanner sc(data, length);
	sc.SetScriptIdentifier(name);

	ParseMap(sc);

	loaded = true;
}

void LumpRemapper::LoadMap(const char* extension, const char* name, const char* data, unsigned int length)
{
	LumpRemapper *iter = remaps.CheckKey(extension);
	if(iter == NULL)
	{
		LumpRemapper remaper(extension);
		remaper.LoadMap(name, data, length);
		remaps.Insert(extension, remaper);
		return;
	}

	iter->LoadMap(name, data, length);
}

void LumpRemapper::ParseMap(Scanner &sc)
{
	while(sc.TokensLeft())
	{
		if(!sc.CheckToken(TK_Identifier))
			sc.ScriptMessage(Scanner::ERROR, "Expected identifier in map.\n");

		bool parseSprites = false;
		TArray<FString> *map = NULL;
		if(sc->str.Compare("graphics") == 0)
			map = &graphics;
		else if(sc->str.Compare("sprites") == 0)
		{
			parseSprites = true;
			map = &sprites;
		}
		else if(sc->str.Compare("sounds") == 0)
			map = &sounds;
		else if(sc->str.Compare("digitalsounds") == 0)
		{
			// Check for sample rate change
			if(sc.CheckToken('('))
			{
				sc.MustGetToken(TK_Identifier);
				if(sc->str.Compare("timervalue") != 0)
					sc.ScriptMessage(Scanner::ERROR, "Expected timervalue.\n");
				sc.MustGetToken('=');
				sc.MustGetToken(TK_IntConst);
				digiTimerValue = sc->number;
				sc.MustGetToken(')');
			}
			map = &digitalsounds;
		}
		else if(sc->str.Compare("music") == 0)
			map = &music;
		else if(sc->str.Compare("textures") == 0)
			map = &textures;
		else
			sc.ScriptMessage(Scanner::ERROR, "Unknown map section '%s'.\n", sc->str.GetChars());

		if(!sc.CheckToken('{'))
			sc.ScriptMessage(Scanner::ERROR, "Expected '{'.");
		if(!sc.CheckToken('}'))
		{
			while(true)
			{
				if(!sc.CheckToken(TK_StringConst))
					sc.ScriptMessage(Scanner::ERROR, "Expected string constant.\n");
				const FString spriteName = sc->str;
				map->Push(spriteName);
				if(parseSprites && sc.CheckToken(':'))
				{
					sc.MustGetToken(TK_Identifier);
					if(sc->str.Compare("pspr") == 0)
						psprites.Push(PSprite(spriteName, PSPR_NORMAL));
					else if(sc->str.Compare("blakepspr") == 0)
						psprites.Push(PSprite(spriteName, PSPR_BLAKE));
					else
						sc.ScriptMessage(Scanner::ERROR, "Expected pspr modifier.\n");
				}
				if(sc.CheckToken('}'))
					break;
				if(!sc.CheckToken(','))
					sc.ScriptMessage(Scanner::ERROR, "Expected ','.\n");
			}
		}
	}
}

void LumpRemapper::RemapAll()
{
	TMap<FName, LumpRemapper>::Pair *pair;
	for(TMap<FName, LumpRemapper>::Iterator iter(remaps);iter.NextPair(pair);)
	{
		pair->Value.DoRemap();
		pair->Value.files.Clear();
	}
}

