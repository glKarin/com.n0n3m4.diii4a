#include "quakedef.h"

#if defined(HAVE_MIXER) || defined(VOICECHAT)
#include "winquake.h"

#ifdef DYNAMIC_SDL
#define SDL_MAJOR_VERSION 2
#define SDL_MINOR_VERSION 0
#define SDL_PATCHLEVEL 5
#define SDL_VERSIONNUM(X, Y, Z)			((X)*1000 + (Y)*100 + (Z))
#define SDL_COMPILEDVERSION				SDL_VERSIONNUM(SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL)
#define SDL_VERSION_ATLEAST(X, Y, Z)	(SDL_COMPILEDVERSION >= SDL_VERSIONNUM(X, Y, Z))


//if we're not an sdl build, we probably want to link to sdl libraries dynamically or something.
#include <stdint.h>
#define SDL_AudioDeviceID uint32_t
#define SDL_INIT_AUDIO          0x00000010
#define SDL_INIT_NOPARACHUTE    0x00100000
#define SDL_AUDIO_ALLOW_FREQUENCY_CHANGE    0x00000001
#define SDL_AUDIO_ALLOW_FORMAT_CHANGE       0x00000002
#define SDL_AUDIO_ALLOW_CHANNELS_CHANGE     0x00000004
#define AUDIO_U8    0x0008
#define AUDIO_S8    0x8008
#define AUDIO_S16LSB    0x8010
#define AUDIO_S16MSB    0x9010
#define AUDIO_F32LSB    0x8120
#define AUDIO_F32MSB    0x9120
#if __BYTE_ORDER == __BIG_ENDIAN
#define AUDIO_S16SYS    AUDIO_S16MSB
#define AUDIO_F32SYS    AUDIO_F32MSB
#else
#define AUDIO_S16SYS    AUDIO_S16LSB
#define AUDIO_F32SYS    AUDIO_F32LSB
#endif
#define SDLCALL QDECL

typedef uint16_t SDL_AudioFormat;
typedef void (SDLCALL *SDL_AudioCallback)(void *userdata, uint8_t *stream, int len);

typedef struct SDL_AudioSpec
{
	int freq;
	SDL_AudioFormat format;
	uint8_t channels;
	uint8_t silence;
	uint16_t samples;
	uint16_t padding;
	uint32_t size;
	SDL_AudioCallback callback;
	void *userdata;
} SDL_AudioSpec;

static int (SDLCALL *SDL_Init)								(uint32_t flags);
static int (SDLCALL *SDL_InitSubSystem)						(uint32_t flags);
static SDL_AudioDeviceID (SDLCALL *SDL_OpenAudioDevice)		(const char *dev, int iscapture, const SDL_AudioSpec *desired, SDL_AudioSpec *obtained, int allowed_changes);
static void (SDLCALL *SDL_PauseAudioDevice)					(SDL_AudioDeviceID fd, int pausestate);
static void (SDLCALL *SDL_LockAudioDevice)					(SDL_AudioDeviceID fd);
static void (SDLCALL *SDL_UnlockAudioDevice)				(SDL_AudioDeviceID fd);
static int (SDLCALL *SDL_CloseAudioDevice)					(SDL_AudioDeviceID fd);
static int (SDLCALL *SDL_GetNumAudioDevices)				(int iscapture);
static const char *(SDLCALL *SDL_GetAudioDeviceName)		(int index, int iscapture);
static const char *(SDLCALL *SDL_GetError)					(void);
#if SDL_VERSION_ATLEAST(2,0,4)
static int (SDLCALL *SDL_QueueAudio)						(SDL_AudioDeviceID dev, const void *data, uint32_t len);
static uint32_t (SDLCALL *SDL_GetQueuedAudioSize)			(SDL_AudioDeviceID dev);
#endif
#if SDL_VERSION_ATLEAST(2,0,5)
static uint32_t (SDLCALL *SDL_DequeueAudio)					(SDL_AudioDeviceID dev, void *data, uint32_t len);
#endif
static dllfunction_t sdl_funcs[] =
{
	{(void*)&SDL_Init, "SDL_Init"},
	{(void*)&SDL_InitSubSystem, "SDL_InitSubSystem"},
	{(void*)&SDL_OpenAudioDevice, "SDL_OpenAudioDevice"},
	{(void*)&SDL_PauseAudioDevice, "SDL_PauseAudioDevice"},
	{(void*)&SDL_LockAudioDevice, "SDL_LockAudioDevice"},
	{(void*)&SDL_UnlockAudioDevice, "SDL_UnlockAudioDevice"},
	{(void*)&SDL_CloseAudioDevice, "SDL_CloseAudioDevice"},
	{(void*)&SDL_GetNumAudioDevices, "SDL_GetNumAudioDevices"},
	{(void*)&SDL_GetAudioDeviceName, "SDL_GetAudioDeviceName"},
	{(void*)&SDL_GetError, "SDL_GetError"},
#if SDL_VERSION_ATLEAST(2,0,4)
	{(void*)&SDL_QueueAudio, "SDL_QueueAudio"},
	{(void*)&SDL_GetQueuedAudioSize, "SDL_GetQueuedAudioSize"},
#endif
#if SDL_VERSION_ATLEAST(2,0,5)
	{(void*)&SDL_DequeueAudio, "SDL_DequeueAudio"},
#endif
	{NULL, NULL}
};
static dllhandle_t *libsdl;
#else
#include <SDL.h>
#endif
#define SDRVNAME "SDL"

