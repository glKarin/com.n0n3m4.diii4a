#include "quakedef.h"

/*
This is based on Jogi's OpenAL support.
Much of it is stripped, to try and get it clean/compliant.

Emscripten/WebAudio is buggy or limited.
This means we force distance models and use hacks to avoid bugs in browsers.
We also have no doppler with WebAudio.
*/

/*Bug list:

	underwater cacaphoy
		openal bug with reverb. either disable reverb or disable openal.

	"build/openal-soft-1.19.1/Alc/filters/filter.c:25: BiquadFilter_setParams: Assertion `gain > 0.00001f' failed." + SIGABRT
		bug started with 1.19.1. Not fte's bug. either disable reverb or disable openal.
		(happens when reverb properties are changed too fast)

	AL_OUT_OF_MEMORY
		shitty openal implementation with too-low limits on number of sources.

	AL_INVALID_VALUE
		shitty (apple) openal implementation with too-low limits on number of sources.

*/


#ifdef AVAIL_OPENAL

#ifdef FTE_TARGET_WEB
#include <emscripten/emscripten.h>
	//emscripten provides an openal -> webaudio wrapper. its not the best, but does get the job done.
	#define OPENAL_STATIC		//our javascript port doesn't support dynamic linking  (bss+data segments get too messy).
	#define SDRVNAME "WebAudio"	//IE doesn't support webaudio, resulting in noticable error messages about no openal, which is technically incorrect. So lets be clear about this.
	#define SDRVNAMEDESC "WebAudio:"

	#define QMIX_SDRVNAME "qmix"	//IE doesn't support webaudio, resulting in noticable error messages about no openal, which is technically incorrect. So lets be clear about this.
	#define QMIX_SDRVNAMEDESC "QMIX:"
#else
	#define SDRVNAME "OpenAL"
	#define SDRVNAMEDESC "OAL:"
	#define QMIX_SDRVNAME "qmix"
	#define QMIX_SDRVNAMEDESC "OALQ:"
	#define USEEFX
#endif
#ifndef HAVE_MIXER
	#undef SDRVNAMEDESC
	#define SDRVNAMEDESC ""	//remove the prefixes in user-visible desciptions when there's (probably) no other devices anyway
#endif

#ifdef OPENAL_STATIC
#include <AL/al.h>	//output
#include <AL/alc.h>	//context+input
#ifdef USEEFX
#include <AL/efx.h>
#endif

#ifndef AL_API
#define AL_API
#endif
#ifndef AL_APIENTRY
#define AL_APIENTRY
#endif

#define palGetError				alGetError
#define palSourcef				alSourcef
#define palSourcei				alSourcei
#define palSourcePlayv			alSourcePlayv
#define palSourceStopv			alSourceStopv
#define palSourcePlay			alSourcePlay
#define palSourceStop			alSourceStop
#define palDopplerFactor		alDopplerFactor
#define palGenBuffers			alGenBuffers
#define palIsBuffer				alIsBuffer
#define palBufferData			alBufferData
#define palBufferiv				alBufferiv
#define palDeleteBuffers		alDeleteBuffers
#define palListenerfv			alListenerfv
#define palSourcefv				alSourcefv
#define palGetString			alGetString
#define palGenSources			alGenSources
#define palIsSource				alIsSource
#define palListenerf			alListenerf
#define palDeleteBuffers		alDeleteBuffers
#define palDeleteSources		alDeleteSources
#define palSpeedOfSound			alSpeedOfSound
#define palDistanceModel		alDistanceModel
#define palGetSourcei			alGetSourcei
#define palSourceQueueBuffers	alSourceQueueBuffers
#define palSourceUnqueueBuffers	alSourceUnqueueBuffers
#define	palIsExtensionPresent	alIsExtensionPresent


#define palcOpenDevice			alcOpenDevice
#define palcCloseDevice			alcCloseDevice
#define palcCreateContext		alcCreateContext
#define palcDestroyContext		alcDestroyContext
#define palcMakeContextCurrent	alcMakeContextCurrent
#define palcProcessContext		alcProcessContext
#define palcGetString			alcGetString
#define palcGetIntegerv			alcGetIntegerv
#define palcIsExtensionPresent	alcIsExtensionPresent
#define palcGetProcAddress		alcGetProcAddress

#define palGetProcAddress alGetProcAddress

//voip stuff
#define palcCaptureOpenDevice	alcCaptureOpenDevice
#define palcCaptureCloseDevice	alcCaptureCloseDevice
#define palcCaptureStart		alcCaptureStart
#define palcCaptureStop			alcCaptureStop
#define palcCaptureSamples		alcCaptureSamples

#ifdef FTE_TARGET_WEB	//emscripten sucks.
AL_API void (AL_APIENTRY alSpeedOfSound)( ALfloat value ) {}
#endif
#else

#if defined(_WIN32)
 #define AL_APIENTRY __cdecl
#else
 #define AL_APIENTRY
#endif
#define AL_API

#undef AL_ALEXT_PROTOTYPES


typedef int ALint;
typedef unsigned int ALuint;
typedef float ALfloat;
typedef int ALenum;
typedef char ALchar;
typedef char ALboolean;
typedef int ALsizei;
typedef void ALvoid;
typedef unsigned char ALubyte;

static dllhandle_t *openallib;
static qboolean openallib_tried;
static AL_API ALenum (AL_APIENTRY *palGetError)( void );
static AL_API void (AL_APIENTRY *palSourcef)( ALuint sid, ALenum param, ALfloat value ); 
static AL_API void (AL_APIENTRY *palSourcei)( ALuint sid, ALenum param, ALint value ); 
static AL_API void (AL_APIENTRY *palSource3i)(ALuint source, ALenum param, ALint value1, ALint value2, ALint value3);

static AL_API void (AL_APIENTRY *palSourcePlayv)( ALsizei ns, const ALuint *sids );
static AL_API void (AL_APIENTRY *palSourceStopv)( ALsizei ns, const ALuint *sids );
static AL_API void (AL_APIENTRY *palSourcePlay)( ALuint sid );
static AL_API void (AL_APIENTRY *palSourceStop)( ALuint sid );

static AL_API void (AL_APIENTRY *palDopplerFactor)( ALfloat value );

static AL_API void (AL_APIENTRY *palGenBuffers)( ALsizei n, ALuint* buffers );
static AL_API ALboolean (AL_APIENTRY *palIsBuffer)( ALuint bid );
static AL_API void (AL_APIENTRY *palBufferData)( ALuint bid, ALenum format, const ALvoid* data, ALsizei size, ALsizei freq );
static AL_API void (AL_APIENTRY *palBufferiv)(ALuint buffer, ALenum param, const ALint *values);
static AL_API void (AL_APIENTRY *palDeleteBuffers)( ALsizei n, const ALuint* buffers );

static AL_API void (AL_APIENTRY *palListenerfv)( ALenum param, const ALfloat* values ); 
static AL_API void (AL_APIENTRY *palSourcefv)( ALuint sid, ALenum param, const ALfloat* values ); 
static AL_API const ALchar* (AL_APIENTRY *palGetString)( ALenum param );
static AL_API void (AL_APIENTRY *palGenSources)( ALsizei n, ALuint* sources ); 
static AL_API ALboolean (AL_APIENTRY *palIsSource)( ALuint sourceName );
static AL_API void (AL_APIENTRY *palListenerf)( ALenum param, ALfloat value );
static AL_API void (AL_APIENTRY *palDeleteBuffers)( ALsizei n, const ALuint* buffers );
static AL_API void (AL_APIENTRY *palDeleteSources)( ALsizei n, const ALuint* sources );
static AL_API void (AL_APIENTRY *palSpeedOfSound)( ALfloat value );
static AL_API void (AL_APIENTRY *palDistanceModel)( ALenum distanceModel );

//needed for streaming
static AL_API void (AL_APIENTRY *palGetSourcei)(ALuint source, ALenum pname, ALint *value);
static AL_API void (AL_APIENTRY *palSourceQueueBuffers)(ALuint source, ALsizei n, ALuint* buffers);
static AL_API void (AL_APIENTRY *palSourceUnqueueBuffers)(ALuint source, ALsizei n, ALuint* buffers);

//for extensions like efx
static AL_API ALboolean (AL_APIENTRY *palIsExtensionPresent)(const ALchar *fextame);
static AL_API void*(AL_APIENTRY *palGetProcAddress)(const ALchar *fname);

#define AL_NONE                                   0
#define AL_FALSE                                  0
#define AL_TRUE                                   1
#define AL_SOURCE_RELATIVE                        0x202
#define AL_PITCH                                  0x1003
#define AL_POSITION                               0x1004
#define AL_VELOCITY                               0x1006
#define AL_LOOPING                                0x1007
#define AL_BUFFER                                 0x1009
#define AL_GAIN                                   0x100A
#define AL_ORIENTATION                            0x100F
#define	AL_SOURCE_STATE                           0x1010
#define	AL_PLAYING                                0x1012	
#define	AL_BUFFERS_QUEUED                         0x1015
#define	AL_BUFFERS_PROCESSED                      0x1016
#define AL_REFERENCE_DISTANCE                     0x1020
#define AL_ROLLOFF_FACTOR                         0x1021
#define AL_MAX_DISTANCE                           0x1023
#define AL_SAMPLE_OFFSET                          0x1025
#define	AL_SOURCE_TYPE                            0x1027
#define	AL_STREAMING                              0x1029
#define AL_FORMAT_MONO8                           0x1100
#define AL_FORMAT_MONO16                          0x1101
#define AL_FORMAT_STEREO8                         0x1102
#define AL_FORMAT_STEREO16                        0x1103
#define AL_INVALID_NAME                           0xA001
#define AL_INVALID_ENUM                           0xA002
#define AL_INVALID_VALUE                          0xA003
#define AL_INVALID_OPERATION                      0xA004
#define AL_OUT_OF_MEMORY                          0xA005
#define AL_VENDOR                                 0xB001
#define AL_VERSION                                0xB002
#define AL_RENDERER                               0xB003
#define AL_EXTENSIONS                             0xB004
#define AL_DISTANCE_MODEL                         0xD000
#define AL_INVERSE_DISTANCE                       0xD001
#define AL_INVERSE_DISTANCE_CLAMPED               0xD002
#define AL_LINEAR_DISTANCE                        0xD003
#define AL_LINEAR_DISTANCE_CLAMPED                0xD004
#define AL_EXPONENT_DISTANCE                      0xD005
#define AL_EXPONENT_DISTANCE_CLAMPED              0xD006




#if defined(_WIN32)
 #define ALC_APIENTRY __cdecl
#else
 #define ALC_APIENTRY
#endif
#define ALC_API

typedef char ALCboolean;
typedef char ALCchar;
typedef int ALCint;
typedef unsigned int ALCuint;
typedef int ALCenum;
typedef size_t ALCsizei;
typedef struct ALCdevice_struct ALCdevice;
typedef struct ALCcontext_struct ALCcontext;
typedef void ALCvoid;

static ALC_API ALCdevice *     (ALC_APIENTRY *palcOpenDevice)( const ALCchar *devicename );
static ALC_API ALCboolean      (ALC_APIENTRY *palcCloseDevice)( ALCdevice *device );

