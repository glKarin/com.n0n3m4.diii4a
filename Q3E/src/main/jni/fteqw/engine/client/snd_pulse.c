#include "quakedef.h"
#ifdef AUDIO_PULSE

#if 0
#include <pulse/simple.h>
#else
typedef struct pa_simple pa_simple;
typedef enum pa_stream_direction {PA_STREAM_PLAYBACK=1} pa_stream_direction_t;
typedef enum pa_sample_format {
    PA_SAMPLE_U8,
    PA_SAMPLE_ALAW,
    PA_SAMPLE_ULAW,
    PA_SAMPLE_S16LE,
    PA_SAMPLE_S16BE,
    PA_SAMPLE_FLOAT32LE,
    PA_SAMPLE_FLOAT32BE,
    PA_SAMPLE_S32LE,
    PA_SAMPLE_S32BE,
    PA_SAMPLE_S24LE,
    PA_SAMPLE_S24BE,
    PA_SAMPLE_S24_32LE,
    PA_SAMPLE_S24_32BE,
    PA_SAMPLE_MAX,
    PA_SAMPLE_INVALID = -1
} pa_sample_format_t;
typedef struct pa_sample_spec {
    pa_sample_format_t format;
    uint32_t rate;
    uint8_t channels;
} pa_sample_spec;
typedef struct pa_channel_map pa_channel_map;
typedef struct pa_buffer_attr pa_buffer_attr;
typedef uint64_t pa_usec_t;

#if __BYTE_ORDER == __BIG_ENDIAN
#define PA_SAMPLE_FLOAT32 PA_SAMPLE_FLOAT32BE
#define PA_SAMPLE_S16NE PA_SAMPLE_S16BE
#else
#define PA_SAMPLE_FLOAT32 PA_SAMPLE_FLOAT32LE
#define PA_SAMPLE_S16NE PA_SAMPLE_S16LE
#endif
#endif


static pa_simple *(*qpa_simple_new)(const char *server,const char *name,pa_stream_direction_t dir, const char *dev, const char *stream_name, const pa_sample_spec *ss, const pa_channel_map *map, const pa_buffer_attr *attr, int *error);
static pa_usec_t (*qpa_simple_get_latency)(pa_simple *s, int *error);
static int (*qpa_simple_write)(pa_simple *s, const void *data, size_t bytes, int *error);
static void (*qpa_simple_free)(pa_simple *s);

static qboolean Pulse_Init(void)
{
	static qboolean tried;
	static void *pulsemodule;
	static dllfunction_t funcs[] =
	{
		{(void**)&qpa_simple_new, "pa_simple_new"},
		{(void**)&qpa_simple_get_latency, "pa_simple_get_latency"},
		{(void**)&qpa_simple_write, "pa_simple_write"},
		{(void**)&qpa_simple_free, "pa_simple_free"},
		{NULL, NULL}
	};
	if (COM_CheckParm("-nopulse"))
		return false;

	if (!tried)
	{
		tried = true;
		pulsemodule = Sys_LoadLibrary("libpulse-simple.so.0", funcs);
	}

	return pulsemodule!=NULL;
}

static unsigned int Pulse_GetDMAPos(soundcardinfo_t *sc)
{
	sc->sn.samplepos = sc->snd_sent / sc->sn.samplebytes;
	return sc->sn.samplepos;
}
static void Pulse_Submit(soundcardinfo_t *sc, int start, int end)
{
}

static void Pulse_Shutdown(soundcardinfo_t *sc)
{
	sc->selfpainting = false;
	if (sc->thread)
		Sys_WaitOnThread(sc->thread);
	sc->thread = NULL;
	*sc->name = '\0';
}

static void *Pulse_Lock(soundcardinfo_t *sc, unsigned int *sampidx)
{
	return sc->sn.buffer;
}

static void Pulse_Unlock(soundcardinfo_t *sc, void *buffer)
{
}

