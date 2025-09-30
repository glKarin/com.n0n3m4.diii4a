/*
Copyright (c) 2010-2013  p5yc0runn3r at gmail.com

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/

#ifndef NO_DMAHD

/*
	IMPLEMENTATION DETAILS:

	1. Remove from static any unresolved extern globals:
	From snd_dma.c remove static keyword for the following:
		'static' int			s_soundStarted;
		'static' qboolean		s_soundMuted;
		'static' int			listener_number;
		'static' vec3_t			listener_origin;
		'static' vec3_t			listener_axis[3];
		'static' loopSound_t	loopSounds[MAX_GENTITIES];

	2. In "snd_local.h" add the following changes (channel_t structure changed + ch_side_t structure added):
		typedef struct 
		{
			int			vol; // Must be first member due to union (see channel_t)
			int 		offset;
			int 		bassvol;
		} ch_side_t;

		typedef struct
		{
			int			allocTime;
			int			startSample;	// START_SAMPLE_IMMEDIATE = set immediately on next mix
			int			entnum;			// to allow overriding a specific sound
			int			entchannel;		// to allow overriding a specific sound
			int			master_vol;		// 0-255 volume before spatialization
			float		dopplerScale;
			float		oldDopplerScale;
			vec3_t		origin;			// only use if fixed_origin is set
			qboolean	fixed_origin;	// use origin instead of fetching entnum's origin
			sfx_t		*thesfx;		// sfx structure
			qboolean	doppler;
			union
			{
				int			leftvol;		// 0-255 volume after spatialization
				ch_side_t	l;
			};
			union
			{
				int			rightvol;		// 0-255 volume after spatialization
				ch_side_t	r;
			};
			vec3_t		sodrot;
		} channel_t;

	3. In "snd_local.h" add the following to sfx_t structure:
		qboolean		weaponsound;

	4. #include "snd_dmahd.h" in snd_mem.c and snd_dma.c

	5. Add the following at the top of S_LoadSound() in "snd_mem.c" after variables:
		#ifndef NO_DMAHD
			if (dmaHD_Enabled()) return dmaHD_LoadSound(sfx);
		#endif

	6. At the bottom of S_Base_Init() in "snd_dma.c" replace "return qtrue" with:
		#ifndef NO_DMAHD
			if (dmaHD_Enabled()) return dmaHD_Init(si);
		#endif

	7. Fix S_UpdateBackgroundTrack in "snd_dma.c"
	Replace: 
		fileSamples = bufferSamples * s_backgroundStream->info.rate / dma.speed;
	With:
		fileSamples = (bufferSamples * dma.speed) / s_backgroundStream->info.rate;

	8. (Skip if source file does not exist)
	Add the following to win_snd.c as a global extern:
		extern cvar_t		*s_khz;

	9. (Skip if source file does not exist)
	Fix SNDDMA_InitDS() in win_snd.c
	Replace:
		//	if (s_khz->integer == 44)
		//		dma.speed = 44100;
		//	else if (s_khz->integer == 22)
		//		dma.speed = 22050;
		//	else
		//		dma.speed = 11025;

		dma.speed = 22050;
	With:
		if (s_khz->integer >= 44) dma.speed = 44100;
		else if (s_khz->integer >= 32) dma.speed = 32000;
		else if (s_khz->integer >= 24) dma.speed = 24000;
		else if (s_khz->integer >= 22) dma.speed = 22050;
		else dma.speed = 11025;

	10. Compile, Run and Enjoy!! :)
*/

#include "client.h"
#include "snd_local.h"
#include "snd_codec.h"
#include "snd_dmahd.h"

void dmaHD_Update_Mix( void );
void S_UpdateBackgroundTrack(void);
void S_GetSoundtime(void);
qboolean S_ScanChannelStarts(void);


// used in dmaEX mixer.
#define							SOUND_FULLVOLUME		80
#define							SOUND_ATTENUATE			0.0007f

extern channel_t				s_channels[];
extern channel_t				loop_channels[];
extern int						numLoopChannels;

extern int						s_soundStarted;
extern qboolean					s_soundMuted;

extern int						listener_number;
vec3_t							g_listener_origin;
vec3_t							g_listener_axis[3];

extern int						s_soundtime;
extern int   					s_paintedtime;
static int						dmaHD_inwater;

// MAX_SFX may be larger than MAX_SOUNDS because of custom player sounds
#define MAX_SFX					4096 // This must be the same as the snd_dma.c
#define MAX_SOUNDBYTES			(256*1024*1024) // 256MiB MAXIMUM...
extern sfx_t					s_knownSfx[];
extern int						s_numSfx;

extern cvar_t					*s_mixahead;
cvar_t							*dmaHD_Enable = NULL;
cvar_t							*dmaHD_Interpolation;
cvar_t							*dmaHD_Mixer;
cvar_t							*dmaEX_StereoSeparation;


extern loopSound_t				loopSounds[];

#ifdef MAX_RAW_STREAMS
extern int						s_rawend[MAX_RAW_STREAMS];
extern portable_samplepair_t	s_rawsamples[MAX_RAW_STREAMS][MAX_RAW_SAMPLES];
#else
extern int						s_rawend;
extern portable_samplepair_t	s_rawsamples[];
#endif

#define DMAHD_PAINTBUFFER_SIZE	65536
static portable_samplepair_t	dmaHD_paintbuffer[DMAHD_PAINTBUFFER_SIZE];
static int						dmaHD_snd_vol;

qboolean g_tablesinit = qfalse;
float g_voltable[256];

#define SMPCLAMP(a) (((a) < -32768) ? -32768 : ((a) > 32767) ? 32767 : (a))
#define VOLCLAMP(a) (((a) < 0) ? 0 : ((a) > 255) ? 255 : (a))

void dmaHD_InitTables(void)
{
	if (!g_tablesinit)
	{
		int i;
		float x, y;

		// Volume table.
		for (i = 0; i < 256; i++)
		{
			x = (i * (9.0 / 256.0)) + 1.0;
			y = 1.0 - log10f(x);
			g_voltable[i] = y;
		}

		g_tablesinit = qtrue;
	}
}

/*
===============================================================================
PART#01: dmaHD: dma sound EXtension : MEMORY
===============================================================================
*/

int g_dmaHD_allocatedsoundmemory = 0;

/*
======================
dmaHD_FreeOldestSound
======================
*/