static ALC_API ALCcontext *    (ALC_APIENTRY *palcCreateContext)( ALCdevice *device, const ALCint* attrlist );
static ALC_API void            (ALC_APIENTRY *palcDestroyContext)( ALCcontext *context );
static ALC_API ALCboolean      (ALC_APIENTRY *palcMakeContextCurrent)( ALCcontext *context );
static ALC_API void            (ALC_APIENTRY *palcProcessContext)( ALCcontext *context );

static ALC_API const ALCchar * (ALC_APIENTRY *palcGetString)( ALCdevice *device, ALCenum param );
static ALC_API ALCboolean      (ALC_APIENTRY *palcIsExtensionPresent)( ALCdevice *device, const ALCchar *extname );
static ALC_API void*           (ALC_APIENTRY *palcGetProcAddress)(ALCdevice *device, const ALCchar *funcname);


#define ALC_DEFAULT_DEVICE_SPECIFIER             0x1004
#define ALC_DEVICE_SPECIFIER                     0x1005
#define ALC_EXTENSIONS                           0x1006
#define ALC_ALL_DEVICES_SPECIFIER				 0x1013	//ALC_ENUMERATE_ALL_EXT

//#include "AL/alut.h"
//#include "AL/al.h"
//#include "AL/alext.h"

#if defined(VOICECHAT)
//capture-specific stuff
static void           (ALC_APIENTRY *palcGetIntegerv)( ALCdevice *device, ALCenum param, ALCsizei size, ALCint *data );
static ALCdevice *    (ALC_APIENTRY *palcCaptureOpenDevice)( const ALCchar *devicename, ALCuint frequency, ALCenum format, ALCsizei buffersize );
static ALCboolean     (ALC_APIENTRY *palcCaptureCloseDevice)( ALCdevice *device );
static void           (ALC_APIENTRY *palcCaptureStart)( ALCdevice *device );
static void           (ALC_APIENTRY *palcCaptureStop)( ALCdevice *device );
static void           (ALC_APIENTRY *palcCaptureSamples)( ALCdevice *device, ALCvoid *buffer, ALCsizei samples );
#define ALC_CAPTURE_DEVICE_SPECIFIER             0x310
#define ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER     0x311
#define ALC_CAPTURE_SAMPLES                      0x312
#endif


//efx
#ifdef USEEFX
#define AL_AUXILIARY_SEND_FILTER                 0x20006
#define AL_FILTER_NULL                           0x0000
#define AL_EFFECTSLOT_EFFECT                     0x0001
#define AL_EFFECT_TYPE                           0x8001
#define AL_EFFECT_REVERB                         0x0001
#define AL_EFFECT_EAXREVERB                      0x8000

#define AL_REVERB_DENSITY                        0x0001
#define AL_REVERB_DIFFUSION                      0x0002
#define AL_REVERB_GAIN                           0x0003
#define AL_REVERB_GAINHF                         0x0004
#define AL_REVERB_DECAY_TIME                     0x0005
#define AL_REVERB_DECAY_HFRATIO                  0x0006
#define AL_REVERB_REFLECTIONS_GAIN               0x0007
#define AL_REVERB_REFLECTIONS_DELAY              0x0008
#define AL_REVERB_LATE_REVERB_GAIN               0x0009
#define AL_REVERB_LATE_REVERB_DELAY              0x000A
#define AL_REVERB_AIR_ABSORPTION_GAINHF          0x000B
#define AL_REVERB_ROOM_ROLLOFF_FACTOR            0x000C
#define AL_REVERB_DECAY_HFLIMIT                  0x000D

/* EAX Reverb effect parameters */
#define AL_EAXREVERB_DENSITY                     0x0001
#define AL_EAXREVERB_DIFFUSION                   0x0002
#define AL_EAXREVERB_GAIN                        0x0003
#define AL_EAXREVERB_GAINHF                      0x0004
#define AL_EAXREVERB_GAINLF                      0x0005
#define AL_EAXREVERB_DECAY_TIME                  0x0006
#define AL_EAXREVERB_DECAY_HFRATIO               0x0007
#define AL_EAXREVERB_DECAY_LFRATIO               0x0008
#define AL_EAXREVERB_REFLECTIONS_GAIN            0x0009
#define AL_EAXREVERB_REFLECTIONS_DELAY           0x000A
#define AL_EAXREVERB_REFLECTIONS_PAN             0x000B
#define AL_EAXREVERB_LATE_REVERB_GAIN            0x000C
#define AL_EAXREVERB_LATE_REVERB_DELAY           0x000D
#define AL_EAXREVERB_LATE_REVERB_PAN             0x000E
#define AL_EAXREVERB_ECHO_TIME                   0x000F
#define AL_EAXREVERB_ECHO_DEPTH                  0x0010
#define AL_EAXREVERB_MODULATION_TIME             0x0011
#define AL_EAXREVERB_MODULATION_DEPTH            0x0012
#define AL_EAXREVERB_AIR_ABSORPTION_GAINHF       0x0013
#define AL_EAXREVERB_HFREFERENCE                 0x0014
#define AL_EAXREVERB_LFREFERENCE                 0x0015
#define AL_EAXREVERB_ROOM_ROLLOFF_FACTOR         0x0016
#define AL_EAXREVERB_DECAY_HFLIMIT               0x0017
#endif
#endif

#ifdef USEEFX
	#if defined(AL_ALEXT_PROTOTYPES) && defined(OPENAL_STATIC)
		#define	palAuxiliaryEffectSloti			alAuxiliaryEffectSloti
		#define	palGenAuxiliaryEffectSlots		alGenAuxiliaryEffectSlots
		#define	palDeleteAuxiliaryEffectSlots	alDeleteAuxiliaryEffectSlots
		#define	palDeleteEffects				alDeleteEffects
		#define	palGenEffects					alGenEffects
		#define	palEffecti						alEffecti
//		#define	palEffectiv						alEffectiv
		#define	palEffectf						alEffectf
		#define	palEffectfv						alEffectfv
	#else
		static void (AL_APIENTRY *palAuxiliaryEffectSloti)(ALuint effectslot, ALenum param, ALint iValue);
		static ALvoid (AL_APIENTRY *palGenAuxiliaryEffectSlots)(ALsizei n, ALuint *effectslots);
		static ALvoid (AL_APIENTRY *palDeleteAuxiliaryEffectSlots)(ALsizei n, const ALuint *effectslots);
		static ALvoid (AL_APIENTRY *palDeleteEffects)(ALsizei n, const ALuint *effects);

		static ALvoid (AL_APIENTRY *palGenEffects)(ALsizei n, ALuint *effects);
		static ALvoid (AL_APIENTRY *palEffecti)(ALuint effect, ALenum param, ALint iValue);
//		static ALvoid (AL_APIENTRY *palEffectiv)(ALuint effect, ALenum param, const ALint *piValues);
		static ALvoid (AL_APIENTRY *palEffectf)(ALuint effect, ALenum param, ALfloat flValue);
		static ALvoid (AL_APIENTRY *palEffectfv)(ALuint effect, ALenum param, const ALfloat *pflValues);
	#endif
#endif

//AL_EXT_float32
#define AL_FORMAT_MONO_FLOAT32                      0x10010
#define AL_FORMAT_STEREO_FLOAT32                    0x10011

//AL_SOFT_source_spatialize
#define AL_SOURCE_SPATIALIZE_SOFT					0x1214

//AL_SOFT_loop_points
#define AL_LOOP_POINTS_SOFT							0x2015

//ALC_SOFT_HRTF
#define	ALC_HRTF_SOFT								0x1992
#define		ALC_DONT_CARE_SOFT						0x0002
#define ALC_HRTF_STATUS_SOFT						0x1993
#define		ALC_HRTF_DISABLED_SOFT					0x0000
#define		ALC_HRTF_ENABLED_SOFT					0x0001
#define		ALC_HRTF_DENIED_SOFT					0x0002
#define		ALC_HRTF_REQUIRED_SOFT					0x0003
#define		ALC_HRTF_HEADPHONES_DETECTED_SOFT		0x0004
#define		ALC_HRTF_UNSUPPORTED_FORMAT_SOFT		0x0005
#define	ALC_NUM_HRTF_SPECIFIERS_SOFT				0x1994
#define	ALC_HRTF_SPECIFIER_SOFT						0x1995
#define	ALC_HRTF_ID_SOFT							0x1996
static const ALCchar *(*palcGetStringiSOFT)(ALCdevice *device, ALCenum paramName, ALCsizei index);

#define SOUNDVARS SDRVNAME" variables"


extern sfx_t *known_sfx; //sfxindex = (sfx-known_sfx);

#ifdef USEEFX
static ALuint OpenAL_LoadEffect(const struct reverbproperties_s *reverb);
#endif
static void OpenAL_Shutdown (soundcardinfo_t *sc);
static void QDECL OnChangeALSettings (cvar_t *var, char *value);
/*
static void S_Init_f(void);
static void S_Info(void);

static void S_Shutdown_f(void);
*/
#ifdef FTE_TARGET_WEB
static cvar_t s_al_disable = CVARD("s_al_disable", "1", "0: OpenAL works (generally as the highest priority).\n1: OpenAL will be used only when a specific device is selected.\n2: Don't allow ANY use of OpenAl.\nWith OpenAL disabled, audio ouput will fall back to platform-specific output, avoiding miscilaneous third-party openal limitation bugs.");
#else
static cvar_t s_al_disable = CVARD("s_al_disable", "0", "0: OpenAL works (generally as the highest priority).\n1: OpenAL will be used only when a specific device is selected.\n2: Don't allow ANY use of OpenAl.\nWith OpenAL disabled, audio ouput will fall back to platform-specific output, avoiding miscilaneous third-party openal limitation bugs.");
#endif
static cvar_t s_al_debug = CVARD("s_al_debug", "0", "Enables periodic checks for OpenAL errors.");
static cvar_t s_al_hrtf = CVARD("s_al_hrtf", "", "Enables use of HRTF, and which HRTF table to use.\nempty: auto, depending on openal config to enable it.\n\0: force off.\n1: Use the default HRTF.");
static cvar_t s_al_use_reverb = CVARD("s_al_use_reverb", "1", "Controls whether reverb effects will be used. Set to 0 to block them. Reverb requires gamecode to configure the reverb properties, other than underwater.");
//static cvar_t s_al_max_distance = CVARFC("s_al_max_distance", "1000",0,OnChangeALSettings);
static cvar_t s_al_speedofsound = CVARFCD("s_al_speedofsound", "343.3",0,OnChangeALSettings, "Configures the speed of sound, in game units per second. This affects doppler.");
static cvar_t s_al_dopplerfactor = CVARFCD("s_al_dopplerfactor", "1.0",0,OnChangeALSettings, "Multiplies the strength of doppler effects.");
static cvar_t s_al_distancemodel = CVARFCD("s_al_distancemodel", legacyval("2","0"),0,OnChangeALSettings, "Controls how sounds fade with distance.\n0: Inverse (most realistic)\n1: Inverse Clamped\n2: Linear (Quake-like)\n3: Linear Clamped\n4: Exponential\n5: Exponential Clamped\n6: None");
//static cvar_t s_al_rolloff_factor = CVAR("s_al_rolloff_factor", "1");
static cvar_t s_al_reference_distance = CVARD("s_al_reference_distance", "120", "This is the distance at which the sound is audiable with standard volume in the inverse distance models. Nearer sounds will be louder than the original sample.");
static cvar_t s_al_velocityscale = CVARD("s_al_velocityscale", "1", "Rescales velocity values, before doppler can be calculated.");
static cvar_t s_al_static_listener = CVAR("s_al_static_listener", "0");	//cheat
extern cvar_t snd_doppler;

