//
//      ID Engine
//      ID_SD.c - Sound Manager for Wolfenstein 3D
//      v1.2
//      By Jason Blochowiak
//

//
//      This module handles dealing with generating sound on the appropriate
//              hardware
//
//      Depends on: User Mgr (for parm checking)
//
//      Globals:
//              For User Mgr:
//                      SoundBlasterPresent - SoundBlaster card present?
//                      AdLibPresent - AdLib card present?
//                      SoundMode - What device is used for sound effects
//                              (Use SM_SetSoundMode() to set)
//                      MusicMode - What device is used for music
//                              (Use SM_SetMusicMode() to set)
//                      DigiMode - What device is used for digitized sound effects
//                              (Use SM_SetDigiDevice() to set)
//
//              For Cache Mgr:
//                      NeedsDigitized - load digitized sounds?
//                      NeedsMusic - load music?
//
#include "wl_def.h"
#include <SDL_mixer.h>
#include "w_wad.h"
#include "zstring.h"
#include "sndinfo.h"
#include "sndseq.h"
#ifdef USE_GPL
#include "dosbox/dbopl.h"
#else
#include "mame/fmopl.h"
#endif
#include "wl_main.h"
#include "id_sd.h"
#include <SDL.h>

#ifndef ECWOLF_MIXER
#pragma message "Not using customized SDL_mixer. Features will be disabled. https://bitbucket.org/ecwolf/sdl_mixer-for-ecwolf"
#endif

// For AdLib sounds & music:
#define MUSIC_RATE 700	// Must be a multiple of SOUND_RATE
#define SOUND_RATE 140	// Also affects PC Speaker sounds
#define SOUND_TICKS (MUSIC_RATE/SOUND_RATE)

// Linear falloff
//#define TO_SDL_POSITION(pos) (((15 - (pos)) << 4) + 15)
// Volume is based on the square of the distance to the source. (More like vanilla)
#define TO_SDL_POSITION(pos) (((64 - ((pos) * (pos))) * 3) + 63)

#define MIN_TICKS_BETWEEN_DIGI_REPEATS 10

// Mutex for thread-safe audio:
SDL_mutex *audioMutex;

globalsoundpos channelSoundPos[MIX_CHANNELS];

//      Global variables
bool	AdLibPresent,
		SoundBlasterPresent,SBProPresent,
		SoundPositioned;
SDMode	SoundMode;
SMMode	MusicMode;
SDSMode	DigiMode;
int		AdlibVolume=MAX_VOLUME;
int		MusicVolume=MAX_VOLUME;
int		SoundVolume=MAX_VOLUME;

// SDL_mixer values from Mix_QuerySpec
static struct
{
	int frequency;
	Uint16 format;
	int channels;
} AudioSpec;
static SDL_AudioCVT AudioCVTStereo;

//      Internal variables
static  bool					SD_Started;
static  bool					nextsoundpos;
FString                 SoundPlaying;
static  word                    SoundPriority;
static  word                    DigiPriority;
static  int                     LeftPosition;
static  int                     RightPosition;

static  bool					DigiPlaying;

static int samplesPerMusicTick;

//      PC Sound variables
static  volatile byte           pcLastSample;
static  byte * volatile         pcSound;
static  longword                pcLengthLeft;

//      AdLib variables
static  byte * volatile         alSound;
static  byte                    alBlock;
static  longword                alLengthLeft;
static  longword                alTimeCount;
static  Instrument              alZeroInst;

//      Sequencer variables
static  volatile bool			sqActive;
static  word                   *sqHack;
static	TUniquePtr<word[]>		sqHackFreeable;
static  word                   *sqHackPtr;
static  int                     sqHackLen;
static  int                     sqHackSeqLen;
static  longword                sqHackTime;


//	Noah3D MIDI playback variables and functions
extern  volatile bool			midiOn;
extern	const byte			*midiData, *midiDataStart;
static	TUniquePtr<const byte[]>	midiChunkFreeable;

void MIDI_IRQService(void);
bool MIDI_TryToStart(const byte *seqPtr, int dataLen);


static int musicchunk=-1;
Mix_Music *music=NULL;
TUniquePtr<byte[]> chunkmem;

void musicFinished(void)
{
	if (music != NULL)
	{
		Mix_HaltMusic();
		Mix_FreeMusic(music);
		music = NULL;

		chunkmem.Reset();

		musicchunk = -1;
	}
}

bool SD_UpdateMusicVolume(int which)
{
	Mix_VolumeMusic(static_cast<int> (ceil(128.0*MULTIPLY_VOLUME(MusicVolume))));
	return 0;
}

#ifdef USE_GPL

DBOPL::Chip oplChip;

static inline bool YM3812Init(int numChips, int clock, int rate)
{
	oplChip.Setup(rate);
	return false;
}

void YM3812Write(DBOPL::Chip &which, Bit32u reg, Bit8u val, const int &volume)
{
	which.SetVolume(volume);
	which.WriteReg(reg, val);
}

static inline void YM3812UpdateOne(DBOPL::Chip &which, int16_t *stream, int length)
{
	Bit32s buffer[512 * 2];
	int i;

	// length is at maximum samplesPerMusicTick = param_samplerate / 700
	// so 512 is sufficient for a sample rate of 358.4 kHz (default 44.1 kHz)
	if(length > 512)
		length = 512;

	if(which.opl3Active)
	{
		which.GenerateBlock3(length, buffer);

		// GenerateBlock3 generates a number of "length" 32-bit stereo samples
		// so we only need to convert them to 16-bit samples
		for(i = 0; i < length * 2; i++)  // * 2 for left/right channel
		{
			// Multiply by 4 to match loudness of MAME emulator.
			Bit32s sample = buffer[i] << 2;
			if(sample > 32767) sample = 32767;
			else if(sample < -32768) sample = -32768;
			stream[i] = LittleShort(sample);
		}
	}
	else
	{
		which.GenerateBlock2(length, buffer);

		// GenerateBlock3 generates a number of "length" 32-bit mono samples
		// so we need to convert them to 32-bit stereo samples
		for(i = 0; i < length; i++)
		{
			// Multiply by 4 to match loudness of MAME emulator.
			// Then upconvert to stereo.
			Bit32s sample = buffer[i] << 2;
			if(sample > 32767) sample = 32767;
			else if(sample < -32768) sample = -32768;
			stream[i * 2] = stream[i * 2 + 1] = (int16_t) LittleShort(sample);
		}
	}
}

