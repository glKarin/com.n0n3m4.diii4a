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
#include "winquake.h"
#ifndef WINRT

// 64K is > 1 second at 16-bit, 22050 Hz
#define	WAV_BUFFERS				64
#define	WAV_MASK				0x3F
#define	WAV_BUFFER_SIZE			0x0400
#define SECONDARY_BUFFER_SIZE	0x10000

static void WAV_Submit(soundcardinfo_t *sc, int start, int end);

typedef struct {
	HWAVEOUT hWaveOut;
	HANDLE hData;
	HGLOBAL hWaveHdr;
	HPSTR lpData;
	LPWAVEHDR lpWaveHdr;
//	DWORD		mmstarttime;
	DWORD gSndBufSize;
} wavhandle_t;

/*
==================
S_BlockSound
==================
*/
//all devices
void S_BlockSound (void)
{
	soundcardinfo_t *sc;
	wavhandle_t *wh;

	snd_blocked++;

	for (sc = sndcardinfo; sc; sc=sc->next)
	{
		if (sc->Submit == WAV_Submit && !sc->inactive_sound)
		{
			wh = sc->handle;
			if (snd_blocked == 1)
				waveOutReset (wh->hWaveOut);
		}
	}
}


/*
==================
S_UnblockSound
==================
*/
//all devices
void S_UnblockSound (void)
{
	snd_blocked--;
}


static void *WAV_Lock (soundcardinfo_t *sc, unsigned int *sampidx)
{
	return sc->sn.buffer;
}
static void WAV_Unlock (soundcardinfo_t *sc, void *buffer)
{
}

/*
==================
FreeSound
==================
*/
//per device
static void WAV_Shutdown (soundcardinfo_t *sc)
{
	int		i;
	wavhandle_t *wh = sc->handle;

	if (!wh)
		return;
	sc->handle = NULL;

	waveOutReset (wh->hWaveOut);

	if (wh->lpWaveHdr)
	{
		for (i=0 ; i< WAV_BUFFERS ; i++)
			waveOutUnprepareHeader (wh->hWaveOut, wh->lpWaveHdr+i, sizeof(WAVEHDR));
	}

	waveOutClose (wh->hWaveOut);

	if (wh->hWaveHdr)
	{
		GlobalUnlock(wh->hWaveHdr);
		GlobalFree(wh->hWaveHdr);
	}

	if (wh->hData)
	{
		GlobalUnlock(wh->hData);
		GlobalFree(wh->hData);
	}

	wh->hWaveOut = 0;
	wh->hData = 0;
	wh->hWaveHdr = 0;
	wh->lpData = NULL;
	wh->lpWaveHdr = NULL;

	Z_Free(wh);
}

/*
==============
SNDDMA_GetDMAPos

return the current sample position (in mono samples read)
inside the recirculating dma buffer, so the mixing code will know
how many sample are required to fill it up.
===============
*/
static unsigned int WAV_GetDMAPos(soundcardinfo_t *sc)
{
	int		s;
	s = sc->snd_sent * WAV_BUFFER_SIZE;
	s >>= sc->sn.samplebytes - 1;
	return s;
}

/*
==============
WAV_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
static void WAV_Submit(soundcardinfo_t *sc, int start, int end)
{
	LPWAVEHDR	h;
	int			wResult;
	wavhandle_t *wh = sc->handle;
	int chunkstosubmit;

	//
	// find which sound blocks have completed
	//
	while (1)
	{
		if ( sc->snd_completed == sc->snd_sent )
		{
			Con_DPrintf ("Sound overrun\n");
			break;
		}

		if ( ! (wh->lpWaveHdr[ sc->snd_completed & WAV_MASK].dwFlags & WHDR_DONE) )
		{
			break;
		}

		sc->snd_completed++;	// this buffer has been played
	}

	//
	// submit two new sound blocks
	//
	if (sc->sn.speed <= 22050)
		chunkstosubmit = 4;
	else
		chunkstosubmit = 4 + (sc->sn.speed/6000);

	while (((sc->snd_sent - sc->snd_completed) >> (sc->sn.samplebytes - 1)) < chunkstosubmit)
	{
		h = wh->lpWaveHdr + ( sc->snd_sent&WAV_MASK );

		sc->snd_sent++;
		/*
		 * Now the data block can be sent to the output device. The
		 * waveOutWrite function returns immediately and waveform
		 * data is sent to the output device in the background.
		 */
		wResult = waveOutWrite(wh->hWaveOut, h, sizeof(WAVEHDR));

		if (wResult != MMSYSERR_NOERROR)
		{
			Con_SafePrintf ("Failed to write block to device\n");
			WAV_Shutdown (sc);
			return;
		}
	}
}