void dmaHD_FreeOldestSound( void ) 
{
	int	i, oldest, used;
	sfx_t *sfx;
	short* buffer;

	oldest = Com_Milliseconds();
	used = 0;

	for (i = 1 ; i < s_numSfx ; i++) 
	{
		sfx = &s_knownSfx[i];
		if (sfx->inMemory && sfx->lastTimeUsed < oldest) 
		{
			used = i;
			oldest = sfx->lastTimeUsed;
		}
	}

	sfx = &s_knownSfx[used];

	Com_DPrintf("dmaHD_FreeOldestSound: freeing sound %s\n", sfx->soundName);

	i = (sfx->soundLength * 2) * sizeof(short);
	g_dmaHD_allocatedsoundmemory -= i;
	if (g_dmaHD_allocatedsoundmemory < 0) g_dmaHD_allocatedsoundmemory = 0;
	if ((buffer = (short*)sfx->soundData) != NULL) free(buffer);
	sfx->inMemory = qfalse;
	sfx->soundData = NULL;
}

/*
======================
dmaHD_AllocateSoundBuffer
======================
*/

short* dmaHD_AllocateSoundBuffer(int samples)
{
	int bytes = samples * sizeof(short);
	short* buffer;
	
	while (g_dmaHD_allocatedsoundmemory > 0 &&
		(g_dmaHD_allocatedsoundmemory + bytes) > MAX_SOUNDBYTES) dmaHD_FreeOldestSound();

	if (s_numSfx >= (MAX_SFX - 8)) dmaHD_FreeOldestSound();

	do
	{
		if ((buffer = (short*)malloc(bytes)) != NULL) break;
		dmaHD_FreeOldestSound();
	} while(g_dmaHD_allocatedsoundmemory > 0);

	if (buffer == NULL) Com_Error (ERR_FATAL, "Out of Memory");

	g_dmaHD_allocatedsoundmemory += bytes;

	return buffer;
}

// =======================================================
// DMAHD - Interpolation functions / No need to optimize a lot here since the sounds are interpolated 
// once on load and not on playback. This also means that at least twice more memory is used.
// =======================================================
// x0-----x1--t--x2-----x3 / x0/2/3/4 are know samples / t = 0.0 - 1.0 between x1 and x2 / returns y value at point t
static float dmaHD_InterpolateCubic(float x0, float x1, float x2, float x3, float t) {
    float a0,a1,a2,a3;a0=x3-x2-x0+x1;a1=x0-x1-a0;a2=x2-x0;a3=x1;
    return (a0*(t*t*t))+(a1*(t*t))+(a2*t)+(a3);
}
static float dmaHD_InterpolateHermite4pt3oX(float x0, float x1, float x2, float x3, float t) {
    float c0,c1,c2,c3;c0=x1;c1=0.5f*(x2-x0);c2=x0-(2.5f*x1)+(2*x2)-(0.5f*x3);c3=(0.5f*(x3-x0))+(1.5f*(x1-x2));
    return (((((c3*t)+c2)*t)+c1)*t)+c0;
}
static float dmaHD_NormalizeSamplePosition(float t, int samples) {
	if (!samples) return t;
	while (t<0.0) t+=(float)samples;
	while (t>=(float)samples) t-=(float)samples;
	return t;
}
static int dmaHD_GetSampleRaw_8bitMono(int index, int samples, byte* data)
{
	if (index < 0) index += samples; else if (index >= samples) index -= samples;
	return (int)(((byte)(data[index])-128)<<8);
}
static int dmaHD_GetSampleRaw_16bitMono(int index, int samples, byte* data)
{
	if (index < 0) index += samples; else if (index >= samples) index -= samples;
	return (int)LittleShort(((short*)data)[index]);
}
static int dmaHD_GetSampleRaw_8bitStereo(int index, int samples, byte* data)
{
	int left, right;
	if (index < 0) index += samples; else if (index >= samples) index -= samples;
	left = (int)(((byte)(data[index * 2])-128)<<8);
	right = (int)(((byte)(data[index * 2 + 1])-128)<<8);
	return (left + right) / 2;
}
static int dmaHD_GetSampleRaw_16bitStereo(int index, int samples, byte* data)
{
	int left, right;
	if (index < 0) index += samples; else if (index >= samples) index -= samples;
	left = (int)LittleShort(((short*)data)[index * 2]);
	right = (int)LittleShort(((short*)data)[index * 2 + 1]);
	return (left + right) / 2;
}

// Get only decimal part (a - floor(a))
#define FLOAT_DECIMAL_PART(a) (a-(float)((int)a))

// t must be a float between 0 and samples
static int dmaHD_GetInterpolatedSampleHermite4pt3oX(float t, int samples, byte *data,
													int (*dmaHD_GetSampleRaw)(int, int, byte*))
{
	int x, val;

	t = dmaHD_NormalizeSamplePosition(t, samples);
	// Get points
	x = (int)t;
	// Interpolate
	val = (int)dmaHD_InterpolateHermite4pt3oX(
		(float)dmaHD_GetSampleRaw(x - 1, samples, data),
		(float)dmaHD_GetSampleRaw(x, samples, data),
		(float)dmaHD_GetSampleRaw(x + 1, samples, data),
		(float)dmaHD_GetSampleRaw(x + 2, samples, data), FLOAT_DECIMAL_PART(t));
	// Clamp
	return SMPCLAMP(val);
}

// t must be a float between 0 and samples
static int dmaHD_GetInterpolatedSampleCubic(float t, int samples, byte *data,
											int (*dmaHD_GetSampleRaw)(int, int, byte*))
{
	int x, val;

	t = dmaHD_NormalizeSamplePosition(t, samples);
	// Get points
	x = (int)t;
	// Interpolate
	val = (int)dmaHD_InterpolateCubic(
		(float)dmaHD_GetSampleRaw(x - 1, samples, data),
		(float)dmaHD_GetSampleRaw(x, samples, data),
		(float)dmaHD_GetSampleRaw(x + 1, samples, data),
		(float)dmaHD_GetSampleRaw(x + 2, samples, data), FLOAT_DECIMAL_PART(t));
	// Clamp
	return SMPCLAMP(val);
}

// t must be a float between 0 and samples
static int dmaHD_GetInterpolatedSampleLinear(float t, int samples, byte *data,
											 int (*dmaHD_GetSampleRaw)(int, int, byte*))
{
	int x, val;
	float c0, c1;

	t = dmaHD_NormalizeSamplePosition(t, samples);

	// Get points
	x = (int)t;
	
	c0 = (float)dmaHD_GetSampleRaw(x, samples, data);
	c1 = (float)dmaHD_GetSampleRaw(x+1, samples, data);
	
	val = (int)(((c1 - c0) * FLOAT_DECIMAL_PART(t)) + c0);
	// No need to clamp for linear
	return val;
}

