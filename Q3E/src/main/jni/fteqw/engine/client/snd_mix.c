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
// snd_mix.c -- portable code to mix sounds for snd_dma.c

#include "quakedef.h"

#ifdef HAVE_MIXER


//#define MIXER_PAINT_F32
#ifdef MIXER_PAINT_F32
	#define MIX_16_8(val) ((val)/(float)(1<<(15+8)))
	#define MIX_8_8(val) ((val)/(float)(1<<(7+8)))
	typedef struct {
		float s[MAXSOUNDCHANNELS];
	} portable_samplegroup_t;
#else
	#define MIX_16_8(val) ((val)>>8)	//value is a 16bit*8bit value (audio*vol) value. discard the lower 8 bits to treat the volume as a fraction
	#define MIX_8_8(val) (val)	//value is a 8bit*8bit value (audio*vol) value. result is 16bit.

	typedef struct {
		int s[MAXSOUNDCHANNELS];	//signed, 1=0x7fff ish. will be clamped to allow oversaturation
	} portable_samplegroup_t;
#endif

#define	PAINTBUFFER_SIZE	2048

static portable_samplegroup_t paintbuffer[PAINTBUFFER_SIZE];	//FIXME: we really ought to be using SSE and floats or something.

void S_TransferPaintBuffer(soundcardinfo_t *sc, int endtime)
{
	unsigned int 	out_idx;
	unsigned int 	count;
	unsigned int 	outlimit;
#ifdef MIXER_PAINT_F32
	float 			*p = (float *fte_restrict)paintbuffer;
#else
	int 			*p = (int *fte_restrict) paintbuffer;
#endif
	int				val;
//	int				snd_vol;
	void			*pbuf;
	int				i, numc;

	count = (endtime - sc->paintedtime) * sc->sn.numchannels;
	outlimit = sc->sn.samples;
	out_idx = (sc->paintedtime * sc->sn.numchannels) % outlimit;
//	snd_vol = (volume.value*voicevolumemod)*256;
	numc = sc->sn.numchannels;

	pbuf = sc->Lock(sc, &out_idx);
	if (!pbuf)
		return;

	switch(sc->sn.sampleformat)
	{
	case QSF_INVALID:	//erk...
	case QSF_EXTERNALMIXER:	//shouldn't reach this.
		break;
	case QSF_U8:
		{
			unsigned char *out = (unsigned char *) pbuf;
			while (count)
			{
				for (i = 0; i < numc; i++)
				{
#ifdef MIXER_PAINT_F32
					val = (*p++ + 1)*128;
					if (val > 255)
						val = 255;
					else if (val < 0)
						val = 0;
					out[out_idx] = val;
#else
					val = *p++;
					if (val > 0x7fff)
						val = 0x7fff;
					else if (val < (short)0x8000)
						val = (short)0x8000;
					out[out_idx] = (val>>8) + 128;
#endif
					out_idx = (out_idx + 1) % outlimit;
				}
				p += MAXSOUNDCHANNELS - numc;
				count -= numc;
			}
		}
		break;
	case QSF_S8:
		{
			char *out = (char *) pbuf;
			while (count)
			{
				for (i = 0; i < numc; i++)
				{
#ifdef MIXER_PAINT_F32
					val = *p++*128;
					if (val > 127)
						val = 127;
					else if (val < -128)
						val = -128;
					out[out_idx] = val;
#else
					val = *p++;
					if (val > 0x7fff)
						val = 0x7fff;
					else if (val < (short)0x8000)
						val = (short)0x8000;
					out[out_idx] = (val>>8);
#endif
					out_idx = (out_idx + 1) % outlimit;
				}
				p += MAXSOUNDCHANNELS - numc;
				count -= numc;
			}
		}
		break;
	case QSF_S16:
		{
			short *out = (short *) pbuf;
			while (count)
			{
				for (i = 0; i < numc; i++)
				{
#ifdef MIXER_PAINT_F32
					val = *p++*0x7fff;
#else
					val = *p++;
#endif
					if (val > 0x7fff)
						val = 0x7fff;
					else if (val < (short)0x8000)
						val = (short)0x8000;
					out[out_idx] = val;
					out_idx = (out_idx + 1) % outlimit;
				}
				p += MAXSOUNDCHANNELS - numc;
				count -= numc;
			}
		}
		break;
	case QSF_F32:
		{
			float *out = (float *) pbuf;
			while (count)
			{
				for (i = 0; i < numc; i++)
				{
#ifdef MIXER_PAINT_F32	//FIXME: replace with a memcpy.
					out[out_idx] = *p++;
#else
					out[out_idx] = *p++ * (1.0 / 32768);
#endif
					out_idx = (out_idx + 1) % outlimit;
				}
				p += MAXSOUNDCHANNELS - numc;
				count -= numc;
			}
		}
		break;
	}

	sc->Unlock(sc, pbuf);
}


/*
===============================================================================

CHANNEL MIXING

===============================================================================
*/