enum distancemodel_e
{
	DM_INVERSE			= 0,
	DM_INVERSE_CLAMPED	= 1,
	DM_LINEAR			= 2,
	DM_LINEAR_CLAMPED	= 3,
	DM_EXPONENT			= 4,
	DM_EXPONENT_CLAMPED	= 5,
	DM_NONE				= 6
};

typedef struct
{
	struct
	{
		ALuint handle;
		unsigned int queuesize;
		ALuint queue[64];
	} qmix;
	struct
	{
		ALuint handle;
		qbyte allocated;	//there is no guarenteed-unused handle (and I don't want to have to keep spamming alIsSource).
		qbyte queuesize;
		ALuint		 queue_b[3];
		usamplepos_t queue_f[3];
	} *source;
	size_t max_sources;

	struct
	{
		ALuint buffer;
		qbyte allocated;	//again no guarentee.
	} *sounds;
	size_t max_sounds;

	ALCdevice *OpenAL_Device;
	ALCcontext *OpenAL_Context;
	qboolean can_source_spatialise;
	qboolean can_looppoints;

	int ListenEnt;			//listener's entity number, so we don't get weird sound displacements
	ALfloat ListenPos[3];	//their origin.
	ALfloat ListenVel[3];	// Velocity of the listener.
	ALfloat ListenOri[6];	// Orientation of the listener. (first 3 elements are "at", second 3 are "up")
	unsigned int listenerdirty;

#ifdef MIXER_F32
	qboolean canfloataudio;
#endif

	int cureffect;
	ALuint effectslot;			//the global reverb slot
	size_t numeffecttypes;
	struct
	{
		ALuint effect;
		unsigned int modificationcount;	//so we know if reverb state needs to get rebuilt
	} *effecttype;
} oalinfo_t;
static void PrintALError(char *string)
{
	ALenum err;
	char *text = NULL;
	if (!s_al_debug.value)
		return;
	err = palGetError();
	switch(err)
	{
	case 0:
		return;
	case AL_INVALID_NAME:
		text = "invalid name";
		break;
	case AL_INVALID_ENUM:
		text = "invalid enum";
		break;
	case AL_INVALID_VALUE:
		text = "invalid value";
		break;
	case AL_INVALID_OPERATION:
		text = "invalid operation";
		break;
	case AL_OUT_OF_MEMORY:
		text = "out of memory";
		break;
	default:
		text = "unknown";
		break;
	}
	Con_Printf("OpenAL - %s: %x: %s\n",string,err,text);
}

static qboolean OpenAL_LoadCache(oalinfo_t *oali, unsigned int *bufptr, sfxcache_t *sc, float volume, int loopstart)
{
	unsigned int fmt;
	unsigned int size;
	switch(sc->format)
	{
#ifdef FTE_TARGET_WEB
	case QAF_BLOB:
		palGenBuffers(1, bufptr);
		emscriptenfte_al_loadaudiofile(*bufptr, sc->data, sc->length);
		//alIsBuffer will report false until success or failure...
		return true;	//but we do have a 'proper' reference to the buffer.
#endif
	case QAF_S8:
		if (sc->numchannels == 2)
		{
			fmt = AL_FORMAT_STEREO8;
			size = sc->length*2;
		}
		else
		{
			fmt = AL_FORMAT_MONO8;
			size = sc->length*1;
		}
		break;
	case QAF_S16:
		if (sc->numchannels == 2)
		{
			fmt = AL_FORMAT_STEREO16;
			size = sc->length*4;
		}
		else
		{
			fmt = AL_FORMAT_MONO16;
			size = sc->length*2;
		}
		break;
#ifdef MIXER_F32
	case QAF_F32:
		if (!oali->canfloataudio)
			return false;
		if (sc->numchannels == 2)
		{
			fmt = AL_FORMAT_STEREO_FLOAT32;
			size = sc->length*8;
		}
		else
		{
			fmt = AL_FORMAT_MONO_FLOAT32;
			size = sc->length*4;
		}
		break;
#endif
	default:
		return false;
	}
	PrintALError("pre Buffer Data");
	palGenBuffers(1, bufptr);
	/*openal is inconsistant and supports only 8bit unsigned or 16bit signed*/
	if (!sc->data)
	{
		//buffer some silence.
		short *tmp = malloc(size);
		memset(tmp, 0, size);
		palBufferData(*bufptr, fmt, tmp, size, sc->speed);
		free(tmp);
	}
	else if (volume != 1)
	{
		switch(sc->format)
		{
#ifdef FTE_TARGET_WEB
		case QAF_BLOB:
			break;	//unreachable
#endif
		case QAF_S8:
			{
				unsigned char *tmp = malloc(size);
				char *src = sc->data;
				int i;
				for (i = 0; i < size; i++)
					tmp[i] = src[i]*volume+128;	//signed->unsigned
				palBufferData(*bufptr, fmt, tmp, size, sc->speed);
				free(tmp);
			}
			break;
		case QAF_S16:
			{
				short *tmp = malloc(size);
				short *src = (short*)sc->data;
				int i;
				for (i = 0; i < (size>>1); i++)
					tmp[i] = bound(-32767, src[i]*volume, 32767);	//signed.
				palBufferData(*bufptr, fmt, tmp, size, sc->speed);
				free(tmp);
			}
			break;
#ifdef MIXER_F32
		case QAF_F32:
			{
				float *tmp = malloc(size);
				float *src = (float*)sc->data;
				int i;
				for (i = 0; i < (size>>2); i++)
					tmp[i] = src[i]*volume;	//signed. oversaturation isn't my problem
				palBufferData(*bufptr, fmt, tmp, size, sc->speed);
				free(tmp);
			}
			break;
#endif
		}
	}
	else
	{
		switch(sc->format)
		{
#ifdef FTE_TARGET_WEB
		case QAF_BLOB:
			break;	//unreachable
#endif
		case QAF_S8:
			{
				unsigned char *tmp = malloc(size);
				char *src = sc->data;
				int i;
				for (i = 0; i < size; i++)
				{
					tmp[i] = src[i]+128;
				}
				palBufferData(*bufptr, fmt, tmp, size, sc->speed);
				free(tmp);
			}
			break;
		//case QAF_U8:
		case QAF_S16:
		//case QAF_S32:
#ifdef MIXER_F32
		case QAF_F32:
#endif
			palBufferData(*bufptr, fmt, sc->data, size, sc->speed);
			break;
		}
	}

	if (oali->can_looppoints && loopstart>0)
	{
		ALint points[2] = {loopstart, sc->length};
		palBufferiv(*bufptr, AL_LOOP_POINTS_SOFT, points);
	}

	//FIXME: we need to handle oal-oom error codes

	PrintALError("Buffer Data");
	return true;
}

static void QDECL OpenAL_CvarInit(void)
{
	Cvar_Register(&s_al_disable, SOUNDVARS);
	Cvar_Register(&s_al_debug, SOUNDVARS);
	Cvar_Register(&s_al_hrtf, SOUNDVARS);
	Cvar_Register(&s_al_use_reverb, SOUNDVARS);
//	Cvar_Register(&s_al_max_distance, SOUNDVARS);
	Cvar_Register(&s_al_dopplerfactor, SOUNDVARS);
	Cvar_Register(&s_al_distancemodel, SOUNDVARS);
	Cvar_Register(&s_al_reference_distance, SOUNDVARS);
//	Cvar_Register(&s_al_rolloff_factor, SOUNDVARS);
	Cvar_Register(&s_al_velocityscale, SOUNDVARS);
	Cvar_Register(&s_al_static_listener, SOUNDVARS);
	Cvar_Register(&s_al_speedofsound, SOUNDVARS);
}

static void OpenAL_ListenerUpdate(soundcardinfo_t *sc, int entnum, vec3_t origin, vec3_t forward, vec3_t right, vec3_t up, vec3_t velocity)
{
	oalinfo_t *oali = sc->handle;
	vec3_t vel;

	if (snd_doppler.modified)
	{
		snd_doppler.modified = false;
		OnChangeALSettings(NULL,NULL);
	}

	if (!VectorCompare(origin, oali->ListenPos))
	{
		VectorCopy(origin, oali->ListenPos);
		oali->listenerdirty |= 1;
	}

	VectorScale(velocity, s_al_velocityscale.value/35.0, vel);
	if (!VectorCompare(vel, oali->ListenVel))
	{
		VectorCopy(vel, oali->ListenVel);
		oali->listenerdirty |= 2;
	}

	oali->ListenEnt = entnum;
	if (!VectorCompare(forward, oali->ListenOri) || !VectorCompare(up, oali->ListenOri+3))
	{
		oali->ListenOri[0] = forward[0];
		oali->ListenOri[1] = forward[1];
		oali->ListenOri[2] = forward[2];
		oali->ListenOri[3] = up[0];
		oali->ListenOri[4] = up[1];
		oali->ListenOri[5] = up[2];
		oali->listenerdirty |= 4;
	}


	if (!s_al_static_listener.value)
	{
		//I'm using listenerdirty flags because emscripten's openal stuff seems to be wasting massive amounts of time on these.
//		palListenerf(AL_GAIN, 1);
		if (oali->listenerdirty & 1)
			palListenerfv(AL_POSITION, oali->ListenPos);
#ifndef FTE_TARGET_WEB	//webaudio sucks.
		if (oali->listenerdirty & 2)
			palListenerfv(AL_VELOCITY, oali->ListenVel);
#endif
		if (oali->listenerdirty & 4)
			palListenerfv(AL_ORIENTATION, oali->ListenOri);

		oali->listenerdirty = 0;
	}
}

static qboolean OpenAL_ReclaimASource(soundcardinfo_t *sc)
{
	oalinfo_t *oali = sc->handle;
	ALuint src;
	ALuint buf;
	int i;
	int success = 0;
	for (i = 0; i < sc->total_chans; i++)
	{
//		channel_t *chan = &sc->channel[i];
		src = oali->source[i].handle;
		if (oali->source[i].allocated)
		{
			palGetSourcei(src, AL_SOURCE_STATE, &buf);
			if (buf != AL_PLAYING)
			{
				palDeleteSources(1, &src);
				if (oali->source[i].queuesize)
					palDeleteBuffers(oali->source[i].queuesize, oali->source[i].queue_b);
				oali->source[i].queuesize = 0;
				oali->source[i].handle = 0;
				oali->source[i].allocated = false;
				success++;
			}
		}
	}

	if (!success)
	{
		int furthest=-1;
		float dist, bdist=-1;
		vec3_t d;
		for (i = DYNAMIC_FIRST; i < sc->total_chans; i++)
		{
			if (oali->source[i].allocated)
			{
				VectorSubtract(sc->channel[i].origin, oali->ListenPos, d);
				dist = DotProduct(d,d);
				if (dist > bdist)
				{
					bdist = dist;
					furthest = i;
				}
			}
		}
		if (furthest >= 0)
		{
			i = furthest;
			palDeleteSources(1, &oali->source[i].handle);
			if (oali->source[i].queuesize)
				palDeleteBuffers(oali->source[i].queuesize, oali->source[i].queue_b);
			oali->source[i].queuesize = 0;
			oali->source[i].handle = 0;
			oali->source[i].allocated = false;
			success++;
		}
	}

	return success;
}

