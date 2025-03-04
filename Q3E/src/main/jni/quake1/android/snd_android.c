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

		sdlaudiotime += RequestedFrames;

		SndSys_UnlockRenderBuffer();
	}
}

/*
====================
SndSys_Init

Create "snd_renderbuffer" with the proper sound format if the call is successful
May return a suggested format if the requested format isn't available
====================
*/
qbool SndSys_Init (snd_format_t* fmt)
{
	unsigned int buffersize;
	snd_threaded = false;

	Con_DPrint ("SndSys_Init: using the Oboe module\n");

	buffersize = bound(512, ceil((double)fmt->speed * snd_bufferlength.value / 1000.0), 8192);

	int freq = fmt->speed;
	if(fmt->width == 1)
	    fmt->width = 2;
	int format = fmt->width == 2 ? Q3E_OBOE_FORMAT_SINT16 : Q3E_OBOE_FORMAT_FLOAT;
	int channels = fmt->channels;
	int samples = CeilPowerOf2(buffersize);  // needs to be a power of 2 on some platforms.

	Con_Printf("Audio Specification:\n"
				"    Channels  : %i\n"
				"    Format    : 0x%X\n"
				"    Frequency : %i\n"
				"    Samples   : %i\n",
				channels, format, freq, samples);

    fmt->speed = freq;
    fmt->channels = channels;

	snd_renderbuffer = Snd_CreateRingBuffer(fmt, 0, NULL);
	Q3E_Oboe_Init(freq, channels, format, Buffer_Callback);
	Q3E_Oboe_Start();

	snd_threaded = true;

	if (snd_channellayout.integer == SND_CHANNELLAYOUT_AUTO)
		Cvar_SetValueQuick (&snd_channellayout, SND_CHANNELLAYOUT_STANDARD);

	sdlaudiotime = 0;

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


/*
====================
SndSys_GetSoundTime

Returns the number of sample frames consumed since the sound started
====================
*/
unsigned int SndSys_GetSoundTime (void)
{
	return sdlaudiotime;
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