static void SND_PaintChannel8_O2I1	(channel_t *ch, sfxcache_t *sc, int starttime, int count, int rate);
static void SND_PaintChannel16_O2I1	(channel_t *ch, sfxcache_t *sc, int starttime, int count, int rate);
static void SND_PaintChannel8_O4I1	(channel_t *ch, sfxcache_t *sc, int count, int rate);
static void SND_PaintChannel16_O4I1	(channel_t *ch, sfxcache_t *sc, int count, int rate);
static void SND_PaintChannel8_O6I1	(channel_t *ch, sfxcache_t *sc, int count, int rate);
static void SND_PaintChannel16_O6I1	(channel_t *ch, sfxcache_t *sc, int count, int rate);
static void SND_PaintChannel8_O8I1	(channel_t *ch, sfxcache_t *sc, int count, int rate);
static void SND_PaintChannel16_O8I1	(channel_t *ch, sfxcache_t *sc, int count, int rate);
static void SND_PaintChannel8_O2I2	(channel_t *ch, sfxcache_t *sc, int starttime, int count, int rate);
static void SND_PaintChannel16_O2I2	(channel_t *ch, sfxcache_t *sc, int starttime, int count, int rate);

#ifdef MIXER_F32
static void SND_PaintChannel32F_O2I1(channel_t *ch, sfxcache_t *sc, int starttime, int count, int rate);
static void SND_PaintChannel32F_O4I1(channel_t *ch, sfxcache_t *sc, int count, int rate);
static void SND_PaintChannel32F_O6I1(channel_t *ch, sfxcache_t *sc, int count, int rate);
static void SND_PaintChannel32F_O8I1(channel_t *ch, sfxcache_t *sc, int count, int rate);
static void SND_PaintChannel32F_O2I2(channel_t *ch, sfxcache_t *sc, int starttime, int count, int rate);
#endif

