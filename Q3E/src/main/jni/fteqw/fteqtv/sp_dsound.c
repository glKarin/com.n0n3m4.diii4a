#include "qtv.h"
#ifdef COMMENTARY
#include <dsound.h>

static HANDLE hInstDS;
static HRESULT (WINAPI *pDirectSoundCreate)(LPGUID, LPDIRECTSOUND *, LPUNKNOWN);

typedef struct {
	soundplay_t funcs;

	LPDIRECTSOUND ds;
	LPDIRECTSOUNDBUFFER dsbuf;

	int buffersize;

	int writepos;
	int readpos;

	int sampbytes;

} dsplay_t;

int DSOUND_UpdatePlayback(soundplay_t *sp, int samplechunks, char *buffer)
{
	int ret;
	dsplay_t *dsp = (dsplay_t*)sp;
	char *sbuf;
	int sbufsize;
	int writable;
	int remaining = samplechunks;

	if (!samplechunks)
		return 0;

	IDirectSoundBuffer_GetCurrentPosition(dsp->dsbuf, &dsp->readpos, NULL);
	dsp->readpos /= dsp->sampbytes;

	while (ret = IDirectSoundBuffer_Lock(dsp->dsbuf, 0, dsp->buffersize*dsp->sampbytes, (void**)&sbuf, &sbufsize, NULL, NULL, 0))
	{
		if (!FAILED(ret))
			break;
		if (ret == DSERR_BUFFERLOST)
			printf("Buffer lost\n");
		else
			break;

//			if (FAILED(IDirectSoundBuffer_Resore(dsp->dsbuf)))
//				return 0;
	}
	//memset(sbuf, 0, sbufsize);
	writable = remaining;
	if (writable > sbufsize/dsp->sampbytes - dsp->writepos)
		writable = sbufsize/dsp->sampbytes - dsp->writepos;
	memcpy(sbuf+dsp->writepos*dsp->sampbytes, buffer, writable*dsp->sampbytes);
	remaining -= writable;
	buffer += writable*dsp->sampbytes;
	dsp->writepos += writable;
	dsp->writepos %= dsp->buffersize;
	if (samplechunks > 0)
	{
		writable = remaining;
		if (writable > dsp->readpos)
			writable = dsp->readpos;
		memcpy(sbuf, buffer, writable*dsp->sampbytes);
		remaining -= writable;
		dsp->writepos += writable;
		dsp->writepos %= dsp->buffersize;
	}
	IDirectSoundBuffer_Unlock(dsp->dsbuf, sbuf, sbufsize, NULL, 0);

	printf("%i %i\n", 100*dsp->readpos / dsp->buffersize, 100*dsp->writepos / dsp->buffersize);

	return samplechunks - remaining;
}

void DSOUND_Shutdown(soundplay_t *dsp)
{
}

