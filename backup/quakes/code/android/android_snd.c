#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <pthread.h>

#include "../client/snd_local.h"
#include "../qcommon/q_shared.h"

#include "oboeaudio/snd_oboe.h"

#ifdef __ANDROID__ //karin: Q3E
#define BUFFER_SIZE 4096
#else
#define BUFFER_SIZE (64*1024)
#endif

static int bytes_per_sample=0;
static int chunkSizeBytes=0;

/* The audio callback. All the magic happens here. */
static int dmapos = 0;
static int dmasize = 0;

/* engine variables */

extern cvar_t *s_khz;
extern cvar_t *s_device;

static qboolean snd_inited = qfalse;

/* we will use static dma buffer */
static unsigned char buffer[ BUFFER_SIZE ];

static int buffer_sz;					// buffers size, in bytes
static int frame_sz;					// frame size, in bytes


void Snd_Memset( void* dest, const int val, const size_t count )
{
    Com_Memset( dest, val, count );
}


void SNDDMA_BeginPainting( void )
{
	Q3E_Oboe_Lock();
}


void SNDDMA_Submit( void )
{
	Q3E_Oboe_Unlock();
}

typedef enum {
	SND_MODE_ASYNC,
	SND_MODE_MMAP,
	SND_MODE_DIRECT
} smode_t;


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

qboolean SNDDMA_Init( void )
{
	if (snd_inited)
		return qtrue;

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

    buffer_sz = dma.samples * bytes_per_sample;
    dma.buffer = calloc(1, buffer_sz);
#else
	buffer_sz = BUFFER_SIZE;
	dma.samples = BUFFER_SIZE / bytes_per_sample;
	dma.submission_chunk = 1;
	dma.buffer = calloc(1, buffer_sz);
#endif

	dmasize = (dma.samples * (dma.samplebits/8));
	chunkSizeBytes = dma.submission_chunk * bytes_per_sample;
	dmapos = 0;
	dma.fullsamples = dma.samples / dma.channels;
	dma.isfloat = 0;

	Com_Printf("Q3E Oboe audio initialized.\n");
	Q3E_Oboe_Init(dma.speed, dma.channels, -1, SNDDMA_AudioCallback);
	Q3E_Oboe_Start();
	snd_inited = qtrue;

	Com_Printf("Starting Q3E Oboe audio callback...\n");
	return qtrue;
}


void SNDDMA_Shutdown( void )
{
	Q3E_Oboe_Shutdown();
	free(dma.buffer);
	dma.buffer = NULL;
	dmapos = dmasize = 0;
	snd_inited = qfalse;
	Com_Printf("Q3E Oboe audio shut down.\n");
}


/*
==============
SNDDMA_GetDMAPos
return the current sample position (in mono samples read)
inside the recirculating dma buffer, so the mixing code will know
how many sample are required to fill it up.
===============
*/
int SNDDMA_GetDMAPos( void )
{
	return dmapos;
}