//NOTE: MAY NOT CALL SYS_ERROR
void S_PaintChannels(soundcardinfo_t *sc, int endtime)
{
	int 	i;
	int 	end;
	channel_t *ch;
	sfxcache_t	scachebuf;
	sfxcache_t	*scache;
	sfx_t *s;
	int		ltime, count;
	int avail;
	unsigned int maxlen = ruleset_allow_overlongsounds.ival?0xffffffffu>>PITCHSHIFT:snd_speed*20;
	int rate;

	while (sc->paintedtime < endtime)
	{
	// if paintbuffer is smaller than DMA buffer
		end = endtime;
		if (endtime - sc->paintedtime > PAINTBUFFER_SIZE)
			end = sc->paintedtime + PAINTBUFFER_SIZE;

	// clear the paint buffer
		Q_memset(paintbuffer, 0, (end - sc->paintedtime) * sizeof(portable_samplegroup_t));

	// paint in the channels.
		ch = sc->channel;
		for (i=0; i<sc->total_chans ; i++, ch++)
		{
			s = ch->sfx;
			if (!s || s->loadstate == SLS_LOADING)
				continue;
			if (!ch->vol[0] && !ch->vol[1] && !ch->vol[2] && !ch->vol[3] && !ch->vol[4] && !ch->vol[5])
			{
				//does it still make a sound if it cannot be heard?...
				//technically no...
				//this code is hacky.
				if (s->decoder.querydata)
					s->decoder.querydata(s, scache=&scachebuf, NULL, 0);
				else if (s->decoder.decodedata)
					scache = s->decoder.decodedata(s, &scachebuf, ch->pos>>PITCHSHIFT, 0);	/*1 for luck - balances audio termination below*/
				else
					scache = s->decoder.buf;
				ch->pos += (end-sc->paintedtime)*ch->rate;
				if (!scache || (ch->pos>>PITCHSHIFT) > scache->soundoffset+scache->length)
				{
					ch->pos = 0;
					if (s->loopstart != -1)
						ch->pos = s->loopstart<<PITCHSHIFT;
					else if (!(ch->flags & CF_FORCELOOP))
					{
						ch->sfx = NULL;
						if (s->decoder.ended)
						{
							if (!S_IsPlayingSomewhere(s))
								s->decoder.ended(s);
						}
					}
				}
				continue;
			}

			ltime = sc->paintedtime;
			while (ltime < end)
			{
				ssamplepos_t spos = ch->pos>>PITCHSHIFT;
				if (s->decoder.decodedata)
					scache = s->decoder.decodedata(s, &scachebuf, spos, 1 + (((end - ltime) * ch->rate)>>PITCHSHIFT));	/*1 for luck - balances audio termination below*/
				else if (s->decoder.buf)
				{
					scache = s->decoder.buf;
					if (spos >= scache->length)
						scache = NULL;	//EOF
				}
				else
					scache = NULL;

				if (!scache)
				{	//hit eof, loop it or stop it
					if (s->loopstart != -1)	/*some wavs contain a loop offset directly in the sound file, such samples loop even if a non-looping builtin was used*/
					{
						ch->pos &= ~((~0u)<<PITCHSHIFT);	/*clear out all but the subsample offset*/
						ch->pos += s->loopstart<<PITCHSHIFT;	//ignore the offset if its off the end of the file
					}
					else if (ch->flags & CF_FORCELOOP)	/*(static)channels which are explicitly looping always loop from the start*/
					{
						/*restart it*/
						ch->pos = 0;
					}
					else
					{	// channel just stopped
						ch->sfx = NULL;
						if (s->decoder.ended)
						{
							if (!S_IsPlayingSomewhere(s))
								s->decoder.ended(s);
						}
						break;
					}

					//retry at that new offset (continue here could give an infinite loop)
					spos = ch->pos>>PITCHSHIFT;
					if (s->decoder.decodedata)
						scache = s->decoder.decodedata(s, &scachebuf, spos, 1 + (((end - ltime) * ch->rate)>>PITCHSHIFT));	/*1 for luck - balances audio termination below*/
					else if (s->decoder.buf)
					{
						scache = s->decoder.buf;
						if (spos >= scache->length)
							scache = NULL;	//EOF
					}
					else
						scache = NULL;

					if (!scache)
						break;
				}

				if (sc->sn.speed != scache->speed)
					rate = (ch->rate * scache->speed) / sc->sn.speed;	//sound was loaded at the wrong speed. just play it back at a different speed and hope that all is well. this is nearest sampling, so expect it to be a little poo.
				else
					rate = ch->rate;

				if (spos < scache->soundoffset || spos > scache->soundoffset+scache->length)
					avail = 0;	//urm, we would be trying to read outside of the buffer. let mixing slip when there's no data available yet.
				else
				{
					// find how many samples till the sample ends (clamp max length)
					avail = scache->length;
					if (avail > maxlen)
						avail = snd_speed*10;
					avail = (((int)(scache->soundoffset + avail)<<PITCHSHIFT) - ch->pos + (rate-1)) / rate;
				}
				// mix the smaller of how much is available or the time left
				count = min(avail, end - ltime);

				if (count > 0)
				{
					if (ch->pos < 0)	//sounds with a pos of 0 are delay-start sounds
					{
						//don't progress past 0, so it actually starts properly at the right time with no clicks or anything
						if (count > (-ch->pos+255)>>PITCHSHIFT)
							count = ((-ch->pos+255)>>PITCHSHIFT);
						ltime += count;
						ch->pos += count*rate;
						continue;
					}

					switch(scache->format)
					{
					case QAF_S8:
						if (scache->numchannels==2)
							SND_PaintChannel8_O2I2(ch, scache, ltime-sc->paintedtime, count, rate);
						else if (sc->sn.numchannels <= 2)
							SND_PaintChannel8_O2I1(ch, scache, ltime-sc->paintedtime, count, rate);
						else if (sc->sn.numchannels <= 4)
							SND_PaintChannel8_O4I1(ch, scache, count, rate);
						else if (sc->sn.numchannels <= 6)
							SND_PaintChannel8_O6I1(ch, scache, count, rate);
						else
							SND_PaintChannel8_O8I1(ch, scache, count, rate);
						break;
					case QAF_S16:
						if (scache->numchannels==2)
							SND_PaintChannel16_O2I2(ch, scache, ltime-sc->paintedtime, count, rate);
						else if (sc->sn.numchannels <= 2)
							SND_PaintChannel16_O2I1(ch, scache, ltime-sc->paintedtime, count, rate);
						else if (sc->sn.numchannels <= 4)
							SND_PaintChannel16_O4I1(ch, scache, count, rate);
						else if (sc->sn.numchannels <= 6)
							SND_PaintChannel16_O6I1(ch, scache, count, rate);
						else
							SND_PaintChannel16_O8I1(ch, scache, count, rate);
						break;
#ifdef MIXER_F32
					case QAF_F32:
						if (scache->numchannels==2)
							SND_PaintChannel32F_O2I2(ch, scache, ltime-sc->paintedtime, count, rate);
						else if (sc->sn.numchannels <= 2)
							SND_PaintChannel32F_O2I1(ch, scache, ltime-sc->paintedtime, count, rate);
						else if (sc->sn.numchannels <= 4)
							SND_PaintChannel32F_O4I1(ch, scache, count, rate);
						else if (sc->sn.numchannels <= 6)
							SND_PaintChannel32F_O6I1(ch, scache, count, rate);
						else
							SND_PaintChannel32F_O8I1(ch, scache, count, rate);
						break;
#endif
#ifdef FTE_TARGET_WEB
					case QAF_BLOB:
						break;
#endif
					}
					ltime += count;
					ch->pos += rate * count;
				}
				else
					break;
			}
		}

	// transfer out according to DMA format
		S_TransferPaintBuffer(sc, end);
		sc->paintedtime = end;
	}
}