//for querying sound offsets (for various hacks).
static ssamplepos_t OpenAL_GetChannelPos(soundcardinfo_t *sc, channel_t *chan)
{
	ALint spos = 0;
	oalinfo_t *oali = sc->handle;
	int chnum = chan - sc->channel;
	ALuint src;
	src = oali->source[chnum].handle;
	if (!oali->source[chnum].allocated)
		return (ssamplepos_t)(~(usamplepos_t)0)>>1;	//not actually playing...

	//alcMakeContextCurrent

	if (oali->source[chnum].queuesize)
	{	//we're streaming, for whatever reason.
		ssamplepos_t pos;
		ALuint processed;
		int i;
		//reclaim any queued buffers
		palGetSourcei(src, AL_BUFFERS_PROCESSED, &processed);	//get number of buffers
		palGetSourcei(src, AL_SAMPLE_OFFSET, &spos);	//get our position within the current one.
		if (processed)
		{
			palSourceUnqueueBuffers(src, processed, oali->source[chnum].queue_b);
			palDeleteBuffers(processed, oali->source[chnum].queue_b);
			oali->source[chnum].queuesize -= processed;
			memmove(oali->source[chnum].queue_b, oali->source[chnum].queue_b+processed, oali->source[chnum].queuesize*sizeof(*oali->source[chnum].queue_b));
			memmove(oali->source[chnum].queue_f, oali->source[chnum].queue_f+processed, oali->source[chnum].queuesize*sizeof(*oali->source[chnum].queue_f));
		}

		pos = chan->pos>>PITCHSHIFT; //this is the point of thedata that was already submitted to openal.
		for (i = 0; i < oali->source[chnum].queuesize; i++)
			pos -= oali->source[chnum].queue_f[i];
		//pos is now 'chan->pos at start of current buffer'
		pos += spos; //current playback position (should always be smaller than chan->pos originally was...)
		return pos;
	}
	else
		palGetSourcei(src, AL_SAMPLE_OFFSET, &spos);
	return spos;	//FIXME: result is probably going to be wrong when streaming
}

//schanged says the sample has changed, otherwise its merely moved around a little, maybe changed in volume, but nothing that will restart it.
static void OpenAL_ChannelUpdate(soundcardinfo_t *sc, channel_t *chan, chanupdatereason_t schanged)
{
	oalinfo_t *oali = sc->handle;
	ALuint src;
	sfx_t *sfx = chan->sfx;
	float pitch, cvolume;
	int chnum = chan - sc->channel;
	ALuint buf;
	qboolean stream;
	qboolean srcrel;
	ALuint processed;

	if (chnum >= oali->max_sources)
		Z_ReallocElements((void**)&oali->source, &oali->max_sources, chnum+1+64, sizeof(*oali->source));

	//alcMakeContextCurrent

	if (!oali->source[chnum].allocated)
	{
		//not currently playing. be prepared to create one
		if (!sfx || chan->master_vol == 0)
			return;
		palGetError(); //gah this is so shite
		palGenSources(1, &src);
		if (palGetError() || !palIsSource(src))
		{	//can't just test for invalid, and failure leaving src unchanged could refer to a different sound.
			//try to kill some pther sound
			if (OpenAL_ReclaimASource(sc))
			{	//okay, we killed one. hopefully we can start a new one now.
				palGenSources(1, &src);
				if (palGetError() || !palIsSource(src))
				{
					PrintALError("alGenSources");
					return;
				}
			}
			else return;
		}
		oali->source[chnum].handle = src;
		oali->source[chnum].allocated = true;
		oali->source[chnum].queuesize = 0;
		schanged |= CUR_EVERYTHING;	//should normally be true anyway, but hey
	}
	else
		src = oali->source[chnum].handle;

	PrintALError("pre start sound");

	if (schanged&CUR_SOUNDCHANGE)
	{
		palSourceStop(src);
		palSourcei(src, AL_BUFFER, 0);
		if (oali->source[chnum].queuesize)
			palDeleteBuffers(oali->source[chnum].queuesize, oali->source[chnum].queue_b);
		oali->source[chnum].queuesize = 0;

	}
	else if (oali->source[chnum].queuesize)
	{
		//reclaim any queued buffers
		palGetSourcei(src, AL_BUFFERS_PROCESSED, &processed);
		if (processed)
		{
			palSourceUnqueueBuffers(src, processed, oali->source[chnum].queue_b);
			palDeleteBuffers(processed, oali->source[chnum].queue_b);
			oali->source[chnum].queuesize -= processed;
			memmove(oali->source[chnum].queue_b, oali->source[chnum].queue_b+processed, oali->source[chnum].queuesize*sizeof(*oali->source[chnum].queue_b));
			memmove(oali->source[chnum].queue_f, oali->source[chnum].queue_f+processed, oali->source[chnum].queuesize*sizeof(*oali->source[chnum].queue_f));
		}
	}

	if (!schanged && sfx)	//if we don't figure out when they've finished, they'll not get replaced properly.
	{
		palGetSourcei(src, AL_SOURCE_STATE, &buf);
		if (buf != AL_PLAYING)
		{
			schanged |= CUR_EVERYTHING;
			if(sfx->loopstart != -1)
				chan->pos = sfx->loopstart<<PITCHSHIFT;
			else if (chan->flags & CF_FORCELOOP)
				chan->pos = 0;
			else
				sfx = chan->sfx = NULL;
		}
	}

	/*just wanted to stop it?*/
	if (!sfx)
	{
#ifdef FTE_TARGET_WEB
		//emscripten's webaudio wrapper spams error messages after alDeleteSources has been called, if the context isn't also killed.
		if (!schanged)
			palSourceStop(src);
#else
		palDeleteSources(1, &src);
		if (oali->source[chnum].queuesize)
			palDeleteBuffers(oali->source[chnum].queuesize, oali->source[chnum].queue_b);
		oali->source[chnum].queuesize = 0;
		oali->source[chnum].handle = 0;
		oali->source[chnum].allocated = false;
#endif
		return;
	}

	cvolume = chan->master_vol/255.0f;
	if (!(chan->flags & CF_CL_ABSVOLUME))
		cvolume *= volume.value*voicevolumemod;
	else
		cvolume *= mastervolume.value;

	//openal doesn't support loopstart (entire sample loops or not at all), so if we're meant to skip the first half then we need to stream it.
	//FIXME: AL_SOFT_loop_points
	stream = sfx->decoder.decodedata || (sfx->loopstart > 0 && !oali->can_looppoints);
	srcrel = (chan->flags & CF_NOSPACIALISE) || (chan->entnum && chan->entnum == oali->ListenEnt) || !chan->dist_mult;

	if ((schanged&CUR_SOUNDCHANGE) || stream)
	{
		int sndnum = sfx-known_sfx;
		int buf;
		if (sndnum >= oali->max_sounds)
			Z_ReallocElements((void**)&oali->sounds, &oali->max_sounds, sndnum+1+64, sizeof(*oali->sounds));
		buf = oali->sounds[sndnum].buffer;
		if (!oali->sounds[sndnum].allocated || stream)
		{
			if (!S_LoadSound(sfx, false))
				return;	//can't load it
			if (sfx->loadstate != SLS_LOADED)
			{
				if (sfx->loadstate == SLS_LOADING)
				{	//kill the source so that it gets regenerated again soonish
					palDeleteSources(1, &src);
					oali->source[chnum].handle = 0;
					oali->source[chnum].allocated = false;
				}
				return;	//not available yet
			}

			if (stream)
			{
				int offset;
				sfxcache_t sbuf, *sc;
				while (oali->source[chnum].queuesize < countof(oali->source[chnum].queue_b))
				{	//decode periodically instead of all at the start.
					int tryduration = snd_speed*0.5;
					ssamplepos_t pos = chan->pos>>PITCHSHIFT;

					if (sfx->decoder.decodedata)
						sc = sfx->decoder.decodedata(sfx, &sbuf, pos, tryduration);
					else
					{
						sc = sfx->decoder.buf;
						if (pos >= sc->length)
							sc = NULL;
					}
					if (sc)
					{
						memcpy(&sbuf, sc, sizeof(sbuf));

						//hack up the sound to offset it correctly
						if (pos < sbuf.soundoffset || pos > sbuf.soundoffset+sbuf.length)
							sbuf.length = 0;	//didn't contain the requested samples... the decoder is struggling.
						else
						{
							offset = pos - sbuf.soundoffset;
							sbuf.data += offset * QAF_BYTES(sc->format)*sc->numchannels;
							sbuf.length -= offset;
						}
						sbuf.soundoffset = 0;

						if (sbuf.length > tryduration)
							sbuf.length = tryduration;	//don't bother queuing more than 3*0.5 secs

						if (sbuf.length)
						{
							//build a buffer with it and queue it up.
							//buffer will be purged later on when its unqueued
							if (OpenAL_LoadCache(oali, &buf, &sbuf, max(1,cvolume), 0))
							{
								palSourceQueueBuffers(src, 1, &buf);
								oali->source[chnum].queue_b[oali->source[chnum].queuesize] = buf;
								oali->source[chnum].queue_f[oali->source[chnum].queuesize] = sbuf.length;
								oali->source[chnum].queuesize++;
							}
						}
						else
						{	//decoder isn't ready yet, but didn't signal an error/eof. queue a little silence, because that's better than constant micro stutters
							sfxcache_t silence;
							silence.speed = snd_speed;
							silence.format = QAF_S16;
							silence.numchannels = 1;
							silence.data = NULL;
							silence.length = 0.1 * silence.speed;
							silence.soundoffset = 0;
							if (OpenAL_LoadCache(oali, &buf, &silence, 1, 0))
							{
								palSourceQueueBuffers(src, 1, &buf);
								oali->source[chnum].queue_b[oali->source[chnum].queuesize] = buf;
								oali->source[chnum].queue_f[oali->source[chnum].queuesize] = 0;	//don't count silence.
								oali->source[chnum].queuesize++;
							}
						}

						//yay
						chan->pos += sbuf.length<<PITCHSHIFT;

						palGetSourcei(src, AL_SOURCE_STATE, &buf);
						if (buf != AL_PLAYING)
							schanged = CUR_EVERYTHING;
					}
					else
					{
						if(sfx->loopstart != -1)
							chan->pos = sfx->loopstart<<PITCHSHIFT;
						else if (chan->flags & CF_FORCELOOP)
							chan->pos = 0;
						else //we don't want to play anything more.
							break;
						if (!oali->source[chnum].queuesize)
						{	//queue 0.1 secs if we're starting/resetting a new stream this is to try to cover up discontinuities caused by packetloss or whatever
							sfxcache_t silence;
							silence.speed = snd_speed;
							silence.format = QAF_S16;
							silence.numchannels = 1;
							silence.data = NULL;
							silence.length = 0.1 * silence.speed;
							silence.soundoffset = 0;
							if (OpenAL_LoadCache(oali, &buf, &silence, 1, 0))
							{
								palSourceQueueBuffers(src, 1, &buf);
								oali->source[chnum].queue_b[oali->source[chnum].queuesize] = buf;
								oali->source[chnum].queue_f[oali->source[chnum].queuesize] = 0;	//don't count silence.
								oali->source[chnum].queuesize++;
								if (oali->can_source_spatialise)	//force spacialisation as desired, if supported (this solves browsers forcing stereo on mono files which should mean static audio is full volume...)
									palSourcei(src, AL_SOURCE_SPATIALIZE_SOFT, !srcrel);
							}
						}
					}
				}
				if (!oali->source[chnum].queuesize)
				{
					palGetSourcei(src, AL_SOURCE_STATE, &buf);
					if (buf != AL_PLAYING)
					{
						chan->sfx = NULL;
						if (sfx->decoder.ended)
						{
							if (!S_IsPlayingSomewhere(sfx))
								sfx->decoder.ended(sfx);
						}
						return;
					}
				}
			}
			else
			{	//unstreamed
				if (!sfx->decoder.buf)
					return;
				oali->sounds[sndnum].allocated = OpenAL_LoadCache(oali, &buf, sfx->decoder.buf, 1, sfx->loopstart);
				if (!oali->sounds[sndnum].allocated)
					return;
				oali->sounds[sndnum].buffer = buf;
			}
		}
		if (!stream)
		{
#ifdef FTE_TARGET_WEB
			//loading an ogg is async, so we must wait until its valid.
			//our javascript will hack the buffer so that its not valid until the browser has decoded it for us.
			if (!palIsBuffer(buf))
			{	//same as the SLS_LOADING case above
				palDeleteSources(1, &src);
				oali->source[chnum].handle = 0;
				oali->source[chnum].allocated = false;
				return;
			}
#endif
			palSourcei(src, AL_BUFFER, buf);
			if (oali->can_source_spatialise)	//force spacialisation as desired, if supported (this solves browsers forcing stereo on mono files which should mean static audio is full volume...)
				palSourcei(src, AL_SOURCE_SPATIALIZE_SOFT, !srcrel);
		}
	}
	palSourcef(src, AL_GAIN, min(cvolume, 1));	//openal only supports a max volume of 1. anything above is an error and will be clamped.
	if (srcrel)
	{
		palSourcefv(src, AL_POSITION, vec3_origin);
#ifndef FTE_TARGET_WEB	//webaudio sucks.
		palSourcefv(src, AL_VELOCITY, vec3_origin);
#endif
	}
	else
	{
		palSourcefv(src, AL_POSITION, chan->origin);
#ifndef FTE_TARGET_WEB	//webaudio sucks.
		palSourcefv(src, AL_VELOCITY, chan->velocity);
#endif
	}

	if (schanged)
	{
		if (schanged & CUR_OFFSET && chan->pos)
		{	//complex update, but not restart. pos contains an offset, rather than an absolute time.
			palSourcei(src, AL_SAMPLE_OFFSET, (chan->pos>>PITCHSHIFT));
		}

		pitch = (float)chan->rate/(1<<PITCHSHIFT);
		pitch = max(0.01, pitch); // OpenAL will clamp inside the implementation if need be, only min is important
		palSourcef(src, AL_PITCH, pitch);

#ifdef USEEFX
		if (chan->flags & CF_NOREVERB)	//don't do the underwater thing on static sounds. it sounds like arse with all those sources.
			palSource3i(src, AL_AUXILIARY_SEND_FILTER, 0, 0, AL_FILTER_NULL);
		else
			palSource3i(src, AL_AUXILIARY_SEND_FILTER, oali->effectslot, 0, AL_FILTER_NULL);
#endif

		palSourcei(src, AL_LOOPING, (!stream && ((chan->flags & CF_FORCELOOP)||(sfx->loopstart>=0&&!stream)))?AL_TRUE:AL_FALSE);
		if (srcrel)
		{
			palSourcei(src, AL_SOURCE_RELATIVE, AL_TRUE);
//			palSourcef(src, AL_ROLLOFF_FACTOR, 0.0f);
		}
		else
		{
			palSourcei(src, AL_SOURCE_RELATIVE, AL_FALSE);
//			palSourcef(src, AL_ROLLOFF_FACTOR, s_al_rolloff_factor.value*chan->dist_mult);
		}

		//this is disgustingly shit.
		//logically we want to set the distance divisor to 1 and the rolloff factor to dist_mult.
		//but openal clamps in an annoying order (probably to keep things signed in hardware) and webaudio refuses infinity, so we need to special case no attenuation to get around the issue
		if (srcrel)
		{
#if 0//def FTE_TARGET_WEB
			switch(DM_INVERSE)	//emscripten omits it, and this is webaudio's default too.
#else
			switch((enum distancemodel_e)s_al_distancemodel.ival)
#endif
			{
			default:
			case DM_INVERSE:
			case DM_INVERSE_CLAMPED:
				palSourcef(src, AL_ROLLOFF_FACTOR, 0);
				palSourcef(src, AL_REFERENCE_DISTANCE, 1);	//0 would be silent, or a division by 0
				palSourcef(src, AL_MAX_DISTANCE, 1);	//only used for clamped mode
				break;
			case DM_LINEAR:
			case DM_LINEAR_CLAMPED:
				palSourcef(src, AL_ROLLOFF_FACTOR, 0);
				palSourcef(src, AL_REFERENCE_DISTANCE, 0);	//doesn't matter when rolloff is 0
				palSourcef(src, AL_MAX_DISTANCE, 1);	//doesn't matter, so long as its not a nan
				break;
			}
		}
		else
		{
#if 0//def FTE_TARGET_WEB
			switch(DM_LINEAR)	//emscripten hardcodes it in a buggy kind of way.
#else
			switch((enum distancemodel_e)s_al_distancemodel.ival)
#endif
			{
			default:
			case DM_INVERSE:
			case DM_INVERSE_CLAMPED:
				palSourcef(src, AL_ROLLOFF_FACTOR, s_al_reference_distance.value);
				palSourcef(src, AL_REFERENCE_DISTANCE, 1);
				palSourcef(src, AL_MAX_DISTANCE, 1/chan->dist_mult);	//clamp to the maximum distance you'd normally be allowed to hear... this is probably going to be annoying.
				break;
			case DM_LINEAR:	//linear, mimic quake.
			case DM_LINEAR_CLAMPED: //linear clamped to further than ref distance
				palSourcef(src, AL_ROLLOFF_FACTOR, 1);
#if 0//def FTE_TARGET_WEB
				//chrome complains about 0.
				//with the expontential model, 0 results in division by zero, but we're not using that model and the maths for the linear model is fine with it.
				//the web audio spec says 'The default value is 1. A RangeError exception must be thrown if this is set to a non-negative value.'
				//which of course means that the PannerNode's constructor must throw an exception, which kinda prevents you ever creating one.
				//it also says elsewhere 'If dref = 0, the value of the [exponential|inverse] model is taken to be 0, ...'
				//which shows that the spec should read 'negative values' for rangeerrors (rather than non-positive). so chrome is being shit.
				//unfortutely due to the nature of javascript and exceptions, this is fucking everything else up. thanks chrome!
				palSourcef(src, AL_REFERENCE_DISTANCE, 0.01);
#else
				palSourcef(src, AL_REFERENCE_DISTANCE, 0);
#endif
				palSourcef(src, AL_MAX_DISTANCE, 1/chan->dist_mult);
				break;
			}
		}

	/*and start it up again*/
		if (schanged != CUR_UPDATE)
			palSourcePlay(src);
	}

	PrintALError(sfx?sfx->name:"post start sound");
}

