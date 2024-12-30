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

#include "quakedef.h"
#include "snd_main.h"

extern cvar_t snd_softclip;

static portable_sampleframe_t paintbuffer[PAINTBUFFER_SIZE];

extern speakerlayout_t snd_speakerlayout; // for querying the listeners

#ifdef CONFIG_VIDEO_CAPTURE
static portable_sampleframe_t paintbuffer_unswapped[PAINTBUFFER_SIZE];

static void S_CaptureAVISound(const portable_sampleframe_t *sampleframes, size_t length)
{
	size_t i;
	unsigned int j;

	if (!cls.capturevideo.active)
		return;

	// undo whatever swapping the channel layout (swapstereo, ALSA) did
	for(j = 0; j < snd_speakerlayout.channels; ++j)
	{
		unsigned int j0 = snd_speakerlayout.listeners[j].channel_unswapped;
		for(i = 0; i < length; ++i)
			paintbuffer_unswapped[i].sample[j0] = sampleframes[i].sample[j];
	}

	SCR_CaptureVideo_SoundFrame(paintbuffer_unswapped, length);
}
#endif

extern cvar_t snd_softclip;

static void S_SoftClipPaintBuffer(portable_sampleframe_t *painted_ptr, int nbframes, int width, int nchannels)
{
	int i;

	if((snd_softclip.integer == 1 && width <= 2) || snd_softclip.integer > 1)
	{
		portable_sampleframe_t *p = painted_ptr;

#if 0
/* Soft clipping, the sound of a dream, thanks to Jon Wattes
   post to Musicdsp.org */
#define SOFTCLIP(x) (x) = sin(bound(-M_PI/2, (x), M_PI/2)) * 0.25
#endif

		// let's do a simple limiter instead, seems to sound better
		static float maxvol = 0;
		maxvol = max(1.0f, maxvol * (1.0f - nbframes / (0.4f * snd_renderbuffer->format.speed)));
#define SOFTCLIP(x) if(fabs(x)>maxvol) maxvol=fabs(x); (x) /= maxvol;

		if (nchannels == 8)  // 7.1 surround
		{
			for (i = 0;i < nbframes;i++, p++)
			{
				SOFTCLIP(p->sample[0]);
				SOFTCLIP(p->sample[1]);
				SOFTCLIP(p->sample[2]);
				SOFTCLIP(p->sample[3]);
				SOFTCLIP(p->sample[4]);
				SOFTCLIP(p->sample[5]);
				SOFTCLIP(p->sample[6]);
				SOFTCLIP(p->sample[7]);
			}
		}
		else if (nchannels == 6)  // 5.1 surround
		{
			for (i = 0; i < nbframes; i++, p++)
			{
				SOFTCLIP(p->sample[0]);
				SOFTCLIP(p->sample[1]);
				SOFTCLIP(p->sample[2]);
				SOFTCLIP(p->sample[3]);
				SOFTCLIP(p->sample[4]);
				SOFTCLIP(p->sample[5]);
			}
		}
		else if (nchannels == 4)  // 4.0 surround
		{
			for (i = 0; i < nbframes; i++, p++)
			{
				SOFTCLIP(p->sample[0]);
				SOFTCLIP(p->sample[1]);
				SOFTCLIP(p->sample[2]);
				SOFTCLIP(p->sample[3]);
			}
		}
		else if (nchannels == 2)  // 2.0 stereo
		{
			for (i = 0; i < nbframes; i++, p++)
			{
				SOFTCLIP(p->sample[0]);
				SOFTCLIP(p->sample[1]);
			}
		}
		else if (nchannels == 1)  // 1.0 mono
		{
			for (i = 0; i < nbframes; i++, p++)
			{
				SOFTCLIP(p->sample[0]);
			}
		}
#undef SOFTCLIP
	}
}