// t must be a float between 0 and samples
static int dmaHD_GetNoInterpolationSample(float t, int samples, byte *data,
										  int (*dmaHD_GetSampleRaw)(int, int, byte*))
{
	int x;

	t = dmaHD_NormalizeSamplePosition(t, samples);

	// Get points
	x = (int)t;
	
	if (FLOAT_DECIMAL_PART(t) > 0.5) x++;

	return dmaHD_GetSampleRaw(x, samples, data);
}

int (*dmaHD_GetInterpolatedSample)(float t, int samples, byte *data, 
								   int (*dmaHD_GetSampleRaw)(int, int, byte*)) = 
	dmaHD_GetInterpolatedSampleHermite4pt3oX;

// =======================================================
// =======================================================

/*
================
dmaHD_ResampleSfx

resample / decimate to the current source rate
================
*/
void dmaHD_ResampleSfx( sfx_t *sfx, int channels, int inrate, int inwidth, byte *data, qboolean compressed)
{
	short* buffer;
	int (*dmaHD_GetSampleRaw)(int, int, byte*);
	float stepscale, idx_smp, sample, bsample;
	float lp_inva, lp_a, hp_a, lp_data, lp_last, hp_data, hp_last, hp_lastsample;
	int outcount, idx_hp, idx_lp;

	stepscale = (float)inrate/(float)dma.speed;
	outcount = (int)((float)sfx->soundLength / stepscale);

	// Create secondary buffer for bass sound while performing lowpass filter;
	buffer = dmaHD_AllocateSoundBuffer(outcount * 2);

	// Check if this is a weapon sound.
	sfx->weaponsound = (memcmp(sfx->soundName, "sound/weapons/", 14) == 0) ? qtrue : qfalse;

	if (channels == 2)
		dmaHD_GetSampleRaw = (inwidth == 2) ? dmaHD_GetSampleRaw_16bitStereo : dmaHD_GetSampleRaw_8bitStereo;
	else
		dmaHD_GetSampleRaw = (inwidth == 2) ? dmaHD_GetSampleRaw_16bitMono : dmaHD_GetSampleRaw_8bitMono;

	// Get last sample from sound effect.
	idx_smp = -(stepscale * 4.0f);
	sample = dmaHD_GetInterpolatedSample(idx_smp, sfx->soundLength, data, dmaHD_GetSampleRaw);
	bsample = dmaHD_GetNoInterpolationSample(idx_smp, sfx->soundLength, data, dmaHD_GetSampleRaw);
	idx_smp += stepscale;

	// Set up high pass filter.
	idx_hp = 0;
	hp_last = sample;
	hp_lastsample = sample;
	//buffer[idx_hp++] = sample;
	hp_a = 0.95f;

	// Set up Low pass filter.
	idx_lp = outcount;
	lp_last = bsample;
	lp_a = 0.03f;
	lp_inva = (1 - lp_a);

	// Now do actual high/low pass on actual data.
	for (;idx_hp < outcount; idx_hp++)
	{ 
		sample = dmaHD_GetInterpolatedSample(idx_smp, sfx->soundLength, data, dmaHD_GetSampleRaw);
		bsample = dmaHD_GetNoInterpolationSample(idx_smp, sfx->soundLength, data, dmaHD_GetSampleRaw);
		idx_smp += stepscale;

		// High pass.
		hp_data = hp_a * (hp_last + sample - hp_lastsample);
		buffer[idx_hp] = SMPCLAMP(hp_data);
		hp_last = hp_data;
		hp_lastsample = sample;

		// Low pass.
		lp_data = lp_a * (float)bsample + lp_inva * lp_last;
		buffer[idx_lp++] = SMPCLAMP(lp_data);
		lp_last = lp_data;
	}
	
	sfx->soundData = (sndBuffer*)buffer;
	sfx->soundLength = outcount;
}
//=============================================================================

qboolean dmaHD_LoadSound(sfx_t *sfx)
{
	byte *data;
	snd_info_t info;
	char dmahd_soundName[MAX_QPATH];
	char *lpext;

	// Player specific sounds are never directly loaded.
	if (sfx->soundName[0] == '*') return qfalse;

	strcpy(dmahd_soundName, sfx->soundName);
	if ((lpext = strrchr(sfx->soundName, '.')) != NULL)
	{
		strcpy(dmahd_soundName, sfx->soundName);
		*(strrchr(dmahd_soundName, '.')) = '\0'; // for sure there is a '.'
	}
	strcat(dmahd_soundName, "_dmahd");
	if (lpext != NULL) strcat(dmahd_soundName, lpext);

	// Just check if file exists
	if (FS_FOpenFileRead(dmahd_soundName, NULL, qtrue) == qtrue)
	{
		// Load it in.
		if (!(data = S_CodecLoad(dmahd_soundName, &info))) return qfalse;
	}
	else
	{
		// Load it in.
		if (!(data = S_CodecLoad(sfx->soundName, &info))) return qfalse;
	}

	// Information
	Com_DPrintf("Loading sound: %s", sfx->soundName);
	if (info.width == 1) Com_DPrintf(" [8 bit -> 16 bit]");
	if (info.rate != dma.speed) Com_DPrintf(" [%d Hz -> %d Hz]", info.rate, dma.speed);
	Com_DPrintf("\n");

	sfx->lastTimeUsed = Com_Milliseconds() + 1;

	// Do not compress.
	sfx->soundCompressionMethod = 0;
	sfx->soundLength = info.samples;
	sfx->soundData = NULL;
	dmaHD_ResampleSfx(sfx, info.channels, info.rate, info.width, data + info.dataofs, qfalse);
	
	// Free data allocated by Codec
	Z_Free(data);

	return qtrue;
}

/*
===============================================================================
PART#02: dmaHD: Mixing
===============================================================================
*/