static void SND_PaintChannel8_O2I1 (channel_t *ch, sfxcache_t *sc, int starttime, int count, int rate)
{
	int 	data;
	signed char *sfx;
	int		i;
	unsigned int pos = ch->pos-(sc->soundoffset<<PITCHSHIFT);

	if (rate != (1<<PITCHSHIFT))
	{
		sfx = (signed char *fte_restrict)sc->data;
		for (i=0 ; i<count ; i++)
		{
			data = sfx[pos>>PITCHSHIFT];
			pos += rate;
			paintbuffer[starttime+i].s[0] += MIX_8_8(ch->vol[0] * data);
			paintbuffer[starttime+i].s[1] += MIX_8_8(ch->vol[1] * data);
		}
	}
	else
	{
		sfx = (signed char *fte_restrict)sc->data + (pos>>PITCHSHIFT);
		for (i=0 ; i<count ; i++)
		{
			data = sfx[i];
			paintbuffer[starttime+i].s[0] += MIX_8_8(ch->vol[0] * data);
			paintbuffer[starttime+i].s[1] += MIX_8_8(ch->vol[1] * data);
		}
	}
}

static void SND_PaintChannel8_O2I2 (channel_t *ch, sfxcache_t *sc, int starttime, int count, int rate)
{
//	int 	data;
	signed char *sfx;
	int		i;
	unsigned int pos = ch->pos-(sc->soundoffset<<PITCHSHIFT);

	if (rate != (1<<PITCHSHIFT))
	{
		sfx = (signed char *fte_restrict)sc->data;
		for (i=0 ; i<count ; i++)
		{
			paintbuffer[starttime+i].s[0] += MIX_8_8(ch->vol[0] * sfx[(pos>>(PITCHSHIFT-1))&~1]);
			paintbuffer[starttime+i].s[1] += MIX_8_8(ch->vol[1] * sfx[(pos>>(PITCHSHIFT-1))|1]);
			pos += rate;
		}
	}
	else
	{
		sfx = (signed char *fte_restrict)sc->data + (pos>>PITCHSHIFT)*2;
		for (i=0 ; i<count ; i++)
		{
			paintbuffer[starttime+i].s[0] += MIX_8_8(ch->vol[0] * sfx[(i<<1)]);
			paintbuffer[starttime+i].s[1] += MIX_8_8(ch->vol[1] * sfx[(i<<1)+1]);
		}
	}
}

static void SND_PaintChannel8_O4I1 (channel_t *ch, sfxcache_t *sc, int count, int rate)
{
	signed char *sfx;
	int		i;
	unsigned int pos = ch->pos-(sc->soundoffset<<PITCHSHIFT);

	if (rate != (1<<PITCHSHIFT))
	{
		signed char data;
		sfx = (signed char *fte_restrict)sc->data;
		for (i=0 ; i<count ; i++)
		{
			data = sfx[pos>>PITCHSHIFT];
			pos += rate;
			paintbuffer[i].s[0] += MIX_8_8(ch->vol[0] * data);
			paintbuffer[i].s[1] += MIX_8_8(ch->vol[1] * data);
			paintbuffer[i].s[2] += MIX_8_8(ch->vol[2] * data);
			paintbuffer[i].s[3] += MIX_8_8(ch->vol[3] * data);
		}
	}
	else
	{
		sfx = (signed char *fte_restrict)sc->data + (pos>>PITCHSHIFT);
		for (i=0 ; i<count ; i++)
		{
			paintbuffer[i].s[0] += MIX_8_8(ch->vol[0] * sfx[i]);
			paintbuffer[i].s[1] += MIX_8_8(ch->vol[1] * sfx[i]);
			paintbuffer[i].s[2] += MIX_8_8(ch->vol[2] * sfx[i]);
			paintbuffer[i].s[3] += MIX_8_8(ch->vol[3] * sfx[i]);
		}
	}
}

static void SND_PaintChannel8_O6I1 (channel_t *ch, sfxcache_t *sc, int count, int rate)
{
	signed char *sfx;
	int		i;
	unsigned int pos = ch->pos-(sc->soundoffset<<PITCHSHIFT);

	if (rate != (1<<PITCHSHIFT))
	{
		signed char data;
		sfx = (signed char *fte_restrict)sc->data;
		for (i=0 ; i<count ; i++)
		{
			data = sfx[pos>>PITCHSHIFT];
			pos += rate;
			paintbuffer[i].s[0] += MIX_8_8(ch->vol[0] * data);
			paintbuffer[i].s[1] += MIX_8_8(ch->vol[1] * data);
			paintbuffer[i].s[2] += MIX_8_8(ch->vol[2] * data);
			paintbuffer[i].s[3] += MIX_8_8(ch->vol[3] * data);
			paintbuffer[i].s[4] += MIX_8_8(ch->vol[4] * data);
			paintbuffer[i].s[5] += MIX_8_8(ch->vol[5] * data);
		}
	}
	else
	{
		sfx = (signed char *fte_restrict)sc->data + (pos>>PITCHSHIFT);
		for (i=0 ; i<count ; i++)
		{
			paintbuffer[i].s[0] += MIX_8_8(ch->vol[0] * sfx[i]);
			paintbuffer[i].s[1] += MIX_8_8(ch->vol[1] * sfx[i]);
			paintbuffer[i].s[2] += MIX_8_8(ch->vol[2] * sfx[i]);
			paintbuffer[i].s[3] += MIX_8_8(ch->vol[3] * sfx[i]);
			paintbuffer[i].s[4] += MIX_8_8(ch->vol[4] * sfx[i]);
			paintbuffer[i].s[5] += MIX_8_8(ch->vol[5] * sfx[i]);
		}
	}
}

