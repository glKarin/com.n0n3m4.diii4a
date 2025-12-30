// Music playback code from DOS port of Super 3-D Noah's Ark
// Originally recreated as a part of reverse-engineering the DOS executable

#ifdef USE_GPL
#include "dosbox/dbopl.h"
#else
#include "mame/fmopl.h"
#endif
#include "id_sd.h"
#include "m_swap.h"

#define MUSIC_RATE 700 // TODO: This should be shared with id_sd.cpp

bool N3DTempoEmulation;

typedef struct
{
	byte	mChar,cChar,
		mScale,cScale,
		mAttack,cAttack,
		mSustain,cSustain,
		mWave,cWave,
		mFeedConn;
} inst_t;

// This table maps channel numbers to carrier and modulator op cells
static byte  carriers[9] =  {3, 4, 5,11,12,13,19,20,21},
            modifiers[9] = { 0, 1, 2, 8, 9,10,16,17,18},
// This table maps percussive voice numbers to op cells
            pcarriers[5] = {19,0xff,0xff,0xff,0xff},
            pmodifiers[5] = {16,17,18,20,21};

volatile    bool   midiOn;
static volatile    int32_t midiError = 0;
static float       midiTimeScale = 1.86;
const byte        *midiData, *midiDataStart;
static byte        midiRunningStatus;
static longword    midiLength, midiDeltaTime;
static int32_t     midiDivision;

static longword
MIDI_VarLength(void)
{
	longword value = 0;
	while (*midiData & 0x80)
		value = (value << 7) + (*midiData++ & 0x7F);
	value = (value << 7) + *midiData++;
	return value;
}



static word	NoteTable[12] = {0x157,0x16b,0x181,0x198,0x1b0,0x1ca,0x1e5,0x202,0x220,0x241,0x263,0x287};

static byte	drums = 0;

static inst_t	instrument[14] = {
	{0x21, 0x31, 0x4f, 0x00, 0xf2, 0xd2, 0x52, 0x73, 0x00, 0x00, 0x06},
	{0x01, 0x31, 0x4f, 0x04, 0xf0, 0x90, 0xff, 0x0f, 0x00, 0x00, 0x06},
	{0x31, 0x22, 0x10, 0x04, 0x83, 0xf4, 0x9f, 0x78, 0x00, 0x00, 0x0a},
	{0x11, 0x31, 0x05, 0x00, 0xf9, 0xf1, 0x25, 0x34, 0x00, 0x00, 0x0a},
	{0x31, 0x61, 0x1c, 0x80, 0x41, 0x92, 0x0b, 0x3b, 0x00, 0x00, 0x0e},
	{0x21, 0x21, 0x19, 0x80, 0x43, 0x85, 0x8c, 0x2f, 0x00, 0x00, 0x0c},
	{0x21, 0x24, 0x94, 0x05, 0xf0, 0x90, 0x09, 0x0a, 0x00, 0x00, 0x0a},
	{0x21, 0xa2, 0x83, 0x8d, 0x74, 0x65, 0x17, 0x17, 0x00, 0x00, 0x07},
	{0x01, 0x01, 0x00, 0x00, 0xff, 0xff, 0x07, 0x07, 0x00, 0x00, 0x07},
	{0x10, 0x00, 0x00, 0x00, 0xd8, 0x87, 0x4a, 0x3c, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x11, 0x11, 0xfa, 0xfa, 0xb5, 0xb5, 0x00, 0x00, 0x00},
	{0x00, 0x00, 0x00, 0x00, 0xf8, 0xf8, 0x88, 0xb5, 0x00, 0x00, 0x00},
	{0x15, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01},
	{0x21, 0x11, 0x4c, 0x00, 0xf1, 0xf2, 0x63, 0x72, 0x00, 0x00, 0xc0}
};


#ifdef USE_GPL
extern DBOPL::Chip oplChip;
void YM3812Write(DBOPL::Chip &which, Bit32u reg, Bit8u val, const int &volume);
#else
extern const int oplChip;
#endif


void
MIDI_SkipMetaEvent(void)
{
	longword length = MIDI_VarLength();
	midiData += length;
}

void
MIDI_NoteOff(int channel, int note, int velocity)
{
	unsigned	fnumber;
	byte	octave;
	if (channel == 9)
	{
		switch (note)
		{
		case 0x23:
		case 0x24:
			drums &= 0xef;
			break;
		case 0x26:
		case 0x28:
			drums &= 0xf7;
			break;
		case 0x2a:
			drums &= 0xfe;
			break;
		default:
			midiError = -11;
		}
		alOutMusic(alEffects,alChar|drums);
		return;
	}

	fnumber = NoteTable[note%12];
	octave = ((note/12)&7)<<2;
	alOutMusic(alFreqL + 1 + channel, fnumber&0xFF);
	alOutMusic(alFreqH + 1 + channel, octave+((fnumber>>8)&3));
}

