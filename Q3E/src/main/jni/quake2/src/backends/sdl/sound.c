/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2010 Yamagi Burmeister
 * Copyright (C) 2005 Ryan C. Gordon
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 * USA.
 *
 * =======================================================================
 *
 * The lower layer of the sound system. It utilizes SDL for writing the
 * sound data to the device. This file was rewritten by Yamagi to solve
 * a lot problems arrising from the old Icculus Quake 2 SDL backend like
 * stuttering and cracking. This implementation is based on ioQuake3s
 * snd_sdl.c.
 *
 * =======================================================================
 */

#include "../../client/header/client.h"
#include "../../client/sound/header/local.h"

#define BUFFER_SIZE 4096

static int buf_size=0;
static int bytes_per_sample=0;
static int chunkSizeBytes=0;
static int dmapos=0;
static int snd_inited=0;

#ifdef __ANDROID__
extern void (*initAudio)(void *buffer, int size);
extern int (*writeAudio)(int offset, int length);
extern void (*shutdownAudio)(void);
#endif

qboolean SNDDMA_Init(void)
{
    if (snd_inited)
    {
	return 1;
    }

    Com_Printf("Initializing Android Sound subsystem\n");

    dma.channels = 2;
    dma.samplebits = 16;
    dma.samplepos = 0;
    dma.speed = 44100;
    bytes_per_sample = dma.samplebits/8;
#if 0
    dma.samples = 32768;

    dma.submission_chunk = BUFFER_SIZE;

    buf_size = dma.samples * bytes_per_sample;
    dma.buffer = calloc(1, buf_size);
#else
    buf_size = BUFFER_SIZE;
    dma.samples = BUFFER_SIZE / bytes_per_sample;
    dma.submission_chunk = 1;
    dma.buffer = calloc(1, buf_size);
#endif
    chunkSizeBytes = dma.submission_chunk * bytes_per_sample;

    s_numchannels = MAX_CHANNELS;
    S_InitScaletable();

    initAudio(dma.buffer, buf_size);
    snd_inited=1;

    return 1;
}


int SNDDMA_GetDMAPos(void)
{
    if(snd_inited)
        return dmapos;
    else
        Com_Printf ("Sound not inizialized\n");
    return 0;
}

void SNDDMA_Shutdown(void)
{
    Com_Printf("SNDDMA_ShutDown\n");
    if (snd_inited) {
        snd_inited = 0;
        shutdownAudio();
    }
    if(dma.buffer)
    {
        free(dma.buffer);
        dma.buffer = NULL;
    }
}

void
SNDDMA_Submit(void)
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

void
SNDDMA_BeginPainting(void)
{
}