/*
static void S_Info (void)
{
	if (OpenAL_Device == NULL)
		return;

	Con_Printf("OpenAL Version        : %s\n",palGetString(AL_VERSION));
	Con_Printf("OpenAL Vendor         : %s\n",palGetString(AL_VENDOR));
	Con_Printf("OpenAL Renderer       : %s\n",palGetString(AL_RENDERER));
	if(palcIsExtensionPresent(NULL, (const ALCchar*)"ALC_ENUMERATION_EXT")==AL_TRUE)
	{
		Con_Printf("OpenAL Device         : %s\n",palcGetString(OpenAL_Device,ALC_DEVICE_SPECIFIER));
	}
	Con_Printf("OpenAL Default Device : %s\n",palcGetString(OpenAL_Device,ALC_DEFAULT_DEVICE_SPECIFIER));
	Con_Printf("OpenAL AL Extension   : %s\n",palGetString(AL_EXTENSIONS));
	Con_Printf("OpenAL ALC Extension  : %s\n",palcGetString(NULL,ALC_EXTENSIONS));
}
*/

static qboolean OpenAL_InitLibrary(void)
{
#ifdef OPENAL_STATIC
	if (s_al_disable.ival > 1)
		return false;
	return true;
#else
	static dllfunction_t openalfuncs[] =
	{
		{(void*)&palGetError, "alGetError"},
		{(void*)&palSourcef, "alSourcef"},
		{(void*)&palSourcei, "alSourcei"},
		{(void*)&palSource3i, "alSource3i"},
		{(void*)&palSourcePlayv, "alSourcePlayv"},
		{(void*)&palSourceStopv, "alSourceStopv"},
		{(void*)&palSourcePlay, "alSourcePlay"},
		{(void*)&palSourceStop, "alSourceStop"},
		{(void*)&palDopplerFactor, "alDopplerFactor"},
		{(void*)&palGenBuffers, "alGenBuffers"},
		{(void*)&palIsBuffer, "alIsBuffer"},
		{(void*)&palBufferData, "alBufferData"},
		{(void*)&palBufferiv, "alBufferiv"},
		{(void*)&palDeleteBuffers, "alDeleteBuffers"},
		{(void*)&palListenerfv, "alListenerfv"},
		{(void*)&palSourcefv, "alSourcefv"},
		{(void*)&palGetString, "alGetString"},
		{(void*)&palGenSources, "alGenSources"},
		{(void*)&palIsSource, "alIsSource"},
		{(void*)&palListenerf, "alListenerf"},
		{(void*)&palDeleteSources, "alDeleteSources"},
		{(void*)&palSpeedOfSound, "alSpeedOfSound"},
		{(void*)&palDistanceModel, "alDistanceModel"},

		{(void*)&palIsExtensionPresent, "alIsExtensionPresent"},
		{(void*)&palGetProcAddress, "alGetProcAddress"},
		{(void*)&palGetSourcei, "alGetSourcei"},
		{(void*)&palSourceQueueBuffers, "alSourceQueueBuffers"},
		{(void*)&palSourceUnqueueBuffers, "alSourceUnqueueBuffers"},

		{(void*)&palcOpenDevice, "alcOpenDevice"},
		{(void*)&palcCloseDevice, "alcCloseDevice"},
		{(void*)&palcCreateContext, "alcCreateContext"},
		{(void*)&palcDestroyContext, "alcDestroyContext"},
		{(void*)&palcMakeContextCurrent, "alcMakeContextCurrent"},
		{(void*)&palcProcessContext, "alcProcessContext"},
		{(void*)&palcGetString, "alcGetString"},
		{(void*)&palcGetIntegerv, "alcGetIntegerv"},
		{(void*)&palcIsExtensionPresent, "alcIsExtensionPresent"},
		{(void*)&palcGetProcAddress, "alcGetProcAddress"},
		{NULL}
	};

	if (s_al_disable.ival > 1)
		return false;
	if (COM_CheckParm("-noopenal"))
		return false;

	if (!openallib_tried)
	{
		openallib_tried = true;
#ifdef _WIN32
		openallib = Sys_LoadLibrary("OpenAL32", openalfuncs);
		if (!openallib)
			openallib = Sys_LoadLibrary("soft_oal", openalfuncs);
#elif defined(__ANDROID__) //karin: use OpenAL
		openallib = Sys_LoadLibrary("libopenal.so", openalfuncs);
#else
		openallib = Sys_LoadLibrary("libopenal.so.1", openalfuncs);
		if (!openallib)
			openallib = Sys_LoadLibrary("libopenal", openalfuncs);
#endif
	}
	return !!openallib;
#endif
}