//SDL calls a callback each time it needs to repaint the 'hardware' buffers
//This results in extra latency due it needing to buffer that much data.
//So we tell it a fairly pathetically sized buffer and try and get it to copy often
//hopefully this lowers sound latency, and has no suddenly starting sounds and stuff.
//It still has greater latency than direct access, of course.
//On the plus side, SDL calls the callback from another thread. this means we can set up some tiny buffer size, and if we're mixing inside the callback then you can actually get lower latency than waiting for an entire frame (yay rtlights)

static qboolean SSDL_InitAudio(void)
{
	static qboolean inited = false;
	if (COM_CheckParm("-nosdlsnd") || COM_CheckParm("-nosdl"))
		return false;
#ifdef DYNAMIC_SDL
	if (!libsdl)
	{
		libsdl = Sys_LoadLibrary("libSDL2-2.0.so.0", sdl_funcs);
		if (!libsdl)
			libsdl = Sys_LoadLibrary("libSDL2.so", sdl_funcs);	//maybe they have a dev package installed that fixes this mess.
#ifdef _WIN32
		if (!libsdl)
			libsdl = Sys_LoadLibrary("SDL2", sdl_funcs);
#endif
		if (libsdl)
			SDL_Init(SDL_INIT_NOPARACHUTE);
		else
		{
			Con_Printf("Unable to load libSDL2 library\n");
			return false;
		}
	}
#endif

	if (!inited)
	{
		if(0==SDL_InitSubSystem(SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE))
			inited = true;
		else
			Con_Printf("Couldn't initialize SDL audio subsystem (%s)\n", SDL_GetError());
	}
	return inited;
}
#else
#define SDL_VERSION_ATLEAST(x,y,z) 0
#endif

#ifdef HAVE_MIXER
#define SELFPAINT
static void SSDL_Shutdown(soundcardinfo_t *sc)
{
	Con_DPrintf("Shutdown SDL sound\n");

#if SDL_MAJOR_VERSION >= 2
	if (sc->audio_fd)
	{
		SDL_PauseAudioDevice(sc->audio_fd, 1);
		SDL_CloseAudioDevice(sc->audio_fd);
	}
#else
	SDL_CloseAudio();
#endif
#ifndef SELFPAINT
	if (sc->sn.buffer)
		free(sc->sn.buffer);
#endif
	sc->sn.buffer = NULL;
}
static unsigned int SSDL_Callback_GetDMAPos(soundcardinfo_t *sc)
{
	sc->sn.samplepos = sc->snd_sent / sc->sn.samplebytes;
	return sc->sn.samplepos;
}