static int Pulse_Thread(void *arg)
{
	char buffer[256];
	soundcardinfo_t *sc = arg;
	void *cond = sc->handle;
	int err = 0;
	int showlatency = 64;

	pa_simple *pulse;
	pa_sample_spec ss;
	ss.rate = sc->sn.speed;
	switch(sc->sn.sampleformat)
	{
	case QSF_INVALID:
	case QSF_EXTERNALMIXER:
	case QSF_S8:	//no signed 8bit formats here
		ss.format = PA_SAMPLE_INVALID;
		break;
	case QSF_U8:
		ss.format = PA_SAMPLE_U8;
		break;
	case QSF_S16:
		ss.format = PA_SAMPLE_S16NE;
		break;
	case QSF_F32:
		ss.format = PA_SAMPLE_FLOAT32;
		break;
	}
	ss.channels = sc->sn.numchannels;

	pulse = qpa_simple_new(	NULL,               // Use the default server.
							FULLENGINENAME,     // Our application's name.
							PA_STREAM_PLAYBACK,
							NULL,               // Use the default device.
							"Game Audio",       // Description of our stream.
							&ss,                // Our sample format.
							NULL,               // Use default channel map
							NULL,               // Use default buffering attributes.
							NULL               // Ignore error code.
							);
	if (pulse)
		sc->selfpainting = true;	//its going!

	Sys_LockConditional(cond);
	Sys_ConditionSignal(cond);
	Sys_UnlockConditional(cond);

	while(sc->selfpainting)
	{
		sc->sn.buffer = buffer;
		sc->sn.samples = sizeof(buffer)/sc->sn.samplebytes;
		sc->samplequeue = sc->sn.samples;
		S_MixerThread(sc);
		sc->snd_sent += sc->sn.samplebytes*sc->samplequeue;

		if (qpa_simple_write(pulse, buffer, sc->sn.samplebytes*sc->samplequeue, &err) < 0)
		{
			Con_Printf("pa_simple_write failed\n");
			sc->selfpainting = false;	//some sort of error
		}

		if (showlatency > 0)
		if (--showlatency == 0)
		{	//we delay this print so that we have a chance of finding out the real value
			pa_usec_t latency = qpa_simple_get_latency(pulse, &err);
			Con_Printf("PulseAudio latency is about %.3f seconds\n", latency/1000000.0);
		}
	}

	if (pulse)
		qpa_simple_free(pulse);
	return 0;
}

static qboolean Pulse_InitCard(soundcardinfo_t *sc, const char *snddev)
{	//FIXME: implement snd_multipledevices somehow.

	if (!Pulse_Init())
		return false;

	sc->inactive_sound = true;	//linux sound devices always play sound, even when we're not the active app...
	sc->sn.samplebytes = 4;
	sc->sn.sampleformat = QSF_F32;
	sc->sn.buffer = NULL;
	sc->sn.samplepos = 0;
	sc->Submit		= Pulse_Submit;
	sc->GetDMAPos	= Pulse_GetDMAPos;
	sc->Lock		= Pulse_Lock;
	sc->Unlock		= Pulse_Unlock;
	sc->Shutdown	= Pulse_Shutdown;

	sc->handle = Sys_CreateConditional();
	Sys_LockConditional(sc->handle);
	sc->thread = Sys_CreateThread("pulse", Pulse_Thread, sc, THREADP_HIGHEST, 0);
	if (sc->thread)
	{
		if (!Sys_ConditionWait(sc->handle))
			sc->selfpainting = false;
		//thread is up and running now.
	}
	Sys_UnlockConditional(sc->handle);
	Sys_DestroyConditional(sc->handle);

	if (!sc->selfpainting)
	{	//err, thread signalled itself to die?
		Pulse_Shutdown(sc);
		return false;
	}
	return true;
}

#define SDRVNAME "Pulse"

static qboolean QDECL Pulse_Enumerate(void (QDECL *cb) (const char *drivername, const char *devicecode, const char *readablename))
{
	if (!Pulse_Init())
		return true;	//sucessfully enumerated no devices
	return false;		//not implemented (we'll get a default device only)
}

sounddriver_t Pulse_Output =
{
	SDRVNAME,
	Pulse_InitCard,
	Pulse_Enumerate
};

#endif