static qboolean OpenAL_Init(soundcardinfo_t *sc, const char *devname, qboolean qmix)
{
	oalinfo_t *oali;

	if (!OpenAL_InitLibrary())
	{
		if (!s_al_disable.ival)
		{
			if (devname)
				Con_Printf(SDRVNAME" library is not installed\n");
			else
				Con_DPrintf(SDRVNAME" library is not installed\n");
		}
		return false;
	}

	if (!devname || !*devname)
	{
		if (s_al_disable.ival && !qmix)
			return false;	//no default device
		devname = palcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);
	}
	Q_snprintfz(sc->name, sizeof(sc->name), "%s", devname);
	if (qmix)
		Con_TPrintf("Initiating "QMIX_SDRVNAME": %s.\n", devname);
	else
		Con_TPrintf("Initiating "SDRVNAME": %s.\n", devname);

	oali = Z_Malloc(sizeof(oalinfo_t));
	sc->handle = oali;

	oali->OpenAL_Device = palcOpenDevice(devname);
	if (oali->OpenAL_Device == NULL)
		PrintALError("Could not init a sound device\n");
	else
	{
		size_t i = 0;
		ALCint attrs[5];

		palcGetStringiSOFT = (!qmix&&palcIsExtensionPresent(oali->OpenAL_Device, "ALC_SOFT_HRTF"))?palcGetProcAddress(oali->OpenAL_Device, "alcGetStringiSOFT"):NULL;
		if (palcGetStringiSOFT)
		{
			if (!*s_al_hrtf.string)
			{
				attrs[i++] = ALC_HRTF_SOFT;
				attrs[i++] = ALC_DONT_CARE_SOFT;
			}
			else if (!strcmp(s_al_hrtf.string, "0") || !strcmp(s_al_hrtf.string, "1"))
			{	//explicitly switch it off or on(default)
				attrs[i++] = ALC_HRTF_SOFT;
				attrs[i++] = !strcmp(s_al_hrtf.string, "1");
			}
			else
			{	//we want an explicit hrtf
				ALCint hrtf_count = 0;
				ALCint idx;
				const ALCchar *hrtfname;
				attrs[i++] = ALC_HRTF_SOFT;
				attrs[i++] = true;

				palcGetIntegerv(oali->OpenAL_Device, ALC_NUM_HRTF_SPECIFIERS_SOFT, 1, &hrtf_count);
				for (idx = 0; idx < hrtf_count; idx++)
				{
					hrtfname = palcGetStringiSOFT(oali->OpenAL_Device, ALC_HRTF_SPECIFIER_SOFT, idx);
					if (hrtfname && !strcmp(hrtfname, s_al_hrtf.string))
						break;
				}

				if (idx < hrtf_count)
				{
					attrs[i++] = ALC_HRTF_ID_SOFT;
					attrs[i++] = idx;
				}
				else if (hrtf_count)
				{
					Con_Printf("HRTF \"%s\" not known, available options are:\n", s_al_hrtf.string);
					for (idx = 0; idx < hrtf_count; idx++)
					{
						hrtfname = palcGetStringiSOFT(oali->OpenAL_Device, ALC_HRTF_SPECIFIER_SOFT, idx);
						if (hrtfname)
							Con_Printf("\t\"%s\"\n", hrtfname);
					}
				}
			}
		}
		attrs[i] = 0;	//EOL

		oali->OpenAL_Context = palcCreateContext(oali->OpenAL_Device, attrs);
		if (!oali->OpenAL_Context)
			PrintALError("Could not init a sound context\n");
		else
		{
			palcMakeContextCurrent(oali->OpenAL_Context);
		//	palcProcessContext(oali->OpenAL_Context);

			//S_Info();

			//fixme...
			memset(oali->source, 0, sizeof(*oali->source)*oali->max_sources);
			PrintALError("alGensources for normal sources");

			palListenerf(AL_GAIN, 1);
			palListenerfv(AL_POSITION, oali->ListenPos);
#ifndef FTE_TARGET_WEB	//webaudio sucks.
			palListenerfv(AL_VELOCITY, oali->ListenVel);
#endif
			palListenerfv(AL_ORIENTATION, oali->ListenOri);

			oali->can_source_spatialise = palIsExtensionPresent("AL_SOFT_source_spatialize");
			oali->can_looppoints = palIsExtensionPresent("AL_SOFT_loop_points");

			if (palcGetStringiSOFT)
			{
				ALCint stat;
				palcGetIntegerv(oali->OpenAL_Device, ALC_HRTF_STATUS_SOFT, 1, &stat);
				safeswitch(stat)
				{
				case ALC_HRTF_DISABLED_SOFT:			Con_Printf("AL_HRTF_STATUS: DISABLED.\n"); break;
				case ALC_HRTF_ENABLED_SOFT:				Con_Printf("AL_HRTF_STATUS: ENABLED.\n"); break;
				case ALC_HRTF_DENIED_SOFT:				Con_Printf("AL_HRTF_STATUS: DENIED.\n"); break;
				case ALC_HRTF_REQUIRED_SOFT:			Con_Printf("AL_HRTF_STATUS: REQUIRED.\n"); break;
				case ALC_HRTF_HEADPHONES_DETECTED_SOFT:	Con_Printf("AL_HRTF_STATUS: HEADPHONES_DETECTED.\n"); break;
				case ALC_HRTF_UNSUPPORTED_FORMAT_SOFT:	Con_Printf("AL_HRTF_STATUS: UNSUPPORTED_FORMAT.\n"); break;
				safedefault:							Con_Printf("AL_HRTF_STATUS: %#x.\n", stat); break;
				}
			}
			return true;
		}
		palcCloseDevice(oali->OpenAL_Device);
	}
	Z_Free(oali);
	return false;
}

//called when some al-specific cvar has changed that is linked to openal state.
static void QDECL OnChangeALSettings (cvar_t *var, char *value)
{
	soundcardinfo_t *sc;
	for (sc = sndcardinfo; sc; sc = sc->next)
	{
		//we only want openal devices.
		if (sc->Shutdown != OpenAL_Shutdown)
			continue;
		//alcMakeContextCurrent

		if (palSpeedOfSound)
			palSpeedOfSound(s_al_speedofsound.value);

		if (palDopplerFactor)
			palDopplerFactor(s_al_dopplerfactor.value * snd_doppler.value);

		if (palDistanceModel)
		{
			switch ((enum distancemodel_e)s_al_distancemodel.ival)
			{
				case DM_INVERSE:
					//gain = AL_REFERENCE_DISTANCE / (AL_REFERENCE_DISTANCE +  AL_ROLLOFF_FACTOR * (distance - AL_REFERENCE_DISTANCE) )
					palDistanceModel(AL_INVERSE_DISTANCE);
					break;
				case DM_INVERSE_CLAMPED:	//openal's default mode
					//istance = max(distance,AL_REFERENCE_DISTANCE); 
					//distance = min(distance,AL_MAX_DISTANCE); 
					//gain = AL_REFERENCE_DISTANCE / (AL_REFERENCE_DISTANCE +  AL_ROLLOFF_FACTOR * (distance - AL_REFERENCE_DISTANCE) )
					palDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
					break;
				case DM_LINEAR:	//most quake-like. linear
					//distance = min(distance, AL_MAX_DISTANCE) // avoid negative gain 
					//gain = ( 1 - AL_ROLLOFF_FACTOR * (distance - AL_REFERENCE_DISTANCE) / (AL_MAX_DISTANCE - AL_REFERENCE_DISTANCE) )
					palDistanceModel(AL_LINEAR_DISTANCE);
					break;
				case DM_LINEAR_CLAMPED: //linear, with near stuff clamped to further away
					//distance = max(distance, AL_REFERENCE_DISTANCE) 
					//distance = min(distance, AL_MAX_DISTANCE) 
					//gain = ( 1 - AL_ROLLOFF_FACTOR * (distance - AL_REFERENCE_DISTANCE) / (AL_MAX_DISTANCE - AL_REFERENCE_DISTANCE) )
					palDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);
					break;
				case DM_EXPONENT:
					//gain = (distance / AL_REFERENCE_DISTANCE) ^ (- AL_ROLLOFF_FACTOR)
					palDistanceModel(AL_EXPONENT_DISTANCE);
					break;
				case DM_EXPONENT_CLAMPED:
					//distance = max(distance, AL_REFERENCE_DISTANCE) 
					//distance = min(distance, AL_MAX_DISTANCE) 
					//gain = (distance / AL_REFERENCE_DISTANCE) ^ (- AL_ROLLOFF_FACTOR)
					palDistanceModel(AL_EXPONENT_DISTANCE_CLAMPED);
					break;
				case DM_NONE:
					//gain = 1
					palDistanceModel(AL_NONE);
					break;
				default:
					Cvar_ForceSet(&s_al_distancemodel, "2");
			}
		}
	}
}

/*stub should not be called*/
static void *OpenAL_LockBuffer (soundcardinfo_t *sc, unsigned int *sampidx)
{
	//Con_Printf("OpenAL: LockBuffer\n");
	return NULL;
}

/*stub should not be called*/
static void OpenAL_UnlockBuffer (soundcardinfo_t *sc, void *buffer)
{
	//Con_Printf("OpenAL: UnlockBuffer\n");
}

/*stub should not be called*/
static void OpenAL_Submit (soundcardinfo_t *sc, int start, int end)
{
	//Con_Printf("OpenAL: Submit\n");
}

/*stub should not be called*/
static unsigned int OpenAL_GetDMAPos (soundcardinfo_t *sc)
{
	//Con_Printf("OpenAL: GetDMAPos\n");
	return 0;
}