//this function is called from inside SDL.
//transfer the 'dma' buffer into the buffer it requests.
static void VARGS SSDL_Callback_Paint(void *userdata, qbyte *stream, int len)
{
	soundcardinfo_t *sc = userdata;

#ifdef SELFPAINT
	sc->sn.buffer = stream;
	sc->sn.samples = len / sc->sn.samplebytes;
	sc->samplequeue = sc->sn.samples;
	S_MixerThread(sc);
	sc->snd_sent += len;
#else
	int buffersize = sc->sn.samples*sc->sn.samplebytes;

	if (len > buffersize)
	{
		len = buffersize;	//whoa nellie!
	}

	if (len + sc->snd_sent%buffersize > buffersize)
	{	//buffer will wrap, fill in the rest
		memcpy(stream, (char*)sc->sn.buffer + (sc->snd_sent%buffersize), buffersize - (sc->snd_sent%buffersize));
		stream += buffersize - sc->snd_sent%buffersize;
		sc->snd_sent += buffersize - sc->snd_sent%buffersize;
		len -= buffersize - (sc->snd_sent%buffersize);
		if (len < 0)
			return;
	}	//and finish from the start
	memcpy(stream, (char*)sc->sn.buffer + (sc->snd_sent%buffersize), len);
	sc->snd_sent += len;
#endif
}

static void *SSDL_Callback_LockBuffer(soundcardinfo_t *sc, unsigned int *sampidx)
{
#if SDL_MAJOR_VERSION >= 2
	SDL_LockAudioDevice(sc->audio_fd);
#else
	SDL_LockAudio();
#endif

	return sc->sn.buffer;
}

static void SSDL_Callback_UnlockBuffer(soundcardinfo_t *sc, void *buffer)
{
#if SDL_MAJOR_VERSION >= 2
	SDL_UnlockAudioDevice(sc->audio_fd);
#else
	SDL_UnlockAudio();
#endif
}

static void SSDL_Callback_Submit(soundcardinfo_t *sc, int start, int end)
{
	//SDL will call SSDL_Paint to paint when it's time, and the sound buffer is always there...
}

#if SDL_VERSION_ATLEAST(2,0,4)
static unsigned int SSDL_Queue_GetDMAPos(soundcardinfo_t *sc)
{	//keep proper track of how much data has actually been sent to the audio device.
	//note that SDL may have already submitted more than this to the physical device.
	//note: if we don't mix enough data then sdl will mix 0s for us.
	uint32_t queued = SDL_GetQueuedAudioSize(sc->audio_fd);
	extern cvar_t _snd_mixahead;
	int ahead = (_snd_mixahead.value*sc->sn.speed) - (queued / (sc->sn.samplebytes*sc->sn.numchannels));
	if (ahead < 0)
		ahead = 0;	//never behind
	sc->samplequeue = -1;	//return value is a desired timestamp
	return sc->sn.samplepos + ahead*sc->sn.numchannels;
}
static void *SSDL_Queue_LockBuffer(soundcardinfo_t *sc, unsigned int *sampidx)
{	//queuing uses private memory, so no need to lock
	*sampidx = 0;	//don't bother ringing it.
	return sc->sn.buffer;
}
static void SSDL_Queue_UnlockBuffer(soundcardinfo_t *sc, void *buffer)
{	//nor a need to unlock
}

static void SSDL_Queue_Submit(soundcardinfo_t *sc, int start, int end)
{
	int bytecount = (end-start)*sc->sn.samplebytes*sc->sn.numchannels;
	SDL_QueueAudio(sc->audio_fd, sc->sn.buffer, bytecount);

	sc->sn.samplepos += bytecount/sc->sn.samplebytes;
}
#endif