static void SND_PaintChannel8_O8I1 (channel_t *ch, sfxcache_t *sc, int count, int rate)
{
	signed char *sfx;
	int		i;
	unsigned int pos = ch->pos-(sc->soundoffset<<PITCHSHIFT);

	if (rate != (1<<PITCHSHIFT))
	{
		signed char data;
		sfx = (signed char *fte_restrict)sc->data;
		for (i=0 ; i<count ; i++)
		{
			data = sfx[pos>>PITCHSHIFT];
			pos += rate;
			paintbuffer[i].s[0] += MIX_8_8(ch->vol[0] * data);
			paintbuffer[i].s[1] += MIX_8_8(ch->vol[1] * data);
			paintbuffer[i].s[2] += MIX_8_8(ch->vol[2] * data);
			paintbuffer[i].s[3] += MIX_8_8(ch->vol[3] * data);
			paintbuffer[i].s[4] += MIX_8_8(ch->vol[4] * data);
			paintbuffer[i].s[5] += MIX_8_8(ch->vol[5] * data);
			paintbuffer[i].s[6] += MIX_8_8(ch->vol[6] * data);
			paintbuffer[i].s[7] += MIX_8_8(ch->vol[7] * data);
		}
	}
	else
	{
		sfx = (signed char *fte_restrict)sc->data + (pos>>PITCHSHIFT);
		for (i=0 ; i<count ; i++)
		{
			paintbuffer[i].s[0] += MIX_8_8(ch->vol[0] * sfx[i]);
			paintbuffer[i].s[1] += MIX_8_8(ch->vol[1] * sfx[i]);
			paintbuffer[i].s[2] += MIX_8_8(ch->vol[2] * sfx[i]);
			paintbuffer[i].s[3] += MIX_8_8(ch->vol[3] * sfx[i]);
			paintbuffer[i].s[4] += MIX_8_8(ch->vol[4] * sfx[i]);
			paintbuffer[i].s[5] += MIX_8_8(ch->vol[5] * sfx[i]);
			paintbuffer[i].s[6] += MIX_8_8(ch->vol[6] * sfx[i]);
			paintbuffer[i].s[7] += MIX_8_8(ch->vol[7] * sfx[i]);
		}
	}
}


static void SND_PaintChannel16_O2I1 (channel_t *ch, sfxcache_t *sc, int starttime, int count, int rate)
{
	int data;
	int leftvol, rightvol;
	signed short *sfx;
	int	i;
	unsigned int pos = ch->pos-(sc->soundoffset<<PITCHSHIFT);

	leftvol = ch->vol[0];
	rightvol = ch->vol[1];

	if (rate != (1<<PITCHSHIFT))
	{
		signed int data;
		sfx = (signed short *fte_restrict)sc->data;
		for (i=0 ; i<count ; i++)
		{
			int frac = pos&((1<<PITCHSHIFT)-1);
			data = sfx[pos>>PITCHSHIFT] * ((1<<PITCHSHIFT)-frac) + sfx[(pos>>PITCHSHIFT)+1] * frac;
			pos += rate;
			paintbuffer[starttime+i].s[0] += MIX_16_8((leftvol * data)>>PITCHSHIFT);
			paintbuffer[starttime+i].s[1] += MIX_16_8((rightvol * data)>>PITCHSHIFT);
		}
	}
	else
	{
		sfx = (signed short *fte_restrict)sc->data + (pos>>PITCHSHIFT);
		for (i=0 ; i<count ; i++)
		{
			data = sfx[i];
			paintbuffer[starttime+i].s[0] += MIX_16_8(data * leftvol);
			paintbuffer[starttime+i].s[1] += MIX_16_8(data * rightvol);
		}
	}
}

static void SND_PaintChannel16_O2I2 (channel_t *ch, sfxcache_t *sc, int starttime, int count, int rate)
{
	int leftvol, rightvol;
	signed short *sfx;
	int	i;
	unsigned int pos = ch->pos-(sc->soundoffset<<PITCHSHIFT);

	leftvol = ch->vol[0];
	rightvol = ch->vol[1];

	if (rate != (1<<PITCHSHIFT))
	{
		signed short l, r;
		sfx = (signed short *fte_restrict)sc->data;
		for (i=0 ; i<count ; i++)
		{
			l = sfx[(pos>>(PITCHSHIFT-1))&~1];
			r = sfx[(pos>>(PITCHSHIFT-1))|1];
			pos += rate;
			paintbuffer[starttime+i].s[0] += MIX_16_8(ch->vol[0] * l);
			paintbuffer[starttime+i].s[1] += MIX_16_8(ch->vol[1] * r);
		}
	}
	else
	{
		sfx = (signed short *fte_restrict)sc->data + (pos>>PITCHSHIFT)*2;
		for (i=0 ; i<count ; i++)
		{
			paintbuffer[starttime+i].s[0] += MIX_16_8(*sfx++ * leftvol);
			paintbuffer[starttime+i].s[1] += MIX_16_8(*sfx++ * rightvol);
		}
	}
}

