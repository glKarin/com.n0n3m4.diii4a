/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/
// snd_mem.c: sound caching

#include "quakedef.h"

#include "winquake.h"
#include "fs.h"

typedef struct
{
	int		format;
	int		rate;
	int		bitwidth;
	int		numchannels;
	int		loopstart;
	int		samples;
	int		dataofs;		// chunk starts this many bytes from file start
} wavinfo_t;

static wavinfo_t GetWavinfo (char *name, qbyte *wav, int wavlength);

#define LINEARUPSCALE(in, inrate, insamps, out, outrate, outlshift, outrshift) \
	{ \
		scale = inrate / (double)outrate; \
		infrac = floor(scale * 65536); \
		outsamps = insamps / scale; \
		inaccum = 0; \
		outnlsamps = floor(1.0 / scale); \
		outsamps -= outnlsamps; \
	\
		while (outsamps) \
		{ \
			*out = ((0xFFFF - inaccum)*in[0] + inaccum*in[1]) >> (16 - outlshift + outrshift); \
			inaccum += infrac; \
			in += (inaccum >> 16); \
			inaccum &= 0xFFFF; \
			out++; \
			outsamps--; \
		} \
		while (outnlsamps) \
		{ \
			*out = (*in >> outrshift) << outlshift; \
			out++; \
			outnlsamps--; \
		} \
	}

#define LINEARUPSCALESTEREO(in, inrate, insamps, out, outrate, outlshift, outrshift) \
	{ \
		scale = inrate / (double)outrate; \
		infrac = floor(scale * 65536); \
		outsamps = insamps / scale; \
		inaccum = 0; \
		outnlsamps = floor(1.0 / scale); \
		outsamps -= outnlsamps; \
	\
		while (outsamps) \
		{ \
			out[0] = ((0xFFFF - inaccum)*in[0] + inaccum*in[2]) >> (16 - outlshift + outrshift); \
			out[1] = ((0xFFFF - inaccum)*in[1] + inaccum*in[3]) >> (16 - outlshift + outrshift); \
			inaccum += infrac; \
			in += (inaccum >> 16) * 2; \
			inaccum &= 0xFFFF; \
			out += 2; \
			outsamps--; \
		} \
		while (outnlsamps) \
		{ \
			out[0] = (in[0] >> outrshift) << outlshift; \
			out[1] = (in[1] >> outrshift) << outlshift; \
			out += 2; \
			outnlsamps--; \
		} \
	}

#define LINEARUPSCALESTEREOTOMONO(in, inrate, insamps, out, outrate, outlshift, outrshift) \
	{ \
		scale = inrate / (double)outrate; \
		infrac = floor(scale * 65536); \
		outsamps = insamps / scale; \
		inaccum = 0; \
		outnlsamps = floor(1.0 / scale); \
		outsamps -= outnlsamps; \
	\
		while (outsamps) \
		{ \
			*out = ((((0xFFFF - inaccum)*in[0] + inaccum*in[2]) >> (16 - outlshift + outrshift)) + \
				(((0xFFFF - inaccum)*in[1] + inaccum*in[3]) >> (16 - outlshift + outrshift))) >> 1; \
			inaccum += infrac; \
			in += (inaccum >> 16) * 2; \
			inaccum &= 0xFFFF; \
			out++; \
			outsamps--; \
		} \
		while (outnlsamps) \
		{ \
			out[0] = (((in[0] >> outrshift) << outlshift) + ((in[1] >> outrshift) << outlshift)) >> 1; \
			out++; \
			outnlsamps--; \
		} \
	}

#define LINEARDOWNSCALE(in, inrate, insamps, out, outrate, outlshift, outrshift) \
	{ \
		scale = outrate / (double)inrate; \
		infrac = floor(scale * 65536); \
		inaccum = 0; \
		insamps--; \
		outsampleft = 0; \
	\
		while (insamps) \
		{ \
			inaccum += infrac; \
			if (inaccum >> 16) \
			{ \
				inaccum &= 0xFFFF; \
				outsampleft += (infrac - inaccum) * (*in); \
				*out = outsampleft >> (16 - outlshift + outrshift); \
				out++; \
				outsampleft = inaccum * (*in); \
			} \
			else \
				outsampleft += infrac * (*in); \
			in++; \
			insamps--; \
		} \
		outsampleft += (0xFFFF - inaccum) * (*in);\
		*out = outsampleft >> (16 - outlshift + outrshift); \
	}

#define LINEARDOWNSCALESTEREO(in, inrate, insamps, out, outrate, outlshift, outrshift) \
	{ \
		scale = outrate / (double)inrate; \
		infrac = floor(scale * 65536); \
		inaccum = 0; \
		insamps--; \
		outsampleft = 0; \
		outsampright = 0; \
	\
		while (insamps) \
		{ \
			inaccum += infrac; \
			if (inaccum >> 16) \
			{ \
				inaccum &= 0xFFFF; \
				outsampleft += (infrac - inaccum) * in[0]; \
				outsampright += (infrac - inaccum) * in[1]; \
				out[0] = outsampleft >> (16 - outlshift + outrshift); \
				out[1] = outsampright >> (16 - outlshift + outrshift); \
				out += 2; \
				outsampleft = inaccum * in[0]; \
				outsampright = inaccum * in[1]; \
			} \
			else \
			{ \
				outsampleft += infrac * in[0]; \
				outsampright += infrac * in[1]; \
			} \
			in += 2; \
			insamps--; \
		} \
		outsampleft += (0xFFFF - inaccum) * in[0];\
		outsampright += (0xFFFF - inaccum) * in[1];\
		out[0] = outsampleft >> (16 - outlshift + outrshift); \
		out[1] = outsampright >> (16 - outlshift + outrshift); \
	}