static void dmaHD_PaintChannelFrom16_HHRTF(channel_t *ch, const sfx_t *sc, int count, int sampleOffset, int bufferOffset, int chan) 
{
	int vol, i, so;
	portable_samplepair_t *samp = &dmaHD_paintbuffer[bufferOffset];
	short *samples;
	short *tsamples;
	int *out;
	ch_side_t* chs = (chan == 0) ? &ch->l : &ch->r;

	if (dmaHD_snd_vol <= 0) return;

	so = sampleOffset - chs->offset;
	if (so < 0) { count += so; so = 0; } // [count -= (-so)] == [count += so]
	if ((so + count) >= sc->soundLength) count = sc->soundLength - so;
	if (count <= 0) return;
	if (chs->bassvol > 0) // Process low frequency
	{
		samples = &((short*)sc->soundData)[sc->soundLength]; // Select bass frequency offset (just after high frequency)
		// Calculate volumes.
		vol = chs->bassvol * dmaHD_snd_vol;
		tsamples = &samples[so];
		out = (int*)samp;
		if (chan == 1) out++;
		for (i = 0; i < count; i++) {
			*out += (*tsamples * vol) >> 8; ++tsamples; ++out; ++out;
		}
	}
	if (chs->vol > 0) // Process high frequency
	{
		samples = (short*)sc->soundData; // Select high frequency offset.
		// Calculate volumes.
		vol = chs->vol * dmaHD_snd_vol;
		tsamples = &samples[so];
		out = (int*)samp;
		if (chan == 1) out++;
		for (i = 0; i < count; i++) {
			*out += (*tsamples * vol) >> 8; ++tsamples; ++out; ++out;
		}
	}
}

static void dmaHD_PaintChannelFrom16_dmaEX2(channel_t *ch, const sfx_t *sc, int count, int sampleOffset, int bufferOffset) 
{
	int data, rvol, lvol, i, so;
	portable_samplepair_t *samp = &dmaHD_paintbuffer[bufferOffset];
	short *samples;
	short *tsamples;
	int *out;

	if (dmaHD_snd_vol <= 0) return;

	so = sampleOffset - ch->l.offset;
	if (so < 0) { count += so; so = 0; } // [count -= (-so)] == [count += so]
	if ((so + count) > sc->soundLength) count = sc->soundLength - so;
	if (count <= 0) return;

	if (ch->l.bassvol > 0) // Process low frequency.
	{
		samples = &((short*)sc->soundData)[sc->soundLength]; // Select bass frequency offset (just after high frequency)
		// Calculate volumes.
		lvol = ch->l.bassvol * dmaHD_snd_vol;
		tsamples = &samples[so];
		out = (int*)samp;
		for (i = 0; i < count; i++) 
		{ 
			data = (*tsamples * lvol) >> 8; ++tsamples;
			*out += data; ++out; // L
			*out += data; ++out; // R
		}
	}
	if (ch->l.vol > 0 || ch->r.vol > 0) // Process high frequency.
	{
		samples = (short*)sc->soundData; // Select high frequency offset.
		// Calculate volumes.
		lvol = ch->l.vol * dmaHD_snd_vol;
		rvol = ch->r.vol * dmaHD_snd_vol;
		// Behind viewer?
		if (ch->fixed_origin && ch->sodrot[0] < 0)
		{
			if (ch->r.vol > ch->l.vol) lvol = -lvol; else rvol = -rvol;
		}
		tsamples = &samples[so];
		out = (int*)samp;
		for (i = 0; i < count; i++)
		{ 
			*out += (*tsamples * lvol) >> 8; ++out; // L
			*out += (*tsamples * rvol) >> 8; ++out; // R
			++tsamples;
		}
	}
	if (ch->l.reverbvol > 0 || ch->r.reverbvol > 0) // Process high frequency reverb.
	{
		samples = (short*)sc->soundData; // Select high frequency offset.
		so = sampleOffset - ch->l.reverboffset;
		if (so < 0) { count += so; so = 0; } // [count -= (-so)] == [count += so]
		if ((so + count) > sc->soundLength) count = sc->soundLength - so;
		// Calculate volumes for reverb.
		lvol = ch->l.reverbvol * dmaHD_snd_vol;
		rvol = ch->r.reverbvol * dmaHD_snd_vol;
		tsamples = &samples[so];
		out = (int*)samp;
		for (i = 0; i < count; i++)
		{ 
			*out += (*tsamples * lvol) >> 8; ++out; // L
			*out += (*tsamples * rvol) >> 8; ++out; // R
			++tsamples;
		}
	}
}

static void dmaHD_PaintChannelFrom16_dmaEX(channel_t *ch, const sfx_t *sc, int count, int sampleOffset, int bufferOffset) 
{
	int rvol, lvol, i, so;
	portable_samplepair_t *samp = &dmaHD_paintbuffer[bufferOffset];
	short *samples, *bsamples;
	int *out;

	if (dmaHD_snd_vol <= 0) return;

	so = sampleOffset - ch->l.offset;
	if (so < 0) { count += so; so = 0; } // [count -= (-so)] == [count += so]
	if ((so + count) > sc->soundLength) count = sc->soundLength - so;
	if (count <= 0) return;
	if (ch->l.vol <= 0 && ch->r.vol <= 0) return;

	samples = &((short*)sc->soundData)[so]; // Select high frequency offset.
	bsamples = &((short*)sc->soundData)[sc->soundLength + so]; // Select bass frequency offset (just after high frequency)
	// Calculate volumes.
	lvol = ch->l.vol * dmaHD_snd_vol;
	rvol = ch->r.vol * dmaHD_snd_vol;
	// Behind viewer?
	if (ch->fixed_origin && ch->sodrot[0] < 0) 
	{
		if (lvol < rvol) lvol = -lvol; else rvol = -rvol;
	}
	out = (int*)samp;
	for (i = 0; i < count; i++) 
	{ 
		*out += ((*samples * lvol) >> 8) + ((*bsamples * lvol) >> 8); ++out; // L
		*out += ((*samples * rvol) >> 8) + ((*bsamples * rvol) >> 8); ++out; // R
		++samples; ++bsamples;
	}
}

static void dmaHD_PaintChannelFrom16(channel_t *ch, const sfx_t *sc, int count, int sampleOffset, int bufferOffset) 
{
	switch (dmaHD_Mixer->integer)
	{
	// Hybrid-HRTF
	case 10:
	case 11:
		dmaHD_PaintChannelFrom16_HHRTF(ch, sc, count, sampleOffset, bufferOffset, 0); // LEFT
		dmaHD_PaintChannelFrom16_HHRTF(ch, sc, count, sampleOffset, bufferOffset, 1); // RIGHT
		break;
	// dmaEX2
	case 20:
		dmaHD_PaintChannelFrom16_dmaEX2(ch, sc, count, sampleOffset, bufferOffset);
		break;
	case 21:
		// No reverb.
		ch->l.reverbvol = ch->r.reverbvol = 0;
		dmaHD_PaintChannelFrom16_dmaEX2(ch, sc, count, sampleOffset, bufferOffset);
		break;
	// dmaEX
	case 30:
		dmaHD_PaintChannelFrom16_dmaEX(ch, sc, count, sampleOffset, bufferOffset);
		break;
	}
}