/*
==================
SNDDM_InitWav

Crappy windows multimedia base
==================
*/
qboolean QDECL WAV_InitCard (soundcardinfo_t *sc, const char *cardname)
{
	WAVEFORMATEX  format;
	int				i;
	HRESULT			hr;
	wavhandle_t *wh;

	if (cardname && *cardname)
		return false;	//we don't support explicit devices, so only accept default devices.

	wh = sc->handle = Z_Malloc(sizeof(wavhandle_t));

	sc->snd_sent = 0;
	sc->snd_completed = 0;

	if (sc->sn.speed > 48000) // limit waveout to 48000 until that buffer issue gets solved
		sc->sn.speed = 48000;

	if (sc->sn.samplebytes > 2)
	{
		sc->sn.samplebytes = 2;
		sc->sn.sampleformat = QSF_S16;
	}
	else
	{
		sc->sn.samplebytes = 1;
		sc->sn.sampleformat = QSF_U8;
	}

	memset (&format, 0, sizeof(format));
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = sc->sn.numchannels;
	format.wBitsPerSample = sc->sn.samplebytes*8;
	format.nSamplesPerSec = sc->sn.speed;
	format.nBlockAlign = format.nChannels
		*format.wBitsPerSample / 8;
	format.cbSize = 0;
	format.nAvgBytesPerSec = format.nSamplesPerSec
		*format.nBlockAlign;

	/* Open a waveform device for output using window callback. */
	while ((hr = waveOutOpen((LPHWAVEOUT)&wh->hWaveOut, WAVE_MAPPER,
					&format,
					0, 0L, CALLBACK_NULL)) != MMSYSERR_NOERROR)
	{
		if (hr != MMSYSERR_ALLOCATED)
		{
			if (hr == WAVERR_BADFORMAT)
				Con_SafePrintf (CON_ERROR "waveOutOpen failed, format not supported\n");
			else
				Con_SafePrintf (CON_ERROR "waveOutOpen failed, return code %i\n", (int)hr);
			WAV_Shutdown (sc);
			return false;
		}

//		if (MessageBox (NULL,
//						"The sound hardware is in use by another app.\n\n"
//					    "Select Retry to try to start sound again or Cancel to run Quake with no sound.",
//						"Sound not available",
//						MB_RETRYCANCEL | MB_SETFOREGROUND | MB_ICONEXCLAMATION) != IDRETRY)
//		{
			Con_SafePrintf (CON_ERROR "waveOutOpen failure;\n"
							"  hardware already in use\nclose the app, then try using snd_restart\n");
			WAV_Shutdown (sc);
			return false;
//		}
	}

	/*
	 * Allocate and lock memory for the waveform data. The memory
	 * for waveform data must be globally allocated with
	 * GMEM_MOVEABLE and GMEM_SHARE flags.

	*/
	wh->gSndBufSize = WAV_BUFFERS*WAV_BUFFER_SIZE;
	wh->hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, wh->gSndBufSize);
	if (!wh->hData)
	{
		Con_SafePrintf (CON_ERROR "Sound: Out of memory.\n");
		WAV_Shutdown (sc);
		return false;
	}
	wh->lpData = GlobalLock(wh->hData);
	if (!wh->lpData)
	{
		Con_SafePrintf (CON_ERROR "Sound: Failed to lock.\n");
		WAV_Shutdown (sc);
		return false;
	}
	memset (wh->lpData, 0, wh->gSndBufSize);

	/*
	 * Allocate and lock memory for the header. This memory must
	 * also be globally allocated with GMEM_MOVEABLE and
	 * GMEM_SHARE flags.
	 */
	wh->hWaveHdr = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE,
		(DWORD) sizeof(WAVEHDR) * WAV_BUFFERS);

	if (wh->hWaveHdr == NULL)
	{
		Con_SafePrintf (CON_ERROR "Sound: Failed to Alloc header.\n");
		WAV_Shutdown (sc);
		return false;
	}

	wh->lpWaveHdr = (LPWAVEHDR) GlobalLock(wh->hWaveHdr);

	if (wh->lpWaveHdr == NULL)
	{
		Con_SafePrintf (CON_ERROR "Sound: Failed to lock header.\n");
		WAV_Shutdown (sc);
		return false;
	}

	memset (wh->lpWaveHdr, 0, sizeof(WAVEHDR) * WAV_BUFFERS);

	/* After allocation, set up and prepare headers. */
	for (i=0 ; i<WAV_BUFFERS ; i++)
	{
		wh->lpWaveHdr[i].dwBufferLength = WAV_BUFFER_SIZE;
		wh->lpWaveHdr[i].lpData = wh->lpData + i*WAV_BUFFER_SIZE;

		if (waveOutPrepareHeader(wh->hWaveOut, wh->lpWaveHdr+i, sizeof(WAVEHDR)) !=
				MMSYSERR_NOERROR)
		{
			Con_SafePrintf (CON_ERROR "Sound: failed to prepare wave headers\n");
			WAV_Shutdown (sc);
			return false;
		}
	}

	sc->sn.samples = wh->gSndBufSize/sc->sn.samplebytes;
	sc->sn.samplepos = 0;
	sc->sn.buffer = (unsigned char *) wh->lpData;
	Q_strncpyz(sc->name, "wav out", sizeof(sc->name));


	sc->Lock		= WAV_Lock;
	sc->Unlock		= WAV_Unlock;
	sc->Submit		= WAV_Submit;
	sc->Shutdown	= WAV_Shutdown;
	sc->GetDMAPos	= WAV_GetDMAPos;

	return true;
}
//int (*pWAV_InitCard) (soundcardinfo_t *sc, int cardnum) = &WAV_InitCard;
sounddriver_t WaveOut_Output =
{
	"WaveOut",
	WAV_InitCard,
	NULL
};
#endif
