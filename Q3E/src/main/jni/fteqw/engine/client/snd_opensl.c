#include "quakedef.h"

#include <SLES/OpenSLES.h>

#define AUDIODRIVERNAME "OpenSLES"

static void QDECL OSL_RegisterCvars(void)
{
}

static void *OSL_Lock(soundcardinfo_t *sc, unsigned int *startoffset)
{
	return sc->sn.buffer;
}
static void OSL_Unlock(soundcardinfo_t *sc, void *buffer)
{
	//no need to do anything
}
static void OSL_Submit(soundcardinfo_t *sc, int start, int end)
{
	//submit happens outside the mixer
}
static unsigned int OSL_GetDMAPos(soundcardinfo_t *sc)
{
	sc->sn.samplepos = sc->snd_sent;
	return sc->sn.samplepos;
}

typedef struct 
{
	SLObjectItf sl;
	SLEngineItf engine;
	SLPlayItf play;
	SLObjectItf player;
	SLObjectItf output;
	SLBufferQueueItf bufferqueue;


	unsigned char *buffer;
	size_t buffersegmentsize;
	size_t buffersegments;
	size_t buffersegment;
} osl_data_t;

//assumption: that each buffer is released in sequence.
static void buffercallback(SLBufferQueueItf queue, void *ctx)
{
	soundcardinfo_t *sc = ctx;
	osl_data_t *p = sc->handle;
	//we got the buffer back.
	if (sc->Shutdown)
	{	//we're not shutting down yet, so paint more stuff into the buffer and throw it back.
		sc->sn.buffer = p->buffer + (p->buffersegment%p->buffersegments)*p->buffersegmentsize;
		sc->sn.samples = p->buffersegmentsize/sc->sn.samplebytes;//numFramesAvailable * sc->sn.numchannels;
		sc->samplequeue = sc->sn.samples;
		S_MixerThread(sc);
		sc->snd_sent = p->buffersegment++*p->buffersegmentsize;

		(*queue)->Enqueue(queue, sc->sn.buffer, p->buffersegmentsize);
	}
}

static void OSL_Shutdown(soundcardinfo_t *sc)
{
	osl_data_t *p = sc->handle;
	sc->Shutdown = NULL;	//stop posting new buffers
	if (p)
	{
		if (p->play)
			(*p->play)->SetPlayState(p->play, SL_PLAYSTATE_STOPPED);
		if (p->player)
			(*p->player)->Destroy(p->player);
		if (p->output)
			(*p->output)->Destroy(p->output);
		if (p->sl)
			(*p->sl)->Destroy(p->sl);
		sc->handle = NULL;
		Z_Free(p);
	}
}

static qboolean QDECL OSL_InitCard (soundcardinfo_t *sc, const char *cardname)
{
	osl_data_t *p;
	size_t segments = 4;		//lets cycle through 4 segments in our single logical buffer
	size_t segmentframes = 256;	//with X frames per segment
					//FIXME: mixahead

	sc->sn.numchannels = 2;
	sc->sn.sampleformat = QSF_S16;
	switch(sc->sn.sampleformat)
	{
	case QSF_U8:
	//case QSF_S8;
		sc->sn.samplebytes = 1;
		break;
	case QSF_S16:
		sc->sn.samplebytes = 2;
		break;
	//case QSF_F32:
		//sc->sn.samplebytes = 4;
		//break;
	default:
		return false;
//		sc->sn.sampleformat = QSF_INVALID;
//		sc->sn.samplebytes = 0;
//		break;
	}

	Con_DPrintf("Opening OpenSLES, %.1fkhz\n", sc->sn.speed/1000.0);

	sc->Shutdown	= OSL_Shutdown;
	sc->Lock		= OSL_Lock;
	sc->Unlock		= OSL_Unlock;
	sc->Submit		= OSL_Submit;
	sc->GetDMAPos	= OSL_GetDMAPos;
	sc->handle = p = Z_Malloc(sizeof(*p) + segmentframes*segments*sc->sn.samplebytes*sc->sn.numchannels);
	if (p)
	{
		p->buffer = (unsigned char*)(p+1);
		p->buffersegments = segments;
		p->buffersegmentsize = segmentframes*sc->sn.samplebytes*sc->sn.numchannels;
		sc->selfpainting = true;
		Q_strncpyz(sc->name, cardname?cardname:"", sizeof(sc->name));

		SLEngineOption options[] = 
		{
			{(SLuint32) SL_ENGINEOPTION_THREADSAFE, (SLuint32)SL_BOOLEAN_TRUE},
	//		{(SLuint32) SL_ENGINEOPTION_MAJORVERSION, (SLuint32)1},
	//		{(SLuint32) SL_ENGINEOPTION_MINORVERSION, (SLuint32)1},
		};
		slCreateEngine(&p->sl, countof(options), options, 0, NULL, NULL);
		if (p->sl)
		{
			(*p->sl)->Realize(p->sl, SL_BOOLEAN_FALSE);
			(*p->sl)->GetInterface(p->sl, SL_IID_ENGINE, &p->engine);
			if (p->engine)
			{
				(*p->engine)->CreateOutputMix(p->engine, &p->output, 0, NULL, NULL);
				if (p->output)
				{
					SLboolean TRUE_ = true;
					SLDataLocator_BufferQueue loc_bufferqueue = {SL_DATALOCATOR_BUFFERQUEUE, segments};
					SLDataFormat_PCM pcmformat = {SL_DATAFORMAT_PCM, sc->sn.numchannels, sc->sn.speed*1000, sc->sn.samplebytes*8, sc->sn.samplebytes*8/*+pad*/, SL_SPEAKER_FRONT_LEFT|SL_SPEAKER_FRONT_RIGHT, SL_BYTEORDER_LITTLEENDIAN};
					SLDataSource audiosource = {&loc_bufferqueue, &pcmformat};
					SLDataLocator_OutputMix loc_outputmix = {SL_DATALOCATOR_OUTPUTMIX, p->output};
					SLDataSink audiosink = {&loc_outputmix, NULL};
					(*p->output)->Realize(p->output, false);
					(*p->engine)->CreateAudioPlayer(p->engine, &p->player, &audiosource, &audiosink, 1, &SL_IID_BUFFERQUEUE, &TRUE_);
					if (p->player)
					{
						(*p->player)->Realize(p->player, false);
						(*p->player)->GetInterface(p->player, SL_IID_PLAY, &p->play);
						(*p->player)->GetInterface(p->player, SL_IID_BUFFERQUEUE, &p->bufferqueue);
						if (p->bufferqueue)
						{
							(*p->bufferqueue)->RegisterCallback(p->bufferqueue, buffercallback, sc);
							buffercallback(p->bufferqueue, sc);
							buffercallback(p->bufferqueue, sc);
							buffercallback(p->bufferqueue, sc);
							if (SL_RESULT_SUCCESS == (*p->play)->SetPlayState(p->play, SL_PLAYSTATE_PLAYING))
							{
								//should now be playing our buffers, and recycling them when one terminates.i
								return true;
							}
						}
					}
				}
			}
		}
	}
	OSL_Shutdown(sc);
	return false;
}

static qboolean QDECL OSL_Enumerate (void (QDECL *callback) (const char *drivername, const char *devicecode, const char *readablename))
{
	//callback(AUDIODRIVERNAME, internalname, nicename);
	return false;
}

sounddriver_t OSL_Output =
{
	AUDIODRIVERNAME,
	OSL_InitCard,
	OSL_Enumerate,
	OSL_RegisterCvars
};