static void SND_PaintChannel16_O4I1 (channel_t *ch, sfxcache_t *sc, int count, int rate)
{
	int vol[4];
	signed short *sfx;
	int	i;
	unsigned int pos = ch->pos-(sc->soundoffset<<PITCHSHIFT);

	vol[0] = ch->vol[0];
	vol[1] = ch->vol[1];
	vol[2] = ch->vol[2];
	vol[3] = ch->vol[3];

	if (rate != (1<<PITCHSHIFT))
	{
		signed short data;
		sfx = (signed short *fte_restrict)sc->data;
		for (i=0 ; i<count ; i++)
		{
			data = sfx[pos>>PITCHSHIFT];
			pos += rate;
			paintbuffer[i].s[0] += MIX_16_8(vol[0] * data);
			paintbuffer[i].s[1] += MIX_16_8(vol[1] * data);
			paintbuffer[i].s[2] += MIX_16_8(vol[2] * data);
			paintbuffer[i].s[3] += MIX_16_8(vol[3] * data);
		}
	}
	else
	{
		sfx = (signed short *fte_restrict)sc->data + (pos>>PITCHSHIFT);
		for (i=0 ; i<count ; i++)
		{
			paintbuffer[i].s[0] += MIX_16_8(sfx[i] * vol[0]);
			paintbuffer[i].s[1] += MIX_16_8(sfx[i] * vol[1]);
			paintbuffer[i].s[2] += MIX_16_8(sfx[i] * vol[2]);
			paintbuffer[i].s[3] += MIX_16_8(sfx[i] * vol[3]);
		}
	}
}

static void SND_PaintChannel16_O6I1 (channel_t *ch, sfxcache_t *sc, int count, int rate)
{
	int vol[6];
	signed short *sfx;
	int	i;
	unsigned int pos = ch->pos-(sc->soundoffset<<PITCHSHIFT);

	vol[0] = ch->vol[0];
	vol[1] = ch->vol[1];
	vol[2] = ch->vol[2];
	vol[3] = ch->vol[3];
	vol[4] = ch->vol[4];
	vol[5] = ch->vol[5];

	if (rate != (1<<PITCHSHIFT))
	{
		signed short data;
		sfx = (signed short *fte_restrict)sc->data;
		for (i=0 ; i<count ; i++)
		{
			data = sfx[pos>>PITCHSHIFT];
			pos += rate;
			paintbuffer[i].s[0] += MIX_16_8(vol[0] * data);
			paintbuffer[i].s[1] += MIX_16_8(vol[1] * data);
			paintbuffer[i].s[2] += MIX_16_8(vol[2] * data);
			paintbuffer[i].s[3] += MIX_16_8(vol[3] * data);
			paintbuffer[i].s[4] += MIX_16_8(vol[4] * data);
			paintbuffer[i].s[5] += MIX_16_8(vol[5] * data);
		}
	}
	else
	{
		sfx = (signed short *fte_restrict)sc->data + (pos>>PITCHSHIFT);
		for (i=0 ; i<count ; i++)
		{
			paintbuffer[i].s[0] += MIX_16_8(sfx[i] * vol[0]);
			paintbuffer[i].s[1] += MIX_16_8(sfx[i] * vol[1]);
			paintbuffer[i].s[2] += MIX_16_8(sfx[i] * vol[2]);
			paintbuffer[i].s[3] += MIX_16_8(sfx[i] * vol[3]);
			paintbuffer[i].s[4] += MIX_16_8(sfx[i] * vol[4]);
			paintbuffer[i].s[5] += MIX_16_8(sfx[i] * vol[5]);
		}
	}
}