void dmaHD_TransferPaintBuffer(int endtime)
{
	int		lpos;
	int		ls_paintedtime;
	int		i;
	int		val;
	int*     snd_p;  
	int      snd_linear_count;
	short*   snd_out;
	short*   snd_outtmp;
	unsigned long *pbuf = (unsigned long *)dma.buffer;

	snd_p = (int*)dmaHD_paintbuffer;
	ls_paintedtime = s_paintedtime;
	
	while (ls_paintedtime < endtime)
	{
		// handle recirculating buffer issues
		lpos = ls_paintedtime & ((dma.samples >> 1) - 1);

		snd_out = (short *)pbuf + (lpos << 1);

		snd_linear_count = (dma.samples >> 1) - lpos;
		if (ls_paintedtime + snd_linear_count > endtime)
			snd_linear_count = endtime - ls_paintedtime;

		snd_linear_count <<= 1;

		// write a linear blast of samples
		for (snd_outtmp = snd_out, i = 0; i < snd_linear_count; ++i)
		{
			val = *snd_p++ >> 8;
			*snd_outtmp++ = SMPCLAMP(val);
		}

		ls_paintedtime += (snd_linear_count>>1);

		if (CL_VideoRecording())
			CL_WriteAVIAudioFrame((byte *)snd_out, snd_linear_count << 1);
	}
}

void dmaHD_PaintChannels( int endtime ) 
{
	int 	i;
	int 	end;
	channel_t *ch;
	sfx_t	*sc;
	int		ltime, count;
	int		sampleOffset;
#ifdef MAX_RAW_STREAMS
	int		stream;
#endif

	dmaHD_snd_vol = 
#ifdef MAX_RAW_STREAMS
		(s_muted->integer) ? 0 : 
#endif
		s_volume->value*256;

	while ( s_paintedtime < endtime ) 
	{
		// if paintbuffer is smaller than DMA buffer we may need to fill it multiple times
		end = endtime;
		if ((endtime - s_paintedtime) >= DMAHD_PAINTBUFFER_SIZE ) 
		{
			end = s_paintedtime + DMAHD_PAINTBUFFER_SIZE;
		}

#ifdef MAX_RAW_STREAMS
		// clear the paint buffer and mix any raw samples...
		Com_Memset(dmaHD_paintbuffer, 0, sizeof (dmaHD_paintbuffer));
		for (stream = 0; stream < MAX_RAW_STREAMS; stream++) 
		{
			if (s_rawend[stream] >= s_paintedtime)
			{
				// copy from the streaming sound source
				const portable_samplepair_t *rawsamples = s_rawsamples[stream];
				const int stop = (end < s_rawend[stream]) ? end : s_rawend[stream];
				for ( i = s_paintedtime ; i < stop ; i++ ) 
				{
					const int s = i&(MAX_RAW_SAMPLES-1);
					dmaHD_paintbuffer[i-s_paintedtime].left += rawsamples[s].left;
					dmaHD_paintbuffer[i-s_paintedtime].right += rawsamples[s].right;
				}
			}
		}
#else
		// clear the paint buffer to either music or zeros
		if ( s_rawend < s_paintedtime ) 
		{
			Com_Memset(dmaHD_paintbuffer, 0, (end - s_paintedtime) * sizeof(portable_samplepair_t));
		}
		else 
		{
			// copy from the streaming sound source
			int		s;
			int		stop;

			stop = (end < s_rawend) ? end : s_rawend;

			for ( i = s_paintedtime ; i < stop ; i++ ) 
			{
				s = i&(MAX_RAW_SAMPLES-1);
				dmaHD_paintbuffer[i-s_paintedtime].left = s_rawsamples[s].left;
				dmaHD_paintbuffer[i-s_paintedtime].right = s_rawsamples[s].right;
			}
			for ( ; i < end ; i++ ) 
			{
				dmaHD_paintbuffer[i-s_paintedtime].left = 0;
				dmaHD_paintbuffer[i-s_paintedtime].right = 0;
			}
		}
#endif

		// paint in the channels.
		ch = s_channels;
		for ( i = 0; i < MAX_CHANNELS ; i++, ch++ ) 
		{
			if (!ch->thesfx) continue;

			ltime = s_paintedtime;
			sc = ch->thesfx;
			sampleOffset = ltime - ch->startSample;
			count = end - ltime;
			if (sampleOffset + count >= sc->soundLength) count = sc->soundLength - sampleOffset;
			if (count > 0) dmaHD_PaintChannelFrom16(ch, sc, count, sampleOffset, 0);
		}

		// paint in the looped channels.
		ch = loop_channels;
		for (i = 0; i < numLoopChannels ; i++, ch++)
		{		
			if (!ch->thesfx) continue;

			ltime = s_paintedtime;
			sc = ch->thesfx;

			if (sc->soundData == NULL || sc->soundLength == 0) continue;
			// we might have to make two passes if it is a looping sound effect and the end of the sample is hit
			do 
			{
				sampleOffset = (ltime % sc->soundLength);
				count = end - ltime;
				if (sampleOffset + count >= sc->soundLength) count = sc->soundLength - sampleOffset;
				if (count > 0) 
				{	
					dmaHD_PaintChannelFrom16(ch, sc, count, sampleOffset, ltime - s_paintedtime);
					ltime += count;
				}
			} while (ltime < end);
		}

		// transfer out according to DMA format
		dmaHD_TransferPaintBuffer(end);
		s_paintedtime = end;
	}
}

/*
===============================================================================
PART#03: dmaHD: main
===============================================================================
*/


/*
=================
dmaHD_SpatializeReset

Reset/Prepares channel before calling dmaHD_SpatializeOrigin
=================
*/
void dmaHD_SpatializeReset (channel_t* ch)
{
	VectorClear(ch->sodrot);
	memset(&ch->l, 0, sizeof(ch_side_t));
	memset(&ch->r, 0, sizeof(ch_side_t));
}

/*
=================
dmaHD_SpatializeOrigin

Used for spatializing s_channels
=================
*/

#define CALCVOL(dist) (((tmp = (int)((float)ch->master_vol * g_voltable[ \
			(((idx = (dist / iattenuation)) > 255) ? 255 : idx)])) < 0) ? 0 : tmp)
#define CALCSMPOFF(dist) (dist * dma.speed) >> ismpshift