#define LINEARDOWNSCALESTEREOTOMONO(in, inrate, insamps, out, outrate, outlshift, outrshift) \
	{ \
		scale = outrate / (double)inrate; \
		infrac = floor(scale * 65536); \
		inaccum = 0; \
		insamps--; \
		outsampleft = 0; \
	\
		while (insamps) \
		{ \
			inaccum += infrac; \
			if (inaccum >> 16) \
			{ \
				inaccum &= 0xFFFF; \
				outsampleft += (infrac - inaccum) * ((in[0] + in[1]) >> 1); \
				*out = outsampleft >> (16 - outlshift + outrshift); \
				out++; \
				outsampleft = inaccum * ((in[0] + in[1]) >> 1); \
			} \
			else \
				outsampleft += infrac * ((in[0] + in[1]) >> 1); \
			in += 2; \
			insamps--; \
		} \
		outsampleft += (0xFFFF - inaccum) * ((in[0] + in[1]) >> 1);\
		*out = outsampleft >> (16 - outlshift + outrshift); \
	}

#define STANDARDRESCALE(in, inrate, insamps, out, outrate, outlshift, outrshift) \
	{ \
		scale = inrate / (double)outrate; \
		infrac = floor(scale * 65536); \
		outsamps = insamps / scale; \
		inaccum = 0; \
	\
		while (outsamps) \
		{ \
			*out = (*in >> outrshift) << outlshift; \
			inaccum += infrac; \
			in += (inaccum >> 16); \
			inaccum &= 0xFFFF; \
			out++; \
			outsamps--; \
		} \
	}

#define STANDARDRESCALESTEREO(in, inrate, insamps, out, outrate, outlshift, outrshift) \
	{ \
		scale = inrate / (double)outrate; \
		infrac = floor(scale * 65536); \
		outsamps = insamps / scale; \
		inaccum = 0; \
	\
		while (outsamps) \
		{ \
			out[0] = (in[0] >> outrshift) << outlshift; \
			out[1] = (in[1] >> outrshift) << outlshift; \
			inaccum += infrac; \
			in += (inaccum >> 16) * 2; \
			inaccum &= 0xFFFF; \
			out += 2; \
			outsamps--; \
		} \
	}

#define STANDARDRESCALESTEREOTOMONO(in, inrate, insamps, out, outrate, outlshift, outrshift) \
	{ \
		scale = inrate / (double)outrate; \
		infrac = floor(scale * 65536); \
		outsamps = insamps / scale; \
		inaccum = 0; \
	\
		while (outsamps) \
		{ \
			out[0] = (((in[0] >> outrshift) << outlshift) + ((in[1] >> outrshift) << outlshift)) >> 1; \
			inaccum += infrac; \
			in += (inaccum >> 16) * 2; \
			inaccum &= 0xFFFF; \
			out++; \
			outsamps--; \
		} \
	}

#define QUICKCONVERT(in, insamps, out, outlshift, outrshift) \
	{ \
		while (insamps) \
		{ \
			*out = (*in >> outrshift) << outlshift; \
			out++; \
			in++; \
			insamps--; \
		} \
	}

#define QUICKCONVERTSTEREOTOMONO(in, insamps, out, outlshift, outrshift) \
	{ \
		while (insamps) \
		{ \
			*out = (((in[0] >> outrshift) << outlshift) + ((in[1] >> outrshift) << outlshift)) >> 1; \
			out++; \
			in += 2; \
			insamps--; \
		} \
	}

