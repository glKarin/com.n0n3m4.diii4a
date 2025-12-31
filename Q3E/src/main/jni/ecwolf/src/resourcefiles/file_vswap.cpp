/*
** file_vswap.cpp
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
#include "resourcefile.h"
#include "w_wad.h"
#include "lumpremap.h"
#include "zstring.h"
#include "wl_main.h"

// Some sounds in the VSwap file are multiparty so we need mean to concationate
// them.
struct FVSwapSound : public FResourceLump
{
	protected:
		static const char WAV_HEADER[44];

		struct Chunk
		{
			public:
				int	offset;
				int	length;
		};

		Chunk *chunks;
		unsigned short numChunks;
		unsigned int numOrigSamples;
		unsigned int origSampleRate;

	public:
		FVSwapSound(int maxNumChunks) : FResourceLump(), numChunks(0), origSampleRate(0)
		{
			if(maxNumChunks < 0)
				maxNumChunks = 0;
			chunks = new Chunk[maxNumChunks];
		}
		~FVSwapSound()
		{
			delete[] chunks;
		}

		void AddChunk(int offset, int length)
		{
			LumpSize += length;

			chunks[numChunks].offset = offset;
			chunks[numChunks].length = length;
			numChunks++;
		}

		// Since SDL_mixer sucks, we need to resample our sounds at load time.
		// Unfortunately it's difficult to tell just how big the sound is until
		// we finish remapping.
		void CalculateLumpSize()
		{
			// Get the sample rate.
			origSampleRate = LumpRemapper::LumpSampleRate(Owner);

			LumpSize = sizeof(WAV_HEADER);
			numOrigSamples = 0;
			for(unsigned int i = 0;i < numChunks;i++)
				numOrigSamples += chunks[i].length;

			if(numOrigSamples == 0)
				LumpSize = 0;
			else
				LumpSize += static_cast<int>(double(numOrigSamples*2*param_samplerate)/origSampleRate);
		}

		void DoFinishRemap()
		{
			// Sample rate may have changed, recalculate size
			CalculateLumpSize();
		}

		int FillCache()
		{
			const unsigned int samples = (LumpSize - sizeof(WAV_HEADER))/2;

			Cache = new char[LumpSize];
			if(LumpSize == 0)
				return 1;

			// Copy our template header and update various values
			memcpy(Cache, WAV_HEADER, sizeof(WAV_HEADER));
			*(DWORD*)(Cache+4) = LittleLong(samples*2 + sizeof(WAV_HEADER) - 8);
			*(DWORD*)(Cache+24) = LittleLong(param_samplerate);
			*(DWORD*)(Cache+28) = LittleLong(param_samplerate*2);
			*(DWORD*)(Cache+sizeof(WAV_HEADER)-4) = LittleLong(samples*2);

			// Read original data
			BYTE* origdata = new BYTE[numOrigSamples];
			unsigned int pos = 0;
			unsigned int i;
			for(i = 0;i < numChunks;i++)
			{
				Owner->Reader->Seek(chunks[i].offset, SEEK_SET);
				Owner->Reader->Read(origdata+pos, chunks[i].length);
				pos += chunks[i].length;
			}

			// Do resampling to param_samplerate 16-bit with linear interpolation
			static const fixed sampleStep = static_cast<fixed>(((double)origSampleRate / param_samplerate)*FRACUNIT);
			SWORD* data = (SWORD*)(Cache+sizeof(WAV_HEADER));
			i = 0;
			fixed samplefrac = 0;
			unsigned int sample = 0;
			while(i++ < samples)
			{
				SWORD curSample = (SWORD(origdata[sample]) - 128)<<8;
				SWORD nextSample = sample+1 < numOrigSamples ? (SWORD(origdata[sample+1]) - 128)<<8 : curSample;

				*data++ = LittleShort(curSample + ((samplefrac*fixed(nextSample-curSample))>>FRACBITS));

				samplefrac += sampleStep;
				if(samplefrac > FRACUNIT)
				{
					samplefrac -= FRACUNIT;
					++sample;
				}
			}
			delete[] origdata;
			return 1;
		}
};
const char FVSwapSound::WAV_HEADER[44] = {
	'R','I','F','F',0,0,0,0,'W','A','V','E',
	'f','m','t',' ',16,0,0,0,1,0,1,0,
	(char)0x82,0x17,0,0,0x37,0x04,0,0,2,0,16,0,
	'd','a','t','a',0,0,0,0
};

class FVSwap : public FResourceFile
{
	public:
		FVSwap(const char* filename, FileReader *file) : FResourceFile(filename, file), spriteStart(0), soundStart(0), Lumps(NULL), SoundLumps(NULL), vswapFile(filename)
		{
			int lastSlash = vswapFile.LastIndexOfAny("/\\:");
			extension = vswapFile.Mid(lastSlash+7);
		}
		~FVSwap()
		{
			if(Lumps != NULL)
				delete[] Lumps;
			if(SoundLumps != NULL)
			{
				for(unsigned int i = 0;i < NumLumps-soundStart;i++)
					delete SoundLumps[i];
				delete[] SoundLumps;
			}
		}

		bool Open(bool quiet)
		{
			BYTE header[6];
			Reader->Read(header, 6);
			WORD numChunks = ReadLittleShort(&header[0]);

			spriteStart = ReadLittleShort(&header[2]);
			soundStart = ReadLittleShort(&header[4]);

			Lumps = new FUncompressedLump[soundStart];


			BYTE* data = new BYTE[6*numChunks];
			Reader->Read(data, 6*numChunks);

			for(unsigned int i = 0;i < soundStart;i++)
			{
				char lumpname[9];
				mysnprintf(lumpname, 9, "VSP%05d", i);

				Lumps[i].Owner = this;
				Lumps[i].LumpNameSetup(lumpname);
				Lumps[i].Namespace = i >= soundStart ? ns_sounds : (i >= spriteStart ? ns_sprites : ns_flats);
				if(Lumps[i].Namespace == ns_flats)
					Lumps[i].Flags |= LUMPF_DONTFLIPFLAT;
				Lumps[i].Position = ReadLittleLong(&data[i*4]);
				Lumps[i].LumpSize = ReadLittleShort(&data[i*2 + 4*numChunks]);
			}

			// Now for sounds we need to get the last Chunk and read the sound information.
			int soundMapOffset = ReadLittleLong(&data[(numChunks-1)*4]);
			int soundMapSize = ReadLittleShort(&data[(numChunks-1)*2 + 4*numChunks]);
			unsigned int numDigi = soundMapSize/4;
			byte* soundMap = new byte[soundMapSize];
			SoundLumps = new FVSwapSound*[numDigi];
			Reader->Seek(soundMapOffset, SEEK_SET);
			Reader->Read(soundMap, soundMapSize);
			for(unsigned int i = 0;i < numDigi;i++)
			{
				WORD start = ReadLittleShort(&soundMap[i*4]);
				WORD end = i == numDigi - 1 ? numChunks - soundStart - 1 : ReadLittleShort(&soundMap[i*4 + 4]);

				if(start + soundStart > numChunks - 1)
				{ // Read past end of chunks.
					numDigi = i;
					break;
				}

				char lumpname[9];
				mysnprintf(lumpname, 9, "VSP%05d", i+soundStart);
				SoundLumps[i] = new FVSwapSound(end-start);
				SoundLumps[i]->Owner = this;
				SoundLumps[i]->LumpNameSetup(lumpname);
				SoundLumps[i]->Namespace = ns_sounds;
				for(unsigned int j = start;j < end && end + soundStart < numChunks;j++)
					SoundLumps[i]->AddChunk(ReadLittleLong(&data[(soundStart+j)*4]), ReadLittleShort(&data[(soundStart+j)*2 + numChunks*4]));
				SoundLumps[i]->CalculateLumpSize();
			}
			delete[] soundMap;

			// Number of lumps is not the number of chunks, but the number of
			// chunks up to sounds + how many sounds are formed from the chunks.
			NumLumps = soundStart + numDigi;

			delete[] data;
			if(!quiet) Printf(", %d lumps\n", NumLumps);

			LumpRemapper::AddFile(extension, this, LumpRemapper::VSWAP);
			return true;
		}

		FResourceLump *GetLump(int no)
		{
			if(no < soundStart)
				return &Lumps[no];
			return SoundLumps[no-soundStart];
		}

	private:
		unsigned short spriteStart;
		unsigned short soundStart;

		FUncompressedLump* Lumps;
		FVSwapSound* *SoundLumps;

		FString	extension;
		FString	vswapFile;
};

FResourceFile *CheckVSwap(const char *filename, FileReader *file, bool quiet)
{
	FString fname(filename);
	int lastSlash = fname.LastIndexOfAny("/\\:");
	if(lastSlash != -1)
		fname = fname.Mid(lastSlash+1, 5);
	else
		fname = fname.Left(5);

	if(fname.Len() == 5 && fname.CompareNoCase("vswap") == 0) // file must be vswap.something
	{
		FResourceFile *rf = new FVSwap(filename, file);
		if(rf->Open(quiet)) return rf;
		rf->Reader = NULL; // to avoid destruction of reader
		delete rf;
	}
	return NULL;
}