void dmaHD_SpatializeOrigin_HHRTF(vec3_t so, channel_t* ch)
{
	// so = sound origin/[d]irection/[n]ormalized/[rot]ated/[d]irection [l]eft/[d]irection [r]ight
	vec3_t sod, sodl, sodr;
	// lo = listener origin/[l]eft/[r]ight
	vec3_t lol, lor;
    // distance to ears/[l]eft/[r]ight
	int distl, distr; // using int since calculations are integer based.
	// temp, index
	int tmp, idx;
	float t;

	int iattenuation = (dmaHD_inwater) ? 2 : 6;
	int ismpshift = (dmaHD_inwater) ? 19 : 17;

	// Increase attenuation for weapon sounds since they would be very loud!
	if (ch->thesfx && ch->thesfx->weaponsound) iattenuation *= 2;

	// Calculate sound direction.
	VectorSubtract(so, g_listener_origin, sod);
	// Rotate sound origin to listener axis
	VectorRotate(sod, g_listener_axis, ch->sodrot);

	// Origin for ears (~20cm apart)
	lol[0] = 0.0; lol[1] = 40; lol[2] = 0.0; // left
	lor[0] = 0.0; lor[1] = -40; lor[2] = 0.0; // right

	// Calculate sound direction.
	VectorSubtract(ch->sodrot, lol, sodl); // left
	VectorSubtract(ch->sodrot, lor, sodr); // right

	VectorNormalize(ch->sodrot);
	// Calculate length of sound origin direction vector.
	distl = (int)VectorNormalize(sodl); // left
	distr = (int)VectorNormalize(sodr); // right
	
	// Close enough to be at full volume?
	if (distl < 80) distl = 0; // left
	if (distr < 80) distr = 0; // right

	// Distance 384units = 1m
	// 340.29m/s (speed of sound at sea level)
	// Do surround effect with doppler.
	// 384.0 * 340.29 = 130671.36
	// Most similar is 2 ^ 17 = 131072; so shift right by 17 to divide by 131072

	// 1484m/s in water
	// 384.0 * 1484 = 569856
	// Most similar is 2 ^ 19 = 524288; so shift right by 19 to divide by 524288

	ch->l.offset = CALCSMPOFF(distl); // left
	ch->r.offset = CALCSMPOFF(distr); // right
	// Calculate volume at ears
	ch->l.bassvol = ch->l.vol = CALCVOL(distl); // left
	ch->r.bassvol = ch->r.vol = CALCVOL(distr); // right

	if (distl != 0 || distr != 0)
	{
		// Sound originating from inside head of left ear (i.e. from right)
		if (ch->sodrot[1] < 0) ch->l.vol *= (1.0 + (ch->sodrot[1] * 0.7f));
		// Sound originating from inside head of right ear (i.e. from left)
		if (ch->sodrot[1] > 0) ch->r.vol *= (1.0 - (ch->sodrot[1] * 0.7f));

		// Calculate HRTF function (lowpass filter) parameters
		//if (ch->fixed_origin)
		{
			// Sound originating from behind viewer
			if (ch->sodrot[0] < 0) 
			{
				ch->l.vol *= (1.0 + (ch->sodrot[0] * 0.05f));
				ch->r.vol *= (1.0 + (ch->sodrot[0] * 0.05f));
				// 2ms max
				//t = -ch->sodrot[0] * 0.04f; if (t > 0.005f) t = 0.005f;
				t = (dma.speed * 0.001f);
				ch->l.offset -= t;
				ch->r.offset += t;
			}
		}

		if (dmaHD_Mixer->integer == 10)
		{
			// Sound originating from above viewer (decrease bass)
			// Sound originating from below viewer (increase bass)
			ch->l.bassvol *= ((1 - ch->sodrot[2]) * 0.5);
			ch->r.bassvol *= ((1 - ch->sodrot[2]) * 0.5);
		}
	}
	// Normalize volume
	ch->l.vol *= 0.5;
	ch->r.vol *= 0.5;

	if (dmaHD_inwater)
	{
		// Keep bass in water.
		ch->l.vol *= 0.2;
		ch->r.vol *= 0.2;
	}
}

void dmaHD_SpatializeOrigin_dmaEX2(vec3_t so, channel_t* ch)
{
	// so = sound origin/[d]irection/[n]ormalized/[rot]ated
	vec3_t sod;
    // distance to head
	int dist; // using int since calculations are integer based.
	// temp, index
	int tmp, idx, vol;
	vec_t dot;

	int iattenuation = (dmaHD_inwater) ? 2 : 6;
	int ismpshift = (dmaHD_inwater) ? 19 : 17;

	// Increase attenuation for weapon sounds since they would be very loud!
	if (ch->thesfx && ch->thesfx->weaponsound) iattenuation *= 2;

	// Calculate sound direction.
	VectorSubtract(so, g_listener_origin, sod);
	// Rotate sound origin to listener axis
	VectorRotate(sod, g_listener_axis, ch->sodrot);

	VectorNormalize(ch->sodrot);
	// Calculate length of sound origin direction vector.
	dist = (int)VectorNormalize(sod); // left
	
	// Close enough to be at full volume?
	if (dist < 0) dist = 0; // left

	// Distance 384units = 1m
	// 340.29m/s (speed of sound at sea level)
	// Do surround effect with doppler.
	// 384.0 * 340.29 = 130671.36
	// Most similar is 2 ^ 17 = 131072; so shift right by 17 to divide by 131072

	// 1484m/s in water
	// 384.0 * 1484 = 569856
	// Most similar is 2 ^ 19 = 524288; so shift right by 19 to divide by 524288

	ch->l.offset = CALCSMPOFF(dist);
	// Calculate volume at ears
	vol = CALCVOL(dist);
	ch->l.vol = vol;
	ch->r.vol = vol;
	ch->l.bassvol = vol;

	dot = -ch->sodrot[1];
	ch->l.vol *= 0.5 * (1.0 - dot);
	ch->r.vol *= 0.5 * (1.0 + dot);

	// Calculate HRTF function (lowpass filter) parameters
	if (ch->fixed_origin)
	{
		// Reverberation
		dist += 768;
		ch->l.reverboffset = CALCSMPOFF(dist);
		vol = CALCVOL(dist);
		ch->l.reverbvol = vol;
		ch->r.reverbvol = vol;
		ch->l.reverbvol *= 0.5 * (1.0 + dot);
		ch->r.reverbvol *= 0.5 * (1.0 - dot);

		// Sound originating from behind viewer: decrease treble + reverb
		if (ch->sodrot[0] < 0) 
		{
			ch->l.vol *= (1.0 + (ch->sodrot[0] * 0.5));
			ch->r.vol *= (1.0 + (ch->sodrot[0] * 0.5));
		}
		else // from front...
		{
			// adjust reverb for each ear.
			if (ch->sodrot[1] < 0) ch->r.reverbvol = 0;
			else if (ch->sodrot[1] > 0) ch->l.reverbvol = 0;
		}
		
		// Sound originating from above viewer (decrease bass)
		// Sound originating from below viewer (increase bass)
		ch->l.bassvol *= ((1 - ch->sodrot[2]) * 0.5);
	}
	else
	{
		// Reduce base volume by half to keep overall valume.
		ch->l.bassvol *= 0.5;
	}

	if (dmaHD_inwater)
	{
		// Keep bass in water.
		ch->l.vol *= 0.2;
		ch->r.vol *= 0.2;
	}
}