static void SND_PaintChannel16_O8I1 (channel_t *ch, sfxcache_t *sc, int count, int rate)
{
	int vol[8];
	signed short *sfx;
	int	i;
	unsigned int pos = ch->pos-(sc->soundoffset<<PITCHSHIFT);

	vol[0] = ch->vol[0];
	vol[1] = ch->vol[1];
	vol[2] = ch->vol[2];
	vol[3] = ch->vol[3];
	vol[4] = ch->vol[4];
	vol[5] = ch->vol[5];
	vol[6] = ch->vol[6];
	vol[7] = ch->vol[7];

	if (rate != (1<<PITCHSHIFT))
	{
		signed short data;
		sfx = (signed short *fte_restrict)sc->data;
		for (i=0 ; i<count ; i++)
		{
			data = sfx[pos>>PITCHSHIFT];
			pos += rate;
			paintbuffer[i].s[0] += MIX_16_8(vol[0] * data);
			paintbuffer[i].s[1] += MIX_16_8(vol[1] * data);
			paintbuffer[i].s[2] += MIX_16_8(vol[2] * data);
			paintbuffer[i].s[3] += MIX_16_8(vol[3] * data);
			paintbuffer[i].s[4] += MIX_16_8(vol[4] * data);
			paintbuffer[i].s[5] += MIX_16_8(vol[5] * data);
			paintbuffer[i].s[6] += MIX_16_8(vol[6] * data);
			paintbuffer[i].s[7] += MIX_16_8(vol[7] * data);
		}
	}
	else
	{
		sfx = (signed short *fte_restrict)sc->data + (pos>>PITCHSHIFT);
		for (i=0 ; i<count ; i++)
		{
			paintbuffer[i].s[0] += MIX_16_8(sfx[i] * vol[0]);
			paintbuffer[i].s[1] += MIX_16_8(sfx[i] * vol[1]);
			paintbuffer[i].s[2] += MIX_16_8(sfx[i] * vol[2]);
			paintbuffer[i].s[3] += MIX_16_8(sfx[i] * vol[3]);
			paintbuffer[i].s[4] += MIX_16_8(sfx[i] * vol[4]);
			paintbuffer[i].s[5] += MIX_16_8(sfx[i] * vol[5]);
			paintbuffer[i].s[6] += MIX_16_8(sfx[i] * vol[6]);
			paintbuffer[i].s[7] += MIX_16_8(sfx[i] * vol[7]);
		}
	}
}

#ifdef MIXER_F32
static void SND_PaintChannel32F_O2I1 (channel_t *ch, sfxcache_t *sc, int starttime, int count, int rate)
{
	float data;
	int left, right;
	int leftvol, rightvol;
	float *sfx;
	int	i;
	unsigned int pos = ch->pos-(sc->soundoffset<<PITCHSHIFT);

	leftvol = ch->vol[0]*128;
	rightvol = ch->vol[1]*128;

	if (rate != (1<<PITCHSHIFT))
	{
		sfx = (float *fte_restrict)sc->data;
		for (i=0 ; i<count ; i++)
		{
			float frac = pos&((1<<PITCHSHIFT)-1);
			data = sfx[pos>>PITCHSHIFT] * (1-frac) + sfx[(pos>>PITCHSHIFT)+1] * frac;
			pos += rate;
			paintbuffer[starttime+i].s[0] += (leftvol * data);
			paintbuffer[starttime+i].s[1] += (rightvol * data);
		}
	}
	else
	{
		sfx = (float *fte_restrict)sc->data + (pos>>PITCHSHIFT);
		for (i=0 ; i<count ; i++)
		{
			data = sfx[i];
			left = (data * leftvol);
			right = (data * rightvol);
			paintbuffer[starttime+i].s[0] += left;
			paintbuffer[starttime+i].s[1] += right;
		}
	}
}

static void SND_PaintChannel32F_O2I2 (channel_t *ch, sfxcache_t *sc, int starttime, int count, int rate)
{
	float leftvol, rightvol;
	float *sfx;
	int	i;
	unsigned int pos = ch->pos-(sc->soundoffset<<PITCHSHIFT);

	leftvol = ch->vol[0]*128;
	rightvol = ch->vol[1]*128;

	if (rate != (1<<PITCHSHIFT))
	{
		float l, r;
		sfx = (float *fte_restrict)sc->data;
		for (i=0 ; i<count ; i++)
		{
			l = sfx[(pos>>(PITCHSHIFT-1))&~1];
			r = sfx[(pos>>(PITCHSHIFT-1))|1];
			pos += rate;
			paintbuffer[starttime+i].s[0] += (l * leftvol);
			paintbuffer[starttime+i].s[1] += (r * rightvol);
		}
	}
	else
	{
		sfx = (float *fte_restrict)sc->data + (pos>>PITCHSHIFT)*2;
		for (i=0 ; i<count ; i++)
		{
			paintbuffer[starttime+i].s[0] += (*sfx++ * leftvol);
			paintbuffer[starttime+i].s[1] += (*sfx++ * rightvol);
		}
	}
}

