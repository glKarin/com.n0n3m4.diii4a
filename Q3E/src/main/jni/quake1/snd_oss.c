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

// OSS module, used by Linux and FreeBSD

#include "quakedef.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <unistd.h>

#include "snd_main.h"


#define NB_FRAGMENTS 4

static int audio_fd = -1;
static int old_osstime = 0;
static unsigned int osssoundtime;


/*
====================
SndSys_Init

Create "snd_renderbuffer" with the proper sound format if the call is successful
May return a suggested format if the requested format isn't available
====================
*/
qbool SndSys_Init (const snd_format_t* requested, snd_format_t* suggested)
{
	int flags, ioctl_param, prev_value;
	unsigned int fragmentsize;

	Con_DPrint("SndSys_Init: using the OSS module\n");

	// Check the requested sound format
	if (requested->width < 1 || requested->width > 2)
	{
		Con_Printf("SndSys_Init: invalid sound width (%hu)\n",
					requested->width);

		if (suggested != NULL)
		{
			memcpy(suggested, requested, sizeof(*suggested));

			if (requested->width < 1)
				suggested->width = 1;
			else
				suggested->width = 2;
		}
		
		return false;
    }

	// Open /dev/dsp
    audio_fd = open("/dev/dsp", O_WRONLY);
	if (audio_fd < 0)
	{
		perror("/dev/dsp");
		Con_Print("SndSys_Init: could not open /dev/dsp\n");
		return false;
	}
	
	// Use non-blocking IOs if possible
	flags = fcntl(audio_fd, F_GETFL);
	if (flags != -1)
	{
		if (fcntl(audio_fd, F_SETFL, flags | O_NONBLOCK) == -1)
			Con_Print("SndSys_Init : fcntl(F_SETFL, O_NONBLOCK) failed!\n");
	}
	else
		Con_Print("SndSys_Init: fcntl(F_GETFL) failed!\n");
	
	// Set the fragment size (up to "NB_FRAGMENTS" fragments of "fragmentsize" bytes)
	fragmentsize = requested->speed * requested->channels * requested->width / 10;
	fragmentsize = (unsigned int)ceilf((float)fragmentsize / (float)NB_FRAGMENTS);
	fragmentsize = CeilPowerOf2(fragmentsize);
	ioctl_param = (NB_FRAGMENTS << 16) | log2i(fragmentsize);
	if (ioctl(audio_fd, SNDCTL_DSP_SETFRAGMENT, &ioctl_param) == -1)
	{
		Con_Print ("SndSys_Init: could not set the fragment size\n");
		SndSys_Shutdown ();
		return false;
	}
	Con_Printf ("SndSys_Init: using %u fragments of %u bytes\n",
				ioctl_param >> 16, 1 << (ioctl_param & 0xFFFF));

	// Set the sound width
	if (requested->width == 1)
		ioctl_param = AFMT_U8;
	else
		ioctl_param = AFMT_S16_NE;
	prev_value = ioctl_param;
	if (ioctl(audio_fd, SNDCTL_DSP_SETFMT, &ioctl_param) == -1 ||
		ioctl_param != prev_value)
	{
		if (ioctl_param != prev_value && suggested != NULL)
		{
			memcpy(suggested, requested, sizeof(*suggested));

			if (ioctl_param == AFMT_S16_NE)
				suggested->width = 2;
			else
				suggested->width = 1;
		}

		Con_Printf("SndSys_Init: could not set the sound width to %hu\n",
					requested->width);
		SndSys_Shutdown();
		return false;
	}

	// Set the sound channels
	ioctl_param = requested->channels;
	if (ioctl(audio_fd, SNDCTL_DSP_CHANNELS, &ioctl_param) == -1 ||
		ioctl_param != requested->channels)
	{
		if (ioctl_param != requested->channels && suggested != NULL)
		{
			memcpy(suggested, requested, sizeof(*suggested));
			suggested->channels = ioctl_param;
		}

		Con_Printf("SndSys_Init: could not set the number of channels to %hu\n",
					requested->channels);
		SndSys_Shutdown();
		return false;
	}

	// Set the sound speed
	ioctl_param = requested->speed;
	if (ioctl(audio_fd, SNDCTL_DSP_SPEED, &ioctl_param) == -1 ||
		(unsigned int)ioctl_param != requested->speed)
	{
		if ((unsigned int)ioctl_param != requested->speed && suggested != NULL)
		{
			memcpy(suggested, requested, sizeof(*suggested));
			suggested->speed = ioctl_param;
		}

		Con_Printf("SndSys_Init: could not set the sound speed to %u\n",
					requested->speed);
		SndSys_Shutdown();
		return false;
	}

	// TOCHECK: I'm not sure which channel layout OSS uses for 5.1 and 7.1
	if (snd_channellayout.integer == SND_CHANNELLAYOUT_AUTO)
		Cvar_SetValueQuick (&snd_channellayout, SND_CHANNELLAYOUT_ALSA);

	old_osstime = 0;
	osssoundtime = 0;
	snd_renderbuffer = Snd_CreateRingBuffer(requested, 0, NULL);
	return true;
}