// SND_ResampleStream: takes a sound stream and converts with given parameters. Limited to
// 8-16-bit signed conversions and mono-to-mono/stereo-to-stereo conversions.
// Not an in-place algorithm.
void SND_ResampleStream (const void *in, int inrate, qaudiofmt_t informat, int inchannels, int insamps, void *out, int outrate, qaudiofmt_t outformat, int outchannels, int resampstyle)
{
	double scale;
	const signed char *in8 = (const signed char *)in;
	const short *in16 = (const short *)in;
	signed char *out8 = (signed char *)out;
	short *out16 = (short *)out;
	int outsamps, outnlsamps, outsampleft, outsampright;
	int infrac, inaccum;

	if (insamps <= 0)
		return;

	if (inchannels == outchannels && informat == outformat && inrate == outrate)
	{
		memcpy(out, in, informat*insamps*inchannels);
		return;
	}

	if (inchannels == 1 && outchannels == 1)
	{
		if (informat == QAF_S8)
		{
			if (outformat == QAF_S8)
			{
				if (inrate < outrate) // upsample
				{
					if (resampstyle)
						LINEARUPSCALE(in8, inrate, insamps, out8, outrate, 0, 0)
					else
						STANDARDRESCALE(in8, inrate, insamps, out8, outrate, 0, 0)
				}
				else // downsample
				{
					if (resampstyle > 1)
						LINEARDOWNSCALE(in8, inrate, insamps, out8, outrate, 0, 0)
					else
						STANDARDRESCALE(in8, inrate, insamps, out8, outrate, 0, 0)
				}
				return;
			}
			else if (outformat == QAF_S16)
			{
				if (inrate == outrate) // quick convert
					QUICKCONVERT(in8, insamps, out16, 8, 0)
				else if (inrate < outrate) // upsample
				{
					if (resampstyle)
						LINEARUPSCALE(in8, inrate, insamps, out16, outrate, 8, 0)
					else
						STANDARDRESCALE(in8, inrate, insamps, out16, outrate, 8, 0)
				}
				else // downsample
				{
					if (resampstyle > 1)
						LINEARDOWNSCALE(in8, inrate, insamps, out16, outrate, 8, 0)
					else
						STANDARDRESCALE(in8, inrate, insamps, out16, outrate, 8, 0)
				}
				return;
			}
		}
		else if (informat == QAF_S16) // 16-bit
		{
			if (outformat == QAF_S16)
			{
				if (inrate < outrate) // upsample
				{
					if (resampstyle)
						LINEARUPSCALE(in16, inrate, insamps, out16, outrate, 0, 0)
					else
						STANDARDRESCALE(in16, inrate, insamps, out16, outrate, 0, 0)
				}
				else // downsample
				{
					if (resampstyle > 1)
						LINEARDOWNSCALE(in16, inrate, insamps, out16, outrate, 0, 0)
					else
						STANDARDRESCALE(in16, inrate, insamps, out16, outrate, 0, 0)
				}
				return;
			}
			else if (outformat == QAF_S8)
			{
				if (inrate == outrate) // quick convert
					QUICKCONVERT(in16, insamps, out8, 0, 8)
				else if (inrate < outrate) // upsample
				{
					if (resampstyle)
						LINEARUPSCALE(in16, inrate, insamps, out8, outrate, 0, 8)
					else
						STANDARDRESCALE(in16, inrate, insamps, out8, outrate, 0, 8)
				}
				else // downsample
				{
					if (resampstyle > 1)
						LINEARDOWNSCALE(in16, inrate, insamps, out8, outrate, 0, 8)
					else
						STANDARDRESCALE(in16, inrate, insamps, out8, outrate, 0, 8)
				}
				return;
			}
		}
	}
	else if (outchannels == 2 && inchannels == 2)
	{
		if (informat == QAF_S8)
		{
			if (outformat == QAF_S8)
			{
				if (inrate < outrate) // upsample
				{
					if (resampstyle)
						LINEARUPSCALESTEREO(in8, inrate, insamps, out8, outrate, 0, 0)
					else
						STANDARDRESCALESTEREO(in8, inrate, insamps, out8, outrate, 0, 0)
				}
				else // downsample
				{
					if (resampstyle > 1)
						LINEARDOWNSCALESTEREO(in8, inrate, insamps, out8, outrate, 0, 0)
					else
						STANDARDRESCALESTEREO(in8, inrate, insamps, out8, outrate, 0, 0)
				}
				return;
			}
			else
			{
				if (inrate == outrate) // quick convert
				{
					insamps *= 2;
					QUICKCONVERT(in8, insamps, out16, 8, 0)
				}
				else if (inrate < outrate) // upsample
				{
					if (resampstyle)
						LINEARUPSCALESTEREO(in8, inrate, insamps, out16, outrate, 8, 0)
					else
						STANDARDRESCALESTEREO(in8, inrate, insamps, out16, outrate, 8, 0)
				}
				else // downsample
				{
					if (resampstyle > 1)
						LINEARDOWNSCALESTEREO(in8, inrate, insamps, out16, outrate, 8, 0)
					else
						STANDARDRESCALESTEREO(in8, inrate, insamps, out16, outrate, 8, 0)
				}
			}
		}
		else if (informat == QAF_S16) // 16-bit
		{
			if (outformat == QAF_S16)
			{
				if (inrate < outrate) // upsample
				{
					if (resampstyle)
						LINEARUPSCALESTEREO(in16, inrate, insamps, out16, outrate, 0, 0)
					else
						STANDARDRESCALESTEREO(in16, inrate, insamps, out16, outrate, 0, 0)
				}
				else // downsample
				{
					if (resampstyle > 1)
						LINEARDOWNSCALESTEREO(in16, inrate, insamps, out16, outrate, 0, 0)
					else
						STANDARDRESCALESTEREO(in16, inrate, insamps, out16, outrate, 0, 0)
				}
			}
			else if (outformat == QAF_S8)
			{
				if (inrate == outrate) // quick convert
				{
					insamps *= 2;
					QUICKCONVERT(in16, insamps, out8, 0, 8)
				}
				else if (inrate < outrate) // upsample
				{
					if (resampstyle)
						LINEARUPSCALESTEREO(in16, inrate, insamps, out8, outrate, 0, 8)
					else
						STANDARDRESCALESTEREO(in16, inrate, insamps, out8, outrate, 0, 8)
				}
				else // downsample
				{
					if (resampstyle > 1)
						LINEARDOWNSCALESTEREO(in16, inrate, insamps, out8, outrate, 0, 8)
					else
						STANDARDRESCALESTEREO(in16, inrate, insamps, out8, outrate, 0, 8)
				}
			}
		}
	}
#if 0
	else if (outchannels == 1 && inchannels == 2)
	{
		if (informat == QAF_S8)
		{
			if (outformat == QAF_S8)
			{
				if (inrate < outrate) // upsample
				{
					if (resampstyle)
						LINEARUPSCALESTEREOTOMONO(in8, inrate, insamps, out8, outrate, 0, 0)
					else
						STANDARDRESCALESTEREOTOMONO(in8, inrate, insamps, out8, outrate, 0, 0)
				}
				else // downsample
					STANDARDRESCALESTEREOTOMONO(in8, inrate, insamps, out8, outrate, 0, 0)
			}
			else if (outformat == QAF_S16)
			{
				if (inrate == outrate) // quick convert
					QUICKCONVERTSTEREOTOMONO(in8, insamps, out16, 8, 0)
				else if (inrate < outrate) // upsample
				{
					if (resampstyle)
						LINEARUPSCALESTEREOTOMONO(in8, inrate, insamps, out16, outrate, 8, 0)
					else
						STANDARDRESCALESTEREOTOMONO(in8, inrate, insamps, out16, outrate, 8, 0)
				}
				else // downsample
					STANDARDRESCALESTEREOTOMONO(in8, inrate, insamps, out16, outrate, 8, 0)
			}
		}
		else if (informat == QAF_S16) // 16-bit
		{
			if (outformat == QAF_S16)
			{
				if (inrate < outrate) // upsample
				{
					if (resampstyle)
						LINEARUPSCALESTEREOTOMONO(in16, inrate, insamps, out16, outrate, 0, 0)
					else
						STANDARDRESCALESTEREOTOMONO(in16, inrate, insamps, out16, outrate, 0, 0)
				}
				else // downsample
					STANDARDRESCALESTEREOTOMONO(in16, inrate, insamps, out16, outrate, 0, 0)
			}
			else if (outformat == QAF_S8)
			{
				if (inrate == outrate) // quick convert
					QUICKCONVERTSTEREOTOMONO(in16, insamps, out8, 0, 8)
				else if (inrate < outrate) // upsample
				{
					if (resampstyle)
						LINEARUPSCALESTEREOTOMONO(in16, inrate, insamps, out8, outrate, 0, 8)
					else
						STANDARDRESCALESTEREOTOMONO(in16, inrate, insamps, out8, outrate, 0, 8)
				}
				else // downsample
					STANDARDRESCALESTEREOTOMONO(in16, inrate, insamps, out8, outrate, 0, 8)
			}
		}
	}
#endif
}

