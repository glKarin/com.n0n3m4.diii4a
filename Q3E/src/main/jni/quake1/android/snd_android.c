/*
Copyright (C) 2004 Andreas Kirsch

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
#include <math.h>

#include "../darkplaces.h"
#include "../vid.h"

#include "../snd_main.h"

#include "oboeaudio/snd_oboe.h"

//karin: using oboe as callback audio, not use Q3E AudioTrack

static unsigned int sdlaudiotime = 0;
static int audio_device = 0;
static unsigned int audiopos = 0;
static unsigned int buffersize;

static void Buffer_Callback (unsigned char*stream, int len)
{
	unsigned int factor, RequestedFrames, MaxFrames, FrameCount;
	unsigned int StartOffset, EndOffset;

	factor = snd_renderbuffer->format.channels * snd_renderbuffer->format.width;
	if ((unsigned int)len % factor != 0)
		Sys_Error("SDL sound: invalid buffer length passed to Buffer_Callback (%d bytes)\n", len);

	RequestedFrames = (unsigned int)len / factor;

	if (SndSys_LockRenderBuffer())
	{
		if (snd_usethreadedmixing)
		{
			S_MixToBuffer(stream, RequestedFrames);
			if (snd_blocked)
				memset(stream, snd_renderbuffer->format.width == 1 ? 0x80 : 0, len);
			SndSys_UnlockRenderBuffer();
			return;
		}

		// Transfert up to a chunk of samples from snd_renderbuffer to stream
		MaxFrames = snd_renderbuffer->endframe - snd_renderbuffer->startframe;
		if (MaxFrames > RequestedFrames)
			FrameCount = RequestedFrames;
		else
			FrameCount = MaxFrames;
		StartOffset = snd_renderbuffer->startframe % snd_renderbuffer->maxframes;
		EndOffset = (snd_renderbuffer->startframe + FrameCount) % snd_renderbuffer->maxframes;
		if (StartOffset > EndOffset)  // if the buffer wraps
		{
			unsigned int PartialLength1, PartialLength2;

			PartialLength1 = (snd_renderbuffer->maxframes - StartOffset) * factor;
			memcpy(stream, &snd_renderbuffer->ring[StartOffset * factor], PartialLength1);

			PartialLength2 = FrameCount * factor - PartialLength1;
			memcpy(&stream[PartialLength1], &snd_renderbuffer->ring[0], PartialLength2);

			// As of SDL 2.0 buffer needs to be fully initialized, so fill leftover part with silence
			// FIXME this is another place that assumes 8bit is always unsigned and others always signed
			memset(&stream[PartialLength1 + PartialLength2], snd_renderbuffer->format.width == 1 ? 0x80 : 0, len - (PartialLength1 + PartialLength2));
		}
		else
		{
			memcpy(stream, &snd_renderbuffer->ring[StartOffset * factor], FrameCount * factor);

			// As of SDL 2.0 buffer needs to be fully initialized, so fill leftover part with silence
			// FIXME this is another place that assumes 8bit is always unsigned and others always signed
			memset(&stream[FrameCount * factor], snd_renderbuffer->format.width == 1 ? 0x80 : 0, len - (FrameCount * factor));
		}

		snd_renderbuffer->startframe += FrameCount;

		if (FrameCount < RequestedFrames && developer_insane.integer && vid_activewindow)
			Con_DPrintf("SDL sound: %u sample frames missing\n", RequestedFrames - FrameCount);

		audiopos += RequestedFrames;

		SndSys_UnlockRenderBuffer();
	}
}

#define SAMPLES 0 // 16384

/*
====================
SndSys_Init

Create "snd_renderbuffer" with the proper sound format if the call is successful
May return a suggested format if the requested format isn't available
====================
*/
qbool SndSys_Init (snd_format_t* fmt)
{
	snd_threaded = false;

	Con_DPrint ("SndSys_Init: using the ANDROID module\n");

	int wantspecfreq = fmt->speed;
	int wantspecformat = (fmt->width);
	int wantspecchannels = fmt->channels;

	Con_Printf("Wanted audio Specification:\n"
			   "\tChannels  : %i\n"
			   "\tFormat    : 0x%X\n"
			   "\tFrequency : %i\n",
			   wantspecchannels, wantspecformat, wantspecfreq);

	int obtainspecchannels=2;
	int obtainspecfreq=44100;
	int obtainspecformat=2;
    fmt->speed = obtainspecfreq;
    fmt->width = obtainspecformat;
    fmt->channels = obtainspecchannels;

	Con_Printf("Obtained audio specification:\n"
			   "\tChannels  : %i\n"
			   "\tFormat    : 0x%X\n"
			   "\tFrequency : %i\n",
			   obtainspecchannels, obtainspecformat, obtainspecfreq);

	// If we haven't obtained what we wanted
/*	if (wantspecfreq != obtainspecfreq ||
		wantspecformat != obtainspecformat ||
		wantspecchannels != obtainspecchannels)
	{
		// Pass the obtained format as a suggested format
		if (suggested != NULL)
		{
			suggested->speed = obtainspecfreq;
			suggested->width = obtainspecformat;
			suggested->channels = obtainspecchannels;
		}

		return false;
	}*/

	snd_threaded = false;
//TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	snd_renderbuffer = Snd_CreateRingBuffer(fmt, SAMPLES, 0);
	if (snd_channellayout.integer == SND_CHANNELLAYOUT_AUTO)
		Cvar_SetValueQuick (&snd_channellayout, SND_CHANNELLAYOUT_STANDARD);

	audiopos = 0;
	buffersize=snd_renderbuffer->maxframes*2*2;
	//initAudio(snd_renderbuffer->ring, buffersize);

	snd_threaded = true;
	Q3E_Oboe_Init(fmt->speed, fmt->channels, -1, Buffer_Callback);
	Q3E_Oboe_Start();

	return true;
}


