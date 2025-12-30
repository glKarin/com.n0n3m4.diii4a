/*
** sndseq.cpp
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

#include "id_sd.h"
#include "m_random.h"
#include "scanner.h"
#include "sndseq.h"
#include "sndinfo.h"
#include "w_wad.h"
#include "wl_game.h"

#include <climits>

enum ESndSeqFlag
{
	SSF_NoStopCutOff = 0x1
};

// We used a fixed size instruction. What the instruction does is determined
// by what flags are set, so a single instruction can play a sound and delay
// for example.
enum ESndSeqInstruction
{
	SSI_PlaySound = 0x1,
	SSI_Delay = 0x2,
	SSI_End = 0x4,
	SSI_WaitForFinish = 0x8,
	SSI_Repeat = 0x10
};

SoundSequence::SoundSequence() : Flags(0)
{
}

void SoundSequence::AddInstruction(const SndSeqInstruction &instr)
{
	Instructions.Push(instr);
}

void SoundSequence::Clear()
{
	for(unsigned int i = 0;i < NUM_SEQ_TYPES;++i)
		AltSequences[i] = NAME_None;
	Instructions.Clear();
}

const SoundSequence &SoundSequence::GetSequence(SequenceType type) const
{
	if(AltSequences[type] == NAME_None)
		return *this;
	return SoundSeq(AltSequences[type], type);
}

void SoundSequence::SetFlag(unsigned int flag, bool set)
{
	if(set)
		Flags |= flag;
	else
		Flags &= ~flag;
}

void SoundSequence::SetSequence(SequenceType type, FName sequence)
{
	AltSequences[type] = sequence;
}

const SndSeqInstruction *SoundSequence::Start() const
{
	if(Instructions.Size() == 0)
		return NULL;
	return &Instructions[0];
}

//------------------------------------------------------------------------------

SndSeqTable SoundSeq;

void SndSeqTable::Init()
{
	Printf("S_Init: Reading SNDSEQ defintions.\n");

	int lastLump = 0;
	int lump = 0;
	while((lump = Wads.FindLump("SNDSEQ", &lastLump)) != -1)
	{
		ParseSoundSequence(lump);
	}
}

void SndSeqTable::ParseSoundSequence(int lumpnum)
{
	Scanner sc(lumpnum);

	while(sc.TokensLeft())
	{
		if(sc.CheckToken('['))
		{
			sc.MustGetToken(TK_Identifier);
			SoundSequence &seq = Sequences[sc->str];
			seq.Name = sc->str;
			seq.Clear();

			while(!sc.CheckToken(']'))
			{
				sc.MustGetToken(TK_IntConst);
				SequenceType type = static_cast<SequenceType>(sc->number);

				if(!sc.GetNextString())
					sc.ScriptMessage(Scanner::ERROR, "Expected logical sound name.");
				seq.SetSequence(type, sc->str);
			}
		}
		else
		{
			sc.MustGetToken(':');
			sc.MustGetToken(TK_Identifier);
			SoundSequence &seq = Sequences[sc->str];
			seq.Name = sc->str;
			seq.Clear();

			do
			{
				sc.MustGetToken(TK_Identifier);

				if(sc->str.CompareNoCase("end") == 0)
				{
					SndSeqInstruction instr = {SSI_End};
					seq.AddInstruction(instr);
					break;
				}
				else if(sc->str.CompareNoCase("delay") == 0)
				{
					SndSeqInstruction instr = {SSI_Delay};
					instr.ArgumentRand = 0;

					sc.MustGetToken(TK_IntConst);
					instr.Argument = sc->number;

					seq.AddInstruction(instr);
				}
				else if(sc->str.CompareNoCase("delayrand") == 0)
				{
					SndSeqInstruction instr = {SSI_Delay};

					sc.MustGetToken(TK_IntConst);
					instr.Argument = sc->number;

					sc.MustGetToken(TK_IntConst);
					instr.ArgumentRand = sc->number - instr.Argument;

					seq.AddInstruction(instr);
				}
				else if(sc->str.CompareNoCase("play") == 0)
				{
					SndSeqInstruction instr = {SSI_PlaySound};

					if(!sc.GetNextString())
						sc.ScriptMessage(Scanner::ERROR, "Expected logical sound name.");
					instr.Sound = sc->str;

					seq.AddInstruction(instr);
				}
				else if(sc->str.CompareNoCase("playrepeat") == 0)
				{
					SndSeqInstruction instr = {SSI_PlaySound|SSI_WaitForFinish|SSI_Repeat};

					if(!sc.GetNextString())
						sc.ScriptMessage(Scanner::ERROR, "Expected logical sound name.");
					instr.Sound = sc->str;

					seq.AddInstruction(instr);
				}
				else if(sc->str.CompareNoCase("nostopcutoff") == 0)
				{
					seq.SetFlag(SSF_NoStopCutOff, true);
				}
				else if(sc->str.CompareNoCase("stopsound") == 0)
				{
					if(!sc.GetNextString())
						sc.ScriptMessage(Scanner::ERROR, "Expected logical sound name.");
					seq.StopSound = sc->str;
				}
				else
				{
					sc.ScriptMessage(Scanner::ERROR, "Unknown sound sequence command '%s'.", sc->str.GetChars());
				}
			}
			while(sc.TokensLeft());
		}
	}
}

const SoundSequence &SndSeqTable::operator() (FName sequence, SequenceType type) const
{
	static const SoundSequence NullSequence;

	const SoundSequence *seq = Sequences.CheckKey(sequence);
	if(seq)
		return seq->GetSequence(type);
	return NullSequence;
}

//------------------------------------------------------------------------------

SndSeqPlayer::SndSeqPlayer(const SoundSequence &sequence, MapSpot Source) :
	Sequence(sequence), Source(Source), Delay(0), Playing(true), WaitForDone(false)
{
	Current = Sequence.Start();
	if(Current == NULL)
		Playing = false;
}

SndSeqPlayer::~SndSeqPlayer()
{
	if(Playing)
		Stop();
}

// SD_SoundPlaying() seems to intentionally be for adlib/pc speaker only. At
// least it has been like that since the beginning of ECWolf.
extern FString SoundPlaying;
void SndSeqPlayer::Tick()
{
	if(!Playing || (Delay != 0 && --Delay > 0))
		return;

	if(WaitForDone)
	{
		if(SoundPlaying.IsNotEmpty())
			return;
		else
			WaitForDone = false;
	}

	do
	{
		if(Current->Instruction & SSI_PlaySound)
		{
			PlaySoundLocMapSpot(Current->Sound, Source);
		}

		if(Current->Instruction & SSI_Delay)
		{
			Delay = Current->Argument + (Current->ArgumentRand ? (M_Random.GenRand32() % Current->ArgumentRand) : 0);
		}

		if(Current->Instruction & SSI_End)
		{
			Playing = false;
		}

		if(Current->Instruction & SSI_WaitForFinish)
		{
			WaitForDone = true;
			if(Delay == 0)
				Delay = 1;
		}

		if(!(Current->Instruction & SSI_Repeat))
		{
			++Current;
		}
	}
	while(Delay == 0 && Playing);
}

void SndSeqPlayer::Stop()
{
	Playing = false;

	// Unfortunately due to limitations of the sound code we can't determine
	// what sound is playing much less stop the sound.

	if(Sequence.GetStopSound() != NAME_None)
		PlaySoundLocMapSpot(Sequence.GetStopSound(), Source);
}

FArchive &operator<< (FArchive &arc, SndSeqPlayer *&seqplayer)
{
	FName seqname;
	MapSpot source;
	unsigned int offs;

	if(arc.IsStoring())
	{
		if(seqplayer == NULL)
		{
			// Can't do much here, so flag and move on.
			offs = UINT_MAX;
			arc << offs;
			return arc;
		}

		seqname = seqplayer->Sequence.GetSeqName();
		source = seqplayer->Source;
		offs = static_cast<unsigned int>(seqplayer->Current - seqplayer->Sequence.Start());
	}

	// First check that we actually stored a sequence player.
	arc << offs;
	if(arc.IsLoading() && offs == UINT_MAX)
	{
		delete seqplayer;
		seqplayer = NULL;
		return arc;
	}

	arc << seqname << source;

	if(arc.IsLoading())
	{
		// We don't need to worry about the sequence type here since seqname
		// should have already resolved to the proper sequence.
		delete seqplayer;
		seqplayer = new SndSeqPlayer(SoundSeq(seqname, SEQ_OpenNormal), source);
		seqplayer->Current += offs;
	}

	arc	<< seqplayer->Delay
		<< seqplayer->Playing
		<< seqplayer->WaitForDone;

	return arc;
}