/*
================
ResampleSfx
================
*/
static qboolean ResampleSfx (sfx_t *sfx, int inrate, int inchannels, qaudiofmt_t informat, int insamps, int inloopstart, qbyte *data)
{
	extern cvar_t snd_linearresample;
	extern cvar_t snd_loadasstereo;
	double scale;
	sfxcache_t	*sc;
	int outsamps;
	int len;
	qaudiofmt_t outformat;

	scale = snd_speed / (double)inrate;
	outsamps = insamps * scale;
	if (snd_loadas8bit.ival < 0)
		outformat = QAF_S16;
	else if (snd_loadas8bit.ival)
		outformat = QAF_S8;
	else
		outformat = informat;
	len = outsamps * QAF_BYTES(outformat) * inchannels;

	sfx->decoder.buf = sc = BZ_Malloc(sizeof(sfxcache_t) + len);
	if (!sc)
	{
		return false;
	}

	sc->numchannels = inchannels;
	sc->format = outformat;
	sc->speed = snd_speed;
	sc->length = outsamps;
	sc->soundoffset = 0;
	sc->data = (qbyte*)(sc+1);
	if (inloopstart == -1)
		sfx->loopstart = inloopstart;
	else
		sfx->loopstart = inloopstart * scale;

	SND_ResampleStream (data,
		inrate,
		informat,
		inchannels,
		insamps,
		sc->data,
		sc->speed,
		sc->format,
		sc->numchannels,
		snd_linearresample.ival);

	if (inchannels == 1 && snd_loadasstereo.ival)
	{	//I'm implementing this to work around what looks like a firefox bug, where mono buffers don't get played (but stereo works just fine despite all the spacialisation issues associated with that).
		sfxcache_t *nc = sfx->decoder.buf = BZ_Malloc(sizeof(sfxcache_t) + len*2);
		*nc = *sc;
		nc->data = (qbyte*)(nc+1);
		SND_ResampleStream (sc->data,
			sc->speed,
			sc->format,
			sc->numchannels,
			outsamps,
			nc->data,
			nc->speed*2,
			nc->format,
			nc->numchannels,
			false);
		nc->numchannels *= 2;
		BZ_Free(sc);
	}

	return true;
}

//=============================================================================
#ifdef PACKAGE_DOOMWAD
#define DSPK_RATE 140
#define DSPK_BASE 170.0
#define DSPK_EXP 0.0433

/*
qboolean QDECL S_LoadDoomSpeakerSound (sfx_t *s, qbyte *data, size_t datalen, int sndspeed, qboolean forcedecode)
{
	sfxcache_t	*sc;

	// format data from Unofficial Doom Specs v1.6
	unsigned short *dataus;
	int samples, len, inrate, inaccum;
	qbyte *outdata;
	qbyte towrite;
	double timeraccum, timerfreq;

	if (datalen < 4)
		return NULL;

	dataus = (unsigned short*)data;

	if (LittleShort(dataus[0]) != 0)
		return NULL;

	samples = LittleShort(dataus[1]);

	data += 4;
	datalen -= 4;

	if (datalen != samples)
		return NULL;

	len = (int)((double)samples * (double)snd_speed / DSPK_RATE);

	sc = Cache_Alloc (&s->cache, len + sizeof(sfxcache_t), s->name);
	if (!sc)
	{
		return NULL;
	}

	sc->length = len;
	s->loopstart = -1;
	sc->numchannels = 1;
	sc->width = 1;
	sc->speed = snd_speed;

	timeraccum = 0;
	outdata = sc->data;
	towrite = 0x40;
	inrate = (int)((double)snd_speed / DSPK_RATE);
	inaccum = inrate;
	if (*data)
		timerfreq = DSPK_BASE * pow((double)2.0, DSPK_EXP * (*data));
	else
		timerfreq = 0;

	while (len > 0)
	{
		timeraccum += timerfreq;
		if (timeraccum > (float)snd_speed)
		{
			towrite ^= 0xFF; // swap speaker component
			timeraccum -= (float)snd_speed;
		}

		inaccum--;
		if (!inaccum)
		{
			data++;
			if (*data)
				timerfreq = DSPK_BASE * pow((double)2.0, DSPK_EXP * (*data));
			inaccum = inrate;
		}
		*outdata = towrite;
		outdata++;
		len--;
	}

	return sc;
}
*/
static qboolean QDECL S_LoadDoomSound (sfx_t *s, qbyte *data, size_t datalen, int sndspeed, qboolean forcedecode)
{
	// format data from Unofficial Doom Specs v1.6
	unsigned short *dataus;
	int samples, rate;

	if (datalen < 8)
		return false;

	dataus = (unsigned short*)data;

	if (LittleShort(dataus[0]) != 3)
		return false;

	rate = LittleShort(dataus[1]);
	samples = LittleShort(dataus[2]);

	data += 8;
	datalen -= 8;

	if (datalen != samples)
		return false;

	COM_CharBias(data, datalen);

	return ResampleSfx (s, rate, 1, 1, samples, -1, data);
}
#endif