static qboolean QDECL SDL_InitCard(soundcardinfo_t *sc, const char *devicename)
{
	SDL_AudioSpec desired, obtained;

	if(!SSDL_InitAudio())
	{
		Con_Printf("Couldn't initialize SDL audio subsystem\n");
		return false;
	}

	memset(&desired, 0, sizeof(desired));

	desired.freq = sc->sn.speed;
	desired.channels = sc->sn.numchannels;	//fixme!
	desired.samples = 0x0200;	//'Good values seem to range between 512 and 8192 inclusive, depending on the application and CPU speed.'
#if SDL_VERSION_ATLEAST(2,0,4)
	if (!snd_mixerthread.ival)
		desired.callback = NULL;
	else
#endif
		desired.callback = (void*)SSDL_Callback_Paint;
	desired.userdata = sc;
	memcpy(&obtained, &desired, sizeof(obtained));

#if SDL_MAJOR_VERSION >= 2
	desired.format = AUDIO_F32SYS;	//most modern audio APIs favour float audio nowadays.
	sc->audio_fd = SDL_OpenAudioDevice(devicename, false, &desired, &obtained, (sndcardinfo?0:SDL_AUDIO_ALLOW_FREQUENCY_CHANGE) | SDL_AUDIO_ALLOW_CHANNELS_CHANGE | SDL_AUDIO_ALLOW_FORMAT_CHANGE);
	if (!sc->audio_fd)
	{
		Con_Printf("SDL_OpenAudioDevice(%s) failed: couldn't open sound device (%s).\n", devicename?devicename:"default", SDL_GetError());
		return false;
	}
	if (obtained.format != AUDIO_U8 && obtained.format != AUDIO_S8 && obtained.format != AUDIO_S16SYS && obtained.format != AUDIO_F32SYS)
	{	//can't cope with that... try again but force the format (so SDL converts)
		SDL_CloseAudioDevice(sc->audio_fd);
		sc->audio_fd = SDL_OpenAudioDevice(devicename, false, &desired, &obtained, (sndcardinfo?0:SDL_AUDIO_ALLOW_FREQUENCY_CHANGE) | SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
	}
	if (devicename)
		Con_Printf("Initing SDL audio device '%s'.\n", devicename);
	else
		Con_Printf("Initing default SDL audio device.\n");
#else
	desired.format = AUDIO_S16SYS;
	if (sndcardinfo)
		return false;	//SDL1 only supports opening one audio device at a time. the existing one might not be sdl, but I don't care.
	if (SDL_OpenAudio(&desired, &obtained) < 0)
	{
		Con_Printf("SDL_OpenAudio failed: couldn't open sound device (%s).\n", SDL_GetError());
		return false;
	}
	Con_Printf("Initing default SDL audio device.\n");
#endif
	sc->sn.numchannels = obtained.channels;
	sc->sn.speed = obtained.freq;
	sc->sn.samples = 32768;//*sc->sn.numchannels;	//doesn't really matter, so long as it's higher than obtained.samples

	switch(obtained.format)
	{
	case AUDIO_U8:
		sc->sn.samplebytes = 1;
		sc->sn.sampleformat = QSF_U8;
		break;
	case AUDIO_S8:
		sc->sn.samplebytes = 1;
		sc->sn.sampleformat = QSF_S8;
		break;
	case AUDIO_S16SYS:
		sc->sn.samplebytes = 2;
		sc->sn.sampleformat = QSF_S16;
		break;
#if SDL_MAJOR_VERSION >= 2
	case AUDIO_F32SYS:
		sc->sn.samplebytes = 4;
		sc->sn.sampleformat = QSF_F32;
		break;
#endif
	default:
		//unsupported. shouldn't have obtained that.
#if SDL_MAJOR_VERSION >= 2
		SDL_CloseAudioDevice(sc->audio_fd);
		sc->audio_fd = 0;
#else
		SDL_CloseAudio();
#endif
		break;
	}

	Con_DPrintf("channels: %i\n", sc->sn.numchannels);
	Con_DPrintf("Speed: %i\n", sc->sn.speed);
	Con_DPrintf("Samplebits: %i\n", sc->sn.samplebytes*8);
	Con_DPrintf("SDLSamples: %i (low for latency)\n", obtained.samples);
	Con_DPrintf("FakeSamples: %i\n", sc->sn.samples);

	Con_DPrintf("Got sound %i-%i\n", obtained.freq, obtained.format);

#if SDL_VERSION_ATLEAST(2,0,4)
	if (!obtained.callback)
	{
		sc->Lock		= SSDL_Queue_LockBuffer;
		sc->Unlock		= SSDL_Queue_UnlockBuffer;
		sc->Submit		= SSDL_Queue_Submit;
		sc->Shutdown	= SSDL_Shutdown;
		sc->GetDMAPos	= SSDL_Queue_GetDMAPos;

		sc->sn.buffer = malloc(sc->sn.samples*sc->sn.samplebytes);

		Con_DPrintf("Using SDL audio queues\n");
	}
	else
#endif
	{
		sc->Lock		= SSDL_Callback_LockBuffer;
		sc->Unlock		= SSDL_Callback_UnlockBuffer;
		sc->Submit		= SSDL_Callback_Submit;
		sc->Shutdown	= SSDL_Shutdown;
		sc->GetDMAPos	= SSDL_Callback_GetDMAPos;

#ifdef SELFPAINT
		sc->selfpainting = true;
		Con_DPrintf("Using SDL audio threading\n");
#else
		sc->sn.buffer = malloc(sc->sn.samples*sc->sn.samplebytes);
		Con_DPrintf("Using SDL audio callbacks\n");
#endif
	}

#if SDL_MAJOR_VERSION >= 2
	SDL_PauseAudioDevice(sc->audio_fd, 0);
#else
	SDL_PauseAudio(0);
#endif

	return true;
}

static qboolean QDECL SDL_Enumerate(void (QDECL *cb) (const char *drivername, const char *devicecode, const char *readablename))
{
#if SDL_MAJOR_VERSION >= 2
	int max, i;
	if(SSDL_InitAudio())
	{
		max = SDL_GetNumAudioDevices(false);
		for (i = 0; i < max; i++)
		{
			const char *devname = SDL_GetAudioDeviceName(i, false);
			if (devname)
				cb(SDRVNAME, devname, va("SDL:%s", devname));
		}
	}
	return true;
#else
	return false;
#endif
}

sounddriver_t SDL_Output =
{
	SDRVNAME,
	SDL_InitCard,
	SDL_Enumerate
};
#endif

#if SDL_VERSION_ATLEAST(2,0,5) && defined(VOICECHAT)
//Requires SDL 2.0.5+ supposedly.
//Bugging out for me on windows, with really low audio levels. looks like there's been some float->int conversion without a multiplier. asking for float audio gives stupidly low values too.
typedef struct
{
	SDL_AudioDeviceID dev;
} sdlcapture_t;

static void QDECL SDL_Capture_Start(void *ctx)
{
	sdlcapture_t *d = ctx;
	SDL_PauseAudioDevice(d->dev, false);
}

static void QDECL SDL_Capture_Stop(void *ctx)
{
	sdlcapture_t *d = ctx;
	SDL_PauseAudioDevice(d->dev, true);
}

static void QDECL SDL_Capture_Shutdown(void *ctx)
{
	sdlcapture_t *d = ctx;
	SDL_CloseAudioDevice(d->dev);
	Z_Free(d);
}

static qboolean QDECL SDL_Capture_Enumerate(void (QDECL *callback) (const char *drivername, const char *devicecode, const char *readablename))
{
	int i, count;
	if (SSDL_InitAudio())
	{
		count = SDL_GetNumAudioDevices(true);
		for (i = 0; i < count; i++)
		{
			const char *name = SDL_GetAudioDeviceName(i, true);
			if (name)
				callback(SDRVNAME, name, va("SDL:%s", name));
		}
	}
	return true;
}
static void *QDECL SDL_Capture_Init (int rate, const char *devname)
{
	SDL_AudioSpec want, have;
	sdlcapture_t c, *r;

	if (SSDL_InitAudio())
	{
		memset(&want, 0, sizeof(want));
		want.freq = rate;
		want.format = AUDIO_S16SYS;
		want.channels = 1;
		want.samples = 256;	//this seems to be chunk sizes rather than total buffer size, so lets keep it reasonably small for lower latencies
		want.callback = NULL;

		c.dev = SDL_OpenAudioDevice(devname, true, &want, &have, 0);
		if (c.dev)
		{
			r = Z_Malloc(sizeof(*r));
			*r = c;
			return r;
		}
	}
	return NULL;
}

/*minbytes is a hint to not bother wasting time*/
static unsigned int QDECL SDL_Capture_Update(void *ctx, unsigned char *buffer, unsigned int minbytes, unsigned int maxbytes)
{
	sdlcapture_t *c = ctx;
	unsigned int queuedsize = SDL_GetQueuedAudioSize(c->dev);
	if (queuedsize < minbytes)
		return 0;
	if (queuedsize > maxbytes)
		queuedsize = maxbytes;

	queuedsize = SDL_DequeueAudio(c->dev, buffer, queuedsize);
	return queuedsize;
}
snd_capture_driver_t SDL_Capture =
{
	1,
	SDRVNAME,
	SDL_Capture_Enumerate,
	SDL_Capture_Init,
	SDL_Capture_Start,
	SDL_Capture_Update,
	SDL_Capture_Stop,
	SDL_Capture_Shutdown
};
#endif