soundplay_t *SND_InitPlayback(int speed, int bits)
{
	int ret;
	DSBCAPS caps;
	DSBUFFERDESC bufdesc;
	LPDIRECTSOUND ds;
	dsplay_t *hnd;
	WAVEFORMATEX	format; 

	if (!hInstDS)
	{
		hInstDS = LoadLibrary("dsound.dll");
		
		if (hInstDS == NULL)
		{
			printf ("Couldn't load dsound.dll\n");
			return NULL;
		}

		pDirectSoundCreate = (void *)GetProcAddress(hInstDS,"DirectSoundCreate");

		if (!pDirectSoundCreate)
		{
			printf ("Couldn't get DS proc addr\n");
			return NULL;
		}
				
//		pDirectSoundEnumerate = (void *)GetProcAddress(hInstDS,"DirectSoundEnumerateA");
	}

	ds = NULL;
	pDirectSoundCreate(NULL, &ds, NULL);

	if (!ds)
		return NULL;
	hnd = malloc(sizeof(*hnd));
	memset(hnd, 0, sizeof(*hnd));

	hnd->funcs.update = DSOUND_UpdatePlayback;
	hnd->funcs.close = DSOUND_Shutdown;

	hnd->ds = ds;
	hnd->sampbytes = bits/8;

	if (FAILED(IDirectSound_SetCooperativeLevel (hnd->ds, GetDesktopWindow(), DSSCL_EXCLUSIVE)))
		printf("SetCooperativeLevel failed\n");

	memset(&bufdesc, 0, sizeof(bufdesc));
	bufdesc.dwSize = sizeof(bufdesc);
//	bufdesc.dwFlags |= DSBCAPS_GLOBALFOCUS;	//so we hear it if quake is loaded
	bufdesc.dwFlags |= DSBCAPS_PRIMARYBUFFER; //so we can set speed
	bufdesc.dwFlags |= DSBCAPS_CTRLVOLUME;
	bufdesc.lpwfxFormat = NULL;
	bufdesc.dwBufferBytes = 0;

	format.wFormatTag = WAVE_FORMAT_PCM;
	format.cbSize = 0;

	format.nChannels = 1;
    format.wBitsPerSample = bits;
    format.nSamplesPerSec = speed;
    format.nBlockAlign = format.nChannels * format.wBitsPerSample / 8;
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

	ret = IDirectSound_CreateSoundBuffer(hnd->ds, &bufdesc, &hnd->dsbuf, NULL);

	if (!hnd->dsbuf)
	{
		printf("Couldn't create primary buffer\n");
		DSOUND_Shutdown(&hnd->funcs);
		return NULL;
	}

	if (FAILED(IDirectSoundBuffer_SetFormat(hnd->dsbuf, &format)))
		printf("SetFormat failed\n");

	//and now make a secondary buffer
	bufdesc.dwFlags = 0;
	bufdesc.dwFlags |= DSBCAPS_CTRLFREQUENCY;
	bufdesc.dwFlags |= DSBCAPS_LOCSOFTWARE;
	bufdesc.dwFlags |= DSBCAPS_GLOBALFOCUS;
	bufdesc.dwBufferBytes = speed * format.nChannels * hnd->sampbytes;
	bufdesc.lpwfxFormat = &format;

	ret = IDirectSound_CreateSoundBuffer(hnd->ds, &bufdesc, &hnd->dsbuf, NULL);
	if (!hnd->dsbuf)
	{
		printf("Couldn't create secondary buffer\n");
		DSOUND_Shutdown(&hnd->funcs);
		return NULL;
	}








	memset(&caps, 0, sizeof(caps));
	caps.dwSize = sizeof(caps);
	IDirectSoundBuffer_GetCaps(hnd->dsbuf, &caps);
	hnd->buffersize = caps.dwBufferBytes / hnd->sampbytes;

	//clear out the buffer
	{
		char *buffer;
		int buffersize=0;
		IDirectSoundBuffer_Play(hnd->dsbuf, 0, 0, DSBPLAY_LOOPING);
		ret = IDirectSoundBuffer_Lock(hnd->dsbuf, 0, hnd->buffersize*hnd->sampbytes, (void**)&buffer, &buffersize, NULL, NULL, 0);
		memset(buffer, 0, buffersize);
		IDirectSoundBuffer_Unlock(hnd->dsbuf, buffer, buffersize, NULL, 0);
		IDirectSoundBuffer_Stop(hnd->dsbuf);
	}
//DSERR_INVALIDPARAM
	IDirectSoundBuffer_Play(hnd->dsbuf, 0, 0, DSBPLAY_LOOPING);
	

	IDirectSoundBuffer_GetCurrentPosition(hnd->dsbuf, &hnd->readpos, &hnd->writepos);

	hnd->writepos = hnd->readpos + speed / 2;	//position our write position a quater of a second infront of the read position

	printf("%i %i\n", 100*hnd->readpos / hnd->buffersize, 100*hnd->writepos / hnd->buffersize);

	return &hnd->funcs;
}


/*


void soundtest(void)
{
	int speed = 22100*2;
	int bits = 16;

	int i;

	int sampsavailable;
	short buffer[1024];
	soundcapt_t *capt;
	soundplay_t *play;

	capt = SND_InitCapture (speed, bits);
	if (!capt)
	{
		printf("Failed to init capturer\n");
		exit(0);
	}

	play = SND_InitPlayback (speed, bits);
	if (!play)
	{
		printf("Failed to init playback\n");
		exit(0);
	}




	for(;;)
	{
		for (i = 0; i < sizeof(buffer)/sizeof(buffer[0]); i++)
			buffer[i] = rand();
		sampsavailable = capt->update(capt, sizeof(buffer)/(bits/8), (char*)buffer);
		play->update(play, sampsavailable, (char*)buffer);

		Sleep(1);
	}
}
*/
#endif