#ifdef USEEFX
static void OpenAL_SetReverb (soundcardinfo_t *sc, size_t reverbeffect)
{
#ifdef USEEFX
	oalinfo_t *oali = sc->handle;

	if (!oali->effectslot)
		return;

	if (reverbeffect >= numreverbproperties)
		return;	//err... you're doing it wrong.

	//alcMakeContextCurrent

	if (reverbeffect >= oali->numeffecttypes)
	{
		void *n = BZ_Realloc(oali->effecttype, sizeof(*oali->effecttype)*numreverbproperties);
		if (!n)
			return;	//erk?
		oali->effecttype = n;
		memset(oali->effecttype + oali->numeffecttypes, 0, sizeof(*oali->effecttype)*(numreverbproperties-oali->numeffecttypes));
		oali->numeffecttypes = numreverbproperties;
	}
	if (oali->effecttype[reverbeffect].modificationcount != reverbproperties[reverbeffect].modificationcount)
	{	//the desired effect was modified
		oali->cureffect = ~0;
		oali->effecttype[reverbeffect].modificationcount = reverbproperties[reverbeffect].modificationcount;

		palDeleteEffects(1, &oali->effecttype[reverbeffect].effect);
		oali->effecttype[reverbeffect].effect = OpenAL_LoadEffect(&reverbproperties[reverbeffect].props);
	}
	else
	{
		//don't spam it
		if (oali->cureffect == reverbeffect)
			return;
	}
	oali->cureffect = reverbeffect;
	PrintALError("preunderwater");
	palAuxiliaryEffectSloti(oali->effectslot, AL_EFFECTSLOT_EFFECT, oali->effecttype[oali->cureffect].effect);
	PrintALError("postunderwater");
	//Con_Printf("OpenAL: SetUnderWater %i\n", underwater);
#endif
}
#endif

static void OpenAL_Shutdown (soundcardinfo_t *sc)
{
	oalinfo_t *oali = sc->handle;
	int i;

	//alcMakeContextCurrent

	for (i=0;i<oali->max_sources;i++)
	{
		if (oali->source[i].allocated)
		{
			palDeleteSources(1, &oali->source[i].handle);
			oali->source[i].handle = 0;
			oali->source[i].allocated = false;
		}
	}

	/*make sure the buffers are cleared from the sound effects*/
	for (i=0;i<oali->max_sounds;i++)
	{
		if (oali->sounds[i].allocated)
		{
			palDeleteBuffers(1,&oali->sounds[i].buffer);
			oali->sounds[i].allocated = false;
		}
	}

#ifdef USEEFX
	if (palDeleteAuxiliaryEffectSlots)
	{
		palDeleteAuxiliaryEffectSlots(1, &oali->effectslot);
		for (i = 0; i < oali->numeffecttypes; i++)
		{
			if (oali->effecttype[i].effect)
				palDeleteEffects(1, &oali->effecttype[i].effect);
		}
	}
	Z_Free(oali->effecttype);
#endif

	palcMakeContextCurrent(NULL);
	palcDestroyContext(oali->OpenAL_Context);
	palcCloseDevice(oali->OpenAL_Device);
	Z_Free(oali->sounds);
	Z_Free(oali->source);
	Z_Free(oali);
}

#ifdef USEEFX
static ALuint OpenAL_LoadEffect(const struct reverbproperties_s *reverb)
{
	ALuint effect = 0;
#ifdef AL_EFFECT_EAXREVERB
	palGetError();
	palGenEffects(1, &effect);

	//try eax reverb for more settings
	palEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);
	if (!palGetError())
	{
		/* EAX Reverb is available. Set the EAX effect type then load the
		 * reverb properties. */
		palEffectf(effect, AL_EAXREVERB_DENSITY, reverb->flDensity);
		palEffectf(effect, AL_EAXREVERB_DIFFUSION, reverb->flDiffusion);
		palEffectf(effect, AL_EAXREVERB_GAIN, reverb->flGain);
		palEffectf(effect, AL_EAXREVERB_GAINHF, reverb->flGainHF);
		palEffectf(effect, AL_EAXREVERB_GAINLF, reverb->flGainLF);
		palEffectf(effect, AL_EAXREVERB_DECAY_TIME, reverb->flDecayTime);
		palEffectf(effect, AL_EAXREVERB_DECAY_HFRATIO, reverb->flDecayHFRatio);
		palEffectf(effect, AL_EAXREVERB_DECAY_LFRATIO, reverb->flDecayLFRatio);
		palEffectf(effect, AL_EAXREVERB_REFLECTIONS_GAIN, reverb->flReflectionsGain);
		palEffectf(effect, AL_EAXREVERB_REFLECTIONS_DELAY, reverb->flReflectionsDelay);
		palEffectfv(effect, AL_EAXREVERB_REFLECTIONS_PAN, reverb->flReflectionsPan);
		palEffectf(effect, AL_EAXREVERB_LATE_REVERB_GAIN, reverb->flLateReverbGain);
		palEffectf(effect, AL_EAXREVERB_LATE_REVERB_DELAY, reverb->flLateReverbDelay);
		palEffectfv(effect, AL_EAXREVERB_LATE_REVERB_PAN, reverb->flLateReverbPan);
		palEffectf(effect, AL_EAXREVERB_ECHO_TIME, reverb->flEchoTime);
		palEffectf(effect, AL_EAXREVERB_ECHO_DEPTH, reverb->flEchoDepth);
		palEffectf(effect, AL_EAXREVERB_MODULATION_TIME, reverb->flModulationTime);
		palEffectf(effect, AL_EAXREVERB_MODULATION_DEPTH, reverb->flModulationDepth);
		palEffectf(effect, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, reverb->flAirAbsorptionGainHF);
		palEffectf(effect, AL_EAXREVERB_HFREFERENCE, reverb->flHFReference);
		palEffectf(effect, AL_EAXREVERB_LFREFERENCE, reverb->flLFReference);
		palEffectf(effect, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, reverb->flRoomRolloffFactor);
		palEffecti(effect, AL_EAXREVERB_DECAY_HFLIMIT, reverb->iDecayHFLimit);
	}
	else
#endif
	{
#ifdef AL_EFFECT_REVERB
		/* No EAX Reverb. Set the standard reverb effect type then load the
		 * available reverb properties. */
		palEffecti(effect, AL_EFFECT_TYPE, AL_EFFECT_REVERB);

		palEffectf(effect, AL_REVERB_DENSITY, reverb->flDensity);
		palEffectf(effect, AL_REVERB_DIFFUSION, reverb->flDiffusion);
		palEffectf(effect, AL_REVERB_GAIN, reverb->flGain);
		palEffectf(effect, AL_REVERB_GAINHF, reverb->flGainHF);
		palEffectf(effect, AL_REVERB_DECAY_TIME, reverb->flDecayTime);
		palEffectf(effect, AL_REVERB_DECAY_HFRATIO, reverb->flDecayHFRatio);
		palEffectf(effect, AL_REVERB_REFLECTIONS_GAIN, reverb->flReflectionsGain);
		palEffectf(effect, AL_REVERB_REFLECTIONS_DELAY, reverb->flReflectionsDelay);
		palEffectf(effect, AL_REVERB_LATE_REVERB_GAIN, reverb->flLateReverbGain);
		palEffectf(effect, AL_REVERB_LATE_REVERB_DELAY, reverb->flLateReverbDelay);
		palEffectf(effect, AL_REVERB_AIR_ABSORPTION_GAINHF, reverb->flAirAbsorptionGainHF);
		palEffectf(effect, AL_REVERB_ROOM_ROLLOFF_FACTOR, reverb->flRoomRolloffFactor);
		palEffecti(effect, AL_REVERB_DECAY_HFLIMIT, reverb->iDecayHFLimit);
#endif
	}
	return effect;
}
#endif

#ifdef HAVE_MIXER
#define CHUNKSAMPLES 1024
static void *OAQM_LockBuffer (soundcardinfo_t *sc, unsigned int *sampidx)
{
	oalinfo_t *oali = sc->handle;
	if (oali->qmix.queuesize==countof(oali->qmix.queue))
		return NULL;	//not available yet.
	return sc->sn.buffer;
}
static void OAQM_UnlockBuffer (soundcardinfo_t *sc, void *buffer)
{
}
static void OAQM_Submit (soundcardinfo_t *sc, int start, int end)
{
	oalinfo_t *oali = sc->handle;
	ALint buf;
	int framesize = sc->sn.samplebytes*sc->sn.numchannels;
	if (end==start)
		return;
	if (oali->qmix.queuesize == countof(oali->qmix.queue))
		return;
	palGenBuffers(1, &buf);
	switch(sc->sn.sampleformat)
	{
	case QSF_F32:
		palBufferData(buf, (sc->sn.numchannels>1)?AL_FORMAT_STEREO_FLOAT32:AL_FORMAT_MONO_FLOAT32, sc->sn.buffer, (end-start)*framesize, sc->sn.speed);
		break;
	case QSF_S16:
		palBufferData(buf, (sc->sn.numchannels>1)?AL_FORMAT_STEREO16:AL_FORMAT_MONO16, sc->sn.buffer, (end-start)*framesize, sc->sn.speed);
		break;
	case QSF_U8:
		palBufferData(buf, (sc->sn.numchannels>1)?AL_FORMAT_STEREO8:AL_FORMAT_MONO8, sc->sn.buffer, (end-start)*framesize, sc->sn.speed);
		break;
	default:
		break;
	}
	palSourceQueueBuffers(oali->qmix.handle, 1, &buf);
	oali->qmix.queue[oali->qmix.queuesize++] = buf;
	sc->snd_completed += (end-start);

	palGetSourcei(oali->qmix.handle, AL_SOURCE_STATE, &buf);
	if (buf != AL_PLAYING)
		palSourcePlay(oali->qmix.handle);
}

/*stub should not be called*/
static unsigned int OAQM_GetDMAPos (soundcardinfo_t *sc)
{
	extern cvar_t _snd_mixahead;
	oalinfo_t *oali = sc->handle;
	ALint src = oali->qmix.handle;
	ALint processed = 0;
	unsigned int avail;
	palGetSourcei(src, AL_BUFFERS_PROCESSED, &processed);
	if (processed)
	{
		palSourceUnqueueBuffers(src, processed, oali->qmix.queue);
		palDeleteBuffers(processed, oali->qmix.queue);
		oali->qmix.queuesize -= processed;
		memmove(oali->qmix.queue, oali->qmix.queue+processed, oali->qmix.queuesize*sizeof(*oali->qmix.queue));
	}

	avail = ((_snd_mixahead.value*sc->sn.speed)+CHUNKSAMPLES/2)/CHUNKSAMPLES;	//how many buffers we want to try using.
	avail = bound(2, avail, countof(oali->qmix.queue));
	if (oali->qmix.queuesize > avail)
		avail = 0;
	else
		avail = avail-oali->qmix.queuesize;
	avail *= CHUNKSAMPLES;

	sc->sn.samplepos = (sc->snd_completed+avail);
	sc->sn.samplepos *= sc->sn.numchannels;
	return sc->sn.samplepos;
}

