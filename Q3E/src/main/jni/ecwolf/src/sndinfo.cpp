/*
** sndinfo.cpp
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

#include "wl_def.h"
#include "m_swap.h"
#include "m_random.h"
#include "id_sd.h"
#include "wl_iwad.h"
#include "w_wad.h"
#include "scanner.h"
#include "zdoomsupport.h"
#include <SDL_mixer.h>


// TFuncDeleter can't be used here since Mix_FreeChunk has various attributes
// on Windows that make it difficult to forward declare. We'd prefer to not
// include SDL_mixer.h in more places than we absolutely have to.
struct Mix_ChunkDeleter
{
	inline explicit Mix_ChunkDeleter(Mix_Chunk *obj) { Mix_FreeChunk(obj); }
};

////////////////////////////////////////////////////////////////////////////////
//
// Sound Index
//
////////////////////////////////////////////////////////////////////////////////

SoundIndex::SoundIndex(const char* logical)
{
	index = SoundInfo.FindSound(logical);
}

////////////////////////////////////////////////////////////////////////////////
//
// Sound Data
//
////////////////////////////////////////////////////////////////////////////////

SoundData::SoundData() : priority(50), isAlias(false)
{
	lump[0] = lump[1] = lump[2] = -1;
}

SoundData::~SoundData()
{
}

template<>
struct TMoveInsert<SoundData>
{
	explicit TMoveInsert(void *mem, const SoundData &other)
	{
		SoundData *data = ::new (mem) SoundData();
		data->logicalName = other.logicalName;
		data->priority = other.priority;
		data->isAlias = other.isAlias;
		data->aliasLinks = other.aliasLinks;
		(void)TMoveInsert<TUniquePtr<Mix_Chunk, Mix_ChunkDeleter> >(&data->digitalData, other.digitalData);
		(void)TMoveInsert<TUniquePtr<byte[]> >(&data->adlibData, other.adlibData);
		(void)TMoveInsert<TUniquePtr<byte[]> >(&data->speakerData, other.speakerData);
		memcpy(data->lump, other.lump, sizeof(data->lump));
	}
};

////////////////////////////////////////////////////////////////////////////////
//
// SNDINFO
//
////////////////////////////////////////////////////////////////////////////////

SoundInformation SoundInfo;

struct SoundInformation::HashIndex
{
	public:
		static const unsigned int HASHTABLE_SIZE = 4096;

		unsigned int	index;
		HashIndex		*next;
};

SoundInformation::SoundInformation() : hashTable(NULL)
{
	sounds.Push(nullIndex);
	lastPlayTicks.Push(0);
}

SoundInformation::~SoundInformation()
{
	if(hashTable)
	{
		// Clean up hash table links
		for(unsigned int i = 0;i < HashIndex::HASHTABLE_SIZE;++i)
		{
			HashIndex *index = hashTable[i].next;
			while(index)
			{
				HashIndex *toDelete = index;
				index = index->next;
				delete toDelete;
			}
		}
		delete[] hashTable;
	}
}

SoundData &SoundInformation::AddSound(const char* logical)
{
	// Look for an already defined sound.  The list isn't indexed so we have to
	// do a linear search.
	for(unsigned int i = 0;i < sounds.Size();++i)
	{
		if(sounds[i].logicalName.CompareNoCase(logical) == 0)
			return sounds[i];
	}

	// Create new sound and set the logical name
	unsigned int idx = sounds.Push(SoundData());
	SoundData &data = sounds[idx];
	data.logicalName = logical;
	data.index = SoundIndex(idx);
	lastPlayTicks.Push(0);
	return data;
}

void SoundInformation::CreateHashTable()
{
	hashTable = new HashIndex[HashIndex::HASHTABLE_SIZE];
	memset(hashTable, 0, sizeof(HashIndex)*HashIndex::HASHTABLE_SIZE);

	// Sound index 0 is nullIndex so we don't need to hash it.
	// Use separate chaining to resolve collisions.
	for(unsigned int i = 1;i < sounds.Size();++i)
	{
		SoundData &data = sounds[i];

		if(data.isAlias)
		{
			// Clean up undefined links
			for(unsigned int j = data.aliasLinks.Size();j-- > 0;)
			{
				SoundData &link = sounds[data.aliasLinks[j]];
				if(link.IsNull())
					data.aliasLinks.Delete(j);
			}

			// No links?  No hash.
			if(data.aliasLinks.Size() == 0)
				continue;
		}
		else
		{
			// Don't hash empty sounds
			if(data.IsNull())
				continue;
		}

		HashIndex *tid = &hashTable[MakeKey(data.logicalName) % HashIndex::HASHTABLE_SIZE];
		while(tid->index)
		{
			if(tid->next)
				tid = tid->next;
			else
			{
				tid->next = new HashIndex;
				tid = tid->next;
				memset(tid, 0, sizeof(HashIndex));
			}
		}

		tid->index = i;
	}
}

SoundIndex SoundInformation::FindSound(const char* logical) const
{
	HashIndex *index = &hashTable[MakeKey(logical) % HashIndex::HASHTABLE_SIZE];
	if(index->index == 0)
		return SoundIndex();

	while(sounds[index->index].logicalName.CompareNoCase(logical) != 0)
	{
		index = index->next;
		if(index == NULL)
			return SoundIndex();
	}

	return SoundIndex(index->index);
}

int SoundInformation::GetMusicLumpNum(FString song) const
{
	const int lump = Wads.CheckNumForName(song, ns_music);
	const int wad = Wads.GetLumpFile(lump);

	const MusicData *alias = MusicAliases.CheckKey(FName(song, true));
	if(alias && wad <= alias->WadNum)
		return GetMusicLumpNum(alias->Name);

	return lump;
}

void SoundInformation::Init()
{
	printf("S_Init: Reading SNDINFO defintions.\n");

	int lastLump = 0;
	int lump = 0;
	while((lump = Wads.FindLump("SNDINFO", &lastLump)) != -1)
	{
		ParseSoundInformation(lump);
	}

	CreateHashTable();
}

void SoundInformation::ParseSoundInformation(int lumpNum)
{
	Scanner sc(lumpNum);

	unsigned int excludeDepth = 0; // $if $endif
	while(sc.TokensLeft() != 0)
	{
		if(sc.CheckToken('$'))
		{
			sc.MustGetToken(TK_Identifier);
			if(excludeDepth)
			{
				if(sc->str.CompareNoCase("endif") == 0)
					--excludeDepth;
			}
			else if(sc->str.CompareNoCase("alias") == 0 || sc->str.CompareNoCase("random") == 0)
			{
				bool isRandom = sc->str.CompareNoCase("random") == 0;

				if(!sc.GetNextString())
					sc.ScriptMessage(Scanner::ERROR, "Expected logical name.");
				SoundIndex aliasIndex;
				{
					// Adding sounds may change the location of the SoundData
					// memory so we must look it up again after we're done.
					SoundData &alias = AddSound(sc->str);
					alias.isAlias = true;
					aliasIndex = alias.index;
				}

				TArray<SoundIndex> aliasLinks;

				if(isRandom)
					sc.MustGetToken('{');
				do
				{
					if(!sc.GetNextString())
						sc.ScriptMessage(Scanner::ERROR, "Expected logical name.");
					aliasLinks.Push(AddSound(sc->str).index);
				}
				while(isRandom && !sc.CheckToken('}'));

				sounds[aliasIndex].aliasLinks = aliasLinks;
			}
			else if(sc->str.Left(2).CompareNoCase("if") == 0)
			{
				if(!IWad::CheckGameFilter(sc->str.Mid(2)))
					++excludeDepth;
			}
			else if(sc->str.CompareNoCase("endif") == 0) {}
			else if(sc->str.CompareNoCase("musicalias") == 0)
			{
				if(!sc.GetNextString())
					sc.ScriptMessage(Scanner::ERROR, "Expected music alias name.");
				FString musicName = sc->str;

				if(!sc.GetNextString())
					sc.ScriptMessage(Scanner::ERROR, "Expected music lump name.");

				MusicData data = {sc->str, Wads.GetLumpFile(lumpNum)};
				MusicAliases[musicName] = data;
			}
			else
				sc.ScriptMessage(Scanner::ERROR, "Unknown command '%s'.", sc->str.GetChars());
		}
		else
		{
			if(excludeDepth)
			{
				sc.GetNextToken();
				continue;
			}

			if(!sc.GetNextString())
				sc.ScriptMessage(Scanner::ERROR, "Expected logical name.");
			assert(sc->str[0] != '}');

			SoundData &idx = AddSound(sc->str);
			// Initialize/clean in case we're replacing
			idx.isAlias = false;
			idx.digitalData.Reset();
			idx.adlibData.Reset();
			idx.speakerData.Reset();
			idx.lump[0] = idx.lump[1] = idx.lump[2] = -1;

			bool hasAlternatives = false;

			if(sc.CheckToken('{'))
				hasAlternatives = true;

			unsigned int i = 0;
			do
			{
				if(hasAlternatives)
				{
					if(i > 2)
					{
						sc.MustGetToken('}');
						break;
					}
					else if(sc.CheckToken('}'))
					{
						break;
					}
				}

				if(!sc.GetNextString())
					sc.ScriptMessage(Scanner::ERROR, "Expected lump name for '%s'.\n", idx.logicalName.GetChars());

				if(sc->str.Compare("NULL") == 0)
					continue;

				int sndLump = Wads.CheckNumForName(sc->str, ns_sounds);
				if(sndLump == -1)
					continue;

				idx.lump[i] = sndLump;
				if(i == 0)
				{
					idx.digitalData.Reset(SD_PrepareSound(sndLump));
				}
				else
				{
					unsigned int length = Wads.LumpLength(sndLump);
					TUniquePtr<byte[]> &data = i == 1 ? idx.adlibData : idx.speakerData;
					data.Reset(new byte[length]);

					FWadLump soundReader = Wads.OpenLumpNum(sndLump);
					soundReader.Read(data.Get(), length);

					if(i == 1 || idx.lump[1] == -1)
						idx.priority = ReadLittleShort(&data[4]);
				}
			}
			while(++i, hasAlternatives);
		}
	}
}

static FRandom pr_randsound("RandSound");
const SoundData	&SoundInformation::operator[] (const SoundIndex &index) const
{
	const SoundData &ret = sounds[index];
	if(ret.isAlias)
		return operator[](ret.aliasLinks[ret.aliasLinks.Size() > 1 ? pr_randsound() % ret.aliasLinks.Size() : 0]);
	return ret;
}