static qboolean QDECL S_LoadWavSound (sfx_t *s, qbyte *data, size_t datalen, int sndspeed, qboolean forcedecode)
{
	wavinfo_t	info;
	qaudiofmt_t	format;

	if (datalen < 4 || strncmp(data, "RIFF", 4))
		return false;

	info = GetWavinfo (s->name, data, datalen);
	if (info.numchannels < 1 || info.numchannels > 2)
	{
		s->loadstate = SLS_FAILED;
		Con_Printf ("%s has an unsupported quantity of channels.\n",s->name);
		return false;
	}

	if (info.format == 1 && info.bitwidth == 8)	//unsigned bytes
	{
		COM_CharBias(data + info.dataofs, info.samples*info.numchannels);
		format = QAF_S8;
	}
	else if (info.format == 1 && (info.bitwidth > 8 && info.bitwidth <= 16))	//signed shorts
	{
		COM_SwapLittleShortBlock((short *)(data + info.dataofs), info.samples*info.numchannels);
		format = QAF_S16;
	}
	else if (info.format == 1 && (info.bitwidth > 16 && info.bitwidth <= 24))
	{	//packed
		short *out = (short *)(data + info.dataofs);
		qbyte *in = (qbyte *)(data + info.dataofs);
		int s;
		size_t samples = info.samples*info.numchannels;
		while(samples --> 0)
		{
			s  = *in++<<0;
			s |= *in++<<8;
			s |= *in++<<16;
			s |= 0    <<24;
			*out++ = s>>8;	//just drop the least significant bits.
		}
		format = QAF_S16;
	}
	else if (info.format == 1 && (info.bitwidth > 24 && info.bitwidth <= 32))	//24(padded) or 32bit int audio
	{
		short *out = (short *)(data + info.dataofs);
		int *in = (int *)(data + info.dataofs);
		size_t samples = info.samples*info.numchannels;
		while(samples --> 0)
		{	//in place size conversion, so we need to do it forwards.
			*out++ = LittleLong(*in++)>>16;	//just drop the least significant bits.
		}
		format = QAF_S16;
	}
#ifdef MIXER_F32
	else if (info.format == 3 && info.bitwidth == 32)	//signed floats
	{
		if (bigendian)
		{
			size_t i = info.samples*info.numchannels;
			float *ptr = (float*)(data + info.dataofs);
			while(i --> 0)
				ptr[i] = LittleFloat(ptr[i]);
		}
		format = QAF_F32;
	}
	else if (info.format == 3 && info.bitwidth == 64)	//signed doubles, converted to floats cos doubles is just silly.
	{
		if (bigendian)
		{
			size_t i = info.samples*info.numchannels;
			qint64_t *in = (qint64_t*)(data + info.dataofs);
			float *out = (short *)(data + info.dataofs);
			union {
				qint64_t i;
				double d;
			} s;
			while(i --> 0)
			{
				s.i = LittleI64(in[i]);
				out[i] = s.d;
			}
		}
		format = QAF_F32;
	}
#else
	else if (info.format == 3 && info.bitwidth == 32)	//signed floats
	{
		short *out = (short *)(data + info.dataofs);
		float *in = (float *)(data + info.dataofs);
		size_t samples = info.samples*info.numchannels;
		int t;
		while(samples --> 0)
		{	//in place size conversion, so we need to do it forwards.
			t = LittleFloat(*in++) * 32767;
			t = bound(-32768, t, 32767);
			*out++ = t;
		}
		format = QAF_S16;
	}
	else if (info.format == 3 && info.bitwidth == 64)	//signed doubles
	{
		short *out = (short *)(data + info.dataofs);
		qint64_t *in = (qint64_t *)(data + info.dataofs);
		union {
			qint64_t i;
			double d;
		} s;
		size_t samples = info.samples*info.numchannels;
		int t;
		while(samples --> 0)
		{	//in place size conversion, so we need to do it forwards.
			s.i = LittleI64(*in++);
			t = s.d * 32767;
			t = bound(-32768, t, 32767);
			*out++ = t;
		}
		format = QAF_S16;
	}
#endif
	else
	{
		s->loadstate = SLS_FAILED;
		switch(info.format)
		{
		case 1/*WAVE_FORMAT_PCM*/:
		case 3/*WAVE_FORMAT_IEEE_FLOAT*/:		Con_Printf ("%s has an unsupported width (%i bits).\n", s->name, info.bitwidth); break;
		case 6/*WAVE_FORMAT_ALAW*/:				Con_Printf ("%s uses unsupported a-law format.\n", s->name); break;
		case 7/*WAVE_FORMAT_MULAW*/:			Con_Printf ("%s uses unsupported mu-law format.\n", s->name); break;
		case 0xfffe/*WAVE_FORMAT_EXTENSIBLE*/:
		default:								Con_Printf ("%s has an unsupported format (%#"PRIX16").\n", s->name, info.format); break;
		}
		return false;
	}

	return ResampleSfx (s, info.rate, info.numchannels, format, info.samples, info.loopstart, data + info.dataofs);
}