/*
====================
SndSys_Shutdown

Stop the sound card, delete "snd_renderbuffer" and free its other resources
====================
*/
void SndSys_Shutdown(void)
{
	Q3E_Oboe_Shutdown();
	if (snd_renderbuffer != NULL)
	{
		Mem_Free(snd_renderbuffer->ring);
		Mem_Free(snd_renderbuffer);
		snd_renderbuffer = NULL;
	}
}

#if 1
/*
====================
SndSys_Submit

Submit the contents of "snd_renderbuffer" to the sound card
====================
*/
void SndSys_Submit (void)
{
	// Nothing to do here (this sound module is callback-based)
}
#else
/*
====================
SndSys_Write
====================
*/
static int SndSys_Write (unsigned int buffer, unsigned int nb_bytes)
{
    int written;
    unsigned int factor;

	if(nb_bytes == 0)
		return 0;

    written = writeAudio (buffer, nb_bytes);

    factor = snd_renderbuffer->format.width * snd_renderbuffer->format.channels;
    if (written % factor != 0)
        Sys_Error ("SndSys_Write: nb of bytes written (%d) isn't aligned to a frame sample!\n",
                   written);

    snd_renderbuffer->startframe += written / factor;

    if ((unsigned int)written < nb_bytes)
    {
        Con_DPrintf("SndSys_Submit: audio can't keep up! (%u < %u)\n",
                    written, nb_bytes);
    }

    return written;
}
/*
====================
SndSys_Submit

Submit the contents of "snd_renderbuffer" to the sound card
====================
*/
void SndSys_Submit (void)
{
	// Nothing to do here (this sound module is callback-based)
/*	int offset = (audiopos*4) & (buffersize - 1);
	if (snd_renderbuffer!=NULL)
		writeAudio(offset, 2048*4);
	audiopos+=2048;*/

#if 0
    unsigned int startoffset, factor, limit, nbframes;
    int written;

    if (snd_renderbuffer->startframe == snd_renderbuffer->endframe)
        return;

    startoffset = snd_renderbuffer->startframe % snd_renderbuffer->maxframes;
    factor = snd_renderbuffer->format.width * snd_renderbuffer->format.channels;
    limit = snd_renderbuffer->maxframes - startoffset;
    nbframes = snd_renderbuffer->endframe - snd_renderbuffer->startframe;
    if (nbframes > limit)
    {
        written = SndSys_Write (startoffset * factor, limit * factor);
        if (written < 0 || (unsigned int)written < limit * factor)
            return;

        nbframes -= limit;
		audiopos += limit;
        startoffset = 0;
    }

    SndSys_Write (startoffset * factor, nbframes * factor);
	audiopos += nbframes;
#endif

#if 1
	unsigned int factor, MaxFrames, FrameCount;
	unsigned int StartOffset, EndOffset;

	factor = snd_renderbuffer->format.channels * snd_renderbuffer->format.width;
	MaxFrames = snd_renderbuffer->endframe - snd_renderbuffer->startframe;
	unsigned int RequestedFrames = MaxFrames;
	if (MaxFrames > RequestedFrames)
		FrameCount = RequestedFrames;
	else
		FrameCount = MaxFrames;
	StartOffset = snd_renderbuffer->startframe % snd_renderbuffer->maxframes;
	EndOffset = (snd_renderbuffer->startframe + FrameCount) % snd_renderbuffer->maxframes;

	if (StartOffset > EndOffset)  // if the buffer wraps
	{
		unsigned int PartialLength1, PartialLength2;
		unsigned int ui = snd_renderbuffer->maxframes - StartOffset;

		PartialLength1 = ui * factor;
		int r = writeAudio(StartOffset * factor, PartialLength1);
        printf("AAA %d %d = %d %d\n", StartOffset * factor, PartialLength1, ui, (int)(r / factor));
		r = (int)(r / factor);
		snd_renderbuffer->startframe += r;
		audiopos += ui;

		PartialLength2 = FrameCount * factor - PartialLength1;
		if(PartialLength2 > 0)
		{
			ui = FrameCount - ui;
            r = writeAudio(0, PartialLength2);
            printf("BBB %d %d = %d %d\n", 0, PartialLength2, ui, (int)(r / factor));
			r = (int)(r / factor);
			snd_renderbuffer->startframe += r;
			audiopos += ui;
		}
	}
	else
	{
        int r = writeAudio(StartOffset * factor, FrameCount * factor);
		printf("CCC %d %d = %d %d\n", StartOffset * factor, FrameCount * factor, FrameCount, (int)(r / factor));
		r = (int)(r / factor);
		snd_renderbuffer->startframe += r;
		audiopos += FrameCount;
	}
	//snd_renderbuffer->startframe += FrameCount;
	//audiopos += RequestedFrames;
    // else the buffer is empty
#endif
}
#endif


/*
====================
SndSys_GetSoundTime

Returns the number of sample frames consumed since the sound started
====================
*/
unsigned int SndSys_GetSoundTime (void)
{
	return audiopos;
}


/*
====================
SndSys_LockRenderBuffer

Get the exclusive lock on "snd_renderbuffer"
====================
*/
qbool SndSys_LockRenderBuffer (void)
{
	Q3E_Oboe_Lock();
	return true;
}


/*
====================
SndSys_UnlockRenderBuffer

Release the exclusive lock on "snd_renderbuffer"
====================
*/
void SndSys_UnlockRenderBuffer (void)
{
	Q3E_Oboe_Unlock();
}

/*
====================
SndSys_SendKeyEvents

Send keyboard events originating from the sound system (e.g. MIDI)
====================
*/
void SndSys_SendKeyEvents(void)
{
	// not supported
}