void
MIDI_NoteOn(int channel, byte note, byte velocity)
{
	unsigned	fnumber;
	int	octave;
	if (velocity)
	{
		if (channel == 9)
		{
			switch (note)
			{
			case 0x23:
			case 0x24:
				drums |= 0x10;
				break;
			case 0x26:
				drums |= 0x8;
				break;
			case 0x28:
				drums |= 0x4;
				break;
			case 0x2a:
				drums |= 1;
				break;
			default:
				midiError = -11;
			}
			alOutMusic(alEffects,alChar|drums);
		}
		else
		{
			fnumber = NoteTable[note%12];
			octave = note/12;
			alOutMusic(alFreqL + 1 + channel, fnumber&0xFF);
			alOutMusic(alFreqH + 1 + channel, alChar|(octave<<2)|((fnumber>>8)&3));
		}
	}
	else
		MIDI_NoteOff (channel,note,velocity);
}

#if 0
void
MIDI_ControllerChange(int channel, int id, int value)
{
}
#endif

void
MIDI_ProgramChange(int channel, int id)
{
	// S3DNA RESTORATION - While an inst_t pointer can be used with direct
	// access to all fields, based on generated machine code it looks like
	// this wasn't the way the code was written
	byte	*inst;
	if (channel == 9)
	{
		int	note;
		unsigned	fnumber;
		int	octave;

		inst = (byte *)&instrument[9];
		alOutMusic(modifiers[6]+alChar, *inst++);
		alOutMusic(carriers[6]+alChar, *inst++);
		alOutMusic(modifiers[6]+alScale, *inst++);
		alOutMusic(carriers[6]+alScale, *inst++);
		alOutMusic(modifiers[6]+alAttack, *inst++);
		alOutMusic(carriers[6]+alAttack, *inst++);
		alOutMusic(modifiers[6]+alSus, *inst++);
		alOutMusic(carriers[6]+alSus, *inst++);
		alOutMusic(modifiers[6]+alWave, *inst++);
		alOutMusic(carriers[6]+alWave, *inst++);

		alOutMusic(alFeedCon+6, *inst);

		note = 24;
		fnumber = NoteTable[note%12];
		octave = ((note/12)&7)<<2;
		alOutMusic(alFreqL+6,fnumber&0xFF);
		alOutMusic(alFreqH+6,octave+((fnumber>>8)&3));
		note = 24;
		fnumber = NoteTable[note%12];
		octave = ((note/12)&7)<<2;
		alOutMusic(alFreqL+7,fnumber&0xFF);
		alOutMusic(alFreqH+7,octave+((fnumber>>8)&3));
		note = 24;
		fnumber = NoteTable[note%12];
		octave = ((note/12)&7)<<2;
		alOutMusic(alFreqL+8,fnumber&0xFF);
		alOutMusic(alFreqH+8,octave+((fnumber>>8)&3));

		inst = (byte *)&instrument[10];
		alOutMusic(0x31,*inst); inst += 2;
		alOutMusic(0x51,*inst); inst += 2;
		alOutMusic(0x71,*inst); inst += 2;
		alOutMusic(0x91,*inst); inst += 2;

		alOutMusic(0xF1,*inst);
		alOutMusic(0xC7,0);

		inst = (byte *)&instrument[12];
		alOutMusic(0x32,*inst); inst += 2;
		alOutMusic(0x52,*inst); inst += 2;
		alOutMusic(0x72,*inst); inst += 2;
		alOutMusic(0x92,*inst); inst += 2;

		alOutMusic(0xF2,*inst);

		inst = (byte *)&instrument[11];
		alOutMusic(0x34,*inst); inst += 2;
		alOutMusic(0x54,*inst); inst += 2;
		alOutMusic(0x74,*inst); inst += 2;
		alOutMusic(0x94,*inst); inst += 2;

		alOutMusic(0xF4,*inst);
		alOutMusic(0xC8,0);

		inst = (byte *)&instrument[10];
		alOutMusic(0x35,*inst); inst += 2;
		alOutMusic(0x55,*inst); inst += 2;
		alOutMusic(0x75,*inst); inst += 2;
		alOutMusic(0x95,*inst); inst += 2;

		alOutMusic(0xF5,*inst);

		return;
	}

	if (channel < 5)
	{
		switch (id & 0xF8)
		{
		case 0:
			inst = (byte *)&instrument[0];
			break;
		case 8:
			inst = (byte *)&instrument[8];
			break;
		case 16:
			inst = (byte *)&instrument[1];
			break;
		case 24:
			inst = (byte *)&instrument[0];
			break;
		case 32:
			inst = (byte *)&instrument[2];
			break;
		case 40:
		case 48:
			inst = (byte *)&instrument[0];
			break;
		case 56:
		case 64:
			inst = (byte *)&instrument[6];
			break;
		case 72:
			inst = (byte *)&instrument[7];
			break;
		case 80:
		case 88:
		case 96:
			inst = (byte *)&instrument[0];
			break;
		case 104:
		case 112:
		case 120:
			inst = (byte *)&instrument[8];
			break;
		default:
			midiError = -8;
			return;
		}

		alOutMusic(modifiers[channel+1]+alChar, *inst++);
		alOutMusic(carriers[channel+1]+alChar, *inst++);
		alOutMusic(modifiers[channel+1]+alScale, *inst++);
		alOutMusic(carriers[channel+1]+alScale, *inst++);
		alOutMusic(modifiers[channel+1]+alAttack, *inst++);
		alOutMusic(carriers[channel+1]+alAttack, *inst++);
		alOutMusic(modifiers[channel+1]+alSus, *inst++);
		alOutMusic(carriers[channel+1]+alSus, *inst++);
		alOutMusic(modifiers[channel+1]+alWave, *inst++);
		alOutMusic(carriers[channel+1]+alWave, *inst++);

		alOutMusic(alFeedCon+channel, *inst);
	}
}