void dmaHD_SpatializeOrigin_dmaEX(vec3_t origin, channel_t* ch)
{
    vec_t		dot;
    vec_t		dist;
    vec_t		lscale, rscale, scale;
    vec3_t		source_vec;
	int tmp;

	const float dist_mult = SOUND_ATTENUATE;
	
	// calculate stereo seperation and distance attenuation
	VectorSubtract(origin, g_listener_origin, source_vec);

	// VectorNormalize returns original length of vector and normalizes vector.
	dist = VectorNormalize(source_vec);
	dist -= SOUND_FULLVOLUME;
	if (dist < 0) dist = 0; // close enough to be at full volume
	dist *= dist_mult;		// different attenuation levels
	
	VectorRotate(source_vec, g_listener_axis, ch->sodrot);

	dot = -ch->sodrot[1];
	// DMAEX - Multiply by the stereo separation CVAR.
	dot *= dmaEX_StereoSeparation->value;

	rscale = 0.5 * (1.0 + dot);
	lscale = 0.5 * (1.0 - dot);
	if (rscale < 0) rscale = 0;
	if (lscale < 0) lscale = 0;

	// add in distance effect
	scale = (1.0 - dist) * rscale;
	tmp = (ch->master_vol * scale);
	if (tmp < 0) tmp = 0;
	ch->r.vol = tmp;

	scale = (1.0 - dist) * lscale;
	tmp = (ch->master_vol * scale);
	if (tmp < 0) tmp = 0;
	ch->l.vol = tmp;
}

void dmaHD_SpatializeOrigin(vec3_t so, channel_t* ch)
{
	switch(dmaHD_Mixer->integer)
	{
	// HHRTF
	case 10:
	case 11: dmaHD_SpatializeOrigin_HHRTF(so, ch); break;
	// dmaEX2
	case 20:
	case 21: dmaHD_SpatializeOrigin_dmaEX2(so, ch); break;
	// dmaEX
	case 30: dmaHD_SpatializeOrigin_dmaEX(so, ch); break;
	}
}

/*
==============================================================
continuous looping sounds are added each frame
==============================================================
*/

/*
==================
dmaHD_AddLoopSounds

Spatialize all of the looping sounds.
All sounds are on the same cycle, so any duplicates can just
sum up the channel multipliers.
==================
*/
void dmaHD_AddLoopSounds (void) 
{
	int			i, time;
	channel_t	*ch;
	loopSound_t	*loop;
	static int	loopFrame;

	numLoopChannels = 0;

	time = Com_Milliseconds();

	loopFrame++;
	//#pragma omp parallel for private(loop, ch)
	for (i = 0 ; i < MAX_GENTITIES; i++) 
	{
		if (numLoopChannels >= MAX_CHANNELS) continue;
		
		loop = &loopSounds[i];
		// already merged into an earlier sound
		if (!loop->active || loop->mergeFrame == loopFrame) continue;

		// allocate a channel
		ch = &loop_channels[numLoopChannels];

		dmaHD_SpatializeReset(ch);
		ch->fixed_origin = qtrue;
		ch->master_vol = (loop->kill) ? 127 : 90; // 3D / Sphere
		dmaHD_SpatializeOrigin(loop->origin, ch);

		loop->sfx->lastTimeUsed = time;

		ch->master_vol = 127;
		// Clip volumes.
		ch->l.vol = VOLCLAMP(ch->l.vol);
		ch->r.vol = VOLCLAMP(ch->r.vol);
		ch->l.bassvol = VOLCLAMP(ch->l.bassvol);
		ch->r.bassvol = VOLCLAMP(ch->r.bassvol);
		ch->thesfx = loop->sfx;
		ch->doppler = qfalse;
		
		//#pragma omp critical
		{
			numLoopChannels++;
		}
	}
}

//=============================================================================

/*
============
dmaHD_Respatialize

Change the volumes of all the playing sounds for changes in their positions
============
*/
void dmaHD_Respatialize( int entityNum, const vec3_t head, vec3_t axis[3], int inwater )
{
	int			i;
	channel_t	*ch;
	vec3_t		origin;

	if (!s_soundStarted || s_soundMuted) return;

	dmaHD_inwater = inwater;

	listener_number = entityNum;
	VectorCopy(head, g_listener_origin);
	VectorCopy(axis[0], g_listener_axis[0]);
	VectorCopy(axis[1], g_listener_axis[1]);
	VectorCopy(axis[2], g_listener_axis[2]);

	// update spatialization for dynamic sounds	
	//#pragma omp parallel for private(ch)
	for (i = 0 ; i < MAX_CHANNELS; i++) 
	{
		ch = &s_channels[i];
		if (!ch->thesfx) continue;
		
		dmaHD_SpatializeReset(ch);
		// Anything coming from the view entity will always be full volume
		if (ch->entnum == listener_number) 
		{
			ch->l.vol = ch->master_vol;
			ch->r.vol = ch->master_vol;
			ch->l.bassvol = ch->master_vol;
			ch->r.bassvol = ch->master_vol;
			switch(dmaHD_Mixer->integer)
			{
				case 10: case 11: case 20: case 21:
					if (dmaHD_inwater)
					{
						ch->l.vol *= 0.2;
						ch->r.vol *= 0.2;
					}
					break;
			}
		} 
		else 
		{
			if (ch->fixed_origin) { VectorCopy( ch->origin, origin ); }
			else { VectorCopy( loopSounds[ ch->entnum ].origin, origin ); }

			dmaHD_SpatializeOrigin(origin, ch);
		}
	}

	// add loopsounds
	dmaHD_AddLoopSounds();
}

/*
============
dmaHD_Update

Called once each time through the main loop
============
*/
void dmaHD_Update( void ) 
{
	if (!s_soundStarted || s_soundMuted) return;
	// add raw data from streamed samples
	S_UpdateBackgroundTrack();
	// mix some sound
	dmaHD_Update_Mix();
}