#ifdef FTE_TARGET_WEB
#if 1
void S_BrowserDecoded (void *ctx, void *dataptr, int frames, int channels, float rate)
{
	sfx_t *sfx = ctx;

	//make sure we were not restarting at the time... FIXME: make stricter?
	extern sfx_t		*known_sfx;
	extern int			num_sfx;
	int id = sfx-known_sfx;
	if (id < 0 || id >= num_sfx || sfx != &known_sfx[id])
		return; //err... don't crash out!

	sfx->loopstart = -1;
	if (dataptr)
	{	//okay, something loaded. woo.
		Z_Free(sfx->decoder.buf);
		sfx->decoder.buf = NULL;
		sfx->decoder.decodedata = NULL;
		ResampleSfx (sfx, rate, channels, QAF_S16, frames, -1, dataptr);
	}
	else
	{
		Con_Printf(CON_WARNING"Failed to decode %s\n", sfx->name);
		sfx->loadstate = SLS_FAILED;
	}
}
static qboolean QDECL S_LoadBrowserFile (sfx_t *s, qbyte *data, size_t datalen, int sndspeed, qboolean forcedecode)
{
	struct sfxcache_s *buf;

	if (datalen > 4 && !strncmp(data, "RIFF", 4))
		return false;	//do NOT use this code for wav files. we have no way to read the looping flags which would break things in certain situations. we MUST fall back on our normal loader.

	s->decoder.buf = buf = Z_Malloc(sizeof(*buf)+128);
	//fill with a placeholder
	buf->length = 128;
	buf->speed = snd_speed;
	buf->format = QAF_S8; //something basic
	buf->numchannels=1;
	buf->soundoffset = 0;
	buf->data = (qbyte*)(buf+1);

	s->loopstart = 0; //keep looping silence until it actually loads something.

	return emscriptenfte_pcm_loadaudiofile(s, S_BrowserDecoded, data, datalen, sndspeed);
}
#else
//web browsers contain their own decoding libraries that our openal stuff can use.
static qboolean QDECL S_LoadBrowserFile (sfx_t *s, qbyte *data, size_t datalen, int sndspeed, qboolean forcedecode)
{
	sfxcache_t *sc;
	s->decoder.buf = sc = BZ_Malloc(sizeof(sfxcache_t) + datalen);
	s->loopstart = -1;
	sc->data = (qbyte*)(sc+1);
	sc->length = datalen;
	sc->format = QAF_BLOB;	//ie: not pcm
	sc->speed = sndspeed;
	sc->numchannels = 2;
	sc->soundoffset = 0;
	memcpy(sc->data, data, datalen);

	return true;
}
#endif
#endif

qboolean QDECL S_LoadOVSound (sfx_t *s, qbyte *data, size_t datalen, int sndspeed, qboolean forcedecode);

//highest priority is last.
static struct
{
	S_LoadSound_t loadfunc;
	void *module;
} AudioInputPlugins[10] =
{
#ifdef FTE_TARGET_WEB
	{S_LoadBrowserFile},
#endif
#ifdef AVAIL_OGGVORBIS
	{S_LoadOVSound},
#endif
	{S_LoadWavSound},
#ifdef PACKAGE_DOOMWAD
	{S_LoadDoomSound},
//	{S_LoadDoomSpeakerSound},
#endif
};

qboolean S_RegisterSoundInputPlugin(void *module, S_LoadSound_t loadfnc)
{
	int i;
	for (i = 0; i < sizeof(AudioInputPlugins)/sizeof(AudioInputPlugins[0]); i++)
	{
		if (!AudioInputPlugins[i].loadfunc)
		{
			AudioInputPlugins[i].module = module;
			AudioInputPlugins[i].loadfunc = loadfnc;
			return true;
		}
	}
	return false;
}
void S_UnregisterSoundInputModule(void *module)
{	//unregister all sound handlers for the given module.
	int i;
	for (i = 0; i < sizeof(AudioInputPlugins)/sizeof(AudioInputPlugins[0]); i++)
	{
		if (AudioInputPlugins[i].module == module)
		{
			AudioInputPlugins[i].module = NULL;
			AudioInputPlugins[i].loadfunc = NULL;
		}
	}
}

static void S_LoadedOrFailed (void *ctx, void *ctxdata, size_t a, size_t b)
{
	sfx_t *s = ctx;
	s->loadstate = a;
}
/*
==============
S_LoadSound
==============
*/

