/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include <stdlib.h>
#include <stdio.h>

#include "../qcommon/q_shared.h"
#include "../client/snd_local.h"
#include "../client/client.h"

#include "oboeaudio/snd_oboe.h"

qboolean snd_inited = qfalse;

/* The audio callback. All the magic happens here. */
static int dmapos = 0;
static int dmasize = 0;


/*
===============
SNDDMA_AudioCallback
===============
*/
static void SNDDMA_AudioCallback(unsigned char*stream, int len)
{
	int pos = (dmapos * (dma.samplebits/8));
	if (pos >= dmasize)
		dmapos = pos = 0;

	if (!snd_inited)  /* shouldn't happen, but just in case... */
	{
		memset(stream, '\0', len);
		return;
	}
	else
	{
		int tobufend = dmasize - pos;  /* bytes to buffer's end. */
		int len1 = len;
		int len2 = 0;

		if (len1 > tobufend)
		{
			len1 = tobufend;
			len2 = len - len1;
		}
		memcpy(stream, dma.buffer + pos, len1);
		if (len2 <= 0)
			dmapos += (len1 / (dma.samplebits/8));
		else  /* wraparound? */
		{
			memcpy(stream+len1, dma.buffer, len2);
			dmapos = (len2 / (dma.samplebits/8));
		}
	}

	if (dmapos >= dmasize)
		dmapos = 0;
}

/*
===============
SNDDMA_Init
===============
*/
qboolean SNDDMA_Init(void)
{
	if (snd_inited)
		return qtrue;

	Com_Printf("Initializing Android Sound subsystem\n");

    int sampleFrequencyInKHz = 44;
	int freq;
	switch (sampleFrequencyInKHz)
	{
		default:
		case 44: freq = 44100; break;
		case 22: freq = 22050; break;
		case 11: freq = 11025; break;
	}

	int samples;
    // just pick a sane default.
    if (freq <= 11025)
        samples = 256;
    else if (freq <= 22050)
        samples = 512;
    else if (freq <= 44100)
        samples = 1024;
    else
        samples = 2048;  // (*shrug*)

    int channels = 2;
    int bits = 16; // 2 * 8

	int tmp = samples * channels * 10;
	if (tmp & (tmp - 1))  // not a power of two? Seems to confuse something.
	{
		int val = 1;
		while (val < tmp)
			val <<= 1;

		tmp = val;
	}

	dmapos = 0;
	dma.samplebits = bits;  // first byte of format is bits.
	dma.channels = channels;
	dma.samples = tmp;
	dma.submission_chunk = 1;
	dma.speed = freq;
	dmasize = (dma.samples * (dma.samplebits/8));
	dma.buffer = (byte *)calloc(1, dmasize);

	dma.fullsamples = dma.samples / dma.channels;
	dma.isfloat = 0;

	Com_Printf("Q3E Oboe audio initialized.\n");
	Q3E_Oboe_Init(dma.speed, dma.channels, -1, SNDDMA_AudioCallback);
	Q3E_Oboe_Start();

	snd_inited = qtrue;
	Com_Printf("Starting Q3E Oboe audio callback...\n");
	return qtrue;
}

/*
===============
SNDDMA_GetDMAPos
===============
*/
int SNDDMA_GetDMAPos(void)
{
	return dmapos;
}

/*
===============
SNDDMA_Shutdown
===============
*/
void SNDDMA_Shutdown(void)
{
	Com_Printf("Closing Q3E audio audio device...\n");
	Q3E_Oboe_Shutdown();
	free(dma.buffer);
	dma.buffer = NULL;
	dmapos = dmasize = 0;
	snd_inited = qfalse;
	Com_Printf("Q3E Oboe audio shutdown.\n");
}

/*
===============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
void SNDDMA_Submit(void)
{
	Q3E_Oboe_Unlock();
}

/*
===============
SNDDMA_BeginPainting
===============
*/
void SNDDMA_BeginPainting (void)
{
	Q3E_Oboe_Lock();
}