static void S_ConvertPaintBuffer(portable_sampleframe_t *painted_ptr, void *rb_ptr, int nbframes, int width, int nchannels)
{
	int i;
	float val;
	if (width == 4)  // 32bit float
	{
		float *snd_out = (float*)rb_ptr;
		if (nchannels == 8)  // 7.1 surround
		{
			for (i = 0; i < nbframes; i++, painted_ptr++)
			{
				*snd_out++ = painted_ptr->sample[0];
				*snd_out++ = painted_ptr->sample[1];
				*snd_out++ = painted_ptr->sample[2];
				*snd_out++ = painted_ptr->sample[3];
				*snd_out++ = painted_ptr->sample[4];
				*snd_out++ = painted_ptr->sample[5];
				*snd_out++ = painted_ptr->sample[6];
				*snd_out++ = painted_ptr->sample[7];
			}
		}
		else if (nchannels == 6)  // 5.1 surround
		{
			for (i = 0; i < nbframes; i++, painted_ptr++)
			{
				*snd_out++ = painted_ptr->sample[0];
				*snd_out++ = painted_ptr->sample[1];
				*snd_out++ = painted_ptr->sample[2];
				*snd_out++ = painted_ptr->sample[3];
				*snd_out++ = painted_ptr->sample[4];
				*snd_out++ = painted_ptr->sample[5];
			}
		}
		else if (nchannels == 4)  // 4.0 surround
		{
			for (i = 0; i < nbframes; i++, painted_ptr++)
			{
				*snd_out++ = painted_ptr->sample[0];
				*snd_out++ = painted_ptr->sample[1];
				*snd_out++ = painted_ptr->sample[2];
				*snd_out++ = painted_ptr->sample[3];
			}
		}
		else if (nchannels == 2)  // 2.0 stereo
		{
			for (i = 0; i < nbframes; i++, painted_ptr++)
			{
				*snd_out++ = painted_ptr->sample[0];
				*snd_out++ = painted_ptr->sample[1];
			}
		}
		else if (nchannels == 1)  // 1.0 mono
		{
			for (i = 0; i < nbframes; i++, painted_ptr++)
			{
				*snd_out++ = painted_ptr->sample[0];
			}
		}
	}
	else if (width == 2)  // 16bit
	{
		short *snd_out = (short*)rb_ptr;
		if (nchannels == 8)  // 7.1 surround
		{
			for (i = 0;i < nbframes;i++, painted_ptr++)
			{
				val = (int)(painted_ptr->sample[0] * 32768.0f);*snd_out++ = bound(-32768, val, 32767);
				val = (int)(painted_ptr->sample[1] * 32768.0f);*snd_out++ = bound(-32768, val, 32767);
				val = (int)(painted_ptr->sample[2] * 32768.0f);*snd_out++ = bound(-32768, val, 32767);
				val = (int)(painted_ptr->sample[3] * 32768.0f);*snd_out++ = bound(-32768, val, 32767);
				val = (int)(painted_ptr->sample[4] * 32768.0f);*snd_out++ = bound(-32768, val, 32767);
				val = (int)(painted_ptr->sample[5] * 32768.0f);*snd_out++ = bound(-32768, val, 32767);
				val = (int)(painted_ptr->sample[6] * 32768.0f);*snd_out++ = bound(-32768, val, 32767);
				val = (int)(painted_ptr->sample[7] * 32768.0f);*snd_out++ = bound(-32768, val, 32767);
			}
		}
		else if (nchannels == 6)  // 5.1 surround
		{
			for (i = 0; i < nbframes; i++, painted_ptr++)
			{
				val = (int)(painted_ptr->sample[0] * 32768.0f);*snd_out++ = bound(-32768, val, 32767);
				val = (int)(painted_ptr->sample[1] * 32768.0f);*snd_out++ = bound(-32768, val, 32767);
				val = (int)(painted_ptr->sample[2] * 32768.0f);*snd_out++ = bound(-32768, val, 32767);
				val = (int)(painted_ptr->sample[3] * 32768.0f);*snd_out++ = bound(-32768, val, 32767);
				val = (int)(painted_ptr->sample[4] * 32768.0f);*snd_out++ = bound(-32768, val, 32767);
				val = (int)(painted_ptr->sample[5] * 32768.0f);*snd_out++ = bound(-32768, val, 32767);
			}
		}
		else if (nchannels == 4)  // 4.0 surround
		{
			for (i = 0; i < nbframes; i++, painted_ptr++)
			{
				val = (int)(painted_ptr->sample[0] * 32768.0f);*snd_out++ = bound(-32768, val, 32767);
				val = (int)(painted_ptr->sample[1] * 32768.0f);*snd_out++ = bound(-32768, val, 32767);
				val = (int)(painted_ptr->sample[2] * 32768.0f);*snd_out++ = bound(-32768, val, 32767);
				val = (int)(painted_ptr->sample[3] * 32768.0f);*snd_out++ = bound(-32768, val, 32767);
			}
		}
		else if (nchannels == 2)  // 2.0 stereo
		{
			for (i = 0; i < nbframes; i++, painted_ptr++)
			{
				val = (int)(painted_ptr->sample[0] * 32768.0f);*snd_out++ = bound(-32768, val, 32767);
				val = (int)(painted_ptr->sample[1] * 32768.0f);*snd_out++ = bound(-32768, val, 32767);
			}
		}
		else if (nchannels == 1)  // 1.0 mono
		{
			for (i = 0; i < nbframes; i++, painted_ptr++)
			{
				val = (int)((painted_ptr->sample[0] + painted_ptr->sample[1]) * 16384.0f);*snd_out++ = bound(-32768, val, 32767);
			}
		}
	}
	else  // 8bit
	{
		unsigned char *snd_out = (unsigned char*)rb_ptr;
		if (nchannels == 8)  // 7.1 surround
		{
			for (i = 0; i < nbframes; i++, painted_ptr++)
			{
				val = (int)(painted_ptr->sample[0] * 128.0f) + 128; *snd_out++ = bound(0, val, 255);
				val = (int)(painted_ptr->sample[1] * 128.0f) + 128; *snd_out++ = bound(0, val, 255);
				val = (int)(painted_ptr->sample[2] * 128.0f) + 128; *snd_out++ = bound(0, val, 255);
				val = (int)(painted_ptr->sample[3] * 128.0f) + 128; *snd_out++ = bound(0, val, 255);
				val = (int)(painted_ptr->sample[4] * 128.0f) + 128; *snd_out++ = bound(0, val, 255);
				val = (int)(painted_ptr->sample[5] * 128.0f) + 128; *snd_out++ = bound(0, val, 255);
				val = (int)(painted_ptr->sample[6] * 128.0f) + 128; *snd_out++ = bound(0, val, 255);
				val = (int)(painted_ptr->sample[7] * 128.0f) + 128; *snd_out++ = bound(0, val, 255);
			}
		}
		else if (nchannels == 6)  // 5.1 surround
		{
			for (i = 0; i < nbframes; i++, painted_ptr++)
			{
				val = (int)(painted_ptr->sample[0] * 128.0f) + 128; *snd_out++ = bound(0, val, 255);
				val = (int)(painted_ptr->sample[1] * 128.0f) + 128; *snd_out++ = bound(0, val, 255);
				val = (int)(painted_ptr->sample[2] * 128.0f) + 128; *snd_out++ = bound(0, val, 255);
				val = (int)(painted_ptr->sample[3] * 128.0f) + 128; *snd_out++ = bound(0, val, 255);
				val = (int)(painted_ptr->sample[4] * 128.0f) + 128; *snd_out++ = bound(0, val, 255);
				val = (int)(painted_ptr->sample[5] * 128.0f) + 128; *snd_out++ = bound(0, val, 255);
			}
		}
		else if (nchannels == 4)  // 4.0 surround
		{
			for (i = 0; i < nbframes; i++, painted_ptr++)
			{
				val = (int)(painted_ptr->sample[0] * 128.0f) + 128; *snd_out++ = bound(0, val, 255);
				val = (int)(painted_ptr->sample[1] * 128.0f) + 128; *snd_out++ = bound(0, val, 255);
				val = (int)(painted_ptr->sample[2] * 128.0f) + 128; *snd_out++ = bound(0, val, 255);
				val = (int)(painted_ptr->sample[3] * 128.0f) + 128; *snd_out++ = bound(0, val, 255);
			}
		}
		else if (nchannels == 2)  // 2.0 stereo
		{
			for (i = 0; i < nbframes; i++, painted_ptr++)
			{
				val = (int)(painted_ptr->sample[0] * 128.0f) + 128; *snd_out++ = bound(0, val, 255);
				val = (int)(painted_ptr->sample[1] * 128.0f) + 128; *snd_out++ = bound(0, val, 255);
			}
		}
		else if (nchannels == 1)  // 1.0 mono
		{
			for (i = 0;i < nbframes;i++, painted_ptr++)
			{
				val = (int)((painted_ptr->sample[0] + painted_ptr->sample[1]) * 64.0f) + 128; *snd_out++ = bound(0, val, 255);
			}
		}
	}
}