void dmaHD_Update_Mix(void) 
{
	unsigned endtime;
	int samps;
	static int lastTime = 0.0f;
	int mixahead, op, thisTime, sane;
	static int lastsoundtime = -1;

	if (!s_soundStarted || s_soundMuted) return;
	
	thisTime = Com_Milliseconds();

	// Updates s_soundtime
	S_GetSoundtime();

	if (s_soundtime <= lastsoundtime) return;
	lastsoundtime = s_soundtime;

	// clear any sound effects that end before the current time,
	// and start any new sounds
	S_ScanChannelStarts();

	if ((sane = thisTime - lastTime) < 8) sane = 8; // ms since last mix (cap to 8ms @ 125fps)
	op = (int)((float)(dma.speed * sane) * 0.001); // samples to mix based on last mix time
	mixahead = (int)((float)dma.speed * s_mixahead->value);
	
	if (mixahead < op) mixahead = op;
	
	// mix ahead of current position
	endtime = s_soundtime + mixahead;

	// never mix more than the complete buffer
	samps = dma.samples >> (dma.channels-1);
	if ((endtime - s_soundtime) > samps) endtime = (s_soundtime + samps);

	SNDDMA_BeginPainting ();

	dmaHD_PaintChannels (endtime);

	SNDDMA_Submit ();

	lastTime = thisTime;
}

/*
================
dmaHD_Enabled
================
*/
qboolean dmaHD_Enabled(void) 
{
	if (dmaHD_Enable == NULL)
		dmaHD_Enable = Cvar_Get("dmaHD_enable", "1", CVAR_ARCHIVE); //@p5yc0runn3r- Turn on by default

	return (dmaHD_Enable->integer);
}

// ====================================================================
// User-setable variables
// ====================================================================
void dmaHD_SoundInfo(void) 
{	
	Com_Printf("\n" );
	Com_Printf("dmaHD 3D software sound engine by p5yc0runn3r\n");
	
	if (!s_soundStarted) 
	{
		Com_Printf (" Engine not started.\n");
	} 
	else 
	{
		switch (dmaHD_Mixer->integer)
		{
			case 10: Com_Printf(" dmaHD full 3D sound mixer [10]\n"); break;
			case 11: Com_Printf(" dmaHD planar 3D sound mixer [11]\n"); break;
			case 20: Com_Printf(" dmaEX2 sound mixer [20]\n"); break;
			case 21: Com_Printf(" dmaEX2 sound mixer with no reverb [21]\n"); break;
			case 30: Com_Printf(" dmaEX sound mixer [30]\n"); break;
		}
		Com_Printf(" %d ch / %d Hz / %d bps\n", dma.channels, dma.speed, dma.samplebits);
		if (s_numSfx > 0 || g_dmaHD_allocatedsoundmemory > 0)
		{
			Com_Printf(" %d sounds in %.2f MiB\n", s_numSfx, (float)g_dmaHD_allocatedsoundmemory / 1048576.0f);
		}
		else
		{
			Com_Printf(" No sounds loaded yet.\n");
		}
	}
	Com_Printf("\n" );
}

void dmaHD_SoundList(void) 
{
	int i;
	sfx_t *sfx;
	
	Com_Printf("\n" );
	Com_Printf("dmaHD HRTF sound engine by p5yc0runn3r\n");

	if (s_numSfx > 0 || g_dmaHD_allocatedsoundmemory > 0)
	{
		for (sfx = s_knownSfx, i = 0; i < s_numSfx; i++, sfx++)
		{
			Com_Printf(" %s %.2f KiB %s\n", 
				sfx->soundName, (float)sfx->soundLength / 1024.0f, 
				(sfx->inMemory ? "" : "!"));
		}
		Com_Printf(" %d sounds in %.2f MiB\n", s_numSfx, (float)g_dmaHD_allocatedsoundmemory / 1048576.0f);
	}
	else
	{
		Com_Printf(" No sounds loaded yet.\n");
	}
	Com_Printf("\n" );
}


/*
================
dmaHD_Init
================
*/
qboolean dmaHD_Init(soundInterface_t *si) 
{
	if (!si) return qfalse;

	// Return if not enabled.
	if (!dmaHD_Enabled()) return qtrue;

	dmaHD_Mixer = Cvar_Get("dmaHD_mixer", "10", CVAR_ARCHIVE);
	if (dmaHD_Mixer->integer != 10 && 
		dmaHD_Mixer->integer != 11 &&
		dmaHD_Mixer->integer != 20 && 
		dmaHD_Mixer->integer != 21 &&
		dmaHD_Mixer->integer != 30)
	{
		Cvar_Set("dmaHD_Mixer", "10");
		dmaHD_Mixer = Cvar_Get("dmaHD_mixer", "10", CVAR_ARCHIVE);
	}

	dmaEX_StereoSeparation = Cvar_Get("dmaEX_StereoSeparation", "0.9", CVAR_ARCHIVE);
	if (dmaEX_StereoSeparation->value < 0.1) 
	{
		Cvar_Set("dmaEX_StereoSeparation", "0.1");
		dmaEX_StereoSeparation = Cvar_Get("dmaEX_StereoSeparation", "0.9", CVAR_ARCHIVE);
	}
	else if (dmaEX_StereoSeparation->value > 2.0) 
	{
		Cvar_Set("dmaEX_StereoSeparation", "2.0");
		dmaEX_StereoSeparation = Cvar_Get("dmaEX_StereoSeparation", "0.9", CVAR_ARCHIVE);
	}

	dmaHD_Interpolation = Cvar_Get("dmaHD_interpolation", "3", CVAR_ARCHIVE);
	if (dmaHD_Interpolation->integer == 0)
	{
		dmaHD_GetInterpolatedSample = dmaHD_GetNoInterpolationSample;
	}
	else if (dmaHD_Interpolation->integer == 1)
	{
		dmaHD_GetInterpolatedSample = dmaHD_GetInterpolatedSampleLinear;
	}
	else if (dmaHD_Interpolation->integer == 2)
	{
		dmaHD_GetInterpolatedSample = dmaHD_GetInterpolatedSampleCubic;
	}
	else //if (dmaHD_Interpolation->integer == 3) // DEFAULT
	{
		dmaHD_GetInterpolatedSample = dmaHD_GetInterpolatedSampleHermite4pt3oX;
	}

	dmaHD_InitTables();

	// Override function pointers to dmaHD version, the rest keep base.
	si->SoundInfo = dmaHD_SoundInfo;
	si->Respatialize = dmaHD_Respatialize;
	si->Update = dmaHD_Update;
	si->SoundList = dmaHD_SoundList;

	return qtrue;
}

#endif//NO_DMAHD
