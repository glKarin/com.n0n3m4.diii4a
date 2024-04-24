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


#include "darkplaces.h"

#include "snd_main.h"
#include "snd_ogg.h"
#include "snd_wav.h"
#ifdef USEXMP
#include "snd_xmp.h"
#endif
#include "sound.h"

void SCR_PushLoadingScreen (const char *, float);
void SCR_PopLoadingScreen (qbool);

/*
====================
Snd_CreateRingBuffer

If "buffer" is NULL, the function allocates one buffer of "sampleframes" sample frames itself
(if "sampleframes" is 0, the function chooses the size).
====================
*/
snd_ringbuffer_t *Snd_CreateRingBuffer (const snd_format_t* format, unsigned int sampleframes, void* buffer)
{
	snd_ringbuffer_t *ringbuffer;

	// If the caller provides a buffer, it must give us its size
	if (sampleframes == 0 && buffer != NULL)
		return NULL;

	ringbuffer = (snd_ringbuffer_t*)Mem_Alloc(snd_mempool, sizeof (*ringbuffer));
	memset(ringbuffer, 0, sizeof(*ringbuffer));
	memcpy(&ringbuffer->format, format, sizeof(ringbuffer->format));

	// If we haven't been given a buffer
	if (buffer == NULL)
	{
		unsigned int maxframes;
		size_t memsize;

		if (sampleframes == 0)
			maxframes = (format->speed + 1) / 2;  // Make the sound buffer large enough for containing 0.5 sec of sound
		else
			maxframes = sampleframes;

		memsize = maxframes * format->width * format->channels;
		ringbuffer->ring = (unsigned char *) Mem_Alloc(snd_mempool, memsize);
		ringbuffer->maxframes = maxframes;
	}
	else
	{
		ringbuffer->ring = (unsigned char *) buffer;
		ringbuffer->maxframes = sampleframes;
	}

	return ringbuffer;
}


//=============================================================================

/*
==============
S_LoadSound
==============
*/
qbool S_LoadSound (sfx_t *sfx, qbool complain)
{
	char namebuffer[MAX_QPATH + 16];
	size_t len;

	// See if already loaded
	if (sfx->fetcher != NULL)
		return true;

	// If we weren't able to load it previously, no need to retry
	// Note: S_PrecacheSound clears this flag to cause a retry
	if (sfx->flags & SFXFLAG_FILEMISSING)
		return false;

	// No sound?
	if (snd_renderbuffer == NULL)
		return false;

	// Initialize volume peak to 0; if ReplayGain is supported, the loader will change this away
	sfx->volume_peak = 0.0;

	if (developer_loading.integer)
		Con_Printf("loading sound %s\n", sfx->name);

	SCR_PushLoadingScreen(sfx->name, 1);

	// LadyHavoc: if the sound filename does not begin with sound/, try adding it
	if (strncasecmp(sfx->name, "sound/", 6))
	{
		dpsnprintf (namebuffer, sizeof(namebuffer), "sound/%s", sfx->name);
		len = strlen(namebuffer);
		if (len >= 4 && !strcasecmp (namebuffer + len - 4, ".wav"))
		{
			if (S_LoadWavFile (namebuffer, sfx))
				goto loaded;
			memcpy (namebuffer + len - 3, "ogg", 4);
		}
		if (len >= 4 && !strcasecmp (namebuffer + len - 4, ".ogg"))
		{
			if (OGG_LoadVorbisFile (namebuffer, sfx))
				goto loaded;
		}
#ifdef USEXMP
		else if (len >= 1)
		{
			if (XMP_LoadModFile (namebuffer, sfx))
				goto loaded;
		}
#endif
	}

	// LadyHavoc: then try without the added sound/ as wav and ogg
	dpsnprintf (namebuffer, sizeof(namebuffer), "%s", sfx->name);
	len = strlen(namebuffer);
	// request foo.wav: tries foo.wav, then foo.ogg
	// request foo.ogg: tries foo.ogg only
	// request foo.mod: tries foo.mod only
	if (len >= 4 && !strcasecmp (namebuffer + len - 4, ".wav"))
	{
		if (S_LoadWavFile (namebuffer, sfx))
			goto loaded;
		memcpy (namebuffer + len - 3, "ogg", 4);
	}
	if (len >= 4 && !strcasecmp (namebuffer + len - 4, ".ogg"))
	{
		if (OGG_LoadVorbisFile (namebuffer, sfx))
			goto loaded;
	}
#ifdef USEXMP
	else if (len >= 1)
	{
		if (XMP_LoadModFile (namebuffer, sfx))
			goto loaded;
	}
#endif

	// Can't load the sound!
	sfx->flags |= SFXFLAG_FILEMISSING;
	if (complain)
		Con_Printf(CON_ERROR "Failed to load sound \"%s\"\n", sfx->name);

	SCR_PopLoadingScreen(false);
	return false;

loaded:
	SCR_PopLoadingScreen(false);
	return true;
}