/*
===============================================================================

UNDERWATER EFFECT

Muffles the intensity of sounds when the player is underwater

===============================================================================
*/

static struct
{
	float intensity;
	float alpha;
	float accum[SND_LISTENERS];
}
underwater = {0.f, 1.f, {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}};

void S_SetUnderwaterIntensity(void)
{
	float target = cl.view_underwater ? bound(0.f, snd_waterfx.value, 2.f) : 0.f;

	if (underwater.intensity < target)
	{
		underwater.intensity += cl.realframetime * 4.f;
		underwater.intensity = min(underwater.intensity, target);
	}
	else if (underwater.intensity > target)
	{
		underwater.intensity -= cl.realframetime * 4.f;
		underwater.intensity = max(underwater.intensity, target);
	}

	underwater.alpha = underwater.intensity ? exp(-underwater.intensity * log(12.f)) : 1.f;
}

static void S_UnderwaterFilter(int endtime)
{
	int i;
	int sl;

	if (!underwater.intensity)
	{
		if (endtime > 0)
			for (sl = 0; sl < SND_LISTENERS; sl++)
				underwater.accum[sl] = paintbuffer[endtime-1].sample[sl];
		return;
	}

	for (i = 0; i < endtime; i++)
		for (sl = 0; sl < SND_LISTENERS; sl++)
		{
			underwater.accum[sl] += underwater.alpha * (paintbuffer[i].sample[sl] - underwater.accum[sl]);
			paintbuffer[i].sample[sl] = underwater.accum[sl];
		}
}