#else

extern const int oplChip = 0;

#endif

#ifndef ECWOLF_MIXER
static int Mix_SetMusicPCMPosition(Uint64 position) { return 0; }
static Uint64 Mix_GetMusicPCMPosition() { return 0; }
#endif

static void SDL_SoundFinished(void)
{
	SoundPlaying = FString();
	SoundPriority = 0;
}

/*
=============================================================================

                     PC SPEAKER EMULATOR -- by K1n9_Duk3

-----------------------------------------------------------------------------

This emulator was designed to be FAST! The sampled audio data created by this
code might not be 100% true to the output of a real PC Speaker, but it is
close enough.

The emulator generates a square wave:
     _____       _____       _____       _____       _____       _____
  __|     |     |     |     |     |     |     |     |     |     |     |___
          |_____|     |_____|     |_____|     |_____|     |_____|

=============================================================================
*/

static short	pcVolume = 5000;	// for 16-bit mixing (8-bit should use 20)
static longword	pcPhaseTick = 0;
static longword	pcPhaseLength;
static bool		pcActive = false;
static longword	pcSamplesPerTick;
static longword	pcNumReadySamples = 0;

#define PC_BASE_TIMER 1193181

// Function prototype is for menu listener
bool SD_UpdatePCSpeakerVolume(int)
{
	SDL_LockMutex(audioMutex);

	if(pcVolume > 0)
		pcVolume = AdlibVolume*250;
	else
		pcVolume = -AdlibVolume*250;

	SDL_UnlockMutex(audioMutex);

	return true;
}

// Note: The inline functions do not lock or unlock 'audioMutex'. Make sure
// that the code calling these functions locks/unlocks the mutex before/after
// calling these functions!

inline void _SDL_turnOnPCSpeaker(byte pcSample)
{
	// Note: You could use a lookup table to make this even faster. Only
	// 256 table entries are required since the PC samples are just byte
	// values.
	pcPhaseLength = (pcSample*60*AudioSpec.frequency)/(2*PC_BASE_TIMER);
#ifdef PC_VIBRATO
	//if(pcVolume<0) pcVolume = -pcVolume;
	pcPhaseTick = 0;
#endif
	pcActive = true;
}

inline void _SDL_turnOffPCSpeaker()
{
	pcActive = false;
	pcPhaseTick = 0;	// Only required in case PC_VIBRATO is not defined
}