static void S_LoadSoundWorker (void *ctx, void *ctxdata, size_t forcedecode, size_t b)
{
	sfx_t *s = ctx;
	char	namebuffer[256];
	qbyte	*data;
	int i;
	size_t result;
	char *name = s->name;
	size_t filesize;

	s->loopstart = -1;

	if (s->syspath)
	{
		vfsfile_t *f;

		if ((f = VFSOS_Open(name, "rb")))
		{
			filesize = VFS_GETLEN(f);
			data = BZ_Malloc (filesize);
			result = VFS_READ(f, data, filesize);

			if (result != filesize)
				Con_SafePrintf("S_LoadSound() fread: Filename: %s, expected %"PRIuSIZE", result was %"PRIuSIZE"\n", name, filesize, result);

			VFS_CLOSE(f);
		}
		else
		{
			Con_SafePrintf ("Couldn't load %s\n", namebuffer);
			COM_AddWork(WG_MAIN, S_LoadedOrFailed, s, NULL, SLS_FAILED, 0);
			return;
		}
	}
	else
	{
	//Con_Printf ("S_LoadSound: %x\n", (int)stackbuf);
	// load it in
		const char *prefixes[] = {"sound/", ""};
		const char *extensions[] = {
			".wav", 
#ifdef AVAIL_OGGOPUS
			".opus",
#endif
#ifdef AVAIL_OGGVORBIS
			".ogg",
#endif
		};
		char altname[sizeof(namebuffer)];
		char orig[16];
		size_t pre, ex;

		data = NULL;
		filesize = 0;
		if (*name == '*')	//q2 sexed sounds
		{
			//clq2_parsestartsound detects this also, and should not try playing these sounds.
			s->loadstate = SLS_FAILED;
			return;
		}

		for (pre = 0; !data && pre < countof(prefixes); pre++)
		{
			if (name[0] == '.' && name[1] == '.' && name[2] == '/')
			{	//someone's being specific. disable prefixes entirely.
				if (pre)
					break;
				//not relative to sound/
				Q_snprintfz(namebuffer, sizeof(namebuffer), "%s", name+3);
			}
			else
				Q_snprintfz(namebuffer, sizeof(namebuffer), "%s%s", prefixes[pre], name);

			data = FS_LoadMallocFile(namebuffer, &filesize);
			if (data)
				break;
			COM_FileExtension(namebuffer, orig, sizeof(orig));
			COM_StripExtension(namebuffer, altname, sizeof(altname));
			for (ex = 0; ex < countof(extensions); ex++)
			{
				if (!strcmp(orig, extensions[ex]+1))
					continue;
				Q_snprintfz(namebuffer, sizeof(namebuffer), "%s%s", altname, extensions[ex]);
				data = FS_LoadMallocFile(namebuffer, &filesize);
				if (data)
				{
					static float throttletimer;
					Con_ThrottlePrintf(&throttletimer, 1, "S_LoadSound: %s%s requested, but could only find %s\n", prefixes[pre], name, namebuffer);
					break;
				}
			}
		}

		if (data && !Ruleset_FileLoaded(name, data, filesize))
		{
			BZ_Free(data);
			data = NULL;
			filesize = 0;
		}
	}

	if (!data)
	{
		//FIXME: check to see if queued for download.
		if (name[0] == '.' && name[1] == '.' && name[2] == '/')
			Con_DPrintf ("Couldn't load %s\n", name+3);
		else
			Con_DPrintf ("Couldn't load sound/%s\n", name);
		COM_AddWork(WG_MAIN, S_LoadedOrFailed, s, NULL, SLS_FAILED, 0);
		return;
	}

	for (i = sizeof(AudioInputPlugins)/sizeof(AudioInputPlugins[0])-1; i >= 0; i--)
	{
		if (AudioInputPlugins[i].loadfunc)
		{
			if (AudioInputPlugins[i].loadfunc(s, data, filesize, snd_speed, forcedecode))
			{
				//wake up the main thread in case it decided to wait for us.
				COM_AddWork(WG_MAIN, S_LoadedOrFailed, s, NULL, SLS_LOADED, 0);
				BZ_Free(data);
				return;
			}
		}
	}

	if (s->loadstate != SLS_FAILED)
		Con_Printf ("Format not recognised: %s\n", namebuffer);

	COM_AddWork(WG_MAIN, S_LoadedOrFailed, s, NULL, SLS_FAILED, 0);
	BZ_Free(data);
	return;
}

qboolean S_LoadSound (sfx_t *s, qboolean force)
{
	if (s->loadstate == SLS_NOTLOADED && sndcardinfo)
	{
		s->loadstate = SLS_LOADING;
		COM_AddWork(WG_LOADER, S_LoadSoundWorker, s, NULL, force, 0);
	}
	if (s->loadstate == SLS_FAILED)
		return false;	//it failed to load once before, don't bother trying again.

	return true;	//loaded okay, or still loading
}

/*
===============================================================================

WAV loading

===============================================================================
*/

typedef struct
{
	char	*wavname;
	qbyte	*data_p;
	qbyte 	*iff_end;
	qbyte 	*last_chunk;
	qbyte 	*iff_data;
	int 	iff_chunk_len;
} wavctx_t;

static short GetLittleShort(wavctx_t *ctx)
{
	short val = 0;
	val = *ctx->data_p;
	val = val + (*(ctx->data_p+1)<<8);
	ctx->data_p += 2;
	return val;
}

static int GetLittleLong(wavctx_t *ctx)
{
	int val = 0;
	val = *ctx->data_p;
	val = val + (*(ctx->data_p+1)<<8);
	val = val + (*(ctx->data_p+2)<<16);
	val = val + (*(ctx->data_p+3)<<24);
	ctx->data_p += 4;
	return val;
}

