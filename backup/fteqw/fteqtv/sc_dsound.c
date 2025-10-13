#include "qtv.h"

#ifdef COMMENTARY
#include <dsound.h>

static HANDLE hInstDS;
static HRESULT (WINAPI *pDirectSoundCaptureCreate)(GUID FAR *lpGUID, LPDIRECTSOUNDCAPTURE FAR *lplpDS, IUnknown FAR *pUnkOuter);

typedef struct {
	soundcapt_t funcs;

	LPDIRECTSOUNDCAPTURE DSCapture;
	LPDIRECTSOUNDCAPTUREBUFFER DSCaptureBuffer;
	long lastreadpos;

	WAVEFORMATEX  wfxFormat;

	long bufferbytes;
} dscapture_t;


void DSOUND_CloseCapture(soundcapt_t *ghnd)
{
	dscapture_t *hnd = (dscapture_t*)ghnd;

	if (hnd->DSCaptureBuffer)
	{
		IDirectSoundCaptureBuffer_Stop(hnd->DSCaptureBuffer);
		IDirectSoundCaptureBuffer_Release(hnd->DSCaptureBuffer);
		hnd->DSCaptureBuffer=NULL;
	}
	if (hnd->DSCapture)
	{
		IDirectSoundCapture_Release(hnd->DSCapture);
		hnd->DSCapture=NULL;
	}

	free(hnd);
}

int DSOUND_UpdateCapture(soundcapt_t *ghnd, int samplechunks, char *buffer)
{
	dscapture_t *hnd = (dscapture_t*)ghnd;
	HRESULT hr;
	LPBYTE lpbuf1 = NULL;
	LPBYTE lpbuf2 = NULL;
	DWORD dwsize1 = 0;
	DWORD dwsize2 = 0;

	DWORD capturePos;
	DWORD readPos;
	long  filled;

	int inputbytes;

	inputbytes = hnd->wfxFormat.wBitsPerSample/8;


// Query to see how much data is in buffer.
	hr = IDirectSoundCaptureBuffer_GetCurrentPosition(hnd->DSCaptureBuffer, &capturePos, &readPos );
	if( hr != DS_OK )
	{
		return 0;
	}
	filled = readPos - hnd->lastreadpos;
	if( filled < 0 )
		filled += hnd->bufferbytes; // unwrap offset

	if (filled > samplechunks)	//figure out how much we need to empty it by, and if that's enough to be worthwhile.
		filled = samplechunks;
	else if (filled < samplechunks)
		return 0;

	if ((filled/inputbytes) & 1)	//force even numbers of samples
		filled -= inputbytes;

	// Lock free space in the DS
	hr = IDirectSoundCaptureBuffer_Lock ( hnd->DSCaptureBuffer, hnd->lastreadpos, filled, (void **) &lpbuf1, &dwsize1,
		(void **) &lpbuf2, &dwsize2, 0);
	if (hr == DS_OK)
	{
		// Copy from DS to the buffer
		memcpy( buffer, lpbuf1, dwsize1);
		if(lpbuf2 != NULL)
		{
			memcpy( buffer+dwsize1, lpbuf2, dwsize2);
		}
		// Update our buffer offset and unlock sound buffer
 		hnd->lastreadpos = (hnd->lastreadpos + dwsize1 + dwsize2) % (hnd->bufferbytes);
		IDirectSoundCaptureBuffer_Unlock ( hnd->DSCaptureBuffer, lpbuf1, dwsize1, lpbuf2, dwsize2);
	}
	else
	{
		return 0;
	}
	return filled/inputbytes;
}


soundcapt_t *SND_InitCapture (int speed, int bits)
{
	dscapture_t *hnd;
	DSCBUFFERDESC bufdesc;

	if (!hInstDS)
	{
		hInstDS = LoadLibrary("dsound.dll");
		
		if (hInstDS == NULL)
		{
			printf ("Couldn't load dsound.dll\n");
			return NULL;
		}
	}
	if (!pDirectSoundCaptureCreate)
	{
		pDirectSoundCaptureCreate = (void *)GetProcAddress(hInstDS,"DirectSoundCaptureCreate");

		if (!pDirectSoundCaptureCreate)
		{
			printf ("Couldn't get DS proc addr (DirectSoundCaptureCreate)\n");
			return NULL;
		}

	}


	hnd = malloc(sizeof(dscapture_t));
	memset(hnd, 0, sizeof(*hnd));

	hnd->wfxFormat.wFormatTag = WAVE_FORMAT_PCM;
    hnd->wfxFormat.nChannels = 1;
    hnd->wfxFormat.nSamplesPerSec = speed;
	hnd->wfxFormat.wBitsPerSample = bits;
    hnd->wfxFormat.nBlockAlign = hnd->wfxFormat.nChannels * (hnd->wfxFormat.wBitsPerSample / 8);
	hnd->wfxFormat.nAvgBytesPerSec = hnd->wfxFormat.nSamplesPerSec * hnd->wfxFormat.nBlockAlign;
    hnd->wfxFormat.cbSize = 0;

	bufdesc.dwSize = sizeof(bufdesc);
	bufdesc.dwBufferBytes = hnd->bufferbytes = speed*bits/8;	//1 sec
	bufdesc.dwFlags = 0;
	bufdesc.dwReserved = 0;
	bufdesc.lpwfxFormat = &hnd->wfxFormat;




	pDirectSoundCaptureCreate(NULL, &hnd->DSCapture, NULL);

	if (FAILED(IDirectSoundCapture_CreateCaptureBuffer(hnd->DSCapture, &bufdesc, &hnd->DSCaptureBuffer, NULL)))
	{
		printf ("Couldn't create a capture buffer\n");
		IDirectSoundCapture_Release(hnd->DSCapture);
		free(hnd);
		return NULL;
	}

	IDirectSoundCaptureBuffer_Start(hnd->DSCaptureBuffer, DSBPLAY_LOOPING);

	hnd->lastreadpos = 0;

	hnd->funcs.update = DSOUND_UpdateCapture;
	hnd->funcs.close = DSOUND_CloseCapture;

	return &hnd->funcs;
}




/*
void soundtestcallback (char *buffer, int samples, int bitspersample)
{
	FILE *f;
	f = fopen("c:/test.raw", "at");
	fseek(f, 0, SEEK_END);
	fwrite(buffer, samples, bitspersample/8, f);
	fclose(f);
}

void soundtest(void)
{
	soundcapt_t *capt;

	capt = SNDDMA_InitCapture(11025, 8);
	while(1)
		capt->update(capt, 1400, soundtestcallback);
	capt->close(capt);
}
*/
#endif