#if 0
void
MIDI_ChannelPressure(int channel, int id)
{
}
#endif

void
MIDI_ProcessEvent(byte event)
{
	byte	note,velocity,id,value;
	switch (event&0xF0)
	{
	case 0x80:
		note = *midiData++;
		velocity = *midiData++;
		MIDI_NoteOff(event&0xF,note,velocity);
		break;
	case 0x90:
		note = *midiData++;
		velocity = *midiData++;
		MIDI_NoteOn(event&0xF,note,velocity);
		break;
	case 0xB0:
		id = *midiData++;
		value = *midiData++;
//		MIDI_ControllerChange(event&0xF,id,value);
		break;
	case 0xC0:
		value = *midiData++;
		MIDI_ProgramChange(event&0xF,value);
		break;
	case 0xD0:
		value = *midiData++;
//		MIDI_ChannelPressure(event&0xF,value);
		break;
	default:
		midiError = -7;
		break;
	}
}

static void
MIDI_DoEvent(void)
{
	byte	event;
	longword	length;
	longword	tempo;

	event = *midiData++;
	if (!(event & 0x80))
	{
		if (!(midiRunningStatus | 0x00))
			return;

		midiData--;
		MIDI_ProcessEvent(midiRunningStatus);
	}
	else if (event < 0xF0)
	{
		midiRunningStatus = event;
		MIDI_ProcessEvent(midiRunningStatus);
	}
	else if (event == 0xF0)
	{
		midiRunningStatus = 0;
		midiError = -4;
	}
	else if (event == 0xF7)
	{
		midiRunningStatus = 0;
		midiError = -5;
	}
	else if (event == 0xFF)
	{
		midiRunningStatus = 0;
		event = *midiData++;
		switch (event)
		{
		case 0x51:
			length = MIDI_VarLength();
			if (N3DTempoEmulation)
			{
				tempo = ((int32_t)(*midiData)<<16) + (int32_t)(int16_t)((*(midiData+1))<<8) + (*(midiData+2));
				midiTimeScale = (double)tempo/2.74176e5;
				midiTimeScale *= 1.1;
			}
			else
			{
				tempo = ((int32_t)(*midiData)<<16) + (int32_t)((*(midiData+1))<<8) + (*(midiData+2));
				midiTimeScale = (double)tempo*MUSIC_RATE/(1000000*midiDivision);
			}
			midiData += length;
			break;
		case 0x2F:
			midiData = midiDataStart;
			midiDeltaTime = 0;
			break;
		default:
			MIDI_SkipMetaEvent();
			break;
		}
	}
	else
		midiError = -6;
}

void
MIDI_IRQService(void)
{
	int	maxevent = 0;

	if (!midiOn)
		return;

	if (midiDeltaTime)
	{
		midiDeltaTime--;
		return;
	}

	while (!midiDeltaTime && (maxevent++ < 32))
	{
		MIDI_DoEvent();
		midiDeltaTime = MIDI_VarLength();
	}

	if (maxevent >= 32)
		midiError = -1;
	else if (midiDeltaTime & 0xFFFF0000)
	{
		midiError = -2;
		return;
	}

	midiDeltaTime = midiDeltaTime * midiTimeScale;
}

// MIDI startup code

bool
MIDI_TryToStart(const byte *seqPtr, int dataLen)
{
    if (dataLen < 10)
        return false;

    if (strncmp((const char *)seqPtr, "MThd", 4) ||
        ReadBigShort(seqPtr + 8) ||
        (ReadBigShort(seqPtr + 10) != 1))
        return false;

    midiDivision = ReadBigShort(seqPtr + 12);
    if (!N3DTempoEmulation && (midiDivision <= 0))
        return false;

    seqPtr += ReadBigLong(seqPtr + 4) + 8;
    if (strncmp((const char *)seqPtr, "MTrk", 4))
        return false;

    int32_t seqLength = ReadBigLong(seqPtr + 4);
    if (!seqLength)
        return false;

    seqPtr += 8;
    midiData = seqPtr;
    midiDataStart = seqPtr;
    midiLength = seqLength;
    midiDeltaTime = 0;
    midiDeltaTime = MIDI_VarLength();
    if (midiDeltaTime & 0xFFFF0000)
        return false;

    if (MusicMode == smm_Midi)
        return false;

    midiRunningStatus = 0;
    MIDI_ProgramChange(9,0);
    alOutMusic(alEffects, alChar);
    drums = 0;

    midiOn = true;
    return true;
}
