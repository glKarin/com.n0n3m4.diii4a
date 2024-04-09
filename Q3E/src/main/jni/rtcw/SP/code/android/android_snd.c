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

qboolean snd_inited = qfalse;

/* The audio callback. All the magic happens here. */
#define BUFFER_SIZE 4096

static int buf_size=0;
static int bytes_per_sample=0;
static int chunkSizeBytes=0;
static int dmapos=0;

extern void (*initAudio)(void *buffer, int size);
extern void (*writeAudio)(int offset, int length);
extern void (*shutdownAudio)(void);



/*
===============
SNDDMA_Init
===============
*/
qboolean SNDDMA_Init(void)
{
	Com_Printf("Initializing Android Sound subsystem\n");

	/* For now hardcode this all :) */
	dma.channels = 2;
	dma.samplebits = 16;

	dma.submission_chunk = 4096; /* This is in single samples, so this would equal 2048 frames (assuming stereo) in Android terminology */
	dma.speed = 44100; /* This is the native sample frequency of the Milestone */

	bytes_per_sample = dma.samplebits/8;
#if 0
	dma.samples = 32768;

    dma.submission_chunk = BUFFER_SIZE; /* This is in single samples, so this would equal 2048 frames (assuming stereo) in Android terminology */

    buf_size = dma.samples * bytes_per_sample;
    dma.buffer = calloc(1, buf_size);
#else
	buf_size = BUFFER_SIZE;
	dma.samples = BUFFER_SIZE / bytes_per_sample;
	dma.submission_chunk = 1;
	dma.buffer = calloc(1, buf_size);
#endif

	chunkSizeBytes = dma.submission_chunk * bytes_per_sample;

	initAudio(dma.buffer, buf_size);

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
	Com_Printf("SNDDMA_ShutDown\n");
	if(dma.buffer)
	{
		shutdownAudio();
		free(dma.buffer);
		dma.buffer = NULL;
	}
}

/*
===============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
void SNDDMA_Submit(void)
{
#if 0
	int offset = (dmapos * bytes_per_sample) & (buf_size - 1);
    writeAudio(offset, chunkSizeBytes);
    dmapos+=dma.submission_chunk;
#else
	writeAudio(0, BUFFER_SIZE);
	/*dma.samplepos*/dmapos += BUFFER_SIZE/bytes_per_sample;
#endif
}

/*
===============
SNDDMA_BeginPainting
===============
*/
void SNDDMA_BeginPainting (void)
{
}


#ifdef USE_VOIP
void SNDDMA_StartCapture(void)
{
#ifdef USE_SDL_AUDIO_CAPTURE
	if (sdlCaptureDevice)
	{
		SDL_ClearQueuedAudio(sdlCaptureDevice);
		SDL_PauseAudioDevice(sdlCaptureDevice, 0);
	}
#endif
}

int SNDDMA_AvailableCaptureSamples(void)
{
#ifdef USE_SDL_AUDIO_CAPTURE
	// divided by 2 to convert from bytes to (mono16) samples.
	return sdlCaptureDevice ? (SDL_GetQueuedAudioSize(sdlCaptureDevice) / 2) : 0;
#else
	return 0;
#endif
}

void SNDDMA_Capture(int samples, byte *data)
{
#ifdef USE_SDL_AUDIO_CAPTURE
	// multiplied by 2 to convert from (mono16) samples to bytes.
	if (sdlCaptureDevice)
	{
		SDL_DequeueAudio(sdlCaptureDevice, data, samples * 2);
	}
	else
#endif
	{
		SDL_memset(data, '\0', samples * 2);
	}
}

void SNDDMA_StopCapture(void)
{
#ifdef USE_SDL_AUDIO_CAPTURE
	if (sdlCaptureDevice)
	{
		SDL_PauseAudioDevice(sdlCaptureDevice, 1);
	}
#endif
}

void SNDDMA_MasterGain( float val )
{
#ifdef USE_SDL_AUDIO_CAPTURE
	sdlMasterGain = val;
#endif
}
#endif