static void SND_PaintChannel32F_O4I1 (channel_t *ch, sfxcache_t *sc, int count, int rate)
{
	int vol[4];
	float *sfx;
	int	i;
	unsigned int pos = ch->pos-(sc->soundoffset<<PITCHSHIFT);

	vol[0] = ch->vol[0]*128;
	vol[1] = ch->vol[1]*128;
	vol[2] = ch->vol[2]*128;
	vol[3] = ch->vol[3]*128;

	if (rate != (1<<PITCHSHIFT))
	{
		float data;
		sfx = (float *fte_restrict)sc->data;
		for (i=0 ; i<count ; i++)
		{
			data = sfx[pos>>PITCHSHIFT];
			pos += rate;
			paintbuffer[i].s[0] += (vol[0] * data);
			paintbuffer[i].s[1] += (vol[1] * data);
			paintbuffer[i].s[2] += (vol[2] * data);
			paintbuffer[i].s[3] += (vol[3] * data);
		}
	}
	else
	{
		sfx = (float *fte_restrict)sc->data + (pos>>PITCHSHIFT);
		for (i=0 ; i<count ; i++)
		{
			paintbuffer[i].s[0] += (sfx[i] * vol[0]);
			paintbuffer[i].s[1] += (sfx[i] * vol[1]);
			paintbuffer[i].s[2] += (sfx[i] * vol[2]);
			paintbuffer[i].s[3] += (sfx[i] * vol[3]);
		}
	}
}

static void SND_PaintChannel32F_O6I1 (channel_t *ch, sfxcache_t *sc, int count, int rate)
{
	int vol[6];
	float *sfx;
	int	i;
	unsigned int pos = ch->pos-(sc->soundoffset<<PITCHSHIFT);

	vol[0] = ch->vol[0]*128;
	vol[1] = ch->vol[1]*128;
	vol[2] = ch->vol[2]*128;
	vol[3] = ch->vol[3]*128;
	vol[4] = ch->vol[4]*128;
	vol[5] = ch->vol[5]*128;

	if (rate != (1<<PITCHSHIFT))
	{
		float data;
		sfx = (float *fte_restrict)sc->data;
		for (i=0 ; i<count ; i++)
		{
			data = sfx[pos>>PITCHSHIFT];
			pos += rate;
			paintbuffer[i].s[0] += (vol[0] * data);
			paintbuffer[i].s[1] += (vol[1] * data);
			paintbuffer[i].s[2] += (vol[2] * data);
			paintbuffer[i].s[3] += (vol[3] * data);
			paintbuffer[i].s[4] += (vol[4] * data);
			paintbuffer[i].s[5] += (vol[5] * data);
		}
	}
	else
	{
		sfx = (float *fte_restrict)sc->data + (pos>>PITCHSHIFT);
		for (i=0 ; i<count ; i++)
		{
			paintbuffer[i].s[0] += (sfx[i] * vol[0]);
			paintbuffer[i].s[1] += (sfx[i] * vol[1]);
			paintbuffer[i].s[2] += (sfx[i] * vol[2]);
			paintbuffer[i].s[3] += (sfx[i] * vol[3]);
			paintbuffer[i].s[4] += (sfx[i] * vol[4]);
			paintbuffer[i].s[5] += (sfx[i] * vol[5]);
		}
	}
}

static void SND_PaintChannel32F_O8I1 (channel_t *ch, sfxcache_t *sc, int count, int rate)
{
	int vol[8];
	float *sfx;
	int	i;
	unsigned int pos = ch->pos-(sc->soundoffset<<PITCHSHIFT);

	//input is +/- 1, output is +/- 32767. ch->vol is 0-255, so we just need to scale up by an extra 128
	vol[0] = ch->vol[0]*128;
	vol[1] = ch->vol[1]*128;
	vol[2] = ch->vol[2]*128;
	vol[3] = ch->vol[3]*128;
	vol[4] = ch->vol[4]*128;
	vol[5] = ch->vol[5]*128;
	vol[6] = ch->vol[6]*128;
	vol[7] = ch->vol[7]*128;

	if (rate != (1<<PITCHSHIFT))
	{
		float data;
		sfx = (float *fte_restrict)sc->data;
		for (i=0 ; i<count ; i++)
		{
			data = sfx[pos>>PITCHSHIFT];
			pos += rate;
			paintbuffer[i].s[0] += (vol[0] * data);
			paintbuffer[i].s[1] += (vol[1] * data);
			paintbuffer[i].s[2] += (vol[2] * data);
			paintbuffer[i].s[3] += (vol[3] * data);
			paintbuffer[i].s[4] += (vol[4] * data);
			paintbuffer[i].s[5] += (vol[5] * data);
			paintbuffer[i].s[6] += (vol[6] * data);
			paintbuffer[i].s[7] += (vol[7] * data);
		}
	}
	else
	{
		sfx = (float *fte_restrict)sc->data + (pos>>PITCHSHIFT);
		for (i=0 ; i<count ; i++)
		{
			paintbuffer[i].s[0] += (sfx[i] * vol[0]);
			paintbuffer[i].s[1] += (sfx[i] * vol[1]);
			paintbuffer[i].s[2] += (sfx[i] * vol[2]);
			paintbuffer[i].s[3] += (sfx[i] * vol[3]);
			paintbuffer[i].s[4] += (sfx[i] * vol[4]);
			paintbuffer[i].s[5] += (sfx[i] * vol[5]);
			paintbuffer[i].s[6] += (sfx[i] * vol[6]);
			paintbuffer[i].s[7] += (sfx[i] * vol[7]);
		}
	}
}
#endif
#endif