/*
====================
SndSys_Shutdown

Stop the sound card, delete "snd_renderbuffer" and free its other resources
====================
*/
void SndSys_Shutdown (void)
{
	// Stop the sound and close the device
	if (audio_fd >= 0)
	{
		ioctl(audio_fd, SNDCTL_DSP_RESET, NULL);
		close(audio_fd);
		audio_fd = -1;
	}

	if (snd_renderbuffer != NULL)
	{
		Mem_Free(snd_renderbuffer->ring);
		Mem_Free(snd_renderbuffer);
		snd_renderbuffer = NULL;
	}
}


/*
====================
SndSys_Write
====================
*/
static int SndSys_Write (const unsigned char* buffer, unsigned int nb_bytes)
{
	int written;
	unsigned int factor;

	written = write (audio_fd, buffer, nb_bytes);
	if (written < 0)
	{
		if (errno != EAGAIN)
			Con_Printf ("SndSys_Write: audio write returned %d! (errno= %d)\n",
						written, errno);
		return written;
	}

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
	unsigned int startoffset, factor, limit, nbframes;
	int written;
	
	if (audio_fd < 0 ||
		snd_renderbuffer->startframe == snd_renderbuffer->endframe)
		return;

	startoffset = snd_renderbuffer->startframe % snd_renderbuffer->maxframes;
	factor = snd_renderbuffer->format.width * snd_renderbuffer->format.channels;
	limit = snd_renderbuffer->maxframes - startoffset;
	nbframes = snd_renderbuffer->endframe - snd_renderbuffer->startframe;
	if (nbframes > limit)
	{
		written = SndSys_Write (&snd_renderbuffer->ring[startoffset * factor], limit * factor);
		if (written < 0 || (unsigned int)written < limit * factor)
			return;
		
		nbframes -= limit;
		startoffset = 0;
	}

	SndSys_Write (&snd_renderbuffer->ring[startoffset * factor], nbframes * factor);
}


/*
====================
SndSys_GetSoundTime

Returns the number of sample frames consumed since the sound started
====================
*/
unsigned int SndSys_GetSoundTime (void)
{
	struct count_info count;
	int new_osstime;
	unsigned int timediff;

	if (ioctl (audio_fd, SNDCTL_DSP_GETOPTR, &count) == -1)
	{
		Con_Print ("SndSys_GetSoundTimeDiff: can't ioctl (SNDCTL_DSP_GETOPTR)\n");
		return 0;
	}
	new_osstime = count.bytes / (snd_renderbuffer->format.width * snd_renderbuffer->format.channels);

	if (new_osstime >= old_osstime)
		timediff = new_osstime - old_osstime;
	else
	{
		Con_Print ("SndSys_GetSoundTime: osstime wrapped\n");
		timediff = 0;
	}

	old_osstime = new_osstime;
	osssoundtime += timediff;
	return osssoundtime;
}


/*
====================
SndSys_LockRenderBuffer

Get the exclusive lock on "snd_renderbuffer"
====================
*/
qbool SndSys_LockRenderBuffer (void)
{
	// Nothing to do
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
	// Nothing to do
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
