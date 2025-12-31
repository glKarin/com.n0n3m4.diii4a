/*
** sndinfo.h
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

#ifndef __SNDINFO_H__
#define __SNDINFO_H__

#include "tarray.h"
#include "tmemory.h"
#include "name.h"
#include "zstring.h"

class SoundInformation;

struct Mix_Chunk;
struct Mix_ChunkDeleter;

class SoundIndex
{
	public:
		SoundIndex(int index = 0) : index(index) {}
		SoundIndex(const char* logical);

		bool IsNull() const { return index == 0; }

		operator int() const { return index; }
	private:
		int index;
};

class SoundData
{
	public:
		enum Type
		{
			DIGITAL,
			ADLIB,
			PCSPEAKER
		};

		SoundData();
		~SoundData();

		byte* GetAdLibData() const { return adlibData; }
		Mix_Chunk *GetDigitalData() const { return digitalData; }
		unsigned short GetPriority() const { return priority; }
		byte* GetSpeakerData() const { return speakerData; }
		bool HasType(Type type=ADLIB) const { return lump[type] != -1; }
		bool IsNull() const { return lump[0] == -1 && lump[1] == -1 && lump[2] == -1 && !isAlias; }

	private:
		const SoundData &operator= (const SoundData &);
		SoundData(const SoundData &);

	protected:
		FString logicalName;
		SoundIndex index;
		TUniquePtr<Mix_Chunk, Mix_ChunkDeleter> digitalData;
		TUniquePtr<byte[]> adlibData, speakerData;
		int lump[3];
		unsigned short priority;

		bool isAlias;
		TArray<SoundIndex> aliasLinks;

		friend class SoundInformation;
		friend struct TMoveInsert<SoundData>;
};

class SoundInformation
{
	public:
		SoundInformation();
		~SoundInformation();

		SoundIndex		FindSound(const char* logical) const;
		void			Init();
		const SoundData	&operator[] (const char* logical) const { return operator[](FindSound(logical)); }
		const SoundData	&operator[] (const SoundIndex &index) const;
		uint32_t		GetLastPlayTick(const SoundData &sound) const { return lastPlayTicks[sound.index]; }
		int				GetMusicLumpNum(FString song) const;
		void			SetLastPlayTick(const SoundData &sound, uint32_t value) const { lastPlayTicks[sound.index] = value; }

	protected:
		SoundData	&AddSound(const char* logical);
		void		CreateHashTable();
		void		ParseSoundInformation(int lumpNum);

	private:
		struct MusicData
		{
			FString Name;
			int WadNum;
		};

		SoundData			nullIndex;
		TArray<SoundData>	sounds;
		TArray<uint32_t>	lastPlayTicks;
		TMap<FName, MusicData> MusicAliases;

		struct HashIndex;
		HashIndex*	hashTable;
};
extern SoundInformation	SoundInfo;

#endif