static unsigned int FindNextChunk(wavctx_t *ctx, char *name)
{
	unsigned int dataleft;

	while (1)
	{
		dataleft = ctx->iff_end - ctx->last_chunk;
		if (dataleft < 8)
		{	// didn't find the chunk
			ctx->data_p = NULL;
			return 0;
		}

		ctx->data_p=ctx->last_chunk;
		ctx->data_p += 4;
		dataleft-= 8;
		ctx->iff_chunk_len = GetLittleLong(ctx);
		if (ctx->iff_chunk_len < 0)
		{
			ctx->data_p = NULL;
			return 0;
		}
		if (ctx->iff_chunk_len > dataleft)
		{
			Con_DPrintf ("\"%s\" seems truncated by %i bytes\n", ctx->wavname, ctx->iff_chunk_len-dataleft);
#if 1
			ctx->iff_chunk_len = dataleft;
#else
			ctx->data_p = NULL;
			return 0;
#endif
		}

		dataleft-= ctx->iff_chunk_len;
//		if (iff_chunk_len > 1024*1024)
//			Sys_Error ("FindNextChunk: %i length is past the 1 meg sanity limit", iff_chunk_len);
		ctx->data_p -= 8;
		ctx->last_chunk = ctx->data_p + 8 + ctx->iff_chunk_len;
		if ((ctx->iff_chunk_len&1) && dataleft)
			ctx->last_chunk++;
		if (!Q_strncmp(ctx->data_p, name, 4))
			return ctx->iff_chunk_len;
	}
}

static unsigned int FindChunk(wavctx_t *ctx, char *name)
{
	ctx->last_chunk = ctx->iff_data;
	return FindNextChunk (ctx, name);
}


#if 0
static void DumpChunks(void)
{
	char	str[5];

	str[4] = 0;
	data_p=iff_data;
	do
	{
		memcpy (str, data_p, 4);
		data_p += 4;
		iff_chunk_len = GetLittleLong();
		Con_Printf ("0x%x : %s (%d)\n", (int)(data_p - 4), str, iff_chunk_len);
		data_p += (iff_chunk_len + 1) & ~1;
	} while (data_p < iff_end);
}
#endif

/*
============
GetWavinfo
============
*/
static wavinfo_t GetWavinfo (char *name, qbyte *wav, int wavlength)
{
	extern cvar_t snd_ignorecueloops;
	wavinfo_t	info;
	int		i;
	int		samples;
	int		chunklen;
	wavctx_t ctx;

	memset (&info, 0, sizeof(info));

	if (!wav)
		return info;

	ctx.data_p = NULL;
	ctx.last_chunk = NULL;
	ctx.iff_chunk_len = 0;

	ctx.iff_data = wav;
	ctx.iff_end = wav + wavlength;
	ctx.wavname = name;

// find "RIFF" chunk
	chunklen = FindChunk(&ctx, "RIFF");
	if (chunklen < 4 ||  Q_strncmp(ctx.data_p+8, "WAVE", 4))
	{
		Con_Printf("Missing RIFF/WAVE chunks in %s\n", name);
		return info;
	}

// get "fmt " chunk
	ctx.iff_data = ctx.data_p + 12;
// DumpChunks ();

	chunklen = FindChunk(&ctx, "fmt ");
	if (chunklen < 24-8)
	{
		Con_Printf("Missing/truncated fmt chunk\n");
		return info;
	}
	ctx.data_p += 8;
	info.format = (unsigned short)GetLittleShort(&ctx);

	info.numchannels = (unsigned short)GetLittleShort(&ctx);
	info.rate = GetLittleLong(&ctx);
	ctx.data_p += 4;	//nAvgBytesPerSec
	ctx.data_p += 2;	//nBlockAlign
	info.bitwidth = (unsigned short)GetLittleShort(&ctx);	//meant to be a multiple of 8, but when its not we will treat it as 'nValidBits' and assume the lower bits are padded to bytes.

	if (info.format == 0xfffe)
	{
		if (GetLittleShort(&ctx) >= 22) //cbSize
		{
			ctx.data_p += 2;	//wValidBitsPerSample. don't really care
			ctx.data_p += 4;	//dwChannelMask. don't really care.
			if      (!memcmp(ctx.data_p, "\x01\x00\x00\x00\x00\x00\x10\x00\x80\x00\x00\xaa\x00\x38\x9b\x71", 16))
				info.format = 1;	//pcm(regular ints)
			else if (!memcmp(ctx.data_p, "\x03\x00\x00\x00\x00\x00\x10\x00\x80\x00\x00\xaa\x00\x38\x9b\x71", 16))
				info.format = 3;	//float
			//else leave it unusable.
			ctx.data_p += 16;	//SubFormat. convert to the real format
		}
	}

// get cue chunk
	chunklen = FindChunk(&ctx, "cue ");
	if (chunklen >= 36-8 && !snd_ignorecueloops.ival)
	{
		ctx.data_p += 32;
		info.loopstart = GetLittleLong(&ctx);
//		Con_Printf("loopstart=%d\n", sfx->loopstart);

	// if the next chunk is a LIST chunk, look for a cue length marker
		chunklen = FindNextChunk (&ctx, "LIST");
		if (chunklen >= 32-8)
		{
			if (!strncmp (ctx.data_p + 28, "mark", 4))
			{	// this is not a proper parse, but it works with cooledit...
				ctx.data_p += 24;
				i = GetLittleLong (&ctx);	// samples in loop
				info.samples = info.loopstart + i;
//				Con_Printf("looped length: %i\n", i);
			}
		}
	}
	else
		info.loopstart = -1;

// find data chunk
	chunklen = FindChunk(&ctx, "data");
	if (!ctx.data_p)
	{
		Con_Printf("Missing data chunk in %s\n", name);
		return info;
	}

	ctx.data_p += 8;
	samples = (chunklen<<3) / ((info.bitwidth+7)&~7) / info.numchannels;

	if (info.samples)
	{
		if (samples < info.samples)
		{
			info.samples = samples;
			Con_Printf ("Sound %s has a bad loop length\n", name);
		}
	}
	else
		info.samples = samples;

	if (info.loopstart > info.samples)
	{
		Con_Printf ("Sound %s has a bad loop start\n", name);
		info.loopstart = info.samples;
	}

	info.dataofs = ctx.data_p - wav;

	return info;
}