/*
===============================================================================

CHANNEL MIXING

===============================================================================
*/

void S_MixToBuffer(void *stream, unsigned int bufferframes)
{
	int channelindex;
	channel_t *ch;
	int totalmixframes;
	unsigned char *outbytes = (unsigned char *) stream;
	sfx_t *sfx;
	portable_sampleframe_t *paint;
	int wantframes;
	int i;
	int count;
	int fetched;
	int fetch;
	int istartframe;
	int iendframe;
	int ilengthframes;
	int totallength;
	int loopstart;
	int indexfrac;
	int indexfracstep;
#define S_FETCHBUFFERSIZE 4096
	float fetchsampleframes[S_FETCHBUFFERSIZE*2];
	const float *fetchsampleframe;
	float vol[SND_LISTENERS];
	float lerp[2];
	float sample[3];
	double posd;
	double speedd;
	float maxvol;
	qbool looping;
	qbool silent;

	// mix as many times as needed to fill the requested buffer
	while (bufferframes)
	{
		// limit to the size of the paint buffer
		totalmixframes = min(bufferframes, PAINTBUFFER_SIZE);

		// clear the paint buffer
		memset(paintbuffer, 0, totalmixframes * sizeof(paintbuffer[0]));

		// paint in the channels.
		// channels with zero volumes still advance in time but don't paint.
		ch = channels; // cppcheck complains here but it is wrong, channels is a channel_t[MAX_CHANNELS] and not an int
		for (channelindex = 0;channelindex < (int)total_channels;channelindex++, ch++)
		{
			sfx = ch->sfx;
			if (sfx == NULL)
				continue;
			if (!S_LoadSound (sfx, true))
				continue;
			if (ch->flags & CHANNELFLAG_PAUSED)
				continue;
			if (!sfx->total_length)
				continue;

			// copy the channel information to the stack for reference, otherwise the
			// values might change during a mix if the spatializer is updating them
			// (note: this still may get some old and some new values!)
			posd = ch->position;
			speedd = ch->mixspeed * sfx->format.speed / snd_renderbuffer->format.speed;
			for (i = 0;i < SND_LISTENERS;i++)
				vol[i] = ch->volume[i];

			// check total volume level, because we can skip some code on silent sounds but other code must still run (position updates mainly)
			maxvol = 0;
			for (i = 0;i < SND_LISTENERS;i++)
				if(vol[i] > maxvol)
					maxvol = vol[i];
			switch(snd_renderbuffer->format.width)
			{
				case 1: // 8bpp
					silent = maxvol < (1.0f / (256.0f));
					// so silent it has zero effect
					break;
				case 2: // 16bpp
					silent = maxvol < (1.0f / (65536.0f));
					// so silent it has zero effect
					break;
				default: // floating point
					silent = maxvol < 1.0e-13f;
					// 130 dB is difference between hearing
					// threshold and a jackhammer from
					// working distance.
					// therefore, anyone who turns up
					// volume so much they notice this
					// cutoff, likely already has their
					// ear-drums blown out anyway.
					break;
			}

			// when doing prologic mixing, some channels invert one side
			if (ch->prologic_invert == -1)
				vol[1] *= -1.0f;

			// get some sfx info in a consistent form
			totallength = sfx->total_length;
			loopstart = (int)sfx->loopstart < totallength ? (int)sfx->loopstart : ((ch->flags & CHANNELFLAG_FORCELOOP) ? 0 : totallength);
			looping = loopstart < totallength;

			// do the actual paint now (may skip work if silent)
			paint = paintbuffer;
			istartframe = 0;
			for (wantframes = totalmixframes;wantframes > 0;posd += count * speedd, wantframes -= count)
			{
				// check if this is a delayed sound
				if (posd < 0)
				{
					// for a delayed sound we have to eat into the delay first
					count = (int)floor(-posd / speedd) + 1;
					count = bound(1, count, wantframes);
					// let the for loop iterator apply the skip
					continue;
				}

				// compute a fetch size that won't overflow our buffer
				count = wantframes;
				for (;;)
				{
					istartframe = (int)floor(posd);
					iendframe = (int)floor(posd + (count-1) * speedd);
					ilengthframes = count > 1 ? (iendframe - istartframe + 2) : 2;
					if (ilengthframes <= S_FETCHBUFFERSIZE)
						break;
					// reduce count by 25% and try again
					count -= count >> 2;
				}

				// zero whole fetch buffer for safety
				// (floating point noise from uninitialized memory = HORRIBLE)
				// otherwise we would only need to clear the excess
				if (!silent)
					memset(fetchsampleframes, 0, ilengthframes*sfx->format.channels*sizeof(fetchsampleframes[0]));

				// if looping, do multiple fetches
				fetched = 0;
				for (;;)
				{
					fetch = min(ilengthframes - fetched, totallength - istartframe);
					if (fetch > 0)
					{
						if (!silent)
							sfx->fetcher->getsamplesfloat(ch, sfx, istartframe, fetch, fetchsampleframes + fetched*sfx->format.channels);
						istartframe += fetch;
						fetched += fetch;
					}
					if (istartframe == totallength && looping && fetched < ilengthframes)
					{
						// loop and fetch some more
						posd += loopstart - totallength;
						istartframe = loopstart;
					}
					else
					{
						break;
					}
				}

				// set up our fixedpoint resampling variables (float to int conversions are expensive so do not do one per sampleframe)
				fetchsampleframe = fetchsampleframes;
				indexfrac = (int)floor((posd - floor(posd)) * 65536.0);
				indexfracstep = (int)floor(speedd * 65536.0);
				if (!silent)
				{
					if (sfx->format.channels == 2)
					{
						// music is stereo
#if SND_LISTENERS != 8
#error the following code only supports up to 8 channels, update it
#endif
						if (snd_speakerlayout.channels > 2)
						{
							// surround mixing
							for (i = 0;i < count;i++, paint++)
							{
								lerp[1] = indexfrac * (1.0f / 65536.0f);
								lerp[0] = 1.0f - lerp[1];
								sample[0] = fetchsampleframe[0] * lerp[0] + fetchsampleframe[2] * lerp[1];
								sample[1] = fetchsampleframe[1] * lerp[0] + fetchsampleframe[3] * lerp[1];
								sample[2] = (sample[0] + sample[1]) * 0.5f;
								paint->sample[0] += sample[0] * vol[0];
								paint->sample[1] += sample[1] * vol[1];
								paint->sample[2] += sample[0] * vol[2];
								paint->sample[3] += sample[1] * vol[3];
								paint->sample[4] += sample[2] * vol[4];
								paint->sample[5] += sample[2] * vol[5];
								paint->sample[6] += sample[0] * vol[6];
								paint->sample[7] += sample[1] * vol[7];
								indexfrac += indexfracstep;
								fetchsampleframe += 2 * (indexfrac >> 16);
								indexfrac &= 0xFFFF;
							}
						}
						else
						{
							// stereo mixing
							for (i = 0;i < count;i++, paint++)
							{
								lerp[1] = indexfrac * (1.0f / 65536.0f);
								lerp[0] = 1.0f - lerp[1];
								sample[0] = fetchsampleframe[0] * lerp[0] + fetchsampleframe[2] * lerp[1];
								sample[1] = fetchsampleframe[1] * lerp[0] + fetchsampleframe[3] * lerp[1];
								paint->sample[0] += sample[0] * vol[0];
								paint->sample[1] += sample[1] * vol[1];
								indexfrac += indexfracstep;
								fetchsampleframe += 2 * (indexfrac >> 16);
								indexfrac &= 0xFFFF;
							}
						}
					}
					else if (sfx->format.channels == 1)
					{
						// most sounds are mono
#if SND_LISTENERS != 8
#error the following code only supports up to 8 channels, update it
#endif
						if (snd_speakerlayout.channels > 2)
						{
							// surround mixing
							for (i = 0;i < count;i++, paint++)
							{
								lerp[1] = indexfrac * (1.0f / 65536.0f);
								lerp[0] = 1.0f - lerp[1];
								sample[0] = fetchsampleframe[0] * lerp[0] + fetchsampleframe[1] * lerp[1];
								paint->sample[0] += sample[0] * vol[0];
								paint->sample[1] += sample[0] * vol[1];
								paint->sample[2] += sample[0] * vol[2];
								paint->sample[3] += sample[0] * vol[3];
								paint->sample[4] += sample[0] * vol[4];
								paint->sample[5] += sample[0] * vol[5];
								paint->sample[6] += sample[0] * vol[6];
								paint->sample[7] += sample[0] * vol[7];
								indexfrac += indexfracstep;
								fetchsampleframe += (indexfrac >> 16);
								indexfrac &= 0xFFFF;
							}
						}
						else
						{
							// stereo mixing
							for (i = 0;i < count;i++, paint++)
							{
								lerp[1] = indexfrac * (1.0f / 65536.0f);
								lerp[0] = 1.0f - lerp[1];
								sample[0] = fetchsampleframe[0] * lerp[0] + fetchsampleframe[1] * lerp[1];
								paint->sample[0] += sample[0] * vol[0];
								paint->sample[1] += sample[0] * vol[1];
								indexfrac += indexfracstep;
								fetchsampleframe += (indexfrac >> 16);
								indexfrac &= 0xFFFF;
							}
						}
					}
				}
			}
			ch->position = posd;
			if (!looping && istartframe == totallength)
				S_StopChannel(ch - channels, false, false);
		}

		S_SoftClipPaintBuffer(paintbuffer, totalmixframes, snd_renderbuffer->format.width, snd_renderbuffer->format.channels);

		S_UnderwaterFilter(totalmixframes);


#ifdef CONFIG_VIDEO_CAPTURE
		if (!snd_usethreadedmixing)
			S_CaptureAVISound(paintbuffer, totalmixframes);
#endif

		S_ConvertPaintBuffer(paintbuffer, outbytes, totalmixframes, snd_renderbuffer->format.width, snd_renderbuffer->format.channels);

		// advance the output pointer
		outbytes += totalmixframes * snd_renderbuffer->format.width * snd_renderbuffer->format.channels;
		bufferframes -= totalmixframes;
	}
}
