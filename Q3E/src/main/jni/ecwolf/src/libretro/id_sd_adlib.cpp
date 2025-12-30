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
#include "w_wad.h"
#include "zstring.h"
#include "sndinfo.h"
#include "sndseq.h"
#include "wl_main.h"
#include "id_sd.h"
#include "state_machine.h"
#include "wl_play.h"
#ifdef USE_GPL
#include "dosbox/dbopl.h"
#else
#include "mame/fmopl.h"
#endif

static const int synthesisRate = 44100;
#define MUSIC_RATE 700	// Must be a multiple of SOUND_RATE
#define SOUND_RATE 140	// Also affects PC Speaker sounds
static const int samplesPerSoundTick = synthesisRate / SOUND_RATE;
static const int samplesPerMusicTick = synthesisRate / MUSIC_RATE;
#undef alOut
#define alOut(n,b) 		YM3812Write(oplChip, n, b, 20)

#ifdef USE_GPL

DBOPL::Chip oplChip;
DBOPL::Chip musicOpl;

static inline bool YM3812Init(int numChips, int clock, int rate)
{
	static int inited = false;
	if (inited)
		return false;
	inited = true;
	oplChip.Setup(rate);
	musicOpl.Setup(rate);
	return false;
}

static inline void YM3812Write(DBOPL::Chip &which, Bit32u reg, Bit8u val, const int &volume)
{
	which.WriteReg(reg, val);
}

void YM3812UpdateOneMono(DBOPL::Chip &which, int16_t *stream, int length)
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
		for(i = 0; i < length; i ++)
		{
			// Multiply by 4 to match loudness of MAME emulator.
			Bit32s sample = buffer[i * 2] << 2;
			if(sample > 32767) sample = 32767;
			else if(sample < -32768) sample = -32768;
			stream[i] = sample;
		}
	}
	else
	{
		which.GenerateBlock2(length, buffer);

		// GenerateBlock3 generates a number of "length" 32-bit mono samples
		// so we need to convert them to 16-bit mono samples
		for(i = 0; i < length; i++)
		{
			// Multiply by 4 to match loudness of MAME emulator.
			// Then upconvert to stereo.
			Bit32s sample = buffer[i] << 2;
			if(sample > 32767) sample = 32767;
			else if(sample < -32768) sample = -32768;
			stream[i] = sample;
		}
	}
}

#else

static const int oplChip = 0;

#endif

void    SD_Startup_Adlib(void)
{
	YM3812Init(1,3579545,synthesisRate);
}

static void SDL_AlSetChanInst(DBOPL::Chip &oplChip, const Instrument *inst, unsigned int chan)
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
static void SDL_AlSetFXInst(DBOPL::Chip &oplChip, const Instrument *inst)
{
	SDL_AlSetChanInst(oplChip, inst, 0);
}

Mix_Chunk *SynthesizeAdlib(const byte *dataRaw)
{
	AdLibSound *sound = (AdLibSound*) dataRaw;

	alOut(alFreqH + 0, 0);

	int alLength = LittleLong(sound->common.length);
	byte alBlock = ((sound->block & 7) << 2) | 0x20;
	Instrument      *inst = &sound->inst;

	if (!(inst->mSus | inst->cSus))
	{
		I_FatalError("SDL_ALPlaySound() - Bad instrument");
	}

	SDL_AlSetFXInst(oplChip, inst);
	byte *alSound = (byte *)sound->data;

	int16_t *samples = (int16_t*) malloc (alLength * samplesPerSoundTick * 2);
	CHECKMALLOCRESULT(samples);
	int16_t *sampleptr = samples;

	for (int i = 0; i < alLength; i++, alSound++) {
		if(*alSound)
		{
			alOut(alFreqL, *alSound);
			alOut(alFreqH, alBlock);
		} else alOut(alFreqH, 0);

		YM3812UpdateOneMono(oplChip, sampleptr, samplesPerSoundTick);
		sampleptr += samplesPerSoundTick;
	}
	return new Mix_Chunk_Digital(
		synthesisRate,
		samples,
		sampleptr - samples,
		FORMAT_16BIT_LINEAR_SIGNED_NATIVE,
		false
		);
}

Mix_Chunk_IMF::Mix_Chunk_IMF(int rate, const byte *imf, size_t imf_size,
			     bool isLooping)
{
	YM3812Init(1,3579545,synthesisRate);
	this->rate = rate;
	this->sample_count = 0;
	this->chunk_samples = NULL;
	this->sample_format = FORMAT_16BIT_LINEAR_SIGNED_NATIVE;
	this->isLooping = isLooping;
	this->samples_allocated = 0;
	this->imf = (byte*) malloc(imf_size * 4);
	CHECKMALLOCRESULT(this->imf);
	memcpy(this->imf, imf, imf_size * 4);
	this->imfptr = 0;
	this->imfsize = imf_size;

	static const Instrument ChannelRelease = {
		0, 0,
		0x3F, 0x3F,
		0xFF, 0xFF,
		0xF, 0xF,
		0, 0,
		0,

		0, 0, {0, 0, 0}
	};

	for (int i = 0;i < OPL_CHANNELS;++i)
		SDL_AlSetChanInst(musicOpl, &ChannelRelease, i);
}

Mix_Chunk *SynthesizeAdlibIMFOrN3D(const byte *dataRaw, size_t size)
{
	if (midiN3DValidate(dataRaw, size)) {
		return new Mix_Chunk_N3D(synthesisRate, dataRaw, size, true);
	}
	int alLength = size / 4;
	const byte *alSound = dataRaw;
	if(alSound[0] != 0 || alSound[1] != 0) {
		alLength = ReadLittleShort(alSound) / 4;
		if (alLength > (int) (size - 2) / 4)
			alLength = (size - 2) / 4;
		alSound += 2;
	}

	return new Mix_Chunk_IMF(synthesisRate, alSound, alLength, true);
}

void Mix_Chunk_IMF::EnsureSpace(int need_samples)
{
	if (samples_allocated >= need_samples)
		return;
	size_t nl = need_samples + need_samples / 2;
	if (nl < 256)
		nl = 256;
	chunk_samples = realloc(chunk_samples, nl * 2);
	samples_allocated = nl;
}

void Mix_Chunk_IMF::EnsureSynthesis(int maxTics)
{
	for (;imfptr < imfsize && sample_count < maxTics * rate / TICRATE; imfptr++) {
		byte reg = imf[4*imfptr];
		byte val = imf[4*imfptr + 1];
		int tics = ReadLittleShort((BYTE *)imf + imfptr * 4 + 2);
		YM3812Write(musicOpl, reg, val, 20);

		EnsureSpace(sample_count + samplesPerMusicTick * tics);

		while (tics) {
			int curtics = tics;
			if (curtics > 500 / samplesPerMusicTick)
				curtics = 500 / samplesPerMusicTick;
			YM3812UpdateOneMono(musicOpl, (int16_t *)chunk_samples + sample_count, samplesPerMusicTick * curtics);
			sample_count += samplesPerMusicTick * curtics;
			tics -= curtics;
		}
	}
//	printf ("synth: %d\n", sample_count);
}