inline void _SDL_PCService()
{
	if(pcSound)
	{
		if(*pcSound!=pcLastSample)
		{
			pcLastSample=*pcSound;
			if(pcLastSample)
				_SDL_turnOnPCSpeaker(pcLastSample);	// don't multiply by 60, just pass the byte value
			else
				_SDL_turnOffPCSpeaker();
		}
		pcSound++;
		pcLengthLeft--;
		if(!pcLengthLeft)
		{
			pcSound=0;
			SoundPriority=0;
			_SDL_turnOffPCSpeaker();
		}
	}
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_PCPlaySound() - Plays the specified sound on the PC speaker
//
///////////////////////////////////////////////////////////////////////////
static void
_SDL_PCPlaySound(PCSound *sound)
{
	// We must stop the digitized sound because we're just faking the digitized PC Speaker playback
	if(DigiMode == sds_PC)
		SD_StopDigitized();

	SDL_LockMutex(audioMutex);

	pcPhaseTick = 0;
	pcLastSample = 0;	// Must be a value that cannot be played, so the PC Speaker is forced to reset (-1 wraps to 255 so it cannot be used here)
    pcLengthLeft = LittleLong(sound->common.length);
    pcSound = sound->data;

	SDL_UnlockMutex(audioMutex);
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_PCStopSound() - Stops the current sound playing on the PC Speaker
//
///////////////////////////////////////////////////////////////////////////
static void
_SDL_PCStopSound(void)
{
	SDL_LockMutex(audioMutex);

    pcSound = 0;
	_SDL_turnOffPCSpeaker();

	SDL_UnlockMutex(audioMutex);
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_ShutPC() - Turns off the pc speaker
//
///////////////////////////////////////////////////////////////////////////
static void
_SDL_ShutPC(void)
{
    _SDL_PCStopSound();
}


///////////////////////////////////////////////////////////////////////////
//
//      SDL_EmulateAndMixPC() - Emulates the pc speaker
//                              and mixes the output into buffer
//
///////////////////////////////////////////////////////////////////////////
void _SDL_EmulateAndMixPC(Sint16 *buffer, int length)
{
	// This should be called in SDL_IMFMusicPlayer() directly after calling
	// YM3812UpdateOne() so that the PC sounds can be mixed into the output
	// of the OPL emulator. This way, all sound hardware is emulated in the
	// same routine. The audio gets mixed into the Music channel, so we do
	// not need an additional channel for the PC Speaker sounds.

	// Note: This code assumes that 'buffer' is a signed 16-bit stereo sound!

	Sint32 mix;	// Needs more bits than the buffer

	if(!pcActive) return; // PC Speaker is turned off

	SDL_LockMutex(audioMutex);

	while(length--)
	{
		mix = *buffer;

		// Mix it by simply adding the volume:
		mix += pcVolume;
		if (mix < -32768) mix = -32768;
		else if (mix >  32767) mix =  32767;
		// This generates pretty much the same output as SDL_MixAudio(), but
		// it does not require a second buffer for the PC Speaker sample data.
		//
		// Note: If you use another mixing method, you cannot simply return
		// from this function if pcActive is false. You will have to mix a
		// PC Volume of 0 into the entire buffer!

		// We assume that the left and right channel in the buffer contain
		// the same value, so we only need to calculate the mix once.
		*buffer++ = mix;	//left channel
		*buffer++ = mix;	//right channel

		// Update the PC speaker state:
		if(pcPhaseTick++ >= pcPhaseLength)
		{
			pcVolume = -pcVolume;
			pcPhaseTick = 0;
		}
	}

	SDL_UnlockMutex(audioMutex);
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_PCSpeakerEmulator() - Emulates the pc speaker
//      (replaces SDL_IMFMusicPlayer if no AdLib emulator is present)
//
///////////////////////////////////////////////////////////////////////////
void _SDL_PCSpeakerEmulator(void *udata, Uint8 *stream, int len)
{
    int stereolen = len>>1;
    int sampleslen = stereolen>>1;
    Sint16 *stream16 = (Sint16 *) (void *) stream;    // expect correct alignment

	SDL_LockMutex(audioMutex);

	while(1)
    {
        if(pcNumReadySamples)
        {
			if(pcActive)
				while(pcNumReadySamples && sampleslen)
				{
					pcNumReadySamples--;
					sampleslen--;

					*stream16++ = pcVolume;
					*stream16++ = pcVolume;

					if(pcPhaseTick++ >= pcPhaseLength)
					{
						pcVolume = -pcVolume;
						pcPhaseTick = 0;
					}
				}
			else
				while(pcNumReadySamples && sampleslen)
				{
					pcNumReadySamples--;
					sampleslen--;

					stream16 += 2;	// No need to set it to 0. SDL should have done that already.
				}

			if(!sampleslen)
				break;	// We need to unlock the mutex, so we cannot just return!
        }

		_SDL_PCService();

        pcNumReadySamples = pcSamplesPerTick;

    }

	SDL_UnlockMutex(audioMutex);
}
/*
=============================================================================
======================== End of PC Speaker emulator ========================
=============================================================================
*/
#define SDL_PCPlaySound(pcsound)				_SDL_PCPlaySound(pcsound)
#define SDL_PCStopSound()						_SDL_PCStopSound()
#define SDL_ShutPC()							_SDL_ShutPC()
#define SDL_PCService()							_SDL_PCService()
#define SDL_PCEmulateAndMix(buffer, length)		if(SoundMode == sdm_PC) _SDL_EmulateAndMixPC(buffer, length)

void SD_StopDigitized(void)
{
	DigiPlaying = false;
	DigiPriority = 0;
	SoundPositioned = false;
	if ((DigiMode == sds_PC) && (SoundMode == sdm_PC))
		SDL_SoundFinished();

	Mix_HaltChannel(-1);
}

void SD_SetPosition(int channel, int leftpos, int rightpos)
{
	if((leftpos < 0) || (leftpos > 15) || (rightpos < 0) || (rightpos > 15)
			|| ((leftpos == 15) && (rightpos == 15)))
		I_FatalError("SD_SetPosition: Illegal position");

	switch (DigiMode)
	{
		default:
			break;
		case sds_SoundBlaster:
//            SDL_PositionSBP(leftpos,rightpos);
			Mix_SetPanning(channel, TO_SDL_POSITION(leftpos), TO_SDL_POSITION(rightpos));
			break;
	}
}

// Mac format sound loading.
struct MacSoundData
{
	uint8_t *data;
	Sint64 pos;
	int size;
};
static Sint64 MacSound_Size(SDL_RWops *ops)
{
	return (Sint64)((MacSoundData*)ops->hidden.unknown.data1)->size;
}
#if SDL_VERSION_ATLEAST(2,0,0)
static Sint64 MacSound_Seek(SDL_RWops *ops, Sint64 pos, int relative)
#else
static int MacSound_Seek(SDL_RWops *ops, int pos, int relative)
#endif
{
	Sint64 &curpos = ((MacSoundData*)ops->hidden.unknown.data1)->pos;
	switch(relative)
	{
		case RW_SEEK_SET:
			curpos = pos;
			break;
		case RW_SEEK_CUR:
			curpos += pos;
			break;
		case RW_SEEK_END:
			curpos = ((MacSoundData*)ops->hidden.unknown.data1)->size+pos;
			break;
	}
	return (int)curpos;
}
#if SDL_VERSION_ATLEAST(2,0,0)
static size_t MacSound_Read(SDL_RWops *ops, void *buffer, size_t size, size_t nmem)
#else
static int MacSound_Read(SDL_RWops *ops, void *buffer, int size, int nmem)
#endif
{
	static const char WAV_HEADER[40] = {
		'R','I','F','F',0,0,0,0,'W','A','V','E',
		'f','m','t',' ',16,0,0,0,1,0,1,0,
		0x22,0x56,0,0,0x22,0x56,0,0,1,0,8,0,
		'd','a','t','a'
	};
	static const unsigned int MacSoundHeaderSize = 0x2A;

	size_t totalsize = size*nmem;
	DWORD ssize = (DWORD)(MacSound_Size(ops)-MacSoundHeaderSize);
	Sint64 &pos = ((MacSoundData*)ops->hidden.unknown.data1)->pos;
	if(pos < (Sint64)sizeof(WAV_HEADER))
	{
		size_t copysize = MIN<size_t>(totalsize, sizeof(WAV_HEADER)-pos);
		memcpy(buffer, WAV_HEADER+pos, copysize);
		pos += copysize;
		buffer = ((char*)buffer)+copysize;
		totalsize -= copysize;
		if(totalsize == 0)
			return nmem;
	}
	if(pos < (Sint64)sizeof(WAV_HEADER)+4)
	{
		size_t copysize = MIN<size_t>(totalsize, sizeof(ssize)+sizeof(WAV_HEADER)-pos);
		memcpy(buffer, (char*)(&ssize)+(pos-sizeof(WAV_HEADER)), copysize);
		pos += copysize;
		buffer = ((char*)buffer)+copysize;
		totalsize -= copysize;
		if(totalsize == 0)
			return nmem;
	}

	size_t copysize = MIN<size_t>(totalsize, ssize-(pos-sizeof(WAV_HEADER)-4));
	memcpy(buffer, ((MacSoundData*)ops->hidden.unknown.data1)->data+pos-sizeof(WAV_HEADER)-4, copysize);

	// Mac sound data is signed, we need unsigned
	unsigned char* pcm = (unsigned char*)buffer;
	for(size_t i = copysize;i-- > 0;)
		*pcm++ += 0x80;

	return (int)(copysize/size);
}
static int MacSound_Close(SDL_RWops *ops)
{
	free(((MacSoundData*)ops->hidden.unknown.data1)->data);
	free(ops->hidden.unknown.data1);
	return 0;
}

Mix_Chunk* SD_PrepareSound(int which)
{
	int size = Wads.LumpLength(which);
	if(size == 0)
		return NULL;

	FMemLump soundLump = Wads.ReadLump(which);

	// 0x2A is the size of the sound header. From what I can tell the csnds
	// have mostly garbage filled headers (outside of what is precisely needed
	// since the sample rate is hard coded). I'm not sure if the sounds are
	// 8-bit or 16-bit, but it looks like the sample rate is coded to ~22050.
	if(BigShort(*(WORD*)soundLump.GetMem()) == 1 && size > 0x2A)
	{
		SDL_RWops *ops = SDL_AllocRW();
		//ops->size = MacSound_Size;
		ops->seek = MacSound_Seek;
		ops->read = MacSound_Read;
		ops->write = NULL;
		ops->close = MacSound_Close;
		ops->type = 0;
		ops->hidden.unknown.data1 = malloc(sizeof(MacSoundData));
		((MacSoundData*)ops->hidden.unknown.data1)->data = (uint8_t*)malloc(size-0x2A);
		((MacSoundData*)ops->hidden.unknown.data1)->size = size-0x2A;
		((MacSoundData*)ops->hidden.unknown.data1)->pos = 0;
		memcpy(((MacSoundData*)ops->hidden.unknown.data1)->data, ((char*)soundLump.GetMem())+0x2A, size-0x2A);
		for(unsigned int i = size-0x2A;i-- > 0;)
			((MacSoundData*)ops->hidden.unknown.data1)->data[i] = 0x80+((MacSoundData*)ops->hidden.unknown.data1)->data[i];
		return Mix_LoadWAV_RW(ops, 1);
	}

	return Mix_LoadWAV_RW(SDL_RWFromMem(soundLump.GetMem(), size), 1);
}

int SD_PlayDigitized(const SoundData &which,int leftpos,int rightpos,SoundChannel chan)
{
	if (!DigiMode)
		return 0;

	// If this sound has been played too recently, don't play it again.
	// (Fix for extremely loud sounds when one plays over itself too much.)
	uint32_t currentTick = SDL_GetTicks();
	if (currentTick - SoundInfo.GetLastPlayTick(which) < MIN_TICKS_BETWEEN_DIGI_REPEATS)
		return 0;

	SoundInfo.SetLastPlayTick(which, currentTick);

	int channel = chan;
	if(chan == SD_GENERIC)
	{
		channel = Mix_GroupAvailable(1);
		if(channel == -1) channel = Mix_GroupOldest(1);
		if(channel == -1)           // All sounds stopped in the meantime?
			channel = Mix_GroupAvailable(1);
	}
	SD_SetPosition(channel, leftpos,rightpos);

	DigiPlaying = true;

	Mix_Chunk *sample = which.GetDigitalData();
	if(sample == NULL)
		return 0;

	Mix_Volume(channel, static_cast<int> (ceil(128.0*MULTIPLY_VOLUME(SoundVolume))));
	if(Mix_PlayChannel(channel, sample, 0) == -1)
	{
		printf("Unable to play sound: %s\n", Mix_GetError());
		return 0;
	}

	// Return channel + 1 because zero is a valid channel.
	return channel + 1;
}

void SD_ChannelFinished(int channel)
{
	SoundPlaying = FString();
	channelSoundPos[channel].valid = 0;
}

void
SD_SetDigiDevice(SDSMode mode)
{
	bool devicenotpresent;

	if (mode == DigiMode)
		return;

	SD_StopDigitized();

	devicenotpresent = false;
	switch (mode)
	{
		default:
			break;
		case sds_SoundBlaster:
			if (!SoundBlasterPresent)
				devicenotpresent = true;
			break;
	}

	if (!devicenotpresent)
	{
		DigiMode = mode;

#ifdef NOTYET
		SDL_SetTimerSpeed();
#endif
	}
}

//      AdLib Code

///////////////////////////////////////////////////////////////////////////
//
//      SDL_ALStopSound() - Turns off any sound effects playing through the
//              AdLib card
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_ALStopSound(void)
{
	SDL_LockMutex(audioMutex);

	alSound = 0;
	alOut(alFreqH + 0, 0);

	SDL_UnlockMutex(audioMutex);
}

static void SDL_AlSetChanInst(const Instrument *inst, unsigned int chan)
{
	static const byte chanOps[OPL_CHANNELS] = {
		0, 1, 2, 8, 9, 0xA, 0x10, 0x11, 0x12
	};
	byte c,m;

	m = chanOps[chan]; // modulator cell for channel
	c = m + 3; // carrier cell for channel
	alOut(m + alChar,inst->mChar);
	alOut(m + alScale,inst->mScale);
	alOut(m + alAttack,inst->mAttack);
	alOut(m + alSus,inst->mSus);
	alOut(m + alWave,inst->mWave);
	alOut(c + alChar,inst->cChar);
	alOut(c + alScale,inst->cScale);
	alOut(c + alAttack,inst->cAttack);
	alOut(c + alSus,inst->cSus);
	alOut(c + alWave,inst->cWave);

	// Note: Switch commenting on these lines for old MUSE compatibility
//    alOutInIRQ(alFeedCon,inst->nConn);

	alOut(chan + alFreqL,0);
	alOut(chan + alFreqH,0);
	alOut(chan + alFeedCon,0);
}
static void SDL_AlSetFXInst(const Instrument *inst)
{
	SDL_AlSetChanInst(inst, 0);
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_ALPlaySound() - Plays the specified sound on the AdLib card
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_ALPlaySound(AdLibSound *sound)
{
	Instrument      *inst;
	byte            *data;

	SDL_ALStopSound();

	SDL_LockMutex(audioMutex);

	alLengthLeft = LittleLong(sound->common.length);
	data = sound->data;
	alBlock = ((sound->block & 7) << 2) | 0x20;
	inst = &sound->inst;

	if (!(inst->mSus | inst->cSus))
	{
		I_FatalError("SDL_ALPlaySound() - Bad instrument");
	}

	SDL_AlSetFXInst(inst);
	alSound = (byte *)data;

	SDL_UnlockMutex(audioMutex);
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_ShutAL() - Shuts down the AdLib card for sound effects
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_ShutAL(void)
{
	SDL_LockMutex(audioMutex);

	alSound = 0;
	//alOut(alEffects,0);	// Sound effects should not mess with the music's rhythm settings!
	alOut(alFreqH + 0,0);
	SDL_AlSetFXInst(&alZeroInst);

	SDL_UnlockMutex(audioMutex);
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_StartAL() - Starts up the AdLib card for sound effects
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_StartAL(void)
{
	//alOut(alEffects, 0);	// Sound effects should not mess with the music's rhythm settings!
	SDL_AlSetFXInst(&alZeroInst);
}

////////////////////////////////////////////////////////////////////////////
//
//      SDL_ShutDevice() - turns off whatever device was being used for sound fx
//
////////////////////////////////////////////////////////////////////////////
static void
SDL_ShutDevice(void)
{
	switch (SoundMode)
	{
		default:
			break;
		case sdm_PC:
			SDL_ShutPC();
			break;
		case sdm_AdLib:
			SDL_ShutAL();
			break;
	}
	SoundMode = sdm_Off;
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_StartDevice() - turns on whatever device is to be used for sound fx
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_StartDevice(void)
{
	switch (SoundMode)
	{
		default:
			break;
		case sdm_AdLib:
			SDL_StartAL();
			break;
	}
	SoundPlaying = FString();
	SoundPriority = 0;
}

/*-----------------------------------------------------------------------------
The variables below are not required unless you WANT to change the behavior of
AdLib sound effects compared to the original Wolfenstein 3-D code.

What would happen is this: SDL_AlPlaySound() resets the AdLib instrument and
the alSound pointer to play the given AdLib sound from the beginning. The check
that was implemented in SDL_IMFMusicPlayer() would not reset the curAlSoundPtr
if the alSound pointer pointed to the data that was already being played. This
resulted in situations where the instrument data was reset, but not the data
pointer. This caused some sounds to be played at the wrong pitch.

If you really WANT to set the behavior so that an AdLib sound will not be re-
started when it is interrupted by itself, you should change the code in the
SDL_AlPlaySound() function instead, making sure not to reset the instrument.

Any thread-safety issues should be solved now. The mutex 'audioMutex' protects
gloabal variables that need to be accessed in SDL_IMFMusicPlayer().

-- K1n9_Duk3
-----------------------------------------------------------------------------*/
//byte *curAlSound = 0;
//byte *curAlSoundPtr = 0;
//longword curAlLengthLeft = 0;

static int numreadysamples = 0;
static int soundTimeCounter = SOUND_TICKS;
static void SDL_IMFMusicPlayer(void *udata, Uint8 *stream, int sampleslen)
{
	Sint16 *stream16 = (Sint16 *) (void *) stream;    // expect correct alignment

	while(1)
	{
		if(numreadysamples)
		{
			if(numreadysamples<sampleslen)
			{
				if(MusicMode != smm_Off || SoundMode == sdm_AdLib)
					YM3812UpdateOne(oplChip, stream16, numreadysamples);

				// Mix the emulated PC sounds into the AdLib buffer:
				SDL_PCEmulateAndMix(stream16, numreadysamples);

				stream16 += numreadysamples*2;
				sampleslen -= numreadysamples;
			}
			else
			{
				if(MusicMode != smm_Off || SoundMode == sdm_AdLib)
					YM3812UpdateOne(oplChip, stream16, sampleslen);

				// Mix the emulated PC sounds into the AdLib buffer:
				SDL_PCEmulateAndMix(stream16, sampleslen);

				numreadysamples -= sampleslen;
				return;
			}
		}

		SDL_LockMutex(audioMutex);

		soundTimeCounter--;
		if(!soundTimeCounter)
		{
			// Sound effects are played at 140 Hz (every 5 cycles of the 700 Hz music service)
			soundTimeCounter = SOUND_TICKS;

			SDL_PCService();

			// THIS is the way the original Wolfenstein 3-D code handled it!
			if(alSound)
			{
				if(*alSound)
				{
					alOut(alFreqL, *alSound);
					alOut(alFreqH, alBlock);
				} else alOut(alFreqH, 0);
				alSound++;
				if (!(--alLengthLeft))
				{
					alSound = 0;
					SoundPriority=0;
					alOut(alFreqH, 0);
				}
			}
		}

		if (sqActive && midiOn)
			MIDI_IRQService();
		else if (sqActive)
		{
			do
			{
				if(sqHackTime > alTimeCount) break;
				sqHackTime = alTimeCount + LittleShort(*(sqHackPtr+1));
				alOutMusic(*(byte *) sqHackPtr, *(((byte *) sqHackPtr)+1));
				sqHackPtr += 2;
				sqHackLen -= 4;
			}
			while(sqHackLen>0);
			alTimeCount++;
			if(!sqHackLen)
			{
				sqHackPtr = sqHack;
				sqHackLen = sqHackSeqLen;
				sqHackTime = 0;
				alTimeCount = 0;
			}
		}

		numreadysamples = samplesPerMusicTick;

		SDL_UnlockMutex(audioMutex);
	}
}

static void SDL_MixEmulators(void *udata, Uint8 *mixed_stream, int len)
{
	if(MusicMode == smm_Off && !(SoundMode == sdm_AdLib || SoundMode == sdm_PC))
		return;

	const int sampleslen = (len/AudioSpec.channels)>>1;

	// Setup buffer that we can write emulator data to before mixing in
	static Uint8 *stream;
	static int stream_len = 0;
	if(len > stream_len)
	{
		// If we're converting from stereo to surround then we need to ensure
		// that this buffer is large enough to hold any intermediate conversions.
		if(AudioCVTStereo.needed && AudioCVTStereo.len_mult > AudioCVTStereo.len_ratio)
			stream_len = (sampleslen<<2)*AudioCVTStereo.len_mult;
		else
			stream_len = len;
		stream = (Uint8*)realloc(stream, stream_len);
	}

	memset(stream, 0, len);
	SDL_IMFMusicPlayer(udata, stream, sampleslen);

	if(AudioCVTStereo.needed)
	{
		// Stereo 16-bit to whatever was opened
		AudioCVTStereo.len = sampleslen<<2;
		AudioCVTStereo.buf = stream;
		SDL_ConvertAudio(&AudioCVTStereo);
	}

#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_MixAudioFormat(mixed_stream, stream, AUDIO_S16, len, SDL_MIX_MAXVOLUME);
#else
	SDL_MixAudio(mixed_stream, stream, len, SDL_MIX_MAXVOLUME);
#endif
}

///////////////////////////////////////////////////////////////////////////
//
//	SDL_StartSB() - Turns on the SoundBlaster
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_StartSB()
{
	if(SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
	{
		Printf("S_Init: Unable to initialize audio.\n");
		return;
	}

	if(Mix_OpenAudio(AudioSpec.frequency, AudioSpec.format, AudioSpec.channels, param_audiobuffer))
	{
		printf("S_Init: Unable to open audio: %s\n", Mix_GetError());
		return;
	}
	atterm(Mix_CloseAudio);

	if(Mix_QuerySpec(&AudioSpec.frequency, &AudioSpec.format, &AudioSpec.channels) == 0)
	{
		printf("S_Init: Failed to query audio format!\n");
	}
	printf("S_Init: Opened audio: %dHz (%d channels)\n", AudioSpec.frequency, AudioSpec.channels);

	if(SDL_BuildAudioCVT(&AudioCVTStereo, AUDIO_S16, 2, AudioSpec.frequency, AudioSpec.format, AudioSpec.channels, AudioSpec.frequency) < 0)
	{
		printf("S_Init: Failed to build stereo audio conversion: %s\n", SDL_GetError());
	}

	Mix_ReserveChannels(2);  // reserve player and boss weapon channels
	Mix_GroupChannels(2, MIX_CHANNELS-1, 1); // group remaining channels

	// Init music
	if(YM3812Init(1,3579545,AudioSpec.frequency))
	{
		printf("S_Init: Unable to create virtual OPL!!\n");
	}

	for(int i=1;i<0xf6;i++)
		YM3812Write(oplChip,i,0,MAX_VOLUME);

	YM3812Write(oplChip,1,0x20,MAX_VOLUME); // Set WSE=1

	samplesPerMusicTick = AudioSpec.frequency / MUSIC_RATE; // SDL_t0FastAsmService played at 700Hz
	Mix_SetPostMix(SDL_MixEmulators, 0);
	Mix_ChannelFinished(SD_ChannelFinished);

	Mix_VolumeMusic(static_cast<int> (ceil(128.0*MULTIPLY_VOLUME(MusicVolume))));

	// Make sure that the musicFinished() function is called when the music stops playing
	Mix_HookMusicFinished(musicFinished);

	AdLibPresent = true;
	SoundBlasterPresent = true;
}

//      Public routines

///////////////////////////////////////////////////////////////////////////
//
//      SD_SetSoundMode() - Sets which sound hardware to use for sound effects
//
///////////////////////////////////////////////////////////////////////////
bool SD_SetSoundMode(SDMode mode)
{
	bool result = false;

	SD_StopSound();

	if ((mode == sdm_AdLib) && !AdLibPresent)
		mode = sdm_PC;

	switch (mode)
	{
		case sdm_Off:
		case sdm_PC:
			result = true;
			break;
		case sdm_AdLib:
			if (AdLibPresent)
				result = true;
			break;
		default:
			I_FatalError("SD_SetSoundMode: Invalid sound mode %i", mode);
			return false;
	}

	if (result && (mode != SoundMode))
	{
		SDL_ShutDevice();
		SoundMode = mode;
		SDL_StartDevice();
	}

	return(result);
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_SetMusicMode() - sets the device to use for background music
//
///////////////////////////////////////////////////////////////////////////
bool SD_SetMusicMode(SMMode mode)
{
	bool result = false;

	SD_FadeOutMusic();
	while (SD_MusicPlaying())
		SDL_Delay(5);

	switch (mode)
	{
		case smm_Off:
			result = true;
			break;
		case smm_AdLib:
		case smm_Midi:
			if (AdLibPresent)
				result = true;
			break;
	}

	if (result)
		MusicMode = mode;

//    SDL_SetTimerSpeed();

	return(result);
}


///////////////////////////////////////////////////////////////////////////
//
//      SD_Startup() - starts up the Sound Mgr
//              Detects all additional sound hardware and installs my ISR
//
///////////////////////////////////////////////////////////////////////////
void
SD_Startup(void)
{
	if (SD_Started)
		return;

	if((audioMutex = SDL_CreateMutex()) == NULL)
	{
		I_Error("Unable to create audio mutex");
		return;
	}

#if defined(__ANDROID__)
	// Working directory will be in the form: Beloko/Wolf3d/FULL
	Mix_SetSoundFonts("../../FluidR3_GM.sf2");
#elif defined(__unix__)
	Mix_SetSoundFonts("/usr/share/sounds/sf2/FluidR3_GM.sf2");
#endif

	AudioSpec.frequency = param_samplerate;
	AudioSpec.format = AUDIO_S16;
	AudioSpec.channels = 2;

	SDL_StartSB();

	alTimeCount = 0;

	SD_SetSoundMode(sdm_Off);
	SD_SetMusicMode(smm_Off);

	SoundInfo.Init();
	SoundSeq.Init();

	SD_Started = true;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_Shutdown() - shuts down the Sound Mgr
//              Removes sound ISR and turns off whatever sound hardware was active
//
///////////////////////////////////////////////////////////////////////////
void
SD_Shutdown(void)
{
	if (!SD_Started)
		return;

	SD_MusicOff();
	SD_StopSound();

	if(audioMutex != NULL)
	{
		SDL_DestroyMutex(audioMutex);
		audioMutex = NULL;
	}

	SDL_QuitSubSystem(SDL_INIT_AUDIO);

	SD_Started = false;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_PositionSound() - Sets up a stereo imaging location for the next
//              sound to be played. Each channel ranges from 0 to 15.
//
///////////////////////////////////////////////////////////////////////////
void
SD_PositionSound(int leftvol,int rightvol)
{
	LeftPosition = leftvol;
	RightPosition = rightvol;
	nextsoundpos = true;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_PlaySound() - plays the specified sound on the appropriate hardware
//              Returns the channel of the sound if it played, else 0.
//
///////////////////////////////////////////////////////////////////////////
int SD_PlaySound(const char* sound, SoundChannel chan)
{
	bool            ispos;
	int             lp,rp;

	lp = LeftPosition;
	rp = RightPosition;
	LeftPosition = 0;
	RightPosition = 0;

	ispos = nextsoundpos;
	nextsoundpos = false;

	const SoundData &sindex = SoundInfo[sound];

	if ((SoundMode != sdm_Off) && sindex.IsNull())
		return 0;

	if ((DigiMode != sds_Off) && sindex.HasType(SoundData::DIGITAL))
	{
		if ((DigiMode == sds_PC) && (SoundMode == sdm_PC))
		{
#ifdef NOTYET
			if (s->priority < SoundPriority)
				return 0;

			SDL_PCStopSound();

			SD_PlayDigitized(sindex,lp,rp);
			SoundPositioned = ispos;
			SoundPriority = s->priority;
#else
			return 0;
#endif
		}
		else
		{
#ifdef NOTYET
			if (s->priority < DigiPriority)
				return(false);
#endif

			int channel = SD_PlayDigitized(sindex, lp, rp, chan);
			SoundPositioned = ispos;
			DigiPriority = sindex.GetPriority();
			SoundPlaying = sound;
			return channel;
		}

		return(true);
	}

	if (SoundMode == sdm_Off)
		return 0;

	if (sindex.GetPriority() < SoundPriority)
		return 0;

#ifndef ECWOLF_MIXER
	// With stock SDL_mixer we can't play music and emulated sounds.
	if (music != NULL)
		return 0;
#endif

	bool didPlaySound = false;

	switch (SoundMode)
	{
		default:
			didPlaySound = true;
			break;
		case sdm_PC:
			if(sindex.HasType(SoundData::PCSPEAKER))
			{
				SDL_PCPlaySound((PCSound *)sindex.GetSpeakerData());
				didPlaySound = true;
			}
			break;
		case sdm_AdLib:
			if(sindex.HasType(SoundData::ADLIB))
			{
				SDL_ALPlaySound((AdLibSound *)sindex.GetAdLibData());
				didPlaySound = true;
			}
			break;
	}

	if (didPlaySound)
	{
		SoundPriority = sindex.GetPriority();
		SoundPlaying = sound;
	}

	return 0;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_SoundPlaying() - returns the sound number that's playing, or 0 if
//              no sound is playing
//
///////////////////////////////////////////////////////////////////////////
bool SD_SoundPlaying(void)
{
	bool result = false;

	switch (SoundMode)
	{
		default:
			break;
		case sdm_PC:
			result = pcSound? true : false;	// not really thread-safe, but a mutex would be overkill
			break;
		case sdm_AdLib:
			result = alSound? true : false;	// not really thread-safe, but a mutex would be overkill
			break;
	}

	if (result)
		return SoundPlaying.IsNotEmpty();
	else
		return false;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_StopSound() - if a sound is playing, stops it
//
///////////////////////////////////////////////////////////////////////////
void
SD_StopSound(void)
{
	if (DigiPlaying)
		SD_StopDigitized();

	switch (SoundMode)
	{
		default:
			break;
		case sdm_PC:
			SDL_PCStopSound();
			break;
		case sdm_AdLib:
			SDL_ALStopSound();
			break;
	}

	SoundPositioned = false;

	SDL_SoundFinished();
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_WaitSoundDone() - waits until the current sound is done playing
//
///////////////////////////////////////////////////////////////////////////
void
SD_WaitSoundDone(void)
{
	while (SD_SoundPlaying())
		SDL_Delay(5);
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_MusicOn() - turns on the sequencer
//
///////////////////////////////////////////////////////////////////////////
void
SD_MusicOn(void)
{
	sqActive = true;	// not really thread-safe, but a mutex would be overkill
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_MusicOff() - turns off the sequencer and any playing notes
//      returns the last music offset for music continue
//
///////////////////////////////////////////////////////////////////////////
int
SD_MusicOff(void)
{
	word    i;
	int musoffs;

	SDL_LockMutex(audioMutex);

	sqActive = false;
	musoffs = (int) (midiOn ? midiData-midiDataStart : sqHackPtr-sqHack);

	SDL_UnlockMutex(audioMutex);

	switch (MusicMode)
	{
		default:
			break;
		case smm_AdLib:
		case smm_Midi:
			if (music == NULL)
			{
				alOut(alEffects, 0);
				for (i = 0;i < sqMaxTracks;i++)
					alOut(alFreqH + i + 1, 0);
			}
			else
			{
				if(Mix_PlayingMusic() == 1)
				{
					Mix_PauseMusic();
					return (int)Mix_GetMusicPCMPosition();
				}
				return 0;
			}
			break;
	}

	return musoffs;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_StartMusic() - starts playing the music pointed to
//
///////////////////////////////////////////////////////////////////////////
void
SD_StartMusic(const char* chunk)
{
	static const Instrument ChannelRelease = {
		0, 0,
		0x3F, 0x3F,
		0xFF, 0xFF,
		0xF, 0xF,
		0, 0,
		0,

		0, 0, {0, 0, 0}
	};

	SD_MusicOff();

	if (MusicMode != smm_Off)
	{
		int lumpNum = SoundInfo.GetMusicLumpNum(chunk);
		if(lumpNum == -1)
			return;

		// Load our music file from chunk
		chunkmem = new byte[Wads.LumpLength(lumpNum)];
		FWadLump lump = Wads.OpenLumpNum(lumpNum);
		lump.Read(chunkmem.Get(), Wads.LumpLength(lumpNum));

		if(music)
			Mix_FreeMusic(music);
		music = NULL;

		// But first try to use the MIDI playback code from Noah3D
		SDL_LockMutex(audioMutex);
		midiOn = MIDI_TryToStart(chunkmem.Get(), Wads.LumpLength(lumpNum));
		if (midiOn)
			midiChunkFreeable = chunkmem.Release();
		SDL_UnlockMutex(audioMutex);

		if (midiOn)
		{
			SD_MusicOn();
			return;
		}

		SDL_RWops *mus_cunk = SDL_RWFromMem(chunkmem.Get(), Wads.LumpLength(lumpNum));

		// Technically an SDL_mixer 2 feature to free the source
#if defined(ECWOLF_MIXER) || SDL_VERSION_ATLEAST(2,0,0)
		music = Mix_LoadMUS_RW(mus_cunk, true);
#else
		music = Mix_LoadMUS_RW(mus_cunk);
#endif

		// We assume that when music equals to NULL, we've an IMF file to play
		if (music == NULL)
		{
			SDL_LockMutex(audioMutex);

			for (int i = 0;i < OPL_CHANNELS;++i)
				SDL_AlSetChanInst(&ChannelRelease, i);

			sqHack = reinterpret_cast<word*>(chunkmem.Release());
			sqHackFreeable = sqHack;
			if(*sqHack == 0) sqHackLen = sqHackSeqLen = Wads.LumpLength(lumpNum);
			else sqHackLen = sqHackSeqLen = LittleShort(*sqHack++);
			sqHackPtr = sqHack;
			sqHackTime = 0;
			alTimeCount = 0;

			SDL_UnlockMutex(audioMutex);

			SD_MusicOn();
		}
		else
		{
			SDL_LockMutex(audioMutex);

			// Play the music
			musicchunk = lumpNum;
			if (Mix_PlayMusic(music, -1) == -1)
			{
				printf("Unable to play music file: %s\n", Mix_GetError());
			}

			SDL_UnlockMutex(audioMutex);
		}
	}
}

void
SD_ContinueMusic(const char* chunk, int startoffs)
{
	SD_MusicOff();

	if (MusicMode != smm_Off)
	{
		int lumpNum = SoundInfo.GetMusicLumpNum(chunk);
		if(lumpNum == -1)
			return;

		if (music == NULL || musicchunk != lumpNum)
		{ // We need this scope to "delete" the lump before modifying the sqHack pointers.
			SDL_LockMutex(audioMutex);
			FWadLump lump = Wads.OpenLumpNum(lumpNum);
			sqHackFreeable.Reset();
			midiChunkFreeable.Reset();
			musicchunk = -1;

			// Load our music file from chunk
			chunkmem = new byte[Wads.LumpLength(lumpNum)];
			lump.Read(chunkmem.Get(), Wads.LumpLength(lumpNum));

			if(music)
				Mix_FreeMusic(music);
			music = NULL;

			// But first try to use the MIDI playback code from Noah3D
			midiOn = MIDI_TryToStart(chunkmem.Get(), Wads.LumpLength(lumpNum));
			if (midiOn)
			{
				midiChunkFreeable = chunkmem.Release();

				// fast forward to correct position
				// (needed to reconstruct the instruments)
				// TODO: This isn't perfect.
				while (midiData-midiDataStart < startoffs)
					MIDI_IRQService();
				SDL_UnlockMutex(audioMutex);
				SD_MusicOn();
				return;
			}

			SDL_RWops *mus_cunk = SDL_RWFromMem(chunkmem.Get(), Wads.LumpLength(lumpNum));
#if defined(ECWOLF_MIXER) || SDL_VERSION_ATLEAST(2,0,0)
			music = Mix_LoadMUS_RW(mus_cunk, true);
#else
			music = Mix_LoadMUS_RW(mus_cunk);
#endif
			if (music == NULL)
			{
				sqHack = reinterpret_cast<word*>(chunkmem.Release());
				sqHackFreeable = sqHack;
				if(*sqHack == 0) sqHackLen = sqHackSeqLen = Wads.LumpLength(lumpNum);
				else sqHackLen = sqHackSeqLen = LittleShort(*sqHack++);
				sqHackPtr = sqHack;
			}
		}

		if (music == NULL)
		{
			if(startoffs >= sqHackLen)
			{
				SDL_UnlockMutex(audioMutex);
				I_FatalError("SD_StartMusic: Illegal startoffs provided!");
			}

			// fast forward to correct position
			// (needed to reconstruct the instruments)

			for(int i = 0; i < startoffs; i += 2)
			{
				byte reg = *(byte *)sqHackPtr;
				byte val = *(((byte *)sqHackPtr) + 1);
				if(reg >= 0xb1 && reg <= 0xb8) val &= 0xdf;           // disable play note flag
				else if(reg == 0xbd) val &= 0xe0;                     // disable drum flags

				alOut(reg,val);
				sqHackPtr += 2;
				sqHackLen -= 4;
			}
			sqHackTime = 0;
			alTimeCount = 0;

			SDL_UnlockMutex(audioMutex);

			SD_MusicOn();
		}
		else
		{
			SDL_UnlockMutex(audioMutex);

			if (Mix_PausedMusic() == 1 && musicchunk == lumpNum)
			{
				Mix_ResumeMusic();
				return;
			}

			// Play the music
			musicchunk = lumpNum;
			if (Mix_PlayMusic(music, -1) == -1)
			{
				printf("Unable to play music file: %s\n", Mix_GetError());
			}

			Mix_SetMusicPCMPosition(startoffs);
		}
	}
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_FadeOutMusic() - starts fading out the music. Call SD_MusicPlaying()
//              to see if the fadeout is complete
//
///////////////////////////////////////////////////////////////////////////
void
SD_FadeOutMusic(void)
{
	switch (MusicMode)
	{
		default:
			break;
		case smm_AdLib:
		case smm_Midi:
			// DEBUG - quick hack to turn the music off
			SD_MusicOff();
			break;
	}
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_MusicPlaying() - returns true if music is currently playing, false if
//              not
//
///////////////////////////////////////////////////////////////////////////
bool SD_MusicPlaying(void)
{
	bool result;

	switch (MusicMode)
	{
		case smm_AdLib:
		case smm_Midi:
			if (music == NULL)
				result = sqActive;	// not really thread-safe, but a mutex would be overkill
			else
				result = Mix_PlayingMusic() && !Mix_PausedMusic();
			break;
		default:
			result = false;
			break;
	}

	return(result);
}
