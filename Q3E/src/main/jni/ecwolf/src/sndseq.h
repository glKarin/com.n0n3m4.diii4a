/*
** sndseq.h
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

#ifndef __SNDSEQ_H__
#define __SNDSEQ_H__

#include "wl_def.h"
#include "gamemap.h"
#include "tarray.h"
#include "name.h"

class SoundSequence;
struct SndSeqInstruction;

enum SequenceType
{
	SEQ_OpenNormal,
	SEQ_CloseNormal,
	SEQ_OpenBlazing,
	SEQ_CloseBlazing,

	NUM_SEQ_TYPES
};

struct SndSeqInstruction
{
public:
	unsigned int Instruction;
	FName Sound;
	unsigned int Argument;
	unsigned int ArgumentRand;
};

/* The SoundSequence class holds the set of instructions to execute for a given
 * sound sequence OR points to which sound sequence to play for a given event.
 *
 * After parsing all of this information should be static so we can just pass
 * a pointer to the first instruction to a player and only refer back to this
 * object for meta data.
 */
class SoundSequence
{
public:
	SoundSequence();

	void AddInstruction(const SndSeqInstruction &instr);
	void Clear();
	const SoundSequence &GetSequence(SequenceType type) const;
	FName GetStopSound() const { return StopSound; }
	FName GetSeqName() const { return Name; }
	void SetFlag(unsigned int flag, bool set);
	void SetSequence(SequenceType type, FName sequence);
	const SndSeqInstruction *Start() const;

private:
	friend class SndSeqTable;

	TArray<SndSeqInstruction> Instructions;
	FName StopSound;
	FName AltSequences[NUM_SEQ_TYPES];
	unsigned int Flags;

	FName Name;
};

class SndSeqTable
{
public:
	void Init();

	const SoundSequence &operator() (FName sequence, SequenceType type) const;
protected:
	void ParseSoundSequence(int lumpnum);

private:
	TMap<FName, SoundSequence> Sequences;
};
extern SndSeqTable SoundSeq;

class SndSeqPlayer
{
public:
	SndSeqPlayer(const SoundSequence &sequence, MapSpot source);
	~SndSeqPlayer();

	bool IsPlaying() const { return Playing; }
	void Tick();
	void SetSource(MapSpot source) { Source = source; }
	void Stop();

private:
	friend FArchive &operator<< (FArchive &, SndSeqPlayer *&);

	const SoundSequence &Sequence;
	const SndSeqInstruction *Current;
	MapSpot Source;

	unsigned int Delay;
	bool Playing;
	bool WaitForDone;
};

FArchive &operator<< (FArchive &, SndSeqPlayer *&);

#endif