static qboolean QDECL OpenAL_Enumerate_QMix(void (QDECL *callback)(const char *driver, const char *devicecode, const char *readabledevice))
{
	const char *devnames;
	if (!OpenAL_InitLibrary())
		return true; //enumerate nothing if al is disabled

	devnames = palcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
	if (!devnames)
		devnames = palcGetString(NULL, ALC_DEVICE_SPECIFIER);
	while(*devnames)
	{
		callback(QMIX_SDRVNAME, devnames, va(QMIX_SDRVNAMEDESC"%s", devnames));
		devnames += strlen(devnames)+1;
	}
	return true;
}
#endif

static qboolean QDECL OpenAL_Enumerate(void (QDECL *callback)(const char *driver, const char *devicecode, const char *readabledevice))
{
	const char *devnames;
	if (!OpenAL_InitLibrary())
		return true; //enumerate nothing if al is disabled

	devnames = palcGetString(NULL, ALC_ALL_DEVICES_SPECIFIER);
	if (!devnames)
		devnames = palcGetString(NULL, ALC_DEVICE_SPECIFIER);
	while(*devnames)
	{
		callback(SDRVNAME, devnames, va(SDRVNAMEDESC"%s", devnames));
		devnames += strlen(devnames)+1;
	}
	return true;
}


static qboolean QDECL OpenAL_InitCard2(soundcardinfo_t *sc, const char *devname, qboolean qmix)
{
	oalinfo_t *oali;

	soundcardinfo_t *old;
//	extern soundcardinfo_t *sndcardinfo;

	//
	for (old = sndcardinfo; old; old = old->next)
	{
		if (old->Shutdown == OpenAL_Shutdown)
		{
			//in theory, we could relax this by using alcMakeContextCurrent lots, but we'd also need to do something about the per-sound audio buffer handle hack
			Con_Printf(CON_ERROR SDRVNAME ": only a single device may be active at once\n");
			return false;
		}
	}


	if (OpenAL_Init(sc, devname, qmix) == false)
		return false;
	oali = sc->handle;

	Con_DPrintf( "AL_VERSION: %s\n",palGetString(AL_VERSION));
	Con_DPrintf( "AL_RENDERER: %s\n",palGetString(AL_RENDERER));
	Con_DPrintf( "AL_VENDOR: %s\n",palGetString(AL_VENDOR));
	Con_DPrintf("AL_EXTENSIONS: %s\n",palGetString(AL_EXTENSIONS));
	Con_DPrintf("ALC_EXTENSIONS: %s\n",palcGetString(oali->OpenAL_Device,ALC_EXTENSIONS));

#ifdef MIXER_F32
	oali->canfloataudio = palIsExtensionPresent("AL_EXT_float32");
#endif

	sc->inactive_sound = true;
	sc->Shutdown = OpenAL_Shutdown;
#ifdef HAVE_MIXER
	if (qmix)
	{
		sc->Lock		= OAQM_LockBuffer;
		sc->Unlock		= OAQM_UnlockBuffer;
		sc->GetDMAPos	= OAQM_GetDMAPos;
		sc->Submit		= OAQM_Submit;

		sc->sn.numchannels = bound(1, sc->sn.numchannels, 2);
		sc->sn.samples = CHUNKSAMPLES*sc->sn.numchannels;
#ifdef MIXER_F32
		if (sc->sn.samplebytes == 4 && oali->canfloataudio)
		{
			sc->sn.sampleformat = QSF_F32;
			sc->sn.samplebytes = 4;
		}
		else
#endif
		if (sc->sn.samplebytes > 1)
		{
			sc->sn.sampleformat = QSF_S16;
			sc->sn.samplebytes = 2;
		}
		else
		{
			sc->sn.sampleformat = QSF_U8;
			sc->sn.samplebytes = 1;
		}
//		sc->sn.speed = 11025;
		sc->sn.buffer = malloc(sc->sn.samples * sc->sn.samplebytes);
		sc->samplequeue = -1;

		oali->qmix.handle = 0;
		oali->qmix.queuesize = 0;
		palGenSources(1, &oali->qmix.handle);
		palSourcef(oali->qmix.handle, AL_GAIN, 1);
		palSourcei(oali->qmix.handle, AL_SOURCE_RELATIVE, AL_TRUE);
		//palSourcePlay(oali->qmix.handle);
	}
	else
#endif
	{
#ifdef USEEFX
		sc->SetEnvironmentReverb = OpenAL_SetReverb;
#endif
		sc->ChannelUpdate = OpenAL_ChannelUpdate;
		sc->ListenerUpdate = OpenAL_ListenerUpdate;
		sc->GetChannelPos = OpenAL_GetChannelPos;
		//these are stubs for our software mixer, and are not used with hardware mixing.
		sc->Lock = OpenAL_LockBuffer;
		sc->Unlock = OpenAL_UnlockBuffer;
		sc->Submit = OpenAL_Submit;
		sc->GetDMAPos = OpenAL_GetDMAPos;

		sc->selfpainting = true;
		sc->sn.sampleformat = QSF_EXTERNALMIXER;

		OnChangeALSettings(NULL, NULL);

#ifdef USEEFX
		PrintALError("preeffects");
	#ifndef AL_ALEXT_PROTOTYPES
		palAuxiliaryEffectSloti = palGetProcAddress("alAuxiliaryEffectSloti");
		palGenAuxiliaryEffectSlots = palGetProcAddress("alGenAuxiliaryEffectSlots");
		palDeleteAuxiliaryEffectSlots = palGetProcAddress("alDeleteAuxiliaryEffectSlots");
		palDeleteEffects = palGetProcAddress("alDeleteEffects");
		palGenEffects = palGetProcAddress("alGenEffects");
		palEffecti = palGetProcAddress("alEffecti");
//		palEffectiv = palGetProcAddress("alEffectiv");
		palEffectf = palGetProcAddress("alEffectf");
		palEffectfv = palGetProcAddress("alEffectfv");
	#endif

		if (palGenAuxiliaryEffectSlots && s_al_use_reverb.ival)
			palGenAuxiliaryEffectSlots(1, &oali->effectslot);

		oali->cureffect = ~0;
		PrintALError("posteffects");
#endif
	}
	return true;
}

static qboolean QDECL OpenAL_InitCard(soundcardinfo_t *sc, const char *devname)
{
	return OpenAL_InitCard2(sc, devname, false);
}

sounddriver_t OPENAL_Output =
{
	SDRVNAME,
	OpenAL_InitCard,
	OpenAL_Enumerate,
	OpenAL_CvarInit
};
#ifdef HAVE_MIXER
static qboolean QDECL OpenAL_InitCard_QMix(soundcardinfo_t *sc, const char *devname)
{
	return OpenAL_InitCard2(sc, devname, true);
}
sounddriver_t OPENAL_Output_Lame =
{
	QMIX_SDRVNAME,
	OpenAL_InitCard_QMix,
	OpenAL_Enumerate_QMix,
	NULL
};
#endif


#if defined(VOICECHAT)

static qboolean OpenAL_InitCapture(void)
{
	if (!OpenAL_InitLibrary())
		return false;

	//is there really much point checking for the name when the functions should exist or not regardless?
	//if its not really supported, I would trust the open+enumerate operations to reliably fail. the functions are exported as actual symbols after all, not some hidden driver feature.
	//it doesn't really matter if the default driver supports it, so long as one does, I guess.
	//if (!palcIsExtensionPresent(NULL, "ALC_EXT_capture"))
	//	return false;

#ifdef OPENAL_STATIC
	return true;
#else
	if(!palcCaptureOpenDevice)
	{
		palcCaptureOpenDevice = Sys_GetAddressForName(openallib, "alcCaptureOpenDevice");
		palcCaptureStart = Sys_GetAddressForName(openallib, "alcCaptureStart");
		palcCaptureSamples = Sys_GetAddressForName(openallib, "alcCaptureSamples");
		palcCaptureStop = Sys_GetAddressForName(openallib, "alcCaptureStop");
		palcCaptureCloseDevice = Sys_GetAddressForName(openallib, "alcCaptureCloseDevice");
	}

	return palcGetIntegerv&&palcCaptureOpenDevice&&palcCaptureStart&&palcCaptureSamples&&palcCaptureStop&&palcCaptureCloseDevice;
#endif
}
static qboolean QDECL OPENAL_Capture_Enumerate (void (QDECL *callback) (const char *drivername, const char *devicecode, const char *readablename))
{
	const char *devnames;
	if (!OpenAL_InitCapture())
		return true; //enumerate nothing if al is disabled

	devnames = palcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
	while(*devnames)
	{
		callback(SDRVNAME, devnames, va(SDRVNAMEDESC"%s", devnames));
		devnames += strlen(devnames)+1;
	}
	return true;
}
//fte's capture api specifies mono 16.
static void *QDECL OPENAL_Capture_Init (int samplerate, const char *device)
{
#ifndef OPENAL_STATIC
	if (!device)	//no default devices please, too buggy for that.
		return NULL;
#endif

	if (!OpenAL_InitCapture())
		return NULL; //enumerate nothing if al is disabled

	if (!device || !*device)
	{
#if defined(FTE_TARGET_WEB) && (__EMSCRIPTEN_major__>2 || (__EMSCRIPTEN_major__==2&&__EMSCRIPTEN_tiny__>=14))
		//emscripten, and recent enough to actually work. don't check s_al_disable here as we don't have dsound/alsa fallbacks and we do want to actually use it.
		//older versions of emscripten are too buggy to use.
#else
		if (s_al_disable.ival)
			return NULL;	//no default device
#endif
		device = palcGetString(NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);
	}

	return palcCaptureOpenDevice(device, samplerate, AL_FORMAT_MONO16, 0.5*samplerate);
}
static void QDECL OPENAL_Capture_Start (void *ctx)
{
	ALCdevice *device = ctx;
	palcCaptureStart(device);
}
static unsigned int QDECL OPENAL_Capture_Update (void *ctx, unsigned char *buffer, unsigned int minbytes, unsigned int maxbytes)
{
#define samplesize sizeof(short)
	ALCdevice *device = ctx;
	int avail = 0;
	palcGetIntegerv(device, ALC_CAPTURE_SAMPLES, sizeof(ALint), &avail);
	if (avail*samplesize < minbytes)
		return 0;	//don't bother grabbing it if its below the threshold.
	palcCaptureSamples(device, (ALCvoid *)buffer, avail);
	return avail * samplesize;
}
static void QDECL OPENAL_Capture_Stop (void *ctx)
{
	ALCdevice *device = ctx;
	palcCaptureStop(device);
}
static void QDECL OPENAL_Capture_Shutdown (void *ctx)
{
	ALCdevice *device = ctx;
	palcCaptureCloseDevice(device);
}

snd_capture_driver_t OPENAL_Capture =
{
	1,
	SDRVNAME,
	OPENAL_Capture_Enumerate,
	OPENAL_Capture_Init,
	OPENAL_Capture_Start,
	OPENAL_Capture_Update,
	OPENAL_Capture_Stop,
	OPENAL_Capture_Shutdown
};
#endif

#endif
